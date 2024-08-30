   /* Copyright (c) 2017-2023 Hans-Kristian Arntzen
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
      #include "radv_meta.h"
   #include "radv_private.h"
   #include "sid.h"
   #include "vk_common_entrypoints.h"
   #include "vk_format.h"
      VkResult
   radv_device_init_meta_astc_decode_state(struct radv_device *device, bool on_demand)
   {
               if (!device->physical_device->emulate_astc)
               }
      void
   radv_device_finish_meta_astc_decode_state(struct radv_device *device)
   {
      struct radv_meta_state *state = &device->meta_state;
            if (!device->physical_device->emulate_astc)
               }
      static void
   decode_astc(struct radv_cmd_buffer *cmd_buffer, struct radv_image_view *src_iview, struct radv_image_view *dst_iview,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_meta_state *state = &device->meta_state;
   struct vk_texcompress_astc_write_descriptor_set write_desc_set;
   VkFormat format = src_iview->image->vk.format;
   int blk_w = vk_format_get_blockwidth(format);
            vk_texcompress_astc_fill_write_descriptor_sets(state->astc_decode, &write_desc_set,
               radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->astc_decode->p_layout,
                  VkPipeline pipeline =
         if (pipeline == VK_NULL_HANDLE)
                     bool is_3Dimage = (src_iview->image->vk.image_type == VK_IMAGE_TYPE_3D) ? true : false;
   int push_constants[5] = {offset->x / blk_w, offset->y / blk_h, extent->width + offset->x, extent->height + offset->y,
         radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.etc_decode.pipeline_layout,
            struct radv_dispatch_info info = {
      .blocks[0] = DIV_ROUND_UP(extent->width, blk_w * 2),
   .blocks[1] = DIV_ROUND_UP(extent->height, blk_h * 2),
   .blocks[2] = extent->depth,
   .offsets[0] = 0,
   .offsets[1] = 0,
   .offsets[2] = offset->z,
      };
      }
      static VkImageViewType
   get_view_type(const struct radv_image *image)
   {
      switch (image->vk.image_type) {
   case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
         default:
            }
      static void
   image_view_init(struct radv_device *device, struct radv_image *image, VkFormat format, VkImageAspectFlags aspectMask,
         {
      VkImageViewCreateInfo iview_create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(image),
   .viewType = get_view_type(image),
   .format = format,
   .subresourceRange =
      {
      .aspectMask = aspectMask,
   .baseMipLevel = baseMipLevel,
   .levelCount = 1,
   .baseArrayLayer = 0,
                  }
      void
   radv_meta_decode_astc(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image, VkImageLayout layout,
         {
      struct radv_meta_saved_state saved_state;
   radv_meta_save(&saved_state, cmd_buffer,
                  uint32_t base_slice = radv_meta_get_iview_layer(image, subresource, &offset);
   uint32_t slice_count = image->vk.image_type == VK_IMAGE_TYPE_3D
                  extent = vk_image_sanitize_extent(&image->vk, extent);
            struct radv_image_view src_iview, dst_iview;
   image_view_init(cmd_buffer->device, image, VK_FORMAT_R32G32B32A32_UINT, VK_IMAGE_ASPECT_COLOR_BIT,
               image_view_init(cmd_buffer->device, image, VK_FORMAT_R8G8B8A8_UINT, VK_IMAGE_ASPECT_PLANE_1_BIT,
                  VkExtent3D extent_copy = {
      .width = extent.width,
   .height = extent.height,
      };
            radv_image_view_finish(&src_iview);
               }
