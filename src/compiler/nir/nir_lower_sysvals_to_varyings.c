   /*
   * Copyright © 2021 Collabora Ltd.
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
   #include "nir_builder.h"
      /*
   * spirv_to_nir() creates system values for some builtin inputs, but
   * backends might want to have those inputs exposed as varyings. This
   * lowering pass allows backends to convert system values to input
   * varyings and should be called just after spirv_to_nir() when needed.
   */
      bool
   nir_lower_sysvals_to_varyings(nir_shader *shader,
         {
               nir_foreach_variable_with_modes(var, shader, nir_var_system_value) {
      #define SYSVAL_TO_VARYING(opt, sysval, varying)       \
      case SYSTEM_VALUE_##sysval:                        \
      if (options->opt) {                             \
      var->data.mode = nir_var_shader_in;          \
   var->data.location = VARYING_SLOT_##varying; \
      }                                               \
               SYSVAL_TO_VARYING(frag_coord, FRAG_COORD, POS);
         #undef SYSVAL_TO_VARYING
            default:
                     if (progress)
            /* Nothing this does actually changes anything tracked by metadata.
   * If we ever made this pass more complicated, we might need to care
   * more about metadata.
   */
               }
