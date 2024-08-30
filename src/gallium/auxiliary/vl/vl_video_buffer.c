   /**************************************************************************
   *
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
      #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "pipe/p_state.h"
      #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_sampler.h"
   #include "util/u_memory.h"
      #include "vl_video_buffer.h"
      const unsigned const_resource_plane_order_YUV[3] = {
      0,
   1,
      };
      const unsigned const_resource_plane_order_YVU[3] = {
      0,
   2,
      };
      void
   vl_get_video_buffer_formats(struct pipe_screen *screen, enum pipe_format format,
         {
      unsigned num_planes = util_format_get_num_planes(format);
            for (i = 0; i < num_planes; i++)
         for (; i < VL_NUM_COMPONENTS; i++)
            if (format == PIPE_FORMAT_YUYV)
         else if (format == PIPE_FORMAT_UYVY)
      }
      const unsigned *
   vl_video_buffer_plane_order(enum pipe_format format)
   {
      switch(format) {
   case PIPE_FORMAT_YV12:
   case PIPE_FORMAT_IYUV:
            case PIPE_FORMAT_NV12:
   case PIPE_FORMAT_NV21:
   case PIPE_FORMAT_Y8_U8_V8_444_UNORM:
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_YUYV:
   case PIPE_FORMAT_UYVY:
   case PIPE_FORMAT_P010:
   case PIPE_FORMAT_P016:
            default:
            }
      static enum pipe_format
   vl_video_buffer_surface_format(enum pipe_format format)
   {
               /* a subsampled formats can't work as surface use RGBA instead */
   if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
               }
      bool
   vl_video_buffer_is_format_supported(struct pipe_screen *screen,
                     {
      enum pipe_format resource_formats[VL_NUM_COMPONENTS];
                     for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
               if (format == PIPE_FORMAT_NONE)
            /* we at least need to sample from it */
   if (!screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, 0, PIPE_BIND_SAMPLER_VIEW))
            format = vl_video_buffer_surface_format(format);
   if (!screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 0, 0, PIPE_BIND_RENDER_TARGET))
                  }
      unsigned
   vl_video_buffer_max_size(struct pipe_screen *screen)
   {
         }
      void
   vl_video_buffer_set_associated_data(struct pipe_video_buffer *vbuf,
                     {
               if (vbuf->associated_data == associated_data)
            if (vbuf->associated_data)
            vbuf->associated_data = associated_data;
      }
      void *
   vl_video_buffer_get_associated_data(struct pipe_video_buffer *vbuf,
         {
      if (vbuf->codec == vcodec)
         else
      }
      void
   vl_video_buffer_template(struct pipe_resource *templ,
                           const struct pipe_video_buffer *tmpl,
   {
               memset(templ, 0, sizeof(*templ));
   if (depth > 1)
         else if (array_size > 1)
         else
         templ->format = resource_format;
   templ->width0 = tmpl->width;
   templ->depth0 = depth;
   templ->array_size = array_size;
   templ->bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET | tmpl->bind;
            vl_video_buffer_adjust_size(&templ->width0, &height, plane,
            }
      void
   vl_video_buffer_destroy(struct pipe_video_buffer *buffer)
   {
      struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
                     for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      pipe_sampler_view_reference(&buf->sampler_view_planes[i], NULL);
   pipe_sampler_view_reference(&buf->sampler_view_components[i], NULL);
               for (i = 0; i < VL_MAX_SURFACES; ++i)
                        }
      static void
   vl_video_buffer_resources(struct pipe_video_buffer *buffer,
         {
      struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   unsigned num_planes = util_format_get_num_planes(buffer->buffer_format);
                     for (i = 0; i < num_planes; ++i) {
            }
      static struct pipe_sampler_view **
   vl_video_buffer_sampler_view_planes(struct pipe_video_buffer *buffer)
   {
      struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   unsigned num_planes = util_format_get_num_planes(buffer->buffer_format);
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
                              for (i = 0; i < num_planes; ++i ) {
      if (!buf->sampler_view_planes[i]) {
                                    buf->sampler_view_planes[i] = pipe->create_sampler_view(pipe, buf->resources[i], &sv_templ);
   if (!buf->sampler_view_planes[i])
                        error:
      for (i = 0; i < num_planes; ++i )
               }
      static struct pipe_sampler_view **
   vl_video_buffer_sampler_view_components(struct pipe_video_buffer *buffer)
   {
      struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_sampler_view sv_templ;
   struct pipe_context *pipe;
   enum pipe_format sampler_format[VL_NUM_COMPONENTS];
   const unsigned *plane_order;
                              vl_get_video_buffer_formats(pipe->screen, buf->base.buffer_format, sampler_format);
            for (component = 0, i = 0; i < buf->num_planes; ++i ) {
      struct pipe_resource *res = buf->resources[plane_order[i]];
   const struct util_format_description *desc = util_format_description(res->format);
   unsigned nr_components = util_format_get_nr_components(res->format);
   if (desc->layout == UTIL_FORMAT_LAYOUT_SUBSAMPLED)
            for (j = 0; j < nr_components && component < VL_NUM_COMPONENTS; ++j, ++component) {
                              memset(&sv_templ, 0, sizeof(sv_templ));
   u_sampler_view_default_template(&sv_templ, res, sampler_format[plane_order[i]]);
   pipe_swizzle = (buf->base.buffer_format == PIPE_FORMAT_YUYV || buf->base.buffer_format == PIPE_FORMAT_UYVY) ?
                              buf->sampler_view_components[component] = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!buf->sampler_view_components[component])
         }
                  error:
      for (i = 0; i < VL_NUM_COMPONENTS; ++i )
               }
      static struct pipe_surface **
   vl_video_buffer_surfaces(struct pipe_video_buffer *buffer)
   {
      struct vl_video_buffer *buf = (struct vl_video_buffer *)buffer;
   struct pipe_surface surf_templ;
   struct pipe_context *pipe;
                              array_size = buffer->interlaced ? 2 : 1;
   for (i = 0, surf = 0; i < VL_NUM_COMPONENTS; ++i) {
      for (j = 0; j < array_size; ++j, ++surf) {
               if (!buf->resources[i]) {
      pipe_surface_reference(&buf->surfaces[surf], NULL);
               if (!buf->surfaces[surf]) {
      memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = vl_video_buffer_surface_format(buf->resources[i]->format);
   surf_templ.u.tex.first_layer = surf_templ.u.tex.last_layer = j;
   buf->surfaces[surf] = pipe->create_surface(pipe, buf->resources[i], &surf_templ);
   if (!buf->surfaces[surf])
                           error:
      for (i = 0; i < VL_MAX_SURFACES; ++i )
               }
      struct pipe_video_buffer *
   vl_video_buffer_create(struct pipe_context *pipe,
         {
      enum pipe_format resource_formats[VL_NUM_COMPONENTS];
   struct pipe_video_buffer templat, *result;
            assert(pipe);
            pot_buffers = !pipe->screen->get_video_param
   (
      pipe->screen,
   PIPE_VIDEO_PROFILE_UNKNOWN,
   PIPE_VIDEO_ENTRYPOINT_UNKNOWN,
                        templat = *tmpl;
   templat.width = pot_buffers ? util_next_power_of_two(tmpl->width)
         templat.height = pot_buffers ? util_next_power_of_two(tmpl->height)
            if (tmpl->interlaced)
            result = vl_video_buffer_create_ex
   (
      pipe, &templat, resource_formats,
   1, tmpl->interlaced ? 2 : 1, PIPE_USAGE_DEFAULT,
                  if (result && tmpl->interlaced)
               }
      struct pipe_video_buffer *
   vl_video_buffer_create_ex(struct pipe_context *pipe,
                           {
      struct pipe_resource res_tmpl;
   struct pipe_resource *resources[VL_NUM_COMPONENTS];
                              vl_video_buffer_template(&res_tmpl, tmpl, resource_formats[0], depth, array_size,
         resources[0] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[0])
            if (resource_formats[1] == PIPE_FORMAT_NONE) {
      assert(resource_formats[2] == PIPE_FORMAT_NONE);
               vl_video_buffer_template(&res_tmpl, tmpl, resource_formats[1], depth, array_size,
         resources[1] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[1])
            if (resource_formats[2] == PIPE_FORMAT_NONE)
            vl_video_buffer_template(&res_tmpl, tmpl, resource_formats[2], depth, array_size,
         resources[2] = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!resources[2])
                  error:
      for (i = 0; i < VL_NUM_COMPONENTS; ++i)
               }
      struct pipe_video_buffer *
   vl_video_buffer_create_ex2(struct pipe_context *pipe,
               {
      struct vl_video_buffer *buffer;
            buffer = CALLOC_STRUCT(vl_video_buffer);
   if (!buffer)
            buffer->base = *tmpl;
   buffer->base.context = pipe;
   buffer->base.destroy = vl_video_buffer_destroy;
   buffer->base.get_resources = vl_video_buffer_resources;
   buffer->base.get_sampler_view_planes = vl_video_buffer_sampler_view_planes;
   buffer->base.get_sampler_view_components = vl_video_buffer_sampler_view_components;
   buffer->base.get_surfaces = vl_video_buffer_surfaces;
            for (i = 0; i < VL_NUM_COMPONENTS; ++i) {
      buffer->resources[i] = resources[i];
   if (resources[i])
                  }
      /* Create pipe_video_buffer by using resource_create with planar formats. */
   struct pipe_video_buffer *
   vl_video_buffer_create_as_resource(struct pipe_context *pipe,
                     {
      struct pipe_resource templ, *resources[VL_NUM_COMPONENTS] = {0};
            memset(&templ, 0, sizeof(templ));
   templ.target = array_size > 1 ? PIPE_TEXTURE_2D_ARRAY : PIPE_TEXTURE_2D;
   templ.width0 = align(tmpl->width, VL_MACROBLOCK_WIDTH);
   templ.height0 = align(tmpl->height / array_size, VL_MACROBLOCK_HEIGHT);
   templ.depth0 = 1;
   templ.array_size = array_size;
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET | tmpl->bind;
            if (tmpl->buffer_format == PIPE_FORMAT_YUYV)
         else if (tmpl->buffer_format == PIPE_FORMAT_UYVY)
         else
            if (modifiers)
      resources[0] = pipe->screen->resource_create_with_modifiers(pipe->screen,
            else
         if (!resources[0])
            if (resources[0]->next) {
      pipe_resource_reference(&resources[1], resources[0]->next);
   if (resources[1]->next)
               struct pipe_video_buffer vidtemplate = *tmpl;
   vidtemplate.width = templ.width0;
   vidtemplate.height = templ.height0 * array_size;
      }
