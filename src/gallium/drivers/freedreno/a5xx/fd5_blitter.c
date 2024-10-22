   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "freedreno_blitter.h"
   #include "freedreno_resource.h"
      #include "fd5_blitter.h"
   #include "fd5_emit.h"
   #include "fd5_format.h"
      /* Make sure none of the requested dimensions extend beyond the size of the
   * resource.  Not entirely sure why this happens, but sometimes it does, and
   * w/ 2d blt doesn't have wrap modes like a sampler, so force those cases
   * back to u_blitter
   */
   static bool
   ok_dims(const struct pipe_resource *r, const struct pipe_box *b, int lvl)
   {
      return (b->x >= 0) && (b->x + b->width <= u_minify(r->width0, lvl)) &&
            }
      /* Not sure if format restrictions differ for src and dst, or if
   * they only matter when src fmt != dst fmt..  but there appear to
   * be *some* limitations so let's just start rejecting stuff that
   * piglit complains about
   */
   static bool
   ok_format(enum pipe_format fmt)
   {
      if (util_format_is_compressed(fmt))
            switch (fmt) {
   case PIPE_FORMAT_R10G10B10A2_SSCALED:
   case PIPE_FORMAT_R10G10B10A2_SNORM:
   case PIPE_FORMAT_B10G10R10A2_USCALED:
   case PIPE_FORMAT_B10G10R10A2_SSCALED:
   case PIPE_FORMAT_B10G10R10A2_SNORM:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_R10G10B10A2_USCALED:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
   case PIPE_FORMAT_B10G10R10A2_UINT:
   case PIPE_FORMAT_R10G10B10A2_UINT:
         default:
                  if (fd5_pipe2color(fmt) == RB5_NONE)
               }
      static bool
   can_do_blit(const struct pipe_blit_info *info)
   {
      /* I think we can do scaling, but not in z dimension since that would
   * require blending..
   */
   if (info->dst.box.depth != info->src.box.depth)
            if (!ok_format(info->dst.format))
            if (!ok_format(info->src.format))
            /* hw ignores {SRC,DST}_INFO.COLOR_SWAP if {SRC,DST}_INFO.TILE_MODE
   * is set (not linear).  We can kind of get around that when tiling/
   * untiling by setting both src and dst COLOR_SWAP=WZYX, but that
   * means the formats must match:
   */
   if ((fd_resource(info->dst.resource)->layout.tile_mode ||
      fd_resource(info->src.resource)->layout.tile_mode) &&
   info->dst.format != info->src.format)
         /* until we figure out a few more registers: */
   if ((info->dst.box.width != info->src.box.width) ||
      (info->dst.box.height != info->src.box.height))
         /* src box can be inverted, which we don't support.. dst box cannot: */
   if ((info->src.box.width < 0) || (info->src.box.height < 0))
            if (!ok_dims(info->src.resource, &info->src.box, info->src.level))
            if (!ok_dims(info->dst.resource, &info->dst.box, info->dst.level))
            assert(info->dst.box.width >= 0);
   assert(info->dst.box.height >= 0);
            if ((info->dst.resource->nr_samples > 1) ||
      (info->src.resource->nr_samples > 1))
         if (info->scissor_enable)
            if (info->window_rectangle_include)
            if (info->render_condition_enable)
            if (info->alpha_blend)
            if (info->filter != PIPE_TEX_FILTER_NEAREST)
            if (info->mask != util_format_get_mask(info->src.format))
            if (info->mask != util_format_get_mask(info->dst.format))
               }
      static void
   emit_setup(struct fd_ringbuffer *ring)
   {
      OUT_PKT4(ring, REG_A5XX_RB_RENDER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_2D_BLIT_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_2D_BLIT_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_2184, 1);
            OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_SP_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_CL_CNTL, 1);
      }
      /* buffers need to be handled specially since x/width can exceed the bounds
   * supported by hw.. if necessary decompose into (potentially) two 2D blits
   */
   static void
   emit_blit_buffer(struct fd_ringbuffer *ring, const struct pipe_blit_info *info)
   {
      const struct pipe_box *sbox = &info->src.box;
   const struct pipe_box *dbox = &info->dst.box;
   struct fd_resource *src, *dst;
            src = fd_resource(info->src.resource);
            assert(src->layout.cpp == 1);
   assert(dst->layout.cpp == 1);
   assert(info->src.resource->format == info->dst.resource->format);
   assert((sbox->y == 0) && (sbox->height == 1));
   assert((dbox->y == 0) && (dbox->height == 1));
   assert((sbox->z == 0) && (sbox->depth == 1));
   assert((dbox->z == 0) && (dbox->depth == 1));
   assert(sbox->width == dbox->width);
   assert(info->src.level == 0);
            /*
   * Buffers can have dimensions bigger than max width, remap into
   * multiple 1d blits to fit within max dimension
   *
   * Note that blob uses .ARRAY_PITCH=128 for blitting buffers, which
   * seems to prevent overfetch related faults.  Not quite sure what
   * the deal is there.
   *
   * Low 6 bits of SRC/DST addresses need to be zero (ie. address
   * aligned to 64) so we need to shift src/dst x1/x2 to make up the
   * difference.  On top of already splitting up the blit so width
   * isn't > 16k.
   *
   * We perhaps could do a bit better, if src and dst are aligned but
   * in the worst case this means we have to split the copy up into
   * 16k (0x4000) minus 64 (0x40).
            sshift = sbox->x & 0x3f;
            for (unsigned off = 0; off < sbox->width; off += (0x4000 - 0x40)) {
               soff = (sbox->x + off) & ~0x3f;
            w = MIN2(sbox->width - off, (0x4000 - 0x40));
            assert((soff + w) <= fd_bo_size(src->bo));
            OUT_PKT7(ring, CP_SET_RENDER_MODE, 1);
            /*
   * Emit source:
   */
   OUT_PKT4(ring, REG_A5XX_RB_2D_SRC_INFO, 9);
   OUT_RING(ring, A5XX_RB_2D_SRC_INFO_COLOR_FORMAT(RB5_R8_UNORM) |
               OUT_RELOC(ring, src->bo, soff, 0, 0); /* RB_2D_SRC_LO/HI */
   OUT_RING(ring, A5XX_RB_2D_SRC_SIZE_PITCH(p) |
         OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_GRAS_2D_SRC_INFO, 1);
   OUT_RING(ring, A5XX_GRAS_2D_SRC_INFO_COLOR_FORMAT(RB5_R8_UNORM) |
            /*
   * Emit destination:
   */
   OUT_PKT4(ring, REG_A5XX_RB_2D_DST_INFO, 9);
   OUT_RING(ring, A5XX_RB_2D_DST_INFO_COLOR_FORMAT(RB5_R8_UNORM) |
               OUT_RELOC(ring, dst->bo, doff, 0, 0); /* RB_2D_DST_LO/HI */
   OUT_RING(ring, A5XX_RB_2D_DST_SIZE_PITCH(p) |
         OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_GRAS_2D_DST_INFO, 1);
   OUT_RING(ring, A5XX_GRAS_2D_DST_INFO_COLOR_FORMAT(RB5_R8_UNORM) |
            /*
   * Blit command:
   */
   OUT_PKT7(ring, CP_BLIT, 5);
   OUT_RING(ring, CP_BLIT_0_OP(BLIT_OP_COPY));
   OUT_RING(ring, CP_BLIT_1_SRC_X1(sshift) | CP_BLIT_1_SRC_Y1(0));
   OUT_RING(ring, CP_BLIT_2_SRC_X2(sshift + w - 1) | CP_BLIT_2_SRC_Y2(0));
   OUT_RING(ring, CP_BLIT_3_DST_X1(dshift) | CP_BLIT_3_DST_Y1(0));
            OUT_PKT7(ring, CP_SET_RENDER_MODE, 1);
                  }
      static void
   emit_blit(struct fd_ringbuffer *ring, const struct pipe_blit_info *info)
   {
      const struct pipe_box *sbox = &info->src.box;
   const struct pipe_box *dbox = &info->dst.box;
   struct fd_resource *src, *dst;
   enum a5xx_color_fmt sfmt, dfmt;
   enum a5xx_tile_mode stile, dtile;
   enum a3xx_color_swap sswap, dswap;
   unsigned spitch, dpitch;
   unsigned sx1, sy1, sx2, sy2;
            src = fd_resource(info->src.resource);
            sfmt = fd5_pipe2color(info->src.format);
            stile = fd_resource_tile_mode(info->src.resource, info->src.level);
            sswap = fd5_pipe2swap(info->src.format);
            spitch = fd_resource_pitch(src, info->src.level);
            /* if dtile, then dswap ignored by hw, and likewise if stile then sswap
   * ignored by hw.. but in this case we have already rejected the blit
   * if src and dst formats differ, so juse use WZYX for both src and
   * dst swap mode (so we don't change component order)
   */
   if (stile || dtile) {
      assert(info->src.format == info->dst.format);
               sx1 = sbox->x;
   sy1 = sbox->y;
   sx2 = sbox->x + sbox->width - 1;
            dx1 = dbox->x;
   dy1 = dbox->y;
   dx2 = dbox->x + dbox->width - 1;
            uint32_t sarray_pitch = fd_resource_layer_stride(src, info->src.level);
            for (unsigned i = 0; i < info->dst.box.depth; i++) {
      unsigned soff = fd_resource_offset(src, info->src.level, sbox->z + i);
            assert((soff + (sbox->height * spitch)) <= fd_bo_size(src->bo));
            OUT_PKT7(ring, CP_SET_RENDER_MODE, 1);
            /*
   * Emit source:
   */
   OUT_PKT4(ring, REG_A5XX_RB_2D_SRC_INFO, 9);
   OUT_RING(ring, A5XX_RB_2D_SRC_INFO_COLOR_FORMAT(sfmt) |
               OUT_RELOC(ring, src->bo, soff, 0, 0); /* RB_2D_SRC_LO/HI */
   OUT_RING(ring, A5XX_RB_2D_SRC_SIZE_PITCH(spitch) |
         OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_GRAS_2D_SRC_INFO, 1);
   OUT_RING(ring, A5XX_GRAS_2D_SRC_INFO_COLOR_FORMAT(sfmt) |
                  /*
   * Emit destination:
   */
   OUT_PKT4(ring, REG_A5XX_RB_2D_DST_INFO, 9);
   OUT_RING(ring, A5XX_RB_2D_DST_INFO_COLOR_FORMAT(dfmt) |
               OUT_RELOC(ring, dst->bo, doff, 0, 0); /* RB_2D_DST_LO/HI */
   OUT_RING(ring, A5XX_RB_2D_DST_SIZE_PITCH(dpitch) |
         OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_GRAS_2D_DST_INFO, 1);
   OUT_RING(ring, A5XX_GRAS_2D_DST_INFO_COLOR_FORMAT(dfmt) |
                  /*
   * Blit command:
   */
   OUT_PKT7(ring, CP_BLIT, 5);
   OUT_RING(ring, CP_BLIT_0_OP(BLIT_OP_COPY));
   OUT_RING(ring, CP_BLIT_1_SRC_X1(sx1) | CP_BLIT_1_SRC_Y1(sy1));
   OUT_RING(ring, CP_BLIT_2_SRC_X2(sx2) | CP_BLIT_2_SRC_Y2(sy2));
   OUT_RING(ring, CP_BLIT_3_DST_X1(dx1) | CP_BLIT_3_DST_Y1(dy1));
            OUT_PKT7(ring, CP_SET_RENDER_MODE, 1);
         }
      bool
   fd5_blitter_blit(struct fd_context *ctx,
         {
               if (!can_do_blit(info)) {
                  struct fd_resource *src = fd_resource(info->src.resource);
                              fd_batch_resource_read(batch, src);
                                                if ((info->src.resource->target == PIPE_BUFFER) &&
      (info->dst.resource->target == PIPE_BUFFER)) {
   assert(fd_resource(info->src.resource)->layout.tile_mode == TILE5_LINEAR);
   assert(fd_resource(info->dst.resource)->layout.tile_mode == TILE5_LINEAR);
      } else {
      /* I don't *think* we need to handle blits between buffer <-> !buffer */
   assert(info->src.resource->target != PIPE_BUFFER);
   assert(info->dst.resource->target != PIPE_BUFFER);
                        fd_batch_flush(batch);
            /* Acc query state will have been dirtied by our fd_batch_update_queries, so
   * the ctx->batch may need to turn its queries back on.
   */
               }
      unsigned
   fd5_tile_mode(const struct pipe_resource *tmpl)
   {
      /* basically just has to be a format we can blit, so uploads/downloads
   * via linear staging buffer works:
   */
   if (ok_format(tmpl->format))
               }
