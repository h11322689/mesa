   /*
   * Copyright © 2012 Intel Corporation
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
   *    Eric Anholt <eric@anholt.net>
   *
   */
      #include "brw_cfg.h"
   #include "brw_shader.h"
      /** @file brw_cfg.cpp
   *
   * Walks the shader instructions generated and creates a set of basic
   * blocks with successor/predecessor edges connecting them.
   */
      using namespace brw;
      static bblock_t *
   pop_stack(exec_list *list)
   {
      bblock_link *link = (bblock_link *)list->get_tail();
   bblock_t *block = link->block;
               }
      static exec_node *
   link(void *mem_ctx, bblock_t *block, enum bblock_link_kind kind)
   {
      bblock_link *l = new(mem_ctx) bblock_link(block, kind);
      }
      void
   push_stack(exec_list *list, void *mem_ctx, bblock_t *block)
   {
      /* The kind of the link is immaterial, but we need to provide one since
   * this is (ab)using the edge data structure in order to implement a stack.
   */
      }
      bblock_t::bblock_t(cfg_t *cfg) :
         {
      instructions.make_empty();
   parents.make_empty();
      }
      void
   bblock_t::add_successor(void *mem_ctx, bblock_t *successor,
         {
      successor->parents.push_tail(::link(mem_ctx, this, kind));
      }
      bool
   bblock_t::is_predecessor_of(const bblock_t *block,
         {
      foreach_list_typed_safe (bblock_link, parent, link, &block->parents) {
      if (parent->block == this && parent->kind <= kind) {
                        }
      bool
   bblock_t::is_successor_of(const bblock_t *block,
         {
      foreach_list_typed_safe (bblock_link, child, link, &block->children) {
      if (child->block == this && child->kind <= kind) {
                        }
      static bool
   ends_block(const backend_instruction *inst)
   {
               return op == BRW_OPCODE_IF ||
         op == BRW_OPCODE_ELSE ||
   op == BRW_OPCODE_CONTINUE ||
   op == BRW_OPCODE_BREAK ||
      }
      static bool
   starts_block(const backend_instruction *inst)
   {
               return op == BRW_OPCODE_DO ||
      }
      bool
   bblock_t::can_combine_with(const bblock_t *that) const
   {
      if ((const bblock_t *)this->link.next != that)
            if (ends_block(this->end()) ||
      starts_block(that->start()))
            }
      void
   bblock_t::combine_with(bblock_t *that)
   {
      assert(this->can_combine_with(that));
   foreach_list_typed (bblock_link, link, link, &that->parents) {
                  this->end_ip = that->end_ip;
               }
      void
   bblock_t::dump() const
   {
               int ip = this->start_ip;
   foreach_inst_in_block(backend_instruction, inst, this) {
      fprintf(stderr, "%5d: ", ip);
   s->dump_instruction(inst);
         }
      cfg_t::cfg_t(const backend_shader *s, exec_list *instructions) :
         {
      mem_ctx = ralloc_context(NULL);
   block_list.make_empty();
   blocks = NULL;
            bblock_t *cur = NULL;
            bblock_t *entry = new_block();
   bblock_t *cur_if = NULL;    /**< BB ending with IF. */
   bblock_t *cur_else = NULL;  /**< BB ending with ELSE. */
   bblock_t *cur_endif = NULL; /**< BB starting with ENDIF. */
   bblock_t *cur_do = NULL;    /**< BB starting with DO. */
   bblock_t *cur_while = NULL; /**< BB immediately following WHILE. */
   exec_list if_stack, else_stack, do_stack, while_stack;
                     foreach_in_list_safe(backend_instruction, inst, instructions) {
      /* set_next_block wants the post-incremented ip */
                     switch (inst->opcode) {
   case BRW_OPCODE_IF:
      /* Push our information onto a stack so we can recover from
      * nested ifs.
   */
               cur_if = cur;
   cur_else = NULL;
            /* Set up our immediately following block, full of "then"
      * instructions.
      next = new_block();
            set_next_block(&cur, next, ip);
   break;
            case BRW_OPCODE_ELSE:
               next = new_block();
            assert(cur_if != NULL);
         set_next_block(&cur, next, ip);
   break;
            case BRW_OPCODE_ENDIF: {
      if (cur->instructions.is_empty()) {
      /* New block was just created; use it. */
                                                      if (cur_else) {
         } else {
      assert(cur_if != NULL);
                     /* Pop the stack so we're in the previous if/else/endif */
   cur_if = pop_stack(&if_stack);
   cur_else = pop_stack(&else_stack);
   break;
         }
   /* Push our information onto a stack so we can recover from
      * nested loops.
   */
               /* Set up the block just after the while.  Don't know when exactly
      * it will start, yet.
      cur_while = new_block();
               if (cur->instructions.is_empty()) {
      /* New block was just created; use it. */
                                                      /* Represent divergent execution of the loop as a pair of alternative
   * edges coming out of the DO instruction: For any physical iteration
   * of the loop a given logical thread can either start off enabled
   * (which is represented as the "next" successor), or disabled (if it
   * has reached a non-uniform exit of the loop during a previous
   * iteration, which is represented as the "cur_while" successor).
   *
   * The disabled edge will be taken by the logical thread anytime we
   * arrive at the DO instruction through a back-edge coming from a
   * conditional exit of the loop where divergent control flow started.
   *
   * This guarantees that there is a control-flow path from any
   * divergence point of the loop into the convergence point
   * (immediately past the WHILE instruction) such that it overlaps the
   * whole IP region of divergent control flow (potentially the whole
   * loop) *and* doesn't imply the execution of any instructions part
   * of the loop (since the corresponding execution mask bit will be
   * disabled for a diverging thread).
   *
   * This way we make sure that any variables that are live throughout
   * the region of divergence for an inactive logical thread are also
   * considered to interfere with any other variables assigned by
   * active logical threads within the same physical region of the
   * program, since otherwise we would risk cross-channel data
   * corruption.
   */
   next = new_block();
   cur->add_successor(mem_ctx, next, bblock_link_logical);
      break;
            case BRW_OPCODE_CONTINUE:
               /* A conditional CONTINUE may start a region of divergent control
   * flow until the start of the next loop iteration (*not* until the
   * end of the loop which is why the successor is not the top-level
   * divergence point at cur_do).  The live interval of any variable
   * extending through a CONTINUE edge is guaranteed to overlap the
   * whole region of divergent execution, because any variable live-out
   * at the CONTINUE instruction will also be live-in at the top of the
   * loop, and therefore also live-out at the bottom-most point of the
   * loop which is reachable from the top (since a control flow path
   * exists from a definition of the variable through this CONTINUE
   * instruction, the top of the loop, the (reachable) bottom of the
   * loop, the top of the loop again, into a use of the variable).
   */
         next = new_block();
   if (inst->predicate)
                        set_next_block(&cur, next, ip);
   break;
            case BRW_OPCODE_BREAK:
               /* A conditional BREAK instruction may start a region of divergent
   * control flow until the end of the loop if the condition is
   * non-uniform, in which case the loop will execute additional
   * iterations with the present channel disabled.  We model this as a
   * control flow path from the divergence point to the convergence
   * point that overlaps the whole IP range of the loop and skips over
   * the execution of any other instructions part of the loop.
   *
   * See the DO case for additional explanation.
   */
   assert(cur_do != NULL);
         next = new_block();
   if (inst->predicate)
                        set_next_block(&cur, next, ip);
   break;
            case BRW_OPCODE_WHILE:
                        /* A conditional WHILE instruction may start a region of divergent
   * control flow until the end of the loop, just like the BREAK
   * instruction.  See the BREAK case for more details.  OTOH an
   * unconditional WHILE instruction is non-divergent (just like an
   * unconditional CONTINUE), and will necessarily lead to the
   * execution of an additional iteration of the loop for all enabled
   * channels, so we may skip over the divergence point at the top of
   * the loop to keep the CFG as unambiguous as possible.
   */
   if (inst->predicate) {
         } else {
         set_next_block(&cur, cur_while, ip);
      /* Pop the stack so we're in the previous loop */
   cur_do = pop_stack(&do_stack);
   cur_while = pop_stack(&while_stack);
   break;
            default:
   break;
                                 }
      cfg_t::~cfg_t()
   {
         }
      void
   cfg_t::remove_block(bblock_t *block)
   {
      foreach_list_typed_safe (bblock_link, predecessor, link, &block->parents) {
      /* Remove block from all of its predecessors' successor lists. */
   foreach_list_typed_safe (bblock_link, successor, link,
            if (block == successor->block) {
      successor->link.remove();
                  /* Add removed-block's successors to its predecessors' successor lists. */
   foreach_list_typed (bblock_link, successor, link, &block->children) {
      if (!successor->block->is_successor_of(predecessor->block,
            predecessor->block->children.push_tail(link(mem_ctx,
                           foreach_list_typed_safe (bblock_link, successor, link, &block->children) {
      /* Remove block from all of its childrens' parents lists. */
   foreach_list_typed_safe (bblock_link, predecessor, link,
            if (block == predecessor->block) {
      predecessor->link.remove();
                  /* Add removed-block's predecessors to its successors' predecessor lists. */
   foreach_list_typed (bblock_link, predecessor, link, &block->parents) {
      if (!predecessor->block->is_predecessor_of(successor->block,
            successor->block->parents.push_tail(link(mem_ctx,
                                    for (int b = block->num; b < this->num_blocks - 1; b++) {
      this->blocks[b] = this->blocks[b + 1];
               this->blocks[this->num_blocks - 1]->num = this->num_blocks - 2;
      }
      bblock_t *
   cfg_t::new_block()
   {
                  }
      void
   cfg_t::set_next_block(bblock_t **cur, bblock_t *block, int ip)
   {
      if (*cur) {
                  block->start_ip = ip;
   block->num = num_blocks++;
   block_list.push_tail(&block->link);
      }
      void
   cfg_t::make_block_array()
   {
               int i = 0;
   foreach_block (block, this) {
         }
      }
      void
   cfg_t::dump()
   {
               foreach_block (block, this) {
      if (idom && idom->parent(block))
      fprintf(stderr, "START B%d IDOM(B%d)", block->num,
      else
            foreach_list_typed(bblock_link, link, link, &block->parents) {
      fprintf(stderr, " <%cB%d",
            }
   fprintf(stderr, "\n");
   if (s != NULL)
         fprintf(stderr, "END B%d", block->num);
   foreach_list_typed(bblock_link, link, link, &block->children) {
      fprintf(stderr, " %c>B%d",
            }
         }
      /* Calculates the immediate dominator of each block, according to "A Simple,
   * Fast Dominance Algorithm" by Keith D. Cooper, Timothy J. Harvey, and Ken
   * Kennedy.
   *
   * The authors claim that for control flow graphs of sizes normally encountered
   * (less than 1000 nodes) that this algorithm is significantly faster than
   * others like Lengauer-Tarjan.
   */
   idom_tree::idom_tree(const backend_shader *s) :
      num_parents(s->cfg->num_blocks),
      {
                        do {
               foreach_block(block, s->cfg) {
                     bblock_t *new_idom = NULL;
   foreach_list_typed(bblock_link, parent_link, link, &block->parents) {
      if (parent(parent_link->block)) {
      new_idom = (new_idom ? intersect(new_idom, parent_link->block) :
                  if (parent(block) != new_idom) {
      parents[block->num] = new_idom;
               }
      idom_tree::~idom_tree()
   {
         }
      bblock_t *
   idom_tree::intersect(bblock_t *b1, bblock_t *b2) const
   {
      /* Note, the comparisons here are the opposite of what the paper says
   * because we index blocks from beginning -> end (i.e. reverse post-order)
   * instead of post-order like they assume.
   */
   while (b1->num != b2->num) {
      while (b1->num > b2->num)
         while (b2->num > b1->num)
      }
   assert(b1);
      }
      void
   idom_tree::dump() const
   {
      printf("digraph DominanceTree {\n");
   for (unsigned i = 0; i < num_parents; i++)
            }
      void
   cfg_t::dump_cfg()
   {
      printf("digraph CFG {\n");
   for (int b = 0; b < num_blocks; b++) {
               foreach_list_typed_safe (bblock_link, child, link, &block->children) {
            }
      }
