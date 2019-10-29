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

static int recorder_mem_alignment = 1;

double recorder_wtime(void) {
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + ((double)time.tv_usec / 1000000));
}

/* Integer to stirng */
static inline char* itoa(int val) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%d", val);
    return str;
}

/* Pointer to string */
static inline char* ptoa(const void *ptr) {
    char *str = malloc(sizeof(char) * 16);
    sprintf(str, "%p", ptr);
    return str;
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

static inline char* get_file_id_by_name(char *filename) {
    int id = get_filename_id(filename);
    return itoa(id);
}

static inline char* fd2name(int fd) {
    size_t len = 256;
    char *fdname = malloc(sizeof(char)*len);
    sprintf(fdname, "/proc/self/fd/%d", fd);

    // when opena file that does not exist, fd will be -1
    if(fd < 0) return fdname;

    MAP_OR_FAIL(readlink)
    char *realname = malloc(sizeof(char)*len);
    int ret = RECORDER_MPI_CALL(readlink(fdname, realname, len));
    if(ret <  0) return fdname;
    realname[ret] = 0; // readlink does not append a null byte

    char *id_str = get_file_id_by_name(realname);
    free(fdname);
    free(realname);
    return id_str;
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
    char* id_str = get_file_id_by_name(real_pathname);
    free(real_pathname);
    return id_str;
}



int RECORDER_POSIX_DECL(close)(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_INTERCEPTOR(int, close, (fd), 1, args);
}

int RECORDER_POSIX_DECL(fclose)(FILE *fp) {
    char** args = assemble_args_list(1, stream2name(fp));
    RECORDER_INTERCEPTOR(int, fclose, (fp), 1, args)
}

int RECORDER_POSIX_DECL(fsync)(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_INTERCEPTOR(int, fsync, (fd), 1, args)
}

int RECORDER_POSIX_DECL(fdatasync)(int fd) {
    char** args = assemble_args_list(1, fd2name(fd));
    RECORDER_INTERCEPTOR(int, fdatasync, (fd), 1, args)
}

void* RECORDER_POSIX_DECL(mmap64)(void *addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    char** args = assemble_args_list(6, ptoa(addr), itoa(length), itoa(prot), itoa(flags), fd2name(fd), itoa(offset));
    RECORDER_INTERCEPTOR(void*, mmap64, (addr, length, prot, flags, fd, offset), 6, args)
}
void* RECORDER_POSIX_DECL(mmap)(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    char** args = assemble_args_list(6, ptoa(addr), itoa(length), itoa(prot), itoa(flags), fd2name(fd), itoa(offset));
    RECORDER_INTERCEPTOR(void*, mmap, (addr, length, prot, flags, fd, offset), 6, args)
}

int RECORDER_POSIX_DECL(creat)(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, path, itoa(mode));
    RECORDER_INTERCEPTOR(int, creat, (path, mode), 2, args)
}

int RECORDER_POSIX_DECL(creat64)(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, path, itoa(mode));
    RECORDER_INTERCEPTOR(int, creat64, (path, mode), 2, args)
}

int RECORDER_POSIX_DECL(open64)(const char *path, int flags, ...) {
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        char** args = assemble_args_list(3, path, itoa(flags), itoa(mode));
        RECORDER_INTERCEPTOR(int, open64, (path, flags, mode), 3, args)
    } else {
        char** args = assemble_args_list(2, path, itoa(flags));
        RECORDER_INTERCEPTOR(int, open64, (path, flags), 2, args)
    }
}

int RECORDER_POSIX_DECL(open)(const char *path, int flags, ...) {
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        char** args = assemble_args_list(3, realrealpath(path), itoa(flags), itoa(mode));
        RECORDER_INTERCEPTOR(int, open, (path, flags, mode), 3, args)
    } else {
        char** args = assemble_args_list(2, realrealpath(path), itoa(flags));
        RECORDER_INTERCEPTOR(int, open, (path, flags), 2, args)
    }
}

