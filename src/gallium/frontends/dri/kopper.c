   /*
   * Copyright 2020 Red Hat, Inc.
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
      #include "GL/internal/mesa_interface.h"
   #include "git_sha1.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/u_box.h"
   #include "util/log.h"
   #include "pipe/p_context.h"
   #include "pipe-loader/pipe_loader.h"
   #include "state_tracker/st_context.h"
   #include "zink/zink_public.h"
   #include "zink/zink_kopper.h"
   #include "driver_trace/tr_screen.h"
      #include "dri_screen.h"
   #include "dri_context.h"
   #include "dri_drawable.h"
   #include "dri_helpers.h"
   #include "dri_query_renderer.h"
      #include <vulkan/vulkan.h>
      #ifdef VK_USE_PLATFORM_XCB_KHR
   #include <xcb/xcb.h>
   #include <xcb/dri3.h>
   #include <xcb/present.h>
   #include <xcb/xfixes.h>
   #include "util/libsync.h"
   #include <X11/Xlib-xcb.h>
   #include "drm-uapi/drm_fourcc.h"
   #endif
      extern const __DRIimageExtension driVkImageExtension;
   extern const __DRIimageExtension driVkImageExtensionSw;
      static struct dri_drawable *
   kopper_create_drawable(struct dri_screen *screen, const struct gl_config *visual,
            static inline void
   kopper_invalidate_drawable(__DRIdrawable *dPriv)
   {
                           }
      static const __DRI2flushExtension driVkFlushExtension = {
               .flush                = dri_flush_drawable,
   .invalidate           = kopper_invalidate_drawable,
      };
      static const __DRIrobustnessExtension dri2Robustness = {
         };
      const __DRIkopperExtension driKopperExtension;
      static const __DRIextension *drivk_screen_extensions[] = {
      &driTexBufferExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2ConfigQueryExtension.base,
   &dri2FenceExtension.base,
   &dri2Robustness.base,
   &driVkImageExtension.base,
   &dri2FlushControlExtension.base,
   &driVkFlushExtension.base,
   &driKopperExtension.base,
      };
      static const __DRIextension *drivk_sw_screen_extensions[] = {
      &driTexBufferExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2ConfigQueryExtension.base,
   &dri2FenceExtension.base,
   &dri2Robustness.base,
   &driVkImageExtensionSw.base,
   &dri2FlushControlExtension.base,
   &driVkFlushExtension.base,
      };
      static const __DRIconfig **
   kopper_init_screen(struct dri_screen *screen)
   {
      const __DRIconfig **configs;
            if (!screen->kopper_loader) {
      fprintf(stderr, "mesa: Kopper interface not found!\n"
                                    bool success;
   if (screen->fd != -1)
         else
            if (success)
            if (!pscreen)
            dri_init_options(screen);
            configs = dri_init_screen(screen, pscreen);
   if (!configs)
            assert(pscreen->get_param(pscreen, PIPE_CAP_DEVICE_RESET_STATUS_QUERY));
   screen->has_reset_status_query = true;
   screen->lookup_egl_image = dri2_lookup_egl_image;
   screen->has_dmabuf = pscreen->get_param(pscreen, PIPE_CAP_DMABUF);
   screen->has_modifiers = pscreen->query_dmabuf_modifiers != NULL;
   screen->is_sw = zink_kopper_is_cpu(pscreen);
   if (screen->has_dmabuf)
         else
            const __DRIimageLookupExtension *image = screen->dri2.image;
   if (image &&
      image->base.version >= 2 &&
   image->validateEGLImage &&
   image->lookupEGLImageValidated) {
   screen->validate_egl_image = dri2_validate_egl_image;
                           fail:
      dri_release_screen(screen);
      }
      // copypasta alert
      extern bool
   dri_image_drawable_get_buffers(struct dri_drawable *drawable,
                        #ifdef VK_USE_PLATFORM_XCB_KHR
   static int
   get_dri_format(enum pipe_format pf)
   {
      int image_format;
   switch (pf) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      image_format = __DRI_IMAGE_FORMAT_ABGR16161616F;
      case PIPE_FORMAT_R16G16B16X16_FLOAT:
      image_format = __DRI_IMAGE_FORMAT_XBGR16161616F;
      case PIPE_FORMAT_B5G6R5_UNORM:
      image_format = __DRI_IMAGE_FORMAT_RGB565;
      case PIPE_FORMAT_B5G5R5A1_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB1555;
      case PIPE_FORMAT_R5G5B5A1_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR1555;
      case PIPE_FORMAT_B4G4R4A4_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ARGB4444;
      case PIPE_FORMAT_R4G4B4A4_UNORM:
      image_format = __DRI_IMAGE_FORMAT_ABGR4444;
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
      default:
      image_format = __DRI_IMAGE_FORMAT_NONE;
      }
      }
      /* the DRIimage createImage function takes __DRI_IMAGE_FORMAT codes, while
   * the createImageFromFds call takes DRM_FORMAT codes. To avoid
   * complete confusion, just deal in __DRI_IMAGE_FORMAT codes for now and
   * translate to DRM_FORMAT codes in the call to createImageFromFds
   */
   static int
   image_format_to_fourcc(int format)
   {
         /* Convert from __DRI_IMAGE_FORMAT to DRM_FORMAT (sigh) */
   switch (format) {
   case __DRI_IMAGE_FORMAT_SARGB8: return __DRI_IMAGE_FOURCC_SARGB8888;
   case __DRI_IMAGE_FORMAT_SABGR8: return __DRI_IMAGE_FOURCC_SABGR8888;
   case __DRI_IMAGE_FORMAT_SXRGB8: return __DRI_IMAGE_FOURCC_SXRGB8888;
   case __DRI_IMAGE_FORMAT_RGB565: return DRM_FORMAT_RGB565;
   case __DRI_IMAGE_FORMAT_XRGB8888: return DRM_FORMAT_XRGB8888;
   case __DRI_IMAGE_FORMAT_ARGB8888: return DRM_FORMAT_ARGB8888;
   case __DRI_IMAGE_FORMAT_ABGR8888: return DRM_FORMAT_ABGR8888;
   case __DRI_IMAGE_FORMAT_XBGR8888: return DRM_FORMAT_XBGR8888;
   case __DRI_IMAGE_FORMAT_XRGB2101010: return DRM_FORMAT_XRGB2101010;
   case __DRI_IMAGE_FORMAT_ARGB2101010: return DRM_FORMAT_ARGB2101010;
   case __DRI_IMAGE_FORMAT_XBGR2101010: return DRM_FORMAT_XBGR2101010;
   case __DRI_IMAGE_FORMAT_ABGR2101010: return DRM_FORMAT_ABGR2101010;
   case __DRI_IMAGE_FORMAT_XBGR16161616F: return DRM_FORMAT_XBGR16161616F;
   case __DRI_IMAGE_FORMAT_ABGR16161616F: return DRM_FORMAT_ABGR16161616F;
   case __DRI_IMAGE_FORMAT_ARGB1555: return DRM_FORMAT_ARGB1555;
   case __DRI_IMAGE_FORMAT_ABGR1555: return DRM_FORMAT_ABGR1555;
   case __DRI_IMAGE_FORMAT_ARGB4444: return DRM_FORMAT_ARGB4444;
   case __DRI_IMAGE_FORMAT_ABGR4444: return DRM_FORMAT_ABGR4444;
   }
      }
      #ifdef HAVE_DRI3_MODIFIERS
   static __DRIimage *
   dri3_create_image_from_buffers(xcb_connection_t *c,
                                 {
      __DRIimage                           *ret;
   int                                  *fds;
   uint32_t                             *strides_in, *offsets_in;
   int                                   strides[4], offsets[4];
   unsigned                              error;
            if (bp_reply->nfd > 4)
            fds = xcb_dri3_buffers_from_pixmap_reply_fds(c, bp_reply);
   strides_in = xcb_dri3_buffers_from_pixmap_strides(bp_reply);
   offsets_in = xcb_dri3_buffers_from_pixmap_offsets(bp_reply);
   for (i = 0; i < bp_reply->nfd; i++) {
      strides[i] = strides_in[i];
               ret = image->createImageFromDmaBufs2(opaque_dri_screen(screen),
                                       bp_reply->width,
            for (i = 0; i < bp_reply->nfd; i++)
               }
   #endif
      static __DRIimage *
   dri3_create_image(xcb_connection_t *c,
                     xcb_dri3_buffer_from_pixmap_reply_t *bp_reply,
   unsigned int format,
   {
      int                                  *fds;
   __DRIimage                           *image_planar, *ret;
            /* Get an FD for the pixmap object
   */
            stride = bp_reply->stride;
            /* createImageFromFds creates a wrapper __DRIimage structure which
   * can deal with multiple planes for things like Yuv images. So, once
   * we've gotten the planar wrapper, pull the single plane out of it and
   * discard the wrapper.
   */
   image_planar = image->createImageFromFds(opaque_dri_screen(screen),
                                 close(fds[0]);
   if (!image_planar)
                     if (!ret)
         else
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
      /** kopper_get_pixmap_buffer
   *
   * Get the DRM object for a pixmap from the X server and
   * wrap that with a __DRIimage structure using createImageFromFds
   */
   static struct pipe_resource *
   kopper_get_pixmap_buffer(struct dri_drawable *drawable,
         {
      xcb_drawable_t                       pixmap;
   int                                  width;
   int                                  height;
   int format = get_dri_format(pf);
   struct kopper_loader_info *info = &drawable->info;
   assert(info->bos.sType == VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR);
   VkXcbSurfaceCreateInfoKHR *xcb = (VkXcbSurfaceCreateInfoKHR *)&info->bos;
   xcb_connection_t *conn = xcb->connection;
            if (drawable->image)
            /* FIXME: probably broken for OBS studio?
   * see dri3_get_pixmap_buffer()
   */
         #ifdef HAVE_DRI3_MODIFIERS
      if (drawable->has_modifiers) {
      xcb_dri3_buffers_from_pixmap_cookie_t bps_cookie;
   xcb_dri3_buffers_from_pixmap_reply_t *bps_reply;
            bps_cookie = xcb_dri3_buffers_from_pixmap(conn, pixmap);
   bps_reply = xcb_dri3_buffers_from_pixmap_reply(conn, bps_cookie, &error);
   if (!bps_reply) {
      mesa_loge("kopper: could not create texture from pixmap (%u)", error->error_code);
      }
   drawable->image =
      dri3_create_image_from_buffers(conn, bps_reply, format,
            if (!drawable->image)
         width = bps_reply->width;
   height = bps_reply->height;
         #endif
      {
      xcb_dri3_buffer_from_pixmap_cookie_t bp_cookie;
   xcb_dri3_buffer_from_pixmap_reply_t *bp_reply;
            bp_cookie = xcb_dri3_buffer_from_pixmap(conn, pixmap);
   bp_reply = xcb_dri3_buffer_from_pixmap_reply(conn, bp_cookie, &error);
   if (!bp_reply) {
      mesa_loge("kopper: could not create texture from pixmap (%u)", error->error_code);
               drawable->image = dri3_create_image(conn, bp_reply, format,
               if (!drawable->image)
         width = bp_reply->width;
   height = bp_reply->height;
               drawable->w = width;
               }
   #endif //VK_USE_PLATFORM_XCB_KHR
      static void
   kopper_allocate_textures(struct dri_context *ctx,
                     {
      struct dri_screen *screen = drawable->screen;
   struct pipe_resource templ;
   unsigned width, height;
   bool resized;
   unsigned i;
   struct __DRIimageList images;
            bool is_window = drawable->is_window;
            width  = drawable->w;
            resized = (drawable->old_w != width ||
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            /* First get the buffers from the loader */
   if (image) {
      if (!dri_image_drawable_get_buffers(drawable, &images,
                     if (image) {
      if (images.image_mask & __DRI_IMAGE_BUFFER_FRONT) {
      struct pipe_resource **buf =
                                             if (images.image_mask & __DRI_IMAGE_BUFFER_BACK) {
      struct pipe_resource **buf =
                                             if (images.image_mask & __DRI_IMAGE_BUFFER_SHARED) {
      struct pipe_resource **buf =
                                             } else {
            } else {
      /* remove outdated textures */
   if (resized) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      if (drawable->textures[i] && i < ST_ATTACHMENT_DEPTH_STENCIL && !is_pixmap) {
      drawable->textures[i]->width0 = width;
   drawable->textures[i]->height0 = height;
   /* force all contexts to revalidate framebuffer */
      } else
         pipe_resource_reference(&drawable->msaa_textures[i], NULL);
   if (is_pixmap && i == ST_ATTACHMENT_FRONT_LEFT) {
      FREE(drawable->image);
                        drawable->old_w = width;
            memset(&templ, 0, sizeof(templ));
   templ.target = screen->target;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
         #if 0
   XXX do this once swapinterval is hooked up
      /* pixmaps always have front buffers.
   * Exchange swaps also mandate fake front buffers.
   */
   if (draw->type != LOADER_DRI3_DRAWABLE_WINDOW)
      #endif
         uint32_t attachments = 0;
   for (i = 0; i < statts_count; i++)
                  for (i = 0; i < statts_count; i++) {
      enum pipe_format format;
            dri_drawable_get_format(drawable, statts[i], &format, &bind);
            /* the texture already exists or not requested */
   if (!drawable->textures[statts[i]]) {
      if (statts[i] == ST_ATTACHMENT_BACK_LEFT ||
      statts[i] == ST_ATTACHMENT_DEPTH_STENCIL ||
                              templ.bind = bind;
                  if (statts[i] < ST_ATTACHMENT_DEPTH_STENCIL && is_window) {
      void *data;
   if (statts[i] == ST_ATTACHMENT_BACK_LEFT || (statts[i] == ST_ATTACHMENT_FRONT_LEFT && front_only))
         else
         assert(data);
   drawable->textures[statts[i]] =
   #ifdef VK_USE_PLATFORM_XCB_KHR
            else if (is_pixmap && statts[i] == ST_ATTACHMENT_FRONT_LEFT && !screen->is_sw) {
      drawable->textures[statts[i]] = kopper_get_pixmap_buffer(drawable, format);
   if (drawable->textures[statts[i]])
   #endif
            if (!drawable->textures[statts[i]])
      drawable->textures[statts[i]] =
   }
   if (drawable->stvis.samples > 1 && !drawable->msaa_textures[statts[i]]) {
      templ.bind = bind &
         templ.nr_samples = drawable->stvis.samples;
   templ.nr_storage_samples = drawable->stvis.samples;
                  dri_pipe_blit(ctx->st->pipe,
                  }
      static inline void
   get_drawable_info(struct dri_drawable *drawable, int *x, int *y, int *w, int *h)
   {
               if (loader)
      loader->getDrawableInfo(opaque_dri_drawable(drawable),
         }
      static void
   kopper_update_drawable_info(struct dri_drawable *drawable)
   {
      struct dri_screen *screen = drawable->screen;
   bool is_window = drawable->info.bos.sType != 0;
   int x, y;
   struct pipe_screen *pscreen = screen->unwrapped_screen;
   struct pipe_resource *ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT] ?
                  bool do_kopper_update = is_window && ptex && screen->fd == -1;
   if (drawable->info.bos.sType == VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR && do_kopper_update)
         else
      }
      static inline void
   kopper_present_texture(struct pipe_context *pipe, struct dri_drawable *drawable,
         {
                  }
      static inline void
   kopper_copy_to_front(struct pipe_context *pipe,
               {
                  }
      static bool
   kopper_flush_frontbuffer(struct dri_context *ctx,
               {
               if (!ctx || statt != ST_ATTACHMENT_FRONT_LEFT)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            if (drawable) {
      /* prevent recursion */
   if (drawable->flushing)
                        if (drawable->stvis.samples > 1) {
      /* Resolve the front buffer. */
   dri_pipe_blit(ctx->st->pipe,
            }
            if (ptex) {
      ctx->st->pipe->flush_resource(ctx->st->pipe, drawable->textures[ST_ATTACHMENT_FRONT_LEFT]);
   struct pipe_screen *screen = drawable->screen->base.screen;
   struct st_context *st;
                     st_context_flush(st, ST_FLUSH_FRONT, &new_fence, NULL, NULL);
   if (drawable) {
         }
   /* throttle on the previous fence */
   if (drawable->throttle_fence) {
      screen->fence_finish(screen, NULL, drawable->throttle_fence, OS_TIMEOUT_INFINITE);
      }
   drawable->throttle_fence = new_fence;
                  }
      static inline void
   get_image(struct dri_drawable *drawable, int x, int y, int width, int height, void *data)
   {
               loader->getImage(opaque_dri_drawable(drawable),
            }
      static inline bool
   get_image_shm(struct dri_drawable *drawable, int x, int y, int width, int height,
         {
      const __DRIswrastLoaderExtension *loader = drawable->screen->swrast_loader;
                     if (loader->base.version < 4 || !loader->getImageShm)
            if (!res->screen->resource_get_handle(res->screen, NULL, res, &whandle, PIPE_HANDLE_USAGE_FRAMEBUFFER_WRITE))
            if (loader->base.version > 5 && loader->getImageShm2)
            loader->getImageShm(opaque_dri_drawable(drawable), x, y, width, height, whandle.handle, drawable->loaderPrivate);
      }
      static void
   kopper_update_tex_buffer(struct dri_drawable *drawable,
               {
      struct dri_screen *screen = drawable->screen;
   struct st_context *st_ctx = (struct st_context *)ctx->st;
   struct pipe_context *pipe = st_ctx->pipe;
   struct pipe_transfer *transfer;
   char *map;
   int x, y, w, h;
   int ximage_stride, line;
   if (screen->has_dmabuf || drawable->is_window || drawable->info.bos.sType != VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR)
                  /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
                     map = pipe_texture_map(pipe, res,
                        /* Copy the Drawable content to the mapped texture buffer */
   if (!get_image_shm(drawable, x, y, w, h, res))
            /* The pipe transfer has a pitch rounded up to the nearest 64 pixels.
         ximage_stride = ((w * cpp) + 3) & -4;
   for (line = h-1; line; --line) {
      memmove(&map[line * transfer->stride],
                        }
      static void
   kopper_flush_swapbuffers(struct dri_context *ctx,
         {
         }
      static void
   kopper_swap_buffers(struct dri_drawable *drawable);
      static struct dri_drawable *
   kopper_create_drawable(struct dri_screen *screen, const struct gl_config *visual,
         {
      /* always pass !pixmap because it isn't "handled" or relevant */
   struct dri_drawable *drawable = dri_create_drawable(screen, visual, false,
         if (!drawable)
            // relocate references to the old struct
            // and fill in the vtable
   drawable->allocate_textures = kopper_allocate_textures;
   drawable->update_drawable_info = kopper_update_drawable_info;
   drawable->flush_frontbuffer = kopper_flush_frontbuffer;
   drawable->update_tex_buffer = kopper_update_tex_buffer;
   drawable->flush_swapbuffers = kopper_flush_swapbuffers;
            drawable->info.has_alpha = visual->alphaBits > 0;
   if (screen->kopper_loader->SetSurfaceCreateInfo)
      screen->kopper_loader->SetSurfaceCreateInfo(drawable->loaderPrivate,
                  }
      static int64_t
   kopperSwapBuffers(__DRIdrawable *dPriv, uint32_t flush_flags)
   {
      struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_context *ctx = dri_get_current();
            if (!ctx)
            ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT];
   if (!ptex)
            /* ensure invalidation is applied before renderpass ends */
   if (flush_flags & __DRI2_FLUSH_INVALIDATE_ANCILLARY)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
                     dri_flush(opaque_dri_context(ctx), opaque_dri_drawable(drawable),
                  kopper_copy_to_front(ctx->st->pipe, drawable, ptex);
   if (drawable->is_window && !zink_kopper_check(ptex))
         if (!drawable->textures[ST_ATTACHMENT_FRONT_LEFT]) {
                  /* have to manually swap the pointers here to make frontbuffer readback work */
   drawable->textures[ST_ATTACHMENT_BACK_LEFT] = drawable->textures[ST_ATTACHMENT_FRONT_LEFT];
               }
      static void
   kopper_swap_buffers(struct dri_drawable *drawable)
   {
         }
      static __DRIdrawable *
   kopperCreateNewDrawable(__DRIscreen *psp,
                     {
               struct dri_screen *screen = dri_screen(psp);
   struct dri_drawable *drawable =
         if (drawable)
               }
      static void
   kopperSetSwapInterval(__DRIdrawable *dPriv, int interval)
   {
      struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_screen *screen = drawable->screen;
   struct pipe_screen *pscreen = screen->unwrapped_screen;
   struct pipe_resource *ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT] ?
                  /* the conditional is because we can be called before buffer allocation.  If
   * we're before allocation, then the initial_swap_interval will be used when
   * the swapchain is eventually created.
   */
   if (ptex)
            }
      static int
   kopperQueryBufferAge(__DRIdrawable *dPriv)
   {
      struct dri_drawable *drawable = dri_drawable(dPriv);
   struct dri_context *ctx = dri_get_current();
   struct pipe_resource *ptex = drawable->textures[ST_ATTACHMENT_BACK_LEFT] ?
                  /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
               }
      const __DRIkopperExtension driKopperExtension = {
      .base = { __DRI_KOPPER, 1 },
   .createNewDrawable          = kopperCreateNewDrawable,
   .swapBuffers                = kopperSwapBuffers,
   .setSwapInterval            = kopperSetSwapInterval,
      };
      static const struct __DRImesaCoreExtensionRec mesaCoreExtension = {
      .base = { __DRI_MESA, 1 },
   .version_string = MESA_INTERFACE_VERSION_STRING,
   .createNewScreen = driCreateNewScreen2,
   .createContext = driCreateContextAttribs,
      };
      const __DRIextension *galliumvk_driver_extensions[] = {
      &driCoreExtension.base,
   &mesaCoreExtension.base,
   &driSWRastExtension.base,
   &driDRI2Extension.base,
   &driImageDriverExtension.base,
   &driKopperExtension.base,
   &gallium_config_options.base,
      };
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
