   /*
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir/nir_builder.h"
   #include "radv_meta.h"
      struct blit_region {
      VkOffset3D src_offset;
   VkExtent3D src_extent;
   VkOffset3D dst_offset;
      };
      static VkResult build_pipeline(struct radv_device *device, VkImageAspectFlagBits aspect, enum glsl_sampler_dim tex_dim,
            static nir_shader *
   build_nir_vertex_shader(struct radv_device *dev)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
            nir_variable *pos_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "gl_Position");
            nir_variable *tex_pos_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "v_tex_pos");
   tex_pos_out->data.location = VARYING_SLOT_VAR0;
                              nir_def *src_box = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
                     /* vertex 0 - src0_x, src0_y, src0_z */
   /* vertex 1 - src0_x, src1_y, src0_z*/
   /* vertex 2 - src1_x, src0_y, src0_z */
   /* so channel 0 is vertex_id != 2 ? src_x : src_x + w
            nir_def *c0cmp = nir_ine_imm(&b, vertex_id, 2);
            nir_def *comp[4];
            comp[1] = nir_bcsel(&b, c1cmp, nir_channel(&b, src_box, 1), nir_channel(&b, src_box, 3));
   comp[2] = src0_z;
   comp[3] = nir_imm_float(&b, 1.0);
   nir_def *out_tex_vec = nir_vec(&b, comp, 4);
   nir_store_var(&b, tex_pos_out, out_tex_vec, 0xf);
      }
      static nir_shader *
   build_nir_copy_fragment_shader(struct radv_device *dev, enum glsl_sampler_dim tex_dim)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec4, "v_tex_pos");
            /* Swizzle the array index which comes in as Z coordinate into the right
   * position.
   */
   unsigned swz[] = {0, (tex_dim == GLSL_SAMPLER_DIM_1D ? 2 : 1), 2};
   nir_def *const tex_pos =
            const struct glsl_type *sampler_type =
         nir_variable *sampler = nir_variable_create(b.shader, nir_var_uniform, sampler_type, "s_tex");
   sampler->data.descriptor_set = 0;
            nir_deref_instr *tex_deref = nir_build_deref_var(&b, sampler);
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
   color_out->data.location = FRAG_RESULT_DATA0;
               }
      static nir_shader *
   build_nir_copy_fragment_shader_depth(struct radv_device *dev, enum glsl_sampler_dim tex_dim)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec4, "v_tex_pos");
            /* Swizzle the array index which comes in as Z coordinate into the right
   * position.
   */
   unsigned swz[] = {0, (tex_dim == GLSL_SAMPLER_DIM_1D ? 2 : 1), 2};
   nir_def *const tex_pos =
            const struct glsl_type *sampler_type =
         nir_variable *sampler = nir_variable_create(b.shader, nir_var_uniform, sampler_type, "s_tex");
   sampler->data.descriptor_set = 0;
            nir_deref_instr *tex_deref = nir_build_deref_var(&b, sampler);
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
   color_out->data.location = FRAG_RESULT_DEPTH;
               }
      static nir_shader *
   build_nir_copy_fragment_shader_stencil(struct radv_device *dev, enum glsl_sampler_dim tex_dim)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec4, "v_tex_pos");
            /* Swizzle the array index which comes in as Z coordinate into the right
   * position.
   */
   unsigned swz[] = {0, (tex_dim == GLSL_SAMPLER_DIM_1D ? 2 : 1), 2};
   nir_def *const tex_pos =
            const struct glsl_type *sampler_type =
         nir_variable *sampler = nir_variable_create(b.shader, nir_var_uniform, sampler_type, "s_tex");
   sampler->data.descriptor_set = 0;
            nir_deref_instr *tex_deref = nir_build_deref_var(&b, sampler);
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
   color_out->data.location = FRAG_RESULT_STENCIL;
               }
      static enum glsl_sampler_dim
   translate_sampler_dim(VkImageType type)
   {
      switch (type) {
   case VK_IMAGE_TYPE_1D:
         case VK_IMAGE_TYPE_2D:
         case VK_IMAGE_TYPE_3D:
         default:
            }
      static void
   meta_emit_blit(struct radv_cmd_buffer *cmd_buffer, struct radv_image *src_image, struct radv_image_view *src_iview,
                     {
      struct radv_device *device = cmd_buffer->device;
   uint32_t src_width = radv_minify(src_iview->image->vk.extent.width, src_iview->vk.base_mip_level);
   uint32_t src_height = radv_minify(src_iview->image->vk.extent.height, src_iview->vk.base_mip_level);
   uint32_t src_depth = radv_minify(src_iview->image->vk.extent.depth, src_iview->vk.base_mip_level);
   uint32_t dst_width = radv_minify(dst_iview->image->vk.extent.width, dst_iview->vk.base_mip_level);
                     float vertex_push_constants[5] = {
      src_offset_0[0] / (float)src_width,  src_offset_0[1] / (float)src_height, src_offset_1[0] / (float)src_width,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.blit.pipeline_layout,
            VkPipeline *pipeline = NULL;
   unsigned fs_key = 0;
            switch (src_iview->vk.aspects) {
   case VK_IMAGE_ASPECT_COLOR_BIT: {
      fs_key = radv_format_meta_fs_key(device, dst_image->vk.format);
            switch (src_image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      pipeline = &device->meta_state.blit.pipeline_1d_src[fs_key];
      case VK_IMAGE_TYPE_2D:
      pipeline = &device->meta_state.blit.pipeline_2d_src[fs_key];
      case VK_IMAGE_TYPE_3D:
      pipeline = &device->meta_state.blit.pipeline_3d_src[fs_key];
      default:
         }
      }
   case VK_IMAGE_ASPECT_DEPTH_BIT: {
               switch (src_image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      pipeline = &device->meta_state.blit.depth_only_1d_pipeline;
      case VK_IMAGE_TYPE_2D:
      pipeline = &device->meta_state.blit.depth_only_2d_pipeline;
      case VK_IMAGE_TYPE_3D:
      pipeline = &device->meta_state.blit.depth_only_3d_pipeline;
      default:
         }
      }
   case VK_IMAGE_ASPECT_STENCIL_BIT: {
               switch (src_image->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      pipeline = &device->meta_state.blit.stencil_only_1d_pipeline;
      case VK_IMAGE_TYPE_2D:
      pipeline = &device->meta_state.blit.stencil_only_2d_pipeline;
      case VK_IMAGE_TYPE_3D:
      pipeline = &device->meta_state.blit.stencil_only_3d_pipeline;
      default:
         }
      }
   default:
                  if (!*pipeline) {
      VkResult ret = build_pipeline(device, src_iview->vk.aspects, translate_sampler_dim(src_image->vk.image_type),
         if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                           radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, device->meta_state.blit.pipeline_layout,
                                 0, /* set */
   1, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
            VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea =
      {
      .offset = {0, 0},
                     VkRenderingAttachmentInfo color_att;
   if (src_iview->image->vk.aspects == VK_IMAGE_ASPECT_COLOR_BIT) {
      unsigned dst_layout = radv_meta_dst_layout_from_layout(dst_image_layout);
            color_att = (VkRenderingAttachmentInfo){
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(dst_iview),
   .imageLayout = layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      };
   rendering_info.colorAttachmentCount = 1;
               VkRenderingAttachmentInfo depth_att;
   if (src_iview->image->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) {
      enum radv_blit_ds_layout ds_layout = radv_meta_blit_ds_to_type(dst_image_layout);
            depth_att = (VkRenderingAttachmentInfo){
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(dst_iview),
   .imageLayout = layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      };
               VkRenderingAttachmentInfo stencil_att;
   if (src_iview->image->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      enum radv_blit_ds_layout ds_layout = radv_meta_blit_ds_to_type(dst_image_layout);
            stencil_att = (VkRenderingAttachmentInfo){
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(dst_iview),
   .imageLayout = layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      };
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
      }
      static void
   blit_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *src_image, VkImageLayout src_image_layout,
         {
      const VkImageSubresourceLayers *src_res = &region->srcSubresource;
   const VkImageSubresourceLayers *dst_res = &region->dstSubresource;
   struct radv_device *device = cmd_buffer->device;
   struct radv_meta_saved_state saved_state;
            /* From the Vulkan 1.0 spec:
   *
   *    vkCmdBlitImage must not be used for multisampled source or
   *    destination images. Use vkCmdResolveImage for this purpose.
   */
   assert(src_image->vk.samples == 1);
            radv_CreateSampler(radv_device_to_handle(device),
                     &(VkSamplerCreateInfo){
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
   .magFilter = filter,
   .minFilter = filter,
   .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
         /* VK_EXT_conditional_rendering says that blit commands should not be
   * affected by conditional rendering.
   */
   radv_meta_save(&saved_state, cmd_buffer,
                  unsigned dst_start, dst_end;
   if (dst_image->vk.image_type == VK_IMAGE_TYPE_3D) {
      assert(dst_res->baseArrayLayer == 0);
   dst_start = region->dstOffsets[0].z;
      } else {
      dst_start = dst_res->baseArrayLayer;
               unsigned src_start, src_end;
   if (src_image->vk.image_type == VK_IMAGE_TYPE_3D) {
      assert(src_res->baseArrayLayer == 0);
   src_start = region->srcOffsets[0].z;
      } else {
      src_start = src_res->baseArrayLayer;
               bool flip_z = flip_coords(&src_start, &src_end, &dst_start, &dst_end);
            /* There is no interpolation to the pixel center during
   * rendering, so add the 0.5 offset ourselves here. */
   float depth_center_offset = 0;
   if (src_image->vk.image_type == VK_IMAGE_TYPE_3D)
            if (flip_z) {
      src_start = src_end;
   src_z_step *= -1;
               unsigned src_x0 = region->srcOffsets[0].x;
   unsigned src_x1 = region->srcOffsets[1].x;
   unsigned dst_x0 = region->dstOffsets[0].x;
            unsigned src_y0 = region->srcOffsets[0].y;
   unsigned src_y1 = region->srcOffsets[1].y;
   unsigned dst_y0 = region->dstOffsets[0].y;
            VkRect2D dst_box;
   dst_box.offset.x = MIN2(dst_x0, dst_x1);
   dst_box.offset.y = MIN2(dst_y0, dst_y1);
   dst_box.extent.width = dst_x1 - dst_x0;
            const VkOffset2D dst_offset_0 = {
      .x = dst_x0,
      };
   const VkOffset2D dst_offset_1 = {
      .x = dst_x1,
               radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                     &(VkViewport){.x = dst_offset_0.x,
                  radv_CmdSetScissor(
      radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
   &(VkRect2D){
      .offset = (VkOffset2D){MIN2(dst_offset_0.x, dst_offset_1.x), MIN2(dst_offset_0.y, dst_offset_1.y)},
            const unsigned num_layers = dst_end - dst_start;
   for (unsigned i = 0; i < num_layers; i++) {
               float src_offset_0[3] = {
      src_x0,
   src_y0,
      };
   float src_offset_1[3] = {
      src_x1,
   src_y1,
      };
            /* 3D images have just 1 layer */
            radv_image_view_init(&dst_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(dst_image),
   .viewType = radv_meta_get_view_type(dst_image),
   .format = dst_image->vk.format,
   .subresourceRange = {.aspectMask = dst_res->aspectMask,
               radv_image_view_init(&src_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(src_image),
   .viewType = radv_meta_get_view_type(src_image),
   .format = src_image->vk.format,
   .subresourceRange = {.aspectMask = src_res->aspectMask,
               meta_emit_blit(cmd_buffer, src_image, &src_iview, src_image_layout, src_offset_0, src_offset_1, dst_image,
            radv_image_view_finish(&dst_iview);
                           }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_image, src_image, pBlitImageInfo->srcImage);
            for (unsigned r = 0; r < pBlitImageInfo->regionCount; r++) {
      blit_image(cmd_buffer, src_image, pBlitImageInfo->srcImageLayout, dst_image, pBlitImageInfo->dstImageLayout,
         }
      void
   radv_device_finish_meta_blit_state(struct radv_device *device)
   {
               for (unsigned i = 0; i < NUM_META_FS_KEYS; ++i) {
      radv_DestroyPipeline(radv_device_to_handle(device), state->blit.pipeline_1d_src[i], &state->alloc);
   radv_DestroyPipeline(radv_device_to_handle(device), state->blit.pipeline_2d_src[i], &state->alloc);
               radv_DestroyPipeline(radv_device_to_handle(device), state->blit.depth_only_1d_pipeline, &state->alloc);
   radv_DestroyPipeline(radv_device_to_handle(device), state->blit.depth_only_2d_pipeline, &state->alloc);
            radv_DestroyPipeline(radv_device_to_handle(device), state->blit.stencil_only_1d_pipeline, &state->alloc);
   radv_DestroyPipeline(radv_device_to_handle(device), state->blit.stencil_only_2d_pipeline, &state->alloc);
            radv_DestroyPipelineLayout(radv_device_to_handle(device), state->blit.pipeline_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->blit.ds_layout,
      }
      static VkResult
   build_pipeline(struct radv_device *device, VkImageAspectFlagBits aspect, enum glsl_sampler_dim tex_dim, VkFormat format,
         {
                        if (*pipeline) {
      mtx_unlock(&device->meta_state.mtx);
               nir_shader *fs;
            VkPipelineRenderingCreateInfo rendering_create_info = {
                  switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT:
      fs = build_nir_copy_fragment_shader(device, tex_dim);
   rendering_create_info.colorAttachmentCount = 1;
   rendering_create_info.pColorAttachmentFormats = &format;
      case VK_IMAGE_ASPECT_DEPTH_BIT:
      fs = build_nir_copy_fragment_shader_depth(device, tex_dim);
   rendering_create_info.depthAttachmentFormat = format;
      case VK_IMAGE_ASPECT_STENCIL_BIT:
      fs = build_nir_copy_fragment_shader_stencil(device, tex_dim);
   rendering_create_info.stencilAttachmentFormat = format;
      default:
         }
   VkPipelineVertexInputStateCreateInfo vi_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
               VkPipelineShaderStageCreateInfo pipeline_shader_stages[] = {
      {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_VERTEX_BIT,
   .module = vk_shader_module_handle_from_nir(vs),
   .pName = "main",
   .pSpecializationInfo = NULL},
   {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
   .module = vk_shader_module_handle_from_nir(fs),
   .pName = "main",
               VkGraphicsPipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
   .stageCount = ARRAY_SIZE(pipeline_shader_stages),
   .pStages = pipeline_shader_stages,
   .pVertexInputState = &vi_create_info,
   .pInputAssemblyState =
      &(VkPipelineInputAssemblyStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
   .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
         .pViewportState =
      &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
         .pRasterizationState =
      &(VkPipelineRasterizationStateCreateInfo){.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                              .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = 1,
   .sampleShadingEnable = false,
         .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .flags = 0,
   .layout = device->meta_state.blit.pipeline_layout,
   .renderPass = VK_NULL_HANDLE,
               VkPipelineColorBlendStateCreateInfo color_blend_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .attachmentCount = 1,
   .pAttachments =
      (VkPipelineColorBlendAttachmentState[]){
      {.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
               VkPipelineDepthStencilStateCreateInfo depth_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = true,
   .depthWriteEnable = true,
               VkPipelineDepthStencilStateCreateInfo stencil_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = false,
   .depthWriteEnable = false,
   .stencilTestEnable = true,
   .front = {.failOp = VK_STENCIL_OP_REPLACE,
            .passOp = VK_STENCIL_OP_REPLACE,
   .depthFailOp = VK_STENCIL_OP_REPLACE,
   .compareOp = VK_COMPARE_OP_ALWAYS,
   .compareMask = 0xff,
      .back = {.failOp = VK_STENCIL_OP_REPLACE,
            .passOp = VK_STENCIL_OP_REPLACE,
   .depthFailOp = VK_STENCIL_OP_REPLACE,
   .compareOp = VK_COMPARE_OP_ALWAYS,
   .compareMask = 0xff,
                  switch (aspect) {
   case VK_IMAGE_ASPECT_COLOR_BIT:
      vk_pipeline_info.pColorBlendState = &color_blend_info;
      case VK_IMAGE_ASPECT_DEPTH_BIT:
      vk_pipeline_info.pDepthStencilState = &depth_info;
      case VK_IMAGE_ASPECT_STENCIL_BIT:
      vk_pipeline_info.pDepthStencilState = &stencil_info;
      default:
                           result = radv_graphics_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info,
         ralloc_free(vs);
   ralloc_free(fs);
   mtx_unlock(&device->meta_state.mtx);
      }
      static VkResult
   radv_device_init_meta_blit_color(struct radv_device *device, bool on_demand)
   {
               for (unsigned i = 0; i < NUM_META_FS_KEYS; ++i) {
      VkFormat format = radv_fs_key_format_exemplars[i];
            if (on_demand)
            result = build_pipeline(device, VK_IMAGE_ASPECT_COLOR_BIT, GLSL_SAMPLER_DIM_1D, format,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_COLOR_BIT, GLSL_SAMPLER_DIM_2D, format,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_COLOR_BIT, GLSL_SAMPLER_DIM_3D, format,
         if (result != VK_SUCCESS)
                  fail:
         }
      static VkResult
   radv_device_init_meta_blit_depth(struct radv_device *device, bool on_demand)
   {
               if (on_demand)
            result = build_pipeline(device, VK_IMAGE_ASPECT_DEPTH_BIT, GLSL_SAMPLER_DIM_1D, VK_FORMAT_D32_SFLOAT,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_DEPTH_BIT, GLSL_SAMPLER_DIM_2D, VK_FORMAT_D32_SFLOAT,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_DEPTH_BIT, GLSL_SAMPLER_DIM_3D, VK_FORMAT_D32_SFLOAT,
         if (result != VK_SUCCESS)
         fail:
         }
      static VkResult
   radv_device_init_meta_blit_stencil(struct radv_device *device, bool on_demand)
   {
               if (on_demand)
            result = build_pipeline(device, VK_IMAGE_ASPECT_STENCIL_BIT, GLSL_SAMPLER_DIM_1D, VK_FORMAT_S8_UINT,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_STENCIL_BIT, GLSL_SAMPLER_DIM_2D, VK_FORMAT_S8_UINT,
         if (result != VK_SUCCESS)
            result = build_pipeline(device, VK_IMAGE_ASPECT_STENCIL_BIT, GLSL_SAMPLER_DIM_3D, VK_FORMAT_S8_UINT,
         if (result != VK_SUCCESS)
         fail:
         }
      VkResult
   radv_device_init_meta_blit_state(struct radv_device *device, bool on_demand)
   {
               VkDescriptorSetLayoutCreateInfo ds_layout_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_layout_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
                     result = radv_CreatePipelineLayout(radv_device_to_handle(device),
                                    &(VkPipelineLayoutCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_blit_color(device, on_demand);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_blit_depth(device, on_demand);
   if (result != VK_SUCCESS)
               }
