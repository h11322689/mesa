   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_image.h"
      #include "venus-protocol/vn_protocol_driver_image.h"
   #include "venus-protocol/vn_protocol_driver_image_view.h"
   #include "venus-protocol/vn_protocol_driver_sampler.h"
   #include "venus-protocol/vn_protocol_driver_sampler_ycbcr_conversion.h"
      #include "vn_android.h"
   #include "vn_device.h"
   #include "vn_device_memory.h"
   #include "vn_physical_device.h"
   #include "vn_wsi.h"
      /* image commands */
      static void
   vn_image_init_memory_requirements(struct vn_image *img,
               {
      uint32_t plane_count = 1;
   if (create_info->flags & VK_IMAGE_CREATE_DISJOINT_BIT) {
      /* TODO VkDrmFormatModifierPropertiesEXT::drmFormatModifierPlaneCount */
            switch (create_info->format) {
   case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
   case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
   case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
   case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
      plane_count = 2;
      case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
   case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
   case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
   case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
   case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
   case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
   case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
      plane_count = 3;
      default:
      plane_count = 1;
         }
            /* TODO add a per-device cache for the requirements */
   for (uint32_t i = 0; i < plane_count; i++) {
      img->requirements[i].memory.sType =
         img->requirements[i].memory.pNext = &img->requirements[i].dedicated;
   img->requirements[i].dedicated.sType =
                     VkDevice dev_handle = vn_device_to_handle(dev);
   VkImage img_handle = vn_image_to_handle(img);
   if (plane_count == 1) {
      vn_call_vkGetImageMemoryRequirements2(
      dev->instance, dev_handle,
   &(VkImageMemoryRequirementsInfo2){
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
                  /* AHB backed image requires dedicated allocation */
   if (img->deferred_info) {
      img->requirements[0].dedicated.prefersDedicatedAllocation = VK_TRUE;
         } else {
      for (uint32_t i = 0; i < plane_count; i++) {
      vn_call_vkGetImageMemoryRequirements2(
      dev->instance, dev_handle,
   &(VkImageMemoryRequirementsInfo2){
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
   .pNext =
      &(VkImagePlaneMemoryRequirementsInfo){
      .sType =
                  },
         }
      static VkResult
   vn_image_deferred_info_init(struct vn_image *img,
               {
      struct vn_image_create_deferred_info *info = NULL;
            info = vk_zalloc(alloc, sizeof(*info), VN_DEFAULT_ALIGN,
         if (!info)
            info->create = *create_info;
            vk_foreach_struct_const(src, create_info->pNext) {
      void *pnext = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO: {
      /* 12.3. Images
   *
   * If viewFormatCount is zero, pViewFormats is ignored and the image
   * is created as if the VkImageFormatListCreateInfo structure were
   * not included in the pNext chain of VkImageCreateInfo.
   */
                                 /* need a deep copy for view formats array */
   const size_t size = sizeof(VkFormat) * info->list.viewFormatCount;
   VkFormat *view_formats = vk_zalloc(
         if (!view_formats) {
      vk_free(alloc, info);
               memcpy(view_formats,
         ((const VkImageFormatListCreateInfo *)src)->pViewFormats,
      } break;
   case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
      memcpy(&info->stencil, src, sizeof(info->stencil));
   pnext = &info->stencil;
      case VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID: {
      const uint32_t drm_format =
         if (drm_format) {
      info->create.format =
               } break;
   default:
                  if (pnext) {
      dst->pNext = pnext;
         }
                        }
      static void
   vn_image_deferred_info_fini(struct vn_image *img,
         {
      if (!img->deferred_info)
            if (img->deferred_info->list.pViewFormats)
               }
      static VkResult
   vn_image_init(struct vn_device *dev,
               {
      VkDevice device = vn_device_to_handle(dev);
   VkImage image = vn_image_to_handle(img);
                     /* TODO async */
   result =
         if (result != VK_SUCCESS)
                        }
      VkResult
   vn_image_create(struct vn_device *dev,
                     {
      struct vn_image *img = NULL;
            img = vk_zalloc(alloc, sizeof(*img), VN_DEFAULT_ALIGN,
         if (!img)
                     result = vn_image_init(dev, create_info, img);
   if (result != VK_SUCCESS) {
      vn_object_base_fini(&img->base);
   vk_free(alloc, img);
                           }
      VkResult
   vn_image_init_deferred(struct vn_device *dev,
               {
      VkResult result = vn_image_init(dev, create_info, img);
   img->deferred_info->initialized = result == VK_SUCCESS;
      }
      static VkResult
   vn_image_create_deferred(struct vn_device *dev,
                     {
      struct vn_image *img = NULL;
            img = vk_zalloc(alloc, sizeof(*img), VN_DEFAULT_ALIGN,
         if (!img)
                     result = vn_image_deferred_info_init(img, create_info, alloc);
   if (result != VK_SUCCESS) {
      vn_object_base_fini(&img->base);
   vk_free(alloc, img);
                           }
      struct vn_image_create_info {
      VkImageCreateInfo create;
   VkExternalMemoryImageCreateInfo external;
   VkImageFormatListCreateInfo format_list;
   VkImageStencilUsageCreateInfo stencil;
   VkImageDrmFormatModifierListCreateInfoEXT modifier_list;
      };
      static const VkImageCreateInfo *
   vn_image_fix_create_info(
      const VkImageCreateInfo *create_info,
   const VkExternalMemoryHandleTypeFlagBits renderer_handle_type,
      {
      local_info->create = *create_info;
            vk_foreach_struct_const(src, create_info->pNext) {
      void *next = NULL;
   switch (src->sType) {
   case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO:
      memcpy(&local_info->external, src, sizeof(local_info->external));
   local_info->external.handleTypes = renderer_handle_type;
   next = &local_info->external;
      case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO:
      memcpy(&local_info->format_list, src,
         next = &local_info->format_list;
      case VK_STRUCTURE_TYPE_IMAGE_STENCIL_USAGE_CREATE_INFO:
      memcpy(&local_info->stencil, src, sizeof(local_info->stencil));
   next = &local_info->stencil;
      case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT:
      memcpy(&local_info->modifier_list, src,
         next = &local_info->modifier_list;
      case VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_EXPLICIT_CREATE_INFO_EXT:
      memcpy(&local_info->modifier_explicit, src,
         next = &local_info->modifier_explicit;
      default:
                  if (next) {
      cur->pNext = next;
                              }
      VkResult
   vn_CreateImage(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
         const VkExternalMemoryHandleTypeFlagBits renderer_handle_type =
         struct vn_image *img;
            const struct wsi_image_create_info *wsi_info = NULL;
   const VkNativeBufferANDROID *anb_info = NULL;
   const VkImageSwapchainCreateInfoKHR *swapchain_info = NULL;
   const VkExternalMemoryImageCreateInfo *external_info = NULL;
            vk_foreach_struct_const(pnext, pCreateInfo->pNext) {
      switch ((uint32_t)pnext->sType) {
   case VK_STRUCTURE_TYPE_WSI_IMAGE_CREATE_INFO_MESA:
      wsi_info = (void *)pnext;
      case VK_STRUCTURE_TYPE_NATIVE_BUFFER_ANDROID:
      anb_info = (void *)pnext;
      case VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR:
      swapchain_info = (void *)pnext;
   if (!swapchain_info->swapchain)
            case VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO:
      external_info = (void *)pnext;
   if (!external_info->handleTypes)
         else if (
      external_info->handleTypes ==
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID)
         default:
                     /* No need to fix external handle type for:
   * - common wsi image: dma_buf is hard-coded in wsi_configure_native_image
   * - common wsi image alias: it aligns with wsi_info on external handle
   * - Android wsi image: VK_ANDROID_native_buffer involves no external info
   * - AHB external image: deferred creation reconstructs external info
   *
   * Must fix the external handle type for:
   * - non-AHB external image requesting handle types different from renderer
   *
   * Will have to fix more when renderer handle type is no longer dma_buf.
   */
   if (wsi_info) {
      assert(external_info->handleTypes == renderer_handle_type);
      } else if (anb_info) {
      result =
      } else if (ahb_info) {
         } else if (swapchain_info) {
      result = vn_wsi_create_image_from_swapchain(
      } else {
      struct vn_image_create_info local_info;
   if (external_info &&
      external_info->handleTypes != renderer_handle_type) {
   pCreateInfo = vn_image_fix_create_info(
                           if (result != VK_SUCCESS)
            *pImage = vn_image_to_handle(img);
      }
      void
   vn_DestroyImage(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_image *img = vn_image_from_handle(image);
   const VkAllocationCallbacks *alloc =
            if (!img)
            if (img->wsi.memory && img->wsi.memory_owned) {
      VkDeviceMemory mem_handle = vn_device_memory_to_handle(img->wsi.memory);
               /* must not ask renderer to destroy uninitialized deferred image */
   if (!img->deferred_info || img->deferred_info->initialized)
                     vn_object_base_fini(&img->base);
      }
      void
   vn_GetImageMemoryRequirements2(VkDevice device,
               {
      const struct vn_image *img = vn_image_from_handle(pInfo->image);
   union {
      VkBaseOutStructure *pnext;
   VkMemoryRequirements2 *two;
               uint32_t plane = 0;
   const VkImagePlaneMemoryRequirementsInfo *plane_info =
      vk_find_struct_const(pInfo->pNext,
      if (plane_info) {
      switch (plane_info->planeAspect) {
   case VK_IMAGE_ASPECT_PLANE_1_BIT:
      plane = 1;
      case VK_IMAGE_ASPECT_PLANE_2_BIT:
      plane = 2;
      default:
      plane = 0;
                  while (u.pnext) {
      switch (u.pnext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2:
      u.two->memoryRequirements =
            case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS:
      u.dedicated->prefersDedicatedAllocation =
         u.dedicated->requiresDedicatedAllocation =
            default:
         }
         }
      void
   vn_GetImageSparseMemoryRequirements2(
      VkDevice device,
   const VkImageSparseMemoryRequirementsInfo2 *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
               /* see vn_GetPhysicalDeviceSparseImageFormatProperties2 */
   if (dev->physical_device->sparse_binding_disabled) {
      *pSparseMemoryRequirementCount = 0;
               /* TODO local or per-device cache */
   vn_call_vkGetImageSparseMemoryRequirements2(dev->instance, device, pInfo,
            }
      static void
   vn_image_bind_wsi_memory(struct vn_image *img, struct vn_device_memory *mem)
   {
      assert(img->wsi.is_wsi && !img->wsi.memory);
      }
      VkResult
   vn_BindImageMemory2(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
            VkBindImageMemoryInfo *local_infos = NULL;
   for (uint32_t i = 0; i < bindInfoCount; i++) {
      const VkBindImageMemoryInfo *info = &pBindInfos[i];
   struct vn_image *img = vn_image_from_handle(info->image);
   struct vn_device_memory *mem =
            /* no bind info fixup needed */
   if (mem && !mem->base_memory) {
      if (img->wsi.is_wsi)
                     #ifdef ANDROID
            /* TODO handle VkNativeBufferANDROID when we bump up
   * VN_ANDROID_NATIVE_BUFFER_SPEC_VERSION
      #else
            const VkBindImageMemorySwapchainInfoKHR *swapchain_info =
      vk_find_struct_const(info->pNext,
               struct vn_image *swapchain_img =
      vn_image_from_handle(wsi_common_get_image(
   #endif
                  if (img->wsi.is_wsi)
            if (!local_infos) {
      const size_t size = sizeof(*local_infos) * bindInfoCount;
   local_infos = vk_alloc(alloc, size, VN_DEFAULT_ALIGN,
                                    /* If mem is suballocated, mem->base_memory is non-NULL and we must
   * patch it in.  If VkBindImageMemorySwapchainInfoKHR is given, we've
   * looked mem up above and also need to patch it in.
   */
   local_infos[i].memory = vn_device_memory_to_handle(
            }
   if (local_infos)
            vn_async_vkBindImageMemory2(dev->instance, device, bindInfoCount,
                        }
      VkResult
   vn_GetImageDrmFormatModifierPropertiesEXT(
      VkDevice device,
   VkImage image,
      {
               /* TODO local cache */
   return vn_call_vkGetImageDrmFormatModifierPropertiesEXT(
      }
      void
   vn_GetImageSubresourceLayout(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
            /* override aspect mask for wsi/ahb images with tiling modifier */
   VkImageSubresource local_subresource;
   if ((img->wsi.is_wsi && img->wsi.tiling_override ==
            img->deferred_info) {
   VkImageAspectFlags aspect = pSubresource->aspectMask;
   switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT:
   case VK_IMAGE_ASPECT_DEPTH_BIT:
   case VK_IMAGE_ASPECT_STENCIL_BIT:
   case VK_IMAGE_ASPECT_PLANE_0_BIT:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_0_BIT_EXT;
      case VK_IMAGE_ASPECT_PLANE_1_BIT:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_1_BIT_EXT;
      case VK_IMAGE_ASPECT_PLANE_2_BIT:
      aspect = VK_IMAGE_ASPECT_MEMORY_PLANE_2_BIT_EXT;
      default:
                  /* only handle supported aspect override */
   if (aspect != pSubresource->aspectMask) {
      local_subresource = *pSubresource;
   local_subresource.aspectMask = aspect;
                  /* TODO local cache */
   vn_call_vkGetImageSubresourceLayout(dev->instance, device, image,
      }
      /* image view commands */
      VkResult
   vn_CreateImageView(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_image *img = vn_image_from_handle(pCreateInfo->image);
   const VkAllocationCallbacks *alloc =
            VkImageViewCreateInfo local_info;
   if (img->deferred_info && img->deferred_info->from_external_format) {
               local_info = *pCreateInfo;
   local_info.format = img->deferred_info->create.format;
                        struct vn_image_view *view =
      vk_zalloc(alloc, sizeof(*view), VN_DEFAULT_ALIGN,
      if (!view)
            vn_object_base_init(&view->base, VK_OBJECT_TYPE_IMAGE_VIEW, &dev->base);
            VkImageView view_handle = vn_image_view_to_handle(view);
   vn_async_vkCreateImageView(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyImageView(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_image_view *view = vn_image_view_from_handle(imageView);
   const VkAllocationCallbacks *alloc =
            if (!view)
                     vn_object_base_fini(&view->base);
      }
      /* sampler commands */
      VkResult
   vn_CreateSampler(VkDevice device,
                     {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_sampler *sampler =
      vk_zalloc(alloc, sizeof(*sampler), VN_DEFAULT_ALIGN,
      if (!sampler)
                     VkSampler sampler_handle = vn_sampler_to_handle(sampler);
   vn_async_vkCreateSampler(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroySampler(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_sampler *sampler = vn_sampler_from_handle(_sampler);
   const VkAllocationCallbacks *alloc =
            if (!sampler)
                     vn_object_base_fini(&sampler->base);
      }
      /* sampler YCbCr conversion commands */
      VkResult
   vn_CreateSamplerYcbcrConversion(
      VkDevice device,
   const VkSamplerYcbcrConversionCreateInfo *pCreateInfo,
   const VkAllocationCallbacks *pAllocator,
      {
      struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
         const VkExternalFormatANDROID *ext_info =
            VkSamplerYcbcrConversionCreateInfo local_info;
   if (ext_info && ext_info->externalFormat) {
               local_info = *pCreateInfo;
   local_info.format =
         local_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
   local_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
   local_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
   local_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
                        struct vn_sampler_ycbcr_conversion *conv =
      vk_zalloc(alloc, sizeof(*conv), VN_DEFAULT_ALIGN,
      if (!conv)
            vn_object_base_init(&conv->base, VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION,
            VkSamplerYcbcrConversion conv_handle =
         vn_async_vkCreateSamplerYcbcrConversion(dev->instance, device, pCreateInfo,
                        }
      void
   vn_DestroySamplerYcbcrConversion(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_sampler_ycbcr_conversion *conv =
         const VkAllocationCallbacks *alloc =
            if (!conv)
            vn_async_vkDestroySamplerYcbcrConversion(dev->instance, device,
            vn_object_base_fini(&conv->base);
      }
      void
   vn_GetDeviceImageMemoryRequirements(
      VkDevice device,
   const VkDeviceImageMemoryRequirements *pInfo,
      {
               /* TODO per-device cache */
   vn_call_vkGetDeviceImageMemoryRequirements(dev->instance, device, pInfo,
      }
      void
   vn_GetDeviceImageSparseMemoryRequirements(
      VkDevice device,
   const VkDeviceImageMemoryRequirements *pInfo,
   uint32_t *pSparseMemoryRequirementCount,
      {
               /* see vn_GetPhysicalDeviceSparseImageFormatProperties2 */
   if (dev->physical_device->sparse_binding_disabled) {
      *pSparseMemoryRequirementCount = 0;
               /* TODO per-device cache */
   vn_call_vkGetDeviceImageSparseMemoryRequirements(
      dev->instance, device, pInfo, pSparseMemoryRequirementCount,
   }
