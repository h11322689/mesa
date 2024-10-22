   from functools import reduce
   import operator
      template = """\
   /* Copyright (C) 2018 Red Hat
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir.h"
      const nir_intrinsic_info nir_intrinsic_infos[nir_num_intrinsics] = {
   % for name, opcode in sorted(INTR_OPCODES.items()):
   {
      .name = "${name}",
      % if opcode.src_components:
      .src_components = {
            % endif
      .has_dest = ${"true" if opcode.has_dest else "false"},
   .dest_components = ${max(opcode.dest_components, 0)},
   .dest_bit_sizes = ${hex(reduce(operator.or_, opcode.bit_sizes, 0))},
   .bit_size_src = ${opcode.bit_size_src},
      % if opcode.indices:
         % for i in range(len(opcode.indices)):
         % endfor
      },
      % for i in range(len(opcode.indices)):
         % endfor
         % endif
         },
   % endfor
   };
      const char *nir_intrinsic_index_names[NIR_INTRINSIC_NUM_INDEX_FLAGS] = {
   % for index in INTR_INDICES:
         % endfor
   };
   """
      from nir_intrinsics import INTR_OPCODES, INTR_INDICES
   from mako.template import Template
   import argparse
   import os
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('--outdir', required=True,
                     path = os.path.join(args.outdir, 'nir_intrinsics.c')
   with open(path, 'w') as f:
      f.write(Template(template).render(
            if __name__ == '__main__':
         