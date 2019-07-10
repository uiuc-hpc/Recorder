/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright 2012 by The HDF Group
 * All rights reserved.
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

FILE *__recorderfh;
int depth;

#define TRACE_LEN 256

#ifdef RECORDER_PRELOAD
    #include <dlfcn.h>
    #ifndef DISABLE_MPIO_TRACE
        #define RECORDER_IMP_CHEN(func, ret, args, attr1, attr2, log_text)      \
            MAP_OR_FAIL(func)                                                   \
            depth++;                                                            \
            double tm1 = recorder_wtime();                                      \
            ret res = RECORDER_MPI_CALL(func) args ;                            \
            double tm2 = recorder_wtime();                                      \
            write_data_operation(#func, "", tm1, tm2, attr1, attr2, log_text);  \
            depth--;                                                            \
            return res;
    #else
        #define RECORDER_IMP_CHEN(func, ret, args, attr1, attr2, log_text)  \
            MAP_OR_FAIL(func)                                               \
            return RECORDER_MPI_CALL(func) args ;
    #endif
#endif

static inline char *comm2name(MPI_Comm comm) {
    char *tmp = malloc(128);
    int len;
    PMPI_Comm_get_name(comm, tmp, &len);
    tmp[len] = 0;
    if(len == 0) strcpy(tmp, "MPI_COMM_UNKNOWN");
    return tmp;
}

static inline char *type2name(MPI_Datatype type) {
    char *tmp = malloc(128);
    int len;
    PMPI_Type_get_name(type, tmp, &len);
    tmp[len] = 0;
    return tmp;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Comm_size (%s, %d)", comm2name(comm), *size);
    RECORDER_IMP_CHEN(PMPI_Comm_size, int, (comm, size), 0, 0, log_text)
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Comm_rank (%s, %d)", comm2name(comm), *rank);
    RECORDER_IMP_CHEN(PMPI_Comm_rank, int, (comm, rank), 0, 0, log_text)
}

int MPI_Comm_Barrier(MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Comm_Barrier (%s)", comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Barrier, int, (comm), 0, 0, log_text)
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Bast (%p, %d, %s, %d, %s)", buffer, count,
            type2name(datatype), root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Bcast, int, (buffer, count, datatype, root, comm), 0, 0, log_text)
}

int MPI_Gather(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Gather (%p, %d, %s, %p, %d, %s, %d, %s)", sbuf, scount,
            type2name(stype), rbuf, rcount, type2name(rtype), root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Gather, int, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm), 0, 0, log_text)
}

int MPI_Scatter(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Scatter (%p, %d, %s, %p, %d, %s, %d, %s)", sbuf, scount,
            type2name(stype), rbuf, rcount, type2name(rtype), root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Scatter, int, (sbuf, scount, stype, rbuf, rcount, rtype, root, comm), 0, 0, log_text)
}

int MPI_Gatherv(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        CONST int *rcount, CONST int *displs, MPI_Datatype rtype, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Gahterv (%p, %d, %s, %p, %d, %d, %s, %d, %s)", sbuf, scount,
            type2name(stype), rbuf, rcount, *displs, type2name(rtype), root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Gatherv, int, (sbuf, scount, stype, rbuf, rcount, displs, rtype, root, comm), 0, 0, log_text)
}

int MPI_Scatterv(CONST void *sbuf, CONST int *scount, CONST int *displa,
        MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Scatterv (%p, %d, %d, %s, %p, %d, %s, %d, %s)", sbuf, scount,
            *displa, type2name(stype), rbuf, rcount, type2name(rtype), root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Scatterv, int, (sbuf, scount, displa, stype, rbuf, rcount, rtype, root, comm), 0, 0, log_text)
}

/*
   int MPI_Allgather(CONST void* sbuf, int scount, MPI_Datatype stype, void* rbuf,
   int rcount, MPI_Datatype rtype, MPI_Comm comm ) {
   }
   */

