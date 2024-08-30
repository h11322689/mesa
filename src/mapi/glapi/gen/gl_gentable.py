      # (C) Copyright IBM Corporation 2004, 2005
   # (C) Copyright Apple Inc. 2011
   # Copyright (C) 2015 Intel Corporation
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
   #    Jeremy Huddleston <jeremyhu@apple.com>
   #
   # Based on code ogiginally by:
   #    Ian Romanick <idr@us.ibm.com>
      import argparse
      import license
   import gl_XML, glX_XML
      header = """/* GLXEXT is the define used in the xserver when the GLX extension is being
   * built.  Hijack this to determine whether this file is being built for the
   * server or the client.
   */
   #ifdef HAVE_DIX_CONFIG_H
   #include <dix-config.h>
   #endif
      #ifndef _WIN32
   #include <dlfcn.h>
   #endif
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
      #include "glapi.h"
   #include "glapitable.h"
      #ifdef GLXEXT
   #include "os.h"
   #endif
      static void
   __glapi_gentable_NoOp(void) {
   #if defined(GLXEXT)
         #else
         #endif
   }
      static void
   __glapi_gentable_set_remaining_noop(struct _glapi_table *disp) {
      GLuint entries = _glapi_get_dispatch_table_size();
   void **dispatch = (void **) disp;
            /* ISO C is annoying sometimes */
   union {_glapi_proc p; void *v;} p;
            for(i=0; i < entries; i++)
      if(dispatch[i] == NULL)
   }
      """
      footer = """
   struct _glapi_table *
   _glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
      struct _glapi_table *disp = calloc(_glapi_get_dispatch_table_size(), sizeof(_glapi_proc));
            if(!disp)
            if(symbol_prefix == NULL)
            /* Note: This code relies on _glapi_table_func_names being sorted by the
   * entry point index of each function.
   */
   for (int func_index = 0; func_index < GLAPI_TABLE_COUNT; ++func_index) {
      const char *name = _glapi_table_func_names[func_index];
            #ifdef _WIN32
         #else
         #endif
      }
               }
      void
   _glapi_table_patch(struct _glapi_table *table, const char *name, void *wrapper)
   {
      for (int func_index = 0; func_index < GLAPI_TABLE_COUNT; ++func_index) {
      if (!strcmp(_glapi_table_func_names[func_index], name)) {
         ((void **)table)[func_index] = wrapper;
      }
      }
      """
         class PrintCode(gl_XML.gl_print_base):
         def __init__(self):
               self.name = "gl_gentable.py (from Mesa)"
   """Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
   (C) Copyright IBM Corporation 2004, 2005
   (C) Copyright Apple Inc 2011""", "BRIAN PAUL, IBM")
                     def get_stack_size(self, f):
      size = 0
   for p in f.parameterIterator():
                                    def printRealHeader(self):
      print(header)
            def printRealFooter(self):
      print(footer)
                        # Determine how many functions have a defined offset.
   func_count = 0
   for f in api.functions_by_name.values():
                  # Build the mapping from offset to function name.
   funcnames = [None] * func_count
   for f in api.functions_by_name.values():
         if f.offset != -1:
                  # Check that the table has no gaps.  We expect a function at every offset,
   # and the code which generates the table relies on this.
   for i in range(0, func_count):
                  print("#define GLAPI_TABLE_COUNT %d" % func_count)
   print("static const char * const _glapi_table_func_names[GLAPI_TABLE_COUNT] = {")
   for i in range(0, func_count):
                        def _parser():
      """Parse arguments and return a namespace object."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                                 def main():
      """Main function."""
                     api = gl_XML.parse_GL_API(args.filename, glX_XML.glx_item_factory())
            if __name__ == '__main__':
      main()
