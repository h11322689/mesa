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
   * @file v3d_opt_dead_code.c
   *
   * This is a simple dead code eliminator for SSA values in VIR.
   *
   * It walks all the instructions finding what temps are used, then walks again
   * to remove instructions writing unused temps.
   *
   * This is an inefficient implementation if you have long chains of
   * instructions where the entire chain is dead, but we expect those to have
   * been eliminated at the NIR level, and here we're just cleaning up small
   * problems produced by NIR->VIR.
   */
      #include "v3d_compiler.h"
      static bool debug;
      static void
   dce(struct v3d_compile *c, struct qinst *inst)
   {
         if (debug) {
            fprintf(stderr, "Removing: ");
      }
   assert(!v3d_qpu_writes_flags(&inst->qpu));
   }
      static bool
   has_nonremovable_reads(struct v3d_compile *c, struct qinst *inst)
   {
         for (int i = 0; i < vir_get_nsrc(inst); i++) {
                        }
      static bool
   can_write_to_null(struct v3d_compile *c, struct qinst *inst)
   {
         /* The SFU instructions must write to a physical register. */
   if (c->devinfo->ver >= 41 && v3d_qpu_uses_sfu(&inst->qpu))
            }
      static void
   vir_dce_flags(struct v3d_compile *c, struct qinst *inst)
   {
         if (debug) {
            fprintf(stderr,
                              inst->qpu.flags.apf = V3D_QPU_PF_NONE;
   inst->qpu.flags.mpf = V3D_QPU_PF_NONE;
   inst->qpu.flags.auf = V3D_QPU_UF_NONE;
   }
      static bool
   check_last_ldunifa(struct v3d_compile *c,
               {
         if (!inst->qpu.sig.ldunifa && !inst->qpu.sig.ldunifarf)
            list_for_each_entry_from(struct qinst, scan_inst, inst->link.next,
                  /* If we find a new write to unifa, then this was the last
   * ldunifa in its sequence and is safe to remove.
   */
   if (scan_inst->dst.file == QFILE_MAGIC &&
                        /* If we find another ldunifa in the same sequence then we
   * can't remove it.
   */
               }
      static bool
   check_first_ldunifa(struct v3d_compile *c,
                     {
         if (!inst->qpu.sig.ldunifa && !inst->qpu.sig.ldunifarf)
            list_for_each_entry_from_rev(struct qinst, scan_inst, inst->link.prev,
                  /* If we find a write to unifa, then this was the first
   * ldunifa in its sequence and is safe to remove.
   */
   if (scan_inst->dst.file == QFILE_MAGIC &&
      scan_inst->dst.index == V3D_QPU_WADDR_UNIFA) {
                     /* If we find another ldunifa in the same sequence then we
   * can't remove it.
   */
               }
      static bool
   increment_unifa_address(struct v3d_compile *c, struct qinst *unifa)
   {
         if (unifa->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
         unifa->qpu.alu.mul.op == V3D_QPU_M_MOV) {
      c->cursor = vir_after_inst(unifa);
   struct qreg unifa_reg = vir_reg(QFILE_MAGIC, V3D_QPU_WADDR_UNIFA);
   vir_ADD_dest(c, unifa_reg, unifa->src[0], vir_uniform_ui(c, 4u));
               if (unifa->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
         unifa->qpu.alu.add.op == V3D_QPU_A_ADD) {
      c->cursor = vir_after_inst(unifa);
   struct qreg unifa_reg = vir_reg(QFILE_MAGIC, V3D_QPU_WADDR_UNIFA);
   struct qreg tmp =
         vir_ADD_dest(c, unifa_reg, unifa->src[0], tmp);
               }
      bool
   vir_opt_dead_code(struct v3d_compile *c)
   {
         bool progress = false;
            /* Defuse the "are you removing the cursor?" assertion in the core.
      * You'll need to set up a new cursor for any new instructions after
   * doing DCE (which we would expect, anyway).
               vir_for_each_inst_inorder(inst, c) {
            for (int i = 0; i < vir_get_nsrc(inst); i++) {
                     vir_for_each_block(block, c) {
            struct qinst *last_flags_write = NULL;
   c->cur_block = block;
   vir_for_each_inst_safe(inst, block) {
            /* If this instruction reads the flags, we can't
                              if (inst->dst.file != QFILE_NULL &&
                                                            bool is_first_ldunifa = false;
   bool is_last_ldunifa = false;
                                                   if (v3d_qpu_writes_flags(&inst->qpu)) {
         /* If we obscure a previous flags write,
      * drop it.
      if (last_flags_write &&
                                             if (v3d_qpu_writes_flags(&inst->qpu) ||
      has_nonremovable_reads(c, inst) ||
   (is_ldunifa && !is_first_ldunifa && !is_last_ldunifa)) {
      /* If we can't remove the instruction, but we
      * don't need its destination value, just
   * remove the destination.  The register
   * allocator would trivially color it and it
   * wouldn't cause any register pressure, but
   * it's nicer to read the VIR code without
   * unused destination regs.
      if (inst->dst.file == QFILE_TEMP &&
         can_write_to_null(c, inst)) {
      if (debug) {
            fprintf(stderr,
                                          /* If we are removing the first ldunifa in a sequence
   * we need to update the unifa address.
   */
   if (is_first_ldunifa) {
                              assert(inst != last_flags_write);
   dce(c, inst);
                     }
