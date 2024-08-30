   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2012 Intel Corporation
   * Copyright 2013 Google
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Chad Versace <chad.versace@linux.intel.com>
   *    Frank Henigman <fjhenigman@google.com>
   */
      #include <string.h>
      #include "util/macros.h"
   #include "util/u_math.h"
   #include "util/rounding.h"
   #include "isl_priv.h"
      #if defined(__SSSE3__)
   #include <tmmintrin.h>
   #elif defined(__SSE2__)
   #include <emmintrin.h>
   #endif
      #define FILE_DEBUG_FLAG DEBUG_TEXTURE
      #define ALIGN_DOWN(a, b) ROUND_DOWN_TO(a, b)
   #define ALIGN_UP(a, b) ALIGN(a, b)
      /* Tile dimensions.  Width and span are in bytes, height is in pixels (i.e.
   * unitless).  A "span" is the most number of bytes we can copy from linear
   * to tiled without needing to calculate a new destination address.
   */
   static const uint32_t xtile_width = 512;
   static const uint32_t xtile_height = 8;
   static const uint32_t xtile_span = 64;
   static const uint32_t ytile_width = 128;
   static const uint32_t ytile_height = 32;
   static const uint32_t ytile_span = 16;
      static inline uint32_t
   ror(uint32_t n, uint32_t d)
   {
         }
      // bswap32 already exists as a macro on some platforms (FreeBSD)
   #ifndef bswap32
   static inline uint32_t
   bswap32(uint32_t n)
   {
   #if defined(HAVE___BUILTIN_BSWAP32)
         #else
      return (n >> 24) |
         ((n >> 8) & 0x0000ff00) |
      #endif
   }
   #endif
      /**
   * Copy RGBA to BGRA - swap R and B.
   */
   static inline void *
   rgba8_copy(void *dst, const void *src, size_t bytes)
   {
      uint32_t *d = dst;
                     while (bytes >= 4) {
      *d = ror(bswap32(*s), 8);
   d += 1;
   s += 1;
      }
      }
      #ifdef __SSSE3__
   static const uint8_t rgba8_permutation[16] =
            static inline void
   rgba8_copy_16_aligned_dst(void *dst, const void *src)
   {
      _mm_store_si128(dst,
            }
      static inline void
   rgba8_copy_16_aligned_src(void *dst, const void *src)
   {
      _mm_storeu_si128(dst,
            }
      #elif defined(__SSE2__)
   static inline void
   rgba8_copy_16_aligned_dst(void *dst, const void *src)
   {
               agmask = _mm_set1_epi32(0xFF00FF00);
            rb = _mm_andnot_si128(agmask, srcreg);
   ag = _mm_and_si128(agmask, srcreg);
   br = _mm_shufflehi_epi16(_mm_shufflelo_epi16(rb, _MM_SHUFFLE(2, 3, 0, 1)),
                     }
      static inline void
   rgba8_copy_16_aligned_src(void *dst, const void *src)
   {
               agmask = _mm_set1_epi32(0xFF00FF00);
            rb = _mm_andnot_si128(agmask, srcreg);
   ag = _mm_and_si128(agmask, srcreg);
   br = _mm_shufflehi_epi16(_mm_shufflelo_epi16(rb, _MM_SHUFFLE(2, 3, 0, 1)),
                     }
   #endif
      /**
   * Copy RGBA to BGRA - swap R and B, with the destination 16-byte aligned.
   */
   static inline void *
   rgba8_copy_aligned_dst(void *dst, const void *src, size_t bytes)
   {
            #if defined(__SSSE3__) || defined(__SSE2__)
      if (bytes == 64) {
      rgba8_copy_16_aligned_dst(dst +  0, src +  0);
   rgba8_copy_16_aligned_dst(dst + 16, src + 16);
   rgba8_copy_16_aligned_dst(dst + 32, src + 32);
   rgba8_copy_16_aligned_dst(dst + 48, src + 48);
               while (bytes >= 16) {
      rgba8_copy_16_aligned_dst(dst, src);
   src += 16;
   dst += 16;
         #endif
                     }
      /**
   * Copy RGBA to BGRA - swap R and B, with the source 16-byte aligned.
   */
   static inline void *
   rgba8_copy_aligned_src(void *dst, const void *src, size_t bytes)
   {
            #if defined(__SSSE3__) || defined(__SSE2__)
      if (bytes == 64) {
      rgba8_copy_16_aligned_src(dst +  0, src +  0);
   rgba8_copy_16_aligned_src(dst + 16, src + 16);
   rgba8_copy_16_aligned_src(dst + 32, src + 32);
   rgba8_copy_16_aligned_src(dst + 48, src + 48);
               while (bytes >= 16) {
      rgba8_copy_16_aligned_src(dst, src);
   src += 16;
   dst += 16;
         #endif
                     }
      /**
   * Each row from y0 to y1 is copied in three parts: [x0,x1), [x1,x2), [x2,x3).
   * These ranges are in bytes, i.e. pixels * bytes-per-pixel.
   * The first and last ranges must be shorter than a "span" (the longest linear
   * stretch within a tile) and the middle must equal a whole number of spans.
   * Ranges may be empty.  The region copied must land entirely within one tile.
   * 'dst' is the start of the tile and 'src' is the corresponding
   * address to copy from, though copying begins at (x0, y0).
   * To enable swizzling 'swizzle_bit' must be 1<<6, otherwise zero.
   * Swizzling flips bit 6 in the copy destination offset, when certain other
   * bits are set in it.
   */
   typedef void (*tile_copy_fn)(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                                    /**
   * Copy texture data from linear to X tile layout.
   *
   * \copydoc tile_copy_fn
   *
   * The mem_copy parameters allow the user to specify an alternative mem_copy
   * function that, for instance, may do RGBA -> BGRA swizzling.  The first
   * function must handle any memory alignment while the second function must
   * only handle 16-byte alignment in whichever side (source or destination) is
   * tiled.
   */
   static inline void
   linear_to_xtiled(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y1,
   char *dst, const char *src,
   int32_t src_pitch,
      {
      /* The copy destination offset for each range copied is the sum of
   * an X offset 'x0' or 'xo' and a Y offset 'yo.'
   */
                     for (yo = y0 * xtile_width; yo < y1 * xtile_width; yo += xtile_width) {
      /* Bits 9 and 10 of the copy destination offset control swizzling.
   * Only 'yo' contributes to those bits in the total offset,
   * so calculate 'swizzle' just once per row.
   * Move bits 9 and 10 three and four places respectively down
   * to bit 6 and xor them.
   */
                     for (xo = x1; xo < x2; xo += xtile_span) {
                                 }
      /**
   * Copy texture data from linear to Y tile layout.
   *
   * \copydoc tile_copy_fn
   */
   static inline void
   linear_to_ytiled(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y3,
   char *dst, const char *src,
   int32_t src_pitch,
      {
      /* Y tiles consist of columns that are 'ytile_span' wide (and the same height
   * as the tile).  Thus the destination offset for (x,y) is the sum of:
   *   (x % column_width)                    // position within column
   *   (x / column_width) * bytes_per_column // column number * bytes per column
   *   y * column_width
   *
   * The copy destination offset for each range copied is the sum of
   * an X offset 'xo0' or 'xo' and a Y offset 'yo.'
   */
   const uint32_t column_width = ytile_span;
            uint32_t y1 = MIN2(y3, ALIGN_UP(y0, 4));
            uint32_t xo0 = (x0 % ytile_span) + (x0 / ytile_span) * bytes_per_column;
            /* Bit 9 of the destination offset control swizzling.
   * Only the X offset contributes to bit 9 of the total offset,
   * so swizzle can be calculated in advance for these X positions.
   * Move bit 9 three places down to bit 6.
   */
   uint32_t swizzle0 = (xo0 >> 3) & swizzle_bit;
                              if (y0 != y1) {
      for (yo = y0 * column_width; yo < y1 * column_width; yo += column_width) {
                              /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + ((xo + yo) ^ swizzle), src + x, ytile_span);
   xo += bytes_per_column;
                                       for (yo = y1 * column_width; yo < y2 * column_width; yo += 4 * column_width) {
      uint32_t xo = xo1;
            if (x0 != x1) {
      mem_copy(dst + ((xo0 + yo + 0 * column_width) ^ swizzle0), src + x0 + 0 * src_pitch, x1 - x0);
   mem_copy(dst + ((xo0 + yo + 1 * column_width) ^ swizzle0), src + x0 + 1 * src_pitch, x1 - x0);
   mem_copy(dst + ((xo0 + yo + 2 * column_width) ^ swizzle0), src + x0 + 2 * src_pitch, x1 - x0);
               /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + ((xo + yo + 0 * column_width) ^ swizzle), src + x + 0 * src_pitch, ytile_span);
   mem_copy_align16(dst + ((xo + yo + 1 * column_width) ^ swizzle), src + x + 1 * src_pitch, ytile_span);
   mem_copy_align16(dst + ((xo + yo + 2 * column_width) ^ swizzle), src + x + 2 * src_pitch, ytile_span);
   mem_copy_align16(dst + ((xo + yo + 3 * column_width) ^ swizzle), src + x + 3 * src_pitch, ytile_span);
   xo += bytes_per_column;
               if (x2 != x3) {
      mem_copy_align16(dst + ((xo + yo + 0 * column_width) ^ swizzle), src + x2 + 0 * src_pitch, x3 - x2);
   mem_copy_align16(dst + ((xo + yo + 1 * column_width) ^ swizzle), src + x2 + 1 * src_pitch, x3 - x2);
   mem_copy_align16(dst + ((xo + yo + 2 * column_width) ^ swizzle), src + x2 + 2 * src_pitch, x3 - x2);
                           if (y2 != y3) {
      for (yo = y2 * column_width; yo < y3 * column_width; yo += column_width) {
                              /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + ((xo + yo) ^ swizzle), src + x, ytile_span);
   xo += bytes_per_column;
                                 }
      /**
   * Copy texture data from linear to Tile-4 layout.
   *
   * \copydoc tile_copy_fn
   */
   static inline void
   linear_to_tile4(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y3,
   char *dst, const char *src,
   int32_t src_pitch,
      {
      /* Tile 4 consist of columns that are 'ytile_span' wide and each 64B tile
   * block consists of 4 row of Y-tile ordered data.
   * Each 512B block within a 4kB tile contains 8 such block.
   *
   * To calculate the tiled  offset, we need to identify:
   * Block X and Block Y offset at each 512B block boundary in X and Y
   * direction.
   *
   * A Tile4 has the following layout :
   *
   *                |<------------- 128 B-------------------|
   *                _________________________________________
   * 512B blk(Blk0)^|  0 |  1 |  2 |  3 |  8 |  9 | 10 | 11 | ^ 512B blk(Blk1)
   * (cell 0..7))  v|  4 |  5 |  6 |  7 | 12 | 13 | 14 | 15 | v (cell 8..15))
   *                -----------------------------------------
   *                | 16 | 17 | 18 | 19 | 24 | 25 | 26 | 27 |
   *                | 20 | 21 | 22 | 23 | 28 | 29 | 30 | 31 |
   *                -----------------------------------------
   *                | 32 | 33 | 34 | 35 | 40 | 41 | 42 | 43 |
   *                | 36 | 37 | 38 | 39 | 44 | 45 | 46 | 47 |
   *                -----------------------------------------
   *                | 48 | 49 | 50 | 51 | 56 | 57 | 58 | 59 |
   *                | 52 | 53 | 54 | 55 | 60 | 61 | 62 | 63 |
   *                -----------------------------------------
   *
   * The tile is divided in 512B blocks[Blk0..Blk7], themselves made of 2
   * rows of 256B sub-blocks.
   *
   * Each sub-block is composed of 4 64B elements[cell(0)-cell(3)] (a cell
   * in the figure above).
   *
   * Each 64B cell represents 4 rows of data.[cell(0), cell(1), .., cell(63)]
   *
   *
   *   Block X - Adds 256B to offset when we encounter block boundary in
   *             X direction.(Ex: Blk 0 --> Blk 1(BlkX_off = 256))
   *   Block Y - Adds 512B to offset when we encounter block boundary in
   *             Y direction.(Ex: Blk 0 --> Blk 3(BlkY_off = 512))
   *
   *   (x / ytile_span) * cacheline_size_B //Byte offset in the X dir of
   *                                         the containing 64B block
   *   x % ytile_span //Byte offset in X dir within a 64B block/cacheline
   *
   *   (y % 4) * 16 // Byte offset of the Y dir within a 64B block/cacheline
   *   (y / 4) * 256// Byte offset of the Y dir within 512B block after 1 row
   *                   of 64B blocks/cachelines
   *
   * The copy destination offset for each range copied is the sum of
   * Block X offset 'BlkX_off', Block Y offset 'BlkY_off', X offset 'xo'
   * and a Y offset 'yo.'
   */
   const uint32_t column_width = ytile_span;
            assert(ytile_span * tile4_blkh == 64);
            /* Find intermediate Y offsets that are aligned to a 64B element
   * (4 rows), so that we can do fully 64B memcpys on those.
   */
   uint32_t y1 = MIN2(y3, ALIGN_UP(y0, 4));
            /* xsb0 and xsb1 are the byte offset within a 256B sub block for x0 and x1 */
   uint32_t xsb0 = (x0 % ytile_span) + (x0 / ytile_span) * cacheline_size_B;
            uint32_t Blkxsb0_off = ALIGN_DOWN(xsb0, 256);
                              /* Y0 determines the initial byte offset in the Y direction */
            /* Y2 determines the byte offset required for reaching y2 if y2 doesn't map
   * exactly to 512B block boundary
   */
                     /* To maximize memcpy speed, we do the copy in 3 parts :
   *   - copy the first lines that are not aligned to the 64B cell's height (4 rows)
   *   - copy the lines that are aligned to 64B cell's height
   *   - copy the remaining lines not making up for a full 64B cell's height
   */
   if (y0 != y1) {
      for (yo = Y0; yo < Y0 + (y1 - y0) * column_width; yo += column_width) {
                                                mem_copy_align16(dst + (Blky0_off + BlkX_off) + (xo + yo), src + x, ytile_span);
               if (x3 != x2) {
      BlkX_off = ALIGN_DOWN(xo, 256);
                              for (yo = y1 * 4 * column_width; yo < y2 * 4 * column_width; yo += 16 * column_width) {
      uint32_t xo = xsb1;
            if (x0 != x1) {
      mem_copy(dst + (BlkY_off + Blkxsb0_off) + (xsb0 + yo + 0 * column_width),
         mem_copy(dst + (BlkY_off + Blkxsb0_off) + (xsb0 + yo + 1 * column_width),
         mem_copy(dst + (BlkY_off + Blkxsb0_off) + (xsb0 + yo + 2 * column_width),
         mem_copy(dst + (BlkY_off + Blkxsb0_off) + (xsb0 + yo + 3 * column_width),
               for (x = x1; x < x2; x += ytile_span) {
               mem_copy_align16(dst + (BlkY_off + BlkX_off) + (xo + yo+ 0 * column_width),
         mem_copy_align16(dst + (BlkY_off + BlkX_off) + (xo + yo + 1 * column_width),
         mem_copy_align16(dst + (BlkY_off + BlkX_off) + (xo + yo + 2 * column_width),
                                    if (x2 != x3) {
               mem_copy(dst + (BlkY_off + BlkX_off) + (xo + yo + 0 * column_width),
         mem_copy(dst + (BlkY_off + BlkX_off) + (xo + yo + 1 * column_width),
         mem_copy(dst + (BlkY_off + BlkX_off) + (xo + yo + 2 * column_width),
         mem_copy(dst + (BlkY_off + BlkX_off) + (xo + yo + 3 * column_width),
                           if (y2 != y3) {
      for (yo = Y2; yo < Y2 + (y3 - y2) * column_width; yo += column_width) {
                                                      mem_copy_align16(dst + (BlkY_off + BlkX_off) + (xo + yo), src + x, ytile_span);
               if (x3 != x2) {
      BlkX_off = ALIGN_DOWN(xo, 256);
                        }
      /**
   * Copy texture data from X tile layout to linear.
   *
   * \copydoc tile_copy_fn
   */
   static inline void
   xtiled_to_linear(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y1,
   char *dst, const char *src,
   int32_t dst_pitch,
      {
      /* The copy destination offset for each range copied is the sum of
   * an X offset 'x0' or 'xo' and a Y offset 'yo.'
   */
                     for (yo = y0 * xtile_width; yo < y1 * xtile_width; yo += xtile_width) {
      /* Bits 9 and 10 of the copy destination offset control swizzling.
   * Only 'yo' contributes to those bits in the total offset,
   * so calculate 'swizzle' just once per row.
   * Move bits 9 and 10 three and four places respectively down
   * to bit 6 and xor them.
   */
                     for (xo = x1; xo < x2; xo += xtile_span) {
                                 }
      /**
   * Copy texture data from Y tile layout to linear.
   *
   * \copydoc tile_copy_fn
   */
   static inline void
   ytiled_to_linear(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y3,
   char *dst, const char *src,
   int32_t dst_pitch,
      {
      /* Y tiles consist of columns that are 'ytile_span' wide (and the same height
   * as the tile).  Thus the destination offset for (x,y) is the sum of:
   *   (x % column_width)                    // position within column
   *   (x / column_width) * bytes_per_column // column number * bytes per column
   *   y * column_width
   *
   * The copy destination offset for each range copied is the sum of
   * an X offset 'xo0' or 'xo' and a Y offset 'yo.'
   */
   const uint32_t column_width = ytile_span;
            uint32_t y1 = MIN2(y3, ALIGN_UP(y0, 4));
            uint32_t xo0 = (x0 % ytile_span) + (x0 / ytile_span) * bytes_per_column;
            /* Bit 9 of the destination offset control swizzling.
   * Only the X offset contributes to bit 9 of the total offset,
   * so swizzle can be calculated in advance for these X positions.
   * Move bit 9 three places down to bit 6.
   */
   uint32_t swizzle0 = (xo0 >> 3) & swizzle_bit;
                              if (y0 != y1) {
      for (yo = y0 * column_width; yo < y1 * column_width; yo += column_width) {
                              /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + x, src + ((xo + yo) ^ swizzle), ytile_span);
   xo += bytes_per_column;
                                       for (yo = y1 * column_width; yo < y2 * column_width; yo += 4 * column_width) {
      uint32_t xo = xo1;
            if (x0 != x1) {
      mem_copy(dst + x0 + 0 * dst_pitch, src + ((xo0 + yo + 0 * column_width) ^ swizzle0), x1 - x0);
   mem_copy(dst + x0 + 1 * dst_pitch, src + ((xo0 + yo + 1 * column_width) ^ swizzle0), x1 - x0);
   mem_copy(dst + x0 + 2 * dst_pitch, src + ((xo0 + yo + 2 * column_width) ^ swizzle0), x1 - x0);
               /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + x + 0 * dst_pitch, src + ((xo + yo + 0 * column_width) ^ swizzle), ytile_span);
   mem_copy_align16(dst + x + 1 * dst_pitch, src + ((xo + yo + 1 * column_width) ^ swizzle), ytile_span);
   mem_copy_align16(dst + x + 2 * dst_pitch, src + ((xo + yo + 2 * column_width) ^ swizzle), ytile_span);
   mem_copy_align16(dst + x + 3 * dst_pitch, src + ((xo + yo + 3 * column_width) ^ swizzle), ytile_span);
   xo += bytes_per_column;
               if (x2 != x3) {
      mem_copy_align16(dst + x2 + 0 * dst_pitch, src + ((xo + yo + 0 * column_width) ^ swizzle), x3 - x2);
   mem_copy_align16(dst + x2 + 1 * dst_pitch, src + ((xo + yo + 1 * column_width) ^ swizzle), x3 - x2);
   mem_copy_align16(dst + x2 + 2 * dst_pitch, src + ((xo + yo + 2 * column_width) ^ swizzle), x3 - x2);
                           if (y2 != y3) {
      for (yo = y2 * column_width; yo < y3 * column_width; yo += column_width) {
                              /* Step by spans/columns.  As it happens, the swizzle bit flips
   * at each step so we don't need to calculate it explicitly.
   */
   for (x = x1; x < x2; x += ytile_span) {
      mem_copy_align16(dst + x, src + ((xo + yo) ^ swizzle), ytile_span);
   xo += bytes_per_column;
                                 }
         /**
   * Copy texture data from linear to Tile-4 layout.
   *
   * \copydoc tile_copy_fn
   */
   static inline void
   tile4_to_linear(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y3,
   char *dst, const char *src,
   int32_t dst_pitch,
      {
         /* Tile 4 consist of columns that are 'ytile_span' wide and each 64B tile block
   * consists of 4 row of Y-tile ordered data.
   * Each 512B block within a 4kB tile contains 8 such block.
   *
   * To calculate the tiled  offset, we need to identify:
   * Block X and Block Y offset at each 512B block boundary in X and Y direction.
   *
   * Refer to the Tile4 layout diagram in linear_to_tile4() function.
   *
   * The tile is divided in 512B blocks[Blk0..Blk7], themselves made of 2
   * rows of 256B sub-blocks
   *
   * Each sub-block is composed of 4 64B elements[cell(0)-cell(3)].
   *
   * Each 64B cell represents 4 rows of data.[cell(0), cell(1), .., cell(63)]
   *
   *
   *   Block X - Adds 256B to offset when we encounter block boundary in
   *             X direction.(Ex: Blk 0 --> Blk 1(BlkX_off = 256))
   *   Block Y - Adds 512B to offset when we encounter block boundary in
   *             Y direction.(Ex: Blk 0 --> Blk 3(BlkY_off = 512))
   *
   *   (x / ytile_span) * cacheline_size_B //Byte offset in the X dir of the
   *                                         containing 64B block
   *   x % ytile_span //Byte offset in X dir within a 64B block/cacheline
   *
   *   (y % 4) * 16 // Byte offset of the Y dir within a 64B block/cacheline
   *   (y / 4) * 256// Byte offset of the Y dir within 512B block after 1 row
   *                   of 64B blocks/cachelines
   *
   * The copy destination offset for each range copied is the sum of
   * Block X offset 'BlkX_off', Block Y offset 'BlkY_off', X offset 'xo'
   * and a Y offset 'yo.'
            const uint32_t column_width = ytile_span;
            assert(ytile_span * tile4_blkh == 64);
            /* Find intermediate Y offsets that are aligned to a 64B element
   * (4 rows), so that we can do fully 64B memcpys on those.
   */
   uint32_t y1 = MIN2(y3, ALIGN_UP(y0, 4));
            /* xsb0 and xsb1 are the byte offset within a 256B sub block for x0 and x1 */
   uint32_t xsb0 = (x0 % ytile_span) + (x0 / ytile_span) * cacheline_size_B;
            uint32_t Blkxsb0_off = ALIGN_DOWN(xsb0, 256);
                              /* Y0 determines the initial byte offset in the Y direction */
            /* Y2 determines the byte offset required for reaching y2 if y2 doesn't map
   * exactly to 512B block boundary
   */
                     /* To maximize memcpy speed, we do the copy in 3 parts :
   *   - copy the first lines that are not aligned to the 64B cell's height (4 rows)
   *   - copy the lines that are aligned to 64B cell's height
   *   - copy the remaining lines not making up for a full 64B cell's height
   */
   if (y0 != y1) {
      for (yo = Y0; yo < Y0 + (y1 - y0) * column_width; yo += column_width) {
                                                mem_copy_align16(dst + x, src + (Blky0_off + BlkX_off) + (xo + yo), ytile_span);
               if (x3 != x2) {
      BlkX_off = ALIGN_DOWN(xo, 256);
                              for (yo = y1 * 4 * column_width; yo < y2 * 4 * column_width; yo += 16 * column_width) {
      uint32_t xo = xsb1;
            if (x0 != x1) {
      mem_copy(dst + x0 + 0 * dst_pitch,
               mem_copy(dst + x0 + 1 * dst_pitch,
               mem_copy(dst + x0 + 2 * dst_pitch,
               mem_copy(dst + x0 + 3 * dst_pitch,
                     for (x = x1; x < x2; x += ytile_span) {
               mem_copy_align16(dst + x + 0 * dst_pitch,
               mem_copy_align16(dst + x + 1 * dst_pitch,
               mem_copy_align16(dst + x + 2 * dst_pitch,
               mem_copy_align16(dst + x + 3 * dst_pitch,
                              if (x2 != x3) {
               mem_copy(dst + x2 + 0 * dst_pitch,
               mem_copy(dst + x2 + 1 * dst_pitch,
               mem_copy(dst + x2 + 2 * dst_pitch,
               mem_copy(dst + x2 + 3 * dst_pitch,
                                 if (y2 != y3) {
      for (yo = Y2; yo < Y2 + (y3 - y2) * column_width; yo += column_width) {
                                                      mem_copy_align16(dst + x, src + (BlkY_off + BlkX_off) + (xo + yo), ytile_span);
               if (x3 != x2) {
      BlkX_off = ALIGN_DOWN(xo, 256);
                        }
      #if defined(INLINE_SSE41)
   static ALWAYS_INLINE void *
   _memcpy_streaming_load(void *dest, const void *src, size_t count)
   {
      if (count == 16) {
      __m128i val = _mm_stream_load_si128((__m128i *)src);
   _mm_storeu_si128((__m128i *)dest, val);
      } else if (count == 64) {
      __m128i val0 = _mm_stream_load_si128(((__m128i *)src) + 0);
   __m128i val1 = _mm_stream_load_si128(((__m128i *)src) + 1);
   __m128i val2 = _mm_stream_load_si128(((__m128i *)src) + 2);
   __m128i val3 = _mm_stream_load_si128(((__m128i *)src) + 3);
   _mm_storeu_si128(((__m128i *)dest) + 0, val0);
   _mm_storeu_si128(((__m128i *)dest) + 1, val1);
   _mm_storeu_si128(((__m128i *)dest) + 2, val2);
   _mm_storeu_si128(((__m128i *)dest) + 3, val3);
      } else {
      assert(count < 64); /* and (count < 16) for ytiled */
         }
   #endif
      static isl_mem_copy_fn
   choose_copy_function(isl_memcpy_type copy_type)
   {
      switch(copy_type) {
   case ISL_MEMCPY:
         case ISL_MEMCPY_BGRA8:
            #if defined(INLINE_SSE41)
         #else
         #endif
      case ISL_MEMCPY_INVALID:
         }
   unreachable("unhandled copy_type");
      }
      /**
   * Copy texture data from linear to X tile layout, faster.
   *
   * Same as \ref linear_to_xtiled but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   linear_to_xtiled_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
               if (x0 == 0 && x3 == xtile_width && y0 == 0 && y1 == xtile_height) {
      if (mem_copy == memcpy)
      return linear_to_xtiled(0, 0, xtile_width, xtile_width, 0, xtile_height,
      else if (mem_copy == rgba8_copy)
      return linear_to_xtiled(0, 0, xtile_width, xtile_width, 0, xtile_height,
            else
      } else {
      if (mem_copy == memcpy)
      return linear_to_xtiled(x0, x1, x2, x3, y0, y1,
            else if (mem_copy == rgba8_copy)
      return linear_to_xtiled(x0, x1, x2, x3, y0, y1,
            else
      }
   linear_to_xtiled(x0, x1, x2, x3, y0, y1,
      }
      /**
   * Copy texture data from linear to Y tile layout, faster.
   *
   * Same as \ref linear_to_ytiled but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   linear_to_ytiled_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
               if (x0 == 0 && x3 == ytile_width && y0 == 0 && y1 == ytile_height) {
      if (mem_copy == memcpy)
      return linear_to_ytiled(0, 0, ytile_width, ytile_width, 0, ytile_height,
      else if (mem_copy == rgba8_copy)
      return linear_to_ytiled(0, 0, ytile_width, ytile_width, 0, ytile_height,
            else
      } else {
      if (mem_copy == memcpy)
      return linear_to_ytiled(x0, x1, x2, x3, y0, y1,
      else if (mem_copy == rgba8_copy)
      return linear_to_ytiled(x0, x1, x2, x3, y0, y1,
            else
      }
   linear_to_ytiled(x0, x1, x2, x3, y0, y1,
      }
      /**
   * Copy texture data from linear to tile 4 layout, faster.
   *
   * Same as \ref linear_to_tile4 but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   linear_to_tile4_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
      isl_mem_copy_fn mem_copy = choose_copy_function(copy_type);
            if (x0 == 0 && x3 == ytile_width && y0 == 0 && y1 == ytile_height) {
      if (mem_copy == memcpy)
      return linear_to_tile4(0, 0, ytile_width, ytile_width, 0, ytile_height,
      else if (mem_copy == rgba8_copy)
      return linear_to_tile4(0, 0, ytile_width, ytile_width, 0, ytile_height,
            else
      } else {
      if (mem_copy == memcpy)
      return linear_to_tile4(x0, x1, x2, x3, y0, y1,
      else if (mem_copy == rgba8_copy)
      return linear_to_tile4(x0, x1, x2, x3, y0, y1,
            else
         }
      /**
   * Copy texture data from X tile layout to linear, faster.
   *
   * Same as \ref xtile_to_linear but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   xtiled_to_linear_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
               if (x0 == 0 && x3 == xtile_width && y0 == 0 && y1 == xtile_height) {
      if (mem_copy == memcpy)
      return xtiled_to_linear(0, 0, xtile_width, xtile_width, 0, xtile_height,
      else if (mem_copy == rgba8_copy)
      return xtiled_to_linear(0, 0, xtile_width, xtile_width, 0, xtile_height,
      #if defined(INLINE_SSE41)
         else if (mem_copy == _memcpy_streaming_load)
      return xtiled_to_linear(0, 0, xtile_width, xtile_width, 0, xtile_height,
      #endif
         else
      } else {
      if (mem_copy == memcpy)
      return xtiled_to_linear(x0, x1, x2, x3, y0, y1,
      else if (mem_copy == rgba8_copy)
      return xtiled_to_linear(x0, x1, x2, x3, y0, y1,
      #if defined(INLINE_SSE41)
         else if (mem_copy == _memcpy_streaming_load)
      return xtiled_to_linear(x0, x1, x2, x3, y0, y1,
      #endif
         else
      }
   xtiled_to_linear(x0, x1, x2, x3, y0, y1,
      }
      /**
   * Copy texture data from Y tile layout to linear, faster.
   *
   * Same as \ref ytile_to_linear but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   ytiled_to_linear_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
               if (x0 == 0 && x3 == ytile_width && y0 == 0 && y1 == ytile_height) {
      if (mem_copy == memcpy)
      return ytiled_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      else if (mem_copy == rgba8_copy)
      return ytiled_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      #if defined(INLINE_SSE41)
         else if (copy_type == ISL_MEMCPY_STREAMING_LOAD)
      return ytiled_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      #endif
         else
      } else {
      if (mem_copy == memcpy)
      return ytiled_to_linear(x0, x1, x2, x3, y0, y1,
      else if (mem_copy == rgba8_copy)
      return ytiled_to_linear(x0, x1, x2, x3, y0, y1,
      #if defined(INLINE_SSE41)
         else if (copy_type == ISL_MEMCPY_STREAMING_LOAD)
      return ytiled_to_linear(x0, x1, x2, x3, y0, y1,
      #endif
         else
      }
   ytiled_to_linear(x0, x1, x2, x3, y0, y1,
      }
      /**
   * Copy texture data from tile4 layout to linear, faster.
   *
   * Same as \ref tile4_to_linear but faster, because it passes constant
   * parameters for common cases, allowing the compiler to inline code
   * optimized for those cases.
   *
   * \copydoc tile_copy_fn
   */
   static FLATTEN void
   tile4_to_linear_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                           uint32_t y0, uint32_t y1,
   {
      isl_mem_copy_fn mem_copy = choose_copy_function(copy_type);
            if (x0 == 0 && x3 == ytile_width && y0 == 0 && y1 == ytile_height) {
      if (mem_copy == memcpy)
      return tile4_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      else if (mem_copy == rgba8_copy)
      return tile4_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      #if defined(INLINE_SSE41)
         else if (copy_type == ISL_MEMCPY_STREAMING_LOAD)
      return tile4_to_linear(0, 0, ytile_width, ytile_width, 0, ytile_height,
      #endif
         else
      } else {
      if (mem_copy == memcpy)
      return tile4_to_linear(x0, x1, x2, x3, y0, y1,
      else if (mem_copy == rgba8_copy)
      return tile4_to_linear(x0, x1, x2, x3, y0, y1,
      #if defined(INLINE_SSE41)
         else if (copy_type == ISL_MEMCPY_STREAMING_LOAD)
      return tile4_to_linear(x0, x1, x2, x3, y0, y1,
      #endif
         else
         }
      /**
   * Copy from linear to tiled texture.
   *
   * Divide the region given by X range [xt1, xt2) and Y range [yt1, yt2) into
   * pieces that do not cross tile boundaries and copy each piece with a tile
   * copy function (\ref tile_copy_fn).
   * The X range is in bytes, i.e. pixels * bytes-per-pixel.
   * The Y range is in pixels (i.e. unitless).
   * 'dst' is the address of (0, 0) in the destination tiled texture.
   * 'src' is the address of (xt1, yt1) in the source linear texture.
   */
   static void
   linear_to_tiled(uint32_t xt1, uint32_t xt2,
                        uint32_t yt1, uint32_t yt2,
   char *dst, const char *src,
      {
      tile_copy_fn tile_copy;
   uint32_t xt0, xt3;
   uint32_t yt0, yt3;
   uint32_t xt, yt;
   uint32_t tw, th, span;
            if (tiling == ISL_TILING_X) {
      tw = xtile_width;
   th = xtile_height;
   span = xtile_span;
      } else if (tiling == ISL_TILING_Y0) {
      tw = ytile_width;
   th = ytile_height;
   span = ytile_span;
      } else if (tiling == ISL_TILING_4) {
      tw = ytile_width;
   th = ytile_height;
   span = ytile_span;
      } else {
                  /* Round out to tile boundaries. */
   xt0 = ALIGN_DOWN(xt1, tw);
   xt3 = ALIGN_UP  (xt2, tw);
   yt0 = ALIGN_DOWN(yt1, th);
            /* Loop over all tiles to which we have something to copy.
   * 'xt' and 'yt' are the origin of the destination tile, whether copying
   * copying a full or partial tile.
   * tile_copy() copies one tile or partial tile.
   * Looping x inside y is the faster memory access pattern.
   */
   for (yt = yt0; yt < yt3; yt += th) {
      for (xt = xt0; xt < xt3; xt += tw) {
      /* The area to update is [x0,x3) x [y0,y1).
   * May not want the whole tile, hence the min and max.
   */
   uint32_t x0 = MAX2(xt1, xt);
   uint32_t y0 = MAX2(yt1, yt);
                  /* [x0,x3) is split into [x0,x1), [x1,x2), [x2,x3) such that
   * the middle interval is the longest span-aligned part.
   * The sub-ranges could be empty.
   */
   uint32_t x1, x2;
   x1 = ALIGN_UP(x0, span);
   if (x1 > x3)
                        assert(x0 <= x1 && x1 <= x2 && x2 <= x3);
   assert(x1 - x0 < span && x3 - x2 < span);
                  /* Translate by (xt,yt) for single-tile copier. */
   tile_copy(x0-xt, x1-xt, x2-xt, x3-xt,
            y0-yt, y1-yt,
   dst + (ptrdiff_t)xt * th  +  (ptrdiff_t)yt        * dst_pitch,
   src + (ptrdiff_t)xt - xt1 + ((ptrdiff_t)yt - yt1) * src_pitch,
   src_pitch,
         }
      /**
   * Copy from tiled to linear texture.
   *
   * Divide the region given by X range [xt1, xt2) and Y range [yt1, yt2) into
   * pieces that do not cross tile boundaries and copy each piece with a tile
   * copy function (\ref tile_copy_fn).
   * The X range is in bytes, i.e. pixels * bytes-per-pixel.
   * The Y range is in pixels (i.e. unitless).
   * 'dst' is the address of (xt1, yt1) in the destination linear texture.
   * 'src' is the address of (0, 0) in the source tiled texture.
   */
   static void
   tiled_to_linear(uint32_t xt1, uint32_t xt2,
                        uint32_t yt1, uint32_t yt2,
   char *dst, const char *src,
      {
      tile_copy_fn tile_copy;
   uint32_t xt0, xt3;
   uint32_t yt0, yt3;
   uint32_t xt, yt;
   uint32_t tw, th, span;
            if (tiling == ISL_TILING_X) {
      tw = xtile_width;
   th = xtile_height;
   span = xtile_span;
      } else if (tiling == ISL_TILING_Y0) {
      tw = ytile_width;
   th = ytile_height;
   span = ytile_span;
      } else if (tiling == ISL_TILING_4) {
      tw = ytile_width;
   th = ytile_height;
   span = ytile_span;
      } else {
               #if defined(INLINE_SSE41)
      if (copy_type == ISL_MEMCPY_STREAMING_LOAD) {
      /* The hidden cacheline sized register used by movntdqa can apparently
   * give you stale data, so do an mfence to invalidate it.
   */
         #endif
         /* Round out to tile boundaries. */
   xt0 = ALIGN_DOWN(xt1, tw);
   xt3 = ALIGN_UP  (xt2, tw);
   yt0 = ALIGN_DOWN(yt1, th);
            /* Loop over all tiles to which we have something to copy.
   * 'xt' and 'yt' are the origin of the destination tile, whether copying
   * copying a full or partial tile.
   * tile_copy() copies one tile or partial tile.
   * Looping x inside y is the faster memory access pattern.
   */
   for (yt = yt0; yt < yt3; yt += th) {
      for (xt = xt0; xt < xt3; xt += tw) {
      /* The area to update is [x0,x3) x [y0,y1).
   * May not want the whole tile, hence the min and max.
   */
   uint32_t x0 = MAX2(xt1, xt);
   uint32_t y0 = MAX2(yt1, yt);
                  /* [x0,x3) is split into [x0,x1), [x1,x2), [x2,x3) such that
   * the middle interval is the longest span-aligned part.
   * The sub-ranges could be empty.
   */
   uint32_t x1, x2;
   x1 = ALIGN_UP(x0, span);
   if (x1 > x3)
                        assert(x0 <= x1 && x1 <= x2 && x2 <= x3);
   assert(x1 - x0 < span && x3 - x2 < span);
                  /* Translate by (xt,yt) for single-tile copier. */
   tile_copy(x0-xt, x1-xt, x2-xt, x3-xt,
            y0-yt, y1-yt,
   dst + (ptrdiff_t)xt - xt1 + ((ptrdiff_t)yt - yt1) * dst_pitch,
   src + (ptrdiff_t)xt * th  +  (ptrdiff_t)yt        * src_pitch,
   dst_pitch,
         }
