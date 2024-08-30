   /*
   * Copyright © 2010 Intel Corporation
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
      /** @file register_allocate.c
   *
   * Graph-coloring register allocator.
   *
   * The basic idea of graph coloring is to make a node in a graph for
   * every thing that needs a register (color) number assigned, and make
   * edges in the graph between nodes that interfere (can't be allocated
   * to the same register at the same time).
   *
   * During the "simplify" process, any any node with fewer edges than
   * there are registers means that that edge can get assigned a
   * register regardless of what its neighbors choose, so that node is
   * pushed on a stack and removed (with its edges) from the graph.
   * That likely causes other nodes to become trivially colorable as well.
   *
   * Then during the "select" process, nodes are popped off of that
   * stack, their edges restored, and assigned a color different from
   * their neighbors.  Because they were pushed on the stack only when
   * they were trivially colorable, any color chosen won't interfere
   * with the registers to be popped later.
   *
   * The downside to most graph coloring is that real hardware often has
   * limitations, like registers that need to be allocated to a node in
   * pairs, or aligned on some boundary.  This implementation follows
   * the paper "Retargetable Graph-Coloring Register Allocation for
   * Irregular Architectures" by Johan Runeson and Sven-Olof Nyström.
   *
   * In this system, there are register classes each containing various
   * registers, and registers may interfere with other registers.  For
   * example, one might have a class of base registers, and a class of
   * aligned register pairs that would each interfere with their pair of
   * the base registers.  Each node has a register class it needs to be
   * assigned to.  Define p(B) to be the size of register class B, and
   * q(B,C) to be the number of registers in B that the worst choice
   * register in C could conflict with.  Then, this system replaces the
   * basic graph coloring test of "fewer edges from this node than there
   * are registers" with "For this node of class B, the sum of q(B,C)
   * for each neighbor node of class C is less than pB".
   *
   * A nice feature of the pq test is that q(B,C) can be computed once
   * up front and stored in a 2-dimensional array, so that the cost of
   * coloring a node is constant with the number of registers.  We do
   * this during ra_set_finalize().
   */
      #include <stdbool.h>
   #include <stdlib.h>
      #include "blob.h"
   #include "ralloc.h"
   #include "util/bitset.h"
   #include "util/u_dynarray.h"
   #include "u_math.h"
   #include "register_allocate.h"
   #include "register_allocate_internal.h"
      /**
   * Creates a set of registers for the allocator.
   *
   * mem_ctx is a ralloc context for the allocator.  The reg set may be freed
   * using ralloc_free().
   */
   struct ra_regs *
   ra_alloc_reg_set(void *mem_ctx, unsigned int count, bool need_conflict_lists)
   {
      unsigned int i;
            regs = rzalloc(mem_ctx, struct ra_regs);
   regs->count = count;
            for (i = 0; i < count; i++) {
      regs->regs[i].conflicts = rzalloc_array(regs->regs, BITSET_WORD,
                  util_dynarray_init(&regs->regs[i].conflict_list,
         if (need_conflict_lists)
                  }
      /**
   * The register allocator by default prefers to allocate low register numbers,
   * since it was written for hardware (gen4/5 Intel) that is limited in its
   * multithreadedness by the number of registers used in a given shader.
   *
   * However, for hardware without that restriction, densely packed register
   * allocation can put serious constraints on instruction scheduling.  This
   * function tells the allocator to rotate around the registers if possible as
   * it allocates the nodes.
   */
   void
   ra_set_allocate_round_robin(struct ra_regs *regs)
   {
         }
      static void
   ra_add_conflict_list(struct ra_regs *regs, unsigned int r1, unsigned int r2)
   {
               if (reg1->conflict_list.mem_ctx) {
         }
      }
      void
   ra_add_reg_conflict(struct ra_regs *regs, unsigned int r1, unsigned int r2)
   {
      if (!BITSET_TEST(regs->regs[r1].conflicts, r2)) {
      ra_add_conflict_list(regs, r1, r2);
         }
      /**
   * Adds a conflict between base_reg and reg, and also between reg and
   * anything that base_reg conflicts with.
   *
   * This can simplify code for setting up multiple register classes
   * which are aggregates of some base hardware registers, compared to
   * explicitly using ra_add_reg_conflict.
   */
   void
   ra_add_transitive_reg_conflict(struct ra_regs *regs,
         {
               util_dynarray_foreach(&regs->regs[base_reg].conflict_list, unsigned int,
                  }
      /**
   * Set up conflicts between base_reg and it's two half registers reg0 and
   * reg1, but take care to not add conflicts between reg0 and reg1.
   *
   * This is useful for architectures where full size registers are aliased by
   * two half size registers (eg 32 bit float and 16 bit float registers).
   */
   void
   ra_add_transitive_reg_pair_conflict(struct ra_regs *regs,
         {
      ra_add_reg_conflict(regs, reg0, base_reg);
            util_dynarray_foreach(&regs->regs[base_reg].conflict_list, unsigned int, i) {
      unsigned int conflict = *i;
   if (conflict != reg1)
         if (conflict != reg0)
         }
      /**
   * Makes every conflict on the given register transitive.  In other words,
   * every register that conflicts with r will now conflict with every other
   * register conflicting with r.
   *
   * This can simplify code for setting up multiple register classes
   * which are aggregates of some base hardware registers, compared to
   * explicitly using ra_add_reg_conflict.
   */
   void
   ra_make_reg_conflicts_transitive(struct ra_regs *regs, unsigned int r)
   {
      struct ra_reg *reg = &regs->regs[r];
            BITSET_FOREACH_SET(c, reg->conflicts, regs->count) {
      struct ra_reg *other = &regs->regs[c];
   unsigned i;
   for (i = 0; i < BITSET_WORDS(regs->count); i++)
         }
      struct ra_class *
   ra_alloc_reg_class(struct ra_regs *regs)
   {
               regs->classes = reralloc(regs->regs, regs->classes, struct ra_class *,
            class = rzalloc(regs, struct ra_class);
            /* Users may rely on the class index being allocated in order starting from 0. */
   class->index = regs->class_count++;
                        }
      /**
   * Creates a register class for contiguous register groups of a base register
   * set.
   *
   * A reg set using this type of register class must use only this type of
   * register class.
   */
   struct ra_class *
   ra_alloc_contig_reg_class(struct ra_regs *regs, int contig_len)
   {
               assert(contig_len != 0);
               }
      struct ra_class *
   ra_get_class_from_index(struct ra_regs *regs, unsigned int class)
   {
         }
      unsigned int
   ra_class_index(struct ra_class *c)
   {
         }
      void
   ra_class_add_reg(struct ra_class *class, unsigned int r)
   {
      assert(r < class->regset->count);
            BITSET_SET(class->regs, r);
      }
      /**
   * Returns true if the register belongs to the given class.
   */
   static bool
   reg_belongs_to_class(unsigned int r, struct ra_class *c)
   {
         }
      /**
   * Must be called after all conflicts and register classes have been
   * set up and before the register set is used for allocation.
   * To avoid costly q value computation, use the q_values paramater
   * to pass precomputed q values to this function.
   */
   void
   ra_set_finalize(struct ra_regs *regs, unsigned int **q_values)
   {
               for (b = 0; b < regs->class_count; b++) {
                  if (q_values) {
      for (b = 0; b < regs->class_count; b++) {
      for (c = 0; c < regs->class_count; c++) {
               } else {
      /* Compute, for each class B and C, how many regs of B an
   * allocation to C could conflict with.
   */
   for (b = 0; b < regs->class_count; b++) {
      for (c = 0; c < regs->class_count; c++) {
                     if (class_b->contig_len && class_c->contig_len) {
      if (class_b->contig_len == 1 && class_c->contig_len == 1) {
      /* If both classes are single registers, then they only
   * conflict if there are any regs shared between them.  This
   * is a cheap test for a common case.
   */
   class_b->q[c] = 0;
   for (int i = 0; i < BITSET_WORDS(regs->count); i++) {
      if (class_b->regs[i] & class_c->regs[i]) {
      class_b->q[c] = 1;
                              unsigned int max_conflicts = 0;
   unsigned int rc;
   BITSET_FOREACH_SET(rc, regs->classes[c]->regs, regs->count) {
      int start = MAX2(0, (int)rc - class_b->contig_len + 1);
   int end = MIN2(regs->count, rc + class_c->contig_len);
   unsigned int conflicts = 0;
   for (int i = start; i < end; i++) {
      if (BITSET_TEST(class_b->regs, i))
      }
   max_conflicts = MAX2(max_conflicts, conflicts);
   /* Unless a class has some restriction like the register
   * bases are all aligned, then we should quickly find this
   * limit and exit the loop.
   */
   if (max_conflicts == max_possible_conflicts)
      }
         } else {
      /* If you're doing contiguous classes, you have to be all in
                                                         util_dynarray_foreach(&regs->regs[rc].conflict_list,
            unsigned int rb = *rbp;
   if (reg_belongs_to_class(rb, regs->classes[b]))
      }
      }
                        for (b = 0; b < regs->count; b++) {
                  bool all_contig = true;
   for (int c = 0; c < regs->class_count; c++)
         if (all_contig) {
      /* In this case, we never need the conflicts lists (and it would probably
   * be a mistake to look at conflicts when doing contiguous classes!), so
   * free them.  TODO: Avoid the allocation in the first place.
   */
   for (int i = 0; i < regs->count; i++) {
      ralloc_free(regs->regs[i].conflicts);
            }
      void
   ra_set_serialize(const struct ra_regs *regs, struct blob *blob)
   {
      blob_write_uint32(blob, regs->count);
            bool is_contig = regs->classes[0]->contig_len != 0;
            if (!is_contig) {
      for (unsigned int r = 0; r < regs->count; r++) {
      struct ra_reg *reg = &regs->regs[r];
   blob_write_bytes(blob, reg->conflicts, BITSET_WORDS(regs->count) *
                        for (unsigned int c = 0; c < regs->class_count; c++) {
      struct ra_class *class = regs->classes[c];
   blob_write_bytes(blob, class->regs, BITSET_WORDS(regs->count) *
         blob_write_uint32(blob, class->contig_len);
   blob_write_uint32(blob, class->p);
                  }
      struct ra_regs *
   ra_set_deserialize(void *mem_ctx, struct blob_reader *blob)
   {
      unsigned int reg_count = blob_read_uint32(blob);
   unsigned int class_count = blob_read_uint32(blob);
            struct ra_regs *regs = ra_alloc_reg_set(mem_ctx, reg_count, false);
            if (is_contig) {
      for (int i = 0; i < regs->count; i++) {
      ralloc_free(regs->regs[i].conflicts);
         } else {
      for (unsigned int r = 0; r < reg_count; r++) {
      struct ra_reg *reg = &regs->regs[r];
   blob_copy_bytes(blob, reg->conflicts, BITSET_WORDS(reg_count) *
                  assert(regs->classes == NULL);
   regs->classes = ralloc_array(regs->regs, struct ra_class *, class_count);
            for (unsigned int c = 0; c < class_count; c++) {
      struct ra_class *class = rzalloc(regs, struct ra_class);
   regs->classes[c] = class;
   class->regset = regs;
            class->regs = ralloc_array(class, BITSET_WORD, BITSET_WORDS(reg_count));
   blob_copy_bytes(blob, class->regs, BITSET_WORDS(reg_count) *
            class->contig_len = blob_read_uint32(blob);
            class->q = ralloc_array(regs->classes[c], unsigned int, class_count);
                           }
      static uint64_t
   ra_get_num_adjacency_bits(uint64_t n)
   {
         }
      static uint64_t
   ra_get_adjacency_bit_index(unsigned n1, unsigned n2)
   {
      assert(n1 != n2);
   unsigned k1 = MAX2(n1, n2);
   unsigned k2 = MIN2(n1, n2);
      }
      static bool
   ra_test_adjacency_bit(struct ra_graph *g, unsigned n1, unsigned n2)
   {
      uint64_t index = ra_get_adjacency_bit_index(n1, n2);
      }
      static void
   ra_set_adjacency_bit(struct ra_graph *g, unsigned n1, unsigned n2)
   {
      unsigned index = ra_get_adjacency_bit_index(n1, n2);
      }
      static void
   ra_clear_adjacency_bit(struct ra_graph *g, unsigned n1, unsigned n2)
   {
      unsigned index = ra_get_adjacency_bit_index(n1, n2);
      }
      static void
   ra_add_node_adjacency(struct ra_graph *g, unsigned int n1, unsigned int n2)
   {
               int n1_class = g->nodes[n1].class;
   int n2_class = g->nodes[n2].class;
               }
      static void
   ra_node_remove_adjacency(struct ra_graph *g, unsigned int n1, unsigned int n2)
   {
      assert(n1 != n2);
            int n1_class = g->nodes[n1].class;
   int n2_class = g->nodes[n2].class;
            util_dynarray_delete_unordered(&g->nodes[n1].adjacency_list, unsigned int,
      }
      static void
   ra_realloc_interference_graph(struct ra_graph *g, unsigned int alloc)
   {
      if (alloc <= g->alloc)
            /* If we always have a whole number of BITSET_WORDs, it makes it much
   * easier to memset the top of the growing bitsets.
   */
   assert(g->alloc % BITSET_WORDBITS == 0);
   alloc = align(alloc, BITSET_WORDBITS);
   g->nodes = rerzalloc(g, g->nodes, struct ra_node, g->alloc, alloc);
   g->adjacency = rerzalloc(g, g->adjacency, BITSET_WORD,
                  /* Initialize new nodes. */
   for (unsigned i = g->alloc; i < alloc; i++) {
      struct ra_node* node = g->nodes + i;
   util_dynarray_init(&node->adjacency_list, g);
   node->q_total = 0;
   node->forced_reg = NO_REG;
               /* These are scratch values and don't need to be zeroed.  We'll clear them
   * as part of ra_select() setup.
   */
   unsigned bitset_count = BITSET_WORDS(alloc);
   g->tmp.stack = reralloc(g, g->tmp.stack, unsigned int, alloc);
            g->tmp.reg_assigned = reralloc(g, g->tmp.reg_assigned, BITSET_WORD,
         g->tmp.pq_test = reralloc(g, g->tmp.pq_test, BITSET_WORD, bitset_count);
   g->tmp.min_q_total = reralloc(g, g->tmp.min_q_total, unsigned int,
         g->tmp.min_q_node = reralloc(g, g->tmp.min_q_node, unsigned int,
               }
      struct ra_graph *
   ra_alloc_interference_graph(struct ra_regs *regs, unsigned int count)
   {
               g = rzalloc(NULL, struct ra_graph);
   g->regs = regs;
   g->count = count;
               }
      void
   ra_resize_interference_graph(struct ra_graph *g, unsigned int count)
   {
      g->count = count;
   if (count > g->alloc)
      }
      void ra_set_select_reg_callback(struct ra_graph *g,
               {
      g->select_reg_callback = callback;
      }
      void
   ra_set_node_class(struct ra_graph *g,
         {
         }
      struct ra_class *
   ra_get_node_class(struct ra_graph *g,
         {
         }
      unsigned int
   ra_add_node(struct ra_graph *g, struct ra_class *class)
   {
      unsigned int n = g->count;
                        }
      void
   ra_add_node_interference(struct ra_graph *g,
         {
      assert(n1 < g->count && n2 < g->count);
   if (n1 != n2 && !ra_test_adjacency_bit(g, n1, n2)) {
      ra_set_adjacency_bit(g, n1, n2);
   ra_add_node_adjacency(g, n1, n2);
         }
      void
   ra_reset_node_interference(struct ra_graph *g, unsigned int n)
   {
      util_dynarray_foreach(&g->nodes[n].adjacency_list, unsigned int, n2p) {
                     }
      static void
   update_pq_info(struct ra_graph *g, unsigned int n)
   {
      int i = n / BITSET_WORDBITS;
   int n_class = g->nodes[n].class;
   if (g->nodes[n].tmp.q_total < g->regs->classes[n_class]->p) {
         } else if (g->tmp.min_q_total[i] != UINT_MAX) {
      /* Only update min_q_total and min_q_node if min_q_total != UINT_MAX so
   * that we don't update while we have stale data and accidentally mark
   * it as non-stale.  Also, in order to remain consistent with the old
   * naive implementation of the algorithm, we do a lexicographical sort
   * to ensure that we always choose the node with the highest node index.
   */
   if (g->nodes[n].tmp.q_total < g->tmp.min_q_total[i] ||
      (g->nodes[n].tmp.q_total == g->tmp.min_q_total[i] &&
   n > g->tmp.min_q_node[i])) {
   g->tmp.min_q_total[i] = g->nodes[n].tmp.q_total;
            }
      static void
   add_node_to_stack(struct ra_graph *g, unsigned int n)
   {
                        util_dynarray_foreach(&g->nodes[n].adjacency_list, unsigned int, n2p) {
      unsigned int n2 = *n2p;
            if (!BITSET_TEST(g->tmp.in_stack, n2) &&
      !BITSET_TEST(g->tmp.reg_assigned, n2)) {
   assert(g->nodes[n2].tmp.q_total >= g->regs->classes[n2_class]->q[n_class]);
   g->nodes[n2].tmp.q_total -= g->regs->classes[n2_class]->q[n_class];
                  g->tmp.stack[g->tmp.stack_count] = n;
   g->tmp.stack_count++;
            /* Flag the min_q_total for n's block as dirty so it gets recalculated */
      }
      /**
   * Simplifies the interference graph by pushing all
   * trivially-colorable nodes into a stack of nodes to be colored,
   * removing them from the graph, and rinsing and repeating.
   *
   * If we encounter a case where we can't push any nodes on the stack, then
   * we optimistically choose a node and push it on the stack. We heuristically
   * push the node with the lowest total q value, since it has the fewest
   * neighbors and therefore is most likely to be allocated.
   */
   static void
   ra_simplify(struct ra_graph *g)
   {
      bool progress = true;
            /* Figure out the high bit and bit mask for the first iteration of a loop
   * over BITSET_WORDs.
   */
            /* Do a quick pre-pass to set things up */
   g->tmp.stack_count = 0;
   for (int i = BITSET_WORDS(g->count) - 1, high_bit = top_word_high_bit;
      i >= 0; i--, high_bit = BITSET_WORDBITS - 1) {
   g->tmp.in_stack[i] = 0;
   g->tmp.reg_assigned[i] = 0;
   g->tmp.pq_test[i] = 0;
   g->tmp.min_q_total[i] = UINT_MAX;
   g->tmp.min_q_node[i] = UINT_MAX;
   for (int j = high_bit; j >= 0; j--) {
      unsigned int n = i * BITSET_WORDBITS + j;
   g->nodes[n].reg = g->nodes[n].forced_reg;
   g->nodes[n].tmp.q_total = g->nodes[n].q_total;
   if (g->nodes[n].reg != NO_REG)
                        while (progress) {
      unsigned int min_q_total = UINT_MAX;
                     for (int i = BITSET_WORDS(g->count) - 1, high_bit = top_word_high_bit;
                     BITSET_WORD skip = g->tmp.in_stack[i] | g->tmp.reg_assigned[i];
                  BITSET_WORD pq = g->tmp.pq_test[i] & ~skip;
   if (pq) {
      /* In this case, we have stuff we can immediately take off the
   * stack.  This also means that we're guaranteed to make progress
   * and we don't need to bother updating lowest_q_total because we
   * know we're going to loop again before attempting to do anything
   * optimistic.
   */
   for (int j = high_bit; j >= 0; j--) {
      if (pq & BITSET_BIT(j)) {
      unsigned int n = i * BITSET_WORDBITS + j;
   assert(n < g->count);
   add_node_to_stack(g, n);
   /* add_node_to_stack() may update pq_test for this word so
   * we need to update our local copy.
   */
   pq = g->tmp.pq_test[i] & ~skip;
            } else if (!progress) {
      if (g->tmp.min_q_total[i] == UINT_MAX) {
      /* The min_q_total and min_q_node are dirty because we added
   * one of these nodes to the stack.  It needs to be
   * recalculated.
   */
                           unsigned int n = i * BITSET_WORDBITS + j;
   assert(n < g->count);
   if (g->nodes[n].tmp.q_total < g->tmp.min_q_total[i]) {
      g->tmp.min_q_total[i] = g->nodes[n].tmp.q_total;
            }
   if (g->tmp.min_q_total[i] < min_q_total) {
      min_q_node = g->tmp.min_q_node[i];
                     if (!progress && min_q_total != UINT_MAX) {
                     add_node_to_stack(g, min_q_node);
                     }
      bool
   ra_class_allocations_conflict(struct ra_class *c1, unsigned int r1,
         {
      if (c1->contig_len) {
               int r1_end = r1 + c1->contig_len;
   int r2_end = r2 + c2->contig_len;
      } else {
            }
      static struct ra_node *
   ra_find_conflicting_neighbor(struct ra_graph *g, unsigned int n, unsigned int r)
   {
      util_dynarray_foreach(&g->nodes[n].adjacency_list, unsigned int, n2p) {
               /* If our adjacent node is in the stack, it's not allocated yet. */
   if (!BITSET_TEST(g->tmp.in_stack, n2) &&
      ra_class_allocations_conflict(g->regs->classes[g->nodes[n].class], r,
                           }
      /* Computes a bitfield of what regs are available for a given register
   * selection.
   *
   * This lets drivers implement a more complicated policy than our simple first
   * or round robin policies (which don't require knowing the whole bitset)
   */
   static bool
   ra_compute_available_regs(struct ra_graph *g, unsigned int n, BITSET_WORD *regs)
   {
               /* Populate with the set of regs that are in the node's class. */
            /* Remove any regs that conflict with nodes that we're adjacent to and have
   * already colored.
   */
   util_dynarray_foreach(&g->nodes[n].adjacency_list, unsigned int, n2p) {
      struct ra_node *n2 = &g->nodes[*n2p];
            if (!BITSET_TEST(g->tmp.in_stack, *n2p)) {
      if (c->contig_len) {
      int start = MAX2(0, (int)n2->reg - c->contig_len + 1);
   int end = MIN2(g->regs->count, n2->reg + n2c->contig_len);
   for (unsigned i = start; i < end; i++)
      } else {
      for (int j = 0; j < BITSET_WORDS(g->regs->count); j++)
                     for (int i = 0; i < BITSET_WORDS(g->regs->count); i++) {
      if (regs[i])
                  }
      /**
   * Pops nodes from the stack back into the graph, coloring them with
   * registers as they go.
   *
   * If all nodes were trivially colorable, then this must succeed.  If
   * not (optimistic coloring), then it may return false;
   */
   static bool
   ra_select(struct ra_graph *g)
   {
      int start_search_reg = 0;
            if (g->select_reg_callback)
            while (g->tmp.stack_count != 0) {
      unsigned int ri;
   unsigned int r = -1;
   int n = g->tmp.stack[g->tmp.stack_count - 1];
            /* set this to false even if we return here so that
   * ra_get_best_spill_node() considers this node later.
   */
            if (g->select_reg_callback) {
      if (!ra_compute_available_regs(g, n, select_regs)) {
      free(select_regs);
               r = g->select_reg_callback(n, select_regs, g->select_reg_callback_data);
      } else {
      /* Find the lowest-numbered reg which is not used by a member
   * of the graph adjacent to us.
   */
   for (ri = 0; ri < g->regs->count; ri++) {
      r = (start_search_reg + ri) % g->regs->count;
                  struct ra_node *conflicting = ra_find_conflicting_neighbor(g, n, r);
   if (!conflicting) {
      /* Found a reg! */
      }
   if (g->regs->classes[conflicting->class]->contig_len) {
      /* Skip to point at the last base reg of the conflicting reg
   * allocation -- the loop will increment us to check the next reg
   * after the conflicting allocaiton.
   */
   unsigned conflicting_end = (conflicting->reg +
         assert(conflicting_end >= r);
                  if (ri >= g->regs->count)
               g->nodes[n].reg = r;
            /* Rotate the starting point except for any nodes above the lowest
   * optimistically colorable node.  The likelihood that we will succeed
   * at allocating optimistically colorable nodes is highly dependent on
   * the way that the previous nodes popped off the stack are laid out.
   * The round-robin strategy increases the fragmentation of the register
   * file and decreases the number of nearby nodes assigned to the same
   * color, what increases the likelihood of spilling with respect to the
   * dense packing strategy.
   */
   if (g->regs->round_robin &&
      g->tmp.stack_count - 1 <= g->tmp.stack_optimistic_start)
                        }
      bool
   ra_allocate(struct ra_graph *g)
   {
      ra_simplify(g);
      }
      unsigned int
   ra_get_node_reg(struct ra_graph *g, unsigned int n)
   {
      if (g->nodes[n].forced_reg != NO_REG)
         else
      }
      /**
   * Forces a node to a specific register.  This can be used to avoid
   * creating a register class containing one node when handling data
   * that must live in a fixed location and is known to not conflict
   * with other forced register assignment (as is common with shader
   * input data).  These nodes do not end up in the stack during
   * ra_simplify(), and thus at ra_select() time it is as if they were
   * the first popped off the stack and assigned their fixed locations.
   * Nodes that use this function do not need to be assigned a register
   * class.
   *
   * Must be called before ra_simplify().
   */
   void
   ra_set_node_reg(struct ra_graph *g, unsigned int n, unsigned int reg)
   {
         }
      static float
   ra_get_spill_benefit(struct ra_graph *g, unsigned int n)
   {
      float benefit = 0;
            /* Define the benefit of eliminating an interference between n, n2
   * through spilling as q(C, B) / p(C).  This is similar to the
   * "count number of edges" approach of traditional graph coloring,
   * but takes classes into account.
   */
   util_dynarray_foreach(&g->nodes[n].adjacency_list, unsigned int, n2p) {
      unsigned int n2 = *n2p;
   unsigned int n2_class = g->nodes[n2].class;
   benefit += ((float)g->regs->classes[n_class]->q[n2_class] /
                  }
      /**
   * Returns a node number to be spilled according to the cost/benefit using
   * the pq test, or -1 if there are no spillable nodes.
   */
   int
   ra_get_best_spill_node(struct ra_graph *g)
   {
      unsigned int best_node = -1;
   float best_benefit = 0.0;
            /* Consider any nodes that we colored successfully or the node we failed to
   * color for spilling. When we failed to color a node in ra_select(), we
   * only considered these nodes, so spilling any other ones would not result
   * in us making progress.
   */
   for (n = 0; n < g->count; n++) {
      float cost = g->nodes[n].spill_cost;
            if (cost <= 0.0f)
            if (BITSET_TEST(g->tmp.in_stack, n))
                     if (benefit / cost > best_benefit) {
      best_benefit = benefit / cost;
                     }
      /**
   * Only nodes with a spill cost set (cost != 0.0) will be considered
   * for register spilling.
   */
   void
   ra_set_node_spill_cost(struct ra_graph *g, unsigned int n, float cost)
   {
         }
