   /*
   * Copyright © 2023 Valve Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "nir_test.h"
      namespace {
      class nir_lower_alu_width_test : public nir_test {
   protected:
      nir_lower_alu_width_test()
         {
      };
      TEST_F(nir_lower_alu_width_test, fdot_order)
   {
                        /* If this isn't done in xyz order, it evaluates to infinity. */
   nir_def *val = nir_fdot(
      b, nir_imm_vec3(b, 1.7014118346046923e+38, 1.7014118346046923e+38, 8.507059173023462e+37),
      nir_intrinsic_instr *store =
            nir_lower_alu_width(b->shader, NULL, NULL);
            ASSERT_TRUE(nir_src_is_const(store->src[1]));
      }
      }
