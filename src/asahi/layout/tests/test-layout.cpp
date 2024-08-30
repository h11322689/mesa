   /*
   * Copyright 2022 Alyssa Rosenzweig
   * SPDX-License-Identifier: MIT
   */
      #include <gtest/gtest.h>
   #include "layout.h"
      TEST(Cubemap, Nonmipmapped)
   {
      struct ail_layout layout = {
      .width_px = 512,
   .height_px = 512,
   .depth_px = 6,
   .sample_count_sa = 1,
   .levels = 1,
   .tiling = AIL_TILING_TWIDDLED,
                        EXPECT_EQ(layout.layer_stride_B, ALIGN_POT(512 * 512 * 4, 0x4000));
      }
      TEST(Cubemap, RoundsToOnePage)
   {
      struct ail_layout layout = {
      .width_px = 63,
   .height_px = 63,
   .depth_px = 6,
   .sample_count_sa = 1,
   .levels = 6,
   .tiling = AIL_TILING_TWIDDLED,
                        EXPECT_EQ(layout.level_offsets_B[0], 0);
   EXPECT_EQ(layout.level_offsets_B[1], 0x4000);
   EXPECT_EQ(layout.level_offsets_B[2], 0x5000);
   EXPECT_EQ(layout.level_offsets_B[3], 0x5400);
   EXPECT_EQ(layout.level_offsets_B[4], 0x5500);
   EXPECT_TRUE(layout.page_aligned_layers);
   EXPECT_EQ(layout.layer_stride_B, 0x8000);
      }
      TEST(Linear, SmokeTestBuffer)
   {
      struct ail_layout layout = {
      .width_px = 81946,
   .height_px = 1,
   .depth_px = 1,
   .sample_count_sa = 1,
   .levels = 1,
   .tiling = AIL_TILING_LINEAR,
                           }
      TEST(Miptree, AllMipLevels)
   {
      struct ail_layout layout = {
      .width_px = 1024,
   .height_px = 1024,
   .depth_px = 1,
   .sample_count_sa = 1,
   .levels = 11,
   .tiling = AIL_TILING_TWIDDLED,
                           }
      TEST(Miptree, SomeMipLevels)
   {
      struct ail_layout layout = {
      .width_px = 1024,
   .height_px = 1024,
   .depth_px = 1,
   .sample_count_sa = 1,
   .levels = 4,
   .tiling = AIL_TILING_TWIDDLED,
                           }
      TEST(Miptree, SmallPartialMiptree2DArray)
   {
      struct ail_layout layout = {
      .width_px = 32,
   .height_px = 16,
   .depth_px = 64,
   .sample_count_sa = 1,
   .levels = 4,
   .tiling = AIL_TILING_TWIDDLED,
                        EXPECT_EQ(layout.layer_stride_B, 0xc00);
      }
      TEST(Miptree, SmallPartialMiptree3D)
   {
      struct ail_layout layout = {
      .width_px = 32,
   .height_px = 16,
   .depth_px = 64,
   .sample_count_sa = 1,
   .levels = 4,
   .mipmapped_z = true,
   .tiling = AIL_TILING_TWIDDLED,
                        EXPECT_EQ(layout.layer_stride_B, 0xc80);
      }
