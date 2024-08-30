   /*
   * Copyright © 2019 Intel Corporation
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
      #include "util/u_dynarray.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      /** @file nir_lower_io_to_vector.c
   *
   * Merges compatible input/output variables residing in different components
   * of the same location. It's expected that further passes such as
   * nir_lower_io_to_temporaries will combine loads and stores of the merged
   * variables, producing vector nir_load_input/nir_store_output instructions
   * when all is said and done.
   */
      /* FRAG_RESULT_MAX+1 instead of just FRAG_RESULT_MAX because of how this pass
   * handles dual source blending */
   #define MAX_SLOTS MAX2(VARYING_SLOT_TESS_MAX, FRAG_RESULT_MAX + 1)
      static unsigned
   get_slot(const nir_variable *var)
   {
      /* This handling of dual-source blending might not be correct when more than
   * one render target is supported, but it seems no driver supports more than
   * one. */
      }
      static const struct glsl_type *
   get_per_vertex_type(const nir_shader *shader, const nir_variable *var,
         {
      if (nir_is_arrayed_io(var, shader->info.stage)) {
      assert(glsl_type_is_array(var->type));
   if (num_vertices)
            } else {
      if (num_vertices)
               }
      static const struct glsl_type *
   resize_array_vec_type(const struct glsl_type *type, unsigned num_components)
   {
      if (glsl_type_is_array(type)) {
      const struct glsl_type *arr_elem =
            } else {
      assert(glsl_type_is_vector_or_scalar(type));
         }
      static bool
   variables_can_merge(const nir_shader *shader,
               {
      if (a->data.compact || b->data.compact)
            if (a->data.per_view || b->data.per_view)
            const struct glsl_type *a_type_tail = a->type;
            if (nir_is_arrayed_io(a, shader->info.stage) !=
      nir_is_arrayed_io(b, shader->info.stage))
         /* They must have the same array structure */
   if (same_array_structure) {
      while (glsl_type_is_array(a_type_tail)) {
                                    a_type_tail = glsl_get_array_element(a_type_tail);
      }
   if (glsl_type_is_array(b_type_tail))
      } else {
      a_type_tail = glsl_without_array(a_type_tail);
               if (!glsl_type_is_vector_or_scalar(a_type_tail) ||
      !glsl_type_is_vector_or_scalar(b_type_tail))
         if (glsl_get_base_type(a_type_tail) != glsl_get_base_type(b_type_tail))
            /* TODO: add 64/16bit support ? */
   if (glsl_get_bit_size(a_type_tail) != 32)
            assert(a->data.mode == b->data.mode);
   if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      a->data.mode == nir_var_shader_in &&
   (a->data.interpolation != b->data.interpolation ||
   a->data.centroid != b->data.centroid ||
   a->data.sample != b->data.sample))
         if (shader->info.stage == MESA_SHADER_FRAGMENT &&
      a->data.mode == nir_var_shader_out &&
   a->data.index != b->data.index)
         /* It's tricky to merge XFB-outputs correctly, because we need there
   * to not be any overlaps when we get to
   * nir_gather_xfb_info_with_varyings later on. We'll end up
   * triggering an assert there if we merge here.
   */
   if ((shader->info.stage == MESA_SHADER_VERTEX ||
      shader->info.stage == MESA_SHADER_TESS_EVAL ||
   shader->info.stage == MESA_SHADER_GEOMETRY) &&
   a->data.mode == nir_var_shader_out &&
   (a->data.explicit_xfb_buffer || b->data.explicit_xfb_buffer))
            }
      static const struct glsl_type *
   get_flat_type(const nir_shader *shader, nir_variable *old_vars[MAX_SLOTS][4],
         {
      unsigned todo = 1;
   unsigned slots = 0;
   unsigned num_vars = 0;
   enum glsl_base_type base = GLSL_TYPE_ERROR;
   *num_vertices = 0;
            while (todo) {
      assert(*loc < MAX_SLOTS);
   for (unsigned frac = 0; frac < 4; frac++) {
      nir_variable *var = old_vars[*loc][frac];
   if (!var)
         if ((*first_var &&
      !variables_can_merge(shader, var, *first_var, false)) ||
   var->data.compact) {
   (*loc)++;
               if (!*first_var) {
      if (!glsl_type_is_vector_or_scalar(glsl_without_array(var->type))) {
      (*loc)++;
      }
   *first_var = var;
   base = glsl_get_base_type(
               bool vs_in = shader->info.stage == MESA_SHADER_VERTEX &&
         unsigned var_slots = glsl_count_attribute_slots(
         todo = MAX2(todo, var_slots);
      }
   todo--;
   slots++;
               if (num_vars <= 1)
            if (slots == 1)
         else
      }
      static bool
   create_new_io_vars(nir_shader *shader, nir_variable_mode mode,
                     {
               bool has_io_var = false;
   nir_foreach_variable_with_modes(var, shader, mode) {
      unsigned frac = var->data.location_frac;
   old_vars[get_slot(var)][frac] = var;
               if (!has_io_var)
                     for (unsigned loc = 0; loc < MAX_SLOTS; loc++) {
      unsigned frac = 0;
   while (frac < 4) {
      nir_variable *first_var = old_vars[loc][frac];
   if (!first_var) {
      frac++;
                              while (frac < 4) {
      nir_variable *var = old_vars[loc][frac];
                  if (var != first_var) {
                                 const unsigned num_components =
         if (!num_components) {
      assert(frac == 0);
                     /* We had better not have any overlapping vars */
                                                      nir_variable *var = nir_variable_clone(old_vars[loc][first], shader);
                  nir_shader_add_variable(shader, var);
   for (unsigned i = first; i < frac; i++) {
      new_vars[loc][i] = var;
   if (old_vars[loc][i]) {
      util_dynarray_append(demote_vars, nir_variable *, old_vars[loc][i]);
                                 /* "flat" mode: tries to ensure there is at most one variable per slot by
   * merging variables into vec4s
   */
   for (unsigned loc = 0; loc < MAX_SLOTS;) {
      nir_variable *first_var;
   unsigned num_vertices;
   unsigned new_loc = loc;
   const struct glsl_type *flat_type =
         if (flat_type) {
               nir_variable *var = nir_variable_clone(first_var, shader);
   var->data.location_frac = 0;
   if (num_vertices)
                        nir_shader_add_variable(shader, var);
   unsigned num_slots = MAX2(glsl_get_length(flat_type), 1);
   for (unsigned i = 0; i < num_slots; i++) {
      for (unsigned j = 0; j < 4; j++)
               }
                  }
      static nir_deref_instr *
   build_array_deref_of_new_var(nir_builder *b, nir_variable *new_var,
         {
      if (leader->deref_type == nir_deref_type_var)
            nir_deref_instr *parent =
               }
      static nir_def *
   build_array_index(nir_builder *b, nir_deref_instr *deref, nir_def *base,
         {
      switch (deref->deref_type) {
   case nir_deref_type_var:
         case nir_deref_type_array: {
      nir_def *index = nir_i2iN(b, deref->arr.index.ssa,
            if (nir_deref_instr_parent(deref)->deref_type == nir_deref_type_var &&
                  return nir_iadd(
      b, build_array_index(b, nir_deref_instr_parent(deref), base, vs_in, per_vertex),
   }
   default:
            }
      static nir_deref_instr *
   build_array_deref_of_new_var_flat(nir_shader *shader,
               {
               bool per_vertex = nir_is_arrayed_io(new_var, shader->info.stage);
   if (per_vertex) {
      nir_deref_path path;
            assert(path.path[0]->deref_type == nir_deref_type_var);
   nir_deref_instr *p = path.path[1];
            nir_def *index = p->arr.index.ssa;
               if (!glsl_type_is_array(deref->type))
            bool vs_in = shader->info.stage == MESA_SHADER_VERTEX &&
         return nir_build_deref_array(b, deref,
      }
      ASSERTED static bool
   nir_shader_can_read_output(const shader_info *info)
   {
      switch (info->stage) {
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_FRAGMENT:
            case MESA_SHADER_TASK:
   case MESA_SHADER_MESH:
      /* TODO(mesh): This will not be allowed on EXT. */
         default:
            }
      static bool
   nir_lower_io_to_vector_impl(nir_function_impl *impl, nir_variable_mode modes)
   {
                                 struct util_dynarray demote_vars;
            nir_shader *shader = impl->function->shader;
   nir_variable *new_inputs[MAX_SLOTS][4] = { { 0 } };
   nir_variable *new_outputs[MAX_SLOTS][4] = { { 0 } };
   bool flat_inputs[MAX_SLOTS] = { 0 };
            if (modes & nir_var_shader_in) {
      /* Vertex shaders support overlapping inputs.  We don't do those */
            /* If we don't actually merge any variables, remove that bit from modes
   * so we don't bother doing extra non-work.
   */
   if (!create_new_io_vars(shader, nir_var_shader_in,
                     if (modes & nir_var_shader_out) {
      /* If we don't actually merge any variables, remove that bit from modes
   * so we don't bother doing extra non-work.
   */
   if (!create_new_io_vars(shader, nir_var_shader_out,
                     if (!modes)
                     /* Actually lower all the IO load/store intrinsics.  Load instructions are
   * lowered to a vector load and an ALU instruction to grab the channels we
   * want.  Outputs are lowered to a write-masked store of the vector output.
   * For non-TCS outputs, we then run nir_lower_io_to_temporaries at the end
   * to clean up the partial writes.
   */
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
   case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex: {
      nir_deref_instr *old_deref = nir_src_as_deref(intrin->src[0]);
                                          const unsigned loc = get_slot(old_var);
   const unsigned old_frac = old_var->data.location_frac;
   nir_variable *new_var = old_var->data.mode == nir_var_shader_in ? new_inputs[loc][old_frac] : new_outputs[loc][old_frac];
   bool flat = old_var->data.mode == nir_var_shader_in ? flat_inputs[loc] : flat_outputs[loc];
                                                   /* Rewrite the load to use the new variable and only select a
   * portion of the result.
   */
   nir_deref_instr *new_deref;
   if (flat) {
      new_deref = build_array_deref_of_new_var_flat(
      } else {
      assert(get_slot(new_var) == loc);
   new_deref = build_array_deref_of_new_var(&b, new_var, old_deref);
                     intrin->num_components =
                           nir_def *new_vec = nir_channels(&b, &intrin->def,
         nir_def_rewrite_uses_after(&intrin->def,
                  progress = true;
               case nir_intrinsic_store_deref: {
      nir_deref_instr *old_deref = nir_src_as_deref(intrin->src[0]);
                           const unsigned loc = get_slot(old_var);
   const unsigned old_frac = old_var->data.location_frac;
   nir_variable *new_var = new_outputs[loc][old_frac];
   bool flat = flat_outputs[loc];
                                    /* Rewrite the store to be a masked store to the new variable */
   nir_deref_instr *new_deref;
   if (flat) {
      new_deref = build_array_deref_of_new_var_flat(
      } else {
      assert(get_slot(new_var) == loc);
   new_deref = build_array_deref_of_new_var(&b, new_var, old_deref);
                                             nir_def *old_value = intrin->src[1].ssa;
   nir_scalar comps[4];
   for (unsigned c = 0; c < intrin->num_components; c++) {
      if (new_frac + c >= old_frac &&
      (old_wrmask & 1 << (new_frac + c - old_frac))) {
   comps[c] = nir_get_scalar(old_value,
      } else {
      comps[c] = nir_get_scalar(nir_undef(&b, old_value->num_components,
               }
                                 progress = true;
               default:
                        /* Demote the old var to a global, so that things like
   * nir_lower_io_to_temporaries() don't trigger on it.
   */
   util_dynarray_foreach(&demote_vars, nir_variable *, varp) {
         }
   nir_fixup_deref_modes(b.shader);
            if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
                  }
      bool
   nir_lower_io_to_vector(nir_shader *shader, nir_variable_mode modes)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
      static bool
   nir_vectorize_tess_levels_impl(nir_function_impl *impl)
   {
      bool progress = false;
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var->data.location != VARYING_SLOT_TESS_LEVEL_OUTER &&
                  assert(deref->deref_type == nir_deref_type_array);
   assert(nir_src_is_const(deref->arr.index));
                  b.cursor = nir_before_instr(instr);
                                    /* Handle out of bounds access. */
   if (index >= vec_size) {
      if (intrin->intrinsic == nir_intrinsic_load_deref) {
      /* Return undef from out of bounds loads. */
   b.cursor = nir_after_instr(instr);
   nir_def *val = &intrin->def;
                     /* Finally, remove the out of bounds access. */
   nir_instr_remove(instr);
   progress = true;
               if (intrin->intrinsic == nir_intrinsic_store_deref) {
      nir_intrinsic_set_write_mask(intrin, 1 << index);
   nir_def *new_val = nir_undef(&b, intrin->num_components, 32);
   new_val = nir_vector_insert_imm(&b, new_val, intrin->src[1].ssa, index);
      } else {
      b.cursor = nir_after_instr(instr);
   nir_def *val = &intrin->def;
   val->num_components = intrin->num_components;
   nir_def *comp = nir_channel(&b, val, index);
                              if (progress)
         else
               }
      /* Make the tess factor variables vectors instead of compact arrays, so accesses
   * can be combined by nir_opt_cse()/nir_opt_combine_stores().
   */
   bool
   nir_vectorize_tess_levels(nir_shader *shader)
   {
               nir_foreach_shader_out_variable(var, shader) {
      if (var->data.location == VARYING_SLOT_TESS_LEVEL_OUTER ||
      var->data.location == VARYING_SLOT_TESS_LEVEL_INNER) {
   var->type = glsl_vector_type(GLSL_TYPE_FLOAT, glsl_get_length(var->type));
   var->data.compact = false;
                  nir_foreach_function_impl(impl, shader) {
                     }