int MPI_Allgatherv(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        CONST int *rcount, CONST int *displs, MPI_Datatype rtype, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Allgatherv (%p, %d, %s, %p, %d, %d, %s, %s)", sbuf, scount,
            type2name(stype), rbuf, *rcount, *displs, type2name(rtype), comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Allgatherv, int, (sbuf, scount, stype, rbuf, rcount, displs, rtype, comm), 0, 0, log_text)
}

int MPI_Alltoall(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
        int rcount, MPI_Datatype rtype, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Alltoall (%p, %d, %s, %p, %d, %s, %s)", sbuf, scount,
            type2name(stype), rbuf, rcount, type2name(rtype), comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Alltoall, int, (sbuf, scount, stype, rbuf, rcount, rtype, comm), 0, 0, log_text)
}

int MPI_Reduce(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
        MPI_Op op, int root, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Reduce (%p, %p, %d, %s, %d, %d, %s)", sbuf, rbuf, count,
            type2name(stype), op, root, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Reduce, int, (sbuf, rbuf, count, stype, op, root, comm), 0, 0, log_text)
}

int MPI_Allreduce(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
        MPI_Op op, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Allreduce (%p, %p, %d, %s, %d, %s)", sbuf, rbuf, count,
            type2name(stype), op, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Allreduce, int, (sbuf, rbuf, count, stype, op, comm), 0, 0, log_text)
}

int MPI_Reduce_scatter(CONST void *sbuf, void *rbuf, CONST int *rcounts,
        MPI_Datatype stype, MPI_Op op, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Reduce_scatter (%p, %p, %d, %s, %d, %s)", sbuf, rbuf, *rcounts,
            type2name(stype), op, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Reduce_scatter, int, (sbuf, rbuf, rcounts, stype, op, comm), 0, 0, log_text)
}

int MPI_Scan(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
        MPI_Op op, MPI_Comm comm) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Scan (%p, %p, %d, %s, %d, %s)", sbuf, rbuf, count,
            type2name(stype), op, comm2name(comm));
    RECORDER_IMP_CHEN(PMPI_Scan, int, (sbuf, rbuf, count, stype, op, comm), 0, 0, log_text)
}

int MPI_Type_create_darray(int size, int rank, int ndims, CONST int array_of_gsizes[], CONST int array_of_distribs[],
        CONST int array_of_dargs[], CONST int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Type_create_darray (%d, %d, %d, %p, %p, %p, %p, %d, %s, %p)",
            size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, type2name(oldtype), newtype);
    RECORDER_IMP_CHEN(PMPI_Type_create_darray, int, (size, rank, ndims, array_of_gsizes, array_of_distribs, array_of_dargs, array_of_psizes, order, oldtype, newtype), 0, 0, log_text)
}

int MPI_Type_commit(MPI_Datatype *datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Type_commit (%p)", datatype);
    RECORDER_IMP_CHEN(PMPI_Type_commit, int, (datatype), 0, 0, log_text)
}

int MPI_File_open(MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_open (%s, %s, %d, %d, %p)", comm2name(comm), filename, amode, info, fh);
    RECORDER_IMP_CHEN(PMPI_File_open, int, (comm, filename, amode, info, fh), amode, 0, log_text)
}

int MPI_File_close(MPI_File *fh) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_close (%p)", fh);
    RECORDER_IMP_CHEN(PMPI_File_close, int, (fh), 0, 0, log_text)
}

int MPI_File_sync(MPI_File fh) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_sync (%p)", fh);
    RECORDER_IMP_CHEN(PMPI_File_sync, int, (fh), 0 ,0, log_text)
}

int MPI_File_set_size(MPI_File fh, MPI_Offset size) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_set_size (%p, %lld)", fh, size);
    RECORDER_IMP_CHEN(PMPI_File_set_size, int, (fh, size), size, 0, log_text)
}

int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
        MPI_Datatype filetype, CONST char *datarep, MPI_Info info) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_set_view (%p, %lld, %s, %s, %s, %d)", fh, disp,
            type2name(etype), type2name(filetype), datarep, info);
    RECORDER_IMP_CHEN(PMPI_File_set_view, int, (fh, disp, etype, filetype, datarep, info), disp, 0, log_text)
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_at (%p, %lld, %p, %d, %s, %p)", fh, offset,
            buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read_at, int, (fh, offset, buf, count, datatype, status), offset, count, log_text)
}

