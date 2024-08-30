   /*
   * Copyright © 2021 Intel Corporation
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
         #include "brw_private.h"
   #include "compiler/shader_info.h"
   #include "intel/dev/intel_debug.h"
   #include "intel/dev/intel_device_info.h"
   #include "util/ralloc.h"
      #include <gtest/gtest.h>
      enum {
      SIMD8  = 0,
   SIMD16 = 1,
      };
      const bool spilled = true;
   const bool not_spilled = false;
      class SIMDSelectionTest : public ::testing::Test {
   protected:
      SIMDSelectionTest()
   : mem_ctx(ralloc_context(NULL))
   , devinfo(rzalloc(mem_ctx, intel_device_info))
   , prog_data(rzalloc(mem_ctx, struct brw_cs_prog_data))
   , simd_state{
      .devinfo = devinfo,
      }
   {
                  ~SIMDSelectionTest() {
                  void *mem_ctx;
   intel_device_info *devinfo;
   struct brw_cs_prog_data *prog_data;
      };
      class SIMDSelectionCS : public SIMDSelectionTest {
   protected:
      SIMDSelectionCS() {
      prog_data->base.stage = MESA_SHADER_COMPUTE;
   prog_data->local_size[0] = 32;
   prog_data->local_size[1] = 1;
                  };
      TEST_F(SIMDSelectionCS, DefaultsToSIMD16)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
               }
      TEST_F(SIMDSelectionCS, TooBigFor16)
   {
      prog_data->local_size[0] = devinfo->max_cs_workgroup_threads;
   prog_data->local_size[1] = 32;
            ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
               }
      TEST_F(SIMDSelectionCS, WorkgroupSize1)
   {
      prog_data->local_size[0] = 1;
   prog_data->local_size[1] = 1;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, WorkgroupSize8)
   {
      prog_data->local_size[0] = 8;
   prog_data->local_size[1] = 1;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, WorkgroupSizeVariable)
   {
      prog_data->local_size[0] = 0;
   prog_data->local_size[1] = 0;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
                     const unsigned wg_8_1_1[] = { 8, 1, 1 };
            const unsigned wg_16_1_1[] = { 16, 1, 1 };
            const unsigned wg_32_1_1[] = { 32, 1, 1 };
      }
      TEST_F(SIMDSelectionCS, WorkgroupSizeVariableSpilled)
   {
      prog_data->local_size[0] = 0;
   prog_data->local_size[1] = 0;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
                     const unsigned wg_8_1_1[] = { 8, 1, 1 };
            const unsigned wg_16_1_1[] = { 16, 1, 1 };
            const unsigned wg_32_1_1[] = { 32, 1, 1 };
      }
      TEST_F(SIMDSelectionCS, WorkgroupSizeVariableNoSIMD8)
   {
      prog_data->local_size[0] = 0;
   prog_data->local_size[1] = 0;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
                     const unsigned wg_8_1_1[] = { 8, 1, 1 };
            const unsigned wg_16_1_1[] = { 16, 1, 1 };
            const unsigned wg_32_1_1[] = { 32, 1, 1 };
      }
      TEST_F(SIMDSelectionCS, WorkgroupSizeVariableNoSIMD16)
   {
      prog_data->local_size[0] = 0;
   prog_data->local_size[1] = 0;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
                     const unsigned wg_8_1_1[] = { 8, 1, 1 };
            const unsigned wg_16_1_1[] = { 16, 1, 1 };
            const unsigned wg_32_1_1[] = { 32, 1, 1 };
      }
      TEST_F(SIMDSelectionCS, WorkgroupSizeVariableNoSIMD8NoSIMD16)
   {
      prog_data->local_size[0] = 0;
   prog_data->local_size[1] = 0;
            ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
                     const unsigned wg_8_1_1[] = { 8, 1, 1 };
            const unsigned wg_16_1_1[] = { 16, 1, 1 };
            const unsigned wg_32_1_1[] = { 32, 1, 1 };
      }
      TEST_F(SIMDSelectionCS, SpillAtSIMD8)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, spilled);
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, SpillAtSIMD16)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, spilled);
               }
      TEST_F(SIMDSelectionCS, EnvironmentVariable32)
   {
               ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
               }
      TEST_F(SIMDSelectionCS, EnvironmentVariable32ButSpills)
   {
               ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
               }
      TEST_F(SIMDSelectionCS, Require8)
   {
               ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   brw_simd_mark_compiled(simd_state, SIMD8, not_spilled);
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, Require8ErrorWhenNotCompile)
   {
               ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, Require16)
   {
               ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   brw_simd_mark_compiled(simd_state, SIMD16, not_spilled);
               }
      TEST_F(SIMDSelectionCS, Require16ErrorWhenNotCompile)
   {
               ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, Require32)
   {
               ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
               }
      TEST_F(SIMDSelectionCS, Require32ErrorWhenNotCompile)
   {
               ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_FALSE(brw_simd_should_compile(simd_state, SIMD16));
               }
      TEST_F(SIMDSelectionCS, FirstCompiledIsSIMD8)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
            ASSERT_TRUE(brw_simd_any_compiled(simd_state));
      }
      TEST_F(SIMDSelectionCS, FirstCompiledIsSIMD16)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
            ASSERT_TRUE(brw_simd_any_compiled(simd_state));
      }
      TEST_F(SIMDSelectionCS, FirstCompiledIsSIMD32)
   {
      ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD8));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD16));
   ASSERT_TRUE(brw_simd_should_compile(simd_state, SIMD32));
            ASSERT_TRUE(brw_simd_any_compiled(simd_state));
      }
