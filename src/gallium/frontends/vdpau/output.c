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
      #include <vdpau/vdpau.h>
      #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_sampler.h"
   #include "util/format/u_format.h"
   #include "util/u_surface.h"
      #include "vl/vl_csc.h"
      #include "frontend/drm_driver.h"
      #include "vdpau_private.h"
      /**
   * Create a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfaceCreate(VdpDevice device,
                     {
      struct pipe_context *pipe;
   struct pipe_resource res_tmpl, *res;
   struct pipe_sampler_view sv_templ;
                     if (!(width && height))
            vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)
            pipe = dev->context;
   if (!pipe)
            vlsurface = CALLOC(1, sizeof(vlVdpOutputSurface));
   if (!vlsurface)
                              /*
   * The output won't look correctly when this buffer is send to X,
   * if the VDPAU RGB component order doesn't match the X11 one so
   * we only allow the X11 format
   */
   vlsurface->send_to_X = dev->vscreen->color_depth == 24 &&
            res_tmpl.target = PIPE_TEXTURE_2D;
   res_tmpl.format = VdpFormatRGBAToPipe(rgba_format);
   res_tmpl.width0 = width;
   res_tmpl.height0 = height;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET |
                           if (!CheckSurfaceParams(pipe->screen, &res_tmpl))
            res = pipe->screen->resource_create(pipe->screen, &res_tmpl);
   if (!res)
            vlVdpDefaultSamplerViewTemplate(&sv_templ, res);
   vlsurface->sampler_view = pipe->create_sampler_view(pipe, res, &sv_templ);
   if (!vlsurface->sampler_view)
            memset(&surf_templ, 0, sizeof(surf_templ));
   surf_templ.format = res->format;
   vlsurface->surface = pipe->create_surface(pipe, res, &surf_templ);
   if (!vlsurface->surface)
            *surface = vlAddDataHTAB(vlsurface);
   if (*surface == 0)
                     if (!vl_compositor_init_state(&vlsurface->cstate, pipe))
            vl_compositor_reset_dirty_area(&vlsurface->dirty_area);
                  err_resource:
      pipe_sampler_view_reference(&vlsurface->sampler_view, NULL);
   pipe_surface_reference(&vlsurface->surface, NULL);
      err_unlock:
      mtx_unlock(&dev->mutex);
   DeviceReference(&vlsurface->device, NULL);
   FREE(vlsurface);
      }
      /**
   * Destroy a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfaceDestroy(VdpOutputSurface surface)
   {
      vlVdpOutputSurface *vlsurface;
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
                              pipe_surface_reference(&vlsurface->surface, NULL);
   pipe_sampler_view_reference(&vlsurface->sampler_view, NULL);
   pipe->screen->fence_reference(pipe->screen, &vlsurface->fence, NULL);
   vl_compositor_cleanup_state(&vlsurface->cstate);
            vlRemoveDataHTAB(surface);
   DeviceReference(&vlsurface->device, NULL);
               }
      /**
   * Retrieve the parameters used to create a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfaceGetParameters(VdpOutputSurface surface,
               {
               vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
            *rgba_format = PipeToFormatRGBA(vlsurface->sampler_view->texture->format);
   *width = vlsurface->sampler_view->texture->width0;
               }
      /**
   * Copy image data from a VdpOutputSurface to application memory in the
   * surface's native format.
   */
   VdpStatus
   vlVdpOutputSurfaceGetBitsNative(VdpOutputSurface surface,
                     {
      vlVdpOutputSurface *vlsurface;
   struct pipe_context *pipe;
   struct pipe_resource *res;
   struct pipe_box box;
   struct pipe_transfer *transfer;
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
            pipe = vlsurface->device->context;
   if (!pipe)
            if (!destination_data || !destination_pitches)
                     res = vlsurface->sampler_view->texture;
   box = RectToPipeBox(source_rect, res);
   map = pipe->texture_map(pipe, res, 0, PIPE_MAP_READ, &box, &transfer);
   if (!map) {
      mtx_unlock(&vlsurface->device->mutex);
               util_copy_rect(*destination_data, res->format, *destination_pitches, 0, 0,
            pipe_texture_unmap(pipe, transfer);
               }
      /**
   * Copy image data from application memory in the surface's native format to
   * a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfacePutBitsNative(VdpOutputSurface surface,
                     {
      vlVdpOutputSurface *vlsurface;
   struct pipe_box dst_box;
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
            pipe = vlsurface->device->context;
   if (!pipe)
            if (!source_data || !source_pitches)
                              /* Check for a no-op. (application bug?) */
   if (!dst_box.width || !dst_box.height) {
      mtx_unlock(&vlsurface->device->mutex);
               pipe->texture_subdata(pipe, vlsurface->sampler_view->texture, 0,
                           }
      /**
   * Copy image data from application memory in a specific indexed format to
   * a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfacePutBitsIndexed(VdpOutputSurface surface,
                                       {
      vlVdpOutputSurface *vlsurface;
   struct pipe_context *context;
   struct vl_compositor *compositor;
            enum pipe_format index_format;
            struct pipe_resource *res, res_tmpl;
   struct pipe_sampler_view sv_tmpl;
            struct pipe_box box;
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
            context = vlsurface->device->context;
   compositor = &vlsurface->device->compositor;
            index_format = FormatIndexedToPipe(source_indexed_format);
   if (index_format == PIPE_FORMAT_NONE)
            if (!source_data || !source_pitch)
            colortbl_format = FormatColorTableToPipe(color_table_format);
   if (colortbl_format == PIPE_FORMAT_NONE)
            if (!color_table)
            memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_2D;
            if (destination_rect) {
      if (destination_rect->x1 > destination_rect->x0 &&
      destination_rect->y1 > destination_rect->y0) {
   res_tmpl.width0 = destination_rect->x1 - destination_rect->x0;
         } else {
      res_tmpl.width0 = vlsurface->surface->texture->width0;
      }
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_STAGING;
                     if (!CheckSurfaceParams(context->screen, &res_tmpl))
            res = context->screen->resource_create(context->screen, &res_tmpl);
   if (!res)
            box.x = box.y = box.z = 0;
   box.width = res->width0;
   box.height = res->height0;
            context->texture_subdata(context, res, 0, PIPE_MAP_WRITE, &box,
                  memset(&sv_tmpl, 0, sizeof(sv_tmpl));
            sv_idx = context->create_sampler_view(context, res, &sv_tmpl);
            if (!sv_idx)
            memset(&res_tmpl, 0, sizeof(res_tmpl));
   res_tmpl.target = PIPE_TEXTURE_1D;
   res_tmpl.format = colortbl_format;
   res_tmpl.width0 = 1 << util_format_get_component_bits(
         res_tmpl.height0 = 1;
   res_tmpl.depth0 = 1;
   res_tmpl.array_size = 1;
   res_tmpl.usage = PIPE_USAGE_STAGING;
            res = context->screen->resource_create(context->screen, &res_tmpl);
   if (!res)
            box.x = box.y = box.z = 0;
   box.width = res->width0;
   box.height = res->height0;
            context->texture_subdata(context, res, 0, PIPE_MAP_WRITE, &box, color_table,
            memset(&sv_tmpl, 0, sizeof(sv_tmpl));
            sv_tbl = context->create_sampler_view(context, res, &sv_tmpl);
            if (!sv_tbl)
            vl_compositor_clear_layers(cstate);
   vl_compositor_set_palette_layer(cstate, compositor, 0, sv_idx, sv_tbl, NULL, NULL, false);
   vl_compositor_set_layer_dst_area(cstate, 0, RectToPipe(destination_rect, &dst_rect));
            pipe_sampler_view_reference(&sv_idx, NULL);
   pipe_sampler_view_reference(&sv_tbl, NULL);
                  error_resource:
      pipe_sampler_view_reference(&sv_idx, NULL);
   pipe_sampler_view_reference(&sv_tbl, NULL);
   mtx_unlock(&vlsurface->device->mutex);
      }
      /**
   * Copy image data from application memory in a specific YCbCr format to
   * a VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfacePutBitsYCbCr(VdpOutputSurface surface,
                                 {
      vlVdpOutputSurface *vlsurface;
   struct vl_compositor *compositor;
            struct pipe_context *pipe;
   enum pipe_format format;
   struct pipe_video_buffer vtmpl, *vbuffer;
   struct u_rect dst_rect;
                     vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface)
               pipe = vlsurface->device->context;
   compositor = &vlsurface->device->compositor;
            format = FormatYCBCRToPipe(source_ycbcr_format);
   if (format == PIPE_FORMAT_NONE)
            if (!source_data || !source_pitches)
            mtx_lock(&vlsurface->device->mutex);
   memset(&vtmpl, 0, sizeof(vtmpl));
            if (destination_rect) {
      if (destination_rect->x1 > destination_rect->x0 &&
      destination_rect->y1 > destination_rect->y0) {
   vtmpl.width = destination_rect->x1 - destination_rect->x0;
         } else {
      vtmpl.width = vlsurface->surface->texture->width0;
               vbuffer = pipe->create_video_buffer(pipe, &vtmpl);
   if (!vbuffer) {
      mtx_unlock(&vlsurface->device->mutex);
               sampler_views = vbuffer->get_sampler_view_planes(vbuffer);
   if (!sampler_views) {
      vbuffer->destroy(vbuffer);
   mtx_unlock(&vlsurface->device->mutex);
               for (i = 0; i < 3; ++i) {
      struct pipe_sampler_view *sv = sampler_views[i];
            struct pipe_box dst_box = {
      0, 0, 0,
               pipe->texture_subdata(pipe, sv->texture, 0, PIPE_MAP_WRITE, &dst_box,
               if (!csc_matrix) {
      vl_csc_matrix csc;
   vl_csc_get_matrix(VL_CSC_COLOR_STANDARD_BT_601, NULL, 1, &csc);
   if (!vl_compositor_set_csc_matrix(cstate, (const vl_csc_matrix*)&csc, 1.0f, 0.0f))
      } else {
      if (!vl_compositor_set_csc_matrix(cstate, csc_matrix, 1.0f, 0.0f))
               vl_compositor_clear_layers(cstate);
   vl_compositor_set_buffer_layer(cstate, compositor, 0, vbuffer, NULL, NULL, VL_COMPOSITOR_WEAVE);
   vl_compositor_set_layer_dst_area(cstate, 0, RectToPipe(destination_rect, &dst_rect));
            vbuffer->destroy(vbuffer);
               err_csc_matrix:
      vbuffer->destroy(vbuffer);
   mtx_unlock(&vlsurface->device->mutex);
      }
      static unsigned
   BlendFactorToPipe(VdpOutputSurfaceRenderBlendFactor factor)
   {
      switch (factor) {
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ZERO:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_ALPHA:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_DST_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_SRC_ALPHA_SATURATE:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_CONSTANT_ALPHA:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA:
         default:
      assert(0);
         }
      static unsigned
   BlendEquationToPipe(VdpOutputSurfaceRenderBlendEquation equation)
   {
      switch (equation) {
   case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_SUBTRACT:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_REVERSE_SUBTRACT:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_ADD:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MIN:
         case VDP_OUTPUT_SURFACE_RENDER_BLEND_EQUATION_MAX:
         default:
      assert(0);
         }
      static void *
   BlenderToPipe(struct pipe_context *context,
         {
               memset(&blend, 0, sizeof blend);
            if (blend_state) {
      blend.rt[0].blend_enable = 1;
   blend.rt[0].rgb_src_factor = BlendFactorToPipe(blend_state->blend_factor_source_color);
   blend.rt[0].rgb_dst_factor = BlendFactorToPipe(blend_state->blend_factor_destination_color);
   blend.rt[0].alpha_src_factor = BlendFactorToPipe(blend_state->blend_factor_source_alpha);
   blend.rt[0].alpha_dst_factor = BlendFactorToPipe(blend_state->blend_factor_destination_alpha);
   blend.rt[0].rgb_func = BlendEquationToPipe(blend_state->blend_equation_color);
      } else {
                  blend.logicop_enable = 0;
   blend.logicop_func = PIPE_LOGICOP_CLEAR;
   blend.rt[0].colormask = PIPE_MASK_RGBA;
               }
      static struct vertex4f *
   ColorsToPipe(VdpColor const *colors, uint32_t flags, struct vertex4f result[4])
   {
      unsigned i;
            if (!colors)
            for (i = 0; i < 4; ++i) {
      dst->x = colors->red;
   dst->y = colors->green;
   dst->z = colors->blue;
            ++dst;
   if (flags & VDP_OUTPUT_SURFACE_RENDER_COLOR_PER_VERTEX)
      }
      }
      /**
   * Composite a sub-rectangle of a VdpOutputSurface into a sub-rectangle of
   * another VdpOutputSurface; Output Surface object VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfaceRenderOutputSurface(VdpOutputSurface destination_surface,
                                       {
               struct pipe_context *context;
   struct pipe_sampler_view *src_sv;
   struct vl_compositor *compositor;
                     struct vertex4f vlcolors[4];
            dst_vlsurface = vlGetDataHTAB(destination_surface);
   if (!dst_vlsurface)
            if (source_surface == VDP_INVALID_HANDLE) {
            } else {
      vlVdpOutputSurface *src_vlsurface = vlGetDataHTAB(source_surface);
   if (!src_vlsurface)
            if (dst_vlsurface->device != src_vlsurface->device)
                                 context = dst_vlsurface->device->context;
   compositor = &dst_vlsurface->device->compositor;
                     vl_compositor_clear_layers(cstate);
   vl_compositor_set_layer_blend(cstate, 0, blend, false);
   vl_compositor_set_rgba_layer(cstate, compositor, 0, src_sv,
               STATIC_ASSERT(VL_COMPOSITOR_ROTATE_0 == VDP_OUTPUT_SURFACE_RENDER_ROTATE_0);
   STATIC_ASSERT(VL_COMPOSITOR_ROTATE_90 == VDP_OUTPUT_SURFACE_RENDER_ROTATE_90);
   STATIC_ASSERT(VL_COMPOSITOR_ROTATE_180 == VDP_OUTPUT_SURFACE_RENDER_ROTATE_180);
   STATIC_ASSERT(VL_COMPOSITOR_ROTATE_270 == VDP_OUTPUT_SURFACE_RENDER_ROTATE_270);
   vl_compositor_set_layer_rotation(cstate, 0, flags & 3);
   vl_compositor_set_layer_dst_area(cstate, 0, RectToPipe(destination_rect, &dst_rect));
            context->delete_blend_state(context, blend);
               }
      /**
   * Composite a sub-rectangle of a VdpBitmapSurface into a sub-rectangle of
   * a VdpOutputSurface; Output Surface object VdpOutputSurface.
   */
   VdpStatus
   vlVdpOutputSurfaceRenderBitmapSurface(VdpOutputSurface destination_surface,
                                       {
               struct pipe_context *context;
   struct pipe_sampler_view *src_sv;
   struct vl_compositor *compositor;
                     struct vertex4f vlcolors[4];
            dst_vlsurface = vlGetDataHTAB(destination_surface);
   if (!dst_vlsurface)
            if (source_surface == VDP_INVALID_HANDLE) {
            } else {
      vlVdpBitmapSurface *src_vlsurface = vlGetDataHTAB(source_surface);
   if (!src_vlsurface)
            if (dst_vlsurface->device != src_vlsurface->device)
                        context = dst_vlsurface->device->context;
   compositor = &dst_vlsurface->device->compositor;
                              vl_compositor_clear_layers(cstate);
   vl_compositor_set_layer_blend(cstate, 0, blend, false);
   vl_compositor_set_rgba_layer(cstate, compositor, 0, src_sv,
               vl_compositor_set_layer_rotation(cstate, 0, flags & 3);
   vl_compositor_set_layer_dst_area(cstate, 0, RectToPipe(destination_rect, &dst_rect));
            context->delete_blend_state(context, blend);
               }
      struct pipe_resource *vlVdpOutputSurfaceGallium(VdpOutputSurface surface)
   {
               vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface || !vlsurface->surface)
            mtx_lock(&vlsurface->device->mutex);
   vlsurface->device->context->flush(vlsurface->device->context, NULL, 0);
               }
      VdpStatus vlVdpOutputSurfaceDMABuf(VdpOutputSurface surface,
         {
      vlVdpOutputSurface *vlsurface;
   struct pipe_screen *pscreen;
            memset(result, 0, sizeof(*result));
            vlsurface = vlGetDataHTAB(surface);
   if (!vlsurface || !vlsurface->surface)
            mtx_lock(&vlsurface->device->mutex);
            memset(&whandle, 0, sizeof(struct winsys_handle));
            pscreen = vlsurface->surface->texture->screen;
   if (!pscreen->resource_get_handle(pscreen, vlsurface->device->context,
                  mtx_unlock(&vlsurface->device->mutex);
                        result->handle = whandle.handle;
   result->width = vlsurface->surface->width;
   result->height = vlsurface->surface->height;
   result->offset = whandle.offset;
   result->stride = whandle.stride;
               }
