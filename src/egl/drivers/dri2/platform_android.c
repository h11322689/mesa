   /*
   * Mesa 3-D graphics library
   *
   * Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
   * Copyright (C) 2010-2011 LunarG Inc.
   *
   * Based on platform_x11, which has
   *
   * Copyright © 2011 Intel Corporation
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
      #include <dirent.h>
   #include <dlfcn.h>
   #include <errno.h>
   #include <fcntl.h>
   #include <stdbool.h>
   #include <stdio.h>
   #include <xf86drm.h>
   #include <cutils/properties.h>
   #include <drm-uapi/drm_fourcc.h>
   #include <sync/sync.h>
   #include <sys/types.h>
      #include "util/compiler.h"
   #include "util/libsync.h"
   #include "util/os_file.h"
      #include "egl_dri2.h"
   #include "eglglobals.h"
   #include "loader.h"
   #include "platform_android.h"
      #define ALIGN(val, align) (((val) + (align)-1) & ~((align)-1))
      enum chroma_order {
      YCbCr,
      };
      struct droid_yuv_format {
      /* Lookup keys */
   int native;                     /* HAL_PIXEL_FORMAT_ */
   enum chroma_order chroma_order; /* chroma order is {Cb, Cr} or {Cr, Cb} */
            /* Result */
      };
      /* The following table is used to look up a DRI image FourCC based
   * on native format and information contained in android_ycbcr struct. */
   static const struct droid_yuv_format droid_yuv_formats[] = {
      /* Native format, YCrCb, Chroma step, DRI image FourCC */
   {HAL_PIXEL_FORMAT_YCbCr_420_888, YCbCr, 2, DRM_FORMAT_NV12},
   {HAL_PIXEL_FORMAT_YCbCr_420_888, YCbCr, 1, DRM_FORMAT_YUV420},
   {HAL_PIXEL_FORMAT_YCbCr_420_888, YCrCb, 1, DRM_FORMAT_YVU420},
   {HAL_PIXEL_FORMAT_YV12, YCrCb, 1, DRM_FORMAT_YVU420},
   /* HACK: See droid_create_image_from_prime_fds() and
   * https://issuetracker.google.com/32077885. */
   {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCbCr, 2, DRM_FORMAT_NV12},
   {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCbCr, 1, DRM_FORMAT_YUV420},
   {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCrCb, 1, DRM_FORMAT_YVU420},
   {HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED, YCrCb, 1, DRM_FORMAT_AYUV},
      };
      static int
   get_fourcc_yuv(int native, enum chroma_order chroma_order, int chroma_step)
   {
      for (int i = 0; i < ARRAY_SIZE(droid_yuv_formats); ++i)
      if (droid_yuv_formats[i].native == native &&
      droid_yuv_formats[i].chroma_order == chroma_order &&
               }
      static bool
   is_yuv(int native)
   {
      for (int i = 0; i < ARRAY_SIZE(droid_yuv_formats); ++i)
      if (droid_yuv_formats[i].native == native)
            }
      static int
   get_format_bpp(int native)
   {
               switch (native) {
   case HAL_PIXEL_FORMAT_RGBA_FP16:
      bpp = 8;
      case HAL_PIXEL_FORMAT_RGBA_8888:
   case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
      /*
   * HACK: Hardcode this to RGBX_8888 as per cros_gralloc hack.
   * TODO: Remove this once https://issuetracker.google.com/32077885 is
   * fixed.
      case HAL_PIXEL_FORMAT_RGBX_8888:
   case HAL_PIXEL_FORMAT_BGRA_8888:
   case HAL_PIXEL_FORMAT_RGBA_1010102:
      bpp = 4;
      case HAL_PIXEL_FORMAT_RGB_565:
      bpp = 2;
      default:
      bpp = 0;
                  }
      /* createImageFromFds requires fourcc format */
   static int
   get_fourcc(int native)
   {
      switch (native) {
   case HAL_PIXEL_FORMAT_RGB_565:
         case HAL_PIXEL_FORMAT_BGRA_8888:
         case HAL_PIXEL_FORMAT_RGBA_8888:
         case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
      /*
   * HACK: Hardcode this to RGBX_8888 as per cros_gralloc hack.
   * TODO: Remove this once https://issuetracker.google.com/32077885 is
   * fixed.
      case HAL_PIXEL_FORMAT_RGBX_8888:
         case HAL_PIXEL_FORMAT_RGBA_FP16:
         case HAL_PIXEL_FORMAT_RGBA_1010102:
         default:
         }
      }
      /* returns # of fds, and by reference the actual fds */
   static unsigned
   get_native_buffer_fds(struct ANativeWindowBuffer *buf, int fds[3])
   {
               if (!handle)
            /*
   * Various gralloc implementations exist, but the dma-buf fd tends
   * to be first. Access it directly to avoid a dependency on specific
   * gralloc versions.
   */
   for (int i = 0; i < handle->numFds; i++)
               }
      static int
   get_yuv_buffer_info(struct dri2_egl_display *dri2_dpy,
               {
      struct android_ycbcr ycbcr;
   enum chroma_order chroma_order;
   int drm_fourcc = 0;
   int num_fds = 0;
   int fds[3];
            num_fds = get_native_buffer_fds(buf, fds);
   if (num_fds == 0)
            if (!dri2_dpy->gralloc->lock_ycbcr) {
      _eglLog(_EGL_WARNING, "Gralloc does not support lock_ycbcr");
               memset(&ycbcr, 0, sizeof(ycbcr));
   ret = dri2_dpy->gralloc->lock_ycbcr(dri2_dpy->gralloc, buf->handle, 0, 0, 0,
         if (ret) {
      /* HACK: See native_window_buffer_get_buffer_info() and
   * https://issuetracker.google.com/32077885.*/
   if (buf->format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            _eglLog(_EGL_WARNING, "gralloc->lock_ycbcr failed: %d", ret);
      }
                     /* .chroma_step is the byte distance between the same chroma channel
   * values of subsequent pixels, assumed to be the same for Cb and Cr. */
   drm_fourcc = get_fourcc_yuv(buf->format, chroma_order, ycbcr.chroma_step);
   if (drm_fourcc == -1) {
      _eglLog(_EGL_WARNING,
         "unsupported YUV format, native = %x, chroma_order = %s, "
   "chroma_step = %d",
   buf->format, chroma_order == YCbCr ? "YCbCr" : "YCrCb",
               *out_buf_info = (struct buffer_info){
      .width = buf->width,
   .height = buf->height,
   .drm_fourcc = drm_fourcc,
   .num_planes = ycbcr.chroma_step == 2 ? 2 : 3,
   .fds = {-1, -1, -1, -1},
   .modifier = DRM_FORMAT_MOD_INVALID,
   .yuv_color_space = EGL_ITU_REC601_EXT,
   .sample_range = EGL_YUV_NARROW_RANGE_EXT,
   .horizontal_siting = EGL_YUV_CHROMA_SITING_0_EXT,
               /* When lock_ycbcr's usage argument contains no SW_READ/WRITE flags
   * it will return the .y/.cb/.cr pointers based on a NULL pointer,
   * so they can be interpreted as offsets. */
   out_buf_info->offsets[0] = (size_t)ycbcr.y;
   /* We assume here that all the planes are located in one DMA-buf. */
   if (chroma_order == YCrCb) {
      out_buf_info->offsets[1] = (size_t)ycbcr.cr;
      } else {
      out_buf_info->offsets[1] = (size_t)ycbcr.cb;
               /* .ystride is the line length (in bytes) of the Y plane,
   * .cstride is the line length (in bytes) of any of the remaining
   * Cb/Cr/CbCr planes, assumed to be the same for Cb and Cr for fully
   * planar formats. */
   out_buf_info->pitches[0] = ycbcr.ystride;
            /*
   * Since this is EGL_NATIVE_BUFFER_ANDROID don't assume that
   * the single-fd case cannot happen.  So handle eithe single
   * fd or fd-per-plane case:
   */
   if (num_fds == 1) {
      out_buf_info->fds[1] = out_buf_info->fds[0] = fds[0];
   if (out_buf_info->num_planes == 3)
      } else {
      assert(num_fds == out_buf_info->num_planes);
   out_buf_info->fds[0] = fds[0];
   out_buf_info->fds[1] = fds[1];
                  }
      static int
   native_window_buffer_get_buffer_info(struct dri2_egl_display *dri2_dpy,
               {
      int num_planes = 0;
   int drm_fourcc = 0;
   int pitch = 0;
            if (is_yuv(buf->format)) {
      int ret = get_yuv_buffer_info(dri2_dpy, buf, out_buf_info);
   /*
   * HACK: https://issuetracker.google.com/32077885
   * There is no API available to properly query the IMPLEMENTATION_DEFINED
   * format. As a workaround we rely here on gralloc allocating either
   * an arbitrary YCbCr 4:2:0 or RGBX_8888, with the latter being recognized
   * by lock_ycbcr failing.
   */
   if (ret != -EAGAIN)
               /*
   * Non-YUV formats could *also* have multiple planes, such as ancillary
   * color compression state buffer, but the rest of the code isn't ready
   * yet to deal with modifiers:
   */
   num_planes = get_native_buffer_fds(buf, fds);
   if (num_planes == 0)
                     drm_fourcc = get_fourcc(buf->format);
   if (drm_fourcc == -1) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
               pitch = buf->stride * get_format_bpp(buf->format);
   if (pitch == 0) {
      _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
               *out_buf_info = (struct buffer_info){
      .width = buf->width,
   .height = buf->height,
   .drm_fourcc = drm_fourcc,
   .num_planes = num_planes,
   .fds = {fds[0], -1, -1, -1},
   .modifier = DRM_FORMAT_MOD_INVALID,
   .offsets = {0, 0, 0, 0},
   .pitches = {pitch, 0, 0, 0},
   .yuv_color_space = EGL_ITU_REC601_EXT,
   .sample_range = EGL_YUV_NARROW_RANGE_EXT,
   .horizontal_siting = EGL_YUV_CHROMA_SITING_0_EXT,
                  }
      /* More recent CrOS gralloc has a perform op that fills out the struct below
   * with canonical information about the buffer and its modifier, planes,
   * offsets and strides.  If we have this, we can skip straight to
   * createImageFromDmaBufs2() and avoid all the guessing and recalculations.
   * This also gives us the modifier and plane offsets/strides for multiplanar
   * compressed buffers (eg Intel CCS buffers) in order to make that work in
   * Android.
   */
      static const char cros_gralloc_module_name[] = "CrOS Gralloc";
      #define CROS_GRALLOC_DRM_GET_BUFFER_INFO               4
   #define CROS_GRALLOC_DRM_GET_USAGE                     5
   #define CROS_GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT 0x1
      struct cros_gralloc0_buffer_info {
      uint32_t drm_fourcc;
   int num_fds;
   int fds[4];
   uint64_t modifier;
   int offset[4];
      };
      static int
   cros_get_buffer_info(struct dri2_egl_display *dri2_dpy,
               {
               if (strcmp(dri2_dpy->gralloc->common.name, cros_gralloc_module_name) == 0 &&
      dri2_dpy->gralloc->perform &&
   dri2_dpy->gralloc->perform(dri2_dpy->gralloc,
               *out_buf_info = (struct buffer_info){
      .width = buf->width,
   .height = buf->height,
   .drm_fourcc = info.drm_fourcc,
   .num_planes = info.num_fds,
   .fds = {-1, -1, -1, -1},
   .modifier = info.modifier,
   .yuv_color_space = EGL_ITU_REC601_EXT,
   .sample_range = EGL_YUV_NARROW_RANGE_EXT,
   .horizontal_siting = EGL_YUV_CHROMA_SITING_0_EXT,
      };
   for (int i = 0; i < out_buf_info->num_planes; i++) {
      out_buf_info->fds[i] = info.fds[i];
   out_buf_info->offsets[i] = info.offset[i];
                              }
      static __DRIimage *
   droid_create_image_from_buffer_info(struct dri2_egl_display *dri2_dpy,
         {
               if (dri2_dpy->image->base.version >= 15 &&
      dri2_dpy->image->createImageFromDmaBufs2 != NULL) {
   return dri2_dpy->image->createImageFromDmaBufs2(
      dri2_dpy->dri_screen_render_gpu, buf_info->width, buf_info->height,
   buf_info->drm_fourcc, buf_info->modifier, buf_info->fds,
   buf_info->num_planes, buf_info->pitches, buf_info->offsets,
   buf_info->yuv_color_space, buf_info->sample_range,
            return dri2_dpy->image->createImageFromDmaBufs(
      dri2_dpy->dri_screen_render_gpu, buf_info->width, buf_info->height,
   buf_info->drm_fourcc, buf_info->fds, buf_info->num_planes,
   buf_info->pitches, buf_info->offsets, buf_info->yuv_color_space,
   buf_info->sample_range, buf_info->horizontal_siting,
   }
      static __DRIimage *
   droid_create_image_from_native_buffer(_EGLDisplay *disp,
               {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct buffer_info buf_info;
            /* If dri driver is gallium virgl, real modifier info queried back from
   * CrOS info (and potentially mapper metadata if integrated later) cannot
   * get resolved and the buffer import will fail. Thus the fallback behavior
   * is preserved down to native_window_buffer_get_buffer_info() so that the
   * buffer can be imported without modifier info as a last resort.
   */
   if (!img && !mapper_metadata_get_buffer_info(buf, &buf_info))
            if (!img && !cros_get_buffer_info(dri2_dpy, buf, &buf_info))
            if (!img && !native_window_buffer_get_buffer_info(dri2_dpy, buf, &buf_info))
               }
      static void
   handle_in_fence_fd(struct dri2_egl_surface *dri2_surf, __DRIimage *img)
   {
      _EGLDisplay *disp = dri2_surf->base.Resource.Display;
            if (dri2_surf->in_fence_fd < 0)
                     if (dri2_dpy->image->base.version >= 21 &&
      dri2_dpy->image->setInFenceFd != NULL) {
      } else {
            }
      static void
   close_in_fence_fd(struct dri2_egl_surface *dri2_surf)
   {
      validate_fence_fd(dri2_surf->in_fence_fd);
   if (dri2_surf->in_fence_fd >= 0)
            }
      static EGLBoolean
   droid_window_dequeue_buffer(struct dri2_egl_surface *dri2_surf)
   {
               if (ANativeWindow_dequeueBuffer(dri2_surf->window, &dri2_surf->buffer,
                                             /* Record all the buffers created by ANativeWindow and update back buffer
   * for updating buffer's age in swap_buffers.
   */
   EGLBoolean updated = EGL_FALSE;
   for (int i = 0; i < dri2_surf->color_buffers_count; i++) {
      if (!dri2_surf->color_buffers[i].buffer) {
         }
   if (dri2_surf->color_buffers[i].buffer == dri2_surf->buffer) {
      dri2_surf->back = &dri2_surf->color_buffers[i];
   updated = EGL_TRUE;
                  if (!updated) {
      /* In case of all the buffers were recreated by ANativeWindow, reset
   * the color_buffers
   */
   for (int i = 0; i < dri2_surf->color_buffers_count; i++) {
      dri2_surf->color_buffers[i].buffer = NULL;
      }
   dri2_surf->color_buffers[0].buffer = dri2_surf->buffer;
                  }
      static EGLBoolean
   droid_window_enqueue_buffer(_EGLDisplay *disp,
         {
               /* Queue the buffer with stored out fence fd. The ANativeWindow or buffer
   * consumer may choose to wait for the fence to signal before accessing
   * it. If fence fd value is -1, buffer can be accessed by consumer
   * immediately. Consumer or application shouldn't rely on timestamp
   * associated with fence if the fence fd is -1.
   *
   * Ownership of fd is transferred to consumer after queueBuffer and the
   * consumer is responsible for closing it. Caller must not use the fd
   * after passing it to queueBuffer.
   */
   int fence_fd = dri2_surf->out_fence_fd;
   dri2_surf->out_fence_fd = -1;
            dri2_surf->buffer = NULL;
            if (dri2_surf->dri_image_back) {
      dri2_dpy->image->destroyImage(dri2_surf->dri_image_back);
                  }
      static void
   droid_window_cancel_buffer(struct dri2_egl_surface *dri2_surf)
   {
      int ret;
            dri2_surf->out_fence_fd = -1;
   ret = ANativeWindow_cancelBuffer(dri2_surf->window, dri2_surf->buffer,
         dri2_surf->buffer = NULL;
   if (ret < 0) {
      _eglLog(_EGL_WARNING, "ANativeWindow_cancelBuffer failed");
                  }
      static bool
   droid_set_shared_buffer_mode(_EGLDisplay *disp, _EGLSurface *surf, bool mode)
   {
   #if ANDROID_API_LEVEL >= 24
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
            assert(surf->Type == EGL_WINDOW_BIT);
                     if (ANativeWindow_setSharedBufferMode(window, mode)) {
      _eglLog(_EGL_WARNING,
         "failed ANativeWindow_setSharedBufferMode"
   "(window=%p, mode=%d)",
               if (mode)
         else
            if (ANativeWindow_setUsage(window, dri2_surf->gralloc_usage)) {
      _eglLog(_EGL_WARNING,
         "failed ANativeWindow_setUsage(window=%p, usage=%u)", window,
                  #else
      _eglLog(_EGL_FATAL, "%s:%d: internal error: unreachable", __FILE__,
            #endif
   }
      static _EGLSurface *
   droid_create_surface(_EGLDisplay *disp, EGLint type, _EGLConfig *conf,
         {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_config *dri2_conf = dri2_egl_config(conf);
   struct dri2_egl_surface *dri2_surf;
   struct ANativeWindow *window = native_window;
            dri2_surf = calloc(1, sizeof *dri2_surf);
   if (!dri2_surf) {
      _eglError(EGL_BAD_ALLOC, "droid_create_surface");
                        if (!dri2_init_surface(&dri2_surf->base, disp, type, conf, attrib_list, true,
                  if (type == EGL_WINDOW_BIT) {
      int format;
   int buffer_count;
            format = ANativeWindow_getFormat(window);
   if (format < 0) {
      _eglError(EGL_BAD_NATIVE_WINDOW, "droid_create_surface");
               /* Query ANativeWindow for MIN_UNDEQUEUED_BUFFER, minimum amount
   * of undequeued buffers.
   */
   if (ANativeWindow_query(window,
                  _eglError(EGL_BAD_NATIVE_WINDOW, "droid_create_surface");
               /* Required buffer caching slots. */
            dri2_surf->color_buffers =
         if (!dri2_surf->color_buffers) {
      _eglError(EGL_BAD_ALLOC, "droid_create_surface");
      }
            if (format != dri2_conf->base.NativeVisualID) {
      _eglLog(_EGL_WARNING, "Native format mismatch: 0x%x != 0x%x", format,
               ANativeWindow_query(window, ANATIVEWINDOW_QUERY_DEFAULT_WIDTH,
         ANativeWindow_query(window, ANATIVEWINDOW_QUERY_DEFAULT_HEIGHT,
            dri2_surf->gralloc_usage =
      strcmp(dri2_dpy->driver_name, "kms_swrast") == 0
               if (dri2_surf->base.ActiveRenderBuffer == EGL_SINGLE_BUFFER)
            if (ANativeWindow_setUsage(window, dri2_surf->gralloc_usage)) {
      _eglError(EGL_BAD_NATIVE_WINDOW, "droid_create_surface");
                  config = dri2_get_dri_config(dri2_conf, type, dri2_surf->base.GLColorspace);
   if (!config) {
      _eglError(EGL_BAD_MATCH,
                     if (!dri2_create_drawable(dri2_dpy, config, dri2_surf, dri2_surf))
            if (window) {
      ANativeWindow_acquire(window);
                     cleanup_surface:
      if (dri2_surf->color_buffers_count)
                     }
      static _EGLSurface *
   droid_create_window_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
      return droid_create_surface(disp, EGL_WINDOW_BIT, conf, native_window,
      }
      static _EGLSurface *
   droid_create_pbuffer_surface(_EGLDisplay *disp, _EGLConfig *conf,
         {
         }
      static EGLBoolean
   droid_destroy_surface(_EGLDisplay *disp, _EGLSurface *surf)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
                     if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      if (dri2_surf->buffer)
                        if (dri2_surf->dri_image_back) {
      _eglLog(_EGL_DEBUG, "%s : %d : destroy dri_image_back", __func__,
         dri2_dpy->image->destroyImage(dri2_surf->dri_image_back);
               if (dri2_surf->dri_image_front) {
      _eglLog(_EGL_DEBUG, "%s : %d : destroy dri_image_front", __func__,
         dri2_dpy->image->destroyImage(dri2_surf->dri_image_front);
                        close_in_fence_fd(dri2_surf);
   dri2_fini_surface(surf);
   free(dri2_surf->color_buffers);
               }
      static EGLBoolean
   droid_swap_interval(_EGLDisplay *disp, _EGLSurface *surf, EGLint interval)
   {
      struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
            if (ANativeWindow_setSwapInterval(window, interval))
            surf->SwapInterval = interval;
      }
      static int
   update_buffers(struct dri2_egl_surface *dri2_surf)
   {
      if (dri2_surf->base.Lost)
            if (dri2_surf->base.Type != EGL_WINDOW_BIT)
            /* try to dequeue the next back buffer */
   if (!dri2_surf->buffer && !droid_window_dequeue_buffer(dri2_surf)) {
      _eglLog(_EGL_WARNING, "Could not dequeue buffer from native window");
   dri2_surf->base.Lost = EGL_TRUE;
               /* free outdated buffers and update the surface size */
   if (dri2_surf->base.Width != dri2_surf->buffer->width ||
      dri2_surf->base.Height != dri2_surf->buffer->height) {
   dri2_egl_surface_free_local_buffers(dri2_surf);
   dri2_surf->base.Width = dri2_surf->buffer->width;
                  }
      static int
   get_front_bo(struct dri2_egl_surface *dri2_surf, unsigned int format)
   {
      struct dri2_egl_display *dri2_dpy =
            if (dri2_surf->dri_image_front)
            if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      /* According current EGL spec, front buffer rendering
   * for window surface is not supported now.
   * and mesa doesn't have the implementation of this case.
   * Add warning message, but not treat it as error.
   */
   _eglLog(
      _EGL_DEBUG,
   } else if (dri2_surf->base.Type == EGL_PBUFFER_BIT) {
      dri2_surf->dri_image_front = dri2_dpy->image->createImage(
      dri2_dpy->dri_screen_render_gpu, dri2_surf->base.Width,
      if (!dri2_surf->dri_image_front) {
      _eglLog(_EGL_WARNING, "dri2_image_front allocation failed");
                     }
      static int
   get_back_bo(struct dri2_egl_surface *dri2_surf)
   {
               if (dri2_surf->dri_image_back)
            if (dri2_surf->base.Type == EGL_WINDOW_BIT) {
      if (!dri2_surf->buffer) {
      _eglLog(_EGL_WARNING, "Could not get native buffer");
               dri2_surf->dri_image_back =
         if (!dri2_surf->dri_image_back) {
      _eglLog(_EGL_WARNING, "failed to create DRI image from FD");
                     } else if (dri2_surf->base.Type == EGL_PBUFFER_BIT) {
      /* The EGL 1.5 spec states that pbuffers are single-buffered.
   * Specifically, the spec states that they have a back buffer but no front
   * buffer, in contrast to pixmaps, which have a front buffer but no back
   * buffer.
   *
   * Single-buffered surfaces with no front buffer confuse Mesa; so we
   * deviate from the spec, following the precedent of Mesa's EGL X11
   * platform. The X11 platform correctly assigns pbuffers to
   * single-buffered configs, but assigns the pbuffer a front buffer instead
   * of a back buffer.
   *
   * Pbuffers in the X11 platform mostly work today, so let's just copy its
   * behavior instead of trying to fix (and hence potentially breaking) the
   * world.
   */
   _eglLog(
      _EGL_DEBUG,
               }
      /* Some drivers will pass multiple bits in buffer_mask.
   * For such case, will go through all the bits, and
   * will not return error when unsupported buffer is requested, only
   * return error when the allocation for supported buffer failed.
   */
   static int
   droid_image_get_buffers(__DRIdrawable *driDrawable, unsigned int format,
               {
               images->image_mask = 0;
   images->front = NULL;
            if (update_buffers(dri2_surf) < 0)
            if (_eglSurfaceInSharedBufferMode(&dri2_surf->base)) {
      if (get_back_bo(dri2_surf) < 0)
            /* We have dri_image_back because this is a window surface and
   * get_back_bo() succeeded.
   */
   assert(dri2_surf->dri_image_back);
   images->back = dri2_surf->dri_image_back;
            /* There exists no accompanying back nor front buffer. */
               if (buffer_mask & __DRI_IMAGE_BUFFER_FRONT) {
      if (get_front_bo(dri2_surf, format) < 0)
            if (dri2_surf->dri_image_front) {
      images->front = dri2_surf->dri_image_front;
                  if (buffer_mask & __DRI_IMAGE_BUFFER_BACK) {
      if (get_back_bo(dri2_surf) < 0)
            if (dri2_surf->dri_image_back) {
      images->back = dri2_surf->dri_image_back;
                     }
      static EGLint
   droid_query_buffer_age(_EGLDisplay *disp, _EGLSurface *surface)
   {
               if (update_buffers(dri2_surf) < 0) {
      _eglError(EGL_BAD_ALLOC, "droid_query_buffer_age");
                  }
      static EGLBoolean
   droid_swap_buffers(_EGLDisplay *disp, _EGLSurface *draw)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   struct dri2_egl_surface *dri2_surf = dri2_egl_surface(draw);
            /* From the EGL_KHR_mutable_render_buffer spec (v12):
   *
   *    If surface is a single-buffered window, pixmap, or pbuffer surface
   *    for which there is no pending change to the EGL_RENDER_BUFFER
   *    attribute, eglSwapBuffers has no effect.
   */
   if (has_mutable_rb && draw->RequestedRenderBuffer == EGL_SINGLE_BUFFER &&
      draw->ActiveRenderBuffer == EGL_SINGLE_BUFFER) {
   _eglLog(_EGL_DEBUG, "%s: remain in shared buffer mode", __func__);
               for (int i = 0; i < dri2_surf->color_buffers_count; i++) {
      if (dri2_surf->color_buffers[i].age > 0)
               /* "XXX: we don't use get_back_bo() since it causes regressions in
   * several dEQP tests.
   */
   if (dri2_surf->back)
            dri2_flush_drawable_for_swapbuffers_flags(disp, draw,
            /* dri2_surf->buffer can be null even when no error has occurred. For
   * example, if the user has called no GL rendering commands since the
   * previous eglSwapBuffers, then the driver may have not triggered
   * a callback to ANativeWindow_dequeueBuffer, in which case
   * dri2_surf->buffer remains null.
   */
   if (dri2_surf->buffer)
                     /* Update the shared buffer mode */
   if (has_mutable_rb &&
      draw->ActiveRenderBuffer != draw->RequestedRenderBuffer) {
   bool mode = (draw->RequestedRenderBuffer == EGL_SINGLE_BUFFER);
   _eglLog(_EGL_DEBUG, "%s: change to shared buffer mode %d", __func__,
            if (!droid_set_shared_buffer_mode(disp, draw, mode))
                        }
      static EGLBoolean
   droid_query_surface(_EGLDisplay *disp, _EGLSurface *surf, EGLint attribute,
         {
      struct dri2_egl_surface *dri2_surf = dri2_egl_surface(surf);
   switch (attribute) {
   case EGL_WIDTH:
      if (dri2_surf->base.Type == EGL_WINDOW_BIT && dri2_surf->window) {
      ANativeWindow_query(dri2_surf->window,
            }
      case EGL_HEIGHT:
      if (dri2_surf->base.Type == EGL_WINDOW_BIT && dri2_surf->window) {
      ANativeWindow_query(dri2_surf->window,
            }
      default:
         }
      }
      static _EGLImage *
   dri2_create_image_android_native_buffer(_EGLDisplay *disp, _EGLContext *ctx,
         {
      if (ctx != NULL) {
      /* From the EGL_ANDROID_image_native_buffer spec:
   *
   *     * If <target> is EGL_NATIVE_BUFFER_ANDROID and <ctx> is not
   *       EGL_NO_CONTEXT, the error EGL_BAD_CONTEXT is generated.
   */
   _eglError(EGL_BAD_CONTEXT,
            "eglCreateEGLImageKHR: for "
                  if (!buf || buf->common.magic != ANDROID_NATIVE_BUFFER_MAGIC ||
      buf->common.version != sizeof(*buf)) {
   _eglError(EGL_BAD_PARAMETER, "eglCreateEGLImageKHR");
               __DRIimage *dri_image =
               #if ANDROID_API_LEVEL >= 26
         #endif
                        }
      static _EGLImage *
   droid_create_image_khr(_EGLDisplay *disp, _EGLContext *ctx, EGLenum target,
         {
      switch (target) {
   case EGL_NATIVE_BUFFER_ANDROID:
      return dri2_create_image_android_native_buffer(
      default:
            }
      static void
   droid_flush_front_buffer(__DRIdrawable *driDrawable, void *loaderPrivate)
   {
   }
      static unsigned
   droid_get_capability(void *loaderPrivate, enum dri_loader_cap cap)
   {
      /* Note: loaderPrivate is _EGLDisplay* */
   switch (cap) {
   case DRI_LOADER_CAP_RGBA_ORDERING:
         default:
            }
      static void
   droid_destroy_loader_image_state(void *loaderPrivate)
   {
   #if ANDROID_API_LEVEL >= 26
      if (loaderPrivate) {
      AHardwareBuffer_release(
         #endif
   }
      static EGLBoolean
   droid_add_configs_for_visuals(_EGLDisplay *disp)
   {
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   static const struct {
      int format;
   int rgba_shifts[4];
      } visuals[] = {
      {HAL_PIXEL_FORMAT_RGBA_8888, {0, 8, 16, 24}, {8, 8, 8, 8}},
   {HAL_PIXEL_FORMAT_RGBX_8888, {0, 8, 16, -1}, {8, 8, 8, 0}},
   {HAL_PIXEL_FORMAT_RGB_565, {11, 5, 0, -1}, {5, 6, 5, 0}},
   /* This must be after HAL_PIXEL_FORMAT_RGBA_8888, we only keep BGRA
   * visual if it turns out RGBA visual is not available.
   */
               unsigned int format_count[ARRAY_SIZE(visuals)] = {0};
            /* The nesting of loops is significant here. Also significant is the order
   * of the HAL pixel formats. Many Android apps (such as Google's official
   * NDK GLES2 example app), and even portions the core framework code (such
   * as SystemServiceManager in Nougat), incorrectly choose their EGLConfig.
   * They neglect to match the EGLConfig's EGL_NATIVE_VISUAL_ID against the
   * window's native format, and instead choose the first EGLConfig whose
   * channel sizes match those of the native window format while ignoring the
   * channel *ordering*.
   *
   * We can detect such buggy clients in logcat when they call
   * eglCreateSurface, by detecting the mismatch between the EGLConfig's
   * format and the window's format.
   *
   * As a workaround, we generate EGLConfigs such that all EGLConfigs for HAL
   * pixel format i precede those for HAL pixel format i+1. In my
   * (chadversary) testing on Android Nougat, this was good enough to pacify
   * the buggy clients.
   */
   bool has_rgba = false;
   for (int i = 0; i < ARRAY_SIZE(visuals); i++) {
      /* Only enable BGRA configs when RGBA is not available. BGRA configs are
   * buggy on stock Android.
   */
   if (visuals[i].format == HAL_PIXEL_FORMAT_BGRA_8888 && has_rgba)
         for (int j = 0; dri2_dpy->driver_configs[j]; j++) {
               const EGLint config_attrs[] = {
      EGL_NATIVE_VISUAL_ID,
   visuals[i].format,
   EGL_NATIVE_VISUAL_TYPE,
   visuals[i].format,
   EGL_FRAMEBUFFER_TARGET_ANDROID,
   EGL_TRUE,
   EGL_RECORDABLE_ANDROID,
   EGL_TRUE,
               struct dri2_egl_config *dri2_conf = dri2_add_config(
      disp, dri2_dpy->driver_configs[j], config_count + 1, surface_type,
      if (dri2_conf) {
      if (dri2_conf->base.ConfigID == config_count + 1)
               }
   if (visuals[i].format == HAL_PIXEL_FORMAT_RGBA_8888 && format_count[i])
               for (int i = 0; i < ARRAY_SIZE(format_count); i++) {
      if (!format_count[i]) {
      _eglLog(_EGL_DEBUG, "No DRI config supports native format 0x%x",
                     }
      static const struct dri2_egl_display_vtbl droid_display_vtbl = {
      .authenticate = NULL,
   .create_window_surface = droid_create_window_surface,
   .create_pbuffer_surface = droid_create_pbuffer_surface,
   .destroy_surface = droid_destroy_surface,
   .create_image = droid_create_image_khr,
   .swap_buffers = droid_swap_buffers,
   .swap_interval = droid_swap_interval,
   .query_buffer_age = droid_query_buffer_age,
   .query_surface = droid_query_surface,
   .get_dri_drawable = dri2_surface_get_dri_drawable,
      };
      static const __DRIimageLoaderExtension droid_image_loader_extension = {
               .getBuffers = droid_image_get_buffers,
   .flushFrontBuffer = droid_flush_front_buffer,
   .getCapability = droid_get_capability,
   .flushSwapBuffers = NULL,
      };
      static void
   droid_display_shared_buffer(__DRIdrawable *driDrawable, int fence_fd,
         {
      struct dri2_egl_surface *dri2_surf = loaderPrivate;
            if (!_eglSurfaceInSharedBufferMode(&dri2_surf->base)) {
      _eglLog(_EGL_WARNING, "%s: internal error: buffer is not shared",
                     if (fence_fd >= 0) {
      /* The driver's fence is more recent than the surface's out fence, if it
   * exists at all. So use the driver's fence.
   */
   if (dri2_surf->out_fence_fd >= 0) {
      close(dri2_surf->out_fence_fd);
         } else if (dri2_surf->out_fence_fd >= 0) {
      fence_fd = dri2_surf->out_fence_fd;
               if (ANativeWindow_queueBuffer(dri2_surf->window, dri2_surf->buffer,
            _eglLog(_EGL_WARNING, "%s: ANativeWindow_queueBuffer failed", __func__);
   close(fence_fd);
                        if (ANativeWindow_dequeueBuffer(dri2_surf->window, &dri2_surf->buffer,
            /* Tear down the surface because it no longer has a back buffer. */
   struct dri2_egl_display *dri2_dpy =
                     dri2_surf->base.Lost = true;
   dri2_surf->buffer = NULL;
            if (dri2_surf->dri_image_back) {
      dri2_dpy->image->destroyImage(dri2_surf->dri_image_back);
               dri2_dpy->flush->invalidate(dri2_surf->dri_drawable);
               close_in_fence_fd(dri2_surf);
   validate_fence_fd(fence_fd);
   dri2_surf->in_fence_fd = fence_fd;
      }
      static const __DRImutableRenderBufferLoaderExtension
      droid_mutable_render_buffer_extension = {
      .base = {__DRI_MUTABLE_RENDER_BUFFER_LOADER, 1},
   };
      static const __DRIextension *droid_image_loader_extensions[] = {
      &droid_image_loader_extension.base,
   &image_lookup_extension.base,
   &use_invalidate.base,
   &droid_mutable_render_buffer_extension.base,
      };
      static EGLBoolean
   droid_load_driver(_EGLDisplay *disp, bool swrast)
   {
               dri2_dpy->driver_name = loader_get_driver_for_fd(dri2_dpy->fd_render_gpu);
   if (dri2_dpy->driver_name == NULL)
            if (swrast) {
      /* Use kms swrast only with vgem / virtio_gpu.
   * virtio-gpu fallbacks to software rendering when 3D features
   * are unavailable since 6c5ab.
   */
   if (strcmp(dri2_dpy->driver_name, "vgem") == 0 ||
      strcmp(dri2_dpy->driver_name, "virtio_gpu") == 0) {
   free(dri2_dpy->driver_name);
      } else {
                     dri2_dpy->loader_extensions = droid_image_loader_extensions;
   if (!dri2_load_driver_dri3(disp)) {
                        error:
      free(dri2_dpy->driver_name);
   dri2_dpy->driver_name = NULL;
      }
      static void
   droid_unload_driver(_EGLDisplay *disp)
   {
               dlclose(dri2_dpy->driver);
   dri2_dpy->driver = NULL;
   free(dri2_dpy->driver_name);
      }
      static int
   droid_filter_device(_EGLDisplay *disp, int fd, const char *vendor)
   {
      drmVersionPtr ver = drmGetVersion(fd);
   if (!ver)
            if (strcmp(vendor, ver->name) != 0) {
      drmFreeVersion(ver);
               drmFreeVersion(ver);
      }
      static EGLBoolean
   droid_probe_device(_EGLDisplay *disp, bool swrast)
   {
      /* Check that the device is supported, by attempting to:
   * - load the dri module
   * - and, create a screen
   */
   if (!droid_load_driver(disp, swrast))
            if (!dri2_create_screen(disp)) {
      _eglLog(_EGL_WARNING, "DRI2: failed to create screen");
   droid_unload_driver(disp);
      }
      }
      static EGLBoolean
   droid_open_device(_EGLDisplay *disp, bool swrast)
   {
   #define MAX_DRM_DEVICES 64
      struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);
   _EGLDevice *dev_list = _eglGlobal.DeviceList;
            char *vendor_name = NULL;
         #ifdef EGL_FORCE_RENDERNODE
         #else
         #endif
         if (property_get("drm.gpu.vendor_name", vendor_buf, NULL) > 0)
            while (dev_list) {
      if (!_eglDeviceSupports(dev_list, _EGL_DEVICE_DRM))
            device = _eglDeviceDrm(dev_list);
            if (!(device->available_nodes & (1 << node_type)))
            dri2_dpy->fd_render_gpu = loader_open_device(device->nodes[node_type]);
   if (dri2_dpy->fd_render_gpu < 0) {
      _eglLog(_EGL_WARNING, "%s() Failed to open DRM device %s", __func__,
                     /* If a vendor is explicitly provided, we use only that.
   * Otherwise we fall-back the first device that is supported.
   */
   if (vendor_name) {
      if (droid_filter_device(disp, dri2_dpy->fd_render_gpu, vendor_name)) {
      /* Device does not match - try next device */
   close(dri2_dpy->fd_render_gpu);
   dri2_dpy->fd_render_gpu = -1;
      }
   /* If the requested device matches - use it. Regardless if
   * init fails, do not fall-back to any other device.
   */
   if (!droid_probe_device(disp, false)) {
      close(dri2_dpy->fd_render_gpu);
                  }
   if (droid_probe_device(disp, swrast))
            /* No explicit request - attempt the next device */
   close(dri2_dpy->fd_render_gpu);
         next:
                  if (dri2_dpy->fd_render_gpu < 0) {
      _eglLog(_EGL_WARNING, "Failed to open %s DRM device",
                        }
      EGLBoolean
   dri2_initialize_android(_EGLDisplay *disp)
   {
      bool device_opened = false;
   struct dri2_egl_display *dri2_dpy;
   const char *err;
            dri2_dpy = calloc(1, sizeof(*dri2_dpy));
   if (!dri2_dpy)
            dri2_dpy->fd_render_gpu = -1;
   dri2_dpy->fd_display_gpu = -1;
   ret = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
         if (ret) {
      err = "DRI2: failed to get gralloc module";
               disp->DriverData = (void *)dri2_dpy;
            if (!device_opened) {
      err = "DRI2: failed to open device";
                        if (!dri2_setup_extensions(disp)) {
      err = "DRI2: failed to setup extensions";
               if (!dri2_setup_device(disp, false)) {
      err = "DRI2: failed to setup EGLDevice";
                        /* We set the maximum swap interval as 1 for Android platform, since it is
   * the maximum value supported by Android according to the value of
   * ANativeWindow::maxSwapInterval.
   */
            disp->Extensions.ANDROID_framebuffer_target = EGL_TRUE;
   disp->Extensions.ANDROID_image_native_buffer = EGL_TRUE;
            /* Querying buffer age requires a buffer to be dequeued.  Without
   * EGL_ANDROID_native_fence_sync, dequeue might call eglClientWaitSync and
   * result in a deadlock (the lock is already held by eglQuerySurface).
   */
   if (disp->Extensions.ANDROID_native_fence_sync) {
         } else {
      /* disable KHR_partial_update that might have been enabled in
   * dri2_setup_screen
   */
                           #if ANDROID_API_LEVEL >= 24
      if (dri2_dpy->mutable_render_buffer &&
      dri2_dpy->loader_extensions == droid_image_loader_extensions &&
   /* In big GL, front rendering is done at the core API level by directly
   * rendering on the front buffer. However, in ES, the front buffer is
   * completely inaccessible through the core ES API.
   *
   * EGL_KHR_mutable_render_buffer is Android's attempt to re-introduce
   * front rendering into ES by squeezing into EGL. Unlike big GL, this
   * extension redirects GL_BACK used by ES for front rendering. Thus we
   * restrict the enabling of this extension to ES only.
   */
   (disp->ClientAPIs & ~(EGL_OPENGL_ES_BIT | EGL_OPENGL_ES2_BIT |
         /* For cros gralloc, if the front rendering query is supported, then all
   * available window surface configs support front rendering because:
   *
   * 1) EGL queries cros gralloc for the front rendering usage bit here
   * 2) EGL combines the front rendering usage bit with the existing usage
   *    if the window surface requests mutable render buffer
   * 3) EGL sets the combined usage onto the ANativeWindow and the next
   *    dequeueBuffer will ask gralloc for an allocation/re-allocation with
   *    the new combined usage
   * 4) cros gralloc(on top of minigbm) resolves the front rendering usage
   *    bit into either BO_USE_FRONT_RENDERING or BO_USE_LINEAR based on
   *    the format support checking.
   *
   * So at least we can force BO_USE_LINEAR as the fallback.
   */
   uint32_t front_rendering_usage = 0;
   if (!strcmp(dri2_dpy->gralloc->common.name, cros_gralloc_module_name) &&
      dri2_dpy->gralloc->perform &&
   dri2_dpy->gralloc->perform(
      dri2_dpy->gralloc, CROS_GRALLOC_DRM_GET_USAGE,
   CROS_GRALLOC_DRM_GET_USAGE_FRONT_RENDERING_BIT,
      dri2_dpy->front_rendering_usage = front_rendering_usage;
            #endif
         /* Create configs *after* enabling extensions because presence of DRI
   * driver extensions can affect the capabilities of EGLConfigs.
   */
   if (!droid_add_configs_for_visuals(disp)) {
      err = "DRI2: failed to add configs";
               /* Fill vtbl last to prevent accidentally calling virtual function during
   * initialization.
   */
                  cleanup:
      dri2_display_destroy(disp);
      }
