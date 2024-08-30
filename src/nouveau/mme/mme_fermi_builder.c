   /*
   * Copyright © 2022 Mary Guillemard
   * SPDX-License-Identifier: MIT
   */
   #include "mme_builder.h"
      #include <stdio.h>
   #include <stdlib.h>
      #include "util/u_math.h"
      void
   mme_fermi_builder_init(struct mme_builder *b)
   {
      /* R0 is reserved for the zero register */
            /* Pre-allocate R1 for the first parameter value */
   ASSERTED struct mme_value r1 = mme_reg_alloc_alloc(&b->reg_alloc);
      }
      static inline bool
   mme_fermi_is_zero_or_reg(struct mme_value x)
   {
      switch (x.type) {
   case MME_VALUE_TYPE_ZERO:  return true;
   case MME_VALUE_TYPE_IMM:   return x.imm == 0;
   case MME_VALUE_TYPE_REG:   return true;
   default: unreachable("Invalid MME value type");
      }
      static inline bool
   mme_fermi_is_zero_or_imm(struct mme_value x)
   {
      switch (x.type) {
   case MME_VALUE_TYPE_ZERO:  return true;
   case MME_VALUE_TYPE_IMM:   return true;
   case MME_VALUE_TYPE_REG:   return false;
   default: unreachable("Invalid MME value type");
      }
      static inline enum mme_fermi_reg
   mme_value_alu_reg(struct mme_value val)
   {
               switch (val.type) {
   case MME_VALUE_TYPE_ZERO:
         case MME_VALUE_TYPE_REG:
      assert(val.reg > 0 && val.reg <= 7);
      case MME_VALUE_TYPE_IMM:
         }
      }
      static inline uint32_t
   mme_value_alu_imm(struct mme_value val)
   {
               switch (val.type) {
   case MME_VALUE_TYPE_ZERO:
         case MME_VALUE_TYPE_IMM:
         case MME_VALUE_TYPE_REG:
         }
      }
      static inline void
   mme_free_reg_if_tmp(struct mme_builder *b,
               {
      if (!mme_is_zero(data) &&
      !mme_is_zero(maybe_tmp) &&
   data.type != maybe_tmp.type)
   }
      static void
   mme_fermi_new_inst(struct mme_fermi_builder *b)
   {
      struct mme_fermi_inst noop = { MME_FERMI_INST_DEFAULTS };
   assert(b->inst_count < ARRAY_SIZE(b->insts));
   b->insts[b->inst_count] = noop;
   b->inst_count++;
      }
      static struct mme_fermi_inst *
   mme_fermi_cur_inst(struct mme_fermi_builder *b)
   {
      assert(b->inst_count > 0 && b->inst_count < ARRAY_SIZE(b->insts));
      }
      void
   mme_fermi_add_inst(struct mme_builder *b,
         {
               if (fb->inst_parts || fb->inst_count == 0)
            *mme_fermi_cur_inst(fb) = *inst;
      }
      static inline void
   mme_fermi_set_inst_parts(struct mme_fermi_builder *b,
         {
      assert(!(b->inst_parts & parts));
      }
      static inline bool
   mme_fermi_next_inst_can_fit_a_full_inst(struct mme_fermi_builder *b)
   {
         }
      void
   mme_fermi_mthd_arr(struct mme_builder *b,
         {
      struct mme_fermi_builder *fb = &b->fermi;
            if (!mme_fermi_next_inst_can_fit_a_full_inst(fb))
                              if (index.type == MME_VALUE_TYPE_REG) {
         } else if (index.type == MME_VALUE_TYPE_IMM) {
                  inst->op = MME_FERMI_OP_ADD_IMM;
   inst->src[0] = mme_value_alu_reg(src_reg);
   inst->imm = mthd_imm;
   inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE_SET_MADDR;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
      }
      static inline bool
   mme_fermi_prev_inst_can_emit(struct mme_fermi_builder *b, struct mme_value data) {
      if (mme_fermi_is_empty(b)) {
                  if ((b->inst_parts & MME_FERMI_INSTR_PART_ASSIGN) == MME_FERMI_INSTR_PART_ASSIGN) {
               if (inst->assign_op == MME_FERMI_ASSIGN_OP_MOVE && data.type == MME_VALUE_TYPE_REG &&
      mme_value_alu_reg(data) == inst->dst) {
                     }
      static inline bool
   mme_fermi_next_inst_can_emit(struct mme_fermi_builder *fb,
         {
      if (mme_fermi_is_empty(fb))
            if (fb->inst_parts == 0)
               }
      static inline struct mme_value
   mme_fermi_reg(uint32_t reg)
   {
      struct mme_value val = {
      .type = MME_VALUE_TYPE_REG,
      };
      }
      static bool
   is_int18(uint32_t i)
   {
         }
      static inline void
   mme_fermi_add_imm18(struct mme_fermi_builder *fb,
                     {
      assert(dst.type == MME_VALUE_TYPE_REG &&
            if (!mme_fermi_next_inst_can_fit_a_full_inst(fb)) {
                           inst->op = MME_FERMI_OP_ADD_IMM;
   inst->src[0] = mme_value_alu_reg(src);
   inst->imm = imm & BITFIELD_MASK(18);
   inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
      }
      static bool
   mme_fermi_bfe_lsl_can_use_imm(struct mme_fermi_builder *b,
               {
      return (mme_fermi_is_zero_or_reg(src_bits) &&
            }
      static bool
   mme_fermi_bfe_lsl_can_use_reg(struct mme_fermi_builder *b,
               {
      return (mme_fermi_is_zero_or_imm(src_bits) &&
            }
      static void
   mme_fermi_bfe(struct mme_fermi_builder *fb,
               struct mme_value dst_reg,
   struct mme_value src_bits,
   struct mme_value src_reg,
   {
      assert(dst_reg.type == MME_VALUE_TYPE_REG &&
         mme_fermi_is_zero_or_reg(src_reg) &&
            if (!mme_fermi_next_inst_can_fit_a_full_inst(fb))
                     if (mme_fermi_bfe_lsl_can_use_imm(fb, src_bits, dst_bits)) {
      inst->op = MME_FERMI_OP_BFE_LSL_IMM;
   inst->src[0] = mme_value_alu_reg(src_bits);
   inst->src[1] = mme_value_alu_reg(src_reg);
   inst->bitfield.dst_bit = mme_value_alu_imm(dst_bits);
      } else if (mme_fermi_bfe_lsl_can_use_reg(fb, src_bits, dst_bits)) {
      inst->op = MME_FERMI_OP_BFE_LSL_REG;
   inst->src[0] = mme_value_alu_reg(dst_bits);
   inst->src[1] = mme_value_alu_reg(src_reg);
   inst->bitfield.src_bit = mme_value_alu_imm(src_bits);
               inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
      }
      static void
   mme_fermi_sll_to(struct mme_fermi_builder *b,
                     {
                  }
      static void
   mme_fermi_srl_to(struct mme_fermi_builder *b,
                     {
                  }
      void
   mme_fermi_bfe_to(struct mme_builder *b, struct mme_value dst,
         {
      struct mme_fermi_builder *fb = &b->fermi;
               }
      static struct mme_value
   mme_fermi_load_imm_to_reg(struct mme_builder *b, struct mme_value data)
   {
               assert(data.type == MME_VALUE_TYPE_IMM ||
            /* If the immediate is zero, we can simplify this */
   if (mme_is_zero(data)) {
         } else {
                        if (is_int18(imm)) {
         } else {
      /* TODO: a possible optimisation involve searching for the first bit
   * offset and see if it can fit in 16 bits.
   */
                  mme_fermi_add_imm18(fb, dst, mme_zero(), high_bits);
   mme_fermi_sll_to(fb, dst, dst, mme_imm(16));
                     }
      static inline struct mme_value
   mme_fermi_value_as_reg(struct mme_builder *b,
         {
      if (data.type == MME_VALUE_TYPE_REG || mme_is_zero(data)) {
                     }
      void mme_fermi_emit(struct mme_builder *b,
         {
      struct mme_fermi_builder *fb = &b->fermi;
            /* Check if previous assign was to the same dst register and modify assign
   * mode if needed
   */
   if (mme_fermi_prev_inst_can_emit(fb, data)) {
      inst = mme_fermi_cur_inst(fb);
      } else {
               /* Because of mme_fermi_value_as_reg, it is possible that a new load
   * that can be simplify
   */
   if (mme_fermi_prev_inst_can_emit(fb, data_reg)) {
      inst = mme_fermi_cur_inst(fb);
      } else {
                     inst = mme_fermi_cur_inst(fb);
   inst->op = MME_FERMI_OP_ALU_REG;
   inst->alu_op = MME_FERMI_ALU_OP_ADD;
   inst->src[0] = mme_value_alu_reg(data_reg);
   inst->src[1] = MME_FERMI_REG_ZERO;
                  mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
                     }
      static void
   mme_fermi_branch(struct mme_fermi_builder *fb,
         {
      if (fb->inst_parts || mme_fermi_is_empty(fb))
                     inst->op = MME_FERMI_OP_BRANCH;
   inst->src[0] = src;
   inst->imm = offset;
   inst->branch.no_delay = true;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
      }
      static void
   mme_fermi_start_cf(struct mme_builder *b,
                     {
               /* The condition here is inverted because we want to branch and skip the
   * block when the condition fails.
   */
   assert(mme_fermi_is_zero_or_reg(cond));
            uint16_t ip = fb->inst_count - 1;
            assert(fb->cf_depth < ARRAY_SIZE(fb->cf_stack));
   fb->cf_stack[fb->cf_depth++] = (struct mme_cf) {
      .type = type,
               /* The inside of control-flow needs to start with a new instruction */
      }
      static struct mme_cf
   mme_fermi_end_cf(struct mme_builder *b, enum mme_cf_type type)
   {
               if (fb->inst_parts)
            assert(fb->cf_depth > 0);
   struct mme_cf cf = fb->cf_stack[--fb->cf_depth];
            assert(fb->insts[cf.start_ip].op == MME_FERMI_OP_BRANCH);
               }
      static struct mme_value
   mme_fermi_neq(struct mme_builder *b, struct mme_value x, struct mme_value y)
   {
               /* Generate some value that's non-zero if x != y */
   struct mme_value res = mme_alloc_reg(b);
   if (x.type == MME_VALUE_TYPE_IMM && is_int18(-x.imm)) {
         } else if (y.type == MME_VALUE_TYPE_IMM && is_int18(-y.imm)) {
         } else {
         }
      }
      void
   mme_fermi_start_if(struct mme_builder *b,
                     enum mme_cmp_op op,
   {
               if (mme_is_zero(x)) {
         } else if (mme_is_zero(y)) {
         } else {
      struct mme_value tmp = mme_fermi_neq(b, x, y);
   mme_fermi_start_cf(b, MME_CF_TYPE_IF, tmp, if_true);
         }
      void
   mme_fermi_end_if(struct mme_builder *b)
   {
         }
      void
   mme_fermi_start_while(struct mme_builder *b)
   {
         }
      static void
   mme_fermi_end_while_zero(struct mme_builder *b,
                     {
               if (fb->inst_parts)
            int delta = fb->inst_count - cf.start_ip - 2;
      }
      void
   mme_fermi_end_while(struct mme_builder *b,
                     enum mme_cmp_op op,
   {
                        if (mme_is_zero(x)) {
         } else if (mme_is_zero(y)) {
         } else {
      struct mme_value tmp = mme_fermi_neq(b, x, y);
   mme_fermi_end_while_zero(b, cf, tmp, if_true);
         }
      void
   mme_fermi_start_loop(struct mme_builder *b,
         {
               assert(mme_is_zero(fb->loop_counter));
               }
      void
   mme_fermi_end_loop(struct mme_builder *b)
   {
               mme_sub_to(b, fb->loop_counter, fb->loop_counter, mme_imm(1));
            mme_free_reg(b, fb->loop_counter);
      }
      static inline bool
   mme_fermi_next_inst_can_load_to(struct mme_fermi_builder *b)
   {
         }
      void mme_fermi_load_to(struct mme_builder *b,
         {
               assert(dst.type == MME_VALUE_TYPE_REG ||
            if (!fb->first_loaded) {
      struct mme_value r1 = {
      .type = MME_VALUE_TYPE_REG,
      };
   mme_mov_to(b, dst, r1);
   mme_free_reg(b, r1);
   fb->first_loaded = true;
               if (!mme_fermi_next_inst_can_load_to(fb))
                     inst->assign_op = MME_FERMI_ASSIGN_OP_LOAD;
               }
         struct mme_value
   mme_fermi_load(struct mme_builder *b)
   {
               if (!fb->first_loaded) {
      struct mme_value r1 = {
      .type = MME_VALUE_TYPE_REG,
      };
   fb->first_loaded = true;
               struct mme_value dst = mme_alloc_reg(b);
               }
      static enum mme_fermi_alu_op
   mme_to_fermi_alu_op(enum mme_alu_op op)
   {
         #define ALU_CASE(op) case MME_ALU_OP_##op: return MME_FERMI_ALU_OP_##op;
      ALU_CASE(ADD)
   ALU_CASE(ADDC)
   ALU_CASE(SUB)
   ALU_CASE(SUBB)
   ALU_CASE(AND)
   ALU_CASE(NAND)
   ALU_CASE(OR)
      #undef ALU_CASE
      default:
            }
      void
   mme_fermi_alu_to(struct mme_builder *b,
                  struct mme_value dst,
      {
               switch (op) {
   case MME_ALU_OP_ADD:
      if (x.type == MME_VALUE_TYPE_IMM && x.imm != 0 && is_int18(x.imm)) {
      mme_fermi_add_imm18(fb, dst, y, x.imm);
      }
   if (y.type == MME_VALUE_TYPE_IMM && y.imm != 0 && is_int18(y.imm)) {
      mme_fermi_add_imm18(fb, dst, x, y.imm);
      }
      case MME_ALU_OP_SUB:
      if (y.type == MME_VALUE_TYPE_IMM && is_int18(-y.imm)) {
      mme_fermi_add_imm18(fb, dst, x, -y.imm);
      }
      case MME_ALU_OP_SLL:
      mme_fermi_sll_to(fb, dst, x, y);
      case MME_ALU_OP_SRL:
      mme_fermi_srl_to(fb, dst, x, y);
      default:
                           struct mme_value x_reg = mme_fermi_value_as_reg(b, x);
            if (!mme_fermi_next_inst_can_fit_a_full_inst(fb))
            struct mme_fermi_inst *inst = mme_fermi_cur_inst(fb);
   inst->op = MME_FERMI_OP_ALU_REG;
   inst->alu_op = mme_to_fermi_alu_op(op);
   inst->src[0] = mme_value_alu_reg(x_reg);
   inst->src[1] = mme_value_alu_reg(y_reg);
   inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
            mme_free_reg_if_tmp(b, x, x_reg);
      }
         void mme_fermi_state_arr_to(struct mme_builder *b,
                     {
               assert(mme_fermi_is_zero_or_reg(dst));
                     if (!mme_fermi_next_inst_can_fit_a_full_inst(fb))
            struct mme_fermi_inst *inst = mme_fermi_cur_inst(fb);
   inst->op = MME_FERMI_OP_STATE;
   inst->src[0] = mme_value_alu_reg(index_reg);
   inst->src[1] = MME_FERMI_REG_ZERO;
   inst->imm = state >> 2;
   inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
               }
      void
   mme_fermi_merge_to(struct mme_builder *b, struct mme_value dst,
               {
               assert(mme_fermi_is_zero_or_reg(dst));
   assert(dst_pos < 32);
   assert(bits < 32);
            struct mme_value x_reg = mme_fermi_value_as_reg(b, x);
            if (!mme_fermi_next_inst_can_fit_a_full_inst(fb))
                     inst->op = MME_FERMI_OP_MERGE;
   inst->src[0] = mme_value_alu_reg(x_reg);
   inst->src[1] = mme_value_alu_reg(y_reg);
   inst->bitfield.dst_bit = dst_pos;
   inst->bitfield.src_bit = src_pos;
            inst->assign_op = MME_FERMI_ASSIGN_OP_MOVE;
            mme_fermi_set_inst_parts(fb, MME_FERMI_INSTR_PART_OP |
            mme_free_reg_if_tmp(b, x, x_reg);
      }
      uint32_t *
   mme_fermi_builder_finish(struct mme_fermi_builder *b, size_t *size_out)
   {
               /* TODO: If there are at least two instructions and we can guarantee the
   * last two instructions get exeucted (not in control-flow), we don't need
   * to add a pair of NOPs.
   */
   mme_fermi_new_inst(b);
                     size_t enc_size = b->inst_count * sizeof(uint32_t);
   uint32_t *enc = malloc(enc_size);
   if (enc != NULL) {
      mme_fermi_encode(enc, b->inst_count, b->insts);
      }
      }
      void
   mme_fermi_builder_dump(struct mme_builder *b, FILE *fp)
   {
                  }
