   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_physical_device.h"
      #include <stdio.h>
      #include "git_sha1.h"
   #include "util/mesa-sha1.h"
   #include "venus-protocol/vn_protocol_driver_device.h"
      #include "vn_android.h"
   #include "vn_instance.h"
      #define VN_EXTENSION_TABLE_INDEX(tbl, ext)                                   \
      ((const bool *)((const void *)(&(tbl)) +                                  \
               /** Add `elem` to the pNext chain of `head`. */
   #define VN_ADD_PNEXT(head, s_type, elem)                                     \
      do {                                                                      \
      (elem).sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##s_type;             \
   (elem).pNext = (head).pNext;                                           \
            /**
   * If the renderer supports the extension, add `elem` to the pNext chain of
   * `head`.
   */
   #define VN_ADD_PNEXT_EXT(head, s_type, elem, ext_cond)                       \
      do {                                                                      \
      if (ext_cond)                                                          \
            /**
   * Set member in core feature/property struct to value. (This provides visual
   * parity with VN_SET_CORE_FIELD).
   */
   #define VN_SET_CORE_VALUE(core_struct, member, val)                          \
      do {                                                                      \
               /** Copy member into core feature/property struct from extension struct. */
   #define VN_SET_CORE_FIELD(core_struct, member, ext_struct)                   \
            /**
   * Copy array member into core feature/property struct from extension struct.
   */
   #define VN_SET_CORE_ARRAY(core_struct, member, ext_struct)                   \
      do {                                                                      \
      memcpy((core_struct)->member, (ext_struct).member,                     \
            static void
   vn_physical_device_init_features(struct vn_physical_device *physical_dev)
   {
      const uint32_t renderer_version = physical_dev->renderer_version;
   struct vn_instance *instance = physical_dev->instance;
   const struct vk_device_extension_table *exts =
         VkPhysicalDeviceFeatures2 feats2 = {
         };
   struct {
      VkPhysicalDeviceFeatures vulkan_1_0;
   VkPhysicalDeviceVulkan11Features vulkan_1_1;
   VkPhysicalDeviceVulkan12Features vulkan_1_2;
            /* Vulkan 1.1 */
   VkPhysicalDevice16BitStorageFeatures _16bit_storage;
   VkPhysicalDeviceMultiviewFeatures multiview;
   VkPhysicalDeviceVariablePointersFeatures variable_pointers;
   VkPhysicalDeviceProtectedMemoryFeatures protected_memory;
   VkPhysicalDeviceSamplerYcbcrConversionFeatures sampler_ycbcr_conversion;
            /* Vulkan 1.2 */
   VkPhysicalDevice8BitStorageFeatures _8bit_storage;
   VkPhysicalDeviceShaderAtomicInt64Features shader_atomic_int64;
   VkPhysicalDeviceShaderFloat16Int8Features shader_float16_int8;
   VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing;
   VkPhysicalDeviceScalarBlockLayoutFeatures scalar_block_layout;
   VkPhysicalDeviceImagelessFramebufferFeatures imageless_framebuffer;
   VkPhysicalDeviceUniformBufferStandardLayoutFeatures
         VkPhysicalDeviceShaderSubgroupExtendedTypesFeatures
         VkPhysicalDeviceSeparateDepthStencilLayoutsFeatures
         VkPhysicalDeviceHostQueryResetFeatures host_query_reset;
   VkPhysicalDeviceTimelineSemaphoreFeatures timeline_semaphore;
   VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address;
            /* Vulkan 1.3 */
   VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering;
   VkPhysicalDeviceImageRobustnessFeatures image_robustness;
   VkPhysicalDeviceInlineUniformBlockFeatures inline_uniform_block;
   VkPhysicalDeviceMaintenance4Features maintenance4;
   VkPhysicalDevicePipelineCreationCacheControlFeatures
         VkPhysicalDevicePrivateDataFeatures private_data;
   VkPhysicalDeviceShaderDemoteToHelperInvocationFeatures
         VkPhysicalDeviceShaderIntegerDotProductFeatures
         VkPhysicalDeviceShaderTerminateInvocationFeatures
         VkPhysicalDeviceSynchronization2Features synchronization2;
   VkPhysicalDeviceSubgroupSizeControlFeatures subgroup_size_control;
   VkPhysicalDeviceTextureCompressionASTCHDRFeatures
         VkPhysicalDeviceZeroInitializeWorkgroupMemoryFeatures
            /* Vulkan 1.3: The extensions for the below structs were promoted, but
   * some struct members were omitted from
   * VkPhysicalDeviceVulkan13Features.
   */
   VkPhysicalDevice4444FormatsFeaturesEXT _4444_formats;
   VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extended_dynamic_state;
   VkPhysicalDeviceExtendedDynamicState2FeaturesEXT
         VkPhysicalDeviceTexelBufferAlignmentFeaturesEXT texel_buffer_alignment;
   VkPhysicalDeviceYcbcr2Plane444FormatsFeaturesEXT
            /* KHR */
            /* EXT */
   VkPhysicalDeviceBorderColorSwizzleFeaturesEXT border_color_swizzle;
   VkPhysicalDeviceColorWriteEnableFeaturesEXT color_write_enable;
   VkPhysicalDeviceConditionalRenderingFeaturesEXT conditional_rendering;
   VkPhysicalDeviceCustomBorderColorFeaturesEXT custom_border_color;
   VkPhysicalDeviceDepthClipControlFeaturesEXT depth_clip_control;
   VkPhysicalDeviceDepthClipEnableFeaturesEXT depth_clip_enable;
   VkPhysicalDeviceDynamicRenderingUnusedAttachmentsFeaturesEXT
         VkPhysicalDeviceFragmentShaderInterlockFeaturesEXT
         VkPhysicalDeviceGraphicsPipelineLibraryFeaturesEXT
         VkPhysicalDeviceImage2DViewOf3DFeaturesEXT image_2d_view_of_3d;
   VkPhysicalDeviceImageViewMinLodFeaturesEXT image_view_min_lod;
   VkPhysicalDeviceIndexTypeUint8FeaturesEXT index_type_uint8;
   VkPhysicalDeviceLineRasterizationFeaturesEXT line_rasterization;
   VkPhysicalDeviceMultiDrawFeaturesEXT multi_draw;
   VkPhysicalDeviceMutableDescriptorTypeFeaturesEXT mutable_descriptor_type;
   VkPhysicalDeviceNonSeamlessCubeMapFeaturesEXT non_seamless_cube_map;
   VkPhysicalDevicePrimitiveTopologyListRestartFeaturesEXT
         VkPhysicalDevicePrimitivesGeneratedQueryFeaturesEXT
         VkPhysicalDeviceProvokingVertexFeaturesEXT provoking_vertex;
   VkPhysicalDeviceRasterizationOrderAttachmentAccessFeaturesEXT
         VkPhysicalDeviceRobustness2FeaturesEXT robustness_2;
   VkPhysicalDeviceTransformFeedbackFeaturesEXT transform_feedback;
   VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT
         VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT
               /* Clear the struct so that all unqueried features will be VK_FALSE. */
                              if (renderer_version >= VK_API_VERSION_1_2) {
      VN_ADD_PNEXT(feats2, VULKAN_1_1_FEATURES, local_feats.vulkan_1_1);
      } else {
      /* Vulkan 1.1 */
   VN_ADD_PNEXT(feats2, 16BIT_STORAGE_FEATURES, local_feats._16bit_storage);
   VN_ADD_PNEXT(feats2, MULTIVIEW_FEATURES, local_feats.multiview);
   VN_ADD_PNEXT(feats2, PROTECTED_MEMORY_FEATURES, local_feats.protected_memory);
   VN_ADD_PNEXT(feats2, SAMPLER_YCBCR_CONVERSION_FEATURES, local_feats.sampler_ycbcr_conversion);
   VN_ADD_PNEXT(feats2, SHADER_DRAW_PARAMETERS_FEATURES, local_feats.shader_draw_parameters);
            /* Vulkan 1.2 */
   VN_ADD_PNEXT_EXT(feats2, 8BIT_STORAGE_FEATURES, local_feats._8bit_storage, exts->KHR_8bit_storage);
   VN_ADD_PNEXT_EXT(feats2, BUFFER_DEVICE_ADDRESS_FEATURES, local_feats.buffer_device_address, exts->KHR_buffer_device_address);
   VN_ADD_PNEXT_EXT(feats2, DESCRIPTOR_INDEXING_FEATURES, local_feats.descriptor_indexing, exts->EXT_descriptor_indexing);
   VN_ADD_PNEXT_EXT(feats2, HOST_QUERY_RESET_FEATURES, local_feats.host_query_reset, exts->EXT_host_query_reset);
   VN_ADD_PNEXT_EXT(feats2, IMAGELESS_FRAMEBUFFER_FEATURES, local_feats.imageless_framebuffer, exts->KHR_imageless_framebuffer);
   VN_ADD_PNEXT_EXT(feats2, SCALAR_BLOCK_LAYOUT_FEATURES, local_feats.scalar_block_layout, exts->EXT_scalar_block_layout);
   VN_ADD_PNEXT_EXT(feats2, SEPARATE_DEPTH_STENCIL_LAYOUTS_FEATURES, local_feats.separate_depth_stencil_layouts, exts->KHR_separate_depth_stencil_layouts);
   VN_ADD_PNEXT_EXT(feats2, SHADER_ATOMIC_INT64_FEATURES, local_feats.shader_atomic_int64, exts->KHR_shader_atomic_int64);
   VN_ADD_PNEXT_EXT(feats2, SHADER_FLOAT16_INT8_FEATURES, local_feats.shader_float16_int8, exts->KHR_shader_float16_int8);
   VN_ADD_PNEXT_EXT(feats2, SHADER_SUBGROUP_EXTENDED_TYPES_FEATURES, local_feats.shader_subgroup_extended_types, exts->KHR_shader_subgroup_extended_types);
   VN_ADD_PNEXT_EXT(feats2, TIMELINE_SEMAPHORE_FEATURES, local_feats.timeline_semaphore, exts->KHR_timeline_semaphore);
   VN_ADD_PNEXT_EXT(feats2, UNIFORM_BUFFER_STANDARD_LAYOUT_FEATURES, local_feats.uniform_buffer_standard_layout, exts->KHR_uniform_buffer_standard_layout);
               if (renderer_version >= VK_API_VERSION_1_3) {
         } else {
      VN_ADD_PNEXT_EXT(feats2, DYNAMIC_RENDERING_FEATURES, local_feats.dynamic_rendering, exts->KHR_dynamic_rendering);
   VN_ADD_PNEXT_EXT(feats2, IMAGE_ROBUSTNESS_FEATURES_EXT, local_feats.image_robustness, exts->EXT_image_robustness);
   VN_ADD_PNEXT_EXT(feats2, INLINE_UNIFORM_BLOCK_FEATURES, local_feats.inline_uniform_block, exts->EXT_inline_uniform_block);
   VN_ADD_PNEXT_EXT(feats2, MAINTENANCE_4_FEATURES, local_feats.maintenance4, exts->KHR_maintenance4);
   VN_ADD_PNEXT_EXT(feats2, PIPELINE_CREATION_CACHE_CONTROL_FEATURES, local_feats.pipeline_creation_cache_control, exts->EXT_pipeline_creation_cache_control);
   VN_ADD_PNEXT_EXT(feats2, PRIVATE_DATA_FEATURES, local_feats.private_data, exts->EXT_private_data);
   VN_ADD_PNEXT_EXT(feats2, SHADER_DEMOTE_TO_HELPER_INVOCATION_FEATURES, local_feats.shader_demote_to_helper_invocation, exts->EXT_shader_demote_to_helper_invocation);
   VN_ADD_PNEXT_EXT(feats2, SHADER_INTEGER_DOT_PRODUCT_FEATURES, local_feats.shader_integer_dot_product, exts->KHR_shader_integer_dot_product);
   VN_ADD_PNEXT_EXT(feats2, SHADER_TERMINATE_INVOCATION_FEATURES, local_feats.shader_terminate_invocation, exts->KHR_shader_terminate_invocation);
   VN_ADD_PNEXT_EXT(feats2, SUBGROUP_SIZE_CONTROL_FEATURES, local_feats.subgroup_size_control, exts->EXT_subgroup_size_control);
   VN_ADD_PNEXT_EXT(feats2, SYNCHRONIZATION_2_FEATURES, local_feats.synchronization2, exts->KHR_synchronization2);
   VN_ADD_PNEXT_EXT(feats2, TEXTURE_COMPRESSION_ASTC_HDR_FEATURES, local_feats.texture_compression_astc_hdr, exts->EXT_texture_compression_astc_hdr);
               /* Vulkan 1.3: The extensions for the below structs were promoted, but some
   * struct members were omitted from VkPhysicalDeviceVulkan13Features.
   */
   VN_ADD_PNEXT_EXT(feats2, 4444_FORMATS_FEATURES_EXT, local_feats._4444_formats, exts->EXT_4444_formats);
   VN_ADD_PNEXT_EXT(feats2, EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT, local_feats.extended_dynamic_state_2, exts->EXT_extended_dynamic_state2);
   VN_ADD_PNEXT_EXT(feats2, EXTENDED_DYNAMIC_STATE_FEATURES_EXT, local_feats.extended_dynamic_state, exts->EXT_extended_dynamic_state);
   VN_ADD_PNEXT_EXT(feats2, TEXEL_BUFFER_ALIGNMENT_FEATURES_EXT, local_feats.texel_buffer_alignment, exts->EXT_texel_buffer_alignment);
            /* KHR */
            /* EXT */
   VN_ADD_PNEXT_EXT(feats2, BORDER_COLOR_SWIZZLE_FEATURES_EXT, local_feats.border_color_swizzle, exts->EXT_border_color_swizzle);
   VN_ADD_PNEXT_EXT(feats2, COLOR_WRITE_ENABLE_FEATURES_EXT, local_feats.color_write_enable, exts->EXT_color_write_enable);
   VN_ADD_PNEXT_EXT(feats2, CONDITIONAL_RENDERING_FEATURES_EXT, local_feats.conditional_rendering, exts->EXT_conditional_rendering);
   VN_ADD_PNEXT_EXT(feats2, CUSTOM_BORDER_COLOR_FEATURES_EXT, local_feats.custom_border_color, exts->EXT_custom_border_color);
   VN_ADD_PNEXT_EXT(feats2, DEPTH_CLIP_CONTROL_FEATURES_EXT, local_feats.depth_clip_control, exts->EXT_depth_clip_control);
   VN_ADD_PNEXT_EXT(feats2, DEPTH_CLIP_ENABLE_FEATURES_EXT, local_feats.depth_clip_enable, exts->EXT_depth_clip_enable);
   VN_ADD_PNEXT_EXT(feats2, DYNAMIC_RENDERING_UNUSED_ATTACHMENTS_FEATURES_EXT, local_feats.dynamic_rendering_unused_attachments, exts->EXT_dynamic_rendering_unused_attachments);
   VN_ADD_PNEXT_EXT(feats2, FRAGMENT_SHADER_INTERLOCK_FEATURES_EXT, local_feats.fragment_shader_interlock, exts->EXT_fragment_shader_interlock);
   VN_ADD_PNEXT_EXT(feats2, GRAPHICS_PIPELINE_LIBRARY_FEATURES_EXT, local_feats.graphics_pipeline_library, exts->EXT_graphics_pipeline_library);
   VN_ADD_PNEXT_EXT(feats2, IMAGE_2D_VIEW_OF_3D_FEATURES_EXT, local_feats.image_2d_view_of_3d, exts->EXT_image_2d_view_of_3d);
   VN_ADD_PNEXT_EXT(feats2, IMAGE_VIEW_MIN_LOD_FEATURES_EXT, local_feats.image_view_min_lod, exts->EXT_image_view_min_lod);
   VN_ADD_PNEXT_EXT(feats2, INDEX_TYPE_UINT8_FEATURES_EXT, local_feats.index_type_uint8, exts->EXT_index_type_uint8);
   VN_ADD_PNEXT_EXT(feats2, LINE_RASTERIZATION_FEATURES_EXT, local_feats.line_rasterization, exts->EXT_line_rasterization);
   VN_ADD_PNEXT_EXT(feats2, MULTI_DRAW_FEATURES_EXT, local_feats.multi_draw, exts->EXT_multi_draw);
   VN_ADD_PNEXT_EXT(feats2, MUTABLE_DESCRIPTOR_TYPE_FEATURES_EXT, local_feats.mutable_descriptor_type, exts->EXT_mutable_descriptor_type || exts->VALVE_mutable_descriptor_type);
   VN_ADD_PNEXT_EXT(feats2, NON_SEAMLESS_CUBE_MAP_FEATURES_EXT, local_feats.non_seamless_cube_map, exts->EXT_non_seamless_cube_map);
   VN_ADD_PNEXT_EXT(feats2, PRIMITIVE_TOPOLOGY_LIST_RESTART_FEATURES_EXT, local_feats.primitive_topology_list_restart, exts->EXT_primitive_topology_list_restart);
   VN_ADD_PNEXT_EXT(feats2, PRIMITIVES_GENERATED_QUERY_FEATURES_EXT, local_feats.primitives_generated_query, exts->EXT_primitives_generated_query);
   VN_ADD_PNEXT_EXT(feats2, PROVOKING_VERTEX_FEATURES_EXT, local_feats.provoking_vertex, exts->EXT_provoking_vertex);
   VN_ADD_PNEXT_EXT(feats2, RASTERIZATION_ORDER_ATTACHMENT_ACCESS_FEATURES_EXT, local_feats.rasterization_order_attachment_access, exts->EXT_rasterization_order_attachment_access);
   VN_ADD_PNEXT_EXT(feats2, ROBUSTNESS_2_FEATURES_EXT, local_feats.robustness_2, exts->EXT_robustness2);
   VN_ADD_PNEXT_EXT(feats2, TRANSFORM_FEEDBACK_FEATURES_EXT, local_feats.transform_feedback, exts->EXT_transform_feedback);
   VN_ADD_PNEXT_EXT(feats2, VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT, local_feats.vertex_attribute_divisor, exts->EXT_vertex_attribute_divisor);
                     vn_call_vkGetPhysicalDeviceFeatures2(
            struct vk_features *feats = &physical_dev->base.base.supported_features;
            /* Enable features for extensions natively implemented in Venus driver.
   * See vn_physical_device_get_native_extensions.
   */
            /* To support sparse binding with feedback, we require sparse binding queue
   * families to  also support submiting feedback commands. Any queue
   * families that exclusively support sparse binding are filtered out. If a
   * device only supports sparse binding with exclusive queue families that
   * get filtered out then disable the feature.
   */
   if (physical_dev->sparse_binding_disabled) {
      VN_SET_CORE_VALUE(feats, sparseBinding, false);
   VN_SET_CORE_VALUE(feats, sparseResidencyBuffer, false);
   VN_SET_CORE_VALUE(feats, sparseResidencyImage2D, false);
   VN_SET_CORE_VALUE(feats, sparseResidencyImage3D, false);
   VN_SET_CORE_VALUE(feats, sparseResidency2Samples, false);
   VN_SET_CORE_VALUE(feats, sparseResidency4Samples, false);
   VN_SET_CORE_VALUE(feats, sparseResidency8Samples, false);
   VN_SET_CORE_VALUE(feats, sparseResidency16Samples, false);
         }
      static void
   vn_physical_device_init_uuids(struct vn_physical_device *physical_dev)
   {
      struct vn_physical_device_properties *props = &physical_dev->properties;
   struct VkPhysicalDeviceProperties *vk10_props = &props->vulkan_1_0;
   struct VkPhysicalDeviceVulkan11Properties *vk11_props = &props->vulkan_1_1;
   struct VkPhysicalDeviceVulkan12Properties *vk12_props = &props->vulkan_1_2;
   struct mesa_sha1 sha1_ctx;
                     _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, &vk10_props->pipelineCacheUUID,
                           _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, &vk10_props->vendorID,
         _mesa_sha1_update(&sha1_ctx, &vk10_props->deviceID,
                           _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, vk12_props->driverName,
         _mesa_sha1_update(&sha1_ctx, vk12_props->driverInfo,
                           memset(vk11_props->deviceLUID, 0, VK_LUID_SIZE);
   vk11_props->deviceNodeMask = 0;
      }
      static void
   vn_physical_device_init_properties(struct vn_physical_device *physical_dev)
   {
      const uint32_t renderer_version = physical_dev->renderer_version;
   struct vn_physical_device_properties *props = &physical_dev->properties;
   struct vn_instance *instance = physical_dev->instance;
   const struct vk_device_extension_table *exts =
         VkPhysicalDeviceProperties2 props2 = {
         };
   struct {
      /* Vulkan 1.1 */
   VkPhysicalDeviceIDProperties id;
   VkPhysicalDeviceSubgroupProperties subgroup;
   VkPhysicalDevicePointClippingProperties point_clipping;
   VkPhysicalDeviceMultiviewProperties multiview;
   VkPhysicalDeviceProtectedMemoryProperties protected_memory;
            /* Vulkan 1.2 */
   VkPhysicalDeviceDriverProperties driver;
   VkPhysicalDeviceFloatControlsProperties float_controls;
   VkPhysicalDeviceDescriptorIndexingProperties descriptor_indexing;
   VkPhysicalDeviceDepthStencilResolveProperties depth_stencil_resolve;
   VkPhysicalDeviceSamplerFilterMinmaxProperties sampler_filter_minmax;
            /* Vulkan 1.3 */
   VkPhysicalDeviceInlineUniformBlockProperties inline_uniform_block;
   VkPhysicalDeviceMaintenance4Properties maintenance4;
   VkPhysicalDeviceShaderIntegerDotProductProperties
         VkPhysicalDeviceSubgroupSizeControlProperties subgroup_size_control;
               /* Clear the structs so all unqueried properties will be well-defined. */
   memset(props, 0, sizeof(*props));
                              if (renderer_version >= VK_API_VERSION_1_2) {
      VN_ADD_PNEXT(props2, VULKAN_1_1_PROPERTIES, props->vulkan_1_1);
      } else {
      /* Vulkan 1.1 */
   VN_ADD_PNEXT(props2, ID_PROPERTIES, local_props.id);
   VN_ADD_PNEXT(props2, MAINTENANCE_3_PROPERTIES, local_props.maintenance_3);
   VN_ADD_PNEXT(props2, MULTIVIEW_PROPERTIES, local_props.multiview);
   VN_ADD_PNEXT(props2, POINT_CLIPPING_PROPERTIES, local_props.point_clipping);
   VN_ADD_PNEXT(props2, PROTECTED_MEMORY_PROPERTIES, local_props.protected_memory);
            /* Vulkan 1.2 */
   VN_ADD_PNEXT_EXT(props2, DEPTH_STENCIL_RESOLVE_PROPERTIES, local_props.depth_stencil_resolve, exts->KHR_depth_stencil_resolve);
   VN_ADD_PNEXT_EXT(props2, DESCRIPTOR_INDEXING_PROPERTIES, local_props.descriptor_indexing, exts->EXT_descriptor_indexing);
   VN_ADD_PNEXT_EXT(props2, DRIVER_PROPERTIES, local_props.driver, exts->KHR_driver_properties);
   VN_ADD_PNEXT_EXT(props2, FLOAT_CONTROLS_PROPERTIES, local_props.float_controls, exts->KHR_shader_float_controls);
   VN_ADD_PNEXT_EXT(props2, SAMPLER_FILTER_MINMAX_PROPERTIES, local_props.sampler_filter_minmax, exts->EXT_sampler_filter_minmax);
               if (renderer_version >= VK_API_VERSION_1_3) {
         } else {
      VN_ADD_PNEXT_EXT(props2, INLINE_UNIFORM_BLOCK_PROPERTIES, local_props.inline_uniform_block, exts->EXT_inline_uniform_block);
   VN_ADD_PNEXT_EXT(props2, MAINTENANCE_4_PROPERTIES, local_props.maintenance4, exts->KHR_maintenance4);
   VN_ADD_PNEXT_EXT(props2, SHADER_INTEGER_DOT_PRODUCT_PROPERTIES, local_props.shader_integer_dot_product, exts->KHR_shader_integer_dot_product);
   VN_ADD_PNEXT_EXT(props2, SUBGROUP_SIZE_CONTROL_PROPERTIES, local_props.subgroup_size_control, exts->EXT_subgroup_size_control);
               /* KHR */
            /* EXT */
   VN_ADD_PNEXT_EXT(props2, CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT, props->conservative_rasterization, exts->EXT_conservative_rasterization);
   VN_ADD_PNEXT_EXT(props2, CUSTOM_BORDER_COLOR_PROPERTIES_EXT, props->custom_border_color, exts->EXT_custom_border_color);
   VN_ADD_PNEXT_EXT(props2, GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT, props->graphics_pipeline_library, exts->EXT_graphics_pipeline_library);
   VN_ADD_PNEXT_EXT(props2, LINE_RASTERIZATION_PROPERTIES_EXT, props->line_rasterization, exts->EXT_line_rasterization);
   VN_ADD_PNEXT_EXT(props2, MULTI_DRAW_PROPERTIES_EXT, props->multi_draw, exts->EXT_multi_draw);
   VN_ADD_PNEXT_EXT(props2, PCI_BUS_INFO_PROPERTIES_EXT, props->pci_bus_info, exts->EXT_pci_bus_info);
   VN_ADD_PNEXT_EXT(props2, PROVOKING_VERTEX_PROPERTIES_EXT, props->provoking_vertex, exts->EXT_provoking_vertex);
   VN_ADD_PNEXT_EXT(props2, ROBUSTNESS_2_PROPERTIES_EXT, props->robustness_2, exts->EXT_robustness2);
   VN_ADD_PNEXT_EXT(props2, TRANSFORM_FEEDBACK_PROPERTIES_EXT, props->transform_feedback, exts->EXT_transform_feedback);
                     vn_call_vkGetPhysicalDeviceProperties2(
            VkPhysicalDeviceProperties *vk10_props = &props->vulkan_1_0;
   VkPhysicalDeviceVulkan11Properties *vk11_props = &props->vulkan_1_1;
   VkPhysicalDeviceVulkan12Properties *vk12_props = &props->vulkan_1_2;
                              /* See comment for sparse binding feature disable */
   if (physical_dev->sparse_binding_disabled) {
      VN_SET_CORE_VALUE(vk10_props, limits.sparseAddressSpaceSize, 0);
               if (renderer_version < VK_API_VERSION_1_2) {
      /* Vulkan 1.1 */
   VN_SET_CORE_ARRAY(vk11_props, deviceUUID, local_props.id);
   VN_SET_CORE_ARRAY(vk11_props, driverUUID, local_props.id);
   VN_SET_CORE_ARRAY(vk11_props, deviceLUID, local_props.id);
   VN_SET_CORE_FIELD(vk11_props, deviceNodeMask, local_props.id);
            /* Cannot use macro because names differ. */
   vk11_props->subgroupSize = local_props.subgroup.subgroupSize;
   vk11_props->subgroupSupportedStages = local_props.subgroup.supportedStages;
   vk11_props->subgroupSupportedOperations = local_props.subgroup.supportedOperations;
            VN_SET_CORE_FIELD(vk11_props, pointClippingBehavior, local_props.point_clipping);
   VN_SET_CORE_FIELD(vk11_props, maxMultiviewViewCount, local_props.multiview);
   VN_SET_CORE_FIELD(vk11_props, maxMultiviewInstanceIndex, local_props.multiview);
   VN_SET_CORE_FIELD(vk11_props, protectedNoFault, local_props.protected_memory);
   VN_SET_CORE_FIELD(vk11_props, maxPerSetDescriptors, local_props.maintenance_3);
            /* Vulkan 1.2 */
   if (exts->KHR_driver_properties) {
      VN_SET_CORE_FIELD(vk12_props, driverID, local_props.driver);
   VN_SET_CORE_ARRAY(vk12_props, driverName, local_props.driver);
   VN_SET_CORE_ARRAY(vk12_props, driverInfo, local_props.driver);
      }
   if (exts->KHR_shader_float_controls) {
      VN_SET_CORE_FIELD(vk12_props, denormBehaviorIndependence, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, roundingModeIndependence, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderSignedZeroInfNanPreserveFloat16, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderSignedZeroInfNanPreserveFloat32, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderSignedZeroInfNanPreserveFloat64, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormPreserveFloat16, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormPreserveFloat32, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormPreserveFloat64, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormFlushToZeroFloat16, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormFlushToZeroFloat32, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderDenormFlushToZeroFloat64, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderRoundingModeRTEFloat16, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderRoundingModeRTEFloat32, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderRoundingModeRTEFloat64, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderRoundingModeRTZFloat16, local_props.float_controls);
   VN_SET_CORE_FIELD(vk12_props, shaderRoundingModeRTZFloat32, local_props.float_controls);
      }
   if (exts->EXT_descriptor_indexing) {
      VN_SET_CORE_FIELD(vk12_props, maxUpdateAfterBindDescriptorsInAllPools, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, shaderUniformBufferArrayNonUniformIndexingNative, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, shaderSampledImageArrayNonUniformIndexingNative, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, shaderStorageBufferArrayNonUniformIndexingNative, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, shaderStorageImageArrayNonUniformIndexingNative, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, shaderInputAttachmentArrayNonUniformIndexingNative, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, robustBufferAccessUpdateAfterBind, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, quadDivergentImplicitLod, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindSamplers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindUniformBuffers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindStorageBuffers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindSampledImages, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindStorageImages, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageDescriptorUpdateAfterBindInputAttachments, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxPerStageUpdateAfterBindResources, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindSamplers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindUniformBuffers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindUniformBuffersDynamic, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindStorageBuffers, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindStorageBuffersDynamic, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindSampledImages, local_props.descriptor_indexing);
   VN_SET_CORE_FIELD(vk12_props, maxDescriptorSetUpdateAfterBindStorageImages, local_props.descriptor_indexing);
      }
   if (exts->KHR_depth_stencil_resolve) {
      VN_SET_CORE_FIELD(vk12_props, supportedDepthResolveModes, local_props.depth_stencil_resolve);
   VN_SET_CORE_FIELD(vk12_props, supportedStencilResolveModes, local_props.depth_stencil_resolve);
   VN_SET_CORE_FIELD(vk12_props, independentResolveNone, local_props.depth_stencil_resolve);
      }
   if (exts->EXT_sampler_filter_minmax) {
      VN_SET_CORE_FIELD(vk12_props, filterMinmaxSingleComponentFormats, local_props.sampler_filter_minmax);
      }
   if (exts->KHR_timeline_semaphore) {
                              if (renderer_version < VK_API_VERSION_1_3) {
      if (exts->EXT_subgroup_size_control) {
      VN_SET_CORE_FIELD(vk13_props, minSubgroupSize, local_props.subgroup_size_control);
   VN_SET_CORE_FIELD(vk13_props, maxSubgroupSize, local_props.subgroup_size_control);
   VN_SET_CORE_FIELD(vk13_props, maxComputeWorkgroupSubgroups, local_props.subgroup_size_control);
      }
   if (exts->EXT_inline_uniform_block) {
      VN_SET_CORE_FIELD(vk13_props, maxInlineUniformBlockSize, local_props.inline_uniform_block);
   VN_SET_CORE_FIELD(vk13_props, maxPerStageDescriptorInlineUniformBlocks, local_props.inline_uniform_block);
   VN_SET_CORE_FIELD(vk13_props, maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks, local_props.inline_uniform_block);
   VN_SET_CORE_FIELD(vk13_props, maxDescriptorSetInlineUniformBlocks, local_props.inline_uniform_block);
      }
   if (exts->KHR_shader_integer_dot_product) {
      VN_SET_CORE_FIELD(vk13_props, integerDotProduct8BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct8BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct8BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct4x8BitPackedUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct4x8BitPackedSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct4x8BitPackedMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct16BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct16BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct16BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct32BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct32BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct32BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct64BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct64BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProduct64BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating8BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating8BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating16BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating16BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating32BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating32BitSignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating64BitUnsignedAccelerated, local_props.shader_integer_dot_product);
   VN_SET_CORE_FIELD(vk13_props, integerDotProductAccumulatingSaturating64BitSignedAccelerated, local_props.shader_integer_dot_product);
      }
   if (exts->EXT_texel_buffer_alignment) {
      VN_SET_CORE_FIELD(vk13_props, storageTexelBufferOffsetAlignmentBytes, local_props.texel_buffer_alignment);
   VN_SET_CORE_FIELD(vk13_props, storageTexelBufferOffsetSingleTexelAlignment, local_props.texel_buffer_alignment);
   VN_SET_CORE_FIELD(vk13_props, uniformTexelBufferOffsetAlignmentBytes, local_props.texel_buffer_alignment);
      }
   if (exts->KHR_maintenance4) {
                              const uint32_t version_override = vk_get_version_override();
   if (version_override) {
         } else {
      /* cap the advertised api version */
   uint32_t ver = MIN3(vk10_props->apiVersion, VN_MAX_API_VERSION,
         if (VK_VERSION_PATCH(ver) > VK_VERSION_PATCH(vk10_props->apiVersion)) {
      ver = ver - VK_VERSION_PATCH(ver) +
               /* Clamp to 1.2 if we disabled VK_KHR_synchronization2 since it
   * is required for 1.3.
   * See vn_physical_device_get_passthrough_extensions()
   */
   if (!physical_dev->base.base.supported_extensions.KHR_synchronization2)
                        /* ANGLE relies on ARM proprietary driver version for workarounds */
   const char *engine_name = instance->base.base.app_info.engine_name;
   const bool forward_driver_version =
      vk12_props->driverID == VK_DRIVER_ID_ARM_PROPRIETARY && engine_name &&
      if (!forward_driver_version)
            char device_name[VK_MAX_PHYSICAL_DEVICE_NAME_SIZE];
   int device_name_len =
      snprintf(device_name, sizeof(device_name), "Virtio-GPU Venus (%s)",
      if (device_name_len >= VK_MAX_PHYSICAL_DEVICE_NAME_SIZE) {
      memcpy(device_name + VK_MAX_PHYSICAL_DEVICE_NAME_SIZE - 5, "...)", 4);
      }
            /* store renderer VkDriverId for implementation specific workarounds */
   physical_dev->renderer_driver_id = vk12_props->driverID;
            snprintf(vk12_props->driverName, sizeof(vk12_props->driverName), "venus");
   snprintf(vk12_props->driverInfo, sizeof(vk12_props->driverInfo),
            VN_SET_CORE_VALUE(vk12_props, conformanceVersion.major, 1);
   VN_SET_CORE_VALUE(vk12_props, conformanceVersion.minor, 3);
   VN_SET_CORE_VALUE(vk12_props, conformanceVersion.subminor, 0);
               }
      static VkResult
   vn_physical_device_init_queue_family_properties(
         {
      struct vn_instance *instance = physical_dev->instance;
   const VkAllocationCallbacks *alloc = &instance->base.base.alloc;
            vn_call_vkGetPhysicalDeviceQueueFamilyProperties2(
            VkQueueFamilyProperties2 *props =
      vk_alloc(alloc, sizeof(*props) * count, VN_DEFAULT_ALIGN,
      if (!props)
            for (uint32_t i = 0; i < count; i++) {
      props[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
      }
   vn_call_vkGetPhysicalDeviceQueueFamilyProperties2(
            /* Filter out queue families that exclusively support sparse binding as
   * we need additional support for submitting feedback commands
   */
   uint32_t sparse_count = 0;
   uint32_t non_sparse_only_count = 0;
   for (uint32_t i = 0; i < count; i++) {
      if (props[i].queueFamilyProperties.queueFlags &
      ~VK_QUEUE_SPARSE_BINDING_BIT) {
   props[non_sparse_only_count++].queueFamilyProperties =
      }
   if (props[i].queueFamilyProperties.queueFlags &
      VK_QUEUE_SPARSE_BINDING_BIT) {
                  if (VN_DEBUG(NO_SPARSE) ||
      (sparse_count && non_sparse_only_count + sparse_count == count))
         physical_dev->queue_family_properties = props;
               }
      static void
   vn_physical_device_init_memory_properties(
         {
      struct vn_instance *instance = physical_dev->instance;
   VkPhysicalDeviceMemoryProperties2 props2 = {
         };
   vn_call_vkGetPhysicalDeviceMemoryProperties2(
                     /* Kernel makes every mapping coherent. If a memory type is truly
   * incoherent, it's better to remove the host-visible flag than silently
   * making it coherent. However, for app compatibility purpose, when
   * coherent-cached memory type is unavailable, we emulate the first cached
   * memory type with the first coherent memory type.
   */
   uint32_t coherent_uncached = VK_MAX_MEMORY_TYPES;
   uint32_t incoherent_cached = VK_MAX_MEMORY_TYPES;
   VkPhysicalDeviceMemoryProperties *props = &physical_dev->memory_properties;
   for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
      const VkMemoryPropertyFlags flags = props->memoryTypes[i].propertyFlags;
   const bool coherent = flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   const bool cached = flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
   if (coherent && cached) {
      coherent_uncached = VK_MAX_MEMORY_TYPES;
   incoherent_cached = VK_MAX_MEMORY_TYPES;
      } else if (coherent && coherent_uncached == VK_MAX_MEMORY_TYPES) {
         } else if (cached && incoherent_cached == VK_MAX_MEMORY_TYPES) {
                     for (uint32_t i = 0; i < props->memoryTypeCount; i++) {
      VkMemoryType *type = &props->memoryTypes[i];
   if (i == incoherent_cached) {
      /* Only get here if no coherent+cached type is available, and the
   * spec guarantees that there is at least one coherent type, so it
   * must be coherent+uncached, hence the index is always valid.
   */
   assert(coherent_uncached < props->memoryTypeCount);
      } else if (!(type->propertyFlags &
            type->propertyFlags &= ~(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  physical_dev->coherent_uncached = coherent_uncached;
      }
      static void
   vn_physical_device_init_external_memory(
         {
      /* When a renderer VkDeviceMemory is exportable, we can create a
   * vn_renderer_bo from it. The vn_renderer_bo can be freely exported as an
   * opaque fd or a dma-buf.
   *
   * When an external memory can be imported as a vn_renderer_bo, that bo
   * might be imported as a renderer side VkDeviceMemory.
   *
   * However, to know if a rendender VkDeviceMemory is exportable or if a bo
   * can be imported as a renderer VkDeviceMemory. We have to start from
   * physical device external image and external buffer properties queries,
   * which requires to know the renderer supported external handle types. For
   * such info, we can reliably retrieve from the external memory extensions
   * advertised by the renderer.
   *
   * We require VK_EXT_external_memory_dma_buf to expose driver side external
   * memory support for a renderer running on Linux. As a comparison, when
   * the renderer runs on Windows, VK_KHR_external_memory_win32 might be
   * required for the same.
   *
   * For vtest, the protocol does not support external memory import. So we
   * only mask out the importable bit so that wsi over vtest can be supported.
   */
   if (physical_dev->renderer_extensions.EXT_external_memory_dma_buf) {
      physical_dev->external_memory.renderer_handle_type =
      #ifdef ANDROID
         physical_dev->external_memory.supported_handle_types |=
   #else  /* ANDROID */
         physical_dev->external_memory.supported_handle_types =
         #endif /* ANDROID */
         }
      static void
   vn_physical_device_init_external_fence_handles(
         {
      /* The current code manipulates the host-side VkFence directly.
   * vkWaitForFences is translated to repeated vkGetFenceStatus.
   *
   * External fence is not possible currently.  Instead, we cheat by
   * translating vkGetFenceFdKHR to an empty renderer submission for the
   * out fence, along with a venus protocol command to fix renderer side
   * fence payload.
   *
   * We would like to create a vn_renderer_sync from a host-side VkFence,
   * similar to how a vn_renderer_bo is created from a host-side
   * VkDeviceMemory.  That would require kernel support and tons of works on
   * the host side.  If we had that, and we kept both the vn_renderer_sync
   * and the host-side VkFence in sync, we would have the freedom to use
   * either of them depending on the occasions, and support external fences
   * and idle waiting.
   */
   if (physical_dev->renderer_extensions.KHR_external_fence_fd) {
      const VkPhysicalDeviceExternalFenceInfo info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_FENCE_INFO,
      };
   VkExternalFenceProperties props = {
         };
   vn_call_vkGetPhysicalDeviceExternalFenceProperties(
                  physical_dev->renderer_sync_fd.fence_exportable =
      props.externalFenceFeatures &
                     if (physical_dev->instance->renderer->info.has_external_sync) {
      physical_dev->external_fence_handles =
         }
      static void
   vn_physical_device_init_external_semaphore_handles(
         {
      /* The current code manipulates the host-side VkSemaphore directly.  It
   * works very well for binary semaphores because there is no CPU operation.
   * But for timeline semaphores, the situation is similar to that of fences.
   * vkWaitSemaphores is translated to repeated vkGetSemaphoreCounterValue.
   *
   * External semaphore is not possible currently.  Instead, we cheat when
   * the semaphore is binary and the handle type is sync file. We do an empty
   * renderer submission for the out fence, along with a venus protocol
   * command to fix renderer side semaphore payload.
   *
   * We would like to create a vn_renderer_sync from a host-side VkSemaphore,
   * similar to how a vn_renderer_bo is created from a host-side
   * VkDeviceMemory.  The reasoning is the same as that for fences.
   * Additionally, we would like the sync file exported from the
   * vn_renderer_sync to carry the necessary information to identify the
   * host-side VkSemaphore.  That would allow the consumers to wait on the
   * host side rather than the guest side.
   */
   if (physical_dev->renderer_extensions.KHR_external_semaphore_fd) {
      const VkPhysicalDeviceExternalSemaphoreInfo info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
      };
   VkExternalSemaphoreProperties props = {
         };
   vn_call_vkGetPhysicalDeviceExternalSemaphoreProperties(
                  physical_dev->renderer_sync_fd.semaphore_exportable =
      props.externalSemaphoreFeatures &
      physical_dev->renderer_sync_fd.semaphore_importable =
      props.externalSemaphoreFeatures &
            physical_dev->external_binary_semaphore_handles = 0;
            if (physical_dev->instance->renderer->info.has_external_sync) {
      physical_dev->external_binary_semaphore_handles =
         }
      static inline bool
   vn_physical_device_get_external_memory_support(
         {
      if (!physical_dev->external_memory.renderer_handle_type)
            /* see vn_physical_device_init_external_memory */
   if (physical_dev->external_memory.renderer_handle_type ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT) {
   const struct vk_device_extension_table *renderer_exts =
         return renderer_exts->EXT_image_drm_format_modifier &&
               /* expand support once the renderer can run on non-Linux platforms */
      }
      static void
   vn_physical_device_get_native_extensions(
      const struct vn_physical_device *physical_dev,
      {
               if (physical_dev->instance->renderer->info.has_external_sync &&
      physical_dev->renderer_sync_fd.fence_exportable)
         if (physical_dev->instance->renderer->info.has_external_sync &&
      physical_dev->renderer_sync_fd.semaphore_importable &&
   physical_dev->renderer_sync_fd.semaphore_exportable)
         const bool can_external_mem =
            #ifdef ANDROID
                  /* For wsi, we require renderer:
   * - semaphore sync fd import for queue submission to skip scrubbing the
   *   wsi wait semaphores.
   * - fence sync fd export for QueueSignalReleaseImageANDROID to export a
   *   sync fd.
   *
   * TODO: relax these requirements by:
   * - properly scrubbing wsi wait semaphores
   * - not creating external fence but exporting sync fd directly
   */
   if (physical_dev->renderer_sync_fd.semaphore_importable &&
         #else  /* ANDROID */
         exts->KHR_external_memory_fd = true;
   #endif /* ANDROID */
            #ifdef VN_USE_WSI_PLATFORM
      if (can_external_mem &&
      physical_dev->renderer_sync_fd.semaphore_importable) {
   exts->KHR_incremental_present = true;
   exts->KHR_swapchain = true;
               /* VK_EXT_pci_bus_info is required by common wsi to decide whether native
   * image or prime blit is used. Meanwhile, venus must stay on native image
   * path for proper fencing.
   * - For virtgpu, VK_EXT_pci_bus_info is natively supported.
   * - For vtest, pci bus info must be queried from the renderer side physical
   *   device to be compared against the render node opened by common wsi.
   */
   exts->EXT_pci_bus_info =
      physical_dev->instance->renderer->info.pci.has_bus_info ||
   #endif
         exts->EXT_physical_device_drm = true;
   /* use common implementation */
   exts->EXT_tooling_info = true;
      }
      static void
   vn_physical_device_get_passthrough_extensions(
      const struct vn_physical_device *physical_dev,
      {
      *exts = (struct vk_device_extension_table){
      /* promoted to VK_VERSION_1_1 */
   .KHR_16bit_storage = true,
   .KHR_bind_memory2 = true,
   .KHR_dedicated_allocation = true,
   .KHR_descriptor_update_template = true,
   .KHR_device_group = true,
   .KHR_external_fence = true,
   .KHR_external_memory = true,
   .KHR_external_semaphore = true,
   .KHR_get_memory_requirements2 = true,
   .KHR_maintenance1 = true,
   .KHR_maintenance2 = true,
   .KHR_maintenance3 = true,
   .KHR_multiview = true,
   .KHR_relaxed_block_layout = true,
   .KHR_sampler_ycbcr_conversion = true,
   .KHR_shader_draw_parameters = true,
   .KHR_storage_buffer_storage_class = true,
            /* promoted to VK_VERSION_1_2 */
   .KHR_8bit_storage = true,
   .KHR_buffer_device_address = true,
   .KHR_create_renderpass2 = true,
   .KHR_depth_stencil_resolve = true,
   .KHR_draw_indirect_count = true,
   .KHR_driver_properties = true,
   .KHR_image_format_list = true,
   .KHR_imageless_framebuffer = true,
   .KHR_sampler_mirror_clamp_to_edge = true,
   .KHR_separate_depth_stencil_layouts = true,
   .KHR_shader_atomic_int64 = true,
   .KHR_shader_float16_int8 = true,
   .KHR_shader_float_controls = true,
   .KHR_shader_subgroup_extended_types = true,
   .KHR_spirv_1_4 = true,
   .KHR_timeline_semaphore = true,
   .KHR_uniform_buffer_standard_layout = true,
   .KHR_vulkan_memory_model = true,
   .EXT_descriptor_indexing = true,
   .EXT_host_query_reset = true,
   .EXT_sampler_filter_minmax = true,
   .EXT_scalar_block_layout = true,
   .EXT_separate_stencil_usage = true,
            /* promoted to VK_VERSION_1_3 */
   .KHR_copy_commands2 = true,
   .KHR_dynamic_rendering = true,
   .KHR_format_feature_flags2 = true,
   .KHR_maintenance4 = true,
   .KHR_shader_integer_dot_product = true,
   .KHR_shader_non_semantic_info = true,
   .KHR_shader_terminate_invocation = true,
   /* Our implementation requires semaphore sync fd import
   * for VK_KHR_synchronization2.
   */
   .KHR_synchronization2 =
         .KHR_zero_initialize_workgroup_memory = true,
   .EXT_4444_formats = true,
   .EXT_extended_dynamic_state = true,
   .EXT_extended_dynamic_state2 = true,
   .EXT_image_robustness = true,
   .EXT_inline_uniform_block = true,
   .EXT_pipeline_creation_cache_control = true,
   /* TODO(VK_EXT_pipeline_creation_feedback): The native implementation
   * invalidates all feedback. Teach the venus protocol to receive valid
   * feedback from renderer.
   *
   * Even though we implement this natively, we still require host driver
   * support to avoid invalid usage in the renderer, because we (the guest
   * driver) do not scrub the extension bits from the
   * VkGraphicsPipelineCreateInfo pNext chain.  The host driver still
   * writes feedback into VkPipelineCreationFeedback, which is harmless,
   * but the renderer does not send the returned feedback to us due to
   * protocol deficiencies.
   */
   .EXT_pipeline_creation_feedback = true,
   .EXT_shader_demote_to_helper_invocation = true,
   .EXT_subgroup_size_control = true,
   .EXT_texel_buffer_alignment = true,
   .EXT_texture_compression_astc_hdr = true,
            /* KHR */
   .KHR_pipeline_library = true,
   .KHR_push_descriptor = true,
            /* EXT */
   .EXT_border_color_swizzle = true,
   .EXT_calibrated_timestamps = true,
   .EXT_color_write_enable = true,
   .EXT_conditional_rendering = true,
   .EXT_conservative_rasterization = true,
   .EXT_custom_border_color = true,
   .EXT_depth_clip_control = true,
   .EXT_depth_clip_enable = true,
   .EXT_dynamic_rendering_unused_attachments = true,
   .EXT_fragment_shader_interlock = true,
   .EXT_graphics_pipeline_library = VN_DEBUG(GPL),
   .EXT_image_2d_view_of_3d = true,
   .EXT_image_drm_format_modifier = true,
   .EXT_image_view_min_lod = true,
   .EXT_index_type_uint8 = true,
   .EXT_line_rasterization = true,
   .EXT_load_store_op_none = true,
   /* TODO: re-enable after generic app compat issues are resolved */
   .EXT_memory_budget = false,
   .EXT_multi_draw = true,
   .EXT_mutable_descriptor_type = true,
   .EXT_non_seamless_cube_map = true,
   .EXT_primitive_topology_list_restart = true,
   .EXT_primitives_generated_query = true,
   /* TODO(VK_EXT_private_data): Support natively.
   *
   * We support this extension with a hybrid native/passthrough model
   * until we teach venus how to do deep surgery on pNext
   * chains to (a) remove VkDevicePrivateDataCreateInfoEXT, (b) remove Vk
   * VkPhysicalDevicePrivateDataFeaturesEXT, and (c) modify its bits in
   * VkPhysicalDeviceVulkan13Features.
   *
   * For now, we implement the extension functions natively by using
   * Mesa's commong implementation. We passthrough
   * VkDevicePrivateDataCreateInfoEXT to the renderer, which is harmless.
   * We passthrough the extension enablement and feature bits to the
   * renderer because otherwise VkDevicePrivateDataCreateInfoEXT would
   * cause invalid usage in the renderer. Therefore, even though we
   * implement the extension natively, we expose the extension only if the
   * renderer supports it too.
   */
   .EXT_private_data = true,
   .EXT_provoking_vertex = true,
   .EXT_queue_family_foreign = true,
   .EXT_rasterization_order_attachment_access = true,
   .EXT_robustness2 = true,
   .EXT_shader_stencil_export = true,
   .EXT_shader_subgroup_ballot = true,
   .EXT_transform_feedback = true,
   .EXT_vertex_attribute_divisor = true,
            /* vendor */
         }
      static void
   vn_physical_device_init_supported_extensions(
         {
      struct vk_device_extension_table native;
   struct vk_device_extension_table passthrough;
   vn_physical_device_get_native_extensions(physical_dev, &native);
            for (uint32_t i = 0; i < VK_DEVICE_EXTENSION_COUNT; i++) {
         #ifdef ANDROID
         if (!vk_android_allowed_device_extensions.extensions[i])
   #endif
            if (native.extensions[i]) {
      physical_dev->base.base.supported_extensions.extensions[i] = true;
      } else if (passthrough.extensions[i] &&
            physical_dev->base.base.supported_extensions.extensions[i] = true;
   physical_dev->extension_spec_versions[i] = MIN2(
                  /* override VK_ANDROID_native_buffer spec version */
   if (native.ANDROID_native_buffer) {
      const uint32_t index =
         physical_dev->extension_spec_versions[index] =
         }
      static VkResult
   vn_physical_device_init_renderer_extensions(
         {
      struct vn_instance *instance = physical_dev->instance;
            /* get renderer extensions */
   uint32_t count;
   VkResult result = vn_call_vkEnumerateDeviceExtensionProperties(
      instance, vn_physical_device_to_handle(physical_dev), NULL, &count,
      if (result != VK_SUCCESS)
            VkExtensionProperties *exts = NULL;
   if (count) {
      exts = vk_alloc(alloc, sizeof(*exts) * count, VN_DEFAULT_ALIGN,
         if (!exts)
            result = vn_call_vkEnumerateDeviceExtensionProperties(
      instance, vn_physical_device_to_handle(physical_dev), NULL, &count,
      if (result < VK_SUCCESS) {
      vk_free(alloc, exts);
                  physical_dev->extension_spec_versions =
      vk_zalloc(alloc,
            sizeof(*physical_dev->extension_spec_versions) *
   if (!physical_dev->extension_spec_versions) {
      vk_free(alloc, exts);
               for (uint32_t i = 0; i < VK_DEVICE_EXTENSION_COUNT; i++) {
      const VkExtensionProperties *props = &vk_device_extensions[i];
   for (uint32_t j = 0; j < count; j++) {
                     /* check encoder support */
   const uint32_t enc_ext_spec_version =
                        physical_dev->renderer_extensions.extensions[i] = true;
                                             }
      static VkResult
   vn_physical_device_init_renderer_version(
         {
               /*
   * We either check and enable VK_KHR_get_physical_device_properties2, or we
   * must use vkGetPhysicalDeviceProperties to get the device-level version.
   */
   VkPhysicalDeviceProperties props;
   vn_call_vkGetPhysicalDeviceProperties(
         if (props.apiVersion < VN_MIN_RENDERER_VERSION) {
      if (VN_DEBUG(INIT)) {
      vn_log(instance, "%s has unsupported renderer device version %d.%d",
            }
               /* device version for internal use is capped */
   physical_dev->renderer_version =
      MIN3(props.apiVersion, instance->renderer_api_version,
            }
      static VkResult
   vn_physical_device_init(struct vn_physical_device *physical_dev)
   {
      struct vn_instance *instance = physical_dev->instance;
   const VkAllocationCallbacks *alloc = &instance->base.base.alloc;
            result = vn_physical_device_init_renderer_extensions(physical_dev);
   if (result != VK_SUCCESS)
            vn_physical_device_init_external_memory(physical_dev);
   vn_physical_device_init_external_fence_handles(physical_dev);
                     result = vn_physical_device_init_queue_family_properties(physical_dev);
   if (result != VK_SUCCESS)
            /* TODO query all caps with minimal round trips */
   vn_physical_device_init_features(physical_dev);
                     result = vn_wsi_init(physical_dev);
   if (result != VK_SUCCESS)
            simple_mtx_init(&physical_dev->format_update_mutex, mtx_plain);
   util_sparse_array_init(&physical_dev->format_properties,
                  fail:
      vk_free(alloc, physical_dev->extension_spec_versions);
   vk_free(alloc, physical_dev->queue_family_properties);
      }
      void
   vn_physical_device_fini(struct vn_physical_device *physical_dev)
   {
      struct vn_instance *instance = physical_dev->instance;
            simple_mtx_destroy(&physical_dev->format_update_mutex);
            vn_wsi_fini(physical_dev);
   vk_free(alloc, physical_dev->extension_spec_versions);
               }
      static struct vn_physical_device *
   find_physical_device(struct vn_physical_device *physical_devs,
               {
      for (uint32_t i = 0; i < count; i++) {
      if (physical_devs[i].base.id == id)
      }
      }
      static VkResult
   vn_instance_enumerate_physical_device_groups_locked(
      struct vn_instance *instance,
   struct vn_physical_device *physical_devs,
      {
      VkInstance instance_handle = vn_instance_to_handle(instance);
   const VkAllocationCallbacks *alloc = &instance->base.base.alloc;
            uint32_t count;
   result = vn_call_vkEnumeratePhysicalDeviceGroups(instance, instance_handle,
         if (result != VK_SUCCESS)
            VkPhysicalDeviceGroupProperties *groups =
      vk_alloc(alloc, sizeof(*groups) * count, VN_DEFAULT_ALIGN,
      if (!groups)
            /* VkPhysicalDeviceGroupProperties::physicalDevices is treated as an input
   * by the encoder.  Each VkPhysicalDevice must point to a valid object.
   * Each object must have id 0 as well, which is interpreted as a query by
   * the renderer.
   */
   struct vn_physical_device_base *temp_objs =
      vk_zalloc(alloc, sizeof(*temp_objs) * VK_MAX_DEVICE_GROUP_SIZE * count,
      if (!temp_objs) {
      vk_free(alloc, groups);
               for (uint32_t i = 0; i < count; i++) {
      VkPhysicalDeviceGroupProperties *group = &groups[i];
   group->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_GROUP_PROPERTIES;
   group->pNext = NULL;
   for (uint32_t j = 0; j < VK_MAX_DEVICE_GROUP_SIZE; j++) {
      struct vn_physical_device_base *temp_obj =
         temp_obj->base.base.type = VK_OBJECT_TYPE_PHYSICAL_DEVICE;
                  result = vn_call_vkEnumeratePhysicalDeviceGroups(instance, instance_handle,
         if (result != VK_SUCCESS) {
      vk_free(alloc, groups);
   vk_free(alloc, temp_objs);
               /* fix VkPhysicalDeviceGroupProperties::physicalDevices to point to
   * physical_devs and discard unsupported ones
   */
   uint32_t supported_count = 0;
   for (uint32_t i = 0; i < count; i++) {
               uint32_t group_physical_dev_count = 0;
   for (uint32_t j = 0; j < group->physicalDeviceCount; j++) {
      struct vn_physical_device_base *temp_obj =
         struct vn_physical_device *physical_dev = find_physical_device(
                        group->physicalDevices[group_physical_dev_count++] =
               group->physicalDeviceCount = group_physical_dev_count;
   if (!group->physicalDeviceCount)
            if (supported_count < i)
                     count = supported_count;
                     instance->physical_device.groups = groups;
               }
      static VkResult
   enumerate_physical_devices(struct vn_instance *instance,
               {
      const VkAllocationCallbacks *alloc = &instance->base.base.alloc;
   struct vn_physical_device *physical_devs = NULL;
   VkPhysicalDevice *handles = NULL;
            uint32_t count = 0;
   result = vn_call_vkEnumeratePhysicalDevices(
         if (result != VK_SUCCESS || !count)
            physical_devs =
      vk_zalloc(alloc, sizeof(*physical_devs) * count, VN_DEFAULT_ALIGN,
      if (!physical_devs)
            handles = vk_alloc(alloc, sizeof(*handles) * count, VN_DEFAULT_ALIGN,
         if (!handles) {
      vk_free(alloc, physical_devs);
               for (uint32_t i = 0; i < count; i++) {
               struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
         vk_physical_device_dispatch_table_from_entrypoints(
         result = vn_physical_device_base_init(
         if (result != VK_SUCCESS) {
      count = i;
                                    result = vn_call_vkEnumeratePhysicalDevices(
         if (result != VK_SUCCESS)
            vk_free(alloc, handles);
   *out_physical_devs = physical_devs;
                  fail:
      for (uint32_t i = 0; i < count; i++)
         vk_free(alloc, physical_devs);
   vk_free(alloc, handles);
      }
      static uint32_t
   filter_physical_devices(struct vn_physical_device *physical_devs,
         {
      uint32_t supported_count = 0;
   for (uint32_t i = 0; i < count; i++) {
               /* init renderer version and discard unsupported devices */
   VkResult result =
         if (result != VK_SUCCESS) {
      vn_physical_device_base_fini(&physical_dev->base);
               if (supported_count < i)
                        }
      static VkResult
   vn_instance_enumerate_physical_devices_and_groups(struct vn_instance *instance)
   {
      const VkAllocationCallbacks *alloc = &instance->base.base.alloc;
   struct vn_physical_device *physical_devs = NULL;
   uint32_t count = 0;
                     if (instance->physical_device.initialized)
                  result = enumerate_physical_devices(instance, &physical_devs, &count);
   if (result != VK_SUCCESS)
            count = filter_physical_devices(physical_devs, count);
   if (!count) {
      vk_free(alloc, physical_devs);
               /* fully initialize physical devices */
   for (uint32_t i = 0; i < count; i++) {
               result = vn_physical_device_init(physical_dev);
   if (result != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; j++)
         for (uint32_t j = i; j < count; j++)
         vk_free(alloc, physical_devs);
                  result = vn_instance_enumerate_physical_device_groups_locked(
         if (result != VK_SUCCESS) {
      for (uint32_t i = 0; i < count; i++)
         vk_free(alloc, physical_devs);
               instance->physical_device.devices = physical_devs;
         unlock:
      mtx_unlock(&instance->physical_device.mutex);
      }
      /* physical device commands */
      VkResult
   vn_EnumeratePhysicalDevices(VkInstance _instance,
               {
               VkResult result =
         if (result != VK_SUCCESS)
            VK_OUTARRAY_MAKE_TYPED(VkPhysicalDevice, out, pPhysicalDevices,
         for (uint32_t i = 0; i < instance->physical_device.device_count; i++) {
      vk_outarray_append_typed(VkPhysicalDevice, &out, physical_dev) {
      *physical_dev = vn_physical_device_to_handle(
                     }
      VkResult
   vn_EnumeratePhysicalDeviceGroups(
      VkInstance _instance,
   uint32_t *pPhysicalDeviceGroupCount,
      {
               VkResult result =
         if (result != VK_SUCCESS)
            VK_OUTARRAY_MAKE_TYPED(VkPhysicalDeviceGroupProperties, out,
               for (uint32_t i = 0; i < instance->physical_device.group_count; i++) {
      vk_outarray_append_typed(VkPhysicalDeviceGroupProperties, &out, props) {
                        }
      VkResult
   vn_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                     {
      struct vn_physical_device *physical_dev =
            if (pLayerName)
            VK_OUTARRAY_MAKE_TYPED(VkExtensionProperties, out, pProperties,
         for (uint32_t i = 0; i < VK_DEVICE_EXTENSION_COUNT; i++) {
      if (physical_dev->base.base.supported_extensions.extensions[i]) {
      vk_outarray_append_typed(VkExtensionProperties, &out, prop) {
      *prop = vk_device_extensions[i];
                        }
      VkResult
   vn_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
               {
      *pPropertyCount = 0;
      }
      static struct vn_format_properties_entry *
   vn_physical_device_get_format_properties(
         {
         }
      static void
   vn_physical_device_add_format_properties(
      struct vn_physical_device *physical_dev,
   struct vn_format_properties_entry *entry,
      {
      simple_mtx_lock(&physical_dev->format_update_mutex);
   if (!entry->valid) {
      entry->properties = *props;
      }
      }
      void
   vn_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
         {
      struct vn_physical_device *physical_dev =
         const struct vn_physical_device_properties *in_props =
                     vk_foreach_struct(out, pProperties->pNext) {
      if (vk_get_physical_device_core_1_1_property_ext(out,
                  if (vk_get_physical_device_core_1_2_property_ext(out,
                  if (vk_get_physical_device_core_1_3_property_ext(out,
                  /* Cast to avoid warnings for values outside VkStructureType. */
      #define CASE(stype, member)                                                  \
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##stype:                           \
      vk_copy_struct_guts(out, (VkBaseInStructure *)&in_props->member,       \
                           /* KHR */
            /* EXT */
   CASE(CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT, conservative_rasterization);
   CASE(CUSTOM_BORDER_COLOR_PROPERTIES_EXT, custom_border_color);
   CASE(GRAPHICS_PIPELINE_LIBRARY_PROPERTIES_EXT, graphics_pipeline_library);
   CASE(LINE_RASTERIZATION_PROPERTIES_EXT, line_rasterization);
   CASE(MULTI_DRAW_PROPERTIES_EXT, multi_draw);
   CASE(PROVOKING_VERTEX_PROPERTIES_EXT, provoking_vertex);
   CASE(ROBUSTNESS_2_PROPERTIES_EXT, robustness_2);
   CASE(TRANSFORM_FEEDBACK_PROPERTIES_EXT, transform_feedback);
                     case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT: {
      VkPhysicalDeviceDrmPropertiesEXT *out_props = (void *)out;
                  out_props->hasPrimary = info->drm.has_primary;
   out_props->primaryMajor = info->drm.primary_major;
   out_props->primaryMinor = info->drm.primary_minor;
   out_props->hasRender = info->drm.has_render;
   out_props->renderMajor = info->drm.render_major;
   out_props->renderMinor = info->drm.render_minor;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT:
      /* this is used by WSI */
   if (physical_dev->instance->renderer->info.pci.has_bus_info) {
      VkPhysicalDevicePCIBusInfoPropertiesEXT *out_props = (void *)out;
                  out_props->pciDomain = info->pci.domain;
   out_props->pciBus = info->pci.bus;
   out_props->pciDevice = info->pci.device;
      } else {
      assert(VN_DEBUG(VTEST));
   vk_copy_struct_guts(out,
            }
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENTATION_PROPERTIES_ANDROID: {
      VkPhysicalDevicePresentationPropertiesANDROID *out_props =
         out_props->sharedImage =
      vn_android_gralloc_get_shared_present_usage() ? VK_TRUE
                  default:
   #undef CASE
               }
      void
   vn_GetPhysicalDeviceQueueFamilyProperties2(
      VkPhysicalDevice physicalDevice,
   uint32_t *pQueueFamilyPropertyCount,
      {
      struct vn_physical_device *physical_dev =
            VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out,
         for (uint32_t i = 0; i < physical_dev->queue_family_count; i++) {
      vk_outarray_append_typed(VkQueueFamilyProperties2, &out, props) {
               }
      void
   vn_GetPhysicalDeviceMemoryProperties2(
      VkPhysicalDevice physicalDevice,
      {
      struct vn_physical_device *physical_dev =
         struct vn_instance *instance = physical_dev->instance;
            /* Don't waste time searching for unsupported structs. */
   if (physical_dev->base.base.supported_extensions.EXT_memory_budget) {
      memory_budget =
      vk_find_struct(pMemoryProperties->pNext,
            /* When the app queries invariant memory properties, we return a cached
   * copy. For dynamic properties, we must query the server.
   */
   if (memory_budget) {
      vn_call_vkGetPhysicalDeviceMemoryProperties2(instance, physicalDevice,
               /* Even when we query the server for memory properties, we must still
   * overwrite the invariant memory properties returned from the server with
   * our cached version.  Our cached version may differ from the server's
   * version due to workarounds.
   */
      }
      void
   vn_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice,
               {
      struct vn_physical_device *physical_dev =
            struct vn_format_properties_entry *entry = NULL;
   if (!pFormatProperties->pNext) {
      entry = vn_physical_device_get_format_properties(physical_dev, format);
   if (entry->valid) {
      pFormatProperties->formatProperties = entry->properties;
                  vn_call_vkGetPhysicalDeviceFormatProperties2(
            if (entry) {
      vn_physical_device_add_format_properties(
         }
      struct vn_physical_device_image_format_info {
      VkPhysicalDeviceImageFormatInfo2 format;
   VkPhysicalDeviceExternalImageFormatInfo external;
   VkImageFormatListCreateInfo list;
   VkImageStencilUsageCreateInfo stencil_usage;
      };
      static const VkPhysicalDeviceImageFormatInfo2 *
   vn_physical_device_fix_image_format_info(
      const VkPhysicalDeviceImageFormatInfo2 *info,
   const VkExternalMemoryHandleTypeFlagBits renderer_handle_type,
      {
      local_info->format = *info;
            bool is_ahb = false;
   bool has_format_list = false;
   /* we should generate deep copy functions... */
   vk_foreach_struct_const(src, info->pNext) {
      void *pnext = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      memcpy(&local_info->external, src, sizeof(local_info->external));
   is_ahb =
      local_info->external.handleType ==
      local_info->external.handleType = renderer_handle_type;
   pnext = &local_info->external;
      case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
      has_format_list = true;
   memcpy(&local_info->list, src, sizeof(local_info->list));
   pnext = &local_info->list;
      case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
      memcpy(&local_info->stencil_usage, src,
         pnext = &local_info->stencil_usage;
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_DRM_FORMAT_MODIFIER_INFO_EXT:
      memcpy(&local_info->modifier, src, sizeof(local_info->modifier));
   pnext = &local_info->modifier;
      default:
                  if (pnext) {
      dst->pNext = pnext;
                  if (is_ahb) {
      assert(local_info->format.tiling !=
         local_info->format.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
   if (!vn_android_get_drm_format_modifier_info(&local_info->format,
                  dst->pNext = (void *)&local_info->modifier;
            if ((info->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) &&
      (!has_format_list || !local_info->list.viewFormatCount)) {
   /* 12.3. Images
   *
   * If tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT and flags
   * contains VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, then the pNext chain
   * must include a VkImageFormatListCreateInfo structure with non-zero
   * viewFormatCount.
   */
   VkImageFormatListCreateInfo *list = &local_info->list;
   uint32_t vcount = 0;
   const VkFormat *vformats =
         if (!vformats) {
      /* local_info persists through the image format query call */
   vformats = &local_info->format.format;
               list->sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO;
                  if (!has_format_list) {
      dst->pNext = (void *)list;
                                 }
      VkResult
   vn_GetPhysicalDeviceImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceImageFormatInfo2 *pImageFormatInfo,
      {
      struct vn_physical_device *physical_dev =
         const VkExternalMemoryHandleTypeFlagBits renderer_handle_type =
         const VkExternalMemoryHandleTypeFlags supported_handle_types =
            const VkPhysicalDeviceExternalImageFormatInfo *external_info =
      vk_find_struct_const(pImageFormatInfo->pNext,
      if (external_info && !external_info->handleType)
            struct vn_physical_device_image_format_info local_info;
   if (external_info) {
      if (!(external_info->handleType & supported_handle_types)) {
      return vn_error(physical_dev->instance,
               /* Check the image tiling against the renderer handle type:
   * - No need to check for AHB since the tiling will either be forwarded
   *   or overwritten based on the renderer external memory type.
   * - For opaque fd and dma_buf fd handle types, passthrough tiling when
   *   the renderer external memory is dma_buf. Then we can avoid
   *   reconstructing the structs to support drm format modifier tiling
   *   like how we support AHB.
   */
   if (external_info->handleType !=
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
   if (renderer_handle_type ==
            pImageFormatInfo->tiling !=
         return vn_error(physical_dev->instance,
                  if (external_info->handleType != renderer_handle_type) {
      pImageFormatInfo = vn_physical_device_fix_image_format_info(
         if (!pImageFormatInfo) {
      return vn_error(physical_dev->instance,
                     VkResult result;
   /* TODO per-device cache */
   result = vn_call_vkGetPhysicalDeviceImageFormatProperties2(
      physical_dev->instance, physicalDevice, pImageFormatInfo,
      if (result != VK_SUCCESS || !external_info)
            if (external_info->handleType ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
   VkAndroidHardwareBufferUsageANDROID *ahb_usage =
      vk_find_struct(pImageFormatProperties->pNext,
      if (ahb_usage) {
      ahb_usage->androidHardwareBufferUsage = vn_android_get_ahb_usage(
               /* AHBs with mipmap usage will ignore this property */
               VkExternalImageFormatProperties *img_props = vk_find_struct(
         if (!img_props)
            VkExternalMemoryProperties *mem_props =
            if (renderer_handle_type ==
            !physical_dev->instance->renderer->info.has_dma_buf_import) {
   mem_props->externalMemoryFeatures &=
               if (external_info->handleType ==
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID) {
   /* AHB backed image requires renderer to support import bit */
   if (!(mem_props->externalMemoryFeatures &
                        mem_props->externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT |
   VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      mem_props->exportFromImportedHandleTypes =
         mem_props->compatibleHandleTypes =
      } else {
      mem_props->compatibleHandleTypes = supported_handle_types;
   mem_props->exportFromImportedHandleTypes =
      (mem_props->exportFromImportedHandleTypes & renderer_handle_type)
                     }
      void
   vn_GetPhysicalDeviceSparseImageFormatProperties2(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceSparseImageFormatInfo2 *pFormatInfo,
   uint32_t *pPropertyCount,
      {
         struct vn_physical_device *physical_dev =
         /* If VK_IMAGE_CREATE_SPARSE_RESIDENCY_BIT is not supported for the given
   * arguments, pPropertyCount will be set to zero upon return, and no data
   * will be written to pProperties.
   */
   if (physical_dev->sparse_binding_disabled) {
      *pPropertyCount = 0;
               /* TODO per-device cache */
   vn_call_vkGetPhysicalDeviceSparseImageFormatProperties2(
      physical_dev->instance, physicalDevice, pFormatInfo, pPropertyCount,
   }
      void
   vn_GetPhysicalDeviceExternalBufferProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalBufferInfo *pExternalBufferInfo,
      {
      struct vn_physical_device *physical_dev =
         const VkExternalMemoryHandleTypeFlagBits renderer_handle_type =
         const VkExternalMemoryHandleTypeFlags supported_handle_types =
         const bool is_ahb =
      pExternalBufferInfo->handleType ==
         VkExternalMemoryProperties *props =
         if (!(pExternalBufferInfo->handleType & supported_handle_types)) {
      props->compatibleHandleTypes = pExternalBufferInfo->handleType;
   props->exportFromImportedHandleTypes = 0;
   props->externalMemoryFeatures = 0;
               VkPhysicalDeviceExternalBufferInfo local_info;
   if (pExternalBufferInfo->handleType != renderer_handle_type) {
      local_info = *pExternalBufferInfo;
   local_info.handleType = renderer_handle_type;
               /* TODO per-device cache */
   vn_call_vkGetPhysicalDeviceExternalBufferProperties(
      physical_dev->instance, physicalDevice, pExternalBufferInfo,
         if (renderer_handle_type ==
            !physical_dev->instance->renderer->info.has_dma_buf_import) {
   props->externalMemoryFeatures &=
               if (is_ahb) {
      props->compatibleHandleTypes =
         /* AHB backed buffer requires renderer to support import bit while it
   * also requires the renderer to must not advertise dedicated only bit
   */
   if (!(props->externalMemoryFeatures &
            (props->externalMemoryFeatures &
   VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT)) {
   props->externalMemoryFeatures = 0;
   props->exportFromImportedHandleTypes = 0;
      }
   props->externalMemoryFeatures =
      VK_EXTERNAL_MEMORY_FEATURE_EXPORTABLE_BIT |
      props->exportFromImportedHandleTypes =
      } else {
      props->compatibleHandleTypes = supported_handle_types;
   props->exportFromImportedHandleTypes =
      (props->exportFromImportedHandleTypes & renderer_handle_type)
            }
      void
   vn_GetPhysicalDeviceExternalFenceProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
      {
      struct vn_physical_device *physical_dev =
            if (pExternalFenceInfo->handleType &
      physical_dev->external_fence_handles) {
   pExternalFenceProperties->compatibleHandleTypes =
         pExternalFenceProperties->exportFromImportedHandleTypes =
         pExternalFenceProperties->externalFenceFeatures =
      VK_EXTERNAL_FENCE_FEATURE_EXPORTABLE_BIT |
   } else {
      pExternalFenceProperties->compatibleHandleTypes = 0;
   pExternalFenceProperties->exportFromImportedHandleTypes = 0;
         }
      void
   vn_GetPhysicalDeviceExternalSemaphoreProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
      {
      struct vn_physical_device *physical_dev =
            const VkSemaphoreTypeCreateInfo *type_info = vk_find_struct_const(
         const VkSemaphoreType sem_type =
         const VkExternalSemaphoreHandleTypeFlags valid_handles =
      sem_type == VK_SEMAPHORE_TYPE_BINARY
      ? physical_dev->external_binary_semaphore_handles
   if (pExternalSemaphoreInfo->handleType & valid_handles) {
      pExternalSemaphoreProperties->compatibleHandleTypes = valid_handles;
   pExternalSemaphoreProperties->exportFromImportedHandleTypes =
         pExternalSemaphoreProperties->externalSemaphoreFeatures =
      VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
   } else {
      pExternalSemaphoreProperties->compatibleHandleTypes = 0;
   pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
         }
      VkResult
   vn_GetPhysicalDeviceCalibrateableTimeDomainsEXT(
      VkPhysicalDevice physicalDevice,
   uint32_t *pTimeDomainCount,
      {
      struct vn_physical_device *physical_dev =
            return vn_call_vkGetPhysicalDeviceCalibrateableTimeDomainsEXT(
      }
