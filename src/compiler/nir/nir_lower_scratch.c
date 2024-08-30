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
      /*
   * This lowering pass converts references to variables with loads/stores to
   * scratch space based on a few configurable parameters.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      static void
   lower_load_store(nir_builder *b,
               {
               nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
            nir_def *offset =
      nir_iadd_imm(b, nir_build_deref_offset(b, deref, size_align),
         unsigned align, UNUSED size;
            if (intrin->intrinsic == nir_intrinsic_load_deref) {
      unsigned bit_size = intrin->def.bit_size;
   nir_def *value = nir_load_scratch(
         if (bit_size == 1)
               } else {
               nir_def *value = intrin->src[1].ssa;
   if (value->bit_size == 1)
            nir_store_scratch(b, value, offset, .align_mul = align,
               nir_instr_remove(&intrin->instr);
      }
      static bool
   only_used_for_load_store(nir_deref_instr *deref)
   {
      nir_foreach_use(src, &deref->def) {
      if (!nir_src_parent_instr(src))
         if (nir_src_parent_instr(src)->type == nir_instr_type_deref) {
      if (!only_used_for_load_store(nir_instr_as_deref(nir_src_parent_instr(src))))
      } else if (nir_src_parent_instr(src)->type != nir_instr_type_intrinsic) {
         } else {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
      intrin->intrinsic != nir_intrinsic_store_deref)
      }
      }
      bool
   nir_lower_vars_to_scratch(nir_shader *shader,
                     {
               /* First, we walk the instructions and flag any variables we want to lower
   * by removing them from their respective list and setting the mode to 0.
   */
   nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                                 nir_variable *var = nir_deref_instr_get_variable(deref);
                  /* We set var->mode to 0 to indicate that a variable will be moved
   * to scratch.  Don't assign a scratch location twice.
   */
                  unsigned var_size, var_align;
   size_align(var->type, &var_size, &var_align);
                                    if (set->entries == 0) {
      _mesa_set_destroy(set, NULL);
                        nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              if (nir_deref_instr_remove_if_unused(deref)) {
                                       struct set_entry *entry = _mesa_set_search(set, deref->var);
                  if (!only_used_for_load_store(deref))
                     set_foreach(set, entry) {
               /* Remove it from its list */
   exec_node_remove(&var->node);
   /* Invalid mode used to flag "moving to scratch" */
            /* We don't allocate space here as iteration in this loop is
   * non-deterministic due to the nir_variable pointers. */
               nir_foreach_function_impl(impl, shader) {
               bool impl_progress = false;
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_variable *var = nir_intrinsic_get_var(intrin, 0);
   /* Variables flagged for lowering above have mode == 0 */
                  if (var->data.location == INT_MAX) {
                                       lower_load_store(&build, intrin, size_align);
                  if (impl_progress) {
      progress = true;
   nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                                 }
