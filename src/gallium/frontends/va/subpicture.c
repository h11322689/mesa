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
      #include "util/u_memory.h"
   #include "util/u_handle_table.h"
   #include "util/u_sampler.h"
      #include "va_private.h"
      static VAImageFormat subpic_formats[] = {
      {
   .fourcc = VA_FOURCC_BGRA,
   .byte_order = VA_LSB_FIRST,
   .bits_per_pixel = 32,
   .depth = 32,
   .red_mask   = 0x00ff0000ul,
   .green_mask = 0x0000ff00ul,
   .blue_mask  = 0x000000fful,
   .alpha_mask = 0xff000000ul,
      };
      VAStatus
   vlVaQuerySubpictureFormats(VADriverContextP ctx, VAImageFormat *format_list,
         {
      if (!ctx)
            if (!(format_list && flags && num_formats))
            *num_formats = sizeof(subpic_formats)/sizeof(VAImageFormat);
               }
      VAStatus
   vlVaCreateSubpicture(VADriverContextP ctx, VAImageID image,
         {
      vlVaDriver *drv;
   vlVaSubpicture *sub;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
   mtx_lock(&drv->mutex);
   img = handle_table_get(drv->htab, image);
   if (!img) {
      mtx_unlock(&drv->mutex);
               sub = CALLOC(1, sizeof(*sub));
   if (!sub) {
      mtx_unlock(&drv->mutex);
               sub->image = img;
   *subpicture = handle_table_add(VL_VA_DRIVER(ctx)->htab, sub);
               }
      VAStatus
   vlVaDestroySubpicture(VADriverContextP ctx, VASubpictureID subpicture)
   {
      vlVaDriver *drv;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
            sub = handle_table_get(drv->htab, subpicture);
   if (!sub) {
      mtx_unlock(&drv->mutex);
               FREE(sub);
   handle_table_remove(drv->htab, subpicture);
               }
      VAStatus
   vlVaSubpictureImage(VADriverContextP ctx, VASubpictureID subpicture, VAImageID image)
   {
      vlVaDriver *drv;
   vlVaSubpicture *sub;
            if (!ctx)
            drv = VL_VA_DRIVER(ctx);
            img = handle_table_get(drv->htab, image);
   if (!img) {
      mtx_unlock(&drv->mutex);
               sub = handle_table_get(drv->htab, subpicture);
   mtx_unlock(&drv->mutex);
   if (!sub)
                        }
      VAStatus
   vlVaSetSubpictureChromakey(VADriverContextP ctx, VASubpictureID subpicture,
         {
      if (!ctx)
               }
      VAStatus
   vlVaSetSubpictureGlobalAlpha(VADriverContextP ctx, VASubpictureID subpicture, float global_alpha)
   {
      if (!ctx)
               }
      VAStatus
   vlVaAssociateSubpicture(VADriverContextP ctx, VASubpictureID subpicture,
                           VASurfaceID *target_surfaces, int num_surfaces,
   {
      vlVaSubpicture *sub;
   struct pipe_resource tex_temp, *tex;
   struct pipe_sampler_view sampler_templ;
   vlVaDriver *drv;
   vlVaSurface *surf;
   int i;
   struct u_rect src_rect = {src_x, src_x + src_width, src_y, src_y + src_height};
            if (!ctx)
         drv = VL_VA_DRIVER(ctx);
            sub = handle_table_get(drv->htab, subpicture);
   if (!sub) {
      mtx_unlock(&drv->mutex);
               for (i = 0; i < num_surfaces; i++) {
      surf = handle_table_get(drv->htab, target_surfaces[i]);
   if (!surf) {
      mtx_unlock(&drv->mutex);
                  sub->src_rect = src_rect;
            memset(&tex_temp, 0, sizeof(tex_temp));
   tex_temp.target = PIPE_TEXTURE_2D;
   tex_temp.format = PIPE_FORMAT_B8G8R8A8_UNORM;
   tex_temp.last_level = 0;
   tex_temp.width0 = src_width;
   tex_temp.height0 = src_height;
   tex_temp.depth0 = 1;
   tex_temp.array_size = 1;
   tex_temp.usage = PIPE_USAGE_DYNAMIC;
   tex_temp.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
   tex_temp.flags = 0;
   if (!drv->pipe->screen->is_format_supported(
         drv->pipe->screen, tex_temp.format, tex_temp.target,
      mtx_unlock(&drv->mutex);
                        memset(&sampler_templ, 0, sizeof(sampler_templ));
   u_sampler_view_default_template(&sampler_templ, tex, tex->format);
   sub->sampler = drv->pipe->create_sampler_view(drv->pipe, tex, &sampler_templ);
   pipe_resource_reference(&tex, NULL);
   if (!sub->sampler) {
      mtx_unlock(&drv->mutex);
               for (i = 0; i < num_surfaces; i++) {
      surf = handle_table_get(drv->htab, target_surfaces[i]);
   if (!surf) {
      mtx_unlock(&drv->mutex);
      }
      }
               }
      VAStatus
   vlVaDeassociateSubpicture(VADriverContextP ctx, VASubpictureID subpicture,
         {
      int i;
   int j;
   vlVaSurface *surf;
   vlVaSubpicture *sub, **array;
            if (!ctx)
         drv = VL_VA_DRIVER(ctx);
            sub = handle_table_get(drv->htab, subpicture);
   if (!sub) {
      mtx_unlock(&drv->mutex);
               for (i = 0; i < num_surfaces; i++) {
      surf = handle_table_get(drv->htab, target_surfaces[i]);
   if (!surf) {
      mtx_unlock(&drv->mutex);
               array = surf->subpics.data;
   if (!array)
            for (j = 0; j < surf->subpics.size/sizeof(vlVaSubpicture *); j++) {
      if (array[j] == sub)
               while (surf->subpics.size && util_dynarray_top(&surf->subpics, vlVaSubpicture *) == NULL)
      }
   pipe_sampler_view_reference(&sub->sampler,NULL);
               }
