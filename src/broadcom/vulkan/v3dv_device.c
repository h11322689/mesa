   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
   #include <fcntl.h>
   #include <stdbool.h>
   #include <string.h>
   #include <sys/mman.h>
   #include <sys/sysinfo.h>
   #include <unistd.h>
   #include <xf86drm.h>
      #ifdef MAJOR_IN_MKDEV
   #include <sys/mkdev.h>
   #endif
   #ifdef MAJOR_IN_SYSMACROS
   #include <sys/sysmacros.h>
   #endif
      #include "v3dv_private.h"
      #include "common/v3d_debug.h"
      #include "compiler/v3d_compiler.h"
      #include "drm-uapi/v3d_drm.h"
   #include "vk_drm_syncobj.h"
   #include "vk_util.h"
   #include "git_sha1.h"
      #include "util/build_id.h"
   #include "util/os_file.h"
   #include "util/u_debug.h"
   #include "util/format/u_format.h"
      #ifdef ANDROID
   #include "vk_android.h"
   #endif
      #ifdef VK_USE_PLATFORM_XCB_KHR
   #include <xcb/xcb.h>
   #include <xcb/dri3.h>
   #include <X11/Xlib-xcb.h>
   #endif
      #ifdef VK_USE_PLATFORM_WAYLAND_KHR
   #include <wayland-client.h>
   #include "wayland-drm-client-protocol.h"
   #endif
      #define V3DV_API_VERSION VK_MAKE_VERSION(1, 2, VK_HEADER_VERSION)
      #ifdef ANDROID
   #if ANDROID_API_LEVEL <= 32
   /* Android 12.1 and lower support only Vulkan API v1.1 */
   #undef V3DV_API_VERSION
   #define V3DV_API_VERSION VK_MAKE_VERSION(1, 1, VK_HEADER_VERSION)
   #endif
   #endif
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = V3DV_API_VERSION;
      }
      #if defined(VK_USE_PLATFORM_WIN32_KHR) ||   \
      defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
   defined(VK_USE_PLATFORM_XCB_KHR) ||     \
   defined(VK_USE_PLATFORM_XLIB_KHR) ||    \
      #define V3DV_USE_WSI_PLATFORM
   #endif
      static const struct vk_instance_extension_table instance_extensions = {
         #ifdef VK_USE_PLATFORM_DISPLAY_KHR
      .KHR_display                         = true,
   .KHR_get_display_properties2         = true,
   .EXT_direct_mode_display             = true,
      #endif
      .KHR_external_fence_capabilities     = true,
   .KHR_external_memory_capabilities    = true,
   .KHR_external_semaphore_capabilities = true,
      #ifdef V3DV_USE_WSI_PLATFORM
      .KHR_get_surface_capabilities2       = true,
   .KHR_surface                         = true,
      #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
         #endif
      .EXT_debug_report                    = true,
      };
      static void
   get_device_extensions(const struct v3dv_physical_device *device,
         {
      *ext = (struct vk_device_extension_table) {
      .KHR_8bit_storage                     = true,
   .KHR_16bit_storage                    = true,
   .KHR_bind_memory2                     = true,
   .KHR_buffer_device_address            = true,
   .KHR_copy_commands2                   = true,
   .KHR_create_renderpass2               = true,
   .KHR_dedicated_allocation             = true,
   .KHR_device_group                     = true,
   .KHR_driver_properties                = true,
   .KHR_descriptor_update_template       = true,
   .KHR_depth_stencil_resolve            = true,
   .KHR_external_fence                   = true,
   .KHR_external_fence_fd                = true,
   .KHR_external_memory                  = true,
   .KHR_external_memory_fd               = true,
   .KHR_external_semaphore               = true,
   .KHR_external_semaphore_fd            = true,
   .KHR_format_feature_flags2            = true,
   .KHR_get_memory_requirements2         = true,
   .KHR_image_format_list                = true,
   .KHR_imageless_framebuffer            = true,
   .KHR_performance_query                = device->caps.perfmon,
   .KHR_relaxed_block_layout             = true,
   .KHR_maintenance1                     = true,
   .KHR_maintenance2                     = true,
   .KHR_maintenance3                     = true,
   .KHR_maintenance4                     = true,
   .KHR_multiview                        = true,
   .KHR_pipeline_executable_properties   = true,
   .KHR_separate_depth_stencil_layouts   = true,
   .KHR_shader_float_controls            = true,
   .KHR_shader_non_semantic_info         = true,
   .KHR_sampler_mirror_clamp_to_edge     = true,
   .KHR_sampler_ycbcr_conversion         = true,
   .KHR_spirv_1_4                        = true,
   .KHR_storage_buffer_storage_class     = true,
   .KHR_timeline_semaphore               = true,
   .KHR_uniform_buffer_standard_layout   = true,
   .KHR_shader_integer_dot_product       = true,
   .KHR_synchronization2                 = true,
   #ifdef V3DV_USE_WSI_PLATFORM
         .KHR_swapchain                        = true,
   .KHR_swapchain_mutable_format         = true,
   #endif
         .KHR_variable_pointers                = true,
   .KHR_vulkan_memory_model              = true,
   .KHR_zero_initialize_workgroup_memory = true,
   .EXT_4444_formats                     = true,
   .EXT_attachment_feedback_loop_layout  = true,
   .EXT_border_color_swizzle             = true,
   .EXT_color_write_enable               = true,
   .EXT_custom_border_color              = true,
   .EXT_depth_clip_control               = true,
   .EXT_load_store_op_none               = true,
   .EXT_inline_uniform_block             = true,
   .EXT_external_memory_dma_buf          = true,
   .EXT_host_query_reset                 = true,
   .EXT_image_drm_format_modifier        = true,
   .EXT_image_robustness                 = true,
   .EXT_index_type_uint8                 = true,
   .EXT_line_rasterization               = true,
   .EXT_memory_budget                    = true,
   .EXT_physical_device_drm              = true,
   .EXT_pipeline_creation_cache_control  = true,
   .EXT_pipeline_creation_feedback       = true,
   .EXT_pipeline_robustness              = true,
   .EXT_primitive_topology_list_restart  = true,
   .EXT_private_data                     = true,
   .EXT_provoking_vertex                 = true,
   .EXT_separate_stencil_usage           = true,
   .EXT_shader_module_identifier         = true,
   .EXT_texel_buffer_alignment           = true,
   .EXT_tooling_info                     = true,
   #ifdef ANDROID
         .ANDROID_external_memory_android_hardware_buffer = true,
   .ANDROID_native_buffer                = true,
   #endif
         }
      static void
   get_features(const struct v3dv_physical_device *physical_device,
         {
      *features = (struct vk_features) {
      /* Vulkan 1.0 */
   .robustBufferAccess = true, /* This feature is mandatory */
   .fullDrawIndexUint32 = physical_device->devinfo.ver >= 71,
   .imageCubeArray = true,
   .independentBlend = true,
   .geometryShader = true,
   .tessellationShader = false,
   .sampleRateShading = true,
   .dualSrcBlend = false,
   .logicOp = true,
   .multiDrawIndirect = false,
   .drawIndirectFirstInstance = true,
   .depthClamp = physical_device->devinfo.ver >= 71,
   .depthBiasClamp = true,
   .fillModeNonSolid = true,
   .depthBounds = physical_device->devinfo.ver >= 71,
   .wideLines = true,
   .largePoints = true,
   .alphaToOne = true,
   .multiViewport = false,
   .samplerAnisotropy = true,
   .textureCompressionETC2 = true,
   .textureCompressionASTC_LDR = true,
   /* Note that textureCompressionBC requires that the driver support all
   * the BC formats. V3D 4.2 only support the BC1-3, so we can't claim
   * that we support it.
   */
   .textureCompressionBC = false,
   .occlusionQueryPrecise = true,
   .pipelineStatisticsQuery = false,
   .vertexPipelineStoresAndAtomics = true,
   .fragmentStoresAndAtomics = true,
   .shaderTessellationAndGeometryPointSize = true,
   .shaderImageGatherExtended = true,
   .shaderStorageImageExtendedFormats = true,
   .shaderStorageImageMultisample = false,
   .shaderStorageImageReadWithoutFormat = true,
   .shaderStorageImageWriteWithoutFormat = false,
   .shaderUniformBufferArrayDynamicIndexing = false,
   .shaderSampledImageArrayDynamicIndexing = false,
   .shaderStorageBufferArrayDynamicIndexing = false,
   .shaderStorageImageArrayDynamicIndexing = false,
   .shaderClipDistance = true,
   .shaderCullDistance = false,
   .shaderFloat64 = false,
   .shaderInt64 = false,
   .shaderInt16 = false,
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
            /* Vulkan 1.1 */
   .storageBuffer16BitAccess = true,
   .uniformAndStorageBuffer16BitAccess = true,
   .storagePushConstant16 = true,
   .storageInputOutput16 = false,
   .multiview = true,
   .multiviewGeometryShader = false,
   .multiviewTessellationShader = false,
   .variablePointersStorageBuffer = true,
   /* FIXME: this needs support for non-constant index on UBO/SSBO */
   .variablePointers = false,
   .protectedMemory = false,
   .samplerYcbcrConversion = true,
            /* Vulkan 1.2 */
   .hostQueryReset = true,
   .uniformAndStorageBuffer8BitAccess = true,
   .uniformBufferStandardLayout = true,
   /* V3D 4.2 wraps TMU vector accesses to 16-byte boundaries, so loads and
   * stores of vectors that cross these boundaries would not work correctly
   * with scalarBlockLayout and would need to be split into smaller vectors
   * (and/or scalars) that don't cross these boundaries. For load/stores
   * with dynamic offsets where we can't identify if the offset is
   * problematic, we would always have to scalarize. Overall, this would
   * not lead to best performance so let's just not support it.
   */
   .scalarBlockLayout = physical_device->devinfo.ver >= 71,
   /* This tells applications 2 things:
   *
   * 1. If they can select just one aspect for barriers. For us barriers
   *    decide if we need to split a job and we don't care if it is only
   *    for one of the aspects of the image or both, so we don't really
   *    benefit from seeing barriers that select just one aspect.
   *
   * 2. If they can program different layouts for each aspect. We
   *    generally don't care about layouts, so again, we don't get any
   *    benefits from this to limit the scope of image layout transitions.
   *
   * Still, Vulkan 1.2 requires this feature to be supported so we
   * advertise it even though we don't really take advantage of it.
   */
   .separateDepthStencilLayouts = true,
   .storageBuffer8BitAccess = true,
   .storagePushConstant8 = true,
   .imagelessFramebuffer = true,
                     /* These are mandatory by Vulkan 1.2, however, we don't support any of
   * the optional features affected by them (non 32-bit types for
   * shaderSubgroupExtendedTypes and additional subgroup ballot for
   * subgroupBroadcastDynamicId), so in practice setting them to true
   * doesn't have any implications for us until we implement any of these
   * optional features.
   */
   .shaderSubgroupExtendedTypes = true,
            .vulkanMemoryModel = true,
   .vulkanMemoryModelDeviceScope = true,
            .bufferDeviceAddress = true,
   .bufferDeviceAddressCaptureReplay = false,
            /* Vulkan 1.3 */
   .inlineUniformBlock  = true,
   /* Inline buffers work like push constants, so after their are bound
   * some of their contents may be copied into the uniform stream as soon
   * as the next draw/dispatch is recorded in the command buffer. This means
   * that if the client updates the buffer contents after binding it to
   * a command buffer, the next queue submit of that command buffer may
   * not use the latest update to the buffer contents, but the data that
   * was present in the buffer at the time it was bound to the command
   * buffer.
   */
   .descriptorBindingInlineUniformBlockUpdateAfterBind = false,
   .pipelineCreationCacheControl = true,
   .privateData = true,
   .maintenance4 = true,
   .shaderZeroInitializeWorkgroupMemory = true,
   .synchronization2 = true,
   .robustImageAccess = true,
            /* VK_EXT_4444_formats */
   .formatA4R4G4B4 = true,
            /* VK_EXT_custom_border_color */
   .customBorderColors = true,
            /* VK_EXT_index_type_uint8 */
            /* VK_EXT_line_rasterization */
   .rectangularLines = true,
   .bresenhamLines = true,
   .smoothLines = false,
   .stippledRectangularLines = false,
   .stippledBresenhamLines = false,
            /* VK_EXT_color_write_enable */
            /* VK_KHR_pipeline_executable_properties */
            /* VK_EXT_provoking_vertex */
   .provokingVertexLast = true,
   /* FIXME: update when supporting EXT_transform_feedback */
            /* VK_EXT_vertex_attribute_divisor */
   .vertexAttributeInstanceRateDivisor = true,
            /* VK_KHR_performance_query */
   .performanceCounterQueryPools = physical_device->caps.perfmon,
            /* VK_EXT_texel_buffer_alignment */
            /* VK_KHR_workgroup_memory_explicit_layout */
   .workgroupMemoryExplicitLayout = true,
   .workgroupMemoryExplicitLayoutScalarBlockLayout = false,
   .workgroupMemoryExplicitLayout8BitAccess = true,
            /* VK_EXT_border_color_swizzle */
   .borderColorSwizzle = true,
            /* VK_EXT_shader_module_identifier */
            /* VK_EXT_depth_clip_control */
            /* VK_EXT_attachment_feedback_loop_layout */
            /* VK_EXT_primitive_topology_list_restart */
   .primitiveTopologyListRestart = true,
   /* FIXME: we don't support tessellation shaders yet */
            /* VK_EXT_pipeline_robustness */
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_EnumerateInstanceExtensionProperties(const char *pLayerName,
               {
      /* We don't support any layers  */
   if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      static VkResult enumerate_devices(struct vk_instance *vk_instance);
      static void destroy_physical_device(struct vk_physical_device *device);
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
      struct v3dv_instance *instance;
                     if (pAllocator == NULL)
            instance = vk_alloc(pAllocator, sizeof(*instance), 8,
         if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(
         vk_instance_dispatch_table_from_entrypoints(
            result = vk_instance_init(&instance->vk,
                        if (result != VK_SUCCESS) {
      vk_free(pAllocator, instance);
                        instance->vk.physical_devices.enumerate = enumerate_devices;
            /* We start with the default values for the pipeline_cache envvars */
   instance->pipeline_cache_enabled = true;
   instance->default_pipeline_cache_enabled = true;
   const char *pipeline_cache_str = getenv("V3DV_ENABLE_PIPELINE_CACHE");
   if (pipeline_cache_str != NULL) {
      if (strncmp(pipeline_cache_str, "full", 4) == 0) {
         } else if (strncmp(pipeline_cache_str, "no-default-cache", 16) == 0) {
         } else if (strncmp(pipeline_cache_str, "off", 3) == 0) {
      instance->pipeline_cache_enabled = false;
      } else {
      fprintf(stderr, "Wrong value for envvar V3DV_ENABLE_PIPELINE_CACHE. "
                  if (instance->pipeline_cache_enabled == false) {
      fprintf(stderr, "WARNING: v3dv pipeline cache is disabled. Performance "
      } else {
      if (instance->default_pipeline_cache_enabled == false) {
   fprintf(stderr, "WARNING: default v3dv pipeline cache is disabled. "
                                          }
      static void
   v3dv_physical_device_free_disk_cache(struct v3dv_physical_device *device)
   {
   #ifdef ENABLE_SHADER_CACHE
      if (device->disk_cache)
      #else
         #endif
   }
      static void
   physical_device_finish(struct v3dv_physical_device *device)
   {
      v3dv_wsi_finish(device);
   v3dv_physical_device_free_disk_cache(device);
                     close(device->render_fd);
   if (device->display_fd >= 0)
                  #if using_v3d_simulator
         #endif
         vk_physical_device_finish(&device->vk);
      }
      static void
   destroy_physical_device(struct vk_physical_device *device)
   {
      physical_device_finish((struct v3dv_physical_device *)device);
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyInstance(VkInstance _instance,
         {
               if (!instance)
                     vk_instance_finish(&instance->vk);
      }
      static uint64_t
   compute_heap_size()
   {
   #if !using_v3d_simulator
      /* Query the total ram from the system */
   struct sysinfo info;
               #else
         #endif
         /* We don't want to burn too much ram with the GPU.  If the user has 4GB
   * or less, we use at most half.  If they have more than 4GB we limit it
   * to 3/4 with a max. of 4GB since the GPU cannot address more than that.
   */
   const uint64_t MAX_HEAP_SIZE = 4ull * 1024ull * 1024ull * 1024ull;
   uint64_t available;
   if (total_ram <= MAX_HEAP_SIZE)
         else
               }
      static uint64_t
   compute_memory_budget(struct v3dv_physical_device *device)
   {
      uint64_t heap_size = device->memory.memoryHeaps[0].size;
   uint64_t heap_used = device->heap_used;
      #if !using_v3d_simulator
      ASSERTED bool has_available_memory =
            #else
         #endif
         /* Let's not incite the app to starve the system: report at most 90% of
   * available system memory.
   */
   uint64_t heap_available = sys_available * 9 / 10;
      }
      static bool
   v3d_has_feature(struct v3dv_physical_device *device, enum drm_v3d_param feature)
   {
      struct drm_v3d_get_param p = {
         };
   if (v3dv_ioctl(device->render_fd, DRM_IOCTL_V3D_GET_PARAM, &p) != 0)
            }
      static bool
   device_has_expected_features(struct v3dv_physical_device *device)
   {
      return v3d_has_feature(device, DRM_V3D_PARAM_SUPPORTS_TFU) &&
            }
         static VkResult
   init_uuids(struct v3dv_physical_device *device)
   {
      const struct build_id_note *note =
         if (!note) {
      return vk_errorf(device->vk.instance,
                     unsigned build_id_len = build_id_length(note);
   if (build_id_len < 20) {
      return vk_errorf(device->vk.instance,
                              uint32_t vendor_id = v3dv_physical_device_vendor_id(device);
            struct mesa_sha1 sha1_ctx;
   uint8_t sha1[20];
            /* The pipeline cache UUID is used for determining when a pipeline cache is
   * invalid.  It needs both a driver build and the PCI ID of the device.
   */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, build_id_data(note), build_id_len);
   _mesa_sha1_update(&sha1_ctx, &device_id, sizeof(device_id));
   _mesa_sha1_final(&sha1_ctx, sha1);
            /* The driver UUID is used for determining sharability of images and memory
   * between two Vulkan instances in separate processes.  People who want to
   * share memory need to also check the device UUID (below) so all this
   * needs to be is the build-id.
   */
            /* The device UUID uniquely identifies the given device within the machine.
   * Since we never have more than one device, this doesn't need to be a real
   * UUID.
   */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, &vendor_id, sizeof(vendor_id));
   _mesa_sha1_update(&sha1_ctx, &device_id, sizeof(device_id));
   _mesa_sha1_final(&sha1_ctx, sha1);
               }
      static void
   v3dv_physical_device_init_disk_cache(struct v3dv_physical_device *device)
   {
   #ifdef ENABLE_SHADER_CACHE
      char timestamp[41];
            assert(device->name);
      #else
         #endif
   }
      static VkResult
   create_physical_device(struct v3dv_instance *instance,
               {
      VkResult result = VK_SUCCESS;
   int32_t display_fd = -1;
            struct v3dv_physical_device *device =
      vk_zalloc(&instance->vk.alloc, sizeof(*device), 8,
         if (!device)
            struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints
         vk_physical_device_dispatch_table_from_entrypoints(
            result = vk_physical_device_init(&device->vk, &instance->vk, NULL, NULL,
            if (result != VK_SUCCESS)
            assert(gpu_device);
   const char *path = gpu_device->nodes[DRM_NODE_RENDER];
   render_fd = open(path, O_RDWR | O_CLOEXEC);
   if (render_fd < 0) {
      fprintf(stderr, "Opening %s failed: %s\n", path, strerror(errno));
   result = VK_ERROR_INITIALIZATION_FAILED;
               /* If we are running on VK_KHR_display we need to acquire the master
   * display device now for the v3dv_wsi_init() call below. For anything else
   * we postpone that until a swapchain is created.
               #if !using_v3d_simulator
      if (display_device)
         else
      #else
         #endif
                  device->has_primary = primary_path;
   if (device->has_primary) {
      if (stat(primary_path, &primary_stat) != 0) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                                       if (fstat(render_fd, &render_stat) != 0) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                  }
   device->has_render = true;
         #if using_v3d_simulator
         #endif
         if (instance->vk.enabled_extensions.KHR_display ||
      instance->vk.enabled_extensions.KHR_xcb_surface ||
   instance->vk.enabled_extensions.KHR_xlib_surface ||
   instance->vk.enabled_extensions.KHR_wayland_surface ||
   #if !using_v3d_simulator
         /* Open the primary node on the vc4 display device */
   assert(display_device);
   #else
         /* There is only one device with primary and render nodes.
   * Open its primary node.
   */
   #endif
            #if using_v3d_simulator
         #endif
         device->render_fd = render_fd;    /* The v3d render node  */
            if (!v3d_get_device_info(device->render_fd, &device->devinfo, &v3dv_ioctl)) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                     if (device->devinfo.ver < 42) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                     if (!device_has_expected_features(device)) {
      result = vk_errorf(instance, VK_ERROR_INITIALIZATION_FAILED,
                     device->caps.multisync =
            device->caps.perfmon =
            result = init_uuids(device);
   if (result != VK_SUCCESS)
            device->compiler = v3d_compiler_init(&device->devinfo,
                  ASSERTED int len =
      asprintf(&device->name, "V3D %d.%d.%d",
            device->devinfo.ver / 10,
                     /* Setup available memory heaps and types */
   VkPhysicalDeviceMemoryProperties *mem = &device->memory;
   mem->memoryHeapCount = 1;
   mem->memoryHeaps[0].size = compute_heap_size();
            /* This is the only combination required by the spec */
   mem->memoryTypeCount = 1;
   mem->memoryTypes[0].propertyFlags =
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
               /* Initialize sparse array for refcounting imported BOs */
                              /* We don't support timelines in the uAPI yet and we don't want it getting
   * suddenly turned on by vk_drm_syncobj_get_type() without us adding v3dv
   * code for it first.
   */
            /* Multiwait is required for emulated timeline semaphores and is supported
   * by the v3d kernel interface.
   */
            device->sync_timeline_type =
            device->sync_types[0] = &device->drm_syncobj_type;
   device->sync_types[1] = &device->sync_timeline_type.sync;
   device->sync_types[2] = NULL;
            result = v3dv_wsi_init(device);
   if (result != VK_SUCCESS) {
      vk_error(instance, result);
               get_device_extensions(device, &device->vk.supported_extensions);
                                    fail:
      vk_physical_device_finish(&device->vk);
            if (render_fd >= 0)
         if (display_fd >= 0)
               }
      /* This driver hook is expected to return VK_SUCCESS (unless a memory
   * allocation error happened) if no compatible device is found. If a
   * compatible device is found, it may return an error code if device
   * inialization failed.
   */
   static VkResult
   enumerate_devices(struct vk_instance *vk_instance)
   {
      struct v3dv_instance *instance =
            /* FIXME: Check for more devices? */
   drmDevicePtr devices[8];
            max_devices = drmGetDevices2(0, devices, ARRAY_SIZE(devices));
   if (max_devices < 1)
                  #if !using_v3d_simulator
      int32_t v3d_idx = -1;
      #endif
         #if using_v3d_simulator
         /* In the simulator, we look for an Intel/AMD render node */
   const int required_nodes = (1 << DRM_NODE_RENDER) | (1 << DRM_NODE_PRIMARY);
   if ((devices[i]->available_nodes & required_nodes) == required_nodes &&
      devices[i]->bustype == DRM_BUS_PCI &&
   (devices[i]->deviceinfo.pci->vendor_id == 0x8086 ||
   devices[i]->deviceinfo.pci->vendor_id == 0x1002)) {
   result = create_physical_device(instance, devices[i], NULL);
   if (result == VK_SUCCESS)
      #else
         /* On actual hardware, we should have a gpu device (v3d) and a display
   * device (vc4). We will need to use the display device to allocate WSI
   * buffers and share them with the render node via prime, but that is a
   * privileged operation so we need t have an authenticated display fd
   * and for that we need the display server to provide the it (with DRI3),
   * so here we only check that the device is present but we don't try to
   * open it.
   */
   if (devices[i]->bustype != DRM_BUS_PLATFORM)
            if (devices[i]->available_nodes & 1 << DRM_NODE_RENDER) {
      char **compat = devices[i]->deviceinfo.platform->compatible;
   while (*compat) {
      if (strncmp(*compat, "brcm,2711-v3d", 13) == 0 ||
      strncmp(*compat, "brcm,2712-v3d", 13) == 0) {
   v3d_idx = i;
      }
         } else if (devices[i]->available_nodes & 1 << DRM_NODE_PRIMARY) {
      char **compat = devices[i]->deviceinfo.platform->compatible;
   while (*compat) {
      if (strncmp(*compat, "brcm,bcm2712-vc6", 16) == 0 ||
      strncmp(*compat, "brcm,bcm2711-vc5", 16) == 0 ||
   strncmp(*compat, "brcm,bcm2835-vc4", 16) == 0) {
   vc4_idx = i;
      }
         #endif
            #if !using_v3d_simulator
      if (v3d_idx != -1) {
      drmDevicePtr v3d_device = devices[v3d_idx];
   drmDevicePtr vc4_device = vc4_idx != -1 ? devices[vc4_idx] : NULL;
         #endif
                     }
      uint32_t
   v3dv_physical_device_vendor_id(struct v3dv_physical_device *dev)
   {
         }
      uint32_t
   v3dv_physical_device_device_id(struct v3dv_physical_device *dev)
   {
   #if using_v3d_simulator
         #else
      switch (dev->devinfo.ver) {
   case 42:
         case 71:
         default:
            #endif
   }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
         {
               STATIC_ASSERT(MAX_SAMPLED_IMAGES + MAX_STORAGE_IMAGES + MAX_INPUT_ATTACHMENTS
         STATIC_ASSERT(MAX_UNIFORM_BUFFERS >= MAX_DYNAMIC_UNIFORM_BUFFERS);
            const uint32_t page_size = 4096;
                     const float v3d_point_line_granularity = 2.0f / (1 << V3D_COORD_SHIFT);
            const VkSampleCountFlags supported_sample_counts =
                     struct timespec clock_res;
   clock_getres(CLOCK_MONOTONIC, &clock_res);
   const float timestamp_period =
            /* FIXME: this will probably require an in-depth review */
   VkPhysicalDeviceLimits limits = {
      .maxImageDimension1D                      = V3D_MAX_IMAGE_DIMENSION,
   .maxImageDimension2D                      = V3D_MAX_IMAGE_DIMENSION,
   .maxImageDimension3D                      = V3D_MAX_IMAGE_DIMENSION,
   .maxImageDimensionCube                    = V3D_MAX_IMAGE_DIMENSION,
   .maxImageArrayLayers                      = V3D_MAX_ARRAY_LAYERS,
   .maxTexelBufferElements                   = (1ul << 28),
   .maxUniformBufferRange                    = V3D_MAX_BUFFER_RANGE,
   .maxStorageBufferRange                    = V3D_MAX_BUFFER_RANGE,
   .maxPushConstantsSize                     = MAX_PUSH_CONSTANTS_SIZE,
   .maxMemoryAllocationCount                 = mem_size / page_size,
   .maxSamplerAllocationCount                = 64 * 1024,
   .bufferImageGranularity                   = V3D_NON_COHERENT_ATOM_SIZE,
   .sparseAddressSpaceSize                   = 0,
   .maxBoundDescriptorSets                   = MAX_SETS,
   .maxPerStageDescriptorSamplers            = V3D_MAX_TEXTURE_SAMPLERS,
   .maxPerStageDescriptorUniformBuffers      = MAX_UNIFORM_BUFFERS,
   .maxPerStageDescriptorStorageBuffers      = MAX_STORAGE_BUFFERS,
   .maxPerStageDescriptorSampledImages       = MAX_SAMPLED_IMAGES,
   .maxPerStageDescriptorStorageImages       = MAX_STORAGE_IMAGES,
   .maxPerStageDescriptorInputAttachments    = MAX_INPUT_ATTACHMENTS,
            /* Some of these limits are multiplied by 6 because they need to
   * include all possible shader stages (even if not supported). See
   * 'Required Limits' table in the Vulkan spec.
   */
   .maxDescriptorSetSamplers                 = 6 * V3D_MAX_TEXTURE_SAMPLERS,
   .maxDescriptorSetUniformBuffers           = 6 * MAX_UNIFORM_BUFFERS,
   .maxDescriptorSetUniformBuffersDynamic    = MAX_DYNAMIC_UNIFORM_BUFFERS,
   .maxDescriptorSetStorageBuffers           = 6 * MAX_STORAGE_BUFFERS,
   .maxDescriptorSetStorageBuffersDynamic    = MAX_DYNAMIC_STORAGE_BUFFERS,
   .maxDescriptorSetSampledImages            = 6 * MAX_SAMPLED_IMAGES,
   .maxDescriptorSetStorageImages            = 6 * MAX_STORAGE_IMAGES,
            /* Vertex limits */
   .maxVertexInputAttributes                 = MAX_VERTEX_ATTRIBS,
   .maxVertexInputBindings                   = MAX_VBS,
   .maxVertexInputAttributeOffset            = 0xffffffff,
   .maxVertexInputBindingStride              = 0xffffffff,
            /* Tessellation limits */
   .maxTessellationGenerationLevel           = 0,
   .maxTessellationPatchSize                 = 0,
   .maxTessellationControlPerVertexInputComponents = 0,
   .maxTessellationControlPerVertexOutputComponents = 0,
   .maxTessellationControlPerPatchOutputComponents = 0,
   .maxTessellationControlTotalOutputComponents = 0,
   .maxTessellationEvaluationInputComponents = 0,
            /* Geometry limits */
   .maxGeometryShaderInvocations             = 32,
   .maxGeometryInputComponents               = 64,
   .maxGeometryOutputComponents              = 64,
   .maxGeometryOutputVertices                = 256,
            /* Fragment limits */
   .maxFragmentInputComponents               = max_varying_components,
   .maxFragmentOutputAttachments             = 4,
   .maxFragmentDualSrcAttachments            = 0,
   .maxFragmentCombinedOutputResources       = max_rts +
                  /* Compute limits */
   .maxComputeSharedMemorySize               = 16384,
   .maxComputeWorkGroupCount                 = { 65535, 65535, 65535 },
   .maxComputeWorkGroupInvocations           = 256,
            .subPixelPrecisionBits                    = V3D_COORD_SHIFT,
   .subTexelPrecisionBits                    = 8,
   .mipmapPrecisionBits                      = 8,
   .maxDrawIndexedIndexValue                 = pdevice->devinfo.ver >= 71 ?
         .maxDrawIndirectCount                     = 0x7fffffff,
   .maxSamplerLodBias                        = 14.0f,
   .maxSamplerAnisotropy                     = 16.0f,
   .maxViewports                             = MAX_VIEWPORTS,
   .maxViewportDimensions                    = { max_fb_size, max_fb_size },
   .viewportBoundsRange                      = { -2.0 * max_fb_size,
         .viewportSubPixelBits                     = 0,
   .minMemoryMapAlignment                    = page_size,
   .minTexelBufferOffsetAlignment            = V3D_TMU_TEXEL_ALIGN,
   .minUniformBufferOffsetAlignment          = 32,
   .minStorageBufferOffsetAlignment          = 32,
   .minTexelOffset                           = -8,
   .maxTexelOffset                           = 7,
   .minTexelGatherOffset                     = -8,
   .maxTexelGatherOffset                     = 7,
   .minInterpolationOffset                   = -0.5,
   .maxInterpolationOffset                   = 0.5,
   .subPixelInterpolationOffsetBits          = V3D_COORD_SHIFT,
   .maxFramebufferWidth                      = max_fb_size,
   .maxFramebufferHeight                     = max_fb_size,
   .maxFramebufferLayers                     = 256,
   .framebufferColorSampleCounts             = supported_sample_counts,
   .framebufferDepthSampleCounts             = supported_sample_counts,
   .framebufferStencilSampleCounts           = supported_sample_counts,
   .framebufferNoAttachmentsSampleCounts     = supported_sample_counts,
   .maxColorAttachments                      = max_rts,
   .sampledImageColorSampleCounts            = supported_sample_counts,
   .sampledImageIntegerSampleCounts          = supported_sample_counts,
   .sampledImageDepthSampleCounts            = supported_sample_counts,
   .sampledImageStencilSampleCounts          = supported_sample_counts,
   .storageImageSampleCounts                 = VK_SAMPLE_COUNT_1_BIT,
   .maxSampleMaskWords                       = 1,
   .timestampComputeAndGraphics              = true,
   .timestampPeriod                          = timestamp_period,
   .maxClipDistances                         = 8,
   .maxCullDistances                         = 0,
   .maxCombinedClipAndCullDistances          = 8,
   .discreteQueuePriorities                  = 2,
   .pointSizeRange                           = { v3d_point_line_granularity,
         .lineWidthRange                           = { 1.0f, V3D_MAX_LINE_WIDTH },
   .pointSizeGranularity                     = v3d_point_line_granularity,
   .lineWidthGranularity                     = v3d_point_line_granularity,
   .strictLines                              = true,
   .standardSampleLocations                  = false,
   .optimalBufferCopyOffsetAlignment         = 32,
   .optimalBufferCopyRowPitchAlignment       = 32,
               *pProperties = (VkPhysicalDeviceProperties) {
      .apiVersion = V3DV_API_VERSION,
   .driverVersion = vk_get_driver_version(),
   .vendorID = v3dv_physical_device_vendor_id(pdevice),
   .deviceID = v3dv_physical_device_device_id(pdevice),
   .deviceType = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
   .limits = limits,
               snprintf(pProperties->deviceName, sizeof(pProperties->deviceName),
         memcpy(pProperties->pipelineCacheUUID,
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
         {
                        /* We don't really have special restrictions for the maximum
   * descriptors per set, other than maybe not exceeding the limits
   * of addressable memory in a single allocation on either the host
   * or the GPU. This will be a much larger limit than any of the
   * per-stage limits already available in Vulkan though, so in practice,
   * it is not expected to limit anything beyond what is already
   * constrained through per-stage limits.
   */
   const uint32_t max_host_descriptors =
      (UINT32_MAX - sizeof(struct v3dv_descriptor_set)) /
      const uint32_t max_gpu_descriptors =
            VkPhysicalDeviceVulkan13Properties vk13 = {
      .maxInlineUniformBlockSize = 4096,
   .maxPerStageDescriptorInlineUniformBlocks = MAX_INLINE_UNIFORM_BUFFERS,
   .maxDescriptorSetInlineUniformBlocks = MAX_INLINE_UNIFORM_BUFFERS,
   .maxPerStageDescriptorUpdateAfterBindInlineUniformBlocks =
         .maxDescriptorSetUpdateAfterBindInlineUniformBlocks =
         .maxBufferSize = V3D_MAX_BUFFER_RANGE,
   .storageTexelBufferOffsetAlignmentBytes = V3D_TMU_TEXEL_ALIGN,
   .storageTexelBufferOffsetSingleTexelAlignment = false,
   .uniformTexelBufferOffsetAlignmentBytes = V3D_TMU_TEXEL_ALIGN,
   .uniformTexelBufferOffsetSingleTexelAlignment = false,
   /* No native acceleration for integer dot product. We use NIR lowering. */
   .integerDotProduct8BitUnsignedAccelerated = false,
   .integerDotProduct8BitMixedSignednessAccelerated = false,
   .integerDotProduct4x8BitPackedUnsignedAccelerated = false,
   .integerDotProduct4x8BitPackedSignedAccelerated = false,
   .integerDotProduct4x8BitPackedMixedSignednessAccelerated = false,
   .integerDotProduct16BitUnsignedAccelerated = false,
   .integerDotProduct16BitSignedAccelerated = false,
   .integerDotProduct16BitMixedSignednessAccelerated = false,
   .integerDotProduct32BitUnsignedAccelerated = false,
   .integerDotProduct32BitSignedAccelerated = false,
   .integerDotProduct32BitMixedSignednessAccelerated = false,
   .integerDotProduct64BitUnsignedAccelerated = false,
   .integerDotProduct64BitSignedAccelerated = false,
   .integerDotProduct64BitMixedSignednessAccelerated = false,
   .integerDotProductAccumulatingSaturating8BitUnsignedAccelerated = false,
   .integerDotProductAccumulatingSaturating8BitSignedAccelerated = false,
   .integerDotProductAccumulatingSaturating8BitMixedSignednessAccelerated = false,
   .integerDotProductAccumulatingSaturating4x8BitPackedUnsignedAccelerated = false,
   .integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated = false,
   .integerDotProductAccumulatingSaturating4x8BitPackedMixedSignednessAccelerated = false,
   .integerDotProductAccumulatingSaturating16BitUnsignedAccelerated = false,
   .integerDotProductAccumulatingSaturating16BitSignedAccelerated = false,
   .integerDotProductAccumulatingSaturating16BitMixedSignednessAccelerated = false,
   .integerDotProductAccumulatingSaturating32BitUnsignedAccelerated = false,
   .integerDotProductAccumulatingSaturating32BitSignedAccelerated = false,
   .integerDotProductAccumulatingSaturating32BitMixedSignednessAccelerated = false,
   .integerDotProductAccumulatingSaturating64BitUnsignedAccelerated = false,
   .integerDotProductAccumulatingSaturating64BitSignedAccelerated = false,
               VkPhysicalDeviceVulkan12Properties vk12 = {
      .driverID = VK_DRIVER_ID_MESA_V3DV,
   .conformanceVersion = {
      .major = 1,
   .minor = 3,
   .subminor = 6,
      },
   .supportedDepthResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT,
   .supportedStencilResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT,
   /* FIXME: if we want to support independentResolveNone then we would
   * need to honor attachment load operations on resolve attachments,
   * which we currently ignore because the resolve makes them irrelevant,
   * as it unconditionally writes all pixels in the render area. However,
   * with independentResolveNone, it is possible to have one aspect of a
   * D/S resolve attachment stay unresolved, in which case the attachment
   * load operation is relevant.
   *
   * NOTE: implementing attachment load for resolve attachments isn't
   * immediately trivial because these attachments are not part of the
   * framebuffer and therefore we can't use the same mechanism we use
   * for framebuffer attachments. Instead, we should probably have to
   * emit a meta operation for that right at the start of the render
   * pass (or subpass).
   */
   .independentResolveNone = false,
   .independentResolve = false,
            .denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .shaderSignedZeroInfNanPreserveFloat16 = true,
   .shaderSignedZeroInfNanPreserveFloat32 = true,
   .shaderSignedZeroInfNanPreserveFloat64 = false,
   .shaderDenormPreserveFloat16 = true,
   .shaderDenormPreserveFloat32 = true,
   .shaderDenormPreserveFloat64 = false,
   .shaderDenormFlushToZeroFloat16 = false,
   .shaderDenormFlushToZeroFloat32 = false,
   .shaderDenormFlushToZeroFloat64 = false,
   .shaderRoundingModeRTEFloat16 = true,
   .shaderRoundingModeRTEFloat32 = true,
   .shaderRoundingModeRTEFloat64 = false,
   .shaderRoundingModeRTZFloat16 = false,
   .shaderRoundingModeRTZFloat32 = false,
            /* V3D doesn't support min/max filtering */
   .filterMinmaxSingleComponentFormats = false,
            .framebufferIntegerColorSampleCounts =
      };
   memset(vk12.driverName, 0, VK_MAX_DRIVER_NAME_SIZE);
   snprintf(vk12.driverName, VK_MAX_DRIVER_NAME_SIZE, "V3DV Mesa");
   memset(vk12.driverInfo, 0, VK_MAX_DRIVER_INFO_SIZE);
   snprintf(vk12.driverInfo, VK_MAX_DRIVER_INFO_SIZE,
            VkPhysicalDeviceVulkan11Properties vk11 = {
      .deviceLUIDValid = false,
   .subgroupSize = V3D_CHANNELS,
   .subgroupSupportedStages = VK_SHADER_STAGE_COMPUTE_BIT,
   .subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT,
   .subgroupQuadOperationsInAllStages = false,
   .pointClippingBehavior = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES,
   .maxMultiviewViewCount = MAX_MULTIVIEW_VIEW_COUNT,
   .maxMultiviewInstanceIndex = UINT32_MAX - 1,
   .protectedNoFault = false,
   .maxPerSetDescriptors = MIN2(max_host_descriptors, max_gpu_descriptors),
   /* Minimum required by the spec */
      };
   memcpy(vk11.deviceUUID, pdevice->device_uuid, VK_UUID_SIZE);
               vk_foreach_struct(ext, pProperties->pNext) {
      if (vk_get_physical_device_core_1_1_property_ext(ext, &vk11))
         if (vk_get_physical_device_core_1_2_property_ext(ext, &vk12))
         if (vk_get_physical_device_core_1_3_property_ext(ext, &vk13))
            switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CUSTOM_BORDER_COLOR_PROPERTIES_EXT: {
      VkPhysicalDeviceCustomBorderColorPropertiesEXT *props =
         props->maxCustomBorderColorSamplers = V3D_MAX_TEXTURE_SAMPLERS;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROVOKING_VERTEX_PROPERTIES_EXT: {
      VkPhysicalDeviceProvokingVertexPropertiesEXT *props =
         props->provokingVertexModePerPipeline = true;
   /* FIXME: update when supporting EXT_transform_feedback */
   props->transformFeedbackPreservesTriangleFanProvokingVertex = false;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT: {
      VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *props =
         props->maxVertexAttribDivisor = 0xffff;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PERFORMANCE_QUERY_PROPERTIES_KHR : {
                     props->allowCommandBufferQueryCopies = true;
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
   props->sharedImage = front_rendering_usage ? VK_TRUE
            #pragma GCC diagnostic pop
   #endif
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRM_PROPERTIES_EXT: {
      VkPhysicalDeviceDrmPropertiesEXT *props =
         props->hasPrimary = pdevice->has_primary;
   if (props->hasPrimary) {
      props->primaryMajor = (int64_t) major(pdevice->primary_devid);
      }
   props->hasRender = pdevice->has_render;
   if (props->hasRender) {
      props->renderMajor = (int64_t) major(pdevice->render_devid);
      }
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_LINE_RASTERIZATION_PROPERTIES_EXT: {
      VkPhysicalDeviceLineRasterizationPropertiesEXT *props =
         props->lineSubPixelPrecisionBits = V3D_COORD_SHIFT;
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PCI_BUS_INFO_PROPERTIES_EXT:
      /* Do nothing, not even logging. This is a non-PCI device, so we will
   * never provide this extension.
   */
      case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_MODULE_IDENTIFIER_PROPERTIES_EXT: {
      VkPhysicalDeviceShaderModuleIdentifierPropertiesEXT *props =
         STATIC_ASSERT(sizeof(vk_shaderModuleIdentifierAlgorithmUUID) ==
         memcpy(props->shaderModuleIdentifierAlgorithmUUID,
         vk_shaderModuleIdentifierAlgorithmUUID,
      }
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_ROBUSTNESS_PROPERTIES_EXT: {
      VkPhysicalDevicePipelineRobustnessPropertiesEXT *props =
         props->defaultRobustnessStorageBuffers =
         props->defaultRobustnessUniformBuffers =
         props->defaultRobustnessVertexInputs =
         props->defaultRobustnessImages =
            }
   default:
      v3dv_debug_ignored_stype(ext->sType);
            }
      /* We support exactly one queue family. */
   static const VkQueueFamilyProperties
   v3dv_queue_family_properties = {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               .queueCount = 1,
   .timestampValidBits = 64,
      };
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
               {
      VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out,
            vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p) {
               vk_foreach_struct(s, p->pNext) {
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
         {
      V3DV_FROM_HANDLE(v3dv_physical_device, device, physicalDevice);
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
         {
               v3dv_GetPhysicalDeviceMemoryProperties(physicalDevice,
            vk_foreach_struct(ext, pMemoryProperties->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT: {
      VkPhysicalDeviceMemoryBudgetPropertiesEXT *p =
                        /* The heapBudget and heapUsage values must be zero for array elements
   * greater than or equal to VkPhysicalDeviceMemoryProperties::memoryHeapCount
   */
   for (unsigned i = 1; i < VK_MAX_MEMORY_HEAPS; i++) {
      p->heapBudget[i] = 0u;
      }
      }
   default:
      v3dv_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   v3dv_GetInstanceProcAddr(VkInstance _instance,
         {
      V3DV_FROM_HANDLE(v3dv_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk,
            }
      /* With version 1+ of the loader interface the ICD should expose
   * vk_icdGetInstanceProcAddr to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction
   VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance,
            PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance,
         {
         }
      /* With version 4+ of the loader interface the ICD should expose
   * vk_icdGetPhysicalDeviceProcAddr()
   */
   PUBLIC
   VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance  _instance,
            PFN_vkVoidFunction
   vk_icdGetPhysicalDeviceProcAddr(VkInstance  _instance,
         {
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
         {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice,
               {
               if (pProperties == NULL) {
      *pPropertyCount = 0;
                  }
      static void
   destroy_queue_syncs(struct v3dv_queue *queue)
   {
      for (int i = 0; i < V3DV_QUEUE_COUNT; i++) {
      if (queue->last_job_syncs.syncs[i]) {
      drmSyncobjDestroy(queue->device->pdevice->render_fd,
            }
      static VkResult
   queue_init(struct v3dv_device *device, struct v3dv_queue *queue,
               {
      VkResult result = vk_queue_init(&queue->vk, &device->vk, create_info,
         if (result != VK_SUCCESS)
            result = vk_queue_enable_submit_thread(&queue->vk);
   if (result != VK_SUCCESS)
            queue->device = device;
            for (int i = 0; i < V3DV_QUEUE_COUNT; i++) {
      queue->last_job_syncs.first[i] = true;
   int ret = drmSyncobjCreate(device->pdevice->render_fd,
               if (ret) {
      result = vk_errorf(device, VK_ERROR_INITIALIZATION_FAILED,
                        queue->noop_job = NULL;
         fail_last_job_syncs:
         fail_submit_thread:
      vk_queue_finish(&queue->vk);
      }
      static void
   queue_finish(struct v3dv_queue *queue)
   {
      if (queue->noop_job)
         destroy_queue_syncs(queue);
      }
      static void
   init_device_meta(struct v3dv_device *device)
   {
      mtx_init(&device->meta.mtx, mtx_plain);
   v3dv_meta_clear_init(device);
   v3dv_meta_blit_init(device);
      }
      static void
   destroy_device_meta(struct v3dv_device *device)
   {
      mtx_destroy(&device->meta.mtx);
   v3dv_meta_clear_finish(device);
   v3dv_meta_blit_finish(device);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateDevice(VkPhysicalDevice physicalDevice,
                     {
      V3DV_FROM_HANDLE(v3dv_physical_device, physical_device, physicalDevice);
   struct v3dv_instance *instance = (struct v3dv_instance*) physical_device->vk.instance;
   VkResult result;
                     /* Check requested queues (we only expose one queue ) */
   assert(pCreateInfo->queueCreateInfoCount == 1);
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      assert(pCreateInfo->pQueueCreateInfos[i].queueFamilyIndex == 0);
   assert(pCreateInfo->pQueueCreateInfos[i].queueCount == 1);
   if (pCreateInfo->pQueueCreateInfos[i].flags != 0)
               device = vk_zalloc2(&physical_device->vk.instance->alloc, pAllocator,
               if (!device)
            struct vk_device_dispatch_table dispatch_table;
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         result = vk_device_init(&device->vk, &physical_device->vk,
         if (result != VK_SUCCESS) {
      vk_free(&device->vk.alloc, device);
            #ifdef ANDROID
      device->gralloc = u_gralloc_create(U_GRALLOC_TYPE_AUTO);
      #endif
         device->instance = instance;
            mtx_init(&device->query_mutex, mtx_plain);
                     vk_device_set_drm_fd(&device->vk, physical_device->render_fd);
            result = queue_init(device, &device->queue,
         if (result != VK_SUCCESS)
                     if (device->vk.enabled_features.robustBufferAccess)
            if (device->vk.enabled_features.robustImageAccess)
            #ifdef DEBUG
         #endif
      init_device_meta(device);
   v3dv_bo_cache_init(device);
   v3dv_pipeline_cache_init(&device->default_pipeline_cache, device, 0,
         device->default_attribute_float =
            device->device_address_mem_ctx = ralloc_context(NULL);
   util_dynarray_init(&device->device_address_bo_list,
            mtx_init(&device->events.lock, mtx_plain);
   result = v3dv_event_allocate_resources(device);
   if (result != VK_SUCCESS)
            if (list_is_empty(&device->events.free_list)) {
      result = vk_error(device, VK_ERROR_OUT_OF_DEVICE_MEMORY);
               result = v3dv_query_allocate_resources(device);
   if (result != VK_SUCCESS)
                           fail:
      cnd_destroy(&device->query_ended);
   mtx_destroy(&device->query_mutex);
   queue_finish(&device->queue);
   destroy_device_meta(device);
   v3dv_pipeline_cache_finish(&device->default_pipeline_cache);
   v3dv_event_free_resources(device);
   v3dv_query_free_resources(device);
      #ifdef ANDROID
         #endif
                  }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyDevice(VkDevice _device,
         {
               device->vk.dispatch_table.DeviceWaitIdle(_device);
            v3dv_event_free_resources(device);
                     destroy_device_meta(device);
            if (device->default_attribute_float) {
      v3dv_bo_free(device, device->default_attribute_float);
                        /* Bo cache should be removed the last, as any other object could be
   * freeing their private bos
   */
            cnd_destroy(&device->query_ended);
               #ifdef ANDROID
         #endif
         }
      static VkResult
   device_alloc(struct v3dv_device *device,
               {
      /* Our kernel interface is 32-bit */
            mem->bo = v3dv_bo_alloc(device, size, "device_alloc", false);
   if (!mem->bo)
               }
      static void
   device_free_wsi_dumb(int32_t display_fd, int32_t dumb_handle)
   {
      assert(display_fd != -1);
   if (dumb_handle < 0)
            struct drm_mode_destroy_dumb destroy_dumb = {
         };
   if (v3dv_ioctl(display_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_dumb)) {
            }
      static void
   device_free(struct v3dv_device *device, struct v3dv_device_memory *mem)
   {
      /* If this memory allocation was for WSI, then we need to use the
   * display device to free the allocated dumb BO.
   */
   if (mem->is_for_wsi) {
                              }
      static void
   device_unmap(struct v3dv_device *device, struct v3dv_device_memory *mem)
   {
      assert(mem && mem->bo->map && mem->bo->map_size > 0);
      }
      static VkResult
   device_map(struct v3dv_device *device, struct v3dv_device_memory *mem)
   {
               /* From the spec:
   *
   *   "After a successful call to vkMapMemory the memory object memory is
   *   considered to be currently host mapped. It is an application error to
   *   call vkMapMemory on a memory object that is already host mapped."
   *
   * We are not concerned with this ourselves (validation layers should
   * catch these errors and warn users), however, the driver may internally
   * map things (for example for debug CLIF dumps or some CPU-side operations)
   * so by the time the user calls here the buffer might already been mapped
   * internally by the driver.
   */
   if (mem->bo->map) {
      assert(mem->bo->map_size == mem->bo->size);
               bool ok = v3dv_bo_map(device, mem->bo, mem->bo->size);
   if (!ok)
               }
      static VkResult
   device_import_bo(struct v3dv_device *device,
                     {
               off_t real_size = lseek(fd, 0, SEEK_END);
   lseek(fd, 0, SEEK_SET);
   if (real_size < 0 || (uint64_t) real_size < size)
            int render_fd = device->pdevice->render_fd;
            int ret;
   uint32_t handle;
   ret = drmPrimeFDToHandle(render_fd, fd, &handle);
   if (ret)
            struct drm_v3d_get_bo_offset get_offset = {
         };
   ret = v3dv_ioctl(render_fd, DRM_IOCTL_V3D_GET_BO_OFFSET, &get_offset);
   if (ret)
                  *bo = v3dv_device_lookup_bo(device->pdevice, handle);
            if ((*bo)->refcnt == 0)
         else
               }
      static VkResult
   device_alloc_for_wsi(struct v3dv_device *device,
                     {
      /* In the simulator we can get away with a regular allocation since both
   * allocation and rendering happen in the same DRM render node. On actual
   * hardware we need to allocate our winsys BOs on the vc4 display device
   * and import them into v3d.
      #if using_v3d_simulator
         #else
      VkResult result;
   struct v3dv_physical_device *pdevice = device->pdevice;
                     int display_fd = pdevice->display_fd;
   struct drm_mode_create_dumb create_dumb = {
      .width = 1024, /* one page */
   .height = align(size, 4096) / 4096,
               int err;
   err = v3dv_ioctl(display_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb);
   if (err < 0)
            int fd;
   err =
         if (err < 0)
            result = device_import_bo(device, pAllocator, fd, size, &mem->bo);
   close(fd);
   if (result != VK_SUCCESS)
            mem->bo->dumb_handle = create_dumb.handle;
         fail_import:
   fail_export:
            fail_create:
         #endif
   }
      static void
   device_add_device_address_bo(struct v3dv_device *device,
         {
      util_dynarray_append(&device->device_address_bo_list,
            }
      static void
   device_remove_device_address_bo(struct v3dv_device *device,
         {
      util_dynarray_delete_unordered(&device->device_address_bo_list,
            }
      static void
   free_memory(struct v3dv_device *device,
               {
      if (mem == NULL)
            if (mem->bo->map)
            if (mem->is_for_device_address)
                        }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_FreeMemory(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   V3DV_FROM_HANDLE(v3dv_device_memory, mem, _mem);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_AllocateMemory(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
   struct v3dv_device_memory *mem;
                     /* We always allocate device memory in multiples of a page, so round up
   * requested size to that.
   */
            if (unlikely(alloc_size > MAX_MEMORY_ALLOCATION_SIZE))
            uint64_t heap_used = p_atomic_read(&pdevice->heap_used);
   if (unlikely(heap_used + alloc_size > pdevice->memory.memoryHeaps[0].size))
            mem = vk_device_memory_create(&device->vk, pAllocateInfo,
         if (mem == NULL)
            assert(pAllocateInfo->memoryTypeIndex < pdevice->memory.memoryTypeCount);
   mem->type = &pdevice->memory.memoryTypes[pAllocateInfo->memoryTypeIndex];
            const struct wsi_memory_allocate_info *wsi_info = NULL;
   const VkImportMemoryFdInfoKHR *fd_info = NULL;
   const VkMemoryAllocateFlagsInfo *flags_info = NULL;
   vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch ((unsigned)ext->sType) {
   case VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA:
      wsi_info = (void *)ext;
      case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR:
      fd_info = (void *)ext;
      case VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO:
      flags_info = (void *)ext;
      case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO:
      /* We don't have particular optimizations associated with memory
   * allocations that won't be suballocated to multiple resources.
   */
      case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO:
      /* The mask of handle types specified here must be supported
   * according to VkExternalImageFormatProperties, so it must be
   * fd or dmabuf, which don't have special requirements for us.
   */
      default:
      v3dv_debug_ignored_stype(ext->sType);
                           if (wsi_info) {
         } else if (fd_info && fd_info->handleType) {
      assert(fd_info->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
         result = device_import_bo(device, pAllocator,
         if (result == VK_SUCCESS)
         #ifdef ANDROID
         const native_handle_t *handle = AHardwareBuffer_getNativeHandle(mem->vk.ahardware_buffer);
   assert(handle->numFds > 0);
   size_t size = lseek(handle->data[0], 0, SEEK_END);
   result = device_import_bo(device, pAllocator,
   #else
         #endif
      } else {
                  if (result != VK_SUCCESS) {
      vk_device_memory_destroy(&device->vk, pAllocator, &mem->vk);
               heap_used = p_atomic_add_return(&pdevice->heap_used, mem->bo->size);
   if (heap_used > pdevice->memory.memoryHeaps[0].size) {
      free_memory(device, mem, pAllocator);
               /* If this memory can be used via VK_KHR_buffer_device_address then we
   * will need to manually add the BO to any job submit that makes use of
   * VK_KHR_buffer_device_address, since such jobs may produce buffer
   * load/store operations that may access any buffer memory allocated with
   * this flag and we don't have any means to tell which buffers will be
   * accessed through this mechanism since they don't even have to be bound
   * through descriptor state.
   */
   if (flags_info &&
      (flags_info->flags & VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR)) {
   mem->is_for_device_address = true;
               *pMem = v3dv_device_memory_to_handle(mem);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_MapMemory(VkDevice _device,
                  VkDeviceMemory _memory,
   VkDeviceSize offset,
      {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (mem == NULL) {
      *ppData = NULL;
                        /* Since the driver can map BOs internally as well and the mapped range
   * required by the user or the driver might not be the same, we always map
   * the entire BO and then add the requested offset to the start address
   * of the mapped region.
   */
   VkResult result = device_map(device, mem);
   if (result != VK_SUCCESS)
            *ppData = ((uint8_t *) mem->bo->map) + offset;
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_UnmapMemory(VkDevice _device,
         {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (mem == NULL)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_FlushMappedMemoryRanges(VkDevice _device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_InvalidateMappedMemoryRanges(VkDevice _device,
               {
         }
      static void
   get_image_memory_requirements(struct v3dv_image *image,
               {
      pMemoryRequirements->memoryRequirements = (VkMemoryRequirements) {
      .memoryTypeBits = 0x1,
   .alignment = image->planes[0].alignment,
               if (planeAspect != VK_IMAGE_ASPECT_NONE) {
      assert(image->format->plane_count > 1);
   /* Disjoint images should have a 0 non_disjoint_size */
                     VkMemoryRequirements *mem_reqs =
         mem_reqs->alignment = image->planes[plane].alignment;
               vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *req =
         req->requiresDedicatedAllocation = image->vk.external_handle_types != 0;
   req->prefersDedicatedAllocation = image->vk.external_handle_types != 0;
      }
   default:
      v3dv_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetImageMemoryRequirements2(VkDevice device,
               {
               VkImageAspectFlagBits planeAspect = VK_IMAGE_ASPECT_NONE;
   vk_foreach_struct_const(ext, pInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO: {
      VkImagePlaneMemoryRequirementsInfo *req =
         planeAspect = req->planeAspect;
      }
   default:
      v3dv_debug_ignored_stype(ext->sType);
                     }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetDeviceImageMemoryRequirementsKHR(
      VkDevice _device,
   const VkDeviceImageMemoryRequirements *pInfo,
      {
               struct v3dv_image image = { 0 };
            ASSERTED VkResult result =
                  /* From VkDeviceImageMemoryRequirements spec:
   *
   *   " planeAspect is a VkImageAspectFlagBits value specifying the aspect
   *     corresponding to the image plane to query. This parameter is ignored
   *     unless pCreateInfo::tiling is
   *     VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, or pCreateInfo::flags has
   *     VK_IMAGE_CREATE_DISJOINT_BIT set"
   *
   * We need to explicitly ignore that flag, or following asserts could be
   * triggered.
   */
   VkImageAspectFlagBits planeAspect =
      pInfo->pCreateInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT ||
   pInfo->pCreateInfo->flags & VK_IMAGE_CREATE_DISJOINT_BIT ?
            }
      static void
   bind_image_memory(const VkBindImageMemoryInfo *info)
   {
      V3DV_FROM_HANDLE(v3dv_image, image, info->image);
            /* Valid usage:
   *
   *   "memoryOffset must be an integer multiple of the alignment member of
   *    the VkMemoryRequirements structure returned from a call to
   *    vkGetImageMemoryRequirements with image"
   */
            uint64_t offset = info->memoryOffset;
   if (image->non_disjoint_size) {
      /* We only check for plane 0 as it is the only one that actually starts
   * at that offset
   */
   assert(offset % image->planes[0].alignment == 0);
   for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      image->planes[plane].mem = mem;
         } else {
      const VkBindImagePlaneMemoryInfo *plane_mem_info =
                  /*
   * From VkBindImagePlaneMemoryInfo spec:
   *
   *    "If the image’s tiling is VK_IMAGE_TILING_LINEAR or
   *     VK_IMAGE_TILING_OPTIMAL, then planeAspect must be a single valid
   *     format plane for the image"
   *
   * <skip>
   *
   *    "If the image’s tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT,
   *     then planeAspect must be a single valid memory plane for the
   *     image"
   *
   * So planeAspect should only refer to one plane.
   */
   uint8_t plane = v3dv_plane_from_aspect(plane_mem_info->planeAspect);
   assert(offset % image->planes[plane].alignment == 0);
   image->planes[plane].mem = mem;
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_BindImageMemory2(VkDevice _device,
               {
         #ifdef ANDROID
         V3DV_FROM_HANDLE(v3dv_device_memory, mem, pBindInfos[i].memory);
   V3DV_FROM_HANDLE(v3dv_device, device, _device);
   if (mem != NULL && mem->vk.ahardware_buffer) {
                                    struct u_gralloc_buffer_handle gr_handle = {
      .handle = handle,
   .pixel_stride = description.stride,
               VkResult result = v3dv_gralloc_to_drm_explicit_layout(
      device->gralloc,
   &gr_handle,
   image->android_explicit_layout,
   image->android_plane_layouts,
                     result = v3dv_update_image_layout(
      device, image, image->android_explicit_layout->drmFormatModifier,
      if (result != VK_SUCCESS)
      #endif
            const VkBindImageMemorySwapchainInfoKHR *swapchain_info =
      vk_find_struct_const(pBindInfos->pNext,
      #ifndef ANDROID
            struct v3dv_image *swapchain_image =
      v3dv_wsi_get_image_from_swapchain(swapchain_info->swapchain,
      /* Making the assumption that swapchain images are a single plane */
   assert(swapchain_image->plane_count == 1);
   VkBindImageMemoryInfo swapchain_bind = {
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
   .image = pBindInfos[i].image,
   .memory = v3dv_device_memory_to_handle(swapchain_image->planes[0].mem),
         #endif
         } else
   {
                        }
      void
   v3dv_buffer_init(struct v3dv_device *device,
                     {
      buffer->size = pCreateInfo->size;
   buffer->usage = pCreateInfo->usage;
      }
      static void
   get_buffer_memory_requirements(struct v3dv_buffer *buffer,
         {
      pMemoryRequirements->memoryRequirements = (VkMemoryRequirements) {
      .memoryTypeBits = 0x1,
   .alignment = buffer->alignment,
               /* UBO and SSBO may be read using ldunifa, which prefetches the next
   * 4 bytes after a read. If the buffer's size is exactly a multiple
   * of a page size and the shader reads the last 4 bytes with ldunifa
   * the prefetching would read out of bounds and cause an MMU error,
   * so we allocate extra space to avoid kernel error spamming.
   */
   bool can_ldunifa = buffer->usage &
               if (can_ldunifa && (buffer->size % 4096 == 0))
            vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *req =
         req->requiresDedicatedAllocation = false;
   req->prefersDedicatedAllocation = false;
      }
   default:
      v3dv_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetBufferMemoryRequirements2(VkDevice device,
               {
      V3DV_FROM_HANDLE(v3dv_buffer, buffer, pInfo->buffer);
      }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetDeviceBufferMemoryRequirementsKHR(
      VkDevice _device,
   const VkDeviceBufferMemoryRequirements *pInfo,
      {
               struct v3dv_buffer buffer = { 0 };
   v3dv_buffer_init(device, pInfo->pCreateInfo, &buffer, V3D_NON_COHERENT_ATOM_SIZE);
      }
      void
   v3dv_buffer_bind_memory(const VkBindBufferMemoryInfo *info)
   {
      V3DV_FROM_HANDLE(v3dv_buffer, buffer, info->buffer);
            /* Valid usage:
   *
   *   "memoryOffset must be an integer multiple of the alignment member of
   *    the VkMemoryRequirements structure returned from a call to
   *    vkGetBufferMemoryRequirements with buffer"
   */
   assert(info->memoryOffset % buffer->alignment == 0);
            buffer->mem = mem;
      }
         VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_BindBufferMemory2(VkDevice device,
               {
      for (uint32_t i = 0; i < bindInfoCount; i++)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateBuffer(VkDevice  _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
            /* We don't support any flags for now */
            buffer = vk_object_zalloc(&device->vk, pAllocator, sizeof(*buffer),
         if (buffer == NULL)
                     /* Limit allocations to 32-bit */
   const VkDeviceSize aligned_size = align64(buffer->size, buffer->alignment);
   if (aligned_size > UINT32_MAX || aligned_size < buffer->size) {
      vk_free(&device->vk.alloc, buffer);
                           }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyBuffer(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!buffer)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateFramebuffer(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
                     size_t size = sizeof(*framebuffer) +
         framebuffer = vk_object_zalloc(&device->vk, pAllocator, size,
         if (framebuffer == NULL)
            framebuffer->width = pCreateInfo->width;
   framebuffer->height = pCreateInfo->height;
   framebuffer->layers = pCreateInfo->layers;
            const VkFramebufferAttachmentsCreateInfo *imageless =
      vk_find_struct_const(pCreateInfo->pNext,
         framebuffer->attachment_count = pCreateInfo->attachmentCount;
   framebuffer->color_attachment_count = 0;
   for (uint32_t i = 0; i < framebuffer->attachment_count; i++) {
      if (!imageless) {
      framebuffer->attachments[i] =
         if (framebuffer->attachments[i]->vk.aspects & VK_IMAGE_ASPECT_COLOR_BIT)
      } else {
      assert(i < imageless->attachmentImageInfoCount);
   if (imageless->pAttachmentImageInfos[i].usage &
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
                                 }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyFramebuffer(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!fb)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetMemoryFdPropertiesKHR(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT:
      pMemoryFdProperties->memoryTypeBits =
            default:
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetMemoryFdKHR(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            assert(pGetFdInfo->sType == VK_STRUCTURE_TYPE_MEMORY_GET_FD_INFO_KHR);
   assert(pGetFdInfo->handleType == VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT ||
            int fd, ret;
   ret = drmPrimeHandleToFD(device->pdevice->render_fd,
               if (ret)
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateSampler(VkDevice _device,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
                     sampler = vk_object_zalloc(&device->vk, pAllocator, sizeof(*sampler),
         if (!sampler)
                     sampler->compare_enable = pCreateInfo->compareEnable;
            const VkSamplerCustomBorderColorCreateInfoEXT *bc_info =
      vk_find_struct_const(pCreateInfo->pNext,
         const VkSamplerYcbcrConversionInfo *ycbcr_conv_info =
                     if (ycbcr_conv_info) {
      VK_FROM_HANDLE(vk_ycbcr_conversion, conversion, ycbcr_conv_info->conversion);
   ycbcr_info = vk_format_get_ycbcr_info(conversion->state.format);
   if (ycbcr_info) {
      sampler->plane_count = ycbcr_info->n_planes;
                                       }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroySampler(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (!sampler)
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetDeviceMemoryCommitment(VkDevice device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetImageSparseMemoryRequirements(
      VkDevice device,
   VkImage image,
   uint32_t *pSparseMemoryRequirementCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetImageSparseMemoryRequirements2(
      VkDevice device,
   const VkImageSparseMemoryRequirementsInfo2 *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
         }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetDeviceImageSparseMemoryRequirementsKHR(
      VkDevice device,
   const VkDeviceImageMemoryRequirements *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
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
      VkDeviceAddress
   v3dv_GetBufferDeviceAddress(VkDevice device,
         {
      V3DV_FROM_HANDLE(v3dv_buffer, buffer, pInfo->buffer);
      }
      uint64_t
   v3dv_GetBufferOpaqueCaptureAddress(VkDevice device,
         {
      /* Not implemented */
      }
      uint64_t
   v3dv_GetDeviceMemoryOpaqueCaptureAddress(
      VkDevice device,
      {
      /* Not implemented */
      }
      VkResult
   v3dv_create_compute_pipeline_from_nir(struct v3dv_device *device,
                     {
               VkPipelineShaderStageCreateInfo set_event_cs_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_to_handle(&cs_m),
               VkComputePipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = set_event_cs_stage,
               VkResult result =
      v3dv_CreateComputePipelines(v3dv_device_to_handle(device), VK_NULL_HANDLE,
            }
