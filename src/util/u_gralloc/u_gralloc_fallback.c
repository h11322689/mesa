   /*
   * Mesa 3-D graphics library
   *
   * Copyright © 2021, Google Inc.
   * SPDX-License-Identifier: MIT
   */
      #include "u_gralloc_internal.h"
      #include <hardware/gralloc.h>
      #include "drm-uapi/drm_fourcc.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "util/u_memory.h"
      #include <dlfcn.h>
   #include <errno.h>
   #include <string.h>
      struct fallback_gralloc {
      struct u_gralloc base;
      };
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
   get_native_buffer_fds(const native_handle_t *handle, int fds[3])
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
   fallback_gralloc_get_yuv_info(struct u_gralloc *gralloc,
               {
      struct fallback_gralloc *gr = (struct fallback_gralloc *)gralloc;
   gralloc_module_t *gr_mod = gr->gralloc_module;
   enum chroma_order chroma_order;
   struct android_ycbcr ycbcr;
   int drm_fourcc = 0;
   int num_fds = 0;
   int fds[3];
            num_fds = get_native_buffer_fds(hnd->handle, fds);
   if (num_fds == 0)
            if (!gr_mod || !gr_mod->lock_ycbcr) {
                  memset(&ycbcr, 0, sizeof(ycbcr));
   ret = gr_mod->lock_ycbcr(gr_mod, hnd->handle, 0, 0, 0, 0, 0, &ycbcr);
   if (ret) {
      /* HACK: See native_window_buffer_get_buffer_info() and
   * https://issuetracker.google.com/32077885.*/
   if (hnd->hal_format == HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED)
            mesa_logw("gralloc->lock_ycbcr failed: %d", ret);
      }
                     /* .chroma_step is the byte distance between the same chroma channel
   * values of subsequent pixels, assumed to be the same for Cb and Cr. */
   drm_fourcc =
         if (drm_fourcc == -1) {
      mesa_logw("unsupported YUV format, native = %x, chroma_order = %s, "
            "chroma_step = %zu",
                  out->drm_fourcc = drm_fourcc;
            out->num_planes = ycbcr.chroma_step == 2 ? 2 : 3;
   /* When lock_ycbcr's usage argument contains no SW_READ/WRITE flags
   * it will return the .y/.cb/.cr pointers based on a NULL pointer,
   * so they can be interpreted as offsets. */
   out->offsets[0] = (size_t)ycbcr.y;
   /* We assume here that all the planes are located in one DMA-buf. */
   if (chroma_order == YCrCb) {
      out->offsets[1] = (size_t)ycbcr.cr;
      } else {
      out->offsets[1] = (size_t)ycbcr.cb;
               /* .ystride is the line length (in bytes) of the Y plane,
   * .cstride is the line length (in bytes) of any of the remaining
   * Cb/Cr/CbCr planes, assumed to be the same for Cb and Cr for fully
   * planar formats. */
   out->strides[0] = ycbcr.ystride;
            /*
   * Since this is EGL_NATIVE_BUFFER_ANDROID don't assume that
   * the single-fd case cannot happen.  So handle eithe single
   * fd or fd-per-plane case:
   */
   if (num_fds == 1) {
      out->fds[1] = out->fds[0] = fds[0];
   if (out->num_planes == 3)
      } else {
      assert(num_fds == out->num_planes);
   out->fds[0] = fds[0];
   out->fds[1] = fds[1];
                  }
      static int
   fallback_gralloc_get_buffer_info(struct u_gralloc *gralloc,
               {
      int num_planes = 0;
   int drm_fourcc = 0;
   int stride = 0;
            if (is_yuv(hnd->hal_format)) {
      int ret = fallback_gralloc_get_yuv_info(gralloc, hnd, out);
   /*
   * HACK: https://issuetracker.google.com/32077885
   * There is no API available to properly query the
   * IMPLEMENTATION_DEFINED format. As a workaround we rely here on
   * gralloc allocating either an arbitrary YCbCr 4:2:0 or RGBX_8888, with
   * the latter being recognized by lock_ycbcr failing.
   */
   if (ret != -EAGAIN)
               /*
   * Non-YUV formats could *also* have multiple planes, such as ancillary
   * color compression state buffer, but the rest of the code isn't ready
   * yet to deal with modifiers:
   */
   num_planes = get_native_buffer_fds(hnd->handle, fds);
   if (num_planes == 0)
                     drm_fourcc = get_fourcc(hnd->hal_format);
   if (drm_fourcc == -1) {
      mesa_loge("Failed to get drm_fourcc");
               stride = hnd->pixel_stride * get_format_bpp(hnd->hal_format);
   if (stride == 0) {
      mesa_loge("Failed to calcuulate stride");
               out->drm_fourcc = drm_fourcc;
   out->modifier = DRM_FORMAT_MOD_INVALID;
   out->num_planes = num_planes;
   out->fds[0] = fds[0];
               }
      static int
   destroy(struct u_gralloc *gralloc)
   {
      struct fallback_gralloc *gr = (struct fallback_gralloc *)gralloc;
   if (gr->gralloc_module) {
                              }
      struct u_gralloc *
   u_gralloc_fallback_create()
   {
      struct fallback_gralloc *gr = CALLOC_STRUCT(fallback_gralloc);
            err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID,
            if (err) {
      mesa_logw(
      } else if (!gr->gralloc_module->lock_ycbcr) {
      mesa_logw("Gralloc doesn't support lock_ycbcr (video buffers won't be "
               gr->base.ops.get_buffer_basic_info = fallback_gralloc_get_buffer_info;
                        }
