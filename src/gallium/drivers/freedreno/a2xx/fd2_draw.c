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
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_string.h"
      #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd2_context.h"
   #include "fd2_draw.h"
   #include "fd2_emit.h"
   #include "fd2_program.h"
   #include "fd2_util.h"
   #include "fd2_zsa.h"
      static inline uint32_t
   pack_rgba(enum pipe_format format, const float *rgba)
   {
      union util_color uc;
   util_pack_color(rgba, format, &uc);
      }
      static void
   emit_cacheflush(struct fd_ringbuffer *ring)
   {
               for (i = 0; i < 12; i++) {
      OUT_PKT3(ring, CP_EVENT_WRITE, 1);
         }
      static void
   emit_vertexbufs(struct fd_context *ctx) assert_dt
   {
      struct fd_vertex_stateobj *vtx = ctx->vtx.vtx;
   struct fd_vertexbuf_stateobj *vertexbuf = &ctx->vtx.vertexbuf;
   struct fd2_vertex_buf bufs[PIPE_MAX_ATTRIBS];
            if (!vtx->num_elements)
            for (i = 0; i < vtx->num_elements; i++) {
      struct pipe_vertex_element *elem = &vtx->pipe[i];
   struct pipe_vertex_buffer *vb = &vertexbuf->vb[elem->vertex_buffer_index];
   bufs[i].offset = vb->buffer_offset;
   bufs[i].size = fd_bo_size(fd_resource(vb->buffer.resource)->bo);
               // NOTE I believe the 0x78 (or 0x9c in solid_vp) relates to the
            fd2_emit_vertex_bufs(ctx->batch->draw, 0x78, bufs, vtx->num_elements);
      }
      static void
   draw_impl(struct fd_context *ctx, const struct pipe_draw_info *info,
               {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
            OUT_PKT0(ring, REG_A2XX_TC_CNTL_STATUS, 1);
            if (is_a20x(ctx->screen)) {
      /* wait for DMA to finish and
   * dummy draw one triangle with indexes 0,0,0.
   * with PRE_FETCH_CULL_ENABLE | GRP_CULL_ENABLE.
   *
   * this workaround is for a HW bug related to DMA alignment:
   * it is necessary for indexed draws and possibly also
   * draws that read binning data
   */
   OUT_PKT3(ring, CP_WAIT_REG_EQ, 4);
   OUT_RING(ring, 0x000005d0); /* RBBM_STATUS */
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00001000); /* bit: 12: VGT_BUSY_NO_DMA */
            OUT_PKT3(ring, CP_DRAW_INDX_BIN, 6);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x0003c004);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000003);
   OUT_RELOC(ring, fd_resource(fd2_context(ctx)->solid_vertexbuf)->bo, 64, 0,
            } else {
               OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
   OUT_RING(ring, info->index_bounds_valid ? info->max_index
         OUT_RING(ring, info->index_bounds_valid ? info->min_index
               /* binning shader will take offset from C64 */
   if (binning && is_a20x(ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, 0x00000180);
   OUT_RING(ring, fui(ctx->batch->num_vertices));
   OUT_RING(ring, fui(0.0f));
   OUT_RING(ring, fui(0.0f));
               enum pc_di_vis_cull_mode vismode = USE_VISIBILITY;
   if (binning || info->mode == MESA_PRIM_POINTS)
            fd_draw_emit(ctx->batch, ring, ctx->screen->primtypes[info->mode],
            if (is_a20x(ctx->screen)) {
      /* not sure why this is required, but it fixes some hangs */
      } else {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_UNKNOWN_2010));
                  }
      static bool
   fd2_draw_vbo(struct fd_context *ctx, const struct pipe_draw_info *pinfo,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (!ctx->prog.fs || !ctx->prog.vs)
            if (pinfo->mode != MESA_PRIM_COUNT && !indirect && !pinfo->primitive_restart &&
      !u_trim_pipe_prim(pinfo->mode, (unsigned *)&pdraw->count))
         if (ctx->dirty & FD_DIRTY_VTXBUF)
                     if (fd_binning_enabled)
                     /* a2xx can draw only 65535 vertices at once
   * on a22x the field in the draw command is 32bits but seems limited too
   * using a limit of 32k because it fixes an unexplained hang
   * 32766 works for all primitives (multiple of 2 and 3)
   */
   if (pdraw->count > 32766) {
      /* clang-format off */
   static const uint16_t step_tbl[MESA_PRIM_COUNT] = {
      [0 ... MESA_PRIM_COUNT - 1]  = 32766,
                  /* needs more work */
   [MESA_PRIM_TRIANGLE_FAN]   = 0,
      };
            struct pipe_draw_start_count_bias draw = *pdraw;
   unsigned count = draw.count;
   unsigned step = step_tbl[pinfo->mode];
            if (!step)
            for (; count + step > 32766; count -= step) {
      draw.count = MIN2(count, 32766);
   draw_impl(ctx, pinfo, &draw, ctx->batch->draw, index_offset, false);
   draw_impl(ctx, pinfo, &draw, ctx->batch->binning, index_offset, true);
   draw.start += step;
      }
   /* changing this value is a hack, restore it */
      } else {
      draw_impl(ctx, pinfo, pdraw, ctx->batch->draw, index_offset, false);
                                    }
      static void
   fd2_draw_vbos(struct fd_context *ctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
         {
      for (unsigned i = 0; i < num_draws; i++)
      }
      static void
   clear_state(struct fd_batch *batch, struct fd_ringbuffer *ring,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd2_context *fd2_ctx = fd2_context(ctx);
            fd2_emit_vertex_bufs(ring, 0x9c,
                              OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
                     OUT_PKT0(ring, REG_A2XX_TC_CNTL_STATUS, 1);
            if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
   reg = 0;
   if (buffers & PIPE_CLEAR_DEPTH) {
      reg |= A2XX_RB_DEPTHCONTROL_ZFUNC(FUNC_ALWAYS) |
         A2XX_RB_DEPTHCONTROL_Z_ENABLE |
      }
   if (buffers & PIPE_CLEAR_STENCIL) {
      reg |= A2XX_RB_DEPTHCONTROL_STENCILFUNC(FUNC_ALWAYS) |
            }
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLORCONTROL));
   OUT_RING(ring, A2XX_RB_COLORCONTROL_ALPHA_FUNC(FUNC_ALWAYS) |
                              OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
   OUT_RING(ring, 0x00000000); /* PA_CL_CLIP_CNTL */
   OUT_RING(
      ring,
   A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST | /* PA_SU_SC_MODE_CNTL */
      A2XX_PA_SU_SC_MODE_CNTL_FRONT_PTYPE(PC_DRAW_TRIANGLES) |
            if (fast_clear) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_CONFIG));
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_MASK));
   if (buffers & PIPE_CLEAR_COLOR) {
      OUT_RING(ring, A2XX_RB_COLOR_MASK_WRITE_RED |
                  } else {
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_CONTROL));
            if (is_a20x(batch->ctx->screen))
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
   OUT_RING(ring, 3); /* VGT_MAX_VTX_INDX */
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
   OUT_RING(ring,
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_A220_RB_LRZ_VSC_CONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
      }
      static void
   clear_state_restore(struct fd_context *ctx, struct fd_ringbuffer *ring)
   {
      if (is_a20x(ctx->screen))
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_A220_RB_LRZ_VSC_CONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
      }
      static void
   clear_fast(struct fd_batch *batch, struct fd_ringbuffer *ring,
         {
               /* zero values are patched in */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_SCREEN_SCISSOR_BR));
            OUT_PKT3(ring, CP_SET_CONSTANT, 4);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_SURFACE_INFO));
   OUT_RING(ring, 0x8000 | 32);
   OUT_RING(ring, 0);
            /* set fill values */
   if (!is_a20x(batch->ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_CLEAR_COLOR));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
   OUT_RING(ring, A2XX_RB_COPY_CONTROL_DEPTH_CLEAR_ENABLE |
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTH_CLEAR));
      } else {
               OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, 0x00000480);
   OUT_RING(ring, fui((float)(color_clear >> 0 & 0xff) * sc));
   OUT_RING(ring, fui((float)(color_clear >> 8 & 0xff) * sc));
   OUT_RING(ring, fui((float)(color_clear >> 16 & 0xff) * sc));
            // XXX if using float the rounding error breaks it..
   float depth = ((double)(depth_clear >> 8)) * (1.0 / (double)0xffffff);
   assert((unsigned)(((double)depth * (double)0xffffff)) ==
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_ZSCALE));
   OUT_RING(ring, fui(0.0f));
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
   OUT_RING(ring,
            0xff000000 |
      OUT_RING(ring, 0xff000000 |
                     fd_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static bool
   fd2_clear_fast(struct fd_context *ctx, unsigned buffers,
               {
      /* using 4x MSAA allows clearing ~2x faster
   * then we can use higher bpp clearing to clear lower bpp
   * 1 "pixel" can clear 64 bits (rgba8+depth24+stencil8)
   * note: its possible to clear with 32_32_32_32 format but its not faster
   * note: fast clear doesn't work with sysmem rendering
   * (sysmem rendering is disabled when clear is used)
   *
   * we only have 16-bit / 32-bit color formats
   * and 16-bit / 32-bit depth formats
   * so there are only a few possible combinations
   *
   * if the bpp of the color/depth doesn't match
   * we clear with depth/color individually
   */
   struct fd2_context *fd2_ctx = fd2_context(ctx);
   struct fd_batch *batch = ctx->batch;
   struct fd_ringbuffer *ring = batch->draw;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   uint32_t color_clear = 0, depth_clear = 0;
   enum pipe_format format = pipe_surface_format(pfb->cbufs[0]);
   int depth_size = -1; /* -1: no clear, 0: clear 16-bit, 1: clear 32-bit */
            /* TODO: need to test performance on a22x */
   if (!is_a20x(ctx->screen))
            if (buffers & PIPE_CLEAR_COLOR)
            if (buffers & (PIPE_CLEAR_DEPTH | PIPE_CLEAR_STENCIL)) {
      /* no fast clear when clearing only one component of depth+stencil buffer */
   if (!(buffers & PIPE_CLEAR_DEPTH))
            if ((pfb->zsbuf->format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
      pfb->zsbuf->format == PIPE_FORMAT_S8_UINT_Z24_UNORM) &&
                                    if (color_size == 0) {
      color_clear = pack_rgba(format, color->f);
      } else if (color_size == 1) {
                  if (depth_size == 0) {
      depth_clear = (uint32_t)(0xffff * depth);
      } else if (depth_size == 1) {
      depth_clear = (((uint32_t)(0xffffff * depth)) << 8);
               /* disable "window" scissor.. */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
   OUT_RING(ring, xy2d(0, 0));
            /* make sure we fill all "pixels" (in SCREEN_SCISSOR) */
   OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
   OUT_RING(ring, fui(4096.0f));
   OUT_RING(ring, fui(4096.0f));
   OUT_RING(ring, fui(4096.0f));
                     if (color_size >= 0 && depth_size != color_size)
      clear_fast(batch, ring, color_clear, color_clear,
         if (depth_size >= 0 && depth_size != color_size)
      clear_fast(batch, ring, depth_clear, depth_clear,
         if (depth_size == color_size)
      clear_fast(batch, ring, color_clear, depth_clear,
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_CONFIG));
            /* can't patch in SCREEN_SCISSOR_BR as it can be different for each tile.
   * MEM_WRITE the value in tile_renderprep, and use CP_LOAD_CONSTANT_CONTEXT
   * the value is read from byte offset 60 in the given bo
   */
   OUT_PKT3(ring, CP_LOAD_CONSTANT_CONTEXT, 3);
   OUT_RELOC(ring, fd_resource(fd2_ctx->solid_vertexbuf)->bo, 0, 0, 0);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_SCREEN_SCISSOR_BR));
            OUT_PKT3(ring, CP_SET_CONSTANT, 4);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_SURFACE_INFO));
   OUT_RINGP(ring, GMEM_PATCH_RESTORE_INFO, &batch->gmem_patches);
   OUT_RING(ring, 0);
   OUT_RING(ring, 0);
      }
      static bool
   fd2_clear(struct fd_context *ctx, enum fd_buffer_mask buffers,
               {
      struct fd_ringbuffer *ring = ctx->batch->draw;
            if (fd2_clear_fast(ctx, buffers, color, depth, stencil))
            /* set clear value */
   if (is_a20x(ctx->screen)) {
      if (buffers & FD_BUFFER_COLOR) {
      /* C0 used by fragment shader */
   OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, 0x00000480);
   OUT_RING(ring, color->ui[0]);
   OUT_RING(ring, color->ui[1]);
   OUT_RING(ring, color->ui[2]);
               if (buffers & FD_BUFFER_DEPTH) {
      /* use viewport to set depth value */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_ZSCALE));
   OUT_RING(ring, fui(0.0f));
               if (buffers & FD_BUFFER_STENCIL) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_STENCILREFMASK_BF));
   OUT_RING(ring, 0xff000000 |
               OUT_RING(ring, 0xff000000 |
               } else {
      if (buffers & FD_BUFFER_COLOR) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_CLEAR_COLOR));
               if (buffers & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) {
      uint32_t clear_mask, depth_clear;
   switch (fd_pipe2depth(fb->zsbuf->format)) {
   case DEPTHX_24_8:
      clear_mask = ((buffers & FD_BUFFER_DEPTH) ? 0xe : 0) |
         depth_clear =
            case DEPTHX_16:
      clear_mask = 0xf;
   depth_clear = (uint32_t)(0xffffffff * depth);
      default:
      unreachable("invalid depth");
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTH_CLEAR));
                  /* scissor state */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
   OUT_RING(ring, xy2d(0, 0));
            /* viewport state */
   OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
   OUT_RING(ring, fui((float)fb->width / 2.0f));
   OUT_RING(ring, fui((float)fb->width / 2.0f));
   OUT_RING(ring, fui((float)fb->height / 2.0f));
            /* common state */
            fd_draw(ctx->batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
                  dirty:
      ctx->dirty |= FD_DIRTY_ZSA | FD_DIRTY_VIEWPORT | FD_DIRTY_RASTERIZER |
                  ctx->dirty_shader[PIPE_SHADER_VERTEX] |= FD_DIRTY_SHADER_PROG;
   ctx->dirty_shader[PIPE_SHADER_FRAGMENT] |=
               }
      void
   fd2_draw_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->draw_vbos = fd2_draw_vbos;
      }
