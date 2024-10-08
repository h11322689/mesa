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
      BEGIN_TEST(regalloc.subdword_alloc.reuse_16bit_operands)
      /* Registers of operands should be "recycled" for the output. But if the
   * input is smaller than the output, that's not generally possible. The
   * first v_cvt_f32_f16 instruction below uses the upper 16 bits of v0
   * while the lower 16 bits are still live, so the output must be stored in
   * a register other than v0. For the second v_cvt_f32_f16, the original
   * value stored in v0 is no longer used and hence it's safe to store the
   * result in v0.
            /* TODO: is this possible to do on GFX11? */
   for (amd_gfx_level cc = GFX8; cc <= GFX10_3; cc = (amd_gfx_level)((unsigned)cc + 1)) {
      for (bool pessimistic : {false, true}) {
               //>> v1: %_:v[#a] = p_startpgm
                  //! v2b: %_:v[#a][0:16], v2b: %res1:v[#a][16:32] = p_split_vector %_:v[#a]
                  //! v1: %_:v[#b] = v_cvt_f32_f16 %_:v[#a][16:32] dst_sel:dword src0_sel:uword1
   //! v1: %_:v[#a] = v_cvt_f32_f16 %_:v[#a][0:16]
   //; success = (b != a)
   auto result1 = bld.vop1(aco_opcode::v_cvt_f32_f16, bld.def(v1), tmp.def(1).getTemp());
   auto result2 = bld.vop1(aco_opcode::v_cvt_f32_f16, bld.def(v1), tmp.def(0).getTemp());
                           END_TEST
      BEGIN_TEST(regalloc._32bit_partial_write)
      //>> v1: %_:v[0] = p_startpgm
   if (!setup_cs("v1", GFX10))
            /* ensure high 16 bits are occupied */
   //! v2b: %_:v[0][0:16], v2b: %_:v[0][16:32] = p_split_vector %_:v[0]
   Temp hi =
            /* This test checks if this instruction uses SDWA. */
   //! v2b: %_:v[0][0:16] = v_not_b32 0 dst_sel:uword0 dst_preserve src0_sel:dword
            //! v1: %_:v[0] = p_create_vector %_:v[0][0:16], %_:v[0][16:32]
               END_TEST
      BEGIN_TEST(regalloc.precolor.swap)
      //>> s2: %op0:s[0-1] = p_startpgm
   if (!setup_cs("s2", GFX10))
                     //! s2: %op1:s[2-3] = p_unit_test
            //! s2: %op0_2:s[2-3], s2: %op1_2:s[0-1] = p_parallelcopy %op0:s[0-1], %op1:s[2-3]
   //! p_unit_test %op0_2:s[2-3], %op1_2:s[0-1]
   Operand op(inputs[0]);
   op.setFixed(PhysReg(2));
               END_TEST
      BEGIN_TEST(regalloc.precolor.blocking_vector)
      //>> s2: %tmp0:s[0-1], s1: %tmp1:s[2] = p_startpgm
   if (!setup_cs("s2 s1", GFX10))
            //! s1: %tmp1_2:s[1], s2: %tmp0_2:s[2-3] = p_parallelcopy %tmp1:s[2], %tmp0:s[0-1]
   //! p_unit_test %tmp1_2:s[1]
   Operand op(inputs[1]);
   op.setFixed(PhysReg(1));
            //! p_unit_test %tmp0_2:s[2-3]
               END_TEST
      BEGIN_TEST(regalloc.precolor.vector.test)
      //>> s2: %tmp0:s[0-1], s1: %tmp1:s[2], s1: %tmp2:s[3] = p_startpgm
   if (!setup_cs("s2 s1 s1", GFX10))
            //! s2: %tmp0_2:s[2-3], s1: %tmp2_2:s[0] = p_parallelcopy %tmp0:s[0-1], %tmp2:s[3]
   //! p_unit_test %tmp0_2:s[2-3]
   Operand op(inputs[0]);
   op.setFixed(PhysReg(2));
            //! p_unit_test %tmp2_2:s[0]
               END_TEST
      BEGIN_TEST(regalloc.precolor.vector.collect)
      //>> s2: %tmp0:s[0-1], s1: %tmp1:s[2], s1: %tmp2:s[3] = p_startpgm
   if (!setup_cs("s2 s1 s1", GFX10))
            //! s2: %tmp0_2:s[2-3], s1: %tmp1_2:s[0], s1: %tmp2_2:s[1] = p_parallelcopy %tmp0:s[0-1], %tmp1:s[2], %tmp2:s[3]
   //! p_unit_test %tmp0_2:s[2-3]
   Operand op(inputs[0]);
   op.setFixed(PhysReg(2));
            //! p_unit_test %tmp1_2:s[0], %tmp2_2:s[1]
               END_TEST
      BEGIN_TEST(regalloc.precolor.vgpr_move)
      //>> v1: %tmp0:v[0], v1: %tmp1:v[1] = p_startpgm
   if (!setup_cs("v1 v1", GFX10))
            //! v1: %tmp1_2:v[0], v1: %tmp0_2:v[1] = p_parallelcopy %tmp1:v[1], %tmp0:v[0]
   //! p_unit_test %tmp0_2:v[1], %tmp1_2:v[0]
               END_TEST
      BEGIN_TEST(regalloc.precolor.multiple_operands)
      //>> v1: %tmp0:v[0], v1: %tmp1:v[1], v1: %tmp2:v[2], v1: %tmp3:v[3] = p_startpgm
   if (!setup_cs("v1 v1 v1 v1", GFX10))
            //! v1: %tmp3_2:v[0], v1: %tmp0_2:v[1], v1: %tmp1_2:v[2], v1: %tmp2_2:v[3] = p_parallelcopy %tmp3:v[3], %tmp0:v[0], %tmp1:v[1], %tmp2:v[2]
   //! p_unit_test %tmp3_2:v[0], %tmp0_2:v[1], %tmp1_2:v[2], %tmp2_2:v[3]
   bld.pseudo(aco_opcode::p_unit_test, Operand(inputs[3], PhysReg(256 + 0)),
                     END_TEST
      BEGIN_TEST(regalloc.precolor.different_regs)
      //>> v1: %tmp0:v[0] = p_startpgm
   if (!setup_cs("v1", GFX10))
            //! v1: %tmp1:v[1], v1: %tmp2:v[2] = p_parallelcopy %tmp0:v[0], %tmp0:v[0]
   //! p_unit_test %tmp0:v[0], %tmp1:v[1], %tmp2:v[2]
   bld.pseudo(aco_opcode::p_unit_test, Operand(inputs[0], PhysReg(256 + 0)),
               END_TEST
      BEGIN_TEST(regalloc.scratch_sgpr.create_vector)
      if (!setup_cs("v1 s1", GFX7))
                     //>> v3b: %0:v[0][0:24] = v_and_b32 0xffffff, %0:v[0][0:24]
   //! s1: %0:s[1] = s_mov_b32 0x1000001
   //! v1: %0:v[0] = v_mul_lo_u32 %0:s[1], %_:v[0][0:8]
            //! p_unit_test %_:s[0]
   //! s_endpgm
               END_TEST
      BEGIN_TEST(regalloc.scratch_sgpr.create_vector_sgpr_operand)
      if (!setup_cs("v2 s1", GFX7))
                     //>> v1: %0:v[0] = v_mov_b32 %_:s[0]
   //! v3b: %0:v[1][0:24] = v_and_b32 0xffffff, %0:v[1][0:24]
   //! s1: %0:s[1] = s_mov_b32 0x1000001
   //! v1: %0:v[1] = v_mul_lo_u32 %0:s[1], %_:v[1][0:8]
            //! p_unit_test %_:s[0]
   //! s_endpgm
               END_TEST
      BEGIN_TEST(regalloc.linear_vgpr.live_range_split.fixed_def)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
                     //! lv1: %tmp1:v[0] = p_unit_test
            //! lv1: %tmp2:v[1] = p_parallelcopy %tmp1:v[0]
   //! v1: %_:v[0] = p_unit_test
            //! p_unit_test %tmp2:v[1]
               END_TEST
      BEGIN_TEST(regalloc.linear_vgpr.live_range_split.get_reg_impl)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
                              //! s1: %scc_tmp:scc, s1: %1:s[0] = p_unit_test
   Temp s0_tmp = bld.tmp(s1);
   Temp scc_tmp = bld.pseudo(aco_opcode::p_unit_test, bld.def(s1, scc),
            //! lv1: %tmp1:v[1] = p_unit_test
            //! lv1: %tmp2:v[2] = p_parallelcopy %tmp1:v[1]
   //! v2: %_:v[0-1] = p_unit_test
            //! p_unit_test %tmp2:v[2], %scc_tmp:scc, %1:s[0]
                     //>> lv1: %5:v[2] = p_parallelcopy %3:v[1] scc:1 scratch:s1
   Pseudo_instruction& parallelcopy = program->blocks[0].instructions[3]->pseudo();
   aco_print_instr(program->gfx_level, &parallelcopy, output);
   fprintf(output, " scc:%u scratch:s%u\n", parallelcopy.tmp_in_scc,
      END_TEST
      BEGIN_TEST(regalloc.linear_vgpr.live_range_split.get_regs_for_copies)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
                     PhysReg reg_v2{258};
            //! lv1: %lin_tmp1:v[4] = p_unit_test
   Temp lin_tmp = bld.pseudo(aco_opcode::p_unit_test, bld.def(v1.as_linear(), reg_v4));
   //! v2: %log_tmp1:v[2-3] = p_unit_test
            //! lv1: %lin_tmp2:v[0], v2: %log_tmp2:v[4-5] = p_parallelcopy %lin_tmp1:v[4], %log_tmp1:v[2-3]
   //! v3: %_:v[1-3] = p_unit_test
            //! p_unit_test %log_tmp2:v[4-5], %lin_tmp2:v[0]
               END_TEST
      BEGIN_TEST(regalloc.linear_vgpr.live_range_split.get_reg_create_vector)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
                     PhysReg reg_v0{256};
            //! lv1: %lin_tmp1:v[0] = p_unit_test
   Temp lin_tmp = bld.pseudo(aco_opcode::p_unit_test, bld.def(v1.as_linear(), reg_v0));
   //! v1: %log_tmp:v[1] = p_unit_test
            //! lv1: %lin_tmp2:v[2] = p_parallelcopy %lin_tmp1:v[0]
   //! v2: %_:v[0-1] = p_create_vector v1: undef, %log_tmp:v[1]
            //! p_unit_test %lin_tmp2:v[2]
               END_TEST
      BEGIN_TEST(regalloc.branch_def_phis_at_merge_block)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
            //! s2: %_:s[2-3] = p_branch
            //! BB1
   //! /* logical preds: / linear preds: BB0, / kind: uniform, */
   bld.reset(program->create_and_insert_block());
            //! s2: %tmp:s[0-1] = p_linear_phi 0
            //! p_unit_test %tmp:s[0-1]
               END_TEST
      BEGIN_TEST(regalloc.branch_def_phis_at_branch_block)
      //>> p_startpgm
   if (!setup_cs("", GFX10))
            //! s2: %tmp:s[0-1] = p_unit_test
            //! s2: %_:s[2-3] = p_cbranch_z %0:scc
            //! BB1
   //! /* logical preds: / linear preds: BB0, / kind: */
   bld.reset(program->create_and_insert_block());
            //! p_unit_test %tmp:s[0-1]
   bld.pseudo(aco_opcode::p_unit_test, tmp);
            bld.reset(program->create_and_insert_block());
                     bld.reset(program->create_and_insert_block());
   program->blocks[3].linear_preds.push_back(1);
               END_TEST
      BEGIN_TEST(regalloc.vinterp_fp16)
      //>> v1: %in0:v[0], v1: %in1:v[1], v1: %in2:v[2] = p_startpgm
   if (!setup_cs("v1 v1 v1", GFX11))
            //! v2b: %lo:v[3][0:16], v2b: %hi:v[3][16:32] = p_split_vector %in0:v[0]
   Temp lo = bld.tmp(v2b);
   Temp hi = bld.tmp(v2b);
            //! v1: %tmp0:v[1] = v_interp_p10_f16_f32_inreg %lo:v[3][0:16], %in1:v[1], hi(%hi:v[3][16:32])
   //! p_unit_test %tmp0:v[1]
   Temp tmp0 =
                  //! v2b: %tmp1:v[0][16:32] = v_interp_p2_f16_f32_inreg %in0:v[0], %in2:v[2], %tmp0:v[1] opsel_hi
   //! v1: %tmp2:v[0] = p_create_vector 0, %tmp1:v[0][16:32]
   //! p_unit_test %tmp2:v[0]
   Temp tmp1 = bld.vinterp_inreg(aco_opcode::v_interp_p2_f16_f32_inreg, bld.def(v2b), inputs[0],
         Temp tmp2 = bld.pseudo(aco_opcode::p_create_vector, bld.def(v1), Operand::zero(2), tmp1);
               END_TEST
