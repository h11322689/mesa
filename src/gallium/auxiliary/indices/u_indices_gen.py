   copyright = '''
   /*
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
   * VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   '''
      import argparse
   import itertools
   import typing as T
      GENERATE, UINT8, UINT16, UINT32 = 'generate', 'uint8', 'uint16', 'uint32'
   FIRST, LAST = 'first', 'last'
   PRDISABLE, PRENABLE = 'prdisable', 'prenable'
      INTYPES = (GENERATE, UINT8, UINT16, UINT32)
   OUTTYPES = (UINT16, UINT32)
   PVS=(FIRST, LAST)
   PRS=(PRDISABLE, PRENABLE)
   PRIMS=('points',
         'lines',
   'linestrip',
   'lineloop',
   'tris',
   'trifan',
   'tristrip',
   'quads',
   'quadstrip',
   'polygon',
   'linesadj',
   'linestripadj',
   'trisadj',
      OUT_TRIS, OUT_QUADS = 'tris', 'quads'
      LONGPRIMS=('MESA_PRIM_POINTS',
            'MESA_PRIM_LINES',
   'MESA_PRIM_LINE_STRIP',
   'MESA_PRIM_LINE_LOOP',
   'MESA_PRIM_TRIANGLES',
   'MESA_PRIM_TRIANGLE_FAN',
   'MESA_PRIM_TRIANGLE_STRIP',
   'MESA_PRIM_QUADS',
   'MESA_PRIM_QUAD_STRIP',
   'MESA_PRIM_POLYGON',
   'MESA_PRIM_LINES_ADJACENCY',
   'MESA_PRIM_LINE_STRIP_ADJACENCY',
         longprim = dict(zip(PRIMS, LONGPRIMS))
   intype_idx = dict(uint8='IN_UINT8', uint16='IN_UINT16', uint32='IN_UINT32')
   outtype_idx = dict(uint16='OUT_UINT16', uint32='OUT_UINT32')
   pv_idx = dict(first='PV_FIRST', last='PV_LAST')
   pr_idx = dict(prdisable='PR_DISABLE', prenable='PR_ENABLE')
      def prolog(f: 'T.TextIO') -> None:
      f.write('/* File automatically generated by u_indices_gen.py */\n')
   f.write(copyright)
         /**
   * @file
   * Functions to translate and generate index lists
   */
      #include "indices/u_indices_priv.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
      #include "c99_compat.h"
      static u_translate_func translate[IN_COUNT][OUT_COUNT][PV_COUNT][PV_COUNT][PR_COUNT][PRIM_COUNT];
   static u_generate_func  generate[OUT_COUNT][PV_COUNT][PV_COUNT][PRIM_COUNT];
      static u_translate_func translate_quads[IN_COUNT][OUT_COUNT][PV_COUNT][PV_COUNT][PR_COUNT][PRIM_COUNT];
   static u_generate_func  generate_quads[OUT_COUNT][PV_COUNT][PV_COUNT][PRIM_COUNT];
         ''')
      def vert( intype, outtype, v0 ):
      if intype == GENERATE:
         else:
         def shape(f: 'T.TextIO', intype, outtype, ptr, *vertices):
      for i, v in enumerate(vertices):
         def do_point(f: 'T.TextIO', intype, outtype, ptr, v0 ):
            def do_line(f: 'T.TextIO', intype, outtype, ptr, v0, v1, inpv, outpv ):
      if inpv == outpv:
         else:
         def do_tri(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2, inpv, outpv ):
      if inpv == outpv:
         elif inpv == FIRST:
         else:
         def do_quad(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2, v3, inpv, outpv, out_prim ):
      if out_prim == OUT_TRIS:
      if inpv == LAST:
         do_tri(f, intype, outtype, ptr+'+0',  v0, v1, v3, inpv, outpv );
   else:
            else:
      if inpv == outpv:
         elif inpv == FIRST:
         else:
      def do_lineadj(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2, v3, inpv, outpv ):
      if inpv == outpv:
         else:
         def do_triadj(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2, v3, v4, v5, inpv, outpv ):
      if inpv == outpv:
         else:
         def name(intype, outtype, inpv, outpv, pr, prim, out_prim):
      if intype == GENERATE:
         else:
         def preamble(f: 'T.TextIO', intype, outtype, inpv, outpv, pr, prim, out_prim):
      f.write('static void ' + name( intype, outtype, inpv, outpv, pr, prim, out_prim ) + '(\n')
   if intype != GENERATE:
         f.write('    unsigned start,\n')
   if intype != GENERATE:
         f.write('    unsigned out_nr,\n')
   if intype != GENERATE:
         f.write('    void * restrict _out )\n')
   f.write('{\n')
   if intype != GENERATE:
         f.write('  ' + outtype + '_t * restrict out = (' + outtype + '_t* restrict)_out;\n')
   f.write('  unsigned i, j;\n')
         def postamble(f: 'T.TextIO'):
            def prim_restart(f: 'T.TextIO', in_verts, out_verts, out_prims, close_func = None):
      f.write('restart:\n')
   f.write('      if (i + ' + str(in_verts) + ' > in_nr) {\n')
   for i, j in itertools.product(range(out_prims), range(out_verts)):
         f.write('         continue;\n')
   f.write('      }\n')
   for i in range(in_verts):
      f.write('      if (in[i + ' + str(i) + '] == restart_index) {\n')
            if close_func is not None:
            f.write('         goto restart;\n')
      def points(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='points')
   f.write('  for (i = start, j = 0; j < out_nr; j++, i++) {\n')
   do_point(f, intype, outtype, 'out+j',  'i' );
   f.write('   }\n')
         def lines(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='lines')
   f.write('  for (i = start, j = 0; j < out_nr; j+=2, i+=2) {\n')
   do_line(f,  intype, outtype, 'out+j',  'i', 'i+1', inpv, outpv );
   f.write('   }\n')
         def linestrip(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='linestrip')
   f.write('  for (i = start, j = 0; j < out_nr; j+=2, i++) {\n')
   do_line(f, intype, outtype, 'out+j',  'i', 'i+1', inpv, outpv );
   f.write('   }\n')
         def lineloop(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='lineloop')
   f.write('  unsigned end = start;\n')
   f.write('  for (i = start, j = 0; j < out_nr - 2; j+=2, i++) {\n')
   if pr == PRENABLE:
      def close_func(index):
         do_line(f, intype, outtype, 'out+j',  'end', 'start', inpv, outpv )
   f.write('         start = i;\n')
                  do_line(f, intype, outtype, 'out+j',  'i', 'i+1', inpv, outpv );
   f.write('      end = i+1;\n')
   f.write('   }\n')
   do_line(f, intype, outtype, 'out+j',  'end', 'start', inpv, outpv );
         def tris(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='tris')
   f.write('  for (i = start, j = 0; j < out_nr; j+=3, i+=3) {\n')
   do_tri(f, intype, outtype, 'out+j',  'i', 'i+1', 'i+2', inpv, outpv );
   f.write('   }\n')
            def tristrip(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='tristrip')
   f.write('  for (i = start, j = 0; j < out_nr; j+=3, i++) {\n')
   if inpv == FIRST:
         else:
         f.write('   }\n')
            def trifan(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='trifan')
            if pr == PRENABLE:
      def close_func(index):
               if inpv == FIRST:
         else:
            f.write('   }')
               def polygon(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='polygon')
   f.write('  for (i = start, j = 0; j < out_nr; j+=3, i++) {\n')
   if pr == PRENABLE:
      def close_func(index):
               if inpv == FIRST:
         else:
         f.write('   }')
            def quads(f: 'T.TextIO', intype, outtype, inpv, outpv, pr, out_prim):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=out_prim, prim='quads')
   if out_prim == OUT_TRIS:
         else:
         if pr == PRENABLE and out_prim == OUT_TRIS:
         elif pr == PRENABLE:
            do_quad(f, intype, outtype, 'out+j', 'i+0', 'i+1', 'i+2', 'i+3', inpv, outpv, out_prim );
   f.write('   }\n')
            def quadstrip(f: 'T.TextIO', intype, outtype, inpv, outpv, pr, out_prim):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=out_prim, prim='quadstrip')
   if out_prim == OUT_TRIS:
         else:
         if pr == PRENABLE and out_prim == OUT_TRIS:
         elif pr == PRENABLE:
            if inpv == LAST:
         else:
         f.write('   }\n')
            def linesadj(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='linesadj')
   f.write('  for (i = start, j = 0; j < out_nr; j+=4, i+=4) {\n')
   do_lineadj(f, intype, outtype, 'out+j',  'i+0', 'i+1', 'i+2', 'i+3', inpv, outpv )
   f.write('  }\n')
            def linestripadj(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='linestripadj')
   f.write('  for (i = start, j = 0; j < out_nr; j+=4, i++) {\n')
   do_lineadj(f, intype, outtype, 'out+j',  'i+0', 'i+1', 'i+2', 'i+3', inpv, outpv )
   f.write('  }\n')
            def trisadj(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='trisadj')
   f.write('  for (i = start, j = 0; j < out_nr; j+=6, i+=6) {\n')
   do_triadj(f, intype, outtype, 'out+j',  'i+0', 'i+1', 'i+2', 'i+3',
         f.write('  }\n')
            def tristripadj(f: 'T.TextIO', intype, outtype, inpv, outpv, pr):
      preamble(f, intype, outtype, inpv, outpv, pr, out_prim=OUT_TRIS, prim='tristripadj')
   f.write('  for (i = start, j = 0; j < out_nr; i+=2, j+=6) {\n')
   f.write('    if (i % 4 == 0) {\n')
   f.write('      /* even triangle */\n')
   do_triadj(f, intype, outtype, 'out+j',
         f.write('    } else {\n')
   f.write('      /* odd triangle */\n')
   do_triadj(f, intype, outtype, 'out+j',
         f.write('    }\n')
   f.write('  }\n')
            def emit_funcs(f: 'T.TextIO') -> None:
      for intype, outtype, inpv, outpv, pr in itertools.product(
            if pr == PRENABLE and intype == GENERATE:
         points(f, intype, outtype, inpv, outpv, pr)
   lines(f, intype, outtype, inpv, outpv, pr)
   linestrip(f, intype, outtype, inpv, outpv, pr)
   lineloop(f, intype, outtype, inpv, outpv, pr)
   tris(f, intype, outtype, inpv, outpv, pr)
   tristrip(f, intype, outtype, inpv, outpv, pr)
   trifan(f, intype, outtype, inpv, outpv, pr)
   quads(f, intype, outtype, inpv, outpv, pr, OUT_TRIS)
   quadstrip(f, intype, outtype, inpv, outpv, pr, OUT_TRIS)
   polygon(f, intype, outtype, inpv, outpv, pr)
   linesadj(f, intype, outtype, inpv, outpv, pr)
   linestripadj(f, intype, outtype, inpv, outpv, pr)
   trisadj(f, intype, outtype, inpv, outpv, pr)
         for intype, outtype, inpv, outpv, pr in itertools.product(
            if pr == PRENABLE and intype == GENERATE:
         quads(f, intype, outtype, inpv, outpv, pr, OUT_QUADS)
      def init(f: 'T.TextIO', intype, outtype, inpv, outpv, pr, prim, out_prim=OUT_TRIS):
      generate_name = 'generate'
   translate_name = 'translate'
   if out_prim == OUT_QUADS:
      generate_name = 'generate_quads'
         if intype == GENERATE:
      f.write(f'{generate_name}[' +
            outtype_idx[outtype] +
   '][' + pv_idx[inpv] +
   '][' + pv_idx[outpv] +
   else:
      f.write(f'{translate_name}[' +
            intype_idx[intype] +
   '][' + outtype_idx[outtype] +
   '][' + pv_idx[inpv] +
   '][' + pv_idx[outpv] +
            def emit_all_inits(f: 'T.TextIO'):
      for intype, outtype, inpv, outpv, pr, prim in itertools.product(
                  for intype, outtype, inpv, outpv, pr, prim in itertools.product(
               def emit_init(f: 'T.TextIO'):
      f.write('void u_index_init( void )\n')
   f.write('{\n')
   f.write('  static int firsttime = 1;\n')
   f.write('  if (!firsttime) return;\n')
   f.write('  firsttime = 0;\n')
   emit_all_inits(f)
                  def epilog(f: 'T.TextIO') -> None:
               def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('output')
            with open(args.output, 'w') as f:
      prolog(f)
   emit_funcs(f)
   emit_init(f)
         if __name__ == '__main__':
      main()
