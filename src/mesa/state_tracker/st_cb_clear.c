   /**************************************************************************
   * 
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   * Copyright 2009 VMware, Inc.  All Rights Reserved.
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
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   *   Brian Paul
   *   Michel Dänzer
   */
      #include "main/errors.h"
   #include "util/glheader.h"
   #include "main/accum.h"
   #include "main/formats.h"
   #include "main/framebuffer.h"
   #include "main/macros.h"
   #include "main/glformats.h"
   #include "program/prog_instruction.h"
   #include "st_context.h"
   #include "st_atom.h"
   #include "st_cb_bitmap.h"
   #include "st_cb_clear.h"
   #include "st_draw.h"
   #include "st_format.h"
   #include "st_nir.h"
   #include "st_program.h"
   #include "st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_shader_tokens.h"
   #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_simple_shaders.h"
      #include "cso_cache/cso_context.h"
         /**
   * Do per-context initialization for glClear.
   */
   void
   st_init_clear(struct st_context *st)
   {
               st->clear.raster.half_pixel_center = 1;
   st->clear.raster.bottom_edge_rule = 1;
   st->clear.raster.depth_clip_near = 1;
      }
         /**
   * Free per-context state for glClear.
   */
   void
   st_destroy_clear(struct st_context *st)
   {
      if (st->clear.fs) {
      st->pipe->delete_fs_state(st->pipe, st->clear.fs);
      }
   if (st->clear.vs) {
      st->pipe->delete_vs_state(st->pipe, st->clear.vs);
      }
   if (st->clear.vs_layered) {
      st->pipe->delete_vs_state(st->pipe, st->clear.vs_layered);
      }
   if (st->clear.gs_layered) {
      st->pipe->delete_gs_state(st->pipe, st->clear.gs_layered);
         }
         /**
   * Helper function to set the clear color fragment shader.
   */
   static void
   set_clearcolor_fs(struct st_context *st, union pipe_color_union *color)
   {
      struct pipe_constant_buffer cb = {
      .user_buffer = color->f,
      };
   st->pipe->set_constant_buffer(st->pipe, PIPE_SHADER_FRAGMENT, 0,
            if (!st->clear.fs) {
                     }
      static void *
   make_nir_clear_vertex_shader(struct st_context *st, bool layered)
   {
      const char *shader_name = layered ? "layered clear VS" : "clear VS";
   unsigned inputs[] = {
      VERT_ATTRIB_POS,
      };
   gl_varying_slot outputs[] = {
      VARYING_SLOT_POS,
               return st_nir_make_passthrough_shader(st, shader_name, MESA_SHADER_VERTEX,
            }
         /**
   * Helper function to set the vertex shader.
   */
   static inline void
   set_vertex_shader(struct st_context *st)
   {
      /* vertex shader - still required to provide the linkage between
   * fragment shader input semantics and vertex_element/buffers.
   */
   if (!st->clear.vs)
            cso_set_vertex_shader_handle(st->cso_context, st->clear.vs);
      }
         static void
   set_vertex_shader_layered(struct st_context *st)
   {
               if (!st->screen->get_param(st->screen, PIPE_CAP_VS_INSTANCEID)) {
      assert(!"Got layered clear, but VS instancing is unsupported");
   set_vertex_shader(st);
               if (!st->clear.vs_layered) {
      bool vs_layer =
         if (vs_layer) {
         } else {
      st->clear.vs_layered = util_make_layered_clear_helper_vertex_shader(pipe);
                  cso_set_vertex_shader_handle(st->cso_context, st->clear.vs_layered);
      }
         /**
   * Do glClear by drawing a quadrilateral.
   * The vertices of the quad will be computed from the
   * ctx->DrawBuffer->_X/Ymin/max fields.
   */
   static void
   clear_with_quad(struct gl_context *ctx, unsigned clear_buffers)
   {
      struct st_context *st = st_context(ctx);
   struct cso_context *cso = st->cso_context;
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat) fb->Width;
                     const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin / fb_width * 2.0f - 1.0f;
   const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax / fb_width * 2.0f - 1.0f;
   const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin / fb_height * 2.0f - 1.0f;
   const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax / fb_height * 2.0f - 1.0f;
            /*
   printf("%s %s%s%s %f,%f %f,%f\n", __func__,
   color ? "color, " : "",
   depth ? "depth, " : "",
   stencil ? "stencil" : "",
   x0, y0,
   x1, y1);
            cso_save_state(cso, (CSO_BIT_BLEND |
                        CSO_BIT_STENCIL_REF |
   CSO_BIT_DEPTH_STENCIL_ALPHA |
   CSO_BIT_RASTERIZER |
   CSO_BIT_SAMPLE_MASK |
   CSO_BIT_MIN_SAMPLES |
   CSO_BIT_VIEWPORT |
         /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
   memset(&blend, 0, sizeof(blend));
   if (clear_buffers & PIPE_CLEAR_COLOR) {
      int num_buffers = ctx->Extensions.EXT_draw_buffers2 ?
                                 for (i = 0; i < num_buffers; i++) {
                                 if (ctx->Color.DitherFlag)
      }
               /* depth_stencil state: always pass/set to ref value */
   {
      struct pipe_depth_stencil_alpha_state depth_stencil;
   memset(&depth_stencil, 0, sizeof(depth_stencil));
   if (clear_buffers & PIPE_CLEAR_DEPTH) {
      depth_stencil.depth_enabled = 1;
   depth_stencil.depth_writemask = 1;
               if (clear_buffers & PIPE_CLEAR_STENCIL) {
      struct pipe_stencil_ref stencil_ref;
   memset(&stencil_ref, 0, sizeof(stencil_ref));
   depth_stencil.stencil[0].enabled = 1;
   depth_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
   depth_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
   depth_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
   depth_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
   depth_stencil.stencil[0].valuemask = 0xff;
   depth_stencil.stencil[0].writemask = ctx->Stencil.WriteMask[0] & 0xff;
   stencil_ref.ref_value[0] = ctx->Stencil.Clear;
                           st->util_velems.count = 1;
            cso_set_stream_outputs(cso, 0, NULL, NULL);
   cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
   st->clear.raster.multisample = st->state.fb_num_samples > 1;
            /* viewport state: viewport matching window dims */
   cso_set_viewport_dims(st->cso_context, fb_width, fb_height,
            /* Set constant buffer */
   set_clearcolor_fs(st, (union pipe_color_union*)&ctx->Color.ClearColor);
   cso_set_tessctrl_shader_handle(cso, NULL);
            if (num_layers > 1)
         else
            /* draw quad matching scissor rect.
   *
   * Note: if we're only clearing depth/stencil we still setup vertices
   * with color, but they'll be ignored.
   *
   * We can't translate the clear color to the colorbuffer format,
   * because different colorbuffers may have different formats.
   */
   if (!st_draw_quad(st, x0, y0, x1, y1,
                     ctx->Depth.Clear * 2.0f - 1.0f,
                  /* Restore pipe state */
   cso_restore_state(cso, 0);
   ctx->Array.NewVertexElements = true;
   ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS |
      }
         /**
   * Return if the scissor must be enabled during the clear.
   */
   static inline GLboolean
   is_scissor_enabled(struct gl_context *ctx, struct gl_renderbuffer *rb)
   {
               return (ctx->Scissor.EnableFlags & 1) &&
         (scissor->X > 0 ||
   scissor->Y > 0 ||
      }
      /**
   * Return if window rectangles must be enabled during the clear.
   */
   static inline bool
   is_window_rectangle_enabled(struct gl_context *ctx)
   {
      if (ctx->DrawBuffer == ctx->WinSysDrawBuffer)
         return ctx->Scissor.NumWindowRects > 0 ||
      }
         /**
   * Return if all of the stencil bits are masked.
   */
   static inline GLboolean
   is_stencil_disabled(struct gl_context *ctx, struct gl_renderbuffer *rb)
   {
               assert(_mesa_get_format_bits(rb->Format, GL_STENCIL_BITS) > 0);
      }
         /**
   * Return if any of the stencil bits are masked.
   */
   static inline GLboolean
   is_stencil_masked(struct gl_context *ctx, struct gl_renderbuffer *rb)
   {
               assert(_mesa_get_format_bits(rb->Format, GL_STENCIL_BITS) > 0);
      }
      void
   st_Clear(struct gl_context *ctx, GLbitfield mask)
   {
      struct st_context *st = st_context(ctx);
   struct gl_renderbuffer *depthRb
         struct gl_renderbuffer *stencilRb
         GLbitfield quad_buffers = 0x0;
   GLbitfield clear_buffers = 0x0;
   bool have_scissor_buffers = false;
            st_flush_bitmap_cache(st);
            /* This makes sure the pipe has the latest scissor, etc values */
            if (mask & BUFFER_BITS_COLOR) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
               if (b != BUFFER_NONE && mask & (1 << b)) {
      struct gl_renderbuffer *rb
                                                                              bool scissor = is_scissor_enabled(ctx, rb);
   if ((scissor && !st->can_scissor_clear) ||
      is_window_rectangle_enabled(ctx) ||
   ((colormask & surf_colormask) != surf_colormask))
      else
                           if (mask & BUFFER_BIT_DEPTH) {
      if (depthRb->surface && ctx->Depth.Mask) {
      bool scissor = is_scissor_enabled(ctx, depthRb);
   if ((scissor && !st->can_scissor_clear) ||
      is_window_rectangle_enabled(ctx))
      else
               }
   if (mask & BUFFER_BIT_STENCIL) {
      if (stencilRb->surface && !is_stencil_disabled(ctx, stencilRb)) {
      bool scissor = is_scissor_enabled(ctx, stencilRb);
   if ((scissor && !st->can_scissor_clear) ||
      is_window_rectangle_enabled(ctx) ||
   is_stencil_masked(ctx, stencilRb))
      else
                        /* Always clear depth and stencil together.
   * This can only happen when the stencil writemask is not a full mask.
   */
   if (quad_buffers & PIPE_CLEAR_DEPTHSTENCIL &&
      clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) {
   quad_buffers |= clear_buffers & PIPE_CLEAR_DEPTHSTENCIL;
               /* Only use quad-based clearing for the renderbuffers which cannot
   * use pipe->clear. We want to always use pipe->clear for the other
   * renderbuffers, because it's likely to be faster.
   */
   if (clear_buffers) {
      const struct gl_scissor_rect *scissor = &ctx->Scissor.ScissorArray[0];
   struct pipe_scissor_state scissor_state = {
      .minx = MAX2(scissor->X, 0),
   .miny = MAX2(scissor->Y, 0),
                        /* Now invert Y if needed.
   * Gallium drivers use the convention Y=0=top for surfaces.
   */
   if (st->state.fb_orientation == Y_0_TOP) {
      const struct gl_framebuffer *fb = ctx->DrawBuffer;
   /* use intermediate variables to avoid uint underflow */
   GLint miny, maxy;
   miny = fb->Height - scissor_state.maxy;
   maxy = fb->Height - scissor_state.miny;
   scissor_state.miny = MAX2(miny, 0);
      }
   if (have_scissor_buffers) {
      const struct gl_framebuffer *fb = ctx->DrawBuffer;
   scissor_state.maxx = MIN2(scissor_state.maxx, fb->Width);
   scissor_state.maxy = MIN2(scissor_state.maxy, fb->Height);
   if (scissor_state.minx >= scissor_state.maxx ||
      scissor_state.miny >= scissor_state.maxy)
   }
   /* We can't translate the clear color to the colorbuffer format,
   * because different colorbuffers may have different formats.
   */
   st->pipe->clear(st->pipe, clear_buffers, have_scissor_buffers ? &scissor_state : NULL,
            }
   if (quad_buffers) {
         }
   if (mask & BUFFER_BIT_ACCUM)
      }
   