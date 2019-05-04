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
#include "recorder-dynamic.h"

#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif

extern char *__progname;
FILE *__recorderfh;
int depth;

void recorder_mpi_initialize(int *argc, char ***argv) {
  int nprocs;
  int rank;

  RECORDER_MPI_CALL(PMPI_Comm_size)(MPI_COMM_WORLD, &nprocs);
  RECORDER_MPI_CALL(PMPI_Comm_rank)(MPI_COMM_WORLD, &rank);

  char *logfile_name;
  char *metafile_name;
  char *logdir_name;
  logfile_name = malloc(PATH_MAX);
  logdir_name = malloc(PATH_MAX);
  metafile_name = malloc(PATH_MAX);
  char cuser[L_cuserid] = {0};
  cuserid(cuser);

  sprintf(logdir_name, "%s_%s", cuser, __progname);
  int status;
  status = mkdir(logdir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);

  // write enabled i/o layers to meta file
  if(rank == 0){
    FILE *__recordermeta;
    sprintf(metafile_name, "%s/recorder.meta", logdir_name);
    __recordermeta = fopen(metafile_name, "w");
    printf(" metafile_name %s\n", metafile_name);

    fprintf(__recordermeta, "enabled_layers:");
    #ifndef DISABLE_HDF5_TRACE
    fprintf(__recordermeta, " HDF5");
    #endif
    #ifndef DISABLE_MPIO_TRACE
    fprintf(__recordermeta, " MPI");
    #endif
    #ifndef DISABLE_POSIX_TRACE
    fprintf(__recordermeta, " POSIX");
    #endif
    fprintf(__recordermeta, "\n");
    fprintf(__recordermeta, "workload_start_time:");
    fprintf(__recordermeta, " %.5f\n",recorder_wtime());
    fclose(__recordermeta);
  }

  sprintf(logfile_name, "%s/log.%d", logdir_name, rank);
  __recorderfh = fopen(logfile_name, "w");
  depth = 0;

  printf(" logfile_name %s ,recorderfh %d\n", logfile_name, __recorderfh);

  free(logfile_name);
  free(logdir_name);

  return;
}

