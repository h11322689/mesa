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
   #include <math.h>
   #include "util/u_vector.h"
   #include "nir.h"
   #include "nir_builder.h"
      /**
   * Lower flrp instructions.
   *
   * Unlike the lowerings that are possible in nir_opt_algrbraic, this pass can
   * examine more global information to determine a possibly more efficient
   * lowering for each flrp.
   */
      static void
   append_flrp_to_dead_list(struct u_vector *dead_flrp, struct nir_alu_instr *alu)
   {
      struct nir_alu_instr **tail = u_vector_add(dead_flrp);
      }
      /**
   * Replace flrp(a, b, c) with ffma(b, c, ffma(-a, c, a)).
   */
   static void
   replace_with_strict_ffma(struct nir_builder *bld, struct u_vector *dead_flrp,
         {
      nir_def *const a = nir_ssa_for_alu_src(bld, alu, 0);
   nir_def *const b = nir_ssa_for_alu_src(bld, alu, 1);
            nir_def *const neg_a = nir_fneg(bld, a);
            nir_def *const inner_ffma = nir_ffma(bld, neg_a, c, a);
            nir_def *const outer_ffma = nir_ffma(bld, b, c, inner_ffma);
                     /* DO NOT REMOVE the original flrp yet.  Many of the lowering choices are
   * based on other uses of the sources.  Removing the flrp may cause the
   * last flrp in a sequence to make a different, incorrect choice.
   */
      }
      /**
   * Replace flrp(a, b, c) with ffma(a, (1 - c), bc)
   */
   static void
   replace_with_single_ffma(struct nir_builder *bld, struct u_vector *dead_flrp,
         {
      nir_def *const a = nir_ssa_for_alu_src(bld, alu, 0);
   nir_def *const b = nir_ssa_for_alu_src(bld, alu, 1);
            nir_def *const neg_c = nir_fneg(bld, c);
            nir_def *const one_minus_c =
                  nir_def *const b_times_c = nir_fmul(bld, b, c);
            nir_def *const final_ffma = nir_ffma(bld, a, one_minus_c, b_times_c);
                     /* DO NOT REMOVE the original flrp yet.  Many of the lowering choices are
   * based on other uses of the sources.  Removing the flrp may cause the
   * last flrp in a sequence to make a different, incorrect choice.
   */
      }
      /**
   * Replace flrp(a, b, c) with a(1-c) + bc.
   */
   static void
   replace_with_strict(struct nir_builder *bld, struct u_vector *dead_flrp,
         {
      nir_def *const a = nir_ssa_for_alu_src(bld, alu, 0);
   nir_def *const b = nir_ssa_for_alu_src(bld, alu, 1);
            nir_def *const neg_c = nir_fneg(bld, c);
            nir_def *const one_minus_c =
                  nir_def *const first_product = nir_fmul(bld, a, one_minus_c);
            nir_def *const second_product = nir_fmul(bld, b, c);
            nir_def *const sum = nir_fadd(bld, first_product, second_product);
                     /* DO NOT REMOVE the original flrp yet.  Many of the lowering choices are
   * based on other uses of the sources.  Removing the flrp may cause the
   * last flrp in a sequence to make a different, incorrect choice.
   */
      }
      /**
   * Replace flrp(a, b, c) with a + c(b-a).
   */
   static void
   replace_with_fast(struct nir_builder *bld, struct u_vector *dead_flrp,
         {
      nir_def *const a = nir_ssa_for_alu_src(bld, alu, 0);
   nir_def *const b = nir_ssa_for_alu_src(bld, alu, 1);
            nir_def *const neg_a = nir_fneg(bld, a);
            nir_def *const b_minus_a = nir_fadd(bld, b, neg_a);
            nir_def *const product = nir_fmul(bld, c, b_minus_a);
            nir_def *const sum = nir_fadd(bld, a, product);
                     /* DO NOT REMOVE the original flrp yet.  Many of the lowering choices are
   * based on other uses of the sources.  Removing the flrp may cause the
   * last flrp in a sequence to make a different, incorrect choice.
   */
      }
      /**
   * Replace flrp(a, b, c) with (b*c ± c) + a => b*c + (a ± c)
   *
   * \note: This only works if a = ±1.
   */
   static void
   replace_with_expanded_ffma_and_add(struct nir_builder *bld,
               {
      nir_def *const a = nir_ssa_for_alu_src(bld, alu, 0);
   nir_def *const b = nir_ssa_for_alu_src(bld, alu, 1);
            nir_def *const b_times_c = nir_fmul(bld, b, c);
                     if (subtract_c) {
      nir_def *const neg_c = nir_fneg(bld, c);
               } else {
                           nir_def *const outer_sum = nir_fadd(bld, inner_sum, b_times_c);
                     /* DO NOT REMOVE the original flrp yet.  Many of the lowering choices are
   * based on other uses of the sources.  Removing the flrp may cause the
   * last flrp in a sequence to make a different, incorrect choice.
   */
      }
      /**
   * Determines whether a swizzled source is constant w/ all components the same.
   *
   * The value of the constant is stored in \c result.
   *
   * \return
   * True if all components of the swizzled source are the same constant.
   * Otherwise false is returned.
   */
   static bool
   all_same_constant(const nir_alu_instr *instr, unsigned src, double *result)
   {
               if (!val)
            const uint8_t *const swizzle = instr->src[src].swizzle;
            if (instr->def.bit_size == 32) {
               for (unsigned i = 1; i < num_components; i++) {
      if (val[swizzle[i]].f32 != first)
                  } else {
               for (unsigned i = 1; i < num_components; i++) {
      if (val[swizzle[i]].f64 != first)
                              }
      static bool
   sources_are_constants_with_similar_magnitudes(const nir_alu_instr *instr)
   {
      nir_const_value *val0 = nir_src_as_const_value(instr->src[0].src);
            if (val0 == NULL || val1 == NULL)
            const uint8_t *const swizzle0 = instr->src[0].swizzle;
   const uint8_t *const swizzle1 = instr->src[1].swizzle;
            if (instr->def.bit_size == 32) {
      for (unsigned i = 0; i < num_components; i++) {
                                    /* If the difference between exponents is >= 24, then A+B will always
   * have the value whichever between A and B has the largest absolute
   * value.  So, [0, 23] is the valid range.  The smaller the limit
   * value, the more precision will be maintained at a potential
   * performance cost.  Somewhat arbitrarilly split the range in half.
   */
   if (abs(exp0 - exp1) > (23 / 2))
         } else {
      for (unsigned i = 0; i < num_components; i++) {
                                    /* If the difference between exponents is >= 53, then A+B will always
   * have the value whichever between A and B has the largest absolute
   * value.  So, [0, 52] is the valid range.  The smaller the limit
   * value, the more precision will be maintained at a potential
   * performance cost.  Somewhat arbitrarilly split the range in half.
   */
   if (abs(exp0 - exp1) > (52 / 2))
                     }
      /**
   * Counts of similar types of nir_op_flrp instructions
   *
   * If a similar instruction fits into more than one category, it will only be
   * counted once.  The assumption is that no other instruction will have all
   * sources the same, or CSE would have removed one of the instructions.
   */
   struct similar_flrp_stats {
      unsigned src2;
   unsigned src0_and_src2;
      };
      /**
   * Collection counts of similar FLRP instructions.
   *
   * This function only cares about similar instructions that have src2 in
   * common.
   */
   static void
   get_similar_flrp_stats(nir_alu_instr *alu, struct similar_flrp_stats *st)
   {
               nir_foreach_use(other_use, alu->src[2].src.ssa) {
      /* Is the use also a flrp? */
   nir_instr *const other_instr = nir_src_parent_instr(other_use);
   if (other_instr->type != nir_instr_type_alu)
            /* Eh-hem... don't match the instruction with itself. */
   if (other_instr == &alu->instr)
            nir_alu_instr *const other_alu = nir_instr_as_alu(other_instr);
   if (other_alu->op != nir_op_flrp)
            /* Does the other flrp use source 2 from the first flrp as its source 2
   * as well?
   */
   if (!nir_alu_srcs_equal(alu, other_alu, 2, 2))
            if (nir_alu_srcs_equal(alu, other_alu, 0, 0))
         else if (nir_alu_srcs_equal(alu, other_alu, 1, 1))
         else
         }
      static void
   convert_flrp_instruction(nir_builder *bld,
                     {
      bool have_ffma = false;
            if (bit_size == 16)
         else if (bit_size == 32)
         else if (bit_size == 64)
         else
                     /* There are two methods to implement flrp(x, y, t).  The strictly correct
   * implementation according to the GLSL spec is:
   *
   *    x(1 - t) + yt
   *
   * This can also be implemented using two chained FMAs
   *
   *    fma(y, t, fma(-x, t, x))
   *
   * This method, using either formulation, has better precision when the
   * difference between x and y is very large.  It guarantess that flrp(x, y,
   * 1) = y.  For example, flrp(1e38, 1.0, 1.0) is 1.0.  This is correct.
   *
   * The other possible implementation is:
   *
   *    x + t(y - x)
   *
   * This can also be formuated as an FMA:
   *
   *    fma(y - x, t, x)
   *
   * For this implementation, flrp(1e38, 1.0, 1.0) is 0.0.  Since 1.0 was
   * expected, that's a pretty significant error.
   *
   * The choice made for lowering depends on a number of factors.
   *
   * - If the flrp is marked precise and FMA is supported:
   *
   *        fma(y, t, fma(-x, t, x))
   *
   *   This is strictly correct (maybe?), and the cost is two FMA
   *   instructions.  It at least maintains the flrp(x, y, 1.0) == y
   *   condition.
   *
   * - If the flrp is marked precise and FMA is not supported:
   *
   *        x(1 - t) + yt
   *
   *   This is strictly correct, and the cost is 4 instructions.  If FMA is
   *   supported, this may or may not be reduced to 3 instructions (a
   *   subtract, a multiply, and an FMA)... but in that case the other
   *   formulation should have been used.
   */
   if (alu->exact) {
      if (have_ffma)
         else
                        /*
   * - If x and y are both immediates and the relative magnitude of the
   *   values is similar (such that x-y does not lose too much precision):
   *
   *        x + t(x - y)
   *
   *   We rely on constant folding to eliminate x-y, and we rely on
   *   nir_opt_algebraic to possibly generate an FMA.  The cost is either one
   *   FMA or two instructions.
   */
   if (sources_are_constants_with_similar_magnitudes(alu)) {
      replace_with_fast(bld, dead_flrp, alu);
               /*
   * - If x = 1:
   *
   *        (yt + -t) + 1
   *
   * - If x = -1:
   *
   *        (yt + t) - 1
   *
   *   In both cases, x is used in place of ±1 for simplicity.  Both forms
   *   lend to ffma generation on platforms that support ffma.
   */
   double src0_as_constant;
   if (all_same_constant(alu, 0, &src0_as_constant)) {
      if (src0_as_constant == 1.0) {
      replace_with_expanded_ffma_and_add(bld, dead_flrp, alu,
            } else if (src0_as_constant == -1.0) {
      replace_with_expanded_ffma_and_add(bld, dead_flrp, alu,
                        /*
   * - If y = ±1:
   *
   *        x(1 - t) + yt
   *
   *   In this case either the multiply in yt will be eliminated by
   *   nir_opt_algebraic.  If FMA is supported, this results in fma(x, (1 -
   *   t), ±t) for two instructions.  If FMA is not supported, then the cost
   *   is 3 instructions.  We rely on nir_opt_algebraic to generate the FMA
   *   instructions as well.
   *
   *   Another possible replacement is
   *
   *        -xt + x ± t
   *
   *   Some groupings of this may be better on some platforms in some
   *   circumstances, bit it is probably dependent on scheduling.  Futher
   *   investigation may be required.
   */
   double src1_as_constant;
   if ((all_same_constant(alu, 1, &src1_as_constant) &&
      (src1_as_constant == -1.0 || src1_as_constant == 1.0))) {
   replace_with_strict(bld, dead_flrp, alu);
               if (have_ffma) {
      if (always_precise) {
      replace_with_strict_ffma(bld, dead_flrp, alu);
               /*
   * - If FMA is supported and other flrp(x, _, t) exists:
   *
   *        fma(y, t, fma(-x, t, x))
   *
   *   The hope is that the inner FMA calculation will be shared with the
   *   other lowered flrp.  This results in two FMA instructions for the
   *   first flrp and one FMA instruction for each additional flrp.  It
   *   also means that the live range for x might be complete after the
   *   inner ffma instead of after the last flrp.
   */
            get_similar_flrp_stats(alu, &st);
   if (st.src0_and_src2 > 0) {
      replace_with_strict_ffma(bld, dead_flrp, alu);
               /*
   * - If FMA is supported and another flrp(_, y, t) exists:
   *
   *        fma(x, (1 - t), yt)
   *
   *   The hope is that the (1 - t) and the yt will be shared with the
   *   other lowered flrp.  This results in 3 insructions for the first
   *   flrp and 1 for each additional flrp.
   */
   if (st.src1_and_src2 > 0) {
      replace_with_single_ffma(bld, dead_flrp, alu);
         } else {
      if (always_precise) {
      replace_with_strict(bld, dead_flrp, alu);
               /*
   * - If FMA is not supported and another flrp(x, _, t) exists:
   *
   *        x(1 - t) + yt
   *
   *   The hope is that the x(1 - t) will be shared with the other lowered
   *   flrp.  This results in 4 insructions for the first flrp and 2 for
   *   each additional flrp.
   *
   * - If FMA is not supported and another flrp(_, y, t) exists:
   *
   *        x(1 - t) + yt
   *
   *   The hope is that the (1 - t) and the yt will be shared with the
   *   other lowered flrp.  This results in 4 insructions for the first
   *   flrp and 2 for each additional flrp.
   */
            get_similar_flrp_stats(alu, &st);
   if (st.src0_and_src2 > 0 || st.src1_and_src2 > 0) {
      replace_with_strict(bld, dead_flrp, alu);
                  /*
   * - If t is constant:
   *
   *        x(1 - t) + yt
   *
   *   The cost is three instructions without FMA or two instructions with
   *   FMA.  This is the same cost as the imprecise lowering, but it gives
   *   the instruction scheduler a little more freedom.
   *
   *   There is no need to handle t = 0.5 specially.  nir_opt_algebraic
   *   already has optimizations to convert 0.5x + 0.5y to 0.5(x + y).
   */
   if (alu->src[2].src.ssa->parent_instr->type == nir_instr_type_load_const) {
      replace_with_strict(bld, dead_flrp, alu);
               /*
   * - Otherwise
   *
   *        x + t(x - y)
   */
      }
      static void
   lower_flrp_impl(nir_function_impl *impl,
                     {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                        if (alu->op == nir_op_flrp &&
      (alu->def.bit_size & lowering_mask)) {
                        nir_metadata_preserve(impl, nir_metadata_block_index |
      }
      /**
   * \param lowering_mask - Bitwise-or of the bit sizes that need to be lowered
   *                        (e.g., 16 | 64 if only 16-bit and 64-bit flrp need
   *                        lowering).
   * \param always_precise - Always require precise lowering for flrp.  This
   *                        will always lower flrp to (a * (1 - c)) + (b * c).
   * \param have_ffma - Set to true if the GPU has an FFMA instruction that
   *                    should be used.
   */
   bool
   nir_lower_flrp(nir_shader *shader,
               {
               if (!u_vector_init_pow2(&dead_flrp, 8, sizeof(struct nir_alu_instr *)))
            nir_foreach_function_impl(impl, shader) {
                  /* Progress was made if the dead list is not empty.  Remove all the
   * instructions from the dead list.
   */
            struct nir_alu_instr **instr;
   u_vector_foreach(instr, &dead_flrp)
                        }
