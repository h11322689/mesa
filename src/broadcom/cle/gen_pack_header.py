   #encoding=utf-8
      # Copyright (C) 2016 Intel Corporation
   # Copyright (C) 2016 Broadcom
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
      import xml.parsers.expat
   import re
   import sys
      license =  """/* Generated code, see v3d_packet_v21.xml, v3d_packet_v33.xml and gen_pack_header.py */
   """
      pack_header = """%(license)s
      /* Packets, enums and structures for %(platform)s.
   *
   * This file has been generated, do not hand edit.
   */
      #ifndef %(guard)s
   #define %(guard)s
      #include "cle/v3d_packet_helpers.h"
      """
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
                  def prefixed_upper_name(prefix, name):
      if prefix:
               def num_from_str(num_str):
      if num_str.lower().startswith('0x'):
         else:
      assert(not num_str.startswith('0') and 'octals numbers not allowed')
      class Field(object):
      ufixed_pattern = re.compile(r"u(\d+)\.(\d+)")
            def __init__(self, parser, attrs):
      self.parser = parser
   if "name" in attrs:
            if str(attrs["start"]).endswith("b"):
         else:
         # packet <field> entries in XML start from the bit after the
   # opcode, so shift everything up by 8 since we'll also have a
   # Field for the opcode.
   if not parser.struct:
            self.end = self.start + int(attrs["size"]) - 1
            if self.type == 'bool' and self.start != self.end:
            if "prefix" in attrs:
         else:
            if "default" in attrs:
         else:
            if "minus_one" in attrs:
         assert(attrs["minus_one"] == "true")
   else:
            ufixed_match = Field.ufixed_pattern.match(self.type)
   if ufixed_match:
                  sfixed_match = Field.sfixed_pattern.match(self.type)
   if sfixed_match:
               def emit_template_struct(self, dim):
      if self.type == 'address':
         elif self.type == 'bool':
         elif self.type == 'float':
         elif self.type == 'f187':
         elif self.type == 'ufixed':
         elif self.type == 'sfixed':
         elif self.type == 'uint' and self.end - self.start > 32:
         elif self.type == 'offset':
         elif self.type == 'int':
         elif self.type == 'uint':
         elif self.type in self.parser.structs:
         elif self.type in self.parser.enums:
         elif self.type == 'mbo':
         else:
                           for value in self.values:
               def overlaps(self, field):
            class Group(object):
      def __init__(self, parser, parent, start, count):
      self.parser = parser
   self.parent = parent
   self.start = start
   self.count = count
   self.size = 0
   self.fields = []
   self.min_ver = 0
         def emit_template_struct(self, dim):
      if self.count == 0:
         else:
                              class Byte:
      def __init__(self):
         self.size = 8
         def collect_bytes(self, bytes):
      for field in self.fields:
                        for b in range(first_byte, last_byte + 1):
                                       def emit_pack_function(self, start):
      # Determine number of bytes in this group.
            bytes = {}
            relocs_emitted = set()
            for field in self.fields:
                  for index in range(self.length):
         # Handle MBZ bytes
   if index not in bytes:
                        # Call out to the driver to note our relocations.  Inside of the
   # packet we only store offsets within the BOs, and we store the
   # handle to the packet outside.  Unlike Intel genxml, we don't
   # need to have the other bits that will be stored together with
   # the address during the reloc process, so there's no need for the
   # complicated combine_address() function.
   if byte.address and byte.address not in relocs_emitted:
                  # Special case: floats can't have any other fields packed into
   # them (since they'd change the meaning of the float), and the
   # per-byte bitshifting math below bloats the pack code for floats,
   # so just copy them directly here.  Also handle 16/32-bit
   # uints/ints with no merged fields.
   if len(byte.fields) == 1:
      field = byte.fields[0]
                           if not any(field.overlaps(scan_field) for scan_field in self.fields):
         assert(field.start == index * 8)
   print("")
                                    field_index = 0
   for field in byte.fields:
                     start = field.start
   end = field.end
   field_byte_start = (field.start // 8) * 8
                                             if field.type == "mbo":
      s = "util_bitpack_ones(%d, %d)" % \
      elif field.type == "address":
      extra_shift = (31 - (end - start)) // 8 * 8
      elif field.type == "uint":
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type in self.parser.enums:
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "int":
      s = "util_bitpack_sint(%s, %d, %d)" % \
      elif field.type == "bool":
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "float":
         elif field.type == "f187":
      s = "util_bitpack_uint(fui(%s) >> 16, %d, %d)" % \
      elif field.type == "offset":
      s = "__gen_offset(%s, %d, %d)" % \
      elif field.type == 'ufixed':
      s = "util_bitpack_ufixed(%s, %d, %d, %d)" % \
      elif field.type == 'sfixed':
      s = "util_bitpack_sfixed(%s, %d, %d, %d)" % \
      elif field.type in self.parser.structs:
      s = "util_bitpack_uint(v%d_%d, %d, %d)" % \
                                 if s is not None:
                           if field == byte.fields[-1]:
                              def emit_unpack_function(self, start):
      for field in self.fields:
                           args = []
                        if field.type == "address":
         elif field.type == "uint":
         elif field.type in self.parser.enums:
         elif field.type == "int":
         elif field.type == "bool":
         elif field.type == "float":
         elif field.type == "f187":
         elif field.type == "offset":
         elif field.type == 'ufixed':
      args.append(str(field.fractional_size))
      elif field.type == 'sfixed':
      args.append(str(field.fractional_size))
                     plusone = ""
   if field.minus_one:
         class Value(object):
      def __init__(self, attrs):
      self.name = attrs["name"]
      class Parser(object):
      def __init__(self, ver):
      self.parser = xml.parsers.expat.ParserCreate()
   self.parser.StartElementHandler = self.start_element
            self.packet = None
   self.struct = None
   self.structs = {}
   # Set of enum names we've seen.
   self.enums = set()
   self.registers = {}
         def gen_prefix(self, name):
      if name[0] == "_":
         else:
         def gen_guard(self):
            def attrs_version_valid(self, attrs):
      if "min_ver" in attrs and self.ver < attrs["min_ver"]:
            if "max_ver" in attrs and self.ver > attrs["max_ver"]:
                  def group_enabled(self):
      if self.group.min_ver != 0 and self.ver < self.group.min_ver:
            if self.group.max_ver != 0 and self.ver > self.group.max_ver:
                  def start_element(self, name, attrs):
      if name == "vcxml":
         self.platform = "V3D {}.{}".format(self.ver[0], self.ver[1])
   elif name in ("packet", "struct", "register"):
                  object_name = self.gen_prefix(safe_name(attrs["name"].upper()))
                     # Add a fixed Field for the opcode.  We only make <field>s in
   # the XML for the fields listed in the spec, and all of those
   # start from bit 0 after of the opcode.
   default_field = {
      "name" : "opcode",
   "default" : attrs["code"],
   "type" : "uint",
   "start" : -8,
         elif name == "struct":
      self.struct = object_name
      elif name == "register":
                        self.group = Group(self, None, 0, 1)
   if default_field:
                        if "min_ver" in attrs:
                  elif name == "field":
         self.group.fields.append(Field(self, attrs))
   elif name == "enum":
         self.values = []
   self.enum = safe_name(attrs["name"])
   self.enums.add(attrs["name"])
   self.enum_enabled = self.attrs_version_valid(attrs)
   if "prefix" in attrs:
         else:
   elif name == "value":
               def end_element(self, name):
      if name  == "packet":
         self.emit_packet()
   self.packet = None
   elif name == "struct":
         self.emit_struct()
   self.struct = None
   elif name == "register":
         self.emit_register()
   self.register = None
   self.reg_num = None
   elif name  == "field":
         elif name  == "enum":
         if self.enum_enabled:
         elif name == "vcxml":
         def emit_template_struct(self, name, group):
      print("struct %s {" % name)
   group.emit_template_struct("")
         def emit_pack_function(self, name, group):
      print("static inline void\n%s_pack(__gen_user_data *data, uint8_t * restrict cl,\n%sconst struct %s * restrict values)\n{" %
                              print('#define %-33s %6d' %
         def emit_unpack_function(self, name, group):
      print("#ifdef __gen_unpack_address")
   print("static inline void")
   print("%s_unpack(const uint8_t * restrict cl,\n%sstruct %s * restrict values)\n{" %
                           def emit_header(self, name):
      default_fields = []
   for field in self.group.fields:
         if type(field) is not Field:
         if field.default is None:
            print('#define %-40s\\' % (name + '_header'))
   print(",  \\\n".join(default_fields))
         def emit_packet(self):
      if not self.group_enabled():
                     assert(self.group.fields[0].name == "opcode")
   print('#define %-33s %6d' %
            self.emit_header(name)
   self.emit_template_struct(self.packet, self.group)
   self.emit_pack_function(self.packet, self.group)
                  def emit_register(self):
      if not self.group_enabled():
            name = self.register
   if self.reg_num is not None:
                  self.emit_template_struct(self.register, self.group)
   self.emit_pack_function(self.register, self.group)
         def emit_struct(self):
      if not self.group_enabled():
                     self.emit_header(name)
   self.emit_template_struct(self.struct, self.group)
   self.emit_pack_function(self.struct, self.group)
                  def emit_enum(self):
      print('enum %s {' % self.gen_prefix(self.enum))
   for value in self.values:
         name = value.name
   if self.prefix:
         name = safe_name(name).upper()
         def parse(self, filename):
      file = open(filename, "rb")
   self.parser.ParseFile(file)
      if len(sys.argv) < 2:
      print("No input xml file specified")
         input_file = sys.argv[1]
      p = Parser(sys.argv[2])
   p.parse(input_file)
