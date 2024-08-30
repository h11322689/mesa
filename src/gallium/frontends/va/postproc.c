   /**************************************************************************
   *
   * Copyright 2015 Advanced Micro Devices, Inc.
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
      #include "util/u_handle_table.h"
   #include "util/u_memory.h"
   #include "util/u_compute.h"
      #include "vl/vl_defines.h"
   #include "vl/vl_video_buffer.h"
   #include "vl/vl_deint_filter.h"
   #include "vl/vl_winsys.h"
      #include "va_private.h"
      static const VARectangle *
   vlVaRegionDefault(const VARectangle *region, vlVaSurface *surf,
         {
      if (region)
            def->x = 0;
   def->y = 0;
   def->width = surf->templat.width;
               }
      static VAStatus
   vlVaPostProcCompositor(vlVaDriver *drv, vlVaContext *context,
                        const VARectangle *src_region,
      {
      struct pipe_surface **surfaces;
   struct u_rect src_rect;
            surfaces = dst->get_surfaces(dst);
   if (!surfaces || !surfaces[0])
            src_rect.x0 = src_region->x;
   src_rect.y0 = src_region->y;
   src_rect.x1 = src_region->x + src_region->width;
            dst_rect.x0 = dst_region->x;
   dst_rect.y0 = dst_region->y;
   dst_rect.x1 = dst_region->x + dst_region->width;
            vl_compositor_clear_layers(&drv->cstate);
   vl_compositor_set_buffer_layer(&drv->cstate, &drv->compositor, 0, src,
         vl_compositor_set_layer_dst_area(&drv->cstate, 0, &dst_rect);
            drv->pipe->flush(drv->pipe, NULL, 0);
      }
      static void vlVaGetBox(struct pipe_video_buffer *buf, unsigned idx,
         {
      unsigned plane = buf->interlaced ? idx / 2: idx;
            x = abs(region->x);
   y = abs(region->y);
   width = region->width;
            vl_video_buffer_adjust_size(&x, &y, plane,
               vl_video_buffer_adjust_size(&width, &height, plane,
                  box->x = region->x < 0 ? -x : x;
   box->y = region->y < 0 ? -y : y;
   box->width = width;
      }
      static bool vlVaGetFullRange(vlVaSurface *surface, uint8_t va_range)
   {
      if (va_range != VA_SOURCE_RANGE_UNKNOWN)
            /* Assume limited for YUV, full for RGB */
      }
      static unsigned vlVaGetChromaLocation(unsigned va_chroma_location,
         {
               if (util_format_get_plane_height(format, 1, 4) != 4) {
      /* Bits 0-1 */
   switch (va_chroma_location & 3) {
   case VA_CHROMA_SITING_VERTICAL_TOP:
      ret |= VL_COMPOSITOR_LOCATION_VERTICAL_TOP;
      case VA_CHROMA_SITING_VERTICAL_BOTTOM:
      ret |= VL_COMPOSITOR_LOCATION_VERTICAL_BOTTOM;
      case VA_CHROMA_SITING_VERTICAL_CENTER:
   default:
      ret |= VL_COMPOSITOR_LOCATION_VERTICAL_CENTER;
                  if (util_format_is_subsampled_422(format) ||
      util_format_get_plane_width(format, 1, 4) != 4) {
   /* Bits 2-3 */
   switch (va_chroma_location & 12) {
   case VA_CHROMA_SITING_HORIZONTAL_CENTER:
      ret |= VL_COMPOSITOR_LOCATION_HORIZONTAL_CENTER;
      case VA_CHROMA_SITING_HORIZONTAL_LEFT:
   default:
      ret |= VL_COMPOSITOR_LOCATION_HORIZONTAL_LEFT;
                     }
      static void vlVaSetProcParameters(vlVaDriver *drv,
                     {
      enum VL_CSC_COLOR_STANDARD color_standard;
   bool src_yuv = util_format_is_yuv(src->buffer->buffer_format);
            if (src_yuv == dst_yuv) {
         } else if (src_yuv) {
      switch (param->surface_color_standard) {
   case VAProcColorStandardBT601:
      color_standard = VL_CSC_COLOR_STANDARD_BT_601;
      case VAProcColorStandardBT709:
   default:
      color_standard = src->full_range ?
      VL_CSC_COLOR_STANDARD_BT_709_FULL :
            } else {
                  vl_csc_get_matrix(color_standard, NULL, dst->full_range, &drv->csc);
            if (src_yuv)
      drv->cstate.chroma_location =
      vlVaGetChromaLocation(param->input_color_properties.chroma_sample_location,
   else if (dst_yuv)
      drv->cstate.chroma_location =
         }
      static VAStatus vlVaVidEngineBlit(vlVaDriver *drv, vlVaContext *context,
                                       {
      if (deinterlace != VL_COMPOSITOR_NONE)
            if (!drv->pipe->screen->is_video_format_supported(drv->pipe->screen,
                              if (!drv->pipe->screen->is_video_format_supported(drv->pipe->screen,
                              struct u_rect src_rect;
            src_rect.x0 = src_region->x;
   src_rect.y0 = src_region->y;
   src_rect.x1 = src_region->x + src_region->width;
            dst_rect.x0 = dst_region->x;
   dst_rect.y0 = dst_region->y;
   dst_rect.x1 = dst_region->x + dst_region->width;
            context->desc.vidproc.base.input_format = src->buffer_format;
            context->desc.vidproc.src_region = src_rect;
            if (param->rotation_state == VA_ROTATION_NONE)
         else if (param->rotation_state == VA_ROTATION_90)
         else if (param->rotation_state == VA_ROTATION_180)
         else if (param->rotation_state == VA_ROTATION_270)
            if (param->mirror_state == VA_MIRROR_HORIZONTAL)
         if (param->mirror_state == VA_MIRROR_VERTICAL)
            memset(&context->desc.vidproc.blend, 0, sizeof(context->desc.vidproc.blend));
   context->desc.vidproc.blend.mode = PIPE_VIDEO_VPP_BLEND_MODE_NONE;
   if (param->blend_state != NULL) {
      if (param->blend_state->flags & VA_BLEND_GLOBAL_ALPHA) {
      context->desc.vidproc.blend.mode = PIPE_VIDEO_VPP_BLEND_MODE_GLOBAL_ALPHA;
                  if (context->needs_begin_frame) {
      context->decoder->begin_frame(context->decoder, dst,
            }
               }
      static VAStatus vlVaPostProcBlit(vlVaDriver *drv, vlVaContext *context,
                                 {
      struct pipe_surface **src_surfaces;
   struct pipe_surface **dst_surfaces;
   struct u_rect src_rect;
   struct u_rect dst_rect;
   bool scale = false;
   bool grab = false;
            if ((src->buffer_format == PIPE_FORMAT_B8G8R8X8_UNORM ||
      src->buffer_format == PIPE_FORMAT_B8G8R8A8_UNORM ||
   src->buffer_format == PIPE_FORMAT_R8G8B8X8_UNORM ||
   src->buffer_format == PIPE_FORMAT_R8G8B8A8_UNORM) &&
   !src->interlaced)
         if ((src->width != dst->width || src->height != dst->height) &&
      (src->interlaced && dst->interlaced))
         src_surfaces = src->get_surfaces(src);
   if (!src_surfaces || !src_surfaces[0])
            if (scale || (src->interlaced != dst->interlaced && dst->interlaced)) {
               surf = handle_table_get(drv->htab, context->target_id);
   if (!surf)
         surf->templat.interlaced = false;
            if (vlVaHandleSurfaceAllocate(drv, surf, &surf->templat, NULL, 0) != VA_STATUS_SUCCESS)
                        dst_surfaces = dst->get_surfaces(dst);
   if (!dst_surfaces || !dst_surfaces[0])
            src_rect.x0 = src_region->x;
   src_rect.y0 = src_region->y;
   src_rect.x1 = src_region->x + src_region->width;
            dst_rect.x0 = dst_region->x;
   dst_rect.y0 = dst_region->y;
   dst_rect.x1 = dst_region->x + dst_region->width;
            if (grab) {
      vl_compositor_convert_rgb_to_yuv(&drv->cstate, &drv->compositor, 0,
                              if (src->buffer_format == PIPE_FORMAT_YUYV ||
      src->buffer_format == PIPE_FORMAT_UYVY ||
   src->buffer_format == PIPE_FORMAT_YV12 ||
   src->buffer_format == PIPE_FORMAT_IYUV) {
   vl_compositor_yuv_deint_full(&drv->cstate, &drv->compositor,
                              if (src->interlaced != dst->interlaced) {
      deinterlace = deinterlace ? deinterlace : VL_COMPOSITOR_WEAVE;
   vl_compositor_yuv_deint_full(&drv->cstate, &drv->compositor,
                              for (i = 0; i < VL_MAX_SURFACES; ++i) {
      struct pipe_surface *from = src_surfaces[i];
            if (src->interlaced) {
      /* Not 100% accurate, but close enough */
   switch (deinterlace) {
   case VL_COMPOSITOR_BOB_TOP:
      from = src_surfaces[i & ~1];
      case VL_COMPOSITOR_BOB_BOTTOM:
      from = src_surfaces[(i & ~1) + 1];
      default:
                     if (!from || !dst_surfaces[i])
            memset(&blit, 0, sizeof(blit));
   blit.src.resource = from->texture;
   blit.src.format = from->format;
   blit.src.level = 0;
   blit.src.box.z = from->u.tex.first_layer;
   blit.src.box.depth = 1;
            blit.dst.resource = dst_surfaces[i]->texture;
   blit.dst.format = dst_surfaces[i]->format;
   blit.dst.level = 0;
   blit.dst.box.z = dst_surfaces[i]->u.tex.first_layer;
   blit.dst.box.depth = 1;
            blit.mask = PIPE_MASK_RGBA;
            if (drv->pipe->screen->get_param(drv->pipe->screen,
               else
               // TODO: figure out why this is necessary for DMA-buf sharing
               }
      static struct pipe_video_buffer *
   vlVaApplyDeint(vlVaDriver *drv, vlVaContext *context,
                     {
               if (param->num_forward_references < 2 ||
      param->num_backward_references < 1)
         prevprev = handle_table_get(drv->htab, param->forward_references[1]);
   prev = handle_table_get(drv->htab, param->forward_references[0]);
            if (!prevprev || !prev || !next)
            if (context->deint && (context->deint->video_width != current->width ||
      context->deint->video_height != current->height)) {
   vl_deint_filter_cleanup(context->deint);
   FREE(context->deint);
               if (!context->deint) {
      context->deint = MALLOC(sizeof(struct vl_deint_filter));
   if (!vl_deint_filter_init(context->deint, drv->pipe, current->width,
            FREE(context->deint);
   context->deint = NULL;
                  if (!vl_deint_filter_check_buffers(context->deint, prevprev->buffer,
                  vl_deint_filter_render(context->deint, prevprev->buffer, prev->buffer,
            }
      static bool can_convert_with_efc(vlVaSurface *src, vlVaSurface *dst)
   {
               if (src->buffer->interlaced)
                     if (src_format != PIPE_FORMAT_B8G8R8A8_UNORM &&
      src_format != PIPE_FORMAT_R8G8B8A8_UNORM &&
   src_format != PIPE_FORMAT_B8G8R8X8_UNORM &&
   src_format != PIPE_FORMAT_R8G8B8X8_UNORM)
         dst_format = dst->encoder_format != PIPE_FORMAT_NONE ?
               }
      VAStatus
   vlVaHandleVAProcPipelineParameterBufferType(vlVaDriver *drv, vlVaContext *context, vlVaBuffer *buf)
   {
      enum vl_compositor_deinterlace deinterlace = VL_COMPOSITOR_NONE;
   VARectangle def_src_region, def_dst_region;
   const VARectangle *src_region, *dst_region;
   VAProcPipelineParameterBuffer *param;
   struct pipe_video_buffer *src, *dst;
   vlVaSurface *src_surface, *dst_surface;
   unsigned i;
   struct pipe_screen *pscreen;
            if (!drv || !context)
            if (!buf || !buf->data)
            if (!context->target)
                     src_surface = handle_table_get(drv->htab, param->surface);
   dst_surface = handle_table_get(drv->htab, context->target_id);
   if (!src_surface || !dst_surface)
         if (!src_surface->buffer || !dst_surface->buffer)
            src_surface->full_range = vlVaGetFullRange(src_surface,
         dst_surface->full_range = vlVaGetFullRange(dst_surface,
                     src_region = vlVaRegionDefault(param->surface_region, src_surface, &def_src_region);
            if (!param->num_filters &&
      src_region->width == dst_region->width &&
   src_region->height == dst_region->height &&
   can_convert_with_efc(src_surface, dst_surface) &&
   pscreen->get_video_param(pscreen,
                                 // EFC will convert the buffer to a format the encoder accepts
   if (src_surface->buffer->buffer_format != surf->buffer->buffer_format) {
               surf->templat.interlaced = src_surface->templat.interlaced;
                  if (vlVaHandleSurfaceAllocate(drv, surf, &surf->templat, NULL, 0) != VA_STATUS_SUCCESS)
               pipe_resource_reference(&(((struct vl_video_buffer *)(surf->buffer))->resources[0]), ((struct vl_video_buffer *)(src_surface->buffer))->resources[0]);
                        src = src_surface->buffer;
            /* convert the destination buffer to progressive if we're deinterlacing
         if (param->num_filters && dst->interlaced) {
      vlVaSurface *surf;
   surf = dst_surface;
   surf->templat.interlaced = false;
            if (vlVaHandleSurfaceAllocate(drv, surf, &surf->templat, NULL, 0) != VA_STATUS_SUCCESS)
                        for (i = 0; i < param->num_filters; i++) {
      vlVaBuffer *buf = handle_table_get(drv->htab, param->filters[i]);
            if (!buf || buf->type != VAProcFilterParameterBufferType)
            filter = buf->data;
   switch (filter->type) {
   case VAProcFilterDeinterlacing: {
      VAProcFilterParameterBufferDeinterlacing *deint = buf->data;
   switch (deint->algorithm) {
   case VAProcDeinterlacingBob:
      if (deint->flags & VA_DEINTERLACING_BOTTOM_FIELD)
         else
               case VAProcDeinterlacingWeave:
                  case VAProcDeinterlacingMotionAdaptive:
   !!(deint->flags & VA_DEINTERLACING_BOTTOM_FIELD));
                        default:
         }
   drv->compositor.deinterlace = deinterlace;
               default:
                     /* If the driver supports video engine post proc, attempt to do that
   * if it fails, fallback to the other existing implementations below
   */
   if (pscreen->get_video_param(pscreen,
                        if (!context->decoder) {
      context->decoder = drv->pipe->create_video_codec(drv->pipe, &context->templat);
   if (!context->decoder)
               context->desc.vidproc.src_surface_fence = src_surface->fence;
   /* Perform VPBlit, if fail, fallback to other implementations below */
   if (VA_STATUS_SUCCESS == vlVaVidEngineBlit(drv, context, src_region, dst_region,
                              /* Try other post proc implementations */
   if (context->target->buffer_format != PIPE_FORMAT_NV12 &&
      context->target->buffer_format != PIPE_FORMAT_P010 &&
   context->target->buffer_format != PIPE_FORMAT_P016)
   ret = vlVaPostProcCompositor(drv, context, src_region, dst_region,
      else
      ret = vlVaPostProcBlit(drv, context, src_region, dst_region,
         /* Reset chroma location */
               }
