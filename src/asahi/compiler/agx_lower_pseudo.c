   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_builder.h"
   #include "agx_compiler.h"
   #include "agx_opcodes.h"
      /* Lower pseudo instructions created during optimization. */
      static agx_instr *
   while_for_break_if(agx_builder *b, agx_instr *I)
   {
      if (I->op == AGX_OPCODE_BREAK_IF_FCMP) {
      return agx_while_fcmp(b, I->src[0], I->src[1], I->nest, I->fcond,
      } else {
      return agx_while_icmp(b, I->src[0], I->src[1], I->nest, I->icond,
         }
      static agx_instr *
   cmpsel_for_break_if(agx_builder *b, agx_instr *I)
   {
               /* If the condition is true, set r0l to nest to break */
   agx_index t = agx_immediate(I->nest);
            if (I->invert_cond) {
      agx_index temp = t;
   t = f;
               if (I->op == AGX_OPCODE_BREAK_IF_FCMP)
         else
               }
      static agx_instr *
   lower(agx_builder *b, agx_instr *I)
   {
               /* Various instructions are implemented as bitwise truth tables */
   case AGX_OPCODE_MOV:
            case AGX_OPCODE_NOT:
            case AGX_OPCODE_AND:
            case AGX_OPCODE_XOR:
            case AGX_OPCODE_OR:
            /* Unfused comparisons are fused with a 0/1 select */
   case AGX_OPCODE_ICMP:
      return agx_icmpsel_to(b, I->dest[0], I->src[0], I->src[1],
               case AGX_OPCODE_FCMP:
      return agx_fcmpsel_to(b, I->dest[0], I->src[0], I->src[1],
               /* Writes to the nesting counter lowered to the real register */
   case AGX_OPCODE_BEGIN_CF:
            case AGX_OPCODE_BREAK:
      agx_mov_imm_to(b, agx_register(0, AGX_SIZE_16), I->nest);
         case AGX_OPCODE_BREAK_IF_ICMP:
   case AGX_OPCODE_BREAK_IF_FCMP: {
      if (I->nest == 1)
         else
               default:
            }
      void
   agx_lower_pseudo(agx_context *ctx)
   {
      agx_foreach_instr_global_safe(ctx, I) {
               if (lower(&b, I))
         }
