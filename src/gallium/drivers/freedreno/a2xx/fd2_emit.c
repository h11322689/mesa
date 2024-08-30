   /*
   * Copyright (C) 2012-2013 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_resource.h"
      #include "fd2_blend.h"
   #include "fd2_context.h"
   #include "fd2_emit.h"
   #include "fd2_program.h"
   #include "fd2_rasterizer.h"
   #include "fd2_texture.h"
   #include "fd2_util.h"
   #include "fd2_zsa.h"
      /* NOTE: just define the position for const regs statically.. the blob
   * driver doesn't seem to change these dynamically, and I can't really
   * think of a good reason to so..
   */
   #define VS_CONST_BASE 0x20
   #define PS_CONST_BASE 0x120
      static void
   emit_constants(struct fd_ringbuffer *ring, uint32_t base,
               {
      uint32_t enabled_mask = constbuf->enabled_mask;
   uint32_t start_base = base;
            /* emit user constants: */
   while (enabled_mask) {
      unsigned index = ffs(enabled_mask) - 1;
   struct pipe_constant_buffer *cb = &constbuf->cb[index];
            // I expect that size should be a multiple of vec4's:
            /* hmm, sometimes we still seem to end up with consts bound,
   * even if shader isn't using them, which ends up overwriting
   * const reg's used for immediates.. this is a hack to work
   * around that:
   */
   if (shader && ((base - start_base) >= (shader->first_immediate * 4)))
                     if (cb->user_buffer) {
         } else {
      struct fd_resource *rsc = fd_resource(cb->buffer);
                        OUT_PKT3(ring, CP_SET_CONSTANT, size + 1);
   OUT_RING(ring, base);
   for (i = 0; i < size; i++)
            base += size;
               /* emit shader immediates: */
   if (shader) {
      for (i = 0; i < shader->num_immediates; i++) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, start_base + (4 * (shader->first_immediate + i)));
   OUT_RING(ring, shader->immediates[i].val[0]);
   OUT_RING(ring, shader->immediates[i].val[1]);
   OUT_RING(ring, shader->immediates[i].val[2]);
   OUT_RING(ring, shader->immediates[i].val[3]);
            }
      typedef uint32_t texmask;
      static texmask
   emit_texture(struct fd_ringbuffer *ring, struct fd_context *ctx,
         {
      unsigned const_idx = fd2_get_const_idx(ctx, tex, samp_id);
   static const struct fd2_sampler_stateobj dummy_sampler = {};
   static const struct fd2_pipe_sampler_view dummy_view = {};
   const struct fd2_sampler_stateobj *sampler;
   const struct fd2_pipe_sampler_view *view;
            if (emitted & (1 << const_idx))
            sampler = tex->samplers[samp_id]
               view = tex->textures[samp_id] ? fd2_pipe_sampler_view(tex->textures[samp_id])
                     OUT_PKT3(ring, CP_SET_CONSTANT, 7);
            OUT_RING(ring, sampler->tex0 | view->tex0);
   if (rsc)
         else
            OUT_RING(ring, view->tex2);
   OUT_RING(ring, sampler->tex3 | view->tex3);
            if (rsc && rsc->b.b.last_level)
         else
               }
      static void
   emit_textures(struct fd_ringbuffer *ring, struct fd_context *ctx)
   {
      struct fd_texture_stateobj *fragtex = &ctx->tex[PIPE_SHADER_FRAGMENT];
   struct fd_texture_stateobj *verttex = &ctx->tex[PIPE_SHADER_VERTEX];
   texmask emitted = 0;
            for (i = 0; i < verttex->num_samplers; i++)
      if (verttex->samplers[i])
         for (i = 0; i < fragtex->num_samplers; i++)
      if (fragtex->samplers[i])
   }
      void
   fd2_emit_vertex_bufs(struct fd_ringbuffer *ring, uint32_t val,
         {
               OUT_PKT3(ring, CP_SET_CONSTANT, 1 + (2 * n));
   OUT_RING(ring, (0x1 << 16) | (val & 0xffff));
   for (i = 0; i < n; i++) {
      struct fd_resource *rsc = fd_resource(vbufs[i].prsc);
   OUT_RELOC(ring, rsc->bo, vbufs[i].offset, 3, 0);
         }
      void
   fd2_emit_state_binning(struct fd_context *ctx,
         {
      struct fd2_blend_stateobj *blend = fd2_blend_stateobj(ctx->blend);
                     if (dirty & (FD_DIRTY_PROG | FD_DIRTY_VTXSTATE))
            if (dirty & (FD_DIRTY_PROG | FD_DIRTY_CONST)) {
      emit_constants(ring, VS_CONST_BASE * 4,
                     if (dirty & FD_DIRTY_VIEWPORT) {
               OUT_PKT3(ring, CP_SET_CONSTANT, 9);
   OUT_RING(ring, 0x00000184);
   OUT_RING(ring, fui(vp->translate[0]));
   OUT_RING(ring, fui(vp->translate[1]));
   OUT_RING(ring, fui(vp->translate[2]));
   OUT_RING(ring, fui(0.0f));
   OUT_RING(ring, fui(vp->scale[0]));
   OUT_RING(ring, fui(vp->scale[1]));
   OUT_RING(ring, fui(vp->scale[2]));
               /* not sure why this is needed */
   if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_FRAMEBUFFER)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_CONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_SC_MODE_CNTL));
      }
      void
   fd2_emit_state(struct fd_context *ctx, const enum fd_dirty_3d_state dirty)
   {
      struct fd2_blend_stateobj *blend = fd2_blend_stateobj(ctx->blend);
   struct fd2_zsa_stateobj *zsa = fd2_zsa_stateobj(ctx->zsa);
   struct fd2_shader_stateobj *fs = ctx->prog.fs;
            /* NOTE: we probably want to eventually refactor this so each state
   * object handles emitting it's own state..  although the mapping of
   * state to registers is not always orthogonal, sometimes a single
   * register contains bitfields coming from multiple state objects,
   * so not sure the best way to deal with that yet.
            if (dirty & FD_DIRTY_SAMPLE_MASK) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF | FD_DIRTY_PROG)) {
      struct pipe_stencil_ref *sr = &ctx->stencil_ref;
            if (fs->has_kill)
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 4);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
   OUT_RING(ring, zsa->rb_stencilrefmask_bf |
         OUT_RING(ring, zsa->rb_stencilrefmask |
                     if (ctx->rasterizer && dirty & FD_DIRTY_RASTERIZER) {
      struct fd2_rasterizer_stateobj *rasterizer =
         OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
   OUT_RING(ring, rasterizer->pa_cl_clip_cntl);
   OUT_RING(ring, rasterizer->pa_su_sc_mode_cntl |
            OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_POINT_SIZE));
   OUT_RING(ring, rasterizer->pa_su_point_size);
   OUT_RING(ring, rasterizer->pa_su_point_minmax);
   OUT_RING(ring, rasterizer->pa_su_line_cntl);
            OUT_PKT3(ring, CP_SET_CONSTANT, 6);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_VTX_CNTL));
   OUT_RING(ring, rasterizer->pa_su_vtx_cntl);
   OUT_RING(ring, fui(1.0f)); /* PA_CL_GB_VERT_CLIP_ADJ */
   OUT_RING(ring, fui(1.0f)); /* PA_CL_GB_VERT_DISC_ADJ */
   OUT_RING(ring, fui(1.0f)); /* PA_CL_GB_HORZ_CLIP_ADJ */
            if (rasterizer->base.offset_tri) {
      /* TODO: why multiply scale by 2 ? without it deqp test fails
   * deqp/piglit tests aren't very precise
   */
   OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_POLY_OFFSET_FRONT_SCALE));
   OUT_RING(ring,
         OUT_RING(ring, fui(rasterizer->base.offset_units));  /* FRONT_OFFSET */
   OUT_RING(ring,
                        /* NOTE: scissor enabled bit is part of rasterizer state: */
   if (dirty & (FD_DIRTY_SCISSOR | FD_DIRTY_RASTERIZER)) {
               OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
   OUT_RING(ring, xy2d(scissor->minx, /* PA_SC_WINDOW_SCISSOR_TL */
         OUT_RING(ring, xy2d(scissor->maxx, /* PA_SC_WINDOW_SCISSOR_BR */
            ctx->batch->max_scissor.minx =
         ctx->batch->max_scissor.miny =
         ctx->batch->max_scissor.maxx =
         ctx->batch->max_scissor.maxy =
               if (dirty & FD_DIRTY_VIEWPORT) {
               OUT_PKT3(ring, CP_SET_CONSTANT, 7);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
   OUT_RING(ring, fui(vp->scale[0]));     /* PA_CL_VPORT_XSCALE */
   OUT_RING(ring, fui(vp->translate[0])); /* PA_CL_VPORT_XOFFSET */
   OUT_RING(ring, fui(vp->scale[1]));     /* PA_CL_VPORT_YSCALE */
   OUT_RING(ring, fui(vp->translate[1])); /* PA_CL_VPORT_YOFFSET */
   OUT_RING(ring, fui(vp->scale[2]));     /* PA_CL_VPORT_ZSCALE */
            /* set viewport in C65/C66, for a20x hw binning and fragcoord.z */
   OUT_PKT3(ring, CP_SET_CONSTANT, 9);
            OUT_RING(ring, fui(vp->translate[0]));
   OUT_RING(ring, fui(vp->translate[1]));
   OUT_RING(ring, fui(vp->translate[2]));
            OUT_RING(ring, fui(vp->scale[0]));
   OUT_RING(ring, fui(vp->scale[1]));
   OUT_RING(ring, fui(vp->scale[2]));
               if (dirty & (FD_DIRTY_PROG | FD_DIRTY_VTXSTATE | FD_DIRTY_TEXSTATE))
            if (dirty & (FD_DIRTY_PROG | FD_DIRTY_CONST)) {
      emit_constants(ring, VS_CONST_BASE * 4,
               emit_constants(ring, PS_CONST_BASE * 4,
                     if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_ZSA)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLORCONTROL));
               if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_FRAMEBUFFER)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_CONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
               if (dirty & FD_DIRTY_BLEND_COLOR) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_RED));
   OUT_RING(ring, float_to_ubyte(ctx->blend_color.color[0]));
   OUT_RING(ring, float_to_ubyte(ctx->blend_color.color[1]));
   OUT_RING(ring, float_to_ubyte(ctx->blend_color.color[2]));
               if (dirty & (FD_DIRTY_TEX | FD_DIRTY_PROG))
      }
      /* emit per-context initialization:
   */
   void
   fd2_emit_restore(struct fd_context *ctx, struct fd_ringbuffer *ring)
   {
      if (is_a20x(ctx->screen)) {
      OUT_PKT0(ring, REG_A2XX_RB_BC_CONTROL, 1);
   OUT_RING(ring, A2XX_RB_BC_CONTROL_ACCUM_TIMEOUT_SELECT(3) |
                              /* not sure why this is required */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_VIZ_QUERY));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_OUT_DEALLOC_CNTL));
      } else {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
               /* enable perfcntrs */
   OUT_PKT0(ring, REG_A2XX_CP_PERFMON_CNTL, 1);
            /* note: perfcntrs don't work without the PM_OVERRIDE bit */
   OUT_PKT0(ring, REG_A2XX_RBBM_PM_OVERRIDE1, 2);
   OUT_RING(ring, 0xffffffff);
            OUT_PKT0(ring, REG_A2XX_TP0_CHICKEN, 1);
            OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_VS_CONST));
   OUT_RING(ring, A2XX_SQ_VS_CONST_BASE(VS_CONST_BASE) |
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_PS_CONST));
   OUT_RING(ring,
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
   OUT_RING(ring, 0xffffffff); /* VGT_MAX_VTX_INDX */
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_CONTEXT_MISC));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_INTERPOLATOR_CNTL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_CONFIG));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_LINE_CNTL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
            // XXX we change this dynamically for draw/clear.. vs gmem<->mem..
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_MODECONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_SAMPLE_POS));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_DEST_MASK));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_DEST_INFO));
   OUT_RING(ring, A2XX_RB_COPY_DEST_INFO_FORMAT(COLORX_4_4_4_4) |
                              OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_SQ_WRAPPING_0));
   OUT_RING(ring, 0x00000000); /* SQ_WRAPPING_0 */
            OUT_PKT3(ring, CP_SET_DRAW_INIT_FLAGS, 1);
            OUT_PKT3(ring, CP_WAIT_REG_EQ, 4);
   OUT_RING(ring, 0x000005d0);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x5f601000);
            OUT_PKT0(ring, REG_A2XX_SQ_INST_STORE_MANAGMENT, 1);
            OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
            OUT_PKT3(ring, CP_SET_SHADER_BASES, 1);
            /* not sure what this form of CP_SET_CONSTANT is.. */
   OUT_PKT3(ring, CP_SET_CONSTANT, 13);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x469c4000);
   OUT_RING(ring, 0x3f800000);
   OUT_RING(ring, 0x3f000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x40000000);
   OUT_RING(ring, 0x3f400000);
   OUT_RING(ring, 0x3ec00000);
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
   OUT_RING(ring,
                  OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_RED));
   OUT_RING(ring, 0x00000000); /* RB_BLEND_RED */
   OUT_RING(ring, 0x00000000); /* RB_BLEND_GREEN */
   OUT_RING(ring, 0x00000000); /* RB_BLEND_BLUE */
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VTE_CNTL));
   OUT_RING(ring, A2XX_PA_CL_VTE_CNTL_VTX_W0_FMT |
                     A2XX_PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
   A2XX_PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
      }
      void
   fd2_emit_init_screen(struct pipe_screen *pscreen)
   {
      struct fd_screen *screen = fd_screen(pscreen);
      }
      void
   fd2_emit_init(struct pipe_context *pctx)
   {
   }
