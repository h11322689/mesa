   #encoding=utf-8
      # Copyright (C) 2016 Intel Corporation
   # Copyright (C) 2016 Broadcom
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
      import os
   import textwrap
   import xml.etree.ElementTree as ET
   import sys
      tree = ET.parse(os.path.join(os.path.dirname(__file__), 'ISA.xml'))
   root = tree.getroot()
      # All instructions in the ISA
   instructions = []
      # All immediates in the ISA
   ilut = root.findall('lut')[0]
   assert(ilut.attrib['name'] == "Immediates")
   immediates = [int(imm.text, base=0) for imm in ilut.findall('constant')]
   enums = {}
      def xmlbool(s):
      assert(s.lower() in ["false", "true"])
         class EnumValue:
      def __init__(self, value, default):
      self.value = value
      class Enum:
      def __init__(self, name, values):
      self.name = name
   self.values = values
            defaults = [x.value for x in values if x.default]
   if len(defaults) > 0:
            def build_enum(el):
               for child in el:
      if child.tag == 'value':
         is_default = child.attrib.get('default', False)
   elif child.tag == 'reserved':
               class Modifier:
      def __init__(self, name, start, size, implied = False, force_enum = None):
      self.name = name
   self.start = start
   self.size = size
   self.implied = implied
   self.is_enum = (force_enum is not None) or size > 1
            if not self.is_enum:
         self.bare_values = ['', name]
   else:
         self.bare_values = [x.value for x in enums[self.enum].values]
                  if len(defaults) > 0:
            def Flag(name, start):
            # Model a single instruction
   class Source:
      def __init__(self, index, size, is_float = False, swizzle = False,
            self.is_float = is_float or absneg
   self.start = (index * 8)
   self.size = size
   self.absneg = absneg
   self.notted = notted
   self.swizzle = swizzle
   self.halfswizzle = halfswizzle
   self.widen = widen
   self.lanes = lanes
   self.lane = lane
   self.combine = combine
            self.offset = {}
   self.bits = {}
   if absneg:
         self.offset['neg'] = 32 + 2 + ((2 - index) * 2)
   self.offset['abs'] = 33 + 2 + ((2 - index) * 2)
   self.bits['neg'] = 1
   if notted:
         self.offset['not'] = 35
   if widen or lanes or halfswizzle:
         self.offset['widen'] = 26 if index == 1 else 36
   if lane:
         self.offset['lane'] = self.lane
   if swizzle:
         assert(size in [16, 32])
   self.offset['swizzle'] = 24 + ((2 - index) * 2)
   if combine:
            class Dest:
      def __init__(self, name = ""):
         class Staging:
      def __init__(self, read = False, write = False, count = 0, flags = 'true', name = ""):
      self.name = name
   self.read = read
   self.write = write
   self.count = count
   self.flags = (flags != 'false')
            if write and not self.flags:
            # For compatibility
   self.absneg = False
   self.swizzle = False
   self.notted = False
   self.widen = False
   self.lanes = False
   self.lane = False
   self.halfswizzle = False
   self.combine = False
            if not self.flags:
         elif flags == 'rw':
         else:
            class Immediate:
      def __init__(self, name, start, size, signed):
      self.name = name
   self.start = start
   self.size = size
      class Instruction:
      def __init__(self, name, opcode, opcode2, srcs = [], dests = [], immediates = [], modifiers = [], staging = None, unit = None):
      self.name = name
   self.srcs = srcs
   self.dests = dests
   self.opcode = opcode
   self.opcode2 = opcode2 or 0
   self.immediates = immediates
   self.modifiers = modifiers
   self.staging = staging
   self.unit = unit
            # Message-passing instruction <===> not ALU instruction
            self.secondary_shift = max(len(self.srcs) * 8, 16)
   self.secondary_mask = 0xF if opcode2 is not None else 0x0
   if "left" in [x.name for x in self.modifiers]:
         if len(srcs) == 3 and (srcs[1].widen or srcs[1].lanes or srcs[1].swizzle):
         if opcode == 0x90:
         # XXX: XMLify this, but disambiguates sign of conversions
   if name.startswith("LOAD.i") or name.startswith("STORE.i") or name.startswith("LD_BUFFER.i"):
         self.secondary_shift = 27 # Alias with memory_size
   if "descriptor_type" in [x.name for x in self.modifiers]:
         self.secondary_mask = 0x3
   elif "memory_width" in [x.name for x in self.modifiers]:
                  assert(len(dests) == 0 or not staging)
         def __str__(self):
         # Build a single source from XML
   def build_source(el, i, size):
      lane = el.get('lane', None)
   if lane == "true":
         elif lane is not None:
            return Source(i, int(el.get('size', size)),
            absneg = el.get('absneg', False),
   is_float = el.get('float', False),
   swizzle = el.get('swizzle', False),
   halfswizzle = el.get('halfswizzle', False),
   widen = el.get('widen', False),
   lanes = el.get('lanes', False),
   combine = el.get('combine', False),
   lane = lane,
      def build_imm(el):
      return Immediate(el.attrib['name'], int(el.attrib['start']),
         def build_staging(i, el):
      r = xmlbool(el.attrib.get('read', 'false'))
   w = xmlbool(el.attrib.get('write', 'false'))
   count = int(el.attrib.get('count', '0'))
                  def build_modifier(el):
      name = el.attrib['name']
   start = int(el.attrib['start'])
   size = int(el.attrib['size'])
                  # Build a single instruction from XML and group based overrides
   def build_instr(el, overrides = {}):
      # Get overridables
   name = overrides.get('name') or el.attrib.get('name')
   opcode = overrides.get('opcode') or el.attrib.get('opcode')
   opcode2 = overrides.get('opcode2') or el.attrib.get('opcode2')
   unit = overrides.get('unit') or el.attrib.get('unit')
   opcode = int(opcode, base=0)
            # Get explicit sources/dests
   tsize = typesize(name)
   sources = []
            for src in el.findall('src'):
      built = build_source(src, i, tsize)
            # 64-bit sources in a 32-bit (message) instruction count as two slots
   # Affects BLEND, ST_CVT
   if tsize != 64 and built.size == 64:
         else:
                  # Get implicit ones
   sources = sources + ([Source(i, int(tsize)) for i in range(int(el.attrib.get('srcs', 0)))])
            # Get staging registers
            # Get immediates
            modifiers = []
   for mod in el:
      if mod.tag in MODIFIERS:
         elif mod.tag =='mod':
                        # Build all the instructions in a group by duplicating the group itself with
   # overrides for each distinct instruction
   def build_group(el):
      for ins in el.findall('ins'):
      build_instr(el, overrides = {
         'name': ins.attrib['name'],
   'opcode': ins.attrib.get('opcode'),
   'opcode2': ins.attrib.get('opcode2'),
      def to_alphanum(name):
      substitutions = {
      ' ': '_',
   '/': '_',
   '[': '',
   ']': '',
   '(': '',
   ')': '',
   '-': '_',
   ':': '',
   '.': '',
   ',': '',
   '=': '',
   '>': '',
   '#': '',
   '&': '',
   '*': '',
   '"': '',
   '+': '',
               for i, j in substitutions.items():
                  def safe_name(name):
      name = to_alphanum(name)
   if not name[0].isalpha():
                  # Parses out the size part of an opocde name
   def typesize(opcode):
      if opcode[-3:] == '128':
         if opcode[-2:] == '48':
         elif opcode[-1] == '8':
         else:
      try:
         except:
      for child in root.findall('enum'):
            MODIFIERS = {
      # Texture instructions share a common encoding
   "wide_indices": Flag("wide_indices", 8),
   "array_enable": Flag("array_enable", 10),
   "texel_offset": Flag("texel_offset", 11),
   "shadow": Flag("shadow", 12),
   "integer_coordinates": Flag("integer_coordinates", 13),
   "fetch_component": Modifier("fetch_component", 14, 2),
   "lod_mode": Modifier("lod_mode", 13, 3),
   "lod_bias_disable": Modifier("lod_mode", 13, 1),
   "lod_clamp_disable": Modifier("lod_mode", 14, 1),
   "write_mask": Modifier("write_mask", 22, 4),
   "register_type": Modifier("register_type", 26, 2),
   "dimension": Modifier("dimension", 28, 2),
   "skip": Flag("skip", 39),
   "register_width": Modifier("register_width", 46, 1, force_enum = "register_width"),
   "secondary_register_width": Modifier("secondary_register_width", 47, 1, force_enum = "register_width"),
            "atom_opc": Modifier("atomic_operation", 22, 4),
   "atom_opc_1": Modifier("atomic_operation_with_1", 22, 4),
   "inactive_result": Modifier("inactive_result", 22, 4),
   "memory_access": Modifier("memory_access", 24, 2),
   "regfmt": Modifier("register_format", 24, 3),
   "source_format": Modifier("source_format", 24, 4),
            "slot": Modifier("slot", 30, 3),
   "roundmode": Modifier("round_mode", 30, 2),
   "result_type": Modifier("result_type", 30, 2),
   "saturate": Flag("saturate", 30),
            "lane_op": Modifier("lane_operation", 32, 2),
   "cmp": Modifier("condition", 32, 3),
   "clamp": Modifier("clamp", 32, 2),
   "sr_count": Modifier("staging_register_count", 33, 3, implied = True),
   "sample_and_update": Modifier("sample_and_update_mode", 33, 3),
            "conservative": Flag("conservative", 35),
   "subgroup": Modifier("subgroup_size", 36, 4),
   "update": Modifier("update_mode", 36, 2),
      }
      # Parse the ISA
   for child in root:
      if child.tag == 'group':
         elif child.tag == 'ins':
         instruction_dict = { ins.name: ins for ins in instructions }
      # Validate there are no duplicated instructions
   if len(instruction_dict) != len(instructions):
      import collections
   counts = collections.Counter([i.name for i in instructions])
   for c in counts:
      if counts[c] != 1:
      assert(len(instruction_dict) == len(instructions))
