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
      #include <stdbool.h>
   #include "util/ralloc.h"
   #include "vc4_qir.h"
   #include "vc4_qpu.h"
      #define QPU_MUX(mux, muxfield)                                  \
            static uint64_t
   set_src_raddr(uint64_t inst, struct qpu_reg src)
   {
         if (src.mux == QPU_MUX_A) {
            assert(QPU_GET_FIELD(inst, QPU_RADDR_A) == QPU_R_NOP ||
               if (src.mux == QPU_MUX_B) {
            assert((QPU_GET_FIELD(inst, QPU_RADDR_B) == QPU_R_NOP ||
                     if (src.mux == QPU_MUX_SMALL_IMM) {
            if (QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_SMALL_IMM) {
         } else {
               }
               }
      uint64_t
   qpu_NOP()
   {
                  inst |= QPU_SET_FIELD(QPU_A_NOP, QPU_OP_ADD);
            /* Note: These field values are actually non-zero */
   inst |= QPU_SET_FIELD(QPU_W_NOP, QPU_WADDR_ADD);
   inst |= QPU_SET_FIELD(QPU_W_NOP, QPU_WADDR_MUL);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_B);
            }
      static uint64_t
   qpu_a_dst(struct qpu_reg dst)
   {
                  if (dst.mux <= QPU_MUX_R5) {
               } else {
            inst |= QPU_SET_FIELD(dst.addr, QPU_WADDR_ADD);
               }
      static uint64_t
   qpu_m_dst(struct qpu_reg dst)
   {
                  if (dst.mux <= QPU_MUX_R5) {
               } else {
            inst |= QPU_SET_FIELD(dst.addr, QPU_WADDR_MUL);
               }
      uint64_t
   qpu_a_MOV(struct qpu_reg dst, struct qpu_reg src)
   {
                  inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);
   inst |= QPU_SET_FIELD(QPU_A_OR, QPU_OP_ADD);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_B);
   inst |= qpu_a_dst(dst);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
   inst |= QPU_MUX(src.mux, QPU_ADD_A);
   inst |= QPU_MUX(src.mux, QPU_ADD_B);
   inst = set_src_raddr(inst, src);
            }
      uint64_t
   qpu_m_MOV(struct qpu_reg dst, struct qpu_reg src)
   {
                  inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);
   inst |= QPU_SET_FIELD(QPU_M_V8MIN, QPU_OP_MUL);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_B);
   inst |= qpu_m_dst(dst);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
   inst |= QPU_MUX(src.mux, QPU_MUL_A);
   inst |= QPU_MUX(src.mux, QPU_MUL_B);
   inst = set_src_raddr(inst, src);
            }
      uint64_t
   qpu_load_imm_ui(struct qpu_reg dst, uint32_t val)
   {
                  inst |= qpu_a_dst(dst);
   inst |= QPU_SET_FIELD(QPU_W_NOP, QPU_WADDR_MUL);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
   inst |= QPU_SET_FIELD(QPU_SIG_LOAD_IMM, QPU_SIG);
            }
      uint64_t
   qpu_load_imm_u2(struct qpu_reg dst, uint32_t val)
   {
         return qpu_load_imm_ui(dst, val) | QPU_SET_FIELD(QPU_LOAD_IMM_MODE_U2,
   }
      uint64_t
   qpu_load_imm_i2(struct qpu_reg dst, uint32_t val)
   {
         return qpu_load_imm_ui(dst, val) | QPU_SET_FIELD(QPU_LOAD_IMM_MODE_I2,
   }
      uint64_t
   qpu_branch(uint32_t cond, uint32_t target)
   {
                  inst |= qpu_a_dst(qpu_ra(QPU_W_NOP));
   inst |= qpu_m_dst(qpu_rb(QPU_W_NOP));
   inst |= QPU_SET_FIELD(cond, QPU_BRANCH_COND);
   inst |= QPU_SET_FIELD(QPU_SIG_BRANCH, QPU_SIG);
            }
      uint64_t
   qpu_a_alu2(enum qpu_op_add op,
         {
                  inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);
   inst |= QPU_SET_FIELD(op, QPU_OP_ADD);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_B);
   inst |= qpu_a_dst(dst);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
   inst |= QPU_MUX(src0.mux, QPU_ADD_A);
   inst = set_src_raddr(inst, src0);
   inst |= QPU_MUX(src1.mux, QPU_ADD_B);
   inst = set_src_raddr(inst, src1);
            }
      uint64_t
   qpu_m_alu2(enum qpu_op_mul op,
         {
                  inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);
   inst |= QPU_SET_FIELD(op, QPU_OP_MUL);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   inst |= QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_B);
   inst |= qpu_m_dst(dst);
   inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
   inst |= QPU_MUX(src0.mux, QPU_MUL_A);
   inst = set_src_raddr(inst, src0);
   inst |= QPU_MUX(src1.mux, QPU_MUL_B);
   inst = set_src_raddr(inst, src1);
            }
      uint64_t
   qpu_m_rot(struct qpu_reg dst, struct qpu_reg src0, int rot)
   {
   uint64_t inst = 0;
   inst = qpu_m_alu2(QPU_M_V8MIN, dst, src0, src0);
      inst = QPU_UPDATE_FIELD(inst, QPU_SIG_SMALL_IMM, QPU_SIG);
   inst = QPU_UPDATE_FIELD(inst, QPU_SMALL_IMM_MUL_ROT + rot,
            return inst;
   }
      static bool
   merge_fields(uint64_t *merge,
               {
         if ((a & mask) == ignore) {
         } else if ((b & mask) == ignore) {
         } else {
                        }
      int
   qpu_num_sf_accesses(uint64_t inst)
   {
         int accesses = 0;
   static const uint32_t specials[] = {
            QPU_W_TLB_COLOR_MS,
   QPU_W_TLB_COLOR_ALL,
   QPU_W_TLB_Z,
   QPU_W_TMU0_S,
   QPU_W_TMU0_T,
   QPU_W_TMU0_R,
   QPU_W_TMU0_B,
   QPU_W_TMU1_S,
   QPU_W_TMU1_T,
   QPU_W_TMU1_R,
   QPU_W_TMU1_B,
   QPU_W_SFU_RECIP,
   QPU_W_SFU_RECIPSQRT,
      };
   uint32_t waddr_add = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
   uint32_t waddr_mul = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
   uint32_t raddr_a = QPU_GET_FIELD(inst, QPU_RADDR_A);
            for (int j = 0; j < ARRAY_SIZE(specials); j++) {
            if (waddr_add == specials[j])
                     if (raddr_a == QPU_R_MUTEX_ACQUIRE)
         if (raddr_b == QPU_R_MUTEX_ACQUIRE &&
                  /* XXX: semaphore, combined color read/write? */
   switch (QPU_GET_FIELD(inst, QPU_SIG)) {
   case QPU_SIG_COLOR_LOAD:
   case QPU_SIG_COLOR_LOAD_END:
   case QPU_SIG_LOAD_TMU0:
   case QPU_SIG_LOAD_TMU1:
                  }
      static bool
   qpu_waddr_ignores_ws(uint32_t waddr)
   {
         switch(waddr) {
   case QPU_W_ACC0:
   case QPU_W_ACC1:
   case QPU_W_ACC2:
   case QPU_W_ACC3:
   case QPU_W_NOP:
   case QPU_W_TLB_Z:
   case QPU_W_TLB_COLOR_MS:
   case QPU_W_TLB_COLOR_ALL:
   case QPU_W_TLB_ALPHA_MASK:
   case QPU_W_VPM:
   case QPU_W_SFU_RECIP:
   case QPU_W_SFU_RECIPSQRT:
   case QPU_W_SFU_EXP:
   case QPU_W_SFU_LOG:
   case QPU_W_TMU0_S:
   case QPU_W_TMU0_T:
   case QPU_W_TMU0_R:
   case QPU_W_TMU0_B:
   case QPU_W_TMU1_S:
   case QPU_W_TMU1_T:
   case QPU_W_TMU1_R:
   case QPU_W_TMU1_B:
                  }
      static void
   swap_ra_file_mux_helper(uint64_t *merge, uint64_t *a, uint32_t mux_shift)
   {
         uint64_t mux_mask = (uint64_t)0x7 << mux_shift;
   uint64_t mux_a_val = (uint64_t)QPU_MUX_A << mux_shift;
            if ((*a & mux_mask) == mux_a_val) {
               }
      static bool
   try_swap_ra_file(uint64_t *merge, uint64_t *a, uint64_t *b)
   {
         uint32_t raddr_a_a = QPU_GET_FIELD(*a, QPU_RADDR_A);
   uint32_t raddr_a_b = QPU_GET_FIELD(*a, QPU_RADDR_B);
   uint32_t raddr_b_a = QPU_GET_FIELD(*b, QPU_RADDR_A);
            if (raddr_a_b != QPU_R_NOP)
            switch (raddr_a_a) {
   case QPU_R_UNIF:
   case QPU_R_VARY:
         default:
                  if (!(*merge & QPU_PM) &&
         QPU_GET_FIELD(*merge, QPU_UNPACK) != QPU_UNPACK_NOP) {
            if (raddr_b_b != QPU_R_NOP &&
                  /* Move raddr A to B in instruction a. */
   *a = (*a & ~QPU_RADDR_A_MASK) | QPU_SET_FIELD(QPU_R_NOP, QPU_RADDR_A);
   *a = (*a & ~QPU_RADDR_B_MASK) | QPU_SET_FIELD(raddr_a_a, QPU_RADDR_B);
   *merge = QPU_UPDATE_FIELD(*merge, raddr_b_a, QPU_RADDR_A);
   *merge = QPU_UPDATE_FIELD(*merge, raddr_a_a, QPU_RADDR_B);
   swap_ra_file_mux_helper(merge, a, QPU_ADD_A_SHIFT);
   swap_ra_file_mux_helper(merge, a, QPU_ADD_B_SHIFT);
   swap_ra_file_mux_helper(merge, a, QPU_MUL_A_SHIFT);
            }
      static bool
   convert_mov(uint64_t *inst)
   {
         uint32_t add_a = QPU_GET_FIELD(*inst, QPU_ADD_A);
   uint32_t waddr_add = QPU_GET_FIELD(*inst, QPU_WADDR_ADD);
            /* Is it a MOV? */
   if (QPU_GET_FIELD(*inst, QPU_OP_ADD) != QPU_A_OR ||
         (add_a != QPU_GET_FIELD(*inst, QPU_ADD_B))) {
            if (QPU_GET_FIELD(*inst, QPU_SIG) != QPU_SIG_NONE)
            /* We could maybe support this in the .8888 and .8a-.8d cases. */
   if (*inst & QPU_PM)
            *inst = QPU_UPDATE_FIELD(*inst, QPU_A_NOP, QPU_OP_ADD);
            *inst = QPU_UPDATE_FIELD(*inst, add_a, QPU_MUL_A);
   *inst = QPU_UPDATE_FIELD(*inst, add_a, QPU_MUL_B);
   *inst = QPU_UPDATE_FIELD(*inst, QPU_MUX_R0, QPU_ADD_A);
            *inst = QPU_UPDATE_FIELD(*inst, waddr_add, QPU_WADDR_MUL);
            *inst = QPU_UPDATE_FIELD(*inst, cond_add, QPU_COND_MUL);
            if (!qpu_waddr_ignores_ws(waddr_add))
            }
      static bool
   writes_a_file(uint64_t inst)
   {
         if (!(inst & QPU_WS))
         else
   }
      static bool
   reads_r4(uint64_t inst)
   {
         return (QPU_GET_FIELD(inst, QPU_ADD_A) == QPU_MUX_R4 ||
               }
      uint64_t
   qpu_merge_inst(uint64_t a, uint64_t b)
   {
         uint64_t merge = a | b;
   bool ok = true;
   uint32_t a_sig = QPU_GET_FIELD(a, QPU_SIG);
            if (QPU_GET_FIELD(a, QPU_OP_ADD) != QPU_A_NOP &&
         QPU_GET_FIELD(b, QPU_OP_ADD) != QPU_A_NOP) {
      if (QPU_GET_FIELD(a, QPU_OP_MUL) != QPU_M_NOP ||
      QPU_GET_FIELD(b, QPU_OP_MUL) != QPU_M_NOP ||
   !(convert_mov(&a) || convert_mov(&b))) {
      } else {
               if (QPU_GET_FIELD(a, QPU_OP_MUL) != QPU_M_NOP &&
                  if (qpu_num_sf_accesses(a) && qpu_num_sf_accesses(b))
            if (a_sig == QPU_SIG_LOAD_IMM ||
         b_sig == QPU_SIG_LOAD_IMM ||
   a_sig == QPU_SIG_SMALL_IMM ||
   b_sig == QPU_SIG_SMALL_IMM ||
   a_sig == QPU_SIG_BRANCH ||
   b_sig == QPU_SIG_BRANCH) {
            ok = ok && merge_fields(&merge, a, b, QPU_SIG_MASK,
            /* Misc fields that have to match exactly. */
            if (!merge_fields(&merge, a, b, QPU_RADDR_A_MASK,
                  /* Since we tend to use regfile A by default both for register
   * allocation and for our special values (uniforms and
   * varyings), try swapping uniforms and varyings to regfile B
   * to resolve raddr A conflicts.
   */
   if (!try_swap_ra_file(&merge, &a, &b) &&
      !try_swap_ra_file(&merge, &b, &a)) {
            ok = ok && merge_fields(&merge, a, b, QPU_RADDR_B_MASK,
            ok = ok && merge_fields(&merge, a, b, QPU_WADDR_ADD_MASK,
         ok = ok && merge_fields(&merge, a, b, QPU_WADDR_MUL_MASK,
            /* Allow disagreement on WS (swapping A vs B physical reg file as the
      * destination for ADD/MUL) if one of the original instructions
   * ignores it (probably because it's just writing to accumulators).
      if (qpu_waddr_ignores_ws(QPU_GET_FIELD(a, QPU_WADDR_ADD)) &&
         qpu_waddr_ignores_ws(QPU_GET_FIELD(a, QPU_WADDR_MUL))) {
   } else if (qpu_waddr_ignores_ws(QPU_GET_FIELD(b, QPU_WADDR_ADD)) &&
               } else {
                        if (!merge_fields(&merge, a, b, QPU_PM, ~0)) {
            /* If one instruction has PM bit set and the other not, the
   * one without PM shouldn't do packing/unpacking, and we
   * have to make sure non-NOP packing/unpacking from PM
                        /* Let a be the one with PM bit */
   if (!(a & QPU_PM)) {
                                                                     } else {
            /* packing: Make sure that non-NOP packs agree, then deal with
   * special-case failing of adding a non-NOP pack to something
   * with a NOP pack.
   */
   if (!merge_fields(&merge, a, b, QPU_PACK_MASK, 0))
         bool new_a_pack = (QPU_GET_FIELD(a, QPU_PACK) !=
         bool new_b_pack = (QPU_GET_FIELD(b, QPU_PACK) !=
         if (!(merge & QPU_PM)) {
            /* Make sure we're not going to be putting a new
                                 } else {
            /* Make sure we're not going to be putting new MUL
   * packing on either half.
                                                /* unpacking: Make sure that non-NOP unpacks agree, then deal
   * with special-case failing of adding a non-NOP unpack to
   * something with a NOP unpack.
   */
   if (!merge_fields(&merge, a, b, QPU_UNPACK_MASK, 0))
         bool new_a_unpack = (QPU_GET_FIELD(a, QPU_UNPACK) !=
         bool new_b_unpack = (QPU_GET_FIELD(b, QPU_UNPACK) !=
         if (!(merge & QPU_PM)) {
            /* Make sure we're not going to be putting a new
   * a-file packing on either half.
                              if (new_b_unpack &&
      } else {
            /* Make sure we're not going to be putting new r4
                                       if (ok)
         else
   }
      uint64_t
   qpu_set_sig(uint64_t inst, uint32_t sig)
   {
         assert(QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_NONE);
   }
      uint64_t
   qpu_set_cond_add(uint64_t inst, uint32_t cond)
   {
         assert(QPU_GET_FIELD(inst, QPU_COND_ADD) == QPU_COND_ALWAYS);
   }
      uint64_t
   qpu_set_cond_mul(uint64_t inst, uint32_t cond)
   {
         assert(QPU_GET_FIELD(inst, QPU_COND_MUL) == QPU_COND_ALWAYS);
   }
      bool
   qpu_waddr_is_tlb(uint32_t waddr)
   {
         switch (waddr) {
   case QPU_W_TLB_COLOR_ALL:
   case QPU_W_TLB_COLOR_MS:
   case QPU_W_TLB_Z:
         default:
         }
      bool
   qpu_inst_is_tlb(uint64_t inst)
   {
                  return (qpu_waddr_is_tlb(QPU_GET_FIELD(inst, QPU_WADDR_ADD)) ||
               }
      /**
   * Returns the small immediate value to be encoded in to the raddr b field if
   * the argument can be represented as one, or ~0 otherwise.
   */
   uint32_t
   qpu_encode_small_immediate(uint32_t i)
   {
         if (i <= 15)
         if ((int)i < 0 && (int)i >= -16)
            switch (i) {
   case 0x3f800000:
         case 0x40000000:
         case 0x40800000:
         case 0x41000000:
         case 0x41800000:
         case 0x42000000:
         case 0x42800000:
         case 0x43000000:
         case 0x3b800000:
         case 0x3c000000:
         case 0x3c800000:
         case 0x3d000000:
         case 0x3d800000:
         case 0x3e000000:
         case 0x3e800000:
         case 0x3f000000:
                  }
      void
   qpu_serialize_one_inst(struct vc4_compile *c, uint64_t inst)
   {
         if (c->qpu_inst_count >= c->qpu_inst_size) {
            c->qpu_inst_size = MAX2(16, c->qpu_inst_size * 2);
      }
   }
