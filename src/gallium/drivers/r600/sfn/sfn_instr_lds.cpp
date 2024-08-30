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
      #include "sfn_instr_lds.h"
      #include "sfn_debug.h"
   #include "sfn_instr_alu.h"
      namespace r600 {
      using std::istream;
      LDSReadInstr::LDSReadInstr(std::vector<PRegister, Allocator<PRegister>>& value,
            m_address(address),
      {
               for (auto& v : value)
            for (auto& s : m_address)
      if (s->as_register())
   }
      void
   LDSReadInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   LDSReadInstr::accept(InstrVisitor& visitor)
   {
         }
      bool
   LDSReadInstr::remove_unused_components()
   {
      uint8_t inactive_mask = 0;
   for (size_t i = 0; i < m_dest_value.size(); ++i) {
      if (m_dest_value[i]->uses().empty())
               if (!inactive_mask)
            auto new_addr = AluInstr::SrcValues();
            for (size_t i = 0; i < m_dest_value.size(); ++i) {
      if ((1 << i) & inactive_mask) {
      if (m_address[i]->as_register())
            } else {
      new_dest.push_back(m_dest_value[i]);
                  m_dest_value.swap(new_dest);
               }
      class SetLDSAddrProperty : public AluInstrVisitor {
      using AluInstrVisitor::visit;
      };
      AluInstr *
   LDSReadInstr::split(std::vector<AluInstr *>& out_block, AluInstr *last_lds_instr)
   {
      AluInstr *first_instr = nullptr;
   SetLDSAddrProperty prop;
   for (auto& addr : m_address) {
      auto reg = addr->as_register();
   if (reg) {
      reg->del_use(this);
   if (reg->parents().size() == 1) {
      for (auto& p : reg->parents()) {
                        auto instr = new AluInstr(DS_OP_READ_RET, nullptr, nullptr, addr);
            if (last_lds_instr)
         out_block.push_back(instr);
   last_lds_instr = instr;
   if (!first_instr) {
      first_instr = instr;
      } else {
      /* In order to make it possible that the scheduler
   * keeps the loads of a group close together, we
   * require that the addresses are all already available
   * when the first read instruction is emitted.
   * Otherwise it might happen that the loads and reads from the
   * queue are split across ALU cf clauses, and this is not allowed */
                  for (auto& dest : m_dest_value) {
      dest->del_parent(this);
   auto instr = new AluInstr(op1_mov,
                     instr->add_required_instr(last_lds_instr);
   instr->set_blockid(block_id(), index());
   instr->set_always_keep();
   out_block.push_back(instr);
      }
   if (last_lds_instr)
               }
      bool
   LDSReadInstr::do_ready() const
   {
      unreachable("This instruction is not handled by the scheduler");
      }
      void
   LDSReadInstr::do_print(std::ostream& os) const
   {
               os << "[ ";
   for (auto d : m_dest_value) {
         }
   os << "] : [ ";
   for (auto a : m_address) {
         }
      }
      bool
   LDSReadInstr::is_equal_to(const LDSReadInstr& rhs) const
   {
      if (m_address.size() != rhs.m_address.size())
            for (unsigned i = 0; i < num_values(); ++i) {
      if (!m_address[i]->equal_to(*rhs.m_address[i]))
         if (!m_dest_value[i]->equal_to(*rhs.m_dest_value[i]))
      }
      }
      auto
   LDSReadInstr::from_string(istream& is, ValueFactory& value_factory) -> Pointer
   {
                        is >> temp_str;
            std::vector<PRegister, Allocator<PRegister>> dests;
            is >> temp_str;
   while (temp_str != "]") {
      auto dst = value_factory.dest_from_string(temp_str);
   assert(dst);
   dests.push_back(dst);
               is >> temp_str;
   assert(temp_str == ":");
   is >> temp_str;
            is >> temp_str;
   while (temp_str != "]") {
      auto src = value_factory.src_from_string(temp_str);
   assert(src);
   srcs.push_back(src);
      };
               }
      bool LDSReadInstr::replace_dest(PRegister new_dest, AluInstr *move_instr)
   {
      if (new_dest->pin() == pin_array)
                              for (unsigned i = 0; i < m_dest_value.size(); ++i) {
               if (!dest->equal_to(*old_dest))
            if (dest->equal_to(*new_dest))
            if (dest->uses().size() > 1)
            if (dest->pin() == pin_fully)
            if (dest->pin() == pin_group)
            if (dest->pin() == pin_chan && new_dest->chan() != dest->chan())
            if (dest->pin() == pin_chan) {
      if (new_dest->pin() == pin_group)
         else
      }
   m_dest_value[i] = new_dest;
      }
      }
      LDSAtomicInstr::LDSAtomicInstr(ESDOp op,
                        m_opcode(op),
   m_address(address),
   m_dest(dest),
      {
      if (m_dest)
            if (m_address->as_register())
            for (auto& s : m_srcs) {
      if (s->as_register())
         }
      void
   LDSAtomicInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   LDSAtomicInstr::accept(InstrVisitor& visitor)
   {
         }
      AluInstr *
   LDSAtomicInstr::split(std::vector<AluInstr *>& out_block, AluInstr *last_lds_instr)
   {
               for (auto& s : m_srcs)
            for (auto& s : srcs) {
      if (s->as_register())
               SetLDSAddrProperty prop;
   auto reg = srcs[0]->as_register();
   if (reg) {
      reg->del_use(this);
   if (reg->parents().size() == 1) {
      for (auto& p : reg->parents()) {
                        auto op_instr = new AluInstr(m_opcode, srcs, {});
            if (last_lds_instr) {
         }
            out_block.push_back(op_instr);
   if (m_dest) {
      op_instr->set_alu_flag(alu_lds_group_start);
   m_dest->del_parent(this);
   auto read_instr = new AluInstr(op1_mov,
                     read_instr->add_required_instr(op_instr);
   read_instr->set_blockid(block_id(), index());
   read_instr->set_alu_flag(alu_lds_group_end);
   out_block.push_back(read_instr);
      }
      }
      bool
   LDSAtomicInstr::replace_source(PRegister old_src, PVirtualValue new_src)
   {
               if (new_src->as_uniform()) {
      if (m_srcs.size() > 2) {
      int nconst = 0;
   for (auto& s : m_srcs) {
      if (s->as_uniform() && !s->equal_to(*old_src))
      }
   /* Conservative check: with two kcache values can always live,
   * tree might be a problem, don't care for now, just reject
   */
   if (nconst > 2)
               /* indirect constant buffer access means new CF, and this is something
   * we can't do in the middle of an LDS read group */
   auto u = new_src->as_uniform();
   if (u->buf_addr())
               /* If the source is an array element, we assume that there
   * might have been an (untracked) indirect access, so don't replace
   * this source */
   if (old_src->pin() == pin_array || new_src->pin() == pin_array)
            for (unsigned i = 0; i < m_srcs.size(); ++i) {
      if (old_src->equal_to(*m_srcs[i])) {
      m_srcs[i] = new_src;
                  if (process) {
      auto r = new_src->as_register();
   if (r)
            }
      }
      bool
   LDSAtomicInstr::do_ready() const
   {
      unreachable("This instruction is not handled by the scheduler");
      }
      void
   LDSAtomicInstr::do_print(std::ostream& os) const
   {
      auto ii = lds_ops.find(m_opcode);
            os << "LDS " << ii->second.name << " ";
   if (m_dest)
         else
            os << " [ " << *m_address << " ] : " << *m_srcs[0];
   if (m_srcs.size() > 1)
      }
      bool
   LDSAtomicInstr::is_equal_to(const LDSAtomicInstr& rhs) const
   {
      if (m_srcs.size() != rhs.m_srcs.size())
            for (unsigned i = 0; i < m_srcs.size(); ++i) {
      if (!m_srcs[i]->equal_to(*rhs.m_srcs[i]))
               return m_opcode == rhs.m_opcode && sfn_value_equal(m_address, rhs.m_address) &&
      }
      auto
   LDSAtomicInstr::from_string(istream& is, ValueFactory& value_factory) -> Pointer
   {
      /* LDS WRITE2 __.x [ R1.x ] : R2.y R3.z */
   /* LDS WRITE __.x [ R1.x ] : R2.y  */
                              ESDOp opcode = DS_OP_INVALID;
            for (auto& [op, opinfo] : lds_ops) {
      if (temp_str == opinfo.name) {
      opcode = op;
   nsrc = opinfo.nsrc;
                                    PRegister dest = nullptr;
   if (temp_str[0] != '_')
            is >> temp_str;
   assert(temp_str == "[");
   is >> temp_str;
            is >> temp_str;
            is >> temp_str;
            AluInstr::SrcValues srcs;
   for (int i = 0; i < nsrc - 1; ++i) {
      is >> temp_str;
   auto src = value_factory.src_from_string(temp_str);
   assert(src);
                  }
      } // namespace r600
