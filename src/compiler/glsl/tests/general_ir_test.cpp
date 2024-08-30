   /*
   * Copyright © 2013 Intel Corporation
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
   #include "util/compiler.h"
   #include "main/macros.h"
   #include "ir.h"
      class ir_variable_constructor : public ::testing::Test {
   public:
      virtual void SetUp();
      };
      void
   ir_variable_constructor::SetUp()
   {
         }
      void
   ir_variable_constructor::TearDown()
   {
         }
      TEST_F(ir_variable_constructor, interface)
   {
               static const glsl_struct_field f[] = {
                  const glsl_type *const iface =
      glsl_type::get_interface_instance(f,
                                    ir_variable *const v =
            EXPECT_STREQ(name, v->name);
   EXPECT_NE(name, v->name);
   EXPECT_EQ(iface, v->type);
               }
      TEST_F(ir_variable_constructor, interface_array)
   {
               static const glsl_struct_field f[] = {
                  const glsl_type *const iface =
      glsl_type::get_interface_instance(f,
                           const glsl_type *const interface_array =
                     ir_variable *const v =
            EXPECT_STREQ(name, v->name);
   EXPECT_NE(name, v->name);
   EXPECT_EQ(interface_array, v->type);
               }
