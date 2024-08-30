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
      #include "nir_builder.h"
      static bool
   lower_vec3_to_vec4_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (!nir_deref_mode_is_in_set(deref, modes))
            const struct glsl_type *vec4_type =
         if (deref->type != vec4_type) {
      deref->type = vec4_type;
      }
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
                     nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                                 b->cursor = nir_after_instr(&intrin->instr);
   nir_def *vec3 = nir_trim_vector(b, &intrin->def, 3);
   nir_def_rewrite_uses_after(&intrin->def,
                           case nir_intrinsic_store_deref: {
                     nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                           b->cursor = nir_before_instr(&intrin->instr);
                  intrin->num_components = 4;
   nir_src_rewrite(&intrin->src[1], data);
               case nir_intrinsic_copy_deref: {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   nir_deref_instr *src = nir_src_as_deref(intrin->src[0]);
   /* If we convert once side of a copy and not the other, that
   * would be very bad.
   */
   if (nir_deref_mode_may_be(dst, modes) ||
      nir_deref_mode_may_be(src, modes)) {
   assert(nir_deref_mode_must_be(dst, modes));
      }
               default:
         }
               default:
                     }
      bool
   nir_lower_vec3_to_vec4(nir_shader *shader, nir_variable_mode modes)
   {
               if (modes & ~nir_var_function_temp) {
      nir_foreach_variable_in_shader(var, shader) {
                     const struct glsl_type *vec4_type =
         if (var->type != vec4_type) {
      var->type = vec4_type;
                     if (modes & nir_var_function_temp) {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_function_temp_variable(var, impl) {
      const struct glsl_type *vec4_type =
         if (var->type != vec4_type) {
      var->type = vec4_type;
                        progress |= nir_shader_instructions_pass(shader,
                                 }
