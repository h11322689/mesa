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
      #include "nir/nir_builder.h"
   #include "radv_meta.h"
   #include "radv_private.h"
   #include "sid.h"
   #include "vk_format.h"
      /* emit 0, 0, 0, 1 */
   static nir_shader *
   build_nir_fs(struct radv_device *dev)
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
                     f_color = nir_variable_create(b.shader, nir_var_shader_out, vec4, "f_color");
   f_color->data.location = FRAG_RESULT_DATA0;
               }
      static VkResult
   create_pipeline(struct radv_device *device, VkShaderModule vs_module_h, VkFormat format, VkPipeline *pipeline)
   {
      VkResult result;
            nir_shader *fs_module = build_nir_fs(device);
   if (!fs_module) {
      /* XXX: Need more accurate error */
   result = VK_ERROR_OUT_OF_HOST_MEMORY;
               VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pSetLayouts = NULL,
   .pushConstantRangeCount = 0,
               if (!device->meta_state.resolve.p_layout) {
      result = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
               VkFormat color_formats[2] = {format, format};
   const VkPipelineRenderingCreateInfo rendering_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
   .colorAttachmentCount = 2,
               result = radv_graphics_pipeline_create(
      device_h, device->meta_state.cache,
   &(VkGraphicsPipelineCreateInfo){
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
   .pNext = &rendering_create_info,
   .stageCount = 2,
   .pStages =
      (VkPipelineShaderStageCreateInfo[]){
      {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_VERTEX_BIT,
   .module = vs_module_h,
      },
   {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
   .module = vk_shader_module_handle_from_nir(fs_module),
            .pVertexInputState =
      &(VkPipelineVertexInputStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
         .pInputAssemblyState =
      &(VkPipelineInputAssemblyStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
   .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
         .pViewportState =
      &(VkPipelineViewportStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
         .pRasterizationState =
      &(VkPipelineRasterizationStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
   .depthClampEnable = false,
   .rasterizerDiscardEnable = false,
   .polygonMode = VK_POLYGON_MODE_FILL,
   .cullMode = VK_CULL_MODE_NONE,
         .pMultisampleState =
      &(VkPipelineMultisampleStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = 1,
   .sampleShadingEnable = false,
   .pSampleMask = NULL,
   .alphaToCoverageEnable = false,
         .pColorBlendState =
      &(VkPipelineColorBlendStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .logicOpEnable = false,
   .attachmentCount = 2,
   .pAttachments =
      (VkPipelineColorBlendAttachmentState[]){
      {
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                           .pDynamicState =
      &(VkPipelineDynamicStateCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 2,
   .pDynamicStates =
      (VkDynamicState[]){
      VK_DYNAMIC_STATE_VIEWPORT,
         .layout = device->meta_state.resolve.p_layout,
   .renderPass = VK_NULL_HANDLE,
      },
   &(struct radv_graphics_pipeline_create_info){
      .use_rectlist = true,
      },
      if (result != VK_SUCCESS)
                  cleanup:
      ralloc_free(fs_module);
      }
      void
   radv_device_finish_meta_resolve_state(struct radv_device *device)
   {
               for (uint32_t j = 0; j < NUM_META_FS_KEYS; j++) {
         }
      }
      VkResult
   radv_device_init_meta_resolve_state(struct radv_device *device, bool on_demand)
   {
      if (on_demand)
            VkResult res = VK_SUCCESS;
   struct radv_meta_state *state = &device->meta_state;
   nir_shader *vs_module = radv_meta_build_nir_vs_generate_vertices(device);
   if (!vs_module) {
      /* XXX: Need more accurate error */
   res = VK_ERROR_OUT_OF_HOST_MEMORY;
               for (uint32_t i = 0; i < NUM_META_FS_KEYS; ++i) {
      VkFormat format = radv_fs_key_format_exemplars[i];
            VkShaderModule vs_module_h = vk_shader_module_handle_from_nir(vs_module);
   res = create_pipeline(device, vs_module_h, format, &state->resolve.pipeline[fs_key]);
   if (res != VK_SUCCESS)
            cleanup:
                  }
      static void
   emit_resolve(struct radv_cmd_buffer *cmd_buffer, const struct radv_image *src_image, const struct radv_image *dst_image,
         {
      struct radv_device *device = cmd_buffer->device;
   VkCommandBuffer cmd_buffer_h = radv_cmd_buffer_to_handle(cmd_buffer);
            cmd_buffer->state.flush_bits |=
      radv_src_access_flush(cmd_buffer, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, src_image) |
   radv_dst_access_flush(cmd_buffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT, src_image) |
                  radv_CmdDraw(cmd_buffer_h, 3, 1, 0, 0);
      }
      enum radv_resolve_method {
      RESOLVE_HW,
   RESOLVE_COMPUTE,
      };
      static bool
   image_hw_resolve_compat(const struct radv_device *device, struct radv_image *src_image, struct radv_image *dst_image)
   {
      if (device->physical_device->rad_info.gfx_level >= GFX9) {
         } else {
            }
      static void
   radv_pick_resolve_method_images(struct radv_device *device, struct radv_image *src_image, VkFormat src_format,
                  {
               if (vk_format_is_color(src_format)) {
      /* Using the fragment resolve path is currently a hint to
   * avoid decompressing DCC for partial resolves and
   * re-initialize it after resolving using compute.
   * TODO: Add support for layered and int to the fragment path.
   */
   if (radv_layout_dcc_compressed(device, dst_image, dst_level, dst_image_layout, queue_mask)) {
         } else if (!image_hw_resolve_compat(device, src_image, dst_image)) {
      /* The micro tile mode only needs to match for the HW
   * resolve path which is the default path for non-DCC
   * resolves.
   */
               if (src_format == VK_FORMAT_R16G16_UNORM || src_format == VK_FORMAT_R16G16_SNORM)
         else if (vk_format_is_int(src_format))
         else if (src_image->vk.array_layers > 1 || dst_image->vk.array_layers > 1)
      } else {
      assert(dst_image_layout == VK_IMAGE_LAYOUT_UNDEFINED);
   if (src_image->vk.array_layers > 1 || dst_image->vk.array_layers > 1 ||
      (dst_image->planes[0].surface.flags & RADEON_SURF_NO_RENDER_TARGET))
      else
         }
      static VkResult
   build_resolve_pipeline(struct radv_device *device, unsigned fs_key)
   {
               if (device->meta_state.resolve.pipeline[fs_key])
            mtx_lock(&device->meta_state.mtx);
   if (device->meta_state.resolve.pipeline[fs_key]) {
      mtx_unlock(&device->meta_state.mtx);
                        VkShaderModule vs_module_h = vk_shader_module_handle_from_nir(vs_module);
   result = create_pipeline(device, vs_module_h, radv_fs_key_format_exemplars[fs_key],
            ralloc_free(vs_module);
   mtx_unlock(&device->meta_state.mtx);
      }
      static void
   radv_meta_resolve_hardware_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *src_image,
               {
      struct radv_device *device = cmd_buffer->device;
                     assert(src_image->vk.samples > 1);
                     /* From the Vulkan 1.0 spec:
   *
   *    - The aspectMask member of srcSubresource and dstSubresource must
   *      only contain VK_IMAGE_ASPECT_COLOR_BIT
   *
   *    - The layerCount member of srcSubresource and dstSubresource must
   *      match
   */
   assert(region->srcSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
   assert(region->dstSubresource.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT);
   assert(vk_image_subresource_layer_count(&src_image->vk, &region->srcSubresource) ==
                              /**
   * From Vulkan 1.0.6 spec: 18.6 Resolving Multisample Images
   *
   *    extent is the size in texels of the source image to resolve in width,
   *    height and depth. 1D images use only x and width. 2D images use x, y,
   *    width and height. 3D images use x, y, z, width, height and depth.
   *
   *    srcOffset and dstOffset select the initial x, y, and z offsets in
   *    texels of the sub-regions of the source and destination image data.
   *    extent is the size in texels of the source image to resolve in width,
   *    height and depth. 1D images use only x and width. 2D images use x, y,
   *    width and height. 3D images use x, y, z, width, height and depth.
   */
   const struct VkExtent3D extent = vk_image_sanitize_extent(&src_image->vk, region->extent);
                     if (radv_layout_dcc_compressed(cmd_buffer->device, dst_image, region->dstSubresource.mipLevel, dst_image_layout,
            VkImageSubresourceRange range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = region->dstSubresource.mipLevel,
   .levelCount = 1,
   .baseArrayLayer = dst_base_layer,
                           VkRect2D resolve_area = {
      .offset = {dstOffset.x, dstOffset.y},
               radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                     &(VkViewport){.x = resolve_area.offset.x,
                                                VkResult ret = build_resolve_pipeline(device, fs_key);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
               struct radv_image_view src_iview;
   radv_image_view_init(&src_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(src_image),
   .viewType = radv_meta_get_view_type(src_image),
   .format = src_image->vk.format,
   .subresourceRange =
      {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = region->srcSubresource.mipLevel,
            struct radv_image_view dst_iview;
   radv_image_view_init(&dst_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(dst_image),
   .viewType = radv_meta_get_view_type(dst_image),
   .format = dst_image->vk.format,
   .subresourceRange =
      {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = region->dstSubresource.mipLevel,
            const VkRenderingAttachmentInfo color_atts[2] = {
      {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&src_iview),
   .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      },
   {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(&dst_iview),
   .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                  const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = resolve_area,
   .layerCount = 1,
   .colorAttachmentCount = 2,
                                          radv_image_view_finish(&src_iview);
                  }
      static void
   resolve_image(struct radv_cmd_buffer *cmd_buffer, struct radv_image *src_image, VkImageLayout src_image_layout,
               {
      switch (resolve_method) {
   case RESOLVE_HW:
      radv_meta_resolve_hardware_image(cmd_buffer, src_image, src_image_layout, dst_image, dst_image_layout, region);
      case RESOLVE_FRAGMENT:
               radv_meta_resolve_fragment_image(cmd_buffer, src_image, src_image_layout, dst_image, dst_image_layout, region);
      case RESOLVE_COMPUTE:
               radv_meta_resolve_compute_image(cmd_buffer, src_image, src_image->vk.format, src_image_layout, dst_image,
            default:
            }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_image, src_image, pResolveImageInfo->srcImage);
   RADV_FROM_HANDLE(radv_image, dst_image, pResolveImageInfo->dstImage);
   VkImageLayout src_image_layout = pResolveImageInfo->srcImageLayout;
   VkImageLayout dst_image_layout = pResolveImageInfo->dstImageLayout;
   const struct radv_physical_device *pdevice = cmd_buffer->device->physical_device;
            /* we can use the hw resolve only for single full resolves */
   if (pResolveImageInfo->regionCount == 1) {
      if (pResolveImageInfo->pRegions[0].srcOffset.x || pResolveImageInfo->pRegions[0].srcOffset.y ||
      pResolveImageInfo->pRegions[0].srcOffset.z)
      if (pResolveImageInfo->pRegions[0].dstOffset.x || pResolveImageInfo->pRegions[0].dstOffset.y ||
                  if (pResolveImageInfo->pRegions[0].extent.width != src_image->vk.extent.width ||
      pResolveImageInfo->pRegions[0].extent.height != src_image->vk.extent.height ||
   pResolveImageInfo->pRegions[0].extent.depth != src_image->vk.extent.depth)
   } else
            for (uint32_t r = 0; r < pResolveImageInfo->regionCount; r++) {
               radv_pick_resolve_method_images(cmd_buffer->device, src_image, src_image->vk.format, dst_image,
                  }
      static void
   radv_cmd_buffer_resolve_rendering_hw(struct radv_cmd_buffer *cmd_buffer)
   {
                                 radv_CmdSetViewport(radv_cmd_buffer_to_handle(cmd_buffer), 0, 1,
                     &(VkViewport){.x = resolve_area->offset.x,
                           for (uint32_t i = 0; i < saved_state.render.color_att_count; ++i) {
      if (saved_state.render.color_att[i].resolve_iview == NULL)
            struct radv_image_view *src_iview = saved_state.render.color_att[i].iview;
   VkImageLayout src_layout = saved_state.render.color_att[i].layout;
            struct radv_image_view *dst_iview = saved_state.render.color_att[i].resolve_iview;
   VkImageLayout dst_layout = saved_state.render.color_att[i].resolve_layout;
                     if (radv_layout_dcc_compressed(cmd_buffer->device, dst_img, dst_iview->vk.base_mip_level, dst_layout,
            VkImageSubresourceRange range = {
      .aspectMask = dst_iview->vk.aspects,
   .baseMipLevel = dst_iview->vk.base_mip_level,
   .levelCount = dst_iview->vk.level_count,
   .baseArrayLayer = dst_iview->vk.base_array_layer,
                           const VkRenderingAttachmentInfo color_atts[2] = {
      {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(src_iview),
   .imageLayout = src_layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
      },
   {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
   .imageView = radv_image_view_to_handle(dst_iview),
   .imageLayout = dst_layout,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
                  const VkRenderingInfo rendering_info = {
      .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
   .renderArea = saved_state.render.area,
   .layerCount = saved_state.render.layer_count,
   .viewMask = saved_state.render.view_mask,
   .colorAttachmentCount = 2,
                        VkResult ret =
         if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                                       }
      /**
   * Emit any needed resolves for the current subpass.
   */
   void
   radv_cmd_buffer_resolve_rendering(struct radv_cmd_buffer *cmd_buffer)
   {
      const struct radv_physical_device *pdevice = cmd_buffer->device->physical_device;
   const struct radv_rendering_state *render = &cmd_buffer->state.render;
            bool has_color_resolve = false;
   for (uint32_t i = 0; i < render->color_att_count; ++i) {
      if (render->color_att[i].resolve_iview != NULL)
      }
            if (!has_color_resolve && !has_ds_resolve)
                     if (render->ds_att.resolve_iview != NULL) {
      struct radv_image_view *src_iview = render->ds_att.iview;
            radv_pick_resolve_method_images(cmd_buffer->device, src_iview->image, src_iview->vk.format, dst_iview->image,
                  if ((src_iview->vk.aspects & VK_IMAGE_ASPECT_DEPTH_BIT) && render->ds_att.resolve_mode != VK_RESOLVE_MODE_NONE) {
      if (resolve_method == RESOLVE_FRAGMENT) {
         } else {
      assert(resolve_method == RESOLVE_COMPUTE);
                  if ((src_iview->vk.aspects & VK_IMAGE_ASPECT_STENCIL_BIT) &&
      render->ds_att.stencil_resolve_mode != VK_RESOLVE_MODE_NONE) {
   if (resolve_method == RESOLVE_FRAGMENT) {
      radv_depth_stencil_resolve_rendering_fs(cmd_buffer, VK_IMAGE_ASPECT_STENCIL_BIT,
      } else {
      assert(resolve_method == RESOLVE_COMPUTE);
   radv_depth_stencil_resolve_rendering_cs(cmd_buffer, VK_IMAGE_ASPECT_STENCIL_BIT,
                  /* From the Vulkan spec 1.2.165:
   *
   * "VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT specifies
   *  write access to a color, resolve, or depth/stencil
   *  resolve attachment during a render pass or via
   *  certain subpass load and store operations."
   *
   * Yes, it's counterintuitive but it makes sense because ds
   * resolve operations happen late at the end of the subpass.
   *
   * That said, RADV is wrong because it executes the subpass
   * end barrier *before* any subpass resolves instead of after.
   *
   * TODO: Fix this properly by executing subpass end barriers
   * after subpass resolves.
   */
   cmd_buffer->state.flush_bits |= RADV_CMD_FLAG_FLUSH_AND_INV_DB;
   if (radv_image_has_htile(dst_iview->image))
               if (has_color_resolve) {
      uint32_t layer_count = render->layer_count;
   VkRect2D resolve_area = render->area;
            if (render->view_mask)
            /* Resolves happen before the end-of-subpass barriers get executed, so we have to make the
   * attachment shader-readable.
   */
   barrier.src_stage_mask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
   barrier.dst_stage_mask = VK_PIPELINE_STAGE_2_RESOLVE_BIT;
   barrier.src_access_mask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
   barrier.dst_access_mask = VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
            for (uint32_t i = 0; i < render->color_att_count; ++i) {
                     struct radv_image_view *src_iview = render->color_att[i].iview;
   VkImageLayout src_layout = render->color_att[i].layout;
   struct radv_image *src_img = src_iview->image;
   struct radv_image_view *dst_iview = render->color_att[i].resolve_iview;
                  radv_pick_resolve_method_images(cmd_buffer->device, src_img, src_iview->vk.format, dst_img,
         VkImageResolve2 region = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2,
   .extent =
      {
      .width = resolve_area.extent.width,
   .height = resolve_area.extent.height,
         .srcSubresource =
      (VkImageSubresourceLayers){
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = src_iview->vk.base_mip_level,
   .baseArrayLayer = src_iview->vk.base_array_layer,
         .dstSubresource =
      (VkImageSubresourceLayers){
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .mipLevel = dst_iview->vk.base_mip_level,
   .baseArrayLayer = dst_iview->vk.base_array_layer,
         .srcOffset = {resolve_area.offset.x, resolve_area.offset.y, 0},
               switch (resolve_method) {
   case RESOLVE_HW:
      radv_cmd_buffer_resolve_rendering_hw(cmd_buffer);
                        radv_cmd_buffer_resolve_rendering_cs(cmd_buffer, src_iview, src_layout, dst_iview, dst_layout, &region);
                        radv_cmd_buffer_resolve_rendering_fs(cmd_buffer, src_iview, src_layout, dst_iview, dst_layout);
      default:
                           }
      /**
   * Decompress CMask/FMask before resolving a multisampled source image inside a
   * subpass.
   */
   void
   radv_decompress_resolve_rendering_src(struct radv_cmd_buffer *cmd_buffer)
   {
               uint32_t layer_count = render->layer_count;
   if (render->view_mask)
            for (uint32_t i = 0; i < render->color_att_count; ++i) {
      if (render->color_att[i].resolve_iview == NULL)
            struct radv_image_view *src_iview = render->color_att[i].iview;
   VkImageLayout src_layout = render->color_att[i].layout;
            VkImageResolve2 region = {0};
   region.sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2;
   region.srcSubresource.aspectMask = src_iview->vk.aspects;
   region.srcSubresource.mipLevel = 0;
   region.srcSubresource.baseArrayLayer = src_iview->vk.base_array_layer;
                  }
      /**
   * Decompress CMask/FMask before resolving a multisampled source image.
   */
   void
   radv_decompress_resolve_src(struct radv_cmd_buffer *cmd_buffer, struct radv_image *src_image,
         {
               VkImageMemoryBarrier2 barrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
   .srcStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
   .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
   .dstStageMask = VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
   .dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT,
   .oldLayout = src_image_layout,
   .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
   .image = radv_image_to_handle(src_image),
   .subresourceRange = (VkImageSubresourceRange){
      .aspectMask = region->srcSubresource.aspectMask,
   .baseMipLevel = region->srcSubresource.mipLevel,
   .levelCount = 1,
   .baseArrayLayer = src_base_layer,
            VkSampleLocationsInfoEXT sample_loc_info;
   if (src_image->vk.create_flags & VK_IMAGE_CREATE_SAMPLE_LOCATIONS_COMPATIBLE_DEPTH_BIT_EXT) {
      /* If the depth/stencil image uses different sample
   * locations, we need them during HTILE decompressions.
   */
            sample_loc_info = (VkSampleLocationsInfoEXT){
      .sType = VK_STRUCTURE_TYPE_SAMPLE_LOCATIONS_INFO_EXT,
   .sampleLocationsPerPixel = sample_locs->per_pixel,
   .sampleLocationGridSize = sample_locs->grid_size,
   .sampleLocationsCount = sample_locs->count,
      };
               VkDependencyInfo dep_info = {
      .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
   .imageMemoryBarrierCount = 1,
                  }
