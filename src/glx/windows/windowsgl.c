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
      #include "windowsgl.h"
   #include "windowsgl_internal.h"
      #include "glapi.h"
   #include "wgl.h"
      #include <dlfcn.h>
   #include <assert.h>
   #include <stdio.h>
   #include <strings.h>
      static struct _glapi_table *windows_api = NULL;
      static void *
   windows_get_dl_handle(void)
   {
               if (!dl_handle)
               }
      static void
   windows_glapi_create_table(void)
   {
      if (windows_api)
            windows_api = _glapi_create_table_from_handle(windows_get_dl_handle(), "gl");
      }
      static void windows_glapi_set_dispatch(void)
   {
      windows_glapi_create_table();
      }
      windowsContext *
   windows_create_context(int pxfi, windowsContext *shared)
   {
               gc = calloc(1, sizeof *gc);
   if (gc == NULL)
               #define GL_TEMP_WINDOW_CLASS "glTempWndClass"
      {
               if (glTempWndClass == 0) {
      WNDCLASSEX wc;
   wc.cbSize = sizeof(WNDCLASSEX);
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = DefWindowProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = 0;
   wc.hCursor = 0;
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = GL_TEMP_WINDOW_CLASS;
   wc.hIconSm = 0;
                  HWND hwnd = CreateWindowExA(0,
                                          // We must set the windows pixel format before we can create a WGL context
   gc->pxfi = pxfi;
                     if (shared && gc->ctx)
            ReleaseDC(hwnd, hdc);
            if (!gc->ctx)
   {
   free(gc);
   return NULL;
               }
      windowsContext *
   windows_create_context_attribs(int pxfi, windowsContext *shared, const int *attribList)
   {
               gc = calloc(1, sizeof *gc);
   if (gc == NULL)
               #define GL_TEMP_WINDOW_CLASS "glTempWndClass"
      {
               if (glTempWndClass == 0) {
      WNDCLASSEX wc;
   wc.cbSize = sizeof(WNDCLASSEX);
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = DefWindowProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = 0;
   wc.hCursor = 0;
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = GL_TEMP_WINDOW_CLASS;
   wc.hIconSm = 0;
                  HWND hwnd = CreateWindowExA(0,
                                 HDC hdc = GetDC(hwnd);
   HGLRC shareContext = NULL;
   if (shared)
            // We must set the windows pixel format before we can create a WGL context
   gc->pxfi = pxfi;
                     ReleaseDC(hwnd, hdc);
            if (!gc->ctx)
   {
   free(gc);
   return NULL;
               }
      void
   windows_destroy_context(windowsContext *context)
   {
      wglDeleteContext(context->ctx);
   context->ctx = NULL;
      }
      int windows_bind_context(windowsContext *context, windowsDrawable *draw, windowsDrawable *read)
   {
               if (!draw->pxfi)
   {
      SetPixelFormat(drawDc, context->pxfi, NULL);
               if ((read != NULL) &&  (read != draw))
   {
      /*
   If there is a separate read drawable, create a separate read DC, and
   use the wglMakeContextCurrent extension to make the context current
            Should only occur when WGL_ARB_make_current_read extension is present
   */
                              if (!ret) {
      printf("wglMakeContextCurrentARB error: %08x\n", (int)GetLastError());
         }
   else
   {
      /* Otherwise, just use wglMakeCurrent */
   BOOL ret = wglMakeCurrent(drawDc, context->ctx);
   if (!ret) {
      printf("wglMakeCurrent error: %08x\n", (int)GetLastError());
                                       }
      void windows_unbind_context(windowsContext * context)
   {
         }
      /*
   *
   */
      void
   windows_swap_buffers(windowsDrawable *draw)
   {
      HDC drawDc = GetDC(draw->hWnd);
   SwapBuffers(drawDc);
      }
         typedef void (__stdcall * PFNGLADDSWAPHINTRECTWIN) (GLint x, GLint y,
                  static void
   glAddSwapHintRectWIN(GLint x, GLint y, GLsizei width,
         {
      PFNGLADDSWAPHINTRECTWIN proc = (PFNGLADDSWAPHINTRECTWIN)wglGetProcAddress("glAddSwapHintRectWIN");
   if (proc)
      }
      void
   windows_copy_subbuffer(windowsDrawable *draw,
         {
      glAddSwapHintRectWIN(x, y, width, height);
      }
      /*
   * Helper function for calling a test function on an initial context
   */
   static void
   windows_call_with_context(void (*proc)(HDC, void *), void *args)
   {
         #define WIN_GL_TEST_WINDOW_CLASS "GLTest"
      {
               if (glTestWndClass == 0) {
               wc.cbSize = sizeof(WNDCLASSEX);
   wc.style = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc = DefWindowProc;
   wc.cbClsExtra = 0;
   wc.cbWndExtra = 0;
   wc.hInstance = GetModuleHandle(NULL);
   wc.hIcon = 0;
   wc.hCursor = 0;
   wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wc.lpszMenuName = NULL;
   wc.lpszClassName = WIN_GL_TEST_WINDOW_CLASS;
   wc.hIconSm = 0;
                  // create an invisible window for a scratch DC
   HWND hwnd = CreateWindowExA(0,
                           if (hwnd) {
               // we must set a pixel format before we can create a context, just use the first one...
   SetPixelFormat(hdc, 1, NULL);
   HGLRC hglrc = wglCreateContext(hdc);
            // call the test function
            // clean up
   wglMakeCurrent(NULL, NULL);
   wglDeleteContext(hglrc);
   ReleaseDC(hwnd, hdc);
         }
      static void
   windows_check_render_test(HDC hdc, void *args)
   {
               /* Rather than play linkage games using stdcall to ensure we get
         void *dlhandle = windows_get_dl_handle();
   const char *(*proc)(int) = dlsym(dlhandle, "glGetString");
            if ((!gl_renderer) || (strcasecmp(gl_renderer, "GDI Generic") == 0))
         else
      }
      int
   windows_check_renderer(void)
   {
      int result;
   windows_call_with_context(windows_check_render_test, &result);
      }
      typedef struct {
      char *gl_extensions;
      } windows_extensions_result;
      static void
   windows_extensions_test(HDC hdc, void *args)
   {
               void *dlhandle = windows_get_dl_handle();
                     wglResolveExtensionProcs();
      }
      void
   windows_extensions(char **gl_extensions, char **wgl_extensions)
   {
               *gl_extensions = "";
                     *gl_extensions = result.gl_extensions;
      }
