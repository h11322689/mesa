   #encoding=utf-8
      # Copyright 2016 Intel Corporation
   # Copyright 2016 Broadcom
   # Copyright 2020 Collabora, Ltd.
   # SPDX-License-Identifier: MIT
      import xml.parsers.expat
   import sys
   import operator
   import math
   from functools import reduce
      global_prefix = "agx"
      pack_header = """
   /* Generated code, see midgard.xml and gen_pack_header.py
   *
   * Packets, enums and structures for Panfrost.
   *
   * This file has been generated, do not hand edit.
   */
      #ifndef AGX_PACK_H
   #define AGX_PACK_H
      #include <stdio.h>
   #include <inttypes.h>
      #include "util/bitpack_helpers.h"
      #define __gen_unpack_float(x, y, z) uif(__gen_unpack_uint(x, y, z))
      static inline uint64_t
   __gen_unpack_uint(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
      uint64_t val = 0;
   const int width = end - start + 1;
            for (unsigned byte = start / 8; byte <= end / 8; byte++) {
                     }
      /*
   * LODs are 4:6 fixed point. We must clamp before converting to integers to
   * avoid undefined behaviour for out-of-bounds inputs like +/- infinity.
   */
   static inline uint32_t
   __gen_pack_lod(float f, uint32_t start, uint32_t end)
   {
      uint32_t fixed = CLAMP(f * (1 << 6), 0 /* 0.0 */, 0x380 /* 14.0 */);
      }
      static inline float
   __gen_unpack_lod(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
         }
      static inline uint64_t
   __gen_unpack_sint(const uint8_t *restrict cl, uint32_t start, uint32_t end)
   {
      int size = end - start + 1;
               }
      static inline uint64_t
   __gen_to_groups(uint32_t value, uint32_t group_size, uint32_t length)
   {
      /* Zero is not representable, clamp to minimum */
   if (value == 0)
            /* Round up to the nearest number of groups */
            /* The 0 encoding means "all" */
   if (groups == (1ull << length))
            /* Otherwise it's encoded as the identity */
   assert(groups < (1u << length) && "out of bounds");
   assert(groups >= 1 && "exhaustive");
      }
      static inline uint64_t
   __gen_from_groups(uint32_t value, uint32_t group_size, uint32_t length)
   {
         }
      #define agx_pack(dst, T, name)                              \\
      for (struct AGX_ ## T name = { AGX_ ## T ## _header }, \\
      *_loop_terminate = (void *) (dst);                  \\
   __builtin_expect(_loop_terminate != NULL, 1);       \\
   ({ AGX_ ## T ## _pack((uint32_t *) (dst), &name);  \\
      #define agx_unpack(fp, src, T, name)                        \\
         struct AGX_ ## T name;                         \\
      #define agx_print(fp, T, var, indent)                   \\
            static inline void agx_merge_helper(uint32_t *dst, const uint32_t *src, size_t bytes)
   {
                  for (unsigned i = 0; i < (bytes / 4); ++i)
   }
      #define agx_merge(packed1, packed2, type) \
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
   '\'': '',
               for i, j in substitutions.items():
                  def safe_name(name):
      name = to_alphanum(name)
   if not name[0].isalpha():
                  def prefixed_upper_name(prefix, name):
      if prefix:
               def enum_name(name):
            MODIFIERS = ["shr", "minus", "align", "log2", "groups"]
      def parse_modifier(modifier):
      if modifier is None:
            for mod in MODIFIERS:
      if modifier[0:len(mod)] == mod:
         if mod == "log2":
                  if modifier[len(mod)] == '(' and modifier[-1] == ')':
      ret = [mod, int(modifier[(len(mod) + 1):-1])]
   if ret[0] == 'align':
                     print("Invalid modifier")
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
         elif self.type in ['float', 'lod']:
         elif self.type in ['uint', 'hex'] and self.end - self.start > 32:
         elif self.type == 'int':
         elif self.type in ['uint', 'hex']:
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
                  for index in range(math.ceil(self.length / 4)):
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
         elif field.modifier[0] == "log2":
                           if field.type in ["uint", "hex", "address"]:
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type in self.parser.enums:
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "int":
      s = "util_bitpack_sint(%s, %d, %d)" % \
      elif field.type == "bool":
      s = "util_bitpack_uint(%s, %d, %d)" % \
      elif field.type == "float":
      assert(start == 0 and end == 31)
      elif field.type == "lod":
      assert(end - start + 1 == 10)
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
                  if field.type in set(["uint", "address", "hex"]) | self.parser.enums:
         elif field.type == "int":
         elif field.type == "bool":
         elif field.type == "float":
         elif field.type == "lod":
                        suffix = ""
   prefix = ""
   if field.modifier:
      if field.modifier[0] == "minus":
         elif field.modifier[0] == "shr":
         if field.modifier[0] == "log2":
         elif field.modifier[0] == "groups":
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
      print('   if ({}_as_str({}))'.format(enum_name(field.type), val))
   print('     fprintf(fp, "%*s{}: %s\\n", indent, "", {}_as_str({}));'.format(name, enum_name(field.type), val))
   print('    else')
      elif field.type == "int":
         elif field.type == "bool":
         elif field.type in ["float", "lod"]:
         elif field.type in ["uint", "hex"] and (field.end - field.start) >= 32:
         elif field.type == "hex":
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
         def gen_prefix(self, name):
            def start_element(self, name, attrs):
      if name == "genxml":
         elif name == "struct":
         name = attrs["name"]
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
         def end_element(self, name):
      if name == "struct":
         self.emit_struct()
   self.struct = None
   elif name  == "field":
         elif name  == "enum":
         self.emit_enum()
   elif name == "genxml":
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
         def emit_pack_function(self, name, group):
      print("static inline void\n%s_pack(uint32_t * restrict cl,\n%sconst struct %s * restrict values)\n{" %
                              print('#define {} {}'.format (name + "_LENGTH", self.group.length))
   if self.group.align != None:
               def emit_unpack_function(self, name, group):
      print("static inline void")
   print("%s_unpack(FILE *fp, const uint8_t * restrict cl,\n%sstruct %s * restrict values)\n{" %
                           def emit_print_function(self, name, group):
      print("static inline void")
                           def emit_struct(self):
               self.emit_template_struct(self.struct, self.group)
   self.emit_header(name)
   self.emit_pack_function(self.struct, self.group)
   self.emit_unpack_function(self.struct, self.group)
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
   print('    default: break;')
   print("    }")
   print("    return NULL;")
         def parse(self, filename):
      file = open(filename, "rb")
   self.parser.ParseFile(file)
      if len(sys.argv) < 2:
      print("No input xml file specified")
         input_file = sys.argv[1]
      p = Parser()
   p.parse(input_file)
