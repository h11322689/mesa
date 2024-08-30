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
   */
      #include "brw_nir.h"
   #include "compiler/nir/nir_builder.h"
      /*
   * Implements a small peephole optimization that looks for a multiply that
   * is only ever used in an add and replaces both with an fma.
   */
      static inline bool
   are_all_uses_fadd(nir_def *def)
   {
      nir_foreach_use_including_if(use_src, def) {
      if (nir_src_is_if(use_src))
            nir_instr *use_instr = nir_src_parent_instr(use_src);
   if (use_instr->type != nir_instr_type_alu)
            nir_alu_instr *use_alu = nir_instr_as_alu(use_instr);
   switch (use_alu->op) {
   case nir_op_fadd:
            case nir_op_mov:
   case nir_op_fneg:
   case nir_op_fabs:
      if (!are_all_uses_fadd(&use_alu->def))
               default:
                        }
      static nir_alu_instr *
   get_mul_for_src(nir_alu_src *src, unsigned num_components,
         {
               nir_instr *instr = src->src.ssa->parent_instr;
   if (instr->type != nir_instr_type_alu)
                     /* We want to bail if any of the other ALU operations involved is labeled
   * exact.  One reason for this is that, while the value that is changing is
   * actually the result of the add and not the multiply, the intention of
   * the user when they specify an exact multiply is that they want *that*
   * value and what they don't care about is the add.  Another reason is that
   * SPIR-V explicitly requires this behaviour.
   */
   if (alu->exact)
            switch (alu->op) {
   case nir_op_mov:
      alu = get_mul_for_src(&alu->src[0], alu->def.num_components,
               case nir_op_fneg:
      alu = get_mul_for_src(&alu->src[0], alu->def.num_components,
         *negate = !*negate;
         case nir_op_fabs:
      alu = get_mul_for_src(&alu->src[0], alu->def.num_components,
         *negate = false;
   *abs = true;
         case nir_op_fmul:
      /* Only absorb a fmul into a ffma if the fmul is only used in fadd
   * operations.  This prevents us from being too aggressive with our
   * fusing which can actually lead to more instructions.
   */
   if (!are_all_uses_fadd(&alu->def))
               default:
                  if (!alu)
            /* Copy swizzle data before overwriting it to avoid setting a wrong swizzle.
   *
   * Example:
   *   Former swizzle[] = xyzw
   *   src->swizzle[] = zyxx
   *
   *   Expected output swizzle = zyxx
   *   If we reuse swizzle in the loop, then output swizzle would be zyzz.
   */
   memcpy(swizzle_tmp, swizzle, NIR_MAX_VEC_COMPONENTS*sizeof(uint8_t));
   for (int i = 0; i < num_components; i++)
               }
      /**
   * Given a list of (at least two) nir_alu_src's, tells if any of them is a
   * constant value and is used only once.
   */
   static bool
   any_alu_src_is_a_constant(nir_alu_src srcs[])
   {
      for (unsigned i = 0; i < 2; i++) {
      if (srcs[i].src.ssa->parent_instr->type == nir_instr_type_load_const) {
                     if (list_is_singular(&load_const->def.uses))
                     }
      static bool
   brw_nir_opt_peephole_ffma_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *add = nir_instr_as_alu(instr);
   if (add->op != nir_op_fadd)
            if (add->exact)
               /* This, is the case a + a.  We would rather handle this with an
   * algebraic reduction than fuse it.  Also, we want to only fuse
   * things where the multiply is used only once and, in this case,
   * it would be used twice by the same instruction.
   */
   if (add->src[0].src.ssa == add->src[1].src.ssa)
            nir_alu_instr *mul;
   uint8_t add_mul_src, swizzle[NIR_MAX_VEC_COMPONENTS];
   bool negate, abs;
   for (add_mul_src = 0; add_mul_src < 2; add_mul_src++) {
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++)
            negate = false;
            mul = get_mul_for_src(&add->src[add_mul_src],
                  if (mul != NULL)
               if (mul == NULL)
                     nir_def *mul_src[2];
   mul_src[0] = mul->src[0].src.ssa;
            /* If any of the operands of the fmul and any of the fadd is a constant,
   * we bypass because it will be more efficient as the constants will be
   * propagated as operands, potentially saving two load_const instructions.
   */
   if (any_alu_src_is_a_constant(mul->src) &&
      any_alu_src_is_a_constant(add->src)) {
                        if (abs) {
      for (unsigned i = 0; i < 2; i++)
               if (negate)
                     for (unsigned i = 0; i < 2; i++) {
      ffma->src[i].src = nir_src_for_ssa(mul_src[i]);
   for (unsigned j = 0; j < add->def.num_components; j++)
      }
            nir_def_init(&ffma->instr, &ffma->def,
                  nir_builder_instr_insert(b, &ffma->instr);
   assert(list_is_empty(&add->def.uses));
               }
      bool
   brw_nir_opt_peephole_ffma(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader, brw_nir_opt_peephole_ffma_instr,
                  }
