   /*
   * Copyright © 2021 Valve Corporation
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
   #include "vk_format.h"
      void
   radv_device_finish_meta_copy_vrs_htile_state(struct radv_device *device)
   {
               radv_DestroyPipeline(radv_device_to_handle(device), state->copy_vrs_htile_pipeline, &state->alloc);
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->copy_vrs_htile_p_layout, &state->alloc);
   device->vk.dispatch_table.DestroyDescriptorSetLayout(radv_device_to_handle(device), state->copy_vrs_htile_ds_layout,
      }
      static nir_shader *
   build_copy_vrs_htile_shader(struct radv_device *device, struct radeon_surf *surf)
   {
      nir_builder b = radv_meta_init_shader(device, MESA_SHADER_COMPUTE, "meta_copy_vrs_htile");
   b.shader->info.workgroup_size[0] = 8;
            /* Get coordinates. */
                     /* Multiply the coordinates by the HTILE block size. */
            /* Load constants. */
   nir_def *constants = nir_load_push_constant(&b, 3, 32, nir_imm_int(&b, 8), .range = 20);
   nir_def *htile_pitch = nir_channel(&b, constants, 0);
   nir_def *htile_slice_size = nir_channel(&b, constants, 1);
            /* Get the HTILE addr from coordinates. */
   nir_def *zero = nir_imm_int(&b, 0);
   nir_def *htile_addr =
      ac_nir_htile_addr_from_coord(&b, &device->physical_device->rad_info, &surf->u.gfx9.zs.htile_equation, htile_pitch,
         /* Set up the input VRS image descriptor. */
   const struct glsl_type *vrs_sampler_type = glsl_sampler_type(GLSL_SAMPLER_DIM_2D, false, false, GLSL_TYPE_FLOAT);
   nir_variable *input_vrs_img = nir_variable_create(b.shader, nir_var_uniform, vrs_sampler_type, "input_vrs_image");
   input_vrs_img->data.descriptor_set = 0;
            /* Load the VRS rates from the 2D image. */
            /* Extract the X/Y rates and clamp them because the maximum supported VRS rate is 2x2 (1x1 in
   * hardware).
   *
   * VRS rate X = min(value >> 2, 1)
   * VRS rate Y = min(value & 3, 1)
   */
   nir_def *x_rate = nir_ushr_imm(&b, nir_channel(&b, value, 0), 2);
            nir_def *y_rate = nir_iand_imm(&b, nir_channel(&b, value, 0), 3);
            /* Compute the final VRS rate. */
            /* Load the HTILE buffer descriptor. */
            /* Load the HTILE value if requested, otherwise use the default value. */
            nir_push_if(&b, nir_ieq_imm(&b, read_htile_value, 1));
   {
      /* Load the existing HTILE 32-bit value for this 8x8 pixels area. */
            /* Clear the 4-bit VRS rates. */
      }
   nir_push_else(&b, NULL);
   {
         }
            /* Set the VRS rates loaded from the image. */
            /* Store the updated HTILE 32-bit which contains the VRS rates. */
               }
      static VkResult
   radv_device_init_meta_copy_vrs_htile_state(struct radv_device *device, struct radeon_surf *surf)
   {
      struct radv_meta_state *state = &device->meta_state;
   nir_shader *cs = build_copy_vrs_htile_shader(device, surf);
            VkDescriptorSetLayoutCreateInfo ds_layout_info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 2,
   .pBindings = (VkDescriptorSetLayoutBinding[]){
      {.binding = 0,
               result = radv_CreateDescriptorSetLayout(radv_device_to_handle(device), &ds_layout_info, &state->alloc,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo p_layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &state->copy_vrs_htile_ds_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
      &(VkPushConstantRange){
      VK_SHADER_STAGE_COMPUTE_BIT,
   0,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &p_layout_info, &state->alloc,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs),
   .pName = "main",
               VkComputePipelineCreateInfo pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), state->cache, &pipeline_info, NULL,
      fail:
      ralloc_free(cs);
      }
      void
   radv_copy_vrs_htile(struct radv_cmd_buffer *cmd_buffer, struct radv_image *vrs_image, const VkRect2D *rect,
         {
      struct radv_device *device = cmd_buffer->device;
   struct radv_meta_state *state = &device->meta_state;
   struct radv_meta_saved_state saved_state;
                     if (!cmd_buffer->device->meta_state.copy_vrs_htile_pipeline) {
      VkResult ret = radv_device_init_meta_copy_vrs_htile_state(cmd_buffer->device, &dst_image->planes[0].surface);
   if (ret != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmd_buffer->vk, ret);
                  cmd_buffer->state.flush_bits |=
      radv_src_access_flush(cmd_buffer, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, NULL) |
         radv_meta_save(&saved_state, cmd_buffer,
            radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
            radv_image_view_init(&vrs_iview, cmd_buffer->device,
                        &(VkImageViewCreateInfo){
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = radv_image_to_handle(vrs_image),
   .viewType = VK_IMAGE_VIEW_TYPE_2D,
   .format = vrs_image->vk.format,
   .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                  radv_meta_push_descriptor_set(
      cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, state->copy_vrs_htile_p_layout, 0, /* set */
   2,                                                                             /* descriptorWriteCount */
   (VkWriteDescriptorSet[]){{.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                           .dstBinding = 0,
   .dstArrayElement = 0,
   .descriptorCount = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .pImageInfo =
      (VkDescriptorImageInfo[]){
      {
      .sampler = VK_NULL_HANDLE,
   .imageView = radv_image_view_to_handle(&vrs_iview),
            {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
   .dstBinding = 1,
   .dstArrayElement = 0,
         const unsigned constants[5] = {
      rect->offset.x,
   rect->offset.y,
   dst_image->planes[0].surface.meta_pitch,
   dst_image->planes[0].surface.meta_slice_size,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), state->copy_vrs_htile_p_layout,
            uint32_t width = DIV_ROUND_UP(rect->extent.width, 8);
                                       cmd_buffer->state.flush_bits |= RADV_CMD_FLAG_CS_PARTIAL_FLUSH | RADV_CMD_FLAG_INV_VCACHE |
      }
