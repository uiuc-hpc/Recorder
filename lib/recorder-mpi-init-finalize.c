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
#define _GNU_SOURCE /* for RTLD_NEXT */

#include <stdlib.h>
#include <stdio.h>

#include "mpi.h"
#include "recorder.h"
#include "recorder-dynamic.h"

#ifdef RECORDER_PRELOAD

#include <dlfcn.h>

#define RECORDER_FORWARD_DECL(name, ret, args) ret(*__real_##name) args = NULL;

#define MAP_OR_FAIL(func)                                                      \
  __real_##func = dlsym(RTLD_NEXT, #func);                                     \
  if (!(__real_##func)) {                                                      \
    fprintf(stderr, "Recorder failed to map symbol: %s\n", #func);              \
  }

RECORDER_FORWARD_DECL(PMPI_File_close, int, (MPI_File * fh));
RECORDER_FORWARD_DECL(PMPI_File_set_size, int, (MPI_File fh, MPI_Offset size));
RECORDER_FORWARD_DECL(PMPI_File_iread_at, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iread, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iread_shared, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite_at, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite_shared, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_open, int,
                      (MPI_Comm comm, char *filename, int amode, MPI_Info info,
                       MPI_File *fh));
RECORDER_FORWARD_DECL(PMPI_File_read_all_begin, int,
                      (MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_all, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_at_all, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_at_all_begin, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_at, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_ordered_begin, int,
                      (MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_ordered, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_shared, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_set_view, int,
                      (MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                       MPI_Datatype filetype, char *datarep, MPI_Info info));
RECORDER_FORWARD_DECL(PMPI_File_sync, int, (MPI_File fh));
RECORDER_FORWARD_DECL(PMPI_File_write_all_begin, int,
                      (MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_all, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_at_all_begin, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_at_all, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_at, int,
                      (MPI_File fh, MPI_Offset offset, void *buf, int count,
                       MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_ordered_begin, int,
                      (MPI_File fh, void *buf, int count,
                       MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_ordered, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_shared, int,
                      (MPI_File fh, void *buf, int count, MPI_Datatype datatype,
                       MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_Finalize, int, ());
RECORDER_FORWARD_DECL(PMPI_Init, int, (int *argc, char ***argv));
RECORDER_FORWARD_DECL(PMPI_Init_thread, int,
                      (int *argc, char ***argv, int required, int *provided));

RECORDER_FORWARD_DECL(PMPI_Wtime, double, ());
RECORDER_FORWARD_DECL(PMPI_Comm_rank, int, (MPI_Comm comm, int *rank));
RECORDER_FORWARD_DECL(PMPI_Comm_size, int, (MPI_Comm comm, int *size));
RECORDER_FORWARD_DECL(PMPI_Barrier, int, (MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Bcast, int,
                      (void *buffer, int count, MPI_Datatype datatype, int root,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Gather, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int rcount, MPI_Datatype rtype, int root,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scatter, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int rcount, MPI_Datatype rtype, int root,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Gatherv, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int *rcount, int *displs, MPI_Datatype rtype, int root,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scatterv, int,
                      (void *sbuf, int *scount, int *displa, MPI_Datatype stype,
                       void *rbuf, int rcount, MPI_Datatype rtype, int root,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allgather, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int rcount, MPI_Datatype rtype, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allgatherv, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int *rcount, int *displs, MPI_Datatype rtype,
                       MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Alltoall, int,
                      (void *sbuf, int scount, MPI_Datatype stype, void *rbuf,
                       int rcount, MPI_Datatype rtype, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Reduce, int,
                      (void *sbuf, void *rbuf, int count, MPI_Datatype stype,
                       MPI_Op op, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allreduce, int,
                      (void *sendbuf, void *recvbuf, int count,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Reduce_scatter, int,
                      (void *sbuf, void *rbuf, int *rcounts, MPI_Datatype stype,
                       MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scan, int,
                      (void *sendbuf, void *recvbuf, int count,
                       MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Type_commit, int, (MPI_Datatype * datatype));
RECORDER_FORWARD_DECL(PMPI_Type_contiguous, int,
                      (int count, MPI_Datatype oldtype, MPI_Datatype *newtype));
RECORDER_FORWARD_DECL(PMPI_Type_extent, int,
                      (MPI_Datatype datatype, MPI_Aint * extent));
RECORDER_FORWARD_DECL(PMPI_Type_free, int, (MPI_Datatype * datatype));
RECORDER_FORWARD_DECL(PMPI_Type_hindexed, int,
                      (int count, int *array_of_blocklengths,
                       MPI_Aint *array_of_displacements, MPI_Datatype oldtype,
                       MPI_Datatype *newtype));
RECORDER_FORWARD_DECL(PMPI_Op_create, int,
                      (MPI_User_function * function, int commute, MPI_Op *op));
RECORDER_FORWARD_DECL(PMPI_Op_free, int, (MPI_Op * op));
RECORDER_FORWARD_DECL(PMPI_Type_get_envelope, int,
                      (MPI_Datatype datatype, int *num_integers,
                       int *num_addresses, int *num_datatypes, int *combiner));
RECORDER_FORWARD_DECL(PMPI_Type_size, int, (MPI_Datatype datatype, int *size));

char *comm2name(MPI_Comm comm) {
  char *tmp = (char *)malloc(1024);
  int len;
  PMPI_Comm_get_name(comm, tmp, &len);
  tmp[len] = 0;
  if(len == 0) strcpy(tmp, "MPI_COMM_UNKNOWN");
  return tmp;
}

char *type2name(MPI_Datatype type) {
  char *tmp = (char *)malloc(1024);
  int len;
  PMPI_Type_get_name(type, tmp, &len);
  tmp[len] = 0;

  return tmp;
}

char *makename(MPI_Datatype *type) {
  char *tmp = (char *)malloc(1024);
  sprintf(tmp, "RECORDER_TYPE_%p", type);
  return tmp;
}

void resolve_mpi_symbols(void) {
  /*
   * Overloaded functions
   */
  MAP_OR_FAIL(PMPI_File_close);
  MAP_OR_FAIL(PMPI_File_set_size);
  MAP_OR_FAIL(PMPI_File_iread_at);
  MAP_OR_FAIL(PMPI_File_iread);
  MAP_OR_FAIL(PMPI_File_iread_shared);
  MAP_OR_FAIL(PMPI_File_iwrite_at);
  MAP_OR_FAIL(PMPI_File_iwrite);
  MAP_OR_FAIL(PMPI_File_iwrite_shared);
  MAP_OR_FAIL(PMPI_File_open);
  MAP_OR_FAIL(PMPI_File_read_all_begin);
  MAP_OR_FAIL(PMPI_File_read_all);
  MAP_OR_FAIL(PMPI_File_read_at_all_begin);
  MAP_OR_FAIL(PMPI_File_read_at_all);
  MAP_OR_FAIL(PMPI_File_read_at);
  MAP_OR_FAIL(PMPI_File_read);
  MAP_OR_FAIL(PMPI_File_read_ordered_begin);
  MAP_OR_FAIL(PMPI_File_read_ordered);
  MAP_OR_FAIL(PMPI_File_read_shared);
  MAP_OR_FAIL(PMPI_File_set_view);
  MAP_OR_FAIL(PMPI_File_sync);
  MAP_OR_FAIL(PMPI_File_write_all_begin);
  MAP_OR_FAIL(PMPI_File_write_all);
  MAP_OR_FAIL(PMPI_File_write_at_all_begin);
  MAP_OR_FAIL(PMPI_File_write_at_all);
  MAP_OR_FAIL(PMPI_File_write_at);
  MAP_OR_FAIL(PMPI_File_write);
  MAP_OR_FAIL(PMPI_File_write_ordered_begin);
  MAP_OR_FAIL(PMPI_File_write_ordered);
  MAP_OR_FAIL(PMPI_File_write_shared);
  MAP_OR_FAIL(PMPI_Finalize);
  MAP_OR_FAIL(PMPI_Allgather);
  MAP_OR_FAIL(PMPI_Init);
  MAP_OR_FAIL(PMPI_Init_thread);

  /*
   * These function are not intercepted but are used
   * by recorder itself.
   */
  MAP_OR_FAIL(PMPI_Wtime);
  MAP_OR_FAIL(PMPI_Allreduce);
  MAP_OR_FAIL(PMPI_Bcast);
  MAP_OR_FAIL(PMPI_Comm_rank);
  MAP_OR_FAIL(PMPI_Comm_size);
  MAP_OR_FAIL(PMPI_Scan);
  MAP_OR_FAIL(PMPI_Type_commit);
  MAP_OR_FAIL(PMPI_Type_contiguous);
  MAP_OR_FAIL(PMPI_Type_extent);
  MAP_OR_FAIL(PMPI_Type_free);
  MAP_OR_FAIL(PMPI_Type_size);
  MAP_OR_FAIL(PMPI_Type_hindexed);
  MAP_OR_FAIL(PMPI_Op_create);
  MAP_OR_FAIL(PMPI_Op_free);
  MAP_OR_FAIL(PMPI_Reduce);
  MAP_OR_FAIL(PMPI_Type_get_envelope);

  return;
}

#endif

int PMPI_Init(int *argc, char ***argv) {
  int ret;

#ifdef RECORDER_PRELOAD
  resolve_mpi_symbols();
#endif

  ret = RECORDER_MPI_CALL(PMPI_Init)(argc, argv);
  if (ret != MPI_SUCCESS) {
    return (ret);
  }

  recorder_mpi_initialize(argc, argv);

  return (ret);
}

int MPI_Init(int *argc, char ***argv) {
  int ret;

#ifdef RECORDER_PRELOAD
  resolve_mpi_symbols();
#endif

  ret = RECORDER_MPI_CALL(PMPI_Init)(argc, argv);
  if (ret != MPI_SUCCESS) {
    return (ret);
  }

  recorder_mpi_initialize(argc, argv);

  return (ret);
}

int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
  int ret;

#ifdef RECORDER_PRELOAD
  resolve_mpi_symbols();
#endif

  ret = RECORDER_MPI_CALL(PMPI_Init_thread)(argc, argv, required, provided);
  if (ret != MPI_SUCCESS) {
    return (ret);
  }

  recorder_mpi_initialize(argc, argv);

  return (ret);
}

int MPI_Finalize(void) {
  int ret;

  /*
if(getenv("RECORDER_INTERNAL_TIMING"))
  recorder_shutdown(1);
else
  recorder_shutdown(0);
  */

  ret = RECORDER_MPI_CALL(PMPI_Finalize)();

  return (ret);
}
