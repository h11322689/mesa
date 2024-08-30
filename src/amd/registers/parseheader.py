   #
   # Copyright 2017-2019 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
   """
   Helper script that parses a register header and produces a register database
   as output. Use as:
      python3 parseheader.py ADDRESS_SPACE < header.h
      This script is included for reference -- we should be able to remove this in
   the future.
   """
      import json
   import math
   import re
   import sys
      from regdb import Object, RegisterDatabase, deduplicate_enums, deduplicate_register_types
         RE_comment = re.compile(r'(/\*(.*)\*/)$|(//(.*))$')
   RE_prefix = re.compile(r'([RSV])_([0-9a-fA-F]+)_')
   RE_set_value = re.compile(r'\(\(\(unsigned\)\(x\) & ([0-9a-fA-Fx]+)\) << ([0-9]+)\)')
   RE_set_value_no_shift = re.compile(r'\((\(unsigned\))?\(x\) & ([0-9a-fA-Fx]+)\)')
      class HeaderParser(object):
      def __init__(self, address_space):
      self.regdb = RegisterDatabase()
   self.chips = ['gfx6', 'gfx7', 'gfx8', 'fiji', 'stoney', 'gfx9']
         def __fini_field(self):
      if self.__field is None:
            if self.__enumentries:
         self.__field.enum_ref = self.__regmap.name + '__' + self.__field.name
   self.regdb.add_enum(self.__field.enum_ref, Object(
                  self.__enumentries = None
         def __fini_register(self):
      if self.__regmap is None:
            if self.__fields:
         self.regdb.add_register_type(self.__regmap.name, Object(
         ))
            self.__regmap = None
         def parse_header(self, filp):
      regdb = RegisterDatabase()
            self.__regmap = None
   self.__fields = None
   self.__field = None
            for line in filp:
                                 comment = None
   m = RE_comment.search(line)
   if m is not None:
                                       m = RE_prefix.match(name)
                  prefix = m.group(1)
                                                         entry = Object(name=name, value=value)
   if comment is not None:
                                                                                 m = RE_set_value.match(split[1])
   if m is not None:
      unshifted_mask = int(m.group(1), 0)
      else:
      m = RE_set_value_no_shift.match(split[1])
   if m is not None:
                                                self.__field = Object(
      name=name,
      )
                     if prefix == 'R':
                                                         self.__regmap = Object(
      name=name,
   chips=self.chips,
            self.__fini_field()
      def main():
               parser = HeaderParser(map_to)
            deduplicate_enums(parser.regdb)
                     if __name__ == '__main__':
            # kate: space-indent on; indent-width 4; replace-tabs on;
