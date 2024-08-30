   /*
   * Copyright 2021 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_android.h"
      #include <dlfcn.h>
   #include <hardware/gralloc.h>
   #include <hardware/hwvulkan.h>
   #include <vndk/hardware_buffer.h>
   #include <vulkan/vk_icd.h>
      #include "drm-uapi/drm_fourcc.h"
   #include "util/os_file.h"
   #include "vk_android.h"
      #include "vn_buffer.h"
   #include "vn_device.h"
   #include "vn_device_memory.h"
   #include "vn_image.h"
   #include "vn_instance.h"
   #include "vn_physical_device.h"
   #include "vn_queue.h"
      /* perform options supported by CrOS Gralloc */
   #define CROS_GRALLOC_DRM_GET_BUFFER_INFO 4
   #define CROS_GRALLOC_DRM_GET_USAGE 5
   #define CROS_GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT 0x1
      struct vn_android_gralloc {
      const gralloc_module_t *module;
      };
      static struct vn_android_gralloc _vn_android_gralloc;
      static int
   vn_android_gralloc_init()
   {
      /* We preload same-process gralloc hw module along with venus icd. When
   * venus gets preloaded in Zygote, the unspecialized Zygote process
   * defaults to no access to dri nodes. So we MUST NOT invoke any gralloc
   * helpers here to avoid initializing cros gralloc driver.
   */
   static const char CROS_GRALLOC_MODULE_NAME[] = "CrOS Gralloc";
   const gralloc_module_t *gralloc = NULL;
            /* get gralloc module for gralloc buffer info query */
   ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
         if (ret) {
      vn_log(NULL, "failed to open gralloc module(ret=%d)", ret);
               if (strcmp(gralloc->common.name, CROS_GRALLOC_MODULE_NAME) != 0) {
      dlclose(gralloc->common.dso);
   vn_log(NULL, "unexpected gralloc (name: %s)", gralloc->common.name);
               /* check the helper without using it here as mentioned above */
   if (!gralloc->perform) {
      dlclose(gralloc->common.dso);
   vn_log(NULL, "missing required gralloc helper: perform");
                           }
      static inline void
   vn_android_gralloc_fini()
   {
         }
      static void
   vn_android_gralloc_shared_present_usage_init_once()
   {
      const gralloc_module_t *gralloc = _vn_android_gralloc.module;
   uint32_t front_rendering_usage = 0;
   if (gralloc->perform(gralloc, CROS_GRALLOC_DRM_GET_USAGE,
                  assert(front_rendering_usage);
         }
      uint32_t
   vn_android_gralloc_get_shared_present_usage()
   {
      static once_flag once = ONCE_FLAG_INIT;
   call_once(&once, vn_android_gralloc_shared_present_usage_init_once);
      }
      struct cros_gralloc0_buffer_info {
      uint32_t drm_fourcc;
   int num_fds; /* ignored */
   int fds[4];  /* ignored */
   uint64_t modifier;
   uint32_t offset[4];
      };
      struct vn_android_gralloc_buffer_properties {
      uint32_t drm_fourcc;
            /* plane order matches VkImageDrmFormatModifierExplicitCreateInfoEXT */
   uint32_t offset[4];
      };
      static bool
   vn_android_gralloc_get_buffer_properties(
      buffer_handle_t handle,
      {
      const gralloc_module_t *gralloc = _vn_android_gralloc.module;
   struct cros_gralloc0_buffer_info info;
   if (gralloc->perform(gralloc, CROS_GRALLOC_DRM_GET_BUFFER_INFO, handle,
            vn_log(NULL, "CROS_GRALLOC_DRM_GET_BUFFER_INFO failed");
               if (info.modifier == DRM_FORMAT_MOD_INVALID) {
      vn_log(NULL, "Unexpected DRM_FORMAT_MOD_INVALID");
               out_props->drm_fourcc = info.drm_fourcc;
   for (uint32_t i = 0; i < 4; i++) {
      out_props->stride[i] = info.stride[i];
               /* YVU420 has a chroma order of CrCb. So we must swap the planes for CrCb
   * to align with VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM. This is to serve
   * VkImageDrmFormatModifierExplicitCreateInfoEXT explicit plane layouts.
   */
   if (info.drm_fourcc == DRM_FORMAT_YVU420) {
      out_props->stride[1] = info.stride[2];
   out_props->offset[1] = info.offset[2];
   out_props->stride[2] = info.stride[1];
                           }
      static int
   vn_android_gralloc_get_dma_buf_fd(const native_handle_t *handle)
   {
      /* There can be multiple fds wrapped inside a native_handle_t, but we
   * expect the 1st one pointing to the dma_buf. For multi-planar format,
   * there should only exist one undelying dma_buf. The other fd(s) could be
   * dups to the same dma_buf or point to the shared memory used to store
   * gralloc buffer metadata.
   */
            if (handle->numFds < 1) {
      vn_log(NULL, "handle->numFds is %d, expected >= 1", handle->numFds);
               if (handle->data[0] < 0) {
      vn_log(NULL, "handle->data[0] < 0");
                  }
      static int
   vn_hal_open(const struct hw_module_t *mod,
                  static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "");
      PUBLIC struct hwvulkan_module_t HAL_MODULE_INFO_SYM = {
      .common = {
      .tag = HARDWARE_MODULE_TAG,
   .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
   .hal_api_version = HARDWARE_HAL_API_VERSION,
   .id = HWVULKAN_HARDWARE_MODULE_ID,
   .name = "Venus Vulkan HAL",
   .author = "Google LLC",
   .methods = &(hw_module_methods_t) {
               };
      static int
   vn_hal_close(UNUSED struct hw_device_t *dev)
   {
      vn_android_gralloc_fini();
      }
      static hwvulkan_device_t vn_hal_dev = {
   .common = {
      .tag = HARDWARE_DEVICE_TAG,
   .version = HWVULKAN_DEVICE_API_VERSION_0_1,
   .module = &HAL_MODULE_INFO_SYM.common,
      },
   .EnumerateInstanceExtensionProperties = vn_EnumerateInstanceExtensionProperties,
   .CreateInstance = vn_CreateInstance,
   .GetInstanceProcAddr = vn_GetInstanceProcAddr,
   };
      static int
   vn_hal_open(const struct hw_module_t *mod,
               {
               assert(mod == &HAL_MODULE_INFO_SYM.common);
            ret = vn_android_gralloc_init();
   if (ret)
                        }
      static uint32_t
   vn_android_ahb_format_from_vk_format(VkFormat format)
   {
      /* Only non-external AHB compatible formats are expected at:
   * - image format query
   * - memory export allocation
   */
   switch (format) {
   case VK_FORMAT_R8G8B8A8_UNORM:
         case VK_FORMAT_R8G8B8_UNORM:
         case VK_FORMAT_R5G6B5_UNORM_PACK16:
         case VK_FORMAT_R16G16B16A16_SFLOAT:
         case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
         default:
            }
      const VkFormat *
   vn_android_format_to_view_formats(VkFormat format, uint32_t *out_count)
   {
      /* For AHB image prop query and creation, venus overrides the tiling to
   * VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT, which requires to chain
   * VkImageFormatListCreateInfo struct in the corresponding pNext when the
   * VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT is set. Those AHB images are assumed
   * to be mutable no more than sRGB-ness, and the implementations can fail
   * whenever going beyond.
   *
   * This helper provides the view formats that have sRGB variants for the
   * image format that venus supports.
   */
   static const VkFormat view_formats_r8g8b8a8[] = {
         };
   static const VkFormat view_formats_r8g8b8[] = { VK_FORMAT_R8G8B8_UNORM,
            switch (format) {
   case VK_FORMAT_R8G8B8A8_UNORM:
      *out_count = ARRAY_SIZE(view_formats_r8g8b8a8);
   return view_formats_r8g8b8a8;
      case VK_FORMAT_R8G8B8_UNORM:
      *out_count = ARRAY_SIZE(view_formats_r8g8b8);
   return view_formats_r8g8b8;
      default:
      /* let the caller handle the fallback case */
   *out_count = 0;
         }
      VkFormat
   vn_android_drm_format_to_vk_format(uint32_t format)
   {
      switch (format) {
   case DRM_FORMAT_ABGR8888:
   case DRM_FORMAT_XBGR8888:
         case DRM_FORMAT_BGR888:
         case DRM_FORMAT_RGB565:
         case DRM_FORMAT_ABGR16161616F:
         case DRM_FORMAT_ABGR2101010:
         case DRM_FORMAT_YVU420:
         case DRM_FORMAT_NV12:
         default:
            }
      static bool
   vn_android_drm_format_is_yuv(uint32_t format)
   {
               switch (format) {
   case DRM_FORMAT_YVU420:
   case DRM_FORMAT_NV12:
         default:
            }
      uint64_t
   vn_android_get_ahb_usage(const VkImageUsageFlags usage,
         {
      uint64_t ahb_usage = 0;
   if (usage &
      (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
         if (usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                  if (flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
            if (flags & VK_IMAGE_CREATE_PROTECTED_BIT)
            /* must include at least one GPU usage flag */
   if (ahb_usage == 0)
               }
      VkResult
   vn_GetSwapchainGrallocUsage2ANDROID(
      VkDevice device,
   VkFormat format,
   VkImageUsageFlags imageUsage,
   VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
   uint64_t *grallocConsumerUsage,
      {
               if (VN_DEBUG(WSI)) {
      vn_log(dev->instance,
                     *grallocConsumerUsage = 0;
   *grallocProducerUsage = 0;
   if (imageUsage & (VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  if (imageUsage &
      (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
   VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
         if (swapchainImageUsage & VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID)
               }
      static VkResult
   vn_android_get_modifier_properties(struct vn_device *dev,
                           {
      VkPhysicalDevice physical_device =
         VkDrmFormatModifierPropertiesListEXT mod_prop_list = {
      .sType = VK_STRUCTURE_TYPE_DRM_FORMAT_MODIFIER_PROPERTIES_LIST_EXT,
   .pNext = NULL,
   .drmFormatModifierCount = 0,
      };
   VkFormatProperties2 format_prop = {
      .sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2,
      };
   VkDrmFormatModifierPropertiesEXT *mod_props = NULL;
            vn_GetPhysicalDeviceFormatProperties2(physical_device, format,
            if (!mod_prop_list.drmFormatModifierCount) {
      vn_log(dev->instance, "No compatible modifier for VkFormat(%u)",
                     mod_props = vk_zalloc(
      alloc, sizeof(*mod_props) * mod_prop_list.drmFormatModifierCount,
      if (!mod_props)
            mod_prop_list.pDrmFormatModifierProperties = mod_props;
   vn_GetPhysicalDeviceFormatProperties2(physical_device, format,
            for (uint32_t i = 0; i < mod_prop_list.drmFormatModifierCount; i++) {
      if (mod_props[i].drmFormatModifier == modifier) {
      *out_props = mod_props[i];
   modifier_found = true;
                           if (!modifier_found) {
      vn_log(dev->instance,
         "No matching modifier(%" PRIu64 ") properties for VkFormat(%u)",
                  }
      struct vn_android_image_builder {
      VkImageCreateInfo create;
   VkSubresourceLayout layouts[4];
   VkImageDrmFormatModifierExplicitCreateInfoEXT modifier;
   VkExternalMemoryImageCreateInfo external;
      };
      static VkResult
   vn_android_get_image_builder(struct vn_device *dev,
                           {
      VkResult result = VK_SUCCESS;
   struct vn_android_gralloc_buffer_properties buf_props;
   VkDrmFormatModifierPropertiesEXT mod_props;
   uint32_t vcount = 0;
            /* Android image builder is only used by ANB or AHB. For ANB, Android
   * Vulkan loader will never pass the below structs. For AHB, struct
   * vn_image_create_deferred_info will never carry below either.
   */
   assert(!vk_find_struct_const(
      create_info->pNext,
      assert(!vk_find_struct_const(create_info->pNext,
            if (!vn_android_gralloc_get_buffer_properties(handle, &buf_props))
            result = vn_android_get_modifier_properties(
         if (result != VK_SUCCESS)
            /* fill VkImageCreateInfo */
   memset(out_builder, 0, sizeof(*out_builder));
   out_builder->create = *create_info;
            /* fill VkImageDrmFormatModifierExplicitCreateInfoEXT */
   for (uint32_t i = 0; i < mod_props.drmFormatModifierPlaneCount; i++) {
      out_builder->layouts[i].offset = buf_props.offset[i];
      }
   out_builder->modifier = (VkImageDrmFormatModifierExplicitCreateInfoEXT){
      .sType =
         .pNext = out_builder->create.pNext,
   .drmFormatModifier = buf_props.modifier,
   .drmFormatModifierPlaneCount = mod_props.drmFormatModifierPlaneCount,
      };
            /* fill VkExternalMemoryImageCreateInfo */
   out_builder->external = (VkExternalMemoryImageCreateInfo){
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
   .pNext = out_builder->create.pNext,
      };
            /* fill VkImageFormatListCreateInfo if needed
   *
   * vn_image::deferred_info only stores VkImageFormatListCreateInfo with a
   * non-zero viewFormatCount, and that stored struct will be respected.
   */
   if ((create_info->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) &&
      !vk_find_struct_const(create_info->pNext,
         /* 12.3. Images
   *
   * If tiling is VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT and flags
   * contains VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT, then the pNext chain
   * must include a VkImageFormatListCreateInfo structure with non-zero
   * viewFormatCount.
   */
   vformats =
         if (!vformats) {
      /* image builder struct persists through the image creation call */
   vformats = &out_builder->create.format;
      }
   out_builder->list = (VkImageFormatListCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
   .pNext = out_builder->create.pNext,
   .viewFormatCount = vcount,
      };
                  }
      VkResult
   vn_android_image_from_anb(struct vn_device *dev,
                           {
      /* If anb_info->handle points to a classic resouce created from
   * virtio_gpu_cmd_resource_create_3d, anb_info->stride is the stride of the
   * guest shadow storage other than the host gpu storage.
   *
   * We also need to pass the correct stride to vn_CreateImage, which will be
   * done via VkImageDrmFormatModifierExplicitCreateInfoEXT and will require
   * VK_EXT_image_drm_format_modifier support in the host driver. The struct
   * needs host storage info which can be queried from cros gralloc.
   */
   VkResult result = VK_SUCCESS;
   VkDevice device = vn_device_to_handle(dev);
   VkDeviceMemory memory = VK_NULL_HANDLE;
   VkImage image = VK_NULL_HANDLE;
   struct vn_image *img = NULL;
   uint64_t alloc_size = 0;
   uint32_t mem_type_bits = 0;
   int dma_buf_fd = -1;
   int dup_fd = -1;
   VkImageCreateInfo local_create_info;
            dma_buf_fd = vn_android_gralloc_get_dma_buf_fd(anb_info->handle);
   if (dma_buf_fd < 0) {
      result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
               assert(!(create_info->flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT));
   assert(!vk_find_struct_const(create_info->pNext,
         assert(!vk_find_struct_const(create_info->pNext,
            /* strip VkNativeBufferANDROID and VkSwapchainImageCreateInfoANDROID */
   local_create_info = *create_info;
   local_create_info.pNext = NULL;
   result = vn_android_get_image_builder(dev, &local_create_info,
         if (result != VK_SUCCESS)
            /* encoder will strip the Android specific pNext structs */
   result = vn_image_create(dev, &builder.create, alloc, &img);
   if (result != VK_SUCCESS) {
      if (VN_DEBUG(WSI))
                              const VkMemoryRequirements *mem_req =
         if (!mem_req->memoryTypeBits) {
      if (VN_DEBUG(WSI))
         result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
               result = vn_get_memory_dma_buf_properties(dev, dma_buf_fd, &alloc_size,
         if (result != VK_SUCCESS)
            if (VN_DEBUG(WSI)) {
      vn_log(dev->instance,
         "size = img(%" PRIu64 ") fd(%" PRIu64 "), "
   "memoryTypeBits = img(0x%X) & fd(0x%X)",
               if (alloc_size < mem_req->size) {
      if (VN_DEBUG(WSI)) {
      vn_log(dev->instance,
            }
   result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
               mem_type_bits &= mem_req->memoryTypeBits;
   if (!mem_type_bits) {
      result = VK_ERROR_INVALID_EXTERNAL_HANDLE;
               dup_fd = os_dupfd_cloexec(dma_buf_fd);
   if (dup_fd < 0) {
      result = (errno == EMFILE) ? VK_ERROR_TOO_MANY_OBJECTS
                     const VkImportMemoryFdInfoKHR import_fd_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
   .pNext = NULL,
   .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_DMA_BUF_BIT_EXT,
      };
   const VkMemoryAllocateInfo memory_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &import_fd_info,
   .allocationSize = mem_req->size,
      };
   result = vn_AllocateMemory(device, &memory_info, alloc, &memory);
   if (result != VK_SUCCESS) {
      /* only need to close the dup_fd on import failure */
   close(dup_fd);
               const VkBindImageMemoryInfo bind_info = {
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
   .pNext = NULL,
   .image = image,
   .memory = memory,
      };
   result = vn_BindImageMemory2(device, 1, &bind_info);
   if (result != VK_SUCCESS)
            img->wsi.is_wsi = true;
   img->wsi.tiling_override = builder.create.tiling;
   img->wsi.drm_format_modifier = builder.modifier.drmFormatModifier;
   /* Android WSI image owns the memory */
   img->wsi.memory = vn_device_memory_from_handle(memory);
   img->wsi.memory_owned = true;
                  fail:
      if (image != VK_NULL_HANDLE)
         if (memory != VK_NULL_HANDLE)
            }
      static VkResult
   vn_android_get_ahb_format_properties(
      struct vn_device *dev,
   const struct AHardwareBuffer *ahb,
      {
      AHardwareBuffer_Desc desc;
   VkFormat format;
   struct vn_android_gralloc_buffer_properties buf_props;
            AHardwareBuffer_describe(ahb, &desc);
   if (!(desc.usage & (AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                  vn_log(dev->instance,
         "AHB usage(%" PRIu64 ") must include at least one GPU bit",
               /* Handle the special AHARDWAREBUFFER_FORMAT_BLOB for VkBuffer case. */
   if (desc.format == AHARDWAREBUFFER_FORMAT_BLOB) {
      out_props->format = VK_FORMAT_UNDEFINED;
               if (!vn_android_gralloc_get_buffer_properties(
                  /* We implement AHB extension support with EXT_image_drm_format_modifier.
   * It requires us to have a compatible VkFormat but not DRM formats. So if
   * the ahb is not intended for backing a VkBuffer, error out early if the
   * format is VK_FORMAT_UNDEFINED.
   */
   format = vn_android_drm_format_to_vk_format(buf_props.drm_fourcc);
   if (format == VK_FORMAT_UNDEFINED) {
      vn_log(dev->instance, "Unknown drm_fourcc(%u) from AHB format(0x%X)",
                     VkResult result = vn_android_get_modifier_properties(
         if (result != VK_SUCCESS)
            /* The spec requires that formatFeatures must include at least one of
   * VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT or
   * VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT.
   */
   const VkFormatFeatureFlags format_features =
      mod_props.drmFormatModifierTilingFeatures |
         /* 11.2.7. Android Hardware Buffer External Memory
   *
   * Implementations may not always be able to determine the color model,
   * numerical range, or chroma offsets of the image contents, so the values
   * in VkAndroidHardwareBufferFormatPropertiesANDROID are only suggestions.
   * Applications should treat these values as sensible defaults to use in the
   * absence of more reliable information obtained through some other means.
   */
   const bool is_yuv = vn_android_drm_format_is_yuv(buf_props.drm_fourcc);
   const VkSamplerYcbcrModelConversion model =
      is_yuv ? VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601
         /* ANGLE expects VK_FORMAT_UNDEFINED with externalFormat resolved from
   * AHARDWAREBUFFER_FORMAT_IMPLEMENTATION_DEFINED and any supported planar
   * AHB formats. Venus supports below explicit ones:
   * - AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420 (DRM_FORMAT_NV12)
   * - AHARDWAREBUFFER_FORMAT_YV12 (DRM_FORMAT_YVU420)
   */
   if (desc.format == AHARDWAREBUFFER_FORMAT_IMPLEMENTATION_DEFINED || is_yuv)
            *out_props = (VkAndroidHardwareBufferFormatPropertiesANDROID) {
      .sType = out_props->sType,
   .pNext = out_props->pNext,
   .format = format,
   .externalFormat = buf_props.drm_fourcc,
   .formatFeatures = format_features,
   .samplerYcbcrConversionComponents = {
      .r = VK_COMPONENT_SWIZZLE_IDENTITY,
   .g = VK_COMPONENT_SWIZZLE_IDENTITY,
   .b = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
   .suggestedYcbcrModel = model,
   /* match EGL_YUV_NARROW_RANGE_EXT used in egl platform_android */
   .suggestedYcbcrRange = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW,
   .suggestedXChromaOffset = VK_CHROMA_LOCATION_MIDPOINT,
                  }
      VkResult
   vn_GetAndroidHardwareBufferPropertiesANDROID(
      VkDevice device,
   const struct AHardwareBuffer *buffer,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   VkResult result = VK_SUCCESS;
   int dma_buf_fd = -1;
   uint64_t alloc_size = 0;
            VkAndroidHardwareBufferFormatProperties2ANDROID *format_props2 =
      vk_find_struct(pProperties->pNext,
      VkAndroidHardwareBufferFormatPropertiesANDROID *format_props =
      vk_find_struct(pProperties->pNext,
      if (format_props2 || format_props) {
      VkAndroidHardwareBufferFormatPropertiesANDROID local_props = {
      .sType =
      };
   if (!format_props)
            result =
         if (result != VK_SUCCESS)
            if (format_props2) {
      format_props2->format = format_props->format;
   format_props2->externalFormat = format_props->externalFormat;
   format_props2->formatFeatures =
         format_props2->samplerYcbcrConversionComponents =
         format_props2->suggestedYcbcrModel =
         format_props2->suggestedYcbcrRange =
         format_props2->suggestedXChromaOffset =
         format_props2->suggestedYChromaOffset =
                  dma_buf_fd = vn_android_gralloc_get_dma_buf_fd(
         if (dma_buf_fd < 0)
            result = vn_get_memory_dma_buf_properties(dev, dma_buf_fd, &alloc_size,
         if (result != VK_SUCCESS)
            pProperties->allocationSize = alloc_size;
               }
      static AHardwareBuffer *
   vn_android_ahb_allocate(uint32_t width,
                           {
      AHardwareBuffer *ahb = NULL;
   AHardwareBuffer_Desc desc;
            memset(&desc, 0, sizeof(desc));
   desc.width = width;
   desc.height = height;
   desc.layers = layers;
   desc.format = format;
            ret = AHardwareBuffer_allocate(&desc, &ahb);
   if (ret) {
      /* We just log the error code here for now since the platform falsely
   * maps all gralloc allocation failures to oom.
   */
   vn_log(NULL, "AHB alloc(w=%u,h=%u,l=%u,f=%u,u=%" PRIu64 ") failed(%d)",
                        }
      bool
   vn_android_get_drm_format_modifier_info(
      const VkPhysicalDeviceImageFormatInfo2 *format_info,
      {
      /* To properly fill VkPhysicalDeviceImageDrmFormatModifierInfoEXT, we have
   * to allocate an ahb to retrieve the drm format modifier. For the image
   * sharing mode, we assume VK_SHARING_MODE_EXCLUSIVE for now.
   */
   AHardwareBuffer *ahb = NULL;
   uint32_t format = 0;
   uint64_t usage = 0;
                     format = vn_android_ahb_format_from_vk_format(format_info->format);
   if (!format)
            usage = vn_android_get_ahb_usage(format_info->usage, format_info->flags);
   ahb = vn_android_ahb_allocate(16, 16, 1, format, usage);
   if (!ahb)
            if (!vn_android_gralloc_get_buffer_properties(
            AHardwareBuffer_release(ahb);
               *out_info = (VkPhysicalDeviceImageDrmFormatModifierInfoEXT){
      .sType =
         .pNext = NULL,
   .drmFormatModifier = buf_props.modifier,
   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
   .queueFamilyIndexCount = 0,
               AHardwareBuffer_release(ahb);
      }
      VkResult
   vn_android_device_import_ahb(struct vn_device *dev,
                                 {
      const VkMemoryDedicatedAllocateInfo *dedicated_info =
         const native_handle_t *handle = NULL;
   int dma_buf_fd = -1;
   int dup_fd = -1;
   uint64_t alloc_size = 0;
   uint32_t mem_type_bits = 0;
   uint32_t mem_type_index = alloc_info->memoryTypeIndex;
   bool force_unmappable = false;
            handle = AHardwareBuffer_getNativeHandle(ahb);
   dma_buf_fd = vn_android_gralloc_get_dma_buf_fd(handle);
   if (dma_buf_fd < 0)
            result = vn_get_memory_dma_buf_properties(dev, dma_buf_fd, &alloc_size,
         if (result != VK_SUCCESS)
            /* If ahb is for an image, finish the deferred image creation first */
   if (dedicated_info && dedicated_info->image != VK_NULL_HANDLE) {
      struct vn_image *img = vn_image_from_handle(dedicated_info->image);
            result = vn_android_get_image_builder(dev, &img->deferred_info->create,
         if (result != VK_SUCCESS)
            result = vn_image_init_deferred(dev, &builder.create, img);
   if (result != VK_SUCCESS)
            const VkMemoryRequirements *mem_req =
         if (alloc_size < mem_req->size) {
      vn_log(dev->instance,
         "alloc_size(%" PRIu64 ") mem_req->size(%" PRIu64 ")",
                        /* XXX Workaround before spec issue #2762 gets resolved. If importing an
   * internally allocated AHB from the exportable path, memoryTypeIndex is
   * undefined while defaulting to zero, which can be incompatible with
   * the queried memoryTypeBits from the combined memory requirement and
   * dma_buf fd properties. Thus we override the requested memoryTypeIndex
   * to an applicable one if existed.
   */
   if (internal_ahb) {
      if ((mem_type_bits & mem_req->memoryTypeBits) == 0) {
      vn_log(dev->instance, "memoryTypeBits: img(0x%X) fd(0x%X)",
                                 /* XXX Workaround before we use cross-domain backend in minigbm. The
   * blob_mem allocated from virgl backend can have a queried guest
   * mappable size smaller than the size returned from image memory
   * requirement.
   */
               if (dedicated_info && dedicated_info->buffer != VK_NULL_HANDLE) {
      struct vn_buffer *buf = vn_buffer_from_handle(dedicated_info->buffer);
   const VkMemoryRequirements *mem_req =
         if (alloc_size < mem_req->size) {
      vn_log(dev->instance,
         "alloc_size(%" PRIu64 ") mem_req->size(%" PRIu64 ")",
                                             errno = 0;
   dup_fd = os_dupfd_cloexec(dma_buf_fd);
   if (dup_fd < 0)
      return (errno == EMFILE) ? VK_ERROR_TOO_MANY_OBJECTS
         /* Spec requires AHB export info to be present, so we must strip it. In
   * practice, the AHB import path here only needs the main allocation info
   * and the dedicated_info.
   */
   VkMemoryDedicatedAllocateInfo local_dedicated_info;
   /* Override when dedicated_info exists and is not the tail struct. */
   if (dedicated_info && dedicated_info->pNext) {
      local_dedicated_info = *dedicated_info;
   local_dedicated_info.pNext = NULL;
      }
   const VkMemoryAllocateInfo local_alloc_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = dedicated_info,
   .allocationSize = alloc_size,
      };
   result = vn_device_memory_import_dma_buf(dev, mem, &local_alloc_info,
         if (result != VK_SUCCESS) {
      close(dup_fd);
               AHardwareBuffer_acquire(ahb);
               }
      VkResult
   vn_android_device_allocate_ahb(struct vn_device *dev,
                     {
      const VkMemoryDedicatedAllocateInfo *dedicated_info =
         uint32_t width = 0;
   uint32_t height = 1;
   uint32_t layers = 1;
   uint32_t format = 0;
   uint64_t usage = 0;
            if (dedicated_info && dedicated_info->image != VK_NULL_HANDLE) {
      const VkImageCreateInfo *image_info =
         assert(image_info);
   width = image_info->extent.width;
   height = image_info->extent.height;
   layers = image_info->arrayLayers;
   format = vn_android_ahb_format_from_vk_format(image_info->format);
      } else {
      const VkPhysicalDeviceMemoryProperties *mem_props =
                     width = alloc_info->allocationSize;
   format = AHARDWAREBUFFER_FORMAT_BLOB;
   usage = AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER;
   if (mem_props->memoryTypes[alloc_info->memoryTypeIndex].propertyFlags &
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
   usage |= AHARDWAREBUFFER_USAGE_CPU_READ_RARELY |
                  ahb = vn_android_ahb_allocate(width, height, layers, format, usage);
   if (!ahb)
            VkResult result =
            /* ahb alloc has already acquired a ref and import will acquire another,
   * must release one here to avoid leak.
   */
               }
      void
   vn_android_release_ahb(struct AHardwareBuffer *ahb)
   {
         }
      VkResult
   vn_GetMemoryAndroidHardwareBufferANDROID(
      VkDevice device,
   const VkMemoryGetAndroidHardwareBufferInfoANDROID *pInfo,
      {
               AHardwareBuffer_acquire(mem->ahb);
               }
      uint32_t
   vn_android_get_ahb_buffer_memory_type_bits(struct vn_device *dev)
   {
      static const uint32_t format = AHARDWAREBUFFER_FORMAT_BLOB;
   /* ensure dma_buf_memory_type_bits covers host visible usage */
   static const uint64_t usage = AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER |
               AHardwareBuffer *ahb = vn_android_ahb_allocate(4096, 1, 1, format, usage);
   if (!ahb)
            int dma_buf_fd =
         if (dma_buf_fd < 0) {
      AHardwareBuffer_release(ahb);
               uint64_t alloc_size = 0;
   uint32_t mem_type_bits = 0;
   VkResult ret = vn_get_memory_dma_buf_properties(
         /* release ahb first as below no longer needs it */
            if (ret != VK_SUCCESS) {
      vn_log(dev->instance, "AHB buffer mem type bits query failed %d", ret);
                  }
