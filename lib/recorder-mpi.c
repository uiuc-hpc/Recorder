/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted for any purpose (including commercial purposes)
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions, and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions, and the following disclaimer in the documentation
 *    and/or materials provided with the distribution.
 *
 * 3. In addition, redistributions of modified forms of the source or binary
 *    code must carry prominent notices stating that the original code was
 *    changed and the date of the change.
 *
 * 4. All publications or advertising materials mentioning features or use of
 *    this software are asked, but not required, to acknowledge that it was
 *    developed by The HDF Group and by the National Center for Supercomputing
 *    Applications at the University of Illinois at Urbana-Champaign and
 *    credit the contributors.
 *
 * 5. Neither the name of The HDF Group, the name of the University, nor the
 *    name of any Contributor may be used to endorse or promote products derived
 *    from this software without specific prior written permission from
 *    The HDF Group, the University, or the Contributor, respectively.
 *
 * DISCLAIMER:
 * THIS SOFTWARE IS PROVIDED BY THE HDF GROUP AND THE CONTRIBUTORS
 * "AS IS" WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED. In no
 * event shall The HDF Group or the Contributors be liable for any damages
 * suffered by the users arising out of the use of this software, even if
 * advised of the possibility of such damage.
 *
 * Portions of Recorder were developed with support from the Lawrence Berkeley
 * National Laboratory (LBNL) and the United States Department of Energy under
 * Prime Contract No. DE-AC02-05CH11231.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE /* for tdestroy() */

#define __D_MPI_REQUEST MPIO_Request

#include <stdio.h>
#ifdef HAVE_MNTENT_H
#include <mntent.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <zlib.h>
#include <assert.h>
#include <search.h>
#include <alloca.h>

#include "mpi.h"
#include "recorder.h"

#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif

#define RECORDER_MPI_IMP(func) imp_##func

typedef struct MPICommHash_t {
    void *key;      // MPI_Comm as key
    char* id;
    UT_hash_handle hh;
} MPICommHash;

typedef struct MPIFileHash_t {
    void* key;
    char* id;
    int   accept;   // Should we intercept calls for this file
    UT_hash_handle hh;
} MPIFileHash;

static MPICommHash *mpi_comm_table = NULL;
static MPIFileHash *mpi_file_table = NULL;
static int mpi_file_id = 0;
static int mpi_comm_id = 0;

// placeholder for C wrappers
static MPI_Fint* ierr = NULL;


/* Our own bcast call during the tracing process
 * it creates a tmp comm to perform the bcast
 * this avoids interfering with applicaiton's
 * bcast calls.
 */
void safe_bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    MPI_Comm tmp_comm;
    PMPI_Comm_dup(comm, &tmp_comm);
    PMPI_Bcast(buffer, count, datatype, root, tmp_comm);
    PMPI_Comm_free(&tmp_comm);
}

void add_mpi_file(MPI_Comm comm, MPI_File *file, CONST char* filename) {
    if(file == NULL)
        return;

    int rank, world_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    PMPI_Comm_rank(comm, &rank);

    MPIFileHash *entry = malloc(sizeof(MPIFileHash));
    entry->key = malloc(sizeof(MPI_File));
    memcpy(entry->key, file, sizeof(MPI_File));

    char* id = calloc(32, sizeof(char));
    if(rank == 0)
        sprintf(id, "%d-%d", world_rank, mpi_file_id++);
    safe_bcast(id, 32, MPI_BYTE, 0, comm);
    entry->id = id;
    char* tmp_filename = realrealpath(filename);
    entry->accept = accept_filename(tmp_filename);
    free(tmp_filename);

    HASH_ADD_KEYPTR(hh, mpi_file_table, entry->key, sizeof(MPI_File), entry);
}

char* file2id(MPI_File *file) {
    if(file == NULL)
        return strdup("MPI_FILE_NULL");
    else {
        MPIFileHash *entry = NULL;
        HASH_FIND(hh, mpi_file_table, file, sizeof(MPI_File), entry);
        if(entry)
            return strdup(entry->id);
        else
            return strdup("MPI_FILE_UNKNOWN");
    }
}

/*
 * Check if we should intercept an MPI-IO call according
 * to the inclusion or exclusion file list
 */
#define FILTER_MPIIO_CALL(func, func_args, fh)                      \
    MPIFileHash *entry = NULL;                                      \
    HASH_FIND(hh, mpi_file_table, fh, sizeof(MPI_File), entry);     \
    if(!entry || !entry->accept) {                                  \
        MAP_OR_FAIL(func);                                          \
        return RECORDER_REAL_CALL(func) func_args;                  \
    }


// Return the relative rank in the new communicator
int add_mpi_comm(MPI_Comm *newcomm) {
    if(newcomm == NULL || *newcomm == MPI_COMM_NULL)
        return - 1;
    int new_rank, world_rank;
    PMPI_Comm_rank(*newcomm, &new_rank);
    PMPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    MPICommHash *entry = malloc(sizeof(MPICommHash));
    entry->key = malloc(sizeof(MPI_Comm));
    memcpy(entry->key, newcomm, sizeof(MPI_Comm));

    // Rank 0 of the new communicator decides an unique id
    // and broadcast it to all others, then everyone stores it
    // in the hash table.
    char *id = calloc(32, sizeof(char));
    if(new_rank == 0)
        sprintf(id, "%d-%d", world_rank, mpi_comm_id++);
    safe_bcast(id, 32, MPI_BYTE, 0, *newcomm);
    entry->id = id;

    HASH_ADD_KEYPTR(hh, mpi_comm_table, entry->key, sizeof(MPI_Comm), entry);
    return new_rank;
}

char* comm2name(MPI_Comm *comm) {
    if(comm == NULL || *comm == MPI_COMM_NULL)
        return strdup("MPI_COMM_NULL");
    else if(*comm == MPI_COMM_WORLD) {
        return strdup("MPI_COMM_WORLD");
    } else if(*comm == MPI_COMM_SELF) {
        return strdup("MPI_COMM_SELF");
    } else {
        MPICommHash *entry = NULL;
        HASH_FIND(hh, mpi_comm_table, comm, sizeof(MPI_Comm), entry);
        if(entry)
            return strdup(entry->id);
        else
            return strdup("MPI_COMM_UNKNOWN");
    }
}

static inline char *type2name(MPI_Datatype type) {
    char *tmp = malloc(128);
    if(type == MPI_DATATYPE_NULL) {
        strcpy(tmp, "MPI_DATATYPE_NULL");
    } else {
        int len;
        PMPI_Type_get_name(type, tmp, &len);
        tmp[len] = 0;
        if(len == 0)
            strcpy(tmp, "MPI_TYPE_UNKNOWN");
    }
    return tmp;
}

static inline char* status2str(MPI_Status *status) {
    char *tmp = calloc(128, sizeof(char));
    /*
    if(status == MPI_STATUS_IGNORE)
        strcpy(tmp, "MPI_STATUS_IGNORE");
    else
        sprintf(tmp, "[%d %d]", status->MPI_SOURCE, status->MPI_TAG);
    */
    // CHEN MPI-IO calls return status that may have wierd status->MPI_SOURCE,
    // affecting compressing grammars across ranks
    memset(tmp, 0, 128);
    return tmp;
}

static inline char* whence2name(int whence) {
    if(whence == MPI_SEEK_SET)
        return strdup("MPI_SEEK_SET");
    if(whence == MPI_SEEK_CUR)
        return strdup("MPI_SEEK_CUR");
    if(whence == MPI_SEEK_END)
        return strdup("MPI_SEEK_END");
}

