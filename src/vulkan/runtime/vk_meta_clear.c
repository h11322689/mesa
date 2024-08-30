   /*
   * Copyright © 2022 Collabora Ltd
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
      #include "vk_meta_private.h"
      #include "vk_command_buffer.h"
   #include "vk_device.h"
   #include "vk_format.h"
   #include "vk_image.h"
   #include "vk_pipeline.h"
   #include "vk_util.h"
      #include "nir_builder.h"
      struct vk_meta_clear_key {
      enum vk_meta_object_key_type key_type;
   struct vk_meta_rendering_info render;
   uint8_t color_attachments_cleared;
   bool clear_depth;
      };
      struct vk_meta_clear_push_data {
         };
      static nir_shader *
   build_clear_shader(const struct vk_meta_clear_key *key)
   {
      nir_builder build = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
                  struct glsl_struct_field push_field = {
      .type = glsl_array_type(glsl_vec4_type(),
                  };
   const struct glsl_type *push_iface_type =
      glsl_interface_type(&push_field, 1, GLSL_INTERFACE_PACKING_STD140,
         nir_variable *push = nir_variable_create(b->shader, nir_var_mem_push_const,
         nir_deref_instr *push_arr =
            u_foreach_bit(a, key->color_attachments_cleared) {
      nir_def *color_value =
            const struct glsl_type *out_type;
   if (vk_format_is_int(key->render.color_attachment_formats[a]))
         else if (vk_format_is_uint(key->render.color_attachment_formats[a]))
         else
            char out_name[8];
            nir_variable *out = nir_variable_create(b->shader, nir_var_shader_out,
                                 }
      static VkResult
   get_clear_pipeline_layout(struct vk_device *device,
               {
               VkPipelineLayout from_cache =
         if (from_cache != VK_NULL_HANDLE) {
      *layout_out = from_cache;
               const VkPushConstantRange push_range = {
      .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
   .offset = 0,
               const VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .pushConstantRangeCount = 1,
               return vk_meta_create_pipeline_layout(device, meta, &info,
      }
      static VkResult
   get_clear_pipeline(struct vk_device *device,
                     struct vk_meta_device *meta,
   {
      VkPipeline from_cache = vk_meta_lookup_pipeline(meta, key, sizeof(*key));
   if (from_cache != VK_NULL_HANDLE) {
      *pipeline_out = from_cache;
               const VkPipelineShaderStageNirCreateInfoMESA fs_nir_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_NIR_CREATE_INFO_MESA,
      };
   const VkPipelineShaderStageCreateInfo fs_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .pNext = &fs_nir_info,
   .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
               VkPipelineDepthStencilStateCreateInfo ds_info = {
         };
   const VkDynamicState dyn_stencil_ref = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
   VkPipelineDynamicStateCreateInfo dyn_info = {
         };
   if (key->clear_depth) {
      ds_info.depthTestEnable = VK_TRUE;
   ds_info.depthWriteEnable = VK_TRUE;
      }
   if (key->clear_stencil) {
      ds_info.stencilTestEnable = VK_TRUE;
   ds_info.front.compareOp = VK_COMPARE_OP_ALWAYS;
   ds_info.front.passOp = VK_STENCIL_OP_REPLACE;
   ds_info.front.compareMask = ~0u;
   ds_info.front.writeMask = ~0u;
   ds_info.back = ds_info.front;
   dyn_info.dynamicStateCount = 1;
               const VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .stageCount = 1,
   .pStages = &fs_info,
   .pDepthStencilState = &ds_info,
   .pDynamicState = &dyn_info,
               VkResult result = vk_meta_create_graphics_pipeline(device, meta, &info,
                                 }
      static int
   vk_meta_rect_cmp_layer(const void *_a, const void *_b)
   {
      const struct vk_meta_rect *a = _a, *b = _b;
   assert(a->layer <= INT_MAX && b->layer <= INT_MAX);
      }
      void
   vk_meta_clear_attachments(struct vk_command_buffer *cmd,
                           struct vk_meta_device *meta,
   const struct vk_meta_rendering_info *render,
   {
      struct vk_device *device = cmd->base.device;
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            struct vk_meta_clear_key key;
   memset(&key, 0, sizeof(key));
   key.key_type = VK_META_OBJECT_KEY_CLEAR_PIPELINE;
            struct vk_meta_clear_push_data push = {0};
   float depth_value = 1.0f;
            for (uint32_t i = 0; i < attachment_count; i++) {
      if (attachments[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      const uint32_t a = attachments[i].colorAttachment;
                  assert(a < MESA_VK_MAX_COLOR_ATTACHMENTS);
                  key.color_attachments_cleared |= BITFIELD_BIT(a);
      }
   if (attachments[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
      key.clear_depth = true;
      }
   if (attachments[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
      key.clear_stencil = true;
                  VkPipelineLayout layout;
   result = get_clear_pipeline_layout(device, meta, &layout);
   if (unlikely(result != VK_SUCCESS)) {
      /* TODO: Report error */
               VkPipeline pipeline;
   result = get_clear_pipeline(device, meta, &key, layout, &pipeline);
   if (unlikely(result != VK_SUCCESS)) {
      /* TODO: Report error */
               disp->CmdBindPipeline(vk_command_buffer_to_handle(cmd),
            if (key.clear_stencil) {
      disp->CmdSetStencilReference(vk_command_buffer_to_handle(cmd),
                     disp->CmdPushConstants(vk_command_buffer_to_handle(cmd),
                  if (render->view_mask == 0) {
      if (clear_rect_count == 1 && clear_rects[0].layerCount > 1) {
      struct vk_meta_rect rect = {
      .x0 = clear_rects[0].rect.offset.x,
   .x1 = clear_rects[0].rect.offset.x +
         .y0 = clear_rects[0].rect.offset.y,
   .y1 = clear_rects[0].rect.offset.y +
         .z = depth_value,
                  } else {
      uint32_t max_rect_count = 0;
                           uint32_t rect_count = 0;
   for (uint32_t r = 0; r < clear_rect_count; r++) {
      struct vk_meta_rect rect = {
      .x0 = clear_rects[r].rect.offset.x,
   .x1 = clear_rects[r].rect.offset.x +
         .y0 = clear_rects[r].rect.offset.y,
   .y1 = clear_rects[r].rect.offset.y +
            };
   for (uint32_t a = 0; a < clear_rects[r].layerCount; a++) {
      rect.layer = clear_rects[r].baseArrayLayer + a;
                        /* If we have more than one clear rect, sort by layer in the hopes
   * the hardware more or less does all the clears for one layer before
   * moving on to the next, thus reducing cache thrashing.
                                 } else {
      const uint32_t rect_count = clear_rect_count *
                  uint32_t rect_idx = 0;
   u_foreach_bit(v, render->view_mask) {
      for (uint32_t r = 0; r < clear_rect_count; r++) {
      assert(clear_rects[r].baseArrayLayer == 0);
   assert(clear_rects[r].layerCount == 1);
   rects[rect_idx++] = (struct vk_meta_rect) {
      .x0 = clear_rects[r].rect.offset.x,
   .x1 = clear_rects[r].rect.offset.x +
         .y0 = clear_rects[r].rect.offset.y,
   .y1 = clear_rects[r].rect.offset.y +
         .z = depth_value,
            }
                           }
      void
   vk_meta_clear_rendering(struct vk_meta_device *meta,
               {
               struct vk_meta_rendering_info render = {
      .view_mask = pRenderingInfo->viewMask,
               uint32_t clear_count = 0;
   VkClearAttachment clear_att[MESA_VK_MAX_COLOR_ATTACHMENTS + 1];
   for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; i++) {
      const VkRenderingAttachmentInfo *att_info =
         if (att_info->imageView == VK_NULL_HANDLE ||
                  VK_FROM_HANDLE(vk_image_view, iview, att_info->imageView);
   render.color_attachment_formats[i] = iview->format;
   assert(render.samples == 0 || render.samples == iview->image->samples);
            clear_att[clear_count++] = (VkClearAttachment) {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .colorAttachment = i,
                  /* One more for depth/stencil, if needed */
            const VkRenderingAttachmentInfo *d_att_info =
         if (d_att_info != NULL && d_att_info->imageView != VK_NULL_HANDLE &&
      d_att_info->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
   VK_FROM_HANDLE(vk_image_view, iview, d_att_info->imageView);
   render.depth_attachment_format = iview->format;
            clear_att[clear_count].aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
   clear_att[clear_count].clearValue.depthStencil.depth =
               const VkRenderingAttachmentInfo *s_att_info =
         if (s_att_info != NULL && s_att_info->imageView != VK_NULL_HANDLE &&
      s_att_info->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
   VK_FROM_HANDLE(vk_image_view, iview, s_att_info->imageView);
   render.stencil_attachment_format = iview->format;
            clear_att[clear_count].aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
   clear_att[clear_count].clearValue.depthStencil.stencil =
      }
   if (clear_att[clear_count].aspectMask != 0)
            if (clear_count > 0) {
      const VkClearRect clear_rect = {
      .rect = pRenderingInfo->renderArea,
   .baseArrayLayer = 0,
   .layerCount = pRenderingInfo->viewMask ?
      };
   vk_meta_clear_attachments(cmd, meta, &render,
               }
      static void
   clear_image_level_layers(struct vk_command_buffer *cmd,
                           struct vk_meta_device *meta,
   struct vk_image *image,
   VkImageLayout image_layout,
   VkFormat format,
   const VkClearValue *clear_value,
   {
      struct vk_device *device = cmd->base.device;
   const struct vk_device_dispatch_table *disp = &device->dispatch_table;
   VkCommandBuffer _cmd = vk_command_buffer_to_handle(cmd);
            const VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = vk_image_to_handle(image),
   .viewType = vk_image_render_view_type(image, layer_count),
   .format = format,
   .subresourceRange = {
      .aspectMask = aspects,
   .baseMipLevel = level,
   .levelCount = 1,
   .baseArrayLayer = base_array_layer,
                  VkImageView image_view;
   result = vk_meta_create_image_view(cmd, meta, &view_info, &image_view);
   if (unlikely(result != VK_SUCCESS)) {
      /* TODO: Report error */
                        VkRenderingAttachmentInfo vk_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = image_view,
   .imageLayout = image_layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      };
   VkRenderingInfo vk_render = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = {
      .offset = { 0, 0 },
      },
      };
   struct vk_meta_rendering_info meta_render = {
                  if (image->aspects == VK_IMAGE_ASPECT_COLOR_BIT) {
      vk_render.colorAttachmentCount = 1;
   vk_render.pColorAttachments = &vk_att;
   meta_render.color_attachment_count = 1;
               if (image->aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      vk_render.pDepthAttachment = &vk_att;
               if (image->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      vk_render.pStencilAttachment = &vk_att;
               const VkClearAttachment clear_att = {
      .aspectMask = aspects,
   .colorAttachment = 0,
               const VkClearRect clear_rect = {
      .rect = {
      .offset = { 0, 0 },
      },
   .baseArrayLayer = 0,
                        vk_meta_clear_attachments(cmd, meta, &meta_render,
               }
      static void
   clear_image_level(struct vk_command_buffer *cmd,
                     struct vk_meta_device *meta,
   struct vk_image *image,
   VkImageLayout image_layout,
   VkFormat format,
   {
               uint32_t base_array_layer, layer_count;
   if (image->image_type == VK_IMAGE_TYPE_3D) {
      base_array_layer = 0;
      } else {
      base_array_layer = range->baseArrayLayer;
               if (layer_count > 1 && !meta->use_layered_rendering) {
      for (uint32_t a = 0; a < layer_count; a++) {
      clear_image_level_layers(cmd, meta, image, image_layout,
                     } else {
      clear_image_level_layers(cmd, meta, image, image_layout,
                     }
      void
   vk_meta_clear_color_image(struct vk_command_buffer *cmd,
                           struct vk_meta_device *meta,
   struct vk_image *image,
   VkImageLayout image_layout,
   {
      const VkClearValue clear_value = {
         };
   for (uint32_t r = 0; r < range_count; r++) {
      const uint32_t level_count =
            for (uint32_t l = 0; l < level_count; l++) {
      clear_image_level(cmd, meta, image, image_layout,
                        }
      void
   vk_meta_clear_depth_stencil_image(struct vk_command_buffer *cmd,
                                       {
      const VkClearValue clear_value = {
         };
   for (uint32_t r = 0; r < range_count; r++) {
      const uint32_t level_count =
            for (uint32_t l = 0; l < level_count; l++) {
      clear_image_level(cmd, meta, image, image_layout,
                        }
