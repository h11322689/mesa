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
   #include "vk_debug_report.h"
   #include "vk_format.h"
   #include "vk_util.h"
      void
   dzn_image_align_extent(const struct dzn_image *image,
         {
      enum pipe_format pfmt = vk_format_to_pipe_format(image->vk.format);
   uint32_t blkw = util_format_get_blockwidth(pfmt);
   uint32_t blkh = util_format_get_blockheight(pfmt);
            assert(util_is_power_of_two_nonzero(blkw) &&
                  extent->width = ALIGN_POT(extent->width, blkw);
   extent->height = ALIGN_POT(extent->height, blkh);
      }
      static void
   dzn_image_destroy(struct dzn_image *image,
         {
      if (!image)
                     if (image->res)
            vk_image_finish(&image->vk);
      }
      static VkResult
   dzn_image_create(struct dzn_device *device,
                     {
      struct dzn_physical_device *pdev =
         VkFormat *compat_formats = NULL;
            if (pdev->options12.RelaxedFormatCastingSupported) {
      VkResult ret =
      vk_image_create_get_format_list(&device->vk, pCreateInfo, pAllocator,
      if (ret != VK_SUCCESS)
               VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct dzn_image, image, 1);
            if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT)) {
      vk_free2(&device->vk.alloc, pAllocator, compat_formats);
            #if 0
      VkExternalMemoryHandleTypeFlags supported =
      VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT |
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_KMT_BIT |
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_BIT |
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D11_TEXTURE_KMT_BIT |
   VK_EXTERNAL_MEMORY_HANDLE_TYPE_D3D12_HEAP_BIT |
         if (create_info && (create_info->handleTypes & supported))
      return dzn_image_from_external(device, pCreateInfo, create_info,
   #endif
      #if 0
      const VkImageSwapchainCreateInfoKHR *swapchain_info = (const VkImageSwapchainCreateInfoKHR *)
         if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE)
      return dzn_image_from_swapchain(device, pCreateInfo, swapchain_info,
   #endif
         vk_image_init(&device->vk, &image->vk, pCreateInfo);
                     image->castable_formats = castable_formats;
   image->castable_format_count = 0;
   for (uint32_t i = 0; i < compat_format_count; i++) {
      castable_formats[image->castable_format_count] =
            if (castable_formats[image->castable_format_count] != DXGI_FORMAT_UNKNOWN)
                                 if (image->vk.tiling == VK_IMAGE_TILING_LINEAR) {
      /* Treat linear images as buffers: they should only be used as copy
   * src/dest, and CopyTextureResource() can manipulate buffers.
   * We only support linear tiling on things strictly required by the spec:
   * "Images created with tiling equal to VK_IMAGE_TILING_LINEAR have
   * further restrictions on their limits and capabilities compared to
   * images created with tiling equal to VK_IMAGE_TILING_OPTIMAL. Creation
   * of images with tiling VK_IMAGE_TILING_LINEAR may not be supported
   * unless other parameters meet all of the constraints:
   * - imageType is VK_IMAGE_TYPE_2D
   * - format is not a depth/stencil format
   * - mipLevels is 1
   * - arrayLayers is 1
   * - samples is VK_SAMPLE_COUNT_1_BIT
   * - usage only includes VK_IMAGE_USAGE_TRANSFER_SRC_BIT and/or VK_IMAGE_USAGE_TRANSFER_DST_BIT
   * "
   */
   assert(!vk_format_is_depth_or_stencil(pCreateInfo->format));
   assert(pCreateInfo->mipLevels == 1);
   assert(pCreateInfo->arrayLayers == 1);
   assert(pCreateInfo->samples == 1);
   assert(pCreateInfo->imageType != VK_IMAGE_TYPE_3D);
   assert(!(usage & ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)));
   D3D12_RESOURCE_DESC tmp_desc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
   .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .Width = ALIGN_POT(image->vk.extent.width, util_format_get_blockwidth(pfmt)),
   .Height = (UINT)ALIGN_POT(image->vk.extent.height, util_format_get_blockheight(pfmt)),
   .DepthOrArraySize = 1,
   .MipLevels = 1,
   .Format =
         .SampleDesc = { .Count = 1, .Quality = 0 },
   .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
      };
   D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
   uint64_t size = 0;
            image->linear.row_stride = footprint.Footprint.RowPitch;
   image->linear.size = size;
   size *= pCreateInfo->arrayLayers;
   image->desc.Format = DXGI_FORMAT_UNKNOWN;
   image->desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
   image->desc.Width = size;
   image->desc.Height = 1;
   image->desc.DepthOrArraySize = 1;
   image->desc.MipLevels = 1;
   image->desc.SampleDesc.Count = 1;
   image->desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
   image->castable_formats = NULL;
      } else {
      image->desc.Format =
      dzn_image_get_dxgi_format(pdev, pCreateInfo->format,
            image->desc.Dimension = (D3D12_RESOURCE_DIMENSION)(D3D12_RESOURCE_DIMENSION_TEXTURE1D + pCreateInfo->imageType);
   image->desc.Width = image->vk.extent.width;
   image->desc.Height = image->vk.extent.height;
   image->desc.DepthOrArraySize = pCreateInfo->imageType == VK_IMAGE_TYPE_3D ?
               image->desc.MipLevels = pCreateInfo->mipLevels;
   image->desc.SampleDesc.Count = pCreateInfo->samples;
   image->desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
   image->valid_access |= D3D12_BARRIER_ACCESS_RESOLVE_DEST |
      D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
            if ((image->vk.create_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) &&
      !pdev->options12.RelaxedFormatCastingSupported)
         if (image->desc.SampleDesc.Count > 1)
         else
                              if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
      image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
               if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
      image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
   image->valid_access |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ |
            if (!(usage & (VK_IMAGE_USAGE_SAMPLED_BIT |
                        image->desc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
         } else if (usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
               /* Images with TRANSFER_DST can be cleared or passed as a blit/resolve
   * destination. Both operations require the RT or DS cap flags.
   */
   if ((usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) &&
               D3D12_FEATURE_DATA_FORMAT_SUPPORT dfmt_info =
         if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) {
      image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
      } else if ((dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) &&
            (image->desc.Flags & (D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET |
   image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
      } else if (dfmt_info.Support1 & D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW) {
      image->desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
                  if (pCreateInfo->sharingMode == VK_SHARING_MODE_CONCURRENT &&
      !(image->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL))
         *out = dzn_image_to_handle(image);
      }
      DXGI_FORMAT
   dzn_image_get_dxgi_format(const struct dzn_physical_device *pdev,
                     {
               if (pdev && !pdev->support_a4b4g4r4) {
      if (pfmt == PIPE_FORMAT_A4R4G4B4_UNORM)
         if (pfmt == PIPE_FORMAT_A4B4G4R4_UNORM)
               if (!vk_format_is_depth_or_stencil(format))
            switch (pfmt) {
   case PIPE_FORMAT_Z16_UNORM:
      return usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ?
         case PIPE_FORMAT_Z32_FLOAT:
      return usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT ?
         case PIPE_FORMAT_Z24X8_UNORM:
      if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
         if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
               case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
         else if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
         else
         case PIPE_FORMAT_X24S8_UINT:
      if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
         if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
               case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (usage == VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
            if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
         else if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
         else
         default:
            }
      DXGI_FORMAT
   dzn_image_get_placed_footprint_format(const struct dzn_physical_device *pdev,
               {
      DXGI_FORMAT out =
      dzn_image_get_dxgi_format(pdev, format,
                     switch (out) {
   case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
   case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
         case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
   case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
         default:
            }
      VkFormat
   dzn_image_get_plane_format(VkFormat format,
         {
      if (aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT)
         else if (aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT)
         else
      }
      uint32_t
   dzn_image_layers_get_subresource_index(const struct dzn_image *image,
                     {
      int planeSlice =
            return subres->mipLevel +
            }
      uint32_t
   dzn_image_range_get_subresource_index(const struct dzn_image *image,
                     {
      int planeSlice =
            return subres->baseMipLevel + level +
            }
      static uint32_t
   dzn_image_get_subresource_index(const struct dzn_image *image,
               {
      int planeSlice =
            return subres->mipLevel +
            }
      D3D12_TEXTURE_COPY_LOCATION
   dzn_image_get_copy_loc(const struct dzn_image *image,
                     {
      struct dzn_physical_device *pdev =
         D3D12_TEXTURE_COPY_LOCATION loc = {
                           if (image->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      assert((subres->baseArrayLayer + layer) == 0);
   assert(subres->mipLevel == 0);
   enum pipe_format pfmt = vk_format_to_pipe_format(image->vk.format);
   uint32_t blkw = util_format_get_blockwidth(pfmt);
   uint32_t blkh = util_format_get_blockheight(pfmt);
   uint32_t blkd = util_format_get_blockdepth(pfmt);
   loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
   loc.PlacedFootprint.Offset = 0;
   loc.PlacedFootprint.Footprint.Format =
         loc.PlacedFootprint.Footprint.Width = ALIGN_POT(image->vk.extent.width, blkw);
   loc.PlacedFootprint.Footprint.Height = ALIGN_POT(image->vk.extent.height, blkh);
   loc.PlacedFootprint.Footprint.Depth = ALIGN_POT(image->vk.extent.depth, blkd);
      } else {
      loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                  }
      D3D12_DEPTH_STENCIL_VIEW_DESC
   dzn_image_get_dsv_desc(const struct dzn_image *image,
               {
      struct dzn_physical_device *pdev =
         uint32_t layer_count = dzn_get_layer_count(image, range);
   D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
      .Format =
      dzn_image_get_dxgi_format(pdev, image->vk.format,
                  switch (image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      dsv_desc.ViewDimension =
      image->vk.array_layers > 1 ?
   D3D12_DSV_DIMENSION_TEXTURE1DARRAY :
         case VK_IMAGE_TYPE_2D:
      if (image->vk.array_layers > 1) {
      dsv_desc.ViewDimension =
      image->vk.samples > 1 ?
   D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY :
   } else {
      dsv_desc.ViewDimension =
      image->vk.samples > 1 ?
   D3D12_DSV_DIMENSION_TEXTURE2DMS :
   }
      default:
                  switch (dsv_desc.ViewDimension) {
   case D3D12_DSV_DIMENSION_TEXTURE1D:
      dsv_desc.Texture1D.MipSlice = range->baseMipLevel + level;
      case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
      dsv_desc.Texture1DArray.MipSlice = range->baseMipLevel + level;
   dsv_desc.Texture1DArray.FirstArraySlice = range->baseArrayLayer;
   dsv_desc.Texture1DArray.ArraySize = layer_count;
      case D3D12_DSV_DIMENSION_TEXTURE2D:
      dsv_desc.Texture2D.MipSlice = range->baseMipLevel + level;
      case D3D12_DSV_DIMENSION_TEXTURE2DMS:
         case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
      dsv_desc.Texture2DArray.MipSlice = range->baseMipLevel + level;
   dsv_desc.Texture2DArray.FirstArraySlice = range->baseArrayLayer;
   dsv_desc.Texture2DArray.ArraySize = layer_count;
      case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
      dsv_desc.Texture2DMSArray.FirstArraySlice = range->baseArrayLayer;
   dsv_desc.Texture2DMSArray.ArraySize = layer_count;
      default:
                     }
      D3D12_RENDER_TARGET_VIEW_DESC
   dzn_image_get_rtv_desc(const struct dzn_image *image,
               {
      struct dzn_physical_device *pdev =
         uint32_t layer_count = dzn_get_layer_count(image, range);
   D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
      .Format =
      dzn_image_get_dxgi_format(pdev, image->vk.format,
                  switch (image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      rtv_desc.ViewDimension =
      image->vk.array_layers > 1 ?
         case VK_IMAGE_TYPE_2D:
      if (image->vk.array_layers > 1) {
      rtv_desc.ViewDimension =
      image->vk.samples > 1 ?
   D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY :
   } else {
      rtv_desc.ViewDimension =
      image->vk.samples > 1 ?
   D3D12_RTV_DIMENSION_TEXTURE2DMS :
   }
      case VK_IMAGE_TYPE_3D:
      rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
      default: unreachable("Invalid image type\n");
            switch (rtv_desc.ViewDimension) {
   case D3D12_RTV_DIMENSION_TEXTURE1D:
      rtv_desc.Texture1D.MipSlice = range->baseMipLevel + level;
      case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
      rtv_desc.Texture1DArray.MipSlice = range->baseMipLevel + level;
   rtv_desc.Texture1DArray.FirstArraySlice = range->baseArrayLayer;
   rtv_desc.Texture1DArray.ArraySize = layer_count;
      case D3D12_RTV_DIMENSION_TEXTURE2D:
      rtv_desc.Texture2D.MipSlice = range->baseMipLevel + level;
   if (range->aspectMask & VK_IMAGE_ASPECT_PLANE_1_BIT)
         else if (range->aspectMask & VK_IMAGE_ASPECT_PLANE_2_BIT)
         else
            case D3D12_RTV_DIMENSION_TEXTURE2DMS:
         case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
      rtv_desc.Texture2DArray.MipSlice = range->baseMipLevel + level;
   rtv_desc.Texture2DArray.FirstArraySlice = range->baseArrayLayer;
   rtv_desc.Texture2DArray.ArraySize = layer_count;
   if (range->aspectMask & VK_IMAGE_ASPECT_PLANE_1_BIT)
         else if (range->aspectMask & VK_IMAGE_ASPECT_PLANE_2_BIT)
         else
            case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
      rtv_desc.Texture2DMSArray.FirstArraySlice = range->baseArrayLayer;
   rtv_desc.Texture2DMSArray.ArraySize = layer_count;
      case D3D12_RTV_DIMENSION_TEXTURE3D:
      rtv_desc.Texture3D.MipSlice = range->baseMipLevel + level;
   rtv_desc.Texture3D.FirstWSlice = range->baseArrayLayer;
   rtv_desc.Texture3D.WSize =
            default:
                     }
      D3D12_RESOURCE_STATES
   dzn_image_layout_to_state(const struct dzn_image *image,
                     {
      D3D12_RESOURCE_STATES shaders_access =
      (image->desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) ?
   0 : (type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
               switch (layout) {
   case VK_IMAGE_LAYOUT_PREINITIALIZED:
   case VK_IMAGE_LAYOUT_UNDEFINED:
   case VK_IMAGE_LAYOUT_GENERAL:
         case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      return type == D3D12_COMMAND_LIST_TYPE_DIRECT ?
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return aspect == VK_IMAGE_ASPECT_STENCIL_BIT ?
               case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
      return aspect == VK_IMAGE_ASPECT_STENCIL_BIT ?
               case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
            default:
            }
      D3D12_BARRIER_LAYOUT
   dzn_vk_layout_to_d3d_layout(VkImageLayout layout,
               {
      if (type == D3D12_COMMAND_LIST_TYPE_COPY)
            switch (layout) {
   case VK_IMAGE_LAYOUT_UNDEFINED:
         case VK_IMAGE_LAYOUT_PREINITIALIZED:
         case VK_IMAGE_LAYOUT_GENERAL:
   case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      switch (type) {
   case D3D12_COMMAND_LIST_TYPE_DIRECT: return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON;
   case D3D12_COMMAND_LIST_TYPE_COMPUTE: return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON;
   default: return D3D12_BARRIER_LAYOUT_COMMON;
      case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
   case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      switch (type) {
   case D3D12_COMMAND_LIST_TYPE_DIRECT: return D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ;
   case D3D12_COMMAND_LIST_TYPE_COMPUTE: return D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ;
   default: return D3D12_BARRIER_LAYOUT_GENERIC_READ;
      case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
         case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      return aspect == VK_IMAGE_ASPECT_DEPTH_BIT ?
      dzn_vk_layout_to_d3d_layout(VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, type, aspect) :
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
      return aspect == VK_IMAGE_ASPECT_DEPTH_BIT ?
      D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE :
   case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
      return aspect == VK_IMAGE_ASPECT_COLOR_BIT ?
      case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
   case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
         case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
         default:
      assert(!"Unexpected layout");
         }
      bool
   dzn_image_formats_are_compatible(const struct dzn_device *device,
                     {
      const struct dzn_physical_device *pdev =
         DXGI_FORMAT orig_dxgi = dzn_image_get_dxgi_format(pdev, orig_fmt, usage, aspect);
            if (orig_dxgi == new_dxgi)
            DXGI_FORMAT typeless_orig = dzn_get_typeless_dxgi_format(orig_dxgi);
            if (!(usage & VK_IMAGE_USAGE_SAMPLED_BIT))
            if (pdev->options3.CastingFullyTypedFormatSupported) {
      enum pipe_format orig_pfmt = vk_format_to_pipe_format(orig_fmt);
            /* Types don't belong to the same group, they're incompatible. */
   if (typeless_orig != typeless_new)
            /* FLOAT <-> non-FLOAT casting is disallowed. */
   if (util_format_is_float(orig_pfmt) != util_format_is_float(new_pfmt))
            /* UNORM <-> SNORM casting is disallowed. */
   bool orig_is_norm =
         bool new_is_norm =
         if (orig_is_norm && new_is_norm &&
                                 }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateImage(VkDevice device,
                     {
      return dzn_image_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyImage(VkDevice device, VkImage image,
         {
         }
      static struct dzn_image *
   dzn_swapchain_get_image(struct dzn_device *device,
               {
      uint32_t n_images = index + 1;
   STACK_ARRAY(VkImage, images, n_images);
                     if (result == VK_SUCCESS || result == VK_INCOMPLETE)
            STACK_ARRAY_FINISH(images);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_BindImageMemory2(VkDevice dev,
               {
               for (uint32_t i = 0; i < bindInfoCount; i++) {
      const VkBindImageMemoryInfo *bind_info = &pBindInfos[i];
   VK_FROM_HANDLE(dzn_device_memory, mem, bind_info->memory);
            vk_foreach_struct_const(s, bind_info->pNext) {
                                    if (mem->dedicated_res) {
      assert(pBindInfos[i].memoryOffset == 0);
   image->res = mem->dedicated_res;
      } else if (device->dev10 && image->castable_format_count > 0) {
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
               hres = ID3D12Device10_CreatePlacedResource2(device->dev10, mem->heap,
                                                } else {
      D3D12_RESOURCE_DESC desc = image->desc;
   desc.Flags |= mem->res_flags;
   hres = ID3D12Device1_CreatePlacedResource(device->dev, mem->heap,
                                    }
   if (FAILED(hres))
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetImageMemoryRequirements2(VkDevice _device,
               {
      VK_FROM_HANDLE(dzn_device, device, _device);
   VK_FROM_HANDLE(dzn_image, image, pInfo->image);
   struct dzn_physical_device *pdev =
            vk_foreach_struct_const(ext, pInfo->pNext) {
                  vk_foreach_struct(ext, pMemoryRequirements->pNext) {
      switch (ext->sType) {
   case VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS: {
      VkMemoryDedicatedRequirements *requirements =
         requirements->requiresDedicatedAllocation = image->vk.external_handle_types != 0;
   requirements->prefersDedicatedAllocation = requirements->requiresDedicatedAllocation ||
                     default:
      dzn_debug_ignored_stype(ext->sType);
                  D3D12_RESOURCE_ALLOCATION_INFO info;
   if (device->dev12 && image->castable_format_count > 0) {
      D3D12_RESOURCE_DESC1 desc1;
   memcpy(&desc1, &image->desc, sizeof(image->desc));
   memset(&desc1.SamplerFeedbackMipRegion, 0, sizeof(desc1.SamplerFeedbackMipRegion));
   info = dzn_ID3D12Device12_GetResourceAllocationInfo3(device->dev12, 0, 1, &desc1,
                  } else {
                  pMemoryRequirements->memoryRequirements = (VkMemoryRequirements) {
      .size = info.SizeInBytes,
   .alignment = info.Alignment,
   .memoryTypeBits =
      dzn_physical_device_get_mem_type_mask_for_resource(pdev, &image->desc,
            /*
   * MSAA images need memory to be aligned on
   * D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT (4MB), but the memory
   * allocation function doesn't know what the memory will be used for,
   * and forcing all allocations to be 4MB-aligned has a cost, so let's
   * force MSAA resources to be at least 4MB, such that the allocation
   * logic can consider sub-4MB allocations to not require this 4MB alignment.
   */
   if (image->vk.samples > 1 &&
      pMemoryRequirements->memoryRequirements.size < D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT)
   }
      VKAPI_ATTR void VKAPI_CALL
   dzn_GetImageSubresourceLayout(VkDevice _device,
                     {
      VK_FROM_HANDLE(dzn_device, device, _device);
            if (image->desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
      assert(subresource->arrayLayer == 0);
   assert(subresource->mipLevel == 0);
   layout->offset = 0;
   layout->rowPitch = image->linear.row_stride;
   layout->depthPitch = 0;
   layout->arrayPitch = 0;
      } else {
      UINT subres_index =
      dzn_image_get_subresource_index(image, subresource,
      D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
   UINT num_rows;
   UINT64 row_size, total_size;
   ID3D12Device1_GetCopyableFootprints(device->dev, &image->desc,
                                    layout->offset = footprint.Offset;
   layout->rowPitch = footprint.Footprint.RowPitch;
   layout->depthPitch = layout->rowPitch * footprint.Footprint.Height;
   layout->arrayPitch = layout->depthPitch; // uuuh... why is this even here?
         }
      static D3D12_SHADER_COMPONENT_MAPPING
   translate_swizzle(VkComponentSwizzle in, uint32_t comp)
   {
      switch (in) {
   case VK_COMPONENT_SWIZZLE_IDENTITY:
      return (D3D12_SHADER_COMPONENT_MAPPING)
      case VK_COMPONENT_SWIZZLE_ZERO:
         case VK_COMPONENT_SWIZZLE_ONE:
         case VK_COMPONENT_SWIZZLE_R:
         case VK_COMPONENT_SWIZZLE_G:
         case VK_COMPONENT_SWIZZLE_B:
         case VK_COMPONENT_SWIZZLE_A:
         default: unreachable("Invalid swizzle");
      }
      static void
   dzn_image_view_prepare_srv_desc(struct dzn_image_view *iview)
   {
      struct dzn_physical_device *pdev =
         uint32_t plane_slice = (iview->vk.aspects & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) ==
         bool ms = iview->vk.image->samples > 1;
   uint32_t layers_per_elem =
      (iview->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE ||
   iview->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) ?
      bool from_3d_image = iview->vk.image->image_type == VK_IMAGE_TYPE_3D;
   bool use_array = iview->vk.base_array_layer > 0 ||
            iview->srv_desc = (D3D12_SHADER_RESOURCE_VIEW_DESC) {
      .Format =
      dzn_image_get_dxgi_format(pdev, iview->vk.format,
                  D3D12_SHADER_COMPONENT_MAPPING swz[] = {
      translate_swizzle(iview->vk.swizzle.r, 0),
   translate_swizzle(iview->vk.swizzle.g, 1),
   translate_swizzle(iview->vk.swizzle.b, 2),
               /* Swap components to fake B4G4R4A4 support. */
   if (iview->vk.format == VK_FORMAT_B4G4R4A4_UNORM_PACK16) {
      if (pdev->support_a4b4g4r4) {
      static const D3D12_SHADER_COMPONENT_MAPPING bgra4_remap[] = {
      D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
   D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
               for (uint32_t i = 0; i < ARRAY_SIZE(swz); i++)
      } else {
      static const D3D12_SHADER_COMPONENT_MAPPING bgra4_remap[] = {
      D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3,
   D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2,
   D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0,
               for (uint32_t i = 0; i < ARRAY_SIZE(swz); i++)
         } else if (iview->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      /* D3D puts stencil in G, not R. Requests for R should be routed to G and vice versa. */
   for (uint32_t i = 0; i < ARRAY_SIZE(swz); i++) {
      if (swz[i] == D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0)
         else if (swz[i] == D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1)
         } else if (iview->vk.view_format == VK_FORMAT_BC1_RGB_SRGB_BLOCK ||
            /* D3D has no opaque version of these; force alpha to 1 */
   for (uint32_t i = 0; i < ARRAY_SIZE(swz); i++) {
      if (swz[i] == D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3)
                  iview->srv_desc.Shader4ComponentMapping =
            switch (iview->vk.view_type) {
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
   case VK_IMAGE_VIEW_TYPE_1D:
      if (use_array) {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
   iview->srv_desc.Texture1DArray.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture1DArray.MipLevels = iview->vk.level_count;
   iview->srv_desc.Texture1DArray.FirstArraySlice = iview->vk.base_array_layer;
   iview->srv_desc.Texture1DArray.ArraySize = iview->vk.layer_count;
      } else {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
   iview->srv_desc.Texture1D.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture1D.MipLevels = iview->vk.level_count;
      }
         case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
   case VK_IMAGE_VIEW_TYPE_2D:
      if (from_3d_image) {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
   iview->srv_desc.Texture3D.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture3D.MipLevels = iview->vk.level_count;
      } else if (use_array && ms) {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
   iview->srv_desc.Texture2DMSArray.FirstArraySlice = iview->vk.base_array_layer;
      } else if (use_array && !ms) {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
   iview->srv_desc.Texture2DArray.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture2DArray.MipLevels = iview->vk.level_count;
   iview->srv_desc.Texture2DArray.FirstArraySlice = iview->vk.base_array_layer;
   iview->srv_desc.Texture2DArray.ArraySize = iview->vk.layer_count;
   iview->srv_desc.Texture2DArray.PlaneSlice = plane_slice;
      } else if (!use_array && ms) {
         } else {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
   iview->srv_desc.Texture2D.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture2D.MipLevels = iview->vk.level_count;
   iview->srv_desc.Texture2D.PlaneSlice = plane_slice;
      }
         case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
   case VK_IMAGE_VIEW_TYPE_CUBE:
      if (use_array) {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
   iview->srv_desc.TextureCubeArray.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.TextureCubeArray.MipLevels = iview->vk.level_count;
   iview->srv_desc.TextureCubeArray.First2DArrayFace = iview->vk.base_array_layer;
   iview->srv_desc.TextureCubeArray.NumCubes = iview->vk.layer_count / 6;
      } else {
      iview->srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
   iview->srv_desc.TextureCube.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.TextureCube.MipLevels = iview->vk.level_count;
      }
         case VK_IMAGE_VIEW_TYPE_3D:
      iview->srv_desc.ViewDimension =  D3D12_SRV_DIMENSION_TEXTURE3D;
   iview->srv_desc.Texture3D.MostDetailedMip = iview->vk.base_mip_level;
   iview->srv_desc.Texture3D.MipLevels = iview->vk.level_count;
   iview->srv_desc.Texture3D.ResourceMinLODClamp = 0.0f;
         default: unreachable("Invalid view type");
      }
      static void
   dzn_image_view_prepare_uav_desc(struct dzn_image_view *iview)
   {
      struct dzn_physical_device *pdev =
                           iview->uav_desc = (D3D12_UNORDERED_ACCESS_VIEW_DESC) {
      .Format =
      dzn_image_get_dxgi_format(pdev, iview->vk.format,
                  switch (iview->vk.view_type) {
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
      if (use_array) {
      iview->uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
   iview->uav_desc.Texture1DArray.MipSlice = iview->vk.base_mip_level;
   iview->uav_desc.Texture1DArray.FirstArraySlice = iview->vk.base_array_layer;
      } else {
      iview->uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
      }
         case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
   case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
      if (use_array) {
      iview->uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
   iview->uav_desc.Texture2DArray.PlaneSlice = 0;
   iview->uav_desc.Texture2DArray.MipSlice = iview->vk.base_mip_level;
   iview->uav_desc.Texture2DArray.FirstArraySlice = iview->vk.base_array_layer;
      } else {
      iview->uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
   iview->uav_desc.Texture2D.MipSlice = iview->vk.base_mip_level;
      }
      case VK_IMAGE_VIEW_TYPE_3D:
      iview->uav_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
   iview->uav_desc.Texture3D.MipSlice = iview->vk.base_mip_level;
   iview->uav_desc.Texture3D.FirstWSlice = 0;
   iview->uav_desc.Texture3D.WSize = iview->vk.extent.depth;
      default: unreachable("Invalid type");
      }
      static void
   dzn_image_view_prepare_rtv_desc(struct dzn_image_view *iview)
   {
      struct dzn_physical_device *pdev =
         bool use_array = iview->vk.base_array_layer > 0 || iview->vk.layer_count > 1;
   bool from_3d_image = iview->vk.image->image_type == VK_IMAGE_TYPE_3D;
   bool ms = iview->vk.image->samples > 1;
   uint32_t plane_slice =
      (iview->vk.aspects & VK_IMAGE_ASPECT_PLANE_2_BIT) ? 2 :
         iview->rtv_desc = (D3D12_RENDER_TARGET_VIEW_DESC) {
      .Format =
      dzn_image_get_dxgi_format(pdev, iview->vk.format,
                  switch (iview->vk.view_type) {
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
      if (use_array) {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
   iview->rtv_desc.Texture1DArray.MipSlice = iview->vk.base_mip_level;
   iview->rtv_desc.Texture1DArray.FirstArraySlice = iview->vk.base_array_layer;
      } else {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
      }
         case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
   case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
      if (from_3d_image) {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
   iview->rtv_desc.Texture3D.MipSlice = iview->vk.base_mip_level;
   iview->rtv_desc.Texture3D.FirstWSlice = iview->vk.base_array_layer;
      } else if (use_array && ms) {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
   iview->rtv_desc.Texture2DMSArray.FirstArraySlice = iview->vk.base_array_layer;
      } else if (use_array && !ms) {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
   iview->rtv_desc.Texture2DArray.MipSlice = iview->vk.base_mip_level;
   iview->rtv_desc.Texture2DArray.FirstArraySlice = iview->vk.base_array_layer;
   iview->rtv_desc.Texture2DArray.ArraySize = iview->vk.layer_count;
      } else if (!use_array && ms) {
         } else {
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
   iview->rtv_desc.Texture2D.MipSlice = iview->vk.base_mip_level;
      }
         case VK_IMAGE_VIEW_TYPE_3D:
      iview->rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
   iview->rtv_desc.Texture3D.MipSlice = iview->vk.base_mip_level;
   iview->rtv_desc.Texture3D.FirstWSlice = 0;
   iview->rtv_desc.Texture3D.WSize = iview->vk.extent.depth;
         default: unreachable("Invalid view type");
      }
      static void
   dzn_image_view_prepare_dsv_desc(struct dzn_image_view *iview)
   {
      struct dzn_physical_device *pdev =
         bool use_array = iview->vk.base_array_layer > 0 || iview->vk.layer_count > 1;
            iview->dsv_desc = (D3D12_DEPTH_STENCIL_VIEW_DESC) {
      .Format =
      dzn_image_get_dxgi_format(pdev, iview->vk.format,
                  switch (iview->vk.view_type) {
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
      if (use_array) {
      iview->dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
   iview->dsv_desc.Texture1DArray.MipSlice = iview->vk.base_mip_level;
   iview->dsv_desc.Texture1DArray.FirstArraySlice = iview->vk.base_array_layer;
      } else {
      iview->dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
      }
         case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
   case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
      if (use_array && ms) {
      iview->dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
   iview->dsv_desc.Texture2DMSArray.FirstArraySlice = iview->vk.base_array_layer;
      } else if (use_array && !ms) {
      iview->dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
   iview->dsv_desc.Texture2DArray.MipSlice = iview->vk.base_mip_level;
   iview->dsv_desc.Texture2DArray.FirstArraySlice = iview->vk.base_array_layer;
      } else if (!use_array && ms) {
         } else {
      iview->dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
      }
         default: unreachable("Invalid view type");
      }
      void
   dzn_image_view_finish(struct dzn_image_view *iview)
   {
         }
      void
   dzn_image_view_init(struct dzn_device *device,
               {
               const VkImageSubresourceRange *range = &pCreateInfo->subresourceRange;
                     assert(layer_count > 0);
            /* View usage should be a subset of image usage */
   assert(iview->vk.usage & (VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                    /* We remove this bit on depth textures, so skip creating a UAV for those */
   if ((iview->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) &&
      !(image->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS))
         switch (image->vk.image_type) {
   default:
         case VK_IMAGE_TYPE_1D:
   case VK_IMAGE_TYPE_2D:
      assert(range->baseArrayLayer + dzn_get_layer_count(image, range) - 1 <= image->vk.array_layers);
      case VK_IMAGE_TYPE_3D:
      assert(range->baseArrayLayer + dzn_get_layer_count(image, range) - 1
                              if (iview->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT)
            if (iview->vk.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
            if (iview->vk.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
      }
      static void
   dzn_image_view_destroy(struct dzn_image_view *iview,
         {
      if (!iview)
                     dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, iview->srv_bindless_slot);
            vk_image_view_finish(&iview->vk);
      }
      static VkResult
   dzn_image_view_create(struct dzn_device *device,
                     {
      VK_FROM_HANDLE(dzn_image, image, pCreateInfo->image);
   struct dzn_image_view *iview =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*iview), 8,
      if (!iview)
                     iview->srv_bindless_slot = iview->uav_bindless_slot = -1;
   if (device->bindless) {
      if (!(image->desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
      iview->srv_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (iview->srv_bindless_slot < 0) {
      dzn_image_view_destroy(iview, pAllocator);
               dzn_descriptor_heap_write_image_view_desc(device,
                        }
   if (iview->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) {
      iview->uav_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (iview->uav_bindless_slot < 0) {
      dzn_image_view_destroy(iview, pAllocator);
               dzn_descriptor_heap_write_image_view_desc(device,
                                    *out = dzn_image_view_to_handle(iview);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateImageView(VkDevice device,
                     {
      return dzn_image_view_create(dzn_device_from_handle(device), pCreateInfo,
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyImageView(VkDevice device,
               {
         }
      static void
   dzn_buffer_view_destroy(struct dzn_buffer_view *bview,
         {
      if (!bview)
                     dzn_device_descriptor_heap_free_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, bview->srv_bindless_slot);
            vk_object_base_finish(&bview->base);
      }
      static VkResult
   dzn_buffer_view_create(struct dzn_device *device,
                     {
               struct dzn_buffer_view *bview =
      vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*bview), 8,
      if (!bview)
                     enum pipe_format pfmt = vk_format_to_pipe_format(pCreateInfo->format);
   unsigned blksz = util_format_get_blocksize(pfmt);
   VkDeviceSize size =
      pCreateInfo->range == VK_WHOLE_SIZE ?
         bview->buffer = buf;
   bview->srv_bindless_slot = bview->uav_bindless_slot = -1;
   if (buf->usage &
      (VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT |
   VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)) {
   bview->srv_desc = (D3D12_SHADER_RESOURCE_VIEW_DESC) {
      .Format = dzn_buffer_get_dxgi_format(pCreateInfo->format),
   .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
   .Shader4ComponentMapping =
         .Buffer = {
      .FirstElement = pCreateInfo->offset / blksz,
   .NumElements = (UINT)(size / blksz),
                  if (device->bindless) {
      bview->srv_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (bview->srv_bindless_slot < 0) {
      dzn_buffer_view_destroy(bview, pAllocator);
      }
   dzn_descriptor_heap_write_buffer_view_desc(device, &device->device_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].heap,
                  if (buf->usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT) {
      bview->uav_desc = (D3D12_UNORDERED_ACCESS_VIEW_DESC) {
      .Format = dzn_buffer_get_dxgi_format(pCreateInfo->format),
   .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
   .Buffer = {
      .FirstElement = pCreateInfo->offset / blksz,
   .NumElements = (UINT)(size / blksz),
                  if (device->bindless) {
      bview->uav_bindless_slot = dzn_device_descriptor_heap_alloc_slot(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
   if (bview->uav_bindless_slot < 0) {
      dzn_buffer_view_destroy(bview, pAllocator);
      }
   dzn_descriptor_heap_write_buffer_view_desc(device, &device->device_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].heap,
                  *out = dzn_buffer_view_to_handle(bview);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_CreateBufferView(VkDevice device,
                     {
      return dzn_buffer_view_create(dzn_device_from_handle(device),
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_DestroyBufferView(VkDevice device,
               {
         }