/**
 * For MPI_Wait and MPI_Test we always fill in MPI_Status
 * even if user passed MPI_STATUS_IGNORE.
 *
 * The purpose is that later when we match MPI calls
 * we are sure we can find out the recveive src/tag.
 * even in this case:
 *      Irecv(ang_source) + MPI_Wait(MPI_STATUS_IGNORE);
 */




/**
 * Intercept the following functions
 */
int RECORDER_MPI_IMP(MPI_Comm_size) (MPI_Comm comm, int *size, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_size, (comm, size), ierr);
    char **args = assemble_args_list(2, comm2name(&comm), itoa(*size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Comm_rank) (MPI_Comm comm, int *rank, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_rank, (comm, rank), ierr);
    char **args = assemble_args_list(2, comm2name(&comm), itoa(*rank));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Get_processor_name) (char *name, int *resultlen, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Get_processor_name, (name, resultlen), ierr);
    char **args = assemble_args_list(2, ptoa(name), ptoa(resultlen));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Comm_set_errhandler) (MPI_Comm comm, MPI_Errhandler errhandler, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_set_errhandler, (comm, errhandler), ierr);
    char **args = assemble_args_list(2, comm2name(&comm), ptoa(&errhandler));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Barrier) (MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Barrier, (comm), ierr);
    char **args = assemble_args_list(1, comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_MPI_IMP(MPI_Bcast) (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Bcast, (buffer, count, datatype, root, comm), ierr);
    char **args = assemble_args_list(5, ptoa(buffer), itoa(count), type2name(datatype), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_Ibcast) (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Ibcast, (buffer, count, datatype, root, comm, request), ierr);
    size_t r = *request;
    char **args = assemble_args_list(6, ptoa(buffer), itoa(count), type2name(datatype), itoa(root), comm2name(&comm), itoa(r));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_Gather) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Gather, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm), ierr);
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

int RECORDER_MPI_IMP(MPI_Scatter) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Scatter, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm), ierr);
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

int RECORDER_MPI_IMP(MPI_Gatherv) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Fint* ierr) {
    // TODO: *displs
    /* TODO: ugly fix for mpich fortran wrapper
     * when (stype == MPI_DATATYPE_NULL) and (sbuf != MPI_INPLACE)
     * mpich will abort and through a null datatype error.
     * the fortran MPI_IN_PLACE has a different value than that of C.
     * so if this was invoked by the fortran wrapper, we never see
     * sbuf == MPI_IN_PLACE.
     * in that case, we scerectly set stype to MPI_INT when scount = 0.
     */
    MPI_Datatype sstype = stype;
    if(stype == MPI_DATATYPE_NULL && sbuf != MPI_IN_PLACE && scount == 0) {
        sstype = MPI_INT;
    }

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Gatherv, (sbuf, scount, sstype, rbuf, rcount, displs, rtype, root, comm), ierr);
    char **args = assemble_args_list(9, ptoa(sbuf), itoa(scount), type2name(stype), ptoa(rbuf),
                                        ptoa(rcount), ptoa(displs), type2name(rtype), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}

int RECORDER_MPI_IMP(MPI_Scatterv) (CONST void *sbuf, CONST int *scount, CONST int *displa, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Scatterv, (sbuf, scount, displa, stype, rbuf, rcount, rtype, root, comm), ierr);
    char **args = assemble_args_list(9, ptoa(sbuf), ptoa(scount), ptoa(displa), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);

}

int RECORDER_MPI_IMP(MPI_Allgather) (CONST void* sbuf, int scount, MPI_Datatype stype, void* rbuf, CONST int rcount, MPI_Datatype rtype, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Allgather, (sbuf, scount, stype, rbuf, rcount, rtype, comm), ierr);
    char **args = assemble_args_list(7, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int RECORDER_MPI_IMP(MPI_Allgatherv) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, MPI_Comm comm, MPI_Fint* ierr) {
    // TODO: displs
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Allgatherv, (sbuf, scount, stype, rbuf, rcount, displs, rtype, comm), ierr);
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), ptoa(rcount), ptoa(displs), type2name(rtype), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);

}

int RECORDER_MPI_IMP(MPI_Alltoall) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Alltoall, (sbuf, scount, stype, rbuf, rcount, rtype, comm), ierr);
    char **args = assemble_args_list(7, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);

}

int RECORDER_MPI_IMP(MPI_Reduce) (CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, int root, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Reduce, (sbuf, rbuf, count, stype, op, root, comm), ierr);
    char **args = assemble_args_list(7, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype),
                                    itoa(op), itoa(root), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

int RECORDER_MPI_IMP(MPI_Allreduce) (CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Allreduce, (sbuf, rbuf, count, stype, op, comm), ierr);
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype), itoa(op), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_Reduce_scatter) (CONST void *sbuf, void *rbuf, CONST int *rcounts, MPI_Datatype stype, MPI_Op op, MPI_Comm comm, MPI_Fint* ierr) {
    // TODO: *rcounts
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Reduce_scatter, (sbuf, rbuf, rcounts, stype, op, comm), ierr);
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), ptoa(rcounts), type2name(stype), itoa(op), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_Scan) (CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Scan, (sbuf, rbuf, count, stype, op, comm), ierr);
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype), itoa(op), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_Type_create_darray) (int size, int rank, int ndims, CONST int array_of_gsizes[], CONST int array_of_distribs[], CONST int array_of_dargs[], CONST int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Type_create_darray, (size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype), ierr);
    char **args = assemble_args_list(10, itoa(size), itoa(rank), itoa(ndims), ptoa(array_of_gsizes), ptoa(array_of_distribs),
                            ptoa(array_of_dargs), ptoa(array_of_psizes), itoa(order), type2name(oldtype), ptoa(newtype));
    RECORDER_INTERCEPTOR_EPILOGUE(10, args);
}

