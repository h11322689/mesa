   /**************************************************************************
   *
   * Copyright (C) 2011 Red Hat Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <stdio.h>
   #include "util/format/u_format.h"
   #include "util/format/u_format_rgtc.h"
   #include "util/format/u_format_latc.h"
   #include "util/rgtc.h"
   #include "util/u_math.h"
      void
   util_format_latc1_unorm_fetch_rgba_8unorm(uint8_t *restrict dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      /* Fix warnings here: */
   (void) util_format_unsigned_encode_rgtc_ubyte;
            util_format_unsigned_fetch_texel_rgtc(0, src, i, j, dst, 1);
   dst[1] = dst[0];
   dst[2] = dst[0];
      }
      void
   util_format_latc1_unorm_unpack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc1_unorm_pack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row,
         {
         }
      void
   util_format_latc1_unorm_unpack_rgba_float(void *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
      unsigned x, y, i, j;
            for(y = 0; y < height; y += 4) {
      const uint8_t *src = src_row;
   for(x = 0; x < width; x += 4) {
      for(j = 0; j < 4; ++j) {
      for(i = 0; i < 4; ++i) {
      float *dst = (float *)((uint8_t *)dst_row + (y + j)*dst_stride + (x + i)*16);
   uint8_t tmp_r;
   util_format_unsigned_fetch_texel_rgtc(0, src, i, j, &tmp_r, 1);
   dst[0] =
   dst[1] =
   dst[2] = ubyte_to_float(tmp_r);
         }
      }
         }
      void
   util_format_latc1_unorm_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride, const float *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc1_unorm_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      float *dst = in_dst;
            util_format_unsigned_fetch_texel_rgtc(0, src, i, j, &tmp_r, 1);
   dst[0] =
   dst[1] =
   dst[2] = ubyte_to_float(tmp_r);
      }
      void
   util_format_latc1_snorm_fetch_rgba_8unorm(UNUSED uint8_t *restrict dst, UNUSED const uint8_t *restrict src,
         {
         }
      void
   util_format_latc1_snorm_unpack_rgba_8unorm(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_latc1_snorm_pack_rgba_8unorm(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_latc1_snorm_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride, const float *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc1_snorm_unpack_rgba_float(void *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
      unsigned x, y, i, j;
            for(y = 0; y < height; y += 4) {
      const int8_t *src = (int8_t *)src_row;
   for(x = 0; x < width; x += 4) {
      for(j = 0; j < 4; ++j) {
      for(i = 0; i < 4; ++i) {
      float *dst = (float *)((uint8_t *)dst_row + (y + j)*dst_stride + (x + i)*16);
   int8_t tmp_r;
   util_format_signed_fetch_texel_rgtc(0, src, i, j, &tmp_r, 1);
   dst[0] =
   dst[1] =
   dst[2] = byte_to_float_tex(tmp_r);
         }
      }
         }
      void
   util_format_latc1_snorm_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      float *dst = in_dst;
            util_format_signed_fetch_texel_rgtc(0, (int8_t *)src, i, j, &tmp_r, 1);
   dst[0] =
   dst[1] =
   dst[2] = byte_to_float_tex(tmp_r);
      }
         void
   util_format_latc2_unorm_fetch_rgba_8unorm(uint8_t *restrict dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      util_format_unsigned_fetch_texel_rgtc(0, src, i, j, dst, 2);
   dst[1] = dst[0];
   dst[2] = dst[0];
      }
      void
   util_format_latc2_unorm_unpack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc2_unorm_pack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc2_unorm_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride, const float *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc2_unorm_unpack_rgba_float(void *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
      unsigned x, y, i, j;
            for(y = 0; y < height; y += 4) {
      const uint8_t *src = src_row;
   for(x = 0; x < width; x += 4) {
      for(j = 0; j < 4; ++j) {
      for(i = 0; i < 4; ++i) {
      float *dst = (float *)((uint8_t *)dst_row + (y + j)*dst_stride + (x + i)*16);
   uint8_t tmp_r, tmp_g;
   util_format_unsigned_fetch_texel_rgtc(0, src, i, j, &tmp_r, 2);
   util_format_unsigned_fetch_texel_rgtc(0, src + 8, i, j, &tmp_g, 2);
   dst[0] =
   dst[1] =
   dst[2] = ubyte_to_float(tmp_r);
         }
      }
         }
      void
   util_format_latc2_unorm_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      float *dst = in_dst;
            util_format_unsigned_fetch_texel_rgtc(0, src, i, j, &tmp_r, 2);
   util_format_unsigned_fetch_texel_rgtc(0, src + 8, i, j, &tmp_g, 2);
   dst[0] =
   dst[1] =
   dst[2] = ubyte_to_float(tmp_r);
      }
         void
   util_format_latc2_snorm_fetch_rgba_8unorm(UNUSED uint8_t *restrict dst, UNUSED const uint8_t *restrict src,
         {
         }
      void
   util_format_latc2_snorm_unpack_rgba_8unorm(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_latc2_snorm_pack_rgba_8unorm(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_latc2_snorm_unpack_rgba_float(void *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
      unsigned x, y, i, j;
            for(y = 0; y < height; y += 4) {
      const int8_t *src = (int8_t *)src_row;
   for(x = 0; x < width; x += 4) {
      for(j = 0; j < 4; ++j) {
      for(i = 0; i < 4; ++i) {
      float *dst = (float *)(uint8_t *)dst_row + (y + j)*dst_stride + (x + i)*16;
   int8_t tmp_r, tmp_g;
   util_format_signed_fetch_texel_rgtc(0, src, i, j, &tmp_r, 2);
   util_format_signed_fetch_texel_rgtc(0, src + 8, i, j, &tmp_g, 2);
   dst[0] =
   dst[1] =
   dst[2] = byte_to_float_tex(tmp_r);
         }
      }
         }
      void
   util_format_latc2_snorm_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride, const float *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_latc2_snorm_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      float *dst = in_dst;
            util_format_signed_fetch_texel_rgtc(0, (int8_t *)src, i, j, &tmp_r, 2);
   util_format_signed_fetch_texel_rgtc(0, (int8_t *)src + 8, i, j, &tmp_g, 2);
   dst[0] =
   dst[1] =
   dst[2] = byte_to_float_tex(tmp_r);
      }
   