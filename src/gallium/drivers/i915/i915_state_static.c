   /**************************************************************************
   *
   * Copyright © 2010 Jakob Bornecrantz
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_resource.h"
   #include "i915_screen.h"
   #include "i915_state.h"
      /***********************************************************************
   * Update framebuffer state
   */
   static unsigned
   translate_format(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
         case PIPE_FORMAT_B5G6R5_UNORM:
         case PIPE_FORMAT_B5G5R5A1_UNORM:
         case PIPE_FORMAT_B4G4R4A4_UNORM:
         case PIPE_FORMAT_B10G10R10A2_UNORM:
         case PIPE_FORMAT_L8_UNORM:
   case PIPE_FORMAT_A8_UNORM:
   case PIPE_FORMAT_I8_UNORM:
         default:
      assert(0);
         }
      static unsigned
   translate_depth_format(enum pipe_format zformat)
   {
      switch (zformat) {
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
         case PIPE_FORMAT_Z16_UNORM:
         default:
      assert(0);
         }
      static void
   update_framebuffer(struct i915_context *i915)
   {
      struct pipe_surface *cbuf_surface = i915->framebuffer.cbufs[0];
   struct pipe_surface *depth_surface = i915->framebuffer.zsbuf;
   unsigned x, y;
   int layer;
            if (cbuf_surface) {
      struct i915_surface *surf = i915_surface(cbuf_surface);
   struct i915_texture *tex = i915_texture(cbuf_surface->texture);
            i915->current.cbuf_bo = tex->buffer;
                     x = tex->image_offset[cbuf_surface->u.tex.level][layer].nblocksx;
      } else {
      i915->current.cbuf_bo = NULL;
      }
            /* What happens if no zbuf??
   */
   if (depth_surface) {
      struct i915_surface *surf = i915_surface(depth_surface);
   struct i915_texture *tex = i915_texture(depth_surface->texture);
   unsigned offset = i915_texture_offset(tex, depth_surface->u.tex.level,
         assert(tex);
   if (offset != 0)
            i915->current.depth_bo = tex->buffer;
      } else
                  /* drawing rect calculations */
   draw_offset = x | (y << 16);
   draw_size = (i915->framebuffer.width - 1 + x) |
         if (i915->current.draw_offset != draw_offset) {
      i915->current.draw_offset = draw_offset;
   i915_set_flush_dirty(i915, I915_PIPELINE_FLUSH);
      }
   if (i915->current.draw_size != draw_size) {
      i915->current.draw_size = draw_size;
                        /* flush the cache in case we sample from the old renderbuffers */
      }
      struct i915_tracked_state i915_hw_framebuffer = {
            static void
   update_dst_buf_vars(struct i915_context *i915)
   {
      struct pipe_surface *cbuf_surface = i915->framebuffer.cbufs[0];
   struct pipe_surface *depth_surface = i915->framebuffer.zsbuf;
   uint32_t dst_buf_vars, cformat, zformat;
            if (cbuf_surface)
         else
                  if (depth_surface) {
      struct i915_texture *tex = i915_texture(depth_surface->texture);
                     if (is->is_i945 && tex->tiling != I915_TILE_NONE &&
      (i915->fs && !i915->fs->info.writes_z))
   } else
            dst_buf_vars = DSTORG_HORT_BIAS(0x8) | /* .5 */
                        if (i915->current.dst_buf_vars != dst_buf_vars) {
      if (early_z != (i915->current.dst_buf_vars & CLASSIC_EARLY_DEPTH))
            i915->current.dst_buf_vars = dst_buf_vars;
   i915->static_dirty |= I915_DST_VARS;
         }
      struct i915_tracked_state i915_hw_dst_buf_vars = {
      "dst buf vars", update_dst_buf_vars, I915_NEW_FRAMEBUFFER | I915_NEW_FS};
