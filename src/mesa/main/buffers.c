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
   * \file buffers.c
   * glReadBuffer, DrawBuffer functions.
   */
            #include "util/glheader.h"
   #include "buffers.h"
   #include "context.h"
   #include "enums.h"
   #include "fbobject.h"
   #include "framebuffer.h"
   #include "hash.h"
   #include "mtypes.h"
   #include "state.h"
   #include "util/bitscan.h"
   #include "util/u_math.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_manager.h"
   #include "state_tracker/st_atom.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_util.h"
      #define BAD_MASK ~0u
         /**
   * Return bitmask of BUFFER_BIT_* flags indicating which color buffers are
   * available to the rendering context (for drawing or reading).
   * This depends on the type of framebuffer.  For window system framebuffers
   * we look at the framebuffer's visual.  But for user-create framebuffers we
   * look at the number of supported color attachments.
   * \param fb  the framebuffer to draw to, or read from
   * \return  bitmask of BUFFER_BIT_* flags
   */
   static GLbitfield
   supported_buffer_bitmask(const struct gl_context *ctx,
         {
               if (_mesa_is_user_fbo(fb)) {
      /* A user-created renderbuffer */
      }
   else {
      /* A window system framebuffer */
   mask = BUFFER_BIT_FRONT_LEFT; /* always have this */
   if (fb->Visual.stereoMode) {
      mask |= BUFFER_BIT_FRONT_RIGHT;
   if (fb->Visual.doubleBufferMode) {
            }
   else if (fb->Visual.doubleBufferMode) {
                        }
      GLenum
   _mesa_back_to_front_if_single_buffered(const struct gl_framebuffer *fb,
         {
      /* If the front buffer is the only buffer, GL_BACK and all other flags
   * that include BACK select the front buffer for drawing. There are
   * several reasons we want to do this.
   *
   * 1) OpenGL ES 3.0 requires it:
   *
   *   Page 181 (page 192 of the PDF) in section 4.2.1 of the OpenGL
   *   ES 3.0.1 specification says:
   *
   *     "When draw buffer zero is BACK, color values are written
   *     into the sole buffer for single-buffered contexts, or into
   *     the back buffer for double-buffered contexts."
   *
   *   We also do this for GLES 1 and 2 because those APIs have no
   *   concept of selecting the front and back buffer anyway and it's
   *   convenient to be able to maintain the magic behaviour of
   *   GL_BACK in that case.
   *
   * 2) Pbuffers are back buffers from the application point of view,
   *    but they are front buffers from the Mesa point of view,
   *    because they are always single buffered.
   */
   if (!fb->Visual.doubleBufferMode) {
      switch (buffer) {
   case GL_BACK:
      buffer = GL_FRONT;
      case GL_BACK_RIGHT:
      buffer = GL_FRONT_RIGHT;
      case GL_BACK_LEFT:
      buffer = GL_FRONT_LEFT;
                     }
      /**
   * Helper routine used by glDrawBuffer and glDrawBuffersARB.
   * Given a GLenum naming one or more color buffers (such as
   * GL_FRONT_AND_BACK), return the corresponding bitmask of BUFFER_BIT_* flags.
   */
   static GLbitfield
   draw_buffer_enum_to_bitmask(const struct gl_context *ctx, GLenum buffer)
   {
               switch (buffer) {
      case GL_NONE:
         case GL_FRONT:
         case GL_BACK:
         case GL_RIGHT:
         case GL_FRONT_RIGHT:
         case GL_BACK_RIGHT:
         case GL_BACK_LEFT:
         case GL_FRONT_AND_BACK:
      return BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT
      case GL_LEFT:
         case GL_FRONT_LEFT:
         case GL_AUX0:
   case GL_AUX1:
   case GL_AUX2:
   case GL_AUX3:
         case GL_COLOR_ATTACHMENT0_EXT:
         case GL_COLOR_ATTACHMENT1_EXT:
         case GL_COLOR_ATTACHMENT2_EXT:
         case GL_COLOR_ATTACHMENT3_EXT:
         case GL_COLOR_ATTACHMENT4_EXT:
         case GL_COLOR_ATTACHMENT5_EXT:
         case GL_COLOR_ATTACHMENT6_EXT:
         case GL_COLOR_ATTACHMENT7_EXT:
         default:
      /* not an error, but also not supported */
   if (buffer >= GL_COLOR_ATTACHMENT8 && buffer <= GL_COLOR_ATTACHMENT31)
         /* error */
      }
         /**
   * Helper routine used by glReadBuffer.
   * Given a GLenum naming a color buffer, return the index of the corresponding
   * renderbuffer (a BUFFER_* value).
   * return BUFFER_NONE for an invalid buffer.
   */
   static gl_buffer_index
   read_buffer_enum_to_index(const struct gl_context *ctx, GLenum buffer)
   {
               switch (buffer) {
      case GL_FRONT:
         case GL_BACK:
         case GL_RIGHT:
         case GL_FRONT_RIGHT:
         case GL_BACK_RIGHT:
         case GL_BACK_LEFT:
         case GL_LEFT:
         case GL_FRONT_LEFT:
         case GL_FRONT_AND_BACK:
         case GL_AUX0:
   case GL_AUX1:
   case GL_AUX2:
   case GL_AUX3:
         case GL_COLOR_ATTACHMENT0_EXT:
         case GL_COLOR_ATTACHMENT1_EXT:
         case GL_COLOR_ATTACHMENT2_EXT:
         case GL_COLOR_ATTACHMENT3_EXT:
         case GL_COLOR_ATTACHMENT4_EXT:
         case GL_COLOR_ATTACHMENT5_EXT:
         case GL_COLOR_ATTACHMENT6_EXT:
         case GL_COLOR_ATTACHMENT7_EXT:
         default:
      /* not an error, but also not supported */
   if (buffer >= GL_COLOR_ATTACHMENT8 && buffer <= GL_COLOR_ATTACHMENT31)
         /* error */
      }
      static bool
   is_legal_es3_readbuffer_enum(GLenum buf)
   {
      return buf == GL_BACK || buf == GL_NONE ||
      }
      /**
   * Called by glDrawBuffer() and glNamedFramebufferDrawBuffer().
   * Specify which renderbuffer(s) to draw into for the first color output.
   * <buffer> can name zero, one, two or four renderbuffers!
   * \sa _mesa_DrawBuffers
   *
   * \param buffer  buffer token such as GL_LEFT or GL_FRONT_AND_BACK, etc.
   *
   * Note that the behaviour of this function depends on whether the
   * current ctx->DrawBuffer is a window-system framebuffer or a user-created
   * framebuffer object.
   *   In the former case, we update the per-context ctx->Color.DrawBuffer
   *   state var _and_ the FB's ColorDrawBuffer state.
   *   In the later case, we update the FB's ColorDrawBuffer state only.
   *
   * Furthermore, upon a MakeCurrent() or BindFramebuffer() call, if the
   * new FB is a window system FB, we need to re-update the FB's
   * ColorDrawBuffer state to match the context.  This is handled in
   * _mesa_update_framebuffer().
   *
   * See the GL_EXT_framebuffer_object spec for more info.
   */
   static ALWAYS_INLINE void
   draw_buffer(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
                        if (MESA_VERBOSE & VERBOSE_API) {
                  if (buffer == GL_NONE) {
         }
   else {
      const GLbitfield supportedMask
         destMask = draw_buffer_enum_to_bitmask(ctx, buffer);
   if (!no_error && destMask == BAD_MASK) {
      /* totally bogus buffer */
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid buffer %s)", caller,
            }
   destMask &= supportedMask;
   if (!no_error && destMask == 0x0) {
      /* none of the named color buffers exist! */
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid buffer %s)",
                        /* if we get here, there's no error so set new state */
   const GLenum16 buffer16 = buffer;
            /* Call device driver function only if fb is the bound draw buffer */
   if (fb == ctx->DrawBuffer) {
      if (_mesa_is_winsys_fbo(ctx->DrawBuffer))
         }
         static void
   draw_buffer_error(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
         }
         static void
   draw_buffer_no_error(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
         }
         void GLAPIENTRY
   _mesa_DrawBuffer_no_error(GLenum buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DrawBuffer(GLenum buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_NamedFramebufferDrawBuffer_no_error(GLuint framebuffer, GLenum buf)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
         } else {
                     }
         void GLAPIENTRY
   _mesa_FramebufferDrawBufferEXT(GLuint framebuffer, GLenum buf)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_dsa(ctx, framebuffer,
         if (!fb)
      }
   else
               }
         void GLAPIENTRY
   _mesa_NamedFramebufferDrawBuffer(GLuint framebuffer, GLenum buf)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_err(ctx, framebuffer,
         if (!fb)
      }
   else
               }
         /**
   * Called by glDrawBuffersARB() and glNamedFramebufferDrawBuffers() to specify
   * the destination color renderbuffers for N fragment program color outputs.
   * \sa _mesa_DrawBuffer
   * \param n  number of outputs
   * \param buffers  array [n] of renderbuffer names.  Unlike glDrawBuffer, the
   *                 names cannot specify more than one buffer.  For example,
   *                 GL_FRONT_AND_BACK is illegal. The only exception is GL_BACK
   *                 that is considered special and allowed as far as n is one
   *                 since 4.5.
   */
   static ALWAYS_INLINE void
   draw_buffers(struct gl_context *ctx, struct gl_framebuffer *fb, GLsizei n,
         {
      GLuint output;
   GLbitfield usedBufferMask, supportedMask;
                     if (!no_error) {
      /* Turns out n==0 is a valid input that should not produce an error.
   * The remaining code below correctly handles the n==0 case.
   *
   * From the OpenGL 3.0 specification, page 258:
   * "An INVALID_VALUE error is generated if n is greater than
   *  MAX_DRAW_BUFFERS."
   */
   if (n < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(n < 0)", caller);
               if (n > (GLsizei) ctx->Const.MaxDrawBuffers) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* From the ES 3.0 specification, page 180:
   * "If the GL is bound to the default framebuffer, then n must be 1
   *  and the constant must be BACK or NONE."
   * (same restriction applies with GL_EXT_draw_buffers specification)
   */
   if (_mesa_is_gles2(ctx) && _mesa_is_winsys_fbo(fb) &&
      (n != 1 || (buffers[0] != GL_NONE && buffers[0] != GL_BACK))) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid buffers)", caller);
                  supportedMask = supported_buffer_bitmask(ctx, fb);
            /* complicated error checking... */
   for (output = 0; output < n; output++) {
      if (!no_error) {
      /* From the OpenGL 4.5 specification, page 493 (page 515 of the PDF)
   * "An INVALID_ENUM error is generated if any value in bufs is FRONT,
   * LEFT, RIGHT, or FRONT_AND_BACK . This restriction applies to both
   * the default framebuffer and framebuffer objects, and exists because
   * these constants may themselves refer to multiple buffers, as shown
   * in table 17.4."
   *
   * From the OpenGL 4.5 specification, page 492 (page 514 of the PDF):
   * "If the default framebuffer is affected, then each of the constants
   * must be one of the values listed in table 17.6 or the special value
   * BACK. When BACK is used, n must be 1 and color values are written
   * into the left buffer for single-buffered contexts, or into the back
   * left buffer for double-buffered contexts."
   *
   * Note "special value BACK". GL_BACK also refers to multiple buffers,
   * but it is consider a special case here. This is a change on 4.5.
   * For OpenGL 4.x we check that behaviour. For any previous version we
   * keep considering it wrong (as INVALID_ENUM).
   */
   if (buffers[output] == GL_BACK &&
      _mesa_is_winsys_fbo(fb) &&
   _mesa_is_desktop_gl(ctx) &&
   ctx->Version >= 40) {
   if (n != 1) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(with GL_BACK n must be 1)",
               } else if (buffers[output] == GL_FRONT ||
            buffers[output] == GL_LEFT ||
   buffers[output] == GL_RIGHT ||
   buffers[output] == GL_FRONT_AND_BACK ||
   (buffers[output] == GL_BACK &&
   _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid buffer %s)",
                                 if (!no_error) {
      /* From the OpenGL 3.0 specification, page 258:
   * "Each buffer listed in bufs must be one of the values from tables
   *  4.5 or 4.6.  Otherwise, an INVALID_ENUM error is generated.
   */
   if (destMask[output] == BAD_MASK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid buffer %s)",
                     /* Section 4.2 (Whole Framebuffer Operations) of the OpenGL ES 3.0
   * specification says:
   *
   *     "If the GL is bound to a draw framebuffer object, the ith
   *     buffer listed in bufs must be COLOR_ATTACHMENTi or NONE .
   *     Specifying a buffer out of order, BACK , or COLOR_ATTACHMENTm
   *     where m is greater than or equal to the value of MAX_-
   *     COLOR_ATTACHMENTS , will generate the error INVALID_OPERATION .
   */
   if (_mesa_is_gles3(ctx) && _mesa_is_user_fbo(fb) &&
      buffers[output] != GL_NONE &&
   (buffers[output] < GL_COLOR_ATTACHMENT0 ||
   buffers[output] >= GL_COLOR_ATTACHMENT0 + ctx->Const.MaxColorAttachments)) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawBuffers(buffer)");
                  if (buffers[output] == GL_NONE) {
         }
   else {
      /* Page 259 (page 275 of the PDF) in section 4.2.1 of the OpenGL 3.0
   * spec (20080923) says:
   *
   *     "If the GL is bound to a framebuffer object and DrawBuffers is
   *     supplied with [...] COLOR_ATTACHMENTm where m is greater than
   *     or equal to the value of MAX_COLOR_ATTACHMENTS, then the error
   *     INVALID_OPERATION results."
   */
   if (!no_error && _mesa_is_user_fbo(fb) && buffers[output] >=
      GL_COLOR_ATTACHMENT0 + ctx->Const.MaxDrawBuffers) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* From the OpenGL 3.0 specification, page 259:
   * "If the GL is bound to the default framebuffer and DrawBuffers is
   *  supplied with a constant (other than NONE) that does not indicate
   *  any of the color buffers allocated to the GL context by the window
   *  system, the error INVALID_OPERATION will be generated.
   *
   *  If the GL is bound to a framebuffer object and DrawBuffers is
   *  supplied with a constant from table 4.6 [...] then the error
   *  INVALID_OPERATION results."
   */
   destMask[output] &= supportedMask;
   if (!no_error) {
      if (destMask[output] == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* ES 3.0 is even more restrictive.  From the ES 3.0 spec, page 180:
   * "If the GL is bound to a framebuffer object, the ith buffer
   * listed in bufs must be COLOR_ATTACHMENTi or NONE. [...]
   * INVALID_OPERATION." (same restriction applies with
   * GL_EXT_draw_buffers specification)
   */
   if (_mesa_is_gles2(ctx) && _mesa_is_user_fbo(fb) &&
      buffers[output] != GL_NONE &&
   buffers[output] != GL_COLOR_ATTACHMENT0 + output) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                           /* From the OpenGL 3.0 specification, page 258:
   * "Except for NONE, a buffer may not appear more than once in the
   * array pointed to by bufs.  Specifying a buffer more then once
   * will result in the error INVALID_OPERATION."
   */
   if (destMask[output] & usedBufferMask) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              /* update bitmask */
                  /* OK, if we get here, there were no errors so set the new state */
   GLenum16 buffers16[MAX_DRAW_BUFFERS];
   for (int i = 0; i < n; i++)
                     /*
   * Call device driver function if fb is the bound draw buffer.
   * Note that n can be equal to 0,
   * in which case we don't want to reference buffers[0], which
   * may not be valid.
   */
   if (fb == ctx->DrawBuffer) {
      if (_mesa_is_winsys_fbo(ctx->DrawBuffer))
         }
         static void
   draw_buffers_error(struct gl_context *ctx, struct gl_framebuffer *fb, GLsizei n,
         {
         }
         static void
   draw_buffers_no_error(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
         }
         void GLAPIENTRY
   _mesa_DrawBuffers_no_error(GLsizei n, const GLenum *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_DrawBuffers(GLsizei n, const GLenum *buffers)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
      void GLAPIENTRY
   _mesa_FramebufferDrawBuffersEXT(GLuint framebuffer, GLsizei n,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_dsa(ctx, framebuffer,
         if (!fb)
      }
   else
               }
      void GLAPIENTRY
   _mesa_NamedFramebufferDrawBuffers_no_error(GLuint framebuffer, GLsizei n,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
         } else {
                     }
         void GLAPIENTRY
   _mesa_NamedFramebufferDrawBuffers(GLuint framebuffer, GLsizei n,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_err(ctx, framebuffer,
         if (!fb)
      }
   else
               }
         /**
   * Performs necessary state updates when _mesa_drawbuffers makes an
   * actual change.
   */
   static void
   updated_drawbuffers(struct gl_context *ctx, struct gl_framebuffer *fb)
   {
               if (_mesa_is_desktop_gl_compat(ctx) && !ctx->Extensions.ARB_ES2_compatibility) {
      /* Flag the FBO as requiring validation. */
   fb->_Status = 0;
               }
         /**
   * Helper function to set the GL_DRAW_BUFFER state for the given context and
   * FBO.  Called via glDrawBuffer(), glDrawBuffersARB()
   *
   * All error checking will have been done prior to calling this function
   * so nothing should go wrong at this point.
   *
   * \param ctx  current context
   * \param fb   the desired draw buffer
   * \param n    number of color outputs to set
   * \param buffers  array[n] of colorbuffer names, like GL_LEFT.
   * \param destMask  array[n] of BUFFER_BIT_* bitmasks which correspond to the
   *                  colorbuffer names.  (i.e. GL_FRONT_AND_BACK =>
   *                  BUFFER_BIT_FRONT_LEFT | BUFFER_BIT_BACK_LEFT).
   */
   void
   _mesa_drawbuffers(struct gl_context *ctx, struct gl_framebuffer *fb,
               {
      GLbitfield mask[MAX_DRAW_BUFFERS];
            if (!destMask) {
      /* compute destMask values now */
   const GLbitfield supportedMask = supported_buffer_bitmask(ctx, fb);
   GLuint output;
   for (output = 0; output < n; output++) {
      mask[output] = draw_buffer_enum_to_bitmask(ctx, buffers[output]);
   assert(mask[output] != BAD_MASK);
      }
               /*
   * destMask[0] may have up to four bits set
   * (ex: glDrawBuffer(GL_FRONT_AND_BACK)).
   * Otherwise, destMask[x] can only have one bit set.
   */
   if (n > 0 && util_bitcount(destMask[0]) > 1) {
      GLuint count = 0, destMask0 = destMask[0];
   while (destMask0) {
      const gl_buffer_index bufIndex = u_bit_scan(&destMask0);
   if (fb->_ColorDrawBufferIndexes[count] != bufIndex) {
      updated_drawbuffers(ctx, fb);
      }
      }
   fb->ColorDrawBuffer[0] = buffers[0];
      }
   else {
      GLuint count = 0;
   for (buf = 0; buf < n; buf++ ) {
      if (destMask[buf]) {
      gl_buffer_index bufIndex = ffs(destMask[buf]) - 1;
   /* only one bit should be set in the destMask[buf] field */
   assert(util_bitcount(destMask[buf]) == 1);
   updated_drawbuffers(ctx, fb);
               }
      }
   else {
      updated_drawbuffers(ctx, fb);
                  }
      }
               /* set remaining outputs to BUFFER_NONE */
   for (buf = fb->_NumColorDrawBuffers; buf < ctx->Const.MaxDrawBuffers; buf++) {
      if (fb->_ColorDrawBufferIndexes[buf] != BUFFER_NONE) {
      updated_drawbuffers(ctx, fb);
         }
   for (buf = n; buf < ctx->Const.MaxDrawBuffers; buf++) {
                  if (_mesa_is_winsys_fbo(fb)) {
      /* also set context drawbuffer state */
   for (buf = 0; buf < ctx->Const.MaxDrawBuffers; buf++) {
      updated_drawbuffers(ctx, fb);
                        }
         /**
   * Update the current drawbuffer's _ColorDrawBufferIndex[] list, etc.
   * from the context's Color.DrawBuffer[] state.
   * Use when changing contexts.
   */
   void
   _mesa_update_draw_buffers(struct gl_context *ctx)
   {
      /* should be a window system FBO */
            _mesa_drawbuffers(ctx, ctx->DrawBuffer, ctx->Const.MaxDrawBuffers,
      }
         /**
   * Like \sa _mesa_drawbuffers(), this is a helper function for setting
   * GL_READ_BUFFER state for the given context and FBO.
   * Note that all error checking should have been done before calling
   * this function.
   * \param ctx  the rendering context
   * \param fb  the framebuffer object to update
   * \param buffer  GL_FRONT, GL_BACK, GL_COLOR_ATTACHMENT0, etc.
   * \param bufferIndex  the numerical index corresponding to 'buffer'
   */
   void
   _mesa_readbuffer(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
      if ((fb == ctx->ReadBuffer) && _mesa_is_winsys_fbo(fb)) {
      /* Only update the per-context READ_BUFFER state if we're bound to
   * a window-system framebuffer.
   */
               fb->ColorReadBuffer = buffer;
               }
            /**
   * Called by glReadBuffer and glNamedFramebufferReadBuffer to set the source
   * renderbuffer for reading pixels.
   * \param mode color buffer such as GL_FRONT, GL_BACK, etc.
   */
   static ALWAYS_INLINE void
   read_buffer(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
                        if (MESA_VERBOSE & VERBOSE_API)
            if (buffer == GL_NONE) {
      /* This is legal--it means that no buffer should be bound for reading. */
      }
   else {
      /* general case / window-system framebuffer */
   if (!no_error &&_mesa_is_gles3(ctx) &&
      !is_legal_es3_readbuffer_enum(buffer))
      else
            if (!no_error) {
               if (srcBuffer == BUFFER_NONE) {
      _mesa_error(ctx, GL_INVALID_ENUM,
                           supportedMask = supported_buffer_bitmask(ctx, fb);
   if (((1 << srcBuffer) & supportedMask) == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                                                   /* Call the device driver function only if fb is the bound read buffer */
   if (fb == ctx->ReadBuffer) {
      /* Check if we need to allocate a front color buffer.
   * Front buffers are often allocated on demand (other color buffers are
   * always allocated in advance).
   */
   if ((fb->_ColorReadBufferIndex == BUFFER_FRONT_LEFT ||
      fb->_ColorReadBufferIndex == BUFFER_FRONT_RIGHT) &&
   fb->Attachment[fb->_ColorReadBufferIndex].Type == GL_NONE) {
   assert(_mesa_is_winsys_fbo(fb));
   /* add the buffer */
   st_manager_add_color_renderbuffer(ctx, fb, fb->_ColorReadBufferIndex);
   _mesa_update_state(ctx);
            }
         static void
   read_buffer_err(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
         }
         static void
   read_buffer_no_error(struct gl_context *ctx, struct gl_framebuffer *fb,
         {
         }
         void GLAPIENTRY
   _mesa_ReadBuffer_no_error(GLenum buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_ReadBuffer(GLenum buffer)
   {
      GET_CURRENT_CONTEXT(ctx);
      }
         void GLAPIENTRY
   _mesa_NamedFramebufferReadBuffer_no_error(GLuint framebuffer, GLenum src)
   {
                        if (framebuffer) {
         } else {
                     }
         void GLAPIENTRY
   _mesa_FramebufferReadBufferEXT(GLuint framebuffer, GLenum buf)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_dsa(ctx, framebuffer,
         if (!fb)
      }
   else
               }
         void GLAPIENTRY
   _mesa_NamedFramebufferReadBuffer(GLuint framebuffer, GLenum src)
   {
      GET_CURRENT_CONTEXT(ctx);
            if (framebuffer) {
      fb = _mesa_lookup_framebuffer_err(ctx, framebuffer,
         if (!fb)
      }
   else
               }
