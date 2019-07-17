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
        /*
         * func: function pointer
         * ret: retruen type
         * real_func_call: the real function call
         * filename: const char*, used to dump trace log
         * attr1, attr2: offset and count or whence, used to dump trace log
         */
        #define RECORDER_IMP_CHEN(func, ret, real_func_call, filename, attr1, attr2, log)   \
            MAP_OR_FAIL(func)                                                               \
            depth++;                                                                        \
            double tm1 = recorder_wtime();                                                  \
            ret res = real_func_call;                                                       \
            double tm2 = recorder_wtime();                                                  \
            write_data_operation(#func, filename, tm1, tm2, attr1, attr2, log);             \
            depth--;                                                                        \
            return res;
    #else
        #define RECORDER_IMP_CHEN(func, ret, real_func_call, filename, attr1, attr2, log)   \
            MAP_OR_FAIL(func)                                                               \
            return real_func_call;
    #endif

#endif


static int recorder_mem_alignment = 1;

static inline char* fd2name(int fd) {
    size_t len = 256;
    struct stat sb;
    char fdname[len];
    sprintf(fdname, "/proc/self/fd/%d", fd);
    if (lstat(fdname, &sb) == -1)
        return NULL;

    char *linkname = malloc(len);
    int r = readlink(fdname, linkname, len);
    linkname[r] = '\x00';
    return linkname;
}

int close(int fd) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    printf("close: %d, %s\n", fd, fn);
    sprintf(log_text, "close (%s)", fn);
    RECORDER_IMP_CHEN(close, int, __real_close(fd), fn, 0, 0, log_text)
}

int fclose(FILE *fp) {
    const char *fn = fd2name(fileno(fp));
    char log_text[TRACE_LEN];
    sprintf(log_text, "fclose (%s)", fn);
    RECORDER_IMP_CHEN(fclose, int, __real_fclose(fp), fn, 0, 0, log_text)
}

int fsync(int fd) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "fsync (%s)", fn);
    RECORDER_IMP_CHEN(fsync, int, __real_fsync(fd), fn, 0, 0, log_text)
}

int fdatasync(int fd) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "fdatasync (%s)", fn);
    RECORDER_IMP_CHEN(fdatasync, int, __real_fdatasync(fd), fn, 0, 0, log_text)
}

// TODO??? not recording this?
void *mmap64(void *addr, size_t length, int prot, int flags, int fd, off64_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "mmap (%p, %ld, %d, %d, %s, %ld)", addr, length, prot, flags, fn, offset);
    RECORDER_IMP_CHEN(mmap64, void*, __real_mmap64(addr, length, prot, flags, fd, offset), fn, 0, 0, log_text)
}
void *mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "mmap (%p, %ld, %d, %d, %s, %ld)", addr, length, prot, flags, fn, offset);
    RECORDER_IMP_CHEN(mmap, void*, __real_mmap(addr, length, prot, flags, fd, offset), fn, 0, 0, log_text)
}

int creat(const char *path, mode_t mode) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "creat (%s, %d)", path, mode);
    RECORDER_IMP_CHEN(creat, int, __real_creat(path, mode), path, mode, 0, log_text)
}

int creat64(const char *path, mode_t mode) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "creat64 (%s, %d)", path, mode);
    RECORDER_IMP_CHEN(creat64, int, __real_creat64(path, mode), path, mode, 0, log_text)
}

int open64(const char *path, int flags, ...) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "open64 (%s, %d)", path, flags);
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        RECORDER_IMP_CHEN(open64, int, __real_open64(path, flags, mode), path, flags, mode, log_text)
    } else {
        RECORDER_IMP_CHEN(open64, int, __real_open64(path, flags), path, flags, 0, log_text)
    }
}

int open(const char *path, int flags, ...) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "open (%s, %d)", path, flags);
    if (flags & O_CREAT) {
        va_list arg;
        va_start(arg, flags);
        int mode = va_arg(arg, int);
        va_end(arg);
        RECORDER_IMP_CHEN(open, int, __real_open(path, flags, mode), path, flags, mode, log_text)
    } else {
        RECORDER_IMP_CHEN(open, int, __real_open(path, flags), path, flags, 0, log_text)
    }
}

FILE *fopen64(const char *path, const char *mode) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "fopen64 (%s, %s)", path, mode);
    // TODO: mode
    RECORDER_IMP_CHEN(fopen64, FILE*, __real_fopen64(path, mode), path, 0, 0, log_text)
}

FILE *fopen(const char *path, const char *mode) {
    char log_text[TRACE_LEN];
    sprintf(log_text, "fopen (%s, %s)", path, mode);
    // TODO: mode
    RECORDER_IMP_CHEN(fopen, FILE*, __real_fopen(path, mode), path, 0, 0, log_text)
}

