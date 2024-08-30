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
      #include "pan_tiling.h"
      #include <gtest/gtest.h>
      /*
   * Reference tiling algorithm, written for clarity rather than performance. See
   * docs/drivers/panfrost.rst for details on the format.
   */
      static unsigned
   u_order(unsigned x, unsigned y)
   {
               unsigned xy0 = ((x ^ y) & 1) ? 1 : 0;
   unsigned xy1 = ((x ^ y) & 2) ? 1 : 0;
   unsigned xy2 = ((x ^ y) & 4) ? 1 : 0;
            unsigned y0 = (y & 1) ? 1 : 0;
   unsigned y1 = (y & 2) ? 1 : 0;
   unsigned y2 = (y & 4) ? 1 : 0;
            return (xy0 << 0) | (y0 << 1) | (xy1 << 2) | (y1 << 3) | (xy2 << 4) |
      }
      /* x/y are in blocks */
   static unsigned
   tiled_offset(unsigned x, unsigned y, unsigned stride, unsigned tilesize,
         {
      unsigned tile_x = x / tilesize;
            unsigned x_in_tile = x % tilesize;
                     unsigned row_offset = tile_y * stride;
   unsigned col_offset = (tile_x * tilesize * tilesize) * blocksize;
               }
      static unsigned
   linear_offset(unsigned x, unsigned y, unsigned stride, unsigned blocksize)
   {
         }
      static void
   ref_access_tiled(void *dst, const void *src, unsigned region_x,
                     {
      const struct util_format_description *desc = util_format_description(format);
            unsigned tilesize = (desc->block.width > 1) ? 4 : 16;
            unsigned w_block = w / desc->block.width;
            unsigned region_x_block = region_x / desc->block.width;
            for (unsigned linear_y_block = 0; linear_y_block < h_block;
      ++linear_y_block) {
   for (unsigned linear_x_block = 0; linear_x_block < w_block;
                                       if (dst_is_tiled) {
      dst_offset = tiled_offset(tiled_x_block, tiled_y_block, dst_stride,
         src_offset = linear_offset(linear_x_block, linear_y_block,
      } else {
      dst_offset = linear_offset(linear_x_block, linear_y_block,
         src_offset = tiled_offset(tiled_x_block, tiled_y_block, src_stride,
               memcpy((uint8_t *)dst + dst_offset, (const uint8_t *)src + src_offset,
            }
      /*
   * Helper to build test cases for tiled texture access. This test suite compares
   * the above reference tiling algorithm to the optimized algorithm used in
   * production.
   */
   static void
   test(unsigned width, unsigned height, unsigned rx, unsigned ry, unsigned rw,
         {
      unsigned bpp = util_format_get_blocksize(format);
            unsigned tiled_width = ALIGN_POT(width, 16);
   unsigned tiled_height = ALIGN_POT(height, 16);
            unsigned dst_stride = store ? tiled_stride : linear_stride;
            void *tiled = calloc(bpp, tiled_width * tiled_height);
   void *linear = calloc(bpp, rw * linear_stride);
   void *ref =
            if (store) {
      for (unsigned i = 0; i < bpp * rw * linear_stride; ++i) {
                  panfrost_store_tiled_image(tiled, linear, rx, ry, rw, rh, dst_stride,
      } else {
      for (unsigned i = 0; i < bpp * tiled_width * tiled_height; ++i) {
                  panfrost_load_tiled_image(linear, tiled, rx, ry, rw, rh, dst_stride,
               ref_access_tiled(ref, store ? linear : tiled, rx, ry, rw, rh, dst_stride,
            if (store)
         else
            free(ref);
   free(tiled);
      }
      static void
   test_ldst(unsigned width, unsigned height, unsigned rx, unsigned ry,
               {
      test(width, height, rx, ry, rw, rh, linear_stride, format, true);
      }
      TEST(UInterleavedTiling, RegulatFormats)
   {
      /* 8-bit */
            /* 16-bit */
            /* 24-bit */
            /* 32-bit */
            /* 48-bit */
            /* 64-bit */
            /* 96-bit */
            /* 128-bit */
      }
      TEST(UInterleavedTiling, UnpackedStrides)
   {
      test_ldst(23, 17, 0, 0, 23, 17, 369 * 1, PIPE_FORMAT_R8_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 2, PIPE_FORMAT_R8G8_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 3, PIPE_FORMAT_R8G8B8_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 4, PIPE_FORMAT_R32_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 6, PIPE_FORMAT_R16G16B16_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 8, PIPE_FORMAT_R32G32_SINT);
   test_ldst(23, 17, 0, 0, 23, 17, 369 * 12, PIPE_FORMAT_R32G32B32_SINT);
      }
      TEST(UInterleavedTiling, PartialAccess)
   {
      test_ldst(23, 17, 3, 1, 13, 7, 369 * 1, PIPE_FORMAT_R8_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 2, PIPE_FORMAT_R8G8_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 3, PIPE_FORMAT_R8G8B8_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 4, PIPE_FORMAT_R32_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 6, PIPE_FORMAT_R16G16B16_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 8, PIPE_FORMAT_R32G32_UNORM);
   test_ldst(23, 17, 3, 1, 13, 7, 369 * 12, PIPE_FORMAT_R32G32B32_UNORM);
      }
      TEST(UInterleavedTiling, ETC)
   {
      /* Block alignment assumed */
   test_ldst(32, 32, 0, 0, 32, 32, 512, PIPE_FORMAT_ETC1_RGB8);
   test_ldst(32, 32, 0, 0, 32, 32, 512, PIPE_FORMAT_ETC2_RGB8A1);
      }
      TEST(UInterleavedTiling, PartialETC)
   {
      /* Block alignment assumed */
   test_ldst(32, 32, 4, 8, 16, 12, 512, PIPE_FORMAT_ETC1_RGB8);
   test_ldst(32, 32, 4, 8, 16, 12, 512, PIPE_FORMAT_ETC2_RGB8A1);
      }
      TEST(UInterleavedTiling, DXT)
   {
      /* Block alignment assumed */
   test_ldst(32, 32, 0, 0, 32, 32, 512, PIPE_FORMAT_DXT1_RGB);
   test_ldst(32, 32, 0, 0, 32, 32, 512, PIPE_FORMAT_DXT3_RGBA);
      }
      TEST(UInterleavedTiling, PartialDXT)
   {
      /* Block alignment assumed */
   test_ldst(32, 32, 4, 8, 16, 12, 512, PIPE_FORMAT_DXT1_RGB);
   test_ldst(32, 32, 4, 8, 16, 12, 512, PIPE_FORMAT_DXT3_RGBA);
      }
      TEST(UInterleavedTiling, ASTC)
   {
      /* Block alignment assumed */
   test_ldst(40, 40, 0, 0, 40, 40, 512, PIPE_FORMAT_ASTC_4x4);
   test_ldst(50, 40, 0, 0, 50, 40, 512, PIPE_FORMAT_ASTC_5x4);
      }
      TEST(UInterleavedTiling, PartialASTC)
   {
      /* Block alignment assumed */
   test_ldst(40, 40, 4, 4, 16, 8, 512, PIPE_FORMAT_ASTC_4x4);
   test_ldst(50, 40, 5, 4, 10, 8, 512, PIPE_FORMAT_ASTC_5x4);
      }
