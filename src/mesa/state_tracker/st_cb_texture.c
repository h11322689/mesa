   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
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
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <stdio.h>
   #include "main/bufferobj.h"
   #include "main/context.h"
   #include "main/enums.h"
   #include "main/errors.h"
   #include "main/fbobject.h"
   #include "main/formats.h"
   #include "main/format_utils.h"
   #include "main/framebuffer.h"
   #include "main/glformats.h"
   #include "main/image.h"
   #include "main/formatquery.h"
      #include "main/macros.h"
   #include "main/mipmap.h"
   #include "main/pack.h"
   #include "main/pbo.h"
   #include "main/pixeltransfer.h"
   #include "main/texcompress.h"
   #include "main/texcompress_astc.h"
   #include "main/texcompress_bptc.h"
   #include "main/texcompress_etc.h"
   #include "main/texcompress_rgtc.h"
   #include "main/texcompress_s3tc.h"
   #include "main/texgetimage.h"
   #include "main/teximage.h"
   #include "main/texobj.h"
   #include "main/texstore.h"
      #include "state_tracker/st_debug.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_cb_bitmap.h"
   #include "state_tracker/st_cb_drawpixels.h"
   #include "state_tracker/st_cb_flush.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_format.h"
   #include "state_tracker/st_pbo.h"
   #include "state_tracker/st_texture.h"
   #include "state_tracker/st_gen_mipmap.h"
   #include "state_tracker/st_atom.h"
   #include "state_tracker/st_sampler_view.h"
   #include "state_tracker/st_texcompress_compute.h"
   #include "state_tracker/st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/log.h"
   #include "util/u_inlines.h"
   #include "util/u_upload_mgr.h"
   #include "pipe/p_shader_tokens.h"
   #include "util/u_tile.h"
   #include "util/format/u_format.h"
   #include "util/u_surface.h"
   #include "util/u_sampler.h"
   #include "util/u_math.h"
   #include "util/u_box.h"
   #include "util/u_memory.h"
   #include "util/u_simple_shaders.h"
   #include "cso_cache/cso_context.h"
      #define DBG if (0) printf
         enum pipe_texture_target
   gl_target_to_pipe(GLenum target)
   {
      switch (target) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
         case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
         case GL_TEXTURE_RECTANGLE_NV:
   case GL_PROXY_TEXTURE_RECTANGLE_NV:
         case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
         case GL_TEXTURE_CUBE_MAP_ARB:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARB:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
         case GL_TEXTURE_1D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_1D_ARRAY_EXT:
         case GL_TEXTURE_2D_ARRAY_EXT:
   case GL_PROXY_TEXTURE_2D_ARRAY_EXT:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
         case GL_TEXTURE_BUFFER:
         case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
         default:
      assert(0);
         }
      enum pipe_format
   st_pbo_get_src_format(struct pipe_screen *screen, enum pipe_format src_format, struct pipe_resource *src)
   {
      /* Convert the source format to what is expected by GetTexImage
   * and see if it's supported.
   *
   * This only applies to glGetTexImage:
   * - Luminance must be returned as (L,0,0,1).
   * - Luminance alpha must be returned as (L,0,0,A).
   * - Intensity must be returned as (I,0,0,1)
   */
   src_format = util_format_linear(src_format);
   src_format = util_format_luminance_to_red(src_format);
            if (!src_format ||
      !screen->is_format_supported(screen, src_format, src->target,
                  }
      }
      static struct pipe_resource *
   create_dst_texture(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
            if (pipe_target == PIPE_TEXTURE_CUBE || pipe_target == PIPE_TEXTURE_CUBE_ARRAY) {
      width = MAX2(width, height);
               /* create the destination texture of size (width X height X depth) */
   memset(&dst_templ, 0, sizeof(dst_templ));
   dst_templ.target = pipe_target;
   dst_templ.format = dst_format;
   dst_templ.bind = bind;
            st_gl_texture_dims_to_pipe_dims(gl_target, width, height, depth,
                     }
      static bool
   copy_to_staging_dest(struct gl_context * ctx, struct pipe_resource *dst,
                  GLint xoffset, GLint yoffset, GLint zoffset,
      {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct gl_texture_object *stObj = texImage->TexObject;
   ASSERTED struct pipe_resource *src = stObj->pt;
   enum pipe_format dst_format = dst->format;
   mesa_format mesa_format;
   GLenum gl_target = texImage->TexObject->Target;
   unsigned dims;
   struct pipe_transfer *tex_xfer;
   uint8_t *map = NULL;
                     map = pipe_texture_map_3d(pipe, dst, 0, PIPE_MAP_READ,
         if (!map) {
                  mesa_format = st_pipe_format_to_mesa_format(dst_format);
            /* copy/pack data into user buffer */
   if (_mesa_format_matches_format_and_type(mesa_format, format, type,
            /* memcpy */
   const uint bytesPerRow = width * util_format_get_blocksize(dst_format);
            for (slice = 0; slice < depth; slice++) {
               for (row = 0; row < height; row++) {
      void *dest = _mesa_image_address(dims, &ctx->Pack, pixels,
                                             }
   else {
      /* format translation via floats */
   GLuint slice;
   GLfloat *rgba;
   uint32_t dstMesaFormat;
                     rgba = malloc(width * height * 4 * sizeof(GLfloat));
   if (!rgba) {
                  if (ST_DEBUG & DEBUG_FALLBACK)
            dstMesaFormat = _mesa_format_from_format_and_type(format, type);
   dstStride = _mesa_image_row_stride(&ctx->Pack, width, format, type);
   srcStride = 4 * width * sizeof(GLfloat);
   for (slice = 0; slice < depth; slice++) {
      void *dest = _mesa_image_address(dims, &ctx->Pack, pixels,
                  /* get float[4] rgba row from surface */
                  _mesa_format_convert(dest, dstMesaFormat, dstStride,
                  /* Handle byte swapping if required */
   if (ctx->Pack.SwapBytes) {
      _mesa_swap_bytes_2d_image(format, type, &ctx->Pack,
                              }
         end:
      if (map)
            _mesa_unmap_pbo_dest(ctx, &ctx->Pack);
      }
      enum pipe_format
   st_pbo_get_dst_format(struct gl_context *ctx, enum pipe_texture_target target,
               {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
   /* Choose the destination format by finding the best match
   * for the format+type combo. */
   enum pipe_format dst_format = st_choose_matching_format(st, bind, format, type,
            if (dst_format == PIPE_FORMAT_NONE) {
               /* Fall back to _mesa_GetTexImage_sw except for compressed formats,
   * where decompression with a blit is always preferred. */
   if (!is_compressed) {
                  /* Set the appropriate format for the decompressed texture.
   * Luminance and sRGB formats shouldn't appear here.*/
   switch (src_format) {
   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
   case PIPE_FORMAT_RGTC1_UNORM:
   case PIPE_FORMAT_RGTC2_UNORM:
   case PIPE_FORMAT_ETC1_RGB8:
   case PIPE_FORMAT_ETC2_RGB8:
   case PIPE_FORMAT_ETC2_RGB8A1:
   case PIPE_FORMAT_ETC2_RGBA8:
   case PIPE_FORMAT_ASTC_4x4:
   case PIPE_FORMAT_ASTC_5x4:
   case PIPE_FORMAT_ASTC_5x5:
   case PIPE_FORMAT_ASTC_6x5:
   case PIPE_FORMAT_ASTC_6x6:
   case PIPE_FORMAT_ASTC_8x5:
   case PIPE_FORMAT_ASTC_8x6:
   case PIPE_FORMAT_ASTC_8x8:
   case PIPE_FORMAT_ASTC_10x5:
   case PIPE_FORMAT_ASTC_10x6:
   case PIPE_FORMAT_ASTC_10x8:
   case PIPE_FORMAT_ASTC_10x10:
   case PIPE_FORMAT_ASTC_12x10:
   case PIPE_FORMAT_ASTC_12x12:
   case PIPE_FORMAT_BPTC_RGBA_UNORM:
   case PIPE_FORMAT_FXT1_RGB:
   case PIPE_FORMAT_FXT1_RGBA:
      dst_glformat = GL_RGBA8;
      case PIPE_FORMAT_RGTC1_SNORM:
   case PIPE_FORMAT_RGTC2_SNORM:
      if (!ctx->Extensions.EXT_texture_snorm)
         dst_glformat = GL_RGBA8_SNORM;
      case PIPE_FORMAT_BPTC_RGB_FLOAT:
   case PIPE_FORMAT_BPTC_RGB_UFLOAT:
      if (!ctx->Extensions.ARB_texture_float)
         dst_glformat = GL_RGBA32F;
      case PIPE_FORMAT_ETC2_R11_UNORM:
      if (bind && !screen->is_format_supported(screen, PIPE_FORMAT_R16_UNORM,
               dst_glformat = GL_R16;
      case PIPE_FORMAT_ETC2_R11_SNORM:
      if (bind && !screen->is_format_supported(screen, PIPE_FORMAT_R16_SNORM,
               dst_glformat = GL_R16_SNORM;
      case PIPE_FORMAT_ETC2_RG11_UNORM:
      if (bind && !screen->is_format_supported(screen, PIPE_FORMAT_R16G16_UNORM,
               dst_glformat = GL_RG16;
      case PIPE_FORMAT_ETC2_RG11_SNORM:
      if (bind && !screen->is_format_supported(screen, PIPE_FORMAT_R16G16_SNORM,
               dst_glformat = GL_RG16_SNORM;
      default:
      assert(0);
               dst_format = st_choose_format(st, dst_glformat, format, type,
            }
      }
      void
   st_FreeTextureImageBuffer(struct gl_context *ctx,
         {
      struct st_context *st = st_context(ctx);
   struct gl_texture_object *stObj = texImage->TexObject;
                     if (stImage->pt) {
                  free(stImage->transfer);
   stImage->transfer = NULL;
            if (stImage->compressed_data &&
      pipe_reference(&stImage->compressed_data->reference, NULL)) {
   free(stImage->compressed_data->ptr);
   FREE(stImage->compressed_data);
               /* if the texture image is being deallocated, the structure of the
   * texture is changing so we'll likely need a new sampler view.
   */
      }
      bool
   st_astc_format_fallback(const struct st_context *st, mesa_format format)
   {
      if (!_mesa_is_format_astc_2d(format))
            if (st->astc_void_extents_need_denorm_flush && !util_format_is_srgb(format))
            if (format == MESA_FORMAT_RGBA_ASTC_5x5 ||
      format == MESA_FORMAT_SRGB8_ALPHA8_ASTC_5x5)
            }
      bool
   st_compressed_format_fallback(struct st_context *st, mesa_format format)
   {
      switch (_mesa_get_format_layout(format)) {
   case MESA_FORMAT_LAYOUT_ETC1:
         case MESA_FORMAT_LAYOUT_ETC2:
         case MESA_FORMAT_LAYOUT_S3TC:
         case MESA_FORMAT_LAYOUT_RGTC:
         case MESA_FORMAT_LAYOUT_LATC:
         case MESA_FORMAT_LAYOUT_BPTC:
         case MESA_FORMAT_LAYOUT_ASTC:
         default:
            }
         static void
   compressed_tex_fallback_allocate(struct st_context *st,
         {
      if (!st_compressed_format_fallback(st, texImage->TexFormat))
            if (texImage->compressed_data &&
      pipe_reference(&texImage->compressed_data->reference, NULL)) {
   free(texImage->compressed_data->ptr);
               unsigned data_size = _mesa_format_image_size(texImage->TexFormat,
                        texImage->compressed_data = CALLOC_STRUCT(st_compressed_data);
   texImage->compressed_data->ptr =
            }
         void
   st_MapTextureImage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
   {
               /* Check for unexpected flags */
   assert((mode & ~(GL_MAP_READ_BIT |
                  const enum pipe_map_flags transfer_flags =
            if (st_compressed_format_fallback(st, texImage->TexFormat)) {
      /* Some compressed formats don't have to be supported by drivers,
   * and st/mesa transparently handles decompression on upload (Unmap),
   * so that drivers don't see the compressed formats.
   *
   * We store the compressed data (it's needed for glGetCompressedTexImage
   * and image copies in OES_copy_image).
   */
   unsigned z = slice + texImage->Face +
            /* Enlarge the transfer array if it's not large enough. */
                     assert(itransfer->box.depth == 0);
   if (transfer_flags & PIPE_MAP_WRITE)
            unsigned blk_w, blk_h;
            unsigned y_blocks = DIV_ROUND_UP(texImage->Height2, blk_h);
   unsigned stride = *rowStrideOut = itransfer->temp_stride =
                  assert(texImage->compressed_data);
   *mapOut = itransfer->temp_data =
      texImage->compressed_data->ptr +
   (z * y_blocks + (y / blk_h)) * stride +
   } else {
      struct pipe_transfer *transfer;
   *mapOut = st_texture_image_map(st, texImage, transfer_flags,
               }
      static void
   log_unmap_time_delta(const struct pipe_box *box,
               {
      assert(start_us >= 0);
   mesa_logi("unmap %dx%d pixels of %s data for %s tex, %s path: "
            "%"PRIi64" us\n", box->width, box->height,
   util_format_short_name(texImage->TexFormat),
   }
      /**
   * Upload ASTC data but flush denorms in any void extent blocks.
   */
   static void
   upload_astc_slice_with_flushed_void_extents(uint8_t *dst,
                                       {
      unsigned blk_w, blk_h;
            unsigned x_blocks = (src_width + blk_w - 1) / blk_w;
            for (int y = 0; y < y_blocks; y++) {
      /* An ASTC block is stored in little endian mode. The byte that
   * contains bits 0..7 is stored at the lower address in memory.
   */
   struct astc_block {
      uint16_t header;
   uint16_t dontcare0;
   uint16_t dontcare1;
   uint16_t dontcare2;
   uint16_t R;
   uint16_t G;
   uint16_t B;
               /* Iterate over every copied block in the row */
   for (int x = 0; x < x_blocks; x++) {
      /* Check if the header matches that of an LDR void-extent block */
   if ((blocks[x].header & 0xfff) == 0xDFC) {
      struct astc_block flushed_block = {
      .header = blocks[x].header,
   .dontcare0 = blocks[x].dontcare0,
   .dontcare1 = blocks[x].dontcare1,
   .dontcare2 = blocks[x].dontcare2,
   .R = blocks[x].R < 4 ? 0 : blocks[x].R,
   .G = blocks[x].G < 4 ? 0 : blocks[x].G,
   .B = blocks[x].B < 4 ? 0 : blocks[x].B,
      };
      } else {
                     dst += dst_stride;
         }
      void
   st_UnmapTextureImage(struct gl_context *ctx,
               {
               if (st_compressed_format_fallback(st, texImage->TexFormat)) {
      /* Decompress the compressed image on upload if the driver doesn't
   * support the compressed format. */
   unsigned z = slice + texImage->Face;
            if (itransfer->box.depth != 0) {
               /* Toggle logging for the different unmap paths. */
                  if (_mesa_is_format_astc_2d(texImage->TexFormat) &&
                     /* DXT5 is the only supported transcode target from ASTC. */
                  /* Try a compute-based transcode. */
   if (itransfer->box.x == 0 &&
      itransfer->box.y == 0 &&
   itransfer->box.width == texImage->Width &&
   itransfer->box.height == texImage->Height &&
   _mesa_has_compute_shaders(ctx) &&
   st_compute_transcode_astc_to_dxt5(st,
      itransfer->temp_data,
   itransfer->temp_stride,
   texImage->TexFormat,
                     if (log_unmap_time) {
                                                            struct pipe_transfer *transfer;
   GLubyte *map = st_texture_image_map(st, texImage,
                                                if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "compressed fallback map");
                        if (_mesa_is_format_astc_2d(texImage->pt->format)) {
      assert(st->astc_void_extents_need_denorm_flush);
   upload_astc_slice_with_flushed_void_extents(map, transfer->stride,
                              } else if (util_format_is_compressed(texImage->pt->format)) {
      /* Transcode into a different compressed format. */
   unsigned size =
      _mesa_format_image_size(PIPE_FORMAT_R8G8B8A8_UNORM,
                     /* Decompress to tmp. */
   if (texImage->TexFormat == MESA_FORMAT_ETC1_RGB8) {
      _mesa_etc1_unpack_rgba8888(tmp, transfer->box.width * 4,
                                          _mesa_unpack_etc2_format(tmp, transfer->box.width * 4,
                           itransfer->temp_data,
      } else if (_mesa_is_format_astc_2d(texImage->TexFormat)) {
      _mesa_unpack_astc_2d_ldr(tmp, transfer->box.width * 4,
                              } else {
                  /* Compress it to the target format. */
                  _mesa_texstore(ctx, 2, GL_RGBA, texImage->pt->format,
                  transfer->stride, &map,
         } else {
      /* Decompress into an uncompressed format. */
   if (texImage->TexFormat == MESA_FORMAT_ETC1_RGB8) {
      _mesa_etc1_unpack_rgba8888(map, transfer->stride,
                                          _mesa_unpack_etc2_format(map, transfer->stride,
                              } else if (_mesa_is_format_astc_2d(texImage->TexFormat)) {
      _mesa_unpack_astc_2d_ldr(map, transfer->stride,
                        } else if (_mesa_is_format_s3tc(texImage->TexFormat)) {
      _mesa_unpack_s3tc(map, transfer->stride,
                        } else if (_mesa_is_format_rgtc(texImage->TexFormat) ||
            _mesa_unpack_rgtc(map, transfer->stride,
                        } else if (_mesa_is_format_bptc(texImage->TexFormat)) {
      _mesa_unpack_bptc(map, transfer->stride,
                        } else {
                              if (log_unmap_time) {
      log_unmap_time_delta(&itransfer->box, texImage, "CPU",
                           itransfer->temp_data = NULL;
      } else {
            }
         /**
   * Return default texture resource binding bitmask for the given format.
   */
   static GLuint
   default_bindings(struct st_context *st, enum pipe_format format)
   {
      struct pipe_screen *screen = st->screen;
   const unsigned target = PIPE_TEXTURE_2D;
            if (util_format_is_depth_or_stencil(format))
         else
            if (screen->is_format_supported(screen, format, target, 0, 0, bindings))
         else {
      /* Try non-sRGB. */
            if (screen->is_format_supported(screen, format, target, 0, 0, bindings))
         else
         }
         /**
   * Given the size of a mipmap image, try to compute the size of the level=0
   * mipmap image.
   *
   * Note that this isn't always accurate for odd-sized, non-POW textures.
   * For example, if level=1 and width=40 then the level=0 width may be 80 or 81.
   *
   * \return GL_TRUE for success, GL_FALSE for failure
   */
   static GLboolean
   guess_base_level_size(GLenum target,
               {
      assert(width >= 1);
   assert(height >= 1);
            if (level > 0) {
      /* Guess the size of the base level.
   * Depending on the image's size, we can't always make a guess here.
   */
   switch (target) {
   case GL_TEXTURE_1D:
   case GL_TEXTURE_1D_ARRAY:
                  case GL_TEXTURE_2D:
   case GL_TEXTURE_2D_ARRAY:
      /* We can't make a good guess here, because the base level dimensions
   * can be non-square.
   */
   if (width == 1 || height == 1) {
         }
   width <<= level;
               case GL_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_ARRAY:
      width <<= level;
               case GL_TEXTURE_3D:
      /* We can't make a good guess here, because the base level dimensions
   * can be non-cube.
   */
   if (width == 1 || height == 1 || depth == 1) {
         }
   width <<= level;
   height <<= level;
               case GL_TEXTURE_RECTANGLE:
            default:
                     *width0 = width;
   *height0 = height;
               }
         /**
   * Try to determine whether we should allocate memory for a full texture
   * mipmap.  The problem is when we get a glTexImage(level=0) call, we
   * can't immediately know if other mipmap levels are coming next.  Here
   * we try to guess whether to allocate memory for a mipmap or just the
   * 0th level.
   *
   * If we guess incorrectly here we'll later reallocate the right amount of
   * memory either in st_AllocTextureImageBuffer() or st_finalize_texture().
   *
   * \param stObj  the texture object we're going to allocate memory for.
   * \param stImage  describes the incoming image which we need to store.
   */
   static bool
   allocate_full_mipmap(const struct gl_texture_object *stObj,
         {
      switch (stObj->Target) {
   case GL_TEXTURE_RECTANGLE_NV:
   case GL_TEXTURE_BUFFER:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
      /* these texture types cannot be mipmapped */
               if (stImage->Level > 0 || stObj->Attrib.GenerateMipmap)
            /* If the application has explicitly called glTextureParameter to set
   * GL_TEXTURE_MAX_LEVEL, such that (max - base) > 0, then they're trying
   * to communicate that they will have multiple miplevels.
   *
   * Core Mesa will initialize MaxLevel to value much larger than
   * MAX_TEXTURE_LEVELS, so we check that to see if it's been set at all.
   */
   if (stObj->Attrib.MaxLevel < MAX_TEXTURE_LEVELS &&
      stObj->Attrib.MaxLevel - stObj->Attrib.BaseLevel > 0)
         if (stImage->_BaseFormat == GL_DEPTH_COMPONENT ||
      stImage->_BaseFormat == GL_DEPTH_STENCIL_EXT)
   /* depth/stencil textures are seldom mipmapped */
         if (stObj->Attrib.BaseLevel == 0 && stObj->Attrib.MaxLevel == 0)
            if (stObj->Sampler.Attrib.MinFilter == GL_NEAREST ||
      stObj->Sampler.Attrib.MinFilter == GL_LINEAR)
   /* not a mipmap minification filter */
         /* If the following sequence of GL calls is used:
   *   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, ...
   *   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   *
   * we would needlessly allocate a mipmapped texture, because the initial
   * MinFilter is GL_NEAREST_MIPMAP_LINEAR. Catch this case and don't
   * allocate a mipmapped texture by default. This may cause texture
   * reallocation later, but GL_NEAREST_MIPMAP_LINEAR is pretty rare.
   */
   if (stObj->Sampler.Attrib.MinFilter == GL_NEAREST_MIPMAP_LINEAR)
            if (stObj->Target == GL_TEXTURE_3D)
      /* 3D textures are seldom mipmapped */
            }
         /**
   * Try to allocate a pipe_resource object for the given gl_texture_object.
   *
   * We use the given st_texture_image as a clue to determine the size of the
   * mipmap image at level=0.
   *
   * \return GL_TRUE for success, GL_FALSE if out of memory.
   */
   static GLboolean
   guess_and_alloc_texture(struct st_context *st,
               {
      const struct gl_texture_image *firstImage;
   GLuint lastLevel, width, height, depth;
   GLuint bindings;
   unsigned ptWidth;
   uint16_t ptHeight, ptDepth, ptLayers;
   enum pipe_format fmt;
                              /* If a base level image with compatible size exists, use that as our guess.
   */
   firstImage = _mesa_base_tex_image(stObj);
   if (firstImage &&
      firstImage->Width2 > 0 &&
   firstImage->Height2 > 0 &&
   firstImage->Depth2 > 0 &&
   guess_base_level_size(stObj->Target,
                        firstImage->Width2,
      if (stImage->Width2 == u_minify(width, stImage->Level) &&
      stImage->Height2 == u_minify(height, stImage->Level) &&
   stImage->Depth2 == u_minify(depth, stImage->Level))
            if (!guessed_box)
      guessed_box = guess_base_level_size(stObj->Target,
                                 if (!guessed_box) {
      /* we can't determine the image size at level=0 */
   /* this is not an out of memory error */
               /* At this point, (width x height x depth) is the expected size of
   * the level=0 mipmap image.
            /* Guess a reasonable value for lastLevel.  With OpenGL we have no
   * idea how many mipmap levels will be in a texture until we start
   * to render with it.  Make an educated guess here but be prepared
   * to re-allocating a texture buffer with space for more (or fewer)
   * mipmap levels later.
   */
   if (allocate_full_mipmap(stObj, stImage)) {
      /* alloc space for a full mipmap */
   lastLevel = _mesa_get_tex_max_num_levels(stObj->Target,
      }
   else {
      /* only alloc space for a single mipmap level */
                                 st_gl_texture_dims_to_pipe_dims(stObj->Target,
                  enum pipe_texture_target target = gl_target_to_pipe(stObj->Target);
   unsigned nr_samples = 0;
   if (stObj->TargetIndex == TEXTURE_2D_MULTISAMPLE_INDEX ||
      stObj->TargetIndex == TEXTURE_2D_MULTISAMPLE_ARRAY_INDEX) {
   int samples[16];
   st_QueryInternalFormat(st->ctx, 0, stImage->InternalFormat, GL_SAMPLES, samples);
      }
   stObj->pt = st_texture_create(st,
                                 target,
   fmt,
   lastLevel,
                                 }
         /**
   * If the texture object/buffer already has space for the indicated image,
   * we're done.  Otherwise, allocate memory for the new texture image.
   */
   GLboolean
   st_AllocTextureImageBuffer(struct gl_context *ctx,
         {
      struct st_context *st = st_context(ctx);
   struct gl_texture_image *stImage = texImage;
   struct gl_texture_object *stObj = texImage->TexObject;
   GLuint width = texImage->Width;
   GLuint height = texImage->Height;
                                       compressed_tex_fallback_allocate(st, stImage);
   const bool allowAllocateToStObj = !stObj->pt ||
                  if (allowAllocateToStObj) {
      /* Look if the parent texture object has space for this image */
   if (stObj->pt &&
      st_texture_match_image(st, stObj->pt, texImage)) {
   /* this image will fit in the existing texture object's memory */
   pipe_resource_reference(&stImage->pt, stObj->pt);
   assert(stImage->pt);
                        pipe_resource_reference(&stObj->pt, NULL);
            if (!guess_and_alloc_texture(st, stObj, stImage)) {
      /* Probably out of memory.
   * Try flushing any pending rendering, then retry.
   */
   st_finish(st);
   if (!guess_and_alloc_texture(st, stObj, stImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage(internalformat=%s)",
                           if (stObj->pt &&
      st_texture_match_image(st, stObj->pt, texImage)) {
   /* The image will live in the object's mipmap memory */
   pipe_resource_reference(&stImage->pt, stObj->pt);
   assert(stImage->pt);
      }
   else {
      /* Create a new, temporary texture/resource/buffer to hold this
   * one texture image.  Note that when we later access this image
   * (either for mapping or copying) we'll want to always specify
   * mipmap level=0, even if the image represents some other mipmap
   * level.
   */
   enum pipe_format format =
         GLuint bindings = default_bindings(st, format);
   unsigned ptWidth;
            st_gl_texture_dims_to_pipe_dims(stObj->Target,
                  stImage->pt = st_texture_create(st,
                                 gl_target_to_pipe(stObj->Target),
   format,
   0, /* lastLevel */
   ptWidth,
         }
         /**
   * Preparation prior to glTexImage.  Basically check the 'surface_based'
   * field and switch to a "normal" tex image if necessary.
   */
   static void
   prep_teximage(struct gl_context *ctx, struct gl_texture_image *texImage,
         {
               /* switch to "normal" */
   if (texObj->surface_based) {
      const GLenum target = texObj->Target;
   const GLuint level = texImage->Level;
            assert(!texImage->pt);
   _mesa_clear_texture_object(ctx, texObj, texImage);
   texObj->layer_override = -1;
   texObj->level_override = -1;
            /* oops, need to init this image again */
   texFormat = _mesa_choose_texture_format(ctx, texObj, target, level,
                  _mesa_init_teximage_fields(ctx, texImage,
                        texObj->surface_based = GL_FALSE;
         }
         /**
   * Return a writemask for the gallium blit. The parameters can be base
   * formats or "format" from glDrawPixels/glTexImage/glGetTexImage.
   */
   unsigned
   st_get_blit_mask(GLenum srcFormat, GLenum dstFormat)
   {
      switch (dstFormat) {
   case GL_DEPTH_STENCIL:
      switch (srcFormat) {
   case GL_DEPTH_STENCIL:
         case GL_DEPTH_COMPONENT:
         case GL_STENCIL_INDEX:
         default:
      assert(0);
            case GL_DEPTH_COMPONENT:
      switch (srcFormat) {
   case GL_DEPTH_STENCIL:
   case GL_DEPTH_COMPONENT:
         default:
      assert(0);
            case GL_STENCIL_INDEX:
      switch (srcFormat) {
   case GL_DEPTH_STENCIL:
   case GL_STENCIL_INDEX:
         default:
      assert(0);
            default:
            }
      /**
   * Converts format to a format with the same components, types
   * and sizes, but with the components in RGBA order.
   */
   static enum pipe_format
   unswizzle_format(enum pipe_format format)
   {
      switch (format)
   {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_A8B8G8R8_UNORM:
            case PIPE_FORMAT_B10G10R10A2_UNORM:
            case PIPE_FORMAT_B10G10R10A2_SNORM:
            case PIPE_FORMAT_B10G10R10A2_UINT:
            default:
            }
         /**
   * Converts PIPE_FORMAT_A* to PIPE_FORMAT_R*.
   */
   static enum pipe_format
   alpha_to_red(enum pipe_format format)
   {
      switch (format)
   {
   case PIPE_FORMAT_A8_UNORM:
         case PIPE_FORMAT_A8_SNORM:
         case PIPE_FORMAT_A8_UINT:
         case PIPE_FORMAT_A8_SINT:
            case PIPE_FORMAT_A16_UNORM:
         case PIPE_FORMAT_A16_SNORM:
         case PIPE_FORMAT_A16_UINT:
         case PIPE_FORMAT_A16_SINT:
         case PIPE_FORMAT_A16_FLOAT:
            case PIPE_FORMAT_A32_UINT:
         case PIPE_FORMAT_A32_SINT:
         case PIPE_FORMAT_A32_FLOAT:
            default:
            }
         /**
   * Converts PIPE_FORMAT_R*A* to PIPE_FORMAT_R*G*.
   */
   static enum pipe_format
   red_alpha_to_red_green(enum pipe_format format)
   {
      switch (format)
   {
   case PIPE_FORMAT_R8A8_UNORM:
         case PIPE_FORMAT_R8A8_SNORM:
         case PIPE_FORMAT_R8A8_UINT:
         case PIPE_FORMAT_R8A8_SINT:
            case PIPE_FORMAT_R16A16_UNORM:
         case PIPE_FORMAT_R16A16_SNORM:
         case PIPE_FORMAT_R16A16_UINT:
         case PIPE_FORMAT_R16A16_SINT:
         case PIPE_FORMAT_R16A16_FLOAT:
            case PIPE_FORMAT_R32A32_UINT:
         case PIPE_FORMAT_R32A32_SINT:
         case PIPE_FORMAT_R32A32_FLOAT:
            default:
            }
         /**
   * Converts PIPE_FORMAT_L*A* to PIPE_FORMAT_R*G*.
   */
   static enum pipe_format
   luminance_alpha_to_red_green(enum pipe_format format)
   {
      switch (format)
   {
   case PIPE_FORMAT_L8A8_UNORM:
         case PIPE_FORMAT_L8A8_SNORM:
         case PIPE_FORMAT_L8A8_UINT:
         case PIPE_FORMAT_L8A8_SINT:
            case PIPE_FORMAT_L16A16_UNORM:
         case PIPE_FORMAT_L16A16_SNORM:
         case PIPE_FORMAT_L16A16_UINT:
         case PIPE_FORMAT_L16A16_SINT:
         case PIPE_FORMAT_L16A16_FLOAT:
            case PIPE_FORMAT_L32A32_UINT:
         case PIPE_FORMAT_L32A32_SINT:
         case PIPE_FORMAT_L32A32_FLOAT:
            default:
            }
         /**
   * Returns true if format is a PIPE_FORMAT_A* format, and false otherwise.
   */
   static bool
   format_is_alpha(enum pipe_format format)
   {
               if (desc->nr_channels == 1 &&
      desc->swizzle[0] == PIPE_SWIZZLE_0 &&
   desc->swizzle[1] == PIPE_SWIZZLE_0 &&
   desc->swizzle[2] == PIPE_SWIZZLE_0 &&
   desc->swizzle[3] == PIPE_SWIZZLE_X)
            }
         /**
   * Returns true if format is a PIPE_FORMAT_R* format, and false otherwise.
   */
   static bool
   format_is_red(enum pipe_format format)
   {
               if (desc->nr_channels == 1 &&
      desc->swizzle[0] == PIPE_SWIZZLE_X &&
   desc->swizzle[1] == PIPE_SWIZZLE_0 &&
   desc->swizzle[2] == PIPE_SWIZZLE_0 &&
   desc->swizzle[3] == PIPE_SWIZZLE_1)
            }
         /**
   * Returns true if format is a PIPE_FORMAT_L* format, and false otherwise.
   */
   static bool
   format_is_luminance(enum pipe_format format)
   {
               if (desc->nr_channels == 1 &&
      desc->swizzle[0] == PIPE_SWIZZLE_X &&
   desc->swizzle[1] == PIPE_SWIZZLE_X &&
   desc->swizzle[2] == PIPE_SWIZZLE_X &&
   desc->swizzle[3] == PIPE_SWIZZLE_1)
            }
      /**
   * Returns true if format is a PIPE_FORMAT_R*A* format, and false otherwise.
   */
   static bool
   format_is_red_alpha(enum pipe_format format)
   {
               if (desc->nr_channels == 2 &&
      desc->swizzle[0] == PIPE_SWIZZLE_X &&
   desc->swizzle[1] == PIPE_SWIZZLE_0 &&
   desc->swizzle[2] == PIPE_SWIZZLE_0 &&
   desc->swizzle[3] == PIPE_SWIZZLE_Y)
            }
         static bool
   format_is_swizzled_rgba(enum pipe_format format)
   {
               if ((desc->swizzle[0] == TGSI_SWIZZLE_X || desc->swizzle[0] == PIPE_SWIZZLE_0) &&
      (desc->swizzle[1] == TGSI_SWIZZLE_Y || desc->swizzle[1] == PIPE_SWIZZLE_0) &&
   (desc->swizzle[2] == TGSI_SWIZZLE_Z || desc->swizzle[2] == PIPE_SWIZZLE_0) &&
   (desc->swizzle[3] == TGSI_SWIZZLE_W || desc->swizzle[3] == PIPE_SWIZZLE_1))
            }
         struct format_table
   {
      unsigned char swizzle[4];
      };
      static const struct format_table table_8888_unorm[] = {
      { { 0, 1, 2, 3 }, PIPE_FORMAT_R8G8B8A8_UNORM },
   { { 2, 1, 0, 3 }, PIPE_FORMAT_B8G8R8A8_UNORM },
   { { 3, 0, 1, 2 }, PIPE_FORMAT_A8R8G8B8_UNORM },
      };
      static const struct format_table table_1010102_unorm[] = {
      { { 0, 1, 2, 3 }, PIPE_FORMAT_R10G10B10A2_UNORM },
      };
      static const struct format_table table_1010102_snorm[] = {
      { { 0, 1, 2, 3 }, PIPE_FORMAT_R10G10B10A2_SNORM },
      };
      static const struct format_table table_1010102_uint[] = {
      { { 0, 1, 2, 3 }, PIPE_FORMAT_R10G10B10A2_UINT },
      };
      static enum pipe_format
   swizzle_format(enum pipe_format format, const int * const swizzle)
   {
               switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_A8B8G8R8_UNORM:
      for (i = 0; i < ARRAY_SIZE(table_8888_unorm); i++) {
      if (swizzle[0] == table_8888_unorm[i].swizzle[0] &&
      swizzle[1] == table_8888_unorm[i].swizzle[1] &&
   swizzle[2] == table_8888_unorm[i].swizzle[2] &&
   swizzle[3] == table_8888_unorm[i].swizzle[3])
   }
         case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
      for (i = 0; i < ARRAY_SIZE(table_1010102_unorm); i++) {
      if (swizzle[0] == table_1010102_unorm[i].swizzle[0] &&
      swizzle[1] == table_1010102_unorm[i].swizzle[1] &&
   swizzle[2] == table_1010102_unorm[i].swizzle[2] &&
   swizzle[3] == table_1010102_unorm[i].swizzle[3])
   }
         case PIPE_FORMAT_R10G10B10A2_SNORM:
   case PIPE_FORMAT_B10G10R10A2_SNORM:
      for (i = 0; i < ARRAY_SIZE(table_1010102_snorm); i++) {
      if (swizzle[0] == table_1010102_snorm[i].swizzle[0] &&
      swizzle[1] == table_1010102_snorm[i].swizzle[1] &&
   swizzle[2] == table_1010102_snorm[i].swizzle[2] &&
   swizzle[3] == table_1010102_snorm[i].swizzle[3])
   }
         case PIPE_FORMAT_R10G10B10A2_UINT:
   case PIPE_FORMAT_B10G10R10A2_UINT:
      for (i = 0; i < ARRAY_SIZE(table_1010102_uint); i++) {
      if (swizzle[0] == table_1010102_uint[i].swizzle[0] &&
      swizzle[1] == table_1010102_uint[i].swizzle[1] &&
   swizzle[2] == table_1010102_uint[i].swizzle[2] &&
   swizzle[3] == table_1010102_uint[i].swizzle[3])
   }
         default:
                     }
      static bool
   reinterpret_formats(enum pipe_format *src_format, enum pipe_format *dst_format)
   {
      enum pipe_format src = *src_format;
            /* Note: dst_format has already been transformed from luminance/intensity
   *       to red when this function is called.  The source format will never
   *       be an intensity format, because GL_INTENSITY is not a legal value
            if (format_is_alpha(src)) {
      if (!format_is_alpha(dst))
            src = alpha_to_red(src);
      } else if (format_is_luminance(src)) {
      if (!format_is_red(dst) && !format_is_red_alpha(dst))
               } else if (util_format_is_luminance_alpha(src)) {
               if (format_is_red_alpha(dst)) {
         } else if (!format_is_red(dst))
      } else if (format_is_swizzled_rgba(src)) {
      const struct util_format_description *src_desc = util_format_description(src);
   const struct util_format_description *dst_desc = util_format_description(dst);
   int swizzle[4];
            /* Make sure the format is an RGBA and not an RGBX format */
   if (src_desc->nr_channels != 4 || src_desc->swizzle[3] == PIPE_SWIZZLE_1)
            if (dst_desc->nr_channels != 4 || dst_desc->swizzle[3] == PIPE_SWIZZLE_1)
            for (i = 0; i < 4; i++)
            dst = swizzle_format(dst, swizzle);
   if (dst == PIPE_FORMAT_NONE)
                        *src_format = src;
   *dst_format = dst;
      }
      static bool
   try_pbo_upload_common(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   struct cso_context *cso = st->cso_context;
   struct pipe_context *pipe = st->pipe;
   bool success = false;
            fs = st_pbo_get_upload_fs(st, src_format, surface->format, addr->depth != 1);
   if (!fs)
            cso_save_state(cso, (CSO_BIT_VERTEX_ELEMENTS |
                        CSO_BIT_FRAMEBUFFER |
   CSO_BIT_VIEWPORT |
   CSO_BIT_BLEND |
   CSO_BIT_DEPTH_STENCIL_ALPHA |
   CSO_BIT_RASTERIZER |
   CSO_BIT_STREAM_OUTPUTS |
   (st->active_queries ? CSO_BIT_PAUSE_QUERIES : 0) |
         cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
            /* Set up the sampler_view */
   {
      struct pipe_sampler_view templ;
            memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_BUFFER;
   templ.format = src_format;
   templ.u.buf.offset = addr->first_element * addr->bytes_per_pixel;
   templ.u.buf.size = (addr->last_element - addr->first_element + 1) *
         templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
            sampler_view = pipe->create_sampler_view(pipe, addr->buffer, &templ);
   if (sampler_view == NULL)
            pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0,
         st->state.num_sampler_views[PIPE_SHADER_FRAGMENT] =
                        /* Framebuffer_state */
   {
      struct pipe_framebuffer_state fb;
   memset(&fb, 0, sizeof(fb));
   fb.width = surface->width;
   fb.height = surface->height;
   fb.nr_cbufs = 1;
                                 /* Blend state */
            /* Depth/stencil/alpha state */
   {
      struct pipe_depth_stencil_alpha_state dsa;
   memset(&dsa, 0, sizeof(dsa));
               /* Set up the fragment shader */
                  fail:
      /* Unbind all because st/mesa won't do it if the current shader doesn't
   * use them.
   */
   cso_restore_state(cso, CSO_UNBIND_FS_SAMPLERVIEWS);
            ctx->Array.NewVertexElements = true;
   ctx->NewDriverState |= ST_NEW_VERTEX_ARRAYS |
                     }
         static bool
   try_pbo_upload(struct gl_context *ctx, GLuint dims,
                  struct gl_texture_image *texImage,
   GLenum format, GLenum type,
   enum pipe_format dst_format,
   GLint xoffset, GLint yoffset, GLint zoffset,
      {
      struct st_context *st = st_context(ctx);
   struct gl_texture_image *stImage = texImage;
   struct gl_texture_object *stObj = texImage->TexObject;
   struct pipe_resource *texture = stImage->pt;
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = st->screen;
   struct pipe_surface *surface = NULL;
   struct st_pbo_addresses addr;
   enum pipe_format src_format;
   const struct util_format_description *desc;
   GLenum gl_target = texImage->TexObject->Target;
            if (!st->pbo.upload_enabled)
            /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
   height = 1;
   zoffset = yoffset;
               if (depth != 1 && !st->pbo.layers)
            /* Choose the source format. Initially, we do so without checking driver
   * support at all because of the remapping we later perform and because
   * at least the Radeon driver actually supports some formats for texture
   * buffers which it doesn't support for regular textures. */
   src_format = st_choose_matching_format(st, 0, format, type,
         if (!src_format) {
                  src_format = util_format_linear(src_format);
            if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN)
            if (desc->colorspace != UTIL_FORMAT_COLORSPACE_RGB)
            if (st->pbo.rgba_only) {
               if (!reinterpret_formats(&src_format, &dst_format)) {
                  if (dst_format != orig_dst_format &&
      !screen->is_format_supported(screen, dst_format, PIPE_TEXTURE_2D, 0,
                        if (!src_format ||
      !screen->is_format_supported(screen, src_format, PIPE_BUFFER, 0, 0,
                     /* Compute buffer addresses */
   addr.xoffset = xoffset;
   addr.yoffset = yoffset;
   addr.width = width;
   addr.height = height;
   addr.depth = depth;
            if (!st_pbo_addresses_pixelstore(st, gl_target, dims == 3, unpack, pixels,
                  /* Set up the surface */
   {
      unsigned level = stObj->pt != stImage->pt
                           struct pipe_surface templ;
   memset(&templ, 0, sizeof(templ));
   templ.format = dst_format;
   templ.u.tex.level = level;
   templ.u.tex.first_layer = MIN2(zoffset, max_layer);
            surface = pipe->create_surface(pipe, texture, &templ);
   if (!surface)
                                    }
      static bool
   try_pbo_download(struct st_context *st,
                     struct gl_texture_image *texImage,
   enum pipe_format src_format, enum pipe_format dst_format,
   {
      struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_resource *texture = texImage->pt;
   struct cso_context *cso = st->cso_context;
   const struct util_format_description *desc;
   struct st_pbo_addresses addr;
   struct pipe_framebuffer_state fb;
   enum pipe_texture_target pipe_target;
   GLenum gl_target = texImage->TexObject->Target;
   GLuint dims;
            if (texture->nr_samples > 1)
            /* GetTexImage only returns a single face for cubemaps. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
         }
   if (gl_target == GL_TEXTURE_CUBE_MAP_ARRAY) {
         }
   pipe_target = gl_target_to_pipe(gl_target);
            /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      depth = height;
   height = 1;
   zoffset = yoffset;
               if (depth != 1 && !st->pbo.layers)
            if (!screen->is_format_supported(screen, dst_format, PIPE_BUFFER, 0, 0,
            util_format_is_compressed(src_format) ||
   util_format_is_compressed(dst_format))
                  /* Compute PBO addresses */
   addr.bytes_per_pixel = desc->block.bits / 8;
   addr.xoffset = xoffset;
   addr.yoffset = yoffset;
   addr.width = width;
   addr.height = height;
   addr.depth = depth;
   if (!st_pbo_addresses_pixelstore(st, gl_target, dims == 3, pack, pixels, &addr))
            cso_save_state(cso, (CSO_BIT_VERTEX_ELEMENTS |
                        CSO_BIT_FRAMEBUFFER |
   CSO_BIT_VIEWPORT |
   CSO_BIT_BLEND |
   CSO_BIT_DEPTH_STENCIL_ALPHA |
   CSO_BIT_RASTERIZER |
   CSO_BIT_STREAM_OUTPUTS |
   (st->active_queries ? CSO_BIT_PAUSE_QUERIES : 0) |
         cso_set_sample_mask(cso, ~0);
   cso_set_min_samples(cso, 1);
            /* Set up the sampler_view */
   {
      struct pipe_sampler_view templ;
   struct pipe_sampler_view *sampler_view;
   struct pipe_sampler_state sampler = {0};
   const struct pipe_sampler_state *samplers[1] = {&sampler};
   unsigned level = texImage->TexObject->Attrib.MinLevel + texImage->Level;
                     templ.target = pipe_target;
   templ.u.tex.first_level = level;
            zoffset += texImage->Face + texImage->TexObject->Attrib.MinLayer;
   templ.u.tex.first_layer = MIN2(zoffset, max_layer);
            sampler_view = pipe->create_sampler_view(pipe, texture, &templ);
   if (sampler_view == NULL)
            pipe->set_sampler_views(pipe, PIPE_SHADER_FRAGMENT, 0, 1, 0, true, &sampler_view);
                        /* Set up destination image */
   {
               memset(&image, 0, sizeof(image));
   image.resource = addr.buffer;
   image.format = dst_format;
   image.access = PIPE_IMAGE_ACCESS_WRITE;
   image.shader_access = PIPE_IMAGE_ACCESS_WRITE;
   image.u.buf.offset = addr.first_element * addr.bytes_per_pixel;
   image.u.buf.size = (addr.last_element - addr.first_element + 1) *
                        /* Set up no-attachment framebuffer */
   memset(&fb, 0, sizeof(fb));
   fb.width = texture->width0;
   fb.height = texture->height0;
   fb.layers = 1;
   fb.samples = 1;
            /* Any blend state would do. Set this just to prevent drivers having
   * blend == NULL.
   */
                     {
      struct pipe_depth_stencil_alpha_state dsa;
   memset(&dsa, 0, sizeof(dsa));
               /* Set up the fragment shader */
   {
      void *fs = st_pbo_get_download_fs(st, pipe_target, src_format, dst_format, addr.depth != 1);
   if (!fs)
                                 /* Buffer written via shader images needs explicit synchronization. */
         fail:
      /* Unbind all because st/mesa won't do it if the current shader doesn't
   * use them.
   */
   cso_restore_state(cso, CSO_UNBIND_FS_SAMPLERVIEWS | CSO_UNBIND_FS_IMAGE0);
            st->ctx->Array.NewVertexElements = true;
   st->ctx->NewDriverState |= ST_NEW_FS_CONSTANTS |
                           }
         void
   st_TexSubImage(struct gl_context *ctx, GLuint dims,
                  struct gl_texture_image *texImage,
   GLint xoffset, GLint yoffset, GLint zoffset,
      {
      struct st_context *st = st_context(ctx);
   struct gl_texture_object *stObj = texImage->TexObject;
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = st->screen;
   struct pipe_resource *dst = texImage->pt;
   struct pipe_resource *src = NULL;
   struct pipe_resource src_templ;
   struct pipe_transfer *transfer;
   struct pipe_blit_info blit;
   enum pipe_format src_format, dst_format;
   mesa_format mesa_src_format;
   GLenum gl_target = texImage->TexObject->Target;
   unsigned bind;
   GLubyte *map;
   unsigned dstz = texImage->Face + texImage->TexObject->Attrib.MinLayer;
   unsigned dst_level = 0;
   bool is_ms;
            st_flush_bitmap_cache(st);
            if (stObj->pt == texImage->pt)
            assert(!_mesa_is_format_etc2(texImage->TexFormat) &&
                  if (!dst)
                     /* Try texture_subdata, which should be the fastest memcpy path. */
   if (pixels &&
      !unpack->BufferObj &&
   !is_ms &&
   _mesa_texstore_can_use_memcpy(ctx, texImage->_BaseFormat,
               struct pipe_box box;
   unsigned stride;
   intptr_t layer_stride;
            stride = _mesa_image_row_stride(unpack, width, format, type);
   layer_stride = _mesa_image_image_stride(unpack, width, height, format,
         data = _mesa_image_address(dims, unpack, pixels, width, height, format,
            /* Convert to Gallium coordinates. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      zoffset = yoffset;
   yoffset = 0;
   depth = height;
   height = 1;
               util_throttle_memory_usage(pipe, &st->throttle,
                  u_box_3d(xoffset, yoffset, zoffset + dstz, width, height, depth, &box);
   pipe->texture_subdata(pipe, dst, dst_level, 0,
                     if (!st->prefer_blit_based_texture_transfer) {
                  /* If the base internal format and the texture format don't match,
   * we can't use blit-based TexSubImage. */
   if (texImage->_BaseFormat !=
      _mesa_get_format_base_format(texImage->TexFormat)) {
               /* We need both the compressed and non-compressed textures updated,
   * which neither the PBO nor memcpy code-paths does */
   if (st_compressed_format_fallback(st, texImage->TexFormat)) {
                  /* See if the destination format is supported. */
   if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
         else
            /* For luminance and intensity, only the red channel is stored
   * in the destination. */
   dst_format = util_format_linear(dst->format);
   dst_format = util_format_luminance_to_red(dst_format);
            if (!dst_format ||
      !screen->is_format_supported(screen, dst_format, dst->target,
                           if (unpack->BufferObj) {
      if (try_pbo_upload(ctx, dims, texImage, format, type, dst_format,
                           /* See if the texture format already matches the format and type,
   * in which case the memcpy-based fast path will likely be used and
   * we don't have to blit. */
   if (_mesa_format_matches_format_and_type(texImage->TexFormat, format,
                        /* Choose the source format. */
   src_format = st_choose_matching_format(st, PIPE_BIND_SAMPLER_VIEW,
         if (!src_format) {
                           /* There is no reason to do this if we cannot use memcpy for the temporary
   * source texture at least. This also takes transfer ops into account,
   * etc. */
   if (!_mesa_texstore_can_use_memcpy(ctx,
                              /* TexSubImage only sets a single cubemap face. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
         }
   /* TexSubImage can specify subsets of cube map array faces
   * so we need to upload via 2D array instead */
   if (gl_target == GL_TEXTURE_CUBE_MAP_ARRAY) {
                  /* Initialize the source texture description. */
   memset(&src_templ, 0, sizeof(src_templ));
   src_templ.target = gl_target_to_pipe(gl_target);
   src_templ.format = src_format;
   src_templ.bind = PIPE_BIND_SAMPLER_VIEW;
            st_gl_texture_dims_to_pipe_dims(gl_target, width, height, depth,
                  /* Check for NPOT texture support. */
   if (!screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES) &&
      (!util_is_power_of_two_or_zero(src_templ.width0) ||
   !util_is_power_of_two_or_zero(src_templ.height0) ||
   !util_is_power_of_two_or_zero(src_templ.depth0))) {
               util_throttle_memory_usage(pipe, &st->throttle,
                        /* Create the source texture. */
   src = screen->resource_create(screen, &src_templ);
   if (!src) {
                  /* Map source pixels. */
   pixels = _mesa_validate_pbo_teximage(ctx, dims, width, height, depth,
               if (!pixels) {
      /* This is a GL error. */
   pipe_resource_reference(&src, NULL);
               /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      zoffset = yoffset;
   yoffset = 0;
   depth = height;
               map = pipe_texture_map_3d(pipe, src, 0, PIPE_MAP_WRITE, 0, 0, 0,
         if (!map) {
      _mesa_unmap_teximage_pbo(ctx, unpack);
   pipe_resource_reference(&src, NULL);
               /* Upload pixels (just memcpy). */
   {
      const uint bytesPerRow = width * util_format_get_blocksize(src_format);
            for (slice = 0; slice < (unsigned) depth; slice++) {
      if (gl_target == GL_TEXTURE_1D_ARRAY) {
      /* 1D array textures.
   * We need to convert gallium coords to GL coords.
   */
   void *src = _mesa_image_address2d(unpack, pixels,
                  }
                     for (row = 0; row < (unsigned) height; row++) {
      void *src = _mesa_image_address(dims, unpack, pixels,
               memcpy(slice_map, src, bytesPerRow);
         }
                  pipe_texture_unmap(pipe, transfer);
            /* Blit. */
   memset(&blit, 0, sizeof(blit));
   blit.src.resource = src;
   blit.src.level = 0;
   blit.src.format = src_format;
   blit.dst.resource = dst;
   blit.dst.level = dst_level;
   blit.dst.format = dst_format;
   blit.src.box.x = blit.src.box.y = blit.src.box.z = 0;
   blit.dst.box.x = xoffset;
   blit.dst.box.y = yoffset;
   blit.dst.box.z = zoffset + dstz;
   blit.src.box.width = blit.dst.box.width = width;
   blit.src.box.height = blit.dst.box.height = height;
   blit.src.box.depth = blit.dst.box.depth = depth;
   blit.mask = st_get_blit_mask(format, texImage->_BaseFormat);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
                     pipe_resource_reference(&src, NULL);
         fallback:
      if (!throttled) {
      util_throttle_memory_usage(pipe, &st->throttle,
            }
   _mesa_store_texsubimage(ctx, dims, texImage, xoffset, yoffset, zoffset,
            }
         void
   st_TexImage(struct gl_context * ctx, GLuint dims,
               struct gl_texture_image *texImage,
   {
                        if (texImage->Width == 0 || texImage->Height == 0 || texImage->Depth == 0)
            /* allocate storage for texture data */
   if (!st_AllocTextureImageBuffer(ctx, texImage)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage%uD(internalformat=%s)",
                        st_TexSubImage(ctx, dims, texImage, 0, 0, 0,
            }
      static bool
   st_try_pbo_compressed_texsubimage(struct gl_context *ctx,
                                 {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_pbo_addresses addr;
   struct pipe_surface *surface = NULL;
            addr = *addr_tmpl;
   if (!st_pbo_addresses_setup(st, buf, buf_offset, &addr))
            surface = pipe->create_surface(pipe, texture, surface_templ);
   if (!surface)
                                 }
      void
   st_CompressedTexSubImage(struct gl_context *ctx, GLuint dims,
                           {
      struct st_context *st = st_context(ctx);
   struct gl_texture_image *stImage = texImage;
   struct gl_texture_object *stObj = texImage->TexObject;
   struct pipe_resource *buf;
   struct pipe_resource *texture = stImage->pt;
   struct pipe_screen *screen = st->screen;
   struct pipe_resource *dst = stImage->pt;
   struct pipe_surface templ;
   struct compressed_pixelstore store;
   struct st_pbo_addresses addr;
   enum pipe_format copy_format;
   unsigned bw, bh, level, max_layer;
   int layer;
   intptr_t buf_offset;
            /* Check basic pre-conditions for PBO upload */
   if (!st->prefer_blit_based_texture_transfer) {
                  if (!ctx->Unpack.BufferObj)
            if (st_compressed_format_fallback(st, texImage->TexFormat))
            if (!dst) {
                  if (!st->pbo.upload_enabled ||
      !screen->get_param(screen, PIPE_CAP_SURFACE_REINTERPRET_BLOCKS)) {
               /* Choose the pipe format for the upload. */
   addr.bytes_per_pixel = util_format_get_blocksize(dst->format);
   bw = util_format_get_blockwidth(dst->format);
            switch (addr.bytes_per_pixel) {
   case 8:
      copy_format = PIPE_FORMAT_R16G16B16A16_UINT;
      case 16:
      copy_format = PIPE_FORMAT_R32G32B32A32_UINT;
      default:
                  if (!screen->is_format_supported(screen, copy_format, PIPE_BUFFER, 0, 0,
                        if (!screen->is_format_supported(screen, copy_format, dst->target,
                              /* Interpret the pixelstore settings. */
   _mesa_compute_compressed_pixelstore(dims, texImage->TexFormat, w, h, d,
         assert(store.CopyBytesPerRow % addr.bytes_per_pixel == 0);
            /* Compute the offset into the buffer */
            if (buf_offset % addr.bytes_per_pixel) {
                                    addr.xoffset = x / bw;
   addr.yoffset = y / bh;
   addr.width = store.CopyBytesPerRow / addr.bytes_per_pixel;
   addr.height = store.CopyRowsPerSlice;
   addr.depth = d;
   addr.pixels_per_row = store.TotalBytesPerRow / addr.bytes_per_pixel;
            /* Set up the surface. */
   level = stObj->pt != stImage->pt
         max_layer = util_max_layer(texture, level);
            memset(&templ, 0, sizeof(templ));
   templ.format = copy_format;
   templ.u.tex.level = level;
   templ.u.tex.first_layer = MIN2(layer, max_layer);
            if (st_try_pbo_compressed_texsubimage(ctx, buf, buf_offset, &addr,
                  /* Some drivers can re-interpret surfaces but only one layer at a time.
   * Fall back to doing a single try_pbo_upload_common per layer.
   */
   while (layer <= max_layer) {
      templ.u.tex.first_layer = MIN2(layer, max_layer);
   templ.u.tex.last_layer = templ.u.tex.first_layer;
   if (!st_try_pbo_compressed_texsubimage(ctx, buf, buf_offset, &addr,
                  /* By incrementing layer here, we ensure the fallback only uploads
   * layers we failed to upload.
   */
   buf_offset += addr.pixels_per_row * addr.image_height;
   layer++;
               if (success)
         fallback:
      _mesa_store_compressed_texsubimage(ctx, dims, texImage,
            }
         void
   st_CompressedTexImage(struct gl_context *ctx, GLuint dims,
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
   * Called via ctx->Driver.GetTexSubImage()
   *
   * This uses a blit to copy the texture to a texture format which matches
   * the format and type combo and then a fast read-back is done using memcpy.
   * We can do arbitrary X/Y/Z/W/0/1 swizzling here as long as there is
   * a format which matches the swizzling.
   *
   * If such a format isn't available, it falls back to _mesa_GetTexImage_sw.
   *
   * NOTE: Drivers usually do a blit to convert between tiled and linear
   *       texture layouts during texture uploads/downloads, so the blit
   *       we do here should be free in such cases.
   */
   void
   st_GetTexSubImage(struct gl_context * ctx,
                     GLint xoffset, GLint yoffset, GLint zoffset,
   {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
   struct gl_texture_image *stImage = texImage;
   struct gl_texture_object *stObj = texImage->TexObject;
   struct pipe_resource *src = stObj->pt;
   struct pipe_resource *dst = NULL;
   enum pipe_format dst_format, src_format;
   GLenum gl_target = texImage->TexObject->Target;
   enum pipe_texture_target pipe_target;
   struct pipe_blit_info blit;
   unsigned bind;
            assert(!_mesa_is_format_etc2(texImage->TexFormat) &&
                  st_flush_bitmap_cache(st);
   if (st->force_compute_based_texture_transfer)
            /* GetTexImage only returns a single face for cubemaps. */
   if (gl_target == GL_TEXTURE_CUBE_MAP) {
         }
            if (!st->prefer_blit_based_texture_transfer &&
      !_mesa_is_format_compressed(texImage->TexFormat)) {
   /* Try to avoid the non_blit_transfer if we're doing texture decompression here */
               if (stImage->pt != stObj->pt)
            /* Handle non-finalized textures. */
   if (!stImage->pt || !src) {
                  /* XXX Fallback to _mesa_GetTexImage_sw for depth-stencil formats
   * due to an incomplete stencil blit implementation in some drivers. */
   if (format == GL_DEPTH_STENCIL || format == GL_STENCIL_INDEX) {
                  /* If the base internal format and the texture format don't match, we have
   * to fall back to _mesa_GetTexImage_sw. */
   if (texImage->_BaseFormat !=
      _mesa_get_format_base_format(texImage->TexFormat)) {
               src_format = st_pbo_get_src_format(screen, stObj->surface_based ? stObj->surface_format : src->format, src);
   if (src_format == PIPE_FORMAT_NONE)
            if (format == GL_DEPTH_COMPONENT || format == GL_DEPTH_STENCIL)
         else
            dst_format = st_pbo_get_dst_format(ctx, pipe_target, src_format, util_format_is_compressed(src->format),
         if (dst_format == PIPE_FORMAT_NONE)
            if (st->pbo.download_enabled && ctx->Pack.BufferObj) {
      if (try_pbo_download(st, texImage,
                        src_format, dst_format,
            /* See if the texture format already matches the format and type,
   * in which case the memcpy-based fast path will be used. */
   if (_mesa_format_matches_format_and_type(texImage->TexFormat, format,
                  dst = create_dst_texture(ctx, dst_format, pipe_target, width, height, depth, gl_target, bind);
   if (!dst)
            /* From now on, we need the gallium representation of dimensions. */
   if (gl_target == GL_TEXTURE_1D_ARRAY) {
      zoffset = yoffset;
   yoffset = 0;
   depth = height;
               assert(texImage->Face == 0 ||
                  memset(&blit, 0, sizeof(blit));
   blit.src.resource = src;
   blit.src.level = texImage->Level + texImage->TexObject->Attrib.MinLevel;
   blit.src.format = src_format;
   blit.dst.resource = dst;
   blit.dst.level = 0;
   blit.dst.format = dst->format;
   blit.src.box.x = xoffset;
   blit.dst.box.x = 0;
   blit.src.box.y = yoffset;
   blit.dst.box.y = 0;
   blit.src.box.z = texImage->Face + texImage->TexObject->Attrib.MinLayer + zoffset;
   blit.dst.box.z = 0;
   blit.src.box.width = blit.dst.box.width = width;
   blit.src.box.height = blit.dst.box.height = height;
   blit.src.box.depth = blit.dst.box.depth = depth;
   blit.mask = st_get_blit_mask(texImage->_BaseFormat, format);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
            /* blit/render/decompress */
            done = copy_to_staging_dest(ctx, dst, xoffset, yoffset, zoffset, width, height,
               non_blit_transfer:
      if (done)
         if (st->allow_compute_based_texture_transfer || st->force_compute_based_texture_transfer) {
      if (st_GetTexSubImage_shader(ctx, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels, texImage))
         cpu_transfer:
      _mesa_GetTexSubImage_sw(ctx, xoffset, yoffset, zoffset,
            }
         /**
   * Do a CopyTexSubImage operation using a read transfer from the source,
   * a write transfer to the destination and get_tile()/put_tile() to access
   * the pixels/texels.
   *
   * Note: srcY=0=TOP of renderbuffer
   */
   static void
   fallback_copy_texsubimage(struct gl_context *ctx,
                           struct gl_renderbuffer *rb,
   struct gl_texture_image *stImage,
   {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer *src_trans;
   GLubyte *texDest;
   enum pipe_map_flags transfer_usage;
   void *map;
   unsigned dst_width = width;
   unsigned dst_height = height;
   unsigned dst_depth = 1;
            if (ST_DEBUG & DEBUG_FALLBACK)
            if (_mesa_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
                  map = pipe_texture_map(pipe,
                           rb->texture,
   rb->surface->u.tex.level,
   if (!map) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage()");
               if ((baseFormat == GL_DEPTH_COMPONENT ||
      baseFormat == GL_DEPTH_STENCIL) &&
   util_format_is_depth_and_stencil(stImage->pt->format))
      else
            texDest = st_texture_image_map(st, stImage, transfer_usage,
                     if (!texDest) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glCopyTexSubImage()");
               if (baseFormat == GL_DEPTH_COMPONENT ||
      baseFormat == GL_DEPTH_STENCIL) {
   const GLboolean scaleOrBias = (ctx->Pixel.DepthScale != 1.0F ||
         GLint row, yStep;
            /* determine bottom-to-top vs. top-to-bottom order for src buffer */
   if (_mesa_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
      srcY = height - 1;
      }
   else {
      srcY = 0;
                        if (data) {
      unsigned dst_stride = (stImage->pt->target == PIPE_TEXTURE_1D_ARRAY ?
         /* To avoid a large temp memory allocation, do copy row by row */
   for (row = 0; row < height; row++, srcY += yStep) {
      util_format_unpack_z_32unorm(rb->texture->format,
               if (scaleOrBias) {
                  util_format_pack_z_32unorm(stImage->pt->format,
         }
   else {
                     }
   else {
      /* RGBA format */
   GLfloat *tempSrc =
            if (tempSrc) {
      const GLint dims = 2;
   GLint dstRowStride;
                  if (_mesa_fb_orientation(ctx->ReadBuffer) == Y_0_TOP) {
                  if (stImage->pt->target == PIPE_TEXTURE_1D_ARRAY) {
         }
   else {
                  /* get float/RGBA image from framebuffer */
   /* XXX this usually involves a lot of int/float conversion.
   * try to avoid that someday.
   */
   pipe_get_tile_rgba(src_trans, map, 0, 0, width, height,
                  /* Store into texture memory.
   * Note that this does some special things such as pixel transfer
   * ops and format conversion.  In particular, if the dest tex format
   * is actually RGBA but the user created the texture as GL_RGB we
   * need to fill-in/override the alpha channel with 1.0.
   */
   _mesa_texstore(ctx, dims,
                  texImage->_BaseFormat,
   texImage->TexFormat,
   dstRowStride,
   &texDest,
   }
   else {
                                 err:
         }
         static bool
   st_can_copyteximage_using_blit(const struct gl_texture_image *texImage,
         {
               /* We don't blit to a teximage where the GL base format doesn't match the
   * texture's chosen format, except in the case of a GL_RGB texture
   * represented with GL_RGBA (where the alpha channel is just being
   * dropped).
   */
   if (texImage->_BaseFormat != tex_baseformat &&
      ((texImage->_BaseFormat != GL_RGB || tex_baseformat != GL_RGBA))) {
               /* We can't blit from a RB where the GL base format doesn't match the RB's
   * chosen format (for example, GL RGB or ALPHA with rb->Format of an RGBA
   * type, because the other channels will be undefined).
   */
   if (rb->_BaseFormat != _mesa_get_format_base_format(rb->Format))
               }
         /**
   * Do a CopyTex[Sub]Image1/2/3D() using a hardware (blit) path if possible.
   * Note that the region to copy has already been clipped so we know we
   * won't read from outside the source renderbuffer's bounds.
   *
   * Note: srcY=0=Bottom of renderbuffer (GL convention)
   */
   void
   st_CopyTexSubImage(struct gl_context *ctx, GLuint dims,
                     struct gl_texture_image *texImage,
   {
      struct gl_texture_image *stImage = texImage;
   struct gl_texture_object *stObj = texImage->TexObject;
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = st->screen;
   struct pipe_blit_info blit;
   enum pipe_format dst_format;
   GLboolean do_flip = (_mesa_fb_orientation(ctx->ReadBuffer) == Y_0_TOP);
   unsigned bind;
            st_flush_bitmap_cache(st);
            assert(!_mesa_is_format_etc2(texImage->TexFormat) &&
                  if (!rb || !rb->surface || !stImage->pt) {
      debug_printf("%s: null rb or stImage\n", __func__);
               if (_mesa_texstore_needs_transfer_ops(ctx, texImage->_BaseFormat,
                        if (!st_can_copyteximage_using_blit(texImage, rb)) {
                  /* Choose the destination format to match the TexImage behavior. */
   dst_format = util_format_linear(stImage->pt->format);
   dst_format = util_format_luminance_to_red(dst_format);
            /* See if the destination format is supported. */
   if (texImage->_BaseFormat == GL_DEPTH_STENCIL ||
      texImage->_BaseFormat == GL_DEPTH_COMPONENT) {
      }
   else {
                  if (!dst_format ||
      !screen->is_format_supported(screen, dst_format, stImage->pt->target,
                           /* Y flipping for the main framebuffer. */
   if (do_flip) {
      srcY1 = rb->Height - srcY - height;
      }
   else {
      srcY0 = srcY;
               /* Blit the texture.
   * This supports flipping, format conversions, and downsampling.
   */
   memset(&blit, 0, sizeof(blit));
   blit.src.resource = rb->texture;
   blit.src.format = util_format_linear(rb->surface->format);
   blit.src.level = rb->surface->u.tex.level;
   blit.src.box.x = srcX;
   blit.src.box.y = srcY0;
   blit.src.box.z = rb->surface->u.tex.first_layer;
   blit.src.box.width = width;
   blit.src.box.height = srcY1 - srcY0;
   blit.src.box.depth = 1;
   blit.dst.resource = stImage->pt;
   blit.dst.format = dst_format;
   blit.dst.level = stObj->pt != stImage->pt
         blit.dst.box.x = destX;
   blit.dst.box.y = destY;
   blit.dst.box.z = stImage->Face + slice +
         blit.dst.box.width = width;
   blit.dst.box.height = height;
   blit.dst.box.depth = 1;
   blit.mask = st_get_blit_mask(rb->_BaseFormat, texImage->_BaseFormat);
   blit.filter = PIPE_TEX_FILTER_NEAREST;
   pipe->blit(pipe, &blit);
         fallback:
      /* software fallback */
   fallback_copy_texsubimage(ctx,
                  }
         /**
   * Copy image data from stImage into the texture object 'stObj' at level
   * 'dstLevel'.
   */
   static void
   copy_image_data_to_texture(struct st_context *st,
                     {
      /* debug checks */
   {
      ASSERTED const struct gl_texture_image *dstImage =
         assert(dstImage);
   assert(dstImage->Width == stImage->Width);
   assert(dstImage->Height == stImage->Height);
               if (stImage->pt) {
      /* Copy potentially with the blitter:
   */
   GLuint src_level;
   if (stImage->pt->last_level == 0)
         else
            assert(src_level <= stImage->pt->last_level);
   assert(u_minify(stImage->pt->width0, src_level) == stImage->Width);
   assert(stImage->pt->target == PIPE_TEXTURE_1D_ARRAY ||
         assert(stImage->pt->target == PIPE_TEXTURE_2D_ARRAY ||
                  st_texture_image_copy(st->pipe,
                           }
      }
         /**
   * Called during state validation.  When this function is finished,
   * the texture object should be ready for rendering.
   * \return GL_TRUE for success, GL_FALSE for failure (out of mem)
   */
   GLboolean
   st_finalize_texture(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   const GLuint nr_faces = _mesa_num_tex_faces(tObj->Target);
   GLuint face;
   const struct gl_texture_image *firstImage;
   enum pipe_format firstImageFormat;
   unsigned ptWidth;
            if (tObj->Immutable)
            if (tObj->_MipmapComplete)
         else if (tObj->_BaseComplete)
            /* Skip the loop over images in the common case of no images having
   * changed.  But if the GL_BASE_LEVEL or GL_MAX_LEVEL change to something we
   * haven't looked at, then we do need to look at those new images.
   */
   if (!tObj->needs_validation &&
      tObj->Attrib.BaseLevel >= tObj->validated_first_level &&
   tObj->lastLevel <= tObj->validated_last_level) {
               /* If this texture comes from a window system, there is nothing else to do. */
   if (tObj->surface_based) {
                  firstImage = st_texture_image_const(tObj->Image[cubeMapFace]
         if (!firstImage)
            /* If both firstImage and tObj point to a texture which can contain
   * all active images, favour firstImage.  Note that because of the
   * completeness requirement, we know that the image dimensions
   * will match.
   */
   if (firstImage->pt &&
      firstImage->pt != tObj->pt &&
   (!tObj->pt || firstImage->pt->last_level >= tObj->pt->last_level)) {
   pipe_resource_reference(&tObj->pt, firstImage->pt);
               /* Find gallium format for the Mesa texture */
   firstImageFormat =
            /* Find size of level=0 Gallium mipmap image, plus number of texture layers */
   {
      unsigned width;
            st_gl_texture_dims_to_pipe_dims(tObj->Target,
                              /* If we previously allocated a pipe texture and its sizes are
   * compatible, use them.
   */
   if (tObj->pt &&
      u_minify(tObj->pt->width0, firstImage->Level) == width &&
   u_minify(tObj->pt->height0, firstImage->Level) == height &&
   u_minify(tObj->pt->depth0, firstImage->Level) == depth) {
   ptWidth = tObj->pt->width0;
   ptHeight = tObj->pt->height0;
      } else {
      /* Otherwise, compute a new level=0 size that is compatible with the
   * base level image.
   */
   ptWidth = width > 1 ? width << firstImage->Level : 1;
                  /* If the base level image is 1x1x1, we still need to ensure that the
   * resulting pipe texture ends up with the required number of levels
   * in total.
   */
                     if (tObj->Target == GL_TEXTURE_CUBE_MAP ||
                     /* At this point, the texture may be incomplete (mismatched cube
   * face sizes, for example).  If that's the case, give up, but
   * don't return GL_FALSE as that would raise an incorrect
   * GL_OUT_OF_MEMORY error.  See Piglit fbo-incomplete-texture-03 test.
   */
   if (!tObj->_BaseComplete) {
      _mesa_test_texobj_completeness(ctx, tObj);
   if (!tObj->_BaseComplete) {
                                    /* If we already have a gallium texture, check that it matches the texture
   * object's format, target, size, num_levels, etc.
   */
   if (tObj->pt) {
      if (tObj->pt->target != gl_target_to_pipe(tObj->Target) ||
      tObj->pt->format != firstImageFormat ||
   tObj->pt->last_level < tObj->lastLevel ||
   tObj->pt->width0 != ptWidth ||
   tObj->pt->height0 != ptHeight ||
   tObj->pt->depth0 != ptDepth ||
   tObj->pt->nr_samples != ptNumSamples ||
      {
      /* The gallium texture does not match the Mesa texture so delete the
   * gallium texture now.  We'll make a new one below.
   */
   pipe_resource_reference(&tObj->pt, NULL);
   st_texture_release_all_sampler_views(st, tObj);
                  /* May need to create a new gallium texture:
   */
   if (!tObj->pt && !tObj->NullTexture) {
               tObj->pt = st_texture_create(st,
                                 gl_target_to_pipe(tObj->Target),
   firstImageFormat,
   tObj->lastLevel,
            if (!tObj->pt) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexImage");
                  /* Pull in any images not in the object's texture:
   */
   for (face = 0; face < nr_faces; face++) {
      GLuint level;
   for (level = tObj->Attrib.BaseLevel; level <= tObj->lastLevel; level++) {
                     /* Need to import images in main memory or held in other textures.
   */
   if (stImage && !tObj->NullTexture && tObj->pt != stImage->pt) {
                     if (tObj->Target != GL_TEXTURE_1D_ARRAY)
                        if (tObj->Target == GL_TEXTURE_3D)
         else if (tObj->Target == GL_TEXTURE_CUBE_MAP)
                        if (level == 0 ||
      (stImage->Width == u_minify(ptWidth, level) &&
   stImage->Height == height &&
   stImage->Depth == depth)) {
   /* src image fits expected dest mipmap level size */
                        tObj->validated_first_level = tObj->Attrib.BaseLevel;
   tObj->validated_last_level = tObj->lastLevel;
               }
         /**
   * Allocate a new pipe_resource object
   * width0, height0, depth0 are the dimensions of the level 0 image
   * (the highest resolution).  last_level indicates how many mipmap levels
   * to allocate storage for.  For non-mipmapped textures, this will be zero.
   */
   static struct pipe_resource *
   st_texture_create_from_memory(struct st_context *st,
                                 struct gl_memory_object *memObj,
   GLuint64 offset,
   enum pipe_texture_target target,
   enum pipe_format format,
   GLuint last_level,
   GLuint width0,
   {
      struct pipe_resource pt, *newtex;
            assert(target < PIPE_MAX_TEXTURE_TYPES);
   assert(width0 > 0);
   assert(height0 > 0);
   assert(depth0 > 0);
   if (target == PIPE_TEXTURE_CUBE)
            DBG("%s target %d format %s last_level %d\n", __func__,
            assert(format);
   assert(screen->is_format_supported(screen, format, target, 0, 0,
            memset(&pt, 0, sizeof(pt));
   pt.target = target;
   pt.format = format;
   pt.last_level = last_level;
   pt.width0 = width0;
   pt.height0 = height0;
   pt.depth0 = depth0;
   pt.array_size = layers;
   pt.usage = PIPE_USAGE_DEFAULT;
   pt.bind = bind;
   /* only set this for OpenGL textures, not renderbuffers */
   pt.flags = PIPE_RESOURCE_FLAG_TEXTURING_MORE_LIKELY;
   if (memObj->TextureTiling == GL_LINEAR_TILING_EXT) {
         } else if (memObj->TextureTiling == GL_CONST_BW_TILING_MESA) {
                  pt.nr_samples = nr_samples;
                                 }
         /**
   * Allocate texture memory for a whole mipmap stack.
   * Note: for multisample textures if the requested sample count is not
   * supported, we search for the next higher supported sample count.
   */
   static GLboolean
   st_texture_storage(struct gl_context *ctx,
                     struct gl_texture_object *texObj,
   GLsizei levels, GLsizei width,
   {
      const GLuint numFaces = _mesa_num_tex_faces(texObj->Target);
   struct gl_texture_image *texImage = texObj->Image[0][0];
   struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
   unsigned ptWidth, bindings;
   uint16_t ptHeight, ptDepth, ptLayers;
   enum pipe_format fmt;
   GLint level;
                                                if (memObj) {
      memObj->TextureTiling = texObj->TextureTiling;
               if (num_samples > 0) {
      /* Find msaa sample count which is actually supported.  For example,
   * if the user requests 1x but only 4x or 8x msaa is supported, we'll
   * choose 4x here.
   */
   enum pipe_texture_target ptarget = gl_target_to_pipe(texObj->Target);
            if (ctx->Const.MaxSamples > 1 && num_samples == 1) {
      /* don't try num_samples = 1 with drivers that support real msaa */
               for (; num_samples <= ctx->Const.MaxSamples; num_samples++) {
      if (screen->is_format_supported(screen, fmt, ptarget,
                  /* Update the sample count in gl_texture_image as well. */
   texImage->NumSamples = num_samples;
   found = true;
                  if (!found) {
      _mesa_error(st->ctx, GL_INVALID_OPERATION, "%s(format/samplecount not supported)", func);
                  st_gl_texture_dims_to_pipe_dims(texObj->Target,
                           if (memObj) {
      texObj->pt = st_texture_create_from_memory(st,
                                             memObj,
   offset,
      }
   else {
      texObj->pt = st_texture_create(st,
                                 gl_target_to_pipe(texObj->Target),
   fmt,
   levels - 1,
               if (!texObj->pt) {
      _mesa_error(st->ctx, GL_OUT_OF_MEMORY, "%s", func);
               /* Set image resource pointers */
   for (level = 0; level < levels; level++) {
      GLuint face;
   for (face = 0; face < numFaces; face++) {
      struct gl_texture_image *stImage =
                                 /* Update gl_texture_object for texture parameter query. */
            /* The texture is in a validated state, so no need to check later. */
   texObj->needs_validation = false;
   texObj->validated_first_level = 0;
               }
      /**
   * Called via ctx->Driver.AllocTextureStorage() to allocate texture memory
   * for a whole mipmap stack.
   */
   GLboolean
   st_AllocTextureStorage(struct gl_context *ctx,
                           {
      return st_texture_storage(ctx, texObj, levels,
            }
         GLboolean
   st_TestProxyTexImage(struct gl_context *ctx, GLenum target,
                     {
               if (width == 0 || height == 0 || depth == 0) {
      /* zero-sized images are legal, and always fit! */
               if (st->screen->can_create_resource) {
      /* Ask the gallium driver if the texture is too large */
   struct gl_texture_object *texObj =
                  /* Setup the pipe_resource object
   */
            pt.target = gl_target_to_pipe(target);
   pt.format = st_mesa_format_to_pipe_format(st, format);
   pt.nr_samples = numSamples;
            st_gl_texture_dims_to_pipe_dims(target,
                        if (numLevels > 0) {
      /* For immutable textures we know the final number of mip levels */
      }
   else if (level == 0 && (texObj->Sampler.Attrib.MinFilter == GL_LINEAR ||
            /* assume just one mipmap level */
      }
   else {
      /* assume a full set of mipmaps */
                  }
   else {
      /* Use core Mesa fallback */
   return _mesa_test_proxy_teximage(ctx, target, numLevels, level, format,
         }
      GLboolean
   st_TextureView(struct gl_context *ctx,
               {
      struct st_context *st = st_context(ctx);
   struct gl_texture_object *orig = origTexObj;
   struct gl_texture_object *tex = texObj;
            const int numFaces = _mesa_num_tex_faces(texObj->Target);
            int face;
                     /* Set image resource pointers */
   for (level = 0; level < numLevels; level++) {
      for (face = 0; face < numFaces; face++) {
      struct gl_texture_image *stImage =
         struct gl_texture_image *origImage =
         pipe_resource_reference(&stImage->pt, tex->pt);
   if (origImage &&
      origImage->compressed_data) {
   pipe_reference(NULL,
                           tex->surface_based = GL_TRUE;
   tex->surface_format =
                     /* free texture sampler views.  They need to be recreated when we
   * change the texture view parameters.
   */
            /* The texture is in a validated state, so no need to check later. */
   tex->needs_validation = false;
   tex->validated_first_level = 0;
               }
         /**
   * Find the mipmap level in 'pt' which matches the level described by
   * 'texImage'.
   */
   static unsigned
   find_mipmap_level(const struct gl_texture_image *texImage,
         {
      const GLenum target = texImage->TexObject->Target;
   GLint texWidth = texImage->Width;
   GLint texHeight = texImage->Height;
   GLint texDepth = texImage->Depth;
   unsigned level, w;
            st_gl_texture_dims_to_pipe_dims(target, texWidth, texHeight, texDepth,
            for (level = 0; level <= pt->last_level; level++) {
      if (u_minify(pt->width0, level) == w &&
      u_minify(pt->height0, level) == h &&
   u_minify(pt->depth0, level) == d) {
                  /* If we get here, there must be some sort of inconsistency between
   * the Mesa texture object/images and the gallium resource.
   */
               }
         void
   st_ClearTexSubImage(struct gl_context *ctx,
                     struct gl_texture_image *texImage,
   {
      static const char zeros[16] = {0};
   struct gl_texture_object *texObj = texImage->TexObject;
   struct gl_texture_image *stImage = texImage;
   struct pipe_resource *pt = stImage->pt;
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   unsigned level;
            if (!pt)
            st_flush_bitmap_cache(st);
            u_box_3d(xoffset, yoffset, zoffset + texImage->Face,
            if (pt->target == PIPE_TEXTURE_1D_ARRAY) {
      box.z = box.y;
   box.depth = box.height;
   box.y = 0;
               if (texObj->Immutable) {
      /* The texture object has to be consistent (no "loose", per-image
   * gallium resources).  If this texture is a view into another
   * texture, we have to apply the MinLevel/Layer offsets.  If this is
   * not a texture view, the offsets will be zero.
   */
   assert(stImage->pt == texObj->pt);
   level = texImage->Level + texObj->Attrib.MinLevel;
      }
   else {
      /* Texture level sizes may be inconsistent.  We my have "loose",
   * per-image gallium resources.  The texImage->Level may not match
   * the gallium resource texture level.
   */
                        if (pipe->clear_texture) {
      pipe->clear_texture(pipe, pt, level, &box,
      } else {
      u_default_clear_texture(pipe, pt, level, &box,
         }
         GLboolean
   st_SetTextureStorageForMemoryObject(struct gl_context *ctx,
                                 {
      return st_texture_storage(ctx, texObj, levels,
            }
      GLboolean
   st_GetSparseTextureVirtualPageSize(struct gl_context *ctx,
               {
      struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->screen;
   enum pipe_texture_target ptarget = gl_target_to_pipe(target);
   enum pipe_format pformat = st_mesa_format_to_pipe_format(st, format);
            /* Get an XYZ page size combination specified by index. */
   return !!screen->get_sparse_texture_virtual_page_size(
      }
      void
   st_TexturePageCommitment(struct gl_context *ctx,
                     {
      struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
                     if (!pipe->resource_commit(pipe, tex_obj->pt, level, &box, commit)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glTexPageCommitmentARB(out of memory)");
         }
