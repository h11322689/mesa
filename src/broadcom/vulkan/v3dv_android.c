   /*
   * Copyright © 2017, Google Inc.
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
      #include "v3dv_private.h"
   #include <hardware/gralloc.h>
      #if ANDROID_API_LEVEL >= 26
   #include <hardware/gralloc1.h>
   #endif
      #include "drm-uapi/drm_fourcc.h"
   #include <hardware/hardware.h>
   #include <hardware/hwvulkan.h>
      #include <vulkan/vk_android_native_buffer.h>
   #include <vulkan/vk_icd.h>
      #include "vk_android.h"
   #include "vulkan/util/vk_enum_defines.h"
      #include "util/libsync.h"
   #include "util/log.h"
   #include "util/os_file.h"
      static int
   v3dv_hal_open(const struct hw_module_t *mod,
               static int
   v3dv_hal_close(struct hw_device_t *dev);
      static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "");
      PUBLIC struct hwvulkan_module_t HAL_MODULE_INFO_SYM = {
      .common =
   {
      .tag = HARDWARE_MODULE_TAG,
   .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
   .hal_api_version = HARDWARE_MAKE_API_VERSION(1, 0),
   .id = HWVULKAN_HARDWARE_MODULE_ID,
   .name = "Broadcom Vulkan HAL",
   .author = "Mesa3D",
   .methods =
      &(hw_module_methods_t) {
   .open = v3dv_hal_open,
      };
      /* If any bits in test_mask are set, then unset them and return true. */
   static inline bool
   unmask32(uint32_t *inout_mask, uint32_t test_mask)
   {
      uint32_t orig_mask = *inout_mask;
   *inout_mask &= ~test_mask;
      }
      static int
   v3dv_hal_open(const struct hw_module_t *mod,
               {
      assert(mod == &HAL_MODULE_INFO_SYM.common);
            hwvulkan_device_t *hal_dev = malloc(sizeof(*hal_dev));
   if (!hal_dev)
            *hal_dev = (hwvulkan_device_t){
      .common =
   {
      .tag = HARDWARE_DEVICE_TAG,
   .version = HWVULKAN_DEVICE_API_VERSION_0_1,
   .module = &HAL_MODULE_INFO_SYM.common,
         .EnumerateInstanceExtensionProperties =
         .CreateInstance = v3dv_CreateInstance,
   .GetInstanceProcAddr = v3dv_GetInstanceProcAddr,
                     *dev = &hal_dev->common;
      }
      static int
   v3dv_hal_close(struct hw_device_t *dev)
   {
      /* hwvulkan.h claims that hw_device_t::close() is never called. */
      }
      VkResult
   v3dv_gralloc_to_drm_explicit_layout(struct u_gralloc *gralloc,
                           {
               if (u_gralloc_get_buffer_basic_info(gralloc, in_hnd, &info) != 0)
            if (info.num_planes > max_planes)
            bool is_disjoint = false;
   for (int i = 1; i < info.num_planes; i++) {
      if (info.offsets[i] == 0) {
      is_disjoint = true;
                  if (is_disjoint) {
      /* We don't support disjoint planes yet */
               memset(out_layouts, 0, sizeof(*out_layouts) * info.num_planes);
            out->sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT;
            out->drmFormatModifier = info.modifier;
   out->drmFormatModifierPlaneCount = info.num_planes;
   for (int i = 0; i < info.num_planes; i++) {
      out_layouts[i].offset = info.offsets[i];
               if (info.drm_fourcc == DRM_FORMAT_YVU420) {
      /* Swap the U and V planes to match the VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM */
   VkSubresourceLayout tmp = out_layouts[1];
   out_layouts[1] = out_layouts[2];
                  }
      VkResult
   v3dv_import_native_buffer_fd(VkDevice device_h,
                     {
                        const VkMemoryDedicatedAllocateInfo ded_alloc = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   .pNext = NULL,
   .buffer = VK_NULL_HANDLE,
               const VkImportMemoryFdInfoKHR import_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
   .pNext = &ded_alloc,
   .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
               result =
      v3dv_AllocateMemory(device_h,
                     &(VkMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            if (result != VK_SUCCESS)
            VkBindImageMemoryInfo bind_info = {
      .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
   .image = image_h,
   .memory = memory_h,
      };
                  fail_create_image:
                  }
      static VkResult
   format_supported_with_usage(VkDevice device_h,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, device_h);
   struct v3dv_physical_device *phys_dev = device->pdevice;
   VkPhysicalDevice phys_dev_h = v3dv_physical_device_to_handle(phys_dev);
            const VkPhysicalDeviceImageFormatInfo2 image_format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = VK_IMAGE_TYPE_2D,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
               VkImageFormatProperties2 image_format_props = {
                  /* Check that requested format and usage are supported. */
   result = v3dv_GetPhysicalDeviceImageFormatProperties2(
         if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
                              }
      static VkResult
   setup_gralloc0_usage(struct v3dv_device *device,
                     {
      if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              /* All VkImageUsageFlags not explicitly checked here are unsupported for
   * gralloc swapchains.
   */
   if (imageUsage != 0) {
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                           /* Swapchain assumes direct displaying, therefore enable COMPOSER flag,
   * In case format is not supported by display controller, gralloc will
   * drop this flag and still allocate the buffer in VRAM
   */
            if (*grallocUsage == 0)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetSwapchainGrallocUsageANDROID(VkDevice device_h,
                     {
      V3DV_FROM_HANDLE(v3dv_device, device, device_h);
            result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
            *grallocUsage = 0;
      }
      #if ANDROID_API_LEVEL >= 26
   VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_GetSwapchainGrallocUsage2ANDROID(
      VkDevice device_h,
   VkFormat format,
   VkImageUsageFlags imageUsage,
   VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
   uint64_t *grallocConsumerUsage,
      {
      V3DV_FROM_HANDLE(v3dv_device, device, device_h);
            *grallocConsumerUsage = 0;
   *grallocProducerUsage = 0;
            result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
            int32_t grallocUsage = 0;
   result = setup_gralloc0_usage(device, format, imageUsage, &grallocUsage);
   if (result != VK_SUCCESS)
                     if (grallocUsage & GRALLOC_USAGE_HW_RENDER) {
                  if (grallocUsage & GRALLOC_USAGE_HW_TEXTURE) {
                  if (grallocUsage & GRALLOC_USAGE_HW_COMPOSER) {
      /* GPU composing case */
   *grallocConsumerUsage |= GRALLOC1_CONSUMER_USAGE_GPU_TEXTURE;
   /* Hardware composing case */
               if (swapchainImageUsage & VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID) {
      uint64_t front_rendering_usage = 0;
   u_gralloc_get_front_rendering_usage(device->gralloc, &front_rendering_usage);
                  }
   #endif
      /* ----------------------------- AHardwareBuffer --------------------------- */
      static VkResult
   get_ahb_buffer_format_properties2(VkDevice device_h, const struct AHardwareBuffer *buffer,
         {
               /* Get a description of buffer contents . */
   AHardwareBuffer_Desc desc;
            /* Verify description. */
   const uint64_t gpu_usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
                  /* "Buffer must be a valid Android hardware buffer object with at least
   * one of the AHARDWAREBUFFER_USAGE_GPU_* usage flags."
   */
   if (!(desc.usage & (gpu_usage)))
            /* Fill properties fields based on description. */
            p->samplerYcbcrConversionComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
            p->suggestedXChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
                                       if (p->format != VK_FORMAT_UNDEFINED)
            /* External format only case
   *
   * From vkGetAndroidHardwareBufferPropertiesANDROID spec:
   * "If the Android hardware buffer has one of the formats listed in the Format
   * Equivalence table (see spec.), then format must have the equivalent Vulkan
   * format listed in the table. Otherwise, format may be VK_FORMAT_UNDEFINED,
   * indicating the Android hardware buffer can only be used with an external format."
   *
   * From SKIA source code analysis: p->format MUST be VK_FORMAT_UNDEFINED, if the
   * format is not in the Equivalence table.
            struct u_gralloc_buffer_handle gr_handle = {
      .handle = AHardwareBuffer_getNativeHandle(buffer),
   .pixel_stride = desc.stride,
                        if (u_gralloc_get_buffer_basic_info(device->gralloc, &gr_handle, &info) != 0)
            switch (info.drm_fourcc) {
   case DRM_FORMAT_YVU420:
      /* Assuming that U and V planes are swapped earlier */
   external_format = VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
      case DRM_FORMAT_NV12:
      external_format = VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
      default:;
      mesa_loge("Unsupported external DRM format: %d", info.drm_fourcc);
               struct u_gralloc_buffer_color_info color_info;
   if (u_gralloc_get_buffer_color_info(device->gralloc, &gr_handle, &color_info) == 0) {
      switch (color_info.yuv_color_space) {
   case __DRI_YUV_COLOR_SPACE_ITU_REC601:
      p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
      case __DRI_YUV_COLOR_SPACE_ITU_REC709:
      p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_709;
      case __DRI_YUV_COLOR_SPACE_ITU_REC2020:
      p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_2020;
      default:
                  p->suggestedYcbcrRange = (color_info.sample_range == __DRI_YUV_NARROW_RANGE) ?
         p->suggestedXChromaOffset = (color_info.horizontal_siting == __DRI_YUV_CHROMA_SITING_0_5) ?
         p->suggestedYChromaOffset = (color_info.vertical_siting == __DRI_YUV_CHROMA_SITING_0_5) ?
            finish:
         v3dv_GetPhysicalDeviceFormatProperties2(v3dv_physical_device_to_handle(device->pdevice),
            /* v3dv doesn't support direct sampling from linear images but has a logic to copy
   * from linear to tiled images implicitly before sampling. Therefore expose optimal
   * features for both linear and optimal tiling.
   */
   p->formatFeatures = format_properties.formatProperties.optimalTilingFeatures;
            /* From vkGetAndroidHardwareBufferPropertiesANDROID spec:
   * "The formatFeatures member *must* include
   *  VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT and at least one of
   *  VK_FORMAT_FEATURE_2_MIDPOINT_CHROMA_SAMPLES_BIT or
   *  VK_FORMAT_FEATURE_2_COSITED_CHROMA_SAMPLES_BIT"
   */
               }
      VkResult
   v3dv_GetAndroidHardwareBufferPropertiesANDROID(VkDevice device_h,
               {
      V3DV_FROM_HANDLE(v3dv_device, dev, device_h);
                     VkAndroidHardwareBufferFormatPropertiesANDROID *format_prop =
            /* Fill format properties of an Android hardware buffer. */
   if (format_prop) {
      VkAndroidHardwareBufferFormatProperties2ANDROID format_prop2 = {
         };
   result = get_ahb_buffer_format_properties2(device_h, buffer, &format_prop2);
   if (result != VK_SUCCESS)
            format_prop->format                 = format_prop2.format;
   format_prop->externalFormat         = format_prop2.externalFormat;
   format_prop->formatFeatures         =
         format_prop->samplerYcbcrConversionComponents =
         format_prop->suggestedYcbcrModel    = format_prop2.suggestedYcbcrModel;
   format_prop->suggestedYcbcrRange    = format_prop2.suggestedYcbcrRange;
   format_prop->suggestedXChromaOffset = format_prop2.suggestedXChromaOffset;
               VkAndroidHardwareBufferFormatProperties2ANDROID *format_prop2 =
         if (format_prop2) {
      result = get_ahb_buffer_format_properties2(device_h, buffer, format_prop2);
   if (result != VK_SUCCESS)
               const native_handle_t *handle = AHardwareBuffer_getNativeHandle(buffer);
   assert(handle && handle->numFds > 0);
            /* All memory types. */
               }
