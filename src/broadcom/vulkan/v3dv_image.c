   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
      #include "drm-uapi/drm_fourcc.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "vk_util.h"
   #include "vulkan/wsi/wsi_common.h"
   #ifdef ANDROID
   #include "vk_android.h"
   #endif
      /**
   * Computes the HW's UIFblock padding for a given height/cpp.
   *
   * The goal of the padding is to keep pages of the same color (bank number) at
   * least half a page away from each other vertically when crossing between
   * columns of UIF blocks.
   */
   static uint32_t
   v3d_get_ub_pad(uint32_t cpp, uint32_t height)
   {
      uint32_t utile_h = v3d_utile_height(cpp);
   uint32_t uif_block_h = utile_h * 2;
                     /* For the perfectly-aligned-for-UIF-XOR case, don't add any pad. */
   if (height_offset_in_pc == 0)
            /* Try padding up to where we're offset by at least half a page. */
   if (height_offset_in_pc < PAGE_UB_ROWS_TIMES_1_5) {
      /* If we fit entirely in the page cache, don't pad. */
   if (height_ub < PAGE_CACHE_UB_ROWS)
         else
               /* If we're close to being aligned to page cache size, then round up
   * and rely on XOR.
   */
   if (height_offset_in_pc > PAGE_CACHE_MINUS_1_5_UB_ROWS)
            /* Otherwise, we're far enough away (top and bottom) to not need any
   * padding.
   */
      }
      /**
   * Computes the dimension with required padding for mip levels.
   *
   * This padding is required for width and height dimensions when the mip
   * level is greater than 1, and for the depth dimension when the mip level
   * is greater than 0. This function expects to be passed a mip level >= 1.
   *
   * Note: Hardware documentation seems to suggest that the third argument
   * should be the utile dimensions, but through testing it was found that
   * the block dimension should be used instead.
   */
   static uint32_t
   v3d_get_dimension_mpad(uint32_t dimension, uint32_t level, uint32_t block_dimension)
   {
      assert(level >= 1);
   uint32_t pot_dim = u_minify(dimension, 1);
   pot_dim = util_next_power_of_two(DIV_ROUND_UP(pot_dim, block_dimension));
   uint32_t padded_dim = block_dimension * pot_dim;
      }
      static bool
   v3d_setup_plane_slices(struct v3dv_image *image, uint8_t plane,
               {
               uint32_t width = image->planes[plane].width;
   uint32_t height = image->planes[plane].height;
            uint32_t utile_w = v3d_utile_width(image->planes[plane].cpp);
   uint32_t utile_h = v3d_utile_height(image->planes[plane].cpp);
   uint32_t uif_block_w = utile_w * 2;
            uint32_t block_width = vk_format_get_blockwidth(image->vk.format);
            /* Note that power-of-two padding is based on level 1.  These are not
   * equivalent to just util_next_power_of_two(dimension), because at a
   * level 0 dimension of 9, the level 1 power-of-two padded value is 4,
   * not 8. Additionally the pot padding is based on the block size.
   */
   uint32_t pot_width = 2 * v3d_get_dimension_mpad(width,
               uint32_t pot_height = 2 * v3d_get_dimension_mpad(height,
               uint32_t pot_depth = 2 * v3d_get_dimension_mpad(depth,
                  assert(image->vk.samples == VK_SAMPLE_COUNT_1_BIT ||
                           assert(image->vk.array_layers > 0);
   assert(depth > 0);
            /* Texture Base Address needs to be 64-byte aligned. If we have an explicit
   * plane layout we will return false to fail image creation with appropriate
   * error code.
   */
   uint32_t offset;
   if (plane_layouts) {
      offset = plane_layouts[plane].offset;
   if (offset % 64 != 0)
      } else {
         }
            for (int32_t i = image->vk.mip_levels - 1; i >= 0; i--) {
               slice->width = u_minify(width, i);
            uint32_t level_width, level_height, level_depth;
   if (i < 2) {
      level_width = slice->width;
      } else {
      level_width = u_minify(pot_width, i);
               if (i < 1)
         else
            if (msaa) {
      level_width *= 2;
               level_width = DIV_ROUND_UP(level_width, block_width);
            if (!image->tiled) {
      slice->tiling = V3D_TILING_RASTER;
   if (image->vk.image_type == VK_IMAGE_TYPE_1D)
      } else {
      if ((i != 0 || !uif_top) &&
      (level_width <= utile_w || level_height <= utile_h)) {
   slice->tiling = V3D_TILING_LINEARTILE;
   level_width = align(level_width, utile_w);
      } else if ((i != 0 || !uif_top) && level_width <= uif_block_w) {
      slice->tiling = V3D_TILING_UBLINEAR_1_COLUMN;
   level_width = align(level_width, uif_block_w);
      } else if ((i != 0 || !uif_top) && level_width <= 2 * uif_block_w) {
      slice->tiling = V3D_TILING_UBLINEAR_2_COLUMN;
   level_width = align(level_width, 2 * uif_block_w);
      } else {
      /* We align the width to a 4-block column of UIF blocks, but we
   * only align height to UIF blocks.
   */
                  slice->ub_pad = v3d_get_ub_pad(image->planes[plane].cpp,
                  /* If the padding set us to to be aligned to the page cache size,
   * then the HW will use the XOR bit on odd columns to get us
   * perfectly misaligned.
   */
   if ((level_height / uif_block_h) %
      (V3D_PAGE_CACHE_SIZE / V3D_UIFBLOCK_ROW_SIZE) == 0) {
      } else {
                        slice->offset = offset;
            /* We assume that rowPitch in the plane layout refers to level 0 */
   if (plane_layouts && i == 0) {
      if (plane_layouts[plane].rowPitch < slice->stride)
         if (plane_layouts[plane].rowPitch % image->planes[plane].cpp)
         if (image->tiled && (plane_layouts[plane].rowPitch % (4 * uif_block_w)))
                     slice->padded_height = level_height;
   if (slice->tiling == V3D_TILING_UIF_NO_XOR ||
      slice->tiling == V3D_TILING_UIF_XOR) {
   slice->padded_height_of_output_image_in_uif_blocks =
      slice->padded_height /
            slice->size = level_height * slice->stride;
            /* The HW aligns level 1's base to a page if any of level 1 or
   * below could be UIF XOR.  The lower levels then inherit the
   * alignment for as long as necessary, thanks to being power of
   * two aligned.
   */
   if (i == 1 &&
      level_width > 4 * uif_block_w &&
   level_height > PAGE_CACHE_MINUS_1_5_UB_ROWS * uif_block_h) {
                                    /* UIF/UBLINEAR levels need to be aligned to UIF-blocks, and LT only
   * needs to be aligned to utile boundaries.  Since tiles are laid out
   * from small to big in memory, we need to align the later UIF slices
   * to UIF blocks, if they were preceded by non-UIF-block-aligned LT
   * slices.
   *
   * We additionally align to 4k, which improves UIF XOR performance.
   *
   * Finally, because the Texture Base Address field must be 64-byte aligned,
   * we also need to align linear images to 64 if the image is going to be
   * used for transfer.
   */
   if (image->tiled) {
         } else {
      image->planes[plane].alignment =
      (image->vk.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) ?
            uint32_t align_offset =
      align(image->planes[plane].slices[0].offset,
            if (align_offset) {
      image->planes[plane].size += align_offset;
   for (int i = 0; i < image->vk.mip_levels; i++)
               /* Arrays and cube textures have a stride which is the distance from
   * one full mipmap tree to the next (64b aligned).  For 3D textures,
   * we need to program the stride between slices of miplevel 0.
   */
   if (image->vk.image_type != VK_IMAGE_TYPE_3D) {
      image->planes[plane].cube_map_stride =
                  if (plane_layouts && image->vk.array_layers > 1) {
      if (plane_layouts[plane].arrayPitch % 64 != 0)
         if (plane_layouts[plane].arrayPitch <
      image->planes[plane].cube_map_stride) {
      }
               image->planes[plane].size += image->planes[plane].cube_map_stride *
      } else {
      image->planes[plane].cube_map_stride = image->planes[plane].slices[0].size;
   if (plane_layouts) {
      /* We assume that depthPitch in the plane layout refers to level 0 */
   if (plane_layouts[plane].depthPitch !=
      image->planes[plane].slices[0].size) {
                        }
      static bool
   v3d_setup_slices(struct v3dv_image *image, bool disjoint,
         {
      if (disjoint && image->plane_count == 1)
            uint32_t offset = 0;
   for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      offset = disjoint ? 0 : offset;
   if (!v3d_setup_plane_slices(image, plane, offset, plane_layouts)) {
      assert(plane_layouts);
      }
               image->non_disjoint_size = disjoint ? 0 : offset;
      }
      uint32_t
   v3dv_layer_offset(const struct v3dv_image *image, uint32_t level, uint32_t layer,
         {
               if (image->vk.image_type == VK_IMAGE_TYPE_3D)
         else
      return image->planes[plane].mem_offset + slice->offset +
   }
      VkResult
   v3dv_update_image_layout(struct v3dv_device *device,
                           {
      assert(!explicit_mod_info ||
            assert(!explicit_mod_info ||
                              bool ok =
      v3d_setup_slices(image, disjoint,
      if (!ok) {
      assert(explicit_mod_info);
                  }
      VkResult
   v3dv_image_init(struct v3dv_device *device,
                     {
      /* When using the simulator the WSI common code will see that our
   * driver wsi device doesn't match the display device and because of that
   * it will not attempt to present directly from the swapchain images,
   * instead it will use the prime blit path (use_buffer_blit flag in
   * struct wsi_swapchain), where it copies the contents of the swapchain
   * images to a linear buffer with appropriate row stride for presentation.
   * As a result, on that path, swapchain images do not have any special
   * requirements and are not created with the pNext structs below.
   */
   VkImageTiling tiling = pCreateInfo->tiling;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
   const VkImageDrmFormatModifierListCreateInfoEXT *mod_info = NULL;
      #ifdef ANDROID
      if (image->is_native_buffer_memory) {
      assert(image->android_explicit_layout);
   explicit_mod_info = image->android_explicit_layout;
         #endif
      if (tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      mod_info =
      vk_find_struct_const(pCreateInfo->pNext,
      explicit_mod_info =
      vk_find_struct_const(pCreateInfo->pNext,
               if (mod_info) {
      for (uint32_t i = 0; i < mod_info->drmFormatModifierCount; i++) {
      switch (mod_info->pDrmFormatModifiers[i]) {
   case DRM_FORMAT_MOD_LINEAR:
      if (modifier == DRM_FORMAT_MOD_INVALID)
            case DRM_FORMAT_MOD_BROADCOM_UIF:
      modifier = DRM_FORMAT_MOD_BROADCOM_UIF;
            } else {
         }
   assert(modifier == DRM_FORMAT_MOD_LINEAR ||
      } else if (pCreateInfo->imageType == VK_IMAGE_TYPE_1D ||
                        if (modifier == DRM_FORMAT_MOD_INVALID)
      modifier = (tiling == VK_IMAGE_TILING_OPTIMAL) ? DRM_FORMAT_MOD_BROADCOM_UIF
         const struct v3dv_format *format =
                  assert(pCreateInfo->samples == VK_SAMPLE_COUNT_1_BIT ||
                              const struct vk_format_ycbcr_info *ycbcr_info =
            for (uint8_t plane = 0; plane < image->plane_count; plane++) {
      VkFormat plane_format =
         image->planes[plane].cpp =
                  image->planes[plane].width = image->vk.extent.width;
            if (ycbcr_info) {
                     image->planes[plane].height /=
                  /* Our meta paths can create image views with compatible formats for any
   * image, so always set this flag to keep the common Vulkan image code
   * happy.
   */
         #ifdef ANDROID
      /* At this time, an AHB handle is not yet provided.
   * Image layout will be filled up during vkBindImageMemory2
   */
   if (image->is_ahb)
      #endif
                  return v3dv_update_image_layout(device, image, modifier, disjoint,
      }
      static VkResult
   create_image(struct v3dv_device *device,
               const VkImageCreateInfo *pCreateInfo,
   {
      VkResult result;
            image = vk_image_create(&device->vk, pCreateInfo, pAllocator, sizeof(*image));
   if (image == NULL)
         #ifdef ANDROID
      const VkExternalMemoryImageCreateInfo *external_info =
            const VkNativeBufferANDROID *native_buffer =
            if (native_buffer != NULL)
            image->is_ahb = external_info && (external_info->handleTypes &
                     if (image->is_ahb || image->is_native_buffer_memory) {
      image->android_explicit_layout = vk_alloc2(&device->vk.alloc, pAllocator,
                     if (!image->android_explicit_layout) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               image->android_plane_layouts = vk_alloc2(&device->vk.alloc, pAllocator,
      sizeof(VkSubresourceLayout) * V3DV_MAX_PLANE_COUNT,
      if (!image->android_plane_layouts) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
                  if (image->is_native_buffer_memory) {
      struct u_gralloc_buffer_handle gr_handle = {
      .handle = native_buffer->handle,
   .hal_format = native_buffer->format,
               result = v3dv_gralloc_to_drm_explicit_layout(device->gralloc,
                           if (result != VK_SUCCESS)
         #endif
         result = v3dv_image_init(device, pCreateInfo, pAllocator, image);
   if (result != VK_SUCCESS)
         #ifdef ANDROID
      if (image->is_native_buffer_memory) {
      result = v3dv_import_native_buffer_fd(v3dv_device_to_handle(device),
               if (result != VK_SUCCESS)
         #endif
                        fail:
   #ifdef ANDROID
      if (image->android_explicit_layout)
         if (image->android_plane_layouts)
      #endif
         vk_image_destroy(&device->vk, pAllocator, &image->vk);
      }
      static VkResult
   create_image_from_swapchain(struct v3dv_device *device,
                           {
      struct v3dv_image *swapchain_image =
                  VkImageCreateInfo local_create_info = *pCreateInfo;
            /* Added by wsi code. */
            /* The spec requires TILING_OPTIMAL as input, but the swapchain image may
   * privately use a different tiling.  See spec anchor
   * #swapchain-wsi-image-create-info .
   */
   assert(local_create_info.tiling == VK_IMAGE_TILING_OPTIMAL);
            VkImageDrmFormatModifierListCreateInfoEXT local_modifier_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_DRM_FORMAT_MODIFIER_LIST_CREATE_INFO_EXT,
   .drmFormatModifierCount = 1,
               if (swapchain_image->vk.drm_format_mod != DRM_FORMAT_MOD_INVALID)
            assert(swapchain_image->vk.image_type == local_create_info.imageType);
   assert(swapchain_image->vk.format == local_create_info.format);
   assert(swapchain_image->vk.extent.width == local_create_info.extent.width);
   assert(swapchain_image->vk.extent.height == local_create_info.extent.height);
   assert(swapchain_image->vk.extent.depth == local_create_info.extent.depth);
   assert(swapchain_image->vk.array_layers == local_create_info.arrayLayers);
   assert(swapchain_image->vk.samples == local_create_info.samples);
   assert(swapchain_image->vk.tiling == local_create_info.tiling);
   assert((swapchain_image->vk.usage & local_create_info.usage) ==
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateImage(VkDevice _device,
                     {
            #ifdef ANDROID
      /* VkImageSwapchainCreateInfoKHR is not useful at all */
      #else
      const VkImageSwapchainCreateInfoKHR *swapchain_info =
      #endif
         if (swapchain_info && swapchain_info->swapchain != VK_NULL_HANDLE)
      return create_image_from_swapchain(device, pCreateInfo, swapchain_info,
            }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_GetImageSubresourceLayout(VkDevice device,
                     {
               uint8_t plane = v3dv_plane_from_aspect(subresource->aspectMask);
   const struct v3d_resource_slice *slice =
            /* About why the offset below works for both disjoint and non-disjoint
   * cases, from the Vulkan spec:
   *
   *   "If the image is disjoint, then the offset is relative to the base
   *    address of the plane."
   *
   *   "If the image is non-disjoint, then the offset is relative to the base
   *    address of the image."
   *
   * In our case, the per-plane mem_offset for non-disjoint images is the
   * same for all planes and matches the base address of the image.
   */
   layout->offset =
      v3dv_layer_offset(image, subresource->mipLevel, subresource->arrayLayer,
      layout->rowPitch = slice->stride;
   layout->depthPitch = image->vk.image_type == VK_IMAGE_TYPE_3D ?
         layout->arrayPitch = image->vk.array_layers > 1 ?
            if (image->vk.image_type != VK_IMAGE_TYPE_3D) {
         } else {
      /* For 3D images, the size of the slice represents the size of a 2D slice
   * in the 3D image, so we have to multiply by the depth extent of the
   * miplevel. For levels other than the first, we just compute the size
   * as the distance between consecutive levels (notice that mip levels are
   * arranged in memory from last to first).
   */
   if (subresource->mipLevel == 0) {
         } else {
         const struct v3d_resource_slice *prev_slice =
               }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyImage(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (image == NULL)
            /* If we have created a shadow tiled image for this image we must also free
   * it (along with its memory allocation).
   */
   if (image->shadow) {
      bool disjoint = image->vk.create_flags & VK_IMAGE_CREATE_DISJOINT_BIT;
   for (int i = 0; i < (disjoint ? image->plane_count : 1); i++) {
      if (image->shadow->planes[i].mem) {
      v3dv_FreeMemory(_device,
               }
   v3dv_DestroyImage(_device, v3dv_image_to_handle(image->shadow),
                  #ifdef ANDROID
      if (image->is_native_buffer_memory)
      v3dv_FreeMemory(_device,
               if (image->android_explicit_layout)
         if (image->android_plane_layouts)
      #endif
            }
      VkImageViewType
   v3dv_image_type_to_view_type(VkImageType type)
   {
      switch (type) {
   case VK_IMAGE_TYPE_1D: return VK_IMAGE_VIEW_TYPE_1D;
   case VK_IMAGE_TYPE_2D: return VK_IMAGE_VIEW_TYPE_2D;
   case VK_IMAGE_TYPE_3D: return VK_IMAGE_VIEW_TYPE_3D;
   default:
            }
      static VkResult
   create_image_view(struct v3dv_device *device,
                     bool driver_internal,
   {
      V3DV_FROM_HANDLE(v3dv_image, image, pCreateInfo->image);
            iview = vk_image_view_create(&device->vk, driver_internal, pCreateInfo,
         if (iview == NULL)
            const VkImageAspectFlagBits any_plane_aspect =
      VK_IMAGE_ASPECT_PLANE_0_BIT |
   VK_IMAGE_ASPECT_PLANE_1_BIT |
         if (image->vk.aspects & any_plane_aspect) {
      assert((image->vk.aspects & ~any_plane_aspect) == 0);
   iview->plane_count = 0;
   static const VkImageAspectFlagBits plane_aspects[]= {
      VK_IMAGE_ASPECT_PLANE_0_BIT,
   VK_IMAGE_ASPECT_PLANE_1_BIT,
      };
   for (uint8_t plane = 0; plane < V3DV_MAX_PLANE_COUNT; plane++) {
      if (iview->vk.aspects & plane_aspects[plane])
         } else {
      iview->plane_count = 1;
      }
   /* At this point we should have at least one plane */
                     /* If we have D24S8 format but the view only selects the stencil aspect
   * we want to re-interpret the format as RGBA8_UINT, then map our stencil
   * data reads to the R component and ignore the GBA channels that contain
   * the depth aspect data.
   *
   * FIXME: thwe code belows calls vk_component_mapping_to_pipe_swizzle
   * only so it can then call util_format_compose_swizzles later. Maybe it
   * makes sense to implement swizzle composition using VkSwizzle directly.
   */
   VkFormat format;
   if (image->vk.format == VK_FORMAT_D24_UNORM_S8_UINT &&
      range->aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT) {
   format = VK_FORMAT_R8G8B8A8_UINT;
   uint8_t stencil_aspect_swizzle[4] = {
         };
   uint8_t view_swizzle[4];
            util_format_compose_swizzles(stencil_aspect_swizzle, view_swizzle,
      } else {
      format = iview->vk.format;
   vk_component_mapping_to_pipe_swizzle(iview->vk.swizzle,
               iview->vk.view_format = format;
   iview->format = v3dv_X(device, get_format)(format);
            for (uint8_t plane = 0; plane < iview->plane_count; plane++) {
      iview->planes[plane].offset = v3dv_layer_offset(image,
                        if (vk_format_is_depth_or_stencil(iview->vk.view_format)) {
      iview->planes[plane].internal_type =
      } else {
      v3dv_X(device, get_internal_type_bpp_for_output_format)
      (iview->format->planes[plane].rt_type,
   &iview->planes[plane].internal_type,
            const uint8_t *format_swizzle =
         util_format_compose_swizzles(format_swizzle, iview->view_swizzle,
            iview->planes[plane].swap_rb = v3dv_format_swizzle_needs_rb_swap(format_swizzle);
                                    }
      VkResult
   v3dv_create_image_view(struct v3dv_device *device,
               {
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateImageView(VkDevice _device,
                     {
                  }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyImageView(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (image_view == NULL)
            if (image_view->shadow) {
      v3dv_DestroyImageView(_device,
                              }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_CreateBufferView(VkDevice _device,
                     {
               struct v3dv_buffer *buffer =
            struct v3dv_buffer_view *view =
      vk_object_zalloc(&device->vk, pAllocator, sizeof(*view),
      if (!view)
            uint32_t range;
   if (pCreateInfo->range == VK_WHOLE_SIZE)
         else
            enum pipe_format pipe_format = vk_format_to_pipe_format(pCreateInfo->format);
            view->buffer = buffer;
   view->offset = pCreateInfo->offset;
   view->size = view->offset + range;
   view->num_elements = num_elements;
   view->vk_format = pCreateInfo->format;
            /* We don't support multi-plane formats for buffer views */
   assert(view->format->plane_count == 1);
   v3dv_X(device, get_internal_type_bpp_for_output_format)
            if (buffer->usage & VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT ||
      buffer->usage & VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT)
                     }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_DestroyBufferView(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
            if (buffer_view == NULL)
               }
