   /*
   * Copyright © 2010 Intel Corporation
   * Copyright © 2014-2017 Broadcom
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
   * The basic model of the list scheduler is to take a basic block, compute a
   * DAG of the dependencies, and make a list of the DAG heads.  Heuristically
   * pick a DAG head, then put all the children that are now DAG heads into the
   * list of things to schedule.
   *
   * The goal of scheduling here is to pack pairs of operations together in a
   * single QPU instruction.
   */
      #include "qpu/qpu_disasm.h"
   #include "v3d_compiler.h"
   #include "util/ralloc.h"
   #include "util/dag.h"
      static bool debug;
      struct schedule_node_child;
      struct schedule_node {
         struct dag_node dag;
   struct list_head link;
            /* Longest cycles + instruction_latency() of any parent of this node. */
            /**
      * Minimum number of cycles from scheduling this instruction until the
   * end of the program, based on the slowest dependency chain through
   * the children.
               /**
      * cycles between this instruction being scheduled and when its result
   * can be consumed.
      };
      /* When walking the instructions in reverse, we need to swap before/after in
   * add_dep().
   */
   enum direction { F, R };
      struct schedule_state {
         const struct v3d_device_info *devinfo;
   struct dag *dag;
   struct schedule_node *last_r[6];
   struct schedule_node *last_rf[64];
   struct schedule_node *last_sf;
   struct schedule_node *last_vpm_read;
   struct schedule_node *last_tmu_write;
   struct schedule_node *last_tmu_config;
   struct schedule_node *last_tmu_read;
   struct schedule_node *last_tlb;
   struct schedule_node *last_vpm;
   struct schedule_node *last_unif;
   struct schedule_node *last_rtop;
   struct schedule_node *last_unifa;
   enum direction dir;
   /* Estimated cycle when the current instruction would start. */
   };
      static void
   add_dep(struct schedule_state *state,
         struct schedule_node *before,
   struct schedule_node *after,
   {
         bool write_after_read = !write && state->dir == R;
            if (!before || !after)
                     if (state->dir == F)
         else
   }
      static void
   add_read_dep(struct schedule_state *state,
               {
         }
      static void
   add_write_dep(struct schedule_state *state,
               {
         add_dep(state, *before, after, true);
   }
      static bool
   qpu_inst_is_tlb(const struct v3d_qpu_instr *inst)
   {
         if (inst->sig.ldtlb || inst->sig.ldtlbu)
            if (inst->type != V3D_QPU_INSTR_TYPE_ALU)
            if (inst->alu.add.op != V3D_QPU_A_NOP &&
         inst->alu.add.magic_write &&
   (inst->alu.add.waddr == V3D_QPU_WADDR_TLB ||
            if (inst->alu.mul.op != V3D_QPU_M_NOP &&
         inst->alu.mul.magic_write &&
   (inst->alu.mul.waddr == V3D_QPU_WADDR_TLB ||
            }
      static void
   process_mux_deps(struct schedule_state *state, struct schedule_node *n,
         {
         assert(state->devinfo->ver < 71);
   switch (mux) {
   case V3D_QPU_MUX_A:
               case V3D_QPU_MUX_B:
            if (!n->inst->qpu.sig.small_imm_b) {
                  default:
               }
         static void
   process_raddr_deps(struct schedule_state *state, struct schedule_node *n,
         {
                  if (!is_small_imm)
   }
      static bool
   tmu_write_is_sequence_terminator(uint32_t waddr)
   {
         switch (waddr) {
   case V3D_QPU_WADDR_TMUS:
   case V3D_QPU_WADDR_TMUSCM:
   case V3D_QPU_WADDR_TMUSF:
   case V3D_QPU_WADDR_TMUSLOD:
   case V3D_QPU_WADDR_TMUA:
   case V3D_QPU_WADDR_TMUAU:
         default:
         }
      static bool
   can_reorder_tmu_write(const struct v3d_device_info *devinfo, uint32_t waddr)
   {
         if (devinfo->ver < 40)
            if (tmu_write_is_sequence_terminator(waddr))
            if (waddr == V3D_QPU_WADDR_TMUD)
            }
      static void
   process_waddr_deps(struct schedule_state *state, struct schedule_node *n,
         {
         if (!magic) {
         } else if (v3d_qpu_magic_waddr_is_tmu(state->devinfo, waddr)) {
            if (can_reorder_tmu_write(state->devinfo, waddr))
                           } else if (v3d_qpu_magic_waddr_is_sfu(waddr)) {
         } else {
            switch (waddr) {
   case V3D_QPU_WADDR_R0:
   case V3D_QPU_WADDR_R1:
   case V3D_QPU_WADDR_R2:
            add_write_dep(state,
            case V3D_QPU_WADDR_R3:
   case V3D_QPU_WADDR_R4:
                        case V3D_QPU_WADDR_VPM:
                        case V3D_QPU_WADDR_TLB:
                        case V3D_QPU_WADDR_SYNC:
   case V3D_QPU_WADDR_SYNCB:
   case V3D_QPU_WADDR_SYNCU:
            /* For CS barrier(): Sync against any other memory
   * accesses.  There doesn't appear to be any need for
   * barriers to affect ALU operations.
                     case V3D_QPU_WADDR_UNIFA:
                                       default:
            }
      /**
   * Common code for dependencies that need to be tracked both forward and
   * backward.
   *
   * This is for things like "all reads of r4 have to happen between the r4
   * writes that surround them".
   */
   static void
   calculate_deps(struct schedule_state *state, struct schedule_node *n)
   {
         const struct v3d_device_info *devinfo = state->devinfo;
   struct qinst *qinst = n->inst;
   struct v3d_qpu_instr *inst = &qinst->qpu;
   /* If the input and output segments are shared, then all VPM reads to
      * a location need to happen before all writes.  We handle this by
   * serializing all VPM operations for now.
   *
   * FIXME: we are assuming that the segments are shared. That is
   * correct right now as we are only using shared, but technically you
   * can choose.
               if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH) {
                           /* XXX: BDI */
                                                      if (v3d_qpu_add_op_num_src(inst->alu.add.op) > 0) {
            if (devinfo->ver < 71) {
         } else {
            }
   if (v3d_qpu_add_op_num_src(inst->alu.add.op) > 1) {
            if (devinfo->ver < 71) {
         } else {
                     if (v3d_qpu_mul_op_num_src(inst->alu.mul.op) > 0) {
            if (devinfo->ver < 71) {
         } else {
            }
   if (v3d_qpu_mul_op_num_src(inst->alu.mul.op) > 1) {
            if (devinfo->ver < 71) {
         } else {
                     switch (inst->alu.add.op) {
   case V3D_QPU_A_VPMSETUP:
            /* Could distinguish read/write by unpacking the uniform. */
               case V3D_QPU_A_STVPMV:
   case V3D_QPU_A_STVPMD:
   case V3D_QPU_A_STVPMP:
                  case V3D_QPU_A_LDVPMV_IN:
   case V3D_QPU_A_LDVPMD_IN:
   case V3D_QPU_A_LDVPMG_IN:
   case V3D_QPU_A_LDVPMP:
                        case V3D_QPU_A_VPMWT:
                  case V3D_QPU_A_MSF:
                  case V3D_QPU_A_SETMSF:
   case V3D_QPU_A_SETREVF:
                  default:
                  switch (inst->alu.mul.op) {
   case V3D_QPU_M_MULTOP:
   case V3D_QPU_M_UMUL24:
            /* MULTOP sets rtop, and UMUL24 implicitly reads rtop and
   * resets it to 0.  We could possibly reorder umul24s relative
   * to each other, but for now just keep all the MUL parts in
   * order.
   */
      default:
                  if (inst->alu.add.op != V3D_QPU_A_NOP) {
               }
   if (inst->alu.mul.op != V3D_QPU_M_NOP) {
               }
   if (v3d_qpu_sig_writes_address(devinfo, &inst->sig)) {
                        if (v3d_qpu_writes_r3(devinfo, inst))
         if (v3d_qpu_writes_r4(devinfo, inst))
         if (v3d_qpu_writes_r5(devinfo, inst))
         if (v3d_qpu_writes_rf0_implicitly(devinfo, inst))
            /* If we add any more dependencies here we should consider whether we
      * also need to update qpu_inst_after_thrsw_valid_in_delay_slot.
      if (inst->sig.thrsw) {
            /* All accumulator contents and flags are undefined after the
   * switch.
   */
   for (int i = 0; i < ARRAY_SIZE(state->last_r); i++)
                        /* Scoreboard-locking operations have to stay after the last
                                    if (v3d_qpu_waits_on_tmu(inst)) {
            /* TMU loads are coming from a FIFO, so ordering is important.
   */
   add_write_dep(state, &state->last_tmu_read, n);
               /* Allow wrtmuc to be reordered with other instructions in the
      * same TMU sequence by using a read dependency on the last TMU
   * sequence terminator.
      if (inst->sig.wrtmuc)
            if (inst->sig.ldtlb | inst->sig.ldtlbu)
            if (inst->sig.ldvpm) {
                     /* At least for now, we're doing shared I/O segments, so queue
   * all writes after all reads.
   */
               /* inst->sig.ldunif or sideband uniform read */
   if (vir_has_uniform(qinst))
            /* Both unifa and ldunifa must preserve ordering */
   if (inst->sig.ldunifa || inst->sig.ldunifarf)
            if (v3d_qpu_reads_flags(inst))
         if (v3d_qpu_writes_flags(inst))
   }
      static void
   calculate_forward_deps(struct v3d_compile *c, struct dag *dag,
         {
                  memset(&state, 0, sizeof(state));
   state.dag = dag;
   state.devinfo = c->devinfo;
            list_for_each_entry(struct schedule_node, node, schedule_list, link)
   }
      static void
   calculate_reverse_deps(struct v3d_compile *c, struct dag *dag,
         {
                  memset(&state, 0, sizeof(state));
   state.dag = dag;
   state.devinfo = c->devinfo;
            list_for_each_entry_rev(struct schedule_node, node, schedule_list,
               }
      struct choose_scoreboard {
         struct dag *dag;
   int tick;
   int last_magic_sfu_write_tick;
   int last_stallable_sfu_reg;
   int last_stallable_sfu_tick;
   int last_ldvary_tick;
   int last_unifa_write_tick;
   int last_uniforms_reset_tick;
   int last_thrsw_tick;
   int last_branch_tick;
   int last_setmsf_tick;
   bool first_thrsw_emitted;
   bool last_thrsw_emitted;
   bool fixup_ldvary;
   int ldvary_count;
   int pending_ldtmu_count;
            /* V3D 7.x */
   int last_implicit_rf0_write_tick;
   };
      static bool
   mux_reads_too_soon(struct choose_scoreboard *scoreboard,
         {
         switch (mux) {
   case V3D_QPU_MUX_R4:
                        case V3D_QPU_MUX_R5:
            if (scoreboard->tick - scoreboard->last_ldvary_tick <= 1)
      default:
                  }
      static bool
   reads_too_soon(struct choose_scoreboard *scoreboard,
         {
         switch (raddr) {
   case 0: /* ldvary delayed write of C coefficient to rf0 */
            if (scoreboard->tick - scoreboard->last_ldvary_tick <= 1)
      default:
                  }
      static bool
   reads_too_soon_after_write(const struct v3d_device_info *devinfo,
               {
                  /* XXX: Branching off of raddr. */
   if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH)
                     if (inst->alu.add.op != V3D_QPU_A_NOP) {
            if (v3d_qpu_add_op_num_src(inst->alu.add.op) > 0) {
            if (devinfo->ver < 71) {
         if (mux_reads_too_soon(scoreboard, inst, inst->alu.add.a.mux))
   } else {
            }
   if (v3d_qpu_add_op_num_src(inst->alu.add.op) > 1) {
            if (devinfo->ver < 71) {
         if (mux_reads_too_soon(scoreboard, inst, inst->alu.add.b.mux))
   } else {
                  if (inst->alu.mul.op != V3D_QPU_M_NOP) {
            if (v3d_qpu_mul_op_num_src(inst->alu.mul.op) > 0) {
            if (devinfo->ver < 71) {
         if (mux_reads_too_soon(scoreboard, inst, inst->alu.mul.a.mux))
   } else {
            }
   if (v3d_qpu_mul_op_num_src(inst->alu.mul.op) > 1) {
            if (devinfo->ver < 71) {
         if (mux_reads_too_soon(scoreboard, inst, inst->alu.mul.b.mux))
   } else {
                           }
      static bool
   writes_too_soon_after_write(const struct v3d_device_info *devinfo,
               {
                  /* Don't schedule any other r4 write too soon after an SFU write.
      * This would normally be prevented by dependency tracking, but might
   * occur if a dead SFU computation makes it to scheduling.
      if (scoreboard->tick - scoreboard->last_magic_sfu_write_tick < 2 &&
                  if (devinfo->ver <= 42)
            /* Don't schedule anything that writes rf0 right after ldvary, since
      * that would clash with the ldvary's delayed rf0 write (the exception
   * is another ldvary, since its implicit rf0 write would also have
   * one cycle of delay and would not clash).
      if (scoreboard->last_ldvary_tick + 1 == scoreboard->tick &&
         (v3d71_qpu_writes_waddr_explicitly(devinfo, inst, 0) ||
   (v3d_qpu_writes_rf0_implicitly(devinfo, inst) &&
   !inst->sig.ldvary))) {
            }
      static bool
   scoreboard_is_locked(struct choose_scoreboard *scoreboard,
         {
         if (lock_scoreboard_on_first_thrsw) {
                        return scoreboard->last_thrsw_emitted &&
   }
      static bool
   pixel_scoreboard_too_soon(struct v3d_compile *c,
               {
         return qpu_inst_is_tlb(inst) &&
         }
      static bool
   qpu_instruction_uses_rf(const struct v3d_device_info *devinfo,
                        if (inst->type != V3D_QPU_INSTR_TYPE_ALU)
            if (devinfo->ver < 71) {
                                 if (v3d_qpu_uses_mux(inst, V3D_QPU_MUX_B) &&
      } else {
                        }
      static bool
   read_stalls(const struct v3d_device_info *devinfo,
               {
         return scoreboard->tick == scoreboard->last_stallable_sfu_tick + 1 &&
         }
      /* We define a max schedule priority to allow negative priorities as result of
   * subtracting this max when an instruction stalls. So instructions that
   * stall have lower priority than regular instructions. */
   #define MAX_SCHEDULE_PRIORITY 16
      static int
   get_instruction_priority(const struct v3d_device_info *devinfo,
         {
         uint32_t baseline_score;
            /* Schedule TLB operations as late as possible, to get more
      * parallelism between shaders.
      if (qpu_inst_is_tlb(inst))
                  /* Empirical testing shows that using priorities to hide latency of
      * TMU operations when scheduling QPU leads to slightly worse
   * performance, even at 2 threads. We think this is because the thread
   * switching is already quite effective at hiding latency and NIR
   * scheduling (and possibly TMU pipelining too) are sufficient to hide
   * TMU latency, so piling up on that here doesn't provide any benefits
   * and instead may cause us to postpone critical paths that depend on
      #if 0
         /* Schedule texture read results collection late to hide latency. */
   if (v3d_qpu_waits_on_tmu(inst))
         #endif
            /* Default score for things that aren't otherwise special. */
   baseline_score = next_score;
      #if 0
         /* Schedule texture read setup early to hide their latency better. */
   if (v3d_qpu_writes_tmu(devinfo, inst))
         #endif
            /* We should increase the maximum if we assert here */
            }
      enum {
         V3D_PERIPHERAL_VPM_READ           = (1 << 0),
   V3D_PERIPHERAL_VPM_WRITE          = (1 << 1),
   V3D_PERIPHERAL_VPM_WAIT           = (1 << 2),
   V3D_PERIPHERAL_SFU                = (1 << 3),
   V3D_PERIPHERAL_TMU_WRITE          = (1 << 4),
   V3D_PERIPHERAL_TMU_READ           = (1 << 5),
   V3D_PERIPHERAL_TMU_WAIT           = (1 << 6),
   V3D_PERIPHERAL_TMU_WRTMUC_SIG     = (1 << 7),
   V3D_PERIPHERAL_TSY                = (1 << 8),
   V3D_PERIPHERAL_TLB_READ           = (1 << 9),
   };
      static uint32_t
   qpu_peripherals(const struct v3d_device_info *devinfo,
         {
         uint32_t result = 0;
   if (v3d_qpu_reads_vpm(inst))
         if (v3d_qpu_writes_vpm(inst))
         if (v3d_qpu_waits_vpm(inst))
            if (v3d_qpu_writes_tmu(devinfo, inst))
         if (inst->sig.ldtmu)
         if (inst->sig.wrtmuc)
            if (v3d_qpu_uses_sfu(inst))
            if (v3d_qpu_reads_tlb(inst))
         if (v3d_qpu_writes_tlb(inst))
            if (inst->type == V3D_QPU_INSTR_TYPE_ALU) {
            if (inst->alu.add.op != V3D_QPU_A_NOP &&
      inst->alu.add.magic_write &&
                                 }
      static bool
   qpu_compatible_peripheral_access(const struct v3d_device_info *devinfo,
               {
         const uint32_t a_peripherals = qpu_peripherals(devinfo, a);
            /* We can always do one peripheral access per instruction. */
   if (util_bitcount(a_peripherals) + util_bitcount(b_peripherals) <= 1)
            if (devinfo->ver < 41)
            /* V3D 4.x can't do more than one peripheral access except in a
      * few cases:
      if (devinfo->ver <= 42) {
            /* WRTMUC signal with TMU register write (other than tmuc). */
   if (a_peripherals == V3D_PERIPHERAL_TMU_WRTMUC_SIG &&
      b_peripherals == V3D_PERIPHERAL_TMU_WRITE) {
      }
   if (b_peripherals == V3D_PERIPHERAL_TMU_WRTMUC_SIG &&
                        /* TMU read with VPM read/write. */
   if (a_peripherals == V3D_PERIPHERAL_TMU_READ &&
      (b_peripherals == V3D_PERIPHERAL_VPM_READ ||
      b_peripherals == V3D_PERIPHERAL_VPM_WRITE)) {
   }
   if (b_peripherals == V3D_PERIPHERAL_TMU_READ &&
      (a_peripherals == V3D_PERIPHERAL_VPM_READ ||
                           /* V3D 7.x can't have more than one of these restricted peripherals */
   const uint32_t restricted = V3D_PERIPHERAL_TMU_WRITE |
                                          const uint32_t a_restricted = a_peripherals & restricted;
   const uint32_t b_restricted = b_peripherals & restricted;
   if (a_restricted && b_restricted) {
            /* WRTMUC signal with TMU register write (other than tmuc) is
   * allowed though.
   */
   if (!((a_restricted == V3D_PERIPHERAL_TMU_WRTMUC_SIG &&
         b_restricted == V3D_PERIPHERAL_TMU_WRITE &&
   v3d_qpu_writes_tmu_not_tmuc(devinfo, b)) ||
   (b_restricted == V3D_PERIPHERAL_TMU_WRTMUC_SIG &&
   a_restricted == V3D_PERIPHERAL_TMU_WRITE &&
               /* Only one TMU read per instruction */
   if ((a_peripherals & V3D_PERIPHERAL_TMU_READ) &&
         (b_peripherals & V3D_PERIPHERAL_TMU_READ)) {
            /* Only one TLB access per instruction */
   if ((a_peripherals & (V3D_PERIPHERAL_TLB_WRITE |
               (b_peripherals & (V3D_PERIPHERAL_TLB_WRITE |
                  }
      /* Compute a bitmask of which rf registers are used between
   * the two instructions.
   */
   static uint64_t
   qpu_raddrs_used(const struct v3d_qpu_instr *a,
         {
         assert(a->type == V3D_QPU_INSTR_TYPE_ALU);
            uint64_t raddrs_used = 0;
   if (v3d_qpu_uses_mux(a, V3D_QPU_MUX_A))
         if (!a->sig.small_imm_b && v3d_qpu_uses_mux(a, V3D_QPU_MUX_B))
         if (v3d_qpu_uses_mux(b, V3D_QPU_MUX_A))
         if (!b->sig.small_imm_b && v3d_qpu_uses_mux(b, V3D_QPU_MUX_B))
            }
      /* Takes two instructions and attempts to merge their raddr fields (including
   * small immediates) into one merged instruction. For V3D 4.x, returns false
   * if the two instructions access more than two different rf registers between
   * them, or more than one rf register and one small immediate. For 7.x returns
   * false if both instructions use small immediates.
   */
   static bool
   qpu_merge_raddrs(struct v3d_qpu_instr *result,
                     {
         if (devinfo->ver >= 71) {
            assert(add_instr->sig.small_imm_a +
         assert(add_instr->sig.small_imm_c +
         assert(mul_instr->sig.small_imm_a +
                        result->sig.small_imm_a = add_instr->sig.small_imm_a;
                        return (result->sig.small_imm_a +
                              uint64_t raddrs_used = qpu_raddrs_used(add_instr, mul_instr);
            if (naddrs > 2)
            if ((add_instr->sig.small_imm_b || mul_instr->sig.small_imm_b)) {
                                                result->sig.small_imm_b = true;
               if (naddrs == 0)
            int raddr_a = ffsll(raddrs_used) - 1;
   raddrs_used &= ~(1ll << raddr_a);
            if (!result->sig.small_imm_b) {
            if (v3d_qpu_uses_mux(add_instr, V3D_QPU_MUX_B) &&
      raddr_a == add_instr->raddr_b) {
         if (add_instr->alu.add.a.mux == V3D_QPU_MUX_B)
         if (add_instr->alu.add.b.mux == V3D_QPU_MUX_B &&
      v3d_qpu_add_op_num_src(add_instr->alu.add.op) > 1) {
   }
   if (v3d_qpu_uses_mux(mul_instr, V3D_QPU_MUX_B) &&
      raddr_a == mul_instr->raddr_b) {
         if (mul_instr->alu.mul.a.mux == V3D_QPU_MUX_B)
         if (mul_instr->alu.mul.b.mux == V3D_QPU_MUX_B &&
         }
   if (!raddrs_used)
            int raddr_b = ffsll(raddrs_used) - 1;
   result->raddr_b = raddr_b;
   if (v3d_qpu_uses_mux(add_instr, V3D_QPU_MUX_A) &&
         raddr_b == add_instr->raddr_a) {
      if (add_instr->alu.add.a.mux == V3D_QPU_MUX_A)
         if (add_instr->alu.add.b.mux == V3D_QPU_MUX_A &&
      v3d_qpu_add_op_num_src(add_instr->alu.add.op) > 1) {
   }
   if (v3d_qpu_uses_mux(mul_instr, V3D_QPU_MUX_A) &&
         raddr_b == mul_instr->raddr_a) {
      if (mul_instr->alu.mul.a.mux == V3D_QPU_MUX_A)
         if (mul_instr->alu.mul.b.mux == V3D_QPU_MUX_A &&
      v3d_qpu_mul_op_num_src(mul_instr->alu.mul.op) > 1) {
            }
      static bool
   can_do_add_as_mul(enum v3d_qpu_add_op op)
   {
         switch (op) {
   case V3D_QPU_A_ADD:
   case V3D_QPU_A_SUB:
         default:
         }
      static enum v3d_qpu_mul_op
   add_op_as_mul_op(enum v3d_qpu_add_op op)
   {
         switch (op) {
   case V3D_QPU_A_ADD:
         case V3D_QPU_A_SUB:
         default:
         }
      static void
   qpu_convert_add_to_mul(const struct v3d_device_info *devinfo,
         {
         STATIC_ASSERT(sizeof(inst->alu.mul) == sizeof(inst->alu.add));
   assert(inst->alu.add.op != V3D_QPU_A_NOP);
            memcpy(&inst->alu.mul, &inst->alu.add, sizeof(inst->alu.mul));
   inst->alu.mul.op = add_op_as_mul_op(inst->alu.add.op);
            inst->flags.mc = inst->flags.ac;
   inst->flags.mpf = inst->flags.apf;
   inst->flags.muf = inst->flags.auf;
   inst->flags.ac = V3D_QPU_COND_NONE;
   inst->flags.apf = V3D_QPU_PF_NONE;
                     inst->alu.mul.a.unpack = inst->alu.add.a.unpack;
   inst->alu.mul.b.unpack = inst->alu.add.b.unpack;
   inst->alu.add.output_pack = V3D_QPU_PACK_NONE;
   inst->alu.add.a.unpack = V3D_QPU_UNPACK_NONE;
            if (devinfo->ver >= 71) {
            assert(!inst->sig.small_imm_c && !inst->sig.small_imm_d);
   assert(inst->sig.small_imm_a + inst->sig.small_imm_b <= 1);
   if (inst->sig.small_imm_a) {
               } else if (inst->sig.small_imm_b) {
            }
      static bool
   can_do_mul_as_add(const struct v3d_device_info *devinfo, enum v3d_qpu_mul_op op)
   {
         switch (op) {
   case V3D_QPU_M_MOV:
   case V3D_QPU_M_FMOV:
         default:
         }
      static enum v3d_qpu_mul_op
   mul_op_as_add_op(enum v3d_qpu_mul_op op)
   {
         switch (op) {
   case V3D_QPU_M_MOV:
         case V3D_QPU_M_FMOV:
         default:
         }
      static void
   qpu_convert_mul_to_add(struct v3d_qpu_instr *inst)
   {
         STATIC_ASSERT(sizeof(inst->alu.add) == sizeof(inst->alu.mul));
   assert(inst->alu.mul.op != V3D_QPU_M_NOP);
            memcpy(&inst->alu.add, &inst->alu.mul, sizeof(inst->alu.add));
   inst->alu.add.op = mul_op_as_add_op(inst->alu.mul.op);
            inst->flags.ac = inst->flags.mc;
   inst->flags.apf = inst->flags.mpf;
   inst->flags.auf = inst->flags.muf;
   inst->flags.mc = V3D_QPU_COND_NONE;
   inst->flags.mpf = V3D_QPU_PF_NONE;
            inst->alu.add.output_pack = inst->alu.mul.output_pack;
   inst->alu.add.a.unpack = inst->alu.mul.a.unpack;
   inst->alu.add.b.unpack = inst->alu.mul.b.unpack;
   inst->alu.mul.output_pack = V3D_QPU_PACK_NONE;
   inst->alu.mul.a.unpack = V3D_QPU_UNPACK_NONE;
            assert(!inst->sig.small_imm_a && !inst->sig.small_imm_b);
   assert(inst->sig.small_imm_c + inst->sig.small_imm_d <= 1);
   if (inst->sig.small_imm_c) {
               } else if (inst->sig.small_imm_d) {
               }
      static bool
   qpu_merge_inst(const struct v3d_device_info *devinfo,
                     {
         if (a->type != V3D_QPU_INSTR_TYPE_ALU ||
         b->type != V3D_QPU_INSTR_TYPE_ALU) {
            if (!qpu_compatible_peripheral_access(devinfo, a, b))
            struct v3d_qpu_instr merge = *a;
            struct v3d_qpu_instr mul_inst;
   if (b->alu.add.op != V3D_QPU_A_NOP) {
                                                            }
   /* If a's add op is used but its mul op is not, then see if we
   * can convert either a's add op or b's add op to a mul op
   * so we can merge.
   */
   else if (a->alu.mul.op == V3D_QPU_M_NOP &&
                                                                  } else if (a->alu.mul.op == V3D_QPU_M_NOP &&
                                                                        } else {
               struct v3d_qpu_instr add_inst;
   if (b->alu.mul.op != V3D_QPU_M_NOP) {
                                                            }
   /* If a's mul op is used but its add op is not, then see if we
   * can convert either a's mul op or b's mul op to an add op
   * so we can merge.
   */
   else if (a->alu.add.op == V3D_QPU_A_NOP &&
                                                                  } else if (a->alu.add.op == V3D_QPU_A_NOP &&
                                                                        } else {
               /* V3D 4.x and earlier use muxes to select the inputs for the ALUs and
      * they have restrictions on the number of raddrs that can be adressed
   * in a single instruction. In V3D 7.x, we don't have that restriction,
   * but we are still limited to a single small immediate per instruction.
      if (add_instr && mul_instr &&
         !qpu_merge_raddrs(&merge, add_instr, mul_instr, devinfo)) {
            merge.sig.thrsw |= b->sig.thrsw;
   merge.sig.ldunif |= b->sig.ldunif;
   merge.sig.ldunifrf |= b->sig.ldunifrf;
   merge.sig.ldunifa |= b->sig.ldunifa;
   merge.sig.ldunifarf |= b->sig.ldunifarf;
   merge.sig.ldtmu |= b->sig.ldtmu;
   merge.sig.ldvary |= b->sig.ldvary;
   merge.sig.ldvpm |= b->sig.ldvpm;
   merge.sig.ldtlb |= b->sig.ldtlb;
   merge.sig.ldtlbu |= b->sig.ldtlbu;
   merge.sig.ucb |= b->sig.ucb;
   merge.sig.rotate |= b->sig.rotate;
            if (v3d_qpu_sig_writes_address(devinfo, &a->sig) &&
         v3d_qpu_sig_writes_address(devinfo, &b->sig))
   merge.sig_addr |= b->sig_addr;
            uint64_t packed;
            *result = merge;
   /* No modifying the real instructions on failure. */
            }
      static inline bool
   try_skip_for_ldvary_pipelining(const struct v3d_qpu_instr *inst)
   {
         }
      static bool
   qpu_inst_after_thrsw_valid_in_delay_slot(struct v3d_compile *c,
                  static struct schedule_node *
   choose_instruction_to_schedule(struct v3d_compile *c,
               {
         struct schedule_node *chosen = NULL;
            /* Don't pair up anything with a thread switch signal -- emit_thrsw()
      * will handle pairing it along with filling the delay slots.
      if (prev_inst) {
                        bool ldvary_pipelining = c->s->info.stage == MESA_SHADER_FRAGMENT &&
         retry:
         list_for_each_entry(struct schedule_node, n, &scoreboard->dag->heads,
                           if (ldvary_pipelining && try_skip_for_ldvary_pipelining(inst)) {
                        /* Don't choose the branch instruction until it's the last one
   * left.  We'll move it up to fit its delay slots after we
   * choose it.
   */
   if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH &&
                        /* We need to have 3 delay slots between a write to unifa and
   * a follow-up ldunifa.
   */
                        /* "An instruction must not read from a location in physical
   *  regfile A or B that was written to by the previous
   *  instruction."
                                       /* "Before doing a TLB access a scoreboard wait must have been
   *  done. This happens either on the first or last thread
   *  switch, depending on a setting (scb_wait_on_first_thrsw) in
   *  the shader state."
                        /* ldunif and ldvary both write the same register (r5 for v42
   * and below, rf0 for v71), but ldunif does so a tick sooner.
   * If the ldvary's register wasn't used, then ldunif might
   * otherwise get scheduled so ldunif and ldvary try to update
   * the register in the same tick.
   */
   if ((inst->sig.ldunif || inst->sig.ldunifa) &&
                        /* If we are in a thrsw delay slot check that this instruction
   * is valid for that.
   */
   if (scoreboard->last_thrsw_tick + 2 >= scoreboard->tick &&
      !qpu_inst_after_thrsw_valid_in_delay_slot(c, scoreboard,
                     if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH) {
            /* Don't try to put a branch in the delay slots of another
   * branch or a unifa write.
   */
                              /* No branch with cond != 0,2,3 and msfign != 0 after
   * setmsf.
   */
   if (scoreboard->last_setmsf_tick == scoreboard->tick - 1 &&
      inst->branch.msfign != V3D_QPU_MSFIGN_NONE &&
   inst->branch.cond != V3D_QPU_BRANCH_COND_ALWAYS &&
                        /* If we're trying to pair with another instruction, check
   * that they're compatible.
   */
   if (prev_inst) {
            /* Don't pair up a thread switch signal -- we'll
                                                /* Simulator complains if we have two uniforms loaded in
      * the the same instruction, which could happen if we
   * have a ldunif or sideband uniform and we pair that
   * with ldunifa.
   */
                              if ((prev_inst->inst->qpu.sig.ldunifa ||
                              /* Don't merge TLB instructions before we have acquired
                              /* When we successfully pair up an ldvary we then try
   * to merge it into the previous instruction if
   * possible to improve pipelining. Don't pick up the
   * ldvary now if the follow-up fixup would place
   * it in the delay slots of a thrsw, which is not
   * allowed and would prevent the fixup from being
   * successful. In V3D 7.x we can allow this to happen
   * as long as it is not the last delay slot.
   */
   if (inst->sig.ldvary) {
         if (c->devinfo->ver <= 42 &&
         scoreboard->last_thrsw_tick + 2 >=
   scoreboard->tick - 1) {
   }
   if (c->devinfo->ver >= 71 &&
                              /* We can emit a new tmu lookup with a previous ldtmu
   * if doing this would free just enough space in the
   * TMU output fifo so we don't overflow, however, this
   * is only safe if the ldtmu cannot stall.
   *
   * A ldtmu can stall if it is not the first following a
   * thread switch and corresponds to the first word of a
   * read request.
   *
   * FIXME: For now we forbid pairing up a new lookup
   * with a previous ldtmu that is not the first after a
   * thrsw if that could overflow the TMU output fifo
   * regardless of whether the ldtmu is reading the first
   * word of a TMU result or not, since we don't track
   * this aspect in the compiler yet.
   */
   if (prev_inst->inst->qpu.sig.ldtmu &&
                                    struct v3d_qpu_instr merged_inst;
   if (!qpu_merge_inst(c->devinfo, &merged_inst,
                              if (read_stalls(c->devinfo, scoreboard, inst)) {
            /* Don't merge an instruction that stalls */
   if (prev_inst)
         else {
         /* Any instruction that don't stall will have
                     /* Found a valid instruction.  If nothing better comes along,
   * this one works.
   */
   if (!chosen) {
                              if (prio > chosen_prio) {
                                    if (n->delay > chosen->delay) {
               } else if (n->delay < chosen->delay) {
               /* If we did not find any instruction to schedule but we discarded
      * some of them to prioritize ldvary pipelining, try again.
      if (!chosen && !prev_inst && skipped_insts_for_ldvary_pipelining) {
            skipped_insts_for_ldvary_pipelining = false;
               if (chosen && chosen->inst->qpu.sig.ldvary) {
            scoreboard->ldvary_count++;
   /* If we are pairing an ldvary, flag it so we can fix it up for
   * optimal pipelining of ldvary sequences.
   */
               }
      static void
   update_scoreboard_for_magic_waddr(struct choose_scoreboard *scoreboard,
               {
         if (v3d_qpu_magic_waddr_is_sfu(waddr))
         else if (devinfo->ver >= 40 && waddr == V3D_QPU_WADDR_UNIFA)
   }
      static void
   update_scoreboard_for_sfu_stall_waddr(struct choose_scoreboard *scoreboard,
         {
         if (v3d_qpu_instr_is_sfu(inst)) {
               }
      static void
   update_scoreboard_tmu_tracking(struct choose_scoreboard *scoreboard,
         {
         /* Track if the have seen any ldtmu after the last thread switch */
   if (scoreboard->tick == scoreboard->last_thrsw_tick + 2)
            /* Track the number of pending ldtmu instructions for outstanding
      * TMU lookups.
      scoreboard->pending_ldtmu_count += inst->ldtmu_count;
   if (inst->qpu.sig.ldtmu) {
            assert(scoreboard->pending_ldtmu_count > 0);
      }
      static void
   set_has_rf0_flops_conflict(struct choose_scoreboard *scoreboard,
               {
         if (scoreboard->last_implicit_rf0_write_tick == scoreboard->tick &&
         v3d_qpu_sig_writes_address(devinfo, &inst->sig) &&
   !inst->sig_magic) {
   }
      static void
   update_scoreboard_for_rf0_flops(struct choose_scoreboard *scoreboard,
               {
         if (devinfo->ver < 71)
            /* Thread switch restrictions:
      *
   * At the point of a thread switch or thread end (when the actual
   * thread switch or thread end happens, not when the signalling
   * instruction is processed):
   *
   *    - If the most recent write to rf0 was from a ldunif, ldunifa, or
   *      ldvary instruction in which another signal also wrote to the
   *      register file, and the final instruction of the thread section
   *      contained a signal which wrote to the register file, then the
   *      value of rf0 is undefined at the start of the new section
   *
   * Here we use the scoreboard to track if our last rf0 implicit write
   * happens at the same time that another signal writes the register
   * file (has_rf0_flops_conflict). We will use that information when
   * scheduling thrsw instructions to avoid putting anything in their
               /* Reset tracking if we have an explicit rf0 write or we are starting
      * a new thread section.
      if (v3d71_qpu_writes_waddr_explicitly(devinfo, inst, 0) ||
         scoreboard->tick - scoreboard->last_thrsw_tick == 3) {
                  if (v3d_qpu_writes_rf0_implicitly(devinfo, inst)) {
                        }
      static void
   update_scoreboard_for_chosen(struct choose_scoreboard *scoreboard,
               {
                  if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH)
                     if (inst->alu.add.op != V3D_QPU_A_NOP)  {
            if (inst->alu.add.magic_write) {
            update_scoreboard_for_magic_waddr(scoreboard,
      } else {
                                    if (inst->alu.mul.op != V3D_QPU_M_NOP) {
            if (inst->alu.mul.magic_write) {
            update_scoreboard_for_magic_waddr(scoreboard,
            if (v3d_qpu_sig_writes_address(devinfo, &inst->sig) && inst->sig_magic) {
            update_scoreboard_for_magic_waddr(scoreboard,
               if (inst->sig.ldvary)
                     }
      static void
   dump_state(const struct v3d_device_info *devinfo, struct dag *dag)
   {
         list_for_each_entry(struct schedule_node, n, &dag->heads, dag.link) {
                                 util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
                                       fprintf(stderr, "                 - ");
   v3d_qpu_dump(devinfo, &child->inst->qpu);
   fprintf(stderr, " (%d parents, %c)\n",
   }
      static uint32_t magic_waddr_latency(const struct v3d_device_info *devinfo,
               {
         /* Apply some huge latency between texture fetch requests and getting
      * their results back.
   *
   * FIXME: This is actually pretty bogus.  If we do:
   *
   * mov tmu0_s, a
   * <a bit of math>
   * mov tmu0_s, b
   * load_tmu0
   * <more math>
   * load_tmu0
   *
   * we count that as worse than
   *
   * mov tmu0_s, a
   * mov tmu0_s, b
   * <lots of math>
   * load_tmu0
   * <more math>
   * load_tmu0
   *
   * because we associate the first load_tmu0 with the *second* tmu0_s.
      if (v3d_qpu_magic_waddr_is_tmu(devinfo, waddr) &&
         v3d_qpu_waits_on_tmu(after)) {
            /* Assume that anything depending on us is consuming the SFU result. */
   if (v3d_qpu_magic_waddr_is_sfu(waddr))
            }
      static uint32_t
   instruction_latency(const struct v3d_device_info *devinfo,
         {
         const struct v3d_qpu_instr *before_inst = &before->inst->qpu;
   const struct v3d_qpu_instr *after_inst = &after->inst->qpu;
            if (before_inst->type != V3D_QPU_INSTR_TYPE_ALU ||
                  if (v3d_qpu_instr_is_sfu(before_inst))
            if (before_inst->alu.add.op != V3D_QPU_A_NOP &&
         before_inst->alu.add.magic_write) {
      latency = MAX2(latency,
                     if (before_inst->alu.mul.op != V3D_QPU_M_NOP &&
         before_inst->alu.mul.magic_write) {
      latency = MAX2(latency,
                     }
      /** Recursive computation of the delay member of a node. */
   static void
   compute_delay(struct dag_node *node, void *state)
   {
         struct schedule_node *n = (struct schedule_node *)node;
                     util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
                           n->delay = MAX2(n->delay, (child->delay +
      }
      /* Removes a DAG head, but removing only the WAR edges. (dag_prune_head()
   * should be called on it later to finish pruning the other edges).
   */
   static void
   pre_remove_head(struct dag *dag, struct schedule_node *n)
   {
                  util_dynarray_foreach(&n->dag.edges, struct dag_edge, edge) {
               }
      static void
   mark_instruction_scheduled(const struct v3d_device_info *devinfo,
                     {
         if (!node)
            util_dynarray_foreach(&node->dag.edges, struct dag_edge, edge) {
                                                      }
   }
      static void
   insert_scheduled_instruction(struct v3d_compile *c,
                     {
                  update_scoreboard_for_chosen(scoreboard, inst, c->devinfo);
   c->qpu_inst_count++;
   }
      static struct qinst *
   vir_nop()
   {
         struct qreg undef = vir_nop_reg();
            }
      static void
   emit_nop(struct v3d_compile *c, struct qblock *block,
         {
         }
      static bool
   qpu_inst_valid_in_thrend_slot(struct v3d_compile *c,
         {
                  if (slot == 2 && qinst->is_tlb_z_write)
            if (slot > 0 && qinst->uniform != ~0)
            if (c->devinfo->ver <= 42 && v3d_qpu_waits_vpm(inst))
            if (inst->sig.ldvary)
            if (inst->type == V3D_QPU_INSTR_TYPE_ALU) {
            /* GFXH-1625: TMUWT not allowed in the final instruction. */
   if (c->devinfo->ver <= 42 && slot == 2 &&
                        if (c->devinfo->ver <= 42) {
            /* No writing physical registers at the end. */
   bool add_is_nop = inst->alu.add.op == V3D_QPU_A_NOP;
   bool mul_is_nop = inst->alu.mul.op == V3D_QPU_M_NOP;
                              if (v3d_qpu_sig_writes_address(c->devinfo, &inst->sig) &&
                     if (c->devinfo->ver >= 71) {
            /* The thread end instruction must not write to the
   * register file via the add/mul ALUs.
   */
   if (slot == 0 &&
                                          if (c->devinfo->ver <= 42) {
            /* RF0-2 might be overwritten during the delay slots by
                              if (inst->raddr_b < 3 &&
                           if (c->devinfo->ver >= 71) {
            /* RF2-3 might be overwritten during the delay slots by
   * fragment shader setup.
   */
                              if (v3d71_qpu_writes_waddr_explicitly(c->devinfo, inst, 2) ||
                  }
      /**
   * This is called when trying to merge a thrsw back into the instruction stream
   * of instructions that were scheduled *before* the thrsw signal to fill its
   * delay slots. Because the actual execution of the thrsw happens after the
   * delay slots, it is usually safe to do this, but there are some cases that
   * need special care.
   */
   static bool
   qpu_inst_before_thrsw_valid_in_delay_slot(struct v3d_compile *c,
                     {
         /* No scheduling SFU when the result would land in the other
      * thread.  The simulator complains for safety, though it
   * would only occur for dead code in our case.
      if (slot > 0 && v3d_qpu_instr_is_legacy_sfu(&qinst->qpu))
            if (qinst->qpu.sig.ldvary) {
            if (c->devinfo->ver <= 42 && slot > 0)
                     /* unifa and the following 3 instructions can't overlap a
      * thread switch/end. The docs further clarify that this means
   * the cycle at which the actual thread switch/end happens
   * and not when the thrsw instruction is processed, which would
   * be after the 2 delay slots following the thrsw instruction.
   * This means that we can move up a thrsw up to the instruction
   * right after unifa:
   *
   * unifa, r5
   * thrsw
   * delay slot 1
   * delay slot 2
   * Thread switch happens here, 4 instructions away from unifa
      if (v3d_qpu_writes_unifa(c->devinfo, &qinst->qpu))
            /* See comment when we set has_rf0_flops_conflict for details */
   if (c->devinfo->ver >= 71 &&
         slot == 2 &&
   v3d_qpu_sig_writes_address(c->devinfo, &qinst->qpu.sig) &&
   !qinst->qpu.sig_magic) {
      if (scoreboard->has_rf0_flops_conflict)
                     }
      /**
   * This is called for instructions scheduled *after* a thrsw signal that may
   * land in the delay slots of the thrsw. Because these instructions were
   * scheduled after the thrsw, we need to be careful when placing them into
   * the delay slots, since that means that we are moving them ahead of the
   * thread switch and we need to ensure that is not a problem.
   */
   static bool
   qpu_inst_after_thrsw_valid_in_delay_slot(struct v3d_compile *c,
               {
         const uint32_t slot = scoreboard->tick - scoreboard->last_thrsw_tick;
            /* We merge thrsw instructions back into the instruction stream
      * manually, so any instructions scheduled after a thrsw should be
   * in the actual delay slots and not in the same slot as the thrsw.
               /* No emitting a thrsw while the previous thrsw hasn't happened yet. */
   if (qinst->qpu.sig.thrsw)
            /* The restrictions for instructions scheduled before the the thrsw
      * also apply to instructions scheduled after the thrsw that we want
   * to place in its delay slots.
      if (!qpu_inst_before_thrsw_valid_in_delay_slot(c, scoreboard, qinst, slot))
            /* TLB access is disallowed until scoreboard wait is executed, which
      * we do on the last thread switch.
      if (qpu_inst_is_tlb(&qinst->qpu))
            /* Instruction sequence restrictions: Branch is not allowed in delay
      * slots of a thrsw.
      if (qinst->qpu.type == V3D_QPU_INSTR_TYPE_BRANCH)
            /* Miscellaneous restrictions: At the point of a thrsw we need to have
      * at least one outstanding lookup or TSY wait.
   *
   * So avoid placing TMU instructions scheduled after the thrsw into
   * its delay slots or we may be compromising the integrity of our TMU
   * sequences. Also, notice that if we moved these instructions into
   * the delay slots of a previous thrsw we could overflow our TMU output
   * fifo, since we could be effectively pipelining a lookup scheduled
   * after the thrsw into the sequence before the thrsw.
      if (v3d_qpu_writes_tmu(c->devinfo, &qinst->qpu) ||
         qinst->qpu.sig.wrtmuc) {
            /* Don't move instructions that wait on the TMU before the thread switch
      * happens since that would make the current thread stall before the
   * switch, which is exactly what we want to avoid with the thrsw
   * instruction.
      if (v3d_qpu_waits_on_tmu(&qinst->qpu))
            /* A thread switch invalidates all accumulators, so don't place any
      * instructions that write accumulators into the delay slots.
      if (v3d_qpu_writes_accum(c->devinfo, &qinst->qpu))
            /* Multop has an implicit write to the rtop register which is an
      * specialized accumulator that is only used with this instruction.
      if (qinst->qpu.alu.mul.op == V3D_QPU_M_MULTOP)
            /* Flags are invalidated across a thread switch, so dont' place
      * instructions that write flags into delay slots.
      if (v3d_qpu_writes_flags(&qinst->qpu))
            /* TSY sync ops materialize at the point of the next thread switch,
      * therefore, if we have a TSY sync right after a thread switch, we
   * cannot place it in its delay slots, or we would be moving the sync
   * to the thrsw before it instead.
      if (qinst->qpu.alu.add.op == V3D_QPU_A_BARRIERID)
            }
      static bool
   valid_thrsw_sequence(struct v3d_compile *c, struct choose_scoreboard *scoreboard,
               {
         for (int slot = 0; slot < instructions_in_sequence; slot++) {
            if (!qpu_inst_before_thrsw_valid_in_delay_slot(c, scoreboard,
                        if (is_thrend &&
                        /* Note that the list is circular, so we can only do this up
   * to instructions_in_sequence.
               }
      /**
   * Emits a THRSW signal in the stream, trying to move it up to pair with
   * another instruction.
   */
   static int
   emit_thrsw(struct v3d_compile *c,
            struct qblock *block,
   struct choose_scoreboard *scoreboard,
      {
                  /* There should be nothing in a thrsw inst being scheduled other than
      * the signal bits.
      assert(inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU);
   assert(inst->qpu.alu.add.op == V3D_QPU_A_NOP);
            /* Don't try to emit a thrsw in the delay slots of a previous thrsw
      * or branch.
      while (scoreboard->last_thrsw_tick + 2 >= scoreboard->tick) {
               }
   while (scoreboard->last_branch_tick + 3 >= scoreboard->tick) {
                        /* Find how far back into previous instructions we can put the THRSW. */
   int slots_filled = 0;
   int invalid_sig_count = 0;
   int invalid_seq_count = 0;
   bool last_thrsw_after_invalid_ok = false;
   struct qinst *merge_inst = NULL;
   vir_for_each_inst_rev(prev_inst, block) {
            /* No emitting our thrsw while the previous thrsw hasn't
   * happened yet.
   */
   if (scoreboard->last_thrsw_tick + 3 >
                           if (!valid_thrsw_sequence(c, scoreboard,
                        /* Even if the current sequence isn't valid, we may
   * be able to get a valid sequence by trying to move the
   * thrsw earlier, so keep going.
                     struct v3d_qpu_sig sig = prev_inst->qpu.sig;
   sig.thrsw = true;
   uint32_t packed_sig;
   if (!v3d_qpu_sig_pack(c->devinfo, &sig, &packed_sig)) {
            /* If we can't merge the thrsw here because of signal
   * incompatibility, keep going, we might be able to
   * merge it in an earlier instruction.
                     /* For last thrsw we need 2 consecutive slots that are
   * thrsw compatible, so if we have previously jumped over
   * an incompatible signal, flag that we have found the first
   * valid slot here and keep going.
   */
   if (inst->is_last_thrsw && invalid_sig_count > 0 &&
      !last_thrsw_after_invalid_ok) {
                           /* We can merge the thrsw in this instruction */
   last_thrsw_after_invalid_ok = false;
         cont_block:
                              /* If we jumped over a signal incompatibility and did not manage to
      * merge the thrsw in the end, we need to adjust slots filled to match
   * the last valid merge point.
      assert((invalid_sig_count == 0 && invalid_seq_count == 0) ||
         if (invalid_sig_count > 0)
         if (invalid_seq_count > 0)
            bool needs_free = false;
   if (merge_inst) {
            merge_inst->qpu.sig.thrsw = true;
      } else {
            scoreboard->last_thrsw_tick = scoreboard->tick;
   insert_scheduled_instruction(c, block, scoreboard, inst);
   time++;
                        /* If we're emitting the last THRSW (other than program end), then
      * signal that to the HW by emitting two THRSWs in a row.
      if (inst->is_last_thrsw) {
            if (slots_filled <= 1) {
               }
   struct qinst *second_inst =
                     /* Make sure the thread end executes within the program lifespan */
   if (is_thrend) {
            for (int i = 0; i < 3 - slots_filled; i++) {
                     /* If we put our THRSW into another instruction, free up the
      * instruction that didn't end up scheduled into the list.
      if (needs_free)
            }
      static bool
   qpu_inst_valid_in_branch_delay_slot(struct v3d_compile *c, struct qinst *inst)
   {
         if (inst->qpu.type == V3D_QPU_INSTR_TYPE_BRANCH)
            if (inst->qpu.sig.thrsw)
            if (v3d_qpu_writes_unifa(c->devinfo, &inst->qpu))
            if (vir_has_uniform(inst))
            }
      static void
   emit_branch(struct v3d_compile *c,
            struct qblock *block,
      {
                  /* We should've not picked up a branch for the delay slots of a previous
      * thrsw, branch or unifa write instruction.
      int branch_tick = scoreboard->tick;
   assert(scoreboard->last_thrsw_tick + 2 < branch_tick);
   assert(scoreboard->last_branch_tick + 3 < branch_tick);
            /* V3D 4.x can't place a branch with msfign != 0 and cond != 0,2,3 after
      * setmsf.
      bool is_safe_msf_branch =
            c->devinfo->ver >= 71 ||
   inst->qpu.branch.msfign == V3D_QPU_MSFIGN_NONE ||
   inst->qpu.branch.cond == V3D_QPU_BRANCH_COND_ALWAYS ||
      assert(scoreboard->last_setmsf_tick != branch_tick - 1 ||
            /* Insert the branch instruction */
            /* Now see if we can move the branch instruction back into the
      * instruction stream to fill its delay slots
      int slots_filled = 0;
   while (slots_filled < 3 && block->instructions.next != &inst->link) {
                           /* Can't move the branch instruction if that would place it
   * in the delay slots of other instructions.
   */
   if (scoreboard->last_branch_tick + 3 >=
                        if (scoreboard->last_thrsw_tick + 2 >=
                        if (scoreboard->last_unifa_write_tick + 3 >=
                        /* Do not move up a branch if it can disrupt an ldvary sequence
   * as that can cause stomping of the r5 register.
   */
   if (scoreboard->last_ldvary_tick + 2 >=
                        /* Can't move a conditional branch before the instruction
   * that writes the flags for its condition.
   */
   if (v3d_qpu_writes_flags(&prev_inst->qpu) &&
                                       if (!is_safe_msf_branch) {
            struct qinst *prev_prev_inst =
         if (prev_prev_inst->qpu.type == V3D_QPU_INSTR_TYPE_ALU &&
                     list_del(&prev_inst->link);
               block->branch_qpu_ip = c->qpu_inst_count - 1 - slots_filled;
            /* Fill any remaining delay slots.
      *
   * For unconditional branches we'll try to fill these with the
   * first instructions in the successor block after scheduling
   * all blocks when setting up branch targets.
      for (int i = 0; i < 3 - slots_filled; i++)
   }
      static bool
   alu_reads_register(const struct v3d_device_info *devinfo,
               {
         uint32_t num_src;
   if (add)
         else
            if (devinfo->ver <= 42) {
            enum v3d_qpu_mux mux_a, mux_b;
   if (add) {
               } else {
                        for (int i = 0; i < num_src; i++) {
            if (magic) {
         if (i == 0 && mux_a == index)
         if (i == 1 && mux_b == index)
   } else {
         if (i == 0 && mux_a == V3D_QPU_MUX_A &&
         inst->raddr_a == index) {
   }
   if (i == 0 && mux_a == V3D_QPU_MUX_B &&
         inst->raddr_b == index) {
   }
   if (i == 1 && mux_b == V3D_QPU_MUX_A &&
         inst->raddr_a == index) {
   }
   if (i == 1 && mux_b == V3D_QPU_MUX_B &&
                           assert(devinfo->ver >= 71);
            uint32_t raddr_a, raddr_b;
   if (add) {
               } else {
                        for (int i = 0; i < num_src; i++) {
            if (i == 0 && raddr_a == index)
                     }
      /**
   * This takes and ldvary signal merged into 'inst' and tries to move it up to
   * the previous instruction to get good pipelining of ldvary sequences,
   * transforming this:
   *
   * nop                  ; nop               ; ldvary.r4
   * nop                  ; fmul  r0, r4, rf0 ;
   * fadd  rf13, r0, r5   ; nop;              ; ldvary.r1  <-- inst
   *
   * into:
   *
   * nop                  ; nop               ; ldvary.r4
   * nop                  ; fmul  r0, r4, rf0 ; ldvary.r1
   * fadd  rf13, r0, r5   ; nop;              ;            <-- inst
   *
   * If we manage to do this successfully (we return true here), then flagging
   * the ldvary as "scheduled" may promote the follow-up fmul to a DAG head that
   * we will be able to pick up to merge into 'inst', leading to code like this:
   *
   * nop                  ; nop               ; ldvary.r4
   * nop                  ; fmul  r0, r4, rf0 ; ldvary.r1
   * fadd  rf13, r0, r5   ; fmul  r2, r1, rf0 ;            <-- inst
   */
   static bool
   fixup_pipelined_ldvary(struct v3d_compile *c,
                     {
                  /* We only call this if we have successfully merged an ldvary into a
      * previous instruction.
      assert(inst->type == V3D_QPU_INSTR_TYPE_ALU);
   assert(inst->sig.ldvary);
   uint32_t ldvary_magic = inst->sig_magic;
            /* The instruction in which we merged the ldvary cannot read
      * the ldvary destination, if it does, then moving the ldvary before
   * it would overwrite it.
      if (alu_reads_register(devinfo, inst, true, ldvary_magic, ldvary_index))
         if (alu_reads_register(devinfo, inst, false, ldvary_magic, ldvary_index))
            /* The implicit ldvary destination may not be written to by a signal
      * in the instruction following ldvary. Since we are planning to move
   * ldvary to the previous instruction, this means we need to check if
   * the current instruction has any other signal that could create this
   * conflict. The only other signal that can write to the implicit
   * ldvary destination that is compatible with ldvary in the same
   * instruction is ldunif.
      if (inst->sig.ldunif)
            /* The previous instruction can't write to the same destination as the
      * ldvary.
      struct qinst *prev = (struct qinst *) block->instructions.prev;
   if (!prev || prev->qpu.type != V3D_QPU_INSTR_TYPE_ALU)
            if (prev->qpu.alu.add.op != V3D_QPU_A_NOP) {
            if (prev->qpu.alu.add.magic_write == ldvary_magic &&
      prev->qpu.alu.add.waddr == ldvary_index) {
            if (prev->qpu.alu.mul.op != V3D_QPU_M_NOP) {
            if (prev->qpu.alu.mul.magic_write == ldvary_magic &&
      prev->qpu.alu.mul.waddr == ldvary_index) {
            /* The previous instruction cannot have a conflicting signal */
   if (v3d_qpu_sig_writes_address(devinfo, &prev->qpu.sig))
            uint32_t sig;
   struct v3d_qpu_sig new_sig = prev->qpu.sig;
   new_sig.ldvary = true;
   if (!v3d_qpu_sig_pack(devinfo, &new_sig, &sig))
            /* The previous instruction cannot use flags since ldvary uses the
      * 'cond' instruction field to store the destination.
      if (v3d_qpu_writes_flags(&prev->qpu))
         if (v3d_qpu_reads_flags(&prev->qpu))
            /* We can't put an ldvary in the delay slots of a thrsw. We should've
      * prevented this when pairing up the ldvary with another instruction
   * and flagging it for a fixup. In V3D 7.x this is limited only to the
   * second delay slot.
      assert((devinfo->ver <= 42 &&
                        /* Move the ldvary to the previous instruction and remove it from the
      * current one.
      prev->qpu.sig.ldvary = true;
   prev->qpu.sig_magic = ldvary_magic;
   prev->qpu.sig_addr = ldvary_index;
            inst->sig.ldvary = false;
   inst->sig_magic = false;
            /* Update rf0 flops tracking for new ldvary delayed rf0 write tick */
   if (devinfo->ver >= 71) {
                        /* By moving ldvary to the previous instruction we make it update r5
      * (rf0 for ver >= 71) in the current one, so nothing else in it
   * should write this register.
   *
   * This should've been prevented by our depedency tracking, which
   * would not allow ldvary to be paired up with an instruction that
   * writes r5/rf0 (since our dependency tracking doesn't know that the
   * ldvary write to r5/rf0 happens in the next instruction).
      assert(!v3d_qpu_writes_r5(devinfo, inst));
   assert(devinfo->ver <= 42 ||
                  }
      static uint32_t
   schedule_instructions(struct v3d_compile *c,
                        struct choose_scoreboard *scoreboard,
      {
         const struct v3d_device_info *devinfo = c->devinfo;
            while (!list_is_empty(&scoreboard->dag->heads)) {
                                 /* If there are no valid instructions to schedule, drop a NOP
   * in.
                        if (debug) {
            fprintf(stderr, "t=%4d: current list:\n",
         dump_state(devinfo, scoreboard->dag);
                     /* We can't mark_instruction_scheduled() the chosen inst until
   * we're done identifying instructions to merge, so put the
   * merged instructions on a list for a moment.
                        /* Schedule this instruction onto the QPU list. Also try to
   * find an instruction to pair with it.
   */
                                 while ((merge =
         choose_instruction_to_schedule(c, scoreboard,
         time = MAX2(merge->unblocked_time, time);
   pre_remove_head(scoreboard->dag, merge);
   list_addtail(&merge->link, &merged_list);
   (void)qpu_merge_inst(devinfo, inst,
                                                   if (debug) {
            fprintf(stderr, "t=%4d: merging: ",
                                       if (scoreboard->fixup_ldvary) {
            scoreboard->fixup_ldvary = false;
   if (fixup_pipelined_ldvary(c, scoreboard, block, inst)) {
            /* Flag the ldvary as scheduled
   * now so we can try to merge the
   * follow-up instruction in the
   * the ldvary sequence into the
   * current instruction.
   */
   mark_instruction_scheduled(
                     /* Update the uniform index for the rewritten location --
   * branch target updating will still need to change
   * c->uniform_data[] using this index.
   */
                                 c->uniform_data[*next_uniform] =
         c->uniform_contents[*next_uniform] =
                                          /* Now that we've scheduled a new instruction, some of its
   * children can be promoted to the list of instructions ready to
   * be scheduled.  Update the children's unblocked time for this
   * DAG edge as we do so.
   */
   mark_instruction_scheduled(devinfo, scoreboard->dag, time, chosen);
                                 /* The merged VIR instruction doesn't get re-added to the
                     if (inst->sig.thrsw) {
         } else if (inst->type == V3D_QPU_INSTR_TYPE_BRANCH) {
         } else {
                     }
      static uint32_t
   qpu_schedule_instructions_block(struct v3d_compile *c,
                                 {
         void *mem_ctx = ralloc_context(NULL);
   scoreboard->dag = dag_create(mem_ctx);
                     /* Wrap each instruction in a scheduler structure. */
   while (!list_is_empty(&block->instructions)) {
                                                            calculate_forward_deps(c, scoreboard->dag, &setup_list);
                     uint32_t cycles = schedule_instructions(c, scoreboard, block,
                        ralloc_free(mem_ctx);
            }
      static void
   qpu_set_branch_targets(struct v3d_compile *c)
   {
         vir_for_each_block(block, c) {
                                 /* If there was no branch instruction, then the successor
   * block must follow immediately after this one.
   */
   if (block->branch_qpu_ip == ~0) {
                              /* Walk back through the delay slots to find the branch
   * instr.
   */
   struct qinst *branch = NULL;
   struct list_head *entry = block->instructions.prev;
   int32_t delay_slot_count = -1;
   struct qinst *delay_slots_start = NULL;
   for (int i = 0; i < 3; i++) {
                                 if (delay_slot_count == -1) {
                                    if (inst->qpu.type == V3D_QPU_INSTR_TYPE_BRANCH) {
            }
                        /* Make sure that the if-we-don't-jump
   * successor was scheduled just after the
   * delay slots.
   */
                        branch->qpu.branch.offset =
                        /* Set up the relative offset to jump in the
   * uniform stream.
   *
   * Use a temporary here, because
   * uniform_data[inst->uniform] may be shared
   * between multiple instructions.
   */
   assert(c->uniform_contents[branch->uniform] == QUNIFORM_CONSTANT);
                        /* If this is an unconditional branch, try to fill any remaining
   * delay slots with the initial instructions of the successor
   * block.
   *
   * FIXME: we can do the same for conditional branches if we
   * predicate the instructions to match the branch condition.
   */
   if (branch->qpu.branch.cond == V3D_QPU_BRANCH_COND_ALWAYS) {
            struct list_head *successor_insts =
         delay_slot_count = MIN2(delay_slot_count,
         struct qinst *s_inst =
         struct qinst *slot = delay_slots_start;
   int slots_filled = 0;
   while (slots_filled < delay_slot_count &&
         qpu_inst_valid_in_branch_delay_slot(c, s_inst)) {
   memcpy(&slot->qpu, &s_inst->qpu,
         s_inst = (struct qinst *) s_inst->link.next;
   slot = (struct qinst *) slot->link.next;
   }
   }
      uint32_t
   v3d_qpu_schedule_instructions(struct v3d_compile *c)
   {
         const struct v3d_device_info *devinfo = c->devinfo;
   struct qblock *end_block = list_last_entry(&c->blocks,
            /* We reorder the uniforms as we schedule instructions, so save the
      * old data off and replace it.
      uint32_t *uniform_data = c->uniform_data;
   enum quniform_contents *uniform_contents = c->uniform_contents;
   c->uniform_contents = ralloc_array(c, enum quniform_contents,
         c->uniform_data = ralloc_array(c, uint32_t, c->num_uniforms);
   c->uniform_array_size = c->num_uniforms;
            struct choose_scoreboard scoreboard;
   memset(&scoreboard, 0, sizeof(scoreboard));
   scoreboard.last_ldvary_tick = -10;
   scoreboard.last_unifa_write_tick = -10;
   scoreboard.last_magic_sfu_write_tick = -10;
   scoreboard.last_uniforms_reset_tick = -10;
   scoreboard.last_thrsw_tick = -10;
   scoreboard.last_branch_tick = -10;
   scoreboard.last_setmsf_tick = -10;
   scoreboard.last_stallable_sfu_tick = -10;
   scoreboard.first_ldtmu_after_thrsw = true;
            if (debug) {
            fprintf(stderr, "Pre-schedule instructions\n");
   vir_for_each_block(block, c) {
            fprintf(stderr, "BLOCK %d\n", block->index);
   list_for_each_entry(struct qinst, qinst,
                              uint32_t cycles = 0;
   vir_for_each_block(block, c) {
                                 cycles += qpu_schedule_instructions_block(c,
                                          /* Emit the program-end THRSW instruction. */;
   struct qinst *thrsw = vir_nop();
   thrsw->qpu.sig.thrsw = true;
                              }
