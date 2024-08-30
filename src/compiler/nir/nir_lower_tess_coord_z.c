   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_intrinsics.h"
   #include "shader_enums.h"
      static bool
   lower_tess_coord_z(nir_builder *b, nir_intrinsic_instr *intr, void *state)
   {
      if (intr->intrinsic != nir_intrinsic_load_tess_coord)
            b->cursor = nir_instr_remove(&intr->instr);
   nir_def *xy = nir_load_tess_coord_xy(b);
   nir_def *x = nir_channel(b, xy, 0);
   nir_def *y = nir_channel(b, xy, 1);
            bool *triangles = state;
   if (*triangles)
         else
            nir_def_rewrite_uses(&intr->def, nir_vec3(b, x, y, z));
      }
      bool
   nir_lower_tess_coord_z(nir_shader *shader, bool triangles)
   {
      return nir_shader_intrinsics_pass(shader, lower_tess_coord_z,
                  }
