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
   * @file vc4_opt_copy_propagation.c
   *
   * This implements simple copy propagation for QIR without control flow.
   *
   * For each temp, it keeps a qreg of which source it was MOVed from, if it
   * was.  If we see that used later, we can just reuse the source value, since
   * we know we don't have control flow, and we have SSA for our values so
   * there's no killing to worry about.
   */
      #include "vc4_qir.h"
      static bool
   is_copy_mov(struct qinst *inst)
   {
         if (!inst)
            if (inst->op != QOP_MOV &&
         inst->op != QOP_FMOV &&
   inst->op != QOP_MMOV) {
            if (inst->dst.file != QFILE_TEMP)
            if (inst->src[0].file != QFILE_TEMP &&
         inst->src[0].file != QFILE_UNIF) {
            if (inst->dst.pack || inst->cond != QPU_COND_ALWAYS)
               }
      static bool
   try_copy_prop(struct vc4_compile *c, struct qinst *inst, struct qinst **movs)
   {
         bool debug = false;
      for (int i = 0; i < qir_get_nsrc(inst); i++) {
                                 /* We have two ways of finding MOVs we can copy propagate
   * from.  One is if it's an SSA def: then we can reuse it from
   * any block in the program, as long as its source is also an
   * SSA def.  Alternatively, if it's in the "movs" array
   * tracked within the block, then we know the sources for it
   * haven't been changed since we saw the instruction within
   * our block.
   */
   struct qinst *mov = movs[inst->src[i].index];
   if (!mov) {
                                                   /* Mul rotation's source needs to be in an r0-r3 accumulator,
   * so no uniforms or regfile-a/r4 unpacking allowed.
   */
   if (inst->op == QOP_ROT_MUL &&
                        uint8_t unpack;
   if (mov->src[0].pack) {
            /* Make sure that the meaning of the unpack
   * would be the same between the two
   * instructions.
   */
                              /* There's only one unpack field, so make sure
   * this instruction doesn't already use it.
   */
   bool already_has_unpack = false;
   for (int j = 0; j < qir_get_nsrc(inst); j++) {
                                    /* A destination pack requires the PM bit to
   * be set to a specific value already, which
                                                if (debug) {
                                             if (debug) {
                                    }
      static void
   apply_kills(struct vc4_compile *c, struct qinst **movs, struct qinst *inst)
   {
         if (inst->dst.file != QFILE_TEMP)
            for (int i = 0; i < c->num_temps; i++) {
            if (movs[i] &&
      (movs[i]->dst.index == inst->dst.index ||
      (movs[i]->src[0].file == QFILE_TEMP &&
      }
      bool
   qir_opt_copy_propagation(struct vc4_compile *c)
   {
         bool progress = false;
            movs = ralloc_array(c, struct qinst *, c->num_temps);
   if (!movs)
            qir_for_each_block(block, c) {
            /* The MOVs array tracks only available movs within the
                                                                           }
