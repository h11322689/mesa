   /*
   * Copyright © 2017 Faith Ekstrand
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #undef NDEBUG
      #include "rb_tree.h"
      #include <assert.h>
   #include <gtest/gtest.h>
   #include <limits.h>
      /* A list of 100 random numbers from 1 to 100.  The number 30 is explicitly
   * missing from this list.
   */
   int test_numbers[] = {
      26, 12, 35, 15, 48, 11, 39, 23, 40, 18,
   39, 15, 40, 11, 42, 2, 5, 2, 28, 8,
   10, 22, 23, 38, 47, 12, 31, 22, 26, 39,
   9, 42, 32, 18, 36, 8, 32, 29, 9, 3,
   32, 49, 23, 11, 43, 41, 22, 42, 6, 35,
   38, 48, 5, 35, 39, 44, 22, 16, 16, 32,
   31, 50, 48, 5, 50, 8, 2, 32, 27, 34,
   42, 48, 22, 47, 10, 48, 39, 36, 28, 40,
   32, 33, 21, 17, 14, 38, 27, 6, 25, 18,
      };
      #define NON_EXISTANT_NUMBER 30
      #define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))
      struct rb_test_node {
      int key;
      };
      static int
   rb_test_node_cmp_void(const struct rb_node *n, const void *v)
   {
      struct rb_test_node *tn = rb_node_data(struct rb_test_node, n, node);
      }
      static int
   rb_test_node_cmp(const struct rb_node *a, const struct rb_node *b)
   {
      struct rb_test_node *ta = rb_node_data(struct rb_test_node, a, node);
               }
      static void
   validate_tree_order(struct rb_tree *tree, unsigned expected_count)
   {
      struct rb_test_node *prev = NULL;
   int max_val = -1;
   unsigned count = 0;
   rb_tree_foreach(struct rb_test_node, n, tree, node) {
      /* Everything should be in increasing order */
   assert(n->key >= max_val);
   if (n->key > max_val) {
         } else {
         /* Things should be stable, i.e., given equal keys, they should
   * show up in the list in order of insertion.  We insert them
   * in the order they are in in the array.
   */
            prev = n;
      }
            prev = NULL;
   max_val = -1;
   count = 0;
   rb_tree_foreach_safe(struct rb_test_node, n, tree, node) {
      /* Everything should be in increasing order */
   assert(n->key >= max_val);
   if (n->key > max_val) {
         } else {
         /* Things should be stable, i.e., given equal keys, they should
   * show up in the list in order of insertion.  We insert them
   * in the order they are in in the array.
   */
            prev = n;
      }
            prev = NULL;
   int min_val = INT_MAX;
   count = 0;
   rb_tree_foreach_rev(struct rb_test_node, n, tree, node) {
      /* Everything should be in increasing order */
   assert(n->key <= min_val);
   if (n->key < min_val) {
         } else {
         /* Things should be stable, i.e., given equal keys, they should
   * show up in the list in order of insertion.  We insert them
   * in the order they are in in the array.
   */
            prev = n;
      }
            prev = NULL;
   min_val = INT_MAX;
   count = 0;
   rb_tree_foreach_rev_safe(struct rb_test_node, n, tree, node) {
      /* Everything should be in increasing order */
   assert(n->key <= min_val);
   if (n->key < min_val) {
         } else {
         /* Things should be stable, i.e., given equal keys, they should
   * show up in the list in order of insertion.  We insert them
   * in the order they are in in the array.
   */
            prev = n;
      }
      }
      static void
   validate_search(struct rb_tree *tree, int first_number,
         {
      struct rb_node *n;
            /* Search for all of the values */
   for (int i = first_number; i <= last_number; i++) {
      n = rb_tree_search(tree, &test_numbers[i], rb_test_node_cmp_void);
   tn = rb_node_data(struct rb_test_node, n, node);
            n = rb_tree_search_sloppy(tree, &test_numbers[i],
         tn = rb_node_data(struct rb_test_node, n, node);
               int missing_key = NON_EXISTANT_NUMBER;
   n = rb_tree_search(tree, &missing_key, rb_test_node_cmp_void);
            n = rb_tree_search_sloppy(tree, &missing_key, rb_test_node_cmp_void);
   if (rb_tree_is_empty(tree)) {
         } else {
      assert(n != NULL);
   tn = rb_node_data(struct rb_test_node, n, node);
   assert(tn->key != missing_key);
   if (tn->key < missing_key) {
         struct rb_node *next = rb_node_next(n);
   if (next != NULL) {
      struct rb_test_node *tnext =
            } else {
         struct rb_node *prev = rb_node_prev(n);
   if (prev != NULL) {
      struct rb_test_node *tprev =
                  }
      TEST(RBTreeTest, InsertAndSearch)
   {
      struct rb_test_node nodes[ARRAY_SIZE(test_numbers)];
                     for (unsigned i = 0; i < ARRAY_SIZE(test_numbers); i++) {
      nodes[i].key = test_numbers[i];
   rb_tree_insert(&tree, &nodes[i].node, rb_test_node_cmp);
   rb_tree_validate(&tree);
   validate_tree_order(&tree, i + 1);
               for (unsigned i = 0; i < ARRAY_SIZE(test_numbers); i++) {
      rb_tree_remove(&tree, &nodes[i].node);
   rb_tree_validate(&tree);
   validate_tree_order(&tree, ARRAY_SIZE(test_numbers) - i - 1);
         }
      TEST(RBTreeTest, FindFirst)
   {
      struct rb_test_node nodes[100];
                              for (unsigned i = 0; i < ARRAY_SIZE(nodes); i++) {
      nodes[i].key = x;
                        ASSERT_NE(nullptr, n);
   EXPECT_EQ(nullptr, rb_node_prev(n));
      }
      TEST(RBTreeTest, FindFirstOfMiddle)
   {
      struct rb_test_node nodes[3 * 33];
                     unsigned i;
   for (i = 0; i < ARRAY_SIZE(nodes) / 3; i++) {
      nodes[i].key = i * 13;
               const int x = i * 13;
   for (/* empty */; i < 2 * ARRAY_SIZE(nodes) / 3; i++) {
      nodes[i].key = x;
               for (/* empty */; i < ARRAY_SIZE(nodes); i++) {
      nodes[i].key = i * 13;
                                                      }
