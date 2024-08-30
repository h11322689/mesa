   /*
   * Copyright © 2022 Google, Inc.
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
      /*
   * A helper to create a passthrough TCS shader for drivers that cannot handle
   * having a TES without TCS.
   *
   * Uses the load_tess_level_outer_default and load_tess_level_inner_default
   * intrinsics to load the gl_TessLevelOuter and gl_TessLevelInner values,
   * so driver will somehow need to implement those to load the values set
   * by pipe_context::set_tess_state() or similar.
   */
      nir_shader *
   nir_create_passthrough_tcs_impl(const nir_shader_compiler_options *options,
               {
      nir_builder b = nir_builder_init_simple_shader(MESA_SHADER_TESS_CTRL, options,
            nir_variable *in_inner =
      nir_create_variable_with_location(b.shader, nir_var_system_value,
         nir_variable *out_inner =
      nir_create_variable_with_location(b.shader, nir_var_shader_out,
         nir_def *inner = nir_load_var(&b, in_inner);
            nir_variable *in_outer =
      nir_create_variable_with_location(b.shader, nir_var_system_value,
         nir_variable *out_outer =
      nir_create_variable_with_location(b.shader, nir_var_shader_out,
         nir_def *outer = nir_load_var(&b, in_outer);
            nir_def *id = nir_load_invocation_id(&b);
   for (unsigned i = 0; i < num_locations; i++) {
      const struct glsl_type *type;
   unsigned semantic = locations[i];
   if ((semantic <= VARYING_SLOT_VAR31 && semantic != VARYING_SLOT_EDGE) ||
      semantic >= VARYING_SLOT_VAR0_16BIT)
      else
            nir_variable *in = nir_create_variable_with_location(b.shader, nir_var_shader_in,
            nir_variable *out = nir_create_variable_with_location(b.shader, nir_var_shader_out,
            /* no need to use copy_var to save a lower pass */
   nir_def *value = nir_load_array_var(&b, in, id);
                                    }
      nir_shader *
   nir_create_passthrough_tcs(const nir_shader_compiler_options *options,
         {
      unsigned locations[MAX_VARYING];
            nir_foreach_shader_out_variable(var, vs) {
      assert(num_outputs < ARRAY_SIZE(locations));
                  }
