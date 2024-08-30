   /*
   * Copyright (C) 2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_string.h"
      #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd3_context.h"
   #include "fd3_draw.h"
   #include "fd3_emit.h"
   #include "fd3_format.h"
   #include "fd3_program.h"
   #include "fd3_zsa.h"
      static inline uint32_t
   add_sat(uint32_t a, int32_t b)
   {
      int64_t ret = (uint64_t)a + (int64_t)b;
   if (ret > ~0U)
         if (ret < 0)
            }
      static void
   draw_impl(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      const struct pipe_draw_info *info = emit->info;
                     if (emit->dirty & (FD_DIRTY_VTXBUF | FD_DIRTY_VTXSTATE))
            OUT_PKT0(ring, REG_A3XX_PC_VERTEX_REUSE_BLOCK_CNTL, 1);
            OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
   OUT_RING(ring, info->index_bounds_valid
                     OUT_RING(ring, info->index_bounds_valid
                     OUT_RING(ring, info->start_instance); /* VFD_INSTANCEID_OFFSET */
   OUT_RING(ring, info->index_size ? emit->draw->index_bias
            OUT_PKT0(ring, REG_A3XX_PC_RESTART_INDEX, 1);
   OUT_RING(ring, info->primitive_restart ? /* PC_RESTART_INDEX */
                  /* points + psize -> spritelist: */
   if (ctx->rasterizer->point_size_per_vertex &&
      fd3_emit_get_vp(emit)->writes_psize && (info->mode == MESA_PRIM_POINTS))
         fd_draw_emit(ctx->batch, ring, primtype,
            }
      static bool
   fd3_draw_vbo(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      struct fd3_emit emit = {
      .debug = &ctx->debug,
   .vtx = &ctx->vtx,
   .info = info,
   .drawid_offset = drawid_offset,
   .indirect = indirect,
   .draw = draw,
   .key = {
      .vs = ctx->prog.vs,
      },
   .rasterflat = ctx->rasterizer->flatshade,
   .sprite_coord_enable = ctx->rasterizer->sprite_coord_enable,
               if (info->mode != MESA_PRIM_COUNT && !indirect && !info->primitive_restart &&
      !u_trim_pipe_prim(info->mode, (unsigned *)&draw->count))
         if (fd3_needs_manual_clipping(ir3_get_shader(ctx->prog.vs), ctx->rasterizer))
                              emit.prog = fd3_program_state(
            /* bail if compile failed: */
   if (!emit.prog)
                     const struct ir3_shader_variant *vp = fd3_emit_get_vp(&emit);
                              if (unlikely(ctx->stats_users > 0)) {
      ctx->stats.vs_regs += ir3_shader_halfregs(vp);
               emit.binning_pass = false;
   emit.dirty = dirty;
            /* and now binning pass: */
   emit.binning_pass = true;
   emit.dirty = dirty & ~(FD_DIRTY_BLEND);
   emit.vs = NULL; /* we changed key so need to refetch vs */
   emit.fs = NULL;
                                 }
      static void
   fd3_draw_vbos(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
         {
      for (unsigned i = 0; i < num_draws; i++)
      }
      void
   fd3_draw_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
      struct fd_context *ctx = fd_context(pctx);
      }
