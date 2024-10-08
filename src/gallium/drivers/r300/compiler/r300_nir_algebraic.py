   #
   # Copyright (C) 2019 Vasily Khoruzhick <anarsoul@gmail.com>
   # Copyright (C) 2021 Pavel Ondračka
   #
   # Permission is hereby granted, free of charge, to any person obtaining a
   # copy of this software and associated documentation files (the "Software"),
   # to deal in the Software without restriction, including without limitation
   # the rights to use, copy, modify, merge, publish, distribute, sublicense,
   # and/or sell copies of the Software, and to permit persons to whom the
   # Software is furnished to do so, subject to the following conditions:
   #
   # The above copyright notice and this permission notice (including the next
   # paragraph) shall be included in all copies or substantial portions of the
   # Software.
   #
   # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   # IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   # FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   # THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   # LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   # FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   # IN THE SOFTWARE.
      import argparse
   import sys
   from math import pi
      # Convenience variables
   a = 'a'
   b = 'b'
   c = 'c'
   d = 'd'
   e = 'e'
      # Transform input to range [-PI, PI]:
   #
   # y = frac(x / 2PI + 0.5) * 2PI - PI
   #
   transform_trig_input_vs_r500 = [
         (('fsin', 'a'), ('fsin', ('fadd', ('fmul', ('ffract', ('fadd', ('fmul', 'a', 1 / (2 * pi)) , 0.5)), 2 * pi), -pi))),
   ]
      # Transform input to range [-PI, PI]:
   #
   # y = frac(x / 2PI)
   #
   transform_trig_input_fs_r500 = [
         (('fsin', 'a'), ('fsin', ('ffract', ('fmul', 'a', 1 / (2 * pi))))),
   ]
      # The is a pattern produced by wined3d for A0 register load.
   # The specific pattern wined3d emits looks like this
   # A0.x = (int(floor(abs(R0.x) + 0.5) * sign(R0.x)));
   # however we lower both sign and floor so here we check for the already lowered
   # sequence.
   r300_nir_fuse_fround_d3d9 = [
         (('fmul', ('fadd', ('fadd', ('fabs', 'a') , 0.5),
                     ]
      # Here are some specific optimizations for code reordering such that the backend
   # has easier task of recognizing output modifiers and presubtract patterns.
   r300_nir_prepare_presubtract = [
         # Backend can only recognize 1 - x pattern.
   (('fadd', ('fneg', a), 1.0), ('fadd', 1.0, ('fneg', a))),
   (('fadd', a, -1.0), ('fneg', ('fadd', 1.0, ('fneg', a)))),
   (('fadd', -1.0, a), ('fneg', ('fadd', 1.0, ('fneg', a)))),
   # Bias presubtract 1 - 2 * x expects MAD -a 2.0 1.0 form.
   (('ffma', 2.0, ('fneg', a), 1.0), ('ffma', ('fneg', a), 2.0, 1.0)),
   (('ffma', a, -2.0, 1.0), ('fneg', ('ffma', ('fneg', a), 2.0, 1.0))),
   (('ffma', -2.0, a, 1.0), ('fneg', ('ffma', ('fneg', a), 2.0, 1.0))),
   (('ffma', 2.0, a, -1.0), ('fneg', ('ffma', ('fneg', a), 2.0, 1.0))),
   (('ffma', a, 2.0, -1.0), ('fneg', ('ffma', ('fneg', a), 2.0, 1.0))),
   # x * 2 can be usually folded into output modifier for the previous
   # instruction, but that only works if x is a temporary. If it is input or
   # constant just convert it to add instead.
   ]
      for multiplier in [2.0, 4.0, 8.0, 16.0, 0.5, 0.25, 0.125, 0.0625]:
      r300_nir_prepare_presubtract.extend([
      ])
      # Previous prepare_presubtract pass can sometimes produce double fneg patterns.
   # The backend copy propagate could handle it, but the nir to tgsi translation
   # does not and blows up. Just run a simple pass to clean it up.
   r300_nir_clean_double_fneg = [
         ]
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   parser.add_argument('output')
   args = parser.parse_args()
            import nir_algebraic  # pylint: disable=import-error
            r300_nir_lower_bool_to_float = [
      (('bcsel@32(is_only_used_as_float)', ignore_exact('feq', 'a@32', 'b@32'), c, d),
         ('fadd', ('fmul', c, ('seq', a, b)), ('fsub', d, ('fmul', d, ('seq', a, b)))),
   (('bcsel@32(is_only_used_as_float)', ignore_exact('fneu', 'a@32', 'b@32'), c, d),
               (('bcsel@32(is_only_used_as_float)', ignore_exact('flt', 'a@32', 'b@32'), c, d),
               (('bcsel@32(is_only_used_as_float)', ignore_exact('fge', 'a@32', 'b@32'), c, d),
         ]
         with open(args.output, 'w') as f:
               f.write(nir_algebraic.AlgebraicPass("r300_transform_vs_trig_input",
            f.write(nir_algebraic.AlgebraicPass("r300_transform_fs_trig_input",
            f.write(nir_algebraic.AlgebraicPass("r300_nir_fuse_fround_d3d9",
            f.write(nir_algebraic.AlgebraicPass("r300_nir_lower_bool_to_float",
            f.write(nir_algebraic.AlgebraicPass("r300_nir_prepare_presubtract",
            f.write(nir_algebraic.AlgebraicPass("r300_nir_clean_double_fneg",
      if __name__ == '__main__':
      main()
