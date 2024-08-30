      # (C) Copyright IBM Corporation 2004, 2005
   # All Rights Reserved.
   # Copyright (c) 2015 Intel Corporation
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
   #    Jeremy Kolb <jkolb@brandeis.edu>
      import argparse
      import gl_XML, glX_XML, glX_proto_common, license
   import copy
      def convertStringForXCB(str):
      tmp = ""
   special = [ "ARB" ]
   i = 0
   while i < len(str):
      if str[i:i+3] in special:
         tmp = '%s_%s' % (tmp, str[i:i+3].lower())
   elif str[i].isupper():
         else:
                  def hash_pixel_function(func):
      """Generate a 'unique' key for a pixel function.  The key is based on
   the parameters written in the command packet.  This includes any
   padding that might be added for the original function and the 'NULL
               h = ""
   hash_pre = ""
   hash_suf = ""
   for param in func.parameterIterateGlxSend():
      if param.is_image():
                                                   if func.pad_after(param):
            n = func.name.replace("%uD" % (dim), "")
            h = hash_pre + h + hash_suf
            class glx_pixel_function_stub(glX_XML.glx_function):
      """Dummy class used to generate pixel "utility" functions that are
   shared by multiple dimension image functions.  For example, these
   objects are used to generate shared functions used to send GLX
   protocol for TexImage1D and TexImage2D, TexSubImage1D and
            def __init__(self, func, name):
      # The parameters to the utility function are the same as the
   # parameters to the real function except for the added "pad"
            self.name = name
   self.images = []
   self.parameters = []
   self.parameters_by_name = {}
   for _p in func.parameterIterator():
         p = copy.copy(_p)
                     if p.is_image():
                                                                        pad_name = func.pad_after(p)
   if pad_name:
      pad = copy.copy(p)
                           self.glx_rop = ~0
   self.glx_sop = 0
                     self.vectorequiv = None
   self.output = None
   self.can_be_large = func.can_be_large
   self.reply_always_array = func.reply_always_array
   self.dimensions_in_reply = func.dimensions_in_reply
            self.server_handcode = 0
   self.client_handcode = 0
            self.count_parameter_list = func.count_parameter_list
   self.counter_list = func.counter_list
   self.offsets_calculated = 0
         class PrintGlxProtoStubs(glX_proto_common.glx_print_proto):
      def __init__(self):
      glX_proto_common.glx_print_proto.__init__(self)
   self.name = "glX_proto_send.py (from Mesa)"
               self.last_category = ""
   self.generic_sizes = [3, 4, 6, 8, 12, 16, 24, 32]
   self.pixel_stubs = {}
   self.debug = 0
         def printRealHeader(self):
      print('')
   print('#include "util/glheader.h"')
   print('#include "indirect.h"')
   print('#include "glxclient.h"')
   print('#include "indirect_size.h"')
   print('#include "glapi.h"')
   print('#include <GL/glxproto.h>')
   print('#include <X11/Xlib-xcb.h>')
   print('#include <xcb/xcb.h>')
   print('#include <xcb/glx.h>')
            print('')
   self.printFastcall()
   self.printNoinline()
            print('static _X_INLINE int safe_add(int a, int b)')
   print('{')
   print('    if (a < 0 || b < 0) return -1;')
   print('    if (INT_MAX - a < b) return -1;')
   print('    return a + b;')
   print('}')
   print('static _X_INLINE int safe_mul(int a, int b)')
   print('{')
   print('    if (a < 0 || b < 0) return -1;')
   print('    if (a == 0 || b == 0) return 0;')
   print('    if (a > INT_MAX / b) return -1;')
   print('    return a * b;')
   print('}')
   print('static _X_INLINE int safe_pad(int a)')
   print('{')
   print('    int ret;')
   print('    if (a < 0) return -1;')
   print('    if ((ret = safe_add(a, 3)) < 0) return -1;')
   print('    return ret & (GLuint)~3;')
   print('}')
            print('#ifndef __GNUC__')
   print('#  define __builtin_expect(x, y) x')
   print('#endif')
   print('')
   print('/* If the size and opcode values are known at compile-time, this will, on')
   print(' * x86 at least, emit them with a single instruction.')
   print(' */')
   print('#define emit_header(dest, op, size)            \\')
   print('    do { union { short s[2]; int i; } temp;    \\')
   print('         temp.s[0] = (size); temp.s[1] = (op); \\')
   print('         *((int *)(dest)) = temp.i; } while(0)')
   print('')
   __glXReadReply( Display *dpy, size_t size, void * dest, GLboolean reply_is_always_array )
   {
      xGLXSingleReply reply;
      (void) _XReply(dpy, (xReply *) & reply, 0, False);
   if (size != 0) {
      if ((reply.length > 0) || reply_is_always_array) {
         const GLint bytes = (reply_is_always_array) 
                  _XRead(dpy, dest, bytes);
   if ( extra < 4 ) {
         }
   else {
                        }
      NOINLINE void
   __glXReadPixelReply( Display *dpy, struct glx_context * gc, unsigned max_dim,
      GLint width, GLint height, GLint depth, GLenum format, GLenum type,
      {
      xGLXSingleReply reply;
   GLint size;
               if ( dimensions_in_reply ) {
      width  = reply.pad3;
   height = reply.pad4;
      if ((height == 0) || (max_dim < 2)) { height = 1; }
   if ((depth  == 0) || (max_dim < 3)) { depth  = 1; }
               size = reply.length * 4;
   if (size != 0) {
               if ( buf == NULL ) {
         _XEatData(dpy, size);
   }
   else {
                  _XRead(dpy, buf, size);
   if ( extra < 4 ) {
                  __glEmptyImage(gc, 3, width, height, depth, format, type,
               }
      #define X_GLXSingle 0
      NOINLINE FASTCALL GLubyte *
   __glXSetupSingleRequest( struct glx_context * gc, GLint sop, GLint cmdlen )
   {
      xGLXSingleReq * req;
            (void) __glXFlushRenderBuffer(gc, gc->pc);
   LockDisplay(dpy);
   GetReqExtra(GLXSingle, cmdlen, req);
   req->reqType = gc->majorOpcode;
   req->contextTag = gc->currentContextTag;
   req->glxCode = sop;
      }
      NOINLINE FASTCALL GLubyte *
   __glXSetupVendorRequest( struct glx_context * gc, GLint code, GLint vop, GLint cmdlen )
   {
      xGLXVendorPrivateReq * req;
            (void) __glXFlushRenderBuffer(gc, gc->pc);
   LockDisplay(dpy);
   GetReqExtra(GLXVendorPrivate, cmdlen, req);
   req->reqType = gc->majorOpcode;
   req->glxCode = code;
   req->vendorCode = vop;
   req->contextTag = gc->currentContextTag;
      }
      const GLuint __glXDefaultPixelStore[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };
      #define zero                        (__glXDefaultPixelStore+0)
   #define one                         (__glXDefaultPixelStore+8)
   #define default_pixel_store_1D      (__glXDefaultPixelStore+4)
   #define default_pixel_store_1D_size 20
   #define default_pixel_store_2D      (__glXDefaultPixelStore+4)
   #define default_pixel_store_2D_size 20
   #define default_pixel_store_3D      (__glXDefaultPixelStore+0)
   #define default_pixel_store_3D_size 36
   #define default_pixel_store_4D      (__glXDefaultPixelStore+0)
   #define default_pixel_store_4D_size 36
   """)
            for size in self.generic_sizes:
                              self.pixel_stubs = {}
            for func in api.functionIterateGlx():
                  # If the function is a pixel function with a certain
   # GLX protocol signature, create a fake stub function
   # for it.  For example, create a single stub function
                  if func.glx_rop != 0:
      do_it = 0
   for image in func.get_images():
                                                                                 self.printFunction(func, func.name)
                  def printFunction(self, func, name):
      footer = '}\n'
   if func.glx_rop == ~0:
         print('static %s' % (func.return_type))
   print('%s( unsigned opcode, unsigned dim, %s )' % (func.name, func.get_parameter_string()))
   else:
         if func.has_different_protocol(name):
      if func.return_type == "void":
                        func_name = func.static_glx_name(name)
   print('#define %s %d' % (func.opcode_vendor_name(name), func.glx_vendorpriv))
   print('%s gl%s(%s)' % (func.return_type, func_name, func.get_parameter_string()))
   print('{')
   print('#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)')
   print('    if (((struct glx_context *)__glXGetCurrentContext())->isDirect) {')
   print('        const _glapi_proc *const disp_table = (_glapi_proc *)GET_DISPATCH();')
   print('        PFNGL%sPROC p =' % (name.upper()))
   print('            (PFNGL%sPROC) disp_table[%d];' % (name.upper(), func.offset))
   print('    %sp(%s);' % (ret_string, func.get_called_parameter_string()))
                                                      if func.glx_rop != 0 or func.vectorequiv != None:
         if len(func.images):
         else:
   elif func.glx_sop != 0 or func.glx_vendorpriv != 0:
         self.printSingleFunction(func, name)
   else:
            print(footer)
            def print_generic_function(self, n):
      size = (n + 3) & ~3
   generic_%u_byte( GLint rop, const void * ptr )
   {
      struct glx_context * const gc = __glXGetCurrentContext();
            emit_header(gc->pc, rop, cmdlen);
   (void) memcpy((void *)(gc->pc + 4), ptr, %u);
   gc->pc += cmdlen;
      }
   """ % (n, size + 4, size))
                  def common_emit_one_arg(self, p, pc, adjust, extra_offset):
      if p.is_array():
         else:
            if p.is_padding:
         print('(void) memset((void *)(%s + %u), 0, %s);' \
   elif not extra_offset:
         print('(void) memcpy((void *)(%s + %u), (void *)(%s), %s);' \
   else:
               def common_emit_args(self, f, pc, adjust, skip_vla):
               for p in f.parameterIterateGlxSend( not skip_vla ):
                           if p.is_variable_length():
      temp = p.size_string()
   if extra_offset:
                     def pixel_emit_args(self, f, pc, large):
      """Emit the arguments for a pixel function.  This differs from
   common_emit_args in that pixel functions may require padding
   be inserted (i.e., for the missing width field for
   TexImage1D), and they may also require a 'NULL image' flag
            if large:
         else:
            for param in f.parameterIterateGlxSend():
                                       else:
      [dim, width, height, depth, extent] = param.get_dimensions()
   if f.glx_rop == ~0:
                                             if param.img_null_flag:
      if large:
                                       if not large:
      if param.img_send_null:
                        print('if (%s) {' % (condition))
   print('    __glFillImage(gc, %s, %s, %s, %s, %s, %s, %s, %s, %s);' % (dim_str, width, height, depth, param.img_format, param.img_type, param.name, pcPtr, pixHeaderPtr))
   print('} else {')
   print('    (void) memcpy( %s, default_pixel_store_%uD, default_pixel_store_%uD_size );' % (pixHeaderPtr, dim, dim))
                     def large_emit_begin(self, f, op_name = None):
      if not op_name:
            print('const GLint op = %s;' % (op_name))
   print('const GLuint cmdlenLarge = cmdlen + 4;')
   print('GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);')
   print('(void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);')
   print('(void) memcpy((void *)(pc + 4), (void *)(&op), 4);')
            def common_func_print_just_start(self, f, name):
               # The only reason that single and vendor private commands need
   # a variable called 'dpy' is because they use the SyncHandle
   # macro.  For whatever brain-dead reason, that macro is hard-
   # coded to use a variable called 'dpy' instead of taking a
            # FIXME Simplify the logic related to skip_condition and
   # FIXME condition_list in this function.  Basically, remove
   # FIXME skip_condition, and just append the "dpy != NULL" type
   # FIXME condition to condition_list from the start.  The only
   # FIXME reason it's done in this confusing way now is to
            if not f.glx_rop:
         for p in f.parameterIterateOutputs():
                        print('    Display * const dpy = gc->currentDpy;')
   elif f.can_be_large:
         else:
               if f.return_type != 'void':
               if name != None and name not in f.glx_vendorpriv_names:
         self.emit_packet_size_calculation(f, 0)
   if name != None and name not in f.glx_vendorpriv_names:
            if f.command_variable_length() != "":
         print("    if (0%s < 0) {" % f.command_variable_length())
   print("        __glXSetError(gc, GL_INVALID_VALUE);")
   if f.return_type != 'void':
         else:
            condition_list = []
   for p in f.parameterIterateCounters():
         condition_list.append( "%s >= 0" % (p.name) )
   # 'counter' parameters cannot be negative
   print("    if (%s < 0) {" % p.name)
   print("        __glXSetError(gc, GL_INVALID_VALUE);")
   if f.return_type != 'void':
         else:
            if skip_condition:
            if len( condition_list ) > 0:
         if len( condition_list ) > 1:
                        print('    if (__builtin_expect(%s, 1)) {' % (skip_condition))
   else:
            def printSingleFunction(self, f, name):
               if self.debug:
                           # XCB specific:
   print('#ifdef USE_XCB')
   if self.debug:
         print('        xcb_connection_t *c = XGetXCBConnection(dpy);')
                  iparams=[]
   extra_iparams = []
   output = None
   for p in f.parameterIterator():
                        if p.is_image():
                                    # Hardcode this in.  lsb_first param (apparently always GL_FALSE)
   # also present in GetPolygonStipple, but taken care of above.
                              if f.needs_reply():
      print('        %s_reply_t *reply = %s_reply(c, %s, NULL);' % (xcb_name, xcb_name, xcb_request))
   if output:
      if output.is_image():
         [dim, w, h, d, junk] = output.get_dimensions()
   if f.dimensions_in_reply:
      w = "reply->width"
   h = "reply->height"
   d = "reply->depth"
   if dim < 2:
         else:
                                 else:
         if f.reply_always_array:
         else:
      print('        /* the XXX_data_length() xcb function name is misleading, it returns the number */')
                        if f.return_type != 'void':
            else:
                     if f.parameters != []:
         else:
            if name in f.glx_vendorpriv_names:
         else:
                              for img in images:
         if img.is_output:
      o = f.command_fixed_length() - 4
                                 return_name = ''
   if f.needs_reply():
         if f.return_type != 'void':
      return_name = " retval"
                              for p in f.parameterIterateOutputs():
      if p.is_image():
      [dim, w, h, d, junk] = p.get_dimensions()
   if f.dimensions_in_reply:
                           else:
      if f.reply_always_array:
                        # gl_parameter.size() returns the size
   # of the entire data item.  If the
   # item is a fixed-size array, this is
   # the size of the whole array.  This
   # is not what __glXReadReply wants. It
   # wants the size of a single data
                                                                     elif self.debug:
         # Only emit the extra glFinish call for functions
            if self.debug:
                        if name not in f.glx_vendorpriv_names:
            print('    }')
   print('    return%s;' % (return_name))
            def printPixelFunction(self, f):
      if f.name in self.pixel_stubs:
         # Normally gl_function::get_parameter_string could be
   # used.  However, this call needs to have the missing
                  p_string = ""
   for param in f.parameterIterateGlxSend():
                                                                     if self.common_func_print_just_start(f, None):
         else:
               if f.can_be_large:
         print('if (cmdlen <= gc->maxSmallRenderCommandSize) {')
   print('    if ( (gc->pc + cmdlen) > gc->bufEnd ) {')
            if f.glx_rop == ~0:
         else:
                     self.pixel_emit_args( f, "gc->pc", 0 )
   print('gc->pc += cmdlen;')
            if f.can_be_large:
                                          if trailer: print(trailer)
            def printRenderFunction(self, f):
      # There is a class of GL functions that take a single pointer
   # as a parameter.  This pointer points to a fixed-size chunk
   # of data, and the protocol for this functions is very
   # regular.  Since they are so regular and there are so many
   # of them, special case them with generic functions.  On
            if f.variable_length_parameter() == None and len(f.parameters) == 1:
         p = f.parameters[0]
   if p.is_pointer():
      cmdlen = f.command_fixed_length()
               if self.common_func_print_just_start(f, None):
         else:
            if self.debug:
            if f.can_be_large:
         print('if (cmdlen <= gc->maxSmallRenderCommandSize) {')
   print('    if ( (gc->pc + cmdlen) > gc->bufEnd ) {')
                     self.common_emit_args(f, "gc->pc", 4, 0)
   print('gc->pc += cmdlen;')
            if f.can_be_large:
                                       p = f.variable_length_parameter()
            if self.debug:
                  if trailer: print(trailer)
         class PrintGlxProtoInit_c(gl_XML.gl_print_base):
      def __init__(self):
               self.name = "glX_proto_send.py (from Mesa)"
   """Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   (C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")
                  def printRealHeader(self):
      * \\file indirect_init.c
   * Initialize indirect rendering dispatch table.
   *
   * \\author Kevin E. Martin <kevin@precisioninsight.com>
   * \\author Brian Paul <brian@precisioninsight.com>
   * \\author Ian Romanick <idr@us.ibm.com>
   */
      #include "indirect_init.h"
   #include "indirect.h"
   #include "glapi.h"
   #include <assert.h>
      #ifndef GLX_USE_APPLEGL
      /**
   * No-op function used to initialize functions that have no GLX protocol
   * support.
   */
   static int NoOp(void)
   {
         }
      /**
   * Create and initialize a new GL dispatch table.  The table is initialized
   * with GLX indirect rendering protocol functions.
   */
   struct _glapi_table * __glXNewIndirectAPI( void )
   {
      _glapi_proc *table;
   unsigned entries;
   unsigned i;
            entries = _glapi_get_dispatch_table_size();
   table = malloc(entries * sizeof(_glapi_proc));
   if (table == NULL)
            /* first, set all entries to point to no-op functions */
   for (i = 0; i < entries; i++) {
                           def printRealFooter(self):
            }
      #endif
   """)
                  def printBody(self, api):
      for [name, number] in api.categoryIterate():
         if number != None:
                        for func in api.functionIterateByCategory(name):
      if func.client_supported_for_indirect():
                           if func.is_abi():
         else:
                  class PrintGlxProtoInit_h(gl_XML.gl_print_base):
      def __init__(self):
               self.name = "glX_proto_send.py (from Mesa)"
   """Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   (C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")
                  self.last_category = ""
            def printRealHeader(self):
      * \\file
   * Prototypes for indirect rendering functions.
   *
   * \\author Kevin E. Martin <kevin@precisioninsight.com>
   * \\author Ian Romanick <idr@us.ibm.com>
   */
   """)
         self.printFastcall()
            #include <X11/Xfuncproto.h>
   #include "glxclient.h"
      extern _X_HIDDEN NOINLINE CARD32 __glXReadReply( Display *dpy, size_t size,
            extern _X_HIDDEN NOINLINE void __glXReadPixelReply( Display *dpy,
      struct glx_context * gc, unsigned max_dim, GLint width, GLint height,
   GLint depth, GLenum format, GLenum type, void * dest,
         extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupSingleRequest(
            extern _X_HIDDEN NOINLINE FASTCALL GLubyte * __glXSetupVendorRequest(
         """)
            def printBody(self, api):
      for func in api.functionIterateGlx():
                           for n in func.entry_points:
      if func.has_different_protocol(n):
      asdf = func.static_glx_name(n)
   if asdf not in func.static_entry_points:
         print('extern _X_HIDDEN %s gl%s(%s);' % (func.return_type, asdf, params))
   # give it a easy-to-remember name
                     print('')
   print('#ifdef GLX_INDIRECT_RENDERING')
   print('extern _X_HIDDEN void (*__indirect_get_proc_address(const char *name))(void);')
         def _parser():
      """Parse input and returned a parsed namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                     parser.add_argument('-m',
                           parser.add_argument('-d',
                              def main():
      """Main function."""
            if args.mode == "proto":
         elif args.mode == "init_c":
         elif args.mode == "init_h":
            printer.debug = args.debug
                     if __name__ == '__main__':
      main()
