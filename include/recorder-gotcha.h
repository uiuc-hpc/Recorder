#ifndef __RECORDER_GOTCHA_H
#define __RECORDER_GOTCHA_H
#include <sys/uio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <utime.h>
#include "mpi.h"
#include "hdf5.h"
#include "gotcha/gotcha.h"
#include "recorder-logger.h"

#define __D_MPI_REQUEST MPIO_Request
#if MPI_VERSION >= 3
#define CONST const
#else
#define CONST
#endif

void gotcha_init();

/**
 * WRAPPER_TYPE:   type of function pointer of the wrapper
 * WRAPPEE_HANDLE: wrapee handle name 
 * WRAPPER_NAME:   wrapper name 
 */
#define WRAPPER_TYPE(func)   fptr_type_##func
#define WRAPPEE_HANDLE(func) wrappee_handle_##func
#define WRAPPER_NAME(func)   wrapper_##func

/*
 * The real call
 */
#define GOTCHA_REAL_CALL(func)   real_##func

#define GOTCHA_WRAP(func, ret, args)                                    \
    gotcha_wrappee_handle_t WRAPPEE_HANDLE(func);                       \
    typedef ret (*WRAPPER_TYPE(func)) args;                             \
    ret (*GOTCHA_REAL_CALL(func)) args;                                 \
    ret WRAPPER_NAME(func) args;

/*
 * Used in recorder-gotcha.c to define binding structures
 */
