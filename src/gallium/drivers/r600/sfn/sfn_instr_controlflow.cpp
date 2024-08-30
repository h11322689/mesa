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
      #include "sfn_instr_controlflow.h"
      #include <sstream>
      namespace r600 {
      ControlFlowInstr::ControlFlowInstr(CFType type):
         {
   }
      bool
   ControlFlowInstr::do_ready() const
   {
      /* Have to rework this, but the CF should always */
      }
      bool
   ControlFlowInstr::is_equal_to(const ControlFlowInstr& rhs) const
   {
         }
      void
   ControlFlowInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   ControlFlowInstr::accept(InstrVisitor& visitor)
   {
         }
      void
   ControlFlowInstr::do_print(std::ostream& os) const
   {
      switch (m_type) {
   case cf_else:
      os << "ELSE";
      case cf_endif:
      os << "ENDIF";
      case cf_loop_begin:
      os << "LOOP_BEGIN";
      case cf_loop_end:
      os << "LOOP_END";
      case cf_loop_break:
      os << "BREAK";
      case cf_loop_continue:
      os << "CONTINUE";
      case cf_wait_ack:
      os << "WAIT_ACK";
      default:
            }
      Instr::Pointer
   ControlFlowInstr::from_string(std::string type_str)
   {
      if (type_str == "ELSE")
         else if (type_str == "ENDIF")
         else if (type_str == "LOOP_BEGIN")
         else if (type_str == "LOOP_END")
         else if (type_str == "BREAK")
         else if (type_str == "CONTINUE")
         else if (type_str == "WAIT_ACK")
         else
      }
      int
   ControlFlowInstr::nesting_corr() const
   {
      switch (m_type) {
   case cf_else:
   case cf_endif:
   case cf_loop_end:
         default:
            }
      int
   ControlFlowInstr::nesting_offset() const
   {
      switch (m_type) {
   case cf_endif:
   case cf_loop_end:
         case cf_loop_begin:
         default:
            }
      IfInstr::IfInstr(AluInstr *pred):
         {
         }
      IfInstr::IfInstr(const IfInstr& orig) { m_predicate = new AluInstr(*orig.m_predicate); }
      bool
   IfInstr::is_equal_to(const IfInstr& rhs) const
   {
         }
      uint32_t IfInstr::slots() const
   {
      /* If we hava a literal value in the predicate evaluation, then
   * we need at most two alu slots, otherwise it's just one. */
   for (auto s : m_predicate->sources())
      if (s->as_literal())
         };
      void
   IfInstr::accept(ConstInstrVisitor& visitor) const
   {
         }
      void
   IfInstr::accept(InstrVisitor& visitor)
   {
         }
      bool
   IfInstr::replace_source(PRegister old_src, PVirtualValue new_src)
   {
         }
      bool
   IfInstr::do_ready() const
   {
         }
      void
   IfInstr::forward_set_scheduled()
   {
         }
      void
   IfInstr::forward_set_blockid(int id, int index)
   {
         }
      void
   IfInstr::do_print(std::ostream& os) const
   {
         }
      void
   IfInstr::set_predicate(AluInstr *new_predicate)
   {
      m_predicate = new_predicate;
      }
      Instr::Pointer
   IfInstr::from_string(std::istream& is, ValueFactory& value_factory, bool is_cayman)
   {
      std::string pred_start;
   is >> pred_start;
   if (pred_start != "((")
                  is.get(buf, 2048, ')');
   std::string pred_end;
            if (pred_end != "))") {
                           std::string instr_type;
            if (instr_type != "ALU")
            auto pred = AluInstr::from_string(bufstr, value_factory, nullptr, is_cayman);
      }
      } // namespace r600
