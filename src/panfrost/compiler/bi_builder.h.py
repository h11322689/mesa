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
      SKIP = set(["lane", "lane_dest", "lanes", "lanes", "replicate", "swz", "widen",
      "swap", "neg", "abs", "not", "sign", "extend", "divzero", "clamp", "sem",
         TEMPLATE = """
   #ifndef _BI_BUILDER_H_
   #define _BI_BUILDER_H_
      #include "compiler.h"
      <%
   # For <32-bit loads/stores, the default extend `none` with a natural sized
   # input is not encodeable! To avoid a footgun, swap the default to `zext` which
   # will work as expected
   ZEXT_DEFAULT = set(["LOAD.i8", "LOAD.i16", "LOAD.i24", "STORE.i8", "STORE.i16", "STORE.i24"])
      def nirtypes(opcode):
      split = opcode.split('.', 1)
   if len(split) < 2:
            if len(split) <= 1:
                     type = split[1]
   if type[0] == 'v':
            if type[0] == 'f':
         elif type[0] == 's':
         elif type[0] == 'u':
         elif type[0] == 'i':
         else:
         def condition(opcode, typecheck, sizecheck):
      cond = ''
   if typecheck == True:
      cond += '('
   types = nirtypes(opcode)
   assert types != None
   for T in types:
               if sizecheck == True:
            cmpf_mods = ops[opcode]["modifiers"]["cmpf"] if "cmpf" in ops[opcode]["modifiers"] else None
   if "cmpf" in ops[opcode]["modifiers"]:
      cond += "{}(".format(' && ' if cond != '' else '')
   for cmpf in ops[opcode]["modifiers"]["cmpf"]:
         if cmpf != 'reserved':
               def to_suffix(op):
            %>
      % for opcode in ops:
   static inline
   bi_instr * bi_${opcode.replace('.', '_').lower()}${to_suffix(ops[opcode])}(${signature(ops[opcode], modifiers)})
   {
   <%
      op = ops[opcode]
   nr_dests = "nr_dests" if op["variable_dests"] else op["dests"]
      %>
      size_t size = sizeof(bi_instr) + sizeof(bi_index) * (${nr_dests} + ${nr_srcs});
            I->op = BI_OPCODE_${opcode.replace('.', '_').upper()};
   I->nr_dests = ${nr_dests};
   I->nr_srcs = ${nr_srcs};
   I->dest = (bi_index *) (&I[1]);
         % if not op["variable_dests"]:
   % for dest in range(op["dests"]):
         % endfor
   %endif
      % if not op["variable_srcs"]:
   % for src in range(src_count(op)):
         % endfor
   % endif
      % for mod in ops[opcode]["modifiers"]:
   % if not should_skip(mod, opcode):
         % endif
   % endfor
   % if ops[opcode]["rtz"]:
         % endif
   % for imm in ops[opcode]["immediates"]:
         % endfor
   % if opcode in ZEXT_DEFAULT:
         % endif
      bi_builder_insert(&b->cursor, I);
      }
      % if ops[opcode]["dests"] == 1 and not ops[opcode]["variable_dests"]:
   static inline
   bi_index bi_${opcode.replace('.', '_').lower()}(${signature(ops[opcode], modifiers, no_dests=True)})
   {
         }
      %endif
   <%
      common_op = opcode.split('.')[0]
   variants = [a for a in ops.keys() if a.split('.')[0] == common_op]
   signatures = [signature(ops[op], modifiers, no_dests=True) for op in variants]
   homogenous = all([sig == signatures[0] for sig in signatures])
   types = [nirtypes(x) for x in variants]
   typeful = False
   for t in types:
      if t != types[0]:
         sizes = [typesize(x) for x in variants]
   sized = False
   for size in sizes:
      if size != sizes[0]:
            %>
   % if homogenous and len(variants) > 1 and last:
   % for (suffix, temp, dests, ret) in (('_to', False, 1, 'instr *'), ('', True, 0, 'index')):
   % if not temp or ops[opcode]["dests"] > 0:
   static inline
   bi_${ret} bi_${common_op.replace('.', '_').lower()}${suffix if ops[opcode]['dests'] > 0 else ''}(${signature(ops[opcode], modifiers, typeful=typeful, sized=sized, no_dests=not dests)})
   {
   % for i, variant in enumerate(variants):
      ${"{}if ({})".format("else " if i > 0 else "", condition(variant, typeful, sized))}
      % endfor
      else
      }
      %endif
   %endfor
   %endif
   %endfor
   #endif"""
      import sys
   from bifrost_isa import *
   from mako.template import Template
      instructions = parse_instructions(sys.argv[1], include_pseudo = True)
   ir_instructions = partition_mnemonics(instructions)
   modifier_lists = order_modifiers(ir_instructions)
      # Generate type signature for a builder routine
      def should_skip(mod, op):
      # FROUND and HADD only make sense in context of a round mode, so override
   # the usual skip
   if mod == "round" and ("FROUND" in op or "HADD" in op):
                  def modifier_signature(op):
            def signature(op, modifiers, typeful = False, sized = False, no_dests = False):
      return ", ".join(
      ["bi_builder *b"] +
   (["nir_alu_type type"] if typeful == True else []) +
   (["unsigned bitsize"] if sized == True else []) +
   (["unsigned nr_dests"] if op["variable_dests"] else
         (["unsigned nr_srcs"] if op["variable_srcs"] else
         ["{} {}".format(
   "bool" if len(modifiers[T[0:-1]] if T[-1] in "0123" else modifiers[T]) == 2 else
   "enum bi_" + T[0:-1] if T[-1] in "0123" else
   "enum bi_" + T,
   T) for T in modifier_signature(op)] +
      def arguments(op, temp_dest = True):
      dest_pattern = "bi_temp(b->shader)" if temp_dest else 'dest{}'
   dests = [dest_pattern.format(i) for i in range(op["dests"])]
            # Variable source/destinations just pass in the count
   if op["variable_dests"]:
            if op["variable_srcs"]:
                  print(Template(COPYRIGHT + TEMPLATE).render(ops = ir_instructions, modifiers =
      modifier_lists, signature = signature, arguments = arguments, src_count =
   src_count, typesize = typesize, should_skip = should_skip))
