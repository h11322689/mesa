      # (C) Copyright IBM Corporation 2004, 2005
   # All Rights Reserved.
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
      import license
   import gl_XML
   import glX_XML
         class PrintGlProcs(gl_XML.gl_print_base):
      def __init__(self, es=False):
               self.es = es
   self.name = "gl_procs.py (from Mesa)"
   """Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
   (C) Copyright IBM Corporation 2004, 2006""", "BRIAN PAUL, IBM")
         def printRealHeader(self):
      /* This file is only included by glapi.c and is used for
   * the GetProcAddress() function
   */
      typedef struct {
         #if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)
         #endif
         } glprocs_table_t;
      #if   !defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
   #  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , o }
   #elif  defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
   #  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f1 , o }
   #elif  defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
   #  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f2 , o }
   #elif !defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
   #  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f3 , o }
   #endif
      """)
               def printRealFooter(self):
      print('')
   print('#undef NAME_FUNC_OFFSET')
         def printFunctionString(self, name):
            def printBody(self, api):
      print('')
            base_offset = 0
   table = []
   for func in api.functionIterateByOffset():
         name = func.dispatch_name()
                                       for func in api.functionIterateByOffset():
         for n in func.entry_points:
                              if func.has_different_protocol(n):
                              print('    ;')
   print('')
   print('')
   print('#if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)')
   for func in api.functionIterateByOffset():
         for n in func.entry_points:
                  if self.es:
         categories = {}
   for func in api.functionIterateByOffset():
      for n in func.entry_points:
      cat, num = api.get_category_for_name(n)
   if (cat.startswith("es") or cat.startswith("GL_OES")):
         if cat not in categories:
         proto = 'GLAPI %s GLAPIENTRY %s(%s);' \
   if categories:
      print('')
   print('/* OpenGL ES specific prototypes */')
   print('')
   keys = sorted(categories.keys())
   for key in keys:
                        print('')
            for info in table:
            print('    NAME_FUNC_OFFSET(-1, NULL, NULL, NULL, 0)')
   print('};')
         def _parser():
               parser = argparse.ArgumentParser()
   parser.add_argument('-f', '--filename',
                           parser.add_argument('-c', '--es-version',
                              def main():
      """Main function."""
   args = _parser()
   api = gl_XML.parse_GL_API(args.file_name, glX_XML.glx_item_factory())
            if __name__ == '__main__':
      main()
