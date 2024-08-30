   /*
   * Copyright 2019 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "os_file.h"
   #include "detect_os.h"
      #include <errno.h>
   #include <fcntl.h>
   #include <stdlib.h>
   #include <sys/stat.h>
      #if DETECT_OS_WINDOWS
   #include <io.h>
   #define open _open
   #define fdopen _fdopen
   #define close _close
   #define dup _dup
   #define read _read
   #define O_CREAT _O_CREAT
   #define O_EXCL _O_EXCL
   #define O_WRONLY _O_WRONLY
   #else
   #include <unistd.h>
   #ifndef F_DUPFD_CLOEXEC
   #define F_DUPFD_CLOEXEC 1030
   #endif
   #endif
         FILE *
   os_file_create_unique(const char *filename, int filemode)
   {
      int fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, filemode);
   if (fd == -1)
            }
         #if DETECT_OS_WINDOWS
   int
   os_dupfd_cloexec(int fd)
   {
      /*
   * On Windows child processes don't inherit handles by default:
   * https://devblogs.microsoft.com/oldnewthing/20111216-00/?p=8873
   */
      }
   #else
   int
   os_dupfd_cloexec(int fd)
   {
      int minfd = 3;
            if (newfd >= 0)
            if (errno != EINVAL)
                     if (newfd < 0)
            long flags = fcntl(newfd, F_GETFD);
   if (flags == -1) {
      close(newfd);
               if (fcntl(newfd, F_SETFD, flags | FD_CLOEXEC) == -1) {
      close(newfd);
                  }
   #endif
      #include <fcntl.h>
   #include <sys/stat.h>
      #if DETECT_OS_WINDOWS
   typedef ptrdiff_t ssize_t;
   #endif
      static ssize_t
   readN(int fd, char *buf, size_t len)
   {
      /* err was initially set to -ENODATA but in some BSD systems
   * ENODATA is not defined and ENOATTR is used instead.
   * As err is not returned by any function it can be initialized
   * to -EFAULT that exists everywhere.
   */
   int err = -EFAULT;
   size_t total = 0;
   do {
               if (ret < 0)
            if (ret == -EINTR || ret == -EAGAIN)
            if (ret <= 0) {
      err = ret;
                              }
      #ifndef O_BINARY
   /* Unix makes no distinction between text and binary files. */
   #define O_BINARY 0
   #endif
      char *
   os_read_file(const char *filename, size_t *size)
   {
      /* Note that this also serves as a slight margin to avoid a 2x grow when
   * the file is just a few bytes larger when we read it than when we
   * fstat'ed it.
   * The string's NULL terminator is also included in here.
   */
            int fd = open(filename, O_RDONLY | O_BINARY);
   if (fd == -1) {
      /* errno set by open() */
               /* Pre-allocate a buffer at least the size of the file if we can read
   * that information.
   */
   struct stat stat;
   if (fstat(fd, &stat) == 0)
            char *buf = malloc(len);
   if (!buf) {
      close(fd);
   errno = -ENOMEM;
               ssize_t actually_read;
   size_t offset = 0, remaining = len - 1;
   while ((actually_read = readN(fd, buf + offset, remaining)) == (ssize_t)remaining) {
      char *newbuf = realloc(buf, 2 * len);
   if (!newbuf) {
      free(buf);
   close(fd);
   errno = -ENOMEM;
               buf = newbuf;
   len *= 2;
   offset += actually_read;
                        if (actually_read > 0)
            /* Final resize to actual size */
   len = offset + 1;
   char *newbuf = realloc(buf, len);
   if (!newbuf) {
      free(buf);
   errno = -ENOMEM;
      }
                     if (size)
               }
      #if DETECT_OS_LINUX && ALLOW_KCMP
      #include <sys/syscall.h>
   #include <unistd.h>
      /* copied from <linux/kcmp.h> */
   #define KCMP_FILE 0
      int
   os_same_file_description(int fd1, int fd2)
   {
               /* Same file descriptor trivially implies same file description */
   if (fd1 == fd2)
               }
      #else
      int
   os_same_file_description(int fd1, int fd2)
   {
      /* Same file descriptor trivially implies same file description */
   if (fd1 == fd2)
            /* Otherwise we can't tell */
      }
      #endif
