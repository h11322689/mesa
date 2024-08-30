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
      #include "st_context.h"
   #include "st_format.h"
   #include "st_texture.h"
   #include "main/enums.h"
      #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_rect.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "tgsi/tgsi_from_mesa.h"
         #define DBG if(0) printf
         /**
   * Allocate a new pipe_resource object
   * width0, height0, depth0 are the dimensions of the level 0 image
   * (the highest resolution).  last_level indicates how many mipmap levels
   * to allocate storage for.  For non-mipmapped textures, this will be zero.
   */
   struct pipe_resource *
   st_texture_create(struct st_context *st,
                     enum pipe_texture_target target,
   enum pipe_format format,
   GLuint last_level,
   GLuint width0,
   GLuint height0,
   GLuint depth0,
   GLuint layers,
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
   pt.nr_samples = nr_samples;
            if (sparse)
                                 }
         /**
   * In OpenGL the number of 1D array texture layers is the "height" and
   * the number of 2D array texture layers is the "depth".  In Gallium the
   * number of layers in an array texture is a separate 'array_size' field.
   * This function converts dimensions from the former to the later.
   */
   void
   st_gl_texture_dims_to_pipe_dims(GLenum texture,
                                 unsigned widthIn,
   uint16_t heightIn,
   {
      switch (texture) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      assert(heightIn == 1);
   assert(depthIn == 1);
   *widthOut = widthIn;
   *heightOut = 1;
   *depthOut = 1;
   *layersOut = 1;
      case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
      assert(depthIn == 1);
   *widthOut = widthIn;
   *heightOut = 1;
   *depthOut = 1;
   *layersOut = heightIn;
      case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE:
      assert(depthIn == 1);
   *widthOut = widthIn;
   *heightOut = heightIn;
   *depthOut = 1;
   *layersOut = 1;
      case GL_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      assert(depthIn == 1);
   *widthOut = widthIn;
   *heightOut = heightIn;
   *depthOut = 1;
   *layersOut = 6;
      case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      *widthOut = widthIn;
   *heightOut = heightIn;
   *depthOut = 1;
   *layersOut = depthIn;
      case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      *widthOut = widthIn;
   *heightOut = heightIn;
   *depthOut = 1;
   *layersOut = util_align_npot(depthIn, 6);
      default:
         case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      *widthOut = widthIn;
   *heightOut = heightIn;
   *depthOut = depthIn;
   *layersOut = 1;
         }
         /**
   * Check if a texture image can be pulled into a unified mipmap texture.
   */
   GLboolean
   st_texture_match_image(struct st_context *st,
               {
      unsigned ptWidth;
            /* Images with borders are never pulled into mipmap textures.
   */
   if (image->Border)
            /* Check if this image's format matches the established texture's format.
   */
   if (st_mesa_format_to_pipe_format(st, image->TexFormat) != pt->format)
            st_gl_texture_dims_to_pipe_dims(image->TexObject->Target,
                  /* Test if this image's size matches what's expected in the
   * established texture.
   */
   if (ptWidth != u_minify(pt->width0, image->Level) ||
      ptHeight != u_minify(pt->height0, image->Level) ||
   ptDepth != u_minify(pt->depth0, image->Level) ||
   ptLayers != pt->array_size)
         if (image->Level > pt->last_level)
               }
      void
   st_texture_image_insert_transfer(struct gl_texture_image *stImage,
               {
      /* Enlarge the transfer array if it's not large enough. */
   if (index >= stImage->num_transfers) {
               stImage->transfer = realloc(stImage->transfer,
         memset(&stImage->transfer[stImage->num_transfers], 0,
         (new_size - stImage->num_transfers) *
               assert(!stImage->transfer[index].transfer);
      }
      /* See st_texture.h for more information. */
   GLuint
   st_texture_image_resource_level(struct gl_texture_image *stImage)
   {
      /* An image for a non-finalized texture object only has a single level. */
   if (stImage->pt != stImage->TexObject->pt)
            /* An immutable texture object may have views with an LOD offset. */
   if (stImage->TexObject->Immutable)
               }
      /**
   * Map a texture image and return the address for a particular 2D face/slice/
   * layer.  The stImage indicates the cube face and mipmap level.  The slice
   * of the 3D texture is passed in 'zoffset'.
   * \param usage  one of the PIPE_MAP_x values
   * \param x, y, w, h  the region of interest of the 2D image.
   * \return address of mapping or NULL if any error
   */
   GLubyte *
   st_texture_image_map(struct st_context *st, struct gl_texture_image *stImage,
                           {
      struct gl_texture_object *stObj = stImage->TexObject;
   GLuint level;
                     if (!stImage->pt)
            if (stObj->pt != stImage->pt)
         else
            if (stObj->Immutable) {
      level += stObj->Attrib.MinLevel;
   z += stObj->Attrib.MinLayer;
   if (stObj->pt->array_size > 1)
                        map = pipe_texture_map_3d(st->pipe, stImage->pt, level, usage,
            if (map)
               }
         void
   st_texture_image_unmap(struct st_context *st,
         {
      struct pipe_context *pipe = st->pipe;
   struct gl_texture_object *stObj = stImage->TexObject;
            if (stObj->Immutable)
                           pipe_texture_unmap(pipe, *transfer);
      }
         /**
   * For debug only: get/print center pixel in the src resource.
   */
   static void
   print_center_pixel(struct pipe_context *pipe, struct pipe_resource *src)
   {
      struct pipe_transfer *xfer;
   struct pipe_box region;
            region.x = src->width0 / 2;
   region.y = src->height0 / 2;
   region.z = 0;
   region.width = 1;
   region.height = 1;
                                 }
         /**
   * Copy the image at level=0 in 'src' to the 'dst' resource at 'dstLevel'.
   * This is used to copy mipmap images from one texture buffer to another.
   * This typically happens when our initial guess at the total texture size
   * is incorrect (see the guess_and_alloc_texture() function).
   */
   void
   st_texture_image_copy(struct pipe_context *pipe,
                     {
      GLuint width = u_minify(dst->width0, dstLevel);
   GLuint height = u_minify(dst->height0, dstLevel);
   GLuint depth = u_minify(dst->depth0, dstLevel);
   struct pipe_box src_box;
            if (u_minify(src->width0, srcLevel) != width ||
      u_minify(src->height0, srcLevel) != height ||
   u_minify(src->depth0, srcLevel) != depth) {
   /* The source image size doesn't match the destination image size.
   * This can happen in some degenerate situations such as rendering to a
   * cube map face which was set up with mismatched texture sizes.
   */
               src_box.x = 0;
   src_box.y = 0;
   src_box.width = width;
   src_box.height = height;
            if (src->target == PIPE_TEXTURE_1D_ARRAY ||
      src->target == PIPE_TEXTURE_2D_ARRAY ||
   src->target == PIPE_TEXTURE_CUBE_ARRAY) {
   face = 0;
               /* Loop over 3D image slices */
   /* could (and probably should) use "true" 3d box here -
         for (i = face; i < face + depth; i++) {
               if (0)  {
                  pipe->resource_copy_region(pipe,
                              dst,
      }
         struct pipe_resource *
   st_create_color_map_texture(struct gl_context *ctx)
   {
      struct st_context *st = st_context(ctx);
   struct pipe_resource *pt;
   enum pipe_format format;
            /* find an RGBA texture format */
   format = st_choose_format(st, GL_RGBA, GL_NONE, GL_NONE,
                  /* create texture for color map/table */
   pt = st_texture_create(st, PIPE_TEXTURE_2D, format, 0,
            }
         /**
   * Destroy bound texture handles for the given stage.
   */
   static void
   st_destroy_bound_texture_handles_per_stage(struct st_context *st,
         {
      struct st_bound_handles *bound_handles = &st->bound_texture_handles[shader];
   struct pipe_context *pipe = st->pipe;
            if (likely(!bound_handles->num_handles))
            for (i = 0; i < bound_handles->num_handles; i++) {
               pipe->make_texture_handle_resident(pipe, handle, false);
      }
   free(bound_handles->handles);
   bound_handles->handles = NULL;
      }
         /**
   * Destroy all bound texture handles in the context.
   */
   void
   st_destroy_bound_texture_handles(struct st_context *st)
   {
               for (i = 0; i < PIPE_SHADER_TYPES; i++) {
            }
         /**
   * Destroy bound image handles for the given stage.
   */
   static void
   st_destroy_bound_image_handles_per_stage(struct st_context *st,
         {
      struct st_bound_handles *bound_handles = &st->bound_image_handles[shader];
   struct pipe_context *pipe = st->pipe;
            if (likely(!bound_handles->num_handles))
            for (i = 0; i < bound_handles->num_handles; i++) {
               pipe->make_image_handle_resident(pipe, handle, GL_READ_WRITE, false);
      }
   free(bound_handles->handles);
   bound_handles->handles = NULL;
      }
         /**
   * Destroy all bound image handles in the context.
   */
   void
   st_destroy_bound_image_handles(struct st_context *st)
   {
               for (i = 0; i < PIPE_SHADER_TYPES; i++) {
            }
         /**
   * Create a texture handle from a texture unit.
   */
   static GLuint64
   st_create_texture_handle_from_unit(struct st_context *st,
         {
      struct pipe_context *pipe = st->pipe;
   struct pipe_sampler_view *view;
   struct pipe_sampler_state sampler = {0};
   const bool glsl130 =
            /* TODO: Clarify the interaction of ARB_bindless_texture and EXT_texture_sRGB_decode */
   view = st_update_single_texture(st, texUnit, glsl130, true, false);
   if (!view)
            if (view->target != PIPE_BUFFER)
                        }
         /**
   * Create an image handle from an image unit.
   */
   static GLuint64
   st_create_image_handle_from_unit(struct st_context *st,
         {
      struct pipe_context *pipe = st->pipe;
                        }
         /**
   * Make all bindless samplers bound to texture units resident in the context.
   */
   void
   st_make_bound_samplers_resident(struct st_context *st,
         {
      enum pipe_shader_type shader = pipe_shader_type_from_mesa(prog->info.stage);
   struct st_bound_handles *bound_handles = &st->bound_texture_handles[shader];
   struct pipe_context *pipe = st->pipe;
   GLuint64 handle;
            /* Remove previous bound texture handles for this stage. */
            if (likely(!prog->sh.HasBoundBindlessSampler))
            for (i = 0; i < prog->sh.NumBindlessSamplers; i++) {
               if (!sampler->bound)
            /* Request a new texture handle from the driver and make it resident. */
   handle = st_create_texture_handle_from_unit(st, prog, sampler->unit);
   if (!handle)
                     /* Overwrite the texture unit value by the resident handle before
   * uploading the constant buffer.
   */
            /* Store the handle in the context. */
   bound_handles->handles = (uint64_t *)
      realloc(bound_handles->handles,
      bound_handles->handles[bound_handles->num_handles] = handle;
         }
         /**
   * Make all bindless images bound to image units resident in the context.
   */
   void
   st_make_bound_images_resident(struct st_context *st,
         {
      enum pipe_shader_type shader = pipe_shader_type_from_mesa(prog->info.stage);
   struct st_bound_handles *bound_handles = &st->bound_image_handles[shader];
   struct pipe_context *pipe = st->pipe;
   GLuint64 handle;
            /* Remove previous bound image handles for this stage. */
            if (likely(!prog->sh.HasBoundBindlessImage))
            for (i = 0; i < prog->sh.NumBindlessImages; i++) {
               if (!image->bound)
            /* Request a new image handle from the driver and make it resident. */
   handle = st_create_image_handle_from_unit(st, prog, image->unit);
   if (!handle)
                     /* Overwrite the image unit value by the resident handle before uploading
   * the constant buffer.
   */
            /* Store the handle in the context. */
   bound_handles->handles = (uint64_t *)
      realloc(bound_handles->handles,
      bound_handles->handles[bound_handles->num_handles] = handle;
         }
