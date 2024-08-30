   /*
   * Copyright © 2019 Google, Inc.
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
      #include "compiler/nir/nir_builder.h"
   #include "ir3_nir.h"
      /**
   * This pass lowers load_barycentric_at_offset to dsx.3d/dsy.3d and alu
   * instructions.
   */
      static nir_def *
   ir3_nir_lower_load_barycentric_at_offset_instr(nir_builder *b, nir_instr *instr,
         {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
         #define chan(var, c) nir_channel(b, var, c)
         nir_def *off = intr->src[0].ssa;
   /* note: at_offset is defined to be relative to the center of the pixel */
            /* Need helper invocations for our ddx/ddys to work. */
   if (b->shader->info.stage == MESA_SHADER_FRAGMENT)
            if (interp_mode != INTERP_MODE_SMOOTH) {
      /* Offset our pixel center ij by the offset argument (units of pixels)
   * times the derivatives of ij in screen space.
   */
   nir_def *new_ij = ij;
   new_ij = nir_ffma(b, chan(off, 0), nir_fddx(b, ij), new_ij);
               } else {
               /* scaled ij -- ij comes in multiplied by by 1/center_w so multiply that
   * back out, plus add center_w as the 3rd component for taking the
   * derivatives.
   *
   * We actually suspect that we should be using rhw here instead of center_w,
   * but no tests seem to distinguish between the two.
   */
   nir_def *sij =
            /* Get the offset value from pixel center for ij, and also for w. */
   nir_def *pos = sij;
   pos = nir_ffma(b, chan(off, 0), nir_fddx(b, sij), pos);
            /* convert back into screen space, dividing by the offset 1/w */
   return nir_fmul(b, nir_trim_vector(b, pos, 2),
         }
      static bool
   ir3_nir_lower_load_barycentric_at_offset_filter(const nir_instr *instr,
         {
      return (instr->type == nir_instr_type_intrinsic &&
            }
      bool
   ir3_nir_lower_load_barycentric_at_offset(nir_shader *shader)
   {
               return nir_shader_lower_instructions(
      shader, ir3_nir_lower_load_barycentric_at_offset_filter,
   }
