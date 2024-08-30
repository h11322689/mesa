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
      static unsigned
   src_swizzle(struct ir2_context *ctx, struct ir2_src *src, unsigned ncomp)
   {
      struct ir2_reg_component *comps;
            switch (src->type) {
   case IR2_SRC_SSA:
   case IR2_SRC_REG:
         default:
         }
   /* we need to take into account where the components were allocated */
   comps = get_reg_src(ctx, src)->comp;
   for (int i = 0; i < ncomp; i++) {
         }
      }
      /* alu instr need to take into how the output components are allocated */
      /* scalar doesn't need to take into account dest swizzle */
      static unsigned
   alu_swizzle_scalar(struct ir2_context *ctx, struct ir2_src *reg)
   {
      /* hardware seems to take from W, but swizzle everywhere just in case */
      }
      static unsigned
   alu_swizzle(struct ir2_context *ctx, struct ir2_instr *instr,
         {
      struct ir2_reg_component *comp = get_reg(instr)->comp;
   unsigned swiz0 = src_swizzle(ctx, src, src_ncomp(instr));
            /* non per component special cases */
   switch (instr->alu.vector_opc) {
   case PRED_SETE_PUSHv ... PRED_SETGTE_PUSHv:
         case DOT2ADDv:
   case DOT3v:
   case DOT4v:
   case CUBEv:
         default:
                  for (int i = 0, j = 0; i < dst_ncomp(instr); j++) {
      if (instr->alu.write_mask & 1 << j) {
      if (comp[j].c != 7)
               }
      }
      static unsigned
   alu_swizzle_scalar2(struct ir2_context *ctx, struct ir2_src *src, unsigned s1)
   {
      /* hardware seems to take from ZW, but swizzle everywhere (ABAB) */
   unsigned s0 = swiz_get(src_swizzle(ctx, src, 1), 0);
      }
      /* write_mask needs to be transformed by allocation information */
      static unsigned
   alu_write_mask(struct ir2_context *ctx, struct ir2_instr *instr)
   {
      struct ir2_reg_component *comp = get_reg(instr)->comp;
            for (int i = 0; i < 4; i++) {
      if (instr->alu.write_mask & 1 << i)
                  }
      /* fetch instructions can swizzle dest, but src swizzle needs conversion */
      static unsigned
   fetch_swizzle(struct ir2_context *ctx, struct ir2_src *src, unsigned ncomp)
   {
      unsigned alu_swiz = src_swizzle(ctx, src, ncomp);
   unsigned swiz = 0;
   for (int i = 0; i < ncomp; i++)
            }
      static unsigned
   fetch_dst_swiz(struct ir2_context *ctx, struct ir2_instr *instr)
   {
      struct ir2_reg_component *comp = get_reg(instr)->comp;
   unsigned dst_swiz = 0xfff;
   for (int i = 0; i < dst_ncomp(instr); i++) {
      dst_swiz &= ~(7 << comp[i].c * 3);
      }
      }
      /* register / export # for instr */
   static unsigned
   dst_to_reg(struct ir2_context *ctx, struct ir2_instr *instr)
   {
      if (is_export(instr))
               }
      /* register # for src */
   static unsigned
   src_to_reg(struct ir2_context *ctx, struct ir2_src *src)
   {
         }
      static unsigned
   src_reg_byte(struct ir2_context *ctx, struct ir2_src *src)
   {
      if (src->type == IR2_SRC_CONST) {
      assert(!src->abs); /* no abs bit for const */
      }
      }
      /* produce the 12 byte binary instruction for a given sched_instr */
   static void
   fill_instr(struct ir2_context *ctx, struct ir2_sched_instr *sched, instr_t *bc,
         {
                        if (instr && instr->type == IR2_FETCH) {
               bc->fetch.opc = instr->fetch.opc;
   bc->fetch.pred_select = !!instr->pred;
                     if (instr->fetch.opc == VTX_FETCH) {
                              vtx->src_reg = src_to_reg(ctx, src);
   vtx->src_swiz = fetch_swizzle(ctx, src, 1);
                  vtx->must_be_one = 1;
                           /* XXX seems like every FETCH but the first has
   * this bit set:
   */
   vtx->reserved3 = instr->idx ? 0x1 : 0x0;
      } else if (instr->fetch.opc == TEX_FETCH) {
               tex->src_reg = src_to_reg(ctx, src);
   tex->src_swiz = fetch_swizzle(ctx, src, 3);
   tex->dst_reg = dst_to_reg(ctx, instr);
   tex->dst_swiz = fetch_dst_swiz(ctx, instr);
   /* tex->const_idx = patch_fetches */
   tex->mag_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->min_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->mip_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->aniso_filter = ANISO_FILTER_USE_FETCH_CONST;
   tex->arbitrary_filter = ARBITRARY_FILTER_USE_FETCH_CONST;
   tex->vol_mag_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->vol_min_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->use_comp_lod = ctx->so->type == MESA_SHADER_FRAGMENT;
   tex->use_reg_lod = instr->src_count == 2;
   tex->sample_location = SAMPLE_CENTER;
      } else if (instr->fetch.opc == TEX_SET_TEX_LOD) {
               tex->src_reg = src_to_reg(ctx, src);
   tex->src_swiz = fetch_swizzle(ctx, src, 1);
                  tex->mag_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->min_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->mip_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->aniso_filter = ANISO_FILTER_USE_FETCH_CONST;
   tex->arbitrary_filter = ARBITRARY_FILTER_USE_FETCH_CONST;
   tex->vol_mag_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->vol_min_filter = TEX_FILTER_USE_FETCH_CONST;
   tex->use_comp_lod = 1;
   tex->use_reg_lod = 0;
      } else {
         }
               instr_v = sched->instr;
            if (instr_v) {
               src1 = instr_v->src[0];
   src2 = instr_v->src[instr_v->src_count > 1];
            bc->alu.vector_opc = instr_v->alu.vector_opc;
   bc->alu.vector_write_mask = alu_write_mask(ctx, instr_v);
   bc->alu.vector_dest = dst_to_reg(ctx, instr_v);
   bc->alu.vector_clamp = instr_v->alu.saturate;
            /* single operand SETEv, use 0.0f as src2 */
   if (instr_v->src_count == 1 &&
      (bc->alu.vector_opc == SETEv || bc->alu.vector_opc == SETNEv ||
               /* export32 instr for a20x hw binning has this bit set..
   * it seems to do more than change the base address of constants
   * XXX this is a hack
   */
   bc->alu.relative_addr =
            bc->alu.src1_reg_byte = src_reg_byte(ctx, &src1);
   bc->alu.src1_swiz = alu_swizzle(ctx, instr_v, &src1);
   bc->alu.src1_reg_negate = src1.negate;
            bc->alu.src2_reg_byte = src_reg_byte(ctx, &src2);
   bc->alu.src2_swiz = alu_swizzle(ctx, instr_v, &src2);
   bc->alu.src2_reg_negate = src2.negate;
            if (src3) {
      bc->alu.src3_reg_byte = src_reg_byte(ctx, src3);
   bc->alu.src3_swiz = alu_swizzle(ctx, instr_v, src3);
   bc->alu.src3_reg_negate = src3->negate;
                           if (instr_s) {
               bc->alu.scalar_opc = instr_s->alu.scalar_opc;
   bc->alu.scalar_write_mask = alu_write_mask(ctx, instr_s);
   bc->alu.scalar_dest = dst_to_reg(ctx, instr_s);
   bc->alu.scalar_clamp = instr_s->alu.saturate;
            if (instr_s->src_count == 1) {
      bc->alu.src3_reg_byte = src_reg_byte(ctx, src);
   bc->alu.src3_swiz = alu_swizzle_scalar(ctx, src);
   bc->alu.src3_reg_negate = src->negate;
      } else {
               bc->alu.src3_reg_byte = src_reg_byte(ctx, src);
   bc->alu.src3_swiz =
         bc->alu.src3_reg_negate = src->negate;
   bc->alu.src3_sel = src->type != IR2_SRC_CONST;
               if (instr_v)
                     *is_fetch = false;
      }
      static unsigned
   write_cfs(struct ir2_context *ctx, instr_cf_t *cfs, unsigned cf_idx,
         {
               if (alloc)
            /* for memory alloc offset for patching */
   if (alloc && alloc->buffer_select == SQ_MEMORY &&
      ctx->info->mem_export_ptr == -1)
         cfs[cf_idx++].exec = *exec;
   exec->address += exec->count;
   exec->serialize = 0;
               }
      /* assemble the final shader */
   void
   assemble(struct ir2_context *ctx, bool binning)
   {
      /* hw seems to have a limit of 384 (num_cf/2+num_instr <= 384)
   * address is 9 bits so could it be 512 ?
   */
   instr_cf_t cfs[384];
   instr_t bytecode[384], bc;
   unsigned block_addr[128];
            /* CF instr state */
   instr_cf_exec_t exec = {.opc = EXEC};
            int sync_id, sync_id_prev = -1;
   bool is_fetch = false;
   bool need_sync = true;
   bool need_alloc = false;
            ctx->info->mem_export_ptr = -1;
            /* vertex shader always needs to allocate at least one parameter
   * if it will never happen,
   */
   if (ctx->so->type == MESA_SHADER_VERTEX && ctx->f->inputs_count == 0) {
      alloc.buffer_select = SQ_PARAMETER_PIXEL;
                        for (int i = 0, j = 0; j < ctx->instr_sched_count; j++) {
               /* catch IR2_CF since it isn't a regular instruction */
   if (instr && instr->type == IR2_CF) {
               /* flush any exec cf before inserting jmp */
                  cfs[num_cf++].jmp_call = (instr_cf_jmp_call_t){
      .opc = COND_JMP,
   .address = instr->cf.block_idx, /* will be fixed later */
   .force_call = !instr->pred,
   .predicated_jmp = 1,
   .direction = instr->cf.block_idx > instr->block_idx,
      };
               /* fill the 3 dwords for the instruction */
            /* we need to sync between ALU/VTX_FETCH/TEX_FETCH types */
   sync_id = 0;
   if (is_fetch)
            need_sync = sync_id != sync_id_prev;
            unsigned block;
               if (ctx->instr_sched[j].instr)
                                    /* info for patching */
   if (is_fetch) {
      struct ir2_fetch_info *info =
                  if (bc.fetch.opc == VTX_FETCH) {
         } else if (bc.fetch.opc == TEX_FETCH) {
      info->tex.samp_id = instr->fetch.tex.samp_id;
      } else {
                     /* exec cf after 6 instr or when switching between fetch / alu */
   if (exec.count == 6 ||
      (exec.count && (need_sync || block != block_idx))) {
   num_cf =
                     /* update block_addrs for jmp patching */
   while (block_idx < block)
            /* export - fill alloc cf */
   if (!is_fetch && bc.alu.export_data) {
      /* get the export buffer from either vector/scalar dest */
   instr_alloc_type_t buffer = export_buf(bc.alu.vector_dest);
   if (bc.alu.scalar_write_mask) {
      if (bc.alu.vector_write_mask)
                                    /* memory export always in 32/33 pair, new alloc on 32 */
                  if (need_new_alloc && exec.count) {
      num_cf =
                                             if (buffer == SQ_PARAMETER_PIXEL &&
                  if (buffer == SQ_POSITION)
               if (is_fetch)
         if (need_sync)
            need_sync = false;
   exec.count += 1;
               /* final exec cf */
   exec.opc = EXEC_END;
            /* insert nop to get an even # of CFs */
   if (num_cf % 2)
            /* patch cf addrs */
   for (int idx = 0; idx < num_cf; idx++) {
      switch (cfs[idx].opc) {
   case NOP:
   case ALLOC:
         case EXEC:
   case EXEC_END:
      cfs[idx].exec.address += num_cf / 2;
      case COND_JMP:
      cfs[idx].jmp_call.address = block_addr[cfs[idx].jmp_call.address];
      default:
                     /* concatenate cfs and alu/fetch */
   uint32_t cfdwords = num_cf / 2 * 3;
   uint32_t alufetchdwords = exec.address * 3;
   uint32_t sizedwords = cfdwords + alufetchdwords;
   uint32_t *dwords = malloc(sizedwords * 4);
   assert(dwords);
   memcpy(dwords, cfs, cfdwords * 4);
            /* finalize ir2_shader_info */
   ctx->info->dwords = dwords;
   ctx->info->sizedwords = sizedwords;
   for (int i = 0; i < ctx->info->num_fetch_instrs; i++)
            if (FD_DBG(DISASM)) {
      DBG("disassemble: type=%d", ctx->so->type);
         }
