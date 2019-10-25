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
#define _GNU_SOURCE
#define TRACE_LEN 256

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <search.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>

#include "recorder.h"

#ifndef HAVE_OFF64_T
typedef int64_t off64_t;
#endif

int depth;

#ifdef RECORDER_PRELOAD
    extern double (*__real_PMPI_Wtime)(void);
    #ifndef DISABLE_POSIX_TRACE
        #define RECORDER_IMP_WANG(func, ret, real_func_call, _arg_count, _args)             \
            MAP_OR_FAIL(func)                                                               \
            depth++;                                                                        \
            double tstart = recorder_wtime();                                               \
            ret res = real_func_call;                                                       \
            double tend = recorder_wtime();                                                 \
            depth--;                                                                        \
            Record record = {                                                               \
                .tstart = tstart,                                                           \
                .tdur = tend - tstart,                                                      \
                .arg_count = _arg_count,                                                    \
                .args = args                                                                \
            };                                                                              \
            write_record(record);                                                           \
            return res;
    #else
        #define RECORDER_IMP_WANG(func, ret, real_func_call, _arg_count, _args)             \
            MAP_OR_FAIL(func)                                                               \
            return real_func_call;
    #endif
#endif


static int recorder_mem_alignment = 1;

double recorder_wtime(void) {
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + ((double)time.tv_usec / 1000000));
}

static inline char** assemble_args_list(int arg_count, ...) {
    char** args = malloc(sizeof(char*) * arg_count);
    int i;
    va_list valist;
    va_start(valist, arg_count);
    for(i = 0; i < arg_count; i++)
        args[i] = va_arg(valist, char*);
    va_end(valist);
    return args;
}

static inline char* fd2name(int fd) {
    size_t len = 256;
    char fdname[len];
    sprintf(fdname, "/proc/self/fd/%d", fd);
    /*
    struct stat sb;
    if (lstat(fdname, &sb) == -1)
        return NULL;
    */

    MAP_OR_FAIL(readlink)
    char *realname = malloc(len);
    int ret = RECORDER_MPI_CALL(readlink(fdname, realname, len));
    if(ret <  0)
        return NULL;
    realname[ret] = '\x00'; // readlink does not append a null byte
    return realname;
}

static inline char* stream2name(FILE *fp) {
    // Need to map the fileno funciton, because here - this file
    // may be invoked even before MPI_Init in recorder-mpi-initialize.c
    // also note that if fileno causes segmentation fault if fp is NULL
    if (fp == NULL) return NULL;
    MAP_OR_FAIL(fileno)
    int fd = RECORDER_MPI_CALL(fileno(fp));
    return fd2name(fd);
}

// My implementation to replace realrealpath() system call
static inline char* realrealpath(const char *path) {
    char *real_pathname = (char*) malloc(PATH_MAX * sizeof(char));
    realpath(path, real_pathname);
    if (real_pathname == NULL)
        strcpy(real_pathname, path);
    return real_pathname;
}

/* Integer to stirng */
static inline char* itoa(int val) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%d", val);
    return str;
}
/* Pointer to string */
static inline char* ptoa(void *ptr) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%p", ptr);
    return str;
}



int close(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_IMP_WANG(close, int, __real_close(fd), 1, args);
}

int fclose(FILE *fp) {
    char** args = assemble_args_list(1, stream2name(fp));
    RECORDER_IMP_WANG(fclose, int, __real_fclose(fp), 1, args)
}

int fsync(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_IMP_WANG(fsync, int, __real_fsync(fd), 1, args)
}

int fdatasync(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_IMP_WANG(fdatasync, int, __real_fdatasync(fd), 1, args)
}

void *mmap64(void *addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    char** args = assemble_args_list(6, ptoa(addr), itoa(length), itoa(prot), itoa(flags), fd2name(fd), itoa(offset));
    RECORDER_IMP_WANG(mmap64, void*, __real_mmap64(addr, length, prot, flags, fd, offset), 6, args)
}
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    char** args = assemble_args_list(6, ptoa(addr), itoa(length), itoa(prot), itoa(flags), fd2name(fd), itoa(offset));
    RECORDER_IMP_WANG(mmap, void*, __real_mmap(addr, length, prot, flags, fd, offset), 6, args)
}

int creat(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, path, itoa(mode));
    RECORDER_IMP_WANG(creat, int, __real_creat(path, mode), 2, args)
}

int creat64(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, path, itoa(mode));
    RECORDER_IMP_WANG(creat64, int, __real_creat64(path, mode), 2, args)
}

