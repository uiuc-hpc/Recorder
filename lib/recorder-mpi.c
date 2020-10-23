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

#include "mpi.h"
#include "recorder.h"

#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif



int depth;
static int split_times;         // how many time we see MPI_Comm_split

static inline char *comm2name(MPI_Comm comm) {
    int len = 0;
    char *tmp = calloc(128, sizeof(char));
    if(comm == MPI_COMM_NULL)
        strcpy(tmp, "MPI_COMM_NULL");
    else {
        PMPI_Comm_get_name(comm, tmp, &len);
        if(len == 0)
            strcpy(tmp, "MPI_COMM_UNKNOWN");
    }
    return tmp;
}

static inline char *type2name(MPI_Datatype type) {
    char *tmp = malloc(128);
    int len;
    PMPI_Type_get_name(type, tmp, &len);
    tmp[len] = 0;
    if(len == 0) strcpy(tmp, "MPI_TYPE_UNKNOWN");
    return tmp;
}

static inline char* status2str(MPI_Status *status) {
    char *tmp = calloc(128, sizeof(char));
    if(status == MPI_STATUS_IGNORE)
        strcpy(tmp, "MPI_STATUS_IGNORE");
    else
        sprintf(tmp, "[%d %d %d]", status->MPI_SOURCE, status->MPI_TAG, status->MPI_ERROR);
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
 * Intercept the following functions
 */
int RECORDER_MPI_DECL(MPI_Comm_size)(MPI_Comm comm, int *size) {
    // TODO: size is only know after the function call
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_size, (comm, size));
    char **args = assemble_args_list(2, comm2name(comm), ptoa(size));
    RECORDER_INTERCEPTOR(2, args);

}

int RECORDER_MPI_DECL(MPI_Comm_rank)(MPI_Comm comm, int *rank) {
    // TODO: rank is only know after the function call
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_rank, (comm, rank));
    char **args = assemble_args_list(2, comm2name(comm), ptoa(rank));
    RECORDER_INTERCEPTOR(2, args);

}

int RECORDER_MPI_DECL(MPI_Get_processor_name)(char *name, int *resultlen) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Get_processor_name, (name, resultlen));
    char **args = assemble_args_list(2, ptoa(name), ptoa(resultlen));
    RECORDER_INTERCEPTOR(2, args);
}

int RECORDER_MPI_DECL(MPI_Comm_set_errhandler)(MPI_Comm comm, MPI_Errhandler errhandler) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_set_errhandler, (comm, errhandler));
    char **args = assemble_args_list(2, comm2name(comm), ptoa(&errhandler));
    RECORDER_INTERCEPTOR(2, args);
}

int RECORDER_MPI_DECL(MPI_Barrier)(MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Barrier, (comm));
    char **args = assemble_args_list(1, comm2name(comm));
    RECORDER_INTERCEPTOR(1, args);
}

int RECORDER_MPI_DECL(MPI_Bcast)(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Bcast, (buffer, count, datatype, root, comm));
    char **args = assemble_args_list(5, ptoa(buffer), itoa(count), type2name(datatype), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_Gather)(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Gather, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm));
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(8, args);
}

int RECORDER_MPI_DECL(MPI_Scatter)(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Scatter, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm));
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(8, args);
}

int RECORDER_MPI_DECL(MPI_Gatherv)(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        CONST int *rcount, CONST int *displs, MPI_Datatype rtype, int root, MPI_Comm comm) {
    // TODO: *displs
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Gatherv, (sbuf, scount, stype, rbuf, rcount, displs, rtype, root, comm));
    char **args = assemble_args_list(9, ptoa(sbuf), itoa(scount), type2name(stype), ptoa(rbuf),
                                        ptoa(rcount), ptoa(displs), type2name(rtype), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(9, args);

}

int RECORDER_MPI_DECL(MPI_Scatterv)(CONST void *sbuf, CONST int *scount, CONST int *displa,
        MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Scatterv, (sbuf, scount, displa, stype, rbuf, rcount, rtype, root, comm));
    char **args = assemble_args_list(9, ptoa(sbuf), ptoa(scount), ptoa(displa), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(9, args);

}

