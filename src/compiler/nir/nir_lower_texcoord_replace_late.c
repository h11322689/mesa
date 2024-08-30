   /*
   * Copyright 2022 Alyssa Rosenzweig
   * Copyright 2020 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      struct opts {
      unsigned coord_replace;
      };
      static nir_def *
   nir_channel_or_undef(nir_builder *b, nir_def *def, signed int channel)
   {
      if (channel >= 0 && channel < def->num_components)
         else
      }
      static bool
   pass(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic != nir_intrinsic_load_interpolated_input &&
      intr->intrinsic != nir_intrinsic_load_input)
         nir_src *offset = nir_get_io_offset_src(intr);
            nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   unsigned location = sem.location + nir_src_as_uint(*offset);
            if (location < VARYING_SLOT_TEX0 || location > VARYING_SLOT_TEX7)
            if (!(opts->coord_replace & BITFIELD_BIT(location - VARYING_SLOT_TEX0)))
            b->cursor = nir_before_instr(instr);
   nir_def *channels[4] = {
      NULL, NULL,
   nir_imm_float(b, 0.0),
               if (opts->point_coord_is_sysval) {
               b->cursor = nir_after_instr(instr);
   channels[0] = nir_channel(b, pntc, 0);
      } else {
      sem.location = VARYING_SLOT_PNTC;
   nir_src_rewrite(offset, nir_imm_int(b, 0));
   nir_intrinsic_set_io_semantics(intr, sem);
            b->cursor = nir_after_instr(instr);
   channels[0] = nir_channel_or_undef(b, raw, 0 - component);
               nir_def *res = nir_vec(b, &channels[component], intr->num_components);
   nir_def_rewrite_uses_after(&intr->def, res,
            }
      void
   nir_lower_texcoord_replace_late(nir_shader *s, unsigned coord_replace,
         {
      assert(s->info.stage == MESA_SHADER_FRAGMENT);
                     /* If no relevant texcoords are read, there's nothing to do */
   if (!(s->info.inputs_read & replace_mask))
            /* Otherwise, we're going to replace these texcoord reads with a PNTC read */
            if (!point_coord_is_sysval)
            nir_shader_instructions_pass(s, pass,
                              }
