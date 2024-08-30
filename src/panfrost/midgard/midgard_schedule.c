   /*
   * Copyright (C) 2018-2019 Alyssa Rosenzweig <alyssa@rosenzweig.io>
   * Copyright (C) 2019-2020 Collabora, Ltd.
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
      #include "util/half_float.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "compiler.h"
   #include "midgard_ops.h"
   #include "midgard_quirks.h"
      /* Scheduling for Midgard is complicated, to say the least. ALU instructions
   * must be grouped into VLIW bundles according to following model:
   *
   * [VMUL] [SADD]
   * [VADD] [SMUL] [VLUT]
   *
   * A given instruction can execute on some subset of the units (or a few can
   * execute on all). Instructions can be either vector or scalar; only scalar
   * instructions can execute on SADD/SMUL units. Units on a given line execute
   * in parallel. Subsequent lines execute separately and can pass results
   * directly via pipeline registers r24/r25, bypassing the register file.
   *
   * A bundle can optionally have 128-bits of embedded constants, shared across
   * all of the instructions within a bundle.
   *
   * Instructions consuming conditionals (branches and conditional selects)
   * require their condition to be written into the conditional register (r31)
   * within the same bundle they are consumed.
   *
   * Fragment writeout requires its argument to be written in full within the
   * same bundle as the branch, with no hanging dependencies.
   *
   * Load/store instructions are also in bundles of simply two instructions, and
   * texture instructions have no bundling.
   *
   * -------------------------------------------------------------------------
   *
   */
      /* We create the dependency graph with per-byte granularity */
      #define BYTE_COUNT 16
      static void
   add_dependency(struct util_dynarray *table, unsigned index, uint16_t mask,
         {
      for (unsigned i = 0; i < BYTE_COUNT; ++i) {
      if (!(mask & (1 << i)))
                     util_dynarray_foreach(parents, unsigned, parent) {
               /* Already have the dependency */
                  BITSET_SET(dependents, child);
            }
      static void
   mark_access(struct util_dynarray *table, unsigned index, uint16_t mask,
         {
      for (unsigned i = 0; i < BYTE_COUNT; ++i) {
      if (!(mask & (1 << i)))
                  }
      static void
   mir_create_dependency_graph(midgard_instruction **instructions, unsigned count,
         {
               struct util_dynarray *last_read = calloc(sizeof(struct util_dynarray), sz);
            for (unsigned i = 0; i < sz; ++i) {
      util_dynarray_init(&last_read[i], NULL);
               /* Initialize dependency graph */
   for (unsigned i = 0; i < count; ++i) {
      instructions[i]->dependents =
                                 /* Populate dependency graph */
   for (signed i = count - 1; i >= 0; --i) {
      if (instructions[i]->compact_branch)
            unsigned dest = instructions[i]->dest;
            mir_foreach_src((*instructions), s) {
               if (src < node_count) {
      unsigned readmask =
                        /* Create a list of dependencies for each type of load/store
   * instruction to prevent reordering. */
   if (instructions[i]->type == TAG_LOAD_STORE_4 &&
                              unsigned idx;
   switch (type) {
   case LDST_SHARED:
      idx = 0;
      case LDST_SCRATCH:
      idx = 1;
      default:
      idx = 2;
                                          /* Already have the dependency */
                  BITSET_SET(dependents, i);
                           if (dest < node_count) {
      add_dependency(last_read, dest, mask, instructions, i);
   add_dependency(last_write, dest, mask, instructions, i);
               mir_foreach_src((*instructions), s) {
               if (src < node_count) {
      unsigned readmask =
                           /* If there is a branch, all instructions depend on it, as interblock
            if (instructions[count - 1]->compact_branch) {
               for (signed i = count - 2; i >= 0; --i) {
                     BITSET_SET(dependents, i);
                  /* Free the intermediate structures */
   for (unsigned i = 0; i < sz; ++i) {
      util_dynarray_fini(&last_read[i]);
               free(last_read);
      }
      /* Does the mask cover more than a scalar? */
      static bool
   is_single_component_mask(unsigned mask)
   {
               for (int c = 0; c < 8; ++c) {
      if (mask & (1 << c))
                  }
      /* Helpers for scheudling */
      static bool
   mir_is_scalar(midgard_instruction *ains)
   {
      /* Do we try to use it as a vector op? */
   if (!is_single_component_mask(ains->mask))
            /* Otherwise, check mode hazards */
   bool could_scalar = true;
   unsigned szd = nir_alu_type_get_type_size(ains->dest_type);
   unsigned sz0 = nir_alu_type_get_type_size(ains->src_types[0]);
            /* Only 16/32-bit can run on a scalar unit */
            if (ains->src[0] != ~0)
            if (ains->src[1] != ~0)
            if (midgard_is_integer_out_op(ains->op) &&
      ains->outmod != midgard_outmod_keeplo)
            }
      /* How many bytes does this ALU instruction add to the bundle? */
      static unsigned
   bytes_for_instruction(midgard_instruction *ains)
   {
      if (ains->unit & UNITS_ANY_VECTOR)
         else if (ains->unit == ALU_ENAB_BRANCH)
         else if (ains->compact_branch)
         else
      }
      /* We would like to flatten the linked list of midgard_instructions in a bundle
   * to an array of pointers on the heap for easy indexing */
      static midgard_instruction **
   flatten_mir(midgard_block *block, unsigned *len)
   {
               if (!(*len))
            midgard_instruction **instructions =
                     mir_foreach_instr_in_block(block, ins)
               }
      /* The worklist is the set of instructions that can be scheduled now; that is,
   * the set of instructions with no remaining dependencies */
      static void
   mir_initialize_worklist(BITSET_WORD *worklist,
         {
      for (unsigned i = 0; i < count; ++i) {
      if (instructions[i]->nr_dependencies == 0)
         }
      /* Update the worklist after an instruction terminates. Remove its edges from
   * the graph and if that causes any node to have no dependencies, add it to the
   * worklist */
      static void
   mir_update_worklist(BITSET_WORD *worklist, unsigned count,
               {
      /* Sanity check: if no instruction terminated, there is nothing to do.
   * If the instruction that terminated had dependencies, that makes no
   * sense and means we messed up the worklist. Finally, as the purpose
   * of this routine is to update dependents, we abort early if there are
            if (!done)
                     if (!done->dependents)
            /* We have an instruction with dependents. Iterate each dependent to
   * remove one dependency (`done`), adding dependents to the worklist
            unsigned i;
   BITSET_FOREACH_SET(i, done->dependents, count) {
               if (!(--instructions[i]->nr_dependencies))
                  }
      /* While scheduling, we need to choose instructions satisfying certain
   * criteria. As we schedule backwards, we choose the *last* instruction in the
   * worklist to simulate in-order scheduling. Chosen instructions must satisfy a
   * given predicate. */
      struct midgard_predicate {
      /* TAG or ~0 for dont-care */
            /* True if we want to pop off the chosen instruction */
            /* For ALU, choose only this unit */
            /* State for bundle constants. constants is the actual constants
   * for the bundle. constant_count is the number of bytes (up to
   * 16) currently in use for constants. When picking in destructive
   * mode, the constants array will be updated, and the instruction
            midgard_constants *constants;
            /* Exclude this destination (if not ~0) */
            /* Don't schedule instructions consuming conditionals (since we already
   * scheduled one). Excludes conditional branches and csel */
            /* Require (or reject) a minimal mask and (if nonzero) given
            unsigned mask;
   unsigned no_mask;
            /* Whether to not-care/only/never schedule imov/fmov instructions This
   * allows non-move instructions to get priority on each unit */
            /* For load/store: how many pipeline registers are in use? The two
   * scheduled instructions cannot use more than the 256-bits of pipeline
   * space available or RA will fail (as it would run out of pipeline
                     /* For load/store: is a ST_VARY.a32 instruction scheduled into the
   * bundle? is a non-ST_VARY.a32 instruction scheduled? Potential
   * hardware issue, unknown cause.
   */
      };
      static bool
   mir_adjust_constant(midgard_instruction *ins, unsigned src,
               {
      unsigned type_size = nir_alu_type_get_type_size(ins->src_types[src]) / 8;
   unsigned type_shift = util_logbase2(type_size);
   unsigned max_comp = mir_components_for_type(ins->src_types[src]);
   unsigned comp_mask = mir_from_bytemask(
      mir_round_bytemask_up(mir_bytemask_of_read_components_index(ins, src),
                     /* Upper only makes sense for 16-bit */
   if (type_size != 16 && upper)
            /* For 16-bit, we need to stay on either upper or lower halves to avoid
   * disrupting the swizzle */
   unsigned start = upper ? 8 : 0;
            for (unsigned comp = 0; comp < max_comp; comp++) {
      if (!(comp_mask & (1 << comp)))
            uint8_t *constantp = ins->constants.u8 + (type_size * comp);
   unsigned best_reuse_bytes = 0;
   signed best_place = -1;
            for (i = start; i < (start + length); i += type_size) {
               for (j = 0; j < type_size; j++) {
      if (!(*bundle_constant_mask & (1 << (i + j))))
         if (constantp[j] != bundle_constants[i + j])
                                    /* Select the place where existing bytes can be
   * reused so we leave empty slots to others
   */
   if (j == type_size &&
      (reuse_bytes > best_reuse_bytes || best_place < 0)) {
   best_reuse_bytes = reuse_bytes;
   best_place = i;
                  /* This component couldn't fit in the remaining constant slot,
   * no need check the remaining components, bail out now
   */
   if (best_place < 0)
            memcpy(&bundle_constants[i], constantp, type_size);
   *bundle_constant_mask |= type_mask << best_place;
                  }
      /* For an instruction that can fit, adjust it to fit and update the constants
   * array, in destructive mode. Returns whether the fitting was successful. */
      static bool
   mir_adjust_constants(midgard_instruction *ins, struct midgard_predicate *pred,
         {
      /* No constant, nothing to adjust */
   if (!ins->has_constants)
            unsigned r_constant = SSA_FIXED_REGISTER(REGISTER_CONSTANT);
   unsigned bundle_constant_mask = pred->constant_mask;
   unsigned comp_mapping[2][16] = {};
                     /* Let's try to find a place for each active component of the constant
   * register.
   */
   for (unsigned src = 0; src < 2; ++src) {
      if (ins->src[src] != SSA_FIXED_REGISTER(REGISTER_CONSTANT))
            /* First, try lower half (or whole for !16) */
   if (mir_adjust_constant(ins, src, &bundle_constant_mask,
                  /* Next, try upper half */
   if (mir_adjust_constant(ins, src, &bundle_constant_mask,
                  /* Otherwise bail */
               /* If non-destructive, we're done */
   if (!destructive)
            /* Otherwise update the constant_mask and constant values */
   pred->constant_mask = bundle_constant_mask;
            /* Use comp_mapping as a swizzle */
   mir_foreach_src(ins, s) {
      if (ins->src[s] == r_constant)
                  }
      /* Conservative estimate of the pipeline registers required for load/store */
      static unsigned
   mir_pipeline_count(midgard_instruction *ins)
   {
               mir_foreach_src(ins, i) {
      /* Skip empty source  */
   if (ins->src[i] == ~0)
            if (i == 0) {
      /* First source is a vector, worst-case the mask */
   unsigned bytemask = mir_bytemask_of_read_components_index(ins, i);
   unsigned max = util_logbase2(bytemask) + 1;
      } else {
      /* Sources 1 on are scalars */
                  unsigned dwords = DIV_ROUND_UP(bytecount, 16);
               }
      /* Matches FADD x, x with modifiers compatible. Since x + x = x * 2, for
   * any x including of the form f(y) for some swizzle/abs/neg function f */
      static bool
   mir_is_add_2(midgard_instruction *ins)
   {
      if (ins->op != midgard_alu_op_fadd)
            if (ins->src[0] != ins->src[1])
            if (ins->src_types[0] != ins->src_types[1])
            for (unsigned i = 0; i < MIR_VEC_COMPONENTS; ++i) {
      if (ins->swizzle[0][i] != ins->swizzle[1][i])
               if (ins->src_abs[0] != ins->src_abs[1])
            if (ins->src_neg[0] != ins->src_neg[1])
               }
      static void
   mir_adjust_unit(midgard_instruction *ins, unsigned unit)
   {
      /* FADD x, x = FMUL x, #2 */
   if (mir_is_add_2(ins) && (unit & (UNITS_MUL | UNIT_VLUT))) {
               ins->src[1] = ~0;
   ins->src_abs[1] = false;
            ins->has_inline_constant = true;
         }
      static unsigned
   mir_has_unit(midgard_instruction *ins, unsigned unit)
   {
      if (alu_opcode_props[ins->op].props & unit)
            /* FADD x, x can run on any adder or any multiplier */
   if (mir_is_add_2(ins))
               }
      /* Net change in liveness if an instruction were scheduled. Loosely based on
   * ir3's scheduler. */
      static int
   mir_live_effect(uint16_t *liveness, midgard_instruction *ins, bool destructive)
   {
      /* TODO: what if dest is used multiple times? */
            if (ins->dest < SSA_FIXED_MINIMUM) {
      unsigned bytemask = mir_bytemask(ins);
   bytemask = util_next_power_of_two(bytemask + 1) - 1;
            if (destructive)
                        mir_foreach_src(ins, s) {
                        for (unsigned q = 0; q < s; ++q)
            if (dupe)
            if (S < SSA_FIXED_MINIMUM) {
                                    if (destructive)
                     }
      static midgard_instruction *
   mir_choose_instruction(midgard_instruction **instructions, uint16_t *liveness,
               {
      /* Parse the predicate */
   unsigned tag = predicate->tag;
   unsigned unit = predicate->unit;
   bool scalar = (unit != ~0) && (unit & UNITS_SCALAR);
            unsigned mask = predicate->mask;
   unsigned dest = predicate->dest;
            /* Iterate to find the best instruction satisfying the predicate */
            signed best_index = -1;
   signed best_effect = INT_MAX;
            /* Enforce a simple metric limiting distance to keep down register
   * pressure. TOOD: replace with liveness tracking for much better
            unsigned max_active = 0;
         #ifndef NDEBUG
      /* Force in-order scheduling */
   if (midgard_debug & MIDGARD_DBG_INORDER)
      #endif
         BITSET_FOREACH_SET(i, worklist, count) {
                  BITSET_FOREACH_SET(i, worklist, count) {
      if ((max_active - i) >= max_distance)
            if (tag != ~0 && instructions[i]->type != tag)
            bool alu = (instructions[i]->type == TAG_ALU_4);
            bool branch = alu && (unit == ALU_ENAB_BR_COMPACT);
   bool is_move = alu && (instructions[i]->op == midgard_alu_op_imov ||
            if (predicate->exclude != ~0 &&
                  if (alu && !branch && unit != ~0 &&
                  /* 0: don't care, 1: no moves, 2: only moves */
   if (predicate->move_mode && ((predicate->move_mode - 1) != is_move))
            if (branch && !instructions[i]->compact_branch)
            if (alu && scalar && !mir_is_scalar(instructions[i]))
            if (alu && predicate->constants &&
                  if (needs_dest && instructions[i]->dest != dest)
            if (mask && ((~instructions[i]->mask) & mask))
            if (instructions[i]->mask & predicate->no_mask)
            if (ldst &&
                           if (ldst && predicate->any_non_st_vary_a32 && st_vary_a32)
            if (ldst && predicate->any_st_vary_a32 && !st_vary_a32)
            bool conditional = alu && !branch && OP_IS_CSEL(instructions[i]->op);
            if (conditional && no_cond)
                     if (effect > best_effect)
            if (effect == best_effect && (signed)i < best_index)
            best_effect = effect;
   best_index = i;
                        if (best_index < 0)
            /* If we found something, remove it from the worklist */
   assert(best_index < count);
            if (predicate->destructive) {
               if (I->type == TAG_ALU_4)
            if (I->type == TAG_LOAD_STORE_4) {
                     if (instructions[best_index]->op == midgard_op_st_vary_32)
         else
               if (I->type == TAG_ALU_4)
            /* Once we schedule a conditional, we can't again */
   predicate->no_cond |= best_conditional;
                  }
      /* Still, we don't choose instructions in a vacuum. We need a way to choose the
   * best bundle type (ALU, load/store, texture). Nondestructive. */
      static unsigned
   mir_choose_bundle(midgard_instruction **instructions, uint16_t *liveness,
         {
      /* At the moment, our algorithm is very simple - use the bundle of the
   * best instruction, regardless of what else could be scheduled
            struct midgard_predicate predicate = {
      .tag = ~0,
   .unit = ~0,
   .destructive = false,
               midgard_instruction *chosen = mir_choose_instruction(
            if (chosen && chosen->type == TAG_LOAD_STORE_4 && !(num_ldst % 2)) {
               predicate.exclude = chosen->dest;
            chosen = mir_choose_instruction(instructions, liveness, worklist, count,
         if (chosen)
                     chosen = mir_choose_instruction(instructions, liveness, worklist, count,
                  if (chosen)
         else
               if (chosen)
         else
      }
      /* We want to choose an ALU instruction filling a given unit */
   static void
   mir_choose_alu(midgard_instruction **slot, midgard_instruction **instructions,
               {
      /* Did we already schedule to this slot? */
   if ((*slot) != NULL)
            /* Try to schedule something, if not */
   predicate->unit = unit;
   *slot =
            /* Store unit upon scheduling */
   if (*slot && !((*slot)->compact_branch))
      }
      /* When we are scheduling a branch/csel, we need the consumed condition in the
   * same block as a pipeline register. There are two options to enable this:
   *
   *  - Move the conditional into the bundle. Preferred, but only works if the
   *    conditional is used only once and is from this block.
   *  - Copy the conditional.
   *
   * We search for the conditional. If it's in this block, single-use, and
   * without embedded constants, we schedule it immediately. Otherwise, we
   * schedule a move for it.
   *
   * mir_comparison_mobile is a helper to find the moveable condition.
   */
      static unsigned
   mir_comparison_mobile(compiler_context *ctx, midgard_instruction **instructions,
               {
      if (!mir_single_use(ctx, cond))
                     for (unsigned i = 0; i < count; ++i) {
      if (instructions[i]->dest != cond)
            /* Must fit in an ALU bundle */
   if (instructions[i]->type != TAG_ALU_4)
            /* If it would itself require a condition, that's recursive */
   if (OP_IS_CSEL(instructions[i]->op))
            /* We'll need to rewrite to .w but that doesn't work for vector
            if (GET_CHANNEL_COUNT(alu_opcode_props[instructions[i]->op].props))
                     if (!mir_adjust_constants(instructions[i], predicate, false))
                     if (ret != ~0)
         else
               /* Inject constants now that we are sure we want to */
   if (ret != ~0)
               }
      /* Using the information about the moveable conditional itself, we either pop
   * that condition off the worklist for use now, or create a move to
   * artificially schedule instead as a fallback */
      static midgard_instruction *
   mir_schedule_comparison(compiler_context *ctx,
                           midgard_instruction **instructions,
   {
      /* TODO: swizzle when scheduling */
   unsigned comp_i =
      (!vector && (swizzle[0] == 0))
               /* If we can, schedule the condition immediately */
   if ((comp_i != ~0) && BITSET_TEST(worklist, comp_i)) {
      assert(comp_i < count);
   BITSET_CLEAR(worklist, comp_i);
                        midgard_instruction mov = v_mov(cond, cond);
   mov.mask = vector ? 0xF : 0x1;
               }
      /* Most generally, we need instructions writing to r31 in the appropriate
   * components */
      static midgard_instruction *
   mir_schedule_condition(compiler_context *ctx,
                           {
      /* For a branch, the condition is the only argument; for csel, third */
   bool branch = last->compact_branch;
            /* csel_v is vector; otherwise, conditions are scalar */
                     midgard_instruction *cond = mir_schedule_comparison(
      ctx, instructions, predicate, worklist, count, last->src[condition_index],
         /* We have exclusive reign over this (possibly move) conditional
            predicate->exclude = cond->dest;
   cond->dest = SSA_FIXED_REGISTER(31);
            if (!vector) {
               mir_foreach_src(cond, s) {
                     for (unsigned q = 0; q < 4; ++q)
                           /* Schedule the unit: csel is always in the latter pipeline, so a csel
   * condition must be in the former pipeline stage (vmul/sadd),
   * depending on scalar/vector of the instruction itself. A branch must
   * be written from the latter pipeline stage and a branch condition is
   * always scalar, so it is always in smul (exception: ball/bany, which
            if (branch)
         else
               }
      /* Schedules a single bundle of the given type */
      static midgard_bundle
   mir_schedule_texture(midgard_instruction **instructions, uint16_t *liveness,
         {
      struct midgard_predicate predicate = {
      .tag = TAG_TEXTURE_4,
   .destructive = true,
               midgard_instruction *ins =
                     struct midgard_bundle out = {
      .tag = ins->op == midgard_tex_op_barrier ? TAG_TEXTURE_4_BARRIER
         : (ins->op == midgard_tex_op_fetch) || is_vertex
         .instruction_count = 1,
                  }
      static midgard_bundle
   mir_schedule_ldst(midgard_instruction **instructions, uint16_t *liveness,
         {
      struct midgard_predicate predicate = {
      .tag = TAG_LOAD_STORE_4,
   .destructive = true,
                        midgard_instruction *ins =
            midgard_instruction *pair =
                     struct midgard_bundle out = {
      .tag = TAG_LOAD_STORE_4,
   .instruction_count = pair ? 2 : 1,
                        /* We have to update the worklist atomically, since the two
            mir_update_worklist(worklist, len, instructions, ins);
               }
      static void
   mir_schedule_zs_write(compiler_context *ctx,
                        struct midgard_predicate *predicate,
   midgard_instruction **instructions, uint16_t *liveness,
      {
      bool success = false;
   unsigned idx = stencil ? 3 : 2;
   unsigned src =
            predicate->dest = src;
            midgard_instruction **units[] = {smul, vadd, vlut};
            for (unsigned i = 0; i < 3; ++i) {
      if (*(units[i]))
            predicate->unit = unit_names[i];
   midgard_instruction *ins = mir_choose_instruction(
            if (ins) {
      ins->unit = unit_names[i];
   *(units[i]) = ins;
   success |= true;
                           if (success)
            midgard_instruction *mov = ralloc(ctx, midgard_instruction);
   *mov = v_mov(src, make_compiler_temp(ctx));
                     if (stencil) {
               for (unsigned c = 0; c < 16; ++c)
               for (unsigned i = 0; i < 3; ++i) {
      if (!(*(units[i]))) {
      *(units[i]) = mov;
   mov->unit = unit_names[i];
                     }
      static midgard_bundle
   mir_schedule_alu(compiler_context *ctx, midgard_instruction **instructions,
         {
                        struct midgard_predicate predicate = {
      .tag = TAG_ALU_4,
   .destructive = true,
   .exclude = ~0,
               midgard_instruction *vmul = NULL;
   midgard_instruction *vadd = NULL;
   midgard_instruction *vlut = NULL;
   midgard_instruction *smul = NULL;
   midgard_instruction *sadd = NULL;
            mir_choose_alu(&branch, instructions, liveness, worklist, len, &predicate,
         mir_update_worklist(worklist, len, instructions, branch);
            if (branch && branch->branch.conditional) {
      midgard_instruction *cond = mir_schedule_condition(
            if (cond->unit == UNIT_VADD)
         else if (cond->unit == UNIT_SMUL)
         else
               /* If we have a render target reference, schedule a move for it. Since
   * this will be in sadd, we boost this to prevent scheduling csel into
            if (writeout && (branch->constants.u32[0] || ctx->inputs->is_blend)) {
      sadd = ralloc(ctx, midgard_instruction);
   *sadd = v_mov(~0, make_compiler_temp(ctx));
   sadd->unit = UNIT_SADD;
   sadd->mask = 0x1;
   sadd->has_inline_constant = true;
   sadd->inline_constant = branch->constants.u32[0];
   branch->src[1] = sadd->dest;
               if (writeout) {
      /* Propagate up */
            /* Mask off any conditionals.
   * This prevents csel and csel_v being scheduled into smul
   * since we might not have room for a conditional in vmul/sadd.
   * This is important because both writeout and csel have same-bundle
   * requirements on their dependencies. */
               /* Set r1.w to the return address so we can return from blend shaders */
   if (writeout) {
      vadd = ralloc(ctx, midgard_instruction);
            if (!ctx->inputs->is_blend) {
      vadd->op = midgard_alu_op_iadd;
                                 vadd->has_inline_constant = true;
      } else {
                     for (unsigned c = 0; c < 16; ++c)
               vadd->unit = UNIT_VADD;
   vadd->mask = 0x1;
   branch->dest = vadd->dest;
               if (writeout & PAN_WRITEOUT_Z)
      mir_schedule_zs_write(ctx, &predicate, instructions, liveness, worklist,
         if (writeout & PAN_WRITEOUT_S)
      mir_schedule_zs_write(ctx, &predicate, instructions, liveness, worklist,
         mir_choose_alu(&smul, instructions, liveness, worklist, len, &predicate,
            for (unsigned mode = 1; mode < 3; ++mode) {
      predicate.move_mode = mode;
   predicate.no_mask = writeout ? (1 << 3) : 0;
   mir_choose_alu(&vlut, instructions, liveness, worklist, len, &predicate,
         predicate.no_mask = 0;
   mir_choose_alu(&vadd, instructions, liveness, worklist, len, &predicate,
               /* Reset */
   predicate.move_mode = 0;
            mir_update_worklist(worklist, len, instructions, vlut);
   mir_update_worklist(worklist, len, instructions, vadd);
            bool vadd_csel = vadd && OP_IS_CSEL(vadd->op);
            if (vadd_csel || smul_csel) {
      midgard_instruction *ins = vadd_csel ? vadd : smul;
   midgard_instruction *cond = mir_schedule_condition(
            if (cond->unit == UNIT_VMUL)
         else if (cond->unit == UNIT_SADD)
         else
               /* Stage 2, let's schedule sadd before vmul for writeout */
   mir_choose_alu(&sadd, instructions, liveness, worklist, len, &predicate,
                     if (writeout) {
      midgard_instruction *stages[] = {sadd, vadd, smul, vlut};
   unsigned src =
         unsigned writeout_mask = 0x0;
            for (unsigned i = 0; i < ARRAY_SIZE(stages); ++i) {
                                    writeout_mask |= stages[i]->mask;
               /* It's possible we'll be able to schedule something into vmul
   * to fill r0. Let's peak into the future, trying to schedule
                     if (!bad_writeout && writeout_mask != full_mask) {
      predicate.unit = UNIT_VMUL;
                                 if (peaked) {
      vmul = peaked;
   vmul->unit = UNIT_VMUL;
   writeout_mask |= predicate.mask;
               /* Cleanup */
               /* Finally, add a move if necessary */
   if (bad_writeout || writeout_mask != full_mask) {
                     vmul = ralloc(ctx, midgard_instruction);
   *vmul = v_mov(src, temp);
                           for (unsigned i = 0; i < ARRAY_SIZE(stages); ++i) {
      if (stages[i]) {
      mir_rewrite_index_dst_single(stages[i], src, temp);
                                 mir_choose_alu(&vmul, instructions, liveness, worklist, len, &predicate,
            mir_update_worklist(worklist, len, instructions, vmul);
                              /* Now that we have finished scheduling, build up the bundle */
            for (unsigned i = 0; i < ARRAY_SIZE(stages); ++i) {
      if (stages[i]) {
      bundle.control |= stages[i]->unit;
                  /* If we branch, we can't spill to TLS since the store
   * instruction will never get executed. We could try to
                  if (branch)
                           if (bytes_emitted & 15) {
      padding = 16 - (bytes_emitted & 15);
               /* Constants must always be quadwords */
   if (bundle.has_embedded_constants)
            /* Size ALU instruction for tag */
            bool tilebuf_wait = branch && branch->compact_branch &&
            /* MRT capable GPUs use a special writeout procedure */
   if ((writeout || tilebuf_wait) && !(ctx->quirks & MIDGARD_NO_UPPER_ALU))
            bundle.padding = padding;
               }
      /* Schedule a single block by iterating its instruction to create bundles.
   * While we go, tally about the bundle sizes to compute the block size. */
      static void
   schedule_block(compiler_context *ctx, midgard_block *block)
   {
      /* Copy list to dynamic array */
   unsigned len = 0;
            if (!len)
            /* Calculate dependencies and initial worklist */
   unsigned node_count = ctx->temp_count + 1;
            /* Allocate the worklist */
   size_t sz = BITSET_WORDS(len) * sizeof(BITSET_WORD);
   BITSET_WORD *worklist = calloc(sz, 1);
   uint16_t *liveness = calloc(node_count, 2);
            /* Count the number of load/store instructions so we know when it's
   * worth trying to schedule them in pairs. */
   unsigned num_ldst = 0;
   for (unsigned i = 0; i < len; ++i) {
      if (instructions[i]->type == TAG_LOAD_STORE_4)
               struct util_dynarray bundles;
                     for (;;) {
      unsigned tag =
                  if (tag == TAG_TEXTURE_4)
      bundle = mir_schedule_texture(instructions, liveness, worklist, len,
      else if (tag == TAG_LOAD_STORE_4)
      bundle =
      else if (tag == TAG_ALU_4)
         else
            for (unsigned i = 0; i < bundle.instruction_count; ++i)
                  util_dynarray_append(&bundles, midgard_bundle, bundle);
                                 util_dynarray_init(&block->bundles, block);
   util_dynarray_foreach_reverse(&bundles, midgard_bundle, bundle) {
         }
            block->scheduled = true;
            /* Reorder instructions to match bundled. First remove existing
            mir_foreach_instr_in_block_safe(block, ins) {
                  mir_foreach_instr_in_block_scheduled_rev(block, ins) {
                  free(instructions); /* Allocated by flatten_mir() */
   free(worklist);
      }
      /* Insert moves to ensure we can register allocate load/store registers */
   static void
   mir_lower_ldst(compiler_context *ctx)
   {
      mir_foreach_instr_global_safe(ctx, I) {
      if (I->type != TAG_LOAD_STORE_4)
            mir_foreach_src(I, s) {
      if (s == 0)
         if (I->src[s] == ~0)
                        unsigned temp = make_compiler_temp(ctx);
   midgard_instruction mov = v_mov(I->src[s], temp);
   mov.mask = 0x1;
   mov.dest_type = I->src_types[s];
                  mir_insert_instruction_before(ctx, I, mov);
   I->src[s] = mov.dest;
            }
      /* Insert moves to ensure we can register allocate blend writeout */
   static void
   mir_lower_blend_input(compiler_context *ctx)
   {
      mir_foreach_block(ctx, _blk) {
               if (list_is_empty(&blk->base.instructions))
                     if (!I || I->type != TAG_ALU_4 || !I->writeout)
            mir_foreach_src(I, s) {
                                             unsigned temp = make_compiler_temp(ctx);
   midgard_instruction mov = v_mov(src, temp);
   mov.mask = 0xF;
   mov.dest_type = nir_type_uint32;
   mir_insert_instruction_before(ctx, I, mov);
            }
      void
   midgard_schedule_program(compiler_context *ctx)
   {
      mir_lower_ldst(ctx);
            /* Must be lowered right before scheduling */
   mir_lower_special_reads(ctx);
            if (ctx->stage == MESA_SHADER_FRAGMENT) {
      mir_invalidate_liveness(ctx);
   mir_compute_liveness(ctx);
                                 mir_foreach_block(ctx, _block) {
      midgard_block *block = (midgard_block *)_block;
   midgard_opt_dead_move_eliminate(ctx, block);
         }
