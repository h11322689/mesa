   /*
   * Copyright © Microsoft Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      static bool
   nir_lower_tex_shadow_filter(const nir_instr *instr,
         {
      if (instr->type != nir_instr_type_tex)
            /* To be consistent we also want to lower tex when we lower anything,
   * otherwise the differences in evaluating the shadow value might lead
   * to artifacts. */
   nir_tex_instr *tex = nir_instr_as_tex(instr);
   if (tex->op != nir_texop_txb &&
      tex->op != nir_texop_txl &&
   tex->op != nir_texop_txd &&
   tex->op != nir_texop_tex)
            }
      static const struct glsl_type *
   strip_shadow(const struct glsl_type *type)
   {
      const struct glsl_type *new_type =
      glsl_sampler_type(
      glsl_get_sampler_dim(type),
   false, glsl_sampler_type_is_array(type),
      }
      static const struct glsl_type *
   strip_shadow_with_array(const struct glsl_type *type)
   {
      if (glsl_type_is_array(type))
      return glsl_array_type(strip_shadow(glsl_without_array(type)),
         }
      typedef struct {
      unsigned n_states;
   enum compare_func *compare_func;
      } sampler_state;
      static nir_def *
   nir_lower_tex_shadow_impl(nir_builder *b, nir_instr *instr, void *options)
      {
               sampler_state *state = (sampler_state *)options;
            b->cursor = nir_after_instr(instr);
            int comp_index = nir_tex_instr_src_index(tex, nir_tex_src_comparator);
            nir_deref_instr *sampler_deref = NULL;
            int sampler_index = nir_tex_instr_src_index(tex, nir_tex_src_sampler_deref);
   if (sampler_index >= 0) {
      sampler_deref = nir_instr_as_deref(tex->src[sampler_index].src.ssa->parent_instr);
   sampler = nir_deref_instr_get_variable(sampler_deref);
               /* NIR expects a vec4 result from the above texture instructions */
            nir_def *tex_r = nir_channel(b, &tex->def, 0);
            int proj_index = nir_tex_instr_src_index(tex, nir_tex_src_projector);
   if (proj_index >= 0)
            nir_def *result =
      nir_compare_func(b,
               result = nir_b2f32(b, result);
   nir_def *one = nir_imm_float(b, 1.0);
            nir_def *lookup[6] = { result, NULL, NULL, NULL, zero, one };
            if (sampler_binding < state->n_states) {
      r[0] = lookup[state->tex_swizzles[sampler_binding].swizzle_r];
   r[1] = lookup[state->tex_swizzles[sampler_binding].swizzle_g];
   r[2] = lookup[state->tex_swizzles[sampler_binding].swizzle_b];
                        if (sampler_index >= 0) {
      sampler->type = strip_shadow_with_array(sampler->type);
               tex->is_shadow = false;
               }
      bool
   nir_lower_tex_shadow(nir_shader *s,
                     {
               bool result =
      nir_shader_lower_instructions(s,
                     }
