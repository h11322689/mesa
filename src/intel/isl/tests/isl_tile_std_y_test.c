   /*
   * Copyright 2018 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <stdlib.h>
      #include "isl/isl.h"
      // An asssert that works regardless of NDEBUG.
   #define t_assert(cond) \
      do { \
      if (!(cond)) { \
      fprintf(stderr, "%s:%d: assertion failed\n", __FILE__, __LINE__); \
               static void
   assert_tile_size(enum isl_tiling tiling, enum isl_surf_dim dim,
                     {
      struct isl_tile_info tile_info;
            /* Sanity */
   t_assert(tile_info.tiling == tiling);
            t_assert(tile_info.logical_extent_el.w == w);
   t_assert(tile_info.logical_extent_el.h == h);
   t_assert(tile_info.logical_extent_el.d == d);
            bool is_Ys = tiling == ISL_TILING_SKL_Ys ||
                  assert(tile_size == tile_info.phys_extent_B.w *
            assert(tile_size == tile_info.logical_extent_el.w *
                        }
      static void
   assert_2d_tile_size(enum isl_tiling tiling, uint32_t bpb,
         {
   #define ASSERT_2D(tiling, bpb, samples, w, h, a) \
      assert_tile_size(tiling, ISL_SURF_DIM_2D, ISL_MSAA_LAYOUT_ARRAY, \
            /* Single sampled */
            /* Multisampled */
   if (tiling == ISL_TILING_SKL_Yf) {
      ASSERT_2D(tiling, bpb,  2, halign, valign, 1);
   ASSERT_2D(tiling, bpb,  4, halign, valign, 1);
   ASSERT_2D(tiling, bpb,  8, halign, valign, 1);
      } else {
      /* For gfx9 Ys and for both Yf and Ys on gfx11, we have to divide the
   * size by the number of samples.
   */
   ASSERT_2D(tiling, bpb,  2, halign / 2, valign / 1,  2);
   ASSERT_2D(tiling, bpb,  4, halign / 2, valign / 2,  4);
   ASSERT_2D(tiling, bpb,  8, halign / 4, valign / 2,  8);
            #undef ASSERT_2D
   }
      static void
   test_2d_tile_dimensions()
   {
      assert_2d_tile_size(ISL_TILING_SKL_Ys,  128,  64,  64);
   assert_2d_tile_size(ISL_TILING_SKL_Ys,   64, 128,  64);
   assert_2d_tile_size(ISL_TILING_SKL_Ys,   32, 128, 128);
   assert_2d_tile_size(ISL_TILING_SKL_Ys,   16, 256, 128);
            assert_2d_tile_size(ISL_TILING_SKL_Yf,  128,  16,  16);
   assert_2d_tile_size(ISL_TILING_SKL_Yf,   64,  32,  16);
   assert_2d_tile_size(ISL_TILING_SKL_Yf,   32,  32,  32);
   assert_2d_tile_size(ISL_TILING_SKL_Yf,   16,  64,  32);
            assert_2d_tile_size(ISL_TILING_ICL_Ys, 128,  64,  64);
   assert_2d_tile_size(ISL_TILING_ICL_Ys,  64, 128,  64);
   assert_2d_tile_size(ISL_TILING_ICL_Ys,  32, 128, 128);
   assert_2d_tile_size(ISL_TILING_ICL_Ys,  16, 256, 128);
            assert_2d_tile_size(ISL_TILING_ICL_Yf, 128,  16,  16);
   assert_2d_tile_size(ISL_TILING_ICL_Yf,  64,  32,  16);
   assert_2d_tile_size(ISL_TILING_ICL_Yf,  32,  32,  32);
   assert_2d_tile_size(ISL_TILING_ICL_Yf,  16,  64,  32);
      }
      static void
   test_3d_tile_dimensions()
   {
   #define ASSERT_3D(tiling, bpb, halign, valign, dalign) \
      assert_tile_size(tiling, ISL_SURF_DIM_3D, ISL_MSAA_LAYOUT_ARRAY, \
            ASSERT_3D(ISL_TILING_SKL_Ys, 128, 16, 16, 16);
   ASSERT_3D(ISL_TILING_SKL_Ys,  64, 32, 16, 16);
   ASSERT_3D(ISL_TILING_SKL_Ys,  32, 32, 32, 16);
   ASSERT_3D(ISL_TILING_SKL_Ys,  16, 32, 32, 32);
            ASSERT_3D(ISL_TILING_SKL_Yf, 128,  4,  8,  8);
   ASSERT_3D(ISL_TILING_SKL_Yf,  64,  8,  8,  8);
   ASSERT_3D(ISL_TILING_SKL_Yf,  32,  8, 16,  8);
   ASSERT_3D(ISL_TILING_SKL_Yf,  16,  8, 16, 16);
            ASSERT_3D(ISL_TILING_ICL_Ys, 128, 16, 16, 16);
   ASSERT_3D(ISL_TILING_ICL_Ys,  64, 32, 16, 16);
   ASSERT_3D(ISL_TILING_ICL_Ys,  32, 32, 32, 16);
   ASSERT_3D(ISL_TILING_ICL_Ys,  16, 32, 32, 32);
            ASSERT_3D(ISL_TILING_ICL_Yf, 128,  4,  8,  8);
   ASSERT_3D(ISL_TILING_ICL_Yf,  64,  8,  8,  8);
   ASSERT_3D(ISL_TILING_ICL_Yf,  32,  8, 16,  8);
   ASSERT_3D(ISL_TILING_ICL_Yf,  16,  8, 16, 16);
         #undef ASSERT_3D
   }
      int main(void)
   {
      test_2d_tile_dimensions();
               }