/*
   int MPI_Allgather(CONST void* sbuf, int scount, MPI_Datatype stype, void* rbuf,
   int rcount, MPI_Datatype rtype, MPI_Comm comm ) {
   }
   */

int RECORDER_MPI_DECL(MPI_Allgatherv)(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        CONST int *rcount, CONST int *displs, MPI_Datatype rtype, MPI_Comm comm) {
    // TODO: displs
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Allgatherv, (sbuf, scount, stype, rbuf, rcount, displs, rtype, comm));
    char **args = assemble_args_list(8, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), ptoa(rcount), ptoa(displs), type2name(rtype), comm2name(comm));
    RECORDER_INTERCEPTOR(8, args);

}

int RECORDER_MPI_DECL(MPI_Alltoall)(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Alltoall, (sbuf, scount, stype, rbuf, rcount, rtype, comm));
    char **args = assemble_args_list(7, ptoa(sbuf), itoa(scount), type2name(stype),
                                        ptoa(rbuf), itoa(rcount), type2name(rtype), comm2name(comm));
    RECORDER_INTERCEPTOR(7, args);

}

int RECORDER_MPI_DECL(MPI_Reduce)(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, int root, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Reduce, (sbuf, rbuf, count, stype, op, root, comm));
    char **args = assemble_args_list(7, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype),
                                    itoa(op), itoa(root), comm2name(comm));
    RECORDER_INTERCEPTOR(7, args);
}

int RECORDER_MPI_DECL(MPI_Allreduce)(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Allreduce, (sbuf, rbuf, count, stype, op, comm));
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype), itoa(op), comm2name(comm));
    RECORDER_INTERCEPTOR(6, args)
}

int RECORDER_MPI_DECL(MPI_Reduce_scatter)(CONST void *sbuf, void *rbuf, CONST int *rcounts,
        MPI_Datatype stype, MPI_Op op, MPI_Comm comm) {
    // TODO: *rcounts
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Reduce_scatter, (sbuf, rbuf, rcounts, stype, op, comm));
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), ptoa(rcounts), type2name(stype), itoa(op), comm2name(comm));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_Scan)(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Scan, (sbuf, rbuf, count, stype, op, comm));
    char **args = assemble_args_list(6, ptoa(sbuf), ptoa(rbuf), itoa(count), type2name(stype), itoa(op), comm2name(comm));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_Type_create_darray)(int size, int rank, int ndims, CONST int array_of_gsizes[], CONST int array_of_distribs[],
        CONST int array_of_dargs[], CONST int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Type_create_darray, (size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype));
    char **args = assemble_args_list(10, itoa(size), itoa(rank), itoa(ndims), ptoa(array_of_gsizes), ptoa(array_of_distribs),
                            ptoa(array_of_dargs), ptoa(array_of_psizes), itoa(order), type2name(oldtype), ptoa(newtype));
    RECORDER_INTERCEPTOR(10, args);
}

int RECORDER_MPI_DECL(MPI_Type_commit)(MPI_Datatype *datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Type_commit, (datatype));
    char **args = assemble_args_list(1, ptoa(datatype));
    RECORDER_INTERCEPTOR(1, args);
}

int RECORDER_MPI_DECL(MPI_File_open)(MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh) {
    // TODO: filename
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_open, (comm, filename, amode, info, fh));
    char **args = assemble_args_list(5, comm2name(comm), realrealpath(filename), itoa(amode), ptoa(&info), ptoa(fh));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_close)(MPI_File *fh) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_close, (fh));
    char **args = assemble_args_list(1, ptoa(fh));
    RECORDER_INTERCEPTOR(1, args);
}

int RECORDER_MPI_DECL(MPI_File_sync)(MPI_File fh) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_sync, (fh));
    char **args = assemble_args_list(1, ptoa(&fh));
    RECORDER_INTERCEPTOR(1, args);
}

