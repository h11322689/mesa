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
      #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/hash_table.h"
      #include "tr_dump.h"
   #include "tr_dump_defines.h"
   #include "tr_dump_state.h"
   #include "tr_texture.h"
   #include "tr_context.h"
   #include "tr_screen.h"
   #include "tr_public.h"
   #include "tr_util.h"
         static bool trace = false;
   static struct hash_table *trace_screens;
      static const char *
   trace_screen_get_name(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                                                            }
         static const char *
   trace_screen_get_vendor(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                                                            }
         static const char *
   trace_screen_get_device_vendor(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                                                            }
         static const void *
   trace_screen_get_compiler_options(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg_enum(pipe_shader_ir, ir);
                                          }
         static struct disk_cache *
   trace_screen_get_disk_shader_cache(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
                                                            }
         static int
   trace_screen_get_param(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                          }
         static int
   trace_screen_get_shader_param(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg_enum(pipe_shader_type, shader);
                                          }
         static float
   trace_screen_get_paramf(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                          }
         static int
   trace_screen_get_compute_param(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg_enum(pipe_shader_ir, ir_type);
   trace_dump_arg_enum(pipe_compute_cap, param);
                                          }
      static int
   trace_screen_get_video_param(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg_enum(pipe_video_profile, profile);
   trace_dump_arg_enum(pipe_video_entrypoint, entrypoint);
                                          }
      static bool
   trace_screen_is_format_supported(struct pipe_screen *_screen,
                                 {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(format, format);
   trace_dump_arg_enum(pipe_texture_target, target);
   trace_dump_arg(uint, sample_count);
   trace_dump_arg(uint, storage_sample_count);
            result = screen->is_format_supported(screen, format, target, sample_count,
                                 }
      static bool
   trace_screen_is_video_format_supported(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(format, format);
   trace_dump_arg_enum(pipe_video_profile, profile);
                                          }
      static void
   trace_screen_driver_thread_add_job(struct pipe_screen *_screen,
                           {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, data);
                        }
      static void
   trace_context_replace_buffer_storage(struct pipe_context *_pipe,
                                 {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
   trace_dump_arg(ptr, dst);
   trace_dump_arg(ptr, src);
   trace_dump_arg(uint, num_rebinds);
   trace_dump_arg(uint, rebind_mask);
   trace_dump_arg(uint, delete_buffer_id);
               }
      static struct pipe_fence_handle *
   trace_context_create_fence(struct pipe_context *_pipe, struct tc_unflushed_batch_token *token)
   {
      struct trace_context *tr_ctx = trace_context(_pipe);
                     trace_dump_arg(ptr, pipe);
            struct pipe_fence_handle *ret = tr_ctx->create_fence(pipe, token);
   trace_dump_ret(ptr, ret);
               }
      static bool
   trace_context_is_resource_busy(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, resource);
                                          }
      struct pipe_context *
   trace_context_create_threaded(struct pipe_screen *screen, struct pipe_context *pipe,
               {
      if (!trace_screens)
            struct hash_entry *he = _mesa_hash_table_search(trace_screens, screen);
   if (!he)
                  if (tr_scr->trace_tc)
            struct pipe_context *ctx = trace_context_create(tr_scr, pipe);
   if (!ctx)
            struct trace_context *tr_ctx = trace_context(ctx);
   tr_ctx->replace_buffer_storage = *replace_buffer;
   tr_ctx->create_fence = options->create_fence;
   tr_scr->is_resource_busy = options->is_resource_busy;
   tr_ctx->threaded = true;
   *replace_buffer = trace_context_replace_buffer_storage;
   if (options->create_fence)
         if (options->is_resource_busy)
            }
      static struct pipe_context *
   trace_screen_context_create(struct pipe_screen *_screen, void *priv,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                              trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, priv);
                              if (result && (tr_scr->trace_tc || result->draw_vbo != tc_draw_vbo))
               }
         static void
   trace_screen_flush_frontbuffer(struct pipe_screen *_screen,
                                 {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, level);
   trace_dump_arg(uint, layer);
   /* XXX: hide, as there is nothing we can do with this
   trace_dump_arg(ptr, context_private);
                        }
         static void
   trace_screen_get_driver_uuid(struct pipe_screen *_screen, char *uuid)
   {
               trace_dump_call_begin("pipe_screen", "get_driver_uuid");
                     trace_dump_ret(string, uuid);
      }
      static void
   trace_screen_get_device_uuid(struct pipe_screen *_screen, char *uuid)
   {
               trace_dump_call_begin("pipe_screen", "get_device_uuid");
                     trace_dump_ret(string, uuid);
      }
      static void
   trace_screen_get_device_luid(struct pipe_screen *_screen, char *luid)
   {
               trace_dump_call_begin("pipe_screen", "get_device_luid");
                     trace_dump_ret(string, luid);
      }
      static uint32_t
   trace_screen_get_device_node_mask(struct pipe_screen *_screen)
   {
      struct pipe_screen *screen = trace_screen(_screen)->screen;
            trace_dump_call_begin("pipe_screen", "get_device_node_mask");
                     trace_dump_ret(uint, result);
               }
         /********************************************************************
   * texture
   */
      static void *
   trace_screen_map_memory(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                          }
      static void
   trace_screen_unmap_memory(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
                           }
      static struct pipe_memory_allocation *
   trace_screen_allocate_memory(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                          }
      static struct pipe_memory_allocation *
   trace_screen_allocate_memory_fd(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(uint, size);
                                          }
      static void
   trace_screen_free_memory(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
                           }
      static void
   trace_screen_free_memory_fd(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
                           }
      static bool
   trace_screen_resource_bind_backing(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(ptr, pmem);
                                          }
      static struct pipe_resource *
   trace_screen_resource_create_unbacked(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                     trace_dump_ret_begin();
   trace_dump_uint(*size_required);
   trace_dump_ret_end();
                     if (result)
            }
      static struct pipe_resource *
   trace_screen_resource_create(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                       if (result)
            }
      static struct pipe_resource *
   trace_screen_resource_create_drawable(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(resource_template, templat);
                                       if (result)
            }
      static struct pipe_resource *
   trace_screen_resource_create_with_modifiers(struct pipe_screen *_screen, const struct pipe_resource *templat,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(resource_template, templat);
                                       if (result)
            }
      static struct pipe_resource *
   trace_screen_resource_from_handle(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_screen = trace_screen(_screen);
   struct pipe_screen *screen = tr_screen->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(resource_template, templ);
   trace_dump_arg(winsys_handle, handle);
                                       if (result)
            }
      static bool
   trace_screen_check_resource_capability(struct pipe_screen *_screen,
               {
                  }
      static bool
   trace_screen_resource_get_handle(struct pipe_screen *_screen,
                           {
      struct trace_screen *tr_screen = trace_screen(_screen);
   struct pipe_context *pipe = _pipe ? trace_get_possibly_threaded_context(_pipe) : NULL;
   struct pipe_screen *screen = tr_screen->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, resource);
                     trace_dump_arg(winsys_handle, handle);
                        }
      static bool
   trace_screen_resource_get_param(struct pipe_screen *_screen,
                                 struct pipe_context *_pipe,
   struct pipe_resource *resource,
   unsigned plane,
   {
      struct trace_screen *tr_screen = trace_screen(_screen);
   struct pipe_context *pipe = _pipe ? trace_get_possibly_threaded_context(_pipe) : NULL;
   struct pipe_screen *screen = tr_screen->screen;
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, resource);
   trace_dump_arg(uint, plane);
   trace_dump_arg(uint, layer);
   trace_dump_arg(uint, level);
   trace_dump_arg_enum(pipe_resource_param, param);
            result = screen->resource_get_param(screen, pipe,
                  trace_dump_arg(uint, *value);
                        }
      static void
   trace_screen_resource_get_info(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_screen = trace_screen(_screen);
            trace_dump_call_begin("pipe_screen", "resource_get_info");
   trace_dump_arg(ptr, screen);
                     trace_dump_arg(uint, *stride);
               }
      static struct pipe_resource *
   trace_screen_resource_from_memobj(struct pipe_screen *_screen,
                     {
               trace_dump_call_begin("pipe_screen", "resource_from_memobj");
   trace_dump_arg(ptr, screen);
   trace_dump_arg(resource_template, templ);
   trace_dump_arg(ptr, memobj);
            struct pipe_resource *res =
            if (!res)
                  trace_dump_ret(ptr, res);
   trace_dump_call_end();
      }
      static void
   trace_screen_resource_changed(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
            if (screen->resource_changed)
               }
      static void
   trace_screen_resource_destroy(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
            /* Don't trace this, because due to the lack of pipe_resource wrapping,
   * we can get this call from inside of driver calls, which would try
   * to lock an already-locked mutex.
   */
      }
         /********************************************************************
   * fence
   */
         static void
   trace_screen_fence_reference(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
            assert(pdst);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, dst);
                        }
         static int
   trace_screen_fence_get_fd(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
                     trace_dump_arg(ptr, screen);
                                          }
      static void
   trace_screen_create_fence_win32(struct pipe_screen *_screen,
                           {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   if (fence)
         trace_dump_arg(ptr, handle);
   trace_dump_arg(ptr, name);
                        }
         static bool
   trace_screen_fence_finish(struct pipe_screen *_screen,
                     {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_context *ctx = _ctx ? trace_get_possibly_threaded_context(_ctx) : NULL;
                                 trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, ctx);
   trace_dump_arg(ptr, fence);
                                 }
         /********************************************************************
   * memobj
   */
      static struct pipe_memory_object *
   trace_screen_memobj_create_from_handle(struct pipe_screen *_screen,
               {
               trace_dump_call_begin("pipe_screen", "memobj_create_from_handle");
   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, handle);
            struct pipe_memory_object *res =
            trace_dump_ret(ptr, res);
               }
      static void
   trace_screen_memobj_destroy(struct pipe_screen *_screen,
         {
               trace_dump_call_begin("pipe_screen", "memobj_destroy");
   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, memobj);
               }
         /********************************************************************
   * screen
   */
      static uint64_t
   trace_screen_get_timestamp(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
   struct pipe_screen *screen = tr_scr->screen;
            trace_dump_call_begin("pipe_screen", "get_timestamp");
                     trace_dump_ret(uint, result);
               }
      static char *
   trace_screen_finalize_nir(struct pipe_screen *_screen, void *nir)
   {
                  }
      static void
   trace_screen_destroy(struct pipe_screen *_screen)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
            trace_dump_call_begin("pipe_screen", "destroy");
   trace_dump_arg(ptr, screen);
            if (trace_screens) {
      struct hash_entry *he = _mesa_hash_table_search(trace_screens, screen);
   if (he) {
      _mesa_hash_table_remove(trace_screens, he);
   if (!_mesa_hash_table_num_entries(trace_screens)) {
      _mesa_hash_table_destroy(trace_screens, NULL);
                                 }
      static void
   trace_screen_query_memory_info(struct pipe_screen *_screen, struct pipe_memory_info *info)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
                                                   }
      static void
   trace_screen_query_dmabuf_modifiers(struct pipe_screen *_screen, enum pipe_format format, int max, uint64_t *modifiers, unsigned int *external_only, int *count)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(format, format);
                     if (max)
         else
         trace_dump_arg_array(uint, external_only, max);
   trace_dump_ret_begin();
   trace_dump_uint(*count);
               }
      static bool
   trace_screen_is_compute_copy_faster(struct pipe_screen *_screen, enum pipe_format src_format,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(format, src_format);
   trace_dump_arg(format, dst_format);
   trace_dump_arg(uint, width);
   trace_dump_arg(uint, height);
   trace_dump_arg(uint, depth);
                              trace_dump_call_end();
      }
      static bool
   trace_screen_is_dmabuf_modifier_supported(struct pipe_screen *_screen, uint64_t modifier, enum pipe_format format, bool *external_only)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(uint, modifier);
                     trace_dump_arg_begin("external_only");
   trace_dump_bool(external_only ? *external_only : false);
                     trace_dump_call_end();
      }
      static unsigned int
   trace_screen_get_dmabuf_modifier_planes(struct pipe_screen *_screen, uint64_t modifier, enum pipe_format format)
   {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(uint, modifier);
                              trace_dump_call_end();
      }
      static int
   trace_screen_get_sparse_texture_virtual_page_size(struct pipe_screen *_screen,
                                 {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg_enum(pipe_texture_target, target);
   trace_dump_arg(format, format);
   trace_dump_arg(uint, offset);
            int ret = screen->get_sparse_texture_virtual_page_size(screen, target, multi_sample,
            if (x)
         else
         if (y)
         else
         if (z)
         else
                     trace_dump_call_end();
      }
      static struct pipe_vertex_state *
   trace_screen_create_vertex_state(struct pipe_screen *_screen,
                                 {
      struct trace_screen *tr_scr = trace_screen(_screen);
                     trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, buffer->buffer.resource);
   trace_dump_arg(vertex_buffer, buffer);
   trace_dump_arg_begin("elements");
   trace_dump_struct_array(vertex_element, elements, num_elements);
   trace_dump_arg_end();
   trace_dump_arg(uint, num_elements);
   trace_dump_arg(ptr, indexbuf);
            struct pipe_vertex_state *vstate =
      screen->create_vertex_state(screen, buffer, elements, num_elements,
      trace_dump_ret(ptr, vstate);
   trace_dump_call_end();
      }
      static void trace_screen_vertex_state_destroy(struct pipe_screen *_screen,
         {
      struct trace_screen *tr_scr = trace_screen(_screen);
            trace_dump_call_begin("pipe_screen", "vertex_state_destroy");
   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, state);
               }
      static void trace_screen_set_fence_timeline_value(struct pipe_screen *_screen,
               {
      struct trace_screen *tr_scr = trace_screen(_screen);
            trace_dump_call_begin("pipe_screen", "set_fence_timeline_value");
   trace_dump_arg(ptr, screen);
   trace_dump_arg(ptr, fence);
   trace_dump_arg(uint, value);
               }
      bool
   trace_enabled(void)
   {
               if (!firstrun)
                  if(trace_dump_trace_begin()) {
      trace_dumping_start();
                  }
      struct pipe_screen *
   trace_screen_create(struct pipe_screen *screen)
   {
               /* if zink+lavapipe is enabled, ensure that only one driver is traced */
   const char *driver = debug_get_option("MESA_LOADER_DRIVER_OVERRIDE", NULL);
   if (driver && !strcmp(driver, "zink")) {
      /* the user wants zink: check whether they want to trace zink or lavapipe */
   bool trace_lavapipe = debug_get_bool_option("ZINK_TRACE_LAVAPIPE", false);
   if (!strncmp(screen->get_name(screen), "zink", 4)) {
      /* this is the zink screen: only trace if lavapipe tracing is disabled */
   if (trace_lavapipe)
      } else {
      /* this is the llvmpipe screen: only trace if lavapipe tracing is enabled */
   if (!trace_lavapipe)
                  if (!trace_enabled())
                     tr_scr = CALLOC_STRUCT(trace_screen);
   if (!tr_scr)
         #define SCR_INIT(_member) \
               tr_scr->base.destroy = trace_screen_destroy;
   tr_scr->base.get_name = trace_screen_get_name;
   tr_scr->base.get_vendor = trace_screen_get_vendor;
   tr_scr->base.get_device_vendor = trace_screen_get_device_vendor;
   SCR_INIT(get_compiler_options);
   SCR_INIT(get_disk_shader_cache);
   tr_scr->base.get_param = trace_screen_get_param;
   tr_scr->base.get_shader_param = trace_screen_get_shader_param;
   tr_scr->base.get_paramf = trace_screen_get_paramf;
   tr_scr->base.get_compute_param = trace_screen_get_compute_param;
   SCR_INIT(get_video_param);
   tr_scr->base.is_format_supported = trace_screen_is_format_supported;
   SCR_INIT(is_video_format_supported);
   assert(screen->context_create);
   tr_scr->base.context_create = trace_screen_context_create;
   tr_scr->base.resource_create = trace_screen_resource_create;
   SCR_INIT(resource_create_with_modifiers);
   tr_scr->base.resource_create_unbacked = trace_screen_resource_create_unbacked;
   SCR_INIT(resource_create_drawable);
   tr_scr->base.resource_bind_backing = trace_screen_resource_bind_backing;
   tr_scr->base.resource_from_handle = trace_screen_resource_from_handle;
   tr_scr->base.allocate_memory = trace_screen_allocate_memory;
   SCR_INIT(allocate_memory_fd);
   tr_scr->base.free_memory = trace_screen_free_memory;
   SCR_INIT(free_memory_fd);
   tr_scr->base.map_memory = trace_screen_map_memory;
   tr_scr->base.unmap_memory = trace_screen_unmap_memory;
   SCR_INIT(query_memory_info);
   SCR_INIT(query_dmabuf_modifiers);
   SCR_INIT(is_compute_copy_faster);
   SCR_INIT(is_dmabuf_modifier_supported);
   SCR_INIT(get_dmabuf_modifier_planes);
   SCR_INIT(check_resource_capability);
   tr_scr->base.resource_get_handle = trace_screen_resource_get_handle;
   SCR_INIT(resource_get_param);
   SCR_INIT(resource_get_info);
   SCR_INIT(resource_from_memobj);
   SCR_INIT(resource_changed);
   tr_scr->base.resource_destroy = trace_screen_resource_destroy;
   tr_scr->base.fence_reference = trace_screen_fence_reference;
   SCR_INIT(fence_get_fd);
   SCR_INIT(create_fence_win32);
   tr_scr->base.fence_finish = trace_screen_fence_finish;
   SCR_INIT(memobj_create_from_handle);
   SCR_INIT(memobj_destroy);
   tr_scr->base.flush_frontbuffer = trace_screen_flush_frontbuffer;
   tr_scr->base.get_timestamp = trace_screen_get_timestamp;
   SCR_INIT(get_driver_uuid);
   SCR_INIT(get_device_uuid);
   SCR_INIT(get_device_luid);
   SCR_INIT(get_device_node_mask);
   SCR_INIT(finalize_nir);
   SCR_INIT(create_vertex_state);
   SCR_INIT(vertex_state_destroy);
   tr_scr->base.transfer_helper = screen->transfer_helper;
   SCR_INIT(get_sparse_texture_virtual_page_size);
   SCR_INIT(set_fence_timeline_value);
                     trace_dump_ret(ptr, screen);
            if (!trace_screens)
                                 error2:
      trace_dump_ret(ptr, screen);
      error1:
         }
         struct trace_screen *
   trace_screen(struct pipe_screen *screen)
   {
      assert(screen);
   assert(screen->destroy == trace_screen_destroy);
      }
      struct pipe_screen *
   trace_screen_unwrap(struct pipe_screen *_screen)
   {
      if (_screen->destroy != trace_screen_destroy)
         struct trace_screen *tr_scr = trace_screen(_screen);
      }
