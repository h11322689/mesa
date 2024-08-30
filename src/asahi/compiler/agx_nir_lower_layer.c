   /*
   * Copyright 2023 Valve Corporation
   * SPDX-License-Identifier: MIT
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "agx_nir.h"
      void
   agx_nir_lower_layer(nir_shader *s)
   {
      assert(s->info.stage == MESA_SHADER_VERTEX);
   if (!(s->info.outputs_written & (VARYING_BIT_LAYER | VARYING_BIT_VIEWPORT)))
            /* Writes are in the last block, search */
   nir_function_impl *impl = nir_shader_get_entrypoint(s);
            nir_def *layer = NULL, *viewport = NULL;
            nir_foreach_instr(instr, last) {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *store = nir_instr_as_intrinsic(instr);
   if (store->intrinsic != nir_intrinsic_store_output)
            nir_io_semantics sem = nir_intrinsic_io_semantics(store);
            if (sem.location == VARYING_SLOT_LAYER) {
      assert(layer == NULL && "only written once");
      } else if (sem.location == VARYING_SLOT_VIEWPORT) {
      assert(viewport == NULL && "only written once");
      } else {
                           /* Leave the store as a varying-only, no sysval output */
   sem.no_sysval_output = true;
                        /* Pack together and write out */
            nir_def *zero = nir_imm_intN_t(&b, 0, 16);
   nir_def *packed =
      nir_pack_32_2x16_split(&b, layer ? nir_u2u16(&b, layer) : zero,
         /* Written with a sysval-only store, no varying output */
   nir_store_output(&b, packed, nir_imm_int(&b, 0),
                  }
