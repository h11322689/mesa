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
      #include "isl/isl.h"
      #include "brw_nir.h"
      static bool
   brw_nir_blockify_uniform_loads_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_intrinsic)
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
      /* BDW PRMs, Volume 7: 3D-Media-GPGPU: OWord Block ReadWrite:
   *
   *    "The surface base address must be OWord-aligned."
   *
   * We can't make that guarantee with SSBOs where the alignment is
   * 4bytes.
   */
   if (devinfo->ver < 9)
            if (nir_src_is_divergent(intrin->src[1]))
            if (intrin->def.bit_size != 32)
            /* Without the LSC, we can only do block loads of at least 4dwords (1
   * oword).
   */
   if (!devinfo->has_lsc && intrin->def.num_components < 4)
            intrin->intrinsic =
      intrin->intrinsic == nir_intrinsic_load_ubo ?
   nir_intrinsic_load_ubo_uniform_block_intel :
            case nir_intrinsic_load_shared:
      /* Block loads on shared memory are not supported before the LSC. */
   if (!devinfo->has_lsc)
            if (nir_src_is_divergent(intrin->src[0]))
            if (intrin->def.bit_size != 32)
            intrin->intrinsic = nir_intrinsic_load_shared_uniform_block_intel;
         case nir_intrinsic_load_global_constant:
      if (nir_src_is_divergent(intrin->src[0]))
            if (intrin->def.bit_size != 32)
            /* Without the LSC, we can only do block loads of at least 4dwords (1
   * oword).
   */
   if (!devinfo->has_lsc && intrin->def.num_components < 4)
            intrin->intrinsic = nir_intrinsic_load_global_constant_uniform_block_intel;
         default:
            }
      bool
   brw_nir_blockify_uniform_loads(nir_shader *shader,
         {
      return nir_shader_instructions_pass(shader,
                              }
