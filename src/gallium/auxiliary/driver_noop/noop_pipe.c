   /*
   * Copyright 2010 Red Hat Inc.
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
   */
   #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_helpers.h"
   #include "util/u_upload_mgr.h"
   #include "util/u_threaded_context.h"
   #include "noop_public.h"
      DEBUG_GET_ONCE_BOOL_OPTION(noop, "GALLIUM_NOOP", false)
      void noop_init_state_functions(struct pipe_context *ctx);
      struct noop_pipe_screen {
      struct pipe_screen	pscreen;
   struct pipe_screen	*oscreen;
      };
      /*
   * query
   */
   struct noop_query {
      struct threaded_query b;
      };
   static struct pipe_query *noop_create_query(struct pipe_context *ctx, unsigned query_type, unsigned index)
   {
                  }
      static void noop_destroy_query(struct pipe_context *ctx, struct pipe_query *query)
   {
         }
      static bool noop_begin_query(struct pipe_context *ctx, struct pipe_query *query)
   {
         }
      static bool noop_end_query(struct pipe_context *ctx, struct pipe_query *query)
   {
         }
      static bool noop_get_query_result(struct pipe_context *ctx,
                     {
               *result = 0;
      }
      static void
   noop_set_active_query_state(struct pipe_context *pipe, bool enable)
   {
   }
         /*
   * resource
   */
   struct noop_resource {
      struct threaded_resource b;
   unsigned		size;
   char			*data;
      };
      static struct pipe_resource *noop_resource_create(struct pipe_screen *screen,
         {
      struct noop_resource *nresource;
            nresource = CALLOC_STRUCT(noop_resource);
   if (!nresource)
            stride = util_format_get_stride(templ->format, templ->width0);
   nresource->b.b = *templ;
   nresource->b.b.screen = screen;
   nresource->size = stride * templ->height0 * templ->depth0;
   nresource->data = MALLOC(nresource->size);
   pipe_reference_init(&nresource->b.b.reference, 1);
   if (nresource->data == NULL) {
      FREE(nresource);
      }
   threaded_resource_init(&nresource->b.b, false);
      }
      static struct pipe_resource *
   noop_resource_create_with_modifiers(struct pipe_screen *screen,
               {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
   struct pipe_screen *oscreen = noop_screen->oscreen;
   struct pipe_resource *result;
            result = oscreen->resource_create_with_modifiers(oscreen, templ,
         noop_resource = noop_resource_create(screen, result);
   pipe_resource_reference(&result, NULL);
      }
      static struct pipe_resource *noop_resource_from_handle(struct pipe_screen *screen,
                     {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
   struct pipe_screen *oscreen = noop_screen->oscreen;
   struct pipe_resource *result;
            result = oscreen->resource_from_handle(oscreen, templ, handle, usage);
   noop_resource = noop_resource_create(screen, result);
   pipe_resource_reference(&result, NULL);
      }
      static bool noop_resource_get_handle(struct pipe_screen *pscreen,
                           {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)pscreen;
   struct pipe_screen *screen = noop_screen->oscreen;
   struct pipe_resource *tex;
            /* resource_get_handle musn't fail. Just create something and return it. */
   tex = screen->resource_create(screen, resource);
   if (!tex)
            result = screen->resource_get_handle(screen, NULL, tex, handle, usage);
   pipe_resource_reference(&tex, NULL);
      }
      static bool noop_resource_get_param(struct pipe_screen *pscreen,
                                       struct pipe_context *ctx,
   struct pipe_resource *resource,
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)pscreen;
   struct pipe_screen *screen = noop_screen->oscreen;
   struct pipe_resource *tex;
            /* resource_get_param mustn't fail. Just create something and return it. */
   tex = screen->resource_create(screen, resource);
   if (!tex)
            result = screen->resource_get_param(screen, NULL, tex, 0, 0, 0, param,
         pipe_resource_reference(&tex, NULL);
      }
      static void noop_resource_destroy(struct pipe_screen *screen,
         {
               threaded_resource_deinit(resource);
   FREE(nresource->data);
      }
         /*
   * transfer
   */
   static void *noop_transfer_map(struct pipe_context *pipe,
                                 {
      struct pipe_transfer *transfer;
            transfer = (struct pipe_transfer*)CALLOC_STRUCT(threaded_transfer);
   if (!transfer)
         pipe_resource_reference(&transfer->resource, resource);
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = 1;
   transfer->layer_stride = 1;
               }
      static void noop_transfer_flush_region(struct pipe_context *pipe,
               {
   }
      static void noop_transfer_unmap(struct pipe_context *pipe,
         {
      pipe_resource_reference(&transfer->resource, NULL);
      }
      static void noop_buffer_subdata(struct pipe_context *pipe,
                     {
   }
      static void noop_texture_subdata(struct pipe_context *pipe,
                                    struct pipe_resource *resource,
      {
   }
         /*
   * clear/copy
   */
   static void noop_clear(struct pipe_context *ctx, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
   }
      static void noop_clear_render_target(struct pipe_context *ctx,
                                 {
   }
      static void noop_clear_depth_stencil(struct pipe_context *ctx,
                                       struct pipe_surface *dst,
   {
   }
      static void noop_resource_copy_region(struct pipe_context *ctx,
                                       {
   }
         static void noop_blit(struct pipe_context *ctx,
         {
   }
         static void
   noop_flush_resource(struct pipe_context *ctx,
         {
   }
         /*
   * context
   */
   static void noop_flush(struct pipe_context *ctx,
               {
      if (fence) {
      struct pipe_reference *f = MALLOC_STRUCT(pipe_reference);
            ctx->screen->fence_reference(ctx->screen, fence, NULL);
         }
      static void noop_destroy_context(struct pipe_context *ctx)
   {
      if (ctx->stream_uploader)
            p_atomic_dec(&ctx->screen->num_contexts);
      }
      static bool noop_generate_mipmap(struct pipe_context *ctx,
                                       {
         }
      static void noop_invalidate_resource(struct pipe_context *ctx,
         {
   }
      static void noop_set_context_param(struct pipe_context *ctx,
               {
   }
      static void noop_set_frontend_noop(struct pipe_context *ctx, bool enable)
   {
   }
      static void noop_replace_buffer_storage(struct pipe_context *ctx,
                                 {
   }
      static struct pipe_fence_handle *
   noop_create_fence(struct pipe_context *ctx,
         {
               f->count = 1;
      }
      static bool noop_is_resource_busy(struct pipe_screen *screen,
               {
         }
      static struct pipe_context *noop_create_context(struct pipe_screen *screen,
         {
               if (!ctx)
            ctx->screen = screen;
            ctx->stream_uploader = u_upload_create_default(ctx);
   if (!ctx->stream_uploader) {
      FREE(ctx);
      }
            ctx->destroy = noop_destroy_context;
   ctx->flush = noop_flush;
   ctx->clear = noop_clear;
   ctx->clear_render_target = noop_clear_render_target;
   ctx->clear_depth_stencil = noop_clear_depth_stencil;
   ctx->resource_copy_region = noop_resource_copy_region;
   ctx->generate_mipmap = noop_generate_mipmap;
   ctx->blit = noop_blit;
   ctx->flush_resource = noop_flush_resource;
   ctx->create_query = noop_create_query;
   ctx->destroy_query = noop_destroy_query;
   ctx->begin_query = noop_begin_query;
   ctx->end_query = noop_end_query;
   ctx->get_query_result = noop_get_query_result;
   ctx->set_active_query_state = noop_set_active_query_state;
   ctx->buffer_map = noop_transfer_map;
   ctx->texture_map = noop_transfer_map;
   ctx->transfer_flush_region = noop_transfer_flush_region;
   ctx->buffer_unmap = noop_transfer_unmap;
   ctx->texture_unmap = noop_transfer_unmap;
   ctx->buffer_subdata = noop_buffer_subdata;
   ctx->texture_subdata = noop_texture_subdata;
   ctx->invalidate_resource = noop_invalidate_resource;
   ctx->set_context_param = noop_set_context_param;
   ctx->set_frontend_noop = noop_set_frontend_noop;
                     if (!(flags & PIPE_CONTEXT_PREFER_THREADED))
            struct pipe_context *tc =
      threaded_context_create(ctx,
                           &((struct noop_pipe_screen*)screen)->pool_transfers,
   noop_replace_buffer_storage,
         if (tc && tc != ctx)
               }
         /*
   * pipe_screen
   */
   static void noop_flush_frontbuffer(struct pipe_screen *_screen,
                           {
   }
      static const char *noop_get_vendor(struct pipe_screen* pscreen)
   {
         }
      static const char *noop_get_device_vendor(struct pipe_screen* pscreen)
   {
         }
      static const char *noop_get_name(struct pipe_screen* pscreen)
   {
         }
      static int noop_get_param(struct pipe_screen* pscreen, enum pipe_cap param)
   {
                  }
      static float noop_get_paramf(struct pipe_screen* pscreen,
         {
                  }
      static int noop_get_shader_param(struct pipe_screen* pscreen,
               {
                  }
      static int noop_get_compute_param(struct pipe_screen *pscreen,
                     {
                  }
      static bool noop_is_format_supported(struct pipe_screen* pscreen,
                                 {
               return screen->is_format_supported(screen, format, target, sample_count,
      }
      static uint64_t noop_get_timestamp(struct pipe_screen *pscreen)
   {
         }
      static void noop_destroy_screen(struct pipe_screen *screen)
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
            oscreen->destroy(oscreen);
   slab_destroy_parent(&noop_screen->pool_transfers);
      }
      static void noop_fence_reference(struct pipe_screen *screen,
               {
      if (pipe_reference((struct pipe_reference*)*ptr,
                     }
      static bool noop_fence_finish(struct pipe_screen *screen,
                     {
         }
      static void noop_query_memory_info(struct pipe_screen *pscreen,
         {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)pscreen;
               }
      static struct disk_cache *noop_get_disk_shader_cache(struct pipe_screen *pscreen)
   {
                  }
      static const void *noop_get_compiler_options(struct pipe_screen *pscreen,
               {
                  }
      static char *noop_finalize_nir(struct pipe_screen *pscreen, void *nir)
   {
                  }
      static bool noop_check_resource_capability(struct pipe_screen *screen,
               {
         }
      static void noop_create_fence_win32(struct pipe_screen *screen,
                           {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen *)screen;
   struct pipe_screen *oscreen = noop_screen->oscreen;
      }
      static void noop_set_max_shader_compiler_threads(struct pipe_screen *screen,
         {
   }
      static bool noop_is_parallel_shader_compilation_finished(struct pipe_screen *screen,
               {
         }
      static bool noop_is_dmabuf_modifier_supported(struct pipe_screen *screen,
               {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static unsigned int noop_get_dmabuf_modifier_planes(struct pipe_screen *screen,
               {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static void noop_get_driver_uuid(struct pipe_screen *screen, char *uuid)
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static void noop_get_device_uuid(struct pipe_screen *screen, char *uuid)
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static void noop_get_device_luid(struct pipe_screen *screen, char *luid)
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static uint32_t noop_get_device_node_mask(struct pipe_screen *screen)
   {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
               }
      static int noop_get_sparse_texture_virtual_page_size(struct pipe_screen *screen,
                                 {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
            return oscreen->get_sparse_texture_virtual_page_size(screen, target, multi_sample,
      }
      static void noop_query_dmabuf_modifiers(struct pipe_screen *screen,
                     {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen*)screen;
            oscreen->query_dmabuf_modifiers(oscreen, format, max, modifiers,
      }
      static struct pipe_vertex_state *
   noop_create_vertex_state(struct pipe_screen *screen,
                           struct pipe_vertex_buffer *buffer,
   {
               if (!state)
            util_init_pipe_vertex_state(screen, buffer, elements, num_elements, indexbuf,
            }
      static void noop_vertex_state_destroy(struct pipe_screen *screen,
         {
      pipe_vertex_buffer_unreference(&state->input.vbuffer);
   pipe_resource_reference(&state->input.indexbuf, NULL);
      }
      static void noop_set_fence_timeline_value(struct pipe_screen *screen,
               {
      struct noop_pipe_screen *noop_screen = (struct noop_pipe_screen *)screen;
   struct pipe_screen *oscreen = noop_screen->oscreen;
      }
      struct pipe_screen *noop_screen_create(struct pipe_screen *oscreen)
   {
      struct noop_pipe_screen *noop_screen;
            if (!debug_get_option_noop()) {
                  noop_screen = CALLOC_STRUCT(noop_pipe_screen);
   if (!noop_screen) {
         }
   noop_screen->oscreen = oscreen;
            screen->destroy = noop_destroy_screen;
   screen->get_name = noop_get_name;
   screen->get_vendor = noop_get_vendor;
   screen->get_device_vendor = noop_get_device_vendor;
   screen->get_param = noop_get_param;
   screen->get_shader_param = noop_get_shader_param;
   screen->get_compute_param = noop_get_compute_param;
   screen->get_paramf = noop_get_paramf;
   screen->is_format_supported = noop_is_format_supported;
   screen->context_create = noop_create_context;
   screen->resource_create = noop_resource_create;
   screen->resource_from_handle = noop_resource_from_handle;
   screen->resource_get_handle = noop_resource_get_handle;
   if (oscreen->resource_get_param)
         screen->resource_destroy = noop_resource_destroy;
   screen->flush_frontbuffer = noop_flush_frontbuffer;
   screen->get_timestamp = noop_get_timestamp;
   screen->fence_reference = noop_fence_reference;
   screen->fence_finish = noop_fence_finish;
   screen->query_memory_info = noop_query_memory_info;
   screen->get_disk_shader_cache = noop_get_disk_shader_cache;
   screen->get_compiler_options = noop_get_compiler_options;
   screen->finalize_nir = noop_finalize_nir;
   if (screen->create_fence_win32)
         screen->check_resource_capability = noop_check_resource_capability;
   screen->set_max_shader_compiler_threads = noop_set_max_shader_compiler_threads;
   screen->is_parallel_shader_compilation_finished = noop_is_parallel_shader_compilation_finished;
   screen->is_dmabuf_modifier_supported = noop_is_dmabuf_modifier_supported;
   screen->get_dmabuf_modifier_planes = noop_get_dmabuf_modifier_planes;
   screen->get_driver_uuid = noop_get_driver_uuid;
   screen->get_device_uuid = noop_get_device_uuid;
   screen->get_device_luid = noop_get_device_luid;
   screen->get_device_node_mask = noop_get_device_node_mask;
   screen->query_dmabuf_modifiers = noop_query_dmabuf_modifiers;
   screen->resource_create_with_modifiers = noop_resource_create_with_modifiers;
   screen->create_vertex_state = noop_create_vertex_state;
   screen->vertex_state_destroy = noop_vertex_state_destroy;
   if (oscreen->get_sparse_texture_virtual_page_size)
         if (oscreen->set_fence_timeline_value)
            slab_create_parent(&noop_screen->pool_transfers,
               }
