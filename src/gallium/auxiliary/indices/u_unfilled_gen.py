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

INTYPES = (GENERATE, UINT8, UINT16, UINT32)
OUTTYPES = (UINT16, UINT32)
PRIMS=('tris',
       'trifan',
       'tristrip',
       'quads',
       'quadstrip',
       'polygon',
       'trisadj',
       'tristripadj')

LONGPRIMS=('MESA_PRIM_TRIANGLES',
           'MESA_PRIM_TRIANGLE_FAN',
           'MESA_PRIM_TRIANGLE_STRIP',
           'MESA_PRIM_QUADS',
           'MESA_PRIM_QUAD_STRIP',
           'MESA_PRIM_POLYGON',
           'MESA_PRIM_TRIANGLES_ADJACENCY',
           'MESA_PRIM_TRIANGLE_STRIP_ADJACENCY')

longprim = dict(zip(PRIMS, LONGPRIMS))
intype_idx = dict(uint8='IN_UINT8', uint16='IN_UINT16', uint32='IN_UINT32')
outtype_idx = dict(uint16='OUT_UINT16', uint32='OUT_UINT32')

def prolog(f: 'T.TextIO'):
    f.write('/* File automatically generated by u_unfilled_gen.py */\n')
    f.write(copyright)
    f.write(r'''

/**
 * @file
 * Functions to translate and generate index lists
 */

#include "indices/u_indices.h"
#include "indices/u_indices_priv.h"
#include "util/compiler.h"
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"

static u_generate_func generate_line[OUT_COUNT][PRIM_COUNT];
static u_translate_func translate_line[IN_COUNT][OUT_COUNT][PRIM_COUNT];

''')

def vert( intype, outtype, v0 ):
    if intype == GENERATE:
        return '(' + outtype + '_t)(' + v0 + ')'
    else:
        return '(' + outtype + '_t)in[' + v0 + ']'

def line(f: 'T.TextIO', intype, outtype, ptr, v0, v1 ):
    f.write('      (' + ptr + ')[0] = ' + vert( intype, outtype, v0 ) + ';\n')
    f.write('      (' + ptr + ')[1] = ' + vert( intype, outtype, v1 ) + ';\n')

# XXX: have the opportunity here to avoid over-drawing shared lines in
# tristrips, fans, etc, by integrating this into the calling functions
# and only emitting each line at most once.
#
def do_tri(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2 ):
    line(f, intype, outtype, ptr, v0, v1 )
    line(f, intype, outtype, ptr + '+2', v1, v2 )
    line(f, intype, outtype, ptr + '+4', v2, v0 )

def do_quad(f: 'T.TextIO', intype, outtype, ptr, v0, v1, v2, v3 ):
    line(f, intype, outtype, ptr, v0, v1 )
    line(f, intype, outtype, ptr + '+2', v1, v2 )
    line(f, intype, outtype, ptr + '+4', v2, v3 )
    line(f, intype, outtype, ptr + '+6', v3, v0 )

def name(intype, outtype, prim):
    if intype == GENERATE:
        return 'generate_' + prim + '_' + outtype
    else:
        return 'translate_' + prim + '_' + intype + '2' + outtype

def preamble(f: 'T.TextIO', intype, outtype, prim):
    f.write('static void ' + name( intype, outtype, prim ) + '(\n')
    if intype != GENERATE:
        f.write('    const void * _in,\n')
    f.write('    unsigned start,\n')
    if intype != GENERATE:
        f.write('    unsigned in_nr,\n')
    f.write('    unsigned out_nr,\n')
    if intype != GENERATE:
        f.write('    unsigned restart_index,\n')
    f.write('    void *_out )\n')
    f.write('{\n')
    if intype != GENERATE:
        f.write('  const ' + intype + '_t *in = (const ' + intype + '_t*)_in;\n')
    f.write('  ' + outtype + '_t *out = (' + outtype + '_t*)_out;\n')
    f.write('  unsigned i, j;\n')
    f.write('  (void)j;\n')

def postamble(f: 'T.TextIO'):
    f.write('}\n')


