   /*
   * Copyright 2021 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "util/u_dynarray.h"
   #include "util/u_qsort.h"
   #include "agx_builder.h"
   #include "agx_compiler.h"
   #include "agx_debug.h"
   #include "agx_opcodes.h"
      /* SSA-based register allocator */
      struct ra_ctx {
      agx_context *shader;
   agx_block *block;
   agx_instr *instr;
   uint8_t *ssa_to_reg;
   uint8_t *ncomps;
   enum agx_size *sizes;
   BITSET_WORD *visited;
            /* Maintained while assigning registers */
            /* For affinities */
            /* If bit i of used_regs is set, and register i is the first consecutive
   * register holding an SSA value, then reg_to_ssa[i] is the SSA index of the
   * value currently in register  i.
   */
            /* Maximum number of registers that RA is allowed to use */
      };
      /** Returns number of registers written by an instruction */
   unsigned
   agx_write_registers(const agx_instr *I, unsigned d)
   {
               switch (I->op) {
   case AGX_OPCODE_ITER:
   case AGX_OPCODE_ITERPROJ:
      assert(1 <= I->channels && I->channels <= 4);
         case AGX_OPCODE_IMAGE_LOAD:
   case AGX_OPCODE_TEXTURE_LOAD:
   case AGX_OPCODE_TEXTURE_SAMPLE:
      /* Even when masked out, these clobber 4 registers */
         case AGX_OPCODE_DEVICE_LOAD:
   case AGX_OPCODE_LOCAL_LOAD:
   case AGX_OPCODE_LD_TILE:
      /* Can write 16-bit or 32-bit. Anything logically 64-bit is already
   * expanded to 32-bit in the mask.
   */
         case AGX_OPCODE_LDCF:
         case AGX_OPCODE_COLLECT:
         default:
            }
      static inline enum agx_size
   agx_split_width(const agx_instr *I)
   {
               agx_foreach_dest(I, d) {
      if (I->dest[d].type == AGX_INDEX_NULL)
         else if (width != ~0)
         else
               assert(width != ~0 && "should have been DCE'd");
      }
      /*
   * Return number of registers required for coordinates for a
   * texture/image instruction. We handle layer + sample index as 32-bit even when
   * only the lower 16-bits are present.
   */
   static unsigned
   agx_coordinate_registers(const agx_instr *I)
   {
      switch (I->dim) {
   case AGX_DIM_1D:
         case AGX_DIM_1D_ARRAY:
         case AGX_DIM_2D:
         case AGX_DIM_2D_ARRAY:
         case AGX_DIM_2D_MS:
         case AGX_DIM_3D:
         case AGX_DIM_CUBE:
         case AGX_DIM_CUBE_ARRAY:
         case AGX_DIM_2D_MS_ARRAY:
                     }
      /*
   * Calculate register demand in 16-bit registers. Becuase we allocate in SSA,
   * this calculation is exact in linear-time. Depends on liveness information.
   */
   static unsigned
   agx_calc_register_demand(agx_context *ctx, uint8_t *widths)
   {
      /* Calculate demand at the start of each block based on live-in, then update
   * for each instruction processed. Calculate rolling maximum.
   */
            agx_foreach_block(ctx, block) {
               /* RA treats the nesting counter as alive throughout if control flow is
   * used anywhere. This could be optimized.
   */
   if (ctx->any_cf)
            /* Everything live-in */
   {
      int i;
   BITSET_FOREACH_SET(i, block->live_in, ctx->alloc) {
                              /* To handle non-power-of-two vectors, sometimes live range splitting
   * needs extra registers for 1 instruction. This counter tracks the number
   * of registers to be freed after 1 extra instruction.
   */
            agx_foreach_instr_in_block(block, I) {
      /* Phis happen in parallel and are already accounted for in the live-in
   * set, just skip them so we don't double count.
   */
                  /* Handle late-kill registers from last instruction */
                  /* Kill sources the first time we see them */
   agx_foreach_src(I, s) {
      if (!I->src[s].kill)
                           for (unsigned backwards = 0; backwards < s; ++backwards) {
      if (agx_is_equiv(I->src[backwards], I->src[s])) {
      skip = true;
                  if (!skip)
               /* Make destinations live */
   agx_foreach_dest(I, d) {
      if (agx_is_null(I->dest[d]))
                  /* Live range splits allocate at power-of-two granularity. Round up
   * destination sizes (temporarily) to powers-of-two.
   */
                  demand += pot_width;
                                          }
      unsigned
   agx_read_registers(const agx_instr *I, unsigned s)
   {
               switch (I->op) {
   case AGX_OPCODE_SPLIT:
            case AGX_OPCODE_DEVICE_STORE:
   case AGX_OPCODE_LOCAL_STORE:
   case AGX_OPCODE_ST_TILE:
      /* See agx_write_registers */
   if (s == 0)
         else
         case AGX_OPCODE_ZS_EMIT:
      if (s == 1) {
      /* Depth (bit 0) is fp32, stencil (bit 1) is u16 in the hw but we pad
   * up to u32 for simplicity
   */
   bool z = !!(I->zs & 1);
                     } else {
               case AGX_OPCODE_IMAGE_WRITE:
      if (s == 0)
         else if (s == 1)
         else
         case AGX_OPCODE_IMAGE_LOAD:
   case AGX_OPCODE_TEXTURE_LOAD:
   case AGX_OPCODE_TEXTURE_SAMPLE:
      if (s == 0) {
         } else if (s == 1) {
      /* LOD */
   if (I->lod_mode == AGX_LOD_MODE_LOD_GRAD) {
      switch (I->dim) {
   case AGX_DIM_1D:
   case AGX_DIM_1D_ARRAY:
         case AGX_DIM_2D:
   case AGX_DIM_2D_ARRAY:
   case AGX_DIM_2D_MS_ARRAY:
   case AGX_DIM_2D_MS:
         case AGX_DIM_CUBE:
   case AGX_DIM_CUBE_ARRAY:
   case AGX_DIM_3D:
                     } else {
            } else if (s == 5) {
      /* Compare/offset */
      } else {
               case AGX_OPCODE_ATOMIC:
   case AGX_OPCODE_LOCAL_ATOMIC:
      if (s == 0 && I->atomic_opc == AGX_ATOMIC_OPC_CMPXCHG)
         else
         default:
            }
      static bool
   find_regs_simple(struct ra_ctx *rctx, unsigned count, unsigned align,
         {
      for (unsigned reg = 0; reg + count <= rctx->bound; reg += align) {
      if (!BITSET_TEST_RANGE(rctx->used_regs, reg, reg + count - 1)) {
      *out = reg;
                     }
      /*
   * Search the register file for the best contiguous aligned region of the given
   * size to evict when shuffling registers. The region must not contain any
   * register marked in the passed bitset.
   *
   * As a hint, this also takes in the set of registers from killed sources passed
   * to this instruction. These should be deprioritized, since they are more
   * expensive to use (extra moves to shuffle the contents away).
   *
   * Precondition: such a region exists.
   *
   * Postcondition: at least one register in the returned region is already free.
   */
   static unsigned
   find_best_region_to_evict(struct ra_ctx *rctx, unsigned size,
         {
      assert(util_is_power_of_two_or_zero(size) && "precondition");
   assert((rctx->bound % size) == 0 &&
            unsigned best_base = ~0;
            for (unsigned base = 0; base + size <= rctx->bound; base += size) {
      /* r0l is unevictable, skip it. By itself, this does not pose a problem.
   * We are allocating n registers, but the region containing r0l has at
   * most n-1 free. Since there are at least n free registers total, there
   * is at least 1 free register outside this region. Thus the region
   * containing that free register contains at most n-1 occupied registers.
   * In the worst case, those n-1 occupied registers are moved to the region
   * with r0l and then the n free registers are used for the destination.
   * Thus, we do not need extra registers to handle "single point"
   * unevictability.
   */
   if (base == 0 && rctx->shader->any_cf)
            /* Do not evict the same register multiple times. It's not necessary since
   * we're just shuffling, there are enough free registers elsewhere.
   */
   if (BITSET_TEST_RANGE(already_evicted, base, base + size - 1))
            /* Estimate the number of moves required if we pick this region */
   unsigned moves = 0;
            for (unsigned reg = base; reg < base + size; ++reg) {
      /* We need a move for each blocked register (TODO: we only need a
   * single move for 32-bit pairs, could optimize to use that instead.)
   */
   if (BITSET_TEST(rctx->used_regs, reg))
                        /* Each clobbered killed register requires a move or a swap. Since
   * swaps require more instructions, assign a higher cost here. In
   * practice, 3 is too high but 2 is slightly better than 1.
   */
   if (BITSET_TEST(killed, reg))
               /* Pick the region requiring fewest moves as a heuristic. Regions with no
   * free registers are skipped even if the heuristic estimates a lower cost
   * (due to killed sources), since the recursive splitting algorithm
   * requires at least one free register.
   */
   if (any_free && moves < best_moves) {
      best_moves = moves;
                  assert(best_base < rctx->bound &&
            }
      static void
   set_ssa_to_reg(struct ra_ctx *rctx, unsigned ssa, unsigned reg)
   {
      *(rctx->max_reg) = MAX2(*(rctx->max_reg), reg + rctx->ncomps[ssa] - 1);
      }
      static unsigned
   assign_regs_by_copying(struct ra_ctx *rctx, unsigned npot_count, unsigned align,
               {
      /* XXX: This needs some special handling but so far it has been prohibitively
   * difficult to hit the case
   */
   if (I->op == AGX_OPCODE_PHI)
            /* Expand the destination to the next power-of-two size. This simplifies
   * splitting and is accounted for by the demand calculation, so is legal.
   */
   unsigned count = util_next_power_of_two(npot_count);
   assert(align <= count && "still aligned");
            /* There's not enough contiguous room in the register file. We need to
   * shuffle some variables around. Look for a range of the register file
   * that is partially blocked.
   */
            assert(count <= 16 && "max allocation size (conservative)");
            /* Store the set of blocking registers that need to be evicted */
   for (unsigned i = 0; i < count; ++i) {
      if (BITSET_TEST(rctx->used_regs, base + i)) {
                     /* We are going to allocate the destination to this range, so it is now fully
   * used. Mark it as such so we don't reassign here later.
   */
            /* Before overwriting the range, we need to evict blocked variables */
   for (unsigned i = 0; i < 16; ++i) {
      /* Look for subranges that needs eviction */
   if (!BITSET_TEST(evict_set, i))
            unsigned reg = base + i;
   uint32_t ssa = rctx->reg_to_ssa[reg];
   uint32_t nr = rctx->ncomps[ssa];
            assert(nr >= 1 && "must be assigned");
   assert(rctx->ssa_to_reg[ssa] == reg &&
            for (unsigned j = 0; j < nr; ++j) {
      assert(BITSET_TEST(evict_set, i + j) &&
                     /* Assign a new location for the variable. This terminates with finite
   * recursion because nr is decreasing because of the gap.
   */
   assert(nr < count && "fully contained in range that's not full");
   unsigned new_reg =
            /* Copy the variable over, register by register */
   for (unsigned i = 0; i < nr; i += align) {
      struct agx_copy copy = {
      .dest = new_reg + i,
               assert((copy.dest % agx_size_align_16(rctx->sizes[ssa])) == 0 &&
         assert((copy.src.value % agx_size_align_16(rctx->sizes[ssa])) == 0 &&
                     /* Mark down the set of clobbered registers, so that killed sources may be
   * handled correctly later.
   */
            /* Update bookkeeping for this variable */
   set_ssa_to_reg(rctx, ssa, new_reg);
            /* Skip to the next variable */
               /* We overallocated for non-power-of-two vectors. Free up the excess now.
   * This is modelled as late kill in demand calculation.
   */
   if (npot_count != count)
               }
      static int
   sort_by_size(const void *a_, const void *b_, void *sizes_)
   {
      const enum agx_size *sizes = sizes_;
               }
      /*
   * Allocating a destination of n consecutive registers may require moving those
   * registers' contents to the locations of killed sources. For the instruction
   * to read the correct values, the killed sources themselves need to be moved to
   * the space where the destination will go.
   *
   * This is legal because there is no interference between the killed source and
   * the destination. This is always possible because, after this insertion, the
   * destination needs to contain the killed sources already overlapping with the
   * destination (size k) plus the killed sources clobbered to make room for
   * livethrough sources overlapping with the destination (at most size |dest|-k),
   * so the total size is at most k + |dest| - k = |dest| and so fits in the dest.
   * Sorting by alignment may be necessary.
   */
   static void
   insert_copies_for_clobbered_killed(struct ra_ctx *rctx, unsigned reg,
                     {
      unsigned vars[16] = {0};
            /* Precondition: the nesting counter is not overwritten. Therefore we do not
   * have to move it.  find_best_region_to_evict knows better than to try.
   */
            /* Consider the destination clobbered for the purpose of source collection.
   * This way, killed sources already in the destination will be preserved
   * (though possibly compacted).
   */
            /* Collect killed clobbered sources, if any */
   agx_foreach_ssa_src(I, s) {
               if (I->src[s].kill && BITSET_TEST(clobbered, reg)) {
                                    if (nr_vars == 0)
            /* Sort by descending alignment so they are packed with natural alignment */
            /* Reassign in the destination region */
            /* We align vectors to their sizes, so this assertion holds as long as no
   * instruction has a source whose scalar size is greater than the entire size
   * of the vector destination. Yet the killed source must fit within this
   * destination, so the destination must be bigger and therefore have bigger
   * alignment.
   */
   assert((base % agx_size_align_16(rctx->sizes[vars[0]])) == 0 &&
            for (unsigned i = 0; i < nr_vars; ++i) {
      unsigned var = vars[i];
   unsigned var_base = rctx->ssa_to_reg[var];
   unsigned var_count = rctx->ncomps[var];
            assert((base % var_align) == 0 && "induction");
            for (unsigned j = 0; j < var_count; j += var_align) {
      struct agx_copy copy = {
      .dest = base + j,
                           set_ssa_to_reg(rctx, var, base);
                           }
      static unsigned
   find_regs(struct ra_ctx *rctx, agx_instr *I, unsigned dest_idx, unsigned count,
         {
      unsigned reg;
            if (find_regs_simple(rctx, count, align, &reg)) {
         } else {
      BITSET_DECLARE(clobbered, AGX_NUM_REGS) = {0};
   BITSET_DECLARE(killed, AGX_NUM_REGS) = {0};
   struct util_dynarray copies = {0};
            /* Initialize the set of registers killed by this instructions' sources */
   agx_foreach_ssa_src(I, s) {
               if (BITSET_TEST(rctx->visited, v)) {
      unsigned base = rctx->ssa_to_reg[v];
   unsigned nr = rctx->ncomps[v];
                  reg = assign_regs_by_copying(rctx, count, align, I, &copies, clobbered,
         insert_copies_for_clobbered_killed(rctx, reg, count, I, &copies,
            /* Insert the necessary copies */
   agx_builder b = agx_init_builder(rctx->shader, agx_before_instr(I));
   agx_emit_parallel_copies(
            /* assign_regs asserts this is cleared, so clear to be reassigned */
   BITSET_CLEAR_RANGE(rctx->used_regs, reg, reg + count - 1);
         }
      /*
   * Loop over live-in values at the start of the block and mark their registers
   * as in-use. We process blocks in dominance order, so this handles everything
   * but loop headers.
   *
   * For loop headers, this handles the forward edges but not the back edge.
   * However, that's okay: we don't want to reserve the registers that are
   * defined within the loop, because then we'd get a contradiction. Instead we
   * leave them available and then they become fixed points of a sort.
   */
   static void
   reserve_live_in(struct ra_ctx *rctx)
   {
      /* If there are no predecessors, there is nothing live-in */
   unsigned nr_preds = agx_num_predecessors(rctx->block);
   if (nr_preds == 0)
            agx_builder b =
            int i;
   BITSET_FOREACH_SET(i, rctx->block->live_in, rctx->shader->alloc) {
      /* Skip values defined in loops when processing the loop header */
   if (!BITSET_TEST(rctx->visited, i))
                     /* If we split live ranges, the variable might be defined differently at
   * the end of each predecessor. Join them together with a phi inserted at
   * the start of the block.
   */
   if (nr_preds > 1) {
      /* We'll fill in the destination after, to coalesce one of the moves */
                                    if ((*pred)->ssa_to_reg_out == NULL) {
      /* If this is a loop header, we don't know where the register
   * will end up. So, we create a phi conservatively but don't fill
   * it in until the end of the loop. Stash in the information
   * we'll need to fill in the real register later.
   */
   assert(rctx->block->loop_header);
      } else {
      /* Otherwise, we can build the phi now */
   unsigned reg = (*pred)->ssa_to_reg_out[i];
                  /* Pick the phi destination to coalesce a move. Predecessor ordering is
   * stable, so this means all live-in values get their registers from a
   * particular predecessor. That means that such a register allocation
   * is valid here, because it was valid in the predecessor.
   */
   phi->dest[0] = phi->src[0];
      } else {
                     agx_block **pred = util_dynarray_begin(&rctx->block->predecessors);
                        for (unsigned j = 0; j < rctx->ncomps[i]; ++j) {
      BITSET_SET(rctx->used_regs, base + j);
            }
      static void
   assign_regs(struct ra_ctx *rctx, agx_index v, unsigned reg)
   {
      assert(reg < rctx->bound && "must not overflow register file");
   assert(v.type == AGX_INDEX_NORMAL && "only SSA gets registers allocated");
            assert(!BITSET_TEST(rctx->visited, v.value) && "SSA violated");
            assert(rctx->ncomps[v.value] >= 1);
            assert(!BITSET_TEST_RANGE(rctx->used_regs, reg, end) && "no interference");
               }
      static void
   agx_set_sources(struct ra_ctx *rctx, agx_instr *I)
   {
               agx_foreach_ssa_src(I, s) {
               unsigned v = rctx->ssa_to_reg[I->src[s].value];
         }
      static void
   agx_set_dests(struct ra_ctx *rctx, agx_instr *I)
   {
      agx_foreach_ssa_dest(I, s) {
      unsigned v = rctx->ssa_to_reg[I->dest[s].value];
   I->dest[s] =
         }
      static unsigned
   affinity_base_of_collect(struct ra_ctx *rctx, agx_instr *collect, unsigned src)
   {
      unsigned src_reg = rctx->ssa_to_reg[collect->src[src].value];
            if (src_reg >= src_offset)
         else
      }
      static bool
   try_coalesce_with(struct ra_ctx *rctx, agx_index ssa, unsigned count,
         {
      assert(ssa.type == AGX_INDEX_NORMAL);
   if (!BITSET_TEST(rctx->visited, ssa.value)) {
      assert(may_be_unvisited);
               unsigned base = rctx->ssa_to_reg[ssa.value];
   if (BITSET_TEST_RANGE(rctx->used_regs, base, base + count - 1))
            assert(base + count <= rctx->bound && "invariant");
   *out = base;
      }
      static unsigned
   pick_regs(struct ra_ctx *rctx, agx_instr *I, unsigned d)
   {
      agx_index idx = I->dest[d];
            unsigned count = rctx->ncomps[idx.value];
                     /* Try to allocate phis compatibly with their sources */
   if (I->op == AGX_OPCODE_PHI) {
      agx_foreach_ssa_src(I, s) {
                     unsigned out;
   if (try_coalesce_with(rctx, I->src[s], count, may_be_unvisited, &out))
                  /* Try to allocate collects compatibly with their sources */
   if (I->op == AGX_OPCODE_COLLECT) {
      agx_foreach_ssa_src(I, s) {
      assert(BITSET_TEST(rctx->visited, I->src[s].value) &&
                  unsigned base = affinity_base_of_collect(rctx, I, s);
                  /* Unaligned destinations can happen when dest size > src size */
                  if (!BITSET_TEST_RANGE(rctx->used_regs, base, base + count - 1))
                  /* Try to allocate sources of collects contiguously */
   agx_instr *collect_phi = rctx->src_to_collect_phi[idx.value];
   if (collect_phi && collect_phi->op == AGX_OPCODE_COLLECT) {
                        /* Find our offset in the collect. If our source is repeated in the
   * collect, this may not be unique. We arbitrarily choose the first.
   */
   unsigned our_source = ~0;
   agx_foreach_ssa_src(collect, s) {
      if (agx_is_equiv(collect->src[s], idx)) {
      our_source = s;
                           /* See if we can allocate compatibly with any source of the collect */
   agx_foreach_ssa_src(collect, s) {
                     /* Determine where the collect should start relative to the source */
   unsigned base = affinity_base_of_collect(rctx, collect, s);
                           /* Don't allocate past the end of the register file */
                  /* If those registers are free, then choose them */
   if (!BITSET_TEST_RANGE(rctx->used_regs, our_reg, our_reg + align - 1))
               unsigned collect_align = rctx->ncomps[collect->dest[0].value];
            /* Prefer ranges of the register file that leave room for all sources of
   * the collect contiguously.
   */
   for (unsigned base = 0; base + (collect->nr_srcs * align) <= rctx->bound;
      base += collect_align) {
   if (!BITSET_TEST_RANGE(rctx->used_regs, base,
                     /* Try to respect the alignment requirement of the collect destination,
   * which may be greater than the sources (e.g. pack_64_2x32_split). Look
   * for a register for the source such that the collect base is aligned.
   */
   if (collect_align > align) {
      for (unsigned reg = offset; reg + collect_align <= rctx->bound;
      reg += collect_align) {
   if (!BITSET_TEST_RANGE(rctx->used_regs, reg, reg + count - 1))
                     /* Try to allocate phi sources compatibly with their phis */
   if (collect_phi && collect_phi->op == AGX_OPCODE_PHI) {
      agx_instr *phi = collect_phi;
            agx_foreach_ssa_src(phi, s) {
      if (try_coalesce_with(rctx, phi->src[s], count, true, &out))
               /* If we're in a loop, we may have already allocated the phi. Try that. */
   if (phi->dest[0].type == AGX_INDEX_REGISTER) {
               if (!BITSET_TEST_RANGE(rctx->used_regs, base, base + count - 1))
                  /* Default to any contiguous sequence of registers */
      }
      /** Assign registers to SSA values in a block. */
      static void
   agx_ra_assign_local(struct ra_ctx *rctx)
   {
      BITSET_DECLARE(used_regs, AGX_NUM_REGS) = {0};
            agx_block *block = rctx->block;
   uint8_t *ncomps = rctx->ncomps;
   rctx->used_regs = used_regs;
                     /* Force the nesting counter r0l live throughout shaders using control flow.
   * This could be optimized (sync with agx_calc_register_demand).
   */
   if (rctx->shader->any_cf)
            agx_foreach_instr_in_block(block, I) {
               /* Optimization: if a split contains the last use of a vector, the split
   * can be removed by assigning the destinations overlapping the source.
   */
   if (I->op == AGX_OPCODE_SPLIT && I->src[0].kill) {
                     agx_foreach_dest(I, d) {
      /* Free up the source */
                  /* Assign the destination where the source was */
   if (!agx_is_null(I->dest[d]))
               unsigned excess =
         if (excess) {
      BITSET_CLEAR_RANGE(used_regs, reg + (I->nr_dests * width),
               agx_set_sources(rctx, I);
   agx_set_dests(rctx, I);
      } else if (I->op == AGX_OPCODE_PRELOAD) {
      /* We must coalesce all preload moves */
                  assign_regs(rctx, I->dest[0], I->src[0].value);
   agx_set_dests(rctx, I);
               /* First, free killed sources */
   agx_foreach_ssa_src(I, s) {
      if (I->src[s].kill) {
                     assert(count >= 1);
                  /* Next, assign destinations one at a time. This is always legal
   * because of the SSA form.
   */
   agx_foreach_ssa_dest(I, d) {
                  /* Phi sources are special. Set in the corresponding predecessors */
   if (I->op != AGX_OPCODE_PHI)
                                 STATIC_ASSERT(sizeof(block->regs_out) == sizeof(used_regs));
            /* Also set the sources for the phis in our successors, since that logically
   * happens now (given the possibility of live range splits, etc)
   */
   agx_foreach_successor(block, succ) {
               agx_foreach_phi_in_block(succ, phi) {
      if (phi->src[pred_idx].type == AGX_INDEX_NORMAL) {
                     agx_replace_src(
      phi, pred_idx,
            }
      /*
   * Lower phis to parallel copies at the logical end of a given block. If a block
   * needs parallel copies inserted, a successor of the block has a phi node. To
   * have a (nontrivial) phi node, a block must have multiple predecessors. So the
   * edge from the block to the successor (with phi) is not the only edge entering
   * the successor. Because the control flow graph has no critical edges, this
   * edge must therefore be the only edge leaving the block, so the block must
   * have only a single successor.
   */
   static void
   agx_insert_parallel_copies(agx_context *ctx, agx_block *block)
   {
      bool any_succ = false;
            /* Phi nodes logically happen on the control flow edge, so parallel copies
   * are added at the end of the predecessor */
            agx_foreach_successor(block, succ) {
               agx_foreach_phi_in_block(succ, phi) {
      assert(!any_succ && "control flow graph has a critical edge");
                        /* Nothing to do if there are no phi nodes */
   if (nr_phi == 0)
                     /* Create a parallel copy lowering all the phi nodes */
                     agx_foreach_phi_in_block(succ, phi) {
                                    copies[i++] = (struct agx_copy){
      .dest = dest.value,
                                 }
      void
   agx_ra(agx_context *ctx)
   {
      agx_compute_liveness(ctx);
   uint8_t *ncomps = calloc(ctx->alloc, sizeof(uint8_t));
   agx_instr **src_to_collect_phi = calloc(ctx->alloc, sizeof(agx_instr *));
   enum agx_size *sizes = calloc(ctx->alloc, sizeof(enum agx_size));
   BITSET_WORD *visited = calloc(BITSET_WORDS(ctx->alloc), sizeof(BITSET_WORD));
            agx_foreach_instr_global(ctx, I) {
      /* Record collects/phis so we can coalesce when assigning */
   if (I->op == AGX_OPCODE_COLLECT || I->op == AGX_OPCODE_PHI) {
      agx_foreach_ssa_src(I, s) {
                     agx_foreach_ssa_dest(I, d) {
      unsigned v = I->dest[d].value;
   assert(ncomps[v] == 0 && "broken SSA");
   /* Round up vectors for easier live range splitting */
                                 /* For live range splitting to work properly, ensure the register file is
   * aligned to the larger vector size. Most of the time, this is a no-op since
   * the largest vector size is usually 128-bit and the register file is
   * naturally 128-bit aligned. However, this is required for correctness with
   * 3D textureGrad, which can have a source vector of length 6x32-bit,
   * rounding up to 256-bit and requiring special accounting here.
   */
   unsigned reg_file_alignment = MAX2(max_ncomps, 8);
            /* Calculate the demand and use it to bound register assignment */
   unsigned demand =
            /* Round up the demand to the maximum number of registers we can use without
   * affecting occupancy. This reduces live range splitting.
   */
   unsigned max_regs = agx_occupancy_for_register_count(demand).max_registers;
            /* Or, we can bound tightly for debugging */
   if (agx_compiler_debug & AGX_DBG_DEMAND)
            /* ...but not too tightly */
   assert((max_regs % reg_file_alignment) == 0 && "occupancy limits aligned");
            /* Assign registers in dominance-order. This coincides with source-order due
   * to a NIR invariant, so we do not need special handling for this.
   */
   agx_foreach_block(ctx, block) {
      agx_ra_assign_local(&(struct ra_ctx){
      .shader = ctx,
   .block = block,
   .src_to_collect_phi = src_to_collect_phi,
   .ncomps = ncomps,
   .sizes = sizes,
   .visited = visited,
   .bound = max_regs,
                  /* Vertex shaders preload the vertex/instance IDs (r5, r6) even if the shader
   * don't use them. Account for that so the preload doesn't clobber GPRs.
   */
   if (ctx->nir->info.stage == MESA_SHADER_VERTEX)
                     agx_foreach_instr_global_safe(ctx, ins) {
      /* Lower away RA pseudo-instructions */
            if (ins->op == AGX_OPCODE_COLLECT) {
      assert(ins->dest[0].type == AGX_INDEX_REGISTER);
                                 /* Move the sources */
   agx_foreach_src(ins, i) {
      if (agx_is_null(ins->src[i]) || ins->src[i].type == AGX_INDEX_UNDEF)
                  copies[n++] = (struct agx_copy){
      .dest = base + (i * width),
                  agx_emit_parallel_copies(&b, copies, n);
   agx_remove_instruction(ins);
      } else if (ins->op == AGX_OPCODE_SPLIT) {
                                                   /* Move the sources */
   agx_foreach_dest(ins, i) {
                     agx_index src = ins->src[0];
                  copies[n++] = (struct agx_copy){
      .dest = ins->dest[i].value,
                  /* Lower away */
   agx_builder b = agx_init_builder(ctx, agx_after_instr(ins));
   agx_emit_parallel_copies(&b, copies, n);
   agx_remove_instruction(ins);
                  /* Insert parallel copies lowering phi nodes */
   agx_foreach_block(ctx, block) {
                  agx_foreach_instr_global_safe(ctx, I) {
      switch (I->op) {
   /* Pseudoinstructions for RA must be removed now */
   case AGX_OPCODE_PHI:
   case AGX_OPCODE_PRELOAD:
                  /* Coalesced moves can be removed */
   case AGX_OPCODE_MOV:
      if (I->src[0].type == AGX_INDEX_REGISTER &&
                     assert(I->dest[0].type == AGX_INDEX_REGISTER);
                  default:
                     agx_foreach_block(ctx, block) {
      free(block->ssa_to_reg_out);
               free(src_to_collect_phi);
   free(ncomps);
   free(sizes);
      }
