   # Copyright 2022 Alyssa Rosenzweig
   # Copyright 2021 Collabora, Ltd.
   # Copyright 2016 Intel Corporation
   # SPDX-License-Identifier: MIT
      import argparse
   import sys
   import math
      a = 'a'
   b = 'b'
   c = 'c'
   d = 'd'
   e = 'e'
      lower_sm5_shift = []
      # Our shifts differ from SM5 for the upper bits. Mask to match the NIR
   # behaviour. Because this happens as a late lowering, NIR won't optimize the
   # masking back out (that happens in the main nir_opt_algebraic).
   for s in [8, 16, 32, 64]:
      for shift in ["ishl", "ishr", "ushr"]:
      lower_sm5_shift += [((shift, f'a@{s}', b),
      lower_pack = [
      (('pack_half_2x16_split', a, b),
            # We don't have 8-bit ALU, so we need to lower this. But if we lower it like
   # this, we can at least coalesce the pack_32_2x16_split and only pay the
   # cost of the iors and ishl. (u2u16 of 8-bit is assumed free.)
   (('pack_32_4x8_split', a, b, c, d),
   ('pack_32_2x16_split', ('ior', ('u2u16', a), ('ishl', ('u2u16', b), 8)),
            (('unpack_half_2x16_split_x', a), ('f2f32', ('unpack_32_2x16_split_x', a))),
            (('extract_u16', 'a@32', 0), ('u2u32', ('unpack_32_2x16_split_x', a))),
   (('extract_u16', 'a@32', 1), ('u2u32', ('unpack_32_2x16_split_y', a))),
   (('extract_i16', 'a@32', 0), ('i2i32', ('unpack_32_2x16_split_x', a))),
            # For optimizing extract->convert sequences for unpack/pack norm
   (('u2f32', ('u2u32', a)), ('u2f32', a)),
            # Chew through some 8-bit before the backend has to deal with it
   (('f2u8', a), ('u2u8', ('f2u16', a))),
            # Based on the VIR lowering
   (('f2f16_rtz', 'a@32'),
   ('bcsel', ('flt', ('fabs', a), ('fabs', ('f2f32', ('f2f16_rtne', a)))),
            # These are based on the lowerings from nir_opt_algebraic, but conditioned
   # on the number of bits not being constant. If the bit count is constant
   # (the happy path) we can use our native instruction instead.
   (('ibitfield_extract', 'value', 'offset', 'bits(is_not_const)'),
   ('bcsel', ('ieq', 0, 'bits'),
      0,
   ('ishr',
   ('ishl', 'value', ('isub', ('isub', 32, 'bits'), 'offset')),
         (('ubitfield_extract', 'value', 'offset', 'bits(is_not_const)'),
   ('iand',
      ('ushr', 'value', 'offset'),
   ('bcsel', ('ieq', 'bits', 32),
   0xffffffff,
         # Codegen depends on this trivial case being optimized out.
   (('ubitfield_extract', 'value', 'offset', 0), 0),
            # At this point, bitfield extracts are constant. We can only do constant
   # unsigned bitfield extract, so lower signed to unsigned + sign extend.
   (('ibitfield_extract', a, b, '#bits'),
   ('ishr', ('ishl', ('ubitfield_extract', a, b, 'bits'), ('isub', 32, 'bits')),
      ]
      # (x * y) + s = (x * y) + (s << 0)
   def imad(x, y, z):
            # (x * y) - s = (x * y) - (s << 0)
   def imsub(x, y, z):
            # x + (y << s) = (x * 1) + (y << s)
   def iaddshl(x, y, s):
            # x - (y << s) = (x * 1) - (y << s)
   def isubshl(x, y, s):
            fuse_imad = [
      # Reassociate imul+iadd chain in order to fuse imads. This pattern comes up
   # in compute shader lowering.
   (('iadd', ('iadd(is_used_once)', ('imul(is_used_once)', a, b),
                  # Fuse regular imad
   (('iadd', ('imul(is_used_once)', a, b), c), imad(a, b, c)),
      ]
      for s in range(1, 5):
      fuse_imad += [
      # Definitions
   (('iadd', a, ('ishl(is_used_once)', b, s)), iaddshl(a, b, s)),
            # ineg(x) is 0 - x
            # Definitions
   (imad(a, b, ('ishl(is_used_once)', c, s)), ('imadshl_agx', a, b, c, s)),
            # a + (a << s) = a + a * (1 << s) = a * (1 + (1 << s))
            # a - (a << s) = a - a * (1 << s) = a * (1 - (1 << s))
            # a - (a << s) = a * (1 - (1 << s)) = -(a * (1 << s) - 1)
            # iadd is SCIB, general shfit is IC (slower)
            # Discard lowering generates this pattern, clean it up
   ixor_bcsel = [
      (('ixor', ('bcsel', a, '#b', '#c'), '#d'),
      ]
      def main():
      parser = argparse.ArgumentParser()
   parser.add_argument('-p', '--import-path', required=True)
   args = parser.parse_args()
   sys.path.insert(0, args.import_path)
         def run():
                        print(nir_algebraic.AlgebraicPass("agx_nir_lower_algebraic_late",
         print(nir_algebraic.AlgebraicPass("agx_nir_fuse_algebraic_late",
         print(nir_algebraic.AlgebraicPass("agx_nir_opt_ixor_bcsel",
            if __name__ == '__main__':
      main()
