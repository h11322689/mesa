   # encoding=utf-8
      # Copyright © 2022 Imagination Technologies Ltd.
      # based on anv driver gen_pack_header.py which is:
   # Copyright © 2016 Intel Corporation
      # based on v3dv driver gen_pack_header.py which is:
   # Copyright (C) 2016 Broadcom
      # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
      from __future__ import annotations
      import copy
   import os
   import textwrap
   import typing as t
   import xml.parsers.expat as expat
   from abc import ABC
   from ast import literal_eval
         MIT_LICENSE_COMMENT = """/*
   * Copyright © %(copyright)s
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */"""
      PACK_FILE_HEADER = """%(license)s
      /* Enums, structures and pack functions for %(platform)s.
   *
   * This file has been generated, do not hand edit.
   */
      #ifndef %(guard)s
   #define %(guard)s
      #include "csbgen/pvr_packet_helpers.h"
      """
         def safe_name(name: str) -> str:
      if not name[0].isalpha():
                     def num_from_str(num_str: str) -> int:
      if num_str.lower().startswith("0x"):
            if num_str.startswith("0") and len(num_str) > 1:
                     class Node(ABC):
               parent: Node
            def __init__(self, parent: Node, name: str, *, name_is_safe: bool = False) -> None:
      self.parent = parent
   if name_is_safe:
         else:
         @property
   def full_name(self) -> str:
      if self.name[0] == "_":
                  @property
   def prefix(self) -> str:
            def add(self, element: Node) -> None:
      raise RuntimeError("Element cannot be nested in %s. Element Type: %s"
         class Csbgen(Node):
               prefix_field: str
   filename: str
   _defines: t.List[Define]
   _enums: t.Dict[str, Enum]
            def __init__(self, name: str, prefix: str, filename: str) -> None:
      super().__init__(None, name.upper())
   self.prefix_field = safe_name(prefix.upper())
            self._defines = []
   self._enums = {}
         @property
   def full_name(self) -> str:
            @property
   def prefix(self) -> str:
            def add(self, element: Node) -> None:
      if isinstance(element, Enum):
                        elif isinstance(element, Struct):
                        elif isinstance(element, Define):
         define_names = [d.full_name for d in self._defines]
                  else:
         def _gen_guard(self) -> str:
            def emit(self) -> None:
      print(PACK_FILE_HEADER % {
         "license": MIT_LICENSE_COMMENT % {"copyright": "2022 Imagination Technologies Ltd."},
   "platform": self.name,
            for define in self._defines:
                     for enum in self._enums.values():
            for struct in self._structs.values():
                  def is_known_struct(self, struct_name: str) -> bool:
            def is_known_enum(self, enum_name: str) -> bool:
            def get_enum(self, enum_name: str) -> Enum:
            def get_struct(self, struct_name: str) -> Struct:
            class Enum(Node):
                        def __init__(self, parent: Node, name: str) -> None:
                              # We override prefix so that the values will contain the enum's name too.
   @property
   def prefix(self) -> str:
            def get_value(self, value_name: str) -> Value:
            def add(self, element: Node) -> None:
      if not isinstance(element, Value):
            if element.name in self._values:
            if element.value in self._values.values():
                  def _emit_to_str(self) -> None:
      print(textwrap.dedent("""\
         static const char *
            print("    switch (value) {")
   for value in self._values.values():
         print("    default: return NULL;")
                  def emit(self) -> None:
      # This check is invalid if tags other than Value can be nested within an enum.
   if not self._values.values():
            print("enum %s {" % self.full_name)
   for value in self._values.values():
                        class Value(Node):
                        def __init__(self, parent: Node, name: str, value: int) -> None:
                              def emit(self):
            class Struct(Node):
               length: int
   size: int
            def __init__(self, parent: Node, name: str, length: int) -> None:
               self.length = length
            if self.length <= 0:
                           @property
   def fields(self) -> t.List[Field]:
               fields = []
   for child in self._children.values():
         if isinstance(child, Condition):
                        @property
   def prefix(self) -> str:
            def add(self, element: Node) -> None:
      # We don't support conditions and field having the same name.
   if isinstance(element, Field):
         if element.name in self._children.keys():
                     elif isinstance(element, Condition):
         # We only save ifs, and ignore the rest. The rest will be linked to
   # the if condition so we just need to call emit() on the if and the
   # rest will also be emitted.
   if element.type == "if":
         else:
            else:
         def _emit_header(self, root: Csbgen) -> None:
      default_fields = []
   for field in (f for f in self.fields if f.default is not None):
         if field.is_builtin_type:
         else:
      if not root.is_known_enum(field.type):
      # Default values should not apply to structures
   raise RuntimeError(
                              try:
                              print("#define %-40s\\" % (self.full_name + "_header"))
   print(",  \\\n".join(default_fields))
         def _emit_helper_macros(self) -> None:
      for field in (f for f in self.fields if f.defines):
                                 def _emit_pack_function(self, root: Csbgen) -> None:
      print(textwrap.dedent("""\
         static inline __attribute__((always_inline)) void
   %s_pack(__attribute__((unused)) void * restrict dst,
            group = Group(0, 1, self.size, self.fields)
   dwords, length = group.collect_dwords_and_length()
   if length:
                                 def _emit_unpack_function(self, root: Csbgen) -> None:
      print(textwrap.dedent("""\
         static inline __attribute__((always_inline)) void
   %s_unpack(__attribute__((unused)) const void * restrict src,
            group = Group(0, 1, self.size, self.fields)
   dwords, length = group.collect_dwords_and_length()
   if length:
                                 def emit(self, root: Csbgen) -> None:
                                 print("struct %s {" % self.full_name)
   for child in self._children.values():
                  self._emit_pack_function(root)
         class Field(Node):
               start: int
   end: int
   type: str
   default: t.Optional[t.Union[str, int]]
   shift: t.Optional[int]
            def __init__(self, parent: Node, name: str, start: int, end: int, ty: str, *,
                     self.start = start
   self.end = end
                              if self.start > self.end:
                  if self.type == "bool" and self.end != self.start:
            if default is not None:
         if not self.is_builtin_type:
      # Assuming it's an enum type.
      else:
   else:
            if shift is not None:
                                 else:
                        @property
   def defines(self) -> t.Iterator[Define]:
            # We override prefix so that the defines will contain the field's name too.
   @property
   def prefix(self) -> str:
            @property
   def is_builtin_type(self) -> bool:
      builtins = {"address", "bool", "float", "mbo", "offset", "int", "uint"}
         def _get_c_type(self, root: Csbgen) -> str:
      if self.type == "address":
         elif self.type == "bool":
         elif self.type == "float":
         elif self.type == "offset":
         elif self.type == "int":
         elif self.type == "uint":
         if self.end - self.start < 32:
                        raise RuntimeError("No known C type found to hold %d bit sized value. Field: '%s'"
   elif root.is_known_struct(self.type):
         elif root.is_known_enum(self.type):
               def add(self, element: Node) -> None:
      if self.type == "mbo":
                  if isinstance(element, Define):
                        else:
         def emit(self, root: Csbgen) -> None:
      if self.type == "mbo":
                  class Define(Node):
                        def __init__(self, parent: Node, name: str, value: int) -> None:
                              def emit(self) -> None:
            class Condition(Node):
               type: str
   _children: t.Dict[str, t.Union[Condition, Field]]
            def __init__(self, parent: Node, name: str, ty: str) -> None:
               self.type = ty
   if not Condition._is_valid_type(self.type):
                     # This is the link to the next branch for the if statement so either
   # elif, else, or endif. They themselves will also have a link to the
   # next branch up until endif which terminates the chain.
                  @property
   def fields(self) -> t.List[Field]:
      # TODO: Should we use some kind of state to indicate the all of the
   # child nodes have been added and then cache the fields in here on the
   # first call so that we don't have to traverse them again per each call?
   # The state could be changed wither when we reach the endif and pop from
                     for child in self._children.values():
         if isinstance(child, Condition):
                  if self._child_branch is not None:
                  @staticmethod
   def _is_valid_type(ty: str) -> bool:
      types = {"if", "elif", "else", "endif"}
         def _is_compatible_child_branch(self, branch):
      types = ["if", "elif", "else", "endif"]
   idx = types.index(self.type)
   return (branch.type in types[idx + 1:] or
         def _add_branch(self, branch: Condition) -> None:
      if branch.type == "elif" and branch.name == self.name:
            if not self._is_compatible_child_branch(branch):
                  # Returns the name of the if condition. This is used for elif branches since
   # they have a different name than the if condition thus we have to traverse
   # the chain of branches.
   # This is used to discriminate nested if conditions from branches since
   # branches like 'endif' and 'else' will have the same name as the 'if' (the
   # elif is an exception) while nested conditions will have different names.
   #
   # TODO: Redo this to improve speed? Would caching this be helpful? We could
   # just save the name of the if instead of having to walk towards it whenever
   # a new condition is being added.
   def _top_branch_name(self) -> str:
      if self.type == "if":
            # If we're not an 'if' condition, our parent must be another condition.
   assert isinstance(self.parent, Condition)
         def add(self, element: Node) -> None:
      if isinstance(element, Field):
                        elif isinstance(element, Condition):
         if element.type == "elif" or self._top_branch_name() == element.name:
         else:
                           # This is a nested condition and we made sure that the name
   # doesn't match _top_branch_name() so we can recognize the else
   # and endif.
   # We recognized the elif by its type however its name differs
   # from the if condition thus when we add an if condition with
   # the same name as the elif nested in it, the _top_branch_name()
   # check doesn't hold true as the name matched the elif and not
   # the if statement which the elif was a branch of, thus the
   # nested if condition is not recognized as an invalid branch of
   # the outer if statement.
   #   Sample:
   #   <condition type="if" check="ROGUEXE"/>
   #       <condition type="elif" check="COMPUTE"/>
   #           <condition type="if" check="COMPUTE"/>
   #           <condition type="endif" check="COMPUTE"/>
   #       <condition type="endif" check="COMPUTE"/>
   #   <condition type="endif" check="ROGUEXE"/>
   #
   # We fix this by checking the if condition name against its
                     else:
         def emit(self, root: Csbgen) -> None:
      if self.type == "if":
         elif self.type == "elif":
         elif self.type == "else":
         elif self.type == "endif":
         print("/* endif %s */" % self.name)
   else:
            for child in self._children.values():
                  class Group:
               start: int
   count: int
   size: int
            def __init__(self, start: int, count: int, size: int, fields) -> None:
      self.start = start
   self.count = count
   self.size = size
         class DWord:
               size: int
   fields: t.List[Field]
            def __init__(self) -> None:
         self.size = 32
         def collect_dwords(self, dwords: t.Dict[int, Group.DWord], start: int) -> None:
      for field in self.fields:
         index = (start + field.start) // 32
                  clone = copy.copy(field)
   clone.start = clone.start + start
                  if field.type == "address":
                  # Coalesce all the dwords covered by this field. The two cases we
   # handle are where multiple fields are in a 64 bit word (typically
   # and address and a few bits) or where a single struct field
   # completely covers multiple dwords.
   while index < (start + field.end) // 32:
      if index + 1 in dwords and not dwords[index] == dwords[index + 1]:
      dwords[index].fields.extend(dwords[index + 1].fields)
               def collect_dwords_and_length(self) -> t.Tuple[t.Dict[int, Group.DWord], int]:
      dwords = {}
            # Determine number of dwords in this group. If we have a size, use
   # that, since that'll account for MBZ dwords at the end of a group
   # (like dword 8 on BDW+ 3DSTATE_HS). Otherwise, use the largest dword
   # index we've seen plus one.
   if self.size > 0:
         elif dwords:
         else:
                  def emit_pack_function(self, root: Csbgen, dwords: t.Dict[int, Group.DWord], length: int) -> None:
      for index in range(length):
         # Handle MBZ dwords
   if index not in dwords:
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
   if root.is_known_struct(field.type) and field.start % 32 == 0:
      print("")
                  # Pack any fields of struct type first so we have integer values
   # to the dword for those fields.
   field_index = 0
   for field in dw.fields:
      if root.is_known_struct(field.type):
      print("")
   print("    uint32_t v%d_%d;" % (index, field_index))
                  print("")
                  if dw.size == 32 and not dw.addresses:
      v = None
      elif len(dw.fields) > address_count:
      v = "v%d" % index
                     field_index = 0
   non_address_fields = []
   for field in dw.fields:
      if field.type == "mbo":
      non_address_fields.append("__pvr_mbo(%d, %d)"
      elif field.type == "address":
         elif field.type == "uint":
      non_address_fields.append("__pvr_uint(values->%s, %d, %d)"
      elif root.is_known_enum(field.type):
      non_address_fields.append("__pvr_uint(values->%s, %d, %d)"
      elif field.type == "int":
      non_address_fields.append("__pvr_sint(values->%s, %d, %d)"
      elif field.type == "bool":
      non_address_fields.append("__pvr_uint(values->%s, %d, %d)"
      elif field.type == "float":
         elif field.type == "offset":
      non_address_fields.append("__pvr_offset(values->%s, %d, %d)"
      elif field.is_struct_type():
      non_address_fields.append("__pvr_uint(v%d_%d, %d, %d)"
                  else:
                                    if dw.size == 32:
      for addr in dw.addresses:
      print("    dw[%d] = __pvr_address(values->%s, %d, %d, %d) | %s;"
                  v_accumulated_addr = ""
   for i, addr in enumerate(dw.addresses):
      v_address = "v%d_address" % i
   v_accumulated_addr += "v%d_address" % i
   print("    const uint64_t %s =" % v_address)
   print("      __pvr_address(values->%s, %d, %d, %d);"
                     if dw.addresses:
      if len(dw.fields) > address_count:
      print("    dw[%d] = %s | %s;" % (index, v_accumulated_addr, v))
   print("    dw[%d] = (%s >> 32) | (%s >> 32);" % (index + 1, v_accumulated_addr, v))
                        def emit_unpack_function(self, root: Csbgen, dwords: t.Dict[int, Group.DWord], length: int) -> None:
      for index in range(length):
         # Ignore MBZ dwords
                  # For 64 bit dwords, we aliased the two dword entries in the dword
   # dict it occupies. Now that we're emitting the unpack function,
   # skip the duplicate entries.
   dw = dwords[index]
                  # Special case: only one field and it's a struct at the beginning
   # of the dword. In this case we unpack directly from the
   # source. This is the only way we handle embedded structs
   # larger than 32 bits.
   if len(dw.fields) == 1:
      field = dw.fields[0]
   if root.is_known_struct(field.type) and field.start % 32 == 0:
      prefix = root.get_struct(field.type)
                           if dw.size == 32:
         elif dw.size == 64:
      v = "v%d" % index
                     # Unpack any fields of struct type first.
   for field_index, field in enumerate(f for f in dw.fields if root.is_known_struct(f.type)):
      prefix = root.get_struct(field.type).prefix
   vname = "v%d_%d" % (index, field_index)
   print("")
                     for field in dw.fields:
                     if field.type == "mbo" or root.is_known_struct(field.type):
         elif field.type == "uint" or root.is_known_enum(field.type) or field.type == "bool":
      print("    values->%s = __pvr_uint_unpack(%s, %d, %d);"
      elif field.type == "int":
      print("    values->%s = __pvr_sint_unpack(%s, %d, %d);"
      elif field.type == "float":
         elif field.type == "offset":
      print("    values->%s = __pvr_offset_unpack(%s, %d, %d);"
      elif field.type == "address":
      print("    values->%s = __pvr_address_unpack(%s, %d, %d, %d);"
         class Parser:
               parser: expat.XMLParserType
   context: t.List[Node]
            def __init__(self) -> None:
      self.parser = expat.ParserCreate()
   self.parser.StartElementHandler = self.start_element
            self.context = []
         def start_element(self, name: str, attrs: t.Dict[str, str]) -> None:
      if name == "csbgen":
         if self.context:
      raise RuntimeError(
                     csbgen = Csbgen(attrs["name"], attrs["prefix"], self.filename)
                     if name == "struct":
                  elif name == "field":
         default = None
                  shift = None
                  field = Field(parent, name=attrs["name"], start=int(attrs["start"]), end=int(attrs["end"]),
            elif name == "enum":
                  elif name == "value":
                  elif name == "define":
                  elif name == "condition":
                  # Starting with the if statement we push it in the context. For each
   # branch following (elif, and else) we assign the top of stack as
   # its parent, pop() and push the new condition. So per branch we end
   # up having [..., struct, condition]. We don't push an endif since
                  if condition.type != "if":
                        if condition.type == "endif":
      if not isinstance(parent, Condition):
      raise RuntimeError("Cannot close unopened or already closed condition. Condition: '%s'"
            else:
         def end_element(self, name: str) -> None:
      if name == "condition":
         element = self.context[-1]
                              if name == "struct":
         if not isinstance(element, Struct):
   elif name == "field":
         if not isinstance(element, Field):
   elif name == "enum":
         if not isinstance(element, Enum):
   elif name == "value":
         if not isinstance(element, Value):
   elif name == "define":
         if not isinstance(element, Define):
   elif name == "csbgen":
                        else:
         def parse(self, filename: str) -> None:
      file = open(filename, "rb")
   self.filename = filename
   self.parser.ParseFile(file)
         if __name__ == "__main__":
               if len(sys.argv) < 2:
      print("No input xml file specified")
                  p = Parser()
   p.parse(input_file)
