   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2014 Adrián Arroyo Calle <adrian.arroyocalle@gmail.com>
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
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <dlfcn.h>
   #include <errno.h>
   #include <stdint.h>
   #include <stdio.h>
      #include <algorithm>
      #include "eglconfig.h"
   #include "eglcontext.h"
   #include "eglcurrent.h"
   #include "egldevice.h"
   #include "egldisplay.h"
   #include "egldriver.h"
   #include "eglimage.h"
   #include "egllog.h"
   #include "eglsurface.h"
   #include "egltypedefs.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "state_tracker/st_context.h"
   #include "util/u_atomic.h"
   #include <mapi/glapi/glapi.h>
      #include "hgl_context.h"
   #include "hgl_sw_winsys.h"
      extern "C" {
   #include "target-helpers/inline_sw_helper.h"
   }
      #define BGL_RGB           0
   #define BGL_INDEX         1
   #define BGL_SINGLE        0
   #define BGL_DOUBLE        2
   #define BGL_DIRECT        0
   #define BGL_INDIRECT      4
   #define BGL_ACCUM         8
   #define BGL_ALPHA         16
   #define BGL_DEPTH         32
   #define BGL_OVERLAY       64
   #define BGL_UNDERLAY      128
   #define BGL_STENCIL       512
   #define BGL_SHARE_CONTEXT 1024
      #ifdef DEBUG
   #define TRACE(x...) printf("egl_haiku: " x)
   #define CALLED()    TRACE("CALLED: %s\n", __PRETTY_FUNCTION__)
   #else
   #define TRACE(x...)
   #define CALLED()
   #endif
   #define ERROR(x...) printf("egl_haiku: " x)
      _EGL_DRIVER_STANDARD_TYPECASTS(haiku_egl)
      struct haiku_egl_display {
      int ref_count;
      };
      struct haiku_egl_config {
         };
      struct haiku_egl_context {
      _EGLContext base;
      };
      struct haiku_egl_surface {
      _EGLSurface base;
   struct hgl_buffer *fb;
      };
      // #pragma mark EGLSurface
      // Called via eglCreateWindowSurface(), drv->CreateWindowSurface().
   static _EGLSurface *
   haiku_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      printf("haiku_create_window_surface\n");
            struct haiku_egl_surface *wgl_surf =
         if (!wgl_surf)
            if (!_eglInitSurface(&wgl_surf->base, disp, EGL_WINDOW_BIT, conf,
            free(wgl_surf);
               struct st_visual visual;
            wgl_surf->fb =
         if (!wgl_surf->fb) {
      free(wgl_surf);
                  }
      static _EGLSurface *
   haiku_create_pixmap_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      printf("haiku_create_pixmap_surface\n");
      }
      static _EGLSurface *
   haiku_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      printf("haiku_create_pbuffer_surface\n");
            struct haiku_egl_surface *wgl_surf =
         if (!wgl_surf)
            if (!_eglInitSurface(&wgl_surf->base, disp, EGL_PBUFFER_BIT, conf,
            free(wgl_surf);
               struct st_visual visual;
            wgl_surf->fb = hgl_create_st_framebuffer(hgl_dpy->disp, &visual, NULL);
   if (!wgl_surf->fb) {
      free(wgl_surf);
               wgl_surf->fb->newWidth = wgl_surf->base.Width;
   wgl_surf->fb->newHeight = wgl_surf->base.Height;
               }
      static EGLBoolean
   haiku_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct haiku_egl_display *hgl_dpy = haiku_egl_display(disp);
   if (_eglPutSurface(surf)) {
      struct haiku_egl_surface *hgl_surf = haiku_egl_surface(surf);
   struct pipe_screen *screen = hgl_dpy->disp->fscreen->screen;
   screen->fence_reference(screen, &hgl_surf->throttle_fence, NULL);
   hgl_destroy_st_framebuffer(hgl_surf->fb);
      }
      }
      static void
   update_size(struct hgl_buffer *buffer)
   {
      uint32_t newWidth, newHeight;
   ((BitmapHook *)buffer->winsysContext)->GetSize(newWidth, newHeight);
   if (buffer->newWidth != newWidth || buffer->newHeight != newHeight) {
      buffer->newWidth = newWidth;
   buffer->newHeight = newHeight;
         }
      static EGLBoolean
   haiku_swap_buffers(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct haiku_egl_display *hgl_dpy = haiku_egl_display(disp);
   struct haiku_egl_surface *hgl_surf = haiku_egl_surface(surf);
   struct haiku_egl_context *hgl_ctx = haiku_egl_context(surf->CurrentContext);
   if (hgl_ctx == NULL)
            struct st_context *st = hgl_ctx->ctx->st;
            struct hgl_buffer *buffer = hgl_surf->fb;
   auto &frontBuffer = buffer->textures[ST_ATTACHMENT_FRONT_LEFT];
            // Inform ST of a flush if double buffering is used
   if (backBuffer != NULL)
                     struct pipe_fence_handle *new_fence = NULL;
   st_context_flush(st, ST_FLUSH_FRONT, &new_fence, NULL, NULL);
   if (hgl_surf->throttle_fence) {
      screen->fence_finish(screen, NULL, hgl_surf->throttle_fence,
            }
            // flush back buffer and swap buffers if double buffering is used
   if (backBuffer != NULL) {
      screen->flush_frontbuffer(screen, st->pipe, backBuffer, 0, 0,
         std::swap(frontBuffer, backBuffer);
                                    }
      // #pragma mark EGLDisplay
      static EGLBoolean
   haiku_add_configs_for_visuals(_EGLDisplay *disp)
   {
               struct haiku_egl_config *conf;
   conf = (struct haiku_egl_config *)calloc(1, sizeof(*conf));
   if (!conf)
            _eglInitConfig(&conf->base, disp, 1);
            conf->base.RedSize = 8;
   conf->base.BlueSize = 8;
   conf->base.GreenSize = 8;
   conf->base.LuminanceSize = 0;
   conf->base.AlphaSize = 8;
   conf->base.ColorBufferType = EGL_RGB_BUFFER;
   conf->base.BufferSize = conf->base.RedSize + conf->base.GreenSize +
         conf->base.ConfigCaveat = EGL_NONE;
   conf->base.ConfigID = 1;
   conf->base.BindToTextureRGB = EGL_FALSE;
   conf->base.BindToTextureRGBA = EGL_FALSE;
   conf->base.StencilSize = 0;
   conf->base.TransparentType = EGL_NONE;
   conf->base.NativeRenderable = EGL_TRUE; // Let's say yes
   conf->base.NativeVisualID = 0;          // No visual
   conf->base.NativeVisualType = EGL_NONE; // No visual
   conf->base.RenderableType = 0x8;
   conf->base.SampleBuffers = 0; // TODO: How to get the right value ?
   conf->base.Samples = conf->base.SampleBuffers == 0 ? 0 : 0;
   conf->base.DepthSize = 24; // TODO: How to get the right value ?
   conf->base.Level = 0;
   conf->base.MaxPbufferWidth = _EGL_MAX_PBUFFER_WIDTH;
   conf->base.MaxPbufferHeight = _EGL_MAX_PBUFFER_HEIGHT;
   conf->base.MaxPbufferPixels = 0; // TODO: How to get the right value ?
            TRACE("Config configuated\n");
   if (!_eglValidateConfig(&conf->base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "Haiku: failed to validate config");
      }
            _eglLinkConfig(&conf->base);
   if (!_eglGetArraySize(disp->Configs)) {
      _eglLog(_EGL_WARNING, "Haiku: failed to create any config");
      }
                  cleanup:
      free(conf);
      }
      static void
   haiku_display_destroy(_EGLDisplay *disp)
   {
      if (!disp)
                     assert(hgl_dpy->ref_count > 0);
   if (!p_atomic_dec_zero(&hgl_dpy->ref_count))
            struct pipe_screen *screen = hgl_dpy->disp->fscreen->screen;
   hgl_destroy_display(hgl_dpy->disp);
   hgl_dpy->disp = NULL;
               }
      static EGLBoolean
   haiku_initialize_impl(_EGLDisplay *disp, void *platformDisplay)
   {
      struct haiku_egl_display *hgl_dpy;
            hgl_dpy =
         if (!hgl_dpy)
            hgl_dpy->ref_count = 1;
            struct sw_winsys *winsys = hgl_create_sw_winsys();
   struct pipe_screen *screen = sw_screen_create(winsys);
            disp->ClientAPIs = 0;
   if (_eglIsApiValid(EGL_OPENGL_API))
         if (_eglIsApiValid(EGL_OPENGL_ES_API))
      disp->ClientAPIs |=
         disp->Extensions.KHR_no_config_context = EGL_TRUE;
   disp->Extensions.KHR_surfaceless_context = EGL_TRUE;
            /* Report back to EGL the bitmask of priorities supported */
   disp->Extensions.IMG_context_priority =
      hgl_dpy->disp->fscreen->screen->get_param(hgl_dpy->disp->fscreen->screen,
                  if (hgl_dpy->disp->fscreen->screen->is_format_supported(
         hgl_dpy->disp->fscreen->screen, PIPE_FORMAT_B8G8R8A8_SRGB,
            disp->Extensions.KHR_create_context = EGL_TRUE;
                           cleanup:
      haiku_display_destroy(disp);
      }
      static EGLBoolean
   haiku_initialize(_EGLDisplay *disp)
   {
      EGLBoolean ret = EGL_FALSE;
            if (hgl_dpy) {
      hgl_dpy->ref_count++;
               switch (disp->Platform) {
   case _EGL_PLATFORM_SURFACELESS:
   case _EGL_PLATFORM_HAIKU:
      ret = haiku_initialize_impl(disp, NULL);
      case _EGL_PLATFORM_DEVICE:
      ret = haiku_initialize_impl(disp, disp->PlatformDisplay);
      default:
      unreachable("Callers ensure we cannot get here.");
               if (!ret)
                        }
      static EGLBoolean
   haiku_terminate(_EGLDisplay *disp)
   {
      haiku_display_destroy(disp);
      }
      // #pragma mark EGLContext
      static _EGLContext *
   haiku_create_context(_EGLDisplay *disp, _EGLConfig *conf,
         {
                        struct st_visual visual;
            struct haiku_egl_context *context =
         if (!context) {
      _eglError(EGL_BAD_ALLOC, "haiku_create_context");
               if (!_eglInitContext(&context->base, disp, conf, share_list, attrib_list))
            context->ctx = hgl_create_context(
      hgl_dpy->disp, &visual,
      if (context->ctx == NULL)
                  cleanup:
      free(context);
      }
      static EGLBoolean
   haiku_destroy_context(_EGLDisplay *disp, _EGLContext *ctx)
   {
      if (_eglPutContext(ctx)) {
      struct haiku_egl_context *hgl_ctx = haiku_egl_context(ctx);
   hgl_destroy_context(hgl_ctx->ctx);
   free(ctx);
      }
      }
      static EGLBoolean
   haiku_make_current(_EGLDisplay *disp, _EGLSurface *dsurf, _EGLSurface *rsurf,
         {
               struct haiku_egl_context *hgl_ctx = haiku_egl_context(ctx);
   struct haiku_egl_surface *hgl_dsurf = haiku_egl_surface(dsurf);
   struct haiku_egl_surface *hgl_rsurf = haiku_egl_surface(rsurf);
   _EGLContext *old_ctx;
            if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
            if (old_ctx == ctx && old_dsurf == dsurf && old_rsurf == rsurf) {
      _eglPutSurface(old_dsurf);
   _eglPutSurface(old_rsurf);
   _eglPutContext(old_ctx);
               if (ctx == NULL) {
         } else {
      if (dsurf != NULL && dsurf != old_dsurf)
            st_api_make_current(hgl_ctx->ctx->st,
                     if (old_dsurf != NULL)
         if (old_rsurf != NULL)
         if (old_ctx != NULL)
               }
      extern "C" const _EGLDriver _eglDriver = {
      .Initialize = haiku_initialize,
   .Terminate = haiku_terminate,
   .CreateContext = haiku_create_context,
   .DestroyContext = haiku_destroy_context,
   .MakeCurrent = haiku_make_current,
   .CreateWindowSurface = haiku_create_window_surface,
   .CreatePixmapSurface = haiku_create_pixmap_surface,
   .CreatePbufferSurface = haiku_create_pbuffer_surface,
   .DestroySurface = haiku_destroy_surface,
      };
