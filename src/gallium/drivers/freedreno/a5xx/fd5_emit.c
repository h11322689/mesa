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
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_viewport.h"
      #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
      #include "fd5_blend.h"
   #include "fd5_blitter.h"
   #include "fd5_context.h"
   #include "fd5_emit.h"
   #include "fd5_format.h"
   #include "fd5_image.h"
   #include "fd5_program.h"
   #include "fd5_rasterizer.h"
   #include "fd5_screen.h"
   #include "fd5_texture.h"
   #include "fd5_zsa.h"
      #define emit_const_user fd5_emit_const_user
   #define emit_const_bo   fd5_emit_const_bo
   #include "ir3_const.h"
      /* regid:          base const register
   * prsc or dwords: buffer containing constant values
   * sizedwords:     size of const value buffer
   */
   static void
   fd5_emit_const_user(struct fd_ringbuffer *ring,
               {
               OUT_PKT7(ring, CP_LOAD_STATE4, 3 + sizedwords);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(regid / 4) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_EXT_SRC_ADDR(0) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   for (int i = 0; i < sizedwords; i++)
      }
      static void
   fd5_emit_const_bo(struct fd_ringbuffer *ring,
               {
      uint32_t dst_off = regid / 4;
   assert(dst_off % 4 == 0);
   uint32_t num_unit = sizedwords / 4;
                     OUT_PKT7(ring, CP_LOAD_STATE4, 3);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(dst_off) |
                        }
      static void
   fd5_emit_const_ptrs(struct fd_ringbuffer *ring, gl_shader_stage type,
               {
      uint32_t anum = align(num, 2);
                     OUT_PKT7(ring, CP_LOAD_STATE4, 3 + (2 * anum));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(regid / 4) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_EXT_SRC_ADDR(0) |
                  for (i = 0; i < num; i++) {
      if (bos[i]) {
         } else {
      OUT_RING(ring, 0xbad00000 | (i << 16));
                  for (; i < anum; i++) {
      OUT_RING(ring, 0xffffffff);
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
   fd5_emit_cs_consts(const struct ir3_shader_variant *v,
               {
         }
      /* Border color layout is diff from a4xx/a5xx.. if it turns out to be
   * the same as a6xx then move this somewhere common ;-)
   *
   * Entry layout looks like (total size, 0x60 bytes):
   */
      struct PACKED bcolor_entry {
      uint32_t fp32[4];
   uint16_t ui16[4];
            uint16_t fp16[4];
   uint16_t rgb565;
   uint16_t rgb5a1;
   uint16_t rgba4;
   uint8_t __pad0[2];
   uint8_t ui8[4];
   int8_t si8[4];
   uint32_t rgb10a2;
            uint16_t
            };
      #define FD5_BORDER_COLOR_SIZE 0x80
   #define FD5_BORDER_COLOR_UPLOAD_SIZE                                           \
            static void
   setup_border_colors(struct fd_texture_stateobj *tex,
         {
      unsigned i, j;
            for (i = 0; i < tex->num_samplers; i++) {
      struct bcolor_entry *e = &entries[i];
   struct pipe_sampler_state *sampler = tex->samplers[i];
            if (!sampler)
                     enum pipe_format format = sampler->border_color_format;
   const struct util_format_description *desc =
            e->rgb565 = 0;
   e->rgb5a1 = 0;
   e->rgba4 = 0;
   e->rgb10a2 = 0;
            for (j = 0; j < 4; j++) {
                     /*
   * HACK: for PIPE_FORMAT_X24S8_UINT we end up w/ the
   * stencil border color value in bc->ui[0] but according
   * to desc->swizzle and desc->channel, the .x component
   * is NONE and the stencil value is in the y component.
   * Meanwhile the hardware wants this in the .x componetn.
   */
   if ((format == PIPE_FORMAT_X24S8_UINT) ||
      (format == PIPE_FORMAT_X32_S8X24_UINT)) {
   if (j == 0) {
      c = 1;
      } else {
                                    if (desc->channel[c].pure_integer) {
      uint16_t clamped;
   switch (desc->channel[c].size) {
   case 2:
      assert(desc->channel[c].type == UTIL_FORMAT_TYPE_UNSIGNED);
   clamped = CLAMP(bc->ui[j], 0, 0x3);
      case 8:
      if (desc->channel[c].type == UTIL_FORMAT_TYPE_SIGNED)
         else
            case 10:
      assert(desc->channel[c].type == UTIL_FORMAT_TYPE_UNSIGNED);
   clamped = CLAMP(bc->ui[j], 0, 0x3ff);
      case 16:
      if (desc->channel[c].type == UTIL_FORMAT_TYPE_SIGNED)
         else
            default:
         case 32:
      clamped = 0;
      }
   e->fp32[cd] = bc->ui[j];
      } else {
      float f = bc->f[j];
                  e->fp32[c] = fui(f);
   e->fp16[c] = _mesa_float_to_half(f);
   e->srgb[c] = _mesa_float_to_half(f_u);
   e->ui16[c] = f_u * 0xffff;
   e->si16[c] = f_s * 0x7fff;
   e->ui8[c] = f_u * 0xff;
   e->si8[c] = f_s * 0x7f;
   if (c == 1)
         else if (c < 3)
         if (c == 3)
         else
         if (c == 3)
         else
         e->rgba4 |= (int)(f_u * 0xf) << (c * 4);
   if (c == 0)
            #ifdef DEBUG
         memset(&e->__pad0, 0, sizeof(e->__pad0));
   #endif
         }
      static void
   emit_border_color(struct fd_context *ctx, struct fd_ringbuffer *ring) assert_dt
   {
      struct fd5_context *fd5_ctx = fd5_context(ctx);
   struct bcolor_entry *entries;
   unsigned off;
                     const unsigned int alignment =
         u_upload_alloc(fd5_ctx->border_color_uploader, 0,
                           setup_border_colors(&ctx->tex[PIPE_SHADER_VERTEX], &entries[0]);
   setup_border_colors(&ctx->tex[PIPE_SHADER_FRAGMENT],
            OUT_PKT4(ring, REG_A5XX_TPL1_TP_BORDER_COLOR_BASE_ADDR_LO, 2);
               }
      static bool
   emit_textures(struct fd_context *ctx, struct fd_ringbuffer *ring,
               {
      bool needs_border = false;
   unsigned bcolor_offset =
                  if (tex->num_samplers > 0) {
      /* output sampler state: */
   OUT_PKT7(ring, CP_LOAD_STATE4, 3 + (4 * tex->num_samplers));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_SHADER) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   for (i = 0; i < tex->num_samplers; i++) {
      static const struct fd5_sampler_stateobj dummy_sampler = {};
   const struct fd5_sampler_stateobj *sampler =
      tex->samplers[i] ? fd5_sampler_stateobj(tex->samplers[i])
      OUT_RING(ring, sampler->texsamp0);
   OUT_RING(ring, sampler->texsamp1);
   OUT_RING(ring, sampler->texsamp2 |
                                 if (tex->num_textures > 0) {
               /* emit texture state: */
   OUT_PKT7(ring, CP_LOAD_STATE4, 3 + (12 * num_textures));
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   for (i = 0; i < tex->num_textures; i++) {
      static const struct fd5_pipe_sampler_view dummy_view = {};
   const struct fd5_pipe_sampler_view *view =
      tex->textures[i] ? fd5_pipe_sampler_view(tex->textures[i])
                              OUT_RING(ring,
         OUT_RING(ring, view->texconst1);
   OUT_RING(ring, view->texconst2);
   OUT_RING(ring, view->texconst3);
   if (view->base.texture) {
      struct fd_resource *rsc = fd_resource(view->base.texture);
   if (view->base.format == PIPE_FORMAT_X32_S8X24_UINT)
         OUT_RELOC(ring, rsc->bo, view->offset,
      } else {
      OUT_RING(ring, 0x00000000);
      }
   OUT_RING(ring, view->texconst6);
   OUT_RING(ring, view->texconst7);
   OUT_RING(ring, view->texconst8);
   OUT_RING(ring, view->texconst9);
   OUT_RING(ring, view->texconst10);
                     }
      static void
   emit_ssbos(struct fd_context *ctx, struct fd_ringbuffer *ring,
               {
               if (count == 0)
            OUT_PKT7(ring, CP_LOAD_STATE4, 3 + 2 * count);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_CONSTANTS) |
                  for (unsigned i = 0; i < count; i++) {
      struct pipe_shader_buffer *buf = &so->sb[i];
            /* Unlike a6xx, SSBO size is in bytes. */
   OUT_RING(ring, A5XX_SSBO_1_0_WIDTH(sz & MASK(16)));
               OUT_PKT7(ring, CP_LOAD_STATE4, 3 + 2 * count);
   OUT_RING(ring, CP_LOAD_STATE4_0_DST_OFF(0) |
                     OUT_RING(ring, CP_LOAD_STATE4_1_STATE_TYPE(ST4_UBO) |
         OUT_RING(ring, CP_LOAD_STATE4_2_EXT_SRC_ADDR_HI(0));
   for (unsigned i = 0; i < count; i++) {
               if (buf->buffer) {
      struct fd_resource *rsc = fd_resource(buf->buffer);
      } else {
      OUT_RING(ring, 0x00000000);
            }
      void
   fd5_emit_vertex_bufs(struct fd_ringbuffer *ring, struct fd5_emit *emit)
   {
      int32_t i, j;
   const struct fd_vertex_state *vtx = emit->vtx;
            for (i = 0, j = 0; i <= vp->inputs_count; i++) {
      if (vp->inputs[i].sysval)
         if (vp->inputs[i].compmask) {
      struct pipe_vertex_element *elem = &vtx->vtx->pipe[i];
   const struct pipe_vertex_buffer *vb =
         struct fd_resource *rsc = fd_resource(vb->buffer.resource);
   enum pipe_format pfmt = elem->src_format;
   enum a5xx_vtx_fmt fmt = fd5_pipe2vtx(pfmt);
   bool isint = util_format_is_pure_integer(pfmt);
   uint32_t off = vb->buffer_offset + elem->src_offset;
                  OUT_PKT4(ring, REG_A5XX_VFD_FETCH(j), 4);
   OUT_RELOC(ring, rsc->bo, off, 0, 0);
                  OUT_PKT4(ring, REG_A5XX_VFD_DECODE(j), 2);
   OUT_RING(
      ring,
   A5XX_VFD_DECODE_INSTR_IDX(j) | A5XX_VFD_DECODE_INSTR_FORMAT(fmt) |
      COND(elem->instance_divisor, A5XX_VFD_DECODE_INSTR_INSTANCED) |
   A5XX_VFD_DECODE_INSTR_SWAP(fd5_pipe2swap(pfmt)) |
   A5XX_VFD_DECODE_INSTR_UNK30 |
   OUT_RING(
                  OUT_PKT4(ring, REG_A5XX_VFD_DEST_CNTL(j), 1);
   OUT_RING(ring,
                                 OUT_PKT4(ring, REG_A5XX_VFD_CONTROL_0, 1);
      }
      void
   fd5_emit_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
      struct pipe_framebuffer_state *pfb = &ctx->batch->framebuffer;
   const struct ir3_shader_variant *vp = fd5_emit_get_vp(emit);
   const struct ir3_shader_variant *fp = fd5_emit_get_fp(emit);
   const enum fd_dirty_3d_state dirty = emit->dirty;
                     if ((dirty & FD_DIRTY_FRAMEBUFFER) && !emit->binning_pass) {
               for (unsigned i = 0; i < A5XX_MAX_RENDER_TARGETS; i++) {
                  OUT_PKT4(ring, REG_A5XX_RB_RENDER_COMPONENTS, 1);
   OUT_RING(ring, A5XX_RB_RENDER_COMPONENTS_RT0(mrt_comp[0]) |
                     A5XX_RB_RENDER_COMPONENTS_RT1(mrt_comp[1]) |
   A5XX_RB_RENDER_COMPONENTS_RT2(mrt_comp[2]) |
   A5XX_RB_RENDER_COMPONENTS_RT3(mrt_comp[3]) |
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_FRAMEBUFFER)) {
      struct fd5_zsa_stateobj *zsa = fd5_zsa_stateobj(ctx->zsa);
            if (util_format_is_pure_integer(pipe_surface_format(pfb->cbufs[0])))
            OUT_PKT4(ring, REG_A5XX_RB_ALPHA_CONTROL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_STENCIL_CONTROL, 1);
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_BLEND | FD_DIRTY_PROG)) {
      struct fd5_blend_stateobj *blend = fd5_blend_stateobj(ctx->blend);
            if (pfb->zsbuf) {
                     if (emit->no_lrz_write || !rsc->lrz || !rsc->lrz_valid)
                        OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_CNTL, 1);
                  if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_STENCIL_REF)) {
      struct fd5_zsa_stateobj *zsa = fd5_zsa_stateobj(ctx->zsa);
            OUT_PKT4(ring, REG_A5XX_RB_STENCILREFMASK, 2);
   OUT_RING(ring, zsa->rb_stencilrefmask |
         OUT_RING(ring, zsa->rb_stencilrefmask_bf |
               if (dirty & (FD_DIRTY_ZSA | FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
      struct fd5_zsa_stateobj *zsa = fd5_zsa_stateobj(ctx->zsa);
   bool fragz = fp->no_earlyz || fp->has_kill || zsa->base.alpha_enabled ||
            OUT_PKT4(ring, REG_A5XX_RB_DEPTH_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_DEPTH_PLANE_CNTL, 1);
   OUT_RING(ring, COND(fragz, A5XX_RB_DEPTH_PLANE_CNTL_FRAG_WRITES_Z) |
                  OUT_PKT4(ring, REG_A5XX_GRAS_SU_DEPTH_PLANE_CNTL, 1);
   OUT_RING(ring, COND(fragz, A5XX_GRAS_SU_DEPTH_PLANE_CNTL_FRAG_WRITES_Z) |
                     /* NOTE: scissor enabled bit is part of rasterizer state: */
   if (dirty & (FD_DIRTY_SCISSOR | FD_DIRTY_RASTERIZER)) {
               OUT_PKT4(ring, REG_A5XX_GRAS_SC_SCREEN_SCISSOR_TL_0, 2);
   OUT_RING(ring, A5XX_GRAS_SC_SCREEN_SCISSOR_TL_0_X(scissor->minx) |
         OUT_RING(ring, A5XX_GRAS_SC_SCREEN_SCISSOR_TL_0_X(scissor->maxx) |
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_VIEWPORT_SCISSOR_TL_0, 2);
   OUT_RING(ring, A5XX_GRAS_SC_VIEWPORT_SCISSOR_TL_0_X(scissor->minx) |
         OUT_RING(ring,
                  ctx->batch->max_scissor.minx =
         ctx->batch->max_scissor.miny =
         ctx->batch->max_scissor.maxx =
         ctx->batch->max_scissor.maxy =
               if (dirty & FD_DIRTY_VIEWPORT) {
                        OUT_PKT4(ring, REG_A5XX_GRAS_CL_VPORT_XOFFSET_0, 6);
   OUT_RING(ring, A5XX_GRAS_CL_VPORT_XOFFSET_0(vp->translate[0]));
   OUT_RING(ring, A5XX_GRAS_CL_VPORT_XSCALE_0(vp->scale[0]));
   OUT_RING(ring, A5XX_GRAS_CL_VPORT_YOFFSET_0(vp->translate[1]));
   OUT_RING(ring, A5XX_GRAS_CL_VPORT_YSCALE_0(vp->scale[1]));
   OUT_RING(ring, A5XX_GRAS_CL_VPORT_ZOFFSET_0(vp->translate[2]));
               if (dirty & FD_DIRTY_PROG)
            if (dirty & FD_DIRTY_RASTERIZER) {
      struct fd5_rasterizer_stateobj *rasterizer =
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_CNTL, 1);
   OUT_RING(ring, rasterizer->gras_su_cntl |
                  OUT_PKT4(ring, REG_A5XX_GRAS_SU_POINT_MINMAX, 2);
   OUT_RING(ring, rasterizer->gras_su_point_minmax);
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_POLY_OFFSET_SCALE, 3);
   OUT_RING(ring, rasterizer->gras_su_poly_offset_scale);
   OUT_RING(ring, rasterizer->gras_su_poly_offset_offset);
            OUT_PKT4(ring, REG_A5XX_PC_RASTER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_CL_CNTL, 1);
               /* note: must come after program emit.. because there is some overlap
   * in registers, ex. PC_PRIMITIVE_CNTL and we rely on some cached
   * values from fd5_program_emit() to avoid having to re-emit the prog
   * every time rast state changes.
   *
   * Since the primitive restart state is not part of a tracked object, we
   * re-emit this register every time.
   */
   if (emit->info && ctx->rasterizer) {
      struct fd5_rasterizer_stateobj *rasterizer =
                  OUT_PKT4(ring, REG_A5XX_PC_PRIMITIVE_CNTL, 1);
   OUT_RING(ring,
            rasterizer->pc_primitive_cntl |
                  if (dirty & (FD_DIRTY_FRAMEBUFFER | FD_DIRTY_RASTERIZER | FD_DIRTY_PROG)) {
      uint32_t posz_regid = ir3_find_output_regid(fp, FRAG_RESULT_DEPTH);
            if (emit->binning_pass)
         else if (ctx->rasterizer->rasterizer_discard)
            OUT_PKT4(ring, REG_A5XX_RB_FS_OUTPUT_CNTL, 1);
   OUT_RING(ring,
                  OUT_PKT4(ring, REG_A5XX_SP_FS_OUTPUT_CNTL, 1);
   OUT_RING(ring, A5XX_SP_FS_OUTPUT_CNTL_MRT(nr) |
                     ir3_emit_vs_consts(vp, ring, ctx, emit->info, emit->indirect, emit->draw);
   if (!emit->binning_pass)
            const struct ir3_stream_output_info *info = &vp->stream_output;
   if (info->num_outputs) {
               for (unsigned i = 0; i < so->num_targets; i++) {
                                    OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_BASE_LO(i), 3);
   /* VPC_SO[i].BUFFER_BASE_LO: */
                                             OUT_PKT7(ring, CP_MEM_WRITE, 3);
                  OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_OFFSET(i), 1);
      } else {
      OUT_PKT7(ring, CP_MEM_TO_REG, 3);
   OUT_RING(ring,
            CP_MEM_TO_REG_0_REG(REG_A5XX_VPC_SO_BUFFER_OFFSET(i)) |
                  // After a draw HW would write the new offset to offset_bo
                                          if (!emit->streamout_mask && info->num_outputs) {
      OUT_PKT7(ring, CP_CONTEXT_REG_BUNCH, 4);
   OUT_RING(ring, REG_A5XX_VPC_SO_CNTL);
   OUT_RING(ring, 0);
   OUT_RING(ring, REG_A5XX_VPC_SO_BUF_CNTL);
      } else if (emit->streamout_mask && !(dirty & FD_DIRTY_PROG)) {
      /* reemit the program (if we haven't already) to re-enable streamout.  We
   * really should switch to setting up program state at compile time so we
   * can separate the SO state from the rest, and not recompute all the
   * time.
   */
               if (dirty & FD_DIRTY_BLEND) {
      struct fd5_blend_stateobj *blend = fd5_blend_stateobj(ctx->blend);
            for (i = 0; i < A5XX_MAX_RENDER_TARGETS; i++) {
      enum pipe_format format = pipe_surface_format(pfb->cbufs[i]);
   bool is_int = util_format_is_pure_integer(format);
                  if (is_int) {
      control &= A5XX_RB_MRT_CONTROL_COMPONENT_ENABLE__MASK;
               if (!has_alpha) {
                                 OUT_PKT4(ring, REG_A5XX_RB_MRT_BLEND_CONTROL(i), 1);
               OUT_PKT4(ring, REG_A5XX_SP_BLEND_CNTL, 1);
               if (dirty & (FD_DIRTY_BLEND | FD_DIRTY_SAMPLE_MASK)) {
               OUT_PKT4(ring, REG_A5XX_RB_BLEND_CNTL, 1);
   OUT_RING(ring, blend->rb_blend_cntl |
               if (dirty & FD_DIRTY_BLEND_COLOR) {
               OUT_PKT4(ring, REG_A5XX_RB_BLEND_RED, 8);
   OUT_RING(ring, A5XX_RB_BLEND_RED_FLOAT(bcolor->color[0]) |
               OUT_RING(ring, A5XX_RB_BLEND_RED_F32(bcolor->color[0]));
   OUT_RING(ring, A5XX_RB_BLEND_GREEN_FLOAT(bcolor->color[1]) |
               OUT_RING(ring, A5XX_RB_BLEND_RED_F32(bcolor->color[1]));
   OUT_RING(ring, A5XX_RB_BLEND_BLUE_FLOAT(bcolor->color[2]) |
               OUT_RING(ring, A5XX_RB_BLEND_BLUE_F32(bcolor->color[2]));
   OUT_RING(ring, A5XX_RB_BLEND_ALPHA_FLOAT(bcolor->color[3]) |
                           if (ctx->dirty_shader[PIPE_SHADER_VERTEX] & FD_DIRTY_SHADER_TEX) {
      needs_border |=
         OUT_PKT4(ring, REG_A5XX_TPL1_VS_TEX_COUNT, 1);
               if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_TEX) {
      needs_border |=
               OUT_PKT4(ring, REG_A5XX_TPL1_FS_TEX_COUNT, 1);
   OUT_RING(ring, ctx->shaderimg[PIPE_SHADER_FRAGMENT].enabled_mask
                  OUT_PKT4(ring, REG_A5XX_TPL1_CS_TEX_COUNT, 1);
            if (needs_border)
            if (!emit->binning_pass) {
      if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_SSBO)
                  if (ctx->dirty_shader[PIPE_SHADER_FRAGMENT] & FD_DIRTY_SHADER_IMAGE)
         }
      void
   fd5_emit_cs_state(struct fd_context *ctx, struct fd_ringbuffer *ring,
         {
               if (dirty & FD_DIRTY_SHADER_TEX) {
      bool needs_border = false;
   needs_border |=
            if (needs_border)
            OUT_PKT4(ring, REG_A5XX_TPL1_VS_TEX_COUNT, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_HS_TEX_COUNT, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_DS_TEX_COUNT, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_GS_TEX_COUNT, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_FS_TEX_COUNT, 1);
               OUT_PKT4(ring, REG_A5XX_TPL1_CS_TEX_COUNT, 1);
   OUT_RING(ring, ctx->shaderimg[PIPE_SHADER_COMPUTE].enabled_mask
                  if (dirty & FD_DIRTY_SHADER_SSBO)
      emit_ssbos(ctx, ring, SB4_CS_SSBO, &ctx->shaderbuf[PIPE_SHADER_COMPUTE],
         if (dirty & FD_DIRTY_SHADER_IMAGE)
      }
      /* emit setup at begin of new cmdstream buffer (don't rely on previous
   * state, there could have been a context switch between ioctls):
   */
   void
   fd5_emit_restore(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
               fd5_set_render_mode(ctx, ring, BYPASS);
            OUT_PKT4(ring, REG_A5XX_HLSQ_UPDATE_CNTL, 1);
            /*
   t7              opcode: CP_PERFCOUNTER_ACTION (50) (4 dwords)
   0000000500024048:               70d08003 00000000 001c5000 00000005
   t7              opcode: CP_PERFCOUNTER_ACTION (50) (4 dwords)
            t7              opcode: CP_WAIT_FOR_IDLE (26) (1 dwords)
   0000000500024068:               70268000
            OUT_PKT4(ring, REG_A5XX_PC_RESTART_INDEX, 1);
            OUT_PKT4(ring, REG_A5XX_PC_RASTER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_POINT_MINMAX, 2);
   OUT_RING(ring, A5XX_GRAS_SU_POINT_MINMAX_MIN(1.0f) |
                  OUT_PKT4(ring, REG_A5XX_GRAS_SU_CONSERVATIVE_RAS_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_SCREEN_SCISSOR_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_SP_VS_CONFIG_MAX_CONST, 1);
            OUT_PKT4(ring, REG_A5XX_SP_FS_CONFIG_MAX_CONST, 1);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E292, 2);
   OUT_RING(ring, 0x00000000); /* UNKNOWN_E292 */
            OUT_PKT4(ring, REG_A5XX_RB_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_DBG_ECO_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VFD_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_PC_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_SP_MODE_CNTL, 1);
            if (ctx->screen->gpu_id == 540) {
      OUT_PKT4(ring, REG_A5XX_SP_DBG_ECO_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_DBG_ECO_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_DBG_ECO_CNTL, 1);
      } else {
      OUT_PKT4(ring, REG_A5XX_SP_DBG_ECO_CNTL, 1);
               OUT_PKT4(ring, REG_A5XX_TPL1_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_TIMEOUT_THRESHOLD_0, 2);
   OUT_RING(ring, 0x00000080); /* HLSQ_TIMEOUT_THRESHOLD_0 */
            OUT_PKT4(ring, REG_A5XX_VPC_DBG_ECO_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_HLSQ_MODE_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_MODE_CNTL, 1);
            /* we don't use this yet.. probably best to disable.. */
   OUT_PKT7(ring, CP_SET_DRAW_STATE, 3);
   OUT_RING(ring, CP_SET_DRAW_STATE__0_COUNT(0) |
               OUT_RING(ring, CP_SET_DRAW_STATE__1_ADDR_LO(0));
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_CONSERVATIVE_RAS_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_BIN_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_BIN_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_FS_PRIMITIVEID_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_OVERRIDE, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_BASE_LO(0), 3);
   OUT_RING(ring, 0x00000000); /* VPC_SO_BUFFER_BASE_LO_0 */
   OUT_RING(ring, 0x00000000); /* VPC_SO_BUFFER_BASE_HI_0 */
            OUT_PKT4(ring, REG_A5XX_VPC_SO_FLUSH_BASE_LO(0), 2);
   OUT_RING(ring, 0x00000000); /* VPC_SO_FLUSH_BASE_LO_0 */
            OUT_PKT4(ring, REG_A5XX_PC_GS_PARAM, 1);
            OUT_PKT4(ring, REG_A5XX_PC_HS_PARAM, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_TP_FS_ROTATION_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E004, 1);
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_LAYERED, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUF_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_OFFSET(0), 1);
            OUT_PKT4(ring, REG_A5XX_PC_GS_LAYERED, 1);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E5AB, 1);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E5C2, 1);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_BASE_LO(1), 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_OFFSET(1), 6);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_OFFSET(2), 6);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_VPC_SO_BUFFER_OFFSET(3), 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E5DB, 1);
            OUT_PKT4(ring, REG_A5XX_SP_HS_CTRL_REG0, 1);
            OUT_PKT4(ring, REG_A5XX_SP_GS_CTRL_REG0, 1);
            OUT_PKT4(ring, REG_A5XX_TPL1_VS_TEX_COUNT, 4);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_TPL1_FS_TEX_COUNT, 2);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7C0, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7C5, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7CA, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7CF, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7D4, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_UNKNOWN_E7D9, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
      }
      static void
   fd5_mem_to_mem(struct fd_ringbuffer *ring, struct pipe_resource *dst,
               {
      struct fd_bo *src_bo = fd_resource(src)->bo;
   struct fd_bo *dst_bo = fd_resource(dst)->bo;
            for (i = 0; i < sizedwords; i++) {
      OUT_PKT7(ring, CP_MEM_TO_MEM, 5);
   OUT_RING(ring, 0x00000000);
   OUT_RELOC(ring, dst_bo, dst_off, 0, 0);
            dst_off += 4;
         }
      void
   fd5_emit_init_screen(struct pipe_screen *pscreen)
   {
      struct fd_screen *screen = fd_screen(pscreen);
   screen->emit_ib = fd5_emit_ib;
      }
      void
   fd5_emit_init(struct pipe_context *pctx)
   {
   }
