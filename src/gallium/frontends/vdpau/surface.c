   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen.
   * Copyright 2011 Christian König.
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
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      #include <assert.h>
      #include "pipe/p_state.h"
      #include "util/u_memory.h"
   #include "util/u_debug.h"
   #include "util/u_rect.h"
   #include "util/u_surface.h"
   #include "util/u_video.h"
   #include "vl/vl_defines.h"
      #include "frontend/drm_driver.h"
      #include "vdpau_private.h"
      enum getbits_conversion {
      CONVERSION_NONE,
   CONVERSION_NV12_TO_YV12,
   CONVERSION_YV12_TO_NV12,
      };
      /**
   * Create a VdpVideoSurface.
   */
   VdpStatus
   vlVdpVideoSurfaceCreate(VdpDevice device, VdpChromaType chroma_type,
               {
      struct pipe_context *pipe;
   vlVdpSurface *p_surf;
            if (!(width && height)) {
      ret = VDP_STATUS_INVALID_SIZE;
               p_surf = CALLOC(1, sizeof(vlVdpSurface));
   if (!p_surf) {
      ret = VDP_STATUS_RESOURCES;
               vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev) {
      ret = VDP_STATUS_INVALID_HANDLE;
               DeviceReference(&p_surf->device, dev);
            mtx_lock(&dev->mutex);
   memset(&p_surf->templat, 0, sizeof(p_surf->templat));
   /* TODO: buffer_format should be selected to match chroma_type */
   p_surf->templat.buffer_format = pipe->screen->get_video_param
   (
      pipe->screen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      );
   p_surf->templat.width = width;
   p_surf->templat.height = height;
   p_surf->templat.interlaced = pipe->screen->get_video_param
   (
      pipe->screen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
      );
   if (p_surf->templat.buffer_format != PIPE_FORMAT_NONE)
            /* do not mandate early allocation of a video buffer */
   vlVdpVideoSurfaceClear(p_surf);
            *surface = vlAddDataHTAB(p_surf);
   if (*surface == 0) {
      ret = VDP_STATUS_ERROR;
                     no_handle:
            inv_device:
      DeviceReference(&p_surf->device, NULL);
         no_res:
   inv_size:
         }
      /**
   * Destroy a VdpVideoSurface.
   */
   VdpStatus
   vlVdpVideoSurfaceDestroy(VdpVideoSurface surface)
   {
               p_surf = (vlVdpSurface *)vlGetDataHTAB((vlHandle)surface);
   if (!p_surf)
            mtx_lock(&p_surf->device->mutex);
   if (p_surf->video_buffer)
                  vlRemoveDataHTAB(surface);
   DeviceReference(&p_surf->device, NULL);
               }
      /**
   * Retrieve the parameters used to create a VdpVideoSurface.
   */
   VdpStatus
   vlVdpVideoSurfaceGetParameters(VdpVideoSurface surface,
               {
      if (!(width && height && chroma_type))
            vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
            if (p_surf->video_buffer) {
      *width = p_surf->video_buffer->width;
   *height = p_surf->video_buffer->height;
      } else {
      *width = p_surf->templat.width;
   *height = p_surf->templat.height;
                  }
      static void
   vlVdpVideoSurfaceSize(vlVdpSurface *p_surf, int component,
         {
      *width = p_surf->templat.width;
            vl_video_buffer_adjust_size(width, height, component,
            }
      /**
   * Copy image data from a VdpVideoSurface to application memory in a specified
   * YCbCr format.
   */
   VdpStatus
   vlVdpVideoSurfaceGetBitsYCbCr(VdpVideoSurface surface,
                     {
      vlVdpSurface *vlsurface;
   struct pipe_context *pipe;
   enum pipe_format format, buffer_format;
   struct pipe_sampler_view **sampler_views;
   enum getbits_conversion conversion = CONVERSION_NONE;
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
            pipe = vlsurface->device->context;
   if (!pipe)
            if (!destination_data || !destination_pitches)
            format = FormatYCBCRToPipe(destination_ycbcr_format);
   if (format == PIPE_FORMAT_NONE)
            if (vlsurface->video_buffer == NULL)
            buffer_format = vlsurface->video_buffer->buffer_format;
   if (format != buffer_format) {
      if (format == PIPE_FORMAT_YV12 && buffer_format == PIPE_FORMAT_NV12)
         else if (format == PIPE_FORMAT_NV12 && buffer_format == PIPE_FORMAT_YV12)
         else if ((format == PIPE_FORMAT_YUYV && buffer_format == PIPE_FORMAT_UYVY) ||
               else
               mtx_lock(&vlsurface->device->mutex);
   sampler_views = vlsurface->video_buffer->get_sampler_view_planes(vlsurface->video_buffer);
   if (!sampler_views) {
      mtx_unlock(&vlsurface->device->mutex);
               for (i = 0; i < 3; ++i) {
      unsigned width, height;
   struct pipe_sampler_view *sv = sampler_views[i];
                     for (j = 0; j < sv->texture->array_size; ++j) {
      struct pipe_box box = {
      0, 0, j,
      };
                  map = pipe->texture_map(pipe, sv->texture, 0,
         if (!map) {
      mtx_unlock(&vlsurface->device->mutex);
               if (conversion == CONVERSION_NV12_TO_YV12 && i == 1) {
      u_copy_nv12_to_yv12(destination_data, destination_pitches,
            } else if (conversion == CONVERSION_YV12_TO_NV12 && i > 0) {
      u_copy_yv12_to_nv12(destination_data, destination_pitches,
            } else if (conversion == CONVERSION_SWAP_YUYV_UYVY) {
      u_copy_swap422_packed(destination_data, destination_pitches,
            } else {
      util_copy_rect(destination_data[i] + destination_pitches[i] * j, sv->texture->format,
                           }
               }
      /**
   * Copy image data from application memory in a specific YCbCr format to
   * a VdpVideoSurface.
   */
   VdpStatus
   vlVdpVideoSurfacePutBitsYCbCr(VdpVideoSurface surface,
                     {
      enum pipe_format pformat = FormatYCBCRToPipe(source_ycbcr_format);
   enum getbits_conversion conversion = CONVERSION_NONE;
   struct pipe_context *pipe;
   struct pipe_sampler_view **sampler_views;
   unsigned i, j;
            vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
            pipe = p_surf->device->context;
   if (!pipe)
            if (!source_data || !source_pitches)
                     if (p_surf->video_buffer == NULL ||
      ((pformat != p_surf->video_buffer->buffer_format))) {
   enum pipe_format nformat = pformat;
            /* Determine the most suitable format for the new surface */
   if (!screen->is_video_format_supported(screen, nformat,
                  nformat = screen->get_video_param(screen,
                     if (nformat == PIPE_FORMAT_NONE) {
      mtx_unlock(&p_surf->device->mutex);
                  if (p_surf->video_buffer == NULL  ||
      nformat != p_surf->video_buffer->buffer_format) {
   /* destroy the old one */
                  /* adjust the template parameters */
   p_surf->templat.buffer_format = nformat;
                                 /* stil no luck? ok forget it we don't support it */
   if (!p_surf->video_buffer) {
      mtx_unlock(&p_surf->device->mutex);
      }
                  if (pformat != p_surf->video_buffer->buffer_format) {
      if (pformat == PIPE_FORMAT_YV12 &&
      p_surf->video_buffer->buffer_format == PIPE_FORMAT_NV12)
      else {
      mtx_unlock(&p_surf->device->mutex);
                  sampler_views = p_surf->video_buffer->get_sampler_view_planes(p_surf->video_buffer);
   if (!sampler_views) {
      mtx_unlock(&p_surf->device->mutex);
               for (i = 0; i < 3; ++i) {
      unsigned width, height;
   struct pipe_sampler_view *sv = sampler_views[i];
   struct pipe_resource *tex;
            tex = sv->texture;
            for (j = 0; j < tex->array_size; ++j) {
      struct pipe_box dst_box = {
      0, 0, j,
               if (conversion == CONVERSION_YV12_TO_NV12 && i == 1) {
                     map = pipe->texture_map(pipe, tex, 0, usage,
         if (!map) {
                        u_copy_nv12_from_yv12(source_data, source_pitches,
                     } else {
      pipe->texture_subdata(pipe, tex, 0,
                        }
   /*
   * This surface has already been synced
   * by the first map.
   */
         }
               }
      /**
   * Helper function to initially clear the VideoSurface after (re-)creation
   */
   void
   vlVdpVideoSurfaceClear(vlVdpSurface *vlsurf)
   {
      struct pipe_context *pipe = vlsurf->device->context;
   struct pipe_surface **surfaces;
            if (!vlsurf->video_buffer)
            surfaces = vlsurf->video_buffer->get_surfaces(vlsurf->video_buffer);
   for (i = 0; i < VL_MAX_SURFACES; ++i) {
               if (!surfaces[i])
            if (i > !!vlsurf->templat.interlaced)
            pipe->clear_render_target(pipe, surfaces[i], &c, 0, 0,
      }
      }
      /**
   * Interop for the GL gallium frontend
   */
   struct pipe_video_buffer *vlVdpVideoSurfaceGallium(VdpVideoSurface surface)
   {
      vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf)
            mtx_lock(&p_surf->device->mutex);
   if (p_surf->video_buffer == NULL) {
               /* try to create a video buffer if we don't already have one */
      }
               }
      VdpStatus vlVdpVideoSurfaceDMABuf(VdpVideoSurface surface,
               {
               struct pipe_screen *pscreen;
                     if (!p_surf)
            if (plane > 3)
            if (!result)
            memset(result, 0, sizeof(*result));
            mtx_lock(&p_surf->device->mutex);
   if (p_surf->video_buffer == NULL) {
               /* try to create a video buffer if we don't already have one */
               /* Check if surface match interop requirements */
   if (p_surf->video_buffer == NULL || !p_surf->video_buffer->interlaced ||
      p_surf->video_buffer->buffer_format != PIPE_FORMAT_NV12) {
   mtx_unlock(&p_surf->device->mutex);
               surf = p_surf->video_buffer->get_surfaces(p_surf->video_buffer)[plane];
   if (!surf) {
      mtx_unlock(&p_surf->device->mutex);
               memset(&whandle, 0, sizeof(struct winsys_handle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
            pscreen = surf->texture->screen;
   if (!pscreen->resource_get_handle(pscreen, p_surf->device->context,
                  mtx_unlock(&p_surf->device->mutex);
                        result->handle = whandle.handle;
   result->width = surf->width;
   result->height = surf->height;
   result->offset = whandle.offset;
            if (surf->format == PIPE_FORMAT_R8_UNORM)
         else
               }
