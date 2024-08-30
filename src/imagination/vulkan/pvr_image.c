   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <string.h>
      #include "pvr_csb.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_private.h"
   #include "pvr_tex_state.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_format.h"
   #include "vk_image.h"
   #include "vk_log.h"
   #include "vk_object.h"
   #include "vk_util.h"
   #include "wsi_common.h"
      static void pvr_image_init_memlayout(struct pvr_image *image)
   {
      switch (image->vk.tiling) {
   default:
         case VK_IMAGE_TILING_OPTIMAL:
      if (image->vk.wsi_legacy_scanout)
         else if (image->vk.image_type == VK_IMAGE_TYPE_3D)
         else
            case VK_IMAGE_TILING_LINEAR:
      image->memlayout = PVR_MEMLAYOUT_LINEAR;
         }
      static void pvr_image_init_physical_extent(struct pvr_image *image)
   {
               /* clang-format off */
   if (image->vk.mip_levels > 1 ||
      image->memlayout == PVR_MEMLAYOUT_TWIDDLED ||
   image->memlayout == PVR_MEMLAYOUT_3DTWIDDLED) {
   /* clang-format on */
   image->physical_extent.width =
         image->physical_extent.height =
         image->physical_extent.depth =
      } else {
      assert(image->memlayout == PVR_MEMLAYOUT_LINEAR);
         }
      static void pvr_image_setup_mip_levels(struct pvr_image *image)
   {
      const uint32_t extent_alignment =
         const unsigned int cpp = vk_format_get_blocksize(image->vk.format);
   VkExtent3D extent =
            /* Mip-mapped textures that are non-dword aligned need dword-aligned levels
   * so they can be TQd from.
   */
                              for (uint32_t i = 0; i < image->vk.mip_levels; i++) {
               mip_level->pitch = cpp * ALIGN(extent.width, extent_alignment);
   mip_level->height_pitch = ALIGN(extent.height, extent_alignment);
   mip_level->size = image->vk.samples * mip_level->pitch *
               mip_level->size = ALIGN(mip_level->size, level_alignment);
                     extent.height = u_minify(extent.height, 1);
   extent.width = u_minify(extent.width, 1);
               /* The hw calculates layer strides as if a full mip chain up until 1x1x1
   * were present so we need to account for that in the `layer_size`.
   */
   while (extent.height != 1 || extent.width != 1 || extent.depth != 1) {
      const uint32_t height_pitch = ALIGN(extent.height, extent_alignment);
            image->layer_size += image->vk.samples * pitch * height_pitch *
            extent.height = u_minify(extent.height, 1);
   extent.width = u_minify(extent.width, 1);
               /* TODO: It might be useful to store the alignment in the image so it can be
   * checked (via an assert?) when setting
   * RGX_CR_TPU_TAG_CEM_4K_FACE_PACKING_EN, assuming this is where the
   * requirement comes from.
   */
   if (image->vk.array_layers > 1)
               }
      VkResult pvr_CreateImage(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            image =
         if (!image)
            /* All images aligned to 4k, in case of arrays/CEM.
   * Refer: pvr_GetImageMemoryRequirements for further details.
   */
            /* Initialize the image using the saved information from pCreateInfo */
   pvr_image_init_memlayout(image);
   pvr_image_init_physical_extent(image);
                        }
      void pvr_DestroyImage(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!image)
            if (image->vma)
               }
      /* clang-format off */
   /* Consider a 4 page buffer object.
   *   _________________________________________
   *  |         |          |         |          |
   *  |_________|__________|_________|__________|
   *                  |
   *                  \__ offset (0.5 page size)
   *
   *                  |___size(2 pages)____|
   *
   *            |__VMA size required (3 pages)__|
   *
   *                  |
   *                  \__ returned dev_addr = vma + offset % page_size
   *
   *   VMA size = align(size + offset % page_size, page_size);
   *
   *   Note: the above handling is currently divided between generic
   *   driver code and winsys layer. Given are the details of how this is
   *   being handled.
   *   * As winsys vma allocation interface does not have offset information,
   *     it can not calculate the extra size needed to adjust for the unaligned
   *     offset. So generic code is responsible for allocating a VMA that has
   *     extra space to deal with the above scenario.
   *   * Remaining work of mapping the vma to bo is done by vma_map interface,
   *     as it contains offset information, we don't need to do any adjustments
   *     in the generic code for this part.
   *
   *  TODO: Look into merging heap_alloc and vma_map into single interface.
   */
   /* clang-format on */
      VkResult pvr_BindImageMemory2(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            for (i = 0; i < bindInfoCount; i++) {
      PVR_FROM_HANDLE(pvr_device_memory, mem, pBindInfos[i].memory);
            VkResult result = pvr_bind_memory(device,
                                       if (result != VK_SUCCESS) {
                                                      }
      void pvr_get_image_subresource_layout(const struct pvr_image *image,
               {
      const struct pvr_mip_level *mip_level =
            pvr_assert(subresource->mipLevel < image->vk.mip_levels);
            layout->offset =
         layout->rowPitch = mip_level->pitch;
   layout->depthPitch = mip_level->pitch * mip_level->height_pitch;
   layout->arrayPitch = image->layer_size;
      }
      void pvr_GetImageSubresourceLayout(VkDevice device,
                     {
                  }
      VkResult pvr_CreateImageView(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_device, device, _device);
   struct pvr_texture_state_info info;
   unsigned char input_swizzle[4];
   const uint8_t *format_swizzle;
   const struct pvr_image *image;
   struct pvr_image_view *iview;
            iview = vk_image_view_create(&device->vk,
                           if (!iview)
                     info.type = iview->vk.view_type;
   info.base_level = iview->vk.base_mip_level;
   info.mip_levels = iview->vk.level_count;
   info.extent = image->vk.extent;
   info.aspect_mask = image->vk.aspects;
   info.is_cube = (info.type == VK_IMAGE_VIEW_TYPE_CUBE ||
         info.array_size = iview->vk.layer_count;
   info.offset = iview->vk.base_array_layer * image->layer_size +
         info.mipmaps_present = (image->vk.mip_levels > 1) ? true : false;
   info.stride = image->physical_extent.width;
   info.tex_state_type = PVR_TEXTURE_STATE_SAMPLE;
   info.mem_layout = image->memlayout;
   info.flags = 0;
   info.sample_count = image->vk.samples;
                     vk_component_mapping_to_pipe_swizzle(iview->vk.swizzle, input_swizzle);
   format_swizzle = pvr_get_format_swizzle(info.format);
            result = pvr_pack_tex_state(device,
               if (result != VK_SUCCESS)
            /* Create an additional texture state for cube type if storage
   * usage flag is set.
   */
   if (info.is_cube && image->vk.usage & VK_IMAGE_USAGE_STORAGE_BIT) {
               result = pvr_pack_tex_state(device,
               if (result != VK_SUCCESS)
               if (image->vk.usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT) {
      /* Attachment state is created as if the mipmaps are not supported, so the
   * baselevel is set to zero and num_mip_levels is set to 1. Which gives an
   * impression that this is the only level in the image. This also requires
   * that width, height and depth be adjusted as well. Given
   * iview->vk.extent is already adjusted for base mip map level we use it
   * here.
   */
   /* TODO: Investigate and document the reason for above approach. */
            info.mip_levels = 1;
   info.mipmaps_present = false;
   info.stride = u_minify(image->physical_extent.width, info.base_level);
   info.base_level = 0;
            if (image->vk.image_type == VK_IMAGE_TYPE_3D &&
      iview->vk.view_type == VK_IMAGE_VIEW_TYPE_2D) {
      } else {
                  result = pvr_pack_tex_state(device,
               if (result != VK_SUCCESS)
                              err_vk_image_view_destroy:
                  }
      void pvr_DestroyImageView(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!iview)
               }
      VkResult pvr_CreateBufferView(VkDevice _device,
                     {
      PVR_FROM_HANDLE(pvr_buffer, buffer, pCreateInfo->buffer);
   PVR_FROM_HANDLE(pvr_device, device, _device);
   struct pvr_texture_state_info info;
   const uint8_t *format_swizzle;
   struct pvr_buffer_view *bview;
            bview = vk_object_alloc(&device->vk,
                     if (!bview)
            bview->format = pCreateInfo->format;
   bview->range =
            /* If the remaining size of the buffer is not a multiple of the element
   * size of the format, the nearest smaller multiple is used.
   */
            /* The range of the buffer view shouldn't be smaller than one texel. */
            info.base_level = 0U;
   info.mip_levels = 1U;
   info.mipmaps_present = false;
   info.extent.width = 8192U;
   info.extent.height = bview->range / vk_format_get_blocksize(bview->format);
   info.extent.height = DIV_ROUND_UP(info.extent.height, info.extent.width);
   info.extent.depth = 0U;
   info.sample_count = 1U;
   info.stride = info.extent.width;
   info.offset = 0U;
   info.addr = PVR_DEV_ADDR_OFFSET(buffer->dev_addr, pCreateInfo->offset);
   info.mem_layout = PVR_MEMLAYOUT_LINEAR;
   info.is_cube = false;
   info.type = VK_IMAGE_VIEW_TYPE_2D;
   info.tex_state_type = PVR_TEXTURE_STATE_SAMPLE;
   info.format = bview->format;
   info.flags = PVR_TEXFLAGS_INDEX_LOOKUP;
            if (PVR_HAS_FEATURE(&device->pdevice->dev_info, tpu_array_textures))
            format_swizzle = pvr_get_format_swizzle(info.format);
            result = pvr_pack_tex_state(device, &info, bview->texture_state);
   if (result != VK_SUCCESS)
                           err_vk_buffer_view_destroy:
                  }
      void pvr_DestroyBufferView(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_buffer_view, bview, bufferView);
            if (!bview)
               }
