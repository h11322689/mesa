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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/set.h"
   #include "nir.h"
   #include "nir_builder.h"
      /* This pass provides a way to move computations that are always the same for
   * an entire draw/compute dispatch into a "preamble" that runs before the main
   * entrypoint.
   *
   * We also expose a separate API to get or construct the preamble of a shader
   * in case backends want to insert their own code.
   */
      nir_function_impl *
   nir_shader_get_preamble(nir_shader *shader)
   {
      nir_function_impl *entrypoint = nir_shader_get_entrypoint(shader);
   if (entrypoint->preamble) {
         } else {
      nir_function *preamble = nir_function_create(shader, "@preamble");
   preamble->is_preamble = true;
   nir_function_impl *impl = nir_function_impl_create(preamble);
   entrypoint->preamble = preamble;
         }
      typedef struct {
      bool can_move;
   bool candidate;
   bool must_stay;
                                       /* Average the cost of a value among its users, to try to account for
   * values that have multiple can_move uses.
   */
            /* Overall benefit, i.e. the value minus any cost to inserting
   * load_preamble.
   */
      } def_state;
      typedef struct {
      /* Per-definition array of states */
            /* Number of levels of non-uniform control flow we're in. We don't
   * reconstruct loops, so loops count as non-uniform conservatively. If-else
   * is counted if the condition is not marked can_move.
   */
            /* Set of nir_if's that must be reconstructed in the preamble. Note an if may
   * need reconstruction even when not entirely moved. This does not account
   * for nesting: the parent CF nodes of ifs in this set must be reconstructed
   * but may not be in this set, even if the parent is another if.
   */
            /* Set of definitions that must be reconstructed in the preamble. This is a
   * subset of can_move instructions, determined after replacement.
   */
                        } opt_preamble_ctx;
      static bool
   instr_can_speculate(nir_instr *instr)
   {
      /* Intrinsics with an ACCESS index can only be speculated if they are
   * explicitly CAN_SPECULATE.
   */
   if (instr->type == nir_instr_type_intrinsic) {
               if (nir_intrinsic_has_access(intr))
               /* For now, everything else can be speculated. TODO: Bindless textures. */
      }
      static float
   get_instr_cost(nir_instr *instr, const nir_opt_preamble_options *options)
   {
      /* No backend will want to hoist load_const or undef by itself, so handle
   * this for them.
   */
   if (instr->type == nir_instr_type_load_const ||
      instr->type == nir_instr_type_undef)
            }
      static bool
   can_move_src(nir_src *src, void *state)
   {
                  }
      static bool
   can_move_srcs(nir_instr *instr, opt_preamble_ctx *ctx)
   {
         }
      static bool
   can_move_intrinsic(nir_intrinsic_instr *instr, opt_preamble_ctx *ctx)
   {
      switch (instr->intrinsic) {
   /* Intrinsics which can always be moved */
   case nir_intrinsic_load_push_constant:
   case nir_intrinsic_load_work_dim:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_workgroup_size:
   case nir_intrinsic_load_ray_launch_size:
   case nir_intrinsic_load_ray_launch_size_addr_amd:
   case nir_intrinsic_load_sbt_base_amd:
   case nir_intrinsic_load_is_indexed_draw:
   case nir_intrinsic_load_viewport_scale:
   case nir_intrinsic_load_user_clip_plane:
   case nir_intrinsic_load_viewport_x_scale:
   case nir_intrinsic_load_viewport_y_scale:
   case nir_intrinsic_load_viewport_z_scale:
   case nir_intrinsic_load_viewport_offset:
   case nir_intrinsic_load_viewport_x_offset:
   case nir_intrinsic_load_viewport_y_offset:
   case nir_intrinsic_load_viewport_z_offset:
   case nir_intrinsic_load_blend_const_color_a_float:
   case nir_intrinsic_load_blend_const_color_b_float:
   case nir_intrinsic_load_blend_const_color_g_float:
   case nir_intrinsic_load_blend_const_color_r_float:
   case nir_intrinsic_load_blend_const_color_rgba:
   case nir_intrinsic_load_blend_const_color_aaaa8888_unorm:
   case nir_intrinsic_load_blend_const_color_rgba8888_unorm:
   case nir_intrinsic_load_line_width:
   case nir_intrinsic_load_aa_line_width:
   case nir_intrinsic_load_fb_layers_v3d:
   case nir_intrinsic_load_tcs_num_patches_amd:
   case nir_intrinsic_load_sample_positions_pan:
   case nir_intrinsic_load_pipeline_stat_query_enabled_amd:
   case nir_intrinsic_load_prim_gen_query_enabled_amd:
   case nir_intrinsic_load_prim_xfb_query_enabled_amd:
   case nir_intrinsic_load_clamp_vertex_color_amd:
   case nir_intrinsic_load_cull_front_face_enabled_amd:
   case nir_intrinsic_load_cull_back_face_enabled_amd:
   case nir_intrinsic_load_cull_ccw_amd:
   case nir_intrinsic_load_cull_small_primitives_enabled_amd:
   case nir_intrinsic_load_cull_any_enabled_amd:
   case nir_intrinsic_load_cull_small_prim_precision_amd:
   case nir_intrinsic_load_vbo_base_agx:
            /* Intrinsics which can be moved depending on hardware */
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_load_base_vertex:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_draw_id:
            case nir_intrinsic_load_subgroup_size:
   case nir_intrinsic_load_num_subgroups:
            /* Intrinsics which can be moved if the sources can */
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ubo_vec4:
   case nir_intrinsic_get_ubo_size:
   case nir_intrinsic_get_ssbo_size:
   case nir_intrinsic_ballot_bitfield_extract:
   case nir_intrinsic_ballot_find_lsb:
   case nir_intrinsic_ballot_find_msb:
   case nir_intrinsic_ballot_bit_count_reduce:
   case nir_intrinsic_load_deref:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_preamble:
   case nir_intrinsic_load_constant:
   case nir_intrinsic_load_sample_pos_from_id:
   case nir_intrinsic_load_kernel_input:
   case nir_intrinsic_load_buffer_amd:
   case nir_intrinsic_image_samples:
   case nir_intrinsic_image_deref_samples:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_vulkan_resource_index:
   case nir_intrinsic_vulkan_resource_reindex:
   case nir_intrinsic_load_vulkan_descriptor:
   case nir_intrinsic_quad_swizzle_amd:
   case nir_intrinsic_masked_swizzle_amd:
   case nir_intrinsic_load_ssbo_address:
   case nir_intrinsic_bindless_resource_ir3:
   case nir_intrinsic_load_constant_agx:
            /* Image/SSBO loads can be moved if they are CAN_REORDER and their
   * sources can be moved.
   */
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_samples_identical:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_ssbo_ir3:
      return (nir_intrinsic_access(instr) & ACCESS_CAN_REORDER) &&
         default:
            }
      static bool
   can_move_instr(nir_instr *instr, opt_preamble_ctx *ctx)
   {
      /* If we are only contained within uniform control flow, no speculation is
   * needed since the control flow will be reconstructed in the preamble. But
   * if we are not, we must be able to speculate instructions to move them.
   */
   if (ctx->nonuniform_cf_nesting > 0 && !instr_can_speculate(instr))
            switch (instr->type) {
   case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
   /* See note below about derivatives. We have special code to convert tex
   * to txd, though, because it's a common case.
   */
   if (nir_tex_instr_has_implicit_derivative(tex) &&
      tex->op != nir_texop_tex) {
      }
      }
   case nir_instr_type_alu: {
      /* The preamble is presumably run with only one thread, so we can't run
   * derivatives in it.
   * TODO: Replace derivatives with 0 instead, if real apps hit this.
   */
   nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (nir_op_is_derivative(alu->op))
         else
      }
   case nir_instr_type_intrinsic:
            case nir_instr_type_load_const:
   case nir_instr_type_undef:
            case nir_instr_type_deref: {
      nir_deref_instr *deref = nir_instr_as_deref(instr);
   if (deref->deref_type == nir_deref_type_var) {
      switch (deref->modes) {
   case nir_var_uniform:
   case nir_var_mem_ubo:
         default:
            } else {
                     /* We can only move phis if all of their sources are movable, and it is a phi
   * for an if-else that is itself movable.
   */
   case nir_instr_type_phi: {
      nir_cf_node *prev_node = nir_cf_node_prev(&instr->block->cf_node);
   if (!prev_node)
            if (prev_node->type != nir_cf_node_if) {
      assert(prev_node->type == nir_cf_node_loop);
               nir_if *nif = nir_cf_node_as_if(prev_node);
   if (!can_move_src(&nif->condition, ctx))
                        default:
            }
      /* True if we should avoid making this a candidate. This is only called on
   * instructions we already determined we can move, this just makes it so that
   * uses of this instruction cannot be rewritten. Typically this happens
   * because of static constraints on the IR, for example some deref chains
   * cannot be broken.
   */
   static bool
   avoid_instr(nir_instr *instr, const nir_opt_preamble_options *options)
   {
      if (instr->type == nir_instr_type_deref)
               }
      static bool
   update_src_value(nir_src *src, void *data)
   {
               def_state *state = &ctx->states[ctx->def->index];
                     /* If an instruction has can_move and non-can_move users, it becomes a
   * candidate and its value shouldn't propagate downwards. For example,
   * imagine a chain like this:
   *
   *         -- F (cannot move)
   *        /
   *  A <-- B <-- C <-- D <-- E (cannot move)
   *
   * B and D are marked candidates. Picking B removes A and B, picking D
   * removes C and D, and picking both removes all 4. Therefore B and D are
   * independent and B's value shouldn't flow into D.
   *
   * A similar argument holds for must_stay values.
   */
   if (!src_state->must_stay && !src_state->candidate)
            }
      static int
   candidate_sort(const void *data1, const void *data2)
   {
      const def_state *state1 = *(def_state **)data1;
            float value1 = state1->value / state1->size;
   float value2 = state2->value / state2->size;
   if (value1 < value2)
         else if (value1 > value2)
         else
      }
      static bool
   calculate_can_move_for_block(opt_preamble_ctx *ctx, nir_block *block)
   {
               nir_foreach_instr(instr, block) {
      nir_def *def = nir_instr_def(instr);
   if (!def)
            def_state *state = &ctx->states[def->index];
   state->can_move = can_move_instr(instr, ctx);
                  }
      static bool
   calculate_can_move_for_cf_list(opt_preamble_ctx *ctx, struct exec_list *list)
   {
               foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block:
      all_can_move &=
               case nir_cf_node_if: {
                                    bool if_can_move = uniform;
                                 all_can_move &= if_can_move;
               case nir_cf_node_loop: {
               /* Conservatively treat loops like conditional control flow, since an
   * instruction might be conditionally unreachabled due to an earlier
   * break in a loop that executes only one iteration.
   */
   ctx->nonuniform_cf_nesting++;
   calculate_can_move_for_cf_list(ctx, &loop->body);
   ctx->nonuniform_cf_nesting--;
   all_can_move = false;
               default:
                        }
      static void
   replace_for_block(nir_builder *b, opt_preamble_ctx *ctx,
         {
      nir_foreach_instr(instr, block) {
      nir_def *def = nir_instr_def(instr);
   if (!def)
            /* Only replace what we actually need. This is a micro-optimization for
   * compile-time performance of regular instructions, but it's required for
   * correctness with phi nodes, since we might not reconstruct the
   * corresponding if.
   */
   if (!BITSET_TEST(ctx->reconstructed_defs, def->index))
            def_state *state = &ctx->states[def->index];
                     if (instr->type == nir_instr_type_phi) {
                     nir_cf_node *nif_cf = nir_cf_node_prev(&block->cf_node);
                                          nir_foreach_phi_src(phi_src, phi) {
      if (phi_src->pred == then_block) {
      assert(then_def == NULL);
      } else if (phi_src->pred == else_block) {
      assert(else_def == NULL);
      } else {
                                    /* Remap */
                                                   } else {
      clone = nir_instr_clone_deep(b->shader, instr, remap_table);
               if (clone->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(clone);
   if (tex->op == nir_texop_tex) {
      /* For maximum compatibility, replace normal textures with
   * textureGrad with a gradient of 0.
   * TODO: Handle txb somehow.
                  nir_def *zero =
         nir_tex_instr_add_src(tex, nir_tex_src_ddx, zero);
                                 if (state->replace) {
      nir_def *clone_def = nir_instr_def(clone);
            }
      static void
   replace_for_cf_list(nir_builder *b, opt_preamble_ctx *ctx,
         {
      foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block: {
      replace_for_block(b, ctx, remap_table, nir_cf_node_as_block(node));
               case nir_cf_node_if: {
               /* If we moved something that requires reconstructing the if, do so */
                     struct hash_entry *entry =
                                                                     nir_pop_if(b, reconstructed_nif);
      } else {
      replace_for_cf_list(b, ctx, remap_table, &nif->then_list);
                           case nir_cf_node_loop: {
      /* We don't try to reconstruct loops */
   nir_loop *loop = nir_cf_node_as_loop(node);
   replace_for_cf_list(b, ctx, remap_table, &loop->body);
               default:
               }
      /*
   * If an if-statement contains an instruction that cannot be speculated, the
   * if-statement must be reconstructed so we avoid the speculation. This applies
   * even for nested if-statements. Determine which if-statements must be
   * reconstructed for this reason by walking the program forward and looking
   * inside uniform if's.
   *
   * Returns whether the CF list contains a reconstructed instruction that would
   * otherwise be speculated, updating the reconstructed_ifs set. This depends on
   * reconstructed_defs being correctly set by analyze_reconstructed.
   */
   static bool
   analyze_speculation_for_cf_list(opt_preamble_ctx *ctx, struct exec_list *list)
   {
               foreach_list_typed(nir_cf_node, node, node, list) {
      switch (node->type) {
   case nir_cf_node_block: {
      nir_foreach_instr(instr, nir_cf_node_as_block(node)) {
      nir_def *def = nir_instr_def(instr);
                                 if (!instr_can_speculate(instr)) {
      reconstruct_cf_list = true;
                              case nir_cf_node_if: {
               /* If we can move the if, we might need to reconstruct */
   if (can_move_src(&nif->condition, ctx)) {
      bool any = false;
                                                         /* We don't reconstruct loops */
   default:
                        }
      static bool
   mark_reconstructed(nir_src *src, void *state)
   {
      BITSET_WORD *reconstructed_defs = state;
   BITSET_SET(reconstructed_defs, src->ssa->index);
      }
      /*
   * If a phi is moved into the preamble, then the if it depends on must also be
   * moved. However, it is not necessary to consider any nested control flow. As
   * an example, if we have a shader:
   *
   *    if (not moveable condition) {
   *       if (moveable condition) {
   *          x = moveable
   *       }
   *       y = phi x, moveable
   *       z = floor y
   *    }
   *
   * Then if 'z' is in the replace set, we need to reconstruct the inner if, but
   * not the outer if, unless there's also speculation to worry about.
   *
   * We do this by marking defs that need to be reconstructed, with a backwards
   * sweep of the program (compatible with reverse dominance), and marking the
   * if's preceding reconstructed phis.
   */
   static void
   analyze_reconstructed(opt_preamble_ctx *ctx, nir_function_impl *impl)
   {
      nir_foreach_block_reverse(block, impl) {
      /* If an if-statement is reconstructed, its condition must be as well */
   nir_if *nif = nir_block_get_following_if(block);
   if (nif && _mesa_set_search(ctx->reconstructed_ifs, nif))
            nir_foreach_instr_reverse(instr, block) {
      nir_def *def = nir_instr_def(instr);
                           /* Anything that's replaced must be reconstructed */
   if (state->replace)
                                                      /* Reconstructed phis need their ifs reconstructed */
                     /* Invariants guaranteed by can_move_instr */
                                 /* Mark the if for reconstruction */
               }
      bool
   nir_opt_preamble(nir_shader *shader, const nir_opt_preamble_options *options,
         {
      opt_preamble_ctx ctx = {
                  nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            /* Step 1: Calculate can_move */
            /* Step 2: Calculate is_candidate. This is complicated by the presence of
   * non-candidate instructions like derefs whose users cannot be rewritten.
   * If a deref chain is used at all by a non-can_move thing, then any offset
   * sources anywhere along the chain should be considered candidates because
   * the entire deref chain will never be deleted, but if it's only used by
   * can_move things then it becomes subsumed by its users and none of the
   * offset sources should be considered candidates as they will be removed
   * when the users of the deref chain are moved. We need to replace "are
   * there any non-can_move users" with "are there any non-can_move users,
   * *recursing through non-candidate users*". We do this by walking backward
   * and marking when a non-candidate instruction must stay in the final
   * program because it has a non-can_move user, including recursively.
   */
   unsigned num_candidates = 0;
   nir_foreach_block_reverse(block, impl) {
      nir_foreach_instr_reverse(instr, block) {
      nir_def *def = nir_instr_def(instr);
                  def_state *state = &ctx.states[def->index];
                  state->value = get_instr_cost(instr, options);
   bool is_candidate = !avoid_instr(instr, options);
   state->candidate = false;
   state->must_stay = false;
                     if (nir_src_is_if(use)) {
         } else {
      nir_def *use_def = nir_instr_def(nir_src_parent_instr(use));
   is_can_move_user = use_def != NULL &&
                     if (is_can_move_user) {
         } else {
      if (is_candidate)
         else
                  if (state->candidate)
                  if (num_candidates == 0) {
      free(ctx.states);
               def_state **candidates = malloc(sizeof(*candidates) * num_candidates);
   unsigned candidate_idx = 0;
            /* Step 3: Calculate value of candidates by propagating downwards. We try
   * to share the value amongst can_move uses, in case there are multiple.
   * This won't always find the most optimal solution, but is hopefully a
   * good heuristic.
   *
   * Note that we use the can_move adjusted in the last pass, because if a
   * can_move instruction cannot be moved because it's not a candidate and it
   * has a non-can_move source then we don't want to count it as a use.
   *
   * While we're here, also collect an array of candidates.
   */
   nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
      nir_def *def = nir_instr_def(instr);
                  def_state *state = &ctx.states[def->index];
                                 /* If this instruction is a candidate, its value shouldn't be
   * propagated so we skip dividing it.
   *
   * Note: if it's can_move but not a candidate, then all its users
   * must be can_move, so if there are no users then it must be dead.
   */
   if (!state->candidate && !state->must_stay) {
      if (state->can_move_users > 0)
         else
               if (state->candidate) {
                     if (state->benefit > 0) {
      options->def_size(def, &state->size, &state->align);
   total_size = ALIGN_POT(total_size, state->align);
   total_size += state->size;
                        assert(candidate_idx <= num_candidates);
            if (num_candidates == 0) {
      free(ctx.states);
   free(candidates);
               /* Step 4: Figure out which candidates we're going to replace and assign an
   * offset. Assuming there is no expression sharing, this is similar to the
   * 0-1 knapsack problem, except when there is a gap introduced by
   * alignment. We use a well-known greedy approximation, sorting by value
   * divided by size.
            if (((*size) + total_size) > options->preamble_storage_size) {
                  unsigned offset = *size;
   for (unsigned i = 0; i < num_candidates; i++) {
      def_state *state = candidates[i];
            if (offset + state->size > options->preamble_storage_size)
            state->replace = true;
                                          /* Determine which if's need to be reconstructed, based on the replacements
   * we did.
   */
   ctx.reconstructed_ifs = _mesa_pointer_set_create(NULL);
   ctx.reconstructed_defs = calloc(BITSET_WORDS(impl->ssa_alloc),
                  /* If we make progress analyzing speculation, we need to re-analyze
   * reconstructed defs to get the if-conditions in there.
   */
   if (analyze_speculation_for_cf_list(&ctx, &impl->body))
            /* Step 5: Actually do the replacement. */
   struct hash_table *remap_table =
         nir_function_impl *preamble =
         nir_builder preamble_builder = nir_builder_at(nir_before_impl(preamble));
                     nir_builder builder = nir_builder_create(impl);
            unsigned max_index = impl->ssa_alloc;
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
      nir_def *def = nir_instr_def(instr);
                  /* Ignore new load_preamble instructions */
                  def_state *state = &ctx.states[def->index];
                           nir_def *new_def =
                  nir_def_rewrite_uses(def, new_def);
                  nir_metadata_preserve(impl,
                  ralloc_free(remap_table);
   free(ctx.states);
   free(ctx.reconstructed_defs);
   _mesa_set_destroy(ctx.reconstructed_ifs, NULL);
      }
