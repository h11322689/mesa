   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2009, VMware, Inc.
   * All Rights Reserved.
   * Copyright (C) 2010 LunarG Inc.
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
   *
   * Authors:
   *    Keith Whitwell <keithw@vmware.com> Jakob Bornecrantz
   *    <wallbraker@gmail.com> Chia-I Wu <olv@lunarg.com>
   */
      #include <xf86drm.h>
   #include "git_sha1.h"
   #include "GL/mesa_glinterop.h"
   #include "GL/internal/mesa_interface.h"
   #include "util/disk_cache.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "util/u_debug.h"
   #include "util/libsync.h"
   #include "util/os_file.h"
   #include "frontend/drm_driver.h"
   #include "state_tracker/st_format.h"
   #include "state_tracker/st_cb_texture.h"
   #include "state_tracker/st_texture.h"
   #include "state_tracker/st_context.h"
   #include "state_tracker/st_interop.h"
   #include "pipe-loader/pipe_loader.h"
   #include "main/bufferobj.h"
   #include "main/texobj.h"
      #include "dri_util.h"
      #include "dri_helpers.h"
   #include "dri_drawable.h"
   #include "dri_query_renderer.h"
      #include "drm-uapi/drm_fourcc.h"
      struct dri2_buffer
   {
      __DRIbuffer base;
      };
      static inline struct dri2_buffer *
   dri2_buffer(__DRIbuffer * driBufferPriv)
   {
         }
      /**
   * Invalidate the drawable.
   *
   * How we get here is listed below.
   *
   * 1. Called by these SwapBuffers implementations where the context is known:
   *       loader_dri3_swap_buffers_msc
   *       EGL: droid_swap_buffers
   *       EGL: dri2_drm_swap_buffers
   *       EGL: dri2_wl_swap_buffers_with_damage
   *       EGL: dri2_x11_swap_buffers_msc
   *
   * 2. Other callers where the context is known:
   *       st_manager_flush_frontbuffer -> dri2_flush_frontbuffer
   *          -> EGL droid_display_shared_buffer
   *
   * 3. Other callers where the context is unknown:
   *       loader: dri3_handle_present_event - XCB_PRESENT_CONFIGURE_NOTIFY
   *       eglQuerySurface -> dri3_query_surface
   *          -> loader_dri3_update_drawable_geometry
   *       EGL: wl_egl_window::resize_callback (called outside Mesa)
   */
   static void
   dri2_invalidate_drawable(__DRIdrawable *dPriv)
   {
               drawable->lastStamp++;
               }
      static const __DRI2flushExtension dri2FlushExtension = {
               .flush                = dri_flush_drawable,
   .invalidate           = dri2_invalidate_drawable,
      };
      /**
   * Retrieve __DRIbuffer from the DRI loader.
   */
   static __DRIbuffer *
   dri2_drawable_get_buffers(struct dri_drawable *drawable,
               {
      const __DRIdri2LoaderExtension *loader = drawable->screen->dri2.loader;
   bool with_format;
   __DRIbuffer *buffers;
   int num_buffers;
   unsigned attachments[__DRI_BUFFER_COUNT];
            assert(loader);
   assert(*count <= __DRI_BUFFER_COUNT);
                     /* for Xserver 1.6.0 (DRI2 version 1) we always need to ask for the front */
   if (!with_format)
            for (i = 0; i < *count; i++) {
      enum pipe_format format;
   unsigned bind;
            dri_drawable_get_format(drawable, atts[i], &format, &bind);
   if (format == PIPE_FORMAT_NONE)
            switch (atts[i]) {
   case ST_ATTACHMENT_FRONT_LEFT:
      /* already added */
   if (!with_format)
         att = __DRI_BUFFER_FRONT_LEFT;
      case ST_ATTACHMENT_BACK_LEFT:
      att = __DRI_BUFFER_BACK_LEFT;
      case ST_ATTACHMENT_FRONT_RIGHT:
      att = __DRI_BUFFER_FRONT_RIGHT;
      case ST_ATTACHMENT_BACK_RIGHT:
      att = __DRI_BUFFER_BACK_RIGHT;
      default:
                  /*
   * In this switch statement we must support all formats that
   * may occur as the stvis->color_format.
   */
   switch(format) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      depth = 64;
      case PIPE_FORMAT_R16G16B16X16_FLOAT:
      depth = 48;
      case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_BGRA8888_UNORM:
   depth = 32;
   break;
         case PIPE_FORMAT_R10G10B10X2_UNORM:
   case PIPE_FORMAT_B10G10R10X2_UNORM:
      depth = 30;
      case PIPE_FORMAT_BGRX8888_UNORM:
   depth = 24;
   break;
         depth = 16;
   break;
         depth = util_format_get_blocksizebits(format);
   assert(!"Unexpected format in dri2_drawable_get_buffers()");
                  attachments[num_attachments++] = att;
   if (with_format) {
                     if (with_format) {
      num_attachments /= 2;
   buffers = loader->getBuffersWithFormat(opaque_dri_drawable(drawable),
         &drawable->w, &drawable->h,
      }
   else {
      buffers = loader->getBuffers(opaque_dri_drawable(drawable),
         &drawable->w, &drawable->h,
               if (buffers)
               }
      bool
   dri_image_drawable_get_buffers(struct dri_drawable *drawable,
                     bool
   dri_image_drawable_get_buffers(struct dri_drawable *drawable,
                     {
      unsigned int image_format = __DRI_IMAGE_FORMAT_NONE;
   enum pipe_format pf;
   uint32_t buffer_mask = 0;
            for (i = 0; i < statts_count; i++) {
      dri_drawable_get_format(drawable, statts[i], &pf, &bind);
   if (pf == PIPE_FORMAT_NONE)
            switch (statts[i]) {
   case ST_ATTACHMENT_FRONT_LEFT:
      buffer_mask |= __DRI_IMAGE_BUFFER_FRONT;
      case ST_ATTACHMENT_BACK_LEFT:
      buffer_mask |= __DRI_IMAGE_BUFFER_BACK;
      default:
                  switch (pf) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      image_format = __DRI_IMAGE_FORMAT_ABGR16161616F;
      case PIPE_FORMAT_R16G16B16X16_FLOAT:
      image_format = __DRI_IMAGE_FORMAT_XBGR16161616F;
      case PIPE_FORMAT_B5G5R5A1_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB1555;
      case PIPE_FORMAT_B5G6R5_UNORM:
      image_format = __DRI_IMAGE_FORMAT_RGB565;
      case PIPE_FORMAT_BGRX8888_UNORM:
      image_format = __DRI_IMAGE_FORMAT_XRGB8888;
      case PIPE_FORMAT_BGRA8888_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB8888;
      case PIPE_FORMAT_RGBX8888_UNORM:
      image_format = __DRI_IMAGE_FORMAT_XBGR8888;
      case PIPE_FORMAT_RGBA8888_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR8888;
      case PIPE_FORMAT_B10G10R10X2_UNORM:
      image_format = __DRI_IMAGE_FORMAT_XRGB2101010;
      case PIPE_FORMAT_B10G10R10A2_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB2101010;
      case PIPE_FORMAT_R10G10B10X2_UNORM:
      image_format = __DRI_IMAGE_FORMAT_XBGR2101010;
      case PIPE_FORMAT_R10G10B10A2_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR2101010;
      case PIPE_FORMAT_R5G5B5A1_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR1555;
      case PIPE_FORMAT_B4G4R4A4_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB4444;
      case PIPE_FORMAT_R4G4B4A4_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR4444;
      default:
      image_format = __DRI_IMAGE_FORMAT_NONE;
                  /* Stamp usage behavior in the getBuffers callback:
   *
   * 1. DRI3 (EGL and GLX):
   *       This calls loader_dri3_get_buffers, which saves the stamp pointer
   *       in loader_dri3_drawable::stamp, which is only changed (incremented)
   *       by loader_dri3_swap_buffers_msc.
   *
   * 2. EGL Android, Device, Surfaceless, Wayland:
   *       The stamp is unused.
   *
   * How do we get here:
   *    dri_set_tex_buffer2 (GLX_EXT_texture_from_pixmap)
   *    st_api_make_current
   *    st_manager_validate_framebuffers (part of st_validate_state)
   */
   return drawable->screen->image.loader->getBuffers(
                              }
      static __DRIbuffer *
   dri2_allocate_buffer(struct dri_screen *screen,
               {
      struct dri2_buffer *buffer;
   struct pipe_resource templ;
   enum pipe_format pf;
   unsigned bind = 0;
            /* struct pipe_resource height0 is 16-bit, avoid overflow */
   if (height > 0xffff)
            switch (attachment) {
      case __DRI_BUFFER_FRONT_LEFT:
   case __DRI_BUFFER_FAKE_FRONT_LEFT:
      bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      case __DRI_BUFFER_BACK_LEFT:
      bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
      case __DRI_BUFFER_DEPTH:
   case __DRI_BUFFER_DEPTH_STENCIL:
   case __DRI_BUFFER_STENCIL:
                     /* because we get the handle and stride */
            switch (format) {
      case 64:
      pf = PIPE_FORMAT_R16G16B16A16_FLOAT;
      case 48:
      pf = PIPE_FORMAT_R16G16B16X16_FLOAT;
      case 32:
      pf = PIPE_FORMAT_BGRA8888_UNORM;
      case 30:
      pf = PIPE_FORMAT_B10G10R10X2_UNORM;
      case 24:
      pf = PIPE_FORMAT_BGRX8888_UNORM;
      case 16:
      pf = PIPE_FORMAT_Z16_UNORM;
      default:
               buffer = CALLOC_STRUCT(dri2_buffer);
   if (!buffer)
            memset(&templ, 0, sizeof(templ));
   templ.bind = bind;
   templ.format = pf;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
            buffer->resource =
         if (!buffer->resource) {
      FREE(buffer);
               memset(&whandle, 0, sizeof(whandle));
   if (screen->can_share_buffer)
         else
            screen->base.screen->resource_get_handle(screen->base.screen, NULL,
                  buffer->base.attachment = attachment;
   buffer->base.name = whandle.handle;
   buffer->base.cpp = util_format_get_blocksize(pf);
               }
      static void
   dri2_release_buffer(__DRIbuffer *bPriv)
   {
               pipe_resource_reference(&buffer->resource, NULL);
      }
      static void
   dri2_set_in_fence_fd(__DRIimage *img, int fd)
   {
      validate_fence_fd(fd);
   validate_fence_fd(img->in_fence_fd);
      }
      static void
   handle_in_fence(struct dri_context *ctx, __DRIimage *img)
   {
      struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_fence_handle *fence;
            if (fd == -1)
                              pipe->create_fence_fd(pipe, &fence, fd, PIPE_FD_TYPE_NATIVE_SYNC);
   pipe->fence_server_sync(pipe, fence);
               }
      /*
   * Backend functions for pipe_frontend_drawable.
   */
      static void
   dri2_allocate_textures(struct dri_context *ctx,
                     {
      struct dri_screen *screen = drawable->screen;
   struct pipe_resource templ;
   bool alloc_depthstencil = false;
   unsigned i, j, bind;
   const __DRIimageLoaderExtension *image = screen->image.loader;
   /* Image specific variables */
   struct __DRIimageList images;
   /* Dri2 specific variables */
   __DRIbuffer *buffers = NULL;
   struct winsys_handle whandle;
                     /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            /* First get the buffers from the loader */
   if (image) {
      if (!dri_image_drawable_get_buffers(drawable, &images,
            }
   else {
      buffers = dri2_drawable_get_buffers(drawable, statts, &num_buffers);
   if (!buffers || (drawable->old_num == num_buffers &&
                  drawable->old_w == drawable->w &&
   drawable->old_h == drawable->h &&
                     /* See if we need a depth-stencil buffer. */
   for (i = 0; i < statts_count; i++) {
      if (statts[i] == ST_ATTACHMENT_DEPTH_STENCIL) {
      alloc_depthstencil = true;
                  /* Delete the resources we won't need. */
   for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      /* Don't delete the depth-stencil buffer, we can reuse it. */
   if (i == ST_ATTACHMENT_DEPTH_STENCIL && alloc_depthstencil)
            /* Flush the texture before unreferencing, so that other clients can
   * see what the driver has rendered.
   */
   if (i != ST_ATTACHMENT_DEPTH_STENCIL && drawable->textures[i]) {
      struct pipe_context *pipe = ctx->st->pipe;
                           if (drawable->stvis.samples > 1) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
               /* Don't delete MSAA resources for the attachments which are enabled,
   * we can reuse them. */
   for (j = 0; j < statts_count; j++) {
      if (i == statts[j]) {
      del = false;
                  if (del) {
                                 memset(&templ, 0, sizeof(templ));
   templ.target = screen->target;
   templ.last_level = 0;
   templ.depth0 = 1;
            if (image) {
      if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      struct pipe_resource **buf =
                                 pipe_resource_reference(buf, texture);
               if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      struct pipe_resource **buf =
                                 pipe_resource_reference(buf, texture);
               if (images.image_mask & __DRI_IMAGE_BUFFER_SHARED) {
      struct pipe_resource **buf =
                                                   } else {
                  /* Note: if there is both a back and a front buffer,
   * then they have the same size.
   */
   templ.width0 = drawable->w;
      }
   else {
               /* Process DRI-provided buffers and get pipe_resources. */
   for (i = 0; i < num_buffers; i++) {
      __DRIbuffer *buf = &buffers[i];
                  switch (buf->attachment) {
   case __DRI_BUFFER_FRONT_LEFT:
      if (!screen->auto_fake_front) {
         }
      case __DRI_BUFFER_FAKE_FRONT_LEFT:
      statt = ST_ATTACHMENT_FRONT_LEFT;
      case __DRI_BUFFER_BACK_LEFT:
      statt = ST_ATTACHMENT_BACK_LEFT;
      default:
                  dri_drawable_get_format(drawable, statt, &format, &bind);
                  /* dri2_drawable_get_buffers has already filled dri_drawable->w
   * and dri_drawable->h */
   templ.width0 = drawable->w;
   templ.height0 = drawable->h;
   templ.format = format;
   templ.bind = bind;
   whandle.handle = buf->name;
   whandle.stride = buf->pitch;
   whandle.offset = 0;
   whandle.format = format;
   whandle.modifier = DRM_FORMAT_MOD_INVALID;
   if (screen->can_share_buffer)
         else
         drawable->textures[statt] =
      screen->base.screen->resource_from_handle(screen->base.screen,
                           /* Allocate private MSAA colorbuffers. */
   if (drawable->stvis.samples > 1) {
      for (i = 0; i < statts_count; i++) {
                              if (drawable->textures[statt]) {
      templ.format = drawable->textures[statt]->format;
   templ.bind = drawable->textures[statt]->bind &
                        /* Try to reuse the resource.
   * (the other resource parameters should be constant)
   */
   if (!drawable->msaa_textures[statt] ||
      drawable->msaa_textures[statt]->width0 != templ.width0 ||
                        drawable->msaa_textures[statt] =
                        /* If there are any MSAA resources, we should initialize them
   * such that they contain the same data as the single-sample
   * resources we just got from the X server.
   *
   * The reason for this is that the gallium frontend (and
   * therefore the app) can access the MSAA resources only.
   * The single-sample resources are not exposed
   * to the gallium frontend.
   *
   */
   dri_pipe_blit(ctx->st->pipe,
               }
   else {
                        /* Allocate a private depth-stencil buffer. */
   if (alloc_depthstencil) {
      enum st_attachment_type statt = ST_ATTACHMENT_DEPTH_STENCIL;
   struct pipe_resource **zsbuf;
   enum pipe_format format;
                     if (format) {
                     if (drawable->stvis.samples > 1) {
      templ.nr_samples = drawable->stvis.samples;
   templ.nr_storage_samples = drawable->stvis.samples;
      }
   else {
      templ.nr_samples = 0;
   templ.nr_storage_samples = 0;
               /* Try to reuse the resource.
   * (the other resource parameters should be constant)
   */
   if (!*zsbuf ||
      (*zsbuf)->width0 != templ.width0 ||
   (*zsbuf)->height0 != templ.height0) {
   /* Allocate a new one. */
   pipe_resource_reference(zsbuf, NULL);
   *zsbuf = screen->base.screen->resource_create(screen->base.screen,
               }
   else {
      pipe_resource_reference(&drawable->msaa_textures[statt], NULL);
                  /* For DRI2, we may get the same buffers again from the server.
   * To prevent useless imports of gem names, drawable->old* is used
   * to bypass the import if we get the same buffers. This doesn't apply
   * to DRI3/Wayland, users of image.loader, since the buffer is managed
   * by the client (no import), and the back buffer is going to change
   * at every redraw.
   */
   if (!image) {
      drawable->old_num = num_buffers;
   drawable->old_w = drawable->w;
   drawable->old_h = drawable->h;
         }
      static bool
   dri2_flush_frontbuffer(struct dri_context *ctx,
               {
      const __DRIimageLoaderExtension *image = drawable->screen->image.loader;
   const __DRIdri2LoaderExtension *loader = drawable->screen->dri2.loader;
   const __DRImutableRenderBufferLoaderExtension *shared_buffer_loader =
         struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_fence_handle *fence = NULL;
            /* We need to flush for front buffer rendering when either we're using the
   * front buffer at the GL API level, or when EGL_KHR_mutable_render_buffer
   * has redirected GL_BACK to the front buffer.
   */
   if (statt != ST_ATTACHMENT_FRONT_LEFT &&
      (!ctx->is_shared_buffer_bound || statt != ST_ATTACHMENT_BACK_LEFT))
         /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            if (drawable->stvis.samples > 1) {
      /* Resolve the buffer used for front rendering. */
   dri_pipe_blit(ctx->st->pipe, drawable->textures[statt],
               if (drawable->textures[statt]) {
                  if (ctx->is_shared_buffer_bound) {
      /* is_shared_buffer_bound should only be true with image extension: */
   assert(image);
      } else {
                  if (image) {
      image->flushFrontBuffer(opaque_dri_drawable(drawable),
         if (ctx->is_shared_buffer_bound) {
                     shared_buffer_loader->displaySharedBuffer(opaque_dri_drawable(drawable),
                        }
   else if (loader->flushFrontBuffer) {
      loader->flushFrontBuffer(opaque_dri_drawable(drawable),
                  }
      /**
   * The struct dri_drawable flush_swapbuffers callback
   */
   static void
   dri2_flush_swapbuffers(struct dri_context *ctx,
         {
               if (image && image->flushSwapBuffers) {
      image->flushSwapBuffers(opaque_dri_drawable(drawable),
         }
      static void
   dri2_update_tex_buffer(struct dri_drawable *drawable,
               {
         }
      static const struct dri2_format_mapping r8_b8_g8_mapping = {
      DRM_FORMAT_YVU420,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_U_V,
   PIPE_FORMAT_R8_B8_G8_420_UNORM,
   3,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8 },
   { 2, 1, 1, __DRI_IMAGE_FORMAT_R8 },
      };
      static const struct dri2_format_mapping r8_g8_b8_mapping = {
      DRM_FORMAT_YUV420,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_U_V,
   PIPE_FORMAT_R8_G8_B8_420_UNORM,
   3,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8 },
   { 1, 1, 1, __DRI_IMAGE_FORMAT_R8 },
      };
      static const struct dri2_format_mapping r8_g8b8_mapping = {
      DRM_FORMAT_NV12,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_UV,
   PIPE_FORMAT_R8_G8B8_420_UNORM,
   2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8 },
      };
      static const struct dri2_format_mapping r8_b8g8_mapping = {
      DRM_FORMAT_NV21,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_UV,
   PIPE_FORMAT_R8_B8G8_420_UNORM,
   2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_R8 },
      };
      static const struct dri2_format_mapping r8g8_r8b8_mapping = {
      DRM_FORMAT_YUYV,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_XUXV,
   PIPE_FORMAT_R8G8_R8B8_UNORM, 2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88 },
      };
      static const struct dri2_format_mapping r8b8_r8g8_mapping = {
      DRM_FORMAT_YVYU,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_XUXV,
   PIPE_FORMAT_R8B8_R8G8_UNORM, 2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88 },
      };
      static const struct dri2_format_mapping b8r8_g8r8_mapping = {
      DRM_FORMAT_VYUY,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_XUXV,
   PIPE_FORMAT_B8R8_G8R8_UNORM, 2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88 },
      };
      static const struct dri2_format_mapping g8r8_b8r8_mapping = {
      DRM_FORMAT_UYVY,
   __DRI_IMAGE_FORMAT_NONE,
   __DRI_IMAGE_COMPONENTS_Y_XUXV,
   PIPE_FORMAT_G8R8_B8R8_UNORM, 2,
   { { 0, 0, 0, __DRI_IMAGE_FORMAT_GR88 },
      };
      static __DRIimage *
   dri2_create_image_from_winsys(__DRIscreen *_screen,
                           {
      struct dri_screen *screen = dri_screen(_screen);
   struct pipe_screen *pscreen = screen->base.screen;
   __DRIimage *img;
   struct pipe_resource templ;
   unsigned tex_usage = 0;
   int i;
   bool use_lowered = false;
            if (pscreen->is_format_supported(pscreen, map->pipe_format, screen->target, 0, 0,
               if (pscreen->is_format_supported(pscreen, map->pipe_format, screen->target, 0, 0,
                  /* For NV12, see if we have support for sampling r8_g8b8 */
   if (!tex_usage && map->pipe_format == PIPE_FORMAT_NV12 &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8_G8B8_420_UNORM,
         map = &r8_g8b8_mapping;
               /* For NV21, see if we have support for sampling r8_b8g8 */
   if (!tex_usage && map->pipe_format == PIPE_FORMAT_NV21 &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8_B8G8_420_UNORM,
         map = &r8_b8g8_mapping;
               /* For YV12 and I420, see if we have support for sampling r8_b8_g8 or r8_g8_b8 */
   if (!tex_usage && map->pipe_format == PIPE_FORMAT_IYUV) {
      if (map->dri_fourcc == DRM_FORMAT_YUV420 &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8_G8_B8_420_UNORM,
         map = &r8_g8_b8_mapping;
      } else if (map->dri_fourcc == DRM_FORMAT_YVU420 &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8_B8_G8_420_UNORM,
         map = &r8_b8_g8_mapping;
                  /* If the hardware supports R8G8_R8B8 style subsampled RGB formats, these
   * can be used for YUYV and UYVY formats.
   */
   if (!tex_usage && map->pipe_format == PIPE_FORMAT_YUYV &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8G8_R8B8_UNORM,
         map = &r8g8_r8b8_mapping;
               if (!tex_usage && map->pipe_format == PIPE_FORMAT_YVYU &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_R8B8_R8G8_UNORM,
         map = &r8b8_r8g8_mapping;
               if (!tex_usage && map->pipe_format == PIPE_FORMAT_UYVY &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_G8R8_B8R8_UNORM,
         map = &g8r8_b8r8_mapping;
               if (!tex_usage && map->pipe_format == PIPE_FORMAT_VYUY &&
      pscreen->is_format_supported(pscreen, PIPE_FORMAT_B8R8_G8R8_UNORM,
         map = &b8r8_g8r8_mapping;
               if (!tex_usage && util_format_is_yuv(map->pipe_format)) {
      /* YUV format sampling can be emulated by the GL gallium frontend by
   * using multiple samplers of varying formats.
   * If no tex_usage is set and we detect a YUV format,
   * test for support of all planes' sampler formats and
   * add sampler view usage.
   */
   use_lowered = true;
   if (dri2_yuv_dma_buf_supported(screen, map))
               if (!tex_usage)
            img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
            memset(&templ, 0, sizeof(templ));
   templ.bind = tex_usage | bind;
   templ.target = screen->target;
   templ.last_level = 0;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.width0 = width;
            for (i = num_handles - 1; i >= format_planes; i--) {
                        tex = pscreen->resource_from_handle(pscreen, &templ, &whandle[i],
         if (!tex) {
      pipe_resource_reference(&img->texture, NULL);
   FREE(img);
                           for (i = (use_lowered ? map->nplanes : format_planes) - 1; i >= 0; i--) {
               templ.next = img->texture;
   templ.width0 = width >> map->planes[i].width_shift;
   templ.height0 = height >> map->planes[i].height_shift;
   if (use_lowered)
         else
                  tex = pscreen->resource_from_handle(pscreen,
               if (!tex) {
      pipe_resource_reference(&img->texture, NULL);
   FREE(img);
               /* Reject image creation if there's an inconsistency between
   * content protection status of tex and img.
   */
   const struct driOptionCache *optionCache = &screen->dev->option_cache;
   if (driQueryOptionb(optionCache, "force_protected_content_check") &&
      (tex->bind & PIPE_BIND_PROTECTED) != (bind & PIPE_BIND_PROTECTED)) {
   pipe_resource_reference(&img->texture, NULL);
   pipe_resource_reference(&tex, NULL);
   FREE(img);
                           img->level = 0;
   img->layer = 0;
   img->use = 0;
   img->in_fence_fd = -1;
   img->loader_private = loaderPrivate;
               }
      static __DRIimage *
   dri2_create_image_from_name(__DRIscreen *_screen,
               {
      const struct dri2_format_mapping *map = dri2_get_mapping_by_format(format);
   struct winsys_handle whandle;
            if (!map)
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = WINSYS_HANDLE_TYPE_SHARED;
   whandle.handle = name;
   whandle.format = map->pipe_format;
                     img = dri2_create_image_from_winsys(_screen, width, height, map,
            if (!img)
            img->dri_components = map->dri_components;
   img->dri_fourcc = map->dri_fourcc;
               }
      static unsigned
   dri2_get_modifier_num_planes(__DRIscreen *_screen,
         {
      struct pipe_screen *pscreen = dri_screen(_screen)->base.screen;
            if (!map)
            switch (modifier) {
   case DRM_FORMAT_MOD_LINEAR:
   /* DRM_FORMAT_MOD_NONE is the same as LINEAR */
   case DRM_FORMAT_MOD_INVALID:
         default:
      if (!pscreen->is_dmabuf_modifier_supported ||
      !pscreen->is_dmabuf_modifier_supported(pscreen, modifier,
                     if (pscreen->get_dmabuf_modifier_planes) {
      return pscreen->get_dmabuf_modifier_planes(pscreen, modifier,
                     }
      static __DRIimage *
   dri2_create_image_from_fd(__DRIscreen *_screen,
                           {
      struct winsys_handle whandles[4];
   const struct dri2_format_mapping *map = dri2_get_mapping_by_fourcc(fourcc);
   __DRIimage *img = NULL;
   unsigned err = __DRI_IMAGE_ERROR_SUCCESS;
   int i;
            if (!map || expected_num_fds == 0) {
      err = __DRI_IMAGE_ERROR_BAD_MATCH;
               if (num_fds != expected_num_fds) {
      err = __DRI_IMAGE_ERROR_BAD_MATCH;
                        for (i = 0; i < num_fds; i++) {
      if (fds[i] < 0) {
      err = __DRI_IMAGE_ERROR_BAD_ALLOC;
               whandles[i].type = WINSYS_HANDLE_TYPE_FD;
   whandles[i].handle = (unsigned)fds[i];
   whandles[i].stride = (unsigned)strides[i];
   whandles[i].offset = (unsigned)offsets[i];
   whandles[i].format = map->pipe_format;
   whandles[i].modifier = modifier;
               img = dri2_create_image_from_winsys(_screen, width, height, map,
               if(img == NULL) {
      err = __DRI_IMAGE_ERROR_BAD_ALLOC;
               img->dri_components = map->dri_components;
   img->dri_fourcc = fourcc;
   img->dri_format = map->dri_format;
         exit:
      if (error)
               }
      static __DRIimage *
   dri2_create_image_common(__DRIscreen *_screen,
                           int width, int height,
   {
      const struct dri2_format_mapping *map = dri2_get_mapping_by_format(format);
   struct dri_screen *screen = dri_screen(_screen);
   struct pipe_screen *pscreen = screen->base.screen;
   __DRIimage *img;
   struct pipe_resource templ;
            if (!map)
            if (pscreen->is_format_supported(pscreen, map->pipe_format, screen->target,
               if (pscreen->is_format_supported(pscreen, map->pipe_format, screen->target,
                  if (!tex_usage)
            if (use & __DRI_IMAGE_USE_SCANOUT)
         if (use & __DRI_IMAGE_USE_SHARE)
         if (use & __DRI_IMAGE_USE_LINEAR)
         if (use & __DRI_IMAGE_USE_CURSOR) {
      if (width != 64 || height != 64)
            }
   if (use & __DRI_IMAGE_USE_PROTECTED)
         if (use & __DRI_IMAGE_USE_PRIME_BUFFER)
         if (use & __DRI_IMAGE_USE_FRONT_RENDERING)
            img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
            memset(&templ, 0, sizeof(templ));
   templ.bind = tex_usage;
   templ.format = map->pipe_format;
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
            if (modifiers)
      img->texture =
      screen->base.screen
      ->resource_create_with_modifiers(screen->base.screen,
            else
      img->texture =
      if (!img->texture) {
      FREE(img);
               img->level = 0;
   img->layer = 0;
   img->dri_format = format;
   img->dri_fourcc = map->dri_fourcc;
   img->dri_components = 0;
   img->use = use;
            img->loader_private = loaderPrivate;
   img->screen = screen;
      }
      static __DRIimage *
   dri2_create_image(__DRIscreen *_screen,
               {
      return dri2_create_image_common(_screen, width, height, format, use,
            }
      static __DRIimage *
   dri2_create_image_with_modifiers(__DRIscreen *dri_screen,
                           {
      return dri2_create_image_common(dri_screen, width, height, format,
            }
      static __DRIimage *
   dri2_create_image_with_modifiers2(__DRIscreen *dri_screen,
                           {
      return dri2_create_image_common(dri_screen, width, height, format, use,
      }
      static bool
   dri2_query_image_common(__DRIimage *image, int attrib, int *value)
   {
      switch (attrib) {
   case __DRI_IMAGE_ATTRIB_FORMAT:
      *value = image->dri_format;
      case __DRI_IMAGE_ATTRIB_WIDTH:
      *value = image->texture->width0;
      case __DRI_IMAGE_ATTRIB_HEIGHT:
      *value = image->texture->height0;
      case __DRI_IMAGE_ATTRIB_COMPONENTS:
      if (image->dri_components == 0)
         *value = image->dri_components;
      case __DRI_IMAGE_ATTRIB_FOURCC:
      if (image->dri_fourcc) {
         } else {
               map = dri2_get_mapping_by_format(image->dri_format);
                     }
      default:
            }
      static bool
   dri2_query_image_by_resource_handle(__DRIimage *image, int attrib, int *value)
   {
      struct pipe_screen *pscreen = image->texture->screen;
   struct winsys_handle whandle;
   struct pipe_resource *tex;
   unsigned usage;
   memset(&whandle, 0, sizeof(whandle));
   whandle.plane = image->plane;
            switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
   case __DRI_IMAGE_ATTRIB_OFFSET:
   case __DRI_IMAGE_ATTRIB_HANDLE:
      whandle.type = WINSYS_HANDLE_TYPE_KMS;
      case __DRI_IMAGE_ATTRIB_NAME:
      whandle.type = WINSYS_HANDLE_TYPE_SHARED;
      case __DRI_IMAGE_ATTRIB_FD:
      whandle.type = WINSYS_HANDLE_TYPE_FD;
      case __DRI_IMAGE_ATTRIB_NUM_PLANES:
      for (i = 0, tex = image->texture; tex; tex = tex->next)
         *value = i;
      case __DRI_IMAGE_ATTRIB_MODIFIER_UPPER:
   case __DRI_IMAGE_ATTRIB_MODIFIER_LOWER:
      whandle.type = WINSYS_HANDLE_TYPE_KMS;
   whandle.modifier = DRM_FORMAT_MOD_INVALID;
      default:
                           if (image->use & __DRI_IMAGE_USE_BACKBUFFER)
            if (!pscreen->resource_get_handle(pscreen, NULL, image->texture,
                  switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      *value = whandle.stride;
      case __DRI_IMAGE_ATTRIB_OFFSET:
      *value = whandle.offset;
      case __DRI_IMAGE_ATTRIB_HANDLE:
   case __DRI_IMAGE_ATTRIB_NAME:
   case __DRI_IMAGE_ATTRIB_FD:
      *value = whandle.handle;
      case __DRI_IMAGE_ATTRIB_MODIFIER_UPPER:
      if (whandle.modifier == DRM_FORMAT_MOD_INVALID)
         *value = (whandle.modifier >> 32) & 0xffffffff;
      case __DRI_IMAGE_ATTRIB_MODIFIER_LOWER:
      if (whandle.modifier == DRM_FORMAT_MOD_INVALID)
         *value = whandle.modifier & 0xffffffff;
      default:
            }
      static bool
   dri2_resource_get_param(__DRIimage *image, enum pipe_resource_param param,
         {
      struct pipe_screen *pscreen = image->texture->screen;
   if (!pscreen->resource_get_param)
            if (image->use & __DRI_IMAGE_USE_BACKBUFFER)
            return pscreen->resource_get_param(pscreen, NULL, image->texture,
            }
      static bool
   dri2_query_image_by_resource_param(__DRIimage *image, int attrib, int *value)
   {
      enum pipe_resource_param param;
   uint64_t res_param;
            if (!image->texture->screen->resource_get_param)
            switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
      param = PIPE_RESOURCE_PARAM_STRIDE;
      case __DRI_IMAGE_ATTRIB_OFFSET:
      param = PIPE_RESOURCE_PARAM_OFFSET;
      case __DRI_IMAGE_ATTRIB_NUM_PLANES:
      param = PIPE_RESOURCE_PARAM_NPLANES;
      case __DRI_IMAGE_ATTRIB_MODIFIER_UPPER:
   case __DRI_IMAGE_ATTRIB_MODIFIER_LOWER:
      param = PIPE_RESOURCE_PARAM_MODIFIER;
      case __DRI_IMAGE_ATTRIB_HANDLE:
      param = PIPE_RESOURCE_PARAM_HANDLE_TYPE_KMS;
      case __DRI_IMAGE_ATTRIB_NAME:
      param = PIPE_RESOURCE_PARAM_HANDLE_TYPE_SHARED;
      case __DRI_IMAGE_ATTRIB_FD:
      param = PIPE_RESOURCE_PARAM_HANDLE_TYPE_FD;
      default:
                           if (!dri2_resource_get_param(image, param, handle_usage, &res_param))
            switch (attrib) {
   case __DRI_IMAGE_ATTRIB_STRIDE:
   case __DRI_IMAGE_ATTRIB_OFFSET:
   case __DRI_IMAGE_ATTRIB_NUM_PLANES:
      if (res_param > INT_MAX)
         *value = (int)res_param;
      case __DRI_IMAGE_ATTRIB_HANDLE:
   case __DRI_IMAGE_ATTRIB_NAME:
   case __DRI_IMAGE_ATTRIB_FD:
      if (res_param > UINT_MAX)
         *value = (int)res_param;
      case __DRI_IMAGE_ATTRIB_MODIFIER_UPPER:
      if (res_param == DRM_FORMAT_MOD_INVALID)
         *value = (res_param >> 32) & 0xffffffff;
      case __DRI_IMAGE_ATTRIB_MODIFIER_LOWER:
      if (res_param == DRM_FORMAT_MOD_INVALID)
         *value = res_param & 0xffffffff;
      default:
            }
      static GLboolean
   dri2_query_image(__DRIimage *image, int attrib, int *value)
   {
      if (dri2_query_image_common(image, attrib, value))
         else if (dri2_query_image_by_resource_param(image, attrib, value))
         else if (dri2_query_image_by_resource_handle(image, attrib, value))
         else
      }
      static __DRIimage *
   dri2_dup_image(__DRIimage *image, void *loaderPrivate)
   {
               img = CALLOC_STRUCT(__DRIimageRec);
   if (!img)
            img->texture = NULL;
   pipe_resource_reference(&img->texture, image->texture);
   img->level = image->level;
   img->layer = image->layer;
   img->dri_format = image->dri_format;
   img->internal_format = image->internal_format;
   /* This should be 0 for sub images, but dup is also used for base images. */
   img->dri_components = image->dri_components;
   img->use = image->use;
   img->in_fence_fd = (image->in_fence_fd > 0) ?
         img->loader_private = loaderPrivate;
               }
      static GLboolean
   dri2_validate_usage(__DRIimage *image, unsigned int use)
   {
      if (!image || !image->texture)
            struct pipe_screen *screen = image->texture->screen;
   if (!screen->check_resource_capability)
            /* We don't want to check these:
   *   __DRI_IMAGE_USE_SHARE (all images are shareable)
   *   __DRI_IMAGE_USE_BACKBUFFER (all images support this)
   */
   unsigned bind = 0;
   if (use & __DRI_IMAGE_USE_SCANOUT)
         if (use & __DRI_IMAGE_USE_LINEAR)
         if (use & __DRI_IMAGE_USE_CURSOR)
            if (!bind)
               }
      static __DRIimage *
   dri2_from_names(__DRIscreen *screen, int width, int height, int fourcc,
               {
      const struct dri2_format_mapping *map = dri2_get_mapping_by_fourcc(fourcc);
   __DRIimage *img;
            if (!map)
            if (num_names != 1)
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = WINSYS_HANDLE_TYPE_SHARED;
   whandle.handle = names[0];
   whandle.stride = strides[0];
   whandle.offset = offsets[0];
   whandle.format = map->pipe_format;
            img = dri2_create_image_from_winsys(screen, width, height, map,
         if (img == NULL)
            img->dri_components = map->dri_components;
   img->dri_fourcc = map->dri_fourcc;
               }
      static __DRIimage *
   dri2_from_planar(__DRIimage *image, int plane, void *loaderPrivate)
   {
               if (plane < 0) {
         } else if (plane > 0) {
      uint64_t planes;
   if (!dri2_resource_get_param(image, PIPE_RESOURCE_PARAM_NPLANES, 0,
            plane >= planes) {
                  if (image->dri_components == 0) {
      uint64_t modifier;
   if (!dri2_resource_get_param(image, PIPE_RESOURCE_PARAM_MODIFIER, 0,
            modifier == DRM_FORMAT_MOD_INVALID) {
                  img = dri2_dup_image(image, loaderPrivate);
   if (img == NULL)
            if (img->texture->screen->resource_changed)
      img->texture->screen->resource_changed(img->texture->screen,
         /* set this to 0 for sub images. */
   img->dri_components = 0;
   img->plane = plane;
      }
      static __DRIimage *
   dri2_from_fds(__DRIscreen *screen, int width, int height, int fourcc,
               {
      return dri2_create_image_from_fd(screen, width, height, fourcc,
            }
      static __DRIimage *
   dri2_from_fds2(__DRIscreen *screen, int width, int height, int fourcc,
               {
      unsigned bind = 0;
   if (flags & __DRI_IMAGE_PROTECTED_CONTENT_FLAG)
         if (flags & __DRI_IMAGE_PRIME_LINEAR_BUFFER)
            return dri2_create_image_from_fd(screen, width, height, fourcc,
            }
      static bool
   dri2_query_dma_buf_modifiers(__DRIscreen *_screen, int fourcc, int max,
               {
      struct dri_screen *screen = dri_screen(_screen);
   struct pipe_screen *pscreen = screen->base.screen;
   const struct dri2_format_mapping *map = dri2_get_mapping_by_fourcc(fourcc);
            if (!map)
                     bool native_sampling = pscreen->is_format_supported(pscreen, format, screen->target, 0, 0,
         if (pscreen->is_format_supported(pscreen, format, screen->target, 0, 0,
            native_sampling ||
   dri2_yuv_dma_buf_supported(screen, map))  {
   if (pscreen->query_dmabuf_modifiers != NULL) {
      pscreen->query_dmabuf_modifiers(pscreen, format, max, modifiers,
         if (!native_sampling && external_only) {
      /* To support it using YUV lowering, we need it to be samplerExternalOES.
   */
   for (int i = 0; i < *count; i++)
         } else {
         }
      }
      }
      static bool
   dri2_query_dma_buf_format_modifier_attribs(__DRIscreen *_screen,
               {
      struct dri_screen *screen = dri_screen(_screen);
            if (!pscreen->query_dmabuf_modifiers)
            switch (attrib) {
   case __DRI_IMAGE_FORMAT_MODIFIER_ATTRIB_PLANE_COUNT: {
      uint64_t mod_planes = dri2_get_modifier_num_planes(_screen, modifier,
         if (mod_planes > 0)
            }
   default:
            }
      static __DRIimage *
   dri2_from_dma_bufs(__DRIscreen *screen,
                     int width, int height, int fourcc,
   int *fds, int num_fds,
   int *strides, int *offsets,
   enum __DRIYUVColorSpace yuv_color_space,
   enum __DRISampleRange sample_range,
   enum __DRIChromaSiting horizontal_siting,
   {
               img = dri2_create_image_from_fd(screen, width, height, fourcc,
               if (img == NULL)
            img->yuv_color_space = yuv_color_space;
   img->sample_range = sample_range;
   img->horizontal_siting = horizontal_siting;
            *error = __DRI_IMAGE_ERROR_SUCCESS;
      }
      static __DRIimage *
   dri2_from_dma_bufs2(__DRIscreen *screen,
                     int width, int height, int fourcc,
   uint64_t modifier, int *fds, int num_fds,
   int *strides, int *offsets,
   enum __DRIYUVColorSpace yuv_color_space,
   enum __DRISampleRange sample_range,
   enum __DRIChromaSiting horizontal_siting,
   {
               img = dri2_create_image_from_fd(screen, width, height, fourcc,
               if (img == NULL)
            img->yuv_color_space = yuv_color_space;
   img->sample_range = sample_range;
   img->horizontal_siting = horizontal_siting;
            *error = __DRI_IMAGE_ERROR_SUCCESS;
      }
      static __DRIimage *
   dri2_from_dma_bufs3(__DRIscreen *screen,
                     int width, int height, int fourcc,
   uint64_t modifier, int *fds, int num_fds,
   int *strides, int *offsets,
   enum __DRIYUVColorSpace yuv_color_space,
   enum __DRISampleRange sample_range,
   enum __DRIChromaSiting horizontal_siting,
   enum __DRIChromaSiting vertical_siting,
   {
               img = dri2_create_image_from_fd(screen, width, height, fourcc,
                           if (img == NULL)
            img->yuv_color_space = yuv_color_space;
   img->sample_range = sample_range;
   img->horizontal_siting = horizontal_siting;
            *error = __DRI_IMAGE_ERROR_SUCCESS;
      }
      static void
   dri2_blit_image(__DRIcontext *context, __DRIimage *dst, __DRIimage *src,
                     {
      struct dri_context *ctx = dri_context(context);
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen;
   struct pipe_fence_handle *fence;
            if (!dst || !src)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
                     memset(&blit, 0, sizeof(blit));
   blit.dst.resource = dst->texture;
   blit.dst.box.x = dstx0;
   blit.dst.box.y = dsty0;
   blit.dst.box.width = dstwidth;
   blit.dst.box.height = dstheight;
   blit.dst.box.depth = 1;
   blit.dst.format = dst->texture->format;
   blit.src.resource = src->texture;
   blit.src.box.x = srcx0;
   blit.src.box.y = srcy0;
   blit.src.box.width = srcwidth;
   blit.src.box.height = srcheight;
   blit.src.box.depth = 1;
   blit.src.format = src->texture->format;
   blit.mask = PIPE_MASK_RGBA;
                     if (flush_flag == __BLIT_FLAG_FLUSH) {
      pipe->flush_resource(pipe, dst->texture);
      } else if (flush_flag == __BLIT_FLAG_FINISH) {
      screen = ctx->screen->base.screen;
   pipe->flush_resource(pipe, dst->texture);
   st_context_flush(ctx->st, 0, &fence, NULL, NULL);
   (void) screen->fence_finish(screen, NULL, fence, OS_TIMEOUT_INFINITE);
         }
      static void *
   dri2_map_image(__DRIcontext *context, __DRIimage *image,
               {
      struct dri_context *ctx = dri_context(context);
   struct pipe_context *pipe = ctx->st->pipe;
   enum pipe_map_flags pipe_access = 0;
   struct pipe_transfer *trans;
            if (!image || !data || *data)
            unsigned plane = image->plane;
   if (plane >= dri2_get_mapping_by_format(image->dri_format)->nplanes)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
                     struct pipe_resource *resource = image->texture;
   while (plane--)
            if (flags & __DRI_IMAGE_TRANSFER_READ)
         if (flags & __DRI_IMAGE_TRANSFER_WRITE)
            map = pipe_texture_map(pipe, resource, 0, 0, pipe_access, x0, y0,
         if (map) {
      *data = trans;
                  }
      static void
   dri2_unmap_image(__DRIcontext *context, __DRIimage *image, void *data)
   {
      struct dri_context *ctx = dri_context(context);
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
               }
      static int
   dri2_get_capabilities(__DRIscreen *_screen)
   {
                  }
      /* The extension is modified during runtime if DRI_PRIME is detected */
   static const __DRIimageExtension dri2ImageExtensionTempl = {
               .createImageFromName          = dri2_create_image_from_name,
   .createImageFromRenderbuffer  = dri2_create_image_from_renderbuffer,
   .destroyImage                 = dri2_destroy_image,
   .createImage                  = dri2_create_image,
   .queryImage                   = dri2_query_image,
   .dupImage                     = dri2_dup_image,
   .validateUsage                = dri2_validate_usage,
   .createImageFromNames         = dri2_from_names,
   .fromPlanar                   = dri2_from_planar,
   .createImageFromTexture       = dri2_create_from_texture,
   .createImageFromFds           = NULL,
   .createImageFromFds2          = NULL,
   .createImageFromDmaBufs       = NULL,
   .blitImage                    = dri2_blit_image,
   .getCapabilities              = dri2_get_capabilities,
   .mapImage                     = dri2_map_image,
   .unmapImage                   = dri2_unmap_image,
   .createImageWithModifiers     = NULL,
   .createImageFromDmaBufs2      = NULL,
   .createImageFromDmaBufs3      = NULL,
   .queryDmaBufFormats           = NULL,
   .queryDmaBufModifiers         = NULL,
   .queryDmaBufFormatModifierAttribs = NULL,
   .createImageFromRenderbuffer2 = dri2_create_image_from_renderbuffer2,
      };
      const __DRIimageExtension driVkImageExtension = {
               .createImageFromName          = dri2_create_image_from_name,
   .createImageFromRenderbuffer  = dri2_create_image_from_renderbuffer,
   .destroyImage                 = dri2_destroy_image,
   .createImage                  = dri2_create_image,
   .queryImage                   = dri2_query_image,
   .dupImage                     = dri2_dup_image,
   .validateUsage                = dri2_validate_usage,
   .createImageFromNames         = dri2_from_names,
   .fromPlanar                   = dri2_from_planar,
   .createImageFromTexture       = dri2_create_from_texture,
   .createImageFromFds           = dri2_from_fds,
   .createImageFromFds2          = dri2_from_fds2,
   .createImageFromDmaBufs       = dri2_from_dma_bufs,
   .blitImage                    = dri2_blit_image,
   .getCapabilities              = dri2_get_capabilities,
   .mapImage                     = dri2_map_image,
   .unmapImage                   = dri2_unmap_image,
   .createImageWithModifiers     = dri2_create_image_with_modifiers,
   .createImageFromDmaBufs2      = dri2_from_dma_bufs2,
   .createImageFromDmaBufs3      = dri2_from_dma_bufs3,
   .queryDmaBufFormats           = dri2_query_dma_buf_formats,
   .queryDmaBufModifiers         = dri2_query_dma_buf_modifiers,
   .queryDmaBufFormatModifierAttribs = dri2_query_dma_buf_format_modifier_attribs,
   .createImageFromRenderbuffer2 = dri2_create_image_from_renderbuffer2,
      };
      const __DRIimageExtension driVkImageExtensionSw = {
               .createImageFromName          = dri2_create_image_from_name,
   .createImageFromRenderbuffer  = dri2_create_image_from_renderbuffer,
   .destroyImage                 = dri2_destroy_image,
   .createImage                  = dri2_create_image,
   .queryImage                   = dri2_query_image,
   .dupImage                     = dri2_dup_image,
   .validateUsage                = dri2_validate_usage,
   .createImageFromNames         = dri2_from_names,
   .fromPlanar                   = dri2_from_planar,
   .createImageFromTexture       = dri2_create_from_texture,
   .createImageFromFds           = dri2_from_fds,
   .createImageFromFds2          = dri2_from_fds2,
   .blitImage                    = dri2_blit_image,
   .getCapabilities              = dri2_get_capabilities,
   .mapImage                     = dri2_map_image,
   .unmapImage                   = dri2_unmap_image,
      };
      static const __DRIrobustnessExtension dri2Robustness = {
         };
      static int
   dri2_interop_query_device_info(__DRIcontext *_ctx,
         {
         }
      static int
   dri2_interop_export_object(__DRIcontext *_ctx,
               {
         }
      static int
   dri2_interop_flush_objects(__DRIcontext *_ctx,
               {
         }
      static const __DRI2interopExtension dri2InteropExtension = {
      .base = { __DRI2_INTEROP, 2 },
   .query_device_info = dri2_interop_query_device_info,
   .export_object = dri2_interop_export_object,
      };
      /**
   * \brief the DRI2bufferDamageExtension set_damage_region method
   */
   static void
   dri2_set_damage_region(__DRIdrawable *dPriv, unsigned int nrects, int *rects)
   {
      struct dri_drawable *drawable = dri_drawable(dPriv);
            if (nrects) {
      boxes = CALLOC(nrects, sizeof(*boxes));
            for (unsigned int i = 0; i < nrects; i++) {
                              FREE(drawable->damage_rects);
   drawable->damage_rects = boxes;
            /* Only apply the damage region if the BACK_LEFT texture is up-to-date. */
   if (drawable->texture_stamp == drawable->lastStamp &&
      (drawable->texture_mask & (1 << ST_ATTACHMENT_BACK_LEFT))) {
   struct pipe_screen *screen = drawable->screen->base.screen;
            if (drawable->stvis.samples > 1)
         else
            screen->set_damage_region(screen, resource,
               }
      static const __DRI2bufferDamageExtension dri2BufferDamageExtensionTempl = {
         };
      /**
   * \brief the DRI2ConfigQueryExtension configQueryb method
   */
   static int
   dri2GalliumConfigQueryb(__DRIscreen *sPriv, const char *var,
         {
               if (!driCheckOption(&screen->dev->option_cache, var, DRI_BOOL))
                        }
      /**
   * \brief the DRI2ConfigQueryExtension configQueryi method
   */
   static int
   dri2GalliumConfigQueryi(__DRIscreen *sPriv, const char *var, int *val)
   {
               if (!driCheckOption(&screen->dev->option_cache, var, DRI_INT) &&
      !driCheckOption(&screen->dev->option_cache, var, DRI_ENUM))
                     }
      /**
   * \brief the DRI2ConfigQueryExtension configQueryf method
   */
   static int
   dri2GalliumConfigQueryf(__DRIscreen *sPriv, const char *var, float *val)
   {
               if (!driCheckOption(&screen->dev->option_cache, var, DRI_FLOAT))
                        }
      /**
   * \brief the DRI2ConfigQueryExtension configQuerys method
   */
   static int
   dri2GalliumConfigQuerys(__DRIscreen *sPriv, const char *var, char **val)
   {
               if (!driCheckOption(&screen->dev->option_cache, var, DRI_STRING))
                        }
      /**
   * \brief the DRI2ConfigQueryExtension struct.
   *
   * We first query the driver option cache. Then the dri2 option cache.
   */
   static const __DRI2configQueryExtension dri2GalliumConfigQueryExtension = {
               .configQueryb        = dri2GalliumConfigQueryb,
   .configQueryi        = dri2GalliumConfigQueryi,
   .configQueryf        = dri2GalliumConfigQueryf,
      };
      /**
   * \brief the DRI2blobExtension set_cache_funcs method
   */
   static void
   set_blob_cache_funcs(__DRIscreen *sPriv, __DRIblobCacheSet set,
         {
      struct dri_screen *screen = dri_screen(sPriv);
            if (!pscreen->get_disk_shader_cache)
                     if (!cache)
               }
      static const __DRI2blobExtension driBlobExtension = {
      .base = { __DRI2_BLOB, 1 },
      };
      static const __DRImutableRenderBufferDriverExtension driMutableRenderBufferExtension = {
         };
      /*
   * Backend function init_screen.
   */
      static const __DRIextension *dri_screen_extensions_base[] = {
      &driTexBufferExtension.base,
   &dri2FlushExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2GalliumConfigQueryExtension.base,
   &dri2ThrottleExtension.base,
   &dri2FenceExtension.base,
   &dri2InteropExtension.base,
   &driBlobExtension.base,
   &driMutableRenderBufferExtension.base,
      };
      /**
   * Set up the DRI extension list for this screen based on its underlying
   * gallium screen's capabilities.
   */
   static void
   dri2_init_screen_extensions(struct dri_screen *screen,
               {
               STATIC_ASSERT(sizeof(screen->screen_extensions) >=
         memcpy(&screen->screen_extensions, dri_screen_extensions_base,
                  /* Point nExt at the end of the extension list */
            screen->image_extension = dri2ImageExtensionTempl;
   if (pscreen->resource_create_with_modifiers) {
      screen->image_extension.createImageWithModifiers =
         screen->image_extension.createImageWithModifiers2 =
               if (pscreen->get_param(pscreen, PIPE_CAP_NATIVE_FENCE_FD)) {
                  if (pscreen->get_param(pscreen, PIPE_CAP_DMABUF) & DRM_PRIME_CAP_IMPORT) {
      screen->image_extension.createImageFromFds = dri2_from_fds;
   screen->image_extension.createImageFromFds2 = dri2_from_fds2;
   screen->image_extension.createImageFromDmaBufs = dri2_from_dma_bufs;
   screen->image_extension.createImageFromDmaBufs2 = dri2_from_dma_bufs2;
   screen->image_extension.createImageFromDmaBufs3 = dri2_from_dma_bufs3;
   screen->image_extension.queryDmaBufFormats =
         screen->image_extension.queryDmaBufModifiers =
         if (!is_kms_screen) {
      screen->image_extension.queryDmaBufFormatModifierAttribs =
         }
            if (!is_kms_screen) {
      screen->buffer_damage_extension = dri2BufferDamageExtensionTempl;
   if (pscreen->set_damage_region)
      screen->buffer_damage_extension.set_damage_region =
                  if (pscreen->get_param(pscreen, PIPE_CAP_DEVICE_RESET_STATUS_QUERY)) {
      *nExt++ = &dri2Robustness.base;
               /* Ensure the extension list didn't overrun its buffer and is still
   * NULL-terminated */
   assert(nExt - screen->screen_extensions <=
            }
      static struct dri_drawable *
   dri2_create_drawable(struct dri_screen *screen, const struct gl_config *visual,
         {
      struct dri_drawable *drawable = dri_create_drawable(screen, visual, isPixmap,
         if (!drawable)
            drawable->allocate_textures = dri2_allocate_textures;
   drawable->flush_frontbuffer = dri2_flush_frontbuffer;
   drawable->update_tex_buffer = dri2_update_tex_buffer;
               }
      /**
   * This is the driver specific part of the createNewScreen entry point.
   *
   * Returns the struct gl_config supported by this driver.
   */
   static const __DRIconfig **
   dri2_init_screen(struct dri_screen *screen)
   {
      const __DRIconfig **configs;
                     if (pipe_loader_drm_probe_fd(&screen->dev, screen->fd, false))
            if (!pscreen)
            dri_init_options(screen);
                     if (pscreen->get_param(pscreen, PIPE_CAP_DEVICE_PROTECTED_CONTEXT))
            configs = dri_init_screen(screen, pscreen);
   if (!configs)
            screen->can_share_buffer = true;
   screen->auto_fake_front = dri_with_format(screen);
            const __DRIimageLookupExtension *loader = screen->dri2.image;
   if (loader &&
      loader->base.version >= 2 &&
   loader->validateEGLImage &&
   loader->lookupEGLImageValidated) {
   screen->validate_egl_image = dri2_validate_egl_image;
               screen->create_drawable = dri2_create_drawable;
   screen->allocate_buffer = dri2_allocate_buffer;
                  fail:
                  }
      /**
   * This is the driver specific part of the createNewScreen entry point.
   *
   * Returns the struct gl_config supported by this driver.
   */
   static const __DRIconfig **
   dri_swrast_kms_init_screen(struct dri_screen *screen)
   {
   #if defined(GALLIUM_SOFTPIPE)
      const __DRIconfig **configs;
         #ifdef HAVE_DRISW_KMS
      if (pipe_loader_sw_probe_kms(&screen->dev, screen->fd))
      #endif
         if (!pscreen)
            dri_init_options(screen);
            configs = dri_init_screen(screen, pscreen);
   if (!configs)
            screen->can_share_buffer = false;
   screen->auto_fake_front = dri_with_format(screen);
            const __DRIimageLookupExtension *loader = screen->dri2.image;
   if (loader &&
      loader->base.version >= 2 &&
   loader->validateEGLImage &&
   loader->lookupEGLImageValidated) {
   screen->validate_egl_image = dri2_validate_egl_image;
               screen->create_drawable = dri2_create_drawable;
   screen->allocate_buffer = dri2_allocate_buffer;
                  fail:
            #endif // GALLIUM_SOFTPIPE
         }
      static int
   dri_query_compatible_render_only_device_fd(int kms_only_fd)
   {
         }
      static const struct __DRImesaCoreExtensionRec mesaCoreExtension = {
      .base = { __DRI_MESA, 1 },
   .version_string = MESA_INTERFACE_VERSION_STRING,
   .createNewScreen = driCreateNewScreen2,
   .createContext = driCreateContextAttribs,
   .initScreen = dri2_init_screen,
      };
      /* This is the table of extensions that the loader will dlsym() for. */
   const __DRIextension *galliumdrm_driver_extensions[] = {
      &driCoreExtension.base,
   &mesaCoreExtension.base,
   &driImageDriverExtension.base,
   &driDRI2Extension.base,
   &gallium_config_options.base,
      };
      static const struct __DRImesaCoreExtensionRec swkmsMesaCoreExtension = {
      .base = { __DRI_MESA, 1 },
   .version_string = MESA_INTERFACE_VERSION_STRING,
   .createNewScreen = driCreateNewScreen2,
   .createContext = driCreateContextAttribs,
      };
      const __DRIextension *dri_swrast_kms_driver_extensions[] = {
      &driCoreExtension.base,
   &swkmsMesaCoreExtension.base,
   &driImageDriverExtension.base,
   &swkmsDRI2Extension.base,
   &gallium_config_options.base,
      };
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
