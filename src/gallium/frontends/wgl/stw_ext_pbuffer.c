   /**************************************************************************
   * 
   * Copyright 2010 VMware, Inc.
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
      #include <windows.h>
      #define WGL_WGLEXT_PROTOTYPES
      #include <GL/gl.h>
   #include <GL/wglext.h>
      #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
      #include "util/u_debug.h"
      #include "stw_device.h"
   #include "stw_pixelformat.h"
   #include "stw_framebuffer.h"
         #define LARGE_WINDOW_SIZE 60000
         static LRESULT CALLBACK
   WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
   {
   #ifndef _GAMING_XBOX
      MINMAXINFO *pMMI;
   switch (uMsg) {
   case WM_GETMINMAXINFO:
      // Allow to create a window bigger than the desktop
   pMMI = (MINMAXINFO *)lParam;
   pMMI->ptMaxSize.x = LARGE_WINDOW_SIZE;
   pMMI->ptMaxSize.y = LARGE_WINDOW_SIZE;
   pMMI->ptMaxTrackSize.x = LARGE_WINDOW_SIZE;
   pMMI->ptMaxTrackSize.y = LARGE_WINDOW_SIZE;
      default:
            #endif /* _GAMING_XBOX */
            }
      struct stw_framebuffer *
   stw_pbuffer_create(const struct stw_pixelformat_info *pfi, int iWidth, int iHeight, struct pipe_frontend_screen *fscreen)
   {
               /*
   * Implement pbuffers through invisible windows
            if (first) {
      WNDCLASS wc;
   memset(&wc, 0, sizeof wc);
   #ifndef _GAMING_XBOX
         wc.hCursor = LoadCursor(NULL, IDC_ARROW);
   #endif
         wc.lpfnWndProc = WndProc;
   wc.lpszClassName = "wglpbuffer";
   wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
   RegisterClass(&wc);
               DWORD dwExStyle = 0;
            if (0) {
      /*
   * Don't hide the window -- useful for debugging what the application is
   * drawing
               } else {
                           /*
   * The CreateWindowEx parameters are the total (outside) dimensions of the
   * window, which can vary with Windows version and user settings.  Use
   * AdjustWindowRect to get the required total area for the given client area.
   *
   * AdjustWindowRectEx does not accept WS_OVERLAPPED style (which is defined
   * as 0), which means we need to use some other style instead, e.g.,
   * WS_OVERLAPPEDWINDOW or WS_POPUPWINDOW as above.
                     HWND hWnd = CreateWindowEx(dwExStyle,
                              "wglpbuffer", /* wc.lpszClassName */
   NULL,
   dwStyle,
   CW_USEDEFAULT, /* x */
   CW_USEDEFAULT, /* y */
   rect.right - rect.left, /* width */
      if (!hWnd) {
               #ifdef DEBUG
      /*
   * Verify the client area size matches the specified size.
            GetClientRect(hWnd, &rect);
   assert(rect.left == 0);
   assert(rect.top == 0);
   assert(rect.right - rect.left == iWidth);
      #endif
            }
         HPBUFFERARB WINAPI
   wglCreatePbufferARB(HDC hCurrentDC,
                     int iPixelFormat,
   {
      const int *piAttrib;
   int useLargest = 0;
   struct stw_framebuffer *fb;
   HWND hWnd;
   int iDisplayablePixelFormat;
   PIXELFORMATDESCRIPTOR pfd;
   BOOL bRet;
   int textureFormat = WGL_NO_TEXTURE_ARB;
   int textureTarget = WGL_NO_TEXTURE_ARB;
   BOOL textureMipmap = false;
            if (!pfi) {
      SetLastError(ERROR_INVALID_PIXEL_FORMAT);
               if (iWidth <= 0 || iHeight <= 0) {
      SetLastError(ERROR_INVALID_DATA);
               if (piAttribList) {
      for (piAttrib = piAttribList; *piAttrib; piAttrib++) {
      switch (*piAttrib) {
   case WGL_PBUFFER_LARGEST_ARB:
      piAttrib++;
   useLargest = *piAttrib;
      case WGL_TEXTURE_FORMAT_ARB:
      /* WGL_ARB_render_texture */
   piAttrib++;
   textureFormat = *piAttrib;
   if (textureFormat != WGL_TEXTURE_RGB_ARB &&
      textureFormat != WGL_TEXTURE_RGBA_ARB &&
   textureFormat != WGL_NO_TEXTURE_ARB) {
   SetLastError(ERROR_INVALID_DATA);
      }
      case WGL_TEXTURE_TARGET_ARB:
      /* WGL_ARB_render_texture */
   piAttrib++;
   textureTarget = *piAttrib;
   if (textureTarget != WGL_TEXTURE_CUBE_MAP_ARB &&
      textureTarget != WGL_TEXTURE_1D_ARB &&
   textureTarget != WGL_TEXTURE_2D_ARB &&
   textureTarget != WGL_NO_TEXTURE_ARB) {
   SetLastError(ERROR_INVALID_DATA);
      }
      case WGL_MIPMAP_TEXTURE_ARB:
      /* WGL_ARB_render_texture */
   piAttrib++;
   textureMipmap = !!*piAttrib;
      default:
      SetLastError(ERROR_INVALID_DATA);
   debug_printf("wgl: Unsupported attribute 0x%x in %s\n",
                           if (iWidth > stw_dev->max_2d_length) {
      if (useLargest) {
         } else {
      SetLastError(ERROR_NO_SYSTEM_RESOURCES);
                  if (iHeight > stw_dev->max_2d_length) {
      if (useLargest) {
         } else {
      SetLastError(ERROR_NO_SYSTEM_RESOURCES);
                  /*
   * We can't pass non-displayable pixel formats to GDI, which is why we
   * create the framebuffer object before calling SetPixelFormat().
   */
   fb = stw_pbuffer_create(pfi, iWidth, iHeight, stw_dev->fscreen);
   if (!fb) {
      SetLastError(ERROR_NO_SYSTEM_RESOURCES);
               /* WGL_ARB_render_texture fields */
   fb->textureTarget = textureTarget;
   fb->textureFormat = textureFormat;
            iDisplayablePixelFormat = fb->iDisplayablePixelFormat;
                     /*
   * We need to set a displayable pixel format on the hidden window DC
   * so that wglCreateContext and wglMakeCurrent are not overruled by GDI.
   */
   bRet = SetPixelFormat(GetDC(hWnd), iDisplayablePixelFormat, &pfd);
               }
         HDC WINAPI
   wglGetPbufferDCARB(HPBUFFERARB hPbuffer)
   {
      struct stw_framebuffer *fb;
            if (!hPbuffer) {
      SetLastError(ERROR_INVALID_HANDLE);
                                    }
         int WINAPI
   wglReleasePbufferDCARB(HPBUFFERARB hPbuffer,
         {
               if (!hPbuffer) {
      SetLastError(ERROR_INVALID_HANDLE);
                           }
         BOOL WINAPI
   wglDestroyPbufferARB(HPBUFFERARB hPbuffer)
   {
               if (!hPbuffer) {
      SetLastError(ERROR_INVALID_HANDLE);
                        /* This will destroy all our data */
      }
         BOOL WINAPI
   wglQueryPbufferARB(HPBUFFERARB hPbuffer,
               {
               if (!hPbuffer) {
      SetLastError(ERROR_INVALID_HANDLE);
                        switch (iAttribute) {
   case WGL_PBUFFER_WIDTH_ARB:
      *piValue = fb->width;
      case WGL_PBUFFER_HEIGHT_ARB:
      *piValue = fb->height;
      case WGL_PBUFFER_LOST_ARB:
      /* We assume that no content is ever lost due to display mode change */
   *piValue = false;
      /* WGL_ARB_render_texture */
   case WGL_TEXTURE_TARGET_ARB:
      *piValue = fb->textureTarget;
      case WGL_TEXTURE_FORMAT_ARB:
      *piValue = fb->textureFormat;
      case WGL_MIPMAP_TEXTURE_ARB:
      *piValue = fb->textureMipmap;
      case WGL_MIPMAP_LEVEL_ARB:
      *piValue = fb->textureLevel;
      case WGL_CUBE_MAP_FACE_ARB:
      *piValue = fb->textureFace + WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
      default:
      SetLastError(ERROR_INVALID_DATA);
         }
