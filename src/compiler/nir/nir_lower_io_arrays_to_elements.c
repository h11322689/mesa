   /*
   * Copyright © 2017 Timothy Arceri
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
      /** @file nir_lower_io_arrays_to_elements.c
   *
   * Split arrays/matrices with direct indexing into individual elements. This
   * will allow optimisation passes to better clean up unused elements.
   *
   */
      static unsigned
   get_io_offset(nir_builder *b, nir_deref_instr *deref, nir_variable *var,
               {
      nir_deref_path path;
            assert(path.path[0]->deref_type == nir_deref_type_var);
            /* For arrayed I/O (e.g., per-vertex input arrays in geometry shader
   * inputs), skip the outermost array index.  Process the rest normally.
   */
   if (nir_is_arrayed_io(var, b->shader->info.stage)) {
      *array_index = (*p)->arr.index.ssa;
               unsigned offset = 0;
   *xfb_offset = 0;
   for (; *p; p++) {
      if ((*p)->deref_type == nir_deref_type_array) {
                                                                  } else if ((*p)->deref_type == nir_deref_type_struct) {
      /* TODO: we could also add struct splitting support to this pass */
                              }
      static nir_variable **
   get_array_elements(struct hash_table *ht, nir_variable *var,
         {
      nir_variable **elements;
   struct hash_entry *entry = _mesa_hash_table_search(ht, var);
   if (!entry) {
      const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, stage)) {
      assert(glsl_type_is_array(type));
                                 elements = (nir_variable **)calloc(num_elements, sizeof(nir_variable *));
      } else {
                     }
      static void
   lower_array(nir_builder *b, nir_intrinsic_instr *intr, nir_variable *var,
         {
               if (nir_deref_instr_is_known_out_of_bounds(nir_src_as_deref(intr->src[0]))) {
      /* See Section 5.11 (Out-of-Bounds Accesses) of the GLSL 4.60 */
   if (intr->intrinsic != nir_intrinsic_store_deref) {
      nir_def *zero = nir_imm_zero(b, intr->def.num_components,
         nir_def_rewrite_uses(&intr->def,
      }
   nir_instr_remove(&intr->instr);
               nir_variable **elements =
            nir_def *array_index = NULL;
   unsigned elements_index = 0;
   unsigned xfb_offset = 0;
   unsigned io_offset = get_io_offset(b, nir_src_as_deref(intr->src[0]),
                  nir_variable *element = elements[elements_index];
   if (!element) {
      element = nir_variable_clone(var, b->shader);
            if (var->data.explicit_offset)
                     /* This pass also splits matrices so we need give them a new type. */
   if (glsl_type_is_matrix(type))
            if (nir_is_arrayed_io(var, b->shader->info.stage)) {
      type = glsl_array_type(type, glsl_get_length(element->type),
               element->type = type;
                                 if (nir_is_arrayed_io(var, b->shader->info.stage)) {
      assert(array_index);
               nir_intrinsic_instr *element_intr =
         element_intr->num_components = intr->num_components;
            if (intr->intrinsic != nir_intrinsic_store_deref) {
      nir_def_init(&element_intr->instr, &element_intr->def,
            if (intr->intrinsic == nir_intrinsic_interp_deref_at_offset ||
      intr->intrinsic == nir_intrinsic_interp_deref_at_sample ||
   intr->intrinsic == nir_intrinsic_interp_deref_at_vertex) {
               nir_def_rewrite_uses(&intr->def,
      } else {
      nir_intrinsic_set_write_mask(element_intr,
                              /* Remove the old load intrinsic */
      }
      static bool
   deref_has_indirect(nir_builder *b, nir_variable *var, nir_deref_path *path)
   {
      assert(path->path[0]->deref_type == nir_deref_type_var);
            if (nir_is_arrayed_io(var, b->shader->info.stage)) {
                  for (; *p; p++) {
      if ((*p)->deref_type != nir_deref_type_array)
            if (!nir_src_is_const((*p)->arr.index))
                  }
      /* Creates a mask of locations that contains arrays that are indexed via
   * indirect indexing.
   */
   static void
   create_indirects_mask(nir_shader *shader,
         {
      nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
                                          if (intr->intrinsic != nir_intrinsic_load_deref &&
      intr->intrinsic != nir_intrinsic_store_deref &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_sample &&
                     nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
                                          int loc = var->data.location * 4 + var->data.location_frac;
                              }
      static void
   lower_io_arrays_to_elements(nir_shader *shader, nir_variable_mode mask,
                     {
      nir_foreach_function_impl(impl, shader) {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              if (intr->intrinsic != nir_intrinsic_load_deref &&
      intr->intrinsic != nir_intrinsic_store_deref &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_centroid &&
   intr->intrinsic != nir_intrinsic_interp_deref_at_sample &&
                     nir_deref_instr *deref = nir_src_as_deref(intr->src[0]);
                           /* Drivers assume compact arrays are, in fact, arrays. */
                  /* Per-view variables are expected to remain arrays. */
                  /* Skip indirects */
   int loc = var->data.location * 4 + var->data.location_frac;
                           const struct glsl_type *type = var->type;
   if (nir_is_arrayed_io(var, b.shader->info.stage)) {
                        /* Skip types we cannot split.
   *
   * TODO: Add support for struct splitting.
   */
   if ((!glsl_type_is_array(type) && !glsl_type_is_matrix(type)) ||
                  /* Skip builtins */
   if (!after_cross_stage_opts &&
                        /* Don't bother splitting if we can't opt away any unused
   * elements.
   */
                  switch (intr->intrinsic) {
   case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_interp_deref_at_vertex:
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
      if ((mask & nir_var_shader_in && mode == nir_var_shader_in) ||
      (mask & nir_var_shader_out && mode == nir_var_shader_out))
         default:
                     }
      void
   nir_lower_io_arrays_to_elements_no_indirects(nir_shader *shader,
         {
      struct hash_table *split_inputs = _mesa_pointer_hash_table_create(NULL);
                     lower_io_arrays_to_elements(shader, nir_var_shader_out,
            if (!outputs_only) {
      lower_io_arrays_to_elements(shader, nir_var_shader_in,
            /* Remove old input from the shaders inputs list */
   hash_table_foreach(split_inputs, entry) {
                                    /* Remove old output from the shaders outputs list */
   hash_table_foreach(split_outputs, entry) {
      nir_variable *var = (nir_variable *)entry->key;
                        _mesa_hash_table_destroy(split_inputs, NULL);
               }
      void
   nir_lower_io_arrays_to_elements(nir_shader *producer, nir_shader *consumer)
   {
      struct hash_table *split_inputs = _mesa_pointer_hash_table_create(NULL);
                     create_indirects_mask(producer, indirects, nir_var_shader_out);
            lower_io_arrays_to_elements(producer, nir_var_shader_out,
            lower_io_arrays_to_elements(consumer, nir_var_shader_in,
            /* Remove old input from the shaders inputs list */
   hash_table_foreach(split_inputs, entry) {
      nir_variable *var = (nir_variable *)entry->key;
                        /* Remove old output from the shaders outputs list */
   hash_table_foreach(split_outputs, entry) {
      nir_variable *var = (nir_variable *)entry->key;
                        _mesa_hash_table_destroy(split_inputs, NULL);
            nir_remove_dead_derefs(producer);
      }
