   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
      #include "util/ralloc.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_framebuffer.h"
      #include "util/format/u_formats.h"
   #include "pipe/p_screen.h"
      #include "tr_dump.h"
   #include "tr_dump_defines.h"
   #include "tr_dump_state.h"
   #include "tr_public.h"
   #include "tr_screen.h"
   #include "tr_texture.h"
   #include "tr_context.h"
   #include "tr_util.h"
   #include "tr_video.h"
         struct trace_query
   {
      struct threaded_query base;
   unsigned type;
               };
         static inline struct trace_query *
   trace_query(struct pipe_query *query)
   {
         }
         static inline struct pipe_query *
   trace_query_unwrap(struct pipe_query *query)
   {
      if (query) {
         } else {
            }
         static inline struct pipe_surface *
   trace_surface_unwrap(struct trace_context *tr_ctx,
         {
               if (!surface)
            assert(surface->texture);
   if (!surface->texture)
                     assert(tr_surf->surface);
      }
      static void
   dump_fb_state(struct trace_context *tr_ctx,
               {
      struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
   if (deep)
         else
                     }
      static void
   trace_context_draw_vbo(struct pipe_context *_pipe,
                        const struct pipe_draw_info *info,
      {
      struct trace_context *tr_ctx = trace_context(_pipe);
            if (!tr_ctx->seen_fb_state && trace_dump_is_triggered())
                     trace_dump_arg(ptr,  pipe);
   trace_dump_arg(draw_info, info);
   trace_dump_arg(int, drawid_offset);
   trace_dump_arg(draw_indirect_info, indirect);
   trace_dump_arg_begin("draws");
   trace_dump_struct_array(draw_start_count, draws, num_draws);
   trace_dump_arg_end();
                                 }
      static void
   trace_context_draw_mesh_tasks(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr,  pipe);
                                 }
         static void
   trace_context_draw_vertex_state(struct pipe_context *_pipe,
                                 {
      struct trace_context *tr_ctx = trace_context(_pipe);
            if (!tr_ctx->seen_fb_state && trace_dump_is_triggered())
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);
   trace_dump_arg(uint, partial_velem_mask);
   trace_dump_arg(draw_vertex_state_info, info);
   trace_dump_arg_begin("draws");
   trace_dump_struct_array(draw_start_count, draws, num_draws);
   trace_dump_arg_end();
                     pipe->draw_vertex_state(pipe, state, partial_velem_mask, info, draws,
            }
         static struct pipe_query *
   trace_context_create_query(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(query_type, query_type);
                                       /* Wrap query object. */
   if (query) {
      struct trace_query *tr_query = CALLOC_STRUCT(trace_query);
   if (tr_query) {
      tr_query->type = query_type;
   tr_query->query = query;
   tr_query->index = index;
      } else {
      pipe->destroy_query(pipe, query);
                     }
         static void
   trace_context_destroy_query(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct trace_query *tr_query = trace_query(_query);
                              trace_dump_arg(ptr, pipe);
                        }
         static bool
   trace_context_begin_query(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                              trace_dump_arg(ptr, pipe);
                     trace_dump_call_end();
      }
         static bool
   trace_context_end_query(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                              trace_dump_arg(ptr, pipe);
            if (tr_ctx->threaded)
                  trace_dump_call_end();
      }
         static bool
   trace_context_get_query_result(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct trace_query *tr_query = trace_query(_query);
   struct pipe_query *query = tr_query->query;
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, query);
            if (tr_ctx->threaded)
                     trace_dump_arg_begin("result");
   if (ret) {
         } else {
         }
                                 }
      static void
   trace_context_get_query_result_resource(struct pipe_context *_pipe,
                                       {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct trace_query *tr_query = trace_query(_query);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, query);
   trace_dump_arg(query_flags, flags);
   trace_dump_arg(uint, result_type);
   trace_dump_arg(uint, index);
   trace_dump_arg(ptr, resource);
            if (tr_ctx->threaded)
                        }
         static void
   trace_context_set_active_query_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void *
   trace_context_create_blend_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
                                       struct pipe_blend_state *blend = ralloc(tr_ctx, struct pipe_blend_state);
   if (blend) {
      memcpy(blend, state, sizeof(struct pipe_blend_state));
                  }
         static void
   trace_context_bind_blend_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   if (state && trace_dump_is_triggered()) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->blend_states, state);
   if (he)
         else
      } else
                        }
         static void
   trace_context_delete_blend_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                     if (state) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->blend_states, state);
   if (he) {
      ralloc_free(he->data);
                     }
         static void *
   trace_context_create_sampler_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
                                          }
         static void
   trace_context_bind_sampler_states(struct pipe_context *_pipe,
                           {
      struct trace_context *tr_ctx = trace_context(_pipe);
            /* remove this when we have pipe->bind_sampler_states(..., start, ...) */
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg_enum(pipe_shader_type, shader);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, num_states);
                        }
         static void
   trace_context_delete_sampler_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void *
   trace_context_create_rasterizer_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
                                       struct pipe_rasterizer_state *rasterizer = ralloc(tr_ctx, struct pipe_rasterizer_state);
   if (rasterizer) {
      memcpy(rasterizer, state, sizeof(struct pipe_rasterizer_state));
                  }
         static void
   trace_context_bind_rasterizer_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   if (state && trace_dump_is_triggered()) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->rasterizer_states, state);
   if (he)
         else
      } else
                        }
         static void
   trace_context_delete_rasterizer_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                              if (state) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->rasterizer_states, state);
   if (he) {
      ralloc_free(he->data);
            }
         static void *
   trace_context_create_depth_stencil_alpha_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                              trace_dump_arg(ptr, pipe);
                              struct pipe_depth_stencil_alpha_state *depth_stencil_alpha = ralloc(tr_ctx, struct pipe_depth_stencil_alpha_state);
   if (depth_stencil_alpha) {
      memcpy(depth_stencil_alpha, state, sizeof(struct pipe_depth_stencil_alpha_state));
                  }
         static void
   trace_context_bind_depth_stencil_alpha_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   if (state && trace_dump_is_triggered()) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->depth_stencil_alpha_states, state);
   if (he)
         else
      } else
                        }
         static void
   trace_context_delete_depth_stencil_alpha_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                              if (state) {
      struct hash_entry *he = _mesa_hash_table_search(&tr_ctx->depth_stencil_alpha_states, state);
   if (he) {
      ralloc_free(he->data);
            }
         #define TRACE_SHADER_STATE(shader_type) \
      static void * \
   trace_context_create_##shader_type##_state(struct pipe_context *_pipe, \
         { \
      struct trace_context *tr_ctx = trace_context(_pipe); \
   struct pipe_context *pipe = tr_ctx->pipe; \
   void * result; \
   trace_dump_call_begin("pipe_context", "create_" #shader_type "_state"); \
   trace_dump_arg(ptr, pipe); \
   trace_dump_arg(shader_state, state); \
   result = pipe->create_##shader_type##_state(pipe, state); \
   trace_dump_ret(ptr, result); \
   trace_dump_call_end(); \
      } \
   \
   static void \
   trace_context_bind_##shader_type##_state(struct pipe_context *_pipe, \
         { \
      struct trace_context *tr_ctx = trace_context(_pipe); \
   struct pipe_context *pipe = tr_ctx->pipe; \
   trace_dump_call_begin("pipe_context", "bind_" #shader_type "_state"); \
   trace_dump_arg(ptr, pipe); \
   trace_dump_arg(ptr, state); \
   pipe->bind_##shader_type##_state(pipe, state); \
      } \
   \
   static void \
   trace_context_delete_##shader_type##_state(struct pipe_context *_pipe, \
         { \
      struct trace_context *tr_ctx = trace_context(_pipe); \
   struct pipe_context *pipe = tr_ctx->pipe; \
   trace_dump_call_begin("pipe_context", "delete_" #shader_type "_state"); \
   trace_dump_arg(ptr, pipe); \
   trace_dump_arg(ptr, state); \
   pipe->delete_##shader_type##_state(pipe, state); \
            TRACE_SHADER_STATE(fs)
   TRACE_SHADER_STATE(vs)
   TRACE_SHADER_STATE(gs)
   TRACE_SHADER_STATE(tcs)
   TRACE_SHADER_STATE(tes)
   TRACE_SHADER_STATE(ms)
   TRACE_SHADER_STATE(ts)
      #undef TRACE_SHADER_STATE
      static void
   trace_context_link_shader(struct pipe_context *_pipe, void **shaders)
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "link_shader");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg_array(ptr, shaders, PIPE_SHADER_TYPES);
   pipe->link_shader(pipe, shaders);
      }
      static inline void *
   trace_context_create_compute_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
            trace_dump_call_begin("pipe_context", "create_compute_state");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(compute_state, state);
   result = pipe->create_compute_state(pipe, state);
   trace_dump_ret(ptr, result);
   trace_dump_call_end();
      }
      static inline void
   trace_context_bind_compute_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "bind_compute_state");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);
   pipe->bind_compute_state(pipe, state);
      }
      static inline void
   trace_context_delete_compute_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "delete_compute_state");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, state);
   pipe->delete_compute_state(pipe, state);
      }
      static void *
   trace_context_create_vertex_elements_state(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
            trace_dump_arg_begin("elements");
   trace_dump_struct_array(vertex_element, elements, num_elements);
                                          }
         static void
   trace_context_bind_vertex_elements_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_delete_vertex_elements_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_blend_color(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_stencil_ref(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_clip_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
      static void
   trace_context_set_sample_mask(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
      static void
   trace_context_set_constant_buffer(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg_enum(pipe_shader_type, shader);
   trace_dump_arg(uint, index);
   trace_dump_arg(bool, take_ownership);
                        }
         static void
   trace_context_set_framebuffer_state(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
            /* Unwrap the input state */
   memcpy(&tr_ctx->unwrapped_state, state, sizeof(tr_ctx->unwrapped_state));
   for (i = 0; i < state->nr_cbufs; ++i)
         for (i = state->nr_cbufs; i < PIPE_MAX_COLOR_BUFS; ++i)
         tr_ctx->unwrapped_state.zsbuf = trace_surface_unwrap(tr_ctx, state->zsbuf);
                        }
      static void
   trace_context_set_inlinable_constants(struct pipe_context *_pipe, enum pipe_shader_type shader,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg_enum(pipe_shader_type, shader);
   trace_dump_arg(uint, num_values);
                        }
         static void
   trace_context_set_polygon_stipple(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
      static void
   trace_context_set_min_samples(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_scissor_states(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, start_slot);
   trace_dump_arg(uint, num_scissors);
                        }
         static void
   trace_context_set_viewport_states(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, start_slot);
   trace_dump_arg(uint, num_viewports);
                        }
         static struct pipe_sampler_view *
   trace_context_create_sampler_view(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
            trace_dump_arg_begin("templ");
   trace_dump_sampler_view_template(templ);
                                                   }
         static void
   trace_context_sampler_view_destroy(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_sampler_view *tr_view = trace_sampler_view(_view);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
                        }
      /********************************************************************
   * surface
   */
         static struct pipe_surface *
   trace_context_create_surface(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
            trace_dump_arg_begin("surf_tmpl");
   trace_dump_surface_template(surf_tmpl, resource->target);
                                                      }
         static void
   trace_context_surface_destroy(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
   struct trace_surface *tr_surf = trace_surface(_surface);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_sampler_views(struct pipe_context *_pipe,
                                 enum pipe_shader_type shader,
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct trace_sampler_view *tr_view;
   struct pipe_context *pipe = tr_ctx->pipe;
   struct pipe_sampler_view *unwrapped_views[PIPE_MAX_SHADER_SAMPLER_VIEWS];
            /* remove this when we have pipe->set_sampler_views(..., start, ...) */
            for (i = 0; i < num; ++i) {
      tr_view = trace_sampler_view(views[i]);
      }
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg_enum(pipe_shader_type, shader);
   trace_dump_arg(uint, start);
   trace_dump_arg(uint, num);
   trace_dump_arg(uint, unbind_num_trailing_slots);
   trace_dump_arg(bool, take_ownership);
            pipe->set_sampler_views(pipe, shader, start, num,
               }
         static void
   trace_context_set_vertex_buffers(struct pipe_context *_pipe,
                           {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_buffers);
   trace_dump_arg(uint, unbind_num_trailing_slots);
            trace_dump_arg_begin("buffers");
   trace_dump_struct_array(vertex_buffer, buffers, num_buffers);
            pipe->set_vertex_buffers(pipe, num_buffers,
                     }
         static struct pipe_stream_output_target *
   trace_context_create_stream_output_target(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, res);
   trace_dump_arg(uint, buffer_offset);
            result = pipe->create_stream_output_target(pipe,
                                 }
         static void
   trace_context_stream_output_target_destroy(
      struct pipe_context *_pipe,
      {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_set_stream_output_targets(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, num_targets);
   trace_dump_arg_array(ptr, tgs, num_targets);
                        }
         static void
   trace_context_resource_copy_region(struct pipe_context *_pipe,
                                       {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(uint, dst_level);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, dstz);
   trace_dump_arg(ptr, src);
   trace_dump_arg(uint, src_level);
            pipe->resource_copy_region(pipe,
                     }
         static void
   trace_context_blit(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_flush_resource(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_clear(struct pipe_context *_pipe,
                     unsigned buffers,
   const struct pipe_scissor_state *scissor_state,
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, buffers);
   trace_dump_arg_begin("scissor_state");
   trace_dump_scissor_state(scissor_state);
   trace_dump_arg_end();
   if (color)
         else
         trace_dump_arg(float, depth);
                        }
         static void
   trace_context_clear_render_target(struct pipe_context *_pipe,
                                 {
      struct trace_context *tr_ctx = trace_context(_pipe);
                              trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg_array(uint, color->ui, 4);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);
            pipe->clear_render_target(pipe, dst, color, dstx, dsty, width, height,
               }
      static void
   trace_context_clear_depth_stencil(struct pipe_context *_pipe,
                                    struct pipe_surface *dst,
      {
      struct trace_context *tr_ctx = trace_context(_pipe);
                              trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(uint, clear_flags);
   trace_dump_arg(float, depth);
   trace_dump_arg(uint, stencil);
   trace_dump_arg(uint, dstx);
   trace_dump_arg(uint, dsty);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);
            pipe->clear_depth_stencil(pipe, dst, clear_flags, depth, stencil,
                     }
      static inline void
   trace_context_clear_buffer(struct pipe_context *_pipe,
                                 {
      struct trace_context *tr_ctx = trace_context(_pipe);
                        trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, res);
   trace_dump_arg(uint, offset);
   trace_dump_arg(uint, size);
   trace_dump_arg(ptr, clear_value);
                        }
      static inline void
   trace_context_clear_texture(struct pipe_context *_pipe,
                           {
      struct trace_context *tr_ctx = trace_context(_pipe);
   const struct util_format_description *desc = util_format_description(res->format);
   struct pipe_context *pipe = tr_ctx->pipe;
   union pipe_color_union color;
   float depth = 0.0f;
            trace_dump_call_begin("pipe_context", "clear_texture");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, res);
   trace_dump_arg(uint, level);
   trace_dump_arg_begin("box");
   trace_dump_box(box);
   trace_dump_arg_end();
   if (util_format_has_depth(desc)) {
      util_format_unpack_z_float(res->format, &depth, data, 1);
      }
   if (util_format_has_stencil(desc)) {
      util_format_unpack_s_8uint(res->format, &stencil, data, 1);
      }
   if (!util_format_is_depth_or_stencil(res->format)) {
      util_format_unpack_rgba(res->format, color.ui, data, 1);
                           }
      static void
   trace_context_flush(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                     if (fence)
                     if (flags & PIPE_FLUSH_END_OF_FRAME) {
      trace_dump_check_trigger();
         }
         static void
   trace_context_create_fence_fd(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg_enum(pipe_fd_type, fd);
                     if (fence)
               }
         static void
   trace_context_fence_server_sync(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static void
   trace_context_fence_server_signal(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
                        }
         static inline bool
   trace_context_generate_mipmap(struct pipe_context *_pipe,
                                 struct pipe_resource *res,
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
                     trace_dump_arg(ptr, pipe);
            trace_dump_arg(format, format);
   trace_dump_arg(uint, base_level);
   trace_dump_arg(uint, last_level);
   trace_dump_arg(uint, first_layer);
            ret = pipe->generate_mipmap(pipe, res, format, base_level, last_level,
            trace_dump_ret(bool, ret);
               }
         static void
   trace_context_destroy(struct pipe_context *_pipe)
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "destroy");
   trace_dump_arg(ptr, pipe);
                        }
         /********************************************************************
   * transfer
   */
         static void *
   trace_context_transfer_map(struct pipe_context *_context,
                                 {
      struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *pipe = tr_context->pipe;
   struct pipe_transfer *xfer = NULL;
            if (resource->target == PIPE_BUFFER)
         else
         if (!map)
         *transfer = trace_transfer_create(tr_context, resource, xfer);
            trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
   trace_dump_arg_enum(pipe_map_flags, usage);
            trace_dump_arg(ptr, xfer);
                     if (map) {
      if (usage & PIPE_MAP_WRITE) {
                        }
      static void
   trace_context_transfer_flush_region( struct pipe_context *_context,
               {
      struct trace_context *tr_context = trace_context(_context);
   struct trace_transfer *tr_transfer = trace_transfer(_transfer);
   struct pipe_context *pipe = tr_context->pipe;
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, transfer);
                        }
      static void
   trace_context_transfer_unmap(struct pipe_context *_context,
         {
      struct trace_context *tr_ctx = trace_context(_context);
   struct trace_transfer *tr_trans = trace_transfer(_transfer);
   struct pipe_context *context = tr_ctx->pipe;
                        trace_dump_arg(ptr, context);
                     if (tr_trans->map && !tr_ctx->threaded) {
      /*
   * Fake a texture/buffer_subdata
            struct pipe_resource *resource = transfer->resource;
   unsigned usage = transfer->usage;
   const struct pipe_box *box = &transfer->box;
   unsigned stride = transfer->stride;
            if (resource->target == PIPE_BUFFER) {
                              trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg_enum(pipe_map_flags, usage);
                  trace_dump_arg_begin("data");
   trace_dump_box_bytes(tr_trans->map,
                                                      } else {
                        trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
                  trace_dump_arg_begin("data");
   trace_dump_box_bytes(tr_trans->map,
                                                                           if (transfer->resource->target == PIPE_BUFFER)
         else
            }
         static void
   trace_context_buffer_subdata(struct pipe_context *_context,
                     {
      struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;
                     trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg_enum(pipe_map_flags, usage);
   trace_dump_arg(uint, offset);
            trace_dump_arg_begin("data");
   u_box_1d(offset, size, &box);
   trace_dump_box_bytes(data, resource, &box, 0, 0);
                        }
         static void
   trace_context_texture_subdata(struct pipe_context *_context,
                                 struct pipe_resource *resource,
   unsigned level,
   {
      struct trace_context *tr_context = trace_context(_context);
                     trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
   trace_dump_arg_enum(pipe_map_flags, usage);
            trace_dump_arg_begin("data");
   trace_dump_box_bytes(data,
                                    trace_dump_arg(uint, stride);
                     context->texture_subdata(context, resource, level, usage, box,
      }
      static void
   trace_context_invalidate_resource(struct pipe_context *_context,
         {
      struct trace_context *tr_context = trace_context(_context);
                     trace_dump_arg(ptr, context);
                        }
      static void
   trace_context_set_context_param(struct pipe_context *_context,
               {
      struct trace_context *tr_context = trace_context(_context);
                     trace_dump_arg(ptr, context);
   trace_dump_arg(uint, param);
                        }
      static void
   trace_context_set_debug_callback(struct pipe_context *_context, const struct util_debug_callback *cb)
   {
      struct trace_context *tr_context = trace_context(_context);
                                          }
      static void
   trace_context_render_condition(struct pipe_context *_context,
                     {
      struct trace_context *tr_context = trace_context(_context);
                              trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, query);
   trace_dump_arg(bool, condition);
                        }
      static void
   trace_context_render_condition_mem(struct pipe_context *_context,
                     {
      struct trace_context *tr_context = trace_context(_context);
                     trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, buffer);
   trace_dump_arg(uint, offset);
                        }
         static void
   trace_context_texture_barrier(struct pipe_context *_context, unsigned flags)
   {
      struct trace_context *tr_context = trace_context(_context);
                     trace_dump_arg(ptr, context);
                        }
         static void
   trace_context_memory_barrier(struct pipe_context *_context,
         {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "memory_barrier");
   trace_dump_arg(ptr, context);
   trace_dump_arg(uint, flags);
               }
         static bool
   trace_context_resource_commit(struct pipe_context *_context,
               {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "resource_commit");
   trace_dump_arg(ptr, context);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
   trace_dump_arg(box, box);
   trace_dump_arg(bool, commit);
               }
      static struct pipe_video_codec *
   trace_context_create_video_codec(struct pipe_context *_context,
         {
      struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;
                     trace_dump_arg(ptr, context);
                     trace_dump_ret(ptr, result);
                        }
      static struct pipe_video_buffer *
   trace_context_create_video_buffer_with_modifiers(struct pipe_context *_context,
                     {
      struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;
                     trace_dump_arg(ptr, context);
   trace_dump_arg(video_buffer_template, templat);
   trace_dump_arg_array(uint, modifiers, modifiers_count);
                     trace_dump_ret(ptr, result);
                        }
      static struct pipe_video_buffer *
   trace_context_create_video_buffer(struct pipe_context *_context,
         {
      struct trace_context *tr_context = trace_context(_context);
   struct pipe_context *context = tr_context->pipe;
                     trace_dump_arg(ptr, context);
                     trace_dump_ret(ptr, result);
                        }
      static void
   trace_context_set_tess_state(struct pipe_context *_context,
               {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "set_tess_state");
   trace_dump_arg(ptr, context);
   trace_dump_arg_array(float, default_outer_level, 4);
   trace_dump_arg_array(float, default_inner_level, 2);
               }
      static void
   trace_context_set_patch_vertices(struct pipe_context *_context,
         {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "set_patch_vertices");
   trace_dump_arg(ptr, context);
   trace_dump_arg(uint, patch_vertices);
               }
      static void trace_context_set_shader_buffers(struct pipe_context *_context,
                           {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "set_shader_buffers");
   trace_dump_arg(ptr, context);
   trace_dump_arg(uint, shader);
   trace_dump_arg(uint, start);
   trace_dump_arg_begin("buffers");
   trace_dump_struct_array(shader_buffer, buffers, nr);
   trace_dump_arg_end();
   trace_dump_arg(uint, writable_bitmask);
            context->set_shader_buffers(context, shader, start, nr, buffers,
      }
      static void trace_context_set_shader_images(struct pipe_context *_context,
                           {
      struct trace_context *tr_context = trace_context(_context);
            trace_dump_call_begin("pipe_context", "set_shader_images");
   trace_dump_arg(ptr, context);
   trace_dump_arg(uint, shader);
   trace_dump_arg(uint, start);
   trace_dump_arg_begin("images");
   trace_dump_struct_array(image_view, images, nr);
   trace_dump_arg_end();
   trace_dump_arg(uint, unbind_num_trailing_slots);
            context->set_shader_images(context, shader, start, nr,
      }
      static void trace_context_launch_grid(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr,  pipe);
                                 }
      static uint64_t trace_context_create_texture_handle(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
            trace_dump_call_begin("pipe_context", "create_texture_handle");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, view);
                     uintptr_t *texture_handle = (uintptr_t*)(uintptr_t)handle;
                        }
      static void trace_context_delete_texture_handle(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "delete_texture_handle");
   trace_dump_arg(ptr, pipe);
   uintptr_t *texture_handle = (uintptr_t*)(uintptr_t)handle;
   trace_dump_ret(ptr, texture_handle);
               }
      static void trace_context_make_texture_handle_resident(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "make_texture_handle_resident");
   trace_dump_arg(ptr, pipe);
   uintptr_t *texture_handle = (uintptr_t*)(uintptr_t)handle;
   trace_dump_ret(ptr, texture_handle);
   trace_dump_arg(bool, resident);
               }
      static uint64_t trace_context_create_image_handle(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
   struct pipe_context *pipe = tr_ctx->pipe;
            trace_dump_call_begin("pipe_context", "create_image_handle");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg_begin("image");
   trace_dump_image_view(image);
                     uintptr_t *image_handle = (uintptr_t*)(uintptr_t)handle;
   trace_dump_ret(ptr, image_handle);
               }
      static void trace_context_delete_image_handle(struct pipe_context *_pipe,
         {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "delete_image_handle");
   trace_dump_arg(ptr, pipe);
   uintptr_t *image_handle = (uintptr_t*)(uintptr_t)handle;
   trace_dump_ret(ptr, image_handle);
               }
      static void trace_context_make_image_handle_resident(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "make_image_handle_resident");
   trace_dump_arg(ptr, pipe);
   uintptr_t *image_handle = (uintptr_t*)(uintptr_t)handle;
   trace_dump_ret(ptr, image_handle);
   trace_dump_arg(uint, access);
   trace_dump_arg(bool, resident);
               }
      static void trace_context_set_global_binding(struct pipe_context *_pipe,
                     {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "set_global_binding");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, first);
   trace_dump_arg(uint, count);
   trace_dump_arg_array(ptr, resources, count);
                     /* TODO: the handles are 64 bit if ADDRESS_BITS are 64, this is better than
   * nothing though
   */
   trace_dump_ret_array_val(uint, handles, count);
      }
      static void
   trace_context_set_hw_atomic_buffers(struct pipe_context *_pipe,
               {
      struct trace_context *tr_ctx = trace_context(_pipe);
            trace_dump_call_begin("pipe_context", "set_hw_atomic_buffers");
   trace_dump_arg(ptr, pipe);
   trace_dump_arg(uint, start_slot);
            trace_dump_arg_begin("buffers");
   trace_dump_struct_array(shader_buffer, buffers, count);
                        }
      struct pipe_context *
   trace_context_create(struct trace_screen *tr_scr,
         {
               if (!pipe)
            if (!trace_enabled())
            tr_ctx = rzalloc(NULL, struct trace_context);
   if (!tr_ctx)
            _mesa_hash_table_init(&tr_ctx->blend_states, tr_ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
   _mesa_hash_table_init(&tr_ctx->rasterizer_states, tr_ctx, _mesa_hash_pointer, _mesa_key_pointer_equal);
            tr_ctx->base.priv = pipe->priv; /* expose wrapped priv data */
   tr_ctx->base.screen = &tr_scr->base;
   tr_ctx->base.stream_uploader = pipe->stream_uploader;
                  #define TR_CTX_INIT(_member) \
               TR_CTX_INIT(draw_vbo);
   TR_CTX_INIT(draw_mesh_tasks);
   TR_CTX_INIT(draw_vertex_state);
   TR_CTX_INIT(render_condition);
   TR_CTX_INIT(render_condition_mem);
   TR_CTX_INIT(create_query);
   TR_CTX_INIT(destroy_query);
   TR_CTX_INIT(begin_query);
   TR_CTX_INIT(end_query);
   TR_CTX_INIT(get_query_result);
   TR_CTX_INIT(get_query_result_resource);
   TR_CTX_INIT(set_active_query_state);
   TR_CTX_INIT(create_blend_state);
   TR_CTX_INIT(bind_blend_state);
   TR_CTX_INIT(delete_blend_state);
   TR_CTX_INIT(create_sampler_state);
   TR_CTX_INIT(bind_sampler_states);
   TR_CTX_INIT(delete_sampler_state);
   TR_CTX_INIT(create_rasterizer_state);
   TR_CTX_INIT(bind_rasterizer_state);
   TR_CTX_INIT(delete_rasterizer_state);
   TR_CTX_INIT(create_depth_stencil_alpha_state);
   TR_CTX_INIT(bind_depth_stencil_alpha_state);
   TR_CTX_INIT(delete_depth_stencil_alpha_state);
   TR_CTX_INIT(create_fs_state);
   TR_CTX_INIT(bind_fs_state);
   TR_CTX_INIT(delete_fs_state);
   TR_CTX_INIT(create_vs_state);
   TR_CTX_INIT(bind_vs_state);
   TR_CTX_INIT(delete_vs_state);
   TR_CTX_INIT(create_gs_state);
   TR_CTX_INIT(bind_gs_state);
   TR_CTX_INIT(delete_gs_state);
   TR_CTX_INIT(create_tcs_state);
   TR_CTX_INIT(bind_tcs_state);
   TR_CTX_INIT(delete_tcs_state);
   TR_CTX_INIT(create_tes_state);
   TR_CTX_INIT(bind_tes_state);
   TR_CTX_INIT(delete_tes_state);
   TR_CTX_INIT(create_ts_state);
   TR_CTX_INIT(bind_ts_state);
   TR_CTX_INIT(delete_ts_state);
   TR_CTX_INIT(create_ms_state);
   TR_CTX_INIT(bind_ms_state);
   TR_CTX_INIT(delete_ms_state);
   TR_CTX_INIT(create_compute_state);
   TR_CTX_INIT(bind_compute_state);
   TR_CTX_INIT(delete_compute_state);
   TR_CTX_INIT(link_shader);
   TR_CTX_INIT(create_vertex_elements_state);
   TR_CTX_INIT(bind_vertex_elements_state);
   TR_CTX_INIT(delete_vertex_elements_state);
   TR_CTX_INIT(set_blend_color);
   TR_CTX_INIT(set_stencil_ref);
   TR_CTX_INIT(set_clip_state);
   TR_CTX_INIT(set_sample_mask);
   TR_CTX_INIT(set_constant_buffer);
   TR_CTX_INIT(set_framebuffer_state);
   TR_CTX_INIT(set_inlinable_constants);
   TR_CTX_INIT(set_polygon_stipple);
   TR_CTX_INIT(set_min_samples);
   TR_CTX_INIT(set_scissor_states);
   TR_CTX_INIT(set_viewport_states);
   TR_CTX_INIT(set_sampler_views);
   TR_CTX_INIT(create_sampler_view);
   TR_CTX_INIT(sampler_view_destroy);
   TR_CTX_INIT(create_surface);
   TR_CTX_INIT(surface_destroy);
   TR_CTX_INIT(set_vertex_buffers);
   TR_CTX_INIT(create_stream_output_target);
   TR_CTX_INIT(stream_output_target_destroy);
   TR_CTX_INIT(set_stream_output_targets);
   /* this is lavapipe-only and can't be traced */
   tr_ctx->base.stream_output_target_offset = pipe->stream_output_target_offset;
   TR_CTX_INIT(resource_copy_region);
   TR_CTX_INIT(blit);
   TR_CTX_INIT(flush_resource);
   TR_CTX_INIT(clear);
   TR_CTX_INIT(clear_render_target);
   TR_CTX_INIT(clear_depth_stencil);
   TR_CTX_INIT(clear_texture);
   TR_CTX_INIT(clear_buffer);
   TR_CTX_INIT(flush);
   TR_CTX_INIT(create_fence_fd);
   TR_CTX_INIT(fence_server_sync);
   TR_CTX_INIT(fence_server_signal);
   TR_CTX_INIT(generate_mipmap);
   TR_CTX_INIT(texture_barrier);
   TR_CTX_INIT(memory_barrier);
   TR_CTX_INIT(resource_commit);
   TR_CTX_INIT(create_video_codec);
   TR_CTX_INIT(create_video_buffer_with_modifiers);
   TR_CTX_INIT(create_video_buffer);
   TR_CTX_INIT(set_tess_state);
   TR_CTX_INIT(set_patch_vertices);
   TR_CTX_INIT(set_shader_buffers);
   TR_CTX_INIT(launch_grid);
   TR_CTX_INIT(set_shader_images);
   TR_CTX_INIT(create_texture_handle);
   TR_CTX_INIT(delete_texture_handle);
   TR_CTX_INIT(make_texture_handle_resident);
   TR_CTX_INIT(create_image_handle);
   TR_CTX_INIT(delete_image_handle);
            tr_ctx->base.buffer_map = tr_ctx->base.texture_map = trace_context_transfer_map;
   tr_ctx->base.buffer_unmap = tr_ctx->base.texture_unmap = trace_context_transfer_unmap;
   TR_CTX_INIT(transfer_flush_region);
   TR_CTX_INIT(buffer_subdata);
   TR_CTX_INIT(texture_subdata);
   TR_CTX_INIT(invalidate_resource);
   TR_CTX_INIT(set_context_param);
   TR_CTX_INIT(set_debug_callback);
   TR_CTX_INIT(set_global_binding);
            #undef TR_CTX_INIT
                        error1:
         }
         /**
   * Sanity checker: check that the given context really is a
   * trace context (and not the wrapped driver's context).
   */
   void
   trace_context_check(const struct pipe_context *pipe)
   {
      ASSERTED struct trace_context *tr_ctx = (struct trace_context *) pipe;
      }
      /**
   * Threaded context is not wrapped, and so it may call fence functions directly
   */
   struct pipe_context *
   trace_get_possibly_threaded_context(struct pipe_context *pipe)
   {
         }