int __xstat64(int vers, const char *path, struct stat64 *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__xstat64);
  tm1 = recorder_wtime();
  ret = __real___xstat64(vers, path, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

int __lxstat64(int vers, const char *path, struct stat64 *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__lxstat64);
  tm1 = recorder_wtime();
  ret = __real___lxstat64(vers, path, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

int __fxstat64(int vers, int fd, struct stat64 *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__fxstat64);
  tm1 = recorder_wtime();
  ret = __real___fxstat64(vers, fd, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

int __xstat(int vers, const char *path, struct stat *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__xstat);
  tm1 = recorder_wtime();
  ret = __real___xstat(vers, path, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

int __lxstat(int vers, const char *path, struct stat *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__lxstat);
  tm1 = recorder_wtime();
  ret = __real___lxstat(vers, path, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

int __fxstat(int vers, int fd, struct stat *buf) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;
  MAP_OR_FAIL(__fxstat);
  tm1 = recorder_wtime();
  ret = __real___fxstat(vers, fd, buf);
  tm2 = recorder_wtime();
  if (ret < 0 || !S_ISREG(buf->st_mode))
    return (ret);
  return (ret);
}

ssize_t pread64(int fd, void *buf, size_t count, off64_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "pread64 (%s, %p, %ld, %ld)", fn, buf, count, offset);
    RECORDER_IMP_CHEN(pread64, ssize_t, __real_pread64(fd, buf, count, offset), fn, offset, count, log_text)
}

ssize_t pread(int fd, void *buf, size_t count, off_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "pread (%s, %p, %ld, %ld)", fn, buf, count, offset);
    RECORDER_IMP_CHEN(pread, ssize_t, __real_pread(fd, buf, count, offset), fn, offset, count, log_text)
}

ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "pwrite (%s, %p, %ld, %ld)", fn, buf, count, offset);
    RECORDER_IMP_CHEN(pwrite, ssize_t, __real_pwrite(fd, buf, count, offset), fn, offset, count, log_text)
}

ssize_t pwrite64(int fd, const void *buf, size_t count, off64_t offset) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "pwrite64 (%s, %p, %ld, %ld)", fn, buf, count, offset);
    RECORDER_IMP_CHEN(pwrite64, ssize_t, __real_pwrite64(fd, buf, count, offset), fn, offset, count, log_text)
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    char iov_str[100];
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    sprintf(log_text, "readv (%s, iov_len:[%s], %d)", fn, iov_str, iovcnt);
    // TODO: iov
    RECORDER_IMP_CHEN(readv, ssize_t, __real_readv(fd, iov, iovcnt), fn, 0, 0, log_text)
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    char iov_str[100];
    int n = 0; int i = 0;
    for (i = 0; i < iovcnt; i++)
        n += sprintf(&iov_str[n], "%d ", iov[i].iov_len);
    sprintf(log_text, "writev (%s, iov_len:[%s], %d)", fn, iov_str, iovcnt);
    // TODO: iov
    RECORDER_IMP_CHEN(writev, ssize_t, __real_writev(fd, iov, iovcnt), fn, 0, 0, log_text)
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    const char *fn = fd2name(fileno(stream));
    char log_text[TRACE_LEN];
    sprintf(log_text, "fread (%p, %ld, %ld, %s)", ptr, size, nmemb, fn);
    RECORDER_IMP_CHEN(fread, size_t, __real_fread(ptr, size, nmemb, stream), fn, size, nmemb, log_text)
}

ssize_t read(int fd, void *buf, size_t count) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "read (%s, %p, %ld)", fn, buf, count);
    RECORDER_IMP_CHEN(read, ssize_t, __real_read(fd, buf, count), fn, 0, count, log_text)
}

ssize_t write(int fd, const void *buf, size_t count) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "write (%s, %p, %ld)", fn, buf, count);
    RECORDER_IMP_CHEN(write, ssize_t, __real_write(fd, buf, count), fn, 0, count, log_text)
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    // TODO:what's this aligned_flag for?
    // int aligned_flag = 0;
    //if ((unsigned long)ptr % recorder_mem_alignment == 0)
    //    aligned_flag = 1;
    const char *fn = fd2name(fileno(stream));
    char log_text[TRACE_LEN];
    sprintf(log_text, "fwrite (%p, %ld, %ld, %s)", ptr, size, nmemb, fn);
    RECORDER_IMP_CHEN(fwrite, size_t, __real_fwrite(ptr, size, nmemb, stream), fn, size, nmemb, log_text)
}

off64_t lseek64(int fd, off64_t offset, int whence) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "lseek64 (%s, %ld, %d)", fn, offset, whence);
    RECORDER_IMP_CHEN(lseek64, off64_t, __real_lseek64(fd, offset, whence), fn, offset, whence, log_text)
}

off_t lseek(int fd, off_t offset, int whence) {
    const char *fn = fd2name(fd);
    char log_text[TRACE_LEN];
    sprintf(log_text, "lseek (%s, %ld, %d)", fn, offset, whence);
    RECORDER_IMP_CHEN(lseek, off_t, __real_lseek(fd, offset, whence), fn, offset, whence, log_text)
}

int fseek(FILE *stream, long offset, int whence) {
    const char *fn = fd2name(fileno(stream));
    char log_text[TRACE_LEN];
    sprintf(log_text, "fseek (%s, %ld, %d)", fn, offset, whence);
    RECORDER_IMP_CHEN(fseek, int, __real_fseek(stream, offset, whence), fn, offset, whence, log_text)
}

double recorder_wtime(void) {
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + ((double)time.tv_usec / 1000000));
}
