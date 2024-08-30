   /**************************************************************************
   * Copyright 2015 VMware, Inc.
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
      #include "state_tracker/st_copytex.h"
      #include "pipe/p_defines.h"
   #include "pipe/p_screen.h"
   #include "pipe/p_state.h"
      #include "stw_gdishim.h"
   #include "gldrv.h"
   #include "stw_context.h"
   #include "stw_device.h"
   #include "stw_pixelformat.h"
   #include "stw_framebuffer.h"
   #include "stw_st.h"
         /** Translate a WGL buffer name to a GLenum */
   static GLenum
   translate_ibuffer(int iBuffer)
   {
      switch (iBuffer) {
   case WGL_FRONT_LEFT_ARB:
         case WGL_BACK_LEFT_ARB:
         case WGL_FRONT_RIGHT_ARB:
         case WGL_BACK_RIGHT_ARB:
         case WGL_AUX0_ARB:
         default:
            }
         /** Translate a WGL texture target type to a GLenum */
   static GLenum
   translate_target(unsigned textureTarget)
   {
      switch (textureTarget) {
   case WGL_TEXTURE_1D_ARB:
         case WGL_TEXTURE_2D_ARB:
         case WGL_TEXTURE_CUBE_MAP_ARB:
         case WGL_NO_TEXTURE_ARB:
   default:
            }
         /** Translate a WGL texture format to a GLenum */
   static GLenum
   translate_texture_format(unsigned wgl_format)
   {
      switch (wgl_format) {
   case WGL_TEXTURE_RGB_ARB:
         case WGL_TEXTURE_RGBA_ARB:
         case WGL_NO_TEXTURE_ARB:
   default:
            }
         BOOL WINAPI
   wglBindTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
   {
      struct stw_context *curctx = stw_current_context();
   struct stw_framebuffer *fb, *old_fb, *old_fbRead;
   GLenum texFormat, srcBuffer, target;
   bool retVal;
            /*
   * Implementation notes:
   * Ideally, we'd implement this function with the
   * st_context_teximage() function which replaces a specific
   * texture image with a different resource (the pbuffer).
   * The main problem however, is the pbuffer image is upside down relative
   * to the texture image.
   * Window system drawing surfaces (windows & pbuffers) are "top to bottom"
   * while OpenGL texture images are "bottom to top".  One possible solution
   * to this is to invert rendering to pbuffers (as we do for renderbuffers)
   * but that could lead to other issues (and would require extensive
   * testing).
   *
   * The simple alternative is to use a copy-based approach which copies the
   * pbuffer image into the texture via glCopyTex[Sub]Image.  That's what
   * we do here.
            if (!curctx) {
      debug_printf("No rendering context in wglBindTexImageARB()\n");
   SetLastError(ERROR_INVALID_OPERATION);
               fb = stw_framebuffer_from_HPBUFFERARB(hPbuffer);
   if (!fb) {
      debug_printf("Invalid pbuffer handle in wglBindTexImageARB()\n");
   SetLastError(ERROR_INVALID_HANDLE);
               srcBuffer = translate_ibuffer(iBuffer);
   if (srcBuffer == GL_NONE) {
      debug_printf("Invalid buffer 0x%x in wglBindTexImageARB()\n", iBuffer);
   SetLastError(ERROR_INVALID_DATA);
               target = translate_target(fb->textureTarget);
   if (target == GL_NONE) {
      debug_printf("no texture target in wglBindTexImageARB()\n");
               texFormat = translate_texture_format(fb->textureFormat);
   if (texFormat == GL_NONE) {
      debug_printf("no texture format in wglBindTexImageARB()\n");
               old_fb = curctx->current_framebuffer;
            /*
   * Bind the pbuffer surface so we can read/copy from it.
   *
   * Before we can call stw_make_current() we have to temporarily
   * change the pbuffer's pixel format to match the context to avoid
   * an error condition.  After the stw_make_current() we restore the
   * buffer's pixel format.
   */
   pfiSave = fb->pfi;
   fb->pfi = curctx->pfi;
   retVal = stw_make_current(fb, fb, curctx);
   fb->pfi = pfiSave;
   if (!retVal) {
      debug_printf("stw_make_current(#1) failed in wglBindTexImageARB()\n");
               st_copy_framebuffer_to_texture(srcBuffer, fb->width, fb->height,
                  /* rebind previous drawing surface */
   retVal = stw_make_current(old_fb, old_fbRead, curctx);
   if (!retVal) {
                     }
         BOOL WINAPI
   wglReleaseTexImageARB(HPBUFFERARB hPbuffer, int iBuffer)
   {
      struct stw_framebuffer *fb = stw_framebuffer_from_HPBUFFERARB(hPbuffer);
                     if (!fb) {
      debug_printf("Invalid pbuffer handle in wglReleaseTexImageARB()\n");
   SetLastError(ERROR_INVALID_HANDLE);
               srcBuffer = translate_ibuffer(iBuffer);
   if (srcBuffer == GL_NONE) {
      debug_printf("Invalid buffer 0x%x in wglReleaseTexImageARB()\n", iBuffer);
   SetLastError(ERROR_INVALID_DATA);
                  }
         BOOL WINAPI
   wglSetPbufferAttribARB(HPBUFFERARB hPbuffer, const int *piAttribList)
   {
      struct stw_framebuffer *fb = stw_framebuffer_from_HPBUFFERARB(hPbuffer);
            if (!fb) {
      SetLastError(ERROR_INVALID_HANDLE);
               for (i = 0; piAttribList[i]; i += 2) {
      switch (piAttribList[i]) {
   case WGL_MIPMAP_LEVEL_ARB:
      fb->textureLevel = piAttribList[i+1];
      case WGL_CUBE_MAP_FACE_ARB:
      face = piAttribList[i+1];
   if (face >= WGL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB &&
      face <= WGL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB) {
      }
   else {
      debug_printf("Invalid cube face 0x%x in "
               SetLastError(ERROR_INVALID_DATA);
      }
      default:
      debug_printf("Invalid attribute 0x%x in wglSetPbufferAttribARB()\n",
         SetLastError(ERROR_INVALID_DATA);
                     }
