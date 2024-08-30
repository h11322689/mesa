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
      #include <limits.h>
      #include "ppir.h"
      static int cmp_int(const void *a, const void *b)
   {
         }
      static void ppir_schedule_calc_sched_info(ppir_instr *instr)
   {
      int n = 0;
            /* update all children's sched info */
   ppir_instr_foreach_pred(instr, dep) {
               if (pred->reg_pressure < 0)
            if (instr->est < pred->est + 1)
            float reg_weight = 1.0 - 1.0 / list_length(&pred->succ_list);
   if (extra_reg > reg_weight)
                        /* leaf instr */
   if (!n) {
      instr->reg_pressure = 0;
               int i = 0, reg[n];
   ppir_instr_foreach_pred(instr, dep) {
      ppir_instr *pred = dep->pred;
               /* sort */
            for (i = 0; i < n; i++) {
      int pressure = reg[i] + n - (i + 1);
   if (pressure > instr->reg_pressure)
               /* If all children of this instr have multi parents, then this
   * instr need an extra reg to store its result. For example,
   * it's not fair for parent has the same reg pressure as child
   * if n==1 and child's successor>1, because we need 2 reg for
   * this.
   *
   * But we can't add a full reg to the reg_pressure, because the
   * last parent of a multi-successor child doesn't need an extra
   * reg. For example, a single child (with multi successor) instr
   * should has less reg pressure than a two children (with single
   * successor) instr.
   *
   * extra reg = min(all child)(1.0 - 1.0 / num successor)
   */
      }
      static void ppir_insert_ready_list(struct list_head *ready_list,
         {
               list_for_each_entry(ppir_instr, instr, ready_list, list) {
      if (insert_instr->parent_index < instr->parent_index ||
      (insert_instr->parent_index == instr->parent_index &&
   (insert_instr->reg_pressure < instr->reg_pressure ||
      (insert_instr->reg_pressure == instr->reg_pressure &&
      insert_pos = &instr->list;
                  list_del(&insert_instr->list);
      }
      static void ppir_schedule_ready_list(ppir_block *block,
         {
      if (list_is_empty(ready_list))
            ppir_instr *instr = list_first_entry(ready_list, ppir_instr, list);
            /* schedule the instr to the block instr list */
   list_add(&instr->list, &block->instr_list);
   instr->scheduled = true;
   block->sched_instr_index--;
            ppir_instr_foreach_pred(instr, dep) {
      ppir_instr *pred = dep->pred;
            bool ready = true;
   ppir_instr_foreach_succ(pred, dep) {
      ppir_instr *succ = dep->succ;
   if (!succ->scheduled) {
      ready = false;
         }
   /* all successor have been scheduled */
   if (ready)
                  }
      /* Register sensitive schedule algorithm from paper:
   * "Register-Sensitive Selection, Duplication, and Sequencing of Instructions"
   * Author: Vivek Sarkar,  Mauricio J. Serrano,  Barbara B. Simons
   */
   static void ppir_schedule_block(ppir_block *block)
   {
      /* move all instr to instr_list, block->instr_list will
   * contain schedule result */
   struct list_head instr_list;
   list_replace(&block->instr_list, &instr_list);
            /* step 2 & 3 */
   list_for_each_entry(ppir_instr, instr, &instr_list, list) {
      if (ppir_instr_is_root(instr))
            }
   block->sched_instr_base = block->comp->sched_instr_base;
            /* step 4 */
   struct list_head ready_list;
            /* step 5 */
   list_for_each_entry_safe(ppir_instr, instr, &instr_list, list) {
      if (ppir_instr_is_root(instr)) {
      instr->parent_index = INT_MAX;
                  /* step 6 */
      }
      bool ppir_schedule_prog(ppir_compiler *comp)
   {
      list_for_each_entry(ppir_block, block, &comp->block_list, list) {
                     }
