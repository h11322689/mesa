   /*
   * Copyright (c) 2017 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/ralloc.h"
   #include "util/register_allocate.h"
   #include "util/u_debug.h"
      #include "ppir.h"
   #include "lima_context.h"
      #define PPIR_REG_COUNT  (6 * 4)
      enum ppir_ra_reg_class {
      ppir_ra_reg_class_vec1,
   ppir_ra_reg_class_vec2,
   ppir_ra_reg_class_vec3,
            /* 4 reg class for load/store instr regs:
   * load/store instr has no swizzle field, so the (virtual) register
   * must be allocated at the beginning of a (physical) register,
   */
   ppir_ra_reg_class_head_vec1,
   ppir_ra_reg_class_head_vec2,
   ppir_ra_reg_class_head_vec3,
               };
      struct ra_regs *ppir_regalloc_init(void *mem_ctx)
   {
      struct ra_regs *ret = ra_alloc_reg_set(mem_ctx, PPIR_REG_COUNT, false);
   if (!ret)
            /* Classes for contiguous 1-4 channel groups anywhere within a register. */
   struct ra_class *classes[ppir_ra_reg_class_num];
   for (int i = 0; i < ppir_ra_reg_class_head_vec1; i++) {
               for (int j = 0; j < PPIR_REG_COUNT; j += 4) {
      for (int swiz = 0; swiz < (4 - i); swiz++)
                  /* Classes for contiguous 1-4 channels with a start channel of .x */
   for (int i = ppir_ra_reg_class_head_vec1; i < ppir_ra_reg_class_num; i++) {
               for (int j = 0; j < PPIR_REG_COUNT; j += 4)
               ra_set_finalize(ret, NULL);
      }
      static void ppir_regalloc_update_reglist_ssa(ppir_compiler *comp)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_node, node, &block->node_list, list) {
                     ppir_dest *dest = ppir_node_get_dest(node);
                     if (dest->type == ppir_target_ssa) {
      reg = &dest->ssa;
   if (node->is_out)
         list_addtail(&reg->list, &comp->reg_list);
                  }
      static void ppir_regalloc_print_result(ppir_compiler *comp)
   {
      printf("======ppir regalloc result======\n");
   list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_instr, instr, &block->instr_list, list) {
      printf("%03d:", instr->index);
   for (int i = 0; i < PPIR_INSTR_SLOT_NUM; i++) {
      ppir_node *node = instr->slots[i];
                           ppir_dest *dest = ppir_node_get_dest(node);
                           for (int i = 0; i < ppir_node_get_src_num(node); i++) {
      if (i)
                        }
         }
            printf("======ppir output regs======\n");
   for (int i = 0; i < ppir_output_num; i++) {
      if (comp->out_type_to_reg[i] != -1)
      printf("%s: $%d\n", ppir_output_type_to_str(i),
   }
      }
      static bool create_new_instr_after(ppir_block *block, ppir_instr *ref,
         {
      ppir_instr *newinstr = ppir_instr_create(block);
   if (unlikely(!newinstr))
            list_del(&newinstr->list);
            if (!ppir_instr_insert_node(newinstr, node))
            list_for_each_entry_from(ppir_instr, instr, ref, &block->instr_list, list) {
         }
   newinstr->seq = ref->seq+1;
   newinstr->scheduled = true;
      }
      static bool create_new_instr_before(ppir_block *block, ppir_instr *ref,
         {
      ppir_instr *newinstr = ppir_instr_create(block);
   if (unlikely(!newinstr))
            list_del(&newinstr->list);
            if (!ppir_instr_insert_node(newinstr, node))
            list_for_each_entry_from(ppir_instr, instr, ref, &block->instr_list, list) {
         }
   newinstr->seq = ref->seq-1;
   newinstr->scheduled = true;
      }
      static bool ppir_update_spilled_src(ppir_compiler *comp, ppir_block *block,
               {
      /* nodes might have multiple references to the same value.
   * avoid creating unnecessary loads for the same fill by
   * saving the node resulting from the temporary load */
   if (*fill_node)
                     /* alloc new node to load value */
   ppir_node *load_node = ppir_node_create(block, ppir_op_load_temp, -1, 0);
   if (!load_node)
         list_addtail(&load_node->list, &node->list);
                     load->index = -comp->prog->state.stack_size; /* index sizes are negative */
            ppir_dest *ld_dest = &load->dest;
   ld_dest->type = ppir_target_pipeline;
   ld_dest->pipeline = ppir_pipeline_reg_uniform;
            /* If the uniform slot is empty, we can insert the load_temp
   * there and use it directly. Exceptionally, if the node is in the
   * varying or texld slot, this doesn't work. */
   if (!node->instr->slots[PPIR_INSTR_SLOT_UNIFORM] &&
      node->instr_pos != PPIR_INSTR_SLOT_VARYING &&
   node->instr_pos != PPIR_INSTR_SLOT_TEXLD) {
   ppir_node_target_assign(src, load_node);
   *fill_node = load_node;
               /* Uniform slot was taken, so fall back to a new instruction with a mov */
   if (!create_new_instr_before(block, node->instr, load_node))
            /* Create move node */
   ppir_node *move_node = ppir_node_create(block, ppir_op_mov, -1 , 0);
   if (unlikely(!move_node))
                           move_alu->num_src = 1;
   move_alu->src->type = ppir_target_pipeline;
   move_alu->src->pipeline = ppir_pipeline_reg_uniform;
   for (int i = 0; i < 4; i++)
            ppir_dest *alu_dest = &move_alu->dest;
   alu_dest->type = ppir_target_ssa;
   alu_dest->ssa.num_components = num_components;
   alu_dest->ssa.spilled = true;
            list_addtail(&alu_dest->ssa.list, &comp->reg_list);
            if (!ppir_instr_insert_node(load_node->instr, move_node))
            /* insert the new node as predecessor */
   ppir_node_foreach_pred_safe(node, dep) {
      ppir_node *pred = dep->pred;
   ppir_node_remove_dep(dep);
      }
   ppir_node_add_dep(node, move_node, ppir_dep_src);
                  update_src:
      /* switch node src to use the fill node dest */
               }
      static bool ppir_update_spilled_dest_load(ppir_compiler *comp, ppir_block *block,
         {
      ppir_dest *dest = ppir_node_get_dest(node);
   assert(dest != NULL);
   assert(dest->type == ppir_target_register);
   ppir_reg *reg = dest->reg;
            /* alloc new node to load value */
   ppir_node *load_node = ppir_node_create(block, ppir_op_load_temp, -1, 0);
   if (!load_node)
         list_addtail(&load_node->list, &node->list);
                     load->index = -comp->prog->state.stack_size; /* index sizes are negative */
            load->dest.type = ppir_target_pipeline;
   load->dest.pipeline = ppir_pipeline_reg_uniform;
            /* New instruction is needed since we're updating a dest register
   * and we can't write to the uniform pipeline reg */
   if (!create_new_instr_before(block, node->instr, load_node))
            /* Create move node */
   ppir_node *move_node = ppir_node_create(block, ppir_op_mov, -1 , 0);
   if (unlikely(!move_node))
                           move_alu->num_src = 1;
   move_alu->src->type = ppir_target_pipeline;
   move_alu->src->pipeline = ppir_pipeline_reg_uniform;
   for (int i = 0; i < 4; i++)
            move_alu->dest.type = ppir_target_register;
   move_alu->dest.reg = reg;
            if (!ppir_instr_insert_node(load_node->instr, move_node))
            ppir_node_foreach_pred_safe(node, dep) {
      ppir_node *pred = dep->pred;
   ppir_node_remove_dep(dep);
      }
   ppir_node_add_dep(node, move_node, ppir_dep_src);
               }
      static bool ppir_update_spilled_dest(ppir_compiler *comp, ppir_block *block,
         {
      ppir_dest *dest = ppir_node_get_dest(node);
   assert(dest != NULL);
            /* alloc new node to store value */
   ppir_node *store_node = ppir_node_create(block, ppir_op_store_temp, -1, 0);
   if (!store_node)
         list_addtail(&store_node->list, &node->list);
                              ppir_node_target_assign(&store->src, node);
            /* insert the new node as successor */
   ppir_node_foreach_succ_safe(node, dep) {
      ppir_node *succ = dep->succ;
   ppir_node_remove_dep(dep);
      }
            /* If the store temp slot is empty, we can insert the store_temp
   * there and use it directly. Exceptionally, if the node is in the
   * combine slot, this doesn't work. */
   if (!node->instr->slots[PPIR_INSTR_SLOT_STORE_TEMP] &&
      node->instr_pos != PPIR_INSTR_SLOT_ALU_COMBINE)
         /* Not possible to merge store, so fall back to a new instruction */
      }
      static bool ppir_regalloc_spill_reg(ppir_compiler *comp, ppir_reg *chosen)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
                  ppir_dest *dest = ppir_node_get_dest(node);
   if (dest && ppir_dest_get_reg(dest) == chosen) {
      /* If dest is a register, it might be updating only some its
   * components, so need to load the existing value first */
   if (dest->type == ppir_target_register) {
      if (!ppir_update_spilled_dest_load(comp, block, node))
      }
   if (!ppir_update_spilled_dest(comp, block, node))
               ppir_node *fill_node = NULL;
   /* nodes might have multiple references to the same value.
   * avoid creating unnecessary loads for the same fill by
   * saving the node resulting from the temporary load */
   for (int i = 0; i < ppir_node_get_src_num(node); i++) {
      ppir_src *src = ppir_node_get_src(node, i);
   ppir_reg *reg = ppir_src_get_reg(src);
   if (reg == chosen) {
      if (!ppir_update_spilled_src(comp, block, node, src, &fill_node))
                           }
      static ppir_reg *ppir_regalloc_choose_spill_node(ppir_compiler *comp,
         {
      float spill_costs[comp->reg_num];
   /* experimentally determined, it seems to be worth scaling cost of
   * regs in instructions that have used uniform/store_temp slots,
   * but not too much as to offset the num_components base cost. */
            memset(spill_costs, 0, sizeof(spill_costs[0]) * comp->reg_num);
   list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
      if (reg->spilled) {
      /* not considered for spilling */
   spill_costs[reg->regalloc_index] = 0.0f;
               /* It is beneficial to spill registers with higher component number,
   * so increase the cost of spilling registers with few components */
   float spill_cost = 4.0f / (float)reg->num_components;
               list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_instr, instr, &block->instr_list, list) {
      if (instr->slots[PPIR_INSTR_SLOT_UNIFORM]) {
      for (int i = 0; i < PPIR_INSTR_SLOT_NUM; i++) {
      ppir_node *node = instr->slots[i];
   if (!node)
         for (int j = 0; j < ppir_node_get_src_num(node); j++) {
      ppir_src *src = ppir_node_get_src(node, j);
   if (!src)
                                       }
   if (instr->slots[PPIR_INSTR_SLOT_STORE_TEMP]) {
      for (int i = 0; i < PPIR_INSTR_SLOT_NUM; i++) {
      ppir_node *node = instr->slots[i];
   if (!node)
         ppir_dest *dest = ppir_node_get_dest(node);
   if (!dest)
                                                   for (int i = 0; i < comp->reg_num; i++)
            int r = ra_get_best_spill_node(g);
   if (r == -1)
            ppir_reg *chosen = NULL;
   int i = 0;
   list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
      if (i++ == r) {
      chosen = reg;
         }
   assert(chosen);
   chosen->spilled = true;
               }
      static void ppir_regalloc_reset_liveness_info(ppir_compiler *comp)
   {
               list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
                  list_for_each_entry(ppir_block, block, &comp->block_list, list) {
                  if (instr->live_mask)
                        if (instr->live_set)
                  if (instr->live_internal)
                  }
      static void ppir_all_interference(ppir_compiler *comp, struct ra_graph *g,
         {
      int i, j;
   BITSET_FOREACH_SET(i, liveness, comp->reg_num) {
      BITSET_FOREACH_SET(j, liveness, comp->reg_num) {
         }
         }
      int lima_ppir_force_spilling = 0;
      static bool ppir_regalloc_prog_try(ppir_compiler *comp, bool *spilled)
   {
               struct ra_graph *g = ra_alloc_interference_graph(
            int n = 0;
   list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
      int c = ppir_ra_reg_class_vec1 + (reg->num_components - 1);
   if (reg->is_head)
                              list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_instr, instr, &block->instr_list, list) {
      int i;
   BITSET_FOREACH_SET(i, instr->live_internal, comp->reg_num) {
         }
                  *spilled = false;
   bool ok = ra_allocate(g);
   if (!ok || (comp->force_spilling-- > 0)) {
      ppir_reg *chosen = ppir_regalloc_choose_spill_node(comp, g);
   if (chosen) {
      /* stack_size will be used to assemble the frame reg in lima_draw.
   * It is also be used in the spilling code, as negative indices
   * starting from -1, to create stack addresses. */
   comp->prog->state.stack_size++;
   if (!ppir_regalloc_spill_reg(comp, chosen))
                        ppir_debug("spilled register %d/%d, num_components: %d\n",
                           ppir_error("regalloc fail\n");
               n = 0;
   list_for_each_entry(ppir_reg, reg, &comp->reg_list, list) {
      reg->index = ra_get_node_reg(g, n++);
   if (reg->out_reg) {
      /* We need actual reg number, we don't have swizzle for output regs */
   assert(!(reg->index & 0x3) && "ppir: output regs don't have swizzle");
                           if (lima_debug & LIMA_DEBUG_PP)
                  err_out:
      ralloc_free(g);
      }
      bool ppir_regalloc_prog(ppir_compiler *comp)
   {
      bool spilled = false;
            /* Set from an environment variable to force spilling
   * for debugging purposes, see lima_screen.c */
                     /* No registers? Probably shader consists of discard instruction */
   if (list_is_empty(&comp->reg_list)) {
      comp->prog->state.frag_color0_reg = 0;
   comp->prog->state.frag_color1_reg = -1;
   comp->prog->state.frag_depth_reg = -1;
               /* this will most likely succeed in the first
   * try, except for very complicated shaders */
   while (!ppir_regalloc_prog_try(comp, &spilled))
      if (!spilled)
         comp->prog->state.frag_color0_reg =
         comp->prog->state.frag_color1_reg =
         comp->prog->state.frag_depth_reg =
               }
