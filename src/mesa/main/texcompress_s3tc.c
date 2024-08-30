   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
   * Copyright (c) 2008 VMware, Inc.
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
         /**
   * \file texcompress_s3tc.c
   * GL_EXT_texture_compression_s3tc support.
   */
      #include "util/glheader.h"
      #include "image.h"
   #include "macros.h"
   #include "mtypes.h"
   #include "texcompress.h"
   #include "texcompress_s3tc.h"
   #include "util/format/texcompress_s3tc_tmp.h"
   #include "texstore.h"
   #include "format_unpack.h"
   #include "util/format_srgb.h"
   #include "util/format/u_format_s3tc.h"
         /**
   * Store user's image in rgb_dxt1 format.
   */
   GLboolean
   _mesa_texstore_rgb_dxt1(TEXSTORE_PARAMS)
   {
      const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
            assert(dstFormat == MESA_FORMAT_RGB_DXT1 ||
            if (!(srcFormat == GL_RGB || srcFormat == GL_RGBA) ||
      srcType != GL_UNSIGNED_BYTE ||
   ctx->_ImageTransferState ||
   _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType) != srccomps * srcWidth * sizeof(GLubyte) ||
   srcPacking->SkipImages ||
   srcPacking->SwapBytes) {
   /* convert image to RGB/GLubyte */
   GLubyte *tempImageSlices[1];
   int rgbRowStride = 3 * srcWidth * sizeof(GLubyte);
   tempImage = malloc(srcWidth * srcHeight * 3 * sizeof(GLubyte));
   if (!tempImage)
         tempImageSlices[0] = (GLubyte *) tempImage;
   _mesa_texstore(ctx, dims,
                  baseInternalFormat,
   MESA_FORMAT_RGB_UNORM8,
   rgbRowStride, tempImageSlices,
      pixels = tempImage;
   srcFormat = GL_RGB;
      }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                        tx_compress_dxt1(srccomps, srcWidth, srcHeight, pixels,
                        }
         /**
   * Store user's image in rgba_dxt1 format.
   */
   GLboolean
   _mesa_texstore_rgba_dxt1(TEXSTORE_PARAMS)
   {
      const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
            assert(dstFormat == MESA_FORMAT_RGBA_DXT1 ||
            if (srcFormat != GL_RGBA ||
      srcType != GL_UNSIGNED_BYTE ||
   ctx->_ImageTransferState ||
   _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType) != rgbaRowStride ||
   srcPacking->SkipImages ||
   srcPacking->SwapBytes) {
   /* convert image to RGBA/GLubyte */
   GLubyte *tempImageSlices[1];
   tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
   if (!tempImage)
         tempImageSlices[0] = (GLubyte *) tempImage;
   _mesa_texstore(ctx, dims,
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                        rgbaRowStride, tempImageSlices,
      pixels = tempImage;
      }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                             }
         /**
   * Store user's image in rgba_dxt3 format.
   */
   GLboolean
   _mesa_texstore_rgba_dxt3(TEXSTORE_PARAMS)
   {
      const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
            assert(dstFormat == MESA_FORMAT_RGBA_DXT3 ||
            if (srcFormat != GL_RGBA ||
      srcType != GL_UNSIGNED_BYTE ||
   ctx->_ImageTransferState ||
   _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType) != rgbaRowStride ||
   srcPacking->SkipImages ||
   srcPacking->SwapBytes) {
   /* convert image to RGBA/GLubyte */
   GLubyte *tempImageSlices[1];
   tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
   if (!tempImage)
         tempImageSlices[0] = (GLubyte *) tempImage;
   _mesa_texstore(ctx, dims,
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                        rgbaRowStride, tempImageSlices,
         }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                             }
         /**
   * Store user's image in rgba_dxt5 format.
   */
   GLboolean
   _mesa_texstore_rgba_dxt5(TEXSTORE_PARAMS)
   {
      const GLubyte *pixels;
   GLubyte *dst;
   const GLubyte *tempImage = NULL;
            assert(dstFormat == MESA_FORMAT_RGBA_DXT5 ||
            if (srcFormat != GL_RGBA ||
      srcType != GL_UNSIGNED_BYTE ||
   ctx->_ImageTransferState ||
   _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType) != rgbaRowStride ||
   srcPacking->SkipImages ||
   srcPacking->SwapBytes) {
   /* convert image to RGBA/GLubyte */
   GLubyte *tempImageSlices[1];
   tempImage = malloc(srcWidth * srcHeight * 4 * sizeof(GLubyte));
   if (!tempImage)
         tempImageSlices[0] = (GLubyte *) tempImage;
   _mesa_texstore(ctx, dims,
   #if UTIL_ARCH_LITTLE_ENDIAN
         #else
         #endif
                        rgbaRowStride, tempImageSlices,
         }
   else {
      pixels = _mesa_image_address2d(srcPacking, srcAddr, srcWidth, srcHeight,
                                             }
         static void
   fetch_rgb_dxt1(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgb_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      }
      static void
   fetch_rgba_dxt1(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      }
      static void
   fetch_rgba_dxt3(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt3(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      }
      static void
   fetch_rgba_dxt5(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt5(rowStride, map, i, j, tex);
   texel[RCOMP] = UBYTE_TO_FLOAT(tex[RCOMP]);
   texel[GCOMP] = UBYTE_TO_FLOAT(tex[GCOMP]);
   texel[BCOMP] = UBYTE_TO_FLOAT(tex[BCOMP]);
      }
         static void
   fetch_srgb_dxt1(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgb_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      }
      static void
   fetch_srgba_dxt1(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt1(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      }
      static void
   fetch_srgba_dxt3(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt3(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      }
      static void
   fetch_srgba_dxt5(const GLubyte *map,
         {
      GLubyte tex[4];
   fetch_2d_texel_rgba_dxt5(rowStride, map, i, j, tex);
   texel[RCOMP] = util_format_srgb_8unorm_to_linear_float(tex[RCOMP]);
   texel[GCOMP] = util_format_srgb_8unorm_to_linear_float(tex[GCOMP]);
   texel[BCOMP] = util_format_srgb_8unorm_to_linear_float(tex[BCOMP]);
      }
            compressed_fetch_func
   _mesa_get_dxt_fetch_func(mesa_format format)
   {
      switch (format) {
   case MESA_FORMAT_RGB_DXT1:
         case MESA_FORMAT_RGBA_DXT1:
         case MESA_FORMAT_RGBA_DXT3:
         case MESA_FORMAT_RGBA_DXT5:
         case MESA_FORMAT_SRGB_DXT1:
         case MESA_FORMAT_SRGBA_DXT1:
         case MESA_FORMAT_SRGBA_DXT3:
         case MESA_FORMAT_SRGBA_DXT5:
         default:
            }
      extern void
   _mesa_unpack_s3tc(uint8_t *dst_row,
                     unsigned dst_stride,
   const uint8_t *src_row,
   unsigned src_stride,
   {
      /* We treat sRGB formats as RGB, because we're unpacking to another sRGB
   * format.
   */
   switch (format) {
   case MESA_FORMAT_RGB_DXT1:
   case MESA_FORMAT_SRGB_DXT1:
      util_format_dxt1_rgb_unpack_rgba_8unorm(dst_row, dst_stride,
                     case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_SRGBA_DXT1:
      util_format_dxt1_rgba_unpack_rgba_8unorm(dst_row, dst_stride,
                     case MESA_FORMAT_RGBA_DXT3:
   case MESA_FORMAT_SRGBA_DXT3:
      util_format_dxt3_rgba_unpack_rgba_8unorm(dst_row, dst_stride,
                     case MESA_FORMAT_RGBA_DXT5:
   case MESA_FORMAT_SRGBA_DXT5:
      util_format_dxt5_rgba_unpack_rgba_8unorm(dst_row, dst_stride,
                     default:
            }
