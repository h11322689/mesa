   /*
   * Copyright © 2020 Google LLC
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
      /**
   * @file
   *
   * Removes unused components of SSA defs.
   *
   * Due to various optimization passes (or frontend implementations,
   * particularly prog_to_nir), we may have instructions generating vectors
   * whose components don't get read by any instruction.
   *
   * For memory loads, while it can be tricky to eliminate unused low components
   * or channels in the middle of a writemask (you might need to increment some
   * offset from a load_uniform, for example), it is trivial to just drop the
   * trailing components.
   * For vector ALU and load_const, only used by other ALU instructions,
   * this pass eliminates arbitrary channels as well as duplicate channels,
   * and reswizzles the uses.
   *
   * This pass is probably only of use to vector backends -- scalar backends
   * typically get unused def channel trimming by scalarizing and dead code
   * elimination.
   */
      #include "util/u_math.h"
   #include "nir.h"
   #include "nir_builder.h"
      /*
   * Round up a vector size to a vector size that's valid in NIR. At present, NIR
   * supports only vec2-5, vec8, and vec16. Attempting to generate other sizes
   * will fail validation.
   */
   static unsigned
   round_up_components(unsigned n)
   {
         }
      static bool
   shrink_dest_to_read_mask(nir_def *def)
   {
      /* early out if there's nothing to do. */
   if (def->num_components == 1)
            /* don't remove any channels if used by an intrinsic */
   nir_foreach_use(use_src, def) {
      if (nir_src_parent_instr(use_src)->type == nir_instr_type_intrinsic)
               unsigned mask = nir_def_components_read(def);
            /* If nothing was read, leave it up to DCE. */
   if (!mask)
            unsigned rounded = round_up_components(last_bit);
   assert(rounded <= def->num_components);
            if (def->num_components > last_bit) {
      def->num_components = last_bit;
                  }
      static bool
   shrink_intrinsic_to_non_sparse(nir_intrinsic_instr *instr)
   {
      unsigned mask = nir_def_components_read(&instr->def);
            /* If the sparse component is used, do nothing. */
   if (last_bit == instr->def.num_components)
            instr->def.num_components -= 1;
            /* Switch to the non-sparse intrinsic. */
   switch (instr->intrinsic) {
   case nir_intrinsic_image_sparse_load:
      instr->intrinsic = nir_intrinsic_image_load;
      case nir_intrinsic_bindless_image_sparse_load:
      instr->intrinsic = nir_intrinsic_bindless_image_load;
      case nir_intrinsic_image_deref_sparse_load:
      instr->intrinsic = nir_intrinsic_image_deref_load;
      default:
                     }
      static void
   reswizzle_alu_uses(nir_def *def, uint8_t *reswizzle)
   {
      nir_foreach_use(use_src, def) {
      /* all uses must be ALU instructions */
   assert(nir_src_parent_instr(use_src)->type == nir_instr_type_alu);
            /* reswizzle ALU sources */
   for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++)
         }
      static bool
   is_only_used_by_alu(nir_def *def)
   {
      nir_foreach_use(use_src, def) {
      if (nir_src_parent_instr(use_src)->type != nir_instr_type_alu)
                  }
      static bool
   opt_shrink_vector(nir_builder *b, nir_alu_instr *instr)
   {
      nir_def *def = &instr->def;
            /* If nothing was read, leave it up to DCE. */
   if (mask == 0)
            /* don't remove any channels if used by non-ALU */
   if (!is_only_used_by_alu(def))
            uint8_t reswizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   nir_scalar srcs[NIR_MAX_VEC_COMPONENTS] = { 0 };
   unsigned num_components = 0;
   for (unsigned i = 0; i < def->num_components; i++) {
      if (!((mask >> i) & 0x1))
                     /* Try reuse a component with the same value */
   unsigned j;
   for (j = 0; j < num_components; j++) {
      if (nir_scalar_equal(scalar, srcs[j])) {
      reswizzle[i] = j;
                  /* Otherwise, just append the value */
   if (j == num_components) {
      srcs[num_components] = scalar;
                  /* return if no component was removed */
   if (num_components == def->num_components)
            /* create new vecN and replace uses */
   nir_def *new_vec = nir_vec_scalars(b, srcs, num_components);
   nir_def_rewrite_uses(def, new_vec);
               }
      static bool
   opt_shrink_vectors_alu(nir_builder *b, nir_alu_instr *instr)
   {
               /* Nothing to shrink */
   if (def->num_components == 1)
            switch (instr->op) {
   /* don't use nir_op_is_vec() as not all vector sizes are supported. */
   case nir_op_vec4:
   case nir_op_vec3:
   case nir_op_vec2:
         default:
      if (nir_op_infos[instr->op].output_size != 0)
                     /* don't remove any channels if used by non-ALU */
   if (!is_only_used_by_alu(def))
            unsigned mask = nir_def_components_read(def);
   /* return, if there is nothing to do */
   if (mask == 0)
            uint8_t reswizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   unsigned num_components = 0;
   bool progress = false;
   for (unsigned i = 0; i < def->num_components; i++) {
      /* skip unused components */
   if (!((mask >> i) & 0x1))
            /* Try reuse a component with the same swizzles */
   unsigned j;
   for (j = 0; j < num_components; j++) {
      bool duplicate_channel = true;
   for (unsigned k = 0; k < nir_op_infos[instr->op].num_inputs; k++) {
      if (nir_op_infos[instr->op].input_sizes[k] != 0 ||
      instr->src[k].swizzle[i] != instr->src[k].swizzle[j]) {
   duplicate_channel = false;
                  if (duplicate_channel) {
      reswizzle[i] = j;
   progress = true;
                  /* Otherwise, just append the value */
   if (j == num_components) {
      for (int k = 0; k < nir_op_infos[instr->op].num_inputs; k++) {
         }
   if (i != num_components)
                        /* update uses */
   if (progress)
            unsigned rounded = round_up_components(num_components);
   assert(rounded <= def->num_components);
   if (rounded < def->num_components)
            /* update dest */
               }
      static bool
   opt_shrink_vectors_intrinsic(nir_builder *b, nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_push_constant:
   case nir_intrinsic_load_constant:
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_kernel_input:
   case nir_intrinsic_load_scratch: {
      /* Must be a vectorized intrinsic that we can resize. */
            /* Trim the dest to the used channels */
   if (!shrink_dest_to_read_mask(&instr->def))
            instr->num_components = instr->def.num_components;
      }
   case nir_intrinsic_image_sparse_load:
   case nir_intrinsic_bindless_image_sparse_load:
   case nir_intrinsic_image_deref_sparse_load:
         default:
            }
      static bool
   opt_shrink_vectors_tex(nir_builder *b, nir_tex_instr *tex)
   {
      if (!tex->is_sparse)
            unsigned mask = nir_def_components_read(&tex->def);
            /* If the sparse component is used, do nothing. */
   if (last_bit == tex->def.num_components)
            tex->def.num_components -= 1;
               }
      static bool
   opt_shrink_vectors_load_const(nir_load_const_instr *instr)
   {
               /* early out if there's nothing to do. */
   if (def->num_components == 1)
            /* don't remove any channels if used by non-ALU */
   if (!is_only_used_by_alu(def))
                     /* If nothing was read, leave it up to DCE. */
   if (!mask)
            uint8_t reswizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   unsigned num_components = 0;
   bool progress = false;
   for (unsigned i = 0; i < def->num_components; i++) {
      if (!((mask >> i) & 0x1))
            /* Try reuse a component with the same constant */
   unsigned j;
   for (j = 0; j < num_components; j++) {
      if (instr->value[i].u64 == instr->value[j].u64) {
      reswizzle[i] = j;
   progress = true;
                  /* Otherwise, just append the value */
   if (j == num_components) {
      instr->value[num_components] = instr->value[i];
   if (i != num_components)
                        if (progress)
            unsigned rounded = round_up_components(num_components);
   assert(rounded <= def->num_components);
   if (rounded < def->num_components)
                        }
      static bool
   opt_shrink_vectors_ssa_undef(nir_undef_instr *instr)
   {
         }
      static bool
   opt_shrink_vectors_phi(nir_builder *b, nir_phi_instr *instr)
   {
               /* early out if there's nothing to do. */
   if (def->num_components == 1)
            /* Ignore large vectors for now. */
   if (def->num_components > 4)
            /* Check the uses. */
   nir_component_mask_t mask = 0;
   nir_foreach_use(src, def) {
      if (nir_src_parent_instr(src)->type != nir_instr_type_alu)
                     nir_alu_src *alu_src = exec_node_data(nir_alu_src, src, src);
   int src_idx = alu_src - &alu->src[0];
                     /* We don't mark the channels used if the only reader is the original phi.
   * This can happen in the case of loops.
   */
   nir_foreach_use(alu_use_src, alu_def) {
      if (nir_src_parent_instr(alu_use_src) != &instr->instr) {
                     /* However, even if the instruction only points back at the phi, we still
   * need to check that the swizzles are trivial.
   */
   if (nir_op_is_vec(alu->op)) {
      if (src_idx != alu->src[src_idx].swizzle[0]) {
            } else if (!nir_alu_src_is_trivial_ssa(alu, src_idx)) {
                     /* DCE will handle this. */
   if (mask == 0)
            /* Nothing to shrink? */
   if (BITFIELD_MASK(def->num_components) == mask)
            /* Set up the reswizzles. */
   unsigned num_components = 0;
   uint8_t reswizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   uint8_t src_reswizzle[NIR_MAX_VEC_COMPONENTS] = { 0 };
   for (unsigned i = 0; i < def->num_components; i++) {
      if (!((mask >> i) & 0x1))
         src_reswizzle[num_components] = i;
               /* Shrink the phi, this part is simple. */
            /* We can't swizzle phi sources directly so just insert extra mov
   * with the correct swizzle and let the other parts of nir_shrink_vectors
   * do its job on the original source instruction. If the original source was
   * used only in the phi, the movs will disappear later after copy propagate.
   */
   nir_foreach_phi_src(phi_src, instr) {
               nir_alu_src alu_src = {
                  for (unsigned i = 0; i < num_components; i++)
                     }
            /* Reswizzle readers. */
               }
      static bool
   opt_shrink_vectors_instr(nir_builder *b, nir_instr *instr)
   {
               switch (instr->type) {
   case nir_instr_type_alu:
            case nir_instr_type_tex:
            case nir_instr_type_intrinsic:
            case nir_instr_type_load_const:
            case nir_instr_type_undef:
            case nir_instr_type_phi:
            default:
                     }
      bool
   nir_opt_shrink_vectors(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
               nir_foreach_block_reverse(block, impl) {
      nir_foreach_instr_reverse(instr, block) {
                     if (progress) {
      nir_metadata_preserve(impl,
            } else {
                        }
