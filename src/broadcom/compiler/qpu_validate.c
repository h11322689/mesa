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
   * @file
   *
   * Validates the QPU instruction sequence after register allocation and
   * scheduling.
   */
      #include <assert.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include "v3d_compiler.h"
   #include "qpu/qpu_disasm.h"
      struct v3d_qpu_validate_state {
         struct v3d_compile *c;
   const struct v3d_qpu_instr *last;
   int ip;
   int last_sfu_write;
   int last_branch_ip;
   int last_thrsw_ip;
            /* Set when we've found the last-THRSW signal, or if we were started
      * in single-segment mode.
               /* Set when we've found the THRSW after the last THRSW */
            };
      static void
   fail_instr(struct v3d_qpu_validate_state *state, const char *msg)
   {
                           int dump_ip = 0;
   vir_for_each_inst_inorder(inst, c) {
                                          fprintf(stderr, "\n");
   }
      static bool
   in_branch_delay_slots(struct v3d_qpu_validate_state *state)
   {
         }
      static bool
   in_thrsw_delay_slots(struct v3d_qpu_validate_state *state)
   {
         }
      static bool
   qpu_magic_waddr_matches(const struct v3d_qpu_instr *inst,
         {
         if (inst->type == V3D_QPU_INSTR_TYPE_ALU)
            if (inst->alu.add.op != V3D_QPU_A_NOP &&
         inst->alu.add.magic_write &&
            if (inst->alu.mul.op != V3D_QPU_M_NOP &&
         inst->alu.mul.magic_write &&
            }
      static void
   qpu_validate_inst(struct v3d_qpu_validate_state *state, struct qinst *qinst)
   {
                  if (qinst->is_tlb_z_write && state->ip < state->first_tlb_z_write)
                     if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH &&
         state->first_tlb_z_write >= 0 &&
   state->ip > state->first_tlb_z_write &&
   inst->branch.msfign != V3D_QPU_MSFIGN_NONE &&
   inst->branch.cond != V3D_QPU_BRANCH_COND_ALWAYS &&
   inst->branch.cond != V3D_QPU_BRANCH_COND_A0 &&
   inst->branch.cond != V3D_QPU_BRANCH_COND_NA0) {
            if (inst->type != V3D_QPU_INSTR_TYPE_ALU)
            if (inst->alu.add.op == V3D_QPU_A_SETMSF &&
         state->first_tlb_z_write >= 0 &&
   state->ip > state->first_tlb_z_write) {
            if (state->first_tlb_z_write >= 0 &&
         state->ip > state->first_tlb_z_write &&
   inst->alu.add.op == V3D_QPU_A_MSF) {
            if (devinfo->ver < 71) {
            if (inst->sig.small_imm_a || inst->sig.small_imm_c ||
      inst->sig.small_imm_d) {
   } else {
            if ((inst->sig.small_imm_a || inst->sig.small_imm_b) &&
      !vir_is_add(qinst)) {
      }
   if ((inst->sig.small_imm_c || inst->sig.small_imm_d) &&
      !vir_is_mul(qinst)) {
      }
   if (inst->sig.small_imm_a + inst->sig.small_imm_b +
      inst->sig.small_imm_c + inst->sig.small_imm_d > 1) {
                  /* LDVARY writes r5 two instructions later and LDUNIF writes
      * r5 one instruction later, which is illegal to have
   * together.
      if (state->last && state->last->sig.ldvary &&
         (inst->sig.ldunif || inst->sig.ldunifa)) {
            /* GFXH-1633 (fixed since V3D 4.2.14, which is Rpi4)
      *
   * FIXME: This would not check correctly for V3D 4.2 versions lower
   * than V3D 4.2.14, but that is not a real issue because the simulator
   * will still catch this, and we are not really targeting any such
   * versions anyway.
      if (state->c->devinfo->ver < 42) {
            bool last_reads_ldunif = (state->last && (state->last->sig.ldunif ||
         bool last_reads_ldunifa = (state->last && (state->last->sig.ldunifa ||
         bool reads_ldunif = inst->sig.ldunif || inst->sig.ldunifrf;
   bool reads_ldunifa = inst->sig.ldunifa || inst->sig.ldunifarf;
   if ((last_reads_ldunif && reads_ldunifa) ||
      (last_reads_ldunifa && reads_ldunif)) {
                  int tmu_writes = 0;
   int sfu_writes = 0;
   int vpm_writes = 0;
   int tlb_writes = 0;
            if (inst->alu.add.op != V3D_QPU_A_NOP) {
            if (inst->alu.add.magic_write) {
            if (v3d_qpu_magic_waddr_is_tmu(state->c->devinfo,
               }
   if (v3d_qpu_magic_waddr_is_sfu(inst->alu.add.waddr))
         if (v3d_qpu_magic_waddr_is_vpm(inst->alu.add.waddr))
         if (v3d_qpu_magic_waddr_is_tlb(inst->alu.add.waddr))
                  if (inst->alu.mul.op != V3D_QPU_M_NOP) {
            if (inst->alu.mul.magic_write) {
            if (v3d_qpu_magic_waddr_is_tmu(state->c->devinfo,
               }
   if (v3d_qpu_magic_waddr_is_sfu(inst->alu.mul.waddr))
         if (v3d_qpu_magic_waddr_is_vpm(inst->alu.mul.waddr))
         if (v3d_qpu_magic_waddr_is_tlb(inst->alu.mul.waddr))
                  if (in_thrsw_delay_slots(state)) {
            /* There's no way you want to start SFU during the THRSW delay
   * slots, since the result would land in the other thread.
   */
   if (sfu_writes) {
                        if (inst->sig.ldvary) {
            if (devinfo->ver <= 42)
         if (devinfo->ver >= 71 &&
                           /* SFU r4 results come back two instructions later.  No doing
      * r4 read/writes or other SFU lookups until it's done.
      if (state->ip - state->last_sfu_write < 2) {
                                                      /* XXX: The docs say VPM can happen with the others, but the simulator
      * disagrees.
      if (tmu_writes +
         sfu_writes +
   vpm_writes +
   tlb_writes +
   tsy_writes +
   (devinfo->ver <= 42 ? inst->sig.ldtmu : 0) +
   inst->sig.ldtlb +
   inst->sig.ldvpm +
   inst->sig.ldtlbu > 1) {
                  if (sfu_writes)
            if (inst->sig.thrsw) {
                                          if (state->last_thrsw_ip == state->ip - 1) {
            /* If it's the second THRSW in a row, then it's just a
   * last-thrsw signal.
   */
   if (state->last_thrsw_found)
      } else {
            if (in_thrsw_delay_slots(state)) {
         fail_instr(state,
   }
            if (state->thrend_found &&
         state->last_thrsw_ip - state->ip <= 2 &&
   inst->type == V3D_QPU_INSTR_TYPE_ALU) {
      if ((inst->alu.add.op != V3D_QPU_A_NOP &&
         !inst->alu.add.magic_write)) {
      if (devinfo->ver <= 42) {
         } else if (devinfo->ver >= 71) {
         if (state->last_thrsw_ip - state->ip == 0) {
               }
   if (inst->alu.add.waddr == 2 ||
                           if ((inst->alu.mul.op != V3D_QPU_M_NOP &&
         !inst->alu.mul.magic_write)) {
      if (devinfo->ver <= 42) {
         } else if (devinfo->ver >= 71) {
                                    if (inst->alu.mul.waddr == 2 ||
                           if (v3d_qpu_sig_writes_address(devinfo, &inst->sig) &&
      !inst->sig_magic) {
         if (devinfo->ver <= 42) {
         } else if (devinfo->ver >= 71 &&
                           /* GFXH-1625: No TMUWT in the last instruction */
   if (state->last_thrsw_ip - state->ip == 2 &&
               if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH) {
            if (in_branch_delay_slots(state))
         if (in_thrsw_delay_slots(state))
      }
      static void
   qpu_validate_block(struct v3d_qpu_validate_state *state, struct qblock *block)
   {
         vir_for_each_inst(qinst, block) {
                        }
      /**
   * Checks for the instruction restrictions from page 37 ("Summary of
   * Instruction Restrictions").
   */
   void
   qpu_validate(struct v3d_compile *c)
   {
         /* We don't want to do validation in release builds, but we want to
      * keep compiling the validation code to make sure it doesn't get
      #ifndef DEBUG
         #endif
            struct v3d_qpu_validate_state state = {
            .c = c,
   .last_sfu_write = -10,
   .last_thrsw_ip = -10,
                              vir_for_each_block(block, c) {
                  if (state.thrsw_count > 1 && !state.last_thrsw_found) {
                        if (!state.thrend_found)
   }
