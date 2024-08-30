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
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_viewport.h"
      #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
      #include "fd3_blend.h"
   #include "fd3_context.h"
   #include "fd3_emit.h"
   #include "fd3_format.h"
   #include "fd3_program.h"
   #include "fd3_rasterizer.h"
   #include "fd3_texture.h"
   #include "fd3_zsa.h"
      #define emit_const_user fd3_emit_const_user
   #define emit_const_bo   fd3_emit_const_bo
   #include "ir3_const.h"
      static const enum adreno_state_block sb[] = {
      [MESA_SHADER_VERTEX] = SB_VERT_SHADER,
      };
      /* regid:          base const register
   * prsc or dwords: buffer containing constant values
   * sizedwords:     size of const value buffer
   */
   static void
   fd3_emit_const_user(struct fd_ringbuffer *ring,
               {
               OUT_PKT3(ring, CP_LOAD_STATE, 2 + sizedwords);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(regid / 2) |
                     OUT_RING(ring, CP_LOAD_STATE_1_EXT_SRC_ADDR(0) |
         for (int i = 0; i < sizedwords; i++)
      }
      static void
   fd3_emit_const_bo(struct fd_ringbuffer *ring,
               {
      uint32_t dst_off = regid / 2;
   /* The blob driver aligns all const uploads dst_off to 64.  We've been
   * successfully aligning to 8 vec4s as const_upload_unit so far with no
   * ill effects.
   */
   assert(dst_off % 16 == 0);
   uint32_t num_unit = sizedwords / 2;
                     OUT_PKT3(ring, CP_LOAD_STATE, 2);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(dst_off) |
                        }
      static void
   fd3_emit_const_ptrs(struct fd_ringbuffer *ring, gl_shader_stage type,
               {
      uint32_t anum = align(num, 4);
                     OUT_PKT3(ring, CP_LOAD_STATE, 2 + anum);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(regid / 2) |
                     OUT_RING(ring, CP_LOAD_STATE_1_EXT_SRC_ADDR(0) |
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
      #define VERT_TEX_OFF 0
   #define FRAG_TEX_OFF 16
   #define BASETABLE_SZ A3XX_MAX_MIP_LEVELS
      static void
   emit_textures(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      static const unsigned tex_off[] = {
      [SB_VERT_TEX] = VERT_TEX_OFF,
      };
   static const enum adreno_state_block mipaddr[] = {
      [SB_VERT_TEX] = SB_VERT_MIPADDR,
      };
   static const uint32_t bcolor_reg[] = {
      [SB_VERT_TEX] = REG_A3XX_TPL1_TP_VS_BORDER_COLOR_BASE_ADDR,
      };
   struct fd3_context *fd3_ctx = fd3_context(ctx);
   bool needs_border = false;
            if (tex->num_samplers > 0) {
      /* output sampler state: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + (2 * tex->num_samplers));
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(tex_off[sb]) |
                     OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_SHADER) |
         for (i = 0; i < tex->num_samplers; i++) {
      static const struct fd3_sampler_stateobj dummy_sampler = {};
   const struct fd3_sampler_stateobj *sampler =
                                                if (tex->num_textures > 0) {
      /* emit texture state: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + (4 * tex->num_textures));
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(tex_off[sb]) |
                     OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
         for (i = 0; i < tex->num_textures; i++) {
      static const struct fd3_pipe_sampler_view dummy_view = {};
   const struct fd3_pipe_sampler_view *view =
      tex->textures[i] ? fd3_pipe_sampler_view(tex->textures[i])
      OUT_RING(ring, view->texconst0);
   OUT_RING(ring, view->texconst1);
   OUT_RING(ring,
                     /* emit mipaddrs: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + (BASETABLE_SZ * tex->num_textures));
   OUT_RING(ring,
            CP_LOAD_STATE_0_DST_OFF(BASETABLE_SZ * tex_off[sb]) |
      CP_LOAD_STATE_0_STATE_SRC(SS_DIRECT) |
   OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
         for (i = 0; i < tex->num_textures; i++) {
      static const struct fd3_pipe_sampler_view dummy_view = {
      .base.target = PIPE_TEXTURE_1D, /* anything !PIPE_BUFFER */
      };
   const struct fd3_pipe_sampler_view *view =
      tex->textures[i] ? fd3_pipe_sampler_view(tex->textures[i])
      struct fd_resource *rsc = fd_resource(view->base.texture);
   if (rsc && rsc->b.b.target == PIPE_BUFFER) {
      OUT_RELOC(ring, rsc->bo, view->base.u.buf.offset, 0, 0);
      } else {
                     for (j = 0; j < (end - start + 1); j++) {
      struct fdl_slice *slice = fd_resource_slice(rsc, j + start);
                  /* pad the remaining entries w/ null: */
   for (; j < BASETABLE_SZ; j++) {
                        if (needs_border) {
      unsigned off;
            u_upload_alloc(fd3_ctx->border_color_uploader, 0,
                           OUT_PKT0(ring, bcolor_reg[sb], 1);
                  }
      /* emit texture state for mem->gmem restore operation.. eventually it would
   * be good to get rid of this and use normal CSO/etc state for more of these
   * special cases, but for now the compiler is not sufficient..
   *
   * Also, for using normal state, not quite sure how to handle the special
   * case format (fd3_gmem_restore_format()) stuff for restoring depth/stencil.
   */
   void
   fd3_emit_gmem_restore_tex(struct fd_ringbuffer *ring,
         {
               /* output sampler state: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + 2 * bufs);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(FRAG_TEX_OFF) |
                     OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_SHADER) |
         for (i = 0; i < bufs; i++) {
      OUT_RING(ring, A3XX_TEX_SAMP_0_XY_MAG(A3XX_TEX_NEAREST) |
                     A3XX_TEX_SAMP_0_XY_MIN(A3XX_TEX_NEAREST) |
               /* emit texture state: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + 4 * bufs);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(FRAG_TEX_OFF) |
                     OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
         for (i = 0; i < bufs; i++) {
      if (!psurf[i]) {
      OUT_RING(ring, A3XX_TEX_CONST_0_TYPE(A3XX_TEX_2D) |
                     A3XX_TEX_CONST_0_SWIZ_X(A3XX_TEX_ONE) |
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, A3XX_TEX_CONST_2_INDX(BASETABLE_SZ * i));
   OUT_RING(ring, 0x00000000);
               struct fd_resource *rsc = fd_resource(psurf[i]->texture);
   enum pipe_format format = fd_gmem_restore_format(psurf[i]->format);
   /* The restore blit_zs shader expects stencil in sampler 0, and depth
   * in sampler 1
   */
   if (rsc->stencil && i == 0) {
      rsc = rsc->stencil;
               /* note: PIPE_BUFFER disallowed for surfaces */
                     OUT_RING(ring, A3XX_TEX_CONST_0_TILE_MODE(rsc->layout.tile_mode) |
                     A3XX_TEX_CONST_0_FMT(fd3_pipe2tex(format)) |
   OUT_RING(ring, A3XX_TEX_CONST_1_WIDTH(psurf[i]->width) |
         OUT_RING(ring, A3XX_TEX_CONST_2_PITCH(fd_resource_pitch(rsc, lvl)) |
                     /* emit mipaddrs: */
   OUT_PKT3(ring, CP_LOAD_STATE, 2 + BASETABLE_SZ * bufs);
   OUT_RING(ring, CP_LOAD_STATE_0_DST_OFF(BASETABLE_SZ * FRAG_TEX_OFF) |
                     OUT_RING(ring, CP_LOAD_STATE_1_STATE_TYPE(ST_CONSTANTS) |
         for (i = 0; i < bufs; i++) {
      if (psurf[i]) {
      struct fd_resource *rsc = fd_resource(psurf[i]->texture);
   /* Matches above logic for blit_zs shader */
   if (rsc->stencil && i == 0)
         unsigned lvl = psurf[i]->u.tex.level;
   uint32_t offset =
            } else {
                  /* pad the remaining entries w/ null: */
   for (j = 1; j < BASETABLE_SZ; j++) {
               }
      void
   fd3_emit_vertex_bufs(struct fd_ringbuffer *ring, struct fd3_emit *emit)
   {
      int32_t i, j, last = -1;
   uint32_t total_in = 0;
   const struct fd_vertex_state *vtx = emit->vtx;
   const struct ir3_shader_variant *vp = fd3_emit_get_vp(emit);
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
   enum a3xx_vtx_fmt fmt = fd3_pipe2vtx(pfmt);
   bool switchnext = (i != last) || (vertex_regid != regid(63, 0)) ||
               bool isint = util_format_is_pure_integer(pfmt);
                           OUT_PKT0(ring, REG_A3XX_VFD_FETCH(j), 2);
   OUT_RING(ring, A3XX_VFD_FETCH_INSTR_0_FETCHSIZE(fs - 1) |
                     A3XX_VFD_FETCH_INSTR_0_BUFSTRIDE(elem->src_stride) |
   COND(switchnext, A3XX_VFD_FETCH_INSTR_0_SWITCHNEXT) |
   A3XX_VFD_FETCH_INSTR_0_INDEXCODE(j) |
                  OUT_PKT0(ring, REG_A3XX_VFD_DECODE_INSTR(j), 1);
   OUT_RING(ring,
            A3XX_VFD_DECODE_INSTR_CONSTFILL |
      A3XX_VFD_DECODE_INSTR_WRITEMASK(vp->inputs[i].compmask) |
   A3XX_VFD_DECODE_INSTR_FORMAT(fmt) |
   A3XX_VFD_DECODE_INSTR_SWAP(fd3_pipe2swap(pfmt)) |
   A3XX_VFD_DECODE_INSTR_REGID(vp->inputs[i].regid) |
                  total_in += util_bitcount(vp->inputs[i].compmask);
                  /* hw doesn't like to be configured for zero vbo's, it seems: */
   if (last < 0) {
      /* just recycle the shader bo, we just need to point to *something*
   * valid:
   */
   struct fd_bo *dummy_vbo = vp->bo;
   bool switchnext = (vertex_regid != regid(63, 0)) ||
                  OUT_PKT0(ring, REG_A3XX_VFD_FETCH(0), 2);
   OUT_RING(ring, A3XX_VFD_FETCH_INSTR_0_FETCHSIZE(0) |
                     A3XX_VFD_FETCH_INSTR_0_BUFSTRIDE(0) |
            OUT_PKT0(ring, REG_A3XX_VFD_DECODE_INSTR(0), 1);
   OUT_RING(ring, A3XX_VFD_DECODE_INSTR_CONSTFILL |
                     A3XX_VFD_DECODE_INSTR_WRITEMASK(0x1) |
   A3XX_VFD_DECODE_INSTR_FORMAT(VFMT_8_UNORM) |
   A3XX_VFD_DECODE_INSTR_SWAP(XYZW) |
            total_in = 1;
               OUT_PKT0(ring, REG_A3XX_VFD_CONTROL_0, 2);
   OUT_RING(ring, A3XX_VFD_CONTROL_0_TOTALATTRTOVS(total_in) |
                     OUT_RING(ring, A3XX_VFD_CONTROL_1_MAXSTORAGE(1) | // XXX
                  OUT_PKT0(ring, REG_A3XX_VFD_VS_THREADING_THRESHOLD, 1);
   OUT_RING(ring,
            }
      void
   fd3_emit_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      const struct ir3_shader_variant *vp = fd3_emit_get_vp(emit);
   const struct ir3_shader_variant *fp = fd3_emit_get_fp(emit);
                     if (dirty & FD_DIRTY_SAMPLE_MASK) {
      OUT_PKT0(ring, REG_A3XX_RB_MSAA_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MSAA_CONTROL_DISABLE |
                     if ((dirty & (FD_DIRTY_ZSA | FD_DIRTY_RASTERIZER | FD_DIRTY_PROG |
            !emit->binning_pass) {
   uint32_t val = fd3_zsa_stateobj(ctx->zsa)->rb_render_control |
            val |= COND(fp->frag_face, A3XX_RB_RENDER_CONTROL_FACENESS);
   val |= COND(fp->fragcoord_compmask != 0,
         val |= COND(ctx->rasterizer->rasterizer_discard,
            /* I suppose if we needed to (which I don't *think* we need
   * to), we could emit this for binning pass too.  But we
   * would need to keep a different patch-list for binning
   * vs render pass.
            OUT_PKT0(ring, REG_A3XX_RB_RENDER_CONTROL, 1);
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF)) {
      struct fd3_zsa_stateobj *zsa = fd3_zsa_stateobj(ctx->zsa);
            OUT_PKT0(ring, REG_A3XX_RB_ALPHA_REF, 1);
            OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
   OUT_RING(ring, zsa->rb_stencilrefmask |
         OUT_RING(ring, zsa->rb_stencilrefmask_bf |
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
      uint32_t val = fd3_zsa_stateobj(ctx->zsa)->rb_depth_control;
   if (fp->writes_pos) {
      val |= A3XX_RB_DEPTH_CONTROL_FRAG_WRITES_Z;
      }
   if (fp->no_earlyz || fp->has_kill) {
         }
   if (!ctx->rasterizer->depth_clip_near) {
         }
   OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
               if (dirty & FD_DIRTY_RASTERIZER) {
      struct fd3_rasterizer_stateobj *rasterizer =
            OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SU_POINT_MINMAX, 2);
   OUT_RING(ring, rasterizer->gras_su_point_minmax);
            OUT_PKT0(ring, REG_A3XX_GRAS_SU_POLY_OFFSET_SCALE, 2);
   OUT_RING(ring, rasterizer->gras_su_poly_offset_scale);
               if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
      uint32_t val =
         uint8_t planes = ctx->rasterizer->clip_plane_enable;
   val |= CONDREG(
      ir3_find_sysval_regid(fp, SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL),
      val |= CONDREG(
      ir3_find_sysval_regid(fp, SYSTEM_VALUE_BARYCENTRIC_LINEAR_PIXEL),
      val |= CONDREG(
      ir3_find_sysval_regid(fp, SYSTEM_VALUE_BARYCENTRIC_PERSP_CENTROID),
      val |= CONDREG(
      ir3_find_sysval_regid(fp, SYSTEM_VALUE_BARYCENTRIC_LINEAR_CENTROID),
      /* docs say enable at least one of IJ_PERSP_CENTER/CENTROID when fragcoord
   * is used */
   val |= CONDREG(ir3_find_sysval_regid(fp, SYSTEM_VALUE_FRAG_COORD),
         val |= COND(fp->writes_pos, A3XX_GRAS_CL_CLIP_CNTL_ZCLIP_DISABLE);
   val |=
      COND(fp->fragcoord_compmask != 0,
      if (!emit->key.key.ucp_enables)
      val |= A3XX_GRAS_CL_CLIP_CNTL_NUM_USER_CLIP_PLANES(
      OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
               if (dirty & (FD_DIRTY_RASTERIZER | FD_DIRTY_PROG | FD_DIRTY_UCP)) {
      uint32_t planes = ctx->rasterizer->clip_plane_enable;
            if (emit->key.key.ucp_enables)
            while (planes && count < 6) {
               planes &= ~(1U << i);
   fd_wfi(ctx->batch, ring);
   OUT_PKT0(ring, REG_A3XX_GRAS_CL_USER_PLANE(count++), 4);
   OUT_RING(ring, fui(ctx->ucp.ucp[i][0]));
   OUT_RING(ring, fui(ctx->ucp.ucp[i][1]));
   OUT_RING(ring, fui(ctx->ucp.ucp[i][2]));
                  /* NOTE: since primitive_restart is not actually part of any
   * state object, we need to make sure that we always emit
   * PRIM_VTX_CNTL.. either that or be more clever and detect
   * when it changes.
   */
   if (emit->info) {
      const struct pipe_draw_info *info = emit->info;
            if (!emit->binning_pass) {
      uint32_t stride_in_vpc = align(fp->total_in, 4) / 4;
   if (stride_in_vpc > 0)
                     if (info->index_size && info->primitive_restart) {
                           OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
               if (dirty & (FD_DIRTY_SCISSOR | FD_DIRTY_RASTERIZER | FD_DIRTY_VIEWPORT)) {
      struct pipe_scissor_state *scissor = fd_context_get_scissor(ctx);
   int minx = scissor->minx;
   int miny = scissor->miny;
   int maxx = scissor->maxx;
            /* Unfortunately there is no separate depth clip disable, only an all
   * or nothing deal. So when we disable clipping, we must handle the
   * viewport clip via scissors.
   */
   if (!ctx->rasterizer->depth_clip_near) {
               minx = MAX2(minx, (int)floorf(vp->translate[0] - fabsf(vp->scale[0])));
   miny = MAX2(miny, (int)floorf(vp->translate[1] - fabsf(vp->scale[1])));
   maxx = MIN2(maxx + 1, (int)ceilf(vp->translate[0] + fabsf(vp->scale[0]))) - 1;
               OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(minx) |
         OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(maxx) |
            ctx->batch->max_scissor.minx = MIN2(ctx->batch->max_scissor.minx, minx);
   ctx->batch->max_scissor.miny = MIN2(ctx->batch->max_scissor.miny, miny);
   ctx->batch->max_scissor.maxx = MAX2(ctx->batch->max_scissor.maxx, maxx);
               if (dirty & FD_DIRTY_VIEWPORT) {
                        OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
   OUT_RING(ring,
         OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE(vp->scale[0]));
   OUT_RING(ring,
         OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(vp->scale[1]));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(vp->translate[2]));
               if (dirty &
      (FD_DIRTY_VIEWPORT | FD_DIRTY_RASTERIZER | FD_DIRTY_FRAMEBUFFER)) {
   float zmin, zmax;
   int depth = 24;
   if (ctx->batch->framebuffer.zsbuf) {
      depth = util_format_get_component_bits(
      pipe_surface_format(ctx->batch->framebuffer.zsbuf),
   }
   util_viewport_zmin_zmax(&ctx->viewport[0], ctx->rasterizer->clip_halfz,
            OUT_PKT0(ring, REG_A3XX_RB_Z_CLAMP_MIN, 2);
   if (depth == 32) {
      OUT_RING(ring, (uint32_t)(zmin * (float)0xffffffff));
      } else if (depth == 16) {
      OUT_RING(ring, (uint32_t)(zmin * 0xffff));
      } else {
      OUT_RING(ring, (uint32_t)(zmin * 0xffffff));
                  if (dirty & (FD_DIRTY_PROG | FD_DIRTY_FRAMEBUFFER | FD_DIRTY_BLEND_DUAL)) {
      struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
   int nr_cbufs = pfb->nr_cbufs;
   if (fd3_blend_stateobj(ctx->blend)->rb_render_control &
      A3XX_RB_RENDER_CONTROL_DUAL_COLOR_IN_ENABLE)
                  /* TODO we should not need this or fd_wfi() before emit_constants():
   */
   OUT_PKT3(ring, CP_EVENT_WRITE, 1);
            if (!emit->skip_consts) {
      ir3_emit_vs_consts(vp, ring, ctx, emit->info, emit->indirect, emit->draw);
   if (!emit->binning_pass)
               if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_FRAMEBUFFER)) {
      struct fd3_blend_stateobj *blend = fd3_blend_stateobj(ctx->blend);
            for (i = 0; i < ARRAY_SIZE(blend->rb_mrt); i++) {
      enum pipe_format format =
         const struct util_format_description *desc =
         bool is_float = util_format_is_float(format);
   bool is_int = util_format_is_pure_integer(format);
                  if (is_int) {
      control &= (A3XX_RB_MRT_CONTROL_COMPONENT_ENABLE__MASK |
                                    if (!has_alpha) {
                  if (format && util_format_get_component_bits(
            const struct pipe_rt_blend_state *rt;
   if (ctx->blend->independent_blend_enable)
                        if (!util_format_colormask_full(desc, rt->colormask))
                              OUT_PKT0(ring, REG_A3XX_RB_MRT_BLEND_CONTROL(i), 1);
   OUT_RING(ring,
                        if (dirty & FD_DIRTY_BLEND_COLOR) {
      struct pipe_blend_color *bcolor = &ctx->blend_color;
   OUT_PKT0(ring, REG_A3XX_RB_BLEND_RED, 4);
   OUT_RING(ring, A3XX_RB_BLEND_RED_UINT(CLAMP(bcolor->color[0], 0.f, 1.f) * 0xff) |
         OUT_RING(ring, A3XX_RB_BLEND_GREEN_UINT(CLAMP(bcolor->color[1], 0.f, 1.f) * 0xff) |
         OUT_RING(ring, A3XX_RB_BLEND_BLUE_UINT(CLAMP(bcolor->color[2], 0.f, 1.f) * 0xff) |
         OUT_RING(ring, A3XX_RB_BLEND_ALPHA_UINT(CLAMP(bcolor->color[3], 0.f, 1.f) * 0xff) |
               if (dirty & FD_DIRTY_TEX)
            if (ctx->dirty_shader[PIPE_SHADER_VERTEX] & FD_DIRTY_SHADER_TEX)
            if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_TEX)
      }
      /* emit setup at begin of new cmdstream buffer (don't rely on previous
   * state, there could have been a context switch between ioctls):
   */
   void
   fd3_emit_restore(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
      struct fd_context *ctx = batch->ctx;
   struct fd3_context *fd3_ctx = fd3_context(ctx);
            if (ctx->screen->gpu_id == 320) {
      OUT_PKT3(ring, CP_REG_RMW, 3);
   OUT_RING(ring, REG_A3XX_RBBM_CLOCK_CTL);
   OUT_RING(ring, 0xfffcffff);
               fd_wfi(batch, ring);
   OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
            OUT_PKT0(ring, REG_A3XX_SP_VS_PVT_MEM_PARAM_REG, 3);
   OUT_RING(ring, 0x08000001);                    /* SP_VS_PVT_MEM_CTRL_REG */
   OUT_RELOC(ring, fd3_ctx->vs_pvt_mem, 0, 0, 0); /* SP_VS_PVT_MEM_ADDR_REG */
            OUT_PKT0(ring, REG_A3XX_SP_FS_PVT_MEM_PARAM_REG, 3);
   OUT_RING(ring, 0x08000001);                    /* SP_FS_PVT_MEM_CTRL_REG */
   OUT_RELOC(ring, fd3_ctx->fs_pvt_mem, 0, 0, 0); /* SP_FS_PVT_MEM_ADDR_REG */
            OUT_PKT0(ring, REG_A3XX_PC_VERTEX_REUSE_BLOCK_CNTL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_RB_MSAA_CONTROL, 2);
   OUT_RING(ring, A3XX_RB_MSAA_CONTROL_DISABLE |
                        OUT_PKT0(ring, REG_A3XX_GRAS_CL_GB_CLIP_ADJ, 1);
   OUT_RING(ring, A3XX_GRAS_CL_GB_CLIP_ADJ_HORZ(0) |
            OUT_PKT0(ring, REG_A3XX_GRAS_TSE_DEBUG_ECO, 1);
            OUT_PKT0(ring, REG_A3XX_TPL1_TP_VS_TEX_OFFSET, 1);
   OUT_RING(ring, A3XX_TPL1_TP_VS_TEX_OFFSET_SAMPLEROFFSET(VERT_TEX_OFF) |
                        OUT_PKT0(ring, REG_A3XX_TPL1_TP_FS_TEX_OFFSET, 1);
   OUT_RING(ring, A3XX_TPL1_TP_FS_TEX_OFFSET_SAMPLEROFFSET(FRAG_TEX_OFF) |
                        OUT_PKT0(ring, REG_A3XX_VPC_VARY_CYLWRAP_ENABLE_0, 2);
   OUT_RING(ring, 0x00000000); /* VPC_VARY_CYLWRAP_ENABLE_0 */
            OUT_PKT0(ring, REG_A3XX_UNKNOWN_0E43, 1);
            OUT_PKT0(ring, REG_A3XX_UNKNOWN_0F03, 1);
            OUT_PKT0(ring, REG_A3XX_UNKNOWN_0EE0, 1);
            OUT_PKT0(ring, REG_A3XX_UNKNOWN_0C3D, 1);
            OUT_PKT0(ring, REG_A3XX_HLSQ_PERFCOUNTER0_SELECT, 1);
            OUT_PKT0(ring, REG_A3XX_HLSQ_CONST_VSPRESV_RANGE_REG, 2);
   OUT_RING(ring, A3XX_HLSQ_CONST_VSPRESV_RANGE_REG_STARTENTRY(0) |
         OUT_RING(ring, A3XX_HLSQ_CONST_FSPRESV_RANGE_REG_STARTENTRY(0) |
                     OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SU_POINT_MINMAX, 2);
   OUT_RING(ring, 0xffc00010); /* GRAS_SU_POINT_MINMAX */
            OUT_PKT0(ring, REG_A3XX_PC_RESTART_INDEX, 1);
            OUT_PKT0(ring, REG_A3XX_RB_WINDOW_OFFSET, 1);
            OUT_PKT0(ring, REG_A3XX_RB_BLEND_RED, 4);
   OUT_RING(ring, A3XX_RB_BLEND_RED_UINT(0) | A3XX_RB_BLEND_RED_FLOAT(0.0f));
   OUT_RING(ring, A3XX_RB_BLEND_GREEN_UINT(0) | A3XX_RB_BLEND_GREEN_FLOAT(0.0f));
   OUT_RING(ring, A3XX_RB_BLEND_BLUE_UINT(0) | A3XX_RB_BLEND_BLUE_FLOAT(0.0f));
   OUT_RING(ring,
            for (i = 0; i < 6; i++) {
      OUT_PKT0(ring, REG_A3XX_GRAS_CL_USER_PLANE(i), 4);
   OUT_RING(ring, 0x00000000); /* GRAS_CL_USER_PLANE[i].X */
   OUT_RING(ring, 0x00000000); /* GRAS_CL_USER_PLANE[i].Y */
   OUT_RING(ring, 0x00000000); /* GRAS_CL_USER_PLANE[i].Z */
               OUT_PKT0(ring, REG_A3XX_PC_VSTREAM_CONTROL, 1);
                     if (is_a3xx_p0(ctx->screen)) {
      OUT_PKT3(ring, CP_DRAW_INDX, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, DRAW(1, DI_SRC_SEL_AUTO_INDEX, INDEX_SIZE_IGN,
                     OUT_PKT3(ring, CP_NOP, 4);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
                        }
      void
   fd3_emit_init_screen(struct pipe_screen *pscreen)
   {
      struct fd_screen *screen = fd_screen(pscreen);
      }
      void
   fd3_emit_init(struct pipe_context *pctx)
   {
   }
