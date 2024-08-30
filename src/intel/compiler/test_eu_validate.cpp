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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <gtest/gtest.h>
   #include "brw_eu.h"
   #include "brw_eu_defines.h"
   #include "util/bitset.h"
   #include "util/ralloc.h"
      static const struct intel_gfx_info {
         } gfx_names[] = {
      { "brw", },
   { "g4x", },
   { "ilk", },
   { "snb", },
   { "ivb", },
   { "hsw", },
   { "byt", },
   { "bdw", },
   { "chv", },
   { "skl", },
   { "bxt", },
   { "kbl", },
   { "aml", },
   { "glk", },
   { "cfl", },
   { "whl", },
   { "cml", },
   { "icl", },
   { "ehl", },
   { "jsl", },
   { "tgl", },
   { "rkl", },
   { "dg1", },
   { "adl", },
   { "sg1", },
   { "rpl", },
   { "dg2", },
      };
      class validation_test: public ::testing::TestWithParam<struct intel_gfx_info> {
            public:
      validation_test();
            struct brw_isa_info isa;
   struct brw_codegen *p;
      };
      validation_test::validation_test()
   {
      p = rzalloc(NULL, struct brw_codegen);
      }
      validation_test::~validation_test()
   {
         }
      void validation_test::SetUp()
   {
      struct intel_gfx_info info = GetParam();
                                 }
      struct gfx_name {
      template <class ParamType>
   std::string
   operator()(const ::testing::TestParamInfo<ParamType>& info) const {
            };
      INSTANTIATE_TEST_SUITE_P(
      eu_assembly, validation_test,
   ::testing::ValuesIn(gfx_names),
      );
      static bool
   validate(struct brw_codegen *p)
   {
      const bool print = getenv("TEST_DEBUG");
            if (print) {
      disasm_new_inst_group(disasm, 0);
               bool ret = brw_validate_instructions(p->isa, p->store, 0,
            if (print) {
         }
               }
      #define last_inst    (&p->store[p->nr_insn - 1])
   #define g0           brw_vec8_grf(0, 0)
   #define acc0         brw_acc_reg(8)
   #define null         brw_null_reg()
   #define zero         brw_imm_f(0.0f)
      static void
   clear_instructions(struct brw_codegen *p)
   {
      p->next_insn_offset = 0;
      }
      TEST_P(validation_test, sanity)
   {
                  }
      TEST_P(validation_test, src0_null_reg)
   {
                  }
      TEST_P(validation_test, src1_null_reg)
   {
                  }
      TEST_P(validation_test, math_src0_null_reg)
   {
      if (devinfo.ver >= 6) {
         } else {
                     }
      TEST_P(validation_test, math_src1_null_reg)
   {
      if (devinfo.ver >= 6) {
      gfx6_math(p, g0, BRW_MATH_FUNCTION_POW, g0, null);
      } else {
      /* Math instructions on Gfx4/5 are actually SEND messages with payloads.
   * src1 is an immediate message descriptor set by gfx4_math.
         }
      TEST_P(validation_test, opcode46)
   {
      /* opcode 46 is "push" on Gen 4 and 5
   *              "fork" on Gen 6
   *              reserved on Gen 7
   *              "goto" on Gfx8+
   */
            if (devinfo.ver == 7) {
         } else {
            }
      TEST_P(validation_test, invalid_exec_size_encoding)
   {
      const struct {
      enum brw_execution_size exec_size;
      } test_case[] = {
      { BRW_EXECUTE_1,      true  },
   { BRW_EXECUTE_2,      true  },
   { BRW_EXECUTE_4,      true  },
   { BRW_EXECUTE_8,      true  },
   { BRW_EXECUTE_16,     true  },
            { (enum brw_execution_size)((int)BRW_EXECUTE_32 + 1), false },
               for (unsigned i = 0; i < ARRAY_SIZE(test_case); i++) {
               brw_inst_set_exec_size(&devinfo, last_inst, test_case[i].exec_size);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
            if (test_case[i].exec_size == BRW_EXECUTE_1) {
      brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
      } else {
      brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_2);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_2);
                              }
      TEST_P(validation_test, invalid_file_encoding)
   {
      /* Register file on Gfx12 is only one bit */
   if (devinfo.ver >= 12)
            brw_MOV(p, g0, g0);
            if (devinfo.ver > 6) {
         } else {
                           if (devinfo.ver < 6) {
         } else {
         }
            if (devinfo.ver > 6) {
         } else {
            }
      TEST_P(validation_test, invalid_type_encoding)
   {
      enum brw_reg_file files[2] = {
      BRW_GENERAL_REGISTER_FILE,
               for (unsigned i = 0; i < ARRAY_SIZE(files); i++) {
      const enum brw_reg_file file = files[i];
   const int num_bits = devinfo.ver >= 8 ? 4 : 3;
            /* The data types are encoded into <num_bits> bits to be used in hardware
   * instructions, so keep a record in a bitset the invalid patterns so
   * they can be verified to be invalid when used.
   */
            const struct {
      enum brw_reg_type type;
      } test_case[] = {
      { BRW_REGISTER_TYPE_NF, devinfo.ver == 11 && file != IMM },
   { BRW_REGISTER_TYPE_DF, devinfo.has_64bit_float && (devinfo.ver >= 8 || file != IMM) },
   { BRW_REGISTER_TYPE_F,  true },
   { BRW_REGISTER_TYPE_HF, devinfo.ver >= 8 },
   { BRW_REGISTER_TYPE_VF, file == IMM },
   { BRW_REGISTER_TYPE_Q,  devinfo.has_64bit_int },
   { BRW_REGISTER_TYPE_UQ, devinfo.has_64bit_int },
   { BRW_REGISTER_TYPE_D,  true },
   { BRW_REGISTER_TYPE_UD, true },
   { BRW_REGISTER_TYPE_W,  true },
   { BRW_REGISTER_TYPE_UW, true },
   { BRW_REGISTER_TYPE_B,  file == FIXED_GRF },
   { BRW_REGISTER_TYPE_UB, file == FIXED_GRF },
   { BRW_REGISTER_TYPE_V,  file == IMM },
               /* Initially assume all hardware encodings are invalid */
                     for (unsigned i = 0; i < ARRAY_SIZE(test_case); i++) {
      if (test_case[i].expected_result) {
      unsigned hw_type = brw_reg_type_to_hw_type(&devinfo, file, test_case[i].type);
   if (hw_type != INVALID_REG_TYPE) {
      /* ... and remove valid encodings from the set */
                     if (file == FIXED_GRF) {
      struct brw_reg g = retype(g0, test_case[i].type);
   brw_MOV(p, g, g);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
                        switch (test_case[i].type) {
   case BRW_REGISTER_TYPE_V:
      t = BRW_REGISTER_TYPE_W;
      case BRW_REGISTER_TYPE_UV:
      t = BRW_REGISTER_TYPE_UW;
      case BRW_REGISTER_TYPE_VF:
      t = BRW_REGISTER_TYPE_F;
      default:
                                                                  /* The remaining encodings in invalid_encodings do not have a mapping
   * from BRW_REGISTER_TYPE_* and must be invalid. Verify that invalid
   * encodings are rejected by the validator.
   */
   int e;
   BITSET_FOREACH_SET(e, invalid_encodings, num_encodings) {
      if (file == FIXED_GRF) {
      brw_MOV(p, g0, g0);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
      } else {
         }
                                    }
      TEST_P(validation_test, invalid_type_encoding_3src_a16)
   {
      /* 3-src instructions in align16 mode only supported on Gfx6-10 */
   if (devinfo.ver < 6 || devinfo.ver > 10)
            const int num_bits = devinfo.ver >= 8 ? 3 : 2;
            /* The data types are encoded into <num_bits> bits to be used in hardware
   * instructions, so keep a record in a bitset the invalid patterns so
   * they can be verified to be invalid when used.
   */
            const struct {
      enum brw_reg_type type;
      } test_case[] = {
      { BRW_REGISTER_TYPE_DF, devinfo.ver >= 7  },
   { BRW_REGISTER_TYPE_F,  true },
   { BRW_REGISTER_TYPE_HF, devinfo.ver >= 8  },
   { BRW_REGISTER_TYPE_D,  devinfo.ver >= 7  },
               /* Initially assume all hardware encodings are invalid */
            brw_set_default_access_mode(p, BRW_ALIGN_16);
            for (unsigned i = 0; i < ARRAY_SIZE(test_case); i++) {
      if (test_case[i].expected_result) {
      unsigned hw_type = brw_reg_type_to_a16_hw_3src_type(&devinfo, test_case[i].type);
   if (hw_type != INVALID_HW_REG_TYPE) {
      /* ... and remove valid encodings from the set */
   assert(BITSET_TEST(invalid_encodings, hw_type));
               struct brw_reg g = retype(g0, test_case[i].type);
   if (!brw_reg_type_is_integer(test_case[i].type)) {
         } else {
                                          /* The remaining encodings in invalid_encodings do not have a mapping
   * from BRW_REGISTER_TYPE_* and must be invalid. Verify that invalid
   * encodings are rejected by the validator.
   */
   int e;
   BITSET_FOREACH_SET(e, invalid_encodings, num_encodings) {
      for (unsigned i = 0; i < 2; i++) {
      if (i == 0) {
         } else {
                                                   if (devinfo.ver == 6)
            }
      TEST_P(validation_test, invalid_type_encoding_3src_a1)
   {
      /* 3-src instructions in align1 mode only supported on Gfx10+ */
   if (devinfo.ver < 10)
            const int num_bits = 3 + 1 /* for exec_type */;
            /* The data types are encoded into <num_bits> bits to be used in hardware
   * instructions, so keep a record in a bitset the invalid patterns so
   * they can be verified to be invalid when used.
   */
            const struct {
      enum brw_reg_type type;
   unsigned exec_type;
         #define E(x) ((unsigned)BRW_ALIGN1_3SRC_EXEC_TYPE_##x)
         { BRW_REGISTER_TYPE_NF, E(FLOAT), devinfo.ver == 11 },
   { BRW_REGISTER_TYPE_DF, E(FLOAT), devinfo.has_64bit_float },
   { BRW_REGISTER_TYPE_F,  E(FLOAT), true  },
   { BRW_REGISTER_TYPE_HF, E(FLOAT), true  },
   { BRW_REGISTER_TYPE_D,  E(INT),   true  },
   { BRW_REGISTER_TYPE_UD, E(INT),   true  },
   { BRW_REGISTER_TYPE_W,  E(INT),   true  },
            /* There are no ternary instructions that can operate on B-type sources
   * on Gfx11-12. Src1/Src2 cannot be B-typed either.
   */
   { BRW_REGISTER_TYPE_B,  E(INT),   false },
               /* Initially assume all hardware encodings are invalid */
            brw_set_default_access_mode(p, BRW_ALIGN_1);
            for (unsigned i = 0; i < ARRAY_SIZE(test_case); i++) {
      if (test_case[i].expected_result) {
      unsigned hw_type = brw_reg_type_to_a1_hw_3src_type(&devinfo, test_case[i].type);
   unsigned hw_exec_type = hw_type | (test_case[i].exec_type << 3);
   if (hw_type != INVALID_HW_REG_TYPE) {
      /* ... and remove valid encodings from the set */
   assert(BITSET_TEST(invalid_encodings, hw_exec_type));
               struct brw_reg g = retype(g0, test_case[i].type);
   if (!brw_reg_type_is_integer(test_case[i].type)) {
         } else {
                                          /* The remaining encodings in invalid_encodings do not have a mapping
   * from BRW_REGISTER_TYPE_* and must be invalid. Verify that invalid
   * encodings are rejected by the validator.
   */
   int e;
   BITSET_FOREACH_SET(e, invalid_encodings, num_encodings) {
      const unsigned hw_type = e & 0x7;
            for (unsigned i = 0; i < 2; i++) {
      if (i == 0) {
      brw_MAD(p, g0, g0, g0, g0);
      } else {
      brw_CSEL(p, g0, g0, g0, g0);
   brw_inst_set_3src_cond_modifier(&devinfo, last_inst, BRW_CONDITIONAL_NZ);
               brw_inst_set_3src_a1_exec_type(&devinfo, last_inst, exec_type);
   brw_inst_set_3src_a1_dst_hw_type (&devinfo, last_inst, hw_type);
   brw_inst_set_3src_a1_src0_hw_type(&devinfo, last_inst, hw_type);
                                    }
      TEST_P(validation_test, 3src_inst_access_mode)
   {
      /* 3-src instructions only supported on Gfx6+ */
   if (devinfo.ver < 6)
            /* No access mode bit on Gfx12+ */
   if (devinfo.ver >= 12)
            const struct {
      unsigned mode;
      } test_case[] = {
      { BRW_ALIGN_1,  devinfo.ver >= 10 },
               for (unsigned i = 0; i < ARRAY_SIZE(test_case); i++) {
      if (devinfo.ver < 10)
            brw_MAD(p, g0, g0, g0, g0);
                           }
      /* When the Execution Data Type is wider than the destination data type, the
   * destination must [...] specify a HorzStride equal to the ratio in sizes of
   * the two data types.
   */
   TEST_P(validation_test, dest_stride_must_be_equal_to_the_ratio_of_exec_size_to_dest_size)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
               }
      /* When the Execution Data Type is wider than the destination data type, the
   * destination must be aligned as required by the wider execution data type
   * [...]
   */
   TEST_P(validation_test, dst_subreg_must_be_aligned_to_exec_type_size)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 2);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_4);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 8);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_1);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
               }
      /* ExecSize must be greater than or equal to Width. */
   TEST_P(validation_test, exec_size_less_than_width)
   {
      brw_ADD(p, g0, g0, g0);
                              brw_ADD(p, g0, g0, g0);
               }
      /* If ExecSize = Width and HorzStride ≠ 0,
   * VertStride must be set to Width * HorzStride.
   */
   TEST_P(validation_test, vertical_stride_is_width_by_horizontal_stride)
   {
      brw_ADD(p, g0, g0, g0);
                              brw_ADD(p, g0, g0, g0);
               }
      /* If Width = 1, HorzStride must be 0 regardless of the values
   * of ExecSize and VertStride.
   */
   TEST_P(validation_test, horizontal_stride_must_be_0_if_width_is_1)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_1);
               }
      /* If ExecSize = Width = 1, both VertStride and HorzStride must be 0. */
   TEST_P(validation_test, scalar_region_must_be_0_1_0)
   {
               brw_ADD(p, g0, g0, g0_0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_1);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_1);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
                              brw_ADD(p, g0, g0_0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_1);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_1);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_1);
               }
      /* If VertStride = HorzStride = 0, Width must be 1 regardless of the value
   * of ExecSize.
   */
   TEST_P(validation_test, zero_stride_implies_0_1_0)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_2);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_2);
               }
      /* Dst.HorzStride must not be 0. */
   TEST_P(validation_test, dst_horizontal_stride_0)
   {
      brw_ADD(p, g0, g0, g0);
                              /* Align16 does not exist on Gfx11+ */
   if (devinfo.ver >= 11)
                     brw_ADD(p, g0, g0, g0);
               }
      /* VertStride must be used to cross BRW_GENERAL_REGISTER_FILE register boundaries. This rule implies
   * that elements within a 'Width' cannot cross BRW_GENERAL_REGISTER_FILE boundaries.
   */
   TEST_P(validation_test, must_not_cross_grf_boundary_in_a_width)
   {
      brw_ADD(p, g0, g0, g0);
                              brw_ADD(p, g0, g0, g0);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
               }
      /* Destination Horizontal must be 1 in Align16 */
   TEST_P(validation_test, dst_hstride_on_align16_must_be_1)
   {
      /* Align16 does not exist on Gfx11+ */
   if (devinfo.ver >= 11)
                     brw_ADD(p, g0, g0, g0);
                              brw_ADD(p, g0, g0, g0);
               }
      /* VertStride must be 0 or 4 in Align16 */
   TEST_P(validation_test, vstride_on_align16_must_be_0_or_4)
   {
      /* Align16 does not exist on Gfx11+ */
   if (devinfo.ver >= 11)
            const struct {
      enum brw_vertical_stride vstride;
      } vstride[] = {
      { BRW_VERTICAL_STRIDE_0, true },
   { BRW_VERTICAL_STRIDE_1, false },
   { BRW_VERTICAL_STRIDE_2, devinfo.verx10 >= 75 },
   { BRW_VERTICAL_STRIDE_4, true },
   { BRW_VERTICAL_STRIDE_8, false },
   { BRW_VERTICAL_STRIDE_16, false },
   { BRW_VERTICAL_STRIDE_32, false },
                        for (unsigned i = 0; i < ARRAY_SIZE(vstride); i++) {
      brw_ADD(p, g0, g0, g0);
                                 for (unsigned i = 0; i < ARRAY_SIZE(vstride); i++) {
      brw_ADD(p, g0, g0, g0);
                           }
      /* In Direct Addressing mode, a source cannot span more than 2 adjacent BRW_GENERAL_REGISTER_FILE
   * registers.
   */
   TEST_P(validation_test, source_cannot_span_more_than_2_registers)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_32);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_8);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_8);
   brw_inst_set_src1_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
                              brw_ADD(p, g0, g0, g0);
               }
      /* A destination cannot span more than 2 adjacent BRW_GENERAL_REGISTER_FILE registers. */
   TEST_P(validation_test, destination_cannot_span_more_than_2_registers)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_32);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_8);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 6);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_4);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_1);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
               }
      TEST_P(validation_test, src_region_spans_two_regs_dst_region_spans_one)
   {
      /* Writes to dest are to the lower OWord */
   brw_ADD(p, g0, g0, g0);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
                              /* Writes to dest are to the upper OWord */
   brw_ADD(p, g0, g0, g0);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
                              /* Writes to dest are evenly split between OWords */
   brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_8);
                              /* Writes to dest are uneven between OWords */
   brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_4);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 10);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_1);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_16);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_2);
            if (devinfo.ver >= 9) {
         } else {
            }
      TEST_P(validation_test, dst_elements_must_be_evenly_split_between_registers)
   {
      brw_ADD(p, g0, g0, g0);
            if (devinfo.ver >= 9 && devinfo.verx10 < 125) {
         } else {
                           brw_ADD(p, g0, g0, g0);
                              if (devinfo.ver >= 6) {
                                 gfx6_math(p, g0, BRW_MATH_FUNCTION_SIN, g0, null);
                  }
      TEST_P(validation_test, two_src_two_dst_source_offsets_must_be_same)
   {
      brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_4);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_4);
   brw_inst_set_src0_da1_subreg_nr(&devinfo, last_inst, 16);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_2);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_0);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
         if (devinfo.ver <= 7 || devinfo.verx10 >= 125) {
            } else {
                           brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_4);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_4);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_0);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_8);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_2);
            if (devinfo.verx10 >= 125)
         else
      }
      TEST_P(validation_test, two_src_two_dst_each_dst_must_be_derived_from_one_src)
   {
      brw_MOV(p, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_da1_subreg_nr(&devinfo, last_inst, 8);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
            if (devinfo.ver <= 7) {
         } else {
                           brw_MOV(p, g0, g0);
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, 16);
   brw_inst_set_src0_da1_subreg_nr(&devinfo, last_inst, 8);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_2);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_2);
            if (devinfo.ver <= 7 || devinfo.verx10 >= 125) {
         } else {
            }
      TEST_P(validation_test, one_src_two_dst)
   {
               brw_ADD(p, g0, g0_0, g0_0);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
                              brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
            if (devinfo.ver >= 8) {
         } else {
                           brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_D);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
            if (devinfo.ver >= 8) {
         } else {
                           brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_1);
            if (devinfo.ver >= 8) {
         } else {
                           brw_ADD(p, g0, g0, g0);
   brw_inst_set_exec_size(&devinfo, last_inst, BRW_EXECUTE_16);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
   brw_inst_set_dst_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_file_type(&devinfo, last_inst, BRW_GENERAL_REGISTER_FILE, BRW_REGISTER_TYPE_W);
   brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_0);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_1);
   brw_inst_set_src0_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_0);
            if (devinfo.ver >= 8) {
         } else {
            }
      TEST_P(validation_test, packed_byte_destination)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src_type;
   bool neg, abs, sat;
      } move[] = {
      { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UB, 0, 0, 0, true },
   { BRW_REGISTER_TYPE_B , BRW_REGISTER_TYPE_B , 0, 0, 0, true },
   { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_B , 0, 0, 0, true },
            { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UB, 1, 0, 0, false },
   { BRW_REGISTER_TYPE_B , BRW_REGISTER_TYPE_B , 1, 0, 0, false },
   { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_B , 1, 0, 0, false },
            { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UB, 0, 1, 0, false },
   { BRW_REGISTER_TYPE_B , BRW_REGISTER_TYPE_B , 0, 1, 0, false },
   { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_B , 0, 1, 0, false },
            { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UB, 0, 0, 1, false },
   { BRW_REGISTER_TYPE_B , BRW_REGISTER_TYPE_B , 0, 0, 1, false },
   { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_B , 0, 0, 1, false },
            { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UW, 0, 0, 0, false },
   { BRW_REGISTER_TYPE_B , BRW_REGISTER_TYPE_W , 0, 0, 0, false },
   { BRW_REGISTER_TYPE_UB, BRW_REGISTER_TYPE_UD, 0, 0, 0, false },
               for (unsigned i = 0; i < ARRAY_SIZE(move); i++) {
      brw_MOV(p, retype(g0, move[i].dst_type), retype(g0, move[i].src_type));
   brw_inst_set_src0_negate(&devinfo, last_inst, move[i].neg);
   brw_inst_set_src0_abs(&devinfo, last_inst, move[i].abs);
                                 brw_SEL(p, retype(g0, BRW_REGISTER_TYPE_UB),
                                          brw_SEL(p, retype(g0, BRW_REGISTER_TYPE_B),
                           }
      TEST_P(validation_test, byte_destination_relaxed_alignment)
   {
      brw_SEL(p, retype(g0, BRW_REGISTER_TYPE_B),
               brw_inst_set_pred_control(&devinfo, last_inst, BRW_PREDICATE_NORMAL);
                              brw_SEL(p, retype(g0, BRW_REGISTER_TYPE_B),
               brw_inst_set_pred_control(&devinfo, last_inst, BRW_PREDICATE_NORMAL);
   brw_inst_set_dst_hstride(&devinfo, last_inst, BRW_HORIZONTAL_STRIDE_2);
            if (devinfo.verx10 >= 45) {
         } else {
            }
      TEST_P(validation_test, byte_64bit_conversion)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src_type;
   unsigned dst_stride;
         #define INST(dst_type, src_type, dst_stride, expected_result)             \
         {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src_type,                                    \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                              \
               INST(B,   Q, 1, false),
   INST(B,  UQ, 1, false),
   INST(B,  DF, 1, false),
   INST(UB,  Q, 1, false),
   INST(UB, UQ, 1, false),
            INST(B,   Q, 2, false),
   INST(B,  UQ, 2, false),
   INST(B , DF, 2, false),
   INST(UB,  Q, 2, false),
   INST(UB, UQ, 2, false),
            INST(B,   Q, 4, false),
   INST(B,  UQ, 4, false),
   INST(B,  DF, 4, false),
   INST(UB,  Q, 4, false),
   INST(UB, UQ, 4, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
                  if (!devinfo.has_64bit_int &&
      (inst[i].src_type == BRW_REGISTER_TYPE_Q ||
               brw_MOV(p, retype(g0, inst[i].dst_type), retype(g0, inst[i].src_type));
   brw_inst_set_dst_hstride(&devinfo, last_inst, inst[i].dst_stride);
                  }
      TEST_P(validation_test, half_float_conversion)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src_type;
   unsigned dst_stride;
   unsigned dst_subnr;
   bool expected_result_bdw;
   bool expected_result_chv_gfx9;
         #define INST(dst_type, src_type, dst_stride, dst_subnr,                     \
               expected_result_bdw, expected_result_chv_gfx9,                 \
   {                                                                     \
      BRW_REGISTER_TYPE_##dst_type,                                      \
   BRW_REGISTER_TYPE_##src_type,                                      \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                \
   dst_subnr,                                                         \
   expected_result_bdw,                                               \
   expected_result_chv_gfx9,                                          \
               /* MOV to half-float destination */
   INST(HF,  B, 1, 0, false, false, false), /* 0 */
   INST(HF,  W, 1, 0, false, false, false),
   INST(HF, HF, 1, 0, true,  true,  true),
   INST(HF, HF, 1, 2, true,  true,  false),
   INST(HF,  D, 1, 0, false, false, false),
   INST(HF,  F, 1, 0, false, true,  false),
   INST(HF,  Q, 1, 0, false, false, false),
   INST(HF,  B, 2, 0, true,  true,  false),
   INST(HF,  B, 2, 2, false, false, false),
   INST(HF,  W, 2, 0, true,  true,  false),
   INST(HF,  W, 2, 2, false, false, false), /* 10 */
   INST(HF, HF, 2, 0, true,  true,  false),
   INST(HF, HF, 2, 2, true,  true,  false),
   INST(HF,  D, 2, 0, true,  true,  true),
   INST(HF,  D, 2, 2, false, false, false),
   INST(HF,  F, 2, 0, true,  true,  true),
   INST(HF,  F, 2, 2, false, true,  false),
   INST(HF,  Q, 2, 0, false, false, false),
   INST(HF, DF, 2, 0, false, false, false),
   INST(HF,  B, 4, 0, false, false, false),
   INST(HF,  W, 4, 0, false, false, false), /* 20 */
   INST(HF, HF, 4, 0, true,  true,  false),
   INST(HF, HF, 4, 2, true,  true,  false),
   INST(HF,  D, 4, 0, false, false, false),
   INST(HF,  F, 4, 0, false, false, false),
   INST(HF,  Q, 4, 0, false, false, false),
            /* MOV from half-float source */
   INST( B, HF, 1, 0, false, false, false),
   INST( W, HF, 1, 0, false, false, false),
   INST( D, HF, 1, 0, true,  true,  true),
   INST( D, HF, 1, 4, true,  true,  true),  /* 30 */
   INST( F, HF, 1, 0, true,  true,  false),
   INST( F, HF, 1, 4, true,  true,  false),
   INST( Q, HF, 1, 0, false, false, false),
   INST(DF, HF, 1, 0, false, false, false),
   INST( B, HF, 2, 0, false, false, false),
   INST( W, HF, 2, 0, true,  true,  true),
   INST( W, HF, 2, 2, false, false, false),
   INST( D, HF, 2, 0, false, false, false),
   INST( F, HF, 2, 0, true,  true,  false),
   INST( B, HF, 4, 0, true,  true,  true),  /* 40 */
   INST( B, HF, 4, 1, false, false, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_DF ||
               if (!devinfo.has_64bit_int &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_Q ||
   inst[i].dst_type == BRW_REGISTER_TYPE_UQ ||
   inst[i].src_type == BRW_REGISTER_TYPE_Q ||
                                 brw_inst_set_dst_hstride(&devinfo, last_inst, inst[i].dst_stride);
            if (inst[i].src_type == BRW_REGISTER_TYPE_B) {
      brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_2);
      } else {
      brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
               if (devinfo.verx10 >= 125) {
      EXPECT_EQ(inst[i].expected_result_gfx125, validate(p)) <<
      } else if (devinfo.platform == INTEL_PLATFORM_CHV || devinfo.ver >= 9) {
      EXPECT_EQ(inst[i].expected_result_chv_gfx9, validate(p)) <<
      } else {
      EXPECT_EQ(inst[i].expected_result_bdw, validate(p)) <<
                     }
      TEST_P(validation_test, mixed_float_source_indirect_addressing)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   bool dst_indirect;
   bool src0_indirect;
   bool expected_result;
         #define INST(dst_type, src0_type, src1_type,                              \
               dst_stride, dst_indirect, src0_indirect, expected_result,    \
   {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                              \
   dst_indirect,                                                    \
   src0_indirect,                                                   \
   expected_result,                                                 \
               /* Source and dest are mixed float: indirect src addressing not allowed */
   INST(HF,  F,  F, 2, false, false, true,  true),
   INST(HF,  F,  F, 2, true,  false, true,  true),
   INST(HF,  F,  F, 2, false, true,  false, false),
   INST(HF,  F,  F, 2, true,  true,  false, false),
   INST( F, HF,  F, 1, false, false, true,  false),
   INST( F, HF,  F, 1, true,  false, true,  false),
   INST( F, HF,  F, 1, false, true,  false, false),
            INST(HF, HF,  F, 2, false, false, true,  false),
   INST(HF, HF,  F, 2, true,  false, true,  false),
   INST(HF, HF,  F, 2, false, true,  false, false),
   INST(HF, HF,  F, 2, true,  true,  false, false),
   INST( F,  F, HF, 1, false, false, true,  false),
   INST( F,  F, HF, 1, true,  false, true,  false),
   INST( F,  F, HF, 1, false, true,  false, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                  brw_inst_set_dst_address_mode(&devinfo, last_inst, inst[i].dst_indirect);
   brw_inst_set_dst_hstride(&devinfo, last_inst, inst[i].dst_stride);
            if (devinfo.verx10 >= 125) {
         } else {
                        }
      TEST_P(validation_test, mixed_float_align1_simd16)
   {
      static const struct {
      unsigned exec_size;
   enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   bool expected_result;
         #define INST(exec_size, dst_type, src0_type, src1_type,                   \
               {                                                                   \
      BRW_EXECUTE_##exec_size,                                         \
   BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                              \
   expected_result,                                                 \
               /* No SIMD16 in mixed mode when destination is packed f16 */
   INST( 8, HF,  F, HF, 2, true,  false),
   INST(16, HF, HF,  F, 2, true,  false),
   INST(16, HF, HF,  F, 1, false, false),
            /* No SIMD16 in mixed mode when destination is f32 */
   INST( 8,  F, HF,  F, 1, true,  false),
   INST( 8,  F,  F, HF, 1, true,  false),
   INST(16,  F, HF,  F, 1, false, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                                    if (devinfo.verx10 >= 125) {
         } else {
                        }
      TEST_P(validation_test, mixed_float_align1_packed_fp16_dst_acc_read_offset_0)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   bool read_acc;
   unsigned subnr;
   bool expected_result_bdw;
   bool expected_result_chv_skl;
         #define INST(dst_type, src0_type, src1_type, dst_stride, read_acc, subnr,   \
               expected_result_bdw, expected_result_chv_skl,                  \
   {                                                                     \
      BRW_REGISTER_TYPE_##dst_type,                                      \
   BRW_REGISTER_TYPE_##src0_type,                                     \
   BRW_REGISTER_TYPE_##src1_type,                                     \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                \
   read_acc,                                                          \
   subnr,                                                             \
   expected_result_bdw,                                               \
   expected_result_chv_skl,                                           \
               /* Destination is not packed */
   INST(HF, HF,  F, 2, true,  0, true, true, false),
   INST(HF, HF,  F, 2, true,  2, true, true, false),
   INST(HF, HF,  F, 2, true,  4, true, true, false),
   INST(HF, HF,  F, 2, true,  8, true, true, false),
            /* Destination is packed, we don't read acc */
   INST(HF, HF,  F, 1, false,  0, false, true, false),
   INST(HF, HF,  F, 1, false,  2, false, true, false),
   INST(HF, HF,  F, 1, false,  4, false, true, false),
   INST(HF, HF,  F, 1, false,  8, false, true, false),
            /* Destination is packed, we read acc */
   INST(HF, HF,  F, 1, true,  0, false, false, false),
   INST(HF, HF,  F, 1, true,  2, false, false, false),
   INST(HF, HF,  F, 1, true,  4, false, false, false),
   INST(HF, HF,  F, 1, true,  8, false, false, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                                    if (devinfo.verx10 >= 125)
         else if (devinfo.platform == INTEL_PLATFORM_CHV || devinfo.ver >= 9)
         else
                  }
      TEST_P(validation_test, mixed_float_fp16_dest_with_acc)
   {
      static const struct {
      unsigned exec_size;
   unsigned opcode;
   enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   bool read_acc;
   bool expected_result_bdw;
   bool expected_result_chv_skl;
         #define INST(exec_size, opcode, dst_type, src0_type, src1_type,           \
               dst_stride, read_acc,expected_result_bdw,                    \
   {                                                                   \
      BRW_EXECUTE_##exec_size,                                         \
   BRW_OPCODE_##opcode,                                             \
   BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                              \
   read_acc,                                                        \
   expected_result_bdw,                                             \
   expected_result_chv_skl,                                         \
               /* Packed fp16 dest with implicit acc needs hstride=2 */
   INST(8, MAC, HF, HF,  F, 1, false, false, false, false),
   INST(8, MAC, HF, HF,  F, 2, false, true,  true,  false),
   INST(8, MAC, HF,  F, HF, 1, false, false, false, false),
            /* Packed fp16 dest with explicit acc needs hstride=2 */
   INST(8, ADD, HF, HF,  F, 1, true,  false, false, false),
   INST(8, ADD, HF, HF,  F, 2, true,  true,  true,  false),
   INST(8, ADD, HF,  F, HF, 1, true,  false, false, false),
            /* If destination is not fp16, restriction doesn't apply */
   INST(8, MAC,  F, HF,  F, 1, false, true, true, false),
            /* If there is no implicit/explicit acc, restriction doesn't apply */
   INST(8, ADD, HF, HF,  F, 1, false, false, true, false),
   INST(8, ADD, HF, HF,  F, 2, false, true,  true, false),
   INST(8, ADD, HF,  F, HF, 1, false, false, true, false),
   INST(8, ADD, HF,  F, HF, 2, false, true,  true, false),
   INST(8, ADD,  F, HF,  F, 1, false, true,  true, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (inst[i].opcode == BRW_OPCODE_MAC) {
      brw_MAC(p, retype(g0, inst[i].dst_type),
            } else {
      assert(inst[i].opcode == BRW_OPCODE_ADD);
   brw_ADD(p, retype(g0, inst[i].dst_type),
                                       if (devinfo.verx10 >= 125)
         else if (devinfo.platform == INTEL_PLATFORM_CHV || devinfo.ver >= 9)
         else
                  }
      TEST_P(validation_test, mixed_float_align1_math_strided_fp16_inputs)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   unsigned src0_stride;
   unsigned src1_stride;
   bool expected_result;
         #define INST(dst_type, src0_type, src1_type,                              \
               dst_stride, src0_stride, src1_stride, expected_result,       \
   {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                              \
   BRW_HORIZONTAL_STRIDE_##src0_stride,                             \
   BRW_HORIZONTAL_STRIDE_##src1_stride,                             \
   expected_result,                                                 \
               INST(HF, HF,  F, 2, 2, 1, true,  false),
   INST(HF,  F, HF, 2, 1, 2, true,  false),
   INST(HF,  F, HF, 1, 1, 2, true,  false),
   INST(HF,  F, HF, 2, 1, 1, false, false),
   INST(HF, HF,  F, 2, 1, 1, false, false),
   INST(HF, HF,  F, 1, 1, 1, false, false),
   INST(HF, HF,  F, 2, 1, 1, false, false),
   INST( F, HF,  F, 1, 1, 1, false, false),
   INST( F,  F, HF, 1, 1, 2, true,  false),
   INST( F, HF, HF, 1, 2, 1, false, false),
      #undef INST
               /* No half-float math in gfx8 */
   if (devinfo.ver < 9)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      gfx6_math(p, retype(g0, inst[i].dst_type),
                                 brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
            brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
            if (devinfo.verx10 >= 125)
         else
                  }
      TEST_P(validation_test, mixed_float_align1_packed_fp16_dst)
   {
      static const struct {
      unsigned exec_size;
   enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned dst_stride;
   unsigned dst_subnr;
   bool expected_result_bdw;
   bool expected_result_chv_skl;
         #define INST(exec_size, dst_type, src0_type, src1_type, dst_stride, dst_subnr, \
               expected_result_bdw, expected_result_chv_skl,                     \
   {                                                                        \
      BRW_EXECUTE_##exec_size,                                              \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   BRW_REGISTER_TYPE_##src0_type,                                        \
   BRW_REGISTER_TYPE_##src1_type,                                        \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                   \
   dst_subnr,                                                            \
   expected_result_bdw,                                                  \
   expected_result_chv_skl,                                              \
               /* SIMD8 packed fp16 dst won't cross oword boundaries if region is
   * oword-aligned
   */
   INST( 8, HF, HF,  F, 1,  0, false, true,  false),
   INST( 8, HF, HF,  F, 1,  2, false, false, false),
   INST( 8, HF, HF,  F, 1,  4, false, false, false),
   INST( 8, HF, HF,  F, 1,  8, false, false, false),
            /* SIMD16 packed fp16 always crosses oword boundaries */
   INST(16, HF, HF,  F, 1,  0, false, false, false),
   INST(16, HF, HF,  F, 1,  2, false, false, false),
   INST(16, HF, HF,  F, 1,  4, false, false, false),
   INST(16, HF, HF,  F, 1,  8, false, false, false),
            /* If destination is not packed (or not fp16) we can cross oword
   * boundaries
   */
   INST( 8, HF, HF,  F, 2,  0, true, true, false),
      #undef INST
               if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                  brw_inst_set_dst_hstride(&devinfo, last_inst, inst[i].dst_stride);
            brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src0_width(&devinfo, last_inst, BRW_WIDTH_4);
            brw_inst_set_src1_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
   brw_inst_set_src1_width(&devinfo, last_inst, BRW_WIDTH_4);
                     if (devinfo.verx10 >= 125)
         else if (devinfo.platform == INTEL_PLATFORM_CHV || devinfo.ver >= 9)
         else
                  }
      TEST_P(validation_test, mixed_float_align16_packed_data)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned src0_vstride;
   unsigned src1_vstride;
         #define INST(dst_type, src0_type, src1_type,                              \
               {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_VERTICAL_STRIDE_##src0_vstride,                              \
   BRW_VERTICAL_STRIDE_##src1_vstride,                              \
               /* We only test with F destination because there is a restriction
   * by which F->HF conversions need to be DWord aligned but Align16 also
   * requires that destination horizontal stride is 1.
   */
   INST(F,  F, HF, 4, 4, true),
   INST(F,  F, HF, 2, 4, false),
   INST(F,  F, HF, 4, 2, false),
   INST(F,  F, HF, 0, 4, false),
   INST(F,  F, HF, 4, 0, false),
   INST(F, HF,  F, 4, 4, true),
   INST(F, HF,  F, 4, 2, false),
   INST(F, HF,  F, 2, 4, false),
   INST(F, HF,  F, 0, 4, false),
      #undef INST
               if (devinfo.ver < 8 || devinfo.ver >= 11)
                     for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                  brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src0_vstride);
                           }
      TEST_P(validation_test, mixed_float_align16_no_simd16)
   {
      static const struct {
      unsigned exec_size;
   enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
         #define INST(exec_size, dst_type, src0_type, src1_type, expected_result)  \
         {                                                                   \
      BRW_EXECUTE_##exec_size,                                         \
   BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
               /* We only test with F destination because there is a restriction
   * by which F->HF conversions need to be DWord aligned but Align16 also
   * requires that destination horizontal stride is 1.
   */
   INST( 8,  F,  F, HF, true),
   INST( 8,  F, HF,  F, true),
   INST( 8,  F,  F, HF, true),
   INST(16,  F,  F, HF, false),
   INST(16,  F, HF,  F, false),
      #undef INST
               if (devinfo.ver < 8 || devinfo.ver >= 11)
                     for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                           brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
                           }
      TEST_P(validation_test, mixed_float_align16_no_acc_read)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   bool read_acc;
         #define INST(dst_type, src0_type, src1_type, read_acc, expected_result)   \
         {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   read_acc,                                                        \
               /* We only test with F destination because there is a restriction
   * by which F->HF conversions need to be DWord aligned but Align16 also
   * requires that destination horizontal stride is 1.
   */
   INST( F,  F, HF, false, true),
   INST( F,  F, HF, true,  false),
   INST( F, HF,  F, false, true),
      #undef INST
               if (devinfo.ver < 8 || devinfo.ver >= 11)
                     for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD(p, retype(g0, inst[i].dst_type),
                  brw_inst_set_src0_vstride(&devinfo, last_inst, BRW_VERTICAL_STRIDE_4);
                           }
      TEST_P(validation_test, mixed_float_align16_math_packed_format)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   unsigned src0_vstride;
   unsigned src1_vstride;
         #define INST(dst_type, src0_type, src1_type,                              \
               {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_VERTICAL_STRIDE_##src0_vstride,                              \
   BRW_VERTICAL_STRIDE_##src1_vstride,                              \
               /* We only test with F destination because there is a restriction
   * by which F->HF conversions need to be DWord aligned but Align16 also
   * requires that destination horizontal stride is 1.
   */
   INST( F, HF,  F, 4, 0, false),
   INST( F, HF, HF, 4, 4, true),
   INST( F,  F, HF, 4, 0, false),
   INST( F,  F, HF, 2, 4, false),
   INST( F,  F, HF, 4, 2, false),
      #undef INST
               /* Align16 Math for mixed float mode is not supported in gfx8 */
   if (devinfo.ver < 9 || devinfo.ver >= 11)
                     for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      gfx6_math(p, retype(g0, inst[i].dst_type),
                        brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src0_vstride);
                           }
      TEST_P(validation_test, vector_immediate_destination_alignment)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src_type;
   unsigned subnr;
   unsigned exec_size;
      } move[] = {
      { BRW_REGISTER_TYPE_F, BRW_REGISTER_TYPE_VF,  0, BRW_EXECUTE_4, true  },
   { BRW_REGISTER_TYPE_F, BRW_REGISTER_TYPE_VF, 16, BRW_EXECUTE_4, true  },
            { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_V,   0, BRW_EXECUTE_8, true  },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_V,  16, BRW_EXECUTE_8, true  },
            { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_UV,  0, BRW_EXECUTE_8, true  },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_UV, 16, BRW_EXECUTE_8, true  },
               for (unsigned i = 0; i < ARRAY_SIZE(move); i++) {
      /* UV type is Gfx6+ */
   if (devinfo.ver < 6 &&
                  brw_MOV(p, retype(g0, move[i].dst_type), retype(zero, move[i].src_type));
   brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, move[i].subnr);
                           }
      TEST_P(validation_test, vector_immediate_destination_stride)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src_type;
   unsigned stride;
      } move[] = {
      { BRW_REGISTER_TYPE_F, BRW_REGISTER_TYPE_VF, BRW_HORIZONTAL_STRIDE_1, true  },
   { BRW_REGISTER_TYPE_F, BRW_REGISTER_TYPE_VF, BRW_HORIZONTAL_STRIDE_2, false },
   { BRW_REGISTER_TYPE_D, BRW_REGISTER_TYPE_VF, BRW_HORIZONTAL_STRIDE_1, true  },
   { BRW_REGISTER_TYPE_D, BRW_REGISTER_TYPE_VF, BRW_HORIZONTAL_STRIDE_2, false },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_VF, BRW_HORIZONTAL_STRIDE_2, true  },
            { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_V,  BRW_HORIZONTAL_STRIDE_1, true  },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_V,  BRW_HORIZONTAL_STRIDE_2, false },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_V,  BRW_HORIZONTAL_STRIDE_4, false },
            { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_UV, BRW_HORIZONTAL_STRIDE_1, true  },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_UV, BRW_HORIZONTAL_STRIDE_2, false },
   { BRW_REGISTER_TYPE_W, BRW_REGISTER_TYPE_UV, BRW_HORIZONTAL_STRIDE_4, false },
               for (unsigned i = 0; i < ARRAY_SIZE(move); i++) {
      /* UV type is Gfx6+ */
   if (devinfo.ver < 6 &&
                  brw_MOV(p, retype(g0, move[i].dst_type), retype(zero, move[i].src_type));
                           }
      TEST_P(validation_test, qword_low_power_align1_regioning_restrictions)
   {
      static const struct {
      enum opcode opcode;
            enum brw_reg_type dst_type;
   unsigned dst_subreg;
            enum brw_reg_type src_type;
   unsigned src_subreg;
   unsigned src_vstride;
   unsigned src_width;
                  #define INST(opcode, exec_size, dst_type, dst_subreg, dst_stride, src_type,    \
               {                                                                        \
      BRW_OPCODE_##opcode,                                                  \
   BRW_EXECUTE_##exec_size,                                              \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   dst_subreg,                                                           \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                   \
   BRW_REGISTER_TYPE_##src_type,                                         \
   src_subreg,                                                           \
   BRW_VERTICAL_STRIDE_##src_vstride,                                    \
   BRW_WIDTH_##src_width,                                                \
   BRW_HORIZONTAL_STRIDE_##src_hstride,                                  \
               /* Some instruction that violate no restrictions, as a control */
   INST(MOV, 4, DF, 0, 1, DF, 0, 4, 4, 1, true ),
   INST(MOV, 4, Q,  0, 1, Q,  0, 4, 4, 1, true ),
            INST(MOV, 4, DF, 0, 1, F,  0, 8, 4, 2, true ),
   INST(MOV, 4, Q,  0, 1, D,  0, 8, 4, 2, true ),
            INST(MOV, 4, F,  0, 2, DF, 0, 4, 4, 1, true ),
   INST(MOV, 4, D,  0, 2, Q,  0, 4, 4, 1, true ),
            INST(MUL, 8, D,  0, 2, D,  0, 8, 4, 2, true ),
            /* Something with subreg nrs */
   INST(MOV, 2, DF, 8, 1, DF, 8, 2, 2, 1, true ),
   INST(MOV, 2, Q,  8, 1, Q,  8, 2, 2, 1, true ),
            INST(MUL, 2, D,  4, 2, D,  4, 4, 2, 2, true ),
            /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, regioning in Align1 must follow these rules:
   *
   *    1. Source and Destination horizontal stride must be aligned to the
   *       same qword.
   */
   INST(MOV, 4, DF, 0, 2, DF, 0, 4, 4, 1, false),
   INST(MOV, 4, Q,  0, 2, Q,  0, 4, 4, 1, false),
            INST(MOV, 4, DF, 0, 2, F,  0, 8, 4, 2, false),
   INST(MOV, 4, Q,  0, 2, D,  0, 8, 4, 2, false),
            INST(MOV, 4, DF, 0, 2, F,  0, 4, 4, 1, false),
   INST(MOV, 4, Q,  0, 2, D,  0, 4, 4, 1, false),
            INST(MUL, 4, D,  0, 2, D,  0, 4, 4, 1, false),
            INST(MUL, 4, D,  0, 1, D,  0, 8, 4, 2, false),
            /*    2. Regioning must ensure Src.Vstride = Src.Width * Src.Hstride. */
   INST(MOV, 4, DF, 0, 1, DF, 0, 0, 2, 1, false),
   INST(MOV, 4, Q,  0, 1, Q,  0, 0, 2, 1, false),
            INST(MOV, 4, DF, 0, 1, F,  0, 0, 2, 2, false),
   INST(MOV, 4, Q,  0, 1, D,  0, 0, 2, 2, false),
            INST(MOV, 8, F,  0, 2, DF, 0, 0, 2, 1, false),
   INST(MOV, 8, D,  0, 2, Q,  0, 0, 2, 1, false),
            INST(MUL, 8, D,  0, 2, D,  0, 0, 4, 2, false),
            INST(MUL, 8, D,  0, 2, D,  0, 0, 4, 2, false),
            /*    3. Source and Destination offset must be the same, except the case
   *       of scalar source.
   */
   INST(MOV, 2, DF, 8, 1, DF, 0, 2, 2, 1, false),
   INST(MOV, 2, Q,  8, 1, Q,  0, 2, 2, 1, false),
            INST(MOV, 2, DF, 0, 1, DF, 8, 2, 2, 1, false),
   INST(MOV, 2, Q,  0, 1, Q,  8, 2, 2, 1, false),
            INST(MUL, 4, D,  4, 2, D,  0, 4, 2, 2, false),
            INST(MUL, 4, D,  0, 2, D,  4, 4, 2, 2, false),
            INST(MOV, 2, DF, 8, 1, DF, 0, 0, 1, 0, true ),
   INST(MOV, 2, Q,  8, 1, Q,  0, 0, 1, 0, true ),
            INST(MOV, 2, DF, 8, 1, F,  4, 0, 1, 0, true ),
   INST(MOV, 2, Q,  8, 1, D,  4, 0, 1, 0, true ),
            INST(MUL, 4, D,  4, 1, D,  0, 0, 1, 0, true ),
            INST(MUL, 4, D,  0, 1, D,  4, 0, 1, 0, true ),
      #undef INST
               /* These restrictions only apply to Gfx8+ */
   if (devinfo.ver < 8)
            /* NoDDChk/NoDDClr does not exist on Gfx12+ */
   if (devinfo.ver >= 12)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_DF ||
               if (!devinfo.has_64bit_int &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_Q ||
   inst[i].dst_type == BRW_REGISTER_TYPE_UQ ||
   inst[i].src_type == BRW_REGISTER_TYPE_Q ||
               if (inst[i].opcode == BRW_OPCODE_MOV) {
      brw_MOV(p, retype(g0, inst[i].dst_type),
      } else {
      assert(inst[i].opcode == BRW_OPCODE_MUL);
   brw_MUL(p, retype(g0, inst[i].dst_type),
            }
            brw_inst_set_dst_da1_subreg_nr(&devinfo, last_inst, inst[i].dst_subreg);
                     brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src_vstride);
   brw_inst_set_src0_width(&devinfo, last_inst, inst[i].src_width);
            if (devinfo.platform == INTEL_PLATFORM_CHV ||
      intel_device_info_is_9lp(&devinfo)) {
      } else {
                        }
      TEST_P(validation_test, qword_low_power_no_indirect_addressing)
   {
      static const struct {
      enum opcode opcode;
            enum brw_reg_type dst_type;
   bool dst_is_indirect;
            enum brw_reg_type src_type;
   bool src_is_indirect;
   unsigned src_vstride;
   unsigned src_width;
                  #define INST(opcode, exec_size, dst_type, dst_is_indirect, dst_stride,         \
               src_type, src_is_indirect, src_vstride, src_width, src_hstride,   \
   {                                                                        \
      BRW_OPCODE_##opcode,                                                  \
   BRW_EXECUTE_##exec_size,                                              \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   dst_is_indirect,                                                      \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                   \
   BRW_REGISTER_TYPE_##src_type,                                         \
   src_is_indirect,                                                      \
   BRW_VERTICAL_STRIDE_##src_vstride,                                    \
   BRW_WIDTH_##src_width,                                                \
   BRW_HORIZONTAL_STRIDE_##src_hstride,                                  \
               /* Some instruction that violate no restrictions, as a control */
   INST(MOV, 4, DF, 0, 1, DF, 0, 4, 4, 1, true ),
   INST(MOV, 4, Q,  0, 1, Q,  0, 4, 4, 1, true ),
            INST(MUL, 8, D,  0, 2, D,  0, 8, 4, 2, true ),
            INST(MOV, 4, F,  1, 1, F,  0, 4, 4, 1, true ),
   INST(MOV, 4, F,  0, 1, F,  1, 4, 4, 1, true ),
            /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, indirect addressing must not be used.
   */
   INST(MOV, 4, DF, 1, 1, DF, 0, 4, 4, 1, false),
   INST(MOV, 4, Q,  1, 1, Q,  0, 4, 4, 1, false),
            INST(MOV, 4, DF, 0, 1, DF, 1, 4, 4, 1, false),
   INST(MOV, 4, Q,  0, 1, Q,  1, 4, 4, 1, false),
            INST(MOV, 4, DF, 1, 1, F,  0, 8, 4, 2, false),
   INST(MOV, 4, Q,  1, 1, D,  0, 8, 4, 2, false),
            INST(MOV, 4, DF, 0, 1, F,  1, 8, 4, 2, false),
   INST(MOV, 4, Q,  0, 1, D,  1, 8, 4, 2, false),
            INST(MOV, 4, F,  1, 2, DF, 0, 4, 4, 1, false),
   INST(MOV, 4, D,  1, 2, Q,  0, 4, 4, 1, false),
            INST(MOV, 4, F,  0, 2, DF, 1, 4, 4, 1, false),
   INST(MOV, 4, D,  0, 2, Q,  1, 4, 4, 1, false),
            INST(MUL, 8, D,  1, 2, D,  0, 8, 4, 2, false),
            INST(MUL, 8, D,  0, 2, D,  1, 8, 4, 2, false),
      #undef INST
               /* These restrictions only apply to Gfx8+ */
   if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_DF ||
               if (!devinfo.has_64bit_int &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_Q ||
   inst[i].dst_type == BRW_REGISTER_TYPE_UQ ||
   inst[i].src_type == BRW_REGISTER_TYPE_Q ||
               if (inst[i].opcode == BRW_OPCODE_MOV) {
      brw_MOV(p, retype(g0, inst[i].dst_type),
      } else {
      assert(inst[i].opcode == BRW_OPCODE_MUL);
   brw_MUL(p, retype(g0, inst[i].dst_type),
            }
            brw_inst_set_dst_address_mode(&devinfo, last_inst, inst[i].dst_is_indirect);
                     brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src_vstride);
   brw_inst_set_src0_width(&devinfo, last_inst, inst[i].src_width);
            if (devinfo.platform == INTEL_PLATFORM_CHV ||
      intel_device_info_is_9lp(&devinfo)) {
      } else {
                        }
      TEST_P(validation_test, qword_low_power_no_64bit_arf)
   {
      static const struct {
      enum opcode opcode;
            struct brw_reg dst;
   enum brw_reg_type dst_type;
            struct brw_reg src;
   enum brw_reg_type src_type;
   unsigned src_vstride;
   unsigned src_width;
            bool acc_wr;
         #define INST(opcode, exec_size, dst, dst_type, dst_stride,                     \
               src, src_type, src_vstride, src_width, src_hstride,               \
   {                                                                        \
      BRW_OPCODE_##opcode,                                                  \
   BRW_EXECUTE_##exec_size,                                              \
   dst,                                                                  \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                   \
   src,                                                                  \
   BRW_REGISTER_TYPE_##src_type,                                         \
   BRW_VERTICAL_STRIDE_##src_vstride,                                    \
   BRW_WIDTH_##src_width,                                                \
   BRW_HORIZONTAL_STRIDE_##src_hstride,                                  \
   acc_wr,                                                               \
               /* Some instruction that violate no restrictions, as a control */
   INST(MOV, 4, g0,   DF, 1, g0,   F,  4, 2, 2, 0, true ),
            INST(MOV, 4, g0,   Q,  1, g0,   D,  4, 2, 2, 0, true ),
            INST(MOV, 4, g0,   UQ, 1, g0,   UD, 4, 2, 2, 0, true ),
            INST(MOV, 4, null, F,  1, g0,   F,  4, 4, 1, 0, true ),
   INST(MOV, 4, acc0, F,  1, g0,   F,  4, 4, 1, 0, true ),
            INST(MOV, 4, null, D,  1, g0,   D,  4, 4, 1, 0, true ),
   INST(MOV, 4, acc0, D,  1, g0,   D,  4, 4, 1, 0, true ),
            INST(MOV, 4, null, UD, 1, g0,   UD, 4, 4, 1, 0, true ),
   INST(MOV, 4, acc0, UD, 1, g0,   UD, 4, 4, 1, 0, true ),
            INST(MUL, 4, g0,   D,  2, g0,   D,  4, 2, 2, 0, true ),
            /* The PRMs say that for CHV, BXT:
   *
   *    ARF registers must never be used with 64b datatype or when
   *    operation is integer DWord multiply.
   */
   INST(MOV, 4, acc0, DF, 1, g0,   F,  4, 2, 2, 0, false),
            INST(MOV, 4, acc0, Q,  1, g0,   D,  4, 2, 2, 0, false),
            INST(MOV, 4, acc0, UQ, 1, g0,   UD, 4, 2, 2, 0, false),
            INST(MOV, 4, acc0, F,  2, g0,   DF, 4, 4, 1, 0, false),
            INST(MOV, 4, acc0, D,  2, g0,   Q,  4, 4, 1, 0, false),
            INST(MOV, 4, acc0, UD, 2, g0,   UQ, 4, 4, 1, 0, false),
            INST(MUL, 4, acc0, D,  2, g0,   D,  4, 2, 2, 0, false),
   INST(MUL, 4, acc0, UD, 2, g0,   UD, 4, 2, 2, 0, false),
            /* We assume that the restriction does not apply to the null register */
   INST(MOV, 4, null, DF, 1, g0,   F,  4, 2, 2, 0, true ),
   INST(MOV, 4, null, Q,  1, g0,   D,  4, 2, 2, 0, true ),
            /* Check implicit accumulator write control */
   INST(MOV, 4, null, DF, 1, g0,   F,  4, 2, 2, 1, false),
      #undef INST
               /* These restrictions only apply to Gfx8+ */
   if (devinfo.ver < 8)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_DF ||
               if (!devinfo.has_64bit_int &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_Q ||
   inst[i].dst_type == BRW_REGISTER_TYPE_UQ ||
   inst[i].src_type == BRW_REGISTER_TYPE_Q ||
               if (inst[i].opcode == BRW_OPCODE_MOV) {
      brw_MOV(p, retype(inst[i].dst, inst[i].dst_type),
      } else {
      assert(inst[i].opcode == BRW_OPCODE_MUL);
   brw_MUL(p, retype(inst[i].dst, inst[i].dst_type),
                  }
   brw_inst_set_exec_size(&devinfo, last_inst, inst[i].exec_size);
                     brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src_vstride);
   brw_inst_set_src0_width(&devinfo, last_inst, inst[i].src_width);
            /* Note: The Broadwell PRM also lists the restriction that destination
   * of DWord multiplication cannot be the accumulator.
   */
   if (devinfo.platform == INTEL_PLATFORM_CHV ||
      intel_device_info_is_9lp(&devinfo) ||
   (devinfo.ver == 8 &&
   inst[i].opcode == BRW_OPCODE_MUL &&
   brw_inst_dst_reg_file(&devinfo, last_inst) == BRW_ARCHITECTURE_REGISTER_FILE &&
   brw_inst_dst_da_reg_nr(&devinfo, last_inst) != BRW_ARF_NULL)) {
      } else {
                              if (!devinfo.has_64bit_float)
            /* MAC implicitly reads the accumulator */
   brw_MAC(p, retype(g0, BRW_REGISTER_TYPE_DF),
               if (devinfo.platform == INTEL_PLATFORM_CHV ||
      intel_device_info_is_9lp(&devinfo)) {
      } else {
            }
      TEST_P(validation_test, align16_64_bit_integer)
   {
      static const struct {
      enum opcode opcode;
            enum brw_reg_type dst_type;
                  #define INST(opcode, exec_size, dst_type, src_type, expected_result)           \
         {                                                                        \
      BRW_OPCODE_##opcode,                                                  \
   BRW_EXECUTE_##exec_size,                                              \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   BRW_REGISTER_TYPE_##src_type,                                         \
               /* Some instruction that violate no restrictions, as a control */
   INST(MOV, 2, Q,  D,  true ),
   INST(MOV, 2, UQ, UD, true ),
            INST(ADD, 2, Q,  D,  true ),
   INST(ADD, 2, UQ, UD, true ),
            /* The PRMs say that for BDW, SKL:
   *
   *    If Align16 is required for an operation with QW destination and non-QW
   *    source datatypes, the execution size cannot exceed 2.
            INST(MOV, 4, Q,  D,  false),
   INST(MOV, 4, UQ, UD, false),
            INST(ADD, 4, Q,  D,  false),
   INST(ADD, 4, UQ, UD, false),
      #undef INST
               /* 64-bit integer types exist on Gfx8+ */
   if (devinfo.ver < 8)
            /* Align16 does not exist on Gfx11+ */
   if (devinfo.ver >= 11)
                     for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (inst[i].opcode == BRW_OPCODE_MOV) {
      brw_MOV(p, retype(g0, inst[i].dst_type),
      } else {
      assert(inst[i].opcode == BRW_OPCODE_ADD);
   brw_ADD(p, retype(g0, inst[i].dst_type),
            }
                           }
      TEST_P(validation_test, qword_low_power_no_depctrl)
   {
      static const struct {
      enum opcode opcode;
            enum brw_reg_type dst_type;
            enum brw_reg_type src_type;
   unsigned src_vstride;
   unsigned src_width;
            bool no_dd_check;
                  #define INST(opcode, exec_size, dst_type, dst_stride,                          \
               src_type, src_vstride, src_width, src_hstride,                    \
   {                                                                        \
      BRW_OPCODE_##opcode,                                                  \
   BRW_EXECUTE_##exec_size,                                              \
   BRW_REGISTER_TYPE_##dst_type,                                         \
   BRW_HORIZONTAL_STRIDE_##dst_stride,                                   \
   BRW_REGISTER_TYPE_##src_type,                                         \
   BRW_VERTICAL_STRIDE_##src_vstride,                                    \
   BRW_WIDTH_##src_width,                                                \
   BRW_HORIZONTAL_STRIDE_##src_hstride,                                  \
   no_dd_check,                                                          \
   no_dd_clear,                                                          \
               /* Some instruction that violate no restrictions, as a control */
   INST(MOV, 4, DF, 1, F,  8, 4, 2, 0, 0, true ),
   INST(MOV, 4, Q,  1, D,  8, 4, 2, 0, 0, true ),
            INST(MOV, 4, F,  2, DF, 4, 4, 1, 0, 0, true ),
   INST(MOV, 4, D,  2, Q,  4, 4, 1, 0, 0, true ),
            INST(MUL, 8, D,  2, D,  8, 4, 2, 0, 0, true ),
                     /* The PRMs say that for CHV, BXT:
   *
   *    When source or destination datatype is 64b or operation is integer
   *    DWord multiply, DepCtrl must not be used.
   */
   INST(MOV, 4, DF, 1, F,  8, 4, 2, 1, 0, false),
   INST(MOV, 4, Q,  1, D,  8, 4, 2, 1, 0, false),
            INST(MOV, 4, F,  2, DF, 4, 4, 1, 1, 0, false),
   INST(MOV, 4, D,  2, Q,  4, 4, 1, 1, 0, false),
            INST(MOV, 4, DF, 1, F,  8, 4, 2, 0, 1, false),
   INST(MOV, 4, Q,  1, D,  8, 4, 2, 0, 1, false),
            INST(MOV, 4, F,  2, DF, 4, 4, 1, 0, 1, false),
   INST(MOV, 4, D,  2, Q,  4, 4, 1, 0, 1, false),
            INST(MUL, 8, D,  2, D,  8, 4, 2, 1, 0, false),
            INST(MUL, 8, D,  2, D,  8, 4, 2, 0, 1, false),
      #undef INST
               /* These restrictions only apply to Gfx8+ */
   if (devinfo.ver < 8)
            /* NoDDChk/NoDDClr does not exist on Gfx12+ */
   if (devinfo.ver >= 12)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      if (!devinfo.has_64bit_float &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_DF ||
               if (!devinfo.has_64bit_int &&
      (inst[i].dst_type == BRW_REGISTER_TYPE_Q ||
   inst[i].dst_type == BRW_REGISTER_TYPE_UQ ||
   inst[i].src_type == BRW_REGISTER_TYPE_Q ||
               if (inst[i].opcode == BRW_OPCODE_MOV) {
      brw_MOV(p, retype(g0, inst[i].dst_type),
      } else {
      assert(inst[i].opcode == BRW_OPCODE_MUL);
   brw_MUL(p, retype(g0, inst[i].dst_type),
            }
                     brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].src_vstride);
   brw_inst_set_src0_width(&devinfo, last_inst, inst[i].src_width);
            brw_inst_set_no_dd_check(&devinfo, last_inst, inst[i].no_dd_check);
            if (devinfo.platform == INTEL_PLATFORM_CHV ||
      intel_device_info_is_9lp(&devinfo)) {
      } else {
                        }
      TEST_P(validation_test, gfx11_no_byte_src_1_2)
   {
      static const struct {
      enum opcode opcode;
            enum brw_reg_type dst_type;
   struct {
      enum brw_reg_type type;
   unsigned vstride;
   unsigned width;
               int  gfx_ver;
         #define INST(opcode, access_mode, dst_type,                             \
               src0_type, src0_vstride, src0_width, src0_hstride,         \
   src1_type, src1_vstride, src1_width, src1_hstride,         \
   src2_type,                                                 \
   {                                                                 \
      BRW_OPCODE_##opcode,                                           \
   BRW_ALIGN_##access_mode,                                       \
   BRW_REGISTER_TYPE_##dst_type,                                  \
   {                                                              \
      {                                                           \
      BRW_REGISTER_TYPE_##src0_type,                           \
   BRW_VERTICAL_STRIDE_##src0_vstride,                      \
   BRW_WIDTH_##src0_width,                                  \
      },                                                          \
   {                                                           \
      BRW_REGISTER_TYPE_##src1_type,                           \
   BRW_VERTICAL_STRIDE_##src1_vstride,                      \
   BRW_WIDTH_##src1_width,                                  \
      },                                                          \
   {                                                           \
            },                                                             \
   gfx_ver,                                                       \
               /* Passes on < 11 */
   INST(MOV, 16,  F, B, 2, 4, 0, UD, 0, 4, 0,  D,  8, true ),
   INST(ADD, 16, UD, F, 0, 4, 0, UB, 0, 1, 0,  D,  7, true ),
            /* Fails on 11+ */
   INST(MAD,  1, UB, W, 1, 1, 0,  D, 0, 4, 0,  B, 11, false ),
   INST(MAD,  1, UB, W, 1, 1, 1, UB, 1, 1, 0,  W, 11, false ),
            /* Passes on 11+ */
   INST(MOV,  1,  W, B, 8, 8, 1,  D, 8, 8, 1,  D, 11, true ),
   INST(ADD,  1, UD, B, 8, 8, 1,  W, 8, 8, 1,  D, 11, true ),
      #undef INST
                  for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      /* Skip instruction not meant for this gfx_ver. */
   if (devinfo.ver != inst[i].gfx_ver)
                     brw_set_default_exec_size(p, BRW_EXECUTE_8);
            switch (inst[i].opcode) {
   case BRW_OPCODE_MOV:
      brw_MOV(p, retype(g0, inst[i].dst_type),
         brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].srcs[0].vstride);
   brw_inst_set_src0_hstride(&devinfo, last_inst, inst[i].srcs[0].hstride);
      case BRW_OPCODE_ADD:
      brw_ADD(p, retype(g0, inst[i].dst_type),
               brw_inst_set_src0_vstride(&devinfo, last_inst, inst[i].srcs[0].vstride);
   brw_inst_set_src0_width(&devinfo, last_inst, inst[i].srcs[0].width);
   brw_inst_set_src0_hstride(&devinfo, last_inst, inst[i].srcs[0].hstride);
   brw_inst_set_src1_vstride(&devinfo, last_inst, inst[i].srcs[1].vstride);
   brw_inst_set_src1_width(&devinfo, last_inst, inst[i].srcs[1].width);
   brw_inst_set_src1_hstride(&devinfo, last_inst, inst[i].srcs[1].hstride);
      case BRW_OPCODE_MAD:
      brw_MAD(p, retype(g0, inst[i].dst_type),
            retype(g0, inst[i].srcs[0].type),
      brw_inst_set_3src_a1_src0_vstride(&devinfo, last_inst, inst[i].srcs[0].vstride);
   brw_inst_set_3src_a1_src0_hstride(&devinfo, last_inst, inst[i].srcs[0].hstride);
   brw_inst_set_3src_a1_src1_vstride(&devinfo, last_inst, inst[i].srcs[0].vstride);
   brw_inst_set_3src_a1_src1_hstride(&devinfo, last_inst, inst[i].srcs[0].hstride);
      default:
                           brw_inst_set_src0_width(&devinfo, last_inst, inst[i].srcs[0].width);
                                    }
      TEST_P(validation_test, add3_source_types)
   {
      static const struct {
      enum brw_reg_type dst_type;
   enum brw_reg_type src0_type;
   enum brw_reg_type src1_type;
   enum brw_reg_type src2_type;
         #define INST(dst_type, src0_type, src1_type, src2_type, expected_result)  \
         {                                                                   \
      BRW_REGISTER_TYPE_##dst_type,                                    \
   BRW_REGISTER_TYPE_##src0_type,                                   \
   BRW_REGISTER_TYPE_##src1_type,                                   \
   BRW_REGISTER_TYPE_##src2_type,                                   \
               INST( F,  F,  F,  F, false),
   INST(HF, HF, HF, HF, false),
   INST( B,  B,  B,  B, false),
            INST( W,  W,  W,  W, true),
   INST(UW, UW, UW, UW, true),
   INST( D,  D,  D,  D, true),
            INST( W,  D,  W,  W, true),
   INST(UW, UW, UD, UW, true),
   INST( D,  D,  W,  D, true),
   #undef INST
                  if (devinfo.verx10 < 125)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD3(p,
            retype(g0, inst[i].dst_type),
                              }
      TEST_P(validation_test, add3_immediate_types)
   {
      static const struct {
      enum brw_reg_type reg_type;
   enum brw_reg_type imm_type;
   unsigned imm_src;
         #define INST(reg_type, imm_type, imm_src, expected_result)                \
         {                                                                   \
      BRW_REGISTER_TYPE_##reg_type,                                    \
   BRW_REGISTER_TYPE_##imm_type,                                    \
   imm_src,                                                         \
               INST( W,  W,  0, true),
   INST( W,  W,  2, true),
   INST(UW, UW,  0, true),
   INST(UW, UW,  2, true),
   INST( D,  W,  0, true),
   INST(UD,  W,  2, true),
   INST( D, UW,  0, true),
            INST( W,  D,  0, false),
   INST( W,  D,  2, false),
   INST(UW, UD,  0, false),
   INST(UW, UD,  2, false),
   INST( D,  D,  0, false),
   INST(UD,  D,  2, false),
   INST( D, UD,  0, false),
   #undef INST
                  if (devinfo.verx10 < 125)
            for (unsigned i = 0; i < ARRAY_SIZE(inst); i++) {
      brw_ADD3(p,
            retype(g0, inst[i].reg_type),
   inst[i].imm_src == 0 ? retype(brw_imm_d(0x1234), inst[i].imm_type)
                                    }
