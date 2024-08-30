   /*
   * Copyright (C) 2021 Collabora, Ltd.
   * Copyright (C) 2014 Valve Corporation
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
      #include "bi_builder.h"
   #include "compiler.h"
      #define XXH_INLINE_ALL
   #include "util/xxhash.h"
      /* This pass handles CSE'ing repeated expressions created in the process of
   * translating from NIR. Also, currently this is intra-block only, to make it
   * work over multiple block we'd need to bring forward dominance calculation.
   */
      static inline uint32_t
   HASH(uint32_t hash, unsigned data)
   {
         }
      static uint32_t
   hash_index(uint32_t hash, bi_index index)
   {
      hash = HASH(hash, index.value);
   hash = HASH(hash, index.abs);
   hash = HASH(hash, index.neg);
   hash = HASH(hash, index.swizzle);
   hash = HASH(hash, index.offset);
   hash = HASH(hash, index.type);
      }
      /* Hash an ALU instruction. */
   static uint32_t
   hash_instr(const void *data)
   {
      const bi_instr *I = data;
            hash = HASH(hash, I->op);
   hash = HASH(hash, I->nr_dests);
                     /* Explcitly skip destinations, except for size details */
   bi_foreach_dest(I, d) {
                  bi_foreach_src(I, s) {
                  /* Explicitly skip branch, regfmt, vecsize, no_spill, tdd, table */
            /* Explicitly skip other immediates */
            for (unsigned i = 0; i < ARRAY_SIZE(I->flags); ++i)
               }
      static bool
   instrs_equal(const void *_i1, const void *_i2)
   {
               if (i1->op != i2->op)
         if (i1->nr_srcs != i2->nr_srcs)
         if (i1->nr_dests != i2->nr_dests)
                     bi_foreach_src(i1, s) {
               if (memcmp(&s1, &s2, sizeof(s1)) != 0)
               if (i1->dest_mod != i2->dest_mod)
            if (i1->shift != i2->shift)
            for (unsigned i = 0; i < ARRAY_SIZE(i1->flags); ++i) {
      if (i1->flags[i] != i2->flags[i])
                  }
      /* Determines what instructions the above routines have to handle */
      static bool
   instr_can_cse(const bi_instr *I)
   {
      switch (I->op) {
   case BI_OPCODE_DTSEL_IMM:
   case BI_OPCODE_DISCARD_F32:
         default:
                  /* Be conservative about which message-passing instructions we CSE,
   * since most are not pure even within a thread.
   */
   if (bi_opcode_props[I->op].message && I->op != BI_OPCODE_LEA_BUF_IMM)
            if (I->branch_target)
               }
      void
   bi_opt_cse(bi_context *ctx)
   {
               bi_foreach_block(ctx, block) {
      bi_index *replacement = calloc(sizeof(bi_index), ctx->ssa_alloc);
            bi_foreach_instr_in_block(block, instr) {
      /* Rewrite before trying to CSE anything so we converge
   * locally in one iteration */
   bi_foreach_ssa_src(instr, s) {
                     bi_index repl = replacement[instr->src[s].value];
   if (!bi_is_null(repl))
                              bool found;
   struct set_entry *entry =
                           bi_foreach_dest(instr, d) {
                                       }
