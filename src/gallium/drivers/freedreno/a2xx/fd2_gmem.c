   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
      #include "freedreno_draw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
      #include "ir2/instr-a2xx.h"
   #include "fd2_context.h"
   #include "fd2_draw.h"
   #include "fd2_emit.h"
   #include "fd2_gmem.h"
   #include "fd2_program.h"
   #include "fd2_util.h"
   #include "fd2_zsa.h"
      static uint32_t
   fmt2swap(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B5G5R5X1_UNORM:
   case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B4G4R4X4_UNORM:
   case PIPE_FORMAT_B2G3R3_UNORM:
         default:
            }
      static bool
   use_hw_binning(struct fd_batch *batch)
   {
               /* we hardcoded a limit of 8 "pipes", we can increase this limit
   * at the cost of a slightly larger command stream
   * however very few cases will need more than 8
   * gmem->num_vsc_pipes == 0 means empty batch (TODO: does it still happen?)
   */
   if (gmem->num_vsc_pipes > 8 || !gmem->num_vsc_pipes)
            /* only a20x hw binning is implement
   * a22x is more like a3xx, but perhaps the a20x works? (TODO)
   */
   if (!is_a20x(batch->ctx->screen))
               }
      /* transfer from gmem to system memory (ie. normal RAM) */
      static void
   emit_gmem2mem_surf(struct fd_batch *batch, uint32_t base,
         {
      struct fd_ringbuffer *ring = batch->tile_store;
   struct fd_resource *rsc = fd_resource(psurf->texture);
   uint32_t offset =
         enum pipe_format format = fd_gmem_restore_format(psurf->format);
            assert((pitch & 31) == 0);
            if (!rsc->valid)
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_INFO));
   OUT_RING(ring, A2XX_RB_COLOR_INFO_BASE(base) |
            OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_CONTROL));
   OUT_RING(ring, 0x00000000);             /* RB_COPY_CONTROL */
   OUT_RELOC(ring, rsc->bo, offset, 0, 0); /* RB_COPY_DEST_BASE */
   OUT_RING(ring, pitch >> 5);             /* RB_COPY_DEST_PITCH */
   OUT_RING(ring,                          /* RB_COPY_DEST_INFO */
            A2XX_RB_COPY_DEST_INFO_FORMAT(fd2_pipe2color(format)) |
      COND(!rsc->layout.tile_mode, A2XX_RB_COPY_DEST_INFO_LINEAR) |
   A2XX_RB_COPY_DEST_INFO_WRITE_RED |
            if (!is_a20x(batch->ctx->screen)) {
               OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
   OUT_RING(ring, 3); /* VGT_MAX_VTX_INDX */
               fd_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   prepare_tile_fini_ib(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct fd2_context *fd2_ctx = fd2_context(ctx);
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            batch->tile_store =
                  fd2_emit_vertex_bufs(ring, 0x9c,
                              OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
            if (!is_a20x(ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
                        OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_SC_MODE_CNTL));
   OUT_RING(
      ring,
   A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST | /* PA_SU_SC_MODE_CNTL */
               OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
   OUT_RING(ring, xy2d(0, 0));                    /* PA_SC_WINDOW_SCISSOR_TL */
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
   OUT_RING(ring, fui((float)gmem->bin_w / 2.0f)); /* XSCALE */
   OUT_RING(ring, fui((float)gmem->bin_w / 2.0f)); /* XOFFSET */
   OUT_RING(ring, fui((float)gmem->bin_h / 2.0f)); /* YSCALE */
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_MODECONTROL));
            if (batch->resolve & (FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))
            if (batch->resolve & FD_BUFFER_COLOR)
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_MODECONTROL));
            if (!is_a20x(ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
         }
      static void
   fd2_emit_tile_gmem2mem(struct fd_batch *batch, const struct fd_tile *tile)
   {
         }
      /* transfer from system memory to gmem */
      static void
   emit_mem2gmem_surf(struct fd_batch *batch, uint32_t base,
         {
      struct fd_ringbuffer *ring = batch->gmem;
   struct fd_resource *rsc = fd_resource(psurf->texture);
   uint32_t offset =
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_INFO));
   OUT_RING(ring, A2XX_RB_COLOR_INFO_BASE(base) |
            /* emit fb as a texture: */
   OUT_PKT3(ring, CP_SET_CONSTANT, 7);
   OUT_RING(ring, 0x00010000);
   OUT_RING(ring, A2XX_SQ_TEX_0_CLAMP_X(SQ_TEX_WRAP) |
                     A2XX_SQ_TEX_0_CLAMP_Y(SQ_TEX_WRAP) |
   OUT_RELOC(ring, rsc->bo, offset,
            A2XX_SQ_TEX_1_FORMAT(fd2_pipe2surface(format).format) |
      OUT_RING(ring, A2XX_SQ_TEX_2_WIDTH(psurf->width - 1) |
         OUT_RING(ring, A2XX_SQ_TEX_3_MIP_FILTER(SQ_TEX_FILTER_BASEMAP) |
                     A2XX_SQ_TEX_3_SWIZ_X(0) | A2XX_SQ_TEX_3_SWIZ_Y(1) |
   OUT_RING(ring, 0x00000000);
            if (!is_a20x(batch->ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_MAX_VTX_INDX));
   OUT_RING(ring, 3); /* VGT_MAX_VTX_INDX */
               fd_draw(batch, ring, DI_PT_RECTLIST, IGNORE_VISIBILITY,
      }
      static void
   fd2_emit_tile_mem2gmem(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd2_context *fd2_ctx = fd2_context(ctx);
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   unsigned bin_w = tile->bin_w;
   unsigned bin_h = tile->bin_h;
            fd2_emit_vertex_bufs(
      ring, 0x9c,
   (struct fd2_vertex_buf[]){
      {.prsc = fd2_ctx->solid_vertexbuf, .size = 36},
      },
         /* write texture coordinates to vertexbuf: */
   x0 = ((float)tile->xoff) / ((float)pfb->width);
   x1 = ((float)tile->xoff + bin_w) / ((float)pfb->width);
   y0 = ((float)tile->yoff) / ((float)pfb->height);
   y1 = ((float)tile->yoff + bin_h) / ((float)pfb->height);
   OUT_PKT3(ring, CP_MEM_WRITE, 7);
   OUT_RELOC(ring, fd_resource(fd2_ctx->solid_vertexbuf)->bo, 36, 0, 0);
   OUT_RING(ring, fui(x0));
   OUT_RING(ring, fui(y0));
   OUT_RING(ring, fui(x1));
   OUT_RING(ring, fui(y0));
   OUT_RING(ring, fui(x0));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_INDX_OFFSET));
                     OUT_PKT0(ring, REG_A2XX_TC_CNTL_STATUS, 1);
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_DEPTHCONTROL));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SU_SC_MODE_CNTL));
   OUT_RING(ring, A2XX_PA_SU_SC_MODE_CNTL_PROVOKING_VTX_LAST |
                  OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_AA_MASK));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLORCONTROL));
   OUT_RING(ring, A2XX_RB_COLORCONTROL_ALPHA_FUNC(FUNC_ALWAYS) |
                              OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_BLEND_CONTROL));
   OUT_RING(ring, A2XX_RB_BLEND_CONTROL_COLOR_SRCBLEND(FACTOR_ONE) |
                     A2XX_RB_BLEND_CONTROL_COLOR_COMB_FCN(BLEND2_DST_PLUS_SRC) |
            OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_SCISSOR_TL));
   OUT_RING(ring, A2XX_PA_SC_WINDOW_OFFSET_DISABLE |
                  OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VPORT_XSCALE));
   OUT_RING(ring, fui((float)bin_w / 2.0f));  /* PA_CL_VPORT_XSCALE */
   OUT_RING(ring, fui((float)bin_w / 2.0f));  /* PA_CL_VPORT_XOFFSET */
   OUT_RING(ring, fui(-(float)bin_h / 2.0f)); /* PA_CL_VPORT_YSCALE */
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VTE_CNTL));
   OUT_RING(ring, A2XX_PA_CL_VTE_CNTL_VTX_XY_FMT |
                     A2XX_PA_CL_VTE_CNTL_VTX_Z_FMT | // XXX check this???
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_CLIP_CNTL));
            if (fd_gmem_needs_restore(batch, tile, FD_BUFFER_DEPTH | FD_BUFFER_STENCIL))
            if (fd_gmem_needs_restore(batch, tile, FD_BUFFER_COLOR))
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_CL_VTE_CNTL));
   OUT_RING(ring, A2XX_PA_CL_VTE_CNTL_VTX_W0_FMT |
                     A2XX_PA_CL_VTE_CNTL_VPORT_X_SCALE_ENA |
   A2XX_PA_CL_VTE_CNTL_VPORT_X_OFFSET_ENA |
               }
      static void
   patch_draws(struct fd_batch *batch, enum pc_di_vis_cull_mode vismode)
   {
               if (!is_a20x(batch->ctx->screen)) {
      /* identical to a3xx */
   for (i = 0; i < fd_patch_num_elements(&batch->draw_patches); i++) {
      struct fd_cs_patch *patch = fd_patch_element(&batch->draw_patches, i);
      }
   util_dynarray_clear(&batch->draw_patches);
               if (vismode == USE_VISIBILITY)
            for (i = 0; i < batch->draw_patches.size / sizeof(uint32_t *); i++) {
      uint32_t *ptr =
                  /* convert CP_DRAW_INDX_BIN to a CP_DRAW_INDX
   * replace first two DWORDS with NOP and move the rest down
   * (we don't want to have to move the idx buffer reloc)
   */
   ptr[0] = CP_TYPE3_PKT | (CP_NOP << 8);
            ptr[4] = ptr[2] & ~(1 << 14 | 1 << 15); /* remove cull_enable bits */
   ptr[2] = CP_TYPE3_PKT | ((cnt - 2) << 16) | (CP_DRAW_INDX << 8);
         }
      static void
   fd2_emit_sysmem_prep(struct fd_batch *batch)
   {
      struct fd_context *ctx = batch->ctx;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            if (!psurf)
            struct fd_resource *rsc = fd_resource(psurf->texture);
   uint32_t offset =
                  assert((pitch & 31) == 0);
                     OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_SURFACE_INFO));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_INFO));
   OUT_RELOC(ring, rsc->bo, offset,
            COND(!rsc->layout.tile_mode, A2XX_RB_COLOR_INFO_LINEAR) |
               OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_SCREEN_SCISSOR_TL));
   OUT_RING(ring, A2XX_PA_SC_SCREEN_SCISSOR_TL_WINDOW_OFFSET_DISABLE);
   OUT_RING(ring, A2XX_PA_SC_SCREEN_SCISSOR_BR_X(pfb->width) |
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
   OUT_RING(ring,
            patch_draws(batch, IGNORE_VISIBILITY);
   util_dynarray_clear(&batch->draw_patches);
      }
      /* before first tile */
   static void
   fd2_emit_tile_init(struct fd_batch *batch) assert_dt
   {
      struct fd_context *ctx = batch->ctx;
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
   const struct fd_gmem_stateobj *gmem = batch->gmem_state;
   enum pipe_format format = pipe_surface_format(pfb->cbufs[0]);
                              OUT_PKT3(ring, CP_SET_CONSTANT, 4);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_SURFACE_INFO));
   OUT_RING(ring, gmem->bin_w); /* RB_SURFACE_INFO */
   OUT_RING(ring, A2XX_RB_COLOR_INFO_SWAP(fmt2swap(format)) |
         reg = A2XX_RB_DEPTH_INFO_DEPTH_BASE(gmem->zsbuf_base[0]);
   if (pfb->zsbuf)
                  /* fast clear patches */
   int depth_size = -1;
            if (pfb->cbufs[0])
            if (pfb->zsbuf)
            for (int i = 0; i < fd_patch_num_elements(&batch->gmem_patches); i++) {
      struct fd_cs_patch *patch = fd_patch_element(&batch->gmem_patches, i);
   uint32_t color_base = 0, depth_base = gmem->zsbuf_base[0];
            /* note: 1 "line" is 512 bytes in both color/depth areas (1K total) */
   switch (patch->val) {
   case GMEM_PATCH_FASTCLEAR_COLOR:
      size = align(gmem->bin_w * gmem->bin_h * color_size, 0x8000);
   lines = size / 1024;
   depth_base = size / 2;
      case GMEM_PATCH_FASTCLEAR_DEPTH:
      size = align(gmem->bin_w * gmem->bin_h * depth_size, 0x8000);
   lines = size / 1024;
   color_base = depth_base;
   depth_base = depth_base + size / 2;
      case GMEM_PATCH_FASTCLEAR_COLOR_DEPTH:
      lines =
            case GMEM_PATCH_RESTORE_INFO:
      patch->cs[0] = gmem->bin_w;
   patch->cs[1] = A2XX_RB_COLOR_INFO_SWAP(fmt2swap(format)) |
         patch->cs[2] = A2XX_RB_DEPTH_INFO_DEPTH_BASE(gmem->zsbuf_base[0]);
   if (pfb->zsbuf)
      patch->cs[2] |= A2XX_RB_DEPTH_INFO_DEPTH_FORMAT(
         default:
                  patch->cs[0] = A2XX_PA_SC_SCREEN_SCISSOR_BR_X(32) |
         patch->cs[4] = A2XX_RB_COLOR_INFO_BASE(color_base) |
         patch->cs[5] = A2XX_RB_DEPTH_INFO_DEPTH_BASE(depth_base) |
      }
            /* set to zero, for some reason hardware doesn't like certain values */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_CURRENT_BIN_ID_MIN));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_CURRENT_BIN_ID_MAX));
            if (use_hw_binning(batch)) {
      /* patch out unneeded memory exports by changing EXEC CF to EXEC_END
   *
   * in the shader compiler, we guarantee that the shader ends with
   * a specific pattern of ALLOC/EXEC CF pairs for the hw binning exports
   *
   * the since patches point only to dwords and CFs are 1.5 dwords
   * the patch is aligned and might point to a ALLOC CF
   */
   for (int i = 0; i < batch->shader_patches.size / sizeof(void *); i++) {
      instr_cf_t *cf =
         if (cf->opc == ALLOC)
         assert(cf->opc == EXEC);
   assert(cf[ctx->screen->info->num_vsc_pipes * 2 - 2].opc == EXEC_END);
                        /* initialize shader constants for the binning memexport */
   OUT_PKT3(ring, CP_SET_CONSTANT, 1 + gmem->num_vsc_pipes * 4);
            for (int i = 0; i < gmem->num_vsc_pipes; i++) {
      /* allocate in 64k increments to avoid reallocs */
   uint32_t bo_size = align(batch->num_vertices, 0x10000);
   if (!ctx->vsc_pipe_bo[i] ||
      fd_bo_size(ctx->vsc_pipe_bo[i]) < bo_size) {
   if (ctx->vsc_pipe_bo[i])
         ctx->vsc_pipe_bo[i] =
                     /* memory export address (export32):
   * .x: (base_address >> 2) | 0x40000000 (?)
   * .y: index (float) - set by shader
   * .z: 0x4B00D000 (?)
   * .w: 0x4B000000 (?) | max_index (?)
   */
   OUT_RELOC(ring, ctx->vsc_pipe_bo[i], 0, 0x40000000, -2);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, 0x4B00D000);
               OUT_PKT3(ring, CP_SET_CONSTANT, 1 + gmem->num_vsc_pipes * 8);
            for (int i = 0; i < gmem->num_vsc_pipes; i++) {
                     /* const to tranform from [-1,1] to bin coordinates for this pipe
   * for x/y, [0,256/2040] = 0, [256/2040,512/2040] = 1, etc
   * 8 possible values on x/y axis,
   * to clip at binning stage: only use center 6x6
   * TODO: set the z parameters too so that hw binning
                  mul_x = 1.0f / (float)(gmem->bin_w * 8);
   mul_y = 1.0f / (float)(gmem->bin_h * 8);
                  OUT_RING(ring, fui(off_x * (256.0f / 255.0f)));
   OUT_RING(ring, fui(off_y * (256.0f / 255.0f)));
                  OUT_RING(ring, fui(mul_x * (256.0f / 255.0f)));
   OUT_RING(ring, fui(mul_y * (256.0f / 255.0f)));
   OUT_RING(ring, fui(0.0f));
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
                     OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_VERTEX_REUSE_BLOCK_CNTL));
      } else {
                  util_dynarray_clear(&batch->draw_patches);
      }
      /* before mem2gmem */
   static void
   fd2_emit_tile_prep(struct fd_batch *batch, const struct fd_tile *tile)
   {
      struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_INFO));
   OUT_RING(ring, A2XX_RB_COLOR_INFO_SWAP(1) | /* RB_COLOR_INFO */
            /* setup screen scissor for current tile (same for mem2gmem): */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_SCREEN_SCISSOR_TL));
   OUT_RING(ring, A2XX_PA_SC_SCREEN_SCISSOR_TL_X(0) |
         OUT_RING(ring, A2XX_PA_SC_SCREEN_SCISSOR_BR_X(tile->bin_w) |
      }
      /* before IB to rendering cmds: */
   static void
   fd2_emit_tile_renderprep(struct fd_batch *batch,
         {
      struct fd_context *ctx = batch->ctx;
   struct fd2_context *fd2_ctx = fd2_context(ctx);
   struct fd_ringbuffer *ring = batch->gmem;
   struct pipe_framebuffer_state *pfb = &batch->framebuffer;
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COLOR_INFO));
   OUT_RING(ring, A2XX_RB_COLOR_INFO_SWAP(fmt2swap(format)) |
            /* setup window scissor and offset for current tile (different
   * from mem2gmem):
   */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_PA_SC_WINDOW_OFFSET));
   OUT_RING(ring, A2XX_PA_SC_WINDOW_OFFSET_X(-tile->xoff) |
            /* write SCISSOR_BR to memory so fast clear path can restore from it */
   OUT_PKT3(ring, CP_MEM_WRITE, 2);
   OUT_RELOC(ring, fd_resource(fd2_ctx->solid_vertexbuf)->bo, 60, 0, 0);
   OUT_RING(ring, A2XX_PA_SC_SCREEN_SCISSOR_BR_X(tile->bin_w) |
            /* set the copy offset for gmem2mem */
   OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_RB_COPY_DEST_OFFSET));
   OUT_RING(ring, A2XX_RB_COPY_DEST_OFFSET_X(tile->xoff) |
            /* tile offset for gl_FragCoord on a20x (C64 in fragment shader) */
   if (is_a20x(ctx->screen)) {
      OUT_PKT3(ring, CP_SET_CONSTANT, 5);
   OUT_RING(ring, 0x00000580);
   OUT_RING(ring, fui(tile->xoff));
   OUT_RING(ring, fui(tile->yoff));
   OUT_RING(ring, fui(0.0f));
               if (use_hw_binning(batch)) {
               OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_CURRENT_BIN_ID_MIN));
            OUT_PKT3(ring, CP_SET_CONSTANT, 2);
   OUT_RING(ring, CP_REG(REG_A2XX_VGT_CURRENT_BIN_ID_MAX));
            /* TODO only emit this when tile->p changes */
   OUT_PKT3(ring, CP_SET_DRAW_INIT_FLAGS, 1);
         }
      void
   fd2_gmem_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->emit_sysmem_prep = fd2_emit_sysmem_prep;
   ctx->emit_tile_init = fd2_emit_tile_init;
   ctx->emit_tile_prep = fd2_emit_tile_prep;
   ctx->emit_tile_mem2gmem = fd2_emit_tile_mem2gmem;
   ctx->emit_tile_renderprep = fd2_emit_tile_renderprep;
      }
