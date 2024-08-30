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
   */
      #include <gtest/gtest.h>
   #include "brw_vec4.h"
      using namespace brw;
      #define register_coalesce(v) _register_coalesce(v, __func__)
      class register_coalesce_vec4_test : public ::testing::Test {
      virtual void SetUp();
         public:
      struct brw_compiler *compiler;
   struct brw_compile_params params;
   struct intel_device_info *devinfo;
   void *ctx;
   struct gl_shader_program *shader_prog;
   struct brw_vue_prog_data *prog_data;
      };
         class register_coalesce_vec4_visitor : public vec4_visitor
   {
   public:
      register_coalesce_vec4_visitor(struct brw_compiler *compiler,
                        : vec4_visitor(compiler, params, NULL, prog_data, shader,
      {
               protected:
      virtual dst_reg *make_reg_for_system_value(int /* location */)
   {
                  virtual void setup_payload()
   {
                  virtual void emit_prolog()
   {
                  virtual void emit_thread_end()
   {
                  virtual void emit_urb_write_header(int /* mrf */)
   {
                  virtual vec4_instruction *emit_urb_write_opcode(bool /* complete */)
   {
            };
         void register_coalesce_vec4_test::SetUp()
   {
      ctx = ralloc_context(NULL);
   compiler = rzalloc(ctx, struct brw_compiler);
   devinfo = rzalloc(ctx, struct intel_device_info);
                     params = {};
            nir_shader *shader =
                     devinfo->ver = 4;
      }
      void register_coalesce_vec4_test::TearDown()
   {
      delete v;
            ralloc_free(ctx);
      }
      static void
   _register_coalesce(vec4_visitor *v, const char *func)
   {
               if (print) {
      printf("%s: instructions before:\n", func);
               v->calculate_cfg();
            if (print) {
      printf("%s: instructions after:\n", func);
         }
      TEST_F(register_coalesce_vec4_test, test_compute_to_mrf)
   {
      src_reg something = src_reg(v, glsl_type::float_type);
   dst_reg temp = dst_reg(v, glsl_type::float_type);
            dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_X;
            vec4_instruction *mul = v->emit(v->MUL(temp, something, brw_imm_f(1.0f)));
                        }
         TEST_F(register_coalesce_vec4_test, test_multiple_use)
   {
      src_reg something = src_reg(v, glsl_type::float_type);
   dst_reg temp = dst_reg(v, glsl_type::vec4_type);
            dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_X;
            dst_reg m1 = dst_reg(MRF, 1);
   m1.writemask = WRITEMASK_XYZW;
            src_reg src = src_reg(temp);
   vec4_instruction *mul = v->emit(v->MUL(temp, something, brw_imm_f(1.0f)));
   src.swizzle = BRW_SWIZZLE_XXXX;
   v->emit(v->MOV(m0, src));
   src.swizzle = BRW_SWIZZLE_XYZW;
                        }
      TEST_F(register_coalesce_vec4_test, test_dp4_mrf)
   {
      src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
            dst_reg m0 = dst_reg(MRF, 0);
   m0.writemask = WRITEMASK_Y;
                     vec4_instruction *dp4 = v->emit(v->DP4(temp, some_src_1, some_src_2));
                     EXPECT_EQ(dp4->dst.file, MRF);
      }
      TEST_F(register_coalesce_vec4_test, test_dp4_grf)
   {
      src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
            dst_reg to = dst_reg(v, glsl_type::vec4_type);
            vec4_instruction *dp4 = v->emit(v->DP4(temp, some_src_1, some_src_2));
   to.writemask = WRITEMASK_Y;
            /* if we don't do something with the result, the automatic dead code
   * elimination will remove all our instructions.
   */
   src_reg src = src_reg(to);
   src.negate = true;
                     EXPECT_EQ(dp4->dst.nr, to.nr);
      }
      TEST_F(register_coalesce_vec4_test, test_channel_mul_grf)
   {
      src_reg some_src_1 = src_reg(v, glsl_type::vec4_type);
   src_reg some_src_2 = src_reg(v, glsl_type::vec4_type);
            dst_reg to = dst_reg(v, glsl_type::vec4_type);
            vec4_instruction *mul = v->emit(v->MUL(temp, some_src_1, some_src_2));
   to.writemask = WRITEMASK_Y;
            /* if we don't do something with the result, the automatic dead code
   * elimination will remove all our instructions.
   */
   src_reg src = src_reg(to);
   src.negate = true;
                        }
