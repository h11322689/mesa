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
      #include "sfn_instr_alu.h"
      #include "sfn_alu_defines.h"
   #include "sfn_debug.h"
   #include "sfn_instr_alugroup.h"
   #include "sfn_instr_tex.h"
   #include "sfn_shader.h"
   #include "sfn_virtualvalues.h"
      #include <algorithm>
   #include <sstream>
      namespace r600 {
      using std::istream;
   using std::string;
   using std::vector;
      AluInstr::AluInstr(EAluOp opcode,
                     PRegister dest,
      m_opcode(opcode),
   m_dest(dest),
   m_bank_swizzle(alu_vec_unknown),
   m_cf_type(cf_alu),
      {
               if (m_src.size() == 3)
            for (auto f : flags)
            ASSERT_OR_THROW(m_src.size() ==
                  if (m_alu_flags.test(alu_write))
                     if (dest && slots > 1) {
      switch (m_opcode) {
   case op2_dot_ieee: m_allowed_dest_mask = (1 << (5 - slots)) - 1;
         default:
      if (has_alu_flag(alu_is_cayman_trans)) {
               }
      }
      AluInstr::AluInstr(EAluOp opcode):
         {
   }
      AluInstr::AluInstr(EAluOp opcode, int chan):
         {
         }
      AluInstr::AluInstr(EAluOp opcode,
                           {
   }
      AluInstr::AluInstr(EAluOp opcode,
                     PRegister dest,
         {
   }
      AluInstr::AluInstr(EAluOp opcode,
                     PRegister dest,
   PVirtualValue src0,
         {
   }
      AluInstr::AluInstr(ESDOp op,
                           {
               m_src.push_back(address);
   if (src0) {
      m_src.push_back(src0);
   if (src1)
      }
      }
      AluInstr::AluInstr(ESDOp op, const SrcValues& src, const std::set<AluModifiers>& flags):
      m_lds_opcode(op),
      {
      for (auto f : flags)
            set_alu_flag(alu_is_lds);
      }
      void
   AluInstr::update_uses()
   {
      for (auto& s : m_src) {
      auto r = s->as_register();
   if (r) {
      r->add_use(this);
   // move this to add_use
   if (r->pin() == pin_array) {
      auto array_elm = static_cast<LocalArrayValue *>(r);
   auto addr = array_elm->addr();
   if (addr && addr->as_register())
         }
   auto u = s->as_uniform();
   if (u && u->buf_addr() && u->buf_addr()->as_register())
               if (m_dest &&
      (has_alu_flag(alu_write) ||
   m_opcode == op1_mova_int ||
   m_opcode == op1_set_cf_idx0 ||
   m_opcode == op1_set_cf_idx1)) {
            if (m_dest->pin() == pin_array) {
      // move this to add_parent
   auto array_elm = static_cast<LocalArrayValue *>(m_dest);
   auto addr = array_elm->addr();
   if (addr && addr->as_register())
            }
      void
   AluInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   AluInstr::accept(InstrVisitor& visitor)
   {
         }
      const std::map<ECFAluOpCode, std::string> AluInstr::cf_map = {
      {cf_alu_break,       "BREAK"      },
   {cf_alu_continue,    "CONT"       },
   {cf_alu_else_after,  "ELSE_AFTER" },
   {cf_alu_extended,    "EXTENDED"   },
   {cf_alu_pop_after,   "POP_AFTER"  },
   {cf_alu_pop2_after,  "POP2_AFTER" },
      };
      const std::map<AluBankSwizzle, std::string> AluInstr::bank_swizzle_map = {
      {alu_vec_012, "VEC_012"},
   {alu_vec_021, "VEC_021"},
   {alu_vec_102, "VEC_102"},
   {alu_vec_120, "VEC_120"},
   {alu_vec_201, "VEC_201"},
      };
      const AluModifiers AluInstr::src_rel_flags[3] = {
            struct ValuePrintFlags {
      ValuePrintFlags(int im, int f):
      index_mode(im),
      {
   }
   int index_mode = 0;
   int flags = 0;
   static const int is_rel = 1;
   static const int has_abs = 2;
   static const int has_neg = 4;
   static const int literal_is_float = 8;
   static const int index_ar = 16;
      };
      void
   AluInstr::do_print(std::ostream& os) const
   {
                                 if (has_alu_flag(alu_is_lds)) {
      os << "LDS " << lds_ops.at(m_lds_opcode).name;
                  os << alu_ops.at(m_opcode).name;
   if (has_alu_flag(alu_dst_clamp))
            if (m_dest) {
      if (has_alu_flag(alu_write) || m_dest->has_flag(Register::addr_or_idx)) {
         } else {
      os << " __"
         if (m_dest->pin() != pin_none)
      }
      } else {
                     const int n_source_per_slot =
                           if (s > 0)
            for (int k = 0; k < n_source_per_slot; ++k) {
      int pflags = 0;
   if (i)
         if (has_source_mod(i, mod_neg))
         if (has_alu_flag(src_rel_flags[k]))
         if (n_source_per_slot <= 2)
                  if (pflags & ValuePrintFlags::has_neg)
         if (pflags & ValuePrintFlags::has_abs)
         os << *m_src[i];
   if (pflags & ValuePrintFlags::has_abs)
                        os << " {";
   if (has_alu_flag(alu_write))
         if (has_alu_flag(alu_last_instr))
         if (has_alu_flag(alu_update_exec))
         if (has_alu_flag(alu_update_pred))
                  auto bs_name = bank_swizzle_map.find(m_bank_swizzle);
   if (bs_name != bank_swizzle_map.end())
            auto cf_name = cf_map.find(m_cf_type);
   if (cf_name != cf_map.end())
      }
      bool
   AluInstr::can_propagate_src() const
   {
      /* We can use the source in the next instruction */
   if (!can_copy_propagate())
            auto src_reg = m_src[0]->as_register();
   if (!src_reg)
                     if (!m_dest->has_flag(Register::ssa)) {
                  if (m_dest->pin() == pin_fully)
            if (m_dest->pin() == pin_chan)
      return src_reg->pin() == pin_none ||
                  }
      class ReplaceIndirectArrayAddr : public RegisterVisitor {
   public:
      void visit(Register& value) override { (void)value; }
   void visit(LocalArray& value) override
   {
      (void)value;
      }
   void visit(LocalArrayValue& value) override;
   void visit(UniformValue& value) override;
   void visit(LiteralConstant& value) override { (void)value; }
               };
      void ReplaceIndirectArrayAddr::visit(LocalArrayValue& value)
   {
      if (new_addr->sel() == 0 && value.addr()
      && value.addr()->as_register())
   }
      void ReplaceIndirectArrayAddr::visit(UniformValue& value)
   {
      if (value.buf_addr() && value.buf_addr()->as_register() &&
      (new_addr->sel() == 1 || new_addr->sel() == 2)) {
         }
      void AluInstr::update_indirect_addr(UNUSED PRegister old_reg, PRegister reg)
   {
               visitor.new_addr = reg;
            if (m_dest)
            for (auto src : m_src)
               }
      bool
   AluInstr::can_propagate_dest() const
   {
      if (!can_copy_propagate()) {
                  auto src_reg = m_src[0]->as_register();
   if (!src_reg) {
                           if (src_reg->pin() == pin_fully) {
                  if (!src_reg->has_flag(Register::ssa))
            if (!m_dest->has_flag(Register::ssa))
            if (src_reg->pin() == pin_chan)
      return m_dest->pin() == pin_none || m_dest->pin() == pin_free ||
                  }
      bool
   AluInstr::can_copy_propagate() const
   {
      if (m_opcode != op1_mov)
            if (has_source_mod(0, mod_abs) || has_source_mod(0, mod_neg) ||
      has_alu_flag(alu_dst_clamp))
            }
      bool
   AluInstr::replace_source(PRegister old_src, PVirtualValue new_src)
   {
      if (!can_replace_source(old_src, new_src))
               }
      bool AluInstr::do_replace_source(PRegister old_src, PVirtualValue new_src)
   {
               for (unsigned i = 0; i < m_src.size(); ++i) {
      if (old_src->equal_to(*m_src[i])) {
      m_src[i] = new_src;
         }
   if (process) {
      auto r = new_src->as_register();
   if (r)
                        }
      bool AluInstr::replace_src(int i, PVirtualValue new_src, uint32_t to_set,
         {
      auto old_src = m_src[i]->as_register();
            if (!can_replace_source(old_src, new_src))
            assert(old_src);
                     auto r = new_src->as_register();
   if (r)
            m_source_modifiers |= to_set << (2 * i);
               }
         bool AluInstr::can_replace_source(PRegister old_src, PVirtualValue new_src)
   {
      if (!check_readport_validation(old_src, new_src))
            /* If the old or new source is an array element, we assume that there
   * might have been an (untracked) indirect access, so don't replace
   * this source */
   if (old_src->pin() == pin_array && new_src->pin() == pin_array)
            auto [addr, dummy, index] = indirect_addr();
   auto addr_reg = addr ?  addr->as_register() : nullptr;
            if (auto u = new_src->as_uniform()) {
                  /* Don't mix indirect buffer and indirect registers, because the
   * scheduler can't handle it yet. */
                  /* Don't allow two different index registers, can't deal with that yet */
   if (index_reg && !index_reg->equal_to(*u->buf_addr()))
                  if (auto new_addr = new_src->get_addr()) {
      auto new_addr_reg = new_addr->as_register();
   bool new_addr_lowered = new_addr_reg &&
            if (addr_reg) {
      if (!addr_reg->equal_to(*new_addr) || new_addr_lowered ||
      addr_reg->has_flag(Register::addr_or_idx))
   }
   if (m_dest->has_flag(Register::addr_or_idx)) {
      if (new_src->pin() == pin_array) {
      auto s = static_cast<const LocalArrayValue *>(new_src)->addr();
   if (!s->as_inline_const() || !s->as_literal())
            }
      }
      void
   AluInstr::set_sources(SrcValues src)
   {
      for (auto& s : m_src) {
      auto r = s->as_register();
   if (r)
      }
   m_src.swap(src);
   for (auto& s : m_src) {
      auto r = s->as_register();
   if (r)
         }
      uint8_t AluInstr::allowed_src_chan_mask() const
   {
      if (m_alu_slots < 2)
                     for (auto s : m_src) {
      auto r = s->as_register();
   if (r)
      }
   /* Each channel can only be loaded in one of three cycles,
   * so if a channel is already used three times, we can't
   * add another source withthis channel.
   * Since we want to move away from one channel to another, it
   * is not important to know which is the old channel that will
   * be freed by the channel switch.*/
            /* Be conservative about channel use when using more than two
   * slots. Currently a constellatioon of
   *
   *  ALU d.x = f(r0.x, r1.y)
   *  ALU _.y = f(r2.y, r3.x)
   *  ALU _.z = f(r4.x, r5.y)
   *
   * will fail to be split. To get constellations like this to be scheduled
   * properly will need some work on the bank swizzle check.
   */
   int maxuse = m_alu_slots > 2 ? 2 : 3;
   for (int i = 0; i < 4; ++i) {
      if (chan_use_count[i] < maxuse)
      }
      }
      bool
   AluInstr::replace_dest(PRegister new_dest, AluInstr *move_instr)
   {
      if (m_dest->equal_to(*new_dest))
            if (m_dest->uses().size() > 1)
            if (new_dest->pin() == pin_array)
            /* Currently we bail out when an array write should be moved, because
   * declaring an array write is currently not well defined. The
   * Whole "backwards" copy propagation should dprobably be replaced by some
   * forward peep holew optimization */
   /*
   if (new_dest->pin() == pin_array) {
      auto dav = static_cast<const LocalArrayValue *>(new_dest)->addr();
   for (auto s: m_src) {
      if (s->pin() == pin_array) {
      auto sav = static_cast<const LocalArrayValue *>(s)->addr();
   if (dav && sav && dav->as_register() &&  !dav->equal_to(*sav))
            }
            if (m_dest->pin() == pin_chan && new_dest->chan() != m_dest->chan())
            if (m_dest->pin() == pin_chan) {
      if (new_dest->pin() == pin_group)
         else if (new_dest->pin() != pin_chgr)
               m_dest = new_dest;
   if (!move_instr->has_alu_flag(alu_last_instr))
            if (has_alu_flag(alu_is_cayman_trans)) {
      /* Copy propagation puts an instruction into the w channel, but we
   * don't have the slots for a w channel */
   if (m_dest->chan() == 3 && m_alu_slots < 4) {
      m_alu_slots = 4;
   assert(m_src.size() == 3);
                     }
      void
   AluInstr::pin_sources_to_chan()
   {
      for (auto s : m_src) {
      auto r = s->as_register();
   if (r) {
      if (r->pin() == pin_free)
         else if (r->pin() == pin_group)
            }
      bool
   AluInstr::check_readport_validation(PRegister old_src, PVirtualValue new_src) const
   {
      if (m_src.size() < 3)
            bool success = true;
            unsigned nsrc = alu_ops.at(m_opcode).nsrc;
            for (int s = 0; s < m_alu_slots && success; ++s) {
      PVirtualValue src[3];
            for (unsigned i = 0; i < nsrc; ++i, ++ireg)
            AluBankSwizzle bs = alu_vec_012;
   while (bs != alu_vec_unknown) {
      AluReadportReservation rpr = rpr_sum;
   if (rpr.schedule_vec_src(src, nsrc, bs)) {
      rpr_sum = rpr;
      }
               if (bs == alu_vec_unknown)
      }
      }
      void
   AluInstr::add_extra_dependency(PVirtualValue value)
   {
      auto reg = value->as_register();
   if (reg)
      }
      bool
   AluInstr::is_equal_to(const AluInstr& lhs) const
   {
      if (lhs.m_opcode != m_opcode || lhs.m_bank_swizzle != m_bank_swizzle ||
      lhs.m_cf_type != m_cf_type || lhs.m_alu_flags != m_alu_flags) {
               if (m_dest) {
      if (!lhs.m_dest) {
         } else {
      if (has_alu_flag(alu_write)) {
      if (!m_dest->equal_to(*lhs.m_dest))
      } else {
      if (m_dest->chan() != lhs.m_dest->chan())
            } else {
      if (lhs.m_dest)
               if (m_src.size() != lhs.m_src.size())
            for (unsigned i = 0; i < m_src.size(); ++i) {
      if (!m_src[i]->equal_to(*lhs.m_src[i]))
                  }
      class ResolveIndirectArrayAddr : public ConstRegisterVisitor {
   public:
      void visit(const Register& value) { (void)value; }
   void visit(const LocalArray& value)
   {
      (void)value;
      }
   void visit(const LocalArrayValue& value);
   void visit(const UniformValue& value);
   void visit(const LiteralConstant& value) { (void)value; }
            PRegister addr{nullptr};
   PRegister index{nullptr};
      };
      void
   ResolveIndirectArrayAddr::visit(const LocalArrayValue& value)
   {
      auto a = value.addr();
   if (a) {
      addr = a->as_register();
         }
      void
   ResolveIndirectArrayAddr::visit(const UniformValue& value)
   {
      auto a = value.buf_addr();
   if (a) {
            }
      std::tuple<PRegister, bool, PRegister>
   AluInstr::indirect_addr() const
   {
               if (m_dest) {
      m_dest->accept(visitor);
   if (visitor.addr)
               for (auto s : m_src) {
         }
      }
      AluGroup *
   AluInstr::split(ValueFactory& vf)
   {
      if (m_alu_slots == 1)
                                       int start_slot = 0;
   bool is_dot = m_opcode == op2_dot_ieee;
            if (is_dot) {
      start_slot = m_dest->chan();
                  for (int k = 0; k < m_alu_slots; ++k) {
               PRegister dst = m_dest->chan() == s ? m_dest : vf.dummy_dest(s);
   if (dst->pin() != pin_chgr) {
      auto pin = pin_chan;
   if (dst->pin() == pin_group && m_dest->chan() == s)
                     SrcValues src;
   int nsrc = alu_ops.at(m_opcode).nsrc;
   for (int i = 0; i < nsrc; ++i) {
      auto old_src = m_src[k * nsrc + i];
   // Make it easy for the scheduler and pin the register to the
   // channel, otherwise scheduler would have to check whether a
   // channel switch is possible
   auto r = old_src->as_register();
   if (r) {
      if (r->pin() == pin_free || r->pin() == pin_none)
         else if (r->pin() == pin_group)
      }
                           auto instr = new AluInstr(opcode, dst, src, {}, 1);
            if (s == 0 || !m_alu_flags.test(alu_64bit_op)) {
      if (has_source_mod(nsrc * k + 0, mod_neg))
         if (has_source_mod(nsrc * k + 1, mod_neg))
         if (has_source_mod(nsrc * k + 2, mod_neg))
         if (has_source_mod(nsrc * k + 0, mod_abs))
         if (has_source_mod(nsrc * k + 1, mod_abs))
      }
   if (has_alu_flag(alu_dst_clamp))
            if (s == m_dest->chan())
            m_dest->add_parent(instr);
            if (!group->add_instruction(instr)) {
                     }
            for (auto s : m_src) {
      auto r = s->as_register();
   if (r) {
            }
               }
      /* Alu instructions that have SSA dest registers increase the  regietsr
   * pressure Alu instructions that read from SSA registers may decresase the
   * register pressure hency evaluate a priorityx values based on register
   * pressure change */
   int
   AluInstr::register_priority() const
   {
      int priority = 0;
               if (m_dest) {
      if (m_dest->has_flag(Register::ssa) && has_alu_flag(alu_write)) {
      if (m_dest->pin() != pin_group && m_dest->pin() != pin_chgr &&
      !m_dest->addr())
   } else {
      // Arrays and registers are pre-allocated, hence scheduling
   // assignments early is unlikely to increase register pressure
                  for (const auto s : m_src) {
      auto r = s->as_register();
   if (r) {
      if (r->has_flag(Register::ssa)) {
      int pending = 0;
   for (auto b : r->uses()) {
      if (!b->is_scheduled())
      }
   if (pending == 1)
      }
   if (r->addr() && r->addr()->as_register())
      }
   if (s->as_uniform())
         }
      }
      bool
   AluInstr::propagate_death()
   {
      if (!m_dest)
            if (m_dest->pin() == pin_group || m_dest->pin() == pin_chan) {
      switch (m_opcode) {
   case op2_interp_x:
   case op2_interp_xy:
   case op2_interp_z:
   case op2_interp_zw:
      reset_alu_flag(alu_write);
      default:;
               if (m_dest->pin() == pin_array)
            /* We assume that nir does a good job in eliminating all ALU results that
   * are not needed, and we don't let copy propagation doesn't make the
   * instruction obsolete, so just keep all */
   if (has_alu_flag(alu_is_cayman_trans))
            for (auto& src : m_src) {
      auto reg = src->as_register();
   if (reg)
      }
      }
      bool
   AluInstr::has_lds_access() const
   {
         }
      bool
   AluInstr::has_lds_queue_read() const
   {
      for (auto& s : m_src) {
      auto ic = s->as_inline_const();
   if (!ic)
            if (ic->sel() == ALU_SRC_LDS_OQ_A_POP || ic->sel() == ALU_SRC_LDS_OQ_B_POP)
      }
      }
      struct OpDescr {
      union {
      EAluOp alu_opcode;
      };
      };
      static std::map<std::string, OpDescr> s_alu_map_by_name;
   static std::map<std::string, OpDescr> s_lds_map_by_name;
      Instr::Pointer
   AluInstr::from_string(istream& is, ValueFactory& value_factory, AluGroup *group, bool is_cayman)
   {
               while (is.good() && !is.eof()) {
      string t;
   is >> t;
   if (t.length() > 0) {
                     std::set<AluModifiers> flags;
                     if (*t == "LDS") {
      is_lds = true;
               string opstr = *t++;
            if (deststr == "CLAMP") {
      flags.insert(alu_dst_clamp);
               assert(*t == ":");
            if (is_lds) {
      auto op = s_lds_map_by_name.find(opstr);
   if (op == s_lds_map_by_name.end()) {
      for (auto [opcode, opdescr] : lds_ops) {
      if (opstr == opdescr.name) {
      op_descr.lds_opcode = opcode;
   op_descr.nsrc = opdescr.nsrc;
   s_alu_map_by_name[opstr] = op_descr;
                  if (op_descr.nsrc == -1) {
      std::cerr << "'" << opstr << "'";
   unreachable("Unknown opcode");
         } else {
            } else {
      auto op = s_alu_map_by_name.find(opstr);
   if (op == s_alu_map_by_name.end()) {
      for (auto [opcode, opdescr] : alu_ops) {
      if (opstr == opdescr.name) {
      op_descr = {{opcode}, opdescr.nsrc};
   s_alu_map_by_name[opstr] = op_descr;
                  if (op_descr.nsrc == -1) {
      std::cerr << "'" << opstr << "'";
   unreachable("Unknown opcode");
         } else {
         }
   if (is_cayman) {
      switch (op_descr.alu_opcode) {
   case op1_cos:
   case op1_exp_ieee:
   case op1_log_clamped:
   case op1_recip_ieee:
   case op1_recipsqrt_ieee1:
   case op1_sqrt_ieee:
   case op1_sin:
   case op2_mullo_int:
   case op2_mulhi_int:
   case op2_mulhi_uint:
         default:
   ;
                           uint32_t src_mods = 0;
   SrcValues sources;
   do {
      ++t;
   for (int i = 0; i < op_descr.nsrc; ++i) {
               if (srcstr[0] == '-') {
      src_mods |= AluInstr::mod_neg << (2 * sources.size());
               if (srcstr[0] == '|') {
      assert(srcstr[srcstr.length() - 1] == '|');
   src_mods |= AluInstr::mod_abs << (2 * sources.size());
               auto src = value_factory.src_from_string(srcstr);
   if (!src) {
      std::cerr << "Unable to create src[" << i << "] from " << srcstr << "\n";
      }
      }
               AluBankSwizzle bank_swizzle = alu_vec_unknown;
                        switch ((*t)[0]) {
   case '{': {
      auto iflag = t->begin() + 1;
   while (iflag != t->end()) {
                     switch (*iflag) {
   case 'L':
      flags.insert(alu_last_instr);
      case 'W':
      flags.insert(alu_write);
      case 'E':
      flags.insert(alu_update_exec);
      case 'P':
      flags.insert(alu_update_pred);
      }
                  case 'V': {
      string bs = *t;
   if (bs == "VEC_012")
         else if (bs == "VEC_021")
         else if (bs == "VEC_102")
         else if (bs == "VEC_120")
         else if (bs == "VEC_201")
         else if (bs == "VEC_210")
         else {
      std::cerr << "'" << bs << "': ";
                  default: {
      string cf_str = *t;
   if (cf_str == "PUSH_BEFORE")
         else if (cf_str == "POP_AFTER")
         else if (cf_str == "POP2_AFTER")
         else if (cf_str == "EXTENDED")
         else if (cf_str == "BREAK")
         else if (cf_str == "CONT")
         else if (cf_str == "ELSE_AFTER")
         else {
      std::cerr << " '" << cf_str << "' ";
         }
   }
               PRegister dest = nullptr;
   // construct instruction
   if (deststr != "(null)")
            AluInstr *retval = nullptr;
   if (is_lds)
         else
            retval->m_source_modifiers = src_mods;
   retval->set_bank_swizzle(bank_swizzle);
   retval->set_cf_type(cf);
   if (group) {
      group->add_instruction(retval);
      }
      }
      bool
   AluInstr::do_ready() const
   {
      /* Alu instructions are shuffled by the scheduler, so
   * we have to make sure that required ops are already
   * scheduled before marking this one ready */
   for (auto i : required_instr()) {
      if (i->is_dead())
            bool is_older_instr = i->block_id() <= block_id() &&
         bool is_lds = i->as_alu() && i->as_alu()->has_lds_access();
   if (!i->is_scheduled() && (is_older_instr || is_lds))
               for (auto s : m_src) {
      auto r = s->as_register();
   if (r) {
      if (!r->ready(block_id(), index()))
      }
   auto u = s->as_uniform();
   if (u && u->buf_addr() && u->buf_addr()->as_register()) {
      if (!u->buf_addr()->as_register()->ready(block_id(), index()))
                  if (m_dest && !m_dest->has_flag(Register::ssa)) {
      if (m_dest->pin() == pin_array) {
      auto av = static_cast<const LocalArrayValue *>(m_dest);
   auto addr = av->addr();
   /* For true indiect dest access we have to make sure that all
   * instructions that write the value before are schedukled */
   if (addr && (!addr->ready(block_id(), index()) ||
                     /* If a register is updates, we have to make sure that uses before that
   * update are scheduled, otherwise we may use the updated value when we
   * shouldn't */
   for (auto u : m_dest->uses()) {
      /* TODO: This is working around some sloppy use updates, dead instrzuctions
   * should remove themselves from uses. */
   if (u->is_dead())
         if (!u->is_scheduled() &&
      u->block_id() <= block_id() &&
   u->index() < index()) {
                     for (auto& r : m_extra_dependencies) {
      if (!r->ready(block_id(), index()))
                  }
      void
   AluInstrVisitor::visit(AluGroup *instr)
   {
      for (auto& i : *instr) {
      if (i)
         }
      void
   AluInstrVisitor::visit(Block *instr)
   {
      for (auto& i : *instr)
      }
      void
   AluInstrVisitor::visit(IfInstr *instr)
   {
         }
      bool AluInstr::is_kill() const
   {
      if (has_alu_flag(alu_is_lds))
            switch (m_opcode) {
   case op2_kille:
   case op2_kille_int:
   case op2_killne:
   case op2_killne_int:
   case op2_killge:
   case op2_killge_int:
   case op2_killge_uint:
   case op2_killgt:
   case op2_killgt_int:
   case op2_killgt_uint:
         default:
            }
      enum AluMods {
      mod_none,
   mod_src0_abs,
   mod_src0_neg,
      };
      static bool
   emit_alu_b2x(const nir_alu_instr& alu, AluInlineConstants mask, Shader& shader);
            static bool
   emit_alu_op1(const nir_alu_instr& alu,
               EAluOp opcode,
   static bool
   emit_alu_op1_64bit(const nir_alu_instr& alu,
                     static bool
   emit_alu_mov_64bit(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_neg(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_op1_64bit_trans(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_alu_op2_64bit(const nir_alu_instr& alu,
                     static bool
   emit_alu_op2_64bit_one_dst(const nir_alu_instr& alu,
                     static bool
   emit_alu_fma_64bit(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_alu_b2f64(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_f2f64(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_i2f64(const nir_alu_instr& alu, EAluOp op, Shader& shader);
   static bool
   emit_alu_f2f32(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_abs64(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_fsat64(const nir_alu_instr& alu, Shader& shader);
      static bool
   emit_alu_op2(const nir_alu_instr& alu,
               EAluOp opcode,
   static bool
   emit_alu_op2_int(const nir_alu_instr& alu,
                     static bool
   emit_alu_op3(const nir_alu_instr& alu,
               EAluOp opcode,
   static bool
   emit_any_all_fcomp2(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_any_all_fcomp(
         static bool
   emit_any_all_icomp(
            static bool
   emit_alu_comb_with_zero(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_unpack_64_2x32_split(const nir_alu_instr& alu, int comp, Shader& shader);
   static bool
   emit_pack_64_2x32(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_unpack_64_2x32(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_pack_64_2x32_split(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_pack_32_2x16_split(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_alu_vec2_64(const nir_alu_instr& alu, Shader& shader);
      static bool
   emit_unpack_32_2x16_split_x(const nir_alu_instr& alu, Shader& shader);
   static bool
   emit_unpack_32_2x16_split_y(const nir_alu_instr& alu, Shader& shader);
      static bool
   emit_dot(const nir_alu_instr& alu, int nelm, Shader& shader);
   static bool
   emit_dot4(const nir_alu_instr& alu, int nelm, Shader& shader);
   static bool
   emit_create_vec(const nir_alu_instr& instr, unsigned nc, Shader& shader);
      static bool
   emit_alu_trans_op1_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_alu_trans_op1_cayman(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
      static bool
   emit_alu_trans_op2_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
   static bool
   emit_alu_trans_op2_cayman(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
      static bool
   emit_alu_f2i32_or_u32_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader);
      static bool
   emit_tex_fdd(const nir_alu_instr& alu, TexInstr::Opcode opcode, bool fine, Shader& shader);
      static bool
   emit_alu_cube(const nir_alu_instr& alu, Shader& shader);
      static bool
   emit_fdph(const nir_alu_instr& alu, Shader& shader);
      static bool
   check_64_bit_op_src(nir_src *src, void *state)
   {
      if (nir_src_bit_size(*src) == 64) {
      *(bool *)state = true;
      }
      }
      static bool
   check_64_bit_op_def(nir_def *def, void *state)
   {
      if (def->bit_size == 64) {
      *(bool *)state = true;
      }
      }
      bool
   AluInstr::from_nir(nir_alu_instr *alu, Shader& shader)
   {
      bool is_64bit_op = false;
   nir_foreach_src(&alu->instr, check_64_bit_op_src, &is_64bit_op);
   if (!is_64bit_op)
            if (is_64bit_op) {
      switch (alu->op) {
   case nir_op_pack_64_2x32:
   case nir_op_unpack_64_2x32:
   case nir_op_pack_64_2x32_split:
   case nir_op_pack_half_2x16_split:
   case nir_op_unpack_64_2x32_split_x:
   case nir_op_unpack_64_2x32_split_y:
         case nir_op_mov:
         case nir_op_fneg:
         case nir_op_fsat:
         case nir_op_ffract:
         case nir_op_feq32:
         case nir_op_fge32:
         case nir_op_flt32:
         case nir_op_fneu32:
         case nir_op_ffma:
            case nir_op_fadd:
         case nir_op_fmul:
         case nir_op_fmax:
         case nir_op_fmin:
         case nir_op_b2f64:
         case nir_op_f2f64:
         case nir_op_i2f64:
         case nir_op_u2f64:
         case nir_op_f2f32:
         case nir_op_fabs:
         case nir_op_fsqrt:
         case nir_op_frcp:
         case nir_op_frsq:
         case nir_op_vec2:
         default:
      return false;
                  if (shader.chip_class() == ISA_CC_CAYMAN) {
      switch (alu->op) {
   case nir_op_fcos_amd:
         case nir_op_fexp2:
         case nir_op_flog2:
         case nir_op_frcp:
         case nir_op_frsq:
         case nir_op_fsqrt:
         case nir_op_fsin_amd:
         case nir_op_i2f32:
         case nir_op_u2f32:
         case nir_op_imul:
         case nir_op_imul_high:
         case nir_op_umul_high:
         case nir_op_f2u32:
         case nir_op_f2i32:
         case nir_op_ishl:
         case nir_op_ishr:
         case nir_op_ushr:
         default:;
      } else {
      if (shader.chip_class() == ISA_CC_EVERGREEN) {
      switch (alu->op) {
   case nir_op_f2i32:
         case nir_op_f2u32:
         default:;
               if (shader.chip_class() >= ISA_CC_R700) {
      switch (alu->op) {
   case nir_op_ishl:
         case nir_op_ishr:
         case nir_op_ushr:
         default:;
      } else {
      switch (alu->op) {
   case nir_op_ishl:
         case nir_op_ishr:
         case nir_op_ushr:
         default:;
               switch (alu->op) {
   case nir_op_f2i32:
         case nir_op_f2u32:
         case nir_op_fcos_amd:
         case nir_op_fexp2:
         case nir_op_flog2:
         case nir_op_frcp:
         case nir_op_frsq:
         case nir_op_fsin_amd:
         case nir_op_fsqrt:
         case nir_op_i2f32:
         case nir_op_u2f32:
         case nir_op_imul:
         case nir_op_imul_high:
         case nir_op_umul_high:
         default:;
               switch (alu->op) {
   case nir_op_b2b1:
         case nir_op_b2b32:
         case nir_op_b2f32:
         case nir_op_b2i32:
            case nir_op_bfm:
         case nir_op_bit_count:
            case nir_op_bitfield_reverse:
         case nir_op_bitfield_select:
            case nir_op_b32all_fequal2:
         case nir_op_b32all_fequal3:
         case nir_op_b32all_fequal4:
         case nir_op_b32all_iequal2:
         case nir_op_b32all_iequal3:
         case nir_op_b32all_iequal4:
         case nir_op_b32any_fnequal2:
         case nir_op_b32any_fnequal3:
         case nir_op_b32any_fnequal4:
         case nir_op_b32any_inequal2:
         case nir_op_b32any_inequal3:
         case nir_op_b32any_inequal4:
         case nir_op_b32csel:
            case nir_op_fabs:
         case nir_op_fadd:
         case nir_op_fceil:
         case nir_op_fcsel:
         case nir_op_fcsel_ge:
         case nir_op_fcsel_gt:
            case nir_op_fdph:
         case nir_op_fdot2:
      if (shader.chip_class() >= ISA_CC_EVERGREEN)
         else
      case nir_op_fdot3:
      if (shader.chip_class() >= ISA_CC_EVERGREEN)
         else
      case nir_op_fdot4:
            case nir_op_feq32:
   case nir_op_feq:
         case nir_op_ffloor:
         case nir_op_ffract:
         case nir_op_fge32:
         case nir_op_fge:
         case nir_op_find_lsb:
            case nir_op_flt32:
         case nir_op_flt:
         case nir_op_fmax:
         case nir_op_fmin:
            case nir_op_fmul:
      if (!shader.has_flag(Shader::sh_legacy_math_rules))
            case nir_op_fmulz:
            case nir_op_fneg:
         case nir_op_fneu32:
         case nir_op_fneu:
            case nir_op_fround_even:
         case nir_op_fsat:
         case nir_op_fsub:
         case nir_op_ftrunc:
         case nir_op_iadd:
         case nir_op_iand:
         case nir_op_ibfe:
         case nir_op_i32csel_ge:
         case nir_op_i32csel_gt:
         case nir_op_ieq32:
         case nir_op_ieq:
         case nir_op_ifind_msb_rev:
         case nir_op_ige32:
         case nir_op_ige:
         case nir_op_ilt32:
         case nir_op_ilt:
         case nir_op_imax:
         case nir_op_imin:
         case nir_op_ine32:
         case nir_op_ine:
         case nir_op_ineg:
         case nir_op_inot:
         case nir_op_ior:
         case nir_op_isub:
         case nir_op_ixor:
         case nir_op_pack_64_2x32:
         case nir_op_unpack_64_2x32:
         case nir_op_pack_64_2x32_split:
         case nir_op_pack_half_2x16_split:
         case nir_op_slt:
         case nir_op_sge:
         case nir_op_seq:
         case nir_op_sne:
         case nir_op_ubfe:
         case nir_op_ufind_msb_rev:
         case nir_op_uge32:
         case nir_op_uge:
         case nir_op_ult32:
         case nir_op_ult:
         case nir_op_umad24:
         case nir_op_umax:
         case nir_op_umin:
         case nir_op_umul24:
         case nir_op_unpack_64_2x32_split_x:
         case nir_op_unpack_64_2x32_split_y:
         case nir_op_unpack_half_2x16_split_x:
         case nir_op_unpack_half_2x16_split_y:
            case nir_op_ffma:
      if (!shader.has_flag(Shader::sh_legacy_math_rules))
            case nir_op_ffmaz:
            case nir_op_mov:
         case nir_op_f2i32:
         case nir_op_vec2:
         case nir_op_vec3:
         case nir_op_vec4:
            case nir_op_fddx:
   case nir_op_fddx_coarse:
         case nir_op_fddx_fine:
         case nir_op_fddy:
   case nir_op_fddy_coarse:
         case nir_op_fddy_fine:
         case nir_op_cube_amd:
         default:
      fprintf(stderr, "Unknown instruction '");
   nir_print_instr(&alu->instr, stderr);
   fprintf(stderr, "'\n");
   assert(0);
         }
      static Pin
   pin_for_components(const nir_alu_instr& alu)
   {
         }
      static bool
   emit_alu_op1_64bit(const nir_alu_instr& alu,
                     {
                                 int swz[2] = {0, 1};
   if (switch_chan) {
      swz[0] = 1;
               for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                              ir = new AluInstr(opcode,
                        }
   if (ir)
         shader.emit_instruction(group);
      }
      static bool
   emit_alu_mov_64bit(const nir_alu_instr& alu, Shader& shader)
   {
                        for (unsigned i = 0; i < alu.def.num_components; ++i) {
      for (unsigned c = 0; c < 2; ++c) {
      ir = new AluInstr(op1_mov,
                           }
   if (ir)
            }
      static bool
   emit_alu_neg(const nir_alu_instr& alu, Shader& shader)
   {
                        for (unsigned i = 0; i < alu.def.num_components; ++i) {
      for (unsigned c = 0; c < 2; ++c) {
      ir = new AluInstr(op1_mov,
                        }
      }
   if (ir)
               }
      static bool
   emit_alu_abs64(const nir_alu_instr& alu, Shader& shader)
   {
                        shader.emit_instruction(new AluInstr(op1_mov,
                        auto ir = new AluInstr(op1_mov,
                     ir->set_source_mod(0, AluInstr::mod_abs);
   shader.emit_instruction(ir);
      }
      static bool
   try_propagat_fsat64(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto src0 = value_factory.src64(alu.src[0], 0, 0);
   auto reg0 = src0->as_register();
   if (!reg0)
            if (!reg0->has_flag(Register::ssa))
            if (reg0->parents().size() != 1)
            if (!reg0->uses().empty())
            auto parent = (*reg0->parents().begin())->as_alu();
   if (!parent)
            auto opinfo = alu_ops.at(parent->opcode());
   if (!opinfo.can_clamp)
            parent->set_alu_flag(alu_dst_clamp);
      }
         static bool
   emit_alu_fsat64(const nir_alu_instr& alu, Shader& shader)
   {
                        if (try_propagat_fsat64(alu, shader)) {
      auto ir = new AluInstr(op1_mov,
                              shader.emit_instruction(new AluInstr(op1_mov,
                              /* dest clamp doesn't work on plain 64 bit move, so add a zero
            auto group = new AluGroup();
   auto ir = new AluInstr(op2_add_64,
                           ir->set_alu_flag(alu_dst_clamp);
            group->add_instruction(new AluInstr(op2_add_64,
                                 }
      }
         static bool
   emit_alu_op2_64bit(const nir_alu_instr& alu,
                     {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
   AluInstr *ir = nullptr;
   int order[2] = {0, 1};
   if (switch_src) {
      order[0] = 1;
                                 for (unsigned k = 0; k < alu.def.num_components; ++k) {
      int i = 0;
   for (; i < num_emit0; ++i) {
                     ir = new AluInstr(opcode,
                     dest,
               auto dest =
            ir = new AluInstr(opcode,
                     dest,
      }
   if (ir)
            shader.emit_instruction(group);
      }
      static bool
   emit_alu_op2_64bit_one_dst(const nir_alu_instr& alu,
                     {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   int order[2] = {0, 1};
   if (switch_order) {
      order[0] = 1;
                        for (unsigned k = 0; k < alu.def.num_components; ++k) {
      auto dest = value_factory.dest(alu.def, 2 * k, pin_chan);
   src[0] = value_factory.src64(alu.src[order[0]], k, 1);
   src[1] = value_factory.src64(alu.src[order[1]], k, 1);
   src[2] = value_factory.src64(alu.src[order[0]], k, 0);
            ir = new AluInstr(opcode, dest, src, AluInstr::write, 2);
               }
   if (ir)
               }
      static bool
   emit_alu_op1_64bit_trans(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 3; ++i) {
      ir = new AluInstr(opcode,
                     i < 2 ? value_factory.dest(alu.def, i, pin_chan)
            if (opcode == op1_sqrt_64)
            }
   if (ir)
         shader.emit_instruction(group);
      }
      static bool
   emit_alu_fma_64bit(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
   AluInstr *ir = nullptr;
               int chan = i < 3 ? 1 : 0;
   auto dest =
            ir = new AluInstr(opcode,
                     dest,
   value_factory.src64(alu.src[0], 0, chan),
      }
   if (ir)
         shader.emit_instruction(group);
      }
      static bool
   emit_alu_b2f64(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
            for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(op2_and_int,
                     value_factory.dest(alu.def, 2 * i, pin_group),
            ir = new AluInstr(op2_and_int,
                     value_factory.dest(alu.def, 2 * i + 1, pin_group),
      }
   if (ir)
         shader.emit_instruction(group);
      }
      static bool
   emit_alu_i2f64(const nir_alu_instr& alu, EAluOp op, Shader& shader)
   {
      /* int 64 to f64 should have been lowered, so we only handle i32 to f64 */
   auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
                     auto tmpx = value_factory.temp_register();
   shader.emit_instruction(new AluInstr(op2_and_int,
                           auto tmpy = value_factory.temp_register();
   shader.emit_instruction(new AluInstr(op2_and_int,
                              auto tmpx2 = value_factory.temp_register();
   auto tmpy2 = value_factory.temp_register();
   shader.emit_instruction(new AluInstr(op, tmpx2, tmpx, AluInstr::last_write));
            auto tmpx3 = value_factory.temp_register(0);
   auto tmpy3 = value_factory.temp_register(1);
   auto tmpz3 = value_factory.temp_register(2);
            ir = new AluInstr(op1_flt32_to_flt64, tmpx3, tmpx2, AluInstr::write);
   group->add_instruction(ir);
   ir = new AluInstr(op1_flt32_to_flt64, tmpy3, value_factory.zero(), AluInstr::write);
   group->add_instruction(ir);
   ir = new AluInstr(op1_flt32_to_flt64, tmpz3, tmpy2, AluInstr::write);
   group->add_instruction(ir);
   ir =
         group->add_instruction(ir);
                     ir = new AluInstr(op2_add_64,
                     value_factory.dest(alu.def, 0, pin_chan),
   group->add_instruction(ir);
   ir = new AluInstr(op2_add_64,
                     value_factory.dest(alu.def, 1, pin_chan),
   group->add_instruction(ir);
               }
      static bool
   emit_alu_f2f64(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
                     ir = new AluInstr(op1_flt32_to_flt64,
                     group->add_instruction(ir);
   ir = new AluInstr(op1_flt32_to_flt64,
                     group->add_instruction(ir);
   shader.emit_instruction(group);
      }
      static bool
   emit_alu_f2f32(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto group = new AluGroup();
            ir = new AluInstr(op1v_flt64_to_flt32,
                     group->add_instruction(ir);
   ir = new AluInstr(op1v_flt64_to_flt32,
                     group->add_instruction(ir);
   shader.emit_instruction(group);
      }
      static bool
   emit_alu_b2x(const nir_alu_instr& alu, AluInlineConstants mask, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
            for (unsigned i = 0; i < alu.def.num_components; ++i) {
      auto src = value_factory.src(alu.src[0], i);
   ir = new AluInstr(op2_and_int,
                     value_factory.dest(alu.def, i, pin),
      }
   if (ir)
            }
      static bool
   emit_alu_op1(const nir_alu_instr& alu,
               EAluOp opcode,
   {
               AluInstr *ir = nullptr;
            for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     switch (mod) {
   case mod_src0_abs:
         case mod_src0_neg:
         case mod_dest_clamp:
      ir->set_alu_flag(alu_dst_clamp);
      }
      }
   if (ir)
            }
      static bool
   emit_alu_op2(const nir_alu_instr& alu,
               EAluOp opcode,
   {
      auto& value_factory = shader.value_factory();
   const nir_alu_src *src0 = &alu.src[0];
            int idx0 = 0;
   int idx1 = 1;
   if (opts & AluInstr::op2_opt_reverse) {
      std::swap(src0, src1);
                        auto pin = pin_for_components(alu);
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     value_factory.dest(alu.def, i, pin),
   if (src1_negate)
            }
   if (ir)
            }
      static bool
   emit_alu_op2_int(const nir_alu_instr& alu,
                     {
         }
      static bool
   emit_alu_op3(const nir_alu_instr& alu,
               EAluOp opcode,
   {
      auto& value_factory = shader.value_factory();
   const nir_alu_src *src[3];
   src[0] = &alu.src[src_shuffle[0]];
   src[1] = &alu.src[src_shuffle[1]];
            auto pin = pin_for_components(alu);
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     value_factory.dest(alu.def, i, pin),
   value_factory.src(*src[0], i),
   ir->set_alu_flag(alu_write);
      }
   if (ir)
            }
      static bool
   emit_any_all_fcomp2(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      AluInstr *ir = nullptr;
            PRegister tmp[2];
   tmp[0] = value_factory.temp_register();
            for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(opcode,
                     tmp[i],
      }
            opcode = (opcode == op2_setne_dx10) ? op2_or_int : op2_and_int;
   ir = new AluInstr(opcode,
                     value_factory.dest(alu.def, 0, pin_free),
   shader.emit_instruction(ir);
      }
      static bool
   emit_any_all_fcomp(const nir_alu_instr& alu, EAluOp op, int nc, bool all, Shader& shader)
   {
      /* This should probabyl be lowered in nir */
            AluInstr *ir = nullptr;
   RegisterVec4 v = value_factory.temp_vec4(pin_group);
            for (int i = 0; i < nc; ++i) {
                  for (int i = nc; i < 4; ++i)
            for (int i = 0; i < nc; ++i) {
      ir = new AluInstr(op,
                     v[i],
      }
   if (ir)
                              if (all) {
      ir->set_source_mod(0, AluInstr::mod_neg);
   ir->set_source_mod(1, AluInstr::mod_neg);
   ir->set_source_mod(2, AluInstr::mod_neg);
                        if (all)
         else
            ir = new AluInstr(op,
                     value_factory.dest(alu.def, 0, pin_free),
   if (all)
                     }
      static bool
   emit_any_all_icomp(const nir_alu_instr& alu, EAluOp op, int nc, bool all, Shader& shader)
   {
      /* This should probabyl be lowered in nir */
            AluInstr *ir = nullptr;
                     for (int i = 0; i < nc + nc / 2; ++i)
                     for (int i = 0; i < nc; ++i) {
      ir = new AluInstr(op,
                     v[i],
      }
   if (ir)
            if (nc == 2) {
      ir = new AluInstr(combine, dest, v[0], v[1], AluInstr::last_write);
   shader.emit_instruction(ir);
               if (nc == 3) {
      ir = new AluInstr(combine, v[3], v[0], v[1], AluInstr::last_write);
   shader.emit_instruction(ir);
   ir = new AluInstr(combine, dest, v[3], v[2], AluInstr::last_write);
   shader.emit_instruction(ir);
               if (nc == 4) {
      ir = new AluInstr(combine, v[4], v[0], v[1], AluInstr::write);
   shader.emit_instruction(ir);
   ir = new AluInstr(combine, v[5], v[2], v[3], AluInstr::last_write);
   shader.emit_instruction(ir);
   ir = new AluInstr(combine, dest, v[4], v[5], AluInstr::last_write);
   shader.emit_instruction(ir);
                  }
      static bool
   emit_dot(const nir_alu_instr& alu, int n, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   const nir_alu_src& src0 = alu.src[0];
                              for (int i = 0; i < n; ++i) {
      srcs[2 * i] = value_factory.src(src0, i);
                        shader.emit_instruction(ir);
               }
      static bool
   emit_dot4(const nir_alu_instr& alu, int nelm, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   const nir_alu_src& src0 = alu.src[0];
                              for (int i = 0; i < nelm; ++i) {
      srcs[2 * i] = value_factory.src(src0, i);
      }
      for (int i = nelm; i < 4; ++i) {
      srcs[2 * i] = value_factory.zero();
                        shader.emit_instruction(ir);
      }
      static bool
   emit_fdph(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   const nir_alu_src& src0 = alu.src[0];
                              for (int i = 0; i < 3; ++i) {
      srcs[2 * i] = value_factory.src(src0, i);
               srcs[6] = value_factory.one();
            AluInstr *ir = new AluInstr(op2_dot4_ieee, dest, srcs, AluInstr::last_write, 4);
   shader.emit_instruction(ir);
      }
      static bool
   emit_create_vec(const nir_alu_instr& instr, unsigned nc, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
            for (unsigned i = 0; i < nc; ++i) {
      auto src = value_factory.src(instr.src[i].src, instr.src[i].swizzle[0]);
   auto dst = value_factory.dest(instr.def, i, pin_none);
               if (ir)
            }
      static bool
   emit_alu_comb_with_zero(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   auto pin = pin_for_components(alu);
   for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     value_factory.dest(alu.def, i, pin),
      }
   if (ir)
               }
      static bool
   emit_pack_64_2x32_split(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   ir->set_alu_flag(alu_last_instr);
      }
      static bool
   emit_pack_64_2x32(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   ir->set_alu_flag(alu_last_instr);
      }
      static bool
   emit_unpack_64_2x32(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   ir->set_alu_flag(alu_last_instr);
      }
      bool
   emit_alu_vec2_64(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   AluInstr *ir = nullptr;
   for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   for (unsigned i = 0; i < 2; ++i) {
      ir = new AluInstr(op1_mov,
                        }
   ir->set_alu_flag(alu_last_instr);
      }
      static bool
   emit_pack_32_2x16_split(const nir_alu_instr& alu, Shader& shader)
   {
               auto x = value_factory.temp_register();
   auto y = value_factory.temp_register();
            shader.emit_instruction(new AluInstr(
            shader.emit_instruction(new AluInstr(
            shader.emit_instruction(
            shader.emit_instruction(new AluInstr(op2_or_int,
                              }
      static bool
   emit_unpack_64_2x32_split(const nir_alu_instr& alu, int comp, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   shader.emit_instruction(new AluInstr(op1_mov,
                        }
      static bool
   emit_unpack_32_2x16_split_x(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   shader.emit_instruction(new AluInstr(op1_flt16_to_flt32,
                        }
   static bool
   emit_unpack_32_2x16_split_y(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
   auto tmp = value_factory.temp_register();
   shader.emit_instruction(new AluInstr(op2_lshr_int,
                              shader.emit_instruction(new AluInstr(op1_flt16_to_flt32,
                        }
      static bool
   emit_alu_trans_op1_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
            AluInstr *ir = nullptr;
            for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     ir->set_alu_flag(alu_is_trans);
                  }
      static bool
   emit_alu_f2i32_or_u32_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
                              for (int i = 0; i < num_comp; ++i) {
      reg[i] = value_factory.temp_register();
   ir = new AluInstr(op1_trunc,
                                 auto pin = pin_for_components(alu);
   for (int i = 0; i < num_comp; ++i) {
      ir = new AluInstr(opcode,
                     if (opcode == op1_flt_to_uint) {
      ir->set_alu_flag(alu_is_trans);
      }
      }
   ir->set_alu_flag(alu_last_instr);
      }
      static bool
   emit_alu_trans_op1_cayman(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
                              for (unsigned j = 0; j < alu.def.num_components; ++j) {
               AluInstr::SrcValues srcs(ncomp);
            for (unsigned i = 0; i < ncomp; ++i)
            auto ir = new AluInstr(opcode, dest, srcs, flags, ncomp);
      }
      }
      static bool
   emit_alu_trans_op2_eg(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
               const nir_alu_src& src0 = alu.src[0];
                     auto pin = pin_for_components(alu);
   for (unsigned i = 0; i < alu.def.num_components; ++i) {
      ir = new AluInstr(opcode,
                     value_factory.dest(alu.def, i, pin),
   ir->set_alu_flag(alu_is_trans);
      }
      }
      static bool
   emit_alu_trans_op2_cayman(const nir_alu_instr& alu, EAluOp opcode, Shader& shader)
   {
               const nir_alu_src& src0 = alu.src[0];
                              for (unsigned k = 0; k < alu.def.num_components; ++k) {
      AluInstr::SrcValues srcs(2 * last_slot);
            for (unsigned i = 0; i < last_slot; ++i) {
      srcs[2 * i] = value_factory.src(src0, k);
               auto ir = new AluInstr(opcode, dest, srcs, flags, last_slot);
   ir->set_alu_flag(alu_is_cayman_trans);
      }
      }
      static bool
   emit_tex_fdd(const nir_alu_instr& alu, TexInstr::Opcode opcode, bool fine, Shader& shader)
   {
               int ncomp = alu.def.num_components;
   RegisterVec4::Swizzle src_swz = {7, 7, 7, 7};
   RegisterVec4::Swizzle tmp_swz = {7, 7, 7, 7};
   for (auto i = 0; i < ncomp; ++i) {
      src_swz[i] = alu.src[0].swizzle[i];
                        auto tmp = value_factory.temp_vec4(pin_group, tmp_swz);
   AluInstr *mv = nullptr;
   for (int i = 0; i < ncomp; ++i) {
      mv = new AluInstr(op1_mov, tmp[i], src[i], AluInstr::write);
      }
   if (mv)
            auto dst = value_factory.dest_vec4(alu.def, pin_group);
   RegisterVec4::Swizzle dst_swz = {7, 7, 7, 7};
   for (auto i = 0; i < ncomp; ++i) {
                           if (fine)
                        }
      static bool
   emit_alu_cube(const nir_alu_instr& alu, Shader& shader)
   {
      auto& value_factory = shader.value_factory();
            const uint16_t src0_chan[4] = {2, 2, 0, 1};
                                 ir = new AluInstr(op2_cube,
                     value_factory.dest(alu.def, i, pin_chan),
      }
   ir->set_alu_flag(alu_last_instr);
   shader.emit_instruction(group);
      }
      const std::set<AluModifiers> AluInstr::empty;
   const std::set<AluModifiers> AluInstr::write({alu_write});
   const std::set<AluModifiers> AluInstr::last({alu_last_instr});
   const std::set<AluModifiers> AluInstr::last_write({alu_write, alu_last_instr});
      } // namespace r600
