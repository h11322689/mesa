   #!/usr/bin/env python3
   # Copyright © 2019, 2022 Intel Corporation
   # SPDX-License-Identifier: MIT
      from __future__ import annotations
   from collections import OrderedDict
   import copy
   import io
   import pathlib
   import os.path
   import re
   import xml.etree.ElementTree as et
   import typing
      if typing.TYPE_CHECKING:
                  files: typing.List[pathlib.Path]
   validate: bool
         def get_filename(element: et.Element) -> str:
            def get_name(element: et.Element) -> str:
            def get_value(element: et.Element) -> int:
            def get_start(element: et.Element) -> int:
               BASE_TYPES = {
      'address',
   'offset',
   'int',
   'uint',
   'bool',
   'float',
   'mbz',
      }
      FIXED_PATTERN = re.compile(r"(s|u)(\d+)\.(\d+)")
      def is_base_type(name: str) -> bool:
            def add_struct_refs(items: typing.OrderedDict[str, bool], node: et.Element) -> None:
      if node.tag == 'field':
      if 'type' in node.attrib and not is_base_type(node.attrib['type']):
         t = node.attrib['type']
      if node.tag not in {'struct', 'group'}:
         for c in node:
            class Struct(object):
      def __init__(self, xml: et.Element):
      self.xml = xml
   self.name = xml.attrib['name']
         def find_deps(self, struct_dict, enum_dict) -> None:
      deps: typing.OrderedDict[str, bool] = OrderedDict()
   add_struct_refs(deps, self.xml)
   for d in deps.keys():
               def add_xml(self, items: typing.OrderedDict[str, et.Element]) -> None:
      for d in self.deps.values():
               # ordering of the various tag attributes
   GENXML_DESC = {
      'genxml'      : [ 'name', 'gen', ],
   'import'      : [ 'name', ],
   'exclude'     : [ 'name', ],
   'enum'        : [ 'name', 'value', 'prefix', ],
   'struct'      : [ 'name', 'length', ],
   'field'       : [ 'name', 'start', 'end', 'type', 'default', 'prefix', 'nonzero' ],
   'instruction' : [ 'name', 'bias', 'length', 'engine', ],
   'value'       : [ 'name', 'value', 'dont_use', ],
   'group'       : [ 'count', 'start', 'size', ],
      }
         def node_validator(old: et.Element, new: et.Element) -> bool:
      """Compare to ElementTree Element nodes.
      There is no builtin equality method, so calling `et.Element == et.Element` is
   equivalent to calling `et.Element is et.Element`. We instead want to compare
   that the contents are the same, including the order of children and attributes
   """
   return (
      # Check that the attributes are the same
   old.tag == new.tag and
   old.text == new.text and
   (old.tail or "").strip() == (new.tail or "").strip() and
   list(old.attrib.items()) == list(new.attrib.items()) and
            # check that there are no unexpected attributes
            # check that the attributes are sorted
   list(new.attrib) == list(old.attrib) and
               def process_attribs(elem: et.Element) -> None:
      valid = GENXML_DESC[elem.tag]
   # sort and prune attributes
   elem.attrib = OrderedDict(sorted(((k, v) for k, v in elem.attrib.items() if k in valid),
         for e in elem:
            def sort_xml(xml: et.ElementTree) -> None:
                        enums = sorted(xml.findall('enum'), key=get_name)
   enum_dict: typing.Dict[str, et.Element] = {}
   for e in enums:
      e[:] = sorted(e, key=get_value)
         # Structs are a bit annoying because they can refer to each other. We sort
   # them alphabetically and then build a graph of dependencies. Finally we go
   # through the alphabetically sorted list and print out dependencies first.
   structs = sorted(xml.findall('./struct'), key=get_name)
   wrapped_struct_dict: typing.Dict[str, Struct] = {}
   for s in structs:
      s[:] = sorted(s, key=get_start)
   ws = Struct(s)
         for ws in wrapped_struct_dict.values():
            sorted_structs: typing.OrderedDict[str, et.Element] = OrderedDict()
   for s in structs:
      _s = wrapped_struct_dict[s.attrib['name']]
         instructions = sorted(xml.findall('./instruction'), key=get_name)
   for i in instructions:
            registers = sorted(xml.findall('./register'), key=get_name)
   for r in registers:
            new_elems = (imports + enums + list(sorted_structs.values()) +
         for n in new_elems:
                  # `default_imports` documents which files should be imported for our
   # genxml files. This is only useful if a genxml file does not already
   # include imports.
   #
   # Basically, this allows the genxml_import.py tool used with the
   # --import switch to know which files should be added as an import.
   # (genxml_import.py uses GenXml.add_xml_imports, which relies on
   # `default_imports`.)
   default_imports = OrderedDict([
      ('gen4.xml', ()),
   ('gen45.xml', ('gen4.xml',)),
   ('gen5.xml', ('gen45.xml',)),
   ('gen6.xml', ('gen5.xml',)),
   ('gen7.xml', ('gen6.xml',)),
   ('gen75.xml', ('gen7.xml',)),
   ('gen8.xml', ('gen75.xml',)),
   ('gen9.xml', ('gen8.xml',)),
   ('gen11.xml', ('gen9.xml',)),
   ('gen12.xml', ('gen11.xml',)),
   ('gen125.xml', ('gen12.xml',)),
   ('gen20.xml', ('gen125.xml',)),
   ('gen20_rt.xml', ('gen125_rt.xml',)),
      known_genxml_files = list(default_imports.keys())
         def genxml_path_to_key(path):
      try:
         except ValueError:
            def sort_genxml_files(files):
            class GenXml(object):
      def __init__(self, filename, import_xml=False, files=None):
      if files is not None:
         else:
                  # Assert that the file hasn't already been loaded which would
   # indicate a loop in genxml imports, and lead to infinite
   # recursion.
            self.files.add(self.filename)
   self.et = et.parse(self.filename)
   if import_xml:
         def process_imported(self, merge=False, drop_dupes=False):
               This helper function scans imported genxml files and has two
            If `merge` is True, then items will be merged into the
            If `drop_dupes` is True, then any item that is a duplicate to
   an item imported will be droped from the `self.et` data
   structure. This is used by `self.optimize_xml_import` to
            """
   assert merge != drop_dupes
   orig_elements = set(self.et.getroot())
   name_and_obj = lambda i: (get_name(i), i)
   filter_ty = lambda s: filter(lambda i: i.tag == s, orig_elements)
            # orig_by_tag stores items defined directly in the genxml
   # file. If a genxml item is defined in the genxml directly,
   # then any imported items of the same name are ignored.
   orig_by_tag = {
         'enum': filter_ty_item('enum'),
   'struct': filter_ty_item('struct'),
   'instruction': filter_ty_item('instruction'),
            for item in orig_elements:
         if item.tag == 'import':
      assert 'name' in item.attrib
   filename = os.path.split(item.attrib['name'])
   exceptions = set()
   for e in item:
      assert e.tag == 'exclude'
      # We should be careful to restrict loaded files to
   # those under the source or build trees. For now, only
   # allow siblings of the current xml file.
   assert filename[0] == '', 'Directories not allowed with import'
                        # Here we load the imported genxml file. We set
   # `import_xml` to true so that any imports in the
   # imported genxml will be merged during the loading
   # process.
   #
   # The `files` parameter is a set of files that have
   # been loaded, and it is used to prevent any cycles
   # (infinite recursion) while loading imported genxml
                        # `to_add` is a set of items that were imported an
   # should be merged into the `self.et` data structure.
   # This is only used when the `merge` parameter is
   # True.
   to_add = set()
   # `to_remove` is a set of items that can safely be
   # imported since the item is equivalent. This is only
   # used when the `drop_duped` parameter is True.
   to_remove = set()
   for i in imported_elements:
      if i.tag not in orig_by_tag:
         if i.attrib['name'] in exceptions:
         if i.attrib['name'] in orig_by_tag[i.tag]:
         if merge:
      # An item with this same name was defined
   # in the genxml directly. There we should
      else:
         if drop_dupes:
      # Since this item is not the imported
      if merge:
         else:
         assert drop_dupes
                     if len(to_add) > 0:
      # Now that we have scanned through all the items
   # in the imported genxml file, if any items were
   # found which should be merged, we add them into
   # our `self.et` data structure. After this it will
   # be as if the items had been directly present in
   # the genxml file.
   assert len(to_remove) == 0
   self.et.getroot().extend(list(to_add))
               def merge_imported(self):
               Genxml <import> tags specify that elements should be brought
   in from another genxml source file. After this function is
   called, these elements will become part of the `self.et` data
   structure as if the elements had been directly included in the
            Items from imported genxml files will be completely ignore if
   an item with the same name is already defined in the genxml
            """
         def flatten_imported(self):
               Essentially this helper will put the `self.et` into a state
   that includes all imported items directly, and does not
   contain any <import> tags. This is used by the
   genxml_import.py with the --flatten switch to "undo" any
            """
   self.merge_imported()
   root = self.et.getroot()
   imports = root.findall('import')
   for i in imports:
         def add_xml_imports(self):
               Using the `default_imports` structure, we add imports to the
            """
   # `imports` is a set of filenames currently imported by the
   # genxml.
   imports = self.et.findall('import')
   imports = set(map(lambda el: el.attrib['name'], imports))
   new_elements = []
   self_flattened = copy.deepcopy(self)
   self_flattened.flatten_imported()
   old_names = { el.attrib['name'] for el in self_flattened.et.getroot() }
   for import_xml in default_imports.get(self.filename.name, tuple()):
         if import_xml in imports:
      # This genxml is already imported, so we don't need to
   # add it as an import.
      el = et.Element('import', {'name': import_xml})
   import_path = self.filename.with_name(import_xml)
   imported_genxml = GenXml(import_path, import_xml=True)
   imported_names = { el.attrib['name']
               # Importing this genxml could add some new items. When
   # adding a genxml import, we don't want to add new items,
   # unless they were already in the current genxml. So, we
   # put them into a list of items to exclude when importing
   # the genxml.
   exclude_names = imported_names - old_names
   for n in sorted(exclude_names):
         if len(new_elements) > 0:
               def optimize_xml_import(self):
               Scans genxml <import> tags, and loads the imported file. If
   any item in the imported file is a duplicate to an item in the
   genxml file, then it will be droped from the `self.et` data
            """
         def filter_engines(self, engines):
      changed = False
   items = []
   for item in self.et.getroot():
         # When an instruction doesn't have the engine specified,
   # it is considered to be for all engines. Otherwise, we
   # check to see if it's tagged for the engines requested.
   if item.tag == 'instruction' and 'engine' in item.attrib:
      i_engines = set(item.attrib["engine"].split('|'))
   if not (i_engines & engines):
      # Drop this instruction because it doesn't support
   # the requested engine types.
   changed = True
   if changed:
         def sort(self):
            def sorted_copy(self):
      clone = copy.deepcopy(self)
   clone.sort()
         def is_equivalent_xml(self, other):
      if len(self.et.getroot()) != len(other.et.getroot()):
         return all(node_validator(old, new)
         def write_file(self):
      try:
         old_genxml = GenXml(self.filename)
   if self.is_equivalent_xml(old_genxml):
   except Exception:
            b_io = io.BytesIO()
   et.indent(self.et, space='  ')
   self.et.write(b_io, encoding="utf-8", xml_declaration=True)
            tmp = self.filename.with_suffix(f'{self.filename.suffix}.tmp')
   tmp.write_bytes(b_io.getvalue())
   tmp.replace(self.filename)
