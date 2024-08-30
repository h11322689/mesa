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
      #include <assert.h>
   #include <stdbool.h>
      #include "radv_meta.h"
   #include "radv_private.h"
   #include "sid.h"
      enum radv_color_op {
      FAST_CLEAR_ELIMINATE,
   FMASK_DECOMPRESS,
      };
      static nir_shader *
   build_dcc_decompress_compute_shader(struct radv_device *dev)
   {
                        /* We need at least 16/16/1 to cover an entire DCC block in a single workgroup. */
   b.shader->info.workgroup_size[0] = 16;
   b.shader->info.workgroup_size[1] = 16;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_image, img_type, "in_img");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
            nir_def *global_id = get_global_ids(&b, 2);
   nir_def *img_coord = nir_vec4(&b, nir_channel(&b, global_id, 0), nir_channel(&b, global_id, 1), nir_undef(&b, 1, 32),
            nir_def *data = nir_image_deref_load(&b, 4, 32, &nir_build_deref_var(&b, input_img)->def, img_coord,
            /* We need a SCOPE_DEVICE memory_scope because ACO will avoid
   * creating a vmcnt(0) because it expects the L1 cache to keep memory
   * operations in-order for the same workgroup. The vmcnt(0) seems
   * necessary however. */
   nir_barrier(&b, .execution_scope = SCOPE_WORKGROUP, .memory_scope = SCOPE_DEVICE,
            nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, img_coord, nir_undef(&b, 1, 32), data,
            }
      static VkResult
   create_dcc_compress_compute(struct radv_device *device)
   {
      VkResult result = VK_SUCCESS;
            VkDescriptorSetLayoutCreateInfo ds_create_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 2,
   .pBindings = (VkDescriptorSetLayoutBinding[]){
      {.binding = 0,
               result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->meta_state.fast_clear_flush.dcc_decompress_compute_ds_layout,
   .pushConstantRangeCount = 0,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
                     VkPipelineShaderStageCreateInfo pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info,
         if (result != VK_SUCCESS)
         cleanup:
      ralloc_free(cs);
      }
      static VkResult
   create_pipeline_layout(struct radv_device *device, VkPipelineLayout *layout)
   {
      VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pSetLayouts = NULL,
   .pushConstantRangeCount = 0,
                  }
      static VkResult
   create_pipeline(struct radv_device *device, VkShaderModule vs_module_h, VkPipelineLayout layout)
   {
      VkResult result;
                     if (!fs_module) {
      /* XXX: Need more accurate error */
   result = VK_ERROR_OUT_OF_HOST_MEMORY;
               const VkPipelineShaderStageCreateInfo stages[2] = {
      {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_VERTEX_BIT,
   .module = vs_module_h,
      },
   {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
   .module = vk_shader_module_handle_from_nir(fs_module),
                  const VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
               const VkPipelineInputAssemblyStateCreateInfo ia_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
   .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
               const VkPipelineColorBlendStateCreateInfo blend_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .logicOpEnable = false,
   .attachmentCount = 1,
   .pAttachments = (VkPipelineColorBlendAttachmentState[]){
      {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            const VkPipelineRasterizationStateCreateInfo rs_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
   .depthClampEnable = false,
   .rasterizerDiscardEnable = false,
   .polygonMode = VK_POLYGON_MODE_FILL,
   .cullMode = VK_CULL_MODE_NONE,
               const VkFormat color_format = VK_FORMAT_R8_UNORM;
   const VkPipelineRenderingCreateInfo rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
   .colorAttachmentCount = 1,
               result = radv_graphics_pipeline_create(device_h, device->meta_state.cache,
                                                                                             .pViewportState =
      &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
         .pRasterizationState = &rs_state,
   .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = 1,
   .sampleShadingEnable = false,
   .pSampleMask = NULL,
   .alphaToCoverageEnable = false,
         .pColorBlendState = &blend_state,
   .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .layout = layout,
   .renderPass = VK_NULL_HANDLE,
         if (result != VK_SUCCESS)
            result = radv_graphics_pipeline_create(device_h, device->meta_state.cache,
                                                                                             .pViewportState =
      &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
         .pRasterizationState = &rs_state,
   .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = 1,
   .sampleShadingEnable = false,
   .pSampleMask = NULL,
   .alphaToCoverageEnable = false,
         .pColorBlendState = &blend_state,
   .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .layout = layout,
   .renderPass = VK_NULL_HANDLE,
         if (result != VK_SUCCESS)
            result = radv_graphics_pipeline_create(
      device_h, device->meta_state.cache,
   &(VkGraphicsPipelineCreateInfo){
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
                                 .pViewportState =
      &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
         .pRasterizationState = &rs_state,
   .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = 1,
   .sampleShadingEnable = false,
   .pSampleMask = NULL,
   .alphaToCoverageEnable = false,
         .pColorBlendState = &blend_state,
   .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .layout = layout,
   .renderPass = VK_NULL_HANDLE,
      },
   &(struct radv_graphics_pipeline_create_info){
      .use_rectlist = true,
   .custom_blend_mode = device->physical_device->rad_info.gfx_level >= GFX11 ? V_028808_CB_DCC_DECOMPRESS_GFX11
      },
      if (result != VK_SUCCESS)
         cleanup:
      ralloc_free(fs_module);
      }
      void
   radv_device_finish_meta_fast_clear_flush_state(struct radv_device *device)
   {
               radv_DestroyPipeline(radv_device_to_handle(device), state->fast_clear_flush.dcc_decompress_pipeline, &state->alloc);
   radv_DestroyPipeline(radv_device_to_handle(device), state->fast_clear_flush.fmask_decompress_pipeline,
         radv_DestroyPipeline(radv_device_to_handle(device), state->fast_clear_flush.cmask_eliminate_pipeline, &state->alloc);
            radv_DestroyPipeline(radv_device_to_handle(device), state->fast_clear_flush.dcc_decompress_compute_pipeline,
         radv_DestroyPipelineLayout(radv_device_to_handle(device), state->fast_clear_flush.dcc_decompress_compute_p_layout,
         device->vk.dispatch_table.DestroyDescriptorSetLayout(
      }
      static VkResult
   radv_device_init_meta_fast_clear_flush_state_internal(struct radv_device *device)
   {
               mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.fast_clear_flush.cmask_eliminate_pipeline) {
      mtx_unlock(&device->meta_state.mtx);
               nir_shader *vs_module = radv_meta_build_nir_vs_generate_vertices(device);
   if (!vs_module) {
      /* XXX: Need more accurate error */
   res = VK_ERROR_OUT_OF_HOST_MEMORY;
               res = create_pipeline_layout(device, &device->meta_state.fast_clear_flush.p_layout);
   if (res != VK_SUCCESS)
            VkShaderModule vs_module_h = vk_shader_module_handle_from_nir(vs_module);
   res = create_pipeline(device, vs_module_h, device->meta_state.fast_clear_flush.p_layout);
   if (res != VK_SUCCESS)
            res = create_dcc_compress_compute(device);
   if (res != VK_SUCCESS)
         cleanup:
      ralloc_free(vs_module);
               }
      VkResult
   radv_device_init_meta_fast_clear_flush_state(struct radv_device *device, bool on_demand)
   {
      if (on_demand)
               }
      static void
   radv_emit_set_predication_state_from_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
               if (value) {
      va = radv_buffer_get_va(image->bindings[0].bo) + image->bindings[0].offset;
                  }
      static void
   radv_process_color_image_layer(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_image_view iview;
            width = radv_minify(image->vk.extent.width, range->baseMipLevel + level);
            radv_image_view_init(&iview, device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(image),
   .viewType = radv_meta_get_view_type(image),
   .format = image->vk.format,
   .subresourceRange =
      {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = range->baseMipLevel + level,
            const VkRenderingAttachmentInfo color_att = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&iview),
   .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
               const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = {.offset = {0, 0}, .extent = {width, height}},
   .layerCount = 1,
   .colorAttachmentCount = 1,
                        if (flush_cb)
                     if (flush_cb)
                        }
      static void
   radv_process_color_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_meta_saved_state saved_state;
   bool old_predicating = false;
   bool flush_cb = false;
   uint64_t pred_offset;
            switch (op) {
   case FAST_CLEAR_ELIMINATE:
      pipeline = &device->meta_state.fast_clear_flush.cmask_eliminate_pipeline;
   pred_offset = image->fce_pred_offset;
      case FMASK_DECOMPRESS:
      pipeline = &device->meta_state.fast_clear_flush.fmask_decompress_pipeline;
            /* Flushing CB is required before and after FMASK_DECOMPRESS. */
   flush_cb = true;
      case DCC_DECOMPRESS:
      pipeline = &device->meta_state.fast_clear_flush.dcc_decompress_pipeline;
            /* Flushing CB is required before and after DCC_DECOMPRESS. */
   flush_cb = true;
      default:
                  if (radv_dcc_enabled(image, subresourceRange->baseMipLevel) &&
      (image->vk.array_layers != vk_image_subresource_layer_count(&image->vk, subresourceRange) ||
   subresourceRange->baseArrayLayer != 0)) {
   /* Only use predication if the image has DCC with mipmaps or
   * if the range of layers covers the whole image because the
   * predication is based on mip level.
   */
               if (!*pipeline) {
               ret = radv_device_init_meta_fast_clear_flush_state_internal(device);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                           if (pred_offset) {
                        radv_emit_set_predication_state_from_image(cmd_buffer, image, pred_offset, true);
                        for (uint32_t l = 0; l < vk_image_subresource_level_count(&image->vk, subresourceRange); ++l) {
               /* Do not decompress levels without DCC. */
   if (op == DCC_DECOMPRESS && !radv_dcc_enabled(image, subresourceRange->baseMipLevel + l))
            width = radv_minify(image->vk.extent.width, subresourceRange->baseMipLevel + l);
            radv_CmdSetViewport(
                  radv_CmdSetScissor(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                              for (uint32_t s = 0; s < vk_image_subresource_layer_count(&image->vk, subresourceRange); s++) {
                              if (pred_offset) {
                                 if (cmd_buffer->state.predication_type != -1) {
      /* Restore previous conditional rendering user state. */
   si_emit_set_predication_state(cmd_buffer, cmd_buffer->state.predication_type, cmd_buffer->state.predication_op,
                           /* Clear the image's fast-clear eliminate predicate because FMASK_DECOMPRESS and DCC_DECOMPRESS
   * also perform a fast-clear eliminate.
   */
            /* Mark the image as being decompressed. */
   if (op == DCC_DECOMPRESS)
      }
      static void
   radv_fast_clear_eliminate(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
               barrier.layout_transitions.fast_clear_eliminate = 1;
               }
      static void
   radv_fmask_decompress(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
               barrier.layout_transitions.fmask_decompress = 1;
               }
      void
   radv_fast_clear_flush_image_inplace(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
      if (radv_image_has_fmask(image) && !image->tc_compatible_cmask) {
      if (radv_image_has_dcc(image) && radv_image_has_cmask(image)) {
      /* MSAA images with DCC and CMASK might have been fast-cleared and might require a FCE but
   * FMASK_DECOMPRESS can't eliminate DCC fast clears.
   */
                  } else {
      /* Skip fast clear eliminate for images that support comp-to-single fast clears. */
   if (image->support_comp_to_single)
                  }
      static void
   radv_decompress_dcc_compute(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
      struct radv_meta_saved_state saved_state;
   struct radv_image_view load_iview = {0};
   struct radv_image_view store_iview = {0};
                     if (!cmd_buffer->device->meta_state.fast_clear_flush.cmask_eliminate_pipeline) {
      VkResult ret = radv_device_init_meta_fast_clear_flush_state_internal(cmd_buffer->device);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                           radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
            for (uint32_t l = 0; l < vk_image_subresource_level_count(&image->vk, subresourceRange); l++) {
               /* Do not decompress levels without DCC. */
   if (!radv_dcc_enabled(image, subresourceRange->baseMipLevel + l))
            width = radv_minify(image->vk.extent.width, subresourceRange->baseMipLevel + l);
            for (uint32_t s = 0; s < vk_image_subresource_layer_count(&image->vk, subresourceRange); s++) {
      radv_image_view_init(&load_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(image),
   .viewType = VK_IMAGE_VIEW_TYPE_2D,
   .format = image->vk.format,
   .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
               radv_image_view_init(&store_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(image),
   .viewType = VK_IMAGE_VIEW_TYPE_2D,
   .format = image->vk.format,
   .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
   device->meta_state.fast_clear_flush.dcc_decompress_compute_p_layout, 0, /* set */
   2,                                                                      /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .pImageInfo =
      (VkDescriptorImageInfo[]){
      {
      .sampler = VK_NULL_HANDLE,
   .imageView = radv_image_view_to_handle(&load_iview),
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .pImageInfo = (VkDescriptorImageInfo[]){
                        radv_image_view_finish(&load_iview);
                  /* Mark this image as actually being decompressed. */
                     cmd_buffer->state.flush_bits |= RADV_CMD_FLAG_CS_PARTIAL_FLUSH | RADV_CMD_FLAG_INV_VCACHE |
            /* Initialize the DCC metadata as "fully expanded". */
      }
      void
   radv_decompress_dcc(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image,
         {
               barrier.layout_transitions.dcc_decompress = 1;
            if (cmd_buffer->qf == RADV_QUEUE_GENERAL)
         else
      }
