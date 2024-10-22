   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2011 Morgan Armand <morgan.devel@gmail.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include <windows.h>
      #define WGL_WGLEXT_PROTOTYPES
      #include <GL/gl.h>
   #include <GL/wglext.h>
      #include "stw_gdishim.h"
   #include "gldrv.h"
   #include "stw_context.h"
   #include "stw_device.h"
   #include "stw_ext_context.h"
      #include "util/u_debug.h"
         static wglCreateContext_t g_pfnwglCreateContext = NULL;
   static wglDeleteContext_t g_pfnwglDeleteContext = NULL;
      /* When this library is used as a opengl32.dll drop-in replacement, ensure we
   * use the wglCreate/Destroy entrypoints above, and not the true opengl32.dll,
   * which could happen if this library's name is not opengl32.dll exactly.
   *
   * For example, Qt 5.4 bundles this as opengl32sw.dll:
   * https://blog.qt.io/blog/2014/11/27/qt-weekly-21-dynamic-opengl-implementation-loading-in-qt-5-4/
   */
   void
   stw_override_opengl32_entry_points(wglCreateContext_t create, wglDeleteContext_t delete)
   {
      g_pfnwglCreateContext = create;
      }
         /**
   * The implementation of this function is tricky.  The OPENGL32.DLL library
   * remaps the context IDs returned by our stw_create_context_attribs()
   * function to different values returned to the caller of wglCreateContext().
   * That is, DHGLRC (driver) handles are not equivalent to HGLRC (public)
   * handles.
   *
   * So we need to generate a new HGLRC ID here.  We do that by calling
   * the regular wglCreateContext() function.  Then, we replace the newly-
   * created stw_context with a new stw_context that reflects the arguments
   * to this function.
   */
   HGLRC WINAPI
   wglCreateContextAttribsARB(HDC hDC, HGLRC hShareContext, const int *attribList)
   {
               int majorVersion = 1, minorVersion = 0, layerPlane = 0;
   int contextFlags = 0x0;
   int profileMask = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
   int resetStrategy = WGL_NO_RESET_NOTIFICATION_ARB;
   int i;
   BOOL done = false;
   const int contextFlagsAll = (WGL_CONTEXT_DEBUG_BIT_ARB |
                  if (!stw_dev)
            /* parse attrib_list */
   if (attribList) {
      for (i = 0; !done && attribList[i]; i++) {
      switch (attribList[i]) {
   case WGL_CONTEXT_MAJOR_VERSION_ARB:
      majorVersion = attribList[++i];
      case WGL_CONTEXT_MINOR_VERSION_ARB:
      minorVersion = attribList[++i];
      case WGL_CONTEXT_LAYER_PLANE_ARB:
      layerPlane = attribList[++i];
      case WGL_CONTEXT_FLAGS_ARB:
      contextFlags = attribList[++i];
      case WGL_CONTEXT_PROFILE_MASK_ARB:
      profileMask = attribList[++i];
      case WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB:
      resetStrategy = attribList[++i];
      case 0:
      /* end of list */
   done = true;
      default:
      /* bad attribute */
   SetLastError(ERROR_INVALID_PARAMETER);
                     /* check contextFlags */
   if (contextFlags & ~contextFlagsAll) {
      SetLastError(ERROR_INVALID_PARAMETER);
               /* check profileMask */
   if (profileMask != WGL_CONTEXT_CORE_PROFILE_BIT_ARB &&
      profileMask != WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB &&
   profileMask != WGL_CONTEXT_ES_PROFILE_BIT_EXT) {
   SetLastError(ERROR_INVALID_PROFILE_ARB);
               /* check version (generate ERROR_INVALID_VERSION_ARB if bad) */
   if (majorVersion <= 0 ||
      minorVersion < 0 ||
   (profileMask != WGL_CONTEXT_ES_PROFILE_BIT_EXT &&
   ((majorVersion == 1 && minorVersion > 5) ||
      (majorVersion == 2 && minorVersion > 1) ||
   (majorVersion == 3 && minorVersion > 3) ||
   (majorVersion == 4 && minorVersion > 6) ||
      (profileMask == WGL_CONTEXT_ES_PROFILE_BIT_EXT &&
   ((majorVersion == 1 && minorVersion > 1) ||
      (majorVersion == 2 && minorVersion > 0) ||
   (majorVersion == 3 && minorVersion > 1) ||
      SetLastError(ERROR_INVALID_VERSION_ARB);
               if ((contextFlags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB) &&
      majorVersion < 3) {
   SetLastError(ERROR_INVALID_VERSION_ARB);
               if (resetStrategy != WGL_NO_RESET_NOTIFICATION_ARB &&
      resetStrategy != WGL_LOSE_CONTEXT_ON_RESET_ARB) {
   SetLastError(ERROR_INVALID_PARAMETER);
               wglCreateContext_t pfnwglCreateContext;
            if (stw_dev->callbacks.pfnGetDhglrc) {
      /* Used as an ICD.
   *
   * Get pointers to OPENGL32.DLL's wglCreate/DeleteContext() functions
   */
   HMODULE opengl_lib = GetModuleHandleA("opengl32.dll");
   if (!opengl_lib) {
      _debug_printf("wgl: GetModuleHandleA(\"opengl32.dll\") failed\n");
               pfnwglCreateContext = (wglCreateContext_t)
         if (!pfnwglCreateContext) {
      _debug_printf("wgl: failed to get wglCreateContext()\n");
               pfnwglDeleteContext = (wglDeleteContext_t)
         if (!pfnwglDeleteContext) {
      _debug_printf("wgl: failed to get wglDeleteContext()\n");
         } else {
      /* Used as opengl32.dll drop-in alternative. */
   assert(g_pfnwglCreateContext != NULL);
   assert(g_pfnwglDeleteContext != NULL);
   pfnwglCreateContext = g_pfnwglCreateContext;
               /* Call wglCreateContext to get a valid context ID */
            if (context) {
      /* Now replace the context we just created with a new one that reflects
   * the attributes passed to this function.
   */
            /* Convert public HGLRC to driver DHGLRC */
   if (stw_dev && stw_dev->callbacks.pfnGetDhglrc) {
      dhglrc = stw_dev->callbacks.pfnGetDhglrc(context);
   if (hShareContext)
      }
   else {
      /* not using ICD */
   dhglrc = (DHGLRC)(INT_PTR)context;
                        const struct stw_pixelformat_info *pfi = stw_pixelformat_get_info_from_hdc(hDC);
   if (!pfi)
            struct stw_context *stw_ctx = stw_create_context_attribs(hDC, layerPlane, share_stw,
                              if (!stw_ctx) {
      pfnwglDeleteContext(context);
               c = stw_create_context_handle(stw_ctx, dhglrc);
   if (!c) {
      stw_destroy_context(stw_ctx);
   pfnwglDeleteContext(context);
                     }
         /** Defined by WGL_ARB_make_current_read */
   BOOL APIENTRY
   wglMakeContextCurrentARB(HDC hDrawDC, HDC hReadDC, HGLRC hglrc)
   {
               if (stw_dev && stw_dev->callbacks.pfnGetDhglrc) {
      /* Convert HGLRC to DHGLRC */
      } else {
      /* not using ICD */
                  }
      HDC APIENTRY
   wglGetCurrentReadDCARB(VOID)
   {
         }
