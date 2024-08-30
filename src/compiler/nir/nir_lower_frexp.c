   /*
   * Copyright © 2015 Intel Corporation
   * Copyright © 2019 Valve Corporation
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
   #include "nir_builder.h"
      static nir_def *
   lower_frexp_sig(nir_builder *b, nir_def *x)
   {
      nir_def *abs_x = nir_fabs(b, x);
   nir_def *zero = nir_imm_floatN_t(b, 0, x->bit_size);
            switch (x->bit_size) {
   case 16:
      /* Half-precision floating-point values are stored as
   *   1 sign bit;
   *   5 exponent bits;
   *   10 mantissa bits.
   *
   * An exponent shift of 10 will shift the mantissa out, leaving only the
   * exponent and sign bit (which itself may be zero, if the absolute value
   * was taken before the bitcast and shift).
   */
   sign_mantissa_mask = nir_imm_intN_t(b, 0x83ffu, 16);
   /* Exponent of floating-point values in the range [0.5, 1.0). */
   exponent_value = nir_imm_intN_t(b, 0x3800u, 16);
      case 32:
      /* Single-precision floating-point values are stored as
   *   1 sign bit;
   *   8 exponent bits;
   *   23 mantissa bits.
   *
   * An exponent shift of 23 will shift the mantissa out, leaving only the
   * exponent and sign bit (which itself may be zero, if the absolute value
   * was taken before the bitcast and shift.
   */
   sign_mantissa_mask = nir_imm_int(b, 0x807fffffu);
   /* Exponent of floating-point values in the range [0.5, 1.0). */
   exponent_value = nir_imm_int(b, 0x3f000000u);
      case 64:
      /* Double-precision floating-point values are stored as
   *   1 sign bit;
   *   11 exponent bits;
   *   52 mantissa bits.
   *
   * An exponent shift of 20 will shift the remaining mantissa bits out,
   * leaving only the exponent and sign bit (which itself may be zero, if
   * the absolute value was taken before the bitcast and shift.
   */
   sign_mantissa_mask = nir_imm_int(b, 0x800fffffu);
   /* Exponent of floating-point values in the range [0.5, 1.0). */
   exponent_value = nir_imm_int(b, 0x3fe00000u);
      default:
                  if (x->bit_size == 64) {
      /* We only need to deal with the exponent so first we extract the upper
   * 32 bits using nir_unpack_64_2x32_split_y.
   */
            /* If x is ±0, ±Inf, or NaN, return x unmodified. */
   nir_def *new_upper =
      nir_bcsel(b,
            nir_iand(b,
               nir_ior(b,
                        } else {
      /* If x is ±0, ±Inf, or NaN, return x unmodified. */
   return nir_bcsel(b,
                  nir_iand(b,
               nir_ior(b,
      }
      static nir_def *
   lower_frexp_exp(nir_builder *b, nir_def *x)
   {
      nir_def *abs_x = nir_fabs(b, x);
   nir_def *zero = nir_imm_floatN_t(b, 0, x->bit_size);
   nir_def *is_not_zero = nir_fneu(b, abs_x, zero);
            switch (x->bit_size) {
   case 16: {
      nir_def *exponent_shift = nir_imm_int(b, 10);
            /* Significand return must be of the same type as the input, but the
   * exponent must be a 32-bit integer.
   */
   exponent = nir_i2i32(b, nir_iadd(b, nir_ushr(b, abs_x, exponent_shift),
            }
   case 32: {
      nir_def *exponent_shift = nir_imm_int(b, 23);
            exponent = nir_iadd(b, nir_ushr(b, abs_x, exponent_shift),
            }
   case 64: {
      nir_def *exponent_shift = nir_imm_int(b, 20);
            nir_def *zero32 = nir_imm_int(b, 0);
            exponent = nir_iadd(b, nir_ushr(b, abs_upper_x, exponent_shift),
            }
   default:
                     }
      static bool
   lower_frexp_instr(nir_builder *b, nir_instr *instr, UNUSED void *cb_data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu_instr = nir_instr_as_alu(instr);
                     switch (alu_instr->op) {
   case nir_op_frexp_sig:
      lower = lower_frexp_sig(b, nir_ssa_for_alu_src(b, alu_instr, 0));
      case nir_op_frexp_exp:
      lower = lower_frexp_exp(b, nir_ssa_for_alu_src(b, alu_instr, 0));
      default:
                  nir_def_rewrite_uses(&alu_instr->def, lower);
   nir_instr_remove(instr);
      }
      bool
   nir_lower_frexp(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader, lower_frexp_instr,
                  }
