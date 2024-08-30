   /*
   * Copyright © 2016 Red Hat
   * based on intel anv code:
   * Copyright © 2015 Intel Corporation
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
      #include "radv_meta.h"
      #include "vk_common_entrypoints.h"
   #include "vk_pipeline_cache.h"
   #include "vk_util.h"
      #include <fcntl.h>
   #include <limits.h>
   #ifndef _WIN32
   #include <pwd.h>
   #endif
   #include <sys/stat.h>
      static void
   radv_suspend_queries(struct radv_meta_saved_state *state, struct radv_cmd_buffer *cmd_buffer)
   {
               if (num_pipeline_stat_queries > 0) {
      cmd_buffer->state.flush_bits &= ~RADV_CMD_FLAG_START_PIPELINE_STATS;
               /* Pipeline statistics queries. */
   if (cmd_buffer->state.active_pipeline_queries > 0) {
      state->active_pipeline_gds_queries = cmd_buffer->state.active_pipeline_gds_queries;
   cmd_buffer->state.active_pipeline_gds_queries = 0;
               /* Occlusion queries. */
   if (cmd_buffer->state.active_occlusion_queries) {
      state->active_occlusion_queries = cmd_buffer->state.active_occlusion_queries;
   cmd_buffer->state.active_occlusion_queries = 0;
               /* Primitives generated queries (legacy). */
   if (cmd_buffer->state.active_prims_gen_queries) {
      cmd_buffer->state.suspend_streamout = true;
               /* Primitives generated queries (NGG). */
   if (cmd_buffer->state.active_prims_gen_gds_queries) {
      state->active_prims_gen_gds_queries = cmd_buffer->state.active_prims_gen_gds_queries;
   cmd_buffer->state.active_prims_gen_gds_queries = 0;
               /* Transform feedback queries (NGG). */
   if (cmd_buffer->state.active_prims_xfb_gds_queries) {
      state->active_prims_xfb_gds_queries = cmd_buffer->state.active_prims_xfb_gds_queries;
   cmd_buffer->state.active_prims_xfb_gds_queries = 0;
         }
      static void
   radv_resume_queries(const struct radv_meta_saved_state *state, struct radv_cmd_buffer *cmd_buffer)
   {
               if (num_pipeline_stat_queries > 0) {
      cmd_buffer->state.flush_bits &= ~RADV_CMD_FLAG_STOP_PIPELINE_STATS;
               /* Pipeline statistics queries. */
   if (cmd_buffer->state.active_pipeline_queries > 0) {
      cmd_buffer->state.active_pipeline_gds_queries = state->active_pipeline_gds_queries;
               /* Occlusion queries. */
   if (state->active_occlusion_queries) {
      cmd_buffer->state.active_occlusion_queries = state->active_occlusion_queries;
               /* Primitives generated queries (legacy). */
   if (cmd_buffer->state.active_prims_gen_queries) {
      cmd_buffer->state.suspend_streamout = false;
               /* Primitives generated queries (NGG). */
   if (state->active_prims_gen_gds_queries) {
      cmd_buffer->state.active_prims_gen_gds_queries = state->active_prims_gen_gds_queries;
               /* Transform feedback queries (NGG). */
   if (state->active_prims_xfb_gds_queries) {
      cmd_buffer->state.active_prims_xfb_gds_queries = state->active_prims_xfb_gds_queries;
         }
      void
   radv_meta_save(struct radv_meta_saved_state *state, struct radv_cmd_buffer *cmd_buffer, uint32_t flags)
   {
      VkPipelineBindPoint bind_point =
                           state->flags = flags;
   state->active_occlusion_queries = 0;
   state->active_prims_gen_gds_queries = 0;
            if (state->flags & RADV_META_SAVE_GRAPHICS_PIPELINE) {
                        /* Save all dynamic states. */
               if (state->flags & RADV_META_SAVE_COMPUTE_PIPELINE) {
                           if (state->flags & RADV_META_SAVE_DESCRIPTORS) {
      state->old_descriptor_set0 = descriptors_state->sets[0];
   if (!(descriptors_state->valid & 1))
               if (state->flags & RADV_META_SAVE_CONSTANTS) {
                  if (state->flags & RADV_META_SAVE_RENDER) {
      state->render = cmd_buffer->state.render;
               if (state->flags & RADV_META_SUSPEND_PREDICATING) {
      state->predicating = cmd_buffer->state.predicating;
                  }
      void
   radv_meta_restore(const struct radv_meta_saved_state *state, struct radv_cmd_buffer *cmd_buffer)
   {
      VkPipelineBindPoint bind_point = state->flags & RADV_META_SAVE_GRAPHICS_PIPELINE ? VK_PIPELINE_BIND_POINT_GRAPHICS
            if (state->flags & RADV_META_SAVE_GRAPHICS_PIPELINE) {
      if (state->old_graphics_pipeline) {
      radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_GRAPHICS,
      } else {
                  /* Restore all dynamic states. */
   cmd_buffer->state.dynamic = state->dynamic;
            /* Re-emit the guardband state because meta operations changed dynamic states. */
               if (state->flags & RADV_META_SAVE_COMPUTE_PIPELINE) {
      if (state->old_compute_pipeline) {
      radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
      } else {
                     if (state->flags & RADV_META_SAVE_DESCRIPTORS) {
                  if (state->flags & RADV_META_SAVE_CONSTANTS) {
               if (state->flags & RADV_META_SAVE_GRAPHICS_PIPELINE)
            radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), VK_NULL_HANDLE, stages, 0, MAX_PUSH_CONSTANTS_SIZE,
               if (state->flags & RADV_META_SAVE_RENDER) {
      cmd_buffer->state.render = state->render;
               if (state->flags & RADV_META_SUSPEND_PREDICATING)
               }
      VkImageViewType
   radv_meta_get_view_type(const struct radv_image *image)
   {
      switch (image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
         case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
         default:
            }
      /**
   * When creating a destination VkImageView, this function provides the needed
   * VkImageViewCreateInfo::subresourceRange::baseArrayLayer.
   */
   uint32_t
   radv_meta_get_iview_layer(const struct radv_image *dst_image, const VkImageSubresourceLayers *dst_subresource,
         {
      switch (dst_image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
   case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
      /* HACK: Vulkan does not allow attaching a 3D image to a framebuffer,
   * but meta does it anyway. When doing so, we translate the
   * destination's z offset into an array offset.
   */
      default:
      assert(!"bad VkImageType");
         }
      static VKAPI_ATTR void *VKAPI_CALL
   meta_alloc(void *_device, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
   {
      struct radv_device *device = _device;
   return device->vk.alloc.pfnAllocation(device->vk.alloc.pUserData, size, alignment,
      }
      static VKAPI_ATTR void *VKAPI_CALL
   meta_realloc(void *_device, void *original, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
   {
      struct radv_device *device = _device;
   return device->vk.alloc.pfnReallocation(device->vk.alloc.pUserData, original, size, alignment,
      }
      static VKAPI_ATTR void VKAPI_CALL
   meta_free(void *_device, void *data)
   {
      struct radv_device *device = _device;
      }
      #ifndef _WIN32
   static bool
   radv_builtin_cache_path(char *path)
   {
      char *xdg_cache_home = secure_getenv("XDG_CACHE_HOME");
   const char *suffix = "/radv_builtin_shaders";
   const char *suffix2 = "/.cache/radv_builtin_shaders";
   struct passwd pwd, *result;
   char path2[PATH_MAX + 1]; /* PATH_MAX is not a real max,but suffices here. */
            if (xdg_cache_home) {
      ret = snprintf(path, PATH_MAX + 1, "%s%s%zd", xdg_cache_home, suffix, sizeof(void *) * 8);
               getpwuid_r(getuid(), &pwd, path2, PATH_MAX - strlen(suffix2), &result);
   if (!result)
            strcpy(path, pwd.pw_dir);
   strcat(path, "/.cache");
   if (mkdir(path, 0755) && errno != EEXIST)
            ret = snprintf(path, PATH_MAX + 1, "%s%s%zd", pwd.pw_dir, suffix2, sizeof(void *) * 8);
      }
   #endif
      static uint32_t
   num_cache_entries(VkPipelineCache cache)
   {
      struct set *s = vk_pipeline_cache_from_handle(cache)->object_cache;
   if (!s)
            }
      static bool
   radv_load_meta_pipeline(struct radv_device *device)
   {
   #ifdef _WIN32
         #else
      char path[PATH_MAX + 1];
   struct stat st;
   void *data = NULL;
   bool ret = false;
   int fd = -1;
            VkPipelineCacheCreateInfo create_info = {
                  struct vk_pipeline_cache_create_info info = {
      .pCreateInfo = &create_info,
               if (!radv_builtin_cache_path(path))
            fd = open(path, O_RDONLY);
   if (fd < 0)
         if (fstat(fd, &st))
         data = malloc(st.st_size);
   if (!data)
         if (read(fd, data, st.st_size) == -1)
            create_info.initialDataSize = st.st_size;
         fail:
               if (cache) {
      device->meta_state.cache = vk_pipeline_cache_to_handle(cache);
   device->meta_state.initial_cache_entries = num_cache_entries(device->meta_state.cache);
               free(data);
   if (fd >= 0)
            #endif
   }
      static void
   radv_store_meta_pipeline(struct radv_device *device)
   {
   #ifndef _WIN32
      char path[PATH_MAX + 1], path2[PATH_MAX + 7];
   size_t size;
            if (device->meta_state.cache == VK_NULL_HANDLE)
            /* Skip serialization if no entries were added. */
   if (num_cache_entries(device->meta_state.cache) <= device->meta_state.initial_cache_entries)
            if (vk_common_GetPipelineCacheData(radv_device_to_handle(device), device->meta_state.cache, &size, NULL))
            if (!radv_builtin_cache_path(path))
            strcpy(path2, path);
   strcat(path2, "XXXXXX");
   int fd = mkstemp(path2); // open(path, O_WRONLY | O_CREAT, 0600);
   if (fd < 0)
         data = malloc(size);
   if (!data)
            if (vk_common_GetPipelineCacheData(radv_device_to_handle(device), device->meta_state.cache, &size, data))
         if (write(fd, data, size) == -1)
               fail:
      free(data);
   close(fd);
      #endif
   }
      VkResult
   radv_device_init_meta(struct radv_device *device)
   {
                        device->meta_state.alloc = (VkAllocationCallbacks){
      .pUserData = device,
   .pfnAllocation = meta_alloc,
   .pfnReallocation = meta_realloc,
               bool loaded_cache = radv_load_meta_pipeline(device);
                     result = radv_device_init_meta_clear_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_resolve_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_blit_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_blit2d_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_bufimage_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_depth_decomp_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_buffer_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_query_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_fast_clear_flush_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_resolve_compute_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_resolve_fragment_state(device, on_demand);
   if (result != VK_SUCCESS)
            if (device->physical_device->use_fmask) {
      result = radv_device_init_meta_fmask_expand_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_fmask_copy_state(device, on_demand);
   if (result != VK_SUCCESS)
               result = radv_device_init_meta_etc_decode_state(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_astc_decode_state(device, on_demand);
   if (result != VK_SUCCESS)
            if (device->uses_device_generated_commands) {
      result = radv_device_init_dgc_prepare_state(device);
   if (result != VK_SUCCESS)
               if (device->vk.enabled_extensions.KHR_acceleration_structure) {
      if (device->vk.enabled_features.nullDescriptor) {
      result = radv_device_init_null_accel_struct(device);
   if (result != VK_SUCCESS)
               /* FIXME: Acceleration structure builds hang when the build shaders are compiled with LLVM.
   * Work around it by forcing ACO for now.
   */
   bool use_llvm = device->physical_device->use_llvm;
   if (loaded_cache || use_llvm) {
      device->physical_device->use_llvm = false;
                  if (result != VK_SUCCESS)
                        fail_accel_struct:
         fail_dgc:
         fail_astc_decode:
         fail_etc_decode:
         fail_fmask_copy:
         fail_fmask_expand:
         fail_resolve_fragment:
         fail_resolve_compute:
         fail_fast_clear:
         fail_query:
         fail_buffer:
         fail_depth_decomp:
         fail_bufimage:
         fail_blit2d:
         fail_blit:
         fail_resolve:
         fail_clear:
               mtx_destroy(&device->meta_state.mtx);
   vk_common_DestroyPipelineCache(radv_device_to_handle(device), device->meta_state.cache, NULL);
      }
      void
   radv_device_finish_meta(struct radv_device *device)
   {
      radv_device_finish_dgc_prepare_state(device);
   radv_device_finish_meta_etc_decode_state(device);
   radv_device_finish_meta_astc_decode_state(device);
   radv_device_finish_accel_struct_build_state(device);
   radv_device_finish_meta_clear_state(device);
   radv_device_finish_meta_resolve_state(device);
   radv_device_finish_meta_blit_state(device);
   radv_device_finish_meta_blit2d_state(device);
   radv_device_finish_meta_bufimage_state(device);
   radv_device_finish_meta_depth_decomp_state(device);
   radv_device_finish_meta_query_state(device);
   radv_device_finish_meta_buffer_state(device);
   radv_device_finish_meta_fast_clear_flush_state(device);
   radv_device_finish_meta_resolve_compute_state(device);
   radv_device_finish_meta_resolve_fragment_state(device);
   radv_device_finish_meta_fmask_expand_state(device);
   radv_device_finish_meta_dcc_retile_state(device);
   radv_device_finish_meta_copy_vrs_htile_state(device);
            radv_store_meta_pipeline(device);
   vk_common_DestroyPipelineCache(radv_device_to_handle(device), device->meta_state.cache, NULL);
      }
      nir_builder PRINTFLIKE(3, 4)
         {
      nir_builder b = nir_builder_init_simple_shader(stage, NULL, NULL);
   if (name) {
      va_list args;
   va_start(args, name);
   b.shader->info.name = ralloc_vasprintf(b.shader, name, args);
                           }
      /* vertex shader that generates vertices */
   nir_shader *
   radv_meta_build_nir_vs_generate_vertices(struct radv_device *dev)
   {
                                          v_position = nir_variable_create(b.shader, nir_var_shader_out, vec4, "gl_Position");
                        }
      nir_shader *
   radv_meta_build_nir_fs_noop(struct radv_device *dev)
   {
         }
      void
   radv_meta_build_resolve_shader_core(struct radv_device *device, nir_builder *b, bool is_integer, int samples,
         {
      nir_deref_instr *input_img_deref = nir_build_deref_var(b, input_img);
            if (is_integer || samples <= 1) {
      nir_store_var(b, color, sample0, 0xf);
               if (device->physical_device->use_fmask) {
      nir_def *all_same = nir_samples_identical_deref(b, input_img_deref, img_coord);
               nir_def *accum = sample0;
   for (int i = 1; i < samples; i++) {
      nir_def *sample = nir_txf_ms_deref(b, input_img_deref, img_coord, nir_imm_int(b, i));
               accum = nir_fdiv_imm(b, accum, samples);
            if (device->physical_device->use_fmask) {
      nir_push_else(b, NULL);
   nir_store_var(b, color, sample0, 0xf);
         }
      nir_def *
   radv_meta_load_descriptor(nir_builder *b, unsigned desc_set, unsigned binding)
   {
      nir_def *rsrc = nir_vulkan_resource_index(b, 3, 32, nir_imm_int(b, 0), .desc_set = desc_set, .binding = binding);
      }
      nir_def *
   get_global_ids(nir_builder *b, unsigned num_components)
   {
               nir_def *local_ids = nir_channels(b, nir_load_local_invocation_id(b), mask);
   nir_def *block_ids = nir_channels(b, nir_load_workgroup_id(b), mask);
   nir_def *block_size =
      nir_channels(b,
                        }
      void
   radv_break_on_count(nir_builder *b, nir_variable *var, nir_def *count)
   {
               nir_push_if(b, nir_uge(b, counter, count));
   nir_jump(b, nir_jump_break);
            counter = nir_iadd_imm(b, counter, 1);
      }
