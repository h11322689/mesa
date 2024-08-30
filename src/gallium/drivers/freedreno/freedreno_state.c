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
   #include "util/u_dual_blend.h"
   #include "util/u_helpers.h"
   #include "util/u_memory.h"
   #include "util/u_string.h"
   #include "util/u_upload_mgr.h"
      #include "common/freedreno_guardband.h"
      #include "freedreno_context.h"
   #include "freedreno_gmem.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_resource.h"
   #include "freedreno_state.h"
   #include "freedreno_texture.h"
   #include "freedreno_util.h"
      #define get_safe(ptr, field) ((ptr) ? (ptr)->field : 0)
      /* All the generic state handling.. In case of CSO's that are specific
   * to the GPU version, when the bind and the delete are common they can
   * go in here.
   */
      static void
   update_draw_cost(struct fd_context *ctx) assert_dt
   {
               ctx->draw_cost = pfb->nr_cbufs;
   for (unsigned i = 0; i < pfb->nr_cbufs; i++)
      if (fd_blend_enabled(ctx, i))
      if (fd_depth_enabled(ctx))
         if (fd_depth_write_enabled(ctx))
      }
      static void
   fd_set_blend_color(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
   ctx->blend_color = *blend_color;
      }
      static void
   fd_set_stencil_ref(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
   ctx->stencil_ref = stencil_ref;
      }
      static void
   fd_set_clip_state(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
   ctx->ucp = *clip;
      }
      static void
   fd_set_sample_mask(struct pipe_context *pctx, unsigned sample_mask) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->sample_mask = (uint16_t)sample_mask;
      }
      static void
   fd_set_sample_locations(struct pipe_context *pctx, size_t size,
         in_dt
   {
               if (!locations) {
      ctx->sample_locations_enabled = false;
               size = MIN2(size, sizeof(ctx->sample_locations));
   memcpy(ctx->sample_locations, locations, size);
               }
      static void
   fd_set_min_samples(struct pipe_context *pctx, unsigned min_samples) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->min_samples = min_samples;
      }
      static void
   upload_user_buffer(struct pipe_context *pctx, struct pipe_constant_buffer *cb)
   {
      u_upload_data(pctx->stream_uploader, 0, cb->buffer_size, 64,
            }
      /* notes from calim on #dri-devel:
   * index==0 will be non-UBO (ie. glUniformXYZ()) all packed together padded
   * out to vec4's
   * I should be able to consider that I own the user_ptr until the next
   * set_constant_buffer() call, at which point I don't really care about the
   * previous values.
   * index>0 will be UBO's.. well, I'll worry about that later
   */
   static void
   fd_set_constant_buffer(struct pipe_context *pctx, enum pipe_shader_type shader,
               {
      struct fd_context *ctx = fd_context(pctx);
                     /* Note that gallium frontends can unbind constant buffers by
   * passing NULL here.
   */
   if (unlikely(!cb)) {
      so->enabled_mask &= ~(1 << index);
               if (cb->user_buffer && ctx->screen->gen >= 6) {
      upload_user_buffer(pctx, &so->cb[index]);
                        fd_context_dirty_shader(ctx, shader, FD_DIRTY_SHADER_CONST);
   fd_resource_set_usage(cb->buffer, FD_DIRTY_CONST);
      }
      void
   fd_set_shader_buffers(struct pipe_context *pctx, enum pipe_shader_type shader,
                     {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_shaderbuf_stateobj *so = &ctx->shaderbuf[shader];
            so->writable_mask &= ~modified_bits;
            for (unsigned i = 0; i < count; i++) {
      unsigned n = i + start;
            if (buffers && buffers[i].buffer) {
      buf->buffer_offset = buffers[i].buffer_offset;
                           fd_resource_set_usage(buffers[i].buffer, FD_DIRTY_SSBO);
                           if (write) {
      struct fd_resource *rsc = fd_resource(buf->buffer);
   util_range_add(&rsc->b.b, &rsc->valid_buffer_range,
               } else {
                                 }
      void
   fd_set_shader_images(struct pipe_context *pctx, enum pipe_shader_type shader,
                     {
      struct fd_context *ctx = fd_context(pctx);
                     if (images) {
      for (unsigned i = 0; i < count; i++) {
                     if ((buf->resource == images[i].resource) &&
      (buf->format == images[i].format) &&
   (buf->access == images[i].access) &&
                                                fd_resource_set_usage(buf->resource, FD_DIRTY_IMAGE);
   fd_dirty_shader_resource(ctx, buf->resource, shader,
                  if (write && (buf->resource->target == PIPE_BUFFER)) {
      struct fd_resource *rsc = fd_resource(buf->resource);
   util_range_add(&rsc->b.b, &rsc->valid_buffer_range,
               } else {
               } else {
               for (unsigned i = 0; i < count; i++) {
                                             for (unsigned i = 0; i < unbind_num_trailing_slots; i++)
            so->enabled_mask &=
               }
      void
   fd_set_framebuffer_state(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
            DBG("%ux%u, %u layers, %u samples", framebuffer->width, framebuffer->height,
                     if (util_framebuffer_state_equal(cso, framebuffer))
            /* Do this *after* checking that the framebuffer state is actually
   * changing.  In the fd_blitter_clear() path, we get a pfb update
   * to restore the current pfb state, which should not trigger us
   * to flush (as that can cause the batch to be freed at a point
   * before fd_clear() returns, but after the point where it expects
   * flushes to potentially happen.
   */
                     STATIC_ASSERT((4 * PIPE_MAX_COLOR_BUFS) == (8 * sizeof(ctx->all_mrt_channel_mask)));
            /* Generate a bitmask of all valid channels for all MRTs.  Blend
   * state with unwritten channels essentially acts as blend enabled,
   * which disables LRZ write.  But only if the cbuf *has* the masked
   * channels, which is not known at the time the blend state is
   * created.
   */
   for (unsigned i = 0; i < framebuffer->nr_cbufs; i++) {
      if (!framebuffer->cbufs[i])
            enum pipe_format format = framebuffer->cbufs[i]->format;
                                 if (ctx->screen->reorder) {
                        if (likely(old_batch))
            fd_batch_reference(&ctx->batch, NULL);
               } else if (ctx->batch) {
      DBG("%d: cbufs[0]=%p, zsbuf=%p", ctx->batch->needs_flush,
                              for (unsigned i = 0; i < PIPE_MAX_VIEWPORTS; i++) {
      ctx->disabled_scissor[i].minx = 0;
   ctx->disabled_scissor[i].miny = 0;
   ctx->disabled_scissor[i].maxx = cso->width - 1;
               fd_context_dirty(ctx, FD_DIRTY_SCISSOR);
      }
      static void
   fd_set_polygon_stipple(struct pipe_context *pctx,
         {
      struct fd_context *ctx = fd_context(pctx);
   ctx->stipple = *stipple;
      }
      static void
   fd_set_scissor_states(struct pipe_context *pctx, unsigned start_slot,
               {
               for (unsigned i = 0; i < num_scissors; i++) {
               if ((scissor[i].minx == scissor[i].maxx) ||
      (scissor[i].miny == scissor[i].maxy)) {
   ctx->scissor[idx].minx = ctx->scissor[idx].miny = 1;
      } else {
      ctx->scissor[idx].minx = scissor[i].minx;
   ctx->scissor[idx].miny = scissor[i].miny;
   ctx->scissor[idx].maxx = MAX2(scissor[i].maxx, 1) - 1;
                     }
      static void
   init_scissor_states(struct pipe_context *pctx)
         {
               for (unsigned idx = 0; idx < ARRAY_SIZE(ctx->scissor); idx++) {
      ctx->scissor[idx].minx = ctx->scissor[idx].miny = 1;
         }
      static void
   fd_set_viewport_states(struct pipe_context *pctx, unsigned start_slot,
               {
               for (unsigned i = 0; i < num_viewports; i++) {
      unsigned idx = start_slot + i;
   struct pipe_scissor_state *scissor = &ctx->viewport_scissor[idx];
                              /* Convert (-1, -1) and (1, 1) from clip space into window space. */
   float minx = -viewport->scale[0] + viewport->translate[0];
   float miny = -viewport->scale[1] + viewport->translate[1];
   float maxx = viewport->scale[0] + viewport->translate[0];
            /* Handle inverted viewports. */
   if (minx > maxx) {
         }
   if (miny > maxy) {
                           /* Clamp, convert to integer and round up the max bounds. */
   scissor->minx = CLAMP(minx, 0.f, max_dims);
   scissor->miny = CLAMP(miny, 0.f, max_dims);
   scissor->maxx = MAX2(CLAMP(ceilf(maxx), 0.f, max_dims), 1) - 1;
                        /* Guardband is only used on a6xx so far: */
   if (!is_a6xx(ctx->screen))
            ctx->guardband.x = ~0;
                     for (unsigned i = 0; i < PIPE_MAX_VIEWPORTS; i++) {
               /* skip unused viewports: */
   if (vp->scale[0] == 0)
            unsigned gx = fd_calc_guardband(vp->translate[0], vp->scale[0], is3x);
            ctx->guardband.x = MIN2(ctx->guardband.x, gx);
         }
      static void
   fd_set_vertex_buffers(struct pipe_context *pctx,
                     {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_vertexbuf_stateobj *so = &ctx->vtx.vertexbuf;
            /* on a2xx, pitch is encoded in the vtx fetch instruction, so
   * we need to mark VTXSTATE as dirty as well to trigger patching
   * and re-emitting the vtx shader:
   */
   if (ctx->screen->gen < 3) {
      for (i = 0; i < count; i++) {
      bool new_enabled = vb && vb[i].buffer.resource;
   bool old_enabled = so->vb[i].buffer.resource != NULL;
   if (new_enabled != old_enabled) {
      fd_context_dirty(ctx, FD_DIRTY_VTXSTATE);
                     util_set_vertex_buffers_mask(so->vb, &so->enabled_mask, vb,
                        if (!vb)
                     for (unsigned i = 0; i < count; i++) {
      assert(!vb[i].is_user_buffer);
   fd_resource_set_usage(vb[i].buffer.resource, FD_DIRTY_VTXBUF);
            /* Robust buffer access: Return undefined data (the start of the buffer)
   * instead of process termination or a GPU hang in case of overflow.
   */
   if (vb[i].buffer.resource &&
      unlikely(vb[i].buffer_offset >= vb[i].buffer.resource->width0)) {
            }
      static void
   fd_blend_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   struct pipe_blend_state *cso = hwcso;
   bool old_is_dual = ctx->blend ? ctx->blend->rt[0].blend_enable &&
               bool new_is_dual =
         fd_context_dirty(ctx, FD_DIRTY_BLEND);
   if (old_is_dual != new_is_dual)
            bool old_coherent = get_safe(ctx->blend, blend_coherent);
   bool new_coherent = get_safe(cso, blend_coherent);
   if (new_coherent != old_coherent) {
         }
   ctx->blend = hwcso;
      }
      static void
   fd_blend_state_delete(struct pipe_context *pctx, void *hwcso) in_dt
   {
         }
      static void
   fd_rasterizer_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   struct pipe_scissor_state *old_scissor = fd_context_get_scissor(ctx);
   bool discard = get_safe(ctx->rasterizer, rasterizer_discard);
            ctx->rasterizer = hwcso;
            if (ctx->rasterizer && ctx->rasterizer->scissor) {
         } else {
                  /* if scissor enable bit changed we need to mark scissor
   * state as dirty as well:
   * NOTE: we can do a shallow compare, since we only care
   * if it changed to/from &ctx->disable_scissor
   */
   if (old_scissor != fd_context_get_scissor(ctx))
            if (discard != get_safe(ctx->rasterizer, rasterizer_discard))
            if (clip_plane_enable != get_safe(ctx->rasterizer, clip_plane_enable))
      }
      static void
   fd_rasterizer_state_delete(struct pipe_context *pctx, void *hwcso) in_dt
   {
         }
      static void
   fd_zsa_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->zsa = hwcso;
   fd_context_dirty(ctx, FD_DIRTY_ZSA);
      }
      static void
   fd_zsa_state_delete(struct pipe_context *pctx, void *hwcso) in_dt
   {
         }
      static void *
   fd_vertex_state_create(struct pipe_context *pctx, unsigned num_elements,
         {
               if (!so)
            memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
   so->num_elements = num_elements;
   for (unsigned i = 0; i < num_elements; i++)
               }
      static void
   fd_vertex_state_delete(struct pipe_context *pctx, void *hwcso) in_dt
   {
         }
      static void
   fd_vertex_state_bind(struct pipe_context *pctx, void *hwcso) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->vtx.vtx = hwcso;
      }
      static struct pipe_stream_output_target *
   fd_create_stream_output_target(struct pipe_context *pctx,
               {
      struct fd_stream_output_target *target;
            target = CALLOC_STRUCT(fd_stream_output_target);
   if (!target)
            pipe_reference_init(&target->base.reference, 1);
            target->base.context = pctx;
   target->base.buffer_offset = buffer_offset;
            target->offset_buf = pipe_buffer_create(
            assert(rsc->b.b.target == PIPE_BUFFER);
   util_range_add(&rsc->b.b, &rsc->valid_buffer_range, buffer_offset,
               }
      static void
   fd_stream_output_target_destroy(struct pipe_context *pctx,
         {
               pipe_resource_reference(&cso->base.buffer, NULL);
               }
      static void
   fd_set_stream_output_targets(struct pipe_context *pctx, unsigned num_targets,
               {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_streamout_stateobj *so = &ctx->streamout;
                     /* Older targets need sw stats enabled for streamout emulation in VS: */
   if (ctx->screen->gen < 5) {
      if (num_targets && !so->num_targets) {
         } else if (so->num_targets && !num_targets) {
                     for (i = 0; i < num_targets; i++) {
      bool changed = targets[i] != so->targets[i];
                     if (targets[i]) {
                     struct fd_stream_output_target *target = fd_stream_output_target(targets[i]);
   fd_resource_set_usage(target->offset_buf, FD_DIRTY_STREAMOUT);
               if (!changed && !reset)
            /* Note that all SO targets will be reset at once at a
   * BeginTransformFeedback().
   */
   if (reset) {
      so->offsets[i] = offsets[i];
                           for (; i < so->num_targets; i++) {
                              }
      static void
   fd_bind_compute_state(struct pipe_context *pctx, void *state) in_dt
   {
      struct fd_context *ctx = fd_context(pctx);
   ctx->compute = state;
      }
      /* TODO pipe_context::set_compute_resources() should DIAF and clover
   * should be updated to use pipe_context::set_constant_buffer() and
   * pipe_context::set_shader_images().  Until then just directly frob
   * the UBO/image state to avoid the rest of the driver needing to
   * know about this bastard api..
   */
   static void
   fd_set_compute_resources(struct pipe_context *pctx, unsigned start,
         {
      struct fd_context *ctx = fd_context(pctx);
            for (unsigned i = 0; i < count; i++) {
               if (!prscs) {
      util_copy_constant_buffer(&so->cb[index], NULL, false);
      } else if (prscs[i]->format == PIPE_FORMAT_NONE) {
      struct pipe_constant_buffer cb = {
         };
   util_copy_constant_buffer(&so->cb[index], &cb, false);
      } else {
      // TODO images
            }
      /* used by clover to bind global objects, returning the bo address
   * via handles[n]
   */
   static void
   fd_set_global_binding(struct pipe_context *pctx, unsigned first, unsigned count,
         {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_global_bindings_stateobj *so = &ctx->global_bindings;
            if (prscs) {
      for (unsigned i = 0; i < count; i++) {
                                 if (so->buf[n]) {
      struct fd_resource *rsc = fd_resource(so->buf[n]);
                  /* Yes, really, despite what the type implies: */
               if (prscs[i])
         else
         } else {
               for (unsigned i = 0; i < count; i++) {
      unsigned n = i + first;
                     }
      void
   fd_state_init(struct pipe_context *pctx)
   {
      pctx->set_blend_color = fd_set_blend_color;
   pctx->set_stencil_ref = fd_set_stencil_ref;
   pctx->set_clip_state = fd_set_clip_state;
   pctx->set_sample_mask = fd_set_sample_mask;
   pctx->set_min_samples = fd_set_min_samples;
   pctx->set_constant_buffer = fd_set_constant_buffer;
   pctx->set_shader_buffers = fd_set_shader_buffers;
   pctx->set_shader_images = fd_set_shader_images;
   pctx->set_framebuffer_state = fd_set_framebuffer_state;
   pctx->set_sample_locations = fd_set_sample_locations;
   pctx->set_polygon_stipple = fd_set_polygon_stipple;
   pctx->set_scissor_states = fd_set_scissor_states;
                     pctx->bind_blend_state = fd_blend_state_bind;
            pctx->bind_rasterizer_state = fd_rasterizer_state_bind;
            pctx->bind_depth_stencil_alpha_state = fd_zsa_state_bind;
            if (!pctx->create_vertex_elements_state)
         pctx->delete_vertex_elements_state = fd_vertex_state_delete;
            pctx->create_stream_output_target = fd_create_stream_output_target;
   pctx->stream_output_target_destroy = fd_stream_output_target_destroy;
            if (has_compute(fd_screen(pctx->screen))) {
      pctx->bind_compute_state = fd_bind_compute_state;
   pctx->set_compute_resources = fd_set_compute_resources;
                  }
