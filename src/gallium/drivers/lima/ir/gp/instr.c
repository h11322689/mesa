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
      #include <string.h>
      #include "util/ralloc.h"
      #include "gpir.h"
      gpir_instr *gpir_instr_create(gpir_block *block)
   {
      gpir_instr *instr = rzalloc(block, gpir_instr);
   if (unlikely(!instr))
            block->comp->num_instr++;
   if (block->comp->num_instr > 512) {
      gpir_error("shader exceeds limit of 512 instructions\n");
               instr->index = block->sched.instr_index++;
   instr->alu_num_slot_free = 6;
   instr->alu_non_cplx_slot_free = 5;
            list_add(&instr->list, &block->instr_list);
      }
      static gpir_node *gpir_instr_get_the_other_acc_node(gpir_instr *instr, int slot)
   {
      if (slot == GPIR_INSTR_SLOT_ADD0)
         else if (slot == GPIR_INSTR_SLOT_ADD1)
               }
      static bool gpir_instr_check_acc_same_op(gpir_instr *instr, gpir_node *node, int slot)
   {
      /* two ACC slots must share the same op code */
            /* spill move case may get acc_node == node */
   if (acc_node && acc_node != node &&
      !gpir_codegen_acc_same_op(node->op, acc_node->op))
            }
      static int gpir_instr_get_consume_slot(gpir_instr *instr, gpir_node *node)
   {
      if (gpir_op_infos[node->op].may_consume_two_slots) {
      gpir_node *acc_node = gpir_instr_get_the_other_acc_node(instr, node->sched.pos);
   if (acc_node)
      /* at this point node must have the same acc op with acc_node,
   * so it just consumes the extra slot acc_node consumed */
      else
      }
   else
      }
      static bool gpir_instr_insert_alu_check(gpir_instr *instr, gpir_node *node)
   {
      if (!gpir_instr_check_acc_same_op(instr, node, node->sched.pos))
            if (node->sched.next_max_node && !node->sched.complex_allowed &&
      node->sched.pos == GPIR_INSTR_SLOT_COMPLEX)
         int consume_slot = gpir_instr_get_consume_slot(instr, node);
   int non_cplx_consume_slot =
         int store_reduce_slot = 0;
   int non_cplx_store_reduce_slot = 0;
   int max_reduce_slot = node->sched.max_node ? 1 : 0;
   int next_max_reduce_slot = node->sched.next_max_node ? 1 : 0;
   int alu_new_max_allowed_next_max =
            /* check if this node is child of one store node.
   * complex1 won't be any of this instr's store node's child,
   * because it has two instr latency before store can use it.
   */
   for (int i = GPIR_INSTR_SLOT_STORE0; i <= GPIR_INSTR_SLOT_STORE3; i++) {
      gpir_store_node *s = gpir_node_to_store(instr->slots[i]);
   if (s && s->child == node) {
      store_reduce_slot = 1;
   if (node->sched.next_max_node && !node->sched.complex_allowed)
                        /* Check that the invariants will be maintained after we adjust everything
            int slot_difference = 
      instr->alu_num_slot_needed_by_store - store_reduce_slot +
   instr->alu_num_slot_needed_by_max - max_reduce_slot +
   MAX2(instr->alu_num_unscheduled_next_max - next_max_reduce_slot -
            if (slot_difference > 0) {
      gpir_debug("failed %d because of alu slot\n", node->index);
               int non_cplx_slot_difference =
      instr->alu_num_slot_needed_by_max - max_reduce_slot +
   instr->alu_num_slot_needed_by_non_cplx_store - non_cplx_store_reduce_slot -
      if (non_cplx_slot_difference > 0) {
      gpir_debug("failed %d because of alu slot\n", node->index);
               if (slot_difference > 0 || non_cplx_slot_difference > 0)
            instr->alu_num_slot_free -= consume_slot;
   instr->alu_non_cplx_slot_free -= non_cplx_consume_slot;
   instr->alu_num_slot_needed_by_store -= store_reduce_slot;
   instr->alu_num_slot_needed_by_non_cplx_store -= non_cplx_store_reduce_slot;
   instr->alu_num_slot_needed_by_max -= max_reduce_slot;
   instr->alu_num_unscheduled_next_max -= next_max_reduce_slot;
   instr->alu_max_allowed_next_max = alu_new_max_allowed_next_max;
      }
      static void gpir_instr_remove_alu(gpir_instr *instr, gpir_node *node)
   {
               for (int i = GPIR_INSTR_SLOT_STORE0; i <= GPIR_INSTR_SLOT_STORE3; i++) {
      gpir_store_node *s = gpir_node_to_store(instr->slots[i]);
   if (s && s->child == node) {
      instr->alu_num_slot_needed_by_store++;
   if (node->sched.next_max_node && !node->sched.complex_allowed)
                        instr->alu_num_slot_free += consume_slot;
   if (node->sched.pos != GPIR_INSTR_SLOT_COMPLEX)
         if (node->sched.max_node)
         if (node->sched.next_max_node)
         if (node->op == gpir_op_complex1)
      }
      static bool gpir_instr_insert_reg0_check(gpir_instr *instr, gpir_node *node)
   {
      gpir_load_node *load = gpir_node_to_load(node);
            if (load->component != i)
            if (instr->reg0_is_attr && node->op != gpir_op_load_attribute)
            if (instr->reg0_use_count) {
      if (instr->reg0_index != load->index)
      }
   else {
      instr->reg0_is_attr = node->op == gpir_op_load_attribute;
               instr->reg0_use_count++;
      }
      static void gpir_instr_remove_reg0(gpir_instr *instr, gpir_node *node)
   {
      instr->reg0_use_count--;
   if (!instr->reg0_use_count)
      }
      static bool gpir_instr_insert_reg1_check(gpir_instr *instr, gpir_node *node)
   {
      gpir_load_node *load = gpir_node_to_load(node);
            if (load->component != i)
            if (instr->reg1_use_count) {
      if (instr->reg1_index != load->index)
      }
   else
            instr->reg1_use_count++;
      }
      static void gpir_instr_remove_reg1(gpir_instr *instr, gpir_node *node)
   {
         }
      static bool gpir_instr_insert_mem_check(gpir_instr *instr, gpir_node *node)
   {
      gpir_load_node *load = gpir_node_to_load(node);
            if (load->component != i)
            if (instr->mem_is_temp && node->op != gpir_op_load_temp)
            if (instr->mem_use_count) {
      if (instr->mem_index != load->index)
      }
   else {
      instr->mem_is_temp = node->op == gpir_op_load_temp;
               instr->mem_use_count++;
      }
      static void gpir_instr_remove_mem(gpir_instr *instr, gpir_node *node)
   {
      instr->mem_use_count--;
   if (!instr->mem_use_count)
      }
      static bool gpir_instr_insert_store_check(gpir_instr *instr, gpir_node *node)
   {
      gpir_store_node *store = gpir_node_to_store(node);
            if (store->component != i)
            i >>= 1;
   switch (instr->store_content[i]) {
   case GPIR_INSTR_STORE_NONE:
      /* store temp has only one address reg for two store unit */
   if (node->op == gpir_op_store_temp &&
      instr->store_content[!i] == GPIR_INSTR_STORE_TEMP &&
   instr->store_index[!i] != store->index)
            case GPIR_INSTR_STORE_VARYING:
      if (node->op != gpir_op_store_varying ||
      instr->store_index[i] != store->index)
            case GPIR_INSTR_STORE_REG:
      if (node->op != gpir_op_store_reg ||
      instr->store_index[i] != store->index)
            case GPIR_INSTR_STORE_TEMP:
      if (node->op != gpir_op_store_temp ||
      instr->store_index[i] != store->index)
                  /* check if any store node has the same child as this node */
   for (int j = GPIR_INSTR_SLOT_STORE0; j <= GPIR_INSTR_SLOT_STORE3; j++) {
      gpir_store_node *s = gpir_node_to_store(instr->slots[j]);
   if (s && s->child == store->child)
               /* check if the child is already in this instr's alu slot,
   * this may happen when store an scheduled alu node to reg
   */
   for (int j = GPIR_INSTR_SLOT_ALU_BEGIN; j <= GPIR_INSTR_SLOT_ALU_END; j++) {
      if (store->child == instr->slots[j])
               /* Check the invariants documented in gpir.h, similar to the ALU case.
   * When the only thing that changes is alu_num_slot_needed_by_store, we
   * can get away with just checking the first one.
   */
   int slot_difference = instr->alu_num_slot_needed_by_store + 1
      + instr->alu_num_slot_needed_by_max +
   MAX2(instr->alu_num_unscheduled_next_max - instr->alu_max_allowed_next_max, 0) -
      if (slot_difference > 0) {
      instr->slot_difference = slot_difference;
               if (store->child->sched.next_max_node &&
      !store->child->sched.complex_allowed) {
   /* The child of the store is already partially ready, and has a use one
   * cycle ago that disqualifies it (or a move replacing it) from being
   * put in the complex slot. Therefore we have to check the non-complex
   * invariant.
   */
   int non_cplx_slot_difference =
      instr->alu_num_slot_needed_by_max +
   instr->alu_num_slot_needed_by_non_cplx_store + 1 -
      if (non_cplx_slot_difference > 0) {
      instr->non_cplx_slot_difference = non_cplx_slot_difference;
                                 out:
      if (instr->store_content[i] == GPIR_INSTR_STORE_NONE) {
      if (node->op == gpir_op_store_varying)
         else if (node->op == gpir_op_store_reg)
         else
               }
      }
      static void gpir_instr_remove_store(gpir_instr *instr, gpir_node *node)
   {
      gpir_store_node *store = gpir_node_to_store(node);
   int component = node->sched.pos - GPIR_INSTR_SLOT_STORE0;
            for (int j = GPIR_INSTR_SLOT_STORE0; j <= GPIR_INSTR_SLOT_STORE3; j++) {
      if (j == node->sched.pos)
            gpir_store_node *s = gpir_node_to_store(instr->slots[j]);
   if (s && s->child == store->child)
               for (int j = GPIR_INSTR_SLOT_ALU_BEGIN; j <= GPIR_INSTR_SLOT_ALU_END; j++) {
      if (store->child == instr->slots[j])
                        if (store->child->sched.next_max_node &&
      !store->child->sched.complex_allowed) {
            out:
      if (!instr->slots[other_slot])
      }
      static bool gpir_instr_spill_move(gpir_instr *instr, int slot, int spill_to_start)
   {
      gpir_node *node = instr->slots[slot];
   if (!node)
            if (node->op != gpir_op_mov)
            for (int i = spill_to_start; i <= GPIR_INSTR_SLOT_DIST_TWO_END; i++) {
      if (i != slot && !instr->slots[i] &&
      gpir_instr_check_acc_same_op(instr, node, i)) {
   instr->slots[i] = node;
                  gpir_debug("instr %d spill move %d from slot %d to %d\n",
                           }
      static bool gpir_instr_slot_free(gpir_instr *instr, gpir_node *node)
   {
      if (node->op == gpir_op_mov ||
      node->sched.pos > GPIR_INSTR_SLOT_DIST_TWO_END) {
   if (instr->slots[node->sched.pos])
      }
   else {
      /* for node needs dist two slot, if the slot has a move, we can
   * spill it to other dist two slot without any side effect */
   int spill_to_start = GPIR_INSTR_SLOT_MUL0;
   if (node->op == gpir_op_complex1 || node->op == gpir_op_select)
            if (!gpir_instr_spill_move(instr, node->sched.pos, spill_to_start))
            if (node->op == gpir_op_complex1 || node->op == gpir_op_select) {
      if (!gpir_instr_spill_move(instr, GPIR_INSTR_SLOT_MUL1, spill_to_start))
                     }
      bool gpir_instr_try_insert_node(gpir_instr *instr, gpir_node *node)
   {
      instr->slot_difference = 0;
            if (!gpir_instr_slot_free(instr, node))
            if (node->sched.pos >= GPIR_INSTR_SLOT_ALU_BEGIN &&
      node->sched.pos <= GPIR_INSTR_SLOT_ALU_END) {
   if (!gpir_instr_insert_alu_check(instr, node))
      }
   else if (node->sched.pos >= GPIR_INSTR_SLOT_REG0_LOAD0 &&
            if (!gpir_instr_insert_reg0_check(instr, node))
      }
   else if (node->sched.pos >= GPIR_INSTR_SLOT_REG1_LOAD0 &&
            if (!gpir_instr_insert_reg1_check(instr, node))
      }
   else if (node->sched.pos >= GPIR_INSTR_SLOT_MEM_LOAD0 &&
            if (!gpir_instr_insert_mem_check(instr, node))
      }
   else if (node->sched.pos >= GPIR_INSTR_SLOT_STORE0 &&
            if (!gpir_instr_insert_store_check(instr, node))
                        if (node->op == gpir_op_complex1 || node->op == gpir_op_select)
               }
      void gpir_instr_remove_node(gpir_instr *instr, gpir_node *node)
   {
               /* This can happen if we merge duplicate loads in the scheduler. */
   if (instr->slots[node->sched.pos] != node) {
      node->sched.pos = -1;
   node->sched.instr = NULL;
               if (node->sched.pos >= GPIR_INSTR_SLOT_ALU_BEGIN &&
      node->sched.pos <= GPIR_INSTR_SLOT_ALU_END)
      else if (node->sched.pos >= GPIR_INSTR_SLOT_REG0_LOAD0 &&
               else if (node->sched.pos >= GPIR_INSTR_SLOT_REG1_LOAD0 &&
               else if (node->sched.pos >= GPIR_INSTR_SLOT_MEM_LOAD0 &&
               else if (node->sched.pos >= GPIR_INSTR_SLOT_STORE0 &&
                           if (node->op == gpir_op_complex1 || node->op == gpir_op_select)
            node->sched.pos = -1;
      }
      void gpir_instr_print_prog(gpir_compiler *comp)
   {
      struct {
      int len;
      } fields[] = {
      [GPIR_INSTR_SLOT_MUL0] = { 4, "mul0" },
   [GPIR_INSTR_SLOT_MUL1] = { 4, "mul1" },
   [GPIR_INSTR_SLOT_ADD0] = { 4, "add0" },
   [GPIR_INSTR_SLOT_ADD1] = { 4, "add1" },
   [GPIR_INSTR_SLOT_REG0_LOAD3] = { 15, "load0" },
   [GPIR_INSTR_SLOT_REG1_LOAD3] = { 15, "load1" },
   [GPIR_INSTR_SLOT_MEM_LOAD3] = { 15, "load2" },
   [GPIR_INSTR_SLOT_STORE3] = { 15, "store" },
   [GPIR_INSTR_SLOT_COMPLEX] = { 4, "cmpl" },
               printf("========prog instr========\n");
   printf("     ");
   for (int i = 0; i < GPIR_INSTR_SLOT_NUM; i++) {
      if (fields[i].len)
      }
            int index = 0;
   list_for_each_entry(gpir_block, block, &comp->block_list, list) {
      list_for_each_entry(gpir_instr, instr, &block->instr_list, list) {
               char buff[16] = "null";
   int start = 0;
   for (int j = 0; j < GPIR_INSTR_SLOT_NUM; j++) {
      gpir_node *node = instr->slots[j];
   if (fields[j].len) {
                           strcpy(buff, "null");
      }
   else {
      if (node)
               }
      }
      }
      }
