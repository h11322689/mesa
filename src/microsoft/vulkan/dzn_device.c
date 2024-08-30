   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_cmd_enqueue_entrypoints.h"
   #include "vk_debug_report.h"
   #include "vk_format.h"
   #include "vk_sync_dummy.h"
   #include "vk_util.h"
      #include "git_sha1.h"
      #include "util/u_debug.h"
   #include "util/disk_cache.h"
   #include "util/macros.h"
   #include "util/mesa-sha1.h"
   #include "util/u_dl.h"
      #include "util/driconf.h"
      #include "glsl_types.h"
      #include "dxil_validator.h"
      #include "git_sha1.h"
      #include <string.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <c99_alloca.h>
      #ifdef _WIN32
   #include <windows.h>
   #include <shlobj.h>
   #include "dzn_dxgi.h"
   #endif
      #include <directx/d3d12sdklayers.h>
      #if defined(VK_USE_PLATFORM_WIN32_KHR) || \
      defined(VK_USE_PLATFORM_WAYLAND_KHR) || \
   defined(VK_USE_PLATFORM_XCB_KHR) || \
      #define DZN_USE_WSI_PLATFORM
   #endif
      #define DZN_API_VERSION VK_MAKE_VERSION(1, 2, VK_HEADER_VERSION)
      #define MAX_TIER2_MEMORY_TYPES 3
      const VkExternalMemoryHandleTypeFlags opaque_external_flag =
   #ifdef _WIN32
         #else
         #endif
      static const struct vk_instance_extension_table instance_extensions = {
      .KHR_get_physical_device_properties2      = true,
      #ifdef DZN_USE_WSI_PLATFORM
      .KHR_surface                              = true,
      #endif
   #ifdef VK_USE_PLATFORM_WIN32_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XLIB_KHR
         #endif
      .EXT_debug_report                         = true,
      };
      static void
   dzn_physical_device_get_extensions(struct dzn_physical_device *pdev)
   {
      pdev->vk.supported_extensions = (struct vk_device_extension_table) {
      .KHR_16bit_storage                     = pdev->options4.Native16BitShaderOpsSupported,
   .KHR_bind_memory2                      = true,
   .KHR_create_renderpass2                = true,
   .KHR_dedicated_allocation              = true,
   .KHR_depth_stencil_resolve             = true,
   .KHR_descriptor_update_template        = true,
   .KHR_device_group                      = true,
   .KHR_draw_indirect_count               = true,
   .KHR_driver_properties                 = true,
   .KHR_dynamic_rendering                 = true,
   .KHR_external_memory                   = true,
   #ifdef _WIN32
         .KHR_external_memory_win32             = true,
   #else
         .KHR_external_memory_fd                = true,
   #endif
         .KHR_image_format_list                 = true,
   .KHR_imageless_framebuffer             = true,
   .KHR_get_memory_requirements2          = true,
   .KHR_maintenance1                      = true,
   .KHR_maintenance2                      = true,
   .KHR_maintenance3                      = true,
   .KHR_multiview                         = true,
   .KHR_relaxed_block_layout              = true,
   .KHR_sampler_mirror_clamp_to_edge      = true,
   .KHR_separate_depth_stencil_layouts    = true,
   .KHR_shader_draw_parameters            = true,
   .KHR_shader_float16_int8               = pdev->options4.Native16BitShaderOpsSupported,
   .KHR_shader_float_controls             = true,
   .KHR_shader_integer_dot_product        = true,
   .KHR_spirv_1_4                         = true,
   #ifdef DZN_USE_WSI_PLATFORM
         #endif
         .KHR_synchronization2                  = true,
   .KHR_timeline_semaphore                = true,
   .KHR_uniform_buffer_standard_layout    = true,
   #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
         #endif
         .EXT_scalar_block_layout               = true,
   .EXT_separate_stencil_usage            = true,
   .EXT_shader_subgroup_ballot            = true,
   .EXT_shader_subgroup_vote              = true,
   .EXT_subgroup_size_control             = true,
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_EnumerateInstanceExtensionProperties(const char *pLayerName,
               {
      /* We don't support any layers  */
   if (pLayerName)
            return vk_enumerate_instance_extension_properties(
      }
      static const struct debug_control dzn_debug_options[] = {
      { "sync", DZN_DEBUG_SYNC },
   { "nir", DZN_DEBUG_NIR },
   { "dxil", DZN_DEBUG_DXIL },
   { "warp", DZN_DEBUG_WARP },
   { "internal", DZN_DEBUG_INTERNAL },
   { "signature", DZN_DEBUG_SIG },
   { "gbv", DZN_DEBUG_GBV },
   { "d3d12", DZN_DEBUG_D3D12 },
   { "debugger", DZN_DEBUG_DEBUGGER },
   { "redirects", DZN_DEBUG_REDIRECTS },
   { "bindless", DZN_DEBUG_BINDLESS },
   { "nobindless", DZN_DEBUG_NO_BINDLESS },
      };
      static void
   dzn_physical_device_destroy(struct vk_physical_device *physical)
   {
      struct dzn_physical_device *pdev = container_of(physical, struct dzn_physical_device, vk);
            if (pdev->dev)
            if (pdev->dev10)
            if (pdev->dev11)
            if (pdev->dev12)
         #if D3D12_SDK_VERSION >= 611
      if (pdev->dev13)
      #endif
         if (pdev->adapter)
            dzn_wsi_finish(pdev);
   vk_physical_device_finish(&pdev->vk);
      }
      static void
   dzn_instance_destroy(struct dzn_instance *instance, const VkAllocationCallbacks *alloc)
   {
      if (!instance)
                  #ifdef _WIN32
         #endif
         if (instance->factory)
            if (instance->d3d12_mod)
               }
      #ifdef _WIN32
   extern IMAGE_DOS_HEADER __ImageBase;
   static const char *
   try_find_d3d12core_next_to_self(char *path, size_t path_arr_size)
   {
      uint32_t path_size = GetModuleFileNameA((HINSTANCE)&__ImageBase,
         if (!path_arr_size || path_size == path_arr_size) {
      mesa_loge("Unable to get path to self\n");
               char *last_slash = strrchr(path, '\\');
   if (!last_slash) {
      mesa_loge("Unable to get path to self\n");
               *(last_slash + 1) = '\0';
   if (strcat_s(path, path_arr_size, "D3D12Core.dll") != 0) {
      mesa_loge("Unable to get path to D3D12Core.dll next to self\n");
               if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) {
                     }
   #endif
      static ID3D12DeviceFactory *
   try_create_device_factory(struct util_dl_library *d3d12_mod)
   {
      /* A device factory allows us to isolate things like debug layer enablement from other callers,
   * and can potentially even refer to a different D3D12 redist implementation from others.
   */
            PFN_D3D12_GET_INTERFACE D3D12GetInterface = (PFN_D3D12_GET_INTERFACE)util_dl_get_proc_address(d3d12_mod, "D3D12GetInterface");
   if (!D3D12GetInterface) {
      mesa_loge("Failed to retrieve D3D12GetInterface\n");
            #ifdef _WIN32
      /* First, try to create a device factory from a DLL-parallel D3D12Core.dll */
   ID3D12SDKConfiguration *sdk_config = NULL;
   if (SUCCEEDED(D3D12GetInterface(&CLSID_D3D12SDKConfiguration, &IID_ID3D12SDKConfiguration, (void **)&sdk_config))) {
      ID3D12SDKConfiguration1 *sdk_config1 = NULL;
   if (SUCCEEDED(IUnknown_QueryInterface(sdk_config, &IID_ID3D12SDKConfiguration1, (void **)&sdk_config1))) {
      char self_path[MAX_PATH];
   const char *d3d12core_path = try_find_d3d12core_next_to_self(self_path, sizeof(self_path));
   if (d3d12core_path) {
      if (SUCCEEDED(ID3D12SDKConfiguration1_CreateDeviceFactory(sdk_config1, D3D12_PREVIEW_SDK_VERSION, d3d12core_path, &IID_ID3D12DeviceFactory, (void **)&factory)) ||
      SUCCEEDED(ID3D12SDKConfiguration1_CreateDeviceFactory(sdk_config1, D3D12_SDK_VERSION, d3d12core_path, &IID_ID3D12DeviceFactory, (void **)&factory))) {
   ID3D12SDKConfiguration_Release(sdk_config);
   ID3D12SDKConfiguration1_Release(sdk_config1);
                  /* Nope, seems we don't have a matching D3D12Core.dll next to ourselves */
               /* It's possible there's a D3D12Core.dll next to the .exe, for development/testing purposes. If so, we'll be notified
   * by environment variables what the relative path is and the version to use.
   */
   const char *d3d12core_relative_path = getenv("DZN_AGILITY_RELATIVE_PATH");
   const char *d3d12core_sdk_version = getenv("DZN_AGILITY_SDK_VERSION");
   if (d3d12core_relative_path && d3d12core_sdk_version) {
         }
         #endif
         (void)D3D12GetInterface(&CLSID_D3D12DeviceFactory, &IID_ID3D12DeviceFactory, (void **)&factory);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyInstance(VkInstance instance,
         {
         }
      static void
   dzn_physical_device_init_uuids(struct dzn_physical_device *pdev)
   {
               struct mesa_sha1 sha1_ctx;
   uint8_t sha1[SHA1_DIGEST_LENGTH];
            /* The pipeline cache UUID is used for determining when a pipeline cache is
   * invalid. Our cache is device-agnostic, but it does depend on the features
   * provided by the D3D12 driver, so let's hash the build ID plus some
   * caps that might impact our NIR lowering passes.
   */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx,  mesa_version, strlen(mesa_version));
   disk_cache_get_function_identifier(dzn_physical_device_init_uuids, &sha1_ctx);
   _mesa_sha1_update(&sha1_ctx,  &pdev->options, sizeof(pdev->options));
   _mesa_sha1_update(&sha1_ctx,  &pdev->options2, sizeof(pdev->options2));
   _mesa_sha1_final(&sha1_ctx, sha1);
            /* The driver UUID is used for determining sharability of images and memory
   * between two Vulkan instances in separate processes.  People who want to
   * share memory need to also check the device UUID (below) so all this
   * needs to be is the build-id.
   */
   _mesa_sha1_compute(mesa_version, strlen(mesa_version), sha1);
            /* The device UUID uniquely identifies the given device within the machine. */
   _mesa_sha1_init(&sha1_ctx);
   _mesa_sha1_update(&sha1_ctx, &pdev->desc.vendor_id, sizeof(pdev->desc.vendor_id));
   _mesa_sha1_update(&sha1_ctx, &pdev->desc.device_id, sizeof(pdev->desc.device_id));
   _mesa_sha1_update(&sha1_ctx, &pdev->desc.subsys_id, sizeof(pdev->desc.subsys_id));
   _mesa_sha1_update(&sha1_ctx, &pdev->desc.revision, sizeof(pdev->desc.revision));
   _mesa_sha1_final(&sha1_ctx, sha1);
      }
      const struct vk_pipeline_cache_object_ops *const dzn_pipeline_cache_import_ops[] = {
      &dzn_cached_blob_ops,
      };
      static void
   dzn_physical_device_cache_caps(struct dzn_physical_device *pdev)
   {
      D3D_FEATURE_LEVEL checklist[] = {
      D3D_FEATURE_LEVEL_11_0,
   D3D_FEATURE_LEVEL_11_1,
   D3D_FEATURE_LEVEL_12_0,
   D3D_FEATURE_LEVEL_12_1,
               D3D12_FEATURE_DATA_FEATURE_LEVELS levels = {
      .NumFeatureLevels = ARRAY_SIZE(checklist),
               ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_FEATURE_LEVELS, &levels, sizeof(levels));
            static const D3D_SHADER_MODEL valid_shader_models[] = {
      D3D_SHADER_MODEL_6_7, D3D_SHADER_MODEL_6_6, D3D_SHADER_MODEL_6_5, D3D_SHADER_MODEL_6_4,
      };
   for (UINT i = 0; i < ARRAY_SIZE(valid_shader_models); ++i) {
      D3D12_FEATURE_DATA_SHADER_MODEL shader_model = { valid_shader_models[i] };
   if (SUCCEEDED(ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_SHADER_MODEL, &shader_model, sizeof(shader_model)))) {
      pdev->shader_model = shader_model.HighestShaderModel;
                  D3D_ROOT_SIGNATURE_VERSION root_sig_versions[] = {
      D3D_ROOT_SIGNATURE_VERSION_1_2,
      };
   for (UINT i = 0; i < ARRAY_SIZE(root_sig_versions); ++i) {
      D3D12_FEATURE_DATA_ROOT_SIGNATURE root_sig = { root_sig_versions[i] };
   if (SUCCEEDED(ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_ROOT_SIGNATURE, &root_sig, sizeof(root_sig)))) {
      pdev->root_sig_version = root_sig.HighestVersion;
                  ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_ARCHITECTURE1, &pdev->architecture, sizeof(pdev->architecture));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS, &pdev->options, sizeof(pdev->options));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS1, &pdev->options1, sizeof(pdev->options1));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS2, &pdev->options2, sizeof(pdev->options2));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS3, &pdev->options3, sizeof(pdev->options3));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS4, &pdev->options4, sizeof(pdev->options4));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS12, &pdev->options12, sizeof(pdev->options12));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS13, &pdev->options13, sizeof(pdev->options13));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS14, &pdev->options14, sizeof(pdev->options14));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS15, &pdev->options15, sizeof(pdev->options15));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS16, &pdev->options16, sizeof(pdev->options16));
   ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS17, &pdev->options17, sizeof(pdev->options17));
   if (FAILED(ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_D3D12_OPTIONS19, &pdev->options19, sizeof(pdev->options19)))) {
      pdev->options19.MaxSamplerDescriptorHeapSize = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
   pdev->options19.MaxSamplerDescriptorHeapSizeWithStaticSamplers = pdev->options19.MaxSamplerDescriptorHeapSize;
      }
   {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT a4b4g4r4_support = {
         };
   pdev->support_a4b4g4r4 =
               pdev->queue_families[pdev->queue_family_count++] = (struct dzn_queue_family) {
      .props = {
      .queueFlags = VK_QUEUE_GRAPHICS_BIT |
               .queueCount = 4,
   .timestampValidBits = 64,
      },
   .desc = {
                     pdev->queue_families[pdev->queue_family_count++] = (struct dzn_queue_family) {
      .props = {
      .queueFlags = VK_QUEUE_COMPUTE_BIT |
         .queueCount = 8,
   .timestampValidBits = 64,
      },
   .desc = {
                              D3D12_COMMAND_QUEUE_DESC queue_desc = {
      .Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
   .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
   .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
               ID3D12CommandQueue *cmdqueue;
   ID3D12Device1_CreateCommandQueue(pdev->dev, &queue_desc,
                  uint64_t ts_freq;
   ID3D12CommandQueue_GetTimestampFrequency(cmdqueue, &ts_freq);
   pdev->timestamp_period = 1000000000.0f / ts_freq;
      }
      static void
   dzn_physical_device_init_memory(struct dzn_physical_device *pdev)
   {
               mem->memoryHeapCount = 1;
   mem->memoryHeaps[0] = (VkMemoryHeap) {
      .size = pdev->desc.shared_system_memory,
               mem->memoryTypes[mem->memoryTypeCount++] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            };
   mem->memoryTypes[mem->memoryTypeCount++] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            .heapIndex = 0,
            if (!pdev->architecture.UMA) {
      mem->memoryHeaps[mem->memoryHeapCount++] = (VkMemoryHeap) {
      .size = pdev->desc.dedicated_video_memory,
      };
   mem->memoryTypes[mem->memoryTypeCount++] = (VkMemoryType) {
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
         } else {
      mem->memoryHeaps[0].flags |= VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;
   mem->memoryTypes[0].propertyFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   mem->memoryTypes[1].propertyFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   /* Get one non-CPU-accessible memory type for shared resources to use */
   mem->memoryTypes[mem->memoryTypeCount++] = (VkMemoryType){
      .propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                           if (pdev->options.ResourceHeapTier == D3D12_RESOURCE_HEAP_TIER_1) {
      unsigned oldMemoryTypeCount = mem->memoryTypeCount;
                     mem->memoryTypeCount = 0;
   for (unsigned oldMemoryTypeIdx = 0; oldMemoryTypeIdx < oldMemoryTypeCount; ++oldMemoryTypeIdx) {
      D3D12_HEAP_FLAGS flags[] = {
      D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
   D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES,
   /* Note: Vulkan requires *all* images to come from the same memory type as long as
   * the tiling property (and a few other misc properties) are the same. So, this
   * non-RT/DS texture flag will only be used for TILING_LINEAR textures, which
   * can't be render targets.
   */
      };
   for (int i = 0; i < ARRAY_SIZE(flags); ++i) {
      D3D12_HEAP_FLAGS flag = flags[i];
   pdev->heap_flags_for_mem_type[mem->memoryTypeCount] = flag;
   mem->memoryTypes[mem->memoryTypeCount] = oldMemoryTypes[oldMemoryTypeIdx];
               }
      static D3D12_HEAP_FLAGS
   dzn_physical_device_get_heap_flags_for_mem_type(const struct dzn_physical_device *pdev,
         {
         }
      uint32_t
   dzn_physical_device_get_mem_type_mask_for_resource(const struct dzn_physical_device *pdev,
               {
      if (pdev->options.ResourceHeapTier > D3D12_RESOURCE_HEAP_TIER_1 && !shared)
            D3D12_HEAP_FLAGS deny_flag = D3D12_HEAP_FLAG_NONE;
   if (pdev->options.ResourceHeapTier <= D3D12_RESOURCE_HEAP_TIER_1) {
      if (desc->Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
         else if (desc->Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
         else
               uint32_t mask = 0;
   for (unsigned i = 0; i < pdev->memory.memoryTypeCount; ++i) {
      if (shared && (pdev->memory.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
         if ((pdev->heap_flags_for_mem_type[i] & deny_flag) == D3D12_HEAP_FLAG_NONE)
      }
      }
      static uint32_t
   dzn_physical_device_get_max_mip_level(bool is_3d)
   {
         }
      static uint32_t
   dzn_physical_device_get_max_extent(bool is_3d)
   {
                  }
      static uint32_t
   dzn_physical_device_get_max_array_layers()
   {
         }
      static void
   dzn_physical_device_get_features(const struct dzn_physical_device *pdev,
         {
               bool support_descriptor_indexing = pdev->shader_model >= D3D_SHADER_MODEL_6_6 &&
         bool support_8bit = driQueryOptionb(&instance->dri_options, "dzn_enable_8bit_loads_stores") &&
            *features = (struct vk_features) {
      .robustBufferAccess = true, /* This feature is mandatory */
   .fullDrawIndexUint32 = false,
   .imageCubeArray = true,
   .independentBlend = true,
   .geometryShader = true,
   .tessellationShader = false,
   .sampleRateShading = true,
   .dualSrcBlend = false,
   .logicOp = false,
   .multiDrawIndirect = true,
   .drawIndirectFirstInstance = true,
   .depthClamp = true,
   .depthBiasClamp = true,
   .fillModeNonSolid = true,
   .depthBounds = pdev->options2.DepthBoundsTestSupported,
   .wideLines = driQueryOptionb(&instance->dri_options, "dzn_claim_wide_lines"),
   .largePoints = false,
   .alphaToOne = false,
   .multiViewport = false,
   .samplerAnisotropy = true,
   .textureCompressionETC2 = false,
   .textureCompressionASTC_LDR = false,
   .textureCompressionBC = true,
   .occlusionQueryPrecise = true,
   .pipelineStatisticsQuery = true,
   .vertexPipelineStoresAndAtomics = true,
   .fragmentStoresAndAtomics = true,
   .shaderTessellationAndGeometryPointSize = false,
   .shaderImageGatherExtended = true,
   .shaderStorageImageExtendedFormats = pdev->options.TypedUAVLoadAdditionalFormats,
   .shaderStorageImageMultisample = false,
   .shaderStorageImageReadWithoutFormat = true,
   .shaderStorageImageWriteWithoutFormat = true,
   .shaderUniformBufferArrayDynamicIndexing = true,
   .shaderSampledImageArrayDynamicIndexing = true,
   .shaderStorageBufferArrayDynamicIndexing = true,
   .shaderStorageImageArrayDynamicIndexing = true,
   .shaderClipDistance = true,
   .shaderCullDistance = true,
   .shaderFloat64 = pdev->options.DoublePrecisionFloatShaderOps,
   .shaderInt64 = pdev->options1.Int64ShaderOps,
   .shaderInt16 = pdev->options4.Native16BitShaderOpsSupported,
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
            .storageBuffer16BitAccess           = pdev->options4.Native16BitShaderOpsSupported,
   .uniformAndStorageBuffer16BitAccess = pdev->options4.Native16BitShaderOpsSupported,
   .storagePushConstant16              = false,
   .storageInputOutput16               = false,
   .multiview                          = true,
   .multiviewGeometryShader            = true,
   .multiviewTessellationShader        = false,
   .variablePointersStorageBuffer      = false,
   .variablePointers                   = false,
   .protectedMemory                    = false,
   .samplerYcbcrConversion             = false,
            .samplerMirrorClampToEdge           = true,
   .drawIndirectCount                  = true,
   .storageBuffer8BitAccess            = support_8bit,
   .uniformAndStorageBuffer8BitAccess  = support_8bit,
   .storagePushConstant8               = support_8bit,
   .shaderBufferInt64Atomics           = false,
   .shaderSharedInt64Atomics           = false,
   .shaderFloat16                      = pdev->options4.Native16BitShaderOpsSupported,
            .descriptorIndexing                                   = support_descriptor_indexing,
   .shaderInputAttachmentArrayDynamicIndexing            = true,
   .shaderUniformTexelBufferArrayDynamicIndexing         = true,
   .shaderStorageTexelBufferArrayDynamicIndexing         = true,
   .shaderUniformBufferArrayNonUniformIndexing           = support_descriptor_indexing,
   .shaderSampledImageArrayNonUniformIndexing            = support_descriptor_indexing,
   .shaderStorageBufferArrayNonUniformIndexing           = support_descriptor_indexing,
   .shaderStorageImageArrayNonUniformIndexing            = support_descriptor_indexing,
   .shaderInputAttachmentArrayNonUniformIndexing         = support_descriptor_indexing,
   .shaderUniformTexelBufferArrayNonUniformIndexing      = support_descriptor_indexing,
   .shaderStorageTexelBufferArrayNonUniformIndexing      = support_descriptor_indexing,
   .descriptorBindingUniformBufferUpdateAfterBind        = support_descriptor_indexing,
   .descriptorBindingSampledImageUpdateAfterBind         = support_descriptor_indexing,
   .descriptorBindingStorageImageUpdateAfterBind         = support_descriptor_indexing,
   .descriptorBindingStorageBufferUpdateAfterBind        = support_descriptor_indexing,
   .descriptorBindingUniformTexelBufferUpdateAfterBind   = support_descriptor_indexing,
   .descriptorBindingStorageTexelBufferUpdateAfterBind   = support_descriptor_indexing,
   .descriptorBindingUpdateUnusedWhilePending            = support_descriptor_indexing,
   .descriptorBindingPartiallyBound                      = support_descriptor_indexing,
   .descriptorBindingVariableDescriptorCount             = support_descriptor_indexing,
            .samplerFilterMinmax                = false,
   .scalarBlockLayout                  = true,
   .imagelessFramebuffer               = true,
   .uniformBufferStandardLayout        = true,
   .shaderSubgroupExtendedTypes        = true,
   .separateDepthStencilLayouts        = true,
   .hostQueryReset                     = true,
   .timelineSemaphore                  = true,
   .bufferDeviceAddress                = false,
   .bufferDeviceAddressCaptureReplay   = false,
   .bufferDeviceAddressMultiDevice     = false,
   .vulkanMemoryModel                  = false,
   .vulkanMemoryModelDeviceScope       = false,
   .vulkanMemoryModelAvailabilityVisibilityChains = false,
   .shaderOutputViewportIndex          = false,
   .shaderOutputLayer                  = false,
            .robustImageAccess                  = false,
   .inlineUniformBlock                 = false,
   .descriptorBindingInlineUniformBlockUpdateAfterBind = false,
   .pipelineCreationCacheControl       = false,
   .privateData                        = true,
   .shaderDemoteToHelperInvocation     = false,
   .shaderTerminateInvocation          = false,
   .subgroupSizeControl                = pdev->options1.WaveOps && pdev->shader_model >= D3D_SHADER_MODEL_6_6,
   .computeFullSubgroups               = true,
   .synchronization2                   = true,
   .textureCompressionASTC_HDR         = false,
   .shaderZeroInitializeWorkgroupMemory = false,
   .dynamicRendering                   = true,
   .shaderIntegerDotProduct            = true,
            .vertexAttributeInstanceRateDivisor = true,
         }
      static VkResult
   dzn_physical_device_create(struct vk_instance *instance,
               {
      struct dzn_physical_device *pdev =
      vk_zalloc(&instance->alloc, sizeof(*pdev), 8,
         if (!pdev)
            struct vk_physical_device_dispatch_table dispatch_table;
   vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table,
               vk_physical_device_dispatch_table_from_entrypoints(&dispatch_table,
                  VkResult result =
      vk_physical_device_init(&pdev->vk, instance,
            if (result != VK_SUCCESS) {
      vk_free(&instance->alloc, pdev);
               pdev->desc = *desc;
   pdev->adapter = adapter;
   IUnknown_AddRef(adapter);
                              uint32_t num_sync_types = 0;
   pdev->sync_types[num_sync_types++] = &dzn_sync_type;
   pdev->sync_types[num_sync_types++] = &dzn_instance->sync_binary_type.sync;
   pdev->sync_types[num_sync_types++] = &vk_sync_dummy_type;
   pdev->sync_types[num_sync_types] = NULL;
   assert(num_sync_types <= MAX_SYNC_TYPES);
                     pdev->dev = d3d12_create_device(dzn_instance->d3d12_mod,
                     if (!pdev->dev) {
      list_del(&pdev->vk.link);
   dzn_physical_device_destroy(&pdev->vk);
               if (FAILED(ID3D12Device1_QueryInterface(pdev->dev, &IID_ID3D12Device10, (void **)&pdev->dev10)))
         if (FAILED(ID3D12Device1_QueryInterface(pdev->dev, &IID_ID3D12Device11, (void **)&pdev->dev11)))
         if (FAILED(ID3D12Device1_QueryInterface(pdev->dev, &IID_ID3D12Device12, (void **)&pdev->dev12)))
      #if D3D12_SDK_VERSION >= 611
      if (FAILED(ID3D12Device1_QueryInterface(pdev->dev, &IID_ID3D12Device13, (void **)&pdev->dev13)))
      #endif
      dzn_physical_device_cache_caps(pdev);
   dzn_physical_device_init_memory(pdev);
            result = dzn_wsi_init(pdev);
   if (result != VK_SUCCESS || !pdev->dev) {
      list_del(&pdev->vk.link);
   dzn_physical_device_destroy(&pdev->vk);
               dzn_physical_device_get_extensions(pdev);
   if (driQueryOptionb(&dzn_instance->dri_options, "dzn_enable_8bit_loads_stores") &&
      pdev->options4.Native16BitShaderOpsSupported)
      if (dzn_instance->debug_flags & DZN_DEBUG_NO_BINDLESS)
                     }
      static DXGI_FORMAT
   dzn_get_most_capable_format_for_casting(VkFormat format, VkImageCreateFlags create_flags)
   {
      enum pipe_format pfmt = vk_format_to_pipe_format(format);
   bool block_compressed = util_format_is_compressed(pfmt);
   if (block_compressed &&
      !(create_flags & VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT))
      unsigned blksz = util_format_get_blocksize(pfmt);
   switch (blksz) {
   case 1: return DXGI_FORMAT_R8_UNORM;
   case 2: return DXGI_FORMAT_R16_UNORM;
   case 4: return DXGI_FORMAT_R32_FLOAT;
   case 8: return DXGI_FORMAT_R32G32_FLOAT;
   case 12: return DXGI_FORMAT_R32G32B32_FLOAT;
   case 16: return DXGI_FORMAT_R32G32B32A32_FLOAT;
   default: unreachable("Unsupported format bit size");;
      }
      D3D12_FEATURE_DATA_FORMAT_SUPPORT
   dzn_physical_device_get_format_support(struct dzn_physical_device *pdev,
               {
      VkImageUsageFlags usage =
      vk_format_is_depth_or_stencil(format) ?
               if (vk_format_has_depth(format))
         if (vk_format_has_stencil(format))
            D3D12_FEATURE_DATA_FORMAT_SUPPORT dfmt_info = {
   .Format = dzn_image_get_dxgi_format(pdev, format, usage, aspects),
            /* KHR_maintenance2: If an image is created with the extended usage flag
   * (or if properties are queried with that flag), then if any compatible
   * format can support a given usage, it should be considered supported.
   * With the exception of depth, which are limited in their cast set,
   * we can do this by just picking a single most-capable format to query
   * the support for, instead of the originally requested format. */
   if (aspects == 0 && dfmt_info.Format != DXGI_FORMAT_UNKNOWN &&
      (create_flags & VK_IMAGE_CREATE_EXTENDED_USAGE_BIT)) {
               ASSERTED HRESULT hres =
      ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_FORMAT_SUPPORT,
               if (usage != VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            /* Depth/stencil resources have different format when they're accessed
   * as textures, query the capabilities for this format too.
   */
   dzn_foreach_aspect(aspect, aspects) {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT dfmt_info2 = {
   .Format = dzn_image_get_dxgi_format(pdev, format, 0, aspect),
            hres = ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_FORMAT_SUPPORT,
            #define DS_SRV_FORMAT_SUPPORT1_MASK \
         (D3D12_FORMAT_SUPPORT1_SHADER_LOAD | \
      D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE | \
   D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_COMPARISON | \
   D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE_MONO_TEXT | \
   D3D12_FORMAT_SUPPORT1_MULTISAMPLE_RESOLVE | \
   D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD | \
   D3D12_FORMAT_SUPPORT1_SHADER_GATHER | \
               dfmt_info.Support1 |= dfmt_info2.Support1 & DS_SRV_FORMAT_SUPPORT1_MASK;
                  }
      static void
   dzn_physical_device_get_format_properties(struct dzn_physical_device *pdev,
               {
      D3D12_FEATURE_DATA_FORMAT_SUPPORT dfmt_info =
                  vk_foreach_struct(ext, properties->pNext) {
                  if (dfmt_info.Format == DXGI_FORMAT_UNKNOWN) {
      if (dzn_graphics_pipeline_patch_vi_format(format) != format)
      *base_props = (VkFormatProperties){
            else
                     *base_props = (VkFormatProperties) {
      .linearTilingFeatures = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
   .optimalTilingFeatures = VK_FORMAT_FEATURE_TRANSFER_SRC_BIT | VK_FORMAT_FEATURE_TRANSFER_DST_BIT,
               if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER)
         #define TEX_FLAGS (D3D12_FORMAT_SUPPORT1_TEXTURE1D | \
                        if ((dfmt_info.Support1 & TEX_FLAGS) &&
      (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD)) {
   base_props->optimalTilingFeatures |=
               if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE) {
      base_props->optimalTilingFeatures |=
               if ((dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) &&
      (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW)) {
   base_props->optimalTilingFeatures |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
   if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_BUFFER)
            #define ATOMIC_FLAGS (D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_ADD | \
                        D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_BITWISE_OPS | \
   D3D12_FORMAT_SUPPORT2_UAV_ATOMIC_COMPARE_STORE_OR_COMPARE_EXCHANGE | \
   if ((dfmt_info.Support2 & ATOMIC_FLAGS) == ATOMIC_FLAGS) {
      base_props->optimalTilingFeatures |= VK_FORMAT_FEATURE_STORAGE_IMAGE_ATOMIC_BIT;
               if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_BUFFER)
            /* Color/depth/stencil attachment cap implies input attachement cap, and input
   * attachment loads are lowered to texture loads in dozen, hence the requirement
   * to have shader-load support.
   */
   if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) {
      if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) {
      base_props->optimalTilingFeatures |=
               if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_BLENDABLE)
            if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) {
      base_props->optimalTilingFeatures |=
                  /* B4G4R4A4 support is required, but d3d12 doesn't support it. The needed
   * d3d12 format would be A4R4G4B4. We map this format to d3d12's B4G4R4A4,
   * which is Vulkan's A4R4G4B4, and adjust the SRV component-mapping to fake
   * B4G4R4A4, but that forces us to limit the usage to sampling, which,
   * luckily, is exactly what we need to support the required features.
   *
   * However, since this involves swizzling the alpha channel, it can cause
   * problems for border colors. Fortunately, d3d12 added an A4B4G4R4 format,
   * which still isn't quite right (it'd be Vulkan R4G4B4A4), but can be
   * swizzled by just swapping R and B, so no border color issues arise.
   */
   if (format == VK_FORMAT_B4G4R4A4_UNORM_PACK16) {
      VkFormatFeatureFlags bgra4_req_features =
      VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
   VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
   VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
   VK_FORMAT_FEATURE_BLIT_SRC_BIT |
      base_props->optimalTilingFeatures &= bgra4_req_features;
   base_props->bufferFeatures =
               /* depth/stencil format shouldn't advertise buffer features */
   if (vk_format_is_depth_or_stencil(format))
      }
      static VkResult
   dzn_physical_device_get_image_format_properties(struct dzn_physical_device *pdev,
               {
      const VkPhysicalDeviceExternalImageFormatInfo *external_info = NULL;
                              /* Extract input structs */
   vk_foreach_struct_const(s, info->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_IMAGE_FORMAT_INFO:
      external_info = (const VkPhysicalDeviceExternalImageFormatInfo *)s;
      case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
      usage |= ((const VkImageStencilUsageCreateInfo *)s)->stencilUsage;
      default:
      dzn_debug_ignored_stype(s->sType);
                           /* Extract output structs */
   vk_foreach_struct(s, properties->pNext) {
      switch (s->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_IMAGE_FORMAT_PROPERTIES:
      external_props = (VkExternalImageFormatProperties *)s;
   external_props->externalMemoryProperties = (VkExternalMemoryProperties) { 0 };
      default:
      dzn_debug_ignored_stype(s->sType);
                  if (external_info && external_info->handleType != 0) {
      const VkExternalMemoryHandleTypeFlags d3d12_resource_handle_types =
         const VkExternalMemoryHandleTypeFlags d3d11_texture_handle_types =
         const VkExternalMemoryFeatureFlags import_export_feature_flags =
         const VkExternalMemoryFeatureFlags dedicated_feature_flags =
            switch (external_info->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT:
      external_props->externalMemoryProperties.compatibleHandleTypes = d3d11_texture_handle_types;
   external_props->externalMemoryProperties.exportFromImportedHandleTypes = d3d11_texture_handle_types;
   external_props->externalMemoryProperties.externalMemoryFeatures = dedicated_feature_flags;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
      external_props->externalMemoryProperties.compatibleHandleTypes = d3d12_resource_handle_types;
   external_props->externalMemoryProperties.exportFromImportedHandleTypes = d3d12_resource_handle_types;
   external_props->externalMemoryProperties.externalMemoryFeatures = dedicated_feature_flags;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
      external_props->externalMemoryProperties.compatibleHandleTypes =
      external_props->externalMemoryProperties.exportFromImportedHandleTypes =
         #ifdef _WIN32
         #else
         #endif
            external_props->externalMemoryProperties.compatibleHandleTypes = d3d11_texture_handle_types;
   external_props->externalMemoryProperties.exportFromImportedHandleTypes = d3d11_texture_handle_types;
      #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
         case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      if (pdev->dev13) {
      external_props->externalMemoryProperties.compatibleHandleTypes =
      external_props->externalMemoryProperties.exportFromImportedHandleTypes =
      external_props->externalMemoryProperties.externalMemoryFeatures = import_export_feature_flags;
         #endif
         default:
                  /* Linear textures not supported, but there's nothing else we can deduce from just a handle type */
   if (info->tiling != VK_IMAGE_TILING_OPTIMAL &&
      external_info->handleType != VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT)
            if (info->tiling != VK_IMAGE_TILING_OPTIMAL &&
      (usage & ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)))
         if (info->tiling != VK_IMAGE_TILING_OPTIMAL &&
      vk_format_is_depth_or_stencil(info->format))
         D3D12_FEATURE_DATA_FORMAT_SUPPORT dfmt_info =
         if (dfmt_info.Format == DXGI_FORMAT_UNKNOWN)
            bool is_bgra4 = info->format == VK_FORMAT_B4G4R4A4_UNORM_PACK16 &&
            if ((info->type == VK_IMAGE_TYPE_1D && !(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE1D)) ||
      (info->type == VK_IMAGE_TYPE_2D && !(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D)) ||
   (info->type == VK_IMAGE_TYPE_3D && !(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE3D)) ||
   ((info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   !(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURECUBE)))
         /* Due to extended capability querying, we might see 1D support for BC, but we don't actually have it */
   if (vk_format_is_block_compressed(info->format) && info->type == VK_IMAGE_TYPE_1D)
            if ((usage & VK_IMAGE_USAGE_SAMPLED_BIT) &&
      /* Note: format support for SAMPLED is not necessarily accurate for integer formats */
   !(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD))
         if ((usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) &&
      (!(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_LOAD) || is_bgra4))
         if ((usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) &&
      (!(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) || is_bgra4))
         if ((usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
      (!(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) || is_bgra4))
         if ((usage & VK_IMAGE_USAGE_STORAGE_BIT) &&
      (!(dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) || is_bgra4))
         if (info->type == VK_IMAGE_TYPE_3D && info->tiling != VK_IMAGE_TILING_OPTIMAL)
            bool is_3d = info->type == VK_IMAGE_TYPE_3D;
            if (info->tiling == VK_IMAGE_TILING_OPTIMAL &&
      dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_MIP)
      else
            if (info->tiling == VK_IMAGE_TILING_OPTIMAL && info->type != VK_IMAGE_TYPE_3D)
         else
            switch (info->type) {
   case VK_IMAGE_TYPE_1D:
      properties->imageFormatProperties.maxExtent.width = max_extent;
   properties->imageFormatProperties.maxExtent.height = 1;
   properties->imageFormatProperties.maxExtent.depth = 1;
      case VK_IMAGE_TYPE_2D:
      properties->imageFormatProperties.maxExtent.width = max_extent;
   properties->imageFormatProperties.maxExtent.height = max_extent;
   properties->imageFormatProperties.maxExtent.depth = 1;
      case VK_IMAGE_TYPE_3D:
      properties->imageFormatProperties.maxExtent.width = max_extent;
   properties->imageFormatProperties.maxExtent.height = max_extent;
   properties->imageFormatProperties.maxExtent.depth = max_extent;
      default:
                  /* From the Vulkan 1.0 spec, section 34.1.1. Supported Sample Counts:
   *
   * sampleCounts will be set to VK_SAMPLE_COUNT_1_BIT if at least one of the
   * following conditions is true:
   *
   *   - tiling is VK_IMAGE_TILING_LINEAR
   *   - type is not VK_IMAGE_TYPE_2D
   *   - flags contains VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
   *   - neither the VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT flag nor the
   *     VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT flag in
   *     VkFormatProperties::optimalTilingFeatures returned by
   *     vkGetPhysicalDeviceFormatProperties is set.
   *
   * D3D12 has a few more constraints:
   *   - no UAVs on multisample resources
   */
   properties->imageFormatProperties.sampleCounts = VK_SAMPLE_COUNT_1_BIT;
   if (info->tiling != VK_IMAGE_TILING_LINEAR &&
      info->type == VK_IMAGE_TYPE_2D &&
   !(info->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) &&
   (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_MULTISAMPLE_LOAD) &&
   !is_bgra4 &&
   !(usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
   for (uint32_t s = VK_SAMPLE_COUNT_2_BIT; s < VK_SAMPLE_COUNT_64_BIT; s <<= 1) {
      D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_info = {
      .Format = dfmt_info.Format,
               HRESULT hres =
      ID3D12Device1_CheckFeatureSupport(pdev->dev, D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
      if (!FAILED(hres) && ms_info.NumQualityLevels > 0)
                  /* TODO: set correct value here */
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceFormatProperties2(VkPhysicalDevice physicalDevice,
               {
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetPhysicalDeviceImageFormatProperties2(VkPhysicalDevice physicalDevice,
               {
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice,
                                       {
      const VkPhysicalDeviceImageFormatInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = type,
   .tiling = tiling,
   .usage = usage,
                        VkResult result =
                     }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice,
                                             {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceSparseImageFormatProperties2(VkPhysicalDevice physicalDevice,
                     {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceExternalBufferProperties(VkPhysicalDevice physicalDevice,
               {
   #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
         #endif
         const VkExternalMemoryHandleTypeFlags d3d12_resource_handle_types =
         const VkExternalMemoryFeatureFlags import_export_feature_flags =
         const VkExternalMemoryFeatureFlags dedicated_feature_flags =
         switch (pExternalBufferInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
      pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes = d3d12_resource_handle_types;
   pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes = d3d12_resource_handle_types;
   pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = dedicated_feature_flags;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
      pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = import_export_feature_flags;
   #ifdef _WIN32
         #else
         #endif
         pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = import_export_feature_flags;
   #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT:
      if (pdev->dev13) {
      pExternalBufferProperties->externalMemoryProperties.compatibleHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.exportFromImportedHandleTypes =
      pExternalBufferProperties->externalMemoryProperties.externalMemoryFeatures = import_export_feature_flags;
      }
   #endif
      default:
      pExternalBufferProperties->externalMemoryProperties = (VkExternalMemoryProperties){ 0 };
         }
      VkResult
   dzn_instance_add_physical_device(struct vk_instance *instance,
               {
      struct dzn_instance *dzn_instance = container_of(instance, struct dzn_instance, vk);
   if ((dzn_instance->debug_flags & DZN_DEBUG_WARP) &&
      !desc->is_warp)
            }
      static VkResult
   dzn_enumerate_physical_devices(struct vk_instance *instance)
   {
         #ifdef _WIN32
      if (result != VK_SUCCESS)
      #endif
            }
      static const driOptionDescription dzn_dri_options[] = {
      DRI_CONF_SECTION_DEBUG
      DRI_CONF_DZN_CLAIM_WIDE_LINES(false)
   DRI_CONF_DZN_ENABLE_8BIT_LOADS_STORES(false)
         };
      static void
   dzn_init_dri_config(struct dzn_instance *instance)
   {
      driParseOptionInfo(&instance->available_dri_options, dzn_dri_options,
         driParseConfigFiles(&instance->dri_options, &instance->available_dri_options, 0, "dzn", NULL, NULL,
            }
      static VkResult
   dzn_instance_create(const VkInstanceCreateInfo *pCreateInfo,
               {
      struct dzn_instance *instance =
      vk_zalloc2(vk_default_allocator(), pAllocator, sizeof(*instance), 8,
      if (!instance)
            struct vk_instance_dispatch_table dispatch_table;
   vk_instance_dispatch_table_from_entrypoints(&dispatch_table,
               vk_instance_dispatch_table_from_entrypoints(&dispatch_table,
                  VkResult result =
      vk_instance_init(&instance->vk, &instance_extensions,
            if (result != VK_SUCCESS) {
      vk_free2(vk_default_allocator(), pAllocator, instance);
               instance->vk.physical_devices.enumerate = dzn_enumerate_physical_devices;
   instance->vk.physical_devices.destroy = dzn_physical_device_destroy;
   instance->debug_flags =
         #ifdef _WIN32
      if (instance->debug_flags & DZN_DEBUG_DEBUGGER) {
      /* wait for debugger to attach... */
   while (!IsDebuggerPresent()) {
                     if (instance->debug_flags & DZN_DEBUG_REDIRECTS) {
      char home[MAX_PATH], path[MAX_PATH];
   if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, home))) {
      snprintf(path, sizeof(path), "%s\\stderr.txt", home);
   freopen(path, "w", stderr);
   snprintf(path, sizeof(path), "%s\\stdout.txt", home);
            #endif
            #ifdef _WIN32
      instance->dxil_validator = dxil_create_validator(NULL);
      #endif
         if (missing_validator) {
      dzn_instance_destroy(instance, pAllocator);
               instance->d3d12_mod = util_dl_open(UTIL_DL_PREFIX "d3d12" UTIL_DL_EXT);
   if (!instance->d3d12_mod) {
      dzn_instance_destroy(instance, pAllocator);
               instance->d3d12.serialize_root_sig = d3d12_get_serialize_root_sig(instance->d3d12_mod);
   if (!instance->d3d12.serialize_root_sig) {
      dzn_instance_destroy(instance, pAllocator);
                        if (instance->debug_flags & DZN_DEBUG_D3D12)
         if (instance->debug_flags & DZN_DEBUG_GBV)
            instance->sync_binary_type = vk_sync_binary_get_type(&dzn_sync_type);
            *out = dzn_instance_to_handle(instance);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_EnumerateInstanceVersion(uint32_t *pApiVersion)
   {
      *pApiVersion = DZN_API_VERSION;
      }
         VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   dzn_GetInstanceProcAddr(VkInstance _instance,
         {
      VK_FROM_HANDLE(dzn_instance, instance, _instance);
   return vk_instance_get_proc_addr(&instance->vk,
            }
      /* Windows will use a dll definition file to avoid build errors. */
   #ifdef _WIN32
   #undef PUBLIC
   #define PUBLIC
   #endif
      /* With version 1+ of the loader interface the ICD should expose
   * vk_icdGetInstanceProcAddr to work around certain LD_PRELOAD issues seen in apps.
   */
   PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance,
            PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetInstanceProcAddr(VkInstance instance,
         {
         }
      /* With version 4+ of the loader interface the ICD should expose
   * vk_icdGetPhysicalDeviceProcAddr()
   */
   PUBLIC VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance  _instance,
            VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL
   vk_icdGetPhysicalDeviceProcAddr(VkInstance  _instance,
         {
      VK_FROM_HANDLE(dzn_instance, instance, _instance);
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
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceProperties2(VkPhysicalDevice physicalDevice,
         {
               /* minimum from the D3D and Vulkan specs */
            VkPhysicalDeviceLimits limits = {
      .maxImageDimension1D                      = D3D12_REQ_TEXTURE1D_U_DIMENSION,
   .maxImageDimension2D                      = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,
   .maxImageDimension3D                      = D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION,
   .maxImageDimensionCube                    = D3D12_REQ_TEXTURECUBE_DIMENSION,
            /* from here on, we simply use the minimum values from the spec for now */
   .maxTexelBufferElements                   = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP,
   .maxUniformBufferRange                    = D3D12_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * D3D12_STANDARD_VECTOR_SIZE * sizeof(float),
   .maxStorageBufferRange                    = 1 << D3D12_REQ_BUFFER_RESOURCE_TEXEL_COUNT_2_TO_EXP,
   .maxPushConstantsSize                     = 128,
   .maxMemoryAllocationCount                 = 4096,
   .maxSamplerAllocationCount                = 4000,
   .bufferImageGranularity                   = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .sparseAddressSpaceSize                   = 0,
   .maxBoundDescriptorSets                   = MAX_SETS,
   .maxPerStageDescriptorSamplers            =
      pdevice->options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ?
      .maxPerStageDescriptorUniformBuffers      =
      pdevice->options.ResourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2 ?
      .maxPerStageDescriptorStorageBuffers      =
      pdevice->options.ResourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2 ?
      .maxPerStageDescriptorSampledImages       =
      pdevice->options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ?
      .maxPerStageDescriptorStorageImages       =
      pdevice->options.ResourceBindingTier <= D3D12_RESOURCE_BINDING_TIER_2 ?
      .maxPerStageDescriptorInputAttachments    =
      pdevice->options.ResourceBindingTier == D3D12_RESOURCE_BINDING_TIER_1 ?
      .maxPerStageResources                     = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetSamplers                 = MAX_DESCS_PER_SAMPLER_HEAP,
   .maxDescriptorSetUniformBuffers           = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUniformBuffersDynamic    = MAX_DYNAMIC_UNIFORM_BUFFERS,
   .maxDescriptorSetStorageBuffers           = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetStorageBuffersDynamic    = MAX_DYNAMIC_STORAGE_BUFFERS,
   .maxDescriptorSetSampledImages            = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetStorageImages            = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetInputAttachments         = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxVertexInputAttributes                 = MIN2(D3D12_STANDARD_VERTEX_ELEMENT_COUNT, MAX_VERTEX_GENERIC_ATTRIBS),
   .maxVertexInputBindings                   = MAX_VBS,
   .maxVertexInputAttributeOffset            = D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES - 1,
   .maxVertexInputBindingStride              = D3D12_REQ_MULTI_ELEMENT_STRUCTURE_SIZE_IN_BYTES,
   .maxVertexOutputComponents                = D3D12_VS_OUTPUT_REGISTER_COUNT * D3D12_VS_OUTPUT_REGISTER_COMPONENTS,
   .maxTessellationGenerationLevel           = 0,
   .maxTessellationPatchSize                 = 0,
   .maxTessellationControlPerVertexInputComponents = 0,
   .maxTessellationControlPerVertexOutputComponents = 0,
   .maxTessellationControlPerPatchOutputComponents = 0,
   .maxTessellationControlTotalOutputComponents = 0,
   .maxTessellationEvaluationInputComponents = 0,
   .maxTessellationEvaluationOutputComponents = 0,
   .maxGeometryShaderInvocations             = D3D12_GS_MAX_INSTANCE_COUNT,
   .maxGeometryInputComponents               = D3D12_GS_INPUT_REGISTER_COUNT * D3D12_GS_INPUT_REGISTER_COMPONENTS,
   .maxGeometryOutputComponents              = D3D12_GS_OUTPUT_REGISTER_COUNT * D3D12_GS_OUTPUT_REGISTER_COMPONENTS,
   .maxGeometryOutputVertices                = D3D12_GS_MAX_OUTPUT_VERTEX_COUNT_ACROSS_INSTANCES,
   .maxGeometryTotalOutputComponents         = D3D12_REQ_GS_INVOCATION_32BIT_OUTPUT_COMPONENT_LIMIT,
   .maxFragmentInputComponents               = D3D12_PS_INPUT_REGISTER_COUNT * D3D12_PS_INPUT_REGISTER_COMPONENTS,
   .maxFragmentOutputAttachments             = D3D12_PS_OUTPUT_REGISTER_COUNT,
   .maxFragmentDualSrcAttachments            = 0,
   .maxFragmentCombinedOutputResources       = D3D12_PS_OUTPUT_REGISTER_COUNT,
   .maxComputeSharedMemorySize               = D3D12_CS_TGSM_REGISTER_COUNT * sizeof(float),
   .maxComputeWorkGroupCount                 = { D3D12_CS_DISPATCH_MAX_THREAD_GROUPS_PER_DIMENSION,
               .maxComputeWorkGroupInvocations           = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP,
   .maxComputeWorkGroupSize                  = { D3D12_CS_THREAD_GROUP_MAX_X, D3D12_CS_THREAD_GROUP_MAX_Y, D3D12_CS_THREAD_GROUP_MAX_Z },
   .subPixelPrecisionBits                    = D3D12_SUBPIXEL_FRACTIONAL_BIT_COUNT,
   .subTexelPrecisionBits                    = D3D12_SUBTEXEL_FRACTIONAL_BIT_COUNT,
   .mipmapPrecisionBits                      = D3D12_MIP_LOD_FRACTIONAL_BIT_COUNT,
   .maxDrawIndexedIndexValue                 = 0x00ffffff,
   .maxDrawIndirectCount                     = UINT32_MAX,
   .maxSamplerLodBias                        = D3D12_MIP_LOD_BIAS_MAX,
   .maxSamplerAnisotropy                     = D3D12_REQ_MAXANISOTROPY,
   .maxViewports                             = MAX_VP,
   .maxViewportDimensions                    = { D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION, D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION },
   .viewportBoundsRange                      = { D3D12_VIEWPORT_BOUNDS_MIN, D3D12_VIEWPORT_BOUNDS_MAX },
   .viewportSubPixelBits                     = 0,
   .minMemoryMapAlignment                    = 64,
   .minTexelBufferOffsetAlignment            = 32,
   .minUniformBufferOffsetAlignment          = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT,
   .minStorageBufferOffsetAlignment          = D3D12_RAW_UAV_SRV_BYTE_ALIGNMENT,
   .minTexelOffset                           = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_NEGATIVE,
   .maxTexelOffset                           = D3D12_COMMONSHADER_TEXEL_OFFSET_MAX_POSITIVE,
   .minTexelGatherOffset                     = -32,
   .maxTexelGatherOffset                     = 31,
   .minInterpolationOffset                   = -0.5f,
   .maxInterpolationOffset                   = 0.5f,
   .subPixelInterpolationOffsetBits          = 4,
   .maxFramebufferWidth                      = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,
   .maxFramebufferHeight                     = D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION,
   .maxFramebufferLayers                     = D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION,
   .framebufferColorSampleCounts             = supported_sample_counts,
   .framebufferDepthSampleCounts             = supported_sample_counts,
   .framebufferStencilSampleCounts           = supported_sample_counts,
   .framebufferNoAttachmentsSampleCounts     = supported_sample_counts,
   .maxColorAttachments                      = MAX_RTS,
   .sampledImageColorSampleCounts            = supported_sample_counts,
   .sampledImageIntegerSampleCounts          = VK_SAMPLE_COUNT_1_BIT,
   .sampledImageDepthSampleCounts            = supported_sample_counts,
   .sampledImageStencilSampleCounts          = supported_sample_counts,
   .storageImageSampleCounts                 = VK_SAMPLE_COUNT_1_BIT,
   .maxSampleMaskWords                       = 1,
   .timestampComputeAndGraphics              = true,
   .timestampPeriod                          = pdevice->timestamp_period,
   .maxClipDistances                         = D3D12_CLIP_OR_CULL_DISTANCE_COUNT,
   .maxCullDistances                         = D3D12_CLIP_OR_CULL_DISTANCE_COUNT,
   .maxCombinedClipAndCullDistances          = D3D12_CLIP_OR_CULL_DISTANCE_COUNT,
   .discreteQueuePriorities                  = 2,
   .pointSizeRange                           = { 1.0f, 1.0f },
   .lineWidthRange                           = { 1.0f, 1.0f },
   .pointSizeGranularity                     = 0.0f,
   .lineWidthGranularity                     = 0.0f,
   .strictLines                              = 0,
   .standardSampleLocations                  = true,
   .optimalBufferCopyOffsetAlignment         = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT,
   .optimalBufferCopyRowPitchAlignment       = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT,
               VkPhysicalDeviceType devtype = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
   if (pdevice->desc.is_warp)
         else if (!pdevice->architecture.UMA) {
                  pProperties->properties = (VkPhysicalDeviceProperties) {
      .apiVersion = DZN_API_VERSION,
            .vendorID = pdevice->desc.vendor_id,
   .deviceID = pdevice->desc.device_id,
            .limits = limits,
               snprintf(pProperties->properties.deviceName,
               memcpy(pProperties->properties.pipelineCacheUUID,
            VkPhysicalDeviceVulkan11Properties core_1_1 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
   .deviceLUIDValid                       = true,
   .pointClippingBehavior                 = VK_POINT_CLIPPING_BEHAVIOR_ALL_CLIP_PLANES,
   .maxMultiviewViewCount                 = 6,
   .maxMultiviewInstanceIndex             = UINT_MAX,
   .protectedNoFault                      = false,
   /* Vulkan 1.1 wants this value to be at least 1024. Let's stick to this
   * minimum requirement for now, and hope the total number of samplers
   * across all descriptor sets doesn't exceed 2048, otherwise we'd exceed
   * the maximum number of samplers per heap. For any descriptor set
   * containing more than 1024 descriptors,
   * vkGetDescriptorSetLayoutSupport() can be called to determine if the
   * layout is within D3D12 descriptor heap bounds.
   */
   .maxPerSetDescriptors                  = 1024,
   /* According to the spec, the maximum D3D12 resource size is
   * min(max(128MB, 0.25f * (amount of dedicated VRAM)), 2GB),
   * but the limit actually depends on the max(system_ram, VRAM) not
   * just the VRAM.
   */
   .maxMemoryAllocationSize               =
      CLAMP(MAX2(pdevice->desc.dedicated_video_memory,
            pdevice->desc.dedicated_system_memory +
   .subgroupSupportedOperations = VK_SUBGROUP_FEATURE_BASIC_BIT |
                                 VK_SUBGROUP_FEATURE_BALLOT_BIT |
   .subgroupSupportedStages = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT |
         .subgroupQuadOperationsInAllStages = true,
      };
   memcpy(core_1_1.driverUUID, pdevice->driver_uuid, VK_UUID_SIZE);
   memcpy(core_1_1.deviceUUID, pdevice->device_uuid, VK_UUID_SIZE);
                     VkPhysicalDeviceVulkan12Properties core_1_2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
   .driverID = VK_DRIVER_ID_MESA_DOZEN,
   .conformanceVersion = (VkConformanceVersion){
      .major = 0,
   .minor = 0,
   .subminor = 0,
      },
   .denormBehaviorIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .roundingModeIndependence = VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
   .shaderSignedZeroInfNanPreserveFloat16 = false,
   .shaderSignedZeroInfNanPreserveFloat32 = false,
   .shaderSignedZeroInfNanPreserveFloat64 = false,
   .shaderDenormPreserveFloat16 = true,
   .shaderDenormPreserveFloat32 = pdevice->shader_model >= D3D_SHADER_MODEL_6_2,
   .shaderDenormPreserveFloat64 = true,
   .shaderDenormFlushToZeroFloat16 = false,
   .shaderDenormFlushToZeroFloat32 = true,
   .shaderDenormFlushToZeroFloat64 = false,
   .shaderRoundingModeRTEFloat16 = true,
   .shaderRoundingModeRTEFloat32 = true,
   .shaderRoundingModeRTEFloat64 = true,
   .shaderRoundingModeRTZFloat16 = false,
   .shaderRoundingModeRTZFloat32 = false,
   .shaderRoundingModeRTZFloat64 = false,
   .shaderUniformBufferArrayNonUniformIndexingNative = true,
   .shaderSampledImageArrayNonUniformIndexingNative = true,
   .shaderStorageBufferArrayNonUniformIndexingNative = true,
   .shaderStorageImageArrayNonUniformIndexingNative = true,
   .shaderInputAttachmentArrayNonUniformIndexingNative = true,
   .robustBufferAccessUpdateAfterBind = true,
   .quadDivergentImplicitLod = false,
   .maxUpdateAfterBindDescriptorsInAllPools = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindSamplers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindUniformBuffers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindStorageBuffers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindSampledImages = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindStorageImages = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageDescriptorUpdateAfterBindInputAttachments = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxPerStageUpdateAfterBindResources = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUpdateAfterBindSamplers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUpdateAfterBindUniformBuffers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUpdateAfterBindUniformBuffersDynamic = MAX_DYNAMIC_UNIFORM_BUFFERS,
   .maxDescriptorSetUpdateAfterBindStorageBuffers = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUpdateAfterBindStorageBuffersDynamic = MAX_DYNAMIC_STORAGE_BUFFERS,
   .maxDescriptorSetUpdateAfterBindSampledImages = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
   .maxDescriptorSetUpdateAfterBindStorageImages = MAX_DESCS_PER_CBV_SRV_UAV_HEAP,
            .supportedDepthResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT | VK_RESOLVE_MODE_AVERAGE_BIT |
         .supportedStencilResolveModes = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT | VK_RESOLVE_MODE_MIN_BIT | VK_RESOLVE_MODE_MAX_BIT,
   .independentResolveNone = true,
   .independentResolve = true,
   .filterMinmaxSingleComponentFormats = false,
   .filterMinmaxImageComponentMapping = false,
   .maxTimelineSemaphoreValueDifference = UINT64_MAX,
               snprintf(core_1_2.driverName, VK_MAX_DRIVER_NAME_SIZE, "Dozen");
            const VkPhysicalDeviceVulkan13Properties core_1_3 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
   .minSubgroupSize = pdevice->options1.WaveOps ? pdevice->options1.WaveLaneCountMin : 1,
   .maxSubgroupSize = pdevice->options1.WaveOps ? pdevice->options1.WaveLaneCountMax : 1,
   .maxComputeWorkgroupSubgroups = D3D12_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP /
         .requiredSubgroupSizeStages = VK_SHADER_STAGE_COMPUTE_BIT,
   .integerDotProduct4x8BitPackedSignedAccelerated = pdevice->shader_model >= D3D_SHADER_MODEL_6_4,
   .integerDotProduct4x8BitPackedUnsignedAccelerated = pdevice->shader_model >= D3D_SHADER_MODEL_6_4,
   .integerDotProductAccumulatingSaturating4x8BitPackedSignedAccelerated = pdevice->shader_model >= D3D_SHADER_MODEL_6_4,
               vk_foreach_struct(ext, pProperties->pNext) {
      if (vk_get_physical_device_core_1_1_property_ext(ext, &core_1_1) ||
      vk_get_physical_device_core_1_2_property_ext(ext, &core_1_2) ||
               switch (ext->sType) {
   case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_PROPERTIES_EXT: {
      VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT *attr_div =
         attr_div->maxVertexAttribDivisor = UINT32_MAX;
      #ifdef _WIN32
         case VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_MEMORY_HOST_PROPERTIES_EXT: {
      VkPhysicalDeviceExternalMemoryHostPropertiesEXT *host_props =
         host_props->minImportedHostPointerAlignment = 65536;
      #endif
         default:
      dzn_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceQueueFamilyProperties2(VkPhysicalDevice physicalDevice,
               {
      VK_FROM_HANDLE(dzn_physical_device, pdev, physicalDevice);
   VK_OUTARRAY_MAKE_TYPED(VkQueueFamilyProperties2, out,
            for (uint32_t i = 0; i < pdev->queue_family_count; i++) {
      vk_outarray_append_typed(VkQueueFamilyProperties2, &out, p) {
               vk_foreach_struct(ext, pQueueFamilyProperties->pNext) {
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
         {
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetPhysicalDeviceMemoryProperties2(VkPhysicalDevice physicalDevice,
         {
      dzn_GetPhysicalDeviceMemoryProperties(physicalDevice,
            vk_foreach_struct(ext, pMemoryProperties->pNext) {
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_EnumerateInstanceLayerProperties(uint32_t *pPropertyCount,
         {
      if (pProperties == NULL) {
      *pPropertyCount = 0;
                  }
      static VkResult
   dzn_queue_sync_wait(struct dzn_queue *queue, const struct vk_sync_wait *wait)
   {
      if (wait->sync->type == &vk_sync_dummy_type)
            struct dzn_device *device = container_of(queue->vk.base.device, struct dzn_device, vk);
   assert(wait->sync->type == &dzn_sync_type);
   struct dzn_sync *sync = container_of(wait->sync, struct dzn_sync, vk);
   uint64_t value =
                     if (value > 0 && FAILED(ID3D12CommandQueue_Wait(queue->cmdqueue, sync->fence, value)))
               }
      static VkResult
   dzn_queue_sync_signal(struct dzn_queue *queue, const struct vk_sync_signal *signal)
   {
      if (signal->sync->type == &vk_sync_dummy_type)
            struct dzn_device *device = container_of(queue->vk.base.device, struct dzn_device, vk);
   assert(signal->sync->type == &dzn_sync_type);
   struct dzn_sync *sync = container_of(signal->sync, struct dzn_sync, vk);
   uint64_t value =
                           if (FAILED(ID3D12CommandQueue_Signal(queue->cmdqueue, sync->fence, value)))
               }
      static VkResult
   dzn_queue_submit(struct vk_queue *q,
         {
      struct dzn_queue *queue = container_of(q, struct dzn_queue, vk);
   struct dzn_device *device = container_of(q->base.device, struct dzn_device, vk);
            for (uint32_t i = 0; i < info->wait_count; i++) {
      result = dzn_queue_sync_wait(queue, &info->waits[i]);
   if (result != VK_SUCCESS)
                        for (uint32_t i = 0; i < info->command_buffer_count; i++) {
      struct dzn_cmd_buffer *cmd_buffer =
                     util_dynarray_foreach(&cmd_buffer->queries.reset, struct dzn_cmd_buffer_query_range, range) {
      mtx_lock(&range->qpool->queries_lock);
   for (uint32_t q = range->start; q < range->start + range->count; q++) {
      struct dzn_query *query = &range->qpool->queries[q];
   if (query->fence) {
      ID3D12Fence_Release(query->fence);
      }
      }
                           for (uint32_t i = 0; i < info->command_buffer_count; i++) {
      struct dzn_cmd_buffer* cmd_buffer =
            util_dynarray_foreach(&cmd_buffer->events.signal, struct dzn_cmd_event_signal, evt) {
      if (FAILED(ID3D12CommandQueue_Signal(queue->cmdqueue, evt->event->fence, evt->value ? 1 : 0)))
               util_dynarray_foreach(&cmd_buffer->queries.signal, struct dzn_cmd_buffer_query_range, range) {
      mtx_lock(&range->qpool->queries_lock);
   for (uint32_t q = range->start; q < range->start + range->count; q++) {
      struct dzn_query *query = &range->qpool->queries[q];
   query->fence_value = queue->fence_point + 1;
   query->fence = queue->fence;
      }
                  for (uint32_t i = 0; i < info->signal_count; i++) {
      result = dzn_queue_sync_signal(queue, &info->signals[i]);
   if (result != VK_SUCCESS)
               if (FAILED(ID3D12CommandQueue_Signal(queue->cmdqueue, queue->fence, ++queue->fence_point)))
               }
      static void
   dzn_queue_finish(struct dzn_queue *queue)
   {
      if (queue->cmdqueue)
            if (queue->fence)
               }
      static VkResult
   dzn_queue_init(struct dzn_queue *queue,
                     {
               VkResult result = vk_queue_init(&queue->vk, &device->vk, pCreateInfo, index_in_family);
   if (result != VK_SUCCESS)
                              D3D12_COMMAND_QUEUE_DESC queue_desc =
            float priority_in = pCreateInfo->pQueuePriorities[index_in_family];
   queue_desc.Priority =
                  if (FAILED(ID3D12Device1_CreateCommandQueue(device->dev, &queue_desc,
                  dzn_queue_finish(queue);
               if (FAILED(ID3D12Device1_CreateFence(device->dev, 0, D3D12_FENCE_FLAG_NONE,
                  dzn_queue_finish(queue);
                  }
      static VkResult
   dzn_device_create_sync_for_memory(struct vk_device *device,
                     {
      return vk_sync_create(device, &vk_sync_dummy_type,
      }
      static VkResult
   dzn_device_query_init(struct dzn_device *device)
   {
      /* FIXME: create the resource in the default heap */
   D3D12_HEAP_PROPERTIES hprops = dzn_ID3D12Device4_GetCustomHeapProperties(device->dev, 0, D3D12_HEAP_TYPE_UPLOAD);
   D3D12_RESOURCE_DESC rdesc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
   .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .Width = DZN_QUERY_REFS_RES_SIZE,
   .Height = 1,
   .DepthOrArraySize = 1,
   .MipLevels = 1,
   .Format = DXGI_FORMAT_UNKNOWN,
   .SampleDesc = { .Count = 1, .Quality = 0 },
   .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
               if (FAILED(ID3D12Device1_CreateCommittedResource(device->dev, &hprops,
                                                uint8_t *queries_ref;
   if (FAILED(ID3D12Resource_Map(device->queries.refs, 0, NULL, (void **)&queries_ref)))
            memset(queries_ref + DZN_QUERY_REFS_ALL_ONES_OFFSET, 0xff, DZN_QUERY_REFS_SECTION_SIZE);
   memset(queries_ref + DZN_QUERY_REFS_ALL_ZEROS_OFFSET, 0x0, DZN_QUERY_REFS_SECTION_SIZE);
               }
      static void
   dzn_device_query_finish(struct dzn_device *device)
   {
      if (device->queries.refs)
      }
      static void
   dzn_device_destroy(struct dzn_device *device, const VkAllocationCallbacks *pAllocator)
   {
      if (!device)
            struct dzn_instance *instance =
            vk_foreach_queue_safe(q, &device->vk) {
                           dzn_device_query_finish(device);
            dzn_foreach_pool_type(type) {
      dzn_descriptor_heap_finish(&device->device_heaps[type].heap);
   util_dynarray_fini(&device->device_heaps[type].slot_freelist);
               if (device->dev_config)
            if (device->dev)
            if (device->dev10)
            if (device->dev11)
            if (device->dev12)
         #if D3D12_SDK_VERSION >= 611
      if (device->dev13)
      #endif
         vk_device_finish(&device->vk);
      }
      static VkResult
   dzn_device_check_status(struct vk_device *dev)
   {
               if (FAILED(ID3D12Device_GetDeviceRemovedReason(device->dev)))
               }
      static VkResult
   dzn_device_create(struct dzn_physical_device *pdev,
                     {
               uint32_t graphics_queue_count = 0;
   uint32_t queue_count = 0;
   for (uint32_t qf = 0; qf < pCreateInfo->queueCreateInfoCount; qf++) {
      const VkDeviceQueueCreateInfo *qinfo = &pCreateInfo->pQueueCreateInfos[qf];
   queue_count += qinfo->queueCount;
   if (pdev->queue_families[qinfo->queueFamilyIndex].props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
               /* Add a swapchain queue if there's no or too many graphics queues */
   if (graphics_queue_count != 1)
            VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_device, device, 1);
            if (!vk_multialloc_zalloc2(&ma, &instance->vk.alloc, pAllocator,
                           /* For secondary command buffer support, overwrite any command entrypoints
   * in the main device-level dispatch table with
   * vk_cmd_enqueue_unless_primary_Cmd*.
   */
   vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
         vk_device_dispatch_table_from_entrypoints(&dispatch_table,
            /* Populate our primary cmd_dispatch table. */
   vk_device_dispatch_table_from_entrypoints(&device->cmd_dispatch,
         vk_device_dispatch_table_from_entrypoints(&device->cmd_dispatch,
                  /* Override entrypoints with alternatives based on supported features. */
   if (pdev->options12.EnhancedBarriersSupported) {
                  VkResult result =
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, device);
               /* Must be done after vk_device_init() because this function memset(0) the
   * whole struct.
   */
   device->vk.command_dispatch_table = &device->cmd_dispatch;
   device->vk.create_sync_for_memory = dzn_device_create_sync_for_memory;
                              if (pdev->dev10) {
      device->dev10 = pdev->dev10;
      }
   if (pdev->dev11) {
      device->dev11 = pdev->dev11;
               if (pdev->dev12) {
      device->dev12 = pdev->dev12;
            #if D3D12_SDK_VERSION >= 611
      if (pdev->dev13) {
      device->dev13 = pdev->dev13;
         #endif
         ID3D12InfoQueue *info_queue;
   if (SUCCEEDED(ID3D12Device1_QueryInterface(device->dev,
                  D3D12_MESSAGE_SEVERITY severities[] = {
      D3D12_MESSAGE_SEVERITY_INFO,
               D3D12_MESSAGE_ID msg_ids[] = {
                  D3D12_INFO_QUEUE_FILTER NewFilter = { 0 };
   NewFilter.DenyList.NumSeverities = ARRAY_SIZE(severities);
   NewFilter.DenyList.pSeverityList = severities;
   NewFilter.DenyList.NumIDs = ARRAY_SIZE(msg_ids);
            ID3D12InfoQueue_PushStorageFilter(info_queue, &NewFilter);
                        result = dzn_meta_init(device);
   if (result != VK_SUCCESS) {
      dzn_device_destroy(device, pAllocator);
               result = dzn_device_query_init(device);
   if (result != VK_SUCCESS) {
      dzn_device_destroy(device, pAllocator);
               uint32_t qindex = 0;
   for (uint32_t qf = 0; qf < pCreateInfo->queueCreateInfoCount; qf++) {
               for (uint32_t q = 0; q < qinfo->queueCount; q++) {
      result =
         if (result != VK_SUCCESS) {
      dzn_device_destroy(device, pAllocator);
      }
   if (graphics_queue_count == 1 &&
      pdev->queue_families[qinfo->queueFamilyIndex].props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
               if (!device->swapchain_queue) {
      const float swapchain_queue_priority = 0.0f;
   VkDeviceQueueCreateInfo swapchain_queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
   .flags = 0,
   .queueCount = 1,
      };
   for (uint32_t qf = 0; qf < pdev->queue_family_count; qf++) {
      if (pdev->queue_families[qf].props.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      swapchain_queue_info.queueFamilyIndex = qf;
         }
   result = dzn_queue_init(&queues[qindex], device, &swapchain_queue_info, 0);
   if (result != VK_SUCCESS) {
      dzn_device_destroy(device, pAllocator);
      }
   device->swapchain_queue = &queues[qindex++];
               device->support_static_samplers = true;
   device->bindless = (instance->debug_flags & DZN_DEBUG_BINDLESS) != 0 ||
      device->vk.enabled_features.descriptorIndexing ||
         if (device->bindless) {
      uint32_t sampler_count = MIN2(pdev->options19.MaxSamplerDescriptorHeapSize, 4000);
   device->support_static_samplers = pdev->options19.MaxSamplerDescriptorHeapSizeWithStaticSamplers >= sampler_count;
   dzn_foreach_pool_type(type) {
      uint32_t descriptor_count = type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ?
         result = dzn_descriptor_heap_init(&device->device_heaps[type].heap, device, type, descriptor_count, true);
   if (result != VK_SUCCESS) {
      dzn_device_destroy(device, pAllocator);
               mtx_init(&device->device_heaps[type].lock, mtx_plain);
   util_dynarray_init(&device->device_heaps[type].slot_freelist, NULL);
                  assert(queue_count == qindex);
   *out = dzn_device_to_handle(device);
      }
      static ID3DBlob *
   serialize_root_sig(struct dzn_device *device,
         {
      struct dzn_instance *instance =
                  HRESULT hr = device->dev_config ?
                  if (FAILED(hr)) {
      if (instance->debug_flags & DZN_DEBUG_SIG) {
      const char *error_msg = (const char *)ID3D10Blob_GetBufferPointer(error);
   fprintf(stderr,
         "== SERIALIZE ROOT SIG ERROR =============================================\n"
   "%s\n"
                  if (error)
               }
      ID3D12RootSignature *
   dzn_device_create_root_sig(struct dzn_device *device,
         {
      ID3DBlob *sig = serialize_root_sig(device, desc);
   if (!sig)
            ID3D12RootSignature *root_sig = NULL;
   ID3D12Device1_CreateRootSignature(device->dev, 0,
                           ID3D10Blob_Release(sig);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateDevice(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(dzn_physical_device, physical_device, physicalDevice);
                     /* Check enabled features */
   if (pCreateInfo->pEnabledFeatures) {
      result = vk_physical_device_check_device_features(&physical_device->vk, pCreateInfo);
   if (result != VK_SUCCESS)
               /* Check requested queues and fail if we are requested to create any
   * queues with flags we don't support.
   */
   assert(pCreateInfo->queueCreateInfoCount > 0);
   for (uint32_t i = 0; i < pCreateInfo->queueCreateInfoCount; i++) {
      if (pCreateInfo->pQueueCreateInfos[i].flags != 0)
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyDevice(VkDevice dev,
         {
                           }
      static void
   dzn_device_memory_destroy(struct dzn_device_memory *mem,
         {
      if (!mem)
                     if (mem->map && mem->map_res)
            if (mem->map_res)
            if (mem->heap)
            if (mem->dedicated_res)
         #ifdef _WIN32
      if (mem->export_handle)
      #else
      if ((intptr_t)mem->export_handle >= 0)
      #endif
         vk_object_base_finish(&mem->base);
      }
      static D3D12_HEAP_PROPERTIES
   deduce_heap_properties_from_memory(struct dzn_physical_device *pdevice,
         {
      D3D12_HEAP_PROPERTIES properties = { .Type = D3D12_HEAP_TYPE_CUSTOM };
   properties.MemoryPoolPreference =
      ((mem_type->propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
   !pdevice->architecture.UMA) ?
      if ((mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) ||
      ((mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && pdevice->architecture.CacheCoherentUMA)) {
      } else if (mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
         } else {
         }
      }
      static VkResult
   dzn_device_memory_create(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdevice =
            const struct dzn_buffer *buffer = NULL;
            VkExternalMemoryHandleTypeFlags export_flags = 0;
   HANDLE import_handle = NULL;
   bool imported_from_d3d11 = false;
      #ifdef _WIN32
      const wchar_t *import_name = NULL;
      #endif
      vk_foreach_struct_const(ext, pAllocateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO: {
                     export_flags = exp->handleTypes;
      #ifdef _WIN32
         case VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR: {
      const VkImportMemoryWin32HandleInfoKHR *imp =
         switch (imp->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT:
      imported_from_d3d11 = true;
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
         default:
         }
   import_handle = imp->handle;
   import_name = imp->name;
      }
   case VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR:
      win32_export = (const VkExportMemoryWin32HandleInfoKHR *)ext;
      case VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT: {
      const VkImportMemoryHostPointerInfoEXT *imp =
         host_pointer = imp->pHostPointer;
      #else
         case VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR: {
      const VkImportMemoryFdInfoKHR *imp =
         switch (imp->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
         default:
         }
   import_handle = (HANDLE)(intptr_t)imp->fd;
      #endif
         case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO: {
                     buffer = dzn_buffer_from_handle(dedicated->buffer);
   image = dzn_image_from_handle(dedicated->image);
   assert(!buffer || !image);
      }
   default:
      dzn_debug_ignored_stype(ext->sType);
                  const VkMemoryType *mem_type =
                     heap_desc.SizeInBytes = pAllocateInfo->allocationSize;
   if (buffer) {
         } else if (image) {
      heap_desc.Alignment =
      image->vk.samples > 1 ?
   D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT :
   } else {
      heap_desc.Alignment =
      heap_desc.SizeInBytes >= D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT ?
   D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT :
            if (mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            VkExternalMemoryHandleTypeFlags valid_flags =
      opaque_external_flag |
   (buffer || image ?
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT :
      if (image && imported_from_d3d11)
            if (export_flags & ~valid_flags)
            struct dzn_device_memory *mem =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*mem), 8,
      if (!mem)
               #ifndef _WIN32
         #endif
         /* The Vulkan 1.0.33 spec says "allocationSize must be greater than 0". */
                     heap_desc.SizeInBytes = ALIGN_POT(heap_desc.SizeInBytes, heap_desc.Alignment);
   if (!image && !buffer)
      heap_desc.Flags =
      heap_desc.Properties = deduce_heap_properties_from_memory(pdevice, mem_type);
   if (export_flags) {
      heap_desc.Flags |= D3D12_HEAP_FLAG_SHARED;
                     #ifdef _WIN32
      HANDLE handle_from_name = NULL;
   if (import_name) {
      if (FAILED(ID3D12Device_OpenSharedHandleByName(device->dev, import_name, GENERIC_ALL, &handle_from_name))) {
      error = VK_ERROR_INVALID_EXTERNAL_HANDLE;
      }
         #endif
         if (host_pointer) {
         #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
         if (!device->dev13)
            if (FAILED(ID3D12Device13_OpenExistingHeapFromAddress1(device->dev13, host_pointer, heap_desc.SizeInBytes, &IID_ID3D12Heap, (void**)&mem->heap)))
            D3D12_HEAP_DESC desc = dzn_ID3D12Heap_GetDesc(mem->heap);
   if (desc.Properties.Type != D3D12_HEAP_TYPE_CUSTOM)
            if ((heap_desc.Flags & ~desc.Flags) ||
      desc.Properties.CPUPageProperty != heap_desc.Properties.CPUPageProperty ||
               mem->map = host_pointer;
   #else
         #endif
      } else if (import_handle) {
      error = VK_ERROR_INVALID_EXTERNAL_HANDLE;
   if (image || buffer) {
                     /* Verify compatibility */
   D3D12_RESOURCE_DESC desc = dzn_ID3D12Resource_GetDesc(mem->dedicated_res);
   D3D12_HEAP_PROPERTIES opened_props = { 0 };
   D3D12_HEAP_FLAGS opened_flags = 0;
   ID3D12Resource_GetHeapProperties(mem->dedicated_res, &opened_props, &opened_flags);
                  /* Don't validate format, cast lists aren't reflectable so it could be valid */
   if (image) {
      if (desc.Dimension != image->desc.Dimension ||
      desc.MipLevels != image->desc.MipLevels ||
   desc.Width != image->desc.Width ||
   desc.Height != image->desc.Height ||
   desc.DepthOrArraySize != image->desc.DepthOrArraySize ||
   (image->desc.Flags & ~desc.Flags) ||
   desc.SampleDesc.Count != image->desc.SampleDesc.Count)
   } else if (desc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
            desc.Width != buffer->desc.Width ||
      if (opened_props.CPUPageProperty != heap_desc.Properties.CPUPageProperty ||
      opened_props.MemoryPoolPreference != heap_desc.Properties.MemoryPoolPreference)
      if ((heap_desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) && desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
         if ((heap_desc.Flags & D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES) && (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))
         else if ((heap_desc.Flags & D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES) && !(desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET))
      } else {
                     D3D12_HEAP_DESC desc = dzn_ID3D12Heap_GetDesc(mem->heap);
                  if (desc.Alignment < heap_desc.Alignment ||
      desc.SizeInBytes < heap_desc.SizeInBytes ||
   (heap_desc.Flags & ~desc.Flags) ||
   desc.Properties.CPUPageProperty != heap_desc.Properties.CPUPageProperty ||
   desc.Properties.MemoryPoolPreference != heap_desc.Properties.MemoryPoolPreference)
      } else if (image) {
      if (device->dev10 && image->castable_format_count > 0) {
      D3D12_RESOURCE_DESC1 desc = {
      .Dimension = image->desc.Dimension,
   .Alignment = image->desc.Alignment,
   .Width = image->desc.Width,
   .Height = image->desc.Height,
   .DepthOrArraySize = image->desc.DepthOrArraySize,
   .MipLevels = image->desc.MipLevels,
   .Format = image->desc.Format,
   .SampleDesc = image->desc.SampleDesc,
   .Layout = image->desc.Layout,
      };
   if (FAILED(ID3D12Device10_CreateCommittedResource3(device->dev10, &heap_desc.Properties,
                                                } else if (FAILED(ID3D12Device1_CreateCommittedResource(device->dev, &heap_desc.Properties,
                                    } else if (buffer) {
      if (FAILED(ID3D12Device1_CreateCommittedResource(device->dev, &heap_desc.Properties,
                                    } else {
      if (FAILED(ID3D12Device1_CreateHeap(device->dev, &heap_desc,
                           if ((mem_type->propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) &&
      !(heap_desc.Flags & D3D12_HEAP_FLAG_DENY_BUFFERS) &&
   !mem->map){
   assert(!image);
   if (buffer) {
      mem->map_res = mem->dedicated_res;
      } else {
      D3D12_RESOURCE_DESC res_desc = { 0 };
   res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   res_desc.Format = DXGI_FORMAT_UNKNOWN;
   res_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
   res_desc.Width = heap_desc.SizeInBytes;
   res_desc.Height = 1;
   res_desc.DepthOrArraySize = 1;
   res_desc.MipLevels = 1;
   res_desc.SampleDesc.Count = 1;
   res_desc.SampleDesc.Quality = 0;
   res_desc.Flags = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
   res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   HRESULT hr = ID3D12Device1_CreatePlacedResource(device->dev, mem->heap, 0, &res_desc,
                           if (FAILED(hr))
                  if (export_flags) {
      error = VK_ERROR_INVALID_EXTERNAL_HANDLE;
   ID3D12DeviceChild *shareable = mem->heap ? (void *)mem->heap : (void *)mem->dedicated_res;
   #ifdef _WIN32
         const SECURITY_ATTRIBUTES *pAttributes = win32_export ? win32_export->pAttributes : NULL;
   #else
         const SECURITY_ATTRIBUTES *pAttributes = NULL;
   #endif
         if (FAILED(ID3D12Device_CreateSharedHandle(device->dev, shareable, pAttributes,
                     *out = dzn_device_memory_to_handle(mem);
         cleanup:
   #ifdef _WIN32
      if (handle_from_name)
      #endif
      dzn_device_memory_destroy(mem, pAllocator);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_AllocateMemory(VkDevice device,
                     {
      return dzn_device_memory_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_FreeMemory(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_MapMemory(VkDevice _device,
               VkDeviceMemory _memory,
   VkDeviceSize offset,
   VkDeviceSize size,
   {
      VK_FROM_HANDLE(dzn_device, device, _device);
            if (mem == NULL) {
      *ppData = NULL;
               if (mem->map && !mem->map_res) {
      *ppData = ((uint8_t *)mem->map) + offset;
               if (size == VK_WHOLE_SIZE)
            /* From the Vulkan spec version 1.0.32 docs for MapMemory:
   *
   *  * If size is not equal to VK_WHOLE_SIZE, size must be greater than 0
   *    assert(size != 0);
   *  * If size is not equal to VK_WHOLE_SIZE, size must be less than or
   *    equal to the size of the memory minus offset
   */
   assert(size > 0);
            assert(mem->map_res);
   D3D12_RANGE range = { 0 };
   range.Begin = offset;
   range.End = offset + size;
   void *map = NULL;
   if (FAILED(ID3D12Resource_Map(mem->map_res, 0, &range, &map)))
            mem->map = map;
                        }
      VKAPI_ATTR void VKAPI_CALL
   dzn_UnmapMemory(VkDevice _device,
         {
               if (mem == NULL)
            if (!mem->map_res)
                     mem->map = NULL;
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_FlushMappedMemoryRanges(VkDevice _device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_InvalidateMappedMemoryRanges(VkDevice _device,
               {
         }
      static void
   dzn_buffer_destroy(struct dzn_buffer *buf, const VkAllocationCallbacks *pAllocator)
   {
      if (!buf)
                     if (buf->res)
            dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, buf->cbv_bindless_slot);
   dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, buf->uav_bindless_slot);
   if (buf->custom_views) {
      hash_table_foreach(buf->custom_views, entry) {
      free((void *)entry->key);
      }
               vk_object_base_finish(&buf->base);
      }
      static VkResult
   dzn_buffer_create(struct dzn_device *device,
                     {
      struct dzn_buffer *buf =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*buf), 8,
      if (!buf)
            vk_object_base_init(&device->vk, &buf->base, VK_OBJECT_TYPE_BUFFER);
   buf->create_flags = pCreateInfo->flags;
   buf->size = pCreateInfo->size;
            if (buf->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
         if (buf->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            buf->desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   buf->desc.Format = DXGI_FORMAT_UNKNOWN;
   buf->desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
   buf->desc.Width = buf->size;
   buf->desc.Height = 1;
   buf->desc.DepthOrArraySize = 1;
   buf->desc.MipLevels = 1;
   buf->desc.SampleDesc.Count = 1;
   buf->desc.SampleDesc.Quality = 0;
   buf->desc.Flags = D3D12_RESOURCE_FLAG_NONE;
   buf->desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   buf->valid_access =
      D3D12_BARRIER_ACCESS_VERTEX_BUFFER |
   D3D12_BARRIER_ACCESS_CONSTANT_BUFFER |
   D3D12_BARRIER_ACCESS_INDEX_BUFFER |
   D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
   D3D12_BARRIER_ACCESS_STREAM_OUTPUT |
   D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT |
   D3D12_BARRIER_ACCESS_PREDICATION |
   D3D12_BARRIER_ACCESS_COPY_DEST |
   D3D12_BARRIER_ACCESS_COPY_SOURCE |
   D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ |
         if (buf->usage &
      (VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
   VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) {
   buf->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
               buf->cbv_bindless_slot = buf->uav_bindless_slot = -1;
   if (device->bindless) {
      if (buf->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
      buf->cbv_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (buf->cbv_bindless_slot < 0) {
      dzn_buffer_destroy(buf, pAllocator);
         }
   if (buf->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
      buf->uav_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (buf->uav_bindless_slot < 0) {
      dzn_buffer_destroy(buf, pAllocator);
                     if (device->bindless)
            const VkExternalMemoryBufferCreateInfo *external_info =
         if (external_info && external_info->handleTypes != 0)
            *out = dzn_buffer_to_handle(buf);
      }
      DXGI_FORMAT
   dzn_buffer_get_dxgi_format(VkFormat format)
   {
                  }
      D3D12_TEXTURE_COPY_LOCATION
   dzn_buffer_get_copy_loc(const struct dzn_buffer *buf,
                           {
      struct dzn_physical_device *pdev =
         const uint32_t buffer_row_length =
                     enum pipe_format pfmt = vk_format_to_pipe_format(plane_format);
   uint32_t blksz = util_format_get_blocksize(pfmt);
   uint32_t blkw = util_format_get_blockwidth(pfmt);
            D3D12_TEXTURE_COPY_LOCATION loc = {
   .pResource = buf->res,
   .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
   .PlacedFootprint = {
      .Footprint = {
      .Format =
         .Width = region->imageExtent.width,
   .Height = region->imageExtent.height,
   .Depth = region->imageExtent.depth,
         },
            uint32_t buffer_layer_stride =
      loc.PlacedFootprint.Footprint.RowPitch *
         loc.PlacedFootprint.Offset =
               }
      D3D12_TEXTURE_COPY_LOCATION
   dzn_buffer_get_line_copy_loc(const struct dzn_buffer *buf, VkFormat format,
                     {
      uint32_t buffer_row_length =
         uint32_t buffer_image_height =
                     enum pipe_format pfmt = vk_format_to_pipe_format(format);
   uint32_t blksz = util_format_get_blocksize(pfmt);
   uint32_t blkw = util_format_get_blockwidth(pfmt);
   uint32_t blkh = util_format_get_blockheight(pfmt);
   uint32_t blkd = util_format_get_blockdepth(pfmt);
   D3D12_TEXTURE_COPY_LOCATION new_loc = *loc;
   uint32_t buffer_row_stride =
         uint32_t buffer_layer_stride =
      buffer_row_stride *
         uint64_t tex_offset =
      ((y / blkh) * buffer_row_stride) +
      uint64_t offset = loc->PlacedFootprint.Offset + tex_offset;
            while (offset_alignment % blksz)
            new_loc.PlacedFootprint.Footprint.Height = blkh;
   new_loc.PlacedFootprint.Footprint.Depth = 1;
   new_loc.PlacedFootprint.Offset = (offset / offset_alignment) * offset_alignment;
   *start_x = ((offset % offset_alignment) / blksz) * blkw;
   new_loc.PlacedFootprint.Footprint.Width = *start_x + region->imageExtent.width;
   new_loc.PlacedFootprint.Footprint.RowPitch =
      ALIGN_POT(DIV_ROUND_UP(new_loc.PlacedFootprint.Footprint.Width, blkw) * blksz,
         }
      bool
   dzn_buffer_supports_region_copy(struct dzn_physical_device *pdev,
         {
      if (pdev->options13.UnrestrictedBufferTextureCopyPitchSupported)
         return !(loc->PlacedFootprint.Offset & (D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT - 1)) &&
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateBuffer(VkDevice device,
                     {
      return dzn_buffer_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyBuffer(VkDevice device,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetBufferMemoryRequirements2(VkDevice dev,
               {
      VK_FROM_HANDLE(dzn_device, device, dev);
   VK_FROM_HANDLE(dzn_buffer, buffer, pInfo->buffer);
   struct dzn_physical_device *pdev =
            uint32_t alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
            if (buffer->usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
      alignment = MAX2(alignment, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
               pMemoryRequirements->memoryRequirements.size = size;
   pMemoryRequirements->memoryRequirements.alignment = alignment;
   pMemoryRequirements->memoryRequirements.memoryTypeBits =
            vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *requirements =
         requirements->requiresDedicatedAllocation = false;
   requirements->prefersDedicatedAllocation = false;
               default:
      dzn_debug_ignored_stype(ext->sType);
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_BindBufferMemory2(VkDevice _device,
               {
               for (uint32_t i = 0; i < bindInfoCount; i++) {
               VK_FROM_HANDLE(dzn_device_memory, mem, pBindInfos[i].memory);
            if (mem->dedicated_res) {
      assert(pBindInfos[i].memoryOffset == 0 &&
         buffer->res = mem->dedicated_res;
      } else {
      D3D12_RESOURCE_DESC desc = buffer->desc;
   desc.Flags |= mem->res_flags;
   if (FAILED(ID3D12Device1_CreatePlacedResource(device->dev, mem->heap,
                                                            if (device->bindless) {
      struct dzn_buffer_desc buf_desc = {
      .buffer = buffer,
   .offset = 0,
      };
   if (buffer->cbv_bindless_slot >= 0) {
      buf_desc.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   dzn_descriptor_heap_write_buffer_desc(device,
                        }
   if (buffer->uav_bindless_slot >= 0) {
      buf_desc.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
   dzn_descriptor_heap_write_buffer_desc(device,
                                          }
      static void
   dzn_event_destroy(struct dzn_event *event,
         {
      if (!event)
            struct dzn_device *device =
            if (event->fence)
            vk_object_base_finish(&event->base);
      }
      static VkResult
   dzn_event_create(struct dzn_device *device,
                     {
      struct dzn_event *event =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*event), 8,
      if (!event)
                     if (FAILED(ID3D12Device1_CreateFence(device->dev, 0, D3D12_FENCE_FLAG_NONE,
                  dzn_event_destroy(event, pAllocator);
               *out = dzn_event_to_handle(event);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateEvent(VkDevice device,
                     {
      return dzn_event_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyEvent(VkDevice device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_ResetEvent(VkDevice dev,
         {
      VK_FROM_HANDLE(dzn_device, device, dev);
            if (FAILED(ID3D12Fence_Signal(event->fence, 0)))
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_SetEvent(VkDevice dev,
         {
      VK_FROM_HANDLE(dzn_device, device, dev);
            if (FAILED(ID3D12Fence_Signal(event->fence, 1)))
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetEventStatus(VkDevice device,
         {
               return ID3D12Fence_GetCompletedValue(event->fence) == 0 ?
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetDeviceMemoryCommitment(VkDevice device,
               {
               // TODO: find if there's a way to query/track actual heap residency
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_QueueBindSparse(VkQueue queue,
                     {
      // FIXME: add proper implem
   dzn_stub();
      }
      static D3D12_TEXTURE_ADDRESS_MODE
   dzn_sampler_translate_addr_mode(VkSamplerAddressMode in)
   {
      switch (in) {
   case VK_SAMPLER_ADDRESS_MODE_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
   case VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
   case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
   case VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
   case VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
   default: unreachable("Invalid address mode");
      }
      static void
   dzn_sampler_destroy(struct dzn_sampler *sampler,
         {
      if (!sampler)
            struct dzn_device *device =
                     vk_object_base_finish(&sampler->base);
      }
      static VkResult
   dzn_sampler_create(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   struct dzn_sampler *sampler =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*sampler), 8,
      if (!sampler)
                     const VkSamplerCustomBorderColorCreateInfoEXT *pBorderColor = (const VkSamplerCustomBorderColorCreateInfoEXT *)
            /* TODO: have a sampler pool to allocate shader-invisible descs which we
   * can copy to the desc_set when UpdateDescriptorSets() is called.
   */
   sampler->desc.Filter = dzn_translate_sampler_filter(pdev, pCreateInfo);
   sampler->desc.AddressU = dzn_sampler_translate_addr_mode(pCreateInfo->addressModeU);
   sampler->desc.AddressV = dzn_sampler_translate_addr_mode(pCreateInfo->addressModeV);
   sampler->desc.AddressW = dzn_sampler_translate_addr_mode(pCreateInfo->addressModeW);
   sampler->desc.MipLODBias = pCreateInfo->mipLodBias;
   sampler->desc.MaxAnisotropy = pCreateInfo->maxAnisotropy;
   sampler->desc.MinLOD = pCreateInfo->minLod;
            if (pCreateInfo->compareEnable)
            bool reads_border_color =
      pCreateInfo->addressModeU == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
   pCreateInfo->addressModeV == VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER ||
         if (reads_border_color) {
      switch (pCreateInfo->borderColor) {
   case VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK:
      sampler->desc.FloatBorderColor[0] = 0.0f;
   sampler->desc.FloatBorderColor[1] = 0.0f;
   sampler->desc.FloatBorderColor[2] = 0.0f;
   sampler->desc.FloatBorderColor[3] =
         sampler->static_border_color =
      pCreateInfo->borderColor == VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK ?
   D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
         case VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE:
      sampler->desc.FloatBorderColor[0] = sampler->desc.FloatBorderColor[1] = 1.0f;
   sampler->desc.FloatBorderColor[2] = sampler->desc.FloatBorderColor[3] = 1.0f;
   sampler->static_border_color = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
      case VK_BORDER_COLOR_FLOAT_CUSTOM_EXT:
      sampler->static_border_color = (D3D12_STATIC_BORDER_COLOR)-1;
   for (unsigned i = 0; i < ARRAY_SIZE(sampler->desc.FloatBorderColor); i++)
            case VK_BORDER_COLOR_INT_TRANSPARENT_BLACK:
   case VK_BORDER_COLOR_INT_OPAQUE_BLACK:
      sampler->desc.UintBorderColor[0] = 0;
   sampler->desc.UintBorderColor[1] = 0;
   sampler->desc.UintBorderColor[2] = 0;
   sampler->desc.UintBorderColor[3] =
         sampler->static_border_color =
      pCreateInfo->borderColor == VK_BORDER_COLOR_INT_TRANSPARENT_BLACK ?
   D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK :
      sampler->desc.Flags = D3D12_SAMPLER_FLAG_UINT_BORDER_COLOR;
      case VK_BORDER_COLOR_INT_OPAQUE_WHITE:
      sampler->desc.UintBorderColor[0] = sampler->desc.UintBorderColor[1] = 1;
   sampler->desc.UintBorderColor[2] = sampler->desc.UintBorderColor[3] = 1;
   sampler->static_border_color = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE_UINT;
   sampler->desc.Flags = D3D12_SAMPLER_FLAG_UINT_BORDER_COLOR;
      case VK_BORDER_COLOR_INT_CUSTOM_EXT:
      sampler->static_border_color = (D3D12_STATIC_BORDER_COLOR)-1;
   for (unsigned i = 0; i < ARRAY_SIZE(sampler->desc.UintBorderColor); i++)
         sampler->desc.Flags = D3D12_SAMPLER_FLAG_UINT_BORDER_COLOR;
      default:
                     if (pCreateInfo->unnormalizedCoordinates && pdev->options17.NonNormalizedCoordinateSamplersSupported)
            sampler->bindless_slot = -1;
   if (device->bindless) {
      sampler->bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
   if (sampler->bindless_slot < 0) {
      dzn_sampler_destroy(sampler, pAllocator);
               dzn_descriptor_heap_write_sampler_desc(device,
                           *out = dzn_sampler_to_handle(sampler);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateSampler(VkDevice device,
                     {
      return dzn_sampler_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroySampler(VkDevice device,
               {
         }
      int
   dzn_device_descriptor_heap_alloc_slot(struct dzn_device *device,
         {
      struct dzn_device_descriptor_heap *heap = &device->device_heaps[type];
            int ret = -1;
   if (heap->slot_freelist.size)
         else if (heap->next_alloc_slot < heap->heap.desc_count)
            mtx_unlock(&heap->lock);
      }
      void
   dzn_device_descriptor_heap_free_slot(struct dzn_device *device,
               {
      struct dzn_device_descriptor_heap *heap = &device->device_heaps[type];
            if (slot < 0)
            mtx_lock(&heap->lock);
   util_dynarray_append(&heap->slot_freelist, int, slot);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetDeviceGroupPeerMemoryFeatures(VkDevice device,
                           {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetImageSparseMemoryRequirements2(VkDevice device,
                     {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateSamplerYcbcrConversion(VkDevice device,
                     {
      unreachable("Ycbcr sampler conversion is not supported");
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroySamplerYcbcrConversion(VkDevice device,
               {
         }
      VKAPI_ATTR VkDeviceAddress VKAPI_CALL
   dzn_GetBufferDeviceAddress(VkDevice device,
         {
                  }
      VKAPI_ATTR uint64_t VKAPI_CALL
   dzn_GetBufferOpaqueCaptureAddress(VkDevice device,
         {
         }
      VKAPI_ATTR uint64_t VKAPI_CALL
   dzn_GetDeviceMemoryOpaqueCaptureAddress(VkDevice device,
         {
         }
      #ifdef _WIN32
   VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetMemoryWin32HandleKHR(VkDevice device,
               {
      VK_FROM_HANDLE(dzn_device_memory, mem, pGetWin32HandleInfo->memory);
   if (!mem->export_handle)
            switch (pGetWin32HandleInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
      if (!DuplicateHandle(GetCurrentProcess(), mem->export_handle, GetCurrentProcess(), pHandle,
                  default:
            }
   #else
   VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetMemoryFdKHR(VkDevice device,
               {
      VK_FROM_HANDLE(dzn_device_memory, mem, pGetFdInfo->memory);
   if (!mem->export_handle)
            switch (pGetFdInfo->handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
      *pFd = (int)(intptr_t)mem->export_handle;
   mem->export_handle = (HANDLE)(intptr_t)-1;
      default:
            }
   #endif
      #ifdef _WIN32
   VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetMemoryWin32HandlePropertiesKHR(VkDevice _device,
                     {
   #else
   VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetMemoryFdPropertiesKHR(VkDevice _device,
                     {
         #endif
      VK_FROM_HANDLE(dzn_device, device, _device);
   IUnknown *opened_object;
   if (FAILED(ID3D12Device_OpenSharedHandle(device->dev, handle, &IID_IUnknown, (void **)&opened_object)))
            VkResult result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
   ID3D12Resource *res = NULL;
   ID3D12Heap *heap = NULL;
            switch (handleType) {
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT:
      (void)IUnknown_QueryInterface(opened_object, &IID_ID3D12Resource, (void **)&res);
   (void)IUnknown_QueryInterface(opened_object, &IID_ID3D12Heap, (void **)&heap);
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT:
   case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_RESOURCE_BIT:
      (void)IUnknown_QueryInterface(opened_object, &IID_ID3D12Resource, (void **)&res);
      case VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT:
      (void)IUnknown_QueryInterface(opened_object, &IID_ID3D12Heap, (void **)&heap);
      default:
         }
   if (!res && !heap)
            D3D12_HEAP_DESC heap_desc;
   if (res)
         else
         if (heap_desc.Properties.Type != D3D12_HEAP_TYPE_CUSTOM)
            for (uint32_t i = 0; i < pdev->memory.memoryTypeCount; ++i) {
      const VkMemoryType *mem_type = &pdev->memory.memoryTypes[i];
   D3D12_HEAP_PROPERTIES required_props = deduce_heap_properties_from_memory(pdev, mem_type);
   if (heap_desc.Properties.CPUPageProperty != required_props.CPUPageProperty ||
                  D3D12_HEAP_FLAGS required_flags = dzn_physical_device_get_heap_flags_for_mem_type(pdev, i);
   if ((heap_desc.Flags & required_flags) != required_flags)
               }
         cleanup:
      IUnknown_Release(opened_object);
   if (res)
         if (heap)
            }
      #if defined(_WIN32) && D3D12_SDK_VERSION >= 611
   VKAPI_ATTR VkResult VKAPI_CALL
   dzn_GetMemoryHostPointerPropertiesEXT(VkDevice _device,
                     {
               if (!device->dev13)
            ID3D12Heap *heap;
   if (FAILED(ID3D12Device13_OpenExistingHeapFromAddress1(device->dev13, pHostPointer, 1, &IID_ID3D12Heap, (void **)&heap)))
            struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   D3D12_HEAP_DESC heap_desc = dzn_ID3D12Heap_GetDesc(heap);
   for (uint32_t i = 0; i < pdev->memory.memoryTypeCount; ++i) {
      const VkMemoryType *mem_type = &pdev->memory.memoryTypes[i];
   D3D12_HEAP_PROPERTIES required_props = deduce_heap_properties_from_memory(pdev, mem_type);
   if (heap_desc.Properties.CPUPageProperty != required_props.CPUPageProperty ||
                     }
   ID3D12Heap_Release(heap);
      }
   #endif
