   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_nir.h"
      #include "spirv_to_dxil.h"
   #include "nir_to_dxil.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
   #include "dxil_nir.h"
   #include "vk_nir_convert_ycbcr.h"
      static nir_def *
   dzn_nir_create_bo_desc(nir_builder *b,
                        nir_variable_mode mode,
      {
      struct glsl_struct_field field = {
      .type = mode == nir_var_mem_ubo ?
         glsl_array_type(glsl_uint_type(), 4096, 4) :
      };
   const struct glsl_type *dummy_type =
            nir_variable *var =
         var->data.descriptor_set = desc_set;
   var->data.binding = binding;
            assert(mode == nir_var_mem_ubo || mode == nir_var_mem_ssbo);
   if (mode == nir_var_mem_ubo)
         else
            VkDescriptorType desc_type =
      var->data.mode == nir_var_mem_ubo ?
   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER :
      nir_address_format addr_format = nir_address_format_32bit_index_offset;
   nir_def *index =
      nir_vulkan_resource_index(b,
                           nir_address_format_num_components(addr_format),
         nir_def *desc =
      nir_load_vulkan_descriptor(b,
                              }
      nir_shader *
   dzn_nir_indirect_draw_shader(enum dzn_indirect_draw_type type)
   {
      const char *type_str[] = {
      "draw",
   "draw_count",
   "indexed_draw",
   "indexed_draw_count",
   "draw_triangle_fan",
   "draw_count_triangle_fan",
   "indexed_draw_triangle_fan",
   "indexed_draw_count_triangle_fan",
   "indexed_draw_triangle_fan_prim_restart",
                        bool indexed = type == DZN_INDIRECT_INDEXED_DRAW ||
                  type == DZN_INDIRECT_INDEXED_DRAW_COUNT ||
   type == DZN_INDIRECT_INDEXED_DRAW_TRIANGLE_FAN ||
      bool triangle_fan = type == DZN_INDIRECT_DRAW_TRIANGLE_FAN ||
                     type == DZN_INDIRECT_DRAW_COUNT_TRIANGLE_FAN ||
   type == DZN_INDIRECT_INDEXED_DRAW_TRIANGLE_FAN ||
   bool indirect_count = type == DZN_INDIRECT_DRAW_COUNT ||
                           bool prim_restart = type == DZN_INDIRECT_INDEXED_DRAW_TRIANGLE_FAN_PRIM_RESTART ||
         nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE,
                           nir_def *params_desc =
         nir_def *draw_buf_desc =
         nir_def *exec_buf_desc =
            unsigned params_size;
   if (triangle_fan)
         else
            nir_def *params =
      nir_load_ubo(&b, params_size / 4, 32,
               nir_def *draw_stride = nir_channel(&b, params, 0);
   nir_def *exec_stride =
      triangle_fan ?
   nir_imm_int(&b, sizeof(struct dzn_indirect_triangle_fan_draw_exec_params)) :
      nir_def *index =
            if (indirect_count) {
      nir_def *count_buf_desc =
            nir_def *draw_count =
            nir_push_if(&b, nir_ieq_imm(&b, index, 0));
   nir_store_ssbo(&b, draw_count, exec_buf_desc, nir_imm_int(&b, 0),
                                             /* The first entry contains the indirect count */
   nir_def *exec_offset =
      indirect_count ?
   nir_imul(&b, exec_stride, nir_iadd_imm(&b, index, 1)) : 
         nir_def *draw_info1 =
         nir_def *draw_info2 =
      indexed ?
   nir_load_ssbo(&b, 1, 32, draw_buf_desc,
               nir_def *first_vertex = nir_channel(&b, draw_info1, indexed ? 3 : 2);
   nir_def *base_instance =
            nir_def *exec_vals[8] = {
      first_vertex,
   base_instance,
               if (triangle_fan) {
      /* Patch {vertex,index}_count and first_index */
   nir_def *triangle_count =
         exec_vals[3] = nir_imul_imm(&b, triangle_count, 3);
   exec_vals[4] = nir_channel(&b, draw_info1, 1);
   exec_vals[5] = nir_imm_int(&b, 0);
   exec_vals[6] = first_vertex;
            nir_def *triangle_fan_exec_buf_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 4,
            nir_def *triangle_fan_index_buf_stride = nir_channel(&b, params, 1);
   nir_def *triangle_fan_index_buf_addr_lo =
                  nir_def *triangle_fan_exec_vals[9] = { 0 };
   uint32_t triangle_fan_exec_param_count = 0;
   nir_def *addr_lo_overflow =
         nir_def *triangle_fan_index_buf_addr_hi =
                  triangle_fan_exec_vals[triangle_fan_exec_param_count++] = triangle_fan_index_buf_addr_lo;
            if (prim_restart) {
      triangle_fan_exec_vals[triangle_fan_exec_param_count++] = nir_channel(&b, draw_info1, 2);
   triangle_fan_exec_vals[triangle_fan_exec_param_count++] = nir_channel(&b, draw_info1, 0);
   uint32_t index_count_offset =
         nir_def *exec_buf_start =
      nir_load_ubo(&b, 2, 32,
            nir_def *exec_buf_start_lo =
      nir_iadd(&b, nir_imm_int(&b, index_count_offset),
            addr_lo_overflow = nir_ult(&b, exec_buf_start_lo, nir_channel(&b, exec_buf_start, 0));
   nir_def *exec_buf_start_hi =
      nir_iadd(&b, nir_channel(&b, exec_buf_start, 0),
      triangle_fan_exec_vals[triangle_fan_exec_param_count++] = exec_buf_start_lo;
   triangle_fan_exec_vals[triangle_fan_exec_param_count++] = exec_buf_start_hi;
      } else {
      triangle_fan_exec_vals[triangle_fan_exec_param_count++] =
         triangle_fan_exec_vals[triangle_fan_exec_param_count++] =
      }
   triangle_fan_exec_vals[triangle_fan_exec_param_count++] = nir_imm_int(&b, 1);
            unsigned rewrite_index_exec_params =
      prim_restart ?
   sizeof(struct dzn_indirect_triangle_fan_prim_restart_rewrite_index_exec_params) :
      nir_def *triangle_fan_exec_stride =
         nir_def *triangle_fan_exec_offset =
            for (uint32_t i = 0; i < triangle_fan_exec_param_count; i += 4) {
                     nir_store_ssbo(&b, nir_vec(&b, &triangle_fan_exec_vals[i], comps),
                           nir_def *ibview_vals[] = {
      triangle_fan_index_buf_addr_lo,
   triangle_fan_index_buf_addr_hi,
   triangle_fan_index_buf_stride,
               nir_store_ssbo(&b, nir_vec(&b, ibview_vals, ARRAY_SIZE(ibview_vals)),
                  } else {
      exec_vals[3] = nir_channel(&b, draw_info1, 0);
   exec_vals[4] = nir_channel(&b, draw_info1, 1);
   exec_vals[5] = nir_channel(&b, draw_info1, 2);
   exec_vals[6] = nir_channel(&b, draw_info1, 3);
               nir_store_ssbo(&b, nir_vec(&b, exec_vals, 4),
               nir_store_ssbo(&b, nir_vec(&b, &exec_vals[4], 4),
                  if (indirect_count)
               }
      nir_shader *
   dzn_nir_triangle_fan_prim_restart_rewrite_index_shader(uint8_t old_index_size)
   {
               nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE,
                           nir_def *params_desc =
         nir_def *new_index_buf_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 1,
      nir_def *old_index_buf_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 2,
      nir_def *new_index_count_ptr_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 3,
         nir_def *params =
      nir_load_ubo(&b, sizeof(struct dzn_triangle_fan_prim_restart_rewrite_index_params) / 4, 32,
               nir_def *prim_restart_val =
         nir_variable *old_index_ptr_var =
         nir_def *old_index_ptr = nir_channel(&b, params, 0);
   nir_store_var(&b, old_index_ptr_var, old_index_ptr, 1);
   nir_variable *new_index_ptr_var =
         nir_store_var(&b, new_index_ptr_var, nir_imm_int(&b, 0), 1);
   nir_def *old_index_count = nir_channel(&b, params, 1);
   nir_variable *index0_var =
                  /*
   * Filter out all primitive-restart magic values, and generate a triangle list
   * from the triangle fan definition.
   *
   * Basically:
   *
   * new_index_ptr = 0;
   * index0 = restart_prim_value; // 0xffff or 0xffffffff
   * for (old_index_ptr = firstIndex; old_index_ptr < indexCount;) {
   *    // If we have no starting-point we need at least 3 vertices,
   *    // otherwise we can do with two. If there's not enough vertices
   *    // to form a primitive, we just bail out.
   *    min_indices = index0 == restart_prim_value ? 3 : 2;
   *    if (old_index_ptr + min_indices > firstIndex + indexCount)
   *       break;
   *
   *    if (index0 == restart_prim_value) {
   *       // No starting point, skip all entries until we have a
   *       // non-primitive-restart value
   *       index0 = old_index_buf[old_index_ptr++];
   *       continue;
   *    }
   *
   *    // If at least one index contains the primitive-restart pattern,
         *    if (old_index_buf[old_index_ptr + 1] == restart_prim_value) {
   *       old_index_ptr += 2;
   *       continue;
   *    }
   *    if (old_index_buf[old_index_ptr] == restart_prim_value) {
   *       old_index_ptr++;
   *       continue;
   *    }
   *
   *    // We have a valid primitive, queue it to the new index buffer
   *    new_index_buf[new_index_ptr++] = old_index_buf[old_index_ptr];
   *    new_index_buf[new_index_ptr++] = old_index_buf[old_index_ptr + 1];
   *    new_index_buf[new_index_ptr++] = index0;
   * }
   *
   * expressed in NIR, which admitedly is not super easy to grasp with.
   * TODO: Might be a good thing to use use the CL compiler we have and turn
   * those shaders into CL kernels.
   */
            old_index_ptr = nir_load_var(&b, old_index_ptr_var);
            nir_def *read_index_count =
      nir_bcsel(&b, nir_ieq(&b, index0, prim_restart_val),
      nir_push_if(&b, nir_ult(&b, old_index_count, nir_iadd(&b, old_index_ptr, read_index_count)));
   nir_jump(&b, nir_jump_break);
            nir_def *old_index_offset =
            nir_push_if(&b, nir_ieq(&b, index0, prim_restart_val));
   nir_def *index_val =
      nir_load_ssbo(&b, 1, 32, old_index_buf_desc,
            if (old_index_size == 2) {
   index_val = nir_bcsel(&b, nir_test_mask(&b, old_index_offset, 0x2),
                        nir_store_var(&b, index0_var, index_val, 1);
   nir_store_var(&b, old_index_ptr_var, nir_iadd_imm(&b, old_index_ptr, 1), 1);
   nir_jump(&b, nir_jump_continue);
            nir_def *index12 =
      nir_load_ssbo(&b, 2, 32, old_index_buf_desc,
            if (old_index_size == 2) {
      nir_def *indices[] = {
      nir_iand_imm(&b, nir_channel(&b, index12, 0), 0xffff),
   nir_ushr_imm(&b, nir_channel(&b, index12, 0), 16),
               index12 = nir_bcsel(&b, nir_test_mask(&b, old_index_offset, 0x2),
                     nir_push_if(&b, nir_ieq(&b, nir_channel(&b, index12, 1), prim_restart_val));
   nir_store_var(&b, old_index_ptr_var, nir_iadd_imm(&b, old_index_ptr, 2), 1);
   nir_store_var(&b, index0_var, prim_restart_val, 1);
   nir_jump(&b, nir_jump_continue);
   nir_push_else(&b, NULL);
   nir_store_var(&b, old_index_ptr_var, nir_iadd_imm(&b, old_index_ptr, 1), 1);
   nir_push_if(&b, nir_ieq(&b, nir_channel(&b, index12, 0), prim_restart_val));
   nir_store_var(&b, index0_var, prim_restart_val, 1);
   nir_jump(&b, nir_jump_continue);
   nir_push_else(&b, NULL);
   nir_def *new_indices =
         nir_def *new_index_ptr = nir_load_var(&b, new_index_ptr_var);
   nir_def *new_index_offset = nir_imul_imm(&b, new_index_ptr, sizeof(uint32_t));
   nir_store_ssbo(&b, new_indices, new_index_buf_desc,
               nir_store_var(&b, new_index_ptr_var, nir_iadd_imm(&b, new_index_ptr, 3), 1);
   nir_pop_if(&b, NULL);
   nir_pop_if(&b, NULL);
            nir_store_ssbo(&b, nir_load_var(&b, new_index_ptr_var),
                     }
      nir_shader *
   dzn_nir_triangle_fan_rewrite_index_shader(uint8_t old_index_size)
   {
               nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_COMPUTE,
                           nir_def *params_desc =
         nir_def *new_index_buf_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 1,
         nir_def *old_index_buf_desc = NULL;
   if (old_index_size > 0) {
      old_index_buf_desc =
      dzn_nir_create_bo_desc(&b, nir_var_mem_ssbo, 0, 2,
            nir_def *params =
      nir_load_ubo(&b, sizeof(struct dzn_triangle_fan_rewrite_index_params) / 4, 32,
               nir_def *triangle = nir_channel(&b, nir_load_global_invocation_id(&b, 32), 0);
            if (old_index_size > 0) {
      nir_def *old_first_index = nir_channel(&b, params, 0);
   nir_def *old_index0_offset =
         nir_def *old_index1_offset =
                  nir_def *old_index0 =
      nir_load_ssbo(&b, 1, 32, old_index_buf_desc,
               if (old_index_size == 2) {
   old_index0 = nir_bcsel(&b, nir_test_mask(&b, old_index0_offset, 0x2),
                        nir_def *old_index12 =
      nir_load_ssbo(&b, 2, 32, old_index_buf_desc,
            if (old_index_size == 2) {
      nir_def *indices[] = {
      nir_iand_imm(&b, nir_channel(&b, old_index12, 0), 0xffff),
   nir_ushr_imm(&b, nir_channel(&b, old_index12, 0), 16),
               old_index12 = nir_bcsel(&b, nir_test_mask(&b, old_index1_offset, 0x2),
                     /* TODO: VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT */
   new_indices =
      nir_vec3(&b, nir_channel(&b, old_index12, 0),
   } else {
      new_indices =
      nir_vec3(&b,
                        nir_def *new_index_offset =
            nir_store_ssbo(&b, new_indices, new_index_buf_desc,
                     }
      nir_shader *
   dzn_nir_blit_vs(void)
   {
      nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_VERTEX,
                     nir_def *params_desc =
            nir_variable *out_pos =
      nir_variable_create(b.shader, nir_var_shader_out, glsl_vec4_type(),
      out_pos->data.location = VARYING_SLOT_POS;
            nir_variable *out_coords =
      nir_variable_create(b.shader, nir_var_shader_out, glsl_vec_type(3),
      out_coords->data.location = VARYING_SLOT_TEX0;
            nir_def *vertex = nir_load_vertex_id(&b);
   nir_def *coords_arr[4] = {
      nir_load_ubo(&b, 4, 32, params_desc, nir_imm_int(&b, 0),
         nir_load_ubo(&b, 4, 32, params_desc, nir_imm_int(&b, 16),
         nir_load_ubo(&b, 4, 32, params_desc, nir_imm_int(&b, 32),
         nir_load_ubo(&b, 4, 32, params_desc, nir_imm_int(&b, 48),
      };
   nir_def *coords =
      nir_bcsel(&b, nir_ieq_imm(&b, vertex, 0), coords_arr[0],
            nir_def *pos =
      nir_vec4(&b, nir_channel(&b, coords, 0), nir_channel(&b, coords, 1),
      nir_def *z_coord =
      nir_load_ubo(&b, 1, 32, params_desc, nir_imm_int(&b, 4 * 4 * sizeof(float)),
               nir_store_var(&b, out_pos, pos, 0xf);
   nir_store_var(&b, out_coords, coords, 0x7);
      }
      nir_shader *
   dzn_nir_blit_fs(const struct dzn_nir_blit_info *info)
   {
      bool ms = info->src_samples > 1;
   nir_alu_type nir_out_type =
         uint32_t coord_comps =
      glsl_get_sampler_dim_coordinate_components(info->sampler_dim) +
         nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT,
                     const struct glsl_type *tex_type =
         nir_variable *tex_var =
                  nir_variable *pos_var =
      nir_variable_create(b.shader, nir_var_shader_in,
            pos_var->data.location = VARYING_SLOT_POS;
            nir_variable *coord_var =
      nir_variable_create(b.shader, nir_var_shader_in,
            coord_var->data.location = VARYING_SLOT_TEX0;
   coord_var->data.driver_location = 1;
   nir_def *coord =
            uint32_t out_comps =
         nir_variable *out =
      nir_variable_create(b.shader, nir_var_shader_out,
                              if (info->resolve_mode != dzn_blit_resolve_none) {
               nir_op resolve_op = nir_op_mov;
   switch (resolve_mode) {
   case dzn_blit_resolve_average:
      /* When resolving a float type, we need to calculate the average of all
   * samples. For integer resolve, Vulkan says that one sample should be
   * chosen without telling which. Let's just pick the first one in that
   * case.
   */
   if (info->out_type == GLSL_TYPE_FLOAT)
         else
            case dzn_blit_resolve_min:
      switch (info->out_type) {
   case GLSL_TYPE_FLOAT: resolve_op = nir_op_fmin; break;
   case GLSL_TYPE_INT: resolve_op = nir_op_imin; break;
   case GLSL_TYPE_UINT: resolve_op = nir_op_umin; break;
   }
      case dzn_blit_resolve_max:
      switch (info->out_type) {
   case GLSL_TYPE_FLOAT: resolve_op = nir_op_fmax; break;
   case GLSL_TYPE_INT: resolve_op = nir_op_imax; break;
   case GLSL_TYPE_UINT: resolve_op = nir_op_umax; break;
   }
      case dzn_blit_resolve_none:
   case dzn_blit_resolve_sample_zero:
                  unsigned nsamples = resolve_mode == dzn_blit_resolve_sample_zero ?
         for (unsigned s = 0; s < nsamples; s++) {
               tex->op = nir_texop_txf_ms;
   tex->dest_type = nir_out_type;
   tex->texture_index = 0;
                  tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord,
                                                                        nir_builder_instr_insert(&b, &tex->instr);
               if (resolve_mode == dzn_blit_resolve_average)
      } else {
      nir_tex_instr *tex =
            tex->dest_type = nir_out_type;
   tex->is_array = info->src_is_array;
            if (ms) {
               tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord,
                                                tex->src[3] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
      } else {
      nir_variable *sampler_var =
                                                               tex->src[2] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
               nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(&b, &tex->instr);
                           }
      static nir_def *
   cull_face(nir_builder *b, nir_variable *vertices, bool ccw)
   {
      nir_def *v0 =
         nir_def *v1 =
         nir_def *v2 =
            nir_def *dir = nir_fdot(b, nir_cross4(b, nir_fsub(b, v1, v0),
               if (ccw)
         else
      }
      static void
   copy_vars(nir_builder *b, nir_deref_instr *dst, nir_deref_instr *src)
   {
      assert(glsl_get_bare_type(dst->type) == glsl_get_bare_type(src->type));
   if (glsl_type_is_struct(dst->type)) {
      for (unsigned i = 0; i < glsl_get_length(dst->type); ++i) {
            } else if (glsl_type_is_array_or_matrix(dst->type)) {
         } else {
            }
      static nir_def *
   load_dynamic_depth_bias(nir_builder *b, struct dzn_nir_point_gs_info *info)
   {
      nir_address_format ubo_format = nir_address_format_32bit_index_offset;
            nir_def *index = nir_vulkan_resource_index(
      b, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
   nir_imm_int(b, 0),
   .desc_set = info->runtime_data_cbv.register_space,
   .binding = info->runtime_data_cbv.base_shader_register,
         nir_def *load_desc = nir_load_vulkan_descriptor(
      b, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
         return nir_load_ubo(
      b, 1, 32,
   nir_channel(b, load_desc, 0),
   nir_imm_int(b, offset),
   .align_mul = 256,
   }
      nir_shader *
   dzn_nir_polygon_point_mode_gs(const nir_shader *previous_shader, struct dzn_nir_point_gs_info *info)
   {
      nir_builder builder;
   nir_builder *b = &builder;
            unsigned num_vars = 0;
   nir_variable *in[VARYING_SLOT_MAX];
               builder = nir_builder_init_simple_shader(MESA_SHADER_GEOMETRY,
                  nir_shader *nir = b->shader;
   nir->info.inputs_read = nir->info.outputs_written = previous_shader->info.outputs_written;
   nir->info.outputs_written |= (1ull << VARYING_SLOT_VAR12);
   nir->info.gs.input_primitive = MESA_PRIM_TRIANGLES;
   nir->info.gs.output_primitive = MESA_PRIM_POINTS;
   nir->info.gs.vertices_in = 3;
   nir->info.gs.vertices_out = 3;
   nir->info.gs.invocations = 1;
            nir_foreach_shader_out_variable(var, previous_shader) {
      char tmp[100];
   snprintf(tmp, ARRAY_SIZE(tmp), "in_%d", num_vars);
   in[num_vars] = nir_variable_create(nir,
                     in[num_vars]->data = var->data;
            if (var->data.location == VARYING_SLOT_POS)
            snprintf(tmp, ARRAY_SIZE(tmp), "out_%d", num_vars);
   out[num_vars] = nir_variable_create(nir, nir_var_shader_out, var->type, tmp);
                        nir_variable *front_facing_var = nir_variable_create(nir,
                     front_facing_var->data.location = VARYING_SLOT_VAR12;
   front_facing_var->data.driver_location = num_vars;
            nir_def *depth_bias_scale = NULL;
   if (info->depth_bias) {
      switch (info->ds_fmt) {
   case DXGI_FORMAT_D16_UNORM:
      depth_bias_scale = nir_imm_float(b, 1.0f / (1 << 16));
      case DXGI_FORMAT_D24_UNORM_S8_UINT:
      depth_bias_scale = nir_imm_float(b, 1.0f / (1 << 24));
      case DXGI_FORMAT_D32_FLOAT:
   case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: {
      nir_deref_instr *deref_pos = nir_build_deref_var(b, pos_var);
   nir_def *max_z = NULL;
   for (uint32_t i = 0; i < 3; ++i) {
      nir_def *pos = nir_load_deref(b, nir_build_deref_array_imm(b, deref_pos, i));
   nir_def *z = nir_iand_imm(b, nir_channel(b, pos, 2), 0x7fffffff);
      }
   nir_def *exponent = nir_ishr_imm(b, nir_iand_imm(b, max_z, 0x7f800000), 23);
   depth_bias_scale = nir_fexp2(b, nir_i2f32(b, nir_iadd_imm(b, exponent, -23)));
      }
   default:
                     /* Temporary variable "loop_index" to loop over input vertices */
   nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_variable *loop_index_var =
         nir_deref_instr *loop_index_deref = nir_build_deref_var(b, loop_index_var);
            nir_def *cull_pass = nir_imm_true(b);
   nir_def *front_facing;
   assert(info->cull_mode != VK_CULL_MODE_FRONT_AND_BACK);
   if (info->cull_mode == VK_CULL_MODE_FRONT_BIT) {
      cull_pass = cull_face(b, pos_var, info->front_ccw);
      } else if (info->cull_mode == VK_CULL_MODE_BACK_BIT) {
      cull_pass = cull_face(b, pos_var, !info->front_ccw);
      } else
            /**
   *  if (cull_pass) {
   *     while {
   *        if (loop_index >= 3)
   *           break;
   */
   nir_if *cull_check = nir_push_if(b, cull_pass);
            nir_def *loop_index = nir_load_deref(b, loop_index_deref);
   nir_def *cmp = nir_ige(b, loop_index,
         nir_if *loop_check = nir_push_if(b, cmp);
   nir_jump(b, nir_jump_break);
            /**
   *        [...] // Copy all variables
   *        EmitVertex();
   */
   for (unsigned i = 0; i < num_vars; ++i) {
      nir_def *index = loop_index;
   nir_deref_instr *in_value = nir_build_deref_array(b, nir_build_deref_var(b, in[i]), index);
   if (in[i] == pos_var && info->depth_bias) {
      nir_def *bias_val;
   if (info->depth_bias_dynamic) {
         } else {
      assert(info->slope_scaled_depth_bias == 0.0f);
      }
   bias_val = nir_fmul(b, bias_val, depth_bias_scale);
   nir_def *old_val = nir_load_deref(b, in_value);
   nir_def *new_val = nir_vector_insert_imm(b, old_val,
                  } else {
            }
   nir_store_var(b, front_facing_var, front_facing, 0x1);
            /**
   *        loop_index++;
   *     }
   *  }
   */
   nir_store_deref(b, loop_index_deref, nir_iadd_imm(b, loop_index, 1), 1);
   nir_pop_loop(b, loop);
                     NIR_PASS_V(nir, nir_lower_var_copies);
      }
