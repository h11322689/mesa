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
      #include "nir.h"
   #include "nir_builder.h"
   #include "nir_control_flow.h"
   #include "nir_vla.h"
      /*
   * TODO: write a proper inliner for GPUs.
   * This heuristic just inlines small functions,
   * and tail calls get inlined as well.
   */
   static bool
   nir_function_can_inline(nir_function *function)
   {
      bool can_inline = true;
   if (!function->should_inline) {
      if (function->impl) {
      if (function->impl->num_blocks > 2)
         if (function->impl->ssa_alloc > 45)
         }
      }
      static bool
   function_ends_in_jump(nir_function_impl *impl)
   {
      nir_block *last_block = nir_impl_last_block(impl);
      }
      void
   nir_inline_function_impl(struct nir_builder *b,
                     {
                        nir_foreach_block(block, copy) {
      nir_foreach_instr_safe(instr, block) {
      switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
                  /* We don't need to remap function variables.  We already cloned
   * them as part of nir_function_impl_clone and appended them to
   * b->impl->locals.
   */
                  /* If no map is provided, we assume that there are either no
   * shader variables or they already live b->shader (this is the
   * case for function inlining within a single shader.
   */
                  struct hash_entry *entry =
         if (entry == NULL) {
      nir_variable *nvar = nir_variable_clone(deref->var, b->shader);
   nir_shader_add_variable(b->shader, nvar);
   entry = _mesa_hash_table_insert(shader_var_remap,
      }
   deref->var = entry->data;
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *load = nir_instr_as_intrinsic(instr);
                  unsigned param_idx = nir_intrinsic_param_idx(load);
   assert(param_idx < impl->function->num_params);
                  /* Remove any left-over load_param intrinsics because they're soon
   * to be in another function and therefore no longer valid.
   */
   nir_instr_remove(&load->instr);
               case nir_instr_type_jump:
      /* Returns have to be lowered for this to work */
               default:
                                 /* Pluck the body out of the function and place it here */
   nir_cf_list body;
            if (nest_if) {
      nir_if *cf = nir_push_if(b, nir_imm_true(b));
   nir_cf_reinsert(&body, nir_after_cf_list(&cf->then_list));
      } else {
      /* Insert a nop at the cursor so we can keep track of where things are as
   * we add/remove stuff from the CFG.
   */
   nir_intrinsic_instr *nop = nir_nop(b);
   nir_cf_reinsert(&body, nir_before_instr(&nop->instr));
         }
      static bool inline_function_impl(nir_function_impl *impl, struct set *inlined);
      static bool inline_functions_pass(nir_builder *b,
               {
      struct set *inlined = cb_data;
   if (instr->type != nir_instr_type_call)
            nir_call_instr *call = nir_instr_as_call(instr);
            if (b->shader->options->driver_functions &&
      b->shader->info.stage == MESA_SHADER_KERNEL) {
   bool last_instr = (instr == nir_block_last_instr(instr->block));
   if (!nir_function_can_inline(call->callee) && !last_instr) {
                     /* Make sure that the function we're calling is already inlined */
                     /* Rewrite all of the uses of the callee's parameters to use the call
   * instructions sources.  In order to ensure that the "load" happens
   * here and not later (for register sources), we make sure to convert it
   * to an SSA value first.
   */
   const unsigned num_params = call->num_params;
   NIR_VLA(nir_def *, params, num_params);
   for (unsigned i = 0; i < num_params; i++) {
                  nir_inline_function_impl(b, call->callee->impl, params, NULL);
      }
      static bool
   inline_function_impl(nir_function_impl *impl, struct set *inlined)
   {
      if (_mesa_set_search(inlined, impl))
            bool progress;
   progress = nir_function_instructions_pass(impl, inline_functions_pass,
         if (progress) {
      /* Indices are completely messed up now */
                           }
      /** A pass to inline all functions in a shader into their callers
   *
   * For most use-cases, function inlining is a multi-step process.  The general
   * pattern employed by SPIR-V consumers and others is as follows:
   *
   *  1. nir_lower_variable_initializers(shader, nir_var_function_temp)
   *
   *     This is needed because local variables from the callee are simply added
   *     to the locals list for the caller and the information about where the
   *     constant initializer logically happens is lost.  If the callee is
   *     called in a loop, this can cause the variable to go from being
   *     initialized once per loop iteration to being initialized once at the
   *     top of the caller and values to persist from one invocation of the
   *     callee to the next.  The simple solution to this problem is to get rid
   *     of constant initializers before function inlining.
   *
   *  2. nir_lower_returns(shader)
   *
   *     nir_inline_functions assumes that all functions end "naturally" by
   *     execution reaching the end of the function without any return
   *     instructions causing instant jumps to the end.  Thanks to NIR being
   *     structured, we can't represent arbitrary jumps to various points in the
   *     program which is what an early return in the callee would have to turn
   *     into when we inline it into the caller.  Instead, we require returns to
   *     be lowered which lets us just copy+paste the callee directly into the
   *     caller.
   *
   *  3. nir_inline_functions(shader)
   *
   *     This does the actual function inlining and the resulting shader will
   *     contain no call instructions.
   *
   *  4. nir_opt_deref(shader)
   *
   *     Most functions contain pointer parameters where the result of a deref
   *     instruction is passed in as a parameter, loaded via a load_param
   *     intrinsic, and then turned back into a deref via a cast.  Function
   *     inlining will get rid of the load_param but we are still left with a
   *     cast.  Running nir_opt_deref gets rid of the intermediate cast and
   *     results in a whole deref chain again.  This is currently required by a
   *     number of optimizations and lowering passes at least for certain
   *     variable modes.
   *
   *  5. Loop over the functions and delete all but the main entrypoint.
   *
   *     In the Intel Vulkan driver this looks like this:
   *
   *        nir_remove_non_entrypoints(nir);
   *
   *    While nir_inline_functions does get rid of all call instructions, it
   *    doesn't get rid of any functions because it doesn't know what the "root
   *    function" is.  Instead, it's up to the individual driver to know how to
   *    decide on a root function and delete the rest.  With SPIR-V,
   *    spirv_to_nir returns the root function and so we can just use == whereas
   *    with GL, you may have to look for a function named "main".
   *
   *  6. nir_lower_variable_initializers(shader, ~nir_var_function_temp)
   *
   *     Lowering constant initializers on inputs, outputs, global variables,
   *     etc. requires that we know the main entrypoint so that we know where to
   *     initialize them.  Otherwise, we would have to assume that anything
   *     could be a main entrypoint and initialize them at the start of every
   *     function but that would clearly be wrong if any of those functions were
   *     ever called within another function.  Simply requiring a single-
   *     entrypoint function shader is the best way to make it well-defined.
   */
   bool
   nir_inline_functions(nir_shader *shader)
   {
      struct set *inlined = _mesa_pointer_set_create(NULL);
            nir_foreach_function_impl(impl, shader) {
                              }
      struct lower_link_state {
      struct hash_table *shader_var_remap;
      };
      static bool
   lower_calls_vars_instr(struct nir_builder *b,
               {
               switch (instr->type) {
   case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type != nir_deref_type_var)
         if (deref->var->data.mode == nir_var_function_temp)
            assert(state->shader_var_remap);
   struct hash_entry *entry =
         if (entry == NULL) {
      nir_variable *nvar = nir_variable_clone(deref->var, b->shader);
   nir_shader_add_variable(b->shader, nvar);
   entry = _mesa_hash_table_insert(state->shader_var_remap,
      }
   deref->var = entry->data;
      }
   case nir_instr_type_call: {
      nir_call_instr *ncall = nir_instr_as_call(instr);
   if (!ncall->callee->name)
            nir_function *func = nir_shader_get_function_for_name(b->shader, ncall->callee->name);
   if (func) {
      ncall->callee = func;
               nir_function *new_func;
   new_func = nir_shader_get_function_for_name(state->link_shader, ncall->callee->name);
   if (new_func)
            }
   default:
         }
      }
      static bool
   lower_call_function_impl(struct nir_builder *b,
                     {
      nir_function_impl *copy = nir_function_impl_clone(b->shader, impl);
   copy->function = callee;
            return nir_function_instructions_pass(copy,
                  }
      static bool
   function_link_pass(struct nir_builder *b,
               {
               if (instr->type != nir_instr_type_call)
            nir_call_instr *call = nir_instr_as_call(instr);
            if (!call->callee->name)
            if (call->callee->impl)
            func = nir_shader_get_function_for_name(state->link_shader, call->callee->name);
   if (!func || !func->impl) {
         }
   return lower_call_function_impl(b, call->callee,
            }
      bool
   nir_link_shader_functions(nir_shader *shader,
         {
      void *ra_ctx = ralloc_context(NULL);
   struct hash_table *copy_vars = _mesa_pointer_hash_table_create(ra_ctx);
            struct lower_link_state state = {
      .shader_var_remap = copy_vars,
      };
   /* do progress passes inside the pass */
   do {
      progress = false;
   nir_foreach_function_impl(impl, shader) {
      bool this_progress = nir_function_instructions_pass(impl,
                     if (this_progress)
            }
                           }
      static void
   nir_mark_used_functions(struct nir_function *func, struct set *used_funcs);
      static bool mark_used_pass_cb(struct nir_builder *b,
         {
      struct set *used_funcs = data;
   if (instr->type != nir_instr_type_call)
                           nir_mark_used_functions(call->callee, used_funcs);
      }
      static void
   nir_mark_used_functions(struct nir_function *func, struct set *used_funcs)
   {
      if (func->impl) {
      nir_function_instructions_pass(func->impl,
                     }
      void
   nir_cleanup_functions(nir_shader *nir)
   {
      if (!nir->options->driver_functions) {
      nir_remove_non_entrypoints(nir);
               struct set *used_funcs = _mesa_set_create(NULL, _mesa_hash_pointer,
         foreach_list_typed_safe(nir_function, func, node, &nir->functions) {
      if (func->is_entrypoint) {
      _mesa_set_add(used_funcs, func);
         }
   foreach_list_typed_safe(nir_function, func, node, &nir->functions) {
      if (!_mesa_set_search(used_funcs, func))
      }
      }