int RECORDER_MPI_IMP(MPI_Type_commit) (MPI_Datatype *datatype, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Type_commit, (datatype), ierr);
    char **args = assemble_args_list(1, ptoa(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_MPI_IMP(MPI_File_open) (MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_open, (comm, filename, amode, info, fh), ierr);
    add_mpi_file(comm, fh, filename);
    // TODO incorporate FILTER_MPIIO_CALL here
    char **args = assemble_args_list(5, comm2name(&comm), realrealpath(filename), itoa(amode), ptoa(&info), file2id(fh));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_close) (MPI_File *fh, MPI_Fint* ierr) {
    char* fid = file2id(fh);
    MPIFileHash *entry = NULL;
    HASH_FIND(hh, mpi_file_table, fh, sizeof(MPI_File), entry);
    if(entry) {
        HASH_DEL(mpi_file_table, entry);
        free(entry->id);
        free(entry->key);
        free(entry);
    }
    // TODO incorporate FILTER_MPIIO_CALL here

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_close, (fh), ierr);
    char **args = assemble_args_list(1, fid);
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_MPI_IMP(MPI_File_sync) (MPI_File fh, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_sync, (fh), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_sync, (fh), ierr);
    char **args = assemble_args_list(1, file2id(&fh));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_MPI_IMP(MPI_File_set_size) (MPI_File fh, MPI_Offset size, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_set_size, (fh, size), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_set_size, (fh, size), ierr);
    char **args = assemble_args_list(2, file2id(&fh), itoa(size));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_File_set_view) (MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, CONST char *datarep, MPI_Info info, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_set_view, (fh, disp, etype, filetype, datarep, info), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_set_view, (fh, disp, etype, filetype, datarep, info), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(disp), type2name(etype), type2name(filetype), ptoa(datarep), ptoa(&info));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_read) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_read_at) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_at, (fh, offset, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_at, (fh, offset, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_read_at_all) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_at_all, (fh, offset, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_at_all, (fh, offset, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_read_all) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_all, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_all, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_read_shared) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_shared, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_shared, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_read_ordered) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_ordered, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_ordered, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_read_at_all_begin) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_at_all_begin, (fh, offset, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_at_all_begin, (fh, offset, buf, count, datatype), ierr);
    char **args = assemble_args_list(5, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_read_all_begin) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_all_begin, (fh, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_all_begin, (fh, buf, count, datatype), ierr);
    char **args = assemble_args_list(4, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_File_read_ordered_begin) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_read_ordered_begin, (fh, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_read_ordered_begin, (fh, buf, count, datatype), ierr);
    char **args = assemble_args_list(4, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_File_iread_at) (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iread_at, (fh, offset, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iread_at, (fh, offset, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_iread) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iread, (fh, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iread, (fh, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_iread_shared) (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iread_shared, (fh, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iread_shared, (fh, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_write) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

static MPI_Offset g_prev_offset = 0;
int RECORDER_MPI_IMP(MPI_File_write_at) (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_at, (fh, offset, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_at, (fh, offset, buf, count, datatype, status), ierr);

    MPI_Offset offset_delta = offset;

    // CHEN
    /*
    offset_delta = offset - g_prev_offset;
    if(offset_delta <= 0)
        offset_delta = offset;
    g_prev_offset = offset;
    */

    char **args = assemble_args_list(6, file2id(&fh), itoa(offset_delta), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_write_at_all) (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_at_all, (fh, offset, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_at_all, (fh, offset, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_write_all) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_all, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_all, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_write_shared) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_shared, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_shared, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_write_ordered) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_ordered, (fh, buf, count, datatype, status), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_ordered, (fh, buf, count, datatype, status), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_write_at_all_begin) (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_at_all_begin, (fh, offset, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_at_all_begin, (fh, offset, buf, count, datatype), ierr);
    char **args = assemble_args_list(5, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_write_all_begin) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_all_begin, (fh, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_all_begin, (fh, buf, count, datatype), ierr);
    char **args = assemble_args_list(4, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_File_write_ordered_begin) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_write_ordered_begin, (fh, buf, count, datatype), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_write_ordered_begin, (fh, buf, count, datatype), ierr);
    char **args = assemble_args_list(4, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_File_iwrite_at) (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iwrite_at, (fh, offset, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iwrite_at, (fh, offset, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(6, file2id(&fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_IMP(MPI_File_iwrite) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iwrite, (fh, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iwrite, (fh, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_iwrite_shared) (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_iwrite_shared, (fh, buf, count, datatype, request), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_iwrite_shared, (fh, buf, count, datatype, request), ierr);
    char **args = assemble_args_list(5, file2id(&fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_File_seek) (MPI_File fh, MPI_Offset offset, int whence, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_seek, (fh, offset, whence), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_seek, (fh, offset, whence), ierr);
    char **args = assemble_args_list(3, file2id(&fh), itoa(offset), whence2name(whence));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int RECORDER_MPI_IMP(MPI_File_seek_shared) (MPI_File fh, MPI_Offset offset, int whence, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_seek_shared, (fh, offset, whence), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_seek_shared, (fh, offset, whence), ierr);
    char **args = assemble_args_list(3, file2id(&fh), itoa(offset), whence2name(whence));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}

int RECORDER_MPI_IMP(MPI_File_get_size) (MPI_File fh, MPI_Offset *offset, MPI_Fint* ierr) {
    FILTER_MPIIO_CALL(PMPI_File_get_size, (fh, offset), &fh);
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_File_get_size, (fh, offset), ierr);
    char **args = assemble_args_list(2, file2id(&fh), itoa(*offset));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Finalized) (int *flag, MPI_Fint* ierr) {
    // TODO: flag
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Finalized, (flag), ierr);
    char **args = assemble_args_list(1, ptoa(flag));
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

// Added 10 new MPI funcitons on 2019/01/07
int RECORDER_MPI_IMP(MPI_Cart_rank) (MPI_Comm comm, CONST int coords[], int *rank, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Cart_rank, (comm, coords, rank), ierr);
    char **args = assemble_args_list(3, comm2name(&comm), ptoa(coords), ptoa(rank));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}
int RECORDER_MPI_IMP(MPI_Cart_create) (MPI_Comm comm_old, int ndims, CONST int dims[], CONST int periods[], int reorder, MPI_Comm *comm_cart, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Cart_create, (comm_old, ndims, dims, periods, reorder, comm_cart), ierr);
    int newrank = add_mpi_comm(comm_cart);
    char **args = assemble_args_list(7, comm2name(&comm_old), itoa(ndims), ptoa(dims), ptoa(periods), itoa(reorder), comm2name(comm_cart), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}
int RECORDER_MPI_IMP(MPI_Cart_get) (MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[], MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Cart_get, (comm, maxdims, dims, periods, coords), ierr);
    char **args = assemble_args_list(5, comm2name(&comm), itoa(maxdims), ptoa(dims), ptoa(periods), ptoa(coords));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}
int RECORDER_MPI_IMP(MPI_Cart_shift) (MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Cart_shift, (comm, direction, disp, rank_source, rank_dest), ierr);
    char **args = assemble_args_list(5, comm2name(&comm), itoa(direction), itoa(disp), ptoa(rank_source), ptoa(rank_dest));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}
int RECORDER_MPI_IMP(MPI_Wait) (MPI_Request *request, MPI_Status *status, MPI_Fint* ierr) {
    size_t r = *request;
    MPI_Status *status_p = (status==MPI_STATUS_IGNORE) ? alloca(sizeof(MPI_Status)) : status;
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Wait, (request, status_p), ierr);
    char** args = assemble_args_list(2, itoa(r), status2str(status_p));
    RECORDER_INTERCEPTOR_EPILOGUE(2, args);
}

int RECORDER_MPI_IMP(MPI_Send) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Send, (buf, count, datatype, dest, tag, comm), ierr);
    char **args = assemble_args_list(6, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}
int RECORDER_MPI_IMP(MPI_Recv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Recv, (buf, count, datatype, source, tag, comm, status), ierr);
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(source), itoa(tag), comm2name(&comm), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}
int RECORDER_MPI_IMP(MPI_Sendrecv) (CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Sendrecv, (sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status), ierr);
    char **args = assemble_args_list(12, ptoa(sendbuf), itoa(sendcount), type2name(sendtype), itoa(dest), itoa(sendtag), ptoa(recvbuf), itoa(recvcount), type2name(recvtype),
                                        itoa(source), itoa(recvtag), comm2name(&comm), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(12, args);
}

int RECORDER_MPI_IMP(MPI_Isend) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Isend, (buf, count, datatype, dest, tag, comm, request), ierr);
    size_t r = *request;
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(&comm), itoa(r));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}
int RECORDER_MPI_IMP(MPI_Irecv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Irecv, (buf, count, datatype, source, tag, comm, request), ierr);
    size_t r = *request;
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(source), itoa(tag), comm2name(&comm), itoa(r));
    RECORDER_INTERCEPTOR_EPILOGUE(7, args);
}

// Add MPI_Waitall, MPI_Waitsome, MPI_Waitany and MPI_Ssend on 2020/08/06
int RECORDER_MPI_IMP(MPI_Waitall) (int count, MPI_Request requests[], MPI_Status statuses[], MPI_Fint* ierr) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Waitall, (count, requests, statuses), ierr);
    char **args = assemble_args_list(3, itoa(count), requests_str, ptoa(statuses));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}
