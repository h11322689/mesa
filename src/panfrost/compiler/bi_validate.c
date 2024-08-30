   /*
   * Copyright (C) 2021 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "util/u_memory.h"
   #include "compiler.h"
      /* Validatation doesn't make sense in release builds */
   #ifndef NDEBUG
      /* Validate that all sources are initialized in all read components. This is
   * required for correct register allocation. We check a weaker condition, that
   * all sources that are read are written at some point (equivalently, the live
   * set is empty at the start of the program). TODO: Strengthen */
      bool
   bi_validate_initialization(bi_context *ctx)
   {
               /* Calculate the live set */
   bi_block *entry = bi_entry_block(ctx);
            /* Validate that the live set is indeed empty */
   for (unsigned i = 0; i < ctx->ssa_alloc; ++i) {
      if (BITSET_TEST(entry->ssa_live_in, i)) {
      fprintf(stderr, "%u\n", i);
                     }
      /*
   * Validate that there are no bi_registers accessed except at the beginning of
   * the start block, and that preloads are unique. This ensures RA can coalesce
   * preloads without interference tracking.
   */
   static bool
   bi_validate_preload(bi_context *ctx)
   {
      bool start = true;
            bi_foreach_block(ctx, block) {
      bi_foreach_instr_in_block(block, I) {
      /* No instruction should have a register destination */
   bi_foreach_dest(I, d) {
      if (I->dest[d].type == BI_INDEX_REGISTER)
               /* Preloads are register moves at the start */
                                 /* Only preloads may have a register source */
   bi_foreach_src(I, s) {
      if (I->src[s].type == BI_INDEX_REGISTER && !is_preload)
               /* Check uniqueness */
                                                   /* Only the first block may preload */
                  }
      /*
   * Type check the dimensionality of sources and destinations. This occurs in two
   * passes, first to gather all destination sizes, second to validate all source
   * sizes. Depends on SSA form.
   */
   static bool
   bi_validate_width(bi_context *ctx)
   {
      bool succ = true;
            bi_foreach_instr_global(ctx, I) {
      bi_foreach_dest(I, d) {
                                             bi_foreach_instr_global(ctx, I) {
      bi_foreach_ssa_src(I, s) {
                     if (width[v] != n) {
      succ = false;
   fprintf(stderr, "source %u, expected width %u, got width %u\n", s,
         bi_print_instr(I, stderr);
                     free(width);
      }
      /*
   * Validate that all destinations of the instruction are present.
   */
   static bool
   bi_validate_dest(bi_context *ctx)
   {
               bi_foreach_instr_global(ctx, I) {
      bi_foreach_dest(I, d) {
      if (bi_is_null(I->dest[d])) {
      succ = false;
   fprintf(stderr, "expected dest %u", d);
   bi_print_instr(I, stderr);
                        }
      /*
   * Validate that phis only appear at the beginning of blocks.
   */
   static bool
   bi_validate_phi_ordering(bi_context *ctx)
   {
      bi_foreach_block(ctx, block) {
               bi_foreach_instr_in_block(block, I) {
      if (start)
         else if (I->op == BI_OPCODE_PHI)
                     }
      void
   bi_validate(bi_context *ctx, const char *after)
   {
               if (bifrost_debug & BIFROST_DBG_NOVALIDATE)
            if (!bi_validate_initialization(ctx)) {
      fprintf(stderr, "Uninitialized data read after %s\n", after);
               if (!bi_validate_preload(ctx)) {
      fprintf(stderr, "Unexpected preload after %s\n", after);
               if (!bi_validate_width(ctx)) {
      fprintf(stderr, "Unexpected vector with after %s\n", after);
               if (!bi_validate_dest(ctx)) {
      fprintf(stderr, "Unexpected source/dest after %s\n", after);
               if (!bi_validate_phi_ordering(ctx)) {
      fprintf(stderr, "Unexpected phi ordering after %s\n", after);
               if (fail) {
      bi_print_shader(ctx, stderr);
         }
      #endif /* NDEBUG */
