   COPYRIGHT = '''
   /*
   * Copyright 2015-2019 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
   '''
   """
   Create the (combined) register header from register JSON. Use --help for usage.
   """
      import argparse
   from collections import defaultdict
   import itertools
   import json
   import re
   import sys
      from regdb import Object, RegisterDatabase, deduplicate_enums, deduplicate_register_types
         ######### BEGIN HARDCODED CONFIGURATION
      # Chips are sorted chronologically
   CHIPS = [
      Object(name='gfx6', disambiguation='GFX6'),
   Object(name='gfx7', disambiguation='GFX7'),
   Object(name='gfx8', disambiguation='GFX8'),
   Object(name='gfx81', disambiguation='GFX81'),
   Object(name='gfx9', disambiguation='GFX9'),
   Object(name='gfx940', disambiguation='GFX940'),
   Object(name='gfx10', disambiguation='GFX10'),
   Object(name='gfx103', disambiguation='GFX103'),
   Object(name='gfx11', disambiguation='GFX11'),
      ]
      ######### END HARDCODED CONFIGURATION
      def get_chip_index(chip):
      """
   Given a chip name, return its index in the global CHIPS list.
   """
         def get_disambiguation_suffix(chips):
      """
   Disambiguation suffix to be used for an enum entry or field name that
   is supported in the given set of chips.
   """
   oldest_chip_index = min([get_chip_index(chip) for chip in chips])
         def get_chips_comment(chips, parent=None):
      """
                     parent is an optional set of chips supporting a parent structure, e.g.
   where chips may be the set of chips supporting a specific enum value,
   parent would be the set of chips supporting the field containing the enum,
   the idea being that no comment is necessary if all chips that support the
   parent also support the child.
   """
   chipflags = [chip.name in chips for chip in CHIPS]
   if all(chipflags):
            if parent is not None:
      parentflags = [chip.name in parent for chip in CHIPS]
   if all(childflag or not parentflag for childflag, parentflag in zip(chipflags, parentflags)):
         prefix = 0
   for idx, chip, flag in zip(itertools.count(), CHIPS, chipflags):
      if not flag:
               suffix = len(CHIPS)
   for idx, chip, flag in zip(itertools.count(), reversed(CHIPS), reversed(chipflags)):
      if not flag:
               comment = []
   if prefix > 0:
         for chip, flag in zip(CHIPS[prefix:suffix], chipflags[prefix:suffix]):
      if flag:
      if suffix < len(CHIPS):
                  def detect_conflict(regdb, field_in_type1, field_in_type2):
      """
   Returns False if field_in_type1 and field_in_type2 can be merged
   into a single field = if writing to field_in_type1 bits won't
   overwrite adjacent fields in type2, and the other way around.
   """
   for idx, type_refs in enumerate([field_in_type1.type_refs, field_in_type2.type_refs]):
      ref = field_in_type2 if idx == 0 else field_in_type1
   for type_ref in type_refs:
         for field in regdb.register_type(type_ref).fields:
      # If a different field in the other type starts in
   # the tested field's bits[0, 1] interval
                  class HeaderWriter(object):
      def __init__(self, regdb, guard=None):
               # The following contain: Object(address, chips, name, regmap/field/enumentry)
   self.register_lines = []
   self.field_lines = []
            regtype_emit = defaultdict(set)
            for regmap in regdb.register_mappings():
         type_ref = getattr(regmap, 'type_ref', None)
   self.register_lines.append(Object(
      address=regmap.map.at,
   chips=set(regmap.chips),
   name=regmap.name,
                     basename = re.sub(r'[0-9]+', '', regmap.name)
   key = '{type_ref}::{basename}'.format(**locals())
                     regtype = regdb.register_type(type_ref)
                           enum_ref = getattr(field, 'enum_ref', None)
   self.field_lines.append(Object(
         address=regmap.map.at,
   chips=set(regmap.chips),
   name=field.name,
   field=field,
                                                   enum = regdb.enum(enum_ref)
   for entry in enum.entries:
      self.value_lines.append(Object(
      address=regmap.map.at,
            # Merge register lines
   lines = self.register_lines
            self.register_lines = []
   for line in lines:
         prev = self.register_lines[-1] if self.register_lines else None
   if prev and prev.address == line.address and prev.name == line.name:
      prev.chips.update(line.chips)
               # Merge field lines
   lines = self.field_lines
            self.field_lines = []
   for line in lines:
         merged = False
   for prev in reversed(self.field_lines):
                     # Can merge fields if they have the same starting bit and the
   # range of the field as intended by the current line does not
                        if prev.bits[1] != line.bits[1]:
      # Current line's field extends beyond the range of prev.
                     prev.bits[1] = max(prev.bits[1], line.bits[1])
   prev.chips.update(line.chips)
   prev.type_refs.update(line.type_refs)
   prev.enum_refs.update(line.enum_refs)
   merged = True
               # Merge value lines
   lines = self.value_lines
            self.value_lines = []
   for line in lines:
         for prev in reversed(self.value_lines):
      if prev.address == line.address and prev.name == line.name and\
      prev.enumentry.value == line.enumentry.value:
   prev.chips.update(line.chips)
   prev.enum_refs.update(line.enum_refs)
            # Disambiguate field and value lines
   for idx, line in enumerate(self.field_lines):
         prev = self.field_lines[idx - 1] if idx > 0 else None
   next = self.field_lines[idx + 1] if idx + 1 < len(self.field_lines) else None
   if (prev and prev.address == line.address and prev.field.name == line.field.name) or\
            for idx, line in enumerate(self.value_lines):
         prev = self.value_lines[idx - 1] if idx > 0 else None
   next = self.value_lines[idx + 1] if idx + 1 < len(self.value_lines) else None
   if (prev and prev.address == line.address and prev.enumentry.name == line.enumentry.name) or\
         def print(self, filp, sort='address'):
      """
   Print out the entire register header.
   """
   if sort == 'address':
         else:
                  # Collect and sort field lines by address
   field_lines_by_address = defaultdict(list)
   for line in self.field_lines:
         for field_lines in field_lines_by_address.values():
         if sort == 'address':
                  # Collect and sort value lines by address
   value_lines_by_address = defaultdict(list)
   for line in self.value_lines:
         for value_lines in value_lines_by_address.values():
         if sort == 'address':
                  print('/* Automatically generated by amd/registers/makeregheader.py */\n', file=filp)
   print(file=filp)
   print(COPYRIGHT.strip(), file=filp)
            if self.guard:
                  for register_line in self.register_lines:
                                 define_name = 'R_{address}_{register_line.name}'.format(**locals()).ljust(63)
                  field_lines = field_lines_by_address[register_line.address]
   field_idx = 0
                     if field_line.type_refs.isdisjoint(register_line.type_refs):
                                 mask = (1 << (field_line.bits[1] - field_line.bits[0] + 1)) - 1
   define_name = '_{address}_{field_line.name}(x)'.format(**locals()).ljust(58)
   comment = ' /* {0} */'.format(comment) if comment else ''
   print(
      '#define   S{define_name} (((unsigned)(x) & 0x{mask:X}) << {field_line.bits[0]}){comment}'
                     complement = ((1 << 32) - 1) ^ (mask << field_line.bits[0])
                        value_lines = value_lines_by_address[register_line.address]
                           if value_line.enum_refs.isdisjoint(field_line.enum_refs):
                                 define_name = 'V_{address}_{value_line.name}'.format(**locals()).ljust(55)
            if self.guard:
         def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--chip', dest='chips', type=str, nargs='*',
         parser.add_argument('--sort', choices=['name', 'address'], default='address',
         parser.add_argument('--guard', type=str, help='Name of the #include guard')
   parser.add_argument('files', metavar='FILE', type=str, nargs='+',
                  regdb = None
   for filename in args.files:
      with open(filename, 'r') as filp:
         db = RegisterDatabase.from_json(json.load(filp))
   if regdb is None:
               deduplicate_enums(regdb)
            w = HeaderWriter(regdb, guard=args.guard)
            if __name__ == '__main__':
            # kate: space-indent on; indent-width 4; replace-tabs on;
