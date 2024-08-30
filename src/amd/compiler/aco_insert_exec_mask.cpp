   /*
   * Copyright © 2019 Valve Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   *
   */
      #include "aco_builder.h"
   #include "aco_ir.h"
      #include "util/u_math.h"
      #include <set>
   #include <vector>
      namespace aco {
      namespace {
      enum WQMState : uint8_t {
      Unspecified = 0,
   Exact = 1 << 0,
      };
      enum mask_type : uint8_t {
      mask_type_global = 1 << 0,
   mask_type_exact = 1 << 1,
   mask_type_wqm = 1 << 2,
      };
      struct loop_info {
      Block* loop_header;
   uint16_t num_exec_masks;
   bool has_divergent_break;
   bool has_divergent_continue;
   bool has_discard; /* has a discard or demote */
   loop_info(Block* b, uint16_t num, bool breaks, bool cont, bool discard)
      : loop_header(b), num_exec_masks(num), has_divergent_break(breaks),
         };
      struct block_info {
      std::vector<std::pair<Operand, uint8_t>>
      };
      struct exec_ctx {
      Program* program;
   std::vector<block_info> info;
   std::vector<loop_info> loop;
   bool handle_wqm = false;
      };
      bool
   needs_exact(aco_ptr<Instruction>& instr)
   {
      if (instr->isMUBUF()) {
         } else if (instr->isMTBUF()) {
         } else if (instr->isMIMG()) {
         } else if (instr->isFlatLike()) {
         } else {
      /* Require Exact for p_jump_to_epilog because if p_exit_early_if is
   * emitted inside the same block, the main FS will always jump to the PS
   * epilog without considering the exec mask.
   */
   return instr->isEXP() || instr->opcode == aco_opcode::p_jump_to_epilog ||
         }
      WQMState
   get_instr_needs(aco_ptr<Instruction>& instr)
   {
      if (needs_exact(instr))
            bool pred_by_exec = needs_exec_mask(instr.get()) || instr->opcode == aco_opcode::p_logical_end ||
               }
      Operand
   get_exec_op(Operand t)
   {
      if (t.isUndefined())
         else
      }
      void
   transition_to_WQM(exec_ctx& ctx, Builder bld, unsigned idx)
   {
      if (ctx.info[idx].exec.back().second & mask_type_wqm)
         if (ctx.info[idx].exec.back().second & mask_type_global) {
      Operand exec_mask = ctx.info[idx].exec.back().first;
   if (exec_mask.isUndefined()) {
      exec_mask = bld.copy(bld.def(bld.lm), Operand(exec, bld.lm));
               exec_mask = bld.sop1(Builder::s_wqm, Definition(exec, bld.lm), bld.def(s1, scc),
         ctx.info[idx].exec.emplace_back(exec_mask, mask_type_global | mask_type_wqm);
      }
   /* otherwise, the WQM mask should be one below the current mask */
   ctx.info[idx].exec.pop_back();
   assert(ctx.info[idx].exec.back().second & mask_type_wqm);
   assert(ctx.info[idx].exec.back().first.size() == bld.lm.size());
   assert(ctx.info[idx].exec.back().first.isTemp());
   ctx.info[idx].exec.back().first =
      }
      void
   transition_to_Exact(exec_ctx& ctx, Builder bld, unsigned idx)
   {
      if (ctx.info[idx].exec.back().second & mask_type_exact)
         /* We can't remove the loop exec mask, because that can cause exec.size() to
   * be less than num_exec_masks. The loop exec mask also needs to be kept
   * around for various uses. */
   if ((ctx.info[idx].exec.back().second & mask_type_global) &&
      !(ctx.info[idx].exec.back().second & mask_type_loop)) {
   ctx.info[idx].exec.pop_back();
   assert(ctx.info[idx].exec.back().second & mask_type_exact);
   assert(ctx.info[idx].exec.back().first.size() == bld.lm.size());
   assert(ctx.info[idx].exec.back().first.isTemp());
   ctx.info[idx].exec.back().first =
            }
   /* otherwise, we create an exact mask and push to the stack */
   Operand wqm = ctx.info[idx].exec.back().first;
   if (wqm.isUndefined()) {
      wqm = bld.sop1(Builder::s_and_saveexec, bld.def(bld.lm), bld.def(s1, scc),
      } else {
      bld.sop2(Builder::s_and, Definition(exec, bld.lm), bld.def(s1, scc),
      }
   ctx.info[idx].exec.back().first = Operand(wqm);
      }
      unsigned
   add_coupling_code(exec_ctx& ctx, Block* block, std::vector<aco_ptr<Instruction>>& instructions)
   {
      unsigned idx = block->index;
   Builder bld(ctx.program, &instructions);
            /* start block */
   if (preds.empty()) {
      aco_ptr<Instruction>& startpgm = block->instructions[0];
   assert(startpgm->opcode == aco_opcode::p_startpgm);
            unsigned count = 1;
   if (block->instructions[1]->opcode == aco_opcode::p_init_scratch) {
      bld.insert(std::move(block->instructions[1]));
                        /* exec seems to need to be manually initialized with combined shaders */
   if (ctx.program->stage.num_sw_stages() > 1 ||
      ctx.program->stage.hw == AC_HW_NEXT_GEN_GEOMETRY_SHADER ||
   (ctx.program->stage.sw == SWStage::VS &&
   (ctx.program->stage.hw == AC_HW_HULL_SHADER ||
         (ctx.program->stage.sw == SWStage::TES &&
   ctx.program->stage.hw == AC_HW_LEGACY_GEOMETRY_SHADER)) {
   start_exec = Operand::c32_or_c64(-1u, bld.lm == s2);
               /* EXEC is automatically initialized by the HW for compute shaders.
   * We know for sure exec is initially -1 when the shader always has full subgroups.
   */
   if (ctx.program->stage == compute_cs && ctx.program->info.cs.uses_full_subgroups)
            if (ctx.handle_wqm) {
      ctx.info[idx].exec.emplace_back(start_exec, mask_type_global | mask_type_exact);
   /* Initialize WQM already */
      } else {
      uint8_t mask = mask_type_global;
   if (ctx.program->needs_wqm) {
      bld.sop1(Builder::s_wqm, Definition(exec, bld.lm), bld.def(s1, scc),
            } else {
         }
                           /* loop entry block */
   if (block->kind & block_kind_loop_header) {
      assert(preds[0] == idx - 1);
   ctx.info[idx].exec = ctx.info[idx - 1].exec;
   loop_info& info = ctx.loop.back();
   while (ctx.info[idx].exec.size() > info.num_exec_masks)
            /* create ssa names for outer exec masks */
   if (info.has_discard) {
      aco_ptr<Pseudo_instruction> phi;
   for (int i = 0; i < info.num_exec_masks - 1; i++) {
      phi.reset(create_instruction<Pseudo_instruction>(aco_opcode::p_linear_phi,
         phi->definitions[0] = bld.def(bld.lm);
   phi->operands[0] = get_exec_op(ctx.info[preds[0]].exec[i].first);
                  /* create ssa name for restore mask */
   if (info.has_divergent_break) {
      /* this phi might be trivial but ensures a parallelcopy on the loop header */
   aco_ptr<Pseudo_instruction> phi{create_instruction<Pseudo_instruction>(
         phi->definitions[0] = bld.def(bld.lm);
   phi->operands[0] = get_exec_op(ctx.info[preds[0]].exec[info.num_exec_masks - 1].first);
               /* create ssa name for loop active mask */
   aco_ptr<Pseudo_instruction> phi{create_instruction<Pseudo_instruction>(
         if (info.has_divergent_continue)
         else
         phi->operands[0] = get_exec_op(ctx.info[preds[0]].exec.back().first);
            if (info.has_divergent_break) {
      uint8_t mask_type =
            } else {
      ctx.info[idx].exec.back().first = Operand(loop_active);
               /* create a parallelcopy to move the active mask to exec */
   unsigned i = 0;
   if (info.has_divergent_continue) {
      while (block->instructions[i]->opcode != aco_opcode::p_logical_start) {
      bld.insert(std::move(block->instructions[i]));
      }
   uint8_t mask_type = ctx.info[idx].exec.back().second & (mask_type_wqm | mask_type_exact);
   assert(ctx.info[idx].exec.back().first.size() == bld.lm.size());
   ctx.info[idx].exec.emplace_back(
                           /* loop exit block */
   if (block->kind & block_kind_loop_exit) {
      Block* header = ctx.loop.back().loop_header;
            for (ASSERTED unsigned pred : preds)
            /* fill the loop header phis */
   std::vector<unsigned>& header_preds = header->linear_preds;
   int instr_idx = 0;
   if (info.has_discard) {
      while (instr_idx < info.num_exec_masks - 1) {
      aco_ptr<Instruction>& phi = header->instructions[instr_idx];
   assert(phi->opcode == aco_opcode::p_linear_phi);
   for (unsigned i = 1; i < phi->operands.size(); i++)
                        {
      aco_ptr<Instruction>& phi = header->instructions[instr_idx++];
   assert(phi->opcode == aco_opcode::p_linear_phi);
   for (unsigned i = 1; i < phi->operands.size(); i++)
      phi->operands[i] =
            if (info.has_divergent_break) {
      aco_ptr<Instruction>& phi = header->instructions[instr_idx];
   assert(phi->opcode == aco_opcode::p_linear_phi);
   for (unsigned i = 1; i < phi->operands.size(); i++)
      phi->operands[i] =
                     /* create the loop exit phis if not trivial */
   for (unsigned exec_idx = 0; exec_idx < info.num_exec_masks; exec_idx++) {
      Operand same = ctx.info[preds[0]].exec[exec_idx].first;
                  for (unsigned i = 1; i < preds.size() && trivial; i++) {
      if (ctx.info[preds[i]].exec[exec_idx].first != same)
               if (trivial) {
         } else {
      /* create phi for loop footer */
   aco_ptr<Pseudo_instruction> phi{create_instruction<Pseudo_instruction>(
         phi->definitions[0] = bld.def(bld.lm);
   if (exec_idx == info.num_exec_masks - 1u) {
         }
   for (unsigned i = 0; i < phi->operands.size(); i++)
                        assert(ctx.info[idx].exec.size() == info.num_exec_masks);
         } else if (preds.size() == 1) {
         } else {
      assert(preds.size() == 2);
   /* if one of the predecessors ends in exact mask, we pop it from stack */
   unsigned num_exec_masks =
            if (block->kind & block_kind_merge)
         if (block->kind & block_kind_top_level)
            /* create phis for diverged exec masks */
   for (unsigned i = 0; i < num_exec_masks; i++) {
      /* skip trivial phis */
   if (ctx.info[preds[0]].exec[i].first == ctx.info[preds[1]].exec[i].first) {
      Operand t = ctx.info[preds[0]].exec[i].first;
   /* discard/demote can change the state of the current exec mask */
   assert(!t.isTemp() ||
         uint8_t mask = ctx.info[preds[0]].exec[i].second & ctx.info[preds[1]].exec[i].second;
   ctx.info[idx].exec.emplace_back(t, mask);
               Temp phi = bld.pseudo(aco_opcode::p_linear_phi, bld.def(bld.lm),
               uint8_t mask_type = ctx.info[preds[0]].exec[i].second & ctx.info[preds[1]].exec[i].second;
                  unsigned i = 0;
   while (block->instructions[i]->opcode == aco_opcode::p_phi ||
            bld.insert(std::move(block->instructions[i]));
               if (ctx.handle_wqm) {
      /* End WQM handling if not needed anymore */
   if (block->kind & block_kind_top_level && ctx.info[idx].exec.size() == 2) {
      if (block->instructions[i]->opcode == aco_opcode::p_end_wqm) {
      ctx.info[idx].exec.back().second |= mask_type_global;
   transition_to_Exact(ctx, bld, idx);
   ctx.handle_wqm = false;
                     /* restore exec mask after divergent control flow */
   if (block->kind & (block_kind_loop_exit | block_kind_merge) &&
      !ctx.info[idx].exec.back().first.isUndefined()) {
   Operand restore = ctx.info[idx].exec.back().first;
   assert(restore.size() == bld.lm.size());
   bld.copy(Definition(exec, bld.lm), restore);
   if (!restore.isConstant())
                  }
      /* Avoid live-range splits in Exact mode:
   * Because the data register of atomic VMEM instructions
   * is shared between src and dst, it might be necessary
   * to create live-range splits during RA.
   * Make the live-range splits explicit in WQM mode.
   */
   void
   handle_atomic_data(exec_ctx& ctx, Builder& bld, unsigned block_idx, aco_ptr<Instruction>& instr)
   {
      /* check if this is an atomic VMEM instruction */
   int idx = -1;
   if (!instr->isVMEM() || instr->definitions.empty())
         else if (instr->isMIMG())
         else if (instr->operands.size() == 4)
            if (idx != -1) {
      /* insert explicit copy of atomic data in WQM-mode */
   transition_to_WQM(ctx, bld, block_idx);
   Temp data = instr->operands[idx].getTemp();
   data = bld.copy(bld.def(data.regClass()), data);
         }
      void
   process_instructions(exec_ctx& ctx, Block* block, std::vector<aco_ptr<Instruction>>& instructions,
         {
      WQMState state;
   if (ctx.info[block->index].exec.back().second & mask_type_wqm) {
         } else {
      assert(!ctx.handle_wqm || ctx.info[block->index].exec.back().second & mask_type_exact);
                        for (; idx < block->instructions.size(); idx++) {
                        if (needs == WQM && state != WQM) {
      transition_to_WQM(ctx, bld, block->index);
      } else if (needs == Exact) {
      if (ctx.handle_wqm)
         transition_to_Exact(ctx, bld, block->index);
               if (instr->opcode == aco_opcode::p_discard_if) {
               if (block->instructions[idx + 1]->opcode == aco_opcode::p_end_wqm) {
      /* Transition to Exact without extra instruction. */
   ctx.info[block->index].exec.resize(1);
   assert(ctx.info[block->index].exec[0].second == (mask_type_exact | mask_type_global));
   current_exec = get_exec_op(ctx.info[block->index].exec[0].first);
   ctx.info[block->index].exec[0].first = Operand(bld.lm);
      } else if (ctx.info[block->index].exec.size() >= 2 && ctx.handle_wqm) {
      /* Preserve the WQM mask */
               Temp cond, exit_cond;
   if (instr->operands[0].isConstant()) {
      assert(instr->operands[0].constantValue() == -1u);
   /* save condition and set exec to zero */
   exit_cond = bld.tmp(s1);
   cond =
      bld.sop1(Builder::s_and_saveexec, bld.def(bld.lm), bld.scc(Definition(exit_cond)),
   } else {
      cond = instr->operands[0].getTemp();
   /* discard from current exec */
   exit_cond = bld.sop2(Builder::s_andn2, Definition(exec, bld.lm), bld.def(s1, scc),
                           /* discard from inner to outer exec mask on stack */
   int num = ctx.info[block->index].exec.size() - 2;
   for (int i = num; i >= 0; i--) {
      Instruction* andn2 = bld.sop2(Builder::s_andn2, bld.def(bld.lm), bld.def(s1, scc),
         ctx.info[block->index].exec[i].first = Operand(andn2->definitions[0].getTemp());
               instr->opcode = aco_opcode::p_exit_early_if;
               } else if (instr->opcode == aco_opcode::p_is_helper) {
      Definition dst = instr->definitions[0];
   assert(dst.size() == bld.lm.size());
   if (state == Exact) {
      instr.reset(create_instruction<SOP1_instruction>(bld.w64or32(Builder::s_mov),
         instr->operands[0] = Operand::zero();
      } else {
                     instr.reset(create_instruction<SOP2_instruction>(bld.w64or32(Builder::s_andn2),
         instr->operands[0] = Operand(exec, bld.lm); /* current exec */
   instr->operands[1] = Operand(exact_mask.first);
   instr->definitions[0] = dst;
         } else if (instr->opcode == aco_opcode::p_demote_to_helper) {
      /* turn demote into discard_if with only exact masks */
                  int num;
   Operand src;
   Temp exit_cond;
   if (instr->operands[0].isConstant() && !(block->kind & block_kind_top_level)) {
      assert(instr->operands[0].constantValue() == -1u);
   /* transition to exact and set exec to zero */
   exit_cond = bld.tmp(s1);
                  num = ctx.info[block->index].exec.size() - 2;
   if (!(ctx.info[block->index].exec.back().second & mask_type_exact)) {
      ctx.info[block->index].exec.back().first = src;
         } else {
      /* demote_if: transition to exact */
   if (block->kind & block_kind_top_level && ctx.info[block->index].exec.size() == 2 &&
      ctx.info[block->index].exec.back().second & mask_type_global) {
   /* We don't need to actually copy anything into exec, since the s_andn2
   * instructions later will do that.
   */
      } else {
         }
   src = instr->operands[0];
               for (int i = num; i >= 0; i--) {
      if (ctx.info[block->index].exec[i].second & mask_type_exact) {
      Instruction* andn2 =
      bld.sop2(Builder::s_andn2, bld.def(bld.lm), bld.def(s1, scc),
                     ctx.info[block->index].exec[i].first = Operand(andn2->definitions[0].getTemp());
      } else {
            }
   instr->opcode = aco_opcode::p_exit_early_if;
               } else if (instr->opcode == aco_opcode::p_elect) {
                     if (all_lanes_enabled) {
         } else {
      Temp first_lane_idx = bld.sop1(Builder::s_ff1_i32, bld.def(s1), Operand(exec, bld.lm));
   bld.sop2(Builder::s_lshl, Definition(dst), bld.def(s1, scc),
      }
      } else if (instr->opcode == aco_opcode::p_end_wqm) {
      assert(block->kind & block_kind_top_level);
   assert(ctx.info[block->index].exec.size() <= 2);
   /* This instruction indicates the end of WQM mode. */
   ctx.info[block->index].exec.back().second |= mask_type_global;
   transition_to_Exact(ctx, bld, block->index);
   state = Exact;
   ctx.handle_wqm = false;
                     }
      void
   add_branch_code(exec_ctx& ctx, Block* block)
   {
      unsigned idx = block->index;
            if (block->linear_succs.empty())
            if (block->kind & block_kind_loop_preheader) {
      /* collect information about the succeeding loop */
   bool has_divergent_break = false;
   bool has_divergent_continue = false;
   bool has_discard = false;
            for (unsigned i = idx + 1; ctx.program->blocks[i].loop_nest_depth >= loop_nest_depth; i++) {
               if (loop_block.kind & block_kind_uses_discard)
                        if (loop_block.kind & block_kind_uniform)
         else if (loop_block.kind & block_kind_break)
         else if (loop_block.kind & block_kind_continue)
               unsigned num_exec_masks = ctx.info[idx].exec.size();
   if (block->kind & block_kind_top_level)
            ctx.loop.emplace_back(&ctx.program->blocks[block->linear_succs[0]], num_exec_masks,
               /* For normal breaks, this is the exec mask. For discard+break, it's the
   * old exec mask before it was zero'd.
   */
            if (block->kind & block_kind_continue_or_break) {
      assert(ctx.program->blocks[ctx.program->blocks[block->linear_succs[1]].linear_succs[0]].kind &
         assert(ctx.program->blocks[ctx.program->blocks[block->linear_succs[0]].linear_succs[0]].kind &
         assert(block->instructions.back()->opcode == aco_opcode::p_branch);
            bool need_parallelcopy = false;
   while (!(ctx.info[idx].exec.back().second & mask_type_loop)) {
      ctx.info[idx].exec.pop_back();
               if (need_parallelcopy)
      ctx.info[idx].exec.back().first =
      bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2), Operand(exec, bld.lm),
                     if (block->kind & block_kind_uniform) {
      Pseudo_branch_instruction& branch = block->instructions.back()->branch();
   if (branch.opcode == aco_opcode::p_branch) {
         } else {
      branch.target[0] = block->linear_succs[1];
      }
               if (block->kind & block_kind_branch) {
      // orig = s_and_saveexec_b64
   assert(block->linear_succs.size() == 2);
   assert(block->instructions.back()->opcode == aco_opcode::p_cbranch_z);
   Temp cond = block->instructions.back()->operands[0].getTemp();
   const bool sel_ctrl = block->instructions.back()->branch().selection_control_remove;
            uint8_t mask_type = ctx.info[idx].exec.back().second & (mask_type_wqm | mask_type_exact);
   if (ctx.info[idx].exec.back().first.constantEquals(-1u)) {
         } else {
                                 /* add next current exec to the stack */
            Builder::Result r = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2), Operand(exec, bld.lm),
         r->branch().selection_control_remove = sel_ctrl;
               if (block->kind & block_kind_invert) {
      // exec = s_andn2_b64 (original_exec, exec)
   assert(block->instructions.back()->opcode == aco_opcode::p_branch);
   const bool sel_ctrl = block->instructions.back()->branch().selection_control_remove;
   block->instructions.pop_back();
   assert(ctx.info[idx].exec.size() >= 2);
   Operand orig_exec = ctx.info[idx].exec[ctx.info[idx].exec.size() - 2].first;
   bld.sop2(Builder::s_andn2, Definition(exec, bld.lm), bld.def(s1, scc), orig_exec,
            Builder::Result r = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2), Operand(exec, bld.lm),
         r->branch().selection_control_remove = sel_ctrl;
               if (block->kind & block_kind_break) {
      // loop_mask = s_andn2_b64 (loop_mask, exec)
   assert(block->instructions.back()->opcode == aco_opcode::p_branch);
            Temp cond = Temp();
   for (int exec_idx = ctx.info[idx].exec.size() - 2; exec_idx >= 0; exec_idx--) {
      cond = bld.tmp(s1);
   Operand exec_mask = ctx.info[idx].exec[exec_idx].first;
   exec_mask = bld.sop2(Builder::s_andn2, bld.def(bld.lm), bld.scc(Definition(cond)),
         ctx.info[idx].exec[exec_idx].first = exec_mask;
   if (ctx.info[idx].exec[exec_idx].second & mask_type_loop)
               /* check if the successor is the merge block, otherwise set exec to 0 */
   // TODO: this could be done better by directly branching to the merge block
   unsigned succ_idx = ctx.program->blocks[block->linear_succs[1]].linear_succs[0];
   Block& succ = ctx.program->blocks[succ_idx];
   if (!(succ.kind & block_kind_invert || succ.kind & block_kind_merge)) {
                  bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2), bld.scc(cond), block->linear_succs[1],
                     if (block->kind & block_kind_continue) {
      assert(block->instructions.back()->opcode == aco_opcode::p_branch);
            Temp cond = Temp();
   for (int exec_idx = ctx.info[idx].exec.size() - 2; exec_idx >= 0; exec_idx--) {
      if (ctx.info[idx].exec[exec_idx].second & mask_type_loop)
         cond = bld.tmp(s1);
   Operand exec_mask = ctx.info[idx].exec[exec_idx].first;
   exec_mask = bld.sop2(Builder::s_andn2, bld.def(bld.lm), bld.scc(Definition(cond)),
            }
            /* check if the successor is the merge block, otherwise set exec to 0 */
   // TODO: this could be done better by directly branching to the merge block
   unsigned succ_idx = ctx.program->blocks[block->linear_succs[1]].linear_succs[0];
   Block& succ = ctx.program->blocks[succ_idx];
   if (!(succ.kind & block_kind_invert || succ.kind & block_kind_merge)) {
                  bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2), bld.scc(cond), block->linear_succs[1],
               }
      void
   process_block(exec_ctx& ctx, Block* block)
   {
      std::vector<aco_ptr<Instruction>> instructions;
                                                   }
      } /* end namespace */
      void
   insert_exec_mask(Program* program)
   {
               if (program->needs_wqm && program->needs_exact)
            for (Block& block : program->blocks)
      }
      } // namespace aco
