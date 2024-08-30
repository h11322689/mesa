   /**************************************************************************
   *
   * Copyright 2015 Advanced Micro Devices, Inc.
   * Copyright 2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include "dd_pipe.h"
   #include "tgsi/tgsi_parse.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
         static void
   safe_memcpy(void *dst, const void *src, size_t size)
   {
      if (src)
         else
      }
         /********************************************************************
   * queries
   */
      static struct pipe_query *
   dd_context_create_query(struct pipe_context *_pipe, unsigned query_type,
         {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
                     /* Wrap query object. */
   if (query) {
      struct dd_query *dd_query = CALLOC_STRUCT(dd_query);
   if (dd_query) {
      dd_query->type = query_type;
   dd_query->query = query;
      } else {
      pipe->destroy_query(pipe, query);
                     }
      static struct pipe_query *
   dd_context_create_batch_query(struct pipe_context *_pipe, unsigned num_queries,
         {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
                     /* Wrap query object. */
   if (query) {
      struct dd_query *dd_query = CALLOC_STRUCT(dd_query);
   if (dd_query) {
      /* no special handling for batch queries yet */
   dd_query->type = query_types[0];
   dd_query->query = query;
      } else {
      pipe->destroy_query(pipe, query);
                     }
      static void
   dd_context_destroy_query(struct pipe_context *_pipe,
         {
               pipe->destroy_query(pipe, dd_query_unwrap(query));
      }
      static bool
   dd_context_begin_query(struct pipe_context *_pipe, struct pipe_query *query)
   {
      struct dd_context *dctx = dd_context(_pipe);
               }
      static bool
   dd_context_end_query(struct pipe_context *_pipe, struct pipe_query *query)
   {
      struct dd_context *dctx = dd_context(_pipe);
               }
      static bool
   dd_context_get_query_result(struct pipe_context *_pipe,
               {
                  }
      static void
   dd_context_set_active_query_state(struct pipe_context *_pipe, bool enable)
   {
                  }
      static void
   dd_context_render_condition(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            pipe->render_condition(pipe, dd_query_unwrap(query), condition, mode);
   dstate->render_cond.query = dd_query(query);
   dstate->render_cond.condition = condition;
      }
         /********************************************************************
   * constant (immutable) non-shader states
   */
      #define DD_CSO_CREATE(name, shortname) \
      static void * \
   dd_context_create_##name##_state(struct pipe_context *_pipe, \
         { \
      struct pipe_context *pipe = dd_context(_pipe)->pipe; \
   \
         if (!hstate) \
         hstate->cso = pipe->create_##name##_state(pipe, state); \
   hstate->state.shortname = *state; \
            #define DD_CSO_BIND(name, shortname) \
      static void \
   dd_context_bind_##name##_state(struct pipe_context *_pipe, void *state) \
   { \
      struct dd_context *dctx = dd_context(_pipe); \
   struct pipe_context *pipe = dctx->pipe; \
   \
         dctx->draw_state.shortname = hstate; \
            #define DD_CSO_DELETE(name) \
      static void \
   dd_context_delete_##name##_state(struct pipe_context *_pipe, void *state) \
   { \
      struct dd_context *dctx = dd_context(_pipe); \
   struct pipe_context *pipe = dctx->pipe; \
   \
         pipe->delete_##name##_state(pipe, hstate->cso); \
            #define DD_CSO_WHOLE(name, shortname) \
      DD_CSO_CREATE(name, shortname) \
   DD_CSO_BIND(name, shortname) \
         DD_CSO_WHOLE(blend, blend)
   DD_CSO_WHOLE(rasterizer, rs)
   DD_CSO_WHOLE(depth_stencil_alpha, dsa)
      DD_CSO_CREATE(sampler, sampler)
   DD_CSO_DELETE(sampler)
      static void
   dd_context_bind_sampler_states(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
            memcpy(&dctx->draw_state.sampler_states[shader][start], states,
            if (states) {
      void *samp[PIPE_MAX_SAMPLERS];
            for (i = 0; i < count; i++) {
      struct dd_state *s = states[i];
                  }
   else
      }
      static void *
   dd_context_create_vertex_elements_state(struct pipe_context *_pipe,
               {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
            if (!hstate)
         hstate->cso = pipe->create_vertex_elements_state(pipe, num_elems, elems);
   memcpy(hstate->state.velems.velems, elems, sizeof(elems[0]) * num_elems);
   hstate->state.velems.count = num_elems;
      }
      DD_CSO_BIND(vertex_elements, velems)
   DD_CSO_DELETE(vertex_elements)
         /********************************************************************
   * shaders
   */
      #define DD_SHADER_NOCREATE(NAME, name) \
      static void \
   dd_context_bind_##name##_state(struct pipe_context *_pipe, void *state) \
   { \
      struct dd_context *dctx = dd_context(_pipe); \
   struct pipe_context *pipe = dctx->pipe; \
      \
      dctx->draw_state.shaders[PIPE_SHADER_##NAME] = hstate; \
      } \
   \
   static void \
   dd_context_delete_##name##_state(struct pipe_context *_pipe, void *state) \
   { \
      struct dd_context *dctx = dd_context(_pipe); \
   struct pipe_context *pipe = dctx->pipe; \
      \
      pipe->delete_##name##_state(pipe, hstate->cso); \
   if (hstate->state.shader.type == PIPE_SHADER_IR_TGSI) \
                  #define DD_SHADER(NAME, name) \
      static void * \
   dd_context_create_##name##_state(struct pipe_context *_pipe, \
         { \
      struct pipe_context *pipe = dd_context(_pipe)->pipe; \
   \
         if (!hstate) \
         hstate->cso = pipe->create_##name##_state(pipe, state); \
   hstate->state.shader = *state; \
   if (hstate->state.shader.type == PIPE_SHADER_IR_TGSI) \
            } \
   \
         DD_SHADER(FRAGMENT, fs)
   DD_SHADER(VERTEX, vs)
   DD_SHADER(GEOMETRY, gs)
   DD_SHADER(TESS_CTRL, tcs)
   DD_SHADER(TESS_EVAL, tes)
      static void * \
   dd_context_create_compute_state(struct pipe_context *_pipe,
         {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
            if (!hstate)
                           if (state->ir_type == PIPE_SHADER_IR_TGSI)
               }
      DD_SHADER_NOCREATE(COMPUTE, compute)
      /********************************************************************
   * immediate states
   */
      #define DD_IMM_STATE(name, type, deref, ref) \
      static void \
   dd_context_set_##name(struct pipe_context *_pipe, type deref) \
   { \
      struct dd_context *dctx = dd_context(_pipe); \
   \
         dctx->draw_state.name = deref; \
            DD_IMM_STATE(blend_color, const struct pipe_blend_color, *state, state)
   DD_IMM_STATE(stencil_ref, const struct pipe_stencil_ref, state, state)
   DD_IMM_STATE(clip_state, const struct pipe_clip_state, *state, state)
   DD_IMM_STATE(sample_mask, unsigned, sample_mask, sample_mask)
   DD_IMM_STATE(min_samples, unsigned, min_samples, min_samples)
   DD_IMM_STATE(framebuffer_state, const struct pipe_framebuffer_state, *state, state)
   DD_IMM_STATE(polygon_stipple, const struct pipe_poly_stipple, *state, state)
      static void
   dd_context_set_constant_buffer(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.constant_buffers[shader][index],
            }
      static void
   dd_context_set_scissor_states(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.scissors[start_slot], states,
            }
      static void
   dd_context_set_viewport_states(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.viewports[start_slot], states,
            }
      static void dd_context_set_tess_state(struct pipe_context *_pipe,
               {
      struct dd_context *dctx = dd_context(_pipe);
            memcpy(dctx->draw_state.tess_default_levels, default_outer_level,
         memcpy(dctx->draw_state.tess_default_levels+4, default_inner_level,
            }
      static void dd_context_set_patch_vertices(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
               }
      static void dd_context_set_window_rectangles(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
               }
         /********************************************************************
   * views
   */
      static struct pipe_surface *
   dd_context_create_surface(struct pipe_context *_pipe,
               {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
   struct pipe_surface *view =
            if (!view)
         view->context = _pipe;
      }
      static void
   dd_context_surface_destroy(struct pipe_context *_pipe,
         {
                  }
      static struct pipe_sampler_view *
   dd_context_create_sampler_view(struct pipe_context *_pipe,
               {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
   struct pipe_sampler_view *view =
            if (!view)
         view->context = _pipe;
      }
      static void
   dd_context_sampler_view_destroy(struct pipe_context *_pipe,
         {
                  }
      static struct pipe_stream_output_target *
   dd_context_create_stream_output_target(struct pipe_context *_pipe,
                     {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
   struct pipe_stream_output_target *view =
      pipe->create_stream_output_target(pipe, res, buffer_offset,
         if (!view)
         view->context = _pipe;
      }
      static void
   dd_context_stream_output_target_destroy(struct pipe_context *_pipe,
         {
                  }
         /********************************************************************
   * set states
   */
      static void
   dd_context_set_sampler_views(struct pipe_context *_pipe,
                                 {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.sampler_views[shader][start], views,
         safe_memcpy(&dctx->draw_state.sampler_views[shader][start + num], views,
         pipe->set_sampler_views(pipe, shader, start, num, take_ownership,
      }
      static void
   dd_context_set_shader_images(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.shader_images[shader][start], views,
         safe_memcpy(&dctx->draw_state.shader_images[shader][start + num], NULL,
         pipe->set_shader_images(pipe, shader, start, num,
      }
      static void
   dd_context_set_shader_buffers(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.shader_buffers[shader][start], buffers,
         pipe->set_shader_buffers(pipe, shader, start, num_buffers, buffers,
      }
      static void
   dd_context_set_vertex_buffers(struct pipe_context *_pipe,
                           {
      struct dd_context *dctx = dd_context(_pipe);
            safe_memcpy(&dctx->draw_state.vertex_buffers[0], buffers,
         safe_memcpy(&dctx->draw_state.vertex_buffers[num_buffers], NULL,
         pipe->set_vertex_buffers(pipe, num_buffers,
            }
      static void
   dd_context_set_stream_output_targets(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
   struct pipe_context *pipe = dctx->pipe;
            dstate->num_so_targets = num_targets;
   safe_memcpy(dstate->so_targets, tgs, sizeof(*tgs) * num_targets);
   safe_memcpy(dstate->so_offsets, offsets, sizeof(*offsets) * num_targets);
      }
         static void
   dd_context_fence_server_sync(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
               }
         static void
   dd_context_create_fence_fd(struct pipe_context *_pipe,
                     {
      struct dd_context *dctx = dd_context(_pipe);
               }
         void
   dd_thread_join(struct dd_context *dctx)
   {
      mtx_lock(&dctx->mutex);
   dctx->kill_thread = true;
   cnd_signal(&dctx->cond);
   mtx_unlock(&dctx->mutex);
      }
      static void
   dd_context_destroy(struct pipe_context *_pipe)
   {
      struct dd_context *dctx = dd_context(_pipe);
            dd_thread_join(dctx);
   mtx_destroy(&dctx->mutex);
                     if (pipe->set_log_context) {
               if (dd_screen(dctx->base.screen)->dump_mode == DD_DUMP_ALL_CALLS) {
      FILE *f = dd_get_file_stream(dd_screen(dctx->base.screen), 0);
   if (f) {
                  u_log_new_page_print(&dctx->log, f);
         }
            pipe->destroy(pipe);
      }
         /********************************************************************
   * miscellaneous
   */
      static void
   dd_context_texture_barrier(struct pipe_context *_pipe, unsigned flags)
   {
                  }
      static void
   dd_context_memory_barrier(struct pipe_context *_pipe, unsigned flags)
   {
                  }
      static bool
   dd_context_resource_commit(struct pipe_context *_pipe,
               {
                  }
      static void
   dd_context_set_compute_resources(struct pipe_context *_pipe,
      unsigned start, unsigned count,
      {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
      }
      static void
   dd_context_set_global_binding(struct pipe_context *_pipe,
            unsigned first, unsigned count,
      {
      struct pipe_context *pipe = dd_context(_pipe)->pipe;
      }
      static void
   dd_context_get_sample_position(struct pipe_context *_pipe,
               {
               pipe->get_sample_position(pipe, sample_count, sample_index,
      }
      static void
   dd_context_invalidate_resource(struct pipe_context *_pipe,
         {
                  }
      static enum pipe_reset_status
   dd_context_get_device_reset_status(struct pipe_context *_pipe)
   {
                  }
      static void
   dd_context_set_device_reset_callback(struct pipe_context *_pipe,
         {
                  }
      static void
   dd_context_emit_string_marker(struct pipe_context *_pipe,
         {
      struct dd_context *dctx = dd_context(_pipe);
            pipe->emit_string_marker(pipe, string, len);
      }
      static void
   dd_context_dump_debug_state(struct pipe_context *_pipe, FILE *stream,
         {
                  }
      static uint64_t
   dd_context_create_texture_handle(struct pipe_context *_pipe,
               {
                  }
      static void
   dd_context_delete_texture_handle(struct pipe_context *_pipe, uint64_t handle)
   {
                  }
      static void
   dd_context_make_texture_handle_resident(struct pipe_context *_pipe,
         {
                  }
      static uint64_t
   dd_context_create_image_handle(struct pipe_context *_pipe,
         {
                  }
      static void
   dd_context_delete_image_handle(struct pipe_context *_pipe, uint64_t handle)
   {
                  }
      static void
   dd_context_make_image_handle_resident(struct pipe_context *_pipe,
               {
                  }
      static void
   dd_context_set_context_param(struct pipe_context *_pipe,
               {
                  }
      struct pipe_context *
   dd_context_create(struct dd_screen *dscreen, struct pipe_context *pipe)
   {
               if (!pipe)
            dctx = CALLOC_STRUCT(dd_context);
   if (!dctx)
            dctx->pipe = pipe;
   dctx->base.priv = pipe->priv; /* expose wrapped priv data */
   dctx->base.screen = &dscreen->base;
   dctx->base.stream_uploader = pipe->stream_uploader;
                     CTX_INIT(render_condition);
   CTX_INIT(create_query);
   CTX_INIT(create_batch_query);
   CTX_INIT(destroy_query);
   CTX_INIT(begin_query);
   CTX_INIT(end_query);
   CTX_INIT(get_query_result);
   CTX_INIT(set_active_query_state);
   CTX_INIT(create_blend_state);
   CTX_INIT(bind_blend_state);
   CTX_INIT(delete_blend_state);
   CTX_INIT(create_sampler_state);
   CTX_INIT(bind_sampler_states);
   CTX_INIT(delete_sampler_state);
   CTX_INIT(create_rasterizer_state);
   CTX_INIT(bind_rasterizer_state);
   CTX_INIT(delete_rasterizer_state);
   CTX_INIT(create_depth_stencil_alpha_state);
   CTX_INIT(bind_depth_stencil_alpha_state);
   CTX_INIT(delete_depth_stencil_alpha_state);
   CTX_INIT(create_fs_state);
   CTX_INIT(bind_fs_state);
   CTX_INIT(delete_fs_state);
   CTX_INIT(create_vs_state);
   CTX_INIT(bind_vs_state);
   CTX_INIT(delete_vs_state);
   CTX_INIT(create_gs_state);
   CTX_INIT(bind_gs_state);
   CTX_INIT(delete_gs_state);
   CTX_INIT(create_tcs_state);
   CTX_INIT(bind_tcs_state);
   CTX_INIT(delete_tcs_state);
   CTX_INIT(create_tes_state);
   CTX_INIT(bind_tes_state);
   CTX_INIT(delete_tes_state);
   CTX_INIT(create_compute_state);
   CTX_INIT(bind_compute_state);
   CTX_INIT(delete_compute_state);
   CTX_INIT(create_vertex_elements_state);
   CTX_INIT(bind_vertex_elements_state);
   CTX_INIT(delete_vertex_elements_state);
   CTX_INIT(set_blend_color);
   CTX_INIT(set_stencil_ref);
   CTX_INIT(set_sample_mask);
   CTX_INIT(set_min_samples);
   CTX_INIT(set_clip_state);
   CTX_INIT(set_constant_buffer);
   CTX_INIT(set_framebuffer_state);
   CTX_INIT(set_polygon_stipple);
   CTX_INIT(set_scissor_states);
   CTX_INIT(set_viewport_states);
   CTX_INIT(set_sampler_views);
   CTX_INIT(set_tess_state);
   CTX_INIT(set_patch_vertices);
   CTX_INIT(set_shader_buffers);
   CTX_INIT(set_shader_images);
   CTX_INIT(set_vertex_buffers);
   CTX_INIT(set_window_rectangles);
   CTX_INIT(create_stream_output_target);
   CTX_INIT(stream_output_target_destroy);
   CTX_INIT(set_stream_output_targets);
   CTX_INIT(create_fence_fd);
   CTX_INIT(fence_server_sync);
   CTX_INIT(create_sampler_view);
   CTX_INIT(sampler_view_destroy);
   CTX_INIT(create_surface);
   CTX_INIT(surface_destroy);
   CTX_INIT(texture_barrier);
   CTX_INIT(memory_barrier);
   CTX_INIT(resource_commit);
   CTX_INIT(set_compute_resources);
   CTX_INIT(set_global_binding);
   /* create_video_codec */
   /* create_video_buffer */
   CTX_INIT(get_sample_position);
   CTX_INIT(invalidate_resource);
   CTX_INIT(get_device_reset_status);
   CTX_INIT(set_device_reset_callback);
   CTX_INIT(dump_debug_state);
   CTX_INIT(emit_string_marker);
   CTX_INIT(create_texture_handle);
   CTX_INIT(delete_texture_handle);
   CTX_INIT(make_texture_handle_resident);
   CTX_INIT(create_image_handle);
   CTX_INIT(delete_image_handle);
   CTX_INIT(make_image_handle_resident);
                     u_log_context_init(&dctx->log);
   if (pipe->set_log_context)
                     list_inithead(&dctx->records);
   (void) mtx_init(&dctx->mutex, mtx_plain);
   (void) cnd_init(&dctx->cond);
   if (thrd_success != u_thread_create(&dctx->thread,dd_thread_main, dctx)) {
      mtx_destroy(&dctx->mutex);
                     fail:
      FREE(dctx);
   pipe->destroy(pipe);
      }