int RECORDER_MPI_IMP(MPI_Waitsome) (int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[], MPI_Fint* ierr) {
    int i;
    size_t arr[incount];
    for(i = 0; i < incount; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, incount);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Waitsome, (incount, requests, outcount, indices, statuses), ierr);
    size_t arr2[*outcount];
    for(i = 0; i < *outcount; i++)
        arr2[i] = (size_t) indices[i];
    char* indices_str = arrtoa(arr2, *outcount);
    char **args = assemble_args_list(5, itoa(incount), requests_str, itoa(*outcount), indices_str, ptoa(statuses));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}
int RECORDER_MPI_IMP(MPI_Waitany) (int count, MPI_Request requests[], int *indx, MPI_Status *status, MPI_Fint* ierr) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Waitany, (count, requests, indx, status), ierr);
    char **args = assemble_args_list(4, itoa(count), requests_str, itoa(*indx), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_Ssend) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Ssend, (buf, count, datatype, dest, tag, comm), ierr);
    char **args = assemble_args_list(6, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(&comm));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}


int RECORDER_MPI_IMP(MPI_Comm_split) (MPI_Comm comm, int color, int key, MPI_Comm *newcomm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_split, (comm, color, key, newcomm), ierr);
    int newrank = add_mpi_comm(newcomm);
    char **args = assemble_args_list(5, comm2name(&comm), itoa(color), itoa(key), comm2name(newcomm), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_Comm_create) (MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_create, (comm, group, newcomm), ierr);
    int newrank = add_mpi_comm(newcomm);
    char **args = assemble_args_list(4, comm2name(&comm), itoa(group), comm2name(newcomm), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_Comm_dup) (MPI_Comm comm, MPI_Comm *newcomm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_dup, (comm, newcomm), ierr);
    int newrank = add_mpi_comm(newcomm);
    char **args = assemble_args_list(3, comm2name(&comm), comm2name(newcomm), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}



// Add MPI_Test, MPI_Testany, MPI_Testsome, MPI_Testall,
// MPI_Ireduce and MPI_Igather on 2020 12/18
int RECORDER_MPI_IMP(MPI_Test) (MPI_Request *request, int *flag, MPI_Status *status, MPI_Fint* ierr) {
    size_t r = *request;
    MPI_Status *status_p = (status==MPI_STATUS_IGNORE) ? alloca(sizeof(MPI_Status)) : status;
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Test, (request, flag, status_p), ierr);
    char **args = assemble_args_list(3, itoa(r), itoa(*flag), status2str(status_p));
    RECORDER_INTERCEPTOR_EPILOGUE(3, args);
}
int RECORDER_MPI_IMP(MPI_Testall) (int count, MPI_Request requests[], int *flag, MPI_Status statuses[], MPI_Fint* ierr) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Testall, (count, requests, flag, statuses), ierr);
    char **args = assemble_args_list(4, itoa(count), requests_str, itoa(*flag), ptoa(statuses));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}
int RECORDER_MPI_IMP(MPI_Testsome) (int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[], MPI_Fint* ierr) {
    int i;
    size_t arr[incount];
    for(i = 0; i < incount; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, incount);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Testsome, (incount, requests, outcount, indices, statuses), ierr);
    size_t arr2[*outcount];
    for(i = 0; i < *outcount; i++)
        arr2[i] = (size_t) indices[i];
    char* indices_str = arrtoa(arr2, *outcount);
    char **args = assemble_args_list(5, itoa(incount), requests_str, itoa(*outcount), indices_str, ptoa(statuses));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}
int RECORDER_MPI_IMP(MPI_Testany) (int count, MPI_Request requests[], int *indx, int *flag, MPI_Status *status, MPI_Fint* ierr) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Testany, (count, requests, indx, flag, status), ierr);
    char **args = assemble_args_list(5, itoa(count), requests_str, itoa(*indx), itoa(*flag), status2str(status));
    RECORDER_INTERCEPTOR_EPILOGUE(5, args);
}

int RECORDER_MPI_IMP(MPI_Ireduce) (CONST void *sbuf, void *rbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Ireduce, (sbuf, rbuf, count, datatype, op, root, comm, request), ierr);
    char **args = assemble_args_list(8, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(datatype),
                                    itoa(op), itoa(root), comm2name(&comm), itoa(*request));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}
int RECORDER_MPI_IMP(MPI_Igather) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Igather, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm, request), ierr);
    char **args = assemble_args_list(9, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(&comm), itoa(*request));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}
int RECORDER_MPI_IMP(MPI_Iscatter) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm, MPI_Request *request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Iscatter, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm, request), ierr);
    char **args = assemble_args_list(9, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(&comm), itoa(*request));
    RECORDER_INTERCEPTOR_EPILOGUE(9, args);
}
int RECORDER_MPI_IMP(MPI_Ialltoall) (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm, MPI_Request * request, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Ialltoall, (sbuf, scount, stype, rbuf, rcount, rtype, comm, request), ierr);
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), comm2name(&comm), itoa(*request));
    RECORDER_INTERCEPTOR_EPILOGUE(8, args);
}

// Add MPI_Comm_Free on 2021/01/25
int RECORDER_MPI_IMP(MPI_Comm_free) (MPI_Comm *comm, MPI_Fint* ierr) {
    char* comm_name = comm2name(comm);
    MPICommHash *entry = NULL;
    HASH_FIND(hh, mpi_comm_table, comm, sizeof(MPI_Comm), entry);
    if(entry) {
        HASH_DEL(mpi_comm_table, entry);
        free(entry->key);
        free(entry->id);
        free(entry);
    }

    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_free, (comm), ierr);
    char **args = assemble_args_list(1, comm_name);
    RECORDER_INTERCEPTOR_EPILOGUE(1, args);
}

int RECORDER_MPI_IMP(MPI_Cart_sub) (MPI_Comm comm, CONST int remain_dims[], MPI_Comm *newcomm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Cart_sub, (comm, remain_dims, newcomm), ierr);
    int newrank = add_mpi_comm(newcomm);
    char **args = assemble_args_list(4, comm2name(&comm), ptoa(remain_dims), comm2name(newcomm), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(4, args);
}

int RECORDER_MPI_IMP(MPI_Comm_split_type) (MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm, MPI_Fint* ierr) {
    RECORDER_INTERCEPTOR_PROLOGUE_F(int, PMPI_Comm_split_type, (comm, split_type, key, info, newcomm), ierr);
    int newrank = add_mpi_comm(newcomm);
    char **args = assemble_args_list(6, comm2name(&comm), itoa(split_type), itoa(key), ptoa(&info), comm2name(newcomm), itoa(newrank));
    RECORDER_INTERCEPTOR_EPILOGUE(6, args);
}

