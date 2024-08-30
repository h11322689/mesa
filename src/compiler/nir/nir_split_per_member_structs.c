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
      #include "nir.h"
   #include "nir_deref.h"
      struct split_struct_state {
                  };
      static nir_variable *
   find_var_member(struct nir_variable *var, unsigned member,
         {
      struct hash_entry *map_entry =
         if (map_entry == NULL)
            nir_variable **members = map_entry->data;
   assert(member < var->num_members);
      }
      static const struct glsl_type *
   member_type(const struct glsl_type *type, unsigned index)
   {
      if (glsl_type_is_array(type)) {
      const struct glsl_type *elem =
         assert(glsl_get_explicit_stride(type) == 0);
      } else {
      assert(glsl_type_is_struct_or_ifc(type));
   assert(index < glsl_get_length(type));
         }
      static void
   split_variable(struct nir_variable *var, nir_shader *shader,
         {
               /* Constant initializers are currently not handled */
            nir_variable **members =
            for (unsigned i = 0; i < var->num_members; i++) {
      char *member_name = NULL;
   if (var->name) {
      /* Calculate a reasonable variable name */
   member_name = ralloc_strdup(dead_ctx, var->name);
   const struct glsl_type *t = var->type;
   while (glsl_type_is_array(t)) {
      ralloc_strcat(&member_name, "[*]");
      }
   const char *field_name = glsl_get_struct_elem_name(t, i);
   if (field_name) {
      member_name = ralloc_asprintf(dead_ctx, "%s.%s",
      } else {
                     members[i] =
      nir_variable_create(shader, var->members[i].mode,
      if (var->interface_type) {
      members[i]->interface_type =
      }
                  }
      static nir_deref_instr *
   build_member_deref(nir_builder *b, nir_deref_instr *deref, nir_variable *member)
   {
      if (deref->deref_type == nir_deref_type_var) {
         } else {
      nir_deref_instr *parent =
               }
      static bool
   rewrite_deref_instr(nir_builder *b, nir_instr *instr, void *cb_data)
   {
      if (instr->type != nir_instr_type_deref)
            nir_deref_instr *deref = nir_instr_as_deref(instr);
            /* We must be a struct deref */
   if (deref->deref_type != nir_deref_type_struct)
            nir_deref_instr *base;
   for (base = nir_deref_instr_parent(deref);
      base && base->deref_type != nir_deref_type_var;
            /* If this struct is nested inside another, bail */
   if (base->deref_type == nir_deref_type_struct)
               /* We must be on a variable with members */
   if (!base || base->var->num_members == 0)
            nir_variable *member = find_var_member(base->var, deref->strct.index,
                  b->cursor = nir_before_instr(&deref->instr);
   nir_deref_instr *member_deref =
         nir_def_rewrite_uses(&deref->def,
            /* The referenced variable is no longer valid, clean up the deref */
               }
      bool
   nir_split_per_member_structs(nir_shader *shader)
   {
      bool progress = false;
   void *dead_ctx = ralloc_context(NULL);
   struct hash_table *var_to_member_map =
            nir_foreach_variable_with_modes_safe(var, shader, nir_var_shader_in | nir_var_shader_out | nir_var_system_value) {
      if (var->num_members == 0)
            split_variable(var, shader, var_to_member_map, dead_ctx);
   exec_node_remove(&var->node);
               if (!progress) {
      ralloc_free(dead_ctx);
               nir_shader_instructions_pass(shader, rewrite_deref_instr,
                                    }
