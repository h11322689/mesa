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
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_draw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "fd5_context.h"
   #include "fd5_draw.h"
   #include "fd5_emit.h"
   #include "fd5_format.h"
   #include "fd5_gmem.h"
   #include "fd5_program.h"
   #include "fd5_zsa.h"
      static void
   emit_mrt(struct fd_ringbuffer *ring, unsigned nr_bufs,
         {
      enum a5xx_tile_mode tile_mode;
            for (i = 0; i < A5XX_MAX_RENDER_TARGETS; i++) {
      enum a5xx_color_fmt format = 0;
   enum a3xx_color_swap swap = WZYX;
   bool srgb = false, sint = false, uint = false;
   struct fd_resource *rsc = NULL;
   uint32_t stride = 0;
   uint32_t size = 0;
   uint32_t base = 0;
            if (gmem) {
         } else {
                  if ((i < nr_bufs) && bufs[i]) {
                              format = fd5_pipe2color(pformat);
   swap = fd5_pipe2swap(pformat);
   srgb = util_format_is_srgb(pformat);
                                          if (gmem) {
      stride = gmem->bin_w * gmem->cbuf_cpp[i];
   size = stride * gmem->bin_h;
      } else {
                     tile_mode =
                  OUT_PKT4(ring, REG_A5XX_RB_MRT_BUF_INFO(i), 5);
   OUT_RING(
      ring,
   A5XX_RB_MRT_BUF_INFO_COLOR_FORMAT(format) |
      A5XX_RB_MRT_BUF_INFO_COLOR_TILE_MODE(tile_mode) |
   A5XX_RB_MRT_BUF_INFO_COLOR_SWAP(swap) |
   COND(gmem,
         OUT_RING(ring, A5XX_RB_MRT_PITCH(stride));
   OUT_RING(ring, A5XX_RB_MRT_ARRAY_PITCH(size));
   if (gmem || (i >= nr_bufs) || !bufs[i]) {
      OUT_RING(ring, base);       /* RB_MRT[i].BASE_LO */
      } else {
                  OUT_PKT4(ring, REG_A5XX_SP_FS_MRT_REG(i), 1);
   OUT_RING(ring, A5XX_SP_FS_MRT_REG_COLOR_FORMAT(format) |
                        /* when we support UBWC, these would be the system memory
   * addr/pitch/etc:
   */
   OUT_PKT4(ring, REG_A5XX_RB_MRT_FLAG_BUFFER(i), 4);
   OUT_RING(ring, 0x00000000); /* RB_MRT_FLAG_BUFFER[i].ADDR_LO */
   OUT_RING(ring, 0x00000000); /* RB_MRT_FLAG_BUFFER[i].ADDR_HI */
   OUT_RING(ring, A5XX_RB_MRT_FLAG_BUFFER_PITCH(0));
         }
      static void
   emit_zs(struct fd_ringbuffer *ring, struct pipe_surface *zsbuf,
         {
      if (zsbuf) {
      struct fd_resource *rsc = fd_resource(zsbuf->texture);
   enum a5xx_depth_format fmt = fd5_pipe2depth(zsbuf->format);
   uint32_t cpp = rsc->layout.cpp;
   uint32_t stride = 0;
            if (gmem) {
      stride = cpp * gmem->bin_w;
      } else {
      stride = fd_resource_pitch(rsc, zsbuf->u.tex.level);
               OUT_PKT4(ring, REG_A5XX_RB_DEPTH_BUFFER_INFO, 5);
   OUT_RING(ring, A5XX_RB_DEPTH_BUFFER_INFO_DEPTH_FORMAT(fmt));
   if (gmem) {
      OUT_RING(ring, gmem->zsbuf_base[0]); /* RB_DEPTH_BUFFER_BASE_LO */
      } else {
      OUT_RELOC(ring, rsc->bo,
      fd_resource_offset(rsc, zsbuf->u.tex.level, zsbuf->u.tex.first_layer),
   }
   OUT_RING(ring, A5XX_RB_DEPTH_BUFFER_PITCH(stride));
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_DEPTH_BUFFER_INFO, 1);
            OUT_PKT4(ring, REG_A5XX_RB_DEPTH_FLAG_BUFFER_BASE_LO, 3);
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_FLAG_BUFFER_BASE_LO */
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_FLAG_BUFFER_BASE_HI */
            if (rsc->lrz) {
      OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_BUFFER_BASE_LO, 3);
                  OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_FAST_CLEAR_BUFFER_BASE_LO, 2);
      } else {
      OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_BUFFER_BASE_LO, 3);
   OUT_RING(ring, 0x00000000);
                  OUT_PKT4(ring, REG_A5XX_GRAS_LRZ_FAST_CLEAR_BUFFER_BASE_LO, 2);
   OUT_RING(ring, 0x00000000);
               if (rsc->stencil) {
      if (gmem) {
      stride = 1 * gmem->bin_w;
      } else {
      stride = fd_resource_pitch(rsc->stencil, zsbuf->u.tex.level);
               OUT_PKT4(ring, REG_A5XX_RB_STENCIL_INFO, 5);
   OUT_RING(ring, A5XX_RB_STENCIL_INFO_SEPARATE_STENCIL);
   if (gmem) {
      OUT_RING(ring, gmem->zsbuf_base[1]); /* RB_STENCIL_BASE_LO */
      } else {
      OUT_RELOC(ring, rsc->stencil->bo,
      fd_resource_offset(rsc->stencil, zsbuf->u.tex.level, zsbuf->u.tex.first_layer),
   }
   OUT_RING(ring, A5XX_RB_STENCIL_PITCH(stride));
      } else {
      OUT_PKT4(ring, REG_A5XX_RB_STENCIL_INFO, 1);
         } else {
      OUT_PKT4(ring, REG_A5XX_RB_DEPTH_BUFFER_INFO, 5);
   OUT_RING(ring, A5XX_RB_DEPTH_BUFFER_INFO_DEPTH_FORMAT(DEPTH5_NONE));
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_BUFFER_BASE_LO */
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_BUFFER_BASE_HI */
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_BUFFER_PITCH */
            OUT_PKT4(ring, REG_A5XX_GRAS_SU_DEPTH_BUFFER_INFO, 1);
            OUT_PKT4(ring, REG_A5XX_RB_DEPTH_FLAG_BUFFER_BASE_LO, 3);
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_FLAG_BUFFER_BASE_LO */
   OUT_RING(ring, 0x00000000); /* RB_DEPTH_FLAG_BUFFER_BASE_HI */
            OUT_PKT4(ring, REG_A5XX_RB_STENCIL_INFO, 1);
         }
      static void
   emit_msaa(struct fd_ringbuffer *ring, uint32_t nr_samples)
   {
               OUT_PKT4(ring, REG_A5XX_TPL1_TP_RAS_MSAA_CNTL, 2);
   OUT_RING(ring, A5XX_TPL1_TP_RAS_MSAA_CNTL_SAMPLES(samples));
   OUT_RING(ring, A5XX_TPL1_TP_DEST_MSAA_CNTL_SAMPLES(samples) |
                  OUT_PKT4(ring, REG_A5XX_RB_RAS_MSAA_CNTL, 2);
   OUT_RING(ring, A5XX_RB_RAS_MSAA_CNTL_SAMPLES(samples));
   OUT_RING(ring,
                  OUT_PKT4(ring, REG_A5XX_GRAS_SC_RAS_MSAA_CNTL, 2);
   OUT_RING(ring, A5XX_GRAS_SC_RAS_MSAA_CNTL_SAMPLES(samples));
   OUT_RING(ring, A5XX_GRAS_SC_DEST_MSAA_CNTL_SAMPLES(samples) |
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
            return fd_binning_enabled && ((gmem->nbins_x * gmem->nbins_y) > 2) &&
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
   update_vsc_pipe(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct fd5_context *fd5_ctx = fd5_context(ctx);
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
            OUT_PKT4(ring, REG_A5XX_VSC_BIN_SIZE, 3);
   OUT_RING(ring, A5XX_VSC_BIN_SIZE_WIDTH(gmem->bin_w) |
                  OUT_PKT4(ring, REG_A5XX_UNKNOWN_0BC5, 2);
   OUT_RING(ring, 0x00000000); /* UNKNOWN_0BC5 */
            OUT_PKT4(ring, REG_A5XX_VSC_PIPE_CONFIG_REG(0), 16);
   for (i = 0; i < 16; i++) {
      const struct fd_vsc_pipe *pipe = &gmem->vsc_pipe[i];
   OUT_RING(ring, A5XX_VSC_PIPE_CONFIG_REG_X(pipe->x) |
                           OUT_PKT4(ring, REG_A5XX_VSC_PIPE_DATA_ADDRESS_LO(0), 32);
   for (i = 0; i < 16; i++) {
      if (!ctx->vsc_pipe_bo[i]) {
      ctx->vsc_pipe_bo[i] = fd_bo_new(
      }
   OUT_RELOC(ring, ctx->vsc_pipe_bo[i], 0, 0,
               OUT_PKT4(ring, REG_A5XX_VSC_PIPE_DATA_LENGTH_REG(0), 16);
   for (i = 0; i < 16; i++) {
      OUT_RING(ring, fd_bo_size(ctx->vsc_pipe_bo[i]) -
         }
      static void
   emit_binning_pass(struct fd_batch *batch) assert_dt
   {
      struct fd_ringbuffer *ring = batch->gmem;
            uint32_t x1 = gmem->minx;
   uint32_t y1 = gmem->miny;
   uint32_t x2 = gmem->minx + gmem->width - 1;
                     OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
   OUT_RING(ring,
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_BR_X(x2) |
            OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_1, 2);
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_1_X(x1) | A5XX_RB_RESOLVE_CNTL_1_Y(y1));
                     OUT_PKT4(ring, REG_A5XX_VPC_MODE_CNTL, 1);
                     OUT_PKT4(ring, REG_A5XX_RB_WINDOW_OFFSET, 1);
            /* emit IB to binning drawcmds: */
                                                         OUT_PKT4(ring, REG_A5XX_VPC_MODE_CNTL, 1);
      }
      /* before first tile */
   static void
   fd5_emit_tile_init(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct fd_ringbuffer *ring = batch->gmem;
                     if (batch->prologue)
                     OUT_PKT4(ring, REG_A5XX_GRAS_CL_CNTL, 1);
            OUT_PKT7(ring, CP_SKIP_IB2_ENABLE_GLOBAL, 1);
            OUT_PKT4(ring, REG_A5XX_PC_POWER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VFD_POWER_CNTL, 1);
            /* 0x10000000 for BYPASS.. 0x7c13c080 for GMEM: */
   fd_wfi(batch, ring);
   OUT_PKT4(ring, REG_A5XX_RB_CCU_CNTL, 1);
            emit_zs(ring, pfb->zsbuf, batch->gmem_state);
            /* Enable stream output for the first pass (likely the binning). */
   OUT_PKT4(ring, REG_A5XX_VPC_SO_OVERRIDE, 1);
            if (use_hw_binning(batch)) {
               /* Disable stream output after binning, since each VS output should get
   * streamed out once.
   */
   OUT_PKT4(ring, REG_A5XX_VPC_SO_OVERRIDE, 1);
            fd5_emit_lrz_flush(batch, ring);
      } else {
                           /* XXX If we're in gmem mode but not doing HW binning, then after the first
   * tile we should disable stream output (fd6_gmem.c doesn't do that either).
      }
      /* before mem2gmem */
   static void
   fd5_emit_tile_prep(struct fd_batch *batch, const struct fd_tile *tile) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd5_context *fd5_ctx = fd5_context(ctx);
            uint32_t x1 = tile->xoff;
   uint32_t y1 = tile->yoff;
   uint32_t x2 = tile->xoff + tile->bin_w - 1;
            OUT_PKT4(ring, REG_A5XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_TL_X(x1) |
         OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_BR_X(x2) |
            OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_1, 2);
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_1_X(x1) | A5XX_RB_RESOLVE_CNTL_1_Y(y1));
            if (use_hw_binning(batch)) {
      const struct fd_vsc_pipe *pipe = &gmem->vsc_pipe[tile->p];
                     OUT_PKT7(ring, CP_SET_VISIBILITY_OVERRIDE, 1);
            OUT_PKT7(ring, CP_SET_BIN_DATA5, 5);
   OUT_RING(ring, CP_SET_BIN_DATA5_0_VSC_SIZE(pipe->w * pipe->h) |
         OUT_RELOC(ring, pipe_bo, 0, 0, 0);     /* VSC_PIPE[p].DATA_ADDRESS */
   OUT_RELOC(ring, fd5_ctx->vsc_size_mem, /* VSC_SIZE_ADDRESS + (p * 4) */
      } else {
      OUT_PKT7(ring, CP_SET_VISIBILITY_OVERRIDE, 1);
               OUT_PKT4(ring, REG_A5XX_RB_WINDOW_OFFSET, 1);
      }
      /*
   * transfer from system memory to gmem
   */
      static void
   emit_mem2gmem_surf(struct fd_batch *batch, uint32_t base,
         {
      struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_resource *rsc = fd_resource(psurf->texture);
                     if (buf == BLIT_S)
            if ((buf == BLIT_ZS) || (buf == BLIT_S)) {
      // XXX hack import via BLIT_MRT0 instead of BLIT_ZS, since I don't
   // know otherwise how to go from linear in sysmem to tiled in gmem.
   // possibly we want to flip this around gmem2mem and keep depth
   // tiled in sysmem (and fixup sampler state to assume tiled).. this
   // might be required for doing depth/stencil in bypass mode?
   enum a5xx_color_fmt format =
            OUT_PKT4(ring, REG_A5XX_RB_MRT_BUF_INFO(0), 5);
   OUT_RING(ring,
            A5XX_RB_MRT_BUF_INFO_COLOR_FORMAT(format) |
      OUT_RING(ring, A5XX_RB_MRT_PITCH(fd_resource_pitch(rsc, psurf->u.tex.level)));
   OUT_RING(ring, A5XX_RB_MRT_ARRAY_PITCH(fd_resource_layer_stride(rsc, psurf->u.tex.level)));
   OUT_RELOC(ring, rsc->bo,
                              stride = gmem->bin_w << fdl_cpp_shift(&rsc->layout);
            OUT_PKT4(ring, REG_A5XX_RB_BLIT_FLAG_DST_LO, 4);
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_LO */
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_HI */
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_PITCH */
            OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_3, 5);
   OUT_RING(ring, 0x00000000); /* RB_RESOLVE_CNTL_3 */
   OUT_RING(ring, base);       /* RB_BLIT_DST_LO */
   OUT_RING(ring, 0x00000000); /* RB_BLIT_DST_HI */
   OUT_RING(ring, A5XX_RB_BLIT_DST_PITCH(stride));
            OUT_PKT4(ring, REG_A5XX_RB_BLIT_CNTL, 1);
               }
      static void
   fd5_emit_tile_mem2gmem(struct fd_batch *batch, const struct fd_tile *tile)
   {
      struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
            /*
   * setup mrt and zs with system memory base addresses:
            emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, NULL);
            OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
   OUT_RING(ring, A5XX_RB_CNTL_WIDTH(gmem->bin_w) |
            if (fd_gmem_needs_restore(batch, tile, FD_BUFFER_COLOR)) {
      unsigned i;
   for (i = 0; i < pfb->nr_cbufs; i++) {
      if (!pfb->cbufs[i])
         if (!(batch->restore & (PIPE_CLEAR_COLOR0 << i)))
         emit_mem2gmem_surf(batch, gmem->cbuf_base[i], pfb->cbufs[i],
                  if (fd_gmem_needs_restore(batch, tile,
                     if (!rsc->stencil || fd_gmem_needs_restore(batch, tile, FD_BUFFER_DEPTH))
         if (rsc->stencil && fd_gmem_needs_restore(batch, tile, FD_BUFFER_STENCIL))
         }
      /* before IB to rendering cmds: */
   static void
   fd5_emit_tile_renderprep(struct fd_batch *batch, const struct fd_tile *tile)
   {
      struct fd_ringbuffer *ring = batch->gmem;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
            OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
   OUT_RING(ring,
            emit_zs(ring, pfb->zsbuf, gmem);
   emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, gmem);
      }
      /*
   * transfer from gmem to system memory (ie. normal RAM)
   */
      static void
   emit_gmem2mem_surf(struct fd_batch *batch, uint32_t base,
         {
      struct fd_ringbuffer *ring = batch->gmem;
   struct fd_resource *rsc = fd_resource(psurf->texture);
   bool tiled;
            if (!rsc->valid)
            if (buf == BLIT_S)
            offset =
                           OUT_PKT4(ring, REG_A5XX_RB_BLIT_FLAG_DST_LO, 4);
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_LO */
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_HI */
   OUT_RING(ring, 0x00000000); /* RB_BLIT_FLAG_DST_PITCH */
                     OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_3, 5);
   OUT_RING(ring, 0x00000004 | /* XXX RB_RESOLVE_CNTL_3 */
         OUT_RELOC(ring, rsc->bo, offset, 0, 0); /* RB_BLIT_DST_LO/HI */
   OUT_RING(ring, A5XX_RB_BLIT_DST_PITCH(pitch));
            OUT_PKT4(ring, REG_A5XX_RB_BLIT_CNTL, 1);
            //	bool msaa_resolve = pfb->samples > 1;
   bool msaa_resolve = false;
   OUT_PKT4(ring, REG_A5XX_RB_CLEAR_CNTL, 1);
               }
      static void
   fd5_emit_tile_gmem2mem(struct fd_batch *batch, const struct fd_tile *tile)
   {
      const struct fd_gmem_stateobj *gmem = batch->gmem_state;
            if (batch->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL)) {
               if (!rsc->stencil || (batch->resolve & FD_BUFFER_DEPTH))
         if (rsc->stencil && (batch->resolve & FD_BUFFER_STENCIL))
               if (batch->resolve & FD_BUFFER_COLOR) {
      unsigned i;
   for (i = 0; i < pfb->nr_cbufs; i++) {
      if (!pfb->cbufs[i])
         if (!(batch->resolve & (PIPE_CLEAR_COLOR0 << i)))
         emit_gmem2mem_surf(batch, gmem->cbuf_base[i], pfb->cbufs[i],
            }
      static void
   fd5_emit_tile_fini(struct fd_batch *batch) assert_dt
   {
               OUT_PKT7(ring, CP_SKIP_IB2_ENABLE_GLOBAL, 1);
                     fd5_cache_flush(batch, ring);
      }
      static void
   fd5_emit_sysmem_prep(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
                              if (batch->prologue)
            OUT_PKT7(ring, CP_SKIP_IB2_ENABLE_GLOBAL, 1);
                     OUT_PKT4(ring, REG_A5XX_PC_POWER_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_VFD_POWER_CNTL, 1);
            /* 0x10000000 for BYPASS.. 0x7c13c080 for GMEM: */
   fd_wfi(batch, ring);
   OUT_PKT4(ring, REG_A5XX_RB_CCU_CNTL, 1);
            OUT_PKT4(ring, REG_A5XX_RB_CNTL, 1);
   OUT_RING(ring, A5XX_RB_CNTL_WIDTH(0) | A5XX_RB_CNTL_HEIGHT(0) |
            /* remaining setup below here does not apply to blit/compute: */
   if (batch->nondraw)
                     OUT_PKT4(ring, REG_A5XX_GRAS_SC_WINDOW_SCISSOR_TL, 2);
   OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_TL_X(0) |
         OUT_RING(ring, A5XX_GRAS_SC_WINDOW_SCISSOR_BR_X(pfb->width - 1) |
            OUT_PKT4(ring, REG_A5XX_RB_RESOLVE_CNTL_1, 2);
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_1_X(0) | A5XX_RB_RESOLVE_CNTL_1_Y(0));
   OUT_RING(ring, A5XX_RB_RESOLVE_CNTL_2_X(pfb->width - 1) |
            OUT_PKT4(ring, REG_A5XX_RB_WINDOW_OFFSET, 1);
            /* Enable stream output, since there's no binning pass to put it in. */
   OUT_PKT4(ring, REG_A5XX_VPC_SO_OVERRIDE, 1);
            OUT_PKT7(ring, CP_SET_VISIBILITY_OVERRIDE, 1);
                     emit_zs(ring, pfb->zsbuf, NULL);
   emit_mrt(ring, pfb->nr_cbufs, pfb->cbufs, NULL);
      }
      static void
   fd5_emit_sysmem_fini(struct fd_batch *batch)
   {
               OUT_PKT7(ring, CP_SKIP_IB2_ENABLE_GLOBAL, 1);
                     fd5_event_write(batch, ring, PC_CCU_FLUSH_COLOR_TS, true);
      }
      void
   fd5_gmem_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->emit_tile_init = fd5_emit_tile_init;
   ctx->emit_tile_prep = fd5_emit_tile_prep;
   ctx->emit_tile_mem2gmem = fd5_emit_tile_mem2gmem;
   ctx->emit_tile_renderprep = fd5_emit_tile_renderprep;
   ctx->emit_tile_gmem2mem = fd5_emit_tile_gmem2mem;
   ctx->emit_tile_fini = fd5_emit_tile_fini;
   ctx->emit_sysmem_prep = fd5_emit_sysmem_prep;
      }
