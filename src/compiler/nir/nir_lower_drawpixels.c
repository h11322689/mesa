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
      /* Lower glDrawPixels().
   *
   * This is based on the logic in st_get_drawpix_shader() in TGSI compiler.
   *
   * Run before nir_lower_io.
   */
      typedef struct {
      const nir_lower_drawpixels_options *options;
   nir_shader *shader;
      } lower_drawpixels_state;
      static nir_def *
   get_texcoord(nir_builder *b, lower_drawpixels_state *state)
   {
      if (state->texcoord == NULL) {
      state->texcoord = nir_get_variable_with_location(state->shader, nir_var_shader_in,
      }
      }
      static nir_def *
   get_scale(nir_builder *b, lower_drawpixels_state *state)
   {
      if (state->scale == NULL) {
      state->scale = nir_state_variable_create(state->shader, glsl_vec4_type(), "gl_PTscale",
      }
      }
      static nir_def *
   get_bias(nir_builder *b, lower_drawpixels_state *state)
   {
      if (state->bias == NULL) {
      state->bias = nir_state_variable_create(state->shader, glsl_vec4_type(), "gl_PTbias",
      }
      }
      static nir_def *
   get_texcoord_const(nir_builder *b, lower_drawpixels_state *state)
   {
      if (state->texcoord_const == NULL) {
      state->texcoord_const = nir_state_variable_create(state->shader, glsl_vec4_type(),
            }
      }
      static bool
   lower_color(nir_builder *b, lower_drawpixels_state *state, nir_intrinsic_instr *intr)
   {
      nir_def *texcoord;
   nir_tex_instr *tex;
                              const struct glsl_type *sampler2D =
            if (!state->tex) {
      state->tex =
         state->tex->data.binding = state->options->drawpix_sampler;
   state->tex->data.explicit_binding = true;
                        /* replace load_var(gl_Color) w/ texture sample:
   *   TEX def, texcoord, drawpix_sampler, 2D
   */
   tex = nir_tex_instr_create(state->shader, 3);
   tex->op = nir_texop_tex;
   tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   tex->coord_components = 2;
   tex->dest_type = nir_type_float32;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
         tex->src[2] =
      nir_tex_src_for_ssa(nir_tex_src_coord,
         nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(b, &tex->instr);
            /* Apply the scale and bias. */
   if (state->options->scale_and_bias) {
      /* MAD def, def, scale, bias; */
               if (state->options->pixel_maps) {
      if (!state->pixelmap) {
      state->pixelmap = nir_variable_create(b->shader, nir_var_uniform,
         state->pixelmap->data.binding = state->options->pixelmap_sampler;
   state->pixelmap->data.explicit_binding = true;
               nir_deref_instr *pixelmap_deref =
            /* do four pixel map look-ups with two TEX instructions: */
            /* TEX def.xy, def.xyyy, pixelmap_sampler, 2D; */
   tex = nir_tex_instr_create(state->shader, 3);
   tex->op = nir_texop_tex;
   tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   tex->coord_components = 2;
   tex->sampler_index = state->options->pixelmap_sampler;
   tex->texture_index = state->options->pixelmap_sampler;
   tex->dest_type = nir_type_float32;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_texture_deref,
         tex->src[1] = nir_tex_src_for_ssa(nir_tex_src_sampler_deref,
         tex->src[2] = nir_tex_src_for_ssa(nir_tex_src_coord,
            nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(b, &tex->instr);
            /* TEX def.zw, def.zwww, pixelmap_sampler, 2D; */
   tex = nir_tex_instr_create(state->shader, 1);
   tex->op = nir_texop_tex;
   tex->sampler_dim = GLSL_SAMPLER_DIM_2D;
   tex->coord_components = 2;
   tex->sampler_index = state->options->pixelmap_sampler;
   tex->dest_type = nir_type_float32;
   tex->src[0] = nir_tex_src_for_ssa(nir_tex_src_coord,
            nir_def_init(&tex->instr, &tex->def, 4, 32);
   nir_builder_instr_insert(b, &tex->instr);
            /* def = vec4(def.xy, def.zw); */
   def = nir_vec4(b,
                  nir_channel(b, def_xy, 0),
            nir_def_rewrite_uses(&intr->def, def);
      }
      static bool
   lower_texcoord(nir_builder *b, lower_drawpixels_state *state, nir_intrinsic_instr *intr)
   {
               nir_def *texcoord_const = get_texcoord_const(b, state);
   nir_def_rewrite_uses(&intr->def, texcoord_const);
      }
      static bool
   lower_drawpixels_instr(nir_builder *b, nir_instr *instr, void *cb_data)
   {
      lower_drawpixels_state *state = cb_data;
   if (instr->type != nir_instr_type_intrinsic)
                     switch (intr->intrinsic) {
   case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
            if (var->data.location == VARYING_SLOT_COL0) {
      /* gl_Color should not have array/struct derefs: */
   assert(deref->deref_type == nir_deref_type_var);
      } else if (var->data.location == VARYING_SLOT_TEX0) {
      /* gl_TexCoord should not have array/struct derefs: */
   assert(deref->deref_type == nir_deref_type_var);
      }
               case nir_intrinsic_load_color0:
            case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_input: {
      if (nir_intrinsic_io_semantics(intr).location == VARYING_SLOT_TEX0)
            }
   default:
                     }
      void
   nir_lower_drawpixels(nir_shader *shader,
         {
      lower_drawpixels_state state = {
      .options = options,
                        nir_shader_instructions_pass(shader, lower_drawpixels_instr,
                  }
