   /*
   * Copyright © 2016 Intel Corporation
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
      static void
   emit_load_store_deref(nir_builder *b, nir_intrinsic_instr *orig_instr,
                        static void
   emit_indirect_load_store_deref(nir_builder *b, nir_intrinsic_instr *orig_instr,
                           {
      assert(start < end);
   if (start == end - 1) {
      emit_load_store_deref(b, orig_instr,
            } else {
                        nir_deref_instr *deref = *deref_arr;
            nir_push_if(b, nir_ilt_imm(b, deref->arr.index.ssa, mid));
   emit_indirect_load_store_deref(b, orig_instr, parent, deref_arr,
         nir_push_else(b, NULL);
   emit_indirect_load_store_deref(b, orig_instr, parent, deref_arr,
                  if (src == NULL)
         }
      static void
   emit_load_store_deref(nir_builder *b, nir_intrinsic_instr *orig_instr,
                     {
      for (; *deref_arr; deref_arr++) {
      nir_deref_instr *deref = *deref_arr;
   if (deref->deref_type == nir_deref_type_array &&
                     emit_indirect_load_store_deref(b, orig_instr, parent, deref_arr,
                                 /* We reached the end of the deref chain.  Emit the instruction */
            if (src == NULL) {
      /* This is a load instruction */
   nir_intrinsic_instr *load =
                           /* Copy over any other sources.  This is needed for interp_deref_at */
   for (unsigned i = 1;
                  nir_def_init(&load->instr, &load->def,
               nir_builder_instr_insert(b, &load->instr);
      } else {
      assert(orig_instr->intrinsic == nir_intrinsic_store_deref);
         }
      static bool
   lower_indirect_derefs_block(nir_block *block, nir_builder *b,
                     {
               nir_foreach_instr_safe(instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
      intrin->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_sample &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_offset &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_vertex &&
                        /* Walk the deref chain back to the base and look for indirects */
   uint32_t indirect_array_len = 1;
   bool has_indirect = false;
   nir_deref_instr *base = deref;
   while (base && base->deref_type != nir_deref_type_var) {
      nir_deref_instr *parent = nir_deref_instr_parent(base);
   if (base->deref_type == nir_deref_type_array &&
      !nir_src_is_const(base->arr.index)) {
   indirect_array_len *= glsl_get_length(parent->type);
                           if (!has_indirect || !base || indirect_array_len > max_lower_array_len)
            if (glsl_type_is_cmat(base->type))
            /* Only lower variables whose mode is in the mask, or compact
   * array variables.  (We can't handle indirects on tightly packed
   * scalar arrays, so we need to lower them regardless.)
   */
   if (!(modes & base->var->data.mode) && !base->var->data.compact)
            if (vars && !_mesa_set_search(vars, base->var))
                     nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
            if (intrin->intrinsic == nir_intrinsic_store_deref) {
      emit_load_store_deref(b, intrin, base, &path.path[1],
      } else {
      nir_def *result;
   emit_load_store_deref(b, intrin, base, &path.path[1],
                                             }
      static bool
   lower_indirects_impl(nir_function_impl *impl, nir_variable_mode modes,
         {
      nir_builder builder = nir_builder_create(impl);
            nir_foreach_block_safe(block, impl) {
      progress |= lower_indirect_derefs_block(block, &builder, modes, vars,
               if (progress)
         else
               }
      /** Lowers indirect variable loads/stores to direct loads/stores.
   *
   * The pass works by replacing any indirect load or store with an if-ladder
   * that does a binary search on the array index.
   */
   bool
   nir_lower_indirect_derefs(nir_shader *shader, nir_variable_mode modes,
         {
               nir_foreach_function_impl(impl, shader) {
      progress = lower_indirects_impl(impl, modes, NULL, max_lower_array_len) ||
                  }
      /** Lowers indirects on any variables in the given set */
   bool
   nir_lower_indirect_var_derefs(nir_shader *shader, const struct set *vars)
   {
               nir_foreach_function_impl(impl, shader) {
      progress = lower_indirects_impl(impl, nir_var_uniform, vars, UINT_MAX) ||
                  }
