   /*
   * Copyright © 2017 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <stdio.h>
   #include <time.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/u_debug.h"
   #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_upload_mgr.h"
   #include "drm-uapi/i915_drm.h"
   #include "iris_context.h"
   #include "iris_resource.h"
   #include "iris_screen.h"
   #include "iris_utrace.h"
   #include "common/intel_sample_positions.h"
      /**
   * The pipe->set_debug_callback() driver hook.
   */
   static void
   iris_set_debug_callback(struct pipe_context *ctx,
         {
      struct iris_context *ice = (struct iris_context *)ctx;
                     if (cb)
         else
      }
      /**
   * Called from the batch module when it detects a GPU hang.
   *
   * In this case, we've lost our GEM context, and can't rely on any existing
   * state on the GPU.  We must mark everything dirty and wipe away any saved
   * assumptions about the last known state of the GPU.
   */
   void
   iris_lost_context_state(struct iris_batch *batch)
   {
               if (batch->name == IRIS_BATCH_RENDER) {
         } else if (batch->name == IRIS_BATCH_COMPUTE) {
         } else if (batch->name == IRIS_BATCH_BLITTER) {
         } else {
                  ice->state.dirty = ~0ull;
   ice->state.stage_dirty = ~0ull;
   ice->state.current_hash_scale = 0;
   memset(&ice->shaders.urb, 0, sizeof(ice->shaders.urb));
   memset(ice->state.last_block, 0, sizeof(ice->state.last_block));
   memset(ice->state.last_grid, 0, sizeof(ice->state.last_grid));
   ice->state.last_grid_dim = 0;
   batch->last_binder_address = ~0ull;
   batch->last_aux_map_state = 0;
      }
      static enum pipe_reset_status
   iris_get_device_reset_status(struct pipe_context *ctx)
   {
                        /* Check the reset status of each batch's hardware context, and take the
   * worst status (if one was guilty, proclaim guilt).
   */
   iris_foreach_batch(ice, batch) {
      enum pipe_reset_status batch_reset =
            if (batch_reset == PIPE_NO_RESET)
            if (worst_reset == PIPE_NO_RESET) {
         } else {
      /* GUILTY < INNOCENT < UNKNOWN */
                  if (worst_reset != PIPE_NO_RESET && ice->reset.reset)
               }
      static void
   iris_set_device_reset_callback(struct pipe_context *ctx,
         {
               if (cb)
         else
      }
      static void
   iris_get_sample_position(struct pipe_context *ctx,
                     {
      union {
      struct {
      float x[16];
      } a;
   struct {
      float  _0XOffset,  _1XOffset,  _2XOffset,  _3XOffset,
         _4XOffset,  _5XOffset,  _6XOffset,  _7XOffset,
   _8XOffset,  _9XOffset, _10XOffset, _11XOffset,
   float  _0YOffset,  _1YOffset,  _2YOffset,  _3YOffset,
         _4YOffset,  _5YOffset,  _6YOffset,  _7YOffset,
         } u;
   switch (sample_count) {
   case 1:  INTEL_SAMPLE_POS_1X(u.v._);  break;
   case 2:  INTEL_SAMPLE_POS_2X(u.v._);  break;
   case 4:  INTEL_SAMPLE_POS_4X(u.v._);  break;
   case 8:  INTEL_SAMPLE_POS_8X(u.v._);  break;
   case 16: INTEL_SAMPLE_POS_16X(u.v._); break;
   default: unreachable("invalid sample count");
            out_value[0] = u.a.x[sample_index];
      }
      static bool
   create_dirty_dmabuf_set(struct iris_context *ice)
   {
               ice->dirty_dmabufs = _mesa_pointer_set_create(ice);
      }
      void
   iris_mark_dirty_dmabuf(struct iris_context *ice,
         {
      if (!_mesa_set_search(ice->dirty_dmabufs, res)) {
      _mesa_set_add(ice->dirty_dmabufs, res);
         }
      static void
   clear_dirty_dmabuf_set(struct iris_context *ice)
   {
      set_foreach(ice->dirty_dmabufs, entry) {
      struct pipe_resource *res = (struct pipe_resource *)entry->key;
   if (pipe_reference(&res->reference, NULL))
                  }
      void
   iris_flush_dirty_dmabufs(struct iris_context *ice)
   {
      set_foreach(ice->dirty_dmabufs, entry) {
      struct pipe_resource *res = (struct pipe_resource *)entry->key;
                  }
      /**
   * Destroy a context, freeing any associated memory.
   */
   void
   iris_destroy_context(struct pipe_context *ctx)
   {
      struct iris_context *ice = (struct iris_context *)ctx;
            if (ctx->stream_uploader)
         if (ctx->const_uploader)
                              for (unsigned i = 0; i < ARRAY_SIZE(ice->shaders.scratch_surfs); i++)
            for (unsigned i = 0; i < ARRAY_SIZE(ice->shaders.scratch_bos); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(ice->shaders.scratch_bos[i]); j++)
               iris_destroy_program_cache(ice);
   if (screen->measure.config)
            u_upload_destroy(ice->state.surface_uploader);
   u_upload_destroy(ice->state.scratch_surface_uploader);
   u_upload_destroy(ice->state.dynamic_uploader);
            iris_destroy_batches(ice);
                     slab_destroy_child(&ice->transfer_pool);
               }
      #define genX_call(devinfo, func, ...)             \
      switch ((devinfo)->verx10) {                   \
   case 200:                                      \
      gfx20_##func(__VA_ARGS__);                  \
      case 125:                                      \
      gfx125_##func(__VA_ARGS__);                 \
      case 120:                                      \
      gfx12_##func(__VA_ARGS__);                  \
      case 110:                                      \
      gfx11_##func(__VA_ARGS__);                  \
      case 90:                                       \
      gfx9_##func(__VA_ARGS__);                   \
      case 80:                                       \
      gfx8_##func(__VA_ARGS__);                   \
      default:                                       \
               /**
   * Create a context.
   *
   * This is where each context begins.
   */
   struct pipe_context *
   iris_create_context(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
      struct iris_screen *screen = (struct iris_screen*)pscreen;
   const struct intel_device_info *devinfo = screen->devinfo;
            if (!ice)
                     ctx->screen = pscreen;
            ctx->stream_uploader = u_upload_create_default(ctx);
   if (!ctx->stream_uploader) {
      ralloc_free(ice);
      }
   ctx->const_uploader = u_upload_create(ctx, 1024 * 1024,
                     if (!ctx->const_uploader) {
      u_upload_destroy(ctx->stream_uploader);
   ralloc_free(ice);
               if (!create_dirty_dmabuf_set(ice)) {
      ralloc_free(ice);
               ctx->destroy = iris_destroy_context;
   ctx->set_debug_callback = iris_set_debug_callback;
   ctx->set_device_reset_callback = iris_set_device_reset_callback;
   ctx->get_device_reset_status = iris_get_device_reset_status;
            iris_init_context_fence_functions(ctx);
   iris_init_blit_functions(ctx);
   iris_init_clear_functions(ctx);
   iris_init_program_functions(ctx);
   iris_init_resource_functions(ctx);
   iris_init_flush_functions(ctx);
            iris_init_program_cache(ice);
            slab_create_child(&ice->transfer_pool, &screen->transfer_pool);
            ice->state.surface_uploader =
      u_upload_create(ctx, 64 * 1024, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
            ice->state.scratch_surface_uploader =
      u_upload_create(ctx, 64 * 1024, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
            ice->state.dynamic_uploader =
      u_upload_create(ctx, 64 * 1024, PIPE_BIND_CUSTOM, PIPE_USAGE_IMMUTABLE,
               ice->query_buffer_uploader =
      u_upload_create(ctx, 16 * 1024, PIPE_BIND_CUSTOM, PIPE_USAGE_STAGING,
         genX_call(devinfo, init_state, ice);
   genX_call(devinfo, init_blorp, ice);
            if (flags & PIPE_CONTEXT_HIGH_PRIORITY)
         if (flags & PIPE_CONTEXT_LOW_PRIORITY)
         if (flags & PIPE_CONTEXT_PROTECTED)
            if (INTEL_DEBUG(DEBUG_BATCH))
            /* Do this before initializing the batches */
                     screen->vtbl.init_render_context(&ice->batches[IRIS_BATCH_RENDER]);
            if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
            /* Clover doesn't support u_threaded_context */
   if (flags & PIPE_CONTEXT_COMPUTE_ONLY)
            return threaded_context_create(ctx, &screen->transfer_pool,
                              }
