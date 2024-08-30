   /*
   * Copyright (C) 2018 Alyssa Rosenzweig
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
      #include "util/bitscan.h"
   #include "compiler.h"
   #include "midgard_ops.h"
      static bool
   identity_swizzle(midgard_instruction *mov)
   {
      /* Write mask should be minimal in SSA form */
   assert(mir_is_ssa(mov->src[1]));
            for (unsigned i = 0; i < util_last_bit(mov->mask); ++i) {
      if (mov->swizzle[1][i] != i)
                  }
      bool
   midgard_opt_copy_prop(compiler_context *ctx, midgard_block *block)
   {
               mir_foreach_instr_in_block_safe(block, ins) {
      if (ins->type != TAG_ALU_4)
         if (!OP_IS_MOVE(ins->op))
         if (ins->is_pack)
            unsigned from = ins->src[1];
                     if (to & PAN_IS_REG)
         if (from & PAN_IS_REG)
            /* Constant propagation is not handled here, either */
   if (ins->has_inline_constant)
         if (ins->has_constants)
            /* Modifier propagation is not handled here */
   if (mir_nontrivial_mod(ins, 1, false))
         if (mir_nontrivial_outmod(ins))
            /* Shortened arguments (bias for textures, extra load/store
   * arguments, etc.) do not get a swizzle, only a start
   * component and even that is restricted. Fragment writeout
                     mir_foreach_instr_global(ctx, q) {
      bool is_tex = q->type == TAG_TEXTURE_4;
                                 /* For textures, we get a real swizzle for the
                           mir_foreach_src(q, s) {
      if ((s >= start) && q->src[s] == to) {
      no_swizzle = true;
                     if (no_swizzle && !identity_swizzle(ins))
            if (ctx->blend_src1 == to)
            /* We're clear -- rewrite, composing the swizzle */
   mir_rewrite_index_src_swizzle(ctx, to, from, ins->swizzle[1]);
   mir_remove_instruction(ins);
                  }
