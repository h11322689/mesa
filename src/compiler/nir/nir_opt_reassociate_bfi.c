   /*
   * Copyright © 2022 Intel Corporation
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
      #include "nir_builder.h"
   #include "nir_search_helpers.h"
      /**
   * Attempt to reassociate and optimize some combinations of bfi instructions.
   *
   * Many shaders will use a sequence of bfi instructions to build complex
   * bitfields.  Sequences in real shaders look like:
   *
   *    bfi(A, B, bfi(C, D, 0))
   *
   * Let A and C be constants,
   *
   *    (A & (B << find_lsb(A)) | (~A & bfi(C, D, 0))
   *
   * If find_lsb(A) = 0 (i.e., A is odd), then
   *
   *    (A & B) | (~A & bfi(C, D, 0))
   *    (A & B) | (~A & ((D << find_lsb(C)) & C) | (0 & ~C))
   *    (A & B) | (~A & ((D << find_lsb(C)) & C))
   *    (A & B) | ((D << find_lsb(C)) & (~A & C))
   *
   * If A and C are completely disjoint, that is (A & C) == 0, then (~A & C) == C
   *
   *    (A & B) | ((D << find_lsb(C)) & C)
   *
   * If A and C are completely disjoint, then A & ~C == A
   *
   *    (~C & (A & B)) | ((D << find_lsb(C)) & C)
   *    bfi(C, D, A & B)
   *
   * On some architectures, bfi instructions cannot use immediate values as
   * sources, so the constants A, C, and 0 would have to be loaded into
   * registers.  After reassociation, only C must be loaded into a register.
   * This saves instructions and register pressure.
   *
   * Ideally this would be implemented as an algebraic optimization, but there
   * is no way to enforce the requirement that (A & C) == 0.
   */
      static bool
   nir_opt_reassociate_bfi_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *bfiCD0 = nir_instr_as_alu(instr);
   if (bfiCD0->op != nir_op_bfi || bfiCD0->def.num_components != 1)
            /* Enforce the bfi('#c', d, 0) part of the pattern. */
   if (!nir_src_is_const(bfiCD0->src[0].src) ||
      !nir_src_is_const(bfiCD0->src[2].src) ||
   nir_src_comp_as_uint(bfiCD0->src[2].src,
                     const uint64_t C = nir_src_comp_as_uint(bfiCD0->src[0].src,
            if (!is_used_once(bfiCD0))
            nir_src *use = list_first_entry(&bfiCD0->def.uses,
            if (nir_src_parent_instr(use)->type != nir_instr_type_alu)
            nir_alu_instr *bfiABx = nir_instr_as_alu(nir_src_parent_instr(use));
   if (bfiABx->op != nir_op_bfi || bfiABx->def.num_components != 1)
            /* Enforce the bfi('#a', b, ...) part of the pattern. */
   if (!nir_src_is_const(bfiABx->src[0].src) ||
      bfiABx->src[2].src.ssa != &bfiCD0->def) {
               const uint64_t A = nir_src_comp_as_uint(bfiABx->src[0].src,
            /* Enforce the find_lsb(A) == 0 part of the pattern. */
   if ((A & 1) == 0)
            /* Enforce the "A and C are disjoint" part of the pattern. */
   if ((A & C) != 0)
            /* Now emit the new instructions. */
            /* The extra nir_mov_alu are to handle swizzles that might be on the
   * original sources.
   */
   nir_def *new_bfi = nir_bfi(b,
                                    nir_def_rewrite_uses(&bfiABx->def, new_bfi);
      }
      bool
   nir_opt_reassociate_bfi(nir_shader *shader)
   {
      bool progress = nir_shader_instructions_pass(shader,
                                 }
