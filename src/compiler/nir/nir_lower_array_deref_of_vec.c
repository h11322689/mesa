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
      #include "nir.h"
   #include "nir_builder.h"
      static void
   build_write_masked_store(nir_builder *b, nir_deref_instr *vec_deref,
         {
      assert(value->num_components == 1);
   unsigned num_components = glsl_get_components(vec_deref->type);
            nir_def *u = nir_undef(b, 1, value->bit_size);
   nir_def *comps[NIR_MAX_VEC_COMPONENTS];
   for (unsigned i = 0; i < num_components; i++)
            nir_def *vec = nir_vec(b, comps, num_components);
      }
      static void
   build_write_masked_stores(nir_builder *b, nir_deref_instr *vec_deref,
               {
      if (start == end - 1) {
         } else {
      unsigned mid = start + (end - start) / 2;
   nir_push_if(b, nir_ilt_imm(b, index, mid));
   build_write_masked_stores(b, vec_deref, value, index, start, mid);
   nir_push_else(b, NULL);
   build_write_masked_stores(b, vec_deref, value, index, mid, end);
         }
      static bool
   nir_lower_array_deref_of_vec_impl(nir_function_impl *impl,
               {
                        nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                    if (intrin->intrinsic != nir_intrinsic_load_deref &&
      intrin->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_sample &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_offset &&
   intrin->intrinsic != nir_intrinsic_interp_deref_at_vertex &&
                        /* We choose to be conservative here.  If the deref contains any
   * modes which weren't specified, we bail and don't bother lowering.
   */
                  /* We only care about array derefs that act on vectors */
                  nir_deref_instr *vec_deref = nir_deref_instr_parent(deref);
                  assert(intrin->num_components == 1);
                                             if (nir_src_is_const(deref->arr.index)) {
                     unsigned index = nir_src_as_uint(deref->arr.index);
   /* If index is OOB, we throw the old store away and don't
   * replace it with anything.
   */
   if (index < num_components)
      } else {
                     nir_def *index = deref->arr.index.ssa;
   build_write_masked_stores(&b, vec_deref, value, index,
                        } else {
      if (nir_src_is_const(deref->arr.index)) {
      if (!(options & nir_lower_direct_array_deref_of_vec_load))
      } else {
                        /* Turn the load into a vector load */
   nir_src_rewrite(&intrin->src[0], &vec_deref->def);
                  nir_def *index = deref->arr.index.ssa;
   nir_def *scalar =
         if (scalar->parent_instr->type == nir_instr_type_undef) {
      nir_def_rewrite_uses(&intrin->def,
            } else {
      nir_def_rewrite_uses_after(&intrin->def,
            }
                     if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      /* Lowers away array dereferences on vectors
   *
   * These are allowed on certain variable types such as SSBOs and TCS outputs.
   * However, not everyone can actually handle them everywhere.  There are also
   * cases where we want to lower them for performance reasons.
   *
   * This patch assumes that copy_deref instructions have already been lowered.
   */
   bool
   nir_lower_array_deref_of_vec(nir_shader *shader, nir_variable_mode modes,
         {
               nir_foreach_function_impl(impl, shader) {
      if (nir_lower_array_deref_of_vec_impl(impl, modes, options))
                  }
