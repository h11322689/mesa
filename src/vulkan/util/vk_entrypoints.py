   # Copyright 2020 Intel Corporation
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the
   # "Software"), to deal in the Software without restriction, including
   # without limitation the rights to use, copy, modify, merge, publish,
   # distribute, sub license, and/or sell copies of the Software, and to
   # permit persons to whom the Software is furnished to do so, subject to
   # the following conditions:
   #
   # The above copyright notice and this permission notice (including the
   # next paragraph) shall be included in all copies or substantial portions
   # of the Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   # IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   # ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   # TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   # SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
      import xml.etree.ElementTree as et
      from collections import OrderedDict, namedtuple
      # Mesa-local imports must be declared in meson variable
   # '{file_without_suffix}_depend_files'.
   from vk_extensions import get_all_required, filter_api
      EntrypointParam = namedtuple('EntrypointParam', 'type name decl len')
      class EntrypointBase:
      def __init__(self, name):
      assert name.startswith('vk')
   self.name = name[2:]
   self.alias = None
   self.guard = None
   self.entry_table_index = None
   # Extensions which require this entrypoint
   self.core_version = None
         def prefixed_name(self, prefix):
         class Entrypoint(EntrypointBase):
      def __init__(self, name, return_type, params):
      super(Entrypoint, self).__init__(name)
   self.return_type = return_type
   self.params = params
   self.guard = None
   self.aliases = []
         def is_physical_device_entrypoint(self):
            def is_device_entrypoint(self):
            def decl_params(self, start=0):
            def call_params(self, start=0):
         class EntrypointAlias(EntrypointBase):
      def __init__(self, name, entrypoint):
      super(EntrypointAlias, self).__init__(name)
   self.alias = entrypoint
         def is_physical_device_entrypoint(self):
            def is_device_entrypoint(self):
            def prefixed_name(self, prefix):
            @property
   def params(self):
            @property
   def return_type(self):
            @property
   def disp_table_index(self):
            def decl_params(self):
            def call_params(self):
         def get_entrypoints(doc, api, beta):
      """Extract the entry points from the registry."""
                     for command in doc.findall('./commands/command'):
      if not filter_api(command, api):
            if 'alias' in command.attrib:
         name = command.attrib['name']
   target = command.attrib['alias']
   else:
         name = command.find('./proto/name').text
   ret_type = command.find('./proto/type').text
   params = [EntrypointParam(
      type=p.find('./type').text,
   name=p.find('./name').text,
   decl=''.join(p.itertext()),
      ) for p in command.findall('./param') if filter_api(p, api)]
            if name not in required:
            r = required[name]
   e.core_version = r.core_version
   e.extensions = r.extensions
            assert name not in entrypoints, name
               def get_entrypoints_from_xml(xml_files, beta, api='vulkan'):
               for filename in xml_files:
      doc = et.parse(filename)
         return entrypoints
