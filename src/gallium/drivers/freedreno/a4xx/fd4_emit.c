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
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_viewport.h"
      #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
      #include "fd4_blend.h"
   #include "fd4_context.h"
   #include "fd4_emit.h"
   #include "fd4_format.h"
   #include "fd4_image.h"
   #include "fd4_program.h"
   #include "fd4_rasterizer.h"
   #include "fd4_texture.h"
   #include "fd4_zsa.h"
      #define emit_const_user fd4_emit_const_user
   #define emit_const_bo   fd4_emit_const_bo
   #include "ir3_const.h"
      /* regid:          base const register
   * prsc or dwords: buffer containing constant values
   * sizedwords:     size of const value buffer
   */
   static void
   fd4_emit_const_user(struct fd_ringbuffer *ring,
               {
               OUT_PKT3(ring, CP_LOAD_STATE4, 2 + sizedwords);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(regid / 4) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_EXT_SRC_ADDR(0) |
         for (int i = 0; i < sizedwords; i++)
      }
      static void
   fd4_emit_const_bo(struct fd_ringbuffer *ring,
               {
      uint32_t dst_off = regid / 4;
   assert(dst_off % 4 == 0);
   uint32_t num_unit = sizedwords / 4;
                     OUT_PKT3(ring, CP_LOAD_STATE4, 2);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(dst_off) |
                        }
      static void
   fd4_emit_const_ptrs(struct fd_ringbuffer *ring, gl_shader_stage type,
               {
      uint32_t anum = align(num, 4);
                     OUT_PKT3(ring, CP_LOAD_STATE4, 2 + anum);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(regid / 4) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_EXT_SRC_ADDR(0) |
            for (i = 0; i < num; i++) {
      if (bos[i]) {
         } else {
                     for (; i < anum; i++)
      }
      static bool
   is_stateobj(struct fd_ringbuffer *ring)
   {
         }
      static void
   emit_const_ptrs(struct fd_ringbuffer *ring, const struct ir3_shader_variant *v,
               {
      /* TODO inline this */
   assert(dst_offset + num <= v->constlen * 4);
      }
      void
   fd4_emit_cs_consts(const struct ir3_shader_variant *v,
               {
         }
      static void
   emit_textures(struct fd_context *ctx, struct fd_ringbuffer *ring,
               {
      static const uint32_t bcolor_reg[] = {
      [SB4_VS_TEX] = REG_A4XX_TPL1_TP_VS_BORDER_COLOR_BASE_ADDR,
   [SB4_FS_TEX] = REG_A4XX_TPL1_TP_FS_BORDER_COLOR_BASE_ADDR,
      };
   struct fd4_context *fd4_ctx = fd4_context(ctx);
   bool needs_border = false;
            if (tex->num_samplers > 0 || tex->num_textures > 0) {
               /* We want to always make sure that there's at least one sampler if
   * there are going to be texture accesses. Gallium might not upload a
   * sampler for e.g. buffer textures.
   */
   if (num_samplers == 0)
            /* not sure if this is an a420.0 workaround, but we seem
   * to need to emit these in pairs.. emit a final dummy
   * entry if odd # of samplers:
   */
            /* output sampler state: */
   OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (2 * num_samplers));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_SHADER) |
         for (i = 0; i < tex->num_samplers; i++) {
      static const struct fd4_sampler_stateobj dummy_sampler = {};
   const struct fd4_sampler_stateobj *sampler =
      tex->samplers[i] ? fd4_sampler_stateobj(tex->samplers[i])
                                 for (; i < num_samplers; i++) {
      OUT_RING(ring, 0x00000000);
                  if (tex->num_textures > 0) {
               /* emit texture state: */
   OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (8 * num_textures));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
         for (i = 0; i < tex->num_textures; i++) {
      static const struct fd4_pipe_sampler_view dummy_view = {};
   const struct fd4_pipe_sampler_view *view =
                  OUT_RING(ring, view->texconst0);
   OUT_RING(ring, view->texconst1);
   OUT_RING(ring, view->texconst2);
   OUT_RING(ring, view->texconst3);
   if (view->base.texture) {
      struct fd_resource *rsc = fd_resource(view->base.texture);
   if (view->base.format == PIPE_FORMAT_X32_S8X24_UINT)
            } else {
         }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
               for (i = 0; i < v->astc_srgb.count; i++) {
      static const struct fd4_pipe_sampler_view dummy_view = {};
                                          OUT_RING(ring, view->texconst0 & ~A4XX_TEX_CONST_0_SRGB);
   OUT_RING(ring, view->texconst1);
   OUT_RING(ring, view->texconst2);
   OUT_RING(ring, view->texconst3);
   if (view->base.texture) {
      struct fd_resource *rsc = fd_resource(view->base.texture);
      } else {
         }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
               for (i = 0; i < v->tg4.count; i++) {
      static const struct fd4_pipe_sampler_view dummy_view = {};
                                 unsigned texconst0 = view->texconst0 & ~(0xfff << 4);
   texconst0 |= A4XX_TEX_CONST_0_SWIZ_X(A4XX_TEX_X) |
      A4XX_TEX_CONST_0_SWIZ_Y(A4XX_TEX_Y) |
               /* Remap integer formats as unorm (will be fixed up in shader) */
   if (util_format_is_pure_integer(view->base.format)) {
      texconst0 &= ~A4XX_TEX_CONST_0_FMT__MASK;
   switch (fd4_pipe2tex(view->base.format)) {
   case TFMT4_8_8_8_8_UINT:
   case TFMT4_8_8_8_8_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_8_8_8_8_UNORM);
      case TFMT4_8_8_UINT:
   case TFMT4_8_8_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_8_8_UNORM);
      case TFMT4_8_UINT:
   case TFMT4_8_SINT:
                  case TFMT4_16_16_16_16_UINT:
   case TFMT4_16_16_16_16_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_16_16_16_16_UNORM);
      case TFMT4_16_16_UINT:
   case TFMT4_16_16_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_16_16_UNORM);
      case TFMT4_16_UINT:
   case TFMT4_16_SINT:
                  case TFMT4_32_32_32_32_UINT:
   case TFMT4_32_32_32_32_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_32_32_32_32_FLOAT);
      case TFMT4_32_32_UINT:
   case TFMT4_32_32_SINT:
      texconst0 |= A4XX_TEX_CONST_0_FMT(TFMT4_32_32_FLOAT);
      case TFMT4_32_UINT:
   case TFMT4_32_SINT:
                  case TFMT4_10_10_10_2_UINT:
                  default:
                     OUT_RING(ring, texconst0);
   OUT_RING(ring, view->texconst1);
   OUT_RING(ring, view->texconst2);
   OUT_RING(ring, view->texconst3);
   if (view->base.texture) {
      struct fd_resource *rsc = fd_resource(view->base.texture);
      } else {
         }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
         } else {
      assert(v->astc_srgb.count == 0);
               if (needs_border) {
      unsigned off;
            u_upload_alloc(fd4_ctx->border_color_uploader, 0,
                  fd_setup_border_colors(tex, ptr, 0);
   OUT_PKT0(ring, bcolor_reg[sb], 1);
                  }
      /* emit texture state for mem->gmem restore operation.. eventually it would
   * be good to get rid of this and use normal CSO/etc state for more of these
   * special cases..
   */
   void
   fd4_emit_gmem_restore_tex(struct fd_ringbuffer *ring, unsigned nr_bufs,
         {
      unsigned char mrt_comp[A4XX_MAX_RENDER_TARGETS];
            for (i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
                  /* output sampler state: */
   OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (2 * nr_bufs));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_SHADER) |
         for (i = 0; i < nr_bufs; i++) {
      OUT_RING(ring, A4XX_TEX_SAMP_0_XY_MAG(A4XX_TEX_NEAREST) |
                     A4XX_TEX_SAMP_0_XY_MIN(A4XX_TEX_NEAREST) |
               /* emit texture state: */
   OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (8 * nr_bufs));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
         for (i = 0; i < nr_bufs; i++) {
      if (bufs[i]) {
                     /* The restore blit_zs shader expects stencil in sampler 0,
   * and depth in sampler 1
   */
   if (rsc->stencil && (i == 0)) {
      rsc = rsc->stencil;
               /* note: PIPE_BUFFER disallowed for surfaces */
   unsigned lvl = bufs[i]->u.tex.level;
                  /* z32 restore is accomplished using depth write.  If there is
   * no stencil component (ie. PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
   * then no render target:
   *
   * (The same applies for z32_s8x24, since for stencil sampler
   * state the above 'if' will replace 'format' with s8)
   */
   if ((format == PIPE_FORMAT_Z32_FLOAT) ||
                           OUT_RING(ring, A4XX_TEX_CONST_0_FMT(fd4_pipe2tex(format)) |
                     OUT_RING(ring, A4XX_TEX_CONST_1_WIDTH(bufs[i]->width) |
         OUT_RING(ring, A4XX_TEX_CONST_2_PITCH(fd_resource_pitch(rsc, lvl)));
   OUT_RING(ring, 0x00000000);
   OUT_RELOC(ring, rsc->bo, offset, 0, 0);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
      } else {
      OUT_RING(ring, A4XX_TEX_CONST_0_FMT(0) |
                     A4XX_TEX_CONST_0_TYPE(A4XX_TEX_2D) |
   A4XX_TEX_CONST_0_SWIZ_X(A4XX_TEX_ONE) |
   OUT_RING(ring, A4XX_TEX_CONST_1_WIDTH(0) | A4XX_TEX_CONST_1_HEIGHT(0));
   OUT_RING(ring, A4XX_TEX_CONST_2_PITCH(0));
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
                  OUT_PKT0(ring, REG_A4XX_RB_RENDER_COMPONENTS, 1);
   OUT_RING(ring, A4XX_RB_RENDER_COMPONENTS_RT0(mrt_comp[0]) |
                     A4XX_RB_RENDER_COMPONENTS_RT1(mrt_comp[1]) |
   A4XX_RB_RENDER_COMPONENTS_RT2(mrt_comp[2]) |
   A4XX_RB_RENDER_COMPONENTS_RT3(mrt_comp[3]) |
      }
      static void
   emit_ssbos(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
               if (count == 0)
            OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (4 * count));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
         CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(sb) |
   OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(0) |
         for (unsigned i = 0; i < count; i++) {
      struct pipe_shader_buffer *buf = &so->sb[i];
   if (buf->buffer) {
      struct fd_resource *rsc = fd_resource(buf->buffer);
      } else {
         }
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
               OUT_PKT3(ring, CP_LOAD_STATE4, 2 + (2 * count));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
         CP_LOAD_STATE4_0_STATE_SRC(SS4_DIRECT) |
   CP_LOAD_STATE4_0_STATE_BLOCK(sb) |
   OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(1) |
         for (unsigned i = 0; i < count; i++) {
      struct pipe_shader_buffer *buf = &so->sb[i];
            /* width is in dwords, overflows into height: */
            OUT_RING(ring, A4XX_SSBO_1_0_WIDTH(sz));
         }
      void
   fd4_emit_vertex_bufs(struct fd_ringbuffer *ring, struct fd4_emit *emit)
   {
      int32_t i, j, last = -1;
   uint32_t total_in = 0;
   const struct fd_vertex_state *vtx = emit->vtx;
   const struct ir3_shader_variant *vp = fd4_emit_get_vp(emit);
   unsigned vertex_regid = regid(63, 0);
   unsigned instance_regid = regid(63, 0);
            /* Note that sysvals come *after* normal inputs: */
   for (i = 0; i < vp->inputs_count; i++) {
      if (!vp->inputs[i].compmask)
         if (vp->inputs[i].sysval) {
      switch (vp->inputs[i].slot) {
   case SYSTEM_VALUE_VERTEX_ID_ZERO_BASE:
      vertex_regid = vp->inputs[i].regid;
      case SYSTEM_VALUE_INSTANCE_ID:
      instance_regid = vp->inputs[i].regid;
      case SYSTEM_VALUE_VERTEX_CNT:
      vtxcnt_regid = vp->inputs[i].regid;
      default:
      unreachable("invalid system value");
         } else if (i < vtx->vtx->num_elements) {
                     for (i = 0, j = 0; i <= last; i++) {
      assert(!vp->inputs[i].sysval);
   if (vp->inputs[i].compmask) {
      struct pipe_vertex_element *elem = &vtx->vtx->pipe[i];
   const struct pipe_vertex_buffer *vb =
         struct fd_resource *rsc = fd_resource(vb->buffer.resource);
   enum pipe_format pfmt = elem->src_format;
   enum a4xx_vtx_fmt fmt = fd4_pipe2vtx(pfmt);
   bool switchnext = (i != last) || (vertex_regid != regid(63, 0)) ||
               bool isint = util_format_is_pure_integer(pfmt);
   uint32_t fs = util_format_get_blocksize(pfmt);
   uint32_t off = vb->buffer_offset + elem->src_offset;
                  OUT_PKT0(ring, REG_A4XX_VFD_FETCH(j), 4);
   OUT_RING(ring, A4XX_VFD_FETCH_INSTR_0_FETCHSIZE(fs - 1) |
                     A4XX_VFD_FETCH_INSTR_0_BUFSTRIDE(elem->src_stride) |
   OUT_RELOC(ring, rsc->bo, off, 0, 0);
   OUT_RING(ring, A4XX_VFD_FETCH_INSTR_2_SIZE(size));
                  OUT_PKT0(ring, REG_A4XX_VFD_DECODE_INSTR(j), 1);
   OUT_RING(ring,
            A4XX_VFD_DECODE_INSTR_CONSTFILL |
      A4XX_VFD_DECODE_INSTR_WRITEMASK(vp->inputs[i].compmask) |
   A4XX_VFD_DECODE_INSTR_FORMAT(fmt) |
   A4XX_VFD_DECODE_INSTR_SWAP(fd4_pipe2swap(pfmt)) |
   A4XX_VFD_DECODE_INSTR_REGID(vp->inputs[i].regid) |
                  total_in += util_bitcount(vp->inputs[i].compmask);
                  /* hw doesn't like to be configured for zero vbo's, it seems: */
   if (last < 0) {
      /* just recycle the shader bo, we just need to point to *something*
   * valid:
   */
   struct fd_bo *dummy_vbo = vp->bo;
   bool switchnext = (vertex_regid != regid(63, 0)) ||
                  OUT_PKT0(ring, REG_A4XX_VFD_FETCH(0), 4);
   OUT_RING(ring, A4XX_VFD_FETCH_INSTR_0_FETCHSIZE(0) |
               OUT_RELOC(ring, dummy_vbo, 0, 0, 0);
   OUT_RING(ring, A4XX_VFD_FETCH_INSTR_2_SIZE(1));
            OUT_PKT0(ring, REG_A4XX_VFD_DECODE_INSTR(0), 1);
   OUT_RING(ring, A4XX_VFD_DECODE_INSTR_CONSTFILL |
                     A4XX_VFD_DECODE_INSTR_WRITEMASK(0x1) |
   A4XX_VFD_DECODE_INSTR_FORMAT(VFMT4_8_UNORM) |
   A4XX_VFD_DECODE_INSTR_SWAP(XYZW) |
            total_in = 1;
               OUT_PKT0(ring, REG_A4XX_VFD_CONTROL_0, 5);
   OUT_RING(ring, A4XX_VFD_CONTROL_0_TOTALATTRTOVS(total_in) |
                     OUT_RING(ring, A4XX_VFD_CONTROL_1_MAXSTORAGE(129) | // XXX
               OUT_RING(ring, 0x00000000); /* XXX VFD_CONTROL_2 */
   OUT_RING(ring, A4XX_VFD_CONTROL_3_REGID_VTXCNT(vtxcnt_regid));
            /* cache invalidate, otherwise vertex fetch could see
   * stale vbo contents:
   */
   OUT_PKT0(ring, REG_A4XX_UCHE_INVALIDATE0, 2);
   OUT_RING(ring, 0x00000000);
      }
      void
   fd4_emit_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      const struct ir3_shader_variant *vp = fd4_emit_get_vp(emit);
   const struct ir3_shader_variant *fp = fd4_emit_get_fp(emit);
                     if ((dirty & FD_DIRTY_FRAMEBUFFER) && !emit->binning_pass) {
      struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
            for (unsigned i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
                  OUT_PKT0(ring, REG_A4XX_RB_RENDER_COMPONENTS, 1);
   OUT_RING(ring, A4XX_RB_RENDER_COMPONENTS_RT0(mrt_comp[0]) |
                     A4XX_RB_RENDER_COMPONENTS_RT1(mrt_comp[1]) |
   A4XX_RB_RENDER_COMPONENTS_RT2(mrt_comp[2]) |
   A4XX_RB_RENDER_COMPONENTS_RT3(mrt_comp[3]) |
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_FRAMEBUFFER)) {
      struct fd4_zsa_stateobj *zsa = fd4_zsa_stateobj(ctx->zsa);
   struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
            if (util_format_is_pure_integer(pipe_surface_format(pfb->cbufs[0])))
            OUT_PKT0(ring, REG_A4XX_RB_ALPHA_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_RB_STENCIL_CONTROL, 2);
   OUT_RING(ring, zsa->rb_stencil_control);
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF)) {
      struct fd4_zsa_stateobj *zsa = fd4_zsa_stateobj(ctx->zsa);
            OUT_PKT0(ring, REG_A4XX_RB_STENCILREFMASK, 2);
   OUT_RING(ring, zsa->rb_stencilrefmask |
         OUT_RING(ring, zsa->rb_stencilrefmask_bf |
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
      struct fd4_zsa_stateobj *zsa = fd4_zsa_stateobj(ctx->zsa);
   bool fragz = fp->no_earlyz || fp->has_kill || fp->writes_pos;
   bool latez = !fp->fs.early_fragment_tests && fragz;
            OUT_PKT0(ring, REG_A4XX_RB_DEPTH_CONTROL, 1);
   OUT_RING(ring, zsa->rb_depth_control |
                              /* maybe this register/bitfield needs a better name.. this
   * appears to be just disabling early-z
   */
   OUT_PKT0(ring, REG_A4XX_GRAS_ALPHA_CONTROL, 1);
   OUT_RING(ring, zsa->gras_alpha_control |
                           if (dirty & FD_DIRTY_RASTERIZER) {
      struct fd4_rasterizer_stateobj *rasterizer =
            OUT_PKT0(ring, REG_A4XX_GRAS_SU_MODE_CONTROL, 1);
   OUT_RING(ring, rasterizer->gras_su_mode_control |
            OUT_PKT0(ring, REG_A4XX_GRAS_SU_POINT_MINMAX, 2);
   OUT_RING(ring, rasterizer->gras_su_point_minmax);
            OUT_PKT0(ring, REG_A4XX_GRAS_SU_POLY_OFFSET_SCALE, 3);
   OUT_RING(ring, rasterizer->gras_su_poly_offset_scale);
   OUT_RING(ring, rasterizer->gras_su_poly_offset_offset);
            OUT_PKT0(ring, REG_A4XX_GRAS_CL_CLIP_CNTL, 1);
               /* NOTE: since primitive_restart is not actually part of any
   * state object, we need to make sure that we always emit
   * PRIM_VTX_CNTL.. either that or be more clever and detect
   * when it changes.
   */
   if (emit->info) {
      const struct pipe_draw_info *info = emit->info;
   struct fd4_rasterizer_stateobj *rast =
                  if (info->index_size && info->primitive_restart)
                     if (fp->total_in > 0) {
      uint32_t varout = align(fp->total_in, 16) / 16;
   if (varout > 1)
                     OUT_PKT0(ring, REG_A4XX_PC_PRIM_VTX_CNTL, 2);
   OUT_RING(ring, val);
               /* NOTE: scissor enabled bit is part of rasterizer state: */
   if (dirty & (FD_DIRTY_SCISSOR | FD_DIRTY_RASTERIZER)) {
               OUT_PKT0(ring, REG_A4XX_GRAS_SC_WINDOW_SCISSOR_BR, 2);
   OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_BR_X(scissor->maxx) |
         OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_TL_X(scissor->minx) |
            ctx->batch->max_scissor.minx =
         ctx->batch->max_scissor.miny =
         ctx->batch->max_scissor.maxx =
         ctx->batch->max_scissor.maxy =
               if (dirty & FD_DIRTY_VIEWPORT) {
                        OUT_PKT0(ring, REG_A4XX_GRAS_CL_VPORT_XOFFSET_0, 6);
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XOFFSET_0(vp->translate[0]));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XSCALE_0(vp->scale[0]));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YOFFSET_0(vp->translate[1]));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YSCALE_0(vp->scale[1]));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_ZOFFSET_0(vp->translate[2]));
               if (dirty &
      (FD_DIRTY_VIEWPORT | FD_DIRTY_RASTERIZER | FD_DIRTY_FRAMEBUFFER)) {
   float zmin, zmax;
   int depth = 24;
   if (ctx->batch->framebuffer.zsbuf) {
      depth = util_format_get_component_bits(
      pipe_surface_format(ctx->batch->framebuffer.zsbuf),
   }
   util_viewport_zmin_zmax(&ctx->viewport[0], ctx->rasterizer->clip_halfz,
            OUT_PKT0(ring, REG_A4XX_RB_VPORT_Z_CLAMP(0), 2);
   if (depth == 32) {
      OUT_RING(ring, fui(zmin));
      } else if (depth == 16) {
      OUT_RING(ring, (uint32_t)(zmin * 0xffff));
      } else {
      OUT_RING(ring, (uint32_t)(zmin * 0xffffff));
                  if (dirty & (FD_DIRTY_PROG | FD_DIRTY_FRAMEBUFFER)) {
      struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
   unsigned n = pfb->nr_cbufs;
   /* if we have depth/stencil, we need at least on MRT: */
   if (pfb->zsbuf)
                     if (!emit->skip_consts) { /* evil hack to deal sanely with clear path */
      ir3_emit_vs_consts(vp, ring, ctx, emit->info, emit->indirect, emit->draw);
   if (!emit->binning_pass)
               if ((dirty & FD_DIRTY_BLEND)) {
      struct fd4_blend_stateobj *blend = fd4_blend_stateobj(ctx->blend);
            for (i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
      enum pipe_format format =
         bool is_int = util_format_is_pure_integer(format);
                  if (is_int) {
      control &= A4XX_RB_MRT_CONTROL_COMPONENT_ENABLE__MASK;
               if (!has_alpha) {
                                 OUT_PKT0(ring, REG_A4XX_RB_MRT_BLEND_CONTROL(i), 1);
               OUT_PKT0(ring, REG_A4XX_RB_FS_OUTPUT, 1);
   OUT_RING(ring,
               if (dirty & FD_DIRTY_BLEND_COLOR) {
               OUT_PKT0(ring, REG_A4XX_RB_BLEND_RED, 8);
   OUT_RING(ring, A4XX_RB_BLEND_RED_FLOAT(bcolor->color[0]) |
               OUT_RING(ring, A4XX_RB_BLEND_RED_F32(bcolor->color[0]));
   OUT_RING(ring, A4XX_RB_BLEND_GREEN_FLOAT(bcolor->color[1]) |
               OUT_RING(ring, A4XX_RB_BLEND_GREEN_F32(bcolor->color[1]));
   OUT_RING(ring, A4XX_RB_BLEND_BLUE_FLOAT(bcolor->color[2]) |
               OUT_RING(ring, A4XX_RB_BLEND_BLUE_F32(bcolor->color[2]));
   OUT_RING(ring, A4XX_RB_BLEND_ALPHA_FLOAT(bcolor->color[3]) |
                           if (ctx->dirty_shader[PIPE_SHADER_VERTEX] & FD_DIRTY_SHADER_TEX)
            if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_TEX)
            if (!emit->binning_pass) {
      if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_SSBO)
            if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_IMAGE)
         }
      void
   fd4_emit_cs_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      enum fd_dirty_shader_state dirty = ctx->dirty_shader[PIPE_SHADER_COMPUTE];
   unsigned num_textures = ctx->tex[PIPE_SHADER_COMPUTE].num_textures +
      cp->astc_srgb.count +
         if (dirty & FD_DIRTY_SHADER_TEX) {
               OUT_PKT0(ring, REG_A4XX_TPL1_TP_TEX_COUNT, 1);
               OUT_PKT0(ring, REG_A4XX_TPL1_TP_FS_TEX_COUNT, 1);
   OUT_RING(ring, A4XX_TPL1_TP_FS_TEX_COUNT_CS(
            if (dirty & FD_DIRTY_SHADER_SSBO)
            if (dirty & FD_DIRTY_SHADER_IMAGE)
      }
      /* emit setup at begin of new cmdstream buffer (don't rely on previous
   * state, there could have been a context switch between ioctls):
   */
   void
   fd4_emit_restore(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
      struct fd_context *ctx = batch->ctx;
            OUT_PKT0(ring, REG_A4XX_RBBM_PERFCTR_CTL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_DEBUG_ECO_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_SP_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_TPL1_TP_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_0D01, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_0E42, 1);
            OUT_PKT0(ring, REG_A4XX_UCHE_CACHE_WAYS_VFD, 1);
            OUT_PKT0(ring, REG_A4XX_UCHE_CACHE_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_UCHE_INVALIDATE0, 2);
   OUT_RING(ring, 0x00000000);
            OUT_PKT0(ring, REG_A4XX_HLSQ_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_0CC5, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_0CC6, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_0EC2, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2001, 1);
            OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_20EF, 1);
            OUT_PKT0(ring, REG_A4XX_RB_BLEND_RED, 4);
   OUT_RING(ring, A4XX_RB_BLEND_RED_UINT(0) | A4XX_RB_BLEND_RED_FLOAT(0.0f));
   OUT_RING(ring, A4XX_RB_BLEND_GREEN_UINT(0) | A4XX_RB_BLEND_GREEN_FLOAT(0.0f));
   OUT_RING(ring, A4XX_RB_BLEND_BLUE_UINT(0) | A4XX_RB_BLEND_BLUE_FLOAT(0.0f));
   OUT_RING(ring,
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2152, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2153, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2154, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2155, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2156, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_2157, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_21C3, 1);
            OUT_PKT0(ring, REG_A4XX_PC_GS_PARAM, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_21E6, 1);
            OUT_PKT0(ring, REG_A4XX_PC_HS_PARAM, 1);
            OUT_PKT0(ring, REG_A4XX_UNKNOWN_22D7, 1);
            OUT_PKT0(ring, REG_A4XX_TPL1_TP_TEX_OFFSET, 1);
            OUT_PKT0(ring, REG_A4XX_TPL1_TP_TEX_COUNT, 1);
   OUT_RING(ring, A4XX_TPL1_TP_TEX_COUNT_VS(16) | A4XX_TPL1_TP_TEX_COUNT_HS(0) |
                  OUT_PKT0(ring, REG_A4XX_TPL1_TP_FS_TEX_COUNT, 1);
            /* we don't use this yet.. probably best to disable.. */
   OUT_PKT3(ring, CP_SET_DRAW_STATE, 2);
   OUT_RING(ring, CP_SET_DRAW_STATE__0_COUNT(0) |
                        OUT_PKT0(ring, REG_A4XX_SP_VS_PVT_MEM_PARAM, 2);
   OUT_RING(ring, 0x08000001);                    /* SP_VS_PVT_MEM_PARAM */
            OUT_PKT0(ring, REG_A4XX_SP_FS_PVT_MEM_PARAM, 2);
   OUT_RING(ring, 0x08000001);                    /* SP_FS_PVT_MEM_PARAM */
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                        OUT_PKT0(ring, REG_A4XX_RB_MSAA_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MSAA_CONTROL_DISABLE |
            OUT_PKT0(ring, REG_A4XX_GRAS_CL_GB_CLIP_ADJ, 1);
   OUT_RING(ring, A4XX_GRAS_CL_GB_CLIP_ADJ_HORZ(0) |
            OUT_PKT0(ring, REG_A4XX_RB_ALPHA_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_RB_FS_OUTPUT, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_ALPHA_CONTROL, 1);
               }
      static void
   fd4_mem_to_mem(struct fd_ringbuffer *ring, struct pipe_resource *dst,
               {
      struct fd_bo *src_bo = fd_resource(src)->bo;
   struct fd_bo *dst_bo = fd_resource(dst)->bo;
            for (i = 0; i < sizedwords; i++) {
      OUT_PKT3(ring, CP_MEM_TO_MEM, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RELOC(ring, dst_bo, dst_off, 0, 0);
            dst_off += 4;
         }
      void
   fd4_emit_init_screen(struct pipe_screen *pscreen)
   {
               screen->emit_ib = fd4_emit_ib;
      }
      void
   fd4_emit_init(struct pipe_context *pctx)
   {
   }
