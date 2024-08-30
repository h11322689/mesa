   /*
   * Copyright © 2019 Valve Corporation
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
      /* This pass optimizes GL access qualifiers. So far it does three things:
   *
   * - Infer readonly when it's missing.
   * - Infer writeonly when it's missing.
   * - Infer ACCESS_CAN_REORDER when the following are true:
   *   - Either there are no writes, or ACCESS_NON_WRITEABLE is set. In either
   *     case there are no writes to the underlying memory.
   *   - ACCESS_VOLATILE is not set.
   *
   * If these conditions are true, then image and buffer reads may be treated as
   * if they were uniform buffer reads, i.e. they may be arbitrarily moved,
   * combined, rematerialized etc.
   */
      struct access_state {
               struct set *vars_written;
   struct set *vars_read;
   bool images_written;
   bool buffers_written;
   bool images_read;
      };
      static void
   gather_buffer_access(struct access_state *state, nir_def *def, bool read, bool write)
   {
      state->buffers_read |= read;
            if (!def)
            const nir_variable *var = nir_get_binding_variable(
         if (var) {
      if (read)
         if (write)
      } else {
      nir_foreach_variable_with_modes(possible_var, state->shader, nir_var_mem_ssbo) {
      if (read)
         if (write)
            }
      static void
   gather_intrinsic(struct access_state *state, nir_intrinsic_instr *instr)
   {
      const nir_variable *var;
   bool read, write;
   switch (instr->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_sparse_load:
   case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
   case nir_intrinsic_image_deref_samples_identical:
      var = nir_intrinsic_get_var(instr, 0);
   read = instr->intrinsic != nir_intrinsic_image_deref_store;
   write = instr->intrinsic != nir_intrinsic_image_deref_load &&
            /* In OpenGL, buffer images use normal buffer objects, whereas other
   * image types use textures which cannot alias with buffer objects.
   * Therefore we have to group buffer samplers together with SSBO's.
   */
   if (glsl_get_sampler_dim(glsl_without_array(var->type)) ==
      GLSL_SAMPLER_DIM_BUF) {
   state->buffers_read |= read;
      } else {
      state->images_read |= read;
               if ((var->data.mode == nir_var_uniform ||
      var->data.mode == nir_var_image) &&
   read)
      if ((var->data.mode == nir_var_uniform ||
      var->data.mode == nir_var_image) &&
   write)
            case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_sparse_load:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_bindless_image_samples_identical:
      read = instr->intrinsic != nir_intrinsic_bindless_image_store;
   write = instr->intrinsic != nir_intrinsic_bindless_image_load &&
            if (nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_BUF) {
      state->buffers_read |= read;
      } else {
      state->images_read |= read;
      }
         case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
   case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap: {
      nir_deref_instr *deref = nir_src_as_deref(instr->src[0]);
   if (!nir_deref_mode_may_be(deref, nir_var_mem_ssbo | nir_var_mem_global))
            bool ssbo = nir_deref_mode_is(deref, nir_var_mem_ssbo);
   gather_buffer_access(state, ssbo ? instr->src[0].ssa : NULL,
                           default:
            }
      static bool
   process_variable(struct access_state *state, nir_variable *var)
   {
      const struct glsl_type *type = glsl_without_array(var->type);
   if (var->data.mode != nir_var_mem_ssbo &&
      !(var->data.mode == nir_var_uniform && glsl_type_is_image(type)) &&
   var->data.mode != nir_var_image)
         /* Ignore variables we've already marked */
   if (var->data.access & ACCESS_CAN_REORDER)
            unsigned access = var->data.access;
   bool is_buffer = var->data.mode == nir_var_mem_ssbo ||
            if (!(access & ACCESS_NON_WRITEABLE)) {
      if (is_buffer ? !state->buffers_written : !state->images_written)
         else if ((access & ACCESS_RESTRICT) && !_mesa_set_search(state->vars_written, var))
               if (!(access & ACCESS_NON_READABLE)) {
      if (is_buffer ? !state->buffers_read : !state->images_read)
         else if ((access & ACCESS_RESTRICT) && !_mesa_set_search(state->vars_read, var))
               bool changed = var->data.access != access;
   var->data.access = access;
      }
      static bool
   update_access(struct access_state *state, nir_intrinsic_instr *instr, bool is_buffer, bool is_global)
   {
               bool is_memory_readonly = access & ACCESS_NON_WRITEABLE;
            if (instr->intrinsic != nir_intrinsic_bindless_image_load &&
      instr->intrinsic != nir_intrinsic_bindless_image_store &&
   instr->intrinsic != nir_intrinsic_bindless_image_sparse_load &&
   !is_global) {
   const nir_variable *var = nir_get_binding_variable(
         is_memory_readonly |= var && (var->data.access & ACCESS_NON_WRITEABLE);
               if (is_global) {
      is_memory_readonly |= !state->buffers_written && !state->images_written;
      } else {
      is_memory_readonly |= is_buffer ? !state->buffers_written : !state->images_written;
               if (is_memory_readonly)
         if (is_memory_writeonly)
         if (!(access & ACCESS_VOLATILE) && is_memory_readonly)
            bool progress = nir_intrinsic_access(instr) != access;
   nir_intrinsic_set_access(instr, access);
      }
      static bool
   process_intrinsic(struct access_state *state, nir_intrinsic_instr *instr)
   {
      switch (instr->intrinsic) {
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_store:
   case nir_intrinsic_bindless_image_sparse_load:
      return update_access(state, instr, nir_intrinsic_image_dim(instr) == GLSL_SAMPLER_DIM_BUF,
         case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref: {
      if (nir_deref_mode_is(nir_src_as_deref(instr->src[0]), nir_var_mem_global))
         else if (nir_deref_mode_is(nir_src_as_deref(instr->src[0]), nir_var_mem_ssbo))
         else
               case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_store:
   case nir_intrinsic_image_deref_sparse_load: {
               bool is_buffer =
                        default:
            }
      static bool
   opt_access_impl(struct access_state *state,
         {
               nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_intrinsic)
      progress |= process_intrinsic(state,
               if (progress) {
      nir_metadata_preserve(impl,
                                    }
      bool
   nir_opt_access(nir_shader *shader, const nir_opt_access_options *options)
   {
      struct access_state state = {
      .shader = shader,
   .vars_written = _mesa_pointer_set_create(NULL),
               bool var_progress = false;
            nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_intrinsic)
                     /* In Vulkan, buffers and images can alias. */
   if (options->is_vulkan) {
      state.buffers_written |= state.images_written;
   state.images_written |= state.buffers_written;
   state.buffers_read |= state.images_read;
               nir_foreach_variable_with_modes(var, shader, nir_var_uniform | nir_var_mem_ubo | nir_var_mem_ssbo | nir_var_image)
            nir_foreach_function_impl(impl, shader) {
               /* If we make a change to the uniforms, update all the impls. */
   if (var_progress) {
      nir_metadata_preserve(impl,
                                             _mesa_set_destroy(state.vars_read, NULL);
   _mesa_set_destroy(state.vars_written, NULL);
      }
