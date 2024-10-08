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
   * @file vc4_opt_dead_code.c
   *
   * This is a simple dead code eliminator for SSA values in QIR.
   *
   * It walks all the instructions finding what temps are used, then walks again
   * to remove instructions writing unused temps.
   *
   * This is an inefficient implementation if you have long chains of
   * instructions where the entire chain is dead, but we expect those to have
   * been eliminated at the NIR level, and here we're just cleaning up small
   * problems produced by NIR->QIR.
   */
      #include "vc4_qir.h"
      static bool debug;
      static void
   dce(struct vc4_compile *c, struct qinst *inst)
   {
         if (debug) {
            fprintf(stderr, "Removing: ");
      }
   assert(!inst->sf);
   }
      static bool
   has_nonremovable_reads(struct vc4_compile *c, struct qinst *inst)
   {
         for (int i = 0; i < qir_get_nsrc(inst); i++) {
                                                         /* Can't get rid of the last VPM read, or the
   * simulator (at least) throws an error.
   */
   uint32_t total_size = 0;
   for (uint32_t i = 0; i < ARRAY_SIZE(c->vattr_sizes); i++)
                     if (inst->src[i].file == QFILE_VARY &&
      c->input_slots[inst->src[i].index].slot == 0xff) {
            }
      bool
   qir_opt_dead_code(struct vc4_compile *c)
   {
         bool progress = false;
            qir_for_each_inst_inorder(inst, c) {
            for (int i = 0; i < qir_get_nsrc(inst); i++) {
                     qir_for_each_block(block, c) {
            qir_for_each_inst_safe(inst, block) {
            if (inst->dst.file != QFILE_NULL &&
                                             if (inst->sf ||
      has_nonremovable_reads(c, inst)) {
      /* If we can't remove the instruction, but we
      * don't need its destination value, just
   * remove the destination.  The register
   * allocator would trivially color it and it
   * wouldn't cause any register pressure, but
   * it's nicer to read the QIR code without
   * unused destination regs.
      if (inst->dst.file == QFILE_TEMP) {
            if (debug) {
            fprintf(stderr,
                                          for (int i = 0; i < qir_get_nsrc(inst); i++) {
                                                               dce(c, inst);
                     }
