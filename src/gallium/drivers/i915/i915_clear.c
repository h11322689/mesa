   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
      /* Authors:
   *    Brian Paul
   */
      #include "util/format/u_format.h"
   #include "util/u_pack_color.h"
   #include "i915_batch.h"
   #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_resource.h"
   #include "i915_screen.h"
   #include "i915_state.h"
      void
   i915_clear_emit(struct pipe_context *pipe, unsigned buffers,
                     {
      struct i915_context *i915 = i915_context(pipe);
   uint32_t clear_params, clear_color, clear_depth, clear_stencil,
         union util_color u_color;
   float f_depth = depth;
   struct i915_texture *cbuf_tex, *depth_tex;
            cbuf_tex = depth_tex = NULL;
   clear_params = 0;
            if (buffers & PIPE_CLEAR_COLOR) {
               clear_params |= CLEARPARAM_WRITE_COLOR;
            util_pack_color(color->f, cbuf->format, &u_color);
   if (util_format_get_blocksize(cbuf_tex->b.format) == 4) {
      clear_color = u_color.ui[0];
      } else {
      clear_color = (u_color.ui[0] & 0xffff) | (u_color.ui[0] << 16);
               /* correctly swizzle clear value */
   if (i915->current.fixup_swizzle)
         else
            } else
            clear_depth = clear_stencil = 0;
   if (buffers & PIPE_CLEAR_DEPTH) {
               clear_params |= CLEARPARAM_WRITE_DEPTH;
   depth_tex = i915_texture(zbuf->texture);
   packed_z_stencil =
            if (util_format_get_blocksize(depth_tex->b.format) == 4) {
      /* Avoid read-modify-write if there's no stencil. */
   if (buffers & PIPE_CLEAR_STENCIL ||
      depth_tex->b.format != PIPE_FORMAT_Z24_UNORM_S8_UINT) {
   clear_params |= CLEARPARAM_WRITE_STENCIL;
               clear_depth = packed_z_stencil & 0xffffff;
      } else {
      clear_depth = (packed_z_stencil & 0xffff) | (packed_z_stencil << 16);
         } else if (buffers & PIPE_CLEAR_STENCIL) {
               clear_params |= CLEARPARAM_WRITE_STENCIL;
   depth_tex = i915_texture(zbuf->texture);
            packed_z_stencil =
         depth_clear_bbp = 32;
               /* hw can't fastclear both depth and color if their bbp mismatch. */
   if (color_clear_bbp && depth_clear_bbp &&
      color_clear_bbp != depth_clear_bbp) {
   if (i915->hardware_dirty)
            if (!BEGIN_BATCH(1 + 2 * (7 + 7))) {
                                                   OUT_BATCH(_3DSTATE_CLEAR_PARAMETERS);
   OUT_BATCH(CLEARPARAM_WRITE_COLOR | CLEARPARAM_CLEAR_RECT);
   /* Used for zone init prim */
   OUT_BATCH(clear_color);
   OUT_BATCH(clear_depth);
   /* Used for clear rect prim */
   OUT_BATCH(clear_color8888);
   OUT_BATCH_F(f_depth);
            OUT_BATCH(_3DPRIMITIVE | PRIM3D_CLEAR_RECT | 5);
   OUT_BATCH_F(destx + width);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
            OUT_BATCH(_3DSTATE_CLEAR_PARAMETERS);
   OUT_BATCH((clear_params & ~CLEARPARAM_WRITE_COLOR) |
         /* Used for zone init prim */
   OUT_BATCH(clear_color);
   OUT_BATCH(clear_depth);
   /* Used for clear rect prim */
   OUT_BATCH(clear_color8888);
   OUT_BATCH_F(f_depth);
            OUT_BATCH(_3DPRIMITIVE | PRIM3D_CLEAR_RECT | 5);
   OUT_BATCH_F(destx + width);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
      } else {
      if (i915->hardware_dirty)
            if (!BEGIN_BATCH(1 + 7 + 7)) {
                                                   OUT_BATCH(_3DSTATE_CLEAR_PARAMETERS);
   OUT_BATCH(clear_params | CLEARPARAM_CLEAR_RECT);
   /* Used for zone init prim */
   OUT_BATCH(clear_color);
   OUT_BATCH(clear_depth);
   /* Used for clear rect prim */
   OUT_BATCH(clear_color8888);
   OUT_BATCH_F(f_depth);
            OUT_BATCH(_3DPRIMITIVE | PRIM3D_CLEAR_RECT | 5);
   OUT_BATCH_F(destx + width);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
               /* Flush after clear, its expected to be a costly operation.
   * This is not required, just a heuristic, but without the flush we'd need to
   * clobber the SCISSOR_ENABLE dynamic state. */
            i915->last_fired_vertices = i915->fired_vertices;
      }
      /**
   * Clear the given buffers to the specified values.
   * No masking, no scissor (clear entire buffer).
   */
   void
   i915_clear_blitter(struct pipe_context *pipe, unsigned buffers,
                     {
      struct pipe_framebuffer_state *framebuffer =
                  for (i = 0; i < framebuffer->nr_cbufs; i++) {
      if (buffers & (PIPE_CLEAR_COLOR0 << i)) {
               if (ps) {
      pipe->clear_render_target(pipe, ps, color, 0, 0, ps->width,
                     if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      struct pipe_surface *ps = framebuffer->zsbuf;
   pipe->clear_depth_stencil(pipe, ps, buffers & PIPE_CLEAR_DEPTHSTENCIL,
               }
      void
   i915_clear_render(struct pipe_context *pipe, unsigned buffers,
                     {
               if (i915->dirty)
            i915_clear_emit(pipe, buffers, color, depth, stencil, 0, 0,
      }
