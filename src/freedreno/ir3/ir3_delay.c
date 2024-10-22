   /*
   * Copyright (C) 2019 Google, Inc.
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
      #include "ir3.h"
      /* The maximum number of nop's we may need to insert between two instructions.
   */
   #define MAX_NOPS 6
      /*
   * Helpers to figure out the necessary delay slots between instructions.  Used
   * both in scheduling pass(es) and the final pass to insert any required nop's
   * so that the shader program is valid.
   *
   * Note that this needs to work both pre and post RA, so we can't assume ssa
   * src iterators work.
   */
      /* calculate required # of delay slots between the instruction that
   * assigns a value and the one that consumes
   */
   int
   ir3_delayslots(struct ir3_instruction *assigner,
         {
      /* generally don't count false dependencies, since this can just be
   * something like a barrier, or SSBO store.
   */
   if (__is_false_dep(consumer, n))
            /* worst case is cat1-3 (alu) -> cat4/5 needing 6 cycles, normal
   * alu -> alu needs 3 cycles, cat4 -> alu and texture fetch
   * handled with sync bits
            if (is_meta(assigner) || is_meta(consumer))
            if (writes_addr0(assigner) || writes_addr1(assigner))
            if (soft && is_ss_producer(assigner))
            /* handled via sync flags: */
   if (is_ss_producer(assigner) || is_sy_producer(assigner))
            /* As far as we know, shader outputs don't need any delay. */
   if (consumer->opc == OPC_END || consumer->opc == OPC_CHMASK)
            /* assigner must be alu: */
   if (is_flow(consumer) || is_sfu(consumer) || is_tex(consumer) ||
      is_mem(consumer)) {
      } else {
      /* In mergedregs mode, there is an extra 2-cycle penalty when half of
   * a full-reg is read as a half-reg or when a half-reg is read as a
   * full-reg.
   */
   bool mismatched_half = (assigner->dsts[0]->flags & IR3_REG_HALF) !=
         unsigned penalty = mismatched_half ? 3 : 0;
   if ((is_mad(consumer->opc) || is_madsh(consumer->opc)) && (n == 2)) {
      /* special case, 3rd src to cat3 not required on first cycle */
      } else {
               }
      static bool
   count_instruction(struct ir3_instruction *n)
   {
      /* NOTE: don't count branch/jump since we don't know yet if they will
   * be eliminated later in resolve_jumps().. really should do that
   * earlier so we don't have this constraint.
   */
   return is_alu(n) ||
      }
      /* Post-RA, we don't have arrays any more, so we have to be a bit careful here
   * and have to handle relative accesses specially.
   */
      static unsigned
   post_ra_reg_elems(struct ir3_register *reg)
   {
      if (reg->flags & IR3_REG_RELATIV)
            }
      static unsigned
   post_ra_reg_num(struct ir3_register *reg)
   {
      if (reg->flags & IR3_REG_RELATIV)
            }
      unsigned
   ir3_delayslots_with_repeat(struct ir3_instruction *assigner,
               {
               struct ir3_register *src = consumer->srcs[consumer_n];
            if (assigner->repeat == 0 && consumer->repeat == 0)
            unsigned src_start = post_ra_reg_num(src) * reg_elem_size(src);
            /* If either side is a relative access, we can't really apply most of the
   * reasoning below because we don't know which component aliases which.
   * Just bail in this case.
   */
   if ((src->flags & IR3_REG_RELATIV) || (dst->flags & IR3_REG_RELATIV))
            /* MOVMSK seems to require that all users wait until the entire
   * instruction is finished, so just bail here.
   */
   if (assigner->opc == OPC_MOVMSK)
            bool mismatched_half =
            /* TODO: Handle the combination of (rpt) and different component sizes
   * better like below. This complicates things significantly because the
   * components don't line up.
   */
   if (mismatched_half)
            /* If an instruction has a (rpt), then it acts as a sequence of
   * instructions, reading its non-(r) sources at each cycle. First, get the
   * register num for the first instruction where they interfere:
                     /* Now, for that first conflicting half/full register, figure out the
   * sub-instruction within assigner/consumer it corresponds to. For (r)
   * sources, this should already return the correct answer of 0. However we
   * have to special-case the multi-mov instructions, where the
   * sub-instructions sometimes come from the src/dst indices instead.
   */
   unsigned first_src_instr;
   if (consumer->opc == OPC_SWZ || consumer->opc == OPC_GAT)
         else
            unsigned first_dst_instr;
   if (assigner->opc == OPC_SWZ || assigner->opc == OPC_SCT)
         else
            /* The delay we return is relative to the *end* of assigner and the
   * *beginning* of consumer, because it's the number of nops (or other
   * things) needed between them. Any instructions after first_dst_instr
   * subtract from the delay, and so do any instructions before
   * first_src_instr. Calculate an offset to subtract from the non-rpt-aware
   * delay to account for that.
   *
   * Now, a priori, we need to go through this process for every
   * conflicting regnum and take the minimum of the offsets to make sure
   * that the appropriate number of nop's is inserted for every conflicting
   * pair of sub-instructions. However, as we go to the next conflicting
   * regnum (if any), the number of instructions after first_dst_instr
   * decreases by 1 and the number of source instructions before
   * first_src_instr correspondingly increases by 1, so the offset stays the
   * same for all conflicting registers.
   */
   unsigned offset = first_src_instr + (assigner->repeat - first_dst_instr);
      }
      static unsigned
   delay_calc_srcn(struct ir3_instruction *assigner,
               {
      struct ir3_register *src = consumer->srcs[consumer_n];
   struct ir3_register *dst = assigner->dsts[assigner_n];
   bool mismatched_half =
            /* In the mergedregs case or when the register is a special register,
   * half-registers do not alias with full registers.
   */
   if ((!mergedregs || is_reg_special(src) || is_reg_special(dst)) &&
      mismatched_half)
         unsigned src_start = post_ra_reg_num(src) * reg_elem_size(src);
   unsigned src_end = src_start + post_ra_reg_elems(src) * reg_elem_size(src);
   unsigned dst_start = post_ra_reg_num(dst) * reg_elem_size(dst);
            if (dst_start >= src_end || src_start >= dst_end)
               }
      static unsigned
   delay_calc(struct ir3_block *block, struct ir3_instruction *start,
               {
      regmask_t mask;
            unsigned delay = 0;
   /* Search backwards starting at the instruction before start, unless it's
   * NULL then search backwards from the block end.
   */
   struct list_head *start_list =
         list_for_each_entry_from_rev (struct ir3_instruction, assigner, start_list,
            if (count_instruction(assigner))
            if (distance + delay >= MAX_NOPS)
            if (is_meta(assigner))
                     foreach_dst_n (dst, dst_n, assigner) {
      if (dst->wrmask == 0)
         if (!regmask_get(&mask, dst))
         foreach_src_n (src, src_n, consumer) {
                     unsigned src_delay = delay_calc_srcn(
            }
               new_delay = new_delay > distance ? new_delay - distance : 0;
            if (count_instruction(assigner))
               /* Note: this allows recursion into "block" if it has already been
   * visited, but *not* recursion into its predecessors. We may have to
   * visit the original block twice, for the loop case where we have to
   * consider definititons in an earlier iterations of the same loop:
   *
   * while (...) {
   *		mov.u32u32 ..., r0.x
   *		...
   *		mov.u32u32 r0.x, ...
   * }
   *
   * However any other recursion would be unnecessary.
            if (block->data != block) {
               for (unsigned i = 0; i < block->predecessors_count; i++) {
      struct ir3_block *pred = block->predecessors[i];
   unsigned pred_delay = delay_calc(pred, NULL, consumer, distance,
                                    }
      /**
   * Calculate delay for nop insertion. This must exactly match hardware
   * requirements, including recursing into predecessor blocks.
   */
   unsigned
   ir3_delay_calc(struct ir3_block *block, struct ir3_instruction *instr,
         {
      regmask_t mask;
   regmask_init(&mask, mergedregs);
   foreach_src (src, instr) {
      if (!(src->flags & (IR3_REG_IMMED | IR3_REG_CONST)))
                  }
