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
      import sys
   import itertools
   from bifrost_isa import parse_instructions, opname_to_c, expand_states
   from mako.template import Template
      instructions = parse_instructions(sys.argv[1], include_unused = True)
      # Constructs a reserved mask for a derived to cull impossible encodings
      def reserved_mask(derived):
      ((pos, width), opts) = derived
   reserved = [x is None for x in opts]
   mask = sum([(y << x) for x, y in enumerate(reserved)])
         def reserved_masks(op):
      masks = [reserved_mask(m) for m in op[2].get("derived", [])]
         # To decode instructions, pattern match based on the rules:
   #
   # 1. Execution unit (FMA or ADD) must line up.
   # 2. All exact bits must match.
   # 3. No fields should be reserved in a legal encoding.
   # 4. Tiebreaker: Longer exact masks (greater unsigned bitwise inverses) win.
   #
   # To implement, filter the execution unit and check for exact bits in
   # descending order of exact mask length.  Check for reserved fields per
   # candidate and succeed if it matches.
   # found.
      def decode_op(instructions, is_fma):
      # Filter out the desired execution unit
            # Sort by exact masks, descending
   MAX_MASK = (1 << (23 if is_fma else 20)) - 1
            # Map to what we need to template
            # Generate checks in order
      bi_disasm_${unit}(FILE *fp, unsigned bits, struct bifrost_regs *srcs, struct bifrost_regs *next_regs, unsigned staging_register, unsigned branch_offset, struct bi_constants *consts, bool last)
   {
            % for (i, (name, (emask, ebits), derived)) in enumerate(options):
   % if len(derived) > 0:
         % for (pos, width, reserved) in derived:
         % endfor
         % else:
         % endif
         % endfor
      else
               }"""
               # Decoding emits a series of function calls to e.g. `fma_fadd_v2f16`. We need to
   # emit functions to disassemble a single decoded instruction in a particular
   # state. Sync prototypes to avoid moves when calling.
      disasm_op_template = Template("""static void
   bi_disasm_${c_name}(FILE *fp, unsigned bits, struct bifrost_regs *srcs, struct bifrost_regs *next_regs, unsigned staging_register, unsigned branch_offset, struct bi_constants *consts, bool last)
   {
         }
   """)
      lut_template_only = Template("""    static const char *${field}[] = {
               """)
      # Given a lookup table written logically, generate an accessor
   lut_template = Template("""    static const char *${field}_table[] = {
                        """)
      # Helpers for decoding follow. pretty_mods applies dot syntax
      def pretty_mods(opts, default):
            # Recursively searches for the set of free variables required by an expression
      def find_context_keys_expr(expr):
      if isinstance(expr, list):
         elif expr[0] == '#':
         else:
         def find_context_keys(desc, test):
               if len(test) > 0:
            for i, (_, vals) in enumerate(desc.get('derived', [])):
      for j, val in enumerate(vals):
                     # Compiles a logic expression to Python expression, ctx -> { T, F }
      EVALUATORS = {
         'and': ' and ',
   'or': ' or ',
   'eq': ' == ',
   }
      def compile_derived_inner(expr, keys):
      if expr == []:
         elif expr is None or expr[0] == 'alias':
         elif isinstance(expr, list):
      args = [compile_derived_inner(arg, keys) for arg in expr[1:]]
      elif expr[0] == '#':
         elif expr == 'ordering':
         else:
         def compile_derived(expr, keys):
            # Generate all possible combinations of values and evaluate the derived values
   # by bruteforce evaluation to generate a forward mapping (values -> deriveds)
      def evaluate_forward_derived(vals, ctx, ordering):
      for j, expr in enumerate(vals):
      if expr(ctx, ordering):
               def evaluate_forward(keys, derivf, testf, ctx, ordering):
      if not testf(ctx, ordering):
                     for vals in derivf:
               if evaled is None:
                        def evaluate_forwards(keys, derivf, testf, mod_vals, ordered):
      orderings = ["lt", "gt"] if ordered else [None]
         # Invert the forward mapping (values -> deriveds) of finite sets to produce a
   # backwards mapping (deriveds -> values), suitable for disassembly. This is
   # possible since the encoding is unambiguous, so this mapping is a bijection
   # (after reserved/impossible encodings)
      def invert_lut(value_size, forward, derived, mod_map, keys, mod_vals):
      backwards = [None] * (1 << value_size)
   for (i, deriveds), ctx in zip(enumerate(forward), itertools.product(*mod_vals)):
      # Skip reserved
   if deriveds == None:
            shift = 0
   param = 0
   for j, ((x, width), y) in enumerate(derived):
                  assert(param not in backwards)
               # Compute the value of all indirectly specified modifiers by using the
   # backwards mapping (deriveds -> values) as a run-time lookup table.
      def build_lut(mnemonic, desc, test):
      # Construct the system
                     for ((name, pos, width), default, values) in desc.get('modifiers', []):
                     # Find the keys and impose an order
   key_set = find_context_keys(desc, test)
   ordered = 'ordering' in key_set
   key_set.discard('ordering')
            # Evaluate the deriveds for every possible state, forming a (state -> deriveds) map
   testf = compile_derived(test, keys)
   derivf = [[compile_derived(expr, keys) for expr in v] for (_, v) in derived]
   mod_vals = [mod_map[k][1] for k in keys]
            # Now invert that map to get a (deriveds -> state) map
   value_size = sum([width for ((x, width), y) in derived])
            # From that map, we can generate LUTs
            if ordered:
            for j, key in enumerate(keys):
      # Only generate tables for indirect specifiers
   if mod_map[key][2] is not None:
            idx_parts = []
            for ((pos, width), _) in derived:
                                    if ordered:
         for i, order in enumerate(backwards):
                  else:
         options = [ctx[j] if ctx is not None and ctx[j] is not None else "reserved" for ctx in backwards[0]]
               def disasm_mod(mod, skip_mods):
      if mod[0][0] in skip_mods:
         else:
         def disasm_op(name, op):
      (mnemonic, test, desc) = op
            # Modifiers may be either direct (pos is not None) or indirect (pos is
            body = ""
                     for ((mod, pos, width), default, opts) in desc.get('modifiers', []):
      if pos is not None:
         # Mnemonic, followed by modifiers
                     for mod in desc.get('modifiers', []):
      # Skip per-source until next block
   if mod[0][0][-1] in "0123" and int(mod[0][0][-1]) < len(srcs):
                  body += '    fputs(" ", fp);\n'
                     for i, (pos, mask) in enumerate(srcs):
      body += '    fputs(", ", fp);\n'
            # Error check if needed
   if (mask != 0xFF):
            # Print modifiers suffixed with this src number (e.g. abs0 for src0)
   for mod in desc.get('modifiers', []):
               # And each immediate
   for (imm, pos, width) in desc.get('immediates', []):
            # Attach a staging register if one is used
   if desc.get('staging'):
                  print('#include "util/macros.h"')
   print('#include "bifrost/disassemble.h"')
      states = expand_states(instructions)
   print('#define _BITS(bits, pos, width) (((bits) >> (pos)) & ((1 << (width)) - 1))')
      for st in states:
            print(decode_op(states, True))
   print(decode_op(states, False))
