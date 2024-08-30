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
   import copy
      import license
   import gl_XML, glX_XML
      def should_use_push(registers):
      for [reg, offset] in registers:
      if reg[1:4] == "xmm":
         N = len(registers)
            def local_size(registers):
      # The x86-64 ABI says "the value (%rsp - 8) is always a multiple of
   # 16 when control is transfered to the function entry point."  This
   # means that the local stack usage must be (16*N)+8 for some value
   # of N.  (16*N)+8 = (8*(2N))+8 = 8*(2N+1).  As long as N is odd, we
            N = (len(registers) | 1)
            def save_all_regs(registers):
      adjust_stack = 0
   if not should_use_push(registers):
      adjust_stack = local_size(registers)
         for [reg, stack_offset] in registers:
                  def restore_all_regs(registers):
      adjust_stack = 0
   if not should_use_push(registers):
            temp = copy.deepcopy(registers)
   while len(temp):
      [reg, stack_offset] = temp.pop()
         if adjust_stack:
                  def save_reg(reg, offset, use_move):
      if use_move:
      if offset == 0:
         else:
      else:
                     def restore_reg(reg, offset, use_move):
      if use_move:
      if offset == 0:
         else:
      else:
                     class PrintGenericStubs(gl_XML.gl_print_base):
         def __init__(self):
               self.name = "gl_x86-64_asm.py (from Mesa)"
   self.license = license.bsd_license_template % ("(C) Copyright IBM Corporation 2005", "IBM")
            def get_stack_size(self, f):
      size = 0
   for p in f.parameterIterator():
                     def printRealHeader(self):
      print("/* If we build with gcc's -fvisibility=hidden flag, we'll need to change")
   print(" * the symbol visibility mode to 'default'.")
   print(' */')
   print('')
   print('#include "x86/assyntax.h"')
   print('')
   print('#ifdef __GNUC__')
   print('#  pragma GCC visibility push(default)')
   print('#  define HIDDEN(x) .hidden x')
   print('#else')
   print('#  define HIDDEN(x)')
   print('#endif')
   print('')
   print('#  define GL_PREFIX(n) GLNAME(CONCAT(gl,n))')
   print('')
   print('\t.text')
   print('')
   print('_x86_64_get_dispatch:')
   print('\tmovq\t_glapi_tls_Dispatch@GOTTPOFF(%rip), %rax')
   print('\tmovq\t%fs:(%rax), %rax')
   print('\tret')
   print('\t.size\t_x86_64_get_dispatch, .-_x86_64_get_dispatch')
   print('')
            def printRealFooter(self):
      print('')
   print('#if defined (__ELF__) && defined (__linux__)')
   print('	.section .note.GNU-stack,"",%progbits')
   print('#endif')
                        # The x86-64 ABI divides function parameters into a couple
   # classes.  For the OpenGL interface, the only ones that are
   # relevant are INTEGER and SSE.  Basically, the first 8
   # GLfloat or GLdouble parameters are placed in %xmm0 - %xmm7,
   # the first 6 non-GLfloat / non-GLdouble parameters are placed
   # in registers listed in int_parameters.
   #
   # If more parameters than that are required, they are passed
   # on the stack.  Therefore, we just have to make sure that
   # %esp hasn't changed when we jump to the actual function.
   # Since we're jumping to the function (and not calling it), we
                     int_class = 0
   sse_class = 0
   stack_offset = 0
   registers = []
   for p in f.parameterIterator():
                  if p.is_pointer() or (type_name != "GLfloat" and type_name != "GLdouble"):
      if int_class < 6:
      registers.append( [int_parameters[int_class], stack_offset] )
   int_class += 1
   else:
      if sse_class < 8:
               if ((int_class & 1) == 0) and (sse_class == 0):
                        print('\t.p2align\t4,,15')
   print('\t.globl\tGL_PREFIX(%s)' % (name))
   print('\t.type\tGL_PREFIX(%s), @function' % (name))
   if not f.is_static_entry_point(f.name):
         print('GL_PREFIX(%s):' % (name))
   print('\tcall\t_x86_64_get_dispatch@PLT')
   print('\tmovq\t%u(%%rax), %%r11' % (f.offset * 8))
            print('\t.size\tGL_PREFIX(%s), .-GL_PREFIX(%s)' % (name, name))
   print('')
            def printBody(self, api):
      for f in api.functionIterateByOffset():
               for f in api.functionIterateByOffset():
         dispatch = f.dispatch_name()
   for n in f.entry_points:
                                    if f.has_different_protocol(n):
                        def _parser():
      """Parse arguments and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                              def main():
      """Main file."""
   args = _parser()
   printer = PrintGenericStubs()
                     if __name__ == '__main__':
      main()
