   /**************************************************************************
   *
   * Copyright 2013 Advanced Micro Devices, Inc.
   * All Rights Reserved.
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
   * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #if ENABLE_ST_OMX_TIZONIA
   #include <tizkernel.h>
   #endif
      #include "util/u_memory.h"
   #include "vl/vl_winsys.h"
   #include "vl/vl_video_buffer.h"
   #include "util/u_surface.h"
      #include "vid_dec_common.h"
   #include "vid_dec_h264_common.h"
      void vid_dec_NeedTarget(vid_dec_PrivateType *priv)
   {
      struct pipe_video_buffer templat = {};
   struct vl_screen *omx_screen;
            omx_screen = priv->screen;
            pscreen = omx_screen->pscreen;
            if (!priv->target) {
               templat.width = priv->codec->width;
   templat.height = priv->codec->height;
   templat.buffer_format = pscreen->get_video_param(
         pscreen,
   priv->profile,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
   );
   templat.interlaced = pscreen->get_video_param(
      pscreen,
   priv->profile,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
                     }
      void vid_dec_FillOutput(vid_dec_PrivateType *priv, struct pipe_video_buffer *buf,
         {
   #if ENABLE_ST_OMX_TIZONIA
      tiz_port_t *out_port = tiz_krn_get_port(tiz_get_krn(handleOf(priv)),
            #else
      omx_base_PortType *port = priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
      #endif
         struct pipe_sampler_view **views;
   unsigned i, j;
                  #if ENABLE_ST_OMX_TIZONIA
      if (!output->pBuffer) {
      struct pipe_video_buffer *dst_buf = NULL;
   struct pipe_surface **dst_surface = NULL;
   struct u_rect src_rect;
   struct u_rect dst_rect;
   struct vl_compositor *compositor = &priv->compositor;
   struct vl_compositor_state *s = &priv->cstate;
            dst_buf = util_hash_table_get(priv->video_buffer_map, output);
            dst_surface = dst_buf->get_surfaces(dst_buf);
            src_rect.x0 = 0;
   src_rect.y0 = 0;
   src_rect.x1 = def->nFrameWidth;
            dst_rect.x0 = 0;
   dst_rect.y0 = 0;
   dst_rect.x1 = def->nFrameWidth;
            vl_compositor_clear_layers(s);
   vl_compositor_set_buffer_layer(s, compositor, 0, buf,
         vl_compositor_set_layer_dst_area(s, 0, &dst_rect);
                           #endif
         for (i = 0; i < 2 /* NV12 */; i++) {
      if (!views[i]) continue;
   width = def->nFrameWidth;
   height = def->nFrameHeight;
   vl_video_buffer_adjust_size(&width, &height, i,
               for (j = 0; j < views[i]->texture->array_size; ++j) {
      struct pipe_box box = {0, 0, j, width, height, 1};
   struct pipe_transfer *transfer;
   uint8_t *map, *dst;
   map = priv->pipe->texture_map(priv->pipe, views[i]->texture, 0,
                        dst = ((uint8_t*)output->pBuffer + output->nOffset) + j * def->nStride +
         util_copy_rect(dst,
      views[i]->texture->format,
                        }
