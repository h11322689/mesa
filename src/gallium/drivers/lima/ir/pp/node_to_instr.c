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
      #include "ppir.h"
         static bool create_new_instr(ppir_block *block, ppir_node *node)
   {
      ppir_instr *instr = ppir_instr_create(block);
   if (unlikely(!instr))
            if (!ppir_instr_insert_node(instr, node))
               }
      /*
   * If a node has a pipeline dest, schedule it in the same instruction as its
   * successor.
   * Since it has a pipeline dest, it must have only one successor and since we
   * schedule nodes backwards, its successor must have already been scheduled.
   * Load varyings can't output to a pipeline register but are also potentially
   * trivial to insert and save an instruction if they have a single successor.
   */
   static bool ppir_do_node_to_instr_try_insert(ppir_block *block, ppir_node *node)
   {
               if (dest && dest->type == ppir_target_pipeline) {
      assert(ppir_node_has_single_src_succ(node));
   ppir_node *succ = ppir_node_first_succ(node);
   assert(succ);
                        if (ppir_node_has_single_succ(node) &&
      ppir_node_has_single_pred(ppir_node_first_succ(node)) &&
            assert(ppir_node_has_single_succ(node));
   ppir_node *succ = ppir_node_first_succ(node);
   assert(succ);
                        switch (node->type) {
      case ppir_node_type_load:
         default:
               if (!ppir_node_has_single_src_succ(node))
            ppir_node *succ = ppir_node_first_succ(node);
   assert(succ);
               }
      static bool ppir_do_one_node_to_instr(ppir_block *block, ppir_node *node)
   {
      switch (node->type) {
   case ppir_node_type_alu:
   {
      /* don't create an instr for undef node */
   if (node->op == ppir_op_undef)
            /* merge pred mul and succ add in the same instr can save a reg
   * by using pipeline reg ^vmul/^fmul */
   ppir_alu_node *alu = ppir_node_to_alu(node);
   if (alu->dest.type == ppir_target_ssa &&
      ppir_node_has_single_succ(node) &&
   ppir_node_has_single_src_succ(node)) {
   ppir_node *succ = ppir_node_first_succ(node);
   if (succ->instr_pos == PPIR_INSTR_SLOT_ALU_VEC_ADD) {
      node->instr_pos = PPIR_INSTR_SLOT_ALU_VEC_MUL;
      }
   else if (succ->instr_pos == PPIR_INSTR_SLOT_ALU_SCL_ADD &&
            node->instr_pos = PPIR_INSTR_SLOT_ALU_SCL_MUL;
                  /* can't inserted to any existing instr, create one */
   if (!node->instr && !create_new_instr(block, node))
               }
   case ppir_node_type_load:
   case ppir_node_type_load_texture:
   {
      if (!create_new_instr(block, node))
            /* load varying output can be a register, it doesn't need a mov */
   switch (node->op) {
   case ppir_op_load_varying:
   case ppir_op_load_coords:
   case ppir_op_load_coords_reg:
   case ppir_op_load_fragcoord:
   case ppir_op_load_pointcoord:
   case ppir_op_load_frontface:
         default:
                  /* Load cannot be pipelined, likely slot is already taken. Create a mov */
   assert(ppir_node_has_single_src_succ(node));
   ppir_dest *dest = ppir_node_get_dest(node);
   assert(dest->type == ppir_target_pipeline);
            /* Turn dest back to SSA, so we can update predecessors */
            /* Single succ can still have multiple references to this node */
   for (int i = 0; i < ppir_node_get_src_num(succ); i++) {
      ppir_src *src = ppir_node_get_src(succ, i);
   if (src && src->node == node) {
      /* Can consume uniforms directly */
   dest->type = ppir_target_ssa;
   dest->ssa.index = -1;
                  ppir_node *move = ppir_node_insert_mov(node);
   if (unlikely(!move))
            ppir_src *mov_src = ppir_node_get_src(move, 0);
   mov_src->type = dest->type = ppir_target_pipeline;
            ppir_debug("node_to_instr create move %d for load %d\n",
            if (!ppir_instr_insert_node(node->instr, move))
               }
   case ppir_node_type_const: {
      /* Const cannot be pipelined, too many consts in the instruction.
            ppir_node *move = ppir_node_insert_mov(node);
   if (!create_new_instr(block, move))
            ppir_debug("node_to_instr create move %d for const %d\n",
            ppir_dest *dest = ppir_node_get_dest(node);
            /* update succ from ^const to ssa mov output */
   ppir_dest *move_dest = ppir_node_get_dest(move);
   move_dest->type = ppir_target_ssa;
   ppir_node *succ = ppir_node_first_succ(move);
            mov_src->type = dest->type = ppir_target_pipeline;
            if (!ppir_instr_insert_node(move->instr, node))
               }
   case ppir_node_type_store:
   {
      if (node->op == ppir_op_store_temp) {
      if (!create_new_instr(block, node))
            }
      }
   case ppir_node_type_discard:
      if (!create_new_instr(block, node))
         block->stop = true;
      case ppir_node_type_branch:
      if (!create_new_instr(block, node))
            default:
                     }
      static unsigned int ppir_node_score(ppir_node *node)
   {
      /* preferentially expand nodes in later instruction slots first, so
   * nodes for earlier slots (which are more likely pipelineable) get
   * added to the ready list. */
   unsigned int late_slot = 0;
   int *slots = ppir_op_infos[node->op].slots;
   if (slots)
      for (int i = 0; slots[i] != PPIR_INSTR_SLOT_END; i++)
         /* to untie, favour nodes with pipelines for earlier expansion.
   * increase that for nodes with chained pipelines */
   unsigned int pipeline = 0;
   ppir_node *n = node;
   ppir_dest *dest = ppir_node_get_dest(n);
   while (dest && dest->type == ppir_target_pipeline) {
      pipeline++;
   assert(ppir_node_has_single_src_succ(n));
   n = ppir_node_first_succ(n);
      }
               }
      static ppir_node *ppir_ready_list_pick_best(ppir_block *block,
         {
      unsigned int best_score = 0;
            list_for_each_entry(ppir_node, node, ready_list, sched_list) {
      unsigned int score = ppir_node_score(node);
   if (!best || score > best_score) {
      best = node;
                  assert(best);
      }
      static bool ppir_do_node_to_instr(ppir_block *block, ppir_node *root)
   {
      struct list_head ready_list;
   list_inithead(&ready_list);
            while (!list_is_empty(&ready_list)) {
      ppir_node *node = ppir_ready_list_pick_best(block, &ready_list);
            /* first try pipeline sched, if that didn't succeed try normal sched */
   if (!ppir_do_node_to_instr_try_insert(block, node))
                  /* The node writes output register. We can't stop at this exact
   * instruction because there may be another node that writes another
   * output, so set stop flag for the block. We will set stop flag on
   * the last instruction of the block during codegen
   */
   if (node->is_out)
            ppir_node_foreach_pred(node, dep) {
                     /* pred may already have been processed by a previous node */
                  /* insert pred only when all its successors have been inserted */
   ppir_node_foreach_succ(pred, dep) {
      ppir_node *succ = dep->succ;
   if (!succ->instr) {
      ready = false;
                  if (ready)
                     }
      static bool ppir_create_instr_from_node(ppir_compiler *comp)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_node, node, &block->node_list, list) {
      if (ppir_node_is_root(node)) {
      if (!ppir_do_node_to_instr(block, node))
                        }
      static void ppir_build_instr_dependency(ppir_compiler *comp)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
      list_for_each_entry(ppir_instr, instr, &block->instr_list, list) {
      for (int i = 0; i < PPIR_INSTR_SLOT_NUM; i++) {
      ppir_node *node = instr->slots[i];
   if (node) {
      ppir_node_foreach_pred(node, dep) {
      ppir_node *pred = dep->pred;
   if (pred->instr && pred->instr != instr)
                     }
      bool ppir_node_to_instr(ppir_compiler *comp)
   {
      if (!ppir_create_instr_from_node(comp))
                  ppir_build_instr_dependency(comp);
               }
