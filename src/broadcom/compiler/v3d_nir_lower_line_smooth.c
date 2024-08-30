   /*
   * Copyright © 2020 Raspberry Pi Ltd
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
      #include "compiler/v3d_compiler.h"
   #include "compiler/nir/nir_builder.h"
   #include <math.h>
      /**
   * Lowers line smoothing by modifying the alpha component of fragment outputs
   * using the distance from the center of the line.
   */
      struct lower_line_smooth_state {
         nir_shader *shader;
   };
      static void
   lower_line_smooth_intrinsic(struct lower_line_smooth_state *state,
               {
                                    nir_def *new_val = nir_fmul(b, nir_vec4(b, one, one, one, coverage),
            }
      static bool
   lower_line_smooth_func(struct lower_line_smooth_state *state,
         {
                           nir_foreach_block(block, impl) {
                                                                                             }
      static void
   initialise_coverage_var(struct lower_line_smooth_state *state,
         {
                                    /* The line coord varies from 0.0 to 1.0 across the width of the line */
            /* fabs(line_coord - 0.5) * real_line_width */
   nir_def *pixels_from_center =
                        /* 0.5 - 1/√2 * (pixels_from_center - line_width * 0.5) */
   nir_def *coverage =
            nir_fsub(&b,
            nir_imm_float(&b, 0.5f),
   nir_fmul(&b,
                  /* Discard fragments that aren’t covered at all by the line */
                     /* Clamp to at most 1.0. If it was less than 0.0 then the fragment will
      * be discarded so we don’t need to handle that.
               }
      static nir_variable *
   make_coverage_var(nir_shader *s)
   {
         nir_variable *var = nir_variable_create(s,
                              }
      bool
   v3d_nir_lower_line_smooth(nir_shader *s)
   {
                           struct lower_line_smooth_state state = {
                        nir_foreach_function_with_impl(function, impl, s) {
                                    if (progress) {
            nir_metadata_preserve(impl,
      } else {
               }
