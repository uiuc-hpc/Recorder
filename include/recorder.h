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


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <utime.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <stdint.h>
#include <mpi.h>
#include "hdf5.h"
#include "recorder-log-format.h"
#include "hashmap.h"

#define __D_MPI_REQUEST MPIO_Request
#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif

extern int depth;                   /* funciton call depth */

double recorder_wtime(void);

void logger_init(int rank);
void logger_exit();
void write_data_operation(const char *func, const char *filename, double start, double end, size_t attr1, size_t attr2, const char *log);


#ifdef RECORDER_PRELOAD
    #include <dlfcn.h>
    /*
     * Declare the function signature for real functions
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

    /*
     * Overwrite the real function calls
     * This is a convienent marco so that
     * we do not intercept functions if RECORDER_PRELOAD is not defined
     */
    #define RECORDER_DECL(func) func

#else
    #define RECORDER_FORWARD_DECL(name, ret, args)
    #define MAP_OR_FAIL(func)
    #define RECORDER_MPI_CALL(func) func
    #define RECORDER_DECL(func) __warp_##func
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
// stat/fstat/lstat are wrappers in GLIBC and dlsym can not hook them.
// Instead, xstat/lxstat/fxstat are their GLIBC implementations so we can hook them.
RECORDER_FORWARD_DECL(__xstat, int, (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__xstat64, int, (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(__lxstat, int, (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__lxstat64, int, (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(__fxstat, int, (int vers, int fd, struct stat *buf));
RECORDER_FORWARD_DECL(__fxstat64, int, (int vers, int fd, struct stat64 *buf));

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

/* Other POSIX Function Calls, not directly related to I/O */
// Files and Directories
RECORDER_FORWARD_DECL(getcwd, char*, (char *buf, size_t size));
RECORDER_FORWARD_DECL(mkdir, int, (const char *pathname, mode_t mode));
RECORDER_FORWARD_DECL(rmdir, int, (const char *pathname));
RECORDER_FORWARD_DECL(chdir, int, (const char *path));
RECORDER_FORWARD_DECL(link, int, (const char *oldpath, const char *newpath));
RECORDER_FORWARD_DECL(unlink, int, (const char *pathname));
RECORDER_FORWARD_DECL(rename, int, (const char *oldpath, const char *newpath));
RECORDER_FORWARD_DECL(chmod, int, (const char *path, mode_t mode));
RECORDER_FORWARD_DECL(chown, int, (const char *path, uid_t owner, gid_t group));
RECORDER_FORWARD_DECL(utime, int, (const char *filename, const struct utimbuf *buf));
RECORDER_FORWARD_DECL(opendir, DIR*, (const char *name));
RECORDER_FORWARD_DECL(readdir, struct dirent*, (DIR *dir));
RECORDER_FORWARD_DECL(closedir, int, (DIR *dir));
RECORDER_FORWARD_DECL(rewinddir, void, (DIR *dir));
// Advanced File Operations
RECORDER_FORWARD_DECL(fcntl, int, (int fd, int cmd, ...));
RECORDER_FORWARD_DECL(dup, int, (int oldfd));
RECORDER_FORWARD_DECL(dup2, int, (int oldfd, int newfd));
RECORDER_FORWARD_DECL(pipe, int, (int pipefd[2]));
RECORDER_FORWARD_DECL(mkfifo, int, (const char *pathname, mode_t mode));
RECORDER_FORWARD_DECL(umask, mode_t, (mode_t mask));
RECORDER_FORWARD_DECL(fdopen, FILE*, (int fd, const char *mode));
RECORDER_FORWARD_DECL(fileno, int, (FILE *stream));


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
RECORDER_FORWARD_DECL(PMPI_Finalized, int, (int *flag));
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
RECORDER_FORWARD_DECL(PMPI_Type_create_darray, int, (int size, int rank, int ndims, CONST int array_of_gsizes[], CONST int array_of_distribs[],CONST int array_of_dargs[], CONST int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype));


/* NOTE: using HDF5 1.8 version */

/* File Interface */
RECORDER_FORWARD_DECL(H5Fcreate, hid_t, (const char *filename, unsigned flags, hid_t create_plist, hid_t access_plist));
RECORDER_FORWARD_DECL(H5Fopen, hid_t, (const char *filename, unsigned flags, hid_t access_plist));
RECORDER_FORWARD_DECL(H5Fclose, herr_t, (hid_t file_id));

/* Group Interface */
RECORDER_FORWARD_DECL(H5Gclose, herr_t, (hid_t group_id));
RECORDER_FORWARD_DECL(H5Gcreate1, hid_t, (hid_t loc_id, const char *name, size_t size_hint));
RECORDER_FORWARD_DECL(H5Gcreate2, hid_t, (hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id));
RECORDER_FORWARD_DECL(H5Gget_objinfo, herr_t, (hid_t loc_id, const char *name, hbool_t follow_link, H5G_stat_t *statbuf));
RECORDER_FORWARD_DECL(H5Giterate, int, (hid_t loc_id, const char *name, int *idx, H5G_iterate_t operator, void *operator_data));
RECORDER_FORWARD_DECL(H5Gopen1, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Gopen2, hid_t, (hid_t loc_id, const char *name, hid_t gapl_id));

/* Dataset Interface  */
RECORDER_FORWARD_DECL(H5Dclose, herr_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dcreate1, hid_t, (hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id));
RECORDER_FORWARD_DECL(H5Dcreate2, hid_t, (hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id));
RECORDER_FORWARD_DECL(H5Dget_create_plist, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dget_space, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dget_type, hid_t, (hid_t dataset_id));
RECORDER_FORWARD_DECL(H5Dopen1, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Dopen2, hid_t, (hid_t loc_id, const char *name, hid_t dapl_id));
RECORDER_FORWARD_DECL(H5Dread, herr_t, (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void *buf));
RECORDER_FORWARD_DECL(H5Dwrite, herr_t, (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void *buf));

/* Dataspace Interface */
RECORDER_FORWARD_DECL(H5Sclose, herr_t, (hid_t space_id));
RECORDER_FORWARD_DECL(H5Screate, hid_t, (H5S_class_t type));

RECORDER_FORWARD_DECL(H5Screate_simple, hid_t, (int rank, const hsize_t *current_dims, const hsize_t *maximum_dims));
RECORDER_FORWARD_DECL(H5Sget_select_npoints, hssize_t, (hid_t space_id));
RECORDER_FORWARD_DECL(H5Sget_simple_extent_dims, int, (hid_t space_id, hsize_t * dims, hsize_t * maxdims));
RECORDER_FORWARD_DECL(H5Sget_simple_extent_npoints, hssize_t, (hid_t space_id));
RECORDER_FORWARD_DECL(H5Sselect_elements, herr_t, (hid_t space_id, H5S_seloper_t op, size_t num_elements, const hsize_t *coord));
RECORDER_FORWARD_DECL(H5Sselect_hyperslab, herr_t, (hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block));
RECORDER_FORWARD_DECL(H5Sselect_none, herr_t, (hid_t space_id));

/* Datatype Interface */
RECORDER_FORWARD_DECL(H5Tclose, herr_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tcopy, hid_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tget_class, H5T_class_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tget_size, size_t, (hid_t dtype_id));
RECORDER_FORWARD_DECL(H5Tset_size, herr_t, (hid_t dtype_id, size_t size));
RECORDER_FORWARD_DECL(H5Tcreate, hid_t, (H5T_class_t class, size_t size));
RECORDER_FORWARD_DECL(H5Tinsert, herr_t, (hid_t dtype_id, const char *name, size_t offset, hid_t field_id));

/* Attribute Interface */
RECORDER_FORWARD_DECL(H5Aclose, herr_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Acreate1, hid_t, (hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id));
RECORDER_FORWARD_DECL(H5Acreate2, hid_t, (hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id));
RECORDER_FORWARD_DECL(H5Aget_name, ssize_t, (hid_t attr_id, size_t buf_size, char *buf));
RECORDER_FORWARD_DECL(H5Aget_num_attrs, int, (hid_t loc_id));
RECORDER_FORWARD_DECL(H5Aget_space, hid_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Aget_type, hid_t, (hid_t attr_id));
RECORDER_FORWARD_DECL(H5Aopen, hid_t, (hid_t obj_id, const char *attr_name, hid_t aapl_id));
RECORDER_FORWARD_DECL(H5Aopen_idx, hid_t, (hid_t loc_id, unsigned int idx));
RECORDER_FORWARD_DECL(H5Aopen_name, hid_t, (hid_t loc_id, const char *name));
RECORDER_FORWARD_DECL(H5Aread, herr_t, (hid_t attr_id, hid_t mem_type_id, void *buf));
RECORDER_FORWARD_DECL(H5Awrite, herr_t, (hid_t attr_id, hid_t mem_type_id, const void *buf));

/* Property List Interface */
RECORDER_FORWARD_DECL(H5Pclose, herr_t, (hid_t plist));
RECORDER_FORWARD_DECL(H5Pcreate, hid_t, (hid_t cls_id));
RECORDER_FORWARD_DECL(H5Pget_chunk, int, (hid_t plist, int max_ndims, hsize_t *dims));
RECORDER_FORWARD_DECL(H5Pget_mdc_config, herr_t, (hid_t plist_id, H5AC_cache_config_t * config_ptr));
RECORDER_FORWARD_DECL(H5Pset_alignment, herr_t, (hid_t plist, hsize_t threshold, hsize_t alignment));
RECORDER_FORWARD_DECL(H5Pset_chunk, herr_t, (hid_t plist, int ndims, const hsize_t *dim));
RECORDER_FORWARD_DECL(H5Pset_dxpl_mpio, herr_t, (hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode));
RECORDER_FORWARD_DECL(H5Pset_fapl_core, herr_t, (hid_t fapl_id, size_t increment, hbool_t backing_store));
RECORDER_FORWARD_DECL(H5Pset_fapl_mpio, herr_t, (hid_t fapl_id, MPI_Comm comm, MPI_Info info));
RECORDER_FORWARD_DECL(H5Pset_fapl_mpiposix, herr_t, (hid_t fapl_id, MPI_Comm comm, hbool_t use_gpfs_hints));
RECORDER_FORWARD_DECL(H5Pset_istore_k, herr_t, (hid_t plist, unsigned ik));
RECORDER_FORWARD_DECL(H5Pset_mdc_config, herr_t, (hid_t plist_id, H5AC_cache_config_t * config_ptr));
RECORDER_FORWARD_DECL(H5Pset_meta_block_size, herr_t, (hid_t fapl_id, hsize_t size));

/* Link Interface */
RECORDER_FORWARD_DECL(H5Lexists, htri_t, (hid_t loc_id, const char *name, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Lget_val, herr_t, (hid_t link_loc_id, const char *link_name, void *linkval_buff, size_t size, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Literate, herr_t, (hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t * idx, H5L_iterate_t op, void *op_data));

/* Object Interface */
RECORDER_FORWARD_DECL(H5Oclose, herr_t, (hid_t object_id));
RECORDER_FORWARD_DECL(H5Oget_info, herr_t, (hid_t object_id, H5O_info_t * object_info));
RECORDER_FORWARD_DECL(H5Oget_info_by_name, herr_t, (hid_t loc_id, const char *object_name, H5O_info_t *object_info, hid_t lapl_id));
RECORDER_FORWARD_DECL(H5Oopen, hid_t, (hid_t loc_id, const char *name, hid_t lapl_id));


#endif /* __RECORDER_H */
