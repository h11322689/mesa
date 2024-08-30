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
   */
      #include <math.h>
      #include "nir/nir_builtin_builder.h"
      #include "vtn_private.h"
   #include "GLSL.std.450.h"
      #ifndef M_PIf
   #define M_PIf   ((float) M_PI)
   #endif
   #ifndef M_PI_2f
   #define M_PI_2f ((float) M_PI_2)
   #endif
   #ifndef M_PI_4f
   #define M_PI_4f ((float) M_PI_4)
   #endif
      static nir_def *build_det(nir_builder *b, nir_def **col, unsigned cols);
      /* Computes the determinate of the submatrix given by taking src and
   * removing the specified row and column.
   */
   static nir_def *
   build_mat_subdet(struct nir_builder *b, struct nir_def **src,
         {
      assert(row < size && col < size);
   if (size == 2) {
         } else {
      /* Swizzle to get all but the specified row */
   unsigned swiz[NIR_MAX_VEC_COMPONENTS] = {0};
   for (unsigned j = 0; j < 3; j++)
            /* Grab all but the specified column */
   nir_def *subcol[3];
   for (unsigned j = 0; j < size; j++) {
      if (j != col) {
                           }
      static nir_def *
   build_det(nir_builder *b, nir_def **col, unsigned size)
   {
      assert(size <= 4);
   nir_def *subdet[4];
   for (unsigned i = 0; i < size; i++)
                     nir_def *result = NULL;
   for (unsigned i = 0; i < size; i += 2) {
      nir_def *term;
   if (i + 1 < size) {
      term = nir_fsub(b, nir_channel(b, prod, i),
      } else {
                                 }
      static nir_def *
   build_mat_det(struct vtn_builder *b, struct vtn_ssa_value *src)
   {
               nir_def *cols[4];
   for (unsigned i = 0; i < size; i++)
               }
      static struct vtn_ssa_value *
   matrix_inverse(struct vtn_builder *b, struct vtn_ssa_value *src)
   {
      nir_def *adj_col[4];
            nir_def *cols[4];
   for (unsigned i = 0; i < size; i++)
            /* Build up an adjugate matrix */
   for (unsigned c = 0; c < size; c++) {
      nir_def *elem[4];
   for (unsigned r = 0; r < size; r++) {
               if ((r + c) % 2)
                                    struct vtn_ssa_value *val = vtn_create_ssa_value(b, src->type);
   for (unsigned i = 0; i < size; i++)
               }
      /**
   * Approximate asin(x) by the piecewise formula:
   * for |x| < 0.5, asin~(x) = x * (1 + x²(pS0 + x²(pS1 + x²*pS2)) / (1 + x²*qS1))
   * for |x| ≥ 0.5, asin~(x) = sign(x) * (π/2 - sqrt(1 - |x|) * (π/2 + |x|(π/4 - 1 + |x|(p0 + |x|p1))))
   *
   * The latter is correct to first order at x=0 and x=±1 regardless of the p
   * coefficients but can be made second-order correct at both ends by selecting
   * the fit coefficients appropriately.  Different p coefficients can be used
   * in the asin and acos implementation to minimize some relative error metric
   * in each case.
   */
   static nir_def *
   build_asin(nir_builder *b, nir_def *x, float p0, float p1, bool piecewise)
   {
      if (x->bit_size == 16) {
      /* The polynomial approximation isn't precise enough to meet half-float
   * precision requirements. Alternatively, we could implement this using
   * the formula:
   *
   * asin(x) = atan2(x, sqrt(1 - x*x))
   *
   * But that is very expensive, so instead we just do the polynomial
   * approximation in 32-bit math and then we convert the result back to
   * 16-bit.
   */
      }
   nir_def *one = nir_imm_floatN_t(b, 1.0f, x->bit_size);
   nir_def *half = nir_imm_floatN_t(b, 0.5f, x->bit_size);
                     nir_def *expr_tail =
      nir_ffma_imm2(b, abs_x,
               nir_def *result0 = nir_fmul(b, nir_fsign(b, x),
                     if (piecewise) {
      /* approximation for |x| < 0.5 */
   const float pS0 =  1.6666586697e-01f;
   const float pS1 = -4.2743422091e-02f;
   const float pS2 = -8.6563630030e-03f;
            nir_def *x2 = nir_fmul(b, x, x);
   nir_def *p = nir_fmul(b,
                              nir_def *q = nir_ffma_imm1(b, x2, qS1, one);
   nir_def *result1 = nir_ffma(b, x, nir_fdiv(b, p, q), x);
      } else {
            }
      static nir_op
   vtn_nir_alu_op_for_spirv_glsl_opcode(struct vtn_builder *b,
                     {
      *exact = false;
   switch (opcode) {
   case GLSLstd450Round:         return nir_op_fround_even;
   case GLSLstd450RoundEven:     return nir_op_fround_even;
   case GLSLstd450Trunc:         return nir_op_ftrunc;
   case GLSLstd450FAbs:          return nir_op_fabs;
   case GLSLstd450SAbs:          return nir_op_iabs;
   case GLSLstd450FSign:         return nir_op_fsign;
   case GLSLstd450SSign:         return nir_op_isign;
   case GLSLstd450Floor:         return nir_op_ffloor;
   case GLSLstd450Ceil:          return nir_op_fceil;
   case GLSLstd450Fract:         return nir_op_ffract;
   case GLSLstd450Sin:           return nir_op_fsin;
   case GLSLstd450Cos:           return nir_op_fcos;
   case GLSLstd450Pow:           return nir_op_fpow;
   case GLSLstd450Exp2:          return nir_op_fexp2;
   case GLSLstd450Log2:          return nir_op_flog2;
   case GLSLstd450Sqrt:          return nir_op_fsqrt;
   case GLSLstd450InverseSqrt:   return nir_op_frsq;
   case GLSLstd450NMin:          *exact = true; return nir_op_fmin;
   case GLSLstd450FMin:          return nir_op_fmin;
   case GLSLstd450UMin:          return nir_op_umin;
   case GLSLstd450SMin:          return nir_op_imin;
   case GLSLstd450NMax:          *exact = true; return nir_op_fmax;
   case GLSLstd450FMax:          return nir_op_fmax;
   case GLSLstd450UMax:          return nir_op_umax;
   case GLSLstd450SMax:          return nir_op_imax;
   case GLSLstd450FMix:          return nir_op_flrp;
   case GLSLstd450Fma:           return nir_op_ffma;
   case GLSLstd450Ldexp:         return nir_op_ldexp;
   case GLSLstd450FindILsb:      return nir_op_find_lsb;
   case GLSLstd450FindSMsb:      return nir_op_ifind_msb;
            /* Packing/Unpacking functions */
   case GLSLstd450PackSnorm4x8:     return nir_op_pack_snorm_4x8;
   case GLSLstd450PackUnorm4x8:     return nir_op_pack_unorm_4x8;
   case GLSLstd450PackSnorm2x16:    return nir_op_pack_snorm_2x16;
   case GLSLstd450PackUnorm2x16:    return nir_op_pack_unorm_2x16;
   case GLSLstd450PackHalf2x16:     return nir_op_pack_half_2x16;
   case GLSLstd450PackDouble2x32:   return nir_op_pack_64_2x32;
   case GLSLstd450UnpackSnorm4x8:   return nir_op_unpack_snorm_4x8;
   case GLSLstd450UnpackUnorm4x8:   return nir_op_unpack_unorm_4x8;
   case GLSLstd450UnpackSnorm2x16:  return nir_op_unpack_snorm_2x16;
   case GLSLstd450UnpackUnorm2x16:  return nir_op_unpack_unorm_2x16;
   case GLSLstd450UnpackHalf2x16:
      if (execution_mode & FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16)
         else
               default:
            }
      #define NIR_IMM_FP(n, v) (nir_imm_floatN_t(n, v, src[0]->bit_size))
      static void
   handle_glsl450_alu(struct vtn_builder *b, enum GLSLstd450 entrypoint,
         {
      struct nir_builder *nb = &b->nb;
   const struct glsl_type *dest_type = vtn_get_type(b, w[1])->type;
            bool mediump_16bit;
   switch (entrypoint) {
   case GLSLstd450PackSnorm4x8:
   case GLSLstd450PackUnorm4x8:
   case GLSLstd450PackSnorm2x16:
   case GLSLstd450PackUnorm2x16:
   case GLSLstd450PackHalf2x16:
   case GLSLstd450PackDouble2x32:
   case GLSLstd450UnpackSnorm4x8:
   case GLSLstd450UnpackUnorm4x8:
   case GLSLstd450UnpackSnorm2x16:
   case GLSLstd450UnpackUnorm2x16:
   case GLSLstd450UnpackHalf2x16:
   case GLSLstd450UnpackDouble2x32:
      /* Asking for relaxed precision snorm 4x8 pack results (for example)
   * doesn't even make sense.  The NIR opcodes have a fixed output size, so
   * no trying to reduce precision.
   */
   mediump_16bit = false;
         case GLSLstd450Frexp:
   case GLSLstd450FrexpStruct:
   case GLSLstd450Modf:
   case GLSLstd450ModfStruct:
      /* Not sure how to detect the ->elems[i] destinations on these in vtn_upconvert_value(). */
   mediump_16bit = false;
         default:
      mediump_16bit = b->options->mediump_16bit_alu && vtn_value_is_relaxed_precision(b, dest_val);
               /* Collect the various SSA sources */
   unsigned num_inputs = count - 5;
   nir_def *src[3] = { NULL, };
   for (unsigned i = 0; i < num_inputs; i++) {
      /* These are handled specially below */
   if (vtn_untyped_value(b, w[i + 5])->value_type == vtn_value_type_pointer)
            src[i] = vtn_get_nir_ssa(b, w[i + 5]);
   if (mediump_16bit) {
      struct vtn_ssa_value *vtn_src = vtn_ssa_value(b, w[i + 5]);
                           vtn_handle_no_contraction(b, vtn_untyped_value(b, w[2]));
   switch (entrypoint) {
   case GLSLstd450Radians:
      dest->def = nir_radians(nb, src[0]);
      case GLSLstd450Degrees:
      dest->def = nir_degrees(nb, src[0]);
      case GLSLstd450Tan:
      dest->def = nir_ftan(nb, src[0]);
         case GLSLstd450Modf: {
      nir_def *inf = nir_imm_floatN_t(&b->nb, INFINITY, src[0]->bit_size);
   nir_def *sign_bit =
      nir_imm_intN_t(&b->nb, (uint64_t)1 << (src[0]->bit_size - 1),
      nir_def *sign = nir_fsign(nb, src[0]);
            /* NaN input should produce a NaN results, and ±Inf input should provide
   * ±0 result.  The fmul(sign(x), ffract(x)) calculation will already
   * produce the expected NaN.  To get ±0, directly compare for equality
   * with Inf instead of using fisfinite (which is false for NaN).
   */
   dest->def = nir_bcsel(nb,
                        struct vtn_pointer *i_ptr = vtn_value(b, w[6], vtn_value_type_pointer)->pointer;
   struct vtn_ssa_value *whole = vtn_create_ssa_value(b, i_ptr->type->type);
   whole->def = nir_fmul(nb, sign, nir_ffloor(nb, abs));
   vtn_variable_store(b, whole, i_ptr, 0);
               case GLSLstd450ModfStruct: {
      nir_def *inf = nir_imm_floatN_t(&b->nb, INFINITY, src[0]->bit_size);
   nir_def *sign_bit =
      nir_imm_intN_t(&b->nb, (uint64_t)1 << (src[0]->bit_size - 1),
      nir_def *sign = nir_fsign(nb, src[0]);
   nir_def *abs = nir_fabs(nb, src[0]);
            /* See GLSLstd450Modf for explanation of the Inf and NaN handling. */
   dest->elems[0]->def = nir_bcsel(nb,
                     dest->elems[1]->def = nir_fmul(nb, sign, nir_ffloor(nb, abs));
               case GLSLstd450Step: {
      /* The SPIR-V Extended Instructions for GLSL spec says:
   *
   *    Result is 0.0 if x < edge; otherwise result is 1.0.
   *
   * Here src[1] is x, and src[0] is edge.  The direct implementation is
   *
   *    bcsel(src[1] < src[0], 0.0, 1.0)
   *
   * This is effectively b2f(!(src1 < src0)).  Previously this was
   * implemented using sge(src1, src0), but that produces incorrect
   * results for NaN.  Instead, we use the identity b2f(!x) = 1 - b2f(x).
   */
   const bool exact = nb->exact;
                     nb->exact = exact;
   dest->def = nir_fsub_imm(nb, 1.0f, cmp);
               case GLSLstd450Length:
      dest->def = nir_fast_length(nb, src[0]);
      case GLSLstd450Distance:
      dest->def = nir_fast_distance(nb, src[0], src[1]);
      case GLSLstd450Normalize:
      dest->def = nir_fast_normalize(nb, src[0]);
         case GLSLstd450Exp:
      dest->def = nir_fexp(nb, src[0]);
         case GLSLstd450Log:
      dest->def = nir_flog(nb, src[0]);
         case GLSLstd450FClamp:
      dest->def = nir_fclamp(nb, src[0], src[1], src[2]);
      case GLSLstd450NClamp:
      nb->exact = true;
   dest->def = nir_fclamp(nb, src[0], src[1], src[2]);
   nb->exact = false;
      case GLSLstd450UClamp:
      dest->def = nir_uclamp(nb, src[0], src[1], src[2]);
      case GLSLstd450SClamp:
      dest->def = nir_iclamp(nb, src[0], src[1], src[2]);
         case GLSLstd450Cross: {
      dest->def = nir_cross3(nb, src[0], src[1]);
               case GLSLstd450SmoothStep: {
      dest->def = nir_smoothstep(nb, src[0], src[1], src[2]);
               case GLSLstd450FaceForward:
      dest->def =
      nir_bcsel(nb, nir_flt(nb, nir_fdot(nb, src[2], src[1]),
                  case GLSLstd450Reflect:
      /* I - 2 * dot(N, I) * N */
   dest->def =
      nir_a_minus_bc(nb, src[0],
                        case GLSLstd450Refract: {
      nir_def *I = src[0];
   nir_def *N = src[1];
   nir_def *eta = src[2];
   nir_def *n_dot_i = nir_fdot(nb, N, I);
   nir_def *one = NIR_IMM_FP(nb, 1.0);
   nir_def *zero = NIR_IMM_FP(nb, 0.0);
   /* According to the SPIR-V and GLSL specs, eta is always a float
   * regardless of the type of the other operands. However in practice it
   * seems that if you try to pass it a float then glslang will just
   * promote it to a double and generate invalid SPIR-V. In order to
   * support a hypothetical fixed version of glslang we’ll promote eta to
   * double if the other operands are double also.
   */
   if (I->bit_size != eta->bit_size) {
      eta = nir_type_convert(nb, eta, nir_type_float,
            }
   /* k = 1.0 - eta * eta * (1.0 - dot(N, I) * dot(N, I)) */
   nir_def *k =
      nir_a_minus_bc(nb, one, eta,
      nir_def *result =
      nir_a_minus_bc(nb, nir_fmul(nb, eta, I),
            /* XXX: bcsel, or if statement? */
   dest->def = nir_bcsel(nb, nir_flt(nb, k, zero), zero, result);
               case GLSLstd450Sinh:
      /* 0.5 * (e^x - e^(-x)) */
   dest->def =
      nir_fmul_imm(nb, nir_fsub(nb, nir_fexp(nb, src[0]),
                  case GLSLstd450Cosh:
      /* 0.5 * (e^x + e^(-x)) */
   dest->def =
      nir_fmul_imm(nb, nir_fadd(nb, nir_fexp(nb, src[0]),
                  case GLSLstd450Tanh: {
      /* tanh(x) := (e^x - e^(-x)) / (e^x + e^(-x))
   *
   * We clamp x to [-10, +10] to avoid precision problems.  When x > 10,
   * e^x dominates the sum, e^(-x) is lost and tanh(x) is 1.0 for 32 bit
   * floating point.
   *
   * For 16-bit precision this we clamp x to [-4.2, +4.2].
   */
   const uint32_t bit_size = src[0]->bit_size;
   const double clamped_x = bit_size > 16 ? 10.0 : 4.2;
   nir_def *x = nir_fclamp(nb, src[0],
                  /* The clamping will filter out NaN values causing an incorrect result.
   * The comparison is carefully structured to get NaN result for NaN and
   * get -0 for -0.
   *
   *    result = abs(s) > 0.0 ? ... : s;
   */
            nb->exact = true;
   nir_def *is_regular = nir_flt(nb,
                  /* The extra 1.0*s ensures that subnormal inputs are flushed to zero
   * when that is selected by the shader.
   */
   nir_def *flushed = nir_fmul(nb,
                        dest->def = nir_bcsel(nb,
                        is_regular,
   nir_fdiv(nb, nir_fsub(nb, nir_fexp(nb, x),
                  case GLSLstd450Asinh:
      dest->def = nir_fmul(nb, nir_fsign(nb, src[0]),
      nir_flog(nb, nir_fadd(nb, nir_fabs(nb, src[0]),
         case GLSLstd450Acosh:
      dest->def = nir_flog(nb, nir_fadd(nb, src[0],
            case GLSLstd450Atanh: {
      dest->def =
      nir_fmul_imm(nb, nir_flog(nb, nir_fdiv(nb, nir_fadd_imm(nb, src[0], 1.0),
                        case GLSLstd450Asin:
      dest->def = build_asin(nb, src[0], 0.086566724, -0.03102955, true);
         case GLSLstd450Acos:
      dest->def =
      nir_fsub_imm(nb, M_PI_2f,
            case GLSLstd450Atan:
      dest->def = nir_atan(nb, src[0]);
         case GLSLstd450Atan2:
      dest->def = nir_atan2(nb, src[0], src[1]);
         case GLSLstd450Frexp: {
               struct vtn_pointer *i_ptr = vtn_value(b, w[6], vtn_value_type_pointer)->pointer;
   struct vtn_ssa_value *exp = vtn_create_ssa_value(b, i_ptr->type->type);
   exp->def = nir_frexp_exp(nb, src[0]);
   vtn_variable_store(b, exp, i_ptr, 0);
               case GLSLstd450FrexpStruct: {
      vtn_assert(glsl_type_is_struct_or_ifc(dest_type));
   dest->elems[0]->def = nir_frexp_sig(nb, src[0]);
   dest->elems[1]->def = nir_frexp_exp(nb, src[0]);
               default: {
      unsigned execution_mode =
         bool exact;
   nir_op op = vtn_nir_alu_op_for_spirv_glsl_opcode(b, entrypoint, execution_mode, &exact);
   /* don't override explicit decoration */
   b->nb.exact |= exact;
   dest->def = nir_build_alu(&b->nb, op, src[0], src[1], src[2], NULL);
      }
   }
            if (mediump_16bit)
               }
      static void
   handle_glsl450_interpolation(struct vtn_builder *b, enum GLSLstd450 opcode,
         {
      nir_intrinsic_op op;
   switch (opcode) {
   case GLSLstd450InterpolateAtCentroid:
      op = nir_intrinsic_interp_deref_at_centroid;
      case GLSLstd450InterpolateAtSample:
      op = nir_intrinsic_interp_deref_at_sample;
      case GLSLstd450InterpolateAtOffset:
      op = nir_intrinsic_interp_deref_at_offset;
      default:
                           struct vtn_pointer *ptr =
                  /* If the value we are interpolating has an index into a vector then
   * interpolate the vector and index the result of that instead. This is
   * necessary because the index will get generated as a series of nir_bcsel
   * instructions so it would no longer be an input variable.
   */
   const bool vec_array_deref = deref->deref_type == nir_deref_type_array &&
            nir_deref_instr *vec_deref = NULL;
   if (vec_array_deref) {
      vec_deref = deref;
      }
            switch (opcode) {
   case GLSLstd450InterpolateAtCentroid:
         case GLSLstd450InterpolateAtSample:
   case GLSLstd450InterpolateAtOffset:
      intrin->src[1] = nir_src_for_ssa(vtn_get_nir_ssa(b, w[6]));
      default:
                  intrin->num_components = glsl_get_vector_elements(deref->type);
   nir_def_init(&intrin->instr, &intrin->def,
                           nir_def *def = &intrin->def;
   if (vec_array_deref)
               }
      bool
   vtn_handle_glsl450_instruction(struct vtn_builder *b, SpvOp ext_opcode,
         {
      switch ((enum GLSLstd450)ext_opcode) {
   case GLSLstd450Determinant: {
      vtn_push_nir_ssa(b, w[2], build_mat_det(b, vtn_ssa_value(b, w[5])));
               case GLSLstd450MatrixInverse: {
      vtn_push_ssa_value(b, w[2], matrix_inverse(b, vtn_ssa_value(b, w[5])));
               case GLSLstd450InterpolateAtCentroid:
   case GLSLstd450InterpolateAtSample:
   case GLSLstd450InterpolateAtOffset:
      handle_glsl450_interpolation(b, (enum GLSLstd450)ext_opcode, w, count);
         default:
                     }
