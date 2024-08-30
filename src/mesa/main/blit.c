   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (C) 1999-2013  VMware, Inc.  All Rights Reserved.
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
      /*
   * glBlitFramebuffer functions.
   */
      #include <stdbool.h>
   #include <stdio.h>
      #include "context.h"
   #include "enums.h"
   #include "blit.h"
   #include "fbobject.h"
   #include "framebuffer.h"
   #include "glformats.h"
   #include "image.h"
   #include "mtypes.h"
   #include "macros.h"
   #include "readpix.h"
   #include "renderbuffer.h"
   #include "state.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_manager.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_scissor.h"
   #include "state_tracker/st_texture.h"
   #include "state_tracker/st_util.h"
      /** Set this to 1 to debug/log glBlitFramebuffer() calls */
   #define DEBUG_BLIT 0
            static const struct gl_renderbuffer_attachment *
   find_attachment(const struct gl_framebuffer *fb,
         {
      GLuint i;
   for (i = 0; i < ARRAY_SIZE(fb->Attachment); i++) {
      if (fb->Attachment[i].Renderbuffer == rb)
      }
      }
         /**
   * \return true if two regions overlap, false otherwise
   */
   bool
   _mesa_regions_overlap(int srcX0, int srcY0,
                     {
      if (MAX2(srcX0, srcX1) <= MIN2(dstX0, dstX1))
            if (MAX2(dstX0, dstX1) <= MIN2(srcX0, srcX1))
            if (MAX2(srcY0, srcY1) <= MIN2(dstY0, dstY1))
            if (MAX2(dstY0, dstY1) <= MIN2(srcY0, srcY1))
               }
         /**
   * Helper function for checking if the datatypes of color buffers are
   * compatible for glBlitFramebuffer.  From the 3.1 spec, page 198:
   *
   * "GL_INVALID_OPERATION is generated if mask contains GL_COLOR_BUFFER_BIT
   *  and any of the following conditions hold:
   *   - The read buffer contains fixed-point or floating-point values and any
   *     draw buffer contains neither fixed-point nor floating-point values.
   *   - The read buffer contains unsigned integer values and any draw buffer
   *     does not contain unsigned integer values.
   *   - The read buffer contains signed integer values and any draw buffer
   *     does not contain signed integer values."
   */
   static GLboolean
   compatible_color_datatypes(mesa_format srcFormat, mesa_format dstFormat)
   {
      GLenum srcType = _mesa_get_format_datatype(srcFormat);
            if (srcType != GL_INT && srcType != GL_UNSIGNED_INT) {
      assert(srcType == GL_UNSIGNED_NORMALIZED ||
         srcType == GL_SIGNED_NORMALIZED ||
   /* Boil any of those types down to GL_FLOAT */
               if (dstType != GL_INT && dstType != GL_UNSIGNED_INT) {
      assert(dstType == GL_UNSIGNED_NORMALIZED ||
         dstType == GL_SIGNED_NORMALIZED ||
   /* Boil any of those types down to GL_FLOAT */
                  }
         static GLboolean
   compatible_resolve_formats(const struct gl_renderbuffer *readRb,
         {
               /* This checks whether the internal formats are compatible rather than the
   * Mesa format for two reasons:
   *
   * • Under some circumstances, the user may request e.g. two GL_RGBA8
   *   textures and get two entirely different Mesa formats like RGBA8888 and
   *   ARGB8888. Drivers behaving like that should be able to cope with
   *   non-matching formats by themselves, because it's not the user's fault.
   *
   * • Picking two different internal formats can end up with the same Mesa
   *   format. For example the driver might be simulating GL_RGB textures
   *   with GL_RGBA internally and in that case both internal formats would
   *   end up with RGBA8888.
   *
   * This function is used to generate a GL error according to the spec so in
   * both cases we want to be looking at the application-level format, which
   * is InternalFormat.
   *
   * Blits between linear and sRGB formats are also allowed.
   */
   readFormat = _mesa_get_nongeneric_internalformat(readRb->InternalFormat);
   drawFormat = _mesa_get_nongeneric_internalformat(drawRb->InternalFormat);
   readFormat = _mesa_get_linear_internalformat(readFormat);
            if (readFormat == drawFormat) {
                     }
         static GLboolean
   is_valid_blit_filter(const struct gl_context *ctx, GLenum filter)
   {
      switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
         case GL_SCALED_RESOLVE_FASTEST_EXT:
   case GL_SCALED_RESOLVE_NICEST_EXT:
         default:
            }
         static bool
   validate_color_buffer(struct gl_context *ctx, struct gl_framebuffer *readFb,
               {
      const GLuint numColorDrawBuffers = drawFb->_NumColorDrawBuffers;
   const struct gl_renderbuffer *colorReadRb = readFb->_ColorReadBuffer;
   const struct gl_renderbuffer *colorDrawRb = NULL;
            for (i = 0; i < numColorDrawBuffers; i++) {
      colorDrawRb = drawFb->_ColorDrawBuffers[i];
   if (!colorDrawRb)
            /* Page 193 (page 205 of the PDF) in section 4.3.2 of the OpenGL
   * ES 3.0.1 spec says:
   *
   *     "If the source and destination buffers are identical, an
   *     INVALID_OPERATION error is generated. Different mipmap levels of a
   *     texture, different layers of a three- dimensional texture or
   *     two-dimensional array texture, and different faces of a cube map
   *     texture do not constitute identical buffers."
   */
   if (_mesa_is_gles3(ctx) && (colorDrawRb == colorReadRb)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           if (!compatible_color_datatypes(colorReadRb->Format,
            _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* extra checks for multisample copies... */
   if (readFb->Visual.samples > 0 || drawFb->Visual.samples > 0) {
      /* color formats must match on GLES. This isn't checked on desktop GL
   * because the GL 4.4 spec was changed to allow it.  In the section
   * entitled “Changes in the released
   * Specification of July 22, 2013” it says:
   *
   * “Relax BlitFramebuffer in section 18.3.1 so that format conversion
   * can take place during multisample blits, since drivers already
   * allow this and some apps depend on it.”
   */
   if (_mesa_is_gles(ctx) &&
      !compatible_resolve_formats(colorReadRb, colorDrawRb)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                              if (filter != GL_NEAREST) {
      /* From EXT_framebuffer_multisample_blit_scaled specification:
   * "Calling BlitFramebuffer will result in an INVALID_OPERATION error if
   * filter is not NEAREST and read buffer contains integer data."
   */
   GLenum type = _mesa_get_format_datatype(colorReadRb->Format);
   if (type == GL_INT || type == GL_UNSIGNED_INT) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
               }
      }
         static bool
   validate_stencil_buffer(struct gl_context *ctx, struct gl_framebuffer *readFb,
         {
      struct gl_renderbuffer *readRb =
         struct gl_renderbuffer *drawRb =
                  if (_mesa_is_gles3(ctx) && (drawRb == readRb)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           if (_mesa_get_format_bits(readRb->Format, GL_STENCIL_BITS) !=
      _mesa_get_format_bits(drawRb->Format, GL_STENCIL_BITS)) {
   /* There is no need to check the stencil datatype here, because
   * there is only one: GL_UNSIGNED_INT.
   */
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     read_z_bits = _mesa_get_format_bits(readRb->Format, GL_DEPTH_BITS);
            /* If both buffers also have depth data, the depth formats must match
   * as well.  If one doesn't have depth, it's not blitted, so we should
   * ignore the depth format check.
   */
   if (read_z_bits > 0 && draw_z_bits > 0 &&
      (read_z_bits != draw_z_bits ||
   _mesa_get_format_datatype(readRb->Format) !=
   _mesa_get_format_datatype(drawRb->Format))) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
      }
         static bool
   validate_depth_buffer(struct gl_context *ctx, struct gl_framebuffer *readFb,
         {
      struct gl_renderbuffer *readRb =
         struct gl_renderbuffer *drawRb =
                  if (_mesa_is_gles3(ctx) && (drawRb == readRb)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                           if ((_mesa_get_format_bits(readRb->Format, GL_DEPTH_BITS) !=
      _mesa_get_format_bits(drawRb->Format, GL_DEPTH_BITS)) ||
   (_mesa_get_format_datatype(readRb->Format) !=
   _mesa_get_format_datatype(drawRb->Format))) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     read_s_bit = _mesa_get_format_bits(readRb->Format, GL_STENCIL_BITS);
            /* If both buffers also have stencil data, the stencil formats must match as
   * well.  If one doesn't have stencil, it's not blitted, so we should ignore
   * the stencil format check.
   */
   if (read_s_bit > 0 && draw_s_bit > 0 && read_s_bit != draw_s_bit) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
      }
         static void
   do_blit_framebuffer(struct gl_context *ctx,
                     struct gl_framebuffer *readFB,
   struct gl_framebuffer *drawFB,
   {
      struct st_context *st = st_context(ctx);
   const GLbitfield depthStencil = (GL_DEPTH_BUFFER_BIT |
         const uint pFilter = ((filter == GL_NEAREST)
               struct {
      GLint srcX0, srcY0, srcX1, srcY1;
      } clip;
                     /* Make sure bitmap rendering has landed in the framebuffers */
   st_flush_bitmap_cache(st);
            clip.srcX0 = srcX0;
   clip.srcY0 = srcY0;
   clip.srcX1 = srcX1;
   clip.srcY1 = srcY1;
   clip.dstX0 = dstX0;
   clip.dstY0 = dstY0;
   clip.dstX1 = dstX1;
            /* NOTE: If the src and dst dimensions don't match, we cannot simply adjust
   * the integer coordinates to account for clipping (or scissors) because that
   * would make us cut off fractional parts, affecting the result of the blit.
   *
   * XXX: This should depend on mask !
   */
   if (!_mesa_clip_blit(ctx, readFB, drawFB,
                     }
   memset(&blit, 0, sizeof(struct pipe_blit_info));
   blit.scissor_enable =
      (dstX0 != clip.dstX0) ||
   (dstY0 != clip.dstY0) ||
   (dstX1 != clip.dstX1) ||
         if (_mesa_fb_orientation(drawFB) == Y_0_TOP) {
      /* invert Y for dest */
   dstY0 = drawFB->Height - dstY0;
   dstY1 = drawFB->Height - dstY1;
   /* invert Y for clip */
   clip.dstY0 = drawFB->Height - clip.dstY0;
      }
   if (blit.scissor_enable) {
      blit.scissor.minx = MIN2(clip.dstX0, clip.dstX1);
   blit.scissor.miny = MIN2(clip.dstY0, clip.dstY1);
   blit.scissor.maxx = MAX2(clip.dstX0, clip.dstX1);
   #if 0
         debug_printf("scissor = (%i,%i)-(%i,%i)\n",
         #endif
               if (_mesa_fb_orientation(readFB) == Y_0_TOP) {
      /* invert Y for src */
   srcY0 = readFB->Height - srcY0;
               if (srcY0 > srcY1 && dstY0 > dstY1) {
      /* Both src and dst are upside down.  Swap Y to make it
   * right-side up to increase odds of using a fast path.
   * Recall that all Gallium raster coords have Y=0=top.
   */
   GLint tmp;
   tmp = srcY0;
   srcY0 = srcY1;
   srcY1 = tmp;
   tmp = dstY0;
   dstY0 = dstY1;
               blit.src.box.depth = 1;
            /* Destination dimensions have to be positive: */
   if (dstX0 < dstX1) {
      blit.dst.box.x = dstX0;
   blit.src.box.x = srcX0;
   blit.dst.box.width = dstX1 - dstX0;
      } else {
      blit.dst.box.x = dstX1;
   blit.src.box.x = srcX1;
   blit.dst.box.width = dstX0 - dstX1;
      }
   if (dstY0 < dstY1) {
      blit.dst.box.y = dstY0;
   blit.src.box.y = srcY0;
   blit.dst.box.height = dstY1 - dstY0;
      } else {
      blit.dst.box.y = dstY1;
   blit.src.box.y = srcY1;
   blit.dst.box.height = dstY0 - dstY1;
               if (drawFB != ctx->WinSysDrawBuffer)
            blit.filter = pFilter;
   blit.render_condition_enable = st->has_conditional_render;
            if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_renderbuffer_attachment *srcAtt =
                           if (srcAtt->Type == GL_TEXTURE) {
      /* Make sure that the st_texture_object->pt is the current storage for
   * our miplevel.  The finalize would happen at some point anyway, might
   * as well be now.
                           if (!srcObj || !srcObj->pt) {
                  blit.src.resource = srcObj->pt;
   blit.src.level = srcAtt->TextureLevel;
                  if (!ctx->Color.sRGBEnabled)
      }
   else {
                                                                     blit.src.resource = srcSurf->texture;
   blit.src.level = srcSurf->u.tex.level;
   blit.src.box.z = srcSurf->u.tex.first_layer;
               for (i = 0; i < drawFB->_NumColorDrawBuffers; i++) {
                                                   if (dstSurf) {
      blit.dst.resource = dstSurf->texture;
                        ctx->pipe->blit(ctx->pipe, &blit);
                        if (mask & depthStencil) {
               /* get src/dst depth surfaces */
   struct gl_renderbuffer *srcDepthRb =
         struct gl_renderbuffer *dstDepthRb =
         struct pipe_surface *dstDepthSurf =
            struct gl_renderbuffer *srcStencilRb =
         struct gl_renderbuffer *dstStencilRb =
         struct pipe_surface *dstStencilSurf =
            if (_mesa_has_depthstencil_combined(readFB) &&
      _mesa_has_depthstencil_combined(drawFB)) {
   blit.mask = 0;
   if (mask & GL_DEPTH_BUFFER_BIT)
                        blit.dst.resource = dstDepthSurf->texture;
   blit.dst.level = dstDepthSurf->u.tex.level;
                  blit.src.resource = srcDepthRb->texture;
   blit.src.level = srcDepthRb->surface->u.tex.level;
                     }
   else {
                                 blit.dst.resource = dstDepthSurf->texture;
   blit.dst.level = dstDepthSurf->u.tex.level;
                  blit.src.resource = srcDepthRb->texture;
   blit.src.level = srcDepthRb->surface->u.tex.level;
                                                blit.dst.resource = dstStencilSurf->texture;
   blit.dst.level = dstStencilSurf->u.tex.level;
                  blit.src.resource = srcStencilRb->texture;
   blit.src.level = srcStencilRb->surface->u.tex.level;
                              }
      static ALWAYS_INLINE void
   blit_framebuffer(struct gl_context *ctx,
                  struct gl_framebuffer *readFb, struct gl_framebuffer *drawFb,
      {
               if (!readFb || !drawFb) {
      /* This will normally never happen but someday we may want to
   * support MakeCurrent() with no drawables.
   */
               /* Update completeness status of readFb and drawFb. */
            /* Make sure drawFb has an initialized bounding box. */
            if (!no_error) {
      const GLbitfield legalMaskBits = (GL_COLOR_BUFFER_BIT |
                  /* check for complete framebuffers */
   if (drawFb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT ||
      readFb->_Status != GL_FRAMEBUFFER_COMPLETE_EXT) {
   _mesa_error(ctx, GL_INVALID_FRAMEBUFFER_OPERATION_EXT,
                     if (!is_valid_blit_filter(ctx, filter)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s(invalid filter %s)", func,
                     if ((filter == GL_SCALED_RESOLVE_FASTEST_EXT ||
      filter == GL_SCALED_RESOLVE_NICEST_EXT) &&
   (readFb->Visual.samples == 0 || drawFb->Visual.samples > 0)) {
   _mesa_error(ctx, GL_INVALID_OPERATION, "%s(%s: invalid samples)", func,
                     if (mask & ~legalMaskBits) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(invalid mask bits set)", func);
               /* depth/stencil must be blitted with nearest filtering */
   if ((mask & (GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT))
      && filter != GL_NEAREST) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (_mesa_is_gles3(ctx)) {
      /* Page 194 (page 206 of the PDF) in section 4.3.2 of the OpenGL ES
   * 3.0.1 spec says:
   *
   *     "If SAMPLE_BUFFERS for the draw framebuffer is greater than
   *     zero, an INVALID_OPERATION error is generated."
   */
   if (drawFb->Visual.samples > 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Page 194 (page 206 of the PDF) in section 4.3.2 of the OpenGL ES
   * 3.0.1 spec says:
   *
   *     "If SAMPLE_BUFFERS for the read framebuffer is greater than
   *     zero, no copy is performed and an INVALID_OPERATION error is
   *     generated if the formats of the read and draw framebuffers are
   *     not identical or if the source and destination rectangles are
   *     not defined with the same (X0, Y0) and (X1, Y1) bounds."
   *
   * The format check was made above because desktop OpenGL has the same
   * requirement.
   */
   if (readFb->Visual.samples > 0
      && (srcX0 != dstX0 || srcY0 != dstY0
         _mesa_error(ctx, GL_INVALID_OPERATION,
               } else {
      if (readFb->Visual.samples > 0 &&
      drawFb->Visual.samples > 0 &&
   readFb->Visual.samples != drawFb->Visual.samples) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* extra checks for multisample copies... */
   if ((readFb->Visual.samples > 0 || drawFb->Visual.samples > 0) &&
      (filter == GL_NEAREST || filter == GL_LINEAR)) {
   /* src and dest region sizes must be the same */
   if (abs(srcX1 - srcX0) != abs(dstX1 - dstX0) ||
      abs(srcY1 - srcY0) != abs(dstY1 - dstY0)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                              /* get color read/draw renderbuffers */
   if (mask & GL_COLOR_BUFFER_BIT) {
      const GLuint numColorDrawBuffers = drawFb->_NumColorDrawBuffers;
            /* From the EXT_framebuffer_object spec:
   *
   *     "If a buffer is specified in <mask> and does not exist in both
   *     the read and draw framebuffers, the corresponding bit is silently
   *     ignored."
   */
   if (!colorReadRb || numColorDrawBuffers == 0) {
         } else if (!no_error) {
      if (!validate_color_buffer(ctx, readFb, drawFb, filter, func))
                  if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_renderbuffer *readRb =
         struct gl_renderbuffer *drawRb =
            /* From the EXT_framebuffer_object spec:
   *
   *     "If a buffer is specified in <mask> and does not exist in both
   *     the read and draw framebuffers, the corresponding bit is silently
   *     ignored."
   */
   if ((readRb == NULL) || (drawRb == NULL)) {
         } else if (!no_error) {
      if (!validate_stencil_buffer(ctx, readFb, drawFb, func))
                  if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_renderbuffer *readRb =
         struct gl_renderbuffer *drawRb =
            /* From the EXT_framebuffer_object spec:
   *
   *     "If a buffer is specified in <mask> and does not exist in both
   *     the read and draw framebuffers, the corresponding bit is silently
   *     ignored."
   */
   if ((readRb == NULL) || (drawRb == NULL)) {
         } else if (!no_error) {
      if (!validate_depth_buffer(ctx, readFb, drawFb, func))
                  /* Debug code */
   if (DEBUG_BLIT) {
      const struct gl_renderbuffer *colorReadRb = readFb->_ColorReadBuffer;
   const struct gl_renderbuffer *colorDrawRb = NULL;
            printf("%s(%d, %d, %d, %d,  %d, %d, %d, %d,"
         " 0x%x, 0x%x)\n", func,
   srcX0, srcY0, srcX1, srcY1,
            if (colorReadRb) {
               att = find_attachment(readFb, colorReadRb);
   printf("  Src FBO %u  RB %u (%dx%d)  ",
         readFb->Name, colorReadRb->Name,
   if (att && att->Texture) {
      printf("Tex %u  tgt 0x%x  level %u  face %u",
         att->Texture->Name,
   att->Texture->Target,
                     /* Print all active color render buffers */
   for (i = 0; i < drawFb->_NumColorDrawBuffers; i++) {
      colorDrawRb = drawFb->_ColorDrawBuffers[i];
                  att = find_attachment(drawFb, colorDrawRb);
   printf("  Dst FBO %u  RB %u (%dx%d)  ",
         drawFb->Name, colorDrawRb->Name,
   if (att && att->Texture) {
      printf("Tex %u  tgt 0x%x  level %u  face %u",
         att->Texture->Name,
   att->Texture->Target,
      }
                     if (!mask ||
      (srcX1 - srcX0) == 0 || (srcY1 - srcY0) == 0 ||
   (dstX1 - dstX0) == 0 || (dstY1 - dstY0) == 0) {
               do_blit_framebuffer(ctx, readFb, drawFb,
                  }
         static void
   blit_framebuffer_err(struct gl_context *ctx,
                        struct gl_framebuffer *readFb,
      {
      /* We are wrapping the err variant of the always inlined
   * blit_framebuffer() to avoid inlining it in every caller.
   */
   blit_framebuffer(ctx, readFb, drawFb, srcX0, srcY0, srcX1, srcY1,
      }
         /**
   * Blit rectangular region, optionally from one framebuffer to another.
   *
   * Note, if the src buffer is multisampled and the dest is not, this is
   * when the samples must be resolved to a single color.
   */
   void GLAPIENTRY
   _mesa_BlitFramebuffer_no_error(GLint srcX0, GLint srcY0, GLint srcX1,
                     {
               blit_framebuffer(ctx, ctx->ReadBuffer, ctx->DrawBuffer,
                  }
         void GLAPIENTRY
   _mesa_BlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
               {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
               "glBlitFramebuffer(%d, %d, %d, %d, "
   " %d, %d, %d, %d, 0x%x, %s)\n",
         blit_framebuffer_err(ctx, ctx->ReadBuffer, ctx->DrawBuffer,
                  }
         static ALWAYS_INLINE void
   blit_named_framebuffer(struct gl_context *ctx,
                           {
               /*
   * According to PDF page 533 of the OpenGL 4.5 core spec (30.10.2014,
   * Section 18.3 Copying Pixels):
   *   "... if readFramebuffer or drawFramebuffer is zero (for
   *   BlitNamedFramebuffer), then the default read or draw framebuffer is
   *   used as the corresponding source or destination framebuffer,
   *   respectively."
   */
   if (readFramebuffer) {
      if (no_error) {
         } else {
      readFb = _mesa_lookup_framebuffer_err(ctx, readFramebuffer,
         if (!readFb)
         } else {
                  if (drawFramebuffer) {
      if (no_error) {
         } else {
      drawFb = _mesa_lookup_framebuffer_err(ctx, drawFramebuffer,
         if (!drawFb)
         } else {
                  blit_framebuffer(ctx, readFb, drawFb,
                  }
         void GLAPIENTRY
   _mesa_BlitNamedFramebuffer_no_error(GLuint readFramebuffer,
                                       {
               blit_named_framebuffer(ctx, readFramebuffer, drawFramebuffer,
                  }
         void GLAPIENTRY
   _mesa_BlitNamedFramebuffer(GLuint readFramebuffer, GLuint drawFramebuffer,
                     {
               if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx,
               "glBlitNamedFramebuffer(%u %u %d, %d, %d, %d, "
   " %d, %d, %d, %d, 0x%x, %s)\n",
   readFramebuffer, drawFramebuffer,
         blit_named_framebuffer(ctx, readFramebuffer, drawFramebuffer,
                  }
