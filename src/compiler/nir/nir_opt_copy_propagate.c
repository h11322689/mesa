   /*
   * Copyright © 2014 Intel Corporation
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
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      /**
   * SSA-based copy propagation
   */
      static bool
   is_swizzleless_move(nir_alu_instr *instr)
   {
               if (instr->src[0].src.ssa->num_components != num_comp)
            if (instr->op == nir_op_mov) {
      for (unsigned i = 0; i < num_comp; i++) {
      if (instr->src[0].swizzle[i] != i)
         } else {
      for (unsigned i = 0; i < num_comp; i++) {
      if (instr->src[i].swizzle[0] != i ||
      instr->src[i].src.ssa != instr->src[0].src.ssa)
                  }
      static bool
   rewrite_to_vec(nir_alu_instr *mov, nir_alu_instr *vec)
   {
      if (mov->op != nir_op_mov)
                     unsigned num_comp = mov->def.num_components;
   nir_alu_instr *new_vec = nir_alu_instr_create(b.shader, nir_op_vec(num_comp));
   for (unsigned i = 0; i < num_comp; i++)
            nir_def *new = nir_builder_alu_instr_finish_and_insert(&b, new_vec);
            /* If we remove "mov" and it's the next instruction in the
               }
      static bool
   copy_propagate_alu(nir_alu_src *src, nir_alu_instr *copy)
   {
      nir_def *def = NULL;
   nir_alu_instr *user = nir_instr_as_alu(nir_src_parent_instr(&src->src));
   unsigned src_idx = src - user->src;
   assert(src_idx < nir_op_infos[user->op].num_inputs);
            if (copy->op == nir_op_mov) {
               for (unsigned i = 0; i < num_comp; i++)
      } else {
               for (unsigned i = 1; i < num_comp; i++) {
      if (copy->src[src->swizzle[i]].src.ssa != def)
               for (unsigned i = 0; i < num_comp; i++)
                           }
      static bool
   copy_propagate(nir_src *src, nir_alu_instr *copy)
   {
      if (!is_swizzleless_move(copy))
                        }
      static bool
   copy_prop_instr(nir_instr *instr)
   {
      if (instr->type != nir_instr_type_alu)
                     if (!nir_op_is_vec_or_mov(mov->op))
                     nir_foreach_use_including_if_safe(src, &mov->def) {
      if (!nir_src_is_if(src) && nir_src_parent_instr(src)->type == nir_instr_type_alu)
         else
               if (progress && nir_def_is_unused(&mov->def))
               }
      bool
   nir_copy_prop_impl(nir_function_impl *impl)
   {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_copy_prop(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      if (nir_copy_prop_impl(impl))
                  }
