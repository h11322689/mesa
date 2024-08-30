   /*
   * Copyright © 2014 Intel Corporation
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
      #include "nir_instr_set.h"
      /*
   * Implements common subexpression elimination
   */
      static bool
   dominates(const nir_instr *old_instr, const nir_instr *new_instr)
   {
         }
      static bool
   nir_opt_cse_impl(nir_function_impl *impl)
   {
                                 bool progress = false;
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block)
               if (progress) {
      nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                  nir_instr_set_destroy(instr_set);
      }
      bool
   nir_opt_cse(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
