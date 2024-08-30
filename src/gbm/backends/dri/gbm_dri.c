   /*
   * Copyright © 2011 Intel Corporation
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
   * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   * Authors:
   *    Benjamin Franzke <benjaminfranzke@googlemail.com>
   */
      #include <stdio.h>
   #include <stdlib.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <stdbool.h>
   #include <string.h>
   #include <errno.h>
   #include <limits.h>
   #include <assert.h>
      #include <sys/types.h>
   #include <unistd.h>
   #include <dlfcn.h>
   #include <xf86drm.h>
   #include "drm-uapi/drm_fourcc.h"
      #include <GL/gl.h> /* dri_interface needs GL types */
   #include <GL/internal/dri_interface.h>
      #include "gbm_driint.h"
      #include "gbmint.h"
   #include "loader_dri_helper.h"
   #include "kopper_interface.h"
   #include "loader.h"
   #include "util/u_debug.h"
   #include "util/macros.h"
      /* For importing wl_buffer */
   #if HAVE_WAYLAND_PLATFORM
   #include "wayland-drm.h"
   #endif
      static __DRIimage *
   dri_lookup_egl_image(__DRIscreen *screen, void *image, void *data)
   {
               if (dri->lookup_image == NULL)
               }
      static GLboolean
   dri_validate_egl_image(void *image, void *data)
   {
               if (dri->validate_image == NULL)
               }
      static __DRIimage *
   dri_lookup_egl_image_validated(void *image, void *data)
   {
               if (dri->lookup_image_validated == NULL)
               }
      static void
   dri_flush_front_buffer(__DRIdrawable * driDrawable, void *data)
   {
      struct gbm_dri_surface *surf = data;
            if (dri->flush_front_buffer != NULL)
      }
      static unsigned
   dri_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
   {
      /* Note: loaderPrivate is _EGLDisplay* */
   switch (cap) {
   case DRI_LOADER_CAP_FP16:
         default:
            }
      static int
   image_get_buffers(__DRIdrawable *driDrawable,
                     unsigned int format,
   uint32_t *stamp,
   {
      struct gbm_dri_surface *surf = loaderPrivate;
            if (dri->image_get_buffers == NULL)
            return dri->image_get_buffers(driDrawable, format, stamp,
      }
      static void
   swrast_get_drawable_info(__DRIdrawable *driDrawable,
                           int           *x,
   {
               *x = 0;
   *y = 0;
   *width = surf->base.v0.width;
      }
      static void
   swrast_put_image2(__DRIdrawable *driDrawable,
                     int            op,
   int            x,
   int            y,
   int            width,
   int            height,
   {
      struct gbm_dri_surface *surf = loaderPrivate;
            dri->swrast_put_image2(driDrawable,
                  }
      static void
   swrast_put_image(__DRIdrawable *driDrawable,
                  int            op,
   int            x,
   int            y,
   int            width,
      {
      swrast_put_image2(driDrawable, op, x, y, width, height,
      }
      static void
   swrast_get_image(__DRIdrawable *driDrawable,
                  int            x,
   int            y,
   int            width,
      {
      struct gbm_dri_surface *surf = loaderPrivate;
            dri->swrast_get_image(driDrawable,
                  }
      static const __DRIuseInvalidateExtension use_invalidate = {
         };
      static const __DRIimageLookupExtension image_lookup_extension = {
               .lookupEGLImage          = dri_lookup_egl_image,
   .validateEGLImage        = dri_validate_egl_image,
      };
      static const __DRIimageLoaderExtension image_loader_extension = {
               .getBuffers          = image_get_buffers,
   .flushFrontBuffer    = dri_flush_front_buffer,
      };
      static const __DRIswrastLoaderExtension swrast_loader_extension = {
               .getDrawableInfo = swrast_get_drawable_info,
   .putImage        = swrast_put_image,
   .getImage        = swrast_get_image,
      };
      static const __DRIkopperLoaderExtension kopper_loader_extension = {
                  };
      static const __DRIextension *gbm_dri_screen_extensions[] = {
      &image_lookup_extension.base,
   &use_invalidate.base,
   &image_loader_extension.base,
   &swrast_loader_extension.base,
   &kopper_loader_extension.base,
      };
      static struct dri_extension_match dri_core_extensions[] = {
      { __DRI2_FLUSH, 1, offsetof(struct gbm_dri_device, flush), false },
      };
      static struct dri_extension_match gbm_dri_device_extensions[] = {
      { __DRI_CORE, 1, offsetof(struct gbm_dri_device, core), false },
   { __DRI_MESA, 1, offsetof(struct gbm_dri_device, mesa), false },
      };
      static struct dri_extension_match gbm_swrast_device_extensions[] = {
      { __DRI_CORE, 1, offsetof(struct gbm_dri_device, core), false },
   { __DRI_MESA, 1, offsetof(struct gbm_dri_device, mesa), false },
   { __DRI_SWRAST, 4, offsetof(struct gbm_dri_device, swrast), false },
      };
      static const __DRIextension **
   dri_open_driver(struct gbm_dri_device *dri)
   {
      /* Temporarily work around dri driver libs that need symbols in libglapi
   * but don't automatically link it in.
   */
   /* XXX: Library name differs on per platforms basis. Update this as
   * osx/cygwin/windows/bsd gets support for GBM..
   */
            static const char *search_path_vars[] = {
      /* Read GBM_DRIVERS_PATH first for compatibility, but LIBGL_DRIVERS_PATH
   * is recommended over GBM_DRIVERS_PATH.
   */
   "GBM_DRIVERS_PATH",
   /* Read LIBGL_DRIVERS_PATH if GBM_DRIVERS_PATH was not set.
   * LIBGL_DRIVERS_PATH is recommended over GBM_DRIVERS_PATH.
   */
   "LIBGL_DRIVERS_PATH",
      };
      }
      static int
   dri_screen_create_for_driver(struct gbm_dri_device *dri, char *driver_name)
   {
                        const __DRIextension **extensions = dri_open_driver(dri);
   if (!extensions)
            bool bind_ok;
   if (!swrast) {
      bind_ok = loader_bind_extensions(dri, gbm_dri_device_extensions,
            } else {
      bind_ok = loader_bind_extensions(dri, gbm_swrast_device_extensions,
                     if (!bind_ok) {
      fprintf(stderr, "failed to bind extensions\n");
               dri->driver_extensions = extensions;
   dri->loader_extensions = gbm_dri_screen_extensions;
   dri->screen = dri->mesa->createNewScreen(0, swrast ? -1 : dri->base.v0.fd,
                     if (dri->screen == NULL)
            if (!swrast) {
      extensions = dri->core->getExtensions(dri->screen);
   if (!loader_bind_extensions(dri, dri_core_extensions,
                                 dri->lookup_image = NULL;
                  free_screen:
            close_driver:
            fail:
      free(dri->driver_name);
      }
      static int
   dri_screen_create(struct gbm_dri_device *dri)
   {
               driver_name = loader_get_driver_for_fd(dri->base.v0.fd);
   if (!driver_name)
               }
      static int
   dri_screen_create_sw(struct gbm_dri_device *dri)
   {
      char *driver_name;
            driver_name = strdup("kms_swrast");
   if (!driver_name)
            ret = dri_screen_create_for_driver(dri, driver_name);
   if (ret != 0)
         if (ret != 0)
            dri->software = true;
      }
      static const struct gbm_dri_visual gbm_dri_visuals_table[] = {
      {
   GBM_FORMAT_R8, __DRI_IMAGE_FORMAT_R8,
   { 0, -1, -1, -1 },
   { 8, 0, 0, 0 },
   },
   {
   GBM_FORMAT_R16, __DRI_IMAGE_FORMAT_R16,
   { 0, -1, -1, -1 },
   { 16, 0, 0, 0 },
   },
   {
   GBM_FORMAT_GR88, __DRI_IMAGE_FORMAT_GR88,
   { 0, 8, -1, -1 },
   { 8, 8, 0, 0 },
   },
   {
   GBM_FORMAT_GR1616, __DRI_IMAGE_FORMAT_GR1616,
   { 0, 16, -1, -1 },
   { 16, 16, 0, 0 },
   },
   {
   GBM_FORMAT_ARGB1555, __DRI_IMAGE_FORMAT_ARGB1555,
   { 10, 5, 0, 11 },
   { 5, 5, 5, 1 },
   },
   {
   GBM_FORMAT_RGB565, __DRI_IMAGE_FORMAT_RGB565,
   { 11, 5, 0, -1 },
   { 5, 6, 5, 0 },
   },
   {
   GBM_FORMAT_XRGB8888, __DRI_IMAGE_FORMAT_XRGB8888,
   { 16, 8, 0, -1 },
   { 8, 8, 8, 0 },
   },
   {
   GBM_FORMAT_ARGB8888, __DRI_IMAGE_FORMAT_ARGB8888,
   { 16, 8, 0, 24 },
   { 8, 8, 8, 8 },
   },
   {
   GBM_FORMAT_XBGR8888, __DRI_IMAGE_FORMAT_XBGR8888,
   { 0, 8, 16, -1 },
   { 8, 8, 8, 0 },
   },
   {
   GBM_FORMAT_ABGR8888, __DRI_IMAGE_FORMAT_ABGR8888,
   { 0, 8, 16, 24 },
   { 8, 8, 8, 8 },
   },
   {
   GBM_FORMAT_XRGB2101010, __DRI_IMAGE_FORMAT_XRGB2101010,
   { 20, 10, 0, -1 },
   { 10, 10, 10, 0 },
   },
   {
   GBM_FORMAT_ARGB2101010, __DRI_IMAGE_FORMAT_ARGB2101010,
   { 20, 10, 0, 30 },
   { 10, 10, 10, 2 },
   },
   {
   GBM_FORMAT_XBGR2101010, __DRI_IMAGE_FORMAT_XBGR2101010,
   { 0, 10, 20, -1 },
   { 10, 10, 10, 0 },
   },
   {
   GBM_FORMAT_ABGR2101010, __DRI_IMAGE_FORMAT_ABGR2101010,
   { 0, 10, 20, 30 },
   { 10, 10, 10, 2 },
   },
   {
   GBM_FORMAT_XBGR16161616, __DRI_IMAGE_FORMAT_XBGR16161616,
   { 0, 16, 32, -1 },
   { 16, 16, 16, 0 },
   },
   {
   GBM_FORMAT_ABGR16161616, __DRI_IMAGE_FORMAT_ABGR16161616,
   { 0, 16, 32, 48 },
   { 16, 16, 16, 16 },
   },
   {
   GBM_FORMAT_XBGR16161616F, __DRI_IMAGE_FORMAT_XBGR16161616F,
   { 0, 16, 32, -1 },
   { 16, 16, 16, 0 },
   true,
   },
   {
   GBM_FORMAT_ABGR16161616F, __DRI_IMAGE_FORMAT_ABGR16161616F,
   { 0, 16, 32, 48 },
   { 16, 16, 16, 16 },
   true,
      };
      static int
   gbm_format_to_dri_format(uint32_t gbm_format)
   {
      gbm_format = gbm_core.v0.format_canonicalize(gbm_format);
   for (size_t i = 0; i < ARRAY_SIZE(gbm_dri_visuals_table); i++) {
      if (gbm_dri_visuals_table[i].gbm_format == gbm_format)
                  }
      static uint32_t
   gbm_dri_to_gbm_format(int dri_format)
   {
      for (size_t i = 0; i < ARRAY_SIZE(gbm_dri_visuals_table); i++) {
      if (gbm_dri_visuals_table[i].dri_image_format == dri_format)
                  }
      static int
   gbm_dri_is_format_supported(struct gbm_device *gbm,
               {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
            if ((usage & GBM_BO_USE_CURSOR) && (usage & GBM_BO_USE_RENDERING))
            format = gbm_core.v0.format_canonicalize(format);
   if (gbm_format_to_dri_format(format) == 0)
            /* If there is no query, fall back to the small table which was originally
   * here. */
   if (!dri->image->queryDmaBufModifiers) {
      switch (format) {
   case GBM_FORMAT_XRGB8888:
   case GBM_FORMAT_ARGB8888:
   case GBM_FORMAT_XBGR8888:
         default:
                     /* This returns false if the format isn't supported */
   if (!dri->image->queryDmaBufModifiers(dri->screen, format, 0, NULL, NULL,
                     }
      static int
   gbm_dri_get_format_modifier_plane_count(struct gbm_device *gbm,
               {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
            if (!dri->image->queryDmaBufFormatModifierAttribs)
            format = gbm_core.v0.format_canonicalize(format);
   if (gbm_format_to_dri_format(format) == 0)
            if (!dri->image->queryDmaBufFormatModifierAttribs(
         dri->screen, format, modifier,
               }
      static int
   gbm_dri_bo_write(struct gbm_bo *_bo, const void *buf, size_t count)
   {
               if (bo->image != NULL) {
      errno = EINVAL;
                           }
      static int
   gbm_dri_bo_get_fd(struct gbm_bo *_bo)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
            if (bo->image == NULL)
            if (!dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_FD, &fd))
               }
      static int
   get_number_planes(struct gbm_dri_device *dri, __DRIimage *image)
   {
               /* Dumb buffers are single-plane only. */
   if (!image)
                     if (num_planes <= 0)
               }
      static int
   gbm_dri_bo_get_planes(struct gbm_bo *_bo)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
               }
      static union gbm_bo_handle
   gbm_dri_bo_get_handle_for_plane(struct gbm_bo *_bo, int plane)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
   union gbm_bo_handle ret;
            if (plane >= get_number_planes(dri, bo->image)) {
      errno = EINVAL;
               /* dumb BOs can only utilize non-planar formats */
   if (!bo->image) {
      assert(plane == 0);
   ret.s32 = bo->handle;
               __DRIimage *image = dri->image->fromPlanar(bo->image, plane, NULL);
   if (image) {
      dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_HANDLE, &ret.s32);
      } else {
      assert(plane == 0);
                  }
      static int
   gbm_dri_bo_get_plane_fd(struct gbm_bo *_bo, int plane)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
            if (!dri->image || !dri->image->fromPlanar) {
      /* Preserve legacy behavior if plane is 0 */
   if (plane == 0)
            errno = ENOSYS;
               /* dumb BOs can only utilize non-planar formats */
   if (!bo->image) {
      errno = EINVAL;
               if (plane >= get_number_planes(dri, bo->image)) {
      errno = EINVAL;
               __DRIimage *image = dri->image->fromPlanar(bo->image, plane, NULL);
   if (image) {
      dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_FD, &fd);
      } else {
      assert(plane == 0);
                  }
      static uint32_t
   gbm_dri_bo_get_stride(struct gbm_bo *_bo, int plane)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
   __DRIimage *image;
            if (!dri->image || !dri->image->fromPlanar) {
      /* Preserve legacy behavior if plane is 0 */
   if (plane == 0)
            errno = ENOSYS;
               if (plane >= get_number_planes(dri, bo->image)) {
      errno = EINVAL;
               if (bo->image == NULL) {
      assert(plane == 0);
               image = dri->image->fromPlanar(bo->image, plane, NULL);
   if (image) {
      dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_STRIDE, &stride);
      } else {
      assert(plane == 0);
                  }
      static uint32_t
   gbm_dri_bo_get_offset(struct gbm_bo *_bo, int plane)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
            if (plane >= get_number_planes(dri, bo->image))
            /* Dumb images have no offset */
   if (bo->image == NULL) {
      assert(plane == 0);
               __DRIimage *image = dri->image->fromPlanar(bo->image, plane, NULL);
   if (image) {
      dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_OFFSET, &offset);
      } else {
      assert(plane == 0);
                  }
      static uint64_t
   gbm_dri_bo_get_modifier(struct gbm_bo *_bo)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
            /* Dumb buffers have no modifiers */
   if (!bo->image)
            uint64_t ret = 0;
   int mod;
   if (!dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_MODIFIER_UPPER,
                           if (!dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_MODIFIER_LOWER,
                              }
      static void
   gbm_dri_bo_destroy(struct gbm_bo *_bo)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
   struct gbm_dri_bo *bo = gbm_dri_bo(_bo);
            if (bo->image != NULL) {
         } else {
      gbm_dri_bo_unmap_dumb(bo);
   memset(&arg, 0, sizeof(arg));
   arg.handle = bo->handle;
                  }
      static struct gbm_bo *
   gbm_dri_bo_import(struct gbm_device *gbm,
         {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct gbm_dri_bo *bo;
   __DRIimage *image;
   unsigned dri_use = 0;
            if (dri->image == NULL) {
      errno = ENOSYS;
                  #if HAVE_WAYLAND_PLATFORM
      case GBM_BO_IMPORT_WL_BUFFER:
   {
               if (!dri->wl_drm) {
      errno = EINVAL;
               wb = wayland_drm_buffer_get(dri->wl_drm, (struct wl_resource *) buffer);
   if (!wb) {
      errno = EINVAL;
                        /* GBM_FORMAT_* is identical to WL_DRM_FORMAT_*, so no conversion
   * required. */
   gbm_format = wb->format;
         #endif
         case GBM_BO_IMPORT_EGL_IMAGE:
   {
      int dri_format;
   if (dri->lookup_image == NULL) {
      errno = EINVAL;
               image = dri->lookup_image(dri->screen, buffer, dri->lookup_user_data);
   image = dri->image->dupImage(image, NULL);
   dri->image->queryImage(image, __DRI_IMAGE_ATTRIB_FORMAT, &dri_format);
   gbm_format = gbm_dri_to_gbm_format(dri_format);
   if (gbm_format == 0) {
      errno = EINVAL;
   dri->image->destroyImage(image);
      }
               case GBM_BO_IMPORT_FD:
   {
      struct gbm_import_fd_data *fd_data = buffer;
   int stride = fd_data->stride, offset = 0;
            /* GBM's GBM_FORMAT_* tokens are a strict superset of the DRI FourCC
   * tokens accepted by createImageFromFds, except for not supporting
   * the sARGB format. */
            image = dri->image->createImageFromFds(dri->screen,
                                       if (image == NULL) {
      errno = EINVAL;
      }
   gbm_format = fd_data->format;
               case GBM_BO_IMPORT_FD_MODIFIER:
   {
      struct gbm_import_fd_modifier_data *fd_data = buffer;
   unsigned int error;
            /* Import with modifier requires createImageFromDmaBufs2 */
   if (dri->image->createImageFromDmaBufs2 == NULL) {
      errno = ENOSYS;
               /* GBM's GBM_FORMAT_* tokens are a strict superset of the DRI FourCC
   * tokens accepted by createImageFromDmaBufs2, except for not supporting
   * the sARGB format. */
            image = dri->image->createImageFromDmaBufs2(dri->screen, fd_data->width,
                                             fd_data->height, fourcc,
   if (image == NULL) {
      errno = ENOSYS;
               gbm_format = fourcc;
               default:
      errno = ENOSYS;
                  bo = calloc(1, sizeof *bo);
   if (bo == NULL) {
      dri->image->destroyImage(image);
                        if (usage & GBM_BO_USE_SCANOUT)
         if (usage & GBM_BO_USE_CURSOR)
         if (!dri->image->validateUsage(bo->image, dri_use)) {
      errno = EINVAL;
   dri->image->destroyImage(bo->image);
   free(bo);
               bo->base.gbm = gbm;
            dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_WIDTH,
         dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HEIGHT,
         dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE,
         dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HANDLE,
               }
      static struct gbm_bo *
   create_dumb(struct gbm_device *gbm,
               {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct drm_mode_create_dumb create_arg;
   struct gbm_dri_bo *bo;
   struct drm_mode_destroy_dumb destroy_arg;
   int ret;
            is_cursor = (usage & GBM_BO_USE_CURSOR) != 0 &&
         is_scanout = (usage & GBM_BO_USE_SCANOUT) != 0 &&
         if (!is_cursor && !is_scanout) {
      errno = EINVAL;
               bo = calloc(1, sizeof *bo);
   if (bo == NULL)
            memset(&create_arg, 0, sizeof(create_arg));
   create_arg.bpp = 32;
   create_arg.width = width;
            ret = drmIoctl(dri->base.v0.fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg);
   if (ret)
            bo->base.gbm = gbm;
   bo->base.v0.width = width;
   bo->base.v0.height = height;
   bo->base.v0.stride = create_arg.pitch;
   bo->base.v0.format = format;
   bo->base.v0.handle.u32 = create_arg.handle;
   bo->handle = create_arg.handle;
            if (gbm_dri_bo_map_dumb(bo) == NULL)
                  destroy_dumb:
      memset(&destroy_arg, 0, sizeof destroy_arg);
   destroy_arg.handle = create_arg.handle;
      free_bo:
                  }
      static struct gbm_bo *
   gbm_dri_bo_create(struct gbm_device *gbm,
                     uint32_t width, uint32_t height,
   {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
   struct gbm_dri_bo *bo;
   int dri_format;
                     if (usage & GBM_BO_USE_WRITE || dri->image == NULL)
            bo = calloc(1, sizeof *bo);
   if (bo == NULL)
            bo->base.gbm = gbm;
   bo->base.v0.width = width;
   bo->base.v0.height = height;
            dri_format = gbm_format_to_dri_format(format);
   if (dri_format == 0) {
      errno = EINVAL;
               if (usage & GBM_BO_USE_SCANOUT)
         if (usage & GBM_BO_USE_CURSOR)
         if (usage & GBM_BO_USE_LINEAR)
         if (usage & GBM_BO_USE_PROTECTED)
         if (usage & GBM_BO_USE_FRONT_RENDERING)
            /* Gallium drivers requires shared in order to get the handle/stride */
            if (modifiers && !dri->image->createImageWithModifiers) {
      errno = ENOSYS;
               bo->image = loader_dri_create_image(dri->screen, dri->image, width, height,
               if (bo->image == NULL)
            if (modifiers)
            dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_HANDLE,
         dri->image->queryImage(bo->image, __DRI_IMAGE_ATTRIB_STRIDE,
                  failed:
      free(bo);
      }
      static void *
   gbm_dri_bo_map(struct gbm_bo *_bo,
               uint32_t x, uint32_t y,
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
            /* If it's a dumb buffer, we already have a mapping */
   if (bo->map) {
      *map_data = (char *)bo->map + (bo->base.v0.stride * y) + (x * 4);
   *stride = bo->base.v0.stride;
               mtx_lock(&dri->mutex);
   if (!dri->context) {
               dri->context =
      dri->image_driver->createContextAttribs(dri->screen,
               }
   assert(dri->context);
            /* GBM flags and DRI flags are the same, so just pass them on */
   return dri->image->mapImage(dri->context, bo->image, x, y,
            }
      static void
   gbm_dri_bo_unmap(struct gbm_bo *_bo, void *map_data)
   {
      struct gbm_dri_device *dri = gbm_dri_device(_bo->gbm);
            /* Check if it's a dumb buffer and check the pointer is in range */
   if (bo->map) {
      assert(map_data >= bo->map);
   assert(map_data < (bo->map + bo->size));
               if (!dri->context || !dri->image->unmapImage)
                     /*
   * Not all DRI drivers use direct maps. They may queue up DMA operations
   * on the mapping context. Since there is no explicit gbm flush
   * mechanism, we need to flush here.
   */
      }
         static struct gbm_surface *
   gbm_dri_surface_create(struct gbm_device *gbm,
                     {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
            if (modifiers && !dri->image->createImageWithModifiers) {
      errno = ENOSYS;
               if (count)
            /* It's acceptable to create an image with INVALID modifier in the list,
   * but it cannot be on the only modifier (since it will certainly fail
   * later). While we could easily catch this after modifier creation, doing
   * the check here is a convenient debug check likely pointing at whatever
   * interface the client is using to build its modifier list.
   */
   if (count == 1 && modifiers[0] == DRM_FORMAT_MOD_INVALID) {
      fprintf(stderr, "Only invalid modifier specified\n");
               surf = calloc(1, sizeof *surf);
   if (surf == NULL) {
      errno = ENOMEM;
               surf->base.gbm = gbm;
   surf->base.v0.width = width;
   surf->base.v0.height = height;
   surf->base.v0.format = gbm_core.v0.format_canonicalize(format);
   surf->base.v0.flags = flags;
   if (!modifiers) {
      assert(!count);
               surf->base.v0.modifiers = calloc(count, sizeof(*modifiers));
   if (count && !surf->base.v0.modifiers) {
      errno = ENOMEM;
   free(surf);
               /* TODO: We are deferring validation of modifiers until the image is actually
   * created. This deferred creation can fail due to a modifier-format
   * mismatch. The result is the client has a surface but no object to back it.
   */
   surf->base.v0.count = count;
               }
      static void
   gbm_dri_surface_destroy(struct gbm_surface *_surf)
   {
               free(surf->base.v0.modifiers);
      }
      static void
   dri_destroy(struct gbm_device *gbm)
   {
      struct gbm_dri_device *dri = gbm_dri_device(gbm);
            if (dri->context)
            dri->core->destroyScreen(dri->screen);
   for (i = 0; dri->driver_configs[i]; i++)
         free(dri->driver_configs);
   dlclose(dri->driver);
               }
      static struct gbm_device *
   dri_device_create(int fd, uint32_t gbm_backend_version)
   {
      struct gbm_dri_device *dri;
   int ret;
            /*
   * Since the DRI backend is built-in to the loader, the loader ABI version is
   * guaranteed to match this backend's ABI version
   */
   assert(gbm_core.v0.core_version == GBM_BACKEND_ABI_VERSION);
            dri = calloc(1, sizeof *dri);
   if (!dri)
            dri->base.v0.fd = fd;
   dri->base.v0.backend_version = gbm_backend_version;
   dri->base.v0.bo_create = gbm_dri_bo_create;
   dri->base.v0.bo_import = gbm_dri_bo_import;
   dri->base.v0.bo_map = gbm_dri_bo_map;
   dri->base.v0.bo_unmap = gbm_dri_bo_unmap;
   dri->base.v0.is_format_supported = gbm_dri_is_format_supported;
   dri->base.v0.get_format_modifier_plane_count =
         dri->base.v0.bo_write = gbm_dri_bo_write;
   dri->base.v0.bo_get_fd = gbm_dri_bo_get_fd;
   dri->base.v0.bo_get_planes = gbm_dri_bo_get_planes;
   dri->base.v0.bo_get_handle = gbm_dri_bo_get_handle_for_plane;
   dri->base.v0.bo_get_plane_fd = gbm_dri_bo_get_plane_fd;
   dri->base.v0.bo_get_stride = gbm_dri_bo_get_stride;
   dri->base.v0.bo_get_offset = gbm_dri_bo_get_offset;
   dri->base.v0.bo_get_modifier = gbm_dri_bo_get_modifier;
   dri->base.v0.bo_destroy = gbm_dri_bo_destroy;
   dri->base.v0.destroy = dri_destroy;
   dri->base.v0.surface_create = gbm_dri_surface_create;
                     dri->visual_table = gbm_dri_visuals_table;
                     force_sw = debug_get_bool_option("GBM_ALWAYS_SOFTWARE", false);
   if (!force_sw) {
      ret = dri_screen_create(dri);
   if (ret)
      } else {
                  if (ret)
                  err_dri:
                  }
      struct gbm_backend gbm_dri_backend = {
      .v0.backend_version = GBM_BACKEND_ABI_VERSION,
   .v0.backend_name = "dri",
      };