int RECORDER_MPI_DECL(MPI_File_set_size)(MPI_File fh, MPI_Offset size) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_set_size, (fh, size));
    char **args = assemble_args_list(2, ptoa(&fh), itoa(size));
    RECORDER_INTERCEPTOR(2, args);
}

int RECORDER_MPI_DECL(MPI_File_set_view)(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
        MPI_Datatype filetype, CONST char *datarep, MPI_Info info) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_set_view, (fh, disp, etype, filetype, datarep, info));
    char **args = assemble_args_list(6, ptoa(fh), itoa(disp), type2name(etype), type2name(filetype), ptoa(datarep), ptoa(&info));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_read)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_read_at)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_at, (fh, offset, buf, count, datatype, status));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_read_at_all)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_at_all, (fh, offset, buf, count, datatype, status));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_read_all)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_all, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_read_shared)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_shared, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_read_ordered)(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_ordered, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_read_at_all_begin)(MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_at_all_begin, (fh, offset, buf, count, datatype));
    char **args = assemble_args_list(5, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_read_all_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_all_begin, (fh, buf, count, datatype));
    char **args = assemble_args_list(4, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(4, args);
}

int RECORDER_MPI_DECL(MPI_File_read_ordered_begin)(MPI_File fh, void *buf, int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_read_ordered_begin, (fh, buf, count, datatype));
    char **args = assemble_args_list(4, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(4, args);
}

int RECORDER_MPI_DECL(MPI_File_iread_at)(MPI_File fh, MPI_Offset offset, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iread_at, (fh, offset, buf, count, datatype, request));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_iread)(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iread, (fh, buf, count, datatype, request));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_iread_shared)(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iread_shared, (fh, buf, count, datatype, request));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write_at)(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_at, (fh, offset, buf, count, datatype, status));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_write_at_all)(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_at_all, (fh, offset, buf, count, datatype, status));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_write_all)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_all, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write_shared)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_shared, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write_ordered)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_ordered, (fh, buf, count, datatype, status));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), status2str(status));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write_at_all_begin)(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_at_all_begin, (fh, offset, buf, count, datatype));
    char **args = assemble_args_list(5, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_write_all_begin)(MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_all_begin, (fh, buf, count, datatype));
    char **args = assemble_args_list(4, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(4, args);
}

int RECORDER_MPI_DECL(MPI_File_write_ordered_begin)(MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_write_ordered_begin, (fh, buf, count, datatype));
    char **args = assemble_args_list(4, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype));
    RECORDER_INTERCEPTOR(4, args);
}

int RECORDER_MPI_DECL(MPI_File_iwrite_at)(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iwrite_at, (fh, offset, buf, count, datatype, request));
    char **args = assemble_args_list(6, ptoa(fh), itoa(offset), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(6, args);
}

int RECORDER_MPI_DECL(MPI_File_iwrite)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iwrite, (fh, buf, count, datatype, request));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_File_iwrite_shared)(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_iwrite_shared, (fh, buf, count, datatype, request));
    char **args = assemble_args_list(5, ptoa(fh), ptoa(buf), itoa(count), type2name(datatype), ptoa(request));
    RECORDER_INTERCEPTOR(5, args);
}

int RECORDER_MPI_DECL(MPI_Finalized)(int *flag) {
    // TODO: flag
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Finalized, (flag));
    char **args = assemble_args_list(1, ptoa(flag));
    RECORDER_INTERCEPTOR(1, args);
}

