   /*
   * Copyright (C) 2018 Jonathan Marek <jonathan@marek.ca>
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
   *    Jonathan Marek <jonathan@marek.ca>
   */
      #include "ir2_private.h"
      static bool
   scalar_possible(struct ir2_instr *instr)
   {
      if (instr->alu.scalar_opc == SCALAR_NONE)
               }
      static bool
   is_alu_compatible(struct ir2_instr *a, struct ir2_instr *b)
   {
      if (!a)
            /* dont use same instruction twice */
   if (a == b)
            /* PRED_SET must be alone */
   if (b->alu.scalar_opc >= PRED_SETEs &&
      b->alu.scalar_opc <= PRED_SET_RESTOREs)
         /* must write to same export (issues otherwise?) */
      }
      /* priority of vector instruction for scheduling (lower=higher prio) */
   static unsigned
   alu_vector_prio(struct ir2_instr *instr)
   {
      if (instr->alu.vector_opc == VECTOR_NONE)
            if (is_export(instr))
            /* TODO check src type and ncomps */
   if (instr->src_count == 3)
            if (!scalar_possible(instr))
               }
      /* priority of scalar instruction for scheduling (lower=higher prio) */
   static unsigned
   alu_scalar_prio(struct ir2_instr *instr)
   {
      if (!scalar_possible(instr))
            /* this case is dealt with later */
   if (instr->src_count > 1)
            if (is_export(instr))
            /* PRED to end of block */
   if (instr->alu.scalar_opc >= PRED_SETEs &&
      instr->alu.scalar_opc <= PRED_SET_RESTOREs)
         /* scalar only have highest priority */
      }
      /* this is a bit messy:
   * we want to find a slot where we can insert a scalar MOV with
   * a vector instruction that was already scheduled
   */
   static struct ir2_sched_instr *
   insert(struct ir2_context *ctx, unsigned block_idx, unsigned reg_idx,
         {
      struct ir2_sched_instr *sched = NULL, *s;
            /* go first earliest point where the mov can be inserted */
   for (i = ctx->instr_sched_count - 1; i > 0; i--) {
               if (s->instr && s->instr->block_idx != block_idx)
         if (s->instr_s && s->instr_s->block_idx != block_idx)
            if (src1.type == IR2_SRC_SSA) {
      if ((s->instr && s->instr->idx == src1.num) ||
      (s->instr_s && s->instr_s->idx == src1.num))
            unsigned mr = ~(s->reg_state[reg_idx / 8] >> reg_idx % 8 * 4 & 0xf);
   if ((mask & mr) == 0)
            mask &= mr;
   if (s->instr_s || s->instr->src_count == 3)
            if (s->instr->type != IR2_ALU || s->instr->alu.export >= 0)
               }
            if (sched) {
      for (s = sched; s != &ctx->instr_sched[ctx->instr_sched_count]; s++)
                  }
      /* case1:
   * in this case, insert a mov to place the 2nd src into to same reg
   * (scalar sources come from the same register)
   *
   * this is a common case which works when one of the srcs is input/const
   * but for instrs which have 2 ssa/reg srcs, then its not ideal
   */
   static bool
   scalarize_case1(struct ir2_context *ctx, struct ir2_instr *instr, bool order)
   {
      struct ir2_src src0 = instr->src[order];
   struct ir2_src src1 = instr->src[!order];
   struct ir2_sched_instr *sched;
   struct ir2_instr *ins;
   struct ir2_reg *reg;
            switch (src0.type) {
   case IR2_SRC_CONST:
   case IR2_SRC_INPUT:
         default:
                  /* TODO, insert needs logic for this */
   if (src1.type == IR2_SRC_REG)
            /* we could do something if they match src1.. */
   if (src0.negate || src0.abs)
                     /* result not used more since we will overwrite */
   for (int i = 0; i < 4; i++)
      if (reg->comp[i].ref_count != !!(instr->alu.write_mask & 1 << i))
         /* find a place to insert the mov */
   sched = insert(ctx, instr->block_idx, reg->idx, src1, &comp);
   if (!sched)
            ins = &ctx->instr[idx = ctx->instr_count++];
   ins->idx = idx;
   ins->type = IR2_ALU;
   ins->src[0] = src1;
   ins->src_count = 1;
   ins->is_ssa = true;
   ins->ssa.idx = reg->idx;
   ins->ssa.ncomp = 1;
   ins->ssa.comp[0].c = comp;
   ins->alu.scalar_opc = MAXs;
   ins->alu.export = -1;
   ins->alu.write_mask = 1;
   ins->pred = instr->pred;
            instr->src[0] = src0;
            sched->instr_s = ins;
      }
      /* fill sched with next fetch or (vector and/or scalar) alu instruction */
   static int
   sched_next(struct ir2_context *ctx, struct ir2_sched_instr *sched)
   {
      struct ir2_instr *avail[0x100], *instr_v = NULL, *instr_s = NULL;
            instr_alloc_type_t export = ~0u;
            /* XXX merge this loop with the other one somehow? */
   ir2_foreach_instr (instr, ctx) {
      if (!instr->need_emit)
         if (is_export(instr))
               ir2_foreach_instr (instr, ctx) {
      if (!instr->need_emit)
            /* dont mix exports */
   if (is_export(instr) && export_buf(instr->alu.export) != export)
            if (block_idx < 0)
         else if (block_idx != instr->block_idx || /* must be same block */
            instr->type == IR2_CF ||         /* CF/MEM must be alone */
      /* it works because IR2_CF is always at end of block
   * and somewhat same idea with MEM exports, which might not be alone
   * but will end up in-order at least
            /* check if dependencies are satisfied */
   bool is_ok = true;
   ir2_foreach_src (src, instr) {
      if (src->type == IR2_SRC_REG) {
      /* need to check if all previous instructions in the block
   * which write the reg have been emitted
   * slow..
   * XXX: check components instead of whole register
   */
   struct ir2_reg *reg = get_reg_src(ctx, src);
   ir2_foreach_instr (p, ctx) {
      if (!p->is_ssa && p->reg == reg && p->idx < instr->idx)
         } else if (src->type == IR2_SRC_SSA) {
      /* in this case its easy, just check need_emit */
         }
   /* don't reorder non-ssa write before read */
   if (!instr->is_ssa) {
      ir2_foreach_instr (p, ctx) {
                     ir2_foreach_src (src, p) {
      if (get_reg_src(ctx, src) == instr->reg)
            }
   /* don't reorder across predicates */
   if (avail_count && instr->pred != avail[0]->pred)
            if (!is_ok)
                        if (!avail_count) {
      assert(block_idx == -1);
               /* priority to FETCH instructions */
   ir2_foreach_avail (instr) {
      if (instr->type == IR2_ALU)
            ra_src_free(ctx, instr);
            instr->need_emit = false;
   sched->instr = instr;
   sched->instr_s = NULL;
                        unsigned prio_v = ~0u, prio_s = ~0u, prio;
   ir2_foreach_avail (instr) {
      prio = alu_vector_prio(instr);
   if (prio < prio_v) {
      instr_v = instr;
                  /* TODO can still insert scalar if src_count=3, if smart about it */
   if (!instr_v || instr_v->src_count < 3) {
      ir2_foreach_avail (instr) {
               prio = alu_scalar_prio(instr);
                  if (prio < prio_s) {
      instr_s = instr;
   prio_s = prio;
   if (!compat)
                              /* now, we try more complex insertion of vector instruction as scalar
   * TODO: if we are smart we can still insert if instr_v->src_count==3
   */
   if (!instr_s && instr_v->src_count < 3) {
      ir2_foreach_avail (instr) {
                                    if (scalarize_case1(ctx, instr, 0)) {
      instr_s = instr;
      }
   if (scalarize_case1(ctx, instr, 1)) {
      instr_s = instr;
                     /* free src registers */
   if (instr_v) {
      instr_v->need_emit = false;
               if (instr_s) {
      instr_s->need_emit = false;
               /* allocate dst registers */
   if (instr_v)
      ra_reg(ctx, get_reg(instr_v), -1, is_export(instr_v),
         if (instr_s)
      ra_reg(ctx, get_reg(instr_s), -1, is_export(instr_s),
         sched->instr = instr_v;
   sched->instr_s = instr_s;
      }
      /* scheduling: determine order of instructions */
   static void
   schedule_instrs(struct ir2_context *ctx)
   {
      struct ir2_sched_instr *sched;
            /* allocate input registers */
   for (unsigned idx = 0; idx < ARRAY_SIZE(ctx->input); idx++)
      if (ctx->input[idx].initialized)
         for (;;) {
      sched = &ctx->instr_sched[ctx->instr_sched_count++];
   block_idx = sched_next(ctx, sched);
   if (block_idx < 0)
                  /* catch texture fetch after scheduling and insert the
   * SET_TEX_LOD right before it if necessary
   * TODO clean this up
   */
   struct ir2_instr *instr = sched->instr, *tex_lod;
   if (instr && instr->type == IR2_FETCH && instr->fetch.opc == TEX_FETCH &&
      instr->src_count == 2) {
   /* generate the SET_LOD instruction */
   tex_lod = &ctx->instr[ctx->instr_count++];
   tex_lod->type = IR2_FETCH;
   tex_lod->block_idx = instr->block_idx;
   tex_lod->pred = instr->pred;
   tex_lod->fetch.opc = TEX_SET_TEX_LOD;
                  sched[1] = sched[0];
   sched->instr = tex_lod;
               bool free_block = true;
   ir2_foreach_instr (instr, ctx)
         if (free_block)
      };
      }
      void
   ir2_compile(struct fd2_shader_stateobj *so, unsigned variant,
         {
      struct ir2_context ctx = {};
            if (fp)
            ctx.so = so;
   ctx.info = &so->variant[variant].info;
   ctx.f = &so->variant[variant].f;
            /* convert nir to internal representation */
            /* copy propagate srcs */
            /* get ref_counts and kill non-needed instructions */
            /* remove movs used to write outputs */
            /* instruction order.. and vector->scalar conversions */
            /* finally, assemble to bitcode */
      }
