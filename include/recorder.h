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

#ifndef __RECORDER_H
#define __RECORDER_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <stdint.h>
#include <mpi.h>
#include "recorder-log-format.h"
#include "hashmap.h"

#define __D_MPI_REQUEST MPIO_Request
#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif

extern FILE* __recorderfh;          /* file handler for each log file */
extern int depth;                   /* funciton call depth */


char* comm2name(MPI_Comm comm);
char* type2name(MPI_Datatype type);
char* makename(MPI_Datatype *type);
double recorder_wtime(void);
char* fd2name(int fd);

void logger_init();
void write_data_operation(const char *func, const char *filename, double start, double end, size_t attr1, size_t attr2, const char *log);



#ifdef RECORDER_PRELOAD

    /*
     * Declare the function signature for all real functions
     * e.g. The real function point to fwrite would be defined as __real_fwrite
     */
    #define RECORDER_FORWARD_DECL(name, ret, args) ret(*__real_##name) args;

    /* Point __read_func to the real funciton using dlsym() */
    #define MAP_OR_FAIL(func)                                                   \
        if (!(__real_##func)) {                                                 \
            __real_##func = dlsym(RTLD_NEXT, #func);                            \
            if (!(__real_##func)) {                                             \
                fprintf(stderr, "Recorder failed to map symbol: %s\n", #func);  \
            }                                                                   \
        }
    /*
     * Call the real funciton
     * Before call the real function, we need to make sure its mapped
     * which means, every time we use this marco directly, we need to call MAP_OR_FAIL before it
     */
    #define RECORDER_MPI_CALL(func) __real_##func

#endif


/* POSIX I/O */
RECORDER_FORWARD_DECL(creat, int, (const char *path, mode_t mode));
RECORDER_FORWARD_DECL(creat64, int, (const char *path, mode_t mode));
RECORDER_FORWARD_DECL(open, int, (const char *path, int flags, ...));
RECORDER_FORWARD_DECL(open64, int, (const char *path, int flags, ...));
RECORDER_FORWARD_DECL(close, int, (int fd));
RECORDER_FORWARD_DECL(write, ssize_t, (int fd, const void *buf, size_t count));
RECORDER_FORWARD_DECL(read, ssize_t, (int fd, void *buf, size_t count));
RECORDER_FORWARD_DECL(lseek, off_t, (int fd, off_t offset, int whence));
RECORDER_FORWARD_DECL(lseek64, off64_t, (int fd, off64_t offset, int whence));
RECORDER_FORWARD_DECL(pread, ssize_t, (int fd, void *buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pread64, ssize_t, (int fd, void *buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(pwrite, ssize_t, (int fd, const void *buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pwrite64, ssize_t, (int fd, const void *buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(readv, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
RECORDER_FORWARD_DECL(writev, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
RECORDER_FORWARD_DECL(__fxstat, int, (int vers, int fd, struct stat *buf));
RECORDER_FORWARD_DECL(__fxstat64, int, (int vers, int fd, struct stat64 *buf));
RECORDER_FORWARD_DECL(__lxstat, int, (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__lxstat64, int, (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(__xstat, int, (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__xstat64, int, (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(mmap, void *, (void *addr, size_t length, int prot, int flags, int fd, off_t offset));
RECORDER_FORWARD_DECL(mmap64, void *, (void *addr, size_t length, int prot, int flags, int fd, off64_t offset));
RECORDER_FORWARD_DECL(fopen, FILE *, (const char *path, const char *mode));
RECORDER_FORWARD_DECL(fopen64, FILE *, (const char *path, const char *mode));
RECORDER_FORWARD_DECL(fclose, int, (FILE * fp));
RECORDER_FORWARD_DECL(fread, size_t, (void *ptr, size_t size, size_t nmemb, FILE *stream));
RECORDER_FORWARD_DECL(fwrite, size_t, (const void *ptr, size_t size, size_t nmemb, FILE *stream));
RECORDER_FORWARD_DECL(fseek, int, (FILE * stream, long offset, int whence));
RECORDER_FORWARD_DECL(fsync, int, (int fd));
RECORDER_FORWARD_DECL(fdatasync, int, (int fd));

/* MPI I/O */
RECORDER_FORWARD_DECL(PMPI_File_close, int, (MPI_File * fh));
RECORDER_FORWARD_DECL(PMPI_File_set_size, int, (MPI_File fh, MPI_Offset size));
RECORDER_FORWARD_DECL(PMPI_File_iread_at, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iread, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iread_shared, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite_at, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_iwrite_shared, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
RECORDER_FORWARD_DECL(PMPI_File_open, int, (MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh));
RECORDER_FORWARD_DECL(PMPI_File_read_all_begin, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_all, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_at_all, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_at_all_begin, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_at, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_ordered_begin, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_read_ordered, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_read_shared, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_set_view, int, (MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, CONST char *datarep, MPI_Info info));
RECORDER_FORWARD_DECL(PMPI_File_sync, int, (MPI_File fh));
RECORDER_FORWARD_DECL(PMPI_File_write_all_begin, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_all, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_at_all_begin, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_at_all, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_at, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_ordered_begin, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype));
RECORDER_FORWARD_DECL(PMPI_File_write_ordered, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_File_write_shared, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
RECORDER_FORWARD_DECL(PMPI_Finalize, int, ());
RECORDER_FORWARD_DECL(PMPI_Init, int, (int *argc, char ***argv));
RECORDER_FORWARD_DECL(PMPI_Init_thread, int, (int *argc, char ***argv, int required, int *provided));
RECORDER_FORWARD_DECL(PMPI_Wtime, double, ());
RECORDER_FORWARD_DECL(PMPI_Comm_rank, int, (MPI_Comm comm, int *rank));
RECORDER_FORWARD_DECL(PMPI_Comm_size, int, (MPI_Comm comm, int *size));
RECORDER_FORWARD_DECL(PMPI_Barrier, int, (MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Bcast, int, (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Gather, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Gatherv, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scatter, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scatterv, int, (CONST void *sbuf, CONST int *scount, CONST int *displa, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allgather, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allgatherv, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Alltoall, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Reduce, int, (CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, int root, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Allreduce, int, (CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Reduce_scatter, int, (CONST void *sbuf, void *rbuf, CONST int *rcounts, MPI_Datatype stype, MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Scan, int, (CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
RECORDER_FORWARD_DECL(PMPI_Type_commit, int, (MPI_Datatype * datatype));
RECORDER_FORWARD_DECL(PMPI_Type_contiguous, int, (int count, MPI_Datatype oldtype, MPI_Datatype *newtype));
RECORDER_FORWARD_DECL(PMPI_Type_extent, int, (MPI_Datatype datatype, MPI_Aint * extent));
RECORDER_FORWARD_DECL(PMPI_Type_free, int, (MPI_Datatype * datatype));
RECORDER_FORWARD_DECL(PMPI_Type_hindexed, int, (int count, int *array_of_blocklengths, MPI_Aint *array_of_displacements, MPI_Datatype oldtype, MPI_Datatype *newtype));
RECORDER_FORWARD_DECL(PMPI_Op_create, int, (MPI_User_function * function, int commute, MPI_Op *op));
RECORDER_FORWARD_DECL(PMPI_Op_free, int, (MPI_Op * op));
RECORDER_FORWARD_DECL(PMPI_Type_get_envelope, int, (MPI_Datatype datatype, int *num_integers, int *num_addresses, int *num_datatypes, int *combiner));
RECORDER_FORWARD_DECL(PMPI_Type_size, int, (MPI_Datatype datatype, int *size));


#endif /* __RECORDER_H */
