      # Mesa 3-D graphics library
   #
   # Copyright (C) 2010 LunarG Inc.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice shall be included
   # in all copies or substantial portions of the Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   # DEALINGS IN THE SOFTWARE.
   #
   # Authors:
   #    Chia-I Wu <olv@lunarg.com>
      import sys
   # make it possible to import glapi
   import os
   GLAPI = os.path.join(".", os.path.dirname(__file__), "glapi", "gen")
   sys.path.insert(0, GLAPI)
      from operator import attrgetter
   import re
   from optparse import OptionParser
   import gl_XML
   import glX_XML
         # number of dynamic entries
   ABI_NUM_DYNAMIC_ENTRIES = 256
      class ABIEntry(object):
               _match_c_param = re.compile(
            def __init__(self, cols, attrs, xml_data = None):
               self.slot = attrs['slot']
   self.hidden = attrs['hidden']
   self.alias = attrs['alias']
   self.handcode = attrs['handcode']
         def c_prototype(self):
            def c_return(self):
      ret = self.ret
   if not ret:
                  def c_params(self):
      """Return the parameter list used in the entry prototype."""
   c_params = []
   for t, n, a in self.params:
         sep = '' if t.endswith('*') else ' '
   arr = '[%d]' % a if a else ''
   if not c_params:
                  def c_args(self):
      """Return the argument list used in the entry invocation."""
   c_args = []
   for t, n, a in self.params:
                  def _parse(self, cols):
      ret = cols.pop(0)
   if ret == 'void':
                     params = []
   if not cols:
         elif len(cols) == 1 and cols[0] == 'void':
         else:
                  self.ret = ret
   self.name = name
         def _parse_param(self, c_param):
      m = self._match_c_param.match(c_param)
   if not m:
            c_type = m.group('type').strip()
   c_name = m.group('name')
   c_array = m.group('array')
                  def __str__(self):
            def __lt__(self, other):
      # compare slot, alias, and then name
   if self.slot == other.slot:
         if not self.alias:
                                 def abi_parse_xml(xml):
      """Parse a GLAPI XML file for ABI entries."""
            entry_dict = {}
   for func in api.functionIterateByOffset():
      # make sure func.name appear first
   entry_points = func.entry_points[:]
   entry_points.remove(func.name)
            for name in entry_points:
         attrs = {
         'slot': func.offset,
   'hidden': not func.is_static_entry_point(name),
                  # post-process attrs
   if attrs['alias']:
      try:
         except KeyError:
         if alias.alias:
            if attrs['handcode']:
                                       cols = []
   cols.append(func.return_type)
   cols.append(name)
                                       def abi_sanity_check(entries):
      if not entries:
            all_names = []
   last_slot = entries[-1].slot
   i = 0
   for slot in range(last_slot + 1):
      if entries[i].slot != slot:
         if entries[i].alias:
         raise Exception('first entry of slot %d aliases %s'
   handcode = None
   while i < len(entries) and entries[i].slot == slot:
         ent = entries[i]
   if not handcode and ent.handcode:
         elif ent.handcode != handcode:
                  if ent.name in all_names:
         if ent.alias and ent.alias.name not in all_names:
            if i < len(entries):
         class ABIPrinter(object):
               def __init__(self, entries):
               # sort entries by their names
            self.indent = ' ' * 3
   self.noop_warn = 'noop_warn'
   self.noop_generic = 'noop_generic'
            self.api_defines = []
   self.api_headers = ['"KHR/khrplatform.h"']
   self.api_call = 'KHRONOS_APICALL'
   self.api_entry = 'KHRONOS_APIENTRY'
                     self.lib_need_table_size = True
   self.lib_need_noop_array = True
   self.lib_need_stubs = True
   self.lib_need_all_entries = True
         def c_notice(self):
            def c_public_includes(self):
      """Return includes of the client API headers."""
   defines = ['#define ' + d for d in self.api_defines]
   includes = ['#include ' + h for h in self.api_headers]
         def need_entry_point(self, ent):
      """Return True if an entry point is needed for the entry."""
   # non-handcode hidden aliases may share the entry they alias
   use_alias = (ent.hidden and ent.alias and not ent.handcode)
         def c_public_declarations(self, prefix):
      """Return the declarations of public entry points."""
   decls = []
   for ent in self.entries:
         if not self.need_entry_point(ent):
         export = self.api_call if not ent.hidden else ''
                  def c_mapi_table(self):
      """Return defines of the dispatch table size."""
   num_static_entries = self.entries[-1].slot + 1
   return ('#define MAPI_TABLE_NUM_STATIC %d\n' + \
               def _c_function(self, ent, prefix, mangle=False, stringify=False):
      """Return the function name of an entry."""
   formats = {
               }
   fmt = formats[prefix.isupper()][stringify]
   name = ent.name
   if mangle and ent.hidden:
               def _c_function_call(self, ent, prefix):
      """Return the function name used for calling."""
   if ent.handcode:
         # _c_function does not handle this case
   formats = { True: '%s(%s)', False: '%s%s' }
   fmt = formats[prefix.isupper()]
   elif self.need_entry_point(ent):
         else:
               def _c_decl(self, ent, prefix, mangle=False, export=''):
      """Return the C declaration for the entry."""
   decl = '%s %s %s(%s)' % (ent.c_return(), self.api_entry,
         if export:
         if self.api_attrs:
                  def _c_cast(self, ent):
      """Return the C cast for the entry."""
   cast = '%s (%s *)(%s)' % (
                  def c_public_dispatches(self, prefix, no_hidden):
      """Return the public dispatch functions."""
   dispatches = []
   for ent in self.entries:
                                                               ret = ''
   if ent.ret:
         stmt1 = self.indent
   stmt1 += 'const struct _glapi_table *_tbl = %s();' % (
         stmt2 = self.indent
   stmt2 += 'mapi_func _func = ((const mapi_func *) _tbl)[%d];' % (
                                                         def c_public_initializer(self, prefix):
      """Return the initializer for public dispatch functions."""
   names = []
   for ent in self.entries:
                        name = '%s(mapi_func) %s' % (self.indent,
                  def c_stub_string_pool(self):
      """Return the string pool for use by stubs."""
   # sort entries by their names
            pool = []
   offsets = {}
   count = 0
   for ent in sorted_entries:
         offsets[ent] = count
            pool_str =  self.indent + '"' + \
               def c_stub_initializer(self, prefix, pool_offsets):
      """Return the initializer for struct mapi_stub array."""
   stubs = []
   for ent in self.entries_sorted_by_names:
                        def c_noop_functions(self, prefix, warn_prefix):
      """Return the noop functions."""
   noops = []
   for ent in self.entries:
                                 stmt1 = self.indent;
   space = ''
   for t, n, a in ent.params:
                                                if ent.ret:
      stmt2 = self.indent + 'return (%s) 0;' % (ent.ret)
                              def c_noop_initializer(self, prefix, use_generic):
      """Return an initializer for the noop dispatch table."""
   entries = [self._c_function(ent, prefix)
         if use_generic:
                     pre = self.indent + '(mapi_func) '
         def c_asm_gcc(self, prefix, no_hidden):
               for ent in self.entries:
                                                                              if ent.alias and not (ent.alias.hidden and no_hidden):
      asm.append('".globl "%s"\\n"' % (name))
   asm.append('".set "%s", "%s"\\n"' % (name,
      else:
                  if ent.handcode:
                  def output_for_lib(self):
               if self.c_header:
                  print()
   print('#ifdef MAPI_TMP_DEFINES')
   print(self.c_public_includes())
   print()
   print('#if defined(_WIN32) && defined(_WINDOWS_)')
   print('#error "Should not include <windows.h> here"')
   print('#endif')
   print()
   print(self.c_public_declarations(self.prefix_lib))
   print('#undef MAPI_TMP_DEFINES')
            if self.lib_need_table_size:
         print()
   print('#ifdef MAPI_TMP_TABLE')
   print(self.c_mapi_table())
            if self.lib_need_noop_array:
         print()
   print('#ifdef MAPI_TMP_NOOP_ARRAY')
   print('#ifdef DEBUG')
   print()
   print(self.c_noop_functions(self.prefix_noop, self.prefix_warn))
   print()
   print('const mapi_func table_%s_array[] = {' % (self.prefix_noop))
   print(self.c_noop_initializer(self.prefix_noop, False))
   print('};')
   print()
   print('#else /* DEBUG */')
   print()
   print('const mapi_func table_%s_array[] = {' % (self.prefix_noop))
   print(self.c_noop_initializer(self.prefix_noop, True))
   print('};')
   print()
   print('#endif /* DEBUG */')
            if self.lib_need_stubs:
         pool, pool_offsets = self.c_stub_string_pool()
   print()
   print('#ifdef MAPI_TMP_PUBLIC_STUBS')
   print('static const char public_string_pool[] =')
   print(pool)
   print()
   print('static const struct mapi_stub public_stubs[] = {')
   print(self.c_stub_initializer(self.prefix_lib, pool_offsets))
   print('};')
            if self.lib_need_all_entries:
         print()
   print('#ifdef MAPI_TMP_PUBLIC_ENTRIES')
   print(self.c_public_dispatches(self.prefix_lib, False))
   print()
   print('static const mapi_func public_entries[] = {')
   print(self.c_public_initializer(self.prefix_lib))
   print('};')
                  print()
   print('#ifdef MAPI_TMP_STUB_ASM_GCC')
   print('__asm__(')
   print(self.c_asm_gcc(self.prefix_lib, False))
   print(');')
            if self.lib_need_non_hidden_entries:
         all_hidden = True
   for ent in self.entries:
      if not ent.hidden:
      all_hidden = False
   if not all_hidden:
      print()
   print('#ifdef MAPI_TMP_PUBLIC_ENTRIES_NO_HIDDEN')
   print(self.c_public_dispatches(self.prefix_lib, True))
   print()
                        print()
   print('#ifdef MAPI_TMP_STUB_ASM_GCC_NO_HIDDEN')
   print('__asm__(')
   print(self.c_asm_gcc(self.prefix_lib, True))
         class GLAPIPrinter(ABIPrinter):
               def __init__(self, entries):
      for ent in entries:
                  self.api_defines = []
   self.api_headers = []
   self.api_call = 'GLAPI'
   self.api_entry = 'GLAPIENTRY'
            self.lib_need_table_size = False
   self.lib_need_noop_array = False
   self.lib_need_stubs = False
   self.lib_need_all_entries = False
            self.prefix_lib = 'GLAPI_PREFIX'
   self.prefix_noop = 'noop'
                  def _override_for_api(self, ent):
      """Override attributes of an entry if necessary for this
   printer."""
   # By default, no override is necessary.
         def _get_c_header(self):
      #define _GLAPI_TMP_H_
   #define GLAPI_PREFIX(func)  gl##func
   #define GLAPI_PREFIX_STR(func)  "gl"#func
      #include "util/glheader.h"
   #endif /* _GLAPI_TMP_H_ */"""
               class SharedGLAPIPrinter(GLAPIPrinter):
               def __init__(self, entries):
               self.lib_need_table_size = True
   self.lib_need_noop_array = True
   self.lib_need_stubs = True
   self.lib_need_all_entries = True
            self.prefix_lib = 'shared'
         def _override_for_api(self, ent):
      ent.hidden = True
         def _get_c_header(self):
      #define _GLAPI_TMP_H_
   #include "util/glheader.h"
   #endif /* _GLAPI_TMP_H_ */"""
               def parse_args():
               parser = OptionParser(usage='usage: %prog [options] <xml_file>')
   parser.add_option('-p', '--printer', dest='printer',
            options, args = parser.parse_args()
   if not args or options.printer not in printers:
      parser.print_help()
         if not args[0].endswith('.xml'):
      parser.print_help()
               def main():
      printers = {
      'glapi': GLAPIPrinter,
                        entries = abi_parse_xml(filename)
            printer = printers[options.printer](entries)
         if __name__ == '__main__':
      main()
