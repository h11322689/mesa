   #encoding=utf-8
      # Copyright (C) 2021 Collabora, Ltd.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      from valhall import instructions, immediates, enums, typesize, safe_name
   from mako.template import Template
   from mako import exceptions
      template = """
   #include "disassemble.h"
      % for name, en in ENUMS.items():
   UNUSED static const char *valhall_${name}[] = {
   % for v in en.values:
         % endfor
   };
      % endfor
   static const uint32_t va_immediates[32] = {
   % for imm in IMMEDIATES:
         % endfor
   };
      static inline void
   va_print_src(FILE *fp, uint8_t src, unsigned fau_page)
   {
   unsigned type = (src >> 6);
   unsigned value = (src & 0x3F);
      if (type == VA_SRC_IMM_TYPE) {
         if (value >= 32) {
         if (fau_page == 0)
         else if (fau_page == 1)
         else if (fau_page == 3)
                        } else {
         } else if (type == VA_SRC_UNIFORM_TYPE) {
   fprintf(fp, "u%u", value | (fau_page << 6));
   } else {
   bool discard = (type & 1);
   fprintf(fp, "%sr%u", discard ? "^" : "", value);
   }
   }
      static inline void
   va_print_float_src(FILE *fp, uint8_t src, unsigned fau_page, bool neg, bool abs)
   {
   unsigned type = (src >> 6);
   unsigned value = (src & 0x3F);
      if (type == VA_SRC_IMM_TYPE) {
         assert(value < 32 && "overflow in LUT");
   } else {
                  if (neg)
   fprintf(fp, ".neg");
      if (abs)
   fprintf(fp, ".abs");
   }
      void
   va_disasm_instr(FILE *fp, uint64_t instr)
   {
      unsigned primary_opc = (instr >> 48) & MASK(9);
   unsigned fau_page = (instr >> 57) & MASK(2);
               % for bucket in OPCODES:
      <%
      ops = OPCODES[bucket]
         % if len(ops) > 0:
         % if ambiguous:
   secondary_opc = (instr >> ${ops[0].secondary_shift}) & ${hex(ops[0].secondary_mask)};
   % endif
   % for op in ops:
   <% no_comma = True %>
   % if ambiguous:
            % endif
         % for mod in op.modifiers:
   % if mod.name not in ["left", "memory_width", "descriptor_type", "staging_register_count", "staging_register_write_count"]:
   % if mod.is_enum:
         % else:
         % endif
   % endif
   % endfor
               % if len(op.dests) > 0:
   <% no_comma = False %>
         % endif
   % for index, sr in enumerate(op.staging):
   % if not no_comma:
         % endif
   <%
               if sr.count != 0:
         elif "staging_register_write_count" in [x.name for x in op.modifiers] and sr.write:
         elif "staging_register_count" in [x.name for x in op.modifiers]:
         else:
      %>
   //            assert(((instr >> ${sr.start}) & 0xC0) == ${sr.encoded_flags});
               fprintf(fp, "@");
   for (unsigned i = 0; i < ${sr_count}; ++i) {
         % endfor
   % for i, src in enumerate(op.srcs):
   % if not no_comma:
         % endif
   <% no_comma = False %>
   % if src.absneg:
               va_print_float_src(fp, instr >> ${src.start}, fau_page,
   % elif src.is_float:
         % else:
         % endif
   % if src.swizzle:
   % if src.size == 32:
         % else:
         % endif
   % endif
   % if src.lanes:
         % elif src.halfswizzle:
         % elif src.widen:
         % elif src.combine:
         % endif
   % if src.lane:
         % endif
   % if 'not' in src.offset:
         % endif
   % endfor
   % for imm in op.immediates:
   <%
      prefix = "#" if imm.name == "constant" else imm.name + ":"
      %>
               % endfor
   % if ambiguous:
         % endif
   % endfor
            % endif
   % endfor
         }
   """
      # Bucket by opcode for hierarchical disassembly
   OPCODE_BUCKETS = {}
   for ins in instructions:
      opc = ins.opcode
         # Check that each bucket may be disambiguated
   for op in OPCODE_BUCKETS:
               # Nothing to disambiguate
   if len(bucket) < 2:
            SECONDARY = {}
   for ins in bucket:
      # Number of sources determines opcode2 placement, must be consistent
            # Must not repeat, else we're ambiguous
   assert(ins.opcode2 not in SECONDARY)
      try:
         except:
      print(exceptions.text_error_template().render())
