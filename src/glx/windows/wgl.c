   /*
   * Copyright © 2014 Jon Turney
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
      /*
   Wrapper functions for calling WGL extension functions
   */
      #include "wgl.h"
      #include <stdio.h>
      #define RESOLVE_DECL(type) \
            #define PRERESOLVE(type, symbol) \
      type##proc = (type)wglGetProcAddress(symbol); \
   if (type##proc == NULL) \
         #define CHECK_RESOLVED(type, retval) \
   if (type##proc == NULL) { \
         }
      #define RESOLVED_PROC(type) type##proc
      RESOLVE_DECL(PFNWGLGETEXTENSIONSSTRINGARBPROC);
   RESOLVE_DECL(PFNWGLCREATECONTEXTATTRIBSARBPROC);
   RESOLVE_DECL(PFNWGLMAKECONTEXTCURRENTARBPROC);
   RESOLVE_DECL(PFNWGLCREATEPBUFFERARBPROC);
   RESOLVE_DECL(PFNWGLGETPBUFFERDCARBPROC);
   RESOLVE_DECL(PFNWGLRELEASEPBUFFERDCARBPROC);
   RESOLVE_DECL(PFNWGLDESTROYPBUFFERARBPROC);
      void wglResolveExtensionProcs(void)
   {
   PRERESOLVE(PFNWGLGETEXTENSIONSSTRINGARBPROC, "wglGetExtensionsStringARB");
   PRERESOLVE(PFNWGLCREATECONTEXTATTRIBSARBPROC, "wglCreateContextAttribsARB");
   PRERESOLVE(PFNWGLMAKECONTEXTCURRENTARBPROC, "wglMakeContextCurrentARB");
   PRERESOLVE(PFNWGLCREATEPBUFFERARBPROC, "wglCreatePbufferARB");
   PRERESOLVE(PFNWGLGETPBUFFERDCARBPROC, "wglGetPbufferDCARB");
   PRERESOLVE(PFNWGLRELEASEPBUFFERDCARBPROC, "wglReleasePbufferDCARB");
   PRERESOLVE(PFNWGLDESTROYPBUFFERARBPROC, "wglDestroyPbufferARB");
   }
      const char *wglGetExtensionsStringARB(HDC hdc_)
   {
      CHECK_RESOLVED(PFNWGLGETEXTENSIONSSTRINGARBPROC, "");
      }
      HGLRC wglCreateContextAttribsARB(HDC hdc_, HGLRC hShareContext_,
         {
      CHECK_RESOLVED(PFNWGLCREATECONTEXTATTRIBSARBPROC, NULL);
      }
      BOOL wglMakeContextCurrentARB(HDC hDrawDC_, HDC hReadDC_, HGLRC hglrc_)
   {
      CHECK_RESOLVED(PFNWGLMAKECONTEXTCURRENTARBPROC, false);
      }
      HPBUFFERARB wglCreatePbufferARB(HDC hDC_, int iPixelFormat_, int iWidth_,
         {
      CHECK_RESOLVED(PFNWGLCREATEPBUFFERARBPROC, NULL);
      }
      HDC wglGetPbufferDCARB(HPBUFFERARB hPbuffer_)
   {
      CHECK_RESOLVED(PFNWGLGETPBUFFERDCARBPROC, NULL);
      }
      int wglReleasePbufferDCARB(HPBUFFERARB hPbuffer_, HDC hDC_)
   {
      CHECK_RESOLVED(PFNWGLRELEASEPBUFFERDCARBPROC, 0)
      }
      BOOL wglDestroyPbufferARB(HPBUFFERARB hPbuffer_)
   {
      CHECK_RESOLVED(PFNWGLDESTROYPBUFFERARBPROC, false);
      }
