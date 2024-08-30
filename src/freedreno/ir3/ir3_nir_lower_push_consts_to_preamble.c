   /*
   * Copyright © 2023 Igalia S.L.
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "util/u_math.h"
   #include "ir3_compiler.h"
   #include "ir3_nir.h"
      bool
   ir3_nir_lower_push_consts_to_preamble(nir_shader *nir,
         {
      nir_function_impl *preamble = nir_shader_get_preamble(nir);
   nir_builder _b = nir_builder_at(nir_before_impl(preamble));
            nir_copy_push_const_to_uniform_ir3(
      b, nir_imm_int(b, 0), .base = v->shader_options.push_consts_base,
         nir_foreach_function_impl(impl, nir) {
         }
      }
