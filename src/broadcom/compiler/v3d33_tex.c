   /*
   * Copyright © 2016-2018 Broadcom
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
      #include "v3d_compiler.h"
      /* We don't do any address packing. */
   #define __gen_user_data void
   #define __gen_address_type uint32_t
   #define __gen_address_offset(reloc) (*reloc)
   #define __gen_emit_reloc(cl, reloc)
   #include "cle/v3d_packet_v33_pack.h"
      void
   v3d33_vir_emit_tex(struct v3d_compile *c, nir_tex_instr *instr)
   {
         /* FIXME: We don't bother implementing pipelining for texture reads
      * for any pre 4.x hardware. It should be straight forward to do but
   * we are not really testing or even targeting this hardware at
   * present.
                        struct V3D33_TEXTURE_UNIFORM_PARAMETER_0_CFG_MODE1 p0_unpacked = {
                           struct V3D33_TEXTURE_UNIFORM_PARAMETER_1_CFG_MODE1 p1_unpacked = {
            switch (instr->sampler_dim) {
   case GLSL_SAMPLER_DIM_1D:
            if (instr->is_array)
         else
      case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_RECT:
            if (instr->is_array)
         else
      case GLSL_SAMPLER_DIM_3D:
               case GLSL_SAMPLER_DIM_CUBE:
               default:
                  struct qreg coords[5];
   int next_coord = 0;
   for (unsigned i = 0; i < instr->num_srcs; i++) {
            switch (instr->src[i].src_type) {
   case nir_tex_src_coord:
            for (int j = 0; j < instr->coord_components; j++) {
         coords[next_coord++] =
   }
   if (instr->coord_components < 2)
                                       case nir_tex_src_lod:
            coords[next_coord++] =
                              if (instr->op != nir_texop_txf &&
      instr->op != nir_texop_tg4) {
                                                                                                if (instr->coord_components >= 3)
                     default:
               /* Limit the number of channels returned to both how many the NIR
      * instruction writes and how many the instruction could produce.
      p1_unpacked.return_words_of_texture_data =
            uint32_t p0_packed;
   V3D33_TEXTURE_UNIFORM_PARAMETER_0_CFG_MODE1_pack(NULL,
                  uint32_t p1_packed;
   V3D33_TEXTURE_UNIFORM_PARAMETER_1_CFG_MODE1_pack(NULL,
               /* Load unit number into the address field, which will be be used by
      * the driver to decide which texture to put in the actual address
   * field.
               /* There is no native support for GL texture rectangle coordinates, so
      * we have to rescale from ([0, width], [0, height]) to ([0, 1], [0,
   * 1]).
      if (instr->sampler_dim == GLSL_SAMPLER_DIM_RECT) {
            coords[0] = vir_FMUL(c, coords[0],
               coords[1] = vir_FMUL(c, coords[1],
               int texture_u[] = {
                        for (int i = 0; i < next_coord; i++) {
                     if (i == next_coord - 1)
                                                      for (int i = 0; i < 4; i++) {
               }
