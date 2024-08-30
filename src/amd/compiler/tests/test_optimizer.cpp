   /*
   * Copyright © 2020 Valve Corporation
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
      BEGIN_TEST(optimize.neg)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, s1: %c, s1: %d = p_startpgm
   if (!setup_cs("v1 v1 s1 s1", (amd_gfx_level)i))
            //! v1: %res0 = v_mul_f32 %a, -%b
   //! p_unit_test 0, %res0
   Temp neg_b = fneg(inputs[1]);
            //~gfx9! v1: %neg_a = v_mul_f32 -1.0, %a
   //~gfx9! v1: %res1 = v_mul_f32 0x123456, %neg_a
   //~gfx10! v1: %res1 = v_mul_f32 0x123456, -%a
   //! p_unit_test 1, %res1
   Temp neg_a = fneg(inputs[0]);
            //! v1: %res2 = v_mul_f32 %a, %b
   //! p_unit_test 2, %res2
   Temp neg_neg_a = fneg(neg_a);
            //! v1: %res3 = v_mul_f32 |%a|, %b
   //! p_unit_test 3, %res3
   Temp abs_neg_a = fabs(neg_a);
            //! v1: %res4 = v_mul_f32 -|%a|, %b
   //! p_unit_test 4, %res4
   Temp abs_a = fabs(inputs[0]);
   Temp neg_abs_a = fneg(abs_a);
            //~gfx9! v1: %res5 = v_mul_f32 -%a, %b row_shl:1 bound_ctrl:1
   //~gfx10! v1: %res5 = v_mul_f32 -%a, %b row_shl:1 bound_ctrl:1 fi
   //! p_unit_test 5, %res5
   writeout(5,
            //! v1: %res6 = v_subrev_f32 %a, %b
   //! p_unit_test 6, %res6
            //! v1: %res7 = v_sub_f32 %b, %a
   //! p_unit_test 7, %res7
            //! v1: %res8 = v_mul_f32 %a, -%c
   //! p_unit_test 8, %res8
   Temp neg_c = fneg(bld.copy(bld.def(v1), inputs[2]));
            // //! v1: %res9 = v_mul_f32 |%neg_a|, %b
   // //! p_unit_test 9, %res9
   Temp abs_neg_abs_a = fabs(neg_abs_a);
                  END_TEST
      BEGIN_TEST(optimize.output_modifiers)
      //>> v1: %a, v1: %b = p_startpgm
   if (!setup_cs("v1 v1", GFX9))
                              //! v1: %res0 = v_add_f32 %a, %b *0.5
   //! p_unit_test 0, %res0
   Temp tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
            //! v1: %res1 = v_add_f32 %a, %b *2
   //! p_unit_test 1, %res1
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
            //! v1: %res2 = v_add_f32 %a, %b *4
   //! p_unit_test 2, %res2
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
            //! v1: %res3 = v_add_f32 %a, %b clamp
   //! p_unit_test 3, %res3
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(3, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
            //! v1: %res4 = v_add_f32 %a, %b *2 clamp
   //! p_unit_test 4, %res4
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   tmp = bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp);
   writeout(4, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
                     //! v2b: %res5 = v_add_f16 %a, %b *0.5
   //! p_unit_test 5, %res5
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
            //! v2b: %res6 = v_add_f16 %a, %b *2
   //! p_unit_test 6, %res6
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
            //! v2b: %res7 = v_add_f16 %a, %b *4
   //! p_unit_test 7, %res7
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
            //! v2b: %res8 = v_add_f16 %a, %b clamp
   //! p_unit_test 8, %res8
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(8, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
            //! v2b: %res9 = v_add_f16 %a, %b *2 clamp
   //! p_unit_test 9, %res9
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   tmp = bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000), tmp);
   writeout(9, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                     //! v1: %res10_tmp = v_add_f32 %a, %b clamp
   //! v1: %res10 = v_mul_f32 2.0, %res10_tmp
   //! p_unit_test 10, %res10
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   tmp = bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(), Operand::c32(0x3f800000u),
                           //! v1: %res11_tmp = v_xor_b32 %a, %b
   //! v1: %res11 = v_mul_f32 2.0, %res11_tmp
   //! p_unit_test 11, %res11
   tmp = bld.vop2(aco_opcode::v_xor_b32, bld.def(v1), inputs[0], inputs[1]);
                     //! v1: %res12_tmp = v_add_f32 %a, %b
   //! p_unit_test %res12_tmp
   //! v1: %res12 = v_mul_f32 2.0, %res12_tmp
   //! p_unit_test 12, %res12
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   bld.pseudo(aco_opcode::p_unit_test, tmp);
            //! v1: %res13 = v_add_f32 %a, %b
   //! p_unit_test 13, %res13
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp);
                     //>> BB1
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_keep;
   program->next_fp_mode.denorm16_64 = fp_denorm_flush;
            //! v1: %res14_tmp = v_add_f32 %a, %b
   //! v1: %res14 = v_mul_f32 2.0, %res13_tmp
   //! p_unit_test 14, %res14
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
            //! v1: %res15 = v_add_f32 %a, %b clamp
   //! p_unit_test 15, %res15
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(15, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
            //>> BB2
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_flush;
   program->next_fp_mode.denorm16_64 = fp_denorm_keep;
            //! v2b: %res16_tmp = v_add_f16 %a, %b
   //! v2b: %res16 = v_mul_f16 2.0, %res15_tmp
   //! p_unit_test 16, %res16
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
            //! v2b: %res17 = v_add_f16 %a, %b clamp
   //! p_unit_test 17, %res17
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(17, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
                     //>> BB3
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm32 = fp_denorm_keep;
   program->next_fp_mode.denorm16_64 = fp_denorm_keep;
   program->next_fp_mode.preserve_signed_zero_inf_nan32 = true;
   program->next_fp_mode.preserve_signed_zero_inf_nan16_64 = false;
            //! v1: %res18_tmp = v_add_f32 %a, %b
   //! v1: %res18 = v_mul_f32 2.0, %res18_tmp
   //! p_unit_test 18, %res18
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(18, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp));
   //! v1: %res19 = v_add_f32 %a, %b clamp
   //! p_unit_test 19, %res19
   tmp = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), inputs[0], inputs[1]);
   writeout(19, bld.vop3(aco_opcode::v_med3_f32, bld.def(v1), Operand::zero(),
            //>> BB4
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.preserve_signed_zero_inf_nan32 = false;
   program->next_fp_mode.preserve_signed_zero_inf_nan16_64 = true;
   bld.reset(program->create_and_insert_block());
   //! v2b: %res20_tmp = v_add_f16 %a, %b
   //! v2b: %res20 = v_mul_f16 2.0, %res20_tmp
   //! p_unit_test 20, %res20
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(20, bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000u), tmp));
   //! v2b: %res21 = v_add_f16 %a, %b clamp
   //! p_unit_test 21, %res21
   tmp = bld.vop2(aco_opcode::v_add_f16, bld.def(v2b), inputs[0], inputs[1]);
   writeout(21, bld.vop3(aco_opcode::v_med3_f16, bld.def(v2b), Operand::c16(0u),
               END_TEST
      Temp
   create_subbrev_co(Operand op0, Operand op1, Operand op2)
   {
         }
      BEGIN_TEST(optimize.cndmask)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, s1: %b, s2: %c = p_startpgm
   if (!setup_cs("v1 s1 s2", (amd_gfx_level)i))
                     //! v1: %res0 = v_cndmask_b32 0, %a, %c
   //! p_unit_test 0, %res0
   subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
            //! v1: %res1 = v_cndmask_b32 0, 42, %c
   //! p_unit_test 1, %res1
   subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
            //~gfx9! v1: %subbrev, s2: %_ = v_subbrev_co_u32 0, 0, %c
   //~gfx9! v1: %res2 = v_and_b32 %b, %subbrev
   //~gfx10! v1: %res2 = v_cndmask_b32 0, %b, %c
   //! p_unit_test 2, %res2
   subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
            //! v1: %subbrev1, s2: %_ = v_subbrev_co_u32 0, 0, %c
   //! v1: %xor = v_xor_b32 %a, %subbrev1
   //! v1: %res3 = v_cndmask_b32 0, %xor, %c
   //! p_unit_test 3, %res3
   subbrev = create_subbrev_co(Operand::zero(), Operand::zero(), Operand(inputs[2]));
   Temp xor_a = bld.vop2(aco_opcode::v_xor_b32, bld.def(v1), inputs[0], subbrev);
            //! v1: %res4 = v_cndmask_b32 0, %a, %c
   //! p_unit_test 4, %res4
   Temp cndmask = bld.vop2_e64(aco_opcode::v_cndmask_b32, bld.def(v1), Operand::zero(),
         Temp sub = bld.vsub32(bld.def(v1), Operand::zero(), cndmask);
                  END_TEST
      BEGIN_TEST(optimize.add_lshl)
      for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> s1: %a, v1: %b = p_startpgm
   if (!setup_cs("s1 v1", (amd_gfx_level)i))
                     //~gfx8! s1: %lshl0, s1: %_:scc = s_lshl_b32 %a, 3
   //~gfx8! s1: %res0, s1: %_:scc = s_add_u32 %lshl0, 4
   //~gfx(9|10)! s1: %res0, s1: %_:scc = s_lshl3_add_u32 %a, 4
   //! p_unit_test 0, %res0
   shift = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), bld.def(s1, scc), Operand(inputs[0]),
         writeout(0, bld.sop2(aco_opcode::s_add_u32, bld.def(s1), bld.def(s1, scc), shift,
            //~gfx8! s1: %lshl1, s1: %_:scc = s_lshl_b32 %a, 3
   //~gfx8! s1: %add1, s1: %_:scc = s_add_u32 %lshl1, 4
   //~gfx8! v1: %add_co1, s2: %_ = v_add_co_u32 %lshl1, %b
   //~gfx8! v1: %res1, s2: %_ = v_add_co_u32 %add1, %add_co1
   //~gfx(9|10)! s1: %lshl1, s1: %_:scc = s_lshl3_add_u32 %a, 4
   //~gfx(9|10)! v1: %lshl_add = v_lshl_add_u32 %a, 3, %b
   //~gfx(9|10)! v1: %res1 = v_add_u32 %lshl1, %lshl_add
   //! p_unit_test 1, %res1
   shift = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), bld.def(s1, scc), Operand(inputs[0]),
         Temp sadd =
         Temp vadd = bld.vadd32(bld.def(v1), shift, Operand(inputs[1]));
            //~gfx8! s1: %lshl2 = s_lshl_b32 %a, 3
   //~gfx8! v1: %res2,  s2: %_ = v_add_co_u32 %lshl2, %b
   //~gfx(9|10)! v1: %res2 = v_lshl_add_u32 %a, 3, %b
   //! p_unit_test 2, %res2
   Temp lshl =
                  //~gfx8! s1: %lshl3 = s_lshl_b32 (is24bit)%a, 7
   //~gfx8! v1: %res3, s2: %_ = v_add_co_u32 %lshl3, %b
   //~gfx(9|10)! v1: %res3 = v_lshl_add_u32 (is24bit)%a, 7, %b
   //! p_unit_test 3, %res3
   Operand a_24bit = Operand(inputs[0]);
   a_24bit.set24bit(true);
   lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(7u));
            //! s1: %lshl4 = s_lshl_b32 (is24bit)%a, 3
   //~gfx(8|9)! v1: %res4, s2: %carry = v_add_co_u32 %lshl4, %b
   //~gfx10! v1: %res4, s2: %carry = v_add_co_u32_e64 %lshl4, %b
   //! p_unit_test 4, %carry
   lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(3u));
   Temp carry = bld.vadd32(bld.def(v1), lshl, Operand(inputs[1]), true).def(1).getTemp();
            //~gfx8! s1: %lshl5 = s_lshl_b32 (is24bit)%a, (is24bit)%a
   //~gfx8! v1: %res5, s2: %_ = v_add_co_u32 %lshl5, %b
   //~gfx(9|10)! v1: %res5 = v_lshl_add_u32 (is24bit)%a, (is24bit)%a, %b
   //! p_unit_test 5, %res5
   lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, a_24bit);
            //~gfx8! v1: %res6 = v_mad_u32_u24 (is24bit)%a, 8, %b
   //~gfx(9|10)! v1: %res6 = v_lshl_add_u32 (is24bit)%a, 3, %b
   //! p_unit_test 6, %res6
   lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_24bit, Operand::c32(3u));
            //~gfx8! v1: %res7 = v_mad_u32_u24 (is16bit)%a, 16, %b
   //~gfx(9|10)! v1: %res7 = v_lshl_add_u32 (is16bit)%a, 4, %b
   //! p_unit_test 7, %res7
   Operand a_16bit = Operand(inputs[0]);
   a_16bit.set16bit(true);
   lshl = bld.sop2(aco_opcode::s_lshl_b32, bld.def(s1), a_16bit, Operand::c32(4u));
                  END_TEST
      BEGIN_TEST(optimize.bcnt)
      for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> v1: %a, s1: %b = p_startpgm
   if (!setup_cs("v1 s1", (amd_gfx_level)i))
                     //! v1: %res0 = v_bcnt_u32_b32 %a, %a
   //! p_unit_test 0, %res0
   bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
            //! v1: %res1 = v_bcnt_u32_b32 %a, %b
   //! p_unit_test 1, %res1
   bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
            //! v1: %res2 = v_bcnt_u32_b32 %a, 42
   //! p_unit_test 2, %res2
   bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
            //! v1: %bnct3 = v_bcnt_u32_b32 %b, 0
   //~gfx8! v1: %res3, s2: %_ = v_add_co_u32 %bcnt3, %a
   //~gfx(9|10)! v1: %res3 = v_add_u32 %bcnt3, %a
   //! p_unit_test 3, %res3
   bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[1]), Operand::zero());
            //! v1: %bnct4 = v_bcnt_u32_b32 %a, 0
   //~gfx(8|9)! v1: %add4, s2: %carry = v_add_co_u32 %bcnt4, %a
   //~gfx10! v1: %add4, s2: %carry = v_add_co_u32_e64 %bcnt4, %a
   //! p_unit_test 4, %carry
   bcnt = bld.vop3(aco_opcode::v_bcnt_u32_b32, bld.def(v1), Operand(inputs[0]), Operand::zero());
   Temp carry = bld.vadd32(bld.def(v1), bcnt, Operand(inputs[0]), true).def(1).getTemp();
                  END_TEST
      struct clamp_config {
      const char* name;
   aco_opcode min, max, med3;
      };
      static const clamp_config clamp_configs[] = {
      /* 0.0, 4.0 */
   {"_0,4f32", aco_opcode::v_min_f32, aco_opcode::v_max_f32, aco_opcode::v_med3_f32,
   Operand::zero(), Operand::c32(0x40800000u)},
   {"_0,4f16", aco_opcode::v_min_f16, aco_opcode::v_max_f16, aco_opcode::v_med3_f16,
   Operand::c16(0u), Operand::c16(0x4400)},
   /* -1.0, 0.0 */
   {"_-1,0f32", aco_opcode::v_min_f32, aco_opcode::v_max_f32, aco_opcode::v_med3_f32,
   Operand::c32(0xbf800000u), Operand::zero()},
   {"_-1,0f16", aco_opcode::v_min_f16, aco_opcode::v_max_f16, aco_opcode::v_med3_f16,
   Operand::c16(0xBC00), Operand::c16(0u)},
   /* 0, 3 */
   {"_0,3u32", aco_opcode::v_min_u32, aco_opcode::v_max_u32, aco_opcode::v_med3_u32,
   Operand::zero(), Operand::c32(3u)},
   {"_0,3u16", aco_opcode::v_min_u16, aco_opcode::v_max_u16, aco_opcode::v_med3_u16,
   Operand::c16(0u), Operand::c16(3u)},
   {"_0,3i32", aco_opcode::v_min_i32, aco_opcode::v_max_i32, aco_opcode::v_med3_i32,
   Operand::zero(), Operand::c32(3u)},
   {"_0,3i16", aco_opcode::v_min_i16, aco_opcode::v_max_i16, aco_opcode::v_med3_i16,
   Operand::c16(0u), Operand::c16(3u)},
   /* -5, 0 */
   {"_-5,0i32", aco_opcode::v_min_i32, aco_opcode::v_max_i32, aco_opcode::v_med3_i32,
   Operand::c32(0xfffffffbu), Operand::zero()},
   {"_-5,0i16", aco_opcode::v_min_i16, aco_opcode::v_max_i16, aco_opcode::v_med3_i16,
      };
      BEGIN_TEST(optimize.clamp)
      for (clamp_config cfg : clamp_configs) {
      if (!setup_cs("v1 v1 v1", GFX9, CHIP_UNKNOWN, cfg.name))
            //! cfg: @match_func(min max med3 lb ub)
   fprintf(output, "cfg: %s ", instr_info.name[(int)cfg.min]);
   fprintf(output, "%s ", instr_info.name[(int)cfg.max]);
   fprintf(output, "%s ", instr_info.name[(int)cfg.med3]);
   aco_print_operand(&cfg.lb, output);
   fprintf(output, " ");
   aco_print_operand(&cfg.ub, output);
                     //! v1: %res0 = @med3 @ub, @lb, %a
   //! p_unit_test 0, %res0
   writeout(0, bld.vop2(cfg.min, bld.def(v1), cfg.ub,
            //! v1: %res1 = @med3 @lb, @ub, %a
   //! p_unit_test 1, %res1
   writeout(1, bld.vop2(cfg.max, bld.def(v1), cfg.lb,
            /* min constant must be greater than max constant */
   //! v1: %res2_tmp = @min @lb, %a
   //! v1: %res2 = @max @ub, %res2_tmp
   //! p_unit_test 2, %res2
   writeout(2, bld.vop2(cfg.max, bld.def(v1), cfg.ub,
            //! v1: %res3_tmp = @max @ub, %a
   //! v1: %res3 = @min @lb, %res3_tmp
   //! p_unit_test 3, %res3
   writeout(3, bld.vop2(cfg.min, bld.def(v1), cfg.lb,
                     //! v1: %res4_tmp = @max @lb, %a
   //! v1: %res4 = @min %b, %res4_tmp
   //! p_unit_test 4, %res4
   writeout(4, bld.vop2(cfg.min, bld.def(v1), inputs[1],
            //! v1: %res5_tmp = @max %b, %a
   //! v1: %res5 = @min @ub, %res5_tmp
   //! p_unit_test 5, %res5
   writeout(5, bld.vop2(cfg.min, bld.def(v1), cfg.ub,
            //! v1: %res6_tmp = @max %c, %a
   //! v1: %res6 = @min %b, %res6_tmp
   //! p_unit_test 6, %res6
   writeout(6, bld.vop2(cfg.min, bld.def(v1), inputs[1],
            /* correct NaN behaviour with precise */
   if (cfg.min == aco_opcode::v_min_f16 || cfg.min == aco_opcode::v_min_f32) {
      //~f(16|32)! v1: %res7 = @med3 @ub, @lb, %a
   //~f(16|32)! p_unit_test 7, %res7
   Builder::Result max = bld.vop2(cfg.max, bld.def(v1), cfg.lb, inputs[0]);
   max.def(0).setPrecise(true);
   Builder::Result min = bld.vop2(cfg.min, bld.def(v1), cfg.ub, max);
                  //~f(16|32)! v1: (precise)%res8_tmp = @min @ub, %a
   //~f(16|32)! v1: %res8 = @max @lb, %res8_tmp
   //~f(16|32)! p_unit_test 8, %res8
   min = bld.vop2(cfg.min, bld.def(v1), cfg.ub, inputs[0]);
   min.def(0).setPrecise(true);
                     END_TEST
      BEGIN_TEST(optimize.const_comparison_ordering)
      //>> v1: %a, v1: %b, v2: %c, v1: %d = p_startpgm
   if (!setup_cs("v1 v1 v2 v1", GFX9))
            /* optimize to unordered comparison */
   //! s2: %res0 = v_cmp_nge_f32 4.0, %a
   //! p_unit_test 0, %res0
   writeout(0, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        //! s2: %res1 = v_cmp_nge_f32 4.0, %a
   //! p_unit_test 1, %res1
   writeout(1, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        //! s2: %res2 = v_cmp_nge_f32 0x40a00000, %a
   //! p_unit_test 2, %res2
   writeout(2, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        /* optimize to ordered comparison */
   //! s2: %res3 = v_cmp_lt_f32 4.0, %a
   //! p_unit_test 3, %res3
   writeout(3, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        //! s2: %res4 = v_cmp_lt_f32 4.0, %a
   //! p_unit_test 4, %res4
   writeout(4, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        //! s2: %res5 = v_cmp_lt_f32 0x40a00000, %a
   //! p_unit_test 5, %res5
   writeout(5, bld.sop2(aco_opcode::s_and_b64, bld.def(bld.lm), bld.def(s1, scc),
                        /* similar but unoptimizable expressions */
   //! s2: %tmp6_0 = v_cmp_lt_f32 4.0, %a
   //! s2: %tmp6_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res6, s1: %_:scc = s_and_b64 %tmp6_1, %tmp6_0
   //! p_unit_test 6, %res6
   Temp src1 =
         Temp src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
            //! s2: %tmp7_0 = v_cmp_nge_f32 4.0, %a
   //! s2: %tmp7_1 = v_cmp_eq_f32 %a, %a
   //! s2: %res7, s1: %_:scc = s_or_b64 %tmp7_1, %tmp7_0
   //! p_unit_test 7, %res7
   src1 =
         src0 = bld.vopc(aco_opcode::v_cmp_eq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
            //! s2: %tmp8_0 = v_cmp_lt_f32 4.0, %d
   //! s2: %tmp8_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res8, s1: %_:scc = s_or_b64 %tmp8_1, %tmp8_0
   //! p_unit_test 8, %res8
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[3]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
            //! s2: %tmp9_0 = v_cmp_lt_f32 4.0, %a
   //! s2: %tmp9_1 = v_cmp_neq_f32 %a, %d
   //! s2: %res9, s1: %_:scc = s_or_b64 %tmp9_1, %tmp9_0
   //! p_unit_test 9, %res9
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(0x40800000u), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[3]);
            /* bit sizes */
   //! s2: %res10 = v_cmp_nge_f16 4.0, %b
   //! p_unit_test 10, %res10
   Temp input1_16 =
         writeout(10, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        //! s2: %res11 = v_cmp_nge_f64 4.0, %c
   //! p_unit_test 11, %res11
   writeout(11, bld.sop2(aco_opcode::s_or_b64, bld.def(bld.lm), bld.def(s1, scc),
                        /* NaN */
   uint16_t nan16 = 0x7e00;
   uint32_t nan32 = 0x7fc00000;
            //! s2: %tmp12_0 = v_cmp_lt_f16 0x7e00, %a
   //! s2: %tmp12_1 = v_cmp_neq_f16 %a, %a
   //! s2: %res12, s1: %_:scc = s_or_b64 %tmp12_1, %tmp12_0
   //! p_unit_test 12, %res12
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), Operand::c16(nan16), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), inputs[0], inputs[0]);
            //! s2: %tmp13_0 = v_cmp_lt_f32 0x7fc00000, %a
   //! s2: %tmp13_1 = v_cmp_neq_f32 %a, %a
   //! s2: %res13, s1: %_:scc = s_or_b64 %tmp13_1, %tmp13_0
   //! p_unit_test 13, %res13
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), Operand::c32(nan32), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), inputs[0], inputs[0]);
            //! s2: %tmp14_0 = v_cmp_lt_f64 -1, %a
   //! s2: %tmp14_1 = v_cmp_neq_f64 %a, %a
   //! s2: %res14, s1: %_:scc = s_or_b64 %tmp14_1, %tmp14_0
   //! p_unit_test 14, %res14
   src1 = bld.vopc(aco_opcode::v_cmp_lt_f64, bld.def(bld.lm), Operand::c64(nan64), inputs[0]);
   src0 = bld.vopc(aco_opcode::v_cmp_neq_f64, bld.def(bld.lm), inputs[0], inputs[0]);
               END_TEST
      BEGIN_TEST(optimize.add3)
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", GFX9))
            //! v1: %res0 = v_add3_u32 %a, %b, %c
   //! p_unit_test 0, %res0
   Builder::Result tmp = bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
            //! v1: %tmp1 = v_add_u32 %b, %c clamp
   //! v1: %res1 = v_add_u32 %a, %tmp1
   //! p_unit_test 1, %res1
   tmp = bld.vop2_e64(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
   tmp->valu().clamp = true;
            //! v1: %tmp2 = v_add_u32 %b, %c
   //! v1: %res2 = v_add_u32 %a, %tmp2 clamp
   //! p_unit_test 2, %res2
   tmp = bld.vop2(aco_opcode::v_add_u32, bld.def(v1), inputs[1], inputs[2]);
   tmp = bld.vop2_e64(aco_opcode::v_add_u32, bld.def(v1), inputs[0], tmp);
   tmp->valu().clamp = true;
               END_TEST
      BEGIN_TEST(optimize.minmax)
      for (unsigned i = GFX10_3; i <= GFX11; i++) {
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", (amd_gfx_level)i))
            Temp a = inputs[0];
   Temp b = inputs[1];
            //! v1: %res0 = v_min3_f32 %a, %b, %c
   //! p_unit_test 0, %res0
            //! v1: %res1 = v_max3_f32 %a, %b, %c
   //! p_unit_test 1, %res1
            //! v1: %res2 = v_min3_f32 -%a, -%b, %c
   //! p_unit_test 2, %res2
            //! v1: %res3 = v_max3_f32 -%a, -%b, %c
   //! p_unit_test 3, %res3
            //! v1: %res4 = v_max3_f32 -%a, %b, %c
   //! p_unit_test 4, %res4
            //~gfx10_3! v1: %res5_tmp = v_max_f32 %a, %b
   //~gfx10_3! v1: %res5 = v_min_f32 %c, %res5_tmp
   //~gfx11! v1: %res5 = v_maxmin_f32 %a, %b, %c
   //! p_unit_test 5, %res5
            //~gfx10_3! v1: %res6_tmp = v_min_f32 %a, %b
   //~gfx10_3! v1: %res6 = v_max_f32 %c, %res6_tmp
   //~gfx11! v1: %res6 = v_minmax_f32 %a, %b, %c
   //! p_unit_test 6, %res6
            //~gfx10_3! v1: %res7_tmp = v_min_f32 %a, %b
   //~gfx10_3! v1: %res7 = v_min_f32 %c, -%res7_tmp
   //~gfx11! v1: %res7 = v_maxmin_f32 -%a, -%b, %c
   //! p_unit_test 7, %res7
            //~gfx10_3! v1: %res8_tmp = v_max_f32 %a, %b
   //~gfx10_3! v1: %res8 = v_max_f32 %c, -%res8_tmp
   //~gfx11! v1: %res8 = v_minmax_f32 -%a, -%b, %c
   //! p_unit_test 8, %res8
            //~gfx10_3! v1: %res9_tmp = v_max_f32 %a, -%b
   //~gfx10_3! v1: %res9 = v_max_f32 %c, -%res9_tmp
   //~gfx11! v1: %res9 = v_minmax_f32 -%a, %b, %c
   //! p_unit_test 9, %res9
                  END_TEST
      BEGIN_TEST(optimize.mad_32_24)
      for (unsigned i = GFX8; i <= GFX9; i++) {
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", (amd_gfx_level)i))
            //! v1: %res0 = v_mad_u32_u24 %b, %c, %a
   //! p_unit_test 0, %res0
   Temp mul = bld.vop2(aco_opcode::v_mul_u32_u24, bld.def(v1), inputs[1], inputs[2]);
            //! v1: %res1_tmp = v_mul_u32_u24 %b, %c
   //! v1: %_, s2: %res1 = v_add_co_u32 %a, %res1_tmp
   //! p_unit_test 1, %res1
   mul = bld.vop2(aco_opcode::v_mul_u32_u24, bld.def(v1), inputs[1], inputs[2]);
                  END_TEST
      BEGIN_TEST(optimize.add_lshlrev)
      for (unsigned i = GFX8; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, s1: %c = p_startpgm
   if (!setup_cs("v1 v1 s1", (amd_gfx_level)i))
                     //~gfx8! v1: %lshl0 = v_lshlrev_b32 3, %a
   //~gfx8! v1: %res0, s2: %_ = v_add_co_u32 %lshl0, %b
   //~gfx(9|10)! v1: %res0 = v_lshl_add_u32 %a, 3, %b
   //! p_unit_test 0, %res0
   lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(3u), Operand(inputs[0]));
            //~gfx8! v1: %lshl1 = v_lshlrev_b32 7, (is24bit)%a
   //~gfx8! v1: %res1, s2: %_ = v_add_co_u32 %lshl1, %b
   //~gfx(9|10)! v1: %res1 = v_lshl_add_u32 (is24bit)%a, 7, %b
   //! p_unit_test 1, %res1
   Operand a_24bit = Operand(inputs[0]);
   a_24bit.set24bit(true);
   lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(7u), a_24bit);
            //~gfx8! v1: %lshl2 = v_lshlrev_b32 (is24bit)%a, (is24bit)%b
   //~gfx8! v1: %res2, s2: %_ = v_add_co_u32 %lshl2, %b
   //~gfx(9|10)! v1: %res2 = v_lshl_add_u32 (is24bit)%b, (is24bit)%a, %b
   //! p_unit_test 2, %res2
   Operand b_24bit = Operand(inputs[1]);
   b_24bit.set24bit(true);
   lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), a_24bit, b_24bit);
            //~gfx8! v1: %res3 = v_mad_u32_u24 (is24bit)%a, 8, %b
   //~gfx(9|10)! v1: %res3 = v_lshl_add_u32 (is24bit)%a, 3, %b
   //! p_unit_test 3, %res3
   lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(3u), a_24bit);
            //~gfx8! v1: %res4 = v_mad_u32_u24 (is16bit)%a, 16, %b
   //~gfx(9|10)! v1: %res4 = v_lshl_add_u32 (is16bit)%a, 4, %b
   //! p_unit_test 4, %res4
   Operand a_16bit = Operand(inputs[0]);
   a_16bit.set16bit(true);
   lshl = bld.vop2(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(4u), a_16bit);
            //~gfx8! v1: %res5 = v_mad_u32_u24 (is24bit)%c, 16, %c
   //~gfx(9|10)! v1: %res5 = v_lshl_add_u32 (is24bit)%c, 4, %c
   //! p_unit_test 5, %res5
   Operand c_24bit = Operand(inputs[2]);
   c_24bit.set24bit(true);
   lshl = bld.vop2_e64(aco_opcode::v_lshlrev_b32, bld.def(v1), Operand::c32(4u), c_24bit);
                  END_TEST
      enum denorm_op {
      denorm_mul1 = 0,
   denorm_fneg = 1,
   denorm_fabs = 2,
      };
      static const char* denorm_op_names[] = {
      "mul1",
   "fneg",
   "fabs",
      };
      struct denorm_config {
      bool flush;
   unsigned op;
   aco_opcode src;
      };
      static const char*
   srcdest_op_name(aco_opcode op)
   {
      switch (op) {
   case aco_opcode::v_cndmask_b32: return "cndmask";
   case aco_opcode::v_min_f32: return "min";
   case aco_opcode::v_rcp_f32: return "rcp";
   default: return "none";
      }
      static Temp
   emit_denorm_srcdest(aco_opcode op, Temp val)
   {
      switch (op) {
   case aco_opcode::v_cndmask_b32:
         case aco_opcode::v_min_f32:
         case aco_opcode::v_rcp_f32: return bld.vop1(aco_opcode::v_rcp_f32, bld.def(v1), val);
   default: return val;
      }
      BEGIN_TEST(optimize.denorm_propagation)
      for (unsigned i = GFX8; i <= GFX9; i++) {
      std::vector<denorm_config> configs;
   for (bool flush : {false, true}) {
                     for (aco_opcode dest : {aco_opcode::v_min_f32, aco_opcode::v_rcp_f32}) {
      for (denorm_op op : {denorm_mul1, denorm_fneg, denorm_fabs, denorm_fnegabs})
               for (aco_opcode src :
      {aco_opcode::v_cndmask_b32, aco_opcode::v_min_f32, aco_opcode::v_rcp_f32}) {
   for (denorm_op op : {denorm_mul1, denorm_fneg, denorm_fabs, denorm_fnegabs})
                  for (denorm_config cfg : configs) {
      char subvariant[128];
   sprintf(subvariant, "_%s_%s_%s_%s", cfg.flush ? "flush" : "keep", srcdest_op_name(cfg.src),
                        bool can_propagate = cfg.src == aco_opcode::v_rcp_f32 ||
                        fprintf(output, "src, dest, op: %s %s %s\n", srcdest_op_name(cfg.src),
         fprintf(output, "can_propagate: %u\n", can_propagate);
   //! src, dest, op: $src $dest $op
                  //; patterns = {'cndmask': 'v1: %{} = v_cndmask_b32 0, {}, %b',
   //;             'min': 'v1: %{} = v_min_f32 0, {}',
   //;             'rcp': 'v1: %{} = v_rcp_f32 {}'}
   //; ops = {'mul1': 'v1: %{} = v_mul_f32 1.0, %{}',
   //;        'fneg': 'v1: %{} = v_mul_f32 -1.0, %{}',
   //;        'fabs': 'v1: %{} = v_mul_f32 1.0, |%{}|',
                  //; name = 'a'
   //; if src != 'none':
                  //; if can_propagate:
   //;    name = inline_ops[op].format(name)
   //; else:
                  //; if dest != 'none':
                                          Temp val = emit_denorm_srcdest(cfg.src, inputs[0]);
   switch (cfg.op) {
   case denorm_mul1:
      val = bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x3f800000u), val);
      case denorm_fneg: val = fneg(val); break;
   case denorm_fabs: val = fabs(val); break;
   case denorm_fnegabs: val = fneg(fabs(val)); break;
   }
   val = emit_denorm_srcdest(cfg.dest, val);
                           END_TEST
      BEGIN_TEST(optimizer.dpp)
      //>> v1: %a, v1: %b, s2: %c, s1: %d = p_startpgm
   if (!setup_cs("v1 v1 s2 s1", GFX10_3))
            Operand a(inputs[0]);
   Operand b(inputs[1]);
   Operand c(inputs[2]);
            /* basic optimization */
   //! v1: %res0 = v_add_f32 %a, %b row_mirror bound_ctrl:1 fi
   //! p_unit_test 0, %res0
   Temp tmp0 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res0 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), tmp0, b);
            /* operand swapping */
   //! v1: %res1 = v_subrev_f32 %a, %b row_mirror bound_ctrl:1 fi
   //! p_unit_test 1, %res1
   Temp tmp1 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res1 = bld.vop2(aco_opcode::v_sub_f32, bld.def(v1), b, tmp1);
            //! v1: %tmp2 = v_mov_b32 %a row_mirror bound_ctrl:1 fi
   //! v1: %res2 = v_sub_f32 %b, %tmp2 row_half_mirror bound_ctrl:1 fi
   //! p_unit_test 2, %res2
   Temp tmp2 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res2 = bld.vop2_dpp(aco_opcode::v_sub_f32, bld.def(v1), b, tmp2, dpp_row_half_mirror);
            /* modifiers */
   //! v1: %res3 = v_add_f32 -%a, %b row_mirror bound_ctrl:1 fi
   //! p_unit_test 3, %res3
   auto tmp3 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp3->dpp16().neg[0] = true;
   Temp res3 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), tmp3, b);
            //! v1: %res4 = v_add_f32 -%a, %b row_mirror bound_ctrl:1 fi
   //! p_unit_test 4, %res4
   Temp tmp4 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res4 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp4, b);
   res4->valu().neg[0] = true;
            //! v1: %tmp5 = v_mov_b32 %a row_mirror bound_ctrl:1 fi
   //! v1: %res5 = v_add_f32 %tmp5, %b clamp
   //! p_unit_test 5, %res5
   Temp tmp5 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res5 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp5, b);
   res5->valu().clamp = true;
            //! v1: %res6 = v_add_f32 |%a|, %b row_mirror bound_ctrl:1 fi
   //! p_unit_test 6, %res6
   auto tmp6 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp6->dpp16().neg[0] = true;
   auto res6 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp6, b);
   res6->valu().abs[0] = true;
            //! v1: %res7 = v_subrev_f32 %a, |%b| row_mirror bound_ctrl:1 fi
   //! p_unit_test 7, %res7
   Temp tmp7 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   auto res7 = bld.vop2_e64(aco_opcode::v_sub_f32, bld.def(v1), b, tmp7);
   res7->valu().abs[0] = true;
            //! v1: %tmp11 = v_mov_b32 -%a row_mirror bound_ctrl:1 fi
   //! v1: %res11 = v_add_u32 %tmp11, %b
   //! p_unit_test 11, %res11
   auto tmp11 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp11->dpp16().neg[0] = true;
   Temp res11 = bld.vop2(aco_opcode::v_add_u32, bld.def(v1), tmp11, b);
            //! v1: %tmp12 = v_mov_b32 -%a row_mirror bound_ctrl:1 fi
   //! v1: %res12 = v_add_f16 %tmp12, %b
   //! p_unit_test 12, %res12
   auto tmp12 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   tmp12->dpp16().neg[0] = true;
   Temp res12 = bld.vop2(aco_opcode::v_add_f16, bld.def(v1), tmp12, b);
            /* vcc */
   //! v1: %res8 = v_cndmask_b32 %a, %b, %c:vcc row_mirror bound_ctrl:1 fi
   //! p_unit_test 8, %res8
   Temp tmp8 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res8 = bld.vop2(aco_opcode::v_cndmask_b32, bld.def(v1), tmp8, b, c);
            /* sgprs */
   //! v1: %tmp9 = v_mov_b32 %a row_mirror bound_ctrl:1 fi
   //! v1: %res9 = v_add_f32 %tmp9, %d
   //! p_unit_test 9, %res9
   Temp tmp9 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res9 = bld.vop2_e64(aco_opcode::v_add_f32, bld.def(v1), tmp9, d);
            //! v1: %tmp10 = v_mov_b32 %a row_mirror bound_ctrl:1 fi
   //! v1: %res10 = v_add_f32 %d, %tmp10
   //! p_unit_test 10, %res10
   Temp tmp10 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp res10 = bld.vop2(aco_opcode::v_add_f32, bld.def(v1), d, tmp10);
               END_TEST
      BEGIN_TEST(optimize.dpp_prop)
      //>> v1: %a, s1: %b = p_startpgm
   if (!setup_cs("v1 s1", GFX10))
            //! v1: %one = p_parallelcopy 1
   //! v1: %res0 = v_mul_f32 1, %a
   //! p_unit_test 0, %res0
   Temp one = bld.copy(bld.def(v1), Operand::c32(1));
            //! v1: %res1 = v_mul_f32 %a, %one row_shl:1 bound_ctrl:1 fi
   //! p_unit_test 1, %res1
            //! v1: %res2 = v_mul_f32 0x12345678, %a
   //! p_unit_test 2, %res2
   Temp literal1 = bld.copy(bld.def(v1), Operand::c32(0x12345678u));
   writeout(2,
            //! v1: %literal2 = p_parallelcopy 0x12345679
   //! v1: %res3 = v_mul_f32 %a, %literal row_shl:1 bound_ctrl:1 fi
   //! p_unit_test 3, %res3
   Temp literal2 = bld.copy(bld.def(v1), Operand::c32(0x12345679u));
   writeout(3,
            //! v1: %b_v = p_parallelcopy %b
   //! v1: %res4 = v_mul_f32 %b, %a
   //! p_unit_test 4, %res4
   Temp b_v = bld.copy(bld.def(v1), inputs[1]);
            //! v1: %res5 = v_mul_f32 %a, %b_v row_shl:1 bound_ctrl:1 fi
   //! p_unit_test 5, %res5
            //! v1: %res6 = v_rcp_f32 %b
   //! p_unit_test 6, %res6
               END_TEST
      BEGIN_TEST(optimize.casts)
      //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", GFX10_3))
            Temp a = inputs[0];
                     //! v1: %res0_tmp = v_mul_f32 -1.0, %a
   //! v2b: %res0 = v_mul_f16 %res0_tmp, %a16
   //! p_unit_test 0, %res0
            //! v2b: %res1_tmp = v_mul_f16 -1.0, %a16
   //! v1: %res1 = v_mul_f32 %res1_tmp, %a
   //! p_unit_test 1, %res1
            //! v1: %res2_tmp = v_mul_f32 -1.0, %a16
   //! v2b: %res2 = v_mul_f16 %res2_tmp, %a16
   //! p_unit_test 2, %res2
   writeout(2, fmul(u2u16(bld.vop2_e64(aco_opcode::v_mul_f32, bld.def(v1),
                  //! v1: %res3_tmp = v_mul_f32 %a, %a
   //! v2b: %res3 = v_add_f16 %res3_tmp, 0 clamp
   //! p_unit_test 3, %res3
            //! v2b: %res4_tmp = v_mul_f16 %a16, %a16
   //! v1: %res4 = v_add_f32 %res4_tmp, 0 clamp
   //! p_unit_test 4, %res4
            //! v1: %res5_tmp = v_mul_f32 %a, %a
   //! v2b: %res5 = v_mul_f16 2.0, %res5_tmp
   //! p_unit_test 5, %res5
            //! v2b: %res6_tmp = v_mul_f16 %a16, %a16
   //! v1: %res6 = v_mul_f32 2.0, %res6_tmp
   //! p_unit_test 6, %res6
   writeout(6,
            //! v1: %res7_tmp = v_mul_f32 %a, %a
   //! v2b: %res7 = v_add_f16 %res7_tmp, %a16
   //! p_unit_test 7, %res7
            //! v2b: %res8_tmp = v_mul_f16 %a16, %a16
   //! v1: %res8 = v_add_f32 %res8_tmp, %a
   //! p_unit_test 8, %res8
            //! v1: %res9_tmp = v_mul_f32 %a, %a
   //! v2b: %res9 = v_mul_f16 -1.0, %res9_tmp
   //! p_unit_test 9, %res9
            //! v2b: %res10_tmp = v_mul_f16 %a16, %a16
   //! v1: %res10 = v_mul_f32 -1.0, %res10_tmp
   //! p_unit_test 10, %res10
   writeout(10, bld.vop2_e64(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0xbf800000u),
               END_TEST
      BEGIN_TEST(optimize.mad_mix.input_conv.basic)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
            //! v1: %res0 = v_fma_mix_f32 %a, lo(%a16), -0
   //! p_unit_test 0, %res0
            //! v1: %res1 = v_fma_mix_f32 1.0, %a, lo(%a16)
   //! p_unit_test 1, %res1
            //! v1: %res2 = v_fma_mix_f32 1.0, lo(%a16), %a
   //! p_unit_test 2, %res2
            //! v1: %res3 = v_fma_mix_f32 %a, %a, lo(%a16)
   //! p_unit_test 3, %res3
            //! v1: %res4 = v_fma_mix_f32 %a, %a, lo(%a16)
   //! p_unit_test 4, %res4
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.input_conv.precision)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
            /* precise arithmetic */
   //~gfx9! v1: %res0_cvt = v_cvt_f32_f16 %a16
   //~gfx9! v1: (precise)%res0 = v_fma_f32 %a, %a, %res0_cvt
   //~gfx10! v1: (precise)%res0 = v_fma_mix_f32 %a, %a, lo(%a16)
   //! p_unit_test 0, %res0
            //! v2b: %res1_cvt = v_cvt_f16_f32 %a
   //! v2b: (precise)%res1 = v_mul_f16 %a16, %res1_cvt
   //! p_unit_test 1, %res1
            //! v2b: %res2_cvt = v_cvt_f16_f32 %a
   //! v2b: (precise)%res2 = v_add_f16 %a16, %res2_cvt
   //! p_unit_test 2, %res2
            //! v2b: %res3_cvt = v_cvt_f16_f32 %a
   //! v2b: (precise)%res3 = v_fma_f16 %a16, %a16, %res3_cvt
   //! p_unit_test 3, %res3
            /* precise conversions */
   //! v2b: (precise)%res4_cvt = v_cvt_f16_f32 %a
   //! v2b: %res4 = v_mul_f16 %a16, %res4_cvt
   //! p_unit_test 4, %res4
            //! v2b: (precise)%res5_cvt = v_cvt_f16_f32 %a
   //! v2b: %res5 = v_add_f16 %a16, %res5_cvt
   //! p_unit_test 5, %res5
            //! v2b: (precise)%res6_cvt = v_cvt_f16_f32 %a
   //! v2b: %res6 = v_fma_f16 %a16, %a16, %res6_cvt
   //! p_unit_test 6, %res6
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.input_conv.modifiers)
      for (unsigned i = GFX9; i <= GFX11; i++) {
      if (i == GFX10_3)
         //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
            /* check whether modifiers are preserved when converting to VOP3P */
   //! v1: %res0 = v_fma_mix_f32 -%a, lo(%a16), -0
   //! p_unit_test 0, %res0
            //! v1: %res1 = v_fma_mix_f32 |%a|, lo(%a16), -0
   //! p_unit_test 1, %res1
            /* fneg modifiers */
   //! v1: %res2 = v_fma_mix_f32 %a, -lo(%a16), -0
   //! p_unit_test 2, %res2
            //! v1: %res3 = v_fma_mix_f32 %a, -lo(%a16), -0
   //! p_unit_test 3, %res3
            /* fabs modifiers */
   //! v1: %res4 = v_fma_mix_f32 %a, |lo(%a16)|, -0
   //! p_unit_test 4, %res4
            //! v1: %res5 = v_fma_mix_f32 %a, |lo(%a16)|, -0
   //! p_unit_test 5, %res5
            /* both fabs and fneg modifiers */
   //! v1: %res6 = v_fma_mix_f32 %a, -|lo(%a16)|, -0
   //! p_unit_test 6, %res6
            //! v1: %res7 = v_fma_mix_f32 %a, |lo(%a16)|, -0
   //! p_unit_test 7, %res7
            //! v1: %res8 = v_fma_mix_f32 %a, -|lo(%a16)|, -0
   //! p_unit_test 8, %res8
            //! v1: %res9 = v_fma_mix_f32 %a, -|lo(%a16)|, -0
   //! p_unit_test 9, %res9
            //! v1: %res10 = v_fma_mix_f32 %a, |lo(%a16)|, -0
   //! p_unit_test 10, %res10
            //! v1: %res11 = v_fma_mix_f32 %a, |lo(%a16)|, -0
   //! p_unit_test 11, %res11
            //! v1: %res12 = v_fma_mix_f32 %a, -|lo(%a16)|, -0
   //! p_unit_test 12, %res12
            /* sdwa */
   //! v1: %res13 = v_fma_mix_f32 lo(%a), %a, -0
   //! p_unit_test 13, %res13
            //! v1: %res14 = v_fma_mix_f32 hi(%a), %a, -0
   //! p_unit_test 14, %res14
            //~gfx(9|10)! v1: %res15_cvt = v_cvt_f32_f16 %a dst_sel:uword0 src0_sel:dword
   //~gfx11! v1: %res16_cvt1 = v_fma_mix_f32 lo(%a), 1.0, -0
   //~gfx11! v1: %res15_cvt = p_extract %res16_cvt1, 0, 16, 0
   //! v1: %res15 = v_mul_f32 %res15_cvt, %a
   //! p_unit_test 15, %res15
            //~gfx(9|10)! v1: %res16_cvt = v_cvt_f32_f16 %a
   //~gfx(9|10)! v1: %res16 = v_mul_f32 %res16_cvt, %a dst_sel:dword src0_sel:uword1 src1_sel:dword
   //~gfx11! v1: %res16_cvt = v_fma_mix_f32 lo(%a), 1.0, -0
   //~gfx11! v1: %res16_ext = p_extract %res16_cvt, 1, 16, 0
   //~gfx11! v1: %res16 = v_mul_f32 %res16_ext, %a
   //! p_unit_test 16, %res16
            //~gfx(9|10)! v1: %res17_cvt = v_cvt_f32_f16 %a dst_sel:dword src0_sel:ubyte2
   //~gfx(9|10)! v1: %res17 = v_mul_f32 %res17_cvt, %a
   //~gfx11! v1: %res17_ext = p_extract %a, 2, 8, 0
   //~gfx11! v1: %res17 = v_fma_mix_f32 lo(%res17_ext), %a, -0
   //! p_unit_test 17, %res17
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.output_conv.basic)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, v1: %c, v2b: %a16, v2b: %b16 = p_startpgm
   if (!setup_cs("v1 v1 v1 v2b v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
   Temp b = inputs[1];
   Temp c = inputs[2];
   Temp a16 = inputs[3];
            //! v2b: %res0 = v_fma_mixlo_f16 %a, %b, -0
   //! p_unit_test 0, %res0
            //! v2b: %res1 = v_fma_mixlo_f16 1.0, %a, %b
   //! p_unit_test 1, %res1
            //! v2b: %res2 = v_fma_mixlo_f16 %a, %b, %c
   //! p_unit_test 2, %res2
            //! v2b: %res3 = v_fma_mixlo_f16 lo(%a16), %b, -0
   //! p_unit_test 3, %res3
            //! v2b: %res4 = v_fma_mixlo_f16 1.0, %a, lo(%b16)
   //! p_unit_test 4, %res4
            //! v2b: %res5 = v_fma_mixlo_f16 %a, lo(%b16), %c
   //! p_unit_test 5, %res5
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.output_conv.precision)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v2b: %a16 = p_startpgm
   if (!setup_cs("v2b", (amd_gfx_level)i))
                              //! v2b: %res0_tmp = v_mul_f16 %a16, %a16
   //! v1: (precise)%res0 = v_cvt_f32_f16 %res0_tmp
   //! p_unit_test 0, %res0
            //! v2b: (precise)%res1_tmp = v_mul_f16 %a16, %a16
   //! v1: %res1 = v_cvt_f32_f16 %res1_tmp
   //! p_unit_test 1, %res1
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.output_conv.modifiers)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, v2b: %a16, v2b: %b16 = p_startpgm
   if (!setup_cs("v1 v1 v2b v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
   Temp b = inputs[1];
   Temp a16 = inputs[2];
            /* fneg/fabs */
   //! v1: %res0_add = v_add_f32 %1, %2
   //! v2b: %res0 = v_cvt_f16_f32 |%res0_add|
   //! p_unit_test 0, %res0
            //! v1: %res1_add = v_add_f32 %1, %2
   //! v2b: %res1 = v_cvt_f16_f32 -%res1_add
   //! p_unit_test 1, %res1
            //! v2b: %res2_add = v_add_f16 %3, %4
   //! v1: %res2 = v_cvt_f32_f16 |%res2_add|
   //! p_unit_test 2, %res2
            //! v2b: %res3_add = v_add_f16 %3, %4
   //! v1: %res3 = v_cvt_f32_f16 -%res3_add
   //! p_unit_test 3, %res3
            /* sdwa */
   //! v2b: %res4_add = v_fma_mixlo_f16 1.0, %a, %b
   //! v2b: %res4 = p_extract %res4_add, 0, 8, 0
   //! p_unit_test 4, %res4
            //! v1: %res5_mul = v_add_f32 %a, %b dst_sel:uword0 src0_sel:dword src1_sel:dword
   //! v2b: %res5 = v_cvt_f16_f32 %res5_mul
   //! p_unit_test 5, %res5
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.fma.basic)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, v1: %c, v2b: %a16, v2b: %c16 = p_startpgm
   if (!setup_cs("v1 v1 v1 v2b v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
   Temp b = inputs[1];
   Temp c = inputs[2];
   Temp a16 = inputs[3];
            //! v1: %res0 = v_fma_mix_f32 lo(%a16), %b, %c
   //! p_unit_test 0, %res0
            //! v1: %res1 = v_fma_mix_f32 %a, %b, lo(%c16)
   //! p_unit_test 1, %res1
            /* omod/clamp check */
   //! v1: %res2_mul = v_fma_mix_f32 lo(%a16), %b, -0
   //! v1: %res2 = v_add_f32 %res2_mul, %c *2
   //! p_unit_test 2, %res2
   writeout(2, bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000),
            /* neg/abs modifiers */
   //! v1: %res3 = v_fma_mix_f32 -lo(%a16), %b, |lo(%c16)|
   //! p_unit_test 3, %res3
            //! v1: %res4 = v_fma_mix_f32 |%a|, |%b|, lo(%c16)
   //! p_unit_test 4, %res4
            //! v1: %res5 = v_fma_mix_f32 %a, -%b, lo(%c16)
   //! p_unit_test 5, %res5
            //! v1: %res6 = v_fma_mix_f32 |%a|, -|%b|, lo(%c16)
   //! p_unit_test 6, %res6
            /* output conversions */
   //! v2b: %res7 = v_fma_mixlo_f16 %a, %b, %c
   //! p_unit_test 7, %res7
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.fma.precision)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v1: %b, v1: %c, v2b: %a16, v2b: %b16 = p_startpgm
   if (!setup_cs("v1 v1 v1 v2b v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
   Temp b = inputs[1];
   Temp c = inputs[2];
   Temp a16 = inputs[3];
            /* the optimization is precise for 32-bit on GFX9 */
   //~gfx9! v1: (precise)%res0 = v_fma_mix_f32 lo(%a16), %b, %c
   //~gfx10! v1: (precise)%res0_tmp = v_fma_mix_f32 lo(%a16), %b, -0
   //~gfx10! v1: %res0 = v_add_f32 %res0_tmp, %c
   //! p_unit_test 0, %res0
            //~gfx9! v1: (precise)%res1 = v_fma_mix_f32 lo(%a16), %b, %c
   //~gfx10! v1: %res1_tmp = v_fma_mix_f32 lo(%a16), %b, -0
   //~gfx10! v1: (precise)%res1 = v_add_f32 %res1_tmp, %c
   //! p_unit_test 1, %res1
            /* never promote 16-bit arithmetic to 32-bit */
   //! v2b: %res2_tmp = v_cvt_f16_f32 %a
   //! v2b: %res2 = v_add_f16 %res2_tmp, %b16
   //! p_unit_test 2, %res2
            //! v2b: %res3_tmp = v_cvt_f16_f32 %a
   //! v2b: %res3 = v_mul_f16 %res3_tmp, %b16
   //! p_unit_test 3, %res3
            //! v2b: %res4_tmp = v_mul_f16 %a16, %b16
   //! v1: %res4 = v_cvt_f32_f16 %res4_tmp
   //! p_unit_test 4, %res4
            //! v2b: %res5_tmp = v_add_f16 %a16, %b16
   //! v1: %res5 = v_cvt_f32_f16 %res5_tmp
   //! p_unit_test 5, %res5
            //! v2b: %res6_tmp = v_fma_mixlo_f16 %a, %b, -0
   //! v2b: %res6 = v_add_f16 %res6_tmp, %a16
   //! p_unit_test 6, %res6
            //! v2b: %res7_tmp = v_mul_f16 %a16, %b16
   //! v1: %res7 = v_fma_mix_f32 1.0, lo(%res7_tmp), %c
   //! p_unit_test 7, %res7
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.clamp)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
            //! v1: %res0 = v_fma_mix_f32 lo(%a16), %a, -0 clamp
   //! p_unit_test 0, %res0
            //! v2b: %res1 = v_fma_mixlo_f16 %a, %a, -0 clamp
   //! p_unit_test 1, %res1
            //! v2b: %res2 = v_fma_mixlo_f16 %a, %a, -0 clamp
   //! p_unit_test 2, %res2
                  END_TEST
      BEGIN_TEST(optimize.mad_mix.cast)
      for (unsigned i = GFX9; i <= GFX10; i++) {
      //>> v1: %a, v2b: %a16 = p_startpgm
   if (!setup_cs("v1 v2b", (amd_gfx_level)i))
                     Temp a = inputs[0];
            /* The optimizer copy-propagates v2b=p_extract_vector(v1, 0) and p_as_uniform, so the
   * optimizer has to check compatibility.
            //! v1: %res0_cvt = v_cvt_f32_f16 %a16
   //! v2b: %res0 = v_mul_f16 %res0_cvt, %a16
   //! p_unit_test 0, %res0
            //! v2b: %res1_cvt = v_cvt_f16_f32 %a
   //! v1: %res1 = v_mul_f32 %res1_cvt, %a
   //! p_unit_test 1, %res1
            //! v2b: %res2_mul = v_mul_f16 %a16, %a16
   //! v2b: %res2 = v_cvt_f16_f32 %res2_mul
   //! p_unit_test 2, %res2
            //! v1: %res3_mul = v_mul_f32 %a, %a
   //! v1: %res3 = v_cvt_f32_f16 %res3_mul
   //! p_unit_test 3, %res3
            //! v1: %res4_mul = v_fma_mix_f32 lo(%a16), %a, -0
   //! v2b: %res4 = v_add_f16 %res4_mul, 0 clamp
   //! p_unit_test 4, %res4
            //! v2b: %res5_mul = v_fma_mixlo_f16 %a, %a, -0
   //! v1: %res5 = v_add_f32 %res5_mul, 0 clamp
   //! p_unit_test 5, %res5
            //! v1: %res6_mul = v_mul_f32 %a, %a
   //! v1: %res6 = v_fma_mix_f32 1.0, lo(%res6_mul), %a
   //! p_unit_test 6, %res6
            //! v2b: %res7_mul = v_mul_f16 %a16, %a16
   //! v1: %res7 = v_fma_mix_f32 1.0, %res7_mul, lo(%a16)
   //! p_unit_test 7, %res7
            /* opsel_hi should be obtained from the original opcode, not the operand regclass */
   //! v1: %res8 = v_fma_mix_f32 lo(%a16), %a16, -0
   //! p_unit_test 8, %res8
                  END_TEST
      static void
   vop3p_constant(unsigned* idx, aco_opcode op, const char* swizzle, uint32_t val)
   {
      uint32_t halves[2] = {val & 0xffff, val >> 16};
   uint32_t expected = halves[swizzle[0] - 'x'] | (halves[swizzle[1] - 'x'] << 16);
            unsigned opsel_lo = swizzle[0] == 'x' ? 0x0 : 0x1;
   unsigned opsel_hi = swizzle[1] == 'x' ? 0x2 : 0x3;
   writeout((*idx)++, bld.vop3p(op, bld.def(v1), bld.copy(bld.def(v1), Operand::c32(val)),
      }
      BEGIN_TEST(optimize.vop3p_constants)
      for (aco_opcode op : {aco_opcode::v_pk_add_f16, aco_opcode::v_pk_add_u16}) {
      for (const char* swizzle : {"xx", "yy", "xy", "yx"}) {
      char variant[16];
   strcpy(variant, op == aco_opcode::v_pk_add_f16 ? "_f16" : "_u16");
                                 //>> v1: %a = p_startpgm
                  //; opcode = 'v_pk_add_u16' if 'u16' in variant else 'v_pk_add_f16'
   //; for i in range(36):
   //;    insert_pattern('v1: %%res%u = %s $got%u %%a' % (i, opcode, i))
                  //; def parse_op(op):
   //;    is_int = opcode == 'v_pk_add_u16'
   //;    op = op.rstrip(',')
   //;
   //;    mods = lambda v: v
   //;    if op.endswith('*[1,-1]'):
   //;       mods = lambda v: v ^ 0x80000000
   //;       assert(not is_int)
   //;    elif op.endswith('*[-1,1]'):
   //;       mods = lambda v: v ^ 0x00008000
   //;       assert(not is_int)
   //;    op = op.split('*')[0]
   //;
   //;    swizzle = lambda v: v
   //;    if op.endswith('.xx'):
   //;       swizzle = lambda v: ((v & 0xffff) | (v << 16)) & 0xffffffff;
   //;    elif op.endswith('.yy'):
   //;       swizzle = lambda v: (v >> 16) | (v & 0xffff0000);
   //;    elif op.endswith('.yx'):
   //;       swizzle = lambda v: ((v >> 16) | (v << 16)) & 0xffffffff;
   //;    op = op.rstrip('xy.')
   //;
   //;    val = None
   //;    if op.startswith('0x'):
   //;       val = int(op[2:], 16)
   //;    elif op == '-1.0':
   //;       val = 0xbf800000 if is_int else 0xbC00
   //;    elif op == '1.0':
   //;       val = 0x3f800000 if is_int else 0x3c00
   //;    else:
   //;       val = int(op) & 0xffffffff
                  //; # Check correctness
   //; for i in range(36):
   //;    expected = globals()['expected%u' % i]
   //;    got = globals()['got%u' % i]
   //;    got_parsed = parse_op(got)
                  //; # Check that all literals are ones that cannot be encoded as inline constants
   //; allowed_literals = [0x00004242, 0x0000fffe, 0x00308030, 0x0030ffff, 0x3c00ffff,
   //;                     0x42420000, 0x42424242, 0x4242c242, 0x4242ffff, 0x7ffefffe,
   //;                     0x80300030, 0xbeefdead, 0xc2424242, 0xdeadbeef, 0xfffe0000,
   //;                     0xfffe7ffe, 0xffff0030, 0xffff3c00, 0xffff4242]
   //; if opcode == 'v_pk_add_u16':
   //;    allowed_literals.extend([0x00003c00, 0x3c000000, 0x3c003c00, 0x3c00bc00, 0xbc003c00])
   //; else:
   //;    allowed_literals.extend([0x00003f80, 0x3f800000])
   //;
   //; for i in range(36):
   //;    got = globals()['got%u' % i]
   //;    if not got.startswith('0x'):
   //;       continue;
   //;    got = int(got[2:].rstrip(',').split('*')[0].split('.')[0], 16)
                  unsigned idx = 0;
   for (uint32_t constant : {0x3C00, 0x0030, 0xfffe, 0x4242}) {
      vop3p_constant(&idx, op, swizzle, constant);
   vop3p_constant(&idx, op, swizzle, constant | 0xffff0000);
   vop3p_constant(&idx, op, swizzle, constant | (constant << 16));
   vop3p_constant(&idx, op, swizzle, constant << 16);
   vop3p_constant(&idx, op, swizzle, (constant << 16) | 0x0000ffff);
   vop3p_constant(&idx, op, swizzle, constant | ((constant ^ 0x8000) << 16));
               for (uint32_t constant : {0x3f800000u, 0xfffffffeu, 0x00000030u, 0xdeadbeefu}) {
      uint32_t lo = constant & 0xffff;
   uint32_t hi = constant >> 16;
   vop3p_constant(&idx, op, swizzle, constant);
                        END_TEST
      BEGIN_TEST(optimize.fmamix_two_literals)
      /* This test has to recreate literals sometimes because we don't combine them at all if there's
   * at least one uncombined use.
   */
   for (unsigned i = GFX10; i <= GFX10_3; i++) {
      //>> v1: %a, v1: %b = p_startpgm
   if (!setup_cs("v1 v1", (amd_gfx_level)i))
            Temp a = inputs[0];
            Temp c15 = bld.copy(bld.def(v1), Operand::c32(fui(1.5f)));
   Temp c30 = bld.copy(bld.def(v1), Operand::c32(fui(3.0f)));
            //! v1: %res0 = v_fma_mix_f32 %a, lo(0x42003e00), hi(0x42003e00)
   //! p_unit_test 0, %res0
            /* No need to use v_fma_mix_f32. */
   //! v1: %res1 = v_fmaak_f32 %a, %b, 0x40400000
   //! p_unit_test 1, %res1
            /* Separate mul/add can become v_fma_mix_f32 if it's not precise. */
   //! v1: %res2 = v_fma_mix_f32 %a, lo(0x42003e00), hi(0x42003e00)
   //! p_unit_test 2, %res2
            //~gfx10! v1: %c15 = p_parallelcopy 0x3fc00000
   c15 = bld.copy(bld.def(v1), Operand::c32(fui(1.5f)));
            /* v_fma_mix_f32 is a fused mul/add, so it can't be used for precise separate mul/add. */
   //~gfx10! v1: (precise)%res3 = v_madak_f32 %a, %c15, 0x40400000
   //~gfx10_3! v1: (precise)%res3_tmp = v_mul_f32 %a, 0x3fc00000
   //~gfx10_3! v1: %res3 = v_add_f32 %res3_tmp, 0x40400000
   //! p_unit_test 3, %res3
            //~gfx10! v1: (precise)%res4 = v_madak_f32 %1, %c16, 0x40400000
   //~gfx10_3! v1: %res4_tmp = v_mul_f32 %a, 0x3fc00000
   //~gfx10_3! v1: (precise)%res4 = v_add_f32 %res4_tmp, 0x40400000
   //! p_unit_test 4, %res4
            /* Can't convert to fp16 if it will be flushed as a denormal. */
   //! v1: %res5 = v_fma_mix_f32 %1, lo(0x3ff3e00), hi(0x3ff3e00)
   //! p_unit_test 5, %res5
   c15 = bld.copy(bld.def(v1), Operand::c32(fui(1.5f)));
            //>> BB1
   //! /* logical preds: / linear preds: / kind: uniform, */
   program->next_fp_mode.denorm16_64 = fp_denorm_flush;
            //~gfx10; del c15
   //! v1: %c15 = p_parallelcopy 0x3fc00000
   //! v1: %res6 = v_fmaak_f32 %a, %c15, 0x387fc000
   //! p_unit_test 6, %res6
   c15 = bld.copy(bld.def(v1), Operand::c32(fui(1.5f)));
   c_denorm = bld.copy(bld.def(v1), Operand::c32(0x387fc000));
            /* Can't accept more than 3 unique fp16 literals. */
   //! v1: %c45 = p_parallelcopy 0x40900000
   //! v1: %res7 = v_fma_mix_f32 lo(0x42003e00), hi(0x42003e00), %c45
   //! p_unit_test 7, %res7
   Temp c45 = bld.copy(bld.def(v1), Operand::c32(fui(4.5f)));
            /* Modifiers must be preserved. */
   //! v1: %res8 = v_fma_mix_f32 -%a, lo(0x44804200), hi(0x44804200)
   //! p_unit_test 8, %res8
            //! v1: %res9 = v_fma_mix_f32 lo(0x44804200), |%a|, hi(0x44804200)
   //! p_unit_test 9, %res9
            //! v1: %res10 = v_fma_mix_f32 %a, lo(0x44804200), hi(0x44804200) clamp
   //! p_unit_test 10, %res10
            /* Output modifiers are not supported by v_fma_mix_f32. */
   c30 = bld.copy(bld.def(v1), Operand::c32(fui(3.0f)));
   //; del c45
   //! v1: %c45 = p_parallelcopy 0x40900000
   //! v1: %res11 = v_fma_f32 %a, 0x40400000, %c45 *0.5
   //! p_unit_test 11, %res11
   c45 = bld.copy(bld.def(v1), Operand::c32(fui(4.5f)));
            /* Has a literal which can't be represented as fp16. */
   //! v1: %c03 = p_parallelcopy 0x3e99999a
   //! v1: %res12 = v_fmaak_f32 %a, %c03, 0x40400000
   //! p_unit_test 12, %res12
   Temp c03 = bld.copy(bld.def(v1), Operand::c32(fui(0.3f)));
            /* We should still use fmaak/fmamk if the two literals are identical. */
   //! v1: %res13 = v_fmaak_f32 0x40400000, %a, 0x40400000
   //! p_unit_test 13, %res13
                  END_TEST
      BEGIN_TEST(optimize.fma_opsel)
      /* TODO make these work before GFX11 using SDWA. */
   for (unsigned i = GFX11; i <= GFX11; i++) {
      //>> v2b: %a, v2b: %b, v1: %c, v1: %d, v1: %e  = p_startpgm
   if (!setup_cs("v2b v2b v1 v1 v1", (amd_gfx_level)i))
            Temp a = inputs[0];
   Temp b = inputs[1];
   Temp c = inputs[2];
   Temp d = inputs[3];
   Temp e = inputs[4];
   Temp c_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), c, Operand::c32(1));
   Temp d_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), d, Operand::c32(1));
            //! v2b: %res0 = v_fma_f16 %b, hi(%c), %a
   //! p_unit_test 0, %res0
            //! v2b: %res1 = v_fma_f16 %a, %b, hi(%d)
   //! p_unit_test 1, %res1
            //! v2b: %res2 = v_fma_f16 %a, %b, hi(%e)
   //! p_unit_test 2, %res2
                  END_TEST
      BEGIN_TEST(optimize.dpp_opsel)
      //>> v1: %a, v1: %b = p_startpgm
   if (!setup_cs("v1  v1", GFX11))
            Temp a = inputs[0];
            Temp dpp16 = bld.vop1_dpp(aco_opcode::v_mov_b32, bld.def(v1), a, dpp_row_mirror);
   Temp dpp16_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), dpp16, Operand::c32(1));
   Temp dpp8 = bld.vop1_dpp8(aco_opcode::v_mov_b32, bld.def(v1), a);
            Temp b_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), b, Operand::c32(1));
            //! v2b: %res0 = v_add_f16 hi(%a), hi(%b) row_mirror bound_ctrl:1 fi
   //! p_unit_test 0, %res0
            //! v2b: %res1 = v_add_f16 hi(%a), %b dpp8:[0,0,0,0,0,0,0,0] fi
   //! p_unit_test 1, %res1
               END_TEST
      BEGIN_TEST(optimize.apply_sgpr_swap_opsel)
      //>> v1: %a, s1: %b = p_startpgm
   if (!setup_cs("v1  s1", GFX11))
            Temp a = inputs[0];
            Temp b_vgpr = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), bld.copy(bld.def(v1), b),
            Temp res0 = bld.tmp(v2b);
   VALU_instruction& valu = bld.vop2(aco_opcode::v_sub_f16, Definition(res0), a, b_vgpr)->valu();
            //! v2b: %res0 = v_subrev_f16 %b, hi(%a)
   //! p_unit_test 0, %res0
               END_TEST
      BEGIN_TEST(optimize.combine_comparison_ordering)
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", GFX11))
            Temp a = inputs[0];
   Temp b = inputs[1];
            Temp a_unordered = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), a, a);
   Temp b_unordered = bld.vopc(aco_opcode::v_cmp_neq_f32, bld.def(bld.lm), b, b);
   Temp unordered =
            Temp a_lt_a = bld.vopc(aco_opcode::v_cmp_lt_f32, bld.def(bld.lm), a, a);
   Temp unordered_cmp =
            //! s2: %res0_unordered = v_cmp_u_f32 %a, %b
   //! s2: %res0_cmp = v_cmp_lt_f32 %a, %a
   //! s2: %res0,  s2: %_:scc = s_or_b64 %res0_unordered, %res0_cmp
   //! p_unit_test 0, %res0
                     Temp c_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), c, c);
   Temp c_hi_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), c_hi, c_hi);
   unordered =
            Temp c_lt_c_hi = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), c, c_hi);
   unordered_cmp =
            //! s2: %res1 = v_cmp_nge_f16 %c, hi(%c)
   //! p_unit_test 1, %res1
               END_TEST
      BEGIN_TEST(optimize.combine_comparison_ordering_opsel)
      //>> v1: %a, v2b: %b = p_startpgm
   if (!setup_cs("v1  v2b", GFX11))
            Temp a = inputs[0];
                     Temp ahi_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), a_hi, a_hi);
   Temp b_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), b, b);
   Temp unordered =
            Temp ahi_lt_b = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), a_hi, b);
   Temp unordered_cmp =
            //! s2: %res0 = v_cmp_nge_f16 hi(%a), %b
   //! p_unit_test 0, %res0
            Temp ahi_cmp_const = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), a_hi,
         Temp ahi_ucmp_const =
         //! s2: %res1 = v_cmp_nle_f16 4.0, hi(%a)
   //! p_unit_test 1, %res1
            a_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), a, Operand::c32(1));
   ahi_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), a_hi, a_hi);
   b_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), b, b);
   unordered =
         Temp alo_lt_b = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), a, b);
   Temp noopt = bld.sop2(Builder::s_or, bld.def(bld.lm), bld.def(bld.lm, scc), unordered, alo_lt_b);
   //! s2: %u2 = v_cmp_u_f16 hi(%a), %b
   //! s2: %cmp2 = v_cmp_lt_f16 %a, %b
   //! s2: %res2,  s2: %scc2:scc = s_or_b64 %u2, %cmp2
   //! p_unit_test 2, %res2
            Temp hi_neq_lo = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), a, a_hi);
   Temp a_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), a, a);
   noopt = bld.sop2(Builder::s_or, bld.def(bld.lm), bld.def(bld.lm, scc), hi_neq_lo, a_unordered);
   //! s2: %nan31 = v_cmp_neq_f16 %a, hi(%a)
   //! s2: %nan32 = v_cmp_neq_f16 %a, %a
   //! s2: %res3,  s2: %scc3:scc = s_or_b64 %nan31, %nan32
   //! p_unit_test 3, %res3
            ahi_cmp_const = bld.vopc(aco_opcode::v_cmp_lt_f16, bld.def(bld.lm), a_hi,
         a_unordered = bld.vopc(aco_opcode::v_cmp_neq_f16, bld.def(bld.lm), a, a);
   noopt =
         //! s2: %cmp4 = v_cmp_gt_f16 4.0, hi(%a)
   //! s2: %nan4 = v_cmp_neq_f16 %a, %a
   //! s2: %res4,  s2: %scc4:scc = s_or_b64 %nan4, %cmp4
   //! p_unit_test 4, %res4
               END_TEST
      BEGIN_TEST(optimize.max3_opsel)
      /* TODO make these work before GFX11 using SDWA. */
   for (unsigned i = GFX11; i <= GFX11; i++) {
      //>> v1: %a, v1: %b, v2b: %c = p_startpgm
   if (!setup_cs("v1  v1 v2b", GFX11))
            Temp a = inputs[0];
   Temp b = inputs[1];
            Temp a_hi = bld.pseudo(aco_opcode::p_extract_vector, bld.def(v2b), a, Operand::c32(1));
            //! v2b: %res0 = v_max3_f16 hi(%a), %c, hi(%b)
   //! p_unit_test 0, %res0
   writeout(0, bld.vop2(aco_opcode::v_max_f16, bld.def(v2b),
                  END_TEST
      BEGIN_TEST(optimize.neg_mul_opsel)
      //>> v1: %a, v2b: %b = p_startpgm
   if (!setup_cs("v1  v2b", GFX11))
            Temp a = inputs[0];
                     //! v2b: %res0 = v_mul_f16 -hi(%a), %b
   //! p_unit_test 0, %res0
            //! v1: %res1 = v_fma_mix_f32 -hi(%a), lo(%b), -0
   //! p_unit_test 1, %res1
               END_TEST
      BEGIN_TEST(optimize.vinterp_inreg_output_modifiers)
      //>> v1: %a, v1: %b, v1: %c = p_startpgm
   if (!setup_cs("v1 v1 v1", GFX11))
            //! v1: %res0 = v_interp_p2_f32_inreg %a, %b, %c clamp
   //! p_unit_test 0, %res0
   Temp tmp = bld.vinterp_inreg(aco_opcode::v_interp_p2_f32_inreg, bld.def(v1), inputs[0],
                  //! v1: %res1 = v_fma_f32 %b, %a, %c *2 quad_perm:[2,2,2,2] fi
   //! p_unit_test 1, %res1
   tmp = bld.vinterp_inreg(aco_opcode::v_interp_p2_f32_inreg, bld.def(v1), inputs[1], inputs[0],
         tmp = bld.vop2(aco_opcode::v_mul_f32, bld.def(v1), Operand::c32(0x40000000u), tmp);
            //! v2b: %res2 = v_interp_p2_f16_f32_inreg %a, %b, %c clamp
   //! p_unit_test 2, %res2
   tmp = bld.vinterp_inreg(aco_opcode::v_interp_p2_f16_f32_inreg, bld.def(v2b), inputs[0],
                  //! v2b: %tmp3 = v_interp_p2_f16_f32_inreg %b, %a, %c
   //! v2b: %res3 = v_mul_f16 2.0, %tmp3
   //! p_unit_test 3, %res3
   tmp = bld.vinterp_inreg(aco_opcode::v_interp_p2_f16_f32_inreg, bld.def(v2b), inputs[1],
         tmp = bld.vop2(aco_opcode::v_mul_f16, bld.def(v2b), Operand::c16(0x4000u), tmp);
            //! v2b: %res4 = v_fma_mixlo_f16 %c, %b, %a quad_perm:[2,2,2,2] fi
   //! p_unit_test 4, %res4
   tmp = bld.vinterp_inreg(aco_opcode::v_interp_p2_f32_inreg, bld.def(v1), inputs[2], inputs[1],
                     END_TEST
