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
   #include "nir_gl_types.h"
   #include "nir_deref.h"
   #include "gl_nir_linker.h"
   #include "compiler/glsl/ir_uniform.h" /* for gl_uniform_storage */
   #include "linker_util.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #include "main/consts_exts.h"
   #include "main/shader_types.h"
      /**
   * This file do the common link for GLSL uniforms, using NIR, instead of IR as
   * the counter-part glsl/link_uniforms.cpp
   */
      #define UNMAPPED_UNIFORM_LOC ~0u
      struct uniform_array_info {
      /** List of dereferences of the uniform array. */
            /** Set of bit-flags to note which array elements have been accessed. */
      };
      static unsigned
   uniform_storage_size(const struct glsl_type *type)
   {
      switch (glsl_get_base_type(type)) {
   case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_INTERFACE: {
      unsigned size = 0;
   for (unsigned i = 0; i < glsl_get_length(type); i++)
            }
   case GLSL_TYPE_ARRAY: {
      const struct glsl_type *e_type = glsl_get_array_element(type);
   enum glsl_base_type e_base_type = glsl_get_base_type(e_type);
   if (e_base_type == GLSL_TYPE_STRUCT ||
      e_base_type == GLSL_TYPE_INTERFACE ||
   e_base_type == GLSL_TYPE_ARRAY) {
   unsigned length = !glsl_type_is_unsized_array(type) ?
            } else
      }
   default:
            }
      /**
   * Update the sizes of linked shader uniform arrays to the maximum
   * array index used.
   *
   * From page 81 (page 95 of the PDF) of the OpenGL 2.1 spec:
   *
   *     If one or more elements of an array are active,
   *     GetActiveUniform will return the name of the array in name,
   *     subject to the restrictions listed above. The type of the array
   *     is returned in type. The size parameter contains the highest
   *     array element index used, plus one. The compiler or linker
   *     determines the highest index used.  There will be only one
   *     active uniform reported by the GL per uniform array.
   */
   static void
   update_array_sizes(struct gl_shader_program *prog, nir_variable *var,
               {
      /* For now we only resize 1D arrays.
   * TODO: add support for resizing more complex array types ??
   */
   if (!glsl_type_is_array(var->type) ||
      glsl_type_is_array(glsl_get_array_element(var->type)))
         /* GL_ARB_uniform_buffer_object says that std140 uniforms
   * will not be eliminated.  Since we always do std140, just
   * don't resize arrays in UBOs.
   *
   * Atomic counters are supposed to get deterministic
   * locations assigned based on the declaration ordering and
   * sizes, array compaction would mess that up.
   *
   * Subroutine uniforms are not removed.
   */
   if (nir_variable_is_in_block(var) || glsl_contains_atomic(var->type) ||
      glsl_get_base_type(glsl_without_array(var->type)) == GLSL_TYPE_SUBROUTINE ||
   var->constant_initializer)
         struct uniform_array_info *ainfo = NULL;
   int words = BITSET_WORDS(glsl_array_size(var->type));
   int max_array_size = 0;
   for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
   if (!sh)
            struct hash_entry *entry =
         if (entry) {
      ainfo = (struct uniform_array_info *)  entry->data;
   max_array_size = MAX2(BITSET_LAST_BIT_SIZED(ainfo->indices, words),
               if (max_array_size == glsl_array_size(var->type))
               if (max_array_size != glsl_array_size(var->type)) {
      /* If this is a built-in uniform (i.e., it's backed by some
   * fixed-function state), adjust the number of state slots to
   * match the new array size.  The number of slots per array entry
   * is not known.  It seems safe to assume that the total number of
   * slots is an integer multiple of the number of array elements.
   * Determine the number of slots per array element by dividing by
   * the old (total) size.
   */
   const unsigned num_slots = var->num_state_slots;
   if (num_slots > 0) {
      var->num_state_slots =
               var->type = glsl_array_type(glsl_get_array_element(var->type),
            /* Update the types of dereferences in case we changed any. */
   struct hash_entry *entry =
         if (entry) {
      struct uniform_array_info *ainfo =
         util_dynarray_foreach(ainfo->deref_list, nir_deref_instr *, deref) {
                  }
      static void
   nir_setup_uniform_remap_tables(const struct gl_constants *consts,
         {
               /* For glsl this may have been allocated by reserve_explicit_locations() so
   * that we can keep track of unused uniforms with explicit locations.
   */
   assert(!prog->data->spirv ||
         if (!prog->UniformRemapTable) {
      prog->UniformRemapTable = rzalloc_array(prog,
                     union gl_constant_value *data =
      rzalloc_array(prog->data,
      if (!prog->UniformRemapTable || !data) {
      linker_error(prog, "Out of memory during linking.\n");
      }
            prog->data->UniformDataDefaults =
                           /* Reserve all the explicit locations of the active uniforms. */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
               if (uniform->hidden)
            if (uniform->is_shader_storage ||
                  if (prog->data->UniformStorage[i].remap_location == UNMAPPED_UNIFORM_LOC)
            /* How many new entries for this uniform? */
   const unsigned entries = MAX2(1, uniform->array_elements);
                     /* Set remap table entries point to correct gl_uniform_storage. */
   for (unsigned j = 0; j < entries; j++) {
                                    /* Reserve locations for rest of the uniforms. */
   if (prog->data->spirv)
            for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
               if (uniform->hidden)
            if (uniform->is_shader_storage ||
                  /* Built-in uniforms should not get any location. */
   if (uniform->builtin)
            /* Explicit ones have been set already. */
   if (uniform->remap_location != UNMAPPED_UNIFORM_LOC)
            /* How many entries for this uniform? */
            /* Add new entries to the total amount for checking against MAX_UNIFORM-
   * _LOCATIONS. This only applies to the default uniform block (-1),
   * because locations of uniform block entries are not assignable.
   */
   if (prog->data->UniformStorage[i].block_index == -1)
            unsigned location =
            if (location == -1) {
               /* resize remap table to fit new entries */
   prog->UniformRemapTable =
      reralloc(prog,
            prog->UniformRemapTable,
               /* set the base location in remap table for the uniform */
                     if (uniform->block_index == -1)
            /* Set remap table entries point to correct gl_uniform_storage. */
   for (unsigned j = 0; j < entries; j++) {
                     if (uniform->block_index == -1)
                  /* Verify that total amount of entries for explicit and implicit locations
   * is less than MAX_UNIFORM_LOCATIONS.
   */
   if (total_entries > consts->MaxUserAssignableUniformLocations) {
      linker_error(prog, "count of uniform locations > MAX_UNIFORM_LOCATIONS"
                     /* Reserve all the explicit locations of the active subroutine uniforms. */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
               if (glsl_get_base_type(uniform->type) != GLSL_TYPE_SUBROUTINE)
            if (prog->data->UniformStorage[i].remap_location == UNMAPPED_UNIFORM_LOC)
            /* How many new entries for this uniform? */
   const unsigned entries =
                     unsigned num_slots = glsl_get_component_slots(uniform->type);
   unsigned mask = prog->data->linked_stages;
   while (mask) {
                                    /* Set remap table entries point to correct gl_uniform_storage. */
   for (unsigned k = 0; k < entries; k++) {
      unsigned element_loc =
                                          /* reserve subroutine locations */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
               if (glsl_get_base_type(uniform->type) != GLSL_TYPE_SUBROUTINE)
            if (prog->data->UniformStorage[i].remap_location !=
                  const unsigned entries =
                     unsigned num_slots = glsl_get_component_slots(uniform->type);
   unsigned mask = prog->data->linked_stages;
   while (mask) {
                                    p->sh.SubroutineUniformRemapTable =
      reralloc(p,
                     for (unsigned k = 0; k < entries; k++) {
                        }
   prog->data->UniformStorage[i].remap_location =
                        /* assign storage to hidden uniforms */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
               if (!uniform->hidden ||
                  const unsigned entries =
                     unsigned num_slots = glsl_get_component_slots(uniform->type);
   for (unsigned k = 0; k < entries; k++)
         }
      static void
   add_var_use_deref(nir_deref_instr *deref, struct hash_table *live,
         {
      nir_deref_path path;
            deref = path.path[0];
   if (deref->deref_type != nir_deref_type_var ||
      !nir_deref_mode_is_one_of(deref, nir_var_uniform |
                     nir_deref_path_finish(&path);
               /* Number of derefs used in current processing. */
            const struct glsl_type *deref_type = deref->var->type;
   nir_deref_instr **p = &path.path[1];
   for (; *p; p++) {
                  /* Skip matrix derefences */
                                    if (ptr == NULL) {
                        *derefs_size += 4096;
                                       if (nir_src_is_const((*p)->arr.index)) {
         } else {
      /* An unsized array can occur at the end of an SSBO.  We can't track
   * accesses to such an array, so bail.
   */
   if (dr->size == 0) {
                                       } else if ((*p)->deref_type == nir_deref_type_struct) {
      /* We have reached the end of the array. */
                                       struct hash_entry *entry =
         if (!entry && glsl_type_is_array(deref->var->type)) {
               unsigned num_bits = MAX2(1, glsl_get_aoa_size(deref->var->type));
            ainfo->deref_list = ralloc(live, struct util_dynarray);
               if (entry)
            if (glsl_type_is_array(deref->var->type)) {
      /* Count the "depth" of the arrays-of-arrays. */
   unsigned array_depth = 0;
   for (const struct glsl_type *type = deref->var->type;
      glsl_type_is_array(type);
   type = glsl_get_array_element(type)) {
               link_util_mark_array_elements_referenced(*derefs, num_derefs, array_depth,
                        assert(deref->modes == deref->var->data.mode);
      }
      /* Iterate over the shader and collect infomation about uniform use */
   static void
   add_var_use_shader(nir_shader *shader, struct hash_table *live)
   {
      /* Currently allocated buffer block of derefs. */
            /* Size of the derefs buffer in bytes. */
            nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   switch (intr->intrinsic) {
   case nir_intrinsic_atomic_counter_read_deref:
   case nir_intrinsic_atomic_counter_inc_deref:
   case nir_intrinsic_atomic_counter_pre_dec_deref:
   case nir_intrinsic_atomic_counter_post_dec_deref:
   case nir_intrinsic_atomic_counter_add_deref:
   case nir_intrinsic_atomic_counter_min_deref:
   case nir_intrinsic_atomic_counter_max_deref:
   case nir_intrinsic_atomic_counter_and_deref:
   case nir_intrinsic_atomic_counter_or_deref:
   case nir_intrinsic_atomic_counter_xor_deref:
   case nir_intrinsic_atomic_counter_exchange_deref:
   case nir_intrinsic_atomic_counter_comp_swap_deref:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
                        default:
      /* Nothing to do */
         } else if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex_instr = nir_instr_as_tex(instr);
   int sampler_idx =
      nir_tex_instr_src_index(tex_instr,
                           if (sampler_idx >= 0) {
      nir_deref_instr *deref =
                     if (texture_idx >= 0) {
      nir_deref_instr *deref =
                                    }
      static void
   mark_stage_as_active(struct gl_uniform_storage *uniform,
         {
         }
      /* Used to build a tree representing the glsl_type so that we can have a place
   * to store the next index for opaque types. Array types are expanded so that
   * they have a single child which is used for all elements of the array.
   * Struct types have a child for each member. The tree is walked while
   * processing a uniform so that we can recognise when an opaque type is
   * encountered a second time in order to reuse the same range of indices that
   * was reserved the first time. That way the sampler indices can be arranged
   * so that members of an array are placed sequentially even if the array is an
   * array of structs containing other opaque members.
   */
   struct type_tree_entry {
      /* For opaque types, this will be the next index to use. If we haven’t
   * encountered this member yet, it will be UINT_MAX.
   */
   unsigned next_index;
   unsigned array_size;
   struct type_tree_entry *parent;
   struct type_tree_entry *next_sibling;
      };
      struct nir_link_uniforms_state {
      /* per-whole program */
   unsigned num_hidden_uniforms;
   unsigned num_values;
            /* per-shader stage */
   unsigned next_bindless_image_index;
   unsigned next_bindless_sampler_index;
   unsigned next_image_index;
   unsigned next_sampler_index;
   unsigned next_subroutine;
   unsigned num_shader_samplers;
   unsigned num_shader_images;
   unsigned num_shader_uniform_components;
   unsigned shader_samplers_used;
   unsigned shader_shadow_samplers;
   unsigned shader_storage_blocks_write_access;
            /* per-variable */
   nir_variable *current_var;
   const struct glsl_type *current_ifc_type;
   int offset;
   bool var_is_in_block;
   bool set_top_level_array;
   int top_level_array_size;
            struct type_tree_entry *current_type;
   struct hash_table *referenced_uniforms[MESA_SHADER_STAGES];
      };
      static void
   add_parameter(struct gl_uniform_storage *uniform,
               const struct gl_constants *consts,
   struct gl_shader_program *prog,
   {
      /* Builtin uniforms are backed by PROGRAM_STATE_VAR, so don't add them as
   * uniforms.
   */
   if (uniform->builtin)
            if (!state->params || uniform->is_shader_storage ||
      (glsl_contains_opaque(type) && !state->current_var->data.bindless))
         unsigned num_params = glsl_get_aoa_size(type);
   num_params = MAX2(num_params, 1);
            bool is_dual_slot = glsl_type_is_dual_slot(glsl_without_array(type));
   if (is_dual_slot)
            struct gl_program_parameter_list *params = state->params;
   int base_index = params->NumParameters;
            if (consts->PackedDriverUniformStorage) {
      for (unsigned i = 0; i < num_params; i++) {
      unsigned dmul = glsl_type_is_64bit(glsl_without_array(type)) ? 2 : 1;
   unsigned comps = glsl_get_vector_elements(glsl_without_array(type)) * dmul;
   if (is_dual_slot) {
      if (i & 0x1)
         else
               /* TODO: This will waste space with 1 and 3 16-bit components. */
                  _mesa_add_parameter(params, PROGRAM_UNIFORM, uniform->name.string, comps,
         } else {
      for (unsigned i = 0; i < num_params; i++) {
      _mesa_add_parameter(params, PROGRAM_UNIFORM, uniform->name.string, 4,
                  /* Each Parameter will hold the index to the backing uniform storage.
   * This avoids relying on names to match parameters and uniform
   * storages.
   */
   for (unsigned i = 0; i < num_params; i++) {
      struct gl_program_parameter *param = &params->Parameters[base_index + i];
   param->UniformStorageIndex = uniform - prog->data->UniformStorage;
         }
      static unsigned
   get_next_index(struct nir_link_uniforms_state *state,
               {
      /* If we’ve already calculated an index for this member then we can just
   * offset from there.
   */
   if (state->current_type->next_index == UINT_MAX) {
      /* Otherwise we need to reserve enough indices for all of the arrays
   * enclosing this member.
                     for (const struct type_tree_entry *p = state->current_type;
      p;
   p = p->parent) {
               state->current_type->next_index = *next_index;
   *next_index += array_size;
      } else
                                 }
      static gl_texture_index
   texture_index_for_type(const struct glsl_type *type)
   {
      const bool sampler_array = glsl_sampler_type_is_array(type);
   switch (glsl_get_sampler_dim(type)) {
   case GLSL_SAMPLER_DIM_1D:
         case GLSL_SAMPLER_DIM_2D:
         case GLSL_SAMPLER_DIM_3D:
         case GLSL_SAMPLER_DIM_CUBE:
         case GLSL_SAMPLER_DIM_RECT:
         case GLSL_SAMPLER_DIM_BUF:
         case GLSL_SAMPLER_DIM_EXTERNAL:
         case GLSL_SAMPLER_DIM_MS:
      return sampler_array ? TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX :
      default:
      assert(!"Should not get here.");
         }
      /* Update the uniforms info for the current shader stage */
   static void
   update_uniforms_shader_info(struct gl_shader_program *prog,
                           {
      unsigned values = glsl_get_component_slots(type);
            if (glsl_type_is_sampler(type_no_array)) {
      bool init_idx;
   /* ARB_bindless_texture spec says:
   *
   *    "When used as shader inputs, outputs, uniform block members,
   *     or temporaries, the value of the sampler is a 64-bit unsigned
   *     integer handle and never refers to a texture image unit."
   */
   bool is_bindless = state->current_var->data.bindless || state->var_is_in_block;
   unsigned *next_index = is_bindless ?
      &state->next_bindless_sampler_index :
      int sampler_index = get_next_index(state, uniform, next_index, &init_idx);
            if (is_bindless) {
      if (init_idx) {
      sh->Program->sh.BindlessSamplers =
      rerzalloc(sh->Program, sh->Program->sh.BindlessSamplers,
                     for (unsigned j = sh->Program->sh.NumBindlessSamplers;
      j < state->next_bindless_sampler_index; j++) {
                     sh->Program->sh.NumBindlessSamplers =
               if (!state->var_is_in_block)
      } else {
      /* Samplers (bound or bindless) are counted as two components
   * as specified by ARB_bindless_texture.
                  if (init_idx) {
      const unsigned shadow = glsl_sampler_type_is_shadow(type_no_array);
   for (unsigned i = sampler_index;
      i < MIN2(state->next_sampler_index, MAX_SAMPLERS); i++) {
   sh->Program->sh.SamplerTargets[i] =
         state->shader_samplers_used |= 1U << i;
                     uniform->opaque[stage].active = true;
      } else if (glsl_type_is_image(type_no_array)) {
               /* Set image access qualifiers */
   enum gl_access_qualifier image_access =
            int image_index;
   if (state->current_var->data.bindless) {
                     sh->Program->sh.BindlessImages =
      rerzalloc(sh->Program, sh->Program->sh.BindlessImages,
                     for (unsigned j = sh->Program->sh.NumBindlessImages;
      j < state->next_bindless_image_index; j++) {
                     } else {
                     /* Images (bound or bindless) are counted as two components as
   * specified by ARB_bindless_texture.
                  for (unsigned i = image_index;
      i < MIN2(state->next_image_index, MAX_IMAGE_UNIFORMS); i++) {
                  uniform->opaque[stage].active = true;
            if (!uniform->is_shader_storage)
      } else {
      if (glsl_get_base_type(type_no_array) == GLSL_TYPE_SUBROUTINE) {
                                       /* Increment the subroutine index by 1 for non-arrays and by the
   * number of array elements for arrays.
   */
               if (!state->var_is_in_block)
         }
      static bool
   find_and_update_named_uniform_storage(const struct gl_constants *consts,
                                       {
      /* gl_uniform_storage can cope with one level of array, so if the type is a
   * composite type or an array where each element occupies more than one
   * location than we need to recursively process it.
   */
   if (glsl_type_is_struct_or_ifc(type) ||
      (glsl_type_is_array(type) &&
   (glsl_type_is_array(glsl_get_array_element(type)) ||
            struct type_tree_entry *old_type = state->current_type;
            /* Shader storage block unsized arrays: add subscript [0] to variable
   * names.
   */
   unsigned length = glsl_get_length(type);
   if (glsl_type_is_unsized_array(type))
            bool result = false;
   for (unsigned i = 0; i < length; i++) {
                                       /* Append '.field' to the current variable name. */
   if (name) {
      ralloc_asprintf_rewrite_tail(name, &new_length, ".%s",
                           /* Append the subscript to the current variable name */
   if (name)
               result = find_and_update_named_uniform_storage(consts, prog, state,
                                       if (!result) {
      state->current_type = old_type;
                              } else {
      struct hash_entry *entry =
         if (entry) {
                     if (*first_element && !state->var_is_in_block) {
      *first_element = false;
                        const struct glsl_type *type_no_array = glsl_without_array(type);
   struct hash_entry *entry = prog->data->spirv ? NULL :
      _mesa_hash_table_search(state->referenced_uniforms[stage],
      if (entry != NULL ||
      glsl_get_base_type(type_no_array) == GLSL_TYPE_SUBROUTINE ||
                                                }
      /**
   * Finds, returns, and updates the stage info for any uniform in UniformStorage
   * defined by @var. For GLSL this is done using the name, for SPIR-V in general
   * is this done using the explicit location, except:
   *
   * * UBOs/SSBOs: as they lack explicit location, binding is used to locate
   *   them. That means that more that one entry at the uniform storage can be
   *   found. In that case all of them are updated, and the first entry is
   *   returned, in order to update the location of the nir variable.
   *
   * * Special uniforms: like atomic counters. They lack a explicit location,
   *   so they are skipped. They will be handled and assigned a location later.
   *
   */
   static bool
   find_and_update_previous_uniform_storage(const struct gl_constants *consts,
                                 {
      if (!prog->data->spirv) {
      bool first_element = true;
   char *name_tmp = ralloc_strdup(NULL, name);
   bool r = find_and_update_named_uniform_storage(consts, prog, state, var,
                                          if (nir_variable_is_in_block(var)) {
               ASSERTED unsigned num_blks = nir_variable_is_in_ubo(var) ?
                  struct gl_uniform_block *blks = nir_variable_is_in_ubo(var) ?
            bool result = false;
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      /* UniformStorage contains both variables from ubos and ssbos */
   if ( prog->data->UniformStorage[i].is_shader_storage !=
                  int block_index = prog->data->UniformStorage[i].block_index;
                     if (var->data.binding == blks[block_index].Binding) {
      if (!uniform)
         mark_stage_as_active(&prog->data->UniformStorage[i],
                           if (result)
                     /* Beyond blocks, there are still some corner cases of uniforms without
   * location (ie: atomic counters) that would have a initial location equal
   * to -1. We just return on that case. Those uniforms will be handled
   * later.
   */
   if (var->data.location == -1)
            /* TODO: following search can be problematic with shaders with a lot of
   * uniforms. Would it be better to use some type of hash
   */
   for (unsigned i = 0; i < prog->data->NumUniformStorage; i++) {
      if (prog->data->UniformStorage[i].remap_location == var->data.location) {
               struct gl_uniform_storage *uniform = &prog->data->UniformStorage[i];
   var->data.location = uniform - prog->data->UniformStorage;
   add_parameter(uniform, consts, prog, var->type, state);
                     }
      static struct type_tree_entry *
   build_type_tree_for_type(const struct glsl_type *type)
   {
               entry->array_size = 1;
   entry->next_index = UINT_MAX;
   entry->children = NULL;
   entry->next_sibling = NULL;
            if (glsl_type_is_array(type)) {
      entry->array_size = glsl_get_length(type);
   entry->children = build_type_tree_for_type(glsl_get_array_element(type));
      } else if (glsl_type_is_struct_or_ifc(type)) {
               for (unsigned i = 0; i < glsl_get_length(type); i++) {
      const struct glsl_type *field_type = glsl_get_struct_field(type, i);
                  if (last == NULL)
                                                   }
      static void
   free_type_tree(struct type_tree_entry *entry)
   {
               for (p = entry->children; p; p = next) {
      next = p->next_sibling;
                  }
      static void
   hash_free_uniform_name(struct hash_entry *entry)
   {
         }
      static void
   enter_record(struct nir_link_uniforms_state *state,
               const struct gl_constants *consts,
   {
      assert(glsl_type_is_struct(type));
   if (!state->var_is_in_block)
            bool use_std430 = consts->UseSTD430AsDefaultPacking;
   const enum glsl_interface_packing packing =
      glsl_get_internal_ifc_packing(state->current_var->interface_type,
         if (packing == GLSL_INTERFACE_PACKING_STD430)
      state->offset = align(
      else
      state->offset = align(
   }
      static void
   leave_record(struct nir_link_uniforms_state *state,
               const struct gl_constants *consts,
   {
      assert(glsl_type_is_struct(type));
   if (!state->var_is_in_block)
            bool use_std430 = consts->UseSTD430AsDefaultPacking;
   const enum glsl_interface_packing packing =
      glsl_get_internal_ifc_packing(state->current_var->interface_type,
         if (packing == GLSL_INTERFACE_PACKING_STD430)
      state->offset = align(
      else
      state->offset = align(
   }
      /**
   * Creates the neccessary entries in UniformStorage for the uniform. Returns
   * the number of locations used or -1 on failure.
   */
   static int
   nir_link_uniform(const struct gl_constants *consts,
                  struct gl_shader_program *prog,
   struct gl_program *stage_program,
   gl_shader_stage stage,
   const struct glsl_type *type,
   unsigned index_in_parent,
      {
               if (state->set_top_level_array &&
      nir_variable_is_in_ssbo(state->current_var)) {
   /* Type is the top level SSBO member */
   if (glsl_type_is_array(type) &&
      (glsl_type_is_array(glsl_get_array_element(type)) ||
   glsl_type_is_struct_or_ifc(glsl_get_array_element(type)))) {
   /* Type is a top-level array (array of aggregate types) */
   state->top_level_array_size = glsl_get_length(type);
      } else {
      state->top_level_array_size = 1;
                           /* gl_uniform_storage can cope with one level of array, so if the type is a
   * composite type or an array where each element occupies more than one
   * location than we need to recursively process it.
   */
   if (glsl_type_is_struct_or_ifc(type) ||
      (glsl_type_is_array(type) &&
   (glsl_type_is_array(glsl_get_array_element(type)) ||
         int location_count = 0;
   struct type_tree_entry *old_type = state->current_type;
                     /* Shader storage block unsized arrays: add subscript [0] to variable
   * names.
   */
   unsigned length = glsl_get_length(type);
   if (glsl_type_is_unsized_array(type))
            if (glsl_type_is_struct(type) && !prog->data->spirv)
            for (unsigned i = 0; i < length; i++) {
      const struct glsl_type *field_type;
                  if (glsl_type_is_struct_or_ifc(type)) {
      field_type = glsl_get_struct_field(type, i);
   /* Use the offset inside the struct only for variables backed by
   * a buffer object. For variables not backed by a buffer object,
   * offset is -1.
   */
   if (state->var_is_in_block) {
      if (prog->data->spirv) {
      state->offset =
      } else if (glsl_get_struct_field_offset(type, i) != -1 &&
                                          /* Append '.field' to the current variable name. */
   if (name) {
                           /* The layout of structures at the top level of the block is set
   * during parsing.  For matrices contained in multiple levels of
   * structures in the block, the inner structures have no layout.
   * These cases must potentially inherit the layout from the outer
   * levels.
   */
   const enum glsl_matrix_layout matrix_layout =
         if (matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR) {
         } else if (matrix_layout == GLSL_MATRIX_LAYOUT_COLUMN_MAJOR) {
                              /* Append the subscript to the current variable name */
   if (name)
               int entries = nir_link_uniform(consts, prog, stage_program, stage,
                                       if (location != -1)
                  if (glsl_type_is_struct_or_ifc(type))
               if (glsl_type_is_struct(type) && !prog->data->spirv)
                        } else {
      /* TODO: reallocating storage is slow, we should figure out a way to
   * allocate storage up front for spirv like we do for GLSL.
   */
   if (prog->data->spirv) {
      /* Create a new uniform storage entry */
   prog->data->UniformStorage =
      reralloc(prog->data,
            prog->data->UniformStorage,
   if (!prog->data->UniformStorage) {
      linker_error(prog, "Out of memory during linking.\n");
                  uniform = &prog->data->UniformStorage[prog->data->NumUniformStorage];
            /* Initialize its members */
            uniform->name.string =
                  const struct glsl_type *type_no_array = glsl_without_array(type);
   if (glsl_type_is_array(type)) {
      uniform->type = type_no_array;
      } else {
      uniform->type = type;
      }
   uniform->top_level_array_size = state->top_level_array_size;
            struct hash_entry *entry = prog->data->spirv ? NULL :
      _mesa_hash_table_search(state->referenced_uniforms[stage],
      if (entry != NULL ||
      glsl_get_base_type(type_no_array) == GLSL_TYPE_SUBROUTINE ||
               if (location >= 0) {
      /* Uniform has an explicit location */
      } else {
                  uniform->hidden = state->current_var->data.how_declared == nir_var_hidden;
   if (uniform->hidden)
            uniform->is_shader_storage = nir_variable_is_in_ssbo(state->current_var);
            /* Set fields whose default value depend on the variable being inside a
   * block.
   *
   * From the OpenGL 4.6 spec, 7.3 Program objects:
   *
   * "For the property ARRAY_STRIDE, ... For active variables not declared
   * as an array of basic types, zero is written to params. For active
   * variables not backed by a buffer object, -1 is written to params,
   * regardless of the variable type."
   *
   * "For the property MATRIX_STRIDE, ... For active variables not declared
   * as a matrix or array of matrices, zero is written to params. For active
   * variables not backed by a buffer object, -1 is written to params,
   * regardless of the variable type."
   *
   * For the property IS_ROW_MAJOR, ... For active variables backed by a
   * buffer object, declared as a single matrix or array of matrices, and
   * stored in row-major order, one is written to params. For all other
   * active variables, zero is written to params.
   */
   uniform->array_stride = -1;
   uniform->matrix_stride = -1;
            if (state->var_is_in_block) {
                     if (glsl_type_is_matrix(uniform->type)) {
      uniform->matrix_stride = glsl_get_explicit_stride(uniform->type);
      } else {
                  if (!prog->data->spirv) {
      bool use_std430 = consts->UseSTD430AsDefaultPacking;
   const enum glsl_interface_packing packing =
                  unsigned alignment =
         if (packing == GLSL_INTERFACE_PACKING_STD430) {
      alignment =
      }
                           int buffer_block_index = -1;
   /* If the uniform is inside a uniform block determine its block index by
   * comparing the bindings, we can not use names.
   */
   if (state->var_is_in_block) {
                                    if (!prog->data->spirv) {
      bool is_interface_array =
                  const char *ifc_name =
         if (is_interface_array) {
      unsigned l = strlen(ifc_name);
   for (unsigned i = 0; i < num_blocks; i++) {
      if (strncmp(ifc_name, blocks[i].name.string, l) == 0 &&
      blocks[i].name.string[l] == '[') {
   buffer_block_index = i;
            } else {
      for (unsigned i = 0; i < num_blocks; i++) {
      if (strcmp(ifc_name, blocks[i].name.string) == 0) {
      buffer_block_index = i;
                     /* Compute the next offset. */
   bool use_std430 = consts->UseSTD430AsDefaultPacking;
   const enum glsl_interface_packing packing =
      glsl_get_internal_ifc_packing(state->current_var->interface_type,
      if (packing == GLSL_INTERFACE_PACKING_STD430)
         else
      } else {
      for (unsigned i = 0; i < num_blocks; i++) {
      if (state->current_var->data.binding == blocks[i].Binding) {
      buffer_block_index = i;
                  /* Compute the next offset. */
      }
               uniform->block_index = buffer_block_index;
   uniform->builtin = is_gl_identifier(uniform->name.string);
            /* The following are not for features not supported by ARB_gl_spirv */
            unsigned entries = MAX2(1, uniform->array_elements);
                     if (uniform->remap_location != UNMAPPED_UNIFORM_LOC &&
                  if (!state->var_is_in_block)
            if (name) {
      _mesa_hash_table_insert(state->uniform_hash, strdup(*name),
                     if (!is_gl_identifier(uniform->name.string) && !uniform->is_shader_storage &&
                        }
      bool
   gl_nir_link_uniforms(const struct gl_constants *consts,
               {
      /* First free up any previous UniformStorage items */
   ralloc_free(prog->data->UniformStorage);
   prog->data->UniformStorage = NULL;
            /* Iterate through all linked shaders */
            if (!prog->data->spirv) {
      /* Gather information on uniform use */
   for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
                  state.referenced_uniforms[stage] =
                  nir_shader *nir = sh->Program->nir;
               if(!consts->DisableUniformArrayResize) {
      /* Resize uniform arrays based on the maximum array index */
   for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
                  nir_foreach_gl_uniform_variable(var, sh->Program->nir)
                     /* Count total number of uniforms and allocate storage */
   unsigned storage_size = 0;
   if (!prog->data->spirv) {
      struct set *storage_counted =
         for (unsigned stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[stage];
                  nir_foreach_gl_uniform_variable(var, sh->Program->nir) {
      const struct glsl_type *type = var->type;
   const char *name = var->name;
   if (nir_variable_is_in_block(var) &&
      glsl_without_array(type) == var->interface_type) {
                     struct set_entry *entry = _mesa_set_search(storage_counted, name);
   if (!entry) {
      storage_size += uniform_storage_size(type);
            }
            prog->data->UniformStorage = rzalloc_array(prog->data,
               if (!prog->data->UniformStorage) {
      linker_error(prog, "Out of memory while linking uniforms.\n");
                  /* Iterate through all linked shaders */
   state.uniform_hash = _mesa_hash_table_create(NULL, _mesa_hash_string,
            for (unsigned shader_type = 0; shader_type < MESA_SHADER_STAGES; shader_type++) {
      struct gl_linked_shader *sh = prog->_LinkedShaders[shader_type];
   if (!sh)
            nir_shader *nir = sh->Program->nir;
            state.next_bindless_image_index = 0;
   state.next_bindless_sampler_index = 0;
   state.next_image_index = 0;
   state.next_sampler_index = 0;
   state.num_shader_samplers = 0;
   state.num_shader_images = 0;
   state.num_shader_uniform_components = 0;
   state.shader_storage_blocks_write_access = 0;
   state.shader_samplers_used = 0;
   state.shader_shadow_samplers = 0;
            nir_foreach_gl_uniform_variable(var, nir) {
      state.current_var = var;
   state.current_ifc_type = NULL;
   state.offset = 0;
   state.var_is_in_block = nir_variable_is_in_block(var);
   state.set_top_level_array = false;
                  /*
   * From ARB_program_interface spec, issue (16):
   *
   * "RESOLVED: We will follow the default rule for enumerating block
   *  members in the OpenGL API, which is:
   *
   *  * If a variable is a member of an interface block without an
   *    instance name, it is enumerated using just the variable name.
   *
   *  * If a variable is a member of an interface block with an
   *    instance name, it is enumerated as "BlockName.Member", where
   *    "BlockName" is the name of the interface block (not the
   *    instance name) and "Member" is the name of the variable.
   *
   * For example, in the following code:
   *
   * uniform Block1 {
   *   int member1;
   * };
   * uniform Block2 {
   *   int member2;
   * } instance2;
   * uniform Block3 {
   *  int member3;
   * } instance3[2];  // uses two separate buffer bindings
   *
   * the three uniforms (if active) are enumerated as "member1",
   * "Block2.member2", and "Block3.member3"."
   *
   * Note that in the last example, with an array of ubo, only one
   * uniform is generated. For that reason, while unrolling the
   * uniforms of a ubo, or the variables of a ssbo, we need to treat
   * arrays of instance as a single block.
   */
   char *name;
   const struct glsl_type *type = var->type;
   if (state.var_is_in_block &&
      ((!prog->data->spirv && glsl_without_array(type) == var->interface_type) ||
   (prog->data->spirv && type == var->interface_type))) {
   type = glsl_without_array(var->type);
   state.current_ifc_type = type;
      } else {
      state.set_top_level_array = true;
               struct type_tree_entry *type_tree =
                           struct gl_uniform_block *blocks = NULL;
   int num_blocks = 0;
   int buffer_block_index = -1;
   bool is_interface_array = false;
   if (state.var_is_in_block) {
      /* If the uniform is inside a uniform block determine its block index by
   * comparing the bindings, we can not use names.
   */
   blocks = nir_variable_is_in_ssbo(state.current_var) ?
                        is_interface_array =
                                                   /* Even when a match is found, do not "break" here.  As this is
   * an array of instances, all elements of the array need to be
   * marked as referenced.
   */
   for (unsigned i = 0; i < num_blocks; i++) {
      if (strncmp(ifc_name, blocks[i].name.string, l) == 0 &&
                           struct hash_entry *entry =
      _mesa_hash_table_search(state.referenced_uniforms[shader_type],
      if (entry) {
      struct uniform_array_info *ainfo =
         if (BITSET_TEST(ainfo->indices, blocks[i].linearized_array_index))
               } else {
      for (unsigned i = 0; i < num_blocks; i++) {
      bool match = false;
   if (!prog->data->spirv) {
         } else {
                                 if (!prog->data->spirv) {
      struct hash_entry *entry =
                                                      if (nir_variable_is_in_ssbo(var) &&
      !(var->data.access & ACCESS_NON_WRITEABLE)) {
                           /* Buffers from each stage are pointers to the one stored in the program. We need
   * to account for this before computing the mask below otherwise the mask will be
   * incorrect.
   *    sh->Program->sh.SSBlocks: [a][b][c][d][e][f]
   *    VS prog->data->SSBlocks : [a][b][c]
   *    FS prog->data->SSBlocks : [d][e][f]
   * eg for FS buffer 1, buffer_block_index will be 4 but sh_block_index will be 1.
   */
                           int sh_block_index = buffer_block_index - base;
   /* Shaders that use too many SSBOs will fail to compile, which
   * we don't care about.
   *
   * This is true for shaders that do not use too many SSBOs:
   */
   if (sh_block_index + array_size <= 32) {
      state.shader_storage_blocks_write_access |=
                  if (blocks && !prog->data->spirv && state.var_is_in_block) {
      if (glsl_without_array(state.current_var->type) != state.current_var->interface_type) {
                           if (glsl_type_is_struct(state.current_var->type)) {
         } else if (glsl_type_is_array(state.current_var->type) &&
                              const unsigned l = strlen(state.current_var->name);
   for (unsigned i = 0; i < num_blocks; i++) {
      for (unsigned j = 0; j < blocks[i].NumUniforms; j++) {
                                             if ((ptrdiff_t) l != (end - begin))
                                                                                             if (found)
      }
   assert(found);
      } else {
      /* this is the base block offset */
   var->data.location = buffer_block_index;
      }
   assert(buffer_block_index >= 0);
   const struct gl_uniform_block *const block =
                                             /* Check if the uniform has been processed already for
   * other stage. If so, validate they are compatible and update
   * the active stage mask.
   */
   if (find_and_update_previous_uniform_storage(consts, prog, &state, var,
            ralloc_free(name);
   free_type_tree(type_tree);
               /* From now on the variable’s location will be its uniform index */
   if (!state.var_is_in_block)
                        bool row_major =
         int res = nir_link_uniform(consts, prog, sh->Program, shader_type, type,
                                                   if (res == -1)
               if (!prog->data->spirv) {
      _mesa_hash_table_destroy(state.referenced_uniforms[shader_type],
               if (state.num_shader_samplers >
      consts->Program[shader_type].MaxTextureImageUnits) {
   linker_error(prog, "Too many %s shader texture samplers\n",
                     if (state.num_shader_images >
      consts->Program[shader_type].MaxImageUniforms) {
   linker_error(prog, "Too many %s shader image uniforms (%u > %u)\n",
               _mesa_shader_stage_to_string(shader_type),
               sh->Program->SamplersUsed = state.shader_samplers_used;
   sh->Program->sh.ShaderStorageBlocksWriteAccess =
         sh->shadow_samplers = state.shader_shadow_samplers;
   sh->Program->info.num_textures = state.num_shader_samplers;
   sh->Program->info.num_images = state.num_shader_images;
   sh->num_uniform_components = state.num_shader_uniform_components;
               prog->data->NumHiddenUniforms = state.num_hidden_uniforms;
                     if (prog->data->spirv)
            nir_setup_uniform_remap_tables(consts, prog);
                        }
