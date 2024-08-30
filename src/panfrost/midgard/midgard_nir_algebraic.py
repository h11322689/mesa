   #
   # Copyright (C) 2018 Alyssa Rosenzweig
   # Copyright (C) 2019-2020 Collabora, Ltd.
   #
   # Copyright (C) 2016 Intel Corporation
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
   import math
      a = 'a'
   b = 'b'
   c = 'c'
      algebraic = [
      # Allows us to schedule as a multiply by 2
            # Midgard scales fsin/fcos arguments by pi.
   (('fsin', a), ('fsin_mdg', ('fdiv', a, math.pi))),
      ]
      algebraic_late = [
      # Likewise we want fsub lowered but not isub
            # These two special-cases save space/an op than the actual csel op +
            (('b32csel', a, 'b@32', 0), ('iand', a, b)),
            # Fuse sat_signed. This should probably be shared with Bifrost
   (('~fmin', ('fmax', a, -1.0), 1.0), ('fsat_signed_mali', a)),
            # Fuse clamp_positive. This should probably be shared with Utgard/bifrost
            (('ishl', 'a@16', b), ('u2u16', ('ishl', ('u2u32', a), b))),
   (('ishr', 'a@16', b), ('i2i16', ('ishr', ('i2i32', a), b))),
            (('ishl', 'a@8', b), ('u2u8', ('u2u16', ('ishl', ('u2u32', ('u2u16', a)), b)))),
   (('ishr', 'a@8', b), ('i2i8', ('i2i16', ('ishr', ('i2i32', ('i2i16', a)), b)))),
            # Canonical form. The scheduler will convert back if it makes sense.
      ]
      # Size conversion is redundant to Midgard but needed for NIR, and writing this
   # lowering in MIR would be painful without a competent builder, so eat the
   # extra instruction
   for sz in ('8', '16', '32'):
      converted = ('u2u32', a) if sz != '32' else a
         # Midgard is able to type convert down by only one "step" per instruction; if
   # NIR wants more than one step, we need to break up into multiple instructions.
   # Nevertheless, we can do both a size step and a floating/int step at once.
      converts = []
      for op in ('u2u', 'i2i', 'f2f', 'i2f', 'u2f', 'f2i', 'f2u'):
      srcsz_max = 64
   dstsz_max = 64
   # 8 bit float doesn't exist
   srcsz_min = 8 if op[0] != 'f' else 16
   dstsz_min = 8 if op[2] != 'f' else 16
   dstsz = dstsz_min
   # Iterate over all possible destination and source sizes
   while dstsz <= dstsz_max:
      srcsz = srcsz_min
   while srcsz <= srcsz_max:
         # Size converter lowering is only needed if src and dst sizes are
   # spaced by a factor > 2.
   if srcsz != dstsz and (srcsz * 2 != dstsz and srcsz != dstsz * 2):
      cursz = srcsz
   rule = a
   # When converting down we first do the type conversion followed
   # by one or more size conversions. When converting up, we do
   # the type conversion at the end. This way we don't have to
   # deal with the fact that f2f8 doesn't exists.
   sizeconvop = op[0] + '2' + op[0] if srcsz < dstsz else op[2] + '2' + op[2]
   if srcsz > dstsz and op[0] != op[2]:
         while cursz != dstsz:
      cursz = cursz / 2 if dstsz < srcsz else cursz * 2
      if srcsz < dstsz and op[0] != op[2]:
               # Try to force constants to the right
   constant_switch = [
         # fge gets flipped to fle, so we invert to keep the order
   (('fge', 'a', '#b'), (('inot', ('flt', a, b)))),
   (('fge32', 'a', '#b'), (('inot', ('flt32', a, b)))),
   (('ige32', 'a', '#b'), (('inot', ('ilt32', a, b)))),
            # fge gets mapped to fle with a flip
   (('flt32', '#a', 'b'), ('inot', ('fge32', a, b))),
   (('ilt32', '#a', 'b'), ('inot', ('ige32', a, b))),
   ]
      # ..since the above switching happens after algebraic stuff is done
   cancel_inot = [
         (('inot', ('inot', a)), a),
   ]
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   args = parser.parse_args()
   sys.path.insert(0, args.import_path)
            def run():
                        print(nir_algebraic.AlgebraicPass("midgard_nir_lower_algebraic_early",
            print(nir_algebraic.AlgebraicPass("midgard_nir_lower_algebraic_late",
            print(nir_algebraic.AlgebraicPass("midgard_nir_cancel_inot",
            if __name__ == '__main__':
      main()
