   #!/usr/bin/env python3
   #
   # Copyright © 2020 Google, Inc.
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
      from mako.template import Template
   from isa import ISA
   import argparse
   import os
   import sys
      class FieldDecode(object):
      def __init__(self, name, map_expr):
      self.name = name
         def get_c_name(self):
         # State and helpers used by the template:
   class State(object):
      def __init__(self, isa):
            def case_name(self, bitset, name):
            # Return a list of all <map> entries for a leaf bitset, with the child
   # bitset overriding the parent bitset's entries. Because we can't resolve
   # which <map>s are used until we resolve which overload is used, we
   # generate code for encoding all of these and then at runtime select which
   # one to call based on the display.
   def decode_fields(self, bitset):
      if bitset.get_root().decode is None:
            seen_fields = set()
   if bitset.encode is not None:
         for name, expr in bitset.encode.maps.items():
            if bitset.extends is not None:
         for field in self.decode_fields(self.isa.bitsets[bitset.extends]):
         # A limited resolver for field type which doesn't properly account for
   # overrides.  In particular, if a field is defined differently in multiple
   # different cases, this just blindly picks the last one.
   #
   # TODO to do this properly, I don't think there is an alternative than
   # to emit code which evaluates the case.expr
   def resolve_simple_field(self, bitset, name):
      field = None
   for case in bitset.cases:
         if name in case.fields:
   if field is not None:
         if bitset.extends is not None:
            template = """\
   /* Copyright (C) 2020 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "${header}"
      /*
   * enum tables, these don't have any link back to other tables so just
   * dump them up front before the bitset tables
   */
      %for name, enum in isa.enums.items():
   static const struct isa_enum ${enum.get_c_name()} = {
      .num_values = ${len(enum.values)},
      %   for val, display in enum.values.items():
         %   endfor
         };
   %endfor
      /*
   * generated expression functions, can be linked from bitset tables, so
   * also dump them up front
   */
      %for name, expr in isa.expressions.items():
   static uint64_t
   ${expr.get_c_name()}(struct decode_scope *scope)
   {
   %   for fieldname in sorted(expr.fieldnames):
         %   endfor
         }
   %endfor
      /* forward-declarations of bitset decode functions */
   %for name, bitset in isa.all_bitsets():
   %   for df in s.decode_fields(bitset):
   static void decode_${bitset.get_c_name()}_gen_${bitset.gen_min}_${df.get_c_name()}(void *out, struct decode_scope *scope, uint64_t val);
   %   endfor
   static const struct isa_field_decode decode_${bitset.get_c_name()}_gen_${bitset.gen_min}_fields[] = {
   %   for df in s.decode_fields(bitset):
      {
      .name = "${df.name}",
         %   endfor
   };
   static void decode_${bitset.get_c_name()}_gen_${bitset.gen_min}(void *out, struct decode_scope *scope);
   %endfor
      /*
   * Forward-declarations (so we don't have to figure out which order to
   * emit various tables when they have pointers to each other)
   */
      %for name, bitset in isa.all_bitsets():
   static const struct isa_bitset bitset_${bitset.get_c_name()}_gen_${bitset.gen_min};
   %endfor
      %for root_name, root in isa.roots.items():
   const struct isa_bitset *${root.get_c_name()}[];
   %endfor
      /*
   * bitset tables:
   */
      %for name, bitset in isa.all_bitsets():
   %   for case in bitset.cases:
   %      for field_name, field in case.fields.items():
   %         if field.get_c_typename() == 'TYPE_BITSET':
   %            if len(field.params) > 0:
   static const struct isa_field_params ${case.get_c_name()}_gen_${bitset.gen_min}_${field.get_c_name()} = {
         .num_params = ${len(field.params)},
   %               for param in field.params:
         %               endfor
            };
   %            endif
   %         endif
   %      endfor
   static const struct isa_case ${case.get_c_name()}_gen_${bitset.gen_min} = {
   %   if case.expr is not None:
         %   endif
   %   if case.display is not None:
         %   endif
         .num_fields = ${len(case.fields)},
   %   for field_name, field in case.fields.items():
         %      if field.expr is not None:
         %      endif
   %      if field.display is not None:
         %      endif
         %      if field.get_c_typename() == 'TYPE_BITSET':
         %         if len(field.params) > 0:
         %         endif
   %      endif
   %      if field.get_c_typename() == 'TYPE_ENUM':
         %      endif
   %      if field.get_c_typename() == 'TYPE_ASSERT':
         %      endif
   %      if field.get_c_typename() == 'TYPE_BRANCH' or field.get_c_typename() == 'TYPE_ABSBRANCH':
         %      endif
         %   endfor
         };
   %   endfor
   static const struct isa_bitset bitset_${bitset.get_c_name()}_gen_${bitset.gen_min} = {
   <% pattern = bitset.get_pattern() %>
   %   if bitset.extends is not None:
         %   endif
         .name     = "${bitset.display_name}",
   .gen      = {
      .min  = ${bitset.get_gen_min()},
      },
   .match.bitset    = { ${', '.join(isa.split_bits(pattern.match, 32))} },
   .dontcare.bitset = { ${', '.join(isa.split_bits(pattern.dontcare, 32))} },
   .mask.bitset     = { ${', '.join(isa.split_bits(pattern.mask, 32))} },
   .decode = decode_${bitset.get_c_name()}_gen_${bitset.gen_min},
   .num_decode_fields = ARRAY_SIZE(decode_${bitset.get_c_name()}_gen_${bitset.gen_min}_fields),
   .decode_fields = decode_${bitset.get_c_name()}_gen_${bitset.gen_min}_fields,
   .num_cases = ${len(bitset.cases)},
   %   for case in bitset.cases:
         %   endfor
         };
   %endfor
      /*
   * bitset hierarchy root tables (where decoding starts from):
   */
      %for root_name, root in isa.roots.items():
   const struct isa_bitset *${root.get_c_name()}[] = {
   %   for leaf_name, leafs in isa.leafs.items():
   %      for leaf in leafs:
   %         if leaf.get_root() == root:
         %         endif
   %      endfor
   %   endfor
         };
   %endfor
      #include "isaspec_decode_impl.c"
      %for name, bitset in isa.all_bitsets():
   %   for df in s.decode_fields(bitset):
   <%  field = s.resolve_simple_field(bitset, df.name) %>
   static void decode_${bitset.get_c_name()}_gen_${bitset.gen_min}_${df.get_c_name()}(void *out, struct decode_scope *scope, uint64_t val)
   {
   %       if bitset.get_root().decode is not None and field is not None:
         %           if field.get_c_typename() == 'TYPE_BITSET':
         %           elif field.get_c_typename() in ['TYPE_BRANCH', 'TYPE_INT', 'TYPE_OFFSET']:
         %           else:
         %           endif
         %       endif
   }
      %   endfor
   static void decode_${bitset.get_c_name()}_gen_${bitset.gen_min}(void *out, struct decode_scope *scope)
   {
   %   if bitset.get_root().decode is not None:
         %       if bitset.get_root().encode.type.endswith('*') and name in isa.leafs and bitset.get_root().encode.case_prefix is not None:
      src = ${bitset.get_root().get_c_name()}_create(${s.case_name(bitset.get_root(), bitset.name)});
      %       endif
   %   endif
   }
   %endfor
      """
      header = """\
   /* Copyright (C) 2020 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #ifndef _${guard}_
   #define _${guard}_
      #include <stdint.h>
   #include <util/bitset.h>
      #define BITMASK_WORDS BITSET_WORDS(${isa.bitsize})
      typedef struct {
         } bitmask_t;
         #define BITSET_FORMAT ${isa.format()}
   #define BITSET_VALUE(v) ${isa.value()}
      static inline void
   next_instruction(bitmask_t *instr, BITSET_WORD *start)
   {
      %for i in range(0, int(isa.bitsize / 32)):
   instr->bitset[${i}] = *(start + ${i});
      }
      static inline uint64_t
   bitmask_to_uint64_t(bitmask_t mask)
   {
   %   if isa.bitsize <= 32:
         %   else:
         %   endif
   }
      static inline bitmask_t
   uint64_t_to_bitmask(uint64_t val)
   {
      bitmask_t mask = {
      %   if isa.bitsize > 32:
         %   endif
                  }
      #include "isaspec_decode_decl.h"
      #endif /* _${guard}_ */
      """
      def guard(p):
            def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--xml', required=True, help='isaspec XML file.')
   parser.add_argument('--out-c', required=True, help='Output C file.')
   parser.add_argument('--out-h', required=True, help='Output H file.')
            isa = ISA(args.xml)
            try:
      with open(args.out_c, 'w') as f:
                  with open(args.out_h, 'w') as f:
         except Exception:
      # In the event there's an error, this imports some helpers from mako
   # to print a useful stack trace and prints it, then exits with
   # status 1, if python is run with debug; otherwise it just raises
   # the exception
   import sys
   from mako import exceptions
   print(exceptions.text_error_template().render(), file=sys.stderr)
      if __name__ == '__main__':
      main()
