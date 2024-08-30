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
   #include <stddef.h>
   #include <stdint.h>
   #include <vulkan/vulkan.h>
      #include "pvr_blit.h"
   #include "pvr_clear.h"
   #include "pvr_csb.h"
   #include "pvr_formats.h"
   #include "pvr_job_transfer.h"
   #include "pvr_private.h"
   #include "pvr_shader_factory.h"
   #include "pvr_static_shaders.h"
   #include "pvr_types.h"
   #include "util/bitscan.h"
   #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_command_buffer.h"
   #include "vk_command_pool.h"
   #include "vk_format.h"
   #include "vk_log.h"
      /* TODO: Investigate where this limit comes from. */
   #define PVR_MAX_TRANSFER_SIZE_IN_TEXELS 2048U
      static struct pvr_transfer_cmd *
   pvr_transfer_cmd_alloc(struct pvr_cmd_buffer *cmd_buffer)
   {
               transfer_cmd = vk_zalloc(&cmd_buffer->vk.pool->alloc,
                     if (!transfer_cmd) {
      vk_command_buffer_set_error(&cmd_buffer->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               /* transfer_cmd->mapping_count is already set to zero. */
   transfer_cmd->sources[0].filter = PVR_FILTER_POINT;
   transfer_cmd->sources[0].resolve_op = PVR_RESOLVE_BLEND;
   transfer_cmd->sources[0].addr_mode = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
               }
      static void pvr_setup_buffer_surface(struct pvr_transfer_cmd_surface *surface,
                                       VkRect2D *rect,
   pvr_dev_addr_t dev_addr,
   {
               surface->dev_addr = PVR_DEV_ADDR_OFFSET(dev_addr, offset);
   surface->width = width;
   surface->height = height;
   surface->stride = stride;
   surface->vk_format = vk_format;
   surface->mem_layout = PVR_MEMLAYOUT_LINEAR;
            /* Initialize rectangle extent. Also, rectangle.offset should be set to
   * zero, as the offset is already adjusted in the device address above. We
   * don't explicitly set offset to zero as transfer_cmd is zero allocated.
   */
   rect->extent.width = width;
            if (util_format_is_compressed(pformat)) {
      uint32_t block_width = util_format_get_blockwidth(pformat);
            surface->width = MAX2(1U, DIV_ROUND_UP(surface->width, block_width));
   surface->height = MAX2(1U, DIV_ROUND_UP(surface->height, block_height));
            rect->offset.x /= block_width;
   rect->offset.y /= block_height;
   rect->extent.width =
         rect->extent.height =
         }
      VkFormat pvr_get_raw_copy_format(VkFormat format)
   {
      switch (vk_format_get_blocksize(format)) {
   case 1:
         case 2:
         case 3:
         case 4:
         case 6:
         case 8:
         case 12:
         case 16:
         default:
            }
      static void pvr_setup_transfer_surface(struct pvr_device *device,
                                          struct pvr_transfer_cmd_surface *surface,
   VkRect2D *rect,
   const struct pvr_image *image,
      {
      const uint32_t height = MAX2(image->vk.extent.height >> mip_level, 1U);
   const uint32_t width = MAX2(image->vk.extent.width >> mip_level, 1U);
   enum pipe_format image_pformat = vk_format_to_pipe_format(image->vk.format);
   enum pipe_format pformat = vk_format_to_pipe_format(format);
   const VkImageSubresource sub_resource = {
      .aspectMask = aspect_mask,
   .mipLevel = mip_level,
      };
   VkSubresourceLayout info;
            if (image->memlayout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
                     surface->dev_addr = PVR_DEV_ADDR_OFFSET(image->dev_addr, info.offset);
   surface->width = width;
   surface->height = height;
            assert(info.rowPitch % vk_format_get_blocksize(format) == 0);
            surface->vk_format = format;
   surface->mem_layout = image->memlayout;
            if (image->memlayout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
            rect->offset.x = offset->x;
   rect->offset.y = offset->y;
   rect->extent.width = extent->width;
            if (util_format_is_compressed(image_pformat) &&
      !util_format_is_compressed(pformat)) {
   uint32_t block_width = util_format_get_blockwidth(image_pformat);
            surface->width = MAX2(1U, DIV_ROUND_UP(surface->width, block_width));
   surface->height = MAX2(1U, DIV_ROUND_UP(surface->height, block_height));
            rect->offset.x /= block_width;
   rect->offset.y /= block_height;
   rect->extent.width =
         rect->extent.height =
         }
      void pvr_CmdBlitImage2KHR(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_image, src, pBlitImageInfo->srcImage);
   PVR_FROM_HANDLE(pvr_image, dst, pBlitImageInfo->dstImage);
   struct pvr_device *device = cmd_buffer->device;
                     if (pBlitImageInfo->filter == VK_FILTER_LINEAR)
            for (uint32_t i = 0U; i < pBlitImageInfo->regionCount; i++) {
               assert(region->srcSubresource.layerCount ==
         const bool inverted_dst_z =
         const bool inverted_src_z =
         const uint32_t min_src_z = inverted_src_z ? region->srcOffsets[1].z
         const uint32_t max_src_z = inverted_src_z ? region->srcOffsets[0].z
         const uint32_t min_dst_z = inverted_dst_z ? region->dstOffsets[1].z
         const uint32_t max_dst_z = inverted_dst_z ? region->dstOffsets[0].z
            const uint32_t src_width =
         const uint32_t src_height =
         uint32_t dst_width;
            float initial_depth_offset;
   VkExtent3D src_extent;
   VkExtent3D dst_extent;
   VkOffset3D dst_offset = region->dstOffsets[0];
   float z_slice_stride;
   bool flip_x;
            if (region->dstOffsets[1].x > region->dstOffsets[0].x) {
      dst_width = region->dstOffsets[1].x - region->dstOffsets[0].x;
      } else {
      dst_width = region->dstOffsets[0].x - region->dstOffsets[1].x;
   flip_x = true;
               if (region->dstOffsets[1].y > region->dstOffsets[0].y) {
      dst_height = region->dstOffsets[1].y - region->dstOffsets[0].y;
      } else {
      dst_height = region->dstOffsets[0].y - region->dstOffsets[1].y;
   flip_y = true;
               /* If any of the extent regions is zero, then reject the blit and
   * continue.
   */
   if (!src_width || !src_height || !dst_width || !dst_height ||
      !(max_dst_z - min_dst_z) || !(max_src_z - min_src_z)) {
   mesa_loge("BlitImage: Region %i has an area of zero", i);
               src_extent = (VkExtent3D){
      .width = src_width,
   .height = src_height,
               dst_extent = (VkExtent3D){
      .width = dst_width,
   .height = dst_height,
               /* The z_position of a transfer surface is intended to be in the range
   * of 0.0f <= z_position <= depth. It will be used as a texture coordinate
   * in the source surface for cases where linear filtering is enabled, so
   * the fractional part will need to represent the exact midpoint of a z
   * slice range in the source texture, as it maps to each destination
   * slice.
   *
   * For destination surfaces, the fractional part is discarded, so
   * we can safely pass the slice index.
            /* Calculate the ratio of z slices in our source region to that of our
   * destination region, to get the number of z slices in our source region
   * to iterate over for each destination slice.
   *
   * If our destination region is inverted, we iterate backwards.
   */
   z_slice_stride =
                  /* Offset the initial depth offset by half of the z slice stride, into the
   * blit region's z range.
   */
   initial_depth_offset =
            for (uint32_t j = 0U; j < region->srcSubresource.layerCount; j++) {
      struct pvr_transfer_cmd_surface src_surface = { 0 };
   struct pvr_transfer_cmd_surface dst_surface = { 0 };
                  /* Get the subresource info for the src and dst images, this is
   * required when incrementing the address of the depth slice used by
   * the transfer surface.
   */
   VkSubresourceLayout src_info, dst_info;
   const VkImageSubresource src_sub_resource = {
      .aspectMask = region->srcSubresource.aspectMask,
   .mipLevel = region->srcSubresource.mipLevel,
      };
   const VkImageSubresource dst_sub_resource = {
      .aspectMask = region->dstSubresource.aspectMask,
   .mipLevel = region->dstSubresource.mipLevel,
                              /* Setup the transfer surfaces once per image layer, which saves us
   * from repeating subresource queries by manually incrementing the
   * depth slices.
   */
   pvr_setup_transfer_surface(device,
                              &src_surface,
   &src_rect,
   src,
   region->srcSubresource.baseArrayLayer + j,
               pvr_setup_transfer_surface(device,
                              &dst_surface,
   &dst_rect,
   dst,
   region->dstSubresource.baseArrayLayer + j,
               for (uint32_t dst_z = min_dst_z; dst_z < max_dst_z; dst_z++) {
                     /* TODO: See if we can allocate all the transfer cmds in one go. */
   transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
                  transfer_cmd->sources[0].mappings[0].src_rect = src_rect;
   transfer_cmd->sources[0].mappings[0].dst_rect = dst_rect;
   transfer_cmd->sources[0].mappings[0].flip_x = flip_x;
                  transfer_cmd->sources[0].surface = src_surface;
                                 result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
                        if (src_surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED) {
         } else {
                        if (dst_surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
               }
      static VkFormat pvr_get_copy_format(VkFormat format)
   {
      switch (format) {
   case VK_FORMAT_R8_SNORM:
         case VK_FORMAT_R8G8_SNORM:
         case VK_FORMAT_R8G8B8_SNORM:
         case VK_FORMAT_R8G8B8A8_SNORM:
         case VK_FORMAT_B8G8R8A8_SNORM:
         default:
            }
      static void
   pvr_setup_surface_for_image(struct pvr_device *device,
                              struct pvr_transfer_cmd_surface *surface,
   VkRect2D *rect,
   const struct pvr_image *image,
   uint32_t array_layer,
   uint32_t array_offset,
   uint32_t mip_level,
      {
      if (image->vk.image_type != VK_IMAGE_TYPE_3D) {
      pvr_setup_transfer_surface(device,
                              surface,
   rect,
   image,
   array_layer + array_offset,
   mip_level,
   } else {
      pvr_setup_transfer_surface(device,
                              surface,
   rect,
   image,
   array_layer,
   mip_level,
      }
      static VkResult
   pvr_copy_or_resolve_image_region(struct pvr_cmd_buffer *cmd_buffer,
                           {
      enum pipe_format src_pformat = vk_format_to_pipe_format(src->vk.format);
   enum pipe_format dst_pformat = vk_format_to_pipe_format(dst->vk.format);
   bool src_block_compressed = util_format_is_compressed(src_pformat);
   bool dst_block_compressed = util_format_is_compressed(dst_pformat);
   VkExtent3D src_extent;
   VkExtent3D dst_extent;
   VkFormat dst_format;
   VkFormat src_format;
   uint32_t dst_layers;
   uint32_t src_layers;
   uint32_t max_slices;
            if (src->vk.format == VK_FORMAT_D24_UNORM_S8_UINT &&
      region->srcSubresource.aspectMask !=
         /* Takes the stencil of the source and the depth of the destination and
   * combines the two interleaved.
   */
            if (region->srcSubresource.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      /* Takes the depth of the source and the stencil of the destination and
   * combines the two interleaved.
   */
                  src_extent = region->extent;
            if (src_block_compressed && !dst_block_compressed) {
      uint32_t block_width = util_format_get_blockwidth(src_pformat);
            dst_extent.width = MAX2(1U, DIV_ROUND_UP(src_extent.width, block_width));
   dst_extent.height =
      } else if (!src_block_compressed && dst_block_compressed) {
      uint32_t block_width = util_format_get_blockwidth(dst_pformat);
            dst_extent.width = MAX2(1U, src_extent.width * block_width);
               /* We don't care what format dst is as it's guaranteed to be size compatible
   * with src.
   */
   dst_format = pvr_get_raw_copy_format(src->vk.format);
            src_layers =
         dst_layers =
            /* srcSubresource.layerCount must match layerCount of dstSubresource in
   * copies not involving 3D images. In copies involving 3D images, if there is
   * a 2D image it's layerCount.
   */
            for (uint32_t i = 0U; i < max_slices; i++) {
      struct pvr_transfer_cmd *transfer_cmd;
            transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
   if (!transfer_cmd)
            transfer_cmd->flags |= flags;
            pvr_setup_surface_for_image(
      cmd_buffer->device,
   &transfer_cmd->sources[0].surface,
   &transfer_cmd->sources[0].mappings[0U].src_rect,
   src,
   region->srcSubresource.baseArrayLayer,
   i,
   region->srcSubresource.mipLevel,
   &region->srcOffset,
   &src_extent,
   region->srcOffset.z + i,
               pvr_setup_surface_for_image(cmd_buffer->device,
                              &transfer_cmd->dst,
   &transfer_cmd->scissor,
   dst,
   region->dstSubresource.baseArrayLayer,
   i,
               transfer_cmd->sources[0].mappings[0U].dst_rect = transfer_cmd->scissor;
   transfer_cmd->sources[0].mapping_count++;
            result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, transfer_cmd);
                     }
      VkResult
   pvr_copy_or_resolve_color_image_region(struct pvr_cmd_buffer *cmd_buffer,
                     {
               if (src->vk.samples > 1U && dst->vk.samples < 2U) {
      /* Integer resolve picks a single sample. */
   if (vk_format_is_int(src->vk.format))
               return pvr_copy_or_resolve_image_region(cmd_buffer,
                        }
      static bool pvr_can_merge_ds_regions(const VkImageCopy2 *pRegionA,
         {
      assert(pRegionA->srcSubresource.aspectMask != 0U);
            if (!((pRegionA->srcSubresource.aspectMask ^
         pRegionB->srcSubresource.aspectMask) &
                  /* Assert if aspectMask mismatch between src and dst, given it's a depth and
   * stencil image so not multi-planar and from the Vulkan 1.0.223 spec:
   *
   *    If neither srcImage nor dstImage has a multi-planar image format then
   *    for each element of pRegions, srcSubresource.aspectMask and
   *    dstSubresource.aspectMask must match.
   */
   assert(pRegionA->srcSubresource.aspectMask ==
         assert(pRegionB->srcSubresource.aspectMask ==
            if (!(pRegionA->srcSubresource.mipLevel ==
               pRegionA->srcSubresource.baseArrayLayer ==
         pRegionA->srcSubresource.layerCount ==
                  if (!(pRegionA->dstSubresource.mipLevel ==
               pRegionA->dstSubresource.baseArrayLayer ==
         pRegionA->dstSubresource.layerCount ==
                  if (!(pRegionA->srcOffset.x == pRegionB->srcOffset.x &&
         pRegionA->srcOffset.y == pRegionB->srcOffset.y &&
                  if (!(pRegionA->dstOffset.x == pRegionB->dstOffset.x &&
         pRegionA->dstOffset.y == pRegionB->dstOffset.y &&
                  if (!(pRegionA->extent.width == pRegionB->extent.width &&
         pRegionA->extent.height == pRegionB->extent.height &&
                     }
      void pvr_CmdCopyImage2KHR(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_image, src, pCopyImageInfo->srcImage);
            const bool can_merge_ds = src->vk.format == VK_FORMAT_D24_UNORM_S8_UINT &&
                     for (uint32_t i = 0U; i < pCopyImageInfo->regionCount; i++) {
               /* If an application has split a copy between D24S8 images into two
   * separate copy regions (one for the depth aspect and one for the
   * stencil aspect) attempt to merge the two regions back into one blit.
   *
   * This can only be merged if both regions are identical apart from the
   * aspectMask, one of which has to be depth and the other has to be
   * stencil.
   *
   * Only attempt to merge consecutive regions, ignore the case of merging
   * non-consecutive regions.
   */
   if (can_merge_ds && i != (pCopyImageInfo->regionCount - 1)) {
      const bool ret =
      pvr_can_merge_ds_regions(&pCopyImageInfo->pRegions[i],
                        region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT |
                        result = pvr_copy_or_resolve_color_image_region(cmd_buffer,
                                    /* Skip the next region as it has been processed with the last
   * region.
                                 result =
      pvr_copy_or_resolve_color_image_region(cmd_buffer,
                  if (result != VK_SUCCESS)
         }
      VkResult
   pvr_copy_buffer_to_image_region_format(struct pvr_cmd_buffer *const cmd_buffer,
                                       {
      enum pipe_format pformat = vk_format_to_pipe_format(dst_format);
   uint32_t row_length_in_texels;
   uint32_t buffer_slice_size;
   uint32_t buffer_layer_size;
   uint32_t height_in_blks;
            if (region->bufferRowLength == 0)
         else
            if (region->bufferImageHeight == 0)
         else
            if (util_format_is_compressed(pformat)) {
      uint32_t block_width = util_format_get_blockwidth(pformat);
   uint32_t block_height = util_format_get_blockheight(pformat);
            height_in_blks = DIV_ROUND_UP(height_in_blks, block_height);
   row_length_in_texels =
                        buffer_slice_size = height_in_blks * row_length;
            for (uint32_t i = 0; i < region->imageExtent.depth; i++) {
               for (uint32_t j = 0; j < region->imageSubresource.layerCount; j++) {
      const VkDeviceSize buffer_offset = region->bufferOffset +
                              transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
                           pvr_setup_buffer_surface(
      &transfer_cmd->sources[0].surface,
   &transfer_cmd->sources[0].mappings[0].src_rect,
   buffer_dev_addr,
   buffer_offset,
   src_format,
   image->vk.format,
   region->imageExtent.width,
                              pvr_setup_transfer_surface(cmd_buffer->device,
                              &transfer_cmd->dst,
   &transfer_cmd->scissor,
   image,
   region->imageSubresource.baseArrayLayer + j,
                              result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, transfer_cmd);
                        }
      VkResult
   pvr_copy_buffer_to_image_region(struct pvr_cmd_buffer *const cmd_buffer,
                     {
      const VkImageAspectFlags aspect_mask = region->imageSubresource.aspectMask;
   VkFormat src_format;
   VkFormat dst_format;
            if (vk_format_has_depth(image->vk.format) &&
      vk_format_has_stencil(image->vk.format)) {
            if ((aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0) {
         } else {
      src_format = vk_format_depth_only(image->vk.format);
                  } else {
      src_format = pvr_get_raw_copy_format(image->vk.format);
               return pvr_copy_buffer_to_image_region_format(cmd_buffer,
                                    }
      void pvr_CmdCopyBufferToImage2KHR(
      VkCommandBuffer commandBuffer,
      {
      PVR_FROM_HANDLE(pvr_buffer, src, pCopyBufferToImageInfo->srcBuffer);
   PVR_FROM_HANDLE(pvr_image, dst, pCopyBufferToImageInfo->dstImage);
                     for (uint32_t i = 0; i < pCopyBufferToImageInfo->regionCount; i++) {
      const VkResult result =
      pvr_copy_buffer_to_image_region(cmd_buffer,
                  if (result != VK_SUCCESS)
         }
      VkResult
   pvr_copy_image_to_buffer_region_format(struct pvr_cmd_buffer *const cmd_buffer,
                                 {
      enum pipe_format pformat = vk_format_to_pipe_format(image->vk.format);
   struct pvr_transfer_cmd_surface dst_surface = { 0 };
   VkImageSubresource sub_resource;
   uint32_t buffer_image_height;
   uint32_t buffer_row_length;
   uint32_t buffer_slice_size;
   uint32_t max_array_layers;
   VkRect2D dst_rect = { 0 };
   uint32_t max_depth_slice;
            /* Only images with VK_SAMPLE_COUNT_1_BIT can be copied to buffer. */
            if (region->bufferRowLength == 0)
         else
            if (region->bufferImageHeight == 0)
         else
            max_array_layers =
      region->imageSubresource.baseArrayLayer +
         buffer_slice_size = buffer_image_height * buffer_row_length *
                     pvr_setup_buffer_surface(&dst_surface,
                           &dst_rect,
   buffer_dev_addr,
   region->bufferOffset,
            dst_rect.extent.width = region->imageExtent.width;
            if (util_format_is_compressed(pformat)) {
      uint32_t block_width = util_format_get_blockwidth(pformat);
            dst_rect.extent.width =
         dst_rect.extent.height =
               sub_resource = (VkImageSubresource){
      .aspectMask = region->imageSubresource.aspectMask,
   .mipLevel = region->imageSubresource.mipLevel,
                        for (uint32_t i = region->imageSubresource.baseArrayLayer;
      i < max_array_layers;
   i++) {
   struct pvr_transfer_cmd_surface src_surface = { 0 };
            /* Note: Set the depth to the initial depth offset, the memory address (or
   * the z_position) for the depth slice will be incremented manually in the
   * loop below.
   */
   pvr_setup_transfer_surface(cmd_buffer->device,
                              &src_surface,
   &src_rect,
   image,
   i,
               for (uint32_t j = region->imageOffset.z; j < max_depth_slice; j++) {
                     /* TODO: See if we can allocate all the transfer cmds in one go. */
   transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
                  transfer_cmd->sources[0].mappings[0].src_rect = src_rect;
                                                result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, transfer_cmd);
                        if (src_surface.mem_layout == PVR_MEMLAYOUT_3DTWIDDLED)
         else
                     }
      VkResult
   pvr_copy_image_to_buffer_region(struct pvr_cmd_buffer *const cmd_buffer,
                     {
               VkFormat src_format = pvr_get_copy_format(image->vk.format);
            /* Color and depth aspect copies can be done using an appropriate raw format.
   */
   if (aspect_mask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT)) {
      src_format = pvr_get_raw_copy_format(src_format);
      } else if (aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      /* From the Vulkan spec:
   *
   *    Data copied to or from the stencil aspect of any depth/stencil
   *    format is tightly packed with one VK_FORMAT_S8_UINT value per texel.
   */
      } else {
      /* YUV Planes require specific formats. */
               return pvr_copy_image_to_buffer_region_format(cmd_buffer,
                              }
      void pvr_CmdCopyImageToBuffer2(
      VkCommandBuffer commandBuffer,
      {
      PVR_FROM_HANDLE(pvr_buffer, dst, pCopyImageToBufferInfo->dstBuffer);
   PVR_FROM_HANDLE(pvr_image, src, pCopyImageToBufferInfo->srcImage);
                     for (uint32_t i = 0U; i < pCopyImageToBufferInfo->regionCount; i++) {
               const VkResult result = pvr_copy_image_to_buffer_region(cmd_buffer,
                     if (result != VK_SUCCESS)
         }
      static void pvr_calc_mip_level_extents(const struct pvr_image *image,
               {
      /* 3D textures are clamped to 4x4x4. */
   const uint32_t clamp = (image->vk.image_type == VK_IMAGE_TYPE_3D) ? 4 : 1;
            extent_out->width = MAX2(extent->width >> mip_level, clamp);
   extent_out->height = MAX2(extent->height >> mip_level, clamp);
      }
      static VkResult pvr_clear_image_range(struct pvr_cmd_buffer *cmd_buffer,
                           {
      const uint32_t layer_count =
         const uint32_t max_layers = psRange->baseArrayLayer + layer_count;
   VkFormat format = image->vk.format;
   const VkOffset3D offset = { 0 };
                     for (uint32_t layer = psRange->baseArrayLayer; layer < max_layers; layer++) {
      const uint32_t level_count =
                           for (uint32_t level = psRange->baseMipLevel; level < max_level; level++) {
               for (uint32_t depth = 0; depth < mip_extent.depth; depth++) {
                     transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
                                                pvr_setup_transfer_surface(cmd_buffer->device,
                              &transfer_cmd->dst,
   &transfer_cmd->scissor,
   image,
                     result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, transfer_cmd);
                           }
      void pvr_CmdClearColorImage(VkCommandBuffer commandBuffer,
                                 {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
            for (uint32_t i = 0; i < rangeCount; i++) {
      const VkResult result =
         if (result != VK_SUCCESS)
         }
      void pvr_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer,
                                 {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
            for (uint32_t i = 0; i < rangeCount; i++) {
      const VkImageAspectFlags ds_aspect = VK_IMAGE_ASPECT_DEPTH_BIT |
         VkClearColorValue clear_ds = { 0 };
   uint32_t flags = 0U;
            if (image->vk.format == VK_FORMAT_D24_UNORM_S8_UINT &&
      pRanges[i].aspectMask != ds_aspect) {
   /* A depth or stencil blit to a packed_depth_stencil requires a merge
   * operation.
                  if (pRanges[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
               clear_ds.float32[0] = pDepthStencil->depth;
            result =
         if (result != VK_SUCCESS)
         }
      static VkResult pvr_cmd_copy_buffer_region(struct pvr_cmd_buffer *cmd_buffer,
                                             {
               while (offset < size) {
      const VkDeviceSize remaining_size = size - offset;
   struct pvr_transfer_cmd *transfer_cmd;
   uint32_t texel_width;
   VkDeviceSize texels;
   VkFormat vk_format;
   VkResult result;
   uint32_t height;
            if (is_fill) {
      vk_format = VK_FORMAT_R32_UINT;
      } else if (remaining_size >= 16U) {
      vk_format = VK_FORMAT_R32G32B32A32_UINT;
      } else if (remaining_size >= 4U) {
      vk_format = VK_FORMAT_R32_UINT;
      } else {
      vk_format = VK_FORMAT_R8_UINT;
                        /* Try to do max-width rects, fall back to a 1-height rect for the
   * remainder.
   */
   if (texels > PVR_MAX_TRANSFER_SIZE_IN_TEXELS) {
      width = PVR_MAX_TRANSFER_SIZE_IN_TEXELS;
   height = texels / PVR_MAX_TRANSFER_SIZE_IN_TEXELS;
      } else {
      width = texels;
               transfer_cmd = pvr_transfer_cmd_alloc(cmd_buffer);
   if (!transfer_cmd)
            if (!is_fill) {
      pvr_setup_buffer_surface(
      &transfer_cmd->sources[0].surface,
   &transfer_cmd->sources[0].mappings[0].src_rect,
   src_addr,
   offset + src_offset,
   vk_format,
   vk_format,
   width,
   height,
         } else {
               for (uint32_t i = 0; i < ARRAY_SIZE(transfer_cmd->clear_color); i++)
               pvr_setup_buffer_surface(&transfer_cmd->dst,
                           &transfer_cmd->scissor,
   dst_addr,
   offset + dst_offset,
            if (transfer_cmd->source_count > 0) {
                           result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, transfer_cmd);
                              }
      void pvr_CmdUpdateBuffer(VkCommandBuffer commandBuffer,
                           {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_buffer, dst, dstBuffer);
   struct pvr_suballoc_bo *pvr_bo;
                     result = pvr_cmd_buffer_upload_general(cmd_buffer, pData, dataSize, &pvr_bo);
   if (result != VK_SUCCESS)
            pvr_cmd_copy_buffer_region(cmd_buffer,
                              pvr_bo->dev_addr,
   0,
   }
      void pvr_CmdCopyBuffer2KHR(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_buffer, src, pCopyBufferInfo->srcBuffer);
   PVR_FROM_HANDLE(pvr_buffer, dst, pCopyBufferInfo->dstBuffer);
                     for (uint32_t i = 0; i < pCopyBufferInfo->regionCount; i++) {
      const VkResult result =
      pvr_cmd_copy_buffer_region(cmd_buffer,
                              src->dev_addr,
   pCopyBufferInfo->pRegions[i].srcOffset,
   if (result != VK_SUCCESS)
         }
      void pvr_CmdFillBuffer(VkCommandBuffer commandBuffer,
                           {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                              /* From the Vulkan spec:
   *
   *    "size is the number of bytes to fill, and must be either a multiple
   *    of 4, or VK_WHOLE_SIZE to fill the range from offset to the end of
   *    the buffer. If VK_WHOLE_SIZE is used and the remaining size of the
   *    buffer is not a multiple of 4, then the nearest smaller multiple is
   *    used."
   */
            pvr_cmd_copy_buffer_region(cmd_buffer,
                              PVR_DEV_ADDR_INVALID,
   0,
   }
      /**
   * \brief Returns the maximum number of layers to clear starting from base_layer
   * that contain or match the target rectangle.
   *
   * \param[in] target_rect      The region which the clear should contain or
   *                             match.
   * \param[in] base_layer       The layer index to start at.
   * \param[in] clear_rect_count Amount of clear_rects
   * \param[in] clear_rects      Array of clear rects.
   *
   * \return Max number of layers that cover or match the target region.
   */
   static uint32_t
   pvr_get_max_layers_covering_target(VkRect2D target_rect,
                     {
      const int32_t target_x0 = target_rect.offset.x;
   const int32_t target_x1 = target_x0 + (int32_t)target_rect.extent.width;
   const int32_t target_y0 = target_rect.offset.y;
                     assert((int64_t)target_x0 + (int64_t)target_rect.extent.width <= INT32_MAX);
            for (uint32_t i = 0; i < clear_rect_count; i++) {
      const VkClearRect *clear_rect = &clear_rects[i];
   const uint32_t max_layer =
         bool target_is_covered;
   int32_t x0, x1;
            if (clear_rect->baseArrayLayer == 0)
            assert((uint64_t)clear_rect->baseArrayLayer + clear_rect->layerCount <=
            /* Check for layer intersection. */
   if (clear_rect->baseArrayLayer > base_layer || max_layer <= base_layer)
            x0 = clear_rect->rect.offset.x;
   x1 = x0 + (int32_t)clear_rect->rect.extent.width;
   y0 = clear_rect->rect.offset.y;
            assert((int64_t)x0 + (int64_t)clear_rect->rect.extent.width <= INT32_MAX);
   assert((int64_t)y0 + (int64_t)clear_rect->rect.extent.height <=
            target_is_covered = x0 <= target_x0 && x1 >= target_x1;
            if (target_is_covered)
                  }
      /* Return true if vertex shader is required to output render target id to pick
   * the texture array layer.
   */
   static inline bool
   pvr_clear_needs_rt_id_output(struct pvr_device_info *dev_info,
               {
      if (!PVR_HAS_FEATURE(dev_info, gs_rta_support))
            for (uint32_t i = 0; i < rect_count; i++) {
      if (rects[i].baseArrayLayer != 0 || rects[i].layerCount > 1)
                  }
      static VkResult pvr_clear_color_attachment_static_create_consts_buffer(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct pvr_shader_factory_info *shader_info,
   const uint32_t clear_color[static const PVR_CLEAR_COLOR_ARRAY_SIZE],
   ASSERTED bool uses_tile_buffer,
   uint32_t tile_buffer_idx,
      {
      struct pvr_device *device = cmd_buffer->device;
   struct pvr_suballoc_bo *const_shareds_buffer;
   struct pvr_bo *tile_buffer;
   uint64_t tile_dev_addr;
   uint32_t *buffer;
            /* TODO: This doesn't need to be aligned to slc size. Alignment to 4 is fine.
   * Change pvr_cmd_buffer_alloc_mem() to take in an alignment?
   */
   result =
      pvr_cmd_buffer_alloc_mem(cmd_buffer,
                  if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < PVR_CLEAR_ATTACHMENT_CONST_COUNT; i++) {
               if (dest_idx == PVR_CLEAR_ATTACHMENT_DEST_ID_UNUSED)
                     switch (i) {
   case PVR_CLEAR_ATTACHMENT_CONST_COMPONENT_0:
   case PVR_CLEAR_ATTACHMENT_CONST_COMPONENT_1:
   case PVR_CLEAR_ATTACHMENT_CONST_COMPONENT_2:
   case PVR_CLEAR_ATTACHMENT_CONST_COMPONENT_3:
                  case PVR_CLEAR_ATTACHMENT_CONST_TILE_BUFFER_UPPER:
      assert(uses_tile_buffer);
   tile_buffer = device->tile_buffer_state.buffers[tile_buffer_idx];
   tile_dev_addr = tile_buffer->vma->dev_addr.addr;
               case PVR_CLEAR_ATTACHMENT_CONST_TILE_BUFFER_LOWER:
      assert(uses_tile_buffer);
   tile_buffer = device->tile_buffer_state.buffers[tile_buffer_idx];
   tile_dev_addr = tile_buffer->vma->dev_addr.addr;
               default:
                     for (uint32_t i = 0; i < shader_info->num_static_const; i++) {
      const struct pvr_static_buffer *static_buff =
                                             }
      static VkResult pvr_clear_color_attachment_static(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct usc_mrt_resource *mrt_resource,
   VkFormat format,
   uint32_t clear_color[static const PVR_CLEAR_COLOR_ARRAY_SIZE],
   uint32_t template_idx,
   uint32_t stencil,
      {
      struct pvr_device *device = cmd_buffer->device;
   ASSERTED const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   ASSERTED const bool has_eight_output_registers =
         const struct pvr_device_static_clear_state *dev_clear_state =
         const bool uses_tile_buffer = mrt_resource->type ==
         const struct pvr_pds_clear_attachment_program_info *clear_attachment_program;
   struct pvr_pds_pixel_shader_sa_program texture_program;
   uint32_t pds_state[PVR_STATIC_CLEAR_PDS_STATE_COUNT];
   const struct pvr_shader_factory_info *shader_info;
   struct pvr_suballoc_bo *pds_texture_program_bo;
   struct pvr_static_clear_ppp_template template;
   struct pvr_suballoc_bo *const_shareds_buffer;
   uint64_t pds_texture_program_addr;
   struct pvr_suballoc_bo *pvr_bo;
   uint32_t tile_buffer_idx = 0;
   uint32_t out_reg_count;
   uint32_t output_offset;
   uint32_t program_idx;
   uint32_t *buffer;
            out_reg_count =
            if (uses_tile_buffer) {
      tile_buffer_idx = mrt_resource->mem.tile_buffer;
      } else {
                           program_idx = pvr_get_clear_attachment_program_index(out_reg_count,
                           result = pvr_clear_color_attachment_static_create_consts_buffer(
      cmd_buffer,
   shader_info,
   clear_color,
   uses_tile_buffer,
   tile_buffer_idx,
      if (result != VK_SUCCESS)
            /* clang-format off */
   texture_program = (struct pvr_pds_pixel_shader_sa_program){
      .num_texture_dma_kicks = 1,
   .texture_dma_address = {
            };
            pvr_csb_pack (&texture_program.texture_dma_control[0],
                  doutd_src1.dest = PVRX(PDSINST_DOUTD_DEST_COMMON_STORE);
               clear_attachment_program =
            /* TODO: This doesn't need to be aligned to slc size. Alignment to 4 is fine.
   * Change pvr_cmd_buffer_alloc_mem() to take in an alignment?
   */
   result = pvr_cmd_buffer_alloc_mem(
      cmd_buffer,
   device->heaps.pds_heap,
   clear_attachment_program->texture_program_data_size,
      if (result != VK_SUCCESS) {
      list_del(&const_shareds_buffer->link);
                        buffer = pvr_bo_suballoc_get_map_addr(pds_texture_program_bo);
   pds_texture_program_addr = pds_texture_program_bo->dev_addr.addr -
            pvr_pds_generate_pixel_shader_sa_texture_state_data(
      &texture_program,
   buffer,
         pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SHADERBASE],
                              pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_TEXUNICODEBASE],
                              pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SIZEINFO1],
                  sizeinfo1.pds_texturestatesize = DIV_ROUND_UP(
                  sizeinfo1.pds_tempsize =
      DIV_ROUND_UP(clear_attachment_program->texture_program_pds_temps_count,
            pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SIZEINFO2],
                  sizeinfo2.usc_sharedsize =
      DIV_ROUND_UP(shader_info->const_shared_regs,
            /* Dummy coefficient loading program. */
            pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_TEXTUREDATABASE],
                              assert(template_idx < PVR_STATIC_CLEAR_VARIANT_COUNT);
   template =
                     template.config.ispctl.upass =
            if (template_idx & VK_IMAGE_ASPECT_STENCIL_BIT)
            if (vs_has_rt_id_output) {
      template.config.output_sel.rhw_pres = true;
   template.config.output_sel.render_tgt_pres = true;
               result = pvr_emit_ppp_from_template(
      &cmd_buffer->state.current_sub_cmd->gfx.control_stream,
   &template,
      if (result != VK_SUCCESS) {
      list_del(&pds_texture_program_bo->link);
            list_del(&const_shareds_buffer->link);
                                    }
      /**
   * \brief Record a deferred clear operation into the command buffer.
   *
   * Devices which don't have gs_rta_support require extra handling for RTA
   * clears. We setup a list of deferred clear transfer commands which will be
   * processed at the end of the graphics sub command to account for the missing
   * feature.
   */
   static VkResult pvr_add_deferred_rta_clear(struct pvr_cmd_buffer *cmd_buffer,
                     {
      struct pvr_render_pass_info *pass_info = &cmd_buffer->state.render_pass_info;
   struct pvr_sub_cmd_gfx *sub_cmd = &cmd_buffer->state.current_sub_cmd->gfx;
   const struct pvr_renderpass_hwsetup_render *hw_render =
         struct pvr_transfer_cmd *transfer_cmd_list;
   const struct pvr_image_view *image_view;
   const struct pvr_image *image;
            const VkOffset3D offset = {
      .x = rect->rect.offset.x,
   .y = rect->rect.offset.y,
      };
   const VkExtent3D extent = {
      .width = rect->rect.extent.width,
   .height = rect->rect.extent.height,
               assert(
            transfer_cmd_list = util_dynarray_grow(&cmd_buffer->deferred_clears,
               if (!transfer_cmd_list) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
               /* From the Vulkan 1.3.229 spec VUID-VkClearAttachment-aspectMask-00019:
   *
   *    "If aspectMask includes VK_IMAGE_ASPECT_COLOR_BIT, it must not
   *    include VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT"
   *
   */
   if (attachment->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
      assert(attachment->aspectMask == VK_IMAGE_ASPECT_DEPTH_BIT ||
         attachment->aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT ||
               } else if (is_render_init) {
               assert(attachment->colorAttachment < hw_render->color_init_count);
               } else {
      const struct pvr_renderpass_hwsetup_subpass *hw_pass =
         const struct pvr_render_subpass *sub_pass =
         const uint32_t attachment_idx =
                                 base_layer = image_view->vk.base_array_layer + rect->baseArrayLayer;
            for (uint32_t i = 0; i < rect->layerCount; i++) {
               /* TODO: Add an init function for when we don't want to use
   * pvr_transfer_cmd_alloc()? And use it here.
   */
   *transfer_cmd = (struct pvr_transfer_cmd){
      .flags = PVR_TRANSFER_CMD_FLAGS_FILL,
   .cmd_buffer = cmd_buffer,
               if (attachment->aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
      for (uint32_t j = 0; j < ARRAY_SIZE(transfer_cmd->clear_color); j++) {
      transfer_cmd->clear_color[j].ui =
         } else {
      transfer_cmd->clear_color[0].f =
         transfer_cmd->clear_color[1].ui =
               pvr_setup_transfer_surface(cmd_buffer->device,
                              &transfer_cmd->dst,
   &transfer_cmd->scissor,
   image,
   base_layer + i,
   0,
               }
      static void pvr_clear_attachments(struct pvr_cmd_buffer *cmd_buffer,
                                 {
      const struct pvr_render_pass *pass = cmd_buffer->state.render_pass_info.pass;
   struct pvr_render_pass_info *pass_info = &cmd_buffer->state.render_pass_info;
   const struct pvr_renderpass_hwsetup_subpass *hw_pass =
         struct pvr_sub_cmd_gfx *sub_cmd = &cmd_buffer->state.current_sub_cmd->gfx;
   struct pvr_device_info *dev_info = &cmd_buffer->device->pdevice->dev_info;
   struct pvr_render_subpass *sub_pass = &pass->subpasses[hw_pass->index];
   uint32_t vs_output_size_in_bytes;
            /* TODO: This function can be optimized so that most of the device memory
   * gets allocated together in one go and then filled as needed. There might
   * also be opportunities to reuse pds code and data segments.
                              /* We'll be emitting to the control stream. */
            vs_has_rt_id_output =
            /* 4 because we're expecting the USC to output X, Y, Z, and W. */
   vs_output_size_in_bytes = PVR_DW_TO_BYTES(4);
   if (vs_has_rt_id_output)
            for (uint32_t i = 0; i < attachment_count; i++) {
      const VkClearAttachment *attachment = &attachments[i];
   struct pvr_pds_vertex_shader_program pds_program;
   struct pvr_pds_upload pds_program_upload = { 0 };
   uint64_t current_base_array_layer = ~0;
   VkResult result;
            if (attachment->aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
      uint32_t packed_clear_color[PVR_CLEAR_COLOR_ARRAY_SIZE];
   const struct usc_mrt_resource *mrt_resource;
   uint32_t global_attachment_idx;
                                                                           assert(local_attachment_idx < hw_render->color_init_count);
   global_attachment_idx =
                        assert(local_attachment_idx < sub_pass->color_count);
   global_attachment_idx =
                                                      pvr_get_hw_clear_color(format,
                  result = pvr_clear_color_attachment_static(cmd_buffer,
                                       if (result != VK_SUCCESS)
      } else if (hw_pass->z_replicate != -1 &&
            const VkClearColorValue clear_color = {
         };
   const uint32_t template_idx = attachment->aspectMask |
         const uint32_t stencil = attachment->clearValue.depthStencil.stencil;
                           pvr_get_hw_clear_color(VK_FORMAT_R32_SFLOAT,
                  result = pvr_clear_color_attachment_static(cmd_buffer,
                                       if (result != VK_SUCCESS)
      } else {
      const uint32_t template_idx = attachment->aspectMask;
                  assert(template_idx < PVR_STATIC_CLEAR_VARIANT_COUNT);
                  if (attachment->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      template.config.ispa.sref =
               if (vs_has_rt_id_output) {
      template.config.output_sel.rhw_pres = true;
   template.config.output_sel.render_tgt_pres = true;
               result = pvr_emit_ppp_from_template(&sub_cmd->control_stream,
               if (result != VK_SUCCESS) {
      pvr_cmd_buffer_set_error_unwarned(cmd_buffer, result);
                           if (attachment->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
         else
            if (vs_has_rt_id_output) {
      const struct pvr_device_static_clear_state *dev_clear_state =
                        /* We can't use the device's passthrough pds program since it doesn't
   * have iterate_instance_id enabled. We'll be uploading code sections
                  /* TODO: See if we can allocate all the code section memory in one go.
   * We'd need to make sure that changing instance_id_modifier doesn't
   * change the code section size.
   * Also check if we can reuse the same code segment for each rect.
   * Seems like the instance_id_modifier is written into the data section
   * and used by the pds ADD instruction that way instead of it being
                  pvr_pds_clear_rta_vertex_shader_program_init_base(&pds_program,
      } else {
      /* We can reuse the device's code section but we'll need to upload data
   * sections so initialize the program.
   */
   pvr_pds_clear_vertex_shader_program_init_base(
                  pds_program_upload.code_offset =
         /* TODO: The code size doesn't get used by pvr_clear_vdm_state() maybe
   * let's change its interface to make that clear and not set this?
   */
   pds_program_upload.code_size =
               for (uint32_t j = 0; j < rect_count; j++) {
      struct pvr_pds_upload pds_program_data_upload;
   const VkClearRect *clear_rect = &rects[j];
   struct pvr_suballoc_bo *vertices_bo;
   uint32_t vdm_cs_size_in_dw;
                  if (!PVR_HAS_FEATURE(dev_info, gs_rta_support) &&
      (clear_rect->baseArrayLayer != 0 || clear_rect->layerCount > 1)) {
   result = pvr_add_deferred_rta_clear(cmd_buffer,
                                    if (clear_rect->baseArrayLayer != 0)
               /* TODO: Allocate all the buffers in one go before the loop, and add
   * support to multi-alloc bo.
   */
   result = pvr_clear_vertices_upload(cmd_buffer->device,
                     if (result != VK_SUCCESS) {
      pvr_cmd_buffer_set_error_unwarned(cmd_buffer, result);
                        if (vs_has_rt_id_output) {
      if (current_base_array_layer != clear_rect->baseArrayLayer) {
                     result =
      pvr_pds_clear_rta_vertex_shader_program_create_and_upload_code(
      &pds_program,
   cmd_buffer,
   base_array_layer,
   if (result != VK_SUCCESS) {
                        pds_program_upload.code_offset =
         /* TODO: The code size doesn't get used by pvr_clear_vdm_state()
   * maybe let's change its interface to make that clear and not
                                    result =
      pvr_pds_clear_rta_vertex_shader_program_create_and_upload_data(
      &pds_program,
   cmd_buffer,
   vertices_bo,
   if (result != VK_SUCCESS)
      } else {
      result = pvr_pds_clear_vertex_shader_program_create_and_upload_data(
      &pds_program,
   cmd_buffer,
   vertices_bo,
      if (result != VK_SUCCESS)
                              vdm_cs_size_in_dw =
                           vdm_cs_buffer =
         if (!vdm_cs_buffer) {
      pvr_cmd_buffer_set_error_unwarned(cmd_buffer,
                     pvr_pack_clear_vdm_state(dev_info,
                                                   }
      void pvr_clear_attachments_render_init(struct pvr_cmd_buffer *cmd_buffer,
               {
         }
      void pvr_CmdClearAttachments(VkCommandBuffer commandBuffer,
                           {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
            PVR_CHECK_COMMAND_BUFFER_BUILDING_STATE(cmd_buffer);
            /* TODO: There are some optimizations that can be made here:
   *  - For a full screen clear, update the clear values for the corresponding
   *    attachment index.
   *  - For a full screen color attachment clear, add its index to a load op
   *    override to add it to the background shader. This will elide any load
   *    op loads currently in the background shader as well as the usual
   *    frag kick for geometry clear.
            /* If we have any depth/stencil clears, update the sub command depth/stencil
   * modification and usage flags.
   */
   if (state->depth_format != VK_FORMAT_UNDEFINED) {
      uint32_t full_screen_clear_count;
   bool has_stencil_clear = false;
            for (uint32_t i = 0; i < attachmentCount; i++) {
                                             if (has_stencil_clear && has_depth_clear)
               sub_cmd->modifies_stencil |= has_stencil_clear;
            /* We only care about clears that have a baseArrayLayer of 0 as any
   * attachment clears we move to the background shader must apply to all of
   * the attachment's sub resources.
   */
   full_screen_clear_count =
      pvr_get_max_layers_covering_target(state->render_pass_info.render_area,
                     if (full_screen_clear_count > 0) {
      if (has_stencil_clear &&
      sub_cmd->stencil_usage == PVR_DEPTH_STENCIL_USAGE_UNDEFINED) {
               if (has_depth_clear &&
      sub_cmd->depth_usage == PVR_DEPTH_STENCIL_USAGE_UNDEFINED) {
                     pvr_clear_attachments(cmd_buffer,
                        attachmentCount,
   }
      void pvr_CmdResolveImage2KHR(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_image, src, pResolveImageInfo->srcImage);
   PVR_FROM_HANDLE(pvr_image, dst, pResolveImageInfo->dstImage);
                     for (uint32_t i = 0U; i < pResolveImageInfo->regionCount; i++) {
      VkImageCopy2 region = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
   .srcSubresource = pResolveImageInfo->pRegions[i].srcSubresource,
   .srcOffset = pResolveImageInfo->pRegions[i].srcOffset,
   .dstSubresource = pResolveImageInfo->pRegions[i].dstSubresource,
   .dstOffset = pResolveImageInfo->pRegions[i].dstOffset,
               VkResult result =
         if (result != VK_SUCCESS)
         }