#define GOTCHA_WRAP_ACTION(func)                                        \
    {#func, WRAPPER_NAME(func), &WRAPPEE_HANDLE(func)}

#define GOTCHA_SET_REAL_CALL_NOCHECK(func)                              \
    do {                                                                \
        void* funcptr = gotcha_get_wrappee(WRAPPEE_HANDLE(func));       \
        GOTCHA_REAL_CALL(func) = (WRAPPER_TYPE(func)) (funcptr);        \
    } while(0);

#define GOTCHA_SET_REAL_CALL(func, func_layer)                          \
    do {                                                                \
        int intercept = 1;                                              \
        if (getenv(func_layer) != NULL) {                               \
            if (atoi(getenv(func_layer)) == 0)                          \
                intercept = 0;                                          \
        }                                                               \
        if (intercept) {                                                \
            void* funcptr = gotcha_get_wrappee(WRAPPEE_HANDLE(func));   \
            GOTCHA_REAL_CALL(func) = (WRAPPER_TYPE(func)) (funcptr);    \
        } else {                                                        \
            GOTCHA_REAL_CALL(func) = func;                              \
        }                                                               \
    } while(0);


/* POSIX I/O */
GOTCHA_WRAP(creat, int, (const char *path, mode_t mode));
GOTCHA_WRAP(creat64, int, (const char *path, mode_t mode));
GOTCHA_WRAP(open, int, (const char *path, int flags, ...));
GOTCHA_WRAP(open64, int, (const char *path, int flags, ...));
GOTCHA_WRAP(close, int, (int fd));
GOTCHA_WRAP(write, ssize_t, (int fd, const void *buf, size_t count));
GOTCHA_WRAP(read, ssize_t, (int fd, void *buf, size_t count));
GOTCHA_WRAP(lseek, off_t, (int fd, off_t offset, int whence));
GOTCHA_WRAP(lseek64, off64_t, (int fd, off64_t offset, int whence));
GOTCHA_WRAP(pread, ssize_t, (int fd, void *buf, size_t count, off_t offset));
GOTCHA_WRAP(pread64, ssize_t, (int fd, void *buf, size_t count, off64_t offset));
GOTCHA_WRAP(pwrite, ssize_t, (int fd, const void *buf, size_t count, off_t offset));
GOTCHA_WRAP(pwrite64, ssize_t, (int fd, const void *buf, size_t count, off64_t offset));
GOTCHA_WRAP(readv, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
GOTCHA_WRAP(writev, ssize_t, (int fd, const struct iovec *iov, int iovcnt));
GOTCHA_WRAP(mmap, void *, (void *addr, size_t length, int prot, int flags, int fd, off_t offset));
GOTCHA_WRAP(mmap64, void *, (void *addr, size_t length, int prot, int flags, int fd, off64_t offset));
GOTCHA_WRAP(msync, int, (void *addr, size_t length, int flags));
GOTCHA_WRAP(fopen, FILE *, (const char *path, const char *mode));
GOTCHA_WRAP(fopen64, FILE *, (const char *path, const char *mode));
GOTCHA_WRAP(fclose, int, (FILE * fp));
GOTCHA_WRAP(fread, size_t, (void *ptr, size_t size, size_t nmemb, FILE *stream));
GOTCHA_WRAP(fwrite, size_t, (const void *ptr, size_t size, size_t nmemb, FILE *stream));
GOTCHA_WRAP(ftell, long, (FILE *stream));
GOTCHA_WRAP(fseek, int, (FILE * stream, long offset, int whence));
GOTCHA_WRAP(fsync, int, (int fd));
GOTCHA_WRAP(fdatasync, int, (int fd));
// we need to use vprintf to trace fprintf so we can pass va_list
//GOTCHA_WRAP(vfprintf, int, (FILE *stream, const char *format, va_list ap));
// stat/fstat/lstat are wrappers in GLIBC and dlsym can not hook them.
// Instead, xstat/lxstat/fxstat are their GLIBC implementations so we can hook them.
GOTCHA_WRAP(__xstat, int, (int vers, const char *path, struct stat *buf));
GOTCHA_WRAP(__xstat64, int, (int vers, const char *path, struct stat64 *buf));
GOTCHA_WRAP(__lxstat, int, (int vers, const char *path, struct stat *buf));
GOTCHA_WRAP(__lxstat64, int, (int vers, const char *path, struct stat64 *buf));
GOTCHA_WRAP(__fxstat, int, (int vers, int fd, struct stat *buf));
GOTCHA_WRAP(__fxstat64, int, (int vers, int fd, struct stat64 *buf));
/* Other POSIX Function Calls, not directly related to I/O */
// Files and Directories
GOTCHA_WRAP(getcwd, char*, (char *buf, size_t size));
GOTCHA_WRAP(mkdir, int, (const char *pathname, mode_t mode));
GOTCHA_WRAP(rmdir, int, (const char *pathname));
GOTCHA_WRAP(chdir, int, (const char *path));
GOTCHA_WRAP(link, int, (const char *oldpath, const char *newpath));
GOTCHA_WRAP(linkat, int, (int fd1, const char *path1, int fd2, const char *path2, int flag));
GOTCHA_WRAP(unlink, int, (const char *pathname));
GOTCHA_WRAP(symlink, int, (const char *path1, const char *path2));
GOTCHA_WRAP(symlinkat, int, (const char *path1, int fd, const char *path2));
GOTCHA_WRAP(readlink, ssize_t, (const char *path, char *buf, size_t bufsize));
GOTCHA_WRAP(readlinkat, ssize_t, (int fd, const char *path, char *buf, size_t bufsize));
GOTCHA_WRAP(rename, int, (const char *oldpath, const char *newpath));
GOTCHA_WRAP(chmod, int, (const char *path, mode_t mode));
GOTCHA_WRAP(chown, int, (const char *path, uid_t owner, gid_t group));
GOTCHA_WRAP(lchown, int, (const char *path, uid_t owner, gid_t group));
GOTCHA_WRAP(utime, int, (const char *filename, const struct utimbuf *buf));
GOTCHA_WRAP(opendir, DIR*, (const char *name));
GOTCHA_WRAP(readdir, struct dirent*, (DIR *dir));
GOTCHA_WRAP(closedir, int, (DIR *dir));
// GOTCHA_WRAP(rewinddir, void, (DIR *dir));
// GOTCHA_WRAP(__xmknod, int, (int ver, const char *path, mode_t mode, dev_t dev));
// GOTCHA_WRAP(__xmknodat, int, (int ver, int fd, const char *path, mode_t mode, dev_t dev));
// Advanced File Operations
GOTCHA_WRAP(fcntl, int, (int fd, int cmd, ...));
GOTCHA_WRAP(dup, int, (int oldfd));
GOTCHA_WRAP(dup2, int, (int oldfd, int newfd));
GOTCHA_WRAP(pipe, int, (int pipefd[2]));
GOTCHA_WRAP(mkfifo, int, (const char *pathname, mode_t mode));
GOTCHA_WRAP(umask, mode_t, (mode_t mask));
GOTCHA_WRAP(fdopen, FILE*, (int fd, const char *mode));
GOTCHA_WRAP(fileno, int, (FILE *stream));
GOTCHA_WRAP(access, int, (const char *path, int amode));
GOTCHA_WRAP(faccessat, int, (int fd, const char *path, int amode, int flag));
GOTCHA_WRAP(tmpfile, FILE*, (void));
GOTCHA_WRAP(remove, int, (const char *pathname));
GOTCHA_WRAP(truncate, int, (const char *pathname, off_t length));
GOTCHA_WRAP(ftruncate, int, (int fd, off_t length));
// Added 01/15/2021
GOTCHA_WRAP(fseeko, int, (FILE *stream, off_t offset, int whence));
GOTCHA_WRAP(ftello, off_t, (FILE *stream));
// Added 10/12/2023
GOTCHA_WRAP(fflush, int, (FILE *stream));

// Others
//int statfs(const char *path, struct statfs *buf);
//int fstatfs(int fd, struct statfs *buf);




/* MPI Function Calls */

GOTCHA_WRAP(MPI_File_close, int, (MPI_File * fh))
GOTCHA_WRAP(MPI_File_set_size, int, (MPI_File fh, MPI_Offset size));
GOTCHA_WRAP(MPI_File_iread_at, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_iread, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_iread_shared, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_iwrite_at, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_iwrite, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_iwrite_shared, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, __D_MPI_REQUEST *request));
GOTCHA_WRAP(MPI_File_open, int, (MPI_Comm comm, CONST char *filename, int amode, MPI_Info info, MPI_File *fh));
GOTCHA_WRAP(MPI_File_read_all_begin, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_read_all, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_read_at_all, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_read_at_all_begin, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_read_at, int, (MPI_File fh, MPI_Offset offset, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_read, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_read_ordered_begin, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_read_ordered, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_read_shared, int, (MPI_File fh, void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_set_view, int, (MPI_File fh, MPI_Offset disp, MPI_Datatype etype, MPI_Datatype filetype, CONST char *datarep, MPI_Info info));
GOTCHA_WRAP(MPI_File_sync, int, (MPI_File fh));
GOTCHA_WRAP(MPI_File_write_all_begin, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_write_all, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_write_at_all_begin, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_write_at_all, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_write_at, int, (MPI_File fh, MPI_Offset offset, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_write, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_write_ordered_begin, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype));
GOTCHA_WRAP(MPI_File_write_ordered, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_File_write_shared, int, (MPI_File fh, CONST void *buf, int count, MPI_Datatype datatype, MPI_Status *status));
GOTCHA_WRAP(MPI_Finalized, int, (int *flag));
/*
GOTCHA_WRAP(MPI_Finalize, int, ());
GOTCHA_WRAP(MPI_Init, int, (int *argc, char ***argv));
GOTCHA_WRAP(MPI_Init_thread, int, (int *argc, char ***argv, int required, int *provided));
*/
GOTCHA_WRAP(MPI_Comm_rank, int, (MPI_Comm comm, int *rank));
GOTCHA_WRAP(MPI_Comm_size, int, (MPI_Comm comm, int *size));
GOTCHA_WRAP(MPI_Get_processor_name, int, (char *name, int *resultlen));
GOTCHA_WRAP(MPI_Comm_set_errhandler, int, (MPI_Comm comm, MPI_Errhandler errhandler));
GOTCHA_WRAP(MPI_Barrier, int, (MPI_Comm comm));
GOTCHA_WRAP(MPI_Bcast, int, (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Gather, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Gatherv, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Scatter, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Scatterv, int, (CONST void *sbuf, CONST int *scount, CONST int *displa, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Allgather, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm));
GOTCHA_WRAP(MPI_Allgatherv, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, CONST int *rcount, CONST int *displs, MPI_Datatype rtype, MPI_Comm comm));
GOTCHA_WRAP(MPI_Alltoall, int, (CONST void *sbuf, int scount, MPI_Datatype stype, void *rbuf, int rcount, MPI_Datatype rtype, MPI_Comm comm));
GOTCHA_WRAP(MPI_Reduce, int, (CONST void *sbuf, void *rbuf, int count, MPI_Datatype stype, MPI_Op op, int root, MPI_Comm comm));
GOTCHA_WRAP(MPI_Allreduce, int, (CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
GOTCHA_WRAP(MPI_Reduce_scatter, int, (CONST void *sbuf, void *rbuf, CONST int *rcounts, MPI_Datatype stype, MPI_Op op, MPI_Comm comm));
GOTCHA_WRAP(MPI_Scan, int, (CONST void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm));
GOTCHA_WRAP(MPI_Type_commit, int, (MPI_Datatype * datatype));
GOTCHA_WRAP(MPI_Type_create_darray, int, (int size, int rank, int ndims, CONST int array_of_gsizes[], CONST int array_of_distribs[],CONST int array_of_dargs[], CONST int array_of_psizes[], int order, MPI_Datatype oldtype, MPI_Datatype *newtype));
GOTCHA_WRAP(MPI_File_get_size, int, (MPI_File fh, MPI_Offset *size));

// Added 10 new MPI functinos on 2019/01/07
GOTCHA_WRAP(MPI_Cart_rank, int, (MPI_Comm comm, CONST int coords[], int *rank));
GOTCHA_WRAP(MPI_Cart_create, int, (MPI_Comm comm_old, int ndims, CONST int dims[], CONST int periods[], int reorder, MPI_Comm *comm_cart));
GOTCHA_WRAP(MPI_Cart_get, int, (MPI_Comm comm, int maxdims, int dims[], int periods[], int coords[]));
GOTCHA_WRAP(MPI_Cart_shift, int, (MPI_Comm comm, int direction, int disp, int *rank_source, int *rank_dest));
GOTCHA_WRAP(MPI_Wait, int, (MPI_Request *request, MPI_Status *status));
GOTCHA_WRAP(MPI_Send, int, (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag,MPI_Comm comm));
GOTCHA_WRAP(MPI_Recv, int, (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status));
GOTCHA_WRAP(MPI_Sendrecv, int, (CONST void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest, int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype, int source, int recvtag, MPI_Comm comm, MPI_Status *status));
GOTCHA_WRAP(MPI_Isend, int, (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request));
GOTCHA_WRAP(MPI_Irecv, int, (void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request *request));
// Add MPI_Waitall, MPI_Waitsome, MPI_Waitany, MPI_Ssend on 2020/08/06
GOTCHA_WRAP(MPI_Waitall, int, (int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]));
GOTCHA_WRAP(MPI_Waitsome, int, (int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]));
GOTCHA_WRAP(MPI_Waitany, int, (int count, MPI_Request array_of_requests[], int *indx, MPI_Status * status));
GOTCHA_WRAP(MPI_Ssend, int, (CONST void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm));
// Add MPI_Comm_split on 2020/08/17
GOTCHA_WRAP(MPI_Comm_split, int, (MPI_Comm comm, int color, int key, MPI_Comm * newcomm));
GOTCHA_WRAP(MPI_Comm_dup, int, (MPI_Comm comm, MPI_Comm * newcomm));
GOTCHA_WRAP(MPI_Comm_create, int, (MPI_Comm comm, MPI_Group group, MPI_Comm * newcomm));
// Add MPI_File_seek and MPI_File_seek_shared on 2020/08/27
GOTCHA_WRAP(MPI_File_seek, int, (MPI_File fh, MPI_Offset offset, int whence));
GOTCHA_WRAP(MPI_File_seek_shared, int, (MPI_File fh, MPI_Offset offset, int whence));
// Add MPI_Ibcast on 2020/11/13
GOTCHA_WRAP(MPI_Ibcast, int, (void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm, MPI_Request *request));
// Add MPI_Test, MPI_Testany, MPI_Testsome, MPI_Testall,
// MPI_Ireduce, MPI_Igather, MPI_Iscatter and MPI_Ialltoall on 2020/12/18
GOTCHA_WRAP(MPI_Test, int, (MPI_Request *request, int *flag, MPI_Status *status));
GOTCHA_WRAP(MPI_Testall, int, (int count, MPI_Request array_of_requests[], int *flag, MPI_Status array_of_statuses[]));
GOTCHA_WRAP(MPI_Testsome, int, (int incount, MPI_Request array_of_requests[], int *outcount, int array_of_indices[], MPI_Status array_of_statuses[]));
GOTCHA_WRAP(MPI_Testany, int, (int count, MPI_Request array_of_requests[], int *indx, int *flag, MPI_Status * status));
GOTCHA_WRAP(MPI_Ireduce, int, (const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm, MPI_Request *request));
GOTCHA_WRAP(MPI_Igather, int, (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
GOTCHA_WRAP(MPI_Iscatter, int, (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request *request));
GOTCHA_WRAP(MPI_Ialltoall, int, (const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm, MPI_Request * request));
// Add on 2021/01/25
GOTCHA_WRAP(MPI_Comm_free, int, (MPI_Comm *comm));
GOTCHA_WRAP(MPI_Cart_sub, int, (MPI_Comm comm, const int remain_dims[], MPI_Comm *newcomm));
GOTCHA_WRAP(MPI_Comm_split_type, int, (MPI_Comm comm, int split_type, int key, MPI_Info info, MPI_Comm *newcomm));



/* NOTE: using HDF5 1.8 version */

/* File Interface */
GOTCHA_WRAP(H5Fcreate, hid_t, (const char *filename, unsigned flags, hid_t create_plist, hid_t access_plist));
GOTCHA_WRAP(H5Fopen, hid_t, (const char *filename, unsigned flags, hid_t access_plist));
GOTCHA_WRAP(H5Fclose, herr_t, (hid_t file_id));
GOTCHA_WRAP(H5Fflush, herr_t, (hid_t object_id, H5F_scope_t scope));
/* Group Interface */
GOTCHA_WRAP(H5Gclose, herr_t, (hid_t group_id));
GOTCHA_WRAP(H5Gcreate1, hid_t, (hid_t loc_id, const char *name, size_t size_hint));
GOTCHA_WRAP(H5Gcreate2, hid_t, (hid_t loc_id, const char *name, hid_t lcpl_id, hid_t gcpl_id, hid_t gapl_id));
GOTCHA_WRAP(H5Gget_objinfo, herr_t, (hid_t loc_id, const char *name, hbool_t follow_link, H5G_stat_t *statbuf));
GOTCHA_WRAP(H5Giterate, int, (hid_t loc_id, const char *name, int *idx, H5G_iterate_t operator, void *operator_data));
GOTCHA_WRAP(H5Gopen1, hid_t, (hid_t loc_id, const char *name));
GOTCHA_WRAP(H5Gopen2, hid_t, (hid_t loc_id, const char *name, hid_t gapl_id));
/* Dataset Interface  */
GOTCHA_WRAP(H5Dclose, herr_t, (hid_t dataset_id));
GOTCHA_WRAP(H5Dcreate1, hid_t, (hid_t loc_id, const char *name, hid_t type_id, hid_t space_id, hid_t dcpl_id));
GOTCHA_WRAP(H5Dcreate2, hid_t, (hid_t loc_id, const char *name, hid_t dtype_id, hid_t space_id, hid_t lcpl_id, hid_t dcpl_id, hid_t dapl_id));
GOTCHA_WRAP(H5Dget_create_plist, hid_t, (hid_t dataset_id));
GOTCHA_WRAP(H5Dget_space, hid_t, (hid_t dataset_id));
GOTCHA_WRAP(H5Dget_type, hid_t, (hid_t dataset_id));
GOTCHA_WRAP(H5Dopen1, hid_t, (hid_t loc_id, const char *name));
GOTCHA_WRAP(H5Dopen2, hid_t, (hid_t loc_id, const char *name, hid_t dapl_id));
GOTCHA_WRAP(H5Dread, herr_t, (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, void *buf));
GOTCHA_WRAP(H5Dwrite, herr_t, (hid_t dataset_id, hid_t mem_type_id, hid_t mem_space_id, hid_t file_space_id, hid_t xfer_plist_id, const void *buf));
GOTCHA_WRAP(H5Dset_extent, herr_t, (hid_t dataset_id, const hsize_t size[]));
/* Dataspace Interface */
GOTCHA_WRAP(H5Sclose, herr_t, (hid_t space_id));
GOTCHA_WRAP(H5Screate, hid_t, (H5S_class_t type));
GOTCHA_WRAP(H5Screate_simple, hid_t, (int rank, const hsize_t *current_dims, const hsize_t *maximum_dims));
GOTCHA_WRAP(H5Sget_select_npoints, hssize_t, (hid_t space_id));
GOTCHA_WRAP(H5Sget_simple_extent_dims, int, (hid_t space_id, hsize_t * dims, hsize_t * maxdims));
GOTCHA_WRAP(H5Sget_simple_extent_npoints, hssize_t, (hid_t space_id));
GOTCHA_WRAP(H5Sselect_elements, herr_t, (hid_t space_id, H5S_seloper_t op, size_t num_elements, const hsize_t *coord));
GOTCHA_WRAP(H5Sselect_hyperslab, herr_t, (hid_t space_id, H5S_seloper_t op, const hsize_t *start, const hsize_t *stride, const hsize_t *count, const hsize_t *block));
GOTCHA_WRAP(H5Sselect_none, herr_t, (hid_t space_id));
/* Datatype Interface */
GOTCHA_WRAP(H5Tclose, herr_t, (hid_t dtype_id));
GOTCHA_WRAP(H5Tcopy, hid_t, (hid_t dtype_id));
GOTCHA_WRAP(H5Tget_class, H5T_class_t, (hid_t dtype_id));
GOTCHA_WRAP(H5Tget_size, size_t, (hid_t dtype_id));
GOTCHA_WRAP(H5Tset_size, herr_t, (hid_t dtype_id, size_t size));
GOTCHA_WRAP(H5Tcreate, hid_t, (H5T_class_t class, size_t size));
GOTCHA_WRAP(H5Tinsert, herr_t, (hid_t dtype_id, const char *name, size_t offset, hid_t field_id));
/* Attribute Interface */
GOTCHA_WRAP(H5Aclose, herr_t, (hid_t attr_id));
GOTCHA_WRAP(H5Acreate1, hid_t, (hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id));
GOTCHA_WRAP(H5Acreate2, hid_t, (hid_t loc_id, const char *attr_name, hid_t type_id, hid_t space_id, hid_t acpl_id, hid_t aapl_id));
GOTCHA_WRAP(H5Aget_name, ssize_t, (hid_t attr_id, size_t buf_size, char *buf));
GOTCHA_WRAP(H5Aget_num_attrs, int, (hid_t loc_id));
GOTCHA_WRAP(H5Aget_space, hid_t, (hid_t attr_id));
GOTCHA_WRAP(H5Aget_type, hid_t, (hid_t attr_id));
GOTCHA_WRAP(H5Aopen, hid_t, (hid_t obj_id, const char *attr_name, hid_t aapl_id));
GOTCHA_WRAP(H5Aopen_idx, hid_t, (hid_t loc_id, unsigned int idx));
GOTCHA_WRAP(H5Aopen_name, hid_t, (hid_t loc_id, const char *name));
GOTCHA_WRAP(H5Aread, herr_t, (hid_t attr_id, hid_t mem_type_id, void *buf));
GOTCHA_WRAP(H5Awrite, herr_t, (hid_t attr_id, hid_t mem_type_id, const void *buf));
/* Property List Interface */
GOTCHA_WRAP(H5Pclose, herr_t, (hid_t plist));
GOTCHA_WRAP(H5Pcreate, hid_t, (hid_t cls_id));
GOTCHA_WRAP(H5Pget_chunk, int, (hid_t plist, int max_ndims, hsize_t *dims));
GOTCHA_WRAP(H5Pget_mdc_config, herr_t, (hid_t plist_id, H5AC_cache_config_t * config_ptr));
GOTCHA_WRAP(H5Pset_alignment, herr_t, (hid_t plist, hsize_t threshold, hsize_t alignment));
GOTCHA_WRAP(H5Pset_chunk, herr_t, (hid_t plist, int ndims, const hsize_t *dim));
GOTCHA_WRAP(H5Pset_dxpl_mpio, herr_t, (hid_t dxpl_id, H5FD_mpio_xfer_t xfer_mode));
GOTCHA_WRAP(H5Pset_fapl_core, herr_t, (hid_t fapl_id, size_t increment, hbool_t backing_store));
GOTCHA_WRAP(H5Pset_fapl_mpio, herr_t, (hid_t fapl_id, MPI_Comm comm, MPI_Info info));
// removed from from HDF5 1.8.13
// GOTCHA_WRAP(H5Pset_fapl_mpiposix, herr_t, (hid_t fapl_id, MPI_Comm comm, hbool_t use_gpfs_hints));
GOTCHA_WRAP(H5Pset_istore_k, herr_t, (hid_t plist, unsigned ik));
GOTCHA_WRAP(H5Pset_mdc_config, herr_t, (hid_t plist_id, H5AC_cache_config_t * config_ptr));
GOTCHA_WRAP(H5Pset_meta_block_size, herr_t, (hid_t fapl_id, hsize_t size));
/* Link Interface */
GOTCHA_WRAP(H5Lexists, htri_t, (hid_t loc_id, const char *name, hid_t lapl_id));
GOTCHA_WRAP(H5Lget_val, herr_t, (hid_t link_loc_id, const char *link_name, void *linkval_buff, size_t size, hid_t lapl_id));
GOTCHA_WRAP(H5Literate, herr_t, (hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t * idx, H5L_iterate_t op, void *op_data));
GOTCHA_WRAP(H5Literate1, herr_t, (hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t * idx, H5L_iterate_t op, void *op_data));
GOTCHA_WRAP(H5Literate2, herr_t, (hid_t group_id, H5_index_t index_type, H5_iter_order_t order, hsize_t * idx, H5L_iterate_t op, void *op_data));
/* Object Interface */
GOTCHA_WRAP(H5Oclose, herr_t, (hid_t object_id));
/* not exist in 1.10
GOTCHA_WRAP(H5Oget_info, herr_t, (hid_t object_id, H5O_info_t * object_info));
GOTCHA_WRAP(H5Oget_info_by_name, herr_t, (hid_t loc_id, const char *object_name, H5O_info_t *object_info, hid_t lapl_id));
*/
GOTCHA_WRAP(H5Oopen, hid_t, (hid_t loc_id, const char *name, hid_t lapl_id));
/* Collective Metadata */
GOTCHA_WRAP(H5Pset_coll_metadata_write, herr_t, (hid_t fapl_id, hbool_t is_collective));
GOTCHA_WRAP(H5Pget_coll_metadata_write, herr_t, (hid_t fapl_id, hbool_t* is_collective));
GOTCHA_WRAP(H5Pset_all_coll_metadata_ops, herr_t, (hid_t accpl_id, hbool_t is_collective));
GOTCHA_WRAP(H5Pget_all_coll_metadata_ops, herr_t, (hid_t accpl_id, hbool_t* is_collective));

#endif /* __RECORDER_GOTCHA_H */
