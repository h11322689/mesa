   /*
   * Copyright 2021 Red Hat, Inc.
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
      /** VK_EXT_headless_surface */
      #include "util/macros.h"
   #include "util/hash_table.h"
   #include "util/timespec.h"
   #include "util/u_thread.h"
   #include "util/xmlconfig.h"
   #include "vk_util.h"
   #include "vk_enum_to_str.h"
   #include "vk_instance.h"
   #include "vk_physical_device.h"
   #include "wsi_common_entrypoints.h"
   #include "wsi_common_private.h"
   #include "wsi_common_queue.h"
      #include "drm-uapi/drm_fourcc.h"
      struct wsi_headless_format {
      VkFormat        format;
      };
      struct wsi_headless {
                        const VkAllocationCallbacks *alloc;
      };
      static VkResult
   wsi_headless_surface_get_support(VkIcdSurfaceBase *surface,
                     {
                  }
      static const VkPresentModeKHR present_modes[] = {
      VK_PRESENT_MODE_MAILBOX_KHR,
      };
      static VkResult
   wsi_headless_surface_get_capabilities(VkIcdSurfaceBase *surface,
               {
      /* For true mailbox mode, we need at least 4 images:
   *  1) One to scan out from
   *  2) One to have queued for scan-out
   *  3) One to be currently held by the Wayland compositor
   *  4) One to render to
   */
   caps->minImageCount = 4;
   /* There is no real maximum */
            caps->currentExtent = (VkExtent2D) { -1, -1 };
   caps->minImageExtent = (VkExtent2D) { 1, 1 };
   caps->maxImageExtent = (VkExtent2D) {
      wsi_device->maxImageDimension2D,
               caps->supportedTransforms = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
   caps->currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
            caps->supportedCompositeAlpha =
      VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR |
         caps->supportedUsageFlags =
      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
   VK_IMAGE_USAGE_SAMPLED_BIT |
   VK_IMAGE_USAGE_TRANSFER_DST_BIT |
   VK_IMAGE_USAGE_STORAGE_BIT |
         VK_FROM_HANDLE(vk_physical_device, pdevice, wsi_device->pdevice);
   if (pdevice->supported_extensions.EXT_attachment_feedback_loop_layout)
               }
      static VkResult
   wsi_headless_surface_get_capabilities2(VkIcdSurfaceBase *surface,
                     {
               VkResult result =
      wsi_headless_surface_get_capabilities(surface, wsi_device,
         vk_foreach_struct(ext, caps->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR: {
      VkSurfaceProtectedCapabilitiesKHR *protected = (void *)ext;
   protected->supportsProtected = VK_FALSE;
               default:
      /* Ignored */
                     }
      static VkResult
   wsi_headless_surface_get_formats(VkIcdSurfaceBase *icd_surface,
                     {
      struct wsi_headless *wsi =
                     if (wsi->wsi->force_bgra8_unorm_first) {
      vk_outarray_append_typed(VkSurfaceFormatKHR, &out, out_fmt) {
      out_fmt->format = VK_FORMAT_B8G8R8A8_UNORM;
      }
   vk_outarray_append_typed(VkSurfaceFormatKHR, &out, out_fmt) {
      out_fmt->format = VK_FORMAT_R8G8B8A8_UNORM;
         } else {
      vk_outarray_append_typed(VkSurfaceFormatKHR, &out, out_fmt) {
      out_fmt->format = VK_FORMAT_R8G8B8A8_UNORM;
      }
   vk_outarray_append_typed(VkSurfaceFormatKHR, &out, out_fmt) {
      out_fmt->format = VK_FORMAT_B8G8R8A8_UNORM;
                     }
      static VkResult
   wsi_headless_surface_get_formats2(VkIcdSurfaceBase *icd_surface,
                           {
      struct wsi_headless *wsi =
                     if (wsi->wsi->force_bgra8_unorm_first) {
      vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, out_fmt) {
      out_fmt->surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
      }
   vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, out_fmt) {
      out_fmt->surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
         } else {
      vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, out_fmt) {
      out_fmt->surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
      }
   vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, out_fmt) {
      out_fmt->surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
                     }
      static VkResult
   wsi_headless_surface_get_present_modes(VkIcdSurfaceBase *surface,
                     {
      if (pPresentModes == NULL) {
      *pPresentModeCount = ARRAY_SIZE(present_modes);
               *pPresentModeCount = MIN2(*pPresentModeCount, ARRAY_SIZE(present_modes));
            if (*pPresentModeCount < ARRAY_SIZE(present_modes))
         else
      }
      static VkResult
   wsi_headless_surface_get_present_rectangles(VkIcdSurfaceBase *surface,
                     {
               vk_outarray_append_typed(VkRect2D, &out, rect) {
      /* We don't know a size so just return the usual "I don't know." */
   *rect = (VkRect2D) {
      .offset = { 0, 0 },
                     }
      struct wsi_headless_image {
      struct wsi_image                             base;
      };
      struct wsi_headless_swapchain {
               VkExtent2D                                  extent;
                     VkPresentModeKHR                            present_mode;
               };
   VK_DEFINE_NONDISP_HANDLE_CASTS(wsi_headless_swapchain, base.base, VkSwapchainKHR,
            static struct wsi_image *
   wsi_headless_swapchain_get_wsi_image(struct wsi_swapchain *wsi_chain,
         {
      struct wsi_headless_swapchain *chain =
            }
      static VkResult
   wsi_headless_swapchain_acquire_next_image(struct wsi_swapchain *wsi_chain,
               {
      struct wsi_headless_swapchain *chain =
         struct timespec start_time, end_time;
                     clock_gettime(CLOCK_MONOTONIC, &start_time);
            while (1) {
      /* Try to find a free image. */
   for (uint32_t i = 0; i < chain->base.image_count; i++) {
      if (!chain->images[i].busy) {
      /* We found a non-busy image */
   *image_index = i;
   chain->images[i].busy = true;
                  /* Check for timeout. */
   struct timespec current_time;
   clock_gettime(CLOCK_MONOTONIC, &current_time);
   if (timespec_after(&current_time, &end_time))
         }
      static VkResult
   wsi_headless_swapchain_queue_present(struct wsi_swapchain *wsi_chain,
                     {
      struct wsi_headless_swapchain *chain =
                                 }
      static VkResult
   wsi_headless_swapchain_destroy(struct wsi_swapchain *wsi_chain,
         {
      struct wsi_headless_swapchain *chain =
            for (uint32_t i = 0; i < chain->base.image_count; i++) {
      if (chain->images[i].base.image != VK_NULL_HANDLE)
                                             }
      static const struct VkDrmFormatModifierPropertiesEXT *
   get_modifier_props(const struct wsi_image_info *info, uint64_t modifier)
   {
      for (uint32_t i = 0; i < info->modifier_prop_count; i++) {
      if (info->modifier_props[i].drmFormatModifier == modifier)
      }
      }
      static VkResult
   wsi_create_null_image_mem(const struct wsi_swapchain *chain,
               {
      const struct wsi_device *wsi = chain->wsi;
            VkMemoryRequirements reqs;
            const VkMemoryDedicatedAllocateInfo memory_dedicated_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   .pNext = NULL,
   .image = image->image,
      };
   const VkMemoryAllocateInfo memory_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &memory_dedicated_info,
   .allocationSize = reqs.size,
   .memoryTypeIndex =
      };
   result = wsi->AllocateMemory(chain->device, &memory_info,
         if (result != VK_SUCCESS)
                     if (info->drm_mod_list.drmFormatModifierCount > 0) {
      VkImageDrmFormatModifierPropertiesEXT image_mod_props = {
         };
   result = wsi->GetImageDrmFormatModifierPropertiesEXT(chain->device,
               if (result != VK_SUCCESS)
            image->drm_modifier = image_mod_props.drmFormatModifier;
            const struct VkDrmFormatModifierPropertiesEXT *mod_props =
                  for (uint32_t p = 0; p < image->num_planes; p++) {
      const VkImageSubresource image_subresource = {
      .aspectMask = VK_IMAGE_ASPECT_PLANE_0_BIT << p,
   .mipLevel = 0,
      };
   VkSubresourceLayout image_layout;
   wsi->GetImageSubresourceLayout(chain->device, image->image,
         image->sizes[p] = image_layout.size;
   image->row_pitches[p] = image_layout.rowPitch;
         } else {
      const VkImageSubresource image_subresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = 0,
      };
   VkSubresourceLayout image_layout;
   wsi->GetImageSubresourceLayout(chain->device, image->image,
            image->drm_modifier = DRM_FORMAT_MOD_INVALID;
   image->num_planes = 1;
   image->sizes[0] = reqs.size;
   image->row_pitches[0] = image_layout.rowPitch;
                  }
      static VkResult
   wsi_headless_surface_create_swapchain(VkIcdSurfaceBase *icd_surface,
                                 {
      struct wsi_headless_swapchain *chain;
                              size_t size = sizeof(*chain) + num_images * sizeof(chain->images[0]);
   chain = vk_zalloc(pAllocator, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (chain == NULL)
            struct wsi_drm_image_params drm_params = {
      .base.image_type = WSI_IMAGE_TYPE_DRM,
               result = wsi_swapchain_init(wsi_device, &chain->base, device,
         if (result != VK_SUCCESS) {
      vk_free(pAllocator, chain);
               chain->base.destroy = wsi_headless_swapchain_destroy;
   chain->base.get_wsi_image = wsi_headless_swapchain_get_wsi_image;
   chain->base.acquire_next_image = wsi_headless_swapchain_acquire_next_image;
   chain->base.queue_present = wsi_headless_swapchain_queue_present;
   chain->base.present_mode = wsi_swapchain_get_present_mode(wsi_device, pCreateInfo);
   chain->base.image_count = num_images;
   chain->extent = pCreateInfo->imageExtent;
            result = wsi_configure_image(&chain->base, pCreateInfo,
         if (result != VK_SUCCESS) {
         }
               for (uint32_t i = 0; i < chain->base.image_count; i++) {
      result = wsi_create_image(&chain->base, &chain->base.image_info,
         if (result != VK_SUCCESS)
                                       fail:
                  }
      VkResult
   wsi_headless_init_wsi(struct wsi_device *wsi_device,
               {
      struct wsi_headless *wsi;
            wsi = vk_alloc(alloc, sizeof(*wsi), 8,
         if (!wsi) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               wsi->physical_device = physical_device;
   wsi->alloc = alloc;
            wsi->base.get_support = wsi_headless_surface_get_support;
   wsi->base.get_capabilities2 = wsi_headless_surface_get_capabilities2;
   wsi->base.get_formats = wsi_headless_surface_get_formats;
   wsi->base.get_formats2 = wsi_headless_surface_get_formats2;
   wsi->base.get_present_modes = wsi_headless_surface_get_present_modes;
   wsi->base.get_present_rectangles = wsi_headless_surface_get_present_rectangles;
                           fail:
                  }
      void
   wsi_headless_finish_wsi(struct wsi_device *wsi_device,
         {
      struct wsi_headless *wsi =
         if (!wsi)
               }
      VkResult wsi_CreateHeadlessSurfaceEXT(
      VkInstance                                  _instance,
   const VkHeadlessSurfaceCreateInfoEXT*       pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
            surface = vk_alloc2(&instance->alloc, pAllocator, sizeof *surface, 8,
         if (surface == NULL)
                     *pSurface = VkIcdSurfaceBase_to_handle(&surface->base);
      }
