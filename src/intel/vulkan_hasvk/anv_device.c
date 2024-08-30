   /*
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
      #include <assert.h>
   #include <inttypes.h>
   #include <stdbool.h>
   #include <string.h>
   #ifdef MAJOR_IN_MKDEV
   #include <sys/mkdev.h>
   #endif
   #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
   #include <sys/mman.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include "drm-uapi/drm_fourcc.h"
   #include "drm-uapi/drm.h"
   #include <xf86drm.h>
      #include "anv_private.h"
   #include "anv_measure.h"
   #include "util/u_debug.h"
   #include "util/build_id.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
   #include "util/os_file.h"
   #include "util/os_misc.h"
   #include "util/u_atomic.h"
   #include "util/u_string.h"
   #include "util/driconf.h"
   #include "git_sha1.h"
   #include "vk_util.h"
   #include "vk_deferred_operation.h"
   #include "vk_drm_syncobj.h"
   #include "common/intel_defines.h"
   #include "common/intel_uuid.h"
   #include "perf/intel_perf.h"
      #include "genxml/gen7_pack.h"
   #include "genxml/genX_bits.h"
      static const driOptionDescription anv_dri_options[] = {
      DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_ADAPTIVE_SYNC(true)
   DRI_CONF_VK_X11_OVERRIDE_MIN_IMAGE_COUNT(0)
   DRI_CONF_VK_X11_STRICT_IMAGE_COUNT(false)
   DRI_CONF_VK_XWAYLAND_WAIT_READY(true)
   DRI_CONF_ANV_ASSUME_FULL_SUBGROUPS(0)
   DRI_CONF_ANV_SAMPLE_MASK_OUT_OPENGL_BEHAVIOUR(false)
               DRI_CONF_SECTION_DEBUG
      DRI_CONF_ALWAYS_FLUSH_CACHE(false)
   DRI_CONF_VK_WSI_FORCE_BGRA8_UNORM_FIRST(false)
   DRI_CONF_VK_WSI_FORCE_SWAPCHAIN_TO_CURRENT_EXTENT(false)
               DRI_CONF_SECTION_QUALITY
            };
      /* This is probably far to big but it reflects the max size used for messages
   * in OpenGLs KHR_debug.
   */
   #define MAX_DEBUG_MESSAGE_LENGTH    4096
      /* Render engine timestamp register */
   #define TIMESTAMP 0x2358
      /* The "RAW" clocks on Linux are called "FAST" on FreeBSD */
   #if !defined(CLOCK_MONOTONIC_RAW) && defined(CLOCK_MONOTONIC_FAST)
   #define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC_FAST
   #endif
      static void
   compiler_debug_log(void *data, UNUSED unsigned *id, const char *fmt, ...)
   {
      char str[MAX_DEBUG_MESSAGE_LENGTH];
   struct anv_device *device = (struct anv_device *)data;
            va_list args;
   va_start(args, fmt);
   (void) vsnprintf(str, MAX_DEBUG_MESSAGE_LENGTH, fmt, args);
               }
      static void
   compiler_perf_log(UNUSED void *data, UNUSED unsigned *id, const char *fmt, ...)
   {
      va_list args;
            if (INTEL_DEBUG(DEBUG_PERF))
               }
      #if defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
      defined(VK_USE_PLATFORM_XCB_KHR) || \
   defined(VK_USE_PLATFORM_XLIB_KHR) || \
      #define ANV_USE_WSI_PLATFORM
   #endif
      #ifdef ANDROID
   #define ANV_API_VERSION VK_MAKE_VERSION(1, 1, VK_HEADER_VERSION)
   #else
   #define ANV_API_VERSION_1_3 VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
   #define ANV_API_VERSION_1_2 VK_MAKE_VERSION(1, 2, VK_HEADER_VERSION)
   #endif
      VkResult anv_EnumerateInstanceVersion(
         {
   #ifdef ANDROID
         #else
         #endif
         }
      static const struct vk_instance_extension_table instance_extensions = {
      .KHR_device_group_creation                = true,
   .KHR_external_fence_capabilities          = true,
   .KHR_external_memory_capabilities         = true,
   .KHR_external_semaphore_capabilities      = true,
   .KHR_get_physical_device_properties2      = true,
   .EXT_debug_report                         = true,
         #ifdef ANV_USE_WSI_PLATFORM
      .KHR_get_surface_capabilities2            = true,
   .KHR_surface                              = true,
      #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
         #endif
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
      .KHR_display                              = true,
   .KHR_get_display_properties2              = true,
   .EXT_direct_mode_display                  = true,
   .EXT_display_surface_counter              = true,
      #endif
   };
      static void
   get_device_extensions(const struct anv_physical_device *device,
         {
      const bool has_syncobj_wait =
            *ext = (struct vk_device_extension_table) {
      .KHR_8bit_storage                      = device->info.ver >= 8,
   .KHR_16bit_storage                     = device->info.ver >= 8 && !device->instance->no_16bit,
   .KHR_bind_memory2                      = true,
   .KHR_buffer_device_address             = device->has_a64_buffer_access,
   .KHR_copy_commands2                    = true,
   .KHR_create_renderpass2                = true,
   .KHR_dedicated_allocation              = true,
   .KHR_deferred_host_operations          = true,
   .KHR_depth_stencil_resolve             = true,
   .KHR_descriptor_update_template        = true,
   .KHR_device_group                      = true,
   .KHR_draw_indirect_count               = true,
   .KHR_driver_properties                 = true,
   .KHR_dynamic_rendering                 = true,
   .KHR_external_fence                    = has_syncobj_wait,
   .KHR_external_fence_fd                 = has_syncobj_wait,
   .KHR_external_memory                   = true,
   .KHR_external_memory_fd                = true,
   .KHR_external_semaphore                = true,
   .KHR_external_semaphore_fd             = true,
   .KHR_format_feature_flags2             = true,
   .KHR_get_memory_requirements2          = true,
   .KHR_image_format_list                 = true,
   #ifdef ANV_USE_WSI_PLATFORM
         #endif
         .KHR_maintenance1                      = true,
   .KHR_maintenance2                      = true,
   .KHR_maintenance3                      = true,
   .KHR_maintenance4                      = true,
   .KHR_multiview                         = true,
   .KHR_performance_query =
      !anv_use_relocations(device) && device->perf &&
   (device->perf->i915_perf_version >= 3 ||
   INTEL_DEBUG(DEBUG_NO_OACONFIG)) &&
      .KHR_pipeline_executable_properties    = true,
   .KHR_push_descriptor                   = true,
   .KHR_relaxed_block_layout              = true,
   .KHR_sampler_mirror_clamp_to_edge      = true,
   .KHR_sampler_ycbcr_conversion          = true,
   .KHR_separate_depth_stencil_layouts    = true,
   .KHR_shader_clock                      = true,
   .KHR_shader_draw_parameters            = true,
   .KHR_shader_float16_int8               = device->info.ver >= 8 && !device->instance->no_16bit,
   .KHR_shader_float_controls             = true,
   .KHR_shader_integer_dot_product        = true,
   .KHR_shader_non_semantic_info          = true,
   .KHR_shader_subgroup_extended_types    = device->info.ver >= 8,
   .KHR_shader_subgroup_uniform_control_flow = true,
   .KHR_shader_terminate_invocation       = true,
   .KHR_spirv_1_4                         = true,
   #ifdef ANV_USE_WSI_PLATFORM
         .KHR_swapchain                         = true,
   #endif
         .KHR_synchronization2                  = true,
   .KHR_timeline_semaphore                = true,
   .KHR_uniform_buffer_standard_layout    = true,
   .KHR_variable_pointers                 = true,
   .KHR_vulkan_memory_model               = true,
   .KHR_workgroup_memory_explicit_layout  = true,
   .KHR_zero_initialize_workgroup_memory  = true,
   .EXT_4444_formats                      = true,
   .EXT_border_color_swizzle              = device->info.ver >= 8,
   .EXT_buffer_device_address             = device->has_a64_buffer_access,
   .EXT_calibrated_timestamps             = device->has_reg_timestamp,
   .EXT_color_write_enable                = true,
   .EXT_conditional_rendering             = device->info.verx10 >= 75,
   .EXT_custom_border_color               = device->info.ver >= 8,
   .EXT_depth_clamp_zero_one              = true,
   .EXT_depth_clip_control                = true,
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
         #endif
         .EXT_extended_dynamic_state            = true,
   .EXT_extended_dynamic_state2           = true,
   .EXT_external_memory_dma_buf           = true,
   .EXT_external_memory_host              = true,
   .EXT_global_priority                   = device->max_context_priority >=
         .EXT_global_priority_query             = device->max_context_priority >=
         .EXT_host_query_reset                  = true,
   .EXT_image_2d_view_of_3d               = true,
   .EXT_image_robustness                  = true,
   .EXT_image_drm_format_modifier         = true,
   .EXT_image_view_min_lod                = true,
   .EXT_index_type_uint8                  = true,
   .EXT_inline_uniform_block              = true,
   .EXT_line_rasterization                = true,
   /* Enable the extension only if we have support on both the local &
   * system memory
   */
   .EXT_memory_budget                     = device->sys.available,
   .EXT_non_seamless_cube_map             = true,
   .EXT_pci_bus_info                      = true,
   .EXT_physical_device_drm               = true,
   .EXT_pipeline_creation_cache_control   = true,
   .EXT_pipeline_creation_feedback        = true,
   .EXT_primitives_generated_query        = true,
   .EXT_primitive_topology_list_restart   = true,
   .EXT_private_data                      = true,
   .EXT_provoking_vertex                  = true,
   .EXT_queue_family_foreign              = true,
   .EXT_robustness2                       = true,
   .EXT_sample_locations                  = true,
   .EXT_scalar_block_layout               = true,
   .EXT_separate_stencil_usage            = true,
   .EXT_shader_atomic_float               = true,
   .EXT_shader_demote_to_helper_invocation = true,
   .EXT_shader_module_identifier          = true,
   .EXT_shader_subgroup_ballot            = true,
   .EXT_shader_subgroup_vote              = true,
   .EXT_shader_viewport_index_layer       = true,
   .EXT_subgroup_size_control             = true,
   .EXT_texel_buffer_alignment            = true,
   .EXT_tooling_info                      = true,
   .EXT_transform_feedback                = true,
   .EXT_vertex_attribute_divisor          = true,
   #ifdef ANDROID
         .ANDROID_external_memory_android_hardware_buffer = true,
   #endif
         .GOOGLE_decorate_string                = true,
   .GOOGLE_hlsl_functionality1            = true,
   .GOOGLE_user_type                      = true,
   .INTEL_performance_query               = device->perf &&
         .INTEL_shader_integer_functions2       = device->info.ver >= 8,
   .EXT_multi_draw                        = true,
   .NV_compute_shader_derivatives         = true,
         }
      static void
   get_features(const struct anv_physical_device *pdevice,
         {
      /* Just pick one; they're all the same */
   const bool has_astc_ldr =
      isl_format_supports_sampling(&pdevice->info,
         *features = (struct vk_features) {
      /* Vulkan 1.0 */
   .robustBufferAccess                       = true,
   .fullDrawIndexUint32                      = true,
   .imageCubeArray                           = true,
   .independentBlend                         = true,
   .geometryShader                           = true,
   .tessellationShader                       = true,
   .sampleRateShading                        = true,
   .dualSrcBlend                             = true,
   .logicOp                                  = true,
   .multiDrawIndirect                        = true,
   .drawIndirectFirstInstance                = true,
   .depthClamp                               = true,
   .depthBiasClamp                           = true,
   .fillModeNonSolid                         = true,
   .depthBounds                              = pdevice->info.ver >= 12,
   .wideLines                                = true,
   .largePoints                              = true,
   .alphaToOne                               = true,
   .multiViewport                            = true,
   .samplerAnisotropy                        = true,
   .textureCompressionETC2                   = pdevice->info.ver >= 8 ||
         .textureCompressionASTC_LDR               = has_astc_ldr,
   .textureCompressionBC                     = true,
   .occlusionQueryPrecise                    = true,
   .pipelineStatisticsQuery                  = true,
   .fragmentStoresAndAtomics                 = true,
   .shaderTessellationAndGeometryPointSize   = true,
   .shaderImageGatherExtended                = true,
   .shaderStorageImageExtendedFormats        = true,
   .shaderStorageImageMultisample            = false,
   .shaderStorageImageReadWithoutFormat      = false,
   .shaderStorageImageWriteWithoutFormat     = true,
   .shaderUniformBufferArrayDynamicIndexing  = true,
   .shaderSampledImageArrayDynamicIndexing   = true,
   .shaderStorageBufferArrayDynamicIndexing  = true,
   .shaderStorageImageArrayDynamicIndexing   = true,
   .shaderClipDistance                       = true,
   .shaderCullDistance                       = true,
   .shaderFloat64                            = pdevice->info.ver >= 8 &&
         .shaderInt64                              = pdevice->info.ver >= 8,
   .shaderInt16                              = pdevice->info.ver >= 8,
   .shaderResourceMinLod                     = false,
   .variableMultisampleRate                  = true,
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess            = pdevice->info.ver >= 8 && !pdevice->instance->no_16bit,
   .uniformAndStorageBuffer16BitAccess  = pdevice->info.ver >= 8 && !pdevice->instance->no_16bit,
   .storagePushConstant16               = pdevice->info.ver >= 8,
   .storageInputOutput16                = false,
   .multiview                           = true,
   .multiviewGeometryShader             = true,
   .multiviewTessellationShader         = true,
   .variablePointersStorageBuffer       = true,
   .variablePointers                    = true,
   .protectedMemory                     = false,
   .samplerYcbcrConversion              = true,
            /* Vulkan 1.2 */
   .samplerMirrorClampToEdge            = true,
   .drawIndirectCount                   = true,
   .storageBuffer8BitAccess             = pdevice->info.ver >= 8,
   .uniformAndStorageBuffer8BitAccess   = pdevice->info.ver >= 8,
   .storagePushConstant8                = pdevice->info.ver >= 8,
   .shaderBufferInt64Atomics            = false,
   .shaderSharedInt64Atomics            = false,
   .shaderFloat16                       = pdevice->info.ver >= 8 && !pdevice->instance->no_16bit,
            .descriptorIndexing                                 = false,
   .shaderInputAttachmentArrayDynamicIndexing          = false,
   .shaderUniformTexelBufferArrayDynamicIndexing       = false,
   .shaderStorageTexelBufferArrayDynamicIndexing       = false,
   .shaderUniformBufferArrayNonUniformIndexing         = false,
   .shaderSampledImageArrayNonUniformIndexing          = false,
   .shaderStorageBufferArrayNonUniformIndexing         = false,
   .shaderStorageImageArrayNonUniformIndexing          = false,
   .shaderInputAttachmentArrayNonUniformIndexing       = false,
   .shaderUniformTexelBufferArrayNonUniformIndexing    = false,
   .shaderStorageTexelBufferArrayNonUniformIndexing    = false,
   .descriptorBindingUniformBufferUpdateAfterBind      = false,
   .descriptorBindingSampledImageUpdateAfterBind       = false,
   .descriptorBindingStorageImageUpdateAfterBind       = false,
   .descriptorBindingStorageBufferUpdateAfterBind      = false,
   .descriptorBindingUniformTexelBufferUpdateAfterBind = false,
   .descriptorBindingStorageTexelBufferUpdateAfterBind = false,
   .descriptorBindingUpdateUnusedWhilePending          = false,
   .descriptorBindingPartiallyBound                    = false,
   .descriptorBindingVariableDescriptorCount           = false,
            .samplerFilterMinmax                 = false,
   .scalarBlockLayout                   = true,
   .imagelessFramebuffer                = true,
   .uniformBufferStandardLayout         = true,
   .shaderSubgroupExtendedTypes         = true,
   .separateDepthStencilLayouts         = true,
   .hostQueryReset                      = true,
   .timelineSemaphore                   = true,
   .bufferDeviceAddress                 = pdevice->has_a64_buffer_access,
   .bufferDeviceAddressCaptureReplay    = pdevice->has_a64_buffer_access,
   .bufferDeviceAddressMultiDevice      = false,
   .vulkanMemoryModel                   = true,
   .vulkanMemoryModelDeviceScope        = true,
   .vulkanMemoryModelAvailabilityVisibilityChains = true,
   .shaderOutputViewportIndex           = true,
   .shaderOutputLayer                   = true,
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
            /* VK_EXT_4444_formats */
   .formatA4R4G4B4 = true,
            /* VK_EXT_border_color_swizzle */
   .borderColorSwizzle = true,
            /* VK_EXT_color_write_enable */
            /* VK_EXT_image_2d_view_of_3d */
   .image2DViewOf3D = true,
            /* VK_NV_compute_shader_derivatives */
   .computeDerivativeGroupQuads = true,
            /* VK_EXT_conditional_rendering */
   .conditionalRendering = pdevice->info.verx10 >= 75,
            /* VK_EXT_custom_border_color */
   .customBorderColors = pdevice->info.ver >= 8,
            /* VK_EXT_depth_clamp_zero_one */
            /* VK_EXT_depth_clip_enable */
            /* VK_KHR_global_priority */
            /* VK_EXT_image_view_min_lod */
            /* VK_EXT_index_type_uint8 */
            /* VK_EXT_line_rasterization */
   /* Rectangular lines must use the strict algorithm, which is not
   * supported for wide lines prior to ICL.  See rasterization_mode for
   * details and how the HW states are programmed.
   */
   .rectangularLines = false,
   .bresenhamLines = true,
   /* Support for Smooth lines with MSAA was removed on gfx11.  From the
   * BSpec section "Multisample ModesState" table for "AA Line Support
   * Requirements":
   *
   *    GFX10:BUG:######## 	NUM_MULTISAMPLES == 1
   *
   * Fortunately, this isn't a case most people care about.
   */
   .smoothLines = pdevice->info.ver < 10,
   .stippledRectangularLines = false,
   .stippledBresenhamLines = true,
            /* VK_EXT_mutable_descriptor_type */
            /* VK_KHR_performance_query */
   .performanceCounterQueryPools = true,
   /* HW only supports a single configuration at a time. */
            /* VK_KHR_pipeline_executable_properties */
            /* VK_EXT_primitives_generated_query */
   .primitivesGeneratedQuery = true,
   .primitivesGeneratedQueryWithRasterizerDiscard = false,
            /* VK_EXT_provoking_vertex */
   .provokingVertexLast = true,
            /* VK_EXT_robustness2 */
   .robustBufferAccess2 = true,
   .robustImageAccess2 = true,
            /* VK_EXT_shader_atomic_float */
   .shaderBufferFloat32Atomics =    true,
   .shaderBufferFloat32AtomicAdd =  pdevice->info.has_lsc,
   .shaderBufferFloat64Atomics =
         .shaderBufferFloat64AtomicAdd =  false,
   .shaderSharedFloat32Atomics =    true,
   .shaderSharedFloat32AtomicAdd =  false,
   .shaderSharedFloat64Atomics =    false,
   .shaderSharedFloat64AtomicAdd =  false,
   .shaderImageFloat32Atomics =     true,
   .shaderImageFloat32AtomicAdd =   false,
   .sparseImageFloat32Atomics =     false,
            /* VK_KHR_shader_clock */
   .shaderSubgroupClock = true,
            /* VK_INTEL_shader_integer_functions2 */
            /* VK_EXT_shader_module_identifier */
            /* VK_KHR_shader_subgroup_uniform_control_flow */
            /* VK_EXT_texel_buffer_alignment */
            /* VK_EXT_transform_feedback */
   .transformFeedback = true,
            /* VK_EXT_vertex_attribute_divisor */
   .vertexAttributeInstanceRateDivisor = true,
            /* VK_KHR_workgroup_memory_explicit_layout */
   .workgroupMemoryExplicitLayout = true,
   .workgroupMemoryExplicitLayoutScalarBlockLayout = true,
   .workgroupMemoryExplicitLayout8BitAccess = true,
            /* VK_EXT_ycbcr_image_arrays */
            /* VK_EXT_extended_dynamic_state */
            /* VK_EXT_extended_dynamic_state2 */
   .extendedDynamicState2 = true,
   .extendedDynamicState2LogicOp = true,
            /* VK_EXT_multi_draw */
            /* VK_EXT_non_seamless_cube_map */
            /* VK_EXT_primitive_topology_list_restart */
   .primitiveTopologyListRestart = true,
            /* VK_EXT_depth_clip_control */
               /* We can't do image stores in vec4 shaders */
   features->vertexPipelineStoresAndAtomics =
      pdevice->compiler->scalar_stage[MESA_SHADER_VERTEX] &&
                  /* The new DOOM and Wolfenstein games require depthBounds without
   * checking for it.  They seem to run fine without it so just claim it's
   * there and accept the consequences.
   */
   if (app_info->engine_name && strcmp(app_info->engine_name, "idTech") == 0)
      }
      static uint64_t
   anv_compute_sys_heap_size(struct anv_physical_device *device,
         {
      /* We don't want to burn too much ram with the GPU.  If the user has 4GiB
   * or less, we use at most half.  If they have more than 4GiB, we use 3/4.
   */
   uint64_t available_ram;
   if (total_ram <= 4ull * 1024ull * 1024ull * 1024ull)
         else
            /* We also want to leave some padding for things we allocate in the driver,
   * so don't go over 3/4 of the GTT either.
   */
            if (available_ram > (2ull << 30) && !device->supports_48bit_addresses) {
      /* When running with an overridden PCI ID, we may get a GTT size from
   * the kernel that is greater than 2 GiB but the execbuf check for 48bit
   * address support can still fail.  Just clamp the address space size to
   * 2 GiB if we don't have 48-bit support.
   */
   mesa_logw("%s:%d: The kernel reported a GTT size larger than 2 GiB but "
                              }
      static VkResult MUST_CHECK
   anv_init_meminfo(struct anv_physical_device *device, int fd)
   {
               device->sys.size =
                     }
      static void
   anv_update_meminfo(struct anv_physical_device *device, int fd)
   {
      if (!intel_device_info_update_memory_info(&device->info, fd))
            const struct intel_device_info *devinfo = &device->info;
      }
      static VkResult
   anv_physical_device_init_heaps(struct anv_physical_device *device, int fd)
   {
      VkResult result = anv_init_meminfo(device, fd);
   if (result != VK_SUCCESS)
                     if (device->info.has_llc) {
      device->memory.heap_count = 1;
   device->memory.heaps[0] = (struct anv_memory_heap) {
      .size = device->sys.size,
               /* Big core GPUs share LLC with the CPU and thus one memory type can be
   * both cached and coherent at the same time.
   *
   * But some game engines can't handle single type well
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/7360#note_1719438
   *
   * And Intel on Windows uses 3 types so it's better to add extra one here
   */
   device->memory.type_count = 2;
   device->memory.types[0] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      };
   device->memory.types[1] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                           } else {
      device->memory.heap_count = 1;
   device->memory.heaps[0] = (struct anv_memory_heap) {
      .size = device->sys.size,
               /* The spec requires that we expose a host-visible, coherent memory
   * type, but Atom GPUs don't share LLC. Thus we offer two memory types
   * to give the application a choice between cached, but not coherent and
   * coherent but uncached (WC though).
   */
   device->memory.type_count = 2;
   device->memory.types[0] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                  };
   device->memory.types[1] = (struct anv_memory_type) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                              device->memory.need_flush = false;
   for (unsigned i = 0; i < device->memory.type_count; i++) {
      VkMemoryPropertyFlags props = device->memory.types[i].propertyFlags;
   if ((props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
      !(props & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
               }
      static VkResult
   anv_physical_device_init_uuids(struct anv_physical_device *device)
   {
      const struct build_id_note *note =
         if (!note) {
      return vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
               unsigned build_id_len = build_id_length(note);
   if (build_id_len < 20) {
      return vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                        struct mesa_sha1 sha1_ctx;
   uint8_t sha1[20];
            /* The pipeline cache UUID is used for determining when a pipeline cache is
   * invalid.  It needs both a driver build and the PCI ID of the device.
   */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, build_id_data(note), build_id_len);
   _mesa_sha1_update(&sha1_ctx, &device->info.pci_device_id,
         _mesa_sha1_update(&sha1_ctx, &device->always_use_bindless,
         _mesa_sha1_update(&sha1_ctx, &device->has_a64_buffer_access,
         _mesa_sha1_update(&sha1_ctx, &device->has_bindless_samplers,
         _mesa_sha1_final(&sha1_ctx, sha1);
            intel_uuid_compute_driver_id(device->driver_uuid, &device->info, VK_UUID_SIZE);
               }
      static void
   anv_physical_device_init_disk_cache(struct anv_physical_device *device)
   {
   #ifdef ENABLE_SHADER_CACHE
      char renderer[10];
   ASSERTED int len = snprintf(renderer, sizeof(renderer), "anv_%04x",
                  char timestamp[41];
            const uint64_t driver_flags =
            #endif
   }
      static void
   anv_physical_device_free_disk_cache(struct anv_physical_device *device)
   {
   #ifdef ENABLE_SHADER_CACHE
      if (device->vk.disk_cache) {
      disk_cache_destroy(device->vk.disk_cache);
         #else
         #endif
   }
      /* The ANV_QUEUE_OVERRIDE environment variable is a comma separated list of
   * queue overrides.
   *
   * To override the number queues:
   *  * "gc" is for graphics queues with compute support
   *  * "g" is for graphics queues with no compute support
   *  * "c" is for compute queues with no graphics support
   *
   * For example, ANV_QUEUE_OVERRIDE=gc=2,c=1 would override the number of
   * advertised queues to be 2 queues with graphics+compute support, and 1 queue
   * with compute-only support.
   *
   * ANV_QUEUE_OVERRIDE=c=1 would override the number of advertised queues to
   * include 1 queue with compute-only support, but it will not change the
   * number of graphics+compute queues.
   *
   * ANV_QUEUE_OVERRIDE=gc=0,c=1 would override the number of advertised queues
   * to include 1 queue with compute-only support, and it would override the
   * number of graphics+compute queues to be 0.
   */
   static void
   anv_override_engine_counts(int *gc_count, int *g_count, int *c_count)
   {
      int gc_override = -1;
   int g_override = -1;
   int c_override = -1;
            if (env == NULL)
            env = strdup(env);
   char *save = NULL;
   char *next = strtok_r(env, ",", &save);
   while (next != NULL) {
      if (strncmp(next, "gc=", 3) == 0) {
         } else if (strncmp(next, "g=", 2) == 0) {
         } else if (strncmp(next, "c=", 2) == 0) {
         } else {
         }
      }
   free(env);
   if (gc_override >= 0)
         if (g_override >= 0)
         if (*g_count > 0 && *gc_count <= 0 && (gc_override >= 0 || g_override >= 0))
      mesa_logw("ANV_QUEUE_OVERRIDE: gc=0 with g > 0 violates the "
      if (c_override >= 0)
      }
      static void
   anv_physical_device_init_queue_families(struct anv_physical_device *pdevice)
   {
               if (pdevice->engine_info) {
      int gc_count =
      intel_engines_count(pdevice->engine_info,
      int g_count = 0;
                     if (gc_count > 0) {
      pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               .queueCount = gc_count,
         }
   if (g_count > 0) {
      pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
         .queueCount = g_count,
         }
   if (c_count > 0) {
      pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_COMPUTE_BIT |
         .queueCount = c_count,
         }
   /* Increase count below when other families are added as a reminder to
   * increase the ANV_MAX_QUEUE_FAMILIES value.
   */
      } else {
      /* Default to a single render queue */
   pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               .queueCount = 1,
      };
      }
   assert(family_count <= ANV_MAX_QUEUE_FAMILIES);
      }
      static VkResult
   anv_physical_device_try_create(struct vk_instance *vk_instance,
               {
      struct anv_instance *instance =
            if (!(drm_device->available_nodes & (1 << DRM_NODE_RENDER)) ||
      drm_device->bustype != DRM_BUS_PCI ||
   drm_device->deviceinfo.pci->vendor_id != 0x8086)
         const char *primary_path = drm_device->nodes[DRM_NODE_PRIMARY];
   const char *path = drm_device->nodes[DRM_NODE_RENDER];
   VkResult result;
   int fd;
                     fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      if (errno == ENOMEM) {
      return vk_errorf(instance, VK_ERROR_OUT_OF_HOST_MEMORY,
      }
   return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
               struct intel_device_info devinfo;
   if (!intel_get_device_info_from_fd(fd, &devinfo)) {
      result = vk_error(instance, VK_ERROR_INCOMPATIBLE_DRIVER);
               bool is_alpha = true;
   bool warn = !debug_get_bool_option("MESA_VK_IGNORE_CONFORMANCE_WARNING", false);
   if (devinfo.platform == INTEL_PLATFORM_HSW) {
      if (warn)
      } else if (devinfo.platform == INTEL_PLATFORM_IVB) {
      if (warn)
      } else if (devinfo.platform == INTEL_PLATFORM_BYT) {
      if (warn)
      } else if (devinfo.ver == 8) {
      /* Gfx8 fully supported */
      } else {
      /* Silently fail here, anv will either pick up this device or display an
   * error message.
   */
   result = VK_ERROR_INCOMPATIBLE_DRIVER;
               struct anv_physical_device *device =
      vk_zalloc(&instance->vk.alloc, sizeof(*device), 8,
      if (device == NULL) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
               struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
         vk_physical_device_dispatch_table_from_entrypoints(
            result = vk_physical_device_init(&device->vk, &instance->vk,
               if (result != VK_SUCCESS) {
      vk_error(instance, result);
      }
            assert(strlen(path) < ARRAY_SIZE(device->path));
            device->info = devinfo;
            device->cmd_parser_version = -1;
   if (device->info.ver == 7) {
      if (!intel_gem_get_param(fd, I915_PARAM_CMD_PARSER_VERSION, &device->cmd_parser_version) ||
      device->cmd_parser_version == -1) {
   result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                        int val;
   if (!intel_gem_get_param(fd, I915_PARAM_HAS_WAIT_TIMEOUT, &val) || !val) {
      result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                     if (!intel_gem_get_param(fd, I915_PARAM_HAS_EXECBUF2, &val) || !val) {
      result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                     if (!device->info.has_llc &&
      (!intel_gem_get_param(fd, I915_PARAM_MMAP_VERSION, &val) || val < 1)) {
   result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                     device->use_relocations = device->info.ver < 8 ||
            if (!device->use_relocations &&
      (!intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_SOFTPIN, &val) || !val)) {
   result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                     if (!intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_FENCE_ARRAY, &val) || !val) {
      result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                     if (intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_ASYNC, &val))
         if (intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_CAPTURE, &val))
            /* Start with medium; sorted low to high */
   const int priorities[] = {
      INTEL_CONTEXT_MEDIUM_PRIORITY,
   INTEL_CONTEXT_HIGH_PRIORITY,
      };
   device->max_context_priority = INT_MIN;
   for (unsigned i = 0; i < ARRAY_SIZE(priorities); i++) {
      if (!anv_gem_has_context_priority(fd, priorities[i]))
                     device->gtt_size = device->info.gtt_size ? device->info.gtt_size :
            /* We only allow 48-bit addresses with softpin because knowing the actual
   * address is required for the vertex cache flush workaround.
   */
   device->supports_48bit_addresses = (device->info.ver >= 8) &&
            result = anv_physical_device_init_heaps(device, fd);
   if (result != VK_SUCCESS)
            assert(device->supports_48bit_addresses == !device->use_relocations);
            if (intel_gem_get_param(fd, I915_PARAM_HAS_EXEC_TIMELINE_FENCES, &val))
         if (debug_get_bool_option("ANV_QUEUE_THREAD_DISABLE", false))
                     device->sync_syncobj_type = vk_drm_syncobj_get_type(fd);
   if (!device->has_exec_timeline)
                  if (!(device->sync_syncobj_type.features & VK_SYNC_FEATURE_CPU_WAIT))
            if (!(device->sync_syncobj_type.features & VK_SYNC_FEATURE_TIMELINE)) {
      device->sync_timeline_type = vk_sync_timeline_get_type(&anv_bo_sync_type);
               device->sync_types[st_idx++] = NULL;
   assert(st_idx <= ARRAY_SIZE(device->sync_types));
                     device->always_use_bindless =
            device->use_call_secondary =
      device->use_softpin &&
         /* We first got the A64 messages on broadwell and we can only use them if
   * we can pass addresses directly into the shader which requires softpin.
   */
   device->has_a64_buffer_access = device->info.ver >= 8 &&
            /* We've had bindless samplers since Ivy Bridge (forever in Vulkan terms)
   * because it's just a matter of setting the sampler address in the sample
   * message header.  However, we've not bothered to wire it up for vec4 so
   * we leave it disabled on gfx7.
   */
            /* Check if we can read the GPU timestamp register from the CPU */
   uint64_t u64_ignore;
   device->has_reg_timestamp = intel_gem_read_render_timestamp(fd,
                  device->always_flush_cache = INTEL_DEBUG(DEBUG_STALL) ||
            device->compiler = brw_compiler_create(NULL, &device->info);
   if (device->compiler == NULL) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
      }
   device->compiler->shader_debug_log = compiler_debug_log;
   device->compiler->shader_perf_log = compiler_perf_log;
   device->compiler->constant_buffer_0_is_relative =
         device->compiler->supports_shader_constants = true;
                     result = anv_physical_device_init_uuids(device);
   if (result != VK_SUCCESS)
                     if (instance->vk.enabled_extensions.KHR_display) {
      master_fd = open(primary_path, O_RDWR | O_CLOEXEC);
   if (master_fd >= 0) {
      /* fail if we don't have permission to even render on this device */
   if (!intel_gem_can_render_on_fd(master_fd, device->info.kmd_type)) {
      close(master_fd);
            }
            device->engine_info = intel_engine_get_info(fd, device->info.kmd_type);
                              get_device_extensions(device, &device->vk.supported_extensions);
            result = anv_init_wsi(device);
   if (result != VK_SUCCESS)
                                                if (stat(primary_path, &st) == 0) {
      device->has_master = true;
   device->master_major = major(st.st_rdev);
      } else {
      device->has_master = false;
   device->master_major = 0;
               if (stat(path, &st) == 0) {
      device->has_local = true;
   device->local_major = major(st.st_rdev);
      } else {
      device->has_local = false;
   device->local_major = 0;
                     fail_perf:
      ralloc_free(device->perf);
   free(device->engine_info);
      fail_compiler:
         fail_base:
         fail_alloc:
         fail_fd:
      close(fd);
   if (master_fd != -1)
            }
      static void
   anv_physical_device_destroy(struct vk_physical_device *vk_device)
   {
      struct anv_physical_device *device =
            anv_finish_wsi(device);
   anv_measure_device_destroy(device);
   free(device->engine_info);
   anv_physical_device_free_disk_cache(device);
   ralloc_free(device->compiler);
   ralloc_free(device->perf);
   close(device->local_fd);
   if (device->master_fd >= 0)
         vk_physical_device_finish(&device->vk);
      }
      VkResult anv_EnumerateInstanceExtensionProperties(
      const char*                                 pLayerName,
   uint32_t*                                   pPropertyCount,
      {
      if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      static void
   anv_init_dri_options(struct anv_instance *instance)
   {
      driParseOptionInfo(&instance->available_dri_options, anv_dri_options,
         driParseConfigFiles(&instance->dri_options,
                     &instance->available_dri_options, 0, "anv", NULL, NULL,
            instance->assume_full_subgroups =
         instance->limit_trig_input_range =
         instance->sample_mask_out_opengl_behaviour =
         instance->lower_depth_range_rate =
         instance->no_16bit =
      }
      VkResult anv_CreateInstance(
      const VkInstanceCreateInfo*                 pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      struct anv_instance *instance;
                     if (pAllocator == NULL)
            instance = vk_alloc(pAllocator, sizeof(*instance), 8,
         if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(
         vk_instance_dispatch_table_from_entrypoints(
            result = vk_instance_init(&instance->vk, &instance_extensions,
         if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
               instance->vk.physical_devices.try_create_for_drm = anv_physical_device_try_create;
                                                   }
      void anv_DestroyInstance(
      VkInstance                                  _instance,
      {
               if (!instance)
                     driDestroyOptionCache(&instance->dri_options);
            vk_instance_finish(&instance->vk);
      }
      #define MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BUFFERS   64
      #define MAX_PER_STAGE_DESCRIPTOR_INPUT_ATTACHMENTS 64
   #define MAX_DESCRIPTOR_SET_INPUT_ATTACHMENTS       256
      #define MAX_CUSTOM_BORDER_COLORS                   4096
      void anv_GetPhysicalDeviceProperties(
      VkPhysicalDevice                            physicalDevice,
      {
      ANV_FROM_HANDLE(anv_physical_device, pdevice, physicalDevice);
            const uint32_t max_ssbos = pdevice->has_a64_buffer_access ? UINT16_MAX : 64;
   const uint32_t max_textures = 128;
   const uint32_t max_samplers =
      pdevice->has_bindless_samplers ? UINT16_MAX :
               /* If we can use bindless for everything, claim a high per-stage limit,
   * otherwise use the binding table size, minus the slots reserved for
   * render targets and one slot for the descriptor buffer. */
            const uint32_t max_workgroup_size =
            VkSampleCountFlags sample_counts =
               VkPhysicalDeviceLimits limits = {
      .maxImageDimension1D                      = (1 << 14),
   /* Gfx7 doesn't support 8xMSAA with depth/stencil images when their width
   * is greater than 8192 pixels. */
   .maxImageDimension2D                      = devinfo->ver == 7 ? (1 << 13) : (1 << 14),
   .maxImageDimension3D                      = (1 << 11),
   .maxImageDimensionCube                    = (1 << 14),
   .maxImageArrayLayers                      = (1 << 11),
   .maxTexelBufferElements                   = 128 * 1024 * 1024,
   .maxUniformBufferRange                    = pdevice->compiler->indirect_ubos_use_sampler ? (1u << 27) : (1u << 30),
   .maxStorageBufferRange                    = MIN2(pdevice->isl_dev.max_buffer_size, UINT32_MAX),
   .maxPushConstantsSize                     = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount                 = UINT32_MAX,
   .maxSamplerAllocationCount                = 64 * 1024,
   .bufferImageGranularity                   = 1,
   .sparseAddressSpaceSize                   = 0,
   .maxBoundDescriptorSets                   = MAX_SETS,
   .maxPerStageDescriptorSamplers            = max_samplers,
   .maxPerStageDescriptorUniformBuffers      = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BUFFERS,
   .maxPerStageDescriptorStorageBuffers      = max_ssbos,
   .maxPerStageDescriptorSampledImages       = max_textures,
   .maxPerStageDescriptorStorageImages       = max_images,
   .maxPerStageDescriptorInputAttachments    = MAX_PER_STAGE_DESCRIPTOR_INPUT_ATTACHMENTS,
   .maxPerStageResources                     = max_per_stage,
   .maxDescriptorSetSamplers                 = 6 * max_samplers, /* number of stages * maxPerStageDescriptorSamplers */
   .maxDescriptorSetUniformBuffers           = 6 * MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BUFFERS,           /* number of stages * maxPerStageDescriptorUniformBuffers */
   .maxDescriptorSetUniformBuffersDynamic    = MAX_DYNAMIC_BUFFERS / 2,
   .maxDescriptorSetStorageBuffers           = 6 * max_ssbos,    /* number of stages * maxPerStageDescriptorStorageBuffers */
   .maxDescriptorSetStorageBuffersDynamic    = MAX_DYNAMIC_BUFFERS / 2,
   .maxDescriptorSetSampledImages            = 6 * max_textures, /* number of stages * maxPerStageDescriptorSampledImages */
   .maxDescriptorSetStorageImages            = 6 * max_images,   /* number of stages * maxPerStageDescriptorStorageImages */
   .maxDescriptorSetInputAttachments         = MAX_DESCRIPTOR_SET_INPUT_ATTACHMENTS,
   .maxVertexInputAttributes                 = MAX_VES,
   .maxVertexInputBindings                   = MAX_VBS,
   /* Broadwell PRMs: Volume 2d: Command Reference: Structures:
   *
   * VERTEX_ELEMENT_STATE::Source Element Offset: [0,2047]
   */
   .maxVertexInputAttributeOffset            = 2047,
   /* Broadwell PRMs: Volume 2d: Command Reference: Structures:
   *
   * VERTEX_BUFFER_STATE::Buffer Pitch: [0,2048]
   *
   * Skylake PRMs: Volume 2d: Command Reference: Structures:
   *
   * VERTEX_BUFFER_STATE::Buffer Pitch: [0,4095]
   */
   .maxVertexInputBindingStride              = devinfo->ver < 9 ? 2048 : 4095,
   .maxVertexOutputComponents                = 128,
   .maxTessellationGenerationLevel           = 64,
   .maxTessellationPatchSize                 = 32,
   .maxTessellationControlPerVertexInputComponents = 128,
   .maxTessellationControlPerVertexOutputComponents = 128,
   .maxTessellationControlPerPatchOutputComponents = 128,
   .maxTessellationControlTotalOutputComponents = 2048,
   .maxTessellationEvaluationInputComponents = 128,
   .maxTessellationEvaluationOutputComponents = 128,
   .maxGeometryShaderInvocations             = 32,
   .maxGeometryInputComponents               = devinfo->ver >= 8 ? 128 : 64,
   .maxGeometryOutputComponents              = 128,
   .maxGeometryOutputVertices                = 256,
   .maxGeometryTotalOutputComponents         = 1024,
   .maxFragmentInputComponents               = 116, /* 128 components - (PSIZ, CLIP_DIST0, CLIP_DIST1) */
   .maxFragmentOutputAttachments             = 8,
   .maxFragmentDualSrcAttachments            = 1,
   .maxFragmentCombinedOutputResources       = MAX_RTS + max_ssbos + max_images,
   .maxComputeSharedMemorySize               = 64 * 1024,
   .maxComputeWorkGroupCount                 = { 65535, 65535, 65535 },
   .maxComputeWorkGroupInvocations           = max_workgroup_size,
   .maxComputeWorkGroupSize = {
      max_workgroup_size,
   max_workgroup_size,
      },
   .subPixelPrecisionBits                    = 8,
   .subTexelPrecisionBits                    = 8,
   .mipmapPrecisionBits                      = 8,
   .maxDrawIndexedIndexValue                 = UINT32_MAX,
   .maxDrawIndirectCount                     = UINT32_MAX,
   .maxSamplerLodBias                        = 16,
   .maxSamplerAnisotropy                     = 16,
   .maxViewports                             = MAX_VIEWPORTS,
   .maxViewportDimensions                    = { (1 << 14), (1 << 14) },
   .viewportBoundsRange                      = { INT16_MIN, INT16_MAX },
   .viewportSubPixelBits                     = 13, /* We take a float? */
   .minMemoryMapAlignment                    = 4096, /* A page */
   /* The dataport requires texel alignment so we need to assume a worst
   * case of R32G32B32A32 which is 16 bytes.
   */
   .minTexelBufferOffsetAlignment            = 16,
   .minUniformBufferOffsetAlignment          = ANV_UBO_ALIGNMENT,
   .minStorageBufferOffsetAlignment          = ANV_SSBO_ALIGNMENT,
   .minTexelOffset                           = -8,
   .maxTexelOffset                           = 7,
   .minTexelGatherOffset                     = -32,
   .maxTexelGatherOffset                     = 31,
   .minInterpolationOffset                   = -0.5,
   .maxInterpolationOffset                   = 0.4375,
   .subPixelInterpolationOffsetBits          = 4,
   .maxFramebufferWidth                      = (1 << 14),
   .maxFramebufferHeight                     = (1 << 14),
   .maxFramebufferLayers                     = (1 << 11),
   .framebufferColorSampleCounts             = sample_counts,
   .framebufferDepthSampleCounts             = sample_counts,
   .framebufferStencilSampleCounts           = sample_counts,
   .framebufferNoAttachmentsSampleCounts     = sample_counts,
   .maxColorAttachments                      = MAX_RTS,
   .sampledImageColorSampleCounts            = sample_counts,
   /* Multisampling with SINT formats is not supported on gfx7 */
   .sampledImageIntegerSampleCounts          = devinfo->ver == 7 ? VK_SAMPLE_COUNT_1_BIT : sample_counts,
   .sampledImageDepthSampleCounts            = sample_counts,
   .sampledImageStencilSampleCounts          = sample_counts,
   .storageImageSampleCounts                 = VK_SAMPLE_COUNT_1_BIT,
   .maxSampleMaskWords                       = 1,
   .timestampComputeAndGraphics              = true,
   .timestampPeriod                          = 1000000000.0 / devinfo->timestamp_frequency,
   .maxClipDistances                         = 8,
   .maxCullDistances                         = 8,
   .maxCombinedClipAndCullDistances          = 8,
   .discreteQueuePriorities                  = 2,
   .pointSizeRange                           = { 0.125, 255.875 },
   /* While SKL and up support much wider lines than we are setting here,
   * in practice we run into conformance issues if we go past this limit.
   * Since the Windows driver does the same, it's probably fair to assume
   * that no one needs more than this.
   */
   .lineWidthRange                           = { 0.0, devinfo->ver >= 9 ? 8.0 : 7.9921875 },
   .pointSizeGranularity                     = (1.0 / 8.0),
   .lineWidthGranularity                     = (1.0 / 128.0),
   .strictLines                              = false,
   .standardSampleLocations                  = true,
   .optimalBufferCopyOffsetAlignment         = 128,
   .optimalBufferCopyRowPitchAlignment       = 128,
                  #ifdef ANDROID
         #else
         #endif
         .driverVersion = vk_get_driver_version(),
   .vendorID = 0x8086,
   .deviceID = pdevice->info.pci_device_id,
   .deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
   .limits = limits,
               snprintf(pProperties->deviceName, sizeof(pProperties->deviceName),
         memcpy(pProperties->pipelineCacheUUID,
      }
      static void
   anv_get_physical_device_properties_1_1(struct anv_physical_device *pdevice,
         {
               memcpy(p->deviceUUID, pdevice->device_uuid, VK_UUID_SIZE);
   memcpy(p->driverUUID, pdevice->driver_uuid, VK_UUID_SIZE);
   memset(p->deviceLUID, 0, VK_LUID_SIZE);
   p->deviceNodeMask = 0;
            p->subgroupSize = BRW_SUBGROUP_SIZE;
   VkShaderStageFlags scalar_stages = 0;
   for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (pdevice->compiler->scalar_stage[stage])
      }
   p->subgroupSupportedStages = scalar_stages;
   p->subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT |
                                 if (pdevice->info.ver >= 8) {
      /* TODO: There's no technical reason why these can't be made to
   * work on gfx7 but they don't at the moment so it's best to leave
   * the feature disabled than enabled and broken.
   */
   p->subgroupSupportedOperations |= VK_SUBGROUP_FEATURE_ARITHMETIC_BIT |
      }
            p->pointClippingBehavior      = VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY;
   p->maxMultiviewViewCount      = 16;
   p->maxMultiviewInstanceIndex  = UINT32_MAX / 16;
   p->protectedNoFault           = false;
   /* This value doesn't matter for us today as our per-stage descriptors are
   * the real limit.
   */
   p->maxPerSetDescriptors       = 1024;
      }
      static void
   anv_get_physical_device_properties_1_2(struct anv_physical_device *pdevice,
         {
               p->driverID = VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA;
   memset(p->driverName, 0, sizeof(p->driverName));
   snprintf(p->driverName, VK_MAX_DRIVER_NAME_SIZE,
         memset(p->driverInfo, 0, sizeof(p->driverInfo));
   snprintf(p->driverInfo, VK_MAX_DRIVER_INFO_SIZE,
            /* Don't advertise conformance with a particular version if the hardware's
   * support is incomplete/alpha.
   */
   if (pdevice->is_alpha) {
      p->conformanceVersion = (VkConformanceVersion) {
      .major = 0,
   .minor = 0,
   .subminor = 0,
         }
   else {
      p->conformanceVersion = (VkConformanceVersion) {
      .major = 1,
   .minor = pdevice->use_softpin ? 3 : 2,
   .subminor = 0,
                  p->denormBehaviorIndependence =
         p->roundingModeIndependence =
            /* Broadwell does not support HF denorms and there are restrictions
   * other gens. According to Kabylake's PRM:
   *
   * "math - Extended Math Function
   * [...]
   * Restriction : Half-float denorms are always retained."
   */
   p->shaderDenormFlushToZeroFloat16         = false;
   p->shaderDenormPreserveFloat16            = pdevice->info.ver > 8;
   p->shaderRoundingModeRTEFloat16           = true;
   p->shaderRoundingModeRTZFloat16           = true;
            p->shaderDenormFlushToZeroFloat32         = true;
   p->shaderDenormPreserveFloat32            = pdevice->info.ver >= 8;
   p->shaderRoundingModeRTEFloat32           = true;
   p->shaderRoundingModeRTZFloat32           = true;
            p->shaderDenormFlushToZeroFloat64         = true;
   p->shaderDenormPreserveFloat64            = true;
   p->shaderRoundingModeRTEFloat64           = true;
   p->shaderRoundingModeRTZFloat64           = true;
            /* It's a bit hard to exactly map our implementation to the limits
   * described by Vulkan.  The bindless surface handle in the extended
   * message descriptors is 20 bits and it's an index into the table of
   * RENDER_SURFACE_STATE structs that starts at bindless surface base
   * address.  This means that we can have at must 1M surface states
   * allocated at any given time.  Since most image views take two
   * descriptors, this means we have a limit of about 500K image views.
   *
   * However, since we allocate surface states at vkCreateImageView time,
   * this means our limit is actually something on the order of 500K image
   * views allocated at any time.  The actual limit describe by Vulkan, on
   * the other hand, is a limit of how many you can have in a descriptor set.
   * Assuming anyone using 1M descriptors will be using the same image view
   * twice a bunch of times (or a bunch of null descriptors), we can safely
   * advertise a larger limit here.
   */
   const unsigned max_bindless_views = 1 << 20;
   p->maxUpdateAfterBindDescriptorsInAllPools            = max_bindless_views;
   p->shaderUniformBufferArrayNonUniformIndexingNative   = false;
   p->shaderSampledImageArrayNonUniformIndexingNative    = false;
   p->shaderStorageBufferArrayNonUniformIndexingNative   = true;
   p->shaderStorageImageArrayNonUniformIndexingNative    = false;
   p->shaderInputAttachmentArrayNonUniformIndexingNative = false;
   p->robustBufferAccessUpdateAfterBind                  = true;
   p->quadDivergentImplicitLod                           = false;
   p->maxPerStageDescriptorUpdateAfterBindSamplers       = max_bindless_views;
   p->maxPerStageDescriptorUpdateAfterBindUniformBuffers = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BUFFERS;
   p->maxPerStageDescriptorUpdateAfterBindStorageBuffers = UINT32_MAX;
   p->maxPerStageDescriptorUpdateAfterBindSampledImages  = max_bindless_views;
   p->maxPerStageDescriptorUpdateAfterBindStorageImages  = max_bindless_views;
   p->maxPerStageDescriptorUpdateAfterBindInputAttachments = MAX_PER_STAGE_DESCRIPTOR_INPUT_ATTACHMENTS;
   p->maxPerStageUpdateAfterBindResources                = UINT32_MAX;
   p->maxDescriptorSetUpdateAfterBindSamplers            = max_bindless_views;
   p->maxDescriptorSetUpdateAfterBindUniformBuffers      = 6 * MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BUFFERS;
   p->maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = MAX_DYNAMIC_BUFFERS / 2;
   p->maxDescriptorSetUpdateAfterBindStorageBuffers      = UINT32_MAX;
   p->maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = MAX_DYNAMIC_BUFFERS / 2;
   p->maxDescriptorSetUpdateAfterBindSampledImages       = max_bindless_views;
   p->maxDescriptorSetUpdateAfterBindStorageImages       = max_bindless_views;
            /* We support all of the depth resolve modes */
   p->supportedDepthResolveModes    = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT |
                     /* Average doesn't make sense for stencil so we don't support that */
   p->supportedStencilResolveModes  = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
   if (pdevice->info.ver >= 8) {
      /* The advanced stencil resolve modes currently require stencil
   * sampling be supported by the hardware.
   */
   p->supportedStencilResolveModes |= VK_RESOLVE_MODE_MIN_BIT |
      }
   p->independentResolveNone  = true;
            p->filterMinmaxSingleComponentFormats  = false;
                     p->framebufferIntegerColorSampleCounts =
      }
      static void
   anv_get_physical_device_properties_1_3(struct anv_physical_device *pdevice,
         {
               p->minSubgroupSize = 8;
   p->maxSubgroupSize = 32;
   p->maxComputeWorkgroupSubgroups = pdevice->info.max_cs_workgroup_threads;
            p->maxInlineUniformBlockSize = MAX_INLINE_UNIFORM_BLOCK_SIZE;
   p->maxPerStageDescriptorInlineUniformBlocks =
         p->maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks =
         p->maxDescriptorSetInlineUniformBlocks =
         p->maxDescriptorSetUpdateAfterBindInlineUniformBlocks =
                  p->integerDotProduct8BitUnsignedAccelerated = false;
   p->integerDotProduct8BitSignedAccelerated = false;
   p->integerDotProduct8BitMixedSignednessAccelerated = false;
   p->integerDotProduct4x8BitPackedUnsignedAccelerated = false;
   p->integerDotProduct4x8BitPackedSignedAccelerated = false;
   p->integerDotProduct4x8BitPackedMixedSignednessAccelerated = false;
   p->integerDotProduct16BitUnsignedAccelerated = false;
   p->integerDotProduct16BitSignedAccelerated = false;
   p->integerDotProduct16BitMixedSignednessAccelerated = false;
   p->integerDotProduct32BitUnsignedAccelerated = false;
   p->integerDotProduct32BitSignedAccelerated = false;
   p->integerDotProduct32BitMixedSignednessAccelerated = false;
   p->integerDotProduct64BitUnsignedAccelerated = false;
   p->integerDotProduct64BitSignedAccelerated = false;
   p->integerDotProduct64BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating8BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating8BitSignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating16BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating16BitSignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitSignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated = false;
   p->integerDotProductAccumulatingSaturating64BitUnsignedAccelerated = false;
   p->integerDotProductAccumulatingSaturating64BitSignedAccelerated = false;
            /* From the SKL PRM Vol. 2d, docs for RENDER_SURFACE_STATE::Surface
   * Base Address:
   *
   *    "For SURFTYPE_BUFFER non-rendertarget surfaces, this field
   *    specifies the base address of the first element of the surface,
   *    computed in software by adding the surface base address to the
   *    byte offset of the element in the buffer. The base address must
   *    be aligned to element size."
   *
   * The typed dataport messages require that things be texel aligned.
   * Otherwise, we may just load/store the wrong data or, in the worst
   * case, there may be hangs.
   */
   p->storageTexelBufferOffsetAlignmentBytes = 16;
            /* The sampler, however, is much more forgiving and it can handle
   * arbitrary byte alignment for linear and buffer surfaces.  It's
   * hard to find a good PRM citation for this but years of empirical
   * experience demonstrate that this is true.
   */
   p->uniformTexelBufferOffsetAlignmentBytes = 1;
               }
      void anv_GetPhysicalDeviceProperties2(
      VkPhysicalDevice                            physicalDevice,
      {
                        VkPhysicalDeviceVulkan11Properties core_1_1 = {
         };
            VkPhysicalDeviceVulkan12Properties core_1_2 = {
         };
            VkPhysicalDeviceVulkan13Properties core_1_3 = {
         };
            vk_foreach_struct(ext, pProperties->pNext) {
      if (vk_get_physical_device_core_1_1_property_ext(ext, &core_1_1))
         if (vk_get_physical_device_core_1_2_property_ext(ext, &core_1_2))
         if (vk_get_physical_device_core_1_3_property_ext(ext, &core_1_3))
            switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT: {
      VkPhysicalDeviceCustomBorderColorPropertiesEXT *properties =
         properties->maxCustomBorderColorSamplers = MAX_CUSTOM_BORDER_COLORS;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT: {
                     props->hasPrimary = pdevice->has_master;
                  props->hasRender = pdevice->has_local;
                              case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT: {
      VkPhysicalDeviceExternalMemoryHostPropertiesEXT *props =
         /* Userptr needs page aligned memory. */
   props->minImportedHostPointerAlignment = 4096;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT: {
      VkPhysicalDeviceLineRasterizationPropertiesEXT *props =
         /* In the Skylake PRM Vol. 7, subsection titled "GIQ (Diamond)
   * Sampling Rules - Legacy Mode", it says the following:
   *
   *    "Note that the device divides a pixel into a 16x16 array of
   *    subpixels, referenced by their upper left corners."
   *
   * This is the only known reference in the PRMs to the subpixel
   * precision of line rasterization and a "16x16 array of subpixels"
   * implies 4 subpixel precision bits.  Empirical testing has shown
   * that 4 subpixel precision bits applies to all line rasterization
   * types.
   */
   props->lineSubPixelPrecisionBits = 4;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES: {
      VkPhysicalDeviceMaintenance4Properties *properties =
         properties->maxBufferSize = pdevice->isl_dev.max_buffer_size;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT: {
      VkPhysicalDevicePCIBusInfoPropertiesEXT *properties =
         properties->pciDomain = pdevice->info.pci_domain;
   properties->pciBus = pdevice->info.pci_bus;
   properties->pciDevice = pdevice->info.pci_dev;
   properties->pciFunction = pdevice->info.pci_func;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR: {
      VkPhysicalDevicePerformanceQueryPropertiesKHR *properties =
         /* We could support this by spawning a shader to do the equation
   * normalization.
   */
   properties->allowCommandBufferQueryCopies = false;
         #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wswitch"
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID: {
      VkPhysicalDevicePresentationPropertiesANDROID *props =
         props->sharedImage = VK_FALSE;
      #pragma GCC diagnostic pop
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT: {
      VkPhysicalDeviceProvokingVertexPropertiesEXT *properties =
         properties->provokingVertexModePerPipeline = true;
   properties->transformFeedbackPreservesTriangleFanProvokingVertex = false;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR: {
      VkPhysicalDevicePushDescriptorPropertiesKHR *properties =
         properties->maxPushDescriptors = MAX_PUSH_DESCRIPTORS;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_PROPERTIES_EXT: {
      VkPhysicalDeviceRobustness2PropertiesEXT *properties = (void *)ext;
   properties->robustStorageBufferAccessSizeAlignment =
         properties->robustUniformBufferAccessSizeAlignment =
                     case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLE_LOCATIONS_PROPERTIES_EXT: {
                                    /* See also anv_GetPhysicalDeviceMultisamplePropertiesEXT */
                  props->sampleLocationCoordinateRange[0] = 0;
                  props->variableSampleLocations = true;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT: {
      VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT *props =
         STATIC_ASSERT(sizeof(vk_shaderModuleIdentifierAlgorithmUUID) ==
         memcpy(props->shaderModuleIdentifierAlgorithmUUID,
         vk_shaderModuleIdentifierAlgorithmUUID,
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_PROPERTIES_EXT: {
                     props->maxTransformFeedbackStreams = MAX_XFB_STREAMS;
   props->maxTransformFeedbackBuffers = MAX_XFB_BUFFERS;
   props->maxTransformFeedbackBufferSize = (1ull << 32);
   props->maxTransformFeedbackStreamDataSize = 128 * 4;
   props->maxTransformFeedbackBufferDataSize = 128 * 4;
   props->maxTransformFeedbackBufferDataStride = 2048;
   props->transformFeedbackQueries = true;
   props->transformFeedbackStreamsLinesTriangles = false;
   props->transformFeedbackRasterizationStreamSelect = false;
   /* This requires MI_MATH */
   props->transformFeedbackDraw = pdevice->info.verx10 >= 75;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT: {
      VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *props =
         /* We have to restrict this a bit for multiview */
   props->maxVertexAttribDivisor = UINT32_MAX / 16;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTI_DRAW_PROPERTIES_EXT: {
      VkPhysicalDeviceMultiDrawPropertiesEXT *props = (VkPhysicalDeviceMultiDrawPropertiesEXT *)ext;
   props->maxMultiDrawCount = 2048;
               default:
      anv_debug_ignored_stype(ext->sType);
            }
      static int
   vk_priority_to_gen(int priority)
   {
      switch (priority) {
   case VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR:
         case VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR:
         case VK_QUEUE_GLOBAL_PRIORITY_HIGH_KHR:
         case VK_QUEUE_GLOBAL_PRIORITY_REALTIME_KHR:
         default:
            }
      static const VkQueueFamilyProperties
   anv_queue_family_properties_template = {
      .timestampValidBits = 36, /* XXX: Real value here */
      };
      void anv_GetPhysicalDeviceQueueFamilyProperties2(
      VkPhysicalDevice                            physicalDevice,
   uint32_t*                                   pQueueFamilyPropertyCount,
      {
      ANV_FROM_HANDLE(anv_physical_device, pdevice, physicalDevice);
   VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out,
            for (uint32_t i = 0; i < pdevice->queue.family_count; i++) {
      struct anv_queue_family *queue_family = &pdevice->queue.families[i];
   vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p) {
      p->queueFamilyProperties = anv_queue_family_properties_template;
                  vk_foreach_struct(ext, p->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_GLOBAL_PRIORITY_PROPERTIES_KHR: {
                     /* Deliberately sorted low to high */
   VkQueueGlobalPriorityKHR all_priorities[] = {
      VK_QUEUE_GLOBAL_PRIORITY_LOW_KHR,
   VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR,
                     uint32_t count = 0;
   for (unsigned i = 0; i < ARRAY_SIZE(all_priorities); i++) {
                              }
                     default:
                     }
      void anv_GetPhysicalDeviceMemoryProperties(
      VkPhysicalDevice                            physicalDevice,
      {
               pMemoryProperties->memoryTypeCount = physical_device->memory.type_count;
   for (uint32_t i = 0; i < physical_device->memory.type_count; i++) {
      pMemoryProperties->memoryTypes[i] = (VkMemoryType) {
      .propertyFlags = physical_device->memory.types[i].propertyFlags,
                  pMemoryProperties->memoryHeapCount = physical_device->memory.heap_count;
   for (uint32_t i = 0; i < physical_device->memory.heap_count; i++) {
      pMemoryProperties->memoryHeaps[i] = (VkMemoryHeap) {
      .size    = physical_device->memory.heaps[i].size,
            }
      static void
   anv_get_memory_budget(VkPhysicalDevice physicalDevice,
         {
               if (!device->vk.supported_extensions.EXT_memory_budget)
                     VkDeviceSize total_sys_heaps_size = 0;
   for (size_t i = 0; i < device->memory.heap_count; i++)
            for (size_t i = 0; i < device->memory.heap_count; i++) {
      VkDeviceSize heap_size = device->memory.heaps[i].size;
   VkDeviceSize heap_used = device->memory.heaps[i].used;
   VkDeviceSize heap_budget, total_heaps_size;
            total_heaps_size = total_sys_heaps_size;
            double heap_proportion = (double) heap_size / total_heaps_size;
            /*
   * Let's not incite the app to starve the system: report at most 90% of
   * the available heap memory.
   */
   uint64_t heap_available = available_prop * 9 / 10;
            /*
   * Round down to the nearest MB
   */
            /*
   * The heapBudget value must be non-zero for array elements less than
   * VkPhysicalDeviceMemoryProperties::memoryHeapCount. The heapBudget
   * value must be less than or equal to VkMemoryHeap::size for each heap.
   */
            memoryBudget->heapUsage[i] = heap_used;
               /* The heapBudget and heapUsage values must be zero for array elements
   * greater than or equal to VkPhysicalDeviceMemoryProperties::memoryHeapCount
   */
   for (uint32_t i = device->memory.heap_count; i < VK_MAX_MEMORY_HEAPS; i++) {
      memoryBudget->heapBudget[i] = 0;
         }
      void anv_GetPhysicalDeviceMemoryProperties2(
      VkPhysicalDevice                            physicalDevice,
      {
      anv_GetPhysicalDeviceMemoryProperties(physicalDevice,
            vk_foreach_struct(ext, pMemoryProperties->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT:
      anv_get_memory_budget(physicalDevice, (void*)ext);
      default:
      anv_debug_ignored_stype(ext->sType);
            }
      PFN_vkVoidFunction anv_GetInstanceProcAddr(
      VkInstance                                  _instance,
      {
      ANV_FROM_HANDLE(anv_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk,
            }
      /* With version 1+ of the loader interface the ICD should expose
   * vk_icdGetInstanceProcAddr to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(
      VkInstance                                  instance,
         PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(
      VkInstance                                  instance,
      {
         }
      /* With version 4+ of the loader interface the ICD should expose
   * vk_icdGetPhysicalDeviceProcAddr()
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(
      VkInstance  _instance,
         PFN_vkVoidFunction vk_icdGetPhysicalDeviceProcAddr(
      VkInstance  _instance,
      {
      ANV_FROM_HANDLE(anv_instance, instance, _instance);
      }
      static struct anv_state
   anv_state_pool_emit_data(struct anv_state_pool *pool, size_t size, size_t align, const void *p)
   {
               state = anv_state_pool_alloc(pool, size, align);
               }
      static void
   anv_device_init_border_colors(struct anv_device *device)
   {
      if (device->info->platform == INTEL_PLATFORM_HSW) {
      static const struct hsw_border_color border_colors[] = {
      [VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK] =  { .float32 = { 0.0, 0.0, 0.0, 0.0 } },
   [VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK] =       { .float32 = { 0.0, 0.0, 0.0, 1.0 } },
   [VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE] =       { .float32 = { 1.0, 1.0, 1.0, 1.0 } },
   [VK_BORDER_COLOR_INT_TRANSPARENT_BLACK] =    { .uint32 = { 0, 0, 0, 0 } },
   [VK_BORDER_COLOR_INT_OPAQUE_BLACK] =         { .uint32 = { 0, 0, 0, 1 } },
               device->border_colors =
      anv_state_pool_emit_data(&device->dynamic_state_pool,
   } else {
      static const struct gfx8_border_color border_colors[] = {
      [VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK] =  { .float32 = { 0.0, 0.0, 0.0, 0.0 } },
   [VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK] =       { .float32 = { 0.0, 0.0, 0.0, 1.0 } },
   [VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE] =       { .float32 = { 1.0, 1.0, 1.0, 1.0 } },
   [VK_BORDER_COLOR_INT_TRANSPARENT_BLACK] =    { .uint32 = { 0, 0, 0, 0 } },
   [VK_BORDER_COLOR_INT_OPAQUE_BLACK] =         { .uint32 = { 0, 0, 0, 1 } },
               device->border_colors =
      anv_state_pool_emit_data(&device->dynamic_state_pool,
      }
      static VkResult
   anv_device_init_trivial_batch(struct anv_device *device)
   {
      VkResult result = anv_device_alloc_bo(device, "trivial-batch", 4096,
                     if (result != VK_SUCCESS)
            struct anv_batch batch = {
      .start = device->trivial_batch_bo->map,
   .next = device->trivial_batch_bo->map,
               anv_batch_emit(&batch, GFX7_MI_BATCH_BUFFER_END, bbe);
         #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
            }
      static bool
   get_bo_from_pool(struct intel_batch_decode_bo *ret,
               {
      anv_block_pool_foreach_bo(bo, pool) {
      uint64_t bo_address = intel_48b_address(bo->offset);
   if (address >= bo_address && address < (bo_address + bo->size)) {
      *ret = (struct intel_batch_decode_bo) {
      .addr = bo_address,
   .size = bo->size,
      };
         }
      }
      /* Finding a buffer for batch decoding */
   static struct intel_batch_decode_bo
   decode_get_bo(void *v_batch, bool ppgtt, uint64_t address)
   {
      struct anv_device *device = v_batch;
                     if (get_bo_from_pool(&ret_bo, &device->dynamic_state_pool.block_pool, address))
         if (get_bo_from_pool(&ret_bo, &device->instruction_state_pool.block_pool, address))
         if (get_bo_from_pool(&ret_bo, &device->binding_table_pool.block_pool, address))
         if (get_bo_from_pool(&ret_bo, &device->surface_state_pool.block_pool, address))
            if (!device->cmd_buffer_being_decoded)
                     u_vector_foreach(bo, &device->cmd_buffer_being_decoded->seen_bbos) {
      /* The decoder zeroes out the top 16 bits, so we need to as well */
            if (address >= bo_address && address < bo_address + (*bo)->bo->size) {
      return (struct intel_batch_decode_bo) {
      .addr = bo_address,
   .size = (*bo)->bo->size,
                        }
      static VkResult anv_device_check_status(struct vk_device *vk_device);
      static VkResult
   anv_device_setup_context(struct anv_device *device,
               {
      struct anv_physical_device *physical_device = device->physical;
            if (device->physical->engine_info) {
      /* The kernel API supports at most 64 engines */
   assert(num_queues <= 64);
   enum intel_engine_class engine_classes[64];
   int engine_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
                     assert(queueCreateInfo->queueFamilyIndex <
                        for (uint32_t j = 0; j < queueCreateInfo->queueCount; j++)
      }
   if (!intel_gem_create_context_engines(device->fd, 0 /* flags */,
                              result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
   } else {
      assert(num_queues == 1);
   if (!intel_gem_create_context(device->fd, &device->context_id))
               if (result != VK_SUCCESS)
            /* Here we tell the kernel not to attempt to recover our context but
   * immediately (on the next batchbuffer submission) report that the
   * context is lost, and we will do the recovery ourselves.  In the case
   * of Vulkan, recovery means throwing VK_ERROR_DEVICE_LOST and letting
   * the client clean up the pieces.
   */
   anv_gem_set_context_param(device->fd, device->context_id,
            /* Check if client specified queue priority. */
   const VkDeviceQueueGlobalPriorityCreateInfoKHR *queue_priority =
      vk_find_struct_const(pCreateInfo->pQueueCreateInfos[0].pNext,
         VkQueueGlobalPriorityKHR priority =
      queue_priority ? queue_priority->globalPriority :
         /* As per spec, the driver implementation may deny requests to acquire
   * a priority above the default priority (MEDIUM) if the caller does not
   * have sufficient privileges. In this scenario VK_ERROR_NOT_PERMITTED_KHR
   * is returned.
   */
   if (physical_device->max_context_priority >= INTEL_CONTEXT_MEDIUM_PRIORITY) {
      int err = anv_gem_set_context_param(device->fd, device->context_id,
               if (err != 0 && priority > VK_QUEUE_GLOBAL_PRIORITY_MEDIUM_KHR) {
      result = vk_error(device, VK_ERROR_NOT_PERMITTED_KHR);
                        fail_context:
      intel_gem_destroy_context(device->fd, device->context_id);
      }
      VkResult anv_CreateDevice(
      VkPhysicalDevice                            physicalDevice,
   const VkDeviceCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   VkResult result;
                     /* Check requested queues and fail if we are requested to create any
   * queues with flags we don't support.
   */
   assert(pCreateInfo->queueCreateInfoCount > 0);
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      if (pCreateInfo->pQueueCreateInfos[i].flags != 0)
               device = vk_zalloc2(&physical_device->instance->vk.alloc, pAllocator,
               if (!device)
                     bool override_initial_entrypoints = true;
   if (physical_device->instance->vk.app_info.app_name &&
      !strcmp(physical_device->instance->vk.app_info.app_name, "DOOM 64")) {
   vk_device_dispatch_table_from_entrypoints(&dispatch_table, &doom64_device_entrypoints, true);
      }
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
      anv_genX(&physical_device->info, device_entrypoints),
      vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
            result = vk_device_init(&device->vk, &physical_device->vk,
         if (result != VK_SUCCESS)
            if (INTEL_DEBUG(DEBUG_BATCH)) {
               intel_batch_decode_ctx_init(&device->decoder_ctx,
                              device->decoder_ctx.dynamic_base = DYNAMIC_STATE_POOL_MIN_ADDRESS;
   device->decoder_ctx.surface_base = SURFACE_STATE_POOL_MIN_ADDRESS;
   device->decoder_ctx.instruction_base =
                        /* XXX(chadv): Can we dup() physicalDevice->fd here? */
   device->fd = open(physical_device->path, O_RDWR | O_CLOEXEC);
   if (device->fd == -1) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
               device->vk.command_buffer_ops = &anv_cmd_buffer_ops;
   device->vk.check_status = anv_device_check_status;
   device->vk.create_sync_for_memory = anv_create_sync_for_memory;
            uint32_t num_queues = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
            result = anv_device_setup_context(device, pCreateInfo, num_queues);
   if (result != VK_SUCCESS)
            device->queues =
      vk_zalloc(&device->vk.alloc, num_queues * sizeof(*device->queues), 8,
      if (device->queues == NULL) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               device->queue_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const VkDeviceQueueCreateInfo *queueCreateInfo =
            for (uint32_t j = 0; j < queueCreateInfo->queueCount; j++) {
      /* When using legacy contexts, we use I915_EXEC_RENDER but, with
   * engine-based contexts, the bottom 6 bits of exec_flags are used
   * for the engine ID.
   */
                  result = anv_queue_init(device, &device->queues[device->queue_count],
                                       if (!anv_use_relocations(physical_device)) {
      if (pthread_mutex_init(&device->vma_mutex, NULL) != 0) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
               /* keep the page with address zero out of the allocator */
   util_vma_heap_init(&device->vma_lo,
            util_vma_heap_init(&device->vma_cva, CLIENT_VISIBLE_HEAP_MIN_ADDRESS,
            /* Leave the last 4GiB out of the high vma range, so that no state
   * base address + size can overflow 48 bits. For more information see
   * the comment about Wa32bitGeneralStateOffset in anv_allocator.c
   */
   util_vma_heap_init(&device->vma_hi, HIGH_HEAP_MIN_ADDRESS,
                              /* On Broadwell and later, we can use batch chaining to more efficiently
   * implement growing command buffers.  Prior to Haswell, the kernel
   * command parser gets in the way and we have to fall back to growing
   * the batch.
   */
            if (pthread_mutex_init(&device->mutex, NULL) != 0) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
               pthread_condattr_t condattr;
   if (pthread_condattr_init(&condattr) != 0) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
      }
   if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) != 0) {
      pthread_condattr_destroy(&condattr);
   result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
      }
   if (pthread_cond_init(&device->queue_submit, &condattr) != 0) {
      pthread_condattr_destroy(&condattr);
   result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
      }
            result = anv_bo_cache_init(&device->bo_cache, device);
   if (result != VK_SUCCESS)
                     /* Because scratch is also relative to General State Base Address, we leave
   * the base address 0 and start the pool memory at an offset.  This way we
   * get the correct offsets in the anv_states that get allocated from it.
   */
   result = anv_state_pool_init(&device->general_state_pool, device,
               if (result != VK_SUCCESS)
            result = anv_state_pool_init(&device->dynamic_state_pool, device,
               if (result != VK_SUCCESS)
            if (device->info->ver >= 8) {
      /* The border color pointer is limited to 24 bits, so we need to make
   * sure that any such color used at any point in the program doesn't
   * exceed that limit.
   * We achieve that by reserving all the custom border colors we support
   * right off the bat, so they are close to the base address.
   */
   anv_state_reserved_pool_init(&device->custom_border_colors,
                           result = anv_state_pool_init(&device->instruction_state_pool, device,
               if (result != VK_SUCCESS)
            result = anv_state_pool_init(&device->surface_state_pool, device,
               if (result != VK_SUCCESS)
            if (!anv_use_relocations(physical_device)) {
      int64_t bt_pool_offset = (int64_t)BINDING_TABLE_POOL_MIN_ADDRESS -
         assert(INT32_MIN < bt_pool_offset && bt_pool_offset < 0);
   result = anv_state_pool_init(&device->binding_table_pool, device,
                        }
   if (result != VK_SUCCESS)
            result = anv_device_alloc_bo(device, "workaround", 4096,
                           if (result != VK_SUCCESS)
            device->workaround_address = (struct anv_address) {
      .bo = device->workaround_bo,
   .offset = align(intel_debug_write_identifiers(device->workaround_bo->map,
                              device->debug_frame_desc =
      intel_debug_get_identifier_block(device->workaround_bo->map,
               result = anv_device_init_trivial_batch(device);
   if (result != VK_SUCCESS)
            /* Allocate a null surface state at surface state offset 0.  This makes
   * NULL descriptor handling trivial because we can just memset structures
   * to zero and they have a valid descriptor.
   */
   device->null_surface_state =
      anv_state_pool_alloc(&device->surface_state_pool,
            isl_null_fill_state(&device->isl_dev, device->null_surface_state.map,
                           result = anv_genX(device->info, init_device_state)(device);
   if (result != VK_SUCCESS)
            struct vk_pipeline_cache_create_info pcc_info = { };
   device->default_pipeline_cache =
         if (!device->default_pipeline_cache) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               /* Internal shaders need their own pipeline cache because, unlike the rest
   * of ANV, it won't work at all without the cache. It depends on it for
   * shaders to remain resident while it runs. Therefore, we need a special
   * cache just for BLORP/RT that's forced to always be enabled.
   */
   pcc_info.force_enable = true;
   device->internal_cache =
         if (device->internal_cache == NULL) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               device->robust_buffer_access =
      device->vk.enabled_features.robustBufferAccess ||
                                                            fail_default_pipeline_cache:
         fail_trivial_batch_bo_and_scratch_pool:
      anv_scratch_pool_finish(device, &device->scratch_pool);
      fail_workaround_bo:
         fail_binding_table_pool:
      if (!anv_use_relocations(physical_device))
      fail_surface_state_pool:
         fail_instruction_state_pool:
         fail_dynamic_state_pool:
      if (device->info->ver >= 8)
            fail_general_state_pool:
         fail_batch_bo_pool:
      anv_bo_pool_finish(&device->batch_bo_pool);
      fail_queue_cond:
         fail_mutex:
         fail_vmas:
      if (!anv_use_relocations(physical_device)) {
      util_vma_heap_finish(&device->vma_hi);
   util_vma_heap_finish(&device->vma_cva);
         fail_queues:
      for (uint32_t i = 0; i < device->queue_count; i++)
            fail_context_id:
         fail_fd:
         fail_device:
         fail_alloc:
                  }
      void anv_DestroyDevice(
      VkDevice                                    _device,
      {
               if (!device)
                              vk_pipeline_cache_destroy(device->internal_cache, NULL);
         #ifdef HAVE_VALGRIND
      /* We only need to free these to prevent valgrind errors.  The backing
   * BO will go away in a couple of lines so we don't actually leak.
   */
   if (device->info->ver >= 8)
         anv_state_pool_free(&device->dynamic_state_pool, device->border_colors);
      #endif
                  anv_device_release_bo(device, device->workaround_bo);
            if (!anv_use_relocations(device->physical))
         anv_state_pool_finish(&device->surface_state_pool);
   anv_state_pool_finish(&device->instruction_state_pool);
   anv_state_pool_finish(&device->dynamic_state_pool);
                              if (!anv_use_relocations(device->physical)) {
      util_vma_heap_finish(&device->vma_hi);
   util_vma_heap_finish(&device->vma_cva);
               pthread_cond_destroy(&device->queue_submit);
            for (uint32_t i = 0; i < device->queue_count; i++)
                           if (INTEL_DEBUG(DEBUG_BATCH))
                     vk_device_finish(&device->vk);
      }
      VkResult anv_EnumerateInstanceLayerProperties(
      uint32_t*                                   pPropertyCount,
      {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
      }
      static VkResult
   anv_device_check_status(struct vk_device *vk_device)
   {
               uint32_t active, pending;
   int ret = anv_gem_context_get_reset_stats(device->fd, device->context_id,
         if (ret == -1) {
      /* We don't know the real error. */
               if (active) {
         } else if (pending) {
                     }
      VkResult
   anv_device_wait(struct anv_device *device, struct anv_bo *bo,
         {
      int ret = anv_gem_wait(device, bo->gem_handle, &timeout);
   if (ret == -1 && errno == ETIME) {
         } else if (ret == -1) {
      /* We don't know the real error. */
      } else {
            }
      uint64_t
   anv_vma_alloc(struct anv_device *device,
               uint64_t size, uint64_t align,
   {
                        if (alloc_flags & ANV_BO_ALLOC_CLIENT_VISIBLE_ADDRESS) {
      if (client_address) {
      if (util_vma_heap_alloc_addr(&device->vma_cva,
                  } else {
         }
   /* We don't want to fall back to other heaps */
                        if (!(alloc_flags & ANV_BO_ALLOC_32BIT_ADDRESS))
            if (addr == 0)
         done:
               assert(addr == intel_48b_address(addr));
      }
      void
   anv_vma_free(struct anv_device *device,
         {
                        if (addr_48b >= LOW_HEAP_MIN_ADDRESS &&
      addr_48b <= LOW_HEAP_MAX_ADDRESS) {
      } else if (addr_48b >= CLIENT_VISIBLE_HEAP_MIN_ADDRESS &&
               } else {
      assert(addr_48b >= HIGH_HEAP_MIN_ADDRESS);
                  }
      VkResult anv_AllocateMemory(
      VkDevice                                    _device,
   const VkMemoryAllocateInfo*                 pAllocateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_physical_device *pdevice = device->physical;
   struct anv_device_memory *mem;
                     /* The Vulkan 1.0.33 spec says "allocationSize must be greater than 0". */
            VkDeviceSize aligned_alloc_size =
            if (aligned_alloc_size > MAX_MEMORY_ALLOCATION_SIZE)
            assert(pAllocateInfo->memoryTypeIndex < pdevice->memory.type_count);
   struct anv_memory_type *mem_type =
         assert(mem_type->heapIndex < pdevice->memory.heap_count);
   struct anv_memory_heap *mem_heap =
            uint64_t mem_heap_used = p_atomic_read(&mem_heap->used);
   if (mem_heap_used + aligned_alloc_size > mem_heap->size)
            mem = vk_object_alloc(&device->vk, pAllocator, sizeof(*mem),
         if (mem == NULL)
            mem->type = mem_type;
   mem->map = NULL;
   mem->map_size = 0;
   mem->map_delta = 0;
   mem->ahw = NULL;
                     const VkExportMemoryAllocateInfo *export_info = NULL;
   const VkImportAndroidHardwareBufferInfoANDROID *ahw_import_info = NULL;
   const VkImportMemoryFdInfoKHR *fd_info = NULL;
   const VkImportMemoryHostPointerInfoEXT *host_ptr_info = NULL;
   const VkMemoryDedicatedAllocateInfo *dedicated_info = NULL;
   VkMemoryAllocateFlags vk_flags = 0;
            vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
                  case VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID:
                  case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
                  case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT:
                  case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO: {
      const VkMemoryAllocateFlagsInfo *flags_info = (void *)ext;
   vk_flags = flags_info->flags;
               case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
                  case VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO: {
      const VkMemoryOpaqueCaptureAddressAllocateInfo *addr_info =
         client_address = addr_info->opaqueCaptureAddress;
               default:
      if (ext->sType != VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA)
      /* this isn't a real enum value,
   * so use conditional to avoid compiler warn
   */
                     if (vk_flags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)
            if ((export_info && export_info->handleTypes) ||
      (fd_info && fd_info->handleType) ||
   (host_ptr_info && host_ptr_info->handleType)) {
   /* Anything imported or exported is EXTERNAL */
               /* Check if we need to support Android HW buffer export. If so,
   * create AHardwareBuffer and import memory from it.
   */
   bool android_export = false;
   if (export_info && export_info->handleTypes &
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)
         if (ahw_import_info) {
      result = anv_import_ahw_memory(_device, mem, ahw_import_info);
   if (result != VK_SUCCESS)
               } else if (android_export) {
      result = anv_create_ahw_memory(_device, mem, pAllocateInfo);
   if (result != VK_SUCCESS)
                        /* The Vulkan spec permits handleType to be 0, in which case the struct is
   * ignored.
   */
   if (fd_info && fd_info->handleType) {
      /* At the moment, we support only the below handle types. */
   assert(fd_info->handleType ==
                        result = anv_device_import_bo(device, fd_info->fd, alloc_flags,
         if (result != VK_SUCCESS)
            /* For security purposes, we reject importing the bo if it's smaller
   * than the requested allocation size.  This prevents a malicious client
   * from passing a buffer to a trusted client, lying about the size, and
   * telling the trusted client to try and texture from an image that goes
   * out-of-bounds.  This sort of thing could lead to GPU hangs or worse
   * in the trusted client.  The trusted client can protect itself against
   * this sort of attack but only if it can trust the buffer size.
   */
   if (mem->bo->size < aligned_alloc_size) {
      result = vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                     "aligned allocationSize too large for "
   anv_device_release_bo(device, mem->bo);
               /* From the Vulkan spec:
   *
   *    "Importing memory from a file descriptor transfers ownership of
   *    the file descriptor from the application to the Vulkan
   *    implementation. The application must not perform any operations on
   *    the file descriptor after a successful import."
   *
   * If the import fails, we leave the file descriptor open.
   */
   close(fd_info->fd);
               if (host_ptr_info && host_ptr_info->handleType) {
      if (host_ptr_info->handleType ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT) {
   result = vk_error(device, VK_ERROR_INVALID_EXTERNAL_HANDLE);
               assert(host_ptr_info->handleType ==
            result = anv_device_import_bo_from_host_ptr(device,
                                 if (result != VK_SUCCESS)
            mem->host_ptr = host_ptr_info->pHostPointer;
                        result = anv_device_alloc_bo(device, "user", pAllocateInfo->allocationSize,
         if (result != VK_SUCCESS)
            if (dedicated_info && dedicated_info->image != VK_NULL_HANDLE) {
               /* Some legacy (non-modifiers) consumers need the tiling to be set on
   * the BO.  In this case, we have a dedicated allocation.
   */
   if (image->vk.wsi_legacy_scanout) {
      const struct isl_surf *surf = &image->planes[0].primary_surface.isl;
   result = anv_device_set_bo_tiling(device, mem->bo,
               if (result != VK_SUCCESS) {
      anv_device_release_bo(device, mem->bo);
                  success:
      mem_heap_used = p_atomic_add_return(&mem_heap->used, mem->bo->size);
   if (mem_heap_used > mem_heap->size) {
      p_atomic_add(&mem_heap->used, -mem->bo->size);
   anv_device_release_bo(device, mem->bo);
   result = vk_errorf(device, VK_ERROR_OUT_OF_DEVICE_MEMORY,
                     pthread_mutex_lock(&device->mutex);
   list_addtail(&mem->link, &device->memory_objects);
                           fail:
                  }
      VkResult anv_GetMemoryFdKHR(
      VkDevice                                    device_h,
   const VkMemoryGetFdInfoKHR*                 pGetFdInfo,
      {
      ANV_FROM_HANDLE(anv_device, dev, device_h);
                     assert(pGetFdInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
               }
      VkResult anv_GetMemoryFdPropertiesKHR(
      VkDevice                                    _device,
   VkExternalMemoryHandleTypeFlagBits          handleType,
   int                                         fd,
      {
               switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      /* dma-buf can be imported as any memory type */
   pMemoryFdProperties->memoryTypeBits =
               default:
      /* The valid usage section for this function says:
   *
   *    "handleType must not be one of the handle types defined as
   *    opaque."
   *
   * So opaque handle types fall into the default "unsupported" case.
   */
         }
      VkResult anv_GetMemoryHostPointerPropertiesEXT(
      VkDevice                                    _device,
   VkExternalMemoryHandleTypeFlagBits          handleType,
   const void*                                 pHostPointer,
      {
               assert(pMemoryHostPointerProperties->sType ==
            switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      /* Host memory can be imported as any memory type. */
   pMemoryHostPointerProperties->memoryTypeBits =
                  default:
            }
      void anv_FreeMemory(
      VkDevice                                    _device,
   VkDeviceMemory                              _mem,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (mem == NULL)
            pthread_mutex_lock(&device->mutex);
   list_del(&mem->link);
            if (mem->map)
            p_atomic_add(&device->physical->memory.heaps[mem->type->heapIndex].used,
                  #if defined(ANDROID) && ANDROID_API_LEVEL >= 26
      if (mem->ahw)
      #endif
            }
      VkResult anv_MapMemory(
      VkDevice                                    _device,
   VkDeviceMemory                              _memory,
   VkDeviceSize                                offset,
   VkDeviceSize                                size,
   VkMemoryMapFlags                            flags,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (mem == NULL) {
      *ppData = NULL;
               if (mem->host_ptr) {
      *ppData = mem->host_ptr + offset;
               /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * memory must have been created with a memory type that reports
   *    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
   */
   if (!(mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
               if (size == VK_WHOLE_SIZE)
            /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * If size is not equal to VK_WHOLE_SIZE, size must be greater than 0
   *    assert(size != 0);
   *  * If size is not equal to VK_WHOLE_SIZE, size must be less than or
   *    equal to the size of the memory minus offset
   */
   assert(size > 0);
            if (size != (size_t)size) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
                     /* From the Vulkan 1.2.194 spec:
   *
   *    "memory must not be currently host mapped"
   */
   if (mem->map != NULL) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
                        if (!device->info->has_llc &&
      (mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
         /* GEM will fail to map if the offset isn't 4k-aligned.  Round down. */
   uint64_t map_offset;
   if (!device->physical->info.has_mmap_offset)
         else
         assert(offset >= map_offset);
            /* Let's map whole pages */
            void *map;
   VkResult result = anv_device_map_bo(device, mem->bo, map_offset,
         if (result != VK_SUCCESS)
            mem->map = map;
   mem->map_size = map_size;
   mem->map_delta = (offset - map_offset);
               }
      void anv_UnmapMemory(
      VkDevice                                    _device,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (mem == NULL || mem->host_ptr)
                     mem->map = NULL;
   mem->map_size = 0;
      }
      VkResult anv_FlushMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
               if (!device->physical->memory.need_flush)
         #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      /* Make sure the writes we're flushing have landed. */
      #endif
         for (uint32_t i = 0; i < memoryRangeCount; i++) {
      ANV_FROM_HANDLE(anv_device_memory, mem, pMemoryRanges[i].memory);
   if (mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            uint64_t map_offset = pMemoryRanges[i].offset + mem->map_delta;
   if (map_offset >= mem->map_size)
      #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
         intel_flush_range(mem->map + map_offset,
         #endif
                  }
      VkResult anv_InvalidateMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
               if (!device->physical->memory.need_flush)
            for (uint32_t i = 0; i < memoryRangeCount; i++) {
      ANV_FROM_HANDLE(anv_device_memory, mem, pMemoryRanges[i].memory);
   if (mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            uint64_t map_offset = pMemoryRanges[i].offset + mem->map_delta;
   if (map_offset >= mem->map_size)
      #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
         intel_invalidate_range(mem->map + map_offset,
         #endif
            #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      /* Make sure no reads get moved up above the invalidate. */
      #endif
            }
      void anv_GetDeviceMemoryCommitment(
      VkDevice                                    device,
   VkDeviceMemory                              memory,
      {
         }
      static void
   anv_bind_buffer_memory(const VkBindBufferMemoryInfo *pBindInfo)
   {
      ANV_FROM_HANDLE(anv_device_memory, mem, pBindInfo->memory);
                     if (mem) {
      assert(pBindInfo->memoryOffset < mem->bo->size);
   assert(mem->bo->size - pBindInfo->memoryOffset >= buffer->vk.size);
   buffer->address = (struct anv_address) {
      .bo = mem->bo,
         } else {
            }
      VkResult anv_BindBufferMemory2(
      VkDevice                                    device,
   uint32_t                                    bindInfoCount,
      {
      for (uint32_t i = 0; i < bindInfoCount; i++)
               }
      VkResult anv_QueueBindSparse(
      VkQueue                                     _queue,
   uint32_t                                    bindInfoCount,
   const VkBindSparseInfo*                     pBindInfo,
      {
      ANV_FROM_HANDLE(anv_queue, queue, _queue);
   if (vk_device_is_lost(&queue->device->vk))
               }
      // Event functions
      VkResult anv_CreateEvent(
      VkDevice                                    _device,
   const VkEventCreateInfo*                    pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     event = vk_object_alloc(&device->vk, pAllocator, sizeof(*event),
         if (event == NULL)
            event->state = anv_state_pool_alloc(&device->dynamic_state_pool,
                              }
      void anv_DestroyEvent(
      VkDevice                                    _device,
   VkEvent                                     _event,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!event)
                        }
      VkResult anv_GetEventStatus(
      VkDevice                                    _device,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (vk_device_is_lost(&device->vk))
               }
      VkResult anv_SetEvent(
      VkDevice                                    _device,
      {
                           }
      VkResult anv_ResetEvent(
      VkDevice                                    _device,
      {
                           }
      // Buffer functions
      static void
   anv_get_buffer_memory_requirements(struct anv_device *device,
                     {
      /* The Vulkan spec (git aaed022) says:
   *
   *    memoryTypeBits is a bitfield and contains one bit set for every
   *    supported memory type for the resource. The bit `1<<i` is set if and
   *    only if the memory type `i` in the VkPhysicalDeviceMemoryProperties
   *    structure for the physical device is supported.
   */
            /* Base alignment requirement of a cache line */
            if (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            pMemoryRequirements->memoryRequirements.size = size;
            /* Storage and Uniform buffers should have their size aligned to
   * 32-bits to avoid boundary checks when last DWord is not complete.
   * This would ensure that not internal padding would be needed for
   * 16-bit types.
   */
   if (device->vk.enabled_features.robustBufferAccess &&
      (usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT ||
   usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
                  vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *requirements = (void *)ext;
   requirements->prefersDedicatedAllocation = false;
   requirements->requiresDedicatedAllocation = false;
               default:
      anv_debug_ignored_stype(ext->sType);
            }
      void anv_GetBufferMemoryRequirements2(
      VkDevice                                    _device,
   const VkBufferMemoryRequirementsInfo2*      pInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            anv_get_buffer_memory_requirements(device,
                  }
      void anv_GetDeviceBufferMemoryRequirementsKHR(
      VkDevice                                    _device,
   const VkDeviceBufferMemoryRequirements*     pInfo,
      {
               anv_get_buffer_memory_requirements(device,
                  }
      VkResult anv_CreateBuffer(
      VkDevice                                    _device,
   const VkBufferCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            /* Don't allow creating buffers bigger than our address space.  The real
   * issue here is that we may align up the buffer size and we don't want
   * doing so to cause roll-over.  However, no one has any business
   * allocating a buffer larger than our GTT size.
   */
   if (pCreateInfo->size > device->physical->gtt_size)
            buffer = vk_buffer_create(&device->vk, pCreateInfo,
         if (buffer == NULL)
                                 }
      void anv_DestroyBuffer(
      VkDevice                                    _device,
   VkBuffer                                    _buffer,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!buffer)
               }
      VkDeviceAddress anv_GetBufferDeviceAddress(
      VkDevice                                    device,
      {
               assert(!anv_address_is_null(buffer->address));
               }
      uint64_t anv_GetBufferOpaqueCaptureAddress(
      VkDevice                                    device,
      {
         }
      uint64_t anv_GetDeviceMemoryOpaqueCaptureAddress(
      VkDevice                                    device,
      {
               assert(anv_bo_is_pinned(memory->bo));
               }
      void
   anv_fill_buffer_surface_state(struct anv_device *device, struct anv_state state,
                                 {
      isl_buffer_fill_state(&device->isl_dev, state.map,
                        .address = anv_address_physical(address),
   .mocs = isl_mocs(&device->isl_dev, usage,
         }
      void anv_DestroySampler(
      VkDevice                                    _device,
   VkSampler                                   _sampler,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!sampler)
            if (sampler->bindless_state.map) {
      anv_state_pool_free(&device->dynamic_state_pool,
               if (sampler->custom_border_color.map) {
      anv_state_reserved_pool_free(&device->custom_border_colors,
                  }
      static const VkTimeDomainEXT anv_time_domains[] = {
      VK_TIME_DOMAIN_DEVICE_EXT,
      #ifdef CLOCK_MONOTONIC_RAW
         #endif
   };
      VkResult anv_GetPhysicalDeviceCalibrateableTimeDomainsEXT(
      VkPhysicalDevice                             physicalDevice,
   uint32_t                                     *pTimeDomainCount,
      {
      int d;
            for (d = 0; d < ARRAY_SIZE(anv_time_domains); d++) {
      vk_outarray_append_typed(VkTimeDomainEXT, &out, i) {
                        }
      static uint64_t
   anv_clock_gettime(clockid_t clock_id)
   {
      struct timespec current;
               #ifdef CLOCK_MONOTONIC_RAW
      if (ret < 0 && clock_id == CLOCK_MONOTONIC_RAW)
      #endif
      if (ret < 0)
               }
      VkResult anv_GetCalibratedTimestampsEXT(
      VkDevice                                     _device,
   uint32_t                                     timestampCount,
   const VkCalibratedTimestampInfoEXT           *pTimestampInfos,
   uint64_t                                     *pTimestamps,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   uint64_t timestamp_frequency = device->info->timestamp_frequency;
   int d;
   uint64_t begin, end;
         #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
         for (d = 0; d < timestampCount; d++) {
      switch (pTimestampInfos[d].timeDomain) {
   case VK_TIME_DOMAIN_DEVICE_EXT:
      if (!intel_gem_read_render_timestamp(device->fd,
                  return vk_device_set_lost(&device->vk, "Failed to read the "
      }
   uint64_t device_period = DIV_ROUND_UP(1000000000, timestamp_frequency);
   max_clock_period = MAX2(max_clock_period, device_period);
      case VK_TIME_DOMAIN_CLOCK_MONOTONIC_EXT:
      pTimestamps[d] = anv_clock_gettime(CLOCK_MONOTONIC);
         #ifdef CLOCK_MONOTONIC_RAW
         case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
         #endif
         default:
      pTimestamps[d] = 0;
               #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
         /*
   * The maximum deviation is the sum of the interval over which we
   * perform the sampling and the maximum period of any sampled
   * clock. That's because the maximum skew between any two sampled
   * clock edges is when the sampled clock with the largest period is
   * sampled at the end of that period but right at the beginning of the
   * sampling interval and some other clock is sampled right at the
   * beginning of its sampling period and right at the end of the
   * sampling interval. Let's assume the GPU has the longest clock
   * period and that the application is sampling GPU and monotonic:
   *
   *                               s                 e
   *			 w x y z 0 1 2 3 4 5 6 7 8 9 a b c d e f
   *	Raw              -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
   *
   *                               g
   *		  0         1         2         3
   *	GPU       -----_____-----_____-----_____-----_____
   *
   *                                                m
   *					    x y z 0 1 2 3 4 5 6 7 8 9 a b c
   *	Monotonic                           -_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-
   *
   *	Interval                     <----------------->
   *	Deviation           <-------------------------->
   *
   *		s  = read(raw)       2
   *		g  = read(GPU)       1
   *		m  = read(monotonic) 2
   *		e  = read(raw)       b
   *
   * We round the sample interval up by one tick to cover sampling error
   * in the interval clock
                                 }
      void anv_GetPhysicalDeviceMultisamplePropertiesEXT(
      VkPhysicalDevice                            physicalDevice,
   VkSampleCountFlagBits                       samples,
      {
               assert(pMultisampleProperties->sType ==
            VkExtent2D grid_size;
   if (samples & isl_device_get_sample_counts(&physical_device->isl_dev)) {
      grid_size.width = 1;
      } else {
      grid_size.width = 0;
      }
            vk_foreach_struct(ext, pMultisampleProperties->pNext)
      }
      /* vk_icd.h does not declare this function, so we declare it here to
   * suppress Wmissing-prototypes.
   */
   PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion);
      PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t* pSupportedVersion)
   {
      /* For the full details on loader interface versioning, see
   * <https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/blob/master/loader/LoaderAndLayerInterface.md>.
   * What follows is a condensed summary, to help you navigate the large and
   * confusing official doc.
   *
   *   - Loader interface v0 is incompatible with later versions. We don't
   *     support it.
   *
   *   - In loader interface v1:
   *       - The first ICD entrypoint called by the loader is
   *         vk_icdGetInstanceProcAddr(). The ICD must statically expose this
   *         entrypoint.
   *       - The ICD must statically expose no other Vulkan symbol unless it is
   *         linked with -Bsymbolic.
   *       - Each dispatchable Vulkan handle created by the ICD must be
   *         a pointer to a struct whose first member is VK_LOADER_DATA. The
   *         ICD must initialize VK_LOADER_DATA.loadMagic to ICD_LOADER_MAGIC.
   *       - The loader implements vkCreate{PLATFORM}SurfaceKHR() and
   *         vkDestroySurfaceKHR(). The ICD must be capable of working with
   *         such loader-managed surfaces.
   *
   *    - Loader interface v2 differs from v1 in:
   *       - The first ICD entrypoint called by the loader is
   *         vk_icdNegotiateLoaderICDInterfaceVersion(). The ICD must
   *         statically expose this entrypoint.
   *
   *    - Loader interface v3 differs from v2 in:
   *        - The ICD must implement vkCreate{PLATFORM}SurfaceKHR(),
   *          vkDestroySurfaceKHR(), and other API which uses VKSurfaceKHR,
   *          because the loader no longer does so.
   *
   *    - Loader interface v4 differs from v3 in:
   *        - The ICD must implement vk_icdGetPhysicalDeviceProcAddr().
   *
   *    - Loader interface v5 differs from v4 in:
   *        - The ICD must support Vulkan API version 1.1 and must not return
   *          VK_ERROR_INCOMPATIBLE_DRIVER from vkCreateInstance() unless a
   *          Vulkan Loader with interface v4 or smaller is being used and the
   *          application provides an API version that is greater than 1.0.
   */
   *pSupportedVersion = MIN2(*pSupportedVersion, 5u);
      }
