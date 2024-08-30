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
   * @file vc4_opt_vpm.c
   *
   * This modifies instructions that exclusively consume a value read from the
   * VPM to directly read the VPM if other operands allow it.
   */
      #include "vc4_qir.h"
      bool
   qir_opt_vpm(struct vc4_compile *c)
   {
         if (c->stage == QSTAGE_FRAG)
            /* For now, only do this pass when we don't have control flow. */
   struct qblock *block = qir_entry_block(c);
   if (block != qir_exit_block(c))
            bool progress = false;
   uint32_t use_count[c->num_temps];
            qir_for_each_inst_inorder(inst, c) {
            for (int i = 0; i < qir_get_nsrc(inst); i++) {
            if (inst->src[i].file == QFILE_TEMP) {
                  /* For instructions reading from a temporary that contains a VPM read
      * result, try to move the instruction up in place of the VPM read.
      qir_for_each_inst_inorder(inst, c) {
                                          if (qir_has_side_effects(c, inst) ||
                        for (int j = 0; j < qir_get_nsrc(inst); j++) {
                                          /* Since VPM reads pull from a FIFO, we only get to
   * read each VPM entry once (unless we reset the read
   * pointer).  That means we can't copy-propagate a VPM
                              struct qinst *mov = c->defs[temp];
   if (!mov ||
      (mov->op != QOP_MOV &&
                                 uint32_t temps = 0;
                              /* The instruction is safe to reorder if its other
                                                                  }
