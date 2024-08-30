   /*
   * Copyright © 2016 Broadcom
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
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      /** @file nir_lower_io_to_scalar.c
   *
   * Replaces nir_load_input/nir_store_output operations with num_components !=
   * 1 with individual per-channel operations.
   */
      static void
   set_io_semantics(nir_intrinsic_instr *scalar_intr,
         {
      nir_io_semantics sem = nir_intrinsic_io_semantics(vec_intr);
   sem.gs_streams = (sem.gs_streams >> (component * 2)) & 0x3;
      }
      static void
   lower_load_input_to_scalar(nir_builder *b, nir_intrinsic_instr *intr)
   {
                        for (unsigned i = 0; i < intr->num_components; i++) {
      bool is_64bit = (nir_intrinsic_instr_dest_type(intr) & NIR_ALU_TYPE_SIZE_MASK) == 64;
   unsigned newi = is_64bit ? i * 2 : i;
   unsigned newc = nir_intrinsic_component(intr);
   nir_intrinsic_instr *chan_intr =
         nir_def_init(&chan_intr->instr, &chan_intr->def, 1,
                  nir_intrinsic_set_base(chan_intr, nir_intrinsic_base(intr));
   nir_intrinsic_set_component(chan_intr, (newc + newi) % 4);
   nir_intrinsic_set_dest_type(chan_intr, nir_intrinsic_dest_type(intr));
   set_io_semantics(chan_intr, intr, i);
   /* offset and vertex (if needed) */
   for (unsigned j = 0; j < nir_intrinsic_infos[intr->intrinsic].num_srcs; ++j)
         if (newc + newi > 3) {
      nir_src *src = nir_get_io_offset_src(chan_intr);
   nir_def *offset = nir_iadd_imm(b, src->ssa, (newc + newi) / 4);
                                    nir_def_rewrite_uses(&intr->def,
            }
      static void
   lower_load_to_scalar(nir_builder *b, nir_intrinsic_instr *intr)
   {
               nir_def *loads[NIR_MAX_VEC_COMPONENTS];
            for (unsigned i = 0; i < intr->num_components; i++) {
      nir_intrinsic_instr *chan_intr =
         nir_def_init(&chan_intr->instr, &chan_intr->def, 1,
                  nir_intrinsic_set_align_offset(chan_intr,
                     nir_intrinsic_set_align_mul(chan_intr, nir_intrinsic_align_mul(intr));
   if (nir_intrinsic_has_access(intr))
         if (nir_intrinsic_has_range(intr))
         if (nir_intrinsic_has_range_base(intr))
         if (nir_intrinsic_has_base(intr))
         for (unsigned j = 0; j < nir_intrinsic_infos[intr->intrinsic].num_srcs - 1; j++)
            /* increment offset per component */
   nir_def *offset = nir_iadd_imm(b, base_offset, i * (intr->def.bit_size / 8));
                                 nir_def_rewrite_uses(&intr->def,
            }
      static void
   lower_store_output_to_scalar(nir_builder *b, nir_intrinsic_instr *intr)
   {
                        for (unsigned i = 0; i < intr->num_components; i++) {
      if (!(nir_intrinsic_write_mask(intr) & (1 << i)))
            bool is_64bit = (nir_intrinsic_instr_src_type(intr, 0) & NIR_ALU_TYPE_SIZE_MASK) == 64;
   unsigned newi = is_64bit ? i * 2 : i;
   unsigned newc = nir_intrinsic_component(intr);
   nir_intrinsic_instr *chan_intr =
                  nir_intrinsic_set_base(chan_intr, nir_intrinsic_base(intr));
   nir_intrinsic_set_write_mask(chan_intr, 0x1);
   nir_intrinsic_set_component(chan_intr, (newc + newi) % 4);
   nir_intrinsic_set_src_type(chan_intr, nir_intrinsic_src_type(intr));
            if (nir_intrinsic_has_io_xfb(intr)) {
                                                         memset(&scalar_xfb, 0, sizeof(scalar_xfb));
   scalar_xfb.out[component % 2].num_components = is_64bit ? 2 : 1;
   scalar_xfb.out[component % 2].buffer = xfb.out[c % 2].buffer;
   scalar_xfb.out[component % 2].offset = xfb.out[c % 2].offset +
         if (component < 2)
         else
                           /* value */
   chan_intr->src[0] = nir_src_for_ssa(nir_channel(b, value, i));
   /* offset and vertex (if needed) */
   for (unsigned j = 1; j < nir_intrinsic_infos[intr->intrinsic].num_srcs; ++j)
         if (newc + newi > 3) {
      nir_src *src = nir_get_io_offset_src(chan_intr);
   nir_def *offset = nir_iadd_imm(b, src->ssa, (newc + newi) / 4);
                              }
      static void
   lower_store_to_scalar(nir_builder *b, nir_intrinsic_instr *intr)
   {
               nir_def *value = intr->src[0].ssa;
            /* iterate wrmask instead of num_components to handle split components */
   u_foreach_bit(i, nir_intrinsic_write_mask(intr)) {
      nir_intrinsic_instr *chan_intr =
                  nir_intrinsic_set_write_mask(chan_intr, 0x1);
   nir_intrinsic_set_align_offset(chan_intr,
                     nir_intrinsic_set_align_mul(chan_intr, nir_intrinsic_align_mul(intr));
   if (nir_intrinsic_has_access(intr))
         if (nir_intrinsic_has_base(intr))
            /* value */
   chan_intr->src[0] = nir_src_for_ssa(nir_channel(b, value, i));
   for (unsigned j = 1; j < nir_intrinsic_infos[intr->intrinsic].num_srcs - 1; j++)
            /* increment offset per component */
   nir_def *offset = nir_iadd_imm(b, base_offset, i * (value->bit_size / 8));
                           }
      struct scalarize_state {
      nir_variable_mode mask;
   nir_instr_filter_cb filter;
      };
      static bool
   nir_lower_io_to_scalar_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->num_components == 1)
            if ((intr->intrinsic == nir_intrinsic_load_input ||
      intr->intrinsic == nir_intrinsic_load_per_vertex_input ||
   intr->intrinsic == nir_intrinsic_load_interpolated_input) &&
   (state->mask & nir_var_shader_in) &&
   (!state->filter || state->filter(instr, state->filter_data))) {
   lower_load_input_to_scalar(b, intr);
               if ((intr->intrinsic == nir_intrinsic_load_output ||
      intr->intrinsic == nir_intrinsic_load_per_vertex_output) &&
   (state->mask & nir_var_shader_out) &&
   (!state->filter || state->filter(instr, state->filter_data))) {
   lower_load_input_to_scalar(b, intr);
               if (((intr->intrinsic == nir_intrinsic_load_ubo && (state->mask & nir_var_mem_ubo)) ||
      (intr->intrinsic == nir_intrinsic_load_ssbo && (state->mask & nir_var_mem_ssbo)) ||
   (intr->intrinsic == nir_intrinsic_load_global && (state->mask & nir_var_mem_global)) ||
   (intr->intrinsic == nir_intrinsic_load_shared && (state->mask & nir_var_mem_shared))) &&
   (!state->filter || state->filter(instr, state->filter_data))) {
   lower_load_to_scalar(b, intr);
               if ((intr->intrinsic == nir_intrinsic_store_output ||
      intr->intrinsic == nir_intrinsic_store_per_vertex_output) &&
   state->mask & nir_var_shader_out &&
   (!state->filter || state->filter(instr, state->filter_data))) {
   lower_store_output_to_scalar(b, intr);
               if (((intr->intrinsic == nir_intrinsic_store_ssbo && (state->mask & nir_var_mem_ssbo)) ||
      (intr->intrinsic == nir_intrinsic_store_global && (state->mask & nir_var_mem_global)) ||
   (intr->intrinsic == nir_intrinsic_store_shared && (state->mask & nir_var_mem_shared))) &&
   (!state->filter || state->filter(instr, state->filter_data))) {
   lower_store_to_scalar(b, intr);
                  }
      bool
   nir_lower_io_to_scalar(nir_shader *shader, nir_variable_mode mask, nir_instr_filter_cb filter, void *filter_data)
   {
      struct scalarize_state state = {
      mask,
   filter,
      };
   return nir_shader_instructions_pass(shader,
                        }
      static nir_variable **
   get_channel_variables(struct hash_table *ht, nir_variable *var)
   {
      nir_variable **chan_vars;
   struct hash_entry *entry = _mesa_hash_table_search(ht, var);
   if (!entry) {
      chan_vars = (nir_variable **)calloc(4, sizeof(nir_variable *));
      } else {
                     }
      /*
   * Note that the src deref that we are cloning is the head of the
   * chain of deref instructions from the original intrinsic, but
   * the dst we are cloning to is the tail (because chains of deref
   * instructions are created back to front)
   */
      static nir_deref_instr *
   clone_deref_array(nir_builder *b, nir_deref_instr *dst_tail,
         {
               if (!parent)
                              return nir_build_deref_array(b, dst_tail,
      }
      static void
   lower_load_to_scalar_early(nir_builder *b, nir_intrinsic_instr *intr,
               {
                        nir_variable **chan_vars;
   if (var->data.mode == nir_var_shader_in) {
         } else {
                  for (unsigned i = 0; i < intr->num_components; i++) {
      nir_variable *chan_var = chan_vars[var->data.location_frac + i];
   if (!chan_vars[var->data.location_frac + i]) {
      chan_var = nir_variable_clone(var, b->shader);
                                       nir_intrinsic_instr *chan_intr =
         nir_def_init(&chan_intr->instr, &chan_intr->def, 1,
                                             if (intr->intrinsic == nir_intrinsic_interp_deref_at_offset ||
      intr->intrinsic == nir_intrinsic_interp_deref_at_sample ||
                                    nir_def_rewrite_uses(&intr->def,
            /* Remove the old load intrinsic */
      }
      static void
   lower_store_output_to_scalar_early(nir_builder *b, nir_intrinsic_instr *intr,
               {
                        nir_variable **chan_vars = get_channel_variables(split_outputs, var);
   for (unsigned i = 0; i < intr->num_components; i++) {
      if (!(nir_intrinsic_write_mask(intr) & (1 << i)))
            nir_variable *chan_var = chan_vars[var->data.location_frac + i];
   if (!chan_vars[var->data.location_frac + i]) {
      chan_var = nir_variable_clone(var, b->shader);
                                       nir_intrinsic_instr *chan_intr =
                                             chan_intr->src[0] = nir_src_for_ssa(&deref->def);
                        /* Remove the old store intrinsic */
      }
      struct io_to_scalar_early_state {
      struct hash_table *split_inputs, *split_outputs;
      };
      static bool
   nir_lower_io_to_scalar_early_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intr->num_components == 1)
            if (intr->intrinsic != nir_intrinsic_load_deref &&
      intr->intrinsic != nir_intrinsic_store_deref &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_sample &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_offset &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_vertex)
         nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
   if (!nir_deref_mode_is_one_of(deref, state->mask))
            nir_variable *var = nir_deref_instr_get_variable(deref);
            /* TODO: add patch support */
   if (var->data.patch)
            /* TODO: add doubles support */
   if (glsl_type_is_64bit(glsl_without_array(var->type)))
            if (!(b->shader->info.stage == MESA_SHADER_VERTEX &&
            var->data.location < VARYING_SLOT_VAR0 &&
   var->data.location >= 0)
         /* Don't bother splitting if we can't opt away any unused
   * components.
   */
   if (var->data.always_active_io)
            if (var->data.must_be_shader_input)
            /* Skip types we cannot split */
   if (glsl_type_is_matrix(glsl_without_array(var->type)) ||
      glsl_type_is_struct_or_ifc(glsl_without_array(var->type)))
         switch (intr->intrinsic) {
   case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex:
   case nir_intrinsic_load_deref:
      if ((state->mask & nir_var_shader_in && mode == nir_var_shader_in) ||
      (state->mask & nir_var_shader_out && mode == nir_var_shader_out)) {
   lower_load_to_scalar_early(b, intr, var, state->split_inputs,
            }
      case nir_intrinsic_store_deref:
      if (state->mask & nir_var_shader_out &&
      mode == nir_var_shader_out) {
   lower_store_output_to_scalar_early(b, intr, var, state->split_outputs);
      }
      default:
                     }
      /*
   * This function is intended to be called earlier than nir_lower_io_to_scalar()
   * i.e. before nir_lower_io() is called.
   */
   bool
   nir_lower_io_to_scalar_early(nir_shader *shader, nir_variable_mode mask)
   {
      struct io_to_scalar_early_state state = {
      .split_inputs = _mesa_pointer_hash_table_create(NULL),
   .split_outputs = _mesa_pointer_hash_table_create(NULL),
               bool progress = nir_shader_instructions_pass(shader,
                              /* Remove old input from the shaders inputs list */
   hash_table_foreach(state.split_inputs, entry) {
      nir_variable *var = (nir_variable *)entry->key;
                        /* Remove old output from the shaders outputs list */
   hash_table_foreach(state.split_outputs, entry) {
      nir_variable *var = (nir_variable *)entry->key;
                        _mesa_hash_table_destroy(state.split_inputs, NULL);
                        }