int RECORDER_MPI_DECL(MPI_Comm_size)(MPI_Comm comm, int *size) { return imp_MPI_Comm_size(comm, size, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_size)(MPI_Fint* comm, int* size, MPI_Fint *ierr){ imp_MPI_Comm_size(PMPI_Comm_f2c(*comm), size, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_size_)(MPI_Fint* comm, int* size, MPI_Fint *ierr){ imp_MPI_Comm_size(PMPI_Comm_f2c(*comm), size, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_size__)(MPI_Fint* comm, int* size, MPI_Fint *ierr){ imp_MPI_Comm_size(PMPI_Comm_f2c(*comm), size, ierr);}
int RECORDER_MPI_DECL(MPI_Comm_rank)(MPI_Comm comm, int *rank) { return imp_MPI_Comm_rank(comm, rank, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_rank)(MPI_Fint* comm, int* rank, MPI_Fint *ierr){ imp_MPI_Comm_rank(PMPI_Comm_f2c(*comm), rank, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_rank_)(MPI_Fint* comm, int* rank, MPI_Fint *ierr){ imp_MPI_Comm_rank(PMPI_Comm_f2c(*comm), rank, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_rank__)(MPI_Fint* comm, int* rank, MPI_Fint *ierr){ imp_MPI_Comm_rank(PMPI_Comm_f2c(*comm), rank, ierr);}
int RECORDER_MPI_DECL(MPI_Get_processor_name)(char *name, int *resultlen) { return imp_MPI_Get_processor_name(name, resultlen, ierr); }
extern void RECORDER_MPI_DECL(mpi_get_processor_name)(char* name, int* resultlen, MPI_Fint *ierr){ imp_MPI_Get_processor_name(name, resultlen, ierr);}
extern void RECORDER_MPI_DECL(mpi_get_processor_name_)(char* name, int* resultlen, MPI_Fint *ierr){ imp_MPI_Get_processor_name(name, resultlen, ierr);}
extern void RECORDER_MPI_DECL(mpi_get_processor_name__)(char* name, int* resultlen, MPI_Fint *ierr){ imp_MPI_Get_processor_name(name, resultlen, ierr);}
int RECORDER_MPI_DECL(MPI_Comm_set_errhandler)(MPI_Comm comm, MPI_Errhandler errhandler) { return imp_MPI_Comm_set_errhandler(comm, errhandler, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_set_errhandler)(MPI_Fint* comm, MPI_Fint* errhandler, MPI_Fint *ierr){ imp_MPI_Comm_set_errhandler(PMPI_Comm_f2c(*comm), PMPI_Errhandler_f2c(*errhandler), ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_set_errhandler_)(MPI_Fint* comm, MPI_Fint* errhandler, MPI_Fint *ierr){ imp_MPI_Comm_set_errhandler(PMPI_Comm_f2c(*comm), PMPI_Errhandler_f2c(*errhandler), ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_set_errhandler__)(MPI_Fint* comm, MPI_Fint* errhandler, MPI_Fint *ierr){ imp_MPI_Comm_set_errhandler(PMPI_Comm_f2c(*comm), PMPI_Errhandler_f2c(*errhandler), ierr);}
int RECORDER_MPI_DECL(MPI_Barrier)(MPI_Comm comm) { return imp_MPI_Barrier(comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_barrier)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Barrier(PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_barrier_)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Barrier(PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_barrier__)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Barrier(PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Bcast)(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) { return imp_MPI_Bcast(buffer, count, datatype, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_bcast)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Bcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_bcast_)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Bcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_bcast__)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Bcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Ibcast)(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request) { return imp_MPI_Ibcast(buffer, count, datatype, root, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_ibcast)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ibcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ibcast_)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ibcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ibcast__)(void* buffer, int* count, MPI_Fint* datatype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ibcast(buffer, (*count), PMPI_Type_f2c(*datatype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Gather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) { return imp_MPI_Gather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_gather)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_gather_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_gather__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Scatter)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) { return imp_MPI_Scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_scatter)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scatter_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scatter__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Gatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, int root, MPI_Comm comm) { return imp_MPI_Gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_gatherv)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_gatherv_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_gatherv__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Gatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Scatterv)(const void *sendbuf, const int sendcounts[], const int displs[], MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) { return imp_MPI_Scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf, recvcount, recvtype, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_scatterv)(const void* sendbuf, const int sendcounts[], const int displs[], MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatterv(sendbuf, sendcounts, displs, PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scatterv_)(const void* sendbuf, const int sendcounts[], const int displs[], MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatterv(sendbuf, sendcounts, displs, PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scatterv__)(const void* sendbuf, const int sendcounts[], const int displs[], MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scatterv(sendbuf, sendcounts, displs, PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Allgather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) { return imp_MPI_Allgather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_allgather)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allgather_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allgather__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Allgatherv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, const int recvcounts[], const int displs[], MPI_Datatype recvtype, MPI_Comm comm) { return imp_MPI_Allgatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_allgatherv)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allgatherv_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allgatherv__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, const int recvcounts[], const int displs[], MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allgatherv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, recvcounts, displs, PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Alltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm) { return imp_MPI_Alltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_alltoall)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Alltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_alltoall_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Alltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_alltoall__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Alltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Reduce)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm) { return imp_MPI_Reduce(sendbuf, recvbuf, count, datatype, op, root, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_reduce)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_reduce_)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_reduce__)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Allreduce)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) { return imp_MPI_Allreduce(sendbuf, recvbuf, count, datatype, op, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_allreduce)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allreduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allreduce_)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allreduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_allreduce__)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Allreduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Reduce_scatter)(const void *sendbuf, void *recvbuf, const int recvcounts[], MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) { return imp_MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_reduce_scatter)(const void* sendbuf, void* recvbuf, const int recvcounts[], MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_reduce_scatter_)(const void* sendbuf, void* recvbuf, const int recvcounts[], MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_reduce_scatter__)(const void* sendbuf, void* recvbuf, const int recvcounts[], MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Reduce_scatter(sendbuf, recvbuf, recvcounts, PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Scan)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) { return imp_MPI_Scan(sendbuf, recvbuf, count, datatype, op, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_scan)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scan(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scan_)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scan(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_scan__)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Scan(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Type_create_darray)(int size, int rank, int ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype) { return imp_MPI_Type_create_darray(size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype, ierr); }
extern void RECORDER_MPI_DECL(mpi_type_create_darray)(int* size, int* rank, int* ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int* order, MPI_Fint* oldtype, MPI_Fint* newtype, MPI_Fint *ierr){ imp_MPI_Type_create_darray((*size), (*rank), (*ndims), array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, (*order), PMPI_Type_f2c(*oldtype), (MPI_Datatype*)newtype, ierr);}
extern void RECORDER_MPI_DECL(mpi_type_create_darray_)(int* size, int* rank, int* ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int* order, MPI_Fint* oldtype, MPI_Fint* newtype, MPI_Fint *ierr){ imp_MPI_Type_create_darray((*size), (*rank), (*ndims), array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, (*order), PMPI_Type_f2c(*oldtype), (MPI_Datatype*)newtype, ierr);}
extern void RECORDER_MPI_DECL(mpi_type_create_darray__)(int* size, int* rank, int* ndims, const int array_of_gsizes[], const int array_of_distribs[], const int array_of_dargs[], const int array_of_psizes[], int* order, MPI_Fint* oldtype, MPI_Fint* newtype, MPI_Fint *ierr){ imp_MPI_Type_create_darray((*size), (*rank), (*ndims), array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, (*order), PMPI_Type_f2c(*oldtype), (MPI_Datatype*)newtype, ierr);}
int RECORDER_MPI_DECL(MPI_Type_commit)(MPI_Datatype *datatype) { return imp_MPI_Type_commit(datatype, ierr); }
extern void RECORDER_MPI_DECL(mpi_type_commit)(MPI_Fint* datatype, MPI_Fint *ierr){
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    imp_MPI_Type_commit(&c_datatype, ierr);
}
extern void RECORDER_MPI_DECL(mpi_type_commit_)(MPI_Fint* datatype, MPI_Fint *ierr){
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    imp_MPI_Type_commit(&c_datatype, ierr);
}
extern void RECORDER_MPI_DECL(mpi_type_commit__)(MPI_Fint* datatype, MPI_Fint *ierr){
    MPI_Datatype c_datatype = PMPI_Type_f2c(*datatype);
    imp_MPI_Type_commit(&c_datatype, ierr);
}
int RECORDER_MPI_DECL(MPI_Finalized)(int *flag) { return imp_MPI_Finalized(flag, ierr); }
extern void RECORDER_MPI_DECL(mpi_finalized)(int* flag, MPI_Fint *ierr){ imp_MPI_Finalized(flag, ierr);}
extern void RECORDER_MPI_DECL(mpi_finalized_)(int* flag, MPI_Fint *ierr){ imp_MPI_Finalized(flag, ierr);}
extern void RECORDER_MPI_DECL(mpi_finalized__)(int* flag, MPI_Fint *ierr){ imp_MPI_Finalized(flag, ierr);}
int RECORDER_MPI_DECL(MPI_Cart_rank)(MPI_Comm comm, const int coords[], int *rank) { return imp_MPI_Cart_rank(comm, coords, rank, ierr); }
extern void RECORDER_MPI_DECL(mpi_cart_rank)(MPI_Fint* comm, const int coords[], int* rank, MPI_Fint *ierr){ imp_MPI_Cart_rank(PMPI_Comm_f2c(*comm), coords, rank, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_rank_)(MPI_Fint* comm, const int coords[], int* rank, MPI_Fint *ierr){ imp_MPI_Cart_rank(PMPI_Comm_f2c(*comm), coords, rank, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_rank__)(MPI_Fint* comm, const int coords[], int* rank, MPI_Fint *ierr){ imp_MPI_Cart_rank(PMPI_Comm_f2c(*comm), coords, rank, ierr);}
int RECORDER_MPI_DECL(MPI_Cart_create)(MPI_Comm comm_old, int ndims, const int dims[], const int periods[], int reorder, MPI_Comm *comm_cart) { return imp_MPI_Cart_create(comm_old, ndims, dims, periods, reorder, comm_cart, ierr); }
extern void RECORDER_MPI_DECL(mpi_cart_create)(MPI_Fint* comm_old, int* ndims, const int dims[], const int periods[], int* reorder, MPI_Fint* comm_cart, MPI_Fint *ierr){ imp_MPI_Cart_create(PMPI_Comm_f2c(*comm_old), (*ndims), dims, periods, (*reorder), (MPI_Comm*)comm_cart, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_create_)(MPI_Fint* comm_old, int* ndims, const int dims[], const int periods[], int* reorder, MPI_Fint* comm_cart, MPI_Fint *ierr){ imp_MPI_Cart_create(PMPI_Comm_f2c(*comm_old), (*ndims), dims, periods, (*reorder), (MPI_Comm*)comm_cart, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_create__)(MPI_Fint* comm_old, int* ndims, const int dims[], const int periods[], int* reorder, MPI_Fint* comm_cart, MPI_Fint *ierr){ imp_MPI_Cart_create(PMPI_Comm_f2c(*comm_old), (*ndims), dims, periods, (*reorder), (MPI_Comm*)comm_cart, ierr);}
int RECORDER_MPI_DECL(MPI_Cart_get)(MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]) { return imp_MPI_Cart_get(comm, maxdims, dims, periods, coords, ierr); }
extern void RECORDER_MPI_DECL(mpi_cart_get)(MPI_Fint* comm, int* maxdims, int dims[], int periods[], int coords[], MPI_Fint *ierr){ imp_MPI_Cart_get(PMPI_Comm_f2c(*comm), (*maxdims), dims, periods, coords, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_get_)(MPI_Fint* comm, int* maxdims, int dims[], int periods[], int coords[], MPI_Fint *ierr){ imp_MPI_Cart_get(PMPI_Comm_f2c(*comm), (*maxdims), dims, periods, coords, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_get__)(MPI_Fint* comm, int* maxdims, int dims[], int periods[], int coords[], MPI_Fint *ierr){ imp_MPI_Cart_get(PMPI_Comm_f2c(*comm), (*maxdims), dims, periods, coords, ierr);}
int RECORDER_MPI_DECL(MPI_Cart_shift)(MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) { return imp_MPI_Cart_shift(comm, direction, disp, rank_source, rank_dest, ierr); }
extern void RECORDER_MPI_DECL(mpi_cart_shift)(MPI_Fint* comm, int* direction, int* disp, int* rank_source, int* rank_dest, MPI_Fint *ierr){ imp_MPI_Cart_shift(PMPI_Comm_f2c(*comm), (*direction), (*disp), rank_source, rank_dest, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_shift_)(MPI_Fint* comm, int* direction, int* disp, int* rank_source, int* rank_dest, MPI_Fint *ierr){ imp_MPI_Cart_shift(PMPI_Comm_f2c(*comm), (*direction), (*disp), rank_source, rank_dest, ierr);}
extern void RECORDER_MPI_DECL(mpi_cart_shift__)(MPI_Fint* comm, int* direction, int* disp, int* rank_source, int* rank_dest, MPI_Fint *ierr){ imp_MPI_Cart_shift(PMPI_Comm_f2c(*comm), (*direction), (*disp), rank_source, rank_dest, ierr);}
int RECORDER_MPI_DECL(MPI_Wait)(MPI_Request *request, MPI_Status *status) {return imp_MPI_Wait(request, status, ierr);}
int RECORDER_MPI_DECL(mpi_wait_)(MPI_Fint* request, MPI_Fint* status) {return imp_MPI_Wait((MPI_Request*)request, (MPI_Status*)status, ierr);}
int RECORDER_MPI_DECL(MPI_Waitall)(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) {return imp_MPI_Waitall(count, array_of_requests, array_of_statuses, ierr);}
int RECORDER_MPI_DECL(mpi_waitall_)(int *count, MPI_Fint* array_of_requests, MPI_Fint* array_of_statuses) {return imp_MPI_Waitall(*count, (MPI_Request*)array_of_requests, (MPI_Status*)array_of_statuses, ierr);}
int RECORDER_MPI_DECL(MPI_Waitany) (int count, MPI_Request requests[], int *indx, MPI_Status *status) {return imp_MPI_Waitany(count, requests, indx, status, ierr);}
int RECORDER_MPI_DECL(MPI_Waitsome) (int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[]) {return imp_MPI_Waitsome(incount, requests, outcount, indices, statuses, ierr);}
int RECORDER_MPI_DECL(MPI_Test) (MPI_Request *request, int *flag, MPI_Status *status) {return imp_MPI_Test(request, flag, status, ierr);}
int RECORDER_MPI_DECL(MPI_Testall) (int count, MPI_Request requests[], int *flag, MPI_Status statuses[]) {return imp_MPI_Testall(count, requests, flag, statuses, ierr);}
int RECORDER_MPI_DECL(MPI_Testsome) (int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[]) {return imp_MPI_Testsome(incount, requests, outcount, indices, statuses, ierr);}
int RECORDER_MPI_DECL(MPI_Testany) (int count, MPI_Request requests[], int *indx, int *flag, MPI_Status *status) {return imp_MPI_Testany(count, requests, indx, flag, status, ierr);}
int RECORDER_MPI_DECL(MPI_Send)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) { return imp_MPI_Send(buf, count, datatype, dest, tag, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_send)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Send(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_send_)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Send(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_send__)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Send(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Recv)(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) { return imp_MPI_Recv(buf, count, datatype, source, tag, comm, status, ierr); }
extern void RECORDER_MPI_DECL(mpi_recv)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Recv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
extern void RECORDER_MPI_DECL(mpi_recv_)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Recv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
extern void RECORDER_MPI_DECL(mpi_recv__)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Recv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
int RECORDER_MPI_DECL(MPI_Sendrecv)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status) { return imp_MPI_Sendrecv(sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status, ierr); }
extern void RECORDER_MPI_DECL(mpi_sendrecv)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, int* dest, int* sendtag, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* source, int* recvtag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Sendrecv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), (*dest), (*sendtag), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*source), (*recvtag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
extern void RECORDER_MPI_DECL(mpi_sendrecv_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, int* dest, int* sendtag, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* source, int* recvtag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Sendrecv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), (*dest), (*sendtag), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*source), (*recvtag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
extern void RECORDER_MPI_DECL(mpi_sendrecv__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, int* dest, int* sendtag, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* source, int* recvtag, MPI_Fint* comm, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_Sendrecv(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), (*dest), (*sendtag), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*source), (*recvtag), PMPI_Comm_f2c(*comm), (MPI_Status*)status, ierr);}
int RECORDER_MPI_DECL(MPI_Isend)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {return imp_MPI_Isend(buf, count, datatype, dest, tag, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_isend)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Isend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_isend_)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Isend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_isend__)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Isend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Irecv)(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {return imp_MPI_Irecv(buf, count, datatype, source, tag, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_irecv)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){imp_MPI_Irecv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_irecv_)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){imp_MPI_Irecv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_irecv__)(void* buf, int* count, MPI_Fint* datatype, int* source, int* tag, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){imp_MPI_Irecv(buf, (*count), PMPI_Type_f2c(*datatype), (*source), (*tag), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Ssend)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) { return imp_MPI_Ssend(buf, count, datatype, dest, tag, comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_ssend)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){imp_MPI_Ssend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_ssend_)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Ssend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
extern void RECORDER_MPI_DECL(mpi_ssend__)(const void* buf, int* count, MPI_Fint* datatype, int* dest, int* tag, MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Ssend(buf, (*count), PMPI_Type_f2c(*datatype), (*dest), (*tag), PMPI_Comm_f2c(*comm), ierr);}
int RECORDER_MPI_DECL(MPI_Ireduce)(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request) { return imp_MPI_Ireduce(sendbuf, recvbuf, count, datatype, op, root, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_ireduce)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ireduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ireduce_)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ireduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ireduce__)(const void* sendbuf, void* recvbuf, int* count, MPI_Fint* datatype, MPI_Fint* op, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ireduce(sendbuf, recvbuf, (*count), PMPI_Type_f2c(*datatype), PMPI_Op_f2c(*op), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Igather)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) { return imp_MPI_Igather(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_igather)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Igather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_igather_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Igather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_igather__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Igather(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Iscatter)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request) { return imp_MPI_Iscatter(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, root, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_iscatter)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Iscatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_iscatter_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Iscatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_iscatter__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, int* root, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Iscatter(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), (*root), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Ialltoall)(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request *request) { return imp_MPI_Ialltoall(sendbuf, sendcount, sendtype, recvbuf, recvcount, recvtype, comm, request, ierr); }
extern void RECORDER_MPI_DECL(mpi_ialltoall)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ialltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ialltoall_)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ialltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
extern void RECORDER_MPI_DECL(mpi_ialltoall__)(const void* sendbuf, int* sendcount, MPI_Fint* sendtype, void* recvbuf, int* recvcount, MPI_Fint* recvtype, MPI_Fint* comm, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_Ialltoall(sendbuf, (*sendcount), PMPI_Type_f2c(*sendtype), recvbuf, (*recvcount), PMPI_Type_f2c(*recvtype), PMPI_Comm_f2c(*comm), (MPI_Request*)request, ierr);}
int RECORDER_MPI_DECL(MPI_Comm_free)(MPI_Comm *comm) { return imp_MPI_Comm_free(comm, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_free)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Comm_free((MPI_Comm*)comm, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_free_)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Comm_free((MPI_Comm*)comm, ierr);}
extern void RECORDER_MPI_DECL(mpi_comm_free__)(MPI_Fint* comm, MPI_Fint *ierr){ imp_MPI_Comm_free((MPI_Comm*)comm, ierr);}
/* Below are the functions that create new communicators
 * For fortran wrappers, we need to pass in a C MPI_Comm
 * and then translate it to MPI_Fint
 */
