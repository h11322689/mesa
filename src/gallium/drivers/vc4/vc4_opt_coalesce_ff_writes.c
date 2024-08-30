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
   * @file vc4_opt_coalesce_ff_writes.c
   *
   * This modifies instructions that generate the value consumed by a VPM or TMU
   * coordinate write to write directly into the VPM or TMU.
   */
      #include "vc4_qir.h"
      bool
   qir_opt_coalesce_ff_writes(struct vc4_compile *c)
   {
         /* For now, only do this pass when we don't have control flow. */
   struct qblock *block = qir_entry_block(c);
   if (block != qir_exit_block(c))
            bool progress = false;
   uint32_t use_count[c->num_temps];
            qir_for_each_inst_inorder(inst, c) {
            for (int i = 0; i < qir_get_nsrc(inst); i++) {
            if (inst->src[i].file == QFILE_TEMP) {
                  qir_for_each_inst_inorder(mov_inst, c) {
            if (!qir_is_raw_mov(mov_inst) || mov_inst->sf)
                        if (!(mov_inst->dst.file == QFILE_VPM ||
                                                                        /* Don't bother trying to fold in an ALU op using a uniform to
   * a texture op, as we'll just have to lower the uniform back
   * out.
                                       if (qir_has_side_effects(c, inst) ||
      qir_has_side_effect_reads(c, inst) ||
   inst->op == QOP_TLB_COLOR_READ ||
                     /* Move the generating instruction into the position of the FF
   * write.
   */
   c->defs[inst->dst.index] = NULL;
   inst->dst.file = mov_inst->dst.file;
   inst->dst.index = mov_inst->dst.index;
   if (qir_has_implicit_tex_uniform(mov_inst)) {
                                                      }
