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
      from xml.etree import ElementTree
   import os
   import re
      def dbg(str):
      if False:
         class BitSetPattern(object):
      """Class that encapsulated the pattern matching, ie.
      the match/dontcare/mask bitmasks.  The following
                           For a leaf node, the mask should be (1 << size) - 1
      """
   def __init__(self, bitset):
      self.match      = bitset.match
   self.dontcare   = bitset.dontcare
   self.mask       = bitset.mask
         def merge(self, pattern):
      p = BitSetPattern(pattern)
   p.match      = p.match      | self.match
   p.dontcare   = p.dontcare   | self.dontcare
   p.mask       = p.mask       | self.mask
   p.field_mask = p.field_mask | self.field_mask
         def defined_bits(self):
         def get_bitrange(field):
      if 'pos' in field.attrib:
      assert('low' not in field.attrib)
   assert('high' not in field.attrib)
   low = int(field.attrib['pos'])
      else:
      low = int(field.attrib['low'])
      assert low <= high
         def extract_pattern(xml, name, is_defined_bits=None):
      low, high = get_bitrange(xml)
                     assert (len(patstr) == (1 + high - low)), "Invalid {} length in {}: {}..{}".format(xml.tag, name, low, high)
   if is_defined_bits is not None:
            match = 0
            for n in range(0, len(patstr)):
      match = match << 1
   dontcare = dontcare << 1
   if patstr[n] == '1':
         elif patstr[n] == 'x':
         elif patstr[n] != '0':
                        def get_c_name(name):
            class BitSetField(object):
      """Class that encapsulates a field defined in a bitset
   """
   def __init__(self, isa, xml):
      self.isa = isa
   self.low, self.high = get_bitrange(xml)
   self.name = xml.attrib['name']
   self.type = xml.attrib['type']
   self.params = []
   for param in xml.findall('param'):
         aas = name = param.attrib['name']
   if 'as' in param.attrib:
         self.expr = None
   self.display = None
   self.call = 'call' in xml.attrib and xml.attrib['call'] == 'true'
   if 'display' in xml.attrib:
         def get_c_name(self):
            def get_c_typename(self):
      if self.type in self.isa.enums:
         if self.type in self.isa.bitsets:
               def mask(self):
            def get_size(self):
         class BitSetAssertField(BitSetField):
      """Similar to BitSetField, but for <assert/>s, which can be
      used to specify that a certain bitpattern is expected in
      """
   def __init__(self, case, xml):
      self.isa = case.bitset.isa
   self.low, self.high = get_bitrange(xml)
   self.name = case.bitset.name + '#assert' + str(len(case.fields))
   self.type = 'uint'
   self.expr = None
            match, dontcare, mask = extract_pattern(xml, case.bitset.name)
                  def get_c_typename(self):
         class BitSetDerivedField(BitSetField):
      """Similar to BitSetField, but for derived fields
   """
   def __init__(self, isa, xml):
      self.isa = isa
   self.low = 0
   self.high = 0
   # NOTE: a width should be provided for 'int' derived fields, ie.
   # where sign extension is needed.  We just repurpose the 'high'
   # field for that to make '1 + high - low' work out
   if 'width' in xml.attrib:
         self.name = xml.attrib['name']
   self.type = xml.attrib['type']
   if 'expr' in xml.attrib:
         else:
         e = isa.parse_one_expression(xml, self.name)
   self.display = None
   if 'display' in xml.attrib:
            class BitSetCase(object):
      """Class that encapsulates a single bitset case
   """
   def __init__(self, bitset, xml, update_field_mask, expr=None):
      self.bitset = bitset
   if expr is not None:
         else:
         self.expr = expr
            for derived in xml.findall('derived'):
                  for assrt in xml.findall('assert'):
         f = BitSetAssertField(self, assrt)
            for field in xml.findall('field'):
         dbg("{}.{}".format(self.name, field.attrib['name']))
   f = BitSetField(bitset.isa, field)
            self.display = None
   for d in xml.findall('display'):
         # Allow <display/> for empty display string:
   if d.text is not None:
         else:
         def get_c_name(self):
         class BitSetEncode(object):
      """Additional data that may be associated with a root bitset node
      to provide additional information needed to generate helpers
   to encode the bitset, such as source data type and "opcode"
   case prefix (ie. how to choose/enumerate which leaf node bitset
      """
   def __init__(self, xml):
      self.type = None
   if 'type' in xml.attrib:
         self.case_prefix = None
   if 'case-prefix' in xml.attrib:
         # The encode element may also contain mappings from encode src
   # to individual field names:
   self.maps = {}
   self.forced = {}
   for map in xml.findall('map'):
         name = map.attrib['name']
   self.maps[name] = map.text.strip()
      class BitSetDecode(object):
      """Additional data that may be associated with a root bitset node
      to provide additional information needed to generate helpers
      """
   def __init__(self, xml):
         class BitSet(object):
      """Class that encapsulates a single bitset rule
   """
   def __init__(self, isa, xml):
      self.isa = isa
   self.xml = xml
   self.name = xml.attrib['name']
   self.display_name = xml.attrib['displayname'] if 'displayname' in xml.attrib else self.name
            # Used for generated encoder, to de-duplicate encoding for
   # similar instructions:
            if 'size' in xml.attrib:
         assert('extends' not in xml.attrib)
   self.size = int(xml.attrib['size'])
   else:
                  self.encode = None
   if xml.find('encode') is not None:
         self.decode = None
   if xml.find('decode') is not None:
            self.gen_min = 0
            for gen in xml.findall('gen'):
         if 'min' in gen.attrib:
                  if xml.find('meta') is not None:
            # Collect up the match/dontcare/mask bitmasks for
   # this bitset case:
   self.match = 0
   self.dontcare = 0
   self.mask = 0
                     # Helper to check for redefined bits:
   def is_defined_bits(m):
            def update_default_bitmask_field(bs, field):
         m = field.mask()
   dbg("field: {}.{} => {:016x}".format(self.name, field.name, m))
   # For default case, we don't expect any bits to be doubly defined:
   assert not is_defined_bits(m), "Redefined bits in field {}.{}: {}..{}".format(
            def update_override_bitmask_field(bs, field):
         m = field.mask()
                     for override in xml.findall('override'):
         if 'expr' in override.attrib:
         else:
      e = isa.parse_one_expression(override, self.name)
               # Default case is expected to be the last one:
            for pattern in xml.findall('pattern'):
                  self.match    |= match
         def get_pattern(self):
      if self.extends is not None:
         parent = self.isa.bitsets[self.extends]
                                    def get_size(self):
      if self.extends is not None:
         parent = self.isa.bitsets[self.extends]
         def get_gen_min(self):
      if self.extends is not None:
                                 def get_gen_max(self):
      if self.extends is not None:
                                 def has_gen_restriction(self):
            def get_c_name(self):
            def get_root(self):
      if self.extends is not None:
               def get_meta(self):
               if self.extends is not None:
            if self.meta:
               class BitSetTemplate(object):
      """Class that encapsulates a template declaration
   """
   def __init__(self, isa, xml):
      self.isa = isa
   self.name = xml.attrib['name']
   self.display = xml.text.strip()
      class BitSetEnumValue(object):
      """Class that encapsulates an enum value
   """
   def __init__(self, isa, xml):
      self.isa = isa
   self.displayname = xml.attrib['display']
   self.value = xml.attrib['val']
         def __str__(self):
            def get_name(self):
            def get_value(self):
         class BitSetEnum(object):
      """Class that encapsulates an enum declaration
   """
   def __init__(self, isa, xml):
      self.isa = isa
            # Table mapping value to name
   self.values = {}
   for value in xml.findall('value'):
               def get_c_name(self):
         class BitSetExpression(object):
      """Class that encapsulates an <expr> declaration
   """
   def __init__(self, isa, xml):
      self.isa = isa
   if 'name' in xml.attrib:
         else:
         self.name = 'anon_' + str(isa.anon_expression_count)
   expr = xml.text.strip()
   self.fieldnames = list(set(re.findall(r"{([a-zA-Z0-9_]+)}", expr)))
   self.expr = re.sub(r"{([a-zA-Z0-9_]+)}", r"\1", expr)
         def get_c_name(self):
         class ISA(object):
      """Class that encapsulates all the parsed bitset rules
   """
   def __init__(self, xmlpath):
               # Counter used to name inline (anonymous) expressions:
            # Table of (globally defined) expressions:
            # Table of templates:
            # Table of enums:
            # Table of toplevel bitset hierarchies:
            # Table of leaf nodes of bitset hierarchies:
   # Note that there may be multiple leaves for a particular name
   # (distinguished by gen), so the values here are lists.
            # Table of all non-ambiguous bitsets (i.e. no per-gen ambiguity):
            # Max needed bitsize for one instruction
            root = ElementTree.parse(xmlpath).getroot()
   self.parse_file(root)
         def parse_expressions(self, root):
      e = None
   for expr in root.findall('expr'):
         e = BitSetExpression(self, expr)
         def parse_one_expression(self, root, name):
      assert len(root.findall('expr')) == 1, "expected a single expression in: {}".format(name)
         def parse_file(self, root):
      # Handle imports up-front:
   for imprt in root.findall('import'):
                  # Extract expressions:
            # Extract templates:
   for template in root.findall('template'):
                  # Extract enums:
   for enum in root.findall('enum'):
                  # Extract bitsets:
   for bitset in root.findall('bitset'):
         b = BitSet(self, bitset)
   if b.size is not None:
      dbg("toplevel: " + b.name)
   self.roots[b.name] = b
      else:
                  # Resolve all templates:
   for _, bitset in self.bitsets.items():
         for case in bitset.cases:
         def validate_isa(self):
      # We only support multiples of 32 bits for now
            # Do one-time fixups
   # Remove non-leaf nodes from the leafs table:
   for name, bitsets in list(self.leafs.items()):
         for bitset in bitsets:
            # Fix multi-gen leaves in bitsets
   for name, bitsets in self.leafs.items():
                           # Validate that all bitset fields have valid types, and in
   # the case of bitset type, the sizes match:
   builtin_types = ['branch', 'absbranch', 'int', 'uint', 'hex', 'offset', 'uoffset', 'float', 'bool', 'enum', 'custom']
   for bitset_name, bitset in self.bitsets.items():
         if bitset.extends is not None:
      assert bitset.extends in self.bitsets, "{} extends invalid type: {}".format(
      for case in bitset.cases:
                              if not isinstance(field, BitSetDerivedField):
                        if field.type in builtin_types:
         if field.type in self.enums:
         assert field.type in self.bitsets, "{}.{}: invalid type: {}".format(
                  # Validate that all the leaf node bitsets have no remaining
   # undefined bits
   for name, bitsets in self.leafs.items():
         for bitset in bitsets:
      pat = bitset.get_pattern()
               # TODO somehow validating that only one bitset in a hierarchy
            # TODO we should probably be able to look at the contexts where
   # an expression is evaluated and verify that it doesn't have any
         def format(self):
      ''' Generate format string used by printf(..) and friends '''
   parts = []
            for i in range(int(words)):
                           def value(self):
      ''' Generate format values used by printf(..) and friends '''
   parts = []
            for i in range(int(words) - 1, -1, -1):
                  def split_bits(self, value, bitsize):
      ''' Split `value` into a list of bitsize-bit integers '''
   mask, parts = (1 << bitsize) - 1, []
            while value:
                  # Add 'missing' words
   while len(parts) < words:
                  # Returns all bitsets in the ISA, including all per-gen variants, in
   # (name, bitset) pairs.
   def all_bitsets(self):
      for name, bitset in self.bitsets.items():
         for name, bitsets in self.leafs.items():
         if len(bitsets) == 1:
               def resolve_templates(self, display_string):
      matches = re.findall(r'\{([^\}]+)\}', display_string)
   for m in matches:
                        def instructions(self):
               for _, root in self.roots.items():
         for _, leafs in self.leafs.items():
      for leaf in leafs:
               return instr
