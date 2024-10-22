      # (C) Copyright IBM Corporation 2004
   # All Rights Reserved.
   # Copyright (c) 2014 Intel Corporation
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
   #
   # Authors:
   #    Ian Romanick <idr@us.ibm.com>
      import argparse
      import gl_XML
   import license
         class PrintGlTable(gl_XML.gl_print_base):
      def __init__(self):
               self.header_tag = '_GLAPI_TABLE_H_'
   self.name = "gl_table.py (from Mesa)"
   """Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
   (C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
               def printBody(self, api):
      for f in api.functionIterateByOffset():
         arg_string = f.get_parameter_string()
         def printRealHeader(self):
      print('#include "util/glheader.h"')
   print('')
   print('#ifdef __cplusplus')
   print('extern "C" {')
   print('#endif')
   print('')
   print('#if defined(_WIN32) && defined(_WINDOWS_)')
   print('#error "Should not include <windows.h> here"')
   print('#endif')
   print('')
   print('struct _glapi_table')
   print('{')
         def printRealFooter(self):
      print('};')
   print('')
   print('#ifdef __cplusplus')
   print('}')
   print('#endif')
         class PrintRemapTable(gl_XML.gl_print_base):
      def __init__(self):
               self.header_tag = '_DISPATCH_H_'
   self.name = "gl_table.py (from Mesa)"
   self.license = license.bsd_license_template % (
                  def printRealHeader(self):
      /**
   * \\file main/dispatch.h
   * Macros for handling GL dispatch tables.
   *
   * For each known GL function, there are 3 macros in this file.  The first
   * macro is named CALL_FuncName and is used to call that GL function using
   * the specified dispatch table.  The other 2 macros, called GET_FuncName
   * can SET_FuncName, are used to get and set the dispatch pointer for the
   * named function in the specified dispatch table.
   */
      #include "util/glheader.h"
   """)
                  def printBody(self, api):
      print('#define CALL_by_offset(disp, cast, offset, parameters) \\')
   print('    (*(cast (GET_by_offset(disp, offset)))) parameters')
   print('#define GET_by_offset(disp, offset) \\')
   print('    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL')
   print('#define SET_by_offset(disp, offset, fn) \\')
   print('    do { \\')
   print('        if ( (offset) < 0 ) { \\')
   print('            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\\n", */ \\')
   print('            /*         __func__, __LINE__, disp, offset, # fn); */ \\')
   print('            /* abort(); */ \\')
   print('        } \\')
   print('        else { \\')
   print('            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \\')
   print('        } \\')
   print('    } while(0)')
            functions = []
   abi_functions = []
   count = 0
   for f in api.functionIterateByOffset():
         if not f.is_abi():
      functions.append([f, count])
               print('/* total number of offsets below */')
   print('#define _gloffset_COUNT %d' % (len(abi_functions + functions)))
            for f, index in abi_functions:
                     print('#define %s_size %u' % (remap_table, count))
   print('extern int %s[ %s_size ];' % (remap_table, remap_table))
            for f, index in functions:
                     for f, index in functions:
                     for f, index in abi_functions + functions:
                  print('typedef %s (GLAPIENTRYP _glptr_%s)(%s);' % (f.return_type, f.name, arg_string))
   print('#define CALL_{0}(disp, parameters) (* GET_{0}(disp)) parameters'.format(f.name))
      {1} (GLAPIENTRYP fn)({2}) = func; \\
      }} while (0)
   """.format(f.name, f.return_type, arg_string))
                  def _parser():
      """Parse arguments and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f', '--filename',
                           parser.add_argument('-m', '--mode',
                                    def main():
      """Main function."""
                     if args.mode == "table":
         elif args.mode == "remap_table":
                     if __name__ == '__main__':
      main()
