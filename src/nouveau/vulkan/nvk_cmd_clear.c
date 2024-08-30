   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_cmd_buffer.h"
      #include "nvk_device.h"
   #include "nvk_entrypoints.h"
   #include "nvk_image.h"
   #include "nvk_image_view.h"
   #include "nvk_mme.h"
   #include "nvk_physical_device.h"
      #include "nil_format.h"
   #include "vk_format.h"
      #include "nvk_cl9097.h"
   #include "drf.h"
      void
   nvk_mme_clear(struct mme_builder *b)
   {
      struct mme_value payload = mme_load(b);
                     mme_if(b, ine, view_mask, mme_zero()) {
               mme_loop(b, mme_imm(32)) {
      mme_if(b, ine, mme_and(b, view_mask, bit), mme_zero()) {
      mme_mthd(b, NV9097_CLEAR_SURFACE);
               mme_add_to(b, payload, payload, mme_imm(arr_idx));
      }
               mme_if(b, ieq, view_mask, mme_zero()) {
               mme_loop(b, layer_count) {
                        }
               mme_free_reg(b, payload);
      }
      static void
   emit_clear_rects(struct nvk_cmd_buffer *cmd,
                  int color_att,
   bool clear_depth,
      {
                        for (uint32_t r = 0; r < rect_count; r++) {
      P_MTHD(p, NV9097, SET_CLEAR_RECT_HORIZONTAL);
   P_NV9097_SET_CLEAR_RECT_HORIZONTAL(p, {
      .xmin = rects[r].rect.offset.x,
      });
   P_NV9097_SET_CLEAR_RECT_VERTICAL(p, {
      .ymin = rects[r].rect.offset.y,
               uint32_t payload;
   V_NV9097_CLEAR_SURFACE(payload, {
      .z_enable       = clear_depth,
   .stencil_enable = clear_stencil,
   .r_enable       = color_att >= 0,
   .g_enable       = color_att >= 0,
   .b_enable       = color_att >= 0,
   .a_enable       = color_att >= 0,
   .mrt_select     = color_att >= 0 ? color_att : 0,
               P_1INC(p, NV9097, CALL_MME_MACRO(NVK_MME_CLEAR));
   P_INLINE_DATA(p, payload);
   if (render->view_mask == 0) {
               }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdClearAttachments(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            P_IMMD(p, NV9097, SET_CLEAR_SURFACE_CONTROL, {
      .respect_stencil_mask   = RESPECT_STENCIL_MASK_FALSE,
   .use_clear_rect         = USE_CLEAR_RECT_TRUE,
   .use_scissor0           = USE_SCISSOR0_FALSE,
               bool clear_depth = false, clear_stencil = false;
   for (uint32_t i = 0; i < attachmentCount; i++) {
      if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      P_IMMD(p, NV9097, SET_Z_CLEAR_VALUE,
                     if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      P_IMMD(p, NV9097, SET_STENCIL_CLEAR_VALUE,
                        for (uint32_t i = 0; i < attachmentCount; i++) {
      if (pAttachments[i].aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)
            if (pAttachments[i].colorAttachment == VK_ATTACHMENT_UNUSED)
            VkClearColorValue color = pAttachments[i].clearValue.color;
            P_MTHD(p, NV9097, SET_COLOR_CLEAR_VALUE(0));
   P_NV9097_SET_COLOR_CLEAR_VALUE(p, 0, color.uint32[0]);
   P_NV9097_SET_COLOR_CLEAR_VALUE(p, 1, color.uint32[1]);
   P_NV9097_SET_COLOR_CLEAR_VALUE(p, 2, color.uint32[2]);
            emit_clear_rects(cmd, pAttachments[i].colorAttachment,
            /* We only need to clear depth/stencil once */
               /* No color clears */
   if (clear_depth || clear_stencil)
      }
      static VkImageViewType
   render_view_type(VkImageType image_type, unsigned layer_count)
   {
      switch (image_type) {
   case VK_IMAGE_TYPE_1D:
      return layer_count == 1 ? VK_IMAGE_VIEW_TYPE_1D :
      case VK_IMAGE_TYPE_2D:
      return layer_count == 1 ? VK_IMAGE_VIEW_TYPE_2D :
      case VK_IMAGE_TYPE_3D:
         default:
            }
      static void
   clear_image(struct nvk_cmd_buffer *cmd,
               struct nvk_image *image,
   VkImageLayout image_layout,
   VkFormat format,
   const VkClearValue *clear_value,
   {
      struct nvk_device *dev = nvk_cmd_buffer_device(cmd);
            for (uint32_t r = 0; r < range_count; r++) {
      const uint32_t level_count =
            for (uint32_t l = 0; l < level_count; l++) {
                              uint32_t base_array_layer, layer_count;
   if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
      base_array_layer = 0;
      } else {
      base_array_layer = ranges[r].baseArrayLayer;
   layer_count = vk_image_subresource_layer_count(&image->vk,
               const VkImageViewUsageCreateInfo view_usage_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
   .usage = (ranges[r].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) ?
            };
   const VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .pNext = &view_usage_info,
   .image = nvk_image_to_handle(image),
   .viewType = render_view_type(image->vk.image_type, layer_count),
   .format = format,
   .subresourceRange = {
      .aspectMask = image->vk.aspects,
   .baseMipLevel = level,
   .levelCount = 1,
   .baseArrayLayer = base_array_layer,
                  struct nvk_image_view view;
                  VkRenderingInfo render = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = {
      .offset = { 0, 0 },
      },
               VkRenderingAttachmentInfo vk_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = nvk_image_view_to_handle(&view),
   .imageLayout = image_layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
               if (ranges[r].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      render.colorAttachmentCount = 1;
      }
   if (ranges[r].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
                                                }
      static VkFormat
   vk_packed_int_format_for_size(unsigned size_B)
   {
      switch (size_B) {
   case 1:  return VK_FORMAT_R8_UINT;
   case 2:  return VK_FORMAT_R16_UINT;
   case 4:  return VK_FORMAT_R32_UINT;
   case 8:  return VK_FORMAT_R32G32_UINT;
   case 16: return VK_FORMAT_R32G32B32A32_UINT;
   default: unreachable("Invalid image format size");
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdClearColorImage(VkCommandBuffer commandBuffer,
                        VkImage _image,
      {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
   struct nvk_device *dev = nvk_cmd_buffer_device(cmd);
            VkClearValue clear_value = {
                  VkFormat vk_format = image->vk.format;
   enum pipe_format p_format = vk_format_to_pipe_format(vk_format);
            if (!nil_format_supports_color_targets(&dev->pdev->info, p_format)) {
      memset(&clear_value, 0, sizeof(clear_value));
   util_format_pack_rgba(p_format, clear_value.color.uint32,
            unsigned bpp = util_format_get_blocksize(p_format);
               clear_image(cmd, image, imageLayout, vk_format,
      }
      VKAPI_ATTR void VKAPI_CALL
   nvk_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer,
                                 {
      VK_FROM_HANDLE(nvk_cmd_buffer, cmd, commandBuffer);
            const VkClearValue clear_value = {
                  clear_image(cmd, image, imageLayout, image->vk.format,
      }
