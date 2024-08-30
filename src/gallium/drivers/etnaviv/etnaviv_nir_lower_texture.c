   /*
   * Copyright © 2023 Igalia S.L.
   * SPDX-License-Identifier: MIT
   */
      #include "etnaviv_nir.h"
      static bool
   lower_txs(nir_builder *b, nir_instr *instr, UNUSED void *data)
   {
      if (instr->type != nir_instr_type_tex)
                     if (tex->op != nir_texop_txs)
                     nir_def *idx = nir_imm_int(b, tex->texture_index);
                        }
      bool
   etna_nir_lower_texture(nir_shader *s, struct etna_shader_key *key)
   {
               nir_lower_tex_options lower_tex_options = {
      .lower_txp = ~0u,
   .lower_txs_lod = true,
                        if (key->has_sample_tex_compare)
      NIR_PASS(progress, s, nir_lower_tex_shadow, key->num_texture_states,
               NIR_PASS(progress, s, nir_shader_instructions_pass, lower_txs,
               }
