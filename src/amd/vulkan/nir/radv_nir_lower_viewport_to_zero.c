   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   * Copyright © 2023 Valve Corporation
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
   #include "radv_nir.h"
      bool
   radv_nir_lower_viewport_to_zero(nir_shader *nir)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
                     /* There should be only one deref load for VIEWPORT after lower_io_to_temporaries. */
   nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_variable *var = nir_intrinsic_get_var(intr, 0);
                           nir_def_rewrite_uses(&intr->def, nir_imm_zero(&b, 1, 32));
   progress = true;
      }
   if (progress)
               if (progress)
         else
               }
