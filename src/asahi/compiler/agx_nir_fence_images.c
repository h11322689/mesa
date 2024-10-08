   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir_builder.h"
   #include "agx_nir.h"
   #include "nir.h"
   #include "nir_builder_opcodes.h"
   #include "nir_intrinsics.h"
      static bool
   pass(struct nir_builder *b, nir_intrinsic_instr *intr, void *data)
   {
               /* If the image is write-only, there is no fencing needed */
   if (nir_intrinsic_has_access(intr) &&
      (nir_intrinsic_access(intr) & ACCESS_NON_READABLE)) {
               switch (intr->intrinsic) {
   case nir_intrinsic_image_store:
   case nir_intrinsic_bindless_image_store:
      nir_fence_pbe_to_tex_agx(b);
         case nir_intrinsic_image_atomic:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_image_atomic_swap:
   case nir_intrinsic_bindless_image_atomic_swap:
      nir_fence_mem_to_tex_agx(b);
         default:
            }
      bool
   agx_nir_fence_images(nir_shader *s)
   {
      return nir_shader_intrinsics_pass(
      }
