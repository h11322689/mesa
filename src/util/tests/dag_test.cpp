   /*
   * Copyright © 2021 Google, Inc.
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
      #include <gtest/gtest.h>
   #include "util/dag.h"
      class dag_test : public ::testing::Test {
   protected:
      dag_test();
            void *mem_ctx;
   struct util_dynarray expect, actual;
      };
      dag_test::dag_test()
   {
      mem_ctx = ralloc_context(NULL);
   util_dynarray_init(&expect, mem_ctx);
   util_dynarray_init(&actual, mem_ctx);
      }
      dag_test::~dag_test()
   {
         }
      struct node: public dag_node {
               /* Overload >> to make describing our test case graphs easier to read */
   struct node &operator>>(struct node &child) {
      dag_add_edge(static_cast<struct dag_node *>(this),
                     void add_edge(struct node &child, uintptr_t data) {
      dag_add_edge(static_cast<struct dag_node *>(this),
               void add_edge_max_data(struct node &child, uintptr_t data) {
      dag_add_edge_max_data(static_cast<struct dag_node *>(this),
         };
      static void output_cb(struct dag_node *dag_node, void *data)
   {
      struct node *node = static_cast<struct node *>(dag_node);
   struct util_dynarray *output = (struct util_dynarray *)data;
      }
      static void
   init_nodes(struct dag *dag, struct node *nodes, unsigned num_nodes)
   {
      for (unsigned i = 0; i < num_nodes; i++) {
      dag_init_node(dag, static_cast<struct dag_node *>(&nodes[i]));
         }
      #define INIT_NODES(num_nodes)                            \
      typedef struct { int order[num_nodes]; } result_type; \
   struct node node[(num_nodes)];                        \
         #define SET_EXPECTED(...) do {                           \
   result_type res = {{ __VA_ARGS__ }};             \
   util_dynarray_append(&expect, result_type, res); \
   } while (0)
      static bool
   int_dynarrays_equal(struct util_dynarray *a, struct util_dynarray *b)
   {
      if (util_dynarray_num_elements(a, int) != util_dynarray_num_elements(b, int))
            for (unsigned i = 0; i < util_dynarray_num_elements(a, int); i++) {
      if (*util_dynarray_element(a, int, i) !=
      *util_dynarray_element(b, int, i)) {
         }
      }
      static testing::AssertionResult
   int_dynarrays_equal_pred(const char *a_expr,
                     {
   if (int_dynarrays_equal(a, b)) return testing::AssertionSuccess();
                           result << ", (";
   for (unsigned i = 0; i < util_dynarray_num_elements(a, int); i++) {
      if (i != 0)
                     result << ") != (";
   for (unsigned i = 0; i < util_dynarray_num_elements(b, int); i++) {
      if (i != 0)
                                 }
      #define TEST_CHECK() EXPECT_PRED_FORMAT2(int_dynarrays_equal_pred, &expect, &actual)
      TEST_F(dag_test, simple)
   {
               /*     0
   *    / \
   *   1   2
   */
   node[0] >> node[1];
            /* Expected traversal order: [1, 2, 0] */
                        }
      TEST_F(dag_test, duplicate_edge)
   {
               node[0].add_edge(node[1], 0);
   node[0].add_edge(node[1], 1);
                                          }
      TEST_F(dag_test, duplicate_edge_max_data)
   {
               node[0].add_edge_max_data(node[1], 0);
   node[0].add_edge_max_data(node[1], 1);
                     util_dynarray_foreach (&node[0].edges, struct dag_edge, edge) {
      if (edge->child == &node[1]) {
         } else {
      EXPECT_EQ(edge->child, &node[2]);
                                       }
      TEST_F(dag_test, simple_many_children)
   {
               /*     _ 0 _
   *    / /|\ \
   *   / / | \ \
   *  |  | | |  |
   *  1  2 3 4  5
   */
   node[0] >> node[1];
   node[0] >> node[2];
   node[0] >> node[3];
   node[0] >> node[4];
            /* Expected traversal order: [1, 2, 3, 4, 5, 0] */
                        }
      TEST_F(dag_test, simple_many_parents)
   {
               /*     _ 0 _
   *    / /|\ \
   *   / / | \ \
   *  |  | | |  |
   *  1  2 3 4  5
   *  |  | | |  |
   *   \ \ | / /
   *    \ \|/ /
   *     ‾ 6 ‾
   */
   node[0] >> node[1] >> node[6];
   node[0] >> node[2] >> node[6];
   node[0] >> node[3] >> node[6];
   node[0] >> node[4] >> node[6];
            /* Expected traversal order: [6, 1, 2, 3, 4, 5, 0] */
                        }
      TEST_F(dag_test, complex)
   {
               /*     0
   *    / \
   *   1   3
   *  / \  |\
   * 2  |  | \
   *  \ / /   5
   *   4 ‾
   */
   node[0] >> node[1] >> node[2] >> node[4];
   node[1] >> node[4];
   node[0] >> node[3];
   node[3] >> node[4];
            /* Expected traversal order: [4, 2, 1, 5, 3, 0] */
                        }
