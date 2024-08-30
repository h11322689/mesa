   /*
   * Copyright (C) 2020 Collabora, Ltd.
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
      #include "bi_quirks.h"
   #include "compiler.h"
      /* This file contains the final passes of the compiler. Running after
   * scheduling and RA, the IR is now finalized, so we need to emit it to actual
   * bits on the wire (as well as fixup branches) */
      static uint64_t
   bi_pack_header(bi_clause *clause, bi_clause *next_1, bi_clause *next_2)
   {
      /* next_dependencies are the union of the dependencies of successors'
            unsigned dependency_wait = next_1 ? next_1->dependencies : 0;
            /* Signal barriers (slot #7) immediately. This is not optimal but good
   * enough. Doing better requires extending the IR and scheduler.
   */
   if (clause->message_type == BIFROST_MESSAGE_BARRIER)
            bool staging_barrier = next_1 ? next_1->staging_barrier : false;
            struct bifrost_header header = {
      .flow_control = (next_1 == NULL && next_2 == NULL) ? BIFROST_FLOW_END
         .terminate_discarded_threads = clause->td,
   .next_clause_prefetch = clause->next_clause_prefetch && next_1,
   .staging_barrier = staging_barrier,
   .staging_register = clause->staging_register,
   .dependency_wait = dependency_wait,
   .dependency_slot = clause->scoreboard_id,
   .message_type = clause->message_type,
   .next_message_type = next_1 ? next_1->message_type : 0,
               uint64_t u = 0;
   memcpy(&u, &header, sizeof(header));
      }
      /* Assigns a slot for reading, before anything is written */
      static void
   bi_assign_slot_read(bi_registers *regs, bi_index src)
   {
      /* We only assign for registers */
   if (src.type != BI_INDEX_REGISTER)
            /* Check if we already assigned the slot */
   for (unsigned i = 0; i <= 1; ++i) {
      if (regs->slot[i] == src.value && regs->enabled[i])
               if (regs->slot[2] == src.value && regs->slot23.slot2 == BIFROST_OP_READ)
                     for (unsigned i = 0; i <= 1; ++i) {
      if (!regs->enabled[i]) {
      regs->slot[i] = src.value;
   regs->enabled[i] = true;
                  if (!regs->slot23.slot3) {
      regs->slot[2] = src.value;
   regs->slot23.slot2 = BIFROST_OP_READ;
               bi_print_slots(regs, stderr);
      }
      static bi_registers
   bi_assign_slots(bi_tuple *now, bi_tuple *prev)
   {
      /* We assign slots for the main register mechanism. Special ops
   * use the data registers, which has its own mechanism entirely
            bool read_dreg = now->add && bi_opcode_props[now->add->op].sr_read;
                     if (now->fma)
      bi_foreach_src(now->fma, src)
         if (now->add) {
      bi_foreach_src(now->add, src) {
      /* This is not a real source, we shouldn't assign a
   * slot for it.
   */
                  if (!(src == 0 && read_dreg))
                  /* Next, assign writes. Staging writes are assigned separately, but
   * +ATEST wants its destination written to both a staging register
            if (prev->add && prev->add->nr_dests &&
      (!write_dreg || prev->add->op == BI_OPCODE_ATEST)) {
            if (idx.type == BI_INDEX_REGISTER) {
      now->regs.slot[3] = idx.value;
                  if (prev->fma && prev->fma->nr_dests) {
               if (idx.type == BI_INDEX_REGISTER) {
      if (now->regs.slot23.slot3) {
      /* Scheduler constraint: cannot read 3 and write 2 */
   assert(!now->regs.slot23.slot2);
   now->regs.slot[2] = idx.value;
      } else {
      now->regs.slot[3] = idx.value;
   now->regs.slot23.slot3 = BIFROST_OP_WRITE;
                        }
      static enum bifrost_reg_mode
   bi_pack_register_mode(bi_registers r)
   {
      /* Handle idle as a special case */
   if (!(r.slot23.slot2 | r.slot23.slot3))
            /* Otherwise, use the LUT */
   for (unsigned i = 0; i < ARRAY_SIZE(bifrost_reg_ctrl_lut); ++i) {
      if (memcmp(bifrost_reg_ctrl_lut + i, &r.slot23, sizeof(r.slot23)) == 0)
               bi_print_slots(&r, stderr);
      }
      static uint64_t
   bi_pack_registers(bi_registers regs)
   {
      enum bifrost_reg_mode mode = bi_pack_register_mode(regs);
   struct bifrost_regs s = {0};
            /* Need to pack 5-bit mode as a 4-bit field. The decoder moves bit 3 to bit 4
            unsigned ctrl;
            if (regs.first_instruction) {
      /* Bit 3 implicitly must be clear for first instructions.
   * The affected patterns all write both ADD/FMA, but that
   * is forbidden for the last instruction (whose writes are
   * encoded by the first), so this does not add additional
   * encoding constraints */
            /* Move bit 4 to bit 3, since bit 3 is clear */
            /* If we can let r2 equal r3, we have to or the hardware raises
   * INSTR_INVALID_ENC (it's unclear why). */
   if (!(regs.slot23.slot2 && regs.slot23.slot3))
      } else {
      /* We force r2=r3 or not for the upper bit */
   ctrl = (mode & 0xF);
               if (regs.enabled[1]) {
      /* Gotta save that bit!~ Required by the 63-x trick */
   assert(regs.slot[1] > regs.slot[0]);
            /* Do the 63-x trick, see docs/disasm */
   if (regs.slot[0] > 31) {
      regs.slot[0] = 63 - regs.slot[0];
               assert(regs.slot[0] <= 31);
            s.ctrl = ctrl;
   s.reg1 = regs.slot[1];
      } else {
      /* slot 1 disabled, so set to zero and use slot 1 for ctrl */
   s.ctrl = 0;
            if (regs.enabled[0]) {
                     /* Rest of slot 0 in usual spot */
      } else {
      /* Bit 1 set if slot 0 also disabled */
                  /* Force r2 =/!= r3 as needed */
   if (r2_equals_r3) {
      assert(regs.slot[3] == regs.slot[2] ||
            if (regs.slot23.slot2)
         else
      } else if (!regs.first_instruction) {
      /* Enforced by the encoding anyway */
               s.reg2 = regs.slot[2];
   s.reg3 = regs.slot[3];
            memcpy(&packed, &s, sizeof(s));
      }
      /* We must ensure slot 1 > slot 0 for the 63-x trick to function, so we fix
   * this up at pack time. (Scheduling doesn't care.) */
      static void
   bi_flip_slots(bi_registers *regs)
   {
      if (regs->enabled[0] && regs->enabled[1] && regs->slot[1] < regs->slot[0]) {
      unsigned temp = regs->slot[0];
   regs->slot[0] = regs->slot[1];
         }
      static inline enum bifrost_packed_src
   bi_get_src_slot(bi_registers *regs, unsigned reg)
   {
      if (regs->slot[0] == reg && regs->enabled[0])
         else if (regs->slot[1] == reg && regs->enabled[1])
         else if (regs->slot[2] == reg && regs->slot23.slot2 == BIFROST_OP_READ)
         else
      }
      static inline enum bifrost_packed_src
   bi_get_src_new(bi_instr *ins, bi_registers *regs, unsigned s)
   {
      if (!ins || s >= ins->nr_srcs)
                     if (src.type == BI_INDEX_REGISTER)
         else if (src.type == BI_INDEX_PASS)
         else {
      /* TODO make safer */
         }
      static struct bi_packed_tuple
   bi_pack_tuple(bi_clause *clause, bi_tuple *tuple, bi_tuple *prev,
         {
      bi_assign_slots(tuple, prev);
   tuple->regs.fau_idx = tuple->fau_idx;
                              uint64_t reg = bi_pack_registers(tuple->regs);
   uint64_t fma =
      bi_pack_fma(tuple->fma, bi_get_src_new(tuple->fma, &tuple->regs, 0),
                     uint64_t add = bi_pack_add(
      tuple->add, bi_get_src_new(tuple->add, &tuple->regs, sr_read + 0),
   bi_get_src_new(tuple->add, &tuple->regs, sr_read + 1),
         if (tuple->add) {
               bool sr_write =
            if (sr_read && !bi_is_null(add->src[0])) {
                     if (sr_write)
      } else if (sr_write) {
      assert(add->dest[0].type == BI_INDEX_REGISTER);
                  struct bi_packed_tuple packed = {
      .lo = reg | (fma << 35) | ((add & 0b111111) << 58),
                  }
      /* A block contains at most one PC-relative constant, from a terminal branch.
   * Find the last instruction and if it is a relative branch, fix up the
   * PC-relative constant to contain the absolute offset. This occurs at pack
   * time instead of schedule time because the number of quadwords between each
   * block is not known until after all other passes have finished.
   */
      static void
   bi_assign_branch_offset(bi_context *ctx, bi_block *block)
   {
      if (list_is_empty(&block->clauses))
            bi_clause *clause = list_last_entry(&block->clauses, bi_clause, link);
            if (!br->branch_target)
            /* Put it in the high place */
   int32_t qwords = bi_block_offset(ctx, clause, br->branch_target);
            /* Copy so we can toy with the sign without undefined behaviour */
   uint32_t raw = 0;
            /* Clear off top bits for A1/B1 bits */
            /* Put in top 32-bits */
   assert(clause->pcrel_idx < 8);
      }
      static void
   bi_pack_constants(unsigned tuple_count, uint64_t *constants, unsigned word_idx,
               {
               /* Do more constants follow */
            /* Indexed first by tuple count and second by constant word number,
   * indicates the position in the clause */
   unsigned pos_lookup[8][3] = {
                  /* Compute the pos, and check everything is reasonable */
   assert((tuple_count - 1) < 8);
   assert(word_idx < 3);
   unsigned pos = pos_lookup[tuple_count - 1][word_idx];
            struct bifrost_fmt_constant quad = {
      .pos = pos,
   .tag = more ? BIFROST_FMTC_CONSTANTS : BIFROST_FMTC_FINAL,
   .imm_1 = constants[index + 0] >> 4,
                  }
      uint8_t
   bi_pack_literal(enum bi_clause_subword literal)
   {
      assert(literal >= BI_CLAUSE_SUBWORD_LITERAL_0);
               }
      static inline uint8_t
   bi_clause_upper(unsigned val, struct bi_packed_tuple *tuples,
         {
               /* top 3-bits of 78-bits is tuple >> 75 == (tuple >> 64) >> 11 */
   struct bi_packed_tuple tuple = tuples[val];
      }
      uint8_t
   bi_pack_upper(enum bi_clause_subword upper, struct bi_packed_tuple *tuples,
         {
      assert(upper >= BI_CLAUSE_SUBWORD_UPPER_0);
            return bi_clause_upper(upper - BI_CLAUSE_SUBWORD_UPPER_0, tuples,
      }
      uint64_t
   bi_pack_tuple_bits(enum bi_clause_subword idx, struct bi_packed_tuple *tuples,
               {
      assert(idx >= BI_CLAUSE_SUBWORD_TUPLE_0);
            unsigned val = (idx - BI_CLAUSE_SUBWORD_TUPLE_0);
                     assert(offset + nbits < 78);
            /* (X >> start) & m
   * = (((hi << 64) | lo) >> start) & m
   * = (((hi << 64) >> start) | (lo >> start)) & m
   * = { ((hi << (64 - start)) | (lo >> start)) & m if start <= 64
   *   { ((hi >> (start - 64)) | (lo >> start)) & m if start >= 64
   * = { ((hi << (64 - start)) & m) | ((lo >> start) & m) if start <= 64
   *   { ((hi >> (start - 64)) & m) | ((lo >> start) & m) if start >= 64
   *
   * By setting m = 2^64 - 1, we justify doing the respective shifts as
   * 64-bit integers. Zero special cased to avoid undefined behaviour.
            uint64_t lo = (tuple.lo >> offset);
   uint64_t hi = (offset == 0)   ? 0
                     }
      static inline uint16_t
   bi_pack_lu(enum bi_clause_subword word, struct bi_packed_tuple *tuples,
         {
      return (word >= BI_CLAUSE_SUBWORD_UPPER_0)
            }
      uint8_t
   bi_pack_sync(enum bi_clause_subword t1, enum bi_clause_subword t2,
               {
      uint8_t sync = (bi_pack_lu(t3, tuples, tuple_count) << 0) |
            if (t1 == BI_CLAUSE_SUBWORD_Z)
         else
               }
      static inline uint64_t
   bi_pack_t_ec(enum bi_clause_subword word, struct bi_packed_tuple *tuples,
         {
      if (word == BI_CLAUSE_SUBWORD_CONSTANT)
         else
      }
      static uint32_t
   bi_pack_subwords_56(enum bi_clause_subword t, struct bi_packed_tuple *tuples,
               {
      switch (t) {
   case BI_CLAUSE_SUBWORD_HEADER:
         case BI_CLAUSE_SUBWORD_RESERVED:
         case BI_CLAUSE_SUBWORD_CONSTANT:
         default:
            }
      static uint16_t
   bi_pack_subword(enum bi_clause_subword t, unsigned format,
                     {
      switch (t) {
   case BI_CLAUSE_SUBWORD_HEADER:
         case BI_CLAUSE_SUBWORD_M:
         case BI_CLAUSE_SUBWORD_CONSTANT:
      return (format == 5 || format == 10) ? (ec0 & ((1 << 15) - 1))
      case BI_CLAUSE_SUBWORD_UPPER_23:
      return (bi_clause_upper(2, tuples, tuple_count) << 12) |
      case BI_CLAUSE_SUBWORD_UPPER_56:
      return (bi_clause_upper(5, tuples, tuple_count) << 12) |
      case BI_CLAUSE_SUBWORD_UPPER_0 ... BI_CLAUSE_SUBWORD_UPPER_7:
         default:
            }
      /* EC0 is 60-bits (bottom 4 already shifted off) */
   void
   bi_pack_format(struct util_dynarray *emission, unsigned index,
               {
               uint8_t sync = bi_pack_sync(format.tag_1, format.tag_2, format.tag_3, tuples,
                     uint16_t s4 = bi_pack_subword(format.s4, format.format, tuples, tuple_count,
            uint32_t s5_s6 =
      bi_pack_subwords_56(format.s5_s6, tuples, tuple_count, header, ec0,
         uint64_t s7 = bi_pack_subword(format.s7, format.format, tuples, tuple_count,
            /* Now that subwords are packed, split into 64-bit halves and emit */
   uint64_t lo = sync | ((s0_s3 & ((1ull << 56) - 1)) << 8);
   uint64_t hi = (s0_s3 >> 56) | ((uint64_t)s4 << 4) | ((uint64_t)s5_s6 << 19) |
            util_dynarray_append(emission, uint64_t, lo);
      }
      static void
   bi_pack_clause(bi_context *ctx, bi_clause *clause, bi_clause *next_1,
               {
               for (unsigned i = 0; i < clause->tuple_count; ++i) {
      unsigned prev = ((i == 0) ? clause->tuple_count : i) - 1;
   ins[i] = bi_pack_tuple(clause, &clause->tuples[i], &clause->tuples[prev],
                     /* Different GPUs support different forms of the CLPER.i32
   * instruction. Check we use the right one for the target.
   */
   if (add && add->op == BI_OPCODE_CLPER_OLD_I32)
         else if (add && add->op == BI_OPCODE_CLPER_I32)
                        if (ec0_packed)
            unsigned constant_quads =
            uint64_t header = bi_pack_header(clause, next_1, next_2);
   uint64_t ec0 = (clause->constants[0] >> 4);
            unsigned counts[8] = {
                  unsigned indices[8][6] = {
      {1},          {0, 2},           {0, 3, 4},        {0, 3, 6},
                        for (unsigned pos = 0; pos < count; ++pos) {
      ASSERTED unsigned idx = indices[clause->tuple_count - 1][pos];
   assert(bi_clause_formats[idx].pos == pos);
   assert((bi_clause_formats[idx].tag_1 == BI_CLAUSE_SUBWORD_Z) ==
            /* Whether to end the clause immediately after the last tuple */
            bi_pack_format(emission, indices[clause->tuple_count - 1][pos], ins,
                        for (unsigned pos = 0; pos < constant_quads; ++pos) {
      bi_pack_constants(clause->tuple_count, clause->constants, pos,
         }
      static void
   bi_collect_blend_ret_addr(bi_context *ctx, struct util_dynarray *emission,
         {
      /* No need to collect return addresses when we're in a blend shader. */
   if (ctx->inputs->is_blend)
            const bi_tuple *tuple = &clause->tuples[clause->tuple_count - 1];
            if (!ins || ins->op != BI_OPCODE_BLEND)
            unsigned loc = tuple->regs.fau_idx - BIR_FAU_BLEND_0;
   assert(loc < ARRAY_SIZE(ctx->info.bifrost->blend));
   assert(!ctx->info.bifrost->blend[loc].return_offset);
   ctx->info.bifrost->blend[loc].return_offset =
            }
      /*
   * The second register destination of TEXC_DUAL is encoded into the texture
   * operation descriptor during register allocation. It's dropped as late as
   * possible (instruction packing) so the register remains recorded in the IR,
   * for clause scoreboarding and so on.
   */
   static void
   bi_lower_texc_dual(bi_context *ctx)
   {
      bi_foreach_instr_global(ctx, I) {
      if (I->op == BI_OPCODE_TEXC_DUAL) {
      /* In hardware, TEXC has 1 destination */
   I->op = BI_OPCODE_TEXC;
            }
      unsigned
   bi_pack(bi_context *ctx, struct util_dynarray *emission)
   {
                        bi_foreach_block(ctx, block) {
               bi_foreach_clause_in_block(block, clause) {
               /* Get the succeeding clauses, either two successors of
                           if (is_last) {
      next = bi_next_clause(ctx, block->successors[0], NULL);
      } else {
                                    if (!is_last)
                     }
