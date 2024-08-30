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
   from bifrost_isa import *
   from mako.template import Template
      # Consider pseudo instructions when getting the modifier list
   instructions_with_pseudo = parse_instructions(sys.argv[1], include_pseudo = True)
   ir_instructions_with_pseudo = partition_mnemonics(instructions_with_pseudo)
   modifier_lists = order_modifiers(ir_instructions_with_pseudo)
      # ...but strip for packing
   instructions = parse_instructions(sys.argv[1])
   ir_instructions = partition_mnemonics(instructions)
      # Packs sources into an argument. Offset argument to work around a quirk of our
   # compiler IR when dealing with staging registers (TODO: reorder in the IR to
   # fix this)
   def pack_sources(sources, body, pack_exprs, offset, is_fma):
      for i, src in enumerate(sources):
      # FMA first two args are restricted, but that's checked once for all
   # FMA so the compiler has less work to do
            # Validate the source
   if src[1] != expected:
                  # Sources are state-invariant
   for state in pack_exprs:
      # Try to map from a modifier list `domain` to the list `target`
   def map_modifier(body, prefix, mod, domain, target):
      # We don't want to map reserveds, that's invalid IR anyway
   def reserved_to_none(arr):
            # Trim out reserveds at the end
   noned_domain = reserved_to_none(domain)
   noned_target = reserved_to_none(target)
   none_indices = [i for i, x in enumerate(noned_target) if x != None]
            if trimmed == noned_domain[0:len(trimmed)]:
      # Identity map, possibly on the left subset
      else:
      # Generate a table as a fallback
   table = ", ".join([str(target.index(x)) if x in target else "~0" for x in domain])
            if len(domain) > 2:
                     def pick_from_bucket(opts, bucket):
      intersection = set(opts) & bucket
   assert(len(intersection) <= 1)
         def pack_modifier(mod, width, default, opts, body, pack_exprs):
      # Destructure the modifier name
                     ir_value = "bytes2" if mod == "bytes2" else "{}[{}]".format(raw, arg) if mod[-1] in "0123" else mod
            # Swizzles need to be packed "specially"
   SWIZZLE_BUCKETS = [
            set(['h00', 'h0']),
   set(['h01', 'none', 'b0123', 'w0']), # Identity
   set(['h10']),
   set(['h11', 'h1']),
   set(['b0000', 'b00', 'b0']),
   set(['b1111', 'b11', 'b1']),
   set(['b2222', 'b22', 'b2']),
   set(['b3333', 'b33', 'b3']),
   set(['b0011', 'b01']),
   set(['b2233', 'b23']),
   set(['b1032']),
               if raw in SWIZZLES:
      # Construct a list
   lists = [pick_from_bucket(opts, bucket) for bucket in SWIZZLE_BUCKETS]
      elif raw == "lane_dest":
      lists = [pick_from_bucket(opts, bucket) for bucket in SWIZZLE_BUCKETS]
      elif raw in ["abs", "sign"]:
         elif raw in ["neg", "not"]:
                     # We need to map from ir_opts to opts
   mapped = map_modifier(body, mod, ir_value, lists, opts)
   body.append('unsigned {} = {};'.format(mod, mapped))
         # Compiles an S-expression (and/or/eq/neq, modifiers, `ordering`, immediates)
   # into a C boolean expression suitable to stick in an if-statement. Takes an
   # imm_map to map modifiers to immediate values, parametrized by the ctx that
   # we're looking up in (the first, non-immediate argument of the equality)
      SEXPR_BINARY = {
         "and": "&&",
   "or": "||",
   "eq": "==",
   }
      def compile_s_expr(expr, imm_map, ctx):
      if expr[0] == 'alias':
         elif expr == ['eq', 'ordering', '#gt']:
         elif expr == ['neq', 'ordering', '#lt']:
         elif expr == ['neq', 'ordering', '#gt']:
         elif expr == ['eq', 'ordering', '#lt']:
         elif expr == ['eq', 'ordering', '#eq']:
         elif isinstance(expr, list):
      sep = " {} ".format(SEXPR_BINARY[expr[0]])
      elif expr[0] == '#':
         else:
         # Packs a derived value. We just iterate through the possible choices and test
   # whether the encoding matches, and if so we use it.
      def pack_derived(pos, exprs, imm_map, body, pack_exprs):
               first = True
   for i, expr in enumerate(exprs):
      if expr is not None:
         cond = compile_s_expr(expr, imm_map, None)
         assert (not first)
   body.append('else unreachable("No pattern match at pos {}");'.format(pos))
            assert(pos is not None)
         # Generates a routine to pack a single variant of a single- instruction.
   # Template applies the needed formatting and combine to OR together all the
   # pack_exprs to avoid bit fields.
   #
   # Argument swapping is sensitive to the order of operations. Dependencies:
   # sources (RW), modifiers (RW), derived values (W). Hence we emit sources and
   # modifiers first, then perform a swap if necessary overwriting
   # sources/modifiers, and last calculate derived values and pack.
      variant_template = Template("""static inline unsigned
   bi_pack_${name}(${", ".join(["bi_instr *I"] + ["enum bifrost_packed_src src{}".format(i) for i in range(srcs)])})
   {
   ${"\\n".join([("    " + x) for x in common_body])}
   % if single_state:
   % for (pack_exprs, s_body, _) in states:
   ${"\\n".join(["    " + x for x in s_body + ["return {};".format( " | ".join(pack_exprs))]])}
   % endfor
   % else:
   % for i, (pack_exprs, s_body, cond) in enumerate(states):
         ${"\\n".join(["        " + x for x in s_body + ["return {};".format(" | ".join(pack_exprs))]])}
   % endfor
      } else {
            % endif
   }
   """)
      def pack_variant(opname, states):
      # Expressions to be ORed together for the final pack, an array per state
            # Computations which need to be done to encode first, across states
            # Map from modifier names to a map from modifier values to encoded values
   # String -> { String -> Uint }. This can be shared across states since
   # modifiers are (except the pos values) constant across state.
            # Pack sources. Offset over to deal with staging/immediate weirdness in our
   # IR (TODO: reorder sources upstream so this goes away). Note sources are
   # constant across states.
   staging = states[0][1].get("staging", "")
   offset = 0
   if staging in ["r", "rw"]:
                     modifiers_handled = []
   for st in states:
      for ((mod, _, width), default, opts) in st[1].get("modifiers", []):
                                       for i, st in enumerate(states):
      for ((mod, pos, width), default, opts) in st[1].get("modifiers", []):
               for ((src_a, src_b), cond, remap) in st[1].get("swaps", []):
      # Figure out which vars to swap, in order to swap the arguments. This
   # always includes the sources themselves, and may include source
   # modifiers (with the same source indices). We swap based on which
   # matches A, this is arbitrary but if we swapped both nothing would end
            vars_to_swap = ['src']
   for ((mod, _, width), default, opts) in st[1].get("modifiers", []):
                           # Emit the swaps. We use a temp, and wrap in a block to avoid naming
            for v in vars_to_swap:
            # Also, remap. Bidrectional swaps are explicit in the XML.
   for v in remap:
                                 common_body.append('}')
         for (name, pos, width) in st[1].get("immediates", []):
      common_body.append('unsigned {} = I->{};'.format(name, name))
            for st in pack_exprs:
         # After this, we have to branch off, since deriveds *do* vary based on state.
            for i, (_, st) in enumerate(states):
      for ((pos, width), exprs) in st.get("derived", []):
         # How do we pick a state? Accumulate the conditions
            if state_conds == None:
            # Finally, we'll collect everything together
         print(COPYRIGHT + '#include "compiler.h"')
      packs = [pack_variant(e, instructions[e]) for e in instructions]
   for p in packs:
            top_pack = Template("""unsigned
   bi_pack_${'fma' if unit == '*' else 'add'}(bi_instr *I,
      enum bifrost_packed_src src0,
   enum bifrost_packed_src src1,
   enum bifrost_packed_src src2,
      {
      if (!I)
         % if unit == '*':
      assert((1 << src0) & 0xfb);
         % endif
         % for opcode in ops:
   % if unit + opcode in instructions:
      case BI_OPCODE_${opcode.replace('.', '_').upper()}:
      % endif
   % endfor
         #ifndef NDEBUG
         #endif
               }
   """)
      for unit in ['*', '+']:
      print(top_pack.render(ops = ir_instructions, instructions = instructions, opname_to_c = opname_to_c, unit = unit))
