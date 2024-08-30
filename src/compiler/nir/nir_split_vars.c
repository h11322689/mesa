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
   #include "nir_builder.h"
   #include "nir_deref.h"
   #include "nir_vla.h"
      #include "util/set.h"
   #include "util/u_math.h"
      static struct set *
   get_complex_used_vars(nir_shader *shader, void *mem_ctx)
   {
               nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                              /* We only need to consider var derefs because
   * nir_deref_instr_has_complex_use is recursive.
   */
   if (deref->deref_type == nir_deref_type_var &&
      nir_deref_instr_has_complex_use(deref,
                           }
      struct split_var_state {
               nir_shader *shader;
               };
      struct field {
                        unsigned num_fields;
            /* The field currently being recursed */
               };
      static int
   num_array_levels_in_array_of_vector_type(const struct glsl_type *type)
   {
      int num_levels = 0;
   while (true) {
      if (glsl_type_is_array_or_matrix(type)) {
      num_levels++;
      } else if (glsl_type_is_vector_or_scalar(type) &&
            /* glsl_type_is_vector_or_scalar would more accruately be called "can
   * be an r-value that isn't an array, structure, or matrix. This
   * optimization pass really shouldn't do anything to cooperative
   * matrices. These matrices will eventually be lowered to something
   * else (dependent on the backend), and that thing may (or may not)
   * be handled by this or another pass.
   */
      } else {
      /* Not an array of vectors */
            }
      static nir_constant *
   gather_constant_initializers(nir_constant *src,
                           {
      if (!src)
         if (glsl_type_is_array(type)) {
      const struct glsl_type *element = glsl_get_array_element(type);
   assert(src->num_elements == glsl_get_length(type));
   nir_constant *dst = rzalloc(var, nir_constant);
   dst->num_elements = src->num_elements;
   dst->elements = rzalloc_array(var, nir_constant *, src->num_elements);
   for (unsigned i = 0; i < src->num_elements; ++i) {
         }
      } else if (glsl_type_is_struct(type)) {
      const struct glsl_type *element = glsl_get_struct_field(type, field->current_index);
      } else {
            }
      static void
   init_field_for_type(struct field *field, struct field *parent,
                     {
      *field = (struct field){
      .parent = parent,
               const struct glsl_type *struct_type = glsl_without_array(type);
   if (glsl_type_is_struct_or_ifc(struct_type)) {
      field->num_fields = glsl_get_length(struct_type),
   field->fields = ralloc_array(state->mem_ctx, struct field,
         for (unsigned i = 0; i < field->num_fields; i++) {
      char *field_name = NULL;
   if (name) {
      field_name = ralloc_asprintf(state->mem_ctx, "%s_%s", name,
      } else {
      field_name = ralloc_asprintf(state->mem_ctx, "{unnamed %s}_%s",
            }
   field->current_index = i;
   init_field_for_type(&field->fields[i], field,
               } else {
      const struct glsl_type *var_type = type;
   struct field *root = field;
   for (struct field *f = field->parent; f; f = f->parent) {
      var_type = glsl_type_wrap_in_arrays(var_type, f->type);
               nir_variable_mode mode = state->base_var->data.mode;
   if (mode == nir_var_function_temp) {
         } else {
         }
   field->var->data.ray_query = state->base_var->data.ray_query;
   field->var->constant_initializer = gather_constant_initializers(state->base_var->constant_initializer,
               }
      static bool
   split_var_list_structs(nir_shader *shader,
                        nir_function_impl *impl,
   struct exec_list *vars,
      {
      struct split_var_state state = {
      .mem_ctx = mem_ctx,
   .shader = shader,
               struct exec_list split_vars;
            /* To avoid list confusion (we'll be adding things as we split variables),
   * pull all of the variables we plan to split off of the list
   */
   nir_foreach_variable_in_list_safe(var, vars) {
      if (var->data.mode != mode)
            if (!glsl_type_is_struct_or_ifc(glsl_without_array(var->type)))
            if (*complex_vars == NULL)
            /* We can't split a variable that's referenced with deref that has any
   * sort of complex usage.
   */
   if (_mesa_set_search(*complex_vars, var))
            exec_node_remove(&var->node);
               nir_foreach_variable_in_list(var, &split_vars) {
               struct field *root_field = ralloc(mem_ctx, struct field);
   init_field_for_type(root_field, NULL, var->type, var->name, &state);
                  }
      static void
   split_struct_derefs_impl(nir_function_impl *impl,
                     {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_deref_instr *deref = nir_instr_as_deref(instr);
                  /* Clean up any dead derefs we find lying around.  They may refer to
   * variables we're planning to split.
   */
                                 nir_variable *base_var = nir_deref_instr_get_variable(deref);
   /* If we can't chase back to the variable, then we're a complex use.
   * This should have been detected by get_complex_used_vars() and the
   * variable should not have been split.  However, we have no way of
   * knowing that here, so we just have to trust it.
   */
                  struct hash_entry *entry =
                                                struct field *tail_field = root_field;
   for (unsigned i = 0; path.path[i]; i++) {
                     assert(i > 0);
   assert(glsl_type_is_struct_or_ifc(path.path[i - 1]->type));
                                    nir_deref_instr *new_deref = NULL;
   for (unsigned i = 0; path.path[i]; i++) {
                     switch (p->deref_type) {
   case nir_deref_type_var:
                        case nir_deref_type_array:
   case nir_deref_type_array_wildcard:
                  case nir_deref_type_struct:
                  default:
                     assert(new_deref->type == deref->type);
   nir_def_rewrite_uses(&deref->def,
                  }
      /** A pass for splitting structs into multiple variables
   *
   * This pass splits arrays of structs into multiple variables, one for each
   * (possibly nested) structure member.  After this pass completes, no
   * variables of the given mode will contain a struct type.
   */
   bool
   nir_split_struct_vars(nir_shader *shader, nir_variable_mode modes)
   {
      void *mem_ctx = ralloc_context(NULL);
   struct hash_table *var_field_map =
                  bool has_global_splits = false;
   nir_variable_mode global_modes = modes & ~nir_var_function_temp;
   if (global_modes) {
      has_global_splits = split_var_list_structs(shader, NULL,
                                       bool progress = false;
   nir_foreach_function_impl(impl, shader) {
      bool has_local_splits = false;
   if (modes & nir_var_function_temp) {
      has_local_splits = split_var_list_structs(shader, impl,
                                       if (has_global_splits || has_local_splits) {
                     nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                                 }
      struct array_level_info {
      unsigned array_len;
      };
      struct array_split {
      /* Only set if this is the tail end of the splitting */
            unsigned num_splits;
      };
      struct array_var_info {
                        bool split_var;
            unsigned num_levels;
      };
      static bool
   init_var_list_array_infos(nir_shader *shader,
                           struct exec_list *vars,
   {
               nir_foreach_variable_in_list(var, vars) {
      if (var->data.mode != mode)
            int num_levels = num_array_levels_in_array_of_vector_type(var->type);
   if (num_levels <= 0)
            if (*complex_vars == NULL)
            /* We can't split a variable that's referenced with deref that has any
   * sort of complex usage.
   */
   if (_mesa_set_search(*complex_vars, var))
            struct array_var_info *info =
                  info->base_var = var;
            const struct glsl_type *type = var->type;
   for (int i = 0; i < num_levels; i++) {
                     /* All levels start out initially as split */
               _mesa_hash_table_insert(var_info_map, var, info);
                  }
      static struct array_var_info *
   get_array_var_info(nir_variable *var,
         {
      struct hash_entry *entry =
            }
      static struct array_var_info *
   get_array_deref_info(nir_deref_instr *deref,
               {
      if (!nir_deref_mode_may_be(deref, modes))
            nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var == NULL)
               }
      static void
   mark_array_deref_used(nir_deref_instr *deref,
                     {
      struct array_var_info *info =
         if (!info)
            nir_deref_path path;
            /* Walk the path and look for indirects.  If we have an array deref with an
   * indirect, mark the given level as not being split.
   */
   for (unsigned i = 0; i < info->num_levels; i++) {
      nir_deref_instr *p = path.path[i + 1];
   if (p->deref_type == nir_deref_type_array &&
      !nir_src_is_const(p->arr.index))
      }
      static void
   mark_array_usage_impl(nir_function_impl *impl,
                     {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_copy_deref:
      mark_array_deref_used(nir_src_as_deref(intrin->src[1]),
               case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
      mark_array_deref_used(nir_src_as_deref(intrin->src[0]),
               default:
                  }
      static void
   create_split_array_vars(struct array_var_info *var_info,
                           unsigned level,
   struct array_split *split,
   {
      while (level < var_info->num_levels && !var_info->levels[level].split) {
      name = ralloc_asprintf(mem_ctx, "%s[*]", name);
               if (level == var_info->num_levels) {
      /* We add parens to the variable name so it looks like "(foo[2][*])" so
   * that further derefs will look like "(foo[2][*])[ssa_6]"
   */
            nir_variable_mode mode = var_info->base_var->data.mode;
   if (mode == nir_var_function_temp) {
      split->var = nir_local_variable_create(impl,
      } else {
      split->var = nir_variable_create(shader, mode,
      }
      } else {
      assert(var_info->levels[level].split);
   split->num_splits = var_info->levels[level].array_len;
   split->splits = rzalloc_array(mem_ctx, struct array_split,
         for (unsigned i = 0; i < split->num_splits; i++) {
      create_split_array_vars(var_info, level + 1, &split->splits[i],
                  }
      static bool
   split_var_list_arrays(nir_shader *shader,
                        nir_function_impl *impl,
      {
      struct exec_list split_vars;
            nir_foreach_variable_in_list_safe(var, vars) {
      if (var->data.mode != mode)
            struct array_var_info *info = get_array_var_info(var, var_info_map);
   if (!info)
            bool has_split = false;
   const struct glsl_type *split_type =
         for (int i = info->num_levels - 1; i >= 0; i--) {
      if (info->levels[i].split) {
      has_split = true;
               /* If the original type was a matrix type, we'd like to keep that so
   * we don't convert matrices into arrays.
   */
   if (i == info->num_levels - 1 &&
      glsl_type_is_matrix(glsl_without_array(var->type))) {
   split_type = glsl_matrix_type(glsl_get_base_type(split_type),
            } else {
                     if (has_split) {
      info->split_var_type = split_type;
   /* To avoid list confusion (we'll be adding things as we split
   * variables), pull all of the variables we plan to split off of the
   * main variable list.
   */
   exec_node_remove(&var->node);
      } else {
      assert(split_type == glsl_get_bare_type(var->type));
   /* If we're not modifying this variable, delete the info so we skip
   * it faster in later passes.
   */
                  nir_foreach_variable_in_list(var, &split_vars) {
      struct array_var_info *info = get_array_var_info(var, var_info_map);
   create_split_array_vars(info, 0, &info->root_split, var->name,
                  }
      static bool
   deref_has_split_wildcard(nir_deref_path *path,
         {
      if (info == NULL)
            assert(path->path[0]->var == info->base_var);
   for (unsigned i = 0; i < info->num_levels; i++) {
      if (path->path[i + 1]->deref_type == nir_deref_type_array_wildcard &&
      info->levels[i].split)
               }
      static bool
   array_path_is_out_of_bounds(nir_deref_path *path,
         {
      if (info == NULL)
            assert(path->path[0]->var == info->base_var);
   for (unsigned i = 0; i < info->num_levels; i++) {
      nir_deref_instr *p = path->path[i + 1];
   if (p->deref_type == nir_deref_type_array_wildcard)
            if (nir_src_is_const(p->arr.index) &&
      nir_src_as_uint(p->arr.index) >= info->levels[i].array_len)
               }
      static void
   emit_split_copies(nir_builder *b,
                     struct array_var_info *dst_info, nir_deref_path *dst_path,
   {
               while ((dst_p = dst_path->path[dst_level + 1])) {
      if (dst_p->deref_type == nir_deref_type_array_wildcard)
            dst = nir_build_deref_follower(b, dst, dst_p);
               while ((src_p = src_path->path[src_level + 1])) {
      if (src_p->deref_type == nir_deref_type_array_wildcard)
            src = nir_build_deref_follower(b, src, src_p);
               if (src_p == NULL || dst_p == NULL) {
      assert(src_p == NULL && dst_p == NULL);
      } else {
      assert(dst_p->deref_type == nir_deref_type_array_wildcard &&
            if ((dst_info && dst_info->levels[dst_level].split) ||
      (src_info && src_info->levels[src_level].split)) {
   /* There are no indirects at this level on one of the source or the
   * destination so we are lowering it.
   */
   assert(glsl_get_length(dst_path->path[dst_level]->type) ==
         unsigned len = glsl_get_length(dst_path->path[dst_level]->type);
   for (unsigned i = 0; i < len; i++) {
      emit_split_copies(b, dst_info, dst_path, dst_level + 1,
                     } else {
      /* Neither side is being split so we just keep going */
   emit_split_copies(b, dst_info, dst_path, dst_level + 1,
                        }
      static void
   split_array_copies_impl(nir_function_impl *impl,
                     {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *copy = nir_instr_as_intrinsic(instr);
                                 struct array_var_info *dst_info =
                                       nir_deref_path dst_path, src_path;
                  if (!deref_has_split_wildcard(&dst_path, dst_info) &&
                           emit_split_copies(&b, dst_info, &dst_path, 0, dst_path.path[0],
            }
      static void
   split_array_access_impl(nir_function_impl *impl,
                     {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_deref) {
      /* Clean up any dead derefs we find lying around.  They may refer
   * to variables we're planning to split.
   */
   nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (nir_deref_mode_may_be(deref, modes))
                                    nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != nir_intrinsic_load_deref &&
      intrin->intrinsic != nir_intrinsic_store_deref &&
                                                struct array_var_info *info =
                                                if (array_path_is_out_of_bounds(&path, info)) {
      /* If one of the derefs is out-of-bounds, we just delete the
   * instruction.  If a destination is out of bounds, then it may
   * have been in-bounds prior to shrinking so we don't want to
   * accidentally stomp something.  However, we've already proven
   * that it will never be read so it's safe to delete.  If a
   * source is out of bounds then it is loading random garbage.
   * For loads, we replace their uses with an undef instruction
   * and for copies we just delete the copy since it was writing
   * undefined garbage anyway and we may as well leave the random
   * garbage in the destination alone.
   */
   if (intrin->intrinsic == nir_intrinsic_load_deref) {
      nir_def *u =
      nir_undef(&b, intrin->def.num_components,
      nir_def_rewrite_uses(&intrin->def,
      }
   nir_instr_remove(&intrin->instr);
   for (unsigned i = 0; i < num_derefs; i++)
                     struct array_split *split = &info->root_split;
   for (unsigned i = 0; i < info->num_levels; i++) {
      if (info->levels[i].split) {
      nir_deref_instr *p = path.path[i + 1];
   unsigned index = nir_src_as_uint(p->arr.index);
   assert(index < info->levels[i].array_len);
                        nir_deref_instr *new_deref = nir_build_deref_var(&b, split->var);
   for (unsigned i = 0; i < info->num_levels; i++) {
      if (!info->levels[i].split) {
      new_deref = nir_build_deref_follower(&b, new_deref,
                        /* Rewrite the deref source to point to the split one */
   nir_src_rewrite(&intrin->src[d], &new_deref->def);
               }
      /** A pass for splitting arrays of vectors into multiple variables
   *
   * This pass looks at arrays (possibly multiple levels) of vectors (not
   * structures or other types) and tries to split them into piles of variables,
   * one for each array element.  The heuristic used is simple: If a given array
   * level is never used with an indirect, that array level will get split.
   *
   * This pass probably could handles structures easily enough but making a pass
   * that could see through an array of structures of arrays would be difficult
   * so it's best to just run nir_split_struct_vars first.
   */
   bool
   nir_split_array_vars(nir_shader *shader, nir_variable_mode modes)
   {
      void *mem_ctx = ralloc_context(NULL);
   struct hash_table *var_info_map = _mesa_pointer_hash_table_create(mem_ctx);
                     bool has_global_array = false;
   if (modes & (nir_var_shader_temp | nir_var_ray_hit_attrib)) {
      has_global_array = init_var_list_array_infos(shader,
                                       bool has_any_array = false;
   nir_foreach_function_impl(impl, shader) {
      bool has_local_array = false;
   if (modes & nir_var_function_temp) {
      has_local_array = init_var_list_array_infos(shader,
                                       if (has_global_array || has_local_array) {
      has_any_array = true;
                  /* If we failed to find any arrays of arrays, bail early. */
   if (!has_any_array) {
      ralloc_free(mem_ctx);
   nir_shader_preserve_all_metadata(shader);
               bool has_global_splits = false;
   if (modes & (nir_var_shader_temp | nir_var_ray_hit_attrib)) {
      has_global_splits = split_var_list_arrays(shader, NULL,
                           bool progress = false;
   nir_foreach_function_impl(impl, shader) {
      bool has_local_splits = false;
   if (modes & nir_var_function_temp) {
      has_local_splits = split_var_list_arrays(shader, impl,
                           if (has_global_splits || has_local_splits) {
                     nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                                 }
      struct array_level_usage {
               /* The value UINT_MAX will be used to indicate an indirect */
   unsigned max_read;
            /* True if there is a copy that isn't to/from a shrinkable array */
   bool has_external_copy;
      };
      struct vec_var_usage {
      /* Convenience set of all components this variable has */
            nir_component_mask_t comps_read;
                     /* True if there is a copy that isn't to/from a shrinkable vector */
   bool has_external_copy;
   bool has_complex_use;
            unsigned num_levels;
      };
      static struct vec_var_usage *
   get_vec_var_usage(nir_variable *var,
               {
      struct hash_entry *entry = _mesa_hash_table_search(var_usage_map, var);
   if (entry)
            if (!add_usage_entry)
            /* Check to make sure that we are working with an array of vectors.  We
   * don't bother to shrink single vectors because we figure that we can
   * clean it up better with SSA than by inserting piles of vecN instructions
   * to compact results.
   */
   int num_levels = num_array_levels_in_array_of_vector_type(var->type);
   if (num_levels < 1)
            struct vec_var_usage *usage =
      rzalloc_size(mem_ctx, sizeof(*usage) +
         usage->num_levels = num_levels;
   const struct glsl_type *type = var->type;
   for (unsigned i = 0; i < num_levels; i++) {
      usage->levels[i].array_len = glsl_get_length(type);
      }
                                 }
      static struct vec_var_usage *
   get_vec_deref_usage(nir_deref_instr *deref,
                     {
      if (!nir_deref_mode_may_be(deref, modes))
            nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var == NULL)
            return get_vec_var_usage(nir_deref_instr_get_variable(deref),
      }
      static void
   mark_deref_if_complex(nir_deref_instr *deref,
                     {
      /* Only bother with var derefs because nir_deref_instr_has_complex_use is
   * recursive.
   */
   if (deref->deref_type != nir_deref_type_var)
            if (!(deref->var->data.mode & modes))
            if (!nir_deref_instr_has_complex_use(deref, nir_deref_instr_has_complex_use_allow_atomics))
            struct vec_var_usage *usage =
         if (!usage)
               }
      static void
   mark_deref_used(nir_deref_instr *deref,
                  nir_component_mask_t comps_read,
   nir_component_mask_t comps_written,
   nir_deref_instr *copy_deref,
      {
      if (!nir_deref_mode_may_be(deref, modes))
            nir_variable *var = nir_deref_instr_get_variable(deref);
   if (var == NULL)
            struct vec_var_usage *usage =
         if (!usage)
            usage->comps_read |= comps_read & usage->all_comps;
            struct vec_var_usage *copy_usage = NULL;
   if (copy_deref) {
      copy_usage = get_vec_deref_usage(copy_deref, var_usage_map, modes,
         if (copy_usage) {
      if (usage->vars_copied == NULL) {
         }
      } else {
                     nir_deref_path path;
            nir_deref_path copy_path;
   if (copy_usage)
            unsigned copy_i = 0;
   for (unsigned i = 0; i < usage->num_levels; i++) {
      struct array_level_usage *level = &usage->levels[i];
   nir_deref_instr *deref = path.path[i + 1];
   assert(deref->deref_type == nir_deref_type_array ||
            unsigned max_used;
   if (deref->deref_type == nir_deref_type_array) {
         } else {
      /* For wildcards, we read or wrote the whole thing. */
                  if (copy_usage) {
      /* Match each wildcard level with the level on copy_usage */
   for (; copy_path.path[copy_i + 1]; copy_i++) {
      if (copy_path.path[copy_i + 1]->deref_type ==
      nir_deref_type_array_wildcard)
   }
                  if (level->levels_copied == NULL) {
         }
      } else {
      /* We have a wildcard and it comes from a variable we aren't
   * tracking; flag it and we'll know to not shorten this array.
   */
                  if (comps_written)
         if (comps_read)
         }
      static bool
   src_is_load_deref(nir_src src, nir_src deref_src)
   {
      nir_intrinsic_instr *load = nir_src_as_intrinsic(src);
   if (load == NULL || load->intrinsic != nir_intrinsic_load_deref)
               }
      /* Returns all non-self-referential components of a store instruction.  A
   * component is self-referential if it comes from the same component of a load
   * instruction on the same deref.  If the only data in a particular component
   * of a variable came directly from that component then it's undefined.  The
   * only way to get defined data into a component of a variable is for it to
   * get written there by something outside or from a different component.
   *
   * This is a fairly common pattern in shaders that come from either GLSL IR or
   * GLSLang because both glsl_to_nir and GLSLang implement write-masking with
   * load-vec-store.
   */
   static nir_component_mask_t
   get_non_self_referential_store_comps(nir_intrinsic_instr *store)
   {
               nir_instr *src_instr = store->src[1].ssa->parent_instr;
   if (src_instr->type != nir_instr_type_alu)
                     if (src_alu->op == nir_op_mov) {
      /* If it's just a swizzle of a load from the same deref, discount any
   * channels that don't move in the swizzle.
   */
   if (src_is_load_deref(src_alu->src[0].src, store->src[0])) {
      for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++) {
      if (src_alu->src[0].swizzle[i] == i)
            } else if (nir_op_is_vec(src_alu->op)) {
      /* If it's a vec, discount any channels that are just loads from the
   * same deref put in the same spot.
   */
   for (unsigned i = 0; i < nir_op_infos[src_alu->op].num_inputs; i++) {
      if (src_is_load_deref(src_alu->src[i].src, store->src[0]) &&
      src_alu->src[i].swizzle[0] == i)
                  }
      static void
   find_used_components_impl(nir_function_impl *impl,
                     {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_deref) {
      mark_deref_if_complex(nir_instr_as_deref(instr),
                              nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   switch (intrin->intrinsic) {
   case nir_intrinsic_load_deref:
      mark_deref_used(nir_src_as_deref(intrin->src[0]),
                     case nir_intrinsic_store_deref:
      mark_deref_used(nir_src_as_deref(intrin->src[0]),
                     case nir_intrinsic_copy_deref: {
      /* Just mark everything used for copies. */
   nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   nir_deref_instr *src = nir_src_as_deref(intrin->src[1]);
   mark_deref_used(dst, 0, ~0, src, var_usage_map, modes, mem_ctx);
   mark_deref_used(src, ~0, 0, dst, var_usage_map, modes, mem_ctx);
               default:
                  }
      static bool
   shrink_vec_var_list(struct exec_list *vars,
               {
      /* Initialize the components kept field of each variable.  This is the
   * AND of the components written and components read.  If a component is
   * written but never read, it's dead.  If it is read but never written,
   * then all values read are undefined garbage and we may as well not read
   * them.
   *
   * The same logic applies to the array length.  We make the array length
   * the minimum needed required length between read and write and plan to
   * discard any OOB access.  The one exception here is indirect writes
   * because we don't know where they will land and we can't shrink an array
   * with indirect writes because previously in-bounds writes may become
   * out-of-bounds and have undefined behavior.
   *
   * Also, if we have a copy that to/from something we can't shrink, we need
   * to leave components and array_len of any wildcards alone.
   */
   nir_foreach_variable_in_list(var, vars) {
      if (var->data.mode != mode)
            struct vec_var_usage *usage =
         if (!usage)
            assert(usage->comps_kept == 0);
   if (usage->has_external_copy || usage->has_complex_use)
         else
            for (unsigned i = 0; i < usage->num_levels; i++) {
                     if (level->max_written == UINT_MAX || level->has_external_copy ||
                  unsigned max_used = MIN2(level->max_read, level->max_written);
                  /* In order for variable copies to work, we have to have the same data type
   * on the source and the destination.  In order to satisfy this, we run a
   * little fixed-point algorithm to transitively ensure that we get enough
   * components and array elements for this to hold for all copies.
   */
   bool fp_progress;
   do {
      fp_progress = false;
   nir_foreach_variable_in_list(var, vars) {
                     struct vec_var_usage *var_usage =
                        set_foreach(var_usage->vars_copied, copy_entry) {
      struct vec_var_usage *copy_usage = (void *)copy_entry->key;
   if (copy_usage->comps_kept != var_usage->comps_kept) {
      nir_component_mask_t comps_kept =
         var_usage->comps_kept = comps_kept;
   copy_usage->comps_kept = comps_kept;
                  for (unsigned i = 0; i < var_usage->num_levels; i++) {
      struct array_level_usage *var_level = &var_usage->levels[i];
                  set_foreach(var_level->levels_copied, copy_entry) {
      struct array_level_usage *copy_level = (void *)copy_entry->key;
   if (var_level->array_len != copy_level->array_len) {
      unsigned array_len =
         var_level->array_len = array_len;
   copy_level->array_len = array_len;
                           bool vars_shrunk = false;
   nir_foreach_variable_in_list_safe(var, vars) {
      if (var->data.mode != mode)
            struct vec_var_usage *usage =
         if (!usage)
            bool shrunk = false;
   const struct glsl_type *vec_type = var->type;
   for (unsigned i = 0; i < usage->num_levels; i++) {
      /* If we've reduced the array to zero elements at some level, just
   * set comps_kept to 0 and delete the variable.
   */
   if (usage->levels[i].array_len == 0) {
      usage->comps_kept = 0;
               assert(usage->levels[i].array_len <= glsl_get_length(vec_type));
   if (usage->levels[i].array_len < glsl_get_length(vec_type))
            }
            assert(usage->comps_kept == (usage->comps_kept & usage->all_comps));
   if (usage->comps_kept != usage->all_comps)
            if (usage->comps_kept == 0) {
      /* This variable is dead, remove it */
   vars_shrunk = true;
   exec_node_remove(&var->node);
               if (!shrunk) {
      /* This variable doesn't need to be shrunk.  Remove it from the
   * hash table so later steps will ignore it.
   */
   _mesa_hash_table_remove_key(var_usage_map, var);
               /* Build the new var type */
   unsigned new_num_comps = util_bitcount(usage->comps_kept);
   const struct glsl_type *new_type =
         for (int i = usage->num_levels - 1; i >= 0; i--) {
      assert(usage->levels[i].array_len > 0);
   /* If the original type was a matrix type, we'd like to keep that so
   * we don't convert matrices into arrays.
   */
   if (i == usage->num_levels - 1 &&
      glsl_type_is_matrix(glsl_without_array(var->type)) &&
   new_num_comps > 1 && usage->levels[i].array_len > 1) {
   new_type = glsl_matrix_type(glsl_get_base_type(new_type),
            } else {
            }
                           }
      static bool
   vec_deref_is_oob(nir_deref_instr *deref,
         {
      nir_deref_path path;
            bool oob = false;
   for (unsigned i = 0; i < usage->num_levels; i++) {
      nir_deref_instr *p = path.path[i + 1];
   if (p->deref_type == nir_deref_type_array_wildcard)
            if (nir_src_is_const(p->arr.index) &&
      nir_src_as_uint(p->arr.index) >= usage->levels[i].array_len) {
   oob = true;
                              }
      static bool
   vec_deref_is_dead_or_oob(nir_deref_instr *deref,
               {
      struct vec_var_usage *usage =
         if (!usage)
               }
      static void
   shrink_vec_var_access_impl(nir_function_impl *impl,
               {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
                  /* Clean up any dead derefs we find lying around.  They may refer
   * to variables we've deleted.
   */
                  /* Update the type in the deref to keep the types consistent as
   * you walk down the chain.  We don't need to check if this is one
   * of the derefs we're shrinking because this is a no-op if it
   * isn't.  The worst that could happen is that we accidentally fix
   * an invalid deref.
   */
   if (deref->deref_type == nir_deref_type_var) {
         } else if (deref->deref_type == nir_deref_type_array ||
            nir_deref_instr *parent = nir_deref_instr_parent(deref);
   assert(glsl_type_is_array(parent->type) ||
            }
                                 /* If we have a copy whose source or destination has been deleted
   * because we determined the variable was dead, then we just
   * delete the copy instruction.  If the source variable was dead
   * then it was writing undefined garbage anyway and if it's the
   * destination variable that's dead then the write isn't needed.
   */
   if (intrin->intrinsic == nir_intrinsic_copy_deref) {
      nir_deref_instr *dst = nir_src_as_deref(intrin->src[0]);
   nir_deref_instr *src = nir_src_as_deref(intrin->src[1]);
   if (vec_deref_is_dead_or_oob(dst, var_usage_map, modes) ||
      vec_deref_is_dead_or_oob(src, var_usage_map, modes)) {
   nir_instr_remove(&intrin->instr);
   nir_deref_instr_remove_if_unused(dst);
                        if (intrin->intrinsic != nir_intrinsic_load_deref &&
                  nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
                  struct vec_var_usage *usage =
                        if (usage->comps_kept == 0 || vec_deref_is_oob(deref, usage)) {
      if (intrin->intrinsic == nir_intrinsic_load_deref) {
      nir_def *u =
      nir_undef(&b, intrin->def.num_components,
      nir_def_rewrite_uses(&intrin->def,
      }
   nir_instr_remove(&intrin->instr);
                     /* If we're not dropping any components, there's no need to
   * compact vectors.
   */
                                    nir_def *undef =
         nir_def *vec_srcs[NIR_MAX_VEC_COMPONENTS];
   unsigned c = 0;
   for (unsigned i = 0; i < intrin->num_components; i++) {
      if (usage->comps_kept & (1u << i))
         else
                                          /* The SSA def is now only used by the swizzle.  It's safe to
   * shrink the number of components.
   */
   assert(list_length(&intrin->def.uses) == c);
   intrin->num_components = c;
      } else {
                     unsigned swizzle[NIR_MAX_VEC_COMPONENTS];
   nir_component_mask_t new_write_mask = 0;
   unsigned c = 0;
   for (unsigned i = 0; i < intrin->num_components; i++) {
      if (usage->comps_kept & (1u << i)) {
      swizzle[c] = i;
   if (write_mask & (1u << i))
                                                /* Rewrite to use the compacted source */
   nir_src_rewrite(&intrin->src[1], swizzled);
   nir_intrinsic_set_write_mask(intrin, new_write_mask);
      }
               default:
                  }
      static bool
   function_impl_has_vars_with_modes(nir_function_impl *impl,
         {
               if (modes & ~nir_var_function_temp) {
      nir_foreach_variable_with_modes(var, shader,
                     if ((modes & nir_var_function_temp) && !exec_list_is_empty(&impl->locals))
               }
      /** Attempt to shrink arrays of vectors
   *
   * This pass looks at variables which contain a vector or an array (possibly
   * multiple dimensions) of vectors and attempts to lower to a smaller vector
   * or array.  If the pass can prove that a component of a vector (or array of
   * vectors) is never really used, then that component will be removed.
   * Similarly, the pass attempts to shorten arrays based on what elements it
   * can prove are never read or never contain valid data.
   */
   bool
   nir_shrink_vec_array_vars(nir_shader *shader, nir_variable_mode modes)
   {
                        struct hash_table *var_usage_map =
            bool has_vars_to_shrink = false;
   nir_foreach_function_impl(impl, shader) {
      /* Don't even bother crawling the IR if we don't have any variables.
   * Given that this pass deletes any unused variables, it's likely that
   * we will be in this scenario eventually.
   */
   if (function_impl_has_vars_with_modes(impl, modes)) {
      has_vars_to_shrink = true;
   find_used_components_impl(impl, var_usage_map,
         }
   if (!has_vars_to_shrink) {
      ralloc_free(mem_ctx);
   nir_shader_preserve_all_metadata(shader);
               bool globals_shrunk = false;
   if (modes & nir_var_shader_temp) {
      globals_shrunk = shrink_vec_var_list(&shader->variables,
                     bool progress = false;
   nir_foreach_function_impl(impl, shader) {
      bool locals_shrunk = false;
   if (modes & nir_var_function_temp) {
      locals_shrunk = shrink_vec_var_list(&impl->locals,
                     if (globals_shrunk || locals_shrunk) {
               nir_metadata_preserve(impl, nir_metadata_block_index |
            } else {
                                 }
