   /*
   * Copyright © 2010 Intel Corporation
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
   *    Kristian Høgsberg <krh@bitplanet.net>
   */
      #include <dlfcn.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <limits.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
   #include <time.h>
   #include <unistd.h>
   #include <c11/threads.h>
   #ifdef HAVE_LIBDRM
   #include <xf86drm.h>
   #include "drm-uapi/drm_fourcc.h"
   #endif
   #include <GL/gl.h>
   #include <GL/internal/dri_interface.h>
   #include <sys/stat.h>
   #include <sys/types.h>
   #include "dri_screen.h"
      #ifdef HAVE_WAYLAND_PLATFORM
   #include "linux-dmabuf-unstable-v1-client-protocol.h"
   #include "wayland-drm-client-protocol.h"
   #include "wayland-drm.h"
   #include <wayland-client.h>
   #endif
      #ifdef HAVE_X11_PLATFORM
   #include "X11/Xlibint.h"
   #endif
      #include "GL/mesa_glinterop.h"
   #include "loader/loader.h"
   #include "mapi/glapi/glapi.h"
   #include "pipe/p_screen.h"
   #include "util/bitscan.h"
   #include "util/driconf.h"
   #include "util/libsync.h"
   #include "util/os_file.h"
   #include "util/u_atomic.h"
   #include "util/u_call_once.h"
   #include "util/u_math.h"
   #include "util/u_vector.h"
   #include "egl_dri2.h"
   #include "egldefines.h"
      #define NUM_ATTRIBS 16
      static const struct dri2_pbuffer_visual {
      const char *format_name;
   unsigned int dri_image_format;
   int rgba_shifts[4];
      } dri2_pbuffer_visuals[] = {
      /* clang-format off */
   {
      "ABGR16F",
   __DRI_IMAGE_FORMAT_ABGR16161616F,
   { 0, 16, 32, 48 },
      },
   {
      "XBGR16F",
   __DRI_IMAGE_FORMAT_XBGR16161616F,
   { 0, 16, 32, -1 },
      },
   {
      "A2RGB10",
   __DRI_IMAGE_FORMAT_ARGB2101010,
   { 20, 10, 0, 30 },
      },
   {
      "X2RGB10",
   __DRI_IMAGE_FORMAT_XRGB2101010,
   { 20, 10, 0, -1 },
      },
   {
      "ARGB8888",
   __DRI_IMAGE_FORMAT_ARGB8888,
   { 16, 8, 0, 24 },
      },
   {
      "RGB888",
   __DRI_IMAGE_FORMAT_XRGB8888,
   { 16, 8, 0, -1 },
      },
   {
      "RGB565",
   __DRI_IMAGE_FORMAT_RGB565,
   { 11, 5, 0, -1 },
      },
      };
      static void
   dri_set_background_context(void *loaderPrivate)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
               }
      static void
   dri2_gl_flush_get(_glapi_proc *glFlush)
   {
         }
      static void
   dri2_gl_flush()
   {
      static void (*glFlush)(void);
            util_call_once_data(&once, (util_call_once_data_func)dri2_gl_flush_get,
            /* if glFlush is not available things are horribly broken */
   if (!glFlush) {
      _eglLog(_EGL_WARNING, "DRI2: failed to find glFlush entry point");
                  }
      static GLboolean
   dri_is_thread_safe(UNUSED void *loaderPrivate)
   {
   #ifdef HAVE_X11_PLATFORM
               /* loader_dri3_blit_context_get creates a context with
   * loaderPrivate being NULL. Enabling glthread for a blitting
   * context isn't useful so return false.
   */
   if (!loaderPrivate)
                              /* Check Xlib is running in thread safe mode when running on EGL/X11-xlib
   * platform
   *
   * 'lock_fns' is the XLockDisplay function pointer of the X11 display 'dpy'.
   * It will be NULL if XInitThreads wasn't called.
   */
   if (display->Platform == _EGL_PLATFORM_X11 && xdpy && !xdpy->lock_fns)
      #endif
            }
      const __DRIbackgroundCallableExtension background_callable_extension = {
               .setBackgroundContext = dri_set_background_context,
      };
      const __DRIuseInvalidateExtension use_invalidate = {
         };
      static void
   dri2_get_pbuffer_drawable_info(__DRIdrawable *draw, int *x, int *y, int *w,
         {
               *x = *y = 0;
   *w = dri2_surf->base.Width;
      }
      static int
   dri2_get_bytes_per_pixel(struct dri2_egl_surface *dri2_surf)
   {
      const int depth = dri2_surf->base.Config->BufferSize;
      }
      static void
   dri2_put_image(__DRIdrawable *draw, int op, int x, int y, int w, int h,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   const int bpp = dri2_get_bytes_per_pixel(dri2_surf);
   const int width = dri2_surf->base.Width;
   const int height = dri2_surf->base.Height;
   const int dst_stride = width * bpp;
   const int src_stride = w * bpp;
   const int x_offset = x * bpp;
            if (!dri2_surf->swrast_device_buffer)
            if (dri2_surf->swrast_device_buffer) {
      const char *src = data;
            dst += x_offset;
            /* Drivers are allowed to submit OOB PutImage requests, so clip here. */
   if (copy_width > dst_stride - x_offset)
         if (h > height - y)
            for (; 0 < h; --h) {
      memcpy(dst, src, copy_width);
   dst += dst_stride;
            }
      static void
   dri2_get_image(__DRIdrawable *read, int x, int y, int w, int h, char *data,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
   const int bpp = dri2_get_bytes_per_pixel(dri2_surf);
   const int width = dri2_surf->base.Width;
   const int height = dri2_surf->base.Height;
   const int src_stride = width * bpp;
   const int dst_stride = w * bpp;
   const int x_offset = x * bpp;
   int copy_width = dst_stride;
   const char *src = dri2_surf->swrast_device_buffer;
            if (!src) {
      memset(data, 0, copy_width * h);
               src += x_offset;
            /* Drivers are allowed to submit OOB GetImage requests, so clip here. */
   if (copy_width > src_stride - x_offset)
         if (h > height - y)
            for (; 0 < h; --h) {
      memcpy(dst, src, copy_width);
   src += src_stride;
         }
      /* HACK: technically we should have swrast_null, instead of these.
   */
   const __DRIswrastLoaderExtension swrast_pbuffer_loader_extension = {
      .base = {__DRI_SWRAST_LOADER, 1},
   .getDrawableInfo = dri2_get_pbuffer_drawable_info,
   .putImage = dri2_put_image,
      };
      static const EGLint dri2_to_egl_attribute_map[__DRI_ATTRIB_MAX] = {
      [__DRI_ATTRIB_BUFFER_SIZE] = EGL_BUFFER_SIZE,
   [__DRI_ATTRIB_LEVEL] = EGL_LEVEL,
   [__DRI_ATTRIB_LUMINANCE_SIZE] = EGL_LUMINANCE_SIZE,
   [__DRI_ATTRIB_DEPTH_SIZE] = EGL_DEPTH_SIZE,
   [__DRI_ATTRIB_STENCIL_SIZE] = EGL_STENCIL_SIZE,
   [__DRI_ATTRIB_SAMPLE_BUFFERS] = EGL_SAMPLE_BUFFERS,
   [__DRI_ATTRIB_SAMPLES] = EGL_SAMPLES,
   [__DRI_ATTRIB_MAX_PBUFFER_WIDTH] = EGL_MAX_PBUFFER_WIDTH,
   [__DRI_ATTRIB_MAX_PBUFFER_HEIGHT] = EGL_MAX_PBUFFER_HEIGHT,
   [__DRI_ATTRIB_MAX_PBUFFER_PIXELS] = EGL_MAX_PBUFFER_PIXELS,
   [__DRI_ATTRIB_MAX_SWAP_INTERVAL] = EGL_MAX_SWAP_INTERVAL,
   [__DRI_ATTRIB_MIN_SWAP_INTERVAL] = EGL_MIN_SWAP_INTERVAL,
      };
      const __DRIconfig *
   dri2_get_dri_config(struct dri2_egl_config *conf, EGLint surface_type,
         {
      const bool double_buffer = surface_type == EGL_WINDOW_BIT;
               }
      static EGLBoolean
   dri2_match_config(const _EGLConfig *conf, const _EGLConfig *criteria)
   {
      if (_eglCompareConfigs(conf, criteria, NULL, EGL_FALSE) != 0)
            if (!_eglMatchConfig(conf, criteria))
               }
      void
   dri2_get_shifts_and_sizes(const __DRIcoreExtension *core,
               {
      core->getConfigAttrib(config, __DRI_ATTRIB_RED_SHIFT,
         core->getConfigAttrib(config, __DRI_ATTRIB_GREEN_SHIFT,
         core->getConfigAttrib(config, __DRI_ATTRIB_BLUE_SHIFT,
         core->getConfigAttrib(config, __DRI_ATTRIB_ALPHA_SHIFT,
         core->getConfigAttrib(config, __DRI_ATTRIB_RED_SIZE, &sizes[0]);
   core->getConfigAttrib(config, __DRI_ATTRIB_GREEN_SIZE, &sizes[1]);
   core->getConfigAttrib(config, __DRI_ATTRIB_BLUE_SIZE, &sizes[2]);
      }
      void
   dri2_get_render_type_float(const __DRIcoreExtension *core,
         {
               core->getConfigAttrib(config, __DRI_ATTRIB_RENDER_TYPE, &render_type);
      }
      unsigned int
   dri2_image_format_for_pbuffer_config(struct dri2_egl_display *dri2_dpy,
         {
      int shifts[4];
                     for (unsigned i = 0; i < ARRAY_SIZE(dri2_pbuffer_visuals); ++i) {
               if (shifts[0] == visual->rgba_shifts[0] &&
      shifts[1] == visual->rgba_shifts[1] &&
   shifts[2] == visual->rgba_shifts[2] &&
   shifts[3] == visual->rgba_shifts[3] &&
   sizes[0] == visual->rgba_sizes[0] &&
   sizes[1] == visual->rgba_sizes[1] &&
   sizes[2] == visual->rgba_sizes[2] &&
   sizes[3] == visual->rgba_sizes[3]) {
                     }
      struct dri2_egl_config *
   dri2_add_config(_EGLDisplay *disp, const __DRIconfig *dri_config, int id,
               {
      struct dri2_egl_config *conf;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLConfig base;
   unsigned int attrib, value, double_buffer;
   bool srgb = false;
   EGLint key, bind_to_texture_rgb, bind_to_texture_rgba;
   int dri_shifts[4] = {-1, -1, -1, -1};
   unsigned int dri_sizes[4] = {0, 0, 0, 0};
   _EGLConfig *matching_config;
   EGLint num_configs = 0;
                     double_buffer = 0;
   bind_to_texture_rgb = 0;
            for (int i = 0; i < __DRI_ATTRIB_MAX; ++i) {
      if (!dri2_dpy->core->indexConfigAttrib(dri_config, i, &attrib, &value))
            switch (attrib) {
   case __DRI_ATTRIB_RENDER_TYPE:
      if (value & __DRI_ATTRIB_FLOAT_BIT)
         if (value & __DRI_ATTRIB_RGBA_BIT)
         else if (value & __DRI_ATTRIB_LUMINANCE_BIT)
         else
                     case __DRI_ATTRIB_CONFIG_CAVEAT:
      if (value & __DRI_ATTRIB_NON_CONFORMANT_CONFIG)
         else if (value & __DRI_ATTRIB_SLOW_BIT)
         else
                     case __DRI_ATTRIB_BIND_TO_TEXTURE_RGB:
                  case __DRI_ATTRIB_BIND_TO_TEXTURE_RGBA:
                  case __DRI_ATTRIB_DOUBLE_BUFFER:
                  case __DRI_ATTRIB_RED_SIZE:
      dri_sizes[0] = value;
               case __DRI_ATTRIB_RED_MASK:
                  case __DRI_ATTRIB_RED_SHIFT:
                  case __DRI_ATTRIB_GREEN_SIZE:
      dri_sizes[1] = value;
               case __DRI_ATTRIB_GREEN_MASK:
                  case __DRI_ATTRIB_GREEN_SHIFT:
                  case __DRI_ATTRIB_BLUE_SIZE:
      dri_sizes[2] = value;
               case __DRI_ATTRIB_BLUE_MASK:
                  case __DRI_ATTRIB_BLUE_SHIFT:
                  case __DRI_ATTRIB_ALPHA_SIZE:
      dri_sizes[3] = value;
               case __DRI_ATTRIB_ALPHA_MASK:
                  case __DRI_ATTRIB_ALPHA_SHIFT:
                  case __DRI_ATTRIB_ACCUM_RED_SIZE:
   case __DRI_ATTRIB_ACCUM_GREEN_SIZE:
   case __DRI_ATTRIB_ACCUM_BLUE_SIZE:
   case __DRI_ATTRIB_ACCUM_ALPHA_SIZE:
      /* Don't expose visuals with the accumulation buffer. */
   if (value > 0)
               case __DRI_ATTRIB_FRAMEBUFFER_SRGB_CAPABLE:
      srgb = value != 0;
   if (!disp->Extensions.KHR_gl_colorspace && srgb)
               case __DRI_ATTRIB_MAX_PBUFFER_WIDTH:
      base.MaxPbufferWidth = _EGL_MAX_PBUFFER_WIDTH;
      case __DRI_ATTRIB_MAX_PBUFFER_HEIGHT:
      base.MaxPbufferHeight = _EGL_MAX_PBUFFER_HEIGHT;
      case __DRI_ATTRIB_MUTABLE_RENDER_BUFFER:
      if (disp->Extensions.KHR_mutable_render_buffer)
            default:
      key = dri2_to_egl_attribute_map[attrib];
   if (key != 0)
                        if (attr_list)
      for (int i = 0; attr_list[i] != EGL_NONE; i += 2)
         if (rgba_shifts && memcmp(rgba_shifts, dri_shifts, sizeof(dri_shifts)))
            if (rgba_sizes && memcmp(rgba_sizes, dri_sizes, sizeof(dri_sizes)))
                     base.SurfaceType = surface_type;
   if (surface_type &
      (EGL_PBUFFER_BIT |
   (disp->Extensions.NOK_texture_from_pixmap ? EGL_PIXMAP_BIT : 0))) {
   base.BindToTextureRGB = bind_to_texture_rgb;
   if (base.AlphaSize > 0)
               if (double_buffer) {
         } else {
                  if (!surface_type)
            base.RenderableType = disp->ClientAPIs;
            base.MinSwapInterval = dri2_dpy->min_swap_interval;
            if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "DRI2: failed to validate config %d", id);
               config_id = base.ConfigID;
   base.ConfigID = EGL_DONT_CARE;
   base.SurfaceType = EGL_DONT_CARE;
   num_configs = _eglFilterArray(disp->Configs, (void **)&matching_config, 1,
            if (num_configs == 1) {
               if (!conf->dri_config[double_buffer][srgb])
         else
      /* a similar config type is already added (unlikely) => discard */
   } else if (num_configs == 0) {
      conf = calloc(1, sizeof *conf);
   if (conf == NULL)
                     memcpy(&conf->base, &base, sizeof base);
   conf->base.SurfaceType = 0;
               } else {
      unreachable("duplicates should not be possible");
                           }
      EGLBoolean
   dri2_add_pbuffer_configs_for_visuals(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   unsigned int format_count[ARRAY_SIZE(dri2_pbuffer_visuals)] = {0};
            for (unsigned i = 0; dri2_dpy->driver_configs[i] != NULL; i++) {
      for (unsigned j = 0; j < ARRAY_SIZE(dri2_pbuffer_visuals); j++) {
               dri2_conf = dri2_add_config(disp, dri2_dpy->driver_configs[i],
                        if (dri2_conf) {
      if (dri2_conf->base.ConfigID == config_count + 1)
                           for (unsigned i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
      _eglLog(_EGL_DEBUG, "No DRI config supports native format %s",
                     }
      GLboolean
   dri2_validate_egl_image(void *image, void *data)
   {
      _EGLDisplay *disp = _eglLockDisplay(data);
   _EGLImage *img = _eglLookupImage(image, disp);
            if (img == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_validate_egl_image");
                  }
      __DRIimage *
   dri2_lookup_egl_image_validated(void *image, void *data)
   {
                                    }
      __DRIimage *
   dri2_lookup_egl_image(__DRIscreen *screen, void *image, void *data)
   {
               if (!dri2_validate_egl_image(image, data))
               }
      const __DRIimageLookupExtension image_lookup_extension = {
               .lookupEGLImage = dri2_lookup_egl_image,
   .validateEGLImage = dri2_validate_egl_image,
      };
      static const struct dri_extension_match dri3_driver_extensions[] = {
      {__DRI_CORE, 1, offsetof(struct dri2_egl_display, core), false},
   {__DRI_MESA, 1, offsetof(struct dri2_egl_display, mesa), false},
   {__DRI_IMAGE_DRIVER, 1, offsetof(struct dri2_egl_display, image_driver),
   false},
   {__DRI_CONFIG_OPTIONS, 2, offsetof(struct dri2_egl_display, configOptions),
      };
      static const struct dri_extension_match dri2_driver_extensions[] = {
      {__DRI_CORE, 1, offsetof(struct dri2_egl_display, core), false},
   {__DRI_MESA, 1, offsetof(struct dri2_egl_display, mesa), false},
   {__DRI_DRI2, 4, offsetof(struct dri2_egl_display, dri2), false},
   {__DRI_CONFIG_OPTIONS, 2, offsetof(struct dri2_egl_display, configOptions),
      };
      static const struct dri_extension_match dri2_core_extensions[] = {
      {__DRI2_FLUSH, 1, offsetof(struct dri2_egl_display, flush), false},
   {__DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer), false},
      };
      static const struct dri_extension_match swrast_driver_extensions[] = {
      {__DRI_CORE, 1, offsetof(struct dri2_egl_display, core), false},
   {__DRI_MESA, 1, offsetof(struct dri2_egl_display, mesa), false},
   {__DRI_SWRAST, 4, offsetof(struct dri2_egl_display, swrast), false},
   {__DRI_CONFIG_OPTIONS, 2, offsetof(struct dri2_egl_display, configOptions),
      };
      static const struct dri_extension_match swrast_core_extensions[] = {
      {__DRI_TEX_BUFFER, 2, offsetof(struct dri2_egl_display, tex_buffer), false},
      };
      static const struct dri_extension_match optional_core_extensions[] = {
      {__DRI2_CONFIG_QUERY, 1, offsetof(struct dri2_egl_display, config), true},
   {__DRI2_FENCE, 2, offsetof(struct dri2_egl_display, fence), true},
   {__DRI2_BUFFER_DAMAGE, 1, offsetof(struct dri2_egl_display, buffer_damage),
   true},
   {__DRI2_INTEROP, 1, offsetof(struct dri2_egl_display, interop), true},
   {__DRI2_FLUSH_CONTROL, 1, offsetof(struct dri2_egl_display, flush_control),
   true},
   {__DRI2_BLOB, 1, offsetof(struct dri2_egl_display, blob), true},
   {__DRI_MUTABLE_RENDER_BUFFER_DRIVER, 1,
   offsetof(struct dri2_egl_display, mutable_render_buffer), true},
      };
      static const __DRIextension **
   dri2_open_driver(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   static const char *search_path_vars[] = {
      "LIBGL_DRIVERS_PATH",
               return loader_open_driver(dri2_dpy->driver_name, &dri2_dpy->driver,
      }
      static EGLBoolean
   dri2_load_driver_common(_EGLDisplay *disp,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            extensions = dri2_open_driver(disp);
   if (!extensions)
            if (!loader_bind_extensions(dri2_dpy, driver_extensions, num_matches,
            dlclose(dri2_dpy->driver);
   dri2_dpy->driver = NULL;
      }
               }
      EGLBoolean
   dri2_load_driver(_EGLDisplay *disp)
   {
      return dri2_load_driver_common(disp, dri2_driver_extensions,
      }
      EGLBoolean
   dri2_load_driver_dri3(_EGLDisplay *disp)
   {
      return dri2_load_driver_common(disp, dri3_driver_extensions,
      }
      EGLBoolean
   dri2_load_driver_swrast(_EGLDisplay *disp)
   {
      return dri2_load_driver_common(disp, swrast_driver_extensions,
      }
      static const char *
   dri2_query_driver_name(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
      }
      static char *
   dri2_query_driver_config(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
                                 }
      static int
   get_screen_param(_EGLDisplay *disp, enum pipe_cap param)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri_screen *screen = dri_screen(dri2_dpy->dri_screen_render_gpu);
      }
      void
   dri2_setup_screen(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri_screen *screen = dri_screen(dri2_dpy->dri_screen_render_gpu);
   struct pipe_screen *pscreen = screen->base.screen;
            /*
   * EGL 1.5 specification defines the default value to 1. Moreover,
   * eglSwapInterval() is required to clamp requested value to the supported
   * range. Since the default value is implicitly assumed to be supported,
   * use it as both minimum and maximum for the platforms that do not allow
   * changing the interval. Platforms, which allow it (e.g. x11, wayland)
   * override these values already.
   */
   dri2_dpy->min_swap_interval = 1;
   dri2_dpy->max_swap_interval = 1;
            disp->ClientAPIs = 0;
   if ((api_mask & (1 << __DRI_API_OPENGL)) && _eglIsApiValid(EGL_OPENGL_API))
         if ((api_mask & (1 << __DRI_API_GLES)) && _eglIsApiValid(EGL_OPENGL_ES_API))
         if ((api_mask & (1 << __DRI_API_GLES2)) && _eglIsApiValid(EGL_OPENGL_ES_API))
         if ((api_mask & (1 << __DRI_API_GLES3)) && _eglIsApiValid(EGL_OPENGL_ES_API))
            assert(dri2_dpy->image_driver || dri2_dpy->dri2 || dri2_dpy->swrast);
   disp->Extensions.KHR_create_context = EGL_TRUE;
   disp->Extensions.KHR_create_context_no_error = EGL_TRUE;
   disp->Extensions.KHR_no_config_context = EGL_TRUE;
            if (dri2_dpy->interop) {
                  if (dri2_dpy->configOptions) {
                  /* Report back to EGL the bitmask of priorities supported */
   disp->Extensions.IMG_context_priority =
                     if (pscreen->is_format_supported(pscreen, PIPE_FORMAT_B8G8R8A8_SRGB,
                              disp->Extensions.EXT_create_context_robustness =
            if (dri2_dpy->fence) {
      disp->Extensions.KHR_fence_sync = EGL_TRUE;
   disp->Extensions.KHR_wait_sync = EGL_TRUE;
   if (dri2_dpy->fence->get_fence_from_cl_event)
         unsigned capabilities =
         disp->Extensions.ANDROID_native_fence_sync =
               if (dri2_dpy->blob)
                     if (dri2_dpy->image) {
      if (dri2_dpy->image->base.version >= 10 &&
                     capabilities =
                        if (dri2_dpy->image->base.version >= 11)
      } else {
      disp->Extensions.MESA_drm_image = EGL_TRUE;
   if (dri2_dpy->image->base.version >= 11)
               disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;
            if (get_screen_param(disp, PIPE_CAP_MAX_TEXTURE_3D_LEVELS) != 0)
      #ifdef HAVE_LIBDRM
         if (dri2_dpy->image->base.version >= 8 &&
      dri2_dpy->image->createImageFromDmaBufs) {
   disp->Extensions.EXT_image_dma_buf_import = EGL_TRUE;
      #endif
               if (dri2_dpy->flush_control)
            if (dri2_dpy->buffer_damage && dri2_dpy->buffer_damage->set_damage_region)
            disp->Extensions.EXT_protected_surface =
         disp->Extensions.EXT_protected_content =
      }
      void
   dri2_setup_swap_interval(_EGLDisplay *disp, int max_swap_interval)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            /* Allow driconf to override applications.*/
   if (dri2_dpy->config)
      dri2_dpy->config->configQueryi(dri2_dpy->dri_screen_render_gpu,
      switch (vblank_mode) {
   case DRI_CONF_VBLANK_NEVER:
      dri2_dpy->min_swap_interval = 0;
   dri2_dpy->max_swap_interval = 0;
   dri2_dpy->default_swap_interval = 0;
      case DRI_CONF_VBLANK_ALWAYS_SYNC:
      dri2_dpy->min_swap_interval = 1;
   dri2_dpy->max_swap_interval = max_swap_interval;
   dri2_dpy->default_swap_interval = 1;
      case DRI_CONF_VBLANK_DEF_INTERVAL_0:
      dri2_dpy->min_swap_interval = 0;
   dri2_dpy->max_swap_interval = max_swap_interval;
   dri2_dpy->default_swap_interval = 0;
      default:
   case DRI_CONF_VBLANK_DEF_INTERVAL_1:
      dri2_dpy->min_swap_interval = 0;
   dri2_dpy->max_swap_interval = max_swap_interval;
   dri2_dpy->default_swap_interval = 1;
         }
      /* All platforms but DRM call this function to create the screen and populate
   * the driver_configs. DRM inherits that information from its display - GBM.
   */
   EGLBoolean
   dri2_create_screen(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu) {
      driver_name_display_gpu =
         if (driver_name_display_gpu) {
      /* check if driver name is matching so that non mesa drivers
   * will not crash.
   */
   if (strcmp(dri2_dpy->driver_name, driver_name_display_gpu) == 0) {
      dri2_dpy->dri_screen_display_gpu = dri2_dpy->mesa->createNewScreen(
      0, dri2_dpy->fd_display_gpu, dri2_dpy->loader_extensions,
   }
                  int screen_fd = dri2_dpy->swrast ? -1 : dri2_dpy->fd_render_gpu;
   dri2_dpy->dri_screen_render_gpu = dri2_dpy->mesa->createNewScreen(
      0, screen_fd, dri2_dpy->loader_extensions, dri2_dpy->driver_extensions,
         if (dri2_dpy->dri_screen_render_gpu == NULL) {
      _eglLog(_EGL_WARNING, "egl: failed to create dri2 screen");
               if (dri2_dpy->fd_render_gpu == dri2_dpy->fd_display_gpu)
            dri2_dpy->own_dri_screen = true;
      }
      EGLBoolean
   dri2_setup_extensions(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                     if (dri2_dpy->image_driver || dri2_dpy->dri2 || disp->Options.Zink) {
      if (!loader_bind_extensions(dri2_dpy, dri2_core_extensions,
            } else {
      if (!loader_bind_extensions(dri2_dpy, swrast_core_extensions,
                        #ifdef HAVE_DRI3_MODIFIERS
      dri2_dpy->multibuffers_available =
      (dri2_dpy->dri3_major_version > 1 ||
   (dri2_dpy->dri3_major_version == 1 &&
   dri2_dpy->dri3_minor_version >= 2)) &&
   (dri2_dpy->present_major_version > 1 ||
   (dri2_dpy->present_major_version == 1 &&
   dri2_dpy->present_minor_version >= 2)) &&
   #endif
         #ifdef HAVE_DRI3_MODIFIERS
         dri2_dpy->dri3_major_version != -1 &&
   #endif
         (disp->Platform == EGL_PLATFORM_X11_KHR ||
   disp->Platform == EGL_PLATFORM_XCB_EXT) &&
   !debug_get_bool_option("LIBGL_KOPPER_DRI2", false))
         loader_bind_extensions(dri2_dpy, optional_core_extensions,
            }
      EGLBoolean
   dri2_setup_device(_EGLDisplay *disp, EGLBoolean software)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLDevice *dev;
            /* Extensions must be loaded before calling this function */
   assert(dri2_dpy->mesa);
   /* If we're not software, we need a DRM node FD */
            /* fd_render_gpu is what we got from WSI, so might actually be a lie and
   * not a render node... */
   if (software) {
         } else if (loader_is_device_render_capable(dri2_dpy->fd_render_gpu)) {
         } else {
      render_fd = dri2_dpy->mesa->queryCompatibleRenderOnlyDeviceFd(
         if (render_fd < 0)
                        if (render_fd >= 0 && render_fd != dri2_dpy->fd_render_gpu)
            if (!dev)
            disp->Device = dev;
      }
      /**
   * Called via eglInitialize(), drv->Initialize().
   *
   * This must be guaranteed to be called exactly once, even if eglInitialize is
   * called many times (without a eglTerminate in between).
   */
   static EGLBoolean
   dri2_initialize(_EGLDisplay *disp)
   {
      EGLBoolean ret = EGL_FALSE;
            /* In the case where the application calls eglMakeCurrent(context1),
   * eglTerminate, then eglInitialize again (without a call to eglReleaseThread
   * or eglMakeCurrent(NULL) before that), dri2_dpy structure is still
   * initialized, as we need it to be able to free context1 correctly.
   *
   * It would probably be safest to forcibly release the display with
   * dri2_display_release, to make sure the display is reinitialized correctly.
   * However, the EGL spec states that we need to keep a reference to the
   * current context (so we cannot call dri2_make_current(NULL)), and therefore
   * we would leak context1 as we would be missing the old display connection
   * to free it up correctly.
   */
   if (dri2_dpy) {
      p_atomic_inc(&dri2_dpy->ref_count);
                        switch (disp->Platform) {
   case _EGL_PLATFORM_SURFACELESS:
      ret = dri2_initialize_surfaceless(disp);
      case _EGL_PLATFORM_DEVICE:
      ret = dri2_initialize_device(disp);
      case _EGL_PLATFORM_X11:
   case _EGL_PLATFORM_XCB:
      ret = dri2_initialize_x11(disp);
      case _EGL_PLATFORM_DRM:
      ret = dri2_initialize_drm(disp);
      case _EGL_PLATFORM_WAYLAND:
      ret = dri2_initialize_wayland(disp);
      case _EGL_PLATFORM_ANDROID:
      ret = dri2_initialize_android(disp);
      default:
      unreachable("Callers ensure we cannot get here.");
               if (!ret)
            dri2_dpy = dri2_egl_display(disp);
                        }
      /**
   * Decrement display reference count, and free up display if necessary.
   */
   static void
   dri2_display_release(_EGLDisplay *disp)
   {
               if (!disp)
                              if (!p_atomic_dec_zero(&dri2_dpy->ref_count))
            _eglCleanupDisplay(disp);
      }
      void
   dri2_display_destroy(_EGLDisplay *disp)
   {
               if (dri2_dpy->own_dri_screen) {
      if (dri2_dpy->vtbl && dri2_dpy->vtbl->close_screen_notify)
                     if (dri2_dpy->dri_screen_display_gpu &&
      dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu)
   }
   if (dri2_dpy->fd_display_gpu >= 0 &&
      dri2_dpy->fd_render_gpu != dri2_dpy->fd_display_gpu)
      if (dri2_dpy->fd_render_gpu >= 0)
               /* Don't dlclose the driver when building with the address sanitizer, so
   * you get good symbols from the leak reports.
   #if !BUILT_WITH_ASAN || defined(NDEBUG)
      if (dri2_dpy->driver)
      #endif
               #ifdef HAVE_WAYLAND_PLATFORM
         #endif
         switch (disp->Platform) {
   case _EGL_PLATFORM_X11:
      dri2_teardown_x11(dri2_dpy);
      case _EGL_PLATFORM_DRM:
      dri2_teardown_drm(dri2_dpy);
      case _EGL_PLATFORM_WAYLAND:
      dri2_teardown_wayland(dri2_dpy);
      default:
      /* TODO: add teardown for other platforms */
               /* The drm platform does not create the screen/driver_configs but reuses
   * the ones from the gbm device. As such the gbm itself is responsible
   * for the cleanup.
   */
   if (disp->Platform != _EGL_PLATFORM_DRM && dri2_dpy->driver_configs) {
      for (unsigned i = 0; dri2_dpy->driver_configs[i]; i++)
            }
   free(dri2_dpy);
      }
      struct dri2_egl_display *
   dri2_display_create(void)
   {
      struct dri2_egl_display *dri2_dpy = calloc(1, sizeof *dri2_dpy);
   if (!dri2_dpy) {
      _eglError(EGL_BAD_ALLOC, "eglInitialize");
               dri2_dpy->fd_render_gpu = -1;
         #ifdef HAVE_DRI3_MODIFIERS
      dri2_dpy->dri3_major_version = -1;
   dri2_dpy->dri3_minor_version = -1;
   dri2_dpy->present_major_version = -1;
      #endif
            }
      __DRIbuffer *
   dri2_egl_surface_alloc_local_buffer(struct dri2_egl_surface *dri2_surf,
         {
      struct dri2_egl_display *dri2_dpy =
            if (att >= ARRAY_SIZE(dri2_surf->local_buffers))
            if (!dri2_surf->local_buffers[att]) {
      dri2_surf->local_buffers[att] = dri2_dpy->dri2->allocateBuffer(
      dri2_dpy->dri_screen_render_gpu, att, format, dri2_surf->base.Width,
               }
      void
   dri2_egl_surface_free_local_buffers(struct dri2_egl_surface *dri2_surf)
   {
      struct dri2_egl_display *dri2_dpy =
            for (int i = 0; i < ARRAY_SIZE(dri2_surf->local_buffers); i++) {
      if (dri2_surf->local_buffers[i]) {
      dri2_dpy->dri2->releaseBuffer(dri2_dpy->dri_screen_render_gpu,
                  }
      /**
   * Called via eglTerminate(), drv->Terminate().
   *
   * This must be guaranteed to be called exactly once, even if eglTerminate is
   * called many times (without a eglInitialize in between).
   */
   static EGLBoolean
   dri2_terminate(_EGLDisplay *disp)
   {
      /* Release all non-current Context/Surfaces. */
                        }
      /**
   * Set the error code after a call to
   * dri2_egl_display::dri2::createContextAttribs.
   */
   static void
   dri2_create_context_attribs_error(int dri_error)
   {
               switch (dri_error) {
   case __DRI_CTX_ERROR_SUCCESS:
            case __DRI_CTX_ERROR_NO_MEMORY:
      egl_error = EGL_BAD_ALLOC;
            /* From the EGL_KHR_create_context spec, section "Errors":
   *
   *   * If <config> does not support a client API context compatible
   *     with the requested API major and minor version, [...] context
   * flags, and context reset notification behavior (for client API types
   * where these attributes are supported), then an EGL_BAD_MATCH error is
   *     generated.
   *
   *   * If an OpenGL ES context is requested and the values for
   *     attributes EGL_CONTEXT_MAJOR_VERSION_KHR and
   *     EGL_CONTEXT_MINOR_VERSION_KHR specify an OpenGL ES version that
   *     is not defined, than an EGL_BAD_MATCH error is generated.
   *
   *   * If an OpenGL context is requested, the requested version is
   *     greater than 3.2, and the value for attribute
   *     EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR has no bits set; has any
   *     bits set other than EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR and
   *     EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT_KHR; has more than
   *     one of these bits set; or if the implementation does not support
   *     the requested profile, then an EGL_BAD_MATCH error is generated.
      case __DRI_CTX_ERROR_BAD_API:
   case __DRI_CTX_ERROR_BAD_VERSION:
   case __DRI_CTX_ERROR_BAD_FLAG:
      egl_error = EGL_BAD_MATCH;
            /* From the EGL_KHR_create_context spec, section "Errors":
   *
   *   * If an attribute name or attribute value in <attrib_list> is not
   *     recognized (including unrecognized bits in bitmask attributes),
   *     then an EGL_BAD_ATTRIBUTE error is generated."
      case __DRI_CTX_ERROR_UNKNOWN_ATTRIBUTE:
   case __DRI_CTX_ERROR_UNKNOWN_FLAG:
      egl_error = EGL_BAD_ATTRIBUTE;
         default:
      assert(!"unknown dri_error code");
   egl_error = EGL_BAD_MATCH;
                  }
      static bool
   dri2_fill_context_attribs(struct dri2_egl_context *dri2_ctx,
               {
                        ctx_attribs[pos++] = __DRI_CTX_ATTRIB_MAJOR_VERSION;
   ctx_attribs[pos++] = dri2_ctx->base.ClientMajorVersion;
   ctx_attribs[pos++] = __DRI_CTX_ATTRIB_MINOR_VERSION;
            if (dri2_ctx->base.Flags != 0) {
      ctx_attribs[pos++] = __DRI_CTX_ATTRIB_FLAGS;
               if (dri2_ctx->base.ResetNotificationStrategy !=
      EGL_NO_RESET_NOTIFICATION_KHR) {
   ctx_attribs[pos++] = __DRI_CTX_ATTRIB_RESET_STRATEGY;
               if (dri2_ctx->base.ContextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG) {
               switch (dri2_ctx->base.ContextPriority) {
   case EGL_CONTEXT_PRIORITY_HIGH_IMG:
      val = __DRI_CTX_PRIORITY_HIGH;
      case EGL_CONTEXT_PRIORITY_MEDIUM_IMG:
      val = __DRI_CTX_PRIORITY_MEDIUM;
      case EGL_CONTEXT_PRIORITY_LOW_IMG:
      val = __DRI_CTX_PRIORITY_LOW;
      default:
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
               ctx_attribs[pos++] = __DRI_CTX_ATTRIB_PRIORITY;
               if (dri2_ctx->base.ReleaseBehavior ==
      EGL_CONTEXT_RELEASE_BEHAVIOR_NONE_KHR) {
   ctx_attribs[pos++] = __DRI_CTX_ATTRIB_RELEASE_BEHAVIOR;
               if (dri2_ctx->base.NoError) {
      ctx_attribs[pos++] = __DRI_CTX_ATTRIB_NO_ERROR;
               if (dri2_ctx->base.Protected) {
      ctx_attribs[pos++] = __DRI_CTX_ATTRIB_PROTECTED;
                           }
      /**
   * Called via eglCreateContext(), drv->CreateContext().
   */
   static _EGLContext *
   dri2_create_context(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_context *dri2_ctx;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_context *dri2_ctx_shared = dri2_egl_context(share_list);
   __DRIcontext *shared = dri2_ctx_shared ? dri2_ctx_shared->dri_context : NULL;
   struct dri2_egl_config *dri2_config = dri2_egl_config(conf);
   const __DRIconfig *dri_config;
   int api;
   unsigned error;
   unsigned num_attribs = NUM_ATTRIBS;
            dri2_ctx = malloc(sizeof *dri2_ctx);
   if (!dri2_ctx) {
      dri2_egl_error_unlock(dri2_dpy, EGL_BAD_ALLOC, "eglCreateContext");
               if (!_eglInitContext(&dri2_ctx->base, disp, conf, share_list, attrib_list))
            switch (dri2_ctx->base.ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (dri2_ctx->base.ClientMajorVersion) {
   case 1:
      api = __DRI_API_GLES;
      case 2:
      api = __DRI_API_GLES2;
      case 3:
      api = __DRI_API_GLES3;
      default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
      }
      case EGL_OPENGL_API:
      if ((dri2_ctx->base.ClientMajorVersion >= 4 ||
      (dri2_ctx->base.ClientMajorVersion == 3 &&
         dri2_ctx->base.Profile == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
      else if (dri2_ctx->base.ClientMajorVersion == 3 &&
               else
            default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
               if (conf != NULL) {
      /* The config chosen here isn't necessarily
   * used for surfaces later.
   * A pixmap surface will use the single config.
   * This opportunity depends on disabling the
   * doubleBufferMode check in
   * src/mesa/main/context.c:check_compatible()
   */
   if (dri2_config->dri_config[1][0])
         else
      } else
            if (!dri2_fill_context_attribs(dri2_ctx, dri2_dpy, ctx_attribs,
                  dri2_ctx->dri_context = dri2_dpy->mesa->createContext(
      dri2_dpy->dri_screen_render_gpu, api, dri_config, shared, num_attribs / 2,
               if (!dri2_ctx->dri_context)
                           cleanup:
      mtx_unlock(&dri2_dpy->lock);
   free(dri2_ctx);
      }
      /**
   * Called via eglDestroyContext(), drv->DestroyContext().
   */
   static EGLBoolean
   dri2_destroy_context(_EGLDisplay *disp, _EGLContext *ctx)
   {
      struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
            if (_eglPutContext(ctx)) {
      dri2_dpy->core->destroyContext(dri2_ctx->dri_context);
                  }
      EGLBoolean
   dri2_init_surface(_EGLSurface *surf, _EGLDisplay *disp, EGLint type,
               {
      struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
            dri2_surf->out_fence_fd = -1;
   dri2_surf->enable_out_fence = false;
   if (dri2_dpy->fence &&
      (dri2_dpy->fence->get_capabilities(dri2_dpy->dri_screen_render_gpu) &
   __DRI_FENCE_CAP_NATIVE_FD)) {
                  }
      static void
   dri2_surface_set_out_fence_fd(_EGLSurface *surf, int fence_fd)
   {
               if (dri2_surf->out_fence_fd >= 0)
               }
      void
   dri2_fini_surface(_EGLSurface *surf)
   {
               dri2_surface_set_out_fence_fd(surf, -1);
      }
      static EGLBoolean
   dri2_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (_eglPutSurface(surf))
               }
      static void
   dri2_surf_update_fence_fd(_EGLContext *ctx, _EGLDisplay *disp,
         {
      __DRIcontext *dri_ctx = dri2_egl_context(ctx)->dri_context;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   int fence_fd = -1;
            if (!dri2_surf->enable_out_fence)
            fence = dri2_dpy->fence->create_fence_fd(dri_ctx, -1);
   if (fence) {
      fence_fd =
            }
      }
      EGLBoolean
   dri2_create_drawable(struct dri2_egl_display *dri2_dpy,
               {
      if (dri2_dpy->kopper) {
      dri2_surf->dri_drawable = dri2_dpy->kopper->createNewDrawable(
         #ifdef HAVE_X11_PLATFORM
         #endif
               .is_pixmap = dri2_surf->base.Type == EGL_PBUFFER_BIT ||
      } else {
      __DRIcreateNewDrawableFunc createNewDrawable;
   if (dri2_dpy->image_driver)
         else if (dri2_dpy->dri2)
         else if (dri2_dpy->swrast)
         else
            dri2_surf->dri_drawable = createNewDrawable(
      }
   if (dri2_surf->dri_drawable == NULL)
               }
      /**
   * Called via eglMakeCurrent(), drv->MakeCurrent().
   */
   static EGLBoolean
   dri2_make_current(_EGLDisplay *disp, _EGLSurface *dsurf, _EGLSurface *rsurf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   _EGLDisplay *old_disp = NULL;
   struct dri2_egl_display *old_dri2_dpy = NULL;
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   _EGLSurface *tmp_dsurf, *tmp_rsurf;
   __DRIdrawable *ddraw, *rdraw;
   __DRIcontext *cctx;
            if (!dri2_dpy)
            /* make new bindings, set the EGL error otherwise */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
            if (old_ctx == ctx && old_dsurf == dsurf && old_rsurf == rsurf) {
      _eglPutSurface(old_dsurf);
   _eglPutSurface(old_rsurf);
   _eglPutContext(old_ctx);
               if (old_ctx) {
      __DRIcontext *old_cctx = dri2_egl_context(old_ctx)->dri_context;
   old_disp = old_ctx->Resource.Display;
            /* Disable shared buffer mode */
   if (old_dsurf && _eglSurfaceInSharedBufferMode(old_dsurf) &&
      old_dri2_dpy->vtbl->set_shared_buffer_mode) {
                        if (old_dsurf)
               ddraw = (dsurf) ? dri2_dpy->vtbl->get_dri_drawable(dsurf) : NULL;
   rdraw = (rsurf) ? dri2_dpy->vtbl->get_dri_drawable(rsurf) : NULL;
            if (cctx) {
      if (!dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
               /* dri2_dpy->core->bindContext failed. We cannot tell for sure why, but
   * setting the error to EGL_BAD_MATCH is surely better than leaving it
   * as EGL_SUCCESS.
                  /* undo the previous _eglBindContext */
   _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &tmp_dsurf,
                        _eglPutSurface(dsurf);
                  _eglPutSurface(old_dsurf);
                  ddraw =
         rdraw =
                  /* undo the previous dri2_dpy->core->unbindContext */
   if (dri2_dpy->core->bindContext(cctx, ddraw, rdraw)) {
      if (old_dsurf && _eglSurfaceInSharedBufferMode(old_dsurf) &&
      old_dri2_dpy->vtbl->set_shared_buffer_mode) {
                                 /* We cannot restore the same state as it was before calling
   * eglMakeCurrent() and the spec isn't clear about what to do. We
   * can prevent EGL from calling into the DRI driver with no DRI
   * context bound.
   */
                  _eglBindContext(ctx, dsurf, rsurf, &tmp_ctx, &tmp_dsurf, &tmp_rsurf);
                     } else {
      /* dri2_dpy->core->bindContext succeeded, so take a reference on the
   * dri2_dpy. This prevents dri2_dpy from being reinitialized when a
   * EGLDisplay is terminated and then initialized again while a
   * context is still bound. See dri2_initialize() for a more in depth
   * explanation. */
                  dri2_destroy_surface(disp, old_dsurf);
            if (old_ctx) {
      dri2_destroy_context(disp, old_ctx);
               if (egl_error != EGL_SUCCESS)
            if (dsurf && _eglSurfaceHasMutableRenderBuffer(dsurf) &&
      dri2_dpy->vtbl->set_shared_buffer_mode) {
   /* Always update the shared buffer mode. This is obviously needed when
   * the active EGL_RENDER_BUFFER is EGL_SINGLE_BUFFER. When
   * EGL_RENDER_BUFFER is EGL_BACK_BUFFER, the update protects us in the
   * case where external non-EGL API may have changed window's shared
   * buffer mode since we last saw it.
   */
   bool mode = (dsurf->ActiveRenderBuffer == EGL_SINGLE_BUFFER);
                  }
      __DRIdrawable *
   dri2_surface_get_dri_drawable(_EGLSurface *surf)
   {
                  }
      static _EGLSurface *
   dri2_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   _EGLSurface *ret = dri2_dpy->vtbl->create_window_surface(
         mtx_unlock(&dri2_dpy->lock);
      }
      static _EGLSurface *
   dri2_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (dri2_dpy->vtbl->create_pixmap_surface)
      ret = dri2_dpy->vtbl->create_pixmap_surface(disp, conf, native_pixmap,
                     }
      static _EGLSurface *
   dri2_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (dri2_dpy->vtbl->create_pbuffer_surface)
                        }
      static EGLBoolean
   dri2_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (dri2_dpy->vtbl->swap_interval)
                        }
      /**
   * Asks the client API to flush any rendering to the drawable so that we can
   * do our swapbuffers.
   */
   void
   dri2_flush_drawable_for_swapbuffers_flags(
      _EGLDisplay *disp, _EGLSurface *draw,
      {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->flush) {
      if (dri2_dpy->flush->base.version >= 4) {
      /* We know there's a current context because:
   *
   *     "If surface is not bound to the calling thread’s current
   *      context, an EGL_BAD_SURFACE error is generated."
   */
                  /* From the EGL 1.4 spec (page 52):
   *
   *     "The contents of ancillary buffers are always undefined
   *      after calling eglSwapBuffers."
   */
   dri2_dpy->flush->flush_with_flags(
      dri2_ctx->dri_context, dri_drawable,
   __DRI2_FLUSH_DRAWABLE | __DRI2_FLUSH_INVALIDATE_ANCILLARY,
   } else {
               }
      void
   dri2_flush_drawable_for_swapbuffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
      dri2_flush_drawable_for_swapbuffers_flags(disp, draw,
      }
      static EGLBoolean
   dri2_swap_buffers(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   __DRIdrawable *dri_drawable = dri2_dpy->vtbl->get_dri_drawable(surf);
   _EGLContext *ctx = _eglGetCurrentContext();
            if (ctx && surf)
                  /* SwapBuffers marks the end of the frame; reset the damage region for
   * use again next time.
   */
   if (ret && dri2_dpy->buffer_damage &&
      dri2_dpy->buffer_damage->set_damage_region)
            }
      static EGLBoolean
   dri2_swap_buffers_with_damage(_EGLDisplay *disp, _EGLSurface *surf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   __DRIdrawable *dri_drawable = dri2_dpy->vtbl->get_dri_drawable(surf);
   _EGLContext *ctx = _eglGetCurrentContext();
            if (ctx && surf)
         if (dri2_dpy->vtbl->swap_buffers_with_damage)
      ret =
      else
            /* SwapBuffers marks the end of the frame; reset the damage region for
   * use again next time.
   */
   if (ret && dri2_dpy->buffer_damage &&
      dri2_dpy->buffer_damage->set_damage_region)
            }
      static EGLBoolean
   dri2_swap_buffers_region(_EGLDisplay *disp, _EGLSurface *surf, EGLint numRects,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   __DRIdrawable *dri_drawable = dri2_dpy->vtbl->get_dri_drawable(surf);
            if (!dri2_dpy->vtbl->swap_buffers_region)
                  /* SwapBuffers marks the end of the frame; reset the damage region for
   * use again next time.
   */
   if (ret && dri2_dpy->buffer_damage &&
      dri2_dpy->buffer_damage->set_damage_region)
            }
      static EGLBoolean
   dri2_set_damage_region(_EGLDisplay *disp, _EGLSurface *surf, EGLint *rects,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (!dri2_dpy->buffer_damage ||
      !dri2_dpy->buffer_damage->set_damage_region) {
   mtx_unlock(&dri2_dpy->lock);
               dri2_dpy->buffer_damage->set_damage_region(drawable, n_rects, rects);
   mtx_unlock(&dri2_dpy->lock);
      }
      static EGLBoolean
   dri2_post_sub_buffer(_EGLDisplay *disp, _EGLSurface *surf, EGLint x, EGLint y,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (dri2_dpy->vtbl->post_sub_buffer)
                        }
      static EGLBoolean
   dri2_copy_buffers(_EGLDisplay *disp, _EGLSurface *surf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   if (!dri2_dpy->vtbl->copy_buffers)
      return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_NATIVE_PIXMAP,
      EGLBoolean ret =
         mtx_unlock(&dri2_dpy->lock);
      }
      static EGLint
   dri2_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   if (!dri2_dpy->vtbl->query_buffer_age)
            }
      static EGLBoolean
   dri2_wait_client(_EGLDisplay *disp, _EGLContext *ctx)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLSurface *surf = ctx->DrawSurface;
            /* FIXME: If EGL allows frontbuffer rendering for window surfaces,
            if (dri2_dpy->flush != NULL)
               }
      static EGLBoolean
   dri2_wait_native(EGLint engine)
   {
      if (engine != EGL_CORE_NATIVE_ENGINE)
                     }
      static EGLBoolean
   dri2_bind_tex_image(_EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint format, target;
            ctx = _eglGetCurrentContext();
            if (!_eglBindTexImage(disp, surf, buffer)) {
      mtx_unlock(&dri2_dpy->lock);
               switch (surf->TextureFormat) {
   case EGL_TEXTURE_RGB:
      format = __DRI_TEXTURE_FORMAT_RGB;
      case EGL_TEXTURE_RGBA:
      format = __DRI_TEXTURE_FORMAT_RGBA;
      default:
      assert(!"Unexpected texture format in dri2_bind_tex_image()");
               switch (surf->TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      default:
      target = GL_TEXTURE_2D;
               dri2_dpy->tex_buffer->setTexBuffer2(dri2_ctx->dri_context, target, format,
                        }
      static EGLBoolean
   dri2_release_tex_image(_EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_context *dri2_ctx;
   _EGLContext *ctx;
   GLint target;
            ctx = _eglGetCurrentContext();
            if (!_eglReleaseTexImage(disp, surf, buffer)) {
      mtx_unlock(&dri2_dpy->lock);
               switch (surf->TextureTarget) {
   case EGL_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      default:
                  if (dri2_dpy->tex_buffer->base.version >= 3 &&
      dri2_dpy->tex_buffer->releaseTexBuffer != NULL) {
   dri2_dpy->tex_buffer->releaseTexBuffer(dri2_ctx->dri_context, target,
                           }
      static _EGLImage *
   dri2_create_image(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   _EGLImage *ret =
         mtx_unlock(&dri2_dpy->lock);
      }
      _EGLImage *
   dri2_create_image_from_dri(_EGLDisplay *disp, __DRIimage *dri_image)
   {
               if (dri_image == NULL) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image");
               dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image");
                                    }
      /**
   * Translate a DRI Image extension error code into an EGL error code.
   */
   static EGLint
   egl_error_from_dri_image_error(int dri_error)
   {
      switch (dri_error) {
   case __DRI_IMAGE_ERROR_SUCCESS:
         case __DRI_IMAGE_ERROR_BAD_ALLOC:
         case __DRI_IMAGE_ERROR_BAD_MATCH:
         case __DRI_IMAGE_ERROR_BAD_PARAMETER:
         case __DRI_IMAGE_ERROR_BAD_ACCESS:
         default:
      assert(!"unknown dri_error code");
         }
      static _EGLImage *
   dri2_create_image_khr_renderbuffer(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   GLuint renderbuffer = (GLuint)(uintptr_t)buffer;
            if (renderbuffer == 0) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
               if (!disp->Extensions.KHR_gl_renderbuffer_image) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
               if (dri2_dpy->image->base.version >= 17 &&
      dri2_dpy->image->createImageFromRenderbuffer2) {
            dri_image = dri2_dpy->image->createImageFromRenderbuffer2(
                     if (!dri_image) {
      _eglError(egl_error_from_dri_image_error(error),
               } else {
      dri_image = dri2_dpy->image->createImageFromRenderbuffer(
         if (!dri_image) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
                     }
      #ifdef HAVE_WAYLAND_PLATFORM
      /* This structure describes how a wl_buffer maps to one or more
   * __DRIimages.  A wl_drm_buffer stores the wl_drm format code and the
   * offsets and strides of the planes in the buffer.  This table maps a
   * wl_drm format code to a description of the planes in the buffer
   * that lets us create a __DRIimage for each of the planes. */
      static const struct wl_drm_components_descriptor {
      uint32_t dri_components;
   EGLint components;
      } wl_drm_components[] = {
      {__DRI_IMAGE_COMPONENTS_RGB, EGL_TEXTURE_RGB, 1},
   {__DRI_IMAGE_COMPONENTS_RGBA, EGL_TEXTURE_RGBA, 1},
   {__DRI_IMAGE_COMPONENTS_Y_U_V, EGL_TEXTURE_Y_U_V_WL, 3},
   {__DRI_IMAGE_COMPONENTS_Y_UV, EGL_TEXTURE_Y_UV_WL, 2},
      };
      static _EGLImage *
   dri2_create_image_wayland_wl_buffer(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct wl_drm_buffer *buffer;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   const struct wl_drm_components_descriptor *f;
   __DRIimage *dri_image;
   _EGLImageAttribs attrs;
            buffer = wayland_drm_buffer_get(dri2_dpy->wl_server_drm,
         if (!buffer)
            if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            plane = attrs.PlaneWL;
   f = buffer->driver_format;
   if (plane < 0 || plane >= f->nplanes) {
      _eglError(EGL_BAD_PARAMETER,
                     dri_image = dri2_dpy->image->fromPlanar(buffer->driver_buffer, plane, NULL);
   if (dri_image == NULL && plane == 0)
         if (dri_image == NULL) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_wayland_wl_buffer");
                  }
   #endif
      static EGLBoolean
   dri2_get_sync_values_chromium(_EGLDisplay *disp, _EGLSurface *surf,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (dri2_dpy->vtbl->get_sync_values)
               }
      static EGLBoolean
   dri2_get_msc_rate_angle(_EGLDisplay *disp, _EGLSurface *surf, EGLint *numerator,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   if (!dri2_dpy->vtbl->get_msc_rate)
            }
      /**
   * Set the error code after a call to
   * dri2_egl_image::dri_image::createImageFromTexture.
   */
   static void
   dri2_create_image_khr_texture_error(int dri_error)
   {
               if (egl_error != EGL_SUCCESS)
      }
      static _EGLImage *
   dri2_create_image_khr_texture(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_image *dri2_img;
   GLuint texture = (GLuint)(uintptr_t)buffer;
   _EGLImageAttribs attrs;
   GLuint depth;
   GLenum gl_target;
            if (texture == 0) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
               if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            switch (target) {
   case EGL_GL_TEXTURE_2D_KHR:
      if (!disp->Extensions.KHR_gl_texture_2D_image) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
      }
   depth = 0;
   gl_target = GL_TEXTURE_2D;
      case EGL_GL_TEXTURE_3D_KHR:
      if (!disp->Extensions.KHR_gl_texture_3D_image) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
               depth = attrs.GLTextureZOffset;
   gl_target = GL_TEXTURE_3D;
      case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
      if (!disp->Extensions.KHR_gl_texture_cubemap_image) {
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
               depth = target - EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR;
   gl_target = GL_TEXTURE_CUBE_MAP;
      default:
      unreachable("Unexpected target in dri2_create_image_khr_texture()");
               dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
                        dri2_img->dri_image = dri2_dpy->image->createImageFromTexture(
      dri2_ctx->dri_context, gl_target, texture, depth, attrs.GLTextureLevel,
               if (!dri2_img->dri_image) {
      free(dri2_img);
      }
      }
      static EGLBoolean
   dri2_query_surface(_EGLDisplay *disp, _EGLSurface *surf, EGLint attribute,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_dpy->vtbl->query_surface) {
         } else {
                     }
      static struct wl_buffer *
   dri2_create_wayland_buffer_from_image(_EGLDisplay *disp, _EGLImage *img)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (dri2_dpy->vtbl->create_wayland_buffer_from_image)
                        }
      #ifdef HAVE_LIBDRM
   static _EGLImage *
   dri2_create_image_mesa_drm_buffer(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   EGLint format, name, pitch;
   _EGLImageAttribs attrs;
                     if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            if (attrs.Width <= 0 || attrs.Height <= 0 ||
      attrs.DRMBufferStrideMESA <= 0) {
   _eglError(EGL_BAD_PARAMETER, "bad width, height or stride");
               switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
   pitch = attrs.DRMBufferStrideMESA;
      default:
      _eglError(EGL_BAD_PARAMETER,
                     dri_image = dri2_dpy->image->createImageFromName(
      dri2_dpy->dri_screen_render_gpu, attrs.Width, attrs.Height, format, name,
            }
      static EGLBoolean
   dri2_check_dma_buf_attribs(const _EGLImageAttribs *attrs)
   {
      /**
   * The spec says:
   *
   * "Required attributes and their values are as follows:
   *
   *  * EGL_WIDTH & EGL_HEIGHT: The logical dimensions of the buffer in pixels
   *
   *  * EGL_LINUX_DRM_FOURCC_EXT: The pixel format of the buffer, as specified
   *    by drm_fourcc.h and used as the pixel_format parameter of the
   *    drm_mode_fb_cmd2 ioctl."
   *
   * and
   *
   * "* If <target> is EGL_LINUX_DMA_BUF_EXT, and the list of attributes is
   *    incomplete, EGL_BAD_PARAMETER is generated."
   */
   if (attrs->Width <= 0 || attrs->Height <= 0 ||
      !attrs->DMABufFourCC.IsPresent)
         /**
   * Also:
   *
   * "If <target> is EGL_LINUX_DMA_BUF_EXT and one or more of the values
   *  specified for a plane's pitch or offset isn't supported by EGL,
   *  EGL_BAD_ACCESS is generated."
   */
   for (unsigned i = 0; i < ARRAY_SIZE(attrs->DMABufPlanePitches); ++i) {
      if (attrs->DMABufPlanePitches[i].IsPresent &&
      attrs->DMABufPlanePitches[i].Value <= 0)
            /**
   * If <target> is EGL_LINUX_DMA_BUF_EXT, both or neither of the following
   * attribute values may be given.
   *
   * This is referring to EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT and
   * EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT, and the same for other planes.
   */
   for (unsigned i = 0; i < DMA_BUF_MAX_PLANES; ++i) {
      if (attrs->DMABufPlaneModifiersLo[i].IsPresent !=
      attrs->DMABufPlaneModifiersHi[i].IsPresent)
   return _eglError(EGL_BAD_PARAMETER,
            /* Although the EGL_EXT_image_dma_buf_import_modifiers spec doesn't
   * mandate it, we only accept the same modifier across all planes. */
   for (unsigned i = 1; i < DMA_BUF_MAX_PLANES; ++i) {
      if (attrs->DMABufPlaneFds[i].IsPresent) {
      if ((attrs->DMABufPlaneModifiersLo[0].IsPresent !=
      attrs->DMABufPlaneModifiersLo[i].IsPresent) ||
   (attrs->DMABufPlaneModifiersLo[0].Value !=
   attrs->DMABufPlaneModifiersLo[i].Value) ||
   (attrs->DMABufPlaneModifiersHi[0].Value !=
   attrs->DMABufPlaneModifiersHi[i].Value))
   return _eglError(EGL_BAD_PARAMETER,
                  }
      /* Returns the total number of planes for the format or zero if it isn't a
   * valid fourcc format.
   */
   static unsigned
   dri2_num_fourcc_format_planes(EGLint format)
   {
      switch (format) {
   case DRM_FORMAT_R8:
   case DRM_FORMAT_RG88:
   case DRM_FORMAT_GR88:
   case DRM_FORMAT_R16:
   case DRM_FORMAT_GR1616:
   case DRM_FORMAT_RGB332:
   case DRM_FORMAT_BGR233:
   case DRM_FORMAT_XRGB4444:
   case DRM_FORMAT_XBGR4444:
   case DRM_FORMAT_RGBX4444:
   case DRM_FORMAT_BGRX4444:
   case DRM_FORMAT_ARGB4444:
   case DRM_FORMAT_ABGR4444:
   case DRM_FORMAT_RGBA4444:
   case DRM_FORMAT_BGRA4444:
   case DRM_FORMAT_XRGB1555:
   case DRM_FORMAT_XBGR1555:
   case DRM_FORMAT_RGBX5551:
   case DRM_FORMAT_BGRX5551:
   case DRM_FORMAT_ARGB1555:
   case DRM_FORMAT_ABGR1555:
   case DRM_FORMAT_RGBA5551:
   case DRM_FORMAT_BGRA5551:
   case DRM_FORMAT_RGB565:
   case DRM_FORMAT_BGR565:
   case DRM_FORMAT_RGB888:
   case DRM_FORMAT_BGR888:
   case DRM_FORMAT_XRGB8888:
   case DRM_FORMAT_XBGR8888:
   case DRM_FORMAT_RGBX8888:
   case DRM_FORMAT_BGRX8888:
   case DRM_FORMAT_ARGB8888:
   case DRM_FORMAT_ABGR8888:
   case DRM_FORMAT_RGBA8888:
   case DRM_FORMAT_BGRA8888:
   case DRM_FORMAT_XRGB2101010:
   case DRM_FORMAT_XBGR2101010:
   case DRM_FORMAT_RGBX1010102:
   case DRM_FORMAT_BGRX1010102:
   case DRM_FORMAT_ARGB2101010:
   case DRM_FORMAT_ABGR2101010:
   case DRM_FORMAT_RGBA1010102:
   case DRM_FORMAT_BGRA1010102:
   case DRM_FORMAT_ABGR16161616:
   case DRM_FORMAT_XBGR16161616:
   case DRM_FORMAT_XBGR16161616F:
   case DRM_FORMAT_ABGR16161616F:
   case DRM_FORMAT_YUYV:
   case DRM_FORMAT_YVYU:
   case DRM_FORMAT_UYVY:
   case DRM_FORMAT_VYUY:
   case DRM_FORMAT_AYUV:
   case DRM_FORMAT_XYUV8888:
   case DRM_FORMAT_Y210:
   case DRM_FORMAT_Y212:
   case DRM_FORMAT_Y216:
   case DRM_FORMAT_Y410:
   case DRM_FORMAT_Y412:
   case DRM_FORMAT_Y416:
            case DRM_FORMAT_NV12:
   case DRM_FORMAT_NV21:
   case DRM_FORMAT_NV16:
   case DRM_FORMAT_NV61:
   case DRM_FORMAT_P010:
   case DRM_FORMAT_P012:
   case DRM_FORMAT_P016:
   case DRM_FORMAT_P030:
            case DRM_FORMAT_YUV410:
   case DRM_FORMAT_YVU410:
   case DRM_FORMAT_YUV411:
   case DRM_FORMAT_YVU411:
   case DRM_FORMAT_YUV420:
   case DRM_FORMAT_YVU420:
   case DRM_FORMAT_YUV422:
   case DRM_FORMAT_YVU422:
   case DRM_FORMAT_YUV444:
   case DRM_FORMAT_YVU444:
            default:
            }
      /* Returns the total number of file descriptors. Zero indicates an error. */
   static unsigned
   dri2_check_dma_buf_format(const _EGLImageAttribs *attrs)
   {
      unsigned plane_n = dri2_num_fourcc_format_planes(attrs->DMABufFourCC.Value);
   if (plane_n == 0) {
      _eglError(EGL_BAD_MATCH, "unknown drm fourcc format");
               for (unsigned i = plane_n; i < DMA_BUF_MAX_PLANES; i++) {
      /**
   * The modifiers extension spec says:
   *
   * "Modifiers may modify any attribute of a buffer import, including
   *  but not limited to adding extra planes to a format which
   *  otherwise does not have those planes. As an example, a modifier
   *  may add a plane for an external compression buffer to a
   *  single-plane format. The exact meaning and effect of any
   *  modifier is canonically defined by drm_fourcc.h, not as part of
   *  this extension."
   */
   if (attrs->DMABufPlaneModifiersLo[i].IsPresent &&
      attrs->DMABufPlaneModifiersHi[i].IsPresent) {
                  /**
   * The spec says:
   *
   * "* If <target> is EGL_LINUX_DMA_BUF_EXT, and the list of attributes is
   *    incomplete, EGL_BAD_PARAMETER is generated."
   */
   for (unsigned i = 0; i < plane_n; ++i) {
      if (!attrs->DMABufPlaneFds[i].IsPresent ||
      !attrs->DMABufPlaneOffsets[i].IsPresent ||
   !attrs->DMABufPlanePitches[i].IsPresent) {
   _eglError(EGL_BAD_PARAMETER, "plane attribute(s) missing");
                  /**
   * The spec also says:
   *
   * "If <target> is EGL_LINUX_DMA_BUF_EXT, and the EGL_LINUX_DRM_FOURCC_EXT
   *  attribute indicates a single-plane format, EGL_BAD_ATTRIBUTE is
   *  generated if any of the EGL_DMA_BUF_PLANE1_* or EGL_DMA_BUF_PLANE2_*
   *  or EGL_DMA_BUF_PLANE3_* attributes are specified."
   */
   for (unsigned i = plane_n; i < DMA_BUF_MAX_PLANES; ++i) {
      if (attrs->DMABufPlaneFds[i].IsPresent ||
      attrs->DMABufPlaneOffsets[i].IsPresent ||
   attrs->DMABufPlanePitches[i].IsPresent) {
   _eglError(EGL_BAD_ATTRIBUTE, "too many plane attributes");
                     }
      static EGLBoolean
   dri2_query_dma_buf_formats(_EGLDisplay *disp, EGLint max, EGLint *formats,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   if (max < 0 || (max > 0 && formats == NULL)) {
      _eglError(EGL_BAD_PARAMETER, "invalid value for max count of formats");
               if (dri2_dpy->image->base.version < 15 ||
      dri2_dpy->image->queryDmaBufFormats == NULL)
         if (!dri2_dpy->image->queryDmaBufFormats(dri2_dpy->dri_screen_render_gpu,
                  if (max > 0) {
      /* Assert that all of the formats returned are actually fourcc formats.
   * Some day, if we want the internal interface function to be able to
   * return the fake fourcc formats defined in dri_interface.h, we'll have
   * to do something more clever here to pair the list down to just real
   * fourcc formats so that we don't leak the fake internal ones.
   */
   for (int i = 0; i < *count; i++) {
                                    fail:
      mtx_unlock(&dri2_dpy->lock);
      }
      static EGLBoolean
   dri2_query_dma_buf_modifiers(_EGLDisplay *disp, EGLint format, EGLint max,
               {
               if (dri2_num_fourcc_format_planes(format) == 0)
      return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_PARAMETER,
         if (max < 0)
      return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_PARAMETER,
         if (max > 0 && modifiers == NULL)
      return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_PARAMETER,
         if (dri2_dpy->image->base.version < 15 ||
      dri2_dpy->image->queryDmaBufModifiers == NULL) {
   mtx_unlock(&dri2_dpy->lock);
               if (dri2_dpy->image->queryDmaBufModifiers(
         dri2_dpy->dri_screen_render_gpu, format, max, modifiers,
      return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_PARAMETER,
                     }
      /**
   * The spec says:
   *
   * "If eglCreateImageKHR is successful for a EGL_LINUX_DMA_BUF_EXT target, the
   *  EGL will take a reference to the dma_buf(s) which it will release at any
   *  time while the EGLDisplay is initialized. It is the responsibility of the
   *  application to close the dma_buf file descriptors."
   *
   * Therefore we must never close or otherwise modify the file descriptors.
   */
   _EGLImage *
   dri2_create_image_dma_buf(_EGLDisplay *disp, _EGLContext *ctx,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLImage *res;
   _EGLImageAttribs attrs;
   __DRIimage *dri_image;
   unsigned num_fds;
   int fds[DMA_BUF_MAX_PLANES];
   int pitches[DMA_BUF_MAX_PLANES];
   int offsets[DMA_BUF_MAX_PLANES];
   uint64_t modifier;
   bool has_modifier = false;
   unsigned error;
            /**
   * The spec says:
   *
   * ""* If <target> is EGL_LINUX_DMA_BUF_EXT and <buffer> is not NULL, the
   *     error EGL_BAD_PARAMETER is generated."
   */
   if (buffer != NULL) {
      _eglError(EGL_BAD_PARAMETER, "buffer not NULL");
               if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            if (!dri2_check_dma_buf_attribs(&attrs))
            num_fds = dri2_check_dma_buf_format(&attrs);
   if (!num_fds)
            for (unsigned i = 0; i < num_fds; ++i) {
      fds[i] = attrs.DMABufPlaneFds[i].Value;
   pitches[i] = attrs.DMABufPlanePitches[i].Value;
               /* dri2_check_dma_buf_attribs ensures that the modifier, if available,
   * will be present in attrs.DMABufPlaneModifiersLo[0] and
   * attrs.DMABufPlaneModifiersHi[0] */
   if (attrs.DMABufPlaneModifiersLo[0].IsPresent) {
      modifier = combine_u32_into_u64(attrs.DMABufPlaneModifiersHi[0].Value,
                     if (attrs.ProtectedContent) {
      if (dri2_dpy->image->base.version < 18 ||
      dri2_dpy->image->createImageFromDmaBufs3 == NULL) {
   _eglError(EGL_BAD_MATCH, "unsupported protected_content attribute");
      }
   if (!has_modifier)
            dri_image = dri2_dpy->image->createImageFromDmaBufs3(
      dri2_dpy->dri_screen_render_gpu, attrs.Width, attrs.Height,
   attrs.DMABufFourCC.Value, modifier, fds, num_fds, pitches, offsets,
   attrs.DMABufYuvColorSpaceHint.Value, attrs.DMABufSampleRangeHint.Value,
   attrs.DMABufChromaHorizontalSiting.Value,
   attrs.DMABufChromaVerticalSiting.Value,
   attrs.ProtectedContent ? __DRI_IMAGE_PROTECTED_CONTENT_FLAG : 0,
   } else if (has_modifier) {
      if (dri2_dpy->image->base.version < 15 ||
      dri2_dpy->image->createImageFromDmaBufs2 == NULL) {
   _eglError(EGL_BAD_MATCH, "unsupported dma_buf format modifier");
      }
   dri_image = dri2_dpy->image->createImageFromDmaBufs2(
      dri2_dpy->dri_screen_render_gpu, attrs.Width, attrs.Height,
   attrs.DMABufFourCC.Value, modifier, fds, num_fds, pitches, offsets,
   attrs.DMABufYuvColorSpaceHint.Value, attrs.DMABufSampleRangeHint.Value,
   attrs.DMABufChromaHorizontalSiting.Value,
   } else {
      dri_image = dri2_dpy->image->createImageFromDmaBufs(
      dri2_dpy->dri_screen_render_gpu, attrs.Width, attrs.Height,
   attrs.DMABufFourCC.Value, fds, num_fds, pitches, offsets,
   attrs.DMABufYuvColorSpaceHint.Value, attrs.DMABufSampleRangeHint.Value,
   attrs.DMABufChromaHorizontalSiting.Value,
            egl_error = egl_error_from_dri_image_error(error);
   if (egl_error != EGL_SUCCESS)
            if (!dri_image)
                        }
   static _EGLImage *
   dri2_create_drm_image_mesa(_EGLDisplay *disp, const EGLint *attr_list)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_image *dri2_img;
   _EGLImageAttribs attrs;
   unsigned int dri_use, valid_mask;
            if (!attr_list) {
      _eglError(EGL_BAD_PARAMETER, __func__);
               if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            if (attrs.Width <= 0 || attrs.Height <= 0) {
      _eglError(EGL_BAD_PARAMETER, __func__);
               switch (attrs.DRMBufferFormatMESA) {
   case EGL_DRM_BUFFER_FORMAT_ARGB32_MESA:
      format = __DRI_IMAGE_FORMAT_ARGB8888;
      default:
      _eglError(EGL_BAD_PARAMETER, __func__);
               valid_mask = EGL_DRM_BUFFER_USE_SCANOUT_MESA |
         if (attrs.DRMBufferUseMESA & ~valid_mask) {
      _eglError(EGL_BAD_PARAMETER, __func__);
               dri_use = 0;
   if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SHARE_MESA)
         if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_SCANOUT_MESA)
         if (attrs.DRMBufferUseMESA & EGL_DRM_BUFFER_USE_CURSOR_MESA)
            dri2_img = malloc(sizeof *dri2_img);
   if (!dri2_img) {
      _eglError(EGL_BAD_ALLOC, "dri2_create_image_khr");
                        dri2_img->dri_image =
      dri2_dpy->image->createImage(dri2_dpy->dri_screen_render_gpu, attrs.Width,
      if (dri2_img->dri_image == NULL) {
      free(dri2_img);
   _eglError(EGL_BAD_ALLOC, "dri2_create_drm_image_mesa");
                              fail:
      mtx_unlock(&dri2_dpy->lock);
      }
      static EGLBoolean
   dri2_export_drm_image_mesa(_EGLDisplay *disp, _EGLImage *img, EGLint *name,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            if (name && !dri2_dpy->image->queryImage(dri2_img->dri_image,
            return dri2_egl_error_unlock(dri2_dpy, EGL_BAD_ALLOC,
         if (handle)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
         if (stride)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
                     }
      /**
   * Checks if we can support EGL_MESA_image_dma_buf_export on this image.
      * The spec provides a boolean return for the driver to reject exporting for
   * basically any reason, but doesn't specify any particular error cases.  For
   * now, we just fail if we don't have a DRM fourcc for the format.
   */
   static bool
   dri2_can_export_dma_buf_image(_EGLDisplay *disp, _EGLImage *img)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);
            if (!dri2_dpy->image->queryImage(dri2_img->dri_image,
                           }
      static EGLBoolean
   dri2_export_dma_buf_image_query_mesa(_EGLDisplay *disp, _EGLImage *img,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);
            if (!dri2_can_export_dma_buf_image(disp, img)) {
      mtx_unlock(&dri2_dpy->lock);
               dri2_dpy->image->queryImage(dri2_img->dri_image,
         if (nplanes)
            if (fourcc)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
         if (modifiers) {
      int mod_hi, mod_lo;
   uint64_t modifier = DRM_FORMAT_MOD_INVALID;
            query = dri2_dpy->image->queryImage(
         query &= dri2_dpy->image->queryImage(
         if (query)
            for (int i = 0; i < num_planes; i++)
                           }
      static EGLBoolean
   dri2_export_dma_buf_image_mesa(_EGLDisplay *disp, _EGLImage *img, int *fds,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_image *dri2_img = dri2_egl_image(img);
            if (!dri2_can_export_dma_buf_image(disp, img)) {
      mtx_unlock(&dri2_dpy->lock);
               /* EGL_MESA_image_dma_buf_export spec says:
   *    "If the number of fds is less than the number of planes, then
   *    subsequent fd slots should contain -1."
   */
   if (fds) {
      /* Query nplanes so that we know how big the given array is. */
   dri2_dpy->image->queryImage(dri2_img->dri_image,
                     /* rework later to provide multiple fds/strides/offsets */
   if (fds)
      dri2_dpy->image->queryImage(dri2_img->dri_image, __DRI_IMAGE_ATTRIB_FD,
         if (strides)
      dri2_dpy->image->queryImage(dri2_img->dri_image,
         if (offsets) {
      int img_offset;
   bool ret = dri2_dpy->image->queryImage(
         if (ret)
         else
                           }
      #endif
      _EGLImage *
   dri2_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
      switch (target) {
   case EGL_GL_TEXTURE_2D_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
   case EGL_GL_TEXTURE_3D_KHR:
      return dri2_create_image_khr_texture(disp, ctx, target, buffer,
      case EGL_GL_RENDERBUFFER_KHR:
      #ifdef HAVE_LIBDRM
      case EGL_DRM_BUFFER_MESA:
         case EGL_LINUX_DMA_BUF_EXT:
      #endif
   #ifdef HAVE_WAYLAND_PLATFORM
      case EGL_WAYLAND_BUFFER_WL:
      #endif
      default:
      _eglError(EGL_BAD_PARAMETER, "dri2_create_image_khr");
         }
      static EGLBoolean
   dri2_destroy_image_khr(_EGLDisplay *disp, _EGLImage *image)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
            dri2_dpy->image->destroyImage(dri2_img->dri_image);
                        }
      #ifdef HAVE_WAYLAND_PLATFORM
      static void
   dri2_wl_reference_buffer(void *user_data, uint32_t name, int fd,
         {
      _EGLDisplay *disp = user_data;
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   __DRIimage *img;
            if (fd == -1)
      img = dri2_dpy->image->createImageFromNames(
      dri2_dpy->dri_screen_render_gpu, buffer->width, buffer->height,
   else
      img = dri2_dpy->image->createImageFromFds(
               if (img == NULL)
            dri2_dpy->image->queryImage(img, __DRI_IMAGE_ATTRIB_COMPONENTS,
            buffer->driver_format = NULL;
   for (int i = 0; i < ARRAY_SIZE(wl_drm_components); i++)
      if (wl_drm_components[i].dri_components == dri_components)
         if (buffer->driver_format == NULL)
         else
      }
      static void
   dri2_wl_release_buffer(void *user_data, struct wl_drm_buffer *buffer)
   {
      _EGLDisplay *disp = user_data;
               }
      static EGLBoolean
   dri2_bind_wayland_display_wl(_EGLDisplay *disp, struct wl_display *wl_dpy)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   const struct wayland_drm_callbacks wl_drm_callbacks = {
      .authenticate = (int (*)(void *, uint32_t))dri2_dpy->vtbl->authenticate,
   .reference_buffer = dri2_wl_reference_buffer,
   .release_buffer = dri2_wl_release_buffer,
      };
   int flags = 0;
   char *device_name;
            if (dri2_dpy->wl_server_drm)
            device_name = drmGetRenderDeviceNameFromFd(dri2_dpy->fd_render_gpu);
   if (!device_name)
         if (!device_name)
            if (drmGetCap(dri2_dpy->fd_render_gpu, DRM_CAP_PRIME, &cap) == 0 &&
      cap == (DRM_PRIME_CAP_IMPORT | DRM_PRIME_CAP_EXPORT) &&
   dri2_dpy->image->base.version >= 7 &&
   dri2_dpy->image->createImageFromFds != NULL)
         dri2_dpy->wl_server_drm =
                     if (!dri2_dpy->wl_server_drm)
         #ifdef HAVE_DRM_PLATFORM
      /* We have to share the wl_drm instance with gbm, so gbm can convert
   * wl_buffers to gbm bos. */
   if (dri2_dpy->gbm_dri)
      #endif
         mtx_unlock(&dri2_dpy->lock);
         fail:
      mtx_unlock(&dri2_dpy->lock);
      }
      static EGLBoolean
   dri2_unbind_wayland_display_wl(_EGLDisplay *disp, struct wl_display *wl_dpy)
   {
               if (!dri2_dpy->wl_server_drm)
            wayland_drm_uninit(dri2_dpy->wl_server_drm);
               }
      static EGLBoolean
   dri2_query_wayland_buffer_wl(_EGLDisplay *disp,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct wl_drm_buffer *buffer;
            buffer = wayland_drm_buffer_get(dri2_dpy->wl_server_drm, buffer_resource);
   if (!buffer)
            format = buffer->driver_format;
   switch (attribute) {
   case EGL_TEXTURE_FORMAT:
      *value = format->components;
      case EGL_WIDTH:
      *value = buffer->width;
      case EGL_HEIGHT:
      *value = buffer->height;
                  }
   #endif
      static void
   dri2_egl_ref_sync(struct dri2_egl_sync *sync)
   {
         }
      static void
   dri2_egl_unref_sync(struct dri2_egl_display *dri2_dpy,
         {
      if (p_atomic_dec_zero(&dri2_sync->refcount)) {
      switch (dri2_sync->base.Type) {
   case EGL_SYNC_REUSABLE_KHR:
      cnd_destroy(&dri2_sync->cond);
      case EGL_SYNC_NATIVE_FENCE_ANDROID:
      if (dri2_sync->base.SyncFd != EGL_NO_NATIVE_FENCE_FD_ANDROID)
            default:
                  if (dri2_sync->fence)
                        }
      static _EGLSync *
   dri2_create_sync(_EGLDisplay *disp, EGLenum type, const EGLAttrib *attrib_list)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_sync *dri2_sync;
   EGLint ret;
            dri2_sync = calloc(1, sizeof(struct dri2_egl_sync));
   if (!dri2_sync) {
      _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
               if (!_eglInitSync(&dri2_sync->base, disp, type, attrib_list)) {
                  switch (type) {
   case EGL_SYNC_FENCE_KHR:
      dri2_sync->fence = dri2_dpy->fence->create_fence(dri2_ctx->dri_context);
   if (!dri2_sync->fence) {
      /* Why did it fail? DRI doesn't return an error code, so we emit
   * a generic EGL error that doesn't communicate user error.
   */
   _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
      }
         case EGL_SYNC_CL_EVENT_KHR:
      dri2_sync->fence = dri2_dpy->fence->get_fence_from_cl_event(
         /* this can only happen if the cl_event passed in is invalid. */
   if (!dri2_sync->fence) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglCreateSyncKHR");
               /* the initial status must be "signaled" if the cl_event is signaled */
   if (dri2_dpy->fence->client_wait_sync(dri2_ctx->dri_context,
                     case EGL_SYNC_REUSABLE_KHR:
      /* initialize attr */
            if (ret) {
      _eglError(EGL_BAD_ACCESS, "eglCreateSyncKHR");
               /* change clock attribute to CLOCK_MONOTONIC */
            if (ret) {
      _eglError(EGL_BAD_ACCESS, "eglCreateSyncKHR");
                        if (ret) {
      _eglError(EGL_BAD_ACCESS, "eglCreateSyncKHR");
               /* initial status of reusable sync must be "unsignaled" */
   dri2_sync->base.SyncStatus = EGL_UNSIGNALED_KHR;
         case EGL_SYNC_NATIVE_FENCE_ANDROID:
      if (dri2_dpy->fence->create_fence_fd) {
      dri2_sync->fence = dri2_dpy->fence->create_fence_fd(
      }
   if (!dri2_sync->fence) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglCreateSyncKHR");
      }
               p_atomic_set(&dri2_sync->refcount, 1);
                  fail:
      free(dri2_sync);
   mtx_unlock(&dri2_dpy->lock);
      }
      static EGLBoolean
   dri2_destroy_sync(_EGLDisplay *disp, _EGLSync *sync)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   struct dri2_egl_sync *dri2_sync = dri2_egl_sync(sync);
   EGLint ret = EGL_TRUE;
            /* if type of sync is EGL_SYNC_REUSABLE_KHR and it is not signaled yet,
   * then unlock all threads possibly blocked by the reusable sync before
   * destroying it.
   */
   if (dri2_sync->base.Type == EGL_SYNC_REUSABLE_KHR &&
      dri2_sync->base.SyncStatus == EGL_UNSIGNALED_KHR) {
   dri2_sync->base.SyncStatus = EGL_SIGNALED_KHR;
   /* unblock all threads currently blocked by sync */
            if (err) {
      _eglError(EGL_BAD_ACCESS, "eglDestroySyncKHR");
                                       }
      static EGLint
   dri2_dup_native_fence_fd(_EGLDisplay *disp, _EGLSync *sync)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
                     if (sync->SyncFd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
      /* try to retrieve the actual native fence fd.. if rendering is
   * not flushed this will just return -1, aka NO_NATIVE_FENCE_FD:
   */
   sync->SyncFd = dri2_dpy->fence->get_fence_fd(
                        if (sync->SyncFd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
      /* if native fence fd still not created, return an error: */
   _eglError(EGL_BAD_PARAMETER, "eglDupNativeFenceFDANDROID");
                           }
      static void
   dri2_set_blob_cache_funcs(_EGLDisplay *disp, EGLSetBlobFuncANDROID set,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display_lock(disp);
   dri2_dpy->blob->set_cache_funcs(dri2_dpy->dri_screen_render_gpu, set, get);
      }
      static EGLint
   dri2_client_wait_sync(_EGLDisplay *disp, _EGLSync *sync, EGLint flags,
         {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
   struct dri2_egl_sync *dri2_sync = dri2_egl_sync(sync);
                     /* The EGL_KHR_fence_sync spec states:
   *
   *    "If no context is current for the bound API,
   *     the EGL_SYNC_FLUSH_COMMANDS_BIT_KHR bit is ignored.
   */
   if (dri2_ctx && flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR)
            /* the sync object should take a reference while waiting */
            switch (sync->Type) {
   case EGL_SYNC_FENCE_KHR:
   case EGL_SYNC_NATIVE_FENCE_ANDROID:
   case EGL_SYNC_CL_EVENT_KHR:
      if (dri2_dpy->fence->client_wait_sync(
         dri2_ctx ? dri2_ctx->dri_context : NULL, dri2_sync->fence,
         else
               case EGL_SYNC_REUSABLE_KHR:
      if (dri2_ctx && dri2_sync->base.SyncStatus == EGL_UNSIGNALED_KHR &&
      (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR)) {
   /* flush context if EGL_SYNC_FLUSH_COMMANDS_BIT_KHR is set */
               /* if timeout is EGL_FOREVER_KHR, it should wait without any timeout.*/
   if (timeout == EGL_FOREVER_KHR) {
      mtx_lock(&dri2_sync->mutex);
   cnd_wait(&dri2_sync->cond, &dri2_sync->mutex);
      } else {
      /* if reusable sync has not been yet signaled */
   if (dri2_sync->base.SyncStatus != EGL_SIGNALED_KHR) {
      /* timespecs for cnd_timedwait */
                  /* We override the clock to monotonic when creating the condition
                  /* calculating when to expire */
                                 /* expire.nsec now is a number between 0 and 1999999998 */
   if (expire.tv_nsec > 999999999L) {
                        mtx_lock(&dri2_sync->mutex);
                  if (ret == thrd_timedout) {
      if (dri2_sync->base.SyncStatus == EGL_UNSIGNALED_KHR) {
         } else {
      _eglError(EGL_BAD_ACCESS, "eglClientWaitSyncKHR");
               }
                           }
      static EGLBoolean
   dri2_signal_sync(_EGLDisplay *disp, _EGLSync *sync, EGLenum mode)
   {
      struct dri2_egl_sync *dri2_sync = dri2_egl_sync(sync);
            if (sync->Type != EGL_SYNC_REUSABLE_KHR)
            if (mode != EGL_SIGNALED_KHR && mode != EGL_UNSIGNALED_KHR)
                     if (mode == EGL_SIGNALED_KHR) {
               /* fail to broadcast */
   if (ret)
                  }
      static EGLint
   dri2_server_wait_sync(_EGLDisplay *disp, _EGLSync *sync)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_context *dri2_ctx = dri2_egl_context(ctx);
            dri2_dpy->fence->server_wait_sync(dri2_ctx->dri_context, dri2_sync->fence,
            }
      static int
   dri2_interop_query_device_info(_EGLDisplay *disp, _EGLContext *ctx,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_dpy->interop)
               }
      static int
   dri2_interop_export_object(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_dpy->interop)
               }
      static int
   dri2_interop_flush_objects(_EGLDisplay *disp, _EGLContext *ctx, unsigned count,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
            if (!dri2_dpy->interop || dri2_dpy->interop->base.version < 2)
            return dri2_dpy->interop->flush_objects(dri2_ctx->dri_context, count,
      }
      const _EGLDriver _eglDriver = {
      .Initialize = dri2_initialize,
   .Terminate = dri2_terminate,
   .CreateContext = dri2_create_context,
   .DestroyContext = dri2_destroy_context,
   .MakeCurrent = dri2_make_current,
   .CreateWindowSurface = dri2_create_window_surface,
   .CreatePixmapSurface = dri2_create_pixmap_surface,
   .CreatePbufferSurface = dri2_create_pbuffer_surface,
   .DestroySurface = dri2_destroy_surface,
   .WaitClient = dri2_wait_client,
   .WaitNative = dri2_wait_native,
   .BindTexImage = dri2_bind_tex_image,
   .ReleaseTexImage = dri2_release_tex_image,
   .SwapInterval = dri2_swap_interval,
   .SwapBuffers = dri2_swap_buffers,
   .SwapBuffersWithDamageEXT = dri2_swap_buffers_with_damage,
   .SwapBuffersRegionNOK = dri2_swap_buffers_region,
   .SetDamageRegion = dri2_set_damage_region,
   .PostSubBufferNV = dri2_post_sub_buffer,
   .CopyBuffers = dri2_copy_buffers,
   .QueryBufferAge = dri2_query_buffer_age,
   .CreateImageKHR = dri2_create_image,
   .DestroyImageKHR = dri2_destroy_image_khr,
   .CreateWaylandBufferFromImageWL = dri2_create_wayland_buffer_from_image,
   .QuerySurface = dri2_query_surface,
   .QueryDriverName = dri2_query_driver_name,
      #ifdef HAVE_LIBDRM
      .CreateDRMImageMESA = dri2_create_drm_image_mesa,
   .ExportDRMImageMESA = dri2_export_drm_image_mesa,
   .ExportDMABUFImageQueryMESA = dri2_export_dma_buf_image_query_mesa,
   .ExportDMABUFImageMESA = dri2_export_dma_buf_image_mesa,
   .QueryDmaBufFormatsEXT = dri2_query_dma_buf_formats,
      #endif
   #ifdef HAVE_WAYLAND_PLATFORM
      .BindWaylandDisplayWL = dri2_bind_wayland_display_wl,
   .UnbindWaylandDisplayWL = dri2_unbind_wayland_display_wl,
      #endif
      .GetSyncValuesCHROMIUM = dri2_get_sync_values_chromium,
   .GetMscRateANGLE = dri2_get_msc_rate_angle,
   .CreateSyncKHR = dri2_create_sync,
   .ClientWaitSyncKHR = dri2_client_wait_sync,
   .SignalSyncKHR = dri2_signal_sync,
   .WaitSyncKHR = dri2_server_wait_sync,
   .DestroySyncKHR = dri2_destroy_sync,
   .GLInteropQueryDeviceInfo = dri2_interop_query_device_info,
   .GLInteropExportObject = dri2_interop_export_object,
   .GLInteropFlushObjects = dri2_interop_flush_objects,
   .DupNativeFenceFDANDROID = dri2_dup_native_fence_fd,
      };
