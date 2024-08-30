   /*
   * Copyright © 2017 Broadcom
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
      /** @file vc4_tiling_lt.c
   *
   * Helper functions from vc4_tiling.c that will be compiled for using NEON
   * assembly or not.
   *
   * If V3D_BUILD_NEON is set, then the functions will be suffixed with _neon.
   * They will only use NEON assembly if __ARM_ARCH is also set, to keep the x86
   * sim build working.
   */
      #include <string.h>
   #include "pipe/p_state.h"
   #include "vc4_tiling.h"
   #include "broadcom/common/v3d_cpu_tiling.h"
      #ifdef V3D_BUILD_NEON
   #define NEON_TAG(x) x ## _neon
   #else
   #define NEON_TAG(x) x ## _base
   #endif
      /** Returns the stride in bytes of a 64-byte microtile. */
   static uint32_t
   vc4_utile_stride(int cpp)
   {
         switch (cpp) {
   case 1:
         case 2:
   case 4:
   case 8:
         default:
         }
      /**
   * Returns the X value into the address bits for LT tiling.
   *
   * The LT tile load/stores rely on the X bits not intersecting with the Y
   * bits.  Because of this, we have to choose to put the utile index within the
   * LT tile into one of the two values, and we do so in swizzle_lt_x() to make
   * NPOT handling easier.
   */
   static uint32_t
   swizzle_lt_x(int x, int cpp)
   {
         switch (cpp) {
   case 1:
            /* 8x8 inside of 4x4 */
      case 2:
            /* 8x4 inside of 4x4 */
      case 4:
            /* 4x4 inside of 4x4 */
      case 8:
            /* 2x4 inside of 4x4 */
      default:
         }
      /**
   * Returns the Y value into the address bits for LT tiling.
   *
   * The LT tile load/stores rely on the X bits not intersecting with the Y
   * bits.
   */
   static uint32_t
   swizzle_lt_y(int y, int cpp)
   {
            switch (cpp) {
   case 1:
               case 2:
               case 4:
               case 8:
               default:
         }
      /**
   * Helper for loading or storing to an LT image, where the box is aligned
   * to utiles.
   *
   * This just breaks the box down into calls to the fast
   * vc4_load_utile/vc4_store_utile helpers.
   */
   static inline void
   vc4_lt_image_aligned(void *gpu, uint32_t gpu_stride,
               {
         uint32_t utile_w = vc4_utile_width(cpp);
   uint32_t utile_h = vc4_utile_height(cpp);
   uint32_t utile_stride = vc4_utile_stride(cpp);
   uint32_t xstart = box->x;
            for (uint32_t y = 0; y < box->height; y += utile_h) {
            for (uint32_t x = 0; x < box->width; x += utile_w) {
            void *gpu_tile = gpu + ((ystart + y) * gpu_stride +
         if (to_cpu) {
         v3d_load_utile(cpu + (cpu_stride * y +
                     } else {
         v3d_store_utile(gpu_tile,
               }
      /**
   * Helper for loading or storing to an LT image, where the box is not aligned
   * to utiles.
   *
   * This walks through the raster-order data, copying to/from the corresponding
   * tiled pixel.  This means we don't get write-combining on stores, but the
   * loop is very few CPU instructions since the memcpy will be inlined.
   */
   static inline void
   vc4_lt_image_unaligned(void *gpu, uint32_t gpu_stride,
               {
            /* These are the address bits for the start of the box, split out into
      * x/y so that they can be incremented separately in their loops.
      uint32_t offs_x0 = swizzle_lt_x(box->x, cpp);
   uint32_t offs_y = swizzle_lt_y(box->y, cpp);
   /* The *_mask values are "what bits of the address are from x or y" */
   uint32_t x_mask = swizzle_lt_x(~0, cpp);
   uint32_t y_mask = swizzle_lt_y(~0, cpp);
                              for (uint32_t y = 0; y < box->height; y++) {
                              for (uint32_t x = 0; x < box->width; x++) {
            /* Use a memcpy here to move a pixel's worth of data.
   * We're relying on this function to be inlined, so
   * this will get expanded into the appropriate 1, 2,
   * or 4-byte move.
   */
   if (to_cpu) {
                              /* This math trick with x_mask increments offs_x by 1
                     offs_y = (offs_y - y_mask) & y_mask;
   /* When offs_y wraps (we hit the end of the utile), we
   * increment offs_x0 by effectively the utile stride.
                     }
      /**
   * General LT image load/store helper.
   */
   static inline void
   vc4_lt_image_helper(void *gpu, uint32_t gpu_stride,
               {
         if (box->x & (vc4_utile_width(cpp) - 1) ||
         box->y & (vc4_utile_height(cpp) - 1) ||
   box->width & (vc4_utile_width(cpp) - 1) ||
   box->height & (vc4_utile_height(cpp) - 1)) {
      vc4_lt_image_unaligned(gpu, gpu_stride,
      } else {
            vc4_lt_image_aligned(gpu, gpu_stride,
      }
      static inline void
   vc4_lt_image_cpp_helper(void *gpu, uint32_t gpu_stride,
               {
         switch (cpp) {
   case 1:
            vc4_lt_image_helper(gpu, gpu_stride, cpu, cpu_stride, 1, box,
      case 2:
            vc4_lt_image_helper(gpu, gpu_stride, cpu, cpu_stride, 2, box,
      case 4:
            vc4_lt_image_helper(gpu, gpu_stride, cpu, cpu_stride, 4, box,
      case 8:
            vc4_lt_image_helper(gpu, gpu_stride, cpu, cpu_stride, 8, box,
      default:
         }
      void
   NEON_TAG(vc4_load_lt_image)(void *dst, uint32_t dst_stride,
               {
         vc4_lt_image_cpp_helper(src, src_stride, dst, dst_stride, cpp, box,
   }
      void
   NEON_TAG(vc4_store_lt_image)(void *dst, uint32_t dst_stride,
               {
         vc4_lt_image_cpp_helper(dst, dst_stride, src, src_stride, cpp, box,
   }
