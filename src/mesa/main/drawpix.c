   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
   #include "draw_validate.h"
   #include "bufferobj.h"
   #include "context.h"
   #include "enums.h"
   #include "feedback.h"
   #include "framebuffer.h"
   #include "image.h"
   #include "pbo.h"
   #include "pixel.h"
   #include "state.h"
   #include "glformats.h"
   #include "fbobject.h"
   #include "util/u_math.h"
   #include "util/rounding.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_cb_drawpixels.h"
      /*
   * Execute glDrawPixels
   */
   void GLAPIENTRY
   _mesa_DrawPixels( GLsizei width, GLsizei height,
         {
      GLenum err;
                     if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDrawPixels(%d, %d, %s, %s, %p) // to %s at %ld, %ld\n",
               width, height,
   _mesa_enum_to_string(format),
   _mesa_enum_to_string(type),
   pixels,
            if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glDrawPixels(width or height < 0)" );
               /* We're not using the current vertex program, and the driver may install
   * its own.  Note: this may dirty some state.
   */
                     if (ctx->NewState)
            if (!ctx->DrawPixValid) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels");
               /* GL 3.0 introduced a new restriction on glDrawPixels() over what was in
   * GL_EXT_texture_integer.  From section 3.7.4 ("Rasterization of Pixel
   * Rectangles) on page 151 of the GL 3.0 specification:
   *
   *     "If format contains integer components, as shown in table 3.6, an
   *      INVALID OPERATION error is generated."
   *
   * Since DrawPixels rendering would be merely undefined if not an error (due
   * to a lack of defined mapping from integer data to gl_Color fragment shader
   * input), NVIDIA's implementation also just returns this error despite
   * exposing GL_EXT_texture_integer, just return an error regardless.
   */
   if (_mesa_is_enum_format_integer(format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glDrawPixels(integer format)");
               err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "glDrawPixels(invalid format %s and/or type %s)",
                           /* do special format-related checks */
   switch (format) {
   case GL_STENCIL_INDEX:
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL_EXT:
      /* these buffers must exist */
   if (!_mesa_dest_buffer_exists(ctx, format)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
      case GL_COLOR_INDEX:
      if (ctx->PixelMaps.ItoR.Size == 0 ||
      ctx->PixelMaps.ItoG.Size == 0 ||
   ctx->PixelMaps.ItoB.Size == 0) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
      default:
      /* for color formats it's not an error if the destination color
   * buffer doesn't exist.
   */
               if (ctx->RasterDiscard) {
                  if (!ctx->Current.RasterPosValid) {
                  if (ctx->RenderMode == GL_RENDER) {
      if (width > 0 && height > 0) {
      /* Round, to satisfy conformance tests (matches SGI's OpenGL) */
                  if (ctx->Unpack.BufferObj) {
      /* unpack from PBO */
   if (!_mesa_validate_pbo_access(2, &ctx->Unpack, width, height,
            _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (_mesa_check_disallowed_mapping(ctx->Unpack.BufferObj)) {
      /* buffer is mapped - that's an error */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                        st_DrawPixels(ctx, x, y, width, height, format, type,
         }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      /* Feedback the current raster pos info */
   FLUSH_CURRENT( ctx, 0 );
   _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_DRAW_PIXEL_TOKEN );
   _mesa_feedback_vertex( ctx,
                  }
   else {
      assert(ctx->RenderMode == GL_SELECT);
            end:
               if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         void GLAPIENTRY
   _mesa_CopyPixels( GLint srcx, GLint srcy, GLsizei width, GLsizei height,
         {
                        if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
               "glCopyPixels(%d, %d, %d, %d, %s) // from %s to %s at %ld, %ld\n",
   srcx, srcy, width, height,
   _mesa_enum_to_string(type),
   _mesa_enum_to_string(ctx->ReadBuffer->ColorReadBuffer),
         if (width < 0 || height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glCopyPixels(width or height < 0)");
               /* Note: more detailed 'type' checking is done by the
   * _mesa_source/dest_buffer_exists() calls below.  That's where we
   * check if the stencil buffer exists, etc.
   */
   if (type != GL_COLOR &&
      type != GL_DEPTH &&
   type != GL_STENCIL &&
   type != GL_DEPTH_STENCIL &&
   type != GL_DEPTH_STENCIL_TO_RGBA_NV &&
   type != GL_DEPTH_STENCIL_TO_BGRA_NV) {
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyPixels(type=%s)",
                     /* Return GL_INVALID_ENUM if the relevant extension is not enabled */
   if ((type == GL_DEPTH_STENCIL_TO_RGBA_NV || type == GL_DEPTH_STENCIL_TO_BGRA_NV) &&
      !ctx->Extensions.NV_copy_depth_to_color) {
   _mesa_error(ctx, GL_INVALID_ENUM, "glCopyPixels(type=%s)",
                     /* We're not using the current vertex program, and the driver may install
   * it's own.  Note: this may dirty some state.
   */
                     if (ctx->NewState)
            if (!ctx->DrawPixValid) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glCopyPixels");
               /* Check read buffer's status (draw buffer was already checked) */
   if (ctx->ReadBuffer->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     if (_mesa_is_user_fbo(ctx->ReadBuffer) &&
      ctx->ReadBuffer->Visual.samples > 0) {
      "glCopyPixels(multisample FBO)");
                  if (!_mesa_source_buffer_exists(ctx, type) ||
      !_mesa_dest_buffer_exists(ctx, type)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (ctx->RasterDiscard) {
                  if (!ctx->Current.RasterPosValid || width == 0 || height == 0) {
                  if (ctx->RenderMode == GL_RENDER) {
      /* Round to satisfy conformance tests (matches SGI's OpenGL) */
   if (width > 0 && height > 0) {
      GLint destx = lroundf(ctx->Current.RasterPos[0]);
   GLint desty = lroundf(ctx->Current.RasterPos[1]);
   st_CopyPixels( ctx, srcx, srcy, width, height, destx, desty,
         }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT( ctx, 0 );
   _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_COPY_PIXEL_TOKEN );
   _mesa_feedback_vertex( ctx,
                  }
   else {
      assert(ctx->RenderMode == GL_SELECT);
            end:
               if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
         void
   _mesa_bitmap(struct gl_context *ctx, GLsizei width, GLsizei height,
               {
               if (width < 0 || height < 0) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glBitmap(width or height < 0)" );
               if (!ctx->Current.RasterPosValid) {
                           if (ctx->NewState)
            if (!ctx->DrawPixValid) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBitmap");
               if (ctx->RasterDiscard)
            if (ctx->RenderMode == GL_RENDER) {
      /* Truncate, to satisfy conformance tests (matches SGI's OpenGL). */
   if (width > 0 && height > 0) {
      const GLfloat epsilon = 0.0001F;
                  if (!tex && ctx->Unpack.BufferObj) {
      /* unpack from PBO */
   if (!_mesa_validate_pbo_access(2, &ctx->Unpack, width, height,
                  _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   if (_mesa_check_disallowed_mapping(ctx->Unpack.BufferObj)) {
      /* buffer is mapped - that's an error */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                              }
   else if (ctx->RenderMode == GL_FEEDBACK) {
      FLUSH_CURRENT(ctx, 0);
   _mesa_feedback_token( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
   _mesa_feedback_vertex( ctx,
                  }
   else {
      assert(ctx->RenderMode == GL_SELECT);
               /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;
            if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
            }
      void GLAPIENTRY
   _mesa_Bitmap(GLsizei width, GLsizei height,
               {
                  }
