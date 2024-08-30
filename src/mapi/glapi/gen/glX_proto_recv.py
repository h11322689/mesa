      # (C) Copyright IBM Corporation 2005
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
   import string
      import gl_XML, glX_XML, glX_proto_common, license
         class PrintGlxDispatch_h(gl_XML.gl_print_base):
      def __init__(self):
               self.name = "glX_proto_recv.py (from Mesa)"
            self.header_tag = "_INDIRECT_DISPATCH_H_"
            def printRealHeader(self):
      print '#  include <X11/Xfuncproto.h>'
   print ''
   print 'struct __GLXclientStateRec;'
   print ''
            def printBody(self, api):
      for func in api.functionIterateAll():
         if not func.ignore and not func.vectorequiv:
      if func.glx_rop:
      print 'extern _X_HIDDEN void __glXDisp_%s(GLbyte * pc);' % (func.name)
                              if func.glx_sop and func.glx_vendorpriv:
                  class PrintGlxDispatchFunctions(glX_proto_common.glx_print_proto):
      def __init__(self, do_swap):
      gl_XML.gl_print_base.__init__(self)
   self.name = "glX_proto_recv.py (from Mesa)"
            self.real_types = [ '', '', 'uint16_t', '', 'uint32_t', '', '', '', 'uint64_t' ]
   self.do_swap = do_swap
            def printRealHeader(self):
      print '#include <inttypes.h>'
   print '#include "glxserver.h"'
   print '#include "indirect_size.h"'
   print '#include "indirect_size_get.h"'
   print '#include "indirect_dispatch.h"'
   print '#include "glxbyteorder.h"'
   print '#include "indirect_util.h"'
   print '#include "singlesize.h"'
   print ''
   print 'typedef struct {'
   print '    __GLX_PIXEL_3D_HDR;'
   print '} __GLXpixel3DHeader;'
   print ''
   print 'extern GLboolean __glXErrorOccured( void );'
   print 'extern void __glXClearErrorOccured( void );'
   print ''
   print 'static const unsigned dummy_answer[2] = {0, 0};'
   print ''
            def printBody(self, api):
      if self.do_swap:
               for func in api.functionIterateByOffset():
         if not func.ignore and not func.server_handcode and not func.vectorequiv and (func.glx_rop or func.glx_sop or func.glx_vendorpriv):
                              fptr = "pfngl" + name + "proc"
   return fptr.upper()
         def printFunction(self, f, name):
      if (f.glx_sop or f.glx_vendorpriv) and (len(f.get_images()) != 0):
            if not self.do_swap:
         else:
            if f.glx_rop:
         else:
                     if not f.is_abi():
            if f.glx_rop or f.vectorequiv:
         elif f.glx_sop or f.glx_vendorpriv:
         if len(f.get_images()) == 0: 
   else:
            print '}'
   print ''
            def swap_name(self, bytes):
               def emit_swap_wrappers(self, api):
      self.type_map = {}
            for t in api.typeIterate():
                                                                     print 'static _X_UNUSED %s' % (t_name)
   print 'bswap_%s(const void * src)' % (t.glx_name)
   print '{'
   print '    union { %s dst; %s ret; } x;' % (real_name, t_name)
   print '    x.dst = bswap_%u(*(%s *) src);' % (t_size * 8, real_name)
   print '    return x.ret;'
            for bits in [16, 32, 64]:
         print 'static void *'
   print 'bswap_%u_array(uint%u_t * src, unsigned count)' % (bits, bits)
   print '{'
   print '    unsigned  i;'
   print ''
   print '    for (i = 0 ; i < count ; i++) {'
   print '        uint%u_t temp = bswap_%u(src[i]);' % (bits, bits)
   print '        src[i] = temp;'
   print '    }'
   print ''
   print '    return src;'
            def fetch_param(self, param):
      t = param.type_string()
   o = param.offset
            if self.do_swap and (element_size != 1):
                           swap_func = self.swap_name( element_size )
      else:
         else:
         if param.is_array():
                           def emit_function_call(self, f, retval_assign, indent):
      list = []
            for param in f.parameterIterator():
                        if param.is_counter or param.is_image() or param.is_output or param.name in f.count_parameter_list or len(param.count_parameter_list):
                                    def common_func_print_just_start(self, f, indent):
      align64 = 0
               f.calculate_offsets()
   for param in f.parameterIterateGlxSend():
                                          # FIXME img_null_flag is over-loaded.  In addition to
   # FIXME being used for images, it is used to signify
   # FIXME NULL data pointers for vertex buffer object
                  if param.img_null_flag:
      print '%s    const CARD32 ptr_is_null = *(CARD32 *)(pc + %s);' % (indent, param.offset - 4)
                                                            if param.depth:
                           elif param.is_counter or param.name in f.count_parameter_list:
      location = self.fetch_param(param)
   print '%s    const %s %s = %s;' % (indent, type_string, param.name, location)
      elif len(param.count_parameter_list):
      if param.size() == 1 and not self.do_swap:
      location = self.fetch_param(param)
                        if need_blank:
            if align64:
                  if f.has_variable_size_request():
      self.emit_packet_size_calculation(f, 4)
                     print '    if ((unsigned long)(pc) & 7) {'
   print '        (void) memmove(pc-4, pc, %s);' % (s)
   print '        pc -= 4;'
   print '    }'
               need_blank = 0
   if self.do_swap:
         for param in f.parameterIterateGlxSend():
      if param.count_parameter_list:
                           if param.counter:
                                       if type_size == 1:
                                                                  if sub[0] == 1:
         else:
      swap_func = self.swap_name(sub[0])
   print '    default:'
   print '        return;'
   else:
                     else:
         for param in f.parameterIterateGlxSend():
                     if need_blank:
                        def printSingleFunction(self, f, name):
      if name not in f.glx_vendorpriv_names:
         else:
                     if self.do_swap:
         else:
            print ''
   if name not in f.glx_vendorpriv_names:
         else:
            print '    if ( cx != NULL ) {'
               if f.return_type != 'void':
         print '        %s retval;' % (f.return_type)
   retval_string = "retval"
   else:
                     type_size = 0
   answer_string = "dummy_answer"
   answer_count = "0"
            for param in f.parameterIterateOutputs():
         answer_type = param.get_base_type_string()
                     c = param.get_element_count()
   type_size = (param.size() / c)
   if type_size == 1:
                           if param.count_parameter_list:
      print '        const GLuint compsize = %s;' % (self.size_call(f, 1))
   print '        %s answerBuffer[200];' %  (answer_type)
                        print ''
   print '        if (%s == NULL) return BadAlloc;' % (param.name)
   print '        __glXClearErrorOccured();'
      elif param.counter:
      print '        %s answerBuffer[200];' %  (answer_type)
   print '        %s %s = __glXGetAnswerBuffer(cl, %s%s, answerBuffer, sizeof(answerBuffer), %u);' % (param.type_string(), param.name, param.counter, size_scale, type_size)
   answer_string = param.name
   answer_count = param.counter
   print ''
   print '        if (%s == NULL) return BadAlloc;' % (param.name)
   print '        __glXClearErrorOccured();'
      elif c >= 1:
                                                if f.needs_reply():
         if self.do_swap:
                                                                     #elif f.note_unflushed:
            print '        error = Success;'
   print '    }'
   print ''
   print '    return error;'
            def printRenderFunction(self, f):
      # There are 4 distinct phases in a rendering dispatch function.
   # In the first phase we compute the sizes and offsets of each
   # element in the command.  In the second phase we (optionally)
   # re-align 64-bit data elements.  In the third phase we
   # (optionally) byte-swap array data.  Finally, in the fourth
                     images = f.get_images()
   if len(images):
         if self.do_swap:
      pre = "bswap_CARD32( & "
      else:
                                                                  print '    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) %shdr->rowLength%s);' % (pre, post)
   if img.depth:
         print '    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) %shdr->skipRows%s);' % (pre, post)
   if img.depth:
         print '    glPixelStorei(GL_UNPACK_SKIP_PIXELS, (GLint) %shdr->skipPixels%s);' % (pre, post)
               self.emit_function_call(f, "", "")
         def _parser():
      """Parse any arguments passed and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                     parser.add_argument('-m',
                           parser.add_argument('-s',
                              def main():
      """Main function."""
            if args.mode == "dispatch_c":
         elif args.mode == "dispatch_h":
            api = gl_XML.parse_GL_API(
                     if __name__ == '__main__':
      main()
