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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "nir.h"
      static void
   add_src(nir_src *src, struct set *invariants)
   {
         }
      static bool
   add_src_cb(nir_src *src, void *state)
   {
      add_src(src, state);
      }
      static bool
   def_is_invariant(nir_def *def, struct set *invariants)
   {
         }
      static void
   add_cf_node(nir_cf_node *cf, struct set *invariants)
   {
      if (cf->type == nir_cf_node_if) {
      nir_if *if_stmt = nir_cf_node_as_if(cf);
               if (cf->parent)
      }
      static void
   add_var(nir_variable *var, struct set *invariants)
   {
      /* Because we pass the result of nir_intrinsic_get_var directly to this
   * function, it's possible for var to be NULL if, for instance, there's a
   * cast somewhere in the chain.
   */
   if (var != NULL)
      }
      static bool
   var_is_invariant(nir_variable *var, struct set *invariants)
   {
      /* Because we pass the result of nir_intrinsic_get_var directly to this
   * function, it's possible for var to be NULL if, for instance, there's a
   * cast somewhere in the chain.
   */
      }
      static void
   propagate_invariant_instr(nir_instr *instr, struct set *invariants)
   {
      switch (instr->type) {
   case nir_instr_type_alu: {
      nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (!def_is_invariant(&alu->def, invariants))
            alu->exact = true;
   nir_foreach_src(instr, add_src_cb, invariants);
               case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (def_is_invariant(&tex->def, invariants))
                     case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_copy_deref:
      /* If the destination is invariant then so is the source */
   if (var_is_invariant(nir_intrinsic_get_var(intrin, 0), invariants))
               case nir_intrinsic_load_deref:
      if (def_is_invariant(&intrin->def, invariants))
               case nir_intrinsic_store_deref:
      if (var_is_invariant(nir_intrinsic_get_var(intrin, 0), invariants))
               default:
      /* Nothing to do */
      }
               case nir_instr_type_deref:
   case nir_instr_type_jump:
   case nir_instr_type_undef:
   case nir_instr_type_load_const:
            case nir_instr_type_phi: {
      nir_phi_instr *phi = nir_instr_as_phi(instr);
   if (!def_is_invariant(&phi->def, invariants))
            nir_foreach_phi_src(src, phi) {
      add_src(&src->src, invariants);
      }
               case nir_instr_type_call:
            case nir_instr_type_parallel_copy:
   default:
            }
      static bool
   propagate_invariant_impl(nir_function_impl *impl, struct set *invariants)
   {
               while (true) {
               nir_foreach_block_reverse(block, impl) {
      nir_foreach_instr_reverse(instr, block)
               /* Keep running until we make no more progress. */
   if (invariants->entries > prev_entries) {
      progress = true;
      } else {
                     if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                     }
      /* If invariant_prim=true, this pass considers all geometry-affecting
   * outputs as invariant. Doing this works around a common class of application
   * bugs appearing as flickering.
   */
   bool
   nir_propagate_invariant(nir_shader *shader, bool invariant_prim)
   {
      /* Hash set of invariant things */
            if (shader->info.stage != MESA_SHADER_FRAGMENT && invariant_prim) {
      nir_foreach_shader_out_variable(var, shader) {
      switch (var->data.location) {
   case VARYING_SLOT_POS:
   case VARYING_SLOT_PSIZ:
   case VARYING_SLOT_CLIP_DIST0:
   case VARYING_SLOT_CLIP_DIST1:
   case VARYING_SLOT_CULL_DIST0:
   case VARYING_SLOT_CULL_DIST1:
   case VARYING_SLOT_TESS_LEVEL_OUTER:
   case VARYING_SLOT_TESS_LEVEL_INNER:
      if (!var->data.invariant)
            default:
                        bool progress = false;
   nir_foreach_function_impl(impl, shader) {
      if (propagate_invariant_impl(impl, invariants))
                           }
