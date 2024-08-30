   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_device.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "panvk_private.h"
      #include "pan_bo.h"
   #include "pan_encoder.h"
   #include "pan_util.h"
   #include "vk_cmd_enqueue_entrypoints.h"
   #include "vk_common_entrypoints.h"
      #include <fcntl.h>
   #include <libsync.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <xf86drm.h>
   #include <sys/mman.h>
   #include <sys/sysinfo.h>
      #include "drm-uapi/panfrost_drm.h"
      #include "util/disk_cache.h"
   #include "util/strtod.h"
   #include "util/u_debug.h"
   #include "vk_drm_syncobj.h"
   #include "vk_format.h"
   #include "vk_util.h"
      #ifdef VK_USE_PLATFORM_WAYLAND_KHR
   #include "wayland-drm-client-protocol.h"
   #include <wayland-client.h>
   #endif
      #include "panvk_cs.h"
      VkResult
   _panvk_device_set_lost(struct panvk_device *device, const char *file, int line,
         {
      /* Set the flag indicating that waits should return in finite time even
   * after device loss.
   */
            /* TODO: Report the log message through VkDebugReportCallbackEXT instead */
   fprintf(stderr, "%s:%d: ", file, line);
   va_list ap;
   va_start(ap, msg);
   vfprintf(stderr, msg, ap);
            if (debug_get_bool_option("PANVK_ABORT_ON_DEVICE_LOSS", false))
               }
      static int
   panvk_device_get_cache_uuid(uint16_t family, void *uuid)
   {
      uint32_t mesa_timestamp;
            if (!disk_cache_get_function_timestamp(panvk_device_get_cache_uuid,
                  memset(uuid, 0, VK_UUID_SIZE);
   memcpy(uuid, &mesa_timestamp, 4);
   memcpy((char *)uuid + 4, &f, 2);
   snprintf((char *)uuid + 6, VK_UUID_SIZE - 10, "pan");
      }
      static void
   panvk_get_driver_uuid(void *uuid)
   {
      memset(uuid, 0, VK_UUID_SIZE);
      }
      static void
   panvk_get_device_uuid(void *uuid)
   {
         }
      static const struct debug_control panvk_debug_options[] = {
      {"startup", PANVK_DEBUG_STARTUP}, {"nir", PANVK_DEBUG_NIR},
   {"trace", PANVK_DEBUG_TRACE},     {"sync", PANVK_DEBUG_SYNC},
   {"afbc", PANVK_DEBUG_AFBC},       {"linear", PANVK_DEBUG_LINEAR},
         #if defined(VK_USE_PLATFORM_WAYLAND_KHR)
   #define PANVK_USE_WSI_PLATFORM
   #endif
      #define PANVK_API_VERSION VK_MAKE_VERSION(1, 0, VK_HEADER_VERSION)
      VkResult
   panvk_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = PANVK_API_VERSION;
      }
      static const struct vk_instance_extension_table panvk_instance_extensions = {
      .KHR_get_physical_device_properties2 = true,
   .EXT_debug_report = true,
         #ifdef PANVK_USE_WSI_PLATFORM
         #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   };
      static void
   panvk_get_device_extensions(const struct panvk_physical_device *device,
         {
      *ext = (struct vk_device_extension_table){
      .KHR_copy_commands2 = true,
   .KHR_storage_buffer_storage_class = true,
   #ifdef PANVK_USE_WSI_PLATFORM
         #endif
         .KHR_synchronization2 = true,
   .KHR_variable_pointers = true,
   .EXT_custom_border_color = true,
   .EXT_index_type_uint8 = true,
         }
      static void
   panvk_get_features(const struct panvk_physical_device *device,
         {
      *features = (struct vk_features){
      /* Vulkan 1.0 */
   .robustBufferAccess = true,
   .fullDrawIndexUint32 = true,
   .independentBlend = true,
   .logicOp = true,
   .wideLines = true,
   .largePoints = true,
   .textureCompressionETC2 = true,
   .textureCompressionASTC_LDR = true,
   .shaderUniformBufferArrayDynamicIndexing = true,
   .shaderSampledImageArrayDynamicIndexing = true,
   .shaderStorageBufferArrayDynamicIndexing = true,
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess = false,
   .uniformAndStorageBuffer16BitAccess = false,
   .storagePushConstant16 = false,
   .storageInputOutput16 = false,
   .multiview = false,
   .multiviewGeometryShader = false,
   .multiviewTessellationShader = false,
   .variablePointersStorageBuffer = true,
   .variablePointers = true,
   .protectedMemory = false,
   .samplerYcbcrConversion = false,
            /* Vulkan 1.2 */
   .samplerMirrorClampToEdge = false,
   .drawIndirectCount = false,
   .storageBuffer8BitAccess = false,
   .uniformAndStorageBuffer8BitAccess = false,
   .storagePushConstant8 = false,
   .shaderBufferInt64Atomics = false,
   .shaderSharedInt64Atomics = false,
   .shaderFloat16 = false,
            .descriptorIndexing = false,
   .shaderInputAttachmentArrayDynamicIndexing = false,
   .shaderUniformTexelBufferArrayDynamicIndexing = false,
   .shaderStorageTexelBufferArrayDynamicIndexing = false,
   .shaderUniformBufferArrayNonUniformIndexing = false,
   .shaderSampledImageArrayNonUniformIndexing = false,
   .shaderStorageBufferArrayNonUniformIndexing = false,
   .shaderStorageImageArrayNonUniformIndexing = false,
   .shaderInputAttachmentArrayNonUniformIndexing = false,
   .shaderUniformTexelBufferArrayNonUniformIndexing = false,
   .shaderStorageTexelBufferArrayNonUniformIndexing = false,
   .descriptorBindingUniformBufferUpdateAfterBind = false,
   .descriptorBindingSampledImageUpdateAfterBind = false,
   .descriptorBindingStorageImageUpdateAfterBind = false,
   .descriptorBindingStorageBufferUpdateAfterBind = false,
   .descriptorBindingUniformTexelBufferUpdateAfterBind = false,
   .descriptorBindingStorageTexelBufferUpdateAfterBind = false,
   .descriptorBindingUpdateUnusedWhilePending = false,
   .descriptorBindingPartiallyBound = false,
   .descriptorBindingVariableDescriptorCount = false,
            .samplerFilterMinmax = false,
   .scalarBlockLayout = false,
   .imagelessFramebuffer = false,
   .uniformBufferStandardLayout = false,
   .shaderSubgroupExtendedTypes = false,
   .separateDepthStencilLayouts = false,
   .hostQueryReset = false,
   .timelineSemaphore = false,
   .bufferDeviceAddress = false,
   .bufferDeviceAddressCaptureReplay = false,
   .bufferDeviceAddressMultiDevice = false,
   .vulkanMemoryModel = false,
   .vulkanMemoryModelDeviceScope = false,
   .vulkanMemoryModelAvailabilityVisibilityChains = false,
   .shaderOutputViewportIndex = false,
   .shaderOutputLayer = false,
            /* Vulkan 1.3 */
   .robustImageAccess = false,
   .inlineUniformBlock = false,
   .descriptorBindingInlineUniformBlockUpdateAfterBind = false,
   .pipelineCreationCacheControl = false,
   .privateData = true,
   .shaderDemoteToHelperInvocation = false,
   .shaderTerminateInvocation = false,
   .subgroupSizeControl = false,
   .computeFullSubgroups = false,
   .synchronization2 = true,
   .textureCompressionASTC_HDR = false,
   .shaderZeroInitializeWorkgroupMemory = false,
   .dynamicRendering = false,
   .shaderIntegerDotProduct = false,
            /* VK_EXT_index_type_uint8 */
            /* VK_EXT_vertex_attribute_divisor */
   .vertexAttributeInstanceRateDivisor = true,
            /* VK_EXT_depth_clip_enable */
            /* VK_EXT_4444_formats */
   .formatA4R4G4B4 = true,
            /* VK_EXT_custom_border_color */
   .customBorderColors = true,
         }
      VkResult panvk_physical_device_try_create(struct vk_instance *vk_instance,
                  static void
   panvk_physical_device_finish(struct panvk_physical_device *device)
   {
               panvk_arch_dispatch(device->pdev.arch, meta_cleanup, device);
   panfrost_close_device(&device->pdev);
   if (device->master_fd != -1)
               }
      static void
   panvk_destroy_physical_device(struct vk_physical_device *device)
   {
      panvk_physical_device_finish((struct panvk_physical_device *)device);
      }
      VkResult
   panvk_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
      struct panvk_instance *instance;
                     pAllocator = pAllocator ?: vk_default_allocator();
   instance = vk_zalloc(pAllocator, sizeof(*instance), 8,
         if (!instance)
                     vk_instance_dispatch_table_from_entrypoints(
         vk_instance_dispatch_table_from_entrypoints(
         result = vk_instance_init(&instance->vk, &panvk_instance_extensions,
         if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
               instance->vk.physical_devices.try_create_for_drm =
                  instance->debug_flags =
            if (instance->debug_flags & PANVK_DEBUG_STARTUP)
                                 }
      void
   panvk_DestroyInstance(VkInstance _instance,
         {
               if (!instance)
            vk_instance_finish(&instance->vk);
      }
      static VkResult
   panvk_physical_device_init(struct panvk_physical_device *device,
               {
      const char *path = drm_device->nodes[DRM_NODE_RENDER];
   VkResult result = VK_SUCCESS;
   drmVersionPtr version;
   int fd;
            if (!getenv("PAN_I_WANT_A_BROKEN_VULKAN_DRIVER")) {
      return vk_errorf(
      instance, VK_ERROR_INCOMPATIBLE_DRIVER,
   "WARNING: panvk is not a conformant vulkan implementation, "
            fd = open(path, O_RDWR | O_CLOEXEC);
   if (fd < 0) {
      return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
               version = drmGetVersion(fd);
   if (!version) {
      close(fd);
   return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                     if (strcmp(version->name, "panfrost")) {
      drmFreeVersion(version);
   close(fd);
   return vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                              if (instance->debug_flags & PANVK_DEBUG_STARTUP)
            struct vk_device_extension_table supported_extensions;
            struct vk_features supported_features;
            struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(
         vk_physical_device_dispatch_table_from_entrypoints(
            result =
      vk_physical_device_init(&device->vk, &instance->vk, &supported_extensions,
         if (result != VK_SUCCESS) {
      vk_error(instance, result);
                        if (instance->vk.enabled_extensions.KHR_display) {
      master_fd = open(drm_device->nodes[DRM_NODE_PRIMARY], O_RDWR | O_CLOEXEC);
   if (master_fd >= 0) {
                     device->master_fd = master_fd;
   if (instance->debug_flags & PANVK_DEBUG_TRACE)
            device->pdev.debug |= PAN_DBG_NO_CACHE;
   panfrost_open_device(NULL, fd, &device->pdev);
            if (device->pdev.arch <= 5 || device->pdev.arch >= 8) {
      result = vk_errorf(instance, VK_ERROR_INCOMPATIBLE_DRIVER,
                              memset(device->name, 0, sizeof(device->name));
            if (panvk_device_get_cache_uuid(device->pdev.gpu_id, device->cache_uuid)) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                              panvk_get_driver_uuid(&device->device_uuid);
            device->drm_syncobj_type = vk_drm_syncobj_get_type(device->pdev.fd);
   /* We don't support timelines in the uAPI yet and we don't want it getting
   * suddenly turned on by vk_drm_syncobj_get_type() without us adding panvk
   * code for it first.
   */
            device->sync_types[0] = &device->drm_syncobj_type;
   device->sync_types[1] = NULL;
            result = panvk_wsi_init(device);
   if (result != VK_SUCCESS) {
      vk_error(instance, result);
                     fail_close_device:
         fail:
      if (fd != -1)
         if (master_fd != -1)
            }
      VkResult
   panvk_physical_device_try_create(struct vk_instance *vk_instance,
               {
      struct panvk_instance *instance =
            if (!(drm_device->available_nodes & (1 << DRM_NODE_RENDER)) ||
      drm_device->bustype != DRM_BUS_PLATFORM)
         struct panvk_physical_device *device =
      vk_zalloc(&instance->vk.alloc, sizeof(*device), 8,
      if (!device)
            VkResult result = panvk_physical_device_init(device, instance, drm_device);
   if (result != VK_SUCCESS) {
      vk_free(&instance->vk.alloc, device);
               *out = &device->vk;
      }
      void
   panvk_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
         {
               VkSampleCountFlags sample_counts =
            /* make sure that the entire descriptor set is addressable with a signed
   * 32-bit int. So the sum of all limits scaled by descriptor size has to
   * be at most 2 GiB. the combined image & samples object count as one of
   * both. This limit is for the pipeline layout, not for the set layout, but
   * there is no set limit, so we just set a pipeline limit. I don't think
   * any app is going to hit this soon. */
   size_t max_descriptor_set_size =
      ((1ull << 31) - 16 * MAX_DYNAMIC_BUFFERS) /
   (32 /* uniform buffer, 32 due to potential space wasted on alignment */ +
   32 /* storage buffer, 32 due to potential space wasted on alignment */ +
   32 /* sampler, largest when combined with image */ +
         const VkPhysicalDeviceLimits limits = {
      .maxImageDimension1D = (1 << 14),
   .maxImageDimension2D = (1 << 14),
   .maxImageDimension3D = (1 << 11),
   .maxImageDimensionCube = (1 << 14),
   .maxImageArrayLayers = (1 << 11),
   .maxTexelBufferElements = 128 * 1024 * 1024,
   .maxUniformBufferRange = UINT32_MAX,
   .maxStorageBufferRange = UINT32_MAX,
   .maxPushConstantsSize = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount = UINT32_MAX,
   .maxSamplerAllocationCount = 64 * 1024,
   .bufferImageGranularity = 64,          /* A cache line */
   .sparseAddressSpaceSize = 0xffffffffu, /* buffer max size */
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
   .maxVertexInputAttributes = 32,
   .maxVertexInputBindings = 32,
   .maxVertexInputAttributeOffset = 2047,
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
   .maxFragmentCombinedOutputResources =
         .maxComputeSharedMemorySize = 32768,
   .maxComputeWorkGroupCount = {65535, 65535, 65535},
   .maxComputeWorkGroupInvocations = 2048,
   .maxComputeWorkGroupSize = {2048, 2048, 2048},
   .subPixelPrecisionBits = 4 /* FIXME */,
   .subTexelPrecisionBits = 4 /* FIXME */,
   .mipmapPrecisionBits = 4 /* FIXME */,
   .maxDrawIndexedIndexValue = UINT32_MAX,
   .maxDrawIndirectCount = UINT32_MAX,
   .maxSamplerLodBias = 16,
   .maxSamplerAnisotropy = 16,
   .maxViewports = MAX_VIEWPORTS,
   .maxViewportDimensions = {(1 << 14), (1 << 14)},
   .viewportBoundsRange = {INT16_MIN, INT16_MAX},
   .viewportSubPixelBits = 8,
   .minMemoryMapAlignment = 4096, /* A page */
   .minTexelBufferOffsetAlignment = 64,
   .minUniformBufferOffsetAlignment = 16,
   .minStorageBufferOffsetAlignment = 4,
   .minTexelOffset = -32,
   .maxTexelOffset = 31,
   .minTexelGatherOffset = -32,
   .maxTexelGatherOffset = 31,
   .minInterpolationOffset = -2,
   .maxInterpolationOffset = 2,
   .subPixelInterpolationOffsetBits = 8,
   .maxFramebufferWidth = (1 << 14),
   .maxFramebufferHeight = (1 << 14),
   .maxFramebufferLayers = (1 << 10),
   .framebufferColorSampleCounts = sample_counts,
   .framebufferDepthSampleCounts = sample_counts,
   .framebufferStencilSampleCounts = sample_counts,
   .framebufferNoAttachmentsSampleCounts = sample_counts,
   .maxColorAttachments = MAX_RTS,
   .sampledImageColorSampleCounts = sample_counts,
   .sampledImageIntegerSampleCounts = VK_SAMPLE_COUNT_1_BIT,
   .sampledImageDepthSampleCounts = sample_counts,
   .sampledImageStencilSampleCounts = sample_counts,
   .storageImageSampleCounts = VK_SAMPLE_COUNT_1_BIT,
   .maxSampleMaskWords = 1,
   .timestampComputeAndGraphics = true,
   .timestampPeriod = 1,
   .maxClipDistances = 8,
   .maxCullDistances = 8,
   .maxCombinedClipAndCullDistances = 8,
   .discreteQueuePriorities = 1,
   .pointSizeRange = {0.125, 4095.9375},
   .lineWidthRange = {0.0, 7.9921875},
   .pointSizeGranularity = (1.0 / 16.0),
   .lineWidthGranularity = (1.0 / 128.0),
   .strictLines = false, /* FINISHME */
   .standardSampleLocations = true,
   .optimalBufferCopyOffsetAlignment = 128,
   .optimalBufferCopyRowPitchAlignment = 128,
               pProperties->properties = (VkPhysicalDeviceProperties){
      .apiVersion = PANVK_API_VERSION,
   .driverVersion = vk_get_driver_version(),
   .vendorID = 0, /* TODO */
   .deviceID = 0,
   .deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
   .limits = limits,
               strcpy(pProperties->properties.deviceName, pdevice->name);
   memcpy(pProperties->properties.pipelineCacheUUID, pdevice->cache_uuid,
            VkPhysicalDeviceVulkan11Properties core_1_1 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
   .deviceLUIDValid = false,
   .pointClippingBehavior = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES,
   .maxMultiviewViewCount = 0,
   .maxMultiviewInstanceIndex = 0,
   .protectedNoFault = false,
   /* Make sure everything is addressable by a signed 32-bit int, and
   * our largest descriptors are 96 bytes. */
   .maxPerSetDescriptors = (1ull << 31) / 96,
   /* Our buffer size fields allow only this much */
      };
   memcpy(core_1_1.driverUUID, pdevice->driver_uuid, VK_UUID_SIZE);
            const VkPhysicalDeviceVulkan12Properties core_1_2 = {
                  const VkPhysicalDeviceVulkan13Properties core_1_3 = {
                  vk_foreach_struct(ext, pProperties->pNext) {
      if (vk_get_physical_device_core_1_1_property_ext(ext, &core_1_1))
         if (vk_get_physical_device_core_1_2_property_ext(ext, &core_1_2))
         if (vk_get_physical_device_core_1_3_property_ext(ext, &core_1_3))
            switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PUSH_DESCRIPTOR_PROPERTIES_KHR: {
      VkPhysicalDevicePushDescriptorPropertiesKHR *properties =
         properties->maxPushDescriptors = MAX_PUSH_DESCRIPTORS;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT: {
      VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *properties =
         /* We have to restrict this a bit for multiview */
   properties->maxVertexAttribDivisor = UINT32_MAX / (16 * 2048);
      }
   default:
               }
      static const VkQueueFamilyProperties panvk_queue_family_properties = {
      .queueFlags =
         .queueCount = 1,
   .timestampValidBits = 64,
      };
      void
   panvk_GetPhysicalDeviceQueueFamilyProperties2(
      VkPhysicalDevice physicalDevice, uint32_t *pQueueFamilyPropertyCount,
      {
      VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out, pQueueFamilyProperties,
            vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p)
   {
            }
      static uint64_t
   panvk_get_system_heap_size()
   {
      struct sysinfo info;
                     /* We don't want to burn too much ram with the GPU.  If the user has 4GiB
   * or less, we use at most half.  If they have more than 4GiB, we use 3/4.
   */
   uint64_t available_ram;
   if (total_ram <= 4ull * 1024 * 1024 * 1024)
         else
               }
      void
   panvk_GetPhysicalDeviceMemoryProperties2(
      VkPhysicalDevice physicalDevice,
      {
      pMemoryProperties->memoryProperties = (VkPhysicalDeviceMemoryProperties){
      .memoryHeapCount = 1,
   .memoryHeaps[0].size = panvk_get_system_heap_size(),
   .memoryHeaps[0].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
   .memoryTypeCount = 1,
   .memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                     }
      static VkResult
   panvk_queue_init(struct panvk_device *device, struct panvk_queue *queue,
         {
               VkResult result = vk_queue_init(&queue->vk, &device->vk, create_info, idx);
   if (result != VK_SUCCESS)
                  struct drm_syncobj_create create = {
                  int ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_CREATE, &create);
   if (ret) {
      vk_queue_finish(&queue->vk);
               switch (pdev->arch) {
   case 6:
      queue->vk.driver_submit = panvk_v6_queue_submit;
      case 7:
      queue->vk.driver_submit = panvk_v7_queue_submit;
      default:
                  queue->sync = create.handle;
      }
      static void
   panvk_queue_finish(struct panvk_queue *queue)
   {
         }
      VkResult
   panvk_CreateDevice(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(panvk_physical_device, physical_device, physicalDevice);
   VkResult result;
            device = vk_zalloc2(&physical_device->instance->vk.alloc, pAllocator,
         if (!device)
            const struct vk_device_entrypoint_table *dev_entrypoints;
   const struct vk_command_buffer_ops *cmd_buffer_ops;
            switch (physical_device->pdev.arch) {
   case 6:
      dev_entrypoints = &panvk_v6_device_entrypoints;
   cmd_buffer_ops = &panvk_v6_cmd_buffer_ops;
      case 7:
      dev_entrypoints = &panvk_v7_device_entrypoints;
   cmd_buffer_ops = &panvk_v7_cmd_buffer_ops;
      default:
                  /* For secondary command buffer support, overwrite any command entrypoints
   * in the main device-level dispatch table with
   * vk_cmd_enqueue_unless_primary_Cmd*.
   */
   vk_device_dispatch_table_from_entrypoints(
            vk_device_dispatch_table_from_entrypoints(&dispatch_table, dev_entrypoints,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
            /* Populate our primary cmd_dispatch table. */
   vk_device_dispatch_table_from_entrypoints(&device->cmd_dispatch,
         vk_device_dispatch_table_from_entrypoints(&device->cmd_dispatch,
         vk_device_dispatch_table_from_entrypoints(
            result = vk_device_init(&device->vk, &physical_device->vk, &dispatch_table,
         if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, device);
               /* Must be done after vk_device_init() because this function memset(0) the
   * whole struct.
   */
   device->vk.command_dispatch_table = &device->cmd_dispatch;
            device->instance = physical_device->instance;
            const struct panfrost_device *pdev = &physical_device->pdev;
            for (unsigned i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      const VkDeviceQueueCreateInfo *queue_create =
         uint32_t qfi = queue_create->queueFamilyIndex;
   device->queues[qfi] =
      vk_alloc(&device->vk.alloc,
            if (!device->queues[qfi]) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               memset(device->queues[qfi], 0,
                     for (unsigned q = 0; q < queue_create->queueCount; q++) {
      result =
         if (result != VK_SUCCESS)
                  *pDevice = panvk_device_to_handle(device);
         fail:
      for (unsigned i = 0; i < PANVK_MAX_QUEUE_FAMILIES; i++) {
      for (unsigned q = 0; q < device->queue_count[i]; q++)
         if (device->queue_count[i])
               vk_free(&device->vk.alloc, device);
      }
      void
   panvk_DestroyDevice(VkDevice _device, const VkAllocationCallbacks *pAllocator)
   {
               if (!device)
            for (unsigned i = 0; i < PANVK_MAX_QUEUE_FAMILIES; i++) {
      for (unsigned q = 0; q < device->queue_count[i]; q++)
         if (device->queue_count[i])
                  }
      VkResult
   panvk_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
         {
      *pPropertyCount = 0;
      }
      VkResult
   panvk_QueueWaitIdle(VkQueue _queue)
   {
               if (panvk_device_is_lost(queue->device))
            const struct panfrost_device *pdev = &queue->device->physical_device->pdev;
   struct drm_syncobj_wait wait = {
      .handles = (uint64_t)(uintptr_t)(&queue->sync),
   .count_handles = 1,
   .timeout_nsec = INT64_MAX,
      };
            ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_WAIT, &wait);
               }
      VkResult
   panvk_EnumerateInstanceExtensionProperties(const char *pLayerName,
               {
      if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      PFN_vkVoidFunction
   panvk_GetInstanceProcAddr(VkInstance _instance, const char *pName)
   {
      VK_FROM_HANDLE(panvk_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk, &panvk_instance_entrypoints,
      }
      /* The loader wants us to expose a second GetInstanceProcAddr function
   * to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName);
      PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance, const char *pName)
   {
         }
      /* With version 4+ of the loader interface the ICD should expose
   * vk_icdGetPhysicalDeviceProcAddr()
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName);
      PFN_vkVoidFunction
   vk_icdGetPhysicalDeviceProcAddr(VkInstance _instance, const char *pName)
   {
                  }
      VkResult
   panvk_AllocateMemory(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     if (pAllocateInfo->allocationSize == 0) {
      /* Apparently, this is allowed */
   *pMem = VK_NULL_HANDLE;
               mem = vk_object_alloc(&device->vk, pAllocator, sizeof(*mem),
         if (mem == NULL)
            const VkImportMemoryFdInfoKHR *fd_info =
            if (fd_info && !fd_info->handleType)
            if (fd_info) {
      assert(
                  /*
   * TODO Importing the same fd twice gives us the same handle without
   * reference counting.  We need to maintain a per-instance handle-to-bo
   * table and add reference count to panvk_bo.
   */
   mem->bo = panfrost_bo_import(&device->physical_device->pdev, fd_info->fd);
   /* take ownership and close the fd */
      } else {
      mem->bo = panfrost_bo_create(&device->physical_device->pdev,
                                          }
      void
   panvk_FreeMemory(VkDevice _device, VkDeviceMemory _mem,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (mem == NULL)
            panfrost_bo_unreference(mem->bo);
      }
      VkResult
   panvk_MapMemory(VkDevice _device, VkDeviceMemory _memory, VkDeviceSize offset,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (mem == NULL) {
      *ppData = NULL;
               if (!mem->bo->ptr.cpu)
                     if (*ppData) {
      *ppData += offset;
                  }
      void
   panvk_UnmapMemory(VkDevice _device, VkDeviceMemory _memory)
   {
   }
      VkResult
   panvk_FlushMappedMemoryRanges(VkDevice _device, uint32_t memoryRangeCount,
         {
         }
      VkResult
   panvk_InvalidateMappedMemoryRanges(VkDevice _device, uint32_t memoryRangeCount,
         {
         }
      void
   panvk_GetBufferMemoryRequirements2(VkDevice device,
               {
               const uint64_t align = 64;
            pMemoryRequirements->memoryRequirements.memoryTypeBits = 1;
   pMemoryRequirements->memoryRequirements.alignment = align;
      }
      void
   panvk_GetImageMemoryRequirements2(VkDevice device,
               {
               const uint64_t align = 4096;
            pMemoryRequirements->memoryRequirements.memoryTypeBits = 1;
   pMemoryRequirements->memoryRequirements.alignment = align;
      }
      void
   panvk_GetImageSparseMemoryRequirements2(
      VkDevice device, const VkImageSparseMemoryRequirementsInfo2 *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
         }
      void
   panvk_GetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory,
         {
         }
      VkResult
   panvk_BindBufferMemory2(VkDevice device, uint32_t bindInfoCount,
         {
      for (uint32_t i = 0; i < bindInfoCount; ++i) {
      VK_FROM_HANDLE(panvk_device_memory, mem, pBindInfos[i].memory);
            if (mem) {
      buffer->bo = mem->bo;
      } else {
            }
      }
      VkResult
   panvk_BindImageMemory2(VkDevice device, uint32_t bindInfoCount,
         {
      for (uint32_t i = 0; i < bindInfoCount; ++i) {
      VK_FROM_HANDLE(panvk_image, image, pBindInfos[i].image);
            if (mem) {
      image->pimage.data.bo = mem->bo;
   image->pimage.data.offset = pBindInfos[i].memoryOffset;
   /* Reset the AFBC headers */
   if (drm_is_afbc(image->pimage.layout.modifier)) {
                     for (unsigned layer = 0; layer < image->pimage.layout.array_size;
      layer++) {
   for (unsigned level = 0; level < image->pimage.layout.nr_slices;
      level++) {
   void *header = base +
               memset(header, 0,
               } else {
      image->pimage.data.bo = NULL;
                     }
      VkResult
   panvk_CreateEvent(VkDevice _device, const VkEventCreateInfo *pCreateInfo,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
   const struct panfrost_device *pdev = &device->physical_device->pdev;
   struct panvk_event *event = vk_object_zalloc(
         if (!event)
            struct drm_syncobj_create create = {
                  int ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_CREATE, &create);
   if (ret)
            event->syncobj = create.handle;
               }
      void
   panvk_DestroyEvent(VkDevice _device, VkEvent _event,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
   VK_FROM_HANDLE(panvk_event, event, _event);
            if (!event)
            struct drm_syncobj_destroy destroy = {.handle = event->syncobj};
               }
      VkResult
   panvk_GetEventStatus(VkDevice _device, VkEvent _event)
   {
      VK_FROM_HANDLE(panvk_device, device, _device);
   VK_FROM_HANDLE(panvk_event, event, _event);
   const struct panfrost_device *pdev = &device->physical_device->pdev;
            struct drm_syncobj_wait wait = {
      .handles = (uintptr_t)&event->syncobj,
   .count_handles = 1,
   .timeout_nsec = 0,
               int ret = drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_WAIT, &wait);
   if (ret) {
      if (errno == ETIME)
         else {
      assert(0);
         } else
               }
      VkResult
   panvk_SetEvent(VkDevice _device, VkEvent _event)
   {
      VK_FROM_HANDLE(panvk_device, device, _device);
   VK_FROM_HANDLE(panvk_event, event, _event);
            struct drm_syncobj_array objs = {
      .handles = (uint64_t)(uintptr_t)&event->syncobj,
         /* This is going to just replace the fence for this syncobj with one that
   * is already in signaled state. This won't be a problem because the spec
   * mandates that the event will have been set before the vkCmdWaitEvents
   * command executes.
   * https://www.khronos.org/registry/vulkan/specs/1.2/html/chap6.html#commandbuffers-submission-progress
   */
   if (drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_SIGNAL, &objs))
               }
      VkResult
   panvk_ResetEvent(VkDevice _device, VkEvent _event)
   {
      VK_FROM_HANDLE(panvk_device, device, _device);
   VK_FROM_HANDLE(panvk_event, event, _event);
            struct drm_syncobj_array objs = {
      .handles = (uint64_t)(uintptr_t)&event->syncobj,
         if (drmIoctl(pdev->fd, DRM_IOCTL_SYNCOBJ_RESET, &objs))
               }
      VkResult
   panvk_CreateBuffer(VkDevice _device, const VkBufferCreateInfo *pCreateInfo,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     buffer =
         if (buffer == NULL)
                        }
      void
   panvk_DestroyBuffer(VkDevice _device, VkBuffer _buffer,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!buffer)
               }
      VkResult
   panvk_CreateFramebuffer(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     size_t size = sizeof(*framebuffer) + sizeof(struct panvk_attachment_info) *
         framebuffer = vk_object_alloc(&device->vk, pAllocator, size,
         if (framebuffer == NULL)
            framebuffer->attachment_count = pCreateInfo->attachmentCount;
   framebuffer->width = pCreateInfo->width;
   framebuffer->height = pCreateInfo->height;
   framebuffer->layers = pCreateInfo->layers;
   for (uint32_t i = 0; i < pCreateInfo->attachmentCount; i++) {
      VkImageView _iview = pCreateInfo->pAttachments[i];
   struct panvk_image_view *iview = panvk_image_view_from_handle(_iview);
               *pFramebuffer = panvk_framebuffer_to_handle(framebuffer);
      }
      void
   panvk_DestroyFramebuffer(VkDevice _device, VkFramebuffer _fb,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (fb)
      }
      void
   panvk_DestroySampler(VkDevice _device, VkSampler _sampler,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!sampler)
               }
      /* vk_icd.h does not declare this function, so we declare it here to
   * suppress Wmissing-prototypes.
   */
   PUBLIC VKAPI_ATTR VkResult VKAPI_CALL
   vk_icdNegotiateLoaderICDInterfaceVersion(uint32_t *pSupportedVersion);
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
   * is linked with -Bsymbolic.
   *       - Each dispatchable Vulkan handle created by the ICD must be
   *         a pointer to a struct whose first member is VK_LOADER_DATA. The
   *         ICD must initialize VK_LOADER_DATA.loadMagic to
   * ICD_LOADER_MAGIC.
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
   *        - The ICD must support 1.1 and must not return
   *          VK_ERROR_INCOMPATIBLE_DRIVER from vkCreateInstance() unless a
   *          Vulkan Loader with interface v4 or smaller is being used and the
   *          application provides an API version that is greater than 1.0.
   */
   *pSupportedVersion = MIN2(*pSupportedVersion, 5u);
      }
      VkResult
   panvk_GetMemoryFdKHR(VkDevice _device, const VkMemoryGetFdInfoKHR *pGetFdInfo,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
                     /* At the moment, we support only the below handle types. */
   assert(
      pGetFdInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
         int prime_fd = panfrost_bo_export(memory->bo);
   if (prime_fd < 0)
            *pFd = prime_fd;
      }
      VkResult
   panvk_GetMemoryFdPropertiesKHR(VkDevice _device,
                     {
      assert(handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT);
   pMemoryFdProperties->memoryTypeBits = 1;
      }
      void
   panvk_GetPhysicalDeviceExternalSemaphoreProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalSemaphoreInfo *pExternalSemaphoreInfo,
      {
      if ((pExternalSemaphoreInfo->handleType ==
            pExternalSemaphoreInfo->handleType ==
         pExternalSemaphoreProperties->exportFromImportedHandleTypes =
      VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT |
      pExternalSemaphoreProperties->compatibleHandleTypes =
      VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT |
      pExternalSemaphoreProperties->externalSemaphoreFeatures =
      VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT |
   } else {
      pExternalSemaphoreProperties->exportFromImportedHandleTypes = 0;
   pExternalSemaphoreProperties->compatibleHandleTypes = 0;
         }
      void
   panvk_GetPhysicalDeviceExternalFenceProperties(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceExternalFenceInfo *pExternalFenceInfo,
      {
      pExternalFenceProperties->exportFromImportedHandleTypes = 0;
   pExternalFenceProperties->compatibleHandleTypes = 0;
      }
