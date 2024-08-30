   /*
   * Copyright © 2023 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <stdint.h>
      #include "pvr_uscgen.h"
   #include "rogue/rogue.h"
   #include "rogue/rogue_builder.h"
   #include "util/u_dynarray.h"
      void pvr_uscgen_tq_frag(const struct pvr_tq_shader_properties *shader_props,
                     {
      rogue_builder b;
            unsigned smp_coord_size = 2;
   unsigned smp_coord_idx = 0;
            unsigned channels = 0;
   unsigned output_idx = 1;
            unsigned image_state_size = 4;
   unsigned image_state_idx;
            unsigned smp_state_size = 4;
   unsigned smp_state_idx;
            rogue_set_shader_name(shader, "TQ (fragment)");
   rogue_builder_init(&b, shader);
            smp_coords =
            /* TODO: Unrestrict. */
   assert(shader_props->full_rate == false);
            const struct pvr_tq_layer_properties *layer_props =
                  /* TODO: Unrestrict. */
   assert(layer_props->msaa == false);
   assert(layer_props->sample_count == 1U);
   assert(layer_props->resolve_op == PVR_RESOLVE_BLEND);
   assert(layer_props->pbe_format == PVR_TRANSFER_PBE_PIXEL_SRC_RAW64 ||
         assert(layer_props->sample == false);
   assert(layer_props->layer_floats == PVR_INT_COORD_SET_FLOATS_0);
   assert(layer_props->byte_unwind == 0);
            loads = pvr_pbe_pixel_num_loads(layer_props->pbe_format);
   for (uint32_t load = 0; load < loads; ++load) {
      if (shader_props->iterated) {
      /* TODO: feed{back,forward} the coeff index to/from shader_info. */
   unsigned coeff_index = 0;
                  rogue_instr *instr = &rogue_FITR_PIXEL(&b,
                                    } else {
      rogue_instr *instr;
   rogue_regarray *smp_coord_x =
                        /* (X,Y).P, pixel (X,Y) coordinates, pixel mode. */
                  instr =
      &rogue_MOV(&b, rogue_ref_regarray(smp_coord_x), rogue_ref_reg(in_x))
               instr =
      &rogue_MOV(&b, rogue_ref_regarray(smp_coord_y), rogue_ref_reg(in_y))
                  if (!layer_props->msaa) {
   } else {
                     /* Source conversion. */
   switch (layer_props->pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW64:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW128:
            default:
                  /* TODO: Select the texture_regs index appropriately. */
   assert(sh_reg_layout->combined_image_samplers.count == 1);
   image_state_idx = sh_reg_layout->combined_image_samplers.offsets[0].image;
   image_state =
            smp_state_idx = sh_reg_layout->combined_image_samplers.offsets[0].sampler;
            /* Pack/blend phase. */
            switch (layer_props->pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW64:
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW128: {
      switch (layer_props->pbe_format) {
   case PVR_TRANSFER_PBE_PIXEL_SRC_RAW64:
                  case PVR_TRANSFER_PBE_PIXEL_SRC_RAW128:
                  default:
                           smp2d = rogue_SMP2D(&b,
                     rogue_ref_regarray(outputs),
   rogue_ref_drc(0),
   rogue_ref_regarray(image_state),
   rogue_ref_regarray(smp_coords),
   rogue_set_backend_op_mod(smp2d, ROGUE_BACKEND_OP_MOD_SLCWRITEBACK);
            if (!shader_props->iterated)
                     default:
                           /* Copy outputs. */
   for (unsigned u = 0; u < channels; ++u) {
      rogue_regarray *output_elem =
         rogue_reg *pixout_elem = rogue_pixout_reg(b.shader, u);
   rogue_MOV(&b,
                              rogue_shader_passes(shader);
                     sh_reg_layout->compiler_out.usc_constants.count = 0;
               }
      void pvr_uscgen_tq_eot(unsigned rt_count,
               {
      rogue_builder b;
   rogue_shader *shader = rogue_shader_create(NULL, MESA_SHADER_NONE);
   rogue_set_shader_name(shader, "TQ (EOT)");
   rogue_builder_init(&b, shader);
            rogue_backend_instr *emitpix = NULL;
   for (unsigned u = 0; u < rt_count; ++u) {
      if (u > 0)
            rogue_reg *state_word_0 = rogue_shared_reg(shader, pbe_regs[u]);
            emitpix = rogue_EMITPIX(&b,
                              rogue_set_backend_op_mod(emitpix, ROGUE_BACKEND_OP_MOD_FREEP);
            rogue_shader_passes(shader);
               }
