      # (C) Copyright IBM Corporation 2004
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
   import gl_XML, glX_XML
      class PrintGenericStubs(gl_XML.gl_print_base):
      def __init__(self):
      gl_XML.gl_print_base.__init__(self)
   self.name = "gl_SPARC_asm.py (from Mesa)"
   """Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
   (C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
            def printRealHeader(self):
      print('#ifdef __arch64__')
   print('#define GL_OFF(N)\t((N) * 8)')
   print('#define GL_LL\t\tldx')
   print('#define GL_TIE_LD(SYM)\t%tie_ldx(SYM)')
   print('#define GL_STACK_SIZE\t128')
   print('#else')
   print('#define GL_OFF(N)\t((N) * 4)')
   print('#define GL_LL\t\tld')
   print('#define GL_TIE_LD(SYM)\t%tie_ld(SYM)')
   print('#define GL_STACK_SIZE\t64')
   print('#endif')
   print('')
   print('#define GLOBL_FN(x) .globl x ; .type x, @function')
   print('#define HIDDEN(x) .hidden x')
   print('')
   print('\t.register %g2, #scratch')
   print('\t.register %g3, #scratch')
   print('')
   print('\t.text')
   print('')
   print('\tGLOBL_FN(__glapi_sparc_icache_flush)')
   print('\tHIDDEN(__glapi_sparc_icache_flush)')
   print('\t.type\t__glapi_sparc_icache_flush, @function')
   print('__glapi_sparc_icache_flush: /* %o0 = insn_addr */')
   print('\tflush\t%o0')
   print('\tretl')
   print('\t nop')
   print('')
   print('\t.align\t32')
   print('')
   print('\t.type\t__glapi_sparc_get_pc, @function')
   print('__glapi_sparc_get_pc:')
   print('\tretl')
   print('\t add\t%o7, %g2, %g2')
   print('\t.size\t__glapi_sparc_get_pc, .-__glapi_sparc_get_pc')
   print('')
   print('')
   print('\tGLOBL_FN(__glapi_sparc_get_dispatch)')
   print('\tHIDDEN(__glapi_sparc_get_dispatch)')
   print('__glapi_sparc_get_dispatch:')
   print('\tmov\t%o7, %g1')
   print('\tsethi\t%hi(_GLOBAL_OFFSET_TABLE_-4), %g2')
   print('\tcall\t__glapi_sparc_get_pc')
   print('\tadd\t%g2, %lo(_GLOBAL_OFFSET_TABLE_+4), %g2')
   print('\tmov\t%g1, %o7')
   print('\tsethi\t%tie_hi22(_glapi_tls_Dispatch), %g1')
   print('\tadd\t%g1, %tie_lo10(_glapi_tls_Dispatch), %g1')
   print('\tGL_LL\t[%g2 + %g1], %g2, GL_TIE_LD(_glapi_tls_Dispatch)')
   print('\tretl')
   print('\t mov\t%g2, %o0')
   print('')
   print('\t.data')
   print('\t.align\t32')
   print('')
   print('\t/* --> sethi %hi(_glapi_tls_Dispatch), %g1 */')
   print('\t/* --> or %g1, %lo(_glapi_tls_Dispatch), %g1 */')
   print('\tGLOBL_FN(__glapi_sparc_tls_stub)')
   print('\tHIDDEN(__glapi_sparc_tls_stub)')
   print('__glapi_sparc_tls_stub: /* Call offset in %g3 */')
   print('\tmov\t%o7, %g1')
   print('\tsethi\t%hi(_GLOBAL_OFFSET_TABLE_-4), %g2')
   print('\tcall\t__glapi_sparc_get_pc')
   print('\tadd\t%g2, %lo(_GLOBAL_OFFSET_TABLE_+4), %g2')
   print('\tmov\t%g1, %o7')
   print('\tsrl\t%g3, 10, %g3')
   print('\tsethi\t%tie_hi22(_glapi_tls_Dispatch), %g1')
   print('\tadd\t%g1, %tie_lo10(_glapi_tls_Dispatch), %g1')
   print('\tGL_LL\t[%g2 + %g1], %g2, GL_TIE_LD(_glapi_tls_Dispatch)')
   print('\tGL_LL\t[%g7+%g2], %g1')
   print('\tGL_LL\t[%g1 + %g3], %g1')
   print('\tjmp\t%g1')
   print('\t nop')
   print('\t.size\t__glapi_sparc_tls_stub, .-__glapi_sparc_tls_stub')
   print('')
   print('#define GL_STUB(fn, off)\t\t\t\t\\')
   print('\tGLOBL_FN(fn);\t\t\t\t\t\\')
   print('fn:\tba\t__glapi_sparc_tls_stub;\t\t\t\\')
   print('\t sethi\tGL_OFF(off), %g3;\t\t\t\\')
   print('\t.size\tfn,.-fn;')
   print('')
   print('#define GL_STUB_ALIAS(fn, alias)		\\')
   print('	.globl	fn;				\\')
   print('	.set	fn, alias')
   print('')
   print('\t.text')
   print('\t.align\t32')
   print('')
   print('\t.globl\tgl_dispatch_functions_start')
   print('\tHIDDEN(gl_dispatch_functions_start)')
   print('gl_dispatch_functions_start:')
   print('')
         def printRealFooter(self):
      print('')
   print('\t.globl\tgl_dispatch_functions_end')
   print('\tHIDDEN(gl_dispatch_functions_end)')
   print('gl_dispatch_functions_end:')
         def printBody(self, api):
      for f in api.functionIterateByOffset():
                                    for f in api.functionIterateByOffset():
                  if f.is_static_entry_point(f.name):
                                    if f.has_different_protocol(n):
                        def _parser():
      """Parse arguments and return a namespace."""
   parser = argparse.ArgumentParser()
   parser.add_argument('-f',
                              def main():
      """Main function."""
   args = _parser()
            api = gl_XML.parse_GL_API(args.filename, glX_XML.glx_item_factory())
            if __name__ == '__main__':
      main()
