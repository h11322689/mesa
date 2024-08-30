   /*
   * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      #include "pipe/p_defines.h"
   #include "util/format/u_format.h"
      #include "fd2_util.h"
      static enum a2xx_sq_surfaceformat
   pipe2surface(enum pipe_format format, struct surface_format *fmt)
   {
               if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
      switch (format) {
   /* Compressed textures. */
   case PIPE_FORMAT_ETC1_RGB8:
         case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
         case PIPE_FORMAT_DXT3_RGBA:
         case PIPE_FORMAT_DXT5_RGBA:
         case PIPE_FORMAT_ATC_RGB:
         case PIPE_FORMAT_ATC_RGBA_EXPLICIT:
         case PIPE_FORMAT_ATC_RGBA_INTERPOLATED:
         /* YUV buffers. */
   case PIPE_FORMAT_UYVY:
         case PIPE_FORMAT_YUYV:
         default:
                     uint32_t channel_size = 0;
   for (unsigned i = 0; i < 4; i++)
            unsigned i = util_format_get_first_non_void_channel(format);
   if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED ||
      desc->channel[i].type == UTIL_FORMAT_TYPE_FIXED)
      if (!desc->channel[i].normalized)
         if (desc->channel[i].type == UTIL_FORMAT_TYPE_FIXED)
               /* Note: the 3 channel 24bpp/48bpp/96bpp formats are only for vertex fetch
   * we can use the 4 channel format and ignore the 4th component just isn't
   * used
   * XXX: is it possible for the extra loaded component to cause a MMU fault?
      #define CASE(r, g, b, a) case (r | g << 8 | b << 16 | a << 24)
         /* clang-format off */
   if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
      switch (channel_size) {
   CASE(16,  0,  0,  0): return FMT_16_FLOAT;
   CASE(16, 16,  0,  0): return FMT_16_16_FLOAT;
   CASE(16, 16, 16,  0): return FMT_16_16_16_16_FLOAT; /* Note: only for vertex */
   CASE(16, 16, 16, 16): return FMT_16_16_16_16_FLOAT;
   CASE(32,  0,  0,  0): return FMT_32_FLOAT;
   CASE(32, 32,  0,  0): return FMT_32_32_FLOAT;
   CASE(32, 32, 32,  0): return FMT_32_32_32_FLOAT;
   CASE(32, 32, 32, 32): return FMT_32_32_32_32_FLOAT;
      } else {
      switch (channel_size) {
   CASE( 8,  0,  0,  0): return FMT_8;
   CASE( 8,  8,  0,  0): return FMT_8_8;
   CASE( 8,  8,  8,  0): return FMT_8_8_8_8; /* Note: only for vertex */
   CASE( 8,  8,  8,  8): return FMT_8_8_8_8;
   CASE(16,  0,  0,  0): return FMT_16;
   CASE(16, 16,  0,  0): return FMT_16_16;
   CASE(16, 16, 16,  0): return FMT_16_16_16_16; /* Note: only for vertex */
   CASE(16, 16, 16, 16): return FMT_16_16_16_16;
   CASE(32,  0,  0,  0): return FMT_32;
   CASE(32, 32,  0,  0): return FMT_32_32;
   CASE(32, 32, 32,  0): return FMT_32_32_32_32; /* Note: only for vertex */
   CASE(32, 32, 32, 32): return FMT_32_32_32_32;
   CASE( 4,  4,  4,  4): return FMT_4_4_4_4;
   CASE( 5,  5,  5,  1): return FMT_1_5_5_5;
   CASE( 5,  6,  5,  0): return FMT_5_6_5;
   CASE(10, 10, 10,  2): return FMT_2_10_10_10;
   CASE( 8, 24,  0,  0): return FMT_24_8;
   CASE( 2,  3,  3,  0): return FMT_2_3_3; /* Note: R/B swapped */
      }
      #undef CASE
            }
      struct surface_format
   fd2_pipe2surface(enum pipe_format format)
   {
      struct surface_format fmt = {
      .sign = SQ_TEX_SIGN_UNSIGNED,
   .num_format = SQ_TEX_NUM_FORMAT_FRAC,
      };
   fmt.format = pipe2surface(format, &fmt);
      }
      enum a2xx_colorformatx
   fd2_pipe2color(enum pipe_format format)
   {
      switch (format) {
   /* 8-bit buffers. */
   case PIPE_FORMAT_R8_UNORM:
         case PIPE_FORMAT_B2G3R3_UNORM:
            /* 16-bit buffers. */
   case PIPE_FORMAT_B5G6R5_UNORM:
         case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B5G5R5X1_UNORM:
         case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B4G4R4X4_UNORM:
         case PIPE_FORMAT_R8G8_UNORM:
            /* 32-bit buffers. */
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
         /* Note: snorm untested */
   case PIPE_FORMAT_R8G8B8A8_SNORM:
   case PIPE_FORMAT_R8G8B8X8_SNORM:
            /* float buffers */
   case PIPE_FORMAT_R16_FLOAT:
         case PIPE_FORMAT_R16G16_FLOAT:
         case PIPE_FORMAT_R16G16B16A16_FLOAT:
         case PIPE_FORMAT_R32_FLOAT:
         case PIPE_FORMAT_R32G32_FLOAT:
         case PIPE_FORMAT_R32G32B32A32_FLOAT:
            default:
            }
      static inline enum sq_tex_swiz
   tex_swiz(unsigned swiz)
   {
      switch (swiz) {
   default:
   case PIPE_SWIZZLE_X:
         case PIPE_SWIZZLE_Y:
         case PIPE_SWIZZLE_Z:
         case PIPE_SWIZZLE_W:
         case PIPE_SWIZZLE_0:
         case PIPE_SWIZZLE_1:
            }
      uint32_t
   fd2_tex_swiz(enum pipe_format format, unsigned swizzle_r, unsigned swizzle_g,
         {
      const struct util_format_description *desc = util_format_description(format);
   unsigned char swiz[4] = {
      swizzle_r,
   swizzle_g,
   swizzle_b,
                        return A2XX_SQ_TEX_3_SWIZ_X(tex_swiz(rswiz[0])) |
         A2XX_SQ_TEX_3_SWIZ_Y(tex_swiz(rswiz[1])) |
      }
      uint32_t
   fd2_vtx_swiz(enum pipe_format format, unsigned swizzle)
   {
      const struct util_format_description *desc = util_format_description(format);
            for (unsigned i = 0; i < 4; i++)
                        }
