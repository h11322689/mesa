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
      import gl_XML, glX_XML
   import license
      class PrintGlOffsets(gl_XML.gl_print_base):
      def __init__(self, es=False):
               self.name = "gl_apitemp.py (from Mesa)"
   """Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
   (C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
                     self.undef_list.append( "KEYWORD1" )
   self.undef_list.append( "KEYWORD1_ALT" )
   self.undef_list.append( "KEYWORD2" )
   self.undef_list.append( "NAME" )
   self.undef_list.append( "DISPATCH" )
   self.undef_list.append( "RETURN_DISPATCH" )
   self.undef_list.append( "DISPATCH_TABLE_NAME" )
   self.undef_list.append( "UNUSED_TABLE_NAME" )
            def printFunction(self, f, name):
      p_string = ""
   o_string = ""
   t_string = ""
            if f.is_static_entry_point(name):
         else:
                     silence = ''
   space = ''
   for p in f.parameterIterator(name):
                        if p.is_pointer():
                        t_string = t_string + comma + p.format_string()
   p_string = p_string + comma + p.name
                              if f.return_type != 'void':
         else:
            need_proto = False
   if not f.is_static_entry_point(name):
         elif self.es:
         cat, num = api.get_category_for_name(name)
   if (cat.startswith("es") or cat.startswith("GL_OES")):
   if need_proto:
                  print('%s %s KEYWORD2 NAME(%s)(%s)' % (keyword, f.return_type, n, f.get_parameter_string(name)))
   print('{')
   if silence:
         if p_string == "":
         print('   %s(%s, (), (F, "gl%s();\\n"));' \
   else:
         print('   %s(%s, (%s), (F, "gl%s(%s);\\n", %s));' \
   print('}')
   print('')
         def printRealHeader(self):
      print('')
   self.printVisibility( "HIDDEN", "hidden" )
   /*
   * This file is a template which generates the OpenGL API entry point
   * functions.  It should be included by a .c file which first defines
   * the following macros:
   *   KEYWORD1 - usually nothing, but might be __declspec(dllexport) on Win32
   *   KEYWORD2 - usually nothing, but might be __stdcall on Win32
   *   NAME(n)  - builds the final function name (usually add "gl" prefix)
   *   DISPATCH(func, args, msg) - code to do dispatch of named function.
   *                               msg is a printf-style debug message.
   *   RETURN_DISPATCH(func, args, msg) - code to do dispatch with a return value
   *
   * Here is an example which generates the usual OpenGL functions:
   *   #define KEYWORD1
   *   #define KEYWORD2
   *   #define NAME(func)  gl##func
   *   #define DISPATCH(func, args, msg)                             \\
   *          struct _glapi_table *dispatch = GLApi; \\
   *          (*dispatch->func) args
   *   #define RETURN DISPATCH(func, args, msg)                      \\
   *          struct _glapi_table *dispatch = GLApi; \\
   *          return (*dispatch->func) args
   *
   */
         #if defined( NAME )
   #ifndef KEYWORD1
   #define KEYWORD1
   #endif
      #ifndef KEYWORD1_ALT
   #define KEYWORD1_ALT HIDDEN
   #endif
      #ifndef KEYWORD2
   #define KEYWORD2
   #endif
      #ifndef DISPATCH
   #error DISPATCH must be defined
   #endif
      #ifndef RETURN_DISPATCH
   #error RETURN_DISPATCH must be defined
   #endif
      #if defined(_WIN32) && defined(_WINDOWS_)
   #error "Should not include <windows.h> here"
   #endif
      """)
                     def printInitDispatch(self, api):
      #endif /* defined( NAME ) */
      /*
   * This is how a dispatch table can be initialized with all the functions
   * we generated above.
   */
   #ifdef DISPATCH_TABLE_NAME
      #ifndef TABLE_ENTRY
   #error TABLE_ENTRY must be defined
   #endif
      #ifdef _GLAPI_SKIP_NORMAL_ENTRY_POINTS
   #error _GLAPI_SKIP_NORMAL_ENTRY_POINTS must not be defined
   #endif
      _glapi_proc DISPATCH_TABLE_NAME[] = {""")
         for f in api.functionIterateByOffset():
            print('   /* A whole bunch of no-op functions.  These might be called')
   print('    * when someone tries to call a dynamically-registered')
   print('    * extension function without a current rendering context.')
   print('    */')
   for i in range(1, 100):
            print('};')
   print('#endif /* DISPATCH_TABLE_NAME */')
   print('')
            def printAliasedTable(self, api):
      /*
   * This is just used to silence compiler warnings.
   * We list the functions which are not otherwise used.
   */
   #ifdef UNUSED_TABLE_NAME
   _glapi_proc UNUSED_TABLE_NAME[] = {""")
            normal_entries = []
   proto_entries = []
   for f in api.functionIterateByOffset():
                  # exclude f.name
   if f.name in normal_ents:
                                                print('#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS')
   for ent in normal_entries:
         print('#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */')
   print('#if GLAPI_EXPORT_PROTO_ENTRY_POINTS')
   for ent in proto_entries:
                  print('};')
   print('#endif /*UNUSED_TABLE_NAME*/')
   print('')
            def classifyEntryPoints(self, func):
      normal_names = []
   normal_stubs = []
   proto_names = []
   proto_stubs = []
   # classify the entry points
   for name in func.entry_points:
         if func.has_different_protocol(name):
      if func.is_static_entry_point(name):
         else:
      else:
      if func.is_static_entry_point(name):
            # there can be at most one stub for a function
   if normal_stubs:
         elif proto_stubs:
                  def printBody(self, api):
      normal_entry_points = []
   proto_entry_points = []
   for func in api.functionIterateByOffset():
         normal_ents, proto_ents = self.classifyEntryPoints(func)
            print('#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS')
   print('')
   for func, ents in normal_entry_points:
         for ent in ents:
   print('')
   print('#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */')
   print('')
   print('/* these entry points might require different protocols */')
   print('#if GLAPI_EXPORT_PROTO_ENTRY_POINTS')
   print('')
   for func, ents in proto_entry_points:
         for ent in ents:
   print('')
   print('#endif /* GLAPI_EXPORT_PROTO_ENTRY_POINTS */')
            self.printInitDispatch(api)
   self.printAliasedTable(api)
         def _parser():
      """Parser arguments and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                           parser.add_argument('-c',
                              def main():
      """Main function."""
                     printer = PrintGlOffsets(args.es)
            if __name__ == '__main__':
      main()
