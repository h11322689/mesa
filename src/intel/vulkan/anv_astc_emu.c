   /*
   * Copyright 2023 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "anv_private.h"
      #include "compiler/nir/nir_builder.h"
      static void
   astc_emu_init_image_view(struct anv_cmd_buffer *cmd_buffer,
                           struct anv_image_view *iview,
   {
               const VkImageViewCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .pNext = &(VkImageViewUsageCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO,
      },
   .image = anv_image_to_handle(image),
   /* XXX we only need 2D but the shader expects 2D_ARRAY */
   .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
   .format = format,
   .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
   .baseMipLevel = level,
   .levelCount = 1,
   .baseArrayLayer = layer,
                  memset(iview, 0, sizeof(*iview));
   anv_image_view_init(device, iview, &create_info,
      }
      static void
   astc_emu_init_push_descriptor_set(struct anv_cmd_buffer *cmd_buffer,
                           {
      struct anv_device *device = cmd_buffer->device;
   struct anv_descriptor_set_layout *layout =
            memset(push_set, 0, sizeof(*push_set));
               }
      static void
   astc_emu_init_flush_denorm_shader(nir_builder *b)
   {
      b->shader->info.workgroup_size[0] = 8;
            const struct glsl_type *src_type =
         nir_variable *src_var =
         src_var->data.descriptor_set = 0;
            const struct glsl_type *dst_type =
         nir_variable *dst_var =
         dst_var->data.descriptor_set = 0;
            nir_def *zero = nir_imm_int(b, 0);
   nir_def *consts = nir_load_push_constant(b, 4, 32, zero, .range = 16);
   nir_def *offset = nir_channels(b, consts, 0x3);
            nir_def *coord = nir_load_global_invocation_id(b, 32);
            nir_def *cond = nir_ilt(b, coord, extent);
   cond = nir_iand(b, nir_channel(b, cond, 0), nir_channel(b, cond, 1));
   nir_push_if(b, cond);
   {
      const struct glsl_type *val_type = glsl_vector_type(GLSL_TYPE_UINT, 4);
   nir_variable *val_var =
            coord = nir_vec3(b, nir_channel(b, coord, 0), nir_channel(b, coord, 1),
         nir_def *val =
                  /* A void-extent block has this layout
   *
   *   struct astc_void_extent_block {
   *      uint16_t header;
   *      uint16_t dontcare0;
   *      uint16_t dontcare1;
   *      uint16_t dontcare2;
   *      uint16_t R;
   *      uint16_t G;
   *      uint16_t B;
   *      uint16_t A;
   *   };
   *
   * where the lower 12 bits are 0xdfc for 2D LDR.
   */
   nir_def *block_mode = nir_iand_imm(b, nir_channel(b, val, 0), 0xfff);
   nir_push_if(b, nir_ieq_imm(b, block_mode, 0xdfc));
   {
                     /* flush denorms */
                  color = nir_unpack_64_2x32(b, nir_pack_64_4x16(b, comps));
   val = nir_vec4(b, nir_channel(b, val, 0), nir_channel(b, val, 1),
            }
            nir_def *dst = &nir_build_deref_var(b, dst_var)->def;
   coord = nir_pad_vector(b, coord, 4);
   val = nir_load_var(b, val_var);
   nir_image_deref_store(b, dst, coord, nir_undef(b, 1, 32), val, zero,
            }
      }
      static VkResult
   astc_emu_init_flush_denorm_pipeline_locked(struct anv_device *device)
   {
      struct anv_device_astc_emu *astc_emu = &device->astc_emu;
   VkDevice _device = anv_device_to_handle(device);
            if (astc_emu->ds_layout == VK_NULL_HANDLE) {
      const VkDescriptorSetLayoutCreateInfo ds_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 2,
   .pBindings = (VkDescriptorSetLayoutBinding[]){
      {
      .binding = 0,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
      },
   {
      .binding = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .descriptorCount = 1,
            };
   result = anv_CreateDescriptorSetLayout(_device, &ds_layout_create_info,
         if (result != VK_SUCCESS)
               if (astc_emu->pipeline_layout == VK_NULL_HANDLE) {
      const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &astc_emu->ds_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges = &(VkPushConstantRange){
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
         };
   result = anv_CreatePipelineLayout(_device, &pipeline_layout_create_info,
         if (result != VK_SUCCESS)
               if (astc_emu->pipeline == VK_NULL_HANDLE) {
      const struct nir_shader_compiler_options *options =
         nir_builder b = nir_builder_init_simple_shader(
                  const VkComputePipelineCreateInfo pipeline_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage =
      (VkPipelineShaderStageCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(b.shader),
            };
   result = anv_CreateComputePipelines(_device, VK_NULL_HANDLE, 1,
                        if (result != VK_SUCCESS)
            out:
         }
      static VkResult
   astc_emu_init_flush_denorm_pipeline(struct anv_device *device)
   {
      struct anv_device_astc_emu *astc_emu = &device->astc_emu;
            simple_mtx_lock(&astc_emu->mutex);
   if (!astc_emu->pipeline)
                     }
      static void
   astc_emu_flush_denorm_slice(struct anv_cmd_buffer *cmd_buffer,
                                 {
      struct anv_device *device = cmd_buffer->device;
   struct anv_device_astc_emu *astc_emu = &device->astc_emu;
            VkResult result = astc_emu_init_flush_denorm_pipeline(device);
   if (result != VK_SUCCESS) {
      anv_batch_set_error(&cmd_buffer->batch, result);
               const uint32_t push_const[] = {
      rect.offset.x,
   rect.offset.y,
   rect.offset.x + rect.extent.width,
               const VkWriteDescriptorSet set_writes[] = {
      {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .pImageInfo = &(VkDescriptorImageInfo){
      .imageView = src_view,
         },
   {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .pImageInfo = &(VkDescriptorImageInfo){
      .imageView = dst_view,
            };
   struct anv_push_descriptor_set push_set;
   astc_emu_init_push_descriptor_set(cmd_buffer,
                                    anv_CmdBindPipeline(cmd_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
         anv_CmdPushConstants(cmd_buffer_, astc_emu->pipeline_layout,
               anv_CmdBindDescriptorSets(cmd_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                  /* each workgroup processes 8x8 texel blocks */
   rect.extent.width = DIV_ROUND_UP(rect.extent.width, 8);
            anv_genX(device->info, CmdDispatchBase)(cmd_buffer_, 0, 0, 0,
                           }
      static void
   astc_emu_decompress_slice(struct anv_cmd_buffer *cmd_buffer,
                           VkFormat astc_format,
   {
      struct anv_device *device = cmd_buffer->device;
   struct anv_device_astc_emu *astc_emu = &device->astc_emu;
            VkPipeline pipeline =
      vk_texcompress_astc_get_decode_pipeline(&device->vk, &device->vk.alloc,
            if (pipeline == VK_NULL_HANDLE) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_UNKNOWN);
                        struct vk_texcompress_astc_write_descriptor_set writes;
   vk_texcompress_astc_fill_write_descriptor_sets(astc_emu->texcompress,
                  struct anv_push_descriptor_set push_set;
   astc_emu_init_push_descriptor_set(cmd_buffer, &push_set,
                        VkDescriptorSet set = anv_descriptor_set_to_handle(&push_set.set);
   anv_CmdBindDescriptorSets(cmd_buffer_, VK_PIPELINE_BIND_POINT_COMPUTE,
                  const uint32_t push_const[] = {
      rect.offset.x,
   rect.offset.y,
   (rect.offset.x + rect.extent.width) *
         (rect.offset.y + rect.extent.height) *
            };
   anv_CmdPushConstants(cmd_buffer_, astc_emu->texcompress->p_layout,
                  /* each workgroup processes 2x2 texel blocks */
   rect.extent.width = DIV_ROUND_UP(rect.extent.width, 2);
            anv_genX(device->info, CmdDispatchBase)(cmd_buffer_, 0, 0, 0,
                           }
      void
   anv_astc_emu_process(struct anv_cmd_buffer *cmd_buffer,
                        struct anv_image *image,
      {
      const bool flush_denorms =
                     const VkRect2D rect = {
      .offset = {
      .x = block_offset.x,
      },
   .extent = {
      .width = block_extent.width,
                  /* process one layer at a time because anv_image_fill_surface_state
   * requires an uncompressed view of a compressed image to be single layer
   */
   const bool is_3d = image->vk.image_type == VK_IMAGE_TYPE_3D;
   const uint32_t slice_base = is_3d ?
         const uint32_t slice_count = is_3d ?
            struct anv_cmd_saved_state saved;
   anv_cmd_buffer_save_state(cmd_buffer,
                              for (uint32_t i = 0; i < slice_count; i++) {
      struct anv_image_view src_view;
   struct anv_image_view dst_view;
   astc_emu_init_image_view(cmd_buffer, &src_view, image,
                     astc_emu_init_image_view(cmd_buffer, &dst_view, image,
                              if (flush_denorms) {
      astc_emu_flush_denorm_slice(cmd_buffer, image->vk.format, layout,
                  } else {
      astc_emu_decompress_slice(cmd_buffer, image->vk.format, layout,
                                 }
      VkResult
   anv_device_init_astc_emu(struct anv_device *device)
   {
      struct anv_device_astc_emu *astc_emu = &device->astc_emu;
            if (device->physical->flush_astc_ldr_void_extent_denorms)
            if (device->physical->emu_astc_ldr) {
      result = vk_texcompress_astc_init(&device->vk, &device->vk.alloc,
                        }
      void
   anv_device_finish_astc_emu(struct anv_device *device)
   {
               if (device->physical->flush_astc_ldr_void_extent_denorms) {
               anv_DestroyPipeline(_device, astc_emu->pipeline, NULL);
   anv_DestroyPipelineLayout(_device, astc_emu->pipeline_layout, NULL);
   anv_DestroyDescriptorSetLayout(_device, astc_emu->ds_layout, NULL);
               if (astc_emu->texcompress) {
      vk_texcompress_astc_finish(&device->vk, &device->vk.alloc,
         }
