   /*
   * Copyright © 2021 Google
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
      #define AC_SURFACE_INCLUDE_NIR
   #include "ac_surface.h"
      #include "radv_meta.h"
   #include "radv_private.h"
      static nir_shader *
   build_dcc_retile_compute_shader(struct radv_device *dev, struct radeon_surf *surf)
   {
      enum glsl_sampler_dim dim = GLSL_SAMPLER_DIM_BUF;
   const struct glsl_type *buf_type = glsl_image_type(dim, false, GLSL_TYPE_UINT);
            b.shader->info.workgroup_size[0] = 8;
            nir_def *src_dcc_size = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 0), .range = 8);
   nir_def *src_dcc_pitch = nir_channels(&b, src_dcc_size, 1);
            nir_def *dst_dcc_size = nir_load_push_constant(&b, 2, 32, nir_imm_int(&b, 8), .range = 8);
   nir_def *dst_dcc_pitch = nir_channels(&b, dst_dcc_size, 1);
   nir_def *dst_dcc_height = nir_channels(&b, dst_dcc_size, 2);
   nir_variable *input_dcc = nir_variable_create(b.shader, nir_var_uniform, buf_type, "dcc_in");
   input_dcc->data.descriptor_set = 0;
   input_dcc->data.binding = 0;
   nir_variable *output_dcc = nir_variable_create(b.shader, nir_var_uniform, buf_type, "dcc_out");
   output_dcc->data.descriptor_set = 0;
            nir_def *input_dcc_ref = &nir_build_deref_var(&b, input_dcc)->def;
            nir_def *coord = get_global_ids(&b, 2);
   nir_def *zero = nir_imm_int(&b, 0);
   coord =
            nir_def *src = ac_nir_dcc_addr_from_coord(&b, &dev->physical_device->rad_info, surf->bpe,
               nir_def *dst = ac_nir_dcc_addr_from_coord(
      &b, &dev->physical_device->rad_info, surf->bpe, &surf->u.gfx9.color.display_dcc_equation, dst_dcc_pitch,
         nir_def *dcc_val = nir_image_deref_load(&b, 1, 32, input_dcc_ref, nir_vec4(&b, src, src, src, src),
            nir_image_deref_store(&b, output_dcc_ref, nir_vec4(&b, dst, dst, dst, dst), nir_undef(&b, 1, 32), dcc_val,
               }
      void
   radv_device_finish_meta_dcc_retile_state(struct radv_device *device)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(state->dcc_retile.pipeline); i++) {
         }
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->dcc_retile.p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->dcc_retile.ds_layout,
            /* Reset for next finish. */
      }
      /*
   * This take a surface, but the only things used are:
   * - BPE
   * - DCC equations
   * - DCC block size
   *
   * BPE is always 4 at the moment and the rest is derived from the tilemode.
   */
   static VkResult
   radv_device_init_meta_dcc_retile_state(struct radv_device *device, struct radeon_surf *surf)
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
   .pSetLayouts = &device->meta_state.dcc_retile.ds_layout,
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
         cleanup:
      ralloc_free(cs);
      }
      void
   radv_retile_dcc(struct radv_cmd_buffer *cmd_buffer, struct radv_image *image)
   {
      struct radv_meta_saved_state saved_state;
   struct radv_device *device = cmd_buffer->device;
            assert(image->vk.image_type == VK_IMAGE_TYPE_2D);
                     state->flush_bits |= radv_dst_access_flush(cmd_buffer, VK_ACCESS_2_SHADER_READ_BIT, image) |
                     /* Compile pipelines if not already done so. */
   if (!cmd_buffer->device->meta_state.dcc_retile.pipeline[swizzle_mode]) {
      VkResult ret = radv_device_init_meta_dcc_retile_state(cmd_buffer->device, &image->planes[0].surface);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  radv_meta_save(&saved_state, cmd_buffer,
            radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                     struct radv_buffer_view views[2];
   VkBufferView view_handles[2];
   radv_buffer_view_init(views, cmd_buffer->device,
                        &(VkBufferViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .buffer = radv_buffer_to_handle(&buffer),
   radv_buffer_view_init(views + 1, cmd_buffer->device,
                        &(VkBufferViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
   .buffer = radv_buffer_to_handle(&buffer),
   for (unsigned i = 0; i < 2; ++i)
            radv_meta_push_descriptor_set(cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, device->meta_state.dcc_retile.p_layout,
                                 0, /* set */
   2, /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){
      {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
      },
   {
      .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            unsigned width = DIV_ROUND_UP(image->vk.extent.width, vk_format_get_blockwidth(image->vk.format));
            unsigned dcc_width = DIV_ROUND_UP(width, image->planes[0].surface.u.gfx9.color.dcc_block_width);
            uint32_t constants[] = {
      image->planes[0].surface.u.gfx9.color.dcc_pitch_max + 1,
   image->planes[0].surface.u.gfx9.color.dcc_height,
   image->planes[0].surface.u.gfx9.color.display_dcc_pitch_max + 1,
      };
   radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.dcc_retile.p_layout,
                     radv_buffer_view_finish(views);
   radv_buffer_view_finish(views + 1);
                     state->flush_bits |=
      }