void recorder_shutdown(int timing_flag) {
  fclose(__recorderfh);

  return;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Comm_size (%s,%p)", tm1, comm_name, size);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Comm_size)(comm, size);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Comm_rank (%s,%p)", tm1, comm_name, rank);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Comm_rank)(comm, rank);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Comm_Barrier(MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Barrier (%s)", tm1, comm_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Barrier)(comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, int root,
              MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *type_name = type2name(datatype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Bcast (%p,%d,%s,%d,%s)", tm1, buffer, count,
            type_name, root, comm_name);

  free(type_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Bcast)(buffer, count, datatype, root, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Gather(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
               int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Gather (%p,%d,%s,%p,%d,%s,%d,%s)", tm1,
            sbuf, scount, stype_name, rbuf, rcount, rtype_name, root, comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Gather)(sbuf, scount, stype, rbuf, rcount, rtype,
                                       root, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Scatter(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                int rcount, MPI_Datatype rtype, int root, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Scatter (%p,%d,%s,%p,%d,%s,%d,%s)", tm1,
            sbuf, scount, stype_name, rbuf, rcount, rtype_name, root, comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Scatter)(sbuf, scount, stype, rbuf, rcount,
                                        rtype, root, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Gatherv(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                CONST int *rcount, CONST int *displs, MPI_Datatype rtype,
                int root, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Gatherv (%p,%d,%s,%p,%p,%p,%s,%d,%s)", tm1,
            sbuf, scount, stype_name, rbuf, rcount, displs, rtype_name, root, comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Gatherv)(sbuf, scount, stype, rbuf, rcount,
                                        displs, rtype, root, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Scatterv(CONST void *sbuf, CONST int *scount, CONST int *displa,
                 MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype,
                 int root, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Scatterv (%p,%p,%p,%s,%p,%d,%s,%d,%s)", tm1,
            sbuf, scount, displa, stype_name, rbuf, rcount, stype_name, root, comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Scatterv)(sbuf, scount, displa, stype, rbuf,
                                         rcount, rtype, root, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

/*
int MPI_Allgather(CONST void* sbuf, int scount, MPI_Datatype stype, void* rbuf,
int rcount, MPI_Datatype rtype, MPI_Comm comm )  {
    int ret;
    double tm1, tm2;

    #ifndef DISABLE_MPIO_TRACE
    depth++;
    tm1 = recorder_wtime();
    char *comm_name = comm2name(comm);
    char* stype_name = type2name(stype);
    char* rtype_name = type2name(rtype);

    if(__recorderfh != NULL && depth == 1)
        fprintf(__recorderfh,"%.5f MPI_Allgather (%p,%d,%s,%p,%d,%s,%s)", tm1,
sbuf, scount, stype_name, rbuf, rcount, rtype_name, comm_name);

    free(stype_name);
    free(rtype_name);
    free(comm_name);
    #endif

    ret = RECORDER_MPI_CALL(PMPI_Allgather)(sbuf, scount, stype, rbuf, rcount,
rtype, comm);

    #ifndef DISABLE_MPIO_TRACE
    tm2 = recorder_wtime();
    if(__recorderfh != NULL && depth == 1)
        fprintf(__recorderfh, " %d %.5f\n", ret, tm2-tm1);
    depth--;
    #endif

    return(ret);
}
*/

int MPI_Allgatherv(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                   CONST int *rcount, CONST int *displs, MPI_Datatype rtype,
                   MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Allgatherv (%p,%d,%s,%p,%p,%p,%s,%s)", tm1,
            sbuf, scount, stype_name, rbuf, rcount, displs, rtype_name,
            comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Allgatherv)(sbuf, scount, stype, rbuf, rcount,
                                           displs, rtype, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Alltoall(CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                 int rcount, MPI_Datatype rtype, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *stype_name = type2name(stype);
  char *rtype_name = type2name(rtype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Alltoall (%p,%d,%s,%p,%d,%s,%s)", tm1, sbuf,
            scount, stype_name, rbuf, rcount, rtype_name, comm_name);

  free(stype_name);
  free(rtype_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Alltoall)(sbuf, scount, stype, rbuf, rcount,
                                         rtype, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_reduce(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
               MPI_Op op, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *type_name = type2name(stype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_reduce (%p,%p,%d,%s,%d,%s)", tm1, sbuf,
            rbuf, count, type_name, op, comm_name);

  free(type_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Allreduce)((void *)sbuf, rbuf, count, stype, op,
                                          comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Allreduce(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
                  MPI_Op op, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *type_name = type2name(stype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Allreduce (%p,%p,%d,%s,%d,%s)", tm1, sbuf,
            rbuf, count, type_name, op, comm_name);

  free(type_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Allreduce)((void *)sbuf, rbuf, count, stype, op,
                                          comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Reduce_scatter(CONST void *sbuf, void *rbuf, CONST int *rcounts,
                       MPI_Datatype stype, MPI_Op op, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *type_name = type2name(stype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Reduce_scatter (%p,%p,%p,%s,%d,%s)", tm1,
            sbuf, rbuf, rcounts, type_name, op, comm_name);

  free(type_name);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Reduce_scatter)(sbuf, rbuf, rcounts, stype, op,
                                               comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Scan(CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype,
             MPI_Op op, MPI_Comm comm) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  depth++;
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  char *type_name = type2name(stype);

  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, "%.5f MPI_Scan (%p,%p,%d,%s,%d,%s)", tm1, sbuf, rbuf,
            count, type_name, op, comm_name);

  free(type_name);
  free(comm_name);
#endif

  ret =
      RECORDER_MPI_CALL(PMPI_Scan)((void *)sbuf, rbuf, count, stype, op, comm);

#ifndef DISABLE_MPIO_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL && depth == 1)
    fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
  depth--;
#endif

  return (ret);
}

int MPI_Type_create_darray(int size, int rank, int ndims,
                           CONST int array_of_gsizes[],
                           CONST int array_of_distribs[],
                           CONST int array_of_dargs[],
                           CONST int array_of_psizes[], int order,
                           MPI_Datatype oldtype, MPI_Datatype *newtype) {

  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *oldtype_name = type2name(oldtype);
  char arr1[1024], arr2[1024], arr3[1024], arr4[1024];
  print_arr(array_of_gsizes, ndims, arr1);
  print_arr(array_of_distribs, ndims, arr2);
  print_arr(array_of_dargs, ndims, arr3);
  print_arr(array_of_psizes, ndims, arr4);

  if (__recorderfh != NULL)
    fprintf(__recorderfh,
            "%.5f MPI_Type_create_darray (%d,%d,%d,%s,%s,%s,%s,%d,%s,%p)", tm1,
            size, rank, ndims, arr1, arr2, arr3, arr4, order, oldtype_name,
            newtype);
  free(oldtype_name);
#endif

  ret = PMPI_Type_create_darray(size, rank, ndims, array_of_gsizes,
                                array_of_distribs, array_of_dargs,
                                array_of_psizes, order, oldtype, newtype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

  return (ret);
}

int MPI_Type_commit(MPI_Datatype *datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_Type_commit (%p)", tm1, datatype);
#endif

  ret = RECORDER_MPI_CALL(PMPI_Type_commit)(datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  char *typename = type2name(*datatype);
  if (strlen(typename) == 0) {
    char *name = makename(datatype);
    MPI_Type_set_name(*datatype, name);
    free(name);
  }
  fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_open(MPI_Comm comm, CONST char *filename, int amode, MPI_Info info,
                  MPI_File *fh) {
  int ret;
  struct recorder_file_runtime *file;
  char *tmp;
  int comm_size;
  int hash_index;
  uint64_t tmp_hash;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *comm_name = comm2name(comm);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_open (%s,%s,%d,%d,%p)", tm1, comm_name,
            filename, amode, info, fh);
  free(comm_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_open)(comm, (char *)filename, amode, info,
                                          fh);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", *fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_close(MPI_File *fh) {
  int hash_index;
  uint64_t tmp_hash;
  struct recorder_file_runtime *file;
  MPI_File tmp_fh = *fh;
  double tm1, tm2;
  int ret;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_close (%p)", tm1, fh);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_close)(fh);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", tmp_fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_sync(MPI_File fh) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_sync (%d)", tm1, fh);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_sync)(fh);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_set_size(MPI_File fh, MPI_Offset size) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_set_size (%d ,%lld)", tm1, fh, size);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_set_size)(fh, size);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                      MPI_Datatype filetype, CONST char *datarep,
                      MPI_Info info) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *etype_name = type2name(etype);
  char *filetype_name = type2name(filetype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_set_view (%d,%ld,%s,%s,%s,%d)", tm1, fh,
            (long)disp, etype_name, filetype_name, datarep, info);
  free(etype_name);
  free(filetype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_set_view)(fh, disp, etype, filetype,
                                              (void *)datarep, info);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                  MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read (%d,%p,%d,%s,%p)", tm1, fh, buf,
            count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read)(fh, buf, count, datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                     MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_at (%d,%ld,%p,%d,%s,%p)", tm1, fh,
            (long)offset, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_at)(fh, offset, buf, count, datatype,
                                             status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf, int count,
                         MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_at_all (%d,%ld,%p,%d,%s,%p)", tm1,
            fh, (long)offset, buf, count, datatype_name, status);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_at_all)(fh, offset, buf, count,
                                                 datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                      MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_all (%d,%p,%d,%s,%p)", tm1, fh,
            buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_all)(fh, buf, count, datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_shared(MPI_File fh, void *buf, int count,
                         MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_shared (%d,%p,%d,%s,%p)", tm1, fh,
            buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_shared)(fh, buf, count, datatype,
                                                 status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_ordered(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_ordered (%d,%p,%d,%s,%p)", tm1,
            fh, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_ordered)(fh, buf, count, datatype,
                                                  status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                               int count, MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_at_all_begin (%d,%ld,%p,%d,%s)",
            tm1, fh, (long)offset, buf, count, datatype_name);
  free(datatype_name);
#endif
  ret = RECORDER_MPI_CALL(PMPI_File_read_at_all_begin)(fh, offset, buf, count,
                                                       datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_all_begin(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_all_begin (%d,%p,%d,%s)", tm1, fh,
            buf, count, datatype_name);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_read_all_begin)(fh, buf, count, datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_read_ordered_begin(MPI_File fh, void *buf, int count,
                                MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_read_ordered_begin (%d,%p,%d,%s) \n",
            tm1, fh, buf, count, datatype_name);
  free(datatype_name);
#endif

  ret =
      RECORDER_MPI_CALL(PMPI_File_read_ordered_begin)(fh, buf, count, datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf, int count,
                      MPI_Datatype datatype, __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *typename = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_iread_at (%d,%ld,%p,%d,%s,%p)", tm1, fh,
            (long)offset, buf, count, typename, request);
  free(typename);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iread_at)(fh, offset, buf, count, datatype,
                                              request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                   __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *typename = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_iread (%d,%p,%d,%s,%p)", tm1, fh,
            buf, count, typename, request);
  free(typename);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iread)(fh, buf, count, datatype, request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
                          MPI_Datatype datatype, __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *typename = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_iread_shared (%d,%p,%d,%s,%p)", tm1,
            fh, buf, count, typename, request);
  free(typename);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iread_shared)(fh, buf, count, datatype,
                                                  request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write(MPI_File fh, CONST void *buf, int count,
                   MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write (%d,%p,%d,%s,%p)", tm1, fh, buf,
            count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write)(fh, (void *)buf, count, datatype,
                                           status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_at(MPI_File fh, MPI_Offset offset, CONST void *buf,
                      int count, MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write_at (%d,%ld,%p,%d,%s,%p)", tm1, fh,
            (long)offset, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_at)(fh, offset, (void *)buf, count,
                                              datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, CONST void *buf,
                          int count, MPI_Datatype datatype,
                          MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write_at_all (%d,%ld,%p,%d,%s,%p)", tm1,
            fh, (long)offset, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_at_all)(fh, offset, (void *)buf,
                                                  count, datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_all(MPI_File fh, CONST void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write_all (%d,%p,%d,%s,%p)", tm1, fh,
            buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_all)(fh, (void *)buf, count, datatype,
                                               status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_shared(MPI_File fh, CONST void *buf, int count,
                          MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write_shared (%d,%p,%d,%s,%p)", tm1,
            fh, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_shared)(fh, (void *)buf, count,
                                                  datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_ordered(MPI_File fh, CONST void *buf, int count,
                           MPI_Datatype datatype, MPI_Status *status) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  char *datatype_name = type2name(datatype);
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_write_ordered (%d,%p,%d,%s,%p)", tm1,
            fh, buf, count, datatype_name, status);
  free(datatype_name);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_ordered)(fh, (void *)buf, count,
                                                   datatype, status);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, CONST void *buf,
                                int count, MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  char *typename = type2name(datatype);
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh,
            "%.5f MPI_File_write_at_all_begin (%d,%ld,%p,%d,%s) \n", tm1, fh,
            (long)offset, buf, count, typename);
  free(typename);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_at_all_begin)(fh, offset, (void *)buf,
                                                        count, datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_all_begin(MPI_File fh, CONST void *buf, int count,
                             MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh,
            "%.5f MPI_File_write_at_all_begin(fh, buf, count, datatype) \n",
            tm1);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_all_begin)(fh, (void *)buf, count,
                                                     datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_write_ordered_begin(MPI_File fh, CONST void *buf, int count,
                                 MPI_Datatype datatype) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh,
            "%.5f MPI_File_write_ordered_begin(fh, buf, count, datatype) \n",
            tm1);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_write_ordered_begin)(fh, (void *)buf, count,
                                                         datatype);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", fh, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, CONST void *buf,
                       int count, MPI_Datatype datatype,
                       __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(
        __recorderfh,
        "%.5f MPI_File_iwrite_at(fh, offset, buf, count, datatype, request) \n",
        tm1);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iwrite_at)(fh, offset, (void *)buf, count,
                                               datatype, request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iwrite(MPI_File fh, CONST void *buf, int count,
                    MPI_Datatype datatype, __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(
        __recorderfh,
        "%.5f MPI_File_iwrite(fh, offset, buf, count, datatype, request) \n",
        tm1);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iwrite)(fh, (void *)buf, count, datatype,
                                            request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

  return (ret);
}

int MPI_File_iwrite_shared(MPI_File fh, CONST void *buf, int count,
                           MPI_Datatype datatype, __D_MPI_REQUEST *request) {
  int ret;
  double tm1, tm2;

#ifndef DISABLE_MPIO_TRACE
  tm1 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f MPI_File_iwrite_shared(fh, offset, buf, count, "
                          "datatype, request) \n",
            tm1);
#endif

  ret = RECORDER_MPI_CALL(PMPI_File_iwrite_shared)(fh, (void *)buf, count,
                                                   datatype, request);
  tm2 = recorder_wtime();

#ifndef DISABLE_MPIO_TRACE
  fprintf(__recorderfh, " %d %.5f\n", ret, tm2 - tm1);
#endif

  return (ret);
}
