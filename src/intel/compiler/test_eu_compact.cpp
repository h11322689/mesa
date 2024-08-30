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
      #include <stdlib.h>
   #include <stdio.h>
   #include <stdbool.h>
   #include "util/ralloc.h"
   #include "brw_eu.h"
      #include <gtest/gtest.h>
      struct CompactParams {
      unsigned verx10;
      };
      std::string
   get_compact_params_name(const testing::TestParamInfo<CompactParams> p)
   {
      CompactParams params = p.param;
   std::stringstream ss;
   ss << params.verx10 << "_";
   switch (params.align) {
   case BRW_ALIGN_1:
      ss << "Align_1";
      case BRW_ALIGN_16:
      ss << "Align_16";
      default:
         }
      }
      static bool
   test_compact_instruction(struct brw_codegen *p, brw_inst src)
   {
      brw_compact_inst dst;
            if (brw_try_compact_instruction(p->isa, &dst, &src)) {
               brw_uncompact_instruction(p->isa, &uncompacted, &dst);
   brw_debug_compact_uncompact(p->isa, &src, &uncompacted);
   return false;
            } else {
      brw_compact_inst unchanged;
   memset(&unchanged, 0xd0, sizeof(unchanged));
   /* It's not supposed to change dst unless it compacted. */
   fprintf(stderr, "Failed to compact, but dst changed\n");
   fprintf(stderr, "  Instruction: ");
   brw_disassemble_inst(stderr, p->isa, &src, false, 0, NULL);
   return false;
                        }
      /**
   * When doing fuzz testing, pad bits won't round-trip.
   *
   * This sort of a superset of skip_bit, which is testing for changing bits that
   * aren't worth testing for fuzzing.  We also just want to clear bits that
   * become meaningless once fuzzing twiddles a related bit.
   */
   static void
   clear_pad_bits(const struct brw_isa_info *isa, brw_inst *inst)
   {
               if (brw_inst_opcode(isa, inst) != BRW_OPCODE_SEND &&
      brw_inst_opcode(isa, inst) != BRW_OPCODE_SENDC &&
   brw_inst_src0_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE &&
   brw_inst_src1_reg_file(devinfo, inst) != BRW_IMMEDIATE_VALUE) {
               if (devinfo->ver == 8 && devinfo->platform != INTEL_PLATFORM_CHV &&
      is_3src(isa, brw_inst_opcode(isa, inst))) {
   brw_inst_set_bits(inst, 105, 105, 0);
   brw_inst_set_bits(inst, 84, 84, 0);
         }
      static bool
   skip_bit(const struct brw_isa_info *isa, brw_inst *src, int bit)
   {
               /* pad bit */
   if (bit == 7)
            /* The compact bit -- uncompacted can't have it set. */
   if (bit == 29)
            if (is_3src(isa, brw_inst_opcode(isa, src))) {
      if (devinfo->ver >= 9 || devinfo->platform == INTEL_PLATFORM_CHV) {
      if (bit == 127)
      } else {
                                                   if (bit >= 35 && bit <= 36)
         } else {
      if (bit == 47)
            if (devinfo->ver >= 8) {
                     if (bit == 95)
      } else {
                     if (bit >= 91 && bit <= 95)
                  /* sometimes these are pad bits. */
   if (brw_inst_opcode(isa, src) != BRW_OPCODE_SEND &&
      brw_inst_opcode(isa, src) != BRW_OPCODE_SENDC &&
   brw_inst_src0_reg_file(devinfo, src) != BRW_IMMEDIATE_VALUE &&
   brw_inst_src1_reg_file(devinfo, src) != BRW_IMMEDIATE_VALUE &&
   bit >= 121) {
                  }
      static bool
   test_fuzz_compact_instruction(struct brw_codegen *p, brw_inst src)
   {
      for (int bit0 = 0; bit0 < 128; bit0++) {
      continue;
            for (int bit1 = 0; bit1 < 128; bit1++) {
   uint64_t *bits = instr.data;
                     bits[bit0 / 64] ^= (1ull << (bit0 & 63));
   bits[bit1 / 64] ^= (1ull << (bit1 & 63));
                              if (!test_compact_instruction(p, instr)) {
      printf("  twiddled bits for fuzzing %d, %d\n", bit0, bit1);
      }
                        }
      class CompactTestFixture : public testing::TestWithParam<CompactParams> {
   protected:
      virtual void SetUp() {
      CompactParams params = GetParam();
   mem_ctx = ralloc_context(NULL);
   devinfo = rzalloc(mem_ctx, intel_device_info);
            devinfo->verx10 = params.verx10;
            brw_init_isa_info(&isa, devinfo);
   brw_init_codegen(&isa, p, p);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
               virtual void TearDown() {
      EXPECT_EQ(p->nr_insn, 1);
   EXPECT_TRUE(test_compact_instruction(p, p->store[0]));
                        void *mem_ctx;
   struct brw_isa_info isa;
   intel_device_info *devinfo;
      };
      class Instructions : public CompactTestFixture {};
      INSTANTIATE_TEST_SUITE_P(
      CompactTest,
   Instructions,
   testing::Values(
      CompactParams{ 50,  BRW_ALIGN_1 }, CompactParams{ 50, BRW_ALIGN_16 },
   CompactParams{ 60,  BRW_ALIGN_1 }, CompactParams{ 60, BRW_ALIGN_16 },
   CompactParams{ 70,  BRW_ALIGN_1 }, CompactParams{ 70, BRW_ALIGN_16 },
   CompactParams{ 75,  BRW_ALIGN_1 }, CompactParams{ 75, BRW_ALIGN_16 },
   CompactParams{ 80,  BRW_ALIGN_1 }, CompactParams{ 80, BRW_ALIGN_16 },
   CompactParams{ 90,  BRW_ALIGN_1 }, CompactParams{ 90, BRW_ALIGN_16 },
   CompactParams{ 110, BRW_ALIGN_1 },
   CompactParams{ 120, BRW_ALIGN_1 },
      ),
         class InstructionsBeforeIvyBridge : public CompactTestFixture {};
      INSTANTIATE_TEST_SUITE_P(
      CompactTest,
   InstructionsBeforeIvyBridge,
   testing::Values(
      CompactParams{ 50,  BRW_ALIGN_1 }, CompactParams{ 50, BRW_ALIGN_16 },
      ),
            TEST_P(Instructions, ADD_GRF_GRF_GRF)
   {
      struct brw_reg g0 = brw_vec8_grf(0, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);
               }
      TEST_P(Instructions, ADD_GRF_GRF_IMM)
   {
      struct brw_reg g0 = brw_vec8_grf(0, 0);
               }
      TEST_P(Instructions, ADD_GRF_GRF_IMM_d)
   {
      struct brw_reg g0 = retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_D);
               }
      TEST_P(Instructions, MOV_GRF_GRF)
   {
      struct brw_reg g0 = brw_vec8_grf(0, 0);
               }
      TEST_P(InstructionsBeforeIvyBridge, ADD_MRF_GRF_GRF)
   {
      struct brw_reg m6 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 6, 0);
   struct brw_reg g2 = brw_vec8_grf(2, 0);
               }
      TEST_P(Instructions, ADD_vec1_GRF_GRF_GRF)
   {
      struct brw_reg g0 = brw_vec1_grf(0, 0);
   struct brw_reg g2 = brw_vec1_grf(2, 0);
               }
      TEST_P(InstructionsBeforeIvyBridge, PLN_MRF_GRF_GRF)
   {
      struct brw_reg m6 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 6, 0);
   struct brw_reg interp = brw_vec1_grf(2, 0);
               }
      TEST_P(Instructions, f0_0_MOV_GRF_GRF)
   {
      struct brw_reg g0 = brw_vec8_grf(0, 0);
            brw_push_insn_state(p);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NORMAL);
   brw_MOV(p, g0, g2);
      }
      /* The handling of f0.1 vs f0.0 changes between gfx6 and gfx7.  Explicitly test
   * it, so that we run the fuzzing can run over all the other bits that might
   * interact with it.
   */
   TEST_P(Instructions, f0_1_MOV_GRF_GRF)
   {
      struct brw_reg g0 = brw_vec8_grf(0, 0);
            brw_push_insn_state(p);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NORMAL);
   brw_inst *mov = brw_MOV(p, g0, g2);
   brw_inst_set_flag_subreg_nr(p->devinfo, mov, 1);
      }
