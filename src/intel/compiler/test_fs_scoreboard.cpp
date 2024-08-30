   /*
   * Copyright © 2019 Intel Corporation
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
   #include "brw_fs.h"
   #include "brw_cfg.h"
      using namespace brw;
      class scoreboard_test : public ::testing::Test {
      virtual void SetUp();
         public:
      struct brw_compiler *compiler;
   struct brw_compile_params params;
   struct intel_device_info *devinfo;
   void *ctx;
   struct brw_wm_prog_data *prog_data;
   struct gl_shader_program *shader_prog;
      };
      void scoreboard_test::SetUp()
   {
      ctx = ralloc_context(NULL);
   compiler = rzalloc(ctx, struct brw_compiler);
   devinfo = rzalloc(ctx, struct intel_device_info);
   devinfo->ver = 12;
            compiler->devinfo = devinfo;
            params = {};
            prog_data = ralloc(ctx, struct brw_wm_prog_data);
   nir_shader *shader =
            v = new fs_visitor(compiler, &params, NULL, &prog_data->base, shader, 8,
         }
      void scoreboard_test::TearDown()
   {
      delete v;
            ralloc_free(ctx);
      }
      static fs_inst *
   instruction(bblock_t *block, int num)
   {
      fs_inst *inst = (fs_inst *)block->start();
   for (int i = 0; i < num; i++) {
         }
      }
      static void
   lower_scoreboard(fs_visitor *v)
   {
               if (print) {
      fprintf(stderr, "= Before =\n");
                        if (print) {
      fprintf(stderr, "\n= After =\n");
         }
      fs_inst *
   emit_SEND(const fs_builder &bld, const fs_reg &dst,
         {
      fs_inst *inst = bld.emit(SHADER_OPCODE_SEND, dst, desc, desc, payload);
   inst->mlen = 1;
      }
      static tgl_swsb
   tgl_swsb_testcase(unsigned regdist, unsigned sbid, enum tgl_sbid_mode mode)
   {
      tgl_swsb swsb = tgl_swsb_sbid(mode, sbid);
   swsb.regdist = regdist;
      }
      bool operator ==(const tgl_swsb &a, const tgl_swsb &b)
   {
      return a.mode == b.mode &&
            }
      std::ostream &operator<<(std::ostream &os, const tgl_swsb &swsb) {
      if (swsb.regdist)
            if (swsb.mode) {
      if (swsb.regdist)
         os << "$" << swsb.sbid;
   if (swsb.mode & TGL_SBID_DST)
         if (swsb.mode & TGL_SBID_SRC)
                  }
      TEST_F(scoreboard_test, RAW_inorder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   fs_reg y = v->vgrf(glsl_type::int_type);
   bld.ADD(   x, g[1], g[2]);
   bld.MUL(   y, g[3], g[4]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, RAW_inorder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.ADD(          x, g[1], g[2]);
   bld.MUL(       g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, RAW_outoforder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   fs_reg y = v->vgrf(glsl_type::int_type);
   emit_SEND(bld,    x, g[1], g[2]);
   bld.MUL(          y, g[3], g[4]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_sbid(TGL_SBID_SET, 0));
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, RAW_outoforder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            /* The second SEND depends on the first, and would need to refer to two
   * SBIDs.  Since it is not possible we expect a SYNC instruction to be
   * added.
   */
   fs_reg x = v->vgrf(glsl_type::int_type);
   emit_SEND(bld,    x, g[1], g[2]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
                     fs_inst *sync = instruction(block0, 1);
   EXPECT_EQ(sync->opcode, BRW_OPCODE_SYNC);
               }
      TEST_F(scoreboard_test, WAR_inorder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.ADD(g[1],    x, g[2]);
   bld.MUL(g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, WAR_inorder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.ADD(       g[1],    x, g[2]);
   bld.MUL(       g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, WAR_outoforder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   emit_SEND(bld, g[1], g[2],    x);
   bld.MUL(       g[4], g[5], g[6]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_sbid(TGL_SBID_SET, 0));
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, WAR_outoforder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   emit_SEND(bld, g[1], g[2],    x);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
                     fs_inst *sync = instruction(block0, 1);
   EXPECT_EQ(sync->opcode, BRW_OPCODE_SYNC);
               }
      TEST_F(scoreboard_test, WAW_inorder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.ADD(   x, g[1], g[2]);
   bld.MUL(g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
            /* NOTE: We only need this RegDist if a long instruction is followed by a
   * short one.  The pass is currently conservative about this and adding the
   * annotation.
   */
      }
      TEST_F(scoreboard_test, WAW_inorder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.ADD(          x, g[1], g[2]);
   bld.MUL(       g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, WAW_outoforder_inorder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   emit_SEND(bld,    x, g[1], g[2]);
   bld.MUL(       g[3], g[4], g[5]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_sbid(TGL_SBID_SET, 0));
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
      TEST_F(scoreboard_test, WAW_outoforder_outoforder)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   emit_SEND(bld, x, g[1], g[2]);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
                     fs_inst *sync = instruction(block0, 1);
   EXPECT_EQ(sync->opcode, BRW_OPCODE_SYNC);
               }
         TEST_F(scoreboard_test, loop1)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
                     bld.ADD(   x, g[1], g[2]);
                     v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[2];
   fs_inst *add = instruction(body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 0);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, loop2)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
                     bld.ADD(   x, g[1], g[2]);
                     v->calculate_cfg();
                     bblock_t *body = v->cfg->blocks[2];
   fs_inst *add = instruction(body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 0);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, loop3)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
                     /* For the ADD in the loop body this extra distance will always apply. */
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
            bld.ADD(   x, g[1], g[2]);
                     v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[2];
   fs_inst *add = instruction(body, 4);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 0);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
         TEST_F(scoreboard_test, conditional1)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
                     bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[1];
   fs_inst *add = instruction(body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[2];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional2)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
                     bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[1];
   fs_inst *add = instruction(body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[2];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional3)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
            bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
            bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[1];
   fs_inst *add = instruction(body, 3);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[2];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional4)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
            bld.ADD(   x, g[1], g[2]);
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
            bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *body = v->cfg->blocks[1];
   fs_inst *add = instruction(body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *last_block = v->cfg->blocks[2];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional5)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
            bld.ADD(   x, g[1], g[2]);
                     bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *then_body = v->cfg->blocks[1];
   fs_inst *add = instruction(then_body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *else_body = v->cfg->blocks[2];
   fs_inst *rol = instruction(else_body, 0);
   EXPECT_EQ(rol->opcode, BRW_OPCODE_ROL);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional6)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
            bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
   bld.ADD(   x, g[1], g[2]);
            bld.XOR(g[6], g[1], g[2]);
   bld.XOR(g[7], g[1], g[2]);
   bld.XOR(g[8], g[1], g[2]);
   bld.XOR(g[9], g[1], g[2]);
            bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *then_body = v->cfg->blocks[1];
   fs_inst *add = instruction(then_body, 3);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *else_body = v->cfg->blocks[2];
   fs_inst *rol = instruction(else_body, 4);
   EXPECT_EQ(rol->opcode, BRW_OPCODE_ROL);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional7)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
            bld.ADD(   x, g[1], g[2]);
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
            bld.ROL(   x, g[1], g[2]);
   bld.XOR(g[6], g[1], g[2]);
   bld.XOR(g[7], g[1], g[2]);
   bld.XOR(g[8], g[1], g[2]);
            bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *then_body = v->cfg->blocks[1];
   fs_inst *add = instruction(then_body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            bblock_t *else_body = v->cfg->blocks[2];
   fs_inst *rol = instruction(else_body, 0);
   EXPECT_EQ(rol->opcode, BRW_OPCODE_ROL);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, conditional8)
   {
      const fs_builder &bld = v->bld;
   fs_reg g[16];
   for (unsigned i = 0; i < ARRAY_SIZE(g); i++)
            fs_reg x = v->vgrf(glsl_type::int_type);
   bld.XOR(   x, g[1], g[2]);
   bld.XOR(g[3], g[1], g[2]);
   bld.XOR(g[4], g[1], g[2]);
   bld.XOR(g[5], g[1], g[2]);
   bld.XOR(g[6], g[1], g[2]);
   bld.XOR(g[7], g[1], g[2]);
            bld.ADD(   x, g[1], g[2]);
                     bld.emit(BRW_OPCODE_ENDIF);
            v->calculate_cfg();
            bblock_t *then_body = v->cfg->blocks[1];
   fs_inst *add = instruction(then_body, 0);
   EXPECT_EQ(add->opcode, BRW_OPCODE_ADD);
            /* Note that the ROL will have RegDist 2 and not 7, illustrating the
   * physical CFG edge between the then-block and the else-block.
   */
   bblock_t *else_body = v->cfg->blocks[2];
   fs_inst *rol = instruction(else_body, 0);
   EXPECT_EQ(rol->opcode, BRW_OPCODE_ROL);
            bblock_t *last_block = v->cfg->blocks[3];
   fs_inst *mul = instruction(last_block, 1);
   EXPECT_EQ(mul->opcode, BRW_OPCODE_MUL);
      }
      TEST_F(scoreboard_test, gfx125_RaR_over_different_pipes)
   {
      devinfo->verx10 = 125;
                     fs_reg a = v->vgrf(glsl_type::int_type);
   fs_reg b = v->vgrf(glsl_type::int_type);
   fs_reg f = v->vgrf(glsl_type::float_type);
            bld.ADD(f, x, x);
   bld.ADD(a, x, x);
            v->calculate_cfg();
   bblock_t *block0 = v->cfg->blocks[0];
   ASSERT_EQ(0, block0->start_ip);
            lower_scoreboard(v);
   ASSERT_EQ(0, block0->start_ip);
            EXPECT_EQ(instruction(block0, 0)->sched, tgl_swsb_null());
   EXPECT_EQ(instruction(block0, 1)->sched, tgl_swsb_null());
      }
