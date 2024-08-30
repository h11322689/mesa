   /*
   * (C) Copyright 2016, NVIDIA CORPORATION.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   * Authors:
   *    Kyle Brenneman <kbrenneman@nvidia.com>
   */
      #include <assert.h>
   #include <string.h>
      #include <glvnd/libeglabi.h>
      #include "eglcurrent.h"
   #include "egldispatchstubs.h"
   #include "eglglobals.h"
      static const __EGLapiExports *__eglGLVNDApiExports = NULL;
      static const char *EGLAPIENTRY
   __eglGLVNDQueryString(EGLDisplay dpy, EGLenum name)
   {
      // For client extensions, return the list of non-platform extensions. The
   // platform extensions are returned by __eglGLVNDGetVendorString.
   if (dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS)
            // For everything else, forward to the normal eglQueryString function.
      }
      static const char *
   __eglGLVNDGetVendorString(int name)
   {
      if (name == __EGL_VENDOR_STRING_PLATFORM_EXTENSIONS)
               }
      static EGLDisplay
   __eglGLVNDGetPlatformDisplay(EGLenum platform, void *native_display,
         {
      if (platform == EGL_NONE) {
      assert(native_display == (void *)EGL_DEFAULT_DISPLAY);
   assert(attrib_list == NULL);
      } else {
            }
      static void *
   __eglGLVNDGetProcAddress(const char *procName)
   {
      if (strcmp(procName, "eglQueryString") == 0)
               }
      PUBLIC EGLAPI EGLBoolean
   __egl_Main(uint32_t version, const __EGLapiExports *exports,
         {
      if (EGL_VENDOR_ABI_GET_MAJOR_VERSION(version) !=
      EGL_VENDOR_ABI_MAJOR_VERSION)
         __eglGLVNDApiExports = exports;
            imports->getPlatformDisplay = __eglGLVNDGetPlatformDisplay;
   imports->getSupportsAPI = _eglIsApiValid;
   imports->getVendorString = __eglGLVNDGetVendorString;
   imports->getProcAddress = __eglGLVNDGetProcAddress;
   imports->getDispatchAddress = __eglDispatchFindDispatchFunction;
               }
