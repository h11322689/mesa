   /*
   * Copyright © 2016 Red Hat
   *
   * based on anv driver:
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
      #include "nir/nir_builder.h"
   #include "radv_meta.h"
   #include "vk_format.h"
      enum blit2d_src_type {
      BLIT2D_SRC_TYPE_IMAGE,
   BLIT2D_SRC_TYPE_IMAGE_3D,
   BLIT2D_SRC_TYPE_BUFFER,
      };
      static VkResult blit2d_init_color_pipeline(struct radv_device *device, enum blit2d_src_type src_type, VkFormat format,
            static VkResult blit2d_init_depth_only_pipeline(struct radv_device *device, enum blit2d_src_type src_type,
            static VkResult blit2d_init_stencil_only_pipeline(struct radv_device *device, enum blit2d_src_type src_type,
            static void
   create_iview(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *surf, struct radv_image_view *iview,
         {
               if (depth_format)
         else
            radv_image_view_init(iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(surf->image),
   .viewType = radv_meta_get_view_type(surf->image),
   .format = format,
   .subresourceRange = {.aspectMask = aspects,
            }
      static void
   create_bview(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_buffer *src, struct radv_buffer_view *bview,
         {
               if (depth_format)
         else
         radv_buffer_view_init(bview, cmd_buffer->device,
                        &(VkBufferViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .flags = 0,
      }
      struct blit2d_src_temps {
      struct radv_image_view iview;
      };
      static void
   blit2d_bind_src(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src_img,
               {
               if (src_type == BLIT2D_SRC_TYPE_BUFFER) {
               radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, device->meta_state.blit2d[log2_samples].p_layouts[src_type],
   0, /* set */
   1, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                 radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
            } else {
               if (src_type == BLIT2D_SRC_TYPE_IMAGE_3D)
      radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
               radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                 device->meta_state.blit2d[log2_samples].p_layouts[src_type], 0, /* set */
   1, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
         }
      struct blit2d_dst_temps {
      VkImage image;
   struct radv_image_view iview;
      };
      static void
   bind_pipeline(struct radv_cmd_buffer *cmd_buffer, enum blit2d_src_type src_type, unsigned fs_key, uint32_t log2_samples)
   {
                  }
      static void
   bind_depth_pipeline(struct radv_cmd_buffer *cmd_buffer, enum blit2d_src_type src_type, uint32_t log2_samples)
   {
                  }
      static void
   bind_stencil_pipeline(struct radv_cmd_buffer *cmd_buffer, enum blit2d_src_type src_type, uint32_t log2_samples)
   {
                  }
      static void
   radv_meta_blit2d_normal_dst(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src_img,
                     {
               for (unsigned r = 0; r < num_rects; ++r) {
      radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                     &(VkViewport){.x = rects[r].dst_x,
                  radv_CmdSetScissor(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                              u_foreach_bit (i, dst->aspect_mask) {
      unsigned aspect_mask = 1u << i;
   unsigned src_aspect_mask = aspect_mask;
   VkFormat depth_format = 0;
   if (aspect_mask == VK_IMAGE_ASPECT_STENCIL_BIT)
         else if (aspect_mask == VK_IMAGE_ASPECT_DEPTH_BIT)
                        struct blit2d_src_temps src_temps;
                                 float vertex_push_constants[4] = {
      rects[r].src_x,
   rects[r].src_y,
   rects[r].src_x + rects[r].width,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer),
                  if (aspect_mask == VK_IMAGE_ASPECT_COLOR_BIT || aspect_mask == VK_IMAGE_ASPECT_PLANE_0_BIT ||
                     if (device->meta_state.blit2d[log2_samples].pipelines[src_type][fs_key] == VK_NULL_HANDLE) {
      VkResult ret =
         if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  const VkRenderingAttachmentInfo color_att_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&dst_temps.iview),
   .imageLayout = dst->current_layout,
                     const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea =
      {
      .offset = {rects[r].dst_x, rects[r].dst_y},
         .layerCount = 1,
                                 } else if (aspect_mask == VK_IMAGE_ASPECT_DEPTH_BIT) {
      if (device->meta_state.blit2d[log2_samples].depth_only_pipeline[src_type] == VK_NULL_HANDLE) {
      VkResult ret = blit2d_init_depth_only_pipeline(device, src_type, log2_samples);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  const VkRenderingAttachmentInfo depth_att_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&dst_temps.iview),
   .imageLayout = dst->current_layout,
                     const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea =
      {
      .offset = {rects[r].dst_x, rects[r].dst_y},
         .layerCount = 1,
                                    } else if (aspect_mask == VK_IMAGE_ASPECT_STENCIL_BIT) {
      if (device->meta_state.blit2d[log2_samples].stencil_only_pipeline[src_type] == VK_NULL_HANDLE) {
      VkResult ret = blit2d_init_stencil_only_pipeline(device, src_type, log2_samples);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  const VkRenderingAttachmentInfo stencil_att_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&dst_temps.iview),
   .imageLayout = dst->current_layout,
                     const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea =
      {
      .offset = {rects[r].dst_x, rects[r].dst_y},
         .layerCount = 1,
                                                                           if (src_type == BLIT2D_SRC_TYPE_BUFFER)
                                 }
      void
   radv_meta_blit2d(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src_img,
               {
      bool use_3d = (src_img && src_img->image->vk.image_type == VK_IMAGE_TYPE_3D);
   enum blit2d_src_type src_type = src_buf  ? BLIT2D_SRC_TYPE_BUFFER
               radv_meta_blit2d_normal_dst(cmd_buffer, src_img, src_buf, dst, num_rects, rects, src_type,
      }
      static nir_shader *
   build_nir_vertex_shader(struct radv_device *device)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const struct glsl_type *vec2 = glsl_vector_type(GLSL_TYPE_FLOAT, 2);
            nir_variable *pos_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "gl_Position");
            nir_variable *tex_pos_out = nir_variable_create(b.shader, nir_var_shader_out, vec2, "v_tex_pos");
   tex_pos_out->data.location = VARYING_SLOT_VAR0;
            nir_def *outvec = nir_gen_rect_vertices(&b, NULL, NULL);
            nir_def *src_box = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
            /* vertex 0 - src_x, src_y */
   /* vertex 1 - src_x, src_y+h */
   /* vertex 2 - src_x+w, src_y */
   /* so channel 0 is vertex_id != 2 ? src_x : src_x + w
            nir_def *c0cmp = nir_ine_imm(&b, vertex_id, 2);
            nir_def *comp[2];
            comp[1] = nir_bcsel(&b, c1cmp, nir_channel(&b, src_box, 1), nir_channel(&b, src_box, 3));
   nir_def *out_tex_vec = nir_vec(&b, comp, 2);
   nir_store_var(&b, tex_pos_out, out_tex_vec, 0x3);
      }
      typedef nir_def *(*texel_fetch_build_func)(struct nir_builder *, struct radv_device *, nir_def *, bool, bool);
      static nir_def *
   build_nir_texel_fetch(struct nir_builder *b, struct radv_device *device, nir_def *tex_pos, bool is_3d,
         {
      enum glsl_sampler_dim dim = is_3d             ? GLSL_SAMPLER_DIM_3D
               const struct glsl_type *sampler_type = glsl_sampler_type(dim, false, false, GLSL_TYPE_UINT);
   nir_variable *sampler = nir_variable_create(b->shader, nir_var_uniform, sampler_type, "s_tex");
   sampler->data.descriptor_set = 0;
            nir_def *tex_pos_3d = NULL;
   nir_def *sample_idx = NULL;
   if (is_3d) {
               nir_def *chans[3];
   chans[0] = nir_channel(b, tex_pos, 0);
   chans[1] = nir_channel(b, tex_pos, 1);
   chans[2] = layer;
      }
   if (is_multisampled) {
                           if (is_multisampled) {
         } else {
            }
      static nir_def *
   build_nir_buffer_fetch(struct nir_builder *b, struct radv_device *device, nir_def *tex_pos, bool is_3d,
         {
      const struct glsl_type *sampler_type = glsl_sampler_type(GLSL_SAMPLER_DIM_BUF, false, false, GLSL_TYPE_UINT);
   nir_variable *sampler = nir_variable_create(b->shader, nir_var_uniform, sampler_type, "s_tex");
   sampler->data.descriptor_set = 0;
                     nir_def *pos_x = nir_channel(b, tex_pos, 0);
   nir_def *pos_y = nir_channel(b, tex_pos, 1);
   pos_y = nir_imul(b, pos_y, width);
            nir_deref_instr *tex_deref = nir_build_deref_var(b, sampler);
      }
      static const VkPipelineVertexInputStateCreateInfo normal_vi_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
      };
      static nir_shader *
   build_nir_copy_fragment_shader(struct radv_device *device, texel_fetch_build_func txf_func, const char *name,
         {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const struct glsl_type *vec2 = glsl_vector_type(GLSL_TYPE_FLOAT, 2);
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec2, "v_tex_pos");
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
            nir_def *pos_int = nir_f2i32(&b, nir_load_var(&b, tex_pos_in));
            nir_def *color = txf_func(&b, device, tex_pos, is_3d, is_multisampled);
                        }
      static nir_shader *
   build_nir_copy_fragment_shader_depth(struct radv_device *device, texel_fetch_build_func txf_func, const char *name,
         {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const struct glsl_type *vec2 = glsl_vector_type(GLSL_TYPE_FLOAT, 2);
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec2, "v_tex_pos");
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
            nir_def *pos_int = nir_f2i32(&b, nir_load_var(&b, tex_pos_in));
            nir_def *color = txf_func(&b, device, tex_pos, is_3d, is_multisampled);
                        }
      static nir_shader *
   build_nir_copy_fragment_shader_stencil(struct radv_device *device, texel_fetch_build_func txf_func, const char *name,
         {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const struct glsl_type *vec2 = glsl_vector_type(GLSL_TYPE_FLOAT, 2);
            nir_variable *tex_pos_in = nir_variable_create(b.shader, nir_var_shader_in, vec2, "v_tex_pos");
            nir_variable *color_out = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
            nir_def *pos_int = nir_f2i32(&b, nir_load_var(&b, tex_pos_in));
            nir_def *color = txf_func(&b, device, tex_pos, is_3d, is_multisampled);
                        }
      void
   radv_device_finish_meta_blit2d_state(struct radv_device *device)
   {
               for (unsigned log2_samples = 0; log2_samples < MAX_SAMPLES_LOG2; ++log2_samples) {
      for (unsigned src = 0; src < BLIT2D_NUM_SRC_TYPES; src++) {
      radv_DestroyPipelineLayout(radv_device_to_handle(device), state->blit2d[log2_samples].p_layouts[src],
                        for (unsigned j = 0; j < NUM_META_FS_KEYS; ++j) {
      radv_DestroyPipeline(radv_device_to_handle(device), state->blit2d[log2_samples].pipelines[src][j],
               radv_DestroyPipeline(radv_device_to_handle(device), state->blit2d[log2_samples].depth_only_pipeline[src],
         radv_DestroyPipeline(radv_device_to_handle(device), state->blit2d[log2_samples].stencil_only_pipeline[src],
            }
      static VkResult
   blit2d_init_color_pipeline(struct radv_device *device, enum blit2d_src_type src_type, VkFormat format,
         {
      VkResult result;
   unsigned fs_key = radv_format_meta_fs_key(device, format);
            mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.blit2d[log2_samples].pipelines[src_type][fs_key]) {
      mtx_unlock(&device->meta_state.mtx);
               texel_fetch_build_func src_func;
   switch (src_type) {
   case BLIT2D_SRC_TYPE_IMAGE:
      src_func = build_nir_texel_fetch;
   name = "meta_blit2d_image_fs";
      case BLIT2D_SRC_TYPE_IMAGE_3D:
      src_func = build_nir_texel_fetch;
   name = "meta_blit3d_image_fs";
      case BLIT2D_SRC_TYPE_BUFFER:
      src_func = build_nir_buffer_fetch;
   name = "meta_blit2d_buffer_fs";
      default:
      unreachable("unknown blit src type\n");
               const VkPipelineVertexInputStateCreateInfo *vi_create_info;
   nir_shader *fs =
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
               const VkPipelineRenderingCreateInfo rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
   .colorAttachmentCount = 1,
               const VkGraphicsPipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
   .stageCount = ARRAY_SIZE(pipeline_shader_stages),
   .pStages = pipeline_shader_stages,
   .pVertexInputState = vi_create_info,
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
   .rasterizationSamples = 1 << log2_samples,
   .sampleShadingEnable = log2_samples > 1,
   .minSampleShading = 1.0,
         .pColorBlendState =
      &(VkPipelineColorBlendStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .attachmentCount = 1,
   .pAttachments =
      (VkPipelineColorBlendAttachmentState[]){
      {.colorWriteMask = VK_COLOR_COMPONENT_A_BIT | VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
         .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .flags = 0,
   .layout = device->meta_state.blit2d[log2_samples].p_layouts[src_type],
   .renderPass = VK_NULL_HANDLE,
                        result = radv_graphics_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info,
                  ralloc_free(vs);
            mtx_unlock(&device->meta_state.mtx);
      }
      static VkResult
   blit2d_init_depth_only_pipeline(struct radv_device *device, enum blit2d_src_type src_type, uint32_t log2_samples)
   {
      VkResult result;
            mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.blit2d[log2_samples].depth_only_pipeline[src_type]) {
      mtx_unlock(&device->meta_state.mtx);
               texel_fetch_build_func src_func;
   switch (src_type) {
   case BLIT2D_SRC_TYPE_IMAGE:
      src_func = build_nir_texel_fetch;
   name = "meta_blit2d_depth_image_fs";
      case BLIT2D_SRC_TYPE_IMAGE_3D:
      src_func = build_nir_texel_fetch;
   name = "meta_blit3d_depth_image_fs";
      case BLIT2D_SRC_TYPE_BUFFER:
      src_func = build_nir_buffer_fetch;
   name = "meta_blit2d_depth_buffer_fs";
      default:
      unreachable("unknown blit src type\n");
               const VkPipelineVertexInputStateCreateInfo *vi_create_info;
   nir_shader *fs = build_nir_copy_fragment_shader_depth(device, src_func, name, src_type == BLIT2D_SRC_TYPE_IMAGE_3D,
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
               const VkPipelineRenderingCreateInfo rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
               const VkGraphicsPipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
   .stageCount = ARRAY_SIZE(pipeline_shader_stages),
   .pStages = pipeline_shader_stages,
   .pVertexInputState = vi_create_info,
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
   .rasterizationSamples = 1 << log2_samples,
   .sampleShadingEnable = false,
         .pColorBlendState =
      &(VkPipelineColorBlendStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .attachmentCount = 0,
   .pAttachments = NULL,
         .pDepthStencilState =
      &(VkPipelineDepthStencilStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = true,
   .depthWriteEnable = true,
   .depthCompareOp = VK_COMPARE_OP_ALWAYS,
   .front =
      {
      .failOp = VK_STENCIL_OP_KEEP,
   .passOp = VK_STENCIL_OP_KEEP,
   .depthFailOp = VK_STENCIL_OP_KEEP,
   .compareOp = VK_COMPARE_OP_NEVER,
   .compareMask = UINT32_MAX,
   .writeMask = UINT32_MAX,
         .back =
      {
      .failOp = VK_STENCIL_OP_KEEP,
   .passOp = VK_STENCIL_OP_KEEP,
   .depthFailOp = VK_STENCIL_OP_KEEP,
   .compareOp = VK_COMPARE_OP_NEVER,
   .compareMask = UINT32_MAX,
   .writeMask = UINT32_MAX,
         .minDepthBounds = 0.0f,
         .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .flags = 0,
   .layout = device->meta_state.blit2d[log2_samples].p_layouts[src_type],
   .renderPass = VK_NULL_HANDLE,
                        result = radv_graphics_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info,
                  ralloc_free(vs);
            mtx_unlock(&device->meta_state.mtx);
      }
      static VkResult
   blit2d_init_stencil_only_pipeline(struct radv_device *device, enum blit2d_src_type src_type, uint32_t log2_samples)
   {
      VkResult result;
            mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.blit2d[log2_samples].stencil_only_pipeline[src_type]) {
      mtx_unlock(&device->meta_state.mtx);
               texel_fetch_build_func src_func;
   switch (src_type) {
   case BLIT2D_SRC_TYPE_IMAGE:
      src_func = build_nir_texel_fetch;
   name = "meta_blit2d_stencil_image_fs";
      case BLIT2D_SRC_TYPE_IMAGE_3D:
      src_func = build_nir_texel_fetch;
   name = "meta_blit3d_stencil_image_fs";
      case BLIT2D_SRC_TYPE_BUFFER:
      src_func = build_nir_buffer_fetch;
   name = "meta_blit2d_stencil_buffer_fs";
      default:
      unreachable("unknown blit src type\n");
               const VkPipelineVertexInputStateCreateInfo *vi_create_info;
   nir_shader *fs = build_nir_copy_fragment_shader_stencil(device, src_func, name, src_type == BLIT2D_SRC_TYPE_IMAGE_3D,
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
               const VkPipelineRenderingCreateInfo rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
               const VkGraphicsPipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
   .stageCount = ARRAY_SIZE(pipeline_shader_stages),
   .pStages = pipeline_shader_stages,
   .pVertexInputState = vi_create_info,
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
   .rasterizationSamples = 1 << log2_samples,
   .sampleShadingEnable = false,
         .pColorBlendState =
      &(VkPipelineColorBlendStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .attachmentCount = 0,
   .pAttachments = NULL,
         .pDepthStencilState =
      &(VkPipelineDepthStencilStateCreateInfo){
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
      .depthCompareOp = VK_COMPARE_OP_ALWAYS,
   .minDepthBounds = 0.0f,
         .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .flags = 0,
   .layout = device->meta_state.blit2d[log2_samples].p_layouts[src_type],
   .renderPass = VK_NULL_HANDLE,
                        result = radv_graphics_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info,
                  ralloc_free(vs);
            mtx_unlock(&device->meta_state.mtx);
      }
      static VkResult
   meta_blit2d_create_pipe_layout(struct radv_device *device, int idx, uint32_t log2_samples)
   {
      VkResult result;
   VkDescriptorType desc_type =
         const VkPushConstantRange push_constant_ranges[] = {
      {VK_SHADER_STAGE_VERTEX_BIT, 0, 16},
      };
            result = radv_CreateDescriptorSetLayout(
      radv_device_to_handle(device),
   &(VkDescriptorSetLayoutCreateInfo){.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                    .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 1,
   .pBindings =
      (VkDescriptorSetLayoutBinding[]){
      if (result != VK_SUCCESS)
            result =
      radv_CreatePipelineLayout(radv_device_to_handle(device),
                           &(VkPipelineLayoutCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   if (result != VK_SUCCESS)
            fail:
         }
      VkResult
   radv_device_init_meta_blit2d_state(struct radv_device *device, bool on_demand)
   {
               for (unsigned log2_samples = 0; log2_samples < MAX_SAMPLES_LOG2; log2_samples++) {
      for (unsigned src = 0; src < BLIT2D_NUM_SRC_TYPES; src++) {
      /* Don't need to handle copies between buffers and multisample images. */
                  /* There are no multisampled 3D images. */
                  result = meta_blit2d_create_pipe_layout(device, src, log2_samples);
                                 for (unsigned j = 0; j < NUM_META_FS_KEYS; ++j) {
      result = blit2d_init_color_pipeline(device, src, radv_fs_key_format_exemplars[j], log2_samples);
   if (result != VK_SUCCESS)
               result = blit2d_init_depth_only_pipeline(device, src, log2_samples);
                  result = blit2d_init_stencil_only_pipeline(device, src, log2_samples);
   if (result != VK_SUCCESS)
                     }
