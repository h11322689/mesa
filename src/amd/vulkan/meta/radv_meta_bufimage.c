   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
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
      /*
   * GFX queue: Compute shader implementation of image->buffer copy
   * Compute queue: implementation also of buffer->image, image->image, and image clear.
   */
      static nir_shader *
   build_nir_itob_compute_shader(struct radv_device *dev, bool is_3d)
   {
      enum glsl_sampler_dim dim = is_3d ? GLSL_SAMPLER_DIM_3D : GLSL_SAMPLER_DIM_2D;
   const struct glsl_type *sampler_type = glsl_sampler_type(dim, false, false, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type = glsl_image_type(GLSL_SAMPLER_DIM_BUF, false, GLSL_TYPE_FLOAT);
   nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, is_3d ? "meta_itob_cs_3d" : "meta_itob_cs");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_uniform, sampler_type, "s_tex");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *offset = nir_load_push_constant(&b, is_3d ? 3 : 2, 32, nir_imm_int(&b, 0), .range = is_3d ? 12 : 8);
            nir_def *img_coord = nir_iadd(&b, global_id, offset);
   nir_def *outval =
            nir_def *pos_x = nir_channel(&b, global_id, 0);
            nir_def *tmp = nir_imul(&b, pos_y, stride);
                     nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, coord, nir_undef(&b, 1, 32), outval,
               }
      /* Image to buffer - don't write use image accessors */
   static VkResult
   radv_device_init_meta_itob_state(struct radv_device *device)
   {
      VkResult result;
   nir_shader *cs = build_nir_itob_compute_shader(device, false);
            /*
   * two descriptors one for the image being sampled
   * one for the buffer being written.
   */
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
   .pSetLayouts = &device->meta_state.itob.img_ds_layout,
   .pushConstantRangeCount = 1,
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
            VkPipelineShaderStageCreateInfo pipeline_shader_stage_3d = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs_3d),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info_3d = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage_3d,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info_3d,
         if (result != VK_SUCCESS)
            ralloc_free(cs_3d);
               fail:
      ralloc_free(cs);
   ralloc_free(cs_3d);
      }
      static void
   radv_device_finish_meta_itob_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->itob.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->itob.img_ds_layout,
         radv_DestroyPipeline(radv_device_to_handle(device), state->itob.pipeline, &state->alloc);
      }
      static nir_shader *
   build_nir_btoi_compute_shader(struct radv_device *dev, bool is_3d)
   {
      enum glsl_sampler_dim dim = is_3d ? GLSL_SAMPLER_DIM_3D : GLSL_SAMPLER_DIM_2D;
   const struct glsl_type *buf_type = glsl_sampler_type(GLSL_SAMPLER_DIM_BUF, false, false, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type = glsl_image_type(dim, false, GLSL_TYPE_FLOAT);
   nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, is_3d ? "meta_btoi_cs_3d" : "meta_btoi_cs");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_uniform, buf_type, "s_tex");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *offset = nir_load_push_constant(&b, is_3d ? 3 : 2, 32, nir_imm_int(&b, 0), .range = is_3d ? 12 : 8);
            nir_def *pos_x = nir_channel(&b, global_id, 0);
            nir_def *buf_coord = nir_imul(&b, pos_y, stride);
            nir_def *coord = nir_iadd(&b, global_id, offset);
            nir_def *img_coord = nir_vec4(&b, nir_channel(&b, coord, 0), nir_channel(&b, coord, 1),
            nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, img_coord, nir_undef(&b, 1, 32), outval,
               }
      /* Buffer to image - don't write use image accessors */
   static VkResult
   radv_device_init_meta_btoi_state(struct radv_device *device)
   {
      VkResult result;
   nir_shader *cs = build_nir_btoi_compute_shader(device, false);
   nir_shader *cs_3d = build_nir_btoi_compute_shader(device, true);
   /*
   * two descriptors one for the image being sampled
   * one for the buffer being written.
   */
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
   .pSetLayouts = &device->meta_state.btoi.img_ds_layout,
   .pushConstantRangeCount = 1,
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
            VkPipelineShaderStageCreateInfo pipeline_shader_stage_3d = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs_3d),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info_3d = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage_3d,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info_3d,
            ralloc_free(cs_3d);
               fail:
      ralloc_free(cs_3d);
   ralloc_free(cs);
      }
      static void
   radv_device_finish_meta_btoi_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->btoi.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->btoi.img_ds_layout,
         radv_DestroyPipeline(radv_device_to_handle(device), state->btoi.pipeline, &state->alloc);
      }
      /* Buffer to image - special path for R32G32B32 */
   static nir_shader *
   build_nir_btoi_r32g32b32_compute_shader(struct radv_device *dev)
   {
      const struct glsl_type *buf_type = glsl_sampler_type(GLSL_SAMPLER_DIM_BUF, false, false, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type = glsl_image_type(GLSL_SAMPLER_DIM_BUF, false, GLSL_TYPE_FLOAT);
   nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_btoi_r32g32b32_cs");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_uniform, buf_type, "s_tex");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *offset = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 0), .range = 8);
   nir_def *pitch = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 8), .range = 12);
            nir_def *pos_x = nir_channel(&b, global_id, 0);
            nir_def *buf_coord = nir_imul(&b, pos_y, stride);
                     nir_def *global_pos = nir_iadd(&b, nir_imul(&b, nir_channel(&b, img_coord, 1), pitch),
                     for (int chan = 0; chan < 3; chan++) {
                        nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, coord, nir_undef(&b, 1, 32),
                  }
      static VkResult
   radv_device_init_meta_btoi_r32g32b32_state(struct radv_device *device)
   {
      VkResult result;
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
   .pSetLayouts = &device->meta_state.btoi_r32g32b32.img_ds_layout,
   .pushConstantRangeCount = 1,
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
         fail:
      ralloc_free(cs);
      }
      static void
   radv_device_finish_meta_btoi_r32g32b32_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->btoi_r32g32b32.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device),
            }
      static nir_shader *
   build_nir_itoi_compute_shader(struct radv_device *dev, bool is_3d, int samples)
   {
      bool is_multisampled = samples > 1;
   enum glsl_sampler_dim dim = is_3d             ? GLSL_SAMPLER_DIM_3D
               const struct glsl_type *buf_type = glsl_sampler_type(dim, false, false, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type = glsl_image_type(dim, false, GLSL_TYPE_FLOAT);
   nir_builder b =
         b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_uniform, buf_type, "s_tex");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *src_offset = nir_load_push_constant(&b, is_3d ? 3 : 2, 32, nir_imm_int(&b, 0), .range = is_3d ? 12 : 8);
            nir_def *src_coord = nir_iadd(&b, global_id, src_offset);
                     nir_def *tex_vals[8];
   if (is_multisampled) {
      for (uint32_t i = 0; i < samples; i++) {
            } else {
                  nir_def *img_coord = nir_vec4(&b, nir_channel(&b, dst_coord, 0), nir_channel(&b, dst_coord, 1),
            for (uint32_t i = 0; i < samples; i++) {
      nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, img_coord, nir_imm_int(&b, i), tex_vals[i],
                  }
      static VkResult
   create_itoi_pipeline(struct radv_device *device, int samples, VkPipeline *pipeline)
   {
      struct radv_meta_state *state = &device->meta_state;
   nir_shader *cs = build_nir_itoi_compute_shader(device, false, samples);
            VkPipelineShaderStageCreateInfo pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage,
   .flags = 0,
               result =
         ralloc_free(cs);
      }
      /* image to image - don't write use image accessors */
   static VkResult
   radv_device_init_meta_itoi_state(struct radv_device *device)
   {
               /*
   * two descriptors one for the image being sampled
   * one for the buffer being written.
   */
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
   .pSetLayouts = &device->meta_state.itoi.img_ds_layout,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < MAX_SAMPLES_LOG2; i++) {
      uint32_t samples = 1 << i;
   result = create_itoi_pipeline(device, samples, &device->meta_state.itoi.pipeline[i]);
   if (result != VK_SUCCESS)
                        VkPipelineShaderStageCreateInfo pipeline_shader_stage_3d = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs_3d),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info_3d = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage_3d,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info_3d,
                     fail:
         }
      static void
   radv_device_finish_meta_itoi_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->itoi.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->itoi.img_ds_layout,
            for (uint32_t i = 0; i < MAX_SAMPLES_LOG2; ++i) {
                     }
      static nir_shader *
   build_nir_itoi_r32g32b32_compute_shader(struct radv_device *dev)
   {
      const struct glsl_type *type = glsl_sampler_type(GLSL_SAMPLER_DIM_BUF, false, false, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type = glsl_image_type(GLSL_SAMPLER_DIM_BUF, false, GLSL_TYPE_FLOAT);
   nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_itoi_r32g32b32_cs");
   b.shader->info.workgroup_size[0] = 8;
   b.shader->info.workgroup_size[1] = 8;
   nir_variable *input_img = nir_variable_create(b.shader, nir_var_uniform, type, "input_img");
   input_img->data.descriptor_set = 0;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "output_img");
   output_img->data.descriptor_set = 0;
                     nir_def *src_offset = nir_load_push_constant(&b, 3, 32, nir_imm_int(&b, 0), .range = 12);
            nir_def *src_stride = nir_channel(&b, src_offset, 2);
            nir_def *src_img_coord = nir_iadd(&b, global_id, src_offset);
            nir_def *src_global_pos = nir_iadd(&b, nir_imul(&b, nir_channel(&b, src_img_coord, 1), src_stride),
            nir_def *dst_global_pos = nir_iadd(&b, nir_imul(&b, nir_channel(&b, dst_img_coord, 1), dst_stride),
            for (int chan = 0; chan < 3; chan++) {
      /* src */
   nir_def *src_local_pos = nir_iadd_imm(&b, src_global_pos, chan);
            /* dst */
                     nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, dst_coord, nir_undef(&b, 1, 32),
                  }
      /* Image to image - special path for R32G32B32 */
   static VkResult
   radv_device_init_meta_itoi_r32g32b32_state(struct radv_device *device)
   {
      VkResult result;
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
   .pSetLayouts = &device->meta_state.itoi_r32g32b32.img_ds_layout,
   .pushConstantRangeCount = 1,
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
         fail:
      ralloc_free(cs);
      }
      static void
   radv_device_finish_meta_itoi_r32g32b32_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->itoi_r32g32b32.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device),
            }
      static nir_shader *
   build_nir_cleari_compute_shader(struct radv_device *dev, bool is_3d, int samples)
   {
      bool is_multisampled = samples > 1;
   enum glsl_sampler_dim dim = is_3d             ? GLSL_SAMPLER_DIM_3D
               const struct glsl_type *img_type = glsl_image_type(dim, false, GLSL_TYPE_FLOAT);
   nir_builder b =
         b.shader->info.workgroup_size[0] = 8;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *clear_val = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
            nir_def *comps[4];
   comps[0] = nir_channel(&b, global_id, 0);
   comps[1] = nir_channel(&b, global_id, 1);
   comps[2] = layer;
   comps[3] = nir_undef(&b, 1, 32);
            for (uint32_t i = 0; i < samples; i++) {
      nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, global_id, nir_imm_int(&b, i), clear_val,
                  }
      static VkResult
   create_cleari_pipeline(struct radv_device *device, int samples, VkPipeline *pipeline)
   {
      nir_shader *cs = build_nir_cleari_compute_shader(device, false, samples);
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
         ralloc_free(cs);
      }
      static VkResult
   radv_device_init_meta_cleari_state(struct radv_device *device)
   {
               /*
   * two descriptors one for the image being sampled
   * one for the buffer being written.
   */
   VkDescriptorSetLayoutCreateInfo ds_create_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                            result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->meta_state.cleari.img_ds_layout,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < MAX_SAMPLES_LOG2; i++) {
      uint32_t samples = 1 << i;
   result = create_cleari_pipeline(device, samples, &device->meta_state.cleari.pipeline[i]);
   if (result != VK_SUCCESS)
                        /* compute shader */
   VkPipelineShaderStageCreateInfo pipeline_shader_stage_3d = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs_3d),
   .pName = "main",
               VkComputePipelineCreateInfo vk_pipeline_info_3d = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = pipeline_shader_stage_3d,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache, &vk_pipeline_info_3d,
                     fail:
         }
      static void
   radv_device_finish_meta_cleari_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->cleari.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->cleari.img_ds_layout,
            for (uint32_t i = 0; i < MAX_SAMPLES_LOG2; ++i) {
                     }
      /* Special path for clearing R32G32B32 images using a compute shader. */
   static nir_shader *
   build_nir_cleari_r32g32b32_compute_shader(struct radv_device *dev)
   {
      const struct glsl_type *img_type = glsl_image_type(GLSL_SAMPLER_DIM_BUF, false, GLSL_TYPE_FLOAT);
   nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_cleari_r32g32b32_cs");
   b.shader->info.workgroup_size[0] = 8;
            nir_variable *output_img = nir_variable_create(b.shader, nir_var_image, img_type, "out_img");
   output_img->data.descriptor_set = 0;
                     nir_def *clear_val = nir_load_push_constant(&b, 3, 32, nir_imm_int(&b, 0), .range = 12);
            nir_def *global_x = nir_channel(&b, global_id, 0);
                     for (unsigned chan = 0; chan < 3; chan++) {
                        nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img)->def, coord, nir_undef(&b, 1, 32),
                  }
      static VkResult
   radv_device_init_meta_cleari_r32g32b32_state(struct radv_device *device)
   {
      VkResult result;
            VkDescriptorSetLayoutCreateInfo ds_create_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                            result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &device->meta_state.cleari_r32g32b32.img_ds_layout,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            /* compute shader */
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
         fail:
      ralloc_free(cs);
      }
      static void
   radv_device_finish_meta_cleari_r32g32b32_state(struct radv_device *device)
   {
               radv_DestroyPipelineLayout(radv_device_to_handle(device), state->cleari_r32g32b32.img_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device),
            }
      void
   radv_device_finish_meta_bufimage_state(struct radv_device *device)
   {
      radv_device_finish_meta_itob_state(device);
   radv_device_finish_meta_btoi_state(device);
   radv_device_finish_meta_btoi_r32g32b32_state(device);
   radv_device_finish_meta_itoi_state(device);
   radv_device_finish_meta_itoi_r32g32b32_state(device);
   radv_device_finish_meta_cleari_state(device);
      }
      VkResult
   radv_device_init_meta_bufimage_state(struct radv_device *device)
   {
               result = radv_device_init_meta_itob_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_btoi_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_btoi_r32g32b32_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_itoi_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_itoi_r32g32b32_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_cleari_state(device);
   if (result != VK_SUCCESS)
            result = radv_device_init_meta_cleari_r32g32b32_state(device);
   if (result != VK_SUCCESS)
               }
      static void
   create_iview(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *surf, struct radv_image_view *iview,
         {
      if (format == VK_FORMAT_UNDEFINED)
            radv_image_view_init(iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(surf->image),
   .viewType = radv_meta_get_view_type(surf->image),
   .format = format,
   .subresourceRange = {.aspectMask = aspects,
                        },
   }
      static void
   create_bview(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer *buffer, unsigned offset, VkFormat format,
         {
      radv_buffer_view_init(bview, cmd_buffer->device,
                        &(VkBufferViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .flags = 0,
      }
      static void
   create_buffer_from_image(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *surf,
         {
      struct radv_device *device = cmd_buffer->device;
                     radv_create_buffer(device,
                     &(VkBufferCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
   .pNext =
      &(VkBufferUsageFlags2CreateInfoKHR){
      .sType = VK_STRUCTURE_TYPE_BUFFER_USAGE_FLAGS_2_CREATE_INFO_KHR,
         .flags = 0,
         radv_BindBufferMemory2(radv_device_to_handle(device), 1,
                        (VkBindBufferMemoryInfo[]){{
                  }
      static void
   create_bview_for_r32g32b32(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer *buffer, unsigned offset,
         {
               switch (src_format) {
   case VK_FORMAT_R32G32B32_UINT:
      format = VK_FORMAT_R32_UINT;
      case VK_FORMAT_R32G32B32_SINT:
      format = VK_FORMAT_R32_SINT;
      case VK_FORMAT_R32G32B32_SFLOAT:
      format = VK_FORMAT_R32_SFLOAT;
      default:
                  radv_buffer_view_init(bview, cmd_buffer->device,
                        &(VkBufferViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .flags = 0,
      }
      /* GFX9+ has an issue where the HW does not calculate mipmap degradations
   * for block-compressed images correctly (see the comment in
   * radv_image_view_init). Some texels are unaddressable and cannot be copied
   * to/from by a compute shader. Here we will perform a buffer copy to copy the
   * texels that the hardware missed.
   *
   * GFX10 will not use this workaround because it can be fixed by adjusting its
   * image view descriptors instead.
   */
   static void
   fixup_gfx9_cs_copy(struct radv_cmd_buffer *cmd_buffer, const struct radv_meta_blit2d_buffer *buf_bsurf,
               {
      const unsigned mip_level = img_bsurf->level;
   const struct radv_image *image = img_bsurf->image;
   const struct radeon_surf *surf = &image->planes[0].surface;
   struct radv_device *device = cmd_buffer->device;
   const struct radeon_info *rad_info = &device->physical_device->rad_info;
   struct ac_addrlib *addrlib = device->ws->get_addrlib(device->ws);
            /* GFX10 will use a different workaround unless this is not a 2D image */
   if (rad_info->gfx_level < GFX9 || (rad_info->gfx_level >= GFX10 && image->vk.image_type == VK_IMAGE_TYPE_2D) ||
      image->vk.mip_levels == 1 || !vk_format_is_block_compressed(image->vk.format))
         /* The physical extent of the base mip */
            /* The hardware-calculated extent of the selected mip
   * (naive divide-by-two integer math)
   */
   VkExtent2D hw_mip_extent = {radv_minify(hw_base_extent.width, mip_level),
            /* The actual extent we want to copy */
                     if (hw_mip_extent.width >= mip_offset.x + mip_extent.width &&
      hw_mip_extent.height >= mip_offset.y + mip_extent.height)
         if (!to_image) {
      /* If we are writing to a buffer, then we need to wait for the compute
   * shader to finish because it may write over the unaddressable texels
   * while we're fixing them. If we're writing to an image, we do not need
   * to wait because the compute shader cannot write to those texels
   */
               for (uint32_t y = 0; y < mip_extent.height; y++) {
      uint32_t coordY = y + mip_offset.y;
   /* If the default copy algorithm (done previously) has already seen this
   * scanline, then we can bias the starting X coordinate over to skip the
   * region already copied by the default copy.
   */
   uint32_t x = (coordY < hw_mip_extent.height) ? hw_mip_extent.width : 0;
   for (; x < mip_extent.width; x++) {
      uint32_t coordX = x + mip_offset.x;
   uint64_t addr = ac_surface_addr_from_coord(addrlib, rad_info, surf, &surf_info, mip_level, coordX, coordY,
         struct radeon_winsys_bo *img_bo = image->bindings[0].bo;
   struct radeon_winsys_bo *mem_bo = buf_bsurf->buffer->bo;
   const uint64_t img_offset = image->bindings[0].offset + addr;
   /* buf_bsurf->offset already includes the layer offset */
   const uint64_t mem_offset =
         if (to_image) {
         } else {
                  }
      static unsigned
   get_image_stride_for_r32g32b32(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *surf)
   {
               if (cmd_buffer->device->physical_device->rad_info.gfx_level >= GFX9) {
         } else {
                     }
      static void
   itob_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_image_view *src, struct radv_buffer_view *dst)
   {
               radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.itob.img_p_layout, 0, /* set */
   2,                                                                                   /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .pImageInfo =
      (VkDescriptorImageInfo[]){
      {
      .sampler = VK_NULL_HANDLE,
   .imageView = radv_image_view_to_handle(src),
            {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      }
      void
   radv_meta_image_to_buffer(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src,
         {
      VkPipeline pipeline = cmd_buffer->device->meta_state.itob.pipeline;
   struct radv_device *device = cmd_buffer->device;
   struct radv_image_view src_view;
            create_iview(cmd_buffer, src, &src_view, VK_FORMAT_UNDEFINED, src->aspect_mask);
   create_bview(cmd_buffer, dst->buffer, dst->offset, dst->format, &dst_view);
            if (src->image->vk.image_type == VK_IMAGE_TYPE_3D)
                     for (unsigned r = 0; r < num_rects; ++r) {
      unsigned push_constants[4] = {rects[r].src_x, rects[r].src_y, src->layer, dst->pitch};
   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.itob.img_p_layout,
            radv_unaligned_dispatch(cmd_buffer, rects[r].width, rects[r].height, 1);
               radv_image_view_finish(&src_view);
      }
      static void
   btoi_r32g32b32_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer_view *src,
         {
               radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.btoi_r32g32b32.img_p_layout, 0, /* set */
   2, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{
                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      },
   {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      }
      static void
   radv_meta_buffer_to_image_cs_r32g32b32(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_buffer *src,
               {
      VkPipeline pipeline = cmd_buffer->device->meta_state.btoi_r32g32b32.pipeline;
   struct radv_device *device = cmd_buffer->device;
   struct radv_buffer_view src_view, dst_view;
   unsigned dst_offset = 0;
   unsigned stride;
            /* This special btoi path for R32G32B32 formats will write the linear
   * image as a buffer with the same underlying memory. The compute
   * shader will copy all components separately using a R32 format.
   */
            create_bview(cmd_buffer, src->buffer, src->offset, src->format, &src_view);
   create_bview_for_r32g32b32(cmd_buffer, radv_buffer_from_handle(buffer), dst_offset, dst->format, &dst_view);
                              for (unsigned r = 0; r < num_rects; ++r) {
      unsigned push_constants[4] = {
      rects[r].dst_x,
   rects[r].dst_y,
   stride,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.btoi_r32g32b32.img_p_layout,
                        radv_buffer_view_finish(&src_view);
   radv_buffer_view_finish(&dst_view);
      }
      static void
   btoi_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer_view *src, struct radv_image_view *dst)
   {
               radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.btoi.img_p_layout, 0, /* set */
   2,                                                                                   /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{
                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      },
   {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .pImageInfo = (VkDescriptorImageInfo[]){
         }
      void
   radv_meta_buffer_to_image_cs(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_buffer *src,
         {
      VkPipeline pipeline = cmd_buffer->device->meta_state.btoi.pipeline;
   struct radv_device *device = cmd_buffer->device;
   struct radv_buffer_view src_view;
            if (dst->image->vk.format == VK_FORMAT_R32G32B32_UINT || dst->image->vk.format == VK_FORMAT_R32G32B32_SINT ||
      dst->image->vk.format == VK_FORMAT_R32G32B32_SFLOAT) {
   radv_meta_buffer_to_image_cs_r32g32b32(cmd_buffer, src, dst, num_rects, rects);
               create_bview(cmd_buffer, src->buffer, src->offset, src->format, &src_view);
   create_iview(cmd_buffer, dst, &dst_view, VK_FORMAT_UNDEFINED, dst->aspect_mask);
            if (dst->image->vk.image_type == VK_IMAGE_TYPE_3D)
                  for (unsigned r = 0; r < num_rects; ++r) {
      unsigned push_constants[4] = {
      rects[r].dst_x,
   rects[r].dst_y,
   dst->layer,
      };
   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.btoi.img_p_layout,
            radv_unaligned_dispatch(cmd_buffer, rects[r].width, rects[r].height, 1);
               radv_image_view_finish(&dst_view);
      }
      static void
   itoi_r32g32b32_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer_view *src,
         {
               radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.itoi_r32g32b32.img_p_layout, 0, /* set */
   2, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{
                              .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
      },
   {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
      }
      static void
   radv_meta_image_to_image_cs_r32g32b32(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src,
               {
      VkPipeline pipeline = cmd_buffer->device->meta_state.itoi_r32g32b32.pipeline;
   struct radv_device *device = cmd_buffer->device;
   struct radv_buffer_view src_view, dst_view;
   unsigned src_offset = 0, dst_offset = 0;
   unsigned src_stride, dst_stride;
            /* 96-bit formats are only compatible to themselves. */
   assert(dst->format == VK_FORMAT_R32G32B32_UINT || dst->format == VK_FORMAT_R32G32B32_SINT ||
            /* This special itoi path for R32G32B32 formats will write the linear
   * image as a buffer with the same underlying memory. The compute
   * shader will copy all components separately using a R32 format.
   */
   create_buffer_from_image(cmd_buffer, src, VK_BUFFER_USAGE_2_UNIFORM_TEXEL_BUFFER_BIT_KHR, &src_buffer);
            create_bview_for_r32g32b32(cmd_buffer, radv_buffer_from_handle(src_buffer), src_offset, src->format, &src_view);
   create_bview_for_r32g32b32(cmd_buffer, radv_buffer_from_handle(dst_buffer), dst_offset, dst->format, &dst_view);
                     src_stride = get_image_stride_for_r32g32b32(cmd_buffer, src);
            for (unsigned r = 0; r < num_rects; ++r) {
      unsigned push_constants[6] = {
         };
   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.itoi_r32g32b32.img_p_layout,
                        radv_buffer_view_finish(&src_view);
   radv_buffer_view_finish(&dst_view);
   radv_DestroyBuffer(radv_device_to_handle(device), src_buffer, NULL);
      }
      static void
   itoi_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_image_view *src, struct radv_image_view *dst)
   {
               radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.itoi.img_p_layout,
                                 0, /* set */
   2, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .pImageInfo =
      (VkDescriptorImageInfo[]){
      {
      .sampler = VK_NULL_HANDLE,
   .imageView = radv_image_view_to_handle(src),
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
      }
      void
   radv_meta_image_to_image_cs(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *src,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_image_view src_view, dst_view;
   uint32_t samples = src->image->vk.samples;
            if (src->format == VK_FORMAT_R32G32B32_UINT || src->format == VK_FORMAT_R32G32B32_SINT ||
      src->format == VK_FORMAT_R32G32B32_SFLOAT) {
   radv_meta_image_to_image_cs_r32g32b32(cmd_buffer, src, dst, num_rects, rects);
               u_foreach_bit (i, dst->aspect_mask) {
      unsigned aspect_mask = 1u << i;
   VkFormat depth_format = 0;
   if (aspect_mask == VK_IMAGE_ASPECT_STENCIL_BIT)
         else if (aspect_mask == VK_IMAGE_ASPECT_DEPTH_BIT)
            create_iview(cmd_buffer, src, &src_view, depth_format, aspect_mask);
                     VkPipeline pipeline = cmd_buffer->device->meta_state.itoi.pipeline[samples_log2];
   if (src->image->vk.image_type == VK_IMAGE_TYPE_3D || dst->image->vk.image_type == VK_IMAGE_TYPE_3D)
                  for (unsigned r = 0; r < num_rects; ++r) {
      unsigned push_constants[6] = {
         };
                              radv_image_view_finish(&src_view);
         }
      static void
   cleari_r32g32b32_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer_view *view)
   {
               radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                                 device->meta_state.cleari_r32g32b32.img_p_layout, 0, /* set */
   1,                                                   /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   }
      static void
   radv_meta_clear_image_cs_r32g32b32(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *dst,
         {
      VkPipeline pipeline = cmd_buffer->device->meta_state.cleari_r32g32b32.pipeline;
   struct radv_device *device = cmd_buffer->device;
   struct radv_buffer_view dst_view;
   unsigned stride;
            /* This special clear path for R32G32B32 formats will write the linear
   * image as a buffer with the same underlying memory. The compute
   * shader will clear all components separately using a R32 format.
   */
            create_bview_for_r32g32b32(cmd_buffer, radv_buffer_from_handle(buffer), 0, dst->format, &dst_view);
                              unsigned push_constants[4] = {
      clear_color->uint32[0],
   clear_color->uint32[1],
   clear_color->uint32[2],
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.cleari_r32g32b32.img_p_layout,
                     radv_buffer_view_finish(&dst_view);
      }
      static void
   cleari_bind_descriptors(struct radv_cmd_buffer *cmd_buffer, struct radv_image_view *dst_iview)
   {
               radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.cleari.img_p_layout,
                                 0, /* set */
   1, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){
      {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .pImageInfo =
      (VkDescriptorImageInfo[]){
      }
      void
   radv_meta_clear_image_cs(struct radv_cmd_buffer *cmd_buffer, struct radv_meta_blit2d_surf *dst,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_image_view dst_iview;
   uint32_t samples = dst->image->vk.samples;
            if (dst->format == VK_FORMAT_R32G32B32_UINT || dst->format == VK_FORMAT_R32G32B32_SINT ||
      dst->format == VK_FORMAT_R32G32B32_SFLOAT) {
   radv_meta_clear_image_cs_r32g32b32(cmd_buffer, dst, clear_color);
               create_iview(cmd_buffer, dst, &dst_iview, VK_FORMAT_UNDEFINED, dst->aspect_mask);
            VkPipeline pipeline = cmd_buffer->device->meta_state.cleari.pipeline[samples_log2];
   if (dst->image->vk.image_type == VK_IMAGE_TYPE_3D)
                     unsigned push_constants[5] = {
                  radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.cleari.img_p_layout,
                        }
