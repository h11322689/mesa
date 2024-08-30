   /*
   * Copyright © 2015 Intel
   * Copyright © 2022 Yonggang Luo
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/futex.h"
      #if UTIL_FUTEX_SUPPORTED
      #if defined(HAVE_LINUX_FUTEX_H)
      #include <limits.h>
   #include <stdint.h>
   #include <unistd.h>
   #include <linux/futex.h>
   #include <sys/syscall.h>
      #ifndef SYS_futex
   #define SYS_futex SYS_futex_time64
   #endif
      static inline long sys_futex(void *addr1, int op, int val1, const struct timespec *timeout, void *addr2, int val3)
   {
         }
      int futex_wake(uint32_t *addr, int count)
   {
         }
      int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout)
   {
      /* FUTEX_WAIT_BITSET with FUTEX_BITSET_MATCH_ANY is equivalent to
   * FUTEX_WAIT, except that it treats the timeout as absolute. */
   return sys_futex(addr, FUTEX_WAIT_BITSET, value, timeout, NULL,
      }
      #elif defined(__FreeBSD__)
      #include <assert.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <sys/types.h>
   #include <sys/umtx.h>
      int futex_wake(uint32_t *addr, int count)
   {
      assert(count == (int)(uint32_t)count); /* Check that bits weren't discarded */
      }
      int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout)
   {
      void *uaddr = NULL, *uaddr2 = NULL;
   struct _umtx_time tmo = {
      ._flags = UMTX_ABSTIME,
                        if (timeout != NULL) {
      tmo._timeout = *timeout;
   uaddr = (void *)(uintptr_t)sizeof(tmo);
                  }
      #elif defined(__OpenBSD__)
      #include <sys/futex.h>
   #include <sys/time.h>
      int futex_wake(uint32_t *addr, int count)
   {
         }
      int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout)
   {
               if (timeout == NULL)
            clock_gettime(CLOCK_MONOTONIC, &tsnow);
   if (timespeccmp(&tsnow, timeout, <))
         else
            }
      #elif defined(_WIN32) && !defined(WINDOWS_NO_FUTEX)
      #include <windows.h>
   #include <stdint.h>
   #include <limits.h>
   #include <assert.h>
   #include <errno.h>
      int futex_wake(uint32_t *addr, int count)
   {
      /* All current callers fall into one of these buckets, and we'll get the semantics
   * wrong if someone tries to be more clever.
   */
   assert(count == 1 || count == INT32_MAX);
   if (count == 1)
         else
            }
      int futex_wait(uint32_t *addr, int32_t value, const struct timespec *timeout)
   {
      DWORD timeout_ms = INFINITE;
   if (timeout != NULL) {
      struct timespec tsnow;
            timeout_ms = (timeout->tv_sec - tsnow.tv_nsec) * 1000 +
               if (WaitOnAddress(addr, &value, sizeof(value), timeout_ms))
            }
      #else
   #error UTIL_FUTEX_SUPPORTED is not implemented but the header told it is supported on this platform
   #endif
      #endif /* UTIL_FUTEX_SUPPORTED */
