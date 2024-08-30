   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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
      #include "util/glheader.h"
   #include "accum.h"
   #include "condrender.h"
   #include "context.h"
   #include "format_unpack.h"
   #include "format_pack.h"
   #include "framebuffer.h"
   #include "renderbuffer.h"
   #include "macros.h"
   #include "state.h"
   #include "mtypes.h"
   #include "api_exec_decl.h"
      void GLAPIENTRY
   _mesa_ClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
   {
      GLfloat tmp[4];
            tmp[0] = CLAMP( red,   -1.0F, 1.0F );
   tmp[1] = CLAMP( green, -1.0F, 1.0F );
   tmp[2] = CLAMP( blue,  -1.0F, 1.0F );
            if (TEST_EQ_4V(tmp, ctx->Accum.ClearColor))
            ctx->PopAttribState |= GL_ACCUM_BUFFER_BIT;
      }
         /**
   * Clear the accumulation buffer by mapping the renderbuffer and
   * writing the clear color to it.  Called by the driver's implementation
   * of the glClear function.
   */
   void
   _mesa_clear_accum_buffer(struct gl_context *ctx)
   {
      GLuint x, y, width, height;
   GLubyte *accMap;
   GLint accRowStride;
            if (!ctx->DrawBuffer)
            accRb = ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer;
   if (!accRb)
                     /* bounds, with scissor */
   x = ctx->DrawBuffer->_Xmin;
   y = ctx->DrawBuffer->_Ymin;
   width = ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
            _mesa_map_renderbuffer(ctx, accRb, x, y, width, height,
                  if (!accMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               if (accRb->Format == MESA_FORMAT_RGBA_SNORM16) {
      const GLshort clearR = FLOAT_TO_SHORT(ctx->Accum.ClearColor[0]);
   const GLshort clearG = FLOAT_TO_SHORT(ctx->Accum.ClearColor[1]);
   const GLshort clearB = FLOAT_TO_SHORT(ctx->Accum.ClearColor[2]);
   const GLshort clearA = FLOAT_TO_SHORT(ctx->Accum.ClearColor[3]);
            for (j = 0; j < height; j++) {
               for (i = 0; i < width; i++) {
      row[i * 4 + 0] = clearR;
   row[i * 4 + 1] = clearG;
   row[i * 4 + 2] = clearB;
      }
         }
   else {
      /* other types someday? */
                  }
         /**
   * if (bias)
   *    Accum += value
   * else
   *    Accum *= value
   */
   static void
   accum_scale_or_bias(struct gl_context *ctx, GLfloat value,
               {
      struct gl_renderbuffer *accRb =
         GLubyte *accMap;
                     _mesa_map_renderbuffer(ctx, accRb, xpos, ypos, width, height,
                        if (!accMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               if (accRb->Format == MESA_FORMAT_RGBA_SNORM16) {
      const GLshort incr = (GLshort) (value * 32767.0f);
   GLint i, j;
   if (bias) {
      for (j = 0; j < height; j++) {
      GLshort *acc = (GLshort *) accMap;
   for (i = 0; i < 4 * width; i++) {
         }
         }
   else {
      /* scale */
   for (j = 0; j < height; j++) {
      GLshort *acc = (GLshort *) accMap;
   for (i = 0; i < 4 * width; i++) {
         }
            }
   else {
                     }
         /**
   * if (load)
   *    Accum = ColorBuf * value
   * else
   *    Accum += ColorBuf * value
   */
   static void
   accum_or_load(struct gl_context *ctx, GLfloat value,
               {
      struct gl_renderbuffer *accRb =
         struct gl_renderbuffer *colorRb = ctx->ReadBuffer->_ColorReadBuffer;
   GLubyte *accMap, *colorMap;
   GLint accRowStride, colorRowStride;
            if (!colorRb) {
      /* no read buffer - OK */
                        mappingFlags = GL_MAP_WRITE_BIT;
   if (!load) /* if we're accumulating */
            /* Map accum buffer */
   _mesa_map_renderbuffer(ctx, accRb, xpos, ypos, width, height,
               if (!accMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               /* Map color buffer */
   _mesa_map_renderbuffer(ctx, colorRb, xpos, ypos, width, height,
                     if (!colorMap) {
      _mesa_unmap_renderbuffer(ctx, accRb);
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               if (accRb->Format == MESA_FORMAT_RGBA_SNORM16) {
      const GLfloat scale = value * 32767.0f;
   GLint i, j;
            rgba = malloc(width * 4 * sizeof(GLfloat));
   if (rgba) {
                                       if (load) {
      for (i = 0; i < width; i++) {
      acc[i * 4 + 0] = (GLshort) (rgba[i][RCOMP] * scale);
   acc[i * 4 + 1] = (GLshort) (rgba[i][GCOMP] * scale);
   acc[i * 4 + 2] = (GLshort) (rgba[i][BCOMP] * scale);
         }
   else {
      /* accumulate */
   for (i = 0; i < width; i++) {
      acc[i * 4 + 0] += (GLshort) (rgba[i][RCOMP] * scale);
   acc[i * 4 + 1] += (GLshort) (rgba[i][GCOMP] * scale);
   acc[i * 4 + 2] += (GLshort) (rgba[i][BCOMP] * scale);
                  colorMap += colorRowStride;
                  }
   else {
            }
   else {
                  _mesa_unmap_renderbuffer(ctx, accRb);
      }
         /**
   * ColorBuffer = Accum * value
   */
   static void
   accum_return(struct gl_context *ctx, GLfloat value,
         {
      struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct gl_renderbuffer *accRb = fb->Attachment[BUFFER_ACCUM].Renderbuffer;
   GLubyte *accMap, *colorMap;
   GLint accRowStride, colorRowStride;
            /* Map accum buffer */
   _mesa_map_renderbuffer(ctx, accRb, xpos, ypos, width, height,
               if (!accMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               /* Loop over destination buffers */
   for (buffer = 0; buffer < fb->_NumColorDrawBuffers; buffer++) {
      struct gl_renderbuffer *colorRb = fb->_ColorDrawBuffers[buffer];
   const GLboolean masking = (!GET_COLORMASK_BIT(ctx->Color.ColorMask, buffer, 0) ||
                              if (masking)
            /* Map color buffer */
   _mesa_map_renderbuffer(ctx, colorRb, xpos, ypos, width, height,
               if (!colorMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glAccum");
               if (accRb->Format == MESA_FORMAT_RGBA_SNORM16) {
      const GLfloat scale = value / 32767.0f;
                                 if (rgba && dest) {
                        for (i = 0; i < width; i++) {
      rgba[i][0] = acc[i * 4 + 0] * scale;
   rgba[i][1] = acc[i * 4 + 1] * scale;
                                                /* use the dest colors where mask[channel] = 0 */
   if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, buffer, 0)) {
      for (i = 0; i < width; i++)
      }
   if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, buffer, 1)) {
      for (i = 0; i < width; i++)
      }
   if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, buffer, 2)) {
      for (i = 0; i < width; i++)
      }
   if (!GET_COLORMASK_BIT(ctx->Color.ColorMask, buffer, 3)) {
                                          accMap += accRowStride;
         }
   else {
         }
   free(rgba);
      }
   else {
                                 }
            /**
   * Software fallback for glAccum.  A hardware driver that supports
   * signed 16-bit color channels could implement hardware accumulation
   * operations, but no driver does so at this time.
   */
   static void
   accum(struct gl_context *ctx, GLenum op, GLfloat value)
   {
               if (!ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer) {
      _mesa_warning(ctx, "Calling glAccum() without an accumulation buffer");
               if (!_mesa_check_conditional_render(ctx))
                     xpos = ctx->DrawBuffer->_Xmin;
   ypos = ctx->DrawBuffer->_Ymin;
   width =  ctx->DrawBuffer->_Xmax - ctx->DrawBuffer->_Xmin;
            switch (op) {
   case GL_ADD:
      if (value != 0.0F) {
         }
      case GL_MULT:
      if (value != 1.0F) {
         }
      case GL_ACCUM:
      if (value != 0.0F) {
         }
      case GL_LOAD:
      accum_or_load(ctx, value, xpos, ypos, width, height, GL_TRUE);
      case GL_RETURN:
      accum_return(ctx, value, xpos, ypos, width, height);
      default:
      unreachable("invalid mode in _mesa_Accum()");
         }
         void
   _mesa_init_accum( struct gl_context *ctx )
   {
      /* Accumulate buffer group */
      }
         void GLAPIENTRY
   _mesa_Accum( GLenum op, GLfloat value )
   {
      GET_CURRENT_CONTEXT(ctx);
            switch (op) {
   case GL_ADD:
   case GL_MULT:
   case GL_ACCUM:
   case GL_LOAD:
   case GL_RETURN:
      /* OK */
      default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glAccum(op)");
               if (ctx->DrawBuffer->Visual.accumRedBits == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glAccum(no accum buffer)");
               if (ctx->DrawBuffer != ctx->ReadBuffer) {
      /* See GLX_SGI_make_current_read or WGL_ARB_make_current_read,
   * or GL_EXT_framebuffer_blit.
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (ctx->NewState)
            if (ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     if (ctx->RasterDiscard)
            if (ctx->RenderMode == GL_RENDER) {
            }
