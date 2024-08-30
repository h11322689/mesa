   /*
   * Copyright 2023 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vk_texcompress_etc2.h"
      #include "compiler/nir/nir_builder.h"
   #include "vk_shader_module.h"
      /* Based on
   * https://github.com/Themaister/Granite/blob/master/assets/shaders/decode/etc2.comp
   * https://github.com/Themaister/Granite/blob/master/assets/shaders/decode/eac.comp
   *
   * With some differences:
   *  - Use the vk format to do all the settings.
   *  - Combine the ETC2 and EAC shaders.
   *  - Since we combined the above, reuse the function for the ETC2 A8 component.
   *  - the EAC shader doesn't do SNORM correctly, so this has that fixed.
   */
      static nir_def *
   flip_endian(nir_builder *b, nir_def *src, unsigned cnt)
   {
      nir_def *v[2];
   for (unsigned i = 0; i < cnt; ++i) {
      nir_def *intermediate[4];
   nir_def *chan = cnt == 1 ? src : nir_channel(b, src, i);
   for (unsigned j = 0; j < 4; ++j)
         v[i] = nir_ior(b, nir_ior(b, nir_ishl_imm(b, intermediate[0], 24), nir_ishl_imm(b, intermediate[1], 16)),
      }
      }
      static nir_def *
   etc1_color_modifier_lookup(nir_builder *b, nir_def *x, nir_def *y)
   {
      const unsigned table[8][2] = {{2, 8}, {5, 17}, {9, 29}, {13, 42}, {18, 60}, {24, 80}, {33, 106}, {47, 183}};
   nir_def *upper = nir_ieq_imm(b, y, 1);
   nir_def *result = NULL;
   for (unsigned i = 0; i < 8; ++i) {
      nir_def *tmp = nir_bcsel(b, upper, nir_imm_int(b, table[i][1]), nir_imm_int(b, table[i][0]));
   if (result)
         else
      }
      }
      static nir_def *
   etc2_distance_lookup(nir_builder *b, nir_def *x)
   {
      const unsigned table[8] = {3, 6, 11, 16, 23, 32, 41, 64};
   nir_def *result = NULL;
   for (unsigned i = 0; i < 8; ++i) {
      if (result)
         else
      }
      }
      static nir_def *
   etc1_alpha_modifier_lookup(nir_builder *b, nir_def *x, nir_def *y)
   {
      const unsigned table[16] = {0xe852, 0xc962, 0xc741, 0xc531, 0xb752, 0xa862, 0xa763, 0xa742,
         nir_def *result = NULL;
   for (unsigned i = 0; i < 16; ++i) {
      nir_def *tmp = nir_imm_int(b, table[i]);
   if (result)
         else
      }
      }
      static nir_def *
   etc_extend(nir_builder *b, nir_def *v, int bits)
   {
      if (bits == 4)
            }
      static nir_def *
   decode_etc2_alpha(struct nir_builder *b, nir_def *alpha_payload, nir_def *linear_pixel, bool eac, nir_def *is_signed)
   {
      alpha_payload = flip_endian(b, alpha_payload, 2);
   nir_def *alpha_x = nir_channel(b, alpha_payload, 1);
   nir_def *alpha_y = nir_channel(b, alpha_payload, 0);
   nir_def *bit_offset = nir_isub_imm(b, 45, nir_imul_imm(b, linear_pixel, 3));
   nir_def *base = nir_ubfe_imm(b, alpha_y, 24, 8);
   nir_def *multiplier = nir_ubfe_imm(b, alpha_y, 20, 4);
            if (eac) {
      nir_def *signed_base = nir_ibfe_imm(b, alpha_y, 24, 8);
   signed_base = nir_imul_imm(b, signed_base, 8);
   base = nir_iadd_imm(b, nir_imul_imm(b, base, 8), 4);
   base = nir_bcsel(b, is_signed, signed_base, base);
               nir_def *lsb_index = nir_ubfe(b, nir_bcsel(b, nir_uge_imm(b, bit_offset, 32), alpha_y, alpha_x),
         bit_offset = nir_iadd_imm(b, bit_offset, 2);
   nir_def *msb = nir_ubfe(b, nir_bcsel(b, nir_uge_imm(b, bit_offset, 32), alpha_y, alpha_x),
         nir_def *mod = nir_ixor(b, etc1_alpha_modifier_lookup(b, table, lsb_index), nir_iadd_imm(b, msb, -1));
            nir_def *low_bound = nir_imm_int(b, 0);
   nir_def *high_bound = nir_imm_int(b, 255);
   nir_def *final_mult = nir_imm_float(b, 1 / 255.0);
   if (eac) {
      low_bound = nir_bcsel(b, is_signed, nir_imm_int(b, -1023), low_bound);
   high_bound = nir_bcsel(b, is_signed, nir_imm_int(b, 1023), nir_imm_int(b, 2047));
                  }
      static nir_def *
   get_global_ids(nir_builder *b, unsigned num_components)
   {
               nir_def *local_ids = nir_channels(b, nir_load_local_invocation_id(b), mask);
   nir_def *block_ids = nir_channels(b, nir_load_workgroup_id(b), mask);
   nir_def *block_size =
      nir_channels(b,
                        }
      static nir_shader *
   etc2_build_shader(struct vk_device *dev, const struct nir_shader_compiler_options *nir_options)
   {
      const struct glsl_type *sampler_type_2d = glsl_sampler_type(GLSL_SAMPLER_DIM_2D, false, true, GLSL_TYPE_UINT);
   const struct glsl_type *sampler_type_3d = glsl_sampler_type(GLSL_SAMPLER_DIM_3D, false, false, GLSL_TYPE_UINT);
   const struct glsl_type *img_type_2d = glsl_image_type(GLSL_SAMPLER_DIM_2D, true, GLSL_TYPE_FLOAT);
   const struct glsl_type *img_type_3d = glsl_image_type(GLSL_SAMPLER_DIM_3D, false, GLSL_TYPE_FLOAT);
   nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_COMPUTE, nir_options, "meta_decode_etc");
   b.shader->info.workgroup_size[0] = 8;
            nir_variable *input_img_2d = nir_variable_create(b.shader, nir_var_uniform, sampler_type_2d, "s_tex_2d");
   input_img_2d->data.descriptor_set = 0;
            nir_variable *input_img_3d = nir_variable_create(b.shader, nir_var_uniform, sampler_type_3d, "s_tex_3d");
   input_img_3d->data.descriptor_set = 0;
            nir_variable *output_img_2d = nir_variable_create(b.shader, nir_var_image, img_type_2d, "out_img_2d");
   output_img_2d->data.descriptor_set = 0;
            nir_variable *output_img_3d = nir_variable_create(b.shader, nir_var_image, img_type_3d, "out_img_3d");
   output_img_3d->data.descriptor_set = 0;
                     nir_def *consts = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
   nir_def *consts2 = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 0), .base = 16, .range = 4);
   nir_def *offset = nir_channels(&b, consts, 7);
   nir_def *format = nir_channel(&b, consts, 3);
   nir_def *image_type = nir_channel(&b, consts2, 0);
   nir_def *is_3d = nir_ieq_imm(&b, image_type, VK_IMAGE_TYPE_3D);
   nir_def *coord = nir_iadd(&b, global_id, offset);
   nir_def *src_coord = nir_vec3(&b, nir_ushr_imm(&b, nir_channel(&b, coord, 0), 2),
            nir_variable *payload_var = nir_variable_create(b.shader, nir_var_shader_temp, glsl_vec4_type(), "payload");
   nir_push_if(&b, is_3d);
   {
      nir_def *color = nir_txf_deref(&b, nir_build_deref_var(&b, input_img_3d), src_coord, nir_imm_int(&b, 0));
      }
   nir_push_else(&b, NULL);
   {
      nir_def *color = nir_txf_deref(&b, nir_build_deref_var(&b, input_img_2d), src_coord, nir_imm_int(&b, 0));
      }
            nir_def *pixel_coord = nir_iand_imm(&b, nir_channels(&b, coord, 3), 3);
   nir_def *linear_pixel =
            nir_def *payload = nir_load_var(&b, payload_var);
   nir_variable *color = nir_variable_create(b.shader, nir_var_shader_temp, glsl_vec4_type(), "color");
   nir_store_var(&b, color, nir_imm_vec4(&b, 1.0, 0.0, 0.0, 1.0), 0xf);
   nir_push_if(&b, nir_ilt_imm(&b, format, VK_FORMAT_EAC_R11_UNORM_BLOCK));
   {
      nir_def *alpha_bits_8 = nir_ige_imm(&b, format, VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK);
   nir_def *alpha_bits_1 = nir_iand(&b, nir_ige_imm(&b, format, VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK),
            nir_def *color_payload =
         color_payload = flip_endian(&b, color_payload, 2);
   nir_def *color_y = nir_channel(&b, color_payload, 0);
   nir_def *color_x = nir_channel(&b, color_payload, 1);
   nir_def *flip = nir_test_mask(&b, color_y, 1);
   nir_def *subblock =
            nir_variable *punchthrough = nir_variable_create(b.shader, nir_var_shader_temp, glsl_bool_type(), "punchthrough");
   nir_def *punchthrough_init = nir_iand(&b, alpha_bits_1, nir_inot(&b, nir_test_mask(&b, color_y, 2)));
            nir_variable *etc1_compat = nir_variable_create(b.shader, nir_var_shader_temp, glsl_bool_type(), "etc1_compat");
            nir_variable *alpha_result =
         nir_push_if(&b, alpha_bits_8);
   {
      nir_store_var(&b, alpha_result, decode_etc2_alpha(&b, nir_channels(&b, payload, 3), linear_pixel, false, NULL),
      }
   nir_push_else(&b, NULL);
   {
         }
            const struct glsl_type *uvec3_type = glsl_vector_type(GLSL_TYPE_UINT, 3);
   nir_variable *rgb_result = nir_variable_create(b.shader, nir_var_shader_temp, uvec3_type, "rgb_result");
   nir_variable *base_rgb = nir_variable_create(b.shader, nir_var_shader_temp, uvec3_type, "base_rgb");
            nir_def *msb = nir_iand_imm(&b, nir_ushr(&b, color_x, nir_iadd_imm(&b, linear_pixel, 15)), 2);
            nir_push_if(&b, nir_iand(&b, nir_inot(&b, alpha_bits_1), nir_inot(&b, nir_test_mask(&b, color_y, 2))));
   {
      nir_store_var(&b, etc1_compat, nir_imm_true(&b), 1);
   nir_def *tmp[3];
   for (unsigned i = 0; i < 3; ++i)
      tmp[i] = etc_extend(
      &b,
   nir_iand_imm(&b, nir_ushr(&b, color_y, nir_isub_imm(&b, 28 - 8 * i, nir_imul_imm(&b, subblock, 4))),
            }
   nir_push_else(&b, NULL);
   {
      nir_def *rb = nir_ubfe_imm(&b, color_y, 27, 5);
   nir_def *rd = nir_ibfe_imm(&b, color_y, 24, 3);
   nir_def *gb = nir_ubfe_imm(&b, color_y, 19, 5);
   nir_def *gd = nir_ibfe_imm(&b, color_y, 16, 3);
   nir_def *bb = nir_ubfe_imm(&b, color_y, 11, 5);
   nir_def *bd = nir_ibfe_imm(&b, color_y, 8, 3);
   nir_def *r1 = nir_iadd(&b, rb, rd);
                  nir_push_if(&b, nir_ugt_imm(&b, r1, 31));
   {
      nir_def *r0 =
         nir_def *g0 = nir_ubfe_imm(&b, color_y, 20, 4);
   nir_def *b0 = nir_ubfe_imm(&b, color_y, 16, 4);
   nir_def *r2 = nir_ubfe_imm(&b, color_y, 12, 4);
   nir_def *g2 = nir_ubfe_imm(&b, color_y, 8, 4);
   nir_def *b2 = nir_ubfe_imm(&b, color_y, 4, 4);
   nir_def *da =
                        nir_store_var(&b, punchthrough,
               nir_push_if(&b, nir_ieq_imm(&b, index, 0));
   {
         }
                     nir_def *tmp = nir_iadd(&b, etc_extend(&b, nir_vec3(&b, r2, g2, b2), 4),
            }
      }
   nir_push_else(&b, NULL);
   nir_push_if(&b, nir_ugt_imm(&b, g1, 31));
   {
      nir_def *r0 = nir_ubfe_imm(&b, color_y, 27, 4);
   nir_def *g0 = nir_ior(&b, nir_ishl_imm(&b, nir_ubfe_imm(&b, color_y, 24, 3), 1),
         nir_def *b0 =
         nir_def *r2 = nir_ubfe_imm(&b, color_y, 11, 4);
   nir_def *g2 = nir_ubfe_imm(&b, color_y, 7, 4);
   nir_def *b2 = nir_ubfe_imm(&b, color_y, 3, 4);
   nir_def *da = nir_iand_imm(&b, color_y, 4);
   nir_def *db = nir_iand_imm(&b, color_y, 1);
   nir_def *d = nir_iadd(&b, da, nir_imul_imm(&b, db, 2));
   nir_def *d0 = nir_iadd(&b, nir_ishl_imm(&b, r0, 16), nir_iadd(&b, nir_ishl_imm(&b, g0, 8), b0));
   nir_def *d2 = nir_iadd(&b, nir_ishl_imm(&b, r2, 16), nir_iadd(&b, nir_ishl_imm(&b, g2, 8), b2));
   d = nir_bcsel(&b, nir_uge(&b, d0, d2), nir_iadd_imm(&b, d, 1), d);
   nir_def *dist = etc2_distance_lookup(&b, d);
   nir_def *base = nir_bcsel(&b, nir_ine_imm(&b, msb, 0), nir_vec3(&b, r2, g2, b2), nir_vec3(&b, r0, g0, b0));
   base = etc_extend(&b, base, 4);
   base = nir_iadd(&b, base, nir_imul(&b, dist, nir_isub_imm(&b, 1, nir_imul_imm(&b, lsb, 2))));
   nir_store_var(&b, rgb_result, base, 0x7);
   nir_store_var(&b, punchthrough,
            }
   nir_push_else(&b, NULL);
   nir_push_if(&b, nir_ugt_imm(&b, b1, 31));
   {
      nir_def *r0 = nir_ubfe_imm(&b, color_y, 25, 6);
   nir_def *g0 =
         nir_def *b0 = nir_ior(
      &b, nir_ishl_imm(&b, nir_ubfe_imm(&b, color_y, 11, 2), 3),
      nir_def *rh =
         nir_def *rv = nir_ubfe_imm(&b, color_x, 13, 6);
   nir_def *gh = nir_ubfe_imm(&b, color_x, 25, 7);
   nir_def *gv = nir_ubfe_imm(&b, color_x, 6, 7);
                  r0 = etc_extend(&b, r0, 6);
   g0 = etc_extend(&b, g0, 7);
   b0 = etc_extend(&b, b0, 6);
   rh = etc_extend(&b, rh, 6);
   rv = etc_extend(&b, rv, 6);
   gh = etc_extend(&b, gh, 7);
   gv = etc_extend(&b, gv, 7);
                  nir_def *rgb = nir_vec3(&b, r0, g0, b0);
   nir_def *dx = nir_imul(&b, nir_isub(&b, nir_vec3(&b, rh, gh, bh), rgb), nir_channel(&b, pixel_coord, 0));
   nir_def *dy = nir_imul(&b, nir_isub(&b, nir_vec3(&b, rv, gv, bv), rgb), nir_channel(&b, pixel_coord, 1));
   rgb = nir_iadd(&b, rgb, nir_ishr_imm(&b, nir_iadd_imm(&b, nir_iadd(&b, dx, dy), 2), 2));
   nir_store_var(&b, rgb_result, rgb, 0x7);
      }
   nir_push_else(&b, NULL);
   {
      nir_store_var(&b, etc1_compat, nir_imm_true(&b), 1);
   nir_def *subblock_b = nir_ine_imm(&b, subblock, 0);
   nir_def *tmp[] = {
      nir_bcsel(&b, subblock_b, r1, rb),
   nir_bcsel(&b, subblock_b, g1, gb),
      };
      }
   nir_pop_if(&b, NULL);
   nir_pop_if(&b, NULL);
      }
   nir_pop_if(&b, NULL);
   nir_push_if(&b, nir_load_var(&b, etc1_compat));
   {
      nir_def *etc1_table_index =
         nir_def *sgn = nir_isub_imm(&b, 1, msb);
   sgn = nir_bcsel(&b, nir_load_var(&b, punchthrough), nir_imul(&b, sgn, lsb), sgn);
   nir_store_var(&b, punchthrough,
         nir_def *off = nir_imul(&b, etc1_color_modifier_lookup(&b, etc1_table_index, lsb), sgn);
   nir_def *result = nir_iadd(&b, nir_load_var(&b, base_rgb), off);
      }
   nir_pop_if(&b, NULL);
   nir_push_if(&b, nir_load_var(&b, punchthrough));
   {
      nir_store_var(&b, alpha_result, nir_imm_float(&b, 0), 0x1);
      }
   nir_pop_if(&b, NULL);
   nir_def *col[4];
   for (unsigned i = 0; i < 3; ++i)
         col[3] = nir_load_var(&b, alpha_result);
      }
   nir_push_else(&b, NULL);
   { /* EAC */
      nir_def *is_signed = nir_ior(&b, nir_ieq_imm(&b, format, VK_FORMAT_EAC_R11_SNORM_BLOCK),
         nir_def *val[4];
   for (int i = 0; i < 2; ++i) {
         }
   val[2] = nir_imm_float(&b, 0.0);
   val[3] = nir_imm_float(&b, 1.0);
      }
            nir_def *outval = nir_load_var(&b, color);
   nir_def *img_coord = nir_vec4(&b, nir_channel(&b, coord, 0), nir_channel(&b, coord, 1), nir_channel(&b, coord, 2),
            nir_push_if(&b, is_3d);
   {
      nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img_3d)->def, img_coord, nir_undef(&b, 1, 32), outval,
      }
   nir_push_else(&b, NULL);
   {
      nir_image_deref_store(&b, &nir_build_deref_var(&b, output_img_2d)->def, img_coord, nir_undef(&b, 1, 32), outval,
      }
   nir_pop_if(&b, NULL);
      }
      static VkResult
   etc2_init_pipeline(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
                     const VkComputePipelineCreateInfo pipeline_create_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage =
      (VkPipelineShaderStageCreateInfo){
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(cs),
                     return disp->CreateComputePipelines(_device, etc2->pipeline_cache, 1, &pipeline_create_info, etc2->allocator,
      }
      static VkResult
   etc2_init_pipeline_layout(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            const VkPipelineLayoutCreateInfo pipeline_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 1,
   .pSetLayouts = &etc2->ds_layout,
   .pushConstantRangeCount = 1,
   .pPushConstantRanges =
      &(VkPushConstantRange){
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
                  }
      static VkResult
   etc2_init_ds_layout(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            const VkDescriptorSetLayoutCreateInfo ds_layout_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
   .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
   .bindingCount = 2,
   .pBindings =
      (VkDescriptorSetLayoutBinding[]){
      {
      .binding = 0,
   .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
   .descriptorCount = 1,
      },
   {
      .binding = 1,
   .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
   .descriptorCount = 1,
                     }
      void
   vk_texcompress_etc2_init(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
         }
      VkResult
   vk_texcompress_etc2_late_init(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
                        if (!etc2->pipeline) {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            result = etc2_init_ds_layout(device, etc2);
   if (result != VK_SUCCESS)
            result = etc2_init_pipeline_layout(device, etc2);
   if (result != VK_SUCCESS) {
      disp->DestroyDescriptorSetLayout(_device, etc2->ds_layout, etc2->allocator);
               result = etc2_init_pipeline(device, etc2);
   if (result != VK_SUCCESS) {
      disp->DestroyPipelineLayout(_device, etc2->pipeline_layout, etc2->allocator);
   disp->DestroyDescriptorSetLayout(_device, etc2->ds_layout, etc2->allocator);
               out:
      simple_mtx_unlock(&etc2->mutex);
      }
      void
   vk_texcompress_etc2_finish(struct vk_device *device, struct vk_texcompress_etc2_state *etc2)
   {
      const struct vk_device_dispatch_table *disp = &device->dispatch_table;
            if (etc2->pipeline != VK_NULL_HANDLE)
            if (etc2->pipeline_layout != VK_NULL_HANDLE)
         if (etc2->ds_layout != VK_NULL_HANDLE)
               }
