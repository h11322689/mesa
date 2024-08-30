   /*
   * Copyright © 2019 Collabora Ltd.
   * Copyright © 2022 Valve Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "gl_nir_linker.h"
   #include "main/shader_types.h"
   #include "util/strndup.h"
      static char*
   get_field_name(const char *name)
   {
      const char *first_dot = strchr(name, '.');
   const char *first_square_bracket = strchr(name, '[');
            if (!first_square_bracket && !first_dot)
         else if ((!first_square_bracket ||
               else
               }
      /* Generate a new name given the old xfb declaration string by replacing dots
   * with '_', brackets with '@' and appending "-xfb" */
   static char *
   generate_new_name(void *mem_ctx, const char *name)
   {
      char *new_name;
            new_name = ralloc_strdup(mem_ctx, name);
   while (new_name[i]) {
      if (new_name[i] == '.') {
         } else if (new_name[i] == '[' || new_name[i] == ']') {
         }
               if (!ralloc_strcat(&new_name, "-xfb")) {
      ralloc_free(new_name);
                  }
      /* Get the dereference for the given variable name. The method is called
   * recursively to parse array indices and struct members. */
   static bool
   get_deref(nir_builder *b, const char *name, nir_variable *toplevel_var,
         {
      if (name[0] == '\0') {
      /* End */
      } else if (name[0] == '[') {
      /* Array index */
   char *endptr = NULL;
   unsigned index = strtol(name + 1, &endptr, 10);
            nir_load_const_instr *c = nir_load_const_instr_create(b->shader, 1, 32);
   c->value[0].u32 = index;
            *deref = nir_build_deref_array(b, *deref, &c->def);
   *type = glsl_without_array(*type);
      } else if (name[0] == '.') {
      /* Struct member */
                     int idx = glsl_get_field_index(*type, field);
   *deref = nir_build_deref_struct(b, *deref, idx);
   *type = glsl_get_struct_field(*type, idx);
   name += 1 + strlen(field);
   free(field);
      } else {
      /* Top level variable */
   char *field = get_field_name(name);
   name += strlen(field);
   free(field);
   if (toplevel_var == NULL) {
                  *deref = nir_build_deref_var(b, toplevel_var);
   *type = toplevel_var->type;
         }
      static void
   copy_to_new_var(nir_builder *b, nir_deref_instr *deref,
         {
      bool is_matrix = glsl_type_is_matrix(type);
   unsigned components = glsl_get_vector_elements(type);
            if (is_matrix) {
      unsigned array_size = glsl_get_length(type);
   for (unsigned i = 0; i < array_size; i++) {
      nir_load_const_instr *c = nir_load_const_instr_create(b->shader, 1, 32);
                  nir_deref_instr *m_deref = nir_build_deref_array(b, deref, &c->def);
                  nir_def *value = nir_load_deref(b, m_deref);
         } else {
      nir_def *value = nir_load_deref(b, deref);
         }
      nir_variable *
   gl_nir_lower_xfb_varying(nir_shader *shader, const char *old_var_name,
         {
                        nir_deref_instr *deref = NULL;
   const struct glsl_type *type = NULL;
   if (!get_deref(&b, old_var_name, toplevel_var, &deref, &type))
            nir_variable *new_variable = rzalloc(shader, nir_variable);
   new_variable->name = generate_new_name(new_variable, old_var_name);
   new_variable->type = type;
   new_variable->data.mode = nir_var_shader_out;
   new_variable->data.location = -1;
   new_variable->data.xfb.buffer = -1;
   new_variable->data.xfb.stride = -1;
   new_variable->data.assigned = true;
   nir_shader_add_variable(shader, new_variable);
            nir_foreach_block(block, impl) {
      if (shader->info.stage != MESA_SHADER_GEOMETRY) {
      /* For shaders other than geometry, outputs need to be lowered before
   * each return statement and at the end of main()
   */
   if (nir_block_ends_in_return_or_halt(block)) {
      b.cursor = nir_before_instr(nir_block_last_instr(block));
      } else if (block == nir_impl_last_block(impl)) {
      b.cursor = nir_after_instr(nir_block_last_instr(block));
         } else {
   /* For geometry shaders, outputs need to be lowered before each call
      * to EmitVertex()
   */
   nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  b.cursor = nir_before_instr(instr);
                        }
