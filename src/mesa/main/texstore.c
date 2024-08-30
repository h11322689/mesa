   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (c) 2008-2009  VMware, Inc.
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
      /*
   * Authors:
   *   Brian Paul
   */
      /**
   * The GL texture image functions in teximage.c basically just do
   * error checking and data structure allocation.  They in turn call
   * device driver functions which actually copy/convert/store the user's
   * texture image data.
   *
   * However, most device drivers will be able to use the fallback functions
   * in this file.  That is, most drivers will have the following bit of
   * code:
   *   ctx->Driver.TexImage = _mesa_store_teximage;
   *   ctx->Driver.TexSubImage = _mesa_store_texsubimage;
   *   etc...
   *
   * Texture image processing is actually kind of complicated.  We have to do:
   *    Format/type conversions
   *    pixel unpacking
   *    pixel transfer (scale, bais, lookup, etc)
   *
   * These functions can handle most everything, including processing full
   * images and sub-images.
   */
         #include "errors.h"
   #include "util/glheader.h"
   #include "bufferobj.h"
   #include "format_pack.h"
   #include "format_utils.h"
   #include "image.h"
   #include "macros.h"
   #include "mipmap.h"
   #include "mtypes.h"
   #include "pack.h"
   #include "pbo.h"
      #include "texcompress.h"
   #include "texcompress_fxt1.h"
   #include "texcompress_rgtc.h"
   #include "texcompress_s3tc.h"
   #include "texcompress_etc.h"
   #include "texcompress_bptc.h"
   #include "teximage.h"
   #include "texstore.h"
   #include "enums.h"
   #include "glformats.h"
   #include "pixeltransfer.h"
   #include "util/format_rgb9e5.h"
   #include "util/format_r11g11b10f.h"
      #include "state_tracker/st_cb_texture.h"
      enum {
      ZERO = 4,
      };
         /**
   * Texture image storage function.
   */
   typedef GLboolean (*StoreTexImageFunc)(TEXSTORE_PARAMS);
         /**
   * Teximage storage routine for when a simple memcpy will do.
   * No pixel transfer operations or special texel encodings allowed.
   * 1D, 2D and 3D images supported.
   */
   void
   _mesa_memcpy_texture(struct gl_context *ctx,
                        GLuint dimensions,
   mesa_format dstFormat,
   GLint dstRowStride,
   GLubyte **dstSlices,
      {
      const GLint srcRowStride = _mesa_image_row_stride(srcPacking, srcWidth,
         const intptr_t srcImageStride = _mesa_image_image_stride(srcPacking,
         const GLubyte *srcImage = (const GLubyte *) _mesa_image_address(dimensions,
         const GLuint texelBytes = _mesa_get_format_bytes(dstFormat);
            if (dstRowStride == srcRowStride &&
      dstRowStride == bytesPerRow) {
   /* memcpy image by image */
   GLint img;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstImage = dstSlices[img];
   memcpy(dstImage, srcImage, bytesPerRow * srcHeight);
         }
   else {
      /* memcpy row by row */
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      const GLubyte *srcRow = srcImage;
   GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      memcpy(dstRow, srcRow, bytesPerRow);
   dstRow += dstRowStride;
      }
            }
         /**
   * Store a 32-bit integer or float depth component texture image.
   */
   static GLboolean
   _mesa_texstore_z32(TEXSTORE_PARAMS)
   {
      const GLuint depthScale = 0xffffffff;
   GLenum dstType;
   (void) dims;
   assert(dstFormat == MESA_FORMAT_Z_UNORM32 ||
                  if (dstFormat == MESA_FORMAT_Z_UNORM32)
         else
            {
      /* general path */
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      const GLvoid *src = _mesa_image_address(dims, srcPacking,
         _mesa_unpack_depth_span(ctx, srcWidth,
                        }
      }
         /**
   * Store a 24-bit integer depth component texture image.
   */
   static GLboolean
   _mesa_texstore_x8_z24(TEXSTORE_PARAMS)
   {
               (void) dims;
            {
      /* general path */
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      const GLvoid *src = _mesa_image_address(dims, srcPacking,
         _mesa_unpack_depth_span(ctx, srcWidth,
                        }
      }
         /**
   * Store a 24-bit integer depth component texture image.
   */
   static GLboolean
   _mesa_texstore_z24_x8(TEXSTORE_PARAMS)
   {
               (void) dims;
            {
      /* general path */
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      const GLvoid *src = _mesa_image_address(dims, srcPacking,
         GLuint *dst = (GLuint *) dstRow;
   GLint i;
   _mesa_unpack_depth_span(ctx, srcWidth,
               for (i = 0; i < srcWidth; i++)
                  }
      }
         /**
   * Store a 16-bit integer depth component texture image.
   */
   static GLboolean
   _mesa_texstore_z16(TEXSTORE_PARAMS)
   {
      const GLuint depthScale = 0xffff;
   (void) dims;
   assert(dstFormat == MESA_FORMAT_Z_UNORM16);
            {
      /* general path */
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      const GLvoid *src = _mesa_image_address(dims, srcPacking,
         GLushort *dst16 = (GLushort *) dstRow;
   _mesa_unpack_depth_span(ctx, srcWidth,
                        }
      }
         /**
   * Texstore for _mesa_texformat_ycbcr or _mesa_texformat_ycbcr_REV.
   */
   static GLboolean
   _mesa_texstore_ycbcr(TEXSTORE_PARAMS)
   {
               assert((dstFormat == MESA_FORMAT_YCBCR) ||
         assert(_mesa_get_format_bytes(dstFormat) == 2);
   assert(ctx->Extensions.MESA_ycbcr_texture);
   assert(srcFormat == GL_YCBCR_MESA);
   assert((srcType == GL_UNSIGNED_SHORT_8_8_MESA) ||
                  /* always just memcpy since no pixel transfer ops apply */
   _mesa_memcpy_texture(ctx, dims,
                              /* Check if we need byte swapping */
   /* XXX the logic here _might_ be wrong */
   if (srcPacking->SwapBytes ^
      (srcType == GL_UNSIGNED_SHORT_8_8_REV_MESA) ^
   (dstFormat == MESA_FORMAT_YCBCR_REV) ^
   !UTIL_ARCH_LITTLE_ENDIAN) {
   GLint img, row;
   for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   for (row = 0; row < srcHeight; row++) {
      _mesa_swap2((GLushort *) dstRow, srcWidth);
            }
      }
         /**
   * Store a combined depth/stencil texture image.
   */
   static GLboolean
   _mesa_texstore_z24_s8(TEXSTORE_PARAMS)
   {
      const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
         GLint img, row;
   GLuint *depth = malloc(srcWidth * sizeof(GLuint));
            assert(dstFormat == MESA_FORMAT_S8_UINT_Z24_UNORM);
   assert(srcFormat == GL_DEPTH_STENCIL_EXT ||
         srcFormat == GL_DEPTH_COMPONENT ||
   assert(srcFormat != GL_DEPTH_STENCIL_EXT ||
                  if (!depth || !stencil) {
      free(depth);
   free(stencil);
               /*
   * The spec "8.5. TEXTURE IMAGE SPECIFICATION" says:
   *
   *    If the base internal format is DEPTH_STENCIL and format is not DEPTH_STENCIL,
   *    then the values of the stencil index texture components are undefined.
   *
   * but there doesn't seem to be corresponding text saying that depth is
   * undefined when a stencil format is supplied.
   */
            /* In case we only upload depth we need to preserve the stencil */
   for (img = 0; img < srcDepth; img++) {
      GLuint *dstRow = (GLuint *) dstSlices[img];
   const GLubyte *src
      = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
         srcWidth, srcHeight,
      for (row = 0; row < srcHeight; row++) {
               if (!keepdepth)
      /* the 24 depth bits will be in the low position: */
   _mesa_unpack_depth_span(ctx, srcWidth,
                           if (srcFormat != GL_DEPTH_COMPONENT)
      /* get the 8-bit stencil values */
   _mesa_unpack_stencil_span(ctx, srcWidth,
                           for (i = 0; i < srcWidth; i++) {
      if (keepdepth)
         else
      }
   src += srcRowStride;
                  free(depth);
   free(stencil);
      }
         /**
   * Store a combined depth/stencil texture image.
   */
   static GLboolean
   _mesa_texstore_s8_z24(TEXSTORE_PARAMS)
   {
      const GLuint depthScale = 0xffffff;
   const GLint srcRowStride
         GLint img, row;
   GLuint *depth;
            assert(dstFormat == MESA_FORMAT_Z24_UNORM_S8_UINT);
   assert(srcFormat == GL_DEPTH_STENCIL_EXT ||
         srcFormat == GL_DEPTH_COMPONENT ||
   assert(srcFormat != GL_DEPTH_STENCIL_EXT ||
                  depth = malloc(srcWidth * sizeof(GLuint));
            if (!depth || !stencil) {
      free(depth);
   free(stencil);
               /*
   * The spec "8.5. TEXTURE IMAGE SPECIFICATION" says:
   *
   *    If the base internal format is DEPTH_STENCIL and format is not DEPTH_STENCIL,
   *    then the values of the stencil index texture components are undefined.
   *
   * but there doesn't seem to be corresponding text saying that depth is
   * undefined when a stencil format is supplied.
   */
            for (img = 0; img < srcDepth; img++) {
      GLuint *dstRow = (GLuint *) dstSlices[img];
   const GLubyte *src
      = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                     for (row = 0; row < srcHeight; row++) {
               if (!keepdepth)
      /* the 24 depth bits will be in the low position: */
   _mesa_unpack_depth_span(ctx, srcWidth,
                           if (srcFormat != GL_DEPTH_COMPONENT)
      /* get the 8-bit stencil values */
   _mesa_unpack_stencil_span(ctx, srcWidth,
                           /* merge stencil values into depth values */
   for (i = 0; i < srcWidth; i++) {
      if (keepdepth)
         else
               src += srcRowStride;
                  free(depth);
               }
         /**
   * Store simple 8-bit/value stencil texture data.
   */
   static GLboolean
   _mesa_texstore_s8(TEXSTORE_PARAMS)
   {
      assert(dstFormat == MESA_FORMAT_S_UINT8);
            {
      const GLint srcRowStride
         GLint img, row;
            if (!stencil)
            for (img = 0; img < srcDepth; img++) {
      GLubyte *dstRow = dstSlices[img];
   const GLubyte *src
      = (const GLubyte *) _mesa_image_address(dims, srcPacking, srcAddr,
                                    /* get the 8-bit stencil values */
   _mesa_unpack_stencil_span(ctx, srcWidth,
                           /* merge stencil values into depth values */
                  src += srcRowStride;
                                 }
         static GLboolean
   _mesa_texstore_z32f_x24s8(TEXSTORE_PARAMS)
   {
      GLint img, row;
   const GLint srcRowStride
      = _mesa_image_row_stride(srcPacking, srcWidth, srcFormat, srcType)
         assert(dstFormat == MESA_FORMAT_Z32_FLOAT_S8X24_UINT);
   assert(srcFormat == GL_DEPTH_STENCIL ||
         srcFormat == GL_DEPTH_COMPONENT ||
   assert(srcFormat != GL_DEPTH_STENCIL ||
                  /* In case we only upload depth we need to preserve the stencil */
   for (img = 0; img < srcDepth; img++) {
      uint64_t *dstRow = (uint64_t *) dstSlices[img];
   const int32_t *src
      = (const int32_t *) _mesa_image_address(dims, srcPacking, srcAddr,
         srcWidth, srcHeight,
      for (row = 0; row < srcHeight; row++) {
      /* The unpack functions with:
   *    dstType = GL_FLOAT_32_UNSIGNED_INT_24_8_REV
   * only write their own dword, so the other dword (stencil
   * or depth) is preserved. */
   if (srcFormat != GL_STENCIL_INDEX)
      _mesa_unpack_depth_span(ctx, srcWidth,
                     if (srcFormat != GL_DEPTH_COMPONENT)
      _mesa_unpack_stencil_span(ctx, srcWidth,
                           src += srcRowStride;
         }
      }
      static GLboolean
   texstore_depth_stencil(TEXSTORE_PARAMS)
   {
      static StoreTexImageFunc table[MESA_FORMAT_COUNT];
            if (!initialized) {
               table[MESA_FORMAT_S8_UINT_Z24_UNORM] = _mesa_texstore_z24_s8;
   table[MESA_FORMAT_Z24_UNORM_S8_UINT] = _mesa_texstore_s8_z24;
   table[MESA_FORMAT_Z_UNORM16] = _mesa_texstore_z16;
   table[MESA_FORMAT_Z24_UNORM_X8_UINT] = _mesa_texstore_x8_z24;
   table[MESA_FORMAT_X8_UINT_Z24_UNORM] = _mesa_texstore_z24_x8;
   table[MESA_FORMAT_Z_UNORM32] = _mesa_texstore_z32;
   table[MESA_FORMAT_S_UINT8] = _mesa_texstore_s8;
   table[MESA_FORMAT_Z_FLOAT32] = _mesa_texstore_z32;
                        assert(table[dstFormat]);
   return table[dstFormat](ctx, dims, baseInternalFormat,
                  }
      static GLboolean
   texstore_compressed(TEXSTORE_PARAMS)
   {
      static StoreTexImageFunc table[MESA_FORMAT_COUNT];
            if (!initialized) {
               table[MESA_FORMAT_SRGB_DXT1] = _mesa_texstore_rgb_dxt1;
   table[MESA_FORMAT_SRGBA_DXT1] = _mesa_texstore_rgba_dxt1;
   table[MESA_FORMAT_SRGBA_DXT3] = _mesa_texstore_rgba_dxt3;
   table[MESA_FORMAT_SRGBA_DXT5] = _mesa_texstore_rgba_dxt5;
   table[MESA_FORMAT_RGB_FXT1] = _mesa_texstore_fxt1;
   table[MESA_FORMAT_RGBA_FXT1] = _mesa_texstore_fxt1;
   table[MESA_FORMAT_RGB_DXT1] = _mesa_texstore_rgb_dxt1;
   table[MESA_FORMAT_RGBA_DXT1] = _mesa_texstore_rgba_dxt1;
   table[MESA_FORMAT_RGBA_DXT3] = _mesa_texstore_rgba_dxt3;
   table[MESA_FORMAT_RGBA_DXT5] = _mesa_texstore_rgba_dxt5;
   table[MESA_FORMAT_R_RGTC1_UNORM] = _mesa_texstore_red_rgtc1;
   table[MESA_FORMAT_R_RGTC1_SNORM] = _mesa_texstore_signed_red_rgtc1;
   table[MESA_FORMAT_RG_RGTC2_UNORM] = _mesa_texstore_rg_rgtc2;
   table[MESA_FORMAT_RG_RGTC2_SNORM] = _mesa_texstore_signed_rg_rgtc2;
   table[MESA_FORMAT_L_LATC1_UNORM] = _mesa_texstore_red_rgtc1;
   table[MESA_FORMAT_L_LATC1_SNORM] = _mesa_texstore_signed_red_rgtc1;
   table[MESA_FORMAT_LA_LATC2_UNORM] = _mesa_texstore_rg_rgtc2;
   table[MESA_FORMAT_LA_LATC2_SNORM] = _mesa_texstore_signed_rg_rgtc2;
   table[MESA_FORMAT_ETC1_RGB8] = _mesa_texstore_etc1_rgb8;
   table[MESA_FORMAT_ETC2_RGB8] = _mesa_texstore_etc2_rgb8;
   table[MESA_FORMAT_ETC2_SRGB8] = _mesa_texstore_etc2_srgb8;
   table[MESA_FORMAT_ETC2_RGBA8_EAC] = _mesa_texstore_etc2_rgba8_eac;
   table[MESA_FORMAT_ETC2_SRGB8_ALPHA8_EAC] = _mesa_texstore_etc2_srgb8_alpha8_eac;
   table[MESA_FORMAT_ETC2_R11_EAC] = _mesa_texstore_etc2_r11_eac;
   table[MESA_FORMAT_ETC2_RG11_EAC] = _mesa_texstore_etc2_rg11_eac;
   table[MESA_FORMAT_ETC2_SIGNED_R11_EAC] = _mesa_texstore_etc2_signed_r11_eac;
   table[MESA_FORMAT_ETC2_SIGNED_RG11_EAC] = _mesa_texstore_etc2_signed_rg11_eac;
   table[MESA_FORMAT_ETC2_RGB8_PUNCHTHROUGH_ALPHA1] =
         table[MESA_FORMAT_ETC2_SRGB8_PUNCHTHROUGH_ALPHA1] =
            table[MESA_FORMAT_BPTC_RGBA_UNORM] =
         table[MESA_FORMAT_BPTC_SRGB_ALPHA_UNORM] =
         table[MESA_FORMAT_BPTC_RGB_SIGNED_FLOAT] =
         table[MESA_FORMAT_BPTC_RGB_UNSIGNED_FLOAT] =
                        assert(table[dstFormat]);
   return table[dstFormat](ctx, dims, baseInternalFormat,
                  }
      static GLboolean
   texstore_rgba(TEXSTORE_PARAMS)
   {
      void *tempImage = NULL;
   int img;
   GLubyte *src, *dst;
   uint8_t rebaseSwizzle[4];
            /* We have to handle MESA_FORMAT_YCBCR manually because it is a special case
   * and _mesa_format_convert does not support it. In this case the we only
   * allow conversions between YCBCR formats and it is mostly a memcpy.
   */
   if (dstFormat == MESA_FORMAT_YCBCR || dstFormat == MESA_FORMAT_YCBCR_REV) {
      return _mesa_texstore_ycbcr(ctx, dims, baseInternalFormat,
                                 /* We have to deal with GL_COLOR_INDEX manually because
   * _mesa_format_convert does not handle this format. So what we do here is
   * convert it to RGBA ubyte first and then convert from that to dst as usual.
   */
   if (srcFormat == GL_COLOR_INDEX) {
      /* Notice that this will already handle byte swapping if necessary */
   tempImage =
      _mesa_unpack_color_index_to_rgba_ubyte(ctx, dims,
                        if (!tempImage)
            /* _mesa_unpack_color_index_to_rgba_ubyte has handled transferops
   * if needed.
   */
            /* Now we only have to adjust our src info for a conversion from
   * the RGBA ubyte and then we continue as usual.
   */
   srcAddr = tempImage;
   srcFormat = GL_RGBA;
      } else if (srcPacking->SwapBytes) {
      /* We have to handle byte-swapping scenarios before calling
   * _mesa_format_convert
   */
   GLint swapSize = _mesa_sizeof_packed_type(srcType);
   if (swapSize == 2 || swapSize == 4) {
      intptr_t imageStride = _mesa_image_image_stride(srcPacking, srcWidth,
               int bufferSize = imageStride * srcDepth;
   int layer;
                  tempImage = malloc(bufferSize);
   if (!tempImage)
         src = srcAddr;
   dst = tempImage;
   for (layer = 0; layer < srcDepth; layer++) {
      _mesa_swap_bytes_2d_image(srcFormat, srcType,
                     src += imageStride;
      }
                  int srcRowStride =
            uint32_t srcMesaFormat =
                     /* If we have transferOps then we need to convert to RGBA float first,
         */
   void *tempRGBA = NULL;
   if (!transferOpsDone &&
      _mesa_texstore_needs_transfer_ops(ctx, baseInternalFormat, dstFormat)) {
   /* Allocate RGBA float image */
   int elementCount = srcWidth * srcHeight * srcDepth;
   tempRGBA = malloc(4 * elementCount * sizeof(float));
   if (!tempRGBA) {
      free(tempImage);
               /* Convert from src to RGBA float */
   src = (GLubyte *) srcAddr;
   dst = (GLubyte *) tempRGBA;
   for (img = 0; img < srcDepth; img++) {
      _mesa_format_convert(dst, RGBA32_FLOAT, 4 * srcWidth * sizeof(float),
               src += srcHeight * srcRowStride;
               /* Apply transferOps */
   _mesa_apply_rgba_transfer_ops(ctx, ctx->_ImageTransferState, elementCount,
            /* Now we have to adjust our src info for a conversion from
   * the RGBA float image and then we continue as usual.
   */
   srcAddr = tempRGBA;
   srcFormat = GL_RGBA;
   srcType = GL_FLOAT;
   srcRowStride = srcWidth * 4 * sizeof(float);
   srcMesaFormat = RGBA32_FLOAT;
               src = (GLubyte *)
      _mesa_image_address(dims, srcPacking, srcAddr, srcWidth, srcHeight,
         bool needRebase;
   if (_mesa_get_format_base_format(dstFormat) != baseInternalFormat) {
      needRebase =
      _mesa_compute_rgba2base2rgba_component_mapping(baseInternalFormat,
   } else {
                  for (img = 0; img < srcDepth; img++) {
      _mesa_format_convert(dstSlices[img], dstFormat, dstRowStride,
                                 free(tempImage);
               }
      GLboolean
   _mesa_texstore_needs_transfer_ops(struct gl_context *ctx,
               {
               /* There are different rules depending on the base format. */
   switch (baseInternalFormat) {
   case GL_DEPTH_COMPONENT:
   case GL_DEPTH_STENCIL:
      return ctx->Pixel.DepthScale != 1.0f ||
         case GL_STENCIL_INDEX:
            default:
      /* Color formats.
   * Pixel transfer ops (scale, bias, table lookup) do not apply
   * to integer formats.
   */
            return dstType != GL_INT && dstType != GL_UNSIGNED_INT &&
         }
         GLboolean
   _mesa_texstore_can_use_memcpy(struct gl_context *ctx,
                     {
      if (_mesa_texstore_needs_transfer_ops(ctx, baseInternalFormat, dstFormat)) {
                  /* The base internal format and the base Mesa format must match. */
   if (baseInternalFormat != _mesa_get_format_base_format(dstFormat)) {
                  /* The Mesa format must match the input format and type. */
   if (!_mesa_format_matches_format_and_type(dstFormat, srcFormat, srcType,
                        /* Depth texture data needs clamping in following cases:
   * - Floating point dstFormat with signed srcType: clamp to [0.0, 1.0].
   * - Fixed point dstFormat with signed srcType: clamp to [0, 2^n -1].
   *
   * All the cases except one (float dstFormat with float srcType) are ruled
   * out by _mesa_format_matches_format_and_type() check above. Handle the
   * remaining case here.
   */
   if ((baseInternalFormat == GL_DEPTH_COMPONENT ||
      baseInternalFormat == GL_DEPTH_STENCIL) &&
   (srcType == GL_FLOAT ||
   srcType == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)) {
                  }
      static GLboolean
   _mesa_texstore_memcpy(TEXSTORE_PARAMS)
   {
      if (!_mesa_texstore_can_use_memcpy(ctx, baseInternalFormat, dstFormat,
                        _mesa_memcpy_texture(ctx, dims,
                              }
         /**
   * Store user data into texture memory.
   * Called via glTex[Sub]Image1/2/3D()
   * \return GL_TRUE for success, GL_FALSE for failure (out of memory).
   */
   GLboolean
   _mesa_texstore(TEXSTORE_PARAMS)
   {
      if (_mesa_texstore_memcpy(ctx, dims, baseInternalFormat,
                                          if (_mesa_is_depth_or_stencil_format(baseInternalFormat)) {
      return texstore_depth_stencil(ctx, dims, baseInternalFormat,
                  } else if (_mesa_is_format_compressed(dstFormat)) {
      return texstore_compressed(ctx, dims, baseInternalFormat,
                  } else {
      return texstore_rgba(ctx, dims, baseInternalFormat,
                     }
         /**
   * Normally, we'll only _write_ texel data to a texture when we map it.
   * But if the user is providing depth or stencil values and the texture
   * image is a combined depth/stencil format, we'll actually read from
   * the texture buffer too (in order to insert the depth or stencil values.
   * \param userFormat  the user-provided image format
   * \param texFormat  the destination texture format
   */
   static GLbitfield
   get_read_write_mode(GLenum userFormat, mesa_format texFormat)
   {
      if ((userFormat == GL_STENCIL_INDEX || userFormat == GL_DEPTH_COMPONENT)
      && _mesa_get_format_base_format(texFormat) == GL_DEPTH_STENCIL)
      else
      }
         /**
   * Helper function for storing 1D, 2D, 3D whole and subimages into texture
   * memory.
   * The source of the image data may be user memory or a PBO.  In the later
   * case, we'll map the PBO, copy from it, then unmap it.
   */
   static void
   store_texsubimage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
   GLint xoffset, GLint yoffset, GLint zoffset,
   GLint width, GLint height, GLint depth,
      {
      const GLbitfield mapMode = get_read_write_mode(format, texImage->TexFormat);
   const GLenum target = texImage->TexObject->Target;
   GLboolean success = GL_FALSE;
   GLuint dims, slice, numSlices = 1, sliceOffset = 0;
   intptr_t srcImageStride = 0;
            assert(xoffset + width <= texImage->Width);
   assert(yoffset + height <= texImage->Height);
            switch (target) {
   case GL_TEXTURE_1D:
      dims = 1;
      case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_3D:
      dims = 3;
      default:
                  /* get pointer to src pixels (may be in a pbo which we'll map here) */
   src = (const GLubyte *)
      _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
      if (!src)
            /* compute slice info (and do some sanity checks) */
   switch (target) {
   case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_RECTANGLE:
   case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_EXTERNAL_OES:
      /* one image slice, nothing special needs to be done */
      case GL_TEXTURE_1D:
      assert(height == 1);
   assert(depth == 1);
   assert(yoffset == 0);
   assert(zoffset == 0);
      case GL_TEXTURE_1D_ARRAY:
      assert(depth == 1);
   assert(zoffset == 0);
   numSlices = height;
   sliceOffset = yoffset;
   height = 1;
   yoffset = 0;
   srcImageStride = _mesa_image_row_stride(packing, width, format, type);
      case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      numSlices = depth;
   sliceOffset = zoffset;
   depth = 1;
   zoffset = 0;
   srcImageStride = _mesa_image_image_stride(packing, width, height,
            case GL_TEXTURE_3D:
      /* we'll store 3D images as a series of slices */
   numSlices = depth;
   sliceOffset = zoffset;
   srcImageStride = _mesa_image_image_stride(packing, width, height,
            case GL_TEXTURE_CUBE_MAP_ARRAY:
      numSlices = depth;
   sliceOffset = zoffset;
   srcImageStride = _mesa_image_image_stride(packing, width, height,
            default:
      _mesa_warning(ctx, "Unexpected target 0x%x in store_texsubimage()",
                              for (slice = 0; slice < numSlices; slice++) {
      GLubyte *dstMap;
            st_MapTextureImage(ctx, texImage,
                     if (dstMap) {
      /* Note: we're only storing a 2D (or 1D) slice at a time but we need
   * to pass the right 'dims' value so that GL_UNPACK_SKIP_IMAGES is
   * used for 3D images.
   */
   success = _mesa_texstore(ctx, dims, texImage->_BaseFormat,
                                                         if (!success)
               if (!success)
               }
            /**
   * Fallback code for TexImage().
   * Basically, allocate storage for the texture image, then copy the
   * user's image into it.
   */
   void
   _mesa_store_teximage(struct gl_context *ctx,
                           {
               if (texImage->Width == 0 || texImage->Height == 0 || texImage->Depth == 0)
            /* allocate storage for texture data */
   if (!st_AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD", dims);
               store_texsubimage(ctx, texImage,
            }
         /*
   * Fallback for Driver.TexSubImage().
   */
   void
   _mesa_store_texsubimage(struct gl_context *ctx, GLuint dims,
                           struct gl_texture_image *texImage,
   {
      store_texsubimage(ctx, texImage,
            }
      static void
   clear_image_to_zero(GLubyte *dstMap, GLint dstRowStride,
               {
               for (y = 0; y < height; y++) {
      memset(dstMap, 0, clearValueSize * width);
         }
      static void
   clear_image_to_value(GLubyte *dstMap, GLint dstRowStride,
                     {
               for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
      memcpy(dstMap, clearValue, clearValueSize);
      }
         }
      /*
   * Fallback for Driver.ClearTexSubImage().
   */
   void
   _mesa_store_cleartexsubimage(struct gl_context *ctx,
                           {
      GLubyte *dstMap;
   GLint dstRowStride;
   GLsizeiptr clearValueSize;
                     for (z = 0; z < depth; z++) {
      st_MapTextureImage(ctx, texImage,
                     z + zoffset, xoffset, yoffset,
   if (dstMap == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glClearTex*Image");
               if (clearValue) {
      clear_image_to_value(dstMap, dstRowStride,
                  } else {
      clear_image_to_zero(dstMap, dstRowStride,
                           }
      /**
   * Fallback for Driver.CompressedTexImage()
   */
   void
   _mesa_store_compressed_teximage(struct gl_context *ctx, GLuint dims,
               {
      /* only 2D and 3D compressed images are supported at this time */
   if (dims == 1) {
      _mesa_problem(ctx, "Unexpected glCompressedTexImage1D call");
               /* This is pretty simple, because unlike the general texstore path we don't
   * have to worry about the usual image unpacking or image transfer
   * operations.
   */
   assert(texImage);
   assert(texImage->Width > 0);
   assert(texImage->Height > 0);
            /* allocate storage for texture data */
   if (!st_AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexImage%uD", dims);
               st_CompressedTexSubImage(ctx, dims, texImage,
                        }
         /**
   * Compute compressed_pixelstore parameters for copying compressed
   * texture data.
   * \param dims  number of texture image dimensions: 1, 2 or 3
   * \param texFormat  the compressed texture format
   * \param width, height, depth  size of image to copy
   * \param packing  pixelstore parameters describing user-space image packing
   * \param store  returns the compressed_pixelstore parameters
   */
   void
   _mesa_compute_compressed_pixelstore(GLuint dims, mesa_format texFormat,
                           {
                        store->SkipBytes = 0;
   store->TotalBytesPerRow = store->CopyBytesPerRow =
         store->TotalRowsPerSlice = store->CopyRowsPerSlice =
                  if (packing->CompressedBlockWidth &&
                        if (packing->RowLength) {
      store->TotalBytesPerRow = packing->CompressedBlockSize *
               store->SkipBytes +=
               if (dims > 1 && packing->CompressedBlockHeight &&
                        store->SkipBytes += packing->SkipRows * store->TotalBytesPerRow / bh;
            if (packing->ImageHeight) {
                     if (dims > 2 && packing->CompressedBlockDepth &&
                        store->SkipBytes += packing->SkipImages * store->TotalBytesPerRow *
         }
         /**
   * Fallback for Driver.CompressedTexSubImage()
   */
   void
   _mesa_store_compressed_texsubimage(struct gl_context *ctx, GLuint dims,
                                 {
      struct compressed_pixelstore store;
   GLint dstRowStride;
   GLint i, slice;
   GLubyte *dstMap;
            if (dims == 1) {
      _mesa_problem(ctx, "Unexpected 1D compressed texsubimage call");
               _mesa_compute_compressed_pixelstore(dims, texImage->TexFormat,
                  /* get pointer to src pixels (may be in a pbo which we'll map here) */
   data = _mesa_validate_pbo_compressed_teximage(ctx, dims, imageSize, data,
               if (!data)
                     for (slice = 0; slice < store.CopySlices; slice++) {
      /* Map dest texture buffer */
   st_MapTextureImage(ctx, texImage, slice + zoffset,
                                    /* copy rows of blocks */
   if (dstRowStride == store.TotalBytesPerRow &&
      dstRowStride == store.CopyBytesPerRow) {
   memcpy(dstMap, src, store.CopyBytesPerRow * store.CopyRowsPerSlice);
      }
   else {
      for (i = 0; i < store.CopyRowsPerSlice; i++) {
      memcpy(dstMap, src, store.CopyBytesPerRow);
   dstMap += dstRowStride;
                           /* advance to next slice */
   src += store.TotalBytesPerRow * (store.TotalRowsPerSlice
      }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCompressedTexSubImage%uD",
                     }
