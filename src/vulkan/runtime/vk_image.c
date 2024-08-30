   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_image.h"
      #ifndef _WIN32
   #include <drm-uapi/drm_fourcc.h>
   #endif
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_format.h"
   #include "vk_format_info.h"
   #include "vk_log.h"
   #include "vk_physical_device.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
   #include "vulkan/wsi/wsi_common.h"
      #ifdef ANDROID
   #include "vk_android.h"
   #include <vulkan/vulkan_android.h>
   #endif
      void
   vk_image_init(struct vk_device *device,
               {
               assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
   assert(pCreateInfo->mipLevels > 0);
   assert(pCreateInfo->arrayLayers > 0);
   assert(pCreateInfo->samples > 0);
   assert(pCreateInfo->extent.width > 0);
   assert(pCreateInfo->extent.height > 0);
            if (pCreateInfo->flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
         if (pCreateInfo->flags & VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT)
            image->create_flags = pCreateInfo->flags;
   image->image_type = pCreateInfo->imageType;
   vk_image_set_format(image, pCreateInfo->format);
   image->extent = vk_image_sanitize_extent(image, pCreateInfo->extent);
   image->mip_levels = pCreateInfo->mipLevels;
   image->array_layers = pCreateInfo->arrayLayers;
   image->samples = pCreateInfo->samples;
   image->tiling = pCreateInfo->tiling;
            if (image->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      const VkImageStencilUsageCreateInfo *stencil_usage_info =
      vk_find_struct_const(pCreateInfo->pNext,
      image->stencil_usage =
      stencil_usage_info ? stencil_usage_info->stencilUsage :
   } else {
                  const VkExternalMemoryImageCreateInfo *ext_mem_info =
         if (ext_mem_info)
         else
            const struct wsi_image_create_info *wsi_info =
               #ifndef _WIN32
         #endif
      #ifdef ANDROID
      const VkExternalFormatANDROID *ext_format =
         if (ext_format && ext_format->externalFormat != 0) {
      assert(image->format == VK_FORMAT_UNDEFINED);
   assert(image->external_handle_types &
                        #endif
   }
      void *
   vk_image_create(struct vk_device *device,
                     {
      struct vk_image *image =
      vk_zalloc2(&device->alloc, alloc, size, 8,
      if (image == NULL)
                        }
      void
   vk_image_finish(struct vk_image *image)
   {
         }
      void
   vk_image_destroy(struct vk_device *device,
               {
         }
      #ifndef _WIN32
   VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetImageDrmFormatModifierPropertiesEXT(UNUSED VkDevice device,
               {
               assert(pProperties->sType ==
            assert(image->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT);
               }
   #endif
      VKAPI_ATTR void VKAPI_CALL
   vk_common_GetImageSubresourceLayout(VkDevice _device, VkImage _image,
               {
               const VkImageSubresource2KHR subresource = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_SUBRESOURCE_2_KHR,
               VkSubresourceLayout2KHR layout = {
                  device->dispatch_table.GetImageSubresourceLayout2KHR(_device, _image,
               }
      void
   vk_image_set_format(struct vk_image *image, VkFormat format)
   {
      image->format = format;
      }
      VkImageUsageFlags
   vk_image_usage(const struct vk_image *image,
         {
      /* From the Vulkan 1.2.131 spec:
   *
   *    "If the image was has a depth-stencil format and was created with
   *    a VkImageStencilUsageCreateInfo structure included in the pNext
   *    chain of VkImageCreateInfo, the usage is calculated based on the
   *    subresource.aspectMask provided:
   *
   *     - If aspectMask includes only VK_IMAGE_ASPECT_STENCIL_BIT, the
   *       implicit usage is equal to
   *       VkImageStencilUsageCreateInfo::stencilUsage.
   *
   *     - If aspectMask includes only VK_IMAGE_ASPECT_DEPTH_BIT, the
   *       implicit usage is equal to VkImageCreateInfo::usage.
   *
   *     - If both aspects are included in aspectMask, the implicit usage
   *       is equal to the intersection of VkImageCreateInfo::usage and
   *       VkImageStencilUsageCreateInfo::stencilUsage.
   */
   if (aspect_mask == VK_IMAGE_ASPECT_STENCIL_BIT) {
         } else if (aspect_mask == (VK_IMAGE_ASPECT_DEPTH_BIT |
               } else {
      /* This also handles the color case */
         }
      #define VK_IMAGE_ASPECT_ANY_COLOR_MASK_MESA ( \
      VK_IMAGE_ASPECT_COLOR_BIT | \
   VK_IMAGE_ASPECT_PLANE_0_BIT | \
   VK_IMAGE_ASPECT_PLANE_1_BIT | \
         /** Expands the given aspect mask relative to the image
   *
   * If the image has color plane aspects VK_IMAGE_ASPECT_COLOR_BIT has been
   * requested, this returns the aspects of the underlying image.
   *
   * For example,
   *
   *    VK_IMAGE_ASPECT_COLOR_BIT
   *
   * will be converted to
   *
   *    VK_IMAGE_ASPECT_PLANE_0_BIT |
   *    VK_IMAGE_ASPECT_PLANE_1_BIT |
   *    VK_IMAGE_ASPECT_PLANE_2_BIT
   *
   * for an image of format VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM.
   */
   VkImageAspectFlags
   vk_image_expand_aspect_mask(const struct vk_image *image,
         {
      if (aspect_mask == VK_IMAGE_ASPECT_COLOR_BIT) {
      assert(image->aspects & VK_IMAGE_ASPECT_ANY_COLOR_MASK_MESA);
      } else {
      assert(aspect_mask && !(aspect_mask & ~image->aspects));
         }
      VkExtent3D
   vk_image_extent_to_elements(const struct vk_image *image, VkExtent3D extent)
   {
      const struct util_format_description *fmt =
            extent = vk_image_sanitize_extent(image, extent);
   extent.width = DIV_ROUND_UP(extent.width, fmt->block.width);
   extent.height = DIV_ROUND_UP(extent.height, fmt->block.height);
               }
      VkOffset3D
   vk_image_offset_to_elements(const struct vk_image *image, VkOffset3D offset)
   {
      const struct util_format_description *fmt =
                     assert(offset.x % fmt->block.width == 0);
   assert(offset.y % fmt->block.height == 0);
            offset.x /= fmt->block.width;
   offset.y /= fmt->block.height;
               }
      struct vk_image_buffer_layout
   vk_image_buffer_copy_layout(const struct vk_image *image,
         {
               const uint32_t row_length = region->bufferRowLength ?
         const uint32_t image_height = region->bufferImageHeight ?
            const VkImageAspectFlags aspect = region->imageSubresource.aspectMask;
   VkFormat format = vk_format_get_aspect_format(image->format, aspect);
            assert(fmt->block.bits % 8 == 0);
            const uint32_t row_stride_B =
         const uint64_t image_stride_B =
            return (struct vk_image_buffer_layout) {
      .row_length = row_length,
   .image_height = image_height,
   .element_size_B = element_size_B,
   .row_stride_B = row_stride_B,
         }
      struct vk_image_buffer_layout
   vk_memory_to_image_copy_layout(const struct vk_image *image,
         {
      const VkBufferImageCopy2 bic = {
      .bufferOffset = 0,
   .bufferRowLength = region->memoryRowLength,
   .bufferImageHeight = region->memoryImageHeight,
   .imageSubresource = region->imageSubresource,
   .imageOffset = region->imageOffset,
      };
      }
      struct vk_image_buffer_layout
   vk_image_to_memory_copy_layout(const struct vk_image *image,
         {
      const VkBufferImageCopy2 bic = {
      .bufferOffset = 0,
   .bufferRowLength = region->memoryRowLength,
   .bufferImageHeight = region->memoryImageHeight,
   .imageSubresource = region->imageSubresource,
   .imageOffset = region->imageOffset,
      };
      }
      static VkComponentSwizzle
   remap_swizzle(VkComponentSwizzle swizzle, VkComponentSwizzle component)
   {
         }
      void
   vk_image_view_init(struct vk_device *device,
                     {
               assert(pCreateInfo->sType == VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
            image_view->create_flags = pCreateInfo->flags;
   image_view->image = image;
            image_view->format = pCreateInfo->format;
   if (image_view->format == VK_FORMAT_UNDEFINED)
            if (!driver_internal) {
      switch (image_view->view_type) {
   case VK_IMAGE_VIEW_TYPE_1D:
   case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
      assert(image->image_type == VK_IMAGE_TYPE_1D);
      case VK_IMAGE_VIEW_TYPE_2D:
   case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
      if (image->create_flags & (VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT |
               else
            case VK_IMAGE_VIEW_TYPE_3D:
      assert(image->image_type == VK_IMAGE_TYPE_3D);
      case VK_IMAGE_VIEW_TYPE_CUBE:
   case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
      assert(image->image_type == VK_IMAGE_TYPE_2D);
   assert(image->create_flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
      default:
                              if (driver_internal) {
      image_view->aspects = range->aspectMask;
      } else {
      image_view->aspects =
                     /* From the Vulkan 1.2.184 spec:
   *
   *    "If the image has a multi-planar format and
   *    subresourceRange.aspectMask is VK_IMAGE_ASPECT_COLOR_BIT, and image
   *    has been created with a usage value not containing any of the
   *    VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR,
   *    VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR,
   *    VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR,
   *    VK_IMAGE_USAGE_VIDEO_ENCODE_DST_BIT_KHR,
   *    VK_IMAGE_USAGE_VIDEO_ENCODE_SRC_BIT_KHR, and
   *    VK_IMAGE_USAGE_VIDEO_ENCODE_DPB_BIT_KHR flags, then the format must
   *    be identical to the image format, and the sampler to be used with the
   *    image view must enable sampler Y′CBCR conversion."
   *
   * Since no one implements video yet, we can ignore the bits about video
   * create flags and assume YCbCr formats match.
   */
   if ((image->aspects & VK_IMAGE_ASPECT_PLANE_1_BIT) &&
                  /* From the Vulkan 1.2.184 spec:
   *
   *    "Each depth/stencil format is only compatible with itself."
   */
   if (image_view->aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
                  if (!(image->create_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT))
            /* Restrict the format to only the planes chosen.
   *
   * For combined depth and stencil images, this means the depth-only or
   * stencil-only format if only one aspect is chosen and the full
   * combined format if both aspects are chosen.
   *
   * For single-plane color images, we just take the format as-is.  For
   * multi-plane views of multi-plane images, this means we want the full
   * multi-plane format.  For single-plane views of multi-plane images, we
   * want a format compatible with the one plane.  Fortunately, this is
   * already what the client gives us.  The Vulkan 1.2.184 spec says:
   *
   *    "If image was created with the VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
   *    and the image has a multi-planar format, and if
   *    subresourceRange.aspectMask is VK_IMAGE_ASPECT_PLANE_0_BIT,
   *    VK_IMAGE_ASPECT_PLANE_1_BIT, or VK_IMAGE_ASPECT_PLANE_2_BIT,
   *    format must be compatible with the corresponding plane of the
   *    image, and the sampler to be used with the image view must not
   *    enable sampler Y′CBCR conversion."
   */
   if (image_view->aspects == VK_IMAGE_ASPECT_STENCIL_BIT) {
         } else if (image_view->aspects == VK_IMAGE_ASPECT_DEPTH_BIT) {
         } else {
                     image_view->swizzle = (VkComponentMapping) {
      .r = remap_swizzle(pCreateInfo->components.r, VK_COMPONENT_SWIZZLE_R),
   .g = remap_swizzle(pCreateInfo->components.g, VK_COMPONENT_SWIZZLE_G),
   .b = remap_swizzle(pCreateInfo->components.b, VK_COMPONENT_SWIZZLE_B),
               assert(range->layerCount > 0);
            image_view->base_mip_level = range->baseMipLevel;
   image_view->level_count = vk_image_subresource_level_count(image, range);
   image_view->base_array_layer = range->baseArrayLayer;
            const VkImageViewMinLodCreateInfoEXT *min_lod_info =
                  /* From the Vulkan 1.3.215 spec:
   *
   *    VUID-VkImageViewMinLodCreateInfoEXT-minLod-06456
   *
   *    "minLod must be less or equal to the index of the last mipmap level
   *    accessible to the view."
   */
   assert(image_view->min_lod <= image_view->base_mip_level +
            image_view->extent =
            /* By default storage uses the same as the image properties, but it can be
   * overriden with VkImageViewSlicedCreateInfoEXT.
   */
   image_view->storage.z_slice_offset = 0;
            const VkImageViewSlicedCreateInfoEXT *sliced_info =
         assert(image_view->base_mip_level + image_view->level_count
         switch (image->image_type) {
   default:
         case VK_IMAGE_TYPE_1D:
   case VK_IMAGE_TYPE_2D:
      assert(image_view->base_array_layer + image_view->layer_count
            case VK_IMAGE_TYPE_3D:
      if (sliced_info && image_view->view_type == VK_IMAGE_VIEW_TYPE_3D) {
      unsigned total = image_view->extent.depth;
   image_view->storage.z_slice_offset = sliced_info->sliceOffset;
   assert(image_view->storage.z_slice_offset < total);
   if (sliced_info->sliceCount == VK_REMAINING_3D_SLICES_EXT) {
         } else {
            } else if (image_view->view_type != VK_IMAGE_VIEW_TYPE_3D) {
      image_view->storage.z_slice_offset = image_view->base_array_layer;
      }
   assert(image_view->storage.z_slice_offset + image_view->storage.z_slice_count
         assert(image_view->base_array_layer + image_view->layer_count
                     /* If we are creating a color view from a depth/stencil image we compute
   * usage from the underlying depth/stencil aspects.
   */
   const VkImageUsageFlags image_usage =
         const VkImageViewUsageCreateInfo *usage_info =
         image_view->usage = usage_info ? usage_info->usage : image_usage;
      }
      void
   vk_image_view_finish(struct vk_image_view *image_view)
   {
         }
      void *
   vk_image_view_create(struct vk_device *device,
                           {
      struct vk_image_view *image_view =
      vk_zalloc2(&device->alloc, alloc, size, 8,
      if (image_view == NULL)
                        }
      void
   vk_image_view_destroy(struct vk_device *device,
               {
         }
      bool
   vk_image_layout_is_read_only(VkImageLayout layout,
         {
               switch (layout) {
   case VK_IMAGE_LAYOUT_UNDEFINED:
   case VK_IMAGE_LAYOUT_PREINITIALIZED:
            case VK_IMAGE_LAYOUT_GENERAL:
   case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
   case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
   case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
   case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
   case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
   case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
   case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_MAX_ENUM:
   case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
   case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR:
      #ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR:
   case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR:
      #endif
                        }
      bool
   vk_image_layout_is_depth_only(VkImageLayout layout)
   {
      switch (layout) {
   case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
   case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
            default:
            }
      static VkResult
   vk_image_create_get_format_list_uncompressed(struct vk_device *device,
                           {
      const struct vk_format_class_info *class =
            *formats = NULL;
            if (class->format_count < 2)
            *formats = vk_alloc2(&device->alloc, pAllocator,
               if (*formats == NULL)
            memcpy(*formats, class->formats, sizeof(VkFormat) * class->format_count);
               }
      static VkResult
   vk_image_create_get_format_list_compressed(struct vk_device *device,
                           {
      if ((pCreateInfo->flags & VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT) == 0) {
      return vk_image_create_get_format_list_uncompressed(device,
                                 const struct vk_format_class_info *class =
                  switch (vk_format_get_blocksizebits(pCreateInfo->format)) {
   case 64:
      uncompr_class = vk_format_class_get_info(MESA_VK_FORMAT_CLASS_64_BIT);
      case 128:
      uncompr_class = vk_format_class_get_info(MESA_VK_FORMAT_CLASS_128_BIT);
               if (!uncompr_class)
                     *formats = vk_alloc2(&device->alloc, pAllocator,
               if (*formats == NULL)
            memcpy(*formats, class->formats, sizeof(VkFormat) * class->format_count);
   memcpy(*formats + class->format_count, uncompr_class->formats,
                     }
      /* Get a list of compatible formats when VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
   * or VK_IMAGE_CREATE_BLOCK_TEXEL_VIEW_COMPATIBLE_BIT is set. This list is
   * either retrieved from a VkImageFormatListCreateInfo passed to the creation
   * chain, or forged from the default compatible list specified in the
   * "formats-compatibility-classes" section of the spec.
   *
   * The value returned in *formats must be freed with
   * vk_free2(&device->alloc, pAllocator), and should not live past the
   * vkCreateImage() call (allocated in the COMMAND scope).
   */
   VkResult
   vk_image_create_get_format_list(struct vk_device *device,
                           {
      *formats = NULL;
            if (!(pCreateInfo->flags &
         (VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT |
                  /* "Each depth/stencil format is only compatible with itself." */
   if (vk_format_is_depth_or_stencil(pCreateInfo->format))
            const VkImageFormatListCreateInfo *format_list = (const VkImageFormatListCreateInfo *)
            if (format_list) {
      if (!format_list->viewFormatCount)
            *formats = vk_alloc2(&device->alloc, pAllocator,
               if (*formats == NULL)
            memcpy(*formats, format_list->pViewFormats, sizeof(VkFormat) * format_list->viewFormatCount);
   *format_count = format_list->viewFormatCount;
               if (vk_format_is_compressed(pCreateInfo->format))
      return vk_image_create_get_format_list_compressed(device,
                           return vk_image_create_get_format_list_uncompressed(device,
                        }
      /* From the Vulkan Specification 1.2.166 - VkAttachmentReference2:
   *
   *   "If layout only specifies the layout of the depth aspect of the
   *    attachment, the layout of the stencil aspect is specified by the
   *    stencilLayout member of a VkAttachmentReferenceStencilLayout structure
   *    included in the pNext chain. Otherwise, layout describes the layout for
   *    all relevant image aspects."
   */
   VkImageLayout
   vk_att_ref_stencil_layout(const VkAttachmentReference2 *att_ref,
         {
      /* From VUID-VkAttachmentReference2-attachment-04755:
   *  "If attachment is not VK_ATTACHMENT_UNUSED, and the format of the
   *   referenced attachment is a depth/stencil format which includes both
   *   depth and stencil aspects [...]
   */
   if (att_ref->attachment == VK_ATTACHMENT_UNUSED ||
      !vk_format_has_stencil(attachments[att_ref->attachment].format))
         const VkAttachmentReferenceStencilLayout *stencil_ref =
            if (stencil_ref)
            /* From VUID-VkAttachmentReference2-attachment-04755:
   *  "If attachment is not VK_ATTACHMENT_UNUSED, and the format of the
   *   referenced attachment is a depth/stencil format which includes both
   *   depth and stencil aspects, and layout is
   *   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL or
   *   VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, the pNext chain must include
   *   a VkAttachmentReferenceStencilLayout structure."
   */
               }
      /* From the Vulkan Specification 1.2.184:
   *
   *   "If the pNext chain includes a VkAttachmentDescriptionStencilLayout
   *    structure, then the stencilInitialLayout and stencilFinalLayout members
   *    specify the initial and final layouts of the stencil aspect of a
   *    depth/stencil format, and initialLayout and finalLayout only apply to the
   *    depth aspect. For depth-only formats, the
   *    VkAttachmentDescriptionStencilLayout structure is ignored. For
   *    stencil-only formats, the initial and final layouts of the stencil aspect
   *    are taken from the VkAttachmentDescriptionStencilLayout structure if
   *    present, or initialLayout and finalLayout if not present."
   *
   *   "If format is a depth/stencil format, and either initialLayout or
   *    finalLayout does not specify a layout for the stencil aspect, then the
   *    application must specify the initial and final layouts of the stencil
   *    aspect by including a VkAttachmentDescriptionStencilLayout structure in
   *    the pNext chain."
   */
   VkImageLayout
   vk_att_desc_stencil_layout(const VkAttachmentDescription2 *att_desc, bool final)
   {
      if (!vk_format_has_stencil(att_desc->format))
            const VkAttachmentDescriptionStencilLayout *stencil_desc =
            if (stencil_desc) {
      return final ?
      stencil_desc->stencilFinalLayout :
            const VkImageLayout main_layout =
            /* From VUID-VkAttachmentDescription2-format-03302/03303:
   *  "If format is a depth/stencil format which includes both depth and
   *   stencil aspects, and initial/finalLayout is
   *   VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL or
   *   VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL, the pNext chain must include
   *   a VkAttachmentDescriptionStencilLayout structure."
   */
               }
      VkImageUsageFlags
   vk_image_layout_to_usage_flags(VkImageLayout layout,
         {
               switch (layout) {
   case VK_IMAGE_LAYOUT_UNDEFINED:
   case VK_IMAGE_LAYOUT_PREINITIALIZED:
            case VK_IMAGE_LAYOUT_GENERAL:
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
      assert(aspect & VK_IMAGE_ASPECT_ANY_COLOR_MASK_MESA);
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
      assert(aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
               case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
      assert(aspect & VK_IMAGE_ASPECT_DEPTH_BIT);
   return vk_image_layout_to_usage_flags(
         case VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL:
      assert(aspect & VK_IMAGE_ASPECT_STENCIL_BIT);
   return vk_image_layout_to_usage_flags(
         case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
      assert(aspect & (VK_IMAGE_ASPECT_DEPTH_BIT |
         return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
               case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL:
      assert(aspect & VK_IMAGE_ASPECT_DEPTH_BIT);
   return vk_image_layout_to_usage_flags(
         case VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL:
      assert(aspect & VK_IMAGE_ASPECT_STENCIL_BIT);
   return vk_image_layout_to_usage_flags(
         case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      return VK_IMAGE_USAGE_SAMPLED_BIT |
         case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            case VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL:
      if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
      return vk_image_layout_to_usage_flags(
      } else if (aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
      return vk_image_layout_to_usage_flags(
      } else {
      assert(!"Must be a depth/stencil aspect");
            case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL:
      if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
      return vk_image_layout_to_usage_flags(
      } else if (aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
      return vk_image_layout_to_usage_flags(
      } else {
      assert(!"Must be a depth/stencil aspect");
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
   /* This needs to be handled specially by the caller */
         case VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR:
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
         case VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR:
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
         case VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT:
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
         case VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL:
      if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT ||
      aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
      } else {
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
            case VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL:
      return VK_IMAGE_USAGE_SAMPLED_BIT |
         case VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT:
      if (aspect == VK_IMAGE_ASPECT_DEPTH_BIT ||
      aspect == VK_IMAGE_ASPECT_STENCIL_BIT) {
   return VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
         VK_IMAGE_USAGE_SAMPLED_BIT |
      } else {
      assert(aspect == VK_IMAGE_ASPECT_COLOR_BIT);
   return VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
         VK_IMAGE_USAGE_SAMPLED_BIT |
            case VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR:
         case VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR:
         case VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR:
      #ifdef VK_ENABLE_BETA_EXTENSIONS
      case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DST_KHR:
         case VK_IMAGE_LAYOUT_VIDEO_ENCODE_SRC_KHR:
         case VK_IMAGE_LAYOUT_VIDEO_ENCODE_DPB_KHR:
      #endif
      case VK_IMAGE_LAYOUT_MAX_ENUM:
                     }
