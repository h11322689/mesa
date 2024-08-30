   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_wsi.h"
      #include "drm-uapi/drm_fourcc.h"
   #include "vk_enum_to_str.h"
   #include "wsi_common_entrypoints.h"
      #include "vn_device.h"
   #include "vn_image.h"
   #include "vn_instance.h"
   #include "vn_physical_device.h"
   #include "vn_queue.h"
      /* The common WSI support makes some assumptions about the driver.
   *
   * In wsi_device_init, it assumes VK_EXT_pci_bus_info is available.  In
   * wsi_create_native_image and wsi_create_prime_image, it assumes
   * VK_KHR_external_memory_fd and VK_EXT_external_memory_dma_buf are enabled.
   *
   * In wsi_create_native_image, if wsi_device::supports_modifiers is set and
   * the window system supports modifiers, it assumes
   * VK_EXT_image_drm_format_modifier is enabled.  Otherwise, it assumes that
   * wsi_image_create_info can be chained to VkImageCreateInfo and
   * vkGetImageSubresourceLayout can be called even the tiling is
   * VK_IMAGE_TILING_OPTIMAL.
   *
   * Together, it knows how to share dma-bufs, with explicit or implicit
   * modifiers, to the window system.
   *
   * For venus, we use explicit modifiers when the renderer and the window
   * system support them.  Otherwise, we have to fall back to
   * VK_IMAGE_TILING_LINEAR (or trigger the prime blit path).  But the fallback
   * can be problematic when the memory is scanned out directly and special
   * requirements (e.g., alignments) must be met.
   *
   * The common WSI support makes other assumptions about the driver to support
   * implicit fencing.  In wsi_create_native_image and wsi_create_prime_image,
   * it assumes wsi_memory_allocate_info can be chained to VkMemoryAllocateInfo.
   * In wsi_common_queue_present, it assumes wsi_memory_signal_submit_info can
   * be chained to VkSubmitInfo.  Finally, in wsi_common_acquire_next_image2, it
   * calls wsi_device::signal_semaphore_for_memory, and
   * wsi_device::signal_fence_for_memory if the driver provides them.
   *
   * Some drivers use wsi_memory_allocate_info to set up implicit fencing.
   * Others use wsi_memory_signal_submit_info to set up implicit IN-fences and
   * use wsi_device::signal_*_for_memory to set up implicit OUT-fences.
   *
   * For venus, implicit fencing is broken (and there is no explicit fencing
   * support yet).  The kernel driver assumes everything is in the same fence
   * context and no synchronization is needed.  It should be fixed for
   * correctness, but it is still not ideal.  venus requires explicit fencing
   * (and renderer-side synchronization) to work well.
   */
      /* cast a WSI object to a pointer for logging */
   #define VN_WSI_PTR(obj) ((const void *)(uintptr_t)(obj))
      static PFN_vkVoidFunction
   vn_wsi_proc_addr(VkPhysicalDevice physicalDevice, const char *pName)
   {
      struct vn_physical_device *physical_dev =
         return vk_instance_get_proc_addr_unchecked(
      }
      VkResult
   vn_wsi_init(struct vn_physical_device *physical_dev)
   {
      const VkAllocationCallbacks *alloc =
         VkResult result = wsi_device_init(
      &physical_dev->wsi_device, vn_physical_device_to_handle(physical_dev),
   vn_wsi_proc_addr, alloc, -1, &physical_dev->instance->dri_options,
      if (result != VK_SUCCESS)
            physical_dev->wsi_device.supports_modifiers = true;
               }
      void
   vn_wsi_fini(struct vn_physical_device *physical_dev)
   {
      const VkAllocationCallbacks *alloc =
         physical_dev->base.base.wsi_device = NULL;
      }
      VkResult
   vn_wsi_create_image(struct vn_device *dev,
                     const VkImageCreateInfo *create_info,
   {
      /* TODO This is the legacy path used by wsi_create_native_image when there
   * is no modifier support.  Instead of forcing linear tiling, we should ask
   * wsi to use wsi_create_prime_image instead.
   *
   * In fact, this is not enough when the image is truely used for scanout by
   * the host compositor.  There can be requirements we fail to meet.  We
   * should require modifier support at some point.
   */
   const uint64_t modifier = DRM_FORMAT_MOD_LINEAR;
   const VkImageDrmFormatModifierListCreateInfoEXT mod_list_info = {
      .sType =
         .pNext = create_info->pNext,
   .drmFormatModifierCount = 1,
      };
   VkImageCreateInfo local_create_info;
   if (wsi_info->scanout) {
      assert(!vk_find_struct_const(
            local_create_info = *create_info;
   local_create_info.pNext = &mod_list_info;
   local_create_info.tiling = VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT;
            if (VN_DEBUG(WSI))
               struct vn_image *img;
   VkResult result = vn_image_create(dev, create_info, alloc, &img);
   if (result != VK_SUCCESS)
            img->wsi.is_wsi = true;
   img->wsi.is_prime_blit_src = wsi_info->blit_src;
            if (create_info->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      VkDevice dev_handle = vn_device_to_handle(dev);
            VkImageDrmFormatModifierPropertiesEXT props = {
         };
   result = vn_GetImageDrmFormatModifierPropertiesEXT(dev_handle,
         if (result != VK_SUCCESS) {
      vn_DestroyImage(dev_handle, img_handle, alloc);
                           *out_img = img;
      }
      VkResult
   vn_wsi_create_image_from_swapchain(
      struct vn_device *dev,
   const VkImageCreateInfo *create_info,
   const VkImageSwapchainCreateInfoKHR *swapchain_info,
   const VkAllocationCallbacks *alloc,
      {
      const struct vn_image *swapchain_img = vn_image_from_handle(
                  /* must match what the common WSI and vn_wsi_create_image do */
            /* match external memory */
   const VkExternalMemoryImageCreateInfo local_external_info = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
   .pNext = local_create_info.pNext,
   .handleTypes =
      };
            /* match image tiling */
            VkImageDrmFormatModifierListCreateInfoEXT local_mod_info;
   if (local_create_info.tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      local_mod_info = (const VkImageDrmFormatModifierListCreateInfoEXT){
      .sType =
         .pNext = local_create_info.pNext,
   .drmFormatModifierCount = 1,
      };
               /* match image usage */
   if (swapchain_img->wsi.is_prime_blit_src)
                     struct vn_image *img;
   VkResult result = vn_image_create(dev, create_info, alloc, &img);
   if (result != VK_SUCCESS)
            img->wsi.is_wsi = true;
   img->wsi.tiling_override = swapchain_img->wsi.tiling_override;
            *out_img = img;
      }
      /* swapchain commands */
      VkResult
   vn_CreateSwapchainKHR(VkDevice device,
                     {
               VkResult result =
         if (VN_DEBUG(WSI) && result == VK_SUCCESS) {
      vn_log(dev->instance,
         "swapchain %p: created with surface %p, min count %d, size "
   "%dx%d, mode %s, old %p",
   VN_WSI_PTR(*pSwapchain), VN_WSI_PTR(pCreateInfo->surface),
   pCreateInfo->minImageCount, pCreateInfo->imageExtent.width,
   pCreateInfo->imageExtent.height,
                  }
      void
   vn_DestroySwapchainKHR(VkDevice device,
               {
               wsi_DestroySwapchainKHR(device, swapchain, pAllocator);
   if (VN_DEBUG(WSI))
      }
      VkResult
   vn_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo)
   {
      VN_TRACE_FUNC();
   struct vk_queue *queue_vk = vk_queue_from_handle(_queue);
            VkResult result = wsi_common_queue_present(
      &dev->physical_device->wsi_device, vn_device_to_handle(dev), _queue,
      if (VN_DEBUG(WSI) && result != VK_SUCCESS) {
      for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
      const VkResult r =
         vn_log(dev->instance, "swapchain %p: presented image %d: %s",
                           }
      VkResult
   vn_AcquireNextImage2KHR(VkDevice device,
               {
      VN_TRACE_FUNC();
            VkResult result = wsi_common_acquire_next_image2(
         if (VN_DEBUG(WSI) && result != VK_SUCCESS) {
      const int idx = result >= VK_SUCCESS ? *pImageIndex : -1;
   vn_log(dev->instance, "swapchain %p: acquired image %d: %s",
                     /* XXX this relies on implicit sync */
   if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
      struct vn_semaphore *sem =
         if (sem)
            struct vn_fence *fence = vn_fence_from_handle(pAcquireInfo->fence);
   if (fence)
                  }
