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
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_draw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd3_context.h"
   #include "fd3_emit.h"
   #include "fd3_format.h"
   #include "fd3_gmem.h"
   #include "fd3_program.h"
   #include "fd3_zsa.h"
      static void
   fd3_gmem_emit_set_prog(struct fd_context *ctx, struct fd3_emit *emit,
         {
      emit->skip_consts = true;
   emit->key.vs = prog->vs;
   emit->key.fs = prog->fs;
   emit->prog = fd3_program_state(
         /* reset the fd3_emit_get_*p cache */
   emit->vs = NULL;
      }
      static void
   emit_mrt(struct fd_ringbuffer *ring, unsigned nr_bufs,
               {
      enum a3xx_tile_mode tile_mode;
            for (i = 0; i < A3XX_MAX_RENDER_TARGETS; i++) {
      enum pipe_format pformat = 0;
   enum a3xx_color_fmt format = 0;
   enum a3xx_color_swap swap = WZYX;
   bool srgb = false;
   struct fd_resource *rsc = NULL;
   uint32_t stride = 0;
   uint32_t base = 0;
            if (bin_w) {
         } else {
                  if ((i < nr_bufs) && bufs[i]) {
               rsc = fd_resource(psurf->texture);
   pformat = psurf->format;
   /* In case we're drawing to Z32F_S8, the "color" actually goes to
   * the stencil
   */
   if (rsc->stencil) {
      rsc = rsc->stencil;
   pformat = rsc->b.b.format;
   if (bases)
      }
   format = fd3_pipe2color(pformat);
   if (decode_srgb)
                                 offset = fd_resource_offset(rsc, psurf->u.tex.level,
                                    if (bases) {
            } else {
      stride = fd_resource_pitch(rsc, psurf->u.tex.level);
         } else if (i < nr_bufs && bases) {
                  OUT_PKT0(ring, REG_A3XX_RB_MRT_BUF_INFO(i), 2);
   OUT_RING(ring, A3XX_RB_MRT_BUF_INFO_COLOR_FORMAT(format) |
                     A3XX_RB_MRT_BUF_INFO_COLOR_TILE_MODE(tile_mode) |
   if (bin_w || (i >= nr_bufs) || !bufs[i]) {
         } else {
                  OUT_PKT0(ring, REG_A3XX_SP_FS_IMAGE_OUTPUT_REG(i), 1);
   OUT_RING(ring, COND((i < nr_bufs) && bufs[i],
               }
      static bool
   use_hw_binning(struct fd_batch *batch)
   {
               /* workaround: combining scissor optimization and hw binning
   * seems problematic.  Seems like we end up with a mismatch
   * between binning pass and rendering pass, wrt. where the hw
   * thinks the vertices belong.  And the blob driver doesn't
   * seem to implement anything like scissor optimization, so
   * not entirely sure what I might be missing.
   *
   * But scissor optimization is mainly for window managers,
   * which don't have many vertices (and therefore doesn't
   * benefit much from binning pass).
   *
   * So for now just disable binning if scissor optimization is
   * used.
   */
   if (gmem->minx || gmem->miny)
            if ((gmem->maxpw * gmem->maxph) > 32)
            if ((gmem->maxpw > 15) || (gmem->maxph > 15))
               }
      /* workaround for (hlsq?) lockup with hw binning on a3xx patchlevel 0 */
   static void update_vsc_pipe(struct fd_batch *batch);
   static void
   emit_binning_workaround(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
   struct fd3_emit emit = {
      .debug = &ctx->debug,
   .vtx = &ctx->solid_vbuf_state,
   .key =
      {
      .vs = ctx->solid_prog.vs,
                        OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 2);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
               OUT_RING(ring, A3XX_RB_RENDER_CONTROL_BIN_WIDTH(32) |
                  OUT_PKT0(ring, REG_A3XX_RB_COPY_CONTROL, 4);
   OUT_RING(ring, A3XX_RB_COPY_CONTROL_MSAA_RESOLVE(MSAA_ONE) |
               OUT_RELOC(ring, fd_resource(ctx->solid_vbuf)->bo, 0x20, 0,
         OUT_RING(ring, A3XX_RB_COPY_DEST_PITCH_PITCH(128));
   OUT_RING(ring, A3XX_RB_COPY_DEST_INFO_TILE(LINEAR) |
                              OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
                  fd3_program_emit(ring, &emit, 0, NULL);
            OUT_PKT0(ring, REG_A3XX_HLSQ_CONTROL_0_REG, 4);
   OUT_RING(ring, A3XX_HLSQ_CONTROL_0_REG_FSTHREADSIZE(FOUR_QUADS) |
                     OUT_RING(ring, A3XX_HLSQ_CONTROL_1_REG_VSTHREADSIZE(TWO_QUADS) |
         OUT_RING(ring, A3XX_HLSQ_CONTROL_2_REG_PRIMALLOCTHRESHOLD(31));
            OUT_PKT0(ring, REG_A3XX_HLSQ_CONST_FSPRESV_RANGE_REG, 1);
   OUT_RING(ring, A3XX_HLSQ_CONST_FSPRESV_RANGE_REG_STARTENTRY(0x20) |
            OUT_PKT0(ring, REG_A3XX_RB_MSAA_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MSAA_CONTROL_DISABLE |
                  OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_NEVER) |
                     A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
            OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
   OUT_RING(ring, 0); /* VFD_INDEX_MIN */
   OUT_RING(ring, 2); /* VFD_INDEX_MAX */
   OUT_RING(ring, 0); /* VFD_INSTANCEID_OFFSET */
            OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
   OUT_RING(ring,
            A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(0) |
               OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(0) |
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(31) |
            fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XOFFSET(0.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE(1.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YOFFSET(0.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(1.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(0.0f));
            OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
   OUT_RING(ring, A3XX_GRAS_CL_CLIP_CNTL_CLIP_DISABLE |
                              OUT_PKT0(ring, REG_A3XX_GRAS_CL_GB_CLIP_ADJ, 1);
   OUT_RING(ring, A3XX_GRAS_CL_GB_CLIP_ADJ_HORZ(0) |
            OUT_PKT3(ring, CP_DRAW_INDX_2, 5);
   OUT_RING(ring, 0x00000000); /* viz query info. */
   OUT_RING(ring, DRAW(DI_PT_RECTLIST, DI_SRC_SEL_IMMEDIATE, INDEX_SIZE_32_BIT,
         OUT_RING(ring, 2); /* NumIndices */
   OUT_RING(ring, 2);
   OUT_RING(ring, 1);
            OUT_PKT0(ring, REG_A3XX_HLSQ_CONTROL_0_REG, 1);
            OUT_PKT0(ring, REG_A3XX_VFD_PERFCOUNTER0_SELECT, 1);
            fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_VSC_BIN_SIZE, 1);
   OUT_RING(ring, A3XX_VSC_BIN_SIZE_WIDTH(gmem->bin_w) |
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
      }
      /* transfer from gmem to system memory (ie. normal RAM) */
      static void
   emit_gmem2mem_surf(struct fd_batch *batch,
               {
      struct fd_ringbuffer *ring = batch->gmem;
   struct fd_resource *rsc = fd_resource(psurf->texture);
            if (!rsc->valid)
            if (stencil) {
      rsc = rsc->stencil;
               uint32_t offset =
                           OUT_PKT0(ring, REG_A3XX_RB_COPY_CONTROL, 4);
   OUT_RING(ring, A3XX_RB_COPY_CONTROL_MSAA_RESOLVE(MSAA_ONE) |
                     A3XX_RB_COPY_CONTROL_MODE(mode) |
            OUT_RELOC(ring, rsc->bo, offset, 0, -1); /* RB_COPY_DEST_BASE */
   OUT_RING(ring, A3XX_RB_COPY_DEST_PITCH_PITCH(pitch));
   OUT_RING(ring, A3XX_RB_COPY_DEST_INFO_TILE(rsc->layout.tile_mode) |
                              fd_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   fd3_emit_tile_gmem2mem(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd3_emit emit = {.debug = &ctx->debug,
                           .vtx = &ctx->solid_vbuf_state,
            emit.prog = fd3_program_state(
            OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_NEVER) |
                     A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
            OUT_PKT0(ring, REG_A3XX_RB_STENCILREFMASK, 2);
   OUT_RING(ring, 0xff000000 | A3XX_RB_STENCILREFMASK_STENCILREF(0) |
               OUT_RING(ring, 0xff000000 | A3XX_RB_STENCILREFMASK_STENCILREF(0) |
                  OUT_PKT0(ring, REG_A3XX_GRAS_SU_MODE_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
            fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XOFFSET((float)pfb->width / 2.0f - 0.5f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE((float)pfb->width / 2.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YOFFSET((float)pfb->height / 2.0f - 0.5f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(-(float)pfb->height / 2.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(0.0f));
            OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
                  OUT_PKT0(ring, REG_A3XX_RB_RENDER_CONTROL, 1);
   OUT_RING(ring,
            A3XX_RB_RENDER_CONTROL_DISABLE_COLOR_PIPE |
               OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RESOLVE_PASS) |
                  OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
   OUT_RING(ring,
            A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(0) |
               OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(pfb->width - 1) |
            OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
   OUT_RING(ring, 0); /* VFD_INDEX_MIN */
   OUT_RING(ring, 2); /* VFD_INDEX_MAX */
   OUT_RING(ring, 0); /* VFD_INSTANCEID_OFFSET */
            fd3_program_emit(ring, &emit, 0, NULL);
            if (batch->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) {
      struct fd_resource *rsc = fd_resource(pfb->zsbuf->texture);
   if (!rsc->stencil || batch->resolve & FD_BUFFER_DEPTH)
      emit_gmem2mem_surf(batch, RB_COPY_DEPTH_STENCIL, false,
      if (rsc->stencil && batch->resolve & FD_BUFFER_STENCIL)
      emit_gmem2mem_surf(batch, RB_COPY_DEPTH_STENCIL, true,
            if (batch->resolve & FD_BUFFER_COLOR) {
      for (i = 0; i < pfb->nr_cbufs; i++) {
      if (!pfb->cbufs[i])
         if (!(batch->resolve & (PIPE_CLEAR_COLOR0 << i)))
         emit_gmem2mem_surf(batch, RB_COPY_RESOLVE, false, gmem->cbuf_base[i],
                  OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
            }
      /* transfer from system memory to gmem */
      static void
   emit_mem2gmem_surf(struct fd_batch *batch, const uint32_t bases[],
         {
      struct fd_ringbuffer *ring = batch->gmem;
                     OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                           if (psurf[0] && (psurf[0]->format == PIPE_FORMAT_Z32_FLOAT ||
            /* Depth is stored as unorm in gmem, so we have to write it in using a
   * special blit shader which writes depth.
   */
   OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
   OUT_RING(ring, (A3XX_RB_DEPTH_CONTROL_FRAG_WRITES_Z |
                              OUT_PKT0(ring, REG_A3XX_RB_DEPTH_INFO, 2);
   OUT_RING(ring, A3XX_RB_DEPTH_INFO_DEPTH_BASE(bases[0]) |
                  if (psurf[0]->format == PIPE_FORMAT_Z32_FLOAT) {
      OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(0), 1);
      } else {
      /* The gmem_restore_tex logic will put the first buffer's stencil
   * as color. Supply it with the proper information to make that
   * happen.
   */
   zsbufs[0] = zsbufs[1] = psurf[0];
   psurf = zsbufs;
         } else {
      OUT_PKT0(ring, REG_A3XX_SP_FS_OUTPUT_REG, 1);
                        fd_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   fd3_emit_tile_mem2gmem(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd3_emit emit = {
      .debug = &ctx->debug,
   .vtx = &ctx->blit_vbuf_state,
      };
   /* NOTE: They all use the same VP, this is for vtx bufs. */
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
                     for (i = 0; i < 4; i++) {
      OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(i), 1);
   OUT_RING(ring, A3XX_RB_MRT_CONTROL_ROP_CODE(ROP_COPY) |
                  OUT_PKT0(ring, REG_A3XX_RB_MRT_BLEND_CONTROL(i), 1);
   OUT_RING(
      ring,
   A3XX_RB_MRT_BLEND_CONTROL_RGB_SRC_FACTOR(FACTOR_ONE) |
      A3XX_RB_MRT_BLEND_CONTROL_RGB_BLEND_OPCODE(BLEND_DST_PLUS_SRC) |
   A3XX_RB_MRT_BLEND_CONTROL_RGB_DEST_FACTOR(FACTOR_ZERO) |
   A3XX_RB_MRT_BLEND_CONTROL_ALPHA_SRC_FACTOR(FACTOR_ONE) |
               OUT_PKT0(ring, REG_A3XX_RB_RENDER_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_RENDER_CONTROL_ALPHA_TEST_FUNC(FUNC_ALWAYS) |
            fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_RB_DEPTH_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_RB_DEPTH_INFO, 2);
   OUT_RING(ring, 0);
            OUT_PKT0(ring, REG_A3XX_GRAS_CL_CLIP_CNTL, 1);
   OUT_RING(ring,
            fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_GRAS_CL_VPORT_XOFFSET, 6);
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XOFFSET((float)bin_w / 2.0f - 0.5f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_XSCALE((float)bin_w / 2.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YOFFSET((float)bin_h / 2.0f - 0.5f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_YSCALE(-(float)bin_h / 2.0f));
   OUT_RING(ring, A3XX_GRAS_CL_VPORT_ZOFFSET(0.0f));
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_WINDOW_SCISSOR_BR_X(bin_w - 1) |
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(bin_w - 1) |
            OUT_PKT0(ring, REG_A3XX_RB_STENCIL_CONTROL, 1);
   OUT_RING(ring, 0x2 | A3XX_RB_STENCIL_CONTROL_FUNC(FUNC_ALWAYS) |
                     A3XX_RB_STENCIL_CONTROL_FAIL(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZPASS(STENCIL_KEEP) |
   A3XX_RB_STENCIL_CONTROL_ZFAIL(STENCIL_KEEP) |
            OUT_PKT0(ring, REG_A3XX_RB_STENCIL_INFO, 2);
   OUT_RING(ring, 0); /* RB_STENCIL_INFO */
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_PC_PRIM_VTX_CNTL, 1);
   OUT_RING(ring,
            A3XX_PC_PRIM_VTX_CNTL_STRIDE_IN_VPC(2) |
               OUT_PKT0(ring, REG_A3XX_VFD_INDEX_MIN, 4);
   OUT_RING(ring, 0); /* VFD_INDEX_MIN */
   OUT_RING(ring, 2); /* VFD_INDEX_MAX */
   OUT_RING(ring, 0); /* VFD_INSTANCEID_OFFSET */
                     /* for gmem pitch/base calculations, we need to use the non-
   * truncated tile sizes:
   */
   bin_w = gmem->bin_w;
            if (fd_gmem_needs_restore(batch, tile, FD_BUFFER_COLOR)) {
      fd3_gmem_emit_set_prog(ctx, &emit, &ctx->blit_prog[pfb->nr_cbufs - 1]);
   fd3_program_emit(ring, &emit, pfb->nr_cbufs, pfb->cbufs);
   emit_mem2gmem_surf(batch, gmem->cbuf_base, pfb->cbufs, pfb->nr_cbufs,
               if (fd_gmem_needs_restore(batch, tile,
            if (pfb->zsbuf->format != PIPE_FORMAT_Z32_FLOAT_S8X24_UINT &&
      pfb->zsbuf->format != PIPE_FORMAT_Z32_FLOAT) {
   /* Non-float can use a regular color write. It's split over 8-bit
   * components, so half precision is always sufficient.
   */
      } else {
      /* Float depth needs special blit shader that writes depth */
   if (pfb->zsbuf->format == PIPE_FORMAT_Z32_FLOAT)
         else
      }
   fd3_program_emit(ring, &emit, 1, &pfb->zsbuf);
               OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
            }
      static void
   patch_draws(struct fd_batch *batch, enum pc_di_vis_cull_mode vismode)
   {
      unsigned i;
   for (i = 0; i < fd_patch_num_elements(&batch->draw_patches); i++) {
      struct fd_cs_patch *patch = fd_patch_element(&batch->draw_patches, i);
      }
      }
      static void
   patch_rbrc(struct fd_batch *batch, uint32_t val)
   {
      unsigned i;
   for (i = 0; i < fd_patch_num_elements(&batch->rbrc_patches); i++) {
      struct fd_cs_patch *patch = fd_patch_element(&batch->rbrc_patches, i);
      }
      }
      /* for rendering directly to system memory: */
   static void
   fd3_emit_sysmem_prep(struct fd_batch *batch) assert_dt
   {
      struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd_ringbuffer *ring = batch->gmem;
            for (i = 0; i < pfb->nr_cbufs; i++) {
      struct pipe_surface *psurf = pfb->cbufs[i];
   if (!psurf)
         struct fd_resource *rsc = fd_resource(psurf->texture);
                        OUT_PKT0(ring, REG_A3XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A3XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
                     /* setup scissor/offset for current tile: */
   OUT_PKT0(ring, REG_A3XX_RB_WINDOW_OFFSET, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(pfb->width - 1) |
            OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                        patch_draws(batch, IGNORE_VISIBILITY);
      }
      static void
   update_vsc_pipe(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd3_context *fd3_ctx = fd3_context(ctx);
   struct fd_ringbuffer *ring = batch->gmem;
            OUT_PKT0(ring, REG_A3XX_VSC_SIZE_ADDRESS, 1);
            for (i = 0; i < 8; i++) {
               if (!ctx->vsc_pipe_bo[i]) {
      ctx->vsc_pipe_bo[i] = fd_bo_new(
               OUT_PKT0(ring, REG_A3XX_VSC_PIPE(i), 3);
   OUT_RING(ring, A3XX_VSC_PIPE_CONFIG_X(pipe->x) |
                     OUT_RELOC(ring, ctx->vsc_pipe_bo[i], 0, 0,
         OUT_RING(ring, fd_bo_size(ctx->vsc_pipe_bo[i]) -
         }
      static void
   emit_binning_pass(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   struct fd_ringbuffer *ring = batch->gmem;
            uint32_t x1 = gmem->minx;
   uint32_t y1 = gmem->miny;
   uint32_t x2 = gmem->minx + gmem->width - 1;
            if (ctx->screen->gpu_id == 320) {
      emit_binning_workaround(batch);
   fd_wfi(batch, ring);
   OUT_PKT3(ring, CP_INVALIDATE_STATE, 1);
               OUT_PKT0(ring, REG_A3XX_VSC_BIN_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_TILING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A3XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
            OUT_PKT0(ring, REG_A3XX_RB_RENDER_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_RENDER_CONTROL_ALPHA_TEST_FUNC(FUNC_NEVER) |
                  /* setup scissor/offset for whole screen: */
   OUT_PKT0(ring, REG_A3XX_RB_WINDOW_OFFSET, 1);
            OUT_PKT0(ring, REG_A3XX_RB_LRZ_VSC_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(x2) |
            OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_TILING_PASS) |
                  for (i = 0; i < 4; i++) {
      OUT_PKT0(ring, REG_A3XX_RB_MRT_CONTROL(i), 1);
   OUT_RING(ring, A3XX_RB_MRT_CONTROL_ROP_CODE(ROP_CLEAR) |
                     OUT_PKT0(ring, REG_A3XX_PC_VSTREAM_CONTROL, 1);
   OUT_RING(ring,
            /* emit IB to binning drawcmds: */
   fd3_emit_ib(ring, batch->binning);
                              OUT_PKT0(ring, REG_A3XX_VSC_BIN_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_SP_SP_CTRL_REG, 1);
   OUT_RING(ring, A3XX_SP_SP_CTRL_REG_RESOLVE |
                        OUT_PKT0(ring, REG_A3XX_RB_LRZ_VSC_CONTROL, 1);
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_CONTROL, 1);
   OUT_RING(ring, A3XX_GRAS_SC_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
                  OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 2);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
               OUT_RING(ring, A3XX_RB_RENDER_CONTROL_ENABLE_GMEM |
                  fd_event_write(batch, ring, CACHE_FLUSH);
            if (ctx->screen->gpu_id == 320) {
      /* dummy-draw workaround: */
   OUT_PKT3(ring, CP_DRAW_INDX, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, DRAW(1, DI_SRC_SEL_AUTO_INDEX, INDEX_SIZE_IGN,
         OUT_RING(ring, 0); /* NumIndices */
               OUT_PKT3(ring, CP_NOP, 4);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x00000000);
                     if (ctx->screen->gpu_id == 320) {
            }
      /* before first tile */
   static void
   fd3_emit_tile_init(struct fd_batch *batch) assert_dt
   {
      struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
                     /* note: use gmem->bin_w/h, the bin_w/h parameters may be truncated
   * at the right and bottom edge tiles
   */
   OUT_PKT0(ring, REG_A3XX_VSC_BIN_SIZE, 1);
   OUT_RING(ring, A3XX_VSC_BIN_SIZE_WIDTH(gmem->bin_w) |
                     fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A3XX_RB_FRAME_BUFFER_DIMENSION, 1);
   OUT_RING(ring, A3XX_RB_FRAME_BUFFER_DIMENSION_WIDTH(pfb->width) |
            if (use_hw_binning(batch)) {
      /* emit hw binning pass: */
               } else {
                  rb_render_control = A3XX_RB_RENDER_CONTROL_ENABLE_GMEM |
               }
      /* before mem2gmem */
   static void
   fd3_emit_tile_prep(struct fd_batch *batch, const struct fd_tile *tile)
   {
      struct fd_ringbuffer *ring = batch->gmem;
            OUT_PKT0(ring, REG_A3XX_RB_MODE_CONTROL, 1);
   OUT_RING(ring, A3XX_RB_MODE_CONTROL_RENDER_MODE(RB_RENDERING_PASS) |
            }
      /* before IB to rendering cmds: */
   static void
   fd3_emit_tile_renderprep(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd3_context *fd3_ctx = fd3_context(ctx);
   struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
            uint32_t x1 = tile->xoff;
   uint32_t y1 = tile->yoff;
   uint32_t x2 = tile->xoff + tile->bin_w - 1;
                     OUT_PKT0(ring, REG_A3XX_RB_DEPTH_INFO, 2);
   reg = A3XX_RB_DEPTH_INFO_DEPTH_BASE(gmem->zsbuf_base[0]);
   if (pfb->zsbuf) {
         }
   OUT_RING(ring, reg);
   if (pfb->zsbuf) {
      struct fd_resource *rsc = fd_resource(pfb->zsbuf->texture);
   OUT_RING(ring,
         if (rsc->stencil) {
      OUT_PKT0(ring, REG_A3XX_RB_STENCIL_INFO, 2);
   OUT_RING(ring, A3XX_RB_STENCIL_INFO_STENCIL_BASE(gmem->zsbuf_base[1]));
   OUT_RING(ring, A3XX_RB_STENCIL_PITCH(gmem->bin_w << fdl_cpp_shift(
         } else {
                  if (use_hw_binning(batch)) {
      const struct fd_vsc_pipe *pipe = &gmem->vsc_pipe[tile->p];
                     fd_event_write(batch, ring, HLSQ_FLUSH);
            OUT_PKT0(ring, REG_A3XX_PC_VSTREAM_CONTROL, 1);
   OUT_RING(ring, A3XX_PC_VSTREAM_CONTROL_SIZE(pipe->w * pipe->h) |
            OUT_PKT3(ring, CP_SET_BIN_DATA, 2);
   OUT_RELOC(ring, pipe_bo, 0, 0,
         OUT_RELOC(ring, fd3_ctx->vsc_size_mem, /* BIN_SIZE_ADDR <-
            } else {
      OUT_PKT0(ring, REG_A3XX_PC_VSTREAM_CONTROL, 1);
               OUT_PKT3(ring, CP_SET_BIN, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, CP_SET_BIN_1_X1(x1) | CP_SET_BIN_1_Y1(y1));
            emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, gmem->cbuf_base, gmem->bin_w,
            /* setup scissor/offset for current tile: */
   OUT_PKT0(ring, REG_A3XX_RB_WINDOW_OFFSET, 1);
   OUT_RING(ring, A3XX_RB_WINDOW_OFFSET_X(tile->xoff) |
            OUT_PKT0(ring, REG_A3XX_GRAS_SC_SCREEN_SCISSOR_TL, 2);
   OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A3XX_GRAS_SC_SCREEN_SCISSOR_BR_X(x2) |
      }
      void
   fd3_gmem_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->emit_sysmem_prep = fd3_emit_sysmem_prep;
   ctx->emit_tile_init = fd3_emit_tile_init;
   ctx->emit_tile_prep = fd3_emit_tile_prep;
   ctx->emit_tile_mem2gmem = fd3_emit_tile_mem2gmem;
   ctx->emit_tile_renderprep = fd3_emit_tile_renderprep;
      }
