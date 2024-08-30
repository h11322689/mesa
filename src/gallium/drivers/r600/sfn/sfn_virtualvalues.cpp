   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2021 Collabora LTD
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
      #include "sfn_virtualvalues.h"
      #include "sfn_alu_defines.h"
   #include "sfn_debug.h"
   #include "sfn_instr.h"
   #include "sfn_valuefactory.h"
   #include "util/macros.h"
   #include "util/u_math.h"
      #include <iomanip>
   #include <iostream>
   #include <limits>
   #include <ostream>
   #include <sstream>
      namespace r600 {
      std::ostream&
   operator<<(std::ostream& os, Pin pin)
   {
   #define PRINT_PIN(X)                                                                     \
      case pin_##X:                                                                         \
      os << #X;                                                                          \
      switch (pin) {
      PRINT_PIN(chan);
   PRINT_PIN(array);
   PRINT_PIN(fully);
   PRINT_PIN(group);
   PRINT_PIN(chgr);
      case pin_none:
   default:;
      #undef PRINT_PIN
         }
      VirtualValue::VirtualValue(int sel, int chan, Pin pin):
      m_sel(sel),
   m_chan(chan),
      {
   #if __cpp_exceptions >= 199711L
      ASSERT_OR_THROW(m_sel < virtual_register_base || pin != pin_fully,
      #endif
   }
      bool
   VirtualValue::ready(int block, int index) const
   {
      (void)block;
   (void)index;
      }
      bool
   VirtualValue::is_virtual() const
   {
         }
      class ValueComparer : public ConstRegisterVisitor {
   public:
      ValueComparer();
   ValueComparer(const Register *value);
   ValueComparer(const LocalArray *value);
   ValueComparer(const LocalArrayValue *value);
   ValueComparer(const UniformValue *value);
   ValueComparer(const LiteralConstant *value);
            void visit(const Register& other) override;
   void visit(const LocalArray& other) override;
   void visit(const LocalArrayValue& other) override;
   void visit(const UniformValue& value) override;
   void visit(const LiteralConstant& other) override;
                  private:
      const Register *m_register;
   const LocalArray *m_array;
   const LocalArrayValue *m_array_value;
   const UniformValue *m_uniform_value;
   const LiteralConstant *m_literal_value;
      };
      class ValueCompareCreater : public ConstRegisterVisitor {
   public:
      void visit(const Register& value) { compare = ValueComparer(&value); }
   void visit(const LocalArray& value) { compare = ValueComparer(&value); }
   void visit(const LocalArrayValue& value) { compare = ValueComparer(&value); }
   void visit(const UniformValue& value) { compare = ValueComparer(&value); }
   void visit(const LiteralConstant& value) { compare = ValueComparer(&value); }
               };
      VirtualValue::Pointer
   VirtualValue::from_string(const std::string& s)
   {
      switch (s[0]) {
   case 'S':
   case 'R':
         case 'L':
         case 'K':
         case 'P':
         case 'I':
            default:
      std::cerr << "'" << s << "'";
         }
      bool
   VirtualValue::equal_to(const VirtualValue& other) const
   {
               if (result) {
      ValueCompareCreater comp_creater;
   accept(comp_creater);
   other.accept(comp_creater.compare);
                  }
      VirtualValue::Pointer
   VirtualValue::get_addr() const
   {
      class GetAddressRegister : public ConstRegisterVisitor {
   public:
      void visit(const VirtualValue& value) { (void)value; }
   void visit(const Register& value) { (void)value; };
   void visit(const LocalArray& value) { (void)value; }
   void visit(const LocalArrayValue& value) { m_result = value.addr(); }
   void visit(const UniformValue& value) { (void)value; }
   void visit(const LiteralConstant& value) { (void)value; }
            GetAddressRegister():
         {
               };
   GetAddressRegister get_addr;
   accept(get_addr);
      }
      Register::Register(int sel, int chan, Pin pin):
         {
   }
      void
   Register::add_parent(Instr *instr)
   {
      m_parents.insert(instr);
      }
      void
   Register::add_parent_to_array(Instr *instr)
   {
         }
      void
   Register::del_parent(Instr *instr)
   {
      m_parents.erase(instr);
      }
      void
   Register::del_parent_from_array(Instr *instr)
   {
         }
      void
   Register::add_use(Instr *instr)
   {
         }
      void
   Register::del_use(Instr *instr)
   {
      sfn_log << SfnLog::opt << "Del use of " << *this << " in " << *instr << "\n";
   if (m_uses.find(instr) != m_uses.end()) {
            }
      bool
   Register::ready(int block, int index) const
   {
      for (auto p : m_parents) {
      if (p->block_id() <= block) {
      if (p->index() < index && !p->is_scheduled()) {
               }
      }
      void
   Register::accept(RegisterVisitor& visitor)
   {
         }
      void
   Register::accept(ConstRegisterVisitor& visitor) const
   {
         }
      void
   Register::print(std::ostream& os) const
   {
      if (m_flags.test(addr_or_idx)) {
      switch (sel()) {
   case AddressRegister::addr: os << "AR"; break;
   case AddressRegister::idx0: os << "IDX0"; break;
   case AddressRegister::idx1: os << "IDX1"; break;
   default:
         }
                        if (pin() != pin_none)
         if (m_flags.any()) {
      os << "{";
   if (m_flags.test(ssa))
         if (m_flags.test(pin_start))
         if (m_flags.test(pin_end))
               }
      Register::Pointer
   Register::from_string(const std::string& s)
   {
      std::string numstr;
   char chan = 0;
            if (s == "AR") {
         } else if (s == "IDX0") {
         } else if (s == "IDX1") {
                           int type = 0;
   for (unsigned i = 1; i < s.length(); ++i) {
      if (s[i] == '.') {
      type = 1;
      } else if (s[i] == '@') {
      type = 2;
               switch (type) {
   case 0:
      numstr.append(1, s[i]);
      case 1:
      chan = s[i];
      case 2:
      pinstr.append(1, s[i]);
      default:
                     int sel;
   if (s[0] != '_') {
      std::istringstream n(numstr);
      } else {
                  auto p = pin_none;
   if (pinstr == "chan")
         else if (pinstr == "array")
         else if (pinstr == "fully")
         else if (pinstr == "group")
         else if (pinstr == "chgr")
         else if (pinstr == "free")
            switch (chan) {
   case 'x':
      chan = 0;
      case 'y':
      chan = 1;
      case 'z':
      chan = 2;
      case 'w':
      chan = 3;
      case '0':
      chan = 4;
      case '1':
      chan = 5;
      case '_':
      chan = 7;
               auto reg = new Register(sel, chan, p);
   if (s[0] == 'S')
         if (p == pin_fully || p == pin_array)
            }
      RegisterVec4::RegisterVec4():
      m_sel(-1),
   m_swz({7, 7, 7, 7}),
      {
   }
      RegisterVec4::RegisterVec4(int sel, bool is_ssa, const Swizzle& swz, Pin pin):
      m_sel(sel),
      {
      for (int i = 0; i < 4; ++i) {
      m_values[i] = new Element(*this, new Register(m_sel, swz[i], pin));
   if (is_ssa)
         }
      RegisterVec4::RegisterVec4(const RegisterVec4& orig):
      m_sel(orig.m_sel),
      {
      for (int i = 0; i < 4; ++i)
      }
      RegisterVec4::RegisterVec4(PRegister x, PRegister y, PRegister z, PRegister w, Pin pin)
   {
               if (x) {
         } else if (y) {
         } else if (z) {
         } else if (w) {
         } else
            if (!(x && y && z && w))
            m_values[0] = new Element(*this, x ? x : dummy);
   m_values[1] = new Element(*this, y ? y : dummy);
   m_values[2] = new Element(*this, z ? z : dummy);
            for (int i = 0; i < 4; ++i) {
      if (m_values[0]->value()->pin() == pin_fully) {
      pin = pin_fully;
                  for (int i = 0; i < 4; ++i) {
      switch (m_values[i]->value()->pin()) {
   case pin_none:
   case pin_free:
      m_values[i]->value()->set_pin(pin);
      case pin_chan:
      if (pin == pin_group)
            default:;
            m_swz[i] = m_values[i]->value()->chan();
         }
      void
   RegisterVec4::add_use(Instr *instr)
   {
      for (auto& r : m_values) {
      if (r->value()->chan() < 4)
         }
      void
   RegisterVec4::del_use(Instr *instr)
   {
      for (auto& r : m_values) {
            }
      bool
   RegisterVec4::has_uses() const
   {
      for (auto& r : m_values) {
      if (r->value()->has_uses())
      }
      }
      int
   RegisterVec4::sel() const
   {
      int comp = 0;
   while (comp < 4 && m_values[comp]->value()->chan() > 3)
            }
      bool
   RegisterVec4::ready(int block_id, int index) const
   {
      for (int i = 0; i < 4; ++i) {
      if (m_values[i]->value()->chan() < 4) {
      if (!m_values[i]->value()->ready(block_id, index))
         }
      }
      void
   RegisterVec4::print(std::ostream& os) const
   {
      os << (m_values[0]->value()->has_flag(Register::ssa) ? 'S' : 'R') << sel() << ".";
   for (int i = 0; i < 4; ++i)
      }
      bool
   operator==(const RegisterVec4& lhs, const RegisterVec4& rhs)
   {
      for (int i = 0; i < 4; ++i) {
      assert(lhs[i]);
   assert(rhs[i]);
   if (!lhs[i]->equal_to(*rhs[i])) {
            }
      }
      RegisterVec4::Element::Element(const RegisterVec4& parent, int chan):
      m_parent(parent),
      {
   }
      RegisterVec4::Element::Element(const RegisterVec4& parent, PRegister value):
      m_parent(parent),
      {
   }
      LiteralConstant::LiteralConstant(uint32_t value):
      VirtualValue(ALU_SRC_LITERAL, -1, pin_none),
      {
   }
      void
   LiteralConstant::accept(RegisterVisitor& vistor)
   {
         }
      void
   LiteralConstant::accept(ConstRegisterVisitor& vistor) const
   {
         }
      void
   LiteralConstant::print(std::ostream& os) const
   {
         }
      LiteralConstant::Pointer
   LiteralConstant::from_string(const std::string& s)
   {
      if (s[1] != '[')
            std::string numstr;
   for (unsigned i = 2; i < s.length(); ++i) {
      if (s[i] == ']')
            if (isxdigit(s[i]))
         if (s[i] == 'x')
                        uint32_t num;
   n >> std::hex >> num;
      }
      // Inline constants usually don't care about the channel but
   // ALU_SRC_PV should be pinned, but we only emit these constants
   // very late, and based on the real register they replace
   InlineConstant::InlineConstant(int sel, int chan):
         {
   }
      void
   InlineConstant::accept(RegisterVisitor& vistor)
   {
         }
      void
   InlineConstant::accept(ConstRegisterVisitor& vistor) const
   {
         }
      void
   InlineConstant::print(std::ostream& os) const
   {
      auto ivalue = alu_src_const.find(static_cast<AluInlineConstants>(sel()));
   if (ivalue != alu_src_const.end()) {
      os << "I[" << ivalue->second.descr << "]";
   if (ivalue->second.use_chan)
      } else if (sel() >= ALU_SRC_PARAM_BASE && sel() < ALU_SRC_PARAM_BASE + 32) {
         } else {
            }
      std::map<std::string, std::pair<AluInlineConstants, bool>> InlineConstant::s_opmap;
      InlineConstant::Pointer
   InlineConstant::from_string(const std::string& s)
   {
      std::string namestr;
                     unsigned i = 2;
   while (i < s.length()) {
      if (s[i] == ']')
         namestr.append(1, s[i]);
                        auto entry = s_opmap.find(namestr);
   AluInlineConstants value = ALU_SRC_UNKNOWN;
            if (entry == s_opmap.end()) {
      for (auto& [opcode, descr] : alu_src_const) {
      if (namestr == descr.descr) {
      value = opcode;
                           } else {
      value = entry->second.first;
                        if (use_chan) {
      ASSERT_OR_THROW(s[i + 1] == '.', "inline const channel not started with '.'");
   switch (s[i + 2]) {
   case 'x':
      chan = 0;
      case 'y':
      chan = 1;
      case 'z':
      chan = 2;
      case 'w':
      chan = 3;
      case '0':
      chan = 4;
      case '1':
      chan = 5;
      case '_':
      chan = 7;
      default:
            }
      }
      InlineConstant::Pointer
   InlineConstant::param_from_string(const std::string& s)
   {
               int param = 0;
   int i = 5;
   while (isdigit(s[i])) {
      param *= 10;
   param += s[i] - '0';
               int chan = 7;
   assert(s[i] == '.');
   switch (s[i + 1]) {
   case 'x':
      chan = 0;
      case 'y':
      chan = 1;
      case 'z':
      chan = 2;
      case 'w':
      chan = 3;
      default:
                     }
      UniformValue::UniformValue(int sel, int chan, int kcache_bank):
      VirtualValue(sel, chan, pin_none),
   m_kcache_bank(kcache_bank),
      {
   }
      UniformValue::UniformValue(int sel, int chan, PVirtualValue buf_addr, int kcache_bank):
      VirtualValue(sel, chan, pin_none),
   m_kcache_bank(kcache_bank),
      {
   }
      void
   UniformValue::accept(RegisterVisitor& vistor)
   {
         }
      void
   UniformValue::accept(ConstRegisterVisitor& vistor) const
   {
         }
      PVirtualValue
   UniformValue::buf_addr() const
   {
         }
      void UniformValue::set_buf_addr(PVirtualValue addr)
   {
         }
      void
   UniformValue::print(std::ostream& os) const
   {
      os << "KC" << m_kcache_bank;
   if (m_buf_addr) {
         }
      }
      bool
   UniformValue::equal_buf_and_cache(const UniformValue& other) const
   {
      bool result = m_kcache_bank == other.m_kcache_bank;
   if (result) {
      if (m_buf_addr && other.m_buf_addr) {
         } else {
            }
      }
      UniformValue::Pointer
   UniformValue::from_string(const std::string& s, ValueFactory *factory)
   {
      assert(s[1] == 'C');
            VirtualValue *bufid = nullptr;
   int bank;
   char c;
   is >> bank;
                                       is >> c;
   while (c != ']' && is.good()) {
      index0_ss << c;
               auto index0_str = index0_ss.str();
   if (isdigit(index0_str[0])) {
      std::istringstream is_digit(index0_str);
      } else {
      bufid = factory ?
               assert(c == ']');
   is >> c;
   assert(c == '[');
   is >> index;
               assert(c == ']');
   is >> c;
            is >> c;
   int chan = 0;
   switch (c) {
   case 'x':
      chan = 0;
      case 'y':
      chan = 1;
      case 'z':
      chan = 2;
      case 'w':
      chan = 3;
      default:
         }
   if (bufid)
         else
      }
      LocalArray::LocalArray(int base_sel, int nchannels, int size, int frac):
      Register(base_sel, nchannels, pin_array),
   m_base_sel(base_sel),
   m_nchannels(nchannels),
   m_size(size),
   m_values(size * nchannels),
      {
      assert(nchannels <= 4);
            sfn_log << SfnLog::reg << "Allocate array A" << base_sel << "(" << size << ", " << frac
            auto pin = m_size > 1 ? pin_array : (nchannels > 1 ? pin_none : pin_free);
   for (int c = 0; c < nchannels; ++c) {
      for (unsigned i = 0; i < m_size; ++i) {
      PRegister reg = new Register(base_sel + i, c + frac, pin);
            }
      void
   LocalArray::accept(RegisterVisitor& vistor)
   {
         }
      void
   LocalArray::accept(ConstRegisterVisitor& vistor) const
   {
         }
      void
   LocalArray::print(std::ostream& os) const
   {
      os << "A" << m_base_sel << "[0 "
         for (unsigned i = 0; i < m_nchannels; ++i) {
            }
      size_t
   LocalArray::size() const
   {
         }
      uint32_t
   LocalArray::nchannels() const
   {
         }
      PRegister
   LocalArray::element(size_t offset, PVirtualValue indirect, uint32_t chan)
   {
      ASSERT_OR_THROW(offset < m_size, "Array: index out of range");
            sfn_log << SfnLog::reg << "Request element A" << m_base_sel << "[" << offset;
   if (indirect)
                  if (indirect) {
      class ResolveDirectArrayElement : public ConstRegisterVisitor {
   public:
      void visit(const Register& value) { (void)value; };
   void visit(const LocalArray& value)
   {
      (void)value;
      }
   void visit(const LocalArrayValue& value) { (void)value; }
   void visit(const UniformValue& value) { (void)value; }
   void visit(const LiteralConstant& value)
   {
      offset = value.value();
                     ResolveDirectArrayElement():
      offset(0),
                     int offset;
               // If the address os a literal constant then update the offset
   // and don't access the value indirectly
   indirect->accept(addr);
   if (addr.is_contant) {
      offset += addr.offset;
   indirect = nullptr;
                  LocalArrayValue *reg = m_values[m_size * chan + offset];
   if (indirect) {
      reg = new LocalArrayValue(reg, indirect, *this);
               sfn_log << SfnLog::reg << "  got " << *reg << "\n";
      }
      void LocalArray::add_parent_to_elements(int chan, Instr *instr)
   {
      for (auto& e : m_values)
      if (e->chan() == chan)
   }
      bool
   LocalArray::ready_for_direct(int block, int index, int chan) const
   {
      if (!Register::ready(block, index))
            /* For direct access to an array value we also have to take indirect
   * writes on the same channels into account */
   for (LocalArrayValue *e : m_values_indirect) {
      if (e->chan() == chan && !e->Register::ready(block, index)) {
                        }
      bool
   LocalArray::ready_for_indirect(int block, int index, int chan) const
   {
      int offset = (chan - m_frac) * m_size;
   for (unsigned i = 0; i < m_size; ++i) {
      if (!m_values[offset + i]->Register::ready(block, index))
                  }
      LocalArrayValue::LocalArrayValue(PRegister reg, PVirtualValue index, LocalArray& array):
      Register(reg->sel(), reg->chan(), pin_array),
   m_addr(index),
      {
   }
      const Register&
   LocalArray::operator()(size_t idx, size_t chan) const
   {
         }
      LocalArrayValue::LocalArrayValue(PRegister reg, LocalArray& array):
         {
   }
      PVirtualValue
   LocalArrayValue::addr() const
   {
         }
      void LocalArrayValue::set_addr(PRegister addr)
   {
         }
         const LocalArray&
   LocalArrayValue::array() const
   {
         }
      void
   LocalArrayValue::forward_del_use(Instr *instr)
   {
      if (m_addr && m_addr->as_register())
      }
      void
   LocalArrayValue::forward_add_use(Instr *instr)
   {
      if (m_addr && m_addr->as_register())
      }
      void
   LocalArrayValue::accept(RegisterVisitor& vistor)
   {
         }
      void
   LocalArrayValue::accept(ConstRegisterVisitor& vistor) const
   {
         }
      void
   LocalArrayValue::add_parent_to_array(Instr *instr)
   {
      if (m_addr)
      }
      void
   LocalArrayValue::del_parent_from_array(Instr *instr)
   {
         }
      void
   LocalArrayValue::print(std::ostream& os) const
   {
      int offset = sel() - m_array.sel();
   os << "A" << m_array.sel() << "[";
   if (offset > 0 && m_addr)
         else if (m_addr)
         else
            }
      bool
   LocalArrayValue::ready(int block, int index) const
   {
      return m_addr ? (m_array.ready_for_indirect(block, index, chan()) &&
            }
      ValueComparer::ValueComparer():
      m_result(false),
   m_register(nullptr),
   m_array(nullptr),
   m_array_value(nullptr),
   m_uniform_value(nullptr),
   m_literal_value(nullptr),
      {
   }
      ValueComparer::ValueComparer(const Register *value):
      m_result(false),
   m_register(value),
   m_array(nullptr),
   m_array_value(nullptr),
   m_uniform_value(nullptr),
   m_literal_value(nullptr),
      {
   }
      ValueComparer::ValueComparer(const LocalArray *value):
      m_result(false),
   m_register(nullptr),
   m_array(value),
   m_array_value(nullptr),
   m_uniform_value(nullptr),
   m_literal_value(nullptr),
      {
   }
      ValueComparer::ValueComparer(const LocalArrayValue *value):
      m_result(false),
   m_register(nullptr),
   m_array(nullptr),
   m_array_value(value),
   m_uniform_value(nullptr),
   m_literal_value(nullptr),
      {
   }
      ValueComparer::ValueComparer(const UniformValue *value):
      m_result(false),
   m_register(nullptr),
   m_array(nullptr),
   m_array_value(nullptr),
   m_uniform_value(value),
   m_literal_value(nullptr),
      {
   }
      ValueComparer::ValueComparer(const LiteralConstant *value):
      m_result(false),
   m_register(nullptr),
   m_array(nullptr),
   m_array_value(nullptr),
   m_uniform_value(nullptr),
   m_literal_value(value),
      {
   }
      ValueComparer::ValueComparer(const InlineConstant *value):
      m_result(false),
   m_register(nullptr),
   m_array(nullptr),
   m_array_value(nullptr),
   m_uniform_value(nullptr),
   m_literal_value(nullptr),
      {
   }
      void
   ValueComparer::visit(const Register& other)
   {
      (void)other;
   if (m_register) {
         } else
      };
      void
   ValueComparer::visit(const LocalArray& other)
   {
      m_result = false;
   if (m_array) {
      m_result =
         };
      void
   ValueComparer::visit(const LocalArrayValue& other)
   {
      m_result = false;
   if (m_array_value) {
      m_result = m_array_value->array().equal_to(other.array());
   if (m_result) {
      auto my_addr = m_array_value->addr();
   auto other_addr = other.addr();
   if (my_addr && other_addr) {
         } else {
                  };
      void
   ValueComparer::visit(const UniformValue& value)
   {
      m_result = false;
   if (m_uniform_value) {
      m_result = m_uniform_value->kcache_bank() == value.kcache_bank();
   if (m_result) {
      auto my_buf_addr = m_uniform_value->buf_addr();
   auto other_buf_addr = value.buf_addr();
   if (my_buf_addr && other_buf_addr) {
         } else {
                  };
      void
   ValueComparer::visit(const LiteralConstant& other)
   {
         };
      void
   ValueComparer::visit(const InlineConstant& other)
   {
      (void)other;
      };
      class CheckConstValue : public ConstRegisterVisitor {
   public:
      CheckConstValue(uint32_t _test_value):
         {
   }
   CheckConstValue(float _test_value):
         {
            void visit(const Register& value) override { (void)value; }
   void visit(const LocalArray& value) override { (void)value; }
   void visit(const LocalArrayValue& value) override { (void)value; }
            void visit(const LiteralConstant& value) override
   {
         }
   void visit(const InlineConstant& value) override
   {
      switch (test_value) {
   case 0:
      result = value.sel() == ALU_SRC_0;
      case 1:
      result = value.sel() == ALU_SRC_1_INT;
      case 0x3f800000 /* 1.0f */:
      result = value.sel() == ALU_SRC_1;
      case 0x3f000000 /* 0.5f */:
      result = value.sel() == ALU_SRC_0_5;
                  uint32_t test_value;
      };
      bool
   value_is_const_uint(const VirtualValue& val, uint32_t value)
   {
      CheckConstValue test(value);
   val.accept(test);
      }
      bool
   value_is_const_float(const VirtualValue& val, float value)
   {
      CheckConstValue test(value);
   val.accept(test);
      }
      } // namespace r600