FILE* RECORDER_POSIX_DECL(fopen64)(const char *path, const char *mode) {
    char** args = assemble_args_list(2, path, mode);
    RECORDER_INTERCEPTOR(FILE*, fopen64, (path, mode), 2, args)
}

FILE* RECORDER_POSIX_DECL(fopen)(const char *path, const char *mode) {
    char** args = assemble_args_list(2, path, mode);
    RECORDER_INTERCEPTOR(FILE*, fopen, (path, mode), 2, args)
}


/**
 * From http://refspecs.linuxbase.org/LSB_3.0.0/LSB-PDA/LSB-PDA/baselib-xstat-1.html:
 * The functions __xstat(), __lxstat(), and __fxstat() shall implement the ISO POSIX (2003) functions stat(), lstat(), and fstat() respectively
 *
 * This means stat(), lstat(), fstat() are just wrappers in GLIC and dlsym() is not able to hook them.
 * So wee need to hook __xstat(), __lxstat(), and __fxstat()
 */
int RECORDER_POSIX_DECL(__xstat)(int vers, const char *path, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __xstat, (vers, path, buf), 3, args)
}
int RECORDER_POSIX_DECL(__xstat64)(int vers, const char *path, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __xstat64, (vers, path, buf), 3, args)
}
int RECORDER_POSIX_DECL(__lxstat)(int vers, const char *path, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __lxstat, (vers, path, buf), 3, args)
}
int RECORDER_POSIX_DECL(__lxstat64)(int vers, const char *path, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), realrealpath(path), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __lxstat64, (vers, path, buf), 3, args)
}
int RECORDER_POSIX_DECL(__fxstat)(int vers, int fd, struct stat *buf) {
    char** args = assemble_args_list(3, itoa(vers), fd2name(fd), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __fxstat, (vers, fd, buf), 3, args)
}
int RECORDER_POSIX_DECL(__fxstat64)(int vers, int fd, struct stat64 *buf) {
    char** args = assemble_args_list(3, itoa(vers), fd2name(fd), ptoa(buf));
    RECORDER_INTERCEPTOR(int, __fxstat64, (vers, fd, buf), 3, args)
}

ssize_t RECORDER_POSIX_DECL(pread64)(int fd, void *buf, size_t count, off64_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_INTERCEPTOR(ssize_t, pread64, (fd, buf, count, offset), 4, args)
}
ssize_t RECORDER_POSIX_DECL(pread)(int fd, void *buf, size_t count, off_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_INTERCEPTOR(ssize_t, pread, (fd, buf, count, offset), 4, args)
}

ssize_t RECORDER_POSIX_DECL(pwrite64)(int fd, const void *buf, size_t count, off64_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_INTERCEPTOR(ssize_t, pwrite64, (fd, buf, count, offset), 4, args)
}
ssize_t RECORDER_POSIX_DECL(pwrite)(int fd, const void *buf, size_t count, off_t offset) {
    char** args = assemble_args_list(4, fd2name(fd), ptoa(buf), itoa(count), itoa(offset));
    RECORDER_INTERCEPTOR(ssize_t, pwrite, (fd, buf, count, offset), 4, args)
}

ssize_t RECORDER_POSIX_DECL(readv)(int fd, const struct iovec *iov, int iovcnt) {
    /* TODO: IOV
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    */
    char** args = assemble_args_list(3, fd2name(fd), ptoa(iov), itoa(iovcnt));
    RECORDER_INTERCEPTOR(ssize_t, readv, (fd, iov, iovcnt), 3, args)
}

ssize_t RECORDER_POSIX_DECL(writev)(int fd, const struct iovec *iov, int iovcnt) {
    /* TODO: IOV
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    */
    char** args = assemble_args_list(3, fd2name(fd), ptoa(iov), itoa(iovcnt));
    RECORDER_INTERCEPTOR(ssize_t, writev, (fd, iov, iovcnt), 3, args)
}

