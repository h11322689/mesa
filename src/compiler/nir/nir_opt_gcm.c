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
   */
      #include "nir.h"
   #include "nir_instr_set.h"
      /*
   * Implements Global Code Motion.  A description of GCM can be found in
   * "Global Code Motion; Global Value Numbering" by Cliff Click.
   * Unfortunately, the algorithm presented in the paper is broken in a
   * number of ways.  The algorithm used here differs substantially from the
   * one in the paper but it is, in my opinion, much easier to read and
   * verify correcness.
   */
      /* This is used to stop GCM moving instruction out of a loop if the loop
   * contains too many instructions and moving them would create excess spilling.
   *
   * TODO: Figure out a better way to decide if we should remove instructions from
   * a loop.
   */
   #define MAX_LOOP_INSTRUCTIONS 100
      struct gcm_block_info {
      /* Number of loops this block is inside */
            /* Number of ifs this block is inside */
                     /* The loop the block is nested inside or NULL */
            /* The last instruction inserted into this block.  This is used as we
   * traverse the instructions and insert them back into the program to
   * put them in the right order.
   */
      };
      struct gcm_instr_info {
         };
      /* Flags used in the instr->pass_flags field for various instruction states */
   enum {
      GCM_INSTR_PINNED = (1 << 0),
   GCM_INSTR_SCHEDULE_EARLIER_ONLY = (1 << 1),
   GCM_INSTR_SCHEDULED_EARLY = (1 << 2),
   GCM_INSTR_SCHEDULED_LATE = (1 << 3),
      };
      struct gcm_state {
      nir_function_impl *impl;
                     /* The list of non-pinned instructions.  As we do the late scheduling,
   * we pull non-pinned instructions out of their blocks and place them in
   * this list.  This saves us from having linked-list problems when we go
   * to put instructions back in their blocks.
   */
                     unsigned num_instrs;
      };
      static unsigned
   get_loop_instr_count(struct exec_list *cf_list)
   {
      unsigned loop_instr_count = 0;
   foreach_list_typed(nir_cf_node, node, node, cf_list) {
      switch (node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
   nir_foreach_instr(instr, block) {
         }
      }
   case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
   loop_instr_count += get_loop_instr_count(&if_stmt->then_list);
   loop_instr_count += get_loop_instr_count(&if_stmt->else_list);
      }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
   assert(!nir_loop_has_continue_construct(loop));
   loop_instr_count += get_loop_instr_count(&loop->body);
      }
   default:
                        }
      /* Recursively walks the CFG and builds the block_info structure */
   static void
   gcm_build_block_info(struct exec_list *cf_list, struct gcm_state *state,
               {
      foreach_list_typed(nir_cf_node, node, node, cf_list) {
      switch (node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
   state->blocks[block->index].if_depth = if_depth;
   state->blocks[block->index].loop_depth = loop_depth;
   state->blocks[block->index].loop_instr_count = loop_instr_count;
   state->blocks[block->index].loop = loop;
      }
   case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
   gcm_build_block_info(&if_stmt->then_list, state, loop, loop_depth,
         gcm_build_block_info(&if_stmt->else_list, state, loop, loop_depth,
            }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
   assert(!nir_loop_has_continue_construct(loop));
   gcm_build_block_info(&loop->body, state, loop, loop_depth + 1, if_depth,
            }
   default:
               }
      static bool
   is_src_scalarizable(nir_src *src)
   {
         nir_instr *src_instr = src->ssa->parent_instr;
   switch (src_instr->type) {
   case nir_instr_type_alu: {
               /* ALU operations with output_size == 0 should be scalarized.  We
   * will also see a bunch of vecN operations from scalarizing ALU
   * operations and, since they can easily be copy-propagated, they
   * are ok too.
   */
   return nir_op_infos[src_alu->op].output_size == 0 ||
         src_alu->op == nir_op_vec2 ||
               case nir_instr_type_load_const:
      /* These are trivially scalarizable */
         case nir_instr_type_undef:
            case nir_instr_type_intrinsic: {
               switch (src_intrin->intrinsic) {
   case nir_intrinsic_load_deref: {
      /* Don't scalarize if we see a load of a local variable because it
   * might turn into one of the things we can't scalarize.
   */
   nir_deref_instr *deref = nir_src_as_deref(src_intrin->src[0]);
   return !nir_deref_mode_may_be(deref, (nir_var_function_temp |
               case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_global:
   case nir_intrinsic_load_global_constant:
   case nir_intrinsic_load_input:
         default:
                              default:
      /* We can't scalarize this type of instruction */
         }
      static bool
   is_binding_uniform(nir_src src)
   {
      nir_binding binding = nir_chase_binding(src);
   if (!binding.success)
            for (unsigned i = 0; i < binding.num_indices; i++) {
      if (!nir_src_is_always_uniform(binding.indices[i]))
                  }
      static void
   pin_intrinsic(nir_intrinsic_instr *intrin)
   {
               if (!nir_intrinsic_can_reorder(intrin)) {
      instr->pass_flags = GCM_INSTR_PINNED;
                        /* If the intrinsic requires a uniform source, we can't safely move it across non-uniform
   * control flow if it's not uniform at the point it's defined.
   * Stores and atomics can never be re-ordered, so we don't have to consider them here.
   */
   bool non_uniform = nir_intrinsic_has_access(intrin) &&
         if (!non_uniform &&
      (intrin->intrinsic == nir_intrinsic_load_ubo ||
   intrin->intrinsic == nir_intrinsic_load_ssbo ||
   intrin->intrinsic == nir_intrinsic_get_ubo_size ||
   intrin->intrinsic == nir_intrinsic_get_ssbo_size ||
   nir_intrinsic_has_image_dim(intrin) ||
   ((intrin->intrinsic == nir_intrinsic_load_deref ||
      intrin->intrinsic == nir_intrinsic_deref_buffer_array_length) &&
   nir_deref_mode_may_be(nir_src_as_deref(intrin->src[0]),
      if (!is_binding_uniform(intrin->src[0]))
      } else if (intrin->intrinsic == nir_intrinsic_load_push_constant) {
      if (!nir_src_is_always_uniform(intrin->src[0]))
      } else if (intrin->intrinsic == nir_intrinsic_load_deref &&
            nir_deref_mode_is(nir_src_as_deref(intrin->src[0]),
   nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
   while (deref->deref_type != nir_deref_type_var) {
      if ((deref->deref_type == nir_deref_type_array ||
      deref->deref_type == nir_deref_type_ptr_as_array) &&
   !nir_src_is_always_uniform(deref->arr.index)) {
   instr->pass_flags = GCM_INSTR_PINNED;
      }
   deref = nir_deref_instr_parent(deref);
   if (!deref) {
      instr->pass_flags = GCM_INSTR_PINNED;
               }
      /* Walks the instruction list and marks immovable instructions as pinned or
   * placed.
   *
   * This function also serves to initialize the instr->pass_flags field.
   * After this is completed, all instructions' pass_flags fields will be set
   * to either GCM_INSTR_PINNED, GCM_INSTR_PLACED or 0.
   */
   static void
   gcm_pin_instructions(nir_function_impl *impl, struct gcm_state *state)
   {
               nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     switch (instr->type) {
                     if (nir_op_is_derivative(alu->op)) {
      /* These can only go in uniform control flow */
      } else if (alu->op == nir_op_mov &&
               } else {
         }
               case nir_instr_type_tex: {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
                  for (unsigned i = 0; i < tex->num_srcs; i++) {
      nir_tex_src *src = &tex->src[i];
   switch (src->src_type) {
   case nir_tex_src_texture_deref:
      if (!tex->texture_non_uniform && !is_binding_uniform(src->src))
            case nir_tex_src_sampler_deref:
      if (!tex->sampler_non_uniform && !is_binding_uniform(src->src))
            case nir_tex_src_texture_offset:
   case nir_tex_src_texture_handle:
      if (!tex->texture_non_uniform && !nir_src_is_always_uniform(src->src))
            case nir_tex_src_sampler_offset:
   case nir_tex_src_sampler_handle:
      if (!tex->sampler_non_uniform && !nir_src_is_always_uniform(src->src))
            default:
            }
               case nir_instr_type_deref:
   case nir_instr_type_load_const:
                  case nir_instr_type_intrinsic:
                  case nir_instr_type_call:
                  case nir_instr_type_jump:
   case nir_instr_type_undef:
   case nir_instr_type_phi:
                  default:
                  if (!(instr->pass_flags & GCM_INSTR_PLACED)) {
      /* If this is an unplaced instruction, go ahead and pull it out of
   * the program and put it on the instrs list.  This has a couple
   * of benifits.  First, it makes the scheduling algorithm more
   * efficient because we can avoid walking over basic blocks.
   * Second, it keeps us from causing linked list confusion when
   * we're trying to put everything in its proper place at the end
   * of the pass.
   *
   * Note that we don't use nir_instr_remove here because that also
   * cleans up uses and defs and we want to keep that information.
   */
   exec_node_remove(&instr->node);
               }
      static void
   gcm_schedule_early_instr(nir_instr *instr, struct gcm_state *state);
      /** Update an instructions schedule for the given source
   *
   * This function is called iteratively as we walk the sources of an
   * instruction.  It ensures that the given source instruction has been
   * scheduled and then update this instruction's block if the source
   * instruction is lower down the tree.
   */
   static bool
   gcm_schedule_early_src(nir_src *src, void *void_state)
   {
      struct gcm_state *state = void_state;
                     /* While the index isn't a proper dominance depth, it does have the
   * property that if A dominates B then A->index <= B->index.  Since we
   * know that this instruction must have been dominated by all of its
   * sources at some point (even if it's gone through value-numbering),
   * all of the sources must lie on the same branch of the dominance tree.
   * Therefore, we can just go ahead and just compare indices.
   */
   struct gcm_instr_info *src_info =
         struct gcm_instr_info *info = &state->instr_infos[instr->index];
   if (info->early_block->index < src_info->early_block->index)
            /* We need to restore the state instruction because it may have been
   * changed through the gcm_schedule_early_instr call above.  Since we
   * may still be iterating through sources and future calls to
   * gcm_schedule_early_src for the same instruction will still need it.
   */
               }
      /** Schedules an instruction early
   *
   * This function performs a recursive depth-first search starting at the
   * given instruction and proceeding through the sources to schedule
   * instructions as early as they can possibly go in the dominance tree.
   * The instructions are "scheduled" by updating the early_block field of
   * the corresponding gcm_instr_state entry.
   */
   static void
   gcm_schedule_early_instr(nir_instr *instr, struct gcm_state *state)
   {
      if (instr->pass_flags & GCM_INSTR_SCHEDULED_EARLY)
                     /* Pinned/placed instructions always get scheduled in their original block so
   * we don't need to do anything.  Also, bailing here keeps us from ever
   * following the sources of phi nodes which can be back-edges.
   */
   if (instr->pass_flags & GCM_INSTR_PINNED ||
      instr->pass_flags & GCM_INSTR_PLACED) {
   state->instr_infos[instr->index].early_block = instr->block;
               /* Start with the instruction at the top.  As we iterate over the
   * sources, it will get moved down as needed.
   */
   state->instr_infos[instr->index].early_block = nir_start_block(state->impl);
               }
      static bool
   set_block_for_loop_instr(struct gcm_state *state, nir_instr *instr,
         {
      /* If the instruction wasn't in a loop to begin with we don't want to push
   * it down into one.
   */
   nir_loop *loop = state->blocks[instr->block->index].loop;
   if (loop == NULL)
            assert(!nir_loop_has_continue_construct(loop));
   if (nir_block_dominates(instr->block, block))
            /* If the loop only executes a single time i.e its wrapped in a:
   *    do{ ... break; } while(true)
   * Don't move the instruction as it will not help anything.
   */
   if (loop->info->limiting_terminator == NULL && !loop->info->complex_loop &&
      nir_block_ends_in_break(nir_loop_last_block(loop)))
         /* Being too aggressive with how we pull instructions out of loops can
   * result in extra register pressure and spilling. For example its fairly
   * common for loops in compute shaders to calculate SSBO offsets using
   * the workgroup id, subgroup id and subgroup invocation, pulling all
   * these calculations outside the loop causes register pressure.
   *
   * To work around these issues for now we only allow constant and texture
   * instructions to be moved outside their original loops, or instructions
   * where the total loop instruction count is less than
   * MAX_LOOP_INSTRUCTIONS.
   *
   * TODO: figure out some more heuristics to allow more to be moved out of
   * loops.
   */
   if (state->blocks[instr->block->index].loop_instr_count < MAX_LOOP_INSTRUCTIONS)
            if (instr->type == nir_instr_type_load_const ||
      instr->type == nir_instr_type_tex ||
   (instr->type == nir_instr_type_intrinsic &&
   nir_instr_as_intrinsic(instr)->intrinsic == nir_intrinsic_resource_intel))
            }
      static bool
   set_block_to_if_block(struct gcm_state *state, nir_instr *instr,
         {
      if (instr->type == nir_instr_type_load_const)
            if (instr->type == nir_instr_type_intrinsic &&
      nir_instr_as_intrinsic(instr)->intrinsic == nir_intrinsic_resource_intel)
         /* TODO: Figure out some more heuristics to allow more to be moved into
   * if-statements.
               }
      static nir_block *
   gcm_choose_block_for_instr(nir_instr *instr, nir_block *early_block,
         {
                        /* First see if we can push the instruction down into an if-statements block */
   nir_block *best = late_block;
   for (nir_block *block = late_block; block != NULL; block = block->imm_dom) {
      if (state->blocks[block->index].loop_depth >
                  if (state->blocks[block->index].if_depth >=
            set_block_to_if_block(state, instr, block)) {
   /* If we are pushing the instruction into an if we want it to be
   * in the earliest block not the latest to avoid creating register
   * pressure issues. So we don't break unless we come across the
   * block the instruction was originally in.
   */
   best = block;
   block_set = true;
   if (block == instr->block)
      } else if (block == instr->block) {
      /* If we couldn't push the instruction later just put is back where it
   * was previously.
   */
   if (!block_set)
                     if (block == early_block)
               /* Now see if we can evict the instruction from a loop */
   for (nir_block *block = late_block; block != NULL; block = block->imm_dom) {
      if (state->blocks[block->index].loop_depth <
      state->blocks[best->index].loop_depth) {
   if (set_block_for_loop_instr(state, instr, block)) {
         } else if (block == instr->block) {
      if (!block_set)
                        if (block == early_block)
                  }
      static void
   gcm_schedule_late_instr(nir_instr *instr, struct gcm_state *state);
      /** Schedules the instruction associated with the given SSA def late
   *
   * This function works by first walking all of the uses of the given SSA
   * definition, ensuring that they are scheduled, and then computing the LCA
   * (least common ancestor) of its uses.  It then schedules this instruction
   * as close to the LCA as possible while trying to stay out of loops.
   */
   static bool
   gcm_schedule_late_def(nir_def *def, void *void_state)
   {
                        nir_foreach_use(use_src, def) {
                        /* Phi instructions are a bit special.  SSA definitions don't have to
   * dominate the sources of the phi nodes that use them; instead, they
   * have to dominate the predecessor block corresponding to the phi
   * source.  We handle this by looking through the sources, finding
   * any that are usingg this SSA def, and using those blocks instead
   * of the one the phi lives in.
   */
   if (use_instr->type == nir_instr_type_phi) {
               nir_foreach_phi_src(phi_src, phi) {
      if (phi_src->src.ssa == def)
         } else {
                     nir_foreach_if_use(use_src, def) {
               /* For if statements, we consider the block to be the one immediately
   * preceding the if CF node.
   */
   nir_block *pred_block =
                        nir_block *early_block =
            /* Some instructions may never be used.  Flag them and the instruction
   * placement code will get rid of them for us.
   */
   if (lca == NULL) {
      def->parent_instr->block = NULL;
               if (def->parent_instr->pass_flags & GCM_INSTR_SCHEDULE_EARLIER_ONLY &&
      lca != def->parent_instr->block &&
   nir_block_dominates(def->parent_instr->block, lca)) {
               /* We now have the LCA of all of the uses.  If our invariants hold,
   * this is dominated by the block that we chose when scheduling early.
   * We now walk up the dominance tree and pick the lowest block that is
   * as far outside loops as we can get.
   */
   nir_block *best_block =
            if (def->parent_instr->block != best_block)
                        }
      /** Schedules an instruction late
   *
   * This function performs a depth-first search starting at the given
   * instruction and proceeding through its uses to schedule instructions as
   * late as they can reasonably go in the dominance tree.  The instructions
   * are "scheduled" by updating their instr->block field.
   *
   * The name of this function is actually a bit of a misnomer as it doesn't
   * schedule them "as late as possible" as the paper implies.  Instead, it
   * first finds the lates possible place it can schedule the instruction and
   * then possibly schedules it earlier than that.  The actual location is as
   * far down the tree as we can go while trying to stay out of loops.
   */
   static void
   gcm_schedule_late_instr(nir_instr *instr, struct gcm_state *state)
   {
      if (instr->pass_flags & GCM_INSTR_SCHEDULED_LATE)
                     /* Pinned/placed instructions are already scheduled so we don't need to do
   * anything.  Also, bailing here keeps us from ever following phi nodes
   * which can be back-edges.
   */
   if (instr->pass_flags & GCM_INSTR_PLACED ||
      instr->pass_flags & GCM_INSTR_PINNED)
            }
      static bool
   gcm_replace_def_with_undef(nir_def *def, void *void_state)
   {
               if (nir_def_is_unused(def))
            nir_undef_instr *undef =
      nir_undef_instr_create(state->impl->function->shader,
      nir_instr_insert(nir_before_impl(state->impl), &undef->instr);
               }
      /** Places an instrution back into the program
   *
   * The earlier passes of GCM simply choose blocks for each instruction and
   * otherwise leave them alone.  This pass actually places the instructions
   * into their chosen blocks.
   *
   * To do so, we simply insert instructions in the reverse order they were
   * extracted. This will simply place instructions that were scheduled earlier
   * onto the end of their new block and instructions that were scheduled later to
   * the start of their new block.
   */
   static void
   gcm_place_instr(nir_instr *instr, struct gcm_state *state)
   {
      if (instr->pass_flags & GCM_INSTR_PLACED)
                     if (instr->block == NULL) {
      nir_foreach_def(instr, gcm_replace_def_with_undef, state);
   nir_instr_remove(instr);
               struct gcm_block_info *block_info = &state->blocks[instr->block->index];
            if (block_info->last_instr) {
      exec_node_insert_node_before(&block_info->last_instr->node,
      } else {
      /* Schedule it at the end of the block */
   nir_instr *jump_instr = nir_block_last_instr(instr->block);
   if (jump_instr && jump_instr->type == nir_instr_type_jump) {
         } else {
                        }
      /**
   * Are instructions a and b both contained in the same if/else block?
   */
   static bool
   weak_gvn(const nir_instr *a, const nir_instr *b)
   {
      const struct nir_cf_node *ap = a->block->cf_node.parent;
   const struct nir_cf_node *bp = b->block->cf_node.parent;
      }
      static bool
   opt_gcm_impl(nir_shader *shader, nir_function_impl *impl, bool value_number)
   {
      nir_metadata_require(impl, nir_metadata_block_index |
         nir_metadata_require(impl, nir_metadata_loop_analysis,
                  /* A previous pass may have left pass_flags dirty, so clear it all out. */
   nir_foreach_block(block, impl)
      nir_foreach_instr(instr, block)
                  state.impl = impl;
   state.instr = NULL;
   state.progress = false;
   exec_list_make_empty(&state.instrs);
                              state.instr_infos =
            /* Perform (at least some) Global Value Numbering (GVN).
   *
   * We perform full GVN when `value_number' is true.  This can be too
   * aggressive, moving values far away and extending their live ranges,
   * so we don't always want to do it.
   *
   * Otherwise, we perform 'weaker' GVN: if identical ALU instructions appear
   * on both sides of the same if/else block, we allow them to be moved.
   * This cleans up a lot of mess without being -too- aggressive.
   */
   struct set *gvn_set = nir_instr_set_create(NULL);
   foreach_list_typed_safe(nir_instr, instr, node, &state.instrs) {
      if (instr->pass_flags & GCM_INSTR_PINNED)
            if (nir_instr_set_add_or_rewrite(gvn_set, instr,
            }
            foreach_list_typed(nir_instr, instr, node, &state.instrs)
            foreach_list_typed(nir_instr, instr, node, &state.instrs)
            while (!exec_list_is_empty(&state.instrs)) {
      nir_instr *instr = exec_node_data(nir_instr,
                     ralloc_free(state.blocks);
            nir_metadata_preserve(impl, nir_metadata_block_index |
                     }
      bool
   nir_opt_gcm(nir_shader *shader, bool value_number)
   {
               nir_foreach_function_impl(impl, shader) {
                     }
