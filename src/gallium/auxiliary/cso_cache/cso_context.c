   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * @file
   *
   * Wrap the cso cache & hash mechanisms in a simplified
   * pipe-driver-specific interface.
   *
   * @author Zack Rusin <zackr@vmware.com>
   * @author Keith Whitwell <keithw@vmware.com>
   */
      #include "pipe/p_state.h"
   #include "util/u_draw.h"
   #include "util/u_framebuffer.h"
   #include "util/u_helpers.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_vbuf.h"
      #include "cso_cache/cso_context.h"
   #include "cso_cache/cso_cache.h"
   #include "cso_cache/cso_hash.h"
   #include "cso_context.h"
   #include "driver_trace/tr_dump.h"
   #include "util/u_threaded_context.h"
      /**
   * Per-shader sampler information.
   */
   struct sampler_info
   {
      struct cso_sampler *cso_samplers[PIPE_MAX_SAMPLERS];
      };
            struct cso_context {
               struct u_vbuf *vbuf;
   struct u_vbuf *vbuf_current;
   bool always_use_vbuf;
            bool has_geometry_shader;
   bool has_tessellation;
   bool has_compute_shader;
   bool has_task_mesh_shader;
                     unsigned saved_state;  /**< bitmask of CSO_BIT_x flags */
            struct sampler_info fragment_samplers_saved;
   struct sampler_info compute_samplers_saved;
            /* Temporary number until cso_single_sampler_done is called.
   * It tracks the highest sampler seen in cso_single_sampler.
   */
            unsigned nr_so_targets;
            unsigned nr_so_targets_saved;
            /** Current and saved state.
   * The saved state is used as a 1-deep stack.
   */
   void *blend, *blend_saved;
   void *depth_stencil, *depth_stencil_saved;
   void *rasterizer, *rasterizer_saved;
   void *fragment_shader, *fragment_shader_saved;
   void *vertex_shader, *vertex_shader_saved;
   void *geometry_shader, *geometry_shader_saved;
   void *tessctrl_shader, *tessctrl_shader_saved;
   void *tesseval_shader, *tesseval_shader_saved;
   void *compute_shader, *compute_shader_saved;
   void *velements, *velements_saved;
   struct pipe_query *render_condition, *render_condition_saved;
   enum pipe_render_cond_flag render_condition_mode, render_condition_mode_saved;
   bool render_condition_cond, render_condition_cond_saved;
            struct pipe_framebuffer_state fb, fb_saved;
   struct pipe_viewport_state vp, vp_saved;
   unsigned sample_mask, sample_mask_saved;
   unsigned min_samples, min_samples_saved;
            /* This should be last to keep all of the above together in memory. */
      };
         static inline bool
   delete_cso(struct cso_context *ctx,
         {
      switch (type) {
   case CSO_BLEND:
      if (ctx->blend == ((struct cso_blend*)state)->data ||
      ctx->blend_saved == ((struct cso_blend*)state)->data)
         case CSO_DEPTH_STENCIL_ALPHA:
      if (ctx->depth_stencil == ((struct cso_depth_stencil_alpha*)state)->data ||
      ctx->depth_stencil_saved == ((struct cso_depth_stencil_alpha*)state)->data)
         case CSO_RASTERIZER:
      if (ctx->rasterizer == ((struct cso_rasterizer*)state)->data ||
      ctx->rasterizer_saved == ((struct cso_rasterizer*)state)->data)
         case CSO_VELEMENTS:
      if (ctx->velements == ((struct cso_velements*)state)->data ||
      ctx->velements_saved == ((struct cso_velements*)state)->data)
         case CSO_SAMPLER:
      /* nothing to do for samplers */
      default:
                  cso_delete_state(ctx->base.pipe, state, type);
      }
         static inline void
   sanitize_hash(struct cso_hash *hash, enum cso_cache_type type,
         {
      struct cso_context *ctx = (struct cso_context *)user_data;
   /* if we're approach the maximum size, remove fourth of the entries
   * otherwise every subsequent call will go through the same */
   const int hash_size = cso_hash_size(hash);
   const int max_entries = (max_size > hash_size) ? max_size : hash_size;
   int to_remove =  (max_size < max_entries) * max_entries/4;
   struct cso_sampler **samplers_to_restore = NULL;
            if (hash_size > max_size)
            if (to_remove == 0)
            if (type == CSO_SAMPLER) {
      samplers_to_restore = MALLOC((PIPE_SHADER_MESH_TYPES + 2) * PIPE_MAX_SAMPLERS *
            /* Temporarily remove currently bound sampler states from the hash
   * table, to prevent them from being deleted
   */
   for (int i = 0; i < PIPE_SHADER_MESH_TYPES; i++) {
                        if (sampler && cso_hash_take(hash, sampler->hash_key))
         }
   for (int j = 0; j < PIPE_MAX_SAMPLERS; j++) {
               if (sampler && cso_hash_take(hash, sampler->hash_key))
      }
   for (int j = 0; j < PIPE_MAX_SAMPLERS; j++) {
               if (sampler && cso_hash_take(hash, sampler->hash_key))
                  struct cso_hash_iter iter = cso_hash_first_node(hash);
   while (to_remove) {
      /*remove elements until we're good */
   /*fixme: currently we pick the nodes to remove at random*/
            if (!cso)
            if (delete_cso(ctx, cso, type)) {
      iter = cso_hash_erase(hash, iter);
      } else {
                     if (type == CSO_SAMPLER) {
      /* Put currently bound sampler states back into the hash table */
   while (to_restore--) {
                                 }
         static void
   cso_init_vbuf(struct cso_context *cso, unsigned flags)
   {
      struct u_vbuf_caps caps;
   bool uses_user_vertex_buffers = !(flags & CSO_NO_USER_VERTEX_BUFFERS);
                     /* Enable u_vbuf if needed. */
   if (caps.fallback_always ||
      (uses_user_vertex_buffers &&
   caps.fallback_only_for_user_vbuffers)) {
   assert(!cso->base.pipe->vbuf);
   cso->vbuf = u_vbuf_create(cso->base.pipe, &caps);
   cso->base.pipe->vbuf = cso->vbuf;
   cso->always_use_vbuf = caps.fallback_always;
   cso->vbuf_current = cso->base.pipe->vbuf =
         }
      static void
   cso_draw_vbo_default(struct pipe_context *pipe,
                        const struct pipe_draw_info *info,
      {
      if (pipe->vbuf)
         else
      }
      struct cso_context *
   cso_create_context(struct pipe_context *pipe, unsigned flags)
   {
      struct cso_context *ctx = CALLOC_STRUCT(cso_context);
   if (!ctx)
            cso_cache_init(&ctx->cache, pipe);
            ctx->base.pipe = pipe;
            if (!(flags & CSO_NO_VBUF))
            /* Only drivers using u_threaded_context benefit from the direct call.
   * This is because drivers can change draw_vbo, but u_threaded_context
   * never changes it.
   */
   if (pipe->draw_vbo == tc_draw_vbo) {
      if (ctx->vbuf_current)
         else
      } else if (ctx->always_use_vbuf) {
         } else {
                  /* Enable for testing: */
            if (pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_GEOMETRY,
               }
   if (pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_TESS_CTRL,
               }
   if (pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_COMPUTE,
            int supported_irs =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_COMPUTE,
      if (supported_irs & ((1 << PIPE_SHADER_IR_TGSI) |
                  }
   if (pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_MESH,
               }
   if (pipe->screen->get_param(pipe->screen,
                        if (pipe->screen->get_param(pipe->screen,
            PIPE_QUIRK_TEXTURE_BORDER_COLOR_SWIZZLE_FREEDRENO)
         ctx->max_fs_samplerviews =
      pipe->screen->get_shader_param(pipe->screen, PIPE_SHADER_FRAGMENT,
         ctx->max_sampler_seen = -1;
      }
         void
   cso_unbind_context(struct cso_context *ctx)
   {
               bool dumping = trace_dumping_enabled_locked();
   if (dumping)
         if (ctx->base.pipe) {
      ctx->base.pipe->bind_blend_state(ctx->base.pipe, NULL);
            {
      static struct pipe_sampler_view *views[PIPE_MAX_SHADER_SAMPLER_VIEWS] = { NULL };
   static struct pipe_shader_buffer ssbos[PIPE_MAX_SHADER_BUFFERS] = { 0 };
   static void *zeros[PIPE_MAX_SAMPLERS] = { NULL };
   struct pipe_screen *scr = ctx->base.pipe->screen;
   enum pipe_shader_type sh;
   for (sh = 0; sh < PIPE_SHADER_MESH_TYPES; sh++) {
      switch (sh) {
   case PIPE_SHADER_GEOMETRY:
      if (!ctx->has_geometry_shader)
            case PIPE_SHADER_TESS_CTRL:
   case PIPE_SHADER_TESS_EVAL:
      if (!ctx->has_tessellation)
            case PIPE_SHADER_COMPUTE:
      if (!ctx->has_compute_shader)
            case PIPE_SHADER_MESH:
   case PIPE_SHADER_TASK:
      if (!ctx->has_task_mesh_shader)
            default:
                  int maxsam = scr->get_shader_param(scr, sh,
         int maxview = scr->get_shader_param(scr, sh,
         int maxssbo = scr->get_shader_param(scr, sh,
         int maxcb = scr->get_shader_param(scr, sh,
         int maximg = scr->get_shader_param(scr, sh,
         assert(maxsam <= PIPE_MAX_SAMPLERS);
   assert(maxview <= PIPE_MAX_SHADER_SAMPLER_VIEWS);
   assert(maxssbo <= PIPE_MAX_SHADER_BUFFERS);
   assert(maxcb <= PIPE_MAX_CONSTANT_BUFFERS);
   assert(maximg <= PIPE_MAX_SHADER_IMAGES);
   if (maxsam > 0) {
         }
   if (maxview > 0) {
         }
   if (maxssbo > 0) {
         }
   if (maximg > 0) {
         }
   for (int i = 0; i < maxcb; i++) {
                        ctx->base.pipe->bind_depth_stencil_alpha_state(ctx->base.pipe, NULL);
   struct pipe_stencil_ref sr = {0};
   ctx->base.pipe->set_stencil_ref(ctx->base.pipe, sr);
   ctx->base.pipe->bind_fs_state(ctx->base.pipe, NULL);
   ctx->base.pipe->set_constant_buffer(ctx->base.pipe, PIPE_SHADER_FRAGMENT, 0, false, NULL);
   ctx->base.pipe->bind_vs_state(ctx->base.pipe, NULL);
   ctx->base.pipe->set_constant_buffer(ctx->base.pipe, PIPE_SHADER_VERTEX, 0, false, NULL);
   if (ctx->has_geometry_shader) {
         }
   if (ctx->has_tessellation) {
      ctx->base.pipe->bind_tcs_state(ctx->base.pipe, NULL);
      }
   if (ctx->has_compute_shader) {
         }
   if (ctx->has_task_mesh_shader) {
      ctx->base.pipe->bind_ts_state(ctx->base.pipe, NULL);
      }
            if (ctx->has_streamout)
            struct pipe_framebuffer_state fb = {0};
               util_unreference_framebuffer_state(&ctx->fb);
            for (i = 0; i < PIPE_MAX_SO_BUFFERS; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
               memset(&ctx->samplers, 0, sizeof(ctx->samplers));
   memset(&ctx->nr_so_targets, 0,
         offsetof(struct cso_context, cache)
   ctx->sample_mask = ~0;
   /*
   * If the cso context is reused (with the same pipe context),
   * need to really make sure the context state doesn't get out of sync.
   */
   ctx->base.pipe->set_sample_mask(ctx->base.pipe, ctx->sample_mask);
   if (ctx->base.pipe->set_min_samples)
         if (dumping)
      }
         /**
   * Free the CSO context.
   */
   void
   cso_destroy_context(struct cso_context *ctx)
   {
      cso_unbind_context(ctx);
            if (ctx->vbuf)
            ctx->base.pipe->vbuf = NULL;
      }
         /* Those function will either find the state of the given template
   * in the cache or they will create a new state from the given
   * template, insert it in the cache and return it.
   */
      #define CSO_BLEND_KEY_SIZE_RT0      offsetof(struct pipe_blend_state, rt[1])
   #define CSO_BLEND_KEY_SIZE_ALL_RT   sizeof(struct pipe_blend_state)
      /*
   * If the driver returns 0 from the create method then they will assign
   * the data member of the cso to be the template itself.
   */
      enum pipe_error
   cso_set_blend(struct cso_context *ctx,
         {
      unsigned key_size, hash_key;
   struct cso_hash_iter iter;
            if (templ->independent_blend_enable) {
      /* This is duplicated with the else block below because we want key_size
   * to be a literal constant, so that memcpy and the hash computation can
   * be inlined and unrolled.
   */
   hash_key = cso_construct_key(templ, CSO_BLEND_KEY_SIZE_ALL_RT);
   iter = cso_find_state_template(&ctx->cache, hash_key, CSO_BLEND,
            } else {
      hash_key = cso_construct_key(templ, CSO_BLEND_KEY_SIZE_RT0);
   iter = cso_find_state_template(&ctx->cache, hash_key, CSO_BLEND,
                     if (cso_hash_iter_is_null(iter)) {
      struct cso_blend *cso = MALLOC(sizeof(struct cso_blend));
   if (!cso)
            memset(&cso->state, 0, sizeof cso->state);
   memcpy(&cso->state, templ, key_size);
            iter = cso_insert_state(&ctx->cache, hash_key, CSO_BLEND, cso);
   if (cso_hash_iter_is_null(iter)) {
      FREE(cso);
                  } else {
                  if (ctx->blend != handle) {
      ctx->blend = handle;
      }
      }
         static void
   cso_save_blend(struct cso_context *ctx)
   {
      assert(!ctx->blend_saved);
      }
         static void
   cso_restore_blend(struct cso_context *ctx)
   {
      if (ctx->blend != ctx->blend_saved) {
      ctx->blend = ctx->blend_saved;
      }
      }
         enum pipe_error
   cso_set_depth_stencil_alpha(struct cso_context *ctx,
         {
      const unsigned key_size = sizeof(struct pipe_depth_stencil_alpha_state);
   const unsigned hash_key = cso_construct_key(templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(&ctx->cache,
                              if (cso_hash_iter_is_null(iter)) {
      struct cso_depth_stencil_alpha *cso =
         if (!cso)
            memcpy(&cso->state, templ, sizeof(*templ));
   cso->data = ctx->base.pipe->create_depth_stencil_alpha_state(ctx->base.pipe,
            iter = cso_insert_state(&ctx->cache, hash_key,
         if (cso_hash_iter_is_null(iter)) {
      FREE(cso);
                  } else {
      handle = ((struct cso_depth_stencil_alpha *)
               if (ctx->depth_stencil != handle) {
      ctx->depth_stencil = handle;
      }
      }
         static void
   cso_save_depth_stencil_alpha(struct cso_context *ctx)
   {
      assert(!ctx->depth_stencil_saved);
      }
         static void
   cso_restore_depth_stencil_alpha(struct cso_context *ctx)
   {
      if (ctx->depth_stencil != ctx->depth_stencil_saved) {
      ctx->depth_stencil = ctx->depth_stencil_saved;
   ctx->base.pipe->bind_depth_stencil_alpha_state(ctx->base.pipe,
      }
      }
         enum pipe_error
   cso_set_rasterizer(struct cso_context *ctx,
         {
      const unsigned key_size = sizeof(struct pipe_rasterizer_state);
   const unsigned hash_key = cso_construct_key(templ, key_size);
   struct cso_hash_iter iter = cso_find_state_template(&ctx->cache,
                              /* We can't have both point_quad_rasterization (sprites) and point_smooth
   * (round AA points) enabled at the same time.
   */
            if (cso_hash_iter_is_null(iter)) {
      struct cso_rasterizer *cso = MALLOC(sizeof(struct cso_rasterizer));
   if (!cso)
            memcpy(&cso->state, templ, sizeof(*templ));
            iter = cso_insert_state(&ctx->cache, hash_key, CSO_RASTERIZER, cso);
   if (cso_hash_iter_is_null(iter)) {
      FREE(cso);
                  } else {
                  if (ctx->rasterizer != handle) {
      ctx->rasterizer = handle;
   ctx->flatshade_first = templ->flatshade_first;
   if (ctx->vbuf)
            }
      }
         static void
   cso_save_rasterizer(struct cso_context *ctx)
   {
      assert(!ctx->rasterizer_saved);
   ctx->rasterizer_saved = ctx->rasterizer;
      }
         static void
   cso_restore_rasterizer(struct cso_context *ctx)
   {
      if (ctx->rasterizer != ctx->rasterizer_saved) {
      ctx->rasterizer = ctx->rasterizer_saved;
   ctx->flatshade_first = ctx->flatshade_first_saved;
   if (ctx->vbuf)
            }
      }
         void
   cso_set_fragment_shader_handle(struct cso_context *ctx, void *handle)
   {
      if (ctx->fragment_shader != handle) {
      ctx->fragment_shader = handle;
         }
         static void
   cso_save_fragment_shader(struct cso_context *ctx)
   {
      assert(!ctx->fragment_shader_saved);
      }
         static void
   cso_restore_fragment_shader(struct cso_context *ctx)
   {
      if (ctx->fragment_shader_saved != ctx->fragment_shader) {
      ctx->base.pipe->bind_fs_state(ctx->base.pipe, ctx->fragment_shader_saved);
      }
      }
         void
   cso_set_vertex_shader_handle(struct cso_context *ctx, void *handle)
   {
      if (ctx->vertex_shader != handle) {
      ctx->vertex_shader = handle;
         }
         static void
   cso_save_vertex_shader(struct cso_context *ctx)
   {
      assert(!ctx->vertex_shader_saved);
      }
         static void
   cso_restore_vertex_shader(struct cso_context *ctx)
   {
      if (ctx->vertex_shader_saved != ctx->vertex_shader) {
      ctx->base.pipe->bind_vs_state(ctx->base.pipe, ctx->vertex_shader_saved);
      }
      }
         void
   cso_set_framebuffer(struct cso_context *ctx,
         {
      if (memcmp(&ctx->fb, fb, sizeof(*fb)) != 0) {
      util_copy_framebuffer_state(&ctx->fb, fb);
         }
         static void
   cso_save_framebuffer(struct cso_context *ctx)
   {
         }
         static void
   cso_restore_framebuffer(struct cso_context *ctx)
   {
      if (memcmp(&ctx->fb, &ctx->fb_saved, sizeof(ctx->fb))) {
      util_copy_framebuffer_state(&ctx->fb, &ctx->fb_saved);
   ctx->base.pipe->set_framebuffer_state(ctx->base.pipe, &ctx->fb);
         }
         void
   cso_set_viewport(struct cso_context *ctx,
         {
      if (memcmp(&ctx->vp, vp, sizeof(*vp))) {
      ctx->vp = *vp;
         }
         /**
   * Setup viewport state for given width and height (position is always (0,0)).
   * Invert the Y axis if 'invert' is true.
   */
   void
   cso_set_viewport_dims(struct cso_context *ctx,
         {
      struct pipe_viewport_state vp;
   vp.scale[0] = width * 0.5f;
   vp.scale[1] = height * (invert ? -0.5f : 0.5f);
   vp.scale[2] = 0.5f;
   vp.translate[0] = 0.5f * width;
   vp.translate[1] = 0.5f * height;
   vp.translate[2] = 0.5f;
   vp.swizzle_x = PIPE_VIEWPORT_SWIZZLE_POSITIVE_X;
   vp.swizzle_y = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Y;
   vp.swizzle_z = PIPE_VIEWPORT_SWIZZLE_POSITIVE_Z;
   vp.swizzle_w = PIPE_VIEWPORT_SWIZZLE_POSITIVE_W;
      }
         static void
   cso_save_viewport(struct cso_context *ctx)
   {
         }
         static void
   cso_restore_viewport(struct cso_context *ctx)
   {
      if (memcmp(&ctx->vp, &ctx->vp_saved, sizeof(ctx->vp))) {
      ctx->vp = ctx->vp_saved;
         }
         void
   cso_set_sample_mask(struct cso_context *ctx, unsigned sample_mask)
   {
      if (ctx->sample_mask != sample_mask) {
      ctx->sample_mask = sample_mask;
         }
         static void
   cso_save_sample_mask(struct cso_context *ctx)
   {
         }
         static void
   cso_restore_sample_mask(struct cso_context *ctx)
   {
         }
         void
   cso_set_min_samples(struct cso_context *ctx, unsigned min_samples)
   {
      if (ctx->min_samples != min_samples && ctx->base.pipe->set_min_samples) {
      ctx->min_samples = min_samples;
         }
         static void
   cso_save_min_samples(struct cso_context *ctx)
   {
         }
         static void
   cso_restore_min_samples(struct cso_context *ctx)
   {
         }
         void
   cso_set_stencil_ref(struct cso_context *ctx,
         {
      if (memcmp(&ctx->stencil_ref, &sr, sizeof(ctx->stencil_ref))) {
      ctx->stencil_ref = sr;
         }
         static void
   cso_save_stencil_ref(struct cso_context *ctx)
   {
         }
         static void
   cso_restore_stencil_ref(struct cso_context *ctx)
   {
      if (memcmp(&ctx->stencil_ref, &ctx->stencil_ref_saved,
            ctx->stencil_ref = ctx->stencil_ref_saved;
         }
         void
   cso_set_render_condition(struct cso_context *ctx,
                     {
               if (ctx->render_condition != query ||
      ctx->render_condition_mode != mode ||
   ctx->render_condition_cond != condition) {
   pipe->render_condition(pipe, query, condition, mode);
   ctx->render_condition = query;
   ctx->render_condition_cond = condition;
         }
         static void
   cso_save_render_condition(struct cso_context *ctx)
   {
      ctx->render_condition_saved = ctx->render_condition;
   ctx->render_condition_cond_saved = ctx->render_condition_cond;
      }
         static void
   cso_restore_render_condition(struct cso_context *ctx)
   {
      cso_set_render_condition(ctx, ctx->render_condition_saved,
            }
         void
   cso_set_geometry_shader_handle(struct cso_context *ctx, void *handle)
   {
               if (ctx->has_geometry_shader && ctx->geometry_shader != handle) {
      ctx->geometry_shader = handle;
         }
         static void
   cso_save_geometry_shader(struct cso_context *ctx)
   {
      if (!ctx->has_geometry_shader) {
                  assert(!ctx->geometry_shader_saved);
      }
         static void
   cso_restore_geometry_shader(struct cso_context *ctx)
   {
      if (!ctx->has_geometry_shader) {
                  if (ctx->geometry_shader_saved != ctx->geometry_shader) {
      ctx->base.pipe->bind_gs_state(ctx->base.pipe, ctx->geometry_shader_saved);
      }
      }
         void
   cso_set_tessctrl_shader_handle(struct cso_context *ctx, void *handle)
   {
               if (ctx->has_tessellation && ctx->tessctrl_shader != handle) {
      ctx->tessctrl_shader = handle;
         }
         static void
   cso_save_tessctrl_shader(struct cso_context *ctx)
   {
      if (!ctx->has_tessellation) {
                  assert(!ctx->tessctrl_shader_saved);
      }
         static void
   cso_restore_tessctrl_shader(struct cso_context *ctx)
   {
      if (!ctx->has_tessellation) {
                  if (ctx->tessctrl_shader_saved != ctx->tessctrl_shader) {
      ctx->base.pipe->bind_tcs_state(ctx->base.pipe, ctx->tessctrl_shader_saved);
      }
      }
         void
   cso_set_tesseval_shader_handle(struct cso_context *ctx, void *handle)
   {
               if (ctx->has_tessellation && ctx->tesseval_shader != handle) {
      ctx->tesseval_shader = handle;
         }
         static void
   cso_save_tesseval_shader(struct cso_context *ctx)
   {
      if (!ctx->has_tessellation) {
                  assert(!ctx->tesseval_shader_saved);
      }
         static void
   cso_restore_tesseval_shader(struct cso_context *ctx)
   {
      if (!ctx->has_tessellation) {
                  if (ctx->tesseval_shader_saved != ctx->tesseval_shader) {
      ctx->base.pipe->bind_tes_state(ctx->base.pipe, ctx->tesseval_shader_saved);
      }
      }
         void
   cso_set_compute_shader_handle(struct cso_context *ctx, void *handle)
   {
               if (ctx->has_compute_shader && ctx->compute_shader != handle) {
      ctx->compute_shader = handle;
         }
         static void
   cso_save_compute_shader(struct cso_context *ctx)
   {
      if (!ctx->has_compute_shader) {
                  assert(!ctx->compute_shader_saved);
      }
         static void
   cso_restore_compute_shader(struct cso_context *ctx)
   {
      if (!ctx->has_compute_shader) {
                  if (ctx->compute_shader_saved != ctx->compute_shader) {
      ctx->base.pipe->bind_compute_state(ctx->base.pipe, ctx->compute_shader_saved);
      }
      }
         static void
   cso_save_compute_samplers(struct cso_context *ctx)
   {
      struct sampler_info *info = &ctx->samplers[PIPE_SHADER_COMPUTE];
            memcpy(saved->cso_samplers, info->cso_samplers,
            }
         static void
   cso_restore_compute_samplers(struct cso_context *ctx)
   {
      struct sampler_info *info = &ctx->samplers[PIPE_SHADER_COMPUTE];
            memcpy(info->cso_samplers, saved->cso_samplers,
                  for (int i = PIPE_MAX_SAMPLERS - 1; i >= 0; i--) {
      if (info->samplers[i]) {
      ctx->max_sampler_seen = i;
                     }
         static void
   cso_set_vertex_elements_direct(struct cso_context *ctx,
         {
      /* Need to include the count into the stored state data too.
   * Otherwise first few count pipe_vertex_elements could be identical
   * even if count is different, and there's no guarantee the hash would
   * be different in that case neither.
   */
   const unsigned key_size =
         const unsigned hash_key = cso_construct_key((void*)velems, key_size);
   struct cso_hash_iter iter =
      cso_find_state_template(&ctx->cache, hash_key, CSO_VELEMENTS,
               if (cso_hash_iter_is_null(iter)) {
      struct cso_velements *cso = MALLOC(sizeof(struct cso_velements));
   if (!cso)
                     /* Lower 64-bit vertex attributes. */
   unsigned new_count = velems->count;
   const struct pipe_vertex_element *new_elems = velems->velems;
   struct pipe_vertex_element tmp[PIPE_MAX_ATTRIBS];
            cso->data = ctx->base.pipe->create_vertex_elements_state(ctx->base.pipe, new_count,
            iter = cso_insert_state(&ctx->cache, hash_key, CSO_VELEMENTS, cso);
   if (cso_hash_iter_is_null(iter)) {
      FREE(cso);
                  } else {
                  if (ctx->velements != handle) {
      ctx->velements = handle;
         }
         enum pipe_error
   cso_set_vertex_elements(struct cso_context *ctx,
         {
               if (vbuf) {
      u_vbuf_set_vertex_elements(vbuf, velems);
               cso_set_vertex_elements_direct(ctx, velems);
      }
         static void
   cso_save_vertex_elements(struct cso_context *ctx)
   {
               if (vbuf) {
      u_vbuf_save_vertex_elements(vbuf);
               assert(!ctx->velements_saved);
      }
         static void
   cso_restore_vertex_elements(struct cso_context *ctx)
   {
               if (vbuf) {
      u_vbuf_restore_vertex_elements(vbuf);
               if (ctx->velements != ctx->velements_saved) {
      ctx->velements = ctx->velements_saved;
      }
      }
      /* vertex buffers */
      void
   cso_set_vertex_buffers(struct cso_context *ctx,
                           {
               if (!count && !unbind_trailing_count)
            if (vbuf) {
      u_vbuf_set_vertex_buffers(vbuf, count, unbind_trailing_count,
                     struct pipe_context *pipe = ctx->base.pipe;
   pipe->set_vertex_buffers(pipe, count, unbind_trailing_count,
      }
         /**
   * Set vertex buffers and vertex elements. Skip u_vbuf if it's only needed
   * for user vertex buffers and user vertex buffers are not set by this call.
   * u_vbuf will be disabled. To re-enable u_vbuf, call this function again.
   *
   * Skipping u_vbuf decreases CPU overhead for draw calls that don't need it,
   * such as VBOs, glBegin/End, and display lists.
   *
   * Internal operations that do "save states, draw, restore states" shouldn't
   * use this, because the states are only saved in either cso_context or
   * u_vbuf, not both.
   */
   void
   cso_set_vertex_buffers_and_elements(struct cso_context *ctx,
                                       {
      struct u_vbuf *vbuf = ctx->vbuf;
            if (vbuf && (ctx->always_use_vbuf || uses_user_vertex_buffers)) {
      if (!ctx->vbuf_current) {
      /* Unbind all buffers in cso_context, because we'll use u_vbuf. */
   unsigned unbind_vb_count = vb_count + unbind_trailing_vb_count;
                  /* Unset this to make sure the CSO is re-bound on the next use. */
   ctx->velements = NULL;
   ctx->vbuf_current = pipe->vbuf = vbuf;
   if (pipe->draw_vbo == tc_draw_vbo)
                     if (vb_count || unbind_trailing_vb_count) {
      u_vbuf_set_vertex_buffers(vbuf, vb_count,
            }
   u_vbuf_set_vertex_elements(vbuf, velems);
               if (ctx->vbuf_current) {
      /* Unbind all buffers in u_vbuf, because we'll use cso_context. */
   unsigned unbind_vb_count = vb_count + unbind_trailing_vb_count;
   if (unbind_vb_count)
            /* Unset this to make sure the CSO is re-bound on the next use. */
   u_vbuf_unset_vertex_elements(vbuf);
   ctx->vbuf_current = pipe->vbuf = NULL;
   if (pipe->draw_vbo == tc_draw_vbo)
                     if (vb_count || unbind_trailing_vb_count) {
      pipe->set_vertex_buffers(pipe, vb_count, unbind_trailing_vb_count,
      }
      }
         ALWAYS_INLINE static struct cso_sampler *
   set_sampler(struct cso_context *ctx, enum pipe_shader_type shader_stage,
               {
      unsigned hash_key = cso_construct_key(templ, key_size);
   struct cso_sampler *cso;
   struct cso_hash_iter iter =
      cso_find_state_template(&ctx->cache,
               if (cso_hash_iter_is_null(iter)) {
      cso = MALLOC(sizeof(struct cso_sampler));
   if (!cso)
            memcpy(&cso->state, templ, sizeof(*templ));
   cso->data = ctx->base.pipe->create_sampler_state(ctx->base.pipe, &cso->state);
            iter = cso_insert_state(&ctx->cache, hash_key, CSO_SAMPLER, cso);
   if (cso_hash_iter_is_null(iter)) {
      FREE(cso);
         } else {
         }
      }
         ALWAYS_INLINE static bool
   cso_set_sampler(struct cso_context *ctx, enum pipe_shader_type shader_stage,
               {
      struct cso_sampler *cso = set_sampler(ctx, shader_stage, idx, templ, size);
   ctx->samplers[shader_stage].cso_samplers[idx] = cso;
   ctx->samplers[shader_stage].samplers[idx] = cso->data;
      }
         void
   cso_single_sampler(struct cso_context *ctx, enum pipe_shader_type shader_stage,
         {
      /* The reasons both blocks are duplicated is that we want the size parameter
   * to be a constant expression to inline and unroll memcmp and hash key
   * computations.
   */
   if (ctx->sampler_format) {
      if (cso_set_sampler(ctx, shader_stage, idx, templ,
            } else {
      if (cso_set_sampler(ctx, shader_stage, idx, templ,
               }
         /**
   * Send staged sampler state to the driver.
   */
   void
   cso_single_sampler_done(struct cso_context *ctx,
         {
               if (ctx->max_sampler_seen == -1)
            ctx->base.pipe->bind_sampler_states(ctx->base.pipe, shader_stage, 0,
                  }
         ALWAYS_INLINE static int
   set_samplers(struct cso_context *ctx,
               enum pipe_shader_type shader_stage,
   unsigned nr,
   {
      int last = -1;
   for (unsigned i = 0; i < nr; i++) {
      if (!templates[i])
            /* Reuse the same sampler state CSO if 2 consecutive sampler states
   * are identical.
   *
   * The trivial case where both pointers are equal doesn't occur in
   * frequented codepaths.
   *
   * Reuse rate:
   * - Borderlands 2: 55%
   * - Hitman: 65%
   * - Rocket League: 75%
   * - Tomb Raider: 50-65%
   * - XCOM 2: 55%
   */
   if (last >= 0 &&
      !memcmp(templates[i], templates[last],
         ctx->samplers[shader_stage].cso_samplers[i] =
         ctx->samplers[shader_stage].samplers[i] =
      } else {
      /* Look up the sampler state CSO. */
                  }
      }
         /*
   * If the function encouters any errors it will return the
   * last one. Done to always try to set as many samplers
   * as possible.
   */
   void
   cso_set_samplers(struct cso_context *ctx,
                     {
               /* ensure sampler size is a constant for memcmp */
   if (ctx->sampler_format) {
      last = set_samplers(ctx, shader_stage, nr, templates,
      } else {
      last = set_samplers(ctx, shader_stage, nr, templates,
               ctx->max_sampler_seen = MAX2(ctx->max_sampler_seen, last);
      }
         static void
   cso_save_fragment_samplers(struct cso_context *ctx)
   {
      struct sampler_info *info = &ctx->samplers[PIPE_SHADER_FRAGMENT];
            memcpy(saved->cso_samplers, info->cso_samplers,
            }
         static void
   cso_restore_fragment_samplers(struct cso_context *ctx)
   {
      struct sampler_info *info = &ctx->samplers[PIPE_SHADER_FRAGMENT];
            memcpy(info->cso_samplers, saved->cso_samplers,
                  for (int i = PIPE_MAX_SAMPLERS - 1; i >= 0; i--) {
      if (info->samplers[i]) {
      ctx->max_sampler_seen = i;
                     }
         void
   cso_set_stream_outputs(struct cso_context *ctx,
                     {
      struct pipe_context *pipe = ctx->base.pipe;
            if (!ctx->has_streamout) {
      assert(num_targets == 0);
               if (ctx->nr_so_targets == 0 && num_targets == 0) {
      /* Nothing to do. */
               /* reference new targets */
   for (i = 0; i < num_targets; i++) {
         }
   /* unref extra old targets, if any */
   for (; i < ctx->nr_so_targets; i++) {
                  pipe->set_stream_output_targets(pipe, num_targets, targets,
            }
         static void
   cso_save_stream_outputs(struct cso_context *ctx)
   {
      if (!ctx->has_streamout) {
                           for (unsigned i = 0; i < ctx->nr_so_targets; i++) {
      assert(!ctx->so_targets_saved[i]);
         }
         static void
   cso_restore_stream_outputs(struct cso_context *ctx)
   {
      struct pipe_context *pipe = ctx->base.pipe;
   unsigned i;
            if (!ctx->has_streamout) {
                  if (ctx->nr_so_targets == 0 && ctx->nr_so_targets_saved == 0) {
      /* Nothing to do. */
               assert(ctx->nr_so_targets_saved <= PIPE_MAX_SO_BUFFERS);
   for (i = 0; i < ctx->nr_so_targets_saved; i++) {
      pipe_so_target_reference(&ctx->so_targets[i], NULL);
   /* move the reference from one pointer to another */
   ctx->so_targets[i] = ctx->so_targets_saved[i];
   ctx->so_targets_saved[i] = NULL;
   /* -1 means append */
      }
   for (; i < ctx->nr_so_targets; i++) {
                  pipe->set_stream_output_targets(pipe, ctx->nr_so_targets_saved,
            ctx->nr_so_targets = ctx->nr_so_targets_saved;
      }
         /**
   * Save all the CSO state items specified by the state_mask bitmask
   * of CSO_BIT_x flags.
   */
   void
   cso_save_state(struct cso_context *cso, unsigned state_mask)
   {
                        if (state_mask & CSO_BIT_BLEND)
         if (state_mask & CSO_BIT_DEPTH_STENCIL_ALPHA)
         if (state_mask & CSO_BIT_FRAGMENT_SAMPLERS)
         if (state_mask & CSO_BIT_FRAGMENT_SHADER)
         if (state_mask & CSO_BIT_FRAMEBUFFER)
         if (state_mask & CSO_BIT_GEOMETRY_SHADER)
         if (state_mask & CSO_BIT_MIN_SAMPLES)
         if (state_mask & CSO_BIT_RASTERIZER)
         if (state_mask & CSO_BIT_RENDER_CONDITION)
         if (state_mask & CSO_BIT_SAMPLE_MASK)
         if (state_mask & CSO_BIT_STENCIL_REF)
         if (state_mask & CSO_BIT_STREAM_OUTPUTS)
         if (state_mask & CSO_BIT_TESSCTRL_SHADER)
         if (state_mask & CSO_BIT_TESSEVAL_SHADER)
         if (state_mask & CSO_BIT_VERTEX_ELEMENTS)
         if (state_mask & CSO_BIT_VERTEX_SHADER)
         if (state_mask & CSO_BIT_VIEWPORT)
         if (state_mask & CSO_BIT_PAUSE_QUERIES)
      }
         /**
   * Restore the state which was saved by cso_save_state().
   */
   void
   cso_restore_state(struct cso_context *cso, unsigned unbind)
   {
                        if (state_mask & CSO_BIT_DEPTH_STENCIL_ALPHA)
         if (state_mask & CSO_BIT_STENCIL_REF)
         if (state_mask & CSO_BIT_FRAGMENT_SHADER)
         if (state_mask & CSO_BIT_GEOMETRY_SHADER)
         if (state_mask & CSO_BIT_TESSEVAL_SHADER)
         if (state_mask & CSO_BIT_TESSCTRL_SHADER)
         if (state_mask & CSO_BIT_VERTEX_SHADER)
         if (unbind & CSO_UNBIND_FS_SAMPLERVIEWS)
      cso->base.pipe->set_sampler_views(cso->base.pipe, PIPE_SHADER_FRAGMENT, 0, 0,
      if (unbind & CSO_UNBIND_FS_SAMPLERVIEW0)
      cso->base.pipe->set_sampler_views(cso->base.pipe, PIPE_SHADER_FRAGMENT, 0, 0,
      if (state_mask & CSO_BIT_FRAGMENT_SAMPLERS)
         if (unbind & CSO_UNBIND_FS_IMAGE0)
         if (state_mask & CSO_BIT_FRAMEBUFFER)
         if (state_mask & CSO_BIT_BLEND)
         if (state_mask & CSO_BIT_RASTERIZER)
         if (state_mask & CSO_BIT_MIN_SAMPLES)
         if (state_mask & CSO_BIT_RENDER_CONDITION)
         if (state_mask & CSO_BIT_SAMPLE_MASK)
         if (state_mask & CSO_BIT_VIEWPORT)
         if (unbind & CSO_UNBIND_VS_CONSTANTS)
         if (unbind & CSO_UNBIND_FS_CONSTANTS)
         if (state_mask & CSO_BIT_VERTEX_ELEMENTS)
         if (unbind & CSO_UNBIND_VERTEX_BUFFER0)
         if (state_mask & CSO_BIT_STREAM_OUTPUTS)
         if (state_mask & CSO_BIT_PAUSE_QUERIES)
               }
         /**
   * Save all the CSO state items specified by the state_mask bitmask
   * of CSO_BIT_COMPUTE_x flags.
   */
   void
   cso_save_compute_state(struct cso_context *cso, unsigned state_mask)
   {
                        if (state_mask & CSO_BIT_COMPUTE_SHADER)
            if (state_mask & CSO_BIT_COMPUTE_SAMPLERS)
      }
         /**
   * Restore the state which was saved by cso_save_compute_state().
   */
   void
   cso_restore_compute_state(struct cso_context *cso)
   {
                        if (state_mask & CSO_BIT_COMPUTE_SHADER)
            if (state_mask & CSO_BIT_COMPUTE_SAMPLERS)
               }
            /* drawing */
      void
   cso_draw_arrays(struct cso_context *cso, unsigned mode, unsigned start, unsigned count)
   {
      struct pipe_draw_info info;
                     info.mode = mode;
   info.index_bounds_valid = true;
   info.min_index = start;
            draw.start = start;
   draw.count = count;
               }
         void
   cso_draw_arrays_instanced(struct cso_context *cso, unsigned mode,
               {
      struct pipe_draw_info info;
                     info.mode = mode;
   info.index_bounds_valid = true;
   info.min_index = start;
   info.max_index = start + count - 1;
   info.start_instance = start_instance;
            draw.start = start;
   draw.count = count;
               }