size_t RECORDER_POSIX_DECL(fread)(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    char** args = assemble_args_list(4, ptoa(ptr), itoa(size), itoa(nmemb), stream2name(stream));
    RECORDER_INTERCEPTOR(size_t, fread, (ptr, size, nmemb, stream), 4, args)
}

size_t RECORDER_POSIX_DECL(fwrite)(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // TODO:what's this aligned_flag for?
    // int aligned_flag = 0;
    //if ((unsigned long)ptr % recorder_mem_alignment == 0)
    //    aligned_flag = 1;
    char** args = assemble_args_list(4, ptoa(ptr), itoa(size), itoa(nmemb), stream2name(stream));
    RECORDER_INTERCEPTOR(size_t, fwrite, (ptr, size, nmemb, stream), 4, args)
}

ssize_t RECORDER_POSIX_DECL(read)(int fd, void *buf, size_t count) {
    char** args = assemble_args_list(3, fd2name(fd), ptoa(buf), itoa(count));
    RECORDER_INTERCEPTOR(ssize_t, read, (fd, buf, count), 3, args)
}

ssize_t RECORDER_POSIX_DECL(write)(int fd, const void *buf, size_t count) {
    char** args = assemble_args_list(3, fd2name(fd), ptoa(buf), itoa(count));
    RECORDER_INTERCEPTOR(ssize_t, write, (fd, buf, count), 3, args)
}

int RECORDER_POSIX_DECL(fseek)(FILE *stream, long offset, int whence) {
    char** args = assemble_args_list(3, stream2name(stream), itoa(offset), itoa(whence));
    RECORDER_INTERCEPTOR(int, fseek, (stream, offset, whence), 3, args)
}

long RECORDER_POSIX_DECL(ftell)(FILE *stream) {
    char** args = assemble_args_list(1, stream2name(stream));
    RECORDER_INTERCEPTOR(long, ftell, (stream), 1, args)
}

off64_t RECORDER_POSIX_DECL(lseek64)(int fd, off64_t offset, int whence) {
    char** args = assemble_args_list(3, fd2name(fd), itoa(offset), itoa(whence));
    RECORDER_INTERCEPTOR(off64_t, lseek64, (fd, offset, whence), 3, args)
}

off_t RECORDER_POSIX_DECL(lseek)(int fd, off_t offset, int whence) {
    char** args = assemble_args_list(3, fd2name(fd), itoa(offset), itoa(whence));
    RECORDER_INTERCEPTOR(off_t, lseek, (fd, offset, whence), 3, args)
}


