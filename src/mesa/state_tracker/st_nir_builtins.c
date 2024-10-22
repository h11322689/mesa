   /*
   * Copyright © 2018 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "st_nir.h"
   #include "st_program.h"
      #include "compiler/nir/nir_builder.h"
   #include "compiler/glsl/gl_nir.h"
   #include "compiler/glsl/gl_nir_linker.h"
      void
   st_nir_finish_builtin_nir(struct st_context *st, nir_shader *nir)
   {
      struct pipe_screen *screen = st->screen;
                     nir->info.separate_shader = true;
   if (stage == MESA_SHADER_FRAGMENT)
            NIR_PASS_V(nir, nir_lower_global_vars_to_local);
   NIR_PASS_V(nir, nir_split_var_copies);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_lower_system_values);
            if (nir->options->lower_to_scalar) {
      nir_variable_mode mask =
                              if (st->lower_rect_tex) {
      const struct nir_lower_tex_options opts = { .lower_rect = true, };
                        st_nir_assign_vs_in_locations(nir);
            st_nir_lower_samplers(screen, nir, NULL, NULL);
   st_nir_lower_uniforms(st, nir);
   if (!screen->get_param(screen, PIPE_CAP_NIR_IMAGES_AS_DEREF))
            if (screen->finalize_nir) {
      char *msg = screen->finalize_nir(screen, nir);
      } else {
            }
      struct pipe_shader_state *
   st_nir_finish_builtin_shader(struct st_context *st,
         {
               struct pipe_shader_state state = {
      .type = PIPE_SHADER_IR_NIR,
                  }
      /**
   * Make a simple shader that copies inputs to corresponding outputs.
   */
   struct pipe_shader_state *
   st_nir_make_passthrough_shader(struct st_context *st,
                                 const char *shader_name,
   gl_shader_stage stage,
   {
      const struct glsl_type *vec4 = glsl_vec4_type();
   const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(stage, options,
            for (unsigned i = 0; i < num_vars; i++) {
      nir_variable *in;
   if (sysval_mask & (1 << i)) {
      in = nir_create_variable_with_location(b.shader, nir_var_system_value,
            } else {
      in = nir_create_variable_with_location(b.shader, nir_var_shader_in,
      }
   if (interpolation_modes)
            nir_variable *out =
      nir_create_variable_with_location(b.shader, nir_var_shader_out,
                              }
      /**
   * Make a simple shader that reads color value from a constant buffer
   * and uses it to clear all color buffers.
   */
   struct pipe_shader_state *
   st_nir_make_clearcolor_shader(struct st_context *st)
   {
      const nir_shader_compiler_options *options =
            nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, options,
         b.shader->info.num_ubos = 1;
   b.shader->num_outputs = 1;
            /* Read clear color from constant buffer */
   nir_def *clear_color = nir_load_uniform(&b, 4, 32, nir_imm_int(&b,0),
                  nir_variable *color_out = nir_create_variable_with_location(b.shader, nir_var_shader_out,
            /* Write out the color */
               }
