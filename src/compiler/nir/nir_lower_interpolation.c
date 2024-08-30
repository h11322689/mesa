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
      /*
   * This lower pass lowers load_interpolated_input for various interpolation
   * modes (as configured via nir_lower_interpolation_options bitmask) into
   * load_attribute_deltas plus alu instructions:
   *
   *    vec3 ad = load_attribute_deltas(varying_slot)
   *    float result = ad.x + ad.y * j + ad.z * i
   *
   */
      #include "nir.h"
   #include "nir_builder.h"
      static bool
   nir_lower_interpolation_instr(nir_builder *b, nir_instr *instr, void *cb_data)
   {
      nir_lower_interpolation_options options =
            if (instr->type != nir_instr_type_intrinsic)
                     if (intr->intrinsic != nir_intrinsic_load_interpolated_input)
            nir_intrinsic_instr *bary_intrinsic =
            /* Leave VARYING_SLOT_POS alone */
   if (nir_intrinsic_base(intr) == VARYING_SLOT_POS)
            const enum glsl_interp_mode interp_mode =
            /* We need actual interpolation modes by the time we get here */
            /* Only lower for inputs that need interpolation */
   if (interp_mode != INTERP_MODE_SMOOTH &&
      interp_mode != INTERP_MODE_NOPERSPECTIVE)
                  switch (op) {
   case nir_intrinsic_load_barycentric_at_sample:
      if (options & nir_lower_interpolation_at_sample)
            case nir_intrinsic_load_barycentric_at_offset:
      if (options & nir_lower_interpolation_at_offset)
            case nir_intrinsic_load_barycentric_centroid:
      if (options & nir_lower_interpolation_centroid)
            case nir_intrinsic_load_barycentric_pixel:
      if (options & nir_lower_interpolation_pixel)
            case nir_intrinsic_load_barycentric_sample:
      if (options & nir_lower_interpolation_sample)
            default:
                           nir_def *comps[NIR_MAX_VEC_COMPONENTS];
   for (int i = 0; i < intr->num_components; i++) {
      nir_def *iid =
      nir_load_fs_input_interp_deltas(b, 32, intr->src[1].ssa,
                     nir_def *bary = intr->src[0].ssa;
            val = nir_ffma(b, nir_channel(b, bary, 1),
               val = nir_ffma(b, nir_channel(b, bary, 0),
                     }
   nir_def *vec = nir_vec(b, comps, intr->num_components);
               }
      bool
   nir_lower_interpolation(nir_shader *shader, nir_lower_interpolation_options options)
   {
      return nir_shader_instructions_pass(shader, nir_lower_interpolation_instr,
                  }