/* Below are non File-I/O related function calls */
char* RECORDER_POSIX_DECL(getcwd)(char *buf, size_t size) {
    char** args = assemble_args_list(2, ptoa(buf), itoa(size));
    RECORDER_INTERCEPTOR(char*, getcwd, (buf, size), 2, args)
}
int RECORDER_POSIX_DECL(mkdir)(const char *pathname, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(pathname), itoa(mode));
    RECORDER_INTERCEPTOR(int, mkdir, (pathname, mode), 2, args)
}
int RECORDER_POSIX_DECL(rmdir)(const char *pathname) {
    char** args = assemble_args_list(1, realrealpath(pathname));
    RECORDER_INTERCEPTOR(int, rmdir, (pathname), 1, args)
}
int RECORDER_POSIX_DECL(chdir)(const char *path) {
    char** args = assemble_args_list(1, realrealpath(path));
    RECORDER_INTERCEPTOR(int, chdir, (path), 1, args)
}
int RECORDER_POSIX_DECL(link)(const char *oldpath, const char *newpath) {
    char** args = assemble_args_list(2, realrealpath(oldpath), realrealpath(newpath));
    RECORDER_INTERCEPTOR(int, link, (oldpath, newpath), 2, args)
}
int RECORDER_POSIX_DECL(unlink)(const char *pathname) {
    char** args = assemble_args_list(1, realrealpath(pathname));
    RECORDER_INTERCEPTOR(int, unlink, (pathname), 1, args)
}
int RECORDER_POSIX_DECL(linkat)(int fd1, const char *path1, int fd2, const char *path2, int flag) {
    char** args = assemble_args_list(5, fd2name(fd1), realrealpath(path1), fd2name(fd2), realrealpath(path2), itoa(flag));
    RECORDER_INTERCEPTOR(int, linkat, (fd1, path1, fd2, path2, flag), 5, args)
}
int RECORDER_POSIX_DECL(symlink)(const char *path1, const char *path2) {
    char** args = assemble_args_list(2, realrealpath(path1), realrealpath(path2));
    RECORDER_INTERCEPTOR(int, symlink, (path1, path2), 2, args)
}
int RECORDER_POSIX_DECL(symlinkat)(const char *path1, int fd, const char *path2) {
    char** args = assemble_args_list(3, realrealpath(path1), itoa(fd), realrealpath(path2));
    RECORDER_INTERCEPTOR(int, symlinkat, (path1, fd, path2), 3, args)
}
ssize_t RECORDER_POSIX_DECL(readlink)(const char *path, char *buf, size_t bufsize) {
    char** args = assemble_args_list(3, realrealpath(path), ptoa(buf), itoa(bufsize));
    RECORDER_INTERCEPTOR(int, readlink, (path, buf, bufsize), 3, args)
}

ssize_t RECORDER_POSIX_DECL(readlinkat)(int fd, const char *path, char *buf, size_t bufsize) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), ptoa(buf), itoa(bufsize));
    RECORDER_INTERCEPTOR(int, readlinkat, (fd, path, buf, bufsize), 4, args)
}

int RECORDER_POSIX_DECL(rename)(const char *oldpath, const char *newpath) {
    char** args = assemble_args_list(2, realrealpath(oldpath), realrealpath(newpath));
    RECORDER_INTERCEPTOR(int, rename, (oldpath, newpath), 2, args)
}
int RECORDER_POSIX_DECL(chmod)(const char *path, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(path), itoa(mode));
    RECORDER_INTERCEPTOR(int, chmod, (path, mode), 2, args)
}
int RECORDER_POSIX_DECL(chown)(const char *path, uid_t owner, gid_t group) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(owner), itoa(group));
    RECORDER_INTERCEPTOR(int, chown, (path, owner, group), 3, args)
}
int RECORDER_POSIX_DECL(lchown)(const char *path, uid_t owner, gid_t group) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(owner), itoa(group));
    RECORDER_INTERCEPTOR(int, lchown, (path, owner, group), 3, args)
}
int RECORDER_POSIX_DECL(utime)(const char *filename, const struct utimbuf *buf) {
    char** args = assemble_args_list(2, realrealpath(filename), ptoa(buf));
    RECORDER_INTERCEPTOR(int, utime, (filename, buf), 2, args)
}
DIR* RECORDER_POSIX_DECL(opendir)(const char *name) {
    char** args = assemble_args_list(1, realrealpath(name));
    RECORDER_INTERCEPTOR(DIR*, opendir, (name), 1, args)
}
struct dirent* RECORDER_POSIX_DECL(readdir)(DIR *dir) {
    // TODO: DIR - get path
    char** args = assemble_args_list(1, ptoa(dir));
    RECORDER_INTERCEPTOR(struct dirent*, readdir, (dir), 1, args)
}
int RECORDER_POSIX_DECL(closedir)(DIR *dir) {
    char** args = assemble_args_list(1, ptoa(dir));
    RECORDER_INTERCEPTOR(int, closedir, (dir), 1, args)
}
/*
void RECORDER_POSIX_DECL(rewinddir)(DIR *dir) {
    // TODO
    char log_text[TRACE_LEN];
    sprintf(log_text, "rewinddir (%p)", dir);
    RECORDER_INTERCEPTOR(rewinddir, NULL, (dir), NULL, 0, 0, log_text)
}
*/
int RECORDER_POSIX_DECL(mknod)(const char *path, mode_t mode, dev_t dev) {
    char** args = assemble_args_list(3, realrealpath(path), itoa(mode), itoa(dev));
    RECORDER_INTERCEPTOR(int, mknod, (path, mode, dev), 3, args)
}
int RECORDER_POSIX_DECL(mknodat)(int fd, const char *path, mode_t mode, dev_t dev) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), itoa(mode), itoa(dev));
    RECORDER_INTERCEPTOR(int, mknodat, (fd, path, mode, dev), 4, args)
}



