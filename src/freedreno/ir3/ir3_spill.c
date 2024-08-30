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
      #include "util/rb_tree.h"
   #include "ir3_ra.h"
   #include "ir3_shader.h"
      /*
   * This pass does two things:
   *
   * 1. Calculates the maximum register pressure. To do this, we need to use the
   *    exact same technique that RA uses for combining meta_split instructions
   *    with their sources, so that our calculation agrees with RA.
   * 2. Spills when the register pressure is exceeded a limit calculated by RA.
   *    The implementation is based on "Register Spilling and Live-Range Splitting
   *    for SSA-Form Programs" by Braun and Hack, although again care has to be
   *    taken to handle combining split/collect instructions.
   */
      struct reg_or_immed {
      unsigned flags;
   union {
      struct ir3_register *def;
   uint32_t uimm;
         };
      struct ra_spill_interval {
               struct rb_node node;
            /* The current SSA value/const/immed this source is mapped to. */
            /* When computing use distances we use the distance relative to the start
   * of the block. So, for example, a value that's defined in cycle 5 of the
   * block and used 6 cycles later will always have a next_use_distance of 11
   * until we reach that use.
   */
            /* Whether this value was reloaded and therefore doesn't need to be
   * spilled again. Corresponds to the S set in the paper.
   */
            /* We need to add sources early for accounting purposes, but we have to
   * insert the reload code for them last. Keep track of whether this interval
   * needs to be reloaded later.
   */
            /* Keep track of whether this interval currently can't be spilled because:
   * - It or one of its children is a source and we're making space for
   *   sources.
   * - It is a destination and we're making space for destinations.
   */
            /* Whether this interval can be rematerialized. */
      };
      struct ra_spill_block_state {
      unsigned *next_use_end;
                     /* Map from SSA def to reg_or_immed it is mapped to at the end of the block.
   * This map only contains values which we didn't spill, so it also serves as
   * a record of the new live-out set for this block.
   */
            /* For blocks whose successors are visited first (i.e. loop backedges), which
   * values should be live at the end.
   */
               };
      struct ra_spill_ctx {
               struct ra_spill_interval **intervals;
            /* rb tree of live intervals that we can spill, ordered by next-use distance.
   * full_live_intervals contains the full+shared intervals in the merged_regs
   * case. We use this list to determine what to spill.
   */
   struct rb_tree full_live_intervals;
                              /* When spilling, we need to reserve a register to serve as the zero'd
   * "base". For simplicity we reserve a register at the beginning so that it's
   * always available.
   */
            /* Current pvtmem offset in bytes. */
                                                   };
      static void
   add_base_reg(struct ra_spill_ctx *ctx, struct ir3 *ir)
   {
               /* We need to stick it after any meta instructions which need to be first. */
   struct ir3_instruction *after = NULL;
   foreach_instr (instr, &start->instr_list) {
      if (instr->opc != OPC_META_INPUT &&
      instr->opc != OPC_META_TEX_PREFETCH) {
   after = instr;
                           if (after)
                     /* We don't create an interval, etc. for the base reg, so just lower the
   * register pressure limit to account for it. We assume it's always
   * available for simplicity.
   */
      }
         /* Compute the number of cycles per instruction used for next-use-distance
   * analysis. This is just approximate, obviously.
   */
   static unsigned
   instr_cycles(struct ir3_instruction *instr)
   {
      if (instr->opc == OPC_META_PARALLEL_COPY) {
      unsigned cycles = 0;
   for (unsigned i = 0; i < instr->dsts_count; i++) {
      if (!instr->srcs[i]->def ||
      instr->srcs[i]->def->merge_set != instr->dsts[i]->merge_set) {
                              if (instr->opc == OPC_META_COLLECT) {
      unsigned cycles = 0;
   for (unsigned i = 0; i < instr->srcs_count; i++) {
      if (!instr->srcs[i]->def ||
      instr->srcs[i]->def->merge_set != instr->dsts[0]->merge_set) {
                              if (is_meta(instr))
               }
      static bool
   compute_block_next_distance(struct ra_spill_ctx *ctx, struct ir3_block *block,
         {
      struct ra_spill_block_state *state = &ctx->blocks[block->index];
   memcpy(tmp_next_use, state->next_use_end,
            unsigned cycle = state->cycles;
   foreach_instr_rev (instr, &block->instr_list) {
      ra_foreach_dst (dst, instr) {
                  ra_foreach_src (src, instr) {
                           if (instr->opc == OPC_META_PARALLEL_COPY) {
      ra_foreach_src_n (src, i, instr) {
      if (src->def->merge_set == instr->dsts[i]->merge_set &&
      src->def->merge_set_offset == instr->dsts[i]->merge_set_offset) {
   tmp_next_use[src->def->name] =
      } else {
               } else if (instr->opc != OPC_META_PHI) {
      ra_foreach_src (src, instr) {
                     ra_foreach_dst (dst, instr) {
                     memcpy(state->next_use_start, tmp_next_use,
            bool progress = false;
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      const struct ir3_block *pred = block->predecessors[i];
            /* Add a large-enough distance in front of edges exiting the loop so that
   * variables that are live-through the loop but not used inside it are
   * prioritized for spilling, as per the paper. This just needs to be
   * larger than the longest path through the loop.
   */
   bool loop_exit = pred->loop_depth < block->loop_depth;
            for (unsigned j = 0; j < ctx->live->definitions_count; j++) {
      if (state->next_use_start[j] < UINT_MAX &&
      state->next_use_start[j] + block_distance <
   pred_state->next_use_end[j]) {
   pred_state->next_use_end[j] = state->next_use_start[j] +
                        foreach_instr (phi, &block->instr_list) {
      if (phi->opc != OPC_META_PHI)
         if (!phi->srcs[i]->def)
         unsigned src = phi->srcs[i]->def->name;
   if (phi->dsts[0]->next_use < UINT_MAX &&
      phi->dsts[0]->next_use + block_distance <
   pred_state->next_use_end[src]) {
   pred_state->next_use_end[src] = phi->dsts[0]->next_use +
                              }
      static void
   compute_next_distance(struct ra_spill_ctx *ctx, struct ir3 *ir)
   {
      for (unsigned i = 0; i < ctx->live->block_count; i++) {
      ctx->blocks[i].next_use_start =
         ctx->blocks[i].next_use_end =
            for (unsigned j = 0; j < ctx->live->definitions_count; j++) {
      ctx->blocks[i].next_use_start[j] = UINT_MAX;
                  foreach_block (block, &ir->block_list) {
      struct ra_spill_block_state *state = &ctx->blocks[block->index];
   state->cycles = 0;
   foreach_instr (instr, &block->instr_list) {
      state->cycles += instr_cycles(instr);
   foreach_dst (dst, instr) {
                        unsigned *tmp_next_use =
            bool progress = true;
   while (progress) {
      progress = false;
   foreach_block_rev (block, &ir->block_list) {
               }
      static bool
   can_rematerialize(struct ir3_register *reg)
   {
      if (reg->flags & IR3_REG_ARRAY)
         if (reg->instr->opc != OPC_MOV)
         if (!(reg->instr->srcs[0]->flags & (IR3_REG_IMMED | IR3_REG_CONST)))
         if (reg->instr->srcs[0]->flags & IR3_REG_RELATIV)
            }
      static struct ir3_register *
   rematerialize(struct ir3_register *reg, struct ir3_instruction *after,
         {
               struct ir3_instruction *remat =
         struct ir3_register *dst = __ssa_dst(remat);
   dst->flags |= reg->flags & (IR3_REG_HALF | IR3_REG_ARRAY);
   for (unsigned i = 0; i < reg->instr->srcs_count; i++) {
      struct ir3_register *src =
                              dst->merge_set = reg->merge_set;
   dst->merge_set_offset = reg->merge_set_offset;
   dst->interval_start = reg->interval_start;
            if (after)
               }
      static void
   ra_spill_interval_init(struct ra_spill_interval *interval,
         {
      ir3_reg_interval_init(&interval->interval, reg);
   interval->dst.flags = reg->flags;
   interval->dst.def = reg;
   interval->already_spilled = false;
   interval->needs_reload = false;
   interval->cant_spill = false;
      }
      static struct ra_spill_interval *
   ir3_reg_interval_to_interval(struct ir3_reg_interval *interval)
   {
         }
      static struct ra_spill_interval *
   ra_spill_interval_root(struct ra_spill_interval *interval)
   {
      struct ir3_reg_interval *ir3_interval = &interval->interval;
   while (ir3_interval->parent)
            }
      static struct ra_spill_ctx *
   ir3_reg_ctx_to_ctx(struct ir3_reg_ctx *ctx)
   {
         }
      static int
   spill_interval_cmp(const struct ra_spill_interval *a,
         {
      /* Prioritize intervals that we can rematerialize. */
   if (a->can_rematerialize && !b->can_rematerialize)
         if (!a->can_rematerialize && b->can_rematerialize)
               }
      static int
   ra_spill_interval_cmp(const struct rb_node *_a, const struct rb_node *_b)
   {
      const struct ra_spill_interval *a =
         const struct ra_spill_interval *b =
            }
      static int
   ra_spill_interval_half_cmp(const struct rb_node *_a, const struct rb_node *_b)
   {
      const struct ra_spill_interval *a =
         const struct ra_spill_interval *b =
            }
      static void
   interval_add(struct ir3_reg_ctx *_ctx, struct ir3_reg_interval *_interval)
   {
      struct ra_spill_interval *interval = ir3_reg_interval_to_interval(_interval);
            unsigned size = reg_size(interval->interval.reg);
   if (interval->interval.reg->flags & IR3_REG_SHARED) {
         } else {
      if (interval->interval.reg->flags & IR3_REG_HALF) {
      ctx->cur_pressure.half += size;
   if (ctx->spilling) {
      rb_tree_insert(&ctx->half_live_intervals, &interval->half_node,
         }
   if (ctx->merged_regs || !(interval->interval.reg->flags & IR3_REG_HALF)) {
      ctx->cur_pressure.full += size;
   if (ctx->spilling) {
      rb_tree_insert(&ctx->full_live_intervals, &interval->node,
               }
      static void
   interval_delete(struct ir3_reg_ctx *_ctx, struct ir3_reg_interval *_interval)
   {
      struct ra_spill_interval *interval = ir3_reg_interval_to_interval(_interval);
            unsigned size = reg_size(interval->interval.reg);
   if (interval->interval.reg->flags & IR3_REG_SHARED) {
         } else {
      if (interval->interval.reg->flags & IR3_REG_HALF) {
      ctx->cur_pressure.half -= size;
   if (ctx->spilling) {
            }
   if (ctx->merged_regs || !(interval->interval.reg->flags & IR3_REG_HALF)) {
      ctx->cur_pressure.full -= size;
   if (ctx->spilling) {
                  }
      static void
   interval_readd(struct ir3_reg_ctx *_ctx, struct ir3_reg_interval *_parent,
         {
         }
      static void
   spill_ctx_init(struct ra_spill_ctx *ctx, struct ir3_shader_variant *v,
         {
      ctx->live = live;
   ctx->intervals = ralloc_array(ctx, struct ra_spill_interval *,
         struct ra_spill_interval *intervals =
      rzalloc_array(ctx, struct ra_spill_interval,
      for (unsigned i = 0; i < ctx->live->definitions_count; i++)
            ctx->intervals_count = ctx->live->definitions_count;
   ctx->compiler = v->compiler;
            rb_tree_init(&ctx->reg_ctx.intervals);
   ctx->reg_ctx.interval_add = interval_add;
   ctx->reg_ctx.interval_delete = interval_delete;
      }
      static void
   ra_spill_ctx_insert(struct ra_spill_ctx *ctx,
         {
         }
      static void
   ra_spill_ctx_remove(struct ra_spill_ctx *ctx,
         {
         }
      static void
   init_dst(struct ra_spill_ctx *ctx, struct ir3_register *dst)
   {
      struct ra_spill_interval *interval = ctx->intervals[dst->name];
   ra_spill_interval_init(interval, dst);
   if (ctx->spilling) {
               /* We only need to keep track of used-ness if this value may be
   * rematerialized. This also keeps us from nuking things that may be
   * in the keeps list (e.g. atomics, input splits).
   */
   if (interval->can_rematerialize)
         }
      static void
   insert_dst(struct ra_spill_ctx *ctx, struct ir3_register *dst)
   {
      struct ra_spill_interval *interval = ctx->intervals[dst->name];
   if (interval->interval.inserted)
            ra_spill_ctx_insert(ctx, interval);
            /* For precolored inputs, make sure we leave enough registers to allow for
   * holes in the inputs. It can happen that the binning shader has a lower
   * register pressure than the main shader, but the main shader decided to
   * add holes between the inputs which means that the binning shader has a
   * higher register demand.
   */
   if (dst->instr->opc == OPC_META_INPUT && dst->num != INVALID_REG) {
      physreg_t physreg = ra_reg_get_physreg(dst);
            if (interval->interval.reg->flags & IR3_REG_SHARED)
         else if (interval->interval.reg->flags & IR3_REG_HALF)
         else
         }
      static void
   insert_src(struct ra_spill_ctx *ctx, struct ir3_register *src)
   {
               if (!interval->interval.inserted) {
      ra_spill_ctx_insert(ctx, interval);
   interval->needs_reload = true;
                     }
      static void
   remove_src_early(struct ra_spill_ctx *ctx, struct ir3_instruction *instr,
         {
               if (!interval->interval.inserted || interval->interval.parent ||
      !rb_tree_is_empty(&interval->interval.children))
            }
      static void
   remove_src(struct ra_spill_ctx *ctx, struct ir3_instruction *instr,
         {
               if (!interval->interval.inserted)
               }
      static void
   finish_dst(struct ra_spill_ctx *ctx, struct ir3_register *dst)
   {
      struct ra_spill_interval *interval = ctx->intervals[dst->name];
      }
      static void
   remove_dst(struct ra_spill_ctx *ctx, struct ir3_register *dst)
   {
               if (!interval->interval.inserted)
               }
      static void
   update_src_next_use(struct ra_spill_ctx *ctx, struct ir3_register *src)
   {
                                 /* If this node is inserted in one of the trees, then it needs to be resorted
   * as its key has changed.
   */
   if (!interval->interval.parent && !(src->flags & IR3_REG_SHARED)) {
      if (src->flags & IR3_REG_HALF) {
      rb_tree_remove(&ctx->half_live_intervals, &interval->half_node);
   rb_tree_insert(&ctx->half_live_intervals, &interval->half_node,
      }
   if (ctx->merged_regs || !(src->flags & IR3_REG_HALF)) {
      rb_tree_remove(&ctx->full_live_intervals, &interval->node);
   rb_tree_insert(&ctx->full_live_intervals, &interval->node,
            }
      static unsigned
   get_spill_slot(struct ra_spill_ctx *ctx, struct ir3_register *reg)
   {
      if (reg->merge_set) {
      if (reg->merge_set->spill_slot == ~0) {
      reg->merge_set->spill_slot = ALIGN_POT(ctx->spill_slot,
            }
      } else {
      if (reg->spill_slot == ~0) {
      reg->spill_slot = ALIGN_POT(ctx->spill_slot, reg_elem_size(reg));
      }
         }
      static void
   set_src_val(struct ir3_register *src, const struct reg_or_immed *val)
   {
      if (val->flags & IR3_REG_IMMED) {
      src->flags = IR3_REG_IMMED | (val->flags & IR3_REG_HALF);
   src->uim_val = val->uimm;
      } else if (val->flags & IR3_REG_CONST) {
      src->flags = IR3_REG_CONST | (val->flags & IR3_REG_HALF);
   src->num = val->const_num;
      } else {
      src->def = val->def;
         }
      static struct ir3_register *
   materialize_pcopy_src(const struct reg_or_immed *src,
               {
      struct ir3_instruction *mov = ir3_instr_create(block, OPC_MOV, 1, 1);
   struct ir3_register *dst = __ssa_dst(mov);
   dst->flags |= src->flags & IR3_REG_HALF;
   struct ir3_register *mov_src = ir3_src_create(mov, INVALID_REG, src->flags);
   set_src_val(mov_src, src);
   mov->cat1.src_type = mov->cat1.dst_type =
            if (instr)
            }
      static void
   spill(struct ra_spill_ctx *ctx, const struct reg_or_immed *val,
         {
               /* If spilling an immed/const pcopy src, we need to actually materialize it
   * first with a mov.
   */
   if (val->flags & (IR3_REG_CONST | IR3_REG_IMMED)) {
         } else {
      reg = val->def;
               d("spilling ssa_%u:%u to %u", reg->instr->serialno, reg->name,
            unsigned elems = reg_elems(reg);
   struct ir3_instruction *spill =
         ir3_src_create(spill, INVALID_REG, ctx->base_reg->flags)->def = ctx->base_reg;
   unsigned src_flags = reg->flags & (IR3_REG_HALF | IR3_REG_IMMED |
               struct ir3_register *src = ir3_src_create(spill, INVALID_REG, src_flags);
   ir3_src_create(spill, INVALID_REG, IR3_REG_IMMED)->uim_val = elems;
   spill->cat6.dst_offset = spill_slot;
   spill->cat6.type = (reg->flags & IR3_REG_HALF) ? TYPE_U16 : TYPE_U32;
      src->def = reg;
   if (reg->flags & IR3_REG_ARRAY) {
      src->size = reg->size;
   src->array.id = reg->array.id;
      } else {
                  if (instr)
      }
      static void
   spill_interval(struct ra_spill_ctx *ctx, struct ra_spill_interval *interval,
         {
      if (interval->can_rematerialize && !interval->interval.reg->merge_set)
            spill(ctx, &interval->dst, get_spill_slot(ctx, interval->interval.reg),
      }
      /* This is similar to "limit" in the paper. */
   static void
   limit(struct ra_spill_ctx *ctx, struct ir3_instruction *instr)
   {
      if (ctx->cur_pressure.half > ctx->limit_pressure.half) {
      d("cur half pressure %u exceeds %u", ctx->cur_pressure.half,
   ctx->limit_pressure.half);
   rb_tree_foreach_safe (struct ra_spill_interval, interval,
            d("trying ssa_%u:%u", interval->interval.reg->instr->serialno,
   interval->interval.reg->name);
   if (!interval->cant_spill) {
      if (!interval->already_spilled)
         ir3_reg_interval_remove_all(&ctx->reg_ctx, &interval->interval);
   if (ctx->cur_pressure.half <= ctx->limit_pressure.half)
                              if (ctx->cur_pressure.full > ctx->limit_pressure.full) {
      d("cur full pressure %u exceeds %u", ctx->cur_pressure.full,
   ctx->limit_pressure.full);
   rb_tree_foreach_safe (struct ra_spill_interval, interval,
            d("trying ssa_%u:%u", interval->interval.reg->instr->serialno,
   interval->interval.reg->name);
   if (!interval->cant_spill) {
      if (!interval->already_spilled)
         ir3_reg_interval_remove_all(&ctx->reg_ctx, &interval->interval);
   if (ctx->cur_pressure.full <= ctx->limit_pressure.full)
      } else {
                           }
      /* There's a corner case where we reload a value which has overlapping live
   * values already reloaded, either because it's the child of some other interval
   * that was already reloaded or some of its children have already been
   * reloaded. Because RA only expects overlapping source/dest intervals for meta
   * instructions (split/collect), and we don't want to add register pressure by
   * creating an entirely separate value, we need to add splits and collects to
   * deal with this case. These splits/collects have to also have correct merge
   * set information, so that it doesn't result in any actual code or register
   * pressure in practice.
   */
      static void
   add_to_merge_set(struct ir3_merge_set *set, struct ir3_register *def,
         {
      def->merge_set = set;
   def->merge_set_offset = offset;
   def->interval_start = set->interval_start + offset;
      }
      static struct ir3_register *
   split(struct ir3_register *def, unsigned offset,
         {
      if (reg_elems(def) == 1) {
      assert(offset == 0);
               assert(!(def->flags & IR3_REG_ARRAY));
   assert(def->merge_set);
   struct ir3_instruction *split =
         struct ir3_register *dst = __ssa_dst(split);
   dst->flags |= def->flags & IR3_REG_HALF;
   struct ir3_register *src = ir3_src_create(split, INVALID_REG, def->flags);
   src->wrmask = def->wrmask;
   src->def = def;
   add_to_merge_set(def->merge_set, dst,
         if (after)
            }
      static struct ir3_register *
   extract(struct ir3_register *parent_def, unsigned offset, unsigned elems,
         {
      if (offset == 0 && elems == reg_elems(parent_def))
            struct ir3_register *srcs[elems];
   for (unsigned i = 0; i < elems; i++) {
                  struct ir3_instruction *collect =
         struct ir3_register *dst = __ssa_dst(collect);
   dst->flags |= parent_def->flags & IR3_REG_HALF;
   dst->wrmask = MASK(elems);
            for (unsigned i = 0; i < elems; i++) {
                  if (after)
            }
      static struct ir3_register *
   reload(struct ra_spill_ctx *ctx, struct ir3_register *reg,
         {
               d("reloading ssa_%u:%u from %u", reg->instr->serialno, reg->name,
            unsigned elems = reg_elems(reg);
   struct ir3_instruction *reload =
         struct ir3_register *dst = __ssa_dst(reload);
   dst->flags |= reg->flags & (IR3_REG_HALF | IR3_REG_ARRAY);
   /* The reload may be split into multiple pieces, and if the destination
   * overlaps with the base register then it could get clobbered before the
   * last ldp in the sequence. Note that we always reserve space for the base
   * register throughout the whole program, so effectively extending its live
   * range past the end of the instruction isn't a problem for our pressure
   * accounting.
   */
   dst->flags |= IR3_REG_EARLY_CLOBBER;
   ir3_src_create(reload, INVALID_REG, ctx->base_reg->flags)->def = ctx->base_reg;
   struct ir3_register *offset_reg =
         offset_reg->uim_val = spill_slot;
   ir3_src_create(reload, INVALID_REG, IR3_REG_IMMED)->uim_val = elems;
            if (reg->flags & IR3_REG_ARRAY) {
      dst->array.offset = 0;
   dst->array.id = reg->array.id;
      } else {
                  dst->merge_set = reg->merge_set;
   dst->merge_set_offset = reg->merge_set_offset;
   dst->interval_start = reg->interval_start;
            if (after)
               }
      static void
   rewrite_src_interval(struct ra_spill_ctx *ctx,
                     struct ra_spill_interval *interval,
   {
      interval->dst.flags = def->flags;
   interval->dst.def = def;
            rb_tree_foreach (struct ra_spill_interval, child, 
            struct ir3_register *child_reg = child->interval.reg;
   struct ir3_register *child_def =
      extract(def, (child_reg->interval_start -
                  }
      static void
   reload_def(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      unsigned elems = reg_elems(def);
                     if (ir3_parent) {
      struct ra_spill_interval *parent =
         if (!parent->needs_reload) {
      interval->dst.flags = def->flags;
   interval->dst.def = extract(
      parent->dst.def, (def->interval_start - parent->dst.def->interval_start) /
                     struct ir3_register *dst;
   if (interval->can_rematerialize)
         else
               }
      static void
   reload_src(struct ra_spill_ctx *ctx, struct ir3_instruction *instr,
         {
               if (interval->needs_reload) {
                     }
      static void
   rewrite_src(struct ra_spill_ctx *ctx, struct ir3_instruction *instr,
         {
                  }
      static void
   update_max_pressure(struct ra_spill_ctx *ctx)
   {
      d("pressure:");
   d("\tfull: %u", ctx->cur_pressure.full);
   d("\thalf: %u", ctx->cur_pressure.half);
            ctx->max_pressure.full =
         ctx->max_pressure.half =
         ctx->max_pressure.shared =
      }
      static void
   handle_instr(struct ra_spill_ctx *ctx, struct ir3_instruction *instr)
   {
      ra_foreach_dst (dst, instr) {
                  if (ctx->spilling) {
      ra_foreach_src (src, instr)
               /* Handle tied and early-kill destinations. If a destination is tied to a
   * source and that source is live-through, then we need to allocate a new
   * register for the destination which is live-through itself and cannot
   * overlap the sources. Similarly early-kill destinations cannot overlap
   * sources.
            ra_foreach_dst (dst, instr) {
      struct ir3_register *tied_src = dst->tied;
   if ((tied_src && !(tied_src->flags & IR3_REG_FIRST_KILL)) ||
      (dst->flags & IR3_REG_EARLY_CLOBBER))
            if (ctx->spilling)
         else
            if (ctx->spilling) {
      ra_foreach_src (src, instr) {
      reload_src(ctx, instr, src);
                  ra_foreach_src (src, instr) {
      if (src->flags & IR3_REG_FIRST_KILL)
               ra_foreach_dst (dst, instr) {
                  if (ctx->spilling)
         else
            /* We have to remove sources before rewriting them so that we can lookup the
   * interval to remove before the source itself is changed.
   */
   ra_foreach_src (src, instr) {
      if (src->flags & IR3_REG_FIRST_KILL)
               if (ctx->spilling) {
      ra_foreach_src (src, instr) {
                     ra_foreach_dst (dst, instr) {
                  for (unsigned i = 0; i < instr->dsts_count; i++) {
      if (ra_reg_is_dst(instr->dsts[i]) &&
      (instr->dsts[i]->flags & IR3_REG_UNUSED))
      }
      static struct ra_spill_interval *
   create_temp_interval(struct ra_spill_ctx *ctx, struct ir3_register *def)
   {
      unsigned name = ctx->intervals_count++;
            /* This is kinda hacky, but we need to create a fake SSA def here that is
   * only used as part of the pcopy accounting. See below.
   */
   struct ir3_register *reg = rzalloc(ctx, struct ir3_register);
   *reg = *def;
   reg->name = name;
   reg->interval_start = offset;
   reg->interval_end = offset + reg_size(def);
            ctx->intervals = reralloc(ctx, ctx->intervals, struct ra_spill_interval *,
         struct ra_spill_interval *interval = rzalloc(ctx, struct ra_spill_interval);
   ra_spill_interval_init(interval, reg);
   ctx->intervals[name] = interval;
   ctx->live->interval_offset += reg_size(def);
      }
      /* In the sequence of copies generated (see below), would this source be killed?
   */
   static bool
   is_last_pcopy_src(struct ir3_instruction *pcopy, unsigned src_n)
   {
      struct ir3_register *src = pcopy->srcs[src_n];
   if (!(src->flags & IR3_REG_KILL))
         for (unsigned j = src_n + 1; j < pcopy->srcs_count; j++) {
      if (pcopy->srcs[j]->def == src->def)
      }
      }
      /* Parallel copies are different from normal instructions. The sources together
   * may be larger than the entire register file, so we cannot just reload every
   * source like normal, and indeed that probably wouldn't be a great idea.
   * Instead we essentially need to lower the parallel copy to "copies," just like
   * in the normal CSSA construction, although we implement the copies by
   * reloading and then possibly spilling values. We essentially just shuffle
   * around the sources until each source either (a) is live or (b) has the same
   * spill slot as its corresponding destination. We do this by decomposing the
   * copy into a series of copies, so:
   *
   * a, b, c = d, e, f
   *
   * becomes:
   *
   * d' = d
   * e' = e
   * f' = f
   * a = d'
   * b = e'
   * c = f'
   *
   * the temporary SSA values d', e', and f' never actually show up in the result.
   * They are only used for our internal accounting. They may, however, have their
   * own spill slot created for them. Similarly, we don't actually emit any copy
   * instructions, although we emit the spills/reloads that *would've* been
   * required if those copies were there.
   *
   * TODO: in order to reduce the number of temporaries and therefore spill slots,
   * we could instead do a more complicated analysis that considers the location
   * transfer graph.
   *
   * In addition, we actually remove the parallel copy and rewrite all its uses
   * (in the phi nodes) rather than rewrite its sources at the end. Recreating it
   * later turns out to be easier than keeping it up-to-date throughout this pass,
   * since we may have to remove entries for phi sources that are spilled and add
   * entries for live-outs that are spilled and reloaded, which can happen here
   * and then possibly be undone or done again when processing live-ins of the
   * successor block.
   */
      static void
   handle_pcopy(struct ra_spill_ctx *ctx, struct ir3_instruction *pcopy)
   {
      foreach_dst (dst, pcopy) {
      struct ra_spill_interval *dst_interval = ctx->intervals[dst->name];
               foreach_src_n (src, i, pcopy) {
      d("processing src %u", i);
            /* Skip the intermediate copy for cases where the source is merged with
   * the destination. Crucially this means that we also don't reload/spill
   * it if it's been spilled, because it shares the same spill slot.
   */
   if (src->def && src->def->merge_set &&
      src->def->merge_set == dst->merge_set &&
   src->def->merge_set_offset == dst->merge_set_offset) {
   struct ra_spill_interval *src_interval = ctx->intervals[src->def->name];
   struct ra_spill_interval *dst_interval = ctx->intervals[dst->name];
   if (src_interval->interval.inserted) {
      update_src_next_use(ctx, src);
   if (is_last_pcopy_src(pcopy, i))
         dst_interval->cant_spill = true;
   ra_spill_ctx_insert(ctx, dst_interval);
   limit(ctx, pcopy);
   dst_interval->cant_spill = false;
         } else if (src->def) {
      struct ra_spill_interval *temp_interval =
                        insert_src(ctx, src);
   limit(ctx, pcopy);
   reload_src(ctx, pcopy, src);
   update_src_next_use(ctx, src);
   if (is_last_pcopy_src(pcopy, i))
         struct ra_spill_interval *src_interval =
                  temp_interval->cant_spill = true;
   ra_spill_ctx_insert(ctx, temp_interval);
                  src->flags = temp->flags;
                           foreach_src_n (src, i, pcopy) {
               if (src->def && src->def->merge_set &&
      src->def->merge_set == dst->merge_set &&
                        if (!src->def) {
      dst_interval->cant_spill = true;
   ra_spill_ctx_insert(ctx, dst_interval);
                  assert(src->flags & (IR3_REG_CONST | IR3_REG_IMMED));
   if (src->flags & IR3_REG_CONST) {
      dst_interval->dst.flags = src->flags;
      } else {
      dst_interval->dst.flags = src->flags;
         } else {
               insert_src(ctx, src);
   limit(ctx, pcopy);
                  dst_interval->dst = temp_interval->dst;
                     }
      static void
   handle_input_phi(struct ra_spill_ctx *ctx, struct ir3_instruction *instr)
   {
      init_dst(ctx, instr->dsts[0]);
   insert_dst(ctx, instr->dsts[0]);
      }
      static void
   remove_input_phi(struct ra_spill_ctx *ctx, struct ir3_instruction *instr)
   {
      if (instr->opc == OPC_META_TEX_PREFETCH) {
      ra_foreach_src (src, instr)
      }
   if (instr->dsts[0]->flags & IR3_REG_UNUSED)
      }
      static void
   handle_live_in(struct ra_spill_ctx *ctx, struct ir3_block *block,
         {
      struct ra_spill_interval *interval = ctx->intervals[def->name];
   ra_spill_interval_init(interval, def);
   if (ctx->spilling) {
      interval->next_use_distance =
                  }
      static bool
   is_live_in_phi(struct ir3_register *def, struct ir3_block *block)
   {
         }
      static bool
   is_live_in_pred(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      struct ir3_block *pred = block->predecessors[pred_idx];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   if (is_live_in_phi(def, block)) {
      def = def->instr->srcs[pred_idx]->def;
   if (!def)
                  }
      static bool
   is_live_in_undef(struct ir3_register *def,
         {
      if (!is_live_in_phi(def, block))
               }
      static struct reg_or_immed *
   read_live_in(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      struct ir3_block *pred = block->predecessors[pred_idx];
            if (is_live_in_phi(def, block)) {
      def = def->instr->srcs[pred_idx]->def;
   if (!def)
               struct hash_entry *entry = _mesa_hash_table_search(state->remap, def);
   if (entry)
         else
      }
      static bool
   is_live_in_all_preds(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      for (unsigned i = 0; i < block->predecessors_count; i++) {
      if (!is_live_in_pred(ctx, def, block, i))
                  }
      static void
   spill_live_in(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
            if (!state->visited)
            struct reg_or_immed *pred_def = read_live_in(ctx, def, block, i);
   if (pred_def) {
               }
      static void
   spill_live_ins(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      bool all_preds_visited = true;
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   if (!state->visited) {
      all_preds_visited = false;
                  /* Note: in the paper they explicitly spill live-through values first, but we
   * should be doing that automatically by virtue of picking the largest
   * distance due to the extra distance added to edges out of loops.
   *
   * TODO: Keep track of pressure in each block and preemptively spill
   * live-through values as described in the paper to avoid spilling them
   * inside the loop.
            if (ctx->cur_pressure.half > ctx->limit_pressure.half) {
      rb_tree_foreach_safe (struct ra_spill_interval, interval,
            if (all_preds_visited &&
      is_live_in_all_preds(ctx, interval->interval.reg, block))
      if (interval->interval.reg->merge_set ||
      !interval->can_rematerialize)
      ir3_reg_interval_remove_all(&ctx->reg_ctx, &interval->interval);
   if (ctx->cur_pressure.half <= ctx->limit_pressure.half)
                  if (ctx->cur_pressure.full > ctx->limit_pressure.full) {
      rb_tree_foreach_safe (struct ra_spill_interval, interval,
            if (all_preds_visited &&
      is_live_in_all_preds(ctx, interval->interval.reg, block))
      spill_live_in(ctx, interval->interval.reg, block);
   ir3_reg_interval_remove_all(&ctx->reg_ctx, &interval->interval);
   if (ctx->cur_pressure.full <= ctx->limit_pressure.full)
            }
      static void
   live_in_rewrite(struct ra_spill_ctx *ctx,
                     {
      struct ir3_block *pred = block->predecessors[pred_idx];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   struct ir3_register *def = interval->interval.reg;
   if (is_live_in_phi(def, block)) {
                  if (def)
            rb_tree_foreach (struct ra_spill_interval, child,
            assert(new_val->flags & IR3_REG_SSA);
   struct ir3_register *child_def =
      extract(new_val->def,
         (child->interval.reg->interval_start - def->interval_start) /
      struct reg_or_immed *child_val = ralloc(ctx, struct reg_or_immed);
   child_val->def = child_def;
   child_val->flags = child_def->flags;
         }
      static void
   reload_live_in(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      struct ra_spill_interval *interval = ctx->intervals[def->name];
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   if (!state->visited)
            if (is_live_in_undef(def, block, i))
                     if (!new_val) {
      new_val = ralloc(ctx, struct reg_or_immed);
   if (interval->can_rematerialize)
         else
            }
         }
      static void
   reload_live_ins(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      rb_tree_foreach (struct ra_spill_interval, interval, &ctx->reg_ctx.intervals,
                  }
      static void
   add_live_in_phi(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      struct ra_spill_interval *interval = ctx->intervals[def->name];
   if (!interval->interval.inserted)
            bool needs_phi = false;
   struct ir3_register *cur_def = NULL;
   for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
            if (!state->visited) {
      needs_phi = true;
               struct hash_entry *entry =
         assert(entry);
   struct reg_or_immed *pred_val = entry->data;
   if ((pred_val->flags & (IR3_REG_IMMED | IR3_REG_CONST)) ||
      !pred_val->def ||
   (cur_def && cur_def != pred_val->def)) {
   needs_phi = true;
      }
               if (!needs_phi) {
      interval->dst.def = cur_def;
   interval->dst.flags = cur_def->flags;
               struct ir3_instruction *phi =
         struct ir3_register *dst = __ssa_dst(phi);
   dst->flags |= def->flags & (IR3_REG_HALF | IR3_REG_ARRAY);
   dst->size = def->size;
            dst->interval_start = def->interval_start;
   dst->interval_end = def->interval_end;
   dst->merge_set = def->merge_set;
            for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   struct ir3_register *src = ir3_src_create(phi, INVALID_REG, dst->flags);
   src->size = def->size;
            if (state->visited) {
      struct hash_entry *entry =
         assert(entry);
   struct reg_or_immed *new_val = entry->data;
      } else {
                     interval->dst.def = dst;
               }
      /* When spilling a block with a single predecessors, the pred may have other
   * successors so we can't choose what's live in and we can't spill/restore
   * anything. Just make the inserted intervals exactly match the predecessor. If
   * it wasn't live in the predecessor then it must've already been spilled. Also,
   * there are no phi nodes and no live-ins.
   */
   static void
   spill_single_pred_live_in(struct ra_spill_ctx *ctx,
         {
      unsigned name;
   BITSET_FOREACH_SET (name, ctx->live->live_in[block->index],
            struct ir3_register *reg = ctx->live->definitions[name];
   struct ra_spill_interval *interval = ctx->intervals[reg->name];
   struct reg_or_immed *val = read_live_in(ctx, reg, block, 0);
   if (val)
         else
         }
      static void
   rewrite_phi(struct ra_spill_ctx *ctx, struct ir3_instruction *phi,
         {
      if (!ctx->intervals[phi->dsts[0]->name]->interval.inserted) {
      phi->flags |= IR3_INSTR_UNUSED;
               for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
            if (!state->visited)
            struct ir3_register *src = phi->srcs[i];
   if (!src->def)
            struct hash_entry *entry =
         assert(entry);
   struct reg_or_immed *new_val = entry->data;
         }
      static void
   spill_live_out(struct ra_spill_ctx *ctx, struct ra_spill_interval *interval,
         {
               if (interval->interval.reg->merge_set ||
      !interval->can_rematerialize)
         }
      static void
   spill_live_outs(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      struct ra_spill_block_state *state = &ctx->blocks[block->index];
   rb_tree_foreach_safe (struct ra_spill_interval, interval,
            if (!BITSET_TEST(state->live_out, interval->interval.reg->name)) {
               }
      static void
   reload_live_out(struct ra_spill_ctx *ctx, struct ir3_register *def,
         {
      struct ra_spill_interval *interval = ctx->intervals[def->name];
               }
      static void
   reload_live_outs(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      struct ra_spill_block_state *state = &ctx->blocks[block->index];
   unsigned name;
   BITSET_FOREACH_SET (name, state->live_out, ctx->live->definitions_count) {
      struct ir3_register *reg = ctx->live->definitions[name];
   struct ra_spill_interval *interval = ctx->intervals[name];
   if (!interval->interval.inserted)
         }
      static void
   update_live_out_phis(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      assert(!block->successors[1]);
   struct ir3_block *succ = block->successors[0];
   unsigned pred_idx = ir3_block_get_pred_index(succ, block);
      foreach_instr (instr, &succ->instr_list) {
      if (instr->opc != OPC_META_PHI)
            struct ir3_register *def = instr->srcs[pred_idx]->def;
   if (!def)
            struct ra_spill_interval *interval = ctx->intervals[def->name];
   if (!interval->interval.inserted)
               }
      static void
   record_pred_live_out(struct ra_spill_ctx *ctx,
               {
      struct ir3_block *pred = block->predecessors[pred_idx];
            struct ir3_register *def = interval->interval.reg;
   if (is_live_in_phi(def, block)) {
         }
            rb_tree_foreach (struct ra_spill_interval, child,
                  }
      static void
   record_pred_live_outs(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   struct ra_spill_block_state *state = &ctx->blocks[pred->index];
   if (state->visited)
            state->live_out = rzalloc_array(ctx, BITSET_WORD,
               rb_tree_foreach (struct ra_spill_interval, interval,
                     }
      static void
   record_live_out(struct ra_spill_ctx *ctx,
               {
      if (!(interval->dst.flags & IR3_REG_SSA) ||
      interval->dst.def) {
   struct reg_or_immed *val = ralloc(ctx, struct reg_or_immed);
   *val = interval->dst;
      }
   rb_tree_foreach (struct ra_spill_interval, child,
                  }
      static void
   record_live_outs(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      struct ra_spill_block_state *state = &ctx->blocks[block->index];
            rb_tree_foreach (struct ra_spill_interval, interval, &ctx->reg_ctx.intervals,
                  }
      static void
   handle_block(struct ra_spill_ctx *ctx, struct ir3_block *block)
   {
      memset(&ctx->cur_pressure, 0, sizeof(ctx->cur_pressure));
   rb_tree_init(&ctx->reg_ctx.intervals);
   rb_tree_init(&ctx->full_live_intervals);
            unsigned name;
   BITSET_FOREACH_SET (name, ctx->live->live_in[block->index],
            struct ir3_register *reg = ctx->live->definitions[name];
               foreach_instr (instr, &block->instr_list) {
      if (instr->opc != OPC_META_PHI && instr->opc != OPC_META_INPUT &&
      instr->opc != OPC_META_TEX_PREFETCH)
                  if (ctx->spilling) {
      if (block->predecessors_count == 1) {
         } else {
      spill_live_ins(ctx, block);
   reload_live_ins(ctx, block);
   record_pred_live_outs(ctx, block);
   foreach_instr (instr, &block->instr_list) {
      if (instr->opc != OPC_META_PHI)
            }
   BITSET_FOREACH_SET (name, ctx->live->live_in[block->index],
            struct ir3_register *reg = ctx->live->definitions[name];
            } else {
                  foreach_instr (instr, &block->instr_list) {
               if (instr->opc == OPC_META_PHI || instr->opc == OPC_META_INPUT ||
      instr->opc == OPC_META_TEX_PREFETCH)
      else if (ctx->spilling && instr->opc == OPC_META_PARALLEL_COPY)
         else if (ctx->spilling && instr->opc == OPC_MOV &&
               else
               if (ctx->spilling && block->successors[0]) {
      struct ra_spill_block_state *state =
         if (state->visited) {
               spill_live_outs(ctx, block);
   reload_live_outs(ctx, block);
                  if (ctx->spilling) {
      record_live_outs(ctx, block);
         }
      static bool
   simplify_phi_node(struct ir3_instruction *phi)
   {
      struct ir3_register *def = NULL;
   foreach_src (src, phi) {
      /* Ignore phi sources which point to the phi itself. */
   if (src->def == phi->dsts[0])
         /* If it's undef or it doesn't match the previous sources, bail */
   if (!src->def || (def && def != src->def))
                     phi->data = def;
   phi->flags |= IR3_INSTR_UNUSED;
      }
      static struct ir3_register *
   simplify_phi_def(struct ir3_register *def)
   {
      if (def->instr->opc == OPC_META_PHI) {
               /* Note: this function is always called at least once after visiting the
   * phi, so either there has been a simplified phi in the meantime, in
   * which case we will set progress=true and visit the definition again, or
   * phi->data already has the most up-to-date value. Therefore we don't
   * have to recursively check phi->data.
   */
   if (phi->data)
                  }
      static void
   simplify_phi_srcs(struct ir3_instruction *instr)
   {
      foreach_src (src, instr) {
      if (src->def)
         }
      /* We insert phi nodes for all live-ins of loops in case we need to split the
   * live range. This pass cleans that up for the case where the live range didn't
   * actually need to be split.
   */
   static void
   simplify_phi_nodes(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (instr->opc != OPC_META_PHI)
                        bool progress;
   do {
      progress = false;
   foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
                                 /* Visit phi nodes in the successors to make sure that phi sources are
   * always visited at least once after visiting the definition they
   * point to. See note in simplify_phi_def() for why this is necessary.
   */
   for (unsigned i = 0; i < 2; i++) {
      struct ir3_block *succ = block->successors[i];
   if (!succ)
         foreach_instr (instr, &succ->instr_list) {
      if (instr->opc != OPC_META_PHI)
         if (instr->flags & IR3_INSTR_UNUSED) {
      if (instr->data)
      } else {
      simplify_phi_srcs(instr);
                     }
      static void
   unmark_dead(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
               }
      /* Simple pass to remove now-dead phi nodes and pcopy instructions. We mark
   * which ones are dead along the way, so there's nothing to compute here.
   */
   static void
   cleanup_dead(struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      foreach_instr_safe (instr, &block->instr_list) {
      if (instr->flags & IR3_INSTR_UNUSED)
            }
      /* Deal with merge sets after spilling. Spilling generally leaves the merge sets
   * in a mess, and even if we properly cleaned up after ourselves, we would want
   * to recompute the merge sets afterward anway. That's because
   * spilling/reloading can "break up" phi webs and split/collect webs so that
   * allocating them to the same register no longer gives any benefit. For
   * example, imagine we have this:
   *
   * if (...) {
   *    foo = ...
   * } else {
   *    bar = ...
   * }
   * baz = phi(foo, bar)
   *
   * and we spill "baz":
   *
   * if (...) {
   *    foo = ...
   *    spill(foo)
   * } else {
   *    bar = ...
   *    spill(bar)
   * }
   * baz = reload()
   *
   * now foo, bar, and baz don't have to be allocated to the same register. How
   * exactly the merge sets change can be complicated, so it's easier just to
   * recompute them.
   *
   * However, there's a wrinkle in this: those same merge sets determine the
   * register pressure, due to multiple values inhabiting the same register! And
   * we assume that this sharing happens when spilling. Therefore we need a
   * three-step procedure:
   *
   * 1. Drop the original merge sets.
   * 2. Calculate which values *must* be merged, being careful to only use the
   *    interval information which isn't trashed by spilling, and forcibly merge
   *    them.
   * 3. Let ir3_merge_regs() finish the job, including recalculating the
   *    intervals.
   */
      static void
   fixup_merge_sets(struct ir3_liveness *live, struct ir3 *ir)
   {
      foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      ra_foreach_dst (dst, instr) {
      dst->merge_set = NULL;
                     foreach_block (block, &ir->block_list) {
      foreach_instr (instr, &block->instr_list) {
      if (instr->opc != OPC_META_SPLIT &&
                  struct ir3_register *dst = instr->dsts[0];
   ra_foreach_src (src, instr) {
      if (!(src->flags & IR3_REG_KILL) &&
      src->def->interval_start < dst->interval_end &&
   dst->interval_start < src->def->interval_end) {
   ir3_force_merge(dst, src->def,
                           }
      void
   ir3_calc_pressure(struct ir3_shader_variant *v, struct ir3_liveness *live,
         {
      struct ra_spill_ctx *ctx = rzalloc(NULL, struct ra_spill_ctx);
            foreach_block (block, &v->ir->block_list) {
                  assert(ctx->cur_pressure.full == 0);
   assert(ctx->cur_pressure.half == 0);
            *max_pressure = ctx->max_pressure;
      }
      bool
   ir3_spill(struct ir3 *ir, struct ir3_shader_variant *v,
               {
      void *mem_ctx = ralloc_parent(*live);
   struct ra_spill_ctx *ctx = rzalloc(mem_ctx, struct ra_spill_ctx);
                     ctx->blocks = rzalloc_array(ctx, struct ra_spill_block_state,
         rb_tree_init(&ctx->full_live_intervals);
            ctx->limit_pressure = *limit_pressure;
            add_base_reg(ctx, ir);
                     foreach_block (block, &ir->block_list) {
                                             /* After this point, we're done mutating the IR. Liveness has been trashed,
   * so recalculate it. We'll need it for recalculating the merge sets.
   */
   ralloc_free(ctx->live);
                     v->pvtmem_size = ctx->spill_slot;
               }
