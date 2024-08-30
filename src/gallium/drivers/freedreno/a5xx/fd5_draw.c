   /*
   * Copyright (C) 2016 Rob Clark <robclark@freedesktop.org>
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
      #include "pipe/p_state.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_string.h"
      #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd5_context.h"
   #include "fd5_draw.h"
   #include "fd5_emit.h"
   #include "fd5_format.h"
   #include "fd5_program.h"
   #include "fd5_zsa.h"
      static void
   draw_impl(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      const struct pipe_draw_info *info = emit->info;
                     if (emit->dirty & (FD_DIRTY_VTXBUF | FD_DIRTY_VTXSTATE))
            OUT_PKT4(ring, REG_A5XX_VFD_INDEX_OFFSET, 2);
   OUT_RING(ring, info->index_size ? emit->draw->index_bias
                  OUT_PKT4(ring, REG_A5XX_PC_RESTART_INDEX, 1);
   OUT_RING(ring, info->primitive_restart ? /* PC_RESTART_INDEX */
                  fd5_emit_render_cntl(ctx, false, emit->binning_pass);
   fd5_draw_emit(ctx->batch, ring, primtype,
            }
      static bool
   fd5_draw_vbo(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      struct fd5_emit emit = {
      .debug = &ctx->debug,
   .vtx = &ctx->vtx,
   .info = info,
   .drawid_offset = drawid_offset,
   .indirect = indirect,
   .draw = draw,
   .key = {
      .vs = ctx->prog.vs,
   .fs = ctx->prog.fs,
   .key = {
         },
      },
   .rasterflat = ctx->rasterizer->flatshade,
   .sprite_coord_enable = ctx->rasterizer->sprite_coord_enable,
                                 emit.prog = fd5_program_state(
            /* bail if compile failed: */
   if (!emit.prog)
                     const struct ir3_shader_variant *vp = fd5_emit_get_vp(&emit);
                              if (unlikely(ctx->stats_users > 0)) {
      ctx->stats.vs_regs += ir3_shader_halfregs(vp);
               /* figure out whether we need to disable LRZ write for binning
   * pass using draw pass's fp:
   */
            emit.binning_pass = false;
                     /* and now binning pass: */
   emit.binning_pass = true;
   emit.dirty = dirty & ~(FD_DIRTY_BLEND);
   emit.vs = NULL; /* we changed key so need to refetch vp */
   emit.fs = NULL;
            if (emit.streamout_mask) {
               for (unsigned i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      if (emit.streamout_mask & (1 << i)) {
                                    }
      static void
   fd5_draw_vbos(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
         {
      for (unsigned i = 0; i < num_draws; i++)
      }
      static void
   fd5_clear_lrz(struct fd_batch *batch, struct fd_resource *zsbuf, double depth)
   {
      struct fd_ringbuffer *ring;
                              OUT_PKT4(ring, REG_A5XX_RB_CCU_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_UPDATE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_CNTL, 1);
   OUT_RING(ring,
            A5XX_GRAS_SU_CNTL_LINEHALFWIDTH(0.0f) |
         OUT_PKT4(ring, REG_A5XX_GRAS_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_CL_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_MRT_BUF_INFO(0), 5);
   OUT_RING(ring, A5XX_RB_MRT_BUF_INFO_COLOR_FORMAT(RB5_R16_UNORM) |
               OUT_RING(ring, A5XX_RB_MRT_PITCH(zsbuf->lrz_pitch * 2));
   OUT_RING(ring, A5XX_RB_MRT_ARRAY_PITCH(fd_bo_size(zsbuf->lrz)));
            OUT_PKT4(ring, REG_A5XX_RB_RENDER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_DEST_MSAA_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_BLIT_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_CLEAR_COLOR_DW0, 1);
            OUT_PKT4(ring, REG_A5XX_VSC_RESOLVE_CNTL, 2);
   OUT_RING(ring, A5XX_VSC_RESOLVE_CNTL_X(zsbuf->lrz_width) |
                  OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_1, 2);
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_1_X(0) | A5XX_RB_RESOLVE_CNTL_1_Y(0));
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_2_X(zsbuf->lrz_width - 1) |
               }
      static bool
   fd5_clear(struct fd_context *ctx, enum fd_buffer_mask buffers,
               {
      struct fd_ringbuffer *ring = ctx->batch->draw;
            if ((buffers & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) &&
      is_z32(pfb->zsbuf->format))
                  if (buffers & FD_BUFFER_COLOR) {
      for (int i = 0; i < pfb->nr_cbufs; i++) {
                                                      // XXX I think RB_CLEAR_COLOR_DWn wants to take into account SWAP??
   union pipe_color_union swapped;
   switch (fd5_pipe2swap(pfmt)) {
   case WZYX:
      swapped.ui[0] = color->ui[0];
   swapped.ui[1] = color->ui[1];
   swapped.ui[2] = color->ui[2];
   swapped.ui[3] = color->ui[3];
      case WXYZ:
      swapped.ui[2] = color->ui[0];
   swapped.ui[1] = color->ui[1];
   swapped.ui[0] = color->ui[2];
   swapped.ui[3] = color->ui[3];
      case ZYXW:
      swapped.ui[3] = color->ui[0];
   swapped.ui[0] = color->ui[1];
   swapped.ui[1] = color->ui[2];
   swapped.ui[2] = color->ui[3];
      case XYZW:
      swapped.ui[3] = color->ui[0];
   swapped.ui[2] = color->ui[1];
   swapped.ui[1] = color->ui[2];
   swapped.ui[0] = color->ui[3];
                                       OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
                  OUT_PKT4(ring, REG_A5XX_RB_CLEAR_COLOR_DW0, 4);
   OUT_RING(ring, uc.ui[0]); /* RB_CLEAR_COLOR_DW0 */
   OUT_RING(ring, uc.ui[1]); /* RB_CLEAR_COLOR_DW1 */
                                 if (pfb->zsbuf && (buffers & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))) {
      uint32_t clear = util_pack_z_stencil(pfb->zsbuf->format, depth, stencil);
            if (buffers & FD_BUFFER_DEPTH)
            if (buffers & FD_BUFFER_STENCIL)
            OUT_PKT4(ring, REG_A5XX_RB_BLIT_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
   OUT_RING(ring,
            OUT_PKT4(ring, REG_A5XX_RB_CLEAR_COLOR_DW0, 1);
                     if (pfb->zsbuf && (buffers & FD_BUFFER_DEPTH)) {
      struct fd_resource *zsbuf = fd_resource(pfb->zsbuf->texture);
   if (zsbuf->lrz) {
      zsbuf->lrz_valid = true;
                     /* disable fast clear to not interfere w/ gmem->mem, etc.. */
   OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
               }
      void
   fd5_draw_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->draw_vbos = fd5_draw_vbos;
      }