// Advanced File Operations
/* TODO: third argument
int RECORDER_POSIX_DECL(fcntl)(int fd, int cmd, ...) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "fcntl %d %d", fd, cmd);
    RECORDER_INTERCEPTOR(fcntl, int, (fd, cmd), NULL, 0, 0, log_text)
}
*/
int RECORDER_POSIX_DECL(dup)(int oldfd) {
    char** args = assemble_args_list(1, fd2name(oldfd));
    RECORDER_INTERCEPTOR(int, dup, (oldfd), 1, args)
}
int RECORDER_POSIX_DECL(dup2)(int oldfd, int newfd) {
    char** args = assemble_args_list(2, fd2name(oldfd), fd2name(newfd));
    RECORDER_INTERCEPTOR(int, dup2, (oldfd, newfd), 2, args)
}
int RECORDER_POSIX_DECL(pipe)(int pipefd[2]) {
    // TODO: pipefd?
    char** args = assemble_args_list(2, fd2name(pipefd[0]), fd2name(pipefd[1]));
    RECORDER_INTERCEPTOR(int, pipe, (pipefd), 2, args)
}
int RECORDER_POSIX_DECL(mkfifo)(const char *pathname, mode_t mode) {
    char** args = assemble_args_list(2, realrealpath(pathname), itoa(mode));
    RECORDER_INTERCEPTOR(int, mkfifo, (pathname, mode), 2, args)
}
mode_t RECORDER_POSIX_DECL(umask)(mode_t mask) {
    char** args = assemble_args_list(1, itoa(mask));
    RECORDER_INTERCEPTOR(mode_t, umask, (mask), 1, args)
}
FILE* RECORDER_POSIX_DECL(fdopen)(int fd, const char *mode) {
    char** args = assemble_args_list(2, fd2name(fd), ptoa(mode));
    RECORDER_INTERCEPTOR(FILE*, fdopen, (fd, mode), 2, args)
}
int RECORDER_POSIX_DECL(fileno)(FILE *stream) {
    char** args = assemble_args_list(1, stream2name(stream));
    RECORDER_INTERCEPTOR(int, fileno, (stream), 1, args)
}
int RECORDER_POSIX_DECL(access)(const char *path, int amode) {
    char** args = assemble_args_list(2, realrealpath(path), itoa(amode));
    RECORDER_INTERCEPTOR(int, access, (path, amode), 2, args)
}
int RECORDER_POSIX_DECL(faccessat)(int fd, const char *path, int amode, int flag) {
    char** args = assemble_args_list(4, fd2name(fd), realrealpath(path), itoa(amode), itoa(flag));
    RECORDER_INTERCEPTOR(int, faccessat, (fd, path, amode, flag), 4, args)
}
FILE* RECORDER_POSIX_DECL(tmpfile)(void) {
    char **args = NULL;
    RECORDER_INTERCEPTOR(FILE*, tmpfile, (), 0, args)
}
int RECORDER_POSIX_DECL(remove)(const char *path) {
    char** args = assemble_args_list(1, realrealpath(path));
    RECORDER_INTERCEPTOR(int, remove, (path), 1, args)
}



