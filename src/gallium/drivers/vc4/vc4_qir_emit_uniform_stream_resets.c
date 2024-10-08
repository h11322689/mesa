   /*
   * Copyright © 2014 Broadcom
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
   * @file vc4_qir_emit_uniform_stream_resets.c
   *
   * Adds updates to the uniform stream address at the start of each basic block
   * that uses uniforms.
   *
   * This will be done just before the translation to QPU instructions, once we
   * have performed optimization know how many uniforms are used in each block.
   */
      #include "vc4_qir.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
      static bool
   block_reads_any_uniform(struct qblock *block)
   {
         qir_for_each_inst(inst, block) {
                        }
      void
   qir_emit_uniform_stream_resets(struct vc4_compile *c)
   {
                  qir_for_each_block(block, c) {
            if (block != qir_entry_block(c) &&
      (block_reads_any_uniform(block) ||
                                    /* Load the offset of the next uniform in the stream
   * after the one we're generating here.
   */
   struct qinst *load_imm =
         qir_inst(QOP_LOAD_IMM,
                                          /* Pushes to the top of the block, so in reverse
   * order.
                     qir_for_each_inst(inst, block) {
            }
