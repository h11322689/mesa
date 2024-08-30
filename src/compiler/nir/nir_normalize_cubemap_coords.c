   /*
   * Copyright © 2015 Intel Corporation
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
      #include "nir.h"
   #include "nir_builtin_builder.h"
      /*
   * Lower cubemap coordinate to have normalized coordinates where the largest
   * magnitude component is -1.0 or 1.0.
   */
   static bool
   normalize_cubemap_coords(nir_builder *b, nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_tex)
            nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->sampler_dim != GLSL_SAMPLER_DIM_CUBE)
                     int idx = nir_tex_instr_src_index(tex, nir_tex_src_coord);
   if (idx < 0)
            nir_def *orig_coord =
                  nir_def *orig_xyz = nir_trim_vector(b, orig_coord, 3);
   nir_def *norm = nir_fmax_abs_vec_comp(b, orig_xyz);
            /* Array indices don't have to be normalized, so make a new vector
   * with the coordinate's array index untouched.
   */
   if (tex->coord_components == 4) {
      normalized = nir_vector_insert_imm(b, normalized,
               nir_src_rewrite(&tex->src[idx].src, normalized);
      }
      bool
   nir_normalize_cubemap_coords(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader, normalize_cubemap_coords,
                  }
