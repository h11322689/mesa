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
   import sys, string
      import gl_XML, glX_XML
   import license
         class glx_enum_function(object):
      def __init__(self, func_name, enum_dict):
      self.name = func_name
   self.mode = 1
            # "enums" is a set of lists.  The element in the set is the
   # value of the enum.  The list is the list of names for that
   # value.  For example, [0x8126] = {"POINT_SIZE_MIN",
   # "POINT_SIZE_MIN_ARB", "POINT_SIZE_MIN_EXT",
                     # "count" is indexed by count values.  Each element of count
   # is a list of index to "enums" that have that number of
   # associated data elements.  For example, [4] = 
   # {GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_EMISSION,
   # GL_AMBIENT_AND_DIFFUSE} (the enum names are used here,
                        # Fill self.count and self.enums using the dictionary of enums
   # that was passed in.  The generic Get functions (e.g.,
   # GetBooleanv and friends) are handled specially here.  In
            if func_name in ["GetIntegerv", "GetBooleanv", "GetFloatv", "GetDoublev"]:
         else:
            mode_set = 0
   for enum_name in enum_dict:
                                                            if e.value in self.enums:
      if e.name not in self.enums[ e.value ]:
                                             def signature( self ):
      if self.sig == None:
         self.sig = ""
   for i in self.count:
                                          def is_set( self ):
               def PrintUsingTable(self):
      """Emit the body of the __gl*_size function using a pair
   of look-up tables and a mask.  The mask is calculated such
   that (e & mask) is unique for all the valid values of e for
   this function.  The result of (e & mask) is used as an index
   into the first look-up table.  If it matches e, then the
   same entry of the second table is returned.  Otherwise zero
            It seems like this should cause better code to be generated.
   However, on x86 at least, the resulting .o file is about 20%
   larger then the switch-statment version.  I am leaving this
   code in because the results may be different on other
            return 0
   count = 0
   for a in self.enums:
            if -1 in self.count:
            # Determine if there is some mask M, such that M = (2^N) - 1,
            mask = 0
   for i in [1, 2, 3, 4, 5, 6, 7, 8]:
                  fail = 0;
   for a in self.enums:
      for b in self.enums:
                     if not fail:
                  if (mask != 0) and (mask < (2 * count)):
                        for i in range(0, mask + 1):
                  for c in self.count:
      for e in self.count[c]:
      i = e & mask
                     print('    static const GLushort a[%u] = {' % (mask + 1))
   for e in masked_enums:
                  print('    static const GLubyte b[%u] = {' % (mask + 1))
   for c in masked_count:
                  print('    const unsigned idx = (e & 0x%02xU);' % (mask))
   print('')
   print('    return (e == a[idx]) ? (GLint) b[idx] : 0;')
   else:
            def PrintUsingSwitch(self, name):
      """Emit the body of the __gl*_size function using a 
                     for c in sorted(self.count):
                           # There may be multiple enums with the same
   # value.  This happens has extensions are
   # promoted from vendor-specific or EXT to
                                             keys = sorted(list.keys())
   for k in keys:
      j = list[k]
   if first:
                        if c == -1:
                  print('        default: return 0;')
            def Print(self, name):
      print('_X_INTERNAL PURE FASTCALL GLint')
   print('__gl%s_size( GLenum e )' % (name))
            if not self.PrintUsingTable():
            print('}')
         class glx_server_enum_function(glx_enum_function):
      def __init__(self, func, enum_dict):
               self.function = func
            def signature( self ):
      if self.sig == None:
                  p = self.function.variable_length_parameter()
                              def Print(self, name, printer):
      f = self.function
                     foo = {}
   for param_name in f.count_parameter_list:
                  for param_name in f.counter_list:
                  keys = sorted(foo.keys())
   for o in keys:
                              print('    GLsizei compsize;')
                     print('')
   print('    compsize = __gl%s_size(%s);' % (f.name, string.join(f.count_parameter_list, ",")))
   p = f.variable_length_parameter()
            print('}')
         class PrintGlxSizeStubs_common(gl_XML.gl_print_base):
      do_get = (1 << 0)
            def __init__(self, which_functions):
               self.name = "glX_proto_size.py (from Mesa)"
            self.emit_set = ((which_functions & PrintGlxSizeStubs_common.do_set) != 0)
   self.emit_get = ((which_functions & PrintGlxSizeStubs_common.do_get) != 0)
         class PrintGlxSizeStubs_c(PrintGlxSizeStubs_common):
      def printRealHeader(self):
      print('')
   print('#include <X11/Xfuncproto.h>')
   print('#include "util/glheader.h"')
   if self.emit_get:
         print('#include "indirect_size_get.h"')
                     print('')
   self.printPure()
   print('')
   self.printFastcall()
   print('')
   print('')
   print('#ifdef HAVE_FUNC_ATTRIBUTE_ALIAS')
   print('#  define ALIAS2(from,to) \\')
   print('    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\')
   print('        __attribute__ ((alias( # to )));')
   print('#  define ALIAS(from,to) ALIAS2( from, __gl ## to ## _size )')
   print('#else')
   print('#  define ALIAS(from,to) \\')
   print('    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\')
   print('    { return __gl ## to ## _size( e ); }')
   print('#endif')
   print('')
            def printBody(self, api):
      enum_sigs = {}
            for func in api.functionIterateGlx():
         ef = glx_enum_function( func.name, api.enums_by_name )
                  if (ef.is_set() and self.emit_set) or (not ef.is_set() and self.emit_get):
      sig = ef.signature()
   if sig in enum_sigs:
                        for [alias_name, real_name] in aliases:
            class PrintGlxSizeStubs_h(PrintGlxSizeStubs_common):
      def printRealHeader(self):
      * \\file
   * Prototypes for functions used to determine the number of data elements in
   * various GLX protocol messages.
   *
   * \\author Ian Romanick <idr@us.ibm.com>
   */
   """)
         print('#include <X11/Xfuncproto.h>')
   print('')
   self.printPure();
   print('')
   self.printFastcall();
            def printBody(self, api):
      for func in api.functionIterateGlx():
         ef = glx_enum_function( func.name, api.enums_by_name )
                        class PrintGlxReqSize_common(gl_XML.gl_print_base):
               The main purpose of this common base class is to provide the infrastructure
   for the derrived classes to iterate over the same set of functions.
            def __init__(self):
               self.name = "glX_proto_size.py (from Mesa)"
         class PrintGlxReqSize_h(PrintGlxReqSize_common):
      def __init__(self):
      PrintGlxReqSize_common.__init__(self)
            def printRealHeader(self):
      print('#include <X11/Xfuncproto.h>')
   print('')
   self.printPure()
            def printBody(self, api):
      for func in api.functionIterateGlx():
               class PrintGlxReqSize_c(PrintGlxReqSize_common):
               Create the server-side functions that are used to determine what the
   size of a varible length command should be.  The server then uses
   this value to determine if the incoming command packed it malformed.
            def __init__(self):
      PrintGlxReqSize_common.__init__(self)
            def printRealHeader(self):
      print('')
   print('#include "util/glheader.h"')
   print('#include "glxserver.h"')
   print('#include "glxbyteorder.h"')
   print('#include "indirect_size.h"')
   print('#include "indirect_reqsize.h"')
   print('')
   print('#ifdef HAVE_FUNC_ATTRIBUTE_ALIAS')
   print('#  define ALIAS2(from,to) \\')
   print('    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap, int reqlen ) \\')
   print('        __attribute__ ((alias( # to )));')
   print('#  define ALIAS(from,to) ALIAS2( from, __glX ## to ## ReqSize )')
   print('#else')
   print('#  define ALIAS(from,to) \\')
   print('    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap, int reqlen ) \\')
   print('    { return __glX ## to ## ReqSize( pc, swap, reqlen ); }')
   print('#endif')
   print('')
            def printBody(self, api):
      aliases = []
   enum_functions = {}
            for func in api.functionIterateGlx():
                                                                        for func in api.functionIterateGlx():
         # Even though server-handcode fuctions are on "the
   # list", and prototypes are generated for them, there
   # isn't enough information to generate a size
   # function.  If there was enough information, they
                                 if func.name in enum_functions:
                     if ef.name != func.name:
                     elif func.images:
         elif func.has_variable_size_request():
               for [alias_name, real_name] in aliases:
                     def common_emit_fixups(self, fixup):
               if fixup:
         print('    if (swap) {')
   for name in fixup:
                     def common_emit_one_arg(self, p, pc, adjust):
      offset = p.offset
   dst = p.string()
   src = '(%s *)' % (p.type_string())
   print('%-18s = *%11s(%s + %u);' % (dst, src, pc, offset + adjust));
            def common_func_print_just_header(self, f):
      print('int')
   print('__glX%sReqSize( const GLbyte * pc, Bool swap, int reqlen )' % (f.name))
            def printPixelFunction(self, f):
               f.offset_of( f.parameters[0].name )
                     if dim < 3:
         fixup = ['row_length', 'skip_rows', 'alignment']
   print('    GLint image_height = 0;')
   print('    GLint skip_images  = 0;')
   print('    GLint skip_rows    = *  (GLint *)(pc +  8);')
   else:
         fixup = ['row_length', 'image_height', 'skip_rows', 'skip_images', 'alignment']
   print('    GLint image_height = *  (GLint *)(pc +  8);')
   print('    GLint skip_rows    = *  (GLint *)(pc + 16);')
            img = f.images[0]
   for p in f.parameterIterateGlxSend():
         if p.name in [w, h, d, img.img_format, img.img_type, img.img_target]:
                              if img.img_null_flag:
         print('')
            print('')
   print('    return __glXImageSize(%s, %s, %s, %s, %s, %s,' % (img.img_format, img.img_type, img.img_target, w, h, d ))
   print('                          image_height, row_length, skip_images,')
   print('                          skip_rows, alignment);')
   print('}')
   print('')
                        sig = ""
   offset = 0
   fixup = []
   params = []
   size = ''
            # Calculate the offset of each counter parameter and the
   # size string for the variable length parameter(s).  While
   # that is being done, calculate a unique signature for this
            for p in f.parameterIterateGlxSend():
         if p.is_counter:
      fixup.append( p.name )
      elif p.counter:
                     sig += "(%u,%u)" % (f.offset_of(p.counter), s)
   if size == '':
               # If the calculated signature matches a function that has
   # already be emitted, don't emit this function.  Instead, add
            if sig in self.counter_sigs:
         n = self.counter_sigs[sig];
   else:
                                                   print('')
                  print('    return safe_pad(%s);' % (size))
                  def _parser():
      """Parse arguments and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.set_defaults(which_functions=(PrintGlxSizeStubs_common.do_get |
         parser.add_argument('-f',
                     parser.add_argument('-m',
                     getset = parser.add_mutually_exclusive_group()
   getset.add_argument('--only-get',
                           getset.add_argument('--only-set',
                           parser.add_argument('--header-tag',
                                    def main():
      """Main function."""
            if args.mode == "size_c":
         elif args.mode == "size_h":
      printer = PrintGlxSizeStubs_h(args.which_functions)
   if args.header_tag is not None:
      elif args.mode == "reqsize_c":
         elif args.mode == "reqsize_h":
                              if __name__ == '__main__':
      main()
