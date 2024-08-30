   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2021 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_compiler.h"
   #include "agx_debug.h"
      /* Validatation doesn't make sense in release builds */
   #ifndef NDEBUG
      #define agx_validate_assert(stmt)                                              \
      if (!(stmt)) {                                                              \
               /*
   * If a block contains phi nodes, they must come at the start of the block. If a
   * block contains control flow, it must come at the beginning/end as applicable.
   * Therefore the form of a valid block is:
   *
   *       Control flow instructions (else)
   *       Phi nodes
   *       General instructions
   *       Control flow instructions (except else)
   *
   * Validate that this form is satisfied.
   *
   * XXX: This only applies before we delete the logical end instructions, maybe
   * that should be deferred though?
   */
   enum agx_block_state {
      AGX_BLOCK_STATE_CF_ELSE = 0,
   AGX_BLOCK_STATE_PHI = 1,
   AGX_BLOCK_STATE_BODY = 2,
      };
      static bool
   agx_validate_block_form(agx_block *block)
   {
               agx_foreach_instr_in_block(block, I) {
      switch (I->op) {
   case AGX_OPCODE_ELSE_ICMP:
   case AGX_OPCODE_ELSE_FCMP:
                  case AGX_OPCODE_PHI:
                                 default:
      if (instr_after_logical_end(I)) {
         } else {
      agx_validate_assert(state != AGX_BLOCK_STATE_CF);
      }
                     }
      static bool
   agx_validate_sources(agx_instr *I)
   {
      agx_foreach_src(I, s) {
               if (src.type == AGX_INDEX_IMMEDIATE) {
      agx_validate_assert(!src.kill);
                           /* Immediates are encoded as 8-bit (16-bit for memory load/store). For
   * integers, they extend to 16-bit. For floating point, they are 8-bit
   * minifloats. The 8-bit minifloats are a strict subset of 16-bit
   * standard floats, so we treat them as such in the IR, with an
   * implicit f16->f32 for 32-bit floating point operations.
   */
   agx_validate_assert(src.size == AGX_SIZE_16);
      } else if (I->op == AGX_OPCODE_COLLECT && !agx_is_null(src)) {
                        }
      static bool
   agx_validate_defs(agx_instr *I, BITSET_WORD *defs)
   {
      agx_foreach_ssa_src(I, s) {
      /* Skip phis, they're special in loop headers */
   if (I->op == AGX_OPCODE_PHI)
            /* Sources must be defined before their use */
   if (!BITSET_TEST(defs, I->src[s].value))
               agx_foreach_ssa_dest(I, d) {
      /* Static single assignment */
   if (BITSET_TEST(defs, I->dest[d].value))
                           }
      /*
   * Type check the dimensionality of sources and destinations. This occurs in two
   * passes, first to gather all destination sizes, second to validate all source
   * sizes. Depends on SSA form.
   */
   static bool
   agx_validate_width(agx_context *ctx)
   {
      bool succ = true;
            agx_foreach_instr_global(ctx, I) {
      agx_foreach_dest(I, d) {
                                                   agx_foreach_instr_global(ctx, I) {
      agx_foreach_ssa_src(I, s) {
                     if (width[v] != n) {
      succ = false;
   fprintf(stderr, "source %u, expected width %u, got width %u\n", s,
         agx_print_instr(I, stderr);
                     free(width);
      }
      static bool
   agx_validate_predecessors(agx_block *block)
   {
      /* Loop headers (only) have predecessors that are later in source form */
            agx_foreach_predecessor(block, pred) {
      if ((*pred)->index >= block->index)
               if (has_later_preds && !block->loop_header)
            /* Successors and predecessors are found together */
   agx_foreach_predecessor(block, pred) {
               agx_foreach_successor((*pred), succ) {
      if (succ == block)
               if (!found)
                  }
      static bool
   agx_validate_sr(const agx_instr *I)
   {
      bool none = (I->op == AGX_OPCODE_GET_SR);
   bool coverage = (I->op == AGX_OPCODE_GET_SR_COVERAGE);
            /* Filter get_sr instructions */
   if (!(none || coverage || barrier))
            switch (I->sr) {
   case AGX_SR_ACTIVE_THREAD_INDEX_IN_QUAD:
   case AGX_SR_ACTIVE_THREAD_INDEX_IN_SUBGROUP:
   case AGX_SR_COVERAGE_MASK:
   case AGX_SR_IS_ACTIVE_THREAD:
            case AGX_SR_OPFIFO_CMD:
   case AGX_SR_OPFIFO_DATA_L:
   case AGX_SR_OPFIFO_DATA_H:
            default:
            }
      void
   agx_validate(agx_context *ctx, const char *after)
   {
               if (agx_compiler_debug & AGX_DBG_NOVALIDATE)
                     agx_foreach_block(ctx, block) {
      if ((int)block->index < last_index) {
      fprintf(stderr, "Out-of-order block index %d vs %d after %s\n",
         agx_print_block(block, stderr);
                        if (!agx_validate_block_form(block)) {
      fprintf(stderr, "Invalid block form after %s\n", after);
   agx_print_block(block, stderr);
               if (!agx_validate_predecessors(block)) {
      fprintf(stderr, "Invalid loop header flag after %s\n", after);
   agx_print_block(block, stderr);
                  {
               agx_foreach_instr_global(ctx, I) {
      if (!agx_validate_defs(I, defs)) {
      fprintf(stderr, "Invalid defs after %s\n", after);
   agx_print_instr(I, stderr);
                              agx_foreach_instr_global(ctx, I) {
      if (!agx_validate_sources(I)) {
      fprintf(stderr, "Invalid sources form after %s\n", after);
   agx_print_instr(I, stderr);
               if (!agx_validate_sr(I)) {
      fprintf(stderr, "Invalid SR after %s\n", after);
   agx_print_instr(I, stdout);
                  if (!agx_validate_width(ctx)) {
      fprintf(stderr, "Invalid vectors after %s\n", after);
                        if (fail) {
      agx_print_shader(ctx, stderr);
         }
      #endif /* NDEBUG */
