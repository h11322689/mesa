   /*
   * Copyright © 2015 Red Hat
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
      #include "nir.h"
   #include "nir_builder.h"
      /* Lower glBitmap().
   *
   * This is based on the logic in st_get_bitmap_shader() in TGSI compiler.
   * From st_cb_bitmap.c:
   *
   *    glBitmaps are drawn as textured quads.  The user's bitmap pattern
   *    is stored in a texture image.  An alpha8 texture format is used.
   *    The fragment shader samples a bit (texel) from the texture, then
   *    discards the fragment if the bit is off.
   *
   *    Note that we actually store the inverse image of the bitmap to
   *    simplify the fragment program.  An "on" bit gets stored as texel=0x0
   *    and an "off" bit is stored as texel=0xff.  Then we kill the
   *    fragment if the negated texel value is less than zero.
   *
   * Note that the texture format will be, according to what driver supports,
   * in order of preference (with swizzle):
   *
   *    I8_UNORM - .xxxx
   *    A8_UNORM - .000x
   *    L8_UNORM - .xxx1
   *
   * If L8_UNORM, options->swizzle_xxxx is true.  Otherwise we can just use
   * the .w comp.
   *
   * Run before nir_lower_io.
   */
      static void
   lower_bitmap(nir_shader *shader, nir_builder *b,
         {
      nir_def *texcoord;
   nir_tex_instr *tex;
            texcoord = nir_load_var(b, nir_get_variable_with_location(shader, nir_var_shader_in,
            const struct glsl_type *sampler2D =
            nir_variable *tex_var =
         tex_var->data.binding = options->sampler;
   tex_var->data.explicit_binding = true;
                     tex = nir_tex_instr_create(shader, 3);
   tex->op = nir_texop_tex;
   tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   tex->coord_components = 2;
   tex->dest_type = nir_type_float32;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
         tex->src[2] = nir_tex_src_for_ssa(nir_tex_src_coord,
            nir_def_init(&tex->instr, &tex->def, 4, 32);
            /* kill if tex != 0.0.. take .x or .w channel according to format: */
   cond = nir_fneu_imm(b, nir_channel(b, &tex->def, options->swizzle_xxxx ? 0 : 3),
                        }
      static void
   lower_bitmap_impl(nir_function_impl *impl,
         {
                        nir_metadata_preserve(impl, nir_metadata_block_index |
      }
      void
   nir_lower_bitmap(nir_shader *shader,
         {
                  }
