   /*
   * Copyright © 2021 Valve Corporation
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
   *
   */
      #include "helpers.h"
      using namespace aco;
      BEGIN_TEST(optimizer_postRA.vcmp)
      PhysReg reg_v0(256);
   PhysReg reg_s0(0);
   PhysReg reg_s2(2);
            //>> v1: %a:v[0] = p_startpgm
   ASSERTED bool setup_ok = setup_cs("v1", GFX8);
            auto& startpgm = bld.instructions->at(0);
   assert(startpgm->opcode == aco_opcode::p_startpgm);
                     {
               //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
   //! s2: %e:s[2-3] = p_cbranch_z %b:vcc
   //! p_unit_test 0, %e:s[2-3]
   auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp),
         auto br =
                              {
               //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
   //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
   //! s2: %f:vcc = s_mov_b64 0
   //! s2: %e:s[2-3] = p_cbranch_z %d:scc
   //! p_unit_test 1, %e:s[2-3], %f:vcc
   auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp),
         auto ovrwr = bld.sop1(Builder::s_mov, bld.def(bld.lm, vcc), Operand::zero());
   auto br =
                              {
               //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
   //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
   //! s1: %f:s[107] = s_mov_b32 0
   //! s2: %e:s[2-3] = p_cbranch_z %d:scc
   //! p_unit_test 1, %e:s[2-3], %f:vcc
   auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp),
         auto ovrwr = bld.sop1(aco_opcode::s_mov_b32, bld.def(s1, vcc_hi), Operand::zero());
   auto br =
                              {
               //! s2: %b:s[4-5] = v_cmp_eq_u32 0, %a:v[0]
   //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:s[4-5], %x:exec
   //! s2: %e:s[2-3] = p_cbranch_z %d:scc
   //! p_unit_test 2, %e:s[2-3]
   auto vcmp = bld.vopc_e64(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, reg_s4), Operand::zero(),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc),
         auto br =
                              {
               //! s2: %b:vcc, s1: %f:scc = s_or_b64 1, %0:s[4-5]
   //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
   //! s2: %e:s[2-3] = p_cbranch_z %d:scc
   //! p_unit_test 2, %e:s[2-3]
   auto salu = bld.sop2(Builder::s_or, bld.def(bld.lm, vcc), bld.def(s1, scc), Operand::c32(1u),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc),
         auto br =
                              {
               //! s2: %b:vcc = v_cmp_eq_u32 0, %a:v[0]
   //! s2: %c:s[0-1], s1: %d:scc = s_and_b64 %b:vcc, %x:exec
   //! s2: %f:exec = s_mov_b64 42
   //! s2: %e:s[2-3] = p_cbranch_z %d:scc
   //! p_unit_test 4, %e:s[2-3], %f:exec
   auto vcmp = bld.vopc(aco_opcode::v_cmp_eq_u32, bld.def(bld.lm, vcc), Operand::zero(),
         auto sand = bld.sop2(Builder::s_and, bld.def(bld.lm, reg_s0), bld.def(s1, scc), bld.vcc(vcmp),
         auto ovrwr = bld.sop1(Builder::s_mov, bld.def(bld.lm, exec), Operand::c32(42u));
   auto br =
                                 END_TEST
      BEGIN_TEST(optimizer_postRA.scc_nocmp_opt)
      //>> s1: %a, s2: %y, s1: %z = p_startpgm
   ASSERTED bool setup_ok = setup_cs("s1 s2 s1", GFX6);
            PhysReg reg_s0{0};
   PhysReg reg_s2{2};
   PhysReg reg_s3{3};
   PhysReg reg_s4{4};
   PhysReg reg_s6{6};
            Temp in_0 = inputs[0];
   Temp in_1 = inputs[1];
   Temp in_2 = inputs[2];
   Operand op_in_0(in_0);
   op_in_0.setFixed(reg_s0);
   Operand op_in_1(in_1);
   op_in_1.setFixed(reg_s4);
   Operand op_in_2(in_2);
            {
      //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s2: %f:vcc = p_cbranch_nz %e:scc
   //! p_unit_test 0, %f:vcc
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
                        {
      //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s2: %f:vcc = p_cbranch_z %e:scc
   //! p_unit_test 1, %f:vcc
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
                        {
      //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s2: %f:vcc = p_cbranch_z %e:scc
   //! p_unit_test 2, %f:vcc
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
                        {
      //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s2: %f:vcc = p_cbranch_nz %e:scc
   //! p_unit_test 3, %f:vcc
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
                        {
      //! s2: %d:s[2-3], s1: %e:scc = s_and_b64 %y:s[4-5], 0x12345
   //! s2: %f:vcc = p_cbranch_z %e:scc
   //! p_unit_test 4, %f:vcc
   auto salu = bld.sop2(aco_opcode::s_and_b64, bld.def(s2, reg_s2), bld.def(s1, scc), op_in_1,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u64, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_nz, bld.def(s2, vcc), bld.scc(scmp));
                        {
               //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
   //! s1: %g:scc = s_cmp_eq_u32 %d:s[2], 0
   //! s2: %f:vcc = p_cbranch_z %g:scc
   //! p_unit_test 5, %f:vcc, %h:s[3]
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
                        {
               //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
   //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s2: %f:vcc = p_cbranch_z %g:scc
   //! p_unit_test 5, %f:vcc, %h:s[3]
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
                        {
               //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
   //! s2: %d:s[8-9], s1: %e:scc = s_and_b64 %b:s[4-5], 0x40018
   //! s2: %f:vcc = p_cbranch_z %g:scc
   //! p_unit_test 5, %f:vcc, %h:s[3]
   auto salu = bld.sop2(aco_opcode::s_and_b64, bld.def(s2, reg_s8), bld.def(s1, scc), op_in_1,
         auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(salu, reg_s8),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
                        {
      //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s1: %f:s[4] = s_cselect_b32 %z:s[6], %a:s[0], %e:scc
   //! p_unit_test 6, %f:s[4]
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.sop2(aco_opcode::s_cselect_b32, bld.def(s1, reg_s4), Operand(op_in_0),
                              {
               //! s1: %d:s[2], s1: %e:scc = s_bfe_u32 %a:s[0], 0x40018
   //! s1: %h:s[3], s1: %x:scc = s_add_u32 %a:s[0], 1
   //! s1: %g:scc = s_cmp_eq_u32 %d:s[2], 0
   //! s1: %f:s[4] = s_cselect_b32 %a:s[0], %z:s[6], %g:scc
   //! p_unit_test 7, %f:s[4], %h:s[3]
   auto salu = bld.sop2(aco_opcode::s_bfe_u32, bld.def(s1, reg_s2), bld.def(s1, scc), op_in_0,
         auto ovrw = bld.sop2(aco_opcode::s_add_u32, bld.def(s1, reg_s3), bld.def(s1, scc), op_in_0,
         auto scmp = bld.sopc(aco_opcode::s_cmp_eq_u32, bld.def(s1, scc), Operand(salu, reg_s2),
         auto br = bld.sop2(aco_opcode::s_cselect_b32, bld.def(s1, reg_s4), Operand(op_in_0),
                                 END_TEST
      BEGIN_TEST(optimizer_postRA.dpp)
      //>> v1: %a:v[0], v1: %b:v[1], s2: %c:vcc, s2: %d:s[0-1] = p_startpgm
   if (!setup_cs("v1 v1 s2 s2", GFX10_3))
            bld.instructions->at(0)->definitions[0].setFixed(PhysReg(256));
   bld.instructions->at(0)->definitions[1].setFixed(PhysReg(257));
   bld.instructions->at(0)->definitions[2].setFixed(vcc);
            PhysReg reg_v0(256);
   PhysReg reg_v2(258);
   Operand a(inputs[0], PhysReg(256));
   Operand b(inputs[1], PhysReg(257));
   Operand c(inputs[2], vcc);
            /* basic optimization */
   //! v1: %res0:v[2] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 0, %res0:v[2]
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res0 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp0, reg_v2), b);
            /* operand swapping */
   //! v1: %res1:v[2] = v_subrev_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 1, %res1:v[2]
   Temp tmp1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res1 = bld.vop2(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp1, reg_v2));
            //! v1: %tmp2:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %res2:v[2] = v_sub_f32 %b:v[1], %tmp2:v[2] row_half_mirror bound_ctrl:1 fi
   //! p_unit_test 2, %res2:v[2]
   Temp tmp2 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res2 = bld.vop2_dpp(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp2, reg_v2),
                  /* modifiers */
   //! v1: %res3:v[2] = v_add_f32 -%a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 3, %res3:v[2]
   auto tmp3 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp3->dpp16().neg[0] = true;
   Temp res3 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp3, reg_v2), b);
            //! v1: %res4:v[2] = v_add_f32 -%a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 4, %res4:v[2]
   Temp tmp4 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res4 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp4, reg_v2), b);
   res4->valu().neg[0] = true;
            //! v1: %tmp5:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %res5:v[2] = v_add_f32 %tmp5:v[2], %b:v[1] clamp
   //! p_unit_test 5, %res5:v[2]
   Temp tmp5 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res5 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp5, reg_v2), b);
   res5->valu().clamp = true;
            //! v1: %res6:v[2] = v_add_f32 |%a:v[0]|, %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 6, %res6:v[2]
   auto tmp6 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp6->dpp16().neg[0] = true;
   auto res6 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp6, reg_v2), b);
   res6->valu().abs[0] = true;
            //! v1: %res7:v[2] = v_subrev_f32 %a:v[0], |%b:v[1]| row_mirror bound_ctrl:1 fi
   //! p_unit_test 7, %res7:v[2]
   Temp tmp7 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   auto res7 = bld.vop2_e64(aco_opcode::v_sub_f32, bld.def(v1, reg_v2), b, Operand(tmp7, reg_v2));
   res7->valu().abs[0] = true;
            //! v1: %tmp12:v[2] = v_mov_b32 -%a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %res12:v[2] = v_add_u32 %tmp12:v[2], %b:v[1]
   //! p_unit_test 12, %res12:v[2]
   auto tmp12 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp12->dpp16().neg[0] = true;
   Temp res12 = bld.vop2(aco_opcode::v_add_u32, bld.def(v1, reg_v2), Operand(tmp12, reg_v2), b);
            //! v1: %tmp13:v[2] = v_mov_b32 -%a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %res13:v[2] = v_add_f16 %tmp13:v[2], %b:v[1]
   //! p_unit_test 13, %res13:v[2]
   auto tmp13 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   tmp13->dpp16().neg[0] = true;
   Temp res13 = bld.vop2(aco_opcode::v_add_f16, bld.def(v1, reg_v2), Operand(tmp13, reg_v2), b);
            /* vcc */
   //! v1: %res8:v[2] = v_cndmask_b32 %a:v[0], %b:v[1], %c:vcc row_mirror bound_ctrl:1 fi
   //! p_unit_test 8, %res8:v[2]
   Temp tmp8 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res8 =
                  //! v1: %tmp9:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %res9:v[2] = v_cndmask_b32 %tmp9:v[2], %b:v[1], %d:s[0-1]
   //! p_unit_test 9, %res9:v[2]
   Temp tmp9 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res9 =
                  /* control flow */
   //! BB1
   //! /* logical preds: BB0, / linear preds: BB0, / kind: uniform, */
   //! v1: %res10:v[2] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 10, %res10:v[2]
            bld.reset(program->create_and_insert_block());
   program->blocks[0].linear_succs.push_back(1);
   program->blocks[0].logical_succs.push_back(1);
   program->blocks[1].linear_preds.push_back(0);
            Temp res10 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp10, reg_v2), b);
            /* can't combine if the v_mov_b32's operand is modified */
   //! v1: %tmp11_1:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
   //! v1: %tmp11_2:v[0] = v_mov_b32 0
   //! v1: %res11:v[2] = v_add_f32 %tmp11_1:v[2], %b:v[1]
   //! p_unit_test 11, %res11_1:v[2], %tmp11_2:v[0]
   Temp tmp11_1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp tmp11_2 = bld.vop1(aco_opcode::v_mov_b32, bld.def(v1, reg_v0), Operand::c32(0));
   Temp res11 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp11_1, reg_v2), b);
               END_TEST
      BEGIN_TEST(optimizer_postRA.dpp_across_exec)
      for (amd_gfx_level gfx : {GFX9, GFX10}) {
      //>> v1: %a:v[0], v1: %b:v[1] = p_startpgm
   if (!setup_cs("v1 v1", gfx))
            bld.instructions->at(0)->definitions[0].setFixed(PhysReg(256));
            PhysReg reg_v2(258);
   Operand a(inputs[0], PhysReg(256));
            //~gfx9! v1: %tmp0:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1
   //! s2: %0:exec,  s1: %0:scc = s_not_b64 %0:exec
   //~gfx9! v1: %res0:v[2] = v_add_f32 %tmp0:v[2], %b:v[1]
   //~gfx10! v1: %res0:v[2] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 0, %res0:v[2]
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   bld.sop1(Builder::s_not, Definition(exec, bld.lm), Definition(scc, s1),
         Temp res0 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1, reg_v2), Operand(tmp0, reg_v2), b);
                  END_TEST
      BEGIN_TEST(optimizer_postRA.dpp_vcmpx)
      //>> v1: %a:v[0], v1: %b:v[1] = p_startpgm
   if (!setup_cs("v1 v1", GFX11))
            bld.instructions->at(0)->definitions[0].setFixed(PhysReg(256));
            PhysReg reg_v2(258);
   Operand a(inputs[0], PhysReg(256));
            //! v1: %tmp0:v[2] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
   //! s2: %res0:exec = v_cmpx_lt_f32 %tmp0:v[2], %b:v[1]
   //! p_unit_test 0, %res0:exec
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1, reg_v2), a, dpp_row_mirror);
   Temp res0 = bld.vopc(aco_opcode::v_cmpx_lt_f32, bld.def(bld.lm, exec), Operand(tmp0, reg_v2), b);
               END_TEST
      BEGIN_TEST(optimizer_postRA.dpp_across_cf)
      //>> v1: %a:v[0], v1: %b:v[1], v1: %c:v[2], v1: %d:v[3], s2: %e:s[0-1] = p_startpgm
   if (!setup_cs("v1 v1 v1 v1 s2", GFX10_3))
            aco_ptr<Instruction>& startpgm = bld.instructions->at(0);
   startpgm->definitions[0].setFixed(PhysReg(256));
   startpgm->definitions[1].setFixed(PhysReg(257));
   startpgm->definitions[2].setFixed(PhysReg(258));
   startpgm->definitions[3].setFixed(PhysReg(259));
            Operand a(inputs[0], PhysReg(256)); /* source for DPP */
   Operand b(inputs[1], PhysReg(257)); /* source for fadd */
   Operand c(inputs[2], PhysReg(258)); /* buffer store address */
   Operand d(inputs[3], PhysReg(259)); /* buffer store value */
   Operand e(inputs[4], PhysReg(0));   /* condition */
                     //! s2: %saved_exec:s[84-85],  s1: %0:scc,  s2: %0:exec = s_and_saveexec_b64 %e:s[0-1], %0:exec
            emit_divergent_if_else(
      program.get(), bld, e,
   [&]() -> void
   {
      /* --- logical then --- */
   //! BB1
                                 //! v1: %res10:v[12] = v_add_f32 %a:v[0], %b:v[1] row_mirror bound_ctrl:1 fi
   //! p_unit_test 10, %res10:v[12]
   Temp result =
                                 /* --- linear then --- */
   //! BB2
                  /* --- invert --- */
   //! BB3
   //! /* logical preds: / linear preds: BB1, BB2, / kind: invert, */
   //! s2: %0:exec,  s1: %0:scc = s_andn2_b64 %saved_exec:s[84-85], %0:exec
      },
   [&]() -> void
   {
      /* --- logical else --- */
   //! BB4
   //! /* logical preds: BB0, / linear preds: BB3, / kind: */
   //! p_logical_start
                  /* --- linear else --- */
   //! BB5
   //! /* logical preds: / linear preds: BB3, / kind: */
            /* --- merge block --- */
   //! BB6
   //! /* logical preds: BB1, BB4, / linear preds: BB4, BB5, / kind: uniform, top-level, merge, */
               END_TEST
      BEGIN_TEST(optimizer_postRA.dpp_across_cf_overwritten)
      //>> v1: %a:v[0], v1: %b:v[1], v1: %c:v[2], v1: %d:v[3], s2: %e:s[0-1], s1: %f:s[2] = p_startpgm
   if (!setup_cs("v1 v1 v1 v1 s2 s1", GFX10_3))
            aco_ptr<Instruction>& startpgm = bld.instructions->at(0);
   startpgm->definitions[0].setFixed(PhysReg(256));
   startpgm->definitions[1].setFixed(PhysReg(257));
   startpgm->definitions[2].setFixed(PhysReg(258));
   startpgm->definitions[3].setFixed(PhysReg(259));
   startpgm->definitions[4].setFixed(PhysReg(0));
            Operand a(inputs[0], PhysReg(256)); /* source for DPP */
   Operand b(inputs[1], PhysReg(257)); /* source for fadd */
   Operand c(inputs[2], PhysReg(258)); /* buffer store address */
   Operand d(inputs[3], PhysReg(259)); /* buffer store value */
   Operand e(inputs[4], PhysReg(0));   /* condition */
   Operand f(inputs[5], PhysReg(2));   /* buffer store address (scalar) */
            //! v1: %dpp_tmp:v[12] = v_mov_b32 %a:v[0] row_mirror bound_ctrl:1 fi
            //! s2: %saved_exec:s[84-85],  s1: %0:scc,  s2: %0:exec = s_and_saveexec_b64 %e:s[0-1], %0:exec
            emit_divergent_if_else(
      program.get(), bld, e,
   [&]() -> void
   {
      /* --- logical then --- */
   //! BB1
                                 //! buffer_store_dword %addr:v[0], 0, %d:v[3], 0 offen
                                 /* --- linear then --- */
   //! BB2
                  /* --- invert --- */
   //! BB3
   //! /* logical preds: / linear preds: BB1, BB2, / kind: invert, */
   //! s2: %0:exec,  s1: %0:scc = s_andn2_b64 %saved_exec:s[84-85], %0:exec
      },
   [&]() -> void
   {
      /* --- logical else --- */
   //! BB4
   //! /* logical preds: BB0, / linear preds: BB3, / kind: */
   //! p_logical_start
                  /* --- linear else --- */
   //! BB5
   //! /* logical preds: / linear preds: BB3, / kind: */
            /* --- merge block --- */
   //! BB6
   //! /* logical preds: BB1, BB4, / linear preds: BB4, BB5, / kind: uniform, top-level, merge, */
            //! v1: %result:v[12] = v_add_f32 %dpp_mov_tmp:v[12], %b:v[1]
   Temp result =
         //! p_unit_test 10, %result:v[12]
               END_TEST
      BEGIN_TEST(optimizer_postRA.scc_nocmp_across_cf)
      //>> s2: %a:s[2-3], v1: %c:v[2], v1: %d:v[3], s2: %e:s[0-1] = p_startpgm
   if (!setup_cs("s2 v1 v1 s2", GFX10_3))
            aco_ptr<Instruction>& startpgm = bld.instructions->at(0);
   startpgm->definitions[0].setFixed(PhysReg(2));
   startpgm->definitions[1].setFixed(PhysReg(258));
   startpgm->definitions[2].setFixed(PhysReg(259));
            Operand a(inputs[0], PhysReg(2));   /* source for s_and */
   Operand c(inputs[1], PhysReg(258)); /* buffer store address */
   Operand d(inputs[2], PhysReg(259)); /* buffer store value */
   Operand e(inputs[3], PhysReg(0));   /* condition */
            auto tmp_salu = bld.sop2(aco_opcode::s_and_b64, bld.def(s2, reg_s8), bld.def(s1, scc), a,
            //! s2: %saved_exec:s[84-85],  s1: %0:scc,  s2: %0:exec = s_and_saveexec_b64 %e:s[0-1], %0:exec
            emit_divergent_if_else(
      program.get(), bld, e,
   [&]() -> void
   {
      /* --- logical then --- */
   //! BB1
                                                /* --- linear then --- */
   //! BB2
                  /* --- invert --- */
   //! BB3
   //! /* logical preds: / linear preds: BB1, BB2, / kind: invert, */
   //! s2: %0:exec,  s1: %0:scc = s_andn2_b64 %saved_exec:s[84-85], %0:exec
      },
   [&]() -> void
   {
      /* --- logical else --- */
   //! BB4
   //! /* logical preds: BB0, / linear preds: BB3, / kind: */
   //! p_logical_start
                  /* --- linear else --- */
   //! BB5
   //! /* logical preds: / linear preds: BB3, / kind: */
            /* --- merge block --- */
   //! BB6
   //! /* logical preds: BB1, BB4, / linear preds: BB4, BB5, / kind: uniform, top-level, merge, */
            //! s2: %tmp_salu:s[8-9], s1: %br_scc:scc = s_and_b64 %a:s[2-3], 0x40018
   //! s2: %br_vcc:vcc = p_cbranch_z %br_scc:scc
   //! p_unit_test 5, %br_vcc:vcc
   auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(tmp_salu, reg_s8),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
               END_TEST
      BEGIN_TEST(optimizer_postRA.scc_nocmp_across_cf_partially_overwritten)
      //>> s2: %a:s[2-3], v1: %c:v[2], v1: %d:v[3], s2: %e:s[0-1], s1: %f:s[4] = p_startpgm
   if (!setup_cs("s2 v1 v1 s2 s1", GFX10_3))
            aco_ptr<Instruction>& startpgm = bld.instructions->at(0);
   startpgm->definitions[0].setFixed(PhysReg(2));
   startpgm->definitions[1].setFixed(PhysReg(258));
   startpgm->definitions[2].setFixed(PhysReg(259));
   startpgm->definitions[3].setFixed(PhysReg(0));
            Operand a(inputs[0], PhysReg(2));   /* source for s_and */
   Operand c(inputs[1], PhysReg(258)); /* buffer store address */
   Operand d(inputs[2], PhysReg(259)); /* buffer store value */
   Operand e(inputs[3], PhysReg(0));   /* condition */
   Operand f(inputs[4], PhysReg(4));   /* overwrite value */
   PhysReg reg_s3(3);                  /* temporary register */
            //! s2: %tmp_salu:s[8-9], s1: %tmp_salu_scc:scc = s_and_b64 %a:s[2-3], 0x40018
   auto tmp_salu = bld.sop2(aco_opcode::s_and_b64, bld.def(s2, reg_s8), bld.def(s1, scc), a,
            //! s2: %saved_exec:s[84-85],  s1: %0:scc,  s2: %0:exec = s_and_saveexec_b64 %e:s[0-1], %0:exec
            emit_divergent_if_else(
      program.get(), bld, e,
   [&]() -> void
   {
      /* --- logical then --- */
   //! BB1
                                 //! buffer_store_dword %c:v[2], %ovrwr:s[3], %d:v[3], 0 offen
                                 /* --- linear then --- */
   //! BB2
                  /* --- invert --- */
   //! BB3
   //! /* logical preds: / linear preds: BB1, BB2, / kind: invert, */
   //! s2: %0:exec,  s1: %0:scc = s_andn2_b64 %saved_exec:s[84-85], %0:exec
      },
   [&]() -> void
   {
      /* --- logical else --- */
   //! BB4
   //! /* logical preds: BB0, / linear preds: BB3, / kind: */
   //! p_logical_start
                  /* --- linear else --- */
   //! BB5
   //! /* logical preds: / linear preds: BB3, / kind: */
            /* --- merge block --- */
   //! BB6
   //! /* logical preds: BB1, BB4, / linear preds: BB4, BB5, / kind: uniform, top-level, merge, */
            //! s1: %br_scc:scc = s_cmp_lg_u32 %tmp_salu:s[8-9], 0
   //! s2: %br_vcc:vcc = p_cbranch_z %br_scc:scc
   //! p_unit_test 5, %br_vcc:vcc
   auto scmp = bld.sopc(aco_opcode::s_cmp_lg_u32, bld.def(s1, scc), Operand(tmp_salu, reg_s8),
         auto br = bld.branch(aco_opcode::p_cbranch_z, bld.def(s2, vcc), bld.scc(scmp));
               END_TEST
