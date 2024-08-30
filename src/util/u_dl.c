   /**************************************************************************
   *
   * Copyright 2009 VMware, Inc.
   * Copyright 1999-2008  Brian Paul
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "detect_os.h"
      #if DETECT_OS_UNIX
   #include <dlfcn.h>
   #endif
   #if DETECT_OS_WINDOWS
   #include <windows.h>
   #endif
      #include "u_dl.h"
   #include "u_pointer.h"
         struct util_dl_library *
   util_dl_open(const char *filename)
   {
   #if DETECT_OS_UNIX
         #elif DETECT_OS_WINDOWS
         #else
         #endif
   }
         util_dl_proc
   util_dl_get_proc_address(struct util_dl_library *library,
         {
   #if DETECT_OS_UNIX
         #elif DETECT_OS_WINDOWS
         #else
         #endif
   }
         void
   util_dl_close(struct util_dl_library *library)
   {
   #if DETECT_OS_UNIX
         #elif DETECT_OS_WINDOWS
         #else
         #endif
   }
         const char *
   util_dl_error(void)
   {
   #if DETECT_OS_UNIX
         #elif DETECT_OS_WINDOWS
         #else
         #endif
   }
