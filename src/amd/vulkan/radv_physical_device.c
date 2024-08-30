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
      #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
      #include "util/disk_cache.h"
   #include "util/hex.h"
   #include "util/u_debug.h"
   #include "radv_debug.h"
   #include "radv_private.h"
      #ifdef _WIN32
   typedef void *drmDevicePtr;
   #include <io.h>
   #else
   #include <amdgpu.h>
   #include <xf86drm.h>
   #include "drm-uapi/amdgpu_drm.h"
   #include "winsys/amdgpu/radv_amdgpu_winsys_public.h"
   #endif
   #include "winsys/null/radv_null_winsys_public.h"
   #include "git_sha1.h"
      #ifdef LLVM_AVAILABLE
   #include "ac_llvm_util.h"
   #endif
      static bool
   radv_perf_query_supported(const struct radv_physical_device *pdev)
   {
      /* SQTT / SPM interfere with the register states for perf counters, and
   * the code has only been tested on GFX10.3 */
      }
      static bool
   radv_taskmesh_enabled(const struct radv_physical_device *pdevice)
   {
      return pdevice->use_ngg && !pdevice->use_llvm && pdevice->rad_info.gfx_level >= GFX10_3 &&
      }
      static bool
   radv_vrs_attachment_enabled(const struct radv_physical_device *pdevice)
   {
         }
      static bool
   radv_NV_device_generated_commands_enabled(const struct radv_physical_device *device)
   {
         }
      static bool
   radv_is_conformant(const struct radv_physical_device *pdevice)
   {
         }
      static void
   parse_hex(char *out, const char *in, unsigned length)
   {
      for (unsigned i = 0; i < length; ++i)
            for (unsigned i = 0; i < 2 * length; ++i) {
      unsigned v = in[i] <= '9' ? in[i] - '0' : (in[i] >= 'a' ? (in[i] - 'a' + 10) : (in[i] - 'A' + 10));
         }
      static int
   radv_device_get_cache_uuid(struct radv_physical_device *pdevice, void *uuid)
   {
      enum radeon_family family = pdevice->rad_info.family;
   bool conformant_trunc_coord = pdevice->rad_info.conformant_trunc_coord;
   struct mesa_sha1 ctx;
   unsigned char sha1[20];
            memset(uuid, 0, VK_UUID_SIZE);
         #ifdef RADV_BUILD_ID_OVERRIDE
      {
      unsigned size = strlen(RADV_BUILD_ID_OVERRIDE) / 2;
   char *data = alloca(size);
   parse_hex(data, RADV_BUILD_ID_OVERRIDE, size);
         #else
      if (!disk_cache_get_function_identifier(radv_device_get_cache_uuid, &ctx))
      #endif
      #ifdef LLVM_AVAILABLE
      if (pdevice->use_llvm && !disk_cache_get_function_identifier(LLVMInitializeAMDGPUTargetInfo, &ctx))
      #endif
         _mesa_sha1_update(&ctx, &family, sizeof(family));
   _mesa_sha1_update(&ctx, &conformant_trunc_coord, sizeof(conformant_trunc_coord));
   _mesa_sha1_update(&ctx, &ptr_size, sizeof(ptr_size));
            memcpy(uuid, sha1, VK_UUID_SIZE);
      }
      static void
   radv_get_driver_uuid(void *uuid)
   {
         }
      static void
   radv_get_device_uuid(const struct radeon_info *info, void *uuid)
   {
         }
      static void
   radv_physical_device_init_queue_table(struct radv_physical_device *pdevice)
   {
      int idx = 0;
   pdevice->vk_queue_to_radv[idx] = RADV_QUEUE_GENERAL;
            for (unsigned i = 1; i < RADV_MAX_QUEUE_FAMILIES; i++)
            if (pdevice->rad_info.ip[AMD_IP_COMPUTE].num_queues > 0 &&
      !(pdevice->instance->debug_flags & RADV_DEBUG_NO_COMPUTE_QUEUE)) {
   pdevice->vk_queue_to_radv[idx] = RADV_QUEUE_COMPUTE;
               if (pdevice->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE) {
      if (pdevice->rad_info.ip[pdevice->vid_decode_ip].num_queues > 0) {
      pdevice->vk_queue_to_radv[idx] = RADV_QUEUE_VIDEO_DEC;
         }
      }
      enum radv_heap {
      RADV_HEAP_VRAM = 1 << 0,
   RADV_HEAP_GTT = 1 << 1,
   RADV_HEAP_VRAM_VIS = 1 << 2,
      };
      static uint64_t
   radv_get_adjusted_vram_size(struct radv_physical_device *device)
   {
      int ov = driQueryOptioni(&device->instance->dri_options, "override_vram_size");
   if (ov >= 0)
            }
      static uint64_t
   radv_get_visible_vram_size(struct radv_physical_device *device)
   {
         }
      static uint64_t
   radv_get_vram_size(struct radv_physical_device *device)
   {
      uint64_t total_size = radv_get_adjusted_vram_size(device);
      }
      static void
   radv_physical_device_init_mem_types(struct radv_physical_device *device)
   {
      uint64_t visible_vram_size = radv_get_visible_vram_size(device);
   uint64_t vram_size = radv_get_vram_size(device);
   uint64_t gtt_size = (uint64_t)device->rad_info.gart_size_kb * 1024;
            device->memory_properties.memoryHeapCount = 0;
            if (!device->rad_info.has_dedicated_vram) {
               if (device->instance->enable_unified_heap_on_apu) {
      /* Some applications seem better when the driver exposes only one heap of VRAM on APUs. */
   visible_vram_size = total_size;
      } else {
      /* On APUs, the carveout is usually too small for games that request a minimum VRAM size
   * greater than it. To workaround this, we compute the total available memory size (GTT +
   * visible VRAM size) and report 2/3 as VRAM and 1/3 as GTT.
   */
   visible_vram_size = align64((total_size * 2) / 3, device->rad_info.gart_page_size);
                           /* Only get a VRAM heap if it is significant, not if it is a 16 MiB
   * remainder above visible VRAM. */
   if (vram_size > 0 && vram_size * 9 >= visible_vram_size) {
      vram_index = device->memory_properties.memoryHeapCount++;
   device->heaps |= RADV_HEAP_VRAM;
   device->memory_properties.memoryHeaps[vram_index] = (VkMemoryHeap){
      .size = vram_size,
                  if (gtt_size > 0) {
      gart_index = device->memory_properties.memoryHeapCount++;
   device->heaps |= RADV_HEAP_GTT;
   device->memory_properties.memoryHeaps[gart_index] = (VkMemoryHeap){
      .size = gtt_size,
                  if (visible_vram_size) {
      visible_vram_index = device->memory_properties.memoryHeapCount++;
   device->heaps |= RADV_HEAP_VRAM_VIS;
   device->memory_properties.memoryHeaps[visible_vram_index] = (VkMemoryHeap){
      .size = visible_vram_size,
                           if (vram_index >= 0 || visible_vram_index >= 0) {
      device->memory_domains[type_count] = RADEON_DOMAIN_VRAM;
   device->memory_flags[type_count] = RADEON_FLAG_NO_CPU_ACCESS;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               device->memory_domains[type_count] = RADEON_DOMAIN_VRAM;
   device->memory_flags[type_count] = RADEON_FLAG_NO_CPU_ACCESS | RADEON_FLAG_32BIT;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  if (gart_index >= 0) {
      device->memory_domains[type_count] = RADEON_DOMAIN_GTT;
   device->memory_flags[type_count] = RADEON_FLAG_GTT_WC | RADEON_FLAG_CPU_ACCESS;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
         }
   if (visible_vram_index >= 0) {
      device->memory_domains[type_count] = RADEON_DOMAIN_VRAM;
   device->memory_flags[type_count] = RADEON_FLAG_CPU_ACCESS;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                     device->memory_domains[type_count] = RADEON_DOMAIN_VRAM;
   device->memory_flags[type_count] = RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_32BIT;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        if (gart_index >= 0) {
      device->memory_domains[type_count] = RADEON_DOMAIN_GTT;
   device->memory_flags[type_count] = RADEON_FLAG_CPU_ACCESS;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                     device->memory_domains[type_count] = RADEON_DOMAIN_GTT;
   device->memory_flags[type_count] = RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_32BIT;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
               }
            if (device->rad_info.has_l2_uncached) {
      for (int i = 0; i < device->memory_properties.memoryTypeCount; i++) {
               if (((mem_type.propertyFlags & (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) ||
                                    device->memory_domains[type_count] = device->memory_domains[i];
   device->memory_flags[type_count] = device->memory_flags[i] | RADEON_FLAG_VA_UNCACHED;
   device->memory_properties.memoryTypes[type_count++] = (VkMemoryType){
      .propertyFlags = property_flags,
            }
               for (unsigned i = 0; i < type_count; ++i) {
      if (device->memory_flags[i] & RADEON_FLAG_32BIT)
         }
      uint32_t
   radv_find_memory_index(const struct radv_physical_device *pdevice, VkMemoryPropertyFlags flags)
   {
      const VkPhysicalDeviceMemoryProperties *mem_properties = &pdevice->memory_properties;
   for (uint32_t i = 0; i < mem_properties->memoryTypeCount; ++i) {
      if (mem_properties->memoryTypes[i].propertyFlags == flags) {
            }
      }
      static void
   radv_get_binning_settings(const struct radv_physical_device *pdevice, struct radv_binning_settings *settings)
   {
      settings->context_states_per_bin = 1;
   settings->persistent_states_per_bin = 1;
      }
      static void
   radv_physical_device_get_supported_extensions(const struct radv_physical_device *device,
         {
      *ext = (struct vk_device_extension_table){
      .KHR_8bit_storage = true,
   .KHR_16bit_storage = true,
   .KHR_acceleration_structure = radv_enable_rt(device, false),
   .KHR_cooperative_matrix = device->rad_info.gfx_level >= GFX11 && !device->use_llvm,
   .KHR_bind_memory2 = true,
   .KHR_buffer_device_address = true,
   .KHR_copy_commands2 = true,
   .KHR_create_renderpass2 = true,
   .KHR_dedicated_allocation = true,
   .KHR_deferred_host_operations = true,
   .KHR_depth_stencil_resolve = true,
   .KHR_descriptor_update_template = true,
   .KHR_device_group = true,
   .KHR_draw_indirect_count = true,
   .KHR_driver_properties = true,
   .KHR_dynamic_rendering = true,
   .KHR_external_fence = true,
   .KHR_external_fence_fd = true,
   .KHR_external_memory = true,
   .KHR_external_memory_fd = true,
   .KHR_external_semaphore = true,
   .KHR_external_semaphore_fd = true,
   .KHR_format_feature_flags2 = true,
   .KHR_fragment_shader_barycentric = device->rad_info.gfx_level >= GFX10_3,
   .KHR_fragment_shading_rate = device->rad_info.gfx_level >= GFX10_3,
   .KHR_get_memory_requirements2 = true,
   .KHR_global_priority = true,
   .KHR_image_format_list = true,
   #ifdef RADV_USE_WSI_PLATFORM
         #endif
         .KHR_maintenance1 = true,
   .KHR_maintenance2 = true,
   .KHR_maintenance3 = true,
   .KHR_maintenance4 = true,
   .KHR_maintenance5 = true,
   .KHR_map_memory2 = true,
   .KHR_multiview = true,
   .KHR_performance_query = radv_perf_query_supported(device),
   .KHR_pipeline_executable_properties = true,
   .KHR_pipeline_library = !device->use_llvm,
   /* Hide these behind dri configs for now since we cannot implement it reliably on
   * all surfaces yet. There is no surface capability query for present wait/id,
   * but the feature is useful enough to hide behind an opt-in mechanism for now.
   * If the instance only enables surface extensions that unconditionally support present wait,
   * we can also expose the extension that way. */
   .KHR_present_id = driQueryOptionb(&device->instance->dri_options, "vk_khr_present_wait") ||
         .KHR_present_wait = driQueryOptionb(&device->instance->dri_options, "vk_khr_present_wait") ||
         .KHR_push_descriptor = true,
   .KHR_ray_query = radv_enable_rt(device, false),
   .KHR_ray_tracing_maintenance1 = radv_enable_rt(device, false),
   .KHR_ray_tracing_pipeline = radv_enable_rt(device, true),
   .KHR_relaxed_block_layout = true,
   .KHR_sampler_mirror_clamp_to_edge = true,
   .KHR_sampler_ycbcr_conversion = true,
   .KHR_separate_depth_stencil_layouts = true,
   .KHR_shader_atomic_int64 = true,
   .KHR_shader_clock = true,
   .KHR_shader_draw_parameters = true,
   .KHR_shader_float16_int8 = true,
   .KHR_shader_float_controls = true,
   .KHR_shader_integer_dot_product = true,
   .KHR_shader_non_semantic_info = true,
   .KHR_shader_subgroup_extended_types = true,
   .KHR_shader_subgroup_uniform_control_flow = true,
   .KHR_shader_terminate_invocation = true,
   .KHR_spirv_1_4 = true,
   #ifdef RADV_USE_WSI_PLATFORM
         .KHR_swapchain = true,
   #endif
         .KHR_synchronization2 = true,
   .KHR_timeline_semaphore = true,
   .KHR_uniform_buffer_standard_layout = true,
   .KHR_variable_pointers = true,
   .KHR_video_queue = !!(device->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE),
   .KHR_video_decode_queue = !!(device->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE),
   .KHR_video_decode_h264 = VIDEO_CODEC_H264DEC && !!(device->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE),
   .KHR_video_decode_h265 = VIDEO_CODEC_H265DEC && !!(device->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE),
   .KHR_vulkan_memory_model = true,
   .KHR_workgroup_memory_explicit_layout = true,
   .KHR_zero_initialize_workgroup_memory = true,
   .EXT_4444_formats = true,
   .EXT_attachment_feedback_loop_dynamic_state = true,
   .EXT_attachment_feedback_loop_layout = true,
   .EXT_border_color_swizzle = device->rad_info.gfx_level >= GFX10,
   .EXT_buffer_device_address = true,
   .EXT_calibrated_timestamps = RADV_SUPPORT_CALIBRATED_TIMESTAMPS &&
         .EXT_color_write_enable = true,
   .EXT_conditional_rendering = true,
   .EXT_conservative_rasterization = device->rad_info.gfx_level >= GFX9,
   .EXT_custom_border_color = true,
   .EXT_debug_marker = device->instance->vk.trace_mode & RADV_TRACE_MODE_RGP,
   .EXT_depth_bias_control = true,
   .EXT_depth_clip_control = true,
   .EXT_depth_clip_enable = true,
   .EXT_depth_range_unrestricted = true,
   .EXT_descriptor_buffer = true,
   .EXT_descriptor_indexing = true,
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
         #endif
         .EXT_dynamic_rendering_unused_attachments = true,
   .EXT_extended_dynamic_state = true,
   .EXT_extended_dynamic_state2 = true,
   .EXT_extended_dynamic_state3 = true,
   .EXT_external_memory_acquire_unmodified = true,
   .EXT_external_memory_dma_buf = true,
   .EXT_external_memory_host = device->rad_info.has_userptr,
   .EXT_fragment_shader_interlock = radv_has_pops(device),
   .EXT_global_priority = true,
   .EXT_global_priority_query = true,
   .EXT_graphics_pipeline_library = !device->use_llvm && !(device->instance->debug_flags & RADV_DEBUG_NO_GPL),
   .EXT_host_query_reset = true,
   .EXT_image_2d_view_of_3d = true,
   .EXT_image_drm_format_modifier = device->rad_info.gfx_level >= GFX9,
   .EXT_image_robustness = true,
   .EXT_image_sliced_view_of_3d = device->rad_info.gfx_level >= GFX10,
   .EXT_image_view_min_lod = true,
   .EXT_index_type_uint8 = device->rad_info.gfx_level >= GFX8,
   .EXT_inline_uniform_block = true,
   .EXT_line_rasterization = true,
   .EXT_load_store_op_none = true,
   .EXT_memory_budget = true,
   .EXT_memory_priority = true,
   .EXT_mesh_shader = radv_taskmesh_enabled(device),
   .EXT_multi_draw = true,
   .EXT_mutable_descriptor_type = true, /* Trivial promotion from VALVE. */
   .EXT_non_seamless_cube_map = true,
   #ifndef _WIN32
         #endif
         .EXT_pipeline_creation_cache_control = true,
   .EXT_pipeline_creation_feedback = true,
   .EXT_pipeline_library_group_handles = radv_enable_rt(device, true),
   .EXT_pipeline_robustness = !device->use_llvm,
   .EXT_post_depth_coverage = device->rad_info.gfx_level >= GFX10,
   .EXT_primitive_topology_list_restart = true,
   .EXT_primitives_generated_query = true,
   .EXT_private_data = true,
   .EXT_provoking_vertex = true,
   .EXT_queue_family_foreign = true,
   .EXT_robustness2 = true,
   .EXT_sample_locations = device->rad_info.gfx_level < GFX10,
   .EXT_sampler_filter_minmax = true,
   .EXT_scalar_block_layout = device->rad_info.gfx_level >= GFX7,
   .EXT_separate_stencil_usage = true,
   .EXT_shader_atomic_float = true,
   .EXT_shader_atomic_float2 = true,
   .EXT_shader_demote_to_helper_invocation = true,
   .EXT_shader_image_atomic_int64 = true,
   .EXT_shader_module_identifier = true,
   .EXT_shader_stencil_export = true,
   .EXT_shader_subgroup_ballot = true,
   .EXT_shader_subgroup_vote = true,
   .EXT_shader_viewport_index_layer = true,
   #ifdef RADV_USE_WSI_PLATFORM
         #endif
         .EXT_texel_buffer_alignment = true,
   .EXT_tooling_info = true,
   .EXT_transform_feedback = true,
   .EXT_vertex_attribute_divisor = true,
   .EXT_vertex_input_dynamic_state = !device->use_llvm && !radv_NV_device_generated_commands_enabled(device),
   .EXT_ycbcr_image_arrays = true,
   .AMD_buffer_marker = true,
   .AMD_device_coherent_memory = true,
   .AMD_draw_indirect_count = true,
   .AMD_gcn_shader = true,
   .AMD_gpu_shader_half_float = device->rad_info.has_packed_math_16bit,
   .AMD_gpu_shader_int16 = device->rad_info.has_packed_math_16bit,
   .AMD_memory_overallocation_behavior = true,
   .AMD_mixed_attachment_samples = true,
   .AMD_rasterization_order = device->rad_info.has_out_of_order_rast,
   .AMD_shader_ballot = true,
   .AMD_shader_core_properties = true,
   .AMD_shader_core_properties2 = true,
   .AMD_shader_early_and_late_fragment_tests = true,
   .AMD_shader_explicit_vertex_parameter = true,
   .AMD_shader_fragment_mask = device->use_fmask,
   .AMD_shader_image_load_store_lod = true,
   .AMD_shader_trinary_minmax = true,
   #ifdef ANDROID
         .ANDROID_external_memory_android_hardware_buffer = RADV_SUPPORT_ANDROID_HARDWARE_BUFFER,
   #endif
         .GOOGLE_decorate_string = true,
   .GOOGLE_hlsl_functionality1 = true,
   .GOOGLE_user_type = true,
   .INTEL_shader_integer_functions2 = true,
   .NV_compute_shader_derivatives = true,
   .NV_device_generated_commands = radv_NV_device_generated_commands_enabled(device),
   .NV_device_generated_commands_compute = radv_NV_device_generated_commands_enabled(device),
   /* Undocumented extension purely for vkd3d-proton. This check is to prevent anyone else from
   * using it.
   */
   .VALVE_descriptor_set_host_mapping =
               }
      static void
   radv_physical_device_get_features(const struct radv_physical_device *pdevice, struct vk_features *features)
   {
      bool taskmesh_en = radv_taskmesh_enabled(pdevice);
   bool has_perf_query = radv_perf_query_supported(pdevice);
   bool has_shader_image_float_minmax = pdevice->rad_info.gfx_level != GFX8 && pdevice->rad_info.gfx_level != GFX9 &&
                  *features = (struct vk_features){
      /* Vulkan 1.0 */
   .robustBufferAccess = true,
   .fullDrawIndexUint32 = true,
   .imageCubeArray = true,
   .independentBlend = true,
   .geometryShader = true,
   .tessellationShader = true,
   .sampleRateShading = true,
   .dualSrcBlend = true,
   .logicOp = true,
   .multiDrawIndirect = true,
   .drawIndirectFirstInstance = true,
   .depthClamp = true,
   .depthBiasClamp = true,
   .fillModeNonSolid = true,
   .depthBounds = true,
   .wideLines = true,
   .largePoints = true,
   .alphaToOne = false,
   .multiViewport = true,
   .samplerAnisotropy = true,
   .textureCompressionETC2 = radv_device_supports_etc(pdevice) || pdevice->emulate_etc2,
   .textureCompressionASTC_LDR = pdevice->emulate_astc,
   .textureCompressionBC = true,
   .occlusionQueryPrecise = true,
   .pipelineStatisticsQuery = true,
   .vertexPipelineStoresAndAtomics = true,
   .fragmentStoresAndAtomics = true,
   .shaderTessellationAndGeometryPointSize = true,
   .shaderImageGatherExtended = true,
   .shaderStorageImageExtendedFormats = true,
   .shaderStorageImageMultisample = true,
   .shaderUniformBufferArrayDynamicIndexing = true,
   .shaderSampledImageArrayDynamicIndexing = true,
   .shaderStorageBufferArrayDynamicIndexing = true,
   .shaderStorageImageArrayDynamicIndexing = true,
   .shaderStorageImageReadWithoutFormat = true,
   .shaderStorageImageWriteWithoutFormat = true,
   .shaderClipDistance = true,
   .shaderCullDistance = true,
   .shaderFloat64 = true,
   .shaderInt64 = true,
   .shaderInt16 = true,
   .sparseBinding = true,
   .sparseResidencyBuffer = pdevice->rad_info.family >= CHIP_POLARIS10,
   .sparseResidencyImage2D = pdevice->rad_info.family >= CHIP_POLARIS10,
   .sparseResidencyImage3D = pdevice->rad_info.gfx_level >= GFX9,
   .sparseResidencyAliased = pdevice->rad_info.family >= CHIP_POLARIS10,
   .variableMultisampleRate = true,
   .shaderResourceMinLod = true,
   .shaderResourceResidency = true,
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess = true,
   .uniformAndStorageBuffer16BitAccess = true,
   .storagePushConstant16 = true,
   .storageInputOutput16 = pdevice->rad_info.has_packed_math_16bit,
   .multiview = true,
   .multiviewGeometryShader = true,
   .multiviewTessellationShader = true,
   .variablePointersStorageBuffer = true,
   .variablePointers = true,
   .protectedMemory = false,
   .samplerYcbcrConversion = true,
            /* Vulkan 1.2 */
   .samplerMirrorClampToEdge = true,
   .drawIndirectCount = true,
   .storageBuffer8BitAccess = true,
   .uniformAndStorageBuffer8BitAccess = true,
   .storagePushConstant8 = true,
   .shaderBufferInt64Atomics = true,
   .shaderSharedInt64Atomics = true,
   .shaderFloat16 = pdevice->rad_info.has_packed_math_16bit,
            .descriptorIndexing = true,
   .shaderInputAttachmentArrayDynamicIndexing = true,
   .shaderUniformTexelBufferArrayDynamicIndexing = true,
   .shaderStorageTexelBufferArrayDynamicIndexing = true,
   .shaderUniformBufferArrayNonUniformIndexing = true,
   .shaderSampledImageArrayNonUniformIndexing = true,
   .shaderStorageBufferArrayNonUniformIndexing = true,
   .shaderStorageImageArrayNonUniformIndexing = true,
   .shaderInputAttachmentArrayNonUniformIndexing = true,
   .shaderUniformTexelBufferArrayNonUniformIndexing = true,
   .shaderStorageTexelBufferArrayNonUniformIndexing = true,
   .descriptorBindingUniformBufferUpdateAfterBind = true,
   .descriptorBindingSampledImageUpdateAfterBind = true,
   .descriptorBindingStorageImageUpdateAfterBind = true,
   .descriptorBindingStorageBufferUpdateAfterBind = true,
   .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
   .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
   .descriptorBindingUpdateUnusedWhilePending = true,
   .descriptorBindingPartiallyBound = true,
   .descriptorBindingVariableDescriptorCount = true,
            .samplerFilterMinmax = true,
   .scalarBlockLayout = pdevice->rad_info.gfx_level >= GFX7,
   .imagelessFramebuffer = true,
   .uniformBufferStandardLayout = true,
   .shaderSubgroupExtendedTypes = true,
   .separateDepthStencilLayouts = true,
   .hostQueryReset = true,
   .timelineSemaphore = true,
   .bufferDeviceAddress = true,
   .bufferDeviceAddressCaptureReplay = true,
   .bufferDeviceAddressMultiDevice = false,
   .vulkanMemoryModel = true,
   .vulkanMemoryModelDeviceScope = true,
   .vulkanMemoryModelAvailabilityVisibilityChains = false,
   .shaderOutputViewportIndex = true,
   .shaderOutputLayer = true,
            /* Vulkan 1.3 */
   .robustImageAccess = true,
   .inlineUniformBlock = true,
   .descriptorBindingInlineUniformBlockUpdateAfterBind = true,
   .pipelineCreationCacheControl = true,
   .privateData = true,
   .shaderDemoteToHelperInvocation = true,
   .shaderTerminateInvocation = true,
   .subgroupSizeControl = true,
   .computeFullSubgroups = true,
   .synchronization2 = true,
   .textureCompressionASTC_HDR = false,
   .shaderZeroInitializeWorkgroupMemory = true,
   .dynamicRendering = true,
   .shaderIntegerDotProduct = true,
            /* VK_EXT_conditional_rendering */
   .conditionalRendering = true,
            /* VK_EXT_vertex_attribute_divisor */
   .vertexAttributeInstanceRateDivisor = true,
            /* VK_EXT_transform_feedback */
   .transformFeedback = true,
            /* VK_EXT_memory_priority */
            /* VK_EXT_depth_clip_enable */
            /* VK_NV_compute_shader_derivatives */
   .computeDerivativeGroupQuads = false,
            /* VK_EXT_ycbcr_image_arrays */
            /* VK_EXT_index_type_uint8 */
            /* VK_KHR_pipeline_executable_properties */
            /* VK_KHR_shader_clock */
   .shaderSubgroupClock = true,
            /* VK_EXT_texel_buffer_alignment */
            /* VK_AMD_device_coherent_memory */
            /* VK_EXT_line_rasterization */
   .rectangularLines = true,
   .bresenhamLines = true,
   .smoothLines = true,
   .stippledRectangularLines = false,
   /* FIXME: Some stippled Bresenham CTS fails on Vega10
   * but work on Raven.
   */
   .stippledBresenhamLines = pdevice->rad_info.gfx_level != GFX9,
            /* VK_EXT_robustness2 */
   .robustBufferAccess2 = true,
   .robustImageAccess2 = true,
            /* VK_EXT_custom_border_color */
   .customBorderColors = true,
            /* VK_EXT_extended_dynamic_state */
            /* VK_EXT_shader_atomic_float */
   .shaderBufferFloat32Atomics = true,
   .shaderBufferFloat32AtomicAdd = pdevice->rad_info.gfx_level >= GFX11,
   .shaderBufferFloat64Atomics = true,
   .shaderBufferFloat64AtomicAdd = false,
   .shaderSharedFloat32Atomics = true,
   .shaderSharedFloat32AtomicAdd = pdevice->rad_info.gfx_level >= GFX8,
   .shaderSharedFloat64Atomics = true,
   .shaderSharedFloat64AtomicAdd = false,
   .shaderImageFloat32Atomics = true,
   .shaderImageFloat32AtomicAdd = false,
   .sparseImageFloat32Atomics = true,
            /* VK_EXT_4444_formats */
   .formatA4R4G4B4 = true,
            /* VK_EXT_shader_image_atomic_int64 */
   .shaderImageInt64Atomics = true,
            /* VK_EXT_mutable_descriptor_type */
            /* VK_KHR_fragment_shading_rate */
   .pipelineFragmentShadingRate = true,
   .primitiveFragmentShadingRate = true,
            /* VK_KHR_workgroup_memory_explicit_layout */
   .workgroupMemoryExplicitLayout = true,
   .workgroupMemoryExplicitLayoutScalarBlockLayout = true,
   .workgroupMemoryExplicitLayout8BitAccess = true,
            /* VK_EXT_provoking_vertex */
   .provokingVertexLast = true,
            /* VK_EXT_extended_dynamic_state2 */
   .extendedDynamicState2 = true,
   .extendedDynamicState2LogicOp = true,
            /* VK_EXT_global_priority_query */
            /* VK_KHR_acceleration_structure */
   .accelerationStructure = true,
   .accelerationStructureCaptureReplay = true,
   .accelerationStructureIndirectBuild = false,
   .accelerationStructureHostCommands = false,
            /* VK_EXT_buffer_device_address */
            /* VK_KHR_shader_subgroup_uniform_control_flow */
            /* VK_EXT_multi_draw */
            /* VK_EXT_color_write_enable */
            /* VK_EXT_shader_atomic_float2 */
   .shaderBufferFloat16Atomics = false,
   .shaderBufferFloat16AtomicAdd = false,
   .shaderBufferFloat16AtomicMinMax = false,
   .shaderBufferFloat32AtomicMinMax = radv_has_shader_buffer_float_minmax(pdevice, 32),
   .shaderBufferFloat64AtomicMinMax = radv_has_shader_buffer_float_minmax(pdevice, 64),
   .shaderSharedFloat16Atomics = false,
   .shaderSharedFloat16AtomicAdd = false,
   .shaderSharedFloat16AtomicMinMax = false,
   .shaderSharedFloat32AtomicMinMax = true,
   .shaderSharedFloat64AtomicMinMax = true,
   .shaderImageFloat32AtomicMinMax = has_shader_image_float_minmax,
            /* VK_KHR_present_id */
            /* VK_KHR_present_wait */
            /* VK_EXT_primitive_topology_list_restart */
   .primitiveTopologyListRestart = true,
            /* VK_KHR_ray_query */
            /* VK_EXT_pipeline_library_group_handles */
            /* VK_KHR_ray_tracing_pipeline */
   .rayTracingPipeline = true,
   .rayTracingPipelineShaderGroupHandleCaptureReplay = true,
   .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false,
   .rayTracingPipelineTraceRaysIndirect = true,
            /* VK_KHR_ray_tracing_maintenance1 */
   .rayTracingMaintenance1 = true,
            /* VK_EXT_vertex_input_dynamic_state */
            /* VK_EXT_image_view_min_lod */
            /* VK_EXT_mesh_shader */
   .meshShader = taskmesh_en,
   .taskShader = taskmesh_en,
   .multiviewMeshShader = taskmesh_en,
   .primitiveFragmentShadingRateMeshShader = taskmesh_en,
            /* VK_VALVE_descriptor_set_host_mapping */
            /* VK_EXT_depth_clip_control */
            /* VK_EXT_image_2d_view_of_3d  */
   .image2DViewOf3D = true,
            /* VK_INTEL_shader_integer_functions2 */
            /* VK_EXT_primitives_generated_query */
   .primitivesGeneratedQuery = true,
   .primitivesGeneratedQueryWithRasterizerDiscard = true,
            /* VK_EXT_non_seamless_cube_map */
            /* VK_EXT_border_color_swizzle */
   .borderColorSwizzle = true,
            /* VK_EXT_shader_module_identifier */
            /* VK_KHR_performance_query */
   .performanceCounterQueryPools = has_perf_query,
            /* VK_NV_device_generated_commands */
            /* VK_EXT_attachment_feedback_loop_layout */
            /* VK_EXT_graphics_pipeline_library */
            /* VK_EXT_extended_dynamic_state3 */
   .extendedDynamicState3TessellationDomainOrigin = true,
   .extendedDynamicState3PolygonMode = true,
   .extendedDynamicState3SampleMask = true,
   .extendedDynamicState3AlphaToCoverageEnable = pdevice->rad_info.gfx_level < GFX11 && !pdevice->use_llvm,
   .extendedDynamicState3LogicOpEnable = true,
   .extendedDynamicState3LineStippleEnable = true,
   .extendedDynamicState3ColorBlendEnable = !pdevice->use_llvm,
   .extendedDynamicState3DepthClipEnable = true,
   .extendedDynamicState3ConservativeRasterizationMode = pdevice->rad_info.gfx_level >= GFX9,
   .extendedDynamicState3DepthClipNegativeOneToOne = true,
   .extendedDynamicState3ProvokingVertexMode = true,
   .extendedDynamicState3DepthClampEnable = true,
   .extendedDynamicState3ColorWriteMask = !pdevice->use_llvm,
   .extendedDynamicState3RasterizationSamples = true,
   .extendedDynamicState3ColorBlendEquation = !pdevice->use_llvm,
   .extendedDynamicState3SampleLocationsEnable = pdevice->rad_info.gfx_level < GFX10,
   .extendedDynamicState3LineRasterizationMode = true,
   .extendedDynamicState3ExtraPrimitiveOverestimationSize = false,
   .extendedDynamicState3AlphaToOneEnable = false,
   .extendedDynamicState3RasterizationStream = false,
   .extendedDynamicState3ColorBlendAdvanced = false,
   .extendedDynamicState3ViewportWScalingEnable = false,
   .extendedDynamicState3ViewportSwizzle = false,
   .extendedDynamicState3CoverageToColorEnable = false,
   .extendedDynamicState3CoverageToColorLocation = false,
   .extendedDynamicState3CoverageModulationMode = false,
   .extendedDynamicState3CoverageModulationTableEnable = false,
   .extendedDynamicState3CoverageModulationTable = false,
   .extendedDynamicState3CoverageReductionMode = false,
   .extendedDynamicState3RepresentativeFragmentTestEnable = false,
            /* VK_EXT_descriptor_buffer */
   .descriptorBuffer = true,
   .descriptorBufferCaptureReplay = false,
   .descriptorBufferImageLayoutIgnored = true,
            /* VK_AMD_shader_early_and_late_fragment_tests */
            /* VK_EXT_image_sliced_view_of_3d */
      #ifdef RADV_USE_WSI_PLATFORM
         /* VK_EXT_swapchain_maintenance1 */
   #endif
            /* VK_EXT_attachment_feedback_loop_dynamic_state */
            /* VK_EXT_dynamic_rendering_unused_attachments */
            /* VK_KHR_fragment_shader_barycentric */
            /* VK_EXT_depth_bias_control */
   .depthBiasControl = true,
   .leastRepresentableValueForceUnormRepresentation = true,
   .floatRepresentation = true,
            /* VK_EXT_fragment_shader_interlock */
   .fragmentShaderSampleInterlock = has_fragment_shader_interlock,
   .fragmentShaderPixelInterlock = has_fragment_shader_interlock,
            /* VK_EXT_pipeline_robustness */
            /* VK_KHR_maintenance5 */
            /* VK_NV_device_generated_commands_compute */
   .deviceGeneratedCompute = true,
   .deviceGeneratedComputePipelines = false,
            /* VK_KHR_cooperative_matrix */
   .cooperativeMatrix = pdevice->rad_info.gfx_level >= GFX11 && !pdevice->use_llvm,
         }
      static size_t
   radv_max_descriptor_set_size()
   {
      /* make sure that the entire descriptor set is addressable with a signed
   * 32-bit int. So the sum of all limits scaled by descriptor size has to
   * be at most 2 GiB. the combined image & samples object count as one of
   * both. This limit is for the pipeline layout, not for the set layout, but
   * there is no set limit, so we just set a pipeline limit. I don't think
   * any app is going to hit this soon. */
   return ((1ull << 31) - 16 * MAX_DYNAMIC_BUFFERS - MAX_INLINE_UNIFORM_BLOCK_SIZE * MAX_INLINE_UNIFORM_BLOCK_COUNT) /
         (32 /* uniform buffer, 32 due to potential space wasted on alignment */ +
      }
      static uint32_t
   radv_uniform_buffer_offset_alignment(const struct radv_physical_device *pdevice)
   {
      uint32_t uniform_offset_alignment =
         if (!util_is_power_of_two_or_zero(uniform_offset_alignment)) {
      fprintf(stderr,
         "ERROR: invalid radv_override_uniform_offset_alignment setting %d:"
   "not a power of two\n",
               /* Take at least the hardware limit. */
      }
      static const char *
   radv_get_compiler_string(struct radv_physical_device *pdevice)
   {
      if (!pdevice->use_llvm) {
      /* Some games like SotTR apply shader workarounds if the LLVM
   * version is too old or if the LLVM version string is
   * missing. This gives 2-5% performance with SotTR and ACO.
   */
   if (driQueryOptionb(&pdevice->instance->dri_options, "radv_report_llvm9_version_string")) {
                           #ifdef LLVM_AVAILABLE
         #else
         #endif
   }
      static void
   radv_get_physical_device_properties(struct radv_physical_device *pdevice)
   {
                        VkPhysicalDeviceType device_type;
   if (pdevice->rad_info.has_dedicated_vram) {
         } else {
                  pdevice->vk.properties = (struct vk_properties){
      .apiVersion = RADV_API_VERSION,
   .driverVersion = vk_get_driver_version(),
   .vendorID = ATI_VENDOR_ID,
   .deviceID = pdevice->rad_info.pci_id,
   .deviceType = device_type,
   .maxImageDimension1D = (1 << 14),
   .maxImageDimension2D = (1 << 14),
   .maxImageDimension3D = (1 << 11),
   .maxImageDimensionCube = (1 << 14),
   .maxImageArrayLayers = (1 << 11),
   .maxTexelBufferElements = UINT32_MAX,
   .maxUniformBufferRange = UINT32_MAX,
   .maxStorageBufferRange = UINT32_MAX,
   .maxPushConstantsSize = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount = UINT32_MAX,
   .maxSamplerAllocationCount = 64 * 1024,
   .bufferImageGranularity = 1,
   .sparseAddressSpaceSize = RADV_MAX_MEMORY_ALLOCATION_SIZE, /* buffer max size */
   .maxBoundDescriptorSets = MAX_SETS,
   .maxPerStageDescriptorSamplers = max_descriptor_set_size,
   .maxPerStageDescriptorUniformBuffers = max_descriptor_set_size,
   .maxPerStageDescriptorStorageBuffers = max_descriptor_set_size,
   .maxPerStageDescriptorSampledImages = max_descriptor_set_size,
   .maxPerStageDescriptorStorageImages = max_descriptor_set_size,
   .maxPerStageDescriptorInputAttachments = max_descriptor_set_size,
   .maxPerStageResources = max_descriptor_set_size,
   .maxDescriptorSetSamplers = max_descriptor_set_size,
   .maxDescriptorSetUniformBuffers = max_descriptor_set_size,
   .maxDescriptorSetUniformBuffersDynamic = MAX_DYNAMIC_UNIFORM_BUFFERS,
   .maxDescriptorSetStorageBuffers = max_descriptor_set_size,
   .maxDescriptorSetStorageBuffersDynamic = MAX_DYNAMIC_STORAGE_BUFFERS,
   .maxDescriptorSetSampledImages = max_descriptor_set_size,
   .maxDescriptorSetStorageImages = max_descriptor_set_size,
   .maxDescriptorSetInputAttachments = max_descriptor_set_size,
   .maxVertexInputAttributes = MAX_VERTEX_ATTRIBS,
   .maxVertexInputBindings = MAX_VBS,
   .maxVertexInputAttributeOffset = UINT32_MAX,
   .maxVertexInputBindingStride = 2048,
   .maxVertexOutputComponents = 128,
   .maxTessellationGenerationLevel = 64,
   .maxTessellationPatchSize = 32,
   .maxTessellationControlPerVertexInputComponents = 128,
   .maxTessellationControlPerVertexOutputComponents = 128,
   .maxTessellationControlPerPatchOutputComponents = 120,
   .maxTessellationControlTotalOutputComponents = 4096,
   .maxTessellationEvaluationInputComponents = 128,
   .maxTessellationEvaluationOutputComponents = 128,
   .maxGeometryShaderInvocations = 127,
   .maxGeometryInputComponents = 64,
   .maxGeometryOutputComponents = 128,
   .maxGeometryOutputVertices = 256,
   .maxGeometryTotalOutputComponents = 1024,
   .maxFragmentInputComponents = 128,
   .maxFragmentOutputAttachments = 8,
   .maxFragmentDualSrcAttachments = 1,
   .maxFragmentCombinedOutputResources = max_descriptor_set_size,
   .maxComputeSharedMemorySize = pdevice->max_shared_size,
   .maxComputeWorkGroupCount = {65535, 65535, 65535},
   .maxComputeWorkGroupInvocations = 1024,
   .maxComputeWorkGroupSize = {1024, 1024, 1024},
   .subPixelPrecisionBits = 8,
   .subTexelPrecisionBits = 8,
   .mipmapPrecisionBits = 8,
   .maxDrawIndexedIndexValue = UINT32_MAX,
   .maxDrawIndirectCount = UINT32_MAX,
   .maxSamplerLodBias = 16,
   .maxSamplerAnisotropy = 16,
   .maxViewports = MAX_VIEWPORTS,
   .maxViewportDimensions = {(1 << 14), (1 << 14)},
   .viewportBoundsRange = {INT16_MIN, INT16_MAX},
   .viewportSubPixelBits = 8,
   .minMemoryMapAlignment = 4096, /* A page */
   .minTexelBufferOffsetAlignment = 4,
   .minUniformBufferOffsetAlignment = radv_uniform_buffer_offset_alignment(pdevice),
   .minStorageBufferOffsetAlignment = 4,
   .minTexelOffset = -32,
   .maxTexelOffset = 31,
   .minTexelGatherOffset = -32,
   .maxTexelGatherOffset = 31,
   .minInterpolationOffset = -2,
   .maxInterpolationOffset = 2,
   .subPixelInterpolationOffsetBits = 8,
   .maxFramebufferWidth = MAX_FRAMEBUFFER_WIDTH,
   .maxFramebufferHeight = MAX_FRAMEBUFFER_HEIGHT,
   .maxFramebufferLayers = (1 << 10),
   .framebufferColorSampleCounts = sample_counts,
   .framebufferDepthSampleCounts = sample_counts,
   .framebufferStencilSampleCounts = sample_counts,
   .framebufferNoAttachmentsSampleCounts = sample_counts,
   .maxColorAttachments = MAX_RTS,
   .sampledImageColorSampleCounts = sample_counts,
   .sampledImageIntegerSampleCounts = sample_counts,
   .sampledImageDepthSampleCounts = sample_counts,
   .sampledImageStencilSampleCounts = sample_counts,
   .storageImageSampleCounts = sample_counts,
   .maxSampleMaskWords = 1,
   .timestampComputeAndGraphics = true,
   .timestampPeriod = 1000000.0 / pdevice->rad_info.clock_crystal_freq,
   .maxClipDistances = 8,
   .maxCullDistances = 8,
   .maxCombinedClipAndCullDistances = 8,
   .discreteQueuePriorities = 2,
   .pointSizeRange = {0.0, 8191.875},
   .lineWidthRange = {0.0, 8.0},
   .pointSizeGranularity = (1.0 / 8.0),
   .lineWidthGranularity = (1.0 / 8.0),
   .strictLines = false, /* FINISHME */
   .standardSampleLocations = true,
   .optimalBufferCopyOffsetAlignment = 1,
   .optimalBufferCopyRowPitchAlignment = 1,
   .nonCoherentAtomSize = 64,
   .sparseResidencyNonResidentStrict = pdevice->rad_info.family >= CHIP_POLARIS10,
   .sparseResidencyStandard2DBlockShape = pdevice->rad_info.family >= CHIP_POLARIS10,
                        /* Vulkan 1.1 */
   strcpy(p->deviceName, pdevice->marketing_name);
            memcpy(p->deviceUUID, pdevice->device_uuid, VK_UUID_SIZE);
   memcpy(p->driverUUID, pdevice->driver_uuid, VK_UUID_SIZE);
   memset(p->deviceLUID, 0, VK_LUID_SIZE);
   /* The LUID is for Windows. */
   p->deviceLUIDValid = false;
            p->subgroupSize = RADV_SUBGROUP_SIZE;
   p->subgroupSupportedStages = VK_SHADER_STAGE_ALL_GRAPHICS | VK_SHADER_STAGE_COMPUTE_BIT;
   if (radv_taskmesh_enabled(pdevice))
            if (radv_enable_rt(pdevice, true))
         p->subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT |
                              p->pointClippingBehavior = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES;
   p->maxMultiviewViewCount = MAX_VIEWS;
   p->maxMultiviewInstanceIndex = INT_MAX;
   p->protectedNoFault = false;
   p->maxPerSetDescriptors = RADV_MAX_PER_SET_DESCRIPTORS;
            /* Vulkan 1.2 */
   p->driverID = VK_DRIVER_ID_MESA_RADV;
   snprintf(p->driverName, VK_MAX_DRIVER_NAME_SIZE, "radv");
   snprintf(p->driverInfo, VK_MAX_DRIVER_INFO_SIZE, "Mesa " PACKAGE_VERSION MESA_GIT_SHA1 "%s",
            if (radv_is_conformant(pdevice)) {
      if (pdevice->rad_info.gfx_level >= GFX10_3) {
      p->conformanceVersion = (VkConformanceVersion){
      .major = 1,
   .minor = 3,
   .subminor = 0,
         } else {
      p->conformanceVersion = (VkConformanceVersion){
      .major = 1,
   .minor = 2,
   .subminor = 7,
            } else {
      p->conformanceVersion = (VkConformanceVersion){
      .major = 0,
   .minor = 0,
   .subminor = 0,
                  /* On AMD hardware, denormals and rounding modes for fp16/fp64 are
   * controlled by the same config register.
   */
   if (pdevice->rad_info.has_packed_math_16bit) {
      p->denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_32_BIT_ONLY;
      } else {
      p->denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL;
               /* With LLVM, do not allow both preserving and flushing denorms because
   * different shaders in the same pipeline can have different settings and
   * this won't work for merged shaders. To make it work, this requires LLVM
   * support for changing the register. The same logic applies for the
   * rounding modes because they are configured with the same config
   * register.
   */
   p->shaderDenormFlushToZeroFloat32 = true;
   p->shaderDenormPreserveFloat32 = !pdevice->use_llvm;
   p->shaderRoundingModeRTEFloat32 = true;
   p->shaderRoundingModeRTZFloat32 = !pdevice->use_llvm;
            p->shaderDenormFlushToZeroFloat16 = pdevice->rad_info.has_packed_math_16bit && !pdevice->use_llvm;
   p->shaderDenormPreserveFloat16 = pdevice->rad_info.has_packed_math_16bit;
   p->shaderRoundingModeRTEFloat16 = pdevice->rad_info.has_packed_math_16bit;
   p->shaderRoundingModeRTZFloat16 = pdevice->rad_info.has_packed_math_16bit && !pdevice->use_llvm;
            p->shaderDenormFlushToZeroFloat64 = pdevice->rad_info.gfx_level >= GFX8 && !pdevice->use_llvm;
   p->shaderDenormPreserveFloat64 = pdevice->rad_info.gfx_level >= GFX8;
   p->shaderRoundingModeRTEFloat64 = pdevice->rad_info.gfx_level >= GFX8;
   p->shaderRoundingModeRTZFloat64 = pdevice->rad_info.gfx_level >= GFX8 && !pdevice->use_llvm;
            p->maxUpdateAfterBindDescriptorsInAllPools = UINT32_MAX / 64;
   p->shaderUniformBufferArrayNonUniformIndexingNative = false;
   p->shaderSampledImageArrayNonUniformIndexingNative = false;
   p->shaderStorageBufferArrayNonUniformIndexingNative = false;
   p->shaderStorageImageArrayNonUniformIndexingNative = false;
   p->shaderInputAttachmentArrayNonUniformIndexingNative = false;
   p->robustBufferAccessUpdateAfterBind = true;
            p->maxPerStageDescriptorUpdateAfterBindSamplers = max_descriptor_set_size;
   p->maxPerStageDescriptorUpdateAfterBindUniformBuffers = max_descriptor_set_size;
   p->maxPerStageDescriptorUpdateAfterBindStorageBuffers = max_descriptor_set_size;
   p->maxPerStageDescriptorUpdateAfterBindSampledImages = max_descriptor_set_size;
   p->maxPerStageDescriptorUpdateAfterBindStorageImages = max_descriptor_set_size;
   p->maxPerStageDescriptorUpdateAfterBindInputAttachments = max_descriptor_set_size;
   p->maxPerStageUpdateAfterBindResources = max_descriptor_set_size;
   p->maxDescriptorSetUpdateAfterBindSamplers = max_descriptor_set_size;
   p->maxDescriptorSetUpdateAfterBindUniformBuffers = max_descriptor_set_size;
   p->maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = MAX_DYNAMIC_UNIFORM_BUFFERS;
   p->maxDescriptorSetUpdateAfterBindStorageBuffers = max_descriptor_set_size;
   p->maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = MAX_DYNAMIC_STORAGE_BUFFERS;
   p->maxDescriptorSetUpdateAfterBindSampledImages = max_descriptor_set_size;
   p->maxDescriptorSetUpdateAfterBindStorageImages = max_descriptor_set_size;
            /* We support all of the depth resolve modes */
   p->supportedDepthResolveModes =
            /* Average doesn't make sense for stencil so we don't support that */
   p->supportedStencilResolveModes =
            p->independentResolveNone = true;
            /* GFX6-8 only support single channel min/max filter. */
   p->filterMinmaxImageComponentMapping = pdevice->rad_info.gfx_level >= GFX9;
                              /* Vulkan 1.3 */
   p->minSubgroupSize = 64;
   p->maxSubgroupSize = 64;
   p->maxComputeWorkgroupSubgroups = UINT32_MAX;
   p->requiredSubgroupSizeStages = 0;
   if (pdevice->rad_info.gfx_level >= GFX10) {
      /* Only GFX10+ supports wave32. */
   p->minSubgroupSize = 32;
            if (radv_taskmesh_enabled(pdevice)) {
                     p->maxInlineUniformBlockSize = MAX_INLINE_UNIFORM_BLOCK_SIZE;
   p->maxPerStageDescriptorInlineUniformBlocks = MAX_INLINE_UNIFORM_BLOCK_SIZE * MAX_SETS;
   p->maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks = MAX_INLINE_UNIFORM_BLOCK_SIZE * MAX_SETS;
   p->maxDescriptorSetInlineUniformBlocks = MAX_INLINE_UNIFORM_BLOCK_COUNT;
   p->maxDescriptorSetUpdateAfterBindInlineUniformBlocks = MAX_INLINE_UNIFORM_BLOCK_COUNT;
            bool accel_dot = pdevice->rad_info.has_accelerated_dot_product;
   bool gfx11plus = pdevice->rad_info.gfx_level >= GFX11;
   p->integerDotProduct8BitUnsignedAccelerated = accel_dot;
   p->integerDotProduct8BitSignedAccelerated = accel_dot;
   p->integerDotProduct8BitMixedSignednessAccelerated = accel_dot && gfx11plus;
   p->integerDotProduct4x8BitPackedUnsignedAccelerated = accel_dot;
   p->integerDotProduct4x8BitPackedSignedAccelerated = accel_dot;
   p->integerDotProduct4x8BitPackedMixedSignednessAccelerated = accel_dot && gfx11plus;
   p->integerDotProduct16BitUnsignedAccelerated = accel_dot && !gfx11plus;
   p->integerDotProduct16BitSignedAccelerated = accel_dot && !gfx11plus;
   p->integerDotProduct16BitMixedSignednessAccelerated = false;
   p->integerDotProduct32BitUnsignedAccelerated = false;
   p->integerDotProduct32BitSignedAccelerated = false;
   p->integerDotProduct32BitMixedSignednessAccelerated = false;
   p->integerDotProduct64BitUnsignedAccelerated = false;
   p->integerDotProduct64BitSignedAccelerated = false;
   p->integerDotProduct64BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating8BitUnsignedAccelerated = accel_dot;
   p->integerDotProductAccumulatingSaturating8BitSignedAccelerated = accel_dot;
   p->integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated = accel_dot && gfx11plus;
   p->integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated = accel_dot;
   p->integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated = accel_dot;
   p->integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated = accel_dot && gfx11plus;
   p->integerDotProductAccumulatingSaturating16BitUnsignedAccelerated = accel_dot && !gfx11plus;
   p->integerDotProductAccumulatingSaturating16BitSignedAccelerated = accel_dot && !gfx11plus;
   p->integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitSignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating64BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating64BitSignedAccelerated = false;
            p->storageTexelBufferOffsetAlignmentBytes = 4;
   p->storageTexelBufferOffsetSingleTexelAlignment = true;
   p->uniformTexelBufferOffsetAlignmentBytes = 4;
                     /* VK_KHR_push_descriptor */
            /* VK_EXT_discard_rectangles */
            /* VK_EXT_external_memory_host */
            /* VK_AMD_shader_core_properties */
   /* Shader engines. */
   p->shaderEngineCount = pdevice->rad_info.max_se;
   p->shaderArraysPerEngineCount = pdevice->rad_info.max_sa_per_se;
   p->computeUnitsPerShaderArray = pdevice->rad_info.min_good_cu_per_sa;
   p->simdPerComputeUnit = pdevice->rad_info.num_simd_per_compute_unit;
   p->wavefrontsPerSimd = pdevice->rad_info.max_wave64_per_simd;
            /* SGPR. */
   p->sgprsPerSimd = pdevice->rad_info.num_physical_sgprs_per_simd;
   p->minSgprAllocation = pdevice->rad_info.min_sgpr_alloc;
   p->maxSgprAllocation = pdevice->rad_info.max_sgpr_alloc;
            /* VGPR. */
   p->vgprsPerSimd = pdevice->rad_info.num_physical_wave64_vgprs_per_simd;
   p->minVgprAllocation = pdevice->rad_info.min_wave64_vgpr_alloc;
   p->maxVgprAllocation = pdevice->rad_info.max_vgpr_alloc;
            /* VK_AMD_shader_core_properties2 */
   p->shaderCoreFeatures = 0;
            /* VK_EXT_vertex_attribute_divisor */
            /* VK_EXT_conservative_rasterization */
   p->primitiveOverestimationSize = 0;
   p->maxExtraPrimitiveOverestimationSize = 0;
   p->extraPrimitiveOverestimationSizeGranularity = 0;
   p->primitiveUnderestimation = true;
   p->conservativePointAndLineRasterization = false;
   p->degenerateTrianglesRasterized = true;
   p->degenerateLinesRasterized = false;
   p->fullyCoveredFragmentShaderInputVariable = true;
               #ifndef _WIN32
      p->pciDomain = pdevice->bus_info.domain;
   p->pciBus = pdevice->bus_info.bus;
   p->pciDevice = pdevice->bus_info.dev;
      #endif
         /* VK_EXT_transform_feedback */
   p->maxTransformFeedbackStreams = MAX_SO_STREAMS;
   p->maxTransformFeedbackBuffers = MAX_SO_BUFFERS;
   p->maxTransformFeedbackBufferSize = UINT32_MAX;
   p->maxTransformFeedbackStreamDataSize = 512;
   p->maxTransformFeedbackBufferDataSize = 512;
   p->maxTransformFeedbackBufferDataStride = 512;
   p->transformFeedbackQueries = true;
   p->transformFeedbackStreamsLinesTriangles = true;
   p->transformFeedbackRasterizationStreamSelect = false;
            /* VK_EXT_sample_locations */
   p->sampleLocationSampleCounts = VK_SAMPLE_COUNT_2_BIT | VK_SAMPLE_COUNT_4_BIT | VK_SAMPLE_COUNT_8_BIT;
   p->maxSampleLocationGridSize = (VkExtent2D){2, 2};
   p->sampleLocationCoordinateRange[0] = 0.0f;
   p->sampleLocationCoordinateRange[1] = 0.9375f;
   p->sampleLocationSubPixelBits = 4;
            /* VK_EXT_line_rasterization */
            /* VK_EXT_robustness2 */
   p->robustStorageBufferAccessSizeAlignment = 4;
            /* VK_EXT_custom_border_color */
            /* VK_KHR_fragment_shading_rate */
   if (radv_vrs_attachment_enabled(pdevice)) {
      p->minFragmentShadingRateAttachmentTexelSize = (VkExtent2D){8, 8};
      } else {
      p->minFragmentShadingRateAttachmentTexelSize = (VkExtent2D){0, 0};
      }
   p->maxFragmentShadingRateAttachmentTexelSizeAspectRatio = 1;
   p->primitiveFragmentShadingRateWithMultipleViewports = true;
   p->layeredShadingRateAttachments = false; /* TODO */
   p->fragmentShadingRateNonTrivialCombinerOps = true;
   p->maxFragmentSize = (VkExtent2D){2, 2};
   p->maxFragmentSizeAspectRatio = 2;
   p->maxFragmentShadingRateCoverageSamples = 32;
   p->maxFragmentShadingRateRasterizationSamples = VK_SAMPLE_COUNT_8_BIT;
   p->fragmentShadingRateWithShaderDepthStencilWrites = !pdevice->rad_info.has_vrs_ds_export_bug;
   p->fragmentShadingRateWithSampleMask = true;
   p->fragmentShadingRateWithShaderSampleMask = false;
   p->fragmentShadingRateWithConservativeRasterization = true;
   p->fragmentShadingRateWithFragmentShaderInterlock = pdevice->rad_info.gfx_level >= GFX11 && radv_has_pops(pdevice);
   p->fragmentShadingRateWithCustomSampleLocations = false;
            /* VK_EXT_provoking_vertex */
   p->provokingVertexModePerPipeline = true;
            /* VK_KHR_acceleration_structure */
   p->maxGeometryCount = (1 << 24) - 1;
   p->maxInstanceCount = (1 << 24) - 1;
   p->maxPrimitiveCount = (1 << 29) - 1;
   p->maxPerStageDescriptorAccelerationStructures = p->maxPerStageDescriptorStorageBuffers;
   p->maxPerStageDescriptorUpdateAfterBindAccelerationStructures = p->maxPerStageDescriptorStorageBuffers;
   p->maxDescriptorSetAccelerationStructures = p->maxDescriptorSetStorageBuffers;
   p->maxDescriptorSetUpdateAfterBindAccelerationStructures = p->maxDescriptorSetStorageBuffers;
               #ifndef _WIN32
      if (pdevice->available_nodes & (1 << DRM_NODE_PRIMARY)) {
      p->drmHasPrimary = true;
   p->drmPrimaryMajor = (int64_t)major(pdevice->primary_devid);
      } else {
         }
   if (pdevice->available_nodes & (1 << DRM_NODE_RENDER)) {
      p->drmHasRender = true;
   p->drmRenderMajor = (int64_t)major(pdevice->render_devid);
      } else {
            #endif
         /* VK_EXT_multi_draw */
                     p->shaderGroupHandleSize = RADV_RT_HANDLE_SIZE;
   p->maxRayRecursionDepth = 31;    /* Minimum allowed for DXR. */
   p->maxShaderGroupStride = 16384; /* dummy */
   /* This isn't strictly necessary, but Doom Eternal breaks if the
   * alignment is any lower. */
   p->shaderGroupBaseAlignment = RADV_RT_HANDLE_SIZE;
   p->shaderGroupHandleCaptureReplaySize = sizeof(struct radv_rt_capture_replay_handle);
   p->maxRayDispatchInvocationCount = 1024 * 1024 * 64;
   p->shaderGroupHandleAlignment = 16;
            /* VK_EXT_shader_module_identifier */
   STATIC_ASSERT(sizeof(vk_shaderModuleIdentifierAlgorithmUUID) == sizeof(p->shaderModuleIdentifierAlgorithmUUID));
   memcpy(p->shaderModuleIdentifierAlgorithmUUID, vk_shaderModuleIdentifierAlgorithmUUID,
            /* VK_KHR_performance_query */
            /* VK_NV_device_generated_commands */
   p->maxIndirectCommandsStreamCount = 1;
   p->maxIndirectCommandsStreamStride = UINT32_MAX;
   p->maxIndirectCommandsTokenCount = UINT32_MAX;
   p->maxIndirectCommandsTokenOffset = UINT16_MAX;
   p->minIndirectCommandsBufferOffsetAlignment = 4;
   p->minSequencesCountBufferOffsetAlignment = 4;
   p->minSequencesIndexBufferOffsetAlignment = 4;
   /* Don't support even a shader group count = 1 until we support shader
   * overrides during pipeline creation. */
   p->maxGraphicsShaderGroupCount = 0;
   /* MSB reserved for signalling indirect count enablement. */
            /* VK_EXT_graphics_pipeline_library */
   p->graphicsPipelineLibraryFastLinking = true;
            /* VK_EXT_mesh_shader */
   p->maxTaskWorkGroupTotalCount = 4194304; /* 2^22 min required */
   p->maxTaskWorkGroupCount[0] = 65535;
   p->maxTaskWorkGroupCount[1] = 65535;
   p->maxTaskWorkGroupCount[2] = 65535;
   p->maxTaskWorkGroupInvocations = 1024;
   p->maxTaskWorkGroupSize[0] = 1024;
   p->maxTaskWorkGroupSize[1] = 1024;
   p->maxTaskWorkGroupSize[2] = 1024;
   p->maxTaskPayloadSize = 16384; /* 16K min required */
   p->maxTaskSharedMemorySize = 65536;
            p->maxMeshWorkGroupTotalCount = 4194304; /* 2^22 min required */
   p->maxMeshWorkGroupCount[0] = 65535;
   p->maxMeshWorkGroupCount[1] = 65535;
   p->maxMeshWorkGroupCount[2] = 65535;
   p->maxMeshWorkGroupInvocations = 256; /* Max NGG HW limit */
   p->maxMeshWorkGroupSize[0] = 256;
   p->maxMeshWorkGroupSize[1] = 256;
   p->maxMeshWorkGroupSize[2] = 256;
   p->maxMeshOutputMemorySize = 32 * 1024;                                                    /* 32K min required */
   p->maxMeshSharedMemorySize = 28672;                                                        /* 28K min required */
   p->maxMeshPayloadAndSharedMemorySize = p->maxTaskPayloadSize + p->maxMeshSharedMemorySize; /* 28K min required */
   p->maxMeshPayloadAndOutputMemorySize = p->maxTaskPayloadSize + p->maxMeshOutputMemorySize; /* 47K min required */
   p->maxMeshOutputComponents = 128; /* 32x vec4 min required */
   p->maxMeshOutputVertices = 256;
   p->maxMeshOutputPrimitives = 256;
   p->maxMeshOutputLayers = 8;
   p->maxMeshMultiviewViewCount = MAX_VIEWS;
   p->meshOutputPerVertexGranularity = 1;
            p->maxPreferredTaskWorkGroupInvocations = 64;
   p->maxPreferredMeshWorkGroupInvocations = 128;
   p->prefersLocalInvocationVertexOutput = true;
   p->prefersLocalInvocationPrimitiveOutput = true;
   p->prefersCompactVertexOutput = true;
            /* VK_EXT_extended_dynamic_state3 */
            /* VK_EXT_descriptor_buffer */
   p->combinedImageSamplerDescriptorSingleArray = true;
   p->bufferlessPushDescriptors = true;
   p->allowSamplerImageViewPostSubmitCreation = false;
   p->descriptorBufferOffsetAlignment = 4;
   p->maxDescriptorBufferBindings = MAX_SETS;
   p->maxResourceDescriptorBufferBindings = MAX_SETS;
   p->maxSamplerDescriptorBufferBindings = MAX_SETS;
   p->maxEmbeddedImmutableSamplerBindings = MAX_SETS;
   p->maxEmbeddedImmutableSamplers = radv_max_descriptor_set_size();
   p->bufferCaptureReplayDescriptorDataSize = 0;
   p->imageCaptureReplayDescriptorDataSize = 0;
   p->imageViewCaptureReplayDescriptorDataSize = 0;
   p->samplerCaptureReplayDescriptorDataSize = 0;
   p->accelerationStructureCaptureReplayDescriptorDataSize = 0;
   p->samplerDescriptorSize = 16;
   p->combinedImageSamplerDescriptorSize = 96;
   p->sampledImageDescriptorSize = 64;
   p->storageImageDescriptorSize = 32;
   p->uniformTexelBufferDescriptorSize = 16;
   p->robustUniformTexelBufferDescriptorSize = 16;
   p->storageTexelBufferDescriptorSize = 16;
   p->robustStorageTexelBufferDescriptorSize = 16;
   p->uniformBufferDescriptorSize = 16;
   p->robustUniformBufferDescriptorSize = 16;
   p->storageBufferDescriptorSize = 16;
   p->robustStorageBufferDescriptorSize = 16;
   p->inputAttachmentDescriptorSize = 64;
   p->accelerationStructureDescriptorSize = 16;
   p->maxSamplerDescriptorBufferRange = UINT32_MAX;
   p->maxResourceDescriptorBufferRange = UINT32_MAX;
   p->samplerDescriptorBufferAddressSpaceSize = RADV_MAX_MEMORY_ALLOCATION_SIZE;
   p->resourceDescriptorBufferAddressSpaceSize = RADV_MAX_MEMORY_ALLOCATION_SIZE;
            /* VK_KHR_fragment_shader_barycentric */
            /* VK_EXT_pipeline_robustness */
   p->defaultRobustnessStorageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT;
   p->defaultRobustnessUniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_EXT;
   p->defaultRobustnessVertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT;
            /* VK_KHR_maintenance5 */
   p->earlyFragmentMultisampleCoverageAfterSampleCounting = false;
   p->earlyFragmentSampleMaskTestBeforeSampleCounting = false;
   p->depthStencilSwizzleOneSupport = false;
   p->polygonModePointSize = true;
   p->nonStrictSinglePixelWideLinesUseParallelogram = false;
            /* VK_KHR_cooperative_matrix */
      }
      static VkResult
   radv_physical_device_try_create(struct radv_instance *instance, drmDevicePtr drm_device,
         {
      VkResult result;
   int fd = -1;
         #ifdef _WIN32
         #else
      if (drm_device) {
      const char *path = drm_device->nodes[DRM_NODE_RENDER];
            fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
                  version = drmGetVersion(fd);
   if (!version) {
               return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
               if (strcmp(version->name, "amdgpu")) {
                     return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
      }
            if (instance->debug_flags & RADV_DEBUG_STARTUP)
         #endif
         struct radv_physical_device *device =
         if (!device) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
               struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table, &radv_physical_device_entrypoints, true);
            result = vk_physical_device_init(&device->vk, &instance->vk, NULL, NULL, NULL, &dispatch_table);
   if (result != VK_SUCCESS) {
                        #ifdef _WIN32
         #else
      if (drm_device) {
                  } else {
            #endif
         if (!device->ws) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED, "failed to initialize winsys");
                     #ifndef _WIN32
      if (drm_device && instance->vk.enabled_extensions.KHR_display) {
      master_fd = open(drm_device->nodes[DRM_NODE_PRIMARY], O_RDWR | O_CLOEXEC);
   if (master_fd >= 0) {
      uint32_t accel_working = 0;
   struct drm_amdgpu_info request = {.return_pointer = (uintptr_t)&accel_working,
                  if (drmCommandWrite(master_fd, DRM_AMDGPU_INFO, &request, sizeof(struct drm_amdgpu_info)) < 0 ||
      !accel_working) {
   close(master_fd);
               #endif
         device->master_fd = master_fd;
   device->local_fd = fd;
               #ifndef LLVM_AVAILABLE
      if (device->use_llvm) {
      fprintf(stderr, "ERROR: LLVM compiler backend selected for radv, but LLVM support was not "
               #endif
      #ifdef ANDROID
      device->emulate_etc2 = !radv_device_supports_etc(device);
      #else
      device->emulate_etc2 =
            #endif
         snprintf(device->name, sizeof(device->name), "AMD RADV %s%s", device->rad_info.name,
            const char *marketing_name = device->ws->get_chip_name(device->ws);
   snprintf(device->marketing_name, sizeof(device->name), "%s (RADV %s%s)",
            if (radv_device_get_cache_uuid(device, device->cache_uuid)) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED, "cannot generate UUID");
               /* The gpu id is already embedded in the uuid so we just pass "radv"
   * when creating the cache.
   */
   char buf[VK_UUID_SIZE * 2 + 1];
   mesa_bytes_to_hex(buf, device->cache_uuid, VK_UUID_SIZE);
            if (!radv_is_conformant(device))
            radv_get_driver_uuid(&device->driver_uuid);
                              device->use_ngg = (device->rad_info.gfx_level >= GFX10 && device->rad_info.family != CHIP_NAVI14 &&
                  /* TODO: Investigate if NGG culling helps on GFX11. */
   device->use_ngg_culling =
      device->use_ngg && device->rad_info.max_render_backends > 1 &&
   (device->rad_info.gfx_level == GFX10_3 || (device->instance->perftest_flags & RADV_PERFTEST_NGGC)) &&
         device->use_ngg_streamout = device->use_ngg && (device->rad_info.gfx_level >= GFX11 ||
                     /* Determine the number of threads per wave for all stages. */
   device->cs_wave_size = 64;
   device->ps_wave_size = 64;
   device->ge_wave_size = 64;
            if (device->rad_info.gfx_level >= GFX10) {
      if (device->instance->perftest_flags & RADV_PERFTEST_CS_WAVE_32)
            /* For pixel shaders, wave64 is recommended. */
   if (device->instance->perftest_flags & RADV_PERFTEST_PS_WAVE_32)
            if (device->instance->perftest_flags & RADV_PERFTEST_GE_WAVE_32)
            /* Default to 32 on RDNA1-2 as that gives better perf due to less issues with divergence.
   * However, on GFX11 default to wave64 as ACO does not support VOPD yet, and with the VALU
   * dependence wave32 would likely be a net-loss (as well as the SALU count becoming more
   * problematic)
   */
   if (!(device->instance->perftest_flags & RADV_PERFTEST_RT_WAVE_64) && !(device->instance->force_rt_wave64) &&
      device->rad_info.gfx_level < GFX11)
                              radv_physical_device_get_supported_extensions(device, &device->vk.supported_extensions);
                  #ifndef _WIN32
      if (drm_device) {
               device->available_nodes = drm_device->available_nodes;
            if ((drm_device->available_nodes & (1 << DRM_NODE_PRIMARY)) &&
      stat(drm_device->nodes[DRM_NODE_PRIMARY], &primary_stat) != 0) {
   result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED, "failed to stat DRM primary node %s",
            }
            if ((drm_device->available_nodes & (1 << DRM_NODE_RENDER)) &&
      stat(drm_device->nodes[DRM_NODE_RENDER], &render_stat) != 0) {
   result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED, "failed to stat DRM render node %s",
            }
         #endif
                  if ((device->instance->debug_flags & RADV_DEBUG_INFO))
                              /* We don't check the error code, but later check if it is initialized. */
            /* The WSI is structured as a layer on top of the driver, so this has
   * to be the last part of initialization (at least until we get other
   * semi-layers).
   */
   result = radv_init_wsi(device);
   if (result != VK_SUCCESS) {
      vk_error(instance, result);
                        ac_get_hs_info(&device->rad_info, &device->hs);
   ac_get_task_info(&device->rad_info, &device->task_info);
                           fail_perfcounters:
      ac_destroy_perfcounters(&device->ac_perfcounters);
      fail_wsi:
         fail_base:
         fail_alloc:
         fail_fd:
      if (fd != -1)
         if (master_fd != -1)
            }
      VkResult
   create_null_physical_device(struct vk_instance *vk_instance)
   {
      struct radv_instance *instance = container_of(vk_instance, struct radv_instance, vk);
            VkResult result = radv_physical_device_try_create(instance, NULL, &pdevice);
   if (result != VK_SUCCESS)
            list_addtail(&pdevice->vk.link, &instance->vk.physical_devices.list);
      }
      VkResult
   create_drm_physical_device(struct vk_instance *vk_instance, struct _drmDevice *device, struct vk_physical_device **out)
   {
   #ifndef _WIN32
      if (!(device->available_nodes & (1 << DRM_NODE_RENDER)) || device->bustype != DRM_BUS_PCI ||
      device->deviceinfo.pci->vendor_id != ATI_VENDOR_ID)
         return radv_physical_device_try_create((struct radv_instance *)vk_instance, device,
      #else
         #endif
   }
      void
   radv_physical_device_destroy(struct vk_physical_device *vk_device)
   {
               radv_finish_wsi(device);
   ac_destroy_perfcounters(&device->ac_perfcounters);
   device->ws->destroy(device->ws);
   disk_cache_destroy(device->vk.disk_cache);
   if (device->local_fd != -1)
         if (device->master_fd != -1)
         vk_physical_device_finish(&device->vk);
      }
      static void
   radv_get_physical_device_queue_family_properties(struct radv_physical_device *pdevice, uint32_t *pCount,
         {
      int num_queue_families = 1;
   int idx;
   if (pdevice->rad_info.ip[AMD_IP_COMPUTE].num_queues > 0 &&
      !(pdevice->instance->debug_flags & RADV_DEBUG_NO_COMPUTE_QUEUE))
         if (pdevice->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE) {
      if (pdevice->rad_info.ip[pdevice->vid_decode_ip].num_queues > 0)
               if (pQueueFamilyProperties == NULL) {
      *pCount = num_queue_families;
               if (!*pCount)
            idx = 0;
   if (*pCount >= 1) {
      *pQueueFamilyProperties[idx] = (VkQueueFamilyProperties){
      .queueFlags =
         .queueCount = 1,
   .timestampValidBits = 64,
      };
               if (pdevice->rad_info.ip[AMD_IP_COMPUTE].num_queues > 0 &&
      !(pdevice->instance->debug_flags & RADV_DEBUG_NO_COMPUTE_QUEUE)) {
   if (*pCount > idx) {
      *pQueueFamilyProperties[idx] = (VkQueueFamilyProperties){
      .queueFlags = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_SPARSE_BINDING_BIT,
   .queueCount = pdevice->rad_info.ip[AMD_IP_COMPUTE].num_queues,
   .timestampValidBits = 64,
      };
                  if (pdevice->instance->perftest_flags & RADV_PERFTEST_VIDEO_DECODE) {
      if (pdevice->rad_info.ip[pdevice->vid_decode_ip].num_queues > 0) {
      if (*pCount > idx) {
      *pQueueFamilyProperties[idx] = (VkQueueFamilyProperties){
      .queueFlags = VK_QUEUE_VIDEO_DECODE_BIT_KHR,
   .queueCount = pdevice->rad_info.ip[pdevice->vid_decode_ip].num_queues,
   .timestampValidBits = 64,
      };
                        }
      static const VkQueueGlobalPriorityKHR radv_global_queue_priorities[] = {
      VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR,
   VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR,
   VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR,
      };
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice, uint32_t *pCount,
         {
      RADV_FROM_HANDLE(radv_physical_device, pdevice, physicalDevice);
   if (!pQueueFamilyProperties) {
      radv_get_physical_device_queue_family_properties(pdevice, pCount, NULL);
      }
   VkQueueFamilyProperties *properties[] = {
      &pQueueFamilyProperties[0].queueFamilyProperties,
   &pQueueFamilyProperties[1].queueFamilyProperties,
      };
   radv_get_physical_device_queue_family_properties(pdevice, pCount, properties);
            for (uint32_t i = 0; i < *pCount; i++) {
      vk_foreach_struct (ext, pQueueFamilyProperties[i].pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR: {
      VkQueueFamilyGlobalPriorityPropertiesKHR *prop = (VkQueueFamilyGlobalPriorityPropertiesKHR *)ext;
   STATIC_ASSERT(ARRAY_SIZE(radv_global_queue_priorities) <= VK_MAX_GLOBAL_PRIORITY_SIZE_KHR);
   prop->priorityCount = ARRAY_SIZE(radv_global_queue_priorities);
   memcpy(&prop->priorities, radv_global_queue_priorities, sizeof(radv_global_queue_priorities));
      }
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR: {
      VkQueueFamilyQueryResultStatusPropertiesKHR *prop = (VkQueueFamilyQueryResultStatusPropertiesKHR *)ext;
   prop->queryResultStatusSupport = VK_FALSE;
      }
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR: {
      VkQueueFamilyVideoPropertiesKHR *prop = (VkQueueFamilyVideoPropertiesKHR *)ext;
   if (pQueueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR)
      prop->videoCodecOperations =
         }
   default:
                  }
      static void
   radv_get_memory_budget_properties(VkPhysicalDevice physicalDevice,
         {
      RADV_FROM_HANDLE(radv_physical_device, device, physicalDevice);
            /* For all memory heaps, the computation of budget is as follow:
   *	heap_budget = heap_size - global_heap_usage + app_heap_usage
   *
   * The Vulkan spec 1.1.97 says that the budget should include any
   * currently allocated device memory.
   *
   * Note that the application heap usages are not really accurate (eg.
   * in presence of shared buffers).
   */
   if (!device->rad_info.has_dedicated_vram) {
      if (device->instance->enable_unified_heap_on_apu) {
      /* When the heaps are unified, only the visible VRAM heap is exposed on APUs. */
   assert(device->heaps == RADV_HEAP_VRAM_VIS);
                                 /* Get the different memory usages. */
   uint64_t vram_vis_internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_VRAM_VIS) +
         uint64_t gtt_internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_GTT);
   uint64_t total_internal_usage = vram_vis_internal_usage + gtt_internal_usage;
   uint64_t total_system_usage = device->ws->query_value(device->ws, RADEON_VRAM_VIS_USAGE) +
                                 memoryBudget->heapBudget[vram_vis_heap_idx] = total_free_space + total_internal_usage;
      } else {
      /* On APUs, the driver exposes fake heaps to the application because usually the carveout
   * is too small for games but the budgets need to be redistributed accordingly.
   */
   assert(device->heaps == (RADV_HEAP_GTT | RADV_HEAP_VRAM_VIS));
   assert(device->memory_properties.memoryHeaps[0].flags == 0); /* GTT */
                  /* Get the visible VRAM/GTT heap sizes and internal usages. */
                  uint64_t vram_vis_internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_VRAM_VIS) +
                  /* Compute the total heap size, internal and system usage. */
   uint64_t total_heap_size = vram_vis_heap_size + gtt_heap_size;
   uint64_t total_internal_usage = vram_vis_internal_usage + gtt_internal_usage;
                                                         /* Distribute the total free space (2/3rd as VRAM and 1/3rd as GTT) to match the heap
   * sizes, and align down to the page size to be conservative.
   */
   vram_vis_free_space =
                  memoryBudget->heapBudget[vram_vis_heap_idx] = vram_vis_free_space + vram_vis_internal_usage;
   memoryBudget->heapUsage[vram_vis_heap_idx] = vram_vis_internal_usage;
   memoryBudget->heapBudget[gtt_heap_idx] = gtt_free_space + gtt_internal_usage;
         } else {
      unsigned mask = device->heaps;
   unsigned heap = 0;
   while (mask) {
                     switch (type) {
   case RADV_HEAP_VRAM:
      internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_VRAM);
   system_usage = device->ws->query_value(device->ws, RADEON_VRAM_USAGE);
      case RADV_HEAP_VRAM_VIS:
      internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_VRAM_VIS);
   if (!(device->heaps & RADV_HEAP_VRAM))
         system_usage = device->ws->query_value(device->ws, RADEON_VRAM_VIS_USAGE);
      case RADV_HEAP_GTT:
      internal_usage = device->ws->query_value(device->ws, RADEON_ALLOCATED_GTT);
   system_usage = device->ws->query_value(device->ws, RADEON_GTT_USAGE);
                        uint64_t free_space = device->memory_properties.memoryHeaps[heap].size -
         memoryBudget->heapBudget[heap] = free_space + internal_usage;
   memoryBudget->heapUsage[heap] = internal_usage;
                           /* The heapBudget and heapUsage values must be zero for array elements
   * greater than or equal to
   * VkPhysicalDeviceMemoryProperties::memoryHeapCount.
   */
   for (uint32_t i = memory_properties->memoryHeapCount; i < VK_MAX_MEMORY_HEAPS; i++) {
      memoryBudget->heapBudget[i] = 0;
         }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
         {
                        VkPhysicalDeviceMemoryBudgetPropertiesEXT *memory_budget =
         if (memory_budget)
      }
      static const VkTimeDomainEXT radv_time_domains[] = {
      VK_TIME_DOMAIN_DEVICE_EXT,
      #ifdef CLOCK_MONOTONIC_RAW
         #endif
   };
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceCalibrateableTimeDomainsEXT(VkPhysicalDevice physicalDevice, uint32_t *pTimeDomainCount,
         {
      int d;
            for (d = 0; d < ARRAY_SIZE(radv_time_domains); d++) {
      vk_outarray_append_typed(VkTimeDomainEXT, &out, i)
   {
                        }
      VKAPI_ATTR void VKAPI_CALL
   radv_GetPhysicalDeviceMultisamplePropertiesEXT(VkPhysicalDevice physicalDevice, VkSampleCountFlagBits samples,
         {
               if (samples & supported_samples) {
         } else {
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceFragmentShadingRatesKHR(VkPhysicalDevice physicalDevice, uint32_t *pFragmentShadingRateCount,
         {
      VK_OUTARRAY_MAKE_TYPED(VkPhysicalDeviceFragmentShadingRateKHR, out, pFragmentShadingRates,
         #define append_rate(w, h, s)                                                                                           \
      {                                                                                                                   \
      VkPhysicalDeviceFragmentShadingRateKHR rate = {                                                                  \
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR,                              \
   .sampleCounts = s,                                                                                            \
      };                                                                                                               \
               for (uint32_t x = 2; x >= 1; x--) {
      for (uint32_t y = 2; y >= 1; y--) {
               if (x == 1 && y == 1) {
         } else {
                           #undef append_rate
            }
      /* VK_EXT_tooling_info */
   VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceToolProperties(VkPhysicalDevice physicalDevice, uint32_t *pToolCount,
         {
               VK_OUTARRAY_MAKE_TYPED(VkPhysicalDeviceToolProperties, out, pToolProperties, pToolCount);
   bool rgp_enabled, rmv_enabled, rra_enabled;
            /* RGP */
   rgp_enabled = pdevice->instance->vk.trace_mode & RADV_TRACE_MODE_RGP;
   if (rgp_enabled)
            /* RMV */
   rmv_enabled = pdevice->instance->vk.trace_mode & VK_TRACE_MODE_RMV;
   if (rmv_enabled)
            /* RRA */
   rra_enabled = pdevice->instance->vk.trace_mode & RADV_TRACE_MODE_RRA;
   if (rra_enabled)
            if (!pToolProperties) {
      *pToolCount = tool_count;
               if (rgp_enabled) {
      VkPhysicalDeviceToolProperties tool = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES,
   .name = "Radeon GPU Profiler",
   .version = "1.15",
   .description = "A ground-breaking low-level optimization tool that provides detailed "
         .purposes = VK_TOOL_PURPOSE_PROFILING_BIT | VK_TOOL_PURPOSE_TRACING_BIT |
            };
               if (rmv_enabled) {
      VkPhysicalDeviceToolProperties tool = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES,
   .name = "Radeon Memory Visualizer",
   .version = "1.6",
   .description = "A tool to allow you to gain a deep understanding of how your application "
            };
               if (rra_enabled) {
      VkPhysicalDeviceToolProperties tool = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TOOL_PROPERTIES,
   .name = "Radeon Raytracing Analyzer",
   .version = "1.2",
   .description = "A tool to investigate the performance of your ray tracing applications and "
            };
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetPhysicalDeviceCooperativeMatrixPropertiesKHR(VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
         {
               vk_outarray_append_typed(VkCooperativeMatrixPropertiesKHR, &out, p)
   {
      *p = (struct VkCooperativeMatrixPropertiesKHR){.sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR,
                                                .MSize = 16,
            vk_outarray_append_typed(VkCooperativeMatrixPropertiesKHR, &out, p)
   {
      *p = (struct VkCooperativeMatrixPropertiesKHR){.sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR,
                                                .MSize = 16,
            for (unsigned asigned = 0; asigned < 2; asigned++) {
      for (unsigned bsigned = 0; bsigned < 2; bsigned++) {
      for (unsigned csigned = 0; csigned < 2; csigned++) {
      for (unsigned saturate = 0; saturate < 2; saturate++) {
      if (!csigned && saturate)
         vk_outarray_append_typed(VkCooperativeMatrixPropertiesKHR, &out, p)
   {
      *p = (struct VkCooperativeMatrixPropertiesKHR){
      .sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR,
   .MSize = 16,
   .NSize = 16,
   .KSize = 16,
   .AType = asigned ? VK_COMPONENT_TYPE_SINT8_KHR : VK_COMPONENT_TYPE_UINT8_KHR,
   .BType = bsigned ? VK_COMPONENT_TYPE_SINT8_KHR : VK_COMPONENT_TYPE_UINT8_KHR,
   .CType = csigned ? VK_COMPONENT_TYPE_SINT32_KHR : VK_COMPONENT_TYPE_UINT32_KHR,
   .ResultType = csigned ? VK_COMPONENT_TYPE_SINT32_KHR : VK_COMPONENT_TYPE_UINT32_KHR,
   .saturatingAccumulation = saturate,
                           }
