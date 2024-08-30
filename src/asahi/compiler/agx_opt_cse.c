   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2021 Collabora, Ltd.
   * Copyright 2014 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "util/compiler.h"
   #include "agx_builder.h"
      #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      /*
   * This pass handles CSE'ing repeated expressions created in the process of
   * translating from NIR. Also, currently this is intra-block only, to make it
   * work over multiple block we'd need to bring forward dominance calculation.
   */
      static inline uint32_t
   HASH(uint32_t hash, unsigned data)
   {
         }
      /* Hash an instruction. XXH32 isn't too speedy, so this is hotspot. */
   static uint32_t
   hash_instr(const void *data)
   {
      const agx_instr *I = data;
            /* Explicitly skip destinations, except for size and type */
   agx_foreach_dest(I, d) {
      hash = HASH(hash, ((uint32_t)I->dest[d].type) |
               /* Hash the source array as-is */
            /* Hash everything else in the instruction starting from the opcode */
               }
      static bool
   instrs_equal(const void *_i1, const void *_i2)
   {
               if (i1->op != i2->op)
         if (i1->nr_srcs != i2->nr_srcs)
         if (i1->nr_dests != i2->nr_dests)
            /* Explicitly skip everything but size and type */
   agx_foreach_dest(i1, d) {
      if (i1->dest[d].type != i2->dest[d].type)
         if (i1->dest[d].size != i2->dest[d].size)
               agx_foreach_src(i1, s) {
               if (memcmp(&s1, &s2, sizeof(s1)) != 0)
               if (i1->imm != i2->imm)
         if (i1->invert_cond != i2->invert_cond)
         if (i1->dim != i2->dim)
         if (i1->offset != i2->offset)
         if (i1->shadow != i2->shadow)
         if (i1->shift != i2->shift)
         if (i1->saturate != i2->saturate)
         if (i1->mask != i2->mask)
               }
      /* Determines what instructions the above routines have to handle */
   static bool
   instr_can_cse(const agx_instr *I)
   {
      return agx_opcodes_info[I->op].can_eliminate &&
      }
      void
   agx_opt_cse(agx_context *ctx)
   {
               agx_foreach_block(ctx, block) {
      agx_index *replacement = calloc(sizeof(agx_index), ctx->alloc);
            agx_foreach_instr_in_block(block, instr) {
      /* Rewrite as we go so we converge locally in 1 iteration */
   agx_foreach_ssa_src(instr, s) {
      agx_index repl = replacement[instr->src[s].value];
   if (!agx_is_null(repl))
                              bool found;
   struct set_entry *entry =
                           agx_foreach_dest(instr, d) {
                                       }
