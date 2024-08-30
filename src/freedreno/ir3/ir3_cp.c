   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include <math.h>
   #include "util/half_float.h"
   #include "util/u_math.h"
      #include "ir3.h"
   #include "ir3_compiler.h"
   #include "ir3_shader.h"
      #define swap(a, b)                                                             \
      do {                                                                        \
      __typeof(a) __tmp = (a);                                                 \
   (a) = (b);                                                               \
            /*
   * Copy Propagate:
   */
      struct ir3_cp_ctx {
      struct ir3 *shader;
   struct ir3_shader_variant *so;
      };
      /* is it a type preserving mov, with ok flags?
   *
   * @instr: the mov to consider removing
   * @dst_instr: the instruction consuming the mov (instr)
   *
   * TODO maybe drop allow_flags since this is only false when dst is
   * NULL (ie. outputs)
   */
   static bool
   is_eligible_mov(struct ir3_instruction *instr,
         {
      if (is_same_type_mov(instr)) {
      struct ir3_register *dst = instr->dsts[0];
   struct ir3_register *src = instr->srcs[0];
            /* only if mov src is SSA (not const/immed): */
   if (!src_instr)
            /* no indirect: */
   if (dst->flags & IR3_REG_RELATIV)
         if (src->flags & IR3_REG_RELATIV)
            if (src->flags & IR3_REG_ARRAY)
            if (!allow_flags)
      if (src->flags & (IR3_REG_FABS | IR3_REG_FNEG | IR3_REG_SABS |
                  }
      }
      /* we can end up with extra cmps.s from frontend, which uses a
   *
   *    cmps.s p0.x, cond, 0
   *
   * as a way to mov into the predicate register.  But frequently 'cond'
   * is itself a cmps.s/cmps.f/cmps.u. So detect this special case.
   */
   static bool
   is_foldable_double_cmp(struct ir3_instruction *cmp)
   {
      struct ir3_instruction *cond = ssa(cmp->srcs[0]);
   return (cmp->dsts[0]->num == regid(REG_P0, 0)) && cond &&
         (cmp->srcs[1]->flags & IR3_REG_IMMED) &&
   (cmp->srcs[1]->iim_val == 0) &&
      }
      /* propagate register flags from src to dst.. negates need special
   * handling to cancel each other out.
   */
   static void
   combine_flags(unsigned *dstflags, struct ir3_instruction *src)
   {
               /* if what we are combining into already has (abs) flags,
   * we can drop (neg) from src:
   */
   if (*dstflags & IR3_REG_FABS)
         if (*dstflags & IR3_REG_SABS)
            if (srcflags & IR3_REG_FABS)
         if (srcflags & IR3_REG_SABS)
         if (srcflags & IR3_REG_FNEG)
         if (srcflags & IR3_REG_SNEG)
         if (srcflags & IR3_REG_BNOT)
            *dstflags &= ~IR3_REG_SSA;
   *dstflags |= srcflags & IR3_REG_SSA;
   *dstflags |= srcflags & IR3_REG_CONST;
   *dstflags |= srcflags & IR3_REG_IMMED;
   *dstflags |= srcflags & IR3_REG_RELATIV;
   *dstflags |= srcflags & IR3_REG_ARRAY;
            /* if src of the src is boolean we can drop the (abs) since we know
   * the source value is already a postitive integer.  This cleans
   * up the absnegs that get inserted when converting between nir and
   * native boolean (see ir3_b2n/n2b)
   */
   struct ir3_instruction *srcsrc = ssa(src->srcs[0]);
   if (srcsrc && is_bool(srcsrc))
      }
      /* Tries lowering an immediate register argument to a const buffer access by
   * adding to the list of immediates to be pushed to the const buffer when
   * switching to this shader.
   */
   static bool
   lower_immed(struct ir3_cp_ctx *ctx, struct ir3_instruction *instr, unsigned n,
         {
      if (!(new_flags & IR3_REG_IMMED))
            new_flags &= ~IR3_REG_IMMED;
            if (!ir3_valid_flags(instr, n, new_flags))
                     /* Half constant registers seems to handle only 32-bit values
   * within floating-point opcodes. So convert back to 32-bit values.
   */
   bool f_opcode =
         if (f_opcode && (new_flags & IR3_REG_HALF))
            /* in some cases, there are restrictions on (abs)/(neg) plus const..
   * so just evaluate those and clear the flags:
   */
   if (new_flags & IR3_REG_SABS) {
      reg->iim_val = abs(reg->iim_val);
               if (new_flags & IR3_REG_FABS) {
      reg->fim_val = fabs(reg->fim_val);
               if (new_flags & IR3_REG_SNEG) {
      reg->iim_val = -reg->iim_val;
               if (new_flags & IR3_REG_FNEG) {
      reg->fim_val = -reg->fim_val;
               /* Reallocate for 4 more elements whenever it's necessary.  Note that ir3
   * printing relies on having groups of 4 dwords, so we fill the unused
   * slots with a dummy value.
   */
   struct ir3_const_state *const_state = ir3_const_state(ctx->so);
   if (const_state->immediates_count == const_state->immediates_size) {
      const_state->immediates = rerzalloc(
      const_state, const_state->immediates,
   __typeof__(const_state->immediates[0]), const_state->immediates_size,
               for (int i = const_state->immediates_count;
      i < const_state->immediates_size; i++)
            int i;
   for (i = 0; i < const_state->immediates_count; i++) {
      if (const_state->immediates[i] == reg->uim_val)
               if (i == const_state->immediates_count) {
      /* Add on a new immediate to be pushed, if we have space left in the
   * constbuf.
   */
   if (const_state->offsets.immediate + const_state->immediates_count / 4 >=
                  const_state->immediates[i] = reg->uim_val;
               reg->flags = new_flags;
                        }
      static void
   unuse(struct ir3_instruction *instr)
   {
               if (--instr->use_count == 0) {
               instr->barrier_class = 0;
            /* we don't want to remove anything in keeps (which could
   * be things like array store's)
   */
   for (unsigned i = 0; i < block->keeps_count; i++) {
               }
      /**
   * Handles the special case of the 2nd src (n == 1) to "normal" mad
   * instructions, which cannot reference a constant.  See if it is
   * possible to swap the 1st and 2nd sources.
   */
   static bool
   try_swap_mad_two_srcs(struct ir3_instruction *instr, unsigned new_flags)
   {
      if (!is_mad(instr->opc))
            /* If we've already tried, nothing more to gain.. we will only
   * have previously swapped if the original 2nd src was const or
   * immed.  So swapping back won't improve anything and could
   * result in an infinite "progress" loop.
   */
   if (instr->cat3.swapped)
            /* cat3 doesn't encode immediate, but we can lower immediate
   * to const if that helps:
   */
   if (new_flags & IR3_REG_IMMED) {
      new_flags &= ~IR3_REG_IMMED;
               /* If the reason we couldn't fold without swapping is something
   * other than const source, then swapping won't help:
   */
   if (!(new_flags & IR3_REG_CONST))
                     /* NOTE: pre-swap first two src's before valid_flags(),
   * which might try to dereference the n'th src:
   */
            bool valid_swap =
      /* can we propagate mov if we move 2nd src to first? */
   ir3_valid_flags(instr, 0, new_flags) &&
   /* and does first src fit in second slot? */
         if (!valid_swap) {
      /* put things back the way they were: */
                  }
      /* Values that are uniform inside a loop can become divergent outside
   * it if the loop has a divergent trip count. This means that we can't
   * propagate a copy of a shared to non-shared register if it would
   * make the shared reg's live range extend outside of its loop. Users
   * outside the loop would see the value for the thread(s) that last
   * exited the loop, rather than for their own thread.
   */
   static bool
   is_valid_shared_copy(struct ir3_instruction *dst_instr,
               {
      return !(src_reg->flags & IR3_REG_SHARED) ||
      }
      /**
   * Handle cp for a given src register.  This additionally handles
   * the cases of collapsing immedate/const (which replace the src
   * register with a non-ssa src) or collapsing mov's from relative
   * src (which needs to also fixup the address src reference by the
   * instruction).
   */
   static bool
   reg_cp(struct ir3_cp_ctx *ctx, struct ir3_instruction *instr,
         {
               if (is_eligible_mov(src, instr, true)) {
      /* simple case, no immed/const/relativ, only mov's w/ ssa src: */
   struct ir3_register *src_reg = src->srcs[0];
            if (!is_valid_shared_copy(instr, src, src_reg))
                     if (ir3_valid_flags(instr, n, new_flags)) {
      if (new_flags & IR3_REG_ARRAY) {
      assert(!(reg->flags & IR3_REG_ARRAY));
      }
                                                      } else if ((is_same_type_mov(src) || is_const_mov(src)) &&
            /* cannot collapse const/immed/etc into control flow: */
   /* immed/const/etc cases, which require some special handling: */
   struct ir3_register *src_reg = src->srcs[0];
            if (!is_valid_shared_copy(instr, src, src_reg))
            if (src_reg->flags & IR3_REG_ARRAY)
                     if (!ir3_valid_flags(instr, n, new_flags)) {
      /* See if lowering an immediate to const would help. */
                  /* special case for "normal" mad instructions, we can
   * try swapping the first two args if that fits better.
   *
   * the "plain" MAD's (ie. the ones that don't shift first
   * src prior to multiply) can swap their first two srcs if
   * src[0] is !CONST and src[1] is CONST:
   */
   if ((n == 1) && try_swap_mad_two_srcs(instr, new_flags)) {
         } else {
                     /* Here we handle the special case of mov from
   * CONST and/or RELATIV.  These need to be handled
   * specially, because in the case of move from CONST
   * there is no src ir3_instruction so we need to
   * replace the ir3_register.  And in the case of
   * RELATIV we need to handle the address register
   * dependency.
   */
   if (src_reg->flags & IR3_REG_CONST) {
      /* an instruction cannot reference two different
   * address registers:
   */
   if ((src_reg->flags & IR3_REG_RELATIV) &&
                  /* These macros expand to a mov in an if statement */
   if ((src_reg->flags & IR3_REG_RELATIV) &&
                  /* This seems to be a hw bug, or something where the timings
   * just somehow don't work out.  This restriction may only
   * apply if the first src is also CONST.
   */
   if ((opc_cat(instr->opc) == 3) && (n == 2) &&
                  /* When narrowing constant from 32b to 16b, it seems
   * to work only for float. So we should do this only with
   * float opcodes.
   */
   if (src->cat1.dst_type == TYPE_F16) {
      /* TODO: should we have a way to tell phi/collect to use a
   * float move so that this is legal?
   */
   if (is_meta(instr))
         if (instr->opc == OPC_MOV && !type_float(instr->cat1.src_type))
         if (!is_cat2_float(instr->opc) && !is_cat3_float(instr->opc))
      } else if (src->cat1.dst_type == TYPE_U16) {
      /* Since we set CONSTANT_DEMOTION_ENABLE, a float reference of
   * what was a U16 value read from the constbuf would incorrectly
   * do 32f->16f conversion, when we want to read a 16f value.
   */
   if (is_cat2_float(instr->opc) || is_cat3_float(instr->opc))
         if (instr->opc == OPC_MOV && type_float(instr->cat1.src_type))
               src_reg = ir3_reg_clone(instr->block->shader, src_reg);
                                             if (src_reg->flags & IR3_REG_IMMED) {
               assert((opc_cat(instr->opc) == 1) ||
                              if ((opc_cat(instr->opc) == 2) &&
            iim_val = ir3_flut(src_reg);
   if (iim_val < 0) {
      /* Fall back to trying to load the immediate as a const: */
                                                               if (ir3_valid_flags(instr, n, new_flags) &&
      ir3_valid_immediate(instr, iim_val)) {
   new_flags &= ~(IR3_REG_SABS | IR3_REG_SNEG | IR3_REG_BNOT);
   src_reg = ir3_reg_clone(instr->block->shader, src_reg);
   src_reg->flags = new_flags;
                     } else {
      /* Fall back to trying to load the immediate as a const: */
                        }
      /* Handle special case of eliminating output mov, and similar cases where
   * there isn't a normal "consuming" instruction.  In this case we cannot
   * collapse flags (ie. output mov from const, or w/ abs/neg flags, cannot
   * be eliminated)
   */
   static struct ir3_instruction *
   eliminate_output_mov(struct ir3_cp_ctx *ctx, struct ir3_instruction *instr)
   {
      if (is_eligible_mov(instr, NULL, false)) {
      struct ir3_register *reg = instr->srcs[0];
   if (!(reg->flags & IR3_REG_ARRAY)) {
      struct ir3_instruction *src_instr = ssa(reg);
   assert(src_instr);
   ctx->progress = true;
         }
      }
      /**
   * Find instruction src's which are mov's that can be collapsed, replacing
   * the mov dst with the mov src
   */
   static void
   instr_cp(struct ir3_cp_ctx *ctx, struct ir3_instruction *instr)
   {
      if (instr->srcs_count == 0)
            if (ir3_instr_check_mark(instr))
            /* walk down the graph from each src: */
   bool progress;
   do {
      progress = false;
   foreach_src_n (reg, n, instr) {
                                       /* TODO non-indirect access we could figure out which register
   * we actually want and allow cp..
   */
                  /* Don't CP absneg into meta instructions, that won't end well: */
   if (is_meta(instr) &&
                  /* Don't CP mova and mova1 into their users */
                  progress |= reg_cp(ctx, instr, reg, n);
                  /* After folding a mov's source we may wind up with a type-converting mov
   * of an immediate. This happens e.g. with texture descriptors, since we
   * narrow the descriptor (which may be a constant) to a half-reg in ir3.
   * By converting the immediate in-place to the destination type, we can
   * turn the mov into a same-type mov so that it can be further propagated.
   */
   if (instr->opc == OPC_MOV && (instr->srcs[0]->flags & IR3_REG_IMMED) &&
      instr->cat1.src_type != instr->cat1.dst_type &&
   /* Only do uint types for now, until we generate other types of
   * mov's during instruction selection.
   */
   full_type(instr->cat1.src_type) == TYPE_U32 &&
   full_type(instr->cat1.dst_type) == TYPE_U32) {
   uint32_t uimm = instr->srcs[0]->uim_val;
   if (instr->cat1.dst_type == TYPE_U16)
         instr->srcs[0]->uim_val = uimm;
   if (instr->dsts[0]->flags & IR3_REG_HALF)
         else
         instr->cat1.src_type = instr->cat1.dst_type;
               /* Re-write the instruction writing predicate register to get rid
   * of the double cmps.
   */
   if ((instr->opc == OPC_CMPS_S) && is_foldable_double_cmp(instr)) {
      struct ir3_instruction *cond = ssa(instr->srcs[0]);
   switch (cond->opc) {
   case OPC_CMPS_S:
   case OPC_CMPS_F:
   case OPC_CMPS_U:
      instr->opc = cond->opc;
   instr->flags = cond->flags;
   instr->cat2 = cond->cat2;
   if (cond->address)
         instr->srcs[0] = ir3_reg_clone(ctx->shader, cond->srcs[0]);
   instr->srcs[1] = ir3_reg_clone(ctx->shader, cond->srcs[1]);
   instr->barrier_class |= cond->barrier_class;
   instr->barrier_conflict |= cond->barrier_conflict;
   unuse(cond);
   ctx->progress = true;
      default:
                     /* Handle converting a sam.s2en (taking samp/tex idx params via register)
   * into a normal sam (encoding immediate samp/tex idx) if they are
   * immediate. This saves some instructions and regs in the common case
   * where we know samp/tex at compile time. This needs to be done in the
   * frontend for bindless tex, though, so don't replicate it here.
   */
   if (is_tex(instr) && (instr->flags & IR3_INSTR_S2EN) &&
      !(instr->flags & IR3_INSTR_B) &&
   !(ir3_shader_debug & IR3_DBG_FORCES2EN)) {
   /* The first src will be a collect, if both of it's
   * two sources are mov from imm, then we can
   */
                     struct ir3_register *samp = samp_tex->srcs[0];
            if ((samp->flags & IR3_REG_IMMED) && (tex->flags & IR3_REG_IMMED) &&
      (samp->iim_val < 16) && (tex->iim_val < 16)) {
   instr->flags &= ~IR3_INSTR_S2EN;
                  /* shuffle around the regs to remove the first src: */
   instr->srcs_count--;
   for (unsigned i = 0; i < instr->srcs_count; i++) {
                           }
      bool
   ir3_cp(struct ir3 *ir, struct ir3_shader_variant *so)
   {
      struct ir3_cp_ctx ctx = {
      .shader = ir,
               /* This is a bit annoying, and probably wouldn't be necessary if we
   * tracked a reverse link from producing instruction to consumer.
   * But we need to know when we've eliminated the last consumer of
   * a mov, so we need to do a pass to first count consumers of a
   * mov.
   */
   foreach_block (block, &ir->block_list) {
                  /* by the way, we don't account for false-dep's, so the CP
   * pass should always happen before false-dep's are inserted
                  foreach_ssa_src (src, instr) {
                                 foreach_block (block, &ir->block_list) {
      if (block->condition) {
      instr_cp(&ctx, block->condition);
               for (unsigned i = 0; i < block->keeps_count; i++) {
      instr_cp(&ctx, block->keeps[i]);
                     }
