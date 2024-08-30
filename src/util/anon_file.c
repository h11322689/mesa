   /*
   * Copyright © 2012 Collabora, Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial
   * portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      /*
   * Based on weston shared/os-compatibility.c
   */
      #include "anon_file.h"
      #ifndef _WIN32
      #include <unistd.h>
   #include <fcntl.h>
   #include <errno.h>
   #include <stdlib.h>
      #if defined(HAVE_MEMFD_CREATE) || defined(__FreeBSD__) || defined(__OpenBSD__)
   #include <sys/mman.h>
   #elif defined(ANDROID)
   #include <sys/syscall.h>
   #include <linux/memfd.h>
   #else
   #include <stdio.h>
   #endif
      #if !(defined(__FreeBSD__) || defined(HAVE_MEMFD_CREATE) || defined(HAVE_MKOSTEMP) || defined(ANDROID))
   static int
   set_cloexec_or_close(int fd)
   {
               if (fd == -1)
            flags = fcntl(fd, F_GETFD);
   if (flags == -1)
            if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
                  err:
      close(fd);
      }
   #endif
      #if !(defined(__FreeBSD__) || defined(HAVE_MEMFD_CREATE) || defined(ANDROID))
   static int
   create_tmpfile_cloexec(char *tmpname)
   {
            #ifdef HAVE_MKOSTEMP
         #else
         #endif
         if (fd < 0) {
               #ifndef HAVE_MKOSTEMP
         #endif
         unlink(tmpname);
      }
   #endif
      /*
   * Create a new, unique, anonymous file of the given size, and
   * return the file descriptor for it. The file descriptor is set
   * CLOEXEC. The file is immediately suitable for mmap()'ing
   * the given size at offset zero.
   *
   * An optional name for debugging can be provided as the second argument.
   *
   * The file should not have a permanent backing store like a disk,
   * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
   *
   * If memfd or SHM_ANON is supported, the filesystem is not touched at all.
   * Otherwise, the file name is deleted from the file system.
   *
   * The file is suitable for buffer sharing between processes by
   * transmitting the file descriptor over Unix sockets using the
   * SCM_RIGHTS methods.
   */
   int
   os_create_anonymous_file(int64_t size, const char *debug_name)
   {
         #if defined(HAVE_MEMFD_CREATE)
      if (!debug_name)
            #elif defined(ANDROID)
      if (!debug_name)
            #elif defined(__FreeBSD__)
         #elif defined(__OpenBSD__)
      char template[] = "/tmp/mesa-XXXXXXXXXX";
   fd = shm_mkstemp(template);
   if (fd != -1)
      #else
      const char *path;
            path = getenv("XDG_RUNTIME_DIR");
   if (!path) {
      errno = ENOENT;
               if (debug_name)
         else
         if (!name)
                        #endif
         if (fd < 0)
            ret = ftruncate(fd, (off_t)size);
   if (ret < 0) {
      close(fd);
                  }
   #else
      #include <windows.h>
   #include <io.h>
      int
   os_create_anonymous_file(int64_t size, const char *debug_name)
   {
      (void)debug_name;
   HANDLE h = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
            }
   #endif
