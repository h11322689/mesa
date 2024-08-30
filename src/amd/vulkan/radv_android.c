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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #ifdef ANDROID
   #include <libsync.h>
   #include <hardware/gralloc.h>
   #include <hardware/hardware.h>
   #include <hardware/hwvulkan.h>
   #include <vulkan/vk_android_native_buffer.h>
   #include <vulkan/vk_icd.h>
      #if ANDROID_API_LEVEL >= 26
   #include <hardware/gralloc1.h>
   #endif
   #endif
      #include "util/os_file.h"
      #include "radv_private.h"
   #include "vk_android.h"
   #include "vk_util.h"
      #ifdef ANDROID
      static int radv_hal_open(const struct hw_module_t *mod, const char *id, struct hw_device_t **dev);
   static int radv_hal_close(struct hw_device_t *dev);
      static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "");
      PUBLIC struct hwvulkan_module_t HAL_MODULE_INFO_SYM = {
      .common =
      {
      .tag = HARDWARE_MODULE_TAG,
   .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
   .hal_api_version = HARDWARE_MAKE_API_VERSION(1, 0),
   .id = HWVULKAN_HARDWARE_MODULE_ID,
   .name = "AMD Vulkan HAL",
   .author = "Google",
   .methods =
      &(hw_module_methods_t){
         };
      /* If any bits in test_mask are set, then unset them and return true. */
   static inline bool
   unmask32(uint32_t *inout_mask, uint32_t test_mask)
   {
      uint32_t orig_mask = *inout_mask;
   *inout_mask &= ~test_mask;
      }
      static int
   radv_hal_open(const struct hw_module_t *mod, const char *id, struct hw_device_t **dev)
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
         .EnumerateInstanceExtensionProperties = radv_EnumerateInstanceExtensionProperties,
   .CreateInstance = radv_CreateInstance,
               *dev = &hal_dev->common;
      }
      static int
   radv_hal_close(struct hw_device_t *dev)
   {
      /* hwvulkan.h claims that hw_device_t::close() is never called. */
      }
      VkResult
   radv_image_from_gralloc(VkDevice device_h, const VkImageCreateInfo *base_info,
                  {
      RADV_FROM_HANDLE(radv_device, device, device_h);
   VkImage image_h = VK_NULL_HANDLE;
   struct radv_image *image = NULL;
            if (gralloc_info->handle->numFds != 1) {
      return vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                           /* Do not close the gralloc handle's dma_buf. The lifetime of the dma_buf
   * must exceed that of the gralloc handle, and we do not own the gralloc
   * handle.
   */
                     const VkImportMemoryFdInfoKHR import_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_FD_INFO_KHR,
   .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT,
               /* Find the first VRAM memory type, or GART for PRIME images. */
   int memory_type_index = -1;
   for (int i = 0; i < device->physical_device->memory_properties.memoryTypeCount; ++i) {
      bool is_local = !!(device->physical_device->memory_properties.memoryTypes[i].propertyFlags &
         bool is_32bit = !!(device->physical_device->memory_types_32bit & (1u << i));
   if (is_local && !is_32bit) {
      memory_type_index = i;
                  /* fallback */
   if (memory_type_index == -1)
            result = radv_AllocateMemory(device_h,
                              &(VkMemoryAllocateInfo){
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &import_info,
   if (result != VK_SUCCESS)
            struct radeon_bo_metadata md;
                     VkExternalMemoryImageCreateInfo external_memory_info = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
   .pNext = updated_base_info.pNext,
                        result = radv_image_create(device_h,
                              &(struct radv_image_create_info){
         if (result != VK_SUCCESS)
                              VkBindImageMemoryInfo bind_info = {.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
                              image->owned_memory = memory_h;
   /* Don't clobber the out-parameter until success is certain. */
                  fail_create_image:
      radv_FreeMemory(device_h, memory_h, alloc);
      }
      VkResult
   radv_GetSwapchainGrallocUsageANDROID(VkDevice device_h, VkFormat format, VkImageUsageFlags imageUsage,
         {
      RADV_FROM_HANDLE(radv_device, device, device_h);
   struct radv_physical_device *phys_dev = device->physical_device;
   VkPhysicalDevice phys_dev_h = radv_physical_device_to_handle(phys_dev);
                     /* WARNING: Android Nougat's libvulkan.so hardcodes the VkImageUsageFlags
   * returned to applications via VkSurfaceCapabilitiesKHR::supportedUsageFlags.
   * The relevant code in libvulkan/swapchain.cpp contains this fun comment:
   *
   *     TODO(jessehall): I think these are right, but haven't thought hard
   *     about it. Do we need to query the driver for support of any of
   *     these?
   *
   * Any disagreement between this function and the hardcoded
   * VkSurfaceCapabilitiesKHR:supportedUsageFlags causes tests
   * dEQP-VK.wsi.android.swapchain.*.image_usage to fail.
            const VkPhysicalDeviceImageFormatInfo2 image_format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = VK_IMAGE_TYPE_2D,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
               VkImageFormatProperties2 image_format_props = {
                  /* Check that requested format and usage are supported. */
   result = radv_GetPhysicalDeviceImageFormatProperties2(phys_dev_h, &image_format_info, &image_format_props);
   if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
                           if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT))
            if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                  /* All VkImageUsageFlags not explicitly checked here are unsupported for
   * gralloc swapchains.
   */
   if (imageUsage != 0) {
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                           /*
   * FINISHME: Advertise all display-supported formats. Mostly
   * DRM_FORMAT_ARGB2101010 and DRM_FORMAT_ABGR2101010, but need to check
   * what we need for 30-bit colors.
   */
   if (format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B5G6R5_UNORM_PACK16) {
                  if (*grallocUsage == 0)
               }
      VkResult
   radv_GetSwapchainGrallocUsage2ANDROID(VkDevice device_h, VkFormat format, VkImageUsageFlags imageUsage,
               {
      /* Before level 26 (Android 8.0/Oreo) the loader uses
      #if ANDROID_API_LEVEL >= 26
      RADV_FROM_HANDLE(radv_device, device, device_h);
   struct radv_physical_device *phys_dev = device->physical_device;
   VkPhysicalDevice phys_dev_h = radv_physical_device_to_handle(phys_dev);
            *grallocConsumerUsage = 0;
            if (swapchainImageUsage & VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID)
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
         const VkPhysicalDeviceImageFormatInfo2 image_format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = VK_IMAGE_TYPE_2D,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
               VkImageFormatProperties2 image_format_props = {
                  /* Check that requested format and usage are supported. */
   result = radv_GetPhysicalDeviceImageFormatProperties2(phys_dev_h, &image_format_info, &image_format_props);
   if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
                           if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
      *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
               if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                        if (imageUsage != 0) {
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                           /*
   * FINISHME: Advertise all display-supported formats. Mostly
   * DRM_FORMAT_ARGB2101010 and DRM_FORMAT_ABGR2101010, but need to check
   * what we need for 30-bit colors.
   */
   if (format == VK_FORMAT_B8G8R8A8_UNORM || format == VK_FORMAT_B5G6R5_UNORM_PACK16) {
      *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
               if (!*grallocProducerUsage && !*grallocConsumerUsage)
               #else
      *grallocConsumerUsage = 0;
   *grallocProducerUsage = 0;
      #endif
   }
   #endif
      #if RADV_SUPPORT_ANDROID_HARDWARE_BUFFER
      enum {
      /* Usage bit equal to GRALLOC_USAGE_HW_CAMERA_MASK */
      };
      static inline VkFormat
   vk_format_from_android(unsigned android_format, unsigned android_usage)
   {
      switch (android_format) {
   case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
         case AHARDWAREBUFFER_FORMAT_IMPLEMENTATION_DEFINED:
      if (android_usage & BUFFER_USAGE_CAMERA_MASK)
         else
      default:
            }
      unsigned
   radv_ahb_format_for_vk_format(VkFormat vk_format)
   {
      switch (vk_format) {
   case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
         default:
            }
      static VkResult
   get_ahb_buffer_format_properties(VkDevice device_h, const struct AHardwareBuffer *buffer,
         {
               /* Get a description of buffer contents . */
   AHardwareBuffer_Desc desc;
            /* Verify description. */
   const uint64_t gpu_usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
            /* "Buffer must be a valid Android hardware buffer object with at least
   * one of the AHARDWAREBUFFER_USAGE_GPU_* usage flags."
   */
   if (!(desc.usage & (gpu_usage)))
            /* Fill properties fields based on description. */
            p->format = vk_format_from_android(desc.format, desc.usage);
                     radv_GetPhysicalDeviceFormatProperties2(radv_physical_device_to_handle(device->physical_device), p->format,
            if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER)
         else
            /* "Images can be created with an external format even if the Android hardware
   *  buffer has a format which has an equivalent Vulkan format to enable
   *  consistent handling of images from sources that might use either category
   *  of format. However, all images created with an external format are subject
   *  to the valid usage requirements associated with external formats, even if
   *  the Android hardware buffer’s format has a Vulkan equivalent."
   *
   * "The formatFeatures member *must* include
   *  VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT and at least one of
   *  VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT or
   *  VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT"
   */
                     /* "Implementations may not always be able to determine the color model,
   *  numerical range, or chroma offsets of the image contents, so the values
   *  in VkAndroidHardwareBufferFormatPropertiesANDROID are only suggestions.
   *  Applications should treat these values as sensible defaults to use in
   *  the absence of more reliable information obtained through some other
   *  means."
   */
   p->samplerYcbcrConversionComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
            p->suggestedXChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
               }
      static VkResult
   get_ahb_buffer_format_properties2(VkDevice device_h, const struct AHardwareBuffer *buffer,
         {
               /* Get a description of buffer contents . */
   AHardwareBuffer_Desc desc;
            /* Verify description. */
   const uint64_t gpu_usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
            /* "Buffer must be a valid Android hardware buffer object with at least
   * one of the AHARDWAREBUFFER_USAGE_GPU_* usage flags."
   */
   if (!(desc.usage & (gpu_usage)))
            /* Fill properties fields based on description. */
            p->format = vk_format_from_android(desc.format, desc.usage);
                     radv_GetPhysicalDeviceFormatProperties2(radv_physical_device_to_handle(device->physical_device), p->format,
            if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER)
         else
            /* "Images can be created with an external format even if the Android hardware
   *  buffer has a format which has an equivalent Vulkan format to enable
   *  consistent handling of images from sources that might use either category
   *  of format. However, all images created with an external format are subject
   *  to the valid usage requirements associated with external formats, even if
   *  the Android hardware buffer’s format has a Vulkan equivalent."
   *
   * "The formatFeatures member *must* include
   *  VK_FORMAT_FEATURE_2_SAMPLED_IMAGE_BIT and at least one of
   *  VK_FORMAT_FEATURE_2_MIDPOINT_CHROMA_SAMPLES_BIT or
   *  VK_FORMAT_FEATURE_2_COSITED_CHROMA_SAMPLES_BIT"
   */
                     /* "Implementations may not always be able to determine the color model,
   *  numerical range, or chroma offsets of the image contents, so the values
   *  in VkAndroidHardwareBufferFormatPropertiesANDROID are only suggestions.
   *  Applications should treat these values as sensible defaults to use in
   *  the absence of more reliable information obtained through some other
   *  means."
   */
   p->samplerYcbcrConversionComponents.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   p->samplerYcbcrConversionComponents.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            p->suggestedYcbcrModel = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
            p->suggestedXChromaOffset = VK_CHROMA_LOCATION_MIDPOINT;
               }
      VkResult
   radv_GetAndroidHardwareBufferPropertiesANDROID(VkDevice device_h, const struct AHardwareBuffer *buffer,
         {
      RADV_FROM_HANDLE(radv_device, dev, device_h);
            VkAndroidHardwareBufferFormatPropertiesANDROID *format_prop =
            /* Fill format properties of an Android hardware buffer. */
   if (format_prop)
            VkAndroidHardwareBufferFormatProperties2ANDROID *format_prop2 =
         if (format_prop2)
            /* NOTE - We support buffers with only one handle but do not error on
   * multiple handle case. Reason is that we want to support YUV formats
   * where we have many logical planes but they all point to the same
   * buffer, like is the case with VK_FORMAT_G8_B8R8_2PLANE_420_UNORM.
   */
   const native_handle_t *handle = AHardwareBuffer_getNativeHandle(buffer);
   int dma_buf = (handle && handle->numFds) ? handle->data[0] : -1;
   if (dma_buf < 0)
            /* All memory types. */
            pProperties->allocationSize = lseek(dma_buf, 0, SEEK_END);
               }
      VkResult
   radv_GetMemoryAndroidHardwareBufferANDROID(VkDevice device_h, const VkMemoryGetAndroidHardwareBufferInfoANDROID *pInfo,
         {
               /* This should always be set due to the export handle types being set on
   * allocation. */
            /* Some quotes from Vulkan spec:
   *
   * "If the device memory was created by importing an Android hardware
   * buffer, vkGetMemoryAndroidHardwareBufferANDROID must return that same
   * Android hardware buffer object."
   *
   * "VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID must
   * have been included in VkExportMemoryAllocateInfo::handleTypes when
   * memory was created."
   */
   *pBuffer = mem->android_hardware_buffer;
   /* Increase refcount. */
   AHardwareBuffer_acquire(mem->android_hardware_buffer);
      }
      #endif
      VkFormat
   radv_select_android_external_format(const void *next, VkFormat default_format)
   {
   #if RADV_SUPPORT_ANDROID_HARDWARE_BUFFER
               if (android_format && android_format->externalFormat) {
            #endif
            }
      VkResult
   radv_import_ahb_memory(struct radv_device *device, struct radv_device_memory *mem, unsigned priority,
         {
   #if RADV_SUPPORT_ANDROID_HARDWARE_BUFFER
      /* Import from AHardwareBuffer to radv_device_memory. */
            /* NOTE - We support buffers with only one handle but do not error on
   * multiple handle case. Reason is that we want to support YUV formats
   * where we have many logical planes but they all point to the same
   * buffer, like is the case with VK_FORMAT_G8_B8R8_2PLANE_420_UNORM.
   */
   int dma_buf = (handle && handle->numFds) ? handle->data[0] : -1;
   if (dma_buf < 0)
            uint64_t alloc_size = 0;
   VkResult result = device->ws->buffer_from_fd(device->ws, dma_buf, priority, &mem->bo, &alloc_size);
   if (result != VK_SUCCESS)
            if (mem->image) {
      struct radeon_bo_metadata metadata;
                     result = radv_image_create_layout(device, create_info, NULL, NULL, mem->image);
   if (result != VK_SUCCESS) {
      device->ws->buffer_destroy(device->ws, mem->bo);
   mem->bo = NULL;
               if (alloc_size < mem->image->size) {
      device->ws->buffer_destroy(device->ws, mem->bo);
   mem->bo = NULL;
         } else if (mem->buffer) {
      if (alloc_size < mem->buffer->vk.size) {
      device->ws->buffer_destroy(device->ws, mem->bo);
   mem->bo = NULL;
                  /* "If the vkAllocateMemory command succeeds, the implementation must
   * acquire a reference to the imported hardware buffer, which it must
   * release when the device memory object is freed. If the command fails,
   * the implementation must not retain a reference."
   */
   AHardwareBuffer_acquire(info->buffer);
               #else /* RADV_SUPPORT_ANDROID_HARDWARE_BUFFER */
         #endif
   }
      VkResult
   radv_create_ahb_memory(struct radv_device *device, struct radv_device_memory *mem, unsigned priority,
         {
   #if RADV_SUPPORT_ANDROID_HARDWARE_BUFFER
      mem->android_hardware_buffer = vk_alloc_ahardware_buffer(pAllocateInfo);
   if (mem->android_hardware_buffer == NULL)
            const struct VkImportAndroidHardwareBufferInfoANDROID import_info = {
                           /* Release a reference to avoid leak for AHB allocation. */
               #else /* RADV_SUPPORT_ANDROID_HARDWARE_BUFFER */
         #endif
   }
      bool
   radv_android_gralloc_supports_format(VkFormat format, VkImageUsageFlagBits usage)
   {
   #if RADV_SUPPORT_ANDROID_HARDWARE_BUFFER
      /* Ideally we check AHardwareBuffer_isSupported.  But that test-allocates on most platforms and
   * seems a bit on the expensive side.  Return true as long as it is a format we understand.
   */
   (void)usage;
      #else
      (void)format;
   (void)usage;
      #endif
   }
