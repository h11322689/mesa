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
      #include "nir.h"
   #include "nir_clc_helpers.h"
   #include "nir_serialize.h"
   #include "glsl_types.h"
   #include "nir_types.h"
   #include "clc_compiler.h"
   #include "clc_helpers.h"
   #include "clc_nir.h"
   #include "../compiler/dxil_nir.h"
   #include "../compiler/dxil_nir_lower_int_samplers.h"
   #include "../compiler/nir_to_dxil.h"
      #include "util/u_debug.h"
   #include <util/u_math.h>
   #include "spirv/nir_spirv.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      #include "git_sha1.h"
      struct clc_image_lower_context
   {
      struct clc_dxil_metadata *metadata;
   unsigned *num_srvs;
   unsigned *num_uavs;
   nir_deref_instr *deref;
   unsigned num_buf_ids;
      };
      static int
   lower_image_deref_impl(nir_builder *b, struct clc_image_lower_context *context,
                     {
      nir_variable *in_var = nir_deref_instr_get_variable(context->deref);
   nir_variable *image = nir_variable_create(b->shader, var_mode, new_var_type, NULL);
   image->data.access = in_var->data.access;
   image->data.binding = in_var->data.binding;
   if (context->num_buf_ids > 0) {
      // Need to assign a new binding
   context->metadata->args[context->metadata_index].
      }
   context->num_buf_ids++;
      }
      static int
   lower_read_only_image_deref(nir_builder *b, struct clc_image_lower_context *context,
         {
               // Non-writeable images should be converted to samplers,
   // since they may have texture operations done on them
   const struct glsl_type *new_var_type =
      glsl_texture_type(glsl_get_sampler_dim(in_var->type),
               }
      static int
   lower_read_write_image_deref(nir_builder *b, struct clc_image_lower_context *context,
         {
      nir_variable *in_var = nir_deref_instr_get_variable(context->deref);
   const struct glsl_type *new_var_type =
      glsl_image_type(glsl_get_sampler_dim(in_var->type),
      glsl_sampler_type_is_array(in_var->type),
      }
      static void
   clc_lower_input_image_deref(nir_builder *b, struct clc_image_lower_context *context)
   {
      // The input variable here isn't actually an image, it's just the
   // image format data.
   //
   // For every use of an image in a different way, we'll add an
   // appropriate image to match it. That can result in up to
   // 3 images (float4, int4, uint4) for each image. Only one of these
   // formats will actually produce correct data, but a single kernel
   // could use runtime conditionals to potentially access any of them.
   //
   // If the image is used in a query that doesn't have a corresponding
   // DXIL intrinsic (CL image channel order or channel format), then
   // we'll add a kernel input for that data that'll be lowered by the
   // explicit IO pass later on.
   //
            enum image_type {
      FLOAT4,
   INT4,
   UINT4,
               int image_bindings[IMAGE_TYPE_COUNT] = {-1, -1, -1};
                     context->metadata_index = 0;
   while (context->metadata->args[context->metadata_index].image.buf_ids[0] != in_var->data.binding)
                     /* Do this in 2 passes:
   * 1. When encountering a strongly-typed access (load/store), replace the deref
   *    with one that references an appropriately typed variable. When encountering
   *    an untyped access (size query), if we have a strongly-typed variable already,
   *    replace the deref to point to it.
   * 2. If there's any references left, they should all be untyped. If we found
   *    a strongly-typed access later in the 1st pass, then just replace the reference.
   *    If we didn't, e.g. the resource is only used for a size query, then pick an
   *    arbitrary type for it.
   */
   for (int pass = 0; pass < 2; ++pass) {
      nir_foreach_use_safe(src, &context->deref->def) {
               if (nir_src_parent_instr(src)->type == nir_instr_type_intrinsic) {
                              switch (intrinsic->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store: {
                     switch (nir_alu_type_get_base_type(dest_type)) {
   case nir_type_float: type = FLOAT4; break;
   case nir_type_int: type = INT4; break;
                        int image_binding = image_bindings[type];
   if (image_binding < 0) {
                        assert((in_var->data.access & ACCESS_NON_WRITEABLE) == 0);
                     case nir_intrinsic_image_deref_size: {
      int image_binding = -1;
   for (unsigned i = 0; i < IMAGE_TYPE_COUNT; ++i) {
      if (image_bindings[i] >= 0) {
      image_binding = image_bindings[i];
         }
   if (image_binding < 0) {
                           type = FLOAT4;
                     assert((in_var->data.access & ACCESS_NON_WRITEABLE) == 0);
                     case nir_intrinsic_image_deref_format:
   case nir_intrinsic_image_deref_order: {
      nir_def **cached_deref = intrinsic->intrinsic == nir_intrinsic_image_deref_format ?
         if (!*cached_deref) {
      nir_variable *new_input = nir_variable_create(b->shader, nir_var_uniform, glsl_uint_type(), NULL);
   new_input->data.driver_location = in_var->data.driver_location;
   if (intrinsic->intrinsic == nir_intrinsic_image_deref_format) {
                                          /* No actual intrinsic needed here, just reference the loaded variable */
   nir_def_rewrite_uses(&intrinsic->def, *cached_deref);
                     default:
            } else if (nir_src_parent_instr(src)->type == nir_instr_type_tex) {
                     switch (nir_alu_type_get_base_type(tex->dest_type)) {
   case nir_type_float: type = FLOAT4; break;
   case nir_type_int: type = INT4; break;
   case nir_type_uint: type = UINT4; break;
                  int image_binding = image_bindings[type];
   if (image_binding < 0) {
                        nir_tex_instr_remove_src(tex, nir_tex_instr_src_index(tex, nir_tex_src_texture_deref));
                              nir_instr_remove(&context->deref->instr);
      }
      static void
   clc_lower_images(nir_shader *nir, struct clc_image_lower_context *context)
   {
      nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                           nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                        if (glsl_type_is_image(context->deref->type)) {
      assert(context->deref->deref_type == nir_deref_type_var);
                     }
      static void
   clc_lower_64bit_semantics(nir_shader *nir)
   {
      nir_foreach_function_impl(impl, nir) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intrinsic = nir_instr_as_intrinsic(instr);
   switch (intrinsic->intrinsic) {
   case nir_intrinsic_load_global_invocation_id:
   case nir_intrinsic_load_global_invocation_id_zero_base:
   case nir_intrinsic_load_base_global_invocation_id:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_workgroup_id_zero_base:
   case nir_intrinsic_load_base_workgroup_id:
   case nir_intrinsic_load_num_workgroups:
                                                            nir_def *i64 = nir_u2u64(&b, &intrinsic->def);
   nir_def_rewrite_uses_after(
      &intrinsic->def,
   i64,
               }
      static void
   clc_lower_nonnormalized_samplers(nir_shader *nir,
         {
      nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                           nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_tex)
                  int sampler_src_idx = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
                  nir_src *sampler_src = &tex->src[sampler_src_idx].src;
   assert(sampler_src->ssa->parent_instr->type == nir_instr_type_deref);
                  // If the sampler returns ints, we'll handle this in the int lowering pass
                  // If sampler uses normalized coords, nothing to do
                           int coords_idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   assert(coords_idx != -1);
                           // Normalize coords for tex
   nir_def *scale = nir_frcp(&b, txs);
   nir_def *comps[4];
   for (unsigned i = 0; i < coords->num_components; ++i) {
      comps[i] = nir_channel(&b, coords, i);
   if (tex->is_array && i == coords->num_components - 1) {
      // Don't scale the array index, but do clamp it
   comps[i] = nir_fround_even(&b, comps[i]);
   comps[i] = nir_fmax(&b, comps[i], nir_imm_float(&b, 0.0f));
                     // The CTS is pretty clear that this value has to be floored for nearest sampling
   // but must not be for linear sampling.
   if (!states[sampler->data.binding].is_linear_filtering)
            }
   nir_def *normalized_coords = nir_vec(&b, comps, coords->num_components);
               }
      static nir_variable *
   add_kernel_inputs_var(struct clc_dxil_object *dxil, nir_shader *nir,
         {
      if (!dxil->kernel->num_args)
                     nir_foreach_variable_with_modes(var, nir, nir_var_uniform)
      size = MAX2(size,
                        const struct glsl_type *array_type = glsl_array_type(glsl_uint_type(), size / 4, 4);
   const struct glsl_struct_field field = { array_type, "arr" };
   nir_variable *var =
      nir_variable_create(nir, nir_var_mem_ubo,
      glsl_struct_type(&field, 1, "kernel_inputs", false),
   var->data.binding = (*cbv_id)++;
   var->data.how_declared = nir_var_hidden;
      }
      static nir_variable *
   add_work_properties_var(struct clc_dxil_object *dxil,
         {
      const struct glsl_type *array_type =
      glsl_array_type(glsl_uint_type(),
      sizeof(struct clc_work_properties_data) / sizeof(unsigned),
   const struct glsl_struct_field field = { array_type, "arr" };
   nir_variable *var =
      nir_variable_create(nir, nir_var_mem_ubo,
      glsl_struct_type(&field, 1, "kernel_work_properties", false),
   var->data.binding = (*cbv_id)++;
   var->data.how_declared = nir_var_hidden;
      }
      static void
   clc_lower_constant_to_ssbo(nir_shader *nir,
         {
      /* Update UBO vars and assign them a binding. */
   nir_foreach_variable_with_modes(var, nir, nir_var_mem_constant) {
      var->data.mode = nir_var_mem_ssbo;
               /* And finally patch all the derefs referincing the constant
   * variables/pointers.
   */
   nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                     nir_foreach_block(block, func->impl) {
      nir_foreach_instr(instr, block) {
                                                         }
      static void
   clc_change_variable_mode(nir_shader *nir, nir_variable_mode from, nir_variable_mode to)
   {
      nir_foreach_variable_with_modes(var, nir, from)
            nir_foreach_function(func, nir) {
      if (!func->is_entrypoint)
                     nir_foreach_block(block, func->impl) {
      nir_foreach_instr(instr, block) {
                                                         }
      static void
   copy_const_initializer(const nir_constant *constant, const struct glsl_type *type,
         {
      if (glsl_type_is_array(type)) {
      const struct glsl_type *elm_type = glsl_get_array_element(type);
            for (unsigned i = 0; i < constant->num_elements; i++) {
      copy_const_initializer(constant->elements[i], elm_type,
         } else if (glsl_type_is_struct(type)) {
      for (unsigned i = 0; i < constant->num_elements; i++) {
      const struct glsl_type *elm_type = glsl_get_struct_field(type, i);
   int offset = glsl_get_struct_field_offset(type, i);
         } else {
               for (unsigned i = 0; i < glsl_get_components(type); i++) {
      switch (glsl_get_bit_size(type)) {
   case 64:
      *((uint64_t *)data) = constant->values[i].u64;
      case 32:
      *((uint32_t *)data) = constant->values[i].u32;
      case 16:
      *((uint16_t *)data) = constant->values[i].u16;
      case 8:
      *((uint8_t *)data) = constant->values[i].u8;
      default:
                           }
      static enum pipe_tex_wrap
   wrap_from_cl_addressing(unsigned addressing_mode)
   {
      switch (addressing_mode)
   {
   default:
   case SAMPLER_ADDRESSING_MODE_NONE:
   case SAMPLER_ADDRESSING_MODE_CLAMP:
      // Since OpenCL's only border color is 0's and D3D specs out-of-bounds loads to return 0, don't apply any wrap mode
      case SAMPLER_ADDRESSING_MODE_CLAMP_TO_EDGE: return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   case SAMPLER_ADDRESSING_MODE_REPEAT: return PIPE_TEX_WRAP_REPEAT;
   case SAMPLER_ADDRESSING_MODE_REPEAT_MIRRORED: return PIPE_TEX_WRAP_MIRROR_REPEAT;
      }
      static bool shader_has_double(nir_shader *nir)
   {
      foreach_list_typed(nir_function, func, node, &nir->functions) {
      if (!func->is_entrypoint)
                     nir_foreach_block(block, func->impl) {
      nir_foreach_instr_safe(instr, block) {
                                    if (info->output_type & nir_type_float &&
      alu->def.bit_size == 64)
                     }
      struct clc_libclc *
   clc_libclc_new_dxil(const struct clc_logger *logger,
         {
      struct clc_libclc_options clc_options = {
      .optimize = options->optimize,
                  }
      bool
   clc_spirv_to_dxil(struct clc_libclc *lib,
                     const struct clc_binary *linked_spirv,
   const struct clc_parsed_spirv *parsed_data,
   const char *entrypoint,
   const struct clc_runtime_kernel_conf *conf,
   {
               for (unsigned i = 0; i < parsed_data->num_kernels; i++) {
      if (!strcmp(parsed_data->kernels[i].name, entrypoint)) {
      out_dxil->kernel = &parsed_data->kernels[i];
                  if (!out_dxil->kernel) {
      clc_error(logger, "no '%s' kernel found", entrypoint);
               const struct spirv_to_nir_options spirv_options = {
      .environment = NIR_SPIRV_OPENCL,
   .clc_shader = clc_libclc_get_clc_shader(lib),
   .constant_addr_format = nir_address_format_32bit_index_offset_pack64,
   .global_addr_format = nir_address_format_32bit_index_offset_pack64,
   .shared_addr_format = nir_address_format_32bit_offset_as_64bit,
   .temp_addr_format = nir_address_format_32bit_offset_as_64bit,
   .float_controls_execution_mode = FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP32,
   .caps = {
      .address = true,
   .float64 = true,
   .int8 = true,
   .int16 = true,
   .int64 = true,
   .kernel = true,
   .kernel_image = true,
   .kernel_image_read_write = true,
   .literal_sampler = true,
         };
   unsigned supported_int_sizes = (16 | 32 | 64);
   unsigned supported_float_sizes = (16 | 32);
   if (conf) {
      supported_int_sizes &= ~conf->lower_bit_size;
      }
   nir_shader_compiler_options nir_options;
   dxil_get_nir_compiler_options(&nir_options,
                                 nir = spirv_to_nir(linked_spirv->data, linked_spirv->size / 4,
                     consts ? (struct nir_spirv_specialization *)consts->specializations : NULL,
   consts ? consts->num_specializations : 0,
   if (!nir) {
      clc_error(logger, "spirv_to_nir() failed");
      }
            NIR_PASS_V(nir, nir_lower_goto_ifs);
                     metadata->args = calloc(out_dxil->kernel->num_args,
         if (!metadata->args) {
      clc_error(logger, "failed to allocate arg positions");
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
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
                  // Inline all functions first.
   // according to the comment on nir_inline_functions
   NIR_PASS_V(nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS_V(nir, nir_lower_returns);
   NIR_PASS_V(nir, nir_link_shader_functions, clc_libclc_get_clc_shader(lib));
            // Pick off the single entrypoint that we want.
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
   NIR_PASS(progress, nir, nir_split_var_copies);
   NIR_PASS(progress, nir, nir_lower_var_copies);
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_opt_if, nir_opt_if_aggressive_last_continue | nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, nir, nir_opt_dead_cf);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, nir, nir_lower_vec3_to_vec4, nir_var_mem_generic | nir_var_uniform);
                           dxil_wrap_sampler_state int_sampler_states[PIPE_MAX_SHADER_SAMPLER_VIEWS] = { {{0}} };
                     // Ensure the printf struct has explicit types, but we'll throw away the scratch size, because we haven't
   // necessarily removed all temp variables (e.g. the printf struct itself) at this point, so we'll rerun this later
   assert(nir->scratch_size == 0);
            nir_lower_printf_options printf_options = {
      .treat_doubles_as_floats = true,
      };
            metadata->printf.info_count = nir->printf_info_count;
   metadata->printf.infos = calloc(nir->printf_info_count, sizeof(struct clc_printf_info));
   for (unsigned i = 0; i < nir->printf_info_count; i++) {
      metadata->printf.infos[i].str = malloc(nir->printf_info[i].string_size);
   memcpy(metadata->printf.infos[i].str, nir->printf_info[i].strings, nir->printf_info[i].string_size);
   metadata->printf.infos[i].num_args = nir->printf_info[i].num_args;
   metadata->printf.infos[i].arg_sizes = malloc(nir->printf_info[i].num_args * sizeof(unsigned));
               // For uniforms (kernel inputs, minus images), run this before adjusting variable list via image/sampler lowering
            // Calculate input offsets/metadata.
   unsigned uav_id = 0;
   nir_foreach_variable_with_modes(var, nir, nir_var_uniform) {
      int i = var->data.location;
   if (i < 0)
                     metadata->args[i].offset = var->data.driver_location;
   metadata->args[i].size = size;
   metadata->kernel_inputs_buf_size = MAX2(metadata->kernel_inputs_buf_size,
         if (out_dxil->kernel->args[i].address_qualifier == CLC_KERNEL_ARG_ADDRESS_GLOBAL ||
      out_dxil->kernel->args[i].address_qualifier == CLC_KERNEL_ARG_ADDRESS_CONSTANT) {
      } else if (glsl_type_is_sampler(var->type)) {
      unsigned address_mode = conf ? conf->args[i].sampler.addressing_mode : 0u;
   int_sampler_states[sampler_id].wrap[0] =
      int_sampler_states[sampler_id].wrap[1] =
      int_sampler_states[sampler_id].is_nonnormalized_coords =
         int_sampler_states[sampler_id].is_linear_filtering =
                                 // Second pass over inputs to calculate image bindings
   unsigned srv_id = 0;
   nir_foreach_image_variable(var, nir) {
      int i = var->data.location;
   if (i < 0)
                     if (var->data.access == ACCESS_NON_WRITEABLE) {
         } else {
      // Write or read-write are UAVs
               metadata->args[i].image.num_buf_ids = 1;
            // Assign location that'll be used for uniforms for format/order
   var->data.driver_location = metadata->kernel_inputs_buf_size;
   metadata->args[i].offset = metadata->kernel_inputs_buf_size;
   metadata->args[i].size = 8;
               // Before removing dead uniforms, dedupe inline samplers to make more dead uniforms
   NIR_PASS_V(nir, nir_dedup_inline_samplers);
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_uniform | nir_var_mem_ubo |
            // Fill out inline sampler metadata, now that they've been deduped and dead ones removed
   nir_foreach_variable_with_modes(var, nir, nir_var_uniform) {
      if (glsl_type_is_sampler(var->type) && var->data.sampler.is_inline_sampler) {
      int_sampler_states[sampler_id].wrap[0] =
      int_sampler_states[sampler_id].wrap[1] =
   int_sampler_states[sampler_id].wrap[2] =
      int_sampler_states[sampler_id].is_nonnormalized_coords =
         int_sampler_states[sampler_id].is_linear_filtering =
                  assert(metadata->num_const_samplers < CLC_MAX_SAMPLERS);
   metadata->const_samplers[metadata->num_const_samplers].sampler_id = var->data.binding;
   metadata->const_samplers[metadata->num_const_samplers].addressing_mode = var->data.sampler.addressing_mode;
   metadata->const_samplers[metadata->num_const_samplers].normalized_coords = var->data.sampler.normalized_coordinates;
   metadata->const_samplers[metadata->num_const_samplers].filter_mode = var->data.sampler.filter_mode;
                  // Needs to come before lower_explicit_io
   NIR_PASS_V(nir, nir_lower_readonly_images_to_tex, false);
   struct clc_image_lower_context image_lower_context = { metadata, &srv_id, &uav_id };
   NIR_PASS_V(nir, clc_lower_images, &image_lower_context);
   NIR_PASS_V(nir, clc_lower_nonnormalized_samplers, int_sampler_states);
   NIR_PASS_V(nir, nir_lower_samplers);
   NIR_PASS_V(nir, dxil_lower_sample_to_txf_for_integer_tex,
                     nir->scratch_size = 0;
   NIR_PASS_V(nir, nir_lower_vars_to_explicit_types,
                  // Lower memcpy - needs to wait until types are sized
   {
      bool progress;
   do {
      progress = false;
   NIR_PASS(progress, nir, nir_opt_memcpy);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
   NIR_PASS(progress, nir, nir_opt_deref);
   NIR_PASS(progress, nir, nir_opt_dce);
   NIR_PASS(progress, nir, nir_split_var_copies);
   NIR_PASS(progress, nir, nir_lower_var_copies);
   NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
         }
            // Attempt to preserve derefs to constants by moving them to shader_temp
   NIR_PASS_V(nir, dxil_nir_lower_constant_to_temp);
   // While inserting new var derefs for our "logical" addressing mode, temporarily
   // switch the pointer size to 32-bit.
   nir->info.cs.ptr_size = 32;
   NIR_PASS_V(nir, nir_split_struct_vars, nir_var_shader_temp);
   NIR_PASS_V(nir, dxil_nir_flatten_var_arrays, nir_var_shader_temp);
   NIR_PASS_V(nir, dxil_nir_lower_var_bit_size, nir_var_shader_temp,
                  NIR_PASS_V(nir, clc_lower_constant_to_ssbo, out_dxil->kernel, &uav_id);
   NIR_PASS_V(nir, clc_change_variable_mode, nir_var_shader_temp, nir_var_mem_constant);
            bool has_printf = false;
   NIR_PASS(has_printf, nir, clc_lower_printf_base, uav_id);
                              assert(nir->info.cs.ptr_size == 64);
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ssbo,
         NIR_PASS_V(nir, nir_lower_explicit_io,
                           nir_lower_compute_system_values_options compute_options = {
      .has_base_global_invocation_id = (conf && conf->support_global_work_id_offsets),
      };
                     NIR_PASS_V(nir, nir_opt_deref);
                     nir_variable *inputs_var =
         nir_variable *work_properties_var =
            memcpy(metadata->local_size, nir->info.workgroup_size,
         memcpy(metadata->local_size_hint, nir->info.cs.workgroup_size_hint,
            // Patch the localsize before calling clc_nir_lower_system_values().
   if (conf) {
      for (unsigned i = 0; i < ARRAY_SIZE(nir->info.workgroup_size); i++) {
      if (!conf->local_size[i] ||
                  if (nir->info.workgroup_size[i] &&
      nir->info.workgroup_size[i] != conf->local_size[i]) {
   debug_printf("D3D12: runtime local size does not match reqd_work_group_size() values\n");
                  }
   memcpy(metadata->local_size, nir->info.workgroup_size,
      } else {
      /* Make sure there's at least one thread that's set to run */
   for (unsigned i = 0; i < ARRAY_SIZE(nir->info.workgroup_size); i++) {
      if (nir->info.workgroup_size[i] == 0)
                  NIR_PASS_V(nir, clc_nir_lower_kernel_input_loads, inputs_var);
   NIR_PASS_V(nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS_V(nir, clc_nir_lower_system_values, work_properties_var);
   const struct dxil_nir_lower_loads_stores_options loads_stores_options = {
         };
      /* Now that function-declared local vars have been sized, append args */
   for (unsigned i = 0; i < out_dxil->kernel->num_args; i++) {
      if (out_dxil->kernel->args[i].address_qualifier != CLC_KERNEL_ARG_ADDRESS_LOCAL)
            /* If we don't have the runtime conf yet, we just create a dummy variable.
   * This will be adjusted when clc_spirv_to_dxil() is called with a conf
   * argument.
   */
   unsigned size = 4;
   if (conf && conf->args)
            /* The alignment required for the pointee type is not easy to get from
   * here, so let's base our logic on the size itself. Anything bigger than
   * the maximum alignment constraint (which is 128 bytes, since ulong16 or
   * doubl16 size are the biggest base types) should be aligned on this
   * maximum alignment constraint. For smaller types, we use the size
   * itself to calculate the alignment.
   */
            nir->info.shared_size = align(nir->info.shared_size, alignment);
   metadata->args[i].localptr.sharedmem_offset = nir->info.shared_size;
               NIR_PASS_V(nir, dxil_nir_lower_loads_stores_to_dxil, &loads_stores_options);
   NIR_PASS_V(nir, dxil_nir_opt_alu_deref_srcs);
   NIR_PASS_V(nir, nir_lower_fp16_casts, nir_lower_fp16_all);
            // Convert pack to pack_split
   NIR_PASS_V(nir, nir_lower_pack);
   // Lower pack_split to bit math
                     nir_validate_shader(nir, "Validate before feeding NIR to the DXIL compiler");
   struct nir_to_dxil_options opts = {
      .interpolate_at_vertex = false,
   .lower_int16 = (conf && (conf->lower_bit_size & 16) != 0),
   .disable_math_refactoring = true,
   .num_kernel_globals = num_global_inputs,
   .environment = DXIL_ENVIRONMENT_CL,
   .shader_model_max = conf && conf->max_shader_model ? conf->max_shader_model : SHADER_MODEL_6_2,
               metadata->local_mem_size = nir->info.shared_size;
            /* DXIL double math is too limited compared to what NIR expects. Let's refuse
   * to compile a shader when it contains double operations until we have
   * double lowering hooked up.
   */
   if (shader_has_double(nir)) {
      clc_error(logger, "NIR shader contains doubles, which we don't support yet");
               struct dxil_logger dxil_logger = { .priv = logger ? logger->priv : NULL,
            struct blob tmp;
   if (!nir_to_dxil(nir, &opts, logger ? &dxil_logger : NULL, &tmp)) {
      debug_printf("D3D12: nir_to_dxil failed\n");
               nir_foreach_variable_with_modes(var, nir, nir_var_mem_ssbo) {
      if (var->constant_initializer) {
      if (glsl_type_is_array(var->type)) {
      int size = align(glsl_get_cl_size(var->type), 4);
   uint8_t *data = malloc(size);
                  copy_const_initializer(var->constant_initializer, var->type, data);
   metadata->consts[metadata->num_consts].data = data;
   metadata->consts[metadata->num_consts].size = size;
   metadata->consts[metadata->num_consts].uav_id = var->data.binding;
      } else
                  metadata->kernel_inputs_cbv_id = inputs_var ? inputs_var->data.binding : 0;
   metadata->work_properties_cbv_id = work_properties_var->data.binding;
   metadata->num_uavs = uav_id;
   metadata->num_srvs = srv_id;
            ralloc_free(nir);
            blob_finish_get_buffer(&tmp, &out_dxil->binary.data,
               err_free_dxil:
      clc_free_dxil_object(out_dxil);
      }
      void clc_free_dxil_object(struct clc_dxil_object *dxil)
   {
      for (unsigned i = 0; i < dxil->metadata.num_consts; i++)
            for (unsigned i = 0; i < dxil->metadata.printf.info_count; i++) {
      free(dxil->metadata.printf.infos[i].arg_sizes);
      }
               }
      uint64_t clc_compiler_get_version(void)
   {
      const char sha1[] = MESA_GIT_SHA1;
   const char* dash = strchr(sha1, '-');
   if (dash) {
         }
      }