// Added 10 new MPI funcitons on 2019/01/07
int RECORDER_MPI_DECL(MPI_Cart_rank) (MPI_Comm comm, CONST int coords[], int *rank) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Cart_rank, (comm, coords, rank));
    char **args = assemble_args_list(3, comm2name(comm), ptoa(coords), ptoa(rank));
    RECORDER_INTERCEPTOR(3, args);
}
int RECORDER_MPI_DECL(MPI_Cart_create) (MPI_Comm comm_old, int ndims, CONST int dims[], CONST int periods[], int reorder, MPI_Comm *comm_cart) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Cart_create, (comm_old, ndims, dims, periods, reorder, comm_cart));
    char **args = assemble_args_list(6, comm2name(comm_old), itoa(ndims), ptoa(dims), ptoa(periods), itoa(reorder), ptoa(comm_cart));
    RECORDER_INTERCEPTOR(6, args)
}
int RECORDER_MPI_DECL(MPI_Cart_get) (MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Cart_get, (comm, maxdims, dims, periods, coords));
    char **args = assemble_args_list(5, comm2name(comm), itoa(maxdims), ptoa(dims), ptoa(periods), ptoa(coords));
    RECORDER_INTERCEPTOR(5, args);
}
int RECORDER_MPI_DECL(MPI_Cart_shift) (MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Cart_shift, (comm, direction, disp, rank_source, rank_dest));
    char **args = assemble_args_list(5, comm2name(comm), itoa(direction), itoa(disp), ptoa(rank_source), ptoa(rank_dest));
    RECORDER_INTERCEPTOR(5, args);
}
int RECORDER_MPI_DECL(MPI_Wait) (MPI_Request *request, MPI_Status *status) {
    char **args = assemble_args_list(2, itoa(*request), status2str(status));
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Wait, (request, status));
    RECORDER_INTERCEPTOR(2, args);
}

int RECORDER_MPI_DECL(MPI_Send) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Send, (buf, count, datatype, dest, tag, comm));
    char **args = assemble_args_list(6, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(comm));
    RECORDER_INTERCEPTOR(6, args);
}
int RECORDER_MPI_DECL(MPI_Recv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Recv, (buf, count, datatype, source, tag, comm, status));
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(source), itoa(tag), comm2name(comm), status2str(status));
    RECORDER_INTERCEPTOR(7, args);
}
int RECORDER_MPI_DECL(MPI_Sendrecv) (CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                                        int source, int recvtag, MPI_Comm comm, MPI_Status *status) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Sendrecv, (sendbuf, sendcount, sendtype, dest, sendtag, recvbuf, recvcount, recvtype, source, recvtag, comm, status));
    char **args = assemble_args_list(12, ptoa(sendbuf), itoa(sendcount), type2name(sendtype), itoa(dest), itoa(sendtag), ptoa(recvbuf), itoa(recvcount), type2name(recvtype),
                                        itoa(source), itoa(recvtag), comm2name(comm), status2str(status));
    RECORDER_INTERCEPTOR(12, args);
}
int RECORDER_MPI_DECL(MPI_Isend) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Isend, (buf, count, datatype, dest, tag, comm, request));
    size_t r = *request;
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(comm), itoa(r));
    RECORDER_INTERCEPTOR(7, args);
}
int RECORDER_MPI_DECL(MPI_Irecv) (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Irecv, (buf, count, datatype, source, tag, comm, request));
    size_t r = *request;
    char **args = assemble_args_list(7, ptoa(buf), itoa(count), type2name(datatype), itoa(source), itoa(tag), comm2name(comm), itoa(r));
    RECORDER_INTERCEPTOR(7, args);
}


