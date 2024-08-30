   #
   # Copyright (C) 2020 Collabora, Ltd.
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
      TEMPLATE = """#include <stdio.h>
   #include "compiler.h"
      static const char *
   bi_swizzle_as_str(enum bi_swizzle swz)
   {
         switch (swz) {
   case BI_SWIZZLE_H00: return ".h00";
   case BI_SWIZZLE_H01: return "";
   case BI_SWIZZLE_H10: return ".h10";
   case BI_SWIZZLE_H11: return ".h11";
   case BI_SWIZZLE_B0000: return ".b0";
   case BI_SWIZZLE_B1111: return ".b1";
   case BI_SWIZZLE_B2222: return ".b2";
   case BI_SWIZZLE_B3333: return ".b3";
   case BI_SWIZZLE_B0011: return ".b0011";
   case BI_SWIZZLE_B2233: return ".b2233";
   case BI_SWIZZLE_B1032: return ".b1032";
   case BI_SWIZZLE_B3210: return ".b3210";
   case BI_SWIZZLE_B0022: return ".b0022";
            }
      static const char *
   bir_fau_name(unsigned fau_idx)
   {
      const char *names[] = {
            "zero", "lane-id", "wrap-id", "core-id", "fb-extent",
   "atest-param", "sample-pos", "reserved",
   "blend_descriptor_0", "blend_descriptor_1",
   "blend_descriptor_2", "blend_descriptor_3",
   "blend_descriptor_4", "blend_descriptor_5",
               assert(fau_idx < ARRAY_SIZE(names));
      }
      static const char *
   bir_passthrough_name(unsigned idx)
   {
      const char *names[] = {
                  assert(idx < ARRAY_SIZE(names));
      }
      static void
   bi_print_index(FILE *fp, bi_index index)
   {
      if (index.discard)
            if (bi_is_null(index))
         else if (index.type == BI_INDEX_CONSTANT)
         else if (index.type == BI_INDEX_FAU && index.value >= BIR_FAU_UNIFORM)
         else if (index.type == BI_INDEX_FAU)
         else if (index.type == BI_INDEX_PASS)
         else if (index.type == BI_INDEX_REGISTER)
         else if (index.type == BI_INDEX_NORMAL)
         else
            if (index.offset)
            if (index.abs)
            if (index.neg)
               }
      % for mod in sorted(modifiers):
   % if len(modifiers[mod]) > 2: # otherwise just boolean
      UNUSED static inline const char *
   bi_${mod}_as_str(enum bi_${mod} ${mod})
   {
         % for i, state in enumerate(sorted(modifiers[mod])):
   % if state == "none":
         % elif state != "reserved":
         % endif
   % endfor
                  };
   % endif
   % endfor
      <%def name="print_modifiers(mods, table)">
      % for mod in mods:
   % if mod not in ["lane_dest"]:
   % if len(table[mod]) > 2:
         % else:
         % endif
   % endif
      </%def>
      <%def name="print_source_modifiers(mods, src, table)">
      % for mod in mods:
   % if mod[0:-1] not in ["lane", "lanes", "replicate", "swz", "widen", "swap", "abs", "neg", "sign", "not"]:
   % if len(table[mod[0:-1]]) > 2:
         % elif mod == "bytes2":
         % else:
         % endif
   %endif
      </%def>
      void
   bi_print_instr(const bi_instr *I, FILE *fp)
   {
               bi_foreach_dest(I, d) {
                           if (I->nr_dests > 0)
                     if (I->table)
            if (I->flow)
            if (I->op == BI_OPCODE_COLLECT_I32 || I->op == BI_OPCODE_PHI) {
      for (unsigned i = 0; i < I->nr_srcs; ++i) {
         if (i > 0)
                                       % for opcode in ops:
   <%
      # Extract modifiers that are not per-source
      %>
      case BI_OPCODE_${opcode.replace('.', '_').upper()}:
      ${print_modifiers(root_modifiers, modifiers)}
      % for src in range(src_count(ops[opcode])):
   % if src > 0:
         % endif
      bi_print_index(fp, I->src[${src}]);
      % endfor
   % for imm in ops[opcode]["immediates"]:
         % endfor
      % endfor
      default:
                  if (I->branch_target)
                  }"""
      import sys
   from bifrost_isa import *
   from mako.template import Template
      instructions = parse_instructions(sys.argv[1], include_pseudo = True)
   ir_instructions = partition_mnemonics(instructions)
   modifier_lists = order_modifiers(ir_instructions)
      print(Template(COPYRIGHT + TEMPLATE).render(ops = ir_instructions, modifiers = modifier_lists, src_count = src_count))
