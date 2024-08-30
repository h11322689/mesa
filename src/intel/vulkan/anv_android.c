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
      #include <hardware/gralloc.h>
      #if ANDROID_API_LEVEL >= 26
   #include <hardware/gralloc1.h>
   #endif
      #include <hardware/hardware.h>
   #include <hardware/hwvulkan.h>
   #include <vulkan/vk_android_native_buffer.h>
   #include <vulkan/vk_icd.h>
   #include <sync/sync.h>
      #include "anv_private.h"
   #include "vk_android.h"
   #include "vk_common_entrypoints.h"
   #include "vk_util.h"
      static int anv_hal_open(const struct hw_module_t* mod, const char* id, struct hw_device_t** dev);
   static int anv_hal_close(struct hw_device_t *dev);
      static_assert(HWVULKAN_DISPATCH_MAGIC == ICD_LOADER_MAGIC, "");
      PUBLIC struct hwvulkan_module_t HAL_MODULE_INFO_SYM = {
      .common = {
      .tag = HARDWARE_MODULE_TAG,
   .module_api_version = HWVULKAN_MODULE_API_VERSION_0_1,
   .hal_api_version = HARDWARE_MAKE_API_VERSION(1, 0),
   .id = HWVULKAN_HARDWARE_MODULE_ID,
   .name = "Intel Vulkan HAL",
   .author = "Intel",
   .methods = &(hw_module_methods_t) {
               };
      /* If any bits in test_mask are set, then unset them and return true. */
   static inline bool
   unmask32(uint32_t *inout_mask, uint32_t test_mask)
   {
      uint32_t orig_mask = *inout_mask;
   *inout_mask &= ~test_mask;
      }
      static int
   anv_hal_open(const struct hw_module_t* mod, const char* id,
         {
      assert(mod == &HAL_MODULE_INFO_SYM.common);
            hwvulkan_device_t *hal_dev = malloc(sizeof(*hal_dev));
   if (!hal_dev)
            *hal_dev = (hwvulkan_device_t) {
      .common = {
      .tag = HARDWARE_DEVICE_TAG,
   .version = HWVULKAN_DEVICE_API_VERSION_0_1,
   .module = &HAL_MODULE_INFO_SYM.common,
         .EnumerateInstanceExtensionProperties = anv_EnumerateInstanceExtensionProperties,
   .CreateInstance = anv_CreateInstance,
   .GetInstanceProcAddr = anv_GetInstanceProcAddr,
            *dev = &hal_dev->common;
      }
      static int
   anv_hal_close(struct hw_device_t *dev)
   {
      /* hwvulkan.h claims that hw_device_t::close() is never called. */
      }
      #if ANDROID_API_LEVEL >= 26
   #include <vndk/hardware_buffer.h>
   /* See i915_private_android_types.h in minigbm. */
   #define HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL 0x100
      enum {
      /* Usage bit equal to GRALLOC_USAGE_HW_CAMERA_MASK */
      };
      inline VkFormat
   vk_format_from_android(unsigned android_format, unsigned android_usage)
   {
      switch (android_format) {
   case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
         case AHARDWAREBUFFER_FORMAT_Y8Cb8Cr8_420:
   case HAL_PIXEL_FORMAT_NV12_Y_TILED_INTEL:
         case AHARDWAREBUFFER_FORMAT_YV12:
         case AHARDWAREBUFFER_FORMAT_IMPLEMENTATION_DEFINED:
      if (android_usage & BUFFER_USAGE_CAMERA_MASK)
         else
      default:
            }
      unsigned
   anv_ahb_format_for_vk_format(VkFormat vk_format)
   {
      switch (vk_format) {
      #ifdef HAVE_CROS_GRALLOC
         #else
         #endif
      default:
            }
      static VkResult
   get_ahw_buffer_format_properties2(
      VkDevice device_h,
   const struct AHardwareBuffer *buffer,
      {
               /* Get a description of buffer contents . */
   AHardwareBuffer_Desc desc;
            /* Verify description. */
   uint64_t gpu_usage =
      AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE |
   AHARDWAREBUFFER_USAGE_GPU_COLOR_OUTPUT |
         /* "Buffer must be a valid Android hardware buffer object with at least
   * one of the AHARDWAREBUFFER_USAGE_GPU_* usage flags."
   */
   if (!(desc.usage & (gpu_usage)))
            /* Fill properties fields based on description. */
            p->format = vk_format_from_android(desc.format, desc.usage);
                     /* Default to OPTIMAL tiling but set to linear in case
   * of AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER usage.
   */
            if (desc.usage & AHARDWAREBUFFER_USAGE_GPU_DATA_BUFFER)
            p->formatFeatures =
      anv_get_image_format_features2(device->physical, p->format, anv_format,
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
   p->formatFeatures |=
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
   anv_GetAndroidHardwareBufferPropertiesANDROID(
      VkDevice device_h,
   const struct AHardwareBuffer *buffer,
      {
               VkAndroidHardwareBufferFormatPropertiesANDROID *format_prop =
      vk_find_struct(pProperties->pNext,
      /* Fill format properties of an Android hardware buffer. */
   if (format_prop) {
      VkAndroidHardwareBufferFormatProperties2ANDROID format_prop2 = {
         };
            format_prop->format                 = format_prop2.format;
   format_prop->externalFormat         = format_prop2.externalFormat;
   format_prop->formatFeatures         =
         format_prop->samplerYcbcrConversionComponents =
         format_prop->suggestedYcbcrModel    = format_prop2.suggestedYcbcrModel;
   format_prop->suggestedYcbcrRange    = format_prop2.suggestedYcbcrRange;
   format_prop->suggestedXChromaOffset = format_prop2.suggestedXChromaOffset;
               VkAndroidHardwareBufferFormatProperties2ANDROID *format_prop2 =
      vk_find_struct(pProperties->pNext,
      if (format_prop2)
            /* NOTE - We support buffers with only one handle but do not error on
   * multiple handle case. Reason is that we want to support YUV formats
   * where we have many logical planes but they all point to the same
   * buffer, like is the case with VK_FORMAT_G8_B8R8_2PLANE_420_UNORM.
   */
   const native_handle_t *handle =
         int dma_buf = (handle && handle->numFds) ? handle->data[0] : -1;
   if (dma_buf < 0)
            /* All memory types. */
            pProperties->allocationSize = lseek(dma_buf, 0, SEEK_END);
               }
      #endif
      /*
   * Called from anv_AllocateMemory when import AHardwareBuffer.
   */
   VkResult
   anv_import_ahw_memory(VkDevice device_h,
         {
   #if ANDROID_API_LEVEL >= 26
               /* Import from AHardwareBuffer to anv_device_memory. */
   const native_handle_t *handle =
            /* NOTE - We support buffers with only one handle but do not error on
   * multiple handle case. Reason is that we want to support YUV formats
   * where we have many logical planes but they all point to the same
   * buffer, like is the case with VK_FORMAT_G8_B8R8_2PLANE_420_UNORM.
   */
   int dma_buf = (handle && handle->numFds) ? handle->data[0] : -1;
   if (dma_buf < 0)
            VkResult result = anv_device_import_bo(device, dma_buf, 0,
                           #else
         #endif
   }
      VkResult
   anv_image_init_from_gralloc(struct anv_device *device,
                     {
      struct anv_bo *bo = NULL;
            struct anv_image_create_info anv_info = {
      .vk_info = base_info,
               /* Do not close the gralloc handle's dma_buf. The lifetime of the dma_buf
   * must exceed that of the gralloc handle, and we do not own the gralloc
   * handle.
   */
            /* We need to set the WRITE flag on window system buffers so that GEM will
   * know we're writing to them and synchronize uses on other rings (for
   * example, if the display server uses the blitter ring).
   *
   * If this function fails and if the imported bo was resident in the cache,
   * we should avoid updating the bo's flags. Therefore, we defer updating
   * the flags until success is certain.
   *
   */
   result = anv_device_import_bo(device, dma_buf,
                                 if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
               enum isl_tiling tiling;
   result = anv_device_get_bo_tiling(device, bo, &tiling);
   if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
      }
                     result = anv_image_init(device, image, &anv_info);
   if (result != VK_SUCCESS)
            VkMemoryRequirements2 mem_reqs = {
                  anv_image_get_memory_requirements(device, image, image->vk.aspects,
            VkDeviceSize aligned_image_size =
      align64(mem_reqs.memoryRequirements.size,
         if (bo->size < aligned_image_size) {
      result = vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                                 assert(!image->disjoint);
   assert(image->n_planes == 1);
   assert(image->planes[0].primary_surface.memory_range.binding ==
         assert(image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.bo == NULL);
   assert(image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.offset == 0);
   image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.bo = bo;
                  fail_size:
         fail_init:
                  }
      VkResult
   anv_image_bind_from_gralloc(struct anv_device *device,
               {
      /* Do not close the gralloc handle's dma_buf. The lifetime of the dma_buf
   * must exceed that of the gralloc handle, and we do not own the gralloc
   * handle.
   */
            /* We need to set the WRITE flag on window system buffers so that GEM will
   * know we're writing to them and synchronize uses on other rings (for
   * example, if the display server uses the blitter ring).
   *
   * If this function fails and if the imported bo was resident in the cache,
   * we should avoid updating the bo's flags. Therefore, we defer updating
   * the flags until success is certain.
   *
   */
   struct anv_bo *bo = NULL;
   VkResult result = anv_device_import_bo(device, dma_buf,
                                 if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
               uint64_t img_size = image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].memory_range.size;
   if (img_size < bo->size) {
      result = vk_errorf(device, VK_ERROR_INVALID_EXTERNAL_HANDLE,
                     anv_device_release_bo(device, bo);
               assert(!image->disjoint);
   assert(image->n_planes == 1);
   assert(image->planes[0].primary_surface.memory_range.binding ==
         assert(image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.bo == NULL);
   assert(image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.offset == 0);
   image->bindings[ANV_IMAGE_MEMORY_BINDING_MAIN].address.bo = bo;
               }
      static VkResult
   format_supported_with_usage(VkDevice device_h, VkFormat format,
         {
      ANV_FROM_HANDLE(anv_device, device, device_h);
   VkPhysicalDevice phys_dev_h = anv_physical_device_to_handle(device->physical);
            const VkPhysicalDeviceImageFormatInfo2 image_format_info = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2,
   .format = format,
   .type = VK_IMAGE_TYPE_2D,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
               VkImageFormatProperties2 image_format_props = {
                  /* Check that requested format and usage are supported. */
   result = anv_GetPhysicalDeviceImageFormatProperties2(phys_dev_h,
         if (result != VK_SUCCESS) {
      return vk_errorf(device, result,
            }
      }
         static VkResult
   setup_gralloc0_usage(struct anv_device *device, VkFormat format,
         {
      /* WARNING: Android's libvulkan.so hardcodes the VkImageUsageFlags
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
            if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                  if (unmask32(&imageUsage, VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                              /* All VkImageUsageFlags not explicitly checked here are unsupported for
   * gralloc swapchains.
   */
   if (imageUsage != 0) {
      return vk_errorf(device, VK_ERROR_FORMAT_NOT_SUPPORTED,
                     /* The below formats support GRALLOC_USAGE_HW_FB (that is, display
   * scanout). This short list of formats is univserally supported on Intel
   * but is incomplete.  The full set of supported formats is dependent on
   * kernel and hardware.
   *
   * FINISHME: Advertise all display-supported formats.
   */
   switch (format) {
      case VK_FORMAT_B8G8R8A8_UNORM:
   case VK_FORMAT_R5G6B5_UNORM_PACK16:
   case VK_FORMAT_R8G8B8A8_UNORM:
   case VK_FORMAT_R8G8B8A8_SRGB:
      *grallocUsage |= GRALLOC_USAGE_HW_FB |
                  default:
               if (*grallocUsage == 0)
               }
      #if ANDROID_API_LEVEL >= 26
   VkResult anv_GetSwapchainGrallocUsage2ANDROID(
      VkDevice            device_h,
   VkFormat            format,
   VkImageUsageFlags   imageUsage,
   VkSwapchainImageUsageFlagsANDROID swapchainImageUsage,
   uint64_t*           grallocConsumerUsage,
      {
      ANV_FROM_HANDLE(anv_device, device, device_h);
            *grallocConsumerUsage = 0;
   *grallocProducerUsage = 0;
   mesa_logd("%s: format=%d, usage=0x%x, swapchainUsage=0x%x", __func__, format,
            result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
            int32_t grallocUsage = 0;
   result = setup_gralloc0_usage(device, format, imageUsage, &grallocUsage);
   if (result != VK_SUCCESS)
                     if (grallocUsage & GRALLOC_USAGE_HW_RENDER) {
      *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
               if (grallocUsage & GRALLOC_USAGE_HW_TEXTURE) {
                  if (grallocUsage & (GRALLOC_USAGE_HW_FB |
                  *grallocProducerUsage |= GRALLOC1_PRODUCER_USAGE_GPU_RENDER_TARGET;
               if ((swapchainImageUsage & VK_SWAPCHAIN_IMAGE_USAGE_SHARED_BIT_ANDROID) &&
      device->u_gralloc != NULL) {
   uint64_t front_rendering_usage = 0;
   u_gralloc_get_front_rendering_usage(device->u_gralloc, &front_rendering_usage);
                  }
   #endif
      VkResult anv_GetSwapchainGrallocUsageANDROID(
      VkDevice            device_h,
   VkFormat            format,
   VkImageUsageFlags   imageUsage,
      {
      ANV_FROM_HANDLE(anv_device, device, device_h);
            *grallocUsage = 0;
            result = format_supported_with_usage(device_h, format, imageUsage);
   if (result != VK_SUCCESS)
               }
