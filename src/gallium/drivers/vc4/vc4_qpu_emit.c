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
      #include <inttypes.h>
      #include "vc4_context.h"
   #include "vc4_qir.h"
   #include "vc4_qpu.h"
   #include "util/ralloc.h"
   #include "util/u_debug_cb.h"
      static void
   vc4_dump_program(struct vc4_compile *c)
   {
         fprintf(stderr, "%s prog %d/%d QPU:\n",
                  for (int i = 0; i < c->qpu_inst_count; i++) {
            fprintf(stderr, "0x%016"PRIx64" ", c->qpu_insts[i]);
      }
   }
      static void
   queue(struct qblock *block, uint64_t inst)
   {
         struct queued_qpu_inst *q = rzalloc(block, struct queued_qpu_inst);
   q->inst = inst;
   }
      static uint64_t *
   last_inst(struct qblock *block)
   {
         struct queued_qpu_inst *q =
         }
      static void
   set_last_cond_add(struct qblock *block, uint32_t cond)
   {
         }
      static void
   set_last_cond_mul(struct qblock *block, uint32_t cond)
   {
         }
      /**
   * Some special registers can be read from either file, which lets us resolve
   * raddr conflicts without extra MOVs.
   */
   static bool
   swap_file(struct qpu_reg *src)
   {
         switch (src->addr) {
   case QPU_R_UNIF:
   case QPU_R_VARY:
            if (src->mux == QPU_MUX_SMALL_IMM) {
         } else {
            if (src->mux == QPU_MUX_A)
                  default:
         }
      /**
   * Sets up the VPM read FIFO before we do any VPM read.
   *
   * VPM reads (vertex attribute input) and VPM writes (varyings output) from
   * the QPU reuse the VRI (varying interpolation) block's FIFOs to talk to the
   * VPM block.  In the VS/CS (unlike in the FS), the block starts out
   * uninitialized, and you need to emit setup to the block before any VPM
   * reads/writes.
   *
   * VRI has a FIFO in each direction, with each FIFO able to hold four
   * 32-bit-per-vertex values.  VPM reads come through the read FIFO and VPM
   * writes go through the write FIFO.  The read/write setup values from QPU go
   * through the write FIFO as well, with a sideband signal indicating that
   * they're setup values.  Once a read setup reaches the other side of the
   * FIFO, the VPM block will start asynchronously reading vertex attributes and
   * filling the read FIFO -- that way hopefully the QPU doesn't have to block
   * on reads later.
   *
   * VPM read setup can configure 16 32-bit-per-vertex values to be read at a
   * time, which is 4 vec4s.  If more than that is being read (since we support
   * 8 vec4 vertex attributes), then multiple read setup writes need to be done.
   *
   * The existence of the FIFO makes it seem like you should be able to emit
   * both setups for the 5-8 attribute cases and then do all the attribute
   * reads.  However, once the setup value makes it to the other end of the
   * write FIFO, it will immediately update the VPM block's setup register.
   * That updated setup register would be used for read FIFO fills from then on,
   * breaking whatever remaining VPM values were supposed to be read into the
   * read FIFO from the previous attribute set.
   *
   * As a result, we need to emit the read setup, pull every VPM read value from
   * that setup, and only then emit the second setup if applicable.
   */
   static void
   setup_for_vpm_read(struct vc4_compile *c, struct qblock *block)
   {
         if (c->num_inputs_in_fifo) {
                                 queue(block,
         qpu_load_imm_ui(qpu_vrsetup(),
               c->num_inputs_remaining -= c->num_inputs_in_fifo;
            }
      /**
   * This is used to resolve the fact that we might register-allocate two
   * different operands of an instruction to the same physical register file
   * even though instructions have only one field for the register file source
   * address.
   *
   * In that case, we need to move one to a temporary that can be used in the
   * instruction, instead.  We reserve ra14/rb14 for this purpose.
   */
   static void
   fixup_raddr_conflict(struct qblock *block,
                     {
         uint32_t mux0 = src0->mux == QPU_MUX_SMALL_IMM ? QPU_MUX_B : src0->mux;
            if (mux0 <= QPU_MUX_R5 ||
         mux0 != mux1 ||
   (src0->addr == src1->addr &&
   src0->mux == src1->mux)) {
            if (swap_file(src0) || swap_file(src1))
            if (mux0 == QPU_MUX_A) {
            /* Make sure we use the same type of MOV as the instruction,
   * in case of unpacks.
   */
   if (qir_is_float_input(inst))
                        /* If we had an unpack on this A-file source, we need to put
   * it into this MOV, not into the later move from regfile B.
   */
   if (inst->src[0].pack) {
                  } else {
               }
      static void
   set_last_dst_pack(struct qblock *block, struct qinst *inst)
   {
         ASSERTED bool had_pm = *last_inst(block) & QPU_PM;
   ASSERTED bool had_ws = *last_inst(block) & QPU_WS;
            if (!inst->dst.pack)
                     if (qir_is_mul(inst)) {
               } else {
               }
      static void
   handle_r4_qpu_write(struct qblock *block, struct qinst *qinst,
         {
         if (dst.mux != QPU_MUX_R4) {
               } else {
            assert(qinst->cond == QPU_COND_ALWAYS);
      }
      static void
   vc4_generate_code_block(struct vc4_compile *c,
               {
                  #if 0
                     #endif
                        #define A(name) [QOP_##name] = {QPU_A_##name}
   #define M(name) [QOP_##name] = {QPU_M_##name}
                           A(FADD),
   A(FSUB),
   A(FMIN),
   A(FMAX),
   A(FMINABS),
   A(FMAXABS),
   A(FTOI),
   A(ITOF),
   A(ADD),
   A(SUB),
   A(SHL),
   A(SHR),
   A(ASR),
   A(MIN),
   A(MAX),
                              M(FMUL),
   M(V8MULD),
   M(V8MIN),
                              /* If we replicate src[0] out to src[1], this works
   * out the same as a MOV.
                                    uint64_t unpack = 0;
   struct qpu_reg src[ARRAY_SIZE(qinst->src)];
   for (int i = 0; i < qir_get_nsrc(qinst); i++) {
            int index = qinst->src[i].index;
   switch (qinst->src[i].file) {
   case QFILE_NULL:
   case QFILE_LOAD_IMM:
         src[i] = qpu_rn(0);
   case QFILE_TEMP:
         src[i] = temp_registers[index];
   if (qinst->src[i].pack) {
            assert(!unpack ||
         unpack = QPU_SET_FIELD(qinst->src[i].pack,
            }
   case QFILE_UNIF:
         src[i] = qpu_unif();
   case QFILE_VARY:
         src[i] = qpu_vary();
   case QFILE_SMALL_IMM:
         src[i].mux = QPU_MUX_SMALL_IMM;
   src[i].addr = qpu_encode_small_immediate(qinst->src[i].index);
   /* This should only have returned a valid
      * small immediate field, not ~0 for failure.
      assert(src[i].addr <= 47);
   case QFILE_VPM:
         setup_for_vpm_read(c, block);
   assert((int)qinst->src[i].index >=
                              case QFILE_FRAG_X:
         src[i] = qpu_ra(QPU_R_XY_PIXEL_COORD);
   case QFILE_FRAG_Y:
         src[i] = qpu_rb(QPU_R_XY_PIXEL_COORD);
   case QFILE_FRAG_REV_FLAG:
                                    case QFILE_TLB_COLOR_WRITE:
   case QFILE_TLB_COLOR_WRITE_MS:
   case QFILE_TLB_Z_WRITE:
   case QFILE_TLB_STENCIL_SETUP:
   case QFILE_TEX_S:
   case QFILE_TEX_S_DIRECT:
   case QFILE_TEX_T:
   case QFILE_TEX_R:
                     struct qpu_reg dst;
   switch (qinst->dst.file) {
   case QFILE_NULL:
               case QFILE_TEMP:
                                                                                                                        case QFILE_TEX_S:
                                                                                       case QFILE_VARY:
   case QFILE_UNIF:
   case QFILE_SMALL_IMM:
   case QFILE_LOAD_IMM:
   case QFILE_FRAG_X:
   case QFILE_FRAG_Y:
   case QFILE_FRAG_REV_FLAG:
   case QFILE_QPU_ELEMENT:
                                 switch (qinst->op) {
   case QOP_RCP:
   case QOP_RSQ:
   case QOP_EXP2:
   case QOP_LOG2:
            switch (qinst->op) {
   case QOP_RCP:
         queue(block, qpu_a_MOV(qpu_rb(QPU_W_SFU_RECIP),
         case QOP_RSQ:
         queue(block, qpu_a_MOV(qpu_rb(QPU_W_SFU_RECIPSQRT),
         case QOP_EXP2:
         queue(block, qpu_a_MOV(qpu_rb(QPU_W_SFU_EXP),
         case QOP_LOG2:
         queue(block, qpu_a_MOV(qpu_rb(QPU_W_SFU_LOG),
                                             case QOP_LOAD_IMM:
                                                                  case QOP_ROT_MUL:
            /* Rotation at the hardware level occurs on the inputs
                              queue(block,
         qpu_m_rot(dst, src[0], qinst->src[1].index -
                     case QOP_MS_MASK:
            src[1] = qpu_ra(QPU_R_MS_REV_FLAGS);
   fixup_raddr_conflict(block, dst, &src[0], &src[1],
                     case QOP_FRAG_Z:
   case QOP_FRAG_W:
                              case QOP_TLB_COLOR_READ:
            queue(block, qpu_NOP());
   *last_inst(block) = qpu_set_sig(*last_inst(block),
                                             case QOP_TEX_RESULT:
            queue(block, qpu_NOP());
   *last_inst(block) = qpu_set_sig(*last_inst(block),
                     case QOP_THRSW:
            queue(block, qpu_NOP());
                     case QOP_BRANCH:
            /* The branch target will be updated at QPU scheduling
   * time.
   */
                                                                                             /* Skip emitting the MOV if it's a no-op. */
                              /* If we have only one source, put it in the second
   * argument slot as well so that we don't take up
                                             if (qir_is_mul(qinst)) {
         queue(block, qpu_m_alu2(translate[qinst->op].op,
               } else {
         queue(block, qpu_a_alu2(translate[qinst->op].op,
                                                            }
      void
   vc4_generate_code(struct vc4_context *vc4, struct vc4_compile *c)
   {
         struct qblock *start_block = list_first_entry(&c->blocks,
            struct qpu_reg *temp_registers = vc4_register_allocate(vc4, c);
   if (!temp_registers)
            switch (c->stage) {
   case QSTAGE_VERT:
   case QSTAGE_COORD:
            c->num_inputs_remaining = c->num_inputs;
      case QSTAGE_FRAG:
                  qir_for_each_block(block, c)
            /* Switch the last SIG_THRSW instruction to SIG_LAST_THRSW.
      *
   * LAST_THRSW is a new signal in BCM2708B0 (including Raspberry Pi)
   * that ensures that a later thread doesn't try to lock the scoreboard
   * and terminate before an earlier-spawned thread on the same QPU, by
   * delaying switching back to the later shader until earlier has
   * finished.  Otherwise, if the earlier thread was hitting the same
   * quad, the scoreboard would deadlock.
      if (c->last_thrsw) {
            assert(QPU_GET_FIELD(*c->last_thrsw, QPU_SIG) ==
         *c->last_thrsw = ((*c->last_thrsw & ~QPU_SIG_MASK) |
               uint32_t cycles = qpu_schedule_instructions(c);
            /* thread end can't have VPM write or read */
   if (QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
               QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
         QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
         QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
                  /* thread end can't have uniform read */
   if (QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
               QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
                  /* thread end can't have TLB operations */
   if (qpu_inst_is_tlb(c->qpu_insts[c->qpu_inst_count - 1]))
            /* Make sure there's no existing signal set (like for a small
      * immediate)
      if (QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
                        c->qpu_insts[c->qpu_inst_count - 1] =
               qpu_serialize_one_inst(c, qpu_NOP());
            switch (c->stage) {
   case QSTAGE_VERT:
   case QSTAGE_COORD:
         case QSTAGE_FRAG:
            c->qpu_insts[c->qpu_inst_count - 1] =
                              if (VC4_DBG(SHADERDB)) {
            util_debug_message(&vc4->base.debug, SHADER_INFO,
                     "%s shader: %d inst, %d threads, %d uniforms, %d max-temps, %d estimated-cycles",
   qir_get_stage_name(c->stage),
               if (VC4_DBG(QPU))
                     }
