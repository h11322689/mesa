   /*
   * Copyright 2015 VMware, Inc.
   * All Rights Reserved.
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
      #include "main/mtypes.h"
   #include "main/buffers.h"
   #include "main/errors.h"
   #include "main/fbobject.h"
   #include "main/get.h"
   #include "main/teximage.h"
   #include "main/texparam.h"
   #include "st_copytex.h"
   #include "api_exec_decl.h"
         /**
   * Copy a colorbuffer from the window system framebuffer (a window or
   * pbuffer) to a texture.
   * This is a helper used by the wglBindTexImageARB() function.
   *
   * \param srcBuffer  source buffer (GL_FRONT_LEFT, GL_BACK_LEFT, etc)
   * \param fbWidth  width of the source framebuffer
   * \param fbHeight  height of the source framebuffer
   * \param texTarget  which texture target to copy to (GL_TEXTURE_1D/2D/CUBE_MAP)
   * \param texLevel  which texture mipmap level to copy to
   * \param cubeFace  which cube face to copy to (in [0,5])
   * \param texFormat  what texture format to use, if texture doesn't exist
   */
   void
   st_copy_framebuffer_to_texture(GLenum srcBuffer,
                     {
                        /* Save current FBO / readbuffer */
   _mesa_GetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFBOSave);
            /* Read from the winsys buffer */
   _mesa_BindFramebuffer(GL_READ_FRAMEBUFFER, 0);
            /* copy image from pbuffer to texture */
   switch (texTarget) {
   case GL_TEXTURE_1D:
      _mesa_GetTexLevelParameteriv(GL_TEXTURE_1D, texLevel,
         if (width == fbWidth) {
      /* replace existing texture */
   _mesa_CopyTexSubImage1D(GL_TEXTURE_1D,
                        } else {
      /* define initial texture */
   _mesa_CopyTexImage1D(GL_TEXTURE_1D,
                        }
      case GL_TEXTURE_2D:
      _mesa_GetTexLevelParameteriv(GL_TEXTURE_2D, texLevel,
         _mesa_GetTexLevelParameteriv(GL_TEXTURE_2D, texLevel,
         if (width == fbWidth && height == fbHeight) {
      /* replace existing texture */
   _mesa_CopyTexSubImage2D(GL_TEXTURE_2D,
                        } else {
      /* define initial texture */
   _mesa_CopyTexImage2D(GL_TEXTURE_2D,
                        }
      case GL_TEXTURE_CUBE_MAP:
      {
      const GLenum target =
         _mesa_GetTexLevelParameteriv(target, texLevel,
         _mesa_GetTexLevelParameteriv(target, texLevel,
         if (width == fbWidth && height == fbHeight) {
      /* replace existing texture */
   _mesa_CopyTexSubImage2D(target,
                        } else {
      /* define new texture */
   _mesa_CopyTexImage2D(target,
                           }
      default:
      _mesa_problem(NULL,
               /* restore readbuffer */
   _mesa_ReadBuffer(readBufSave);
      }
