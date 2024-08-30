   /*
   * Copyright © 2015 Red Hat
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
   #include "nir_control_flow.h"
   #include "nir_xfb_info.h"
      /* Secret Decoder Ring:
   *   clone_foo():
   *        Allocate and clone a foo.
   *   __clone_foo():
   *        Clone body of foo (ie. parent class, embedded struct, etc)
   */
      typedef struct {
      /* True if we are cloning an entire shader. */
            /* If true allows the clone operation to fall back to the original pointer
   * if no clone pointer is found in the remap table.  This allows us to
   * clone a loop body without having to add srcs from outside the loop to
   * the remap table. This is useful for loop unrolling.
   */
            /* maps orig ptr -> cloned ptr: */
            /* List of phi sources. */
            /* new shader object, used as memctx for just about everything else: */
      } clone_state;
      static void
   init_clone_state(clone_state *state, struct hash_table *remap_table,
         {
      state->global_clone = global;
            if (remap_table) {
         } else {
                     }
      static void
   free_clone_state(clone_state *state)
   {
         }
      static inline void *
   _lookup_ptr(clone_state *state, const void *ptr, bool global)
   {
               if (!ptr)
            if (!state->global_clone && global)
            if (unlikely(!state->remap_table)) {
      assert(state->allow_remap_fallback);
               entry = _mesa_hash_table_search(state->remap_table, ptr);
   if (!entry) {
      assert(state->allow_remap_fallback);
                  }
      static void
   add_remap(clone_state *state, void *nptr, const void *ptr)
   {
         }
      static void *
   remap_local(clone_state *state, const void *ptr)
   {
         }
      static void *
   remap_global(clone_state *state, const void *ptr)
   {
         }
      static nir_variable *
   remap_var(clone_state *state, const nir_variable *var)
   {
         }
      nir_constant *
   nir_constant_clone(const nir_constant *c, nir_variable *nvar)
   {
               memcpy(nc->values, c->values, sizeof(nc->values));
   nc->is_null_constant = c->is_null_constant;
   nc->num_elements = c->num_elements;
   nc->elements = ralloc_array(nvar, nir_constant *, c->num_elements);
   for (unsigned i = 0; i < c->num_elements; i++) {
                     }
      /* NOTE: for cloning nir_variables, bypass nir_variable_create to avoid
   * having to deal with locals and globals separately:
   */
   nir_variable *
   nir_variable_clone(const nir_variable *var, nir_shader *shader)
   {
               nvar->type = var->type;
   nvar->name = ralloc_strdup(nvar, var->name);
   nvar->data = var->data;
   nvar->num_state_slots = var->num_state_slots;
   if (var->num_state_slots) {
      nvar->state_slots = ralloc_array(nvar, nir_state_slot, var->num_state_slots);
   memcpy(nvar->state_slots, var->state_slots,
      }
   if (var->constant_initializer) {
      nvar->constant_initializer =
      }
            nvar->num_members = var->num_members;
   if (var->num_members) {
      nvar->members = ralloc_array(nvar, struct nir_variable_data,
         memcpy(nvar->members, var->members,
                  }
      static nir_variable *
   clone_variable(clone_state *state, const nir_variable *var)
   {
      nir_variable *nvar = nir_variable_clone(var, state->ns);
               }
      /* clone list of nir_variable: */
   static void
   clone_var_list(clone_state *state, struct exec_list *dst,
         {
      exec_list_make_empty(dst);
   foreach_list_typed(nir_variable, var, node, list) {
      nir_variable *nvar = clone_variable(state, var);
         }
      static void
   __clone_src(clone_state *state, void *ninstr_or_if,
         {
         }
      static void
   __clone_def(clone_state *state, nir_instr *ninstr,
         {
      nir_def_init(ninstr, ndef, def->num_components, def->bit_size);
   if (likely(state->remap_table))
      }
      static nir_alu_instr *
   clone_alu(clone_state *state, const nir_alu_instr *alu)
   {
      nir_alu_instr *nalu = nir_alu_instr_create(state->ns, alu->op);
   nalu->exact = alu->exact;
   nalu->no_signed_wrap = alu->no_signed_wrap;
                     for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      __clone_src(state, &nalu->instr, &nalu->src[i].src, &alu->src[i].src);
   memcpy(nalu->src[i].swizzle, alu->src[i].swizzle,
                  }
      nir_alu_instr *
   nir_alu_instr_clone(nir_shader *shader, const nir_alu_instr *orig)
   {
      clone_state state = {
      .allow_remap_fallback = true,
      };
      }
      static nir_deref_instr *
   clone_deref_instr(clone_state *state, const nir_deref_instr *deref)
   {
      nir_deref_instr *nderef =
                     nderef->modes = deref->modes;
            if (deref->deref_type == nir_deref_type_var) {
      nderef->var = remap_var(state, deref->var);
                        switch (deref->deref_type) {
   case nir_deref_type_struct:
      nderef->strct.index = deref->strct.index;
         case nir_deref_type_array:
   case nir_deref_type_ptr_as_array:
      __clone_src(state, &nderef->instr,
         nderef->arr.in_bounds = deref->arr.in_bounds;
         case nir_deref_type_array_wildcard:
      /* Nothing to do */
         case nir_deref_type_cast:
      nderef->cast.ptr_stride = deref->cast.ptr_stride;
   nderef->cast.align_mul = deref->cast.align_mul;
   nderef->cast.align_offset = deref->cast.align_offset;
         default:
                     }
      static nir_intrinsic_instr *
   clone_intrinsic(clone_state *state, const nir_intrinsic_instr *itr)
   {
      nir_intrinsic_instr *nitr =
                     if (nir_intrinsic_infos[itr->intrinsic].has_dest)
            nitr->num_components = itr->num_components;
            for (unsigned i = 0; i < num_srcs; i++)
               }
      static nir_load_const_instr *
   clone_load_const(clone_state *state, const nir_load_const_instr *lc)
   {
      nir_load_const_instr *nlc =
      nir_load_const_instr_create(state->ns, lc->def.num_components,
                              }
      static nir_undef_instr *
   clone_ssa_undef(clone_state *state, const nir_undef_instr *sa)
   {
      nir_undef_instr *nsa =
      nir_undef_instr_create(state->ns, sa->def.num_components,
                     }
      static nir_tex_instr *
   clone_tex(clone_state *state, const nir_tex_instr *tex)
   {
               ntex->sampler_dim = tex->sampler_dim;
   ntex->dest_type = tex->dest_type;
   ntex->op = tex->op;
   __clone_def(state, &ntex->instr, &ntex->def, &tex->def);
   for (unsigned i = 0; i < ntex->num_srcs; i++) {
      ntex->src[i].src_type = tex->src[i].src_type;
      }
   ntex->coord_components = tex->coord_components;
   ntex->is_array = tex->is_array;
   ntex->array_is_lowered_cube = tex->array_is_lowered_cube;
   ntex->is_shadow = tex->is_shadow;
   ntex->is_new_style_shadow = tex->is_new_style_shadow;
   ntex->is_sparse = tex->is_sparse;
   ntex->component = tex->component;
            ntex->texture_index = tex->texture_index;
            ntex->texture_non_uniform = tex->texture_non_uniform;
                        }
      static nir_phi_instr *
   clone_phi(clone_state *state, const nir_phi_instr *phi, nir_block *nblk)
   {
                        /* Cloning a phi node is a bit different from other instructions.  The
   * sources of phi instructions are the only time where we can use an SSA
   * def before it is defined.  In order to handle this, we just copy over
   * the sources from the old phi instruction directly and then fix them up
   * in a second pass once all the instrutions in the function have been
   * properly cloned.
   *
   * In order to ensure that the copied sources (which are the same as the
   * old phi instruction's sources for now) don't get inserted into the old
   * shader's use-def lists, we have to add the phi instruction *before* we
   * set up its sources.
   */
            nir_foreach_phi_src(src, phi) {
               /* Stash it in the list of phi sources.  We'll walk this list and fix up
   * sources at the very end of clone_function_impl.
   */
                  }
      static nir_jump_instr *
   clone_jump(clone_state *state, const nir_jump_instr *jmp)
   {
      /* These aren't handled because they require special block linking */
                        }
      static nir_call_instr *
   clone_call(clone_state *state, const nir_call_instr *call)
   {
      nir_function *ncallee = remap_global(state, call->callee);
            for (unsigned i = 0; i < ncall->num_params; i++)
               }
      static nir_instr *
   clone_instr(clone_state *state, const nir_instr *instr)
   {
      switch (instr->type) {
   case nir_instr_type_alu:
         case nir_instr_type_deref:
         case nir_instr_type_intrinsic:
         case nir_instr_type_load_const:
         case nir_instr_type_undef:
         case nir_instr_type_tex:
         case nir_instr_type_phi:
         case nir_instr_type_jump:
         case nir_instr_type_call:
         case nir_instr_type_parallel_copy:
         default:
      unreachable("bad instr type");
         }
      nir_instr *
   nir_instr_clone(nir_shader *shader, const nir_instr *orig)
   {
      clone_state state = {
      .allow_remap_fallback = true,
      };
      }
      nir_instr *
   nir_instr_clone_deep(nir_shader *shader, const nir_instr *orig,
         {
      clone_state state = {
      .allow_remap_fallback = true,
   .ns = shader,
      };
      }
      static nir_block *
   clone_block(clone_state *state, struct exec_list *cf_list, const nir_block *blk)
   {
      /* Don't actually create a new block.  Just use the one from the tail of
   * the list.  NIR guarantees that the tail of the list is a block and that
   * no two blocks are side-by-side in the IR;  It should be empty.
   */
   nir_block *nblk =
         assert(nblk->cf_node.type == nir_cf_node_block);
            /* We need this for phi sources */
            nir_foreach_instr(instr, blk) {
      if (instr->type == nir_instr_type_phi) {
      /* Phi instructions are a bit of a special case when cloning because
   * we don't want inserting the instruction to automatically handle
   * use/defs for us.  Instead, we need to wait until all the
   * blocks/instructions are in so that we can set their sources up.
   */
      } else {
      nir_instr *ninstr = clone_instr(state, instr);
                     }
      static void
   clone_cf_list(clone_state *state, struct exec_list *dst,
            static nir_if *
   clone_if(clone_state *state, struct exec_list *cf_list, const nir_if *i)
   {
      nir_if *ni = nir_if_create(state->ns);
                              clone_cf_list(state, &ni->then_list, &i->then_list);
               }
      static nir_loop *
   clone_loop(clone_state *state, struct exec_list *cf_list, const nir_loop *loop)
   {
      nir_loop *nloop = nir_loop_create(state->ns);
   nloop->control = loop->control;
                     clone_cf_list(state, &nloop->body, &loop->body);
   if (nir_loop_has_continue_construct(loop)) {
      nir_loop_add_continue_construct(nloop);
                  }
      /* clone list of nir_cf_node: */
   static void
   clone_cf_list(clone_state *state, struct exec_list *dst,
         {
      foreach_list_typed(nir_cf_node, cf, node, list) {
      switch (cf->type) {
   case nir_cf_node_block:
      clone_block(state, dst, nir_cf_node_as_block(cf));
      case nir_cf_node_if:
      clone_if(state, dst, nir_cf_node_as_if(cf));
      case nir_cf_node_loop:
      clone_loop(state, dst, nir_cf_node_as_loop(cf));
      default:
               }
      /* After we've cloned almost everything, we have to walk the list of phi
   * sources and fix them up.  Thanks to loops, the block and SSA value for a
   * phi source may not be defined when we first encounter it.  Instead, we
   * add it to the phi_srcs list and we fix it up here.
   */
   static void
   fixup_phi_srcs(clone_state *state)
   {
      list_for_each_entry_safe(nir_phi_src, src, &state->phi_srcs, src.use_link) {
               /* Remove from this list */
            src->src.ssa = remap_local(state, src->src.ssa);
      }
      }
      void
   nir_cf_list_clone(nir_cf_list *dst, nir_cf_list *src, nir_cf_node *parent,
         {
      exec_list_make_empty(&dst->list);
            if (exec_list_is_empty(&src->list))
            clone_state state;
            /* We use the same shader */
            /* The control-flow code assumes that the list of cf_nodes always starts
   * and ends with a block.  We start by adding an empty block.
   */
   nir_block *nblk = nir_block_create(state.ns);
   nblk->cf_node.parent = parent;
                              if (!remap_table)
      }
      static nir_function_impl *
   clone_function_impl(clone_state *state, const nir_function_impl *fi)
   {
               if (fi->preamble)
                                                /* All metadata is invalidated in the cloning process */
               }
      nir_function_impl *
   nir_function_impl_clone(nir_shader *shader, const nir_function_impl *fi)
   {
      clone_state state;
                                          }
      nir_function *
   nir_function_clone(nir_shader *ns, const nir_function *fxn)
   {
      nir_function *nfxn = nir_function_create(ns, fxn->name);
   nfxn->num_params = fxn->num_params;
   if (fxn->num_params) {
      nfxn->params = ralloc_array(ns, nir_parameter, fxn->num_params);
      }
   nfxn->is_entrypoint = fxn->is_entrypoint;
   nfxn->is_preamble = fxn->is_preamble;
   nfxn->should_inline = fxn->should_inline;
            /* At first glance, it looks like we should clone the function_impl here.
   * However, call instructions need to be able to reference at least the
   * function and those will get processed as we clone the function_impls.
   * We stop here and do function_impls as a second pass.
   */
      }
      static nir_function *
   clone_function(clone_state *state, const nir_function *fxn, nir_shader *ns)
   {
               nir_function *nfxn = nir_function_clone(ns, fxn);
   /* Needed for call instructions */
   add_remap(state, nfxn, fxn);
      }
      static u_printf_info *
   clone_printf_info(void *mem_ctx, const nir_shader *s)
   {
               for (unsigned i = 0; i < s->printf_info_count; i++) {
               infos[i].num_args = src_info->num_args;
   infos[i].arg_sizes = ralloc_size(mem_ctx,
               memcpy(infos[i].arg_sizes, src_info->arg_sizes,
               infos[i].string_size = src_info->string_size;
   infos[i].strings = ralloc_size(mem_ctx,
         memcpy(infos[i].strings, src_info->strings,
                  }
      nir_shader *
   nir_shader_clone(void *mem_ctx, const nir_shader *s)
   {
      clone_state state;
            nir_shader *ns = nir_shader_create(mem_ctx, s->info.stage, s->options, NULL);
                     /* Go through and clone functions */
   foreach_list_typed(nir_function, fxn, node, &s->functions)
            /* Only after all functions are cloned can we clone the actual function
   * implementations.  This is because nir_call_instrs and preambles need to
   * reference the functions of other functions and we don't know what order
   * the functions will have in the list.
   */
   nir_foreach_function_with_impl(fxn, impl, s) {
      nir_function *nfxn = remap_global(&state, fxn);
               ns->info = s->info;
   ns->info.name = ralloc_strdup(ns, ns->info.name);
   if (ns->info.label)
            ns->num_inputs = s->num_inputs;
   ns->num_uniforms = s->num_uniforms;
   ns->num_outputs = s->num_outputs;
            ns->constant_data_size = s->constant_data_size;
   if (s->constant_data_size > 0) {
      ns->constant_data = ralloc_size(ns, s->constant_data_size);
               if (s->xfb_info) {
      size_t size = nir_xfb_info_size(s->xfb_info->output_count);
   ns->xfb_info = ralloc_size(ns, size);
               if (s->printf_info_count > 0) {
      ns->printf_info = clone_printf_info(ns, s);
                           }
      /** Overwrites dst and replaces its contents with src
   *
   * Everything ralloc parented to dst and src itself (but not its children)
   * will be freed.
   *
   * This should only be used by test code which needs to swap out shaders with
   * a cloned or deserialized version.
   */
   void
   nir_shader_replace(nir_shader *dst, nir_shader *src)
   {
      /* Delete all of dest's ralloc children */
   void *dead_ctx = ralloc_context(NULL);
   ralloc_adopt(dead_ctx, dst);
            /* Re-parent all of src's ralloc children to dst */
                     /* We have to move all the linked lists over separately because we need the
   * pointers in the list elements to point to the lists in dst and not src.
   */
            /* Now move the functions over.  This takes a tiny bit more work */
   exec_list_move_nodes_to(&src->functions, &dst->functions);
   nir_foreach_function(function, dst)
               }
