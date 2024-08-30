   /*
   * Copyright © 2022 Raspberry Pi Ltd
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3d_context.h"
   #include "broadcom/common/v3d_tfu.h"
      bool
   v3dX(tfu)(struct pipe_context *pctx,
            struct pipe_resource *pdst,
   struct pipe_resource *psrc,
   unsigned int src_level,
   unsigned int base_level,
   unsigned int last_level,
   unsigned int src_layer,
      {
         struct v3d_context *v3d = v3d_context(pctx);
   struct v3d_screen *screen = v3d->screen;
   struct v3d_resource *src = v3d_resource(psrc);
   struct v3d_resource *dst = v3d_resource(pdst);
   struct v3d_resource_slice *src_base_slice = &src->slices[src_level];
   struct v3d_resource_slice *dst_base_slice = &dst->slices[base_level];
   int msaa_scale = pdst->nr_samples > 1 ? 2 : 1;
   int width = u_minify(pdst->width0, base_level) * msaa_scale;
   int height = u_minify(pdst->height0, base_level) * msaa_scale;
            if (psrc->format != pdst->format)
         if (psrc->nr_samples != pdst->nr_samples)
            if (pdst->target != PIPE_TEXTURE_2D || psrc->target != PIPE_TEXTURE_2D)
            /* Can't write to raster. */
   if (dst_base_slice->tiling == V3D_TILING_RASTER)
            /* When using TFU for blit, we are doing exact copies (both input and
      * output format must be the same, no scaling, etc), so there is no
   * pixel format conversions. Thus we can rewrite the format to use one
   * that is TFU compatible based on its texel size.
      if (for_mipmap) {
         } else {
            switch (dst->cpp) {
   case 16: pformat = PIPE_FORMAT_R32G32B32A32_FLOAT;   break;
   case 8:  pformat = PIPE_FORMAT_R16G16B16A16_FLOAT;   break;
   case 4:  pformat = PIPE_FORMAT_R32_FLOAT;            break;
   case 2:  pformat = PIPE_FORMAT_R16_FLOAT;            break;
   case 1:  pformat = PIPE_FORMAT_R8_UNORM;             break;
                        if (!v3dX(tfu_supports_tex_format)(tex_format, for_mipmap)) {
                        v3d_flush_jobs_writing_resource(v3d, psrc, V3D_FLUSH_DEFAULT, false);
            struct drm_v3d_submit_tfu tfu = {
            .ios = (height << 16) | width,
   .bo_handles = {
               },
      };
   uint32_t src_offset = (src->bo->offset +
                  uint32_t dst_offset = (dst->bo->offset +
                  switch (src_base_slice->tiling) {
   case V3D_TILING_UIF_NO_XOR:
   case V3D_TILING_UIF_XOR:
            tfu.iis |= (src_base_slice->padded_height /
      case V3D_TILING_RASTER:
               case V3D_TILING_LINEARTILE:
   case V3D_TILING_UBLINEAR_1_COLUMN:
   case V3D_TILING_UBLINEAR_2_COLUMN:
            #if V3D_VERSION <= 42
         if (src_base_slice->tiling == V3D_TILING_RASTER) {
               } else {
            tfu.icfg |= ((V3D33_TFU_ICFG_FORMAT_LINEARTILE +
      }
            if (last_level != base_level)
            tfu.ioa |= ((V3D33_TFU_IOA_FORMAT_LINEARTILE +
                           /* If we're writing level 0 (!IOA_DIMTW), then we need to supply the
      * OPAD field for the destination (how many extra UIF blocks beyond
   * those necessary to cover the height).  When filling mipmaps, the
   * miplevel 1+ tiling state is inferred.
      if (dst_base_slice->tiling == V3D_TILING_UIF_NO_XOR ||
         dst_base_slice->tiling == V3D_TILING_UIF_XOR) {
                     tfu.icfg |= (((dst_base_slice->padded_height -
      #endif /* V3D_VERSION <= 42 */
      #if V3D_VERSION >= 71
         if (src_base_slice->tiling == V3D_TILING_RASTER) {
         } else {
            tfu.icfg = (V3D71_TFU_ICFG_FORMAT_LINEARTILE +
      }
            if (last_level != base_level)
            tfu.v71.ioc |= ((V3D71_TFU_IOC_FORMAT_LINEARTILE +
                  switch (dst_base_slice->tiling) {
   case V3D_TILING_UIF_NO_XOR:
   case V3D_TILING_UIF_XOR:
            tfu.v71.ioc |=
            case V3D_TILING_RASTER:
            tfu.v71.ioc |= (dst_base_slice->padded_height / dst->cpp) <<
      default:
                  #endif /* V3D_VERSION >= 71*/
            int ret = v3d_ioctl(screen->fd, DRM_IOCTL_V3D_SUBMIT_TFU, &tfu);
   if (ret != 0) {
                                 }
   