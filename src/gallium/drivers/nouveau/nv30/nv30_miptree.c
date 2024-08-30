   /*
   * Copyright 2012 Red Hat Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Ben Skeggs
   *
   */
      #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_surface.h"
      #include "nv_m2mf.xml.h"
   #include "nv_object.xml.h"
   #include "nv30/nv30_screen.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_resource.h"
   #include "nv30/nv30_transfer.h"
   #include "nv30/nv30_winsys.h"
      static inline unsigned
   layer_offset(struct pipe_resource *pt, unsigned level, unsigned layer)
   {
      struct nv30_miptree *mt = nv30_miptree(pt);
            if (pt->target == PIPE_TEXTURE_CUBE)
               }
      bool
   nv30_miptree_get_handle(struct pipe_screen *pscreen,
                           {
      if (pt->target == PIPE_BUFFER)
            struct nv30_miptree *mt = nv30_miptree(pt);
            if (!mt || !mt->base.bo)
                        }
      void
   nv30_miptree_destroy(struct pipe_screen *pscreen, struct pipe_resource *pt)
   {
               nouveau_bo_ref(NULL, &mt->base.bo);
      }
      struct nv30_transfer {
      struct pipe_transfer base;
   struct nv30_rect img;
   struct nv30_rect tmp;
   unsigned nblocksx;
      };
      static inline struct nv30_transfer *
   nv30_transfer(struct pipe_transfer *ptx)
   {
         }
      static inline void
   define_rect(struct pipe_resource *pt, unsigned level, unsigned z,
               {
      struct nv30_miptree *mt = nv30_miptree(pt);
            rect->w = u_minify(pt->width0, level) << mt->ms_x;
   rect->w = util_format_get_nblocksx(pt->format, rect->w);
   rect->h = u_minify(pt->height0, level) << mt->ms_y;
   rect->h = util_format_get_nblocksy(pt->format, rect->h);
   rect->d = 1;
   rect->z = 0;
   if (mt->swizzled) {
      if (pt->target == PIPE_TEXTURE_3D) {
      rect->d = u_minify(pt->depth0, level);
      }
      } else {
                  rect->bo     = mt->base.bo;
   rect->domain = NOUVEAU_BO_VRAM;
   rect->offset = layer_offset(pt, level, z);
            rect->x0     = util_format_get_nblocksx(pt->format, x) << mt->ms_x;
   rect->y0     = util_format_get_nblocksy(pt->format, y) << mt->ms_y;
   rect->x1     = rect->x0 + (util_format_get_nblocksx(pt->format, w) << mt->ms_x);
            /* XXX There's some indication that swizzled formats > 4 bytes are treated
   * differently. However that only applies to RGBA16_FLOAT, RGBA32_FLOAT,
   * and the DXT* formats. The former aren't properly supported yet, and the
            if (mt->swizzled && rect->cpp > 4) {
      unsigned scale = rect->cpp / 4;
   rect->w *= scale;
   rect->x0 *= scale;
   rect->x1 *= scale;
      }
      }
      void
   nv30_resource_copy_region(struct pipe_context *pipe,
                           {
      struct nv30_context *nv30 = nv30_context(pipe);
            if (dstres->target == PIPE_BUFFER && srcres->target == PIPE_BUFFER) {
      nouveau_copy_buffer(&nv30->base,
                           define_rect(srcres, src_level, src_box->z, src_box->x, src_box->y,
         define_rect(dstres, dst_level, dstz, dstx, dsty,
               }
      static void
   nv30_resource_resolve(struct nv30_context *nv30,
         {
      struct nv30_miptree *src_mt = nv30_miptree(info->src.resource);
   struct nv30_rect src, dst;
            define_rect(info->src.resource, 0, info->src.box.z, info->src.box.x,
         define_rect(info->dst.resource, 0, info->dst.box.z, info->dst.box.x,
            x0 = src.x0;
   x1 = src.x1;
            /* On nv3x we must use sifm which is restricted to 1024x1024 tiles */
   for (y = src.y0; y < y1; y += h) {
      h = y1 - y;
   if (h > 1024)
            src.y0 = 0;
   src.y1 = h;
            dst.y1 = dst.y0 + (h >> src_mt->ms_y);
            for (x = x0; x < x1; x += w) {
      w = x1 - x;
                  src.offset = y * src.pitch + x * src.cpp;
   src.x0 = 0;
                  dst.offset = (y >> src_mt->ms_y) * dst.pitch +
                                 }
      void
   nv30_blit(struct pipe_context *pipe,
         {
      struct nv30_context *nv30 = nv30_context(pipe);
            if (info.src.resource->nr_samples > 1 &&
      info.dst.resource->nr_samples <= 1 &&
   !util_format_is_depth_or_stencil(info.src.resource->format) &&
   !util_format_is_pure_integer(info.src.resource->format)) {
   nv30_resource_resolve(nv30, blit_info);
               if (util_try_blit_via_copy_region(pipe, &info, nv30->render_cond_query != NULL)) {
                  if (info.mask & PIPE_MASK_S) {
      debug_printf("nv30: cannot blit stencil, skipping\n");
               if (!util_blitter_is_blit_supported(nv30->blitter, &info)) {
      debug_printf("nv30: blit unsupported %s -> %s\n",
                                    util_blitter_save_vertex_buffer_slot(nv30->blitter, nv30->vtxbuf);
   util_blitter_save_vertex_elements(nv30->blitter, nv30->vertex);
   util_blitter_save_vertex_shader(nv30->blitter, nv30->vertprog.program);
   util_blitter_save_rasterizer(nv30->blitter, nv30->rast);
   util_blitter_save_viewport(nv30->blitter, &nv30->viewport);
   util_blitter_save_scissor(nv30->blitter, &nv30->scissor);
   util_blitter_save_fragment_shader(nv30->blitter, nv30->fragprog.program);
   util_blitter_save_blend(nv30->blitter, nv30->blend);
   util_blitter_save_depth_stencil_alpha(nv30->blitter,
         util_blitter_save_stencil_ref(nv30->blitter, &nv30->stencil_ref);
   util_blitter_save_sample_mask(nv30->blitter, nv30->sample_mask, 0);
   util_blitter_save_framebuffer(nv30->blitter, &nv30->framebuffer);
   util_blitter_save_fragment_sampler_states(nv30->blitter,
               util_blitter_save_fragment_sampler_views(nv30->blitter,
         util_blitter_save_render_condition(nv30->blitter, nv30->render_cond_query,
            }
      void
   nv30_flush_resource(struct pipe_context *pipe,
         {
   }
      void *
   nv30_miptree_transfer_map(struct pipe_context *pipe, struct pipe_resource *pt,
                     {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_device *dev = nv30->screen->base.device;
   struct nv30_miptree *mt = nv30_miptree(pt);
   struct nv30_transfer *tx;
   unsigned access = 0;
            tx = CALLOC_STRUCT(nv30_transfer);
   if (!tx)
         pipe_resource_reference(&tx->base.resource, pt);
   tx->base.level = level;
   tx->base.usage = usage;
   tx->base.box = *box;
   tx->base.stride = align(util_format_get_nblocksx(pt->format, box->width) *
         tx->base.layer_stride = util_format_get_nblocksy(pt->format, box->height) *
            tx->nblocksx = util_format_get_nblocksx(pt->format, box->width);
            define_rect(pt, level, box->z, box->x, box->y,
            ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
               if (ret) {
      pipe_resource_reference(&tx->base.resource, NULL);
   FREE(tx);
               tx->tmp.domain = NOUVEAU_BO_GART;
   tx->tmp.offset = 0;
   tx->tmp.pitch  = tx->base.stride;
   tx->tmp.cpp    = tx->img.cpp;
   tx->tmp.w      = tx->nblocksx;
   tx->tmp.h      = tx->nblocksy;
   tx->tmp.d      = 1;
   tx->tmp.x0     = 0;
   tx->tmp.y0     = 0;
   tx->tmp.x1     = tx->tmp.w;
   tx->tmp.y1     = tx->tmp.h;
            if (usage & PIPE_MAP_READ) {
      bool is_3d = mt->base.base.target == PIPE_TEXTURE_3D;
   unsigned offset = tx->img.offset;
   unsigned z = tx->img.z;
   unsigned i;
   for (i = 0; i < box->depth; ++i) {
      nv30_transfer_rect(nv30, NEAREST, &tx->img, &tx->tmp);
   if (is_3d && mt->swizzled)
         else if (is_3d)
         else
            }
   tx->img.z = z;
   tx->img.offset = offset;
               if (tx->tmp.bo->map) {
      *ptransfer = &tx->base;
               if (usage & PIPE_MAP_READ)
         if (usage & PIPE_MAP_WRITE)
            ret = BO_MAP(nv30->base.screen, tx->tmp.bo, access, nv30->base.client);
   if (ret) {
      pipe_resource_reference(&tx->base.resource, NULL);
   FREE(tx);
               *ptransfer = &tx->base;
      }
      void
   nv30_miptree_transfer_unmap(struct pipe_context *pipe,
         {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_transfer *tx = nv30_transfer(ptx);
   struct nv30_miptree *mt = nv30_miptree(tx->base.resource);
            if (ptx->usage & PIPE_MAP_WRITE) {
      bool is_3d = mt->base.base.target == PIPE_TEXTURE_3D;
   for (i = 0; i < tx->base.box.depth; ++i) {
      nv30_transfer_rect(nv30, NEAREST, &tx->tmp, &tx->img);
   if (is_3d && mt->swizzled)
         else if (is_3d)
         else
                     /* Allow the copies above to finish executing before freeing the source */
   nouveau_fence_work(nv30->base.fence,
      } else {
         }
   pipe_resource_reference(&ptx->resource, NULL);
      }
      struct pipe_resource *
   nv30_miptree_create(struct pipe_screen *pscreen,
         {
      struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nv30_miptree *mt = CALLOC_STRUCT(nv30_miptree);
   struct pipe_resource *pt = &mt->base.base;
   unsigned blocksz, size;
   unsigned w, h, d, l;
            switch (tmpl->nr_samples) {
   case 4:
      mt->ms_mode = 0x00004000;
   mt->ms_x = 1;
   mt->ms_y = 1;
      case 2:
      mt->ms_mode = 0x00003000;
   mt->ms_x = 1;
   mt->ms_y = 0;
      default:
      mt->ms_mode = 0x00000000;
   mt->ms_x = 0;
   mt->ms_y = 0;
               *pt = *tmpl;
   pipe_reference_init(&pt->reference, 1);
            w = pt->width0 << mt->ms_x;
   h = pt->height0 << mt->ms_y;
   d = (pt->target == PIPE_TEXTURE_3D) ? pt->depth0 : 1;
            if ((pt->target == PIPE_TEXTURE_RECT) ||
      (pt->bind & PIPE_BIND_SCANOUT) ||
   !util_is_power_of_two_or_zero(pt->width0) ||
   !util_is_power_of_two_or_zero(pt->height0) ||
   !util_is_power_of_two_or_zero(pt->depth0) ||
   mt->ms_mode) {
   mt->uniform_pitch = util_format_get_nblocksx(pt->format, w) * blocksz;
   mt->uniform_pitch = align(mt->uniform_pitch, 64);
   if (pt->bind & PIPE_BIND_SCANOUT) {
      struct nv30_screen *screen = nv30_screen(pscreen);
   int pitch_align = MAX2(
         screen->eng3d->oclass >= NV40_3D_CLASS ? 1024 : 256,
   /* round_down_pow2(mt->uniform_pitch / 4) */
                  if (util_format_is_compressed(pt->format)) {
      // Compressed (DXT) formats are packed tightly. We don't mark them as
   // swizzled, since their layout is largely linear. However we do end up
   // omitting the LINEAR flag when texturing them, as the levels are not
      } else if (!mt->uniform_pitch) {
                  size = 0;
   for (l = 0; l <= pt->last_level; l++) {
      struct nv30_miptree_level *lvl = &mt->level[l];
   unsigned nbx = util_format_get_nblocksx(pt->format, w);
            lvl->offset = size;
   lvl->pitch  = mt->uniform_pitch;
   if (!lvl->pitch)
            lvl->zslice_size = lvl->pitch * nby;
            w = u_minify(w, 1);
   h = u_minify(h, 1);
               mt->layer_size = size;
   if (pt->target == PIPE_TEXTURE_CUBE) {
      if (!mt->uniform_pitch)
                     ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 256, size, NULL, &mt->base.bo);
   if (ret) {
      FREE(mt);
               mt->base.domain = NOUVEAU_BO_VRAM;
      }
      struct pipe_resource *
   nv30_miptree_from_handle(struct pipe_screen *pscreen,
               {
      struct nv30_miptree *mt;
            /* only supports 2D, non-mipmapped textures for the moment */
   if ((tmpl->target != PIPE_TEXTURE_2D &&
      tmpl->target != PIPE_TEXTURE_RECT) ||
   tmpl->last_level != 0 ||
   tmpl->depth0 != 1 ||
   tmpl->array_size > 1)
         mt = CALLOC_STRUCT(nv30_miptree);
   if (!mt)
            mt->base.bo = nouveau_screen_bo_from_handle(pscreen, handle, &stride);
   if (mt->base.bo == NULL) {
      FREE(mt);
               mt->base.base = *tmpl;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = pscreen;
   mt->uniform_pitch = stride;
   mt->level[0].pitch = mt->uniform_pitch;
            /* no need to adjust bo reference count */
      }
      struct pipe_surface *
   nv30_miptree_surface_new(struct pipe_context *pipe,
               {
      struct nv30_miptree *mt = nv30_miptree(pt); /* guaranteed */
   struct nv30_surface *ns;
   struct pipe_surface *ps;
            ns = CALLOC_STRUCT(nv30_surface);
   if (!ns)
                  pipe_reference_init(&ps->reference, 1);
   pipe_resource_reference(&ps->texture, pt);
   ps->context = pipe;
   ps->format = tmpl->format;
   ps->u.tex.level = tmpl->u.tex.level;
   ps->u.tex.first_layer = tmpl->u.tex.first_layer;
            ns->width = u_minify(pt->width0, ps->u.tex.level);
   ns->height = u_minify(pt->height0, ps->u.tex.level);
   ns->depth = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
   ns->offset = layer_offset(pt, ps->u.tex.level, ps->u.tex.first_layer);
   if (mt->swizzled)
         else
            /* comment says there are going to be removed, but they're used by the st */
   ps->width = ns->width;
   ps->height = ns->height;
      }
      void
   nv30_miptree_surface_del(struct pipe_context *pipe, struct pipe_surface *ps)
   {
               pipe_resource_reference(&ps->texture, NULL);
      }
