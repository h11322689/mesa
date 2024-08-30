   /*
   * Copyright © 2015 Intel Corporation
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
      /*
   * Implements a pass that lowers output and/or input variables to a
   * temporary plus an output variable with a single copy at each exit
   * point of the shader and/or an input variable with a single copy
   * at the entrance point of the shader.  This way the output variable
   * is only ever written once and/or input is only read once, and there
   * are no indirect outut/input accesses.
   */
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_deref.h"
      struct lower_io_state {
      nir_shader *shader;
   nir_function_impl *entrypoint;
   struct exec_list old_outputs;
   struct exec_list old_inputs;
   struct exec_list new_outputs;
            /* map from temporary to new input */
      };
      static void
   emit_copies(nir_builder *b, struct exec_list *dest_vars,
         {
               foreach_two_lists(dest_node, dest_vars, src_node, src_vars) {
      nir_variable *dest = exec_node_data(nir_variable, dest_node, node);
            /* No need to copy the contents of a non-fb_fetch_output output variable
   * to the temporary allocated for it, since its initial value is
   * undefined.
   */
   if (src->data.mode == nir_var_shader_out &&
                  /* Can't copy the contents of the temporary back to a read-only
   * interface variable.  The value of the temporary won't have been
   * modified by the shader anyway.
   */
   if (dest->data.read_only)
                  }
      static void
   emit_output_copies_impl(struct lower_io_state *state, nir_function_impl *impl)
   {
               if (state->shader->info.stage == MESA_SHADER_GEOMETRY) {
      /* For geometry shaders, we have to emit the output copies right
   * before each EmitVertex call.
   */
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic == nir_intrinsic_emit_vertex ||
      intrin->intrinsic == nir_intrinsic_emit_vertex_with_counter) {
   b.cursor = nir_before_instr(&intrin->instr);
               } else if (impl == state->entrypoint) {
      b.cursor = nir_before_impl(impl);
            /* For all other shader types, we need to do the copies right before
   * the jumps to the end block.
   */
   set_foreach(impl->end_block->predecessors, block_entry) {
      struct nir_block *block = (void *)block_entry->key;
   b.cursor = nir_after_block_before_jump(block);
            }
      /* For fragment shader inputs, when we lower to temporaries we'll invalidate
   * interpolateAt*() because now they'll be pointing to the temporary instead
   * of the actual variable. Since the caller presumably doesn't support
   * indirect indexing of inputs, we'll need to lower something like:
   *
   * in vec4 foo[3];
   *
   * ... = interpolateAtCentroid(foo[i]);
   *
   * to a sequence of interpolations that store to our temporary, then a
   * load at the end:
   *
   * in vec4 foo[3];
   * vec4 foo_tmp[3];
   *
   * foo_tmp[0] = interpolateAtCentroid(foo[0]);
   * foo_tmp[1] = interpolateAtCentroid(foo[1]);
   * ... = foo_tmp[i];
   */
      /*
   * Recursively emit the interpolation instructions. Here old_interp_deref
   * refers to foo[i], temp_deref is foo_tmp[0/1], and new_interp_deref is
   * foo[0/1].
   */
      static void
   emit_interp(nir_builder *b, nir_deref_instr **old_interp_deref,
               {
      while (*old_interp_deref) {
      switch ((*old_interp_deref)->deref_type) {
   case nir_deref_type_struct:
      temp_deref =
      nir_build_deref_struct(b, temp_deref,
      new_interp_deref =
      nir_build_deref_struct(b, new_interp_deref,
         case nir_deref_type_array:
      if (nir_src_is_const((*old_interp_deref)->arr.index)) {
      temp_deref =
      nir_build_deref_array(b, temp_deref,
      new_interp_deref =
      nir_build_deref_array(b, new_interp_deref,
         } else {
      /* We have an indirect deref, so we have to emit interpolations
   * for every index. Recurse in case we have an array of arrays.
   */
   unsigned length = glsl_get_length(temp_deref->type);
   for (unsigned i = 0; i < length; i++) {
      nir_deref_instr *new_temp =
                                                   case nir_deref_type_var:
   case nir_deref_type_array_wildcard:
   case nir_deref_type_ptr_as_array:
   case nir_deref_type_cast:
                              /* Now that we've constructed a fully-qualified deref with all the indirect
   * derefs replaced with direct ones, it's time to actually emit the new
   * interpolation instruction.
            nir_intrinsic_instr *new_interp =
            new_interp->src[0] = nir_src_for_ssa(&new_interp_deref->def);
   if (interp->intrinsic == nir_intrinsic_interp_deref_at_sample ||
      interp->intrinsic == nir_intrinsic_interp_deref_at_offset ||
   interp->intrinsic == nir_intrinsic_interp_deref_at_vertex) {
               new_interp->num_components = interp->num_components;
   nir_def_init(&new_interp->instr, &new_interp->def,
            nir_builder_instr_insert(b, &new_interp->instr);
   nir_store_deref(b, temp_deref, &new_interp->def,
      }
      static void
   fixup_interpolation_instr(struct lower_io_state *state,
         {
      nir_deref_path interp_path;
                     /* The original interpolation instruction should contain a deref path
   * starting with the original variable, which is now the temporary.
   */
            /* Fish out the newly-created input variable. */
   assert(temp_root->deref_type == nir_deref_type_var);
   struct hash_entry *entry = _mesa_hash_table_search(state->input_map,
         assert(entry);
   nir_variable *input = entry->data;
            /* Emit the interpolation instructions. */
            /* Now the temporary contains the interpolation results, and we can just
   * load from it. We can reuse the original deref, since it points to the
   * correct part of the temporary.
   */
   nir_def *load = nir_load_deref(b, nir_src_as_deref(interp->src[0]));
   nir_def_rewrite_uses(&interp->def, load);
               }
      static void
   fixup_interpolation(struct lower_io_state *state, nir_function_impl *impl,
         {
      nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              if (interp->intrinsic == nir_intrinsic_interp_deref_at_centroid ||
      interp->intrinsic == nir_intrinsic_interp_deref_at_sample ||
   interp->intrinsic == nir_intrinsic_interp_deref_at_offset ||
   interp->intrinsic == nir_intrinsic_interp_deref_at_vertex) {
               }
      static void
   emit_input_copies_impl(struct lower_io_state *state, nir_function_impl *impl)
   {
      if (impl == state->entrypoint) {
      nir_builder b = nir_builder_at(nir_before_impl(impl));
   emit_copies(&b, &state->old_inputs, &state->new_inputs);
   if (state->shader->info.stage == MESA_SHADER_FRAGMENT)
         }
      static nir_variable *
   create_shadow_temp(struct lower_io_state *state, nir_variable *var)
   {
      nir_variable *nvar = ralloc(state->shader, nir_variable);
   memcpy(nvar, var, sizeof *nvar);
            /* The original is now the temporary */
            /* Reparent the name to the new variable */
                     /* Give the original a new name with @<mode>-temp appended */
   const char *mode = (temp->data.mode == nir_var_shader_in) ? "in" : "out";
   temp->name = ralloc_asprintf(var, "%s@%s-temp", mode, nvar->name);
   temp->data.mode = nir_var_shader_temp;
   temp->data.read_only = false;
   temp->data.fb_fetch_output = false;
               }
      static void
   move_variables_to_list(nir_shader *shader, nir_variable_mode mode,
         {
      nir_foreach_variable_with_modes_safe(var, shader, mode) {
      exec_node_remove(&var->node);
         }
      void
   nir_lower_io_to_temporaries(nir_shader *shader, nir_function_impl *entrypoint,
         {
               if (shader->info.stage == MESA_SHADER_TESS_CTRL ||
      shader->info.stage == MESA_SHADER_TASK ||
   shader->info.stage == MESA_SHADER_MESH)
         state.shader = shader;
   state.entrypoint = entrypoint;
            exec_list_make_empty(&state.old_inputs);
   if (inputs)
            exec_list_make_empty(&state.old_outputs);
   if (outputs)
            exec_list_make_empty(&state.new_inputs);
            /* Walk over all of the outputs turn each output into a temporary and
   * make a new variable for the actual output.
   */
   nir_foreach_variable_in_list(var, &state.old_outputs) {
      nir_variable *output = create_shadow_temp(&state, var);
               /* and same for inputs: */
   nir_foreach_variable_in_list(var, &state.old_inputs) {
      nir_variable *input = create_shadow_temp(&state, var);
   exec_list_push_tail(&state.new_inputs, &input->node);
               nir_foreach_function_impl(impl, shader) {
      if (inputs)
            if (outputs)
            nir_metadata_preserve(impl, nir_metadata_block_index |
               exec_list_append(&shader->variables, &state.old_inputs);
   exec_list_append(&shader->variables, &state.old_outputs);
   exec_list_append(&shader->variables, &state.new_inputs);
                        }
