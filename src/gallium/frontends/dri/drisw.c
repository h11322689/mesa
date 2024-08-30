   /**************************************************************************
   *
   * Copyright 2009, VMware, Inc.
   * All Rights Reserved.
   * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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
      #include "GL/internal/mesa_interface.h"
   #include "git_sha1.h"
   #include "util/format/u_format.h"
   #include "util/u_memory.h"
   #include "util/u_inlines.h"
   #include "util/u_box.h"
   #include "pipe/p_context.h"
   #include "pipe-loader/pipe_loader.h"
   #include "frontend/drisw_api.h"
   #include "state_tracker/st_context.h"
      #include "dri_screen.h"
   #include "dri_context.h"
   #include "dri_drawable.h"
   #include "dri_helpers.h"
   #include "dri_query_renderer.h"
      DEBUG_GET_ONCE_BOOL_OPTION(swrast_no_present, "SWRAST_NO_PRESENT", false);
      static inline void
   get_drawable_info(struct dri_drawable *drawable, int *x, int *y, int *w, int *h)
   {
               loader->getDrawableInfo(opaque_dri_drawable(drawable),
            }
      static inline void
   put_image(struct dri_drawable *drawable, void *data, unsigned width, unsigned height)
   {
               loader->putImage(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
            }
      static inline void
   put_image2(struct dri_drawable *drawable, void *data, int x, int y,
         {
               loader->putImage2(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
            }
      static inline void
   put_image_shm(struct dri_drawable *drawable, int shmid, char *shmaddr,
               {
               /* if we have the newer interface, don't have to add the offset_x here. */
   if (loader->base.version > 4 && loader->putImageShm2)
   loader->putImageShm2(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
               else
   loader->putImageShm(opaque_dri_drawable(drawable), __DRI_SWRAST_IMAGE_OP_SWAP,
            }
      static inline void
   get_image(struct dri_drawable *drawable, int x, int y, int width, int height, void *data)
   {
               loader->getImage(opaque_dri_drawable(drawable),
            }
      static inline void
   get_image2(struct dri_drawable *drawable, int x, int y, int width, int height, int stride, void *data)
   {
               /* getImage2 support is only in version 3 or newer */
   if (loader->base.version < 3)
            loader->getImage2(opaque_dri_drawable(drawable),
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
   drisw_update_drawable_info(struct dri_drawable *drawable)
   {
                  }
      static void
   drisw_get_image(struct dri_drawable *drawable,
               {
               get_drawable_info(drawable, &draw_x, &draw_y, &draw_w, &draw_h);
      }
      static void
   drisw_put_image(struct dri_drawable *drawable,
         {
         }
      static void
   drisw_put_image2(struct dri_drawable *drawable,
               {
         }
      static inline void
   drisw_put_image_shm(struct dri_drawable *drawable,
                     int shmid, char *shmaddr, unsigned offset,
   {
         }
      static inline void
   drisw_present_texture(struct pipe_context *pipe, struct dri_drawable *drawable,
         {
               if (screen->swrast_no_present)
               }
      static inline void
   drisw_invalidate_drawable(struct dri_drawable *drawable)
   {
                  }
      static inline void
   drisw_copy_to_front(struct pipe_context *pipe,
               {
                  }
      /*
   * Backend functions for pipe_frontend_drawable and swap_buffers.
   */
      static void
   drisw_swap_buffers(struct dri_drawable *drawable)
   {
      struct dri_context *ctx = dri_get_current();
   struct dri_screen *screen = drawable->screen;
            if (!ctx)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
                     if (ptex) {
      struct pipe_fence_handle *fence = NULL;
   if (ctx->pp)
            if (ctx->hud)
                     if (drawable->stvis.samples > 1) {
      /* Resolve the back buffer. */
   dri_pipe_blit(ctx->st->pipe,
                     screen->base.screen->fence_finish(screen->base.screen, ctx->st->pipe,
         screen->base.screen->fence_reference(screen->base.screen, &fence, NULL);
            /* TODO: remove this if the framebuffer state doesn't change. */
         }
      static void
   drisw_copy_sub_buffer(struct dri_drawable *drawable, int x, int y,
         {
      struct dri_context *ctx = dri_get_current();
   struct dri_screen *screen = drawable->screen;
   struct pipe_resource *ptex;
   struct pipe_box box;
   if (!ctx)
                     if (ptex) {
      /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            struct pipe_fence_handle *fence = NULL;
   if (ctx->pp && drawable->textures[ST_ATTACHMENT_DEPTH_STENCIL])
                     screen->base.screen->fence_finish(screen->base.screen, ctx->st->pipe,
                  if (drawable->stvis.samples > 1) {
      /* Resolve the back buffer. */
   dri_pipe_blit(ctx->st->pipe,
                     u_box_2d(x, drawable->h - y - h, w, h, &box);
         }
      static bool
   drisw_flush_frontbuffer(struct dri_context *ctx,
               {
               if (!ctx || statt != ST_ATTACHMENT_FRONT_LEFT)
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            if (drawable->stvis.samples > 1) {
      /* Resolve the front buffer. */
   dri_pipe_blit(ctx->st->pipe,
            }
            if (ptex) {
                     }
      /**
   * Allocate framebuffer attachments.
   *
   * During fixed-size operation, the function keeps allocating new attachments
   * as they are requested. Unused attachments are not removed, not until the
   * framebuffer is resized or destroyed.
   */
   static void
   drisw_allocate_textures(struct dri_context *stctx,
                     {
      struct dri_screen *screen = drawable->screen;
   const __DRIswrastLoaderExtension *loader = drawable->screen->swrast_loader;
   struct pipe_resource templ;
   unsigned width, height;
   bool resized;
            /* Wait for glthread to finish because we can't use pipe_context from
   * multiple threads.
   */
            width  = drawable->w;
            resized = (drawable->old_w != width ||
            /* remove outdated textures */
   if (resized) {
      for (i = 0; i < ST_ATTACHMENT_COUNT; i++) {
      pipe_resource_reference(&drawable->textures[i], NULL);
                  memset(&templ, 0, sizeof(templ));
   templ.target = screen->target;
   templ.width0 = width;
   templ.height0 = height;
   templ.depth0 = 1;
   templ.array_size = 1;
            for (i = 0; i < count; i++) {
      enum pipe_format format;
            /* the texture already exists or not requested */
   if (drawable->textures[statts[i]])
                     /* if we don't do any present, no need for display targets */
   if (statts[i] != ST_ATTACHMENT_DEPTH_STENCIL && !screen->swrast_no_present)
            if (format == PIPE_FORMAT_NONE)
            templ.format = format;
   templ.bind = bind;
   templ.nr_samples = 0;
            if (statts[i] == ST_ATTACHMENT_FRONT_LEFT &&
            screen->base.screen->resource_create_front &&
   drawable->textures[statts[i]] =
      } else
                  if (drawable->stvis.samples > 1) {
      templ.bind = templ.bind &
         templ.nr_samples = drawable->stvis.samples;
   templ.nr_storage_samples = drawable->stvis.samples;
                  dri_pipe_blit(stctx->st->pipe,
                        drawable->old_w = width;
      }
      static void
   drisw_update_tex_buffer(struct dri_drawable *drawable,
               {
      struct st_context *st_ctx = (struct st_context *)ctx->st;
   struct pipe_context *pipe = st_ctx->pipe;
   struct pipe_transfer *transfer;
   char *map;
   int x, y, w, h;
   int ximage_stride, line;
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
      static __DRIimageExtension driSWImageExtension = {
               .createImageFromRenderbuffer  = dri2_create_image_from_renderbuffer,
   .createImageFromTexture = dri2_create_from_texture,
      };
      static const __DRIrobustnessExtension dri2Robustness = {
         };
      /*
   * Backend function for init_screen.
   */
      static const __DRIextension *drisw_screen_extensions[] = {
      &driTexBufferExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2ConfigQueryExtension.base,
   &dri2FenceExtension.base,
   &driSWImageExtension.base,
   &dri2FlushControlExtension.base,
      };
      static const __DRIextension *drisw_robust_screen_extensions[] = {
      &driTexBufferExtension.base,
   &dri2RendererQueryExtension.base,
   &dri2ConfigQueryExtension.base,
   &dri2FenceExtension.base,
   &dri2Robustness.base,
   &driSWImageExtension.base,
   &dri2FlushControlExtension.base,
      };
      static const struct drisw_loader_funcs drisw_lf = {
      .get_image = drisw_get_image,
   .put_image = drisw_put_image,
      };
      static const struct drisw_loader_funcs drisw_shm_lf = {
      .get_image = drisw_get_image,
   .put_image = drisw_put_image,
   .put_image2 = drisw_put_image2,
      };
      static struct dri_drawable *
   drisw_create_drawable(struct dri_screen *screen, const struct gl_config * visual,
         {
      struct dri_drawable *drawable = dri_create_drawable(screen, visual, isPixmap,
         if (!drawable)
            drawable->allocate_textures = drisw_allocate_textures;
   drawable->update_drawable_info = drisw_update_drawable_info;
   drawable->flush_frontbuffer = drisw_flush_frontbuffer;
   drawable->update_tex_buffer = drisw_update_tex_buffer;
               }
      static const __DRIconfig **
   drisw_init_screen(struct dri_screen *screen)
   {
      const __DRIswrastLoaderExtension *loader = screen->swrast_loader;
   const __DRIconfig **configs;
   struct pipe_screen *pscreen = NULL;
                     if (loader->base.version >= 4) {
      if (loader->putImageShm)
                  #ifdef HAVE_DRISW_KMS
      if (screen->fd != -1)
      #endif
      if (!success)
            if (success)
            if (!pscreen)
            dri_init_options(screen);
   configs = dri_init_screen(screen, pscreen);
   if (!configs)
            if (pscreen->get_param(pscreen, PIPE_CAP_DEVICE_RESET_STATUS_QUERY)) {
      screen->extensions = drisw_robust_screen_extensions;
      }
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
      /* swrast copy sub buffer entrypoint. */
   static void driswCopySubBuffer(__DRIdrawable *pdp, int x, int y,
         {
                           }
      /* for swrast only */
   const __DRIcopySubBufferExtension driSWCopySubBufferExtension = {
                  };
      static const struct __DRImesaCoreExtensionRec mesaCoreExtension = {
      .base = { __DRI_MESA, 1 },
   .version_string = MESA_INTERFACE_VERSION_STRING,
   .createNewScreen = driCreateNewScreen2,
   .createContext = driCreateContextAttribs,
      };
      /* This is the table of extensions that the loader will dlsym() for. */
   const __DRIextension *galliumsw_driver_extensions[] = {
      &driCoreExtension.base,
   &mesaCoreExtension.base,
   &driSWRastExtension.base,
   &driSWCopySubBufferExtension.base,
   &gallium_config_options.base,
      };
      /* vim: set sw=3 ts=8 sts=3 expandtab: */
