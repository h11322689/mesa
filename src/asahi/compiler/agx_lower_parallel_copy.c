   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2021 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "agx_builder.h"
   #include "agx_compiler.h"
      /*
   * Emits code for
   *
   *    for (int i = 0; i < n; ++i)
   *       registers[dests[i]] = registers[srcs[i]];
   *
   * ...with all copies happening in parallel.
   *
   * That is, emit machine instructions equivalent to a parallel copy. This is
   * used to lower not only parallel copies but also collects and splits, which
   * also have parallel copy semantics.
   *
   * We only handles register-register copies, not general agx_index sources. This
   * suffices for its internal use for register allocation.
   */
   static void
   do_copy(agx_builder *b, const struct agx_copy *copy)
   {
         }
      static void
   do_swap(agx_builder *b, const struct agx_copy *copy)
   {
               if (copy->dest == copy->src.value)
            /* We can swap lo/hi halves of a 32-bit register with a 32-bit extr */
   if (copy->src.size == AGX_SIZE_16 &&
               assert(((copy->dest & 1) == (1 - (copy->src.value & 1))) &&
            /* r0 = extr r0, r0, #16
   *    = (((r0 << 32) | r0) >> 16) & 0xFFFFFFFF
   *    = (((r0 << 32) >> 16) & 0xFFFFFFFF) | (r0 >> 16)
   *    = (r0l << 16) | r0h
   */
   agx_index reg32 = agx_register(copy->dest & ~1, AGX_SIZE_32);
   agx_extr_to(b, reg32, reg32, reg32, agx_immediate(16), 0);
               agx_index x = agx_register(copy->dest, copy->src.size);
            agx_xor_to(b, x, x, y);
   agx_xor_to(b, y, x, y);
      }
      struct copy_ctx {
      /* Number of copies being processed */
            /* For each physreg, the number of pending copy entries that use it as a
   * source. Once this drops to zero, then the physreg is unblocked and can
   * be moved to.
   */
            /* For each physreg, the pending copy_entry that uses it as a dest. */
               };
      static bool
   entry_blocked(struct agx_copy *entry, struct copy_ctx *ctx)
   {
      for (unsigned i = 0; i < agx_size_align_16(entry->src.size); i++) {
      if (ctx->physreg_use_count[entry->dest + i] != 0)
                  }
      static bool
   is_real(struct agx_copy *entry)
   {
         }
      /* TODO: Generalize to other bit sizes */
   static void
   split_32bit_copy(struct copy_ctx *ctx, struct agx_copy *entry)
   {
      assert(!entry->done);
   assert(is_real(entry));
   assert(agx_size_align_16(entry->src.size) == 2);
            new_entry->dest = entry->dest + 1;
   new_entry->src = entry->src;
   new_entry->src.value += 1;
   new_entry->done = false;
   entry->src.size = AGX_SIZE_16;
   new_entry->src.size = AGX_SIZE_16;
      }
      void
   agx_emit_parallel_copies(agx_builder *b, struct agx_copy *copies,
         {
      /* First, lower away 64-bit copies to smaller chunks, since we don't have
   * 64-bit ALU so we always want to split.
   */
   struct agx_copy *copies2 = calloc(sizeof(copies[0]), num_copies * 2);
            for (unsigned i = 0; i < num_copies; ++i) {
               if (copy.src.size == AGX_SIZE_64) {
                     copy.src.value += 2;
   copy.dest += 2;
      } else {
                     copies = copies2;
            /* Set up the bookkeeping */
   struct copy_ctx _ctx = {.entry_count = num_copies};
            memset(ctx->physreg_dest, 0, sizeof(ctx->physreg_dest));
            for (unsigned i = 0; i < ctx->entry_count; i++) {
                        for (unsigned j = 0; j < agx_size_align_16(entry->src.size); j++) {
                     /* Copies should not have overlapping destinations. */
   assert(!ctx->physreg_dest[entry->dest + j]);
                  /* Try to vectorize aligned 16-bit copies to use 32-bit operations instead */
   for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct agx_copy *entry = &ctx->entries[i];
   if (entry->src.size != AGX_SIZE_16)
            if ((entry->dest & 1) || (entry->src.value & 1))
            if (entry->src.type != AGX_INDEX_UNIFORM &&
                  unsigned next_dest = entry->dest + 1;
            struct agx_copy *next_copy = ctx->physreg_dest[next_dest];
   if (!next_copy)
            assert(next_copy->dest == next_dest && "data structure invariant");
            if (next_copy->src.type != entry->src.type)
            if (next_copy->src.value != (entry->src.value + 1))
            /* Vectorize the copies */
   ctx->physreg_dest[next_dest] = entry;
   entry->src.size = AGX_SIZE_32;
               bool progress = true;
   while (progress) {
               /* Step 1: resolve paths in the transfer graph. This means finding
   * copies whose destination aren't blocked by something else and then
   * emitting them, continuing this process until every copy is blocked
   * and there are only cycles left.
   *
   * TODO: We should note that src is also available in dest to unblock
   * cycles that src is involved in.
            for (unsigned i = 0; i < ctx->entry_count; i++) {
      struct agx_copy *entry = &ctx->entries[i];
   if (!entry->done && !entry_blocked(entry, ctx)) {
      entry->done = true;
   progress = true;
   do_copy(b, entry);
   for (unsigned j = 0; j < agx_size_align_16(entry->src.size); j++) {
      if (is_real(entry))
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
      struct agx_copy *entry = &ctx->entries[i];
                  if (((ctx->physreg_use_count[entry->dest] == 0 ||
            is_real(entry)) {
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
      struct agx_copy *entry = &ctx->entries[i];
   if (entry->done)
                     /* catch trivial copies */
   if (entry->dest == entry->src.value) {
      entry->done = true;
                        /* Split any blocking copies whose sources are only partially
   * contained within our destination.
   */
   if (agx_size_align_16(entry->src.size) == 1) {
                                       if (blocking->src.value <= entry->dest &&
      blocking->src.value + 1 >= entry->dest &&
   agx_size_align_16(blocking->src.size) == 2) {
                     /* Update sources of blocking copies.
   *
   * Note: at this point, every blocking copy's source should be
   * contained within our destination.
   */
   for (unsigned j = 0; j < ctx->entry_count; j++) {
      struct agx_copy *blocking = &ctx->entries[j];
   if (blocking->src.value >= entry->dest &&
      blocking->src.value <
         blocking->src.value =
                                 }
