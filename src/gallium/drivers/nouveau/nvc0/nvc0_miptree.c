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
      #include "drm-uapi/drm_fourcc.h"
      #include "pipe/p_state.h"
   #include "pipe/p_defines.h"
   #include "frontend/drm_driver.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_resource.h"
      static uint32_t
   nvc0_tex_choose_tile_dims(unsigned nx, unsigned ny, unsigned nz, bool is_3d)
   {
         }
      static uint32_t
   tu102_choose_tiled_storage_type(enum pipe_format format,
                  {
               switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (compressed)
         else
            case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8X24_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      if (compressed)
         else
            case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (compressed)
         else
            case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (compressed)
         else
            case PIPE_FORMAT_Z32_FLOAT:
   default:
      kind = 0x06;
                  }
      uint32_t
   nvc0_choose_tiled_storage_type(struct pipe_screen *pscreen,
                     {
               if (nouveau_screen(pscreen)->device->chipset >= 0x160)
            switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (compressed)
         else
            case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8X24_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      if (compressed)
         else
            case PIPE_FORMAT_X24S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (compressed)
         else
            case PIPE_FORMAT_Z32_FLOAT:
      if (compressed)
         else
            case PIPE_FORMAT_X32_S8X24_UINT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (compressed)
         else
            default:
      switch (util_format_get_blocksizebits(format)) {
   case 128:
      if (compressed)
         else
            case 64:
      if (compressed) {
      switch (ms) {
   case 0: tile_flags = 0xe6; break;
   case 1: tile_flags = 0xeb; break;
   case 2: tile_flags = 0xed; break;
   case 3: tile_flags = 0xf2; break;
   default:
            } else {
         }
      case 32:
      if (compressed && ms) {
      switch (ms) {
         case 0: tile_flags = 0xdb; break;
         case 1: tile_flags = 0xdd; break;
   case 2: tile_flags = 0xdf; break;
   case 3: tile_flags = 0xe4; break;
   default:
            } else {
         }
      case 16:
   case 8:
      tile_flags = 0xfe;
      default:
         }
                  }
      static uint32_t
   nvc0_mt_choose_storage_type(struct pipe_screen *pscreen,
               {
               if (unlikely(mt->base.base.bind & PIPE_BIND_CURSOR))
         if (unlikely(mt->base.base.flags & NOUVEAU_RESOURCE_FLAG_LINEAR))
               }
      static inline bool
   nvc0_miptree_init_ms_mode(struct nv50_miptree *mt)
   {
      switch (mt->base.base.nr_samples) {
   case 8:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS8;
   mt->ms_x = 2;
   mt->ms_y = 1;
      case 4:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS4;
   mt->ms_x = 1;
   mt->ms_y = 1;
      case 2:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS2;
   mt->ms_x = 1;
      case 1:
   case 0:
      mt->ms_mode = NVC0_3D_MULTISAMPLE_MODE_MS1;
      default:
      NOUVEAU_ERR("invalid nr_samples: %u\n", mt->base.base.nr_samples);
      }
      }
      static void
   nvc0_miptree_init_layout_video(struct nv50_miptree *mt)
   {
      const struct pipe_resource *pt = &mt->base.base;
            assert(pt->last_level == 0);
   assert(mt->ms_x == 0 && mt->ms_y == 0);
                     mt->level[0].tile_mode = 0x10;
   mt->level[0].pitch = align(pt->width0 * blocksize, 64);
            if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size, NVC0_TILE_SIZE(0x10));
         }
      static void
   nvc0_miptree_init_layout_tiled(struct nv50_miptree *mt, uint64_t modifier)
   {
      struct pipe_resource *pt = &mt->base.base;
   unsigned w, h, d, l;
                     w = pt->width0 << mt->ms_x;
            /* For 3D textures, a mipmap is spanned by all the layers, for array
   * textures and cube maps, each layer contains its own mipmaps.
   */
            assert(!mt->ms_mode || !pt->last_level);
   assert(modifier == DRM_FORMAT_MOD_INVALID ||
                  for (l = 0; l <= pt->last_level; ++l) {
      struct nv50_miptree_level *lvl = &mt->level[l];
   unsigned tsx, tsy, tsz;
   unsigned nbx = util_format_get_nblocksx(pt->format, w);
                     if (modifier != DRM_FORMAT_MOD_INVALID)
      /* Extract the log2(block height) field from the modifier and pack it
   * into tile_mode's y field. Other tile dimensions are always 1
   * (represented using 0 here) for 2D surfaces, and non-2D surfaces are
   * not supported by the current modifiers (asserted above). Note the
   * modifier must be validated prior to calling this function.
   */
      else
            tsx = NVC0_TILE_SIZE_X(lvl->tile_mode); /* x is tile row pitch in bytes */
   tsy = NVC0_TILE_SIZE_Y(lvl->tile_mode);
                              w = u_minify(w, 1);
   h = u_minify(h, 1);
               if (pt->array_size > 1) {
      mt->layer_stride = align(mt->total_size,
               }
      static uint64_t
   nvc0_miptree_get_modifier(struct pipe_screen *pscreen, struct nv50_miptree *mt)
   {
      const union nouveau_bo_config *config = &mt->base.bo->config;
   const uint32_t uc_kind =
      nvc0_choose_tiled_storage_type(pscreen,
                           if (mt->layout_3d)
         if (mt->base.base.nr_samples > 1)
         if (config->nvc0.memtype == 0x00)
         if (NVC0_TILE_MODE_Y(config->nvc0.tile_mode) > 5)
         if (config->nvc0.memtype != uc_kind)
            return DRM_FORMAT_MOD_NVIDIA_BLOCK_LINEAR_2D(
            0,
   nouveau_screen(pscreen)->tegra_sector_layout ? 0 : 1,
   kind_gen,
   }
      bool
   nvc0_miptree_get_handle(struct pipe_screen *pscreen,
                           {
      struct nv50_miptree *mt = nv50_miptree(pt);
            ret = nv50_miptree_get_handle(pscreen, context, pt, whandle, usage);
   if (!ret)
                        }
      static uint64_t
   nvc0_miptree_select_best_modifier(struct pipe_screen *pscreen,
                     {
      /*
   * Supported block heights are 1,2,4,8,16,32, stored as log2() their
   * value. Reserve one slot for each, as well as the linear modifier.
   */
   uint64_t prio_supported_mods[] = {
      DRM_FORMAT_MOD_INVALID,
   DRM_FORMAT_MOD_INVALID,
   DRM_FORMAT_MOD_INVALID,
   DRM_FORMAT_MOD_INVALID,
   DRM_FORMAT_MOD_INVALID,
   DRM_FORMAT_MOD_INVALID,
      };
   const uint32_t uc_kind = nvc0_mt_choose_storage_type(pscreen, mt, false);
   int top_mod_slot = ARRAY_SIZE(prio_supported_mods);
   const uint32_t kind_gen = nvc0_get_kind_generation(pscreen);
   unsigned int i;
            if (uc_kind != 0u) {
      const struct pipe_resource *pt = &mt->base.base;
   const unsigned nbx = util_format_get_nblocksx(pt->format, pt->width0);
   const unsigned nby = util_format_get_nblocksy(pt->format, pt->height0);
   const uint32_t lbh_preferred =
         uint32_t lbh = lbh_preferred;
   bool dec_lbh = true;
            for (i = 0; i < ARRAY_SIZE(prio_supported_mods) - 1; i++) {
      assert(lbh <= 5u);
                  /*
   * The preferred block height is the largest block size that doesn't
   * waste excessive space with unused padding bytes relative to the
   * height of the image.  Construct the priority array such that
   * the preferred block height is highest priority, followed by
   * progressively smaller block sizes down to a block height of one,
   * followed by progressively larger (more wasteful) block sizes up
   * to 5.
   */
   if (lbh == 0u) {
      lbh = lbh_preferred + 1u;
      } else if (dec_lbh) {
         } else {
                        assert(prio_supported_mods[ARRAY_SIZE(prio_supported_mods) - 1] ==
            for (i = 0u; i < count; i++) {
      for (p = 0; p < ARRAY_SIZE(prio_supported_mods); p++) {
      if (prio_supported_mods[p] != DRM_FORMAT_MOD_INVALID) {
      if (modifiers[i] == DRM_FORMAT_MOD_INVALID ||
      prio_supported_mods[p] == modifiers[i]) {
   if (top_mod_slot > p) top_mod_slot = p;
                        if (top_mod_slot >= ARRAY_SIZE(prio_supported_mods))
               }
      struct pipe_resource *
   nvc0_miptree_create(struct pipe_screen *pscreen,
               {
      struct nouveau_device *dev = nouveau_screen(pscreen)->device;
   struct nouveau_drm *drm = nouveau_screen(pscreen)->drm;
   struct nv50_miptree *mt = CALLOC_STRUCT(nv50_miptree);
   struct pipe_resource *pt = &mt->base.base;
   bool compressed = drm->version >= 0x01000101;
   int ret;
   union nouveau_bo_config bo_config;
   uint32_t bo_flags;
   unsigned pitch_align;
            if (!mt)
            *pt = *templ;
   pipe_reference_init(&pt->reference, 1);
            if (pt->usage == PIPE_USAGE_STAGING) {
      /* PIPE_USAGE_STAGING, and usage in general, should not be specified when
   * modifiers are used. */
   assert(count == 0);
   switch (pt->target) {
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      if (pt->last_level == 0 &&
      !util_format_is_depth_or_stencil(pt->format) &&
   pt->nr_samples <= 1)
         default:
                     if (pt->bind & PIPE_BIND_LINEAR)
            if (count > 0) {
      modifier = nvc0_miptree_select_best_modifier(pscreen, mt,
            if (modifier == DRM_FORMAT_MOD_INVALID) {
      FREE(mt);
               if (modifier == DRM_FORMAT_MOD_LINEAR) {
      pt->flags |= NOUVEAU_RESOURCE_FLAG_LINEAR;
      } else {
            } else {
                  if (!nvc0_miptree_init_ms_mode(mt)) {
      FREE(mt);
               if (unlikely(pt->flags & NVC0_RESOURCE_FLAG_VIDEO)) {
      assert(modifier == DRM_FORMAT_MOD_INVALID);
      } else
   if (likely(bo_config.nvc0.memtype)) {
         } else {
      /* When modifiers are supplied, usage is zero. TODO: detect the
   * modifiers+cursor case. */
   if (pt->usage & PIPE_BIND_CURSOR)
         else if ((pt->usage & PIPE_BIND_SCANOUT) || count > 0)
         else
         if (!nv50_miptree_init_layout_linear(mt, pitch_align)) {
      FREE(mt);
         }
            if (!bo_config.nvc0.memtype && (pt->usage == PIPE_USAGE_STAGING || pt->bind & PIPE_BIND_SHARED))
         else
                     if (mt->base.base.bind & (PIPE_BIND_CURSOR | PIPE_BIND_DISPLAY_TARGET))
            ret = nouveau_bo_new(dev, bo_flags, 4096, mt->total_size, &bo_config,
         if (ret) {
      FREE(mt);
      }
            NOUVEAU_DRV_STAT(nouveau_screen(pscreen), tex_obj_current_count, 1);
   NOUVEAU_DRV_STAT(nouveau_screen(pscreen), tex_obj_current_bytes,
               }
      /* Offset of zslice @z from start of level @l. */
   inline unsigned
   nvc0_mt_zslice_offset(const struct nv50_miptree *mt, unsigned l, unsigned z)
   {
               unsigned tds = NVC0_TILE_SHIFT_Z(mt->level[l].tile_mode);
            unsigned nby = util_format_get_nblocksy(pt->format,
            /* to next 2D tile slice within a 3D tile */
            /* to slice in the next (in z direction) 3D tile */
               }
      /* Surface functions.
   */
      struct pipe_surface *
   nvc0_miptree_surface_new(struct pipe_context *pipe,
               {
      struct nv50_surface *ns = nv50_surface_from_miptree(nv50_miptree(pt), templ);
   if (!ns)
         ns->base.context = pipe;
      }
