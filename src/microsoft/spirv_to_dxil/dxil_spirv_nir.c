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
      #include "spirv_to_dxil.h"
   #include "nir_to_dxil.h"
   #include "dxil_nir.h"
   #include "dxil_nir_lower_int_cubemaps.h"
   #include "shader_enums.h"
   #include "spirv/nir_spirv.h"
   #include "util/blob.h"
   #include "dxil_spirv_nir.h"
      #include "git_sha1.h"
   #include "vulkan/vulkan.h"
      static const struct spirv_to_nir_options
   spirv_to_nir_options = {
      .caps = {
      .draw_parameters = true,
   .multiview = true,
   .subgroup_basic = true,
   .subgroup_ballot = true,
   .subgroup_vote = true,
   .subgroup_shuffle = true,
   .subgroup_quad = true,
   .subgroup_arithmetic = true,
   .descriptor_array_dynamic_indexing = true,
   .float_controls = true,
   .float16 = true,
   .int16 = true,
   .storage_16bit = true,
   .storage_8bit = true,
   .descriptor_indexing = true,
   .runtime_descriptor_array = true,
   .descriptor_array_non_uniform_indexing = true,
   .image_read_without_format = true,
   .image_write_without_format = true,
   .int64 = true,
      },
   .ubo_addr_format = nir_address_format_32bit_index_offset,
   .ssbo_addr_format = nir_address_format_32bit_index_offset,
            .min_ubo_alignment = 256, /* D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT */
            .mediump_16bit_alu = true,
      };
      const struct spirv_to_nir_options*
   dxil_spirv_nir_get_spirv_options(void)
   {
         }
      /* Logic extracted from vk_spirv_to_nir() so we have the same preparation
   * steps for both the vulkan driver and the lib used by the WebGPU
   * implementation.
   * Maybe we should move those steps out of vk_spirv_to_nir() and make
   * them vk agnosting (right, the only vk specific thing is the vk_device
   * object that's used for the debug callback passed to spirv_to_nir()).
   */
   void
   dxil_spirv_nir_prep(nir_shader *nir)
   {
      /* We have to lower away local constant initializers right before we
   * inline functions.  That way they get properly initialized at the top
   * of the function and not at the top of its caller.
   */
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_inline_functions);
   NIR_PASS_V(nir, nir_copy_prop);
            /* Pick off the single entrypoint that we want */
            /* Now that we've deleted all but the main function, we can go ahead and
   * lower the rest of the constant initializers.  We do this here so that
   * nir_remove_dead_variables and split_per_member_structs below see the
   * corresponding stores.
   */
            /* Split member structs.  We do this before lower_io_to_temporaries so that
   * it doesn't lower system values to temporaries by accident.
   */
   NIR_PASS_V(nir, nir_split_var_copies);
            NIR_PASS_V(nir, nir_remove_dead_variables,
            nir_var_shader_in | nir_var_shader_out | nir_var_system_value |
            }
      static void
   shared_var_info(const struct glsl_type* type, unsigned* size, unsigned* align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type) ? 4 : glsl_get_bit_size(type) / 8;
   unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length;
      }
      static void
   temp_var_info(const struct glsl_type* type, unsigned* size, unsigned* align)
   {
      uint32_t base_size, base_align;
   switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_ARRAY:
      temp_var_info(glsl_get_array_element(type), &base_size, align);
   *size = base_size * glsl_array_size(type);
      case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE:
      *size = 0;
   *align = 0;
   for (uint32_t i = 0; i < glsl_get_length(type); ++i) {
      temp_var_info(glsl_get_struct_field(type, i), &base_size, &base_align);
   *size = ALIGN_POT(*size, base_align) + base_size;
      }
      default:
               *align = MAX2(base_align, 4);
   *size = ALIGN_POT(base_size, *align);
         }
      static nir_variable *
   add_runtime_data_var(nir_shader *nir, unsigned desc_set, unsigned binding)
   {
      unsigned runtime_data_size =
      nir->info.stage == MESA_SHADER_COMPUTE
               const struct glsl_type *array_type =
      glsl_array_type(glsl_uint_type(), runtime_data_size / sizeof(unsigned),
      const struct glsl_struct_field field = {array_type, "arr"};
   nir_variable *var = nir_variable_create(
      nir, nir_var_mem_ubo,
      var->data.descriptor_set = desc_set;
   // Check that desc_set fits on descriptor_set
   assert(var->data.descriptor_set == desc_set);
   var->data.binding = binding;
   var->data.how_declared = nir_var_hidden;
      }
      static bool
   lower_shader_system_values(struct nir_builder *builder, nir_instr *instr,
         {
      if (instr->type != nir_instr_type_intrinsic) {
                           /* All the intrinsics we care about are loads */
   if (!nir_intrinsic_infos[intrin->intrinsic].has_dest)
               const struct dxil_spirv_runtime_conf *conf =
            int offset = 0;
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_num_workgroups:
      offset =
            case nir_intrinsic_load_base_workgroup_id:
      offset =
            case nir_intrinsic_load_first_vertex:
      offset = offsetof(struct dxil_spirv_vertex_runtime_data, first_vertex);
      case nir_intrinsic_load_is_indexed_draw:
      offset =
            case nir_intrinsic_load_base_instance:
      offset = offsetof(struct dxil_spirv_vertex_runtime_data, base_instance);
      case nir_intrinsic_load_draw_id:
      offset = offsetof(struct dxil_spirv_vertex_runtime_data, draw_id);
      case nir_intrinsic_load_view_index:
      if (!conf->lower_view_index)
         offset = offsetof(struct dxil_spirv_vertex_runtime_data, view_index);
      default:
                  builder->cursor = nir_after_instr(instr);
            nir_def *index = nir_vulkan_resource_index(
      builder, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
   nir_imm_int(builder, 0),
   .desc_set = conf->runtime_data_cbv.register_space,
   .binding = conf->runtime_data_cbv.base_shader_register,
         nir_def *load_desc = nir_load_vulkan_descriptor(
      builder, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
         nir_def *load_data = nir_load_ubo(
      builder, 
   intrin->def.num_components,
   intrin->def.bit_size,
   nir_channel(builder, load_desc, 0),
   nir_imm_int(builder, offset),
   .align_mul = 256,
   .align_offset = offset,
   .range_base = offset,
         nir_def_rewrite_uses(&intrin->def, load_data);
   nir_instr_remove(instr);
      }
      static bool
   dxil_spirv_nir_lower_shader_system_values(nir_shader *shader,
         {
      return nir_shader_instructions_pass(shader, lower_shader_system_values,
                        }
      static nir_variable *
   add_push_constant_var(nir_shader *nir, unsigned size, unsigned desc_set, unsigned binding)
   {
      /* Size must be a multiple of 16 as buffer load is loading 16 bytes at a time */
            const struct glsl_type *array_type =
         const struct glsl_struct_field field = {array_type, "arr"};
   nir_variable *var = nir_variable_create(
      nir, nir_var_mem_ubo,
      var->data.descriptor_set = desc_set;
   var->data.binding = binding;
   var->data.how_declared = nir_var_hidden;
      }
      struct lower_load_push_constant_data {
      nir_address_format ubo_format;
   unsigned desc_set;
   unsigned binding;
      };
      static bool
   lower_load_push_constant(struct nir_builder *builder, nir_instr *instr,
         {
      struct lower_load_push_constant_data *data =
            if (instr->type != nir_instr_type_intrinsic)
                     /* All the intrinsics we care about are loads */
   if (intrin->intrinsic != nir_intrinsic_load_push_constant)
            uint32_t base = nir_intrinsic_base(intrin);
                     builder->cursor = nir_after_instr(instr);
            nir_def *index = nir_vulkan_resource_index(
      builder, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
   nir_imm_int(builder, 0),
   .desc_set = data->desc_set, .binding = data->binding,
         nir_def *load_desc = nir_load_vulkan_descriptor(
      builder, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
         nir_def *offset = intrin->src[0].ssa;
   nir_def *load_data = nir_load_ubo(
      builder, 
   intrin->def.num_components,
   intrin->def.bit_size, 
   nir_channel(builder, load_desc, 0),
   nir_iadd_imm(builder, offset, base),
   .align_mul = nir_intrinsic_align_mul(intrin),
   .align_offset = nir_intrinsic_align_offset(intrin),
   .range_base = base,
         nir_def_rewrite_uses(&intrin->def, load_data);
   nir_instr_remove(instr);
      }
      static bool
   dxil_spirv_nir_lower_load_push_constant(nir_shader *shader,
                     {
      bool ret;
   struct lower_load_push_constant_data data = {
      .ubo_format = ubo_format,
   .desc_set = desc_set,
      };
   ret = nir_shader_instructions_pass(shader, lower_load_push_constant,
                                                   }
      struct lower_yz_flip_data {
      bool *reads_sysval_ubo;
      };
      static bool
   lower_yz_flip(struct nir_builder *builder, nir_instr *instr,
         {
      struct lower_yz_flip_data *data =
            if (instr->type != nir_instr_type_intrinsic)
                     if (intrin->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   if (var->data.mode != nir_var_shader_out ||
      var->data.location != VARYING_SLOT_POS)
                           nir_def *pos = intrin->src[1].ssa;
   nir_def *y_pos = nir_channel(builder, pos, 1);
   nir_def *z_pos = nir_channel(builder, pos, 2);
            if (rt_conf->yz_flip.mode & DXIL_SPIRV_YZ_FLIP_CONDITIONAL) {
      // conditional YZ-flip. The flip bitmask is passed through the vertex
   // runtime data UBO.
   unsigned offset =
                  nir_def *index = nir_vulkan_resource_index(
      builder, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
   nir_imm_int(builder, 0),
   .desc_set = rt_conf->runtime_data_cbv.register_space,
               nir_def *load_desc = nir_load_vulkan_descriptor(
      builder, nir_address_format_num_components(ubo_format),
               dyn_yz_flip_mask =
      nir_load_ubo(builder, 1, 32,
                     nir_channel(builder, load_desc, 0),
   nir_imm_int(builder, offset),
                  if (rt_conf->yz_flip.mode & DXIL_SPIRV_Y_FLIP_UNCONDITIONAL)
         else if (rt_conf->yz_flip.mode & DXIL_SPIRV_Y_FLIP_CONDITIONAL)
            if (rt_conf->yz_flip.mode & DXIL_SPIRV_Z_FLIP_UNCONDITIONAL)
         else if (rt_conf->yz_flip.mode & DXIL_SPIRV_Z_FLIP_CONDITIONAL)
                     if (y_flip_mask) {
               // Z-flip => pos.y = -pos.y
               if (z_flip_mask) {
               // Z-flip => pos.z = -pos.z + 1.0f
   z_pos = nir_bcsel(builder, flip,
                     nir_def *def = nir_vec4(builder,
                           nir_src_rewrite(&intrin->src[1], def);
      }
      bool
   dxil_spirv_nir_lower_yz_flip(nir_shader *shader,
               {
      struct lower_yz_flip_data data = {
      .rt_conf = rt_conf,
               return nir_shader_instructions_pass(shader, lower_yz_flip,
                        }
      static bool
   discard_psiz_access(struct nir_builder *builder, nir_intrinsic_instr *intrin,
         {
      if (intrin->intrinsic != nir_intrinsic_store_deref &&
      intrin->intrinsic != nir_intrinsic_load_deref)
         nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   if (!var || var->data.mode != nir_var_shader_out ||
      var->data.location != VARYING_SLOT_PSIZ)
                  if (intrin->intrinsic == nir_intrinsic_load_deref)
            nir_instr_remove(&intrin->instr);
      }
      static bool
   dxil_spirv_nir_discard_point_size_var(nir_shader *shader)
   {
      if (shader->info.stage != MESA_SHADER_VERTEX &&
      shader->info.stage != MESA_SHADER_TESS_EVAL &&
   shader->info.stage != MESA_SHADER_GEOMETRY)
         nir_variable *psiz = NULL;
   nir_foreach_shader_out_variable(var, shader) {
      if (var->data.location == VARYING_SLOT_PSIZ) {
      psiz = var;
                  if (!psiz)
            if (!nir_shader_intrinsics_pass(shader, discard_psiz_access,
                                    nir_remove_dead_derefs(shader);
      }
      static bool
   kill_undefined_varyings(struct nir_builder *b,
               {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_load_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!var || var->data.mode != nir_var_shader_in)
            /* Ignore most builtins for now, some of them get default values
   * when not written from previous stages.
   */
   if (var->data.location < VARYING_SLOT_VAR0 &&
      var->data.location != VARYING_SLOT_POS)
         uint32_t loc = var->data.patch ?
         uint64_t written = var->data.patch ?
               if (BITFIELD64_RANGE(loc, glsl_varying_count(var->type)) & written)
            b->cursor = nir_after_instr(instr);
   /* Note: zero is used instead of undef, because optimization is not run here, but is
   * run later on. If we load an undef here, and that undef ends up being used to store
   * to position later on, that can cause some or all of the components in that position
   * write to be removed, which is problematic especially in the case of all components,
   * since that would remove the store instruction, and would make it tricky to satisfy
   * the DXIL requirements of writing all position components.
   */
   nir_def *zero = nir_imm_zero(b, intr->def.num_components,
         nir_def_rewrite_uses(&intr->def, zero);
   nir_instr_remove(instr);
      }
      static bool
   dxil_spirv_nir_kill_undefined_varyings(nir_shader *shader,
         {
      if (!nir_shader_instructions_pass(shader,
                                          nir_remove_dead_derefs(shader);
   nir_remove_dead_variables(shader, nir_var_shader_in, NULL);
      }
      static bool
   kill_unused_outputs(struct nir_builder *b,
               {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!var || var->data.mode != nir_var_shader_out)
            unsigned loc = var->data.patch ?
               if (!(BITFIELD64_RANGE(loc, glsl_varying_count(var->type)) & kill_mask))
            nir_instr_remove(instr);
      }
      static bool
   dxil_spirv_nir_kill_unused_outputs(nir_shader *shader,
         {
      uint64_t kill_var_mask =
                  /* Don't kill buitin vars */
            if (nir_shader_instructions_pass(shader,
                                          if (shader->info.stage == MESA_SHADER_TESS_EVAL) {
      kill_var_mask =
      (shader->info.patch_outputs_written |
   shader->info.patch_outputs_read) &
      if (nir_shader_instructions_pass(shader,
                                             if (progress) {
      nir_opt_dce(shader);
   nir_remove_dead_derefs(shader);
                  }
      struct lower_pntc_data {
      const struct dxil_spirv_runtime_conf *conf;
      };
      static bool
   write_pntc_with_pos(nir_builder *b, nir_instr *instr, void *_data)
   {
      struct lower_pntc_data *data = (struct lower_pntc_data *)_data;
   if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_store_deref)
         nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!var || var->data.location != VARYING_SLOT_POS)
                     unsigned offset =
         static_assert(offsetof(struct dxil_spirv_vertex_runtime_data, viewport_width) % 16 == 4,
                  b->cursor = nir_before_instr(instr);
   nir_def *index = nir_vulkan_resource_index(
      b, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
   nir_imm_int(b, 0),
   .desc_set = data->conf->runtime_data_cbv.register_space,
   .binding = data->conf->runtime_data_cbv.base_shader_register,
         nir_def *load_desc = nir_load_vulkan_descriptor(
      b, nir_address_format_num_components(ubo_format),
   nir_address_format_bit_size(ubo_format),
         nir_def *transform = nir_channels(b,
                                       nir_load_ubo(b, 4, 32,
   nir_def *point_center_in_clip = nir_fmul(b, nir_trim_vector(b, pos, 2),
         nir_def *point_center =
      nir_fmul(b, nir_fadd_imm(b,
                  nir_store_var(b, data->pntc, nir_pad_vec4(b, point_center), 0xf);
      }
      static void
   dxil_spirv_write_pntc(nir_shader *nir, const struct dxil_spirv_runtime_conf *conf)
   {
      struct lower_pntc_data data = { .conf = conf };
   data.pntc = nir_variable_create(nir, nir_var_shader_out, glsl_vec4_type(), "gl_PointCoord");
   data.pntc->data.location = VARYING_SLOT_PNTC;
   nir_shader_instructions_pass(nir, write_pntc_with_pos,
                                    /* Add the runtime data var if it's not already there */
   nir_binding binding = {
      .binding = conf->runtime_data_cbv.base_shader_register,
   .desc_set = conf->runtime_data_cbv.register_space,
      };
   nir_variable *ubo_var = nir_get_binding_variable(nir, binding);
   if (!ubo_var)
      }
      static bool
   lower_pntc_read(nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
      if (intr->intrinsic != nir_intrinsic_load_deref)
         nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!var || var->data.location != VARYING_SLOT_PNTC)
            nir_def *point_center = &intr->def;
                     nir_def *pos;
   if (var->data.sample == pos_var->data.sample)
         else if (var->data.sample)
      pos = nir_interp_deref_at_sample(b, 4, 32,
            else
      pos = nir_interp_deref_at_offset(b, 4, 32,
               nir_def *pntc = nir_fadd_imm(b,
               nir_def_rewrite_uses_after(point_center, pntc, pntc->parent_instr);
      }
      static void
   dxil_spirv_compute_pntc(nir_shader *nir)
   {
      nir_variable *pos = nir_find_variable_with_location(nir, nir_var_shader_in, VARYING_SLOT_POS);
   if (!pos) {
      pos = nir_variable_create(nir, nir_var_shader_in, glsl_vec4_type(), "gl_FragCoord");
   pos->data.location = VARYING_SLOT_POS;
      }
   nir_shader_intrinsics_pass(nir, lower_pntc_read,
                        }
      static bool
   lower_view_index_to_rt_layer_instr(nir_builder *b, nir_intrinsic_instr *intr,
         {
      if (intr->intrinsic != nir_intrinsic_store_deref)
            nir_variable *var = nir_intrinsic_get_var(intr, 0);
   if (!var ||
      var->data.mode != nir_var_shader_out ||
   var->data.location != VARYING_SLOT_LAYER)
         b->cursor = nir_before_instr(&intr->instr);
   nir_def *layer = intr->src[1].ssa;
   nir_def *new_layer = nir_iadd(b, layer,
         nir_src_rewrite(&intr->src[1], new_layer);
      }
      static bool
   add_layer_write(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr) {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_emit_vertex &&
      intr->intrinsic != nir_intrinsic_emit_vertex_with_counter)
         }
   nir_variable *var = (nir_variable *)data;
   nir_store_var(b, var, nir_load_view_index(b), 0x1);
      }
      static void
   lower_view_index_to_rt_layer(nir_shader *nir)
   {
      bool existing_write =
      nir_shader_intrinsics_pass(nir, lower_view_index_to_rt_layer_instr,
                     if (existing_write)
            nir_variable *var = nir_variable_create(nir, nir_var_shader_out,
         var->data.location = VARYING_SLOT_LAYER;
   var->data.interpolation = INTERP_MODE_FLAT;
   if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      nir_shader_instructions_pass(nir,
                        } else {
      nir_function_impl *func = nir_shader_get_entrypoint(nir);
   nir_builder b = nir_builder_at(nir_after_impl(func));
         }
      void
   dxil_spirv_nir_link(nir_shader *nir, nir_shader *prev_stage_nir,
               {
               *requires_runtime_data = false;
   if (prev_stage_nir) {
      if (nir->info.stage == MESA_SHADER_FRAGMENT) {
               if (nir->info.inputs_read & VARYING_BIT_PNTC) {
      NIR_PASS_V(prev_stage_nir, dxil_spirv_write_pntc, conf);
   NIR_PASS_V(nir, dxil_spirv_compute_pntc);
                  NIR_PASS_V(nir, dxil_spirv_nir_kill_undefined_varyings, prev_stage_nir);
            nir->info.inputs_read =
                  prev_stage_nir->info.outputs_written =
      dxil_reassign_driver_locations(prev_stage_nir, nir_var_shader_out,
               }
      static unsigned
   lower_bit_size_callback(const nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
         nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_reduce:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
         default:
            }
      void
   dxil_spirv_nir_passes(nir_shader *nir,
               {
               NIR_PASS_V(nir, nir_lower_io_to_vector,
               NIR_PASS_V(nir, nir_opt_combine_stores, nir_var_shader_out);
            const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
      .frag_coord = true,
      };
                     nir_lower_compute_system_values_options compute_options = {
         };
   NIR_PASS_V(nir, nir_lower_compute_system_values, &compute_options);
   NIR_PASS_V(nir, dxil_nir_lower_subgroup_id);
            nir_lower_subgroups_options subgroup_options = {
      .ballot_bit_size = 32,
   .ballot_components = 4,
   .lower_subgroup_masks = true,
   .lower_to_scalar = true,
   .lower_relative_shuffle = true,
      };
   if (nir->info.stage != MESA_SHADER_FRAGMENT &&
      nir->info.stage != MESA_SHADER_COMPUTE)
      NIR_PASS_V(nir, nir_lower_subgroups, &subgroup_options);
            // Ensure subgroup scans on bools are gone
   NIR_PASS_V(nir, nir_opt_dce);
            // Force sample-rate shading if we're asked to.
   if (conf->force_sample_rate_shading) {
      assert(nir->info.stage == MESA_SHADER_FRAGMENT);
               if (conf->zero_based_vertex_instance_id) {
      // vertex_id and instance_id should have already been transformed to
   // base zero before spirv_to_dxil was called. Therefore, we can zero out
   // base/firstVertex/Instance.
   gl_system_value system_values[] = {SYSTEM_VALUE_FIRST_VERTEX,
               NIR_PASS_V(nir, dxil_nir_lower_system_values_to_zero, system_values,
               if (conf->lower_view_index_to_rt_layer)
            *requires_runtime_data = false;
   NIR_PASS(*requires_runtime_data, nir,
                  if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS_V(nir, nir_lower_input_attachments,
            &(nir_input_attachment_options){
                     /* This will lower load_helper to a memoized is_helper if needed; otherwise, load_helper
   * will stay, but trivially translatable to IsHelperLane(), which will be known to be
   * constant across the invocation since no demotion would have been used.
   */
            NIR_PASS_V(nir, dxil_nir_lower_discard_and_terminate);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, dxil_nir_lower_sample_pos);
                        if (conf->inferred_read_only_images_as_srvs) {
      const nir_opt_access_options opt_access_options = {
         };
                        NIR_PASS_V(nir, nir_remove_dead_variables,
            nir_var_shader_in | nir_var_shader_out |
         uint32_t push_constant_size = 0;
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_push_const,
         NIR_PASS_V(nir, dxil_spirv_nir_lower_load_push_constant,
            nir_address_format_32bit_index_offset,
   conf->push_constant_cbv.register_space,
         NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ubo | nir_var_mem_ssbo,
            if (nir->info.shared_memory_explicit_layout) {
      NIR_PASS_V(nir, nir_lower_vars_to_explicit_types, nir_var_mem_shared,
         NIR_PASS_V(nir, dxil_nir_split_unaligned_loads_stores, nir_var_mem_shared);
      } else {
      NIR_PASS_V(nir, nir_split_struct_vars, nir_var_mem_shared);
   NIR_PASS_V(nir, dxil_nir_flatten_var_arrays, nir_var_mem_shared);
   NIR_PASS_V(nir, dxil_nir_lower_var_bit_size, nir_var_mem_shared,
                        NIR_PASS_V(nir, nir_lower_clip_cull_distance_arrays);
   NIR_PASS_V(nir, nir_lower_io_to_temporaries, nir_shader_get_entrypoint(nir), true, true);
   NIR_PASS_V(nir, nir_lower_global_vars_to_local);
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
               if (conf->yz_flip.mode != DXIL_SPIRV_YZ_FLIP_NONE) {
      assert(nir->info.stage == MESA_SHADER_VERTEX ||
         NIR_PASS_V(nir,
                     if (*requires_runtime_data) {
      add_runtime_data_var(nir, conf->runtime_data_cbv.register_space,
               if (push_constant_size > 0) {
      add_push_constant_var(nir, push_constant_size,
                     NIR_PASS_V(nir, nir_lower_fp16_casts, nir_lower_fp16_all & ~nir_lower_fp16_rtz);
   NIR_PASS_V(nir, nir_lower_alu_to_scalar, NULL, NULL);
   NIR_PASS_V(nir, nir_opt_dce);
            {
      bool progress;
   do
   {
      progress = false;
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
   NIR_PASS(progress, nir, nir_opt_deref);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_opt_undef);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
   NIR_PASS(progress, nir, nir_opt_cse);
   if (nir_opt_trivial_continues(nir)) {
      progress = true;
   NIR_PASS(progress, nir, nir_copy_prop);
      }
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
                  NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
   NIR_PASS_V(nir, nir_split_struct_vars, nir_var_function_temp);
   NIR_PASS_V(nir, dxil_nir_flatten_var_arrays, nir_var_function_temp);
   NIR_PASS_V(nir, dxil_nir_lower_var_bit_size, nir_var_function_temp,
                     if (conf->declared_read_only_images_as_srvs)
         nir_lower_tex_options lower_tex_options = {
      .lower_txp = UINT32_MAX,
   .lower_invalid_implicit_lod = true,
      };
            NIR_PASS_V(nir, dxil_nir_split_clip_cull_distance);
   const struct dxil_nir_lower_loads_stores_options loads_stores_options = {
         };
   NIR_PASS_V(nir, dxil_nir_lower_loads_stores_to_dxil, &loads_stores_options);
   NIR_PASS_V(nir, dxil_nir_split_typed_samplers);
   NIR_PASS_V(nir, dxil_nir_lower_ubo_array_one_to_static);
   NIR_PASS_V(nir, nir_opt_dce);
   NIR_PASS_V(nir, nir_remove_dead_derefs);
   NIR_PASS_V(nir, nir_remove_dead_variables,
                  if (nir->info.stage == MESA_SHADER_FRAGMENT) {
         } else {
      /* Dummy linking step so we get different driver_location
   * assigned even if there's just a single vertex shader in the
   * pipeline. The real linking happens in dxil_spirv_nir_link().
   */
   nir->info.outputs_written =
               if (nir->info.stage == MESA_SHADER_VERTEX) {
      nir_foreach_variable_with_modes(var, nir, nir_var_shader_in) {
      /* spirv_to_dxil() only emits generic vertex attributes. */
   assert(var->data.location >= VERT_ATTRIB_GENERIC0);
               nir->info.inputs_read =
      } else {
      nir->info.inputs_read =
                           }
