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
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_draw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd4_context.h"
   #include "fd4_draw.h"
   #include "fd4_emit.h"
   #include "fd4_format.h"
   #include "fd4_gmem.h"
   #include "fd4_program.h"
   #include "fd4_zsa.h"
      static void
   fd4_gmem_emit_set_prog(struct fd_context *ctx, struct fd4_emit *emit,
         {
      emit->skip_consts = true;
   emit->key.vs = prog->vs;
   emit->key.fs = prog->fs;
   emit->prog = fd4_program_state(
         /* reset the fd4_emit_get_*p cache */
   emit->vs = NULL;
      }
      static void
   emit_mrt(struct fd_ringbuffer *ring, unsigned nr_bufs,
               {
      enum a4xx_tile_mode tile_mode;
            if (bin_w) {
         } else {
                  for (i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
      enum a4xx_color_fmt format = 0;
   enum a3xx_color_swap swap = WZYX;
   bool srgb = false;
   struct fd_resource *rsc = NULL;
   uint32_t stride = 0;
   uint32_t base = 0;
            if ((i < nr_bufs) && bufs[i]) {
                              /* In case we're drawing to Z32F_S8, the "color" actually goes to
   * the stencil
   */
   if (rsc->stencil) {
      rsc = rsc->stencil;
   pformat = rsc->b.b.format;
   if (bases)
                              if (decode_srgb)
                                                                  if (bases) {
            } else {
            } else if ((i < nr_bufs) && bases) {
                  OUT_PKT0(ring, REG_A4XX_RB_MRT_BUF_INFO(i), 3);
   OUT_RING(ring, A4XX_RB_MRT_BUF_INFO_COLOR_FORMAT(format) |
                     A4XX_RB_MRT_BUF_INFO_COLOR_TILE_MODE(tile_mode) |
   if (bin_w || (i >= nr_bufs) || !bufs[i]) {
      OUT_RING(ring, base);
      } else {
      OUT_RELOC(ring, rsc->bo, offset, 0, 0);
   /* RB_MRT[i].CONTROL3.STRIDE not emitted by c2d..
   * not sure if we need to skip it for bypass or
   * not.
   */
            }
      static bool
   use_hw_binning(struct fd_batch *batch)
   {
               /* workaround: Like on a3xx, hw binning and scissor optimization
   * don't play nice together.
   *
   * Disable binning if scissor optimization is used.
   */
   if (gmem->minx || gmem->miny)
            if ((gmem->maxpw * gmem->maxph) > 32)
            if ((gmem->maxpw > 15) || (gmem->maxph > 15))
               }
      /* transfer from gmem to system memory (ie. normal RAM) */
      static void
   emit_gmem2mem_surf(struct fd_batch *batch, bool stencil, uint32_t base,
         {
      struct fd_ringbuffer *ring = batch->gmem;
   struct fd_resource *rsc = fd_resource(psurf->texture);
   enum pipe_format pformat = psurf->format;
            if (!rsc->valid)
            if (stencil) {
      assert(rsc->stencil);
   rsc = rsc->stencil;
               offset =
                           OUT_PKT0(ring, REG_A4XX_RB_COPY_CONTROL, 4);
   OUT_RING(ring, A4XX_RB_COPY_CONTROL_MSAA_RESOLVE(MSAA_ONE) |
               OUT_RELOC(ring, rsc->bo, offset, 0, 0); /* RB_COPY_DEST_BASE */
   OUT_RING(ring, A4XX_RB_COPY_DEST_PITCH_PITCH(pitch));
   OUT_RING(ring, A4XX_RB_COPY_DEST_INFO_TILE(TILE4_LINEAR) |
                              fd4_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   fd4_emit_tile_gmem2mem(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd4_emit emit = {
      .debug = &ctx->debug,
      };
            OUT_PKT0(ring, REG_A4XX_RB_DEPTH_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_RB_STENCIL_CONTROL, 2);
   OUT_RING(ring, A4XX_RB_STENCIL_CONTROL_FUNC(FUNC_NEVER) |
                     A4XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_NEVER) |
            OUT_PKT0(ring, REG_A4XX_RB_STENCILREFMASK, 2);
   OUT_RING(ring, 0xff000000 | A4XX_RB_STENCILREFMASK_STENCILREF(0) |
               OUT_RING(ring, 0xff000000 | A4XX_RB_STENCILREFMASK_BF_STENCILREF(0) |
                  OUT_PKT0(ring, REG_A4XX_GRAS_SU_MODE_CONTROL, 1);
                     OUT_PKT0(ring, REG_A4XX_GRAS_CL_CLIP_CNTL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_CL_VPORT_XOFFSET_0, 6);
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XOFFSET_0((float)pfb->width / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XSCALE_0((float)pfb->width / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YOFFSET_0((float)pfb->height / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YSCALE_0(-(float)pfb->height / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_ZOFFSET_0(0.0f));
            OUT_PKT0(ring, REG_A4XX_RB_RENDER_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
                        OUT_PKT0(ring, REG_A4XX_PC_PRIM_VTX_CNTL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_ALPHA_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_WINDOW_SCISSOR_BR, 2);
   OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_BR_X(pfb->width - 1) |
         OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
            OUT_PKT0(ring, REG_A4XX_VFD_INDEX_OFFSET, 2);
   OUT_RING(ring, 0); /* VFD_INDEX_OFFSET */
            fd4_program_emit(ring, &emit, 0, NULL);
            if (batch->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) {
      struct fd_resource *rsc = fd_resource(pfb->zsbuf->texture);
   if (!rsc->stencil || (batch->resolve & FD_BUFFER_DEPTH))
         if (rsc->stencil && (batch->resolve & FD_BUFFER_STENCIL))
               if (batch->resolve & FD_BUFFER_COLOR) {
      unsigned i;
   for (i = 0; i < pfb->nr_cbufs; i++) {
      if (!pfb->cbufs[i])
         if (!(batch->resolve & (PIPE_CLEAR_COLOR0 << i)))
                        OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  }
      /* transfer from system memory to gmem */
      static void
   emit_mem2gmem_surf(struct fd_batch *batch, const uint32_t *bases,
         {
      struct fd_ringbuffer *ring = batch->gmem;
                     if (bufs[0] && (bufs[0]->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)) {
      /* The gmem_restore_tex logic will put the first buffer's stencil
   * as color. Supply it with the proper information to make that
   * happen.
   */
   zsbufs[0] = zsbufs[1] = bufs[0];
   bufs = zsbufs;
                        fd4_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   fd4_emit_tile_mem2gmem(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd4_emit emit = {
      .debug = &ctx->debug,
   .vtx = &ctx->blit_vbuf_state,
   .sprite_coord_enable = 1,
      };
   /* NOTE: They all use the same VP, this is for vtx bufs. */
            unsigned char mrt_comp[A4XX_MAX_RENDER_TARGETS] = {0};
   float x0, y0, x1, y1;
   unsigned bin_w = tile->bin_w;
   unsigned bin_h = tile->bin_h;
            /* write texture coordinates to vertexbuf: */
   x0 = ((float)tile->xoff) / ((float)pfb->width);
   x1 = ((float)tile->xoff + bin_w) / ((float)pfb->width);
   y0 = ((float)tile->yoff) / ((float)pfb->height);
            OUT_PKT3(ring, CP_MEM_WRITE, 5);
   OUT_RELOC(ring, fd_resource(ctx->blit_texcoord_vbuf)->bo, 0, 0, 0);
   OUT_RING(ring, fui(x0));
   OUT_RING(ring, fui(y0));
   OUT_RING(ring, fui(x1));
            for (i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
               OUT_PKT0(ring, REG_A4XX_RB_MRT_CONTROL(i), 1);
   OUT_RING(ring, A4XX_RB_MRT_CONTROL_ROP_CODE(ROP_COPY) |
            OUT_PKT0(ring, REG_A4XX_RB_MRT_BLEND_CONTROL(i), 1);
   OUT_RING(
      ring,
   A4XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(FACTOR_ONE) |
      A4XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
   A4XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(FACTOR_ZERO) |
   A4XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(FACTOR_ONE) |
               OUT_PKT0(ring, REG_A4XX_RB_RENDER_COMPONENTS, 1);
   OUT_RING(ring, A4XX_RB_RENDER_COMPONENTS_RT0(mrt_comp[0]) |
                     A4XX_RB_RENDER_COMPONENTS_RT1(mrt_comp[1]) |
   A4XX_RB_RENDER_COMPONENTS_RT2(mrt_comp[2]) |
   A4XX_RB_RENDER_COMPONENTS_RT3(mrt_comp[3]) |
            OUT_PKT0(ring, REG_A4XX_RB_RENDER_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_RB_DEPTH_CONTROL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_CL_CLIP_CNTL, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SU_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SU_MODE_CONTROL_LINEHALFWIDTH(0) |
            OUT_PKT0(ring, REG_A4XX_GRAS_CL_VPORT_XOFFSET_0, 6);
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XOFFSET_0((float)bin_w / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_XSCALE_0((float)bin_w / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YOFFSET_0((float)bin_h / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_YSCALE_0(-(float)bin_h / 2.0f));
   OUT_RING(ring, A4XX_GRAS_CL_VPORT_ZOFFSET_0(0.0f));
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_WINDOW_SCISSOR_BR, 2);
   OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_BR_X(bin_w - 1) |
         OUT_RING(ring, A4XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_BR_X(bin_w - 1) |
            OUT_PKT0(ring, REG_A4XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MODE_CONTROL_WIDTH(gmem->bin_w) |
            OUT_PKT0(ring, REG_A4XX_RB_STENCIL_CONTROL, 2);
   OUT_RING(ring, A4XX_RB_STENCIL_CONTROL_FUNC(FUNC_ALWAYS) |
                     A4XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
   A4XX_RB_STENCIL_CONTROL_FUNC_BF(FUNC_ALWAYS) |
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                        OUT_PKT0(ring, REG_A4XX_PC_PRIM_VTX_CNTL, 1);
   OUT_RING(ring, A4XX_PC_PRIM_VTX_CNTL_PROVOKING_VTX_LAST |
            OUT_PKT0(ring, REG_A4XX_VFD_INDEX_OFFSET, 2);
   OUT_RING(ring, 0); /* VFD_INDEX_OFFSET */
                     /* for gmem pitch/base calculations, we need to use the non-
   * truncated tile sizes:
   */
   bin_w = gmem->bin_w;
            if (fd_gmem_needs_restore(batch, tile, FD_BUFFER_COLOR)) {
      fd4_gmem_emit_set_prog(ctx, &emit, &ctx->blit_prog[pfb->nr_cbufs - 1]);
   fd4_program_emit(ring, &emit, pfb->nr_cbufs, pfb->cbufs);
   emit_mem2gmem_surf(batch, gmem->cbuf_base, pfb->cbufs, pfb->nr_cbufs,
               if (fd_gmem_needs_restore(batch, tile,
            switch (pfb->zsbuf->format) {
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
   case PIPE_FORMAT_Z32_FLOAT:
      if (pfb->zsbuf->format == PIPE_FORMAT_Z32_FLOAT)
                        OUT_PKT0(ring, REG_A4XX_RB_DEPTH_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_DEPTH_CONTROL_Z_TEST_ENABLE |
                                                         default:
      /* Non-float can use a regular color write. It's split over 8-bit
   * components, so half precision is always sufficient.
   */
   fd4_gmem_emit_set_prog(ctx, &emit, &ctx->blit_prog[0]);
      }
   fd4_program_emit(ring, &emit, 1, &pfb->zsbuf);
               OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A4XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MODE_CONTROL_WIDTH(gmem->bin_w) |
            }
      static void
   patch_draws(struct fd_batch *batch, enum pc_di_vis_cull_mode vismode)
   {
      unsigned i;
   for (i = 0; i < fd_patch_num_elements(&batch->draw_patches); i++) {
      struct fd_cs_patch *patch = fd_patch_element(&batch->draw_patches, i);
      }
      }
      /* for rendering directly to system memory: */
   static void
   fd4_emit_sysmem_prep(struct fd_batch *batch) assert_dt
   {
      struct pipe_framebuffer_state *pfb = &batch->framebuffer;
                     OUT_PKT0(ring, REG_A4XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A4XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
                     /* setup scissor/offset for current tile: */
   OUT_PKT0(ring, REG_A4XX_RB_BIN_OFFSET, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_BR_X(pfb->width - 1) |
            OUT_PKT0(ring, REG_A4XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MODE_CONTROL_WIDTH(0) |
            OUT_PKT0(ring, REG_A4XX_RB_RENDER_CONTROL, 1);
               }
      static void
   update_vsc_pipe(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd4_context *fd4_ctx = fd4_context(ctx);
   struct fd_ringbuffer *ring = batch->gmem;
            OUT_PKT0(ring, REG_A4XX_VSC_SIZE_ADDRESS, 1);
            OUT_PKT0(ring, REG_A4XX_VSC_PIPE_CONFIG_REG(0), 8);
   for (i = 0; i < 8; i++) {
      const struct fd_vsc_pipe *pipe = &gmem->vsc_pipe[i];
   OUT_RING(ring, A4XX_VSC_PIPE_CONFIG_REG_X(pipe->x) |
                           OUT_PKT0(ring, REG_A4XX_VSC_PIPE_DATA_ADDRESS_REG(0), 8);
   for (i = 0; i < 8; i++) {
      if (!ctx->vsc_pipe_bo[i]) {
      ctx->vsc_pipe_bo[i] = fd_bo_new(
      }
   OUT_RELOC(ring, ctx->vsc_pipe_bo[i], 0, 0,
               OUT_PKT0(ring, REG_A4XX_VSC_PIPE_DATA_LENGTH_REG(0), 8);
   for (i = 0; i < 8; i++) {
      OUT_RING(ring, fd_bo_size(ctx->vsc_pipe_bo[i]) -
         }
      static void
   emit_binning_pass(struct fd_batch *batch) assert_dt
   {
      const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd_ringbuffer *ring = batch->gmem;
            uint32_t x1 = gmem->minx;
   uint32_t y1 = gmem->miny;
   uint32_t x2 = gmem->minx + gmem->width - 1;
            OUT_PKT0(ring, REG_A4XX_PC_BINNING_COMMAND, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_TILING_PASS) |
                        OUT_PKT0(ring, REG_A4XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A4XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
            /* setup scissor/offset for whole screen: */
   OUT_PKT0(ring, REG_A4XX_RB_BIN_OFFSET, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_BR_X(x2) |
            for (i = 0; i < A4XX_MAX_RENDER_TARGETS; i++) {
      OUT_PKT0(ring, REG_A4XX_RB_MRT_CONTROL(i), 1);
   OUT_RING(ring, A4XX_RB_MRT_CONTROL_ROP_CODE(ROP_CLEAR) |
               /* emit IB to binning drawcmds: */
            fd_reset_wfi(batch);
                     OUT_PKT0(ring, REG_A4XX_PC_BINNING_COMMAND, 1);
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A4XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                        fd_event_write(batch, ring, CACHE_FLUSH);
      }
      /* before first tile */
   static void
   fd4_emit_tile_init(struct fd_batch *batch) assert_dt
   {
      struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
                     OUT_PKT0(ring, REG_A4XX_VSC_BIN_SIZE, 1);
   OUT_RING(ring, A4XX_VSC_BIN_SIZE_WIDTH(gmem->bin_w) |
                     fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A4XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A4XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
            if (use_hw_binning(batch)) {
      OUT_PKT0(ring, REG_A4XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MODE_CONTROL_WIDTH(gmem->bin_w) |
            OUT_PKT0(ring, REG_A4XX_RB_RENDER_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_RENDER_CONTROL_BINNING_PASS |
            /* emit hw binning pass: */
               } else {
                  OUT_PKT0(ring, REG_A4XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A4XX_RB_MODE_CONTROL_WIDTH(gmem->bin_w) |
            }
      /* before mem2gmem */
   static void
   fd4_emit_tile_prep(struct fd_batch *batch, const struct fd_tile *tile)
   {
      struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            if (pfb->zsbuf) {
      struct fd_resource *rsc = fd_resource(pfb->zsbuf->texture);
            OUT_PKT0(ring, REG_A4XX_RB_DEPTH_INFO, 3);
   OUT_RING(ring, A4XX_RB_DEPTH_INFO_DEPTH_BASE(gmem->zsbuf_base[0]) |
               OUT_RING(ring, A4XX_RB_DEPTH_PITCH(cpp * gmem->bin_w));
            OUT_PKT0(ring, REG_A4XX_RB_STENCIL_INFO, 2);
   if (rsc->stencil) {
      OUT_RING(ring,
               OUT_RING(ring, A4XX_RB_STENCIL_PITCH(rsc->stencil->layout.cpp *
      } else {
      OUT_RING(ring, 0x00000000);
         } else {
      OUT_PKT0(ring, REG_A4XX_RB_DEPTH_INFO, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
            OUT_PKT0(ring, REG_A4XX_RB_STENCIL_INFO, 2);
   OUT_RING(ring, 0); /* RB_STENCIL_INFO */
               OUT_PKT0(ring, REG_A4XX_GRAS_DEPTH_CONTROL, 1);
   if (pfb->zsbuf) {
      OUT_RING(ring, A4XX_GRAS_DEPTH_CONTROL_FORMAT(
      } else {
            }
      /* before IB to rendering cmds: */
   static void
   fd4_emit_tile_renderprep(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd4_context *fd4_ctx = fd4_context(ctx);
   struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
            uint32_t x1 = tile->xoff;
   uint32_t y1 = tile->yoff;
   uint32_t x2 = tile->xoff + tile->bin_w - 1;
            if (use_hw_binning(batch)) {
      const struct fd_vsc_pipe *pipe = &gmem->vsc_pipe[tile->p];
                     fd_event_write(batch, ring, HLSQ_FLUSH);
            OUT_PKT0(ring, REG_A4XX_PC_VSTREAM_CONTROL, 1);
   OUT_RING(ring, A4XX_PC_VSTREAM_CONTROL_SIZE(pipe->w * pipe->h) |
            OUT_PKT3(ring, CP_SET_BIN_DATA, 2);
   OUT_RELOC(ring, pipe_bo, 0, 0,
         OUT_RELOC(ring, fd4_ctx->vsc_size_mem, /* BIN_SIZE_ADDR <-
            } else {
      OUT_PKT0(ring, REG_A4XX_PC_VSTREAM_CONTROL, 1);
               OUT_PKT3(ring, CP_SET_BIN, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, CP_SET_BIN_1_X1(x1) | CP_SET_BIN_1_Y1(y1));
            emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, gmem->cbuf_base, gmem->bin_w,
            /* setup scissor/offset for current tile: */
   OUT_PKT0(ring, REG_A4XX_RB_BIN_OFFSET, 1);
   OUT_RING(ring, A4XX_RB_BIN_OFFSET_X(tile->xoff) |
            OUT_PKT0(ring, REG_A4XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A4XX_GRAS_SC_SCREEN_SCISSOR_BR_X(x2) |
            OUT_PKT0(ring, REG_A4XX_RB_RENDER_CONTROL, 1);
      }
      void
   fd4_gmem_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->emit_sysmem_prep = fd4_emit_sysmem_prep;
   ctx->emit_tile_init = fd4_emit_tile_init;
   ctx->emit_tile_prep = fd4_emit_tile_prep;
   ctx->emit_tile_mem2gmem = fd4_emit_tile_mem2gmem;
   ctx->emit_tile_renderprep = fd4_emit_tile_renderprep;
      }
