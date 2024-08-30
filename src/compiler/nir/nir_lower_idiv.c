   /*
   * Copyright © 2015 Red Hat
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
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "nir.h"
   #include "nir_builder.h"
      /* ported from LLVM's AMDGPUTargetLowering::LowerUDIVREM */
   static nir_def *
   emit_udiv(nir_builder *bld, nir_def *numer, nir_def *denom, bool modulo)
   {
      nir_def *rcp = nir_frcp(bld, nir_u2f32(bld, denom));
            nir_def *neg_rcp_times_denom =
                  /* Get initial estimate for quotient/remainder, then refine the estimate
   * in two iterations after */
   nir_def *quotient = nir_umul_high(bld, numer, rcp);
   nir_def *num_s_remainder = nir_imul(bld, quotient, denom);
            /* First refinement step */
   nir_def *remainder_ge_den = nir_uge(bld, remainder, denom);
   if (!modulo) {
      quotient = nir_bcsel(bld, remainder_ge_den,
      }
   remainder = nir_bcsel(bld, remainder_ge_den,
            /* Second refinement step */
   remainder_ge_den = nir_uge(bld, remainder, denom);
   if (modulo) {
      return nir_bcsel(bld, remainder_ge_den, nir_isub(bld, remainder, denom),
      } else {
      return nir_bcsel(bld, remainder_ge_den, nir_iadd_imm(bld, quotient, 1),
         }
      /* ported from LLVM's AMDGPUTargetLowering::LowerSDIVREM */
   static nir_def *
   emit_idiv(nir_builder *bld, nir_def *numer, nir_def *denom, nir_op op)
   {
      nir_def *lh_sign = nir_ilt_imm(bld, numer, 0);
            nir_def *lhs = nir_iabs(bld, numer);
            if (op == nir_op_idiv) {
      nir_def *d_sign = nir_ixor(bld, lh_sign, rh_sign);
   nir_def *res = emit_udiv(bld, lhs, rhs, false);
      } else {
      nir_def *res = emit_udiv(bld, lhs, rhs, true);
   res = nir_bcsel(bld, lh_sign, nir_ineg(bld, res), res);
   if (op == nir_op_imod) {
      nir_def *cond = nir_ieq_imm(bld, res, 0);
   cond = nir_ior(bld, nir_ieq(bld, lh_sign, rh_sign), cond);
      }
         }
      static nir_def *
   convert_instr_small(nir_builder *b, nir_op op,
               {
      unsigned sz = numer->bit_size;
   nir_alu_type int_type = nir_op_infos[op].output_type | sz;
            nir_def *p = nir_type_convert(b, numer, int_type, float_type, nir_rounding_mode_undef);
            /* Take 1/q but offset mantissa by 1 to correct for rounding. This is
   * needed for correct results and has been checked exhaustively for
   * all pairs of 16-bit integers */
            /* Divide by multiplying by adjusted reciprocal */
            /* Convert back to integer space with rounding inferred by type */
            /* Get remainder given the quotient */
   if (op == nir_op_umod || op == nir_op_imod || op == nir_op_irem)
            /* Adjust for sign, see constant folding definition */
   if (op == nir_op_imod) {
      nir_def *zero = nir_imm_zero(b, 1, sz);
   nir_def *diff_sign =
            nir_def *adjust = nir_iand(b, diff_sign, nir_ine(b, res, zero));
                  }
      static nir_def *
   lower_idiv(nir_builder *b, nir_instr *instr, void *_data)
   {
      const nir_lower_idiv_options *options = _data;
            nir_def *numer = nir_ssa_for_alu_src(b, alu, 0);
                     if (numer->bit_size < 32)
         else if (alu->op == nir_op_udiv || alu->op == nir_op_umod)
         else
      }
      static bool
   inst_is_idiv(const nir_instr *instr, UNUSED const void *_state)
   {
      if (instr->type != nir_instr_type_alu)
                     if (alu->def.bit_size > 32)
            switch (alu->op) {
   case nir_op_idiv:
   case nir_op_udiv:
   case nir_op_imod:
   case nir_op_umod:
   case nir_op_irem:
         default:
            }
      bool
   nir_lower_idiv(nir_shader *shader, const nir_lower_idiv_options *options)
   {
      return nir_shader_lower_instructions(shader,
                  }
