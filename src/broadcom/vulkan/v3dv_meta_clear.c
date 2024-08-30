   /*
   * Copyright © 2020 Raspberry Pi Ltd
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
      #include "v3dv_private.h"
   #include "v3dv_meta_common.h"
      #include "compiler/nir/nir_builder.h"
   #include "util/u_pack_color.h"
      static void
   get_hw_clear_color(struct v3dv_device *device,
                     const VkClearColorValue *color,
   VkFormat fb_format,
   VkFormat image_format,
   {
               /* If the image format doesn't match the framebuffer format, then we are
   * trying to clear an unsupported tlb format using a compatible
   * format for the framebuffer. In this case, we want to make sure that
   * we pack the clear value according to the original format semantics,
   * not the compatible format.
   */
   if (fb_format == image_format) {
      v3dv_X(device, get_hw_clear_color)(color, internal_type, internal_size,
      } else {
      union util_color uc;
   enum pipe_format pipe_image_format =
         util_pack_color(color->float32, pipe_image_format, &uc);
         }
      /* Returns true if the implementation is able to handle the case, false
   * otherwise.
   */
   static bool
   clear_image_tlb(struct v3dv_cmd_buffer *cmd_buffer,
                     {
      const VkOffset3D origin = { 0, 0, 0 };
            /* From vkCmdClearColorImage spec:
   *  "image must not use any of the formats that require a sampler YCBCR
   *   conversion"
   */
   assert(image->plane_count == 1);
   if (!v3dv_meta_can_use_tlb(image, 0, 0, &origin, NULL, &fb_format))
            uint32_t internal_type, internal_bpp;
   v3dv_X(cmd_buffer->device, get_internal_type_bpp_for_image_aspects)
      (fb_format, range->aspectMask,
         union v3dv_clear_value hw_clear_value = { 0 };
   if (range->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      get_hw_clear_color(cmd_buffer->device, &clear_value->color, fb_format,
            } else {
      assert((range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) ||
         hw_clear_value.z = clear_value->depthStencil.depth;
               uint32_t level_count = vk_image_subresource_level_count(&image->vk, range);
   uint32_t min_level = range->baseMipLevel;
            /* For 3D images baseArrayLayer and layerCount must be 0 and 1 respectively.
   * Instead, we need to consider the full depth dimension of the image, which
   * goes from 0 up to the level's depth extent.
   */
   uint32_t min_layer;
   uint32_t max_layer;
   if (image->vk.image_type != VK_IMAGE_TYPE_3D) {
      min_layer = range->baseArrayLayer;
   max_layer = range->baseArrayLayer +
      } else {
      min_layer = 0;
               for (uint32_t level = min_level; level < max_level; level++) {
      if (image->vk.image_type == VK_IMAGE_TYPE_3D)
            uint32_t width = u_minify(image->vk.extent.width, level);
            struct v3dv_job *job =
            if (!job)
            v3dv_job_start_frame(job, width, height, max_layer,
                        struct v3dv_meta_framebuffer framebuffer;
   v3dv_X(job->device, meta_framebuffer_init)(&framebuffer, fb_format,
                           /* If this triggers it is an application bug: the spec requires
   * that any aspects to clear are present in the image.
   */
            v3dv_X(job->device, meta_emit_clear_image_rcl)
                                 }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdClearColorImage(VkCommandBuffer commandBuffer,
                           VkImage _image,
   {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
            const VkClearValue clear_value = {
                           for (uint32_t i = 0; i < rangeCount; i++) {
      if (clear_image_tlb(cmd_buffer, image, &clear_value, &pRanges[i]))
                        }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer,
                                 {
      V3DV_FROM_HANDLE(v3dv_cmd_buffer, cmd_buffer, commandBuffer);
            const VkClearValue clear_value = {
                           for (uint32_t i = 0; i < rangeCount; i++) {
      if (clear_image_tlb(cmd_buffer, image, &clear_value, &pRanges[i]))
                        }
      static void
   destroy_color_clear_pipeline(VkDevice _device,
               {
      struct v3dv_meta_color_clear_pipeline *p =
         v3dv_DestroyPipeline(_device, p->pipeline, alloc);
   if (p->cached)
            }
      static void
   destroy_depth_clear_pipeline(VkDevice _device,
               {
      v3dv_DestroyPipeline(_device, p->pipeline, alloc);
      }
      static VkResult
   create_color_clear_pipeline_layout(struct v3dv_device *device,
         {
      /* FIXME: this is abusing a bit the API, since not all of our clear
   * pipelines have a geometry shader. We could create 2 different pipeline
   * layouts, but this works for us for now.
   */
   VkPushConstantRange ranges[2] = {
      { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 16 },
               VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pushConstantRangeCount = 2,
               return v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
      }
      static VkResult
   create_depth_clear_pipeline_layout(struct v3dv_device *device,
         {
      /* FIXME: this is abusing a bit the API, since not all of our clear
   * pipelines have a geometry shader. We could create 2 different pipeline
   * layouts, but this works for us for now.
   */
   VkPushConstantRange ranges[2] = {
      { VK_SHADER_STAGE_FRAGMENT_BIT, 0, 4 },
               VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pushConstantRangeCount = 2,
               return v3dv_CreatePipelineLayout(v3dv_device_to_handle(device),
      }
      void
   v3dv_meta_clear_init(struct v3dv_device *device)
   {
      device->meta.color_clear.cache =
            create_color_clear_pipeline_layout(device,
            device->meta.depth_clear.cache =
            create_depth_clear_pipeline_layout(device,
      }
      void
   v3dv_meta_clear_finish(struct v3dv_device *device)
   {
               hash_table_foreach(device->meta.color_clear.cache, entry) {
      struct v3dv_meta_color_clear_pipeline *item = entry->data;
      }
            if (device->meta.color_clear.p_layout) {
      v3dv_DestroyPipelineLayout(_device, device->meta.color_clear.p_layout,
               hash_table_foreach(device->meta.depth_clear.cache, entry) {
      struct v3dv_meta_depth_clear_pipeline *item = entry->data;
      }
            if (device->meta.depth_clear.p_layout) {
      v3dv_DestroyPipelineLayout(_device, device->meta.depth_clear.p_layout,
         }
      static nir_shader *
   get_clear_rect_vs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_VERTEX, options,
            const struct glsl_type *vec4 = glsl_vec4_type();
   nir_variable *vs_out_pos =
                  nir_def *pos = nir_gen_rect_vertices(&b, NULL, NULL);
               }
      static nir_shader *
   get_clear_rect_gs(uint32_t push_constant_layer_base)
   {
      /* FIXME: this creates a geometry shader that takes the index of a single
   * layer to clear from push constants, so we need to emit a draw call for
   * each layer that we want to clear. We could actually do better and have it
   * take a range of layers and then emit one triangle per layer to clear,
   * however, if we were to do this we would need to be careful not to exceed
   * the maximum number of output vertices allowed in a geometry shader.
   */
   const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY, options,
         nir_shader *nir = b.shader;
   nir->info.inputs_read = 1ull << VARYING_SLOT_POS;
   nir->info.outputs_written = (1ull << VARYING_SLOT_POS) |
         nir->info.gs.input_primitive = MESA_PRIM_TRIANGLES;
   nir->info.gs.output_primitive = MESA_PRIM_TRIANGLE_STRIP;
   nir->info.gs.vertices_in = 3;
   nir->info.gs.vertices_out = 3;
   nir->info.gs.invocations = 1;
            /* in vec4 gl_Position[3] */
   nir_variable *gs_in_pos =
      nir_variable_create(b.shader, nir_var_shader_in,
                     /* out vec4 gl_Position */
   nir_variable *gs_out_pos =
      nir_variable_create(b.shader, nir_var_shader_out, glsl_vec4_type(),
               /* out float gl_Layer */
   nir_variable *gs_out_layer =
      nir_variable_create(b.shader, nir_var_shader_out, glsl_float_type(),
               /* Emit output triangle */
   for (uint32_t i = 0; i < 3; i++) {
      /* gl_Position from shader input */
   nir_deref_instr *in_pos_i =
                  /* gl_Layer from push constants */
   nir_def *layer =
      nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 0),
                                       }
      static nir_shader *
   get_color_clear_rect_fs(uint32_t rt_idx, VkFormat format)
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, options,
            enum pipe_format pformat = vk_format_to_pipe_format(format);
   const struct glsl_type *fs_out_type =
            nir_variable *fs_out_color =
                  nir_def *color_load = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .base = 0, .range = 16);
               }
      static nir_shader *
   get_depth_clear_rect_fs()
   {
      const nir_shader_compiler_options *options = v3dv_pipeline_get_nir_options();
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, options,
            nir_variable *fs_out_depth =
      nir_variable_create(b.shader, nir_var_shader_out, glsl_float_type(),
               nir_def *depth_load =
                        }
      static VkResult
   create_pipeline(struct v3dv_device *device,
                  struct v3dv_render_pass *pass,
   uint32_t subpass_idx,
   uint32_t samples,
   struct nir_shader *vs_nir,
   struct nir_shader *gs_nir,
   struct nir_shader *fs_nir,
   const VkPipelineVertexInputStateCreateInfo *vi_state,
   const VkPipelineDepthStencilStateCreateInfo *ds_state,
      {
      VkPipelineShaderStageCreateInfo stages[3] = { 0 };
   struct vk_shader_module vs_m = vk_shader_module_from_nir(vs_nir);
   struct vk_shader_module gs_m;
            uint32_t stage_count = 0;
   stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stages[stage_count].stage = VK_SHADER_STAGE_VERTEX_BIT;
   stages[stage_count].module = vk_shader_module_to_handle(&vs_m);
   stages[stage_count].pName = "main";
            if (gs_nir) {
      gs_m = vk_shader_module_from_nir(gs_nir);
   stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stages[stage_count].stage = VK_SHADER_STAGE_GEOMETRY_BIT;
   stages[stage_count].module = vk_shader_module_to_handle(&gs_m);
   stages[stage_count].pName = "main";
               if (fs_nir) {
      fs_m = vk_shader_module_from_nir(fs_nir);
   stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   stages[stage_count].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   stages[stage_count].module = vk_shader_module_to_handle(&fs_m);
   stages[stage_count].pName = "main";
               VkGraphicsPipelineCreateInfo info = {
               .stageCount = stage_count,
                     .pInputAssemblyState = &(VkPipelineInputAssemblyStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
   .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
               .pViewportState = &(VkPipelineViewportStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
   .viewportCount = 1,
               .pRasterizationState = &(VkPipelineRasterizationStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
   .rasterizerDiscardEnable = false,
   .polygonMode = VK_POLYGON_MODE_FILL,
   .cullMode = VK_CULL_MODE_NONE,
   .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
               .pMultisampleState = &(VkPipelineMultisampleStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
   .rasterizationSamples = samples,
   .sampleShadingEnable = false,
   .pSampleMask = NULL,
   .alphaToCoverageEnable = false,
                                 /* The meta clear pipeline declares all state as dynamic.
   * As a consequence, vkCmdBindPipeline writes no dynamic state
   * to the cmd buffer. Therefore, at the end of the meta clear,
   * we need only restore dynamic state that was vkCmdSet.
   */
   .pDynamicState = &(VkPipelineDynamicStateCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
   .dynamicStateCount = 6,
   .pDynamicStates = (VkDynamicState[]) {
      VK_DYNAMIC_STATE_VIEWPORT,
   VK_DYNAMIC_STATE_SCISSOR,
   VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
   VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
   VK_DYNAMIC_STATE_STENCIL_REFERENCE,
   VK_DYNAMIC_STATE_BLEND_CONSTANTS,
   VK_DYNAMIC_STATE_DEPTH_BIAS,
                  .flags = 0,
   .layout = layout,
   .renderPass = v3dv_render_pass_to_handle(pass),
               VkResult result =
      v3dv_CreateGraphicsPipelines(v3dv_device_to_handle(device),
                           ralloc_free(vs_nir);
   ralloc_free(gs_nir);
               }
      static VkResult
   create_color_clear_pipeline(struct v3dv_device *device,
                              struct v3dv_render_pass *pass,
   uint32_t subpass_idx,
   uint32_t rt_idx,
   VkFormat format,
      {
      nir_shader *vs_nir = get_clear_rect_vs();
   nir_shader *fs_nir = get_color_clear_rect_fs(rt_idx, format);
            const VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
               const VkPipelineDepthStencilStateCreateInfo ds_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = false,
   .depthWriteEnable = false,
   .depthBoundsTestEnable = false,
               assert(subpass_idx < pass->subpass_count);
   const uint32_t color_count = pass->subpasses[subpass_idx].color_count;
            VkPipelineColorBlendAttachmentState blend_att_state[V3D_MAX_DRAW_BUFFERS];
   for (uint32_t i = 0; i < color_count; i++) {
      blend_att_state[i] = (VkPipelineColorBlendAttachmentState) {
      .blendEnable = false,
                  const VkPipelineColorBlendStateCreateInfo cb_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .logicOpEnable = false,
   .attachmentCount = color_count,
               return create_pipeline(device,
                        pass, subpass_idx,
   samples,
   vs_nir, gs_nir, fs_nir,
   &vi_state,
   }
      static VkResult
   create_depth_clear_pipeline(struct v3dv_device *device,
                              VkImageAspectFlags aspects,
   struct v3dv_render_pass *pass,
      {
      const bool has_depth = aspects & VK_IMAGE_ASPECT_DEPTH_BIT;
   const bool has_stencil = aspects & VK_IMAGE_ASPECT_STENCIL_BIT;
            nir_shader *vs_nir = get_clear_rect_vs();
   nir_shader *fs_nir = has_depth ? get_depth_clear_rect_fs() : NULL;
            const VkPipelineVertexInputStateCreateInfo vi_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
   .vertexBindingDescriptionCount = 0,
               const VkPipelineDepthStencilStateCreateInfo ds_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
   .depthTestEnable = has_depth,
   .depthWriteEnable = has_depth,
   .depthCompareOp = VK_COMPARE_OP_ALWAYS,
   .depthBoundsTestEnable = false,
   .stencilTestEnable = has_stencil,
   .front = {
      .passOp = VK_STENCIL_OP_REPLACE,
   .compareOp = VK_COMPARE_OP_ALWAYS,
      },
               assert(subpass_idx < pass->subpass_count);
   VkPipelineColorBlendAttachmentState blend_att_state[V3D_MAX_DRAW_BUFFERS] = { 0 };
   const VkPipelineColorBlendStateCreateInfo cb_state = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
   .logicOpEnable = false,
   .attachmentCount = pass->subpasses[subpass_idx].color_count,
               return create_pipeline(device,
                        pass, subpass_idx,
   samples,
   vs_nir, gs_nir, fs_nir,
   &vi_state,
   }
      static VkResult
   create_color_clear_render_pass(struct v3dv_device *device,
                           {
      VkAttachmentDescription2 att = {
      .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
   .format = format,
   .samples = samples,
   .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
   .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
   .initialLayout = VK_IMAGE_LAYOUT_GENERAL,
               VkAttachmentReference2 att_ref = {
      .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
   .attachment = rt_idx,
               VkSubpassDescription2 subpass = {
      .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
   .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
   .inputAttachmentCount = 0,
   .colorAttachmentCount = 1,
   .pColorAttachments = &att_ref,
   .pResolveAttachments = NULL,
   .pDepthStencilAttachment = NULL,
   .preserveAttachmentCount = 0,
               VkRenderPassCreateInfo2 info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
   .attachmentCount = 1,
   .pAttachments = &att,
   .subpassCount = 1,
   .pSubpasses = &subpass,
   .dependencyCount = 0,
               return v3dv_CreateRenderPass2(v3dv_device_to_handle(device),
      }
      static inline uint64_t
   get_color_clear_pipeline_cache_key(uint32_t rt_idx,
                           {
               uint64_t key = 0;
            key |= rt_idx;
            key |= ((uint64_t) format) << bit_offset;
            key |= ((uint64_t) samples) << bit_offset;
            key |= ((uint64_t) components) << bit_offset;
            key |= (is_layered ? 1ull : 0ull) << bit_offset;
            assert(bit_offset <= 64);
      }
      static inline uint64_t
   get_depth_clear_pipeline_cache_key(VkImageAspectFlags aspects,
                     {
      uint64_t key = 0;
            key |= format;
            key |= ((uint64_t) samples) << bit_offset;
            const bool has_depth = (aspects & VK_IMAGE_ASPECT_DEPTH_BIT) ? 1 : 0;
   key |= ((uint64_t) has_depth) << bit_offset;
            const bool has_stencil = (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) ? 1 : 0;
   key |= ((uint64_t) has_stencil) << bit_offset;
            key |= (is_layered ? 1ull : 0ull) << bit_offset;
            assert(bit_offset <= 64);
      }
      static VkResult
   get_color_clear_pipeline(struct v3dv_device *device,
                           struct v3dv_render_pass *pass,
   uint32_t subpass_idx,
   uint32_t rt_idx,
   uint32_t attachment_idx,
   VkFormat format,
   {
                        /* If pass != NULL it means that we are emitting the clear as a draw call
   * in the current pass bound by the application. In that case, we can't
   * cache the pipeline, since it will be referencing that pass and the
   * application could be destroying it at any point. Hopefully, the perf
   * impact is not too big since we still have the device pipeline cache
   * around and we won't end up re-compiling the clear shader.
   *
   * FIXME: alternatively, we could refcount (or maybe clone) the render pass
   * provided by the application and include it in the pipeline key setup
   * to make caching safe in this scenario, however, based on tests with
   * vkQuake3, the fact that we are not caching here doesn't seem to have
   * any significant impact in performance, so it might not be worth it.
   */
            uint64_t key;
   if (can_cache_pipeline) {
      key = get_color_clear_pipeline_cache_key(rt_idx, format, samples,
         mtx_lock(&device->meta.mtx);
   struct hash_entry *entry =
         if (entry) {
      mtx_unlock(&device->meta.mtx);
   *pipeline = entry->data;
                  *pipeline = vk_zalloc2(&device->vk.alloc, NULL, sizeof(**pipeline), 8,
            if (*pipeline == NULL) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               if (!pass) {
      result = create_color_clear_render_pass(device,
                           if (result != VK_SUCCESS)
               } else {
                  result = create_color_clear_pipeline(device,
                                       pass,
   subpass_idx,
   rt_idx,
   if (result != VK_SUCCESS)
            if (can_cache_pipeline) {
      (*pipeline)->key = key;
   (*pipeline)->cached = true;
   _mesa_hash_table_insert(device->meta.color_clear.cache,
                              fail:
      if (can_cache_pipeline)
            VkDevice _device = v3dv_device_to_handle(device);
   if (*pipeline) {
      if ((*pipeline)->cached)
         if ((*pipeline)->pipeline)
         vk_free(&device->vk.alloc, *pipeline);
                  }
      static VkResult
   get_depth_clear_pipeline(struct v3dv_device *device,
                           VkImageAspectFlags aspects,
   struct v3dv_render_pass *pass,
   {
      assert(subpass_idx < pass->subpass_count);
   assert(attachment_idx != VK_ATTACHMENT_UNUSED);
                     const uint32_t samples = pass->attachments[attachment_idx].desc.samples;
   const VkFormat format = pass->attachments[attachment_idx].desc.format;
            const uint64_t key =
         mtx_lock(&device->meta.mtx);
   struct hash_entry *entry =
         if (entry) {
      mtx_unlock(&device->meta.mtx);
   *pipeline = entry->data;
               *pipeline = vk_zalloc2(&device->vk.alloc, NULL, sizeof(**pipeline), 8,
            if (*pipeline == NULL) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               result = create_depth_clear_pipeline(device,
                                       aspects,
   if (result != VK_SUCCESS)
            (*pipeline)->key = key;
   _mesa_hash_table_insert(device->meta.depth_clear.cache,
            mtx_unlock(&device->meta.mtx);
         fail:
               VkDevice _device = v3dv_device_to_handle(device);
   if (*pipeline) {
      if ((*pipeline)->pipeline)
         vk_free(&device->vk.alloc, *pipeline);
                  }
      /* Emits a scissored quad in the clear color */
   static void
   emit_subpass_color_clear_rects(struct v3dv_cmd_buffer *cmd_buffer,
                                 struct v3dv_render_pass *pass,
   struct v3dv_subpass *subpass,
   uint32_t rt_idx,
   {
      /* Skip if attachment is unused in the current subpass */
   assert(rt_idx < subpass->color_count);
   const uint32_t attachment_idx = subpass->color_attachments[rt_idx].attachment;
   if (attachment_idx == VK_ATTACHMENT_UNUSED)
            /* Obtain a pipeline for this clear */
   assert(attachment_idx < cmd_buffer->state.pass->attachment_count);
   const VkFormat format =
         const VkSampleCountFlagBits samples =
         const uint32_t components = VK_COLOR_COMPONENT_R_BIT |
                     struct v3dv_meta_color_clear_pipeline *pipeline = NULL;
   VkResult result = get_color_clear_pipeline(cmd_buffer->device,
                                             pass,
   cmd_buffer->state.subpass_idx,
   if (result != VK_SUCCESS) {
      if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
            }
            /* Emit clear rects */
            VkCommandBuffer cmd_buffer_handle = v3dv_cmd_buffer_to_handle(cmd_buffer);
   v3dv_CmdPushConstants(cmd_buffer_handle,
                        v3dv_CmdBindPipeline(cmd_buffer_handle,
                  for (uint32_t i = 0; i < rect_count; i++) {
      const VkViewport viewport = {
      .x = rects[i].rect.offset.x,
   .y = rects[i].rect.offset.y,
   .width = rects[i].rect.extent.width,
   .height = rects[i].rect.extent.height,
   .minDepth = 0.0f,
      };
   v3dv_CmdSetViewport(cmd_buffer_handle, 0, 1, &viewport);
            if (is_layered) {
      for (uint32_t layer_offset = 0; layer_offset < rects[i].layerCount;
      layer_offset++) {
   uint32_t layer = rects[i].baseArrayLayer + layer_offset;
   v3dv_CmdPushConstants(cmd_buffer_handle,
                     } else {
      assert(rects[i].baseArrayLayer == 0 && rects[i].layerCount == 1);
                  /* Subpass pipelines can't be cached because they include a reference to the
   * render pass currently bound by the application, which means that we need
   * to destroy them manually here.
   */
   assert(!pipeline->cached);
   v3dv_cmd_buffer_add_private_obj(
      cmd_buffer, (uintptr_t)pipeline,
            }
      /* Emits a scissored quad, clearing the depth aspect by writing to gl_FragDepth
   * and the stencil aspect by using stencil testing.
   */
   static void
   emit_subpass_ds_clear_rects(struct v3dv_cmd_buffer *cmd_buffer,
                              struct v3dv_render_pass *pass,
   struct v3dv_subpass *subpass,
   VkImageAspectFlags aspects,
      {
      /* Skip if attachment is unused in the current subpass */
   const uint32_t attachment_idx = subpass->ds_attachment.attachment;
   if (attachment_idx == VK_ATTACHMENT_UNUSED)
            /* Obtain a pipeline for this clear */
   assert(attachment_idx < cmd_buffer->state.pass->attachment_count);
   struct v3dv_meta_depth_clear_pipeline *pipeline = NULL;
   VkResult result = get_depth_clear_pipeline(cmd_buffer->device,
                                       if (result != VK_SUCCESS) {
      if (result == VK_ERROR_OUT_OF_HOST_MEMORY)
            }
            /* Emit clear rects */
            VkCommandBuffer cmd_buffer_handle = v3dv_cmd_buffer_to_handle(cmd_buffer);
   v3dv_CmdPushConstants(cmd_buffer_handle,
                        v3dv_CmdBindPipeline(cmd_buffer_handle,
                  if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      v3dv_CmdSetStencilReference(cmd_buffer_handle,
               v3dv_CmdSetStencilWriteMask(cmd_buffer_handle,
         v3dv_CmdSetStencilCompareMask(cmd_buffer_handle,
               for (uint32_t i = 0; i < rect_count; i++) {
      const VkViewport viewport = {
      .x = rects[i].rect.offset.x,
   .y = rects[i].rect.offset.y,
   .width = rects[i].rect.extent.width,
   .height = rects[i].rect.extent.height,
   .minDepth = 0.0f,
      };
   v3dv_CmdSetViewport(cmd_buffer_handle, 0, 1, &viewport);
   v3dv_CmdSetScissor(cmd_buffer_handle, 0, 1, &rects[i].rect);
   if (is_layered) {
      for (uint32_t layer_offset = 0; layer_offset < rects[i].layerCount;
      layer_offset++) {
   uint32_t layer = rects[i].baseArrayLayer + layer_offset;
   v3dv_CmdPushConstants(cmd_buffer_handle,
                     } else {
      assert(rects[i].baseArrayLayer == 0 && rects[i].layerCount == 1);
                     }
      static void
   gather_layering_info(uint32_t rect_count, const VkClearRect *rects,
         {
               uint32_t min_layer = rects[0].baseArrayLayer;
   uint32_t max_layer = rects[0].baseArrayLayer + rects[0].layerCount - 1;
   for (uint32_t i = 1; i < rect_count; i++) {
      if (rects[i].baseArrayLayer != rects[i - 1].baseArrayLayer ||
      rects[i].layerCount != rects[i - 1].layerCount) {
   *all_rects_same_layers = false;
   min_layer = MIN2(min_layer, rects[i].baseArrayLayer);
   max_layer = MAX2(max_layer, rects[i].baseArrayLayer +
                     }
      VKAPI_ATTR void VKAPI_CALL
   v3dv_CmdClearAttachments(VkCommandBuffer commandBuffer,
                           {
               /* We can have at most max_color_RTs + 1 D/S attachments */
   assert(attachmentCount <=
            /* We can only clear attachments in the current subpass */
            assert(cmd_buffer->state.subpass_idx < pass->subpass_count);
   struct v3dv_subpass *subpass =
            /* Emit a clear rect inside the current job for this subpass. For layered
   * framebuffers, we use a geometry shader to redirect clears to the
   * appropriate layers.
                     bool is_layered, all_rects_same_layers;
   gather_layering_info(rectCount, pRects, &is_layered, &all_rects_same_layers);
   for (uint32_t i = 0; i < attachmentCount; i++) {
      if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      emit_subpass_color_clear_rects(cmd_buffer, pass, subpass,
                        } else {
      emit_subpass_ds_clear_rects(cmd_buffer, pass, subpass,
                                       }
