   /*
   * Copyright (c) 2012-2015 Etnaviv Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Wladimir J. van der Laan <laanwj@gmail.com>
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_context.h"
      #include "etnaviv_blend.h"
   #include "etnaviv_clear_blit.h"
   #include "etnaviv_compiler.h"
   #include "etnaviv_debug.h"
   #include "etnaviv_emit.h"
   #include "etnaviv_fence.h"
   #include "etnaviv_query.h"
   #include "etnaviv_query_acc.h"
   #include "etnaviv_rasterizer.h"
   #include "etnaviv_resource.h"
   #include "etnaviv_screen.h"
   #include "etnaviv_shader.h"
   #include "etnaviv_state.h"
   #include "etnaviv_surface.h"
   #include "etnaviv_texture.h"
   #include "etnaviv_transfer.h"
   #include "etnaviv_translate.h"
   #include "etnaviv_zsa.h"
      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/hash_table.h"
   #include "util/u_blitter.h"
   #include "util/u_draw.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_debug_cb.h"
   #include "util/u_surface.h"
   #include "util/u_transfer.h"
      #include "hw/common.xml.h"
      static inline void
   etna_emit_nop_with_data(struct etna_cmd_stream *stream, uint32_t value)
   {
      etna_cmd_stream_emit(stream, VIV_FE_NOP_HEADER_OP_NOP);
      }
      static void
   etna_emit_string_marker(struct pipe_context *pctx, const char *string, int len)
   {
      struct etna_context *ctx = etna_context(pctx);
   struct etna_cmd_stream *stream = ctx->stream;
                     while (len >= 4) {
      etna_emit_nop_with_data(stream, *buf);
   buf++;
               /* copy remainder bytes without reading past end of input string */
   if (len > 0) {
      uint32_t w = 0;
   memcpy(&w, buf, len);
         }
      static void
   etna_set_frontend_noop(struct pipe_context *pctx, bool enable)
   {
               pctx->flush(pctx, NULL, 0);
      }
      static void
   etna_context_destroy(struct pipe_context *pctx)
   {
               if (ctx->pending_resources)
            if (ctx->flush_resources)
                     if (ctx->blitter)
            if (pctx->stream_uploader)
            if (ctx->stream)
                              if (ctx->in_fence_fd != -1)
               }
      /* Update render state where needed based on draw operation */
   static void
   etna_update_state_for_draw(struct etna_context *ctx, const struct pipe_draw_info *info)
   {
      /* Handle primitive restart:
   * - If not an indexed draw, we don't care about the state of the primitive restart bit.
   * - Otherwise, set the bit in INDEX_STREAM_CONTROL in the index buffer state
   *   accordingly
   * - If the value of the INDEX_STREAM_CONTROL register changed due to this, or
   *   primitive restart is enabled and the restart index changed, mark the index
   *   buffer state as dirty
            if (info->index_size) {
               if (info->primitive_restart)
         else
            if (ctx->index_buffer.FE_INDEX_STREAM_CONTROL != new_control ||
      (info->primitive_restart && ctx->index_buffer.FE_PRIMITIVE_RESTART_INDEX != info->restart_index)) {
   ctx->index_buffer.FE_INDEX_STREAM_CONTROL = new_control;
   ctx->index_buffer.FE_PRIMITIVE_RESTART_INDEX = info->restart_index;
            }
      static bool
   etna_get_vs(struct etna_context *ctx, struct etna_shader_key* const key)
   {
                        if (!ctx->shader.vs)
            if (old != ctx->shader.vs)
               }
      static bool
   etna_get_fs(struct etna_context *ctx, struct etna_shader_key* const key)
   {
               /* update the key if we need to run nir_lower_sample_tex_compare(..). */
   if (ctx->screen->specs.halti < 2 &&
               for (unsigned int i = 0; i < ctx->num_fragment_sampler_views; i++) {
                                    key->tex_swizzle[i].swizzle_r = ctx->sampler_view[i]->swizzle_r;
   key->tex_swizzle[i].swizzle_g = ctx->sampler_view[i]->swizzle_g;
                                          if (!ctx->shader.fs)
            if (old != ctx->shader.fs)
               }
      static void
   etna_draw_vbo(struct pipe_context *pctx, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(pctx, info, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !info->instance_count))
            struct etna_context *ctx = etna_context(pctx);
   struct etna_screen *screen = ctx->screen;
   struct pipe_framebuffer_state *pfb = &ctx->framebuffer_s;
   uint32_t draw_mode;
            if (!indirect &&
      !info->primitive_restart &&
   !u_trim_pipe_prim(info->mode, (unsigned*)&draws[0].count))
         if (ctx->vertex_elements == NULL || ctx->vertex_elements->num_elements == 0)
            if (unlikely(ctx->rasterizer->cull_face == PIPE_FACE_FRONT_AND_BACK &&
                  if (!etna_render_condition_check(pctx))
            int prims = u_decomposed_prims_for_vertices(info->mode, draws[0].count);
   if (unlikely(prims <= 0)) {
      DBG("Invalid draw primitive mode=%i or no primitives to be drawn", info->mode);
               draw_mode = translate_draw_mode(info->mode);
   if (draw_mode == ETNA_NO_MATCH) {
      BUG("Unsupported draw mode");
               /* Upload a user index buffer. */
   unsigned index_offset = 0;
            if (info->index_size) {
      indexbuf = info->has_user_indices ? NULL : info->index.resource;
   if (info->has_user_indices &&
      !util_upload_index_buffer(pctx, info, &draws[0], &indexbuf, &index_offset, 4)) {
   BUG("Index buffer upload failed.");
      }
   /* Add start to index offset, when rendering indexed */
            ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.bo = etna_resource(indexbuf)->bo;
   ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.offset = index_offset;
   ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.flags = ETNA_RELOC_READ;
            if (!ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.bo) {
      BUG("Unsupported or no index buffer");
         } else {
      ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.bo = 0;
   ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.offset = 0;
   ctx->index_buffer.FE_INDEX_STREAM_BASE_ADDR.flags = 0;
      }
            struct etna_shader_key key = {
      .front_ccw = ctx->rasterizer->front_ccw,
   .sprite_coord_enable = ctx->rasterizer->sprite_coord_enable,
               if (pfb->cbufs[0])
            if (!etna_get_vs(ctx, &key) || !etna_get_fs(ctx, &key)) {
      BUG("compiled shaders are not okay");
               /* Update any derived state */
   if (!etna_state_update(ctx))
            /*
   * Figure out the buffers/features we need:
   */
   if (ctx->dirty & ETNA_DIRTY_ZSA) {
      if (etna_depth_enabled(ctx))
            if (etna_stencil_enabled(ctx))
               if (ctx->dirty & ETNA_DIRTY_FRAMEBUFFER) {
      for (i = 0; i < pfb->nr_cbufs; i++) {
                              surf = pfb->cbufs[i]->texture;
                  if (ctx->dirty & ETNA_DIRTY_SHADER) {
      /* Mark constant buffers as being read */
   u_foreach_bit(i, ctx->constant_buffer[PIPE_SHADER_VERTEX].enabled_mask)
            u_foreach_bit(i, ctx->constant_buffer[PIPE_SHADER_FRAGMENT].enabled_mask)
               if (ctx->dirty & ETNA_DIRTY_VERTEX_BUFFERS) {
      /* Mark VBOs as being read */
   u_foreach_bit(i, ctx->vertex_buffer.enabled_mask) {
      assert(!ctx->vertex_buffer.vb[i].is_user_buffer);
                  if (ctx->dirty & ETNA_DIRTY_INDEX_BUFFER) {
      /* Mark index buffer as being read */
               /* Mark textures as being read */
   for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
      if (ctx->sampler_view[i]) {
                     /* if texture was modified since the last update,
   * we need to clear the texture cache and possibly
   * resolve/update ts
   */
                  ctx->stats.prims_generated += u_reduced_prims_for_vertices(info->mode, draws[0].count);
            /* Update state for this draw operation */
            /* First, sync state, then emit DRAW_PRIMITIVES or DRAW_INDEXED_PRIMITIVES */
            if (!VIV_FEATURE(screen, chipMinorFeatures6, NEW_GPIPE)) {
      switch (draw_mode) {
   case PRIMITIVE_TYPE_LINE_LOOP:
   case PRIMITIVE_TYPE_LINE_STRIP:
   case PRIMITIVE_TYPE_TRIANGLE_STRIP:
   case PRIMITIVE_TYPE_TRIANGLE_FAN:
      etna_set_state(ctx->stream, VIVS_GL_VERTEX_ELEMENT_CONFIG,
                  default:
      etna_set_state(ctx->stream, VIVS_GL_VERTEX_ELEMENT_CONFIG,
                        if (screen->specs.halti >= 2) {
      /* On HALTI2+ (GC3000 and higher) only use instanced drawing commands, as the blob does */
   etna_draw_instanced(ctx->stream, info->index_size, draw_mode, info->instance_count,
      } else {
      if (info->index_size)
         else
               if (DBG_ENABLED(ETNA_DBG_DRAW_STALL)) {
      /* Stall the FE after every draw operation.  This allows better
   * debug of GPU hang conditions, as the FE will indicate which
   * draw op has caused the hang. */
               if (DBG_ENABLED(ETNA_DBG_FLUSH_ALL))
            if (ctx->framebuffer_s.cbufs[0])
         if (ctx->framebuffer_s.zsbuf)
         if (info->index_size && indexbuf != info->index.resource)
      }
      static void
   etna_reset_gpu_state(struct etna_context *ctx)
   {
      struct etna_cmd_stream *stream = ctx->stream;
   struct etna_screen *screen = ctx->screen;
            etna_set_state(stream, VIVS_GL_API_MODE, VIVS_GL_API_MODE_OPENGL);
   etna_set_state(stream, VIVS_PA_W_CLIP_LIMIT, 0x34000001);
   etna_set_state(stream, VIVS_PA_FLAGS, 0x00000000); /* blob sets ZCONVERT_BYPASS on GC3000+, this messes up z for us */
   etna_set_state(stream, VIVS_PA_VIEWPORT_UNK00A80, 0x38a01404);
   etna_set_state(stream, VIVS_PA_VIEWPORT_UNK00A84, fui(8192.0));
   etna_set_state(stream, VIVS_PA_ZFARCLIPPING, 0x00000000);
   etna_set_state(stream, VIVS_RA_HDEPTH_CONTROL, 0x00007000);
            /* There is no HALTI0 specific state */
   if (screen->specs.halti >= 1) { /* Only on HALTI1+ */
         }
   if (screen->specs.halti >= 2) { /* Only on HALTI2+ */
         }
   if (screen->specs.halti >= 3) { /* Only on HALTI3+ */
         }
   if (screen->specs.halti >= 4) { /* Only on HALTI4+ */
      etna_set_state(stream, VIVS_PS_MSAA_CONFIG, 0x6fffffff & 0xf70fffff & 0xfff6ffff &
            }
   if (screen->specs.halti >= 5) { /* Only on HALTI5+ */
      etna_set_state(stream, VIVS_NTE_DESCRIPTOR_UNK14C40, 0x00000001);
   etna_set_state(stream, VIVS_FE_HALTI5_UNK007D8, 0x00000002);
   etna_set_state(stream, VIVS_PS_SAMPLER_BASE, 0x00000000);
   etna_set_state(stream, VIVS_VS_SAMPLER_BASE, 0x00000020);
      } else { /* Only on pre-HALTI5 */
      etna_set_state(stream, VIVS_GL_UNK03838, 0x00000000);
               if (VIV_FEATURE(screen, chipMinorFeatures4, BUG_FIXES18))
            if (!screen->specs.use_blt) {
      /* Enable SINGLE_BUFFER for resolve, if supported */
               if (screen->specs.halti >= 5) {
      /* TXDESC cache flush - do this once at the beginning, as texture
   * descriptors are only written by the CPU once, then patched by the kernel
   * before command stream submission. It does not need flushing if the
   * referenced image data changes.
   */
   etna_set_state(stream, VIVS_NTE_DESCRIPTOR_FLUSH, 0);
   etna_set_state(stream, VIVS_GL_FLUSH_CACHE,
                  /* Icache invalidate (should do this on shader change?) */
   etna_set_state(stream, VIVS_VS_ICACHE_INVALIDATE,
         VIVS_VS_ICACHE_INVALIDATE_UNK0 | VIVS_VS_ICACHE_INVALIDATE_UNK1 |
               /* It seems that some GPUs (at least some GC400 have shown this behavior)
   * come out of reset with random vertex attributes enabled and also don't
   * disable them on the write to the first config register as normal. Enabling
   * all attributes seems to provide the GPU with the required edge to actually
   * disable the unused attributes on the next draw.
   */
   if (screen->specs.halti >= 5) {
      etna_set_state_multi(stream, VIVS_NFE_GENERIC_ATTRIB_CONFIG0(0),
      } else {
      etna_set_state_multi(stream, VIVS_FE_VERTEX_ELEMENT_CONFIG(0),
                        ctx->dirty = ~0L;
   ctx->dirty_sampler_views = ~0L;
      }
      void
   etna_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
         {
      struct etna_context *ctx = etna_context(pctx);
            list_for_each_entry(struct etna_acc_query, aq, &ctx->active_acc_queries, node)
            if (!internal) {
      /* flush all resources that need an implicit flush */
   set_foreach(ctx->flush_resources, entry) {
               pctx->flush_resource(pctx, prsc);
      }
               etna_cmd_stream_flush(ctx->stream, ctx->in_fence_fd,
                  list_for_each_entry(struct etna_acc_query, aq, &ctx->active_acc_queries, node)
            if (fence)
                        }
      static void
   etna_context_flush(struct pipe_context *pctx, struct pipe_fence_handle **fence,
         {
         }
      static void
   etna_context_force_flush(struct etna_cmd_stream *stream, void *priv)
   {
                        /* update derived states as the context is now fully dirty */
      }
      void
   etna_context_add_flush_resource(struct etna_context *ctx,
         {
                        if (!found)
      }
      static void
   etna_set_debug_callback(struct pipe_context *pctx,
         {
      struct etna_context *ctx = etna_context(pctx);
            util_queue_finish(&screen->shader_compiler_queue);
      }
      struct pipe_context *
   etna_context_create(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
      struct etna_context *ctx = CALLOC_STRUCT(etna_context);
   struct etna_screen *screen;
            if (ctx == NULL)
            pctx = &ctx->base;
   pctx->priv = ctx;
   pctx->screen = pscreen;
   pctx->stream_uploader = u_upload_create_default(pctx);
   if (!pctx->stream_uploader)
                  screen = etna_screen(pscreen);
   ctx->stream = etna_cmd_stream_new(screen->pipe, 0x2000,
         if (ctx->stream == NULL)
            ctx->pending_resources = _mesa_pointer_hash_table_create(NULL);
   if (!ctx->pending_resources)
            ctx->flush_resources = _mesa_set_create(NULL, _mesa_hash_pointer,
         if (!ctx->flush_resources)
            /* context ctxate setup */
   ctx->screen = screen;
   /* need some sane default in case gallium frontends don't set some state: */
            /*  Set sensible defaults for state */
                     pctx->destroy = etna_context_destroy;
   pctx->draw_vbo = etna_draw_vbo;
   pctx->flush = etna_context_flush;
   pctx->set_debug_callback = etna_set_debug_callback;
   pctx->create_fence_fd = etna_create_fence_fd;
   pctx->fence_server_sync = etna_fence_server_sync;
   pctx->emit_string_marker = etna_emit_string_marker;
   pctx->set_frontend_noop = etna_set_frontend_noop;
   pctx->clear_buffer = u_default_clear_buffer;
            /* creation of compile states */
   pctx->create_blend_state = etna_blend_state_create;
   pctx->create_rasterizer_state = etna_rasterizer_state_create;
            etna_clear_blit_init(pctx);
   etna_query_context_init(pctx);
   etna_state_init(pctx);
   etna_surface_init(pctx);
   etna_shader_init(pctx);
   etna_texture_init(pctx);
            ctx->blitter = util_blitter_create(pctx);
   if (!ctx->blitter)
            slab_create_child(&ctx->transfer_pool, &screen->transfer_pool);
                  fail:
                  }
      bool
   etna_render_condition_check(struct pipe_context *pctx)
   {
               if (!ctx->cond_query)
                     union pipe_query_result res = { 0 };
   bool wait =
      ctx->cond_mode != PIPE_RENDER_COND_NO_WAIT &&
         if (pctx->get_query_result(pctx, ctx->cond_query, wait, &res))
               }
