   /*
   * Copyright (C) 2011 LunarG, Inc.
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
      /**
   * \file texcompress_etc.c
   * GL_OES_compressed_ETC1_RGB8_texture support.
   * Supported ETC2 texture formats are:
   * GL_COMPRESSED_RGB8_ETC2
   * GL_COMPRESSED_SRGB8_ETC2
   * GL_COMPRESSED_RGBA8_ETC2_EAC
   * GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC
   * GL_COMPRESSED_R11_EAC
   * GL_COMPRESSED_RG11_EAC
   * GL_COMPRESSED_SIGNED_R11_EAC
   * GL_COMPRESSED_SIGNED_RG11_EAC
   * MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1
   * MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1
   */
      #include <stdbool.h>
   #include "texcompress.h"
   #include "texcompress_etc.h"
   #include "texstore.h"
   #include "config.h"
   #include "macros.h"
   #include "format_unpack.h"
   #include "util/format_srgb.h"
         struct etc2_block {
      int distance;
   uint64_t pixel_indices[2];
   const int *modifier_tables[2];
   bool flipped;
   bool opaque;
   bool is_ind_mode;
   bool is_diff_mode;
   bool is_t_mode;
   bool is_h_mode;
   bool is_planar_mode;
   uint8_t base_colors[3][3];
   uint8_t paint_colors[4][3];
   uint8_t base_codeword;
   uint8_t multiplier;
      };
      static const int etc2_distance_table[8] = {
            static const int etc2_modifier_tables[16][8] = {
      {  -3,   -6,   -9,  -15,   2,   5,   8,   14},
   {  -3,   -7,  -10,  -13,   2,   6,   9,   12},
   {  -2,   -5,   -8,  -13,   1,   4,   7,   12},
   {  -2,   -4,   -6,  -13,   1,   3,   5,   12},
   {  -3,   -6,   -8,  -12,   2,   5,   7,   11},
   {  -3,   -7,   -9,  -11,   2,   6,   8,   10},
   {  -4,   -7,   -8,  -11,   3,   6,   7,   10},
   {  -3,   -5,   -8,  -11,   2,   4,   7,   10},
   {  -2,   -6,   -8,  -10,   1,   5,   7,    9},
   {  -2,   -5,   -8,  -10,   1,   4,   7,    9},
   {  -2,   -4,   -8,  -10,   1,   3,   7,    9},
   {  -2,   -5,   -7,  -10,   1,   4,   6,    9},
   {  -3,   -4,   -7,  -10,   2,   3,   6,    9},
   {  -1,   -2,   -3,  -10,   0,   1,   2,    9},
   {  -4,   -6,   -8,   -9,   3,   5,   7,    8},
      };
      static const int etc2_modifier_tables_non_opaque[8][4] = {
      { 0,   8,   0,    -8},
   { 0,   17,  0,   -17},
   { 0,   29,  0,   -29},
   { 0,   42,  0,   -42},
   { 0,   60,  0,   -60},
   { 0,   80,  0,   -80},
   { 0,   106, 0,  -106},
      };
      /* define etc1_parse_block and etc. */
   #define UINT8_TYPE GLubyte
   #define TAG(x) x
   #include "util/format/texcompress_etc_tmp.h"
   #undef TAG
   #undef UINT8_TYPE
      GLboolean
   _mesa_texstore_etc1_rgb8(UNUSED_TEXSTORE_PARAMS)
   {
      /* GL_ETC1_RGB8_OES is only valid in glCompressedTexImage2D */
               }
         /**
   * Decode texture data in format `MESA_FORMAT_ETC1_RGB8` to
   * `MESA_FORMAT_ABGR8888`.
   *
   * The size of the source data must be a multiple of the ETC1 block size,
   * which is 8, even if the texture image's dimensions are not aligned to 4.
   * From the GL_OES_compressed_ETC1_RGB8_texture spec:
   *   The texture is described as a number of 4x4 pixel blocks. If the
   *   texture (or a particular mip-level) is smaller than 4 pixels in
   *   any dimension (such as a 2x2 or a 8x1 texture), the texture is
   *   found in the upper left part of the block(s), and the rest of the
   *   pixels are not used. For instance, a texture of size 4x2 will be
   *   placed in the upper half of a 4x4 block, and the lower half of the
   *   pixels in the block will not be accessed.
   *
   * \param src_width in pixels
   * \param src_height in pixels
   * \param dst_stride in bytes
   */
   void
   _mesa_etc1_unpack_rgba8888(uint8_t *dst_row,
                                 {
      etc1_unpack_rgba8888(dst_row, dst_stride,
            }
      static uint8_t
   etc2_base_color1_t_mode(const uint8_t *in, GLuint index)
   {
      uint8_t R1a = 0, x = 0;
   /* base col 1 = extend_4to8bits( (R1a << 2) | R1b, G1, B1) */
   switch(index) {
   case 0:
      R1a = (in[0] >> 3) & 0x3;
   x = ((R1a << 2) | (in[0] & 0x3));
      case 1:
      x = ((in[1] >> 4) & 0xf);
      case 2:
      x = (in[1] & 0xf);
      default:
      /* invalid index */
      }
      }
      static uint8_t
   etc2_base_color2_t_mode(const uint8_t *in, GLuint index)
   {
      uint8_t x = 0;
   /*extend 4to8bits(R2, G2, B2)*/
   switch(index) {
   case 0:
      x = ((in[2] >> 4) & 0xf );
      case 1:
      x = (in[2] & 0xf);
      case 2:
      x = ((in[3] >> 4) & 0xf);
      default:
      /* invalid index */
      }
      }
      static uint8_t
   etc2_base_color1_h_mode(const uint8_t *in, GLuint index)
   {
      uint8_t x = 0;
   /* base col 1 = extend 4to8bits(R1, (G1a << 1) | G1b, (B1a << 3) | B1b) */
   switch(index) {
   case 0:
      x = ((in[0] >> 3) & 0xf);
      case 1:
      x = (((in[0] & 0x7) << 1) | ((in[1] >> 4) & 0x1));
      case 2:
      x = ((in[1] & 0x8) |
            default:
      /* invalid index */
      }
      }
      static uint8_t
   etc2_base_color2_h_mode(const uint8_t *in, GLuint index)
   {
      uint8_t x = 0;
   /* base col 2 = extend 4to8bits(R2, G2, B2) */
   switch(index) {
   case 0:
      x = ((in[2] >> 3) & 0xf );
      case 1:
      x = (((in[2] & 0x7) << 1) | ((in[3] >> 7) & 0x1));
      case 2:
      x = ((in[3] >> 3) & 0xf);
      default:
      /* invalid index */
      }
      }
      static uint8_t
   etc2_base_color_o_planar(const uint8_t *in, GLuint index)
   {
      GLuint tmp;
   switch(index) {
   case 0:
      tmp = ((in[0] >> 1) & 0x3f); /* RO */
      case 1:
      tmp = (((in[0] & 0x1) << 6) | /* GO1 */
            case 2:
      tmp = (((in[1] & 0x1) << 5) | /* BO1 */
         (in[2] & 0x18) | /* BO2 */
      default:
      /* invalid index */
         }
      static uint8_t
   etc2_base_color_h_planar(const uint8_t *in, GLuint index)
   {
      GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[3] & 0x7c) >> 1) | /* RH1 */
            case 1:
      tmp = (in[4] >> 1) & 0x7f; /* GH */
      case 2:
      tmp = (((in[4] & 0x1) << 5) |
            default:
      /* invalid index */
         }
      static uint8_t
   etc2_base_color_v_planar(const uint8_t *in, GLuint index)
   {
      GLuint tmp;
   switch(index) {
   case 0:
      tmp = (((in[5] & 0x7) << 0x3) |
            case 1:
      tmp = (((in[6] & 0x1f) << 2) |
            case 2:
      tmp = in[7] & 0x3f; /* BV */
      default:
      /* invalid index */
         }
      static GLint
   etc2_get_pixel_index(const struct etc2_block *block, int x, int y)
   {
      int bit = ((3 - y) + (3 - x) * 4) * 3;
   int idx = (block->pixel_indices[1] >> bit) & 0x7;
      }
      static uint8_t
   etc2_clamp(int color)
   {
      /* CLAMP(color, 0, 255) */
      }
      static GLushort
   etc2_clamp2(int color)
   {
      /* CLAMP(color, 0, 2047) */
      }
      static GLshort
   etc2_clamp3(int color)
   {
      /* CLAMP(color, -1023, 1023) */
      }
      static void
   etc2_rgb8_parse_block(struct etc2_block *block,
               {
      unsigned i;
   GLboolean diffbit = false;
            const int R_plus_dR = (src[0] >> 3) + lookup[src[0] & 0x7];
   const int G_plus_dG = (src[1] >> 3) + lookup[src[1] & 0x7];
            /* Reset the mode flags */
   block->is_ind_mode = false;
   block->is_diff_mode = false;
   block->is_t_mode = false;
   block->is_h_mode = false;
            if (punchthrough_alpha)
         else
            if (!diffbit && !punchthrough_alpha) {
      /* individual mode */
            for (i = 0; i < 3; i++) {
      /* Texture decode algorithm is same for individual mode in etc1
   * & etc2.
   */
   block->base_colors[0][i] = etc1_base_color_ind_hi(src[i]);
         }
   else if (R_plus_dR < 0 || R_plus_dR > 31){
      /* T mode */
            for(i = 0; i < 3; i++) {
      block->base_colors[0][i] = etc2_base_color1_t_mode(src, i);
      }
   /* pick distance */
   block->distance =
                  for (i = 0; i < 3; i++) {
      block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i]);
   block->paint_colors[1][i] = etc2_clamp(block->base_colors[1][i] +
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i]);
   block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
         }
   else if (G_plus_dG < 0 || G_plus_dG > 31){
               /* H mode */
            for(i = 0; i < 3; i++) {
      block->base_colors[0][i] = etc2_base_color1_h_mode(src, i);
               base_color_1_value = (block->base_colors[0][0] << 16) +
               base_color_2_value = (block->base_colors[1][0] << 16) +
               /* pick distance */
   block->distance =
      etc2_distance_table[(src[3] & 0x4) |
               for (i = 0; i < 3; i++) {
      block->paint_colors[0][i] = etc2_clamp(block->base_colors[0][i] +
         block->paint_colors[1][i] = etc2_clamp(block->base_colors[0][i] -
         block->paint_colors[2][i] = etc2_clamp(block->base_colors[1][i] +
         block->paint_colors[3][i] = etc2_clamp(block->base_colors[1][i] -
         }
   else if (B_plus_dB < 0 || B_plus_dB > 31) {
      /* Planar mode */
            /* opaque bit must be set in planar mode */
            for (i = 0; i < 3; i++) {
      block->base_colors[0][i] = etc2_base_color_o_planar(src, i);
   block->base_colors[1][i] = etc2_base_color_h_planar(src, i);
         }
   else if (diffbit || punchthrough_alpha) {
      /* differential mode */
            for (i = 0; i < 3; i++) {
      /* Texture decode algorithm is same for differential mode in etc1
   * & etc2.
   */
   block->base_colors[0][i] = etc1_base_color_diff_hi(src[i]);
                  if (block->is_ind_mode || block->is_diff_mode) {
      int table1_idx = (src[3] >> 5) & 0x7;
            /* Use same modifier tables as for etc1 textures if opaque bit is set
   * or if non punchthrough texture format
   */
   block->modifier_tables[0] = (!punchthrough_alpha || block->opaque) ?
               block->modifier_tables[1] = (!punchthrough_alpha || block->opaque) ?
                              block->pixel_indices[0] =
      }
      static void
   etc2_rgb8_fetch_texel(const struct etc2_block *block,
               {
      const uint8_t *base_color;
            /* get pixel index */
   bit = y + x * 4;
   idx = ((block->pixel_indices[0] >> (15 + bit)) & 0x2) |
            if (block->is_ind_mode || block->is_diff_mode) {
      /* check for punchthrough_alpha format */
   if (punchthrough_alpha) {
      if (!block->opaque && idx == 2) {
      dst[0] = dst[1] = dst[2] = dst[3] = 0;
      }
   else
               /* Use pixel index and subblock to get the modifier */
   blk = (block->flipped) ? (y >= 2) : (x >= 2);
   base_color = block->base_colors[blk];
            dst[0] = etc2_clamp(base_color[0] + modifier);
   dst[1] = etc2_clamp(base_color[1] + modifier);
      }
   else if (block->is_t_mode || block->is_h_mode) {
      /* check for punchthrough_alpha format */
   if (punchthrough_alpha) {
      if (!block->opaque && idx == 2) {
      dst[0] = dst[1] = dst[2] = dst[3] = 0;
      }
   else
               /* Use pixel index to pick one of the paint colors */
   dst[0] = block->paint_colors[idx][0];
   dst[1] = block->paint_colors[idx][1];
      }
   else if (block->is_planar_mode) {
      /* {R(x, y) = clamp255((x × (RH − RO) + y × (RV − RO) + 4 × RO + 2) >> 2)
   * {G(x, y) = clamp255((x × (GH − GO) + y × (GV − GO) + 4 × GO + 2) >> 2)
   * {B(x, y) = clamp255((x × (BH − BO) + y × (BV − BO) + 4 × BO + 2) >> 2)
   */
   int red, green, blue;
   red = (x * (block->base_colors[1][0] - block->base_colors[0][0]) +
                  green = (x * (block->base_colors[1][1] - block->base_colors[0][1]) +
                  blue = (x * (block->base_colors[1][2] - block->base_colors[0][2]) +
                  dst[0] = etc2_clamp(red);
   dst[1] = etc2_clamp(green);
            /* check for punchthrough_alpha format */
   if (punchthrough_alpha)
      }
   else
      }
      static void
   etc2_alpha8_fetch_texel(const struct etc2_block *block,
         {
      int modifier, alpha, idx;
   /* get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
   modifier = etc2_modifier_tables[block->table_index][idx];
   alpha = block->base_codeword + modifier * block->multiplier;
      }
      static void
   etc2_r11_fetch_texel(const struct etc2_block *block,
         {
      GLint modifier, idx;
   GLshort color;
   /* Get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
            if (block->multiplier != 0)
      /* clamp2(base codeword × 8 + 4 + modifier × multiplier × 8) */
   color = etc2_clamp2(((block->base_codeword << 3) | 0x4)  +
      else
            /* Extend 11 bits color value to 16 bits. OpenGL ES 3.0 specification
   * allows extending the color value to any number of bits. But, an
   * implementation is not allowed to truncate the 11-bit value to less than
   * 11 bits."
   */
   color = (color << 5) | (color >> 6);
      }
      static void
   etc2_signed_r11_fetch_texel(const struct etc2_block *block,
         {
      GLint modifier, idx;
   GLshort color;
            if (base_codeword == -128)
            /* Get pixel index */
   idx = etc2_get_pixel_index(block, x, y);
            if (block->multiplier != 0)
      /* clamp3(base codeword × 8 + modifier × multiplier × 8) */
   color = etc2_clamp3((base_codeword << 3)  +
      else
            /* Extend 11 bits color value to 16 bits. OpenGL ES 3.0 specification
   * allows extending the color value to any number of bits. But, an
   * implementation is not allowed to truncate the 11-bit value to less than
   * 11 bits. A negative 11-bit value must first be made positive before bit
   * replication, and then made negative again
   */
   if (color >= 0)
         else {
      color = -color;
   color = (color << 5) | (color >> 5);
      }
      }
      static void
   etc2_alpha8_parse_block(struct etc2_block *block, const uint8_t *src)
   {
      block->base_codeword = src[0];
   block->multiplier = (src[1] >> 4) & 0xf;
   block->table_index = src[1] & 0xf;
   block->pixel_indices[1] = (((uint64_t)src[2] << 40) |
                              }
      static void
   etc2_r11_parse_block(struct etc2_block *block, const uint8_t *src)
   {
      /* Parsing logic remains same as for etc2_alpha8_parse_block */
      }
      static void
   etc2_rgba8_parse_block(struct etc2_block *block, const uint8_t *src)
   {
      /* RGB component is parsed the same way as for MESA_FORMAT_ETC2_RGB8 */
   etc2_rgb8_parse_block(block, src + 8,
         /* Parse Alpha component */
      }
      static void
   etc2_rgba8_fetch_texel(const struct etc2_block *block,
         {
      etc2_rgb8_fetch_texel(block, x, y, dst,
            }
      static void
   etc2_unpack_rgb8(uint8_t *dst_row,
                  unsigned dst_stride,
   const uint8_t *src_row,
      {
      const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;
   /*
   * Destination texture may not be a multiple of four texels in
   * height. Compute a safe height to avoid writing outside the texture.
   */
            for (x = 0; x < width; x+= bw) {
      /*
   * Destination texture may not be a multiple of four texels in
   * width. Compute a safe width to avoid writing outside the texture.
                                 for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
   for (i = 0; i < w; i++) {
      etc2_rgb8_fetch_texel(&block, i, j, dst,
         dst[3] = 255;
                                    }
      static void
   etc2_unpack_srgb8(uint8_t *dst_row,
                     unsigned dst_stride,
      unsigned height,
      {
      const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc2_block block;
   unsigned x, y, i, j;
            for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;
            for (x = 0; x < width; x+= bw) {
      const unsigned w = MIN2(bw, width - x);
                     for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
   for (i = 0; i < w; i++) {
               /* Convert to MESA_FORMAT_B8G8R8A8_SRGB */
   tmp = dst[0];
   dst[0] = dst[2];
   dst[2] = tmp;
      }
                           }
                     }
      static void
   etc2_unpack_rgba8(uint8_t *dst_row,
                     unsigned dst_stride,
   const uint8_t *src_row,
   {
      /* If internalformat is COMPRESSED_RGBA8_ETC2_EAC, each 4 × 4 block of
   * RGBA8888 information is compressed to 128 bits. To decode a block, the
   * two 64-bit integers int64bitAlpha and int64bitColor are calculated.
   */
   const unsigned bw = 4, bh = 4, bs = 16, comps = 4;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const uint8_t *src = src_row;
            for (x = 0; x < width; x+= bw) {
                     for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
   for (i = 0; i < w; i++) {
      etc2_rgba8_fetch_texel(&block, i, j, dst);
         }
                     }
      static void
   etc2_unpack_srgb8_alpha8(uint8_t *dst_row,
                              unsigned height,
      {
      /* If internalformat is COMPRESSED_SRGB8_ALPHA8_ETC2_EAC, each 4 × 4 block
   * of RGBA8888 information is compressed to 128 bits. To decode a block, the
   * two 64-bit integers int64bitAlpha and int64bitColor are calculated.
   */
   const unsigned bw = 4, bh = 4, bs = 16, comps = 4;
   struct etc2_block block;
   unsigned x, y, i, j;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
                     for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
               /* Convert to MESA_FORMAT_B8G8R8A8_SRGB */
   tmp = dst[0];
   dst[0] = dst[2];
   dst[2] = tmp;
   dst[3] = dst[3];
                              }
                     }
      static void
   etc2_unpack_r11(uint8_t *dst_row,
                  unsigned dst_stride,
   const uint8_t *src_row,
      {
      /* If internalformat is COMPRESSED_R11_EAC, each 4 × 4 block of
         */
   const unsigned bw = 4, bh = 4, bs = 8, comps = 1, comp_size = 2;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
                     for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps * comp_size;
   for (i = 0; i < w; i++) {
      etc2_r11_fetch_texel(&block, i, j, dst);
         }
                     }
      static void
   etc2_unpack_rg11(uint8_t *dst_row,
                  unsigned dst_stride,
   const uint8_t *src_row,
      {
      /* If internalformat is COMPRESSED_RG11_EAC, each 4 × 4 block of
         */
   const unsigned bw = 4, bh = 4, bs = 16, comps = 2, comp_size = 2;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
      const unsigned w = MIN2(bw, width - x);
                  for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride +
         for (i = 0; i < w; i++) {
      etc2_r11_fetch_texel(&block, i, j, dst);
         }
                  for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride +
         for (i = 0; i < w; i++) {
      etc2_r11_fetch_texel(&block, i, j, dst + comp_size);
         }
                     }
      static void
   etc2_unpack_signed_r11(uint8_t *dst_row,
                        unsigned dst_stride,
      {
      /* If internalformat is COMPRESSED_SIGNED_R11_EAC, each 4 × 4 block of
         */
   const unsigned bw = 4, bh = 4, bs = 8, comps = 1, comp_size = 2;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
                     for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride +
         for (i = 0; i < w; i++) {
      etc2_signed_r11_fetch_texel(&block, i, j, dst);
         }
                     }
      static void
   etc2_unpack_signed_rg11(uint8_t *dst_row,
                           unsigned dst_stride,
   {
      /* If internalformat is COMPRESSED_SIGNED_RG11_EAC, each 4 × 4 block of
         */
   const unsigned bw = 4, bh = 4, bs = 16, comps = 2, comp_size = 2;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
      const unsigned w = MIN2(bw, width - x);
                  for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride +
         for (i = 0; i < w; i++) {
      etc2_signed_r11_fetch_texel(&block, i, j, dst);
         }
                  for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride +
         for (i = 0; i < w; i++) {
      etc2_signed_r11_fetch_texel(&block, i, j, dst + comp_size);
         }
                     }
      static void
   etc2_unpack_rgb8_punchthrough_alpha1(uint8_t *dst_row,
                                 {
      const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc2_block block;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
      const unsigned w = MIN2(bw, width - x);
   etc2_rgb8_parse_block(&block, src,
         for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
   for (i = 0; i < w; i++) {
      etc2_rgb8_fetch_texel(&block, i, j, dst,
                                          }
      static void
   etc2_unpack_srgb8_punchthrough_alpha1(uint8_t *dst_row,
                                       {
      const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc2_block block;
   unsigned x, y, i, j;
            for (y = 0; y < height; y += bh) {
      const unsigned h = MIN2(bh, height - y);
            for (x = 0; x < width; x+= bw) {
      const unsigned w = MIN2(bw, width - x);
   etc2_rgb8_parse_block(&block, src,
         for (j = 0; j < h; j++) {
      uint8_t *dst = dst_row + (y + j) * dst_stride + x * comps;
   for (i = 0; i < w; i++) {
               /* Convert to MESA_FORMAT_B8G8R8A8_SRGB */
   tmp = dst[0];
   dst[0] = dst[2];
   dst[2] = tmp;
   dst[3] = dst[3];
                                                         }
      /* ETC2 texture formats are valid in glCompressedTexImage2D and
   * glCompressedTexSubImage2D functions */
   GLboolean
   _mesa_texstore_etc2_rgb8(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_srgb8(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_rgba8_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_srgb8_alpha8_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_r11_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_signed_r11_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_rg11_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_signed_rg11_eac(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_rgb8_punchthrough_alpha1(UNUSED_TEXSTORE_PARAMS)
   {
                  }
      GLboolean
   _mesa_texstore_etc2_srgb8_punchthrough_alpha1(UNUSED_TEXSTORE_PARAMS)
   {
                  }
         /**
   * Decode texture data in any one of following formats:
   * `MESA_FORMAT_ETC2_RGB8`
   * `MESA_FORMAT_ETC2_SRGB8`
   * `MESA_FORMAT_ETC2_RGBA8_EAC`
   * `MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC`
   * `MESA_FORMAT_ETC2_R11_EAC`
   * `MESA_FORMAT_ETC2_RG11_EAC`
   * `MESA_FORMAT_ETC2_SIGNED_R11_EAC`
   * `MESA_FORMAT_ETC2_SIGNED_RG11_EAC`
   * `MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1`
   * `MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1`
   *
   * The size of the source data must be a multiple of the ETC2 block size
   * even if the texture image's dimensions are not aligned to 4.
   *
   * \param src_width in pixels
   * \param src_height in pixels
   * \param dst_stride in bytes
   */
      void
   _mesa_unpack_etc2_format(uint8_t *dst_row,
                           unsigned dst_stride,
      mesa_format format,
      {
      if (format == MESA_FORMAT_ETC2_RGB8)
      etc2_unpack_rgb8(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_SRGB8)
      etc2_unpack_srgb8(dst_row, dst_stride,
      src_width, src_height, bgra);
   else if (format == MESA_FORMAT_ETC2_RGBA8_EAC)
      etc2_unpack_rgba8(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC)
      etc2_unpack_srgb8_alpha8(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_R11_EAC)
      etc2_unpack_r11(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_RG11_EAC)
      etc2_unpack_rg11(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_SIGNED_R11_EAC)
      etc2_unpack_signed_r11(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_SIGNED_RG11_EAC)
      etc2_unpack_signed_rg11(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1)
      etc2_unpack_rgb8_punchthrough_alpha1(dst_row, dst_stride,
            else if (format == MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1)
      etc2_unpack_srgb8_punchthrough_alpha1(dst_row, dst_stride,
         }
            static void
   fetch_etc1_rgb8(const GLubyte *map,
               {
      struct etc1_block block;
   GLubyte dst[3];
                     etc1_parse_block(&block, src);
            texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
      }
         static void
   fetch_etc2_rgb8(const GLubyte *map,
         {
      struct etc2_block block;
   uint8_t dst[3];
                     etc2_rgb8_parse_block(&block, src,
         etc2_rgb8_fetch_texel(&block, i % 4, j % 4, dst,
            texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
      }
      static void
   fetch_etc2_srgb8(const GLubyte *map,
         {
      struct etc2_block block;
   uint8_t dst[3];
                     etc2_rgb8_parse_block(&block, src,
         etc2_rgb8_fetch_texel(&block, i % 4, j % 4, dst,
            texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(dst[0]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(dst[1]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(dst[2]);
      }
      static void
   fetch_etc2_rgba8_eac(const GLubyte *map,
         {
      struct etc2_block block;
   uint8_t dst[4];
                     etc2_rgba8_parse_block(&block, src);
            texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
      }
      static void
   fetch_etc2_srgb8_alpha8_eac(const GLubyte *map,
         {
      struct etc2_block block;
   uint8_t dst[4];
                     etc2_rgba8_parse_block(&block, src);
            texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(dst[0]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(dst[1]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(dst[2]);
      }
      static void
   fetch_etc2_r11_eac(const GLubyte *map,
         {
      struct etc2_block block;
   GLushort dst;
                     etc2_r11_parse_block(&block, src);
            texel[RCOMP] = USHORT_TO_FLOAT(dst);
   texel[GCOMP] = 0.0f;
   texel[BCOMP] = 0.0f;
      }
      static void
   fetch_etc2_rg11_eac(const GLubyte *map,
         {
      struct etc2_block block;
   GLushort dst[2];
                     /* red component */
   etc2_r11_parse_block(&block, src);
            /* green component */
   etc2_r11_parse_block(&block, src + 8);
            texel[RCOMP] = USHORT_TO_FLOAT(dst[0]);
   texel[GCOMP] = USHORT_TO_FLOAT(dst[1]);
   texel[BCOMP] = 0.0f;
      }
      static void
   fetch_etc2_signed_r11_eac(const GLubyte *map,
         {
      struct etc2_block block;
   GLushort dst;
                     etc2_r11_parse_block(&block, src);
            texel[RCOMP] = SHORT_TO_FLOAT(dst);
   texel[GCOMP] = 0.0f;
   texel[BCOMP] = 0.0f;
      }
      static void
   fetch_etc2_signed_rg11_eac(const GLubyte *map,
         {
      struct etc2_block block;
   GLushort dst[2];
                     /* red component */
   etc2_r11_parse_block(&block, src);
            /* green component */
   etc2_r11_parse_block(&block, src + 8);
            texel[RCOMP] = SHORT_TO_FLOAT(dst[0]);
   texel[GCOMP] = SHORT_TO_FLOAT(dst[1]);
   texel[BCOMP] = 0.0f;
      }
      static void
   fetch_etc2_rgb8_punchthrough_alpha1(const GLubyte *map,
               {
      struct etc2_block block;
   uint8_t dst[4];
                     etc2_rgb8_parse_block(&block, src,
         etc2_rgb8_fetch_texel(&block, i % 4, j % 4, dst,
         texel[RCOMP] = UBYTE_TO_FLOAT(dst[0]);
   texel[GCOMP] = UBYTE_TO_FLOAT(dst[1]);
   texel[BCOMP] = UBYTE_TO_FLOAT(dst[2]);
      }
      static void
   fetch_etc2_srgb8_punchthrough_alpha1(const GLubyte *map,
               {
      struct etc2_block block;
   uint8_t dst[4];
                     etc2_rgb8_parse_block(&block, src,
         etc2_rgb8_fetch_texel(&block, i % 4, j % 4, dst,
         texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(dst[0]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(dst[1]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(dst[2]);
      }
         compressed_fetch_func
   _mesa_get_etc_fetch_func(mesa_format format)
   {
      switch (format) {
   case MESA_FORMAT_ETC1_RGB8:
         case MESA_FORMAT_ETC2_RGB8:
         case MESA_FORMAT_ETC2_SRGB8:
         case MESA_FORMAT_ETC2_RGBA8_EAC:
         case MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC:
         case MESA_FORMAT_ETC2_R11_EAC:
         case MESA_FORMAT_ETC2_RG11_EAC:
         case MESA_FORMAT_ETC2_SIGNED_R11_EAC:
         case MESA_FORMAT_ETC2_SIGNED_RG11_EAC:
         case MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1:
         case MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1:
         default:
            }
