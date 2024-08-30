   /**************************************************************************
   *
   * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
   * Copyright 2014 Advanced Micro Devices, Inc.
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
      #include "pipe/p_screen.h"
   #include "pipe/p_video_codec.h"
      #include "util/u_memory.h"
   #include "util/u_handle_table.h"
   #include "util/u_rect.h"
   #include "util/u_sampler.h"
   #include "util/u_surface.h"
   #include "util/u_video.h"
   #include "util/set.h"
      #include "vl/vl_compositor.h"
   #include "vl/vl_video_buffer.h"
   #include "vl/vl_winsys.h"
      #include "va_private.h"
      #ifdef _WIN32
   #include "frontend/winsys_handle.h"
   #include <va/va_win32.h>
   #else
   #include "frontend/drm_driver.h"
   #include <va/va_drmcommon.h>
   #include "drm-uapi/drm_fourcc.h"
   #endif
      static const enum pipe_format vpp_surface_formats[] = {
      PIPE_FORMAT_B8G8R8A8_UNORM, PIPE_FORMAT_R8G8B8A8_UNORM,
      };
      VAStatus
   vlVaCreateSurfaces(VADriverContextP ctx, int width, int height, int format,
         {
      return vlVaCreateSurfaces2(ctx, format, width, height, surfaces, num_surfaces,
      }
      VAStatus
   vlVaDestroySurfaces(VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces)
   {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   for (i = 0; i < num_surfaces; ++i) {
      vlVaSurface *surf = handle_table_get(drv->htab, surface_list[i]);
   if (!surf) {
      mtx_unlock(&drv->mutex);
      }
   if (surf->buffer)
         if (surf->deint_buffer)
         if (surf->ctx) {
      assert(_mesa_set_search(surf->ctx->surfaces, surf));
   _mesa_set_remove_key(surf->ctx->surfaces, surf);
   if (surf->fence && surf->ctx->decoder && surf->ctx->decoder->destroy_fence)
      }
   util_dynarray_fini(&surf->subpics);
   FREE(surf);
      }
               }
      VAStatus
   vlVaSyncSurface(VADriverContextP ctx, VASurfaceID render_target)
   {
      vlVaDriver *drv;
   vlVaContext *context;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
            mtx_lock(&drv->mutex);
            if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               /* This is checked before getting the context below as
   * surf->ctx is only set in begin_frame
   * and not when the surface is created
   * Some apps try to sync/map the surface right after creation and
   * would get VA_STATUS_ERROR_INVALID_CONTEXT
   */
   if ((!surf->feedback) && (!surf->fence)) {
      // No outstanding encode/decode operation: nothing to do.
   mtx_unlock(&drv->mutex);
               context = surf->ctx;
   if (!context) {
      mtx_unlock(&drv->mutex);
               if (!context->decoder) {
      mtx_unlock(&drv->mutex);
               if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
      /* If driver does not implement get_processor_fence assume no
   * async work needed to be waited on and return success
   */
            if (context->decoder->get_processor_fence)
      ret = context->decoder->get_processor_fence(context->decoder,
               mtx_unlock(&drv->mutex);
   // Assume that the GPU has hung otherwise.
      } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
               if (context->decoder->get_decoder_fence)
      ret = context->decoder->get_decoder_fence(context->decoder,
               mtx_unlock(&drv->mutex);
   // Assume that the GPU has hung otherwise.
      } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if (!drv->pipe->screen->get_video_param(drv->pipe->screen,
                        if (u_reduce_video_profile(context->templat.profile) == PIPE_VIDEO_FORMAT_MPEG4_AVC) {
      int frame_diff;
   if (context->desc.h264enc.frame_num_cnt >= surf->frame_num_cnt)
         else
         if ((frame_diff == 0) &&
      (surf->force_flushed == false) &&
   (context->desc.h264enc.frame_num_cnt % 2 != 0)) {
   context->decoder->flush(context->decoder);
            }
   context->decoder->get_feedback(context->decoder, surf->feedback, &(surf->coded_buf->coded_size));
   surf->feedback = NULL;
   surf->coded_buf->feedback = NULL;
      }
   mtx_unlock(&drv->mutex);
      }
      VAStatus
   vlVaQuerySurfaceStatus(VADriverContextP ctx, VASurfaceID render_target, VASurfaceStatus *status)
   {
      vlVaDriver *drv;
   vlVaSurface *surf;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   if (!drv)
                     surf = handle_table_get(drv->htab, render_target);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
               /* This is checked before getting the context below as
   * surf->ctx is only set in begin_frame
   * and not when the surface is created
   * Some apps try to sync/map the surface right after creation and
   * would get VA_STATUS_ERROR_INVALID_CONTEXT
   */
   if ((!surf->feedback) && (!surf->fence)) {
      // No outstanding encode/decode operation: nothing to do.
   *status = VASurfaceReady;
   mtx_unlock(&drv->mutex);
               context = surf->ctx;
   if (!context) {
      mtx_unlock(&drv->mutex);
               if (!context->decoder) {
      mtx_unlock(&drv->mutex);
               if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE) {
      if(surf->feedback == NULL)
         else
      } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_BITSTREAM) {
               if (context->decoder->get_decoder_fence)
                  if (ret)
         else
   /* An approach could be to just tell the client that this is not
   * implemented, but this breaks other code.  Compromise by at least
   * conservatively setting the status to VASurfaceRendering if we can't
   * query the hardware.  Note that we _must_ set the status here, otherwise
   * it comes out of the function unchanged. As we are returning
   * VA_STATUS_SUCCESS, the client would be within his/her rights to use a
   * potentially uninitialized/invalid status value unknowingly.
   */
      } else if (context->decoder->entrypoint == PIPE_VIDEO_ENTRYPOINT_PROCESSING) {
      /* If driver does not implement get_processor_fence assume no
   * async work needed to be waited on and return surface ready
   */
            if (context->decoder->get_processor_fence)
      ret = context->decoder->get_processor_fence(context->decoder,
      if (ret)
         else
                           }
      VAStatus
   vlVaQuerySurfaceError(VADriverContextP ctx, VASurfaceID render_target, VAStatus error_status, void **error_info)
   {
      if (!ctx)
               }
      static void
   upload_sampler(struct pipe_context *pipe, struct pipe_sampler_view *dst,
               {
      struct pipe_transfer *transfer;
            map = pipe->texture_map(pipe, dst->texture, 0, PIPE_MAP_WRITE,
         if (!map)
            util_copy_rect(map, dst->texture->format, transfer->stride, 0, 0,
                     }
      static VAStatus
   vlVaPutSubpictures(vlVaSurface *surf, vlVaDriver *drv,
               {
      vlVaSubpicture *sub;
            if (!(surf->subpics.data || surf->subpics.size))
            for (i = 0; i < surf->subpics.size/sizeof(vlVaSubpicture *); i++) {
      struct pipe_blend_state blend;
   void *blend_state;
   vlVaBuffer *buf;
   struct pipe_box box;
   struct u_rect *s, *d, sr, dr, c;
            sub = ((vlVaSubpicture **)surf->subpics.data)[i];
   if (!sub)
            buf = handle_table_get(drv->htab, sub->image->buf);
   if (!buf)
            box.x = 0;
   box.y = 0;
   box.z = 0;
   box.width = sub->dst_rect.x1 - sub->dst_rect.x0;
   box.height = sub->dst_rect.y1 - sub->dst_rect.y0;
            s = &sub->src_rect;
   d = &sub->dst_rect;
   sw = s->x1 - s->x0;
   sh = s->y1 - s->y0;
   dw = d->x1 - d->x0;
   dh = d->y1 - d->y0;
   c.x0 = MAX2(d->x0, s->x0);
   c.y0 = MAX2(d->y0, s->y0);
   c.x1 = MIN2(d->x0 + dw, src_rect->x1);
   c.y1 = MIN2(d->y0 + dh, src_rect->y1);
   sr.x0 = s->x0 + (c.x0 - d->x0)*(sw/(float)dw);
   sr.y0 = s->y0 + (c.y0 - d->y0)*(sh/(float)dh);
   sr.x1 = s->x0 + (c.x1 - d->x0)*(sw/(float)dw);
            s = src_rect;
   d = dst_rect;
   sw = s->x1 - s->x0;
   sh = s->y1 - s->y0;
   dw = d->x1 - d->x0;
   dh = d->y1 - d->y0;
   dr.x0 = d->x0 + c.x0*(dw/(float)sw);
   dr.y0 = d->y0 + c.y0*(dh/(float)sh);
   dr.x1 = d->x0 + c.x1*(dw/(float)sw);
            memset(&blend, 0, sizeof(blend));
   blend.independent_blend_enable = 0;
   blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_SRC_ALPHA;
   blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_INV_SRC_ALPHA;
   blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.rt[0].rgb_func = PIPE_BLEND_ADD;
   blend.rt[0].alpha_func = PIPE_BLEND_ADD;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
   blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.dither = 0;
            vl_compositor_clear_layers(&drv->cstate);
   vl_compositor_set_layer_blend(&drv->cstate, 0, blend_state, false);
   upload_sampler(drv->pipe, sub->sampler, &box, buf->data,
         vl_compositor_set_rgba_layer(&drv->cstate, &drv->compositor, 0, sub->sampler,
         vl_compositor_set_layer_dst_area(&drv->cstate, 0, &dr);
   vl_compositor_render(&drv->cstate, &drv->compositor, surf_draw, dirty_area, false);
                  }
      VAStatus
   vlVaPutSurface(VADriverContextP ctx, VASurfaceID surface_id, void* draw, short srcx, short srcy,
                     {
      vlVaDriver *drv;
   vlVaSurface *surf;
   struct pipe_screen *screen;
   struct pipe_resource *tex;
   struct pipe_surface surf_templ, *surf_draw;
   struct vl_screen *vscreen;
   struct u_rect src_rect, *dirty_area;
   struct u_rect dst_rect = {destx, destx + destw, desty, desty + desth};
   enum pipe_format format;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   surf = handle_table_get(drv->htab, surface_id);
   if (!surf) {
      mtx_unlock(&drv->mutex);
               screen = drv->pipe->screen;
            tex = vscreen->texture_from_drawable(vscreen, draw);
   if (!tex) {
      mtx_unlock(&drv->mutex);
                        memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = tex->format;
   surf_draw = drv->pipe->create_surface(drv->pipe, tex, &surf_templ);
   if (!surf_draw) {
      pipe_resource_reference(&tex, NULL);
   mtx_unlock(&drv->mutex);
               src_rect.x0 = srcx;
   src_rect.y0 = srcy;
   src_rect.x1 = srcw + srcx;
                              if (format == PIPE_FORMAT_B8G8R8A8_UNORM || format == PIPE_FORMAT_B8G8R8X8_UNORM ||
      format == PIPE_FORMAT_R8G8B8A8_UNORM || format == PIPE_FORMAT_R8G8B8X8_UNORM ||
   format == PIPE_FORMAT_L8_UNORM || format == PIPE_FORMAT_Y8_400_UNORM) {
            views = surf->buffer->get_sampler_view_planes(surf->buffer);
      } else
            vl_compositor_set_layer_dst_area(&drv->cstate, 0, &dst_rect);
            status = vlVaPutSubpictures(surf, drv, surf_draw, dirty_area, &src_rect, &dst_rect);
   if (status) {
      mtx_unlock(&drv->mutex);
               /* flush before calling flush_frontbuffer so that rendering is flushed
   * to back buffer so the texture can be copied in flush_frontbuffer
   */
            screen->flush_frontbuffer(screen, drv->pipe, tex, 0, 0,
               pipe_resource_reference(&tex, NULL);
   pipe_surface_reference(&surf_draw, NULL);
               }
      VAStatus
   vlVaLockSurface(VADriverContextP ctx, VASurfaceID surface, unsigned int *fourcc,
                     {
      if (!ctx)
               }
      VAStatus
   vlVaUnlockSurface(VADriverContextP ctx, VASurfaceID surface)
   {
      if (!ctx)
               }
      VAStatus
   vlVaQuerySurfaceAttributes(VADriverContextP ctx, VAConfigID config_id,
         {
      vlVaDriver *drv;
   vlVaConfig *config;
   VASurfaceAttrib *attribs;
   struct pipe_screen *pscreen;
                     if (config_id == VA_INVALID_ID)
            if (!attrib_list && !num_attribs)
            if (!attrib_list) {
      *num_attribs = VL_VA_MAX_IMAGE_FORMATS + VASurfaceAttribCount;
               if (!ctx)
                     if (!drv)
            mtx_lock(&drv->mutex);
   config = handle_table_get(drv->htab, config_id);
            if (!config)
                     if (!pscreen)
            attribs = CALLOC(VL_VA_MAX_IMAGE_FORMATS + VASurfaceAttribCount,
            if (!attribs)
                     /* vlVaCreateConfig returns PIPE_VIDEO_PROFILE_UNKNOWN
   * only for VAEntrypointVideoProc. */
   if (config->profile == PIPE_VIDEO_PROFILE_UNKNOWN) {
      if (config->rt_format & VA_RT_FORMAT_RGB32) {
      for (j = 0; j < ARRAY_SIZE(vpp_surface_formats); ++j) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = PipeFormatToVaFourcc(vpp_surface_formats[j]);
                     if (config->rt_format & VA_RT_FORMAT_YUV420) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_NV12;
               if (config->rt_format & VA_RT_FORMAT_YUV420_10 ||
      (config->rt_format & VA_RT_FORMAT_YUV420 &&
   config->entrypoint == PIPE_VIDEO_ENTRYPOINT_ENCODE)) {
   attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_P010;
   i++;
   attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_P016;
               if (config->profile == PIPE_VIDEO_PROFILE_JPEG_BASELINE) {
      if (config->rt_format & VA_RT_FORMAT_YUV400) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_Y800;
               if (config->rt_format & VA_RT_FORMAT_YUV422) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_YUY2;
               if (config->rt_format & VA_RT_FORMAT_YUV444) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_444P;
      }
   if (config->rt_format & VA_RT_FORMAT_RGBP) {
      attribs[i].type = VASurfaceAttribPixelFormat;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.i = VA_FOURCC_RGBP;
                  attribs[i].type = VASurfaceAttribMemoryType;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE | VA_SURFACE_ATTRIB_SETTABLE;
      #ifdef _WIN32
               #else
               #endif
               attribs[i].type = VASurfaceAttribExternalBufferDescriptor;
   attribs[i].value.type = VAGenericValueTypePointer;
   attribs[i].flags = VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.p = NULL; /* ignore */
         #ifdef HAVE_VA_SURFACE_ATTRIB_DRM_FORMAT_MODIFIERS
      if (drv->pipe->create_video_buffer_with_modifiers) {
      attribs[i].type = VASurfaceAttribDRMFormatModifiers;
   attribs[i].value.type = VAGenericValueTypePointer;
   attribs[i].flags = VA_SURFACE_ATTRIB_SETTABLE;
   attribs[i].value.value.p = NULL; /* ignore */
         #endif
         /* If VPP supported entry, use the max dimensions cap values, if not fallback to this below */
   if (config->entrypoint != PIPE_VIDEO_ENTRYPOINT_PROCESSING ||
      pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
            {
      unsigned min_width, min_height;
   min_width = pscreen->get_video_param(pscreen,
               min_height = pscreen->get_video_param(pscreen,
                  if (min_width > 0 && min_height > 0) {
      attribs[i].type = VASurfaceAttribMinWidth;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
                  attribs[i].type = VASurfaceAttribMinHeight;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
   attribs[i].value.value.i = min_height;
               attribs[i].type = VASurfaceAttribMaxWidth;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
   attribs[i].value.value.i =
      pscreen->get_video_param(pscreen,
                     attribs[i].type = VASurfaceAttribMaxHeight;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
   attribs[i].value.value.i =
      pscreen->get_video_param(pscreen,
               } else {
      attribs[i].type = VASurfaceAttribMaxWidth;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
   attribs[i].value.value.i = vl_video_buffer_max_size(pscreen);
            attribs[i].type = VASurfaceAttribMaxHeight;
   attribs[i].value.type = VAGenericValueTypeInteger;
   attribs[i].flags = VA_SURFACE_ATTRIB_GETTABLE;
   attribs[i].value.value.i = vl_video_buffer_max_size(pscreen);
               if (i > *num_attribs) {
      *num_attribs = i;
   FREE(attribs);
               *num_attribs = i;
   memcpy(attrib_list, attribs, i * sizeof(VASurfaceAttrib));
               }
      #ifndef _WIN32
   static VAStatus
   surface_from_external_memory(VADriverContextP ctx, vlVaSurface *surface,
               {
      vlVaDriver *drv;
   struct pipe_screen *pscreen;
   struct pipe_resource res_templ;
   struct winsys_handle whandle;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
   enum pipe_format resource_formats[VL_NUM_COMPONENTS];
   VAStatus result;
            pscreen = VL_VA_PSCREEN(ctx);
            if (!memory_attribute || !memory_attribute->buffers ||
      index > memory_attribute->num_buffers)
         if (surface->templat.width != memory_attribute->width ||
      surface->templat.height != memory_attribute->height ||
   memory_attribute->num_planes < 1)
         if (memory_attribute->num_planes > VL_NUM_COMPONENTS)
                     memset(&res_templ, 0, sizeof(res_templ));
   res_templ.target = PIPE_TEXTURE_2D;
   res_templ.last_level = 0;
   res_templ.depth0 = 1;
   res_templ.array_size = 1;
   res_templ.bind = PIPE_BIND_SAMPLER_VIEW;
            memset(&whandle, 0, sizeof(struct winsys_handle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
   whandle.handle = memory_attribute->buffers[index];
   whandle.modifier = DRM_FORMAT_MOD_INVALID;
            // Create a resource for each plane.
   memset(resources, 0, sizeof resources);
   for (i = 0; i < memory_attribute->num_planes; i++) {
               res_templ.format = resource_formats[i];
   if (res_templ.format == PIPE_FORMAT_NONE) {
      if (i < num_planes) {
      result = VA_STATUS_ERROR_INVALID_PARAMETER;
      } else {
                     res_templ.width0 = util_format_get_plane_width(templat->buffer_format, i,
         res_templ.height0 = util_format_get_plane_height(templat->buffer_format, i,
            whandle.stride = memory_attribute->pitches[i];
   whandle.offset = memory_attribute->offsets[i];
   resources[i] = pscreen->resource_from_handle(pscreen, &res_templ, &whandle,
         if (!resources[i]) {
      result = VA_STATUS_ERROR_ALLOCATION_FAILED;
                  surface->buffer = vl_video_buffer_create_ex2(drv->pipe, templat, resources);
   if (!surface->buffer) {
      result = VA_STATUS_ERROR_ALLOCATION_FAILED;
      }
         fail:
      for (i = 0; i < VL_NUM_COMPONENTS; i++)
            }
      static VAStatus
   surface_from_prime_2(VADriverContextP ctx, vlVaSurface *surface,
               {
      vlVaDriver *drv;
   struct pipe_screen *pscreen;
   struct pipe_resource res_templ;
   struct winsys_handle whandle;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
   enum pipe_format resource_formats[VL_NUM_COMPONENTS];
   unsigned num_format_planes, expected_planes, input_planes, plane;
            num_format_planes = util_format_get_num_planes(templat->buffer_format);
   pscreen = VL_VA_PSCREEN(ctx);
            if (!desc || desc->num_layers >= 4 ||desc->num_objects == 0)
            if (surface->templat.width != desc->width ||
      surface->templat.height != desc->height ||
   desc->num_layers < 1)
         if (desc->num_layers > VL_NUM_COMPONENTS)
            input_planes = 0;
   for (unsigned i = 0; i < desc->num_layers; ++i) {
      if (desc->layers[i].num_planes == 0 || desc->layers[i].num_planes > 4)
            for (unsigned j = 0; j < desc->layers[i].num_planes; ++j)
                              expected_planes = num_format_planes;
   if (desc->objects[0].drm_format_modifier != DRM_FORMAT_MOD_INVALID &&
      pscreen->is_dmabuf_modifier_supported &&
   pscreen->is_dmabuf_modifier_supported(pscreen, desc->objects[0].drm_format_modifier,
         pscreen->get_dmabuf_modifier_planes)
   expected_planes = pscreen->get_dmabuf_modifier_planes(pscreen, desc->objects[0].drm_format_modifier,
         if (input_planes != expected_planes)
                     memset(&res_templ, 0, sizeof(res_templ));
   res_templ.target = PIPE_TEXTURE_2D;
   res_templ.last_level = 0;
   res_templ.depth0 = 1;
   res_templ.array_size = 1;
   res_templ.bind = PIPE_BIND_SAMPLER_VIEW;
   res_templ.usage = PIPE_USAGE_DEFAULT;
            memset(&whandle, 0, sizeof(struct winsys_handle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
   whandle.format = templat->buffer_format;
            // Create a resource for each plane.
            /* This does a backwards walk to set the next pointers. It interleaves so
   * that the main planes always come first and then the first compression metadata
   * plane of each main plane etc. */
   plane = input_planes - 1;
   for (int layer_plane = 3; layer_plane >= 0; --layer_plane) {
      for (int layer = desc->num_layers - 1; layer >= 0; --layer) {
                                    res_templ.width0 = util_format_get_plane_width(templat->buffer_format, plane,
         res_templ.height0 = util_format_get_plane_height(templat->buffer_format, plane,
         whandle.stride = desc->layers[layer].pitch[layer_plane];
   whandle.offset = desc->layers[layer].offset[layer_plane];
                  resources[plane] = pscreen->resource_from_handle(pscreen, &res_templ, &whandle,
         if (!resources[plane]) {
      result = VA_STATUS_ERROR_ALLOCATION_FAILED;
                              if (plane)
                        surface->buffer = vl_video_buffer_create_ex2(drv->pipe, templat, resources);
   if (!surface->buffer) {
      result = VA_STATUS_ERROR_ALLOCATION_FAILED;
      }
         fail:
      pipe_resource_reference(&res_templ.next, NULL);
   for (int i = 0; i < VL_NUM_COMPONENTS; i++)
            }
   #else
   static VAStatus
   surface_from_external_win32_memory(VADriverContextP ctx, vlVaSurface *surface,
               {
      vlVaDriver *drv;
   struct pipe_screen *pscreen;
   struct winsys_handle whandle;
            pscreen = VL_VA_PSCREEN(ctx);
            templat->buffer_format = surface->templat.buffer_format;
   templat->width = surface->templat.width;
            memset(&whandle, 0, sizeof(whandle));
   whandle.format = surface->templat.buffer_format;
   if (memory_type == VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE) {
      whandle.type = WINSYS_HANDLE_TYPE_FD;
      } else if (memory_type == VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE) {
      whandle.type = WINSYS_HANDLE_TYPE_D3D12_RES;
      } else {
                  surface->buffer = drv->pipe->video_buffer_from_handle(drv->pipe, templat, &whandle, PIPE_USAGE_DEFAULT);
   if (!surface->buffer) {
      result = VA_STATUS_ERROR_ALLOCATION_FAILED;
      }
         fail:
         }
      #endif
      VAStatus
   vlVaHandleSurfaceAllocate(vlVaDriver *drv, vlVaSurface *surface,
                     {
      struct pipe_surface **surfaces;
            if (modifiers_count > 0) {
      if (!drv->pipe->create_video_buffer_with_modifiers)
         surface->buffer =
      drv->pipe->create_video_buffer_with_modifiers(drv->pipe, templat,
         } else {
         }
   if (!surface->buffer)
            surfaces = surface->buffer->get_surfaces(surface->buffer);
   for (i = 0; i < VL_MAX_SURFACES; ++i) {
      union pipe_color_union c;
            if (!surfaces[i])
            if (i > !!surface->buffer->interlaced)
            drv->pipe->clear_render_target(drv->pipe, surfaces[i], &c, 0, 0,
      surfaces[i]->width, surfaces[i]->height,
   }
               }
      VAStatus
   vlVaCreateSurfaces2(VADriverContextP ctx, unsigned int format,
                     {
      vlVaDriver *drv;
      #ifdef _WIN32
         #else
         #ifdef HAVE_VA_SURFACE_ATTRIB_DRM_FORMAT_MODIFIERS
         #endif
   #endif
      struct pipe_video_buffer templat;
   struct pipe_screen *pscreen;
   int i;
   int memory_type;
   int expected_fourcc;
   VAStatus vaStatus;
   vlVaSurface *surf;
   bool protected;
   const uint64_t *modifiers;
            if (!ctx)
            if (!(width && height))
                     if (!drv)
                     if (!pscreen)
            /* Default. */
   memory_attribute = NULL;
   memory_type = VA_SURFACE_ATTRIB_MEM_TYPE_VA;
   expected_fourcc = 0;
   modifiers = NULL;
            for (i = 0; i < num_attribs && attrib_list; i++) {
      if (!(attrib_list[i].flags & VA_SURFACE_ATTRIB_SETTABLE))
            switch (attrib_list[i].type) {
   case VASurfaceAttribPixelFormat:
      if (attrib_list[i].value.type != VAGenericValueTypeInteger)
         expected_fourcc = attrib_list[i].value.value.i;
      case VASurfaceAttribMemoryType:
                           #ifdef _WIN32
               #else
               #endif
               memory_type = attrib_list[i].value.value.i;
      default:
         }
      case VASurfaceAttribExternalBufferDescriptor:
         #ifndef _WIN32
               #else
            else if (memory_type == VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE ||
      #endif
            else
      #ifndef _WIN32
   #ifdef HAVE_VA_SURFACE_ATTRIB_DRM_FORMAT_MODIFIERS
         case VASurfaceAttribDRMFormatModifiers:
      if (attrib_list[i].value.type != VAGenericValueTypePointer)
         modifier_list = attrib_list[i].value.value.p;
   if (modifier_list != NULL) {
      modifiers = modifier_list->modifiers;
         #endif
   #endif
         case VASurfaceAttribUsageHint:
      if (attrib_list[i].value.type != VAGenericValueTypeInteger)
            default:
                     protected = format & VA_RT_FORMAT_PROTECTED;
            if (VA_RT_FORMAT_YUV420 != format &&
      VA_RT_FORMAT_YUV422 != format &&
   VA_RT_FORMAT_YUV444 != format &&
   VA_RT_FORMAT_YUV400 != format &&
   VA_RT_FORMAT_YUV420_10BPP != format &&
   VA_RT_FORMAT_RGBP != format &&
   VA_RT_FORMAT_RGB32  != format) {
               switch (memory_type) {
   case VA_SURFACE_ATTRIB_MEM_TYPE_VA:
      #ifdef _WIN32
            case VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE:
   case VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE:
   if (!win32_handles)
      #else
      case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME:
      if (!memory_attribute)
         if (modifiers)
            expected_fourcc = memory_attribute->pixel_format;
      case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2:
      if (!prime_desc)
            expected_fourcc = prime_desc->fourcc;
   #endif
      default:
                           templat.buffer_format = pscreen->get_video_param(
      pscreen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_BITSTREAM,
               if (modifiers)
         else
      templat.interlaced =
      pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
            if (expected_fourcc) {
         #ifndef _WIN32
         #else
         #endif
                              templat.width = width;
   templat.height = height;
   if (protected)
                     mtx_lock(&drv->mutex);
   for (i = 0; i < num_surfaces; i++) {
      surf = CALLOC(1, sizeof(vlVaSurface));
   if (!surf) {
      vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
                        switch (memory_type) {
   case VA_SURFACE_ATTRIB_MEM_TYPE_VA:
      /* The application will clear the TILING flag when the surface is
   * intended to be exported as dmabuf. Adding shared flag because not
   * null memory_attribute means VASurfaceAttribExternalBuffers is used.
   */
   if (memory_attribute &&
         vaStatus = vlVaHandleSurfaceAllocate(drv, surf, &templat, modifiers,
                  if (vaStatus != VA_STATUS_SUCCESS)
         #ifdef _WIN32
         case VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE:
   case VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE:
      vaStatus = surface_from_external_win32_memory(ctx, surf, memory_type, win32_handles[i], &templat);
   if (vaStatus != VA_STATUS_SUCCESS)
      #else
         case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME:
      vaStatus = surface_from_external_memory(ctx, surf, memory_attribute, i, &templat);
   if (vaStatus != VA_STATUS_SUCCESS)
               case VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2:
      vaStatus = surface_from_prime_2(ctx, surf, prime_desc, &templat);
   if (vaStatus != VA_STATUS_SUCCESS)
      #endif
         default:
                  util_dynarray_init(&surf->subpics, NULL);
   surfaces[i] = handle_table_add(drv->htab, surf);
   if (!surfaces[i]) {
      vaStatus = VA_STATUS_ERROR_ALLOCATION_FAILED;
         }
                  destroy_surf:
            free_surf:
            no_res:
      mtx_unlock(&drv->mutex);
   if (i)
               }
      VAStatus
   vlVaQueryVideoProcFilters(VADriverContextP ctx, VAContextID context,
         {
               if (!ctx)
            if (!num_filters || !filters)
                                 }
      VAStatus
   vlVaQueryVideoProcFilterCaps(VADriverContextP ctx, VAContextID context,
               {
               if (!ctx)
            if (!filter_caps || !num_filter_caps)
                     switch (type) {
   case VAProcFilterNone:
         case VAProcFilterDeinterlacing: {
               if (*num_filter_caps < 3) {
      *num_filter_caps = 3;
               deint[i++].type = VAProcDeinterlacingBob;
   deint[i++].type = VAProcDeinterlacingWeave;
   deint[i++].type = VAProcDeinterlacingMotionAdaptive;
               case VAProcFilterNoiseReduction:
   case VAProcFilterSharpening:
   case VAProcFilterColorBalance:
   case VAProcFilterSkinToneEnhancement:
         default:
                              }
      static VAProcColorStandardType vpp_input_color_standards[] = {
      VAProcColorStandardBT601,
      };
      static VAProcColorStandardType vpp_output_color_standards[] = {
      VAProcColorStandardBT601,
      };
      VAStatus
   vlVaQueryVideoProcPipelineCaps(VADriverContextP ctx, VAContextID context,
               {
               if (!ctx)
            if (!pipeline_cap)
            if (num_filters && !filters)
            pipeline_cap->pipeline_flags = 0;
   pipeline_cap->filter_flags = 0;
   pipeline_cap->num_forward_references = 0;
   pipeline_cap->num_backward_references = 0;
   pipeline_cap->num_input_color_standards = ARRAY_SIZE(vpp_input_color_standards);
   pipeline_cap->input_color_standards = vpp_input_color_standards;
   pipeline_cap->num_output_color_standards = ARRAY_SIZE(vpp_output_color_standards);
            struct pipe_screen *pscreen = VL_VA_PSCREEN(ctx);
   uint32_t pipe_orientation_flags = pscreen->get_video_param(pscreen,
                        pipeline_cap->rotation_flags = VA_ROTATION_NONE;
   if(pipe_orientation_flags & PIPE_VIDEO_VPP_ROTATION_90)
         if(pipe_orientation_flags & PIPE_VIDEO_VPP_ROTATION_180)
         if(pipe_orientation_flags & PIPE_VIDEO_VPP_ROTATION_270)
            pipeline_cap->mirror_flags = VA_MIRROR_NONE;
   if(pipe_orientation_flags & PIPE_VIDEO_VPP_FLIP_HORIZONTAL)
         if(pipe_orientation_flags & PIPE_VIDEO_VPP_FLIP_VERTICAL)
            pipeline_cap->max_input_width = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->max_input_height = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->min_input_width = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->min_input_height = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->max_output_width = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->max_output_height = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->min_output_width = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->min_output_height = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  uint32_t pipe_blend_modes = pscreen->get_video_param(pscreen, PIPE_VIDEO_PROFILE_UNKNOWN,
                  pipeline_cap->blend_flags = 0;
   if (pipe_blend_modes & PIPE_VIDEO_VPP_BLEND_MODE_GLOBAL_ALPHA)
            for (i = 0; i < num_filters; i++) {
      vlVaBuffer *buf = handle_table_get(VL_VA_DRIVER(ctx)->htab, filters[i]);
            if (!buf || buf->type != VAProcFilterParameterBufferType)
            filter = buf->data;
   switch (filter->type) {
   case VAProcFilterDeinterlacing: {
      VAProcFilterParameterBufferDeinterlacing *deint = buf->data;
   if (deint->algorithm == VAProcDeinterlacingMotionAdaptive) {
      pipeline_cap->num_forward_references = 2;
      }
      }
   default:
                        }
      #ifndef _WIN32
   static uint32_t pipe_format_to_drm_format(enum pipe_format format)
   {
      switch (format) {
   case PIPE_FORMAT_R8_UNORM:
         case PIPE_FORMAT_R8G8_UNORM:
         case PIPE_FORMAT_R16_UNORM:
         case PIPE_FORMAT_R16G16_UNORM:
         case PIPE_FORMAT_B8G8R8A8_UNORM:
         case PIPE_FORMAT_R8G8B8A8_UNORM:
         case PIPE_FORMAT_B8G8R8X8_UNORM:
         case PIPE_FORMAT_R8G8B8X8_UNORM:
         case PIPE_FORMAT_NV12:
         case PIPE_FORMAT_P010:
         case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_R8G8_R8B8_UNORM:
         default:
            }
   #endif
      #if VA_CHECK_VERSION(1, 1, 0)
   VAStatus
   vlVaExportSurfaceHandle(VADriverContextP ctx,
                           {
      vlVaDriver *drv;
   vlVaSurface *surf;
   struct pipe_surface **surfaces;
   struct pipe_screen *screen;
   VAStatus ret;
   unsigned int usage;
         #ifdef _WIN32
      if ((mem_type != VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE)
      && (mem_type != VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE))
         if ((flags & VA_EXPORT_SURFACE_COMPOSED_LAYERS) == 0)
      #else
      int i, p;
   if (mem_type != VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2)
      #endif
         drv    = VL_VA_DRIVER(ctx);
   screen = VL_VA_PSCREEN(ctx);
            surf = handle_table_get(drv->htab, surface_id);
   if (!surf || !surf->buffer) {
      mtx_unlock(&drv->mutex);
                        if (buffer->interlaced) {
      struct pipe_video_buffer *interlaced = buffer;
            if (!surf->deint_buffer) {
               ret = vlVaHandleSurfaceAllocate(drv, surf, &surf->templat, NULL, 0);
   if (ret != VA_STATUS_SUCCESS) {
      mtx_unlock(&drv->mutex);
               surf->deint_buffer = surf->buffer;
   surf->buffer = interlaced;
               src_rect.x0 = dst_rect.x0 = 0;
   src_rect.y0 = dst_rect.y0 = 0;
   src_rect.x1 = dst_rect.x1 = surf->templat.width;
            vl_compositor_yuv_deint_full(&drv->cstate, &drv->compositor,
                                             usage = 0;
   if (flags & VA_EXPORT_SURFACE_WRITE_ONLY)
         #ifdef _WIN32
      struct winsys_handle whandle;
   memset(&whandle, 0, sizeof(struct winsys_handle));
            if (mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE)
         else if (mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE)
            if (!screen->resource_get_handle(screen, drv->pipe, resource,
            ret = VA_STATUS_ERROR_INVALID_SURFACE;
               if (mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_NTHANDLE)
         else if (mem_type == VA_SURFACE_ATTRIB_MEM_TYPE_D3D12_RESOURCE)
         #else
      VADRMPRIMESurfaceDescriptor *desc = descriptor;
   desc->fourcc = PipeFormatToVaFourcc(buffer->buffer_format);
   desc->width  = surf->templat.width;
            for (p = 0; p < ARRAY_SIZE(desc->objects); p++) {
      struct winsys_handle whandle;
   struct pipe_resource *resource;
            if (!surfaces[p])
                     drm_format = pipe_format_to_drm_format(resource->format);
   if (drm_format == DRM_FORMAT_INVALID) {
      ret = VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE;
               memset(&whandle, 0, sizeof(whandle));
            if (!screen->resource_get_handle(screen, drv->pipe, resource,
            ret = VA_STATUS_ERROR_INVALID_SURFACE;
               desc->objects[p].fd   = (int)whandle.handle;
   /* As per VADRMPRIMESurfaceDescriptor documentation, size must be the
   * "Total size of this object (may include regions which are not part
   * of the surface)."" */
   desc->objects[p].size = (uint32_t) whandle.size;
            if (flags & VA_EXPORT_SURFACE_COMPOSED_LAYERS) {
      desc->layers[0].object_index[p] = p;
   desc->layers[0].offset[p]       = whandle.offset;
      } else {
      desc->layers[p].drm_format      = drm_format;
   desc->layers[p].num_planes      = 1;
   desc->layers[p].object_index[0] = p;
   desc->layers[p].offset[0]       = whandle.offset;
                           if (flags & VA_EXPORT_SURFACE_COMPOSED_LAYERS) {
      uint32_t drm_format = pipe_format_to_drm_format(buffer->buffer_format);
   if (drm_format == DRM_FORMAT_INVALID) {
      ret = VA_STATUS_ERROR_UNSUPPORTED_MEMORY_TYPE;
               desc->num_layers = 1;
   desc->layers[0].drm_format = drm_format;
      } else {
            #endif
                        fail:
   #ifndef _WIN32
      for (i = 0; i < p; i++)
      #else
      if(whandle.handle)
      #endif
                     }
   #endif
