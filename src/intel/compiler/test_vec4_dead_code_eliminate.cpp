   /*
   * Copyright © 2018 Intel Corporation
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
      class dead_code_eliminate_vec4_test : public ::testing::Test {
      virtual void SetUp();
         public:
      struct brw_compiler *compiler;
   struct brw_compile_params params;
   struct intel_device_info *devinfo;
   void *ctx;
   struct gl_shader_program *shader_prog;
   struct brw_vue_prog_data *prog_data;
      };
      class dead_code_eliminate_vec4_visitor : public vec4_visitor
   {
   public:
      dead_code_eliminate_vec4_visitor(struct brw_compiler *compiler,
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
         void dead_code_eliminate_vec4_test::SetUp()
   {
      ctx = ralloc_context(NULL);
   compiler = rzalloc(ctx, struct brw_compiler);
   devinfo = rzalloc(ctx, struct intel_device_info);
            params = {};
            prog_data = ralloc(ctx, struct brw_vue_prog_data);
   nir_shader *shader =
         v = new dead_code_eliminate_vec4_visitor(compiler, &params, shader, prog_data);
         devinfo->ver = 4;
      }
      void dead_code_eliminate_vec4_test::TearDown()
   {
      delete v;
            ralloc_free(ctx);
      }
      static void
   dead_code_eliminate(vec4_visitor *v)
   {
               if (print) {
      fprintf(stderr, "instructions before:\n");
               v->calculate_cfg();
            if (print) {
      fprintf(stderr, "instructions after:\n");
         }
      TEST_F(dead_code_eliminate_vec4_test, some_dead_channels_all_flags_used)
   {
      const vec4_builder bld = vec4_builder(v).at_end();
   src_reg r1 = src_reg(v, glsl_type::vec4_type);
   src_reg r2 = src_reg(v, glsl_type::vec4_type);
   src_reg r3 = src_reg(v, glsl_type::vec4_type);
   src_reg r4 = src_reg(v, glsl_type::vec4_type);
   src_reg r5 = src_reg(v, glsl_type::vec4_type);
            /* Sequence like the following should not be modified by DCE.
   *
   *     cmp.l.f0(8)     g4<1>F         g2<4,4,1>.wF   g1<4,4,1>.xF
   *     mov(8)          g5<1>.xF       g4<4,4,1>.xF
   *     (+f0.x) sel(8)  g6<1>UD        g3<4>UD        g6<4>UD
   */
   vec4_instruction *test_cmp =
            test_cmp->src[0].swizzle = BRW_SWIZZLE_WWWW;
            vec4_instruction *test_mov =
            test_mov->dst.writemask = WRITEMASK_X;
            vec4_instruction *test_sel =
                     /* The scratch write is here just to make r5 and r6 be live so that the
   * whole program doesn't get eliminated by DCE.
   */
                        }
