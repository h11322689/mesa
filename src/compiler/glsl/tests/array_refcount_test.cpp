   /*
   * Copyright © 2016 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
   #include <gtest/gtest.h>
   #include "ir.h"
   #include "ir_array_refcount.h"
   #include "ir_builder.h"
   #include "util/hash_table.h"
      using namespace ir_builder;
      class array_refcount_test : public ::testing::Test {
   public:
      virtual void SetUp();
            exec_list instructions;
   ir_factory *body;
            /**
   * glsl_type for a vec4[3][4][5].
   *
   * The exceptionally verbose name is picked because it matches the syntax
   * of http://cdecl.org/.
   */
            /**
   * glsl_type for a int[3].
   *
   * The exceptionally verbose name is picked because it matches the syntax
   * of http://cdecl.org/.
   */
            /**
   * Wrapper to access private member "bits" of ir_array_refcount_entry
   *
   * The test class is a friend to ir_array_refcount_entry, but the
   * individual tests are not part of the class.  Since the friendliness of
   * the test class does not extend to the tests, provide a wrapper.
   */
   const BITSET_WORD *get_bits(const ir_array_refcount_entry &entry)
   {
                  /**
   * Wrapper to access private member "num_bits" of ir_array_refcount_entry
   *
   * The test class is a friend to ir_array_refcount_entry, but the
   * individual tests are not part of the class.  Since the friendliness of
   * the test class does not extend to the tests, provide a wrapper.
   */
   unsigned get_num_bits(const ir_array_refcount_entry &entry)
   {
                  /**
   * Wrapper to access private member "array_depth" of ir_array_refcount_entry
   *
   * The test class is a friend to ir_array_refcount_entry, but the
   * individual tests are not part of the class.  Since the friendliness of
   * the test class does not extend to the tests, provide a wrapper.
   */
   unsigned get_array_depth(const ir_array_refcount_entry &entry)
   {
            };
      void
   array_refcount_test::SetUp()
   {
                        instructions.make_empty();
            /* The type of vec4 x[3][4][5]; */
   const glsl_type *const array_5_of_vec4 =
         const glsl_type *const array_4_of_array_5_of_vec4 =
         array_3_of_array_4_of_array_5_of_vec4 =
               }
      void
   array_refcount_test::TearDown()
   {
      delete body;
            ralloc_free(mem_ctx);
               }
      static operand
   deref_array(operand array, operand index)
   {
                           }
      static operand
   deref_struct(operand s, const char *field)
   {
                           }
      /**
   * Verify that only the specified set of ir_variables exists in the hash table
   */
   static void
   validate_variables_in_hash_table(struct hash_table *ht,
               {
      ir_variable **vars = new ir_variable *[count];
            /* Make a copy of the list of expected ir_variables.  The copied list can
   * be modified during the checking.
   */
            for (unsigned i = 0; i < count; i++)
                     hash_table_foreach(ht, entry) {
      const ir_instruction *const ir = (ir_instruction *) entry->key;
            if (v == NULL) {
      ADD_FAILURE() << "Invalid junk in hash table: ir_type = "
                           unsigned i;
   for (i = 0; i < count; i++) {
                     if (vars[i] == v)
               if (i == count) {
         ADD_FAILURE() << "Invalid variable in hash table: \""
   } else {
      /* As each variable is encountered, remove it from the set.  Don't
   * bother compacting the set because we don't care about
   * performance here.
   */
                  /* Check that there's nothing left in the set. */
   for (unsigned i = 0; i < count; i++) {
      if (vars[i] != NULL) {
      ADD_FAILURE() << "Variable was not in the hash table: \""
                     }
      TEST_F(array_refcount_test, ir_array_refcount_entry_initial_state_for_scalar)
   {
      ir_variable *const var =
                     ASSERT_NE((void *)0, get_bits(entry));
   EXPECT_FALSE(entry.is_referenced);
   EXPECT_EQ(1, get_num_bits(entry));
   EXPECT_EQ(0, get_array_depth(entry));
      }
      TEST_F(array_refcount_test, ir_array_refcount_entry_initial_state_for_vector)
   {
      ir_variable *const var =
                     ASSERT_NE((void *)0, get_bits(entry));
   EXPECT_FALSE(entry.is_referenced);
   EXPECT_EQ(1, get_num_bits(entry));
   EXPECT_EQ(0, get_array_depth(entry));
      }
      TEST_F(array_refcount_test, ir_array_refcount_entry_initial_state_for_matrix)
   {
      ir_variable *const var =
                     ASSERT_NE((void *)0, get_bits(entry));
   EXPECT_FALSE(entry.is_referenced);
   EXPECT_EQ(1, get_num_bits(entry));
   EXPECT_EQ(0, get_array_depth(entry));
      }
      TEST_F(array_refcount_test, ir_array_refcount_entry_initial_state_for_array)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                              ASSERT_NE((void *)0, get_bits(entry));
   EXPECT_FALSE(entry.is_referenced);
   EXPECT_EQ(total_elements, get_num_bits(entry));
            for (unsigned i = 0; i < total_elements; i++)
      }
      TEST_F(array_refcount_test, mark_array_elements_referenced_simple)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                              static const array_deref_range dr[] = {
         };
            link_util_mark_array_elements_referenced(dr, 3, entry.array_depth,
            for (unsigned i = 0; i < total_elements; i++)
      }
      TEST_F(array_refcount_test, mark_array_elements_referenced_whole_first_array)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                        static const array_deref_range dr[] = {
                  link_util_mark_array_elements_referenced(dr, 3, entry.array_depth,
            for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
               }
      TEST_F(array_refcount_test, mark_array_elements_referenced_whole_second_array)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                        static const array_deref_range dr[] = {
                  link_util_mark_array_elements_referenced(dr, 3, entry.array_depth,
            for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
               }
      TEST_F(array_refcount_test, mark_array_elements_referenced_whole_third_array)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                        static const array_deref_range dr[] = {
                  link_util_mark_array_elements_referenced(dr, 3, entry.array_depth,
            for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
               }
      TEST_F(array_refcount_test, mark_array_elements_referenced_whole_first_and_third_arrays)
   {
      ir_variable *const var =
      new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                        static const array_deref_range dr[] = {
                  link_util_mark_array_elements_referenced(dr, 3, entry.array_depth,
            for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
               }
      TEST_F(array_refcount_test, do_not_process_vector_indexing)
   {
      /* Vectors and matrices can also be indexed in much the same manner as
   * arrays.  The visitor should not try to track per-element accesses to
   * these types.
   */
   ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::float_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(glsl_type::int_type,
               ir_variable *var_c = new(mem_ctx) ir_variable(glsl_type::vec4_type,
                                             ir_array_refcount_entry *entry_a = v.get_variable_entry(var_a);
   ir_array_refcount_entry *entry_b = v.get_variable_entry(var_b);
            EXPECT_TRUE(entry_a->is_referenced);
   EXPECT_TRUE(entry_b->is_referenced);
            /* As validated by previous tests, for non-array types, num_bits is 1. */
   ASSERT_EQ(1, get_num_bits(*entry_c));
      }
      TEST_F(array_refcount_test, do_not_process_matrix_indexing)
   {
      /* Vectors and matrices can also be indexed in much the same manner as
   * arrays.  The visitor should not try to track per-element accesses to
   * these types.
   */
   ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::vec4_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(glsl_type::int_type,
               ir_variable *var_c = new(mem_ctx) ir_variable(glsl_type::mat4_type,
                                             ir_array_refcount_entry *entry_a = v.get_variable_entry(var_a);
   ir_array_refcount_entry *entry_b = v.get_variable_entry(var_b);
            EXPECT_TRUE(entry_a->is_referenced);
   EXPECT_TRUE(entry_b->is_referenced);
            /* As validated by previous tests, for non-array types, num_bits is 1. */
   ASSERT_EQ(1, get_num_bits(*entry_c));
      }
      TEST_F(array_refcount_test, do_not_process_array_inside_structure)
   {
      /* Structures can contain arrays.  The visitor should not try to track
   * per-element accesses to arrays contained inside structures.
   */
   const glsl_struct_field fields[] = {
                  const glsl_type *const record_of_array_3_of_int =
            ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::int_type,
                  ir_variable *var_b = new(mem_ctx) ir_variable(record_of_array_3_of_int,
                  /* a = b.i[2] */
   body->emit(assign(var_a,
                                          ir_array_refcount_entry *entry_a = v.get_variable_entry(var_a);
            EXPECT_TRUE(entry_a->is_referenced);
            ASSERT_EQ(1, get_num_bits(*entry_b));
               }
      TEST_F(array_refcount_test, visit_simple_indexing)
   {
      ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::vec4_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
                  /* a = b[2][1][0] */
   body->emit(assign(var_a,
                     deref_array(
                              const unsigned accessed_element = 0 + (1 * 5) + (2 * 4 * 5);
   ir_array_refcount_entry *entry_b = v.get_variable_entry(var_b);
            for (unsigned i = 0; i < total_elements; i++)
      EXPECT_EQ(i == accessed_element, entry_b->is_linearized_index_referenced(i)) <<
            }
      TEST_F(array_refcount_test, visit_whole_second_array_indexing)
   {
      ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::vec4_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
               ir_variable *var_i = new(mem_ctx) ir_variable(glsl_type::int_type,
                  /* a = b[2][i][1] */
   body->emit(assign(var_a,
                     deref_array(
                              ir_array_refcount_entry *const entry_b = v.get_variable_entry(var_b);
   for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
                              }
      TEST_F(array_refcount_test, visit_array_indexing_an_array)
   {
      ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::vec4_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(array_3_of_array_4_of_array_5_of_vec4,
               ir_variable *var_c = new(mem_ctx) ir_variable(array_3_of_int,
               ir_variable *var_i = new(mem_ctx) ir_variable(glsl_type::int_type,
                  /* a = b[2][3][c[i]] */
   body->emit(assign(var_a,
                     deref_array(
                                       for (unsigned i = 0; i < 3; i++) {
      for (unsigned j = 0; j < 4; j++) {
      for (unsigned k = 0; k < 5; k++) {
                     EXPECT_EQ(accessed,
            "array b[" << i << "][" << j << "][" << k << "], " <<
                           for (int i = 0; i < var_c->type->array_size(); i++) {
      EXPECT_EQ(true, entry_c->is_linearized_index_referenced(i)) <<
                  }
      TEST_F(array_refcount_test, visit_array_indexing_with_itself)
   {
      const glsl_type *const array_2_of_array_3_of_int =
            const glsl_type *const array_2_of_array_2_of_array_3_of_int =
            ir_variable *var_a = new(mem_ctx) ir_variable(glsl_type::int_type,
               ir_variable *var_b = new(mem_ctx) ir_variable(array_2_of_array_2_of_array_3_of_int,
                  /* Given GLSL code:
   *
   *    int b[2][2][3];
   *    a = b[ b[0][0][0] ][ b[ b[0][1][0] ][ b[1][0][0] ][1] ][2]
   *
   * b[0][0][0], b[0][1][0], and b[1][0][0] are trivially accessed.
   *
   * b[*][*][1] and b[*][*][2] are accessed.
   *
   * Only b[1][1][0] is not accessed.
   */
   operand b000 = deref_array(
      deref_array(
      deref_array(var_b, body->constant(int(0))),
            operand b010 = deref_array(
      deref_array(
      deref_array(var_b, body->constant(int(0))),
            operand b100 = deref_array(
      deref_array(
      deref_array(var_b, body->constant(int(1))),
            operand b_b010_b100_1 = deref_array(
      deref_array(
      deref_array(var_b, b010),
            body->emit(assign(var_a,
                     deref_array(
                                       for (unsigned i = 0; i < 2; i++) {
      for (unsigned j = 0; j < 2; j++) {
      for (unsigned k = 0; k < 3; k++) {
                     EXPECT_EQ(accessed,
            "array b[" << i << "][" << j << "][" << k << "], " <<
                     }
