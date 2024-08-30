   /*
   * Copyright (c) 2022 Amazon.com, Inc. or its affiliates.
   * Copyright (C) 2019-2022 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "pan_ir.h"
      static enum pipe_format
   varying_format(nir_alu_type t, unsigned ncomps)
   {
            #define VARYING_FORMAT(ntype, nsz, ptype, psz)                                 \
      {                                                                           \
      .type = nir_type_##ntype##nsz, .formats = {                              \
      PIPE_FORMAT_R##psz##_##ptype,                                         \
   PIPE_FORMAT_R##psz##G##psz##_##ptype,                                 \
   PIPE_FORMAT_R##psz##G##psz##B##psz##_##ptype,                         \
                  static const struct {
      nir_alu_type type;
      } conv[] = {
      VARYING_FORMAT(float, 32, FLOAT, 32),
   VARYING_FORMAT(uint, 32, UINT, 32),
         #undef VARYING_FORMAT
                  for (unsigned i = 0; i < ARRAY_SIZE(conv); i++) {
      if (conv[i].type == t)
                  }
      struct slot_info {
      nir_alu_type type;
   unsigned count;
      };
      static bool
   walk_varyings(UNUSED nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            /* Only consider intrinsics that access varyings */
   switch (intr->intrinsic) {
   case nir_intrinsic_store_output:
      if (b->shader->info.stage != MESA_SHADER_VERTEX)
            count = nir_src_num_components(intr->src[0]);
         case nir_intrinsic_load_input:
   case nir_intrinsic_load_interpolated_input:
      if (b->shader->info.stage != MESA_SHADER_FRAGMENT)
            count = intr->def.num_components;
         default:
                           if (sem.no_varying)
            /* In a fragment shader, flat shading is lowered to load_input but
   * interpolation is lowered to load_interpolated_input, so we can check
   * the intrinsic to distinguish.
   *
   * In a vertex shader, we consider everything flat, as the information
   * will not contribute to the final linked varyings -- flatness is used
   * only to determine the type, and the GL linker uses the type from the
   * fragment shader instead.
   */
   bool flat = (intr->intrinsic != nir_intrinsic_load_interpolated_input);
            /* Demote interpolated float varyings to fp16 where possible. We do not
   * demote flat varyings, including integer varyings, due to various
   * issues with the Midgard hardware behaviour and TGSI shaders, as well
   * as having no demonstrable benefit in practice.
   */
   if (type == nir_type_float && sem.medium_precision)
         else
            /* Count currently contains the number of components accessed by this
   * intrinsics. However, we may be accessing a fractional location,
   * indicating by the NIR component. Add that in. The final value be the
   * maximum (component + count), an upper bound on the number of
   * components possibly used.
   */
            /* Consider each slot separately */
   for (unsigned offset = 0; offset < sem.num_slots; ++offset) {
      unsigned location = sem.location + offset;
            if (slots[location].type) {
      assert(slots[location].type == type);
      } else {
      slots[location].type = type;
                              }
      void
   pan_nir_collect_varyings(nir_shader *s, struct pan_shader_info *info)
   {
      if (s->info.stage != MESA_SHADER_VERTEX &&
      s->info.stage != MESA_SHADER_FRAGMENT)
         struct slot_info slots[64] = {0};
            struct pan_shader_varying *varyings = (s->info.stage == MESA_SHADER_VERTEX)
                           for (unsigned i = 0; i < ARRAY_SIZE(slots); ++i) {
      if (!slots[i].type)
            enum pipe_format format = varying_format(slots[i].type, slots[i].count);
            unsigned index = slots[i].index;
            varyings[index].location = i;
               if (s->info.stage == MESA_SHADER_VERTEX)
         else
      }
