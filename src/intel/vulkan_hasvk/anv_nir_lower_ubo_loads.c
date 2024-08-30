   /*
   * Copyright © 2020 Intel Corporation
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
      #include "anv_nir.h"
   #include "nir_builder.h"
      static bool
   lower_ubo_load_instr(nir_builder *b, nir_intrinsic_instr *load,
         {
      if (load->intrinsic != nir_intrinsic_load_global_constant_offset &&
      load->intrinsic != nir_intrinsic_load_global_constant_bounded)
                  nir_def *base_addr = load->src[0].ssa;
   nir_def *bound = NULL;
   if (load->intrinsic == nir_intrinsic_load_global_constant_bounded)
            unsigned bit_size = load->def.bit_size;
   assert(bit_size >= 8 && bit_size % 8 == 0);
            nir_def *val;
   if (nir_src_is_const(load->src[1])) {
               /* Things should be component-aligned. */
                     unsigned suboffset = offset % 64;
            /* Load two just in case we go over a 64B boundary */
   nir_def *data[2];
   for (unsigned i = 0; i < 2; i++) {
      nir_def *pred;
   if (bound) {
         } else {
                                             val = nir_extract_bits(b, data, 2, suboffset * 8,
      } else {
      nir_def *offset = load->src[1].ssa;
            if (bound) {
               unsigned load_size = byte_size * load->num_components;
                           nir_def *load_val =
      nir_build_load_global_constant(b, load->def.num_components,
                                       } else {
      val = nir_build_load_global_constant(b, load->def.num_components,
                                    nir_def_rewrite_uses(&load->def, val);
               }
      bool
   anv_nir_lower_ubo_loads(nir_shader *shader)
   {
      return nir_shader_intrinsics_pass(shader, lower_ubo_load_instr,
            }