def tris(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='tris')
    f.write('  for (i = start, j = 0; j < out_nr; j+=6, i+=3) {\n')
    do_tri(f, intype, outtype, 'out+j',  'i', 'i+1', 'i+2' );
    f.write('   }\n')
    postamble(f)


def tristrip(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='tristrip')
    f.write('  for (i = start, j = 0; j < out_nr; j+=6, i++) {\n')
    do_tri(f, intype, outtype, 'out+j',  'i', 'i+1/*+(i&1)*/', 'i+2/*-(i&1)*/' );
    f.write('   }\n')
    postamble(f)


def trifan(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='trifan')
    f.write('  for (i = start, j = 0; j < out_nr; j+=6, i++) {\n')
    do_tri(f, intype, outtype, 'out+j',  '0', 'i+1', 'i+2' );
    f.write('   }\n')
    postamble(f)



def polygon(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='polygon')
    f.write('  for (i = start, j = 0; j < out_nr; j+=2, i++) {\n')
    line(f, intype, outtype, 'out+j', 'i', '(i+1)%(out_nr/2)\n' )
    f.write('   }\n')
    postamble(f)


def quads(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='quads')
    f.write('  for (i = start, j = 0; j < out_nr; j+=8, i+=4) {\n')
    do_quad(f, intype, outtype, 'out+j', 'i+0', 'i+1', 'i+2', 'i+3' );
    f.write('   }\n')
    postamble(f)


def quadstrip(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='quadstrip')
    f.write('  for (i = start, j = 0; j < out_nr; j+=8, i+=2) {\n')
    do_quad(f, intype, outtype, 'out+j', 'i+2', 'i+0', 'i+1', 'i+3' );
    f.write('   }\n')
    postamble(f)


def trisadj(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='trisadj')
    f.write('  for (i = start, j = 0; j < out_nr; j+=6, i+=6) {\n')
    do_tri(f, intype, outtype, 'out+j',  'i', 'i+2', 'i+4' );
    f.write('   }\n')
    postamble(f)


def tristripadj(f: 'T.TextIO', intype, outtype):
    preamble(f, intype, outtype, prim='tristripadj')
    f.write('  for (i = start, j = 0; j < out_nr; j+=6, i+=2) {\n')
    do_tri(f, intype, outtype, 'out+j',  'i', 'i+2', 'i+4' );
    f.write('   }\n')
    postamble(f)


def emit_funcs(f: 'T.TextIO'):
    for intype, outtype in itertools.product(INTYPES, OUTTYPES):
        tris(f, intype, outtype)
        tristrip(f, intype, outtype)
        trifan(f, intype, outtype)
        quads(f, intype, outtype)
        quadstrip(f, intype, outtype)
        polygon(f, intype, outtype)
        trisadj(f, intype, outtype)
        tristripadj(f, intype, outtype)

def init(f: 'T.TextIO', intype, outtype, prim):
    if intype == GENERATE:
        f.write('generate_line[' +
                outtype_idx[outtype] +
                '][' + longprim[prim] +
                '] = ' + name( intype, outtype, prim ) + ';\n')
    else:
        f.write('translate_line[' +
                intype_idx[intype] +
                '][' + outtype_idx[outtype] +
                '][' + longprim[prim] +
                '] = ' + name( intype, outtype, prim ) + ';\n')


def emit_all_inits(f: 'T.TextIO'):
    for intype, outtype, prim in itertools.product(INTYPES, OUTTYPES, PRIMS):
        init(f, intype, outtype, prim)

def emit_init(f: 'T.TextIO'):
    f.write('void u_unfilled_init( void )\n')
    f.write('{\n')
    f.write('  static int firsttime = 1;\n')
    f.write('  if (!firsttime) return;\n')
    f.write('  firsttime = 0;\n')
    emit_all_inits(f)
    f.write('}\n')




def epilog(f: 'T.TextIO'):
    f.write('#include "indices/u_unfilled_indices.c"\n')


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('output')
    args = parser.parse_args()

    with open(args.output, 'w') as f:
        prolog(f)
        emit_funcs(f)
        emit_init(f)
        epilog(f)


if __name__ == '__main__':
    main()
