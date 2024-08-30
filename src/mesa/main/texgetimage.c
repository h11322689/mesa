   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
   * Copyright (c) 2009 VMware, Inc.
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
   * Code for glGetTexImage() and glGetCompressedTexImage().
   */
         #include "util/glheader.h"
   #include "bufferobj.h"
   #include "enums.h"
   #include "context.h"
   #include "formats.h"
   #include "format_unpack.h"
   #include "glformats.h"
   #include "image.h"
   #include "mtypes.h"
   #include "pack.h"
   #include "pbo.h"
   #include "pixelstore.h"
   #include "texcompress.h"
   #include "texgetimage.h"
   #include "teximage.h"
   #include "texobj.h"
   #include "texstore.h"
   #include "format_utils.h"
   #include "pixeltransfer.h"
   #include "api_exec_decl.h"
      #include "state_tracker/st_cb_texture.h"
      /**
   * Can the given type represent negative values?
   */
   static inline GLboolean
   type_needs_clamping(GLenum type)
   {
      switch (type) {
   case GL_BYTE:
   case GL_SHORT:
   case GL_INT:
   case GL_FLOAT:
   case GL_HALF_FLOAT_ARB:
   case GL_UNSIGNED_INT_10F_11F_11F_REV:
   case GL_UNSIGNED_INT_5_9_9_9_REV:
         default:
            }
         /**
   * glGetTexImage for depth/Z pixels.
   */
   static void
   get_tex_depth(struct gl_context *ctx, GLuint dimensions,
               GLint xoffset, GLint yoffset, GLint zoffset,
   GLsizei width, GLsizei height, GLint depth,
   {
      GLint img, row;
            if (!depthRow) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
               for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + img,
                  if (srcMap) {
      for (row = 0; row < height; row++) {
      void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
               const GLubyte *src = srcMap + row * srcRowStride;
   _mesa_unpack_float_z_row(texImage->TexFormat, width, src, depthRow);
                  }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
                     }
         /**
   * glGetTexImage for depth/stencil pixels.
   */
   static void
   get_tex_depth_stencil(struct gl_context *ctx, GLuint dimensions,
                           {
                        for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + img,
                  if (srcMap) {
      for (row = 0; row < height; row++) {
      const GLubyte *src = srcMap + row * rowstride;
   void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
               switch (type) {
   case GL_UNSIGNED_INT_24_8:
      _mesa_unpack_uint_24_8_depth_stencil_row(texImage->TexFormat,
            case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
      _mesa_unpack_float_32_uint_24_8_depth_stencil_row(texImage->TexFormat,
                  default:
         }
   if (ctx->Pack.SwapBytes) {
                        }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
            }
      /**
   * glGetTexImage for stencil pixels.
   */
   static void
   get_tex_stencil(struct gl_context *ctx, GLuint dimensions,
                  GLint xoffset, GLint yoffset, GLint zoffset,
      {
                        for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + img,
                        if (srcMap) {
      for (row = 0; row < height; row++) {
      const GLubyte *src = srcMap + row * rowstride;
   void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
               _mesa_unpack_ubyte_stencil_row(texImage->TexFormat,
                              }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
            }
         /**
   * glGetTexImage for YCbCr pixels.
   */
   static void
   get_tex_ycbcr(struct gl_context *ctx, GLuint dimensions,
               GLint xoffset, GLint yoffset, GLint zoffset,
   GLsizei width, GLsizei height, GLint depth,
   {
               for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + img,
                  if (srcMap) {
      for (row = 0; row < height; row++) {
      const GLubyte *src = srcMap + row * rowstride;
   void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                        /* check for byte swapping */
   if ((texImage->TexFormat == MESA_FORMAT_YCBCR
      && type == GL_UNSIGNED_SHORT_8_8_REV_MESA) ||
   (texImage->TexFormat == MESA_FORMAT_YCBCR_REV
   && type == GL_UNSIGNED_SHORT_8_8_MESA)) {
   if (!ctx->Pack.SwapBytes)
      }
   else if (ctx->Pack.SwapBytes) {
                        }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
            }
      /**
   * Depending on the base format involved we may need to apply a rebase
   * transform (for example: if we download to a Luminance format we want
   * G=0 and B=0).
   */
   static bool
   teximage_needs_rebase(mesa_format texFormat, GLenum baseFormat,
         {
               if (baseFormat == GL_LUMINANCE ||
      baseFormat == GL_INTENSITY) {
   needsRebase = true;
   rebaseSwizzle[0] = MESA_FORMAT_SWIZZLE_X;
   rebaseSwizzle[1] = MESA_FORMAT_SWIZZLE_ZERO;
   rebaseSwizzle[2] = MESA_FORMAT_SWIZZLE_ZERO;
      } else if (baseFormat == GL_LUMINANCE_ALPHA) {
      needsRebase = true;
   rebaseSwizzle[0] = MESA_FORMAT_SWIZZLE_X;
   rebaseSwizzle[1] = MESA_FORMAT_SWIZZLE_ZERO;
   rebaseSwizzle[2] = MESA_FORMAT_SWIZZLE_ZERO;
      } else if (!is_compressed &&
            needsRebase =
      _mesa_compute_rgba2base2rgba_component_mapping(baseFormat,
               }
         /**
   * Get a color texture image with decompression.
   */
   static void
   get_tex_rgba_compressed(struct gl_context *ctx, GLuint dimensions,
                           GLint xoffset, GLint yoffset, GLint zoffset,
   {
      /* don't want to apply sRGB -> RGB conversion here so override the format */
   const mesa_format texFormat =
         const GLenum baseFormat = _mesa_get_format_base_format(texFormat);
   GLfloat *tempImage, *tempSlice;
   GLuint slice;
   int srcStride, dstStride;
   uint32_t dstFormat;
   bool needsRebase;
            /* Decompress into temp float buffer, then pack into user buffer */
   tempImage = malloc(width * height * depth * 4 * sizeof(GLfloat));
   if (!tempImage) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage()");
               /* Decompress the texture image slices - results in 'tempImage' */
   for (slice = 0; slice < depth; slice++) {
      GLubyte *srcMap;
                     st_MapTextureImage(ctx, texImage, zoffset + slice,
                     if (srcMap) {
                        }
   else {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
   free(tempImage);
                  needsRebase = teximage_needs_rebase(texFormat, baseFormat, true,
            srcStride = 4 * width * sizeof(GLfloat);
   dstStride = _mesa_image_row_stride(&ctx->Pack, width, format, type);
   dstFormat = _mesa_format_from_format_and_type(format, type);
   tempSlice = tempImage;
   for (slice = 0; slice < depth; slice++) {
      void *dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
               _mesa_format_convert(dest, dstFormat, dstStride,
                        /* Handle byte swapping if required */
   if (ctx->Pack.SwapBytes) {
      _mesa_swap_bytes_2d_image(format, type, &ctx->Pack,
                              }
         /**
   * Return a base GL format given the user-requested format
   * for glGetTexImage().
   */
   GLenum
   _mesa_base_pack_format(GLenum format)
   {
      switch (format) {
   case GL_ABGR_EXT:
   case GL_BGRA:
   case GL_BGRA_INTEGER:
   case GL_RGBA_INTEGER:
         case GL_BGR:
   case GL_BGR_INTEGER:
   case GL_RGB_INTEGER:
         case GL_RED_INTEGER:
         case GL_GREEN_INTEGER:
         case GL_BLUE_INTEGER:
         case GL_ALPHA_INTEGER:
         case GL_LUMINANCE_INTEGER_EXT:
         case GL_LUMINANCE_ALPHA_INTEGER_EXT:
         default:
            }
         /**
   * Get an uncompressed color texture image.
   */
   static void
   get_tex_rgba_uncompressed(struct gl_context *ctx, GLuint dimensions,
                           GLint xoffset, GLint yoffset, GLint zoffset,
   {
      /* don't want to apply sRGB -> RGB conversion here so override the format */
   const mesa_format texFormat =
         GLuint img;
   GLboolean dst_is_integer;
   uint32_t dst_format;
   int dst_stride;
   uint8_t rebaseSwizzle[4];
   bool needsRebase;
            needsRebase = teximage_needs_rebase(texFormat, texImage->_BaseFormat, false,
            /* Describe the dst format */
   dst_is_integer = _mesa_is_enum_format_integer(format);
   dst_format = _mesa_format_from_format_and_type(format, type);
            /* Since _mesa_format_convert does not handle transferOps we need to handle
   * them before we call the function. This requires to convert to RGBA float
   * first so we can call _mesa_apply_rgba_transfer_ops. If the dst format is
   * integer then transferOps do not apply.
   */
   assert(!transferOps || (transferOps && !dst_is_integer));
            for (img = 0; img < depth; img++) {
      GLubyte *srcMap;
   GLint rowstride;
   GLubyte *img_src;
   void *dest;
   void *src;
   int src_stride;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + img,
                     if (!srcMap) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage");
               img_src = srcMap;
   dest = _mesa_image_address(dimensions, &ctx->Pack, pixels,
                  if (transferOps) {
      uint32_t rgba_format;
                  /* We will convert to RGBA float */
                  /* If we are lucky and the dst format matches the RGBA format we need
   * to convert to, then we can convert directly into the dst buffer
   * and avoid the final conversion/copy from the rgba buffer to the dst
   * buffer.
   */
   if (format == rgba_format) {
         } else {
      need_convert = true;
   if (rgba == NULL) { /* Allocate the RGBA buffer only once */
      rgba = malloc(height * rgba_stride);
   if (!rgba) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage()");
   st_UnmapTextureImage(ctx, texImage, img);
                     _mesa_format_convert(rgba, rgba_format, rgba_stride,
                                                      /* If we were lucky and our RGBA conversion matches the dst format,
   * then we are done.
   */
                  /* Otherwise, we need to convert from RGBA to dst next */
   src = rgba;
   src_format = rgba_format;
      } else {
      /* No RGBA conversion needed, convert directly to dst */
   src = img_src;
   src_format = texFormat;
               /* Do the conversion to destination format */
   _mesa_format_convert(dest, dst_format, dst_stride,
                     do_swap:
      /* Handle byte swapping if required */
   if (ctx->Pack.SwapBytes)
                  /* Unmap the src texture buffer */
            done:
         }
         /**
   * glGetTexImage for color formats (RGBA, RGB, alpha, LA, etc).
   * Compressed textures are handled here as well.
   */
   static void
   get_tex_rgba(struct gl_context *ctx, GLuint dimensions,
               GLint xoffset, GLint yoffset, GLint zoffset,
   GLsizei width, GLsizei height, GLint depth,
   {
      const GLenum dataType = _mesa_get_format_datatype(texImage->TexFormat);
            /* In general, clamping does not apply to glGetTexImage, except when
   * the returned type of the image can't hold negative values.
   */
   if (type_needs_clamping(type)) {
      /* the returned image type can't have negative values */
   if (dataType == GL_FLOAT ||
      dataType == GL_HALF_FLOAT ||
   dataType == GL_SIGNED_NORMALIZED ||
   format == GL_LUMINANCE ||
   format == GL_LUMINANCE_ALPHA) {
                  if (_mesa_is_format_compressed(texImage->TexFormat)) {
      get_tex_rgba_compressed(ctx, dimensions,
                        }
   else {
      get_tex_rgba_uncompressed(ctx, dimensions,
                           }
         /**
   * Try to do glGetTexImage() with simple memcpy().
   * \return GL_TRUE if done, GL_FALSE otherwise
   */
   static GLboolean
   get_tex_memcpy(struct gl_context *ctx,
                  GLint xoffset, GLint yoffset, GLint zoffset,
      {
      const GLenum target = texImage->TexObject->Target;
   GLboolean memCopy = GL_FALSE;
            /*
   * Check if we can use memcpy to copy from the hardware texture
   * format to the user's format/type.
   * Note that GL's pixel transfer ops don't apply to glGetTexImage()
   */
   if ((target == GL_TEXTURE_1D ||
      target == GL_TEXTURE_2D ||
   target == GL_TEXTURE_RECTANGLE ||
   _mesa_is_cube_face(target)) &&
   texBaseFormat == texImage->_BaseFormat) {
   memCopy = _mesa_format_matches_format_and_type(texImage->TexFormat,
                     if (depth > 1) {
      /* only a single slice is supported at this time */
               if (memCopy) {
      const GLuint bpp = _mesa_get_format_bytes(texImage->TexFormat);
   const GLint bytesPerRow = width * bpp;
   GLubyte *dst =
      _mesa_image_address2d(&ctx->Pack, pixels, width, height,
      const GLint dstRowStride =
         GLubyte *src;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset,
                  if (src) {
      if (bytesPerRow == dstRowStride && bytesPerRow == srcRowStride) {
         }
   else {
      GLuint row;
   for (row = 0; row < height; row++) {
      memcpy(dst, src, bytesPerRow);
   dst += dstRowStride;
                  /* unmap src texture buffer */
      }
   else {
                        }
         /**
   * This is the software fallback for GetTexSubImage().
   * All error checking will have been done before this routine is called.
   */
   void
   _mesa_GetTexSubImage_sw(struct gl_context *ctx,
                           {
      const GLuint dimensions =
            /* map dest buffer, if PBO */
   if (ctx->Pack.BufferObj) {
      /* Packing texture image into a PBO.
   * Map the (potentially) VRAM-based buffer into our process space so
   * we can write into it with the code below.
   * A hardware driver might use a sophisticated blit to move the
   * texture data to the PBO if the PBO is in VRAM along with the texture.
   */
   GLubyte *buf = (GLubyte *)
      _mesa_bufferobj_map_range(ctx, 0, ctx->Pack.BufferObj->Size,
            if (!buf) {
      /* out of memory or other unexpected error */
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "glGetTexImage(map PBO failed)");
      }
   /* <pixels> was an offset into the PBO.
   * Now make it a real, client-side pointer inside the mapped region.
   */
               /* for all array textures, the Z axis selects the layer */
   if (texImage->TexObject->Target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
   height = 1;
   zoffset = yoffset;
   yoffset = 0;
      } else {
                  if (get_tex_memcpy(ctx, xoffset, yoffset, zoffset, width, height, depth,
               }
   else if (format == GL_DEPTH_COMPONENT) {
      get_tex_depth(ctx, dimensions, xoffset, yoffset, zoffset,
      }
   else if (format == GL_DEPTH_STENCIL_EXT) {
      get_tex_depth_stencil(ctx, dimensions, xoffset, yoffset, zoffset,
            }
   else if (format == GL_STENCIL_INDEX) {
      get_tex_stencil(ctx, dimensions, xoffset, yoffset, zoffset,
      }
   else if (format == GL_YCBCR_MESA) {
      get_tex_ycbcr(ctx, dimensions, xoffset, yoffset, zoffset,
      }
   else {
      get_tex_rgba(ctx, dimensions, xoffset, yoffset, zoffset,
               if (ctx->Pack.BufferObj) {
            }
            /**
   * This function assumes that all error checking has been done.
   */
   static void
   get_compressed_texsubimage_sw(struct gl_context *ctx,
                                 {
      const GLuint dimensions =
         struct compressed_pixelstore store;
   GLint slice;
            _mesa_compute_compressed_pixelstore(dimensions, texImage->TexFormat,
                  if (ctx->Pack.BufferObj) {
      /* pack texture image into a PBO */
   dest = (GLubyte *)
      _mesa_bufferobj_map_range(ctx, 0, ctx->Pack.BufferObj->Size,
            if (!dest) {
      /* out of memory or other unexpected error */
   _mesa_error(ctx, GL_OUT_OF_MEMORY,
            }
      } else {
                           for (slice = 0; slice < store.CopySlices; slice++) {
      GLint srcRowStride;
            /* map src texture buffer */
   st_MapTextureImage(ctx, texImage, zoffset + slice,
                  if (src) {
      GLint i;
   for (i = 0; i < store.CopyRowsPerSlice; i++) {
      memcpy(dest, src, store.CopyBytesPerRow);
   dest += store.TotalBytesPerRow;
                        /* Advance to next slice */
               } else {
                     if (ctx->Pack.BufferObj) {
            }
         /**
   * Validate the texture target enum supplied to glGetTex(ture)Image or
   * glGetCompressedTex(ture)Image.
   */
   static GLboolean
   legal_getteximage_target(struct gl_context *ctx, GLenum target, bool dsa)
   {
      switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_3D:
         case GL_TEXTURE_RECTANGLE_NV:
         case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_TEXTURE_2D_ARRAY_EXT:
         case GL_TEXTURE_CUBE_MAP_ARRAY:
            /* Section 8.11 (Texture Queries) of the OpenGL 4.5 core profile spec
   * (30.10.2014) says:
   *    "An INVALID_ENUM error is generated if the effective target is not
   *    one of TEXTURE_1D, TEXTURE_2D, TEXTURE_3D, TEXTURE_1D_ARRAY,
   *    TEXTURE_2D_ARRAY, TEXTURE_CUBE_MAP_ARRAY, TEXTURE_RECTANGLE, one of
   *    the targets from table 8.19 (for GetTexImage and GetnTexImage *only*),
   *    or TEXTURE_CUBE_MAP (for GetTextureImage *only*)." (Emphasis added.)
   */
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         case GL_TEXTURE_CUBE_MAP:
         default:
            }
         /**
   * Wrapper for _mesa_select_tex_image() which can handle target being
   * GL_TEXTURE_CUBE_MAP in which case we use zoffset to select a cube face.
   * This can happen for glGetTextureImage and glGetTextureSubImage (DSA
   * functions).
   */
   static struct gl_texture_image *
   select_tex_image(const struct gl_texture_object *texObj, GLenum target,
         {
      assert(level >= 0);
   assert(level < MAX_TEXTURE_LEVELS);
   if (target == GL_TEXTURE_CUBE_MAP) {
      assert(zoffset >= 0);
   assert(zoffset < 6);
      }
      }
         /**
   * Error-check the offset and size arguments to
   * glGet[Compressed]TextureSubImage().
   * \return true if error, false if no error.
   */
   static bool
   dimensions_error_check(struct gl_context *ctx,
                        struct gl_texture_object *texObj,
      {
      const struct gl_texture_image *texImage;
            if (xoffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(xoffset = %d)", caller, xoffset);
               if (yoffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(yoffset = %d)", caller, yoffset);
               if (zoffset < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(zoffset = %d)", caller, zoffset);
               if (width < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(width = %d)", caller, width);
               if (height < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(height = %d)", caller, height);
               if (depth < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(depth = %d)", caller, depth);
               /* do special per-target checks */
   switch (target) {
   case GL_TEXTURE_1D:
      if (yoffset != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
   if (height != 1) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
      case GL_TEXTURE_1D_ARRAY:
   case GL_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
      if (zoffset != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
   if (depth != 1) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
      case GL_TEXTURE_CUBE_MAP:
      /* Non-array cube maps are special because we have a gl_texture_image
   * per face.
   */
   if (zoffset + depth > 6) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
      default:
                  texImage = select_tex_image(texObj, target, level, zoffset);
   if (texImage) {
      imageWidth = texImage->Width;
   imageHeight = texImage->Height;
               if (xoffset + width > imageWidth) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (yoffset + height > imageHeight) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                           if (target != GL_TEXTURE_CUBE_MAP) {
      /* Cube map error checking was done above */
   if (zoffset + depth > imageDepth) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                              /* Extra checks for compressed textures */
   if (texImage) {
      GLuint bw, bh, bd;
   _mesa_get_format_block_size_3d(texImage->TexFormat, &bw, &bh, &bd);
   if (bw > 1 || bh > 1 || bd > 1) {
      /* offset must be multiple of block size */
   if (xoffset % bw != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
            }
   if (target != GL_TEXTURE_1D && target != GL_TEXTURE_1D_ARRAY) {
      if (yoffset % bh != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                        if (zoffset % bd != 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     /* The size must be a multiple of bw x bh x bd, or we must be using a
   * offset+size that exactly hits the edge of the image.
   */
   if ((width % bw != 0) &&
      (xoffset + width != (GLint) texImage->Width)) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                     if ((height % bh != 0) &&
      (yoffset + height != (GLint) texImage->Height)) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                     if ((depth % bd != 0) &&
      (zoffset + depth != (GLint) texImage->Depth)) {
   _mesa_error(ctx, GL_INVALID_VALUE,
                           if (width == 0 || height == 0 || depth == 0) {
      /* Not an error, but nothing to do.  Return 'true' so that the
   * caller simply returns.
   */
                  }
         /**
   * Do PBO-related error checking for getting uncompressed images.
   * \return true if there was an error (or the GetTexImage is to be a no-op)
   */
   static bool
   pbo_error_check(struct gl_context *ctx, GLenum target,
                  GLsizei width, GLsizei height, GLsizei depth,
      {
               if (!_mesa_validate_pbo_access(dimensions, &ctx->Pack, width, height, depth,
            if (ctx->Pack.BufferObj) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
      } else {
      _mesa_error(ctx, GL_INVALID_OPERATION,
            }
               if (ctx->Pack.BufferObj) {
      /* PBO should not be mapped */
   if (_mesa_check_disallowed_mapping(ctx->Pack.BufferObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        if (!ctx->Pack.BufferObj && !pixels) {
      /* not an error, do nothing */
                  }
         /**
   * Do teximage-related error checking for getting uncompressed images.
   * \return true if there was an error
   */
   static bool
   teximage_error_check(struct gl_context *ctx,
               {
      GLenum baseFormat;
            /*
   * Format and type checking has been moved up to GetnTexImage and
   * GetTextureImage so that it happens before getting the texImage object.
                     /* Make sure the requested image format is compatible with the
   * texture's format.
   */
   if (_mesa_is_color_format(format)
      && !_mesa_is_color_format(baseFormat)) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   else if (_mesa_is_depth_format(format)
            && !_mesa_is_depth_format(baseFormat)
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   else if (_mesa_is_stencil_format(format)
            _mesa_error(ctx, GL_INVALID_ENUM,
            }
   else if (_mesa_is_stencil_format(format)
            && !_mesa_is_depthstencil_format(baseFormat)
   _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   else if (_mesa_is_ycbcr_format(format)
            _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   else if (_mesa_is_depthstencil_format(format)
            _mesa_error(ctx, GL_INVALID_OPERATION,
            }
   else if (!_mesa_is_stencil_format(format) &&
            _mesa_is_enum_format_integer(format) !=
   _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Do common teximage-related error checking for getting uncompressed images.
   * \return true if there was an error
   */
   static bool
   common_error_check(struct gl_context *ctx,
                     struct gl_texture_object *texObj,
   GLenum target, GLint level,
   {
      GLenum err;
            if (texObj->Target == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid texture)", caller);
               maxLevels = _mesa_max_texture_levels(ctx, target);
   if (level < 0 || level >= maxLevels) {
      _mesa_error(ctx, GL_INVALID_VALUE, "%s(level = %d)", caller, level);
               err = _mesa_error_check_format_and_type(ctx, format, type);
   if (err != GL_NO_ERROR) {
      _mesa_error(ctx, err, "%s(format/type)", caller);
               /* According to OpenGL 4.6 spec, section 8.11.4 ("Texture Image Queries"):
   *
   *   "An INVALID_OPERATION error is generated by GetTextureImage if the
   *   effective target is TEXTURE_CUBE_MAP or TEXTURE_CUBE_MAP_ARRAY ,
   *   and the texture object is not cube complete or cube array complete,
   *   respectively."
   *
   * This applies also to GetTextureSubImage, GetCompressedTexImage,
   * GetCompressedTextureImage, and GetnCompressedTexImage.
   */
   if (target == GL_TEXTURE_CUBE_MAP && !_mesa_cube_complete(texObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                        }
         /**
   * Do error checking for all (non-compressed) get-texture-image functions.
   * \return true if any error, false if no errors.
   */
   static bool
   getteximage_error_check(struct gl_context *ctx,
                           struct gl_texture_object *texObj,
   {
                        if (common_error_check(ctx, texObj, target, level, width, height, depth,
                        if (width == 0 || height == 0 || depth == 0) {
      /* Not an error, but nothing to do.  Return 'true' so that the
   * caller simply returns.
   */
               if (pbo_error_check(ctx, target, width, height, depth,
                        texImage = select_tex_image(texObj, target, level, 0);
   if (teximage_error_check(ctx, texImage, format, caller)) {
                     }
         /**
   * Do error checking for all (non-compressed) get-texture-image functions.
   * \return true if any error, false if no errors.
   */
   static bool
   gettexsubimage_error_check(struct gl_context *ctx,
                              struct gl_texture_object *texObj,
      {
                        if (common_error_check(ctx, texObj, target, level, width, height, depth,
                        if (dimensions_error_check(ctx, texObj, target, level,
                              if (pbo_error_check(ctx, target, width, height, depth,
                        texImage = select_tex_image(texObj, target, level, zoffset);
   if (teximage_error_check(ctx, texImage, format, caller)) {
                     }
         /**
   * Return the width, height and depth of a texture image.
   * This function must be resilient to bad parameter values since
   * this is called before full error checking.
   */
   static void
   get_texture_image_dims(const struct gl_texture_object *texObj,
               {
               if (level >= 0 && level < MAX_TEXTURE_LEVELS) {
                  if (texImage) {
      *width = texImage->Width;
   *height = texImage->Height;
   if (target == GL_TEXTURE_CUBE_MAP) {
         }
   else {
            }
   else {
            }
         /**
   * Common code for all (uncompressed) get-texture-image functions.
   * \param texObj  the texture object (should not be null)
   * \param target  user-provided target, or 0 for DSA
   * \param level image level.
   * \param format pixel data format for returned image.
   * \param type pixel data type for returned image.
   * \param bufSize size of the pixels data buffer.
   * \param pixels returned pixel data.
   * \param caller  name of calling function
   */
   static void
   get_texture_image(struct gl_context *ctx,
                     struct gl_texture_object *texObj,
   GLenum target, GLint level,
   GLint xoffset, GLint yoffset, GLint zoffset,
   {
      struct gl_texture_image *texImage;
   unsigned firstFace, numFaces, i;
                     texImage = select_tex_image(texObj, target, level, zoffset);
            if (_mesa_is_zero_size_texture(texImage)) {
      /* no image data to return */
               if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx, "%s(tex %u) format = %s, w=%d, h=%d,"
               " dstFmt=0x%x, dstType=0x%x\n",
   caller, texObj->Name,
               if (target == GL_TEXTURE_CUBE_MAP) {
      /* Compute stride between cube faces */
   imageStride = _mesa_image_image_stride(&ctx->Pack, width, height,
         firstFace = zoffset;
   numFaces = depth;
   zoffset = 0;
      }
   else {
      imageStride = 0;
   firstFace = _mesa_tex_target_to_face(target);
               if (ctx->Pack.BufferObj)
                     for (i = 0; i < numFaces; i++) {
      texImage = texObj->Image[firstFace + i][level];
            st_GetTexSubImage(ctx, xoffset, yoffset, zoffset,
                  /* next cube face */
                  }
      static void
   _get_texture_image(struct gl_context *ctx,
                     struct gl_texture_object *texObj,
   GLenum target, GLint level,
   {
      GLsizei width, height, depth;
   /* EXT/ARB direct_state_access variants don't call _get_texture_image
   * with a NULL texObj */
            if (!is_dsa) {
      texObj = _mesa_get_current_tex_object(ctx, target);
                           if (getteximage_error_check(ctx, texObj, target, level,
                              get_texture_image(ctx, texObj, target, level,
            }
         void GLAPIENTRY
   _mesa_GetnTexImageARB(GLenum target, GLint level, GLenum format, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!legal_getteximage_target(ctx, target, false)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", caller);
               _get_texture_image(ctx, NULL, target, level, format, type,
      }
         void GLAPIENTRY
   _mesa_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
            if (!legal_getteximage_target(ctx, target, false)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", caller);
               _get_texture_image(ctx, NULL, target, level, format, type,
      }
         void GLAPIENTRY
   _mesa_GetTextureImage(GLuint texture, GLint level, GLenum format, GLenum type,
         {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetTextureImage";
   struct gl_texture_object *texObj =
            if (!texObj) {
                  if (!legal_getteximage_target(ctx, texObj->Target, true)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
               _get_texture_image(ctx, texObj, texObj->Target, level, format, type,
      }
         void GLAPIENTRY
   _mesa_GetTextureImageEXT(GLuint texture, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetTextureImageEXT";
   struct gl_texture_object *texObj =
      _mesa_lookup_or_create_texture(ctx, target, texture,
         if (!texObj) {
                  if (!legal_getteximage_target(ctx, target, true)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", caller);
               _get_texture_image(ctx, texObj, target, level, format, type,
      }
         void GLAPIENTRY
   _mesa_GetMultiTexImageEXT(GLenum texunit, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   GLsizei width, height, depth;
            struct gl_texture_object *texObj =
      _mesa_get_texobj_by_target_and_texunit(ctx, target,
                     if (!texObj) {
                  if (!legal_getteximage_target(ctx, texObj->Target, true)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s", caller);
               get_texture_image_dims(texObj, texObj->Target, level,
            if (getteximage_error_check(ctx, texObj, texObj->Target, level,
                              get_texture_image(ctx, texObj, texObj->Target, level,
            }
         void GLAPIENTRY
   _mesa_GetTextureSubImage(GLuint texture, GLint level,
                           {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetTextureSubImage";
   struct gl_texture_object *texObj =
            if (!texObj) {
                  if (!legal_getteximage_target(ctx, texObj->Target, true)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     if (gettexsubimage_error_check(ctx, texObj, texObj->Target, level,
                                    get_texture_image(ctx, texObj, texObj->Target, level,
            }
            /**
   * Compute the number of bytes which will be written when retrieving
   * a sub-region of a compressed texture.
   */
   static GLsizei
   packed_compressed_size(GLuint dimensions, mesa_format format,
               {
      struct compressed_pixelstore st;
            _mesa_compute_compressed_pixelstore(dimensions, format,
               totalBytes =
      (st.CopySlices - 1) * st.TotalRowsPerSlice * st.TotalBytesPerRow +
   st.SkipBytes +
   (st.CopyRowsPerSlice - 1) * st.TotalBytesPerRow +
            }
         /**
   * Do error checking for getting compressed texture images.
   * \return true if any error, false if no errors.
   */
   static bool
   getcompressedteximage_error_check(struct gl_context *ctx,
                                       {
      struct gl_texture_image *texImage;
   GLint maxLevels;
   GLsizei totalBytes;
                     if (texObj->Target == 0) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(invalid texture)", caller);
               maxLevels = _mesa_max_texture_levels(ctx, target);
   if (level < 0 || level >= maxLevels) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                     if (dimensions_error_check(ctx, texObj, target, level,
                              texImage = select_tex_image(texObj, target, level, zoffset);
            if (!_mesa_is_format_compressed(texImage->TexFormat)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* Check for invalid pixel storage modes */
   dimensions = _mesa_get_texture_dimensions(texObj->Target);
   if (!_mesa_compressed_pixel_storage_error_check(ctx, dimensions,
                              /* Compute number of bytes that may be touched in the dest buffer */
   totalBytes = packed_compressed_size(dimensions, texImage->TexFormat,
                  /* Do dest buffer bounds checking */
   if (ctx->Pack.BufferObj) {
      /* do bounds checking on PBO write */
   if ((GLubyte *) pixels + totalBytes >
      (GLubyte *) ctx->Pack.BufferObj->Size) {
   _mesa_error(ctx, GL_INVALID_OPERATION,
                     /* make sure PBO is not mapped */
   if (_mesa_check_disallowed_mapping(ctx->Pack.BufferObj)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "%s(PBO is mapped)", caller);
         }
   else {
      /* do bounds checking on writing to client memory */
   if (totalBytes > bufSize) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                              if (!ctx->Pack.BufferObj && !pixels) {
      /* not an error, but do nothing */
                  }
         /**
   * Common helper for all glGetCompressed-teximage functions.
   */
   static void
   get_compressed_texture_image(struct gl_context *ctx,
                              struct gl_texture_object *texObj,
      {
      struct gl_texture_image *texImage;
                     texImage = select_tex_image(texObj, target, level, zoffset);
            if (_mesa_is_zero_size_texture(texImage))
            if (MESA_VERBOSE & (VERBOSE_API | VERBOSE_TEXTURE)) {
      _mesa_debug(ctx,
               "%s(tex %u) format = %s, w=%d, h=%d\n",
               if (target == GL_TEXTURE_CUBE_MAP) {
               /* Compute image stride between cube faces */
   _mesa_compute_compressed_pixelstore(2, texImage->TexFormat,
                        firstFace = zoffset;
   numFaces = depth;
   zoffset = 0;
      }
   else {
      imageStride = 0;
   firstFace = _mesa_tex_target_to_face(target);
               if (ctx->Pack.BufferObj)
                     for (i = 0; i < numFaces; i++) {
      texImage = texObj->Image[firstFace + i][level];
            get_compressed_texsubimage_sw(ctx, texImage,
                  /* next cube face */
                  }
         void GLAPIENTRY
   _mesa_GetnCompressedTexImageARB(GLenum target, GLint level, GLsizei bufSize,
         {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetnCompressedTexImageARB";
   GLsizei width, height, depth;
            if (!legal_getteximage_target(ctx, target, false)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", caller);
               texObj = _mesa_get_current_tex_object(ctx, target);
                     if (getcompressedteximage_error_check(ctx, texObj, target, level,
                              get_compressed_texture_image(ctx, texObj, target, level,
            }
         void GLAPIENTRY
   _mesa_GetCompressedTexImage(GLenum target, GLint level, GLvoid *pixels)
   {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetCompressedTexImage";
   GLsizei width, height, depth;
            if (!legal_getteximage_target(ctx, target, false)) {
      _mesa_error(ctx, GL_INVALID_ENUM, "%s", caller);
               texObj = _mesa_get_current_tex_object(ctx, target);
            get_texture_image_dims(texObj, target, level,
            if (getcompressedteximage_error_check(ctx, texObj, target, level,
                              get_compressed_texture_image(ctx, texObj, target, level,
            }
         void GLAPIENTRY
   _mesa_GetCompressedTextureImageEXT(GLuint texture, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_object*  texObj;
   GLsizei width, height, depth;
            texObj = _mesa_lookup_or_create_texture(ctx, target, texture,
            get_texture_image_dims(texObj, texObj->Target, level,
            if (getcompressedteximage_error_check(ctx, texObj, texObj->Target, level,
                              get_compressed_texture_image(ctx, texObj, texObj->Target, level,
            }
         void GLAPIENTRY
   _mesa_GetCompressedMultiTexImageEXT(GLenum texunit, GLenum target, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   struct gl_texture_object*  texObj;
   GLsizei width, height, depth;
            texObj = _mesa_get_texobj_by_target_and_texunit(ctx, target,
                        get_texture_image_dims(texObj, texObj->Target, level,
            if (getcompressedteximage_error_check(ctx, texObj, texObj->Target, level,
                              get_compressed_texture_image(ctx, texObj, texObj->Target, level,
            }
         void GLAPIENTRY
   _mesa_GetCompressedTextureImage(GLuint texture, GLint level,
         {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetCompressedTextureImage";
   GLsizei width, height, depth;
   struct gl_texture_object *texObj =
            if (!texObj) {
                  get_texture_image_dims(texObj, texObj->Target, level,
            if (getcompressedteximage_error_check(ctx, texObj, texObj->Target, level,
                              get_compressed_texture_image(ctx, texObj, texObj->Target, level,
            }
         void GLAPIENTRY
   _mesa_GetCompressedTextureSubImage(GLuint texture, GLint level,
                           {
      GET_CURRENT_CONTEXT(ctx);
   static const char *caller = "glGetCompressedTextureImage";
            texObj = _mesa_lookup_texture_err(ctx, texture, caller);
   if (!texObj) {
                  if (getcompressedteximage_error_check(ctx, texObj, texObj->Target, level,
                                    get_compressed_texture_image(ctx, texObj, texObj->Target, level,
                  }
