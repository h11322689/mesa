   /**********************************************************
   * Copyright 2009-2011 VMware, Inc. All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   *********************************************************
   * Authors:
   * Thomas Hellstrom <thellstrom-at-vmware-dot-com>
   */
      #include <unistd.h>
   #include "xa_tracker.h"
   #include "xa_priv.h"
   #include "pipe/p_state.h"
   #include "util/format/u_formats.h"
   #include "pipe-loader/pipe_loader.h"
   #include "frontend/drm_driver.h"
   #include "util/u_inlines.h"
      /*
   * format_map [xa_surface_type][first..last in list].
   * Needs to be updated when enum xa_formats is updated.
   */
      static const enum xa_formats preferred_a[] = { xa_format_a8 };
      static const enum xa_formats preferred_argb[] =
      { xa_format_a8r8g8b8, xa_format_x8r8g8b8, xa_format_r5g6b5,
      };
   static const enum xa_formats preferred_z[] =
         static const enum xa_formats preferred_sz[] =
         static const enum xa_formats preferred_zs[] =
         static const enum xa_formats preferred_yuv[] = { xa_format_yuv8 };
      static const enum xa_formats *preferred[] =
      { NULL, preferred_a, preferred_argb, NULL, NULL,
      };
      static const unsigned int num_preferred[] = { 0,
      sizeof(preferred_a) / sizeof(enum xa_formats),
   sizeof(preferred_argb) / sizeof(enum xa_formats),
   0,
   0,
   sizeof(preferred_z) / sizeof(enum xa_formats),
   sizeof(preferred_zs) / sizeof(enum xa_formats),
   sizeof(preferred_sz) / sizeof(enum xa_formats),
      };
      static const unsigned int stype_bind[XA_LAST_SURFACE_TYPE] = { 0,
      PIPE_BIND_SAMPLER_VIEW,
   PIPE_BIND_SAMPLER_VIEW,
   PIPE_BIND_SAMPLER_VIEW,
   PIPE_BIND_SAMPLER_VIEW,
   PIPE_BIND_DEPTH_STENCIL,
   PIPE_BIND_DEPTH_STENCIL,
   PIPE_BIND_DEPTH_STENCIL,
      };
      static struct xa_format_descriptor
   xa_get_pipe_format(struct xa_tracker *xa, enum xa_formats xa_format)
   {
                        switch (xa_format) {
   case xa_format_a8:
      if (xa->screen->is_format_supported(xa->screen, PIPE_FORMAT_R8_UNORM,
                           else
   break;
         fdesc.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_B5G6R5_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_B5G5R5A1_UNORM;
   break;
      case xa_format_a4r4g4b4:
      fdesc.format = PIPE_FORMAT_B4G4R4A4_UNORM;
      case xa_format_a2b10g10r10:
      fdesc.format = PIPE_FORMAT_R10G10B10A2_UNORM;
      case xa_format_x2b10g10r10:
      fdesc.format = PIPE_FORMAT_R10G10B10X2_UNORM;
      case xa_format_b8g8r8a8:
      fdesc.format = PIPE_FORMAT_A8R8G8B8_UNORM;
      case xa_format_b8g8r8x8:
      fdesc.format = PIPE_FORMAT_X8R8G8B8_UNORM;
         fdesc.format = PIPE_FORMAT_Z24X8_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_Z16_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_Z32_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_Z24X8_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_X8Z24_UNORM;
   break;
         fdesc.format = PIPE_FORMAT_Z24_UNORM_S8_UINT;
   break;
         fdesc.format = PIPE_FORMAT_S8_UINT_Z24_UNORM;
   break;
      case xa_format_yuv8:
      if (xa->screen->is_format_supported(xa->screen, PIPE_FORMAT_R8_UNORM,
                     else
   break;
         unreachable("Unexpected format");
   break;
      }
      }
      XA_EXPORT struct xa_tracker *
   xa_tracker_create(int drm_fd)
   {
      struct xa_tracker *xa = calloc(1, sizeof(struct xa_tracker));
   enum xa_surface_type stype;
               return NULL;
            xa->screen = pipe_loader_create_screen(xa->dev);
            goto out_no_screen;
         xa->default_ctx = xa_context_create(xa);
      goto out_no_pipe;
         num_formats = 0;
      num_formats += num_preferred[stype];
         num_formats += 1;
   xa->supported_formats = calloc(num_formats, sizeof(*xa->supported_formats));
      goto out_sf_alloc_fail;
         xa->supported_formats[0] = xa_format_unknown;
   num_formats = 1;
               unsigned int bind = stype_bind[stype];
   enum xa_formats xa_format;
   int i;
      for (i = 0; i < num_preferred[stype]; ++i) {
               struct xa_format_descriptor fdesc =
            if (xa->screen->is_format_supported(xa->screen, fdesc.format,
      if (xa->format_map[stype][0] == 0)
         xa->format_map[stype][1] = num_formats;
   xa->supported_formats[num_formats++] = xa_format;
         }
      }
         out_sf_alloc_fail:
         out_no_pipe:
         out_no_screen:
         pipe_loader_release(&xa->dev, 1);
         free(xa);
      }
      XA_EXPORT void
   xa_tracker_destroy(struct xa_tracker *xa)
   {
      free(xa->supported_formats);
   xa_context_destroy(xa->default_ctx);
   xa->screen->destroy(xa->screen);
   pipe_loader_release(&xa->dev, 1);
   /* CHECK: The XA API user preserves ownership of the original fd */
      }
      static int
   xa_flags_compat(unsigned int old_flags, unsigned int new_flags)
   {
                  return 1;
            return 0;
      /*
   * Don't recreate if we're dropping the render target flag.
   */
      return ((new_flags & XA_FLAG_RENDER_TARGET) == 0);
         /*
   * Don't recreate if we're dropping the scanout flag.
   */
      return ((new_flags & XA_FLAG_SCANOUT) == 0);
         /*
   * Always recreate for unknown / unimplemented flags.
   */
      }
      static struct xa_format_descriptor
   xa_get_format_stype_depth(struct xa_tracker *xa,
         {
      unsigned int i;
   struct xa_format_descriptor fdesc;
               fdesc = xa_get_pipe_format(xa, xa->supported_formats[i]);
   if (fdesc.xa_format != xa_format_unknown &&
      xa_format_depth(fdesc.xa_format) == depth) {
   found = 1;
      }
                  fdesc.xa_format = xa_format_unknown;
            }
      XA_EXPORT int
   xa_format_check_supported(struct xa_tracker *xa,
         {
      struct xa_format_descriptor fdesc = xa_get_pipe_format(xa, xa_format);
               return -XA_ERR_INVAL;
         bind = stype_bind[xa_format_type(fdesc.xa_format)];
      bind |= PIPE_BIND_SHARED;
         bind |= PIPE_BIND_RENDER_TARGET;
         bind |= PIPE_BIND_SCANOUT;
         if (!xa->screen->is_format_supported(xa->screen, fdesc.format,
      return -XA_ERR_INVAL;
            }
      static unsigned
   handle_type(enum xa_handle_type type)
   {
      switch (type) {
      return WINSYS_HANDLE_TYPE_KMS;
      case xa_handle_type_fd:
         case xa_handle_type_shared:
      return WINSYS_HANDLE_TYPE_SHARED;
         }
      static struct xa_surface *
   surface_create(struct xa_tracker *xa,
      int width,
   int height,
   int depth,
   enum xa_surface_type stype,
   enum xa_formats xa_format, unsigned int flags,
      {
      struct pipe_resource *template;
   struct xa_surface *srf;
               fdesc = xa_get_format_stype_depth(xa, stype, depth);
         fdesc = xa_get_pipe_format(xa, xa_format);
            return NULL;
         srf = calloc(1, sizeof(*srf));
      return NULL;
         template = &srf->template;
   template->format = fdesc.format;
   template->target = PIPE_TEXTURE_2D;
   template->width0 = width;
   template->height0 = height;
   template->depth0 = 1;
   template->array_size = 1;
   template->last_level = 0;
               template->bind |= PIPE_BIND_SHARED;
         template->bind |= PIPE_BIND_RENDER_TARGET;
         template->bind |= PIPE_BIND_SCANOUT;
            srf->tex = xa->screen->resource_from_handle(xa->screen, template, whandle,
               srf->tex = xa->screen->resource_create(xa->screen, template);
         goto out_no_tex;
         srf->refcount = 1;
   srf->xa = xa;
   srf->flags = flags;
               out_no_tex:
      free(srf);
      }
         XA_EXPORT struct xa_surface *
   xa_surface_create(struct xa_tracker *xa,
      int width,
   int height,
   int depth,
   enum xa_surface_type stype,
      {
         }
         XA_EXPORT struct xa_surface *
   xa_surface_from_handle(struct xa_tracker *xa,
      int width,
   int height,
   int depth,
   enum xa_surface_type stype,
   enum xa_formats xa_format, unsigned int flags,
      {
      return xa_surface_from_handle2(xa, width, height, depth, stype, xa_format,
            }
      XA_EXPORT struct xa_surface *
   xa_surface_from_handle2(struct xa_tracker *xa,
                           int width,
   int height,
   int depth,
   {
      struct winsys_handle whandle;
   memset(&whandle, 0, sizeof(whandle));
   whandle.type = handle_type(type);
   whandle.handle = handle;
   whandle.stride = stride;
      }
      XA_EXPORT int
   xa_surface_redefine(struct xa_surface *srf,
         int width,
   int height,
   int depth,
   enum xa_surface_type stype,
   enum xa_formats xa_format,
   unsigned int new_flags,
   {
      struct pipe_resource *template = &srf->template;
   struct pipe_resource *texture;
   struct pipe_box src_box;
   struct xa_tracker *xa = srf->xa;
   int save_width;
   int save_height;
   unsigned int save_format;
                  fdesc = xa_get_format_stype_depth(xa, stype, depth);
         fdesc = xa_get_pipe_format(xa, xa_format);
            template->format == fdesc.format &&
   xa_flags_compat(srf->flags, new_flags))
   return XA_ERR_NONE;
         template->bind = stype_bind[xa_format_type(fdesc.xa_format)];
      template->bind |= PIPE_BIND_SHARED;
         template->bind |= PIPE_BIND_RENDER_TARGET;
         template->bind |= PIPE_BIND_SCANOUT;
            if (!xa_format_type_is_color(fdesc.xa_format) ||
      xa_format_type(fdesc.xa_format) == xa_type_a)
         if (!xa->screen->is_format_supported(xa->screen, fdesc.format,
            PIPE_TEXTURE_2D, 0, 0,
   template->bind |
   return -XA_ERR_INVAL;
            save_width = template->width0;
   save_height = template->height0;
            template->width0 = width;
   template->height0 = height;
            texture = xa->screen->resource_create(xa->screen, template);
      template->width0 = save_width;
   template->height0 = save_height;
   template->format = save_format;
   return -XA_ERR_NORES;
                  struct pipe_context *pipe = xa->default_ctx->pipe;
      u_box_origin_2d(xa_min(save_width, template->width0),
         pipe->resource_copy_region(pipe, texture,
         xa_context_flush(xa->default_ctx);
               pipe_resource_reference(&srf->tex, texture);
   pipe_resource_reference(&texture, NULL);
   srf->fdesc = fdesc;
               }
      XA_EXPORT struct xa_surface*
   xa_surface_ref(struct xa_surface *srf)
   {
         return NULL;
      }
   srf->refcount++;
      }
      XA_EXPORT void
   xa_surface_unref(struct xa_surface *srf)
   {
         return;
      }
   pipe_resource_reference(&srf->tex, NULL);
      }
      XA_EXPORT void
   xa_tracker_version(int *major, int *minor, int *patch)
   {
      *major = XA_TRACKER_VERSION_MAJOR;
   *minor = XA_TRACKER_VERSION_MINOR;
      }
      XA_EXPORT int
   xa_surface_handle(struct xa_surface *srf,
      enum xa_handle_type type,
      {
               struct pipe_screen *screen = srf->xa->screen;
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = handle_type(type);
   res = screen->resource_get_handle(screen, srf->xa->default_ctx->pipe,
                  return -XA_ERR_INVAL;
         *handle = whandle.handle;
               }
      XA_EXPORT enum xa_formats
   xa_surface_format(const struct xa_surface *srf)
   {
         }
