   /*
   * Copyright © 2021 Advanced Micro Devices, Inc.
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
      /* This is a new block-level load instruction scheduler where loads are grouped
   * according to their indirection level within a basic block. An indirection
   * is when a result of one load is used as a source of another load. The result
   * is that disjoint ALU opcode groups and load (texture) opcode groups are
   * created where each next load group is the next level of indirection.
   * It's done by finding the first and last load with the same indirection
   * level, and moving all unrelated instructions between them after the last
   * load except for load sources, which are moved before the first load.
   * It naturally suits hardware that has limits on texture indirections, but
   * other hardware can benefit too. Only texture, image, and SSBO load and
   * atomic instructions are grouped.
   *
   * There is an option to group only those loads that use the same resource
   * variable. This increases the chance to get more cache hits than if the loads
   * were spread out.
   *
   * The increased register usage is offset by the increase in observed memory
   * bandwidth due to more cache hits (dependent on hw behavior) and thus
   * decrease the subgroup lifetime, which allows registers to be deallocated
   * and reused sooner. In some bandwidth-bound cases, low register usage doesn't
   * benefit at all. Doubling the register usage and using those registers to
   * amplify observed bandwidth can improve performance a lot.
   *
   * It's recommended to run a hw-specific instruction scheduler after this to
   * prevent spilling.
   */
      #include "nir.h"
      static bool
   is_memory_load(nir_instr *instr)
   {
      /* Count texture_size too because it has the same latency as cache hits. */
   if (instr->type == nir_instr_type_tex)
            if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            /* TODO: nir_intrinsics.py could do this */
   /* load_ubo is ignored because it's usually cheap. */
   if (!nir_intrinsic_writes_external_memory(intr) &&
      !strstr(name, "shared") &&
   (strstr(name, "ssbo") || strstr(name, "image")))
               }
      static nir_instr *
   get_intrinsic_resource(nir_intrinsic_instr *intr)
   {
      /* This is also the list of intrinsics that are grouped. */
   /* load_ubo is ignored because it's usually cheap. */
   switch (intr->intrinsic) {
   case nir_intrinsic_image_load:
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_sparse_load:
   case nir_intrinsic_image_deref_sparse_load:
   /* Group image_size too because it has the same latency as cache hits. */
   case nir_intrinsic_image_samples_identical:
   case nir_intrinsic_image_deref_samples_identical:
   case nir_intrinsic_bindless_image_samples_identical:
   case nir_intrinsic_image_size:
   case nir_intrinsic_image_deref_size:
   case nir_intrinsic_bindless_image_load:
   case nir_intrinsic_bindless_image_sparse_load:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_image_fragment_mask_load_amd:
   case nir_intrinsic_image_deref_fragment_mask_load_amd:
   case nir_intrinsic_bindless_image_fragment_mask_load_amd:
         default:
            }
      /* Track only those that we want to group. */
   static bool
   is_grouped_load(nir_instr *instr)
   {
      /* Count texture_size too because it has the same latency as cache hits. */
   if (instr->type == nir_instr_type_tex)
            if (instr->type == nir_instr_type_intrinsic)
               }
      static bool
   can_move(nir_instr *instr, uint8_t current_indirection_level)
   {
      /* Grouping is done by moving everything else out of the first/last
   * instruction range of the indirection level.
   */
   if (is_grouped_load(instr) && instr->pass_flags == current_indirection_level)
            if (instr->type == nir_instr_type_alu ||
      instr->type == nir_instr_type_deref ||
   instr->type == nir_instr_type_tex ||
   instr->type == nir_instr_type_load_const ||
   instr->type == nir_instr_type_undef)
         if (instr->type == nir_instr_type_intrinsic &&
      nir_intrinsic_can_reorder(nir_instr_as_intrinsic(instr)))
            }
      static nir_instr *
   get_uniform_inst_resource(nir_instr *instr)
   {
      if (instr->type == nir_instr_type_tex) {
               if (tex->texture_non_uniform)
            for (unsigned i = 0; i < tex->num_srcs; i++) {
      switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
   case nir_tex_src_texture_handle:
         default:
            }
               if (instr->type == nir_instr_type_intrinsic)
               }
      struct check_sources_state {
      nir_block *block;
      };
      static bool
   has_only_sources_less_than(nir_src *src, void *data)
   {
               /* true if nir_foreach_src should keep going */
   return state->block != src->ssa->parent_instr->block ||
      }
      static void
   group_loads(nir_instr *first, nir_instr *last)
   {
      /* Walk the instruction range between the first and last backward, and
   * move those that have no uses within the range after the last one.
   */
   for (nir_instr *instr = exec_node_data_backward(nir_instr,
            instr != first;
   instr = exec_node_data_backward(nir_instr, instr->node.prev, node)) {
   /* Only move instructions without side effects. */
   if (!can_move(instr, first->pass_flags))
            nir_def *def = nir_instr_def(instr);
   if (def) {
               nir_foreach_use(use, def) {
      if (nir_src_parent_instr(use)->block == instr->block &&
      nir_src_parent_instr(use)->index <= last->index) {
   all_uses_after_last = false;
                  if (all_uses_after_last) {
      nir_instr *move_instr = instr;
                  /* Move the instruction after the last and update its index
   * to indicate that it's after it.
   */
   nir_instr_move(nir_after_instr(last), move_instr);
                     struct check_sources_state state;
   state.block = first->block;
            /* Walk the instruction range between the first and last forward, and move
   * those that have no sources within the range before the first one.
   */
   for (nir_instr *instr = exec_node_data_forward(nir_instr,
            instr != last;
   instr = exec_node_data_forward(nir_instr, instr->node.next, node)) {
   /* Only move instructions without side effects. */
   if (!can_move(instr, first->pass_flags))
            if (nir_foreach_src(instr, has_only_sources_less_than, &state)) {
      nir_instr *move_instr = instr;
                  /* Move the instruction before the first and update its index
   * to indicate that it's before it.
   */
   nir_instr_move(nir_before_instr(first), move_instr);
            }
      static bool
   is_pseudo_inst(nir_instr *instr)
   {
      /* Other instructions do not usually contribute to the shader binary size. */
   return instr->type != nir_instr_type_alu &&
         instr->type != nir_instr_type_call &&
      }
      static void
   set_instr_indices(nir_block *block)
   {
      /* Start with 1 because we'll move instruction before the first one
   * and will want to label it 0.
   */
   unsigned counter = 1;
            nir_foreach_instr(instr, block) {
      /* Make sure grouped instructions don't have the same index as pseudo
   * instructions.
   */
   if (last && is_pseudo_inst(last) && is_grouped_load(instr))
            /* Set each instruction's index within the block. */
            /* Only count non-pseudo instructions. */
   if (!is_pseudo_inst(instr))
                  }
      static void
   handle_load_range(nir_instr **first, nir_instr **last,
         {
      if (*first && *last &&
      (!current || current->index > (*first)->index + max_distance)) {
   assert(*first != *last);
   group_loads(*first, *last);
   set_instr_indices((*first)->block);
   *first = NULL;
         }
      static bool
   is_barrier(nir_instr *instr)
   {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
            if (intr->intrinsic == nir_intrinsic_discard ||
      intr->intrinsic == nir_intrinsic_discard_if ||
   intr->intrinsic == nir_intrinsic_terminate ||
   intr->intrinsic == nir_intrinsic_terminate_if ||
   /* TODO: nir_intrinsics.py could do this */
   strstr(name, "barrier"))
               }
      struct indirection_state {
      nir_block *block;
      };
      static unsigned
   get_num_indirections(nir_instr *instr);
      static bool
   gather_indirections(nir_src *src, void *data)
   {
      struct indirection_state *state = (struct indirection_state *)data;
            /* We only count indirections within the same block. */
   if (instr->block == state->block) {
               if (instr->type == nir_instr_type_tex || is_memory_load(instr))
                           }
      /* Return the number of load indirections within the block. */
   static unsigned
   get_num_indirections(nir_instr *instr)
   {
      /* Don't traverse phis because we could end up in an infinite recursion
   * if the phi points to the current block (such as a loop body).
   */
   if (instr->type == nir_instr_type_phi)
            if (instr->index != UINT32_MAX)
            struct indirection_state state;
   state.block = instr->block;
                     instr->index = state.indirections;
      }
      static void
   process_block(nir_block *block, nir_load_grouping grouping,
         {
      int max_indirection = -1;
            /* UINT32_MAX means the instruction has not been visited. Once
   * an instruction has been visited and its indirection level has been
   * determined, we'll store the indirection level in the index. The next
   * instruction that visits it will use the index instead of recomputing
   * the indirection level, which would result in an exponetial time
   * complexity.
   */
   nir_foreach_instr(instr, block) {
                  /* Count the number of load indirections for each load instruction
   * within this block. Store it in pass_flags.
   */
   nir_foreach_instr(instr, block) {
      if (is_grouped_load(instr)) {
               /* pass_flags has only 8 bits */
   indirections = MIN2(indirections, 255);
                                 /* 255 contains all indirection levels >= 255, so ignore them. */
            /* Each indirection level is grouped. */
   for (int level = 0; level <= max_indirection; level++) {
      if (num_inst_per_level[level] <= 1)
                     nir_instr *resource = NULL;
            /* Find the first and last instruction that use the same
   * resource and are within a certain distance of each other.
   * If found, group them by moving all movable instructions
   * between them out.
   */
   nir_foreach_instr(current, block) {
      /* Don't group across barriers. */
   if (is_barrier(current)) {
      /* Group unconditionally.  */
   handle_load_range(&first_load, &last_load, NULL, 0);
   first_load = NULL;
   last_load = NULL;
               /* Only group load instructions with the same indirection level. */
                     switch (grouping) {
   case nir_group_all:
      if (!first_load)
                                             if (current_resource) {
      if (!first_load) {
      first_load = current;
      } else if (current_resource == resource) {
                           /* Group only if we exceeded the maximum distance. */
               /* Group unconditionally.  */
         }
      /* max_distance is the maximum distance between the first and last instruction
   * in a group.
   */
   void
   nir_group_loads(nir_shader *shader, nir_load_grouping grouping,
         {
      nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
                  nir_metadata_preserve(impl, nir_metadata_block_index |
               }
