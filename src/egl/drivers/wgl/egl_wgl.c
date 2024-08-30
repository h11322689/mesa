   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "state_tracker/st_context.h"
      #include <eglcontext.h>
   #include <eglcurrent.h>
   #include <egldriver.h>
   #include <egllog.h>
   #include <eglsurface.h>
      #include "egl_wgl.h"
      #include <stw_context.h>
   #include <stw_device.h>
   #include <stw_ext_interop.h>
   #include <stw_framebuffer.h>
   #include <stw_image.h>
   #include <stw_pixelformat.h>
   #include <stw_winsys.h>
      #include <GL/wglext.h>
      #include <pipe/p_context.h>
   #include <pipe/p_screen.h>
   #include <pipe/p_state.h>
      #include "util/u_call_once.h"
   #include <mapi/glapi/glapi.h>
      #include <GL/mesa_glinterop.h>
      static EGLBoolean
   wgl_match_config(const _EGLConfig *conf, const _EGLConfig *criteria)
   {
      if (_eglCompareConfigs(conf, criteria, NULL, EGL_FALSE) != 0)
            if (!_eglMatchConfig(conf, criteria))
               }
      static struct wgl_egl_config *
   wgl_add_config(_EGLDisplay *disp, const struct stw_pixelformat_info *stw_config,
         {
      struct wgl_egl_config *conf;
   _EGLConfig base;
   unsigned int double_buffer;
   _EGLConfig *matching_config;
   EGLint num_configs = 0;
                              if (stw_config->pfd.iPixelType != PFD_TYPE_RGBA)
            base.RedSize = stw_config->pfd.cRedBits;
   base.GreenSize = stw_config->pfd.cGreenBits;
   base.BlueSize = stw_config->pfd.cBlueBits;
   base.AlphaSize = stw_config->pfd.cAlphaBits;
            if (stw_config->pfd.cAccumBits) {
      /* Don't expose visuals with the accumulation buffer. */
               base.MaxPbufferWidth = _EGL_MAX_PBUFFER_WIDTH;
            base.DepthSize = stw_config->pfd.cDepthBits;
   base.StencilSize = stw_config->pfd.cStencilBits;
   base.Samples = stw_config->stvis.samples;
                     if (surface_type & EGL_PBUFFER_BIT) {
      base.BindToTextureRGB = stw_config->bindToTextureRGB;
   if (base.AlphaSize > 0)
               if (double_buffer) {
                  if (!(stw_config->pfd.dwFlags & PFD_DRAW_TO_WINDOW)) {
                  if (!surface_type)
            base.SurfaceType = surface_type;
   base.RenderableType = disp->ClientAPIs;
            base.MinSwapInterval = 0;
   base.MaxSwapInterval = 4;
            if (!_eglValidateConfig(&base, EGL_FALSE)) {
      _eglLog(_EGL_DEBUG, "wgl: failed to validate config %d", id);
               config_id = base.ConfigID;
   base.ConfigID = EGL_DONT_CARE;
   base.SurfaceType = EGL_DONT_CARE;
   num_configs = _eglFilterArray(disp->Configs, (void **)&matching_config, 1,
            if (num_configs == 1) {
               if (!conf->stw_config[double_buffer])
         else
      /* a similar config type is already added (unlikely) => discard */
   } else if (num_configs == 0) {
      conf = calloc(1, sizeof(*conf));
   if (conf == NULL)
                     memcpy(&conf->base, &base, sizeof base);
   conf->base.SurfaceType = 0;
               } else {
      unreachable("duplicates should not be possible");
                           }
      static EGLBoolean
   wgl_add_configs(_EGLDisplay *disp)
   {
      unsigned int config_count = 0;
            // This is already a filtered set of what the driver supports,
   // and there's no further filtering needed per-visual
               struct wgl_egl_config *wgl_conf = wgl_add_config(
            if (wgl_conf) {
      if (wgl_conf->base.ConfigID == config_count + 1)
                     }
      static void
   wgl_display_destroy(_EGLDisplay *disp)
   {
               st_screen_destroy(&wgl_dpy->base);
      }
      static int
   wgl_egl_st_get_param(struct pipe_frontend_screen *fscreen,
         {
      /* no-op */
      }
      static bool
   wgl_get_egl_image(struct pipe_frontend_screen *fscreen, void *image,
         {
      struct wgl_egl_image *wgl_img = (struct wgl_egl_image *)image;
   stw_translate_image(wgl_img->img, out);
      }
      static bool
   wgl_validate_egl_image(struct pipe_frontend_screen *fscreen, void *image)
   {
      struct wgl_egl_display *wgl_dpy = (struct wgl_egl_display *)fscreen;
   _EGLDisplay *disp = _eglLockDisplay(wgl_dpy->parent);
   _EGLImage *img = _eglLookupImage(image, disp);
            if (img == NULL) {
      _eglError(EGL_BAD_PARAMETER, "wgl_validate_egl_image");
                  }
      static EGLBoolean
   wgl_initialize_impl(_EGLDisplay *disp, HDC hdc)
   {
      struct wgl_egl_display *wgl_dpy;
            wgl_dpy = calloc(1, sizeof(*wgl_dpy));
   if (!wgl_dpy)
            disp->DriverData = (void *)wgl_dpy;
            if (!stw_init_screen(hdc)) {
      err = "wgl: failed to initialize screen";
               struct stw_device *stw_dev = stw_get_device();
            wgl_dpy->base.screen = stw_dev->screen;
   wgl_dpy->base.get_param = wgl_egl_st_get_param;
   wgl_dpy->base.get_egl_image = wgl_get_egl_image;
            disp->ClientAPIs = 0;
   if (_eglIsApiValid(EGL_OPENGL_API))
         if (_eglIsApiValid(EGL_OPENGL_ES_API))
      disp->ClientAPIs |=
         disp->Extensions.KHR_no_config_context = EGL_TRUE;
   disp->Extensions.KHR_surfaceless_context = EGL_TRUE;
            /* Report back to EGL the bitmask of priorities supported */
   disp->Extensions.IMG_context_priority = wgl_dpy->screen->get_param(
                     if (wgl_dpy->screen->is_format_supported(
         wgl_dpy->screen, PIPE_FORMAT_B8G8R8A8_SRGB, PIPE_TEXTURE_2D, 0, 0,
                     disp->Extensions.KHR_image_base = EGL_TRUE;
   disp->Extensions.KHR_gl_renderbuffer_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_2D_image = EGL_TRUE;
   disp->Extensions.KHR_gl_texture_cubemap_image = EGL_TRUE;
            disp->Extensions.KHR_fence_sync = EGL_TRUE;
   disp->Extensions.KHR_reusable_sync = EGL_TRUE;
            if (!wgl_add_configs(disp)) {
      err = "wgl: failed to add configs";
                     cleanup:
      wgl_display_destroy(disp);
      }
      static EGLBoolean
   wgl_initialize(_EGLDisplay *disp)
   {
      EGLBoolean ret = EGL_FALSE;
            /* In the case where the application calls eglMakeCurrent(context1),
   * eglTerminate, then eglInitialize again (without a call to eglReleaseThread
   * or eglMakeCurrent(NULL) before that), wgl_dpy structure is still
   * initialized, as we need it to be able to free context1 correctly.
   *
   * It would probably be safest to forcibly release the display with
   * wgl_display_release, to make sure the display is reinitialized correctly.
   * However, the EGL spec states that we need to keep a reference to the
   * current context (so we cannot call wgl_make_current(NULL)), and therefore
   * we would leak context1 as we would be missing the old display connection
   * to free it up correctly.
   */
   if (wgl_dpy) {
      p_atomic_inc(&wgl_dpy->ref_count);
               switch (disp->Platform) {
   case _EGL_PLATFORM_SURFACELESS:
      ret = wgl_initialize_impl(disp, NULL);
      case _EGL_PLATFORM_WINDOWS:
      ret = wgl_initialize_impl(disp, disp->PlatformDisplay);
      default:
      unreachable("Callers ensure we cannot get here.");
               if (!ret)
            wgl_dpy = wgl_egl_display(disp);
               }
      /**
   * Decrement display reference count, and free up display if necessary.
   */
   static void
   wgl_display_release(_EGLDisplay *disp)
   {
               if (!disp)
                     assert(wgl_dpy->ref_count > 0);
   if (!p_atomic_dec_zero(&wgl_dpy->ref_count))
            _eglCleanupDisplay(disp);
      }
      /**
   * Called via eglTerminate(), drv->Terminate().
   *
   * This must be guaranteed to be called exactly once, even if eglTerminate is
   * called many times (without a eglInitialize in between).
   */
   static EGLBoolean
   wgl_terminate(_EGLDisplay *disp)
   {
      /* Release all non-current Context/Surfaces. */
                        }
      /**
   * Called via eglCreateContext(), drv->CreateContext().
   */
   static _EGLContext *
   wgl_create_context(_EGLDisplay *disp, _EGLConfig *conf, _EGLContext *share_list,
         {
      struct wgl_egl_context *wgl_ctx;
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_context *wgl_ctx_shared = wgl_egl_context(share_list);
   struct stw_context *shared = wgl_ctx_shared ? wgl_ctx_shared->ctx : NULL;
   struct wgl_egl_config *wgl_config = wgl_egl_config(conf);
            wgl_ctx = malloc(sizeof(*wgl_ctx));
   if (!wgl_ctx) {
      _eglError(EGL_BAD_ALLOC, "eglCreateContext");
               if (!_eglInitContext(&wgl_ctx->base, disp, conf, share_list, attrib_list))
            unsigned profile_mask = 0;
   switch (wgl_ctx->base.ClientAPI) {
   case EGL_OPENGL_ES_API:
      profile_mask = WGL_CONTEXT_ES_PROFILE_BIT_EXT;
      case EGL_OPENGL_API:
      if ((wgl_ctx->base.ClientMajorVersion >= 4 ||
      (wgl_ctx->base.ClientMajorVersion == 3 &&
         wgl_ctx->base.Profile == EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR)
      else if (wgl_ctx->base.ClientMajorVersion == 3 &&
               else
            default:
      _eglError(EGL_BAD_PARAMETER, "eglCreateContext");
   free(wgl_ctx);
               if (conf != NULL) {
      /* The config chosen here isn't necessarily
   * used for surfaces later.
   * A pixmap surface will use the single config.
   * This opportunity depends on disabling the
   * doubleBufferMode check in
   * src/mesa/main/context.c:check_compatible()
   */
   if (wgl_config->stw_config[1])
         else
      } else
            unsigned flags = 0;
   if (wgl_ctx->base.Flags & EGL_CONTEXT_OPENGL_FORWARD_COMPATIBLE_BIT_KHR)
         if (wgl_ctx->base.Flags & EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR)
         unsigned resetStrategy = WGL_NO_RESET_NOTIFICATION_ARB;
   if (wgl_ctx->base.ResetNotificationStrategy != EGL_NO_RESET_NOTIFICATION)
         wgl_ctx->ctx = stw_create_context_attribs(
      disp->PlatformDisplay, 0, shared, &wgl_dpy->base,
   wgl_ctx->base.ClientMajorVersion, wgl_ctx->base.ClientMinorVersion, flags,
         if (!wgl_ctx->ctx)
                  cleanup:
      free(wgl_ctx);
      }
      /**
   * Called via eglDestroyContext(), drv->DestroyContext().
   */
   static EGLBoolean
   wgl_destroy_context(_EGLDisplay *disp, _EGLContext *ctx)
   {
               if (_eglPutContext(ctx)) {
      stw_destroy_context(wgl_ctx->ctx);
                  }
      static EGLBoolean
   wgl_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
               if (!_eglPutSurface(surf))
            if (wgl_surf->fb->owner == STW_FRAMEBUFFER_PBUFFER) {
         } else {
      struct stw_context *ctx = stw_current_context();
   stw_framebuffer_lock(wgl_surf->fb);
      }
      }
      static void
   wgl_gl_flush_get(_glapi_proc *glFlush)
   {
         }
      static void
   wgl_gl_flush()
   {
      static void (*glFlush)(void);
            util_call_once_data(&once, (util_call_once_data_func)wgl_gl_flush_get,
            /* if glFlush is not available things are horribly broken */
   if (!glFlush) {
      _eglLog(_EGL_WARNING, "wgl: failed to find glFlush entry point");
                  }
      /**
   * Called via eglMakeCurrent(), drv->MakeCurrent().
   */
   static EGLBoolean
   wgl_make_current(_EGLDisplay *disp, _EGLSurface *dsurf, _EGLSurface *rsurf,
         {
      struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   _EGLDisplay *old_disp = NULL;
   _EGLContext *old_ctx;
   _EGLSurface *old_dsurf, *old_rsurf;
   _EGLSurface *tmp_dsurf, *tmp_rsurf;
   struct stw_framebuffer *ddraw, *rdraw;
   struct stw_context *cctx;
            if (!wgl_dpy)
            /* make new bindings, set the EGL error otherwise */
   if (!_eglBindContext(ctx, dsurf, rsurf, &old_ctx, &old_dsurf, &old_rsurf))
            if (old_ctx) {
      struct stw_context *old_cctx = wgl_egl_context(old_ctx)->ctx;
            /* flush before context switch */
                        ddraw = (dsurf) ? wgl_egl_surface(dsurf)->fb : NULL;
   rdraw = (rsurf) ? wgl_egl_surface(rsurf)->fb : NULL;
            if (cctx || ddraw || rdraw) {
      if (!stw_make_current(ddraw, rdraw, cctx)) {
               /* stw_make_current failed. We cannot tell for sure why, but
   * setting the error to EGL_BAD_MATCH is surely better than leaving it
   * as EGL_SUCCESS.
                  /* undo the previous _eglBindContext */
   _eglBindContext(old_ctx, old_dsurf, old_rsurf, &ctx, &tmp_dsurf,
                        _eglPutSurface(dsurf);
                  _eglPutSurface(old_dsurf);
                  ddraw = (old_dsurf) ? wgl_egl_surface(old_dsurf)->fb : NULL;
                  /* undo the previous wgl_dpy->core->unbindContext */
   if (stw_make_current(ddraw, rdraw, cctx)) {
                  /* We cannot restore the same state as it was before calling
   * eglMakeCurrent() and the spec isn't clear about what to do. We
   * can prevent EGL from calling into the DRI driver with no DRI
   * context bound.
   */
                  _eglBindContext(ctx, dsurf, rsurf, &tmp_ctx, &tmp_dsurf, &tmp_rsurf);
                     } else {
      /* wgl_dpy->core->bindContext succeeded, so take a reference on the
   * wgl_dpy. This prevents wgl_dpy from being reinitialized when a
   * EGLDisplay is terminated and then initialized again while a
   * context is still bound. See wgl_initialize() for a more in depth
   * explanation. */
                  wgl_destroy_surface(disp, old_dsurf);
            if (old_ctx) {
      wgl_destroy_context(disp, old_ctx);
               if (egl_error != EGL_SUCCESS)
               }
      static _EGLSurface *
   wgl_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
            struct wgl_egl_surface *wgl_surf = calloc(1, sizeof(*wgl_surf));
   if (!wgl_surf)
            if (!_eglInitSurface(&wgl_surf->base, disp, EGL_WINDOW_BIT, conf,
            free(wgl_surf);
               const struct stw_pixelformat_info *stw_conf = wgl_conf->stw_config[1]
               wgl_surf->fb = stw_framebuffer_create(
         if (!wgl_surf->fb) {
      free(wgl_surf);
               wgl_surf->fb->swap_interval = 1;
               }
      static _EGLSurface *
   wgl_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
            struct wgl_egl_surface *wgl_surf = calloc(1, sizeof(*wgl_surf));
   if (!wgl_surf)
            if (!_eglInitSurface(&wgl_surf->base, disp, EGL_PBUFFER_BIT, conf,
            free(wgl_surf);
               const struct stw_pixelformat_info *stw_conf = wgl_conf->stw_config[1]
               wgl_surf->fb = stw_pbuffer_create(stw_conf, wgl_surf->base.Width,
         if (!wgl_surf->fb) {
      free(wgl_surf);
               wgl_surf->fb->swap_interval = 1;
               }
      static EGLBoolean
   wgl_query_surface(_EGLDisplay *disp, _EGLSurface *surf, EGLint attribute,
         {
      struct wgl_egl_surface *wgl_surf = wgl_egl_surface(surf);
            switch (attribute) {
   case EGL_WIDTH:
   case EGL_HEIGHT:
      if (GetClientRect(wgl_surf->fb->hWnd, &client_rect)) {
      surf->Width = client_rect.right;
      }
      default:
         }
      }
      static EGLBoolean
   wgl_bind_tex_image(_EGLDisplay *disp, _EGLSurface *surf, EGLint buffer)
   {
               _EGLContext *ctx = _eglGetCurrentContext();
            if (!_eglBindTexImage(disp, surf, buffer))
            struct pipe_resource *pres = stw_get_framebuffer_resource(
                  switch (surf->TextureFormat) {
   case EGL_TEXTURE_RGB:
      switch (format) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
      format = PIPE_FORMAT_R16G16B16X16_FLOAT;
      case PIPE_FORMAT_B10G10R10A2_UNORM:
      format = PIPE_FORMAT_B10G10R10X2_UNORM;
      case PIPE_FORMAT_R10G10B10A2_UNORM:
      format = PIPE_FORMAT_R10G10B10X2_UNORM;
      case PIPE_FORMAT_BGRA8888_UNORM:
      format = PIPE_FORMAT_BGRX8888_UNORM;
      case PIPE_FORMAT_ARGB8888_UNORM:
      format = PIPE_FORMAT_XRGB8888_UNORM;
      default:
         }
      case EGL_TEXTURE_RGBA:
         default:
                  switch (surf->TextureTarget) {
   case EGL_TEXTURE_2D:
         default:
                              }
      static EGLBoolean
   wgl_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct wgl_egl_surface *wgl_surf = wgl_egl_surface(surf);
   wgl_surf->fb->swap_interval = interval;
      }
      static EGLBoolean
   wgl_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
               stw_framebuffer_lock(wgl_surf->fb);
   HDC hdc = GetDC(wgl_surf->fb->hWnd);
   BOOL ret = stw_framebuffer_swap_locked(hdc, wgl_surf->fb);
               }
      static EGLBoolean
   wgl_wait_client(_EGLDisplay *disp, _EGLContext *ctx)
   {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   struct pipe_fence_handle *fence = NULL;
   st_context_flush(wgl_ctx->ctx->st, ST_FLUSH_END_OF_FRAME | ST_FLUSH_WAIT,
            }
      static EGLBoolean
   wgl_wait_native(EGLint engine)
   {
      if (engine != EGL_CORE_NATIVE_ENGINE)
         /* It's unclear what "native" means, but GDI is as good a guess as any */
   GdiFlush();
      }
      static EGLint
   egl_error_from_stw_image_error(enum stw_image_error err)
   {
      switch (err) {
   case STW_IMAGE_ERROR_SUCCESS:
         case STW_IMAGE_ERROR_BAD_ALLOC:
         case STW_IMAGE_ERROR_BAD_MATCH:
         case STW_IMAGE_ERROR_BAD_PARAMETER:
         case STW_IMAGE_ERROR_BAD_ACCESS:
         default:
      assert(!"unknown stw_image_error code");
         }
      static _EGLImage *
   wgl_create_image_khr_texture(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   struct wgl_egl_image *wgl_img;
   GLuint texture = (GLuint)(uintptr_t)buffer;
   _EGLImageAttribs attrs;
   GLuint depth;
   GLenum gl_target;
            if (texture == 0) {
      _eglError(EGL_BAD_PARAMETER, "wgl_create_image_khr");
               if (!_eglParseImageAttribList(&attrs, disp, attr_list))
            switch (target) {
   case EGL_GL_TEXTURE_2D_KHR:
      depth = 0;
   gl_target = GL_TEXTURE_2D;
      case EGL_GL_TEXTURE_3D_KHR:
      depth = attrs.GLTextureZOffset;
   gl_target = GL_TEXTURE_3D;
      case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_X_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_Z_KHR:
   case EGL_GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_KHR:
      depth = target - EGL_GL_TEXTURE_CUBE_MAP_POSITIVE_X_KHR;
   gl_target = GL_TEXTURE_CUBE_MAP;
      default:
      unreachable("Unexpected target in wgl_create_image_khr_texture()");
               wgl_img = malloc(sizeof *wgl_img);
   if (!wgl_img) {
      _eglError(EGL_BAD_ALLOC, "wgl_create_image_khr");
                        wgl_img->img = stw_create_image_from_texture(
                  if (!wgl_img->img) {
      free(wgl_img);
   _eglError(egl_error_from_stw_image_error(error), "wgl_create_image_khr");
      }
      }
      static _EGLImage *
   wgl_create_image_khr_renderbuffer(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
   struct wgl_egl_image *wgl_img;
   GLuint renderbuffer = (GLuint)(uintptr_t)buffer;
            if (renderbuffer == 0) {
      _eglError(EGL_BAD_PARAMETER, "wgl_create_image_khr");
               wgl_img = malloc(sizeof(*wgl_img));
   if (!wgl_img) {
      _eglError(EGL_BAD_ALLOC, "wgl_create_image");
                        wgl_img->img =
                  if (!wgl_img->img) {
      free(wgl_img);
   _eglError(egl_error_from_stw_image_error(error), "wgl_create_image_khr");
                  }
      static _EGLImage *
   wgl_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
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
         case EGL_GL_RENDERBUFFER_KHR:
         default:
      _eglError(EGL_BAD_PARAMETER, "wgl_create_image_khr");
         }
      static EGLBoolean
   wgl_destroy_image_khr(_EGLDisplay *disp, _EGLImage *img)
   {
      struct wgl_egl_image *wgl_img = wgl_egl_image(img);
   stw_destroy_image(wgl_img->img);
   free(wgl_img);
      }
      static _EGLSync *
   wgl_create_sync_khr(_EGLDisplay *disp, EGLenum type,
         {
         _EGLContext *ctx = _eglGetCurrentContext();
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
                     wgl_sync = calloc(1, sizeof(struct wgl_egl_sync));
   if (!wgl_sync) {
      _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
               if (!_eglInitSync(&wgl_sync->base, disp, type, attrib_list)) {
      free(wgl_sync);
               switch (type) {
   case EGL_SYNC_FENCE_KHR:
      st_context_flush(st, 0, &wgl_sync->fence, NULL, NULL);
   if (!wgl_sync->fence) {
      _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
   free(wgl_sync);
      }
         case EGL_SYNC_REUSABLE_KHR:
      wgl_sync->event = CreateEvent(NULL, true, false, NULL);
   if (!wgl_sync->event) {
      _eglError(EGL_BAD_ALLOC, "eglCreateSyncKHR");
   free(wgl_sync);
                  wgl_sync->refcount = 1;
      }
      static void
   wgl_egl_unref_sync(struct wgl_egl_display *wgl_dpy,
         {
      if (InterlockedDecrement((volatile LONG *)&wgl_sync->refcount) > 0)
            if (wgl_sync->fence)
         if (wgl_sync->event)
            }
      static EGLBoolean
   wgl_destroy_sync_khr(_EGLDisplay *disp, _EGLSync *sync)
   {
      struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_sync *wgl_sync = wgl_egl_sync(sync);
   wgl_egl_unref_sync(wgl_dpy, wgl_sync);
      }
      static EGLint
   wgl_client_wait_sync_khr(_EGLDisplay *disp, _EGLSync *sync, EGLint flags,
         {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct wgl_egl_display *wgl_dpy = wgl_egl_display(disp);
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
                     /* the sync object should take a reference while waiting */
            switch (sync->Type) {
   case EGL_SYNC_FENCE_KHR:
      if (wgl_dpy->screen->fence_finish(wgl_dpy->screen, NULL, wgl_sync->fence,
               else
               case EGL_SYNC_REUSABLE_KHR:
      if (wgl_ctx && wgl_sync->base.SyncStatus == EGL_UNSIGNALED_KHR &&
      (flags & EGL_SYNC_FLUSH_COMMANDS_BIT_KHR)) {
   /* flush context if EGL_SYNC_FLUSH_COMMANDS_BIT_KHR is set */
               DWORD wait_milliseconds = (timeout == EGL_FOREVER_KHR)
               DWORD wait_ret = WaitForSingleObject(wgl_sync->event, wait_milliseconds);
   switch (wait_ret) {
   case WAIT_OBJECT_0:
      assert(wgl_sync->base.SyncStatus == EGL_SIGNALED_KHR);
      case WAIT_TIMEOUT:
      assert(wgl_sync->base.SyncStatus == EGL_UNSIGNALED_KHR);
   ret = EGL_TIMEOUT_EXPIRED_KHR;
      default:
      _eglError(EGL_BAD_ACCESS, "eglClientWaitSyncKHR");
   ret = EGL_FALSE;
      }
      }
               }
      static EGLint
   wgl_wait_sync_khr(_EGLDisplay *disp, _EGLSync *sync)
   {
      _EGLContext *ctx = _eglGetCurrentContext();
   struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
            if (!wgl_sync->fence)
            struct pipe_context *pipe = wgl_ctx->ctx->st->pipe;
   if (pipe->fence_server_sync)
               }
      static EGLBoolean
   wgl_signal_sync_khr(_EGLDisplay *disp, _EGLSync *sync, EGLenum mode)
   {
               if (sync->Type != EGL_SYNC_REUSABLE_KHR)
            if (mode != EGL_SIGNALED_KHR && mode != EGL_UNSIGNALED_KHR)
                     if (mode == EGL_SIGNALED_KHR) {
      if (!SetEvent(wgl_sync->event))
      } else {
      if (!ResetEvent(wgl_sync->event))
                  }
      static const char *
   wgl_query_driver_name(_EGLDisplay *disp)
   {
         }
      static char *
   wgl_query_driver_config(_EGLDisplay *disp)
   {
         }
      static int
   wgl_interop_query_device_info(_EGLDisplay *disp, _EGLContext *ctx,
         {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
      }
      static int
   wgl_interop_export_object(_EGLDisplay *disp, _EGLContext *ctx,
               {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
      }
      static int
   wgl_interop_flush_objects(_EGLDisplay *disp, _EGLContext *ctx, unsigned count,
               {
      struct wgl_egl_context *wgl_ctx = wgl_egl_context(ctx);
      }
      struct _egl_driver _eglDriver = {
      .Initialize = wgl_initialize,
   .Terminate = wgl_terminate,
   .CreateContext = wgl_create_context,
   .DestroyContext = wgl_destroy_context,
   .MakeCurrent = wgl_make_current,
   .CreateWindowSurface = wgl_create_window_surface,
   .CreatePbufferSurface = wgl_create_pbuffer_surface,
   .DestroySurface = wgl_destroy_surface,
   .QuerySurface = wgl_query_surface,
   .BindTexImage = wgl_bind_tex_image,
   .ReleaseTexImage = _eglReleaseTexImage,
   .SwapInterval = wgl_swap_interval,
   .SwapBuffers = wgl_swap_buffers,
   .WaitClient = wgl_wait_client,
   .WaitNative = wgl_wait_native,
   .CreateImageKHR = wgl_create_image_khr,
   .DestroyImageKHR = wgl_destroy_image_khr,
   .CreateSyncKHR = wgl_create_sync_khr,
   .DestroySyncKHR = wgl_destroy_sync_khr,
   .ClientWaitSyncKHR = wgl_client_wait_sync_khr,
   .WaitSyncKHR = wgl_wait_sync_khr,
   .SignalSyncKHR = wgl_signal_sync_khr,
   .QueryDriverName = wgl_query_driver_name,
   .QueryDriverConfig = wgl_query_driver_config,
   .GLInteropQueryDeviceInfo = wgl_interop_query_device_info,
   .GLInteropExportObject = wgl_interop_export_object,
      };
