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

extern char *__progname_full;
extern char *__progname;
FILE *__recorderfh;
int depth;

#ifdef RECORDER_PRELOAD
#include <dlfcn.h>
#include <stdlib.h>

#define RECORDER_FORWARD_DECL(name, ret, args) ret(*__real_##name) args = NULL;

#define RECORDER_DECL(__name) __name

#define RECORDER_MPI_CALL(func) __real_##func

#define MAP_OR_FAIL(func)                                                      \
  if (!(__real_##func)) {                                                      \
    __real_##func = dlsym(RTLD_NEXT, #func);                                   \
    if (!(__real_##func)) {                                                    \
      fprintf(stderr, "Recorder failed to map symbol: %s\n", #func);            \
      exit(1);                                                                 \
    }                                                                          \
  }
extern double (*__real_PMPI_Wtime)(void);

#else

#define RECORDER_FORWARD_DECL(name, ret, args) extern ret __real_##name args;

#define RECORDER_DECL(__name) __wrap_##__name

#define MAP_OR_FAIL(func)

#define RECORDER_MPI_CALL(func) func

#endif

/*
__attribute__((constructor))
void recorder_posix_initialize() {
  int pid = getpid();

  char *hostname = malloc(PATH_MAX);
  gethostname(hostname, PATH_MAX);
  char *logfile_name;
  char *logdir_name;
  logfile_name = malloc(PATH_MAX);
  logdir_name = malloc(PATH_MAX);
  char cuser[L_cuserid] = {0};
  cuserid(cuser);

  sprintf(logdir_name, "%s_%s", cuser, __progname);
  int status;
  status = mkdir(logdir_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
  sprintf(logfile_name, "%s/log.%s.%d", logdir_name, hostname, pid);
  __recorderfh = fopen(logfile_name, "w");
  depth = 0;

  printf(" logfile_name %s ,recorderfh %d\n", logfile_name, __recorderfh);

  free(hostname);
  free(logfile_name);
  free(logdir_name);

  return;
}

__attribute__((destructor))
void recorder_shutdown() {
  //fclose(__recorderfh);

  return;
}*/

RECORDER_FORWARD_DECL(creat, int, (const char *path, mode_t mode));
RECORDER_FORWARD_DECL(creat64, int, (const char *path, mode_t mode));
RECORDER_FORWARD_DECL(open, int, (const char *path, int flags, ...));
RECORDER_FORWARD_DECL(open64, int, (const char *path, int flags, ...));
RECORDER_FORWARD_DECL(close, int, (int fd));
RECORDER_FORWARD_DECL(write, ssize_t, (int fd, const void *buf, size_t count));
RECORDER_FORWARD_DECL(read, ssize_t, (int fd, void *buf, size_t count));
RECORDER_FORWARD_DECL(lseek, off_t, (int fd, off_t offset, int whence));
RECORDER_FORWARD_DECL(lseek64, off64_t, (int fd, off64_t offset, int whence));
RECORDER_FORWARD_DECL(pread, ssize_t,
                      (int fd, void *buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pread64, ssize_t,
                      (int fd, void *buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(pwrite, ssize_t,
                      (int fd, const void *buf, size_t count, off_t offset));
RECORDER_FORWARD_DECL(pwrite64, ssize_t,
                      (int fd, const void *buf, size_t count, off64_t offset));
RECORDER_FORWARD_DECL(readv, ssize_t,
                      (int fd, const struct iovec *iov, int iovcnt));
RECORDER_FORWARD_DECL(writev, ssize_t,
                      (int fd, const struct iovec *iov, int iovcnt));
RECORDER_FORWARD_DECL(__fxstat, int, (int vers, int fd, struct stat *buf));
RECORDER_FORWARD_DECL(__fxstat64, int, (int vers, int fd, struct stat64 *buf));
RECORDER_FORWARD_DECL(__lxstat, int,
                      (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__lxstat64, int,
                      (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(__xstat, int,
                      (int vers, const char *path, struct stat *buf));
RECORDER_FORWARD_DECL(__xstat64, int,
                      (int vers, const char *path, struct stat64 *buf));
RECORDER_FORWARD_DECL(mmap, void *, (void *addr, size_t length, int prot,
                                     int flags, int fd, off_t offset));
RECORDER_FORWARD_DECL(mmap64, void *, (void *addr, size_t length, int prot,
                                       int flags, int fd, off64_t offset));
RECORDER_FORWARD_DECL(fopen, FILE *, (const char *path, const char *mode));
RECORDER_FORWARD_DECL(fopen64, FILE *, (const char *path, const char *mode));
RECORDER_FORWARD_DECL(fclose, int, (FILE * fp));
RECORDER_FORWARD_DECL(fread, size_t,
                      (void *ptr, size_t size, size_t nmemb, FILE *stream));
RECORDER_FORWARD_DECL(fwrite, size_t, (const void *ptr, size_t size,
                                       size_t nmemb, FILE *stream));
RECORDER_FORWARD_DECL(fseek, int, (FILE * stream, long offset, int whence));
RECORDER_FORWARD_DECL(fsync, int, (int fd));
RECORDER_FORWARD_DECL(fdatasync, int, (int fd));

static int recorder_mem_alignment = 1;

/* these are paths that we will not trace */
static char *exclusions[] = {"/etc/",  "/dev/",  "/usr/", "/bin/",
                             "/boot/", "/lib/",  "/opt/", "/sbin/",
                             "/sys/",  "/proc/", NULL};

static double posix_wtime(void);

char *fd2name(int fd) {
  size_t len = 256;
  struct stat sb;
  char fdname[len];
  sprintf(fdname, "/proc/self/fd/%d", fd);
  if (lstat(fdname, &sb) == -1) {
    return "null";
  }

  char *linkname = malloc(sb.st_size + 1);
  int r = readlink(fdname, linkname, sb.st_size + 1);
  linkname[r] = '\x00';
  return linkname;
}

int RECORDER_DECL(close)(int fd) {
  struct recorder_file_runtime *file;
  int hash_index;
  int tmp_fd = fd;
  double tm1, tm2;
  int ret;

  MAP_OR_FAIL(close);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_filename = fd2name(fd);
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f close (%s)", tm1, actual_filename);
#endif

  ret = __real_close(fd);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f close (%s) %d %.5f\n", tm1, actual_filename, fd, tm2 - tm1);
#endif

  return (ret);
}

int RECORDER_DECL(fclose)(FILE *fp) {
  struct recorder_file_runtime *file;
  int hash_index;
  int fd = fileno(fp);
  double tm1, tm2;
  int ret;

  MAP_OR_FAIL(fclose);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fclose (fp=%p ) ", tm1, fp);
#endif

  ret = __real_fclose(fp);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fclose (fp=%p ) %d %.5f\n", tm1, fp, fd, tm2 - tm1);
#endif

  return (ret);
}

int RECORDER_DECL(fsync)(int fd) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

  MAP_OR_FAIL(fsync);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_filename = fd2name(fd);
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fsync (%s)", tm1, actual_filename);

#endif

  ret = __real_fsync(fd);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fsync (%s) %d %.5f\n", tm1, actual_filename, fd, tm2 - tm1);
#endif

  if (ret < 0)
    return (ret);

  return (ret);
}

int RECORDER_DECL(fdatasync)(int fd) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

  MAP_OR_FAIL(fdatasync);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_filename = fd2name(fd);
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fdatasync (%s)", tm1, actual_filename);
#endif

  ret = __real_fdatasync(fd);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fdatasync (%s) %d %.5f\n", tm1, actual_filename, fd, tm2 - tm1);
#endif

  return (ret);
}

void *RECORDER_DECL(mmap64)(void *addr, size_t length, int prot, int flags,
                            int fd, off64_t offset) {
  void *ret;
  struct recorder_file_runtime *file;

  MAP_OR_FAIL(mmap64);

  ret = __real_mmap64(addr, length, prot, flags, fd, offset);
  if (ret == MAP_FAILED)
    return (ret);

  return (ret);
}

void *RECORDER_DECL(mmap)(void *addr, size_t length, int prot, int flags,
                          int fd, off_t offset) {
  void *ret;
  struct recorder_file_runtime *file;

  MAP_OR_FAIL(mmap);

  ret = __real_mmap(addr, length, prot, flags, fd, offset);
  if (ret == MAP_FAILED)
    return (ret);

  return (ret);
}

int RECORDER_DECL(creat)(const char *path, mode_t mode) {
  int ret;
  double tm1, tm2;

  MAP_OR_FAIL(creat);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f creat (%s, %d)", tm1, path, mode);
#endif

  ret = __real_creat(path, mode);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f creat (%s, %d) %d %.5f\n", tm1, path, mode, ret, tm2 - tm1);
#endif

  return (ret);
}

int RECORDER_DECL(creat64)(const char *path, mode_t mode) {
  int ret;
  double tm1, tm2;

  MAP_OR_FAIL(creat64);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f creat64 (%s, %d)", tm1, path, mode);
#endif

  ret = __real_creat64(path, mode);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f creat64 (%s, %d) %d %.5f\n", tm1, path, mode, ret, tm2 - tm1);
#endif

  return (ret);
}

int RECORDER_DECL(open64)(const char *path, int flags, ...) {
  int mode = 0;
  int ret;
  double tm1, tm2;

  MAP_OR_FAIL(open64);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f open64 (%s, %d, %d)", tm1, path, flags);
#endif

  if (flags & O_CREAT) {
    va_list arg;
    va_start(arg, flags);
    mode = va_arg(arg, int);
    va_end(arg);

    ret = __real_open64(path, flags, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
      fprintf(__recorderfh, "%.5f open64 (%s, %d, %d) %d %.5f\n", tm1, path, flags, mode, ret, tm2 - tm1);
#endif
  } else {
    ret = __real_open64(path, flags);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
      fprintf(__recorderfh, "%.5f open64 (%s, %d) %d %.5f\n", tm1, path, flags, ret, tm2 - tm1);
#endif
  }

  return (ret);
}

int RECORDER_DECL(open)(const char *path, int flags, ...) {
  int mode = 0;
  int ret;
  double tm1, tm2;

  MAP_OR_FAIL(open);

  if (flags & O_CREAT) {
    va_list arg;
    va_start(arg, flags);
    mode = va_arg(arg, int);
    va_end(arg);

#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    //if (__recorderfh != NULL)
      //fprintf(__recorderfh, "%.5f open (%s, %d, %d)", tm1, path, flags, mode);
#endif

    ret = __real_open(path, flags, mode);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
      fprintf(__recorderfh, "%.5f open (%s, %d, %d) %d %.5f\n", tm1, path, flags, mode, ret, tm2 - tm1);
#endif
  } else {
#ifndef DISABLE_POSIX_TRACE
    tm1 = recorder_wtime();
    //if (__recorderfh != NULL)
      //fprintf(__recorderfh, "%.5f open (%s, %d)", tm1, path, flags);
#endif

    ret = __real_open(path, flags);

#ifndef DISABLE_POSIX_TRACE
    tm2 = recorder_wtime();

    if (__recorderfh != NULL)
      fprintf(__recorderfh, "%.5f open (%s, %d) %d %.5f\n", tm1, path, flags, ret, tm2 - tm1);
#endif
  }

  return (ret);
}

FILE *RECORDER_DECL(fopen64)(const char *path, const char *mode) {
  FILE *ret;
  int fd;
  double tm1, tm2;

  MAP_OR_FAIL(fopen64);

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f fopen64 (%s)", tm1, path);

#endif

  ret = __real_fopen64(path, mode);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fopen64 (%s) %d %.5f\n", tm1, path, fileno(ret), tm2 - tm1);
#endif

  if (ret == 0)
    fd = -1;
  else
    fd = fileno(ret);

  return (ret);
}

FILE *RECORDER_DECL(fopen)(const char *path, const char *mode) {
  FILE *ret;
  int fd;
  double tm1, tm2;

  MAP_OR_FAIL(fopen);
#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f fopen (%s)", tm1, path);

#endif

  ret = __real_fopen(path, mode);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fopen (%s) %d %.5f\n", tm1, path, fileno(ret), tm2 - tm1);
#endif

  if (ret == 0)
    fd = -1;
  else
    fd = fileno(ret);

  return (ret);
}

int RECORDER_DECL(__xstat64)(int vers, const char *path, struct stat64 *buf) {
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

int RECORDER_DECL(__lxstat64)(int vers, const char *path, struct stat64 *buf) {
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

int RECORDER_DECL(__fxstat64)(int vers, int fd, struct stat64 *buf) {
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

int RECORDER_DECL(__xstat)(int vers, const char *path, struct stat *buf) {
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

int RECORDER_DECL(__lxstat)(int vers, const char *path, struct stat *buf) {
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

int RECORDER_DECL(__fxstat)(int vers, int fd, struct stat *buf) {
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

ssize_t RECORDER_DECL(pread64)(int fd, void *buf, size_t count,
                               off64_t offset) {
  ssize_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(pread64);
#ifndef DISABLE_POSIX_TRACE
  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

  tm1 = recorder_wtime();

  char *actual_file = fd2name(fd);
  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f pread64 (%s, buf=%p, %d, %d)", tm1, actual_file,
    //        buf, count, offset);
#endif

  ret = __real_pread64(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f pread64 (%s, buf=%p, %ld, %ld) %d %.5f\n", tm1, actual_file, buf, count, offset, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(pread)(int fd, void *buf, size_t count, off_t offset) {
  ssize_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(pread);
  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_file = fd2name(fd);
  //if (__recorderfh != NULL)
   // fprintf(__recorderfh, "%.5f pread(%d, buf=%p, %d, %d)", tm1, actual_file,
   //         buf, count, offset);
#endif

  ret = __real_pread(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f pread(%s, buf=%p, %ld, %ld) %d %.5f\n", tm1, actual_file, buf, count, offset, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(pwrite)(int fd, const void *buf, size_t count,
                              off_t offset) {
  ssize_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(pwrite);
  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_file = fd2name(fd);

  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f pwrite (%s, buf=%p, %d, %d)", tm1, actual_file,
  //          buf, count, offset);
#endif

  ret = __real_pwrite(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f pwrite (%s, buf=%p, %ld, %ld) %d %.5f\n", tm1, actual_file, buf, count, offset, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(pwrite64)(int fd, const void *buf, size_t count,
                                off64_t offset) {
  ssize_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(pwrite64);

  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f pwrite64 (%d, const void *buf, %d, %d)", tm1,
  //          fd, count, offset);
#endif

  ret = __real_pwrite64(fd, buf, count, offset);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f pwrite64 (%d, buf=%p, %ld, %ld) %d %.5f\n", tm1, fd, buf, count, offset, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(readv)(int fd, const struct iovec *iov, int iovcnt) {
  ssize_t ret;
  int aligned_flag = 1;
  int i;
  double tm1, tm2;

  MAP_OR_FAIL(readv);

  for (i = 0; i < iovcnt; i++) {
    if (((unsigned long)iov[i].iov_base % recorder_mem_alignment) != 0)
      aligned_flag = 0;
  }

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f readv (%d, iov, %d)", tm1, fd, iovcnt);
#endif

  ret = __real_readv(fd, iov, iovcnt);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f readv (%d, iov, %d) %d %.5f\n", tm1, fd, iovcnt, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(writev)(int fd, const struct iovec *iov, int iovcnt) {
  ssize_t ret;
  int aligned_flag = 1;
  int i;
  double tm1, tm2;

  MAP_OR_FAIL(writev);

  for (i = 0; i < iovcnt; i++) {
    if (!((unsigned long)iov[i].iov_base % recorder_mem_alignment == 0))
      aligned_flag = 0;
  }

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f writev (%d, const iov, %d)", tm1, fd, iovcnt);
#endif

  ret = __real_writev(fd, iov, iovcnt);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f writev (%d, const iov, %d) %d %.5f\n", tm1, fd, iovcnt, fd, tm2 - tm1);
#endif

  return (ret);
}

size_t RECORDER_DECL(fread)(void *ptr, size_t size, size_t nmemb,
                            FILE *stream) {
  size_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(fread);

  if ((unsigned long)ptr % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fread (ptr, %d, %d, %d)", tm1, size, nmemb,
  //          fileno(stream));

#endif

  ret = __real_fread(ptr, size, nmemb, stream);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fread (ptr, %ld, %ld, %d) %d %.5f\n", tm1, size, nmemb, fileno(stream), fileno(stream), tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(read)(int fd, void *buf, size_t count) {
  ssize_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(read);

  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

  tm1 = recorder_wtime();

#ifndef DISABLE_POSIX_TRACE
  //if (__recorderfh != NULL)
    //fprintf(__recorderfh, "%.5f read (%d, buf, %d)", tm1, fd, count);

#endif

  ret = __real_read(fd, buf, count);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f read (%d, %p, %ld) %d %.5f\n", tm1, fd, buf, count, fd, tm2 - tm1);
#endif

  return (ret);
}

ssize_t RECORDER_DECL(write)(int fd, const void *buf, size_t count) {
  ssize_t ret;

  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(write);

  if ((unsigned long)buf % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  char *actual_filename = fd2name(fd);
  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f write (%s, buf=%p, %d)", tm1, actual_filename,
   //         buf, count);
#endif

  ret = __real_write(fd, buf, count);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f write (%s, buf=%p, %ld) %d %.5f\n", tm1, actual_filename, buf, count, fd, tm2 - tm1);
#endif

  return (ret);
}

size_t RECORDER_DECL(fwrite)(const void *ptr, size_t size, size_t nmemb,
                             FILE *stream) {
  size_t ret;
  int aligned_flag = 0;
  double tm1, tm2;

  MAP_OR_FAIL(fwrite);

  if ((unsigned long)ptr % recorder_mem_alignment == 0)
    aligned_flag = 1;

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fwrite (ptr, %d, %d, %d)", tm1, size, nmemb,
  //          fileno(stream));
#endif

  ret = __real_fwrite(ptr, size, nmemb, stream);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fwrite (ptr, %ld, %ld, %d) %d %.5f\n", tm1, size, nmemb, fileno(stream), fileno(stream), tm2 - tm1);
#endif

  return (ret);
}

off64_t RECORDER_DECL(lseek64)(int fd, off64_t offset, int whence) {
  off64_t ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

  MAP_OR_FAIL(lseek64);

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
#endif

  ret = __real_lseek64(fd, offset, whence);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();
  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f lseek64 (%d, %ld, %d) %d %.5f\n", tm1, fd, offset, whence, fd, tm2 - tm1);
#endif
  return (ret);
}

off_t RECORDER_DECL(lseek)(int fd, off_t offset, int whence) {
  off_t ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

  MAP_OR_FAIL(lseek);

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();
  // if (__recorderfh != NULL)
  //   fprintf(__recorderfh, "%.5f lseek (fd, offset, whence)", tm1);
#endif

  ret = __real_lseek(fd, offset, whence);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f lseek (%d, %ld, %d) %d %.5f\n", tm1, fd, offset, whence, fd, tm2 - tm1);
#endif

  return (ret);
}

int RECORDER_DECL(fseek)(FILE *stream, long offset, int whence) {
  int ret;
  struct recorder_file_runtime *file;
  double tm1, tm2;

  MAP_OR_FAIL(fseek);

#ifndef DISABLE_POSIX_TRACE
  tm1 = recorder_wtime();

  //if (__recorderfh != NULL)
  //  fprintf(__recorderfh, "%.5f fseek (stream=%p, offset=%d, whence=%d)", tm1,
  //          stream, offset, whence);
#endif

  ret = __real_fseek(stream, offset, whence);

#ifndef DISABLE_POSIX_TRACE
  tm2 = recorder_wtime();

  if (__recorderfh != NULL)
    fprintf(__recorderfh, "%.5f fseek (stream=%p, offset=%ld, whence=%d) %d %.5f\n", tm1, stream, offset, whence, fileno(stream), tm2 - tm1);
#endif

  return (ret);
}

static double posix_wtime(void) { return RECORDER_MPI_CALL(PMPI_Wtime)(); }

double recorder_wtime(void) {
  struct timeval time;
  gettimeofday(&time, NULL);
  return (time.tv_sec + ((double)time.tv_usec / 1000000));
}
