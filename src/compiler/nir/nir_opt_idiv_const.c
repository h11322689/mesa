   /*
   * Copyright © 2018 Intel Corporation
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
      #include "util/fast_idiv_by_const.h"
   #include "util/u_math.h"
   #include "nir.h"
   #include "nir_builder.h"
      static nir_def *
   build_udiv(nir_builder *b, nir_def *n, uint64_t d)
   {
      if (d == 0) {
         } else if (util_is_power_of_two_or_zero64(d)) {
         } else {
      struct util_fast_udiv_info m =
            if (m.pre_shift)
         if (m.increment)
         n = nir_umul_high(b, n, nir_imm_intN_t(b, m.multiplier, n->bit_size));
   if (m.post_shift)
                  }
      static nir_def *
   build_umod(nir_builder *b, nir_def *n, uint64_t d)
   {
      if (d == 0) {
         } else if (util_is_power_of_two_or_zero64(d)) {
         } else {
            }
      static nir_def *
   build_idiv(nir_builder *b, nir_def *n, int64_t d)
   {
      int64_t int_min = u_intN_min(n->bit_size);
   if (d == int_min)
                     if (d == 0) {
         } else if (d == 1) {
         } else if (d == -1) {
         } else if (util_is_power_of_two_or_zero64(abs_d)) {
      nir_def *uq = nir_ushr_imm(b, nir_iabs(b, n), util_logbase2_64(abs_d));
   nir_def *n_neg = nir_ilt_imm(b, n, 0);
   nir_def *neg = d < 0 ? nir_inot(b, n_neg) : n_neg;
      } else {
      struct util_fast_sdiv_info m =
            nir_def *res =
         if (d > 0 && m.multiplier < 0)
         if (d < 0 && m.multiplier > 0)
         if (m.shift)
                        }
      static nir_def *
   build_irem(nir_builder *b, nir_def *n, int64_t d)
   {
      int64_t int_min = u_intN_min(n->bit_size);
   if (d == 0) {
         } else if (d == int_min) {
         } else {
      d = d < 0 ? -d : d;
   if (util_is_power_of_two_or_zero64(d)) {
      nir_def *tmp = nir_bcsel(b, nir_ilt_imm(b, n, 0),
            } else {
               }
      static nir_def *
   build_imod(nir_builder *b, nir_def *n, int64_t d)
   {
      int64_t int_min = u_intN_min(n->bit_size);
   if (d == 0) {
         } else if (d == int_min) {
      nir_def *int_min_def = nir_imm_intN_t(b, int_min, n->bit_size);
   nir_def *is_neg_not_int_min = nir_ult(b, int_min_def, n);
   nir_def *is_zero = nir_ieq_imm(b, n, 0);
      } else if (d > 0 && util_is_power_of_two_or_zero64(d)) {
         } else if (d < 0 && util_is_power_of_two_or_zero64(-d)) {
      nir_def *d_def = nir_imm_intN_t(b, d, n->bit_size);
   nir_def *res = nir_ior(b, n, d_def);
      } else {
      nir_def *rem = build_irem(b, n, d);
   nir_def *zero = nir_imm_intN_t(b, 0, n->bit_size);
   nir_def *sign_same = d < 0 ? nir_ilt(b, n, zero) : nir_ige(b, n, zero);
   nir_def *rem_zero = nir_ieq(b, rem, zero);
         }
      static bool
   nir_opt_idiv_const_instr(nir_builder *b, nir_instr *instr, void *user_data)
   {
               if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (alu->op != nir_op_udiv &&
      alu->op != nir_op_idiv &&
   alu->op != nir_op_umod &&
   alu->op != nir_op_imod &&
   alu->op != nir_op_irem)
         if (alu->def.bit_size < *min_bit_size)
            if (!nir_src_is_const(alu->src[1].src))
                              nir_def *q[NIR_MAX_VEC_COMPONENTS];
   for (unsigned comp = 0; comp < alu->def.num_components; comp++) {
      /* Get the numerator for the channel */
   nir_def *n = nir_channel(b, alu->src[0].src.ssa,
            /* Get the denominator for the channel */
   int64_t d = nir_src_comp_as_int(alu->src[1].src,
            nir_alu_type d_type = nir_op_infos[alu->op].input_types[1];
   if (nir_alu_type_get_base_type(d_type) == nir_type_uint) {
      /* The code above sign-extended.  If we're lowering an unsigned op,
   * we need to mask it off to the correct number of bits so that a
   * cast to uint64_t will do the right thing.
   */
   if (bit_size < 64)
               switch (alu->op) {
   case nir_op_udiv:
      q[comp] = build_udiv(b, n, d);
      case nir_op_idiv:
      q[comp] = build_idiv(b, n, d);
      case nir_op_umod:
      q[comp] = build_umod(b, n, d);
      case nir_op_imod:
      q[comp] = build_imod(b, n, d);
      case nir_op_irem:
      q[comp] = build_irem(b, n, d);
      default:
                     nir_def *qvec = nir_vec(b, q, alu->def.num_components);
   nir_def_rewrite_uses(&alu->def, qvec);
               }
      bool
   nir_opt_idiv_const(nir_shader *shader, unsigned min_bit_size)
   {
      return nir_shader_instructions_pass(shader, nir_opt_idiv_const_instr,
                  }
