   #encoding=utf-8
   # Copyright © 2017 Intel Corporation
      # Permission is hereby granted, free of charge, to any person obtaining a copy
   # of this software and associated documentation files (the "Software"), to deal
   # in the Software without restriction, including without limitation the rights
   # to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   # copies of the Software, and to permit persons to whom the Software is
   # furnished to do so, subject to the following conditions:
      # The above copyright notice and this permission notice shall be included in
   # all copies or substantial portions of the Software.
      # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   # AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   # OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   # SOFTWARE.
      import argparse
   import intel_genxml
   import os
      from mako.template import Template
   from util import *
      TEMPLATE = Template("""\
   <%!
   from operator import itemgetter
   %>\
   /*
   * Copyright © 2017 Intel Corporation
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
      /* THIS FILE HAS BEEN GENERATED, DO NOT HAND EDIT.
   *
   * Sizes of bitfields in genxml instructions, structures, and registers.
   */
      #ifndef ${guard}
   #define ${guard}
      #include <stdint.h>
      #include "dev/intel_device_info.h"
   #include "util/macros.h"
      <%def name="emit_per_gen_prop_func(item, prop, protect_defines)">
   %if item.has_prop(prop):
   % for gen, value in sorted(item.iter_prop(prop), reverse=True):
   %  if protect_defines:
   #ifndef ${gen.prefix(item.token_name)}_${prop}
   #define ${gen.prefix(item.token_name)}_${prop}  ${value}
   #endif
   %  else:
   #define ${gen.prefix(item.token_name)}_${prop}  ${value}
   %  endif
   % endfor
      static inline uint32_t ATTRIBUTE_PURE
   ${item.token_name}_${prop}(const struct intel_device_info *devinfo)
   {
      switch (devinfo->verx10) {
   case 200: return ${item.get_prop(prop, 20)};
   case 125: return ${item.get_prop(prop, 12.5)};
   case 120: return ${item.get_prop(prop, 12)};
   case 110: return ${item.get_prop(prop, 11)};
   case 90: return ${item.get_prop(prop, 9)};
   case 80: return ${item.get_prop(prop, 8)};
   case 75: return ${item.get_prop(prop, 7.5)};
   case 70: return ${item.get_prop(prop, 7)};
   case 60: return ${item.get_prop(prop, 6)};
   case 50: return ${item.get_prop(prop, 5)};
   case 45: return ${item.get_prop(prop, 4.5)};
   case 40: return ${item.get_prop(prop, 4)};
   default:
            }
   %endif
   </%def>
      #ifdef __cplusplus
   extern "C" {
   #endif
   % for _, container in sorted(containers.items(), key=itemgetter(0)):
   %  if container.allowed:
      /* ${container.name} */
      ${emit_per_gen_prop_func(container, 'length', True)}
      %   for _, field in sorted(container.fields.items(), key=itemgetter(0)):
   %    if field.allowed:
      /* ${container.name}::${field.name} */
      ${emit_per_gen_prop_func(field, 'bits', False)}
      ${emit_per_gen_prop_func(field, 'start', False)}
   %    endif
   %   endfor
   %  endif
   % endfor
      #ifdef __cplusplus
   }
   #endif
      #endif /* ${guard} */""")
      class Gen(object):
         def __init__(self, z):
      # Convert potential "major.minor" string
         def __lt__(self, other):
            def __hash__(self):
            def __eq__(self, other):
            def prefix(self, token):
               if gen % 10 == 0:
            if token[0] == '_':
               class Container(object):
         def __init__(self, name):
      self.name = name
   self.token_name = safe_name(name)
   self.length_by_gen = {}
   self.fields = {}
         def add_gen(self, gen, xml_attrs):
      assert isinstance(gen, Gen)
   if 'length' in xml_attrs:
         def get_field(self, field_name, create=False):
      key = to_alphanum(field_name)
   if key not in self.fields:
         if create:
         else:
         def has_prop(self, prop):
      if prop == 'length':
         else:
         def iter_prop(self, prop):
      if prop == 'length':
         else:
         def get_prop(self, prop, gen):
      if not isinstance(gen, Gen):
            if prop == 'length':
         else:
      class Field(object):
         def __init__(self, container, name):
      self.name = name
   self.token_name = safe_name('_'.join([container.name, self.name]))
   self.bits_by_gen = {}
   self.start_by_gen = {}
         def add_gen(self, gen, xml_attrs):
      assert isinstance(gen, Gen)
   start = int(xml_attrs['start'])
   end = int(xml_attrs['end'])
   self.start_by_gen[gen] = start
         def has_prop(self, prop):
            def iter_prop(self, prop):
      if prop == 'bits':
         elif prop == 'start':
         else:
         def get_prop(self, prop, gen):
      if not isinstance(gen, Gen):
            if prop == 'bits':
         elif prop == 'start':
         else:
      class XmlParser(object):
         def __init__(self, containers):
      self.gen = None
   self.containers = containers
   self.container_stack = []
         def emit_genxml(self, genxml):
      root = genxml.et.getroot()
   self.gen = Gen(root.attrib['gen'])
   for item in root:
         def process_item(self, item):
      name = item.tag
   attrs = item.attrib
   if name in ('instruction', 'struct', 'register'):
         self.start_container(attrs)
   for struct_item in item:
         elif name == 'group':
         self.container_stack.append(None)
   for group_item in item:
         elif name == 'field':
         elif name in ('enum', 'import'):
         else:
         def start_container(self, attrs):
      assert self.container_stack[-1] is None
   name = attrs['name']
   if name not in self.containers:
         self.container_stack.append(self.containers[name])
         def process_field(self, attrs):
      if self.container_stack[-1] is None:
            field_name = attrs.get('name', None)
   if not field_name:
               def parse_args():
      p = argparse.ArgumentParser()
   p.add_argument('-o', '--output', type=str,
         p.add_argument('--cpp-guard', type=str,
         p.add_argument('--engines', nargs='?', type=str, default='render',
         p.add_argument('--include-symbols', type=str, action='store',
                                 if pargs.output in (None, '-'):
            if pargs.cpp_guard is None:
                  def main():
               engines = set(pargs.engines.split(','))
   valid_engines = [ 'render', 'blitter', 'video' ]
   if engines - set(valid_engines):
      print("Invalid engine specified, valid engines are:\n")
   for e in valid_engines:
               # Maps name => Container
            for source in pargs.xml_sources:
      p = XmlParser(containers)
   genxml = intel_genxml.GenXml(source)
   genxml.filter_engines(engines)
   genxml.merge_imported()
         included_symbols_list = pargs.include_symbols.split(',')
   for _name_field in included_symbols_list:
      name_field = _name_field.split('::')
   container = containers[name_field[0]]
   container.allowed = True
   if len(name_field) > 1:
         field = container.get_field(name_field[1])
         with open(pargs.output, 'w') as f:
         if __name__ == '__main__':
      main()
