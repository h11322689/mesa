   /*
   * Copyright © 2014-2017 Broadcom
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
      /** @file v3d_tiling.c
   *
   * Handles information about the V3D tiling formats, and loading and storing
   * from them.
   */
      #include <stdint.h>
   #include "v3d_tiling.h"
   #include "broadcom/common/v3d_cpu_tiling.h"
      /** Return the width in pixels of a 64-byte microtile. */
   uint32_t
   v3d_utile_width(int cpp)
   {
         switch (cpp) {
   case 1:
   case 2:
         case 4:
   case 8:
         case 16:
         default:
         }
      /** Return the height in pixels of a 64-byte microtile. */
   uint32_t
   v3d_utile_height(int cpp)
   {
         switch (cpp) {
   case 1:
         case 2:
   case 4:
         case 8:
   case 16:
         default:
         }
      /**
   * Returns the byte address for a given pixel within a utile.
   *
   * Utiles are 64b blocks of pixels in raster order, with 32bpp being a 4x4
   * arrangement.
   */
   static inline uint32_t
   v3d_get_utile_pixel_offset(uint32_t cpp, uint32_t x, uint32_t y)
   {
                           }
      /**
   * Returns the byte offset for a given pixel in a LINEARTILE layout.
   *
   * LINEARTILE is a single line of utiles in either the X or Y direction.
   */
   static inline uint32_t
   v3d_get_lt_pixel_offset(uint32_t cpp, uint32_t image_h, uint32_t x, uint32_t y)
   {
         uint32_t utile_w = v3d_utile_width(cpp);
   uint32_t utile_h = v3d_utile_height(cpp);
   uint32_t utile_index_x = x / utile_w;
                     return (64 * (utile_index_x + utile_index_y) +
               }
      /**
   * Returns the byte offset for a given pixel in a UBLINEAR layout.
   *
   * UBLINEAR is the layout where pixels are arranged in UIF blocks (2x2
   * utiles), and the UIF blocks are in 1 or 2 columns in raster order.
   */
   static inline uint32_t
   v3d_get_ublinear_pixel_offset(uint32_t cpp, uint32_t x, uint32_t y,
         {
         uint32_t utile_w = v3d_utile_width(cpp);
   uint32_t utile_h = v3d_utile_height(cpp);
   uint32_t ub_w = utile_w * 2;
   uint32_t ub_h = utile_h * 2;
   uint32_t ub_x = x / ub_w;
            return (256 * (ub_y * ublinear_number +
                  ((x & utile_w) ? 64 : 0) +
   ((y & utile_h) ? 128 : 0) +
      }
      static inline uint32_t
   v3d_get_ublinear_2_column_pixel_offset(uint32_t cpp, uint32_t image_h,
         {
         }
      static inline uint32_t
   v3d_get_ublinear_1_column_pixel_offset(uint32_t cpp, uint32_t image_h,
         {
         }
      /**
   * Returns the byte offset for a given pixel in a UIF layout.
   *
   * UIF is the general V3D tiling layout shared across 3D, media, and scanout.
   * It stores pixels in UIF blocks (2x2 utiles), and UIF blocks are stored in
   * 4x4 groups, and those 4x4 groups are then stored in raster order.
   */
   static inline uint32_t
   v3d_get_uif_pixel_offset(uint32_t cpp, uint32_t image_h, uint32_t x, uint32_t y,
         {
         uint32_t utile_w = v3d_utile_width(cpp);
   uint32_t utile_h = v3d_utile_height(cpp);
   uint32_t mb_width = utile_w * 2;
   uint32_t mb_height = utile_h * 2;
   uint32_t log2_mb_width = ffs(mb_width) - 1;
            /* Macroblock X, y */
   uint32_t mb_x = x >> log2_mb_width;
   uint32_t mb_y = y >> log2_mb_height;
   /* X, y within the macroblock */
   uint32_t mb_pixel_x = x - (mb_x << log2_mb_width);
            if (do_xor && (mb_x / 4) & 1)
            uint32_t mb_h = align(image_h, 1 << log2_mb_height) >> log2_mb_height;
                     bool top = mb_pixel_y < utile_h;
            /* Docs have this in pixels, we do bytes here. */
            uint32_t utile_x = mb_pixel_x & (utile_w - 1);
            uint32_t mb_pixel_address = (mb_base_addr +
                              }
      static inline uint32_t
   v3d_get_uif_xor_pixel_offset(uint32_t cpp, uint32_t image_h,
         {
         }
      static inline uint32_t
   v3d_get_uif_no_xor_pixel_offset(uint32_t cpp, uint32_t image_h,
         {
         }
      /* Loads/stores non-utile-aligned boxes by walking over the destination
   * rectangle, computing the address on the GPU, and storing/loading a pixel at
   * a time.
   */
   static inline void
   v3d_move_pixels_unaligned(void *gpu, uint32_t gpu_stride,
                           void *cpu, uint32_t cpu_stride,
   int cpp, uint32_t image_h,
   const struct pipe_box *box,
   {
         for (uint32_t y = 0; y < box->height; y++) {
                     for (int x = 0; x < box->width; x++) {
                                 if (false) {
                              if (is_load) {
         memcpy(cpu_row + x * cpp,
         } else {
         memcpy(gpu + pixel_offset,
   }
      /* Breaks the image down into utiles and calls either the fast whole-utile
   * load/store functions, or the unaligned fallback case.
   */
   static inline void
   v3d_move_pixels_general_percpp(void *gpu, uint32_t gpu_stride,
                                 void *cpu, uint32_t cpu_stride,
   int cpp, uint32_t image_h,
   {
         uint32_t utile_w = v3d_utile_width(cpp);
   uint32_t utile_h = v3d_utile_height(cpp);
   uint32_t utile_gpu_stride = utile_w * cpp;
   uint32_t x1 = box->x;
   uint32_t y1 = box->y;
   uint32_t x2 = box->x + box->width;
   uint32_t y2 = box->y + box->height;
   uint32_t align_x1 = align(x1, utile_w);
   uint32_t align_y1 = align(y1, utile_h);
   uint32_t align_x2 = x2 & ~(utile_w - 1);
            /* Load/store all the whole utiles first. */
   for (uint32_t y = align_y1; y < align_y2; y += utile_h) {
                     for (uint32_t x = align_x1; x < align_x2; x += utile_w) {
                                 if (is_load) {
         v3d_load_utile(utile_cpu, cpu_stride,
   } else {
                  /* If there were no aligned utiles in the middle, load/store the whole
      * thing unaligned.
      if (align_y2 <= align_y1 ||
         align_x2 <= align_x1) {
      v3d_move_pixels_unaligned(gpu, gpu_stride,
                                 /* Load/store the partial utiles. */
   struct pipe_box partial_boxes[4] = {
            /* Top */
   {
            .x = x1,
   .width = x2 - x1,
      },
   /* Bottom */
   {
            .x = x1,
   .width = x2 - x1,
      },
   /* Left */
   {
            .x = x1,
   .width = align_x1 - x1,
      },
   /* Right */
   {
            .x = align_x2,
   .width = x2 - align_x2,
   };
   for (int i = 0; i < ARRAY_SIZE(partial_boxes); i++) {
                                 v3d_move_pixels_unaligned(gpu, gpu_stride,
                  }
      static inline void
   v3d_move_pixels_general(void *gpu, uint32_t gpu_stride,
                                 void *cpu, uint32_t cpu_stride,
   int cpp, uint32_t image_h,
   {
         switch (cpp) {
   case 1:
            v3d_move_pixels_general_percpp(gpu, gpu_stride,
                        case 2:
            v3d_move_pixels_general_percpp(gpu, gpu_stride,
                        case 4:
            v3d_move_pixels_general_percpp(gpu, gpu_stride,
                        case 8:
            v3d_move_pixels_general_percpp(gpu, gpu_stride,
                        case 16:
            v3d_move_pixels_general_percpp(gpu, gpu_stride,
                        }
      static inline void
   v3d_move_tiled_image(void *gpu, uint32_t gpu_stride,
                        void *cpu, uint32_t cpu_stride,
   enum v3d_tiling_mode tiling_format,
      {
         switch (tiling_format) {
   case V3D_TILING_UIF_XOR:
            v3d_move_pixels_general(gpu, gpu_stride,
                        case V3D_TILING_UIF_NO_XOR:
            v3d_move_pixels_general(gpu, gpu_stride,
                        case V3D_TILING_UBLINEAR_2_COLUMN:
            v3d_move_pixels_general(gpu, gpu_stride,
                        case V3D_TILING_UBLINEAR_1_COLUMN:
            v3d_move_pixels_general(gpu, gpu_stride,
                        case V3D_TILING_LINEARTILE:
            v3d_move_pixels_general(gpu, gpu_stride,
                        default:
               }
      /**
   * Loads pixel data from the start (microtile-aligned) box in \p src to the
   * start of \p dst according to the given tiling format.
   */
   void
   v3d_load_tiled_image(void *dst, uint32_t dst_stride,
                           {
         v3d_move_tiled_image(src, src_stride,
                        dst, dst_stride,
      }
      /**
   * Stores pixel data from the start of \p src into a (microtile-aligned) box in
   * \p dst according to the given tiling format.
   */
   void
   v3d_store_tiled_image(void *dst, uint32_t dst_stride,
                           {
         v3d_move_tiled_image(dst, dst_stride,
                        src, src_stride,
      }
