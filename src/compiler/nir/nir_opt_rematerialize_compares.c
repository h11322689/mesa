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
      #include "nir/nir_builder.h"
   #include "nir.h"
   #include "nir_constant_expressions.h"
   #include "nir_control_flow.h"
   #include "nir_loop_analyze.h"
      static bool
   is_two_src_comparison(const nir_alu_instr *instr)
   {
      switch (instr->op) {
   case nir_op_flt:
   case nir_op_flt32:
   case nir_op_fge:
   case nir_op_fge32:
   case nir_op_feq:
   case nir_op_feq32:
   case nir_op_fneu:
   case nir_op_fneu32:
   case nir_op_ilt:
   case nir_op_ilt32:
   case nir_op_ult:
   case nir_op_ult32:
   case nir_op_ige:
   case nir_op_ige32:
   case nir_op_uge:
   case nir_op_uge32:
   case nir_op_ieq:
   case nir_op_ieq32:
   case nir_op_ine:
   case nir_op_ine32:
         default:
            }
      static inline bool
   is_zero(const nir_alu_instr *instr, unsigned src, unsigned num_components,
         {
      /* only constant srcs: */
   if (!nir_src_is_const(instr->src[src].src))
            for (unsigned i = 0; i < num_components; i++) {
      nir_alu_type type = nir_op_infos[instr->op].input_types[src];
   switch (nir_alu_type_get_base_type(type)) {
   case nir_type_int:
   case nir_type_uint: {
      if (nir_src_comp_as_int(instr->src[src].src, swizzle[i]) != 0)
            }
   case nir_type_float: {
      if (nir_src_comp_as_float(instr->src[src].src, swizzle[i]) != 0)
            }
   default:
                        }
      static bool
   all_uses_are_bcsel(const nir_alu_instr *instr)
   {
      nir_foreach_use(use, &instr->def) {
      if (nir_src_parent_instr(use)->type != nir_instr_type_alu)
            nir_alu_instr *const alu = nir_instr_as_alu(nir_src_parent_instr(use));
   if (alu->op != nir_op_bcsel &&
                  /* Not only must the result be used by a bcsel, but it must be used as
   * the first source (the condition).
   */
   if (alu->src[0].src.ssa != &instr->def)
                  }
      static bool
   all_uses_are_compare_with_zero(const nir_alu_instr *instr)
   {
      nir_foreach_use(use, &instr->def) {
      if (nir_src_parent_instr(use)->type != nir_instr_type_alu)
            nir_alu_instr *const alu = nir_instr_as_alu(nir_src_parent_instr(use));
   if (!is_two_src_comparison(alu))
            if (!is_zero(alu, 0, 1, alu->src[0].swizzle) &&
                  if (!all_uses_are_bcsel(alu))
                  }
      static bool
   nir_opt_rematerialize_compares_impl(nir_shader *shader, nir_function_impl *impl)
   {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_alu_instr *const alu = nir_instr_as_alu(instr);
                                 /* At this point it is known that alu is a comparison instruction
   * that is only used by nir_op_bcsel and possibly by if-statements
   * (though the latter has not been explicitly checked).
   *
   * Iterate through each use of the comparison.  For every use (or use
   * by an if-statement) that is in a different block, emit a copy of
   * the comparison.  Care must be taken here.  The original
   * instruction must be duplicated only once in each block because CSE
   * cannot be run after this pass.
   */
   nir_foreach_use_including_if_safe(use, &alu->def) {
                                       /* If the compare is from the previous block, don't
   * rematerialize.
                                          nir_src_rewrite(&if_stmt->condition, &clone->def);
                        /* If the use is in the same block as the def, don't
   * rematerialize.
                                          nir_alu_instr *const use_alu = nir_instr_as_alu(use_instr);
   for (unsigned i = 0; i < nir_op_infos[use_alu->op].num_inputs; i++) {
      if (use_alu->src[i].src.ssa == &alu->def) {
      nir_src_rewrite(&use_alu->src[i].src, &clone->def);
                              if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      static bool
   nir_opt_rematerialize_alu_impl(nir_shader *shader, nir_function_impl *impl)
   {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                              /* This list only include ALU ops that are likely to be able to have
   * cmod propagation on Intel GPUs.
   */
   switch (alu->op) {
   case nir_op_ineg:
   case nir_op_iabs:
   case nir_op_fneg:
   case nir_op_fabs:
   case nir_op_fadd:
   case nir_op_iadd:
   case nir_op_iadd_sat:
   case nir_op_uadd_sat:
   case nir_op_isub_sat:
   case nir_op_usub_sat:
   case nir_op_irhadd:
   case nir_op_urhadd:
   case nir_op_fmul:
   case nir_op_inot:
   case nir_op_iand:
   case nir_op_ior:
   case nir_op_ixor:
   case nir_op_ffloor:
   case nir_op_ffract:
   case nir_op_uclz:
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
   case nir_op_urol:
   case nir_op_uror:
         default:
                  /* To help prevent increasing live ranges, require that one of the
   * sources be a constant.
   */
   if (nir_op_infos[alu->op].num_inputs == 2 &&
      !nir_src_is_const(alu->src[0].src) &&
                              /* At this point it is known that the alu is only used by a
   * comparison with zero that is used by nir_op_bcsel and possibly by
   * if-statements (though the latter has not been explicitly checked).
   *
   * Iterate through each use of the ALU.  For every use that is in a
   * different block, emit a copy of the ALU.  Care must be taken here.
   * The original instruction must be duplicated only once in each
   * block because CSE cannot be run after this pass.
   */
                     /* If the use is in the same block as the def, don't
   * rematerialize.
   */
                                    nir_alu_instr *const use_alu = nir_instr_as_alu(use_instr);
   for (unsigned i = 0; i < nir_op_infos[use_alu->op].num_inputs; i++) {
      if (use_alu->src[i].src.ssa == &alu->def) {
      nir_src_rewrite(&use_alu->src[i].src, &clone->def);
                           if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_opt_rematerialize_compares(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                              }
