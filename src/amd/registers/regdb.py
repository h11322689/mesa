   #
   # Copyright 2017-2019 Advanced Micro Devices, Inc.
   #
   # SPDX-License-Identifier: MIT
   #
   """
   Python package containing common tools for manipulating register JSON.
   """
      import itertools
   import json
   import re
   import sys
      from collections import defaultdict
   from contextlib import contextmanager
      class UnionFind(object):
      """
   Simplistic implementation of a union-find data structure that also keeps
            - add: add an element to the implied global set of elements
   - union: unify the sets containing the two given elements
   - find: return the representative element of the set containing the
         - get_set: get the set containing the given element
   - sets: iterate over all sets (the sets form a partition of the set of all
         """
   def __init__(self):
            def add(self, k):
      if k not in self.d:
         def union(self, k1, k2):
      k1 = self.find(k1)
   k2 = self.find(k2)
   if k1 == k2:
         if len(k1) < len(k2):
         self.d[k1].update(self.d[k2])
         def find(self, k):
      e = self.d[k]
   if isinstance(e, set):
         assert isinstance(e, tuple)
   r = self.find(e[0])
   self.d[k] = (r,)
         def get_set(self, k):
      k = self.find(k)
   assert isinstance(self.d[k], set)
         def sets(self):
      for v in self.d.values():
               class Object(object):
      """
   Convenience helper class that essentially acts as a dictionary for convenient
   conversion from and to JSON while allowing the use of .field notation
   instead of subscript notation for member access.
   """
   def __init__(self, **kwargs):
      for k, v in kwargs.items():
         def update(self, **kwargs):
      for key, value in kwargs.items():
               def __str__(self):
      return 'Object(' + ', '.join(
               @staticmethod
   def from_json(json, keys=None):
      if isinstance(json, list):
         elif isinstance(json, dict):
         obj = Object()
   for k, v in json.items():
      if keys is not None and k in keys:
         else:
            else:
         @staticmethod
   def to_json(obj):
      if isinstance(obj, Object):
         elif isinstance(obj, dict):
         elif isinstance(obj, list):
         else:
      class MergeError(Exception):
      def __init__(self, msg):
         class RegisterDatabaseError(Exception):
      def __init__(self, msg):
         @contextmanager
   def merge_scope(name):
      """
   Wrap a merge handling function in a "scope" whose name will be added when
   propagating MergeErrors.
   """
   try:
         except Exception as e:
         def merge_dicts(dicts, keys=None, values=None):
      """
            dicts -- list of (origin, dictionary) pairs to merge
   keys -- optional dictionary to provide a merge-strategy per key;
               value -- optional function which provides a merge-strategy for values;
                  The default strategy is to allow merging keys if all origin dictionaries
   that contain the key have the same value for it.
   """
   ks = set()
   for _, d in dicts:
            result = {}
   for k in ks:
      vs = [(o, d[k]) for o, d in dicts if k in d]
   with merge_scope('Key {k}'.format(**locals())):
         if keys is not None and k in keys:
         elif values is not None:
         else:
      base_origin, base = vs[0]
   for other_origin, other in vs[1:]:
               def merge_objects(objects, keys=None):
      """
   Like merge_dicts, but applied to instances of Object.
   """
         class RegisterDatabase(object):
      """
            - enums: these are lists of named values that can occur in a register field
   - register types: description of a register type or template as a list of
         - register mappings: named and typed registers mapped at locations in an
         """
   def __init__(self):
      self.__enums = {}
   self.__register_types = {}
   self.__register_mappings = []
   self.__regmap_by_addr = None
         def __post_init(self):
      """
   Perform some basic canonicalization:
   - enum entries are sorted by value
   - register type fields are sorted by starting bit
   - __register_mappings is sorted by offset
            Lazily computes the set of all chips mentioned by register mappings.
   """
   if self.__regmap_by_addr is not None:
            for enum in self.__enums.values():
            for regtype in self.__register_types.values():
            self.__regmap_by_addr = defaultdict(list)
            # Merge register mappings using sort order and garbage collect enums
   # and register types.
   old_register_mappings = self.__register_mappings
            self.__register_mappings = []
   for regmap in old_register_mappings:
         addr = (regmap.map.to, regmap.map.at)
                           merged = False
   for other in reversed(self.__register_mappings):
                                          if addr == other_addr and\
      (type_ref is None or other_type_ref is None or type_ref == other_type_ref):
   other.chips = sorted(list(chips.union(other_chips)))
   if type_ref is not None:
                                          for other in addrmappings:
      other_type_ref = getattr(other, 'type_ref', None)
   other_chips = getattr(other, 'chips', ['undef'])
   if type_ref is not None and other_type_ref is not None and \
      type_ref != other_type_ref and chips.intersection(other_chips):
                        def garbage_collect(self):
      """
   Remove unreferenced enums and register types.
   """
   old_enums = self.__enums
            self.__enums = {}
   self.__register_types = {}
   for regmap in self.__register_mappings:
         if hasattr(regmap, 'type_ref') and regmap.type_ref not in self.__register_types:
      regtype = old_register_types[regmap.type_ref]
   self.__register_types[regmap.type_ref] = regtype
            def __validate_register_type(self, regtype):
      for field in regtype.fields:
         if hasattr(field, 'enum_ref') and field.enum_ref not in self.__enums:
               def __validate_register_mapping(self, regmap):
      if hasattr(regmap, 'type_ref') and regmap.type_ref not in self.__register_types:
         raise RegisterDatabaseError(
         def __validate(self):
      for regtype in self.__register_types.values():
         for regmap in self.__register_mappings:
         @staticmethod
   def enum_key(enum):
      """
   Return a key that uniquely describes the signature of the given
   enum (assuming that it has been canonicalized). Two enums with the
   same key can be merged.
   """
   return ''.join(
         ':{0}:{1}'.format(entry.name, entry.value)
         def add_enum(self, name, enum):
      if name in self.__enums:
               @staticmethod
   def __merge_enums(enums, union=False):
      def merge_entries(entries_lists):
         values = defaultdict(list)
   for origin, enum in entries_lists:
                  if not union:
                        return [
                  return merge_objects(
         enums,
   keys={
               def merge_enums(self, names, newname, union=False):
      """
   Given a list of enum names, merge them all into one with a new name and
   update all references.
   """
   if newname not in names and newname in self.__enums:
            newenum = self.__merge_enums(
         [(name, self.__enums[name]) for name in names],
            for name in names:
                  for regtype in self.__register_types.values():
         for field in regtype.fields:
                  def add_register_type(self, name, regtype):
      if regtype in self.__register_types:
         self.__register_types[name] = regtype
         def register_type(self, name):
      self.__post_init()
         @staticmethod
   def __merge_register_types(regtypes, union=False, field_keys={}):
      def merge_fields(fields_lists):
         fields = defaultdict(list)
   for origin, fields_list in fields_lists:
                  if not union:
                        return [
                  with merge_scope('Register types {0}'.format(', '.join(name for name, _ in regtypes))):
         return merge_objects(
      regtypes,
   keys={
            def merge_register_types(self, names, newname, union=False):
      """
   Given a list of register type names, merge them all into one with a
   new name and update all references.
   """
   if newname not in names and newname in self.__register_types:
            newregtype = self.__merge_register_types(
         [(name, self.__register_types[name]) for name in names],
            for name in names:
                  for regmap in self.__register_mappings:
                        def add_register_mapping(self, regmap):
      self.__regmap_by_addr = None
   self.__register_mappings.append(regmap)
         def remove_register_mappings(self, regmaps_to_remove):
                        regmaps = self.__register_mappings
   self.__register_mappings = []
   for regmap in regmaps:
                        def enum(self, name):
      """
   Return the enum of the given name, if any.
   """
   self.__post_init()
         def enums(self):
      """
   Yields all (name, enum) pairs.
   """
   self.__post_init()
   for name, enum in self.__enums.items():
         def fields(self):
      """
   Yields all (register_type, fields) pairs.
   """
   self.__post_init()
   for regtype in self.__register_types.values():
               def register_types(self):
      """
   Yields all (name, register_type) pairs.
   """
   self.__post_init()
   for name, regtype in self.__register_types.items():
         def register_mappings_by_name(self, name):
      """
   Return a list of register mappings with the given name.
   """
            begin = 0
   end = len(self.__register_mappings)
   while begin < end:
         middle = (begin + end) // 2
   if self.__register_mappings[middle].name < name:
         elif name < self.__register_mappings[middle].name:
                  if begin >= end:
            # We now have begin <= mid < end with begin.name <= name, mid.name == name, name < end.name
   # Narrow down begin and end
   hi = middle
   while begin < hi:
         mid = (begin + hi) // 2
   if self.__register_mappings[mid].name < name:
                  lo = middle + 1
   while lo < end:
         mid = (lo + end) // 2
   if self.__register_mappings[mid].name == name:
                        def register_mappings(self):
      """
   Yields all register mappings.
   """
   self.__post_init()
   for regmap in self.__register_mappings:
         def chips(self):
      """
   Yields all chips.
   """
   self.__post_init()
         def merge_chips(self, chips, newchip):
      """
   Merge register mappings of the given chips into a single chip of the
   given name. Recursively merges register types and enums when appropriate.
   """
                     regtypes_merge = UnionFind()
            # Walk register mappings to find register types that should be merged.
   for idx, regmap in itertools.islice(enumerate(self.__register_mappings), 1, None):
         if not hasattr(regmap, 'type_ref'):
                        for other in self.__register_mappings[idx-1::-1]:
      if regmap.name != other.name:
         if chips.isdisjoint(other.chips):
         if regmap.map.to != other.map.to or regmap.map.at != other.map.at:
      raise RegisterDatabaseError(
                     if regmap.type_ref != other.type_ref:
               # Walk over regtype sets that are to be merged and find enums that
   # should be merged.
   for type_refs in regtypes_merge.sets():
         fields_merge = defaultdict(set)
   for type_ref in type_refs:
      regtype = self.__register_types[type_ref]
                     for enum_refs in fields_merge.values():
      if len(enum_refs) > 1:
      enum_refs = list(enum_refs)
   enums_merge.add(enum_refs[0])
            # Merge all mergeable enum sets
   remap_enum_refs = {}
   for enum_refs in enums_merge.sets():
         enum_refs = sorted(enum_refs)
   newname = enum_refs[0] + '_' + newchip
   i = 0
   while newname in self.__enums:
                                 # Don't use self.merge_enums, because we don't want to automatically
   # update _all_ references to the merged enums (some may be from
   # register types that aren't going to be merged).
   self.add_enum(newname, self.__merge_enums(
                  # Merge all mergeable type refs
   remap_type_refs = {}
   for type_refs in regtypes_merge.sets():
         type_refs = sorted(type_refs)
   newname = type_refs[0] + '_' + newchip
   i = 0
   while newname in self.__enums:
                  updated_regtypes = []
                     regtype = Object.from_json(Object.to_json(self.__register_types[type_ref]))
                              def merge_enum_refs(enum_refs):
      enum_refs = set(
      remap_enum_refs.get(enum_ref, enum_ref)
                        self.add_register_type(newname, self.__merge_register_types(
      [(type_ref, self.__register_types[type_ref]) for type_ref in type_refs],
   field_keys={
                     # Merge register mappings
   register_mappings = self.__register_mappings
            regmap_accum = None
   for regmap in register_mappings:
         if regmap_accum and regmap.name != regmap_accum.name:
                        joining_chips = chips.intersection(regmap.chips)
   if not joining_chips:
                        type_ref = getattr(regmap, 'type_ref', None)
   if type_ref is None:
                        type_ref = remap_type_refs.get(type_ref, type_ref)
   if remaining_chips:
      regmap.chips = sorted(remaining_chips)
   self.__register_mappings.append(regmap)
   if not regmap_accum:
                     if not regmap_accum:
         else:
      if not hasattr(regmap_accum.type_ref, 'type_ref'):
      if type_ref is not None:
         if regmap_accum:
         def update(self, other):
      """
            Doesn't de-duplicate entries.
   """
   self.__post_init()
            enum_remap = {}
            for regmap in other.__register_mappings:
                  type_ref = getattr(regmap, 'type_ref', None)
                                    for field in regtype.fields:
                                 remapped = enum_ref + suffix if enum_ref in self.__enums else enum_ref
   i = 0
   while remapped in self.__enums:
                                    remapped = type_ref + suffix if type_ref in self.__register_types else type_ref
   i = 0
   while remapped in self.__register_types:
      remapped = type_ref + suffix + str(i)
                                 def to_json(self):
      self.__post_init()
   return {
         'enums': Object.to_json(self.__enums),
   'register_types': Object.to_json(self.__register_types),
         def encode_json_pretty(self):
      """
   Use a custom JSON encoder which pretty prints, but keeps inner structures compact
   """
   # Since the JSON module isn't very extensible, this ends up being
   # really hacky.
            replacements = []
   def placeholder(s):
         placeholder = "JSON-{key}-NOSJ".format(key=len(replacements))
            # Pre-create non-indented encodings for inner objects
   for enum in obj['enums'].values():
         enum['entries'] = [
                  for regtype in obj['register_types'].values():
         regtype['fields'] = [
                  for regmap in obj['register_mappings']:
         regmap['map'] = placeholder(regmap['map'])
            # Now create the 'outer' encoding with indentation and search-and-replace
   # placeholders
            result = re.sub(
         '"JSON-([0-9]+)-NOSJ"',
   lambda m: replacements[int(m.group(1))],
                  @staticmethod
   def from_json(json):
               db.__enums = dict((k, Object.from_json(v)) for k, v in json['enums'].items())
   if 'register_types' in json:
         db.__register_types = dict(
      (k, Object.from_json(v))
      if 'register_mappings' in json:
            # Old format
   if 'registers' in json:
         for reg in json['registers']:
      type_ref = None
   if 'fields' in reg and reg['fields']:
      type_ref = reg['names'][0]
                     for name in reg['names']:
      regmap = Object(
         name=name,
   )
            db.__post_init()
      def deduplicate_enums(regdb):
      """
   Find enums that have the exact same entries and merge them.
   """
   buckets = defaultdict(list)
   for name, enum in regdb.enums():
            for bucket in buckets.values():
      if len(bucket) > 1:
      def deduplicate_register_types(regdb):
      """
   Find register types with the exact same fields (identified by name and
            However, register types *aren't* merged if they have different enums for
   the same field (as an exception, if one of them has an enum and the other
   one doesn't, we assume that one is simply missing a bit of information and
   merge the register types).
   """
   buckets = defaultdict(list)
   for name, regtype in regdb.register_types():
      key = ''.join(
         ':{0}:{1}:{2}:'.format(
         )
   )
         for bucket in buckets.values():
      # Register types in the same bucket have the same fields in the same
   # places, but they may have different enum_refs. Allow merging when
   # one has an enum_ref and another doesn't, but don't merge if they
   # have enum_refs that differ.
   bucket_enum_refs = [
         [getattr(field, 'enum_ref', None) for field in fields]
   ]
   while bucket:
         regtypes = [bucket[0][0]]
   enum_refs = bucket_enum_refs[0]
                  idx = 0
   while idx < len(bucket):
      if all([
      not lhs or not rhs or lhs == rhs
      ]):
      regtypes.append(bucket[idx][0])
   enum_refs = [lhs or rhs for lhs, rhs in zip(enum_refs, bucket_enum_refs[idx])]
   del bucket[idx]
                     # kate: space-indent on; indent-width 4; replace-tabs on;
