   /*
   * Copyright (C) 2019-2020 Collabora, Ltd.
   * © Copyright 2018 Alyssa Rosenzweig
   * Copyright © 2014-2017 Broadcom
   * Copyright (C) 2017 Intel Corporation
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
   */
      #include <errno.h>
   #include <poll.h>
      #include "pan_bo.h"
   #include "pan_context.h"
   #include "pan_minmax_cache.h"
      #include "util/format/u_format.h"
   #include "util/half_float.h"
   #include "util/libsync.h"
   #include "util/macros.h"
   #include "util/u_debug_cb.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "util/u_prim_restart.h"
   #include "util/u_surface.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_vbuf.h"
      #include "compiler/nir/nir_serialize.h"
   #include "util/pan_lower_framebuffer.h"
   #include "decode.h"
   #include "pan_fence.h"
   #include "pan_screen.h"
   #include "pan_util.h"
      static void
   panfrost_clear(struct pipe_context *pipe, unsigned buffers,
                     {
      if (!panfrost_render_condition_check(pan_context(pipe)))
            /* Only get batch after checking the render condition, since the check can
   * cause the batch to be flushed.
   */
   struct panfrost_context *ctx = pan_context(pipe);
            /* At the start of the batch, we can clear for free */
   if (!batch->scoreboard.first_job) {
      panfrost_batch_clear(batch, buffers, color, depth, stencil);
               /* Once there is content, clear with a fullscreen quad */
            perf_debug_ctx(ctx, "Clearing with quad");
   util_blitter_clear(
      ctx->blitter, ctx->pipe_framebuffer.width, ctx->pipe_framebuffer.height,
   util_framebuffer_get_num_layers(&ctx->pipe_framebuffer), buffers, color,
   depth, stencil,
   }
      bool
   panfrost_writes_point_size(struct panfrost_context *ctx)
   {
      struct panfrost_compiled_shader *vs = ctx->prog[PIPE_SHADER_VERTEX];
               }
      /* The entire frame is in memory -- send it off to the kernel! */
      void
   panfrost_flush(struct pipe_context *pipe, struct pipe_fence_handle **fence,
         {
      struct panfrost_context *ctx = pan_context(pipe);
            /* Submit all pending jobs */
            if (fence) {
      struct pipe_fence_handle *f = panfrost_fence_create(ctx);
   pipe->screen->fence_reference(pipe->screen, fence, NULL);
               if (dev->debug & PAN_DBG_TRACE)
      }
      static void
   panfrost_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
      struct panfrost_context *ctx = pan_context(pipe);
      }
      static void
   panfrost_set_frontend_noop(struct pipe_context *pipe, bool enable)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   panfrost_flush_all_batches(ctx, "Frontend no-op change");
      }
      static void
   panfrost_generic_cso_delete(struct pipe_context *pctx, void *hwcso)
   {
         }
      static void
   panfrost_bind_blend_state(struct pipe_context *pipe, void *cso)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   ctx->blend = cso;
      }
      static void
   panfrost_set_blend_color(struct pipe_context *pipe,
         {
      struct panfrost_context *ctx = pan_context(pipe);
            if (blend_color)
      }
      /* Create a final blend given the context */
      mali_ptr
   panfrost_get_blend(struct panfrost_batch *batch, unsigned rti,
         {
      struct panfrost_context *ctx = batch->ctx;
   struct panfrost_device *dev = pan_device(ctx->base.screen);
   struct panfrost_blend_state *blend = ctx->blend;
   struct pan_blend_info info = blend->info[rti];
   struct pipe_surface *surf = batch->key.cbufs[rti];
            /* Use fixed-function if the equation permits, the format is blendable,
   * and no more than one unique constant is accessed */
   if (info.fixed_function && panfrost_blendable_formats_v7[fmt].internal &&
      pan_blend_is_homogenous_constant(info.constant_mask,
                     /* On all architectures, we can disable writes for a blend descriptor,
   * at which point the format doesn't matter.
   */
   if (!info.enabled)
            /* On Bifrost and newer, we can also use fixed-function for opaque
   * output regardless of the format by configuring the appropriate
   * conversion descriptor in the internal blend descriptor. (Midgard
   * requires a blend shader even for this case.)
   */
   if (dev->arch >= 6 && info.opaque)
            /* Otherwise, we need to grab a shader */
   struct pan_blend_state pan_blend = blend->pan;
            pan_blend.rts[rti].format = fmt;
   pan_blend.rts[rti].nr_samples = nr_samples;
   memcpy(pan_blend.constants, ctx->blend_color.color,
            /* Upload the shader, sharing a BO */
   if (!(*bo)) {
      *bo = panfrost_batch_create_bo(batch, 4096, PAN_BO_EXECUTE,
                        /* Default for Midgard */
   nir_alu_type col0_type = nir_type_float32;
            /* Bifrost has per-output types, respect them */
   if (dev->arch >= 6) {
      col0_type = ss->info.bifrost.blend[rti].type;
               pthread_mutex_lock(&dev->blend_shaders.lock);
   struct pan_blend_shader_variant *shader =
      pan_screen(ctx->base.screen)
         /* Size check and upload */
   unsigned offset = *shader_offset;
   assert((offset + shader->binary.size) < 4096);
   memcpy((*bo)->ptr.cpu + offset, shader->binary.data, shader->binary.size);
   *shader_offset += shader->binary.size;
               }
      static void
   panfrost_bind_rasterizer_state(struct pipe_context *pctx, void *hwcso)
   {
      struct panfrost_context *ctx = pan_context(pctx);
            /* We can assume rasterizer is always dirty, the dependencies are
   * too intricate to bother tracking in detail. However we could
   * probably diff the renderers for viewport dirty tracking, that
   * just cares about the scissor enable and the depth clips. */
      }
      static void
   panfrost_set_shader_images(struct pipe_context *pctx,
                     {
      struct panfrost_context *ctx = pan_context(pctx);
            /* Unbind start_slot...start_slot+count */
   if (!iviews) {
      for (int i = start_slot;
      i < start_slot + count + unbind_num_trailing_slots; i++) {
               ctx->image_mask[shader] &= ~(((1ull << count) - 1) << start_slot);
               /* Bind start_slot...start_slot+count */
   for (int i = 0; i < count; i++) {
      const struct pipe_image_view *image = &iviews[i];
            if (!image->resource) {
      util_copy_image_view(&ctx->images[shader][start_slot + i], NULL);
                        /* Images don't work with AFBC, since they require pixel-level granularity
   */
   if (drm_is_afbc(rsrc->image.layout.modifier)) {
      pan_resource_modifier_convert(
      ctx, rsrc, DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED, true,
                        /* Unbind start_slot+count...start_slot+count+unbind_num_trailing_slots */
   for (int i = 0; i < unbind_num_trailing_slots; i++) {
      SET_BIT(ctx->image_mask[shader], 1 << (start_slot + count + i), NULL);
         }
      static void
   panfrost_bind_vertex_elements_state(struct pipe_context *pctx, void *hwcso)
   {
      struct panfrost_context *ctx = pan_context(pctx);
   ctx->vertex = hwcso;
      }
      static void
   panfrost_bind_sampler_states(struct pipe_context *pctx,
               {
      struct panfrost_context *ctx = pan_context(pctx);
            for (unsigned i = 0; i < num_sampler; i++) {
      unsigned p = start_slot + i;
   ctx->samplers[shader][p] = sampler ? sampler[i] : NULL;
   if (ctx->samplers[shader][p])
         else
                  }
      static void
   panfrost_set_vertex_buffers(struct pipe_context *pctx, unsigned num_buffers,
                     {
               util_set_vertex_buffers_mask(ctx->vertex_buffers, &ctx->vb_mask, buffers,
                     }
      static void
   panfrost_set_constant_buffer(struct pipe_context *pctx,
                     {
      struct panfrost_context *ctx = pan_context(pctx);
                              if (unlikely(!buf)) {
      pbuf->enabled_mask &= ~mask;
               pbuf->enabled_mask |= mask;
      }
      static void
   panfrost_set_stencil_ref(struct pipe_context *pctx,
         {
      struct panfrost_context *ctx = pan_context(pctx);
   ctx->stencil_ref = ref;
      }
      static void
   panfrost_set_sampler_views(struct pipe_context *pctx,
                                 {
      struct panfrost_context *ctx = pan_context(pctx);
            unsigned new_nr = 0;
            for (i = 0; i < num_views; ++i) {
      struct pipe_sampler_view *view = views ? views[i] : NULL;
            if (view)
            if (take_ownership) {
      pipe_sampler_view_reference(
            } else {
      pipe_sampler_view_reference(
                  for (; i < num_views + unbind_num_trailing_slots; i++) {
      unsigned p = i + start_slot;
   pipe_sampler_view_reference(
               /* If the sampler view count is higher than the greatest sampler view
   * we touch, it can't change */
   if (ctx->sampler_view_count[shader] >
      start_slot + num_views + unbind_num_trailing_slots)
         /* If we haven't set any sampler views here, search lower numbers for
   * set sampler views */
   if (new_nr == 0) {
      for (i = 0; i < start_slot; ++i) {
      if (ctx->sampler_views[shader][i])
                     }
      static void
   panfrost_set_shader_buffers(struct pipe_context *pctx,
                           {
               util_set_shader_buffers_mask(ctx->ssbo[shader], &ctx->ssbo_mask[shader],
               }
      static void
   panfrost_set_framebuffer_state(struct pipe_context *pctx,
         {
               util_copy_framebuffer_state(&ctx->pipe_framebuffer, fb);
            /* Hot draw call path needs the mask of active render targets */
            for (unsigned i = 0; i < ctx->pipe_framebuffer.nr_cbufs; ++i) {
      if (ctx->pipe_framebuffer.cbufs[i])
         }
      static void
   panfrost_bind_depth_stencil_state(struct pipe_context *pipe, void *cso)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   ctx->depth_stencil = cso;
      }
      static void
   panfrost_set_sample_mask(struct pipe_context *pipe, unsigned sample_mask)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   ctx->sample_mask = sample_mask;
      }
      static void
   panfrost_set_min_samples(struct pipe_context *pipe, unsigned min_samples)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   ctx->min_samples = min_samples;
      }
      static void
   panfrost_set_clip_state(struct pipe_context *pipe,
         {
         }
      static void
   panfrost_set_viewport_states(struct pipe_context *pipe, unsigned start_slot,
               {
               assert(start_slot == 0);
            ctx->pipe_viewport = *viewports;
      }
      static void
   panfrost_set_scissor_states(struct pipe_context *pipe, unsigned start_slot,
               {
               assert(start_slot == 0);
            ctx->scissor = *scissors;
      }
      static void
   panfrost_set_polygon_stipple(struct pipe_context *pipe,
         {
         }
      static void
   panfrost_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   ctx->active_queries = enable;
      }
      static void
   panfrost_render_condition(struct pipe_context *pipe, struct pipe_query *query,
         {
               ctx->cond_query = (struct panfrost_query *)query;
   ctx->cond_cond = condition;
      }
      static void
   panfrost_destroy(struct pipe_context *pipe)
   {
      struct panfrost_context *panfrost = pan_context(pipe);
                     if (panfrost->blitter)
            util_unreference_framebuffer_state(&panfrost->pipe_framebuffer);
            panfrost_pool_cleanup(&panfrost->descs);
   panfrost_pool_cleanup(&panfrost->shaders);
            drmSyncobjDestroy(dev->fd, panfrost->in_sync_obj);
   if (panfrost->in_sync_fd != -1)
            drmSyncobjDestroy(dev->fd, panfrost->syncobj);
      }
      static struct pipe_query *
   panfrost_create_query(struct pipe_context *pipe, unsigned type, unsigned index)
   {
               q->type = type;
               }
      static void
   panfrost_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
   {
               if (query->rsrc)
               }
      static bool
   panfrost_begin_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct panfrost_context *ctx = pan_context(pipe);
   struct panfrost_device *dev = pan_device(ctx->base.screen);
            switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: {
               /* Allocate a resource for the query results to be stored */
   if (!query->rsrc) {
      query->rsrc = pipe_buffer_create(ctx->base.screen,
               /* Default to 0 if nothing at all drawn. */
   uint8_t *zeroes = alloca(size);
   memset(zeroes, 0, size);
            query->msaa = (ctx->pipe_framebuffer.samples > 1);
   ctx->occlusion_query = query;
   ctx->dirty |= PAN_DIRTY_OQ;
                  /* Geometry statistics are computed in the driver. XXX: geom/tess
         case PIPE_QUERY_PRIMITIVES_GENERATED:
      query->start = ctx->prims_generated;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      query->start = ctx->tf_prims_generated;
         case PAN_QUERY_DRAW_CALLS:
      query->start = ctx->draw_calls;
         default:
      /* TODO: timestamp queries, etc? */
                  }
      static bool
   panfrost_end_query(struct pipe_context *pipe, struct pipe_query *q)
   {
      struct panfrost_context *ctx = pan_context(pipe);
            switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      ctx->occlusion_query = NULL;
   ctx->dirty |= PAN_DIRTY_OQ;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      query->end = ctx->prims_generated;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      query->end = ctx->tf_prims_generated;
      case PAN_QUERY_DRAW_CALLS:
      query->end = ctx->draw_calls;
                  }
      static bool
   panfrost_get_query_result(struct pipe_context *pipe, struct pipe_query *q,
         {
      struct panfrost_query *query = (struct panfrost_query *)q;
   struct panfrost_context *ctx = pan_context(pipe);
   struct panfrost_device *dev = pan_device(ctx->base.screen);
            switch (query->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      panfrost_flush_writer(ctx, rsrc, "Occlusion query");
            /* Read back the query results */
            if (query->type == PIPE_QUERY_OCCLUSION_COUNTER) {
      uint64_t passed = 0;
                                    } else {
                        case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      panfrost_flush_all_batches(ctx, "Primitive count query");
   vresult->u64 = query->end - query->start;
         case PAN_QUERY_DRAW_CALLS:
      vresult->u64 = query->end - query->start;
         default:
      /* TODO: more queries */
                  }
      /*
   * Check the render condition for software condition rendering.
   *
   * Note: this may invalidate the batch!
   */
   bool
   panfrost_render_condition_check(struct panfrost_context *ctx)
   {
      if (!ctx->cond_query)
                     union pipe_query_result res = {0};
   bool wait = ctx->cond_mode != PIPE_RENDER_COND_NO_WAIT &&
                     if (panfrost_get_query_result(&ctx->base, pq, wait, &res))
               }
      static struct pipe_stream_output_target *
   panfrost_create_stream_output_target(struct pipe_context *pctx,
                     {
                        if (!target)
            pipe_reference_init(&target->reference, 1);
            target->context = pctx;
   target->buffer_offset = buffer_offset;
               }
      static void
   panfrost_stream_output_target_destroy(struct pipe_context *pctx,
         {
      pipe_resource_reference(&target->buffer, NULL);
      }
      static void
   panfrost_set_stream_output_targets(struct pipe_context *pctx,
                     {
      struct panfrost_context *ctx = pan_context(pctx);
                     for (unsigned i = 0; i < num_targets; i++) {
      if (targets[i] && offsets[i] != -1)
                        for (unsigned i = num_targets; i < so->num_targets; i++)
            so->num_targets = num_targets;
      }
      static void
   panfrost_set_global_binding(struct pipe_context *pctx, unsigned first,
               {
      if (!resources)
            struct panfrost_context *ctx = pan_context(pctx);
            for (unsigned i = first; i < first + count; ++i) {
      struct panfrost_resource *rsrc = pan_resource(resources[i]);
            util_range_add(&rsrc->base, &rsrc->valid_buffer_range, 0,
            /* The handle points to uint32_t, but space is allocated for 64
   * bits. We need to respect the offset passed in. This interface
   * is so bad.
   */
   mali_ptr addr = 0;
            memcpy(&addr, handles[i], sizeof(addr));
                  }
      static void
   panfrost_memory_barrier(struct pipe_context *pctx, unsigned flags)
   {
      /* TODO: Be smart and only flush the minimum needed, maybe emitting a
   * cache flush job if that would help */
      }
      static void
   panfrost_create_fence_fd(struct pipe_context *pctx,
               {
         }
      static void
   panfrost_fence_server_sync(struct pipe_context *pctx,
         {
      struct panfrost_device *dev = pan_device(pctx->screen);
   struct panfrost_context *ctx = pan_context(pctx);
            ret = drmSyncobjExportSyncFile(dev->fd, f->syncobj, &fd);
            sync_accumulate("panfrost", &ctx->in_sync_fd, fd);
      }
      struct pipe_context *
   panfrost_create_context(struct pipe_screen *screen, void *priv, unsigned flags)
   {
      struct panfrost_context *ctx = rzalloc(NULL, struct panfrost_context);
   struct pipe_context *gallium = (struct pipe_context *)ctx;
                              gallium->set_framebuffer_state = panfrost_set_framebuffer_state;
            gallium->create_fence_fd = panfrost_create_fence_fd;
            gallium->flush = panfrost_flush;
   gallium->clear = panfrost_clear;
   gallium->clear_texture = u_default_clear_texture;
   gallium->texture_barrier = panfrost_texture_barrier;
            gallium->set_vertex_buffers = panfrost_set_vertex_buffers;
   gallium->set_constant_buffer = panfrost_set_constant_buffer;
   gallium->set_shader_buffers = panfrost_set_shader_buffers;
                              gallium->bind_rasterizer_state = panfrost_bind_rasterizer_state;
            gallium->bind_vertex_elements_state = panfrost_bind_vertex_elements_state;
            gallium->delete_sampler_state = panfrost_generic_cso_delete;
            gallium->bind_depth_stencil_alpha_state = panfrost_bind_depth_stencil_state;
            gallium->set_sample_mask = panfrost_set_sample_mask;
            gallium->set_clip_state = panfrost_set_clip_state;
   gallium->set_viewport_states = panfrost_set_viewport_states;
   gallium->set_scissor_states = panfrost_set_scissor_states;
   gallium->set_polygon_stipple = panfrost_set_polygon_stipple;
   gallium->set_active_query_state = panfrost_set_active_query_state;
            gallium->create_query = panfrost_create_query;
   gallium->destroy_query = panfrost_destroy_query;
   gallium->begin_query = panfrost_begin_query;
   gallium->end_query = panfrost_end_query;
            gallium->create_stream_output_target = panfrost_create_stream_output_target;
   gallium->stream_output_target_destroy =
                  gallium->bind_blend_state = panfrost_bind_blend_state;
                     gallium->set_global_binding = panfrost_set_global_binding;
                     panfrost_resource_context_init(gallium);
   panfrost_shader_context_init(gallium);
            gallium->stream_uploader = u_upload_create_default(gallium);
            panfrost_pool_init(&ctx->descs, ctx, dev, 0, 4096, "Descriptors", true,
            panfrost_pool_init(&ctx->shaders, ctx, dev, PAN_BO_EXECUTE, 4096, "Shaders",
                     ctx->writers = _mesa_hash_table_create(gallium, _mesa_hash_pointer,
                              /* By default mask everything on */
   ctx->sample_mask = ~0;
                     /* Create a syncobj in a signaled state. Will be updated to point to the
   * last queued job out_sync every time we submit a new job.
   */
   ret = drmSyncobjCreate(dev->fd, DRM_SYNCOBJ_CREATE_SIGNALED, &ctx->syncobj);
            /* Sync object/FD used for NATIVE_FENCE_FD. */
   ctx->in_sync_fd = -1;
   ret = drmSyncobjCreate(dev->fd, 0, &ctx->in_sync_obj);
               }
