   /*
   * Copyright © 2016 Intel Corporation
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
      #include "anv_private.h"
   #include "genxml/gen8_pack.h"
      static bool
   lookup_blorp_shader(struct blorp_batch *batch,
               {
      struct blorp_context *blorp = batch->blorp;
            struct anv_shader_bin *bin =
      anv_device_search_for_kernel(device, device->internal_cache,
      if (!bin)
            /* The cache already has a reference and it's not going anywhere so there
   * is no need to hold a second reference.
   */
            *kernel_out = bin->kernel.offset;
               }
      static bool
   upload_blorp_shader(struct blorp_batch *batch, uint32_t stage,
                     const void *key, uint32_t key_size,
   const void *kernel, uint32_t kernel_size,
   {
      struct blorp_context *blorp = batch->blorp;
            struct anv_pipeline_bind_map bind_map = {
      .surface_count = 0,
      };
            struct anv_shader_bin *bin =
      anv_device_upload_kernel(device, device->internal_cache, stage,
                                 if (!bin)
            /* The cache already has a reference and it's not going anywhere so there
   * is no need to hold a second reference.
   */
            *kernel_out = bin->kernel.offset;
               }
      void
   anv_device_init_blorp(struct anv_device *device)
   {
      const struct blorp_config config = {
                  blorp_init(&device->blorp, device, &device->isl_dev, &config);
   device->blorp.compiler = device->physical->compiler;
   device->blorp.lookup_shader = lookup_blorp_shader;
   device->blorp.upload_shader = upload_blorp_shader;
      }
      void
   anv_device_finish_blorp(struct anv_device *device)
   {
         }
      static void
   anv_blorp_batch_init(struct anv_cmd_buffer *cmd_buffer,
         {
               if (queue_flags & VK_QUEUE_GRAPHICS_BIT) {
         } else if (queue_flags & VK_QUEUE_COMPUTE_BIT) {
         } else if (queue_flags & VK_QUEUE_TRANSFER_BIT) {
         } else {
                     }
      static void
   anv_blorp_batch_finish(struct blorp_batch *batch)
   {
         }
      static void
   get_blorp_surf_for_anv_address(struct anv_device *device,
                                 struct anv_address address,
   {
               *blorp_surf = (struct blorp_surf) {
      .surf = isl_surf,
   .addr = {
      .buffer = address.bo,
   .offset = address.offset,
   .mocs = anv_mocs(device, address.bo,
                        ok = isl_surf_init(&device->isl_dev, isl_surf,
                     .dim = ISL_SURF_DIM_2D,
   .format = format,
   .width = width,
   .height = height,
   .depth = 1,
   .levels = 1,
   .array_len = 1,
   .samples = 1,
   .row_pitch_B = row_pitch,
      }
      static void
   get_blorp_surf_for_anv_buffer(struct anv_device *device,
                                 struct anv_buffer *buffer, uint64_t offset,
   {
      get_blorp_surf_for_anv_address(device,
                  }
      /* Pick something high enough that it won't be used in core and low enough it
   * will never map to an extension.
   */
   #define ANV_IMAGE_LAYOUT_EXPLICIT_AUX (VkImageLayout)10000000
      static struct blorp_address
   anv_to_blorp_address(struct anv_address addr)
   {
      return (struct blorp_address) {
      .buffer = addr.bo,
         }
      static void
   get_blorp_surf_for_anv_image(const struct anv_cmd_buffer *cmd_buffer,
                              const struct anv_image *image,
      {
      const struct anv_device *device = cmd_buffer->device;
            if (layout != ANV_IMAGE_LAYOUT_EXPLICIT_AUX) {
      assert(usage != 0);
   aux_usage = anv_layout_to_aux_usage(device->info, image,
                     isl_surf_usage_flags_t mocs_usage =
      (usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) ?
         const struct anv_surface *surface = &image->planes[plane].primary_surface;
   const struct anv_address address =
            *blorp_surf = (struct blorp_surf) {
      .surf = &surface->isl,
   .addr = {
      .buffer = address.bo,
   .offset = address.offset,
                  if (aux_usage != ISL_AUX_USAGE_NONE) {
      const struct anv_surface *aux_surface = &image->planes[plane].aux_surface;
   const struct anv_address aux_address =
            blorp_surf->aux_usage = aux_usage;
            if (!anv_address_is_null(aux_address)) {
      blorp_surf->aux_addr = (struct blorp_address) {
      .buffer = aux_address.bo,
   .offset = aux_address.offset,
                  /* If we're doing a partial resolve, then we need the indirect clear
   * color.  If we are doing a fast clear and want to store/update the
   * clear color, we also pass the address to blorp, otherwise it will only
   * stomp the CCS to a particular value and won't care about format or
   * clear value
   */
   if (aspect & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
      const struct anv_address clear_color_addr =
            } else if (aspect & VK_IMAGE_ASPECT_DEPTH_BIT) {
      const struct anv_address clear_color_addr =
         blorp_surf->clear_color_addr = anv_to_blorp_address(clear_color_addr);
   blorp_surf->clear_color = (union isl_color_value) {
                  }
      static void
   copy_image(struct anv_cmd_buffer *cmd_buffer,
            struct blorp_batch *batch,
   struct anv_image *src_image,
   VkImageLayout src_image_layout,
   struct anv_image *dst_image,
      {
      VkOffset3D srcOffset =
         VkOffset3D dstOffset =
         VkExtent3D extent =
            const uint32_t dst_level = region->dstSubresource.mipLevel;
   unsigned dst_base_layer, layer_count;
   if (dst_image->vk.image_type == VK_IMAGE_TYPE_3D) {
      dst_base_layer = region->dstOffset.z;
      } else {
      dst_base_layer = region->dstSubresource.baseArrayLayer;
   layer_count = vk_image_subresource_layer_count(&dst_image->vk,
               const uint32_t src_level = region->srcSubresource.mipLevel;
   unsigned src_base_layer;
   if (src_image->vk.image_type == VK_IMAGE_TYPE_3D) {
         } else {
      src_base_layer = region->srcSubresource.baseArrayLayer;
   assert(layer_count ==
                     VkImageAspectFlags src_mask = region->srcSubresource.aspectMask,
                     if (util_bitcount(src_mask) > 1) {
      anv_foreach_image_aspect_bit(aspect_bit, src_image, src_mask) {
      struct blorp_surf src_surf, dst_surf;
   get_blorp_surf_for_anv_image(cmd_buffer,
                           get_blorp_surf_for_anv_image(cmd_buffer,
                           anv_cmd_buffer_mark_image_written(cmd_buffer, dst_image,
                        for (unsigned i = 0; i < layer_count; i++) {
      blorp_copy(batch, &src_surf, src_level, src_base_layer + i,
            &dst_surf, dst_level, dst_base_layer + i,
   srcOffset.x, srcOffset.y,
         } else {
      struct blorp_surf src_surf, dst_surf;
   get_blorp_surf_for_anv_image(cmd_buffer, src_image, src_mask,
                     get_blorp_surf_for_anv_image(cmd_buffer, dst_image, dst_mask,
                     anv_cmd_buffer_mark_image_written(cmd_buffer, dst_image, dst_mask,
                  for (unsigned i = 0; i < layer_count; i++) {
      blorp_copy(batch, &src_surf, src_level, src_base_layer + i,
            &dst_surf, dst_level, dst_base_layer + i,
   srcOffset.x, srcOffset.y,
         }
      static struct anv_state
   record_main_rcs_cmd_buffer_done(struct anv_cmd_buffer *cmd_buffer)
   {
               if (cmd_buffer->companion_rcs_cmd_buffer == NULL) {
      anv_create_companion_rcs_command_buffer(cmd_buffer);
   /* Re-emit the aux table register in every command buffer.  This way we're
   * ensured that we have the table even if this command buffer doesn't
   * initialize any images.
   */
   if (cmd_buffer->device->info->has_aux_map) {
      assert(cmd_buffer->companion_rcs_cmd_buffer != NULL);
   anv_add_pending_pipe_bits(cmd_buffer->companion_rcs_cmd_buffer,
                        assert(cmd_buffer->companion_rcs_cmd_buffer != NULL);
      }
      static void
   end_main_rcs_cmd_buffer_done(struct anv_cmd_buffer *cmd_buffer,
         {
      const struct intel_device_info *info = cmd_buffer->device->info;
   anv_genX(info, cmd_buffer_end_companion_rcs_syncpoint)(cmd_buffer,
      }
      static bool
   anv_blorp_execute_on_companion(struct anv_cmd_buffer *cmd_buffer,
         {
      /* MSAA images have to be dealt with on the companion RCS command buffer
   * for both CCS && BCS engines.
   */
   if ((anv_cmd_buffer_is_blitter_queue(cmd_buffer) ||
      anv_cmd_buffer_is_compute_queue(cmd_buffer)) &&
   dst_image->vk.samples > 1)
         /* Emulation of formats is done through a compute shader, so we need
   * the companion command buffer for the BCS engine.
   */
   if (anv_cmd_buffer_is_blitter_queue(cmd_buffer) &&
      dst_image->emu_plane_format != VK_FORMAT_UNDEFINED)
            }
      void anv_CmdCopyImage2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_image, src_image, pCopyImageInfo->srcImage);
            struct anv_cmd_buffer *main_cmd_buffer = cmd_buffer;
            if (anv_blorp_execute_on_companion(cmd_buffer, dst_image)) {
      rcs_done = record_main_rcs_cmd_buffer_done(cmd_buffer);
               struct blorp_batch batch;
            for (unsigned r = 0; r < pCopyImageInfo->regionCount; r++) {
      copy_image(cmd_buffer, &batch,
            src_image, pCopyImageInfo->srcImageLayout,
                     if (dst_image->emu_plane_format != VK_FORMAT_UNDEFINED) {
      assert(!anv_cmd_buffer_is_blitter_queue(cmd_buffer));
   const enum anv_pipe_bits pipe_bits =
      anv_cmd_buffer_is_compute_queue(cmd_buffer) ?
   ANV_PIPE_HDC_PIPELINE_FLUSH_BIT :
      anv_add_pending_pipe_bits(cmd_buffer, pipe_bits,
            for (unsigned r = 0; r < pCopyImageInfo->regionCount; r++) {
      const VkImageCopy2 *region = &pCopyImageInfo->pRegions[r];
   const VkOffset3D block_offset = vk_image_offset_to_elements(
         const VkExtent3D block_extent = vk_image_extent_to_elements(
         anv_astc_emu_process(cmd_buffer, dst_image,
                              if (rcs_done.alloc_size)
      }
      static enum isl_format
   isl_format_for_size(unsigned size_B)
   {
      /* Prefer 32-bit per component formats for CmdFillBuffer */
   switch (size_B) {
   case 1:  return ISL_FORMAT_R8_UINT;
   case 2:  return ISL_FORMAT_R16_UINT;
   case 3:  return ISL_FORMAT_R8G8B8_UINT;
   case 4:  return ISL_FORMAT_R32_UINT;
   case 6:  return ISL_FORMAT_R16G16B16_UINT;
   case 8:  return ISL_FORMAT_R32G32_UINT;
   case 12: return ISL_FORMAT_R32G32B32_UINT;
   case 16: return ISL_FORMAT_R32G32B32A32_UINT;
   default:
            }
      static void
   copy_buffer_to_image(struct anv_cmd_buffer *cmd_buffer,
                        struct blorp_batch *batch,
   struct anv_buffer *anv_buffer,
      {
      struct {
      struct blorp_surf surf;
   uint32_t level;
               buffer.level = 0;
            if (buffer_to_image) {
      src = &buffer;
      } else {
      src = &image;
                        get_blorp_surf_for_anv_image(cmd_buffer, anv_image, aspect,
                                 image.offset =
                  VkExtent3D extent =
         if (anv_image->vk.image_type != VK_IMAGE_TYPE_3D) {
      image.offset.z = region->imageSubresource.baseArrayLayer;
   extent.depth =
      vk_image_subresource_layer_count(&anv_image->vk,
            const enum isl_format linear_format =
      anv_get_isl_format(cmd_buffer->device->info, anv_image->vk.format,
      const struct isl_format_layout *linear_fmtl =
            const struct vk_image_buffer_layout buffer_layout =
            /* Some formats have additional restrictions which may cause ISL to
   * fail to create a surface for us.  For example, YCbCr formats
   * have to have 2-pixel aligned strides.
   *
   * To avoid these issues, we always bind the buffer as if it's a
   * "normal" format like RGBA32_UINT.  Since we're using blorp_copy,
   * the format doesn't matter as long as it has the right bpb.
   */
   const VkExtent2D buffer_extent = {
      .width = DIV_ROUND_UP(extent.width, linear_fmtl->bw),
      };
   const enum isl_format buffer_format =
            struct isl_surf buffer_isl_surf;
   get_blorp_surf_for_anv_buffer(cmd_buffer->device,
                              if (&image == dst) {
      /* In this case, the source is the buffer and, since blorp takes its
   * copy dimensions in terms of the source format, we have to use the
   * scaled down version for compressed textures because the source
   * format is an RGB format.
   */
   extent.width = buffer_extent.width;
            anv_cmd_buffer_mark_image_written(cmd_buffer, anv_image,
                           for (unsigned z = 0; z < extent.depth; z++) {
      blorp_copy(batch, &src->surf, src->level, src->offset.z,
                        image.offset.z++;
         }
      void anv_CmdCopyBufferToImage2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, src_buffer, pCopyBufferToImageInfo->srcBuffer);
            struct anv_cmd_buffer *main_cmd_buffer = cmd_buffer;
            if (anv_blorp_execute_on_companion(cmd_buffer, dst_image)) {
      rcs_done = record_main_rcs_cmd_buffer_done(cmd_buffer);
               struct blorp_batch batch;
            for (unsigned r = 0; r < pCopyBufferToImageInfo->regionCount; r++) {
      copy_buffer_to_image(cmd_buffer, &batch, src_buffer, dst_image,
                              if (dst_image->emu_plane_format != VK_FORMAT_UNDEFINED) {
      assert(!anv_cmd_buffer_is_blitter_queue(cmd_buffer));
   const enum anv_pipe_bits pipe_bits =
      anv_cmd_buffer_is_compute_queue(cmd_buffer) ?
   ANV_PIPE_HDC_PIPELINE_FLUSH_BIT :
      anv_add_pending_pipe_bits(cmd_buffer, pipe_bits,
            for (unsigned r = 0; r < pCopyBufferToImageInfo->regionCount; r++) {
      const VkBufferImageCopy2 *region =
         const VkOffset3D block_offset = vk_image_offset_to_elements(
         const VkExtent3D block_extent = vk_image_extent_to_elements(
         anv_astc_emu_process(cmd_buffer, dst_image,
                              if (rcs_done.alloc_size)
      }
      static void
   anv_add_buffer_write_pending_bits(struct anv_cmd_buffer *cmd_buffer,
         {
               cmd_buffer->state.queries.buffer_write_bits |=
      (cmd_buffer->queue_family->queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 ?
   ANV_QUERY_COMPUTE_WRITES_PENDING_BITS :
   }
      void anv_CmdCopyImageToBuffer2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_image, src_image, pCopyImageToBufferInfo->srcImage);
            struct blorp_batch batch;
            for (unsigned r = 0; r < pCopyImageToBufferInfo->regionCount; r++) {
      copy_buffer_to_image(cmd_buffer, &batch, dst_buffer, src_image,
                                 }
      static bool
   flip_coords(unsigned *src0, unsigned *src1, unsigned *dst0, unsigned *dst1)
   {
      bool flip = false;
   if (*src0 > *src1) {
      unsigned tmp = *src0;
   *src0 = *src1;
   *src1 = tmp;
               if (*dst0 > *dst1) {
      unsigned tmp = *dst0;
   *dst0 = *dst1;
   *dst1 = tmp;
                  }
      static void
   blit_image(struct anv_cmd_buffer *cmd_buffer,
            struct blorp_batch *batch,
   struct anv_image *src_image,
   VkImageLayout src_image_layout,
   struct anv_image *dst_image,
   VkImageLayout dst_image_layout,
      {
      const VkImageSubresourceLayers *src_res = &region->srcSubresource;
                     enum blorp_filter blorp_filter;
   switch (filter) {
   case VK_FILTER_NEAREST:
      blorp_filter = BLORP_FILTER_NEAREST;
      case VK_FILTER_LINEAR:
      blorp_filter = BLORP_FILTER_BILINEAR;
      default:
                  assert(anv_image_aspects_compatible(src_res->aspectMask,
            anv_foreach_image_aspect_bit(aspect_bit, src_image, src_res->aspectMask) {
      get_blorp_surf_for_anv_image(cmd_buffer,
                     get_blorp_surf_for_anv_image(cmd_buffer,
                                 if (src_image->emu_plane_format != VK_FORMAT_UNDEFINED) {
      /* redirect src to the hidden plane */
   const uint32_t plane = src_image->n_planes;
   const struct anv_surface *surface =
         const struct anv_address address =
                                    struct anv_format_plane src_format =
      anv_get_format_aspect(cmd_buffer->device->info, src_vk_format,
      struct anv_format_plane dst_format =
                  unsigned dst_start, dst_end;
   if (dst_image->vk.image_type == VK_IMAGE_TYPE_3D) {
      assert(dst_res->baseArrayLayer == 0);
   dst_start = region->dstOffsets[0].z;
      } else {
      dst_start = dst_res->baseArrayLayer;
   dst_end = dst_start +
               unsigned src_start, src_end;
   if (src_image->vk.image_type == VK_IMAGE_TYPE_3D) {
      assert(src_res->baseArrayLayer == 0);
   src_start = region->srcOffsets[0].z;
      } else {
      src_start = src_res->baseArrayLayer;
   src_end = src_start +
               bool flip_z = flip_coords(&src_start, &src_end, &dst_start, &dst_end);
   const unsigned num_layers = dst_end - dst_start;
            /* There is no interpolation to the pixel center during rendering, so
   * add the 0.5 offset ourselves here. */
   float depth_center_offset = 0;
   if (src_image->vk.image_type == VK_IMAGE_TYPE_3D)
            if (flip_z) {
      src_start = src_end;
   src_z_step *= -1;
               unsigned src_x0 = region->srcOffsets[0].x;
   unsigned src_x1 = region->srcOffsets[1].x;
   unsigned dst_x0 = region->dstOffsets[0].x;
   unsigned dst_x1 = region->dstOffsets[1].x;
            unsigned src_y0 = region->srcOffsets[0].y;
   unsigned src_y1 = region->srcOffsets[1].y;
   unsigned dst_y0 = region->dstOffsets[0].y;
   unsigned dst_y1 = region->dstOffsets[1].y;
            anv_cmd_buffer_mark_image_written(cmd_buffer, dst_image,
                              for (unsigned i = 0; i < num_layers; i++) {
                     blorp_blit(batch, &src, src_res->mipLevel, src_z,
            src_format.isl_format, src_format.swizzle,
   &dst, dst_res->mipLevel, dst_z,
   dst_format.isl_format, dst_format.swizzle,
   src_x0, src_y0, src_x1, src_y1,
         }
      void anv_CmdBlitImage2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_image, src_image, pBlitImageInfo->srcImage);
            struct blorp_batch batch;
            for (unsigned r = 0; r < pBlitImageInfo->regionCount; r++) {
      blit_image(cmd_buffer, &batch,
            src_image, pBlitImageInfo->srcImageLayout,
               }
      /**
   * Returns the greatest common divisor of a and b that is a power of two.
   */
   static uint64_t
   gcd_pow2_u64(uint64_t a, uint64_t b)
   {
               unsigned a_log2 = ffsll(a) - 1;
            /* If either a or b is 0, then a_log2 or b_log2 till be UINT_MAX in which
   * case, the MIN2() will take the other one.  If both are 0 then we will
   * hit the assert above.
   */
      }
      /* This is maximum possible width/height our HW can handle */
   #define MAX_SURFACE_DIM (1ull << 14)
      static void
   copy_buffer(struct anv_device *device,
               struct blorp_batch *batch,
   struct anv_buffer *src_buffer,
   {
      struct blorp_address src = {
      .buffer = src_buffer->address.bo,
   .offset = src_buffer->address.offset + region->srcOffset,
   .mocs = anv_mocs(device, src_buffer->address.bo,
      };
   struct blorp_address dst = {
      .buffer = dst_buffer->address.bo,
   .offset = dst_buffer->address.offset + region->dstOffset,
   .mocs = anv_mocs(device, dst_buffer->address.bo,
                  }
      void anv_CmdCopyBuffer2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_buffer, src_buffer, pCopyBufferInfo->srcBuffer);
            struct blorp_batch batch;
            for (unsigned r = 0; r < pCopyBufferInfo->regionCount; r++) {
      copy_buffer(cmd_buffer->device, &batch, src_buffer, dst_buffer,
                           }
         void anv_CmdUpdateBuffer(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    dstBuffer,
   VkDeviceSize                                dstOffset,
   VkDeviceSize                                dataSize,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            struct blorp_batch batch;
            /* We can't quite grab a full block because the state stream needs a
   * little data at the top to build its linked list.
   */
   const uint32_t max_update_size =
                     /* We're about to read data that was written from the CPU.  Flush the
   * texture cache so we don't get anything stale.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                  while (dataSize) {
               struct anv_state tmp_data =
                     struct blorp_address src = {
      .buffer = cmd_buffer->device->dynamic_state_pool.block_pool.bo,
   .offset = tmp_data.offset,
   .mocs = isl_mocs(&cmd_buffer->device->isl_dev,
      };
   struct blorp_address dst = {
      .buffer = dst_buffer->address.bo,
   .offset = dst_buffer->address.offset + dstOffset,
   .mocs = anv_mocs(cmd_buffer->device, dst_buffer->address.bo,
                        dataSize -= copy_size;
   dstOffset += copy_size;
                           }
      void
   anv_cmd_buffer_fill_area(struct anv_cmd_buffer *cmd_buffer,
                     {
      struct blorp_surf surf;
            struct blorp_batch batch;
            /* First, we compute the biggest format that can be used with the
   * given offsets and size.
   */
   int bs = 16;
   uint64_t offset = address.offset;
   bs = gcd_pow2_u64(bs, offset);
   bs = gcd_pow2_u64(bs, size);
            union isl_color_value color = {
                  const uint64_t max_fill_size = MAX_SURFACE_DIM * MAX_SURFACE_DIM * bs;
   while (size >= max_fill_size) {
      get_blorp_surf_for_anv_address(cmd_buffer->device,
                                          blorp_clear(&batch, &surf, isl_format, ISL_SWIZZLE_IDENTITY,
               size -= max_fill_size;
               uint64_t height = size / (MAX_SURFACE_DIM * bs);
   assert(height < MAX_SURFACE_DIM);
   if (height != 0) {
      const uint64_t rect_fill_size = height * MAX_SURFACE_DIM * bs;
   get_blorp_surf_for_anv_address(cmd_buffer->device,
                                          blorp_clear(&batch, &surf, isl_format, ISL_SWIZZLE_IDENTITY,
               size -= rect_fill_size;
               if (size != 0) {
      const uint32_t width = size / bs;
   get_blorp_surf_for_anv_address(cmd_buffer->device,
                                          blorp_clear(&batch, &surf, isl_format, ISL_SWIZZLE_IDENTITY,
                        }
      void anv_CmdFillBuffer(
      VkCommandBuffer                             commandBuffer,
   VkBuffer                                    dstBuffer,
   VkDeviceSize                                dstOffset,
   VkDeviceSize                                fillSize,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
                     /* From the Vulkan spec:
   *
   *    "size is the number of bytes to fill, and must be either a multiple
   *    of 4, or VK_WHOLE_SIZE to fill the range from offset to the end of
   *    the buffer. If VK_WHOLE_SIZE is used and the remaining size of the
   *    buffer is not a multiple of 4, then the nearest smaller multiple is
   *    used."
   */
            anv_cmd_buffer_fill_area(cmd_buffer,
                     }
      void anv_CmdClearColorImage(
      VkCommandBuffer                             commandBuffer,
   VkImage                                     _image,
   VkImageLayout                               imageLayout,
   const VkClearColorValue*                    pColor,
   uint32_t                                    rangeCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            struct anv_cmd_buffer *main_cmd_buffer = cmd_buffer;
            if (anv_blorp_execute_on_companion(cmd_buffer, image)) {
      rcs_done = record_main_rcs_cmd_buffer_done(cmd_buffer);
               struct blorp_batch batch;
            for (unsigned r = 0; r < rangeCount; r++) {
      if (pRanges[r].aspectMask == 0)
                     struct blorp_surf surf;
   get_blorp_surf_for_anv_image(cmd_buffer,
                        struct anv_format_plane src_format =
                  unsigned base_layer = pRanges[r].baseArrayLayer;
   uint32_t layer_count =
         uint32_t level_count =
            for (uint32_t i = 0; i < level_count; i++) {
      const unsigned level = pRanges[r].baseMipLevel + i;
                  if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
      base_layer = 0;
               anv_cmd_buffer_mark_image_written(cmd_buffer, image,
                        blorp_clear(&batch, &surf,
               src_format.isl_format, src_format.swizzle,
                           if (rcs_done.alloc_size)
      }
      void anv_CmdClearDepthStencilImage(
      VkCommandBuffer                             commandBuffer,
   VkImage                                     image_h,
   VkImageLayout                               imageLayout,
   const VkClearDepthStencilValue*             pDepthStencil,
   uint32_t                                    rangeCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch, 0);
            struct blorp_surf depth, stencil;
   if (image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      get_blorp_surf_for_anv_image(cmd_buffer,
                  } else {
                  if (image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      get_blorp_surf_for_anv_image(cmd_buffer,
                  } else {
                  for (unsigned r = 0; r < rangeCount; r++) {
      if (pRanges[r].aspectMask == 0)
            bool clear_depth = pRanges[r].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT;
            unsigned base_layer = pRanges[r].baseArrayLayer;
   uint32_t layer_count =
         uint32_t level_count =
            for (uint32_t i = 0; i < level_count; i++) {
      const unsigned level = pRanges[r].baseMipLevel + i;
                                 blorp_clear_depth_stencil(&batch, &depth, &stencil,
                                             }
      VkResult
   anv_cmd_buffer_alloc_blorp_binding_table(struct anv_cmd_buffer *cmd_buffer,
                     {
      *bt_state = anv_cmd_buffer_alloc_binding_table(cmd_buffer, num_entries,
         if (bt_state->map == NULL) {
      /* We ran out of space.  Grab a new binding table block. */
   VkResult result = anv_cmd_buffer_new_binding_table_block(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Re-emit state base addresses so we get the new surface state base
   * address before we start emitting binding tables etc.
   */
            *bt_state = anv_cmd_buffer_alloc_binding_table(cmd_buffer, num_entries,
                        }
      static VkResult
   binding_table_for_surface_state(struct anv_cmd_buffer *cmd_buffer,
               {
      uint32_t state_offset;
            VkResult result =
      anv_cmd_buffer_alloc_blorp_binding_table(cmd_buffer, 1, &state_offset,
      if (result != VK_SUCCESS)
            uint32_t *bt_map = bt_state.map;
            *bt_offset = bt_state.offset;
      }
      static bool
   can_fast_clear_color_att(struct anv_cmd_buffer *cmd_buffer,
                           {
      union isl_color_value clear_color =
            if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            /* We don't support fast clearing with conditional rendering at the
   * moment. All the tracking done around fast clears (clear color updates
   * and fast-clear type updates) happens unconditionally.
   */
   if (batch->flags & BLORP_BATCH_PREDICATE_ENABLE)
            if (rectCount > 1) {
      anv_perf_warn(VK_LOG_OBJS(&cmd_buffer->device->vk.base),
                     /* We only support fast-clears on the first layer */
   if (pRects[0].layerCount > 1 || pRects[0].baseArrayLayer > 0)
            bool is_multiview = cmd_buffer->state.gfx.view_mask != 0;
   if (is_multiview && (cmd_buffer->state.gfx.view_mask != 1))
            return anv_can_fast_clear_color_view(cmd_buffer->device,
                                    }
      static void
   exec_ccs_op(struct anv_cmd_buffer *cmd_buffer,
               struct blorp_batch *batch,
   const struct anv_image *image,
   enum isl_format format, struct isl_swizzle swizzle,
   VkImageAspectFlagBits aspect, uint32_t level,
   {
      assert(image->vk.aspects & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV);
   assert(image->vk.samples == 1);
   assert(level < anv_image_aux_levels(image, aspect));
   /* Multi-LOD YcBcR is not allowed */
   assert(image->n_planes == 1 || level == 0);
   assert(base_layer + layer_count <=
            const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
            struct blorp_surf surf;
   get_blorp_surf_for_anv_image(cmd_buffer, image, aspect,
                        uint32_t level_width = u_minify(surf.surf->logical_level0_px.w, level);
            /* Blorp will store the clear color for us if we provide the clear color
   * address and we are doing a fast clear. So we save the clear value into
   * the blorp surface.
   */
   if (clear_value)
            char flush_reason[64];
   int ret =
      snprintf(flush_reason, sizeof(flush_reason),
               /* From the Sky Lake PRM Vol. 7, "Render Target Fast Clear":
   *
   *    "After Render target fast clear, pipe-control with color cache
   *    write-flush must be issued before sending any DRAW commands on
   *    that render target."
   *
   * This comment is a bit cryptic and doesn't really tell you what's going
   * or what's really needed.  It appears that fast clear ops are not
   * properly synchronized with other drawing.  This means that we cannot
   * have a fast clear operation in the pipe at the same time as other
   * regular drawing operations.  We need to use a PIPE_CONTROL to ensure
   * that the contents of the previous draw hit the render target before we
   * resolve and then use a second PIPE_CONTROL after the resolve to ensure
   * that it is completed before any additional drawing occurs.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                           ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
   ANV_PIPE_TILE_CACHE_FLUSH_BIT |
   (devinfo->verx10 == 120 ?
         (devinfo->verx10 == 125 ?
            switch (ccs_op) {
   case ISL_AUX_OP_FAST_CLEAR:
      /* From the ICL PRMs, Volume 9: Render Engine, State Caching :
   *
   *    "Any values referenced by pointers within the RENDER_SURFACE_STATE
   *     or SAMPLER_STATE (e.g. Clear Color Pointer, Border Color or
   *     Indirect State Pointer) are considered to be part of that state
   *     and any changes to these referenced values requires an
   *     invalidation of the L1 state cache to ensure the new values are
   *     being used as part of the state. In the case of surface data
   *     pointed to by the Surface Base Address in RENDER SURFACE STATE,
   *     the Texture Cache must be invalidated if the surface data
   *     changes."
   *
   * and From the Render Target Fast Clear section,
   *
   *   "HwManaged FastClear allows SW to store FastClearValue in separate
   *   graphics allocation, instead of keeping them in
   *   RENDER_SURFACE_STATE. This behavior can be enabled by setting
   *   ClearValueAddressEnable in RENDER_SURFACE_STATE.
   *
   *    Proper sequence of commands is as follows:
   *
   *       1. Storing clear color to allocation
   *       2. Ensuring that step 1. is finished and visible for TextureCache
   *       3. Performing FastClear
   *
   *    Step 2. is required on products with ClearColorConversion feature.
   *    This feature is enabled by setting ClearColorConversionEnable.
   *    This causes HW to read stored color from ClearColorAllocation and
   *    write back with the native format or RenderTarget - and clear
   *    color needs to be present and visible. Reading is done from
   *    TextureCache, writing is done to RenderCache."
   *
   * We're going to change the clear color. Invalidate the texture cache
   * now to ensure the clear color conversion feature works properly.
   * Although the docs seem to require invalidating the texture cache
   * after updating the clear color allocation, we can do this beforehand
   * so long as we ensure:
   *
   *    1. Step 1 is complete before the texture cache is accessed in step 3
   *    2. We don't access the texture cache between invalidation and step 3
   *
   * The second requirement is satisfied because we'll be performing step
   * 1 and 3 right after invalidating. The first is satisfied because
   * BLORP updates the clear color before performing the fast clear and it
   * performs the synchronizations suggested by the Render Target Fast
   * Clear section (not quoted here) to ensure its completion.
   *
   * While we're here, also invalidate the state cache as suggested.
   */
   if (devinfo->ver >= 11) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           blorp_fast_clear(batch, &surf, format, swizzle,
                  case ISL_AUX_OP_FULL_RESOLVE:
   case ISL_AUX_OP_PARTIAL_RESOLVE: {
      /* Wa_1508744258: Enable RHWO optimization for resolves */
   const bool enable_rhwo_opt =
            if (enable_rhwo_opt)
            blorp_ccs_resolve(batch, &surf, level, base_layer, layer_count,
            if (enable_rhwo_opt)
            }
   case ISL_AUX_OP_AMBIGUATE:
      for (uint32_t a = 0; a < layer_count; a++) {
      const uint32_t layer = base_layer + a;
      }
      default:
                  anv_add_pending_pipe_bits(cmd_buffer,
                           ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
   (devinfo->verx10 == 120 ?
      }
      static void
   exec_mcs_op(struct anv_cmd_buffer *cmd_buffer,
               struct blorp_batch *batch,
   const struct anv_image *image,
   enum isl_format format, struct isl_swizzle swizzle,
   VkImageAspectFlagBits aspect,
   {
      assert(image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT);
   assert(image->vk.samples > 1);
            /* Multisampling with multi-planar formats is not supported */
            const struct intel_device_info *devinfo = cmd_buffer->device->info;
   struct blorp_surf surf;
   get_blorp_surf_for_anv_image(cmd_buffer, image, aspect,
                  /* Blorp will store the clear color for us if we provide the clear color
   * address and we are doing a fast clear. So we save the clear value into
   * the blorp surface.
   */
   if (clear_value)
            /* From the Sky Lake PRM Vol. 7, "Render Target Fast Clear":
   *
   *    "After Render target fast clear, pipe-control with color cache
   *    write-flush must be issued before sending any DRAW commands on
   *    that render target."
   *
   * This comment is a bit cryptic and doesn't really tell you what's going
   * or what's really needed.  It appears that fast clear ops are not
   * properly synchronized with other drawing.  This means that we cannot
   * have a fast clear operation in the pipe at the same time as other
   * regular drawing operations.  We need to use a PIPE_CONTROL to ensure
   * that the contents of the previous draw hit the render target before we
   * resolve and then use a second PIPE_CONTROL after the resolve to ensure
   * that it is completed before any additional drawing occurs.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                           ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
   ANV_PIPE_TILE_CACHE_FLUSH_BIT |
   (devinfo->verx10 == 120 ?
         (devinfo->verx10 == 125 ?
            switch (mcs_op) {
   case ISL_AUX_OP_FAST_CLEAR:
      /* From the ICL PRMs, Volume 9: Render Engine, State Caching :
   *
   *    "Any values referenced by pointers within the RENDER_SURFACE_STATE
   *     or SAMPLER_STATE (e.g. Clear Color Pointer, Border Color or
   *     Indirect State Pointer) are considered to be part of that state
   *     and any changes to these referenced values requires an
   *     invalidation of the L1 state cache to ensure the new values are
   *     being used as part of the state. In the case of surface data
   *     pointed to by the Surface Base Address in RENDER SURFACE STATE,
   *     the Texture Cache must be invalidated if the surface data
   *     changes."
   *
   * and From the Render Target Fast Clear section,
   *
   *   "HwManaged FastClear allows SW to store FastClearValue in separate
   *   graphics allocation, instead of keeping them in
   *   RENDER_SURFACE_STATE. This behavior can be enabled by setting
   *   ClearValueAddressEnable in RENDER_SURFACE_STATE.
   *
   *    Proper sequence of commands is as follows:
   *
   *       1. Storing clear color to allocation
   *       2. Ensuring that step 1. is finished and visible for TextureCache
   *       3. Performing FastClear
   *
   *    Step 2. is required on products with ClearColorConversion feature.
   *    This feature is enabled by setting ClearColorConversionEnable.
   *    This causes HW to read stored color from ClearColorAllocation and
   *    write back with the native format or RenderTarget - and clear
   *    color needs to be present and visible. Reading is done from
   *    TextureCache, writing is done to RenderCache."
   *
   * We're going to change the clear color. Invalidate the texture cache
   * now to ensure the clear color conversion feature works properly.
   * Although the docs seem to require invalidating the texture cache
   * after updating the clear color allocation, we can do this beforehand
   * so long as we ensure:
   *
   *    1. Step 1 is complete before the texture cache is accessed in step 3
   *    2. We don't access the texture cache between invalidation and step 3
   *
   * The second requirement is satisfied because we'll be performing step
   * 1 and 3 right after invalidating. The first is satisfied because
   * BLORP updates the clear color before performing the fast clear and it
   * performs the synchronizations suggested by the Render Target Fast
   * Clear section (not quoted here) to ensure its completion.
   *
   * While we're here, also invalidate the state cache as suggested.
   */
   if (devinfo->ver >= 11) {
      anv_add_pending_pipe_bits(cmd_buffer,
                           blorp_fast_clear(batch, &surf, format, swizzle,
                  case ISL_AUX_OP_PARTIAL_RESOLVE:
      blorp_mcs_partial_resolve(batch, &surf, format,
            case ISL_AUX_OP_AMBIGUATE:
      blorp_mcs_ambiguate(batch, &surf, base_layer, layer_count);
      case ISL_AUX_OP_FULL_RESOLVE:
   default:
                  anv_add_pending_pipe_bits(cmd_buffer,
                           ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT |
   (devinfo->verx10 == 120 ?
      }
      static void
   clear_color_attachment(struct anv_cmd_buffer *cmd_buffer,
                     {
      struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
   const uint32_t att_idx = attachment->colorAttachment;
   assert(att_idx < gfx->color_att_count);
            if (att->vk_format == VK_FORMAT_UNDEFINED)
            union isl_color_value clear_color =
            const struct anv_image_view *iview = att->iview;
   if (iview &&
      can_fast_clear_color_att(cmd_buffer, batch, att,
         if (iview->image->vk.samples == 1) {
      exec_ccs_op(cmd_buffer, batch, iview->image,
               iview->planes[0].isl.format,
   iview->planes[0].isl.swizzle,
      } else {
      exec_mcs_op(cmd_buffer, batch, iview->image,
               iview->planes[0].isl.format,
   iview->planes[0].isl.swizzle,
               anv_cmd_buffer_mark_image_fast_cleared(cmd_buffer, iview->image,
               anv_cmd_buffer_load_clear_color_from_image(cmd_buffer,
                           uint32_t binding_table;
   VkResult result =
      binding_table_for_surface_state(cmd_buffer, att->surface_state.state,
      if (result != VK_SUCCESS)
            /* If multiview is enabled we ignore baseArrayLayer and layerCount */
   if (gfx->view_mask) {
      u_foreach_bit(view_idx, gfx->view_mask) {
      for (uint32_t r = 0; r < rectCount; ++r) {
      const VkOffset2D offset = pRects[r].rect.offset;
   const VkExtent2D extent = pRects[r].rect.extent;
   blorp_clear_attachments(batch, binding_table,
                           ISL_FORMAT_UNSUPPORTED,
   gfx->samples,
         }
               for (uint32_t r = 0; r < rectCount; ++r) {
      const VkOffset2D offset = pRects[r].rect.offset;
   const VkExtent2D extent = pRects[r].rect.extent;
   assert(pRects[r].layerCount != VK_REMAINING_ARRAY_LAYERS);
   blorp_clear_attachments(batch, binding_table,
                           ISL_FORMAT_UNSUPPORTED,
   gfx->samples,
         }
      static void
   anv_fast_clear_depth_stencil(struct anv_cmd_buffer *cmd_buffer,
                              struct blorp_batch *batch,
      {
      assert(image->vk.aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
            struct blorp_surf depth = {};
   if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      const uint32_t plane =
         assert(base_layer + layer_count <=
         get_blorp_surf_for_anv_image(cmd_buffer,
                           struct blorp_surf stencil = {};
   if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      const uint32_t plane =
         get_blorp_surf_for_anv_image(cmd_buffer,
                           /* From the Sky Lake PRM Volume 7, "Depth Buffer Clear":
   *
   *    "The following is required when performing a depth buffer clear with
   *    using the WM_STATE or 3DSTATE_WM:
   *
   *       * If other rendering operations have preceded this clear, a
   *         PIPE_CONTROL with depth cache flush enabled, Depth Stall bit
   *         enabled must be issued before the rectangle primitive used for
   *         the depth buffer clear operation.
   *       * [...]"
   *
   * Even though the PRM only says that this is required if using 3DSTATE_WM
   * and a 3DPRIMITIVE, the GPU appears to also need this to avoid occasional
   * hangs when doing a clear with WM_HZ_OP.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                        if ((aspects & VK_IMAGE_ASPECT_DEPTH_BIT) &&
      depth.aux_usage == ISL_AUX_USAGE_HIZ_CCS_WT) {
   /* From Bspec 47010 (Depth Buffer Clear):
   *
   *    Since the fast clear cycles to CCS are not cached in TileCache,
   *    any previous depth buffer writes to overlapping pixels must be
   *    flushed out of TileCache before a succeeding Depth Buffer Clear.
   *    This restriction only applies to Depth Buffer with write-thru
   *    enabled, since fast clears to CCS only occur for write-thru mode.
   *
   * There may have been a write to this depth buffer. Flush it from the
   * tile cache just in case.
   *
   * Set CS stall bit to guarantee that the fast clear starts the execution
   * after the tile cache flush completed.
   *
   * There is no Bspec requirement to flush the data cache but the
   * experiment shows that flusing the data cache helps to resolve the
   * corruption.
   */
   unsigned wa_flush = cmd_buffer->device->info->verx10 >= 125 ?
         anv_add_pending_pipe_bits(cmd_buffer,
                                       blorp_hiz_clear_depth_stencil(batch, &depth, &stencil,
                                 level, base_layer, layer_count,
   area.offset.x, area.offset.y,
            /* From the SKL PRM, Depth Buffer Clear:
   *
   *    "Depth Buffer Clear Workaround
   *
   *    Depth buffer clear pass using any of the methods (WM_STATE,
   *    3DSTATE_WM or 3DSTATE_WM_HZ_OP) must be followed by a PIPE_CONTROL
   *    command with DEPTH_STALL bit and Depth FLUSH bits “set” before
   *    starting to render.  DepthStall and DepthFlush are not needed between
   *    consecutive depth clear passes nor is it required if the depth-clear
   *    pass was done with “full_surf_clear” bit set in the
   *    3DSTATE_WM_HZ_OP."
   *
   * Even though the PRM provides a bunch of conditions under which this is
   * supposedly unnecessary, we choose to perform the flush unconditionally
   * just to be safe.
   *
   * From Bspec 46959, a programming note applicable to Gfx12+:
   *
   *    "Since HZ_OP has to be sent twice (first time set the clear/resolve state
   *    and 2nd time to clear the state), and HW internally flushes the depth
   *    cache on HZ_OP, there is no need to explicitly send a Depth Cache flush
   *    after Clear or Resolve."
   */
   if (cmd_buffer->device->info->verx10 < 120) {
      anv_add_pending_pipe_bits(cmd_buffer,
                     }
      static bool
   can_hiz_clear_att(struct anv_cmd_buffer *cmd_buffer,
                     struct blorp_batch *batch,
   {
      if (INTEL_DEBUG(DEBUG_NO_FAST_CLEAR))
            /* From Bspec's section MI_PREDICATE:
   *
   *    "The MI_PREDICATE command is used to control the Predicate state bit,
   *    which in turn can be used to enable/disable the processing of
   *    3DPRIMITIVE commands."
   *
   * Also from BDW/CHV Bspec's 3DSTATE_WM_HZ_OP programming notes:
   *
   *    "This command does NOT support predication from the use of the
   *    MI_PREDICATE register. To predicate depth clears and resolves on you
   *    must fall back to using the 3D_PRIMITIVE or GPGPU_WALKER commands."
   *
   * Since BLORP's predication is currently dependent on MI_PREDICATE, fall
   * back to the slow depth clear path when the BLORP_BATCH_PREDICATE_ENABLE
   * flag is set.
   */
   if (batch->flags & BLORP_BATCH_PREDICATE_ENABLE)
            if (rectCount > 1) {
      anv_perf_warn(VK_LOG_OBJS(&cmd_buffer->device->vk.base),
                     /* When the BLORP_BATCH_NO_EMIT_DEPTH_STENCIL flag is set, BLORP can only
   * clear the first slice of the currently configured depth/stencil view.
   */
   assert(batch->flags & BLORP_BATCH_NO_EMIT_DEPTH_STENCIL);
   if (pRects[0].layerCount > 1 || pRects[0].baseArrayLayer > 0)
            return anv_can_hiz_clear_ds_view(cmd_buffer->device, ds_att->iview,
                              }
      static void
   clear_depth_stencil_attachment(struct anv_cmd_buffer *cmd_buffer,
                     {
      static const union isl_color_value color_value = { .u32 = { 0, } };
   struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
   const struct anv_attachment *d_att = &gfx->depth_att;
   const struct anv_attachment *s_att = &gfx->stencil_att;
   if (d_att->vk_format == VK_FORMAT_UNDEFINED &&
      s_att->vk_format == VK_FORMAT_UNDEFINED)
         const struct anv_attachment *ds_att = d_att->iview ? d_att : s_att;
   if (ds_att->iview &&
      can_hiz_clear_att(cmd_buffer, batch, ds_att, attachment, rectCount, pRects)) {
   anv_fast_clear_depth_stencil(cmd_buffer, batch, ds_att->iview->image,
                                             bool clear_depth = attachment->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT;
            enum isl_format depth_format = ISL_FORMAT_UNSUPPORTED;
   if (d_att->vk_format != VK_FORMAT_UNDEFINED) {
      depth_format = anv_get_isl_format(cmd_buffer->device->info,
                           uint32_t binding_table;
   VkResult result =
      binding_table_for_surface_state(cmd_buffer,
            if (result != VK_SUCCESS)
            /* If multiview is enabled we ignore baseArrayLayer and layerCount */
   if (gfx->view_mask) {
      u_foreach_bit(view_idx, gfx->view_mask) {
      for (uint32_t r = 0; r < rectCount; ++r) {
      const VkOffset2D offset = pRects[r].rect.offset;
   const VkExtent2D extent = pRects[r].rect.extent;
   VkClearDepthStencilValue value = attachment->clearValue.depthStencil;
   blorp_clear_attachments(batch, binding_table,
                           depth_format,
   gfx->samples,
   view_idx, 1,
   offset.x, offset.y,
         }
               for (uint32_t r = 0; r < rectCount; ++r) {
      const VkOffset2D offset = pRects[r].rect.offset;
   const VkExtent2D extent = pRects[r].rect.extent;
   VkClearDepthStencilValue value = attachment->clearValue.depthStencil;
   assert(pRects[r].layerCount != VK_REMAINING_ARRAY_LAYERS);
   blorp_clear_attachments(batch, binding_table,
                           depth_format,
   gfx->samples,
   pRects[r].baseArrayLayer,
   pRects[r].layerCount,
         }
      void anv_CmdClearAttachments(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    attachmentCount,
   const VkClearAttachment*                    pAttachments,
   uint32_t                                    rectCount,
      {
               /* Because this gets called within a render pass, we tell blorp not to
   * trash our depth and stencil buffers.
   */
   struct blorp_batch batch;
   enum blorp_batch_flags flags = BLORP_BATCH_NO_EMIT_DEPTH_STENCIL;
   if (cmd_buffer->state.conditional_render_enabled) {
      anv_cmd_emit_conditional_render_predicate(cmd_buffer);
      }
            for (uint32_t a = 0; a < attachmentCount; ++a) {
      if (pAttachments[a].aspectMask & VK_IMAGE_ASPECT_ANY_COLOR_BIT_ANV) {
      assert(pAttachments[a].aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
   clear_color_attachment(cmd_buffer, &batch,
            } else {
      clear_depth_stencil_attachment(cmd_buffer, &batch,
                           }
      void
   anv_image_msaa_resolve(struct anv_cmd_buffer *cmd_buffer,
                        const struct anv_image *src_image,
   enum isl_aux_usage src_aux_usage,
   uint32_t src_level, uint32_t src_base_layer,
   const struct anv_image *dst_image,
   enum isl_aux_usage dst_aux_usage,
   uint32_t dst_level, uint32_t dst_base_layer,
   VkImageAspectFlagBits aspect,
   uint32_t src_x, uint32_t src_y,
      {
      struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch, 0);
            assert(src_image->vk.image_type == VK_IMAGE_TYPE_2D);
   assert(src_image->vk.samples > 1);
   assert(dst_image->vk.image_type == VK_IMAGE_TYPE_2D);
            struct blorp_surf src_surf, dst_surf;
   get_blorp_surf_for_anv_image(cmd_buffer, src_image, aspect,
                     if (src_aux_usage == ISL_AUX_USAGE_MCS) {
      src_surf.clear_color_addr = anv_to_blorp_address(
      anv_image_get_clear_color_addr(cmd_buffer->device, src_image,
   }
   get_blorp_surf_for_anv_image(cmd_buffer, dst_image, aspect,
                     anv_cmd_buffer_mark_image_written(cmd_buffer, dst_image,
                  if (filter == BLORP_FILTER_NONE) {
      /* If no explicit filter is provided, then it's implied by the type of
   * the source image.
   */
   if ((src_surf.surf->usage & ISL_SURF_USAGE_DEPTH_BIT) ||
      (src_surf.surf->usage & ISL_SURF_USAGE_STENCIL_BIT) ||
   isl_format_has_int_channel(src_surf.surf->format)) {
      } else {
                     for (uint32_t l = 0; l < layer_count; l++) {
      blorp_blit(&batch,
            &src_surf, src_level, src_base_layer + l,
   ISL_FORMAT_UNSUPPORTED, ISL_SWIZZLE_IDENTITY,
   &dst_surf, dst_level, dst_base_layer + l,
   ISL_FORMAT_UNSUPPORTED, ISL_SWIZZLE_IDENTITY,
   src_x, src_y, src_x + width, src_y + height,
               }
      static void
   resolve_image(struct anv_cmd_buffer *cmd_buffer,
               struct anv_image *src_image,
   VkImageLayout src_image_layout,
   struct anv_image *dst_image,
   {
      assert(region->srcSubresource.aspectMask == region->dstSubresource.aspectMask);
   assert(vk_image_subresource_layer_count(&src_image->vk, &region->srcSubresource) ==
            const uint32_t layer_count =
            anv_foreach_image_aspect_bit(aspect_bit, src_image,
            enum isl_aux_usage src_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info, src_image,
                        enum isl_aux_usage dst_aux_usage =
      anv_layout_to_aux_usage(cmd_buffer->device->info, dst_image,
                           anv_image_msaa_resolve(cmd_buffer,
                        src_image, src_aux_usage,
   region->srcSubresource.mipLevel,
   region->srcSubresource.baseArrayLayer,
   dst_image, dst_aux_usage,
   region->dstSubresource.mipLevel,
   region->dstSubresource.baseArrayLayer,
   (1 << aspect_bit),
   region->srcOffset.x,
   region->srcOffset.y,
   region->dstOffset.x,
      }
      void anv_CmdResolveImage2(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_image, src_image, pResolveImageInfo->srcImage);
            for (uint32_t r = 0; r < pResolveImageInfo->regionCount; r++) {
      resolve_image(cmd_buffer,
                     }
      void
   anv_image_clear_color(struct anv_cmd_buffer *cmd_buffer,
                        const struct anv_image *image,
   VkImageAspectFlagBits aspect,
      {
               /* We don't support planar images with multisampling yet */
            struct blorp_batch batch;
            struct blorp_surf surf;
   get_blorp_surf_for_anv_image(cmd_buffer, image, aspect,
                     anv_cmd_buffer_mark_image_written(cmd_buffer, image, aspect, aux_usage,
            blorp_clear(&batch, &surf, format, anv_swizzle_for_render(swizzle),
               level, base_layer, layer_count,
   area.offset.x, area.offset.y,
               }
      void
   anv_image_clear_depth_stencil(struct anv_cmd_buffer *cmd_buffer,
                                 const struct anv_image *image,
   VkImageAspectFlags aspects,
   {
      assert(image->vk.aspects & (VK_IMAGE_ASPECT_DEPTH_BIT |
            struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch, 0);
            struct blorp_surf depth = {};
   if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      get_blorp_surf_for_anv_image(cmd_buffer,
                           struct blorp_surf stencil = {};
   if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      const uint32_t plane =
         get_blorp_surf_for_anv_image(cmd_buffer,
                           /* Blorp may choose to clear stencil using RGBA32_UINT for better
   * performance.  If it does this, we need to flush it out of the depth
   * cache before rendering to it.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                        blorp_clear_depth_stencil(&batch, &depth, &stencil,
                           level, base_layer, layer_count,
   area.offset.x, area.offset.y,
   area.offset.x + area.extent.width,
            /* Blorp may choose to clear stencil using RGBA32_UINT for better
   * performance.  If it does this, we need to flush it out of the render
   * cache before someone starts trying to do stencil on it.
   */
   anv_add_pending_pipe_bits(cmd_buffer,
                           }
      void
   anv_image_hiz_op(struct anv_cmd_buffer *cmd_buffer,
                  const struct anv_image *image,
      {
      assert(aspect == VK_IMAGE_ASPECT_DEPTH_BIT);
   assert(base_layer + layer_count <= anv_image_aux_layers(image, aspect, level));
   const uint32_t plane = anv_image_aspect_to_plane(image, aspect);
            struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch, 0);
            struct blorp_surf surf;
   get_blorp_surf_for_anv_image(cmd_buffer,
                                    }
      void
   anv_image_hiz_clear(struct anv_cmd_buffer *cmd_buffer,
                     const struct anv_image *image,
   VkImageAspectFlags aspects,
   {
      struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch, 0);
            anv_fast_clear_depth_stencil(cmd_buffer, &batch, image, aspects, level,
               }
      void
   anv_image_mcs_op(struct anv_cmd_buffer *cmd_buffer,
                  const struct anv_image *image,
   enum isl_format format, struct isl_swizzle swizzle,
   VkImageAspectFlagBits aspect,
      {
      struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch,
                        exec_mcs_op(cmd_buffer, &batch, image, format, swizzle, aspect,
               }
      void
   anv_image_ccs_op(struct anv_cmd_buffer *cmd_buffer,
                  const struct anv_image *image,
   enum isl_format format, struct isl_swizzle swizzle,
   VkImageAspectFlagBits aspect, uint32_t level,
      {
      struct blorp_batch batch;
   anv_blorp_batch_init(cmd_buffer, &batch,
                        exec_ccs_op(cmd_buffer, &batch, image, format, swizzle, aspect, level,
               }
