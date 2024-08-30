   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "state_tracker/st_interop.h"
   #include "stw_ext_interop.h"
      #include "stw_context.h"
   #include "stw_device.h"
      int
   wglMesaGLInteropQueryDeviceInfo(HDC dpy, HGLRC context,
         {
               if (stw_dev && stw_dev->callbacks.pfnGetDhglrc) {
      /* Convert HGLRC to DHGLRC */
      } else {
      /* not using ICD */
               struct stw_context *ctx = stw_lookup_context(dhglrc);
   if (!ctx)
               }
      int
   stw_interop_query_device_info(struct stw_context *ctx,
         {
         }
      int
   wglMesaGLInteropExportObject(HDC dpy, HGLRC context,
               {
               if (stw_dev && stw_dev->callbacks.pfnGetDhglrc) {
      /* Convert HGLRC to DHGLRC */
      } else {
      /* not using ICD */
               struct stw_context *ctx = stw_lookup_context(dhglrc);
   if (!ctx)
               }
      int
   stw_interop_export_object(struct stw_context *ctx,
               {
         }
      int
   wglMesaGLInteropFlushObjects(HDC dpy, HGLRC context,
               {
               if (stw_dev && stw_dev->callbacks.pfnGetDhglrc) {
      /* Convert HGLRC to DHGLRC */
      } else {
      /* not using ICD */
               struct stw_context *ctx = stw_lookup_context(dhglrc);
   if (!ctx)
               }
      int
   stw_interop_flush_objects(struct stw_context *ctx,
               {
         }
   