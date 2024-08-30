   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * based in part on anv driver which is:
   * Copyright © 2015 Intel Corporation
   *
   * based in part on v3dv driver which is:
   * Copyright © 2019 Raspberry Pi
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <fcntl.h>
   #include <inttypes.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
   #include <xf86drm.h>
      #include "git_sha1.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_border.h"
   #include "pvr_clear.h"
   #include "pvr_csb.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_debug.h"
   #include "pvr_device_info.h"
   #include "pvr_dump_info.h"
   #include "pvr_hardcode.h"
   #include "pvr_job_render.h"
   #include "pvr_limits.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_robustness.h"
   #include "pvr_tex_state.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "pvr_util.h"
   #include "pvr_winsys.h"
   #include "rogue/rogue.h"
   #include "util/build_id.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "util/mesa-sha1.h"
   #include "util/os_misc.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_extensions.h"
   #include "vk_log.h"
   #include "vk_object.h"
   #include "vk_physical_device_features.h"
   #include "vk_physical_device_properties.h"
   #include "vk_sampler.h"
   #include "vk_util.h"
      #define PVR_GLOBAL_FREE_LIST_INITIAL_SIZE (2U * 1024U * 1024U)
   #define PVR_GLOBAL_FREE_LIST_MAX_SIZE (256U * 1024U * 1024U)
   #define PVR_GLOBAL_FREE_LIST_GROW_SIZE (1U * 1024U * 1024U)
      /* After PVR_SECONDARY_DEVICE_THRESHOLD devices per instance are created,
   * devices will have a smaller global free list size, as usually this use-case
   * implies smaller amounts of work spread out. The free list can still grow as
   * required.
   */
   #define PVR_SECONDARY_DEVICE_THRESHOLD (4U)
   #define PVR_SECONDARY_DEVICE_FREE_LIST_INITAL_SIZE (512U * 1024U)
      /* The grow threshold is a percentage. This is intended to be 12.5%, but has
   * been rounded up since the percentage is treated as an integer.
   */
   #define PVR_GLOBAL_FREE_LIST_GROW_THRESHOLD 13U
      #if defined(VK_USE_PLATFORM_DISPLAY_KHR)
   #   define PVR_USE_WSI_PLATFORM_DISPLAY true
   #else
   #   define PVR_USE_WSI_PLATFORM_DISPLAY false
   #endif
      #if PVR_USE_WSI_PLATFORM_DISPLAY
   #   define PVR_USE_WSI_PLATFORM true
   #else
   #   define PVR_USE_WSI_PLATFORM false
   #endif
      #define PVR_API_VERSION VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)
      /* Amount of padding required for VkBuffers to ensure we don't read beyond
   * a page boundary.
   */
   #define PVR_BUFFER_MEMORY_PADDING_SIZE 4
      /* Default size in bytes used by pvr_CreateDevice() for setting up the
   * suballoc_general, suballoc_pds and suballoc_usc suballocators.
   *
   * TODO: Investigate if a different default size can improve the overall
   * performance of internal driver allocations.
   */
   #define PVR_SUBALLOCATOR_GENERAL_SIZE (128 * 1024)
   #define PVR_SUBALLOCATOR_PDS_SIZE (128 * 1024)
   #define PVR_SUBALLOCATOR_TRANSFER_SIZE (128 * 1024)
   #define PVR_SUBALLOCATOR_USC_SIZE (128 * 1024)
   #define PVR_SUBALLOCATOR_VIS_TEST_SIZE (128 * 1024)
      struct pvr_drm_device_config {
      struct pvr_drm_device_info {
      const char *name;
         };
      #define DEF_CONFIG(render_, display_)                               \
      {                                                                \
      .render = { .name = render_, .len = sizeof(render_) - 1 },    \
            /* This is the list of supported DRM render/display driver configs. */
   static const struct pvr_drm_device_config pvr_drm_configs[] = {
      DEF_CONFIG("mediatek,mt8173-gpu", "mediatek-drm"),
      };
      #undef DEF_CONFIG
      static const struct vk_instance_extension_table pvr_instance_extensions = {
      .KHR_display = PVR_USE_WSI_PLATFORM_DISPLAY,
   .KHR_external_fence_capabilities = true,
   .KHR_external_memory_capabilities = true,
   .KHR_external_semaphore_capabilities = true,
   .KHR_get_display_properties2 = PVR_USE_WSI_PLATFORM_DISPLAY,
   .KHR_get_physical_device_properties2 = true,
   .KHR_get_surface_capabilities2 = PVR_USE_WSI_PLATFORM,
   .KHR_surface = PVR_USE_WSI_PLATFORM,
   .EXT_debug_report = true,
      };
      static void pvr_physical_device_get_supported_extensions(
         {
      *extensions = (struct vk_device_extension_table){
      .KHR_bind_memory2 = true,
   .KHR_copy_commands2 = true,
   /* TODO: enable this extension when the conformance tests get
   * updated to version 1.3.6.0, the current version does not
   * include the imagination driver ID, which will make a dEQP
   * test fail
   */
   .KHR_driver_properties = false,
   .KHR_external_fence = true,
   .KHR_external_fence_fd = true,
   .KHR_external_memory = true,
   .KHR_external_memory_fd = true,
   .KHR_format_feature_flags2 = true,
   .KHR_external_semaphore = PVR_USE_WSI_PLATFORM,
   .KHR_external_semaphore_fd = PVR_USE_WSI_PLATFORM,
   .KHR_get_memory_requirements2 = true,
   .KHR_image_format_list = true,
   .KHR_swapchain = PVR_USE_WSI_PLATFORM,
   .KHR_timeline_semaphore = true,
   .KHR_uniform_buffer_standard_layout = true,
   .EXT_external_memory_dma_buf = true,
   .EXT_host_query_reset = true,
   .EXT_private_data = true,
   .EXT_scalar_block_layout = true,
   .EXT_texel_buffer_alignment = true,
         }
      static void pvr_physical_device_get_supported_features(
      const struct pvr_device_info *const dev_info,
      {
      *features = (struct vk_features){
      /* Vulkan 1.0 */
   .robustBufferAccess = true,
   .fullDrawIndexUint32 = true,
   .imageCubeArray = true,
   .independentBlend = false,
   .geometryShader = false,
   .tessellationShader = false,
   .sampleRateShading = true,
   .dualSrcBlend = false,
   .logicOp = false,
   .multiDrawIndirect = true,
   .drawIndirectFirstInstance = true,
   .depthClamp = true,
   .depthBiasClamp = true,
   .fillModeNonSolid = false,
   .depthBounds = false,
   .wideLines = true,
   .largePoints = true,
   .alphaToOne = false,
   .multiViewport = false,
   .samplerAnisotropy = false,
   .textureCompressionETC2 = true,
   .textureCompressionASTC_LDR = false,
   .textureCompressionBC = false,
   .occlusionQueryPrecise = false,
   .pipelineStatisticsQuery = false,
   .vertexPipelineStoresAndAtomics = true,
   .fragmentStoresAndAtomics = true,
   .shaderTessellationAndGeometryPointSize = false,
   .shaderImageGatherExtended = false,
   .shaderStorageImageExtendedFormats = true,
   .shaderStorageImageMultisample = false,
   .shaderStorageImageReadWithoutFormat = true,
   .shaderStorageImageWriteWithoutFormat = false,
   .shaderUniformBufferArrayDynamicIndexing = true,
   .shaderSampledImageArrayDynamicIndexing = true,
   .shaderStorageBufferArrayDynamicIndexing = true,
   .shaderStorageImageArrayDynamicIndexing = true,
   .shaderClipDistance = false,
   .shaderCullDistance = false,
   .shaderFloat64 = false,
   .shaderInt64 = true,
   .shaderInt16 = true,
   .shaderResourceResidency = false,
   .shaderResourceMinLod = false,
   .sparseBinding = false,
   .sparseResidencyBuffer = false,
   .sparseResidencyImage2D = false,
   .sparseResidencyImage3D = false,
   .sparseResidency2Samples = false,
   .sparseResidency4Samples = false,
   .sparseResidency8Samples = false,
   .sparseResidency16Samples = false,
   .sparseResidencyAliased = false,
   .variableMultisampleRate = false,
            /* Vulkan 1.2 / VK_KHR_timeline_semaphore */
            /* Vulkan 1.2 / VK_KHR_uniform_buffer_standard_layout */
            /* Vulkan 1.2 / VK_EXT_host_query_reset */
            /* Vulkan 1.3 / VK_EXT_private_data */
            /* Vulkan 1.2 / VK_EXT_scalar_block_layout */
            /* Vulkan 1.3 / VK_EXT_texel_buffer_alignment */
            }
      static bool pvr_physical_device_init_pipeline_cache_uuid(
      const struct pvr_device_info *const dev_info,
      {
      struct mesa_sha1 sha1_ctx;
   unsigned build_id_len;
   uint8_t sha1[20];
            const struct build_id_note *note =
         if (!note) {
      mesa_loge("Failed to find build-id");
               build_id_len = build_id_length(note);
   if (build_id_len < 20) {
      mesa_loge("Build-id too short. It needs to be a SHA");
                        _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, build_id_data(note), build_id_len);
   _mesa_sha1_update(&sha1_ctx, &bvnc, sizeof(bvnc));
   _mesa_sha1_final(&sha1_ctx, sha1);
               }
      struct pvr_descriptor_limits {
      uint32_t max_per_stage_resources;
   uint32_t max_per_stage_samplers;
   uint32_t max_per_stage_uniform_buffers;
   uint32_t max_per_stage_storage_buffers;
   uint32_t max_per_stage_sampled_images;
   uint32_t max_per_stage_storage_images;
      };
      static const struct pvr_descriptor_limits *
   pvr_get_physical_device_descriptor_limits(
      const struct pvr_device_info *dev_info,
      {
      enum pvr_descriptor_cs_level {
      /* clang-format off */
   CS4096, /* 6XT and some XE cores with large CS. */
   CS2560, /* Mid range Rogue XE cores. */
   CS2048, /* Low end Rogue XE cores. */
   CS1536, /* Ultra-low-end 9XEP. */
   CS680,  /* lower limits for older devices. */
   CS408,  /* 7XE. */
               static const struct pvr_descriptor_limits descriptor_limits[] = {
      [CS4096] = { 1160U, 256U, 192U, 144U, 256U, 256U, 8U, },
   [CS2560] = {  648U, 128U, 128U, 128U, 128U, 128U, 8U, },
   [CS2048] = {  584U, 128U,  96U,  64U, 128U, 128U, 8U, },
   [CS1536] = {  456U,  64U,  96U,  64U, 128U,  64U, 8U, },
   [CS680]  = {  224U,  32U,  64U,  36U,  48U,   8U, 8U, },
               const uint32_t common_size =
      pvr_calc_fscommon_size_and_tiles_in_flight(dev_info,
                           if (common_size >= 2048) {
         } else if (common_size >= 1526) {
         } else if (common_size >= 680) {
         } else if (common_size >= 408) {
         } else {
      mesa_loge("This core appears to have a very limited amount of shared "
                        }
      static bool pvr_physical_device_get_properties(
      const struct pvr_device_info *const dev_info,
   const struct pvr_device_runtime_info *const dev_runtime_info,
      {
      const struct pvr_descriptor_limits *descriptor_limits =
            /* Default value based on the minimum value found in all existing cores. */
   const uint32_t max_multisample =
            /* Default value based on the minimum value found in all existing cores. */
            /* Default value based on the minimum value found in all existing cores. */
   const uint32_t uvs_pba_entries =
            /* Default value based on the minimum value found in all existing cores. */
   const uint32_t num_user_clip_planes =
            const uint32_t sub_pixel_precision =
                              const uint32_t max_user_vertex_components =
            /* The workgroup invocations are limited by the case where we have a compute
   * barrier - each slot has a fixed number of invocations, the whole workgroup
   * may need to span multiple slots. As each slot will WAIT at the barrier
   * until the last invocation completes, all have to be schedulable at the
   * same time.
   *
   * Typically all Rogue cores have 16 slots. Some of the smallest cores are
   * reduced to 14.
   *
   * The compute barrier slot exhaustion scenario can be tested with:
   * dEQP-VK.memory_model.message_passing*u32.coherent.fence_fence
   *    .atomicwrite*guard*comp
            /* Default value based on the minimum value found in all existing cores. */
            /* Default value based on the minimum value found in all existing cores. */
   const uint32_t max_instances_per_pds_task =
            const uint32_t max_compute_work_group_invocations =
                     *properties = (struct vk_properties){
      /* Vulkan 1.0 */
   .apiVersion = PVR_API_VERSION,
   .driverVersion = vk_get_driver_version(),
   .vendorID = VK_VENDOR_ID_IMAGINATION,
   .deviceID = dev_info->ident.device_id,
   .deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
            .maxImageDimension1D = max_render_size,
   .maxImageDimension2D = max_render_size,
   .maxImageDimension3D = PVR_MAX_TEXTURE_EXTENT_Z,
   .maxImageDimensionCube = max_render_size,
   .maxImageArrayLayers = PVR_MAX_ARRAY_LAYERS,
   .maxTexelBufferElements = 64U * 1024U,
   .maxUniformBufferRange = 128U * 1024U * 1024U,
   .maxStorageBufferRange = 128U * 1024U * 1024U,
   .maxPushConstantsSize = PVR_MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount = UINT32_MAX,
   .maxSamplerAllocationCount = UINT32_MAX,
   .bufferImageGranularity = 1U,
   .sparseAddressSpaceSize = 256ULL * 1024ULL * 1024ULL * 1024ULL,
   /* Maximum number of descriptor sets that can be bound simultaneously. */
   .maxBoundDescriptorSets = PVR_MAX_DESCRIPTOR_SETS,
   .maxPerStageResources = descriptor_limits->max_per_stage_resources,
   .maxPerStageDescriptorSamplers =
         .maxPerStageDescriptorUniformBuffers =
         .maxPerStageDescriptorStorageBuffers =
         .maxPerStageDescriptorSampledImages =
         .maxPerStageDescriptorStorageImages =
         .maxPerStageDescriptorInputAttachments =
         .maxDescriptorSetSamplers = 256U,
   .maxDescriptorSetUniformBuffers = 256U,
   .maxDescriptorSetUniformBuffersDynamic =
         .maxDescriptorSetStorageBuffers = 256U,
   .maxDescriptorSetStorageBuffersDynamic =
         .maxDescriptorSetSampledImages = 256U,
   .maxDescriptorSetStorageImages = 256U,
            /* Vertex Shader Limits */
   .maxVertexInputAttributes = PVR_MAX_VERTEX_INPUT_BINDINGS,
   .maxVertexInputBindings = PVR_MAX_VERTEX_INPUT_BINDINGS,
   .maxVertexInputAttributeOffset = 0xFFFF,
   .maxVertexInputBindingStride = 1024U * 1024U * 1024U * 2U,
            /* Tessellation Limits */
   .maxTessellationGenerationLevel = 0,
   .maxTessellationPatchSize = 0,
   .maxTessellationControlPerVertexInputComponents = 0,
   .maxTessellationControlPerVertexOutputComponents = 0,
   .maxTessellationControlPerPatchOutputComponents = 0,
   .maxTessellationControlTotalOutputComponents = 0,
   .maxTessellationEvaluationInputComponents = 0,
            /* Geometry Shader Limits */
   .maxGeometryShaderInvocations = 0,
   .maxGeometryInputComponents = 0,
   .maxGeometryOutputComponents = 0,
   .maxGeometryOutputVertices = 0,
            /* Fragment Shader Limits */
   .maxFragmentInputComponents = max_user_vertex_components,
   .maxFragmentOutputAttachments = PVR_MAX_COLOR_ATTACHMENTS,
   .maxFragmentDualSrcAttachments = 0,
   .maxFragmentCombinedOutputResources =
      descriptor_limits->max_per_stage_storage_buffers +
               /* Compute Shader Limits */
   .maxComputeSharedMemorySize = 16U * 1024U,
   .maxComputeWorkGroupCount = { 64U * 1024U, 64U * 1024U, 64U * 1024U },
   .maxComputeWorkGroupInvocations = max_compute_work_group_invocations,
   .maxComputeWorkGroupSize = { max_compute_work_group_invocations,
                  /* Rasterization Limits */
   .subPixelPrecisionBits = sub_pixel_precision,
   .subTexelPrecisionBits = 8U,
            .maxDrawIndexedIndexValue = UINT32_MAX,
   .maxDrawIndirectCount = 2U * 1024U * 1024U * 1024U,
   .maxSamplerLodBias = 16.0f,
   .maxSamplerAnisotropy = 1.0f,
            .maxViewportDimensions[0] = max_render_size,
   .maxViewportDimensions[1] = max_render_size,
   .viewportBoundsRange[0] = -(int32_t)(2U * max_render_size),
            .viewportSubPixelBits = 0,
   .minMemoryMapAlignment = 64U,
   .minTexelBufferOffsetAlignment = 16U,
   .minUniformBufferOffsetAlignment = 4U,
            .minTexelOffset = -8,
   .maxTexelOffset = 7U,
   .minTexelGatherOffset = -8,
   .maxTexelGatherOffset = 7,
   .minInterpolationOffset = -0.5,
   .maxInterpolationOffset = 0.5,
            .maxFramebufferWidth = max_render_size,
   .maxFramebufferHeight = max_render_size,
            .framebufferColorSampleCounts = max_sample_bits,
   .framebufferDepthSampleCounts = max_sample_bits,
   .framebufferStencilSampleCounts = max_sample_bits,
   .framebufferNoAttachmentsSampleCounts = max_sample_bits,
   .maxColorAttachments = PVR_MAX_COLOR_ATTACHMENTS,
   .sampledImageColorSampleCounts = max_sample_bits,
   .sampledImageIntegerSampleCounts = max_sample_bits,
   .sampledImageDepthSampleCounts = max_sample_bits,
   .sampledImageStencilSampleCounts = max_sample_bits,
   .storageImageSampleCounts = max_sample_bits,
   .maxSampleMaskWords = 1U,
   .timestampComputeAndGraphics = false,
   .timestampPeriod = 0.0f,
   .maxClipDistances = num_user_clip_planes,
   .maxCullDistances = num_user_clip_planes,
   .maxCombinedClipAndCullDistances = num_user_clip_planes,
   .discreteQueuePriorities = 2U,
   .pointSizeRange[0] = 1.0f,
   .pointSizeRange[1] = 511.0f,
   .pointSizeGranularity = 0.0625f,
   .lineWidthRange[0] = 1.0f / 16.0f,
   .lineWidthRange[1] = 16.0f,
   .lineWidthGranularity = 1.0f / 16.0f,
   .strictLines = false,
   .standardSampleLocations = true,
   .optimalBufferCopyOffsetAlignment = 4U,
   .optimalBufferCopyRowPitchAlignment = 4U,
            /* Vulkan 1.2 / VK_KHR_driver_properties */
   .driverID = VK_DRIVER_ID_IMAGINATION_OPEN_SOURCE_MESA,
   .driverName = "Imagination open-source Mesa driver",
   .driverInfo = "Mesa " PACKAGE_VERSION MESA_GIT_SHA1,
   .conformanceVersion = {
      .major = 1,
   .minor = 3,
   .subminor = 4,
               /* Vulkan 1.2 / VK_KHR_timeline_semaphore */
            /* Vulkan 1.3 / VK_EXT_texel_buffer_alignment */
   .storageTexelBufferOffsetAlignmentBytes = 16,
   .storageTexelBufferOffsetSingleTexelAlignment = true,
   .uniformTexelBufferOffsetAlignmentBytes = 16,
               snprintf(properties->deviceName,
            sizeof(properties->deviceName),
   "Imagination PowerVR %s %s",
         ret = pvr_physical_device_init_pipeline_cache_uuid(
      dev_info,
      if (!ret)
               }
      VkResult pvr_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = PVR_API_VERSION;
      }
      VkResult
   pvr_EnumerateInstanceExtensionProperties(const char *pLayerName,
               {
      if (pLayerName)
            return vk_enumerate_instance_extension_properties(&pvr_instance_extensions,
            }
      static void pvr_physical_device_destroy(struct vk_physical_device *vk_pdevice)
   {
      struct pvr_physical_device *pdevice =
            /* Be careful here. The device might not have been initialized. This can
   * happen since initialization is done in vkEnumeratePhysicalDevices() but
   * finish is done in vkDestroyInstance(). Make sure that you check for NULL
   * before freeing or that the freeing functions accept NULL pointers.
            if (pdevice->compiler)
                     if (pdevice->ws)
            vk_free(&pdevice->vk.instance->alloc, pdevice->render_path);
                        }
      void pvr_DestroyInstance(VkInstance _instance,
         {
               if (!instance)
                     vk_instance_finish(&instance->vk);
      }
      static uint64_t pvr_compute_heap_size(void)
   {
      /* Query the total ram from the system */
   uint64_t total_ram;
   if (!os_get_total_physical_memory(&total_ram))
            /* We don't want to burn too much ram with the GPU. If the user has 4GiB
   * or less, we use at most half. If they have more than 4GiB, we use 3/4.
   */
   uint64_t available_ram;
   if (total_ram <= 4ULL * 1024ULL * 1024ULL * 1024ULL)
         else
               }
      static VkResult pvr_physical_device_init(struct pvr_physical_device *pdevice,
                     {
      struct vk_physical_device_dispatch_table dispatch_table;
   struct vk_device_extension_table supported_extensions;
   struct vk_properties supported_properties;
   struct vk_features supported_features;
   struct pvr_winsys *ws;
   char *display_path;
   char *render_path;
            if (!getenv("PVR_I_WANT_A_BROKEN_VULKAN_DRIVER")) {
      return vk_errorf(instance,
                  VK_ERROR_INCOMPATIBLE_DRIVER,
   "WARNING: powervr is not a conformant Vulkan "
            render_path = vk_strdup(&instance->vk.alloc,
               if (!render_path) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               if (instance->vk.enabled_extensions.KHR_display) {
      display_path = vk_strdup(&instance->vk.alloc,
               if (!display_path) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
         } else {
                  result =
         if (result != VK_SUCCESS)
            pdevice->instance = instance;
   pdevice->render_path = render_path;
   pdevice->display_path = display_path;
            result = ws->ops->device_info_init(ws,
               if (result != VK_SUCCESS)
            pvr_physical_device_get_supported_extensions(&supported_extensions);
   pvr_physical_device_get_supported_features(&pdevice->dev_info,
         if (!pvr_physical_device_get_properties(&pdevice->dev_info,
                  result = vk_errorf(instance,
                           vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table,
   &pvr_physical_device_entrypoints,
         vk_physical_device_dispatch_table_from_entrypoints(
      &dispatch_table,
   &wsi_physical_device_entrypoints,
         result = vk_physical_device_init(&pdevice->vk,
                                 if (result != VK_SUCCESS)
                     /* Setup available memory heaps and types */
   pdevice->memory.memoryHeapCount = 1;
   pdevice->memory.memoryHeaps[0].size = pvr_compute_heap_size();
            pdevice->memory.memoryTypeCount = 1;
   pdevice->memory.memoryTypes[0].propertyFlags =
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
               result = pvr_wsi_init(pdevice);
   if (result != VK_SUCCESS) {
      vk_error(instance, result);
               pdevice->compiler = rogue_compiler_create(&pdevice->dev_info);
   if (!pdevice->compiler) {
      result = vk_errorf(instance,
                                 err_wsi_finish:
            err_vk_physical_device_finish:
            err_pvr_winsys_destroy:
            err_vk_free_display_path:
            err_vk_free_render_path:
            err_out:
         }
      static VkResult pvr_get_drm_devices(void *const obj,
                     {
      int ret = drmGetDevices2(0, devices, max_devices);
   if (ret < 0) {
      return vk_errorf(obj,
                  VK_ERROR_INITIALIZATION_FAILED,
            if (num_devices_out)
               }
      static bool
   pvr_drm_device_compatible(const struct pvr_drm_device_info *const info,
         {
               for (char **compat = compatible; *compat; compat++) {
      if (strncmp(*compat, info->name, info->len) == 0)
                  }
      static const struct pvr_drm_device_config *
   pvr_drm_device_get_config(drmDevice *const drm_dev)
   {
      for (size_t i = 0U; i < ARRAY_SIZE(pvr_drm_configs); i++) {
      if (pvr_drm_device_compatible(&pvr_drm_configs[i].render, drm_dev))
                  }
      static void
   pvr_physical_device_dump_info(const struct pvr_physical_device *pdevice,
               {
      drmVersionPtr version_display, version_render;
            version_display = drmGetVersion(pdevice->ws->display_fd);
   if (!version_display)
            version_render = drmGetVersion(pdevice->ws->render_fd);
   if (!version_render) {
      drmFreeVersion(version_display);
               info.device_info = &pdevice->dev_info;
   info.device_runtime_info = &pdevice->dev_runtime_info;
   info.drm_display.patchlevel = version_display->version_patchlevel;
   info.drm_display.major = version_display->version_major;
   info.drm_display.minor = version_display->version_minor;
   info.drm_display.name = version_display->name;
   info.drm_display.date = version_display->date;
   info.drm_display.comp = comp_display;
   info.drm_render.patchlevel = version_render->version_patchlevel;
   info.drm_render.major = version_render->version_major;
   info.drm_render.minor = version_render->version_minor;
   info.drm_render.name = version_render->name;
   info.drm_render.date = version_render->date;
                     drmFreeVersion(version_display);
      }
      static VkResult
   pvr_physical_device_enumerate(struct vk_instance *const vk_instance)
   {
      struct pvr_instance *const instance =
                     drmDevicePtr drm_display_device = NULL;
   drmDevicePtr drm_render_device = NULL;
   struct pvr_physical_device *pdevice;
   drmDevicePtr *drm_devices;
   int num_drm_devices = 0;
            result = pvr_get_drm_devices(instance, NULL, 0, &num_drm_devices);
   if (result != VK_SUCCESS)
            if (num_drm_devices == 0) {
      result = VK_SUCCESS;
               drm_devices = vk_alloc(&vk_instance->alloc,
                     if (!drm_devices) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_get_drm_devices(instance, drm_devices, num_drm_devices, NULL);
   if (result != VK_SUCCESS)
            /* First search for our render node... */
   for (int i = 0; i < num_drm_devices; i++) {
               if (drm_dev->bustype != DRM_BUS_PLATFORM)
            if (!(drm_dev->available_nodes & BITFIELD_BIT(DRM_NODE_RENDER)))
            config = pvr_drm_device_get_config(drm_dev);
   if (config) {
      drm_render_device = drm_dev;
                  if (!config) {
      result = VK_SUCCESS;
               mesa_logd("Found compatible render device '%s'.",
            /* ...then find the compatible display node. */
   for (int i = 0; i < num_drm_devices; i++) {
               if (!(drm_dev->available_nodes & BITFIELD_BIT(DRM_NODE_PRIMARY)))
            if (pvr_drm_device_compatible(&config->display, drm_dev)) {
      drm_display_device = drm_dev;
                  if (!drm_display_device) {
      mesa_loge("Render device '%s' has no compatible display device.",
         result = VK_SUCCESS;
               mesa_logd("Found compatible display device '%s'.",
            pdevice = vk_alloc(&vk_instance->alloc,
                     if (!pdevice) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_physical_device_init(pdevice,
                     if (result != VK_SUCCESS) {
      if (result == VK_ERROR_INCOMPATIBLE_DRIVER)
                        if (PVR_IS_DEBUG_SET(INFO)) {
      pvr_physical_device_dump_info(
      pdevice,
   drm_display_device->deviceinfo.platform->compatible,
                     result = VK_SUCCESS;
         err_free_pdevice:
            out_free_drm_devices:
            out_free_drm_device_ptrs:
            out:
         }
      VkResult pvr_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
      struct vk_instance_dispatch_table dispatch_table;
   struct pvr_instance *instance;
                     if (!pAllocator)
            instance = vk_alloc(pAllocator,
                     if (!instance)
            vk_instance_dispatch_table_from_entrypoints(&dispatch_table,
                  vk_instance_dispatch_table_from_entrypoints(&dispatch_table,
                  result = vk_instance_init(&instance->vk,
                           if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
                                 instance->vk.physical_devices.enumerate = pvr_physical_device_enumerate;
                                 }
      static uint32_t pvr_get_simultaneous_num_allocs(
      const struct pvr_device_info *dev_info,
      {
               if (PVR_HAS_FEATURE(dev_info, s8xe))
            assert(dev_runtime_info->num_phantoms == 1);
            if (min_cluster_per_phantom >= 4)
         else if (min_cluster_per_phantom == 2)
         else
      }
      uint32_t pvr_calc_fscommon_size_and_tiles_in_flight(
      const struct pvr_device_info *dev_info,
   const struct pvr_device_runtime_info *dev_runtime_info,
   uint32_t fs_common_size,
      {
      const uint32_t available_shareds =
         const uint32_t max_tiles_in_flight =
         uint32_t num_tile_in_flight;
            if (fs_common_size == 0)
                     if (fs_common_size == UINT32_MAX) {
                        if (!PVR_HAS_ERN(dev_info, 38748)) {
      /* Hardware needs space for one extra shared allocation. */
               /* Double resource requirements to deal with fragmentation. */
   max_common_size /= num_allocs * 2;
   max_common_size = MIN2(max_common_size, ROGUE_MAX_PIXEL_SHARED_REGISTERS);
   max_common_size =
                                       if (!PVR_HAS_ERN(dev_info, 38748))
                  #if defined(DEBUG)
               assert(num_tile_in_flight >= MIN2(num_tile_in_flight, max_tiles_in_flight));
            if (!PVR_HAS_ERN(dev_info, 38748)) {
      /* Hardware needs space for one extra shared allocation. */
                  #endif
            }
      const static VkQueueFamilyProperties pvr_queue_family_properties = {
      .queueFlags = VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT |
         .queueCount = PVR_MAX_QUEUES,
   .timestampValidBits = 0,
      };
      void pvr_GetPhysicalDeviceQueueFamilyProperties2(
      VkPhysicalDevice physicalDevice,
   uint32_t *pQueueFamilyPropertyCount,
      {
      VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2,
                        vk_outarray_append_typed (VkQueueFamilyProperties2, &out, p) {
               vk_foreach_struct (ext, p->pNext) {
               }
      void pvr_GetPhysicalDeviceMemoryProperties2(
      VkPhysicalDevice physicalDevice,
      {
                        vk_foreach_struct (ext, pMemoryProperties->pNext) {
            }
      PFN_vkVoidFunction pvr_GetInstanceProcAddr(VkInstance _instance,
         {
      PVR_FROM_HANDLE(pvr_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk,
            }
      /* With version 1+ of the loader interface the ICD should expose
   * vk_icdGetInstanceProcAddr to work around certain LD_PRELOAD issues seen in
   * apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName)
   {
         }
      /* With version 4+ of the loader interface the ICD should expose
   * vk_icdGetPhysicalDeviceProcAddr().
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName)
   {
      PVR_FROM_HANDLE(pvr_instance, instance, _instance);
      }
      VkResult pvr_pds_compute_shader_create_and_upload(
      struct pvr_device *device,
   struct pvr_pds_compute_shader_program *program,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t cache_line_size = rogue_get_slc_cache_line_size(dev_info);
   size_t staging_buffer_size;
   uint32_t *staging_buffer;
   uint32_t *data_buffer;
   uint32_t *code_buffer;
            /* Calculate how much space we'll need for the compute shader PDS program.
   */
            /* FIXME: Fix the below inconsistency of code size being in bytes whereas
   * data size being in dwords.
   */
   /* Code size is in bytes, data size in dwords. */
   staging_buffer_size =
            staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer)
            data_buffer = staging_buffer;
   code_buffer = pvr_pds_compute_shader(program,
                        pvr_pds_compute_shader(program,
                        result = pvr_gpu_upload_pds(device,
                              data_buffer,
   program->data_size,
   PVRX(CDMCTRL_KERNEL1_DATA_ADDR_ALIGNMENT),
                     }
      static VkResult pvr_device_init_compute_fence_program(struct pvr_device *device)
   {
               pvr_pds_compute_shader_program_init(&program);
   /* Fence kernel. */
   program.fence = true;
            return pvr_pds_compute_shader_create_and_upload(
      device,
   &program,
   }
      static VkResult pvr_device_init_compute_empty_program(struct pvr_device *device)
   {
               pvr_pds_compute_shader_program_init(&program);
            return pvr_pds_compute_shader_create_and_upload(
      device,
   &program,
   }
      static VkResult pvr_pds_idfwdf_programs_create_and_upload(
      struct pvr_device *device,
   pvr_dev_addr_t usc_addr,
   uint32_t shareds,
   uint32_t temps,
   pvr_dev_addr_t shareds_buffer_addr,
   struct pvr_pds_upload *const upload_out,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   struct pvr_pds_vertex_shader_sa_program program = {
      .kick_usc = true,
      };
   size_t staging_buffer_size;
   uint32_t *staging_buffer;
            /* We'll need to DMA the shareds into the USC's Common Store. */
   program.num_dma_kicks = pvr_pds_encode_dma_burst(program.dma_control,
                                          /* DMA temp regs. */
   pvr_pds_setup_doutu(&program.usc_task_control,
                                                staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer)
            /* FIXME: Add support for PDS_GENERATE_CODEDATA_SEGMENTS? */
   pvr_pds_vertex_shader_sa(&program,
                     pvr_pds_vertex_shader_sa(&program,
                        /* At the time of writing, the SW_COMPUTE_PDS_BARRIER variant of the program
   * is bigger so we handle it first (if needed) and realloc() for a smaller
   * size.
   */
   if (PVR_NEED_SW_COMPUTE_PDS_BARRIER(dev_info)) {
      /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              &staging_buffer[0],
   program.data_size,
   16,
      if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, staging_buffer);
                                 staging_buffer_size =
            staging_buffer = vk_realloc(&device->vk.alloc,
                           if (!staging_buffer) {
                           /* FIXME: Add support for PDS_GENERATE_CODEDATA_SEGMENTS? */
   pvr_pds_vertex_shader_sa(&program,
                     pvr_pds_vertex_shader_sa(&program,
                  } else {
      *sw_compute_barrier_upload_out = (struct pvr_pds_upload){
                     /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              &staging_buffer[0],
   program.data_size,
   16,
      if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, staging_buffer);
                                    }
      static VkResult pvr_device_init_compute_idfwdf_state(struct pvr_device *device)
   {
      uint64_t sampler_state[ROGUE_NUM_TEXSTATE_SAMPLER_WORDS];
   uint64_t image_state[ROGUE_NUM_TEXSTATE_IMAGE_WORDS];
   struct util_dynarray usc_program;
   struct pvr_texture_state_info tex_info;
   uint32_t *dword_ptr;
   uint32_t usc_shareds;
   uint32_t usc_temps;
            util_dynarray_init(&usc_program, NULL);
   pvr_hard_code_get_idfwdf_program(&device->pdevice->dev_info,
                                 /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_usc(device,
                                    if (result != VK_SUCCESS)
            /* TODO: Get the store buffer size from the compiler? */
   /* TODO: How was the size derived here? */
   result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
                     pvr_csb_pack (&sampler_state[0], TEXSTATE_SAMPLER, sampler) {
      sampler.dadjust = PVRX(TEXSTATE_DADJUST_ZERO_UINT);
   sampler.magfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
               /* clang-format off */
   pvr_csb_pack (&sampler_state[1], TEXSTATE_SAMPLER_WORD1, sampler_word1) {}
                     tex_info = (struct pvr_texture_state_info){
      .format = VK_FORMAT_R32G32B32A32_SFLOAT,
   .mem_layout = PVR_MEMLAYOUT_LINEAR,
   .flags = PVR_TEXFLAGS_INDEX_LOOKUP,
   .type = VK_IMAGE_VIEW_TYPE_2D,
   .extent = { .width = 4, .height = 2, .depth = 0 },
   .mip_levels = 1,
   .sample_count = 1,
   .stride = 4,
   .swizzle = { PIPE_SWIZZLE_X,
               PIPE_SWIZZLE_Y,
               result = pvr_pack_tex_state(device, &tex_info, image_state);
   if (result != VK_SUCCESS)
                           #define HIGH_32(val) ((uint32_t)((val) >> 32U))
   #define LOW_32(val) ((uint32_t)(val))
         /* TODO: Should we use compiler info to setup the shareds data instead of
   * assuming there's always 12 and this is how they should be setup?
            dword_ptr[0] = HIGH_32(device->idfwdf_state.store_bo->vma->dev_addr.addr);
            /* Pad the shareds as the texture/sample state words are 128 bit aligned. */
   dword_ptr[2] = 0U;
            dword_ptr[4] = LOW_32(image_state[0]);
   dword_ptr[5] = HIGH_32(image_state[0]);
   dword_ptr[6] = LOW_32(image_state[1]);
            dword_ptr[8] = LOW_32(sampler_state[0]);
   dword_ptr[9] = HIGH_32(sampler_state[0]);
   dword_ptr[10] = LOW_32(sampler_state[1]);
   dword_ptr[11] = HIGH_32(sampler_state[1]);
         #undef HIGH_32
   #undef LOW_32
         pvr_bo_cpu_unmap(device, device->idfwdf_state.shareds_bo);
            /* Generate and upload PDS programs. */
   result = pvr_pds_idfwdf_programs_create_and_upload(
      device,
   device->idfwdf_state.usc->dev_addr,
   usc_shareds,
   usc_temps,
   device->idfwdf_state.shareds_bo->vma->dev_addr,
   &device->idfwdf_state.pds,
      if (result != VK_SUCCESS)
                  err_free_shareds_buffer:
            err_free_store_buffer:
            err_free_usc_program:
                  }
      static void pvr_device_finish_compute_idfwdf_state(struct pvr_device *device)
   {
      pvr_bo_suballoc_free(device->idfwdf_state.pds.pvr_bo);
   pvr_bo_suballoc_free(device->idfwdf_state.sw_compute_barrier_pds.pvr_bo);
   pvr_bo_free(device, device->idfwdf_state.shareds_bo);
   pvr_bo_free(device, device->idfwdf_state.store_bo);
      }
      /* FIXME: We should be calculating the size when we upload the code in
   * pvr_srv_setup_static_pixel_event_program().
   */
   static void pvr_device_get_pixel_event_pds_program_data_size(
      const struct pvr_device_info *dev_info,
      {
      struct pvr_pds_event_program program = {
      /* No data to DMA, just a DOUTU needed. */
                           }
      static VkResult pvr_device_init_nop_program(struct pvr_device *device)
   {
      const uint32_t cache_line_size =
         struct pvr_pds_kickusc_program program = { 0 };
   struct util_dynarray nop_usc_bin;
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
                     result = pvr_gpu_upload_usc(device,
                           util_dynarray_fini(&nop_usc_bin);
   if (result != VK_SUCCESS)
            /* Setup a PDS program that kicks the static USC program. */
   pvr_pds_setup_doutu(&program.usc_task_control,
                                                staging_buffer = vk_alloc(&device->vk.alloc,
                     if (!staging_buffer) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
                        /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              staging_buffer,
   program.data_size,
   16U,
      if (result != VK_SUCCESS)
                           err_free_staging_buffer:
            err_free_nop_usc_bo:
                  }
      static void pvr_device_init_tile_buffer_state(struct pvr_device *device)
   {
               for (uint32_t i = 0; i < ARRAY_SIZE(device->tile_buffer_state.buffers); i++)
               }
      static void pvr_device_finish_tile_buffer_state(struct pvr_device *device)
   {
      /* Destroy the mutex first to trigger asserts in case it's still locked so
   * that we don't put things in an inconsistent state by freeing buffers that
   * might be in use or attempt to free buffers while new buffers are being
   * allocated.
   */
            for (uint32_t i = 0; i < device->tile_buffer_state.buffer_count; i++)
      }
      /**
   * \brief Ensures that a certain amount of tile buffers are allocated.
   *
   * Make sure that \p capacity amount of tile buffers are allocated. If less were
   * present, append new tile buffers of \p size_in_bytes each to reach the quota.
   */
   VkResult pvr_device_tile_buffer_ensure_cap(struct pvr_device *device,
               {
      struct pvr_device_tile_buffer_state *tile_buffer_state =
         const uint32_t cache_line_size =
                           /* Clamping in release and asserting in debug. */
   assert(capacity <= ARRAY_SIZE(tile_buffer_state->buffers));
   capacity = CLAMP(capacity,
                  /* TODO: Implement bo multialloc? To reduce the amount of syscalls and
   * allocations.
   */
   for (uint32_t i = tile_buffer_state->buffer_count; i < capacity; i++) {
      result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS) {
                                                            err_release_lock:
                  }
      static void pvr_device_init_default_sampler_state(struct pvr_device *device)
   {
      pvr_csb_pack (&device->input_attachment_sampler, TEXSTATE_SAMPLER, sampler) {
      sampler.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.addrmode_v = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.addrmode_w = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.dadjust = PVRX(TEXSTATE_DADJUST_ZERO_UINT);
   sampler.magfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.minfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.anisoctl = PVRX(TEXSTATE_ANISOCTL_DISABLED);
         }
      VkResult pvr_CreateDevice(VkPhysicalDevice physicalDevice,
                     {
      PVR_FROM_HANDLE(pvr_physical_device, pdevice, physicalDevice);
   uint32_t initial_free_list_size = PVR_GLOBAL_FREE_LIST_INITIAL_SIZE;
   struct pvr_instance *instance = pdevice->instance;
   struct vk_device_dispatch_table dispatch_table;
   struct pvr_device *device;
   struct pvr_winsys *ws;
                     result = pvr_winsys_create(pdevice->render_path,
                     if (result != VK_SUCCESS)
            device = vk_alloc2(&instance->vk.alloc,
                     pAllocator,
   if (!device) {
      result = vk_error(instance, VK_ERROR_OUT_OF_HOST_MEMORY);
               vk_device_dispatch_table_from_entrypoints(&dispatch_table,
                  vk_device_dispatch_table_from_entrypoints(&dispatch_table,
                  result = vk_device_init(&device->vk,
                           if (result != VK_SUCCESS)
            device->instance = instance;
   device->pdevice = pdevice;
                     if (ws->features.supports_threaded_submit) {
      /* Queue submission can be blocked if the kernel CCBs become full,
   * so enable threaded submit to not block the submitter.
   */
                        result = pvr_bo_store_create(device);
   if (result != VK_SUCCESS)
            pvr_bo_suballocator_init(&device->suballoc_general,
                     pvr_bo_suballocator_init(&device->suballoc_pds,
                     pvr_bo_suballocator_init(&device->suballoc_transfer,
                     pvr_bo_suballocator_init(&device->suballoc_usc,
                     pvr_bo_suballocator_init(&device->suballoc_vis_test,
                        if (p_atomic_inc_return(&instance->active_device_count) >
      PVR_SECONDARY_DEVICE_THRESHOLD) {
               result = pvr_free_list_create(device,
                                 initial_free_list_size,
   if (result != VK_SUCCESS)
            result = pvr_device_init_nop_program(device);
   if (result != VK_SUCCESS)
            result = pvr_device_init_compute_fence_program(device);
   if (result != VK_SUCCESS)
            result = pvr_device_init_compute_empty_program(device);
   if (result != VK_SUCCESS)
            result = pvr_device_create_compute_query_programs(device);
   if (result != VK_SUCCESS)
            result = pvr_device_init_compute_idfwdf_state(device);
   if (result != VK_SUCCESS)
            result = pvr_device_init_graphics_static_clear_state(device);
   if (result != VK_SUCCESS)
            result = pvr_device_init_spm_load_state(device);
   if (result != VK_SUCCESS)
                     result = pvr_queues_create(device, pCreateInfo);
   if (result != VK_SUCCESS)
                              result = pvr_init_robustness_buffer(device);
   if (result != VK_SUCCESS)
            result = pvr_border_color_table_init(&device->border_color_table, device);
   if (result != VK_SUCCESS)
            /* FIXME: Move this to a later stage and possibly somewhere other than
   * pvr_device. The purpose of this is so that we don't have to get the size
   * on each kick.
   */
   pvr_device_get_pixel_event_pds_program_data_size(
      &pdevice->dev_info,
         device->global_cmd_buffer_submit_count = 0;
                           err_pvr_robustness_buffer_finish:
            err_pvr_spm_finish_scratch_buffer_store:
                     err_pvr_finish_tile_buffer_state:
      pvr_device_finish_tile_buffer_state(device);
         err_pvr_finish_graphics_static_clear_state:
            err_pvr_finish_compute_idfwdf:
            err_pvr_destroy_compute_query_programs:
            err_pvr_free_compute_empty:
            err_pvr_free_compute_fence:
            err_pvr_free_nop_program:
      pvr_bo_suballoc_free(device->nop_program.pds.pvr_bo);
         err_pvr_free_list_destroy:
            err_dec_device_count:
               pvr_bo_suballocator_fini(&device->suballoc_vis_test);
   pvr_bo_suballocator_fini(&device->suballoc_usc);
   pvr_bo_suballocator_fini(&device->suballoc_transfer);
   pvr_bo_suballocator_fini(&device->suballoc_pds);
                  err_vk_device_finish:
            err_free_device:
            err_pvr_winsys_destroy:
            err_out:
         }
      void pvr_DestroyDevice(VkDevice _device,
         {
               if (!device)
            pvr_border_color_table_finish(&device->border_color_table, device);
   pvr_robustness_buffer_finish(device);
   pvr_spm_finish_scratch_buffer_store(device);
   pvr_queues_destroy(device);
   pvr_device_finish_tile_buffer_state(device);
   pvr_device_finish_spm_load_state(device);
   pvr_device_finish_graphics_static_clear_state(device);
   pvr_device_finish_compute_idfwdf_state(device);
   pvr_device_destroy_compute_query_programs(device);
   pvr_bo_suballoc_free(device->pds_compute_empty_program.pvr_bo);
   pvr_bo_suballoc_free(device->pds_compute_fence_program.pvr_bo);
   pvr_bo_suballoc_free(device->nop_program.pds.pvr_bo);
   pvr_bo_suballoc_free(device->nop_program.usc);
   pvr_free_list_destroy(device->global_free_list);
   pvr_bo_suballocator_fini(&device->suballoc_vis_test);
   pvr_bo_suballocator_fini(&device->suballoc_usc);
   pvr_bo_suballocator_fini(&device->suballoc_transfer);
   pvr_bo_suballocator_fini(&device->suballoc_pds);
   pvr_bo_suballocator_fini(&device->suballoc_general);
   pvr_bo_store_destroy(device);
   pvr_winsys_destroy(device->ws);
   p_atomic_dec(&device->instance->active_device_count);
   vk_device_finish(&device->vk);
      }
      VkResult pvr_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
         {
      if (!pProperties) {
      *pPropertyCount = 0;
                  }
      VkResult pvr_AllocateMemory(VkDevice _device,
                     {
      const VkImportMemoryFdInfoKHR *fd_info = NULL;
   PVR_FROM_HANDLE(pvr_device, device, _device);
   enum pvr_winsys_bo_type type = PVR_WINSYS_BO_TYPE_GPU;
   struct pvr_device_memory *mem;
            assert(pAllocateInfo->sType == VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
            mem = vk_object_alloc(&device->vk,
                     if (!mem)
            vk_foreach_struct_const (ext, pAllocateInfo->pNext) {
      switch ((unsigned)ext->sType) {
   case VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA:
      type = PVR_WINSYS_BO_TYPE_DISPLAY;
      case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
      fd_info = (void *)ext;
      default:
      pvr_debug_ignored_stype(ext->sType);
                  if (fd_info && fd_info->handleType) {
      VkDeviceSize aligned_alloc_size =
            assert(
                  result = device->ws->ops->buffer_create_from_fd(device->ws,
               if (result != VK_SUCCESS)
            /* For security purposes, we reject importing the bo if it's smaller
   * than the requested allocation size. This prevents a malicious client
   * from passing a buffer to a trusted client, lying about the size, and
   * telling the trusted client to try and texture from an image that goes
   * out-of-bounds. This sort of thing could lead to GPU hangs or worse
   * in the trusted client. The trusted client can protect itself against
   * this sort of attack but only if it can trust the buffer size.
   */
   if (aligned_alloc_size > mem->bo->size) {
      result = vk_errorf(device,
                     VK_ERROR_INVALID_EXTERNAL_HANDLE,
   "Aligned requested size too large for the given fd "
   device->ws->ops->buffer_destroy(mem->bo);
               /* From the Vulkan spec:
   *
   *    "Importing memory from a file descriptor transfers ownership of
   *    the file descriptor from the application to the Vulkan
   *    implementation. The application must not perform any operations on
   *    the file descriptor after a successful import."
   *
   * If the import fails, we leave the file descriptor open.
   */
      } else {
      /* Align physical allocations to the page size of the heap that will be
   * used when binding device memory (see pvr_bind_memory()) to ensure the
   * entire allocation can be mapped.
   */
            /* FIXME: Need to determine the flags based on
   * device->pdevice->memory.memoryTypes[pAllocateInfo->memoryTypeIndex].propertyFlags.
   *
   * The alternative would be to store the flags alongside the memory
   * types as an array that's indexed by pAllocateInfo->memoryTypeIndex so
   * that they can be looked up.
   */
   result = device->ws->ops->buffer_create(device->ws,
                                 if (result != VK_SUCCESS)
                              err_vk_object_free_mem:
                  }
      VkResult pvr_GetMemoryFdKHR(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
                     assert(
      pGetFdInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
            }
      VkResult
   pvr_GetMemoryFdPropertiesKHR(VkDevice _device,
                     {
               switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      /* FIXME: This should only allow memory types having
   * VK_MEMORY_PROPERTY_HOST_CACHED_BIT flag set, as
   * dma-buf should be imported using cacheable memory types,
   * given exporter's mmap will always map it as cacheable.
   * Ref:
   * https://www.kernel.org/doc/html/latest/driver-api/dma-buf.html#c.dma_buf_ops
   */
   pMemoryFdProperties->memoryTypeBits =
            default:
            }
      void pvr_FreeMemory(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!mem)
            /* From the Vulkan spec (§11.2.13. Freeing Device Memory):
   *   If a memory object is mapped at the time it is freed, it is implicitly
   *   unmapped.
   */
   if (mem->bo->map)
                        }
      VkResult pvr_MapMemory(VkDevice _device,
                        VkDeviceMemory _memory,
      {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   PVR_FROM_HANDLE(pvr_device_memory, mem, _memory);
            if (!mem) {
      *ppData = NULL;
               if (size == VK_WHOLE_SIZE)
            /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * If size is not equal to VK_WHOLE_SIZE, size must be greater than 0
   *    assert(size != 0);
   *  * If size is not equal to VK_WHOLE_SIZE, size must be less than or
   *    equal to the size of the memory minus offset
            assert(size > 0);
            /* Check if already mapped */
   if (mem->bo->map) {
      *ppData = (uint8_t *)mem->bo->map + offset;
               /* Map it all at once */
   result = device->ws->ops->buffer_map(mem->bo);
   if (result != VK_SUCCESS)
                        }
      void pvr_UnmapMemory(VkDevice _device, VkDeviceMemory _memory)
   {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!mem || !mem->bo->map)
               }
      VkResult pvr_FlushMappedMemoryRanges(VkDevice _device,
               {
         }
      VkResult
   pvr_InvalidateMappedMemoryRanges(VkDevice _device,
               {
         }
      void pvr_GetImageSparseMemoryRequirements2(
      VkDevice device,
   const VkImageSparseMemoryRequirementsInfo2 *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
         }
      void pvr_GetDeviceMemoryCommitment(VkDevice device,
               {
         }
      VkResult pvr_bind_memory(struct pvr_device *device,
                           struct pvr_device_memory *mem,
   VkDeviceSize offset,
   {
      VkDeviceSize virt_size =
         struct pvr_winsys_vma *vma;
   pvr_dev_addr_t dev_addr;
            /* Valid usage:
   *
   *   "memoryOffset must be an integer multiple of the alignment member of
   *    the VkMemoryRequirements structure returned from a call to
   *    vkGetBufferMemoryRequirements with buffer"
   *
   *   "memoryOffset must be an integer multiple of the alignment member of
   *    the VkMemoryRequirements structure returned from a call to
   *    vkGetImageMemoryRequirements with image"
   */
   assert(offset % alignment == 0);
            result = device->ws->ops->heap_alloc(device->heaps.general_heap,
                     if (result != VK_SUCCESS)
            result = device->ws->ops->vma_map(vma, mem->bo, offset, size, &dev_addr);
   if (result != VK_SUCCESS)
            *dev_addr_out = dev_addr;
                  err_free_vma:
            err_out:
         }
      void pvr_unbind_memory(struct pvr_device *device, struct pvr_winsys_vma *vma)
   {
      device->ws->ops->vma_unmap(vma);
      }
      VkResult pvr_BindBufferMemory2(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            for (i = 0; i < bindInfoCount; i++) {
      PVR_FROM_HANDLE(pvr_device_memory, mem, pBindInfos[i].memory);
            VkResult result = pvr_bind_memory(device,
                                       if (result != VK_SUCCESS) {
      while (i--) {
      PVR_FROM_HANDLE(pvr_buffer, buffer, pBindInfos[i].buffer);
                                 }
      VkResult pvr_QueueBindSparse(VkQueue _queue,
                     {
         }
      /* Event functions. */
      VkResult pvr_CreateEvent(VkDevice _device,
                     {
               struct pvr_event *event = vk_object_alloc(&device->vk,
                     if (!event)
            event->sync = NULL;
                        }
      void pvr_DestroyEvent(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!event)
            if (event->sync)
               }
      VkResult pvr_GetEventStatus(VkDevice _device, VkEvent _event)
   {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   PVR_FROM_HANDLE(pvr_event, event, _event);
            switch (event->state) {
   case PVR_EVENT_STATE_SET_BY_DEVICE:
      if (!event->sync)
            result =
         result = (result == VK_SUCCESS) ? VK_EVENT_SET : VK_EVENT_RESET;
         case PVR_EVENT_STATE_RESET_BY_DEVICE:
      if (!event->sync)
            result =
         result = (result == VK_SUCCESS) ? VK_EVENT_RESET : VK_EVENT_SET;
         case PVR_EVENT_STATE_SET_BY_HOST:
      result = VK_EVENT_SET;
         case PVR_EVENT_STATE_RESET_BY_HOST:
      result = VK_EVENT_RESET;
         default:
                     }
      VkResult pvr_SetEvent(VkDevice _device, VkEvent _event)
   {
               if (event->sync) {
               const VkResult result = vk_sync_signal(&device->vk, event->sync, 0);
   if (result != VK_SUCCESS)
                           }
      VkResult pvr_ResetEvent(VkDevice _device, VkEvent _event)
   {
               if (event->sync) {
               const VkResult result = vk_sync_reset(&device->vk, event->sync);
   if (result != VK_SUCCESS)
                           }
      /* Buffer functions. */
      VkResult pvr_CreateBuffer(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   const uint32_t alignment = 4096;
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
            /* We check against (ULONG_MAX - alignment) to prevent overflow issues */
   if (pCreateInfo->size >= ULONG_MAX - alignment)
            buffer =
         if (!buffer)
                                 }
      void pvr_DestroyBuffer(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!buffer)
            if (buffer->vma)
               }
      VkResult pvr_gpu_upload(struct pvr_device *device,
                           struct pvr_winsys_heap *heap,
   {
      struct pvr_suballoc_bo *suballoc_bo = NULL;
   struct pvr_suballocator *allocator;
   VkResult result;
                     if (heap == device->heaps.general_heap)
         else if (heap == device->heaps.pds_heap)
         else if (heap == device->heaps.transfer_frag_heap)
         else if (heap == device->heaps.usc_heap)
         else
            result = pvr_bo_suballoc(allocator, size, alignment, false, &suballoc_bo);
   if (result != VK_SUCCESS)
            map = pvr_bo_suballoc_get_map_addr(suballoc_bo);
                        }
      VkResult pvr_gpu_upload_usc(struct pvr_device *device,
                           {
      struct pvr_suballoc_bo *suballoc_bo = NULL;
   VkResult result;
                     /* The USC will prefetch the next instruction, so over allocate by 1
   * instruction to prevent reading off the end of a page into a potentially
   * unallocated page.
   */
   result = pvr_bo_suballoc(&device->suballoc_usc,
                           if (result != VK_SUCCESS)
            map = pvr_bo_suballoc_get_map_addr(suballoc_bo);
                        }
      /**
   * \brief Upload PDS program data and code segments from host memory to device
   * memory.
   *
   * \param[in] device            Logical device pointer.
   * \param[in] data              Pointer to PDS data segment to upload.
   * \param[in] data_size_dwords  Size of PDS data segment in dwords.
   * \param[in] data_alignment    Required alignment of the PDS data segment in
   *                              bytes. Must be a power of two.
   * \param[in] code              Pointer to PDS code segment to upload.
   * \param[in] code_size_dwords  Size of PDS code segment in dwords.
   * \param[in] code_alignment    Required alignment of the PDS code segment in
   *                              bytes. Must be a power of two.
   * \param[in] min_alignment     Minimum alignment of the bo holding the PDS
   *                              program in bytes.
   * \param[out] pds_upload_out   On success will be initialized based on the
   *                              uploaded PDS program.
   * \return VK_SUCCESS on success, or error code otherwise.
   */
   VkResult pvr_gpu_upload_pds(struct pvr_device *device,
                              const uint32_t *data,
   uint32_t data_size_dwords,
   uint32_t data_alignment,
      {
      /* All alignment and sizes below are in bytes. */
   const size_t data_size = PVR_DW_TO_BYTES(data_size_dwords);
   const size_t code_size = PVR_DW_TO_BYTES(code_size_dwords);
   const uint64_t data_aligned_size = ALIGN_POT(data_size, data_alignment);
   const uint64_t code_aligned_size = ALIGN_POT(code_size, code_alignment);
   const uint32_t code_offset = ALIGN_POT(data_aligned_size, code_alignment);
   const uint64_t bo_alignment = MAX2(min_alignment, data_alignment);
   const uint64_t bo_size = (!!code) ? (code_offset + code_aligned_size)
         VkResult result;
            assert(code || data);
   assert(!code || (code_size_dwords != 0 && code_alignment != 0));
            result = pvr_bo_suballoc(&device->suballoc_pds,
                           if (result != VK_SUCCESS)
                     if (data) {
               pds_upload_out->data_offset = pds_upload_out->pvr_bo->dev_addr.addr -
            /* Store data size in dwords. */
   assert(data_aligned_size % 4 == 0);
      } else {
      pds_upload_out->data_offset = 0;
               if (code) {
               pds_upload_out->code_offset =
                  /* Store code size in dwords. */
   assert(code_aligned_size % 4 == 0);
      } else {
      pds_upload_out->code_offset = 0;
                  }
      static VkResult
   pvr_framebuffer_create_ppp_state(struct pvr_device *device,
         {
      const uint32_t cache_line_size =
         uint32_t ppp_state[3];
            pvr_csb_pack (&ppp_state[0], TA_STATE_HEADER, header) {
                  pvr_csb_pack (&ppp_state[1], TA_STATE_TERMINATE0, term0) {
      term0.clip_right =
      DIV_ROUND_UP(
      framebuffer->width,
         term0.clip_bottom =
      DIV_ROUND_UP(
      framebuffer->height,
               pvr_csb_pack (&ppp_state[2], TA_STATE_TERMINATE1, term1) {
      term1.render_target = 0;
               result = pvr_gpu_upload(device,
                           device->heaps.general_heap,
   if (result != VK_SUCCESS)
            /* Calculate the size of PPP state in dwords. */
               }
      static bool pvr_render_targets_init(struct pvr_render_target *render_targets,
         {
               for (i = 0; i < render_targets_count; i++) {
      if (pthread_mutex_init(&render_targets[i].mutex, NULL))
                     err_mutex_destroy:
      while (i--)
               }
      static void pvr_render_targets_fini(struct pvr_render_target *render_targets,
         {
      for (uint32_t i = 0; i < render_targets_count; i++) {
      if (render_targets[i].valid) {
      pvr_render_target_dataset_destroy(render_targets[i].rt_dataset);
                     }
      VkResult pvr_CreateFramebuffer(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_render_pass, pass, pCreateInfo->renderPass);
   PVR_FROM_HANDLE(pvr_device, device, _device);
   struct pvr_spm_bgobj_state *spm_bgobj_state_per_render;
   struct pvr_spm_eot_state *spm_eot_state_per_render;
   struct pvr_render_target *render_targets;
   struct pvr_framebuffer *framebuffer;
   struct pvr_image_view **attachments;
   uint32_t render_targets_count;
   uint64_t scratch_buffer_size;
                     render_targets_count =
            VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &framebuffer, __typeof__(*framebuffer), 1);
   vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                        if (!vk_multialloc_zalloc2(&ma,
                              vk_object_base_init(&device->vk,
                  framebuffer->width = pCreateInfo->width;
   framebuffer->height = pCreateInfo->height;
            framebuffer->attachments = attachments;
   framebuffer->attachment_count = pCreateInfo->attachmentCount;
   for (uint32_t i = 0; i < framebuffer->attachment_count; i++) {
      framebuffer->attachments[i] =
               result = pvr_framebuffer_create_ppp_state(device, framebuffer);
   if (result != VK_SUCCESS)
            framebuffer->render_targets = render_targets;
   framebuffer->render_targets_count = render_targets_count;
   if (!pvr_render_targets_init(framebuffer->render_targets,
            result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               scratch_buffer_size =
      pvr_spm_scratch_buffer_calc_required_size(pass,
               result = pvr_spm_scratch_buffer_get_buffer(device,
               if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
               result = pvr_spm_init_eot_state(device,
                           if (result != VK_SUCCESS)
            result = pvr_spm_init_bgobj_state(device,
                           if (result != VK_SUCCESS)
               err_finish_bgobj_state:
                  for (uint32_t j = 0; j < i; j++)
      err_finish_eot_state:
         for (uint32_t j = 0; j < i; j++)
                        framebuffer->render_count = pass->hw_setup->render_count;
   framebuffer->spm_eot_state_per_render = spm_eot_state_per_render;
                           err_finish_render_targets:
            err_free_ppp_state_bo:
            err_free_framebuffer:
      vk_object_base_finish(&framebuffer->base);
               }
      void pvr_DestroyFramebuffer(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_framebuffer, framebuffer, _fb);
            if (!framebuffer)
            for (uint32_t i = 0; i < framebuffer->render_count; i++) {
      pvr_spm_finish_bgobj_state(device,
            pvr_spm_finish_eot_state(device,
               pvr_spm_scratch_buffer_release(device, framebuffer->scratch_buffer);
   pvr_render_targets_fini(framebuffer->render_targets,
         pvr_bo_suballoc_free(framebuffer->ppp_state_bo);
   vk_object_base_finish(&framebuffer->base);
      }
      PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion)
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
   *       - The ICD must statically expose no other Vulkan symbol unless it
   *         is linked with -Bsymbolic.
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
   */
   *pSupportedVersion = MIN2(*pSupportedVersion, 4u);
      }
      static uint32_t
   pvr_sampler_get_hw_filter_from_vk(const struct pvr_device_info *dev_info,
         {
      switch (filter) {
   case VK_FILTER_NEAREST:
         case VK_FILTER_LINEAR:
         default:
            }
      static uint32_t
   pvr_sampler_get_hw_addr_mode_from_vk(VkSamplerAddressMode addr_mode)
   {
      switch (addr_mode) {
   case VK_SAMPLER_ADDRESS_MODE_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
         case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
         case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
         default:
            }
      VkResult pvr_CreateSampler(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   uint32_t border_color_table_index;
   struct pvr_sampler *sampler;
   float lod_rounding_bias;
   VkFilter min_filter;
   VkFilter mag_filter;
   VkResult result;
   float min_lod;
            STATIC_ASSERT(sizeof(((union pvr_sampler_descriptor *)NULL)->data) ==
            sampler =
         if (!sampler) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               mag_filter = pCreateInfo->magFilter;
            result =
      pvr_border_color_table_get_or_create_entry(&device->border_color_table,
            if (result != VK_SUCCESS)
            if (PVR_HAS_QUIRK(&device->pdevice->dev_info, 51025)) {
      /* The min/mag filters may need adjustment here, the GPU should decide
   * which of the two filters to use based on the clamped LOD value: LOD
   * <= 0 implies magnification, while LOD > 0 implies minification.
   *
   * As a workaround, we override magFilter with minFilter if we know that
   * the magnification filter will never be used due to clamping anyway
   * (i.e. minLod > 0). Conversely, we override minFilter with magFilter
   * if maxLod <= 0.
   */
   if (pCreateInfo->minLod > 0.0f) {
      /* The clamped LOD will always be positive => always minify. */
               if (pCreateInfo->maxLod <= 0.0f) {
      /* The clamped LOD will always be negative or zero => always
   * magnify.
   */
                  if (pCreateInfo->compareEnable) {
      sampler->descriptor.data.compare_op =
      } else {
      sampler->descriptor.data.compare_op =
               sampler->descriptor.data.word3 = 0;
   pvr_csb_pack (&sampler->descriptor.data.sampler_word,
                  const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const float lod_clamp_max = (float)PVRX(TEXSTATE_CLAMP_MAX) /
         const float max_dadjust = ((float)(PVRX(TEXSTATE_DADJUST_MAX_UINT) -
               const float min_dadjust = ((float)(PVRX(TEXSTATE_DADJUST_MIN_UINT) -
                  word.magfilter = pvr_sampler_get_hw_filter_from_vk(dev_info, mag_filter);
            if (pCreateInfo->mipmapMode == VK_SAMPLER_MIPMAP_MODE_LINEAR)
            word.addrmode_u =
         word.addrmode_v =
         word.addrmode_w =
            /* TODO: Figure out defines for these. */
   if (word.addrmode_u == PVRX(TEXSTATE_ADDRMODE_FLIP))
            if (word.addrmode_v == PVRX(TEXSTATE_ADDRMODE_FLIP))
            /* The Vulkan 1.0.205 spec says:
   *
   *    The absolute value of mipLodBias must be less than or equal to
   *    VkPhysicalDeviceLimits::maxSamplerLodBias.
   */
   word.dadjust =
      PVRX(TEXSTATE_DADJUST_ZERO_UINT) +
   util_signed_fixed(
               /* Anisotropy is not supported for now. */
            if (PVR_HAS_QUIRK(&device->pdevice->dev_info, 51025) &&
      pCreateInfo->mipmapMode == VK_SAMPLER_MIPMAP_MODE_NEAREST) {
   /* When MIPMAP_MODE_NEAREST is enabled, the LOD level should be
   * selected by adding 0.5 and then truncating the input LOD value.
   * This hardware adds the 0.5 bias before clamping against
   * lodmin/lodmax, while Vulkan specifies the bias to be added after
   * clamping. We compensate for this difference by adding the 0.5
   * bias to the LOD bounds, too.
   */
      } else {
                  min_lod = pCreateInfo->minLod + lod_rounding_bias;
   word.minlod = util_unsigned_fixed(CLAMP(min_lod, 0.0f, lod_clamp_max),
            max_lod = pCreateInfo->maxLod + lod_rounding_bias;
   word.maxlod = util_unsigned_fixed(CLAMP(max_lod, 0.0f, lod_clamp_max),
                     if (pCreateInfo->unnormalizedCoordinates)
                              err_free_sampler:
            err_out:
         }
      void pvr_DestroySampler(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!sampler)
               }
      void pvr_GetBufferMemoryRequirements2(
      VkDevice _device,
   const VkBufferMemoryRequirementsInfo2 *pInfo,
      {
      PVR_FROM_HANDLE(pvr_buffer, buffer, pInfo->buffer);
   PVR_FROM_HANDLE(pvr_device, device, _device);
            /* The Vulkan 1.0.166 spec says:
   *
   *    memoryTypeBits is a bitmask and contains one bit set for every
   *    supported memory type for the resource. Bit 'i' is set if and only
   *    if the memory type 'i' in the VkPhysicalDeviceMemoryProperties
   *    structure for the physical device is supported for the resource.
   *
   * All types are currently supported for buffers.
   */
   pMemoryRequirements->memoryRequirements.memoryTypeBits =
                              if (size % device->ws->page_size == 0 ||
      size % device->ws->page_size >
         /* TODO: We can save memory by having one extra virtual page mapped
   * in and having the first and last virtual page mapped to the first
   * physical address.
   */
               pMemoryRequirements->memoryRequirements.size =
      }
      void pvr_GetImageMemoryRequirements2(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            /* The Vulkan 1.0.166 spec says:
   *
   *    memoryTypeBits is a bitmask and contains one bit set for every
   *    supported memory type for the resource. Bit 'i' is set if and only
   *    if the memory type 'i' in the VkPhysicalDeviceMemoryProperties
   *    structure for the physical device is supported for the resource.
   *
   * All types are currently supported for images.
   */
   const uint32_t memory_types =
            /* TODO: The returned size is aligned here in case of arrays/CEM (as is done
   * in GetImageMemoryRequirements()), but this should be known at image
   * creation time (pCreateInfo->arrayLayers > 1). This is confirmed in
   * ImageCreate()/ImageGetMipMapOffsetInBytes() where it aligns the size to
   * 4096 if pCreateInfo->arrayLayers > 1. So is the alignment here actually
   * necessary? If not, what should it be when pCreateInfo->arrayLayers == 1?
   *
   * Note: Presumably the 4096 alignment requirement comes from the Vulkan
   * driver setting RGX_CR_TPU_TAG_CEM_4K_FACE_PACKING_EN when setting up
   * render and compute jobs.
   */
   pMemoryRequirements->memoryRequirements.alignment = image->alignment;
   pMemoryRequirements->memoryRequirements.size =
            }
