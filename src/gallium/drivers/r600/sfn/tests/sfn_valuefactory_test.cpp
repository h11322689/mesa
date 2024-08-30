   /* -*- mesa-c++  -*-
   *
   * Copyright (c) 2021 Collabora LTD
   *
   * Author: Gert Wollny <gert.wollny@collabora.com>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "../sfn_alu_defines.h"
   #include "../sfn_debug.h"
   #include "../sfn_valuefactory.h"
   #include "nir_builder.h"
   #include "util/ralloc.h"
      #include "gtest/gtest.h"
      using namespace r600;
      class ValuefactoryTest : public ::testing::Test {
      public:
            protected:
      void SetUp() override;
            ValueFactory *factory;
   nir_builder b;
      };
      TEST_F(ValuefactoryTest, test_create_ssa)
   {
      auto c1 = nir_imm_float(&b, 2.0);
   auto c2 = nir_imm_float(&b, 4.0);
   auto sum = nir_fadd(&b, c1, c2);
            sfn_log << SfnLog::reg << "Search (test) " << &alu->def << "\n";
   auto dest_value = factory->dest(alu->def, 0, pin_none);
   EXPECT_EQ(dest_value->sel(), 1024);
   EXPECT_EQ(dest_value->chan(), 0);
            nir_src src = nir_src_for_ssa(sum);
   sfn_log << SfnLog::reg << "Search (test) " << &src << "\n";
   PVirtualValue value = factory->src(src, 0);
   EXPECT_EQ(value->sel(), 1024);
   EXPECT_EQ(value->chan(), 0);
      }
      TEST_F(ValuefactoryTest, test_create_ssa_pinned_chan)
   {
      auto c1 = nir_imm_float(&b, 2.0);
   auto c2 = nir_imm_float(&b, 4.0);
   auto sum = nir_fadd(&b, c1, c2);
            auto dest_value = factory->dest(alu->def, 0, pin_chan);
   EXPECT_EQ(dest_value->sel(), 1024);
   EXPECT_EQ(dest_value->chan(), 0);
            PVirtualValue value = factory->src(nir_src_for_ssa(sum), 0);
   EXPECT_EQ(value->sel(), 1024);
   EXPECT_EQ(value->chan(), 0);
      }
      TEST_F(ValuefactoryTest, test_create_ssa_pinned_chan_and_reg)
   {
      auto c1 = nir_imm_float(&b, 2.0);
   auto c2 = nir_imm_float(&b, 4.0);
   auto sum = nir_fadd(&b, c1, c2);
            auto dest_value = factory->dest(alu->def, 1, pin_chan);
   EXPECT_EQ(dest_value->sel(), 1024);
   EXPECT_EQ(dest_value->chan(), 1);
            PVirtualValue value = factory->src(nir_src_for_ssa(sum), 1);
   EXPECT_EQ(value->sel(), 1024);
   EXPECT_EQ(value->chan(), 1);
      }
      TEST_F(ValuefactoryTest, test_create_const)
   {
      auto c1 = nir_imm_int(&b, 2);
   auto c2 = nir_imm_int(&b, 4);
            auto ci1 = nir_instr_as_load_const(c1->parent_instr);
            auto ci2 = nir_instr_as_load_const(c2->parent_instr);
                     PVirtualValue value1 = factory->src(alu->src[0], 0);
            const auto* cvalue1 = value1->as_literal();
            ASSERT_TRUE(cvalue1);
            EXPECT_EQ(cvalue1->value(), 2);
      }
      TEST_F(ValuefactoryTest, test_create_sysvalue)
   {
               EXPECT_EQ(ic->sel(), ALU_SRC_TIME_LO);
      }
      class GetKCache : public ConstRegisterVisitor {
   public:
      void visit(const VirtualValue& value) { (void)value; }
   void visit(const Register& value) { (void)value; };
   void visit(const LocalArray& value) { (void)value; }
   void visit(const LocalArrayValue& value) { (void)value; }
   void visit(const UniformValue& value)
   {
      (void)value;
      }
   void visit(const LiteralConstant& value) { (void)value; }
            GetKCache():
         {
               };
      ValuefactoryTest::ValuefactoryTest()
   {
      memset(&options, 0, sizeof(options));
      }
      void
   ValuefactoryTest::SetUp()
   {
      glsl_type_singleton_init_or_ref();
   b = nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, &options, "test shader");
      }
      void
   ValuefactoryTest::TearDown()
   {
      ralloc_free(b.shader);
   glsl_type_singleton_decref();
      }
