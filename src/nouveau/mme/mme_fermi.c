   /*
   * Copyright © 2022 Mary Guillemard
   * SPDX-License-Identifier: MIT
   */
   #include "mme_fermi.h"
   #include "mme_fermi_encode.h"
      #include "util/u_math.h"
      #define OP_TO_STR(OP) [MME_FERMI_OP_##OP] = #OP
   static const char *op_to_str[] = {
      OP_TO_STR(ALU_REG),
   OP_TO_STR(ADD_IMM),
   OP_TO_STR(MERGE),
   OP_TO_STR(BFE_LSL_IMM),
   OP_TO_STR(BFE_LSL_REG),
   OP_TO_STR(STATE),
   OP_TO_STR(UNK6),
      };
   #undef OP_TO_STR
      const char *
   mme_fermi_op_to_str(enum mme_fermi_op op)
   {
      assert(op < ARRAY_SIZE(op_to_str));
      }
      #define ALU_OP_TO_STR(OP) [MME_FERMI_ALU_OP_##OP] = #OP
   static const char *alu_op_to_str[] = {
      ALU_OP_TO_STR(ADD),
   ALU_OP_TO_STR(ADDC),
   ALU_OP_TO_STR(SUB),
   ALU_OP_TO_STR(SUBB),
   ALU_OP_TO_STR(RESERVED4),
   ALU_OP_TO_STR(RESERVED5),
   ALU_OP_TO_STR(RESERVED6),
   ALU_OP_TO_STR(RESERVED7),
   ALU_OP_TO_STR(XOR),
   ALU_OP_TO_STR(OR),
   ALU_OP_TO_STR(AND),
   ALU_OP_TO_STR(AND_NOT),
   ALU_OP_TO_STR(NAND),
   ALU_OP_TO_STR(RESERVED13),
   ALU_OP_TO_STR(RESERVED14),
   ALU_OP_TO_STR(RESERVED15),
   ALU_OP_TO_STR(RESERVED16),
   ALU_OP_TO_STR(RESERVED17),
   ALU_OP_TO_STR(RESERVED18),
   ALU_OP_TO_STR(RESERVED19),
   ALU_OP_TO_STR(RESERVED20),
   ALU_OP_TO_STR(RESERVED21),
   ALU_OP_TO_STR(RESERVED22),
   ALU_OP_TO_STR(RESERVED23),
   ALU_OP_TO_STR(RESERVED24),
   ALU_OP_TO_STR(RESERVED25),
   ALU_OP_TO_STR(RESERVED26),
   ALU_OP_TO_STR(RESERVED27),
   ALU_OP_TO_STR(RESERVED28),
   ALU_OP_TO_STR(RESERVED29),
   ALU_OP_TO_STR(RESERVED30),
      };
   #undef ALU_OP_TO_STR
      const char *
   mme_fermi_alu_op_to_str(enum mme_fermi_alu_op op)
   {
      assert(op < ARRAY_SIZE(alu_op_to_str));
      }
      #define ASSIGN_OP_TO_STR(OP) [MME_FERMI_ASSIGN_OP_##OP] = #OP
   static const char *assign_op_to_str[] = {
      ASSIGN_OP_TO_STR(LOAD),
   ASSIGN_OP_TO_STR(MOVE),
   ASSIGN_OP_TO_STR(MOVE_SET_MADDR),
   ASSIGN_OP_TO_STR(LOAD_EMIT),
   ASSIGN_OP_TO_STR(MOVE_EMIT),
   ASSIGN_OP_TO_STR(LOAD_SET_MADDR),
   ASSIGN_OP_TO_STR(MOVE_SET_MADDR_LOAD_EMIT),
      };
   #undef ASSIGN_OP_TO_STR
      const char *
   mme_fermi_assign_op_to_str(enum mme_fermi_assign_op op)
   {
      assert(op < ARRAY_SIZE(assign_op_to_str));
      }
      void mme_fermi_encode(uint32_t *out, uint32_t inst_count,
         {
      for (uint32_t i = 0; i < inst_count; i++) {
      bitmask_t enc = encode__instruction(NULL, NULL, insts[i]);
         }
      static uint64_t
   unpack_field(bitmask_t bitmask, unsigned low, unsigned high, bool is_signed)
   {
                        BITSET_ZERO(mask.bitset);
            BITSET_COPY(field.bitset, bitmask.bitset);
   BITSET_SHR(field.bitset, low);
            uint64_t data = bitmask_to_uint64_t(field);
   if (is_signed)
               }
      void mme_fermi_decode(struct mme_fermi_inst *insts,
         {
      for (uint32_t i = 0; i < inst_count; i++) {
               insts[i].op       = unpack_field(enc, 0, 3, false);
   insts[i].end_next = unpack_field(enc, 7, 7, false);
            if (insts[i].op != MME_FERMI_OP_BRANCH) {
                  if (insts[i].op == MME_FERMI_OP_ALU_REG) {
      insts[i].src[0] = unpack_field(enc, 11, 13, false);
   insts[i].src[1] = unpack_field(enc, 14, 16, false);
      } else if (insts[i].op == MME_FERMI_OP_ADD_IMM ||
            insts[i].src[0] = unpack_field(enc, 11, 13, false);
      } else if (insts[i].op == MME_FERMI_OP_MERGE ||
            insts[i].op == MME_FERMI_OP_BFE_LSL_IMM ||
   insts[i].src[0] = unpack_field(enc, 11, 13, false);
   insts[i].src[1] = unpack_field(enc, 14, 16, false);
   insts[i].bitfield.src_bit = unpack_field(enc, 17, 21, false);
   insts[i].bitfield.size = unpack_field(enc, 22, 26, false);
      } else if (insts[i].op == MME_FERMI_OP_BRANCH) {
      insts[i].branch.not_zero = unpack_field(enc, 4, 4, false);
   insts[i].branch.no_delay = unpack_field(enc, 5, 5, false);
   insts[i].src[0] = unpack_field(enc, 11, 13, false);
            }
      static void
   print_indent(FILE *fp, unsigned depth)
   {
      for (unsigned i = 0; i < depth; i++)
      }
      static void
   print_reg(FILE *fp, enum mme_fermi_reg reg)
   {
      if (reg == MME_FERMI_REG_ZERO) {
         } else {
            }
      static void
   print_imm(FILE *fp, const struct mme_fermi_inst *inst)
   {
                  }
      void
   mme_fermi_print_inst(FILE *fp, unsigned indent,
         {
               switch (inst->op) {
      case MME_FERMI_OP_ALU_REG:
      fprintf(fp, "%s", mme_fermi_alu_op_to_str(inst->alu_op));
                  if (inst->alu_op == MME_FERMI_ALU_OP_ADDC) {
         } else if (inst->alu_op == MME_FERMI_ALU_OP_SUBB) {
         }
      case MME_FERMI_OP_ADD_IMM:
   case MME_FERMI_OP_STATE:
      fprintf(fp, "%s", mme_fermi_op_to_str(inst->op));
   print_reg(fp, inst->src[0]);
   print_imm(fp, inst);
      case MME_FERMI_OP_MERGE:
      fprintf(fp, "%s", mme_fermi_op_to_str(inst->op));
   print_reg(fp, inst->src[0]);
   print_reg(fp, inst->src[1]);
   fprintf(fp, " (%u, %u, %u)", inst->bitfield.src_bit,
                  case MME_FERMI_OP_BFE_LSL_IMM:
      fprintf(fp, "%s", mme_fermi_op_to_str(inst->op));
   print_reg(fp, inst->src[0]);
   print_reg(fp, inst->src[1]);
   fprintf(fp, " (%u, %u)", inst->bitfield.dst_bit,
            case MME_FERMI_OP_BFE_LSL_REG:
      fprintf(fp, "%s", mme_fermi_op_to_str(inst->op));
   print_reg(fp, inst->src[0]);
   print_reg(fp, inst->src[1]);
   fprintf(fp, " (%u, %u)", inst->bitfield.src_bit,
            case MME_FERMI_OP_BRANCH:
      if (inst->branch.not_zero) {
         } else {
         }
                  if (inst->branch.no_delay) {
                     default:
      fprintf(fp, "%s", mme_fermi_op_to_str(inst->op));
            if (inst->op != MME_FERMI_OP_BRANCH) {
      fprintf(fp, "\n");
            fprintf(fp, "%s", mme_fermi_assign_op_to_str(inst->assign_op));
            if (inst->assign_op != MME_FERMI_ASSIGN_OP_LOAD) {
                     if (inst->end_next) {
      fprintf(fp, "\n");
   print_indent(fp, indent);
                     }
      void
   mme_fermi_print(FILE *fp, const struct mme_fermi_inst *insts,
         {
      for (uint32_t i = 0; i < inst_count; i++) {
      fprintf(fp, "%u:\n", i);
         }
