   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
         /**
   * \file clear.c
   * glClearColor, glClearIndex, glClear() functions.
   */
            #include "glformats.h"
   #include "util/glheader.h"
   #include "context.h"
   #include "enums.h"
   #include "fbobject.h"
   #include "get.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "state.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_clear.h"
      void GLAPIENTRY
   _mesa_ClearIndex( GLfloat c )
   {
               ctx->PopAttribState |= GL_COLOR_BUFFER_BIT;
      }
         /**
   * Specify the clear values for the color buffers.
   *
   * \param red red color component.
   * \param green green color component.
   * \param blue blue color component.
   * \param alpha alpha component.
   *
   * \sa glClearColor().
   */
   void GLAPIENTRY
   _mesa_ClearColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
   {
               ctx->PopAttribState |= GL_COLOR_BUFFER_BIT;
   ctx->Color.ClearColor.f[0] = red;
   ctx->Color.ClearColor.f[1] = green;
   ctx->Color.ClearColor.f[2] = blue;
      }
         /**
   * GL_EXT_texture_integer
   */
   void GLAPIENTRY
   _mesa_ClearColorIiEXT(GLint r, GLint g, GLint b, GLint a)
   {
               ctx->PopAttribState |= GL_COLOR_BUFFER_BIT;
   ctx->Color.ClearColor.i[0] = r;
   ctx->Color.ClearColor.i[1] = g;
   ctx->Color.ClearColor.i[2] = b;
      }
         /**
   * GL_EXT_texture_integer
   */
   void GLAPIENTRY
   _mesa_ClearColorIuiEXT(GLuint r, GLuint g, GLuint b, GLuint a)
   {
               ctx->PopAttribState |= GL_COLOR_BUFFER_BIT;
   ctx->Color.ClearColor.ui[0] = r;
   ctx->Color.ClearColor.ui[1] = g;
   ctx->Color.ClearColor.ui[2] = b;
      }
         /**
   * Returns true if color writes are enabled for the given color attachment.
   *
   * Beyond checking ColorMask, this uses _mesa_format_has_color_component to
   * ignore components that don't actually exist in the format (such as X in
   * XRGB).
   */
   static bool
   color_buffer_writes_enabled(const struct gl_context *ctx, unsigned idx)
   {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[idx];
            if (rb) {
      for (c = 0; c < 4; c++) {
      if (GET_COLORMASK_BIT(ctx->Color.ColorMask, idx, c) &&
      _mesa_format_has_color_component(rb->Format, c)) {
                        }
         /**
   * Clear buffers.
   *
   * \param mask bit-mask indicating the buffers to be cleared.
   *
   * Flushes the vertices and verifies the parameter.
   * If __struct gl_contextRec::NewState is set then calls _mesa_update_clear_state()
   * to update gl_frame_buffer::_Xmin, etc.  If the rasterization mode is set to
   * GL_RENDER then requests the driver to clear the buffers, via the
   * dd_function_table::Clear callback.
   */
   static ALWAYS_INLINE void
   clear(struct gl_context *ctx, GLbitfield mask, bool no_error)
   {
               if (!no_error) {
      if (mask & ~(GL_COLOR_BUFFER_BIT |
               GL_DEPTH_BUFFER_BIT |
      _mesa_error( ctx, GL_INVALID_VALUE, "glClear(0x%x)", mask);
               /* Accumulation buffers were removed in core contexts, and they never
   * existed in OpenGL ES.
   */
   if ((mask & GL_ACCUM_BUFFER_BIT) != 0
      && (_mesa_is_desktop_gl_core(ctx) || _mesa_is_gles(ctx))) {
   _mesa_error( ctx, GL_INVALID_VALUE, "glClear(GL_ACCUM_BUFFER_BIT)");
                  if (ctx->NewState) {
                  if (!no_error && ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     if (ctx->RasterDiscard)
            if (ctx->RenderMode == GL_RENDER) {
               /* don't clear depth buffer if depth writing disabled */
   if (!ctx->Depth.Mask)
            /* Build the bitmask to send to device driver's Clear function.
   * Note that the GL_COLOR_BUFFER_BIT flag will expand to 0, 1, 2 or 4
   * of the BUFFER_BIT_FRONT/BACK_LEFT/RIGHT flags, or one of the
   * BUFFER_BIT_COLORn flags.
   */
   bufferMask = 0;
   if (mask & GL_COLOR_BUFFER_BIT) {
      GLuint i;
                     if (buf != BUFFER_NONE && color_buffer_writes_enabled(ctx, i)) {
                        if ((mask & GL_DEPTH_BUFFER_BIT)
      && ctx->DrawBuffer->Visual.depthBits > 0) {
               if ((mask & GL_STENCIL_BUFFER_BIT)
      && ctx->DrawBuffer->Visual.stencilBits > 0) {
               if ((mask & GL_ACCUM_BUFFER_BIT)
      && ctx->DrawBuffer->Visual.accumRedBits > 0) {
                     }
         void GLAPIENTRY
   _mesa_Clear_no_error(GLbitfield mask)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_Clear(GLbitfield mask)
   {
               if (MESA_VERBOSE & VERBOSE_API)
               }
         /** Returned by make_color_buffer_mask() for errors */
   #define INVALID_MASK ~0x0U
         /**
   * Convert the glClearBuffer 'drawbuffer' parameter into a bitmask of
   * BUFFER_BIT_x values.
   * Return INVALID_MASK if the drawbuffer value is invalid.
   */
   static GLbitfield
   make_color_buffer_mask(struct gl_context *ctx, GLint drawbuffer)
   {
      const struct gl_renderbuffer_attachment *att = ctx->DrawBuffer->Attachment;
            /* From the GL 4.0 specification:
   *	If buffer is COLOR, a particular draw buffer DRAW_BUFFERi is
   *	specified by passing i as the parameter drawbuffer, and value
   *	points to a four-element vector specifying the R, G, B, and A
   *	color to clear that draw buffer to. If the draw buffer is one
   *	of FRONT, BACK, LEFT, RIGHT, or FRONT_AND_BACK, identifying
   *	multiple buffers, each selected buffer is cleared to the same
   *	value.
   *
   * Note that "drawbuffer" and "draw buffer" have different meaning.
   * "drawbuffer" specifies DRAW_BUFFERi, while "draw buffer" is what's
   * assigned to DRAW_BUFFERi. It could be COLOR_ATTACHMENT0, FRONT, BACK,
   * etc.
   */
   if (drawbuffer < 0 || drawbuffer >= (GLint)ctx->Const.MaxDrawBuffers) {
                  switch (ctx->DrawBuffer->ColorDrawBuffer[drawbuffer]) {
   case GL_FRONT:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
            case GL_BACK:
      /* For GLES contexts with a single buffered configuration, we actually
   * only have a front renderbuffer, so any clear calls to GL_BACK should
   * affect that buffer. See draw_buffer_enum_to_bitmask for details.
   */
   if (_mesa_is_gles(ctx))
      if (!ctx->DrawBuffer->Visual.doubleBufferMode)
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
   if (att[BUFFER_BACK_LEFT].Renderbuffer)
         if (att[BUFFER_BACK_RIGHT].Renderbuffer)
            case GL_LEFT:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         if (att[BUFFER_BACK_LEFT].Renderbuffer)
            case GL_RIGHT:
      if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
         if (att[BUFFER_BACK_RIGHT].Renderbuffer)
            case GL_FRONT_AND_BACK:
      if (att[BUFFER_FRONT_LEFT].Renderbuffer)
         if (att[BUFFER_BACK_LEFT].Renderbuffer)
         if (att[BUFFER_FRONT_RIGHT].Renderbuffer)
         if (att[BUFFER_BACK_RIGHT].Renderbuffer)
            default:
      {
                     if (buf != BUFFER_NONE && att[buf].Renderbuffer) {
                           }
            /**
   * New in GL 3.0
   * Clear signed integer color buffer or stencil buffer (not depth).
   */
   static ALWAYS_INLINE void
   clear_bufferiv(struct gl_context *ctx, GLenum buffer, GLint drawbuffer,
         {
               if (ctx->NewState) {
                  if (!no_error && ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     switch (buffer) {
   case GL_STENCIL:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "ClearBuffer generates an INVALID VALUE error if buffer is
   *     COLOR and drawbuffer is less than zero, or greater than the
   *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
   *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
   */
   if (!no_error && drawbuffer != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferiv(drawbuffer=%d)",
            }
   else if (ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer
            /* Save current stencil clear value, set to 'value', do the
   * stencil clear and restore the clear value.
   * XXX in the future we may have a new st_ClearBuffer()
   * hook instead.
   */
   const GLuint clearSave = ctx->Stencil.Clear;
   ctx->Stencil.Clear = *value;
   st_Clear(ctx, BUFFER_BIT_STENCIL);
      }
      case GL_COLOR:
      {
      const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
   if (!no_error && mask == INVALID_MASK) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferiv(drawbuffer=%d)",
            }
                     /* save color */
   clearSave = ctx->Color.ClearColor;
   /* set color */
   COPY_4V(ctx->Color.ClearColor.i, value);
   /* clear buffer(s) */
   st_Clear(ctx, mask);
   /* restore color */
         }
      default:
      if (!no_error) {
      /* Page 498 of the PDF, section '17.4.3.1 Clearing Individual Buffers'
   * of the OpenGL 4.5 spec states:
   *
   *    "An INVALID_ENUM error is generated by ClearBufferiv and
   *     ClearNamedFramebufferiv if buffer is not COLOR or STENCIL."
   */
   _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferiv(buffer=%s)",
      }
         }
         void GLAPIENTRY
   _mesa_ClearBufferiv_no_error(GLenum buffer, GLint drawbuffer, const GLint *value)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint *value)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * The ClearBuffer framework is so complicated and so riddled with the
   * assumption that the framebuffer is bound that, for now, we will just fake
   * direct state access clearing for the user.
   */
   void GLAPIENTRY
   _mesa_ClearNamedFramebufferiv(GLuint framebuffer, GLenum buffer,
         {
               _mesa_GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldfb);
   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
   _mesa_ClearBufferiv(buffer, drawbuffer, value);
      }
         /**
   * New in GL 3.0
   * Clear unsigned integer color buffer (not depth, not stencil).
   */
   static ALWAYS_INLINE void
   clear_bufferuiv(struct gl_context *ctx, GLenum buffer, GLint drawbuffer,
         {
               if (ctx->NewState) {
                  if (!no_error && ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION,
                     switch (buffer) {
   case GL_COLOR:
      {
      const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
   if (!no_error && mask == INVALID_MASK) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferuiv(drawbuffer=%d)",
            }
                     /* save color */
   clearSave = ctx->Color.ClearColor;
   /* set color */
   COPY_4V(ctx->Color.ClearColor.ui, value);
   /* clear buffer(s) */
   st_Clear(ctx, mask);
   /* restore color */
         }
      default:
      if (!no_error) {
      /* Page 498 of the PDF, section '17.4.3.1 Clearing Individual Buffers'
   * of the OpenGL 4.5 spec states:
   *
   *    "An INVALID_ENUM error is generated by ClearBufferuiv and
   *     ClearNamedFramebufferuiv if buffer is not COLOR."
   */
   _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferuiv(buffer=%s)",
      }
         }
         void GLAPIENTRY
   _mesa_ClearBufferuiv_no_error(GLenum buffer, GLint drawbuffer,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint *value)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * The ClearBuffer framework is so complicated and so riddled with the
   * assumption that the framebuffer is bound that, for now, we will just fake
   * direct state access clearing for the user.
   */
   void GLAPIENTRY
   _mesa_ClearNamedFramebufferuiv(GLuint framebuffer, GLenum buffer,
         {
               _mesa_GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldfb);
   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
   _mesa_ClearBufferuiv(buffer, drawbuffer, value);
      }
         /**
   * New in GL 3.0
   * Clear fixed-pt or float color buffer or depth buffer (not stencil).
   */
   static ALWAYS_INLINE void
   clear_bufferfv(struct gl_context *ctx, GLenum buffer, GLint drawbuffer,
         {
               if (ctx->NewState) {
                  if (!no_error && ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION,
                     switch (buffer) {
   case GL_DEPTH:
      /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "ClearBuffer generates an INVALID VALUE error if buffer is
   *     COLOR and drawbuffer is less than zero, or greater than the
   *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
   *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
   */
   if (!no_error && drawbuffer != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfv(drawbuffer=%d)",
            }
   else if (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer
            /* Save current depth clear value, set to 'value', do the
   * depth clear and restore the clear value.
   * XXX in the future we may have a new st_ClearBuffer()
   * hook instead.
                  /* Page 263 (page 279 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "If buffer is DEPTH, drawbuffer must be zero, and value points
   *     to the single depth value to clear the depth buffer to.
   *     Clamping and type conversion for fixed-point depth buffers are
   *     performed in the same fashion as for ClearDepth."
   */
   const struct gl_renderbuffer *rb =
         const bool is_float_depth =
                  st_Clear(ctx, BUFFER_BIT_DEPTH);
      }
   /* clear depth buffer to value */
      case GL_COLOR:
      {
      const GLbitfield mask = make_color_buffer_mask(ctx, drawbuffer);
   if (!no_error && mask == INVALID_MASK) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfv(drawbuffer=%d)",
            }
                     /* save color */
   clearSave = ctx->Color.ClearColor;
   /* set color */
   COPY_4V(ctx->Color.ClearColor.f, value);
   /* clear buffer(s) */
   st_Clear(ctx, mask);
   /* restore color */
         }
      default:
      if (!no_error) {
      /* Page 498 of the PDF, section '17.4.3.1 Clearing Individual Buffers'
   * of the OpenGL 4.5 spec states:
   *
   *    "An INVALID_ENUM error is generated by ClearBufferfv and
   *     ClearNamedFramebufferfv if buffer is not COLOR or DEPTH."
   */
   _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferfv(buffer=%s)",
      }
         }
         void GLAPIENTRY
   _mesa_ClearBufferfv_no_error(GLenum buffer, GLint drawbuffer,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat *value)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * The ClearBuffer framework is so complicated and so riddled with the
   * assumption that the framebuffer is bound that, for now, we will just fake
   * direct state access clearing for the user.
   */
   void GLAPIENTRY
   _mesa_ClearNamedFramebufferfv(GLuint framebuffer, GLenum buffer,
         {
               _mesa_GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldfb);
   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
   _mesa_ClearBufferfv(buffer, drawbuffer, value);
      }
         /**
   * New in GL 3.0
   * Clear depth/stencil buffer only.
   */
   static ALWAYS_INLINE void
   clear_bufferfi(struct gl_context *ctx, GLenum buffer, GLint drawbuffer,
         {
                        if (!no_error) {
      if (buffer != GL_DEPTH_STENCIL) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glClearBufferfi(buffer=%s)",
                     /* Page 264 (page 280 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "ClearBuffer generates an INVALID VALUE error if buffer is
   *     COLOR and drawbuffer is less than zero, or greater than the
   *     value of MAX DRAW BUFFERS minus one; or if buffer is DEPTH,
   *     STENCIL, or DEPTH STENCIL and drawbuffer is not zero."
   */
   if (drawbuffer != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glClearBufferfi(drawbuffer=%d)",
                        if (ctx->RasterDiscard)
            if (ctx->NewState) {
                  if (!no_error && ctx->DrawBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     if (ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer)
         if (ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer)
            if (mask) {
      /* save current clear values */
   const GLclampd clearDepthSave = ctx->Depth.Clear;
            /* set new clear values
   *
   * Page 263 (page 279 of the PDF) of the OpenGL 3.0 spec says:
   *
   *     "depth and stencil are the values to clear the depth and stencil
   *     buffers to, respectively. Clamping and type conversion for
   *     fixed-point depth buffers are performed in the same fashion as
   *     for ClearDepth."
   */
   const struct gl_renderbuffer *rb =
         const bool has_float_depth = rb &&
         ctx->Depth.Clear = has_float_depth ? depth : SATURATE(depth);
            /* clear buffers */
            /* restore */
   ctx->Depth.Clear = clearDepthSave;
         }
         void GLAPIENTRY
   _mesa_ClearBufferfi_no_error(GLenum buffer, GLint drawbuffer,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ClearBufferfi(GLenum buffer, GLint drawbuffer,
         {
      GET_CURRENT_CONTEXT(ctx);
      }
         /**
   * The ClearBuffer framework is so complicated and so riddled with the
   * assumption that the framebuffer is bound that, for now, we will just fake
   * direct state access clearing for the user.
   */
   void GLAPIENTRY
   _mesa_ClearNamedFramebufferfi(GLuint framebuffer, GLenum buffer,
         {
               _mesa_GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldfb);
   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
   _mesa_ClearBufferfi(buffer, drawbuffer, depth, stencil);
      }
