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
   #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_upload_mgr.h"
   #include "drm-uapi/i915_drm.h"
   #include "crocus_context.h"
   #include "crocus_resource.h"
   #include "crocus_screen.h"
   #include "common/intel_defines.h"
   #include "common/intel_sample_positions.h"
      /**
   * The pipe->set_debug_callback() driver hook.
   */
   static void
   crocus_set_debug_callback(struct pipe_context *ctx,
         {
               if (cb)
         else
      }
      static bool
   crocus_init_identifier_bo(struct crocus_context *ice)
   {
               bo_map = crocus_bo_map(NULL, ice->workaround_bo, MAP_READ | MAP_WRITE);
   if (!bo_map)
            ice->workaround_bo->kflags |= EXEC_OBJECT_CAPTURE;
   ice->workaround_offset = ALIGN(
                        }
      /**
   * Called from the batch module when it detects a GPU hang.
   *
   * In this case, we've lost our GEM context, and can't rely on any existing
   * state on the GPU.  We must mark everything dirty and wipe away any saved
   * assumptions about the last known state of the GPU.
   */
   void
   crocus_lost_context_state(struct crocus_batch *batch)
   {
      /* The batch module doesn't have an crocus_context, because we want to
   * avoid introducing lots of layering violations.  Unfortunately, here
   * we do need to inform the context of batch catastrophe.  We know the
   * batch is one of our context's, so hackily claw our way back.
   */
   struct crocus_context *ice = batch->ice;
   struct crocus_screen *screen = batch->screen;
   if (batch->name == CROCUS_BATCH_RENDER) {
         } else if (batch->name == CROCUS_BATCH_COMPUTE) {
         } else {
                  ice->state.dirty = ~0ull;
   memset(ice->state.last_grid, 0, sizeof(ice->state.last_grid));
   batch->state_base_address_emitted = false;
      }
      static enum pipe_reset_status
   crocus_get_device_reset_status(struct pipe_context *ctx)
   {
                        /* Check the reset status of each batch's hardware context, and take the
   * worst status (if one was guilty, proclaim guilt).
   */
   for (int i = 0; i < ice->batch_count; i++) {
      /* This will also recreate the hardware contexts as necessary, so any
   * future queries will show no resets.  We only want to report once.
   */
   enum pipe_reset_status batch_reset =
            if (batch_reset == PIPE_NO_RESET)
            if (worst_reset == PIPE_NO_RESET) {
         } else {
      /* GUILTY < INNOCENT < UNKNOWN */
                  if (worst_reset != PIPE_NO_RESET && ice->reset.reset)
               }
      static void
   crocus_set_device_reset_callback(struct pipe_context *ctx,
         {
               if (cb)
         else
      }
      static void
   crocus_get_sample_position(struct pipe_context *ctx,
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
      /**
   * Destroy a context, freeing any associated memory.
   */
   static void
   crocus_destroy_context(struct pipe_context *ctx)
   {
      struct crocus_context *ice = (struct crocus_context *)ctx;
   struct crocus_screen *screen = (struct crocus_screen *)ctx->screen;
   if (ctx->stream_uploader)
            if (ice->blitter)
                  for (unsigned i = 0; i < ARRAY_SIZE(ice->shaders.scratch_bos); i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(ice->shaders.scratch_bos[i]); j++)
               crocus_destroy_program_cache(ice);
                     slab_destroy_child(&ice->transfer_pool);
            crocus_batch_free(&ice->batches[CROCUS_BATCH_RENDER]);
   if (ice->batches[CROCUS_BATCH_COMPUTE].ice)
               }
      #define genX_call(devinfo, func, ...)                   \
      switch ((devinfo)->verx10) {                         \
   case 80:                                             \
      gfx8_##func(__VA_ARGS__);                         \
      case 75:                                             \
      gfx75_##func(__VA_ARGS__);                        \
      case 70:                                             \
      gfx7_##func(__VA_ARGS__);                         \
      case 60:                                             \
      gfx6_##func(__VA_ARGS__);                         \
      case 50:                                             \
      gfx5_##func(__VA_ARGS__);                         \
      case 45:                                             \
      gfx45_##func(__VA_ARGS__);                        \
      case 40:                                             \
      gfx4_##func(__VA_ARGS__);                         \
      default:                                             \
               /**
   * Create a context.
   *
   * This is where each context begins.
   */
   struct pipe_context *
   crocus_create_context(struct pipe_screen *pscreen, void *priv, unsigned flags)
   {
      struct crocus_screen *screen = (struct crocus_screen*)pscreen;
   const struct intel_device_info *devinfo = &screen->devinfo;
            if (!ice)
                     ctx->screen = pscreen;
            ctx->stream_uploader = u_upload_create_default(ctx);
   if (!ctx->stream_uploader) {
      ralloc_free(ice);
      }
            ctx->destroy = crocus_destroy_context;
   ctx->set_debug_callback = crocus_set_debug_callback;
   ctx->set_device_reset_callback = crocus_set_device_reset_callback;
   ctx->get_device_reset_status = crocus_get_device_reset_status;
                     crocus_init_context_fence_functions(ctx);
   crocus_init_blit_functions(ctx);
   crocus_init_clear_functions(ctx);
   crocus_init_program_functions(ctx);
   crocus_init_resource_functions(ctx);
   crocus_init_flush_functions(ctx);
                     slab_create_child(&ice->transfer_pool, &screen->transfer_pool);
            ice->query_buffer_uploader =
      u_upload_create(ctx, 4096, PIPE_BIND_CUSTOM, PIPE_USAGE_STAGING,
         ice->workaround_bo =
         if (!ice->workaround_bo)
            if (!crocus_init_identifier_bo(ice))
            genX_call(devinfo, crocus_init_state, ice);
   genX_call(devinfo, crocus_init_blorp, ice);
            ice->blitter = util_blitter_create(&ice->ctx);
   if (ice->blitter == NULL)
         int priority = 0;
   if (flags & PIPE_CONTEXT_HIGH_PRIORITY)
         if (flags & PIPE_CONTEXT_LOW_PRIORITY)
            ice->batch_count = devinfo->ver >= 7 ? CROCUS_BATCH_COUNT : 1;
   for (int i = 0; i < ice->batch_count; i++) {
      crocus_init_batch(ice, (enum crocus_batch_name) i,
               ice->urb.size = devinfo->urb.size;
   screen->vtbl.init_render_context(&ice->batches[CROCUS_BATCH_RENDER]);
   if (ice->batch_count > 1)
            if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
            return threaded_context_create(ctx, &screen->transfer_pool,
                  }
      bool
   crocus_sw_check_cond_render(struct crocus_context *ice)
   {
      struct crocus_query *q = ice->condition.query;
            bool wait = ice->condition.mode == PIPE_RENDER_COND_WAIT ||
         if (!q)
            bool ret = ice->ctx.get_query_result(&ice->ctx, (void *)q, wait, &result);
   if (!ret)
               }
