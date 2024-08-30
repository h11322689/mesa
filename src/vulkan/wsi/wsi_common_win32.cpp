   /*
   * Copyright © 2015 Intel Corporation
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
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
      #include "vk_format.h"
   #include "vk_instance.h"
   #include "vk_physical_device.h"
   #include "vk_util.h"
   #include "wsi_common_entrypoints.h"
   #include "wsi_common_private.h"
      #define D3D12_IGNORE_SDK_LAYERS
   #include <dxgi1_4.h>
   #include <directx/d3d12.h>
   #include <dxguids/dxguids.h>
      #include <dcomp.h>
      #if defined(__GNUC__)
   #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"      // warning: cast to pointer from integer of different size
   #endif
      struct wsi_win32;
      struct wsi_win32 {
                        const VkAllocationCallbacks *alloc;
   VkPhysicalDevice physical_device;
   struct {
      IDXGIFactory4 *factory;
         };
      enum wsi_win32_image_state {
      WSI_IMAGE_IDLE,
   WSI_IMAGE_DRAWING,
      };
      struct wsi_win32_image {
      struct wsi_image base;
   enum wsi_win32_image_state state;
   struct wsi_win32_swapchain *chain;
   struct {
         } dxgi;
   struct {
      HDC dc;
   HBITMAP bmp;
   int bmp_row_pitch;
         };
      struct wsi_win32_surface {
               /* The first time a swapchain is created against this surface, a DComp
   * target/visual will be created for it and that swapchain will be bound.
   * When a new swapchain is created, we delay changing the visual's content
   * until that swapchain has completed its first present once, otherwise the
   * window will flash white. When the currently-bound swapchain is destroyed,
   * the visual's content is unset.
   */
   IDCompositionTarget *target;
   IDCompositionVisual *visual;
      };
      struct wsi_win32_swapchain {
      struct wsi_swapchain         base;
   IDXGISwapChain3            *dxgi;
   struct wsi_win32           *wsi;
   wsi_win32_surface          *surface;
   uint64_t                     flip_sequence;
   VkResult                     status;
   VkExtent2D                 extent;
   HWND wnd;
   HDC chain_dc;
      };
      VKAPI_ATTR VkBool32 VKAPI_CALL
   wsi_GetPhysicalDeviceWin32PresentationSupportKHR(VkPhysicalDevice physicalDevice,
         {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   wsi_CreateWin32SurfaceKHR(VkInstance _instance,
                     {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
                     surface = (wsi_win32_surface *)vk_zalloc2(&instance->alloc, pAllocator, sizeof(*surface), 8,
            if (surface == NULL)
                     surface->base.hinstance = pCreateInfo->hinstance;
                        }
      void
   wsi_win32_surface_destroy(VkIcdSurfaceBase *icd_surface, VkInstance _instance,
         {
      VK_FROM_HANDLE(vk_instance, instance, _instance);
   wsi_win32_surface *surface = (wsi_win32_surface *)icd_surface;
   if (surface->visual)
         if (surface->target)
            }
      static VkResult
   wsi_win32_surface_get_support(VkIcdSurfaceBase *surface,
                     {
                  }
      static VkResult
   wsi_win32_surface_get_capabilities(VkIcdSurfaceBase *surf,
               {
               RECT win_rect;
   if (!GetClientRect(surface->hwnd, &win_rect))
                     if (!wsi_device->sw && wsi_device->win32.get_d3d12_command_queue) {
      /* DXGI doesn't support random presenting order (images need to
   * be presented in the order they were acquired), so we can't
   * expose more than two image per swapchain.
   */
      } else {
      caps->minImageCount = 1;
   /* Software callbacke, there is no real maximum */
               caps->currentExtent = {
      (uint32_t)win_rect.right - (uint32_t)win_rect.left,
      };
   caps->minImageExtent = { 1u, 1u };
   caps->maxImageExtent = {
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
   VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
         VK_FROM_HANDLE(vk_physical_device, pdevice, wsi_device->pdevice);
   if (pdevice->supported_extensions.EXT_attachment_feedback_loop_layout)
               }
      static VkResult
   wsi_win32_surface_get_capabilities2(VkIcdSurfaceBase *surface,
                     {
               const VkSurfacePresentModeEXT *present_mode =
            VkResult result =
      wsi_win32_surface_get_capabilities(surface, wsi_device,
         vk_foreach_struct(ext, caps->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_SURFACE_PROTECTED_CAPABILITIES_KHR: {
      VkSurfaceProtectedCapabilitiesKHR *protected_cap = (VkSurfaceProtectedCapabilitiesKHR *)ext;
   protected_cap->supportsProtected = VK_FALSE;
               case VK_STRUCTURE_TYPE_SURFACE_PRESENT_SCALING_CAPABILITIES_EXT: {
      /* Unsupported. */
   VkSurfacePresentScalingCapabilitiesEXT *scaling =
         scaling->supportedPresentScaling = 0;
   scaling->supportedPresentGravityX = 0;
   scaling->supportedPresentGravityY = 0;
   scaling->minScaledImageExtent = caps->surfaceCapabilities.minImageExtent;
   scaling->maxScaledImageExtent = caps->surfaceCapabilities.maxImageExtent;
               case VK_STRUCTURE_TYPE_SURFACE_PRESENT_MODE_COMPATIBILITY_EXT: {
      /* Unsupported, just report the input present mode. */
   VkSurfacePresentModeCompatibilityEXT *compat =
         if (compat->pPresentModes) {
      if (compat->presentModeCount) {
      assert(present_mode);
   compat->pPresentModes[0] = present_mode->presentMode;
         } else {
      if (!present_mode)
      wsi_common_vk_warn_once("Use of VkSurfacePresentModeCompatibilityEXT "
               }
               default:
      /* Ignored */
                     }
         static const struct {
         } available_surface_formats[] = {
      { VK_FORMAT_B8G8R8A8_SRGB },
      };
         static void
   get_sorted_vk_formats(struct wsi_device *wsi_device, VkFormat *sorted_formats)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(available_surface_formats); i++)
            if (wsi_device->force_bgra8_unorm_first) {
      for (unsigned i = 0; i < ARRAY_SIZE(available_surface_formats); i++) {
      if (sorted_formats[i] == VK_FORMAT_B8G8R8A8_UNORM) {
      sorted_formats[i] = sorted_formats[0];
   sorted_formats[0] = VK_FORMAT_B8G8R8A8_UNORM;
               }
      static VkResult
   wsi_win32_surface_get_formats(VkIcdSurfaceBase *icd_surface,
                     {
               VkFormat sorted_formats[ARRAY_SIZE(available_surface_formats)];
            for (unsigned i = 0; i < ARRAY_SIZE(sorted_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormatKHR, &out, f) {
      f->format = sorted_formats[i];
                     }
      static VkResult
   wsi_win32_surface_get_formats2(VkIcdSurfaceBase *icd_surface,
                           {
               VkFormat sorted_formats[ARRAY_SIZE(available_surface_formats)];
            for (unsigned i = 0; i < ARRAY_SIZE(sorted_formats); i++) {
      vk_outarray_append_typed(VkSurfaceFormat2KHR, &out, f) {
      assert(f->sType == VK_STRUCTURE_TYPE_SURFACE_FORMAT_2_KHR);
   f->surfaceFormat.format = sorted_formats[i];
                     }
      static const VkPresentModeKHR present_modes_gdi[] = {
         };
   static const VkPresentModeKHR present_modes_dxgi[] = {
      VK_PRESENT_MODE_IMMEDIATE_KHR,
   VK_PRESENT_MODE_MAILBOX_KHR,
      };
      static VkResult
   wsi_win32_surface_get_present_modes(VkIcdSurfaceBase *surface,
                     {
      const VkPresentModeKHR *array;
   size_t array_size;
   if (wsi_device->sw || !wsi_device->win32.get_d3d12_command_queue) {
      array = present_modes_gdi;
      } else {
      array = present_modes_dxgi;
               if (pPresentModes == NULL) {
      *pPresentModeCount = array_size;
               *pPresentModeCount = MIN2(*pPresentModeCount, array_size);
            if (*pPresentModeCount < array_size)
         else
      }
      static VkResult
   wsi_win32_surface_get_present_rectangles(VkIcdSurfaceBase *surface,
                     {
               vk_outarray_append_typed(VkRect2D, &out, rect) {
      /* We don't know a size so just return the usual "I don't know." */
   *rect = {
      { 0, 0 },
                     }
      static VkResult
   wsi_create_dxgi_image_mem(const struct wsi_swapchain *drv_chain,
               {
      struct wsi_win32_swapchain *chain = (struct wsi_win32_swapchain *)drv_chain;
                     struct wsi_win32_image *win32_image =
         uint32_t image_idx =
      ((uintptr_t)win32_image - (uintptr_t)chain->images) /
      if (FAILED(chain->dxgi->GetBuffer(image_idx,
                  VkResult result =
      wsi->win32.create_image_memory(chain->base.device,
                        if (result != VK_SUCCESS)
            if (chain->base.blit.type == WSI_SWAPCHAIN_NO_BLIT)
                     create.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
            result = wsi->CreateImage(chain->base.device, &create,
         if (result != VK_SUCCESS)
            result = wsi->BindImageMemory(chain->base.device, image->blit.image,
         if (result != VK_SUCCESS)
            VkMemoryRequirements reqs;
            const VkMemoryDedicatedAllocateInfo memory_dedicated_info = {
      VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO,
   nullptr,
   image->blit.image,
      };
   const VkMemoryAllocateInfo memory_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
   &memory_dedicated_info,
   reqs.size,
               return wsi->AllocateMemory(chain->base.device, &memory_info,
      }
      enum wsi_swapchain_blit_type
   wsi_dxgi_image_needs_blit(const struct wsi_device *wsi,
               {
      if (wsi->win32.requires_blits && wsi->win32.requires_blits(device))
         else if (params->storage_image)
            }
      VkResult
   wsi_dxgi_configure_image(const struct wsi_swapchain *chain,
                     {
      VkResult result =
         if (result != VK_SUCCESS)
                     if (chain->blit.type != WSI_SWAPCHAIN_NO_BLIT) {
      wsi_configure_image_blit_image(chain, info);
   info->select_image_memory_type = wsi_select_device_memory_type;
                  }
      static VkResult
   wsi_win32_image_init(VkDevice device_h,
                           {
      VkResult result = wsi_create_image(&chain->base, &chain->base.image_info,
         if (result != VK_SUCCESS)
            VkIcdSurfaceWin32 *win32_surface = (VkIcdSurfaceWin32 *)create_info->surface;
   chain->wnd = win32_surface->hwnd;
            if (chain->dxgi)
            chain->chain_dc = GetDC(chain->wnd);
   image->sw.dc = CreateCompatibleDC(chain->chain_dc);
            BITMAPINFO info = { 0 };
   info.bmiHeader.biSize = sizeof(BITMAPINFO);
   info.bmiHeader.biWidth = create_info->imageExtent.width;
   info.bmiHeader.biHeight = -create_info->imageExtent.height;
   info.bmiHeader.biPlanes = 1;
   info.bmiHeader.biBitCount = 32;
            bmp = CreateDIBSection(image->sw.dc, &info, DIB_RGB_COLORS, &image->sw.ppvBits, NULL, 0);
                     BITMAP header;
   int status = GetObject(bmp, sizeof(BITMAP), &header);
   (void)status;
   image->sw.bmp_row_pitch = header.bmWidthBytes;
               }
      static void
   wsi_win32_image_finish(struct wsi_win32_swapchain *chain,
               {
      if (image->dxgi.swapchain_res)
            if (image->sw.dc)
         if(image->sw.bmp)
            }
      static VkResult
   wsi_win32_swapchain_destroy(struct wsi_swapchain *drv_chain,
         {
      struct wsi_win32_swapchain *chain =
            for (uint32_t i = 0; i < chain->base.image_count; i++)
                     if (chain->surface->current_swapchain == chain)
            if (chain->dxgi)
            wsi_swapchain_finish(&chain->base);
   vk_free(allocator, chain);
      }
      static struct wsi_image *
   wsi_win32_get_wsi_image(struct wsi_swapchain *drv_chain,
         {
      struct wsi_win32_swapchain *chain =
               }
      static VkResult
   wsi_win32_release_images(struct wsi_swapchain *drv_chain,
         {
      struct wsi_win32_swapchain *chain =
            if (chain->status == VK_ERROR_SURFACE_LOST_KHR)
            for (uint32_t i = 0; i < count; i++) {
      uint32_t index = indices[i];
   assert(index < chain->base.image_count);
   assert(chain->images[index].state == WSI_IMAGE_DRAWING);
                  }
         static VkResult
   wsi_win32_acquire_next_image(struct wsi_swapchain *drv_chain,
               {
      struct wsi_win32_swapchain *chain =
            /* Bail early if the swapchain is broken */
   if (chain->status != VK_SUCCESS)
            for (uint32_t i = 0; i < chain->base.image_count; i++) {
      if (chain->images[i].state == WSI_IMAGE_IDLE) {
      *image_index = i;
   chain->images[i].state = WSI_IMAGE_DRAWING;
                  assert(chain->dxgi);
   uint32_t index = chain->dxgi->GetCurrentBackBufferIndex();
   if (chain->images[index].state == WSI_IMAGE_DRAWING) {
      index = (index + 1) % chain->base.image_count;
      }
   if (chain->wsi->wsi->WaitForFences(chain->base.device, 1,
                        *image_index = index;
   chain->images[index].state = WSI_IMAGE_DRAWING;
      }
      static VkResult
   wsi_win32_queue_present_dxgi(struct wsi_win32_swapchain *chain,
               {
      uint32_t rect_count = damage ? damage->rectangleCount : 0;
            for (uint32_t r = 0; r < rect_count; r++) {
      rects[r].left = damage->pRectangles[r].offset.x;
   rects[r].top = damage->pRectangles[r].offset.y;
   rects[r].right = damage->pRectangles[r].offset.x + damage->pRectangles[r].extent.width;
               DXGI_PRESENT_PARAMETERS params = {
      rect_count,
               image->state = WSI_IMAGE_QUEUED;
   UINT sync_interval = chain->base.present_mode == VK_PRESENT_MODE_FIFO_KHR ? 1 : 0;
   UINT present_flags = chain->base.present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ?
            HRESULT hres = chain->dxgi->Present1(sync_interval, present_flags, &params);
   switch (hres) {
   case DXGI_ERROR_DEVICE_REMOVED: return VK_ERROR_DEVICE_LOST;
   case E_OUTOFMEMORY: return VK_ERROR_OUT_OF_DEVICE_MEMORY;
   default:
      if (FAILED(hres))
                     if (chain->surface->current_swapchain != chain) {
      chain->surface->visual->SetContent(chain->dxgi);
   chain->wsi->dxgi.dcomp->Commit();
               /* Mark the other image idle */
   chain->status = VK_SUCCESS;
      }
      static VkResult
   wsi_win32_queue_present(struct wsi_swapchain *drv_chain,
                     {
      struct wsi_win32_swapchain *chain = (struct wsi_win32_swapchain *) drv_chain;
   assert(image_index < chain->base.image_count);
                     if (chain->dxgi)
            char *ptr = (char *)image->base.cpu_map;
            for (unsigned h = 0; h < chain->extent.height; h++) {
      memcpy(dptr, ptr, chain->extent.width * 4);
   dptr += image->sw.bmp_row_pitch;
      }
   if (!StretchBlt(chain->chain_dc, 0, 0, chain->extent.width, chain->extent.height, image->sw.dc, 0, 0, chain->extent.width, chain->extent.height, SRCCOPY))
                        }
      static VkResult
   wsi_win32_surface_create_swapchain_dxgi(
      wsi_win32_surface *surface,
   VkDevice device,
   struct wsi_win32 *wsi,
   const VkSwapchainCreateInfoKHR *create_info,
      {
      IDXGIFactory4 *factory = wsi->dxgi.factory;
   ID3D12CommandQueue *queue =
            DXGI_SWAP_CHAIN_DESC1 desc = {
      create_info->imageExtent.width,
   create_info->imageExtent.height,
   DXGI_FORMAT_B8G8R8A8_UNORM,
   create_info->imageArrayLayers > 1,  // Stereo
   { 1 },                              // SampleDesc
   0,                                  // Usage (filled in below)
   create_info->minImageCount,
   DXGI_SCALING_STRETCH,
   DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
   DXGI_ALPHA_MODE_UNSPECIFIED,
   chain->base.present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR ?
               if (create_info->imageUsage &
      (VK_IMAGE_USAGE_SAMPLED_BIT |
   VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT))
         if (create_info->imageUsage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            IDXGISwapChain1 *swapchain1;
   if (FAILED(factory->CreateSwapChainForComposition(queue, &desc, NULL, &swapchain1)) ||
      FAILED(swapchain1->QueryInterface(&chain->dxgi)))
                  if (!surface->target &&
      FAILED(wsi->dxgi.dcomp->CreateTargetForHwnd(surface->base.hwnd, false, &surface->target)))
         if (!surface->visual) {
      if (FAILED(wsi->dxgi.dcomp->CreateVisual(&surface->visual)) ||
      FAILED(surface->target->SetRoot(surface->visual)) ||
   FAILED(surface->visual->SetContent(chain->dxgi)) ||
                  }
      }
      static VkResult
   wsi_win32_surface_create_swapchain(
      VkIcdSurfaceBase *icd_surface,
   VkDevice device,
   struct wsi_device *wsi_device,
   const VkSwapchainCreateInfoKHR *create_info,
   const VkAllocationCallbacks *allocator,
      {
      wsi_win32_surface *surface = (wsi_win32_surface *)icd_surface;
   struct wsi_win32 *wsi =
                     const unsigned num_images = create_info->minImageCount;
   struct wsi_win32_swapchain *chain;
            chain = (wsi_win32_swapchain *)vk_zalloc(allocator, size,
            if (chain == NULL)
            struct wsi_dxgi_image_params dxgi_image_params = {
         };
            struct wsi_cpu_image_params cpu_image_params = {
                  bool supports_dxgi = wsi->dxgi.factory &&
               struct wsi_base_image_params *image_params = supports_dxgi ?
            VkResult result = wsi_swapchain_init(wsi_device, &chain->base, device,
               if (result != VK_SUCCESS) {
      vk_free(allocator, chain);
               chain->base.destroy = wsi_win32_swapchain_destroy;
   chain->base.get_wsi_image = wsi_win32_get_wsi_image;
   chain->base.acquire_next_image = wsi_win32_acquire_next_image;
   chain->base.release_images = wsi_win32_release_images;
   chain->base.queue_present = wsi_win32_queue_present;
   chain->base.present_mode = wsi_swapchain_get_present_mode(wsi_device, create_info);
            chain->wsi = wsi;
                     if (image_params->image_type == WSI_IMAGE_TYPE_DXGI) {
      result = wsi_win32_surface_create_swapchain_dxgi(surface, device, wsi, create_info, chain);
   if (result != VK_SUCCESS)
               for (uint32_t image = 0; image < num_images; image++) {
      result = wsi_win32_image_init(device, chain,
               if (result != VK_SUCCESS)
                                       fail:
      if (surface->visual) {
      surface->visual->SetContent(NULL);
   surface->current_swapchain = NULL;
      }
   wsi_win32_swapchain_destroy(&chain->base, allocator);
      }
      static IDXGIFactory4 *
   dxgi_get_factory(bool debug)
   {
      HMODULE dxgi_mod = LoadLibraryA("DXGI.DLL");
   if (!dxgi_mod) {
                  typedef HRESULT(WINAPI *PFN_CREATE_DXGI_FACTORY2)(UINT flags, REFIID riid, void **ppFactory);
            CreateDXGIFactory2 = (PFN_CREATE_DXGI_FACTORY2)GetProcAddress(dxgi_mod, "CreateDXGIFactory2");
   if (!CreateDXGIFactory2) {
                  UINT flags = 0;
   if (debug)
            IDXGIFactory4 *factory;
   HRESULT hr = CreateDXGIFactory2(flags, IID_PPV_ARGS(&factory));
   if (FAILED(hr)) {
                     }
      static IDCompositionDevice *
   dcomp_get_device()
   {
      HMODULE dcomp_mod = LoadLibraryA("DComp.DLL");
   if (!dcomp_mod) {
                  typedef HRESULT (STDAPICALLTYPE *PFN_DCOMP_CREATE_DEVICE)(IDXGIDevice *, REFIID, void **);
            DCompositionCreateDevice = (PFN_DCOMP_CREATE_DEVICE)GetProcAddress(dcomp_mod, "DCompositionCreateDevice");
   if (!DCompositionCreateDevice) {
                  IDCompositionDevice *device;
   HRESULT hr = DCompositionCreateDevice(NULL, IID_PPV_ARGS(&device));
   if (FAILED(hr)) {
                     }
      VkResult
   wsi_win32_init_wsi(struct wsi_device *wsi_device,
               {
      struct wsi_win32 *wsi;
            wsi = (wsi_win32 *)vk_zalloc(alloc, sizeof(*wsi), 8,
         if (!wsi) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               wsi->physical_device = physical_device;
   wsi->alloc = alloc;
            if (!wsi_device->sw) {
      wsi->dxgi.factory = dxgi_get_factory(WSI_DEBUG & WSI_DEBUG_DXGI);
   if (!wsi->dxgi.factory) {
      vk_free(alloc, wsi);
   result = VK_ERROR_INITIALIZATION_FAILED;
      }
   wsi->dxgi.dcomp = dcomp_get_device();
   if (!wsi->dxgi.dcomp) {
      wsi->dxgi.factory->Release();
   vk_free(alloc, wsi);
   result = VK_ERROR_INITIALIZATION_FAILED;
                  wsi->base.get_support = wsi_win32_surface_get_support;
   wsi->base.get_capabilities2 = wsi_win32_surface_get_capabilities2;
   wsi->base.get_formats = wsi_win32_surface_get_formats;
   wsi->base.get_formats2 = wsi_win32_surface_get_formats2;
   wsi->base.get_present_modes = wsi_win32_surface_get_present_modes;
   wsi->base.get_present_rectangles = wsi_win32_surface_get_present_rectangles;
                           fail:
                  }
      void
   wsi_win32_finish_wsi(struct wsi_device *wsi_device,
         {
      struct wsi_win32 *wsi =
         if (!wsi)
            if (wsi->dxgi.factory)
         if (wsi->dxgi.dcomp)
               }
