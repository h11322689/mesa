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
   #include "ir_builder.h"
   #include "opt_add_neg_to_sub.h"
      using namespace ir_builder;
      class add_neg_to_sub : public ::testing::Test {
   public:
      virtual void SetUp();
            exec_list instructions;
   ir_factory *body;
   void *mem_ctx;
   ir_variable *var_a;
   ir_variable *var_b;
   ir_variable *var_c;
      };
      void
   add_neg_to_sub::SetUp()
   {
                        instructions.make_empty();
            var_a = new(mem_ctx) ir_variable(glsl_type::float_type,
                  var_b = new(mem_ctx) ir_variable(glsl_type::float_type,
                  var_c = new(mem_ctx) ir_variable(glsl_type::float_type,
            }
      void
   add_neg_to_sub::TearDown()
   {
      delete body;
            ralloc_free(mem_ctx);
               }
      TEST_F(add_neg_to_sub, a_plus_b)
   {
                                                   /* The resulting instruction should be 'c = a + b'. */
   ir_assignment *const assign = ir->as_assignment();
                     ir_expression *const expr = assign->rhs->as_expression();
   ASSERT_NE((void *)0, expr);
            ir_dereference_variable *const deref_a =
         ir_dereference_variable *const deref_b =
            ASSERT_NE((void *)0, deref_a);
   EXPECT_EQ(var_a, deref_a->var);
   ASSERT_NE((void *)0, deref_b);
      }
      TEST_F(add_neg_to_sub, a_plus_neg_b)
   {
                                                   /* The resulting instruction should be 'c = a - b'. */
   ir_assignment *const assign = ir->as_assignment();
                     ir_expression *const expr = assign->rhs->as_expression();
   ASSERT_NE((void *)0, expr);
            ir_dereference_variable *const deref_a =
         ir_dereference_variable *const deref_b =
            ASSERT_NE((void *)0, deref_a);
   EXPECT_EQ(var_a, deref_a->var);
   ASSERT_NE((void *)0, deref_b);
      }
      TEST_F(add_neg_to_sub, neg_a_plus_b)
   {
                                                   /* The resulting instruction should be 'c = b - a'. */
   ir_assignment *const assign = ir->as_assignment();
                     ir_expression *const expr = assign->rhs->as_expression();
   ASSERT_NE((void *)0, expr);
            ir_dereference_variable *const deref_b =
         ir_dereference_variable *const deref_a =
            ASSERT_NE((void *)0, deref_a);
   EXPECT_EQ(var_a, deref_a->var);
   ASSERT_NE((void *)0, deref_b);
      }
      TEST_F(add_neg_to_sub, neg_a_plus_neg_b)
   {
                                                   /* The resulting instruction should be 'c = -b - a'. */
   ir_assignment *const assign = ir->as_assignment();
                     ir_expression *const expr = assign->rhs->as_expression();
   ASSERT_NE((void *)0, expr);
            ir_expression *const neg_b = expr->operands[0]->as_expression();
   ir_dereference_variable *const deref_a =
            ASSERT_NE((void *)0, deref_a);
                     ir_dereference_variable *const deref_b =
            ASSERT_NE((void *)0, deref_b);
      }
