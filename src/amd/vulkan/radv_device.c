   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
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
      #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
      #ifdef __FreeBSD__
   #include <sys/types.h>
   #endif
   #ifdef MAJOR_IN_MKDEV
   #include <sys/mkdev.h>
   #endif
   #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
      #ifdef __linux__
   #include <sys/inotify.h>
   #endif
      #include "util/disk_cache.h"
   #include "util/u_debug.h"
   #include "radv_cs.h"
   #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "vk_common_entrypoints.h"
   #include "vk_pipeline_cache.h"
   #include "vk_semaphore.h"
   #include "vk_util.h"
   #ifdef _WIN32
   typedef void *drmDevicePtr;
   #include <io.h>
   #else
   #include <amdgpu.h>
   #include <xf86drm.h>
   #include "drm-uapi/amdgpu_drm.h"
   #include "winsys/amdgpu/radv_amdgpu_winsys_public.h"
   #endif
   #include "util/build_id.h"
   #include "util/driconf.h"
   #include "util/mesa-sha1.h"
   #include "util/os_time.h"
   #include "util/timespec.h"
   #include "util/u_atomic.h"
   #include "util/u_process.h"
   #include "vulkan/vk_icd.h"
   #include "winsys/null/radv_null_winsys_public.h"
   #include "git_sha1.h"
   #include "sid.h"
   #include "vk_common_entrypoints.h"
   #include "vk_format.h"
   #include "vk_sync.h"
   #include "vk_sync_dummy.h"
      #ifdef LLVM_AVAILABLE
   #include "ac_llvm_util.h"
   #endif
      static bool
   radv_spm_trace_enabled(struct radv_instance *instance)
   {
      return (instance->vk.trace_mode & RADV_TRACE_MODE_RGP) &&
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetMemoryHostPointerPropertiesEXT(VkDevice _device, VkExternalMemoryHandleTypeFlagBits handleType,
               {
               switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT: {
      const struct radv_physical_device *physical_device = device->physical_device;
   uint32_t memoryTypeBits = 0;
   for (int i = 0; i < physical_device->memory_properties.memoryTypeCount; i++) {
      if (physical_device->memory_domains[i] == RADEON_DOMAIN_GTT &&
      !(physical_device->memory_flags[i] & RADEON_FLAG_GTT_WC)) {
   memoryTypeBits = (1 << i);
         }
   pMemoryHostPointerProperties->memoryTypeBits = memoryTypeBits;
      }
   default:
            }
      static VkResult
   radv_device_init_border_color(struct radv_device *device)
   {
               result =
      device->ws->buffer_create(device->ws, RADV_BORDER_COLOR_BUFFER_SIZE, 4096, RADEON_DOMAIN_VRAM,
               if (result != VK_SUCCESS)
                     result = device->ws->buffer_make_resident(device->ws, device->border_color_data.bo, true);
   if (result != VK_SUCCESS)
            device->border_color_data.colors_gpu_ptr = device->ws->buffer_map(device->border_color_data.bo);
   if (!device->border_color_data.colors_gpu_ptr)
                     }
      static void
   radv_device_finish_border_color(struct radv_device *device)
   {
      if (device->border_color_data.bo) {
      radv_rmv_log_border_color_palette_destroy(device, device->border_color_data.bo);
   device->ws->buffer_make_resident(device->ws, device->border_color_data.bo, false);
                  }
      static VkResult
   radv_device_init_vs_prologs(struct radv_device *device)
   {
      u_rwlock_init(&device->vs_prologs_lock);
   device->vs_prologs = _mesa_hash_table_create(NULL, &radv_hash_vs_prolog, &radv_cmp_vs_prolog);
   if (!device->vs_prologs)
            /* don't pre-compile prologs if we want to print them */
   if (device->instance->debug_flags & RADV_DEBUG_DUMP_PROLOGS)
            struct radv_vs_input_state state;
   state.nontrivial_divisors = 0;
   memset(state.offsets, 0, sizeof(state.offsets));
   state.alpha_adjust_lo = 0;
   state.alpha_adjust_hi = 0;
            struct radv_vs_prolog_key key;
   key.state = &state;
   key.misaligned_mask = 0;
   key.as_ls = false;
   key.is_ngg = device->physical_device->use_ngg;
   key.next_stage = MESA_SHADER_VERTEX;
            for (unsigned i = 1; i <= MAX_VERTEX_ATTRIBS; i++) {
      state.attribute_mask = BITFIELD_MASK(i);
                     device->simple_vs_prologs[i - 1] = radv_create_vs_prolog(device, &key);
   if (!device->simple_vs_prologs[i - 1])
               unsigned idx = 0;
   for (unsigned num_attributes = 1; num_attributes <= 16; num_attributes++) {
               for (unsigned i = 0; i < num_attributes; i++)
            for (unsigned count = 1; count <= num_attributes; count++) {
                                 struct radv_shader_part *prolog = radv_create_vs_prolog(device, &key);
                  assert(idx == radv_instance_rate_prolog_index(num_attributes, state.instance_rate_inputs));
            }
               }
      static void
   radv_device_finish_vs_prologs(struct radv_device *device)
   {
      if (device->vs_prologs) {
      hash_table_foreach (device->vs_prologs, entry) {
      free((void *)entry->key);
      }
               for (unsigned i = 0; i < ARRAY_SIZE(device->simple_vs_prologs); i++) {
      if (!device->simple_vs_prologs[i])
                        for (unsigned i = 0; i < ARRAY_SIZE(device->instance_rate_vs_prologs); i++) {
      if (!device->instance_rate_vs_prologs[i])
                  }
      static VkResult
   radv_device_init_ps_epilogs(struct radv_device *device)
   {
               device->ps_epilogs = _mesa_hash_table_create(NULL, &radv_hash_ps_epilog, &radv_cmp_ps_epilog);
   if (!device->ps_epilogs)
               }
      static void
   radv_device_finish_ps_epilogs(struct radv_device *device)
   {
      if (device->ps_epilogs) {
      hash_table_foreach (device->ps_epilogs, entry) {
      free((void *)entry->key);
      }
         }
      static VkResult
   radv_device_init_tcs_epilogs(struct radv_device *device)
   {
               device->tcs_epilogs = _mesa_hash_table_create(NULL, &radv_hash_tcs_epilog, &radv_cmp_tcs_epilog);
   if (!device->tcs_epilogs)
               }
      static void
   radv_device_finish_tcs_epilogs(struct radv_device *device)
   {
      if (device->tcs_epilogs) {
      hash_table_foreach (device->tcs_epilogs, entry) {
      free((void *)entry->key);
      }
         }
      VkResult
   radv_device_init_vrs_state(struct radv_device *device)
   {
      VkDeviceMemory mem;
   VkBuffer buffer;
   VkResult result;
            VkImageCreateInfo image_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
   .imageType = VK_IMAGE_TYPE_2D,
   .format = VK_FORMAT_D16_UNORM,
   .extent = {MAX_FRAMEBUFFER_WIDTH, MAX_FRAMEBUFFER_HEIGHT, 1},
   .mipLevels = 1,
   .arrayLayers = 1,
   .samples = VK_SAMPLE_COUNT_1_BIT,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
   .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
   .queueFamilyIndexCount = 0,
   .pQueueFamilyIndices = NULL,
               result =
      radv_image_create(radv_device_to_handle(device), &(struct radv_image_create_info){.vk_info = &image_create_info},
      if (result != VK_SUCCESS)
            VkBufferCreateInfo buffer_create_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .pNext =
      &(VkBufferUsageFlags2CreateInfoKHR){
      .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR,
         .size = radv_image_from_handle(image)->planes[0].surface.meta_size,
               result = radv_create_buffer(device, &buffer_create_info, &device->meta_state.alloc, &buffer, true);
   if (result != VK_SUCCESS)
            VkBufferMemoryRequirementsInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2,
      };
   VkMemoryRequirements2 mem_req = {
         };
            VkMemoryAllocateInfo alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
               result = radv_alloc_memory(device, &alloc_info, &device->meta_state.alloc, &mem, true);
   if (result != VK_SUCCESS)
            VkBindBufferMemoryInfo bind_info = {.sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO,
                        result = radv_BindBufferMemory2(radv_device_to_handle(device), 1, &bind_info);
   if (result != VK_SUCCESS)
            device->vrs.image = radv_image_from_handle(image);
   device->vrs.buffer = radv_buffer_from_handle(buffer);
                  fail_bind:
         fail_alloc:
         fail_create:
                  }
      static void
   radv_device_finish_vrs_image(struct radv_device *device)
   {
      if (!device->vrs.image)
            radv_FreeMemory(radv_device_to_handle(device), radv_device_memory_to_handle(device->vrs.mem),
         radv_DestroyBuffer(radv_device_to_handle(device), radv_buffer_to_handle(device->vrs.buffer),
            }
      static enum radv_force_vrs
   radv_parse_vrs_rates(const char *str)
   {
      if (!strcmp(str, "2x2")) {
         } else if (!strcmp(str, "2x1")) {
         } else if (!strcmp(str, "1x2")) {
         } else if (!strcmp(str, "1x1")) {
                  fprintf(stderr, "radv: Invalid VRS rates specified (valid values are 2x2, 2x1, 1x2 and 1x1)\n");
      }
      static const char *
   radv_get_force_vrs_config_file(void)
   {
         }
      static enum radv_force_vrs
   radv_parse_force_vrs_config_file(const char *config_file)
   {
      enum radv_force_vrs force_vrs = RADV_FORCE_VRS_1x1;
   char buf[4];
            f = fopen(config_file, "r");
   if (!f) {
      fprintf(stderr, "radv: Can't open file: '%s'.\n", config_file);
               if (fread(buf, sizeof(buf), 1, f) == 1) {
      buf[3] = '\0';
               fclose(f);
      }
      #ifdef __linux__
      #define BUF_LEN ((10 * (sizeof(struct inotify_event) + NAME_MAX + 1)))
      static int
   radv_notifier_thread_run(void *data)
   {
      struct radv_device *device = data;
   struct radv_notifier *notifier = &device->notifier;
            while (!notifier->quit) {
      const char *file = radv_get_force_vrs_config_file();
   struct timespec tm = {.tv_nsec = 100000000}; /* 1OOms */
            length = read(notifier->fd, buf, BUF_LEN);
   while (i < length) {
               i += sizeof(struct inotify_event) + event->len;
   if (event->mask & IN_MODIFY || event->mask & IN_DELETE_SELF) {
      /* Sleep 100ms for editors that use a temporary file and delete the original. */
                           if (event->mask & IN_DELETE_SELF) {
      inotify_rm_watch(notifier->fd, notifier->watch);
                                    }
      #endif
      static int
   radv_device_init_notifier(struct radv_device *device)
   {
   #ifndef __linux__
         #else
      struct radv_notifier *notifier = &device->notifier;
   const char *file = radv_get_force_vrs_config_file();
            notifier->fd = inotify_init1(IN_NONBLOCK);
   if (notifier->fd < 0)
            notifier->watch = inotify_add_watch(notifier->fd, file, IN_MODIFY | IN_DELETE_SELF);
   if (notifier->watch < 0)
            ret = thrd_create(&notifier->thread, radv_notifier_thread_run, device);
   if (ret)
                  fail_thread:
         fail_watch:
                  #endif
   }
      static void
   radv_device_finish_notifier(struct radv_device *device)
   {
   #ifdef __linux__
               if (!notifier->thread)
            notifier->quit = true;
   thrd_join(notifier->thread, NULL);
   inotify_rm_watch(notifier->fd, notifier->watch);
      #endif
   }
      static void
   radv_device_finish_perf_counter_lock_cs(struct radv_device *device)
   {
      if (!device->perf_counter_lock_cs)
            for (unsigned i = 0; i < 2 * PERF_CTR_MAX_PASSES; ++i) {
      if (device->perf_counter_lock_cs[i])
                  }
      struct dispatch_table_builder {
      struct vk_device_dispatch_table *tables[RADV_DISPATCH_TABLE_COUNT];
   bool used[RADV_DISPATCH_TABLE_COUNT];
      };
      static void
   add_entrypoints(struct dispatch_table_builder *b, const struct vk_device_entrypoint_table *entrypoints,
         {
      for (int32_t i = table - 1; i >= RADV_DEVICE_DISPATCH_TABLE; i--) {
      if (i == RADV_DEVICE_DISPATCH_TABLE || b->used[i]) {
      vk_device_dispatch_table_from_entrypoints(b->tables[i], entrypoints, !b->initialized[i]);
                  if (table < RADV_DISPATCH_TABLE_COUNT)
      }
      static void
   init_dispatch_tables(struct radv_device *device, struct radv_physical_device *physical_device)
   {
      struct dispatch_table_builder b = {0};
   b.tables[RADV_DEVICE_DISPATCH_TABLE] = &device->vk.dispatch_table;
   b.tables[RADV_APP_DISPATCH_TABLE] = &device->layer_dispatch.app;
   b.tables[RADV_RGP_DISPATCH_TABLE] = &device->layer_dispatch.rgp;
   b.tables[RADV_RRA_DISPATCH_TABLE] = &device->layer_dispatch.rra;
            if (!strcmp(physical_device->instance->app_layer, "metroexodus")) {
         } else if (!strcmp(physical_device->instance->app_layer, "rage2")) {
                  if (physical_device->instance->vk.trace_mode & RADV_TRACE_MODE_RGP)
            if ((physical_device->instance->vk.trace_mode & RADV_TRACE_MODE_RRA) && radv_enable_rt(physical_device, false))
         #ifndef _WIN32
      if (physical_device->instance->vk.trace_mode & VK_TRACE_MODE_RMV)
      #endif
         add_entrypoints(&b, &radv_device_entrypoints, RADV_DISPATCH_TABLE_COUNT);
   add_entrypoints(&b, &wsi_device_entrypoints, RADV_DISPATCH_TABLE_COUNT);
      }
      static void
   radv_report_gpuvm_fault(struct radv_device *device)
   {
               if (!radv_vm_fault_occurred(device, &fault_info))
               }
      static VkResult
   radv_check_status(struct vk_device *vk_device)
   {
      struct radv_device *device = container_of(vk_device, struct radv_device, vk);
   enum radv_reset_status status;
            /* If an INNOCENT_CONTEXT_RESET is found in one of the contexts, we need to
   * keep querying in case there's a guilty one, so we can correctly log if the
   * hung happened in this app or not */
   for (int i = 0; i < RADV_NUM_HW_CTX; i++) {
      if (device->hw_ctx[i]) {
               if (status == RADV_GUILTY_CONTEXT_RESET) {
      radv_report_gpuvm_fault(device);
      } else if (status == RADV_INNOCENT_CONTEXT_RESET) {
                        if (context_reset) {
      radv_report_gpuvm_fault(device);
                  }
      static VkResult
   capture_trace(VkQueue _queue)
   {
                        char filename[2048];
   struct tm now;
            t = time(NULL);
            if (queue->device->instance->vk.trace_mode & RADV_TRACE_MODE_RRA) {
      if (_mesa_hash_table_num_entries(queue->device->rra_trace.accel_structs) == 0) {
         } else {
                              if (result == VK_SUCCESS)
         else
                  if (queue->device->vk.memory_trace_data.is_enabled) {
      simple_mtx_lock(&queue->device->vk.memory_trace_data.token_mtx);
   radv_rmv_collect_trace_events(queue->device);
   vk_dump_rmv_capture(&queue->device->vk.memory_trace_data);
               if (queue->device->instance->vk.trace_mode & RADV_TRACE_MODE_RGP)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_physical_device, physical_device, physicalDevice);
   VkResult result;
            enum radv_buffer_robustness buffer_robustness = RADV_BUFFER_ROBUSTNESS_DISABLED;
   bool keep_shader_info = false;
   bool overallocation_disallowed = false;
   bool custom_border_colors = false;
   bool attachment_vrs_enabled = false;
   bool image_float32_atomics = false;
   bool vs_prologs = false;
   UNUSED bool tcs_epilogs = false; /* TODO: Enable for shader object */
   bool ps_epilogs = false;
   bool global_bo_list = false;
   bool image_2d_view_of_3d = false;
   bool primitives_generated_query = false;
   bool use_perf_counters = false;
   bool use_dgc = false;
   bool smooth_lines = false;
   bool mesh_shader_queries = false;
            /* Check enabled features */
   if (pCreateInfo->pEnabledFeatures) {
      if (pCreateInfo->pEnabledFeatures->robustBufferAccess)
                     vk_foreach_struct_const (ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2: {
      const VkPhysicalDeviceFeatures2 *features = (const void *)ext;
   if (features->features.robustBufferAccess)
         dual_src_blend |= features->features.dualSrcBlend;
      }
   case VK_STRUCTURE_TYPE_DEVICE_MEMORY_OVERALLOCATION_CREATE_INFO_AMD: {
      const VkDeviceMemoryOverallocationCreateInfoAMD *overallocation = (const void *)ext;
   if (overallocation->overallocationBehavior == VK_MEMORY_OVERALLOCATION_BEHAVIOR_DISALLOWED_AMD)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_FEATURES_EXT: {
      const VkPhysicalDeviceCustomBorderColorFeaturesEXT *border_color_features = (const void *)ext;
   custom_border_colors = border_color_features->customBorderColors;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_FEATURES_KHR: {
      const VkPhysicalDeviceFragmentShadingRateFeaturesKHR *vrs = (const void *)ext;
   attachment_vrs_enabled = vrs->attachmentFragmentShadingRate;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT: {
      const VkPhysicalDeviceRobustness2FeaturesEXT *features = (const void *)ext;
   if (features->robustBufferAccess2)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_FEATURES_EXT: {
      const VkPhysicalDeviceShaderAtomicFloatFeaturesEXT *features = (const void *)ext;
   if (features->shaderImageFloat32Atomics || features->sparseImageFloat32Atomics)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_FLOAT_2_FEATURES_EXT: {
      const VkPhysicalDeviceShaderAtomicFloat2FeaturesEXT *features = (const void *)ext;
   if (features->shaderImageFloat32AtomicMinMax || features->sparseImageFloat32AtomicMinMax)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT: {
      const VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT *features = (const void *)ext;
   if (features->vertexInputDynamicState)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES: {
      const VkPhysicalDeviceVulkan12Features *features = (const void *)ext;
   if (features->bufferDeviceAddress || features->descriptorIndexing)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_2D_VIEW_OF_3D_FEATURES_EXT: {
      const VkPhysicalDeviceImage2DViewOf3DFeaturesEXT *features = (const void *)ext;
   if (features->image2DViewOf3D)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRIMITIVES_GENERATED_QUERY_FEATURES_EXT: {
      const VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT *features = (const void *)ext;
   if (features->primitivesGeneratedQuery || features->primitivesGeneratedQueryWithRasterizerDiscard ||
      features->primitivesGeneratedQueryWithNonZeroStreams)
         }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_FEATURES_KHR: {
      const VkPhysicalDevicePerformanceQueryFeaturesKHR *features = (const void *)ext;
   if (features->performanceCounterQueryPools)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_FEATURES_NV: {
      const VkPhysicalDeviceDeviceGeneratedCommandsFeaturesNV *features = (const void *)ext;
   if (features->deviceGeneratedCommands)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DEVICE_GENERATED_COMMANDS_COMPUTE_FEATURES_NV: {
      const VkPhysicalDeviceDeviceGeneratedCommandsComputeFeaturesNV *features = (const void *)ext;
   if (features->deviceGeneratedCompute)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT: {
      const VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT *features = (const void *)ext;
   if (features->graphicsPipelineLibrary)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_FEATURES_EXT: {
      const VkPhysicalDeviceExtendedDynamicState3FeaturesEXT *features = (const void *)ext;
   if (features->extendedDynamicState3ColorBlendEnable || features->extendedDynamicState3ColorWriteMask ||
      features->extendedDynamicState3AlphaToCoverageEnable || features->extendedDynamicState3ColorBlendEquation)
         }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_FEATURES_EXT: {
      const VkPhysicalDeviceLineRasterizationFeaturesEXT *features = (const void *)ext;
   if (features->smoothLines)
            }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT: {
      const VkPhysicalDeviceMeshShaderFeaturesEXT *features = (const void *)ext;
   if (features->meshShaderQueries)
            }
   default:
                     device = vk_zalloc2(&physical_device->instance->vk.alloc, pAllocator, sizeof(*device), 8,
         if (!device)
            result = vk_device_init(&device->vk, &physical_device->vk, NULL, pCreateInfo, pAllocator);
   if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, device);
                                 device->vk.command_buffer_ops = &radv_cmd_buffer_ops;
            device->instance = physical_device->instance;
   device->physical_device = physical_device;
   simple_mtx_init(&device->trace_mtx, mtx_plain);
   simple_mtx_init(&device->pstate_mtx, mtx_plain);
                     device->ws = physical_device->ws;
            /* With update after bind we can't attach bo's to the command buffer
   * from the descriptor set anymore, so we have to use a global BO list.
   */
   device->use_global_bo_list = global_bo_list || (device->instance->perftest_flags & RADV_PERFTEST_BO_LIST) ||
                              device->vk.enabled_extensions.EXT_descriptor_indexing ||
                                             device->primitives_generated_query = primitives_generated_query;
   device->uses_device_generated_commands = use_dgc;
   device->smooth_lines = smooth_lines;
                     device->overallocation_disallowed = overallocation_disallowed;
            if (physical_device->rad_info.register_shadowing_required || device->instance->debug_flags & RADV_DEBUG_SHADOW_REGS)
            /* Create one context per queue priority. */
   for (unsigned i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const VkDeviceQueueCreateInfo *queue_create = &pCreateInfo->pQueueCreateInfos[i];
   const VkDeviceQueueGlobalPriorityCreateInfoKHR *global_priority =
                  if (device->hw_ctx[priority])
            result = device->ws->ctx_create(device->ws, priority, &device->hw_ctx[priority]);
   if (result != VK_SUCCESS)
               for (unsigned i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const VkDeviceQueueCreateInfo *queue_create = &pCreateInfo->pQueueCreateInfos[i];
   uint32_t qfi = queue_create->queueFamilyIndex;
   const VkDeviceQueueGlobalPriorityCreateInfoKHR *global_priority =
            device->queues[qfi] = vk_alloc(&device->vk.alloc, queue_create->queueCount * sizeof(struct radv_queue), 8,
         if (!device->queues[qfi]) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
                                 for (unsigned q = 0; q < queue_create->queueCount; q++) {
      result = radv_queue_init(device, &device->queues[qfi][q], q, queue_create, global_priority);
   if (result != VK_SUCCESS)
         }
            device->shader_use_invisible_vram = (device->instance->perftest_flags & RADV_PERFTEST_DMA_SHADERS) &&
               result = radv_init_shader_upload_queue(device);
   if (result != VK_SUCCESS)
            device->pbb_allowed =
            device->mesh_fast_launch_2 = (device->instance->perftest_flags & RADV_PERFTEST_GS_FAST_LAUNCH_2) &&
            /* The maximum number of scratch waves. Scratch space isn't divided
   * evenly between CUs. The number is only a function of the number of CUs.
   * We can decrease the constant to decrease the scratch buffer size.
   *
   * sctx->scratch_waves must be >= the maximum possible size of
   * 1 threadgroup, so that the hw doesn't hang from being unable
   * to start any.
   *
   * The recommended value is 4 per CU at most. Higher numbers don't
   * bring much benefit, but they still occupy chip resources (think
   * async compute). I've seen ~2% performance difference between 4 and 32.
   */
   uint32_t max_threads_per_block = 2048;
                     if (device->physical_device->rad_info.gfx_level >= GFX7) {
      /* If the KMD allows it (there is a KMD hw register for it),
   * allow launching waves out-of-order.
   */
               /* Disable partial preemption for task shaders.
   * The kernel may not support preemption, but PAL always sets this bit,
   * so let's also set it here for consistency.
   */
            if (device->instance->debug_flags & RADV_DEBUG_HANG) {
      /* Enable GPU hangs detection and dump logs if a GPU hang is
   * detected.
   */
            if (!radv_init_trace(device)) {
      result = VK_ERROR_INITIALIZATION_FAILED;
               fprintf(stderr, "*****************************************************************************\n");
   fprintf(stderr, "* WARNING: RADV_DEBUG=hang is costly and should only be used for debugging! *\n");
            /* Wait for idle after every draw/dispatch to identify the
   * first bad call.
   */
                        if (device->instance->vk.trace_mode & RADV_TRACE_MODE_RGP) {
      if (device->physical_device->rad_info.gfx_level < GFX8 || device->physical_device->rad_info.gfx_level > GFX11) {
      fprintf(stderr, "GPU hardware not supported: refer to "
                           if (!radv_sqtt_init(device)) {
      result = VK_ERROR_INITIALIZATION_FAILED;
               fprintf(stderr,
         "radv: Thread trace support is enabled (initial buffer size: %u MiB, "
   "instruction timing: %s, cache counters: %s).\n",
            if (radv_spm_trace_enabled(device->instance)) {
      if (device->physical_device->rad_info.gfx_level >= GFX10) {
      if (!radv_spm_init(device)) {
      result = VK_ERROR_INITIALIZATION_FAILED;
         } else {
                     #ifndef _WIN32
      if (physical_device->instance->vk.trace_mode & VK_TRACE_MODE_RMV) {
      struct vk_rmv_device_info info;
   memset(&info, 0, sizeof(struct vk_rmv_device_info));
   radv_rmv_fill_device_info(physical_device, &info);
   vk_memory_trace_init(&device->vk, &info);
         #endif
         if (getenv("RADV_TRAP_HANDLER")) {
      /* TODO: Add support for more hardware. */
            fprintf(stderr, "**********************************************************************\n");
   fprintf(stderr, "* WARNING: RADV_TRAP_HANDLER is experimental and only for debugging! *\n");
            /* To get the disassembly of the faulty shaders, we have to
   * keep some shader info around.
   */
            if (!radv_trap_handler_init(device)) {
      result = VK_ERROR_INITIALIZATION_FAILED;
                  if (device->physical_device->rad_info.gfx_level == GFX10_3) {
      if (getenv("RADV_FORCE_VRS_CONFIG_FILE")) {
                        if (radv_device_init_notifier(device)) {
         } else {
            } else if (getenv("RADV_FORCE_VRS")) {
               device->force_vrs = radv_parse_vrs_rates(vrs_rates);
                  /* PKT3_LOAD_SH_REG_INDEX is supported on GFX8+, but it hangs with compute queues until GFX10.3. */
            device->keep_shader_info = keep_shader_info;
   result = radv_device_init_meta(device);
   if (result != VK_SUCCESS)
                     /* If the border color extension is enabled, let's create the buffer we need. */
   if (custom_border_colors) {
      result = radv_device_init_border_color(device);
   if (result != VK_SUCCESS)
               if (vs_prologs) {
      result = radv_device_init_vs_prologs(device);
   if (result != VK_SUCCESS)
               if (tcs_epilogs) {
      result = radv_device_init_tcs_epilogs(device);
   if (result != VK_SUCCESS)
               if (ps_epilogs) {
      result = radv_device_init_ps_epilogs(device);
   if (result != VK_SUCCESS)
               if (!(device->instance->debug_flags & RADV_DEBUG_NO_IBS))
            struct vk_pipeline_cache_create_info info = {.weak_ref = true};
   device->mem_cache = vk_pipeline_cache_create(&device->vk, &info, NULL);
   if (!device->mem_cache) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               device->force_aniso = MIN2(16, (int)debug_get_num_option("RADV_TEX_ANISO", -1));
   if (device->force_aniso >= 0) {
                           if (device->instance->vk.app_info.engine_name && !strcmp(device->instance->vk.app_info.engine_name, "DXVK")) {
      /* For DXVK 2.3.0 and older, use dualSrcBlend to determine if this is D3D9. */
   bool is_d3d9 = !dual_src_blend;
   if (device->instance->vk.app_info.engine_version > VK_MAKE_VERSION(2, 3, 0))
                        if (use_perf_counters) {
      size_t bo_size = PERF_CTR_BO_PASS_OFFSET + sizeof(uint64_t) * PERF_CTR_MAX_PASSES;
   result = device->ws->buffer_create(device->ws, bo_size, 4096, RADEON_DOMAIN_GTT,
               if (result != VK_SUCCESS)
            device->perf_counter_lock_cs = calloc(sizeof(struct radeon_winsys_cs *), 2 * PERF_CTR_MAX_PASSES);
   if (!device->perf_counter_lock_cs) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               if (!device->physical_device->ac_perfcounters.blocks) {
      result = VK_ERROR_INITIALIZATION_FAILED;
                  if ((device->instance->vk.trace_mode & RADV_TRACE_MODE_RRA) && radv_enable_rt(physical_device, false)) {
                  if (device->vk.enabled_features.rayTracingPipelineShaderGroupHandleCaptureReplay) {
                  *pDevice = radv_device_to_handle(device);
         fail_cache:
         fail_meta:
         fail:
                        radv_trap_handler_finish(device);
            radv_device_finish_perf_counter_lock_cs(device);
   if (device->perf_counter_bo)
         if (device->gfx_init)
            radv_device_finish_notifier(device);
   radv_device_finish_vs_prologs(device);
   radv_device_finish_tcs_epilogs(device);
   radv_device_finish_ps_epilogs(device);
                  fail_queue:
      for (unsigned i = 0; i < RADV_MAX_QUEUE_FAMILIES; i++) {
      for (unsigned q = 0; q < device->queue_count[i]; q++)
         if (device->queue_count[i])
               for (unsigned i = 0; i < RADV_NUM_HW_CTX; i++) {
      if (device->hw_ctx[i])
                                 simple_mtx_destroy(&device->pstate_mtx);
   simple_mtx_destroy(&device->trace_mtx);
   simple_mtx_destroy(&device->rt_handles_mtx);
            vk_device_finish(&device->vk);
   vk_free(&device->vk.alloc, device);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_DestroyDevice(VkDevice _device, const VkAllocationCallbacks *pAllocator)
   {
               if (!device)
            if (device->capture_replay_arena_vas)
            radv_device_finish_perf_counter_lock_cs(device);
   if (device->perf_counter_bo)
            if (device->gfx_init)
            radv_device_finish_notifier(device);
   radv_device_finish_vs_prologs(device);
   radv_device_finish_tcs_epilogs(device);
   radv_device_finish_ps_epilogs(device);
   radv_device_finish_border_color(device);
            for (unsigned i = 0; i < RADV_MAX_QUEUE_FAMILIES; i++) {
      for (unsigned q = 0; q < device->queue_count[i]; q++)
         if (device->queue_count[i])
      }
   if (device->private_sdma_queue != VK_NULL_HANDLE) {
      radv_queue_finish(device->private_sdma_queue);
                                                   for (unsigned i = 0; i < RADV_NUM_HW_CTX; i++) {
      if (device->hw_ctx[i])
               mtx_destroy(&device->overallocation_mutex);
   simple_mtx_destroy(&device->pstate_mtx);
   simple_mtx_destroy(&device->trace_mtx);
            radv_trap_handler_finish(device);
                                                         vk_device_finish(&device->vk);
      }
      bool
   radv_get_memory_fd(struct radv_device *device, struct radv_device_memory *memory, int *pFD)
   {
      /* Only set BO metadata for the first plane */
   if (memory->image && memory->image->bindings[0].offset == 0) {
      struct radeon_bo_metadata metadata;
   radv_init_metadata(device, memory->image, &metadata);
                  }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetImageMemoryRequirements2(VkDevice _device, const VkImageMemoryRequirementsInfo2 *pInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
            pMemoryRequirements->memoryRequirements.memoryTypeBits =
      ((1u << device->physical_device->memory_properties.memoryTypeCount) - 1u) &
         pMemoryRequirements->memoryRequirements.size = image->size;
            vk_foreach_struct (ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *req = (VkMemoryDedicatedRequirements *)ext;
   req->requiresDedicatedAllocation = image->shareable && image->vk.tiling != VK_IMAGE_TILING_LINEAR;
   req->prefersDedicatedAllocation = req->requiresDedicatedAllocation;
      }
   default:
               }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDeviceImageMemoryRequirements(VkDevice device, const VkDeviceImageMemoryRequirements *pInfo,
         {
      UNUSED VkResult result;
            /* Determining the image size/alignment require to create a surface, which is complicated without
   * creating an image.
   * TODO: Avoid creating an image.
   */
   result =
                  VkImageMemoryRequirementsInfo2 info2 = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_BindImageMemory2(VkDevice _device, uint32_t bindInfoCount, const VkBindImageMemoryInfo *pBindInfos)
   {
               for (uint32_t i = 0; i < bindInfoCount; ++i) {
      RADV_FROM_HANDLE(radv_device_memory, mem, pBindInfos[i].memory);
            #ifdef RADV_USE_WSI_PLATFORM
         const VkBindImageMemorySwapchainInfoKHR *swapchain_info =
            if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE) {
                     image->bindings[0].bo = swapchain_img->bindings[0].bo;
   image->bindings[0].offset = swapchain_img->bindings[0].offset;
      #endif
            if (mem->alloc_size) {
      VkImageMemoryRequirementsInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
      };
   VkMemoryRequirements2 reqs = {
                           if (pBindInfos[i].memoryOffset + reqs.memoryRequirements.size > mem->alloc_size) {
                     if (image->disjoint) {
                     switch (plane_info->planeAspect) {
   case VK_IMAGE_ASPECT_PLANE_0_BIT:
      image->bindings[0].bo = mem->bo;
   image->bindings[0].offset = pBindInfos[i].memoryOffset;
      case VK_IMAGE_ASPECT_PLANE_1_BIT:
      image->bindings[1].bo = mem->bo;
   image->bindings[1].offset = pBindInfos[i].memoryOffset;
      case VK_IMAGE_ASPECT_PLANE_2_BIT:
      image->bindings[2].bo = mem->bo;
   image->bindings[2].offset = pBindInfos[i].memoryOffset;
      default:
            } else {
      image->bindings[0].bo = mem->bo;
      }
      }
      }
      static inline unsigned
   si_tile_mode_index(const struct radv_image_plane *plane, unsigned level, bool stencil)
   {
      if (stencil)
         else
      }
      static uint32_t
   radv_surface_max_layer_count(struct radv_image_view *iview)
   {
      return iview->vk.view_type == VK_IMAGE_VIEW_TYPE_3D ? iview->extent.depth
      }
      static unsigned
   get_dcc_max_uncompressed_block_size(const struct radv_device *device, const struct radv_image_view *iview)
   {
      if (device->physical_device->rad_info.gfx_level < GFX10 && iview->image->vk.samples > 1) {
      if (iview->image->planes[0].surface.bpe == 1)
         else if (iview->image->planes[0].surface.bpe == 2)
                  }
      static unsigned
   get_dcc_min_compressed_block_size(const struct radv_device *device)
   {
      if (!device->physical_device->rad_info.has_dedicated_vram) {
      /* amdvlk: [min-compressed-block-size] should be set to 32 for
   * dGPU and 64 for APU because all of our APUs to date use
   * DIMMs which have a request granularity size of 64B while all
   * other chips have a 32B request size.
   */
                  }
      static uint32_t
   radv_init_dcc_control_reg(struct radv_device *device, struct radv_image_view *iview)
   {
      unsigned max_uncompressed_block_size = get_dcc_max_uncompressed_block_size(device, iview);
   unsigned min_compressed_block_size = get_dcc_min_compressed_block_size(device);
   unsigned max_compressed_block_size;
   unsigned independent_128b_blocks;
            if (!radv_dcc_enabled(iview->image, iview->vk.base_mip_level))
            /* For GFX9+ ac_surface computes values for us (except min_compressed
   * and max_uncompressed) */
   if (device->physical_device->rad_info.gfx_level >= GFX9) {
      max_compressed_block_size = iview->image->planes[0].surface.u.gfx9.color.dcc.max_compressed_block_size;
   independent_128b_blocks = iview->image->planes[0].surface.u.gfx9.color.dcc.independent_128B_blocks;
      } else {
               if (iview->image->vk.usage &
      (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT)) {
   /* If this DCC image is potentially going to be used in texture
   * fetches, we need some special settings.
   */
   independent_64b_blocks = 1;
      } else {
      /* MAX_UNCOMPRESSED_BLOCK_SIZE must be >=
   * MAX_COMPRESSED_BLOCK_SIZE. Set MAX_COMPRESSED_BLOCK_SIZE as
   * big as possible for better compression state.
   */
   independent_64b_blocks = 0;
                  uint32_t result = S_028C78_MAX_UNCOMPRESSED_BLOCK_SIZE(max_uncompressed_block_size) |
                        if (device->physical_device->rad_info.gfx_level >= GFX11) {
      result |= S_028C78_INDEPENDENT_128B_BLOCKS_GFX11(independent_128b_blocks) |
            } else {
                     }
      void
   radv_initialise_color_surface(struct radv_device *device, struct radv_color_buffer_info *cb,
         {
      const struct util_format_description *desc;
   unsigned ntype, format, swap, endian;
   unsigned blend_clamp = 0, blend_bypass = 0;
   uint64_t va;
   const struct radv_image_plane *plane = &iview->image->planes[iview->plane_id];
   const struct radeon_surf *surf = &plane->surface;
                              /* Intensity is implemented as Red, so treat it that way. */
   if (device->physical_device->rad_info.gfx_level >= GFX11)
         else
            uint32_t plane_id = iview->image->disjoint ? iview->plane_id : 0;
            if (iview->nbc_view.valid) {
      va += iview->nbc_view.base_address_offset;
                        if (device->physical_device->rad_info.gfx_level >= GFX9) {
      if (device->physical_device->rad_info.gfx_level >= GFX11) {
      cb->cb_color_attrib3 |= S_028EE0_COLOR_SW_MODE(surf->u.gfx9.swizzle_mode) |
      } else if (device->physical_device->rad_info.gfx_level >= GFX10) {
      cb->cb_color_attrib3 |= S_028EE0_COLOR_SW_MODE(surf->u.gfx9.swizzle_mode) |
                  } else {
      struct gfx9_surf_meta_flags meta = {
      .rb_aligned = 1,
                              cb->cb_color_attrib |= S_028C74_COLOR_SW_MODE(surf->u.gfx9.swizzle_mode) |
                           cb->cb_color_base += surf->u.gfx9.surf_offset >> 8;
      } else {
      const struct legacy_surf_level *level_info = &surf->u.legacy.level[iview->vk.base_mip_level];
            cb->cb_color_base += level_info->offset_256B;
   if (level_info->mode == RADEON_SURF_MODE_2D)
            pitch_tile_max = level_info->nblk_x / 8 - 1;
   slice_tile_max = (level_info->nblk_x * level_info->nblk_y) / 64 - 1;
            cb->cb_color_pitch = S_028C64_TILE_MAX(pitch_tile_max);
   cb->cb_color_slice = S_028C68_TILE_MAX(slice_tile_max);
                     if (radv_image_has_fmask(iview->image)) {
      if (device->physical_device->rad_info.gfx_level >= GFX7)
         cb->cb_color_attrib |= S_028C74_FMASK_TILE_MODE_INDEX(surf->u.legacy.color.fmask.tiling_index);
      } else {
      /* This must be set for fast clear to work without FMASK. */
   if (device->physical_device->rad_info.gfx_level >= GFX7)
         cb->cb_color_attrib |= S_028C74_FMASK_TILE_MODE_INDEX(tile_mode_index);
                  /* CMASK variables */
   va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset;
   va += surf->cmask_offset;
            va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset;
            if (radv_dcc_enabled(iview->image, iview->vk.base_mip_level) && device->physical_device->rad_info.gfx_level <= GFX8)
            unsigned dcc_tile_swizzle = tile_swizzle;
            cb->cb_dcc_base = va >> 8;
            /* GFX10 field has the same base shift as the GFX6 field. */
   uint32_t max_slice = radv_surface_max_layer_count(iview) - 1;
   uint32_t slice_start = iview->nbc_view.valid ? 0 : iview->vk.base_array_layer;
            if (iview->image->vk.samples > 1) {
               if (device->physical_device->rad_info.gfx_level >= GFX11)
         else
               if (radv_image_has_fmask(iview->image)) {
      va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset + surf->fmask_offset;
   cb->cb_color_fmask = va >> 8;
      } else {
                  ntype = ac_get_cb_number_type(desc->format);
   format = ac_get_cb_format(device->physical_device->rad_info.gfx_level, desc->format);
            swap = radv_translate_colorswap(iview->vk.format, false);
            /* blend clamp should be set for all NORM/SRGB types */
   if (ntype == V_028C70_NUMBER_UNORM || ntype == V_028C70_NUMBER_SNORM || ntype == V_028C70_NUMBER_SRGB)
            /* set blend bypass according to docs if SINT/UINT or
         if (ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT || format == V_028C70_COLOR_8_24 ||
      format == V_028C70_COLOR_24_8 || format == V_028C70_COLOR_X24_8_32_FLOAT) {
   blend_clamp = 0;
         #if 0
   if ((ntype == V_028C70_NUMBER_UINT || ntype == V_028C70_NUMBER_SINT) &&
      (format == V_028C70_COLOR_8 ||
      format == V_028C70_COLOR_8_8 ||
   ->color_is_int8 = true;
   #endif
      cb->cb_color_info = S_028C70_COMP_SWAP(swap) | S_028C70_BLEND_CLAMP(blend_clamp) |
                     S_028C70_BLEND_BYPASS(blend_bypass) | S_028C70_SIMPLE_FLOAT(1) |
            if (device->physical_device->rad_info.gfx_level >= GFX11)
         else
            if (radv_image_has_fmask(iview->image)) {
      cb->cb_color_info |= S_028C70_COMPRESSION(1);
   if (device->physical_device->rad_info.gfx_level == GFX6) {
      unsigned fmask_bankh = util_logbase2(surf->u.legacy.color.fmask.bankh);
               if (radv_image_is_tc_compat_cmask(iview->image)) {
      /* Allow the texture block to read FMASK directly
   * without decompressing it. This bit must be cleared
   * when performing FMASK_DECOMPRESS or DCC_COMPRESS,
   * otherwise the operation doesn't happen.
                  if (device->physical_device->rad_info.gfx_level == GFX8) {
      /* Set CMASK into a tiling format that allows
   * the texture block to read it.
   */
                     if (radv_image_has_cmask(iview->image) && !(device->instance->debug_flags & RADV_DEBUG_NO_FAST_CLEARS))
            if (radv_dcc_enabled(iview->image, iview->vk.base_mip_level) && !iview->disable_dcc_mrt &&
      device->physical_device->rad_info.gfx_level < GFX11)
                  /* This must be set for fast clear to work without FMASK. */
   if (!radv_image_has_fmask(iview->image) && device->physical_device->rad_info.gfx_level == GFX6) {
      unsigned bankh = util_logbase2(surf->u.legacy.bankh);
               if (device->physical_device->rad_info.gfx_level >= GFX9) {
      unsigned mip0_depth = iview->image->vk.image_type == VK_IMAGE_TYPE_3D ? (iview->extent.depth - 1)
         unsigned width = vk_format_get_plane_width(iview->image->vk.format, iview->plane_id, iview->extent.width);
   unsigned height = vk_format_get_plane_height(iview->image->vk.format, iview->plane_id, iview->extent.height);
            if (device->physical_device->rad_info.gfx_level >= GFX10) {
               if (iview->nbc_view.valid) {
      base_level = iview->nbc_view.level;
                        cb->cb_color_attrib3 |= S_028EE0_MIP0_DEPTH(mip0_depth) | S_028EE0_RESOURCE_TYPE(surf->u.gfx9.resource_type) |
      } else {
      cb->cb_color_view |= S_028C6C_MIP_LEVEL_GFX9(iview->vk.base_mip_level);
               /* GFX10.3+ can set a custom pitch for 1D and 2D non-array, but it must be a multiple
   * of 256B. Only set it for 2D linear for multi-GPU interop.
   *
   * We set the pitch in MIP0_WIDTH.
   */
   if (device->physical_device->rad_info.gfx_level && iview->image->vk.image_type == VK_IMAGE_TYPE_2D &&
                              /* Subsampled images have the pitch in the units of blocks. */
   if (plane->surface.blk_w == 2)
               cb->cb_color_attrib2 =
         }
      static unsigned
   radv_calc_decompress_on_z_planes(const struct radv_device *device, struct radv_image_view *iview)
   {
                        if (device->physical_device->rad_info.gfx_level >= GFX9) {
      /* Default value for 32-bit depth surfaces. */
            if (iview->vk.format == VK_FORMAT_D16_UNORM && iview->image->vk.samples > 1)
            /* Workaround for a DB hang when ITERATE_256 is set to 1. Only affects 4X MSAA D/S images. */
   if (device->physical_device->rad_info.has_two_planes_iterate256_bug &&
      radv_image_get_iterate256(device, iview->image) && !radv_image_tile_stencil_disabled(device, iview->image) &&
   iview->image->vk.samples == 4) {
                  } else {
      if (iview->vk.format == VK_FORMAT_D16_UNORM) {
      /* Do not enable Z plane compression for 16-bit depth
   * surfaces because isn't supported on GFX8. Only
   * 32-bit depth surfaces are supported by the hardware.
   * This allows to maintain shader compatibility and to
   * reduce the number of depth decompressions.
   */
      } else {
      if (iview->image->vk.samples <= 1)
         else if (iview->image->vk.samples <= 4)
         else
                     }
      void
   radv_initialise_vrs_surface(struct radv_image *image, struct radv_buffer *htile_buffer, struct radv_ds_buffer_info *ds)
   {
               assert(image->vk.format == VK_FORMAT_D16_UNORM);
            ds->db_z_info = S_028038_FORMAT(V_028040_Z_16) | S_028038_SW_MODE(surf->u.gfx9.swizzle_mode) |
                           ds->db_htile_data_base = radv_buffer_get_va(htile_buffer->bo) >> 8;
   ds->db_htile_surface =
      }
      void
   radv_initialise_ds_surface(const struct radv_device *device, struct radv_ds_buffer_info *ds,
         {
      unsigned level = iview->vk.base_mip_level;
   unsigned format, stencil_format;
   uint64_t va, s_offs, z_offs;
   bool stencil_only = iview->image->vk.format == VK_FORMAT_S8_UINT;
   const struct radv_image_plane *plane = &iview->image->planes[0];
                              format = radv_translate_dbformat(iview->image->vk.format);
            uint32_t max_slice = radv_surface_max_layer_count(iview) - 1;
   ds->db_depth_view = S_028008_SLICE_START(iview->vk.base_array_layer) | S_028008_SLICE_MAX(max_slice) |
               if (device->physical_device->rad_info.gfx_level >= GFX10) {
      ds->db_depth_view |=
               ds->db_htile_data_base = 0;
            va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset;
            /* Recommended value for better performance with 4x and 8x. */
   ds->db_render_override2 = S_028010_DECOMPRESS_Z_ON_FLUSH(iview->image->vk.samples >= 4) |
            if (device->physical_device->rad_info.gfx_level >= GFX9) {
      assert(surf->u.gfx9.surf_offset == 0);
            ds->db_z_info = S_028038_FORMAT(format) | S_028038_NUM_SAMPLES(util_logbase2(iview->image->vk.samples)) |
                     ds->db_stencil_info = S_02803C_FORMAT(stencil_format) | S_02803C_SW_MODE(surf->u.gfx9.zs.stencil_swizzle_mode) |
            if (device->physical_device->rad_info.gfx_level == GFX9) {
      ds->db_z_info2 = S_028068_EPITCH(surf->u.gfx9.epitch);
               ds->db_depth_view |= S_028008_MIPID(level);
   ds->db_depth_size =
            if (radv_htile_enabled(iview->image, level)) {
                                                            ds->db_z_info |= S_028040_ITERATE_FLUSH(1);
   ds->db_stencil_info |= S_028044_ITERATE_FLUSH(1);
   ds->db_z_info |= S_028040_ITERATE_256(iterate256);
      } else {
      ds->db_z_info |= S_028038_ITERATE_FLUSH(1);
                  if (radv_image_tile_stencil_disabled(device, iview->image)) {
                  va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset + surf->meta_offset;
                  if (device->physical_device->rad_info.gfx_level == GFX9) {
                  if (radv_image_has_vrs_htile(device, iview->image)) {
                     if (device->physical_device->rad_info.gfx_level >= GFX11) {
            } else {
               if (stencil_only)
            z_offs += (uint64_t)surf->u.legacy.level[level].offset_256B * 256;
            ds->db_depth_info = S_02803C_ADDR5_SWIZZLE_MASK(!radv_image_is_tc_compat_htile(iview->image));
   ds->db_z_info = S_028040_FORMAT(format) | S_028040_ZRANGE_PRECISION(1);
            if (iview->image->vk.samples > 1)
            if (device->physical_device->rad_info.gfx_level >= GFX7) {
      const struct radeon_info *info = &device->physical_device->rad_info;
   unsigned tiling_index = surf->u.legacy.tiling_index[level];
   unsigned stencil_index = surf->u.legacy.zs.stencil_tiling_index[level];
   unsigned macro_index = surf->u.legacy.macro_tile_index;
   unsigned tile_mode = info->si_tile_mode_array[tiling_index];
                                 ds->db_depth_info |= S_02803C_ARRAY_MODE(G_009910_ARRAY_MODE(tile_mode)) |
                        S_02803C_PIPE_CONFIG(G_009910_PIPE_CONFIG(tile_mode)) |
      ds->db_z_info |= S_028040_TILE_SPLIT(G_009910_TILE_SPLIT(tile_mode));
      } else {
      unsigned tile_mode_index = si_tile_mode_index(&iview->image->planes[0], level, false);
   ds->db_z_info |= S_028040_TILE_MODE_INDEX(tile_mode_index);
   tile_mode_index = si_tile_mode_index(&iview->image->planes[0], level, true);
   ds->db_stencil_info |= S_028044_TILE_MODE_INDEX(tile_mode_index);
   if (stencil_only)
               ds->db_depth_size =
                  if (radv_htile_enabled(iview->image, level)) {
               if (radv_image_tile_stencil_disabled(device, iview->image)) {
                  va = radv_buffer_get_va(iview->image->bindings[0].bo) + iview->image->bindings[0].offset + surf->meta_offset;
                                    ds->db_htile_surface |= S_028ABC_TC_COMPATIBLE(1);
                     ds->db_z_read_base = ds->db_z_write_base = z_offs >> 8;
      }
      void
   radv_gfx11_set_db_render_control(const struct radv_device *device, unsigned num_samples, unsigned *db_render_control)
   {
      const struct radv_physical_device *pdevice = device->physical_device;
            if (pdevice->rad_info.has_dedicated_vram) {
      if (num_samples == 8)
         else if (num_samples == 4)
      } else {
      if (num_samples == 8)
               /* TODO: We may want to disable this workaround for future chips. */
   if (num_samples >= 4) {
      if (max_allowed_tiles_in_wave)
         else
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetMemoryFdKHR(VkDevice _device, const VkMemoryGetFdInfoKHR *pGetFdInfo, int *pFD)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
                     /* At the moment, we support only the below handle types. */
   assert(pGetFdInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
            bool ret = radv_get_memory_fd(device, memory, pFD);
   if (ret == false)
            }
      static uint32_t
   radv_compute_valid_memory_types_attempt(struct radv_physical_device *dev, enum radeon_bo_domain domains,
         {
      /* Don't count GTT/CPU as relevant:
   *
   * - We're not fully consistent between the two.
   * - Sometimes VRAM gets VRAM|GTT.
   */
   const enum radeon_bo_domain relevant_domains = RADEON_DOMAIN_VRAM | RADEON_DOMAIN_GDS | RADEON_DOMAIN_OA;
   uint32_t bits = 0;
   for (unsigned i = 0; i < dev->memory_properties.memoryTypeCount; ++i) {
      if ((domains & relevant_domains) != (dev->memory_domains[i] & relevant_domains))
            if ((flags & ~ignore_flags) != (dev->memory_flags[i] & ~ignore_flags))
                           }
      static uint32_t
   radv_compute_valid_memory_types(struct radv_physical_device *dev, enum radeon_bo_domain domains,
         {
      enum radeon_bo_flag ignore_flags = ~(RADEON_FLAG_NO_CPU_ACCESS | RADEON_FLAG_GTT_WC);
            if (!bits) {
      ignore_flags |= RADEON_FLAG_GTT_WC;
               if (!bits) {
      ignore_flags |= RADEON_FLAG_NO_CPU_ACCESS;
               /* Avoid 32-bit memory types for shared memory. */
               }
   VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetMemoryFdPropertiesKHR(VkDevice _device, VkExternalMemoryHandleTypeFlagBits handleType, int fd,
         {
               switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT: {
      enum radeon_bo_domain domains;
   enum radeon_bo_flag flags;
   if (!device->ws->buffer_get_flags_from_fd(device->ws, fd, &domains, &flags))
            pMemoryFdProperties->memoryTypeBits = radv_compute_valid_memory_types(device->physical_device, domains, flags);
      }
   default:
      /* The valid usage section for this function says:
   *
   *    "handleType must not be one of the handle types defined as
   *    opaque."
   *
   * So opaque handle types fall into the default "unsupported" case.
   */
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetCalibratedTimestampsEXT(VkDevice _device, uint32_t timestampCount,
               {
   #ifndef _WIN32
      RADV_FROM_HANDLE(radv_device, device, _device);
   uint32_t clock_crystal_freq = device->physical_device->rad_info.clock_crystal_freq;
   int d;
   uint64_t begin, end;
         #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
         for (d = 0; d < timestampCount; d++) {
      switch (pTimestampInfos[d].timeDomain) {
   case VK_TIME_DOMAIN_DEVICE_EXT:
      pTimestamps[d] = device->ws->query_value(device->ws, RADEON_TIMESTAMP);
   uint64_t device_period = DIV_ROUND_UP(1000000, clock_crystal_freq);
   max_clock_period = MAX2(max_clock_period, device_period);
      case VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT:
      pTimestamps[d] = vk_clock_gettime(CLOCK_MONOTONIC);
         #ifdef CLOCK_MONOTONIC_RAW
         case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
         #endif
         default:
      pTimestamps[d] = 0;
               #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
                     #else
         #endif
   }
      bool
   radv_device_set_pstate(struct radv_device *device, bool enable)
   {
      struct radeon_winsys *ws = device->ws;
            if (device->physical_device->rad_info.has_stable_pstate) {
      /* pstate is per-device; setting it for one ctx is sufficient.
   * We pick the first initialized one below. */
   for (unsigned i = 0; i < RADV_NUM_HW_CTX; i++)
      if (device->hw_ctx[i])
               }
      bool
   radv_device_acquire_performance_counters(struct radv_device *device)
   {
      bool result = true;
            if (device->pstate_cnt == 0) {
      result = radv_device_set_pstate(device, true);
   if (result)
               simple_mtx_unlock(&device->pstate_mtx);
      }
      void
   radv_device_release_performance_counters(struct radv_device *device)
   {
               if (--device->pstate_cnt == 0)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_AcquireProfilingLockKHR(VkDevice _device, const VkAcquireProfilingLockInfoKHR *pInfo)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
   bool result = radv_device_acquire_performance_counters(device);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_ReleaseProfilingLockKHR(VkDevice _device)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetDeviceImageSubresourceLayoutKHR(VkDevice device, const VkDeviceImageSubresourceInfoKHR *pInfo,
         {
      UNUSED VkResult result;
            result =
                              }
