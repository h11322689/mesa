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
      import xml.parsers.expat
   import sys
   import operator
   from functools import reduce
      global_prefix = "mali"
      pack_header = """
   /* Generated code, see midgard.xml and gen_pack_header.py
   *
   * Packets, enums and structures for Panfrost.
   *
   * This file has been generated, do not hand edit.
   */
      #ifndef PAN_PACK_H
   #define PAN_PACK_H
      #include <stdio.h>
   #include <inttypes.h>
      #include "util/bitpack_helpers.h"
      #define __gen_unpack_float(x, y, z) uif(__gen_unpack_uint(x, y, z))
      static inline uint32_t
   __gen_padded(uint32_t v, uint32_t start, uint32_t end)
   {
      unsigned shift = __builtin_ctz(v);
         #ifndef NDEBUG
      assert((v >> shift) & 1);
   assert(shift <= 31);
   assert(odd <= 7);
      #endif
            }
         static inline uint64_t
   __gen_unpack_uint(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
      uint64_t val = 0;
   const int width = end - start + 1;
            for (uint32_t byte = start / 8; byte <= end / 8; byte++) {
                     }
      static inline uint64_t
   __gen_unpack_sint(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
      int size = end - start + 1;
               }
      static inline float
   __gen_unpack_ulod(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
                  }
      static inline float
   __gen_unpack_slod(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
                  }
      static inline uint64_t
   __gen_unpack_padded(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
      unsigned val = __gen_unpack_uint(cl, start, end);
   unsigned shift = val & 0b11111;
               }
      #define PREFIX1(A) MALI_ ## A
   #define PREFIX2(A, B) MALI_ ## A ## _ ## B
   #define PREFIX4(A, B, C, D) MALI_ ## A ## _ ## B ## _ ## C ## _ ## D
      #define pan_pack(dst, T, name)                              \\
      for (struct PREFIX1(T) name = { PREFIX2(T, header) }, \\
      *_loop_terminate = (void *) (dst);                  \\
   __builtin_expect(_loop_terminate != NULL, 1);       \\
   ({ PREFIX2(T, pack)((uint32_t *) (dst), &name);  \\
      #define pan_unpack(src, T, name)                        \\
         struct PREFIX1(T) name;                         \\
      #define pan_print(fp, T, var, indent)                   \\
            #define pan_size(T) PREFIX2(T, LENGTH)
   #define pan_alignment(T) PREFIX2(T, ALIGN)
      #define pan_section_offset(A, S) \\
            #define pan_section_ptr(base, A, S) \\
            #define pan_section_pack(dst, A, S, name)                                                         \\
      for (PREFIX4(A, SECTION, S, TYPE) name = { PREFIX4(A, SECTION, S, header) }, \\
      *_loop_terminate = (void *) (dst);                                                        \\
   __builtin_expect(_loop_terminate != NULL, 1);                                             \\
   ({ PREFIX4(A, SECTION, S, pack) (pan_section_ptr(dst, A, S), &name);              \\
      #define pan_section_unpack(src, A, S, name)                               \\
         PREFIX4(A, SECTION, S, TYPE) name;                             \\
      #define pan_section_print(fp, A, S, var, indent)                          \\
            static inline void pan_merge_helper(uint32_t *dst, const uint32_t *src, size_t bytes)
   {
                  for (unsigned i = 0; i < (bytes / 4); ++i)
   }
      #define pan_merge(packed1, packed2, type) \
            /* From presentations, 16x16 tiles externally. Use shift for fast computation
   * of tile numbers. */
      #define MALI_TILE_SHIFT 4
   #define MALI_TILE_LENGTH (1 << MALI_TILE_SHIFT)
      """
      v6_format_printer = """
      #define mali_pixel_format_print(fp, format) \\
      fprintf(fp, "%*sFormat (v6): %s%s%s %s%s%s%s\\n", indent, "", \\
      mali_format_as_str((enum mali_format)((format >> 12) & 0xFF)), \\
   (format & (1 << 20)) ? " sRGB" : "", \\
   (format & (1 << 21)) ? " big-endian" : "", \\
   mali_channel_as_str((enum mali_channel)((format >> 0) & 0x7)), \\
   mali_channel_as_str((enum mali_channel)((format >> 3) & 0x7)), \\
   mali_channel_as_str((enum mali_channel)((format >> 6) & 0x7)), \\
      """
      v7_format_printer = """
      #define mali_pixel_format_print(fp, format) \\
      fprintf(fp, "%*sFormat (v7): %s%s %s%s\\n", indent, "", \\
      mali_format_as_str((enum mali_format)((format >> 12) & 0xFF)), \\
   (format & (1 << 20)) ? " sRGB" : "", \\
   mali_rgb_component_order_as_str((enum mali_rgb_component_order)(format & ((1 << 12) - 1))), \\
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
   '%': '',
   '*': '',
   '"': '',
   '+': '',
               for i, j in substitutions.items():
                  def safe_name(name):
      name = to_alphanum(name)
   if not name[0].isalpha():
                  def prefixed_upper_name(prefix, name):
      if prefix:
               def enum_name(name):
            MODIFIERS = ["shr", "minus", "align", "log2"]
      def parse_modifier(modifier):
      if modifier is None:
            for mod in MODIFIERS:
      if modifier[0:len(mod)] == mod:
         if mod == "log2":
                  if modifier[len(mod)] == '(' and modifier[-1] == ')':
      ret = [mod, int(modifier[(len(mod) + 1):-1])]
   if ret[0] == 'align':
                     print("Invalid modifier")
         class Aggregate(object):
      def __init__(self, parser, name, attrs):
      self.parser = parser
   self.sections = []
   self.name = name
   self.explicit_size = int(attrs["size"]) if "size" in attrs else 0
   self.size = 0
         class Section:
      def __init__(self, name):
         def get_size(self):
      if self.size > 0:
            size = 0
   for section in self.sections:
            if self.explicit_size > 0:
         assert(self.explicit_size >= size)
   else:
               def add_section(self, type_name, attrs):
      assert("name" in attrs)
   section = self.Section(safe_name(attrs["name"]).lower())
   section.human_name = attrs["name"]
   section.offset = int(attrs["offset"])
   assert(section.offset % 4 == 0)
   section.type = self.parser.structs[attrs["type"]]
   section.type_name = type_name
      class Field(object):
      def __init__(self, parser, attrs):
      self.parser = parser
   if "name" in attrs:
                  if ":" in str(attrs["start"]):
         (word, bit) = attrs["start"].split(":")
   else:
            self.end = self.start + int(attrs["size"]) - 1
            if self.type == 'bool' and self.start != self.end:
            if "prefix" in attrs:
         else:
                     # Map enum values
   if self.type in self.parser.enums and self.default is not None:
                  def emit_template_struct(self, dim):
      if self.type == 'address':
         elif self.type == 'bool':
         elif self.type in ['float', 'ulod', 'slod']:
         elif self.type in ['uint', 'hex'] and self.end - self.start > 32:
         elif self.type == 'int':
         elif self.type in ['uint', 'hex', 'uint/float', 'padded', 'Pixel Format']:
         elif self.type in self.parser.structs:
         elif self.type in self.parser.enums:
         else:
                           for value in self.values:
               def overlaps(self, field):
         class Group(object):
      def __init__(self, parser, parent, start, count, label):
      self.parser = parser
   self.parent = parent
   self.start = start
   self.count = count
   self.label = label
   self.size = 0
   self.length = 0
         def get_length(self):
      # Determine number of bytes in this group.
   calculated = max(field.end // 8 for field in self.fields) + 1 if len(self.fields) > 0 else 0
   if self.length > 0:
         else:
                  def emit_template_struct(self, dim):
      if self.count == 0:
         else:
                                             class Word:
      def __init__(self):
               class FieldRef:
      def __init__(self, field, path, start, end):
         self.field = field
   self.path = path
         def collect_fields(self, fields, offset, path, all_fields):
      for field in fields:
                        if field.type in self.parser.structs:
                        start = field_offset
         def collect_words(self, fields, offset, path, words):
      for field in fields:
                        if field.type in self.parser.structs:
                        end = offset + field.end
   contributor = self.FieldRef(field, field_path, start, end)
   first_word = contributor.start // 32
   last_word = contributor.end // 32
   for b in range(first_word, last_word + 1):
               def emit_pack_function(self):
               words = {}
            # Validate the modifier is lossless
   for field in self.fields:
                        if field.modifier[0] == "shr":
      shift = field.modifier[1]
   mask = hex((1 << shift) - 1)
      elif field.modifier[0] == "minus":
                  for index in range(self.length // 4):
         # Handle MBZ words
   if not index in words:
                                                   for contributor in word.contributors:
      field = contributor.field
   name = field.name
   start = contributor.start
   end = contributor.end
                        value = "values->{}".format(contributor.path)
   if field.modifier is not None:
      if field.modifier[0] == "shr":
         elif field.modifier[0] == "minus":
         elif field.modifier[0] == "align":
                     if field.type in ["uint", "hex", "uint/float", "address", "Pixel Format"]:
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "padded":
      s = "__gen_padded(%s, %d, %d)" % \
      elif field.type in self.parser.enums:
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "int":
      s = "util_bitpack_sint(%s, %d, %d)" % \
      elif field.type == "bool":
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "float":
      assert(start == 0 and end == 31)
      elif field.type == "ulod":
      s = "util_bitpack_ufixed_clamp({}, {}, {}, 8)".format(value,
            elif field.type == "slod":
      s = "util_bitpack_sfixed_clamp({}, {}, {}, 8)".format(value,
                           if not s == None:
                           if contributor == word.contributors[-1]:
                        # Given a field (start, end) contained in word `index`, generate the 32-bit
   # mask of present bits relative to the word
   def mask_for_word(self, index, start, end):
      field_word_start = index * 32
   start -= field_word_start
   end -= field_word_start
   # Cap multiword at one word
   start = max(start, 0)
   end = min(end, 32 - 1)
   count = (end - start + 1)
         def emit_unpack_function(self):
      # First, verify there is no garbage in unused bits
   words = {}
            for index in range(self.length // 4):
         base = index * 32
   word = words.get(index, self.Word())
                           if mask != ALL_ONES:
            fieldrefs = []
   self.collect_fields(self.fields, 0, '', fieldrefs)
   for fieldref in fieldrefs:
                        args = []
   args.append('cl')
                  if field.type in set(["uint", "hex", "uint/float", "address", "Pixel Format"]):
         elif field.type in self.parser.enums:
         elif field.type == "int":
         elif field.type == "padded":
         elif field.type == "bool":
         elif field.type == "float":
         elif field.type == "ulod":
         elif field.type == "slod":
                        suffix = ""
   prefix = ""
   if field.modifier:
      if field.modifier[0] == "minus":
         elif field.modifier[0] == "shr":
                              print('   values->{} = {};'.format(fieldref.path, decoded))
   if field.modifier and field.modifier[0] == "align":
         def emit_print_function(self):
      for field in self.fields:
                        if field.type in self.parser.structs:
      pack_name = self.parser.gen_prefix(safe_name(field.type)).upper()
   print('   fprintf(fp, "%*s{}:\\n", indent, "");'.format(field.human_name))
      elif field.type == "address":
      # TODO resolve to name
      elif field.type in self.parser.enums:
         elif field.type == "int":
         elif field.type == "bool":
         elif field.type in ["float", "ulod", "slod"]:
         elif field.type in ["uint", "hex"] and (field.end - field.start) >= 32:
         elif field.type == "hex":
         elif field.type == "uint/float":
         elif field.type == "Pixel Format":
            class Value(object):
      def __init__(self, attrs):
      self.name = attrs["name"]
      class Parser(object):
      def __init__(self):
      self.parser = xml.parsers.expat.ParserCreate()
   self.parser.StartElementHandler = self.start_element
            self.struct = None
   self.structs = {}
   # Set of enum names we've seen.
   self.enums = set()
   self.aggregate = None
         def gen_prefix(self, name):
            def start_element(self, name, attrs):
      if name == "panxml":
         print(pack_header)
   if "arch" in attrs:
      arch = int(attrs["arch"])
   if arch <= 6:
            elif name == "struct":
         name = attrs["name"]
   self.no_direct_packing = attrs.get("no-direct-packing", False)
                  self.group = Group(self, None, 0, 1, name)
   if "size" in attrs:
         self.group.align = int(attrs["align"]) if "align" in attrs else None
   elif name == "field":
         self.group.fields.append(Field(self, attrs))
   elif name == "enum":
         self.values = []
   self.enum = safe_name(attrs["name"])
   self.enums.add(attrs["name"])
   if "prefix" in attrs:
         else:
   elif name == "value":
         elif name == "aggregate":
         aggregate_name = self.gen_prefix(safe_name(attrs["name"].upper()))
   self.aggregate = Aggregate(self, aggregate_name, attrs)
   elif name == "section":
               def end_element(self, name):
      if name == "struct":
         self.emit_struct()
   self.struct = None
   elif name  == "field":
         elif name  == "enum":
         self.emit_enum()
   elif name == "aggregate":
         self.emit_aggregate()
   elif name == "panxml":
         # Include at the end so it can depend on us but not the converse
         def emit_header(self, name):
      default_fields = []
   for field in self.group.fields:
         if not type(field) is Field:
         if field.default is not None:
                  print('#define %-40s\\' % (name + '_header'))
   if default_fields:
         else:
               def emit_template_struct(self, name, group):
      print("struct %s {" % name)
   group.emit_template_struct("")
         def emit_aggregate(self):
      aggregate = self.aggregate
   print("struct %s_packed {" % aggregate.name.lower())
   print("   uint32_t opaque[{}];".format(aggregate.get_size() // 4))
   print("};\n")
   print('#define {}_LENGTH {}'.format(aggregate.name.upper(), aggregate.size))
   if aggregate.align != None:
         for section in aggregate.sections:
         print('#define {}_SECTION_{}_TYPE struct {}'.format(aggregate.name.upper(), section.name.upper(), section.type_name))
   print('#define {}_SECTION_{}_header {}_header'.format(aggregate.name.upper(), section.name.upper(), section.type_name))
   print('#define {}_SECTION_{}_pack {}_pack'.format(aggregate.name.upper(), section.name.upper(), section.type_name))
   print('#define {}_SECTION_{}_unpack {}_unpack'.format(aggregate.name.upper(), section.name.upper(), section.type_name))
   print('#define {}_SECTION_{}_print {}_print'.format(aggregate.name.upper(), section.name.upper(), section.type_name))
         def emit_pack_function(self, name, group):
      print("static ALWAYS_INLINE void\n%s_pack(uint32_t * restrict cl,\n%sconst struct %s * restrict values)\n{" %
                              # Should be a whole number of words
            print('#define {} {}'.format (name + "_LENGTH", self.group.length))
   if self.group.align != None:
               def emit_unpack_function(self, name, group):
      print("static inline void")
   print("%s_unpack(const uint8_t * restrict cl,\n%sstruct %s * restrict values)\n{" %
                           def emit_print_function(self, name, group):
      print("static inline void")
                           def emit_struct(self):
               self.emit_template_struct(self.struct, self.group)
   self.emit_header(name)
   if self.no_direct_packing == False:
         self.emit_pack_function(self.struct, self.group)
         def enum_prefix(self, name):
            def emit_enum(self):
      e_name = enum_name(self.enum)
   prefix = e_name if self.enum != 'Format' else global_prefix
            for value in self.values:
         name = '{}_{}'.format(prefix, value.name)
   name = safe_name(name).upper()
            print("static inline const char *")
   print("{}_as_str(enum {} imm)\n{{".format(e_name.lower(), e_name))
   print("    switch (imm) {")
   for value in self.values:
         name = '{}_{}'.format(prefix, value.name)
   name = safe_name(name).upper()
   print('    default: return "XXX: INVALID";')
   print("    }")
         def parse(self, filename):
      file = open(filename, "rb")
   self.parser.ParseFile(file)
      if len(sys.argv) < 2:
      print("No input xml file specified")
         input_file = sys.argv[1]
      p = Parser()
   p.parse(input_file)
