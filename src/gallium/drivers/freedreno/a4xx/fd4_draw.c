   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "fd4_context.h"
   #include "fd4_draw.h"
   #include "fd4_emit.h"
   #include "fd4_format.h"
   #include "fd4_program.h"
   #include "fd4_zsa.h"
      static void
   draw_impl(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      const struct pipe_draw_info *info = emit->info;
                     if (emit->dirty & (FD_DIRTY_VTXBUF | FD_DIRTY_VTXSTATE))
            OUT_PKT0(ring, REG_A4XX_VFD_INDEX_OFFSET, 2);
   OUT_RING(ring, info->index_size ? emit->draw->index_bias
                  OUT_PKT0(ring, REG_A4XX_PC_RESTART_INDEX, 1);
   OUT_RING(ring, info->primitive_restart ? /* PC_RESTART_INDEX */
                  /* points + psize -> spritelist: */
   if (ctx->rasterizer->point_size_per_vertex &&
      fd4_emit_get_vp(emit)->writes_psize && (info->mode == MESA_PRIM_POINTS))
         fd4_draw_emit(ctx->batch, ring, primtype,
            }
      static bool
   fd4_draw_vbo(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      struct fd4_context *fd4_ctx = fd4_context(ctx);
   struct fd4_emit emit = {
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
      .rasterflat = ctx->rasterizer->flatshade,
   .ucp_enables = ctx->rasterizer->clip_plane_enable,
   .has_per_samp = fd4_ctx->fastc_srgb || fd4_ctx->vastc_srgb,
   .vastc_srgb = fd4_ctx->vastc_srgb,
         },
   .rasterflat = ctx->rasterizer->flatshade,
   .sprite_coord_enable = ctx->rasterizer->sprite_coord_enable,
               /* Check if we actually need the tg4 workarounds */
   if (ir3_get_shader_info(emit.key.vs)->uses_texture_gather) {
      emit.key.key.has_per_samp = true;
   memcpy(emit.key.key.vsampler_swizzles, fd4_ctx->vsampler_swizzles,
      }
   if (ir3_get_shader_info(emit.key.fs)->uses_texture_gather) {
      emit.key.key.has_per_samp = true;
   memcpy(emit.key.key.fsampler_swizzles, fd4_ctx->fsampler_swizzles,
               if (info->mode != MESA_PRIM_COUNT && !indirect && !info->primitive_restart &&
      !u_trim_pipe_prim(info->mode, (unsigned *)&draw->count))
                           emit.prog = fd4_program_state(
            /* bail if compile failed: */
   if (!emit.prog)
                     const struct ir3_shader_variant *vp = fd4_emit_get_vp(&emit);
                              if (unlikely(ctx->stats_users > 0)) {
      ctx->stats.vs_regs += ir3_shader_halfregs(vp);
               emit.binning_pass = false;
                     if (ctx->rasterizer->rasterizer_discard) {
      fd_wfi(ctx->batch, ring);
   OUT_PKT3(ring, CP_REG_RMW, 3);
   OUT_RING(ring, REG_A4XX_RB_RENDER_CONTROL);
   OUT_RING(ring, ~A4XX_RB_RENDER_CONTROL_DISABLE_COLOR_PIPE);
                        if (ctx->rasterizer->rasterizer_discard) {
      fd_wfi(ctx->batch, ring);
   OUT_PKT3(ring, CP_REG_RMW, 3);
   OUT_RING(ring, REG_A4XX_RB_RENDER_CONTROL);
   OUT_RING(ring, ~A4XX_RB_RENDER_CONTROL_DISABLE_COLOR_PIPE);
               /* and now binning pass: */
   emit.binning_pass = true;
   emit.dirty = dirty & ~(FD_DIRTY_BLEND);
   emit.vs = NULL; /* we changed key so need to refetch vs */
   emit.fs = NULL;
                        }
      static void
   fd4_draw_vbos(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
         {
      for (unsigned i = 0; i < num_draws; i++)
      }
      void
   fd4_draw_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
      struct fd_context *ctx = fd_context(pctx);
      }
