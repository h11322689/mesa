   /*
   * Copyright © 2014-2018 NVIDIA Corporation
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
      #include <inttypes.h>
   #include <stdlib.h>
      #include "util/u_debug.h"
   #include "util/u_draw.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
      #include "tegra_context.h"
   #include "tegra_resource.h"
   #include "tegra_screen.h"
      static void
   tegra_destroy(struct pipe_context *pcontext)
   {
               if (context->base.stream_uploader)
            context->gpu->destroy(context->gpu);
      }
      static void
   tegra_draw_vbo(struct pipe_context *pcontext,
                  const struct pipe_draw_info *pinfo,
   unsigned drawid_offset,
      {
      if (num_draws > 1) {
      util_draw_multi(pcontext, pinfo, drawid_offset, pindirect, draws, num_draws);
               if (!pindirect && (!draws[0].count || !pinfo->instance_count))
            struct tegra_context *context = to_tegra_context(pcontext);
   struct pipe_draw_indirect_info indirect;
            if (pinfo && ((pindirect && pindirect->buffer) || pinfo->index_size)) {
               if (pindirect && pindirect->buffer) {
      memcpy(&indirect, pindirect, sizeof(indirect));
   indirect.buffer = tegra_resource_unwrap(pindirect->buffer);
   indirect.indirect_draw_count = tegra_resource_unwrap(pindirect->indirect_draw_count);
               if (pinfo->index_size && !pinfo->has_user_indices)
                           }
      static void
   tegra_render_condition(struct pipe_context *pcontext,
                     {
                  }
      static struct pipe_query *
   tegra_create_query(struct pipe_context *pcontext, unsigned int query_type,
         {
                  }
      static struct pipe_query *
   tegra_create_batch_query(struct pipe_context *pcontext,
               {
               return context->gpu->create_batch_query(context->gpu, num_queries,
      }
      static void
   tegra_destroy_query(struct pipe_context *pcontext, struct pipe_query *query)
   {
                  }
      static bool
   tegra_begin_query(struct pipe_context *pcontext, struct pipe_query *query)
   {
                  }
      static bool
   tegra_end_query(struct pipe_context *pcontext, struct pipe_query *query)
   {
                  }
      static bool
   tegra_get_query_result(struct pipe_context *pcontext,
                     {
               return context->gpu->get_query_result(context->gpu, query, wait,
      }
      static void
   tegra_get_query_result_resource(struct pipe_context *pcontext,
                                 struct pipe_query *query,
   {
               context->gpu->get_query_result_resource(context->gpu, query, flags,
            }
      static void
   tegra_set_active_query_state(struct pipe_context *pcontext, bool enable)
   {
                  }
      static void *
   tegra_create_blend_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_blend_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_blend_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_sampler_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_sampler_states(struct pipe_context *pcontext, enum pipe_shader_type shader,
               {
               context->gpu->bind_sampler_states(context->gpu, shader, start_slot,
      }
      static void
   tegra_delete_sampler_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_rasterizer_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_rasterizer_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_rasterizer_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_depth_stencil_alpha_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_depth_stencil_alpha_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_depth_stencil_alpha_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_fs_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_fs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_fs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_vs_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_vs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_vs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_gs_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_gs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_gs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_tcs_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_tcs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_tcs_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_tes_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_tes_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_tes_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void *
   tegra_create_vertex_elements_state(struct pipe_context *pcontext,
               {
               return context->gpu->create_vertex_elements_state(context->gpu,
            }
      static void
   tegra_bind_vertex_elements_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_vertex_elements_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_set_blend_color(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_stencil_ref(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_sample_mask(struct pipe_context *pcontext, unsigned int mask)
   {
                  }
      static void
   tegra_set_min_samples(struct pipe_context *pcontext, unsigned int samples)
   {
                  }
      static void
   tegra_set_clip_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_constant_buffer(struct pipe_context *pcontext, enum pipe_shader_type shader,
               {
      struct tegra_context *context = to_tegra_context(pcontext);
            if (buf && buf->buffer) {
      memcpy(&buffer, buf, sizeof(buffer));
   buffer.buffer = tegra_resource_unwrap(buffer.buffer);
                  }
      static void
   tegra_set_framebuffer_state(struct pipe_context *pcontext,
         {
      struct tegra_context *context = to_tegra_context(pcontext);
   struct pipe_framebuffer_state state;
            if (fb) {
               for (i = 0; i < fb->nr_cbufs; i++)
            while (i < PIPE_MAX_COLOR_BUFS)
                                    }
      static void
   tegra_set_polygon_stipple(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_scissor_states(struct pipe_context *pcontext, unsigned start_slot,
               {
               context->gpu->set_scissor_states(context->gpu, start_slot, num_scissors,
      }
      static void
   tegra_set_window_rectangles(struct pipe_context *pcontext, bool include,
               {
               context->gpu->set_window_rectangles(context->gpu, include, num_rectangles,
      }
      static void
   tegra_set_viewport_states(struct pipe_context *pcontext, unsigned start_slot,
               {
               context->gpu->set_viewport_states(context->gpu, start_slot, num_viewports,
      }
      static void
   tegra_set_sampler_views(struct pipe_context *pcontext, enum pipe_shader_type shader,
                           {
      struct pipe_sampler_view *views[PIPE_MAX_SHADER_SAMPLER_VIEWS];
   struct tegra_context *context = to_tegra_context(pcontext);
   struct tegra_sampler_view *view;
            for (i = 0; i < num_views; i++) {
      /* adjust private reference count */
   view = to_tegra_sampler_view(pviews[i]);
   if (view) {
      view->refcount--;
   if (!view->refcount) {
      view->refcount = 100000000;
                              context->gpu->set_sampler_views(context->gpu, shader, start_slot,
            }
      static void
   tegra_set_tess_state(struct pipe_context *pcontext,
               {
               context->gpu->set_tess_state(context->gpu, default_outer_level,
      }
      static void
   tegra_set_debug_callback(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_shader_buffers(struct pipe_context *pcontext, enum pipe_shader_type shader,
                     {
               context->gpu->set_shader_buffers(context->gpu, shader, start, count,
      }
      static void
   tegra_set_shader_images(struct pipe_context *pcontext, enum pipe_shader_type shader,
                     {
               context->gpu->set_shader_images(context->gpu, shader, start, count,
      }
      static void
   tegra_set_vertex_buffers(struct pipe_context *pcontext,
                     {
      struct tegra_context *context = to_tegra_context(pcontext);
   struct pipe_vertex_buffer buf[PIPE_MAX_SHADER_INPUTS];
            if (num_buffers && buffers) {
               for (i = 0; i < num_buffers; i++) {
      if (!buf[i].is_user_buffer)
                           context->gpu->set_vertex_buffers(context->gpu, num_buffers,
            }
      static struct pipe_stream_output_target *
   tegra_create_stream_output_target(struct pipe_context *pcontext,
                     {
      struct tegra_resource *resource = to_tegra_resource(presource);
            return context->gpu->create_stream_output_target(context->gpu,
                  }
      static void
   tegra_stream_output_target_destroy(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_set_stream_output_targets(struct pipe_context *pcontext,
                     {
               context->gpu->set_stream_output_targets(context->gpu, num_targets,
      }
      static void
   tegra_resource_copy_region(struct pipe_context *pcontext,
                              struct pipe_resource *pdst,
   unsigned int dst_level,
   unsigned int dstx,
      {
      struct tegra_context *context = to_tegra_context(pcontext);
   struct tegra_resource *dst = to_tegra_resource(pdst);
            context->gpu->resource_copy_region(context->gpu, dst->gpu, dst_level, dstx,
            }
      static void
   tegra_blit(struct pipe_context *pcontext, const struct pipe_blit_info *pinfo)
   {
      struct tegra_context *context = to_tegra_context(pcontext);
            if (pinfo) {
      memcpy(&info, pinfo, sizeof(info));
   info.dst.resource = tegra_resource_unwrap(info.dst.resource);
   info.src.resource = tegra_resource_unwrap(info.src.resource);
                  }
      static void
   tegra_clear(struct pipe_context *pcontext, unsigned buffers, const struct pipe_scissor_state *scissor_state,
               {
                  }
      static void
   tegra_clear_render_target(struct pipe_context *pcontext,
                           struct pipe_surface *pdst,
   const union pipe_color_union *color,
   unsigned int dstx,
   {
      struct tegra_context *context = to_tegra_context(pcontext);
            context->gpu->clear_render_target(context->gpu, dst->gpu, color, dstx,
      }
      static void
   tegra_clear_depth_stencil(struct pipe_context *pcontext,
                           struct pipe_surface *pdst,
   unsigned int flags,
   double depth,
   unsigned int stencil,
   unsigned int dstx,
   {
      struct tegra_context *context = to_tegra_context(pcontext);
            context->gpu->clear_depth_stencil(context->gpu, dst->gpu, flags, depth,
            }
      static void
   tegra_clear_texture(struct pipe_context *pcontext,
                     struct pipe_resource *presource,
   {
      struct tegra_resource *resource = to_tegra_resource(presource);
               }
      static void
   tegra_clear_buffer(struct pipe_context *pcontext,
                     struct pipe_resource *presource,
   unsigned int offset,
   {
      struct tegra_resource *resource = to_tegra_resource(presource);
            context->gpu->clear_buffer(context->gpu, resource->gpu, offset, size,
      }
      static void
   tegra_flush(struct pipe_context *pcontext, struct pipe_fence_handle **fence,
         {
                  }
      static void
   tegra_create_fence_fd(struct pipe_context *pcontext,
               {
               assert(type == PIPE_FD_TYPE_NATIVE_SYNC);
      }
      static void
   tegra_fence_server_sync(struct pipe_context *pcontext,
         {
                  }
      static struct pipe_sampler_view *
   tegra_create_sampler_view(struct pipe_context *pcontext,
               {
      struct tegra_resource *resource = to_tegra_resource(presource);
   struct tegra_context *context = to_tegra_context(pcontext);
            view = calloc(1, sizeof(*view));
   if (!view)
            view->base = *template;
   view->base.context = pcontext;
   /* overwrite to prevent reference from being released */
   view->base.texture = NULL;
   pipe_reference_init(&view->base.reference, 1);
            view->gpu = context->gpu->create_sampler_view(context->gpu, resource->gpu,
            /* use private reference count */
   view->gpu->reference.count += 100000000;
               }
      static void
   tegra_sampler_view_destroy(struct pipe_context *pcontext,
         {
               pipe_resource_reference(&view->base.texture, NULL);
   /* adjust private reference count */
   p_atomic_add(&view->gpu->reference.count, -view->refcount);
   pipe_sampler_view_reference(&view->gpu, NULL);
      }
      static struct pipe_surface *
   tegra_create_surface(struct pipe_context *pcontext,
               {
      struct tegra_resource *resource = to_tegra_resource(presource);
   struct tegra_context *context = to_tegra_context(pcontext);
            surface = calloc(1, sizeof(*surface));
   if (!surface)
            surface->gpu = context->gpu->create_surface(context->gpu, resource->gpu,
         if (!surface->gpu) {
      free(surface);
               memcpy(&surface->base, surface->gpu, sizeof(*surface->gpu));
   /* overwrite to prevent reference from being released */
            pipe_reference_init(&surface->base.reference, 1);
   pipe_resource_reference(&surface->base.texture, presource);
               }
      static void
   tegra_surface_destroy(struct pipe_context *pcontext,
         {
               pipe_resource_reference(&surface->base.texture, NULL);
   pipe_surface_reference(&surface->gpu, NULL);
      }
      static void *
   tegra_transfer_map(struct pipe_context *pcontext,
                     struct pipe_resource *presource,
   {
      struct tegra_resource *resource = to_tegra_resource(presource);
   struct tegra_context *context = to_tegra_context(pcontext);
            transfer = calloc(1, sizeof(*transfer));
   if (!transfer)
            if (presource->target == PIPE_BUFFER) {
      transfer->map = context->gpu->buffer_map(context->gpu, resource->gpu,
            } else {
      transfer->map = context->gpu->texture_map(context->gpu, resource->gpu,
            }
   memcpy(&transfer->base, transfer->gpu, sizeof(*transfer->gpu));
   transfer->base.resource = NULL;
                        }
      static void
   tegra_transfer_flush_region(struct pipe_context *pcontext,
               {
      struct tegra_transfer *transfer = to_tegra_transfer(ptransfer);
               }
      static void
   tegra_transfer_unmap(struct pipe_context *pcontext,
         {
      struct tegra_transfer *transfer = to_tegra_transfer(ptransfer);
            if (ptransfer->resource->target == PIPE_BUFFER)
         else
         pipe_resource_reference(&transfer->base.resource, NULL);
      }
      static void
   tegra_buffer_subdata(struct pipe_context *pcontext,
                     {
      struct tegra_resource *resource = to_tegra_resource(presource);
            context->gpu->buffer_subdata(context->gpu, resource->gpu, usage, offset,
      }
      static void
   tegra_texture_subdata(struct pipe_context *pcontext,
                        struct pipe_resource *presource,
   unsigned level,
   unsigned usage,
      {
      struct tegra_resource *resource = to_tegra_resource(presource);
            context->gpu->texture_subdata(context->gpu, resource->gpu, level, usage,
      }
      static void
   tegra_texture_barrier(struct pipe_context *pcontext, unsigned int flags)
   {
                  }
      static void
   tegra_memory_barrier(struct pipe_context *pcontext, unsigned int flags)
   {
               if (!(flags & ~PIPE_BARRIER_UPDATE))
               }
      static struct pipe_video_codec *
   tegra_create_video_codec(struct pipe_context *pcontext,
         {
                  }
      static struct pipe_video_buffer *
   tegra_create_video_buffer(struct pipe_context *pcontext,
         {
                  }
      static void *
   tegra_create_compute_state(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_bind_compute_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_delete_compute_state(struct pipe_context *pcontext, void *so)
   {
                  }
      static void
   tegra_set_compute_resources(struct pipe_context *pcontext,
               {
                           }
      static void
   tegra_set_global_binding(struct pipe_context *pcontext, unsigned int first,
               {
                        context->gpu->set_global_binding(context->gpu, first, count, resources,
      }
      static void
   tegra_launch_grid(struct pipe_context *pcontext,
         {
                           }
      static void
   tegra_get_sample_position(struct pipe_context *pcontext, unsigned int count,
         {
                  }
      static uint64_t
   tegra_get_timestamp(struct pipe_context *pcontext)
   {
                  }
      static void
   tegra_flush_resource(struct pipe_context *pcontext,
         {
      struct tegra_resource *resource = to_tegra_resource(presource);
               }
      static void
   tegra_invalidate_resource(struct pipe_context *pcontext,
         {
      struct tegra_resource *resource = to_tegra_resource(presource);
               }
      static enum pipe_reset_status
   tegra_get_device_reset_status(struct pipe_context *pcontext)
   {
                  }
      static void
   tegra_set_device_reset_callback(struct pipe_context *pcontext,
         {
                  }
      static void
   tegra_dump_debug_state(struct pipe_context *pcontext, FILE *stream,
         {
                  }
      static void
   tegra_emit_string_marker(struct pipe_context *pcontext, const char *string,
         {
                  }
      static bool
   tegra_generate_mipmap(struct pipe_context *pcontext,
                        struct pipe_resource *presource,
   enum pipe_format format,
      {
      struct tegra_resource *resource = to_tegra_resource(presource);
            return context->gpu->generate_mipmap(context->gpu, resource->gpu, format,
            }
      static uint64_t
   tegra_create_texture_handle(struct pipe_context *pcontext,
               {
                  }
      static void tegra_delete_texture_handle(struct pipe_context *pcontext,
         {
                  }
      static void tegra_make_texture_handle_resident(struct pipe_context *pcontext,
         {
                  }
      static uint64_t tegra_create_image_handle(struct pipe_context *pcontext,
         {
                  }
      static void tegra_delete_image_handle(struct pipe_context *pcontext,
         {
                  }
      static void tegra_make_image_handle_resident(struct pipe_context *pcontext,
               {
               context->gpu->make_image_handle_resident(context->gpu, handle, access,
      }
      struct pipe_context *
   tegra_screen_context_create(struct pipe_screen *pscreen, void *priv,
         {
      struct tegra_screen *screen = to_tegra_screen(pscreen);
            context = calloc(1, sizeof(*context));
   if (!context)
            context->gpu = screen->gpu->context_create(screen->gpu, priv, flags);
   if (!context->gpu) {
      debug_error("failed to create GPU context\n");
               context->base.screen = &screen->base;
            /*
   * Create custom stream and const uploaders. Note that technically nouveau
   * already creates uploaders that could be reused, but that would make the
   * resource unwrapping rather complicate. The reason for that is that both
   * uploaders create resources based on the context that they were created
   * from, which means that nouveau's uploader will use the nouveau context
   * which means that those resources must not be unwrapped. So before each
   * resource is unwrapped, the code would need to check that it does not
   * correspond to the uploaders' buffers.
   *
   * However, duplicating the uploaders here sounds worse than it is. The
   * default implementation that nouveau uses allocates buffers lazily, and
   * since it is never used, no buffers will every be allocated and the only
   * memory wasted is that occupied by the nouveau uploader itself.
   */
   context->base.stream_uploader = u_upload_create_default(&context->base);
   if (!context->base.stream_uploader)
                                                context->base.create_query = tegra_create_query;
   context->base.create_batch_query = tegra_create_batch_query;
   context->base.destroy_query = tegra_destroy_query;
   context->base.begin_query = tegra_begin_query;
   context->base.end_query = tegra_end_query;
   context->base.get_query_result = tegra_get_query_result;
   context->base.get_query_result_resource = tegra_get_query_result_resource;
            context->base.create_blend_state = tegra_create_blend_state;
   context->base.bind_blend_state = tegra_bind_blend_state;
            context->base.create_sampler_state = tegra_create_sampler_state;
   context->base.bind_sampler_states = tegra_bind_sampler_states;
            context->base.create_rasterizer_state = tegra_create_rasterizer_state;
   context->base.bind_rasterizer_state = tegra_bind_rasterizer_state;
            context->base.create_depth_stencil_alpha_state = tegra_create_depth_stencil_alpha_state;
   context->base.bind_depth_stencil_alpha_state = tegra_bind_depth_stencil_alpha_state;
            context->base.create_fs_state = tegra_create_fs_state;
   context->base.bind_fs_state = tegra_bind_fs_state;
            context->base.create_vs_state = tegra_create_vs_state;
   context->base.bind_vs_state = tegra_bind_vs_state;
            context->base.create_gs_state = tegra_create_gs_state;
   context->base.bind_gs_state = tegra_bind_gs_state;
            context->base.create_tcs_state = tegra_create_tcs_state;
   context->base.bind_tcs_state = tegra_bind_tcs_state;
            context->base.create_tes_state = tegra_create_tes_state;
   context->base.bind_tes_state = tegra_bind_tes_state;
            context->base.create_vertex_elements_state = tegra_create_vertex_elements_state;
   context->base.bind_vertex_elements_state = tegra_bind_vertex_elements_state;
            context->base.set_blend_color = tegra_set_blend_color;
   context->base.set_stencil_ref = tegra_set_stencil_ref;
   context->base.set_sample_mask = tegra_set_sample_mask;
   context->base.set_min_samples = tegra_set_min_samples;
            context->base.set_constant_buffer = tegra_set_constant_buffer;
   context->base.set_framebuffer_state = tegra_set_framebuffer_state;
   context->base.set_polygon_stipple = tegra_set_polygon_stipple;
   context->base.set_scissor_states = tegra_set_scissor_states;
   context->base.set_window_rectangles = tegra_set_window_rectangles;
   context->base.set_viewport_states = tegra_set_viewport_states;
   context->base.set_sampler_views = tegra_set_sampler_views;
                     context->base.set_shader_buffers = tegra_set_shader_buffers;
   context->base.set_shader_images = tegra_set_shader_images;
            context->base.create_stream_output_target = tegra_create_stream_output_target;
   context->base.stream_output_target_destroy = tegra_stream_output_target_destroy;
            context->base.resource_copy_region = tegra_resource_copy_region;
   context->base.blit = tegra_blit;
   context->base.clear = tegra_clear;
   context->base.clear_render_target = tegra_clear_render_target;
   context->base.clear_depth_stencil = tegra_clear_depth_stencil;
   context->base.clear_texture = tegra_clear_texture;
   context->base.clear_buffer = tegra_clear_buffer;
            context->base.create_fence_fd = tegra_create_fence_fd;
            context->base.create_sampler_view = tegra_create_sampler_view;
            context->base.create_surface = tegra_create_surface;
            context->base.buffer_map = tegra_transfer_map;
   context->base.texture_map = tegra_transfer_map;
   context->base.transfer_flush_region = tegra_transfer_flush_region;
   context->base.buffer_unmap = tegra_transfer_unmap;
   context->base.texture_unmap = tegra_transfer_unmap;
   context->base.buffer_subdata = tegra_buffer_subdata;
            context->base.texture_barrier = tegra_texture_barrier;
            context->base.create_video_codec = tegra_create_video_codec;
            context->base.create_compute_state = tegra_create_compute_state;
   context->base.bind_compute_state = tegra_bind_compute_state;
   context->base.delete_compute_state = tegra_delete_compute_state;
   context->base.set_compute_resources = tegra_set_compute_resources;
   context->base.set_global_binding = tegra_set_global_binding;
   context->base.launch_grid = tegra_launch_grid;
   context->base.get_sample_position = tegra_get_sample_position;
            context->base.flush_resource = tegra_flush_resource;
            context->base.get_device_reset_status = tegra_get_device_reset_status;
   context->base.set_device_reset_callback = tegra_set_device_reset_callback;
   context->base.dump_debug_state = tegra_dump_debug_state;
                     context->base.create_texture_handle = tegra_create_texture_handle;
   context->base.delete_texture_handle = tegra_delete_texture_handle;
   context->base.make_texture_handle_resident = tegra_make_texture_handle_resident;
   context->base.create_image_handle = tegra_create_image_handle;
   context->base.delete_image_handle = tegra_delete_image_handle;
                  destroy:
         free:
      free(context);
      }
