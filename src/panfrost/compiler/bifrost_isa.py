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
      # Useful for autogeneration
   COPYRIGHT = """/*
   * Copyright (C) 2020 Collabora, Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      /* Autogenerated file, do not edit */
      """
      # Parse instruction set XML into a normalized form for processing
      import xml.etree.ElementTree as ET
   import copy
   import itertools
   from collections import OrderedDict
      def parse_cond(cond, aliased = False):
      if cond.tag == 'reserved':
            if cond.attrib.get('alias', False) and not aliased:
            if 'left' in cond.attrib:
         else:
         def parse_exact(obj):
            def parse_derived(obj):
               for deriv in obj.findall('derived'):
      loc = [int(deriv.attrib['start']), int(deriv.attrib['size'])]
            opts = [parse_cond(d) for d in deriv.findall('*')]
   default = [None] * count
                        def parse_modifiers(obj, include_pseudo):
               for mod in obj.findall('mod'):
      if mod.attrib.get('pseudo', False) and not include_pseudo:
            name = mod.attrib['name']
   start = mod.attrib.get('start', None)
            if start is not None:
                     if len(opts) == 0:
                  # Find suitable default
            # Pad out as reserved
   count = (1 << size)
   opts = (opts + (['reserved'] * count))[0:count]
               def parse_copy(enc, existing):
      for node in enc.findall('copy'):
      name = node.get('name')
   for ex in existing:
            def parse_instruction(ins, include_pseudo):
      common = {
            'srcs': [],
   'modifiers': [],
   'immediates': [],
   'swaps': [],
   'derived': [],
   'staging': ins.attrib.get('staging', '').split('=')[0],
   'staging_count': ins.attrib.get('staging', '=0').split('=')[1],
   'dests': int(ins.attrib.get('dests', '1')),
   'variable_dests': ins.attrib.get('variable_dests', False),
   'variable_srcs': ins.attrib.get('variable_srcs', False),
   'unused': ins.attrib.get('unused', False),
   'pseudo': ins.attrib.get('pseudo', False),
   'message': ins.attrib.get('message', 'none'),
               if 'exact' in ins.attrib:
            for src in ins.findall('src'):
      if src.attrib.get('pseudo', False) and not include_pseudo:
            mask = int(src.attrib['mask'], 0) if ('mask' in src.attrib) else 0xFF
         for imm in ins.findall('immediate'):
      if imm.attrib.get('pseudo', False) and not include_pseudo:
            start = int(imm.attrib['start']) if 'start' in imm.attrib else None
         common['derived'] = parse_derived(ins)
            for swap in ins.findall('swap'):
      lr = [int(swap.get('left')), int(swap.get('right'))]
   cond = parse_cond(swap.findall('*')[0])
            for rw in swap.findall('rewrite'):
                                          encodings = ins.findall('encoding')
            if len(encodings) == 0:
         else:
      for enc in encodings:
                        variant['exact'] = parse_exact(enc)
                              def parse_instructions(xml, include_unused = False, include_pseudo = False):
      final = {}
            for ins in instructions:
               # Some instructions are for useful disassembly only and can be stripped
   # out of the compiler, particularly useful for release builds
   if parsed[0][1]["unused"] and not include_unused:
            # On the other hand, some instructions are only for the IR, not disassembly
   if parsed[0][1]["pseudo"] and not include_pseudo:
                        # Expand out an opcode name to something C-escaped
      def opname_to_c(name):
            # Expand out distinct states to distrinct instructions, with a placeholder
   # condition for instructions with a single state
      def expand_states(instructions):
               for ins in instructions:
               for ((test, desc), i) in zip(c, range(len(c))):
                              # Drop keys used for packing to simplify IR representation, so we can check for
   # equivalence easier
      def simplify_to_ir(ins):
      return {
            'staging': ins['staging'],
   'srcs': len(ins['srcs']),
   'dests': ins['dests'],
   'variable_dests': ins['variable_dests'],
   'variable_srcs': ins['variable_srcs'],
   'modifiers': [[m[0][0], m[2]] for m in ins['modifiers']],
      # Converstions to integers default to rounding-to-zero
   # All other opcodes default to rounding to nearest even
   def default_round_to_zero(name):
      # 8-bit int to float is exact
   subs = ['_TO_U', '_TO_S', '_TO_V2U', '_TO_V2S', '_TO_V4U', '_TO_V4S']
         def combine_ir_variants(instructions, key):
      seen = [op for op in instructions.keys() if op[1:] == key]
   variant_objs = [[simplify_to_ir(Q[1]) for Q in instructions[x]] for x in seen]
            # Accumulate modifiers across variants
            for s in variants[0:]:
      # Check consistency
   assert(s['srcs'] == variants[0]['srcs'])
   assert(s['dests'] == variants[0]['dests'])
   assert(s['immediates'] == variants[0]['immediates'])
            for name, opts in s['modifiers']:
         if name not in modifiers:
               # Great, we've checked srcs/immediates are consistent and we've summed over
   # modifiers
   return {
            'key': key,
   'srcs': variants[0]['srcs'],
   'dests': variants[0]['dests'],
   'variable_dests': variants[0]['variable_dests'],
   'variable_srcs': variants[0]['variable_srcs'],
   'staging': variants[0]['staging'],
   'immediates': sorted(variants[0]['immediates']),
   'modifiers': modifiers,
   'v': len(variants),
   'ir': variants,
      # Partition instructions to mnemonics, considering units and variants
   # equivalent.
      def partition_mnemonics(instructions):
      key_func = lambda x: x[1:]
   sorted_instrs = sorted(instructions.keys(), key = key_func)
   partitions = itertools.groupby(sorted_instrs, key_func)
         # Generate modifier lists, by accumulating all the possible modifiers, and
   # deduplicating thus assigning canonical enum values. We don't try _too_ hard
   # to be clever, but by preserving as much of the original orderings as
   # possible, later instruction encoding is simplified a bit.  Probably a micro
   # optimization but we have to pick _some_ ordering, might as well choose the
   # most convenient.
   #
   # THIS MUST BE DETERMINISTIC
      def order_modifiers(ir_instructions):
               # modifier name -> (list of option strings)
            for ins in sorted(ir_instructions):
               for name in modifiers:
                  if name_ not in modifier_lists:
               for mod in modifier_lists:
               # Ensure none is false for booleans so the builder makes sense
   if len(lst) == 2 and lst[1] == "none":
         elif mod == "table":
         # We really need a zero sentinel to materialize DTSEL
   assert(lst[2] == "none")
                        # Count sources for a simplified (IR) instruction, including a source for a
   # staging register if necessary
   def src_count(op):
      staging = 1 if (op["staging"] in ["r", "rw"]) else 0
         # Parses out the size part of an opocde name
   def typesize(opcode):
      if opcode[-3:] == '128':
         if opcode[-2:] == '48':
         elif opcode[-1] == '8':
         else:
      try:
         except:
         return 32
