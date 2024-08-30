   /*
   * Copyright (c) 2014 Scott Mansell
   * Copyright © 2014 Broadcom
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/u_blitter.h"
   #include "util/u_draw.h"
   #include "util/u_prim.h"
   #include "util/format/u_format.h"
   #include "util/u_pack_color.h"
   #include "util/u_split_draw.h"
   #include "util/u_upload_mgr.h"
      #include "vc4_context.h"
   #include "vc4_resource.h"
      #define VC4_HW_2116_COUNT		0x1ef0
      static void
   vc4_get_draw_cl_space(struct vc4_job *job, int vert_count)
   {
         /* The SW-5891 workaround may cause us to emit multiple shader recs
      * and draw packets.
               /* Binner gets our packet state -- vc4_emit.c contents,
      * and the primitive itself.
      cl_ensure_space(&job->bcl,
                           /* shader_rec gets up to 12 dwords of reloc handles plus a maximally
      * sized shader_rec (104 bytes base for 8 vattrs plus 32 bytes of
   * vattr stride).
      cl_ensure_space(&job->shader_rec,
                     /* There could be up to 16 textures per stage, plus misc other
      * pointers.
      cl_ensure_space(&job->bo_handles, (2 * 16 + 20) * sizeof(uint32_t));
   cl_ensure_space(&job->bo_pointers,
   }
      /**
   * Does the initial bining command list setup for drawing to a given FBO.
   */
   static void
   vc4_start_draw(struct vc4_context *vc4)
   {
                  if (job->needs_flush)
                     cl_emit(&job->bcl, TILE_BINNING_MODE_CONFIGURATION, bin) {
            bin.width_in_tiles = job->draw_tiles_x;
               /* START_TILE_BINNING resets the statechange counters in the hardware,
      * which are what is used when a primitive is binned to a tile to
   * figure out what new state packets need to be written to that tile's
   * command list.
               /* Reset the current compressed primitives format.  This gets modified
      * by VC4_PACKET_GL_INDEXED_PRIMITIVE and
   * VC4_PACKET_GL_ARRAY_PRIMITIVE, so it needs to be reset at the start
   * of every tile.
      cl_emit(&job->bcl, PRIMITIVE_LIST_FORMAT, list) {
                        job->needs_flush = true;
   job->draw_width = vc4->framebuffer.width;
   }
      static void
   vc4_predraw_check_textures(struct pipe_context *pctx,
         {
                  for (int i = 0; i < stage_tex->num_textures; i++) {
            struct vc4_sampler_view *view =
                                    }
      static void
   vc4_emit_gl_shader_state(struct vc4_context *vc4,
                     {
         struct vc4_job *job = vc4->job;
   /* VC4_DIRTY_VTXSTATE */
   struct vc4_vertex_stateobj *vtx = vc4->vtx;
   /* VC4_DIRTY_VTXBUF */
            /* The simulator throws a fit if VS or CS don't read an attribute, so
      * we emit a dummy read.
               /* Emit the shader record. */
            cl_emit(&job->shader_rec, SHADER_RECORD, rec) {
                                          /* VC4_DIRTY_PRIM_MODE | VC4_DIRTY_RASTERIZER */
                        /* VC4_DIRTY_COMPILED_FS */
   rec.fragment_shader_number_of_varyings =
                        rec.coordinate_shader_attribute_array_select_bits =
         rec.coordinate_shader_total_attributes_size =
                        rec.vertex_shader_attribute_array_select_bits =
         rec.vertex_shader_total_attributes_size =
                     uint32_t max_index = 0xffff;
   unsigned index_bias = info->index_size ? draws->index_bias : 0;
   for (int i = 0; i < vtx->num_elements; i++) {
            struct pipe_vertex_element *elem = &vtx->pipe[i];
   struct pipe_vertex_buffer *vb =
         struct vc4_resource *rsc = vc4_resource(vb->buffer.resource);
   /* not vc4->dirty tracked: vc4->last_index_bias */
   uint32_t offset = (vb->buffer_offset +
                                          cl_emit(&job->shader_rec, ATTRIBUTE_RECORD, attr) {
            attr.address = cl_address(rsc->bo, offset);
   attr.number_of_bytes_minus_1 = elem_size - 1;
   attr.stride = elem->src_stride;
   attr.coordinate_shader_vpm_offset =
                     if (elem->src_stride > 0) {
                     if (vtx->num_elements == 0) {
                           cl_emit(&job->shader_rec, ATTRIBUTE_RECORD, attr) {
            attr.address = cl_address(bo, 0);
   attr.number_of_bytes_minus_1 = 16 - 1;
                           cl_emit(&job->bcl, GL_SHADER_STATE, shader_state) {
            /* Note that number of attributes == 0 in the packet means 8
   * attributes.  This field also contains the offset into
   * shader_rec.
   */
   assert(vtx->num_elements <= 8);
               vc4_write_uniforms(vc4, vc4->prog.fs,
               vc4_write_uniforms(vc4, vc4->prog.vs,
               vc4_write_uniforms(vc4, vc4->prog.cs,
                  vc4->last_index_bias = index_bias + extra_index_bias;
   vc4->max_index = max_index;
   }
      /**
   * HW-2116 workaround: Flush the batch before triggering the hardware state
   * counter wraparound behavior.
   *
   * State updates are tracked by a global counter which increments at the first
   * state update after a draw or a START_BINNING.  Tiles can then have their
   * state updated at draw time with a set of cheap checks for whether the
   * state's copy of the global counter matches the global counter the last time
   * that state was written to the tile.
   *
   * The state counters are relatively small and wrap around quickly, so you
   * could get false negatives for needing to update a particular state in the
   * tile.  To avoid this, the hardware attempts to write all of the state in
   * the tile at wraparound time.  This apparently is broken, so we just flush
   * everything before that behavior is triggered.  A batch flush is sufficient
   * to get our current contents drawn and reset the counters to 0.
   *
   * Note that we can't just use VC4_PACKET_FLUSH_ALL, because that caps the
   * tiles with VC4_PACKET_RETURN_FROM_LIST.
   */
   static void
   vc4_hw_2116_workaround(struct pipe_context *pctx, int vert_count)
   {
         struct vc4_context *vc4 = vc4_context(pctx);
            if (job->draw_calls_queued + vert_count / 65535 >= VC4_HW_2116_COUNT) {
            perf_debug("Flushing batch due to HW-2116 workaround "
      }
      /* A HW bug fails to draw 2-vert line loops.  Just draw it as two GL_LINES. */
   static bool
   vc4_draw_workaround_line_loop_2(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   {
         if (draw->count != 2 || info->mode != MESA_PRIM_LINE_LOOP)
            struct pipe_draw_info local_info = *info;
            /* Draw twice.  The vertex order will be wrong on the second prim, but
      * that's probably not worth rewriting an index buffer over.
      for (int i = 0; i < 2; i++)
            }
      static void
   vc4_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
         if (num_draws > 1) {
                        if (!indirect && (!draws[0].count || !info->instance_count))
               if (!indirect &&
      !info->primitive_restart &&
      return;
            if (vc4_draw_workaround_line_loop_2(pctx, info, drawid_offset, indirect, draws))
            /* Before setting up the draw, do any fixup blits necessary. */
   vc4_predraw_check_textures(pctx, &vc4->verttex);
                              /* Make sure that the raster order flags haven't changed, which can
      * only be set at job granularity.
      if (job->flags != vc4->rasterizer->tile_raster_order_flags) {
                                 if (vc4->prim_mode != info->mode) {
                        vc4_start_draw(vc4);
   if (!vc4_update_compiled_shaders(vc4, info->mode)) {
                                          unsigned index_bias = info->index_size ? draws->index_bias : 0;
   if ((vc4->dirty & (VC4_DIRTY_VTXBUF |
                        VC4_DIRTY_VTXSTATE |
   VC4_DIRTY_PRIM_MODE |
   VC4_DIRTY_RASTERIZER |
   VC4_DIRTY_COMPILED_CS |
   VC4_DIRTY_COMPILED_VS |
   VC4_DIRTY_COMPILED_FS |
      vc4->last_index_bias != index_bias) {
      if (info->index_size)
                              /* Note that the primitive type fields match with OpenGL/gallium
      * definitions, up to but not including QUADS.
      if (info->index_size) {
            uint32_t index_size = info->index_size;
   uint32_t offset = draws[0].start * index_size;
   struct pipe_resource *prsc;
   if (info->index_size == 4) {
            prsc = vc4_get_shadow_index_buffer(pctx, info,
            } else {
            if (info->has_user_indices) {
         unsigned start_offset = draws[0].start * info->index_size;
   prsc = NULL;
   u_upload_data(vc4->uploader, start_offset,
               } else {
                              /* The original design for the VC4 kernel UABI had multiple
   * packets that used relocations in the BCL (some of which
   * needed two BOs), but later modifications eliminated all but
   * this one usage.  We have an arbitrary 32-bit offset value,
   * and need to also supply an arbitrary 32-bit index buffer
   * GEM handle, so we have this fake packet we emit in our BCL
   * to be validated, which the kernel uses at validation time
   * to perform the relocation in the IB packet (without
   * emitting to the actual HW).
   */
   uint32_t hindex = vc4_gem_hindex(job, rsc->bo);
   if (job->last_gem_handle_hindex != hindex) {
            cl_u8(&bcl, VC4_PACKET_GEM_HANDLES);
                     cl_u8(&bcl, VC4_PACKET_GL_INDEXED_PRIMITIVE);
   cl_u8(&bcl,
         info->mode |
   (index_size == 2 ?
   VC4_INDEX_BUFFER_U16:
                                          } else {
            uint32_t count = draws[0].count;
                        /* GFXH-515 / SW-5891: The binner emits 16 bit indices for
   * drawarrays, which means that if start + count > 64k it
   * would truncate the top bits.  Work around this by emitting
   * a limited number of primitives at a time and reemitting the
   * shader state pointing farther down the vertex attribute
   * arrays.
   *
   * To do this properly for line loops or trifans, we'd need to
   * make a new VB containing the first vertex plus whatever
   * remainder.
   */
   if (start + count > max_verts) {
                                                                                                cl_emit(&job->bcl, VERTEX_ARRAY_PRIMITIVES, array) {
                                    count -= step;
   extra_index_bias += start + step;
            /* We shouldn't have tripped the HW_2116 bug with the GFXH-515
      * workaround.
               if (vc4->zsa && vc4->framebuffer.zsbuf) {
                           if (vc4->zsa->base.depth_enabled) {
                        if (vc4->zsa->base.stencil[0].enabled) {
                              /* If we've used half of the presumably 256MB CMA area, flush the job
      * so that we don't accumulate a job that will end up not being
   * executable.
      if (job->bo_space > 128 * 1024 * 1024)
            if (VC4_DBG(ALWAYS_FLUSH))
   }
      static uint32_t
   pack_rgba(enum pipe_format format, const float *rgba)
   {
         union util_color uc;
   util_pack_color(rgba, format, &uc);
   if (util_format_get_blocksize(format) == 2)
         else
   }
      static void
   vc4_clear(struct pipe_context *pctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
         struct vc4_context *vc4 = vc4_context(pctx);
            if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
                                 /* Clearing ZS will clear both Z and stencil, so if we're
   * trying to clear just one then we need to draw a quad to do
   * it instead.  We need to do this before setting up
   * tile-based clears in vc4->job, because the blitter may
   * submit the current job.
   */
   if ((zsclear == PIPE_CLEAR_DEPTH ||
                                       perf_debug("Partial clear of Z+stencil buffer, "
         vc4_blitter_save(vc4);
   util_blitter_clear(vc4->blitter,
                     vc4->framebuffer.width,
   vc4->framebuffer.height,
   1,
   buffers &= ~zsclear;
   if (!buffers)
            /* We can't flag new buffers for clearing once we've queued draws.  We
      * could avoid this by using the 3d engine to clear.
      if (job->draw_calls_queued) {
            perf_debug("Flushing rendering to process new clear.\n");
               if (buffers & PIPE_CLEAR_COLOR0) {
                                 if (vc4_rt_format_is_565(vc4->framebuffer.cbufs[0]->format)) {
            /* In 565 mode, the hardware will be packing our color
   * for us.
   */
      } else {
            /* Otherwise, we need to do this packing because we
   * support multiple swizzlings of RGBA8888.
   */
   clear_color =
      }
               if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
                           /* Though the depth buffer is stored with Z in the high 24,
   * for this field we just need to store it in the low 24.
   */
   if (buffers & PIPE_CLEAR_DEPTH) {
                                          job->draw_min_x = 0;
   job->draw_min_y = 0;
   job->draw_max_x = vc4->framebuffer.width;
   job->draw_max_y = vc4->framebuffer.height;
   job->cleared |= buffers;
            }
      static void
   vc4_clear_render_target(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
                  vc4_blitter_save(vc4);
   }
      static void
   vc4_clear_depth_stencil(struct pipe_context *pctx, struct pipe_surface *ps,
                     {
                  vc4_blitter_save(vc4);
   util_blitter_clear_depth_stencil(vc4->blitter, ps, buffers, depth,
   }
      void
   vc4_draw_init(struct pipe_context *pctx)
   {
         pctx->draw_vbo = vc4_draw_vbo;
   pctx->clear = vc4_clear;
   pctx->clear_render_target = vc4_clear_render_target;
   }
