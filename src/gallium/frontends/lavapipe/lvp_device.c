   /*
   * Copyright © 2019 Red Hat.
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
      #include "lvp_private.h"
   #include "lvp_conv.h"
      #include "pipe-loader/pipe_loader.h"
   #include "git_sha1.h"
   #include "vk_cmd_enqueue_entrypoints.h"
   #include "vk_sampler.h"
   #include "vk_util.h"
   #include "util/detect.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "frontend/drisw_api.h"
      #include "util/u_inlines.h"
   #include "util/os_memory.h"
   #include "util/os_time.h"
   #include "util/u_thread.h"
   #include "util/u_atomic.h"
   #include "util/timespec.h"
   #include "util/ptralloc.h"
   #include "nir.h"
   #include "nir_builder.h"
      #if DETECT_OS_LINUX
   #include <sys/mman.h>
   #endif
      #if defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
      defined(VK_USE_PLATFORM_WIN32_KHR) || \
   defined(VK_USE_PLATFORM_XCB_KHR) || \
      #define LVP_USE_WSI_PLATFORM
   #endif
   #define LVP_API_VERSION VK_MAKE_VERSION(1, 3, VK_HEADER_VERSION)
      VKAPI_ATTR VkResult VKAPI_CALL lvp_EnumerateInstanceVersion(uint32_t* pApiVersion)
   {
      *pApiVersion = LVP_API_VERSION;
      }
      static const struct vk_instance_extension_table lvp_instance_extensions_supported = {
      .KHR_device_group_creation                = true,
   .KHR_external_fence_capabilities          = true,
   .KHR_external_memory_capabilities         = true,
   .KHR_external_semaphore_capabilities      = true,
   .KHR_get_physical_device_properties2      = true,
   .EXT_debug_report                         = true,
      #ifdef LVP_USE_WSI_PLATFORM
      .KHR_get_surface_capabilities2            = true,
   .KHR_surface                              = true,
      #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_WIN32_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
   };
      static const struct vk_device_extension_table lvp_device_extensions_supported = {
      .KHR_8bit_storage                      = true,
   .KHR_16bit_storage                     = true,
   .KHR_bind_memory2                      = true,
   .KHR_buffer_device_address             = true,
   .KHR_create_renderpass2                = true,
   .KHR_copy_commands2                    = true,
   .KHR_dedicated_allocation              = true,
   .KHR_depth_stencil_resolve             = true,
   .KHR_descriptor_update_template        = true,
   .KHR_device_group                      = true,
   .KHR_draw_indirect_count               = true,
   .KHR_driver_properties                 = true,
   .KHR_dynamic_rendering                 = true,
   .KHR_format_feature_flags2             = true,
   .KHR_external_fence                    = true,
      #ifdef PIPE_MEMORY_FD
         #endif
      .KHR_external_semaphore                = true,
   .KHR_shader_float_controls             = true,
      #ifdef LVP_USE_WSI_PLATFORM
         #endif
      .KHR_image_format_list                 = true,
   .KHR_imageless_framebuffer             = true,
   .KHR_maintenance1                      = true,
   .KHR_maintenance2                      = true,
   .KHR_maintenance3                      = true,
   .KHR_maintenance4                      = true,
   .KHR_maintenance5                      = true,
   .KHR_map_memory2                       = true,
   .KHR_multiview                         = true,
   .KHR_push_descriptor                   = true,
   .KHR_pipeline_library                  = true,
   .KHR_relaxed_block_layout              = true,
   .KHR_sampler_mirror_clamp_to_edge      = true,
   .KHR_sampler_ycbcr_conversion          = true,
   .KHR_separate_depth_stencil_layouts    = true,
   .KHR_shader_atomic_int64               = true,
   .KHR_shader_clock                      = true,
   .KHR_shader_draw_parameters            = true,
   .KHR_shader_float16_int8               = true,
   .KHR_shader_integer_dot_product        = true,
   .KHR_shader_non_semantic_info          = true,
   .KHR_shader_subgroup_extended_types    = true,
   .KHR_shader_terminate_invocation       = true,
   .KHR_spirv_1_4                         = true,
      #ifdef LVP_USE_WSI_PLATFORM
      .KHR_swapchain                         = true,
      #endif
      .KHR_synchronization2                  = true,
   .KHR_timeline_semaphore                = true,
   .KHR_uniform_buffer_standard_layout    = true,
   .KHR_variable_pointers                 = true,
   .KHR_vulkan_memory_model               = true,
   .KHR_zero_initialize_workgroup_memory  = true,
   .ARM_rasterization_order_attachment_access = true,
   .EXT_4444_formats                      = true,
   .EXT_attachment_feedback_loop_layout   = true,
   .EXT_attachment_feedback_loop_dynamic_state = true,
   .EXT_border_color_swizzle              = true,
   .EXT_calibrated_timestamps             = true,
   .EXT_color_write_enable                = true,
   .EXT_conditional_rendering             = true,
   .EXT_depth_clip_enable                 = true,
   .EXT_depth_clip_control                = true,
   .EXT_depth_range_unrestricted          = true,
   .EXT_dynamic_rendering_unused_attachments = true,
   .EXT_descriptor_buffer                 = true,
   .EXT_descriptor_indexing               = true,
   .EXT_extended_dynamic_state            = true,
   .EXT_extended_dynamic_state2           = true,
   .EXT_extended_dynamic_state3           = true,
   .EXT_external_memory_host              = true,
   .EXT_graphics_pipeline_library         = true,
   .EXT_host_image_copy                   = true,
   .EXT_host_query_reset                  = true,
   .EXT_image_2d_view_of_3d               = true,
   .EXT_image_sliced_view_of_3d           = true,
   .EXT_image_robustness                  = true,
   .EXT_index_type_uint8                  = true,
   .EXT_inline_uniform_block              = true,
   .EXT_load_store_op_none                = true,
      #if DETECT_OS_LINUX
         #endif
      .EXT_mesh_shader                       = true,
   .EXT_multisampled_render_to_single_sampled = true,
   .EXT_multi_draw                        = true,
   .EXT_mutable_descriptor_type           = true,
   .EXT_nested_command_buffer             = true,
      #if DETECT_OS_LINUX
         #endif
      .EXT_pipeline_creation_feedback        = true,
   .EXT_pipeline_creation_cache_control   = true,
   .EXT_post_depth_coverage               = true,
   .EXT_private_data                      = true,
   .EXT_primitives_generated_query        = true,
   .EXT_primitive_topology_list_restart   = true,
   .EXT_rasterization_order_attachment_access = true,
   .EXT_sampler_filter_minmax             = true,
   .EXT_scalar_block_layout               = true,
   .EXT_separate_stencil_usage            = true,
   .EXT_shader_atomic_float               = true,
   .EXT_shader_atomic_float2              = true,
   .EXT_shader_demote_to_helper_invocation= true,
   .EXT_shader_object                     = true,
   .EXT_shader_stencil_export             = true,
   .EXT_shader_subgroup_ballot            = true,
   .EXT_shader_subgroup_vote              = true,
   .EXT_shader_viewport_index_layer       = true,
   .EXT_subgroup_size_control             = true,
   .EXT_texel_buffer_alignment            = true,
   .EXT_transform_feedback                = true,
   .EXT_vertex_attribute_divisor          = true,
   .EXT_vertex_input_dynamic_state        = true,
   .EXT_ycbcr_image_arrays                = true,
   .EXT_ycbcr_2plane_444_formats          = true,
   .EXT_custom_border_color               = true,
   .EXT_provoking_vertex                  = true,
   .EXT_line_rasterization                = true,
   .EXT_robustness2                       = true,
   .AMDX_shader_enqueue                   = true,
   .GOOGLE_decorate_string                = true,
   .GOOGLE_hlsl_functionality1            = true,
      };
      static int
   min_vertex_pipeline_param(struct pipe_screen *pscreen, enum pipe_shader_cap param)
   {
      int val = INT_MAX;
   for (int i = 0; i < MESA_SHADER_COMPUTE; ++i) {
      if (i == MESA_SHADER_FRAGMENT ||
      !pscreen->get_shader_param(pscreen, i,
                  }
      }
      static int
   min_shader_param(struct pipe_screen *pscreen, enum pipe_shader_cap param)
   {
      return MIN3(min_vertex_pipeline_param(pscreen, param),
            }
      static void
   lvp_get_features(const struct lvp_physical_device *pdevice,
         {
               *features = (struct vk_features){
      /* Vulkan 1.0 */
   .robustBufferAccess                       = true,
   .fullDrawIndexUint32                      = true,
   .imageCubeArray                           = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_CUBE_MAP_ARRAY) != 0),
   .independentBlend                         = true,
   .geometryShader                           = (pdevice->pscreen->get_shader_param(pdevice->pscreen, MESA_SHADER_GEOMETRY, PIPE_SHADER_CAP_MAX_INSTRUCTIONS) != 0),
   .tessellationShader                       = (pdevice->pscreen->get_shader_param(pdevice->pscreen, MESA_SHADER_TESS_EVAL, PIPE_SHADER_CAP_MAX_INSTRUCTIONS) != 0),
   .sampleRateShading                        = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_SAMPLE_SHADING) != 0),
   .dualSrcBlend                             = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_MAX_DUAL_SOURCE_RENDER_TARGETS) != 0),
   .logicOp                                  = true,
   .multiDrawIndirect                        = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_MULTI_DRAW_INDIRECT) != 0),
   .drawIndirectFirstInstance                = true,
   .depthClamp                               = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_DEPTH_CLIP_DISABLE) != 0),
   .depthBiasClamp                           = true,
   .fillModeNonSolid                         = true,
   .depthBounds                              = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_DEPTH_BOUNDS_TEST) != 0),
   .wideLines                                = true,
   .largePoints                              = true,
   .alphaToOne                               = true,
   .multiViewport                            = true,
   .samplerAnisotropy                        = true,
   .textureCompressionETC2                   = false,
   .textureCompressionASTC_LDR               = false,
   .textureCompressionBC                     = true,
   .occlusionQueryPrecise                    = true,
   .pipelineStatisticsQuery                  = true,
   .vertexPipelineStoresAndAtomics           = (min_vertex_pipeline_param(pdevice->pscreen, PIPE_SHADER_CAP_MAX_SHADER_BUFFERS) != 0),
   .fragmentStoresAndAtomics                 = (pdevice->pscreen->get_shader_param(pdevice->pscreen, MESA_SHADER_FRAGMENT, PIPE_SHADER_CAP_MAX_SHADER_BUFFERS) != 0),
   .shaderTessellationAndGeometryPointSize   = true,
   .shaderImageGatherExtended                = true,
   .shaderStorageImageExtendedFormats        = (min_shader_param(pdevice->pscreen, PIPE_SHADER_CAP_MAX_SHADER_IMAGES) != 0),
   .shaderStorageImageMultisample            = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_TEXTURE_MULTISAMPLE) != 0),
   .shaderUniformBufferArrayDynamicIndexing  = true,
   .shaderSampledImageArrayDynamicIndexing   = true,
   .shaderStorageBufferArrayDynamicIndexing  = true,
   .shaderStorageImageArrayDynamicIndexing   = true,
   .shaderStorageImageReadWithoutFormat      = true,
   .shaderStorageImageWriteWithoutFormat     = true,
   .shaderClipDistance                       = true,
   .shaderCullDistance                       = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_CULL_DISTANCE) == 1),
   .shaderFloat64                            = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_DOUBLES) == 1),
   .shaderInt64                              = (pdevice->pscreen->get_param(pdevice->pscreen, PIPE_CAP_INT64) == 1),
   .shaderInt16                              = (min_shader_param(pdevice->pscreen, PIPE_SHADER_CAP_INT16) == 1),
   .variableMultisampleRate                  = false,
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess            = true,
   .uniformAndStorageBuffer16BitAccess  = true,
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
   .samplerMirrorClampToEdge = true,
   .drawIndirectCount = true,
   .storageBuffer8BitAccess = true,
   .uniformAndStorageBuffer8BitAccess = true,
   .storagePushConstant8 = true,
   .shaderBufferInt64Atomics = true,
   .shaderSharedInt64Atomics = true,
   .shaderFloat16 = pdevice->pscreen->get_shader_param(pdevice->pscreen, MESA_SHADER_FRAGMENT, PIPE_SHADER_CAP_FP16) != 0,
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
   .scalarBlockLayout = true,
   .imagelessFramebuffer = true,
   .uniformBufferStandardLayout = true,
   .shaderSubgroupExtendedTypes = true,
   .separateDepthStencilLayouts = true,
   .hostQueryReset = true,
   .timelineSemaphore = true,
   .bufferDeviceAddress = true,
   .bufferDeviceAddressCaptureReplay = false,
   .bufferDeviceAddressMultiDevice = false,
   .vulkanMemoryModel = true,
   .vulkanMemoryModelDeviceScope = true,
   .vulkanMemoryModelAvailabilityVisibilityChains = true,
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
   .textureCompressionASTC_HDR = VK_FALSE,
   .shaderZeroInitializeWorkgroupMemory = true,
   .dynamicRendering = true,
   .shaderIntegerDotProduct = true,
            /* VK_EXT_descriptor_buffer */
   .descriptorBuffer = true,
   .descriptorBufferCaptureReplay = false,
   .descriptorBufferPushDescriptors = true,
            /* VK_EXT_primitives_generated_query */
   .primitivesGeneratedQuery = true,
   .primitivesGeneratedQueryWithRasterizerDiscard = true,
            /* VK_EXT_border_color_swizzle */
   .borderColorSwizzle = true,
            /* VK_EXT_non_seamless_cube_map */
            /* VK_EXT_attachment_feedback_loop_layout */
            /* VK_EXT_rasterization_order_attachment_access */
   .rasterizationOrderColorAttachmentAccess = true,
   .rasterizationOrderDepthAttachmentAccess = true,
            /* VK_EXT_line_rasterization */
   .rectangularLines = true,
   .bresenhamLines = true,
   .smoothLines = true,
   .stippledRectangularLines = true,
   .stippledBresenhamLines = true,
            /* VK_EXT_vertex_attribute_divisor */
   .vertexAttributeInstanceRateZeroDivisor = instance_divisor,
            /* VK_EXT_multisampled_render_to_single_sampled */
            /* VK_EXT_mutable_descriptor_type */
            /* VK_EXT_index_type_uint8 */
            /* VK_EXT_vertex_input_dynamic_state */
            /* VK_EXT_image_sliced_view_of_3d */
            /* VK_EXT_depth_clip_control */
            /* VK_EXT_attachment_feedback_loop_layout_dynamic_state */
            /* VK_EXT_shader_object */
            /* VK_KHR_shader_clock */
   .shaderSubgroupClock = true,
            /* VK_EXT_texel_buffer_alignment */
            /* VK_EXT_transform_feedback */
   .transformFeedback = true,
            /* VK_EXT_conditional_rendering */
   .conditionalRendering = true,
            /* VK_EXT_extended_dynamic_state */
            /* VK_EXT_4444_formats */
   .formatA4R4G4B4 = true,
            /* VK_EXT_custom_border_color */
   .customBorderColors = true,
            /* VK_EXT_color_write_enable */
            /* VK_EXT_image_2d_view_of_3d  */
   .image2DViewOf3D = true,
            /* VK_EXT_provoking_vertex */
   .provokingVertexLast = true,
            /* VK_EXT_multi_draw */
            /* VK_EXT_depth_clip_enable */
            /* VK_EXT_extended_dynamic_state2 */
   .extendedDynamicState2 = true,
   .extendedDynamicState2LogicOp = true,
            /* VK_EXT_extended_dynamic_state3 */
   .extendedDynamicState3PolygonMode = true,
   .extendedDynamicState3TessellationDomainOrigin = true,
   .extendedDynamicState3DepthClampEnable = true,
   .extendedDynamicState3DepthClipEnable = true,
   .extendedDynamicState3LogicOpEnable = true,
   .extendedDynamicState3SampleMask = true,
   .extendedDynamicState3RasterizationSamples = true,
   .extendedDynamicState3AlphaToCoverageEnable = true,
   .extendedDynamicState3AlphaToOneEnable = true,
   .extendedDynamicState3DepthClipNegativeOneToOne = true,
   .extendedDynamicState3RasterizationStream = false,
   .extendedDynamicState3ConservativeRasterizationMode = false,
   .extendedDynamicState3ExtraPrimitiveOverestimationSize = false,
   .extendedDynamicState3LineRasterizationMode = true,
   .extendedDynamicState3LineStippleEnable = true,
   .extendedDynamicState3ProvokingVertexMode = true,
   .extendedDynamicState3SampleLocationsEnable = false,
   .extendedDynamicState3ColorBlendEnable = true,
   .extendedDynamicState3ColorBlendEquation = true,
   .extendedDynamicState3ColorWriteMask = true,
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
            /* VK_EXT_dynamic_rendering_unused_attachments */
            /* VK_EXT_robustness2 */
   .robustBufferAccess2 = true,
   .robustImageAccess2 = true,
            /* VK_NV_device_generated_commands */
            /* VK_EXT_primitive_topology_list_restart */
   .primitiveTopologyListRestart = true,
            /* VK_EXT_graphics_pipeline_library */
            /* VK_EXT_shader_atomic_float */
   .shaderBufferFloat32Atomics =    true,
   .shaderBufferFloat32AtomicAdd =  true,
   .shaderBufferFloat64Atomics =    false,
   .shaderBufferFloat64AtomicAdd =  false,
   .shaderSharedFloat32Atomics =    true,
   .shaderSharedFloat32AtomicAdd =  true,
   .shaderSharedFloat64Atomics =    false,
   .shaderSharedFloat64AtomicAdd =  false,
   .shaderImageFloat32Atomics =     true,
   .shaderImageFloat32AtomicAdd =   true,
   .sparseImageFloat32Atomics =     false,
            /* VK_EXT_shader_atomic_float2 */
   .shaderBufferFloat16Atomics      = false,
   .shaderBufferFloat16AtomicAdd    = false,
   .shaderBufferFloat16AtomicMinMax = false,
   .shaderBufferFloat32AtomicMinMax = LLVM_VERSION_MAJOR >= 15,
   .shaderBufferFloat64AtomicMinMax = false,
   .shaderSharedFloat16Atomics      = false,
   .shaderSharedFloat16AtomicAdd    = false,
   .shaderSharedFloat16AtomicMinMax = false,
   .shaderSharedFloat32AtomicMinMax = LLVM_VERSION_MAJOR >= 15,
   .shaderSharedFloat64AtomicMinMax = false,
   .shaderImageFloat32AtomicMinMax  = LLVM_VERSION_MAJOR >= 15,
            /* VK_EXT_memory_priority */
            /* VK_EXT_pageable_device_local_memory */
            /* VK_EXT_nested_command_buffer */
   .nestedCommandBuffer = true,
   .nestedCommandBufferRendering = true,
            /* VK_EXT_mesh_shader */
   .taskShader = true,
   .meshShader = true,
   .multiviewMeshShader = false,
   .primitiveFragmentShadingRateMeshShader = false,
            /* host_image_copy */
            /* maintenance5 */
            /* VK_EXT_ycbcr_2plane_444_formats */
            /* VK_EXT_ycbcr_image_arrays */
            #ifdef VK_ENABLE_BETA_EXTENSIONS
         #endif
         }
      extern unsigned lp_native_vector_width;
      static VkImageLayout lvp_host_copy_image_layouts[] = {
      VK_IMAGE_LAYOUT_GENERAL,
   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
   VK_IMAGE_LAYOUT_PREINITIALIZED,
   VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL,
   VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
   VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
   VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR,
   VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR,
   VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR,
   VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR,
   VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT,
      };
      static void
   lvp_get_properties(const struct lvp_physical_device *device, struct vk_properties *p)
   {
               uint64_t grid_size[3], block_size[3];
            device->pscreen->get_compute_param(device->pscreen, PIPE_SHADER_IR_NIR,
         device->pscreen->get_compute_param(device->pscreen, PIPE_SHADER_IR_NIR,
         device->pscreen->get_compute_param(device->pscreen, PIPE_SHADER_IR_NIR,
               device->pscreen->get_compute_param(device->pscreen, PIPE_SHADER_IR_NIR,
                                    *p = (struct vk_properties) {
      /* Vulkan 1.0 */
   .apiVersion = LVP_API_VERSION,
   .driverVersion = 1,
   .vendorID = VK_VENDOR_ID_MESA,
   .deviceID = 0,
   .deviceType = VK_PHYSICAL_DEVICE_TYPE_CPU,
   .maxImageDimension1D                      = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_2D_SIZE),
   .maxImageDimension2D                      = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_2D_SIZE),
   .maxImageDimension3D                      = (1 << device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_3D_LEVELS)),
   .maxImageDimensionCube                    = (1 << device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS)),
   .maxImageArrayLayers                      = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS),
   .maxTexelBufferElements                   = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXEL_BUFFER_ELEMENTS_UINT),
   .maxUniformBufferRange                    = min_shader_param(device->pscreen, PIPE_SHADER_CAP_MAX_CONST_BUFFER0_SIZE),
   .maxStorageBufferRange                    = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_SHADER_BUFFER_SIZE_UINT),
   .maxPushConstantsSize                     = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount                 = UINT32_MAX,
   .maxSamplerAllocationCount                = 32 * 1024,
   .bufferImageGranularity                   = 64, /* A cache line */
   .sparseAddressSpaceSize                   = 0,
   .maxBoundDescriptorSets                   = MAX_SETS,
   .maxPerStageDescriptorSamplers            = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUniformBuffers      = MAX_DESCRIPTORS,
   .maxPerStageDescriptorStorageBuffers      = MAX_DESCRIPTORS,
   .maxPerStageDescriptorSampledImages       = MAX_DESCRIPTORS,
   .maxPerStageDescriptorStorageImages       = MAX_DESCRIPTORS,
   .maxPerStageDescriptorInputAttachments    = MAX_DESCRIPTORS,
   .maxPerStageResources                     = MAX_DESCRIPTORS,
   .maxDescriptorSetSamplers                 = MAX_DESCRIPTORS,
   .maxDescriptorSetUniformBuffers           = MAX_DESCRIPTORS,
   .maxDescriptorSetUniformBuffersDynamic    = MAX_DESCRIPTORS,
   .maxDescriptorSetStorageBuffers           = MAX_DESCRIPTORS,
   .maxDescriptorSetStorageBuffersDynamic    = MAX_DESCRIPTORS,
   .maxDescriptorSetSampledImages            = MAX_DESCRIPTORS,
   .maxDescriptorSetStorageImages            = MAX_DESCRIPTORS,
   .maxDescriptorSetInputAttachments         = MAX_DESCRIPTORS,
   .maxVertexInputAttributes                 = 32,
   .maxVertexInputBindings                   = 32,
   .maxVertexInputAttributeOffset            = 2047,
   .maxVertexInputBindingStride              = 2048,
   .maxVertexOutputComponents                = 128,
   .maxTessellationGenerationLevel           = 64,
   .maxTessellationPatchSize                 = 32,
   .maxTessellationControlPerVertexInputComponents = 128,
   .maxTessellationControlPerVertexOutputComponents = 128,
   .maxTessellationControlPerPatchOutputComponents = 128,
   .maxTessellationControlTotalOutputComponents = 4096,
   .maxTessellationEvaluationInputComponents = 128,
   .maxTessellationEvaluationOutputComponents = 128,
   .maxGeometryShaderInvocations             = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_GS_INVOCATIONS),
   .maxGeometryInputComponents               = 64,
   .maxGeometryOutputComponents              = 128,
   .maxGeometryOutputVertices                = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_GEOMETRY_OUTPUT_VERTICES),
   .maxGeometryTotalOutputComponents         = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_GEOMETRY_TOTAL_OUTPUT_COMPONENTS),
   .maxFragmentInputComponents               = 128,
   .maxFragmentOutputAttachments             = 8,
   .maxFragmentDualSrcAttachments            = 2,
   .maxFragmentCombinedOutputResources       = max_render_targets +
                           .maxComputeSharedMemorySize               = max_local_size,
   .maxComputeWorkGroupCount                 = { grid_size[0], grid_size[1], grid_size[2] },
   .maxComputeWorkGroupInvocations           = max_threads_per_block,
   .maxComputeWorkGroupSize                  = { block_size[0], block_size[1], block_size[2] },
   .subPixelPrecisionBits                    = device->pscreen->get_param(device->pscreen, PIPE_CAP_RASTERIZER_SUBPIXEL_BITS),
   .subTexelPrecisionBits                    = 8,
   .mipmapPrecisionBits                      = 4,
   .maxDrawIndexedIndexValue                 = UINT32_MAX,
   .maxDrawIndirectCount                     = UINT32_MAX,
   .maxSamplerLodBias                        = 16,
   .maxSamplerAnisotropy                     = 16,
   .maxViewports                             = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_VIEWPORTS),
   .maxViewportDimensions                    = { (1 << 14), (1 << 14) },
   .viewportBoundsRange                      = { -32768.0, 32768.0 },
   .viewportSubPixelBits                     = device->pscreen->get_param(device->pscreen, PIPE_CAP_VIEWPORT_SUBPIXEL_BITS),
   .minMemoryMapAlignment                    = device->pscreen->get_param(device->pscreen, PIPE_CAP_MIN_MAP_BUFFER_ALIGNMENT),
   .minTexelBufferOffsetAlignment            = device->pscreen->get_param(device->pscreen, PIPE_CAP_TEXTURE_BUFFER_OFFSET_ALIGNMENT),
   .minUniformBufferOffsetAlignment          = device->pscreen->get_param(device->pscreen, PIPE_CAP_CONSTANT_BUFFER_OFFSET_ALIGNMENT),
   .minStorageBufferOffsetAlignment          = device->pscreen->get_param(device->pscreen, PIPE_CAP_SHADER_BUFFER_OFFSET_ALIGNMENT),
   .minTexelOffset                           = device->pscreen->get_param(device->pscreen, PIPE_CAP_MIN_TEXEL_OFFSET),
   .maxTexelOffset                           = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXEL_OFFSET),
   .minTexelGatherOffset                     = device->pscreen->get_param(device->pscreen, PIPE_CAP_MIN_TEXTURE_GATHER_OFFSET),
   .maxTexelGatherOffset                     = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_GATHER_OFFSET),
   .minInterpolationOffset                   = -2, /* FIXME */
   .maxInterpolationOffset                   = 2, /* FIXME */
   .subPixelInterpolationOffsetBits          = 8, /* FIXME */
   .maxFramebufferWidth                      = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_2D_SIZE),
   .maxFramebufferHeight                     = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_2D_SIZE),
   .maxFramebufferLayers                     = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS),
   .framebufferColorSampleCounts             = sample_counts,
   .framebufferDepthSampleCounts             = sample_counts,
   .framebufferStencilSampleCounts           = sample_counts,
   .framebufferNoAttachmentsSampleCounts     = sample_counts,
   .maxColorAttachments                      = max_render_targets,
   .sampledImageColorSampleCounts            = sample_counts,
   .sampledImageIntegerSampleCounts          = sample_counts,
   .sampledImageDepthSampleCounts            = sample_counts,
   .sampledImageStencilSampleCounts          = sample_counts,
   .storageImageSampleCounts                 = sample_counts,
   .maxSampleMaskWords                       = 1,
   .timestampComputeAndGraphics              = true,
   .timestampPeriod                          = 1,
   .maxClipDistances                         = 8,
   .maxCullDistances                         = 8,
   .maxCombinedClipAndCullDistances          = 8,
   .discreteQueuePriorities                  = 2,
   .pointSizeRange                           = { 0.0, device->pscreen->get_paramf(device->pscreen, PIPE_CAPF_MAX_POINT_SIZE) },
   .lineWidthRange                           = { 1.0, device->pscreen->get_paramf(device->pscreen, PIPE_CAPF_MAX_LINE_WIDTH) },
   .pointSizeGranularity                     = (1.0 / 8.0),
   .lineWidthGranularity                     = 1.0 / 128.0,
   .strictLines                              = true,
   .standardSampleLocations                  = true,
   .optimalBufferCopyOffsetAlignment         = 128,
   .optimalBufferCopyRowPitchAlignment       = 128,
            /* Vulkan 1.1 */
   /* The LUID is for Windows. */
   .deviceLUIDValid = false,
            .subgroupSize = lp_native_vector_width / 32,
   .subgroupSupportedStages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_TASK_BIT_EXT | VK_SHADER_STAGE_MESH_BIT_EXT,
   .subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT | VK_SUBGROUP_FEATURE_VOTE_BIT | VK_SUBGROUP_FEATURE_ARITHMETIC_BIT | VK_SUBGROUP_FEATURE_BALLOT_BIT,
            .pointClippingBehavior = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES,
   .maxMultiviewViewCount = 6,
   .maxMultiviewInstanceIndex = INT_MAX,
   .protectedNoFault = false,
   .maxPerSetDescriptors = MAX_DESCRIPTORS,
            /* Vulkan 1.2 */
            .conformanceVersion = (VkConformanceVersion){
      .major = 1,
   .minor = 3,
   .subminor = 1,
               .denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .shaderDenormFlushToZeroFloat16 = false,
   .shaderDenormPreserveFloat16 = false,
   .shaderRoundingModeRTEFloat16 = true,
   .shaderRoundingModeRTZFloat16 = false,
            .shaderDenormFlushToZeroFloat32 = false,
   .shaderDenormPreserveFloat32 = false,
   .shaderRoundingModeRTEFloat32 = true,
   .shaderRoundingModeRTZFloat32 = false,
            .shaderDenormFlushToZeroFloat64 = false,
   .shaderDenormPreserveFloat64 = false,
   .shaderRoundingModeRTEFloat64 = true,
   .shaderRoundingModeRTZFloat64 = false,
            .maxUpdateAfterBindDescriptorsInAllPools = UINT32_MAX,
   .shaderUniformBufferArrayNonUniformIndexingNative = true,
   .shaderSampledImageArrayNonUniformIndexingNative = true,
   .shaderStorageBufferArrayNonUniformIndexingNative = true,
   .shaderStorageImageArrayNonUniformIndexingNative = true,
   .shaderInputAttachmentArrayNonUniformIndexingNative = true,
   .robustBufferAccessUpdateAfterBind = true,
   .quadDivergentImplicitLod = true,
   .maxPerStageDescriptorUpdateAfterBindSamplers = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUpdateAfterBindUniformBuffers = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUpdateAfterBindStorageBuffers = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUpdateAfterBindSampledImages = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUpdateAfterBindStorageImages = MAX_DESCRIPTORS,
   .maxPerStageDescriptorUpdateAfterBindInputAttachments = MAX_DESCRIPTORS,
   .maxPerStageUpdateAfterBindResources = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindSamplers = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindUniformBuffers = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindStorageBuffers = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindSampledImages = MAX_DESCRIPTORS,
   .maxDescriptorSetUpdateAfterBindStorageImages = MAX_DESCRIPTORS,
            .supportedDepthResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT | VK_RESOLVE_MODE_AVERAGE_BIT,
   .supportedStencilResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT,
   .independentResolveNone = false,
            .filterMinmaxImageComponentMapping = true,
            .maxTimelineSemaphoreValueDifference = UINT64_MAX,
            /* Vulkan 1.3 */
   .minSubgroupSize = lp_native_vector_width / 32,
   .maxSubgroupSize = lp_native_vector_width / 32,
   .maxComputeWorkgroupSubgroups = 32,
   .requiredSubgroupSizeStages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
   .maxInlineUniformTotalSize = MAX_DESCRIPTOR_UNIFORM_BLOCK_SIZE * MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BLOCKS * MAX_SETS,
   .maxInlineUniformBlockSize = MAX_DESCRIPTOR_UNIFORM_BLOCK_SIZE,
   .maxPerStageDescriptorInlineUniformBlocks = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BLOCKS,
   .maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BLOCKS,
   .maxDescriptorSetInlineUniformBlocks = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BLOCKS,
   .maxDescriptorSetUpdateAfterBindInlineUniformBlocks = MAX_PER_STAGE_DESCRIPTOR_UNIFORM_BLOCKS,
   .storageTexelBufferOffsetAlignmentBytes = texel_buffer_alignment,
   .storageTexelBufferOffsetSingleTexelAlignment = true,
   .uniformTexelBufferOffsetAlignmentBytes = texel_buffer_alignment,
   .uniformTexelBufferOffsetSingleTexelAlignment = true,
            /* VK_KHR_push_descriptor */
            /* VK_EXT_host_image_copy */
   .pCopySrcLayouts = lvp_host_copy_image_layouts,
   .copySrcLayoutCount = ARRAY_SIZE(lvp_host_copy_image_layouts),
   .pCopyDstLayouts = lvp_host_copy_image_layouts,
   .copyDstLayoutCount = ARRAY_SIZE(lvp_host_copy_image_layouts),
            /* VK_EXT_transform_feedback */
   .maxTransformFeedbackStreams = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_VERTEX_STREAMS),
   .maxTransformFeedbackBuffers = device->pscreen->get_param(device->pscreen, PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS),
   .maxTransformFeedbackBufferSize = UINT32_MAX,
   .maxTransformFeedbackStreamDataSize = 512,
   .maxTransformFeedbackBufferDataSize = 512,
   .maxTransformFeedbackBufferDataStride = 512,
   .transformFeedbackQueries = true,
   .transformFeedbackStreamsLinesTriangles = false,
   .transformFeedbackRasterizationStreamSelect = false,
            /* VK_KHR_maintenance5 */
   /* FIXME No idea about most of these ones. */
   .earlyFragmentMultisampleCoverageAfterSampleCounting = true,
   .earlyFragmentSampleMaskTestBeforeSampleCounting = false,
   .depthStencilSwizzleOneSupport = false,
   .polygonModePointSize = true, /* This one is correct. */
   .nonStrictSinglePixelWideLinesUseParallelogram = false,
            /* VK_EXT_extended_dynamic_state3 */
            /* VK_EXT_line_rasterization */
            /* VK_NV_device_generated_commands */
   .maxGraphicsShaderGroupCount = 1<<12,
   .maxIndirectSequenceCount = 1<<20,
   .maxIndirectCommandsTokenCount = MAX_DGC_TOKENS,
   .maxIndirectCommandsStreamCount = MAX_DGC_STREAMS,
   .maxIndirectCommandsTokenOffset = 2047,
   .maxIndirectCommandsStreamStride = 2048,
   .minSequencesCountBufferOffsetAlignment = 4,
   .minSequencesIndexBufferOffsetAlignment = 4,
            /* VK_EXT_external_memory_host */
            /* VK_EXT_custom_border_color */
            /* VK_EXT_provoking_vertex */
   .provokingVertexModePerPipeline = true,
            /* VK_EXT_multi_draw */
            /* VK_EXT_descriptor_buffer */
   .combinedImageSamplerDescriptorSingleArray = VK_TRUE,
   .bufferlessPushDescriptors = VK_TRUE,
   .descriptorBufferOffsetAlignment = 4,
   .maxDescriptorBufferBindings = MAX_SETS,
   .maxResourceDescriptorBufferBindings = MAX_SETS,
   .maxSamplerDescriptorBufferBindings = MAX_SETS,
   .maxEmbeddedImmutableSamplerBindings = MAX_SETS,
   .maxEmbeddedImmutableSamplers = 2032,
   .bufferCaptureReplayDescriptorDataSize = 0,
   .imageCaptureReplayDescriptorDataSize = 0,
   .imageViewCaptureReplayDescriptorDataSize = 0,
   .samplerCaptureReplayDescriptorDataSize = 0,
   .accelerationStructureCaptureReplayDescriptorDataSize = 0,
   .samplerDescriptorSize = sizeof(struct lp_descriptor),
   .combinedImageSamplerDescriptorSize = sizeof(struct lp_descriptor),
   .sampledImageDescriptorSize = sizeof(struct lp_descriptor),
   .storageImageDescriptorSize = sizeof(struct lp_descriptor),
   .uniformTexelBufferDescriptorSize = sizeof(struct lp_descriptor),
   .robustUniformTexelBufferDescriptorSize = sizeof(struct lp_descriptor),
   .storageTexelBufferDescriptorSize = sizeof(struct lp_descriptor),
   .robustStorageTexelBufferDescriptorSize = sizeof(struct lp_descriptor),
   .uniformBufferDescriptorSize = sizeof(struct lp_descriptor),
   .robustUniformBufferDescriptorSize = sizeof(struct lp_descriptor),
   .storageBufferDescriptorSize = sizeof(struct lp_descriptor),
   .robustStorageBufferDescriptorSize = sizeof(struct lp_descriptor),
   .inputAttachmentDescriptorSize = sizeof(struct lp_descriptor),
   .accelerationStructureDescriptorSize = 0,
   .maxSamplerDescriptorBufferRange = 1<<27, //spec minimum
   .maxResourceDescriptorBufferRange = 1<<27, //spec minimum
   .resourceDescriptorBufferAddressSpaceSize = 1<<27, //spec minimum
   .samplerDescriptorBufferAddressSpaceSize = 1<<27, //spec minimum
            /* VK_EXT_graphics_pipeline_library */
   .graphicsPipelineLibraryFastLinking = VK_TRUE,
            /* VK_EXT_robustness2 */
   .robustStorageBufferAccessSizeAlignment = 1,
            /* VK_EXT_mesh_shader */
   .maxTaskWorkGroupTotalCount = 4194304,
   .maxTaskWorkGroupCount[0] = 65536,
   .maxTaskWorkGroupCount[1] = 65536,
   .maxTaskWorkGroupCount[2] = 65536,
   .maxTaskWorkGroupInvocations = 1024,
   .maxTaskWorkGroupSize[0] = 1024,
   .maxTaskWorkGroupSize[1] = 1024,
   .maxTaskWorkGroupSize[2] = 1024,
   .maxTaskPayloadSize = 16384,
   .maxTaskSharedMemorySize = 32768,
            .maxMeshWorkGroupTotalCount = 4194304,
   .maxMeshWorkGroupCount[0] = 65536,
   .maxMeshWorkGroupCount[1] = 65536,
   .maxMeshWorkGroupCount[2] = 65536,
   .maxMeshWorkGroupInvocations = 1024,
   .maxMeshWorkGroupSize[0] = 1024,
   .maxMeshWorkGroupSize[1] = 1024,
   .maxMeshWorkGroupSize[2] = 1024,
   .maxMeshOutputMemorySize = 32768, /* 32K min required */
   .maxMeshSharedMemorySize = 28672,     /* 28K min required */
   .maxMeshOutputComponents = 128, /* 32x vec4 min required */
   .maxMeshOutputVertices = 256,
   .maxMeshOutputPrimitives = 256,
   .maxMeshOutputLayers = 8,
   .meshOutputPerVertexGranularity = 1,
   .meshOutputPerPrimitiveGranularity = 1,
   .maxPreferredTaskWorkGroupInvocations = 64,
   .maxPreferredMeshWorkGroupInvocations = 128,
   .prefersLocalInvocationVertexOutput = true,
   .prefersLocalInvocationPrimitiveOutput = true,
   .prefersCompactVertexOutput = true,
            #ifdef VK_ENABLE_BETA_EXTENSIONS
         .maxExecutionGraphDepth = 32,
   .maxExecutionGraphShaderOutputNodes = LVP_MAX_EXEC_GRAPH_PAYLOADS,
   .maxExecutionGraphShaderPayloadSize = 0xFFFF,
   .maxExecutionGraphShaderPayloadCount = LVP_MAX_EXEC_GRAPH_PAYLOADS,
   #endif
               /* Vulkan 1.0 */
   strcpy(p->deviceName, device->pscreen->get_name(device->pscreen));
            /* Vulkan 1.1 */
   device->pscreen->get_device_uuid(device->pscreen, (char*)(p->deviceUUID));
   device->pscreen->get_driver_uuid(device->pscreen, (char*)(p->driverUUID));
         #if LLVM_VERSION_MAJOR >= 10
         #endif
         /* Vulkan 1.2 */
   snprintf(p->driverName, VK_MAX_DRIVER_NAME_SIZE, "llvmpipe");
      #ifdef MESA_LLVM_VERSION_STRING
         #endif
               /* VK_EXT_nested_command_buffer */
            /* VK_EXT_host_image_copy */
            /* VK_EXT_vertex_attribute_divisor */
   if (device->pscreen->get_param(device->pscreen, PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR) != 0)
         else
            /* VK_EXT_shader_object */
   /* this is basically unsupported */
   lvp_device_get_cache_uuid(p->shaderBinaryUUID);
            /* VK_EXT_mesh_shader */
   p->maxMeshPayloadAndSharedMemorySize = p->maxTaskPayloadSize + p->maxMeshSharedMemorySize; /* 28K min required */
      }
      static VkResult VKAPI_CALL
   lvp_physical_device_init(struct lvp_physical_device *device,
               {
               struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
         vk_physical_device_dispatch_table_from_entrypoints(
         result = vk_physical_device_init(&device->vk, &instance->vk,
         if (result != VK_SUCCESS) {
      vk_error(instance, result);
      }
            device->pscreen = pipe_loader_create_screen_vk(device->pld, true);
   if (!device->pscreen)
         for (unsigned i = 0; i < ARRAY_SIZE(device->drv_options); i++)
            device->sync_timeline_type = vk_sync_timeline_get_type(&lvp_pipe_sync_type);
   device->sync_types[0] = &lvp_pipe_sync_type;
   device->sync_types[1] = &device->sync_timeline_type.sync;
   device->sync_types[2] = NULL;
            device->max_images = device->pscreen->get_shader_param(device->pscreen, MESA_SHADER_FRAGMENT, PIPE_SHADER_CAP_MAX_SHADER_IMAGES);
   device->vk.supported_extensions = lvp_device_extensions_supported;
   lvp_get_features(device, &device->vk.supported_features);
            result = lvp_init_wsi(device);
   if (result != VK_SUCCESS) {
      vk_physical_device_finish(&device->vk);
   vk_error(instance, result);
                  fail:
         }
      static void VKAPI_CALL
   lvp_physical_device_finish(struct lvp_physical_device *device)
   {
      lvp_finish_wsi(device);
   device->pscreen->destroy(device->pscreen);
      }
      static void
   lvp_destroy_physical_device(struct vk_physical_device *device)
   {
      lvp_physical_device_finish((struct lvp_physical_device *)device);
      }
      static VkResult
   lvp_enumerate_physical_devices(struct vk_instance *vk_instance);
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateInstance(
      const VkInstanceCreateInfo*                 pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      struct lvp_instance *instance;
                     if (pAllocator == NULL)
            instance = vk_zalloc(pAllocator, sizeof(*instance), 8,
         if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(
         vk_instance_dispatch_table_from_entrypoints(
            result = vk_instance_init(&instance->vk,
                           if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
                        instance->vk.physical_devices.enumerate = lvp_enumerate_physical_devices;
            //   _mesa_locale_init();
                        }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyInstance(
      VkInstance                                  _instance,
      {
               if (!instance)
                           vk_instance_finish(&instance->vk);
      }
      #if defined(HAVE_DRI)
   static void lvp_get_image(struct dri_drawable *dri_drawable,
               {
      }
      static void lvp_put_image(struct dri_drawable *dri_drawable,
         {
         }
      static void lvp_put_image2(struct dri_drawable *dri_drawable,
               {
         }
      static struct drisw_loader_funcs lvp_sw_lf = {
      .get_image = lvp_get_image,
   .put_image = lvp_put_image,
      };
   #endif
      static VkResult
   lvp_enumerate_physical_devices(struct vk_instance *vk_instance)
   {
      struct lvp_instance *instance =
            /* sw only for now */
                  #if defined(HAVE_DRI)
         #else
         #endif
         struct lvp_physical_device *device =
      vk_zalloc2(&instance->vk.alloc, NULL, sizeof(*device), 8,
      if (!device)
            VkResult result = lvp_physical_device_init(device, instance, &instance->devs[0]);
   if (result == VK_SUCCESS)
         else
               }
      void
   lvp_device_get_cache_uuid(void *uuid)
   {
      memset(uuid, 'a', VK_UUID_SIZE);
   if (MESA_GIT_SHA1[0])
      /* debug build */
      else
      /* release build */
   }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceQueueFamilyProperties2(
      VkPhysicalDevice                            physicalDevice,
   uint32_t*                                   pCount,
      {
               vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p) {
      p->queueFamilyProperties = (VkQueueFamilyProperties) {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
   VK_QUEUE_COMPUTE_BIT |
   VK_QUEUE_TRANSFER_BIT,
   .queueCount = 1,
   .timestampValidBits = 64,
            }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceMemoryProperties(
      VkPhysicalDevice                            physicalDevice,
      {
      pMemoryProperties->memoryTypeCount = 1;
   pMemoryProperties->memoryTypes[0] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
   VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
               VkDeviceSize low_size = 3ULL*1024*1024*1024;
   VkDeviceSize total_size;
   os_get_total_physical_memory(&total_size);
   pMemoryProperties->memoryHeapCount = 1;
   pMemoryProperties->memoryHeaps[0] = (VkMemoryHeap) {
      .size = low_size,
      };
   if (sizeof(void*) > sizeof(uint32_t))
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceMemoryProperties2(
      VkPhysicalDevice                            physicalDevice,
      {
      lvp_GetPhysicalDeviceMemoryProperties(physicalDevice,
         VkPhysicalDeviceMemoryBudgetPropertiesEXT *props = vk_find_struct(pMemoryProperties, PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT);
   if (props) {
      props->heapBudget[0] = pMemoryProperties->memoryProperties.memoryHeaps[0].size;
   os_get_available_system_memory(&props->heapUsage[0]);
   props->heapUsage[0] = props->heapBudget[0] - props->heapUsage[0];
   memset(&props->heapBudget[1], 0, sizeof(props->heapBudget[0]) * (VK_MAX_MEMORY_HEAPS - 1));
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   lvp_GetMemoryHostPointerPropertiesEXT(
      VkDevice _device,
   VkExternalMemoryHandleTypeFlagBits handleType,
   const void *pHostPointer,
      {
      switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT: {
      pMemoryHostPointerProperties->memoryTypeBits = 1;
      }
   default:
            }
      VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL lvp_GetInstanceProcAddr(
      VkInstance                                  _instance,
      {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
   return vk_instance_get_proc_addr(instance,
            }
      /* Windows will use a dll definition file to avoid build errors. */
   #ifdef _WIN32
   #undef PUBLIC
   #define PUBLIC
   #endif
      /* The loader wants us to expose a second GetInstanceProcAddr function
   * to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(
      VkInstance                                  instance,
         PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(
      VkInstance                                  instance,
      {
         }
      PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(
      VkInstance                                  _instance,
         PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetPhysicalDeviceProcAddr(
      VkInstance                                  _instance,
      {
      LVP_FROM_HANDLE(lvp_instance, instance, _instance);
      }
      static void
   destroy_pipelines(struct lvp_queue *queue)
   {
      simple_mtx_lock(&queue->lock);
   while (util_dynarray_contains(&queue->pipeline_destroys, struct lvp_pipeline*)) {
         }
      }
      static VkResult
   lvp_queue_submit(struct vk_queue *vk_queue,
         {
               VkResult result = vk_sync_wait_many(&queue->device->vk,
               if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < submit->command_buffer_count; i++) {
      struct lvp_cmd_buffer *cmd_buffer =
                                 if (submit->command_buffer_count > 0)
            for (uint32_t i = 0; i < submit->signal_count; i++) {
      struct lvp_pipe_sync *sync =
            }
               }
      static VkResult
   lvp_queue_init(struct lvp_device *device, struct lvp_queue *queue,
               {
      VkResult result = vk_queue_init(&queue->vk, &device->vk, create_info,
         if (result != VK_SUCCESS)
            result = vk_queue_enable_submit_thread(&queue->vk);
   if (result != VK_SUCCESS) {
      vk_queue_finish(&queue->vk);
                        queue->ctx = device->pscreen->context_create(device->pscreen, NULL, PIPE_CONTEXT_ROBUST_BUFFER_ACCESS);
   queue->cso = cso_create_context(queue->ctx, CSO_NO_VBUF);
                     simple_mtx_init(&queue->lock, mtx_plain);
               }
      static void
   lvp_queue_finish(struct lvp_queue *queue)
   {
               destroy_pipelines(queue);
   simple_mtx_destroy(&queue->lock);
            u_upload_destroy(queue->uploader);
   cso_destroy_context(queue->cso);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateDevice(
      VkPhysicalDevice                            physicalDevice,
   const VkDeviceCreateInfo*                   pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_physical_device, physical_device, physicalDevice);
   struct lvp_device *device;
                     size_t state_size = lvp_get_rendering_state_size();
   device = vk_zalloc2(&physical_device->vk.instance->alloc, pAllocator,
               if (!device)
            device->queue.state = device + 1;
   device->poison_mem = debug_get_bool_option("LVP_POISON_MEMORY", false);
            struct vk_device_dispatch_table dispatch_table;
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         lvp_add_enqueue_cmd_entrypoints(&dispatch_table);
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         VkResult result = vk_device_init(&device->vk,
                     if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, device);
               vk_device_enable_threaded_submit(&device->vk);
            device->instance = (struct lvp_instance *)physical_device->vk.instance;
                     assert(pCreateInfo->queueCreateInfoCount == 1);
   assert(pCreateInfo->pQueueCreateInfos[0].queueFamilyIndex == 0);
   assert(pCreateInfo->pQueueCreateInfos[0].queueCount == 1);
   result = lvp_queue_init(device, &device->queue, pCreateInfo->pQueueCreateInfos, 0);
   if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, device);
               nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, NULL, "dummy_frag");
   struct pipe_shader_state shstate = {0};
   shstate.type = PIPE_SHADER_IR_NIR;
   shstate.ir.nir = b.shader;
   device->noop_fs = device->queue.ctx->create_fs_state(device->queue.ctx, &shstate);
   _mesa_hash_table_init(&device->bda, NULL, _mesa_hash_pointer, _mesa_key_pointer_equal);
            uint32_t zero = 0;
            device->null_texture_handle = (void *)(uintptr_t)device->queue.ctx->create_texture_handle(device->queue.ctx,
         device->null_image_handle = (void *)(uintptr_t)device->queue.ctx->create_image_handle(device->queue.ctx,
            util_dynarray_init(&device->bda_texture_handles, NULL);
                           }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyDevice(
      VkDevice                                    _device,
      {
               util_dynarray_foreach(&device->bda_texture_handles, struct lp_texture_handle *, handle)
                     util_dynarray_foreach(&device->bda_image_handles, struct lp_texture_handle *, handle)
                     device->queue.ctx->delete_texture_handle(device->queue.ctx, (uint64_t)(uintptr_t)device->null_texture_handle);
                     if (device->queue.last_fence)
         ralloc_free(device->bda.table);
   simple_mtx_destroy(&device->bda_lock);
            lvp_queue_finish(&device->queue);
   vk_device_finish(&device->vk);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_EnumerateInstanceExtensionProperties(
      const char*                                 pLayerName,
   uint32_t*                                   pPropertyCount,
      {
      if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_EnumerateInstanceLayerProperties(
      uint32_t*                                   pPropertyCount,
      {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_EnumerateDeviceLayerProperties(
      VkPhysicalDevice                            physicalDevice,
   uint32_t*                                   pPropertyCount,
      {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
               /* None supported at this time */
      }
      static void
   set_mem_priority(struct lvp_device_memory *mem, int priority)
   {
   #if DETECT_OS_LINUX
      if (priority) {
      #ifdef MADV_COLD
         if (priority < 0)
   #endif
         if (priority > 0)
         if (advice)
         #endif
   }
      static int
   get_mem_priority(float priority)
   {
      if (priority < 0.3)
         if (priority < 0.6)
            }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_AllocateMemory(
      VkDevice                                    _device,
   const VkMemoryAllocateInfo*                 pAllocateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   struct lvp_device_memory *mem;
   ASSERTED const VkExportMemoryAllocateInfo *export_info = NULL;
   ASSERTED const VkImportMemoryFdInfoKHR *import_info = NULL;
   const VkImportMemoryHostPointerInfoEXT *host_ptr_info = NULL;
   VkResult error = VK_ERROR_OUT_OF_DEVICE_MEMORY;
   assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
            if (pAllocateInfo->allocationSize == 0) {
      /* Apparently, this is allowed */
   *pMem = VK_NULL_HANDLE;
               vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch ((unsigned)ext->sType) {
   case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT:
      host_ptr_info = (VkImportMemoryHostPointerInfoEXT*)ext;
   assert(host_ptr_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT);
      case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
      export_info = (VkExportMemoryAllocateInfo*)ext;
   assert(!export_info->handleTypes || export_info->handleTypes == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
      case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
      import_info = (VkImportMemoryFdInfoKHR*)ext;
   assert(import_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT);
      case VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT: {
      VkMemoryPriorityAllocateInfoEXT *prio = (VkMemoryPriorityAllocateInfoEXT*)ext;
   priority = get_mem_priority(prio->priority);
      }
   default:
                  #ifdef PIPE_MEMORY_FD
      if (import_info != NULL && import_info->fd < 0) {
            #endif
         mem = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*mem), 8,
         if (mem == NULL)
            vk_object_base_init(&device->vk, &mem->base,
            mem->memory_type = LVP_DEVICE_MEMORY_TYPE_DEFAULT;
   mem->backed_fd = -1;
            if (host_ptr_info) {
      mem->pmem = host_ptr_info->pHostPointer;
         #ifdef PIPE_MEMORY_FD
      else if(import_info) {
      uint64_t size;
   if(!device->pscreen->import_memory_fd(device->pscreen, import_info->fd, &mem->pmem, &size)) {
      close(import_info->fd);
   error = VK_ERROR_INVALID_EXTERNAL_HANDLE;
      }
   if(size < pAllocateInfo->allocationSize) {
      device->pscreen->free_memory_fd(device->pscreen, mem->pmem);
   close(import_info->fd);
      }
   if (export_info && export_info->handleTypes) {
         }
   else {
         }
      }
   else if (export_info && export_info->handleTypes) {
      mem->pmem = device->pscreen->allocate_memory_fd(device->pscreen, pAllocateInfo->allocationSize, &mem->backed_fd);
   if (!mem->pmem || mem->backed_fd < 0) {
         }
         #endif
      else {
      mem->pmem = device->pscreen->allocate_memory(device->pscreen, pAllocateInfo->allocationSize);
   if (!mem->pmem) {
         }
   if (device->poison_mem)
      /* this is a value that will definitely break things */
                                          fail:
      vk_free2(&device->vk.alloc, pAllocator, mem);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_FreeMemory(
      VkDevice                                    _device,
   VkDeviceMemory                              _mem,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (mem == NULL)
            switch(mem->memory_type) {
   case LVP_DEVICE_MEMORY_TYPE_DEFAULT:
      device->pscreen->free_memory(device->pscreen, mem->pmem);
   #ifdef PIPE_MEMORY_FD
      case LVP_DEVICE_MEMORY_TYPE_OPAQUE_FD:
      device->pscreen->free_memory_fd(device->pscreen, mem->pmem);
   if(mem->backed_fd >= 0)
         #endif
      case LVP_DEVICE_MEMORY_TYPE_USER_PTR:
   default:
         }
   vk_object_base_finish(&mem->base);
         }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_MapMemory2KHR(
      VkDevice                                    _device,
   const VkMemoryMapInfoKHR*                   pMemoryMapInfo,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   LVP_FROM_HANDLE(lvp_device_memory, mem, pMemoryMapInfo->memory);
   void *map;
   if (mem == NULL) {
      *ppData = NULL;
                        *ppData = (char *)map + pMemoryMapInfo->offset;
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_UnmapMemory2KHR(
      VkDevice                                    _device,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (mem == NULL)
            device->pscreen->unmap_memory(device->pscreen, mem->pmem);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_FlushMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
         }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_InvalidateMappedMemoryRanges(
      VkDevice                                    _device,
   uint32_t                                    memoryRangeCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDeviceBufferMemoryRequirements(
      VkDevice                                    _device,
   const VkDeviceBufferMemoryRequirements*     pInfo,
      {
      pMemoryRequirements->memoryRequirements.memoryTypeBits = 1;
   pMemoryRequirements->memoryRequirements.alignment = 64;
            VkBuffer _buffer;
   if (lvp_CreateBuffer(_device, pInfo->pCreateInfo, NULL, &_buffer) != VK_SUCCESS)
         LVP_FROM_HANDLE(lvp_buffer, buffer, _buffer);
   pMemoryRequirements->memoryRequirements.size = buffer->total_size;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDeviceImageSparseMemoryRequirements(
      VkDevice                                    device,
   const VkDeviceImageMemoryRequirements*      pInfo,
   uint32_t*                                   pSparseMemoryRequirementCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDeviceImageMemoryRequirements(
      VkDevice                                    _device,
   const VkDeviceImageMemoryRequirements*     pInfo,
      {
      pMemoryRequirements->memoryRequirements.memoryTypeBits = 1;
   pMemoryRequirements->memoryRequirements.alignment = 0;
            VkImage _image;
   if (lvp_CreateImage(_device, pInfo->pCreateInfo, NULL, &_image) != VK_SUCCESS)
         LVP_FROM_HANDLE(lvp_image, image, _image);
   pMemoryRequirements->memoryRequirements.size = image->size;
   pMemoryRequirements->memoryRequirements.alignment = image->alignment;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetBufferMemoryRequirements(
      VkDevice                                    device,
   VkBuffer                                    _buffer,
      {
               /* The Vulkan spec (git aaed022) says:
   *
   *    memoryTypeBits is a bitfield and contains one bit set for every
   *    supported memory type for the resource. The bit `1<<i` is set if and
   *    only if the memory type `i` in the VkPhysicalDeviceMemoryProperties
   *    structure for the physical device is supported.
   *
   * We support exactly one memory type.
   */
            pMemoryRequirements->size = buffer->total_size;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetBufferMemoryRequirements2(
      VkDevice                                     device,
   const VkBufferMemoryRequirementsInfo2       *pInfo,
      {
      lvp_GetBufferMemoryRequirements(device, pInfo->buffer,
         vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *req =
         req->requiresDedicatedAllocation = false;
   req->prefersDedicatedAllocation = req->requiresDedicatedAllocation;
      }
   default:
               }
      VKAPI_ATTR void VKAPI_CALL lvp_GetImageMemoryRequirements(
      VkDevice                                    device,
   VkImage                                     _image,
      {
      LVP_FROM_HANDLE(lvp_image, image, _image);
            pMemoryRequirements->size = image->size;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetImageMemoryRequirements2(
      VkDevice                                    device,
   const VkImageMemoryRequirementsInfo2       *pInfo,
      {
      lvp_GetImageMemoryRequirements(device, pInfo->image,
            vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *req =
         req->requiresDedicatedAllocation = false;
   req->prefersDedicatedAllocation = req->requiresDedicatedAllocation;
      }
   default:
               }
      VKAPI_ATTR void VKAPI_CALL lvp_GetImageSparseMemoryRequirements(
      VkDevice                                    device,
   VkImage                                     image,
   uint32_t*                                   pSparseMemoryRequirementCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_GetImageSparseMemoryRequirements2(
      VkDevice                                    device,
   const VkImageSparseMemoryRequirementsInfo2* pInfo,
   uint32_t* pSparseMemoryRequirementCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDeviceMemoryCommitment(
      VkDevice                                    device,
   VkDeviceMemory                              memory,
      {
         }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_BindBufferMemory2(VkDevice _device,
               {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   for (uint32_t i = 0; i < bindInfoCount; ++i) {
      LVP_FROM_HANDLE(lvp_device_memory, mem, pBindInfos[i].memory);
            buffer->pmem = mem->pmem;
   buffer->offset = pBindInfos[i].memoryOffset;
   device->pscreen->resource_bind_backing(device->pscreen,
                  }
      }
      static VkResult
   lvp_image_plane_bind(struct lvp_device *device,
                           {
      if (!device->pscreen->resource_bind_backing(device->pscreen,
                        /* This is probably caused by the texture being too large, so let's
   * report this as the *closest* allowed error-code. It's not ideal,
   * but it's unlikely that anyone will care too much.
   */
      }
   plane->pmem = mem->pmem;
   plane->memory_offset = memory_offset;
   plane->plane_offset = *plane_offset;
   *plane_offset += plane->size;
      }
         VKAPI_ATTR VkResult VKAPI_CALL lvp_BindImageMemory2(VkDevice _device,
               {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   for (uint32_t i = 0; i < bindInfoCount; ++i) {
      const VkBindImageMemoryInfo *bind_info = &pBindInfos[i];
   LVP_FROM_HANDLE(lvp_device_memory, mem, bind_info->memory);
   LVP_FROM_HANDLE(lvp_image, image, bind_info->image);
            vk_foreach_struct_const(s, bind_info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_SWAPCHAIN_INFO_KHR: {
      const VkBindImageMemorySwapchainInfoKHR *swapchain_info =
         struct lvp_image *swapchain_image =
                  image->planes[0].pmem = swapchain_image->planes[0].pmem;
   image->planes[0].memory_offset = swapchain_image->planes[0].memory_offset;
   device->pscreen->resource_bind_backing(device->pscreen,
                     did_bind = true;
      }
   default:
                     if (!did_bind) {
      uint64_t offset_B = 0;
   VkResult result;
   if (image->disjoint) {
      const VkBindImagePlaneMemoryInfo *plane_info =
         uint8_t plane = lvp_image_aspects_to_plane(image, plane_info->planeAspect);
   result = lvp_image_plane_bind(device, &image->planes[plane],
         if (result != VK_SUCCESS)
      } else {
      for (unsigned plane = 0; plane < image->plane_count; plane++) {
      result = lvp_image_plane_bind(device, &image->planes[plane],
         if (result != VK_SUCCESS)
               }
      }
      #ifdef PIPE_MEMORY_FD
      VkResult
   lvp_GetMemoryFdKHR(VkDevice _device, const VkMemoryGetFdInfoKHR *pGetFdInfo, int *pFD)
   {
               assert(pGetFdInfo->sType == VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR);
            *pFD = dup(memory->backed_fd);
   assert(*pFD >= 0);
      }
      VkResult
   lvp_GetMemoryFdPropertiesKHR(VkDevice _device,
                     {
                        if(handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT) {
      // There is only one memoryType so select this one
      }
   else
            }
      #endif
      VKAPI_ATTR VkResult VKAPI_CALL lvp_QueueBindSparse(
      VkQueue                                     queue,
   uint32_t                                    bindInfoCount,
   const VkBindSparseInfo*                     pBindInfo,
      {
         }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateEvent(
      VkDevice                                    _device,
   const VkEventCreateInfo*                    pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   struct lvp_event *event = vk_alloc2(&device->vk.alloc, pAllocator,
                  if (!event)
            vk_object_base_init(&device->vk, &event->base, VK_OBJECT_TYPE_EVENT);
   *pEvent = lvp_event_to_handle(event);
               }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyEvent(
      VkDevice                                    _device,
   VkEvent                                     _event,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!event)
            vk_object_base_finish(&event->base);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetEventStatus(
      VkDevice                                    _device,
      {
      LVP_FROM_HANDLE(lvp_event, event, _event);
   if (event->event_storage == 1)
            }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_SetEvent(
      VkDevice                                    _device,
      {
      LVP_FROM_HANDLE(lvp_event, event, _event);
               }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_ResetEvent(
      VkDevice                                    _device,
      {
      LVP_FROM_HANDLE(lvp_event, event, _event);
               }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateSampler(
      VkDevice                                    _device,
   const VkSamplerCreateInfo*                  pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            sampler = vk_sampler_create(&device->vk, pCreateInfo,
         if (!sampler)
            struct pipe_sampler_state state = {0};
   VkClearColorValue border_color =
                  state.wrap_s = vk_conv_wrap_mode(pCreateInfo->addressModeU);
   state.wrap_t = vk_conv_wrap_mode(pCreateInfo->addressModeV);
   state.wrap_r = vk_conv_wrap_mode(pCreateInfo->addressModeW);
   state.min_img_filter = pCreateInfo->minFilter == VK_FILTER_LINEAR ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
   state.min_mip_filter = pCreateInfo->mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR ? PIPE_TEX_MIPFILTER_LINEAR : PIPE_TEX_MIPFILTER_NEAREST;
   state.mag_img_filter = pCreateInfo->magFilter == VK_FILTER_LINEAR ? PIPE_TEX_FILTER_LINEAR : PIPE_TEX_FILTER_NEAREST;
   state.min_lod = pCreateInfo->minLod;
   state.max_lod = pCreateInfo->maxLod;
   state.lod_bias = pCreateInfo->mipLodBias;
   if (pCreateInfo->anisotropyEnable)
         else
         state.unnormalized_coords = pCreateInfo->unnormalizedCoordinates;
   state.compare_mode = pCreateInfo->compareEnable ? PIPE_TEX_COMPARE_R_TO_TEXTURE : PIPE_TEX_COMPARE_NONE;
   state.compare_func = pCreateInfo->compareOp;
   state.seamless_cube_map = !(pCreateInfo->flags & VK_SAMPLER_CREATE_NON_SEAMLESS_CUBE_MAP_BIT_EXT);
   STATIC_ASSERT((unsigned)VK_SAMPLER_REDUCTION_MODE_WEIGHTED_AVERAGE == (unsigned)PIPE_TEX_REDUCTION_WEIGHTED_AVERAGE);
   STATIC_ASSERT((unsigned)VK_SAMPLER_REDUCTION_MODE_MIN == (unsigned)PIPE_TEX_REDUCTION_MIN);
   STATIC_ASSERT((unsigned)VK_SAMPLER_REDUCTION_MODE_MAX == (unsigned)PIPE_TEX_REDUCTION_MAX);
   state.reduction_mode = (enum pipe_tex_reduction_mode)sampler->vk.reduction_mode;
            simple_mtx_lock(&device->queue.lock);
   sampler->texture_handle = (void *)(uintptr_t)device->queue.ctx->create_texture_handle(device->queue.ctx, NULL, &state);
            lp_jit_sampler_from_pipe(&sampler->desc.sampler, &state);
                        }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroySampler(
      VkDevice                                    _device,
   VkSampler                                   _sampler,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!_sampler)
            simple_mtx_lock(&device->queue.lock);
   device->queue.ctx->delete_texture_handle(device->queue.ctx, (uint64_t)(uintptr_t)sampler->texture_handle);
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
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreatePrivateDataSlotEXT(
      VkDevice                                    _device,
   const VkPrivateDataSlotCreateInfo*          pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   return vk_private_data_slot_create(&device->vk, pCreateInfo, pAllocator,
      }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyPrivateDataSlotEXT(
      VkDevice                                    _device,
   VkPrivateDataSlot                           privateDataSlot,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_SetPrivateDataEXT(
      VkDevice                                    _device,
   VkObjectType                                objectType,
   uint64_t                                    objectHandle,
   VkPrivateDataSlot                           privateDataSlot,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   return vk_object_base_set_private_data(&device->vk, objectType,
            }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPrivateDataEXT(
      VkDevice                                    _device,
   VkObjectType                                objectType,
   uint64_t                                    objectHandle,
   VkPrivateDataSlot                           privateDataSlot,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
   vk_object_base_get_private_data(&device->vk, objectType, objectHandle,
      }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_CreateIndirectCommandsLayoutNV(
      VkDevice                                    _device,
   const VkIndirectCommandsLayoutCreateInfoNV* pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
                     dlayout =
      vk_zalloc2(&device->vk.alloc, pAllocator, size, alignof(struct lvp_indirect_command_layout),
      if (!dlayout)
                     dlayout->stream_count = pCreateInfo->streamCount;
   dlayout->token_count = pCreateInfo->tokenCount;
   for (unsigned i = 0; i < pCreateInfo->streamCount; i++)
                  *pIndirectCommandsLayout = lvp_indirect_command_layout_to_handle(dlayout);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_DestroyIndirectCommandsLayoutNV(
      VkDevice                                    _device,
   VkIndirectCommandsLayoutNV                  indirectCommandsLayout,
      {
      LVP_FROM_HANDLE(lvp_device, device, _device);
            if (!layout)
            vk_object_base_finish(&layout->base);
      }
      enum vk_cmd_type
   lvp_nv_dgc_token_to_cmd_type(const VkIndirectCommandsLayoutTokenNV *token)
   {
      switch (token->tokenType) {
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV:
         case VK_INDIRECT_COMMANDS_TOKEN_TYPE_STATE_FLAGS_NV:
      if (token->indirectStateFlags & VK_INDIRECT_STATE_FLAG_FRONTFACE_BIT_NV) {
         }
   assert(!"unknown token type!");
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV:
         case VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV:
         case VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV:
   return VK_CMD_BIND_VERTEX_BUFFERS2;
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV:
         case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV:
         // only available if VK_EXT_mesh_shader is supported
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_MESH_TASKS_NV:
         // only available if VK_NV_mesh_shader is supported
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_TASKS_NV:
         default:
      }
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetGeneratedCommandsMemoryRequirementsNV(
      VkDevice                                    device,
   const VkGeneratedCommandsMemoryRequirementsInfoNV* pInfo,
      {
                        for (unsigned i = 0; i < dlayout->token_count; i++) {
      const VkIndirectCommandsLayoutTokenNV *token = &dlayout->tokens[i];
   UNUSED struct vk_cmd_queue_entry *cmd;
   enum vk_cmd_type type = lvp_nv_dgc_token_to_cmd_type(token);
            switch (token->tokenType) {
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_VERTEX_BUFFER_NV:
      size += sizeof(*cmd->u.bind_vertex_buffers.buffers);
   size += sizeof(*cmd->u.bind_vertex_buffers.offsets);
   size += sizeof(*cmd->u.bind_vertex_buffers2.sizes) + sizeof(*cmd->u.bind_vertex_buffers2.strides);
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_PUSH_CONSTANT_NV:
      size += token->pushconstantSize;
      case VK_INDIRECT_COMMANDS_TOKEN_TYPE_SHADER_GROUP_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_INDEX_BUFFER_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_STATE_FLAGS_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_INDEXED_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_TASKS_NV:
   case VK_INDIRECT_COMMANDS_TOKEN_TYPE_DRAW_MESH_TASKS_NV:
         default:
                              pMemoryRequirements->memoryRequirements.memoryTypeBits = 1;
   pMemoryRequirements->memoryRequirements.alignment = 4;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceExternalFenceProperties(
      VkPhysicalDevice                           physicalDevice,
   const VkPhysicalDeviceExternalFenceInfo    *pExternalFenceInfo,
      {
      pExternalFenceProperties->exportFromImportedHandleTypes = 0;
   pExternalFenceProperties->compatibleHandleTypes = 0;
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetPhysicalDeviceExternalSemaphoreProperties(
      VkPhysicalDevice                            physicalDevice,
   const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
      {
      pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
   pExternalSemaphoreProperties->compatibleHandleTypes = 0;
      }
      static const VkTimeDomainEXT lvp_time_domains[] = {
         VK_TIME_DOMAIN_DEVICE_EXT,
   };
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetPhysicalDeviceCalibrateableTimeDomainsEXT(
      VkPhysicalDevice physicalDevice,
   uint32_t *pTimeDomainCount,
      {
      int d;
   VK_OUTARRAY_MAKE_TYPED(VkTimeDomainEXT, out, pTimeDomains,
            for (d = 0; d < ARRAY_SIZE(lvp_time_domains); d++) {
      vk_outarray_append_typed(VkTimeDomainEXT, &out, i) {
                        }
      VKAPI_ATTR VkResult VKAPI_CALL lvp_GetCalibratedTimestampsEXT(
      VkDevice device,
   uint32_t timestampCount,
   const VkCalibratedTimestampInfoEXT *pTimestampInfos,
   uint64_t *pTimestamps,
      {
               uint64_t now = os_time_get_nano();
   for (unsigned i = 0; i < timestampCount; i++) {
         }
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetDeviceGroupPeerMemoryFeaturesKHR(
      VkDevice device,
   uint32_t heapIndex,
   uint32_t localDeviceIndex,
   uint32_t remoteDeviceIndex,
      {
         }
      VKAPI_ATTR void VKAPI_CALL lvp_SetDeviceMemoryPriorityEXT(
      VkDevice                                    _device,
   VkDeviceMemory                              _memory,
      {
      LVP_FROM_HANDLE(lvp_device_memory, mem, _memory);
      }
      VKAPI_ATTR void VKAPI_CALL lvp_GetRenderingAreaGranularityKHR(
      VkDevice                                    device,
   const VkRenderingAreaInfoKHR*               pRenderingAreaInfo,
      {
      VkExtent2D tile_size = {64, 64};
      }
