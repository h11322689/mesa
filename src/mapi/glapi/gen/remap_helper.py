      # Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
   # All Rights Reserved.
   #
   # This is based on extension_helper.py by Ian Romanick.
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # on the rights to use, copy, modify, merge, publish, distribute, sub
   # license, and/or sell copies of the Software, and to permit persons to whom
   # the Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   # IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      import argparse
      import license
   import gl_XML
         class PrintGlRemap(gl_XML.gl_print_base):
      def __init__(self):
               self.name = "remap_helper.py (from Mesa)"
   self.license = license.bsd_license_template % ("Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>", "Chia-I Wu")
            def printRealHeader(self):
      print('#include "main/dispatch.h"')
   print('#include "main/remap.h"')
   print('')
            def printBody(self, api):
               print('/* this is internal to remap.c */')
   print('#ifndef need_MESA_remap_table')
   print('#error Only remap.c should include this file!')
   print('#endif /* need_MESA_remap_table */')
            print('')
            # output string pool
   index = 0;
   for f in api.functionIterateAll():
                  # a function has either assigned offset, fixed offset,
   # or no offset
   if f.assign_offset:
         elif f.offset > 0:
                        print('   /* _mesa_function_pool[%d]: %s (%s) */' \
         print('   "gl%s\\0"' % f.entry_points[0])
   print('   ;')
            print('/* these functions need to be remapped */')
   print('static const struct gl_function_pool_remap MESA_remap_table_functions[] = {')
   # output all functions that need to be remapped
   # iterate by offsets so that they are sorted by remap indices
   for f in api.functionIterateByOffset():
         if not f.assign_offset:
         print('   { %5d, %s_remap_index },' \
   print('   {    -1, -1 }')
   print('};')
   print('')
         def _parser():
      """Parse input options and return a namsepace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f', '--filename',
                                    def main():
      """Main function."""
                     printer = PrintGlRemap()
            if __name__ == '__main__':
      main()