int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_at_all (%p, %lld, %p, %d, %s, %p)", fh, offset,
            buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read_at_all, int, (fh, offset, buf, count, datatype, status), offset, count, log_text)
}

int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
        MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_all (%p, %p, %d, %s, %p)", fh, buf,
            count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read_all, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_read_shared(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_shared (%p, %p, %d, %s, %p)", fh, buf,
            count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read_shared, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_read_ordered(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_ordered (%p, %p, %d, %s, %p)", fh, buf,
            count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_read_ordered, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
        int count, MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_at_all_begin (%p, %lld, %p, %d, %s)", fh, offset,
            buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_read_at_all_begin, int, (fh, offset, buf, count, datatype), offset, count, log_text)
}

int MPI_File_read_all_begin(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_all_begin (%p, %p, %d, %s)", fh, buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_read_all_begin, int, (fh, buf, count, datatype), count, 0, log_text)
}

int MPI_File_read_ordered_begin(MPI_File fh, void *buf, int count, MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_read_ordered_begin (%p, %p, %d, %s)", fh, buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_read_ordered_begin, int, (fh, buf, count, datatype), count, 0, log_text)
}

int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iread_at (%p, %lld, %p, %d, %s, %p)", fh, offset,
            buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iread_at, int, (fh, offset, buf, count, datatype, request), offset, count, log_text)
}

int MPI_File_iread(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iread (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iread, int, (fh, buf, count, datatype, request), count, 0, log_text)
}

int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iread_sared (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iread_shared, int, (fh, buf, count, datatype, request), count, 0, log_text)
}

int MPI_File_write(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_write_at(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_at (%p, %lld, %p, %d, %s, %p)", fh, offset,
            buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write_at, int, (fh, offset, buf, count, datatype, status), offset, count, log_text)
}

int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype,
        MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_at_all (%p, %lld, %p, %d, %s, %p)", fh, offset,
            buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write_at_all, int, (fh, offset, buf, count, datatype, status), offset, count, log_text)
}

int MPI_File_write_all(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_all (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write_all, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_write_shared(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_shared (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write_shared, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_write_ordered(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, MPI_Status *status) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_ordered (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), status);
    RECORDER_IMP_CHEN(PMPI_File_write_ordered, int, (fh, buf, count, datatype, status), count, 0, log_text)
}

int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_at_all_begin (%p, %lld, %p, %d, %s)", fh, offset,
            buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_write_at_all_begin, int, (fh, offset, buf, count, datatype), count, 0, log_text)
}

int MPI_File_write_all_begin(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_all_begin (%p, %p, %d, %s)", fh, buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_write_all_begin, int, (fh, buf, count, datatype), count, 0, log_text)
}

int MPI_File_write_ordered_begin(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_write_ordered_begin (%p, %p, %d, %s)", fh, buf, count, type2name(datatype));
    RECORDER_IMP_CHEN(PMPI_File_write_ordered_begin, int, (fh, buf, count, datatype), count, 0, log_text)
}

int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, CONST void *buf,
        int count, MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iwrite_at (%p, %lld, %p, %d, %s, %p)", fh, offset, buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iwrite_at, int, (fh, offset, buf, count, datatype, request), offset, count, log_text)
}

int MPI_File_iwrite(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iwrite (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iwrite, int, (fh, buf, count, datatype, request), count, 0, log_text)
}

int MPI_File_iwrite_shared(MPI_File fh, CONST void *buf, int count,
        MPI_Datatype datatype, __D_MPI_REQUEST *request) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_File_iwrite_shared (%p, %p, %d, %s, %p)", fh, buf, count, type2name(datatype), request);
    RECORDER_IMP_CHEN(PMPI_File_iwrite_shared, int, (fh, buf, count, datatype, request), count, 0, log_text)
}

int MPI_Finalized(int *flag) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "MPI_Finalized (%p)", flag);
    RECORDER_IMP_CHEN(PMPI_Finalized, int, (flag), 0, 0, log_text)
}
