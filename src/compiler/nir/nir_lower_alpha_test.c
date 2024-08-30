   /*
   * Copyright © 2017 Broadcom
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
      /**
   * @file
   *
   * Implements GL alpha testing by comparing the output color's alpha to the
   * alpha_ref intrinsic and emitting a discard based on it.
   *
   * The alpha_to_one value overrides the source alpha to 1.0 to implement
   * GL_SAMPLE_ALPHA_TO_ONE, which applies before the alpha test (and would be
   * rather silly to use with alpha test, but the spec permits).
   */
      #include "nir/nir.h"
   #include "nir/nir_builder.h"
      void
   nir_lower_alpha_test(nir_shader *shader, enum compare_func func,
               {
      assert(alpha_ref_state_tokens);
            nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                                 switch (intr->intrinsic) {
   case nir_intrinsic_store_deref:
      out = nir_intrinsic_get_var(intr, 0);
      case nir_intrinsic_store_output:
      /* already had i/o lowered.. lookup the matching output var: */
   nir_foreach_shader_out_variable(var, shader) {
      int drvloc = var->data.driver_location;
   if (nir_intrinsic_base(intr) == drvloc) {
      out = var;
         }
   assume(out);
                                                                        nir_def *alpha;
   if (alpha_to_one) {
         } else if (intr->intrinsic == nir_intrinsic_store_deref) {
      alpha = nir_channel(&b, intr->src[1].ssa,
      } else {
                        nir_variable *var = nir_state_variable_create(shader, glsl_float_type(),
                                       nir_discard_if(&b, nir_inot(&b, condition));
                     nir_metadata_preserve(impl, nir_metadata_block_index |
         }
