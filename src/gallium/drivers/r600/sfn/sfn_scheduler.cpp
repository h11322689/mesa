   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2022 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "sfn_scheduler.h"
      #include "amd_family.h"
   #include "r600_isa.h"
   #include "sfn_alu_defines.h"
   #include "sfn_debug.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_controlflow.h"
   #include "sfn_instr_export.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_instr_lds.h"
   #include "sfn_instr_mem.h"
   #include "sfn_instr_tex.h"
      #include <algorithm>
   #include <sstream>
      namespace r600 {
      class CollectInstructions : public InstrVisitor {
      public:
      CollectInstructions(ValueFactory& vf):
         {
            void visit(AluInstr *instr) override
   {
      if (instr->has_alu_flag(alu_is_trans))
         else {
      if (instr->alu_slots() == 1)
         else
         }
   void visit(AluGroup *instr) override { alu_groups.push_back(instr); }
   void visit(TexInstr *instr) override { tex.push_back(instr); }
   void visit(ExportInstr *instr) override { exports.push_back(instr); }
   void visit(FetchInstr *instr) override { fetches.push_back(instr); }
   void visit(Block *instr) override
   {
      for (auto& i : *instr)
               void visit(ControlFlowInstr *instr) override
   {
      assert(!m_cf_instr);
               void visit(IfInstr *instr) override
   {
      assert(!m_cf_instr);
               void visit(EmitVertexInstr *instr) override
   {
      assert(!m_cf_instr);
                                                            void visit(LDSReadInstr *instr) override
   {
      std::vector<AluInstr *> buffer;
   m_last_lds_instr = instr->split(buffer, m_last_lds_instr);
   for (auto& i : buffer) {
                     void visit(LDSAtomicInstr *instr) override
   {
      std::vector<AluInstr *> buffer;
   m_last_lds_instr = instr->split(buffer, m_last_lds_instr);
   for (auto& i : buffer) {
                              std::list<AluInstr *> alu_trans;
   std::list<AluInstr *> alu_vec;
   std::list<TexInstr *> tex;
   std::list<AluGroup *> alu_groups;
   std::list<ExportInstr *> exports;
   std::list<FetchInstr *> fetches;
   std::list<WriteOutInstr *> mem_write_instr;
   std::list<MemRingOutInstr *> mem_ring_writes;
   std::list<GDSInstr *> gds_op;
   std::list<WriteTFInstr *> write_tf;
            Instr *m_cf_instr{nullptr};
               };
      struct ArrayChanHash
   {
      std::size_t operator()(std::pair<int, int> const& s) const noexcept
   {
            };
      using ArrayCheckSet = std::unordered_set<std::pair<int, int>, ArrayChanHash>;
      class BlockScheduler {
   public:
      BlockScheduler(r600_chip_class chip_class,
                           private:
      void
                     template <typename T>
            bool collect_ready_alu_vec(std::list<AluInstr *>& ready,
            bool schedule_tex(Shader::ShaderBlocks& out_blocks);
            template <typename I>
            template <typename I>
            bool schedule_alu(Shader::ShaderBlocks& out_blocks);
            bool schedule_alu_to_group_vec(AluGroup *group);
            bool schedule_exports(Shader::ShaderBlocks& out_blocks,
                                       void update_array_writes(const AluGroup& group);
   bool check_array_reads(const AluInstr& instr);
            std::list<AluInstr *> alu_vec_ready;
   std::list<AluInstr *> alu_trans_ready;
   std::list<AluGroup *> alu_groups_ready;
   std::list<TexInstr *> tex_ready;
   std::list<ExportInstr *> exports_ready;
   std::list<FetchInstr *> fetches_ready;
   std::list<WriteOutInstr *> memops_ready;
   std::list<MemRingOutInstr *> mem_ring_writes_ready;
   std::list<GDSInstr *> gds_ready;
   std::list<WriteTFInstr *> write_tf_ready;
            enum {
      sched_alu,
   sched_tex,
   sched_fetch,
   sched_free,
   sched_mem_ring,
   sched_gds,
   sched_write_tf,
               ExportInstr *m_last_pos;
   ExportInstr *m_last_pixel;
                     int m_lds_addr_count{0};
   int m_alu_groups_scheduled{0};
   r600_chip_class m_chip_class;
   radeon_family m_chip_family;
   bool m_idx0_loading{false};
   bool m_idx1_loading{false};
   bool m_idx0_pending{false};
            bool m_nop_after_rel_dest{false};
   bool m_nop_befor_rel_src{false};
               ArrayCheckSet m_last_indirect_array_write;
      };
      Shader *
   schedule(Shader *original)
   {
      Block::set_chipclass(original->chip_class());
            sfn_log << SfnLog::schedule << "Original shader\n";
   if (sfn_log.has_debug_flag(SfnLog::schedule)) {
      std::stringstream ss;
   original->print(ss);
               // TODO later it might be necessary to clone the shader
                              s.run(scheduled_shader);
            sfn_log << SfnLog::schedule << "Scheduled shader\n";
   if (sfn_log.has_debug_flag(SfnLog::schedule)) {
      std::stringstream ss;
   scheduled_shader->print(ss);
                  }
      BlockScheduler::BlockScheduler(r600_chip_class chip_class,
            current_shed(sched_alu),
   m_last_pos(nullptr),
   m_last_pixel(nullptr),
   m_last_param(nullptr),
   m_current_block(nullptr),
   m_chip_class(chip_class),
      {
               m_nop_befor_rel_src = m_chip_class == ISA_CC_R600 &&
                  }
      void
   BlockScheduler::run(Shader *shader)
   {
               for (auto& block : shader->func()) {
      sfn_log << SfnLog::schedule << "Process block " << block->id() << "\n";
   if (sfn_log.has_debug_flag(SfnLog::schedule)) {
      std::stringstream ss;
   block->print(ss);
      }
                  }
      void
   BlockScheduler::schedule_block(Block& in_block,
               {
                  current_shed = sched_fetch;
            CollectInstructions cir(vf);
                     m_current_block = new Block(in_block.nesting_depth(), m_next_block_id++);
   m_current_block->set_instr_flag(Instr::force_cf);
                                 if (alu_vec_ready.size())
            if (alu_trans_ready.size())
            if (alu_groups_ready.size())
            if (exports_ready.size())
         if (tex_ready.size())
         if (fetches_ready.size())
         if (mem_ring_writes_ready.size())
      sfn_log << SfnLog::schedule << "  MEM_RING:" << mem_ring_writes_ready.size()
      if (memops_ready.size())
                  if (!m_current_block->lds_group_active() &&
      m_current_block->expected_ar_uses() == 0) {
   if (last_shed != sched_free && memops_ready.size() > 8)
         else if (mem_ring_writes_ready.size() > 15)
         else if (rat_instr_ready.size() > 3)
         else if (tex_ready.size() > (m_chip_class >= ISA_CC_EVERGREEN ? 15 : 7))
               switch (current_shed) {
   case sched_alu:
      if (!schedule_alu(out_blocks)) {
      assert(!m_current_block->lds_group_active());
   current_shed = sched_tex;
      }
   last_shed = current_shed;
      case sched_tex:
      if (tex_ready.empty() || !schedule_tex(out_blocks)) {
      current_shed = sched_fetch;
      }
   last_shed = current_shed;
      case sched_fetch:
      if (!fetches_ready.empty()) {
      schedule_vtx(out_blocks);
      }
   current_shed = sched_gds;
      case sched_gds:
      if (!gds_ready.empty()) {
      schedule_gds(out_blocks, gds_ready);
      }
   current_shed = sched_mem_ring;
      case sched_mem_ring:
      if (mem_ring_writes_ready.empty() ||
      !schedule_cf(out_blocks, mem_ring_writes_ready)) {
   current_shed = sched_write_tf;
      }
   last_shed = current_shed;
      case sched_write_tf:
      if (write_tf_ready.empty() || !schedule_gds(out_blocks, write_tf_ready)) {
      current_shed = sched_rat;
      }
   last_shed = current_shed;
      case sched_rat:
      if (rat_instr_ready.empty() || !schedule_cf(out_blocks, rat_instr_ready)) {
      current_shed = sched_free;
      }
   last_shed = current_shed;
      case sched_free:
      if (memops_ready.empty() || !schedule_cf(out_blocks, memops_ready)) {
      current_shed = sched_alu;
      }
                           /* Emit exports always at end of a block */
   while (collect_ready_type(exports_ready, cir.exports))
                     if (!cir.alu_groups.empty()) {
      std::cerr << "Unscheduled ALU groups:\n";
   for (auto& a : cir.alu_groups) {
         }
               if (!cir.alu_vec.empty()) {
      std::cerr << "Unscheduled ALU vec ops:\n";
   for (auto& a : cir.alu_vec) {
      std::cerr << "   [" << a->block_id() << ":"
         for (auto& d : a->required_instr())
      std::cerr << "      R["<< d->block_id() << ":" << d->index() <<"]:"
   }
               if (!cir.alu_trans.empty()) {
      std::cerr << "Unscheduled ALU trans ops:\n";
   for (auto& a : cir.alu_trans) {
      std::cerr << "   " << "   [" << a->block_id() << ":"
         for (auto& d : a->required_instr())
      }
      }
   if (!cir.mem_write_instr.empty()) {
      std::cerr << "Unscheduled MEM ops:\n";
   for (auto& a : cir.mem_write_instr) {
         }
               if (!cir.fetches.empty()) {
      std::cerr << "Unscheduled Fetch ops:\n";
   for (auto& a : cir.fetches) {
         }
               if (!cir.tex.empty()) {
      std::cerr << "Unscheduled Tex ops:\n";
   for (auto& a : cir.tex) {
         }
               if (fail) {
      std::cerr << "Failing block:\n";
   for (auto& i : in_block)
      std::cerr << "[" << i->block_id() << ":" << i->index() << "] "
                     for (auto i : *m_current_block)
                     assert(cir.tex.empty());
   assert(cir.exports.empty());
   assert(cir.fetches.empty());
   assert(cir.alu_vec.empty());
   assert(cir.mem_write_instr.empty());
                     if (cir.m_cf_instr) {
      // Assert that if condition is ready
   if (m_current_block->type() != Block::alu) {
         }
   m_current_block->push_back(cir.m_cf_instr);
               if (m_current_block->type() == Block::alu)
         else
      }
      void
   BlockScheduler::finalize()
   {
      if (m_last_pos)
         if (m_last_pixel)
         if (m_last_param)
      }
      bool
   BlockScheduler::schedule_alu(Shader::ShaderBlocks& out_blocks)
   {
      bool success = false;
            sfn_log << SfnLog::schedule << "Schedule alu with " <<
                           bool has_lds_ready =
            bool has_ar_read_ready = !alu_vec_ready.empty() &&
            /* If we have ready ALU instructions we have to start a new ALU block */
   if (has_alu_ready || !alu_groups_ready.empty()) {
      if (m_current_block->type() != Block::alu) {
      start_new_block(out_blocks, Block::alu);
                  /* Schedule groups first. unless we have a pending LDS instruction
   * We don't want the LDS instructions to be too far apart because the
   * fetch + read from queue has to be in the same ALU CF block */
   if (!alu_groups_ready.empty() && !has_lds_ready && !has_ar_read_ready) {
                                             /* Only start a new CF if we have no pending AR reads */
   if (m_current_block->try_reserve_kcache(*group)) {
      alu_groups_ready.erase(alu_groups_ready.begin());
      } else {
                        if (!m_current_block->try_reserve_kcache(*group))
         alu_groups_ready.erase(alu_groups_ready.begin());
   sfn_log << SfnLog::schedule << "Schedule ALU group\n";
      } else {
      sfn_log << SfnLog::schedule << "Don't add group because of " <<
                                    if (!group && has_alu_ready) {
      group = new AluGroup();
      } else if (!success) {
                                    while (free_slots && has_alu_ready) {
      if (!alu_vec_ready.empty())
            /* Apparently one can't schedule a t-slot if there is already
   * and LDS instruction scheduled.
   * TODO: check whether this is only relevant for actual LDS instructions
            if (free_slots & 0x10 && !has_lds_ready) {
      sfn_log << SfnLog::schedule << "Try schedule TRANS channel\n";
   if (!alu_trans_ready.empty())
         if (!alu_vec_ready.empty())
               if (success) {
      ++m_alu_groups_scheduled;
      } else if (m_current_block->kcache_reservation_failed()) {
      // LDS read groups should not lead to impossible
                  // AR is loaded but not all uses are done, we don't want
                  // kcache reservation failed, so we have to start a new CF
      } else {
      // Ready is not empty, but we didn't schedule anything, this
   // means we had a indirect array read or write conflict that we
   // can resolve with an extra group that has a NOP instruction
   if (!alu_trans_ready.empty()  || !alu_vec_ready.empty()) {
      group->add_vec_instructions(new AluInstr(op0_nop, 0));
      } else {
                              sfn_log << SfnLog::schedule << "Finalize ALU group\n";
   group->set_scheduled();
   group->fix_last_flag();
            auto [addr, is_index] = group->addr();
   if (is_index) {
      if (addr->sel() == AddressRegister::idx0 && m_idx0_pending) {
      assert(!group->has_lds_group_start());
   assert(m_current_block->expected_ar_uses() == 0);
   start_new_block(out_blocks, Block::alu);
      }
   if (addr->sel() == AddressRegister::idx1 && m_idx1_pending) {
      assert(!group->has_lds_group_start());
   assert(m_current_block->expected_ar_uses() == 0);
   start_new_block(out_blocks, Block::alu);
                                    m_idx0_pending |= m_idx0_loading;
            m_idx1_pending |= m_idx1_loading;
            if (!m_current_block->lds_group_active() &&
      m_current_block->expected_ar_uses() == 0 &&
   (!addr || is_index)) {
               if (group->has_lds_group_start())
            if (group->has_lds_group_end())
            if (group->has_kill_op()) {
      assert(!group->has_lds_group_start());
   assert(m_current_block->expected_ar_uses() == 0);
                  }
      bool
   BlockScheduler::schedule_tex(Shader::ShaderBlocks& out_blocks)
   {
      if (m_current_block->type() != Block::tex || m_current_block->remaining_slots() == 0) {
      start_new_block(out_blocks, Block::tex);
               if (!tex_ready.empty() && m_current_block->remaining_slots() > 0) {
      auto ii = tex_ready.begin();
            if ((unsigned)m_current_block->remaining_slots() < 1 + (*ii)->prepare_instr().size())
            for (auto prep : (*ii)->prepare_instr()) {
      prep->set_scheduled();
               (*ii)->set_scheduled();
   m_current_block->push_back(*ii);
   tex_ready.erase(ii);
      }
      }
      bool
   BlockScheduler::schedule_vtx(Shader::ShaderBlocks& out_blocks)
   {
      if (m_current_block->type() != Block::vtx || m_current_block->remaining_slots() == 0) {
      start_new_block(out_blocks, Block::vtx);
      }
      }
      template <typename I>
   bool
   BlockScheduler::schedule_gds(Shader::ShaderBlocks& out_blocks, std::list<I *>& ready_list)
   {
      bool was_full = m_current_block->remaining_slots() == 0;
   if (m_current_block->type() != Block::gds || was_full) {
      start_new_block(out_blocks, Block::gds);
   if (was_full)
      }
      }
      void
   BlockScheduler::start_new_block(Shader::ShaderBlocks& out_blocks, Block::Type type)
   {
      if (!m_current_block->empty()) {
      sfn_log << SfnLog::schedule << "Start new block\n";
            if (m_current_block->type() != Block::alu)
         else
         m_current_block = new Block(m_current_block->nesting_depth(), m_next_block_id++);
   m_current_block->set_instr_flag(Instr::force_cf);
         }
      }
      void BlockScheduler::maybe_split_alu_block(Shader::ShaderBlocks& out_blocks)
   {
      // TODO: needs fixing
   if (m_current_block->remaining_slots() > 0) {
      out_blocks.push_back(m_current_block);
               int used_slots = 0;
            Instr *next_block_start = nullptr;
   for (auto cur_group : *m_current_block) {
      /* This limit is a bit fishy, it should be 128 */
   if (used_slots + pending_slots + cur_group->slots() < 128) {
      if (cur_group->can_start_alu_block()) {
      next_block_start = cur_group;
   used_slots += pending_slots;
      } else {
            } else {
      assert(next_block_start);
   next_block_start->set_instr_flag(Instr::force_cf);
   used_slots = pending_slots;
                  Block *sub_block = new Block(m_current_block->nesting_depth(),
         sub_block->set_type(Block::alu, m_chip_class);
            for (auto instr : *m_current_block) {
      auto group = instr->as_alu_group();
   if (!group) {
         sub_block->push_back(instr);
            if (group->group_force_alu_cf()) {
      assert(!sub_block->lds_group_active());
   out_blocks.push_back(sub_block);
   sub_block = new Block(m_current_block->nesting_depth(),
         sub_block->set_type(Block::alu, m_chip_class);
      }
   sub_block->push_back(group);
   if (group->has_lds_group_start())
            if (group->has_lds_group_end())
         }
   if (!sub_block->empty())
      }
      template <typename I>
   bool
   BlockScheduler::schedule_cf(Shader::ShaderBlocks& out_blocks, std::list<I *>& ready_list)
   {
      if (ready_list.empty())
         if (m_current_block->type() != Block::cf)
            }
      bool
   BlockScheduler::schedule_alu_to_group_vec(AluGroup *group)
   {
      assert(group);
            bool success = false;
   auto i = alu_vec_ready.begin();
   auto e = alu_vec_ready.end();
   while (i != e) {
               if (check_array_reads(**i)) {
      ++i;
               // precausion: don't kill while we hae LDS queue reads in the pipeline
   if ((*i)->is_kill() && m_current_block->lds_group_active())
            if (!m_current_block->try_reserve_kcache(**i)) {
      sfn_log << SfnLog::schedule << " failed (kcache)\n";
   ++i;
               if (group->add_vec_instructions(*i)) {
      auto old_i = i;
   ++i;
   if ((*old_i)->has_alu_flag(alu_is_lds)) {
                  if ((*old_i)->num_ar_uses())
                        bool is_idx_load_on_eg = false;
   if (!(*old_i)->has_alu_flag(alu_is_lds)) {
      bool load_idx0_eg = (*old_i)->opcode() == op1_set_cf_idx0;
                  bool load_idx1_eg = (*old_i)->opcode() == op1_set_cf_idx1;
                                                            m_idx0_loading |= load_idx0;
                              alu_vec_ready.erase(old_i);
   success = true;
      } else {
      ++i;
         }
      }
      bool
   BlockScheduler::schedule_alu_to_group_trans(AluGroup *group,
         {
               bool success = false;
   auto i = readylist.begin();
   auto e = readylist.end();
               if (check_array_reads(**i)) {
      ++i;
               sfn_log << SfnLog::schedule << "Try schedule to trans " << **i;
   if (!m_current_block->try_reserve_kcache(**i)) {
      sfn_log << SfnLog::schedule << " failed (kcache)\n";
   ++i;
               if (group->add_trans_instructions(*i)) {
      auto old_i = i;
   ++i;
   auto addr = std::get<0>((*old_i)->indirect_addr());
                  readylist.erase(old_i);
   success = true;
   sfn_log << SfnLog::schedule << " success\n";
      } else {
      ++i;
         }
      }
      template <typename I>
   bool
   BlockScheduler::schedule(std::list<I *>& ready_list)
   {
      if (!ready_list.empty() && m_current_block->remaining_slots() > 0) {
      auto ii = ready_list.begin();
   sfn_log << SfnLog::schedule << "Schedule: " << **ii << "\n";
   (*ii)->set_scheduled();
   m_current_block->push_back(*ii);
   ready_list.erase(ii);
      }
      }
      template <typename I>
   bool
   BlockScheduler::schedule_block(std::list<I *>& ready_list)
   {
      bool success = false;
   while (!ready_list.empty() && m_current_block->remaining_slots() > 0) {
      auto ii = ready_list.begin();
   sfn_log << SfnLog::schedule << "Schedule: " << **ii << " "
         (*ii)->set_scheduled();
   m_current_block->push_back(*ii);
   ready_list.erase(ii);
      }
      }
      bool
   BlockScheduler::schedule_exports(Shader::ShaderBlocks& out_blocks,
         {
      if (m_current_block->type() != Block::cf)
            if (!ready_list.empty()) {
      auto ii = ready_list.begin();
   sfn_log << SfnLog::schedule << "Schedule: " << **ii << "\n";
   (*ii)->set_scheduled();
   m_current_block->push_back(*ii);
   switch ((*ii)->export_type()) {
   case ExportInstr::pos:
      m_last_pos = *ii;
      case ExportInstr::param:
      m_last_param = *ii;
      case ExportInstr::pixel:
      m_last_pixel = *ii;
      }
   (*ii)->set_is_last_export(false);
   ready_list.erase(ii);
      }
      }
      bool
   BlockScheduler::collect_ready(CollectInstructions& available)
   {
      sfn_log << SfnLog::schedule << "Ready instructions\n";
   bool result = false;
   result |= collect_ready_alu_vec(alu_vec_ready, available.alu_vec);
   result |= collect_ready_type(alu_trans_ready, available.alu_trans);
   result |= collect_ready_type(alu_groups_ready, available.alu_groups);
   result |= collect_ready_type(gds_ready, available.gds_op);
   result |= collect_ready_type(tex_ready, available.tex);
   result |= collect_ready_type(fetches_ready, available.fetches);
   result |= collect_ready_type(memops_ready, available.mem_write_instr);
   result |= collect_ready_type(mem_ring_writes_ready, available.mem_ring_writes);
   result |= collect_ready_type(write_tf_ready, available.write_tf);
            sfn_log << SfnLog::schedule << "\n";
      }
      bool
   BlockScheduler::collect_ready_alu_vec(std::list<AluInstr *>& ready,
         {
      auto i = available.begin();
            for (auto alu : ready) {
                  int max_check = 0;
   while (i != e && max_check++ < 64) {
                  int priority = 0;
   /* LDS fetches that use static offsets are usually ready ery fast,
   * so that they would get schedules early, and this leaves the
   * problem that we allocate too many registers with just constant
   * values, and this will make problems with RA. So limit the number of
   * LDS address registers.
   */
   if ((*i)->has_alu_flag(alu_lds_address)) {
      if (m_lds_addr_count > 64) {
      ++i;
      } else {
                     /* LDS instructions are scheduled with high priority.
   * instractions that can go into the t slot and don't have
   * indirect access are put in last, so that they don't block
                           if ((*i)->has_lds_access()) {
      priority = 100000;
   if ((*i)->has_alu_flag(alu_is_lds))
      } else if (addr) {
         } else if (AluGroup::has_t()) {
      auto opinfo = alu_ops.find((*i)->opcode());
   assert(opinfo != alu_ops.end());
   if (opinfo->second.can_channel(AluOp::t, m_chip_class))
                                       auto old_i = i;
   ++i;
      } else
               for (auto& i : ready)
            ready.sort([](const AluInstr *lhs, const AluInstr *rhs) {
                  for (auto& i : ready)
               }
      template <typename T> struct type_char {
   };
      template <> struct type_char<AluInstr> {
         };
      template <> struct type_char<AluGroup> {
         };
      template <> struct type_char<ExportInstr> {
         };
      template <> struct type_char<TexInstr> {
         };
      template <> struct type_char<FetchInstr> {
         };
      template <> struct type_char<WriteOutInstr> {
         };
      template <> struct type_char<MemRingOutInstr> {
         };
      template <> struct type_char<WriteTFInstr> {
         };
      template <> struct type_char<GDSInstr> {
         };
      template <> struct type_char<RatInstr> {
         };
      template <typename T>
   bool
   BlockScheduler::collect_ready_type(std::list<T *>& ready, std::list<T *>& available)
   {
      auto i = available.begin();
            int lookahead = 16;
   while (i != e && ready.size() < 16 && lookahead-- > 0) {
      if ((*i)->ready()) {
      ready.push_back(*i);
   auto old_i = i;
   ++i;
      } else
               for (auto& i : ready)
               }
      class CheckArrayAccessVisitor : public  ConstRegisterVisitor {
   public:
      using ConstRegisterVisitor::visit;
   void visit(const Register& value) override {(void)value;}
   void visit(const LocalArray& value) override {(void)value;}
   void visit(const UniformValue& value) override {(void)value;}
   void visit(const LiteralConstant& value) override {(void)value;}
      };
      class UpdateArrayWrite : public CheckArrayAccessVisitor {
   public:
      UpdateArrayWrite(ArrayCheckSet& indirect_arrays,
                  last_indirect_array_write(indirect_arrays),
   last_direct_array_write(direct_arrays),
      {
            void visit(const LocalArrayValue& value) override {
      int array_base = value.array().base_sel();
   auto entry = std::make_pair(array_base, value.chan());
   if (value.addr())
         else if (track_direct_writes)
         private:
      ArrayCheckSet& last_indirect_array_write;
   ArrayCheckSet& last_direct_array_write;
      };
         void BlockScheduler::update_array_writes(const AluGroup& group)
   {
      if (m_nop_after_rel_dest || m_nop_befor_rel_src) {
      m_last_direct_array_write.clear();
            UpdateArrayWrite visitor(m_last_indirect_array_write,
                  for (auto alu : group) {
      if (alu && alu->dest())
            }
      class CheckArrayRead : public CheckArrayAccessVisitor {
   public:
      CheckArrayRead(const ArrayCheckSet& indirect_arrays,
            last_indirect_array_write(indirect_arrays),
      {
            void visit(const LocalArrayValue& value) override {
      int array_base = value.array().base_sel();
            if (last_indirect_array_write.find(entry) !=
                  if (value.addr() && last_direct_array_write.find(entry) !=
      last_direct_array_write.end()) {
                  const ArrayCheckSet& last_indirect_array_write;
   const ArrayCheckSet& last_direct_array_write;
      };
         bool BlockScheduler::check_array_reads(const AluInstr& instr)
   {
                  CheckArrayRead visitor(m_last_indirect_array_write,
            for (auto& s : instr.sources()) {
         }
      }
      }
      bool BlockScheduler::check_array_reads(const AluGroup& group)
   {
                  CheckArrayRead visitor(m_last_indirect_array_write,
            for (auto alu : group) {
      if (!alu)
         for (auto& s : alu->sources()) {
            }
      }
      }
         } // namespace r600
