   /*
   * Copyright © 2016 Broadcom
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
      #include "compiler/v3d_compiler.h"
   #include "qpu/qpu_instr.h"
   #include "qpu/qpu_disasm.h"
      static inline struct qpu_reg
   qpu_reg(int index)
   {
         struct qpu_reg reg = {
               };
   }
      static inline struct qpu_reg
   qpu_magic(enum v3d_qpu_waddr waddr)
   {
         struct qpu_reg reg = {
               };
   }
      struct v3d_qpu_instr
   v3d_qpu_nop(void)
   {
         struct v3d_qpu_instr instr = {
            .type = V3D_QPU_INSTR_TYPE_ALU,
   .alu = {
            .add = {
         .op = V3D_QPU_A_NOP,
   .waddr = V3D_QPU_WADDR_NOP,
   },
   .mul = {
         .op = V3D_QPU_M_NOP,
            }
      static struct qinst *
   vir_nop(void)
   {
         struct qreg undef = vir_nop_reg();
            }
      static struct qinst *
   new_qpu_nop_before(struct qinst *inst)
   {
                           }
      static void
   v3d71_set_src(struct v3d_qpu_instr *instr, uint8_t *raddr, struct qpu_reg src)
   {
         /* If we have a small immediate move it from inst->raddr_b to the
      * corresponding raddr.
      if (src.smimm) {
            assert(instr->sig.small_imm_a || instr->sig.small_imm_b ||
                     assert(!src.magic);
   }
      /**
   * Allocates the src register (accumulator or register file) into the RADDR
   * fields of the instruction.
   */
   static void
   v3d33_set_src(struct v3d_qpu_instr *instr, enum v3d_qpu_mux *mux, struct qpu_reg src)
   {
         if (src.smimm) {
            assert(instr->sig.small_imm_b);
               if (src.magic) {
            assert(src.index >= V3D_QPU_WADDR_R0 &&
                     if (instr->alu.add.a.mux != V3D_QPU_MUX_A &&
         instr->alu.add.b.mux != V3D_QPU_MUX_A &&
   instr->alu.mul.a.mux != V3D_QPU_MUX_A &&
   instr->alu.mul.b.mux != V3D_QPU_MUX_A) {
         } else {
            if (instr->raddr_a == src.index) {
         } else {
            assert(!(instr->alu.add.a.mux == V3D_QPU_MUX_B &&
                              }
      /*
   * The main purpose of the following wrapper is to make calling set_src
   * cleaner. This is the reason it receives both mux and raddr pointers. Those
   * will be filled or not based on the device version.
   */
   static void
   set_src(struct v3d_qpu_instr *instr,
         enum v3d_qpu_mux *mux,
   uint8_t *raddr,
   struct qpu_reg src,
   {
         if (devinfo->ver < 71)
         else
   }
      static bool
   v3d33_mov_src_and_dst_equal(struct qinst *qinst)
   {
         enum v3d_qpu_waddr waddr = qinst->qpu.alu.mul.waddr;
   if (qinst->qpu.alu.mul.magic_write) {
                           if (qinst->qpu.alu.mul.a.mux !=
      V3D_QPU_MUX_R0 + (waddr - V3D_QPU_WADDR_R0)) {
   } else {
                     switch (qinst->qpu.alu.mul.a.mux) {
   case V3D_QPU_MUX_A:
               case V3D_QPU_MUX_B:
               default:
         }
               }
      static bool
   v3d71_mov_src_and_dst_equal(struct qinst *qinst)
   {
         if (qinst->qpu.alu.mul.magic_write)
            enum v3d_qpu_waddr waddr = qinst->qpu.alu.mul.waddr;
            raddr = qinst->qpu.alu.mul.a.raddr;
   if (raddr != waddr)
            }
      static bool
   mov_src_and_dst_equal(struct qinst *qinst,
         {
         if (devinfo->ver < 71)
         else
   }
         static bool
   is_no_op_mov(struct qinst *qinst,
         {
                  /* Make sure it's just a lone MOV. We only check for M_MOV. Although
      * for V3D 7.x there is also A_MOV, we don't need to check for it as
   * we always emit using M_MOV. We could use A_MOV later on the
   * squedule to improve performance
      if (qinst->qpu.type != V3D_QPU_INSTR_TYPE_ALU ||
         qinst->qpu.alu.mul.op != V3D_QPU_M_MOV ||
   qinst->qpu.alu.add.op != V3D_QPU_A_NOP ||
   memcmp(&qinst->qpu.sig, &no_sig, sizeof(no_sig)) != 0) {
            if (!mov_src_and_dst_equal(qinst, devinfo))
            /* No packing or flags updates, or we need to execute the
      * instruction.
      if (qinst->qpu.alu.mul.a.unpack != V3D_QPU_UNPACK_NONE ||
         qinst->qpu.alu.mul.output_pack != V3D_QPU_PACK_NONE ||
   qinst->qpu.flags.mc != V3D_QPU_COND_NONE ||
   qinst->qpu.flags.mpf != V3D_QPU_PF_NONE ||
   qinst->qpu.flags.muf != V3D_QPU_UF_NONE) {
            }
      static void
   v3d_generate_code_block(struct v3d_compile *c,
               {
                  #if 0
                     #endif
                                             int nsrc = vir_get_nsrc(qinst);
   struct qpu_reg src[ARRAY_SIZE(qinst->src)];
   for (int i = 0; i < nsrc; i++) {
            int index = qinst->src[i].index;
   switch (qinst->src[i].file) {
   case QFILE_REG:
         src[i] = qpu_reg(qinst->src[i].index);
   case QFILE_MAGIC:
         src[i] = qpu_magic(qinst->src[i].index);
   case QFILE_NULL:
         /* QFILE_NULL is an undef, so we can load
      * anything. Using a reg that doesn't have
   * sched. restrictions.
      src[i] = qpu_reg(5);
   case QFILE_LOAD_IMM:
         assert(!"not reached");
   case QFILE_TEMP:
                                    case QFILE_VPM:
                                                                     struct qpu_reg dst;
   switch (qinst->dst.file) {
                                                                                                            case QFILE_SMALL_IMM:
   case QFILE_LOAD_IMM:
                        if (qinst->qpu.type == V3D_QPU_INSTR_TYPE_ALU) {
                                       bool use_rf;
   if (c->devinfo->has_accumulators) {
                                                            if (qinst->qpu.sig.ldunif) {
      qinst->qpu.sig.ldunif = false;
      } else {
      qinst->qpu.sig.ldunifa = false;
      }
                                                                  if (nsrc >= 1) {
            set_src(&qinst->qpu,
            }
   if (nsrc >= 2) {
                                    qinst->qpu.alu.add.waddr = dst.index;
   } else {
         if (nsrc >= 1) {
            set_src(&qinst->qpu,
            }
   if (nsrc >= 2) {
                                                   if (is_no_op_mov(qinst, c->devinfo)) {
            } else {
      }
      static bool
   reads_uniform(const struct v3d_device_info *devinfo, uint64_t instruction)
   {
         struct v3d_qpu_instr qpu;
   ASSERTED bool ok = v3d_qpu_instr_unpack(devinfo, instruction, &qpu);
            if (qpu.sig.ldunif ||
         qpu.sig.ldunifrf ||
   qpu.sig.ldtlbu ||
   qpu.sig.wrtmuc) {
            if (qpu.type == V3D_QPU_INSTR_TYPE_BRANCH)
            if (qpu.type == V3D_QPU_INSTR_TYPE_ALU) {
            if (qpu.alu.add.magic_write &&
                        if (qpu.alu.mul.magic_write &&
      v3d_qpu_magic_waddr_loads_unif(qpu.alu.mul.waddr)) {
            }
      static void
   v3d_dump_qpu(struct v3d_compile *c)
   {
         fprintf(stderr, "%s prog %d/%d QPU:\n",
                  int next_uniform = 0;
   for (int i = 0; i < c->qpu_inst_count; i++) {
                           /* We can only do this on 4.x, because we're not tracking TMU
   * implicit uniforms here on 3.x.
   */
   if (c->devinfo->ver >= 40 &&
      reads_uniform(c->devinfo, c->qpu_insts[i])) {
         fprintf(stderr, " (");
   vir_dump_uniform(c->uniform_contents[next_uniform],
            }
               /* Make sure our dumping lined up. */
   if (c->devinfo->ver >= 40)
            }
      void
   v3d_vir_to_qpu(struct v3d_compile *c, struct qpu_reg *temp_registers)
   {
         /* Reset the uniform count to how many will be actually loaded by the
      * generated QPU code.
               vir_for_each_block(block, c)
                     c->qpu_insts = rzalloc_array(c, uint64_t, c->qpu_inst_count);
   int i = 0;
   vir_for_each_inst_inorder(inst, c) {
            bool ok = v3d_qpu_instr_pack(c->devinfo, &inst->qpu,
         if (!ok) {
            fprintf(stderr, "Failed to pack instruction %d:\n", i);
   vir_dump_inst(c, inst);
                        }
            if (V3D_DBG(QPU) ||
         v3d_debug_flag_for_shader_stage(c->s->info.stage)) {
                     }
