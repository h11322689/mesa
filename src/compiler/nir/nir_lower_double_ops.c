   /*
   * Copyright © 2015 Intel Corporation
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
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      #include <float.h>
   #include <math.h>
      /*
   * Lowers some unsupported double operations, using only:
   *
   * - pack/unpackDouble2x32
   * - conversion to/from single-precision
   * - double add, mul, and fma
   * - conditional select
   * - 32-bit integer and floating point arithmetic
   */
      /* Creates a double with the exponent bits set to a given integer value */
   static nir_def *
   set_exponent(nir_builder *b, nir_def *src, nir_def *exp)
   {
      /* Split into bits 0-31 and 32-63 */
   nir_def *lo = nir_unpack_64_2x32_split_x(b, src);
            /* The exponent is bits 52-62, or 20-30 of the high word, so set the exponent
   * to 1023
   */
   nir_def *new_hi = nir_bitfield_insert(b, hi, exp,
               /* recombine */
      }
      static nir_def *
   get_exponent(nir_builder *b, nir_def *src)
   {
      /* get bits 32-63 */
            /* extract bits 20-30 of the high word */
      }
      /* Return infinity with the sign of the given source which is +/-0 */
      static nir_def *
   get_signed_inf(nir_builder *b, nir_def *zero)
   {
               /* The bit pattern for infinity is 0x7ff0000000000000, where the sign bit
   * is the highest bit. Only the sign bit can be non-zero in the passed in
   * source. So we essentially need to OR the infinity and the zero, except
   * the low 32 bits are always 0 so we can construct the correct high 32
   * bits and then pack it together with zero low 32 bits.
   */
   nir_def *inf_hi = nir_ior_imm(b, zero_hi, 0x7ff00000);
      }
      /*
   * Generates the correctly-signed infinity if the source was zero, and flushes
   * the result to 0 if the source was infinity or the calculated exponent was
   * too small to be representable.
   */
      static nir_def *
   fix_inv_result(nir_builder *b, nir_def *res, nir_def *src,
         {
      /* If the exponent is too small or the original input was infinity/NaN,
   * force the result to 0 (flush denorms) to avoid the work of handling
   * denorms properly. Note that this doesn't preserve positive/negative
   * zeros, but GLSL doesn't require it.
   */
   res = nir_bcsel(b, nir_ior(b, nir_ile_imm(b, exp, 0), nir_feq_imm(b, nir_fabs(b, src), INFINITY)),
            /* If the original input was 0, generate the correctly-signed infinity */
   res = nir_bcsel(b, nir_fneu_imm(b, src, 0.0f),
               }
      static nir_def *
   lower_rcp(nir_builder *b, nir_def *src)
   {
      /* normalize the input to avoid range issues */
            /* cast to float, do an rcp, and then cast back to get an approximate
   * result
   */
            /* Fixup the exponent of the result - note that we check if this is too
   * small below.
   */
   nir_def *new_exp = nir_isub(b, get_exponent(b, ra),
                           /* Do a few Newton-Raphson steps to improve precision.
   *
   * Each step doubles the precision, and we started off with around 24 bits,
   * so we only need to do 2 steps to get to full precision. The step is:
   *
   * x_new = x * (2 - x*src)
   *
   * But we can re-arrange this to improve precision by using another fused
   * multiply-add:
   *
   * x_new = x + x * (1 - x*src)
   *
   * See https://en.wikipedia.org/wiki/Division_algorithm for more details.
            ra = nir_ffma(b, nir_fneg(b, ra), nir_ffma_imm2(b, ra, src, -1), ra);
               }
      static nir_def *
   lower_sqrt_rsq(nir_builder *b, nir_def *src, bool sqrt)
   {
      /* We want to compute:
   *
   * 1/sqrt(m * 2^e)
   *
   * When the exponent is even, this is equivalent to:
   *
   * 1/sqrt(m) * 2^(-e/2)
   *
   * and then the exponent is odd, this is equal to:
   *
   * 1/sqrt(m * 2) * 2^(-(e - 1)/2)
   *
   * where the m * 2 is absorbed into the exponent. So we want the exponent
   * inside the square root to be 1 if e is odd and 0 if e is even, and we
   * want to subtract off e/2 from the final exponent, rounded to negative
   * infinity. We can do the former by first computing the unbiased exponent,
   * and then AND'ing it with 1 to get 0 or 1, and we can do the latter by
   * shifting right by 1.
            nir_def *unbiased_exp = nir_iadd_imm(b, get_exponent(b, src),
         nir_def *even = nir_iand_imm(b, unbiased_exp, 1);
            nir_def *src_norm = set_exponent(b, src,
            nir_def *ra = nir_f2f64(b, nir_frsq(b, nir_f2f32(b, src_norm)));
   nir_def *new_exp = nir_isub(b, get_exponent(b, ra), half);
            /*
   * The following implements an iterative algorithm that's very similar
   * between sqrt and rsqrt. We start with an iteration of Goldschmit's
   * algorithm, which looks like:
   *
   * a = the source
   * y_0 = initial (single-precision) rsqrt estimate
   *
   * h_0 = .5 * y_0
   * g_0 = a * y_0
   * r_0 = .5 - h_0 * g_0
   * g_1 = g_0 * r_0 + g_0
   * h_1 = h_0 * r_0 + h_0
   *
   * Now g_1 ~= sqrt(a), and h_1 ~= 1/(2 * sqrt(a)). We could continue
   * applying another round of Goldschmit, but since we would never refer
   * back to a (the original source), we would add too much rounding error.
   * So instead, we do one last round of Newton-Raphson, which has better
   * rounding characteristics, to get the final rounding correct. This is
   * split into two cases:
   *
   * 1. sqrt
   *
   * Normally, doing a round of Newton-Raphson for sqrt involves taking a
   * reciprocal of the original estimate, which is slow since it isn't
   * supported in HW. But we can take advantage of the fact that we already
   * computed a good estimate of 1/(2 * g_1) by rearranging it like so:
   *
   * g_2 = .5 * (g_1 + a / g_1)
   *     = g_1 + .5 * (a / g_1 - g_1)
   *     = g_1 + (.5 / g_1) * (a - g_1^2)
   *     = g_1 + h_1 * (a - g_1^2)
   *
   * The second term represents the error, and by splitting it out we can get
   * better precision by computing it as part of a fused multiply-add. Since
   * both Newton-Raphson and Goldschmit approximately double the precision of
   * the result, these two steps should be enough.
   *
   * 2. rsqrt
   *
   * First off, note that the first round of the Goldschmit algorithm is
   * really just a Newton-Raphson step in disguise:
   *
   * h_1 = h_0 * (.5 - h_0 * g_0) + h_0
   *     = h_0 * (1.5 - h_0 * g_0)
   *     = h_0 * (1.5 - .5 * a * y_0^2)
   *     = (.5 * y_0) * (1.5 - .5 * a * y_0^2)
   *
   * which is the standard formula multiplied by .5. Unlike in the sqrt case,
   * we don't need the inverse to do a Newton-Raphson step; we just need h_1,
   * so we can skip the calculation of g_1. Instead, we simply do another
   * Newton-Raphson step:
   *
   * y_1 = 2 * h_1
   * r_1 = .5 - h_1 * y_1 * a
   * y_2 = y_1 * r_1 + y_1
   *
   * Where the difference from Goldschmit is that we calculate y_1 * a
   * instead of using g_1. Doing it this way should be as fast as computing
   * y_1 up front instead of h_1, and it lets us share the code for the
   * initial Goldschmit step with the sqrt case.
   *
   * Putting it together, the computations are:
   *
   * h_0 = .5 * y_0
   * g_0 = a * y_0
   * r_0 = .5 - h_0 * g_0
   * h_1 = h_0 * r_0 + h_0
   * if sqrt:
   *    g_1 = g_0 * r_0 + g_0
   *    r_1 = a - g_1 * g_1
   *    g_2 = h_1 * r_1 + g_1
   * else:
   *    y_1 = 2 * h_1
   *    r_1 = .5 - y_1 * (h_1 * a)
   *    y_2 = y_1 * r_1 + y_1
   *
   * For more on the ideas behind this, see "Software Division and Square
   * Root Using Goldschmit's Algorithms" by Markstein and the Wikipedia page
   * on square roots
   * (https://en.wikipedia.org/wiki/Methods_of_computing_square_roots).
            nir_def *one_half = nir_imm_double(b, 0.5);
   nir_def *h_0 = nir_fmul(b, one_half, ra);
   nir_def *g_0 = nir_fmul(b, src, ra);
   nir_def *r_0 = nir_ffma(b, nir_fneg(b, h_0), g_0, one_half);
   nir_def *h_1 = nir_ffma(b, h_0, r_0, h_0);
   nir_def *res;
   if (sqrt) {
      nir_def *g_1 = nir_ffma(b, g_0, r_0, g_0);
   nir_def *r_1 = nir_ffma(b, nir_fneg(b, g_1), g_1, src);
      } else {
      nir_def *y_1 = nir_fmul_imm(b, h_1, 2.0);
   nir_def *r_1 = nir_ffma(b, nir_fneg(b, y_1), nir_fmul(b, h_1, src),
                     if (sqrt) {
      /* Here, the special cases we need to handle are
   * 0 -> 0 and
   * +inf -> +inf
   */
   const bool preserve_denorms =
      b->shader->info.float_controls_execution_mode &
      nir_def *src_flushed = src;
   if (!preserve_denorms) {
      src_flushed = nir_bcsel(b,
                  }
   res = nir_bcsel(b, nir_ior(b, nir_feq_imm(b, src_flushed, 0.0), nir_feq_imm(b, src, INFINITY)),
      } else {
                     }
      static nir_def *
   lower_trunc(nir_builder *b, nir_def *src)
   {
      nir_def *unbiased_exp = nir_iadd_imm(b, get_exponent(b, src),
                     /*
   * Decide the operation to apply depending on the unbiased exponent:
   *
   * if (unbiased_exp < 0)
   *    return 0
   * else if (unbiased_exp > 52)
   *    return src
   * else
   *    return src & (~0 << frac_bits)
   *
   * Notice that the else branch is a 64-bit integer operation that we need
   * to implement in terms of 32-bit integer arithmetics (at least until we
   * support 64-bit integer arithmetics).
            /* Compute "~0 << frac_bits" in terms of hi/lo 32-bit integer math */
   nir_def *mask_lo =
      nir_bcsel(b,
                     nir_def *mask_hi =
      nir_bcsel(b,
            nir_ilt_imm(b, frac_bits, 33),
   nir_imm_int(b, ~0),
            nir_def *src_lo = nir_unpack_64_2x32_split_x(b, src);
            return nir_bcsel(b,
                  nir_ilt_imm(b, unbiased_exp, 0),
   nir_imm_double(b, 0.0),
   nir_bcsel(b, nir_ige_imm(b, unbiased_exp, 53),
         }
      static nir_def *
   lower_floor(nir_builder *b, nir_def *src)
   {
      /*
   * For x >= 0, floor(x) = trunc(x)
   * For x < 0,
   *    - if x is integer, floor(x) = x
   *    - otherwise, floor(x) = trunc(x) - 1
   */
   nir_def *tr = nir_ftrunc(b, src);
   nir_def *positive = nir_fge_imm(b, src, 0.0);
   return nir_bcsel(b,
                  }
      static nir_def *
   lower_ceil(nir_builder *b, nir_def *src)
   {
      /* if x < 0,                    ceil(x) = trunc(x)
   * else if (x - trunc(x) == 0), ceil(x) = x
   * else,                        ceil(x) = trunc(x) + 1
   */
   nir_def *tr = nir_ftrunc(b, src);
   nir_def *negative = nir_flt_imm(b, src, 0.0);
   return nir_bcsel(b,
                  }
      static nir_def *
   lower_fract(nir_builder *b, nir_def *src)
   {
         }
      static nir_def *
   lower_round_even(nir_builder *b, nir_def *src)
   {
      /* Add and subtract 2**52 to round off any fractional bits. */
   nir_def *two52 = nir_imm_double(b, (double)(1ull << 52));
   nir_def *sign = nir_iand_imm(b, nir_unpack_64_2x32_split_y(b, src),
            b->exact = true;
   nir_def *res = nir_fsub(b, nir_fadd(b, nir_fabs(b, src), two52), two52);
            return nir_bcsel(b, nir_flt(b, nir_fabs(b, src), two52),
                  }
      static nir_def *
   lower_mod(nir_builder *b, nir_def *src0, nir_def *src1)
   {
      /* mod(x,y) = x - y * floor(x/y)
   *
   * If the division is lowered, it could add some rounding errors that make
   * floor() to return the quotient minus one when x = N * y. If this is the
   * case, we should return zero because mod(x, y) output value is [0, y).
   * But fortunately Vulkan spec allows this kind of errors; from Vulkan
   * spec, appendix A (Precision and Operation of SPIR-V instructions:
   *
   *   "The OpFRem and OpFMod instructions use cheap approximations of
   *   remainder, and the error can be large due to the discontinuity in
   *   trunc() and floor(). This can produce mathematically unexpected
   *   results in some cases, such as FMod(x,x) computing x rather than 0,
   *   and can also cause the result to have a different sign than the
   *   infinitely precise result."
   *
   * In practice this means the output value is actually in the interval
   * [0, y].
   *
   * While Vulkan states this behaviour explicitly, OpenGL does not, and thus
   * we need to assume that value should be in range [0, y); but on the other
   * hand, mod(a,b) is defined as "a - b * floor(a/b)" and OpenGL allows for
   * some error in division, so a/a could actually end up being 1.0 - 1ULP;
   * so in this case floor(a/a) would end up as 0, and hence mod(a,a) == a.
   *
   * In summary, in the practice mod(a,a) can be "a" both for OpenGL and
   * Vulkan.
   */
               }
      static nir_def *
   lower_doubles_instr_to_soft(nir_builder *b, nir_alu_instr *instr,
               {
      if (!(options & nir_lower_fp64_full_software))
            const char *name;
   const char *mangled_name;
            switch (instr->op) {
   case nir_op_f2i64:
      if (instr->src[0].src.ssa->bit_size != 64)
         name = "__fp64_to_int64";
   mangled_name = "__fp64_to_int64(u641;";
   return_type = glsl_int64_t_type();
      case nir_op_f2u64:
      if (instr->src[0].src.ssa->bit_size != 64)
         name = "__fp64_to_uint64";
   mangled_name = "__fp64_to_uint64(u641;";
      case nir_op_f2f64:
      name = "__fp32_to_fp64";
   mangled_name = "__fp32_to_fp64(f1;";
      case nir_op_f2f32:
      name = "__fp64_to_fp32";
   mangled_name = "__fp64_to_fp32(u641;";
   return_type = glsl_float_type();
      case nir_op_f2i32:
      name = "__fp64_to_int";
   mangled_name = "__fp64_to_int(u641;";
   return_type = glsl_int_type();
      case nir_op_f2u32:
      name = "__fp64_to_uint";
   mangled_name = "__fp64_to_uint(u641;";
   return_type = glsl_uint_type();
      case nir_op_b2f64:
      name = "__bool_to_fp64";
   mangled_name = "__bool_to_fp64(b1;";
      case nir_op_i2f64:
      if (instr->src[0].src.ssa->bit_size == 64) {
      name = "__int64_to_fp64";
      } else {
      name = "__int_to_fp64";
      }
      case nir_op_u2f64:
      if (instr->src[0].src.ssa->bit_size == 64) {
      name = "__uint64_to_fp64";
      } else {
      name = "__uint_to_fp64";
      }
      case nir_op_fabs:
      name = "__fabs64";
   mangled_name = "__fabs64(u641;";
      case nir_op_fneg:
      name = "__fneg64";
   mangled_name = "__fneg64(u641;";
      case nir_op_fround_even:
      name = "__fround64";
   mangled_name = "__fround64(u641;";
      case nir_op_ftrunc:
      name = "__ftrunc64";
   mangled_name = "__ftrunc64(u641;";
      case nir_op_ffloor:
      name = "__ffloor64";
   mangled_name = "__ffloor64(u641;";
      case nir_op_ffract:
      name = "__ffract64";
   mangled_name = "__ffract64(u641;";
      case nir_op_fsign:
      name = "__fsign64";
   mangled_name = "__fsign64(u641;";
      case nir_op_feq:
      name = "__feq64";
   mangled_name = "__feq64(u641;u641;";
   return_type = glsl_bool_type();
      case nir_op_fneu:
      name = "__fneu64";
   mangled_name = "__fneu64(u641;u641;";
   return_type = glsl_bool_type();
      case nir_op_flt:
      name = "__flt64";
   mangled_name = "__flt64(u641;u641;";
   return_type = glsl_bool_type();
      case nir_op_fge:
      name = "__fge64";
   mangled_name = "__fge64(u641;u641;";
   return_type = glsl_bool_type();
      case nir_op_fmin:
      name = "__fmin64";
   mangled_name = "__fmin64(u641;u641;";
      case nir_op_fmax:
      name = "__fmax64";
   mangled_name = "__fmax64(u641;u641;";
      case nir_op_fadd:
      name = "__fadd64";
   mangled_name = "__fadd64(u641;u641;";
      case nir_op_fmul:
      name = "__fmul64";
   mangled_name = "__fmul64(u641;u641;";
      case nir_op_ffma:
      name = "__ffma64";
   mangled_name = "__ffma64(u641;u641;u641;";
      case nir_op_fsat:
      name = "__fsat64";
   mangled_name = "__fsat64(u641;";
      case nir_op_fisfinite:
      name = "__fisfinite64";
   mangled_name = "__fisfinite64(u641;";
   return_type = glsl_bool_type();
      default:
                  assert(softfp64 != NULL);
            /* Another attempt, but this time with mangled names if softfp64
   * shader is taken from SPIR-V.
   */
   if (!func)
            if (!func || !func->impl) {
      fprintf(stderr, "Cannot find function \"%s\"\n", name);
               nir_def *params[4] = {
                  nir_variable *ret_tmp =
         nir_deref_instr *ret_deref = nir_build_deref_var(b, ret_tmp);
            assert(nir_op_infos[instr->op].num_inputs + 1 == func->num_params);
   for (unsigned i = 0; i < nir_op_infos[instr->op].num_inputs; i++) {
      assert(i + 1 < ARRAY_SIZE(params));
                           }
      nir_lower_doubles_options
   nir_lower_doubles_op_to_options_mask(nir_op opcode)
   {
      switch (opcode) {
   case nir_op_frcp:
         case nir_op_fsqrt:
         case nir_op_frsq:
         case nir_op_ftrunc:
         case nir_op_ffloor:
         case nir_op_fceil:
         case nir_op_ffract:
         case nir_op_fround_even:
         case nir_op_fmod:
         case nir_op_fsub:
         case nir_op_fdiv:
         default:
            }
      struct lower_doubles_data {
      const nir_shader *softfp64;
      };
      static bool
   should_lower_double_instr(const nir_instr *instr, const void *_data)
   {
      const struct lower_doubles_data *data = _data;
            if (instr->type != nir_instr_type_alu)
                              unsigned num_srcs = nir_op_infos[alu->op].num_inputs;
   for (unsigned i = 0; i < num_srcs; i++) {
                  if (!is_64)
            if (options & nir_lower_fp64_full_software)
               }
      static nir_def *
   lower_doubles_instr(nir_builder *b, nir_instr *instr, void *_data)
   {
      const struct lower_doubles_data *data = _data;
   const nir_lower_doubles_options options = data->options;
            nir_def *soft_def =
         if (soft_def)
            if (!(options & nir_lower_doubles_op_to_options_mask(alu->op)))
            nir_def *src = nir_mov_alu(b, alu->src[0],
            switch (alu->op) {
   case nir_op_frcp:
         case nir_op_fsqrt:
         case nir_op_frsq:
         case nir_op_ftrunc:
         case nir_op_ffloor:
         case nir_op_fceil:
         case nir_op_ffract:
         case nir_op_fround_even:
            case nir_op_fdiv:
   case nir_op_fsub:
   case nir_op_fmod: {
      nir_def *src1 = nir_mov_alu(b, alu->src[1],
         switch (alu->op) {
   case nir_op_fdiv:
         case nir_op_fsub:
         case nir_op_fmod:
         default:
            }
   default:
            }
      static bool
   nir_lower_doubles_impl(nir_function_impl *impl,
               {
      struct lower_doubles_data data = {
      .softfp64 = softfp64,
               bool progress =
      nir_function_impl_lower_instructions(impl,
                     if (progress && (options & nir_lower_fp64_full_software)) {
      /* Indices are completely messed up now */
                     /* And we have deref casts we need to clean up thanks to function
   * inlining.
   */
      } else if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                     }
      bool
   nir_lower_doubles(nir_shader *shader,
               {
               nir_foreach_function_impl(impl, shader) {
                     }
