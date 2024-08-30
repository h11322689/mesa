   /*
   * Copyright 2008 Ben Skeggs
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
   */
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_resource.h"
      uint32_t
   nv50_tex_choose_tile_dims_helper(unsigned nx, unsigned ny, unsigned nz,
         {
               if (ny > 64) tile_mode = 0x040; /* height 128 tiles */
   else
   if (ny > 32) tile_mode = 0x030; /* height 64 tiles */
   else
   if (ny > 16) tile_mode = 0x020; /* height 32 tiles */
   else
            if (!is_3d)
         else
      if (tile_mode > 0x020)
         if (nz > 16 && tile_mode < 0x020)
         if (nz > 8) return tile_mode | 0x400; /* depth 16 tiles */
   if (nz > 4) return tile_mode | 0x300; /* depth 8 tiles */
   if (nz > 2) return tile_mode | 0x200; /* depth 4 tiles */
               }
      static uint32_t
   nv50_tex_choose_tile_dims(unsigned nx, unsigned ny, unsigned nz, bool is_3d)
   {
         }
      static uint32_t
   nv50_mt_choose_storage_type(struct nv50_miptree *mt, bool compressed)
   {
      const unsigned ms = util_logbase2(mt->base.base.nr_samples);
            if (unlikely(mt->base.base.flags & NOUVEAU_RESOURCE_FLAG_LINEAR))
         if (unlikely(mt->base.base.bind & PIPE_BIND_CURSOR))
            switch (mt->base.base.format) {
   case PIPE_FORMAT_Z16_UNORM:
      tile_flags = 0x6c + ms;
      case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8X24_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      tile_flags = 0x18 + ms;
      case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      tile_flags = 0x128 + ms;
      case PIPE_FORMAT_Z32_FLOAT:
      tile_flags = 0x40 + ms;
      case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      tile_flags = 0x60 + ms;
      default:
      /* Most color formats don't work with compression. */
   compressed = false;
      case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8A8_SRGB:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_SRGB:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_SRGB:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_SRGB:
   case PIPE_FORMAT_R10G10B10A2_UNORM:
   case PIPE_FORMAT_B10G10R10A2_UNORM:
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
   case PIPE_FORMAT_R16G16B16X16_FLOAT:
   case PIPE_FORMAT_R11G11B10_FLOAT:
      switch (util_format_get_blocksizebits(mt->base.base.format)) {
   case 128:
      assert(ms < 3);
   tile_flags = 0x74;
      case 64:
      switch (ms) {
   case 2: tile_flags = 0xfc; break;
   case 3: tile_flags = 0xfd; break;
   default:
      tile_flags = 0x70;
      }
      case 32:
      if (mt->base.base.bind & PIPE_BIND_SCANOUT) {
      assert(ms == 0);
      } else {
      switch (ms) {
   case 2: tile_flags = 0xf8; break;
   case 3: tile_flags = 0xf9; break;
   default:
      tile_flags = 0x70;
         }
      case 16:
   case 8:
      tile_flags = 0x70;
      default:
         }
   if (mt->base.base.bind & PIPE_BIND_CURSOR)
               if (!compressed)
               }
      void
   nv50_miptree_destroy(struct pipe_screen *pscreen, struct pipe_resource *pt)
   {
               nouveau_fence_work(mt->base.fence, nouveau_fence_unref_bo, mt->base.bo);
   nouveau_fence_ref(NULL, &mt->base.fence);
            NOUVEAU_DRV_STAT(nouveau_screen(pscreen), tex_obj_current_count, -1);
   NOUVEAU_DRV_STAT(nouveau_screen(pscreen), tex_obj_current_bytes,
               }
      bool
   nv50_miptree_get_handle(struct pipe_screen *pscreen,
                           {
      if (pt->target == PIPE_BUFFER)
            struct nv50_miptree *mt = nv50_miptree(pt);
            if (!mt || !mt->base.bo)
                     return nouveau_screen_bo_get_handle(pscreen,
                  }
      static inline bool
   nv50_miptree_init_ms_mode(struct nv50_miptree *mt)
   {
      switch (mt->base.base.nr_samples) {
   case 8:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS8;
   mt->ms_x = 2;
   mt->ms_y = 1;
      case 4:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS4;
   mt->ms_x = 1;
   mt->ms_y = 1;
      case 2:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS2;
   mt->ms_x = 1;
      case 1:
   case 0:
      mt->ms_mode = NV50_3D_MULTISAMPLE_MODE_MS1;
      default:
      NOUVEAU_ERR("invalid nr_samples: %u\n", mt->base.base.nr_samples);
      }
      }
      bool
   nv50_miptree_init_layout_linear(struct nv50_miptree *mt, unsigned pitch_align)
   {
      struct pipe_resource *pt = &mt->base.base;
   const unsigned blocksize = util_format_get_blocksize(pt->format);
            if (util_format_is_depth_or_stencil(pt->format))
            if ((pt->last_level > 0) || (pt->depth0 > 1) || (pt->array_size > 1))
         if (mt->ms_x | mt->ms_y)
                     /* Account for very generous prefetch (allocate size as if tiled). */
   h = MAX2(h, 8);
                        }
      static void
   nv50_miptree_init_layout_video(struct nv50_miptree *mt)
   {
      const struct pipe_resource *pt = &mt->base.base;
            assert(pt->last_level == 0);
   assert(mt->ms_x == 0 && mt->ms_y == 0);
                     mt->level[0].tile_mode = 0x20;
   mt->level[0].pitch = align(pt->width0 * blocksize, 64);
            if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size, NV50_TILE_SIZE(0x20));
         }
      static void
   nv50_miptree_init_layout_tiled(struct nv50_miptree *mt)
   {
      struct pipe_resource *pt = &mt->base.base;
   unsigned w, h, d, l;
                     w = pt->width0 << mt->ms_x;
            /* For 3D textures, a mipmap is spanned by all the layers, for array
   * textures and cube maps, each layer contains its own mipmaps.
   */
            for (l = 0; l <= pt->last_level; ++l) {
      struct nv50_miptree_level *lvl = &mt->level[l];
   unsigned tsx, tsy, tsz;
   unsigned nbx = util_format_get_nblocksx(pt->format, w);
                              tsx = NV50_TILE_SIZE_X(lvl->tile_mode); /* x is tile row pitch in bytes */
   tsy = NV50_TILE_SIZE_Y(lvl->tile_mode);
                              w = u_minify(w, 1);
   h = u_minify(h, 1);
               if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size,
               }
      struct pipe_resource *
   nv50_miptree_create(struct pipe_screen *pscreen,
         {
      struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nouveau_drm *drm = nouveau_screen(pscreen)->drm;
   struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
   struct pipe_resource *pt = &mt->base.base;
   bool compressed = drm->version >= 0x01000101;
   int ret;
   union nouveau_bo_config bo_config;
   uint32_t bo_flags;
            if (!mt)
            *pt = *templ;
   pipe_reference_init(&pt->reference, 1);
            if (pt->bind & PIPE_BIND_LINEAR)
                     if (!nv50_miptree_init_ms_mode(mt)) {
      FREE(mt);
               if (unlikely(pt->flags & NV50_RESOURCE_FLAG_VIDEO)) {
      nv50_miptree_init_layout_video(mt);
   if (pt->flags & NV50_RESOURCE_FLAG_NOALLOC) {
      /* BO allocation done by client */
         } else
   if (bo_config.nv50.memtype != 0) {
         } else {
      if (pt->usage & PIPE_BIND_CURSOR)
         else if (pt->usage & PIPE_BIND_SCANOUT)
         else
         if (!nv50_miptree_init_layout_linear(mt, pitch_align)) {
      FREE(mt);
         }
            if (!bo_config.nv50.memtype && (pt->bind & PIPE_BIND_SHARED))
         else
            bo_flags = mt->base.domain | NOUVEAU_BO_NOSNOOP;
   if (mt->base.base.bind & (PIPE_BIND_CURSOR | PIPE_BIND_DISPLAY_TARGET))
            ret = nouveau_bo_new(dev, bo_flags, 4096, mt->total_size, &bo_config,
         if (ret) {
      FREE(mt);
      }
               }
      struct pipe_resource *
   nv50_miptree_from_handle(struct pipe_screen *pscreen,
               {
      struct nv50_miptree *mt;
            /* only supports 2D, non-mipmapped textures for the moment */
   if ((templ->target != PIPE_TEXTURE_2D &&
      templ->target != PIPE_TEXTURE_RECT) ||
   templ->last_level != 0 ||
   templ->depth0 != 1 ||
   templ->array_size > 1)
         mt = CALLOC_STRUCT(nv50_miptree);
   if (!mt)
            mt->base.bo = nouveau_screen_bo_from_handle(pscreen, whandle, &stride);
   if (mt->base.bo == NULL) {
      FREE(mt);
      }
   mt->base.domain = mt->base.bo->flags & NOUVEAU_BO_APER;
            mt->base.base = *templ;
   pipe_reference_init(&mt->base.base.reference, 1);
   mt->base.base.screen = pscreen;
   mt->level[0].pitch = stride;
   mt->level[0].offset = 0;
                     /* no need to adjust bo reference count */
      }
         /* Offset of zslice @z from start of level @l. */
   inline unsigned
   nv50_mt_zslice_offset(const struct nv50_miptree *mt, unsigned l, unsigned z)
   {
               unsigned tds = NV50_TILE_SHIFT_Z(mt->level[l].tile_mode);
            unsigned nby = util_format_get_nblocksy(pt->format,
            /* to next 2D tile slice within a 3D tile */
            /* to slice in the next (in z direction) 3D tile */
               }
      /* Surface functions.
   */
      struct nv50_surface *
   nv50_surface_from_miptree(struct nv50_miptree *mt,
         {
      struct pipe_surface *ps;
   struct nv50_surface *ns = CALLOC_STRUCT(nv50_surface);
   if (!ns)
                  pipe_reference_init(&ps->reference, 1);
            ps->format = templ->format;
   ps->writable = templ->writable;
   ps->u.tex.level = templ->u.tex.level;
   ps->u.tex.first_layer = templ->u.tex.first_layer;
            ns->width = u_minify(mt->base.base.width0, ps->u.tex.level);
   ns->height = u_minify(mt->base.base.height0, ps->u.tex.level);
   ns->depth = ps->u.tex.last_layer - ps->u.tex.first_layer + 1;
            /* comment says there are going to be removed, but they're used by the st */
   ps->width = ns->width;
            ns->width <<= mt->ms_x;
               }
      struct pipe_surface *
   nv50_miptree_surface_new(struct pipe_context *pipe,
               {
      struct nv50_miptree *mt = nv50_miptree(pt);
   struct nv50_surface *ns = nv50_surface_from_miptree(mt, templ);
   if (!ns)
                  if (ns->base.u.tex.first_layer) {
      const unsigned l = ns->base.u.tex.level;
            if (mt->layout_3d) {
               /* TODO: switch to depth 1 tiles; but actually this shouldn't happen */
   if (ns->depth > 1 &&
      (z & (NV50_TILE_SIZE_Z(mt->level[l].tile_mode) - 1)))
   } else {
                        }
