   /*
   * Copyright (C) 2018 Alyssa Rosenzweig
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
      #include "bi_builder.h"
   #include "compiler.h"
      /* SSA copy propagation */
      static bool
   bi_reads_fau(bi_instr *ins)
   {
      bi_foreach_src(ins, s) {
      if (ins->src[s].type == BI_INDEX_FAU)
                  }
      void
   bi_opt_copy_prop(bi_context *ctx)
   {
      /* Chase SPLIT of COLLECT. Instruction selection usually avoids this
   * pattern (due to the split cache), but it is inevitably generated by
   * the UBO pushing pass.
   */
   bi_instr **collects = calloc(sizeof(bi_instr *), ctx->ssa_alloc);
   bi_foreach_instr_global_safe(ctx, I) {
      if (I->op == BI_OPCODE_COLLECT_I32) {
      /* Rewrite trivial collects while we're at it */
                     } else if (I->op == BI_OPCODE_SPLIT_I32) {
      /* Rewrite trivial splits while we're at it */
                  bi_instr *collect = collects[I->src[0].value];
                                                                                 bi_foreach_instr_global_safe(ctx, ins) {
      if (ins->op == BI_OPCODE_MOV_I32 &&
                     /* Peek through one layer so copyprop converges in one
   * iteration for chained moves */
                     if (!bi_is_null(chained))
               assert(ins->nr_dests == 1);
               bi_foreach_src(ins, s) {
               if (use.type != BI_INDEX_NORMAL)
                                                if (!bi_is_null(repl))
                     }
