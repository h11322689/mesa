   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_image.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "panvk_private.h"
      #include "drm-uapi/drm_fourcc.h"
   #include "util/u_atomic.h"
   #include "util/u_debug.h"
   #include "vk_format.h"
   #include "vk_object.h"
   #include "vk_util.h"
      unsigned
   panvk_image_get_plane_size(const struct panvk_image *image, unsigned plane)
   {
      assert(!plane);
      }
      unsigned
   panvk_image_get_total_size(const struct panvk_image *image)
   {
      assert(util_format_get_num_planes(image->pimage.layout.format) == 1);
      }
      static enum mali_texture_dimension
   panvk_image_type_to_mali_tex_dim(VkImageType type)
   {
      switch (type) {
   case VK_IMAGE_TYPE_1D:
         case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
         default:
            }
      static VkResult
   panvk_image_create(VkDevice _device, const VkImageCreateInfo *pCreateInfo,
               {
      VK_FROM_HANDLE(panvk_device, device, _device);
   const struct panfrost_device *pdev = &device->physical_device->pdev;
            image = vk_image_create(&device->vk, pCreateInfo, alloc, sizeof(*image));
   if (!image)
            image->pimage.layout = (struct pan_image_layout){
      .modifier = modifier,
   .format = vk_format_to_pipe_format(image->vk.format),
   .dim = panvk_image_type_to_mali_tex_dim(image->vk.image_type),
   .width = image->vk.extent.width,
   .height = image->vk.extent.height,
   .depth = image->vk.extent.depth,
   .array_size = image->vk.array_layers,
   .nr_samples = image->vk.samples,
                        *pImage = panvk_image_to_handle(image);
      }
      static uint64_t
   panvk_image_select_mod(VkDevice _device, const VkImageCreateInfo *pCreateInfo,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
   const struct panfrost_device *pdev = &device->physical_device->pdev;
   enum pipe_format fmt = vk_format_to_pipe_format(pCreateInfo->format);
   bool noafbc =
         bool linear =
                     if (pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR)
            if (pCreateInfo->tiling == VK_IMAGE_TILING_DRM_FORMAT_MODIFIER_EXT) {
      const VkImageDrmFormatModifierListCreateInfoEXT *mod_info =
      vk_find_struct_const(pCreateInfo->pNext,
      const VkImageDrmFormatModifierExplicitCreateInfoEXT *drm_explicit_info =
      vk_find_struct_const(
                                 if (mod_info) {
      modifier = DRM_FORMAT_MOD_LINEAR;
   for (unsigned i = 0; i < mod_info->drmFormatModifierCount; i++) {
      if (drm_is_afbc(mod_info->pDrmFormatModifiers[i]) && !noafbc) {
      modifier = mod_info->pDrmFormatModifiers[i];
            } else {
      modifier = drm_explicit_info->drmFormatModifier;
   assert(modifier == DRM_FORMAT_MOD_LINEAR ||
         modifier == DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED ||
                           const struct wsi_image_create_info *wsi_info =
         if (wsi_info && wsi_info->scanout)
                     if (linear)
            /* Image store don't work on AFBC images */
   if (pCreateInfo->usage & VK_IMAGE_USAGE_STORAGE_BIT)
            /* AFBC does not support layered multisampling */
   if (pCreateInfo->samples > 1)
            if (!pdev->has_afbc)
            /* Only a small selection of formats are AFBC'able */
   if (!panfrost_format_supports_afbc(pdev, fmt))
            /* 3D AFBC is only supported on Bifrost v7+. It's supposed to
   * be supported on Midgard but it doesn't seem to work.
   */
   if (pCreateInfo->imageType == VK_IMAGE_TYPE_3D && pdev->arch < 7)
            /* For one tile, AFBC is a loss compared to u-interleaved */
   if (pCreateInfo->extent.width <= 16 && pCreateInfo->extent.height <= 16)
            if (noafbc)
            uint64_t afbc_type =
            if (panfrost_afbc_can_ytr(fmt))
               }
      VkResult
   panvk_CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
         {
      const VkSubresourceLayout *plane_layouts;
   uint64_t modifier =
            return panvk_image_create(device, pCreateInfo, pAllocator, pImage, modifier,
      }
      void
   panvk_DestroyImage(VkDevice _device, VkImage _image,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!image)
               }
      static unsigned
   panvk_plane_index(VkFormat format, VkImageAspectFlags aspect_mask)
   {
      switch (aspect_mask) {
   default:
         case VK_IMAGE_ASPECT_PLANE_1_BIT:
         case VK_IMAGE_ASPECT_PLANE_2_BIT:
         case VK_IMAGE_ASPECT_STENCIL_BIT:
            }
      void
   panvk_GetImageSubresourceLayout(VkDevice _device, VkImage _image,
               {
               unsigned plane =
                  const struct pan_image_slice_layout *slice_layout =
            pLayout->offset = slice_layout->offset + (pSubresource->arrayLayer *
         pLayout->size = slice_layout->size;
   pLayout->rowPitch = slice_layout->row_stride;
   pLayout->arrayPitch = image->pimage.layout.array_stride;
      }
      void
   panvk_DestroyImageView(VkDevice _device, VkImageView _view,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!view)
            panfrost_bo_unreference(view->bo);
      }
      void
   panvk_DestroyBufferView(VkDevice _device, VkBufferView bufferView,
         {
      VK_FROM_HANDLE(panvk_device, device, _device);
            if (!view)
            panfrost_bo_unreference(view->bo);
      }
      VkResult
   panvk_GetImageDrmFormatModifierPropertiesEXT(
      VkDevice device, VkImage _image,
      {
               assert(pProperties->sType ==
            pProperties->drmFormatModifier = image->pimage.layout.modifier;
      }
