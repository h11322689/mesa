   /*
   * Copyright (C) 2019 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors (Collabora):
   *      Alyssa Rosenzweig <alyssa.rosenzweig@collabora.com>
   */
      #include "lcra.h"
   #include <assert.h>
   #include <limits.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include "util/macros.h"
   #include "util/u_math.h"
      /* This module is the reference implementation of "Linearly Constrained
   * Register Allocation". The paper is available in PDF form
   * (https://people.collabora.com/~alyssa/LCRA.pdf) as well as Markdown+LaTeX
   * (https://gitlab.freedesktop.org/alyssa/lcra/blob/master/LCRA.md)
   */
      struct lcra_state *
   lcra_alloc_equations(unsigned node_count, unsigned class_count)
   {
               l->node_count = node_count;
            l->alignment = calloc(sizeof(l->alignment[0]), node_count);
   l->linear = calloc(sizeof(l->linear[0]), node_count * node_count);
   l->modulus = calloc(sizeof(l->modulus[0]), node_count);
   l->class = calloc(sizeof(l->class[0]), node_count);
   l->class_start = calloc(sizeof(l->class_start[0]), class_count);
   l->class_disjoint =
         l->class_size = calloc(sizeof(l->class_size[0]), class_count);
   l->spill_cost = calloc(sizeof(l->spill_cost[0]), node_count);
                        }
      void
   lcra_free(struct lcra_state *l)
   {
      if (!l)
            free(l->alignment);
   free(l->linear);
   free(l->modulus);
   free(l->class);
   free(l->class_start);
   free(l->class_disjoint);
   free(l->class_size);
   free(l->spill_cost);
               }
      void
   lcra_set_alignment(struct lcra_state *l, unsigned node, unsigned align_log2,
         {
         }
      void
   lcra_set_disjoint_class(struct lcra_state *l, unsigned c1, unsigned c2)
   {
      l->class_disjoint[(c1 * l->class_count) + c2] = true;
      }
      void
   lcra_restrict_range(struct lcra_state *l, unsigned node, unsigned len)
   {
      if (node < l->node_count && l->alignment[node]) {
      unsigned BA = l->alignment[node];
   unsigned alignment = (BA & 0xffff) - 1;
   unsigned bound = BA >> 16;
         }
      void
   lcra_add_node_interference(struct lcra_state *l, unsigned i, unsigned cmask_i,
         {
      if (i == j)
            if (l->class_disjoint[(l->class[i] * l -> class_count) + l->class[j]])
            uint32_t constraint_fw = 0;
            for (unsigned D = 0; D < 16; ++D) {
      if (cmask_i & (cmask_j << D)) {
      constraint_bw |= (1 << (15 + D));
               if (cmask_i & (cmask_j >> D)) {
      constraint_fw |= (1 << (15 + D));
                  l->linear[j * l->node_count + i] |= constraint_fw;
      }
      static bool
   lcra_test_linear(struct lcra_state *l, unsigned *solutions, unsigned i)
   {
      unsigned *row = &l->linear[i * l->node_count];
            for (unsigned j = 0; j < l->node_count; ++j) {
      if (solutions[j] == ~0)
                     if (lhs < -15 || lhs > 15)
            if (row[j] & (1 << (lhs + 15)))
                  }
      bool
   lcra_solve(struct lcra_state *l)
   {
      for (unsigned step = 0; step < l->node_count; ++step) {
      if (l->solutions[step] != ~0)
         if (l->alignment[step] == 0)
            unsigned _class = l->class[step];
            unsigned BA = l->alignment[step];
   unsigned shift = (BA & 0xffff) - 1;
            unsigned P = bound >> shift;
   unsigned Q = l->modulus[step];
   unsigned r_max = l->class_size[_class];
   unsigned k_max = r_max >> shift;
   unsigned m_max = k_max / P;
            for (unsigned m = 0; m < m_max; ++m) {
      for (unsigned n = 0; n < Q; ++n) {
                     if (succ)
               if (succ)
               /* Out of registers - prepare to spill */
   if (!succ) {
      l->spill_class = l->class[step];
                     }
      /* Register spilling is implemented with a cost-benefit system. Costs are set
   * by the user. Benefits are calculated from the constraints. */
      void
   lcra_set_node_spill_cost(struct lcra_state *l, unsigned node, signed cost)
   {
      if (node < l->node_count)
      }
      static unsigned
   lcra_count_constraints(struct lcra_state *l, unsigned i)
   {
      unsigned count = 0;
            for (unsigned j = 0; j < l->node_count; ++j)
               }
      signed
   lcra_get_best_spill_node(struct lcra_state *l)
   {
      /* If there are no constraints on a node, do not pick it to spill under
   * any circumstance, or else we would hang rather than fail RA */
   float best_benefit = 0.0;
            for (unsigned i = 0; i < l->node_count; ++i) {
      /* Find spillable nodes */
   if (l->class[i] != l->spill_class)
         if (l->spill_cost[i] < 0)
            /* Adapted from Chaitin's heuristic */
   float constraints = lcra_count_constraints(l, i);
   float cost = (l->spill_cost[i] + 1);
            if (benefit > best_benefit) {
      best_benefit = benefit;
                     }
