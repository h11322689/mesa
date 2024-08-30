   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "agx_builder.h"
   #include "agx_compiler.h"
   #include "agx_debug.h"
      #define AGX_MAX_PENDING (8)
      /*
   * Returns whether an instruction is asynchronous and needs a scoreboard slot
   */
   static bool
   instr_is_async(agx_instr *I)
   {
         }
      struct slot {
      /* Set of registers this slot is currently writing */
            /* Number of pending messages on this slot. Must not exceed
   * AGX_MAX_PENDING for correct results.
   */
      };
      /*
   * Insert waits within a block to stall after every async instruction. Useful
   * for debugging.
   */
   static void
   agx_insert_waits_trivial(agx_context *ctx, agx_block *block)
   {
      agx_foreach_instr_in_block_safe(block, I) {
      if (instr_is_async(I)) {
      agx_builder b = agx_init_builder(ctx, agx_after_instr(I));
            }
      /*
   * Insert waits within a block, assuming scoreboard slots have already been
   * assigned. This waits for everything at the end of the block, rather than
   * doing something more intelligent/global. This should be optimized.
   *
   * XXX: Do any instructions read their sources asynchronously?
   */
   static void
   agx_insert_waits_local(agx_context *ctx, agx_block *block)
   {
               agx_foreach_instr_in_block_safe(block, I) {
               /* Check for read-after-write */
   agx_foreach_src(I, s) {
                     unsigned nr_read = agx_read_registers(I, s);
   for (unsigned slot = 0; slot < ARRAY_SIZE(slots); ++slot) {
      if (BITSET_TEST_RANGE(slots[slot].writes, I->src[s].value,
                        /* Check for write-after-write */
   agx_foreach_dest(I, d) {
                     unsigned nr_writes = agx_write_registers(I, d);
   for (unsigned slot = 0; slot < ARRAY_SIZE(slots); ++slot) {
      if (BITSET_TEST_RANGE(slots[slot].writes, I->dest[d].value,
                        /* Check for barriers */
   if (I->op == AGX_OPCODE_THREADGROUP_BARRIER ||
               for (unsigned slot = 0; slot < ARRAY_SIZE(slots); ++slot) {
      if (slots[slot].nr_pending)
                  /* Try to assign a free slot */
   if (instr_is_async(I)) {
      for (unsigned slot = 0; slot < ARRAY_SIZE(slots); ++slot) {
      if (slots[slot].nr_pending == 0) {
      I->scoreboard = slot;
                     /* Check for slot overflow */
   if (instr_is_async(I) &&
                  /* Insert the appropriate waits, clearing the slots */
   u_foreach_bit(slot, wait_mask) {
                     BITSET_ZERO(slots[slot].writes);
               /* Record access */
   if (instr_is_async(I)) {
      agx_foreach_dest(I, d) {
                     assert(I->dest[d].type == AGX_INDEX_REGISTER);
   BITSET_SET_RANGE(slots[I->scoreboard].writes, I->dest[d].value,
                              /* If there are outstanding messages, wait for them. We don't do this for the
   * exit block, though, since nothing else will execute in the shader so
   * waiting is pointless.
   */
   if (block != agx_exit_block(ctx)) {
               for (unsigned slot = 0; slot < ARRAY_SIZE(slots); ++slot) {
      if (slots[slot].nr_pending)
            }
      /*
   * Assign scoreboard slots to asynchronous instructions and insert waits for the
   * appropriate hazard tracking.
   */
   void
   agx_insert_waits(agx_context *ctx)
   {
      agx_foreach_block(ctx, block) {
      if (agx_compiler_debug & AGX_DBG_WAIT)
         else
         }
