   /*
   * Copyright © 2021 Valve Corporation
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
      /**
   * Lower constant arrays to uniform arrays.
   *
   * Some driver backends (such as i965 and nouveau) don't handle constant arrays
   * gracefully, instead treating them as ordinary writable temporary arrays.
   * Since arrays can be large, this often means spilling them to scratch memory,
   * which usually involves a large number of instructions.
   *
   * This must be called prior to gl_nir_set_uniform_initializers(); we need the
   * linker to process our new uniform's constant initializer.
   *
   * This should be called after optimizations, since those can result in
   * splitting and removing arrays that are indexed by constant expressions.
   */
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      struct var_info {
               bool is_constant;
            /* Block that has all the variable stores.  All the blocks with reads
   * should be dominated by this block.
   */
      };
      static void
   set_const_initialiser(nir_deref_instr **p, nir_constant *top_level_init,
         {
               nir_constant *ret = top_level_init;
   for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
                        /* Just return if this is an out of bounds write */
                     } else if ((*p)->deref_type == nir_deref_type_struct) {
         } else {
                     /* Now that we have selected the corrent nir_constant we copy the constant
   * values to it.
   */
   nir_instr *src_instr = const_src->ssa->parent_instr;
   assert(src_instr->type == nir_instr_type_load_const);
            for (unsigned i = 0; i < load->def.num_components; i++) {
      if (!(writemask & (1 << i)))
                           }
      static nir_constant *
   rebuild_const_array_initialiser(const struct glsl_type *type, void *mem_ctx)
   {
               if (glsl_type_is_matrix(type) && glsl_get_matrix_columns(type) > 1) {
      ret->num_elements = glsl_get_matrix_columns(type);
            for (unsigned i = 0; i < ret->num_elements; i++) {
                              if (glsl_type_is_array(type) || glsl_type_is_struct(type)) {
      ret->num_elements = glsl_get_length(type);
            for (unsigned i = 0; i < ret->num_elements; i++) {
      if (glsl_type_is_array(type)) {
      ret->elements[i] =
      } else {
      ret->elements[i] =
                        }
      static bool
   lower_const_array_to_uniform(nir_shader *shader, struct var_info *info,
                     {
               if (!info->is_constant)
            if (!glsl_type_is_array(var->type))
            /* TODO: Add support for 8bit and 16bit types */
   if (!glsl_type_is_32bit(glsl_without_array(var->type)) &&
      !glsl_type_is_64bit(glsl_without_array(var->type)))
         /* How many uniform component slots are required? */
            /* We would utilize more than is available, bail out. */
   if (component_slots > *free_uni_components)
                     /* In the very unlikely event of 4294967295 constant arrays in a single
   * shader, don't promote this to a uniform.
   */
   unsigned limit = ~0;
   if (*const_count == limit)
                     /* Rebuild constant initialiser */
            /* Set constant initialiser */
   nir_function_impl *impl = nir_shader_get_entrypoint(shader);
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   assert(intrin->intrinsic != nir_intrinsic_copy_deref);
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   nir_variable *deref_var = nir_deref_instr_get_variable(deref);
                  nir_deref_path path;
                  nir_deref_instr **p = &path.path[1];
                                 uni->constant_initializer = const_init;
   uni->data.how_declared = nir_var_hidden;
   uni->data.read_only = true;
   uni->data.mode = nir_var_uniform;
   uni->type = info->var->type;
   uni->name = ralloc_asprintf(uni, "constarray_%x_%u",
                                                   }
      static unsigned
   count_uniforms(nir_shader *shader)
   {
               nir_foreach_variable_with_modes(var, shader, nir_var_uniform) {
                     }
      bool
   nir_lower_const_arrays_to_uniforms(nir_shader *shader,
         {
      /* This only works with a single entrypoint */
            unsigned num_locals = nir_function_impl_index_vars(impl);
   if (num_locals == 0) {
      nir_shader_preserve_all_metadata(shader);
               bool progress = false;
   unsigned uniform_components = count_uniforms(shader);
   unsigned free_uni_components = max_uniform_components - uniform_components;
            struct var_info *var_infos = ralloc_array(NULL, struct var_info, num_locals);
   nir_foreach_function_temp_variable(var, impl) {
      var_infos[var->index] = (struct var_info){
      .var = var,
   .is_constant = true,
                           struct hash_table *const_array_vars =
            /* First, walk through the shader and figure out what variables we can
   * lower to a uniform.
   */
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_deref) {
      /* If we ever see a complex use of a deref_var, we have to assume
   * that variable is non-constant because we can't guarantee we
   * will find all of the writers of that variable.
   */
   nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type == nir_deref_type_var &&
      deref->var->data.mode == nir_var_function_temp &&
   nir_deref_instr_has_complex_use(deref, 0))
                                          bool src_is_const = false;
   nir_deref_instr *src_deref = NULL, *dst_deref = NULL;
   switch (intrin->intrinsic) {
   case nir_intrinsic_store_deref:
      dst_deref = nir_src_as_deref(intrin->src[0]);
               case nir_intrinsic_load_deref:
                  case nir_intrinsic_copy_deref:
                  default:
                  if (dst_deref && nir_deref_mode_must_be(dst_deref, nir_var_function_temp)) {
      nir_variable *var = nir_deref_instr_get_variable(dst_deref);
                           struct var_info *info = &var_infos[var->index];
                                 /* We only consider variables constant if they only have constant
   * stores, all the stores come before any reads, and all stores
   * come from the same block.  We also can't handle indirect stores.
   */
   if (!src_is_const || info->found_read || block != info->block ||
      nir_deref_instr_has_indirect(dst_deref)) {
                  if (src_deref && nir_deref_mode_must_be(src_deref, nir_var_function_temp)) {
      nir_variable *var = nir_deref_instr_get_variable(src_deref);
                           /* We only consider variables constant if all the reads are
   * dominated by the block that writes to it.
   */
   struct var_info *info = &var_infos[var->index];
                                                   /* Now lower the constants to uniforms */
   for (int i = 0; i < num_locals; i++) {
      struct var_info *info = &var_infos[i];
   if (!lower_const_array_to_uniform(shader, info, const_array_vars,
                           /* Finally rewrite its uses */
   nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
                                 nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                                 struct hash_entry *entry =
                                                nir_deref_path path;
                  nir_deref_instr **p = &path.path[1];
   for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
      new_deref_instr = nir_build_deref_array(&b, new_deref_instr,
      } else if ((*p)->deref_type == nir_deref_type_struct) {
      new_deref_instr = nir_build_deref_struct(&b, new_deref_instr,
      } else {
                                    nir_def_rewrite_uses(&intrin->def, new_def);
                  nir_metadata_preserve(impl, nir_metadata_block_index |
            ralloc_free(var_infos);
               }
