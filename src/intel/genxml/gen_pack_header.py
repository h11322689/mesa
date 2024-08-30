   #encoding=utf-8
      import argparse
   import ast
   import intel_genxml
   import re
   import sys
   import copy
   import textwrap
   from util import *
      license =  """/*
   * Copyright (C) 2016 Intel Corporation
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
   """
      pack_header = """%(license)s
      /* Instructions, enums and structures for %(platform)s.
   *
   * This file has been generated, do not hand edit.
   */
      #ifndef %(guard)s
   #define %(guard)s
      #include <stdio.h>
      #include "util/bitpack_helpers.h"
      #ifndef __gen_validate_value
   #define __gen_validate_value(x)
   #endif
      #ifndef __intel_field_functions
   #define __intel_field_functions
      #ifdef NDEBUG
   #define NDEBUG_UNUSED __attribute__((unused))
   #else
   #define NDEBUG_UNUSED
   #endif
      static inline __attribute__((always_inline)) uint64_t
   __gen_offset(uint64_t v, NDEBUG_UNUSED uint32_t start, NDEBUG_UNUSED uint32_t end)
   {
         #ifndef NDEBUG
                  #endif
            }
      static inline __attribute__((always_inline)) uint64_t
   __gen_offset_nonzero(uint64_t v, uint32_t start, uint32_t end)
   {
      assert(v != 0ull);
      }
      static inline __attribute__((always_inline)) uint64_t
   __gen_address(__gen_user_data *data, void *location,
               {
      uint64_t addr_u64 = __gen_combine_address(data, location, address, delta);
   if (end == 31) {
         } else if (end < 63) {
      const unsigned shift = 63 - end;
      } else {
            }
      #ifndef __gen_address_type
   #error #define __gen_address_type before including this file
   #endif
      #ifndef __gen_user_data
   #error #define __gen_combine_address before including this file
   #endif
      #undef NDEBUG_UNUSED
      #endif
      """
      def num_from_str(num_str):
      if num_str.lower().startswith('0x'):
            assert not num_str.startswith('0'), 'octals numbers not allowed'
         def bool_from_str(bool_str):
      options = { "true": True, "false": False }
         class Field(object):
      ufixed_pattern = re.compile(r"u(\d+)\.(\d+)")
            def __init__(self, parser, attrs):
      self.parser = parser
   if "name" in attrs:
         self.start = int(attrs["start"])
   self.end = int(attrs["end"])
   self.type = attrs["type"]
   self.nonzero = bool_from_str(attrs.get("nonzero", "false"))
            assert self.start <= self.end, \
               if self.type == 'bool':
                  if "default" in attrs:
         # Base 0 recognizes 0x, 0o, 0b prefixes in addition to decimal ints.
   else:
            ufixed_match = Field.ufixed_pattern.match(self.type)
   if ufixed_match:
                  sfixed_match = Field.sfixed_pattern.match(self.type)
   if sfixed_match:
               def is_builtin_type(self):
      builtins =  [ 'address', 'bool', 'float', 'ufixed',
                     def is_struct_type(self):
            def is_enum_type(self):
            def emit_template_struct(self, dim):
      if self.type == 'address':
         elif self.type == 'bool':
         elif self.type == 'float':
         elif self.type == 'ufixed':
         elif self.type == 'sfixed':
         elif self.type == 'uint' and self.end - self.start > 32:
         elif self.type == 'offset':
         elif self.type == 'int':
         elif self.type == 'uint':
         elif self.is_struct_type():
         elif self.is_enum_type():
         elif self.type == 'mbo' or self.type == 'mbz':
         else:
                                    for value in self.values:
         name = value.name
               class Group(object):
      def __init__(self, parser, parent, start, count, size):
      self.parser = parser
   self.parent = parent
   self.start = start
   self.count = count
   self.size = size
         def emit_template_struct(self, dim):
      if self.count == 0:
         else:
                              class DWord:
      def __init__(self):
         self.size = 32
         def collect_dwords(self, dwords, start, dim):
      for field in self.fields:
         if isinstance(field, Group):
      if field.count == 1:
         else:
      for i in range(field.count):
                        index = (start + field.start) // 32
                  clone = copy.copy(field)
   clone.start = clone.start + start
   clone.end = clone.end + start
                  if field.type == "address":
                  # Coalesce all the dwords covered by this field. The two cases we
   # handle are where multiple fields are in a 64 bit word (typically
   # and address and a few bits) or where a single struct field
   # completely covers multiple dwords.
   while index < (start + field.end) // 32:
      if index + 1 in dwords and not dwords[index] == dwords[index + 1]:
                  def collect_dwords_and_length(self):
      dwords = {}
            # Determine number of dwords in this group. If we have a size, use
   # that, since that'll account for MBZ dwords at the end of a group
   # (like dword 8 on BDW+ 3DSTATE_HS). Otherwise, use the largest dword
   # index we've seen plus one.
   if self.size > 0:
         elif dwords:
         else:
                  def emit_pack_function(self, dwords, length):
      for index in range(length):
         # Handle MBZ dwords
   if not index in dwords:
                        # For 64 bit dwords, we aliased the two dword entries in the dword
   # dict it occupies. Now that we're emitting the pack function,
   # skip the duplicate entries.
   dw = dwords[index]
                  # Special case: only one field and it's a struct at the beginning
   # of the dword. In this case we pack directly into the
   # destination. This is the only way we handle embedded structs
   # larger than 32 bits.
   if len(dw.fields) == 1:
      field = dw.fields[0]
   name = field.name + field.dim
   if field.is_struct_type() and field.start % 32 == 0:
      print("")
                  # Pack any fields of struct type first so we have integer values
   # to the dword for those fields.
   field_index = 0
   for field in dw.fields:
      if isinstance(field, Field) and field.is_struct_type():
      name = field.name + field.dim
   print("")
   print("   uint32_t v%d_%d;" % (index, field_index))
                  print("")
   dword_start = index * 32
   if dw.address == None:
                        # Assert in dont_use values
   for field in dw.fields:
      for value in field.values:
                     if dw.size == 32 and dw.address == None:
      v = None
      elif len(dw.fields) > address_count:
      v = "v%d" % index
                     field_index = 0
   non_address_fields = []
   for field in dw.fields:
                              if field.type == "mbo":
      non_address_fields.append("util_bitpack_ones(%d, %d)" % \
      elif field.type == "mbz":
         elif field.type == "address":
         elif field.type == "uint":
      non_address_fields.append("util_bitpack_uint%s(values->%s, %d, %d)" % \
      elif field.is_enum_type():
      non_address_fields.append("util_bitpack_uint%s(values->%s, %d, %d)" % \
      elif field.type == "int":
      non_address_fields.append("util_bitpack_sint%s(values->%s, %d, %d)" % \
      elif field.type == "bool":
      non_address_fields.append("util_bitpack_uint%s(values->%s, %d, %d)" % \
      elif field.type == "float":
         elif field.type == "offset":
      non_address_fields.append("__gen_offset%s(values->%s, %d, %d)" % \
      elif field.type == 'ufixed':
      non_address_fields.append("util_bitpack_ufixed%s(values->%s, %d, %d, %d)" % \
      elif field.type == 'sfixed':
      non_address_fields.append("util_bitpack_sfixed%s(values->%s, %d, %d, %d)" % \
      elif field.is_struct_type():
      non_address_fields.append("util_bitpack_uint(v%d_%d, %d, %d)" % \
                                             if dw.size == 32:
      if dw.address:
      print("   dw[%d] = __gen_address(data, &dw[%d], values->%s, %s, %d, %d);" %
                  if dw.address:
      v_address = "v%d_address" % index
   print("   const uint64_t %s =\n      __gen_address(data, &dw[%d], values->%s, %s, %d, %d);" %
         (v_address, index, dw.address.name + field.dim, v,
   if len(dw.fields) > address_count:
      print("   dw[%d] = %s;" % (index, v_address))
   print("   dw[%d] = (%s >> 32) | (%s >> 32);" % (index + 1, v_address, v))
      else:
         class Value(object):
      def __init__(self, attrs):
      self.name = safe_name(attrs["name"])
   self.value = ast.literal_eval(attrs["value"])
      class Parser(object):
      def __init__(self):
      self.instruction = None
   self.structs = {}
   # Set of enum names we've seen.
   self.enums = set()
         def gen_prefix(self, name):
      if name[0] == "_":
               def gen_guard(self):
            def process_item(self, item):
      name = item.tag
   assert name != "genxml"
            if name in ("instruction", "struct", "register"):
         if name == "instruction":
      self.instruction = safe_name(attrs["name"])
      elif name == "struct":
      self.struct = safe_name(attrs["name"])
      elif name == "register":
      self.register = safe_name(attrs["name"])
   self.reg_num = num_from_str(attrs["num"])
      if "length" in attrs:
      self.length = int(attrs["length"])
      else:
                  elif name == "group":
         group = Group(self, self.group,
         self.group.fields.append(group)
   elif name == "field":
         self.group.fields.append(Field(self, attrs))
   elif name == "enum":
         self.values = []
   self.enum = safe_name(attrs["name"])
   self.enums.add(attrs["name"])
   if "prefix" in attrs:
         else:
   elif name == "value":
         elif name in ("import", "exclude"):
         else:
            for child_item in item:
            if name  == "instruction":
         self.emit_instruction()
   self.instruction = None
   elif name == "struct":
         self.emit_struct()
   self.struct = None
   elif name == "register":
         self.emit_register()
   self.register = None
   self.reg_num = None
   elif name == "group":
         elif name  == "field":
         elif name  == "enum":
         self.emit_enum()
   elif name in ("import", "exclude", "value"):
         else:
         def emit_template_struct(self, name, group):
      print("struct %s {" % self.gen_prefix(name))
   group.emit_template_struct("")
         def emit_pack_function(self, name, group):
      name = self.gen_prefix(name)
   print(textwrap.dedent("""\
         static inline __attribute__((always_inline)) void
   %s_pack(__attribute__((unused)) __gen_user_data *data,
                  (dwords, length) = group.collect_dwords_and_length()
   if length:
                                 def emit_instruction(self):
               if not self.length is None:
         print('#define %-33s %6d' %
   print('#define %-33s %6d' %
            default_fields = []
   for field in self.group.fields:
         if not isinstance(field, Field):
                        if field.is_builtin_type():
         else:
                  if default_fields:
         print('#define %-40s\\' % (self.gen_prefix(name + '_header')))
                           def emit_register(self):
      name = self.register
   if not self.reg_num is None:
                  if not self.length is None:
                  self.emit_template_struct(self.register, self.group)
         def emit_struct(self):
      name = self.struct
   if not self.length is None:
                  self.emit_template_struct(self.struct, self.group)
         def emit_enum(self):
      print('enum %s {' % self.gen_prefix(self.enum))
   for value in self.values:
         if self.prefix:
         else:
               def emit_genxml(self, genxml):
      root = genxml.et.getroot()
   self.platform = root.attrib["name"]
   self.gen = root.attrib["gen"].replace('.', '')
   print(pack_header % {'license': license, 'platform': self.platform, 'guard': self.gen_guard()})
   for item in root:
            def parse_args():
      p = argparse.ArgumentParser()
   p.add_argument('xml_source', metavar='XML_SOURCE',
         p.add_argument('--engines', nargs='?', type=str, default='render',
                     if pargs.engines is None:
      print("No engines specified")
               def main():
               engines = set(pargs.engines.split(','))
   valid_engines = [ 'render', 'blitter', 'video' ]
   if engines - set(valid_engines):
      print("Invalid engine specified, valid engines are:\n")
   for e in valid_engines:
               genxml = intel_genxml.GenXml(pargs.xml_source)
   genxml.filter_engines(engines)
   genxml.merge_imported()
   p = Parser()
         if __name__ == '__main__':
      main()
