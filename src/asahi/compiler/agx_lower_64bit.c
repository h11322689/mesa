   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_builder.h"
   #include "agx_compiler.h"
      /*
   * Lower 64-bit moves to 32-bit moves. Although there are not 64-bit moves in
   * the ISA, it is convenient to pretend there are for instruction selection.
   * They are lowered trivially after register allocation.
   *
   * General 64-bit lowering happens in nir_lower_int64.
   */
   static bool
   lower(agx_builder *b, agx_instr *I)
   {
      if (I->op != AGX_OPCODE_MOV && I->op != AGX_OPCODE_MOV_IMM)
            if (I->dest[0].size != AGX_SIZE_64)
            agx_index dest = I->dest[0];
            if (I->op == AGX_OPCODE_MOV) {
      assert(I->src[0].type == AGX_INDEX_REGISTER ||
         assert(I->src[0].size == AGX_SIZE_64);
   agx_index src = I->src[0];
            /* Low 32-bit */
            /* High 32-bits */
   dest.value += 2;
   src.value += 2;
      } else {
      /* Low 32-bit */
            /* High 32-bits */
   dest.value += 2;
                  }
      void
   agx_lower_64bit_postra(agx_context *ctx)
   {
      agx_foreach_instr_global_safe(ctx, I) {
               if (lower(&b, I))
         }
