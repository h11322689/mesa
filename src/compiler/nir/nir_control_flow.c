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
      #include "nir_control_flow_private.h"
      /**
   * \name Control flow modification
   *
   * These functions modify the control flow tree while keeping the control flow
   * graph up-to-date. The invariants respected are:
   * 1. Each then statement, else statement, or loop body must have at least one
   *    control flow node.
   * 2. Each if-statement and loop must have one basic block before it and one
   *    after.
   * 3. Two basic blocks cannot be directly next to each other.
   * 4. If a basic block has a jump instruction, there must be only one and it
   *    must be at the end of the block.
   *
   * The purpose of the second one is so that we have places to insert code during
   * GCM, as well as eliminating the possibility of critical edges.
   */
   /*@{*/
      static inline void
   block_add_pred(nir_block *block, nir_block *pred)
   {
         }
      static inline void
   block_remove_pred(nir_block *block, nir_block *pred)
   {
                           }
      static void
   link_blocks(nir_block *pred, nir_block *succ1, nir_block *succ2)
   {
      pred->successors[0] = succ1;
   if (succ1 != NULL)
            pred->successors[1] = succ2;
   if (succ2 != NULL)
      }
      static void
   unlink_blocks(nir_block *pred, nir_block *succ)
   {
      if (pred->successors[0] == succ) {
      pred->successors[0] = pred->successors[1];
      } else {
      assert(pred->successors[1] == succ);
                  }
      static void
   unlink_block_successors(nir_block *block)
   {
      if (block->successors[1] != NULL)
         if (block->successors[0] != NULL)
      }
      static void
   link_non_block_to_block(nir_cf_node *node, nir_block *block)
   {
      if (node->type == nir_cf_node_if) {
      /*
   * We're trying to link an if to a block after it; this just means linking
   * the last block of the then and else branches.
                     nir_block *last_then_block = nir_if_last_then_block(if_stmt);
            if (!nir_block_ends_in_jump(last_then_block)) {
      unlink_block_successors(last_then_block);
               if (!nir_block_ends_in_jump(last_else_block)) {
      unlink_block_successors(last_else_block);
         } else {
            }
      static void
   link_block_to_non_block(nir_block *block, nir_cf_node *node)
   {
      if (node->type == nir_cf_node_if) {
      /*
   * We're trying to link a block to an if after it; this just means linking
   * the block to the first block of the then and else branches.
                     nir_block *first_then_block = nir_if_first_then_block(if_stmt);
            unlink_block_successors(block);
      } else if (node->type == nir_cf_node_loop) {
      /*
   * For similar reasons as the corresponding case in
   * link_non_block_to_block(), don't worry about if the loop header has
   * any predecessors that need to be unlinked.
                              unlink_block_successors(block);
         }
      /**
   * Replace a block's successor with a different one.
   */
   static void
   replace_successor(nir_block *block, nir_block *old_succ, nir_block *new_succ)
   {
      if (block->successors[0] == old_succ) {
         } else {
      assert(block->successors[1] == old_succ);
               block_remove_pred(old_succ, block);
      }
      /**
   * Takes a basic block and inserts a new empty basic block before it, making its
   * predecessors point to the new block. This essentially splits the block into
   * an empty header and a body so that another non-block CF node can be inserted
   * between the two. Note that this does *not* link the two basic blocks, so
   * some kind of cleanup *must* be performed after this call.
   */
      static nir_block *
   split_block_beginning(nir_block *block)
   {
      nir_block *new_block = nir_block_create(ralloc_parent(block));
   new_block->cf_node.parent = block->cf_node.parent;
            set_foreach(block->predecessors, entry) {
      nir_block *pred = (nir_block *)entry->key;
               /* Any phi nodes must stay part of the new block, or else their
   * sources will be messed up.
   */
   nir_foreach_phi_safe(phi, block) {
      exec_node_remove(&phi->instr.node);
   phi->instr.block = new_block;
                  }
      static void
   rewrite_phi_preds(nir_block *block, nir_block *old_pred, nir_block *new_pred)
   {
      nir_foreach_phi_safe(phi, block) {
      nir_foreach_phi_src(src, phi) {
      if (src->pred == old_pred) {
      src->pred = new_pred;
               }
      void
   nir_insert_phi_undef(nir_block *block, nir_block *pred)
   {
      nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
   nir_foreach_phi(phi, block) {
      nir_undef_instr *undef =
      nir_undef_instr_create(impl->function->shader,
            nir_instr_insert_before_cf_list(&impl->body, &undef->instr);
   nir_phi_src *src = nir_phi_instr_add_src(phi, pred, &undef->def);
         }
      /**
   * Moves the successors of source to the successors of dest, leaving both
   * successors of source NULL.
   */
      static void
   move_successors(nir_block *source, nir_block *dest)
   {
      nir_block *succ1 = source->successors[0];
            if (succ1) {
      unlink_blocks(source, succ1);
               if (succ2) {
      unlink_blocks(source, succ2);
               unlink_block_successors(dest);
      }
      /* Given a basic block with no successors that has been inserted into the
   * control flow tree, gives it the successors it would normally have assuming
   * it doesn't end in a jump instruction. Also inserts phi sources with undefs
   * if necessary.
   */
   static void
   block_add_normal_succs(nir_block *block)
   {
      if (exec_node_is_tail_sentinel(block->cf_node.node.next)) {
      nir_cf_node *parent = block->cf_node.parent;
   if (parent->type == nir_cf_node_if) {
                     link_blocks(block, next_block, NULL);
      } else if (parent->type == nir_cf_node_loop) {
               nir_block *cont_block;
   if (block == nir_loop_last_block(loop)) {
         } else {
      assert(block == nir_loop_last_continue_block(loop));
               link_blocks(block, cont_block, NULL);
      } else {
      nir_function_impl *impl = nir_cf_node_as_function(parent);
         } else {
      nir_cf_node *next = nir_cf_node_next(&block->cf_node);
   if (next->type == nir_cf_node_if) {
                              link_blocks(block, first_then_block, first_else_block);
   nir_insert_phi_undef(first_then_block, block);
      } else if (next->type == nir_cf_node_loop) {
                        link_blocks(block, first_block, NULL);
            }
      static nir_block *
   split_block_end(nir_block *block)
   {
      nir_block *new_block = nir_block_create(ralloc_parent(block));
   new_block->cf_node.parent = block->cf_node.parent;
            if (nir_block_ends_in_jump(block)) {
      /* Figure out what successor block would've had if it didn't have a jump
   * instruction, and make new_block have that successor.
   */
      } else {
                     }
      static nir_block *
   split_block_before_instr(nir_instr *instr)
   {
      assert(instr->type != nir_instr_type_phi);
            nir_foreach_instr_safe(cur_instr, instr->block) {
      if (cur_instr == instr)
            exec_node_remove(&cur_instr->node);
   cur_instr->block = new_block;
                  }
      /* Splits a basic block at the point specified by the cursor. The "before" and
   * "after" arguments are filled out with the blocks resulting from the split
   * if non-NULL. Note that the "beginning" of the block is actually interpreted
   * as before the first non-phi instruction, and it's illegal to split a block
   * before a phi instruction.
   */
      static void
   split_block_cursor(nir_cursor cursor,
         {
      nir_block *before, *after;
   switch (cursor.option) {
   case nir_cursor_before_block:
      after = cursor.block;
   before = split_block_beginning(cursor.block);
         case nir_cursor_after_block:
      before = cursor.block;
   after = split_block_end(cursor.block);
         case nir_cursor_before_instr:
      after = cursor.instr->block;
   before = split_block_before_instr(cursor.instr);
         case nir_cursor_after_instr:
      /* We lower this to split_block_before_instr() so that we can keep the
   * after-a-jump-instr case contained to split_block_end().
   */
   if (nir_instr_is_last(cursor.instr)) {
      before = cursor.instr->block;
      } else {
      after = cursor.instr->block;
      }
         default:
                  if (_before)
         if (_after)
      }
      /**
   * Inserts a non-basic block between two basic blocks and links them together.
   */
      static void
   insert_non_block(nir_block *before, nir_cf_node *node, nir_block *after)
   {
      node->parent = before->cf_node.parent;
   exec_node_insert_after(&before->cf_node.node, &node->node);
   if (!nir_block_ends_in_jump(before))
            }
      /* walk up the control flow tree to find the innermost enclosed loop */
   static nir_loop *
   nearest_loop(nir_cf_node *node)
   {
      while (node->type != nir_cf_node_loop) {
                     }
      void
   nir_loop_add_continue_construct(nir_loop *loop)
   {
               nir_block *cont = nir_block_create(ralloc_parent(loop));
   exec_list_push_tail(&loop->continue_list, &cont->cf_node.node);
            /* change predecessors and successors */
   nir_block *header = nir_loop_first_block(loop);
   nir_block *preheader = nir_block_cf_tree_prev(header);
   set_foreach(header->predecessors, entry) {
      nir_block *pred = (nir_block *)entry->key;
   if (pred != preheader)
                  }
      void
   nir_loop_remove_continue_construct(nir_loop *loop)
   {
               /* change predecessors and successors */
   nir_block *header = nir_loop_first_block(loop);
   nir_block *cont = nir_loop_first_continue_block(loop);
   set_foreach(cont->predecessors, entry) {
      nir_block *pred = (nir_block *)entry->key;
      }
               }
      static void
   remove_phi_src(nir_block *block, nir_block *pred)
   {
      nir_foreach_phi(phi, block) {
      nir_foreach_phi_src_safe(src, phi) {
      if (src->pred == pred) {
      list_del(&src->src.use_link);
   exec_node_remove(&src->node);
               }
      /*
   * update the CFG after a jump instruction has been added to the end of a block
   */
      void
   nir_handle_add_jump(nir_block *block)
   {
      nir_instr *instr = nir_block_last_instr(block);
            if (block->successors[0])
         if (block->successors[1])
                  nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
            switch (jump_instr->type) {
   case nir_jump_return:
   case nir_jump_halt:
      link_blocks(block, impl->end_block, NULL);
         case nir_jump_break: {
      nir_loop *loop = nearest_loop(&block->cf_node);
   nir_cf_node *after = nir_cf_node_next(&loop->cf_node);
   nir_block *after_block = nir_cf_node_as_block(after);
   link_blocks(block, after_block, NULL);
               case nir_jump_continue: {
      nir_loop *loop = nearest_loop(&block->cf_node);
   nir_block *cont_block = nir_loop_continue_target(loop);
   link_blocks(block, cont_block, NULL);
               case nir_jump_goto:
      link_blocks(block, jump_instr->target, NULL);
         case nir_jump_goto_if:
      link_blocks(block, jump_instr->else_target, jump_instr->target);
         default:
            }
      /* Removes the successor of a block with a jump. Note that the jump to be
   * eliminated may be free-floating.
   */
      static void
   unlink_jump(nir_block *block, nir_jump_type type, bool add_normal_successors)
   {
      if (block->successors[0])
         if (block->successors[1])
            unlink_block_successors(block);
   if (add_normal_successors)
      }
      void
   nir_handle_remove_jump(nir_block *block, nir_jump_type type)
   {
               nir_function_impl *impl = nir_cf_node_get_function(&block->cf_node);
      }
      static void
   update_if_uses(nir_cf_node *node)
   {
      if (node->type != nir_cf_node_if)
            nir_if *if_stmt = nir_cf_node_as_if(node);
            list_addtail(&if_stmt->condition.use_link,
      }
      /**
   * Stitch two basic blocks together into one. The aggregate must have the same
   * predecessors as the first and the same successors as the second.
   *
   * Returns a cursor pointing at the end of the before block (i.e.m between the
   * two blocks) once stiched together.
   */
      static nir_cursor
   stitch_blocks(nir_block *before, nir_block *after)
   {
      /*
   * We move after into before, so we have to deal with up to 2 successors vs.
   * possibly a large number of predecessors.
   *
   * TODO: special case when before is empty and after isn't?
            if (nir_block_ends_in_jump(before)) {
      assert(exec_list_is_empty(&after->instr_list));
   if (after->successors[0])
         if (after->successors[1])
         unlink_block_successors(after);
               } else {
                        foreach_list_typed(nir_instr, instr, node, &after->instr_list) {
                  exec_list_append(&before->instr_list, &after->instr_list);
                  }
      void
   nir_cf_node_insert(nir_cursor cursor, nir_cf_node *node)
   {
                        if (node->type == nir_cf_node_block) {
      nir_block *block = nir_cf_node_as_block(node);
   exec_node_insert_after(&before->cf_node.node, &block->cf_node.node);
   block->cf_node.parent = before->cf_node.parent;
   /* stitch_blocks() assumes that any block that ends with a jump has
   * already been setup with the correct successors, so we need to set
   * up jumps here as the block is being inserted.
   */
   if (nir_block_ends_in_jump(block))
            stitch_blocks(block, after);
      } else {
      update_if_uses(node);
         }
      static bool
   replace_ssa_def_uses(nir_def *def, void *void_impl)
   {
               nir_undef_instr *undef =
      nir_undef_instr_create(impl->function->shader,
            nir_instr_insert_before_cf_list(&impl->body, &undef->instr);
   nir_def_rewrite_uses(def, &undef->def);
      }
      static void
   cleanup_cf_node(nir_cf_node *node, nir_function_impl *impl)
   {
      switch (node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
   /* We need to walk the instructions and clean up defs/uses */
   nir_foreach_instr_safe(instr, block) {
      if (instr->type == nir_instr_type_jump) {
      nir_jump_instr *jump = nir_instr_as_jump(instr);
   unlink_jump(block, jump->type, false);
   if (jump->type == nir_jump_goto_if)
      } else {
      nir_foreach_def(instr, replace_ssa_def_uses, impl);
         }
               case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
   foreach_list_typed(nir_cf_node, child, node, &if_stmt->then_list)
         foreach_list_typed(nir_cf_node, child, node, &if_stmt->else_list)
            list_del(&if_stmt->condition.use_link);
               case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
   foreach_list_typed(nir_cf_node, child, node, &loop->body)
         foreach_list_typed(nir_cf_node, child, node, &loop->continue_list)
            }
   case nir_cf_node_function: {
      nir_function_impl *impl = nir_cf_node_as_function(node);
   foreach_list_typed(nir_cf_node, child, node, &impl->body)
            }
   default:
            }
      /**
   * Extracts everything between two cursors.  Returns the cursor which is
   * equivalent to the old begin/end curosors.
   */
   nir_cursor
   nir_cf_extract(nir_cf_list *extracted, nir_cursor begin, nir_cursor end)
   {
               if (nir_cursors_equal(begin, end)) {
      exec_list_make_empty(&extracted->list);
   extracted->impl = NULL; /* we shouldn't need this */
                        /* Splitting a block twice with two cursors created before either split is
   * tricky and there are a couple of places it can go wrong if both cursors
   * point to the same block.  One is if the second cursor is an block-based
   * cursor and, thanks to the split above, it ends up pointing to the wrong
   * block.  If it's a before_block cursor and it's in the same block as
   * begin, then begin must also be a before_block cursor and it should be
   * caught by the nir_cursors_equal check above and we won't get here.  If
   * it's an after_block cursor, we need to re-adjust to ensure that it
   * points to the second one of the split blocks, regardless of which it is.
   */
   if (end.option == nir_cursor_after_block && end.block == block_before)
                     /* The second place this can all go wrong is that it could be that the
   * second split places the original block after the new block in which case
   * the block_begin pointer that we saved off above is pointing to the block
   * at the end rather than the block in the middle like it's supposed to be.
   * In this case, we have to re-adjust begin_block to point to the middle
   * one.
   */
   if (block_begin == block_after)
            extracted->impl = nir_cf_node_get_function(&block_begin->cf_node);
            /* Dominance and other block-related information is toast. */
            nir_cf_node *cf_node = &block_begin->cf_node;
   nir_cf_node *cf_node_end = &block_end->cf_node;
   while (true) {
               exec_node_remove(&cf_node->node);
   cf_node->parent = NULL;
            if (cf_node == cf_node_end)
                           }
      static void
   relink_jump_halt_cf_node(nir_cf_node *node, nir_block *end_block)
   {
      switch (node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
   nir_instr *last_instr = nir_block_last_instr(block);
   if (last_instr == NULL || last_instr->type != nir_instr_type_jump)
            nir_jump_instr *jump = nir_instr_as_jump(last_instr);
   /* We can't move a CF list from one function to another while we still
   * have returns.
   */
            if (jump->type == nir_jump_halt) {
      unlink_block_successors(block);
      }
               case nir_cf_node_if: {
      nir_if *if_stmt = nir_cf_node_as_if(node);
   foreach_list_typed(nir_cf_node, child, node, &if_stmt->then_list)
         foreach_list_typed(nir_cf_node, child, node, &if_stmt->else_list)
                     case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(node);
   foreach_list_typed(nir_cf_node, child, node, &loop->body)
         foreach_list_typed(nir_cf_node, child, node, &loop->continue_list)
                     case nir_cf_node_function:
            default:
            }
      /**
   * Inserts a list at a given cursor. Returns the cursor at the end of the
   * insertion (i.e., at the end of the instructions contained in cf_list).
   */
   nir_cursor
   nir_cf_reinsert(nir_cf_list *cf_list, nir_cursor cursor)
   {
               if (exec_list_is_empty(&cf_list->list))
            nir_function_impl *cursor_impl =
         if (cf_list->impl != cursor_impl) {
      foreach_list_typed(nir_cf_node, node, node, &cf_list->list)
                        foreach_list_typed_safe(nir_cf_node, node, node, &cf_list->list) {
      exec_node_remove(&node->node);
   node->parent = before->cf_node.parent;
               stitch_blocks(before,
         return stitch_blocks(nir_cf_node_as_block(nir_cf_node_prev(&after->cf_node)),
      }
      void
   nir_cf_delete(nir_cf_list *cf_list)
   {
      foreach_list_typed(nir_cf_node, node, node, &cf_list->list) {
            }
