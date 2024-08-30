   /*
   * Copyright © 2017 Intel Corporation
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
      #include "wsi_common_private.h"
   #include "wsi_common_entrypoints.h"
   #include "util/u_debug.h"
   #include "util/macros.h"
   #include "util/os_file.h"
   #include "util/os_time.h"
   #include "util/xmlconfig.h"
   #include "vk_device.h"
   #include "vk_fence.h"
   #include "vk_format.h"
   #include "vk_instance.h"
   #include "vk_physical_device.h"
   #include "vk_queue.h"
   #include "vk_semaphore.h"
   #include "vk_sync.h"
   #include "vk_sync_dummy.h"
   #include "vk_util.h"
      #include <time.h>
   #include <stdlib.h>
   #include <stdio.h>
      #ifndef _WIN32
   #include <unistd.h>
   #endif
      uint64_t WSI_DEBUG;
      static const struct debug_control debug_control[] = {
      { "buffer",       WSI_DEBUG_BUFFER },
   { "sw",           WSI_DEBUG_SW },
   { "noshm",        WSI_DEBUG_NOSHM },
   { "linear",       WSI_DEBUG_LINEAR },
   { "dxgi",         WSI_DEBUG_DXGI },
      };
      VkResult
   wsi_device_init(struct wsi_device *wsi,
                  VkPhysicalDevice pdevice,
   WSI_FN_GetPhysicalDeviceProcAddr proc_addr,
   const VkAllocationCallbacks *alloc,
      {
      const char *present_mode;
                                       wsi->instance_alloc = *alloc;
   wsi->pdevice = pdevice;
   wsi->supports_scanout = true;
   wsi->sw = device_options->sw_device || (WSI_DEBUG & WSI_DEBUG_SW);
   wsi->wants_linear = (WSI_DEBUG & WSI_DEBUG_LINEAR) != 0;
      #define WSI_GET_CB(func) \
      PFN_vk##func func = (PFN_vk##func)proc_addr(pdevice, "vk" #func)
   WSI_GET_CB(GetPhysicalDeviceExternalSemaphoreProperties);
   WSI_GET_CB(GetPhysicalDeviceProperties2);
   WSI_GET_CB(GetPhysicalDeviceMemoryProperties);
      #undef WSI_GET_CB
         wsi->drm_info.sType =
         wsi->pci_bus_info.sType =
         wsi->pci_bus_info.pNext = &wsi->drm_info;
   VkPhysicalDeviceProperties2 pdp2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      };
            wsi->maxImageDimension2D = pdp2.properties.limits.maxImageDimension2D;
   assert(pdp2.properties.limits.optimalBufferCopyRowPitchAlignment <= UINT32_MAX);
   wsi->optimalBufferCopyRowPitchAlignment =
                  GetPhysicalDeviceMemoryProperties(pdevice, &wsi->memory_props);
            for (VkExternalSemaphoreHandleTypeFlags handle_type = 1;
      handle_type <= VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT;
   handle_type <<= 1) {
   const VkPhysicalDeviceExternalSemaphoreInfo esi = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
      };
   VkExternalSemaphoreProperties esp = {
         };
            if (esp.externalSemaphoreFeatures &
      VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT)
            const struct vk_device_extension_table *supported_extensions =
         wsi->has_import_memory_host =
         wsi->khr_present_wait =
      supported_extensions->KHR_present_id &&
         /* We cannot expose KHR_present_wait without timeline semaphores. */
                  #define WSI_GET_CB(func) \
      wsi->func = (PFN_vk##func)proc_addr(pdevice, "vk" #func)
   WSI_GET_CB(AllocateMemory);
   WSI_GET_CB(AllocateCommandBuffers);
   WSI_GET_CB(BindBufferMemory);
   WSI_GET_CB(BindImageMemory);
   WSI_GET_CB(BeginCommandBuffer);
   WSI_GET_CB(CmdPipelineBarrier);
   WSI_GET_CB(CmdCopyImage);
   WSI_GET_CB(CmdCopyImageToBuffer);
   WSI_GET_CB(CreateBuffer);
   WSI_GET_CB(CreateCommandPool);
   WSI_GET_CB(CreateFence);
   WSI_GET_CB(CreateImage);
   WSI_GET_CB(CreateSemaphore);
   WSI_GET_CB(DestroyBuffer);
   WSI_GET_CB(DestroyCommandPool);
   WSI_GET_CB(DestroyFence);
   WSI_GET_CB(DestroyImage);
   WSI_GET_CB(DestroySemaphore);
   WSI_GET_CB(EndCommandBuffer);
   WSI_GET_CB(FreeMemory);
   WSI_GET_CB(FreeCommandBuffers);
   WSI_GET_CB(GetBufferMemoryRequirements);
   WSI_GET_CB(GetFenceStatus);
   WSI_GET_CB(GetImageDrmFormatModifierPropertiesEXT);
   WSI_GET_CB(GetImageMemoryRequirements);
   WSI_GET_CB(GetImageSubresourceLayout);
   if (!wsi->sw)
         WSI_GET_CB(GetPhysicalDeviceFormatProperties);
   WSI_GET_CB(GetPhysicalDeviceFormatProperties2KHR);
   WSI_GET_CB(GetPhysicalDeviceImageFormatProperties2);
   WSI_GET_CB(GetSemaphoreFdKHR);
   WSI_GET_CB(ResetFences);
   WSI_GET_CB(QueueSubmit);
   WSI_GET_CB(WaitForFences);
   WSI_GET_CB(MapMemory);
   WSI_GET_CB(UnmapMemory);
   if (wsi->khr_present_wait)
      #undef WSI_GET_CB
      #ifdef VK_USE_PLATFORM_XCB_KHR
      result = wsi_x11_init_wsi(wsi, alloc, dri_options);
   if (result != VK_SUCCESS)
      #endif
      #ifdef VK_USE_PLATFORM_WAYLAND_KHR
      result = wsi_wl_init_wsi(wsi, alloc, pdevice);
   if (result != VK_SUCCESS)
      #endif
      #ifdef VK_USE_PLATFORM_WIN32_KHR
      result = wsi_win32_init_wsi(wsi, alloc, pdevice);
   if (result != VK_SUCCESS)
      #endif
      #ifdef VK_USE_PLATFORM_DISPLAY_KHR
      result = wsi_display_init_wsi(wsi, alloc, display_fd);
   if (result != VK_SUCCESS)
      #endif
      #ifndef VK_USE_PLATFORM_WIN32_KHR
      result = wsi_headless_init_wsi(wsi, alloc, pdevice);
   if (result != VK_SUCCESS)
      #endif
         present_mode = getenv("MESA_VK_WSI_PRESENT_MODE");
   if (present_mode) {
      if (!strcmp(present_mode, "fifo")) {
         } else if (!strcmp(present_mode, "relaxed")) {
         } else if (!strcmp(present_mode, "mailbox")) {
         } else if (!strcmp(present_mode, "immediate")) {
         } else {
                     wsi->force_headless_swapchain =
            if (dri_options) {
      if (driCheckOption(dri_options, "adaptive_sync", DRI_BOOL))
                  if (driCheckOption(dri_options, "vk_wsi_force_bgra8_unorm_first",  DRI_BOOL)) {
      wsi->force_bgra8_unorm_first =
               if (driCheckOption(dri_options, "vk_wsi_force_swapchain_to_current_extent",  DRI_BOOL)) {
      wsi->force_swapchain_to_currentExtent =
                     fail:
      wsi_device_finish(wsi, alloc);
      }
      void
   wsi_device_finish(struct wsi_device *wsi,
         {
   #ifndef VK_USE_PLATFORM_WIN32_KHR
         #endif
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
         #endif
   #ifdef VK_USE_PLATFORM_WAYLAND_KHR
         #endif
   #ifdef VK_USE_PLATFORM_WIN32_KHR
         #endif
   #ifdef VK_USE_PLATFORM_XCB_KHR
         #endif
   }
      VKAPI_ATTR void VKAPI_CALL
   wsi_DestroySurfaceKHR(VkInstance _instance,
               {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
            if (!surface)
         #ifdef VK_USE_PLATFORM_WAYLAND_KHR
      if (surface->platform == VK_ICD_WSI_PLATFORM_WAYLAND) {
      wsi_wl_surface_destroy(surface, _instance, pAllocator);
         #endif
   #ifdef VK_USE_PLATFORM_WIN32_KHR
      if (surface->platform == VK_ICD_WSI_PLATFORM_WIN32) {
      wsi_win32_surface_destroy(surface, _instance, pAllocator);
         #endif
            }
      void
   wsi_device_setup_syncobj_fd(struct wsi_device *wsi_device,
         {
   #ifdef VK_USE_PLATFORM_DISPLAY_KHR
         #endif
   }
      static enum wsi_swapchain_blit_type
   get_blit_type(const struct wsi_device *wsi,
               {
      switch (params->image_type) {
   case WSI_IMAGE_TYPE_CPU: {
      const struct wsi_cpu_image_params *cpu_params =
         return wsi_cpu_image_needs_buffer_blit(wsi, cpu_params) ?
         #ifdef HAVE_LIBDRM
      case WSI_IMAGE_TYPE_DRM: {
      const struct wsi_drm_image_params *drm_params =
         return wsi_drm_image_needs_buffer_blit(wsi, drm_params) ?
         #endif
   #ifdef _WIN32
      case WSI_IMAGE_TYPE_DXGI: {
      const struct wsi_dxgi_image_params *dxgi_params =
               #endif
      default:
            }
      static VkResult
   configure_image(const struct wsi_swapchain *chain,
                     {
      switch (params->image_type) {
   case WSI_IMAGE_TYPE_CPU: {
      const struct wsi_cpu_image_params *cpu_params =
               #ifdef HAVE_LIBDRM
      case WSI_IMAGE_TYPE_DRM: {
      const struct wsi_drm_image_params *drm_params =
               #endif
   #ifdef _WIN32
      case WSI_IMAGE_TYPE_DXGI: {
      const struct wsi_dxgi_image_params *dxgi_params =
               #endif
      default:
            }
      #if defined(HAVE_PTHREAD) && !defined(_WIN32)
   bool
   wsi_init_pthread_cond_monotonic(pthread_cond_t *cond)
   {
      pthread_condattr_t condattr;
            if (pthread_condattr_init(&condattr) != 0)
            if (pthread_condattr_setclock(&condattr, CLOCK_MONOTONIC) != 0)
            if (pthread_cond_init(cond, &condattr) != 0)
                  fail_cond_init:
   fail_attr_set:
         fail_attr_init:
         }
   #endif
      VkResult
   wsi_swapchain_init(const struct wsi_device *wsi,
                     struct wsi_swapchain *chain,
   VkDevice _device,
   {
      VK_FROM_HANDLE(vk_device, device, _device);
                              chain->wsi = wsi;
   chain->device = _device;
   chain->alloc = *pAllocator;
            chain->blit.queue = VK_NULL_HANDLE;
   if (chain->blit.type != WSI_SWAPCHAIN_NO_BLIT && wsi->get_blit_queue)
                     chain->cmd_pools =
      vk_zalloc(pAllocator, sizeof(VkCommandPool) * cmd_pools_count, 8,
      if (!chain->cmd_pools)
            for (uint32_t i = 0; i < cmd_pools_count; i++) {
               if (chain->blit.queue != VK_NULL_HANDLE) {
      VK_FROM_HANDLE(vk_queue, queue, chain->blit.queue);
      }
   const VkCommandPoolCreateInfo cmd_pool_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
   .pNext = NULL,
   .flags = 0,
      };
   result = wsi->CreateCommandPool(_device, &cmd_pool_info, &chain->alloc,
         if (result != VK_SUCCESS)
               result = configure_image(chain, pCreateInfo, image_params,
         if (result != VK_SUCCESS)
                  fail:
      wsi_swapchain_finish(chain);
      }
      static bool
   wsi_swapchain_is_present_mode_supported(struct wsi_device *wsi,
               {
         ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, pCreateInfo->surface);
   struct wsi_interface *iface = wsi->wsi[surface->platform];
   VkPresentModeKHR *present_modes;
   uint32_t present_mode_count;
   bool supported = false;
            result = iface->get_present_modes(surface, wsi, &present_mode_count, NULL);
   if (result != VK_SUCCESS)
            present_modes = malloc(present_mode_count * sizeof(*present_modes));
   if (!present_modes)
            result = iface->get_present_modes(surface, wsi, &present_mode_count,
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < present_mode_count; i++) {
      if (present_modes[i] == mode) {
      supported = true;
            fail:
         free(present_modes);
   }
      enum VkPresentModeKHR
   wsi_swapchain_get_present_mode(struct wsi_device *wsi,
         {
      if (wsi->override_present_mode == VK_PRESENT_MODE_MAX_ENUM_KHR)
            if (!wsi_swapchain_is_present_mode_supported(wsi, pCreateInfo,
            fprintf(stderr, "Unsupported MESA_VK_WSI_PRESENT_MODE value!\n");
                  }
      void
   wsi_swapchain_finish(struct wsi_swapchain *chain)
   {
               if (chain->fences) {
      for (unsigned i = 0; i < chain->image_count; i++)
               }
   if (chain->blit.semaphores) {
      for (unsigned i = 0; i < chain->image_count; i++)
               }
   chain->wsi->DestroySemaphore(chain->device, chain->dma_buf_semaphore,
         chain->wsi->DestroySemaphore(chain->device, chain->present_id_timeline,
            int cmd_pools_count = chain->blit.queue != VK_NULL_HANDLE ?
         for (uint32_t i = 0; i < cmd_pools_count; i++) {
      chain->wsi->DestroyCommandPool(chain->device, chain->cmd_pools[i],
      }
               }
      VkResult
   wsi_configure_image(const struct wsi_swapchain *chain,
                     {
      memset(info, 0, sizeof(*info));
            if (pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT)
            /*
   * TODO: there should be no reason to allocate this, but
   * 15331 shows that games crashed without doing this.
   */
   uint32_t *queue_family_indices =
      vk_alloc(&chain->alloc,
            sizeof(*queue_family_indices) *
   if (!queue_family_indices)
            if (pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT)
      for (uint32_t i = 0; i < pCreateInfo->queueFamilyIndexCount; i++)
         info->create = (VkImageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
   .flags = VK_IMAGE_CREATE_ALIAS_BIT,
   .imageType = VK_IMAGE_TYPE_2D,
   .format = pCreateInfo->imageFormat,
   .extent = {
      .width = pCreateInfo->imageExtent.width,
   .height = pCreateInfo->imageExtent.height,
      },
   .mipLevels = 1,
   .arrayLayers = 1,
   .samples = VK_SAMPLE_COUNT_1_BIT,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
   .usage = pCreateInfo->imageUsage,
   .sharingMode = pCreateInfo->imageSharingMode,
   .queueFamilyIndexCount = queue_family_count,
   .pQueueFamilyIndices = queue_family_indices,
               if (handle_types != 0) {
      info->ext_mem = (VkExternalMemoryImageCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,
      };
               info->wsi = (struct wsi_image_create_info) {
         };
            if (pCreateInfo->flags & VK_SWAPCHAIN_CREATE_MUTABLE_FORMAT_BIT_KHR) {
      info->create.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
            const VkImageFormatListCreateInfo *format_list_in =
                           const uint32_t view_format_count = format_list_in->viewFormatCount;
   VkFormat *view_formats =
      vk_alloc(&chain->alloc, sizeof(VkFormat) * view_format_count,
      if (!view_formats)
            ASSERTED bool format_found = false;
   for (uint32_t i = 0; i < format_list_in->viewFormatCount; i++) {
      if (pCreateInfo->imageFormat == format_list_in->pViewFormats[i])
            }
            info->format_list = (VkImageFormatListCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO,
   .viewFormatCount = view_format_count,
      };
                     err_oom:
      wsi_destroy_image_info(chain, info);
      }
      void
   wsi_destroy_image_info(const struct wsi_swapchain *chain,
         {
      if (info->create.pQueueFamilyIndices != NULL) {
      vk_free(&chain->alloc, (void *)info->create.pQueueFamilyIndices);
      }
   if (info->format_list.pViewFormats != NULL) {
      vk_free(&chain->alloc, (void *)info->format_list.pViewFormats);
      }
   if (info->drm_mod_list.pDrmFormatModifiers != NULL) {
      vk_free(&chain->alloc, (void *)info->drm_mod_list.pDrmFormatModifiers);
      }
   if (info->modifier_props != NULL) {
      vk_free(&chain->alloc, info->modifier_props);
         }
      VkResult
   wsi_create_image(const struct wsi_swapchain *chain,
               {
      const struct wsi_device *wsi = chain->wsi;
                  #ifndef _WIN32
         #endif
         result = wsi->CreateImage(chain->device, &info->create,
         if (result != VK_SUCCESS)
            result = info->create_mem(chain, info, image);
   if (result != VK_SUCCESS)
            result = wsi->BindImageMemory(chain->device, image->image,
         if (result != VK_SUCCESS)
            if (info->finish_create) {
      result = info->finish_create(chain, info, image);
   if (result != VK_SUCCESS)
                     fail:
      wsi_destroy_image(chain, image);
      }
      void
   wsi_destroy_image(const struct wsi_swapchain *chain,
         {
            #ifndef _WIN32
      if (image->dma_buf_fd >= 0)
      #endif
         if (image->cpu_map != NULL) {
      wsi->UnmapMemory(chain->device, image->blit.buffer != VK_NULL_HANDLE ?
               if (image->blit.cmd_buffers) {
      int cmd_buffer_count =
            for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      wsi->FreeCommandBuffers(chain->device, chain->cmd_pools[i],
      }
               wsi->FreeMemory(chain->device, image->memory, &chain->alloc);
   wsi->DestroyImage(chain->device, image->image, &chain->alloc);
   wsi->DestroyImage(chain->device, image->blit.image, &chain->alloc);
   wsi->FreeMemory(chain->device, image->blit.memory, &chain->alloc);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_support(surface, wsi_device,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceCapabilitiesKHR(
      VkPhysicalDevice physicalDevice,
   VkSurfaceKHR _surface,
      {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            VkSurfaceCapabilities2KHR caps2 = {
                           if (result == VK_SUCCESS)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceCapabilities2KHR(
      VkPhysicalDevice physicalDevice,
   const VkPhysicalDeviceSurfaceInfo2KHR *pSurfaceInfo,
      {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, pSurfaceInfo->surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_capabilities2(surface, wsi_device, pSurfaceInfo->pNext,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceCapabilities2EXT(
      VkPhysicalDevice physicalDevice,
   VkSurfaceKHR _surface,
      {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            assert(pSurfaceCapabilities->sType ==
            struct wsi_surface_supported_counters counters = {
      .sType = VK_STRUCTURE_TYPE_WSI_SURFACE_SUPPORTED_COUNTERS_MESA,
   .pNext = pSurfaceCapabilities->pNext,
               VkSurfaceCapabilities2KHR caps2 = {
      .sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR,
                        if (result == VK_SUCCESS) {
      VkSurfaceCapabilities2EXT *ext_caps = pSurfaceCapabilities;
            ext_caps->minImageCount = khr_caps.minImageCount;
   ext_caps->maxImageCount = khr_caps.maxImageCount;
   ext_caps->currentExtent = khr_caps.currentExtent;
   ext_caps->minImageExtent = khr_caps.minImageExtent;
   ext_caps->maxImageExtent = khr_caps.maxImageExtent;
   ext_caps->maxImageArrayLayers = khr_caps.maxImageArrayLayers;
   ext_caps->supportedTransforms = khr_caps.supportedTransforms;
   ext_caps->currentTransform = khr_caps.currentTransform;
   ext_caps->supportedCompositeAlpha = khr_caps.supportedCompositeAlpha;
   ext_caps->supportedUsageFlags = khr_caps.supportedUsageFlags;
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_formats(surface, wsi_device,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfaceFormats2KHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, pSurfaceInfo->surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_formats2(surface, wsi_device, pSurfaceInfo->pNext,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_present_modes(surface, wsi_device, pPresentModeCount,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetPhysicalDevicePresentRectanglesKHR(VkPhysicalDevice physicalDevice,
                     {
      VK_FROM_HANDLE(vk_physical_device, device, physicalDevice);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, _surface);
   struct wsi_device *wsi_device = device->wsi_device;
            return iface->get_present_rectangles(surface, wsi_device,
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_CreateSwapchainKHR(VkDevice _device,
                     {
      MESA_TRACE_FUNC();
   VK_FROM_HANDLE(vk_device, device, _device);
   ICD_FROM_HANDLE(VkIcdSurfaceBase, surface, pCreateInfo->surface);
   struct wsi_device *wsi_device = device->physical->wsi_device;
   struct wsi_interface *iface = wsi_device->force_headless_swapchain ?
      wsi_device->wsi[VK_ICD_WSI_PLATFORM_HEADLESS] :
      const VkAllocationCallbacks *alloc;
            if (pAllocator)
   alloc = pAllocator;
   else
                     if (wsi_device->force_swapchain_to_currentExtent) {
      VkSurfaceCapabilities2KHR caps2 = {
         };
   iface->get_capabilities2(surface, wsi_device, NULL, &caps2);
               /* Ignore DEFERRED_MEMORY_ALLOCATION_BIT. Would require deep plumbing to be able to take advantage of it.
   * bool deferred_allocation = pCreateInfo->flags & VK_SWAPCHAIN_CREATE_DEFERRED_MEMORY_ALLOCATION_BIT_EXT;
            VkResult result = iface->create_swapchain(surface, _device, wsi_device,
               if (result != VK_SUCCESS)
            swapchain->fences = vk_zalloc(alloc,
                     if (!swapchain->fences) {
      swapchain->destroy(swapchain, alloc);
               if (wsi_device->khr_present_wait) {
      const VkSemaphoreTypeCreateInfo type_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
               const VkSemaphoreCreateInfo sem_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   .pNext = &type_info,
               /* We assume here that a driver exposing present_wait also exposes VK_KHR_timeline_semaphore. */
   result = wsi_device->CreateSemaphore(_device, &sem_info, alloc, &swapchain->present_id_timeline);
   if (result != VK_SUCCESS) {
      swapchain->destroy(swapchain, alloc);
                  if (swapchain->blit.queue != VK_NULL_HANDLE) {
      swapchain->blit.semaphores = vk_zalloc(alloc,
                     if (!swapchain->blit.semaphores) {
      wsi_device->DestroySemaphore(_device, swapchain->present_id_timeline, alloc);
   swapchain->destroy(swapchain, alloc);
                              }
      VKAPI_ATTR void VKAPI_CALL
   wsi_DestroySwapchainKHR(VkDevice _device,
               {
      MESA_TRACE_FUNC();
   VK_FROM_HANDLE(vk_device, device, _device);
   VK_FROM_HANDLE(wsi_swapchain, swapchain, _swapchain);
            if (!swapchain)
            if (pAllocator)
   alloc = pAllocator;
   else
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_ReleaseSwapchainImagesEXT(VkDevice _device,
         {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, pReleaseInfo->swapchain);
   VkResult result = swapchain->release_images(swapchain,
                  if (result != VK_SUCCESS)
            if (swapchain->wsi->set_memory_ownership) {
      for (uint32_t i = 0; i < pReleaseInfo->imageIndexCount; i++) {
      uint32_t image_index = pReleaseInfo->pImageIndices[i];
   VkDeviceMemory mem = swapchain->get_wsi_image(swapchain, image_index)->memory;
                     }
      VkResult
   wsi_common_get_images(VkSwapchainKHR _swapchain,
               {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, _swapchain);
            for (uint32_t i = 0; i < swapchain->image_count; i++) {
      vk_outarray_append_typed(VkImage, &images, image) {
                        }
      VkImage
   wsi_common_get_image(VkSwapchainKHR _swapchain, uint32_t index)
   {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, _swapchain);
   assert(index < swapchain->image_count);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetSwapchainImagesKHR(VkDevice device,
                     {
      MESA_TRACE_FUNC();
   return wsi_common_get_images(swapchain,
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_AcquireNextImageKHR(VkDevice _device,
                           VkSwapchainKHR swapchain,
   {
      MESA_TRACE_FUNC();
            const VkAcquireNextImageInfoKHR acquire_info = {
      .sType = VK_STRUCTURE_TYPE_ACQUIRE_NEXT_IMAGE_INFO_KHR,
   .swapchain = swapchain,
   .timeout = timeout,
   .semaphore = semaphore,
   .fence = fence,
               return device->dispatch_table.AcquireNextImage2KHR(_device, &acquire_info,
      }
      static VkResult
   wsi_signal_semaphore_for_image(struct vk_device *device,
                     {
      if (device->physical->supported_sync_types == NULL)
                           #ifdef HAVE_LIBDRM
      VkResult result = wsi_create_sync_for_dma_buf_wait(chain, image,
               if (result != VK_ERROR_FEATURE_NOT_PRESENT)
      #endif
         if (chain->wsi->signal_semaphore_with_memory) {
      return device->create_sync_for_memory(device, image->memory,
            } else {
      return vk_sync_create(device, &vk_sync_dummy_type,
               }
      static VkResult
   wsi_signal_fence_for_image(struct vk_device *device,
                     {
      if (device->physical->supported_sync_types == NULL)
                           #ifdef HAVE_LIBDRM
      VkResult result = wsi_create_sync_for_dma_buf_wait(chain, image,
               if (result != VK_ERROR_FEATURE_NOT_PRESENT)
      #endif
         if (chain->wsi->signal_fence_with_memory) {
      return device->create_sync_for_memory(device, image->memory,
            } else {
      return vk_sync_create(device, &vk_sync_dummy_type,
               }
      VkResult
   wsi_common_acquire_next_image2(const struct wsi_device *wsi,
                     {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, pAcquireInfo->swapchain);
            VkResult result = swapchain->acquire_next_image(swapchain, pAcquireInfo,
         if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
         struct wsi_image *image =
            if (pAcquireInfo->semaphore != VK_NULL_HANDLE) {
      VkResult signal_result =
      wsi_signal_semaphore_for_image(device, swapchain, image,
      if (signal_result != VK_SUCCESS)
               if (pAcquireInfo->fence != VK_NULL_HANDLE) {
      VkResult signal_result =
      wsi_signal_fence_for_image(device, swapchain, image,
      if (signal_result != VK_SUCCESS)
               if (wsi->set_memory_ownership)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_AcquireNextImage2KHR(VkDevice _device,
               {
      MESA_TRACE_FUNC();
            return wsi_common_acquire_next_image2(device->physical->wsi_device,
      }
      static VkResult wsi_signal_present_id_timeline(struct wsi_swapchain *swapchain,
               {
               const VkTimelineSemaphoreSubmitInfo timeline_info = {
      .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
   .pSignalSemaphoreValues = &present_id,
               const VkSubmitInfo submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .pNext = &timeline_info,
   .signalSemaphoreCount = 1,
               uint32_t submit_count = present_id ? 1 : 0;
      }
      static VkResult
   handle_trace(VkQueue queue, struct vk_device *device)
   {
      struct vk_instance *instance = device->physical->instance;
   if (!instance->trace_mode)
                     bool frame_trigger = device->current_frame == instance->trace_frame;
   if (device->current_frame <= instance->trace_frame)
               #ifndef _WIN32
      if (instance->trace_trigger_file && access(instance->trace_trigger_file, W_OK) == 0) {
      if (unlink(instance->trace_trigger_file) == 0) {
         } else {
      /* Do not enable tracing if we cannot remove the file,
   * because by then we'll trace every frame ... */
            #endif
         VkResult result = VK_SUCCESS;
   if (frame_trigger || file_trigger || device->trace_hotkey_trigger)
                                 }
      VkResult
   wsi_common_queue_present(const struct wsi_device *wsi,
                           {
               STACK_ARRAY(VkPipelineStageFlags, stage_flags,
         for (uint32_t s = 0; s < MAX2(1, pPresentInfo->waitSemaphoreCount); s++)
            const VkPresentRegionsKHR *regions =
         const VkPresentIdKHR *present_ids =
         const VkSwapchainPresentFenceInfoEXT *present_fence_info =
         const VkSwapchainPresentModeInfoEXT *present_mode_info =
            for (uint32_t i = 0; i < pPresentInfo->swapchainCount; i++) {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, pPresentInfo->pSwapchains[i]);
   uint32_t image_index = pPresentInfo->pImageIndices[i];
            /* Update the present mode for this present and any subsequent present. */
   if (present_mode_info && present_mode_info->pPresentModes && swapchain->set_present_mode)
            if (swapchain->fences[image_index] == VK_NULL_HANDLE) {
      const VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
   .pNext = NULL,
      };
   result = wsi->CreateFence(device, &fence_info,
                              if (swapchain->blit.type != WSI_SWAPCHAIN_NO_BLIT &&
      swapchain->blit.queue != VK_NULL_HANDLE) {
   const VkSemaphoreCreateInfo sem_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   .pNext = NULL,
      };
   result = wsi->CreateSemaphore(device, &sem_info,
               if (result != VK_SUCCESS)
         } else {
      MESA_TRACE_SCOPE("throttle");
   result =
      wsi->WaitForFences(device, 1, &swapchain->fences[image_index],
      if (result != VK_SUCCESS)
               result = wsi->ResetFences(device, 1, &swapchain->fences[image_index]);
   if (result != VK_SUCCESS)
            VkSubmitInfo submit_info = {
                  if (i == 0) {
      /* We only need/want to wait on semaphores once.  After that, we're
   * guaranteed ordering since it all happens on the same queue.
   */
   submit_info.waitSemaphoreCount = pPresentInfo->waitSemaphoreCount;
   submit_info.pWaitSemaphores = pPresentInfo->pWaitSemaphores;
               struct wsi_image *image =
            VkQueue submit_queue = queue;
   if (swapchain->blit.type != WSI_SWAPCHAIN_NO_BLIT) {
      if (swapchain->blit.queue == VK_NULL_HANDLE) {
      submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers =
      } else {
      /* If we are using a blit using the driver's private queue, then
   * do an empty submit signalling a semaphore, and then submit the
   * blit waiting on that.  This ensures proper queue ordering of
   * vkQueueSubmit() calls.
   */
   submit_info.signalSemaphoreCount = 1;
                  result = wsi->QueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
                  /* Now prepare the blit submit.  It needs to then wait on the
   * semaphore we signaled above.
   */
   submit_queue = swapchain->blit.queue;
   submit_info.waitSemaphoreCount = 1;
   submit_info.pWaitSemaphores = submit_info.pSignalSemaphores;
   submit_info.signalSemaphoreCount = 0;
   submit_info.pSignalSemaphores = NULL;
   submit_info.commandBufferCount = 1;
   submit_info.pCommandBuffers = &image->blit.cmd_buffers[0];
                           #ifdef HAVE_LIBDRM
         result = wsi_prepare_signal_dma_buf_from_semaphore(swapchain, image);
   if (result == VK_SUCCESS) {
      assert(submit_info.signalSemaphoreCount == 0);
   submit_info.signalSemaphoreCount = 1;
   submit_info.pSignalSemaphores = &swapchain->dma_buf_semaphore;
      } else if (result == VK_ERROR_FEATURE_NOT_PRESENT) {
      result = VK_SUCCESS;
      } else {
         #endif
            struct wsi_memory_signal_submit_info mem_signal;
   if (!has_signal_dma_buf) {
      /* If we don't have dma-buf signaling, signal the memory object by
   * chaining wsi_memory_signal_submit_info into VkSubmitInfo.
   */
   result = VK_SUCCESS;
   has_signal_dma_buf = false;
   mem_signal = (struct wsi_memory_signal_submit_info) {
      .sType = VK_STRUCTURE_TYPE_WSI_MEMORY_SIGNAL_SUBMIT_INFO_MESA,
      };
               result = wsi->QueueSubmit(submit_queue, 1, &submit_info, fence);
   if (result != VK_SUCCESS)
      #ifdef HAVE_LIBDRM
         if (has_signal_dma_buf) {
      result = wsi_signal_dma_buf_from_semaphore(swapchain, image);
   if (result != VK_SUCCESS)
      #else
         #endif
            if (wsi->sw)
                  const VkPresentRegionKHR *region = NULL;
   if (regions && regions->pRegions)
            uint64_t present_id = 0;
   if (present_ids && present_ids->pPresentIds)
         VkFence present_fence = VK_NULL_HANDLE;
   if (present_fence_info && present_fence_info->pFences)
            if (present_id || present_fence) {
      result = wsi_signal_present_id_timeline(swapchain, queue, present_id, present_fence);
   if (result != VK_SUCCESS)
               result = swapchain->queue_present(swapchain, image_index, present_id, region);
   if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            if (wsi->set_memory_ownership) {
      VkDeviceMemory mem = swapchain->get_wsi_image(swapchain, image_index)->memory;
            fail_present:
      if (pPresentInfo->pResults != NULL)
            /* Let the final result be our first unsuccessful result */
   if (final_result == VK_SUCCESS)
                           }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo)
   {
      MESA_TRACE_FUNC();
            return wsi_common_queue_present(queue->base.device->physical->wsi_device,
                        }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDeviceGroupPresentCapabilitiesKHR(VkDevice device,
         {
      memset(pCapabilities->presentMask, 0,
         pCapabilities->presentMask[0] = 0x1;
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_GetDeviceGroupSurfacePresentModesKHR(VkDevice device,
               {
                  }
      bool
   wsi_common_vk_instance_supports_present_wait(const struct vk_instance *instance)
   {
      /* We can only expose KHR_present_wait and KHR_present_id
   * if we are guaranteed support on all potential VkSurfaceKHR objects. */
   if (instance->enabled_extensions.KHR_wayland_surface ||
         instance->enabled_extensions.KHR_win32_surface ||
                     }
      VkResult
   wsi_common_create_swapchain_image(const struct wsi_device *wsi,
                     {
            #ifndef NDEBUG
      const VkImageCreateInfo *swcInfo = &chain->image_info.create;
   assert(pCreateInfo->flags == 0);
   assert(pCreateInfo->imageType == swcInfo->imageType);
   assert(pCreateInfo->format == swcInfo->format);
   assert(pCreateInfo->extent.width == swcInfo->extent.width);
   assert(pCreateInfo->extent.height == swcInfo->extent.height);
   assert(pCreateInfo->extent.depth == swcInfo->extent.depth);
   assert(pCreateInfo->mipLevels == swcInfo->mipLevels);
   assert(pCreateInfo->arrayLayers == swcInfo->arrayLayers);
   assert(pCreateInfo->samples == swcInfo->samples);
   assert(pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL);
            vk_foreach_struct_const(ext, pCreateInfo->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO: {
      const VkImageFormatListCreateInfo *iflci =
                        for (uint32_t i = 0; i < iflci->viewFormatCount; i++) {
      bool found = false;
   for (uint32_t j = 0; j < swc_iflci->viewFormatCount; j++) {
      if (iflci->pViewFormats[i] == swc_iflci->pViewFormats[j]) {
      found = true;
         }
      }
               case VK_STRUCTURE_TYPE_IMAGE_SWAPCHAIN_CREATE_INFO_KHR:
            default:
               #endif
         return wsi->CreateImage(chain->device, &chain->image_info.create,
      }
      VkResult
   wsi_common_bind_swapchain_image(const struct wsi_device *wsi,
                     {
      VK_FROM_HANDLE(wsi_swapchain, chain, _swapchain);
               }
      VkResult
   wsi_swapchain_wait_for_present_semaphore(const struct wsi_swapchain *chain,
         {
      assert(chain->present_id_timeline);
   const VkSemaphoreWaitInfo wait_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
   .semaphoreCount = 1,
   .pSemaphores = &chain->present_id_timeline,
                  }
      uint32_t
   wsi_select_memory_type(const struct wsi_device *wsi,
                     {
               VkMemoryPropertyFlags common_props = ~0;
   u_foreach_bit(t, type_bits) {
                        if (deny_props & type.propertyFlags)
            if (!(req_props & ~type.propertyFlags))
               if ((deny_props & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) &&
      (common_props & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
   /* If they asked for non-device-local and all the types are device-local
   * (this is commonly true for UMA platforms), try again without denying
   * device-local types
   */
   deny_props &= ~VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                  }
      uint32_t
   wsi_select_device_memory_type(const struct wsi_device *wsi,
         {
      return wsi_select_memory_type(wsi, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      }
      static uint32_t
   wsi_select_host_memory_type(const struct wsi_device *wsi,
         {
      return wsi_select_memory_type(wsi, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      }
      VkResult
   wsi_create_buffer_blit_context(const struct wsi_swapchain *chain,
                           {
               const struct wsi_device *wsi = chain->wsi;
            const VkExternalMemoryBufferCreateInfo buffer_external_info = {
      .sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO,
   .pNext = NULL,
      };
   const VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .pNext = &buffer_external_info,
   .size = info->linear_size,
   .usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      };
   result = wsi->CreateBuffer(chain->device, &buffer_info,
         if (result != VK_SUCCESS)
            VkMemoryRequirements reqs;
   wsi->GetBufferMemoryRequirements(chain->device, image->blit.buffer, &reqs);
            struct wsi_memory_allocate_info memory_wsi_info = {
      .sType = VK_STRUCTURE_TYPE_WSI_MEMORY_ALLOCATE_INFO_MESA,
   .pNext = NULL,
      };
   VkMemoryDedicatedAllocateInfo buf_mem_dedicated_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   .pNext = &memory_wsi_info,
   .image = VK_NULL_HANDLE,
      };
   VkMemoryAllocateInfo buf_mem_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &buf_mem_dedicated_info,
   .allocationSize = info->linear_size,
   .memoryTypeIndex =
               void *sw_host_ptr = NULL;
   if (info->alloc_shm)
            VkExportMemoryAllocateInfo memory_export_info;
   VkImportMemoryHostPointerInfoEXT host_ptr_info;
   if (sw_host_ptr != NULL) {
      host_ptr_info = (VkImportMemoryHostPointerInfoEXT) {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
   .pHostPointer = sw_host_ptr,
      };
      } else if (handle_types != 0) {
      memory_export_info = (VkExportMemoryAllocateInfo) {
      .sType = VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO,
      };
               result = wsi->AllocateMemory(chain->device, &buf_mem_info,
         if (result != VK_SUCCESS)
            result = wsi->BindBufferMemory(chain->device, image->blit.buffer,
         if (result != VK_SUCCESS)
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
               result = wsi->AllocateMemory(chain->device, &memory_info,
         if (result != VK_SUCCESS)
            image->num_planes = 1;
   image->sizes[0] = info->linear_size;
   image->row_pitches[0] = info->linear_stride;
               }
      VkResult
   wsi_finish_create_blit_context(const struct wsi_swapchain *chain,
               {
      const struct wsi_device *wsi = chain->wsi;
            int cmd_buffer_count =
         image->blit.cmd_buffers =
      vk_zalloc(&chain->alloc,
            if (!image->blit.cmd_buffers)
            for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      const VkCommandBufferAllocateInfo cmd_buffer_info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
   .pNext = NULL,
   .commandPool = chain->cmd_pools[i],
   .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      };
   result = wsi->AllocateCommandBuffers(chain->device, &cmd_buffer_info,
         if (result != VK_SUCCESS)
            const VkCommandBufferBeginInfo begin_info = {
         };
            VkImageMemoryBarrier img_mem_barriers[] = {
      {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
   .pNext = NULL,
   .srcAccessMask = 0,
   .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
   .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
   .image = image->image,
   .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = 0,
   .levelCount = 1,
   .baseArrayLayer = 0,
         },
   {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
   .pNext = NULL,
   .srcAccessMask = 0,
   .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
   .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
   .image = image->blit.image,
   .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = 0,
   .levelCount = 1,
   .baseArrayLayer = 0,
            };
   uint32_t img_mem_barrier_count =
         wsi->CmdPipelineBarrier(image->blit.cmd_buffers[i],
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            if (chain->blit.type == WSI_SWAPCHAIN_BUFFER_BLIT) {
      struct VkBufferImageCopy buffer_image_copy = {
      .bufferOffset = 0,
   .bufferRowLength = info->linear_stride /
         .bufferImageHeight = 0,
   .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = 0,
   .baseArrayLayer = 0,
      },
   .imageOffset = { .x = 0, .y = 0, .z = 0 },
      };
   wsi->CmdCopyImageToBuffer(image->blit.cmd_buffers[i],
                        } else {
      struct VkImageCopy image_copy = {
      .srcSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = 0,
   .baseArrayLayer = 0,
      },
   .srcOffset = { .x = 0, .y = 0, .z = 0 },
   .dstSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = 0,
   .baseArrayLayer = 0,
      },
   .dstOffset = { .x = 0, .y = 0, .z = 0 },
               wsi->CmdCopyImage(image->blit.cmd_buffers[i],
                     image->image,
               img_mem_barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
   img_mem_barriers[0].dstAccessMask = 0;
   img_mem_barriers[0].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
   img_mem_barriers[0].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   img_mem_barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
   img_mem_barriers[1].dstAccessMask = 0;
   img_mem_barriers[1].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
   img_mem_barriers[1].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
   wsi->CmdPipelineBarrier(image->blit.cmd_buffers[i],
                           VK_PIPELINE_STAGE_TRANSFER_BIT,
            result = wsi->EndCommandBuffer(image->blit.cmd_buffers[i]);
   if (result != VK_SUCCESS)
                  }
      void
   wsi_configure_buffer_image(UNUSED const struct wsi_swapchain *chain,
                     {
               assert(util_is_power_of_two_nonzero(stride_align));
            info->create.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            const uint32_t cpp = vk_format_get_blocksize(pCreateInfo->imageFormat);
   info->linear_stride = pCreateInfo->imageExtent.width * cpp;
            /* Since we can pick the stride to be whatever we want, also align to the
   * device's optimalBufferCopyRowPitchAlignment so we get efficient copies.
   */
   assert(wsi->optimalBufferCopyRowPitchAlignment > 0);
   info->linear_stride = align(info->linear_stride,
            info->linear_size = (uint64_t)info->linear_stride *
                     }
      void
   wsi_configure_image_blit_image(UNUSED const struct wsi_swapchain *chain,
         {
      info->create.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
   info->wsi.blit_src = true;
      }
      static VkResult
   wsi_create_cpu_linear_image_mem(const struct wsi_swapchain *chain,
               {
      const struct wsi_device *wsi = chain->wsi;
            VkMemoryRequirements reqs;
            VkSubresourceLayout layout;
   wsi->GetImageSubresourceLayout(chain->device, image->image,
                                          const VkMemoryDedicatedAllocateInfo memory_dedicated_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   .image = image->image,
      };
   VkMemoryAllocateInfo memory_info = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   .pNext = &memory_dedicated_info,
   .allocationSize = reqs.size,
   .memoryTypeIndex =
               void *sw_host_ptr = NULL;
   if (info->alloc_shm)
            VkImportMemoryHostPointerInfoEXT host_ptr_info;
   if (sw_host_ptr != NULL) {
      host_ptr_info = (VkImportMemoryHostPointerInfoEXT) {
      .sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT,
   .pHostPointer = sw_host_ptr,
      };
               result = wsi->AllocateMemory(chain->device, &memory_info,
         if (result != VK_SUCCESS)
            result = wsi->MapMemory(chain->device, image->memory,
         if (result != VK_SUCCESS)
            image->num_planes = 1;
   image->sizes[0] = reqs.size;
   image->row_pitches[0] = layout.rowPitch;
               }
      static VkResult
   wsi_create_cpu_buffer_image_mem(const struct wsi_swapchain *chain,
               {
               result = wsi_create_buffer_blit_context(chain, info, image, 0,
         if (result != VK_SUCCESS)
            result = chain->wsi->MapMemory(chain->device, image->blit.memory,
         if (result != VK_SUCCESS)
               }
      bool
   wsi_cpu_image_needs_buffer_blit(const struct wsi_device *wsi,
         {
      if (WSI_DEBUG & WSI_DEBUG_BUFFER)
            if (wsi->wants_linear)
               }
      VkResult
   wsi_configure_cpu_image(const struct wsi_swapchain *chain,
                     {
      assert(params->base.image_type == WSI_IMAGE_TYPE_CPU);
   assert(chain->blit.type == WSI_SWAPCHAIN_NO_BLIT ||
            VkExternalMemoryHandleTypeFlags handle_types = 0;
   if (params->alloc_shm && chain->blit.type != WSI_SWAPCHAIN_NO_BLIT)
            VkResult result = wsi_configure_image(chain, pCreateInfo,
         if (result != VK_SUCCESS)
            if (chain->blit.type != WSI_SWAPCHAIN_NO_BLIT) {
      wsi_configure_buffer_image(chain, pCreateInfo,
                        info->select_blit_dst_memory_type = wsi_select_host_memory_type;
   info->select_image_memory_type = wsi_select_device_memory_type;
      } else {
      /* Force the image to be linear */
                                    }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_WaitForPresentKHR(VkDevice device, VkSwapchainKHR _swapchain,
         {
      VK_FROM_HANDLE(wsi_swapchain, swapchain, _swapchain);
   assert(swapchain->wait_for_present);
      }
