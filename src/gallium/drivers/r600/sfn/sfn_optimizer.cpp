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
      #include "sfn_optimizer.h"
      #include "sfn_debug.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_controlflow.h"
   #include "sfn_instr_export.h"
   #include "sfn_instr_fetch.h"
   #include "sfn_instr_lds.h"
   #include "sfn_instr_mem.h"
   #include "sfn_instr_tex.h"
   #include "sfn_peephole.h"
   #include "sfn_valuefactory.h"
   #include "sfn_virtualvalues.h"
      #include <sstream>
      namespace r600 {
      bool
   optimize(Shader& shader)
   {
               sfn_log << SfnLog::opt << "Shader before optimization\n";
   if (sfn_log.has_debug_flag(SfnLog::opt)) {
      std::stringstream ss;
   shader.print(ss);
               do {
      progress = false;
   progress |= copy_propagation_fwd(shader);
   progress |= dead_code_elimination(shader);
   progress |= copy_propagation_backward(shader);
   progress |= dead_code_elimination(shader);
   progress |= simplify_source_vectors(shader);
   progress |= peephole(shader);
                  }
      class DCEVisitor : public InstrVisitor {
   public:
               void visit(AluInstr *instr) override;
   void visit(AluGroup *instr) override;
   void visit(TexInstr *instr) override;
   void visit(ExportInstr *instr) override { (void)instr; };
   void visit(FetchInstr *instr) override;
            void visit(ControlFlowInstr *instr) override { (void)instr; };
   void visit(IfInstr *instr) override { (void)instr; };
   void visit(ScratchIOInstr *instr) override { (void)instr; };
   void visit(StreamOutInstr *instr) override { (void)instr; };
   void visit(MemRingOutInstr *instr) override { (void)instr; };
   void visit(EmitVertexInstr *instr) override { (void)instr; };
   void visit(GDSInstr *instr) override { (void)instr; };
   void visit(WriteTFInstr *instr) override { (void)instr; };
   void visit(LDSAtomicInstr *instr) override { (void)instr; };
   void visit(LDSReadInstr *instr) override;
               };
      bool
   dead_code_elimination(Shader& shader)
   {
                                    dce.progress = false;
   for (auto& b : shader.func())
                           sfn_log << SfnLog::opt << "Shader after DCE\n";
   if (sfn_log.has_debug_flag(SfnLog::opt)) {
      std::stringstream ss;
   shader.print(ss);
                  }
      DCEVisitor::DCEVisitor():
         {
   }
      void
   DCEVisitor::visit(AluInstr *instr)
   {
               if (instr->has_instr_flag(Instr::dead))
            if (instr->dest() && (instr->dest()->has_uses())) {
      sfn_log << SfnLog::opt << " dest used\n";
               switch (instr->opcode()) {
   case op2_kille:
   case op2_killne:
   case op2_kille_int:
   case op2_killne_int:
   case op2_killge:
   case op2_killge_int:
   case op2_killge_uint:
   case op2_killgt:
   case op2_killgt_int:
   case op2_killgt_uint:
   case op0_group_barrier:
      sfn_log << SfnLog::opt << " never kill\n";
      default:;
            bool dead = instr->set_dead();
   sfn_log << SfnLog::opt << (dead ? "dead" : "alive") << "\n";
      }
      void
   DCEVisitor::visit(LDSReadInstr *instr)
   {
      sfn_log << SfnLog::opt << "visit " << *instr << "\n";
      }
      void
   DCEVisitor::visit(AluGroup *instr)
   {
      /* Groups are created because the instructions are used together
   * so don't try to eliminate code there */
      }
      void
   DCEVisitor::visit(TexInstr *instr)
   {
               bool has_uses = false;
   RegisterVec4::Swizzle swz = instr->all_dest_swizzle();
   for (int i = 0; i < 4; ++i) {
      if (!dest[i]->has_uses())
         else
      }
            if (has_uses)
               }
      void
   DCEVisitor::visit(FetchInstr *instr)
   {
               bool has_uses = false;
   RegisterVec4::Swizzle swz = instr->all_dest_swizzle();
   for (int i = 0; i < 4; ++i) {
      if (!dest[i]->has_uses())
         else
      }
            if (has_uses)
                        }
      void
   DCEVisitor::visit(Block *block)
   {
      auto i = block->begin();
   auto e = block->end();
   while (i != e) {
      auto n = i++;
   if (!(*n)->keep()) {
      (*n)->accept(*this);
   if ((*n)->is_dead()) {
                  }
      class CopyPropFwdVisitor : public InstrVisitor {
   public:
               void visit(AluInstr *instr) override;
   void visit(AluGroup *instr) override;
   void visit(TexInstr *instr) override;
   void visit(ExportInstr *instr) override;
   void visit(FetchInstr *instr) override;
   void visit(Block *instr) override;
   void visit(ControlFlowInstr *instr) override { (void)instr; }
   void visit(IfInstr *instr) override { (void)instr; }
   void visit(ScratchIOInstr *instr) override { (void)instr; }
   void visit(StreamOutInstr *instr) override { (void)instr; }
   void visit(MemRingOutInstr *instr) override { (void)instr; }
   void visit(EmitVertexInstr *instr) override { (void)instr; }
   void visit(GDSInstr *instr) override;
   void visit(WriteTFInstr *instr) override { (void)instr; };
            // TODO: these two should use copy propagation
   void visit(LDSAtomicInstr *instr) override { (void)instr; };
            void propagate_to(RegisterVec4& src, Instr *instr);
            ValueFactory& value_factory;
      };
      class CopyPropBackVisitor : public InstrVisitor {
   public:
               void visit(AluInstr *instr) override;
   void visit(AluGroup *instr) override;
   void visit(TexInstr *instr) override;
   void visit(ExportInstr *instr) override { (void)instr; }
   void visit(FetchInstr *instr) override;
   void visit(Block *instr) override;
   void visit(ControlFlowInstr *instr) override { (void)instr; }
   void visit(IfInstr *instr) override { (void)instr; }
   void visit(ScratchIOInstr *instr) override { (void)instr; }
   void visit(StreamOutInstr *instr) override { (void)instr; }
   void visit(MemRingOutInstr *instr) override { (void)instr; }
   void visit(EmitVertexInstr *instr) override { (void)instr; }
   void visit(GDSInstr *instr) override { (void)instr; };
   void visit(WriteTFInstr *instr) override { (void)instr; };
   void visit(LDSAtomicInstr *instr) override { (void)instr; };
   void visit(LDSReadInstr *instr) override { (void)instr; };
               };
      bool
   copy_propagation_fwd(Shader& shader)
   {
      auto& root = shader.func();
            do {
      copy_prop.progress = false;
   for (auto b : root)
               sfn_log << SfnLog::opt << "Shader after Copy Prop forward\n";
   if (sfn_log.has_debug_flag(SfnLog::opt)) {
      std::stringstream ss;
   shader.print(ss);
                  }
      bool
   copy_propagation_backward(Shader& shader)
   {
               do {
      copy_prop.progress = false;
   for (auto b : shader.func())
               sfn_log << SfnLog::opt << "Shader after Copy Prop backwards\n";
   if (sfn_log.has_debug_flag(SfnLog::opt)) {
      std::stringstream ss;
   shader.print(ss);
                  }
      CopyPropFwdVisitor::CopyPropFwdVisitor(ValueFactory& vf):
      value_factory(vf),
      {
   }
      void
   CopyPropFwdVisitor::visit(AluInstr *instr)
   {
      sfn_log << SfnLog::opt << "CopyPropFwdVisitor:[" << instr->block_id() << ":"
            if (instr->dest()) {
                           if (!instr->can_propagate_src()) {
                  auto src = instr->psrc(0);
            /* Don't propagate an indirect load to more than one
   * instruction, because we may have to split the address loads
   * creating more instructions */
   if (dest->uses().size() > 1) {
      auto [addr, is_for_dest, index] = instr->indirect_addr();
   if (addr && !is_for_dest)
                  auto ii = dest->uses().begin();
                     /** libc++ seems to invalidate the end iterator too if a std::set is
   *  made empty by an erase operation,
   *  https://gitlab.freedesktop.org/mesa/mesa/-/issues/7931
   */
   while(ii != ie && !dest->uses().empty()) {
      auto i = *ii;
            ++ii;
   /* SSA can always be propagated, registers only in the same block
   * and only if they are assigned in the same block */
                        /* Register can propagate if the assignment was in the same
   * block, and we don't have a second assignment coming later
   * (e.g. helper invocation evaluation does
   *
   * 1: MOV R0.x, -1
   * 2: FETCH R0.0 VPM
   * 3: MOV SN.x, R0.x
   *
   * Here we can't prpagate the move in 1 to SN.x in 3 */
   if ((mov_block_id == target_block_id && instr->index() < i->index())) {
      dest_can_propagate = true;
   if (dest->parents().size() > 1) {
      for (auto p : dest->parents()) {
      if (p->block_id() == i->block_id() && p->index() > instr->index()) {
      dest_can_propagate = false;
                  }
   bool move_addr_use = false;
   bool src_can_propagate = false;
   if (auto rsrc = src->as_register()) {
      if (rsrc->has_flag(Register::ssa)) {
         } else if (mov_block_id == target_block_id) {
      if (auto a = rsrc->addr()) {
      if (a->as_register() &&
      !a->as_register()->has_flag(Register::addr_or_idx) &&
   i->block_id() == mov_block_id &&
   i->index() == instr->index() + 1) {
   src_can_propagate = true;
         } else {
         }
   for (auto p : rsrc->parents()) {
      if (p->block_id() == mov_block_id &&
      p->index() > instr->index() &&
   p->index() < i->index()) {
   src_can_propagate = false;
               } else {
                  if (dest_can_propagate && src_can_propagate) {
                     if (i->as_alu() && i->as_alu()->parent_group()) {
         } else {
      bool success = i->replace_source(dest, src);
   if (success && move_addr_use) {
      for (auto r : instr->required_instr()){
      std::cerr << "add " << *r << " to " << *i << "\n";
         }
            }
   if (instr->dest()) {
         }
      }
      void
   CopyPropFwdVisitor::visit(AluGroup *instr)
   {
         }
      void
   CopyPropFwdVisitor::visit(TexInstr *instr)
   {
         }
      void CopyPropFwdVisitor::visit(GDSInstr *instr)
   {
         }
      void
   CopyPropFwdVisitor::visit(ExportInstr *instr)
   {
         }
      static bool register_sel_can_change(Pin pin)
   {
         }
      static bool register_chan_is_pinned(Pin pin)
   {
      return pin == pin_chan ||
            }
         void
   CopyPropFwdVisitor::propagate_to(RegisterVec4& value, Instr *instr)
   {
      /* Collect parent instructions - only ALU move without modifiers
   * and without indirect access are allowed. */
   AluInstr *parents[4] = {nullptr};
   bool have_candidates = false;
   for (int i = 0; i < 4; ++i) {
      if (value[i]->chan() < 4 && value[i]->has_flag(Register::ssa)) {
      /*  We have a pre-define value, so we can't propagate a copy */
                                                /* Parent op is not an ALU instruction, so we can't
                           if ((parents[i]->opcode() != op1_mov) ||
      parents[i]->has_source_mod(0, AluInstr::mod_neg) ||
   parents[i]->has_source_mod(0, AluInstr::mod_abs) ||
   parents[i]->has_alu_flag(alu_dst_clamp) ||
                        /* Don't accept moves with indirect reads, because they are not
   * supported with instructions that use vec4 values */
                                 if (!have_candidates)
            /* Collect the new source registers. We may have to move all registers
            PRegister new_src[4] = {0};
            uint8_t used_chan_mask = 0;
   int new_sel = -1;
                                 /* No parent means we either ignore the channel or insert 0 or 1.*/
   if (!parents[i])
                     auto src = parents[i]->src(0).as_register();
   if (!src)
            /* Don't accept an array element for now, we would need extra checking
   * that the value is not overwritten by an indirect access */
   if (src->pin() == pin_array)
            /* Is this check still needed ? */
   if (!src->has_flag(Register::ssa) &&
      !assigned_register_direct(src)) {
               /* If the channel chan't switch we have to update the channel mask
   * TODO: assign channel pinned registers first might give more
   *  opportunities for this optimization */
   if (register_chan_is_pinned(src->pin()))
            /* Update the possible channel mask based on the sourcee's parent
   * instruction(s) */
   for (auto p : src->parents()) {
      auto alu = p->as_alu();
   if (alu)
               for (auto u : src->uses()) {
      auto alu = u->as_alu();
   if (alu)
               if (!allowed_mask)
            /* Prefer keeping the channel, but if that's not possible
   * i.e. if the sel has to change, then  pick the next free channel
   * (see below) */
            if (new_sel < 0) {
      new_sel = src->sel();
      } else if (new_sel != src->sel()) {
      /* If we have to assign a new register sel index do so only
   * if all already assigned source can get a new register index,
   * and all registers are either SSA or registers.
   * TODO: check whether this last restriction is required */
   if (all_sel_can_change &&
      register_sel_can_change(src->pin()) &&
   (is_ssa == src->has_flag(Register::ssa))) {
   new_sel = value_factory.new_register_index();
      } else /* Sources can't be combined to a vec4 so bail out */
               new_src[i] = src;
   used_chan_mask |= 1 << new_chan[i];
   if (!register_sel_can_change(src->pin()))
               /* Apply the changes to the vec4 source */
   value.del_use(instr);
   for (int i = 0; i < 4; ++i) {
      if (parents[i]) {
      new_src[i]->set_sel(new_sel);
   if (is_ssa)
                           if (new_src[i]->pin() != pin_fully &&
      new_src[i]->pin() != pin_chgr) {
   if (new_src[i]->pin() == pin_chan)
         else
      }
         }
   value.add_use(instr);
   if (progress)
      }
      bool CopyPropFwdVisitor::assigned_register_direct(PRegister reg)
   {
      for (auto p: reg->parents()) {
      if (p->as_alu())  {
      auto [addr, dummy, index_reg] = p->as_alu()->indirect_addr();
   if (addr)
         }
      }
      void
   CopyPropFwdVisitor::visit(FetchInstr *instr)
   {
         }
      void
   CopyPropFwdVisitor::visit(Block *instr)
   {
      for (auto& i : *instr)
      }
      CopyPropBackVisitor::CopyPropBackVisitor():
         {
   }
      void
   CopyPropBackVisitor::visit(AluInstr *instr)
   {
               sfn_log << SfnLog::opt << "CopyPropBackVisitor:[" << instr->block_id() << ":"
            if (!instr->can_propagate_dest()) {
                  auto src_reg = instr->psrc(0)->as_register();
   if (!src_reg) {
                  if (src_reg->uses().size() > 1)
            auto dest = instr->dest();
   if (!dest || !instr->has_alu_flag(alu_write)) {
                  if (!dest->has_flag(Register::ssa) && dest->parents().size() > 1)
            for (auto& i : src_reg->parents()) {
      sfn_log << SfnLog::opt << "Try replace dest in " << i->block_id() << ":"
            if (i->replace_dest(dest, instr)) {
      dest->del_parent(instr);
   dest->add_parent(i);
   for (auto d : instr->dependend_instr()) {
         }
                  if (local_progress)
               }
      void
   CopyPropBackVisitor::visit(AluGroup *instr)
   {
      for (auto& i : *instr) {
      if (i)
         }
      void
   CopyPropBackVisitor::visit(TexInstr *instr)
   {
         }
      void
   CopyPropBackVisitor::visit(FetchInstr *instr)
   {
         }
      void
   CopyPropBackVisitor::visit(Block *instr)
   {
      for (auto i = instr->rbegin(); i != instr->rend(); ++i)
      if (!(*i)->is_dead())
   }
      class SimplifySourceVecVisitor : public InstrVisitor {
   public:
      SimplifySourceVecVisitor():
         {
            void visit(AluInstr *instr) override { (void)instr; }
   void visit(AluGroup *instr) override { (void)instr; }
   void visit(TexInstr *instr) override;
   void visit(ExportInstr *instr) override;
   void visit(FetchInstr *instr) override;
   void visit(Block *instr) override;
   void visit(ControlFlowInstr *instr) override;
   void visit(IfInstr *instr) override;
   void visit(ScratchIOInstr *instr) override;
   void visit(StreamOutInstr *instr) override;
   void visit(MemRingOutInstr *instr) override;
   void visit(EmitVertexInstr *instr) override { (void)instr; }
   void visit(GDSInstr *instr) override { (void)instr; };
   void visit(WriteTFInstr *instr) override { (void)instr; };
   void visit(LDSAtomicInstr *instr) override { (void)instr; };
   void visit(LDSReadInstr *instr) override { (void)instr; };
                        };
      class HasVecDestVisitor : public ConstInstrVisitor {
   public:
      HasVecDestVisitor():
         {
            void visit(const AluInstr& instr) override { (void)instr; }
   void visit(const AluGroup& instr) override { (void)instr; }
   void visit(const TexInstr& instr) override  {  (void)instr; has_group_dest = true; };
   void visit(const ExportInstr& instr) override { (void)instr; }
   void visit(const FetchInstr& instr) override  {  (void)instr; has_group_dest = true; };
   void visit(const Block& instr) override { (void)instr; };
   void visit(const ControlFlowInstr& instr) override{ (void)instr; }
   void visit(const IfInstr& instr) override{ (void)instr; }
   void visit(const ScratchIOInstr& instr) override  { (void)instr; };
   void visit(const StreamOutInstr& instr) override { (void)instr; }
   void visit(const MemRingOutInstr& instr) override { (void)instr; }
   void visit(const EmitVertexInstr& instr) override { (void)instr; }
   void visit(const GDSInstr& instr) override { (void)instr; }
   void visit(const WriteTFInstr& instr) override { (void)instr; };
   void visit(const LDSAtomicInstr& instr) override { (void)instr; };
   void visit(const LDSReadInstr& instr) override { (void)instr; };
               };
      class HasVecSrcVisitor : public ConstInstrVisitor {
   public:
      HasVecSrcVisitor():
         {
            void visit(UNUSED const AluInstr& instr) override { }
   void visit(UNUSED const AluGroup& instr) override { }
   void visit(UNUSED const FetchInstr& instr) override  { };
   void visit(UNUSED const Block& instr) override { };
   void visit(UNUSED const ControlFlowInstr& instr) override{ }
   void visit(UNUSED const IfInstr& instr) override{ }
   void visit(UNUSED const LDSAtomicInstr& instr) override { };
            void visit(const TexInstr& instr) override { check(instr.src()); }
   void visit(const ExportInstr& instr) override { check(instr.value()); }
            // No swizzling supported, so we want to keep the register group
   void visit(UNUSED const ScratchIOInstr& instr) override  { has_group_src = true; };
   void visit(UNUSED const StreamOutInstr& instr) override { has_group_src = true; }
   void visit(UNUSED const MemRingOutInstr& instr) override { has_group_src = true; }
                     // We always emit at least two values
                           };
      void HasVecSrcVisitor::check(const RegisterVec4& value)
   {
      int nval = 0;
   for (int i = 0; i < 4 && nval < 2; ++i) {
      if (value[i]->chan() < 4)
      }
      }
      bool
   simplify_source_vectors(Shader& sh)
   {
               for (auto b : sh.func())
               }
      void
   SimplifySourceVecVisitor::visit(TexInstr *instr)
   {
         if (instr->opcode() != TexInstr::get_resinfo) {
      auto& src = instr->src();
   replace_src(instr, src);
   int nvals = 0;
   for (int i = 0; i < 4; ++i)
      if (src[i]->chan() < 4)
      if (nvals == 1) {
      for (int i = 0; i < 4; ++i)
      if (src[i]->chan() < 4) {
      HasVecDestVisitor check_dests;
   for (auto p : src[i]->parents()) {
      p->accept(check_dests);
                     HasVecSrcVisitor check_src;
   for (auto p : src[i]->uses()) {
      p->accept(check_src);
                                    if (src[i]->pin() == pin_group)
         else if (src[i]->pin() == pin_chgr)
         }
   for (auto& prep : instr->prepare_instr()) {
            }
      void
   SimplifySourceVecVisitor::visit(ScratchIOInstr *instr)
   {
         }
      class ReplaceConstSource : public AluInstrVisitor {
   public:
      ReplaceConstSource(Instr *old_use_, RegisterVec4& vreg_, int i):
      old_use(old_use_),
   vreg(vreg_),
   index(i),
      {
                              Instr *old_use;
   RegisterVec4& vreg;
   int index;
      };
      void
   SimplifySourceVecVisitor::visit(ExportInstr *instr)
   {
         }
      void
   SimplifySourceVecVisitor::replace_src(Instr *instr, RegisterVec4& reg4)
   {
      for (int i = 0; i < 4; ++i) {
               if (s->chan() > 3)
            if (!s->has_flag(Register::ssa))
            /* Cayman trans ops have more then one parent for
   * one dest */
   if (s->parents().size() != 1)
                                             }
      void
   SimplifySourceVecVisitor::visit(StreamOutInstr *instr)
   {
         }
      void
   SimplifySourceVecVisitor::visit(MemRingOutInstr *instr)
   {
         }
      void
   ReplaceConstSource::visit(AluInstr *alu)
   {
      if (alu->opcode() != op1_mov)
            if (alu->has_source_mod(0, AluInstr::mod_abs) ||
      alu->has_source_mod(0, AluInstr::mod_neg))
         auto src = alu->psrc(0);
                     if (value_is_const_uint(*src, 0)) {
         } else if (value_is_const_float(*src, 1.0f)) {
                  if (override_chan >= 0) {
      vreg[index]->del_use(old_use);
   auto reg = new Register(vreg.sel(), override_chan, vreg[index]->pin());
   vreg.set_value(index, reg);
         }
      void
   SimplifySourceVecVisitor::visit(FetchInstr *instr)
   {
         }
      void
   SimplifySourceVecVisitor::visit(Block *instr)
   {
      for (auto i = instr->rbegin(); i != instr->rend(); ++i)
      if (!(*i)->is_dead())
   }
      void
   SimplifySourceVecVisitor::visit(ControlFlowInstr *instr)
   {
         }
      void
   SimplifySourceVecVisitor::visit(IfInstr *instr)
   {
         }
      } // namespace r600
