   /*
   * Copyright © 2023 Intel Corporation
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
      /*
   * Lower non uniform at sample messages to the interpolator.
   *
   * This is pretty much identical to what nir_lower_non_uniform_access() does.
   * We do it here because otherwise GCM would undo this optimization. Also we
   * can assume divergence analysis here.
   */
      #include "brw_nir.h"
   #include "compiler/nir/nir_builder.h"
      static bool
   brw_nir_lower_non_uniform_barycentric_at_sample_instr(nir_builder *b,
               {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_barycentric_at_sample)
            if (nir_src_is_always_uniform(intrin->src[0]) ||
      !nir_src_is_divergent(intrin->src[0]))
                                                                                    }
      bool
   brw_nir_lower_non_uniform_barycentric_at_sample(nir_shader *nir)
   {
      return nir_shader_instructions_pass(
      nir,
   brw_nir_lower_non_uniform_barycentric_at_sample_instr,
   nir_metadata_none,
   }
