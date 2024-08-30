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
      #include "sfn_valuefactory.h"
      #include "gallium/drivers/r600/r600_shader.h"
   #include "sfn_debug.h"
   #include "sfn_instr.h"
      #include <algorithm>
   #include <iostream>
   #include <queue>
   #include <sstream>
      namespace r600 {
      using std::istringstream;
   using std::string;
      ValueFactory::ValueFactory():
      m_next_register_index(VirtualValue::virtual_register_base),
      {
   }
      void
   ValueFactory::set_virtual_register_base(int base)
   {
         }
      bool
   ValueFactory::allocate_registers(const std::list<nir_intrinsic_instr *>& regs)
   {
      struct array_entry {
      unsigned index;
   unsigned length;
            bool operator()(const array_entry& a, const array_entry& b) const
   {
      return a.ncomponents < b.ncomponents ||
                  using array_list =
            std::list<unsigned> non_array;
   array_list arrays;
   for(auto intr : regs) {
      unsigned num_elms = nir_intrinsic_num_array_elems(intr);
   int num_comp = nir_intrinsic_num_components(intr);
            if (num_elms > 0 || num_comp > 1 || bit_size > 32) {
      array_entry ae = {
      intr->def.index,
   num_elms ? num_elms : 1,
         } else {
                     int free_components = 4;
   int sel = m_next_register_index;
            while (!arrays.empty()) {
      auto a = arrays.top();
            /* This is a bit hackish, return an id that encodes the array merge. To
   * make sure that the mapping doesn't go wrong we have to make sure the
   * arrays is longer than the number of instances in this arrays slot */
   if (a.ncomponents > free_components || a.length > length) {
      sel = m_next_register_index;
   free_components = 4;
                                 for (int i = 0; i < a.ncomponents; ++i) {
      RegisterKey key(a.index, i, vp_array);
   m_channel_counts.inc_count(frac + i, a.length);
   m_registers[key] = array;
   sfn_log << SfnLog::reg << __func__ << ": Allocate array " << key << ":" << *array
               free_components -= a.ncomponents;
                        for (auto index : non_array) {
      RegisterKey key(index, 0, vp_register);
   auto chan = m_channel_counts.least_used(0xf);
   m_registers[key] = new Register(m_next_register_index++,
                        }
      int ValueFactory::new_register_index()
   {
         }
      PRegister
   ValueFactory::allocate_pinned_register(int sel, int chan)
   {
      if (m_next_register_index <= sel)
            auto reg = new Register(sel, chan, pin_fully);
   reg->set_flag(Register::pin_start);
   reg->set_flag(Register::ssa);
   m_pinned_registers.push_back(reg);
      }
      RegisterVec4
   ValueFactory::allocate_pinned_vec4(int sel, bool is_ssa)
   {
      if (m_next_register_index <= sel)
            RegisterVec4 retval(sel, is_ssa, {0, 1, 2, 3}, pin_fully);
   for (int i = 0; i < 4; ++i) {
      retval[i]->set_flag(Register::pin_start);
   retval[i]->set_flag(Register::ssa);
      }
      }
      void
   ValueFactory::inject_value(const nir_def& def, int chan, PVirtualValue value)
   {
      RegisterKey key(def.index, chan, vp_ssa);
   sfn_log << SfnLog::reg << "Inject value with key " << key << "\n";
   assert(m_values.find(key) == m_values.end());
      }
      class TranslateRegister : public RegisterVisitor {
   public:
      void visit(VirtualValue& value) { (void)value; }
   void visit(Register& value) { (void)value; };
   void visit(LocalArray& value) { m_value = value.element(m_offset, m_addr, m_chan); }
   void visit(LocalArrayValue& value) { (void)value; }
   void visit(UniformValue& value) { (void)value; }
   void visit(LiteralConstant& value) { (void)value; }
            TranslateRegister(int offset, PVirtualValue addr, int chan):
      m_addr(addr),
   m_value(nullptr),
   m_offset(offset),
      {
            PVirtualValue m_addr;
   PRegister m_value;
   int m_offset;
      };
      void
   ValueFactory::allocate_const(nir_load_const_instr *load_const)
   {
      assert(load_const->def.bit_size == 32);
   for (int i = 0; i < load_const->def.num_components; ++i) {
      RegisterKey key(load_const->def.index, i, vp_ssa);
   m_values[key] = literal(load_const->value[i].i32);
   sfn_log << SfnLog::reg << "Add const with key " << key << " as " << m_values[key]
         }
      PVirtualValue
   ValueFactory::uniform(nir_intrinsic_instr *load_uniform, int chan)
   {
      auto literal = nir_src_as_const_value(load_uniform->src[0]);
                        }
      PVirtualValue
   ValueFactory::uniform(uint32_t index, int chan, int kcache)
   {
         }
      PRegister
   ValueFactory::temp_register(int pinned_channel, bool is_ssa)
   {
      int sel = m_next_register_index++;
            auto reg = new Register(sel, chan, pinned_channel >= 0 ? pin_chan : pin_free);
            if (is_ssa)
            m_registers[RegisterKey(sel, chan, vp_temp)] = reg;
      }
      RegisterVec4
   ValueFactory::temp_vec4(Pin pin, const RegisterVec4::Swizzle& swizzle)
   {
               if (pin == pin_free)
                     for (int i = 0; i < 4; ++i) {
      vec4[i] = new Register(sel, swizzle[i], pin);
   vec4[i]->set_flag(Register::ssa);
      }
      }
      RegisterVec4
   ValueFactory::dest_vec4(const nir_def& def, Pin pin)
   {
      if (pin != pin_group && pin != pin_chgr)
         PRegister x = dest(def, 0, pin);
   PRegister y = dest(def, 1, pin);
   PRegister z = dest(def, 2, pin);
   PRegister w = dest(def, 3, pin);
      }
      PRegister ValueFactory::addr()
   {
      if (!m_ar)
            }
      PRegister ValueFactory::idx_reg(unsigned idx)
   {
         if (idx == 0) {
      if (!m_idx0)
            } else {
      assert(idx == 1);
   if (!m_idx1)
               }
         PVirtualValue
   ValueFactory::src(const nir_alu_src& alu_src, int chan)
   {
         }
      PVirtualValue
   ValueFactory::src64(const nir_alu_src& alu_src, int chan, int comp)
   {
         }
      PVirtualValue
   ValueFactory::src(const nir_src& src, int chan)
   {
               sfn_log << SfnLog::reg << "search ssa " << src.ssa->index << " c:" << chan
         auto val = ssa_src(*src.ssa, chan);
   sfn_log << *val << "\n";
      }
      PVirtualValue
   ValueFactory::src(const nir_tex_src& tex_src, int chan)
   {
         }
      PRegister
   ValueFactory::dummy_dest(unsigned chan)
   {
      assert(chan < 4);
      }
      PRegister
   ValueFactory::dest(const nir_def& ssa, int chan, Pin pin_channel, uint8_t chan_mask)
   {
               /* dirty workaround for Cayman trans ops, because we may request
   * the same sa reg more than once, but only write to it once.  */
   auto ireg = m_registers.find(key);
   if (ireg != m_registers.end())
            auto isel = m_ssa_index_to_sel.find(ssa.index);
   int sel;
   if (isel != m_ssa_index_to_sel.end())
         else {
      sel = m_next_register_index++;
   sfn_log << SfnLog::reg << "Assign " << sel << " to index " << ssa.index << " in "
                     if (pin_channel == pin_free)
            auto vreg = new Register(sel, chan, pin_channel);
   m_channel_counts.inc_count(chan);
   vreg->set_flag(Register::ssa);
   m_registers[key] = vreg;
   sfn_log << SfnLog::reg << "allocate Ssa " << key << ":" << *vreg << "\n";
      }
      PVirtualValue
   ValueFactory::zero()
   {
         }
      PVirtualValue
   ValueFactory::one()
   {
         }
      PVirtualValue
   ValueFactory::one_i()
   {
         }
      PRegister
   ValueFactory::undef(int index, int chan)
   {
      RegisterKey key(index, chan, vp_ssa);
   PRegister reg = new Register(m_next_register_index++, 0, pin_free);
   reg->set_flag(Register::ssa);
   m_registers[key] = reg;
      }
      PVirtualValue
   ValueFactory::ssa_src(const nir_def& ssa, int chan)
   {
      RegisterKey key(ssa.index, chan, vp_ssa);
            auto ireg = m_registers.find(key);
   if (ireg != m_registers.end())
            auto ival = m_values.find(key);
   if (ival != m_values.end())
            RegisterKey rkey(ssa.index, chan, vp_register);
            ireg = m_registers.find(rkey);
   if (ireg != m_registers.end())
            RegisterKey array_key(ssa.index, chan, vp_array);
   sfn_log << SfnLog::reg << "search array with key" << array_key << "\n";
   auto iarray = m_registers.find(array_key);
   if (iarray != m_registers.end())
            std::cerr << "Didn't find source with key " << key << "\n";
      }
      PVirtualValue
   ValueFactory::literal(uint32_t value)
   {
      auto iv = m_literal_values.find(value);
   if (iv != m_literal_values.end())
            auto v = new LiteralConstant(value);
   m_literal_values[value] = v;
      }
      PInlineConstant
   ValueFactory::inline_const(AluInlineConstants sel, int chan)
   {
      int hash = (sel << 3) | chan;
   auto iv = m_inline_constants.find(hash);
   if (iv != m_inline_constants.end())
         auto v = new InlineConstant(sel, chan);
   m_inline_constants[hash] = v;
      }
      std::vector<PVirtualValue, Allocator<PVirtualValue>>
   ValueFactory::src_vec(const nir_src& source, int components)
   {
      std::vector<PVirtualValue, Allocator<PVirtualValue>> retval;
   retval.reserve(components);
   for (int i = 0; i < components; ++i)
            }
      std::vector<PRegister, Allocator<PRegister>>
   ValueFactory::dest_vec(const nir_def& def, int num_components)
   {
      std::vector<PRegister, Allocator<PRegister>> retval;
   retval.reserve(num_components);
   for (int i = 0; i < num_components; ++i)
            }
      RegisterVec4
   ValueFactory::src_vec4(const nir_src& source, Pin pin, const RegisterVec4::Swizzle& swz)
   {
      auto sx = swz[0] < 4 ? src(source, swz[0])->as_register() : nullptr;
   auto sy = swz[1] < 4 ? src(source, swz[1])->as_register() : nullptr;
   auto sz = swz[2] < 4 ? src(source, swz[2])->as_register() : nullptr;
                     int sel = sx ? sx->sel() : (sy ? sy->sel() : (sz ? sz->sel() : sw ? sw->sel() : -1));
   if (sel < 0)
            if (!sx)
         if (!sy)
         if (!sz)
         if (!sw)
               }
      static Pin
   pin_from_string(const std::string& pinstr)
   {
      if (pinstr == "chan")
         if (pinstr == "array")
         if (pinstr == "fully")
         if (pinstr == "group")
         if (pinstr == "chgr")
         if (pinstr == "free")
            }
      static int
   chan_from_char(char chan)
   {
      switch (chan) {
   case 'x':
         case 'y':
         case 'z':
         case 'w':
         case '0':
         case '1':
         case '_':
         }
      }
      static int
   str_to_int(const string& s)
   {
      istringstream ss(s);
   int retval;
   ss >> retval;
      }
      static bool
   split_register_string(const string& s,
                           {
      int type = 0;
   for (unsigned i = 1; i < s.length(); ++i) {
      if (s[i] == '.' && type != 3) {
      type = 1;
      } else if (s[i] == '@' && type != 3) {
      type = 2;
      } else if (s[i] == '[') {
      type = 3;
      } else if (s[i] == ']') {
      if (type != 3)
                  type = 4;
               switch (type) {
   case 0:
      index_str.append(1, s[i]);
      case 1:
      swizzle_str.append(1, s[i]);
      case 2:
      pin_str.append(1, s[i]);
      case 3:
      size_str.append(1, s[i]);
      default:
            }
      }
      PRegister
   ValueFactory::dest_from_string(const std::string& s)
   {
      if (s == "AR") {
      if (!m_ar)
            } else if (s == "IDX0") {
      if (!m_idx0)
            } else if (s == "IDX1") {
      if (!m_idx1)
                     string index_str;
   string size_str;
   string swizzle_str;
                                       int sel = 0;
   if (s[0] == '_') {
      /* Since these instructions still may use or switch to a different
   * channel we have to create a new instance for each occurrence */
      } else {
      std::istringstream n(index_str);
               auto p = pin_from_string(pin_str);
            EValuePool pool = vp_temp;
   switch (s[0]) {
   case 'A':
      pool = vp_array;
      case 'R':
      pool = vp_register;
      case '_':
      pool = vp_ignore;
      case 'S':
      pool = vp_ssa;
      default:
                                             auto ireg = m_registers.find(key);
   if (ireg == m_registers.end()) {
      auto reg = new Register(sel, chan, p);
   if (s[0] == 'S')
         if (p == pin_fully)
         m_registers[key] = reg;
      } else if (pool == vp_ignore) {
      assert(ireg->second->sel() == std::numeric_limits<int>::max());
      } else {
               if (size_str.length()) {
      auto array = static_cast<LocalArray *>(ireg->second);
   PVirtualValue addr = nullptr;
   int offset = 0;
   if (size_str[0] == 'S' || size_str[0] == 'R' ||
      size_str == "AR" || size_str.substr(0,3) == "IDX") {
      } else {
      istringstream num_str(size_str);
                  } else
         }
      PVirtualValue
   ValueFactory::src_from_string(const std::string& s)
   {
      if (s == "AR") {
      assert(m_ar);
      } else if (s == "IDX0") {
      assert(m_idx0);
      } else if (s == "IDX1") {
      assert(m_idx1);
               switch (s[0]) {
   case 'A':
   case 'S':
   case 'R':
         case 'L':
         case 'K':
         case 'P':
         case 'I':
            default:
      std::cerr << "'" << s << "'";
                        string index_str;
   string size_str;
   string swizzle_str;
                     int sel = 0;
   if (s[0] == '_') {
         } else {
      std::istringstream n(index_str);
               auto p = pin_from_string(pin_str);
            EValuePool pool = vp_temp;
   switch (s[0]) {
   case 'A':
      pool = vp_array;
      case 'R':
      pool = vp_register;
      case '_':
      pool = vp_ignore;
      case 'S':
      pool = vp_ssa;
      default:
                           auto ireg = m_registers.find(key);
   if (ireg != m_registers.end()) {
      if (pool != vp_ssa && size_str.length()) {
      auto array = static_cast<LocalArray *>(ireg->second);
   PVirtualValue addr = nullptr;
   int offset = 0;
   if (size_str[0] == 'S' || size_str[0] == 'R' ||
      size_str == "AR" || size_str.substr(0,3) == "IDX") {
      } else {
      istringstream num_str(size_str);
      }
      } else {
            } else {
      if (sel != std::numeric_limits<int>::max()) {
      std::cerr << "register " << key << "not found \n";
      } else {
      auto reg = new Register(sel, chan, p);
   m_registers[key] = reg;
            }
      RegisterVec4
   ValueFactory::dest_vec4_from_string(const std::string& s,
               {
      bool is_ssa = false;
                     for (int i = 0; i < 4; ++i) {
      auto pool = is_ssa ? vp_ssa : vp_register;
   if (swz[i] > 3)
            RegisterKey key(sel, i, pool);
   auto ireg = m_registers.find(key);
   if (ireg != m_registers.end()) {
      v[i] = ireg->second;
      } else {
      v[i] = new Register(sel, i, pin);
   if (is_ssa)
               }
      }
      RegisterVec4
   ValueFactory::src_vec4_from_string(const std::string& s)
   {
      RegisterVec4::Swizzle swz;
   bool is_ssa = false;
                     PRegister used_reg = nullptr;
   for (int i = 0; i < 4; ++i) {
      if (swz[i] < 4) {
      RegisterKey key(sel, swz[i], is_ssa ? vp_ssa : vp_register);
   auto ireg = m_registers.find(key);
   if (ireg == m_registers.end()) {
      std::cerr << s << ": Register with key " << key << " not found\n";
      }
      } else {
            }
   sel = used_reg ? used_reg->sel() : 0;
            for (int i = 0; i < 4; ++i) {
      if (!v[i]) {
      v[i] = new Register(sel, swz[i], pin);
   if (is_ssa)
      } else {
      if (v[i]->pin() == pin_none)
         }
      }
      LocalArray *
   ValueFactory::array_from_string(const std::string& s)
   {
      assert(s[0] == 'A');
   string index_str;
   string size_str;
   string swizzle_str;
            int type = 0;
   for (unsigned i = 1; i < s.length(); ++i) {
      if (s[i] == '.') {
      type = 1;
      } else if (s[i] == '@') {
      type = 2;
      } else if (s[i] == '[') {
      type = 3;
      } else if (s[i] == ']') {
      assert(type == 3);
   type = 4;
               switch (type) {
   case 0:
      index_str.append(1, s[i]);
      case 1:
      swizzle_str.append(1, s[i]);
      case 2:
      pin_str.append(1, s[i]);
      case 3:
      size_str.append(1, s[i]);
      default:
                     int sel = str_to_int(index_str);
   int size = str_to_int(size_str);
            if (ncomp > 4 || ncomp <= 0) {
      std::cerr << "Error reading array from '" << s << ": ";
   std::cerr << "index:'" << index_str << "' -> '" << sel << "' size:'" << size_str
                           const char *swz = "xyzw";
   const char *first_swz = strchr(swz, swizzle_str[0]);
   long frac = first_swz - swz;
                     for (int i = 0; i < ncomp; ++i) {
      RegisterKey key(sel, i + frac, vp_array);
      }
      }
      void
   LiveRangeMap::append_register(Register *reg)
   {
               auto chan = reg->chan();
            LiveRangeEntry entry(reg);
      }
      std::array<size_t, 4>
   LiveRangeMap::sizes() const
   {
      std::array<size_t, 4> result;
   std::transform(m_life_ranges.begin(),
                        }
      LiveRangeMap
   ValueFactory::prepare_live_range_map()
   {
               for (auto [key, reg] : m_registers) {
      if (key.value.pool == vp_ignore)
            if (key.value.pool == vp_array) {
      auto array = static_cast<LocalArray *>(reg);
   for (auto& a : *array) {
            } else {
      if (reg->chan() < 4)
                  for (auto r : m_pinned_registers) {
                  for (int i = 0; i < 4; ++i) {
      auto& comp = result.component(i);
   std::sort(comp.begin(),
            comp.end(),
   [](const LiveRangeEntry& lhs, const LiveRangeEntry& rhs) {
      for (size_t j = 0; j < comp.size(); ++j)
                  }
      void
   ValueFactory::clear_pins()
   {
      for (auto [key, reg] : m_registers)
            for (auto reg : m_pinned_registers)
      }
      void
   ValueFactory::clear()
   {
      m_registers.clear();
   m_values.clear();
   m_literal_values.clear();
   m_inline_constants.clear();
      }
      void
   ValueFactory::get_shader_info(r600_shader *sh_info)
   {
               for (auto& [key, reg] : m_registers) {
      if (key.value.pool == vp_array)
                           sh_info->num_arrays = arrays.size();
   sh_info->arrays =
            for (auto& arr : arrays) {
      sh_info->arrays->gpr_start = arr->sel();
   sh_info->arrays->gpr_count = arr->size();
      }
         }
      } // namespace r600
