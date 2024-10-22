   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2014  Intel Corporation  All Rights Reserved.
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
   */
      #include <stdlib.h>
      #include "errors.h"
   #include "format_utils.h"
   #include "glformats.h"
   #include "format_pack.h"
   #include "format_unpack.h"
      const mesa_array_format RGBA32_FLOAT =
      MESA_ARRAY_FORMAT(MESA_ARRAY_FORMAT_BASE_FORMAT_RGBA_VARIANTS,
         const mesa_array_format RGBA8_UBYTE =
      MESA_ARRAY_FORMAT(MESA_ARRAY_FORMAT_BASE_FORMAT_RGBA_VARIANTS,
         const mesa_array_format BGRA8_UBYTE =
      MESA_ARRAY_FORMAT(MESA_ARRAY_FORMAT_BASE_FORMAT_RGBA_VARIANTS,
         const mesa_array_format RGBA32_UINT =
      MESA_ARRAY_FORMAT(MESA_ARRAY_FORMAT_BASE_FORMAT_RGBA_VARIANTS,
         const mesa_array_format RGBA32_INT =
      MESA_ARRAY_FORMAT(MESA_ARRAY_FORMAT_BASE_FORMAT_RGBA_VARIANTS,
         static void
   invert_swizzle(uint8_t dst[4], const uint8_t src[4])
   {
               dst[0] = MESA_FORMAT_SWIZZLE_NONE;
   dst[1] = MESA_FORMAT_SWIZZLE_NONE;
   dst[2] = MESA_FORMAT_SWIZZLE_NONE;
            for (i = 0; i < 4; ++i)
      for (j = 0; j < 4; ++j)
         }
      /* Takes a src to RGBA swizzle and applies a rebase swizzle to it. This
   * is used when we need to rebase a format to match a different
   * base internal format.
   *
   * The rebase swizzle can be NULL, which means that no rebase is necessary,
   * in which case the src to RGBA swizzle is copied to the output without
   * changes.
   *
   * The resulting rebased swizzle and well as the input swizzles are
   * all 4-element swizzles, but the rebase swizzle can be NULL if no rebase
   * is necessary.
   */
   static void
   compute_rebased_rgba_component_mapping(uint8_t *src2rgba,
               {
               if (rebase_swizzle) {
      for (i = 0; i < 4; i++) {
      if (rebase_swizzle[i] > MESA_FORMAT_SWIZZLE_W)
         else
         } else {
      /* No rebase needed, so src2rgba is all that we need */
         }
      /* Computes the final swizzle transform to apply from src to dst in a
   * conversion that might involve a rebase swizzle.
   *
   * This is used to compute the swizzle transform to apply in conversions
   * between array formats where we have a src2rgba swizzle, a rgba2dst swizzle
   * and possibly, a rebase swizzle.
   *
   * The final swizzle transform to apply (src2dst) when a rebase swizzle is
   * involved is: src -> rgba -> base -> rgba -> dst
   */
   static void
   compute_src2dst_component_mapping(uint8_t *src2rgba, uint8_t *rgba2dst,
         {
               if (!rebase_swizzle) {
      for (i = 0; i < 4; i++) {
      if (rgba2dst[i] > MESA_FORMAT_SWIZZLE_W) {
         } else {
               } else {
      for (i = 0; i < 4; i++) {
      if (rgba2dst[i] > MESA_FORMAT_SWIZZLE_W) {
         } else if (rebase_swizzle[rgba2dst[i]] > MESA_FORMAT_SWIZZLE_W) {
         } else {
                  }
      /**
   * This function is used by clients of _mesa_format_convert to obtain
   * the rebase swizzle to use in a format conversion based on the base
   * format involved.
   *
   * \param baseFormat  the base internal format involved in the conversion.
   * \param map  the rebase swizzle to consider
   *
   * This function computes 'map' as rgba -> baseformat -> rgba and returns true
   * if the resulting swizzle transform is not the identity transform (thus, a
   * rebase is needed). If the function returns false then a rebase swizzle
   * is not necessary and the value of 'map' is undefined. In this situation
   * clients of _mesa_format_convert should pass NULL in the 'rebase_swizzle'
   * parameter.
   */
   bool
   _mesa_compute_rgba2base2rgba_component_mapping(GLenum baseFormat, uint8_t *map)
   {
      uint8_t rgba2base[6], base2rgba[6];
            switch (baseFormat) {
   case GL_ALPHA:
   case GL_RED:
   case GL_GREEN:
   case GL_BLUE:
   case GL_RG:
   case GL_RGB:
   case GL_BGR:
   case GL_RGBA:
   case GL_BGRA:
   case GL_ABGR_EXT:
   case GL_LUMINANCE:
   case GL_INTENSITY:
   case GL_LUMINANCE_ALPHA:
      {
      bool needRebase = false;
   _mesa_compute_component_mapping(GL_RGBA, baseFormat, rgba2base);
   _mesa_compute_component_mapping(baseFormat, GL_RGBA, base2rgba);
   for (i = 0; i < 4; i++) {
      if (base2rgba[i] > MESA_FORMAT_SWIZZLE_W) {
         } else {
         }
   if (map[i] != i)
      }
         default:
            }
         /**
   * Special case conversion function to swap r/b channels from the source
   * image to the dest image.
   */
   static void
   convert_ubyte_rgba_to_bgra(size_t width, size_t height,
               {
               if (sizeof(void *) == 8 &&
      src_stride % 8 == 0 &&
   dst_stride % 8 == 0 &&
   (GLsizeiptr) src % 8 == 0 &&
   (GLsizeiptr) dst % 8 == 0) {
   /* use 64-bit word to swizzle two 32-bit pixels.  We need 8-byte
   * alignment for src/dst addresses and strides.
   */
   for (row = 0; row < height; row++) {
      const GLuint64 *s = (const GLuint64 *) src;
   GLuint64 *d = (GLuint64 *) dst;
   int i;
   for (i = 0; i < width/2; i++) {
      d[i] = ( (s[i] & 0xff00ff00ff00ff00) |
            }
   if (width & 1) {
      /* handle the case of odd widths */
   const GLuint s = ((const GLuint *) src)[width - 1];
   GLuint *d = (GLuint *) dst + width - 1;
   *d = ( (s & 0xff00ff00) |
            }
   src += src_stride;
         } else {
      for (row = 0; row < height; row++) {
      const GLuint *s = (const GLuint *) src;
   GLuint *d = (GLuint *) dst;
   int i;
   for (i = 0; i < width; i++) {
      d[i] = ( (s[i] & 0xff00ff00) |
            }
   src += src_stride;
            }
         /**
   * This can be used to convert between most color formats.
   *
   * Limitations:
   * - This function doesn't handle GL_COLOR_INDEX or YCBCR formats.
   * - This function doesn't handle byte-swapping or transferOps, these should
   *   be handled by the caller.
   *
   * \param void_dst  The address where converted color data will be stored.
   *                  The caller must ensure that the buffer is large enough
   *                  to hold the converted pixel data.
   * \param dst_format  The destination color format. It can be a mesa_format
   *                    or a mesa_array_format represented as an uint32_t.
   * \param dst_stride  The stride of the destination format in bytes.
   * \param void_src  The address of the source color data to convert.
   * \param src_format  The source color format. It can be a mesa_format
   *                    or a mesa_array_format represented as an uint32_t.
   * \param src_stride  The stride of the source format in bytes.
   * \param width  The width, in pixels, of the source image to convert.
   * \param height  The height, in pixels, of the source image to convert.
   * \param rebase_swizzle  A swizzle transform to apply during the conversion,
   *                        typically used to match a different internal base
   *                        format involved. NULL if no rebase transform is needed
   *                        (i.e. the internal base format and the base format of
   *                        the dst or the src -depending on whether we are doing
   *                        an upload or a download respectively- are the same).
   */
   void
   _mesa_format_convert(void *void_dst, uint32_t dst_format, size_t dst_stride,
               {
      uint8_t *dst = (uint8_t *)void_dst;
   uint8_t *src = (uint8_t *)void_src;
   mesa_array_format src_array_format, dst_array_format;
   bool src_format_is_mesa_array_format, dst_format_is_mesa_array_format;
   uint8_t src2dst[4], src2rgba[4], rgba2dst[4], dst2rgba[4];
   uint8_t rebased_src2rgba[4];
   enum mesa_array_format_datatype src_type = 0, dst_type = 0, common_type;
   bool normalized, dst_integer, src_integer, is_signed;
   int src_num_channels = 0, dst_num_channels = 0;
   uint8_t (*tmp_ubyte)[4];
   float (*tmp_float)[4];
   uint32_t (*tmp_uint)[4];
   int bits;
            if (_mesa_format_is_mesa_array_format(src_format)) {
      src_format_is_mesa_array_format = true;
      } else {
      assert(_mesa_is_format_color_format(src_format));
   src_format_is_mesa_array_format = false;
               if (_mesa_format_is_mesa_array_format(dst_format)) {
      dst_format_is_mesa_array_format = true;
      } else {
      assert(_mesa_is_format_color_format(dst_format));
   dst_format_is_mesa_array_format = false;
               /* First we see if we can implement the conversion with a direct pack
   * or unpack.
   *
   * In this case we want to be careful when we need to apply a swizzle to
   * match an internal base format, since in these cases a simple pack/unpack
   * to the dst format from the src format may not match the requirements
   * of the internal base format. For now we decide to be safe and
   * avoid this path in these scenarios but in the future we may want to
   * enable it for specific combinations that are known to work.
   */
   if (!rebase_swizzle) {
      /* Do a direct memcpy where possible */
   if ((dst_format_is_mesa_array_format &&
      src_format_is_mesa_array_format &&
   src_array_format == dst_array_format) ||
   src_format == dst_format) {
   int format_size = _mesa_get_format_bytes(src_format);
   for (row = 0; row < height; row++) {
      memcpy(dst, src, width * format_size);
   src += src_stride;
      }
               /* Handle the cases where we can directly unpack */
   if (!src_format_is_mesa_array_format) {
      if (dst_array_format == RGBA32_FLOAT) {
      for (row = 0; row < height; ++row) {
      _mesa_unpack_rgba_row(src_format, width,
         src += src_stride;
      }
      } else if (dst_array_format == RGBA8_UBYTE) {
      assert(!_mesa_is_format_integer_color(src_format));
   for (row = 0; row < height; ++row) {
      _mesa_unpack_ubyte_rgba_row(src_format, width,
         src += src_stride;
      #if UTIL_ARCH_LITTLE_ENDIAN
            } else if (dst_array_format == BGRA8_UBYTE &&
            convert_ubyte_rgba_to_bgra(width, height, src, src_stride,
   #endif
            } else if (dst_array_format == RGBA32_UINT &&
            assert(_mesa_is_format_integer_color(src_format));
   for (row = 0; row < height; ++row) {
      _mesa_unpack_uint_rgba_row(src_format, width,
         src += src_stride;
      }
                  /* Handle the cases where we can directly pack */
   if (!dst_format_is_mesa_array_format) {
      if (src_array_format == RGBA32_FLOAT) {
      for (row = 0; row < height; ++row) {
      _mesa_pack_float_rgba_row(dst_format, width,
         src += src_stride;
      }
            #if UTIL_ARCH_LITTLE_ENDIAN
               if (dst_format == MESA_FORMAT_B8G8R8A8_UNORM) {
      convert_ubyte_rgba_to_bgra(width, height, src, src_stride,
      #endif
               {
      for (row = 0; row < height; ++row) {
      _mesa_pack_ubyte_rgba_row(dst_format, width, src, dst);
   src += src_stride;
         }
      } else if (src_array_format == RGBA32_UINT &&
            assert(_mesa_is_format_integer_color(dst_format));
   for (row = 0; row < height; ++row) {
      _mesa_pack_uint_rgba_row(dst_format, width,
         src += src_stride;
      }
                     /* Handle conversions between array formats */
   normalized = false;
   if (src_array_format) {
                                             if (dst_array_format) {
                        _mesa_array_format_get_swizzle(dst_array_format, dst2rgba);
                        if (src_array_format && dst_array_format) {
      assert(_mesa_array_format_is_normalized(src_array_format) ==
            compute_src2dst_component_mapping(src2rgba, rgba2dst, rebase_swizzle,
            for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(dst, dst_type, dst_num_channels,
               src += src_stride;
      }
               /* At this point, we're fresh out of fast-paths and we need to convert
   * to float, uint32, or, if we're lucky, uint8.
   */
   dst_integer = false;
            if (src_array_format) {
      if (!_mesa_array_format_is_float(src_array_format) &&
      !_mesa_array_format_is_normalized(src_array_format))
   } else {
      switch (_mesa_get_format_datatype(src_format)) {
   case GL_UNSIGNED_INT:
   case GL_INT:
      src_integer = true;
                  /* If the destination format is signed but the source is unsigned, then we
   * don't loose any data by converting to a signed intermediate format above
   * and beyond the precision that we loose in the conversion itself. If the
   * destination is unsigned then, by using an unsigned intermediate format,
   * we make the conversion function that converts from the source to the
   * intermediate format take care of truncating at zero. The exception here
   * is if the intermediate format is float, in which case the first
   * conversion will leave it signed and the second conversion will truncate
   * at zero.
   */
   is_signed = false;
   if (dst_array_format) {
      if (!_mesa_array_format_is_float(dst_array_format) &&
      !_mesa_array_format_is_normalized(dst_array_format))
      is_signed = _mesa_array_format_is_signed(dst_array_format);
      } else {
      switch (_mesa_get_format_datatype(dst_format)) {
   case GL_UNSIGNED_NORMALIZED:
      is_signed = false;
      case GL_SIGNED_NORMALIZED:
      is_signed = true;
      case GL_FLOAT:
      is_signed = true;
      case GL_UNSIGNED_INT:
      is_signed = false;
   dst_integer = true;
      case GL_INT:
      is_signed = true;
   dst_integer = true;
      }
                        if (src_integer && dst_integer) {
               /* The [un]packing functions for unsigned datatypes treat the 32-bit
   * integer array as signed for signed formats and as unsigned for
   * unsigned formats. This is a bit of a problem if we ever convert from
   * a signed to an unsigned format because the unsigned packing function
   * doesn't know that the input is signed and will treat it as unsigned
   * and not do the trunctation. The thing that saves us here is that all
   * of the packed formats are unsigned, so we can just always use
   * _mesa_swizzle_and_convert for signed formats, which is aware of the
   * truncation problem.
   */
   common_type = is_signed ? MESA_ARRAY_FORMAT_TYPE_INT :
         if (src_array_format) {
      compute_rebased_rgba_component_mapping(src2rgba, rebase_swizzle,
         for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(tmp_uint + row * width, common_type, 4,
                     } else {
      for (row = 0; row < height; ++row) {
      _mesa_unpack_uint_rgba_row(src_format, width,
         if (rebase_swizzle)
      _mesa_swizzle_and_convert(tmp_uint + row * width, common_type, 4,
                           /* At this point, we have already done the truncation if the source is
   * signed but the destination is unsigned, so no need to force the
   * _mesa_swizzle_and_convert path.
   */
   if (dst_format_is_mesa_array_format) {
      for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(dst, dst_type, dst_num_channels,
                     } else {
      for (row = 0; row < height; ++row) {
      _mesa_pack_uint_rgba_row(dst_format, width,
                           } else if (is_signed || bits > 8) {
               if (src_format_is_mesa_array_format) {
      compute_rebased_rgba_component_mapping(src2rgba, rebase_swizzle,
         for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(tmp_float + row * width,
                           } else {
      for (row = 0; row < height; ++row) {
      _mesa_unpack_rgba_row(src_format, width,
         if (rebase_swizzle)
      _mesa_swizzle_and_convert(tmp_float + row * width,
                                       if (dst_format_is_mesa_array_format) {
      for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(dst, dst_type, dst_num_channels,
                           } else {
      for (row = 0; row < height; ++row) {
      _mesa_pack_float_rgba_row(dst_format, width,
                           } else {
               if (src_format_is_mesa_array_format) {
      compute_rebased_rgba_component_mapping(src2rgba, rebase_swizzle,
         for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(tmp_ubyte + row * width,
                           } else {
      for (row = 0; row < height; ++row) {
      _mesa_unpack_ubyte_rgba_row(src_format, width,
         if (rebase_swizzle)
      _mesa_swizzle_and_convert(tmp_ubyte + row * width,
                                       if (dst_format_is_mesa_array_format) {
      for (row = 0; row < height; ++row) {
      _mesa_swizzle_and_convert(dst, dst_type, dst_num_channels,
                           } else {
      for (row = 0; row < height; ++row) {
      _mesa_pack_ubyte_rgba_row(dst_format, width,
                              }
      static const uint8_t map_identity[7] = { 0, 1, 2, 3, 4, 5, 6 };
   #if UTIL_ARCH_BIG_ENDIAN
   static const uint8_t map_3210[7] = { 3, 2, 1, 0, 4, 5, 6 };
   static const uint8_t map_1032[7] = { 1, 0, 3, 2, 4, 5, 6 };
   #endif
      /**
   * Describes a format as an array format, if possible
   *
   * A helper function for figuring out if a (possibly packed) format is
   * actually an array format and, if so, what the array parameters are.
   *
   * \param[in]  format         the mesa format
   * \param[out] type           the GL type of the array (GL_BYTE, etc.)
   * \param[out] num_components the number of components in the array
   * \param[out] swizzle        a swizzle describing how to get from the
   *                            given format to RGBA
   * \param[out] normalized     for integer formats, this represents whether
   *                            the format is a normalized integer or a
   *                            regular integer
   * \return  true if this format is an array format, false otherwise
   */
   bool
   _mesa_format_to_array(mesa_format format, GLenum *type, int *num_components,
         {
      int i;
   GLuint format_components;
   uint8_t packed_swizzle[4];
            if (_mesa_is_format_compressed(format))
                              switch (_mesa_get_format_layout(format)) {
   case MESA_FORMAT_LAYOUT_ARRAY:
      *num_components = format_components;
   _mesa_get_format_swizzle(format, swizzle);
      case MESA_FORMAT_LAYOUT_PACKED:
      switch (*type) {
   case GL_UNSIGNED_BYTE:
   case GL_BYTE:
      if (_mesa_get_format_max_bits(format) != 8)
         *num_components = _mesa_get_format_bytes(format);
   switch (*num_components) {
   case 1:
      endian = map_identity;
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
               #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                  default:
      endian = map_identity;
      }
      case GL_UNSIGNED_SHORT:
   case GL_SHORT:
   case GL_HALF_FLOAT:
      if (_mesa_get_format_max_bits(format) != 16)
         *num_components = _mesa_get_format_bytes(format) / 2;
   switch (*num_components) {
   case 1:
      endian = map_identity;
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                  default:
      endian = map_identity;
      }
      case GL_UNSIGNED_INT:
   case GL_INT:
   case GL_FLOAT:
      /* This isn't packed.  At least not really. */
   assert(format_components == 1);
   if (_mesa_get_format_max_bits(format) != 32)
         *num_components = format_components;
   endian = map_identity;
      default:
                           for (i = 0; i < 4; ++i)
               case MESA_FORMAT_LAYOUT_OTHER:
   default:
            }
      /**
   * Attempts to perform the given swizzle-and-convert operation with memcpy
   *
   * This function determines if the given swizzle-and-convert operation can
   * be done with a simple memcpy and, if so, does the memcpy.  If not, it
   * returns false and we fall back to the standard version below.
   *
   * The arguments are exactly the same as for _mesa_swizzle_and_convert
   *
   * \return  true if it successfully performed the swizzle-and-convert
   *          operation with memcpy, false otherwise
   */
   static bool
   swizzle_convert_try_memcpy(void *dst,
                              enum mesa_array_format_datatype dst_type,
      {
               if (src_type != dst_type)
         if (num_src_channels != num_dst_channels)
            for (i = 0; i < num_dst_channels; ++i)
      if (swizzle[i] != i && swizzle[i] != MESA_FORMAT_SWIZZLE_NONE)
         memcpy(dst, src, count * num_src_channels *
               }
      /**
   * Represents a single instance of the standard swizzle-and-convert loop
   *
   * Any swizzle-and-convert operation simply loops through the pixels and
   * performs the transformation operation one pixel at a time.  This macro
   * embodies one instance of the conversion loop.  This way we can do all
   * control flow outside of the loop and allow the compiler to unroll
   * everything inside the loop.
   *
   * Note: This loop is carefully crafted for performance.  Be careful when
   * changing it and run some benchmarks to ensure no performance regressions
   * if you do.
   *
   * \param   DST_TYPE    the C datatype of the destination
   * \param   DST_CHANS   the number of destination channels
   * \param   SRC_TYPE    the C datatype of the source
   * \param   SRC_CHANS   the number of source channels
   * \param   CONV        an expression for converting from the source data,
   *                      storred in the variable "src", to the destination
   *                      format
   */
   #define SWIZZLE_CONVERT_LOOP(DST_TYPE, DST_CHANS, SRC_TYPE, SRC_CHANS, CONV) \
      do {                                           \
      int s, j;                                   \
   for (s = 0; s < count; ++s) {               \
      for (j = 0; j < SRC_CHANS; ++j) {        \
      SRC_TYPE src = typed_src[j];          \
      }                                        \
         typed_dst[0] = tmp[swizzle_x];           \
   if (DST_CHANS > 1) {                     \
      typed_dst[1] = tmp[swizzle_y];        \
   if (DST_CHANS > 2) {                  \
      typed_dst[2] = tmp[swizzle_z];     \
   if (DST_CHANS > 3) {               \
               }                                        \
   typed_src += SRC_CHANS;                  \
               /**
   * Represents a single swizzle-and-convert operation
   *
   * This macro represents everything done in a single swizzle-and-convert
   * operation.  The actual work is done by the SWIZZLE_CONVERT_LOOP macro.
   * This macro acts as a wrapper that uses a nested switch to ensure that
   * all looping parameters get unrolled.
   *
   * This macro makes assumptions about variables etc. in the calling
   * function.  Changes to _mesa_swizzle_and_convert may require changes to
   * this macro.
   *
   * \param   DST_TYPE    the C datatype of the destination
   * \param   SRC_TYPE    the C datatype of the source
   * \param   CONV        an expression for converting from the source data,
   *                      storred in the variable "src", to the destination
   *                      format
   */
   #define SWIZZLE_CONVERT(DST_TYPE, SRC_TYPE, CONV)                 \
      do {                                                           \
      const uint8_t swizzle_x = swizzle[0];                       \
   const uint8_t swizzle_y = swizzle[1];                       \
   const uint8_t swizzle_z = swizzle[2];                       \
   const uint8_t swizzle_w = swizzle[3];                       \
   const SRC_TYPE *typed_src = void_src;                       \
   DST_TYPE *typed_dst = void_dst;                             \
   DST_TYPE tmp[7];                                            \
   tmp[4] = 0;                                                 \
   tmp[5] = one;                                               \
   switch (num_dst_channels) {                                 \
   case 1:                                                     \
      switch (num_src_channels) {                              \
   case 1:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 1, CONV); \
      case 2:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 2, CONV); \
      case 3:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 3, CONV); \
      case 4:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 1, SRC_TYPE, 4, CONV); \
      }                                                        \
      case 2:                                                     \
      switch (num_src_channels) {                              \
   case 1:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 1, CONV); \
      case 2:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 2, CONV); \
      case 3:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 3, CONV); \
      case 4:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 2, SRC_TYPE, 4, CONV); \
      }                                                        \
      case 3:                                                     \
      switch (num_src_channels) {                              \
   case 1:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 1, CONV); \
      case 2:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 2, CONV); \
      case 3:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 3, CONV); \
      case 4:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 3, SRC_TYPE, 4, CONV); \
      }                                                        \
      case 4:                                                     \
      switch (num_src_channels) {                              \
   case 1:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 1, CONV); \
      case 2:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 2, CONV); \
      case 3:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 3, CONV); \
      case 4:                                                  \
      SWIZZLE_CONVERT_LOOP(DST_TYPE, 4, SRC_TYPE, 4, CONV); \
      }                                                        \
                  static void
   convert_float(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      SWIZZLE_CONVERT(float, float, src);
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      SWIZZLE_CONVERT(float, uint16_t, _mesa_half_to_float(src));
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
         static void
   convert_half_float(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      SWIZZLE_CONVERT(uint16_t, float, _mesa_float_to_half(src));
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      SWIZZLE_CONVERT(uint16_t, uint16_t, src);
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
      static void
   convert_ubyte(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      SWIZZLE_CONVERT(uint8_t, uint8_t, src);
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
         static void
   convert_byte(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      SWIZZLE_CONVERT(int8_t, int8_t, src);
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
         static void
   convert_ushort(void *void_dst, int num_dst_channels,
               {
      const uint16_t one = normalized ? UINT16_MAX : 1;
      switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      SWIZZLE_CONVERT(uint16_t, uint16_t, src);
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
         static void
   convert_short(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      SWIZZLE_CONVERT(int16_t, int16_t, src);
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
      static void
   convert_uint(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      SWIZZLE_CONVERT(uint32_t, uint32_t, src);
      case MESA_ARRAY_FORMAT_TYPE_INT:
      if (normalized) {
         } else {
         }
      default:
            }
         static void
   convert_int(void *void_dst, int num_dst_channels,
               {
               switch (src_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_HALF:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_BYTE:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_USHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_SHORT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_UINT:
      if (normalized) {
         } else {
         }
      case MESA_ARRAY_FORMAT_TYPE_INT:
      SWIZZLE_CONVERT(int32_t, int32_t, src);
      default:
            }
         /**
   * Convert between array-based color formats.
   *
   * Most format conversion operations required by GL can be performed by
   * converting one channel at a time, shuffling the channels around, and
   * optionally filling missing channels with zeros and ones.  This function
   * does just that in a general, yet efficient, way.
   *
   * The swizzle parameter is an array of 4 numbers (see
   * _mesa_get_format_swizzle) that describes where each channel in the
   * destination should come from in the source.  If swizzle[i] < 4 then it
   * means that dst[i] = CONVERT(src[swizzle[i]]).  If swizzle[i] is
   * MESA_FORMAT_SWIZZLE_ZERO or MESA_FORMAT_SWIZZLE_ONE, the corresponding
   * dst[i] will be filled with the appropreate representation of zero or one
   * respectively.
   *
   * Under most circumstances, the source and destination images must be
   * different as no care is taken not to clobber one with the other.
   * However, if they have the same number of bits per pixel, it is safe to
   * do an in-place conversion.
   *
   * \param[out] dst               pointer to where the converted data should
   *                               be stored
   *
   * \param[in]  dst_type          the destination GL type of the converted
   *                               data (GL_BYTE, etc.)
   *
   * \param[in]  num_dst_channels  the number of channels in the converted
   *                               data
   *
   * \param[in]  src               pointer to the source data
   *
   * \param[in]  src_type          the GL type of the source data (GL_BYTE,
   *                               etc.)
   *
   * \param[in]  num_src_channels  the number of channels in the source data
   *                               (the number of channels total, not just
   *                               the number used)
   *
   * \param[in]  swizzle           describes how to get the destination data
   *                               from the source data.
   *
   * \param[in]  normalized        for integer types, this indicates whether
   *                               the data should be considered as integers
   *                               or as normalized integers;
   *
   * \param[in]  count             the number of pixels to convert
   */
   void
   _mesa_swizzle_and_convert(void *void_dst, enum mesa_array_format_datatype dst_type, int num_dst_channels,
               {
      if (swizzle_convert_try_memcpy(void_dst, dst_type, num_dst_channels,
                        switch (dst_type) {
   case MESA_ARRAY_FORMAT_TYPE_FLOAT:
      convert_float(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_HALF:
      convert_half_float(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_UBYTE:
      convert_ubyte(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_BYTE:
      convert_byte(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_USHORT:
      convert_ushort(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_SHORT:
      convert_short(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_UINT:
      convert_uint(void_dst, num_dst_channels, void_src, src_type,
            case MESA_ARRAY_FORMAT_TYPE_INT:
      convert_int(void_dst, num_dst_channels, void_src, src_type,
            default:
            }
