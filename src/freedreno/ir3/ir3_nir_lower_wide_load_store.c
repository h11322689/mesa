   /*
   * Copyright © 2021 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "ir3_nir.h"
         /*
   * Lowering for wide (larger than vec4) load/store
   */
      static bool
   lower_wide_load_store_filter(const nir_instr *instr, const void *unused)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (is_intrinsic_store(intr->intrinsic))
            if (is_intrinsic_load(intr->intrinsic))
               }
      static nir_def *
   lower_wide_load_store(nir_builder *b, nir_instr *instr, void *unused)
   {
                        if (is_intrinsic_store(intr->intrinsic)) {
      unsigned num_comp = nir_intrinsic_src_components(intr, 0);
   unsigned wrmask = nir_intrinsic_write_mask(intr);
   nir_def *val = intr->src[0].ssa;
            for (unsigned off = 0; off < num_comp; off += 4) {
                     nir_intrinsic_instr *store =
         store->num_components = c;
   store->src[0] = nir_src_for_ssa(v);
   store->src[1] = nir_src_for_ssa(addr);
   nir_intrinsic_set_align(store, nir_intrinsic_align(intr), 0);
                  addr = nir_iadd(b,
                        } else {
      unsigned num_comp = nir_intrinsic_dest_components(intr);
   unsigned bit_size = intr->def.bit_size;
   nir_def *addr = intr->src[0].ssa;
            for (unsigned off = 0; off < num_comp;) {
               nir_intrinsic_instr *load =
         load->num_components = c;
   load->src[0] = nir_src_for_ssa(addr);
   nir_intrinsic_set_align(load, nir_intrinsic_align(intr), 0);
                  addr = nir_iadd(b,
                  for (unsigned i = 0; i < c; i++) {
                           }
      bool
   ir3_nir_lower_wide_load_store(nir_shader *shader)
   {
      return nir_shader_lower_instructions(
            }
