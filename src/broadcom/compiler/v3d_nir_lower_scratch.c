   /*
   * Copyright © 2018 Intel Corporation
   * Copyright © 2018 Broadcom
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
      #include "v3d_compiler.h"
   #include "compiler/nir/nir_builder.h"
   #include "compiler/nir/nir_format_convert.h"
      /** @file v3d_nir_lower_scratch.c
   *
   * Swizzles around the addresses of
   * nir_intrinsic_load_scratch/nir_intrinsic_store_scratch so that a QPU stores
   * a cacheline at a time per dword of scratch access, scalarizing and removing
   * writemasks in the process.
   */
      static nir_def *
   v3d_nir_scratch_offset(nir_builder *b, nir_intrinsic_instr *instr)
   {
         bool is_store = instr->intrinsic == nir_intrinsic_store_scratch;
            assert(nir_intrinsic_align_mul(instr) >= 4);
            /* The spill_offset register will already have the subgroup ID (EIDX)
      * shifted and ORed in at bit 2, so all we need to do is to move the
   * dword index up above V3D_CHANNELS.
      }
      static void
   v3d_nir_lower_load_scratch(nir_builder *b, nir_intrinsic_instr *instr)
   {
                           nir_def *chans[NIR_MAX_VEC_COMPONENTS];
   for (int i = 0; i < instr->num_components; i++) {
                           nir_intrinsic_instr *chan_instr =
                                                               nir_def *result = nir_vec(b, chans, instr->num_components);
   nir_def_rewrite_uses(&instr->def, result);
   }
      static void
   v3d_nir_lower_store_scratch(nir_builder *b, nir_intrinsic_instr *instr)
   {
                  nir_def *offset = v3d_nir_scratch_offset(b, instr);
            for (int i = 0; i < instr->num_components; i++) {
                                                               chan_instr->src[0] = nir_src_for_ssa(nir_channel(b,
                                          }
      static bool
   v3d_nir_lower_scratch_cb(nir_builder *b,
               {
         switch (intr->intrinsic) {
   case nir_intrinsic_load_scratch:
               case nir_intrinsic_store_scratch:
               default:
                  }
      bool
   v3d_nir_lower_scratch(nir_shader *s)
   {
         return nir_shader_intrinsics_pass(s, v3d_nir_lower_scratch_cb,
         }
