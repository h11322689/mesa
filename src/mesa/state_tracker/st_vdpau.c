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
      /*
   * Authors:
   *      Christian König <christian.koenig@amd.com>
   *
   */
      #ifdef HAVE_ST_VDPAU
      #include "main/texobj.h"
   #include "main/teximage.h"
   #include "main/errors.h"
   #include "program/prog_instruction.h"
      #include "pipe/p_state.h"
   #include "pipe/p_video_codec.h"
      #include "util/u_inlines.h"
      #include "st_vdpau.h"
   #include "st_context.h"
   #include "st_sampler_view.h"
   #include "st_texture.h"
   #include "st_format.h"
   #include "st_cb_flush.h"
      #include "frontend/vdpau_interop.h"
   #include "frontend/vdpau_dmabuf.h"
   #include "frontend/vdpau_funcs.h"
   #include "frontend/drm_driver.h"
      #include "drm-uapi/drm_fourcc.h"
      static struct pipe_resource *
   st_vdpau_video_surface_gallium(struct gl_context *ctx, const void *vdpSurface,
         {
      int (*getProcAddr)(uint32_t device, uint32_t id, void **ptr);
   uint32_t device = (uintptr_t)ctx->vdpDevice;
   struct pipe_sampler_view *sv;
            struct pipe_video_buffer *buffer;
   struct pipe_sampler_view **samplers;
            getProcAddr = (void *)ctx->vdpGetProcAddress;
   if (getProcAddr(device, VDP_FUNC_ID_VIDEO_SURFACE_GALLIUM, (void**)&f))
            buffer = f((uintptr_t)vdpSurface);
   if (!buffer)
            samplers = buffer->get_sampler_view_planes(buffer);
   if (!samplers)
            sv = samplers[index >> 1];
   if (!sv)
            pipe_resource_reference(&res, sv->texture);
      }
      static struct pipe_resource *
   st_vdpau_output_surface_gallium(struct gl_context *ctx, const void *vdpSurface)
   {
      int (*getProcAddr)(uint32_t device, uint32_t id, void **ptr);
   uint32_t device = (uintptr_t)ctx->vdpDevice;
   struct pipe_resource *res = NULL;
            getProcAddr = (void *)ctx->vdpGetProcAddress;
   if (getProcAddr(device, VDP_FUNC_ID_OUTPUT_SURFACE_GALLIUM, (void**)&f))
            pipe_resource_reference(&res, f((uintptr_t)vdpSurface));
      }
      static struct pipe_resource *
   st_vdpau_resource_from_description(struct gl_context *ctx,
         {
      struct st_context *st = st_context(ctx);
   struct pipe_resource templ, *res;
            if (desc->handle == -1)
            memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.depth0 = 1;
   templ.array_size = 1;
   templ.width0 = desc->width;
   templ.height0 = desc->height;
   templ.format = VdpFormatRGBAToPipe(desc->format);
   templ.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
            memset(&whandle, 0, sizeof(whandle));
   whandle.type = WINSYS_HANDLE_TYPE_FD;
   whandle.handle = desc->handle;
   whandle.modifier = DRM_FORMAT_MOD_INVALID;
   whandle.offset = desc->offset;
   whandle.stride = desc->stride;
            res = st->screen->resource_from_handle(st->screen, &templ, &whandle,
                     }
      static struct pipe_resource *
   st_vdpau_output_surface_dma_buf(struct gl_context *ctx, const void *vdpSurface)
   {
      int (*getProcAddr)(uint32_t device, uint32_t id, void **ptr);
            struct VdpSurfaceDMABufDesc desc;
            getProcAddr = (void *)ctx->vdpGetProcAddress;
   if (getProcAddr(device, VDP_FUNC_ID_OUTPUT_SURFACE_DMA_BUF, (void**)&f))
            if (f((uintptr_t)vdpSurface, &desc) != VDP_STATUS_OK)
               }
      static struct pipe_resource *
   st_vdpau_video_surface_dma_buf(struct gl_context *ctx, const void *vdpSurface,
         {
      int (*getProcAddr)(uint32_t device, uint32_t id, void **ptr);
            struct VdpSurfaceDMABufDesc desc;
            getProcAddr = (void *)ctx->vdpGetProcAddress;
   if (getProcAddr(device, VDP_FUNC_ID_VIDEO_SURFACE_DMA_BUF, (void**)&f))
            if (f((uintptr_t)vdpSurface, index, &desc) != VDP_STATUS_OK)
               }
      void
   st_vdpau_map_surface(struct gl_context *ctx, GLenum target, GLenum access,
                     {
      struct st_context *st = st_context(ctx);
            struct pipe_resource *res;
   mesa_format texFormat;
            if (output) {
               if (!res)
         } else {
               if (!res) {
      res = st_vdpau_video_surface_gallium(ctx, vdpSurface, index);
                  /* If the resource is from a different screen, try re-importing it */
   if (res && res->screen != screen) {
      struct pipe_resource *new_res = NULL;
   struct winsys_handle whandle = { .type = WINSYS_HANDLE_TYPE_FD };
            if (screen->get_param(screen, PIPE_CAP_DMABUF) &&
      res->screen->get_param(res->screen, PIPE_CAP_DMABUF) &&
   res->screen->resource_get_handle(res->screen, NULL, res, &whandle,
         whandle.modifier = DRM_FORMAT_MOD_INVALID;
   new_res = screen->resource_from_handle(screen, res, &whandle, usage);
               pipe_resource_reference(&res, NULL);
               if (!res) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
               /* switch to surface based */
   if (!texObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj, NULL);
                        _mesa_init_teximage_fields(ctx, texImage,
                        pipe_resource_reference(&texObj->pt, res);
   st_texture_release_all_sampler_views(st, texObj);
            texObj->surface_format = res->format;
   texObj->level_override = -1;
            _mesa_dirty_texobj(ctx, texObj);
      }
      void
   st_vdpau_unmap_surface(struct gl_context *ctx, GLenum target, GLenum access,
                     {
               pipe_resource_reference(&texObj->pt, NULL);
   st_texture_release_all_sampler_views(st, texObj);
            texObj->level_override = -1;
                     /* NV_vdpau_interop does not specify an explicit synchronization mechanism
   * between the GL and VDPAU contexts. Provide automatic synchronization here.
   */
      }
      #endif
