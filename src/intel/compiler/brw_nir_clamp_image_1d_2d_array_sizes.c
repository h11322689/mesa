   /*
   * Copyright © 2020 Intel Corporation
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
      #include "compiler/nir/nir_builder.h"
   #include "brw_nir.h"
      /**
   * Wa_1806565034:
   *
   * Gfx12+ allows to set RENDER_SURFACE_STATE::SurfaceArray to 1 only if
   * array_len > 1. Setting RENDER_SURFACE_STATE::SurfaceArray to 0 results in
   * the HW RESINFO message to report an array size of 0 which breaks texture
   * array size queries.
   *
   * This NIR pass works around this by patching the array size with a
   * MAX(array_size, 1) for array textures.
   */
      static bool
   brw_nir_clamp_image_1d_2d_array_sizes_instr(nir_builder *b,
               {
               switch (instr->type) {
   case nir_instr_type_intrinsic: {
               switch (intr->intrinsic) {
   case nir_intrinsic_image_size:
   case nir_intrinsic_bindless_image_size:
                                 case nir_intrinsic_image_deref_size: {
                                       image_size = &intr->def;
               default:
         }
               case nir_instr_type_tex: {
      nir_tex_instr *tex_instr = nir_instr_as_tex(instr);
   if (tex_instr->op != nir_texop_txs)
            if (!tex_instr->is_array)
            image_size = &tex_instr->def;
               default:
                  if (!image_size)
                     nir_def *components[4];
   /* OR all the sizes for all components but the last. */
   nir_def *or_components = nir_imm_int(b, 0);
   for (int i = 0; i < image_size->num_components; i++) {
      if (i == (image_size->num_components - 1)) {
      nir_def *null_or_size[2] = {
      nir_imm_int(b, 0),
   nir_imax(b, nir_channel(b, image_size, i),
                     /* Using the ORed sizes select either the element 0 or 1
   * from this vec2. For NULL textures which have a size of
   * 0x0x0, we'll select the first element which is 0 and for
   * the rest MAX(depth, 1).
   */
   components[i] =
      nir_vector_extract(b, vec2_null_or_size,
         } else {
      components[i] = nir_channel(b, image_size, i);
         }
   nir_def *image_size_replacement =
                     nir_def_rewrite_uses_after(image_size,
                     }
      bool
   brw_nir_clamp_image_1d_2d_array_sizes(nir_shader *shader)
   {
      return nir_shader_instructions_pass(shader,
                        }
