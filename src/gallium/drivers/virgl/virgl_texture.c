   /*
   * Copyright 2014, 2015 Red Hat.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
      #include "virgl_context.h"
   #include "virgl_encode.h"
   #include "virgl_resource.h"
   #include "virgl_screen.h"
      static void virgl_copy_region_with_blit(struct pipe_context *pipe,
                                       {
               memset(&blit, 0, sizeof(blit));
   blit.src.resource = src;
   blit.src.format = src->format;
   blit.src.level = src_level;
   blit.src.box = *src_box;
   blit.dst.resource = dst;
   blit.dst.format = dst->format;
   blit.dst.level = dst_level;
   blit.dst.box.x = dst_box->x;
   blit.dst.box.y = dst_box->y;
   blit.dst.box.z = dst_box->z;
   blit.dst.box.width = dst_box->width;
   blit.dst.box.height = dst_box->height;
   blit.dst.box.depth = dst_box->depth;
   blit.mask = util_format_get_mask(src->format) &
                  if (blit.mask) {
            }
      static unsigned temp_bind(unsigned orig)
   {
      unsigned warn = ~(PIPE_BIND_RENDER_TARGET | PIPE_BIND_DEPTH_STENCIL |
         if (orig & warn)
      debug_printf("VIRGL: Warning, possibly unhandled bind: %x\n",
            }
      static void virgl_init_temp_resource_from_box(struct pipe_resource *res,
                           {
      memset(res, 0, sizeof(*res));
   res->bind = temp_bind(orig->bind);
   res->format = fmt;
   res->width0 = box->width;
   res->height0 = box->height;
   res->depth0 = 1;
   res->array_size = 1;
   res->usage = PIPE_USAGE_STAGING;
            /* We must set the correct texture target and dimensions for a 3D box. */
   if (box->depth > 1 && util_max_layer(orig, level) > 0)
         else
            if (res->target != PIPE_BUFFER)
            switch (res->target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      res->array_size = box->depth;
      case PIPE_TEXTURE_3D:
      res->depth0 = box->depth;
      default:
            }
      static void *texture_transfer_map_resolve(struct pipe_context *ctx,
                                 {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_resource *vtex = virgl_resource(resource);
   struct pipe_resource templ, *resolve_tmp;
            trans = virgl_resource_create_transfer(vctx, resource,
         if (!trans)
            enum pipe_format fmt = resource->format;
   if (!virgl_has_readback_format(ctx->screen, pipe_to_virgl_format(fmt), true)) {
      if (util_format_fits_8unorm(util_format_description(fmt)))
         else if (util_format_is_pure_sint(fmt))
         else if (util_format_is_pure_uint(fmt))
         else
                     struct pipe_box dst_box = *box;
   dst_box.x = dst_box.y = dst_box.z = 0;
   if (usage & PIPE_MAP_READ) {
      /* readback should scale to the block size */
   dst_box.width = align(dst_box.width,
         dst_box.height = align(dst_box.height,
                        resolve_tmp = ctx->screen->resource_create(ctx->screen, &templ);
   if (!resolve_tmp)
            if (usage & PIPE_MAP_READ) {
      virgl_copy_region_with_blit(ctx, resolve_tmp, 0, &dst_box, resource,
                     void *ptr = virgl_resource_transfer_map(ctx, resolve_tmp, 0, usage, &dst_box,
         if (!ptr)
            /* trans->resolve_transfer owns resolve_tmp now */
            *transfer = &trans->base;
   if (fmt == resource->format) {
      trans->base.stride = trans->resolve_transfer->stride;
   trans->base.layer_stride = trans->resolve_transfer->layer_stride;
      } else {
      if (usage & PIPE_MAP_READ) {
      struct virgl_winsys *vws = virgl_screen(ctx->screen)->vws;
   void *src = ptr;
   ptr = vws->resource_map(vws, vtex->hw_res);
                  if (!util_format_translate_3d(resource->format,
                                 ptr + vtex->metadata.level_offset[level],
   trans->base.stride,
   trans->base.layer_stride,
   box->x, box->y, box->z,
   fmt,
   src,
   trans->resolve_transfer->stride,
      debug_printf("failed to translate format %s to %s\n",
                              if ((usage & PIPE_MAP_WRITE) == 0)
                     fail:
      pipe_resource_reference(&resolve_tmp, NULL);
   virgl_resource_destroy_transfer(vctx, trans);
      }
      static bool needs_resolve(struct pipe_screen *screen,
         {
      if (resource->nr_samples > 1)
            if (usage & PIPE_MAP_READ)
      return !util_format_is_depth_or_stencil(resource->format) &&
            }
      void *virgl_texture_transfer_map(struct pipe_context *ctx,
                                 {
      if (needs_resolve(ctx->screen, resource, usage))
      return texture_transfer_map_resolve(ctx, resource, level, usage, box,
            }
      static void flush_data(struct pipe_context *ctx,
               {
      struct virgl_winsys *vws = virgl_screen(ctx->screen)->vws;
   vws->transfer_put(vws, trans->hw_res, box,
            }
      void virgl_texture_transfer_unmap(struct pipe_context *ctx,
         {
      struct virgl_context *vctx = virgl_context(ctx);
   struct virgl_transfer *trans = virgl_transfer(transfer);
            if (transfer->usage & PIPE_MAP_WRITE &&
               if (trans->resolve_transfer && (trans->base.resource->format ==
      trans->resolve_transfer->resource->format)) {
                  /* FINISHME: In case the destination format isn't renderable here, the
   * blit here will currently fail. This could for instance happen if the
   * mapped resource is of a compressed format, and it's mapped with both
                  virgl_copy_region_with_blit(ctx,
                              } else
               if (trans->resolve_transfer) {
      virgl_resource_destroy_transfer(vctx,
               if (queue_unmap) {
      if (trans->copy_src_hw_res && trans->direction == VIRGL_TRANSFER_TO_HOST) {
      virgl_encode_copy_transfer(vctx, trans);
      } else if (trans->copy_src_hw_res && trans->direction == VIRGL_TRANSFER_FROM_HOST) {
      // if it is readback, then we have already encoded transfer
      } else {
            } else {
            }
      void virgl_texture_init(struct virgl_resource *res)
   {
   }
