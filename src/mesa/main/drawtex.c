   /*
   * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "main/errors.h"
   #include "main/state.h"
      #include "main/mtypes.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_drawtex.h"
      static void
   draw_texture(struct gl_context *ctx, GLfloat x, GLfloat y, GLfloat z,
         {
      if (!ctx->Extensions.OES_draw_texture) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (width <= 0.0f || height <= 0.0f) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glDrawTex(width or height <= 0)");
                        if (ctx->NewState)
                        }
         void GLAPIENTRY
   _mesa_DrawTexfOES(GLfloat x, GLfloat y, GLfloat z, GLfloat width, GLfloat height)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DrawTexfvOES(const GLfloat *coords)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DrawTexiOES(GLint x, GLint y, GLint z, GLint width, GLint height)
   {
      GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) x, (GLfloat) y, (GLfloat) z,
      }
         void GLAPIENTRY
   _mesa_DrawTexivOES(const GLint *coords)
   {
      GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) coords[0], (GLfloat) coords[1],
      }
         void GLAPIENTRY
   _mesa_DrawTexsOES(GLshort x, GLshort y, GLshort z, GLshort width, GLshort height)
   {
      GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) x, (GLfloat) y, (GLfloat) z,
      }
         void GLAPIENTRY
   _mesa_DrawTexsvOES(const GLshort *coords)
   {
      GET_CURRENT_CONTEXT(ctx);
   draw_texture(ctx, (GLfloat) coords[0], (GLfloat) coords[1],
      }
