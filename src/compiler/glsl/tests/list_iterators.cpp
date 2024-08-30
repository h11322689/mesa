   /*
   * Copyright © 2020 Intel Corporation
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
      #include "list.h"
      class test_node_inherite : public exec_node  {
   public:
                  };
      class list_iterators_node_inherite : public ::testing::TestWithParam<size_t> {
   public:
      virtual void SetUp();
                        };
      void
   list_iterators_node_inherite::SetUp()
   {
                        for (size_t i = 0; i < GetParam(); i++) {
      test_node_inherite *node = new(mem_ctx) test_node_inherite();
   node->value = i;
         }
      void
   list_iterators_node_inherite::TearDown()
   {
               ralloc_free(mem_ctx);
      }
      INSTANTIATE_TEST_SUITE_P(
      list_iterators_node_inherite,
   list_iterators_node_inherite,
      );
      TEST_P(list_iterators_node_inherite, foreach_in_list)
   {
      size_t i = 0;
   foreach_in_list(test_node_inherite, n, &node_list) {
      EXPECT_EQ(n->value, i);
         }
      TEST_P(list_iterators_node_inherite, foreach_in_list_reverse)
   {
      size_t i = GetParam() - 1;
   foreach_in_list_reverse(test_node_inherite, n, &node_list) {
      EXPECT_EQ(n->value, i);
         }
      TEST_P(list_iterators_node_inherite, foreach_in_list_safe)
   {
      size_t i = 0;
   foreach_in_list_safe(test_node_inherite, n, &node_list) {
               if (i % 2 == 0) {
                                 }
      TEST_P(list_iterators_node_inherite, foreach_in_list_reverse_safe)
   {
      size_t i = GetParam() - 1;
   foreach_in_list_reverse_safe(test_node_inherite, n, &node_list) {
               if (i % 2 == 0) {
                                 }
      TEST_P(list_iterators_node_inherite, foreach_in_list_use_after)
   {
      size_t i = 0;
   foreach_in_list_use_after(test_node_inherite, n, &node_list) {
               if (i == GetParam() / 2) {
                              if (GetParam() > 0) {
            }
      class test_node_embed {
         public:
         uint32_t value_header;
   exec_node node;
               };
      class list_iterators_node_embed : public ::testing::TestWithParam<size_t> {
   public:
      virtual void SetUp();
                        };
      void
   list_iterators_node_embed::SetUp()
   {
                        for (size_t i = 0; i < GetParam(); i++) {
      test_node_embed *node = new(mem_ctx) test_node_embed();
   node->value_header = i;
   node->value_footer = i;
         }
      void
   list_iterators_node_embed::TearDown()
   {
               ralloc_free(mem_ctx);
      }
      INSTANTIATE_TEST_SUITE_P(
      list_iterators_node_embed,
   list_iterators_node_embed,
      );
      TEST_P(list_iterators_node_embed, foreach_list_typed)
   {
      size_t i = 0;
   foreach_list_typed(test_node_embed, n, node, &node_list) {
      EXPECT_EQ(n->value_header, i);
   EXPECT_EQ(n->value_footer, i);
         }
      TEST_P(list_iterators_node_embed, foreach_list_typed_from)
   {
      if (GetParam() == 0) {
                           size_t i = 0;
   for (; i < GetParam() / 2; i++) {
                  foreach_list_typed_from(test_node_embed, n, node, &node_list, start_node) {
      EXPECT_EQ(n->value_header, i);
   EXPECT_EQ(n->value_footer, i);
         }
      TEST_P(list_iterators_node_embed, foreach_list_typed_reverse)
   {
      size_t i = GetParam() - 1;
   foreach_list_typed_reverse(test_node_embed, n, node, &node_list) {
      EXPECT_EQ(n->value_header, i);
   EXPECT_EQ(n->value_footer, i);
         }
      TEST_P(list_iterators_node_embed, foreach_list_typed_safe)
   {
      size_t i = 0;
   foreach_list_typed_safe(test_node_embed, n, node, &node_list) {
      EXPECT_EQ(n->value_header, i);
            if (i % 2 == 0) {
                                 }
      TEST_P(list_iterators_node_embed, foreach_list_typed_reverse_safe)
   {
      size_t i = GetParam() - 1;
   foreach_list_typed_reverse_safe(test_node_embed, n, node, &node_list) {
      EXPECT_EQ(n->value_header, i);
            if (i % 2 == 0) {
                                 }