// Add MPI_Waitall, MPI_Waitsome, MPI_Waitany and MPI_Ssend on 2020/08/06
int RECORDER_MPI_DECL(MPI_Waitall) (int count, MPI_Request requests[], MPI_Status statuses[]) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Waitall, (count, requests, statuses));
    char **args = assemble_args_list(3, itoa(count), requests_str, ptoa(statuses));
    RECORDER_INTERCEPTOR(3, args);
}
int RECORDER_MPI_DECL(MPI_Waitsome) (int incount, MPI_Request requests[], int *outcount, int indices[], MPI_Status statuses[]) {
    int i;
    size_t arr[incount];
    for(i = 0; i < incount; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, incount);

    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Waitsome, (incount, requests, outcount, indices, statuses));
    size_t arr2[*outcount];
    for(i = 0; i < *outcount; i++)
        arr2[i] = (size_t) indices[i];
    char* indices_str = arrtoa(arr2, *outcount);
    char **args = assemble_args_list(5, itoa(incount), requests_str, itoa(*outcount), indices_str, ptoa(statuses));
    RECORDER_INTERCEPTOR(5, args);
}
int RECORDER_MPI_DECL(MPI_Waitany) (int count, MPI_Request requests[], int *indx, MPI_Status *status) {
    int i;
    size_t arr[count];
    for(i = 0; i < count; i++)
        arr[i] = (size_t) requests[i];
    char* requests_str = arrtoa(arr, count);

    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Waitany, (count, requests, indx, status));
    char **args = assemble_args_list(4, itoa(count), requests_str, itoa(*indx), status2str(status));
    RECORDER_INTERCEPTOR(4, args);
}
int RECORDER_MPI_DECL(MPI_Ssend) (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Ssend, (buf, count, datatype, dest, tag, comm));
    char **args = assemble_args_list(6, ptoa(buf), itoa(count), type2name(datatype), itoa(dest), itoa(tag), comm2name(comm));
    RECORDER_INTERCEPTOR(6, args);
}


int RECORDER_MPI_DECL(MPI_Comm_split) (MPI_Comm comm, int color, int key, MPI_Comm *newcomm) {
    //printf("comm: %d, color: %d, key: %d\n", comm, color, key);
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_split, (comm, color, key, newcomm));

    if((newcomm != NULL) && (*newcomm != MPI_COMM_NULL)) {
        int rank;
        PMPI_Comm_rank(*newcomm, &rank);

        char new_comm_name[128] = {0};
        if(rank == 0) {
            char* parent_name = comm2name(comm);

            int global_rank;
            PMPI_Comm_rank(MPI_COMM_WORLD, &global_rank);

            sprintf(new_comm_name, "%s_(%d,%d)", parent_name, split_times++, global_rank);
            free(parent_name);
        }
        PMPI_Bcast(&split_times, 1, MPI_INT, 0, *newcomm);
        PMPI_Bcast(new_comm_name, 128, MPI_CHAR, 0, *newcomm);
        PMPI_Comm_set_name(*newcomm, new_comm_name);
    }

    char **args = assemble_args_list(4, comm2name(comm), itoa(color), itoa(key), comm2name(*newcomm));
    RECORDER_INTERCEPTOR(4, args);
}

int RECORDER_MPI_DECL(MPI_Comm_create) (MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_create, (comm, group, newcomm));
    char **args = assemble_args_list(3, comm2name(comm), itoa(group), comm2name(*newcomm));
    RECORDER_INTERCEPTOR(3, args);
}

int RECORDER_MPI_DECL(MPI_Comm_dup) (MPI_Comm comm, MPI_Comm * newcomm) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_Comm_dup, (comm, newcomm));

    char new_comm_name[128] = {0};
    char* parent_name = comm2name(comm);
    sprintf(new_comm_name, "%s_dup", parent_name);
    PMPI_Comm_set_name(*newcomm, new_comm_name);

    char **args = assemble_args_list(2, comm2name(comm), comm2name(*newcomm));
    RECORDER_INTERCEPTOR(2, args);
}


int RECORDER_MPI_DECL(MPI_File_seek) (MPI_File fh, MPI_Offset offset, int whence) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_seek, (fh, offset, whence));
    char **args = assemble_args_list(3, ptoa(fh), itoa(offset), whence2name(whence));
    RECORDER_INTERCEPTOR(3, args);
}

int RECORDER_MPI_DECL(MPI_File_seek_shared) (MPI_File fh, MPI_Offset offset, int whence) {
    RECORDER_INTERCEPTOR_NOIO(int, PMPI_File_seek_shared, (fh, offset, whence));
    char **args = assemble_args_list(3, ptoa(fh), itoa(offset), whence2name(whence));
    RECORDER_INTERCEPTOR(3, args);
}


