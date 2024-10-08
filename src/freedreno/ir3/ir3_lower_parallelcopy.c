   /*
   * Copyright (C) 2021 Valve Corporation
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
      #include "ir3_ra.h"
   #include "ir3_shader.h"
      struct copy_src {
      unsigned flags;
   union {
      uint32_t imm;
   physreg_t reg;
         };
      struct copy_entry {
      physreg_t dst;
   unsigned flags;
               };
      static unsigned
   copy_entry_size(const struct copy_entry *entry)
   {
         }
      static struct copy_src
   get_copy_src(const struct ir3_register *reg, unsigned offset)
   {
      if (reg->flags & IR3_REG_IMMED) {
      return (struct copy_src){
      .flags = IR3_REG_IMMED,
         } else if (reg->flags & IR3_REG_CONST) {
      return (struct copy_src){
      .flags = IR3_REG_CONST,
         } else {
      return (struct copy_src){
      .flags = 0,
            }
      static void
   do_xor(struct ir3_instruction *instr, unsigned dst_num, unsigned src1_num,
         {
      struct ir3_instruction * xor
         ir3_dst_create(xor, dst_num, flags);
   ir3_src_create(xor, src1_num, flags);
               }
      static void
   do_swap(struct ir3_compiler *compiler, struct ir3_instruction *instr,
         {
               if (entry->flags & IR3_REG_HALF) {
      /* We currently make sure to never emit parallel copies where the
   * source/destination is a half-reg above the range accessable to half
   * registers. However, when a full-reg source overlaps a half-reg
   * destination or vice versa, it can be very, very complicated to come
   * up with a series of "legal" swaps and copies to resolve the
   * parallel copy. So here we provide a fallback to implement the
   * "illegal" swap instead. This may also be useful for implementing
   * "spilling" half-regs to the inaccessable space.
   */
   if (entry->src.reg >= RA_HALF_SIZE) {
                     /* Swap src and the temporary */
   do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = entry->src.reg & ~1u},
               /* If src and dst are within the same full register, then swapping src
   * with tmp above will also move dst to tmp. Account for that here.
   */
   unsigned dst =
                  /* Do the original swap with src replaced with tmp */
   do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = tmp + (entry->src.reg & 1)},
               /* Swap src and the temporary back */
   do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = entry->src.reg & ~1u},
   .dst = tmp,
                  /* If dst is not addressable, we only need to swap the arguments and
   * let the case above handle it.
   */
   if (entry->dst >= RA_HALF_SIZE) {
      do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = entry->dst},
   .dst = entry->src.reg,
                     unsigned src_num = ra_physreg_to_num(entry->src.reg, entry->flags);
            /* a5xx+ is known to support swz, which enables us to swap two registers
   * in-place. If unsupported we emulate it using the xor trick.
   */
   if (compiler->gen < 5) {
      /* Shared regs only exist since a5xx, so we don't have to provide a
   * fallback path for them.
   */
   assert(!(entry->flags & IR3_REG_SHARED));
   do_xor(instr, dst_num, dst_num, src_num, entry->flags);
   do_xor(instr, src_num, src_num, dst_num, entry->flags);
      } else {
      /* Use a macro for shared regs because any shared reg writes need to
   * be wrapped in a getone block to work correctly. Writing shared regs
   * with multiple threads active does not work, even if they all return
   * the same value.
   */
   unsigned opc =
         struct ir3_instruction *swz = ir3_instr_create(instr->block, opc, 2, 2);
   ir3_dst_create(swz, dst_num, entry->flags);
   ir3_dst_create(swz, src_num, entry->flags);
   ir3_src_create(swz, src_num, entry->flags);
   ir3_src_create(swz, dst_num, entry->flags);
   swz->cat1.dst_type = (entry->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
   swz->cat1.src_type = (entry->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
   swz->repeat = 1;
         }
      static void
   do_copy(struct ir3_compiler *compiler, struct ir3_instruction *instr,
         {
      if (entry->flags & IR3_REG_HALF) {
      /* See do_swap() for why this is here. */
   if (entry->dst >= RA_HALF_SIZE) {
                     do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = entry->dst & ~1u},
               /* Similar to in do_swap(), account for src being swapped with tmp if
   * src and dst are in the same register.
   */
   struct copy_src src = entry->src;
                  do_copy(compiler, instr,
         &(struct copy_entry){
      .src = src,
               do_swap(compiler, instr,
         &(struct copy_entry){
      .src = {.reg = entry->dst & ~1u},
   .dst = tmp,
                  if (!entry->src.flags && entry->src.reg >= RA_HALF_SIZE) {
      unsigned src_num = ra_physreg_to_num(entry->src.reg & ~1u,
                  if (entry->src.reg % 2 == 0) {
      /* cov.u32u16 dst, src */
   struct ir3_instruction *cov =
         ir3_dst_create(cov, dst_num, entry->flags);
   ir3_src_create(cov, src_num, entry->flags & ~IR3_REG_HALF);
   cov->cat1.dst_type = TYPE_U16;
   cov->cat1.src_type = TYPE_U32;
      } else {
      /* shr.b dst, src, (16) */
   struct ir3_instruction *shr =
         ir3_dst_create(shr, dst_num, entry->flags);
   ir3_src_create(shr, src_num, entry->flags & ~IR3_REG_HALF);
   ir3_src_create(shr, 0, IR3_REG_IMMED)->uim_val = 16;
      }
                  unsigned src_num = ra_physreg_to_num(entry->src.reg, entry->flags);
            /* Similar to the swap case, we have to use a macro for shared regs. */
   unsigned opc =
         struct ir3_instruction *mov = ir3_instr_create(instr->block, opc, 1, 1);
   ir3_dst_create(mov, dst_num, entry->flags);
   ir3_src_create(mov, src_num, entry->flags | entry->src.flags);
   mov->cat1.dst_type = (entry->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
   mov->cat1.src_type = (entry->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
   if (entry->src.flags & IR3_REG_IMMED)
         else if (entry->src.flags & IR3_REG_CONST)
            }
      struct copy_ctx {
      /* For each physreg, the number of pending copy entries that use it as a
   * source. Once this drops to zero, then the physreg is unblocked and can
   * be moved to.
   */
            /* For each physreg, the pending copy_entry that uses it as a dest. */
            struct copy_entry entries[RA_MAX_FILE_SIZE];
      };
      static bool
   entry_blocked(struct copy_entry *entry, struct copy_ctx *ctx)
   {
      for (unsigned i = 0; i < copy_entry_size(entry); i++) {
      if (ctx->physreg_use_count[entry->dst + i] != 0)
                  }
      static void
   split_32bit_copy(struct copy_ctx *ctx, struct copy_entry *entry)
   {
      assert(!entry->done);
   assert(!(entry->src.flags & (IR3_REG_IMMED | IR3_REG_CONST)));
   assert(copy_entry_size(entry) == 2);
            new_entry->dst = entry->dst + 1;
   new_entry->src.flags = entry->src.flags;
   new_entry->src.reg = entry->src.reg + 1;
   new_entry->done = false;
   entry->flags |= IR3_REG_HALF;
   new_entry->flags = entry->flags;
      }
      static void
   _handle_copies(struct ir3_compiler *compiler, struct ir3_instruction *instr,
         {
      /* Set up the bookkeeping */
   memset(ctx->physreg_dst, 0, sizeof(ctx->physreg_dst));
            for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct copy_entry *entry = &ctx->entries[i];
   for (unsigned j = 0; j < copy_entry_size(entry); j++) {
                     /* Copies should not have overlapping destinations. */
   assert(!ctx->physreg_dst[entry->dst + j]);
                  bool progress = true;
   while (progress) {
               /* Step 1: resolve paths in the transfer graph. This means finding
   * copies whose destination aren't blocked by something else and then
   * emitting them, continuing this process until every copy is blocked
   * and there are only cycles left.
   *
   * TODO: We should note that src is also available in dst to unblock
   * cycles that src is involved in.
            for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct copy_entry *entry = &ctx->entries[i];
   if (!entry->done && !entry_blocked(entry, ctx)) {
      entry->done = true;
   progress = true;
   do_copy(compiler, instr, entry);
   for (unsigned j = 0; j < copy_entry_size(entry); j++) {
      if (!entry->src.flags)
                           if (progress)
            /* Step 2: Find partially blocked copies and split them. In the
   * mergedregs case, we can 32-bit copies which are only blocked on one
   * 16-bit half, and splitting them helps get things moving.
   *
   * We can skip splitting copies if the source isn't a register,
   * however, because it does not unblock anything and therefore doesn't
   * contribute to making forward progress with step 1. These copies
   * should still be resolved eventually in step 1 because they can't be
   * part of a cycle.
   */
   for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct copy_entry *entry = &ctx->entries[i];
                  if (((ctx->physreg_use_count[entry->dst] == 0 ||
            !(entry->src.flags & (IR3_REG_IMMED | IR3_REG_CONST))) {
   split_32bit_copy(ctx, entry);
                     /* Step 3: resolve cycles through swapping.
   *
   * At this point, the transfer graph should consist of only cycles.
   * The reason is that, given any physreg n_1 that's the source of a
   * remaining entry, it has a destination n_2, which (because every
   * copy is blocked) is the source of some other copy whose destination
   * is n_3, and so we can follow the chain until we get a cycle. If we
   * reached some other node than n_1:
   *
   *  n_1 -> n_2 -> ... -> n_i
   *          ^             |
   *          |-------------|
   *
   *  then n_2 would be the destination of 2 copies, which is illegal
   *  (checked above in an assert). So n_1 must be part of a cycle:
   *
   *  n_1 -> n_2 -> ... -> n_i
   *  ^                     |
   *  |---------------------|
   *
   *  and this must be only cycle n_1 is involved in, because any other
   *  path starting from n_1 would also have to end in n_1, resulting in
   *  a node somewhere along the way being the destination of 2 copies
   *  when the 2 paths merge.
   *
   *  The way we resolve the cycle is through picking a copy (n_1, n_2)
   *  and swapping n_1 and n_2. This moves n_1 to n_2, so n_2 is taken
   *  out of the cycle:
   *
   *  n_1 -> ... -> n_i
   *  ^              |
   *  |--------------|
   *
   *  and we can keep repeating this until the cycle is empty.
            for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct copy_entry *entry = &ctx->entries[i];
   if (entry->done)
                     /* catch trivial copies */
   if (entry->dst == entry->src.reg) {
      entry->done = true;
                        /* Split any blocking copies whose sources are only partially
   * contained within our destination.
   */
   if (entry->flags & IR3_REG_HALF) {
                                       if (blocking->src.reg <= entry->dst &&
      blocking->src.reg + 1 >= entry->dst &&
   !(blocking->flags & IR3_REG_HALF)) {
                     /* Update sources of blocking copies.
   *
   * Note: at this point, every blocking copy's source should be
   * contained within our destination.
   */
   for (unsigned j = 0; j < ctx->entry_count; j++) {
      struct copy_entry *blocking = &ctx->entries[j];
   if (blocking->src.reg >= entry->dst &&
      blocking->src.reg < entry->dst + copy_entry_size(entry)) {
   blocking->src.reg =
                        }
      static void
   handle_copies(struct ir3_shader_variant *v, struct ir3_instruction *instr,
         {
               /* handle shared copies first */
   ctx.entry_count = 0;
   for (unsigned i = 0; i < entry_count; i++) {
      if (entries[i].flags & IR3_REG_SHARED)
      }
            if (v->mergedregs) {
      /* Half regs and full regs are in the same file, so handle everything
   * at once.
   */
   ctx.entry_count = 0;
   for (unsigned i = 0; i < entry_count; i++) {
      if (!(entries[i].flags & IR3_REG_SHARED))
      }
      } else {
      /* There may be both half copies and full copies, so we have to split
   * them up since they don't interfere.
   */
   ctx.entry_count = 0;
   for (unsigned i = 0; i < entry_count; i++) {
      if (entries[i].flags & IR3_REG_HALF)
      }
            ctx.entry_count = 0;
   for (unsigned i = 0; i < entry_count; i++) {
      if (!(entries[i].flags & (IR3_REG_HALF | IR3_REG_SHARED)))
      }
         }
      void
   ir3_lower_copies(struct ir3_shader_variant *v)
   {
      DECLARE_ARRAY(struct copy_entry, copies);
   copies_count = copies_sz = 0;
            foreach_block (block, &v->ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->opc == OPC_META_PARALLEL_COPY) {
      copies_count = 0;
   for (unsigned i = 0; i < instr->dsts_count; i++) {
      struct ir3_register *dst = instr->dsts[i];
   struct ir3_register *src = instr->srcs[i];
   unsigned flags = src->flags & (IR3_REG_HALF | IR3_REG_SHARED);
   unsigned dst_physreg = ra_reg_get_physreg(dst);
   for (unsigned j = 0; j < reg_elems(dst); j++) {
      array_insert(
      NULL, copies,
   (struct copy_entry){
      .dst = dst_physreg + j * reg_elem_size(dst),
   .src = get_copy_src(src, j * reg_elem_size(dst)),
         }
   handle_copies(v, instr, copies, copies_count);
      } else if (instr->opc == OPC_META_COLLECT) {
      copies_count = 0;
   struct ir3_register *dst = instr->dsts[0];
   unsigned flags = dst->flags & (IR3_REG_HALF | IR3_REG_SHARED);
   for (unsigned i = 0; i < instr->srcs_count; i++) {
      struct ir3_register *src = instr->srcs[i];
   array_insert(NULL, copies,
               (struct copy_entry){
      .dst = ra_num_to_physreg(dst->num + i, flags),
   }
   handle_copies(v, instr, copies, copies_count);
      } else if (instr->opc == OPC_META_SPLIT) {
      copies_count = 0;
   struct ir3_register *dst = instr->dsts[0];
   struct ir3_register *src = instr->srcs[0];
   unsigned flags = src->flags & (IR3_REG_HALF | IR3_REG_SHARED);
   array_insert(NULL, copies,
               (struct copy_entry){
      .dst = ra_reg_get_physreg(dst),
   .src = get_copy_src(
      handle_copies(v, instr, copies, copies_count);
      } else if (instr->opc == OPC_META_PHI) {
                        if (copies)
      }
