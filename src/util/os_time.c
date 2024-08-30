   /**************************************************************************
   *
   * Copyright 2008-2010 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   * OS independent time-manipulation functions.
   *
   * @author Jose Fonseca <jfonseca@vmware.com>
   */
      #include "os_time.h"
   #include "detect_os.h"
      #include "c11/time.h"
      #include "util/u_atomic.h"
      #if DETECT_OS_UNIX
   #  include <unistd.h> /* usleep */
   #  include <time.h> /* timeval */
   #  include <sys/time.h> /* timeval */
   #  include <sched.h> /* sched_yield */
   #  include <errno.h>
   #elif DETECT_OS_WINDOWS
   #  include <windows.h>
   #else
   #  error Unsupported OS
   #endif
         int64_t
   os_time_get_nano(void)
   {
      struct timespec ts;
   timespec_get(&ts, TIME_MONOTONIC);
      }
            void
   os_time_sleep(int64_t usecs)
   {
   #if DETECT_OS_LINUX
      struct timespec time;
   time.tv_sec = usecs / 1000000;
   time.tv_nsec = (usecs % 1000000) * 1000;
         #elif DETECT_OS_UNIX
            #elif DETECT_OS_WINDOWS
      DWORD dwMilliseconds = (DWORD) ((usecs + 999) / 1000);
   /* Avoid Sleep(O) as that would cause to sleep for an undetermined duration */
   if (dwMilliseconds) {
            #else
   #  error Unsupported OS
   #endif
   }
            int64_t
   os_time_get_absolute_timeout(uint64_t timeout)
   {
               /* Also check for the type upper bound. */
   if (timeout == OS_TIMEOUT_INFINITE || timeout > INT64_MAX)
            time = os_time_get_nano();
            /* Check for overflow. */
   if (abs_timeout < time)
               }
         bool
   os_wait_until_zero(volatile int *var, uint64_t timeout)
   {
      if (!p_atomic_read(var))
            if (!timeout)
            if (timeout == OS_TIMEOUT_INFINITE) {
      #if DETECT_OS_UNIX
         #endif
         }
      }
   else {
      int64_t start_time = os_time_get_nano();
            while (p_atomic_read(var)) {
            #if DETECT_OS_UNIX
         #endif
         }
         }
         bool
   os_wait_until_zero_abs_timeout(volatile int *var, int64_t timeout)
   {
      if (!p_atomic_read(var))
            if (timeout == OS_TIMEOUT_INFINITE)
            while (p_atomic_read(var)) {
      if (os_time_get_nano() >= timeout)
      #if DETECT_OS_UNIX
         #endif
      }
      }
