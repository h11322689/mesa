   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
      /* Midgard has some accelerated support for perspective projection on the
   * load/store pipes. So the first perspective projection pass looks for
   * lowered/open-coded perspective projection of the form "fmul (A.xyz,
   * frcp(A.w))" or "fmul (A.xy, frcp(A.z))" and rewrite with a native
   * perspective division opcode (on the load/store pipe). Caveats apply: the
   * frcp should be used only once to make this optimization worthwhile. And the
   * source of the frcp ought to be a varying to make it worthwhile...
   *
   * The second pass in this file is a step #2 of sorts: fusing that load/store
   * projection into a varying load instruction (they can be done together
   * implicitly). This depends on the combination pass. Again caveat: the vary
   * should only be used once to make this worthwhile.
   */
      #include "compiler.h"
      static bool
   is_swizzle_0(unsigned *swizzle)
   {
      for (unsigned c = 0; c < MIR_VEC_COMPONENTS; ++c)
      if (swizzle[c])
            }
      bool
   midgard_opt_combine_projection(compiler_context *ctx, midgard_block *block)
   {
               mir_foreach_instr_in_block_safe(block, ins) {
      /* First search for fmul */
   if (ins->type != TAG_ALU_4)
         if (ins->op != midgard_alu_op_fmul)
                              if (!mir_is_simple_swizzle(ins->swizzle[0], ins->mask))
         if (!is_swizzle_0(ins->swizzle[1]))
            /* Awesome, we're the right form. Now check where src2 is from */
   unsigned frcp = ins->src[1];
            if (frcp & PAN_IS_REG)
         if (to & PAN_IS_REG)
            bool frcp_found = false;
   unsigned frcp_component = 0;
            mir_foreach_instr_in_block_safe(block, sub) {
                                    frcp_found =
                     if (!frcp_found)
         if (frcp_from != ins->src[0])
         if (frcp_component != COMPONENT_W && frcp_component != COMPONENT_Z)
         if (!mir_single_use(ctx, frcp))
                              /* One for frcp and one for fmul */
   if (mir_use_count(ctx, frcp_from) > 2)
            mir_foreach_instr_in_block_safe(block, v) {
      if (v->dest != frcp_from)
         if (v->type != TAG_LOAD_STORE_4)
                        ok = true;
               if (!ok)
                     midgard_instruction accel = {
      .type = TAG_LOAD_STORE_4,
   .mask = ins->mask,
   .dest = to,
   .dest_type = nir_type_float32,
   .src =
      {
      frcp_from,
   ~0,
   ~0,
         .src_types =
      {
            .swizzle = SWIZZLE_IDENTITY_4,
   .op = frcp_component == COMPONENT_W
               .load_store =
      {
                  mir_insert_instruction_before(ctx, ins, accel);
                           }
      bool
   midgard_opt_varying_projection(compiler_context *ctx, midgard_block *block)
   {
               mir_foreach_instr_in_block_safe(block, ins) {
      /* Search for a projection */
   if (ins->type != TAG_LOAD_STORE_4)
         if (!OP_IS_PROJECTION(ins->op))
            unsigned vary = ins->src[0];
            if (vary & PAN_IS_REG)
         if (to & PAN_IS_REG)
         if (!mir_single_use(ctx, vary))
                              mir_foreach_instr_in_block_safe(block, v) {
      if (v->dest != vary)
         if (v->type != TAG_LOAD_STORE_4)
                                                                                                                     rewritten = true;
               if (rewritten)
                           }
