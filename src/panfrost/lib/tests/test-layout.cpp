   /*
   * Copyright (C) 2022 Collabora, Ltd.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "pan_texture.h"
      #include <gtest/gtest.h>
      TEST(BlockSize, Linear)
   {
      enum pipe_format format[] = {PIPE_FORMAT_R32G32B32_FLOAT,
                  for (unsigned i = 0; i < ARRAY_SIZE(format); ++i) {
      struct pan_block_size blk =
            EXPECT_EQ(blk.width, 1);
         }
      TEST(BlockSize, UInterleavedRegular)
   {
      enum pipe_format format[] = {
      PIPE_FORMAT_R32G32B32_FLOAT,
               for (unsigned i = 0; i < ARRAY_SIZE(format); ++i) {
      struct pan_block_size blk = panfrost_block_size(
            EXPECT_EQ(blk.width, 16);
         }
      TEST(BlockSize, UInterleavedBlockCompressed)
   {
               for (unsigned i = 0; i < ARRAY_SIZE(format); ++i) {
      struct pan_block_size blk = panfrost_block_size(
            EXPECT_EQ(blk.width, 4);
         }
      TEST(BlockSize, AFBCFormatInvariant16x16)
   {
      enum pipe_format format[] = {PIPE_FORMAT_R32G32B32_FLOAT,
                  uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         for (unsigned i = 0; i < ARRAY_SIZE(format); ++i) {
               EXPECT_EQ(blk.width, 16);
         }
      TEST(BlockSize, AFBCFormatInvariant32x8)
   {
      enum pipe_format format[] = {PIPE_FORMAT_R32G32B32_FLOAT,
                  uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
         for (unsigned i = 0; i < ARRAY_SIZE(format); ++i) {
               EXPECT_EQ(blk.width, 32);
         }
      TEST(BlockSize, AFBCSuperblock16x16)
   {
      uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         EXPECT_EQ(panfrost_afbc_superblock_size(modifier).width, 16);
            EXPECT_EQ(panfrost_afbc_superblock_size(modifier).height, 16);
               }
      TEST(BlockSize, AFBCSuperblock32x8)
   {
      uint64_t modifier = DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
            EXPECT_EQ(panfrost_afbc_superblock_size(modifier).width, 32);
            EXPECT_EQ(panfrost_afbc_superblock_size(modifier).height, 8);
               }
      TEST(BlockSize, AFBCSuperblock64x4)
   {
      uint64_t modifier = DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_64x4 |
            EXPECT_EQ(panfrost_afbc_superblock_size(modifier).width, 64);
            EXPECT_EQ(panfrost_afbc_superblock_size(modifier).height, 4);
               }
      /* Calculate Bifrost line stride, since we have reference formulas for Bifrost
   * stride calculations.
   */
   static uint32_t
   pan_afbc_line_stride(uint64_t modifier, uint32_t width)
   {
      return pan_afbc_stride_blocks(modifier,
      }
      /* Which form of the stride we specify is hardware specific (row stride for
   * Valhall, line stride for Bifrost). However, the layout code is hardware
   * independent, so we test both row stride and line stride calculations.
   */
   TEST(AFBCStride, Linear)
   {
      uint64_t modifiers[] = {
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
         DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_64x4 |
               for (unsigned m = 0; m < ARRAY_SIZE(modifiers); ++m) {
               uint32_t sw = panfrost_afbc_superblock_width(modifier);
            for (unsigned i = 0; i < ARRAY_SIZE(cases); ++i) {
                              EXPECT_EQ(pan_afbc_line_stride(modifier, width),
            }
      TEST(AFBCStride, Tiled)
   {
      uint64_t modifiers[] = {
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
         DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_64x4 |
               for (unsigned m = 0; m < ARRAY_SIZE(modifiers); ++m) {
               uint32_t sw = panfrost_afbc_superblock_width(modifier);
            for (unsigned i = 0; i < ARRAY_SIZE(cases); ++i) {
                              EXPECT_EQ(pan_afbc_line_stride(modifier, width),
            }
      TEST(LegacyStride, FromLegacyLinear)
   {
      EXPECT_EQ(panfrost_from_legacy_stride(1920 * 4, PIPE_FORMAT_R8G8B8A8_UINT,
               EXPECT_EQ(panfrost_from_legacy_stride(53, PIPE_FORMAT_R8_SNORM,
               EXPECT_EQ(panfrost_from_legacy_stride(60, PIPE_FORMAT_ETC2_RGB8,
            }
      TEST(LegacyStride, FromLegacyInterleaved)
   {
      EXPECT_EQ(
      panfrost_from_legacy_stride(1920 * 4, PIPE_FORMAT_R8G8B8A8_UINT,
               EXPECT_EQ(
      panfrost_from_legacy_stride(53, PIPE_FORMAT_R8_SNORM,
               EXPECT_EQ(
      panfrost_from_legacy_stride(60, PIPE_FORMAT_ETC2_RGB8,
         }
      TEST(LegacyStride, FromLegacyAFBC)
   {
      uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_32x8 |
         EXPECT_EQ(panfrost_from_legacy_stride(1920 * 4, PIPE_FORMAT_R8G8B8A8_UINT,
               EXPECT_EQ(panfrost_from_legacy_stride(64, PIPE_FORMAT_R8_SNORM, modifier),
      }
      /* dEQP-GLES3.functional.texture.format.compressed.etc1_2d_pot */
   TEST(Layout, ImplicitLayoutInterleavedETC2)
   {
               struct pan_image_layout l = {
      .modifier = DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED,
   .format = PIPE_FORMAT_ETC2_RGB8,
   .width = 128,
   .height = 128,
   .depth = 1,
   .nr_samples = 1,
   .dim = MALI_TEXTURE_DIMENSION_2D,
         unsigned offsets[9] = {0,     8192,  10240, 10752, 10880,
                     for (unsigned i = 0; i < 8; ++i) {
      unsigned size = (offsets[i + 1] - offsets[i]);
            if (size == 64)
         else
         }
      TEST(Layout, ImplicitLayoutInterleavedASTC5x5)
   {
               struct pan_image_layout l = {
      .modifier = DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED,
   .format = PIPE_FORMAT_ASTC_5x5,
   .width = 50,
   .height = 50,
   .depth = 1,
   .nr_samples = 1,
   .dim = MALI_TEXTURE_DIMENSION_2D,
                  /* The image is 50x50 pixels, with 5x5 blocks. So it is a 10x10 grid of ASTC
   * blocks. 4x4 tiles of ASTC blocks are u-interleaved, so we have to round up
   * to a 12x12 grid. So we need space for 144 ASTC blocks. Each ASTC block is
   * 16 bytes (128-bits), so we require 2304 bytes, with a row stride of 12 *
   * 16 * 4 = 192 bytes.
   */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 768);
   EXPECT_EQ(l.slices[0].surface_stride, 2304);
      }
      TEST(Layout, ImplicitLayoutLinearASTC5x5)
   {
               struct pan_image_layout l = {.modifier = DRM_FORMAT_MOD_LINEAR,
                              .format = PIPE_FORMAT_ASTC_5x5,
   .width = 50,
                  /* The image is 50x50 pixels, with 5x5 blocks. So it is a 10x10 grid of ASTC
   * blocks. Each ASTC block is 16 bytes, so the row stride is 160 bytes,
   * rounded up to the cache line (192 bytes).  There are 10 rows, so we have
   * 1920 bytes total.
   */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 192);
   EXPECT_EQ(l.slices[0].surface_stride, 1920);
      }
      /* dEQP-GLES3.functional.texture.format.unsized.rgba_unsigned_byte_3d_pot */
   TEST(AFBCLayout, Linear3D)
   {
               uint64_t modifier = DRM_FORMAT_MOD_ARM_AFBC(
            struct pan_image_layout l = {.modifier = modifier,
                              .format = PIPE_FORMAT_R8G8B8A8_UNORM,
   .width = 8,
                  /* AFBC Surface stride is bytes between consecutive surface headers, which is
   * the header size since this is a 3D texture. At superblock size 16x16, the
   * 8x32 layer has 1x2 superblocks, so the header size is 2 * 16 = 32 bytes,
   * rounded up to cache line 64.
   *
   * There is only 1 superblock per row, so the row stride is the bytes per 1
   * header block = 16.
   *
   * There are 16 layers of size 64 so afbc.header_size = 16 * 64 = 1024.
   *
   * Each 16x16 superblock consumes 16 * 16 * 4 = 1024 bytes. There are 2 * 1 *
   * 16 superblocks in the image, so body size is 32768.
   */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 16);
   EXPECT_EQ(l.slices[0].afbc.header_size, 1024);
   EXPECT_EQ(l.slices[0].afbc.body_size, 32768);
   EXPECT_EQ(l.slices[0].afbc.surface_stride, 64);
   EXPECT_EQ(l.slices[0].surface_stride, 2048); /* XXX: Not meaningful? */
      }
      TEST(AFBCLayout, Tiled16x16)
   {
               uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         struct pan_image_layout l = {.modifier = modifier,
                              .format = PIPE_FORMAT_R8G8B8A8_UNORM,
   .width = 917,
                  /* The image is 917x417. Superblocks are 16x16, so there are 58x27
   * superblocks. Superblocks are grouped into 8x8 tiles, so there are 8x4
   * tiles of superblocks. So the row stride is 16 * 8 * 8 * 8 = 8192 bytes.
   * There are 4 tiles vertically, so the header is 8192 * 4 = 32768 bytes.
   * This is already 4096-byte aligned.
   *
   * Each tile of superblock contains 128x128 pixels and each pixel is 4 bytes,
   * so tiles are 65536 bytes, meaning the payload is 8 * 4 * 65536 = 2097152
   * bytes.
   *
   * In total, the AFBC surface is 32768 + 2097152 = 2129920 bytes.
   */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 8192);
   EXPECT_EQ(l.slices[0].afbc.header_size, 32768);
   EXPECT_EQ(l.slices[0].afbc.body_size, 2097152);
   EXPECT_EQ(l.slices[0].surface_stride, 2129920);
      }
      TEST(AFBCLayout, Linear16x16Minimal)
   {
               uint64_t modifier = DRM_FORMAT_MOD_ARM_AFBC(
            struct pan_image_layout l = {.modifier = modifier,
                              .format = PIPE_FORMAT_R8_UNORM,
   .width = 1,
                  /* Image is 1x1 to test for correct alignment everywhere. */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 16);
   EXPECT_EQ(l.slices[0].afbc.header_size, 64);
   EXPECT_EQ(l.slices[0].afbc.body_size, 32 * 8);
   EXPECT_EQ(l.slices[0].surface_stride, 64 + (32 * 8));
      }
      TEST(AFBCLayout, Tiled16x16Minimal)
   {
               uint64_t modifier =
      DRM_FORMAT_MOD_ARM_AFBC(AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 |
         struct pan_image_layout l = {.modifier = modifier,
                              .format = PIPE_FORMAT_R8_UNORM,
   .width = 1,
                  /* Image is 1x1 to test for correct alignment everywhere. */
   EXPECT_EQ(l.slices[0].offset, 0);
   EXPECT_EQ(l.slices[0].row_stride, 16 * 8 * 8);
   EXPECT_EQ(l.slices[0].afbc.header_size, 4096);
   EXPECT_EQ(l.slices[0].afbc.body_size, 32 * 8 * 8 * 8);
   EXPECT_EQ(l.slices[0].surface_stride, 4096 + (32 * 8 * 8 * 8));
      }
