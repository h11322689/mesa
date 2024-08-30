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
   #ifdef ANDROID
   #include "util/u_gralloc/u_gralloc.h"
   #endif
   #include "util/u_string.h"
   #include "util/driconf.h"
   #include "git_sha1.h"
   #include "vk_common_entrypoints.h"
   #include "vk_util.h"
   #include "vk_deferred_operation.h"
   #include "vk_drm_syncobj.h"
   #include "common/intel_aux_map.h"
   #include "common/intel_uuid.h"
   #include "common/i915/intel_gem.h"
   #include "perf/intel_perf.h"
      #include "i915/anv_device.h"
   #include "xe/anv_device.h"
   #include "xe/anv_queue.h"
      #include "genxml/gen7_pack.h"
   #include "genxml/genX_bits.h"
      static const driOptionDescription anv_dri_options[] = {
      DRI_CONF_SECTION_PERFORMANCE
      DRI_CONF_ADAPTIVE_SYNC(true)
   DRI_CONF_VK_X11_OVERRIDE_MIN_IMAGE_COUNT(0)
   DRI_CONF_VK_X11_STRICT_IMAGE_COUNT(false)
   DRI_CONF_VK_KHR_PRESENT_WAIT(false)
   DRI_CONF_VK_XWAYLAND_WAIT_READY(true)
   DRI_CONF_ANV_ASSUME_FULL_SUBGROUPS(0)
   DRI_CONF_ANV_DISABLE_FCV(false)
   DRI_CONF_ANV_SAMPLE_MASK_OUT_OPENGL_BEHAVIOUR(false)
   DRI_CONF_ANV_FORCE_FILTER_ADDR_ROUNDING(false)
   DRI_CONF_ANV_FP64_WORKAROUND_ENABLED(false)
   DRI_CONF_ANV_GENERATED_INDIRECT_THRESHOLD(4)
   DRI_CONF_ANV_GENERATED_INDIRECT_RING_THRESHOLD(100)
   DRI_CONF_NO_16BIT(false)
   DRI_CONF_INTEL_ENABLE_WA_14018912822(false)
   DRI_CONF_ANV_QUERY_CLEAR_WITH_BLORP_THRESHOLD(6)
   DRI_CONF_ANV_QUERY_COPY_WITH_SHADER_THRESHOLD(6)
   DRI_CONF_ANV_FORCE_INDIRECT_DESCRIPTORS(false)
               DRI_CONF_SECTION_DEBUG
      DRI_CONF_ALWAYS_FLUSH_CACHE(false)
   DRI_CONF_VK_WSI_FORCE_BGRA8_UNORM_FIRST(false)
   DRI_CONF_VK_WSI_FORCE_SWAPCHAIN_TO_CURRENT_EXTENT(false)
   DRI_CONF_LIMIT_TRIG_INPUT_RANGE(false)
   DRI_CONF_ANV_MESH_CONV_PRIM_ATTRS_TO_VERT_ATTRS(-2)
   DRI_CONF_FORCE_VK_VENDOR(0)
   #if defined(ANDROID) && ANDROID_API_LEVEL >= 34
         #else
         #endif
               DRI_CONF_SECTION_QUALITY
            };
      /* This is probably far to big but it reflects the max size used for messages
   * in OpenGLs KHR_debug.
   */
   #define MAX_DEBUG_MESSAGE_LENGTH    4096
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
   #if ANDROID_API_LEVEL >= 33
   #define ANV_API_VERSION VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
   #else
   #define ANV_API_VERSION VK_MAKE_VERSION(1, 1, VK_HEADER_VERSION)
   #endif
   #else
   #define ANV_API_VERSION VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
   #endif
      VkResult anv_EnumerateInstanceVersion(
         {
      *pApiVersion = ANV_API_VERSION;
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
   .KHR_surface_protected_capabilities       = true,
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
      .KHR_8bit_storage                      = true,
   .KHR_16bit_storage                     = !device->instance->no_16bit,
   .KHR_acceleration_structure            = rt_enabled,
   .KHR_bind_memory2                      = true,
   .KHR_buffer_device_address             = true,
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
   .KHR_fragment_shading_rate             = device->info.ver >= 11,
   .KHR_get_memory_requirements2          = true,
   .KHR_global_priority                   = device->max_context_priority >=
         .KHR_image_format_list                 = true,
   #ifdef ANV_USE_WSI_PLATFORM
         #endif
         .KHR_maintenance1                      = true,
   .KHR_maintenance2                      = true,
   .KHR_maintenance3                      = true,
   .KHR_maintenance4                      = true,
   .KHR_maintenance5                      = true,
   .KHR_map_memory2                       = true,
   .KHR_multiview                         = true,
   .KHR_performance_query =
      device->perf &&
   (device->perf->i915_perf_version >= 3 ||
   INTEL_DEBUG(DEBUG_NO_OACONFIG)) &&
      .KHR_pipeline_executable_properties    = true,
   .KHR_pipeline_library                  = true,
   /* Hide these behind dri configs for now since we cannot implement it reliably on
   * all surfaces yet. There is no surface capability query for present wait/id,
   * but the feature is useful enough to hide behind an opt-in mechanism for now.
   * If the instance only enables surface extensions that unconditionally support present wait,
   * we can also expose the extension that way. */
   .KHR_present_id =
      driQueryOptionb(&device->instance->dri_options, "vk_khr_present_wait") ||
      .KHR_present_wait =
      driQueryOptionb(&device->instance->dri_options, "vk_khr_present_wait") ||
      .KHR_push_descriptor                   = true,
   .KHR_ray_query                         = rt_enabled,
   .KHR_ray_tracing_maintenance1          = rt_enabled,
   .KHR_ray_tracing_pipeline              = rt_enabled,
   .KHR_ray_tracing_position_fetch        = rt_enabled,
   .KHR_relaxed_block_layout              = true,
   .KHR_sampler_mirror_clamp_to_edge      = true,
   .KHR_sampler_ycbcr_conversion          = true,
   .KHR_separate_depth_stencil_layouts    = true,
   .KHR_shader_atomic_int64               = true,
   .KHR_shader_clock                      = true,
   .KHR_shader_draw_parameters            = true,
   .KHR_shader_float16_int8               = !device->instance->no_16bit,
   .KHR_shader_float_controls             = true,
   .KHR_shader_integer_dot_product        = true,
   .KHR_shader_non_semantic_info          = true,
   .KHR_shader_subgroup_extended_types    = true,
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
   .KHR_video_queue                       = device->video_decode_enabled,
   .KHR_video_decode_queue                = device->video_decode_enabled,
   .KHR_video_decode_h264                 = VIDEO_CODEC_H264DEC && device->video_decode_enabled,
   .KHR_video_decode_h265                 = VIDEO_CODEC_H265DEC && device->video_decode_enabled,
   .KHR_vulkan_memory_model               = true,
   .KHR_workgroup_memory_explicit_layout  = true,
   .KHR_zero_initialize_workgroup_memory  = true,
   .EXT_4444_formats                      = true,
   .EXT_border_color_swizzle              = true,
   .EXT_buffer_device_address             = true,
   .EXT_calibrated_timestamps             = device->has_reg_timestamp,
   .EXT_color_write_enable                = true,
   .EXT_conditional_rendering             = true,
   .EXT_conservative_rasterization        = true,
   .EXT_custom_border_color               = true,
   .EXT_depth_bias_control                = true,
   .EXT_depth_clamp_zero_one              = true,
   .EXT_depth_clip_control                = true,
   .EXT_depth_clip_enable                 = true,
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
         #endif
         .EXT_dynamic_rendering_unused_attachments = true,
   .EXT_extended_dynamic_state            = true,
   .EXT_extended_dynamic_state2           = true,
   .EXT_extended_dynamic_state3           = true,
   .EXT_external_memory_dma_buf           = true,
   .EXT_external_memory_host              = true,
   .EXT_fragment_shader_interlock         = true,
   .EXT_global_priority                   = device->max_context_priority >=
         .EXT_global_priority_query             = device->max_context_priority >=
         .EXT_graphics_pipeline_library         = !debug_get_bool_option("ANV_NO_GPL", false),
   .EXT_host_query_reset                  = true,
   .EXT_image_2d_view_of_3d               = true,
   .EXT_image_robustness                  = true,
   .EXT_image_drm_format_modifier         = true,
   .EXT_image_sliced_view_of_3d           = true,
   .EXT_image_view_min_lod                = true,
   .EXT_index_type_uint8                  = true,
   .EXT_inline_uniform_block              = true,
   .EXT_line_rasterization                = true,
   .EXT_load_store_op_none                = true,
   /* Enable the extension only if we have support on both the local &
   * system memory
   */
   .EXT_memory_budget                     = (!device->info.has_local_mem ||
               .EXT_mesh_shader                       = device->info.has_mesh_shading,
   .EXT_mutable_descriptor_type           = true,
   .EXT_nested_command_buffer             = true,
   .EXT_non_seamless_cube_map             = true,
   .EXT_pci_bus_info                      = true,
   .EXT_physical_device_drm               = true,
   .EXT_pipeline_creation_cache_control   = true,
   .EXT_pipeline_creation_feedback        = true,
   .EXT_pipeline_library_group_handles    = rt_enabled,
   .EXT_pipeline_robustness               = true,
   .EXT_post_depth_coverage               = true,
   .EXT_primitives_generated_query        = true,
   .EXT_primitive_topology_list_restart   = true,
   .EXT_private_data                      = true,
   .EXT_provoking_vertex                  = true,
   .EXT_queue_family_foreign              = true,
   .EXT_robustness2                       = true,
   .EXT_sample_locations                  = true,
   .EXT_sampler_filter_minmax             = true,
   .EXT_scalar_block_layout               = true,
   .EXT_separate_stencil_usage            = true,
   .EXT_shader_atomic_float               = true,
   .EXT_shader_atomic_float2              = true,
   .EXT_shader_demote_to_helper_invocation = true,
   .EXT_shader_module_identifier          = true,
   .EXT_shader_stencil_export             = true,
   .EXT_shader_subgroup_ballot            = true,
   .EXT_shader_subgroup_vote              = true,
   .EXT_shader_viewport_index_layer       = true,
   .EXT_subgroup_size_control             = true,
   .EXT_texel_buffer_alignment            = true,
   .EXT_tooling_info                      = true,
   .EXT_transform_feedback                = true,
   .EXT_vertex_attribute_divisor          = true,
   .EXT_vertex_input_dynamic_state        = true,
   #ifdef ANDROID
         .ANDROID_external_memory_android_hardware_buffer = true,
   #endif
         .GOOGLE_decorate_string                = true,
   .GOOGLE_hlsl_functionality1            = true,
   .GOOGLE_user_type                      = true,
   .INTEL_performance_query               = device->perf &&
         .INTEL_shader_integer_functions2       = true,
   .EXT_multi_draw                        = true,
   .NV_compute_shader_derivatives         = true,
         }
      static void
   get_features(const struct anv_physical_device *pdevice,
         {
                        const bool mesh_shader =
            const bool has_sparse_or_fake = pdevice->instance->has_fake_sparse ||
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
   .textureCompressionETC2                   = true,
   .textureCompressionASTC_LDR               = pdevice->has_astc_ldr ||
         .textureCompressionBC                     = true,
   .occlusionQueryPrecise                    = true,
   .pipelineStatisticsQuery                  = true,
   /* We can't do image stores in vec4 shaders */
   .vertexPipelineStoresAndAtomics =
      pdevice->compiler->scalar_stage[MESA_SHADER_VERTEX] &&
      .fragmentStoresAndAtomics                 = true,
   .shaderTessellationAndGeometryPointSize   = true,
   .shaderImageGatherExtended                = true,
   .shaderStorageImageExtendedFormats        = true,
   .shaderStorageImageMultisample            = false,
   /* Gfx12.5 has all the required format supported in HW for typed
   * read/writes
   */
   .shaderStorageImageReadWithoutFormat      = pdevice->info.verx10 >= 125,
   .shaderStorageImageWriteWithoutFormat     = true,
   .shaderUniformBufferArrayDynamicIndexing  = true,
   .shaderSampledImageArrayDynamicIndexing   = true,
   .shaderStorageBufferArrayDynamicIndexing  = true,
   .shaderStorageImageArrayDynamicIndexing   = true,
   .shaderClipDistance                       = true,
   .shaderCullDistance                       = true,
   .shaderFloat64                            = pdevice->info.has_64bit_float,
   .shaderInt64                              = true,
   .shaderInt16                              = true,
   .shaderResourceMinLod                     = true,
   .shaderResourceResidency                  = has_sparse_or_fake,
   .sparseBinding                            = has_sparse_or_fake,
   .sparseResidencyAliased                   = has_sparse_or_fake,
   .sparseResidencyBuffer                    = has_sparse_or_fake,
   .sparseResidencyImage2D                   = has_sparse_or_fake,
   .sparseResidencyImage3D                   = has_sparse_or_fake,
   .sparseResidency2Samples                  = false,
   .sparseResidency4Samples                  = false,
   .sparseResidency8Samples                  = false,
   .sparseResidency16Samples                 = false,
   .variableMultisampleRate                  = true,
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess            = !pdevice->instance->no_16bit,
   .uniformAndStorageBuffer16BitAccess  = !pdevice->instance->no_16bit,
   .storagePushConstant16               = true,
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
   .storageBuffer8BitAccess             = true,
   .uniformAndStorageBuffer8BitAccess   = true,
   .storagePushConstant8                = true,
   .shaderBufferInt64Atomics            = true,
   .shaderSharedInt64Atomics            = false,
   .shaderFloat16                       = !pdevice->instance->no_16bit,
            .descriptorIndexing                                 = true,
   .shaderInputAttachmentArrayDynamicIndexing          = false,
   .shaderUniformTexelBufferArrayDynamicIndexing       = true,
   .shaderStorageTexelBufferArrayDynamicIndexing       = true,
   .shaderUniformBufferArrayNonUniformIndexing         = true,
   .shaderSampledImageArrayNonUniformIndexing          = true,
   .shaderStorageBufferArrayNonUniformIndexing         = true,
   .shaderStorageImageArrayNonUniformIndexing          = true,
   .shaderInputAttachmentArrayNonUniformIndexing       = false,
   .shaderUniformTexelBufferArrayNonUniformIndexing    = true,
   .shaderStorageTexelBufferArrayNonUniformIndexing    = true,
   .descriptorBindingUniformBufferUpdateAfterBind      = true,
   .descriptorBindingSampledImageUpdateAfterBind       = true,
   .descriptorBindingStorageImageUpdateAfterBind       = true,
   .descriptorBindingStorageBufferUpdateAfterBind      = true,
   .descriptorBindingUniformTexelBufferUpdateAfterBind = true,
   .descriptorBindingStorageTexelBufferUpdateAfterBind = true,
   .descriptorBindingUpdateUnusedWhilePending          = true,
   .descriptorBindingPartiallyBound                    = true,
   .descriptorBindingVariableDescriptorCount           = true,
            .samplerFilterMinmax                 = true,
   .scalarBlockLayout                   = true,
   .imagelessFramebuffer                = true,
   .uniformBufferStandardLayout         = true,
   .shaderSubgroupExtendedTypes         = true,
   .separateDepthStencilLayouts         = true,
   .hostQueryReset                      = true,
   .timelineSemaphore                   = true,
   .bufferDeviceAddress                 = true,
   .bufferDeviceAddressCaptureReplay    = true,
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
            /* VK_KHR_acceleration_structure */
   .accelerationStructure = rt_enabled,
   .accelerationStructureCaptureReplay = false, /* TODO */
   .accelerationStructureIndirectBuild = false, /* TODO */
   .accelerationStructureHostCommands = false,
            /* VK_EXT_border_color_swizzle */
   .borderColorSwizzle = true,
            /* VK_EXT_color_write_enable */
            /* VK_EXT_image_2d_view_of_3d  */
   .image2DViewOf3D = true,
            /* VK_EXT_image_sliced_view_of_3d */
            /* VK_NV_compute_shader_derivatives */
   .computeDerivativeGroupQuads = true,
            /* VK_EXT_conditional_rendering */
   .conditionalRendering = true,
            /* VK_EXT_custom_border_color */
   .customBorderColors = true,
            /* VK_EXT_depth_clamp_zero_one */
            /* VK_EXT_depth_clip_enable */
            /* VK_EXT_fragment_shader_interlock */
   .fragmentShaderSampleInterlock = true,
   .fragmentShaderPixelInterlock = true,
            /* VK_EXT_global_priority_query */
            /* VK_EXT_graphics_pipeline_library */
   .graphicsPipelineLibrary =
            /* VK_KHR_fragment_shading_rate */
   .pipelineFragmentShadingRate = true,
   .primitiveFragmentShadingRate =
         .attachmentFragmentShadingRate =
            /* VK_EXT_image_view_min_lod */
            /* VK_EXT_index_type_uint8 */
            /* VK_EXT_line_rasterization */
   /* Rectangular lines must use the strict algorithm, which is not
   * supported for wide lines prior to ICL.  See rasterization_mode for
   * details and how the HW states are programmed.
   */
   .rectangularLines = pdevice->info.ver >= 10,
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
            /* VK_NV_mesh_shader */
   .taskShaderNV = false,
            /* VK_EXT_mesh_shader */
   .taskShader = mesh_shader,
   .meshShader = mesh_shader,
   .multiviewMeshShader = false,
   .primitiveFragmentShadingRateMeshShader = mesh_shader,
            /* VK_EXT_mutable_descriptor_type */
            /* VK_KHR_performance_query */
   .performanceCounterQueryPools = true,
   /* HW only supports a single configuration at a time. */
            /* VK_KHR_pipeline_executable_properties */
            /* VK_EXT_primitives_generated_query */
   .primitivesGeneratedQuery = true,
   .primitivesGeneratedQueryWithRasterizerDiscard = false,
            /* VK_EXT_pipeline_library_group_handles */
            /* VK_EXT_provoking_vertex */
   .provokingVertexLast = true,
            /* VK_KHR_ray_query */
            /* VK_KHR_ray_tracing_maintenance1 */
   .rayTracingMaintenance1 = rt_enabled,
            /* VK_KHR_ray_tracing_pipeline */
   .rayTracingPipeline = rt_enabled,
   .rayTracingPipelineShaderGroupHandleCaptureReplay = false,
   .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = false,
   .rayTracingPipelineTraceRaysIndirect = rt_enabled,
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
            /* VK_EXT_shader_atomic_float2 */
   .shaderBufferFloat16Atomics      = pdevice->info.has_lsc,
   .shaderBufferFloat16AtomicAdd    = false,
   .shaderBufferFloat16AtomicMinMax = pdevice->info.has_lsc,
   .shaderBufferFloat32AtomicMinMax = true,
   .shaderBufferFloat64AtomicMinMax =
         .shaderSharedFloat16Atomics      = pdevice->info.has_lsc,
   .shaderSharedFloat16AtomicAdd    = false,
   .shaderSharedFloat16AtomicMinMax = pdevice->info.has_lsc,
   .shaderSharedFloat32AtomicMinMax = true,
   .shaderSharedFloat64AtomicMinMax = false,
   .shaderImageFloat32AtomicMinMax  = false,
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
            /* VK_EXT_extended_dynamic_state3 */
   .extendedDynamicState3PolygonMode = true,
   .extendedDynamicState3TessellationDomainOrigin = true,
   .extendedDynamicState3RasterizationStream = true,
   .extendedDynamicState3LineStippleEnable = true,
   .extendedDynamicState3LineRasterizationMode = true,
   .extendedDynamicState3LogicOpEnable = true,
   .extendedDynamicState3AlphaToOneEnable = true,
   .extendedDynamicState3DepthClipEnable = true,
   .extendedDynamicState3DepthClampEnable = true,
   .extendedDynamicState3DepthClipNegativeOneToOne = true,
   .extendedDynamicState3ProvokingVertexMode = true,
   .extendedDynamicState3ColorBlendEnable = true,
   .extendedDynamicState3ColorWriteMask = true,
   .extendedDynamicState3ColorBlendEquation = true,
   .extendedDynamicState3SampleLocationsEnable = true,
   .extendedDynamicState3SampleMask = true,
            .extendedDynamicState3RasterizationSamples = false,
   .extendedDynamicState3AlphaToCoverageEnable = false,
   .extendedDynamicState3ExtraPrimitiveOverestimationSize = false,
   .extendedDynamicState3ViewportWScalingEnable = false,
   .extendedDynamicState3ViewportSwizzle = false,
   .extendedDynamicState3ShadingRateImageEnable = false,
   .extendedDynamicState3CoverageToColorEnable = false,
   .extendedDynamicState3CoverageToColorLocation = false,
   .extendedDynamicState3CoverageModulationMode = false,
   .extendedDynamicState3CoverageModulationTableEnable = false,
   .extendedDynamicState3CoverageModulationTable = false,
   .extendedDynamicState3CoverageReductionMode = false,
   .extendedDynamicState3RepresentativeFragmentTestEnable = false,
            /* VK_EXT_multi_draw */
            /* VK_EXT_non_seamless_cube_map */
            /* VK_EXT_primitive_topology_list_restart */
   .primitiveTopologyListRestart = true,
            /* VK_EXT_depth_clip_control */
            /* VK_KHR_present_id */
            /* VK_KHR_present_wait */
            /* VK_EXT_vertex_input_dynamic_state */
            /* VK_KHR_ray_tracing_position_fetch */
            /* VK_EXT_dynamic_rendering_unused_attachments */
            /* VK_EXT_depth_bias_control */
   .depthBiasControl = true,
   .floatRepresentation = true,
   .leastRepresentableValueForceUnormRepresentation = false,
            /* VK_EXT_pipeline_robustness */
            /* VK_KHR_maintenance5 */
            /* VK_EXT_nested_command_buffer */
   .nestedCommandBuffer = true,
   .nestedCommandBufferRendering = true,
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
               }
      static VkResult MUST_CHECK
   anv_init_meminfo(struct anv_physical_device *device, int fd)
   {
               device->sys.region = &devinfo->mem.sram.mem;
   device->sys.size =
                  device->vram_mappable.region = &devinfo->mem.vram.mem;
   device->vram_mappable.size = devinfo->mem.vram.mappable.size;
            device->vram_non_mappable.region = &devinfo->mem.vram.mem;
   device->vram_non_mappable.size = devinfo->mem.vram.unmappable.size;
               }
      static void
   anv_update_meminfo(struct anv_physical_device *device, int fd)
   {
      if (!intel_device_info_update_memory_info(&device->info, fd))
            const struct intel_device_info *devinfo = &device->info;
   device->sys.available = devinfo->mem.sram.mappable.free;
   device->vram_mappable.available = devinfo->mem.vram.mappable.free;
      }
      static VkResult
   anv_physical_device_init_heaps(struct anv_physical_device *device, int fd)
   {
      VkResult result = anv_init_meminfo(device, fd);
   if (result != VK_SUCCESS)
                     if (anv_physical_device_has_vram(device)) {
      /* We can create 2 or 3 different heaps when we have local memory
   * support, first heap with local memory size and second with system
   * memory size and the third is added only if part of the vram is
   * mappable to the host.
   */
   device->memory.heap_count = 2;
   device->memory.heaps[0] = (struct anv_memory_heap) {
      /* If there is a vram_non_mappable, use that for the device only
   * heap. Otherwise use the vram_mappable.
   */
   .size = device->vram_non_mappable.size != 0 ?
         .flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
      };
   device->memory.heaps[1] = (struct anv_memory_heap) {
      .size = device->sys.size,
   .flags = 0,
      };
   /* Add an additional smaller vram mappable heap if we can't map all the
   * vram to the host.
   */
   if (device->vram_non_mappable.size > 0) {
      device->memory.heap_count++;
   device->memory.heaps[2] = (struct anv_memory_heap) {
      .size = device->vram_mappable.size,
   .flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
            } else {
      device->memory.heap_count = 1;
   device->memory.heaps[0] = (struct anv_memory_heap) {
      .size = device->sys.size,
   .flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
                  switch (device->info.kmd_type) {
   case INTEL_KMD_TYPE_XE:
      result = anv_xe_physical_device_init_memory_types(device);
      case INTEL_KMD_TYPE_I915:
   default:
      result = anv_i915_physical_device_init_memory_types(device);
               if (result != VK_SUCCESS)
            for (unsigned i = 0; i < device->memory.type_count; i++) {
      VkMemoryPropertyFlags props = device->memory.types[i].propertyFlags;
   if ((props & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
   #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
         #else
               #endif
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
   *  * "v" is for video queues with no graphics support
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
   anv_override_engine_counts(int *gc_count, int *g_count, int *c_count, int *v_count)
   {
      int gc_override = -1;
   int g_override = -1;
   int c_override = -1;
   int v_override = -1;
            if (env == NULL)
            env = strdup(env);
   char *save = NULL;
   char *next = strtok_r(env, ",", &save);
   while (next != NULL) {
      if (strncmp(next, "gc=", 3) == 0) {
         } else if (strncmp(next, "g=", 2) == 0) {
         } else if (strncmp(next, "c=", 2) == 0) {
         } else if (strncmp(next, "v=", 2) == 0) {
         } else {
         }
      }
   free(env);
   if (gc_override >= 0)
         if (g_override >= 0)
         if (*g_count > 0 && *gc_count <= 0 && (gc_override >= 0 || g_override >= 0))
      mesa_logw("ANV_QUEUE_OVERRIDE: gc=0 with g > 0 violates the "
      if (c_override >= 0)
         if (v_override >= 0)
      }
      static void
   anv_physical_device_init_queue_families(struct anv_physical_device *pdevice)
   {
      uint32_t family_count = 0;
   VkQueueFlags sparse_flags = (pdevice->instance->has_fake_sparse ||
                  if (pdevice->engine_info) {
      int gc_count =
      intel_engines_count(pdevice->engine_info,
      int v_count =
         int g_count = 0;
   int c_count = 0;
   if (debug_get_bool_option("INTEL_COMPUTE_CLASS", false))
      c_count = intel_engines_count(pdevice->engine_info,
      enum intel_engine_class compute_class =
            int blit_count = 0;
   if (debug_get_bool_option("INTEL_COPY_CLASS", false) &&
      pdevice->info.verx10 >= 125) {
   blit_count = intel_engines_count(pdevice->engine_info,
                        if (gc_count > 0) {
      pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               VK_QUEUE_COMPUTE_BIT |
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
   if (v_count > 0 && pdevice->video_decode_enabled) {
      /* HEVC support on Gfx9 is only available on VCS0. So limit the number of video queues
   * to the first VCS engine instance.
   *
   * We should be able to query HEVC support from the kernel using the engine query uAPI,
   * but this appears to be broken :
   *    https://gitlab.freedesktop.org/drm/intel/-/issues/8832
   *
   * When this bug is fixed we should be able to check HEVC support to determine the
   * correct number of queues.
   */
   pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_VIDEO_DECODE_BIT_KHR,
   .queueCount = pdevice->info.ver == 9 ? MIN2(1, v_count) : v_count,
         }
   if (blit_count > 0) {
      pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_TRANSFER_BIT,
   .queueCount = blit_count,
                  /* Increase count below when other families are added as a reminder to
   * increase the ANV_MAX_QUEUE_FAMILIES value.
   */
      } else {
      /* Default to a single render queue */
   pdevice->queue.families[family_count++] = (struct anv_queue_family) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               VK_QUEUE_COMPUTE_BIT |
   .queueCount = 1,
      };
      }
   assert(family_count <= ANV_MAX_QUEUE_FAMILIES);
      }
      static VkResult
   anv_physical_device_get_parameters(struct anv_physical_device *device)
   {
      switch (device->info.kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("Missing");
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
               if (devinfo.ver == 20) {
         } else if (devinfo.ver > 12) {
      result = vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
            } else if (devinfo.ver < 9) {
      /* Silently fail here, hasvk should pick up this device. */
   result = VK_ERROR_INCOMPATIBLE_DRIVER;
               if (!devinfo.has_context_isolation) {
      result = vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
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
                     device->local_fd = fd;
   result = anv_physical_device_get_parameters(device);
   if (result != VK_SUCCESS)
            device->gtt_size = device->info.gtt_size ? device->info.gtt_size :
            if (device->gtt_size < (4ULL << 30 /* GiB */)) {
      vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                     /* We currently only have the right bits for instructions in Gen12+. If the
   * kernel ever starts supporting that feature on previous generations,
   * we'll need to edit genxml prior to enabling here.
   */
   device->has_protected_contexts = device->info.ver >= 12 &&
            /* Just pick one; they're all the same */
   device->has_astc_ldr =
      isl_format_supports_sampling(&device->info,
      if (!device->has_astc_ldr &&
      driQueryOptionb(&device->instance->dri_options, "vk_require_astc"))
      if (devinfo.ver == 9 && !intel_device_info_is_9lp(&devinfo)) {
      device->flush_astc_ldr_void_extent_denorms =
      }
   device->disable_fcv = intel_device_info_is_mtl(&device->info) ||
            result = anv_physical_device_init_heaps(device, fd);
   if (result != VK_SUCCESS)
            if (debug_get_bool_option("ANV_QUEUE_THREAD_DISABLE", false))
               device->generated_indirect_draws =
      debug_get_bool_option("ANV_ENABLE_GENERATED_INDIRECT_DRAWS",
                  device->sync_syncobj_type = vk_drm_syncobj_get_type(fd);
   if (!device->has_exec_timeline)
                  /* anv_bo_sync_type is only supported with i915 for now  */
   if (device->info.kmd_type == INTEL_KMD_TYPE_I915) {
      if (!(device->sync_syncobj_type.features & VK_SYNC_FEATURE_CPU_WAIT))
            if (!(device->sync_syncobj_type.features & VK_SYNC_FEATURE_TIMELINE)) {
      device->sync_timeline_type = vk_sync_timeline_get_type(&anv_bo_sync_type);
         } else {
      assert(device->sync_syncobj_type.features & VK_SYNC_FEATURE_TIMELINE);
               device->sync_types[st_idx++] = NULL;
   assert(st_idx <= ARRAY_SIZE(device->sync_types));
                     device->always_use_bindless =
            device->use_call_secondary =
                              /* For now always use indirect descriptors. We'll update this
   * to !uses_ex_bso when all the infrastructure is built up.
   */
   device->indirect_descriptors =
      !device->uses_ex_bso ||
         /* Check if we can read the GPU timestamp register from the CPU */
   uint64_t u64_ignore;
   device->has_reg_timestamp = intel_gem_read_render_timestamp(fd,
                           device->has_sparse = device->info.kmd_type == INTEL_KMD_TYPE_XE &&
            device->always_flush_cache = INTEL_DEBUG(DEBUG_STALL) ||
            device->compiler = brw_compiler_create(NULL, &device->info);
   if (device->compiler == NULL) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
      }
   device->compiler->shader_debug_log = compiler_debug_log;
   device->compiler->shader_perf_log = compiler_perf_log;
   device->compiler->constant_buffer_0_is_relative = false;
   device->compiler->supports_shader_constants = true;
   device->compiler->indirect_ubos_use_sampler = device->info.ver < 12;
   device->compiler->extended_bindless_surface_offset = device->uses_ex_bso;
   device->compiler->use_bindless_sampler_offset = !device->indirect_descriptors;
   device->compiler->spilling_rate =
            isl_device_init(&device->isl_dev, &device->info);
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
   device->info.has_compute_engine = device->engine_info &&
                                 get_device_extensions(device, &device->vk.supported_extensions);
            /* Gather major/minor before WSI. */
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
               result = anv_init_wsi(device);
   if (result != VK_SUCCESS)
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
         instance->force_filter_addr_rounding =
         instance->lower_depth_range_rate =
         instance->no_16bit =
         instance->intel_enable_wa_14018912822 =
         instance->mesh_conv_prim_attrs_to_vert_attrs =
         instance->fp64_workaround_enabled =
         instance->generated_indirect_threshold =
         instance->generated_indirect_ring_threshold =
         instance->query_clear_with_blorp_threshold =
         instance->query_copy_with_shader_threshold =
         instance->force_vk_vendor =
         instance->has_fake_sparse =
         instance->disable_fcv =
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
      static VkDeviceSize
   anx_get_physical_device_max_heap_size(struct anv_physical_device *pdevice)
   {
               for (uint32_t i = 0; i < pdevice->memory.heap_count; i++) {
      if (pdevice->memory.heaps[i].size > ret)
                  }
      void anv_GetPhysicalDeviceProperties(
      VkPhysicalDevice                            physicalDevice,
      {
      ANV_FROM_HANDLE(anv_physical_device, pdevice, physicalDevice);
            const uint32_t max_ssbos = UINT16_MAX;
   const uint32_t max_textures = UINT16_MAX;
   const uint32_t max_samplers = UINT16_MAX;
   const uint32_t max_images = UINT16_MAX;
            /* Claim a high per-stage limit since we have bindless. */
            const uint32_t max_workgroup_size =
            const bool has_sparse_or_fake = pdevice->instance->has_fake_sparse ||
            VkSampleCountFlags sample_counts =
               VkPhysicalDeviceLimits limits = {
      .maxImageDimension1D                      = (1 << 14),
   .maxImageDimension2D                      = (1 << 14),
   .maxImageDimension3D                      = (1 << 11),
   .maxImageDimensionCube                    = (1 << 14),
   .maxImageArrayLayers                      = (1 << 11),
   .maxTexelBufferElements                   = 128 * 1024 * 1024,
   .maxUniformBufferRange                    = pdevice->compiler->indirect_ubos_use_sampler ? (1u << 27) : (1u << 30),
   .maxStorageBufferRange                    = MIN3(pdevice->isl_dev.max_buffer_size, max_heap_size, UINT32_MAX),
   .maxPushConstantsSize                     = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount                 = UINT32_MAX,
   .maxSamplerAllocationCount                = 64 * 1024,
   .bufferImageGranularity                   = 1,
   .sparseAddressSpaceSize                   = has_sparse_or_fake ? (1uLL << 48) : 0,
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
   /* Skylake PRMs: Volume 2d: Command Reference: Structures:
   *
   * VERTEX_BUFFER_STATE::Buffer Pitch: [0,4095]
   */
   .maxVertexInputBindingStride              = 4095,
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
   .maxGeometryInputComponents               = 128,
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
   .sampledImageIntegerSampleCounts          = sample_counts,
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
   .lineWidthRange                           = { 0.0, 8.0 },
   .pointSizeGranularity                     = (1.0 / 8.0),
   .lineWidthGranularity                     = (1.0 / 128.0),
   .strictLines                              = false,
   .standardSampleLocations                  = true,
   .optimalBufferCopyOffsetAlignment         = 128,
   .optimalBufferCopyRowPitchAlignment       = 128,
               *pProperties = (VkPhysicalDeviceProperties) {
      .apiVersion = ANV_API_VERSION,
   .driverVersion = vk_get_driver_version(),
   .vendorID = 0x8086,
   .deviceID = pdevice->info.pci_device_id,
   .deviceType = pdevice->info.has_local_mem ?
               .limits = limits,
   .sparseProperties = {
      .residencyStandard2DBlockShape = has_sparse_or_fake,
   .residencyStandard2DMultisampleBlockShape = false,
   .residencyStandard3DBlockShape = has_sparse_or_fake,
   .residencyAlignedMipSize = false,
                  if (unlikely(pdevice->instance->force_vk_vendor))
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
   if (pdevice->vk.supported_extensions.KHR_ray_tracing_pipeline) {
      scalar_stages |= VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                  VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
   }
   if (pdevice->vk.supported_extensions.EXT_mesh_shader) {
      scalar_stages |= VK_SHADER_STAGE_TASK_BIT_EXT |
      }
   p->subgroupSupportedStages = scalar_stages;
   p->subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT |
                                    VK_SUBGROUP_FEATURE_VOTE_BIT |
               p->pointClippingBehavior      = VK_POINT_CLIPPING_BEHAVIOR_USER_CLIP_PLANES_ONLY;
   p->maxMultiviewViewCount      = 16;
   p->maxMultiviewInstanceIndex  = UINT32_MAX / 16;
   p->protectedNoFault           = false;
   /* This value doesn't matter for us today as our per-stage descriptors are
   * the real limit.
   */
            for (uint32_t i = 0; i < pdevice->memory.heap_count; i++) {
      p->maxMemoryAllocationSize = MAX2(p->maxMemoryAllocationSize,
         }
      static void
   anv_get_physical_device_properties_1_2(struct anv_physical_device *pdevice,
         {
               p->driverID = VK_DRIVER_ID_INTEL_OPEN_SOURCE_MESA;
   memset(p->driverName, 0, sizeof(p->driverName));
   snprintf(p->driverName, VK_MAX_DRIVER_NAME_SIZE,
         memset(p->driverInfo, 0, sizeof(p->driverInfo));
   snprintf(p->driverInfo, VK_MAX_DRIVER_INFO_SIZE,
            p->conformanceVersion = (VkConformanceVersion) {
      .major = 1,
   .minor = 3,
   .subminor = 6,
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
   p->shaderDenormPreserveFloat16            = true;
   p->shaderRoundingModeRTEFloat16           = true;
   p->shaderRoundingModeRTZFloat16           = true;
            p->shaderDenormFlushToZeroFloat32         = true;
   p->shaderDenormPreserveFloat32            = true;
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
   const unsigned max_bindless_views =
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
   p->supportedStencilResolveModes  = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT |
               p->independentResolveNone  = true;
            p->filterMinmaxSingleComponentFormats  = true;
                     p->framebufferIntegerColorSampleCounts =
      }
      static void
   anv_get_physical_device_properties_1_3(struct anv_physical_device *pdevice,
         {
               p->minSubgroupSize = 8;
   p->maxSubgroupSize = 32;
   p->maxComputeWorkgroupSubgroups = pdevice->info.max_cs_workgroup_threads;
   p->requiredSubgroupSizeStages = VK_SHADER_STAGE_COMPUTE_BIT |
                  p->maxInlineUniformBlockSize = MAX_INLINE_UNIFORM_BLOCK_SIZE;
   p->maxPerStageDescriptorInlineUniformBlocks =
         p->maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks =
         p->maxDescriptorSetInlineUniformBlocks =
         p->maxDescriptorSetUpdateAfterBindInlineUniformBlocks =
                  p->integerDotProduct8BitUnsignedAccelerated = false;
   p->integerDotProduct8BitSignedAccelerated = false;
   p->integerDotProduct8BitMixedSignednessAccelerated = false;
   p->integerDotProduct4x8BitPackedUnsignedAccelerated = pdevice->info.ver >= 12;
   p->integerDotProduct4x8BitPackedSignedAccelerated = pdevice->info.ver >= 12;
   p->integerDotProduct4x8BitPackedMixedSignednessAccelerated = pdevice->info.ver >= 12;
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
   p->integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated = pdevice->info.ver >= 12;
   p->integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated = pdevice->info.ver >= 12;
   p->integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated = pdevice->info.ver >= 12;
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
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_PROPERTIES_KHR: {
      VkPhysicalDeviceAccelerationStructurePropertiesKHR *props = (void *)ext;
   props->maxGeometryCount = (1u << 24) - 1;
   props->maxInstanceCount = (1u << 24) - 1;
   props->maxPrimitiveCount = (1u << 29) - 1;
   props->maxPerStageDescriptorAccelerationStructures = UINT16_MAX;
   props->maxPerStageDescriptorUpdateAfterBindAccelerationStructures = UINT16_MAX;
   props->maxDescriptorSetAccelerationStructures = UINT16_MAX;
   props->maxDescriptorSetUpdateAfterBindAccelerationStructures = UINT16_MAX;
   props->minAccelerationStructureScratchOffsetAlignment = 64;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT: {
      /* TODO: Real limits */
   VkPhysicalDeviceConservativeRasterizationPropertiesEXT *properties =
         /* There's nothing in the public docs about this value as far as I
   * can tell.  However, this is the value the Windows driver reports
   * and there's a comment on a rejected HW feature in the internal
   * docs that says:
   *
   *    "This is similar to conservative rasterization, except the
   *    primitive area is not extended by 1/512 and..."
   *
   * That's a bit of an obtuse reference but it's the best we've got
   * for now.
   */
   properties->primitiveOverestimationSize = 1.0f / 512.0f;
   properties->maxExtraPrimitiveOverestimationSize = 0.0f;
   properties->extraPrimitiveOverestimationSizeGranularity = 0.0f;
   properties->primitiveUnderestimation = false;
   properties->conservativePointAndLineRasterization = false;
   properties->degenerateTrianglesRasterized = true;
   properties->degenerateLinesRasterized = false;
   properties->fullyCoveredFragmentShaderInputVariable = false;
   properties->conservativeRasterizationPostDepthCoverage = true;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT: {
      VkPhysicalDeviceCustomBorderColorPropertiesEXT *properties =
         properties->maxCustomBorderColorSamplers = MAX_CUSTOM_BORDER_COLORS;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR: {
      VkPhysicalDeviceFragmentShadingRatePropertiesKHR *props =
         props->primitiveFragmentShadingRateWithMultipleViewports =
         props->layeredShadingRateAttachments = pdevice->info.has_coarse_pixel_primitive_and_cb;
   props->fragmentShadingRateNonTrivialCombinerOps =
         props->maxFragmentSize = (VkExtent2D) { 4, 4 };
   props->maxFragmentSizeAspectRatio =
      pdevice->info.has_coarse_pixel_primitive_and_cb ?
      props->maxFragmentShadingRateCoverageSamples = 4 * 4 *
         props->maxFragmentShadingRateRasterizationSamples =
      pdevice->info.has_coarse_pixel_primitive_and_cb ?
      props->fragmentShadingRateWithShaderDepthStencilWrites = false;
   props->fragmentShadingRateWithSampleMask = true;
   props->fragmentShadingRateWithShaderSampleMask = false;
   props->fragmentShadingRateWithConservativeRasterization = true;
                  /* Fix in DG2_G10_C0 and DG2_G11_B0. Consider any other Sku as having
   * the fix.
   */
   props->fragmentShadingRateStrictMultiplyCombiner =
      pdevice->info.platform == INTEL_PLATFORM_DG2_G10 ?
   pdevice->info.revision >= 8 :
               if (pdevice->info.has_coarse_pixel_primitive_and_cb) {
      props->minFragmentShadingRateAttachmentTexelSize = (VkExtent2D) { 8, 8 };
   props->maxFragmentShadingRateAttachmentTexelSize = (VkExtent2D) { 8, 8 };
      } else {
      /* Those must be 0 if attachmentFragmentShadingRate is not
   * supported.
   */
   props->minFragmentShadingRateAttachmentTexelSize = (VkExtent2D) { 0, 0 };
   props->maxFragmentShadingRateAttachmentTexelSize = (VkExtent2D) { 0, 0 };
      }
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT: {
                     props->hasPrimary = pdevice->has_master;
                  props->hasRender = pdevice->has_local;
                              case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_3_PROPERTIES_EXT: {
      VkPhysicalDeviceExtendedDynamicState3PropertiesEXT *props =
         props->dynamicPrimitiveTopologyUnrestricted = true;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT: {
      VkPhysicalDeviceExternalMemoryHostPropertiesEXT *props =
         /* Userptr needs page aligned memory. */
   props->minImportedHostPointerAlignment = 4096;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT: {
      VkPhysicalDeviceGraphicsPipelineLibraryPropertiesEXT *props =
         props->graphicsPipelineLibraryFastLinking = true;
   props->graphicsPipelineLibraryIndependentInterpolationDecoration = true;
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
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_PROPERTIES_KHR: {
      VkPhysicalDeviceMaintenance5PropertiesKHR *properties =
         properties->earlyFragmentMultisampleCoverageAfterSampleCounting = false;
   properties->earlyFragmentSampleMaskTestBeforeSampleCounting = false;
   properties->depthStencilSwizzleOneSupport = true;
   properties->polygonModePointSize = true;
   properties->nonStrictSinglePixelWideLinesUseParallelogram = false;
   properties->nonStrictWideLinesUseParallelogram = false;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_PROPERTIES_EXT: {
                     /* Bounded by the maximum representable size in
   * 3DSTATE_MESH_SHADER_BODY::SharedLocalMemorySize.  Same for Task.
                  /* Bounded by the maximum representable size in
   * 3DSTATE_MESH_SHADER_BODY::LocalXMaximum.  Same for Task.
                                                         properties->maxTaskWorkGroupTotalCount = max_threadgroup_count;
   properties->maxTaskWorkGroupCount[0] = max_threadgroup_xyz;
                  properties->maxTaskWorkGroupInvocations = max_workgroup_size;
   properties->maxTaskWorkGroupSize[0] = max_workgroup_size;
                                 properties->maxTaskPayloadSize = max_urb_size - task_payload_reserved;
   properties->maxTaskSharedMemorySize = max_slm_size;
   properties->maxTaskPayloadAndSharedMemorySize =
                  properties->maxMeshWorkGroupTotalCount = max_threadgroup_count;
   properties->maxMeshWorkGroupCount[0] = max_threadgroup_xyz;
                  properties->maxMeshWorkGroupInvocations = max_workgroup_size;
   properties->maxMeshWorkGroupSize[0] = max_workgroup_size;
                  properties->maxMeshSharedMemorySize = max_slm_size;
   properties->maxMeshPayloadAndSharedMemorySize =
                  /* Unfortunately spec's formula for the max output size doesn't match our hardware
   * (because some per-primitive and per-vertex attributes have alignment restrictions),
   * so we have to advertise the minimum value mandated by the spec to not overflow it.
   */
                  /* NumPrim + Primitive Data List */
   const uint32_t max_indices_memory =
                           properties->maxMeshPayloadAndOutputMemorySize =
                                                   /* Elements in Vertex Data Array must be aligned to 32 bytes (8 dwords). */
   properties->meshOutputPerVertexGranularity = 8;
                  /* SIMD16 */
                  properties->prefersLocalInvocationVertexOutput = false;
   properties->prefersLocalInvocationPrimitiveOutput = false;
                  /* Spec minimum values */
   assert(properties->maxTaskWorkGroupTotalCount >= (1U << 22));
   assert(properties->maxTaskWorkGroupCount[0] >= 65535);
                  assert(properties->maxTaskWorkGroupInvocations >= 128);
   assert(properties->maxTaskWorkGroupSize[0] >= 128);
                  assert(properties->maxTaskPayloadSize >= 16384);
                     assert(properties->maxMeshWorkGroupTotalCount >= (1U << 22));
   assert(properties->maxMeshWorkGroupCount[0] >= 65535);
                  assert(properties->maxMeshWorkGroupInvocations >= 128);
   assert(properties->maxMeshWorkGroupSize[0] >= 128);
                  assert(properties->maxMeshSharedMemorySize >= 28672);
   assert(properties->maxMeshPayloadAndSharedMemorySize >= 28672);
                           assert(properties->maxMeshOutputVertices >= 256);
   assert(properties->maxMeshOutputPrimitives >= 256);
                              case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT: {
      VkPhysicalDevicePCIBusInfoPropertiesEXT *properties =
         properties->pciDomain = pdevice->info.pci_domain;
   properties->pciBus = pdevice->info.pci_bus;
   properties->pciDevice = pdevice->info.pci_dev;
   properties->pciFunction = pdevice->info.pci_func;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_NESTED_COMMAND_BUFFER_PROPERTIES_EXT: {
      VkPhysicalDeviceNestedCommandBufferPropertiesEXT *properties =
         properties->maxCommandBufferNestingLevel = UINT32_MAX;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR: {
      VkPhysicalDevicePerformanceQueryPropertiesKHR *properties =
         /* We could support this by spawning a shader to do the equation
   * normalization.
   */
   properties->allowCommandBufferQueryCopies = false;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES_EXT: {
      VkPhysicalDevicePipelineRobustnessPropertiesEXT *properties =
         properties->defaultRobustnessStorageBuffers =
         properties->defaultRobustnessUniformBuffers =
         properties->defaultRobustnessVertexInputs =
         properties->defaultRobustnessImages =
               #ifdef ANDROID
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wswitch"
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID: {
      VkPhysicalDevicePresentationPropertiesANDROID *props =
         uint64_t front_rendering_usage = 0;
   struct u_gralloc *gralloc = u_gralloc_create(U_GRALLOC_TYPE_AUTO);
   if (gralloc != NULL) {
      u_gralloc_get_front_rendering_usage(gralloc, &front_rendering_usage);
      }
   props->sharedImage = front_rendering_usage ? VK_TRUE : VK_FALSE;
      #pragma GCC diagnostic pop
   #endif
            case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT: {
      VkPhysicalDeviceProvokingVertexPropertiesEXT *properties =
         properties->provokingVertexModePerPipeline = true;
   properties->transformFeedbackPreservesTriangleFanProvokingVertex = false;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR: {
      VkPhysicalDevicePushDescriptorPropertiesKHR *properties =
         properties->maxPushDescriptors = MAX_PUSH_DESCRIPTORS;
               case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR: {
      VkPhysicalDeviceRayTracingPipelinePropertiesKHR *props = (void *)ext;
   /* TODO */
   props->shaderGroupHandleSize = 32;
   props->maxRayRecursionDepth = 31;
   /* MemRay::hitGroupSRStride is 16 bits */
   props->maxShaderGroupStride = UINT16_MAX;
   /* MemRay::hitGroupSRBasePtr requires 16B alignment */
   props->shaderGroupBaseAlignment = 16;
   props->shaderGroupHandleAlignment = 16;
   props->shaderGroupHandleCaptureReplaySize = 32;
   props->maxRayDispatchInvocationCount = 1U << 30; /* required min limit */
   props->maxRayHitAttributeSize = BRW_RT_SIZEOF_HIT_ATTRIB_DATA;
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
   props->transformFeedbackDraw = true;
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
                              }
   properties->priorityCount = count;
      }
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_QUERY_RESULT_STATUS_PROPERTIES_KHR: {
      VkQueueFamilyQueryResultStatusPropertiesKHR *prop =
         prop->queryResultStatusSupport = VK_TRUE;
      }
   case VK_STRUCTURE_TYPE_QUEUE_FAMILY_VIDEO_PROPERTIES_KHR: {
      VkQueueFamilyVideoPropertiesKHR *prop =
         if (queue_family->queueFlags & VK_QUEUE_VIDEO_DECODE_BIT_KHR) {
      prop->videoCodecOperations = VK_VIDEO_CODEC_OPERATION_DECODE_H264_BIT_KHR |
      }
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
                     VkDeviceSize total_sys_heaps_size = 0, total_vram_heaps_size = 0;
   for (size_t i = 0; i < device->memory.heap_count; i++) {
      if (device->memory.heaps[i].is_local_mem) {
         } else {
                     for (size_t i = 0; i < device->memory.heap_count; i++) {
      VkDeviceSize heap_size = device->memory.heaps[i].size;
   VkDeviceSize heap_used = device->memory.heaps[i].used;
   VkDeviceSize heap_budget, total_heaps_size;
            if (device->memory.heaps[i].is_local_mem) {
      total_heaps_size = total_vram_heaps_size;
   if (device->vram_non_mappable.size > 0 && i == 0) {
         } else {
            } else {
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
         if (get_bo_from_pool(&ret_bo, &device->scratch_surface_state_pool.block_pool, address))
         if (device->physical->indirect_descriptors &&
      get_bo_from_pool(&ret_bo, &device->bindless_surface_state_pool.block_pool, address))
      if (get_bo_from_pool(&ret_bo, &device->internal_surface_state_pool.block_pool, address))
         if (get_bo_from_pool(&ret_bo, &device->push_descriptor_pool.block_pool, address))
            if (!device->cmd_buffer_being_decoded)
            struct anv_batch_bo **bbo;
   u_vector_foreach(bbo, &device->cmd_buffer_being_decoded->seen_bbos) {
      /* The decoder zeroes out the top 16 bits, so we need to as well */
            if (address >= bo_address && address < bo_address + (*bbo)->bo->size) {
      return (struct intel_batch_decode_bo) {
      .addr = bo_address,
   .size = (*bbo)->bo->size,
                  uint32_t dep_words = (*bbo)->relocs.dep_words;
   BITSET_WORD *deps = (*bbo)->relocs.deps;
   for (uint32_t w = 0; w < dep_words; w++) {
      BITSET_WORD mask = deps[w];
   while (mask) {
      int i = u_bit_scan(&mask);
   uint32_t gem_handle = w * BITSET_WORDBITS + i;
   struct anv_bo *bo = anv_device_lookup_bo(device, gem_handle);
   assert(bo->refcount > 0);
   bo_address = bo->offset & (~0ull >> 16);
   if (address >= bo_address && address < bo_address + bo->size) {
      return (struct intel_batch_decode_bo) {
      .addr = bo_address,
   .size = bo->size,
                              }
      struct intel_aux_map_buffer {
      struct intel_buffer base;
      };
      static struct intel_buffer *
   intel_aux_map_buffer_alloc(void *driver_ctx, uint32_t size)
   {
      struct intel_aux_map_buffer *buf = malloc(sizeof(struct intel_aux_map_buffer));
   if (!buf)
                     struct anv_state_pool *pool = &device->dynamic_state_pool;
            buf->base.gpu = pool->block_pool.bo->offset + buf->state.offset;
   buf->base.gpu_end = buf->base.gpu + buf->state.alloc_size;
   buf->base.map = buf->state.map;
   buf->base.driver_bo = &buf->state;
      }
      static void
   intel_aux_map_buffer_free(void *driver_ctx, struct intel_buffer *buffer)
   {
      struct intel_aux_map_buffer *buf = (struct intel_aux_map_buffer*)buffer;
   struct anv_device *device = (struct anv_device*)driver_ctx;
   struct anv_state_pool *pool = &device->dynamic_state_pool;
   anv_state_pool_free(pool, buf->state);
      }
      static struct intel_mapped_pinned_buffer_alloc aux_map_allocator = {
      .alloc = intel_aux_map_buffer_alloc,
      };
      static VkResult
   anv_device_setup_context_or_vm(struct anv_device *device,
               {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
         case INTEL_KMD_TYPE_XE:
         default:
      unreachable("Missing");
         }
      static bool
   anv_device_destroy_context_or_vm(struct anv_device *device)
   {
      switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      if (device->physical->has_vm_control)
         else
      case INTEL_KMD_TYPE_XE:
         default:
      unreachable("Missing");
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
      !strcmp(physical_device->instance->vk.app_info.app_name, "HITMAN3.exe")) {
   vk_device_dispatch_table_from_entrypoints(&dispatch_table, &hitman3_device_entrypoints, true);
      }
   if (physical_device->info.ver < 12 &&
      physical_device->instance->vk.app_info.app_name &&
   !strcmp(physical_device->instance->vk.app_info.app_name, "DOOM 64")) {
   vk_device_dispatch_table_from_entrypoints(&dispatch_table, &doom64_device_entrypoints, true);
         #ifdef ANDROID
      vk_device_dispatch_table_from_entrypoints(&dispatch_table, &android_device_entrypoints, true);
      #endif
      vk_device_dispatch_table_from_entrypoints(&dispatch_table,
      anv_genX(&physical_device->info, device_entrypoints),
      vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
            result = vk_device_init(&device->vk, &physical_device->vk,
         if (result != VK_SUCCESS)
            if (INTEL_DEBUG(DEBUG_BATCH | DEBUG_BATCH_STATS)) {
      for (unsigned i = 0; i < physical_device->queue.family_count; i++) {
                        intel_batch_decode_ctx_init(decoder,
                                    decoder->engine = physical_device->queue.families[i].engine_class;
   decoder->dynamic_base = physical_device->va.dynamic_state_pool.addr;
   decoder->surface_base = physical_device->va.internal_surface_state_pool.addr;
                  anv_device_set_physical(device, physical_device);
            /* XXX(chadv): Can we dup() physicalDevice->fd here? */
   device->fd = open(physical_device->path, O_RDWR | O_CLOEXEC);
   if (device->fd == -1) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
               switch (device->info->kmd_type) {
   case INTEL_KMD_TYPE_I915:
      device->vk.check_status = anv_i915_device_check_status;
      case INTEL_KMD_TYPE_XE:
      device->vk.check_status = anv_xe_device_check_status;
      default:
                  device->vk.command_buffer_ops = &anv_cmd_buffer_ops;
   device->vk.create_sync_for_memory = anv_create_sync_for_memory;
   if (physical_device->info.kmd_type == INTEL_KMD_TYPE_I915)
                  uint32_t num_queues = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++)
            result = anv_device_setup_context_or_vm(device, pCreateInfo, num_queues);
   if (result != VK_SUCCESS)
            device->queues =
      vk_zalloc(&device->vk.alloc, num_queues * sizeof(*device->queues), 8,
      if (device->queues == NULL) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               device->queue_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const VkDeviceQueueCreateInfo *queueCreateInfo =
            for (uint32_t j = 0; j < queueCreateInfo->queueCount; j++) {
      result = anv_queue_init(device, &device->queues[device->queue_count],
                                       if (pthread_mutex_init(&device->vma_mutex, NULL) != 0) {
      result = vk_error(device, VK_ERROR_INITIALIZATION_FAILED);
               /* keep the page with address zero out of the allocator */
   util_vma_heap_init(&device->vma_lo,
                  util_vma_heap_init(&device->vma_cva,
                  util_vma_heap_init(&device->vma_hi,
                  util_vma_heap_init(&device->vma_desc,
                  list_inithead(&device->memory_objects);
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
            anv_bo_pool_init(&device->batch_bo_pool, device, "batch",
                     if (device->vk.enabled_extensions.KHR_acceleration_structure) {
      anv_bo_pool_init(&device->bvh_bo_pool, device, "bvh build",
               /* Because scratch is also relative to General State Base Address, we leave
   * the base address 0 and start the pool memory at an offset.  This way we
   * get the correct offsets in the anv_states that get allocated from it.
   */
   result = anv_state_pool_init(&device->general_state_pool, device,
               if (result != VK_SUCCESS)
            result = anv_state_pool_init(&device->dynamic_state_pool, device,
               if (result != VK_SUCCESS)
            /* The border color pointer is limited to 24 bits, so we need to make
   * sure that any such color used at any point in the program doesn't
   * exceed that limit.
   * We achieve that by reserving all the custom border colors we support
   * right off the bat, so they are close to the base address.
   */
   anv_state_reserved_pool_init(&device->custom_border_colors,
                        result = anv_state_pool_init(&device->instruction_state_pool, device,
                     if (result != VK_SUCCESS)
            if (device->info->verx10 >= 125) {
      /* Put the scratch surface states at the beginning of the internal
   * surface state pool.
   */
   result = anv_state_pool_init(&device->scratch_surface_state_pool, device,
                     if (result != VK_SUCCESS)
            result = anv_state_pool_init(&device->internal_surface_state_pool, device,
                        } else {
      result = anv_state_pool_init(&device->internal_surface_state_pool, device,
                  }
   if (result != VK_SUCCESS)
            if (device->physical->indirect_descriptors) {
      result = anv_state_pool_init(&device->bindless_surface_state_pool, device,
                     if (result != VK_SUCCESS)
               if (device->info->verx10 >= 125) {
      /* We're using 3DSTATE_BINDING_TABLE_POOL_ALLOC to give the binding
   * table its own base address separately from surface state base.
   */
   result = anv_state_pool_init(&device->binding_table_pool, device,
                  } else {
      /* The binding table should be in front of the surface states in virtual
   * address space so that all surface states can be express as relative
   * offsets from the binding table location.
   */
   assert(device->physical->va.binding_table_pool.addr <
         int64_t bt_pool_offset = (int64_t)device->physical->va.binding_table_pool.addr -
         assert(INT32_MIN < bt_pool_offset && bt_pool_offset < 0);
   result = anv_state_pool_init(&device->binding_table_pool, device,
                        }
   if (result != VK_SUCCESS)
            result = anv_state_pool_init(&device->push_descriptor_pool, device,
                     if (result != VK_SUCCESS)
            if (device->info->has_aux_map) {
      device->aux_map_ctx = intel_aux_map_init(device, &aux_map_allocator,
         if (!device->aux_map_ctx)
               result = anv_device_alloc_bo(device, "workaround", 8192,
                           if (result != VK_SUCCESS)
            device->workaround_address = (struct anv_address) {
      .bo = device->workaround_bo,
   .offset = align(intel_debug_write_identifiers(device->workaround_bo->map,
                              device->rt_uuid_addr = anv_address_add(device->workaround_address, 8);
   memcpy(device->rt_uuid_addr.bo->map + device->rt_uuid_addr.offset,
                  device->debug_frame_desc =
      intel_debug_get_identifier_block(device->workaround_bo->map,
               if (device->vk.enabled_extensions.KHR_ray_query) {
      uint32_t ray_queries_size =
            result = anv_device_alloc_bo(device, "ray queries",
                           if (result != VK_SUCCESS)
               result = anv_device_init_trivial_batch(device);
   if (result != VK_SUCCESS)
            /* Emit the CPS states before running the initialization batch as those
   * structures are referenced.
   */
   if (device->info->ver >= 12) {
               if (device->info->has_coarse_pixel_primitive_and_cb)
                     /* Each of the combinaison must be replicated on all viewports */
            device->cps_states =
      anv_state_pool_alloc(&device->dynamic_state_pool,
            if (device->cps_states.map == NULL)
                        if (device->physical->indirect_descriptors) {
      /* Allocate a null surface state at surface state offset 0. This makes
   * NULL descriptor handling trivial because we can just memset
   * structures to zero and they have a valid descriptor.
   */
   device->null_surface_state =
      anv_state_pool_alloc(&device->bindless_surface_state_pool,
            isl_null_fill_state(&device->isl_dev, device->null_surface_state.map,
            } else {
      /* When using direct descriptors, those can hold the null surface state
   * directly. We still need a null surface for the binding table entries
   * though but this one can live anywhere the internal surface state
   * pool.
   */
   device->null_surface_state =
      anv_state_pool_alloc(&device->internal_surface_state_pool,
            isl_null_fill_state(&device->isl_dev, device->null_surface_state.map,
                        /* TODO(RT): Do we want some sort of data structure for this? */
            if (ANV_SUPPORT_RT && device->info->has_ray_tracing) {
      /* The docs say to always allocate 128KB per DSS */
   const uint32_t btd_fifo_bo_size =
         result = anv_device_alloc_bo(device,
                                 if (result != VK_SUCCESS)
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
               /* The device (currently is ICL/TGL) does not have float64 support. */
   if (!device->info->has_64bit_float &&
      device->physical->instance->fp64_workaround_enabled)
         result = anv_device_init_rt_shaders(device);
   if (result != VK_SUCCESS) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
            #ifdef ANDROID
         #endif
         device->robust_buffer_access =
      device->vk.enabled_features.robustBufferAccess ||
         device->breakpoint = anv_state_pool_alloc(&device->dynamic_state_pool, 4,
                  /* Create a separate command pool for companion RCS command buffer. */
   if (device->info->verx10 >= 125) {
      VkCommandPoolCreateInfo pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
   .queueFamilyIndex =
               result = vk_common_CreateCommandPool(anv_device_to_handle(device),
               if (result != VK_SUCCESS) {
                                                                           BITSET_ONES(device->gfx_dirty_state);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_INDEX_BUFFER);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_SO_DECL_LIST);
   if (device->info->ver < 11)
         if (device->info->ver < 12) {
      BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_PRIMITIVE_REPLICATION);
      }
   if (!device->vk.enabled_extensions.EXT_sample_locations)
         if (!device->vk.enabled_extensions.KHR_fragment_shading_rate)
         if (!device->vk.enabled_extensions.EXT_mesh_shader) {
      BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_SBE_MESH);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_CLIP_MESH);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_MESH_CONTROL);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_MESH_SHADER);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_MESH_DISTRIB);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_TASK_CONTROL);
   BITSET_CLEAR(device->gfx_dirty_state, ANV_GFX_STATE_TASK_SHADER);
      }
   if (!intel_needs_workaround(device->info, 18019816803))
         if (device->info->ver > 9)
                           fail_internal_cache:
         fail_default_pipeline_cache:
         fail_btd_fifo_bo:
      if (ANV_SUPPORT_RT && device->info->has_ray_tracing)
      fail_trivial_batch_bo_and_scratch_pool:
         fail_trivial_batch:
         fail_ray_query_bo:
      if (device->ray_query_bo)
      fail_workaround_bo:
         fail_surface_aux_map_pool:
      if (device->info->has_aux_map) {
      intel_aux_map_finish(device->aux_map_ctx);
         fail_push_descriptor_pool:
         fail_binding_table_pool:
         fail_bindless_surface_state_pool:
      if (device->physical->indirect_descriptors)
      fail_internal_surface_state_pool:
         fail_scratch_surface_state_pool:
      if (device->info->verx10 >= 125)
      fail_instruction_state_pool:
         fail_dynamic_state_pool:
      anv_state_reserved_pool_finish(&device->custom_border_colors);
      fail_general_state_pool:
         fail_batch_bo_pool:
      if (device->vk.enabled_extensions.KHR_acceleration_structure)
         anv_bo_pool_finish(&device->batch_bo_pool);
      fail_queue_cond:
         fail_mutex:
         fail_vmas:
      util_vma_heap_finish(&device->vma_desc);
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
         #ifdef ANDROID
         #endif
                  for (uint32_t i = 0; i < device->queue_count; i++)
                                                               vk_pipeline_cache_destroy(device->internal_cache, NULL);
            if (ANV_SUPPORT_RT && device->info->has_ray_tracing)
            if (device->info->verx10 >= 125) {
      vk_common_DestroyCommandPool(anv_device_to_handle(device),
            #ifdef HAVE_VALGRIND
      /* We only need to free these to prevent valgrind errors.  The backing
   * BO will go away in a couple of lines so we don't actually leak.
   */
   anv_state_reserved_pool_finish(&device->custom_border_colors);
   anv_state_pool_free(&device->dynamic_state_pool, device->border_colors);
   anv_state_pool_free(&device->dynamic_state_pool, device->slice_hash);
   anv_state_pool_free(&device->dynamic_state_pool, device->cps_states);
      #endif
         for (unsigned i = 0; i < ARRAY_SIZE(device->rt_scratch_bos); i++) {
      if (device->rt_scratch_bos[i] != NULL)
                        if (device->vk.enabled_extensions.KHR_ray_query) {
      for (unsigned i = 0; i < ARRAY_SIZE(device->ray_query_shadow_bos); i++) {
      if (device->ray_query_shadow_bos[i] != NULL)
      }
      }
   anv_device_release_bo(device, device->workaround_bo);
            if (device->info->has_aux_map) {
      intel_aux_map_finish(device->aux_map_ctx);
               anv_state_pool_finish(&device->push_descriptor_pool);
   anv_state_pool_finish(&device->binding_table_pool);
   if (device->info->verx10 >= 125)
         anv_state_pool_finish(&device->internal_surface_state_pool);
   if (device->physical->indirect_descriptors)
         anv_state_pool_finish(&device->instruction_state_pool);
   anv_state_pool_finish(&device->dynamic_state_pool);
            if (device->vk.enabled_extensions.KHR_acceleration_structure)
                           util_vma_heap_finish(&device->vma_desc);
   util_vma_heap_finish(&device->vma_hi);
   util_vma_heap_finish(&device->vma_cva);
            pthread_cond_destroy(&device->queue_submit);
                              if (INTEL_DEBUG(DEBUG_BATCH | DEBUG_BATCH_STATS)) {
      for (unsigned i = 0; i < pdevice->queue.family_count; i++) {
      if (INTEL_DEBUG(DEBUG_BATCH_STATS))
                                 vk_device_finish(&device->vk);
      }
      VkResult anv_EnumerateInstanceLayerProperties(
      uint32_t*                                   pPropertyCount,
      {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
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
      static struct util_vma_heap *
   anv_vma_heap_for_flags(struct anv_device *device,
         {
      if (alloc_flags & ANV_BO_ALLOC_CLIENT_VISIBLE_ADDRESS)
            if (alloc_flags & ANV_BO_ALLOC_32BIT_ADDRESS)
            if (alloc_flags & ANV_BO_ALLOC_DESCRIPTOR_POOL)
               }
      uint64_t
   anv_vma_alloc(struct anv_device *device,
               uint64_t size, uint64_t align,
   enum anv_bo_alloc_flags alloc_flags,
   {
               uint64_t addr = 0;
            if (alloc_flags & ANV_BO_ALLOC_CLIENT_VISIBLE_ADDRESS) {
      if (client_address) {
      if (util_vma_heap_alloc_addr(*out_vma_heap,
                  } else {
         }
   /* We don't want to fall back to other heaps */
                              done:
               assert(addr == intel_48b_address(addr));
      }
      void
   anv_vma_free(struct anv_device *device,
               {
      assert(vma_heap == &device->vma_lo ||
         vma_heap == &device->vma_cva ||
                                          }
      VkResult anv_AllocateMemory(
      VkDevice                                    _device,
   const VkMemoryAllocateInfo*                 pAllocateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   struct anv_physical_device *pdevice = device->physical;
   struct anv_device_memory *mem;
                     VkDeviceSize aligned_alloc_size =
            assert(pAllocateInfo->memoryTypeIndex < pdevice->memory.type_count);
   const struct anv_memory_type *mem_type =
         assert(mem_type->heapIndex < pdevice->memory.heap_count);
   struct anv_memory_heap *mem_heap =
            if (aligned_alloc_size > mem_heap->size)
            uint64_t mem_heap_used = p_atomic_read(&mem_heap->used);
   if (mem_heap_used + aligned_alloc_size > mem_heap->size)
            mem = vk_device_memory_create(&device->vk, pAllocateInfo,
         if (mem == NULL)
            mem->type = mem_type;
   mem->map = NULL;
   mem->map_size = 0;
                     const VkImportMemoryFdInfoKHR *fd_info = NULL;
   const VkMemoryDedicatedAllocateInfo *dedicated_info = NULL;
            vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
   case VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID:
   case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT:
   case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
   case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO:
                  case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
                  case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
      dedicated_info = (void *)ext;
               case VK_STRUCTURE_TYPE_MEMORY_OPAQUE_CAPTURE_ADDRESS_ALLOCATE_INFO: {
      const VkMemoryOpaqueCaptureAddressAllocateInfo *addr_info =
         client_address = addr_info->opaqueCaptureAddress;
               default:
      /* VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA isn't a real
   * enum value, so use conditional to avoid compiler warn
   */
   if (ext->sType == VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA) {
      /* TODO: Android, ChromeOS and other applications may need another
   * way to allocate buffers that can be scanout to display but it
   * should pretty easy to catch those as Xe KMD driver will print
   * warnings in dmesg when scanning buffers allocated without
   * proper flag set.
   */
      } else {
         }
                  /* If i915 reported a mappable/non_mappable vram regions and the
   * application want lmem mappable, then we need to use the
   * I915_GEM_CREATE_EXT_FLAG_NEEDS_CPU_ACCESS flag to create our BO.
   */
   if (pdevice->vram_mappable.size > 0 &&
      pdevice->vram_non_mappable.size > 0 &&
   (mem_type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
   (mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
         if (!mem_heap->is_local_mem)
            if (mem->vk.alloc_flags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT)
            /* Anything imported or exported is EXTERNAL. Apply implicit sync to be
   * compatible with clients relying on implicit fencing. This matches the
   * behavior in iris i915_batch_submit. An example client is VA-API.
   */
   if (mem->vk.export_handle_types || mem->vk.import_handle_type)
            if (mem->vk.ahardware_buffer) {
      result = anv_import_ahw_memory(_device, mem);
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
               if (mem->vk.host_ptr) {
      if (mem->vk.import_handle_type ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_MAPPED_FOREIGN_MEMORY_BIT_EXT) {
   result = vk_error(device, VK_ERROR_INVALID_EXTERNAL_HANDLE);
               assert(mem->vk.import_handle_type ==
            result = anv_device_import_bo_from_host_ptr(device,
                                 if (result != VK_SUCCESS)
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
            if (mem->map) {
      const VkMemoryUnmapInfoKHR unmap = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_UNMAP_INFO_KHR,
      };
               p_atomic_add(&device->physical->memory.heaps[mem->type->heapIndex].used,
                        }
      VkResult anv_MapMemory2KHR(
      VkDevice                                    _device,
   const VkMemoryMapInfoKHR*                   pMemoryMapInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (mem == NULL) {
      *ppData = NULL;
               if (mem->vk.host_ptr) {
      *ppData = mem->vk.host_ptr + pMemoryMapInfo->offset;
               /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * memory must have been created with a memory type that reports
   *    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
   */
   if (!(mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
               assert(pMemoryMapInfo->size > 0);
   const VkDeviceSize offset = pMemoryMapInfo->offset;
   const VkDeviceSize size =
      vk_device_memory_range(&mem->vk, pMemoryMapInfo->offset,
         if (size != (size_t)size) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
                     /* From the Vulkan 1.2.194 spec:
   *
   *    "memory must not be currently host mapped"
   */
   if (mem->map != NULL) {
      return vk_errorf(device, VK_ERROR_MEMORY_MAP_FAILED,
               /* GEM will fail to map if the offset isn't 4k-aligned.  Round down. */
   uint64_t map_offset;
   if (!device->physical->info.has_mmap_offset)
         else
         assert(offset >= map_offset);
            /* Let's map whole pages */
            void *map;
   VkResult result = anv_device_map_bo(device, mem->bo, map_offset, map_size,
         if (result != VK_SUCCESS)
            mem->map = map;
   mem->map_size = map_size;
   mem->map_delta = (offset - map_offset);
               }
      VkResult anv_UnmapMemory2KHR(
      VkDevice                                    _device,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (mem == NULL || mem->vk.host_ptr)
                     mem->map = NULL;
   mem->map_size = 0;
               }
      VkResult anv_FlushMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
   #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
               if (!device->physical->memory.need_flush)
            /* Make sure the writes we're flushing have landed. */
            for (uint32_t i = 0; i < memoryRangeCount; i++) {
      ANV_FROM_HANDLE(anv_device_memory, mem, pMemoryRanges[i].memory);
   if (mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            uint64_t map_offset = pMemoryRanges[i].offset + mem->map_delta;
   if (map_offset >= mem->map_size)
            intel_flush_range(mem->map + map_offset,
               #endif
         }
      VkResult anv_InvalidateMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
   #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
               if (!device->physical->memory.need_flush)
            for (uint32_t i = 0; i < memoryRangeCount; i++) {
      ANV_FROM_HANDLE(anv_device_memory, mem, pMemoryRanges[i].memory);
   if (mem->type->propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            uint64_t map_offset = pMemoryRanges[i].offset + mem->map_delta;
   if (map_offset >= mem->map_size)
            intel_invalidate_range(mem->map + map_offset,
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
            assert(pBindInfo->sType == VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO);
            if (mem) {
      assert(pBindInfo->memoryOffset < mem->vk.size);
   assert(mem->vk.size - pBindInfo->memoryOffset >= buffer->vk.size);
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
            /* The GPU appears to write back to main memory in cachelines. Writes to a
   * buffers should not clobber with writes to another buffers so make sure
   * those are in different cachelines.
   */
            /* From the spec, section "Sparse Buffer and Fully-Resident Image Block
   * Size":
   *   "The sparse block size in bytes for sparse buffers and fully-resident
   *    images is reported as VkMemoryRequirements::alignment. alignment
   *    represents both the memory alignment requirement and the binding
   *    granularity (in bytes) for sparse resources."
   */
   if (is_sparse) {
      alignment = ANV_SPARSE_BLOCK_SIZE;
               pMemoryRequirements->memoryRequirements.size = size;
            /* Storage and Uniform buffers should have their size aligned to
   * 32-bits to avoid boundary checks when last DWord is not complete.
   * This would ensure that not internal padding would be needed for
   * 16-bit types.
   */
   if (device->robust_buffer_access &&
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
      void anv_GetDeviceBufferMemoryRequirementsKHR(
      VkDevice                                    _device,
   const VkDeviceBufferMemoryRequirements*     pInfo,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   const bool is_sparse =
            if (!device->physical->has_sparse &&
      INTEL_DEBUG(DEBUG_SPARSE) &&
   pInfo->pCreateInfo->flags & (VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
               fprintf(stderr, "=== %s %s:%d flags:0x%08x\n", __func__, __FILE__,
         anv_get_buffer_memory_requirements(device,
                        }
      VkResult anv_CreateBuffer(
      VkDevice                                    _device,
   const VkBufferCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!device->physical->has_sparse &&
      INTEL_DEBUG(DEBUG_SPARSE) &&
   pCreateInfo->flags & (VK_BUFFER_CREATE_SPARSE_BINDING_BIT |
               fprintf(stderr, "=== %s %s:%d flags:0x%08x\n", __func__, __FILE__,
         /* Don't allow creating buffers bigger than our address space.  The real
   * issue here is that we may align up the buffer size and we don't want
   * doing so to cause roll-over.  However, no one has any business
   * allocating a buffer larger than our GTT size.
   */
   if (pCreateInfo->size > device->physical->gtt_size)
            buffer = vk_buffer_create(&device->vk, pCreateInfo,
         if (buffer == NULL)
            buffer->address = ANV_NULL_ADDRESS;
   if (anv_buffer_is_sparse(buffer)) {
      const VkBufferOpaqueCaptureAddressCreateInfo *opaque_addr_info =
      vk_find_struct_const(pCreateInfo->pNext,
      enum anv_bo_alloc_flags alloc_flags = 0;
            if (opaque_addr_info) {
      alloc_flags = ANV_BO_ALLOC_CLIENT_VISIBLE_ADDRESS;
               VkResult result = anv_init_sparse_bindings(device, buffer->vk.size,
                     if (result != VK_SUCCESS) {
      vk_buffer_destroy(&device->vk, pAllocator, &buffer->vk);
                              }
      void anv_DestroyBuffer(
      VkDevice                                    _device,
   VkBuffer                                    _buffer,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!buffer)
            if (anv_buffer_is_sparse(buffer)) {
      assert(buffer->address.offset == buffer->sparse_data.address);
                  }
      VkDeviceAddress anv_GetBufferDeviceAddress(
      VkDevice                                    device,
      {
                           }
      uint64_t anv_GetBufferOpaqueCaptureAddress(
      VkDevice                                    device,
      {
                  }
      uint64_t anv_GetDeviceMemoryOpaqueCaptureAddress(
      VkDevice                                    device,
      {
                           }
      void
   anv_fill_buffer_surface_state(struct anv_device *device,
                                 void *surface_state_ptr,
   {
      isl_buffer_fill_state(&device->isl_dev, surface_state_ptr,
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
      pTimestamps[d] = vk_clock_gettime(CLOCK_MONOTONIC);
         #ifdef CLOCK_MONOTONIC_RAW
         case VK_TIME_DOMAIN_CLOCK_MONOTONIC_RAW_EXT:
         #endif
         default:
      pTimestamps[d] = 0;
               #ifdef CLOCK_MONOTONIC_RAW
         #else
         #endif
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
      VkResult anv_GetPhysicalDeviceFragmentShadingRatesKHR(
      VkPhysicalDevice                            physicalDevice,
   uint32_t*                                   pFragmentShadingRateCount,
      {
      ANV_FROM_HANDLE(anv_physical_device, physical_device, physicalDevice);
   VK_OUTARRAY_MAKE_TYPED(VkPhysicalDeviceFragmentShadingRateKHR, out,
         #define append_rate(_samples, _width, _height)                                      \
      do {                                                                             \
      vk_outarray_append_typed(VkPhysicalDeviceFragmentShadingRateKHR, &out, __r) { \
      __r->sampleCounts = _samples;                                              \
   __r->fragmentSize = (VkExtent2D) {                                         \
      .width = _width,                                                        \
                     VkSampleCountFlags sample_counts =
            /* BSpec 47003: There are a number of restrictions on the sample count
   * based off the coarse pixel size.
   */
   static const VkSampleCountFlags cp_size_sample_limits[] = {
      [1]  = ISL_SAMPLE_COUNT_16_BIT | ISL_SAMPLE_COUNT_8_BIT |
         [2]  = ISL_SAMPLE_COUNT_4_BIT | ISL_SAMPLE_COUNT_2_BIT | ISL_SAMPLE_COUNT_1_BIT,
   [4]  = ISL_SAMPLE_COUNT_4_BIT | ISL_SAMPLE_COUNT_2_BIT | ISL_SAMPLE_COUNT_1_BIT,
   [8]  = ISL_SAMPLE_COUNT_2_BIT | ISL_SAMPLE_COUNT_1_BIT,
               for (uint32_t x = 4; x >= 1; x /= 2) {
      for (uint32_t y = 4; y >= 1; y /= 2) {
      if (physical_device->info.has_coarse_pixel_primitive_and_cb) {
      /* BSpec 47003:
   *   "CPsize 1x4 and 4x1 are not supported"
   */
                  /* For size {1, 1}, the sample count must be ~0
   *
   * 4x2 is also a specially case.
   */
   if (x == 1 && y == 1)
         else if (x == 4 && y == 2)
         else
      } else {
      /* For size {1, 1}, the sample count must be ~0 */
   if (x == 1 && y == 1)
         else
                  #undef append_rate
            }