int RECORDER_MPI_DECL(MPI_Comm_split)(MPI_Comm comm, int color, int key, MPI_Comm *newcomm) { return imp_MPI_Comm_split(comm, color, key, newcomm, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_split)(MPI_Fint* comm, int* color, int* key, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split(PMPI_Comm_f2c(*comm), (*color), (*key), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_split_)(MPI_Fint* comm, int* color, int* key, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split(PMPI_Comm_f2c(*comm), (*color), (*key), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_split__)(MPI_Fint* comm, int* color, int* key, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split(PMPI_Comm_f2c(*comm), (*color), (*key), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
int RECORDER_MPI_DECL(MPI_Comm_create)(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) { return imp_MPI_Comm_create(comm, group, newcomm, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_create)(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_create(PMPI_Comm_f2c(*comm), PMPI_Group_f2c(*group), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_create_)(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_create(PMPI_Comm_f2c(*comm), PMPI_Group_f2c(*group), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_create__)(MPI_Fint* comm, MPI_Fint* group, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_create(PMPI_Comm_f2c(*comm), PMPI_Group_f2c(*group), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
int RECORDER_MPI_DECL(MPI_Comm_dup)(MPI_Comm comm, MPI_Comm *newcomm) { return imp_MPI_Comm_dup(comm, newcomm, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_dup)(MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_dup(PMPI_Comm_f2c(*comm), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_dup_)(MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_dup(PMPI_Comm_f2c(*comm), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_dup__)(MPI_Fint* comm, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_dup(PMPI_Comm_f2c(*comm), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
int RECORDER_MPI_DECL(MPI_Cart_sub)(MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm) { return imp_MPI_Cart_sub(comm, remain_dims, newcomm, ierr); }
extern void RECORDER_MPI_DECL(mpi_cart_sub)(MPI_Fint* comm, const int remain_dims[], MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Cart_sub(PMPI_Comm_f2c(*comm), remain_dims, &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_cart_sub_)(MPI_Fint* comm, const int remain_dims[], MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Cart_sub(PMPI_Comm_f2c(*comm), remain_dims, &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_cart_sub__)(MPI_Fint* comm, const int remain_dims[], MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Cart_sub(PMPI_Comm_f2c(*comm), remain_dims, &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
int RECORDER_MPI_DECL(MPI_Comm_split_type)(MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm) { return imp_MPI_Comm_split_type(comm, split_type, key, info, newcomm, ierr); }
extern void RECORDER_MPI_DECL(mpi_comm_split_type)(MPI_Fint* comm, int* split_type, int* key, MPI_Fint* info, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split_type(PMPI_Comm_f2c(*comm), (*split_type), (*key), PMPI_Info_f2c(*info), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_split_type_)(MPI_Fint* comm, int* split_type, int* key, MPI_Fint* info, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split_type(PMPI_Comm_f2c(*comm), (*split_type), (*key), PMPI_Info_f2c(*info), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}
extern void RECORDER_MPI_DECL(mpi_comm_split_type__)(MPI_Fint* comm, int* split_type, int* key, MPI_Fint* info, MPI_Fint* newcomm, MPI_Fint *ierr){
    MPI_Comm c_newcomm;
    imp_MPI_Comm_split_type(PMPI_Comm_f2c(*comm), (*split_type), (*key), PMPI_Info_f2c(*info), &c_newcomm, ierr);
    *newcomm = PMPI_Comm_c2f(c_newcomm);
}



int RECORDER_MPIIO_DECL(MPI_File_open)(MPI_Comm comm, const char *filename, int amode, MPI_Info info, MPI_File *fh) { return imp_MPI_File_open(comm, filename, amode, info, fh, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_open)(MPI_Fint* comm, const char* filename, int* amode, MPI_Fint* info, MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_open(PMPI_Comm_f2c(*comm), filename, (*amode), PMPI_Info_f2c(*info), (MPI_File*)fh, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_open_)(MPI_Fint* comm, const char* filename, int* amode, MPI_Fint* info, MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_open(PMPI_Comm_f2c(*comm), filename, (*amode), PMPI_Info_f2c(*info), (MPI_File*)fh, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_open__)(MPI_Fint* comm, const char* filename, int* amode, MPI_Fint* info, MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_open(PMPI_Comm_f2c(*comm), filename, (*amode), PMPI_Info_f2c(*info), (MPI_File*)fh, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_close)(MPI_File *fh) { return imp_MPI_File_close(fh, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_close)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_close((MPI_File*)fh, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_close_)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_close((MPI_File*)fh, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_close__)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_close((MPI_File*)fh, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_sync)(MPI_File fh) { return imp_MPI_File_sync(fh, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_sync)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_sync(PMPI_File_f2c(*fh), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_sync_)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_sync(PMPI_File_f2c(*fh), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_sync__)(MPI_Fint* fh, MPI_Fint *ierr){ imp_MPI_File_sync(PMPI_File_f2c(*fh), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_set_size)(MPI_File fh, MPI_Offset size) { return imp_MPI_File_set_size(fh, size, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_set_size)(MPI_Fint* fh, MPI_Offset size, MPI_Fint *ierr){ imp_MPI_File_set_size(PMPI_File_f2c(*fh), size, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_set_size_)(MPI_Fint* fh, MPI_Offset size, MPI_Fint *ierr){ imp_MPI_File_set_size(PMPI_File_f2c(*fh), size, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_set_size__)(MPI_Fint* fh, MPI_Offset size, MPI_Fint *ierr){ imp_MPI_File_set_size(PMPI_File_f2c(*fh), size, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_set_view)(MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, const char *datarep, MPI_Info info) { return imp_MPI_File_set_view(fh, disp, etype, filetype, datarep, info, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_set_view)(MPI_Fint* fh, MPI_Offset disp, MPI_Fint* etype, MPI_Fint* filetype, const char* datarep, MPI_Fint* info, MPI_Fint *ierr){ imp_MPI_File_set_view(PMPI_File_f2c(*fh), disp, PMPI_Type_f2c(*etype), PMPI_Type_f2c(*filetype), datarep, PMPI_Info_f2c(*info), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_set_view_)(MPI_Fint* fh, MPI_Offset disp, MPI_Fint* etype, MPI_Fint* filetype, const char* datarep, MPI_Fint* info, MPI_Fint *ierr){ imp_MPI_File_set_view(PMPI_File_f2c(*fh), disp, PMPI_Type_f2c(*etype), PMPI_Type_f2c(*filetype), datarep, PMPI_Info_f2c(*info), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_set_view__)(MPI_Fint* fh, MPI_Offset disp, MPI_Fint* etype, MPI_Fint* filetype, const char* datarep, MPI_Fint* info, MPI_Fint *ierr){ imp_MPI_File_set_view(PMPI_File_f2c(*fh), disp, PMPI_Type_f2c(*etype), PMPI_Type_f2c(*filetype), datarep, PMPI_Info_f2c(*info), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_at)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read_at(fh, offset, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_at)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at__)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_at_all)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read_at_all(fh, offset, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all_)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all__)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_all)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read_all(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_all)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_all_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_all__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_shared)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read_shared(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_shared)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_shared_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_shared__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_ordered)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_read_ordered(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_read_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_at_all_begin)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_read_at_all_begin(fh, offset, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all_begin)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all_begin_)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_at_all_begin__)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_all_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_read_all_begin(fh, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_all_begin)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_all_begin_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_all_begin__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_read_ordered_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_read_ordered_begin(fh, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered_begin)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered_begin_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_read_ordered_begin__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_read_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iread_at)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iread_at(fh, offset, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iread_at)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread_at_)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread_at__)(MPI_Fint* fh, MPI_Offset offset, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iread)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iread(fh, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iread)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iread_shared)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iread_shared(fh, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iread_shared)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread_shared_)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iread_shared__)(MPI_Fint* fh, void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iread_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_at)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write_at(fh, offset, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_at)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at__)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_at_all)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write_at_all(fh, offset, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all_)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all__)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_at_all(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_all)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write_all(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_all)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_all_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_all__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_all(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_shared)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write_shared(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_shared)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_shared_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_shared__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_ordered)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Status *status) { return imp_MPI_File_write_ordered(fh, buf, count, datatype, status, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* status, MPI_Fint *ierr){ imp_MPI_File_write_ordered(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Status*)status, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_at_all_begin)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_write_at_all_begin(fh, offset, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all_begin)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all_begin_)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_at_all_begin__)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_at_all_begin(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_all_begin)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_write_all_begin(fh, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_all_begin)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_all_begin_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_all_begin__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_all_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_write_ordered_begin)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype) { return imp_MPI_File_write_ordered_begin(fh, buf, count, datatype, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered_begin)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered_begin_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_write_ordered_begin__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint *ierr){ imp_MPI_File_write_ordered_begin(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iwrite_at)(MPI_File fh, MPI_Offset offset, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iwrite_at(fh, offset, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_at)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_at_)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_at__)(MPI_Fint* fh, MPI_Offset offset, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_at(PMPI_File_f2c(*fh), offset, buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iwrite)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iwrite(fh, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_iwrite_shared)(MPI_File fh, const void *buf, int count, MPI_Datatype datatype, MPI_Request *request) { return imp_MPI_File_iwrite_shared(fh, buf, count, datatype, request, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_shared)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_shared_)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_iwrite_shared__)(MPI_Fint* fh, const void* buf, int* count, MPI_Fint* datatype, MPI_Fint* request, MPI_Fint *ierr){ imp_MPI_File_iwrite_shared(PMPI_File_f2c(*fh), buf, (*count), PMPI_Type_f2c(*datatype), (MPI_Request*)request, ierr);}
int RECORDER_MPIIO_DECL(MPI_File_seek)(MPI_File fh, MPI_Offset offset, int whence) { return imp_MPI_File_seek(fh, offset, whence, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_seek)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_seek_)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_seek__)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_seek_shared)(MPI_File fh, MPI_Offset offset, int whence) { return imp_MPI_File_seek_shared(fh, offset, whence, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_seek_shared)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek_shared(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_seek_shared_)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek_shared(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_seek_shared__)(MPI_Fint* fh, MPI_Offset offset, int* whence, MPI_Fint *ierr){ imp_MPI_File_seek_shared(PMPI_File_f2c(*fh), offset, (*whence), ierr);}
int RECORDER_MPIIO_DECL(MPI_File_get_size)(MPI_File fh, MPI_Offset *size) { return imp_MPI_File_get_size(fh, size, ierr); }
extern void RECORDER_MPIIO_DECL(mpi_file_get_size)(MPI_Fint* fh, MPI_Offset* size, MPI_Fint *ierr){ imp_MPI_File_get_size(PMPI_File_f2c(*fh), size, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_get_size_)(MPI_Fint* fh, MPI_Offset* size, MPI_Fint *ierr){ imp_MPI_File_get_size(PMPI_File_f2c(*fh), size, ierr);}
extern void RECORDER_MPIIO_DECL(mpi_file_get_size__)(MPI_Fint* fh, MPI_Offset* size, MPI_Fint *ierr){ imp_MPI_File_get_size(PMPI_File_f2c(*fh), size, ierr);}