int open64(const char *path, int flags, ...) {
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        char** args = assemble_args_list(3, path, itoa(flags), itoa(mode));
        RECORDER_IMP_WANG(open64, int, __real_open64(path, flags, mode), 3, args)
    } else {
        char** args = assemble_args_list(2, path, itoa(flags));
        RECORDER_IMP_WANG(open64, int, __real_open64(path, flags), 2, args)
    }
}

int open(const char *path, int flags, ...) {
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        char** args = assemble_args_list(3, path, itoa(flags), itoa(mode));
        RECORDER_IMP_WANG(open, int, __real_open(path, flags, mode), 3, args)
    } else {
        char** args = assemble_args_list(2, path, itoa(flags));
        RECORDER_IMP_WANG(open, int, __real_open(path, flags), 2, args)
    }
}

FILE *fopen64(const char *path, const char *mode) {
    char** args = assemble_args_list(2, path, mode);
    RECORDER_IMP_WANG(fopen64, FILE*, __real_fopen64(path, mode), 2, args)
}

FILE *fopen(const char *path, const char *mode) {
    char** args = assemble_args_list(2, path, mode);
    RECORDER_IMP_WANG(fopen, FILE*, __real_fopen(path, mode), 2, args)
}


/**
 * From http://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/baselib-xstat-1.html:
 * The functions __xstat(), __lxstat(), and __fxstat() shall implement the ISO POSIX (2003) functions stat(), lstat(), and fstat() respectively
 *
 * This means stat(), lstat(), fstat() are just wrappers in GLIC and dlsym() is not able to hook them.
 * So wee need to hook __xstat(), __lxstat(), and __fxstat()
 */
int __xstat(int vers, const char *path, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_IMP_WANG(__xstat, int, __real___xstat(vers, path, buf), 3, args)
}
int __xstat64(int vers, const char *path, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_IMP_WANG(__xstat64, int, __real___xstat64(vers, path, buf), 3, args)
}
int __lxstat(int vers, const char *path, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_IMP_WANG(__lxstat, int, __real___lxstat(vers, path, buf), 3, args)
}
int __lxstat64(int vers, const char *path, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_IMP_WANG(__lxstat64, int, __real___lxstat64(vers, path, buf), 3, args)
}
int __fxstat(int vers, int fd, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), fd2name(fd), ptoa(buf));
    RECORDER_IMP_WANG(__fxstat, int, __real___fxstat(vers, fd, buf), 3, args)
}
int __fxstat64(int vers, int fd, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), fd2name(fd), ptoa(buf));
    RECORDER_IMP_WANG(__fxstat64, int, __real___fxstat64(vers, fd, buf), 3, args)
}

ssize_t pread64(int fd, void *buf, size_t count, off64_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_IMP_WANG(pread64, ssize_t, __real_pread64(fd, buf, count, offset), 4, args)
}
ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_IMP_WANG(pread, ssize_t, __real_pread(fd, buf, count, offset), 4, args)
}

ssize_t pwrite64(int fd, const void *buf, size_t count, off64_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_IMP_WANG(pwrite64, ssize_t, __real_pwrite64(fd, buf, count, offset), 4, args)
}
ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_IMP_WANG(pwrite, ssize_t, __real_pwrite(fd, buf, count, offset), 4, args)
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    /* TODO: IOV
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    */
    char** args = assemble_args_list(3, fd2name(fd), ptoa(iov), itoa(iovcnt));
    RECORDER_IMP_WANG(readv, ssize_t, __real_readv(fd, iov, iovcnt), 3, args)
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    /* TODO: IOV
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    */
    char** args = assemble_args_list(3, fd2name(fd), ptoa(iov), itoa(iovcnt));
    RECORDER_IMP_WANG(writev, ssize_t, __real_writev(fd, iov, iovcnt), 3, args)
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    char** args = assemble_args_list(4, ptoa(ptr), itoa(size), itoa(nmemb), stream2name(stream));
    RECORDER_IMP_WANG(fread, size_t, __real_fread(ptr, size, nmemb, stream), 4, args)
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // TODO:what's this aligned_flag for?
    // int aligned_flag = 0;
    //if ((unsigned long)ptr % recorder_mem_alignment == 0)
    //    aligned_flag = 1;
    char** args = assemble_args_list(4, ptoa(ptr), itoa(size), itoa(nmemb), stream2name(stream));
    RECORDER_IMP_WANG(fwrite, size_t, __real_fwrite(ptr, size, nmemb, stream), 4, args)
}

ssize_t read(int fd, void *buf, size_t count) {
    char** args = assemble_args_list(3, fd2name(fd), ptoa(buf), itoa(count));
    RECORDER_IMP_WANG(read, ssize_t, __real_read(fd, buf, count), 3, args)
}

