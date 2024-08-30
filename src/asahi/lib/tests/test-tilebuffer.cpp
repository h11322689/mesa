   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include "util/format/u_format.h"
   #include "agx_tilebuffer.h"
      #include <gtest/gtest.h>
      struct test {
      const char *name;
   uint8_t nr_samples;
   enum pipe_format formats[8];
   struct agx_tilebuffer_layout layout;
      };
      /* clang-format off */
   struct test tests[] = {
      {
      "Simple test",
   1,
   { PIPE_FORMAT_R8G8B8A8_UNORM },
   {
      ._offset_B = { 0 },
   .sample_size_B = 8,
   .nr_samples = 1,
      },
      },
   {
      "MSAA 2x",
   2,
   { PIPE_FORMAT_R8G8B8A8_UNORM },
   {
      ._offset_B = { 0 },
   .sample_size_B = 8,
   .nr_samples = 2,
      },
      },
   {
      "MSAA 4x",
   4,
   { PIPE_FORMAT_R8G8B8A8_UNORM },
   {
      ._offset_B = { 0 },
   .sample_size_B = 8,
   .nr_samples = 4,
      },
      },
   {
      "MRT",
   1,
   {
      PIPE_FORMAT_R16_SINT,
   PIPE_FORMAT_R32G32_FLOAT,
   PIPE_FORMAT_R8_SINT,
      },
   {
      ._offset_B = { 0, 4, 12, 16 },
   .sample_size_B = 24,
   .nr_samples = 1,
      },
      },
   {
      "MRT with MSAA 2x",
   2,
   {
      PIPE_FORMAT_R16_SINT,
   PIPE_FORMAT_R32G32_FLOAT,
   PIPE_FORMAT_R8_SINT,
      },
   {
      ._offset_B = { 0, 4, 12, 16 },
   .sample_size_B = 24,
   .nr_samples = 2,
      },
      },
   {
      "MRT with MSAA 4x",
   4,
   {
      PIPE_FORMAT_R16_SINT,
   PIPE_FORMAT_R32G32_FLOAT,
   PIPE_FORMAT_R8_SINT,
      },
   {
      ._offset_B = { 0, 4, 12, 16 },
   .sample_size_B = 24,
   .nr_samples = 4,
      },
      },
   {
      "MRT test requiring 2 alignment on the second RT",
   1,
   { PIPE_FORMAT_R8_UNORM, PIPE_FORMAT_R16G16_SNORM },
   {
      ._offset_B = { 0, 2 },
   .sample_size_B = 8,
   .nr_samples = 1,
      },
      },
   {
      "Simple MRT test requiring 4 alignment on the second RT",
   1,
   { PIPE_FORMAT_R8_UNORM, PIPE_FORMAT_R10G10B10A2_UNORM },
   {
      ._offset_B = { 0, 4 },
   .sample_size_B = 8,
   .nr_samples = 1,
      },
         };
   /* clang-format on */
      TEST(Tilebuffer, Layouts)
   {
      for (unsigned i = 0; i < ARRAY_SIZE(tests); ++i) {
               for (nr_cbufs = 0; nr_cbufs < ARRAY_SIZE(tests[i].formats) &&
                        struct agx_tilebuffer_layout actual = agx_build_tilebuffer_layout(
            ASSERT_EQ(tests[i].layout.sample_size_B, actual.sample_size_B)
         ASSERT_EQ(tests[i].layout.nr_samples, actual.nr_samples) << tests[i].name;
   ASSERT_EQ(tests[i].layout.tile_size.width, actual.tile_size.width)
         ASSERT_EQ(tests[i].layout.tile_size.height, actual.tile_size.height)
         ASSERT_EQ(tests[i].total_size, agx_tilebuffer_total_size(&tests[i].layout))
         }
