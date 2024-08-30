   #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/format/u_format_etc.h"
      /* define etc1_parse_block and etc. */
   #define UINT8_TYPE uint8_t
   #define TAG(x) x
   #include "util/format/texcompress_etc_tmp.h"
   #undef TAG
   #undef UINT8_TYPE
      void
   util_format_etc1_rgb8_unpack_rgba_8unorm(uint8_t *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
         }
      void
   util_format_etc1_rgb8_pack_rgba_8unorm(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_etc1_rgb8_unpack_rgba_float(void *restrict dst_row, unsigned dst_stride, const uint8_t *restrict src_row, unsigned src_stride, unsigned width, unsigned height)
   {
      const unsigned bw = 4, bh = 4, bs = 8, comps = 4;
   struct etc1_block block;
            for (y = 0; y < height; y += bh) {
               for (x = 0; x < width; x+= bw) {
               for (j = 0; j < bh; j++) {
                     for (i = 0; i < bw; i++) {
      etc1_fetch_texel(&block, i, j, tmp);
   dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
   dst[3] = 1.0f;
                                    }
      void
   util_format_etc1_rgb8_pack_rgba_float(UNUSED uint8_t *restrict dst_row, UNUSED unsigned dst_stride,
               {
         }
      void
   util_format_etc1_rgb8_fetch_rgba(void *restrict in_dst, const uint8_t *restrict src, unsigned i, unsigned j)
   {
      float *dst = in_dst;
   struct etc1_block block;
                     etc1_parse_block(&block, src);
            dst[0] = ubyte_to_float(tmp[0]);
   dst[1] = ubyte_to_float(tmp[1]);
   dst[2] = ubyte_to_float(tmp[2]);
      }
