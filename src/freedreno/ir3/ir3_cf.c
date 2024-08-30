   /*
   * Copyright (C) 2019 Google.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "util/ralloc.h"
      #include "ir3.h"
      static bool
   is_safe_conv(struct ir3_instruction *instr, type_t src_type, opc_t *src_opc)
   {
      if (instr->opc != OPC_MOV)
            /* Only allow half->full or full->half without any type conversion (like
   * int to float).
   */
   if (type_size(instr->cat1.src_type) == type_size(instr->cat1.dst_type) ||
      full_type(instr->cat1.src_type) != full_type(instr->cat1.dst_type))
         /* mul.s24/u24 always return 32b result regardless of its sources size,
   * hence we cannot guarantee the high 16b of dst being zero or sign extended.
   */
   if ((*src_opc == OPC_MUL_S24 || *src_opc == OPC_MUL_U24) &&
      type_size(instr->cat1.src_type) == 16)
         struct ir3_register *dst = instr->dsts[0];
            /* disallow conversions that cannot be folded into
   * alu instructions:
   */
   if (instr->cat1.round != ROUND_ZERO)
            if (dst->flags & (IR3_REG_RELATIV | IR3_REG_ARRAY))
         if (src->flags & (IR3_REG_RELATIV | IR3_REG_ARRAY))
            /* Check that the source of the conv matches the type of the src
   * instruction.
   */
   if (src_type == instr->cat1.src_type)
            /* We can handle mismatches with integer types by converting the opcode
   * but not when an integer is reinterpreted as a float or vice-versa.
   */
   if (type_float(src_type) != type_float(instr->cat1.src_type))
            /* We have types with mismatched signedness. Mismatches on the signedness
   * don't matter when narrowing:
   */
   if (type_size(instr->cat1.dst_type) < type_size(instr->cat1.src_type))
            /* Try swapping the opcode: */
   bool can_swap = true;
   *src_opc = ir3_try_swap_signedness(*src_opc, &can_swap);
      }
      static bool
   all_uses_safe_conv(struct ir3_instruction *conv_src, type_t src_type)
   {
      opc_t opc = conv_src->opc;
   bool first = true;
   foreach_ssa_use (use, conv_src) {
      opc_t new_opc = opc;
   if (!is_safe_conv(use, src_type, &new_opc))
         /* Check if multiple uses have conflicting requirements on the opcode.
   */
   if (!first && opc != new_opc)
         first = false;
      }
   conv_src->opc = opc;
      }
      /* For an instruction which has a conversion folded in, re-write the
   * uses of *all* conv's that used that src to be a simple mov that
   * cp can eliminate.  This avoids invalidating the SSA uses, it just
   * shifts the use to a simple mov.
   */
   static void
   rewrite_src_uses(struct ir3_instruction *src)
   {
      foreach_ssa_use (use, src) {
               if (is_half(src)) {
         } else {
                        }
      static bool
   try_conversion_folding(struct ir3_instruction *conv)
   {
               if (conv->opc != OPC_MOV)
            /* NOTE: we can have non-ssa srcs after copy propagation: */
   src = ssa(conv->srcs[0]);
   if (!src)
            if (!is_alu(src))
            bool can_fold;
   type_t base_type = ir3_output_conv_type(src, &can_fold);
   if (!can_fold)
            type_t src_type = ir3_output_conv_src_type(src, base_type);
            /* Avoid cases where we've already folded in a conversion. We assume that
   * if there is a chain of conversions that's foldable then it's been
   * folded in NIR already.
   */
   if (src_type != dst_type)
            if (!all_uses_safe_conv(src, src_type))
            ir3_set_dst_type(src, is_half(conv));
               }
      bool
   ir3_cf(struct ir3 *ir)
   {
      void *mem_ctx = ralloc_context(NULL);
                     foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                                 }
