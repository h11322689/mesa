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
   from isa import ISA, BitSetDerivedField, BitSetAssertField
   import argparse
   import sys
   import re
      # Encoding is driven by the display template that would be used
   # to decode any given instruction, essentially working backwards
   # from the decode case.  (Or put another way, the decoded bitset
   # should contain enough information to re-encode it again.)
   #
   # In the xml, we can have multiple override cases per bitset,
   # which can override display template and/or fields.  Iterating
   # all this from within the template is messy, so use helpers
   # outside of the template for this.
   #
   # The hierarchy of iterators for encoding is:
   #
   #   // First level - Case()  (s.bitset_cases() iterator)
   #   if (caseA.expression()) {  // maps to <override/> in xml
   #      // Second level - DisplayField()  (case.display_fields() iterator)
   #      ... encode field A ...
   #      ... encode field B ...
   #
   #      // Third level - each display field can be potentially resolved
   #      // by multiple different overrides, you can end up with
   #      // an if/else ladder for an individual display field
   #      if (field_c_case1.expression()) {
   #         ... encode field C ...
   #      } else if (field_c_case2.expression() {
   #         ... encode field C ...
   #      } else {
   #      }
   #
   #   } else if (caseB.expression())(
   #   } else {  // maps to the default case in bitset, ie. outside <override/>
   #   }
         # Represents a concrete field, ie. a field can be overriden
   # by an override, so the exact choice to encode a given field
   # in a bitset may be conditional
   class FieldCase(object):
      def __init__(self, bitset, field, case):
      self.field = field
   self.expr  = None
   if case.expr is not None:
         def signed(self):
      if self.field.type in ['int', 'offset', 'branch']:
            class AssertField(object):
      def __init__(self, bitset, field, case):
      self.field = field
   self.expr  = None
   if case.expr is not None:
         def signed(self):
         # Represents a field to be encoded:
   class DisplayField(object):
      def __init__(self, bitset, case, name):
      self.bitset = bitset   # leaf bitset
   self.case = case
         def fields(self, bitset=None):
      if bitset is None:
         # resolving the various cases for encoding a given
   # field is similar to resolving the display template
   # string
   for case in bitset.cases:
         if case.expr is not None:
      expr = bitset.isa.expressions[case.expr]
      if self.name in case.fields:
      field = case.fields[self.name]
   # For bitset fields, the bitset type could reference
   # fields in this (the containing) bitset, in addition
   # to the ones which are directly used to encode the
   # field itself.
   if field.get_c_typename() == 'TYPE_BITSET':
      for param in field.params:
      # For derived fields, we want to consider any other
   # fields that are referenced by the expr
   if isinstance(field, BitSetDerivedField):
      expr = bitset.isa.expressions[field.expr]
      elif not isinstance(field, BitSetAssertField):
         # if we've found an unconditional case specifying
   # the named field, we are done
      if bitset.extends is not None:
      # Represents an if/else case in bitset encoding which has a display
   # template string:
   class Case(object):
      def __init__(self, bitset, case):
      self.bitset = bitset   # leaf bitset
   self.case = case
   self.expr = None
   if case.expr is not None:
         self.fieldnames = re.findall(r"{([a-zA-Z0-9_:=]+)}", case.display)
            # remove special fieldname properties e.g. :align=
         # Handle fields which don't appear in display template but have
   # force="true"
   def append_forced(self, bitset):
      if bitset.encode is not None:
         for name, val in bitset.encode.forced.items():
   if bitset.extends is not None:
         # In the process of resolving a field, we might discover additional
   # fields that need resolving:
   #
   # a) a derived field which maps to one or more other concrete fields
   # b) a bitset field, which may be "parameterized".. for example a
   #    #multisrc field which refers back to SRC1_R/SRC2_R outside of
   #    the range of bits covered by the #multisrc field itself
   def append_field(self, fieldname):
      if fieldname not in self.fieldnames:
         def append_expr_fields(self, expr):
      for fieldname in expr.fieldnames:
         def display_fields(self):
      for fieldname in self.fieldnames:
         def assert_cases(self, bitset=None):
      if bitset is None:
         for case in bitset.cases:
         for name, field in case.fields.items():
         if bitset.extends is not None:
      # State and helpers used by the template:
   class State(object):
      def __init__(self, isa):
      self.isa = isa
         def bitset_cases(self, bitset, leaf_bitset=None):
      if leaf_bitset is None:
         for case in bitset.cases:
         if case.display is None:
      # if this is the last case (ie. case.expr is None)
   # then we need to go up the inheritance chain:
   if case.expr is None and bitset.extends is not None:
      parent_bitset = bitset.isa.bitsets[bitset.extends]
         # Find unique bitset remap/parameter names, to generate a struct
   # used to pass "parameters" to bitset fields:
   def unique_param_names(self):
      unique_names = []
   for root in self.encode_roots():
         for leaf in self.encode_leafs(root):
      for case in self.bitset_cases(leaf):
      for df in case.display_fields():
         for f in df.fields():
      if f.field.get_c_typename() == 'TYPE_BITSET':
            def case_name(self, bitset, name):
            def encode_roots(self):
      for name, root in self.isa.roots.items():
      if root.encode is None:
            def encode_leafs(self, root):
      for name, leafs in self.isa.leafs.items():
         for leaf in leafs:
               def encode_leaf_groups(self, root):
      for name, leafs in self.isa.leafs.items():
         if leafs[0].get_root() != root:
         # expressions used in a bitset (case or field or recursively parent bitsets)
   def bitset_used_exprs(self, bitset):
      for case in bitset.cases:
      if case.expr:
         for name, field in case.fields.items():
      if isinstance(field, BitSetDerivedField):
   if bitset.extends is not None:
         def extractor_impl(self, bitset, name):
      if bitset.encode is not None:
         if name in bitset.encode.maps:
   if bitset.extends is not None:
               # Default fallback when no mapping is defined, simply to avoid
   # having to deal with encoding at the same time as r/e new
   # instruction decoding.. but we can at least print warnings:
   def extractor_fallback(self, bitset, name):
      extr_name = bitset.name + '.' + name
   if extr_name not in self.warned_missing_extractors:
         print('WARNING: no encode mapping for {}.{}'.format(bitset.name, name))
         def extractor(self, bitset, name):
      extr = self.extractor_impl(bitset, name)
   if extr is not None:
               # In the special case of needing to access a field with bitset type
   # for an expr, we need to encode the field so we end up with an
   # integer, and not some pointer to a thing that will be encoded to
   # an integer
   def expr_extractor(self, bitset, name, p):
      extr = self.extractor_impl(bitset, name)
   field = self.resolve_simple_field(bitset, name)
   if isinstance(field, BitSetDerivedField):
         expr = self.isa.expressions[field.expr]
   if extr is None:
         if name in self.unique_param_names():
         else:
   if field and field.get_c_typename() == 'TYPE_BITSET':
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
               def encode_type(self, bitset):
      if bitset.encode is not None:
         if bitset.encode.type is not None:
   if bitset.extends is not None:
               def expr_name(self, root, expr):
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
      #include <stdbool.h>
   #include <stdint.h>
   #include <util/bitset.h>
      <%
   isa = s.isa
   %>
      #define BITMASK_WORDS BITSET_WORDS(${isa.bitsize})
      typedef struct {
         } bitmask_t;
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
      static inline void
   store_instruction(BITSET_WORD *dst, bitmask_t instr)
   {
   %   for i in range(0, int(isa.bitsize / 32)):
         %   endfor
   }
      /**
   * Opaque type from the PoV of generated code, but allows state to be passed
   * thru to the hand written helpers used by the generated code.
   */
   struct encode_state;
      /**
   * Allows to use gpu_id in expr functions
   */
   #define ISA_GPU_ID() s->gen
      struct bitset_params;
      static bitmask_t
   pack_field(unsigned low, unsigned high, int64_t val, bool is_signed)
   {
               if (is_signed) {
      /* NOTE: Don't assume val is already sign-extended to 64b,
   * just check that the bits above the valid range are either
   * all zero or all one:
   */
   assert(!(( val & ~BITFIELD64_MASK(1 + high - low)) &&
      } else {
                           if (!val)
            BITSET_ZERO(mask.bitset);
            field = uint64_t_to_bitmask(val);
   BITSET_AND(field.bitset, field.bitset, mask.bitset);
               }
      /*
   * Forward-declarations (so we don't have to figure out which order to
   * emit various encoders when they have reference each other)
   */
      %for root in s.encode_roots():
   static bitmask_t encode${root.get_c_name()}(struct encode_state *s, struct bitset_params *p, ${root.encode.type} src);
   %endfor
      ## TODO before the expr evaluators, we should generate extract_FOO() for
   ## derived fields.. which probably also need to be in the context of the
   ## respective root so they take the correct src arg??
      /*
   * Expression evaluators:
   */
      struct bitset_params {
   %for name in s.unique_param_names():
         %endfor
   };
      ## TODO can we share this def between the two templates somehow?
   <%def name="encode_params(leaf, field)">
   struct bitset_params bp = {
   %for param in field.params:
         %endfor
   };
   </%def>
      <%def name="render_expr(leaf, expr)">
   static inline int64_t
   ${s.expr_name(leaf.get_root(), expr)}(struct encode_state *s, struct bitset_params *p, ${leaf.get_root().encode.type} src)
   {
   %   for fieldname in expr.fieldnames:
         %   endfor
   %   for fieldname in expr.fieldnames:
   <% field = s.resolve_simple_field(leaf, fieldname) %>
   %      if field is not None and field.get_c_typename() == 'TYPE_BITSET':
            { ${encode_params(leaf, field)}
   const bitmask_t tmp = ${s.expr_extractor(leaf, fieldname, '&bp')};
      %      else:
         %      endif
   %   endfor
         }
   </%def>
      ## note, we can't just iterate all the expressions, but we need to find
   ## the context in which they are used to know the correct src type
      %for root in s.encode_roots():
   %   for leaf in s.encode_leafs(root):
   %      for expr in s.bitset_used_exprs(leaf):
   static inline int64_t ${s.expr_name(leaf.get_root(), expr)}(struct encode_state *s, struct bitset_params *p, ${leaf.get_root().encode.type} src);
   %      endfor
   %   endfor
   %endfor
      %for root in s.encode_roots():
   <%
         %>
   %   for leaf in s.encode_leafs(root):
   %      for expr in s.bitset_used_exprs(leaf):
   <%
            if expr in rendered_exprs:
      %>
         %      endfor
   %   endfor
   %endfor
         /*
   * The actual encoder definitions
   */
      %for root in s.encode_roots():
   %   for leaf in s.encode_leafs(root):
   <% snippet = encode_bitset.render(s=s, root=root, leaf=leaf) %>
   %      if snippet not in root.snippets.keys():
   <% snippet_name = "snippet" + root.get_c_name() + "_" + str(len(root.snippets)) %>
   static bitmask_t
   ${snippet_name}(struct encode_state *s, struct bitset_params *p, ${root.encode.type} src)
   {
         ${snippet}
         }
   <% root.snippets[snippet] = snippet_name %>
   %      endif
   %   endfor
      static bitmask_t
   encode${root.get_c_name()}(struct encode_state *s, struct bitset_params *p, ${root.encode.type} src)
   {
   %   if root.encode.case_prefix is not None:
         %      for leafs in s.encode_leaf_groups(root):
         %         for leaf in leafs:
   %           if leaf.has_gen_restriction():
         %           endif
   <% snippet = encode_bitset.render(s=s, root=root, leaf=leaf) %>
   <%    words = isa.split_bits((leaf.get_pattern().match), 64) %>
            <%    words.pop() %>
      %     for x in reversed(range(len(words))):
         {
      bitmask_t word = uint64_t_to_bitmask(${words[x]});
   BITSET_SHL(val.bitset, 64);
      %     endfor
            BITSET_OR(val.bitset, val.bitset, ${root.snippets[snippet]}(s, p, src).bitset);
   %           if leaf.has_gen_restriction():
         %           endif
   %         endfor
   %         if leaf.has_gen_restriction():
         %         endif
         %      endfor
      default:
      /* Note that we need the default case, because there are
   * instructions which we never expect to be encoded, (ie.
   * meta/macro instructions) as they are removed/replace
   * in earlier stages of the compiler.
   */
      }
   mesa_loge("Unhandled ${root.name} encode case: 0x%x\\n", ${root.get_c_name()}_case(s, src));
      %   else: # single case bitset, no switch
   %      for leaf in s.encode_leafs(root):
   <% snippet = encode_bitset.render(s=s, root=root, leaf=leaf) %>
         bitmask_t val = uint64_t_to_bitmask(${hex(leaf.get_pattern().match)});
   BITSET_OR(val.bitset, val.bitset, ${root.snippets[snippet]}(s, p, src).bitset);
   %      endfor
   %   endif
   }
   %endfor
   """
      encode_bitset_template = """
   <%
   isa = s.isa
   %>
      <%def name="case_pre(root, expr)">
   %if expr is not None:
         %else:
         %endif
   </%def>
      <%def name="case_post(root, expr)">
   %if expr is not None:
         %else:
         %endif
   </%def>
      <%def name="encode_params(leaf, field)">
   struct bitset_params bp = {
   %for param in field.params:
         %endfor
   };
   </%def>
                     <% visited_exprs = [] %>
   %for case in s.bitset_cases(leaf):
   <%
      if case.expr is not None:
            # per-expression-case track display-field-names that we have
   # already emitted encoding for.  It is possible that an
   # <override> case overrides a given field (for ex. #cat5-src3)
   # and we don't want to emit encoding for both the override and
   # the fallback
      %>
         %   for df in case.display_fields():
   %       for f in df.fields():
   <%
            # simplify the control flow a bit to give the compiler a bit
   # less to clean up
   expr = f.expr
   if expr == case.expr:
      # Don't need to evaluate the same condition twice:
      elif expr in visited_exprs:
      # We are in an 'else'/'else-if' leg that we wouldn't
                              if f.field.name in seen_fields[expr]:
      %>
         %         if f.field.get_c_typename() == 'TYPE_BITSET':
               { ${encode_params(leaf, f.field)}
         %         else:
         %         endif
               const bitmask_t packed = pack_field(${f.field.low}, ${f.field.high}, fld, ${f.signed()});  /* ${f.field.name} */
   %       endfor
   %   endfor
      %   for f in case.assert_cases():
   <%
         # simplify the control flow a bit to give the compiler a bit
   # less to clean up
   expr = f.expr
   if expr == case.expr:
      # Don't need to evaluate the same condition twice:
      elif expr in visited_exprs:
      # We are in an 'else'/'else-if' leg that we wouldn't
      %>
         ${case_pre(root, expr)}
   const bitmask_t packed = pack_field(${f.field.low}, ${f.field.high}, ${f.field.val}, ${f.signed()});
   BITSET_OR(val.bitset, val.bitset, packed.bitset);
   %   endfor
               %endfor
   """
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--xml', required=True, help='isaspec XML file.')
   parser.add_argument('--out-h', required=True, help='Output H file.')
            isa = ISA(args.xml)
            try:
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
