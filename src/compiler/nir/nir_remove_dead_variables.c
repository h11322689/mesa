   /*
   * Copyright © 2014 Intel Corporation
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
   *
   * Authors:
   *    Connor Abbott (cwabbott0@gmail.com)
   *
   */
      #include "nir.h"
      static bool
   deref_used_for_not_store(nir_deref_instr *deref)
   {
      nir_foreach_use(src, &deref->def) {
      switch (nir_src_parent_instr(src)->type) {
   case nir_instr_type_deref:
      if (deref_used_for_not_store(nir_instr_as_deref(nir_src_parent_instr(src))))
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin =
         /* The first source of copy and store intrinsics is the deref to
   * write.  Don't record those.
   */
   if ((intrin->intrinsic != nir_intrinsic_store_deref &&
      intrin->intrinsic != nir_intrinsic_copy_deref) ||
   src != &intrin->src[0])
                  default:
      /* If it's used by any other instruction type (most likely a texture
   * or call instruction), consider it used.
   */
                     }
      static void
   add_var_use_deref(nir_deref_instr *deref, struct set *live)
   {
      if (deref->deref_type != nir_deref_type_var)
            /* Since these local variables don't escape the shader, writing doesn't
   * make them live.  Only keep them if they are used by some intrinsic.
   */
   if ((deref->var->data.mode & (nir_var_function_temp |
            !deref_used_for_not_store(deref))
         /*
   * Shared memory blocks (interface type) alias each other, so be
   * conservative in that case.
   */
   if ((deref->var->data.mode & nir_var_mem_shared) &&
      !glsl_type_is_interface(deref->var->type) &&
   !deref_used_for_not_store(deref))
         nir_variable *var = deref->var;
   do {
      _mesa_set_add(live, var);
   /* Also mark the chain of variables used to initialize it. */
         }
      static void
   add_var_use_shader(nir_shader *shader, struct set *live, nir_variable_mode modes)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_deref)
               }
      static void
   remove_dead_var_writes(nir_shader *shader)
   {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
                        nir_variable_mode parent_modes;
   if (deref->deref_type == nir_deref_type_var)
                        /* If the parent mode is 0, then it references a dead variable.
   * Flag this deref as dead and remove it.
   */
   if (parent_modes == 0) {
      deref->modes = 0;
                        case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                        if (nir_src_as_deref(intrin->src[0])->modes == 0)
                     default:
                     }
      static bool
   remove_dead_vars(struct exec_list *var_list, nir_variable_mode modes,
         {
               nir_foreach_variable_in_list_safe(var, var_list) {
      if (!(var->data.mode & modes))
            if (opts && opts->can_remove_var &&
                  struct set_entry *entry = _mesa_set_search(live, var);
   if (entry == NULL) {
      /* Mark this variable as dead by setting the mode to 0 */
   var->data.mode = 0;
   exec_node_remove(&var->node);
                     }
      bool
   nir_remove_dead_variables(nir_shader *shader, nir_variable_mode modes,
         {
      bool progress = false;
                     if (modes & ~nir_var_function_temp) {
      progress = remove_dead_vars(&shader->variables, modes,
                     if (modes & nir_var_function_temp) {
      nir_foreach_function_impl(impl, shader) {
      if (remove_dead_vars(&impl->locals,
                                       nir_foreach_function_impl(impl, shader) {
      if (progress) {
      remove_dead_var_writes(shader);
   nir_metadata_preserve(impl, nir_metadata_block_index |
      } else {
                        }
