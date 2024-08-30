   /**************************************************************************
   *
   * Copyright 2010 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   **************************************************************************/
         #include "util/format/u_format_other.h"
   #include "util/u_math.h"
   #include "util/format_rgb9e5.h"
   #include "util/format_r11g11b10f.h"
         void
   util_format_r9g9b9e5_float_unpack_rgba_float(void *restrict dst_row,
               {
      unsigned x;
   float *dst = dst_row;
   const uint8_t *src = src_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   rgb9e5_to_float3(value, dst);
   dst[3] = 1; /* a */
   src += 4;
         }
      void
   util_format_r9g9b9e5_float_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride,
               {
      unsigned x, y;
   for(y = 0; y < height; y += 1) {
      const float *src = src_row;
   uint8_t *dst = dst_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(float3_to_rgb9e5(src));
   *(uint32_t *)dst = value;
   src += 4;
      }
   dst_row += dst_stride;
         }
      void
   util_format_r9g9b9e5_float_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src,
         {
      float *dst = in_dst;
   uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   rgb9e5_to_float3(value, dst);
      }
         void
   util_format_r9g9b9e5_float_unpack_rgba_8unorm(uint8_t *restrict dst_row,
               {
      unsigned x;
   float p[3];
   uint8_t *dst = dst_row;
   const uint8_t *src = src_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   rgb9e5_to_float3(value, p);
   dst[0] = float_to_ubyte(p[0]); /* r */
   dst[1] = float_to_ubyte(p[1]); /* g */
   dst[2] = float_to_ubyte(p[2]); /* b */
   dst[3] = 255; /* a */
   src += 4;
         }
         void
   util_format_r9g9b9e5_float_pack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride,
               {
      unsigned x, y;
   float p[3];
   for(y = 0; y < height; y += 1) {
      const uint8_t *src = src_row;
   uint8_t *dst = dst_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value;
   p[0] = ubyte_to_float(src[0]);
   p[1] = ubyte_to_float(src[1]);
   p[2] = ubyte_to_float(src[2]);
   value = util_cpu_to_le32(float3_to_rgb9e5(p));
   *(uint32_t *)dst = value;
   src += 4;
      }
   dst_row += dst_stride;
         }
         void
   util_format_r11g11b10_float_unpack_rgba_float(void *restrict dst_row,
               {
      unsigned x;
   float *dst = dst_row;
   const uint8_t *src = src_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   r11g11b10f_to_float3(value, dst);
   dst[3] = 1; /* a */
   src += 4;
         }
      void
   util_format_r11g11b10_float_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride,
               {
      unsigned x, y;
   for(y = 0; y < height; y += 1) {
      const float *src = src_row;
   uint8_t *dst = dst_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(float3_to_r11g11b10f(src));
   *(uint32_t *)dst = value;
   src += 4;
      }
   dst_row += dst_stride;
         }
      void
   util_format_r11g11b10_float_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src,
         {
      float *dst = in_dst;
   uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   r11g11b10f_to_float3(value, dst);
      }
         void
   util_format_r11g11b10_float_unpack_rgba_8unorm(uint8_t *restrict dst_row,
               {
      unsigned x;
   float p[3];
   uint8_t *dst = dst_row;
   const uint8_t *src = src_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value = util_cpu_to_le32(*(const uint32_t *)src);
   r11g11b10f_to_float3(value, p);
   dst[0] = float_to_ubyte(p[0]); /* r */
   dst[1] = float_to_ubyte(p[1]); /* g */
   dst[2] = float_to_ubyte(p[2]); /* b */
   dst[3] = 255; /* a */
   src += 4;
         }
         void
   util_format_r11g11b10_float_pack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride,
               {
      unsigned x, y;
   float p[3];
   for(y = 0; y < height; y += 1) {
      const uint8_t *src = src_row;
   uint8_t *dst = dst_row;
   for(x = 0; x < width; x += 1) {
      uint32_t value;
   p[0] = ubyte_to_float(src[0]);
   p[1] = ubyte_to_float(src[1]);
   p[2] = ubyte_to_float(src[2]);
   value = util_cpu_to_le32(float3_to_r11g11b10f(p));
   *(uint32_t *)dst = value;
   src += 4;
      }
   dst_row += dst_stride;
         }
      /*
   * PIPE_FORMAT_R8G8Bx_SNORM
   *
   * A.k.a. D3DFMT_CxV8U8
   */
      static uint8_t
   r8g8bx_derive(int16_t r, int16_t g)
   {
      /* Derive blue from red and green components.
   * Apparently, we must always use integers to perform calculations,
   * otherwise the results won't match D3D's CxV8U8 definition.
   */
      }
      void
   util_format_r8g8bx_snorm_unpack_rgba_float(void *restrict dst_row,
         {
      unsigned x;
   float *dst = dst_row;
   const uint16_t *src = (const uint16_t *)src_row;
   for(x = 0; x < width; x += 1) {
      uint16_t value = util_cpu_to_le16(*src++);
            r = ((int16_t)(value << 8)) >> 8;
            dst[0] = (float)(r * (1.0f/0x7f)); /* r */
   dst[1] = (float)(g * (1.0f/0x7f)); /* g */
   dst[2] = r8g8bx_derive(r, g) * (1.0f/0xff); /* b */
   dst[3] = 1.0f; /* a */
         }
         void
   util_format_r8g8bx_snorm_unpack_rgba_8unorm(uint8_t *restrict dst,
               {
      unsigned x;
   const uint16_t *src = (const uint16_t *)src_row;
   for(x = 0; x < width; x += 1) {
      uint16_t value = util_cpu_to_le16(*src++);
            r = ((int16_t)(value << 8)) >> 8;
            dst[0] = (uint8_t)(((uint16_t)MAX2(r, 0)) * 0xff / 0x7f); /* r */
   dst[1] = (uint8_t)(((uint16_t)MAX2(g, 0)) * 0xff / 0x7f); /* g */
   dst[2] = r8g8bx_derive(r, g); /* b */
   dst[3] = 255; /* a */
         }
         void
   util_format_r8g8bx_snorm_pack_rgba_float(uint8_t *restrict dst_row, unsigned dst_stride,
               {
      unsigned x, y;
   for(y = 0; y < height; y += 1) {
      const float *src = src_row;
   uint16_t *dst = (uint16_t *)dst_row;
   for(x = 0; x < width; x += 1) {
                                          }
   dst_row += dst_stride;
         }
         void
   util_format_r8g8bx_snorm_pack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride,
               {
               for(y = 0; y < height; y += 1) {
      const uint8_t *src = src_row;
   uint16_t *dst = (uint16_t *)dst_row;
   for(x = 0; x < width; x += 1) {
                                          }
   dst_row += dst_stride;
         }
         void
   util_format_r8g8bx_snorm_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src,
         {
      float *dst = in_dst;
   uint16_t value = util_cpu_to_le16(*(const uint16_t *)src);
            r = ((int16_t)(value << 8)) >> 8;
            dst[0] = r * (1.0f/0x7f); /* r */
   dst[1] = g * (1.0f/0x7f); /* g */
   dst[2] = r8g8bx_derive(r, g) * (1.0f/0xff); /* b */
      }