ssize_t write(int fd, const void *buf, size_t count) {
    char** args = assemble_args_list(3, fd2name(fd), ptoa(buf), itoa(count));
    RECORDER_IMP_WANG(write, ssize_t, __real_write(fd, buf, count), 3, args)
}

int fseek(FILE *stream, long offset, int whence) {
    char** args = assemble_args_list(3, stream2name(stream), itoa(offset), itoa(whence));
    RECORDER_IMP_WANG(fseek, int, __real_fseek(stream, offset, whence), 3, args)
}

long ftell(FILE *stream) {
    char** args = assemble_args_list(1, stream2name(stream));
    RECORDER_IMP_WANG(ftell, long, __real_ftell(stream), 1, args)
}

off64_t lseek64(int fd, off64_t offset, int whence) {
    char** args = assemble_args_list(3, fd2name(fd), itoa(offset), itoa(whence));
    RECORDER_IMP_WANG(lseek64, off64_t, __real_lseek64(fd, offset, whence), 3, args)
}

off_t lseek(int fd, off_t offset, int whence) {
    char** args = assemble_args_list(3, fd2name(fd), itoa(offset), itoa(whence));
    RECORDER_IMP_WANG(lseek, off_t, __real_lseek(fd, offset, whence), 3, args)
}


/* Below are non File-I/O related function calls */
char* getcwd(char *buf, size_t size) {
    char** args = assemble_args_list(2, ptoa(buf), itoa(size));
    RECORDER_IMP_WANG(getcwd, char*, __real_getcwd(buf, size), 2, args)
}
int mkdir(const char *pathname, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(pathname), itoa(mode));
    RECORDER_IMP_WANG(mkdir, int, __real_mkdir(pathname, mode), 2, args)
}
int rmdir(const char *pathname) {
    char** args = assemble_args_list(1, realrealpath(pathname));
    RECORDER_IMP_WANG(rmdir, int, __real_rmdir(pathname), 1, args)
}
int chdir(const char *path) {
    char** args = assemble_args_list(1, realrealpath(path));
    RECORDER_IMP_WANG(chdir, int, __real_chdir(path), 1, args)
}
int link(const char *oldpath, const char *newpath) {
    char** args = assemble_args_list(2, realrealpath(oldpath), realrealpath(newpath));
    RECORDER_IMP_WANG(link, int, __real_link(oldpath, newpath), 2, args)
}
int unlink(const char *pathname) {
    char** args = assemble_args_list(1, realrealpath(pathname));
    RECORDER_IMP_WANG(unlink, int, __real_unlink(pathname), 1, args)
}
int linkat(int fd1, const char *path1, int fd2, const char *path2, int flag) {
    char** args = assemble_args_list(5, fd2name(fd1), realrealpath(path1), fd2name(fd2), realrealpath(path2), itoa(flag));
    RECORDER_IMP_WANG(linkat, int, __real_linkat(fd1, path1, fd2, path2, flag), 5, args)
}
int symlink(const char *path1, const char *path2) {
    char** args = assemble_args_list(2, realrealpath(path1), realrealpath(path2));
    RECORDER_IMP_WANG(symlink, int, __real_symlink(path1, path2), 2, args)
}
int symlinkat(const char *path1, int fd, const char *path2) {
    char** args = assemble_args_list(3, realrealpath(path1), itoa(fd), realrealpath(path2));
    RECORDER_IMP_WANG(symlinkat, int, __real_symlinkat(path1, fd, path2), 3, args)
}
ssize_t readlink(const char *path, char *buf, size_t bufsize) {
    char** args = assemble_args_list(3, realrealpath(path), ptoa(buf), itoa(bufsize));
    RECORDER_IMP_WANG(readlink, int, __real_readlink(path, buf, bufsize), 3, args)
}

ssize_t readlinkat(int fd, const char *path, char *buf, size_t bufsize) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), ptoa(buf), itoa(bufsize));
    RECORDER_IMP_WANG(readlinkat, int, __real_readlinkat(fd, path, buf, bufsize), 4, args)
}

int rename(const char *oldpath, const char *newpath) {
    char** args = assemble_args_list(2, realrealpath(oldpath), realrealpath(newpath));
    RECORDER_IMP_WANG(rename, int, __real_rename(oldpath, newpath), 2, args)
}
int chmod(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(path), itoa(mode));
    RECORDER_IMP_WANG(chmod, int, __real_chmod(path, mode), 2, args)
}
int chown(const char *path, uid_t owner, gid_t group) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(owner), itoa(group));
    RECORDER_IMP_WANG(chown, int, __real_chown(path, owner, group), 3, args)
}
int lchown(const char *path, uid_t owner, gid_t group) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(owner), itoa(group));
    RECORDER_IMP_WANG(lchown, int, __real_lchown(path, owner, group), 3, args)
}
int utime(const char *filename, const struct utimbuf *buf) {
    char** args = assemble_args_list(2, realrealpath(filename), ptoa(buf));
    RECORDER_IMP_WANG(utime, int, __real_utime(filename, buf), 2, args)
}
DIR* opendir(const char *name) {
    char** args = assemble_args_list(1, realrealpath(name));
    RECORDER_IMP_WANG(opendir, DIR*, __real_opendir(name), 1, args)
}
struct dirent* readdir(DIR *dir) {
    // TODO: DIR - get path
    char** args = assemble_args_list(1, ptoa(dir));
    RECORDER_IMP_WANG(readdir, struct dirent*, __real_readdir(dir), 1, args)
}
int closedir(DIR *dir) {
    char** args = assemble_args_list(1, ptoa(dir));
    RECORDER_IMP_WANG(closedir, int, __real_closedir(dir), 1, args)
}
void rewinddir(DIR *dir) {
    // TODO
    /*
    char log_text[TRACE_LEN];
    sprintf(log_text, "rewinddir (%p)", dir);
    RECORDER_IMP_WANG(rewinddir, NULL, __real_rewinddir(dir), NULL, 0, 0, log_text)
    */
}
int mknod(const char *path, mode_t mode, dev_t dev) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(mode), ptoa(dev));
    RECORDER_IMP_WANG(mknod, int, __real_mknod(path, mode, dev), 3, args)
}
int mknodat(int fd, const char *path, mode_t mode, dev_t dev) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), itoa(mode), ptoa(dev));
    RECORDER_IMP_WANG(mknodat, int, __real_mknodat(fd, path, mode, dev), 4, args)
}



// Advanced File Operations
/* TODO: third argument
int fcntl(int fd, int cmd, ...) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "fcntl %d %d", fd, cmd);
    RECORDER_IMP_WANG(fcntl, int, __real_fcntl(fd, cmd), NULL, 0, 0, log_text)
}
*/
int dup(int oldfd) {
    char** args = assemble_args_list(1, fd2name(oldfd));
    RECORDER_IMP_WANG(dup, int, __real_dup(oldfd), 1, args)
}
int dup2(int oldfd, int newfd) {
    char** args = assemble_args_list(2, fd2name(oldfd), fd2name(newfd));
    RECORDER_IMP_WANG(dup2, int, __real_dup2(oldfd, newfd), 2, args)
}
int pipe(int pipefd[2]) {
    // TODO: pipefd?
    char** args = assemble_args_list(2, fd2name(pipefd[0]), fd2name(pipefd[1]));
    RECORDER_IMP_WANG(pipe, int, __real_pipe(pipefd), 2, args)
}
int mkfifo(const char *pathname, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(pathname), itoa(mode));
    RECORDER_IMP_WANG(mkfifo, int, __real_mkfifo(pathname, mode), 2, args)
}
mode_t umask(mode_t mask) {
    char** args = assemble_args_list(1, itoa(mask));
    RECORDER_IMP_WANG(umask, mode_t, __real_umask(mask), 1, args)
}
FILE* fdopen(int fd, const char *mode) {
    char** args = assemble_args_list(2, fd2name(fd), itoa(mode));
    RECORDER_IMP_WANG(fdopen, FILE*, __real_fdopen(fd, mode), 2, args)
}
int fileno(FILE *stream) {
    char** args = assemble_args_list(1, stream2name(stream));
    RECORDER_IMP_WANG(fileno, int, __real_fileno(stream), 1, args)
}
int access(const char *path, int amode) {
    char** args = assemble_args_list(2, realrealpath(path), itoa(amode));
    RECORDER_IMP_WANG(access, int, __real_access(path, amode), 2, args)
}
int faccessat(int fd, const char *path, int amode, int flag) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), itoa(amode), itoa(flag));
    RECORDER_IMP_WANG(faccessat, int, __real_faccessat(fd, path, amode, flag), 4, args)
}
FILE *tmpfile(void) {
    char **args = NULL;
    RECORDER_IMP_WANG(tmpfile, FILE*, __real_tmpfile(), 0, args)
}
int remove(const char *path) {
    char** args = assemble_args_list(1, realrealpath(path));
    RECORDER_IMP_WANG(remove, int, __real_remove(path), 1, args)
}



