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
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_resource.h"
   #include "nvc0/gm107_texture.xml.h"
   #include "nvc0/nvc0_compute.xml.h"
   #include "nv50/g80_texture.xml.h"
   #include "nv50/g80_defs.xml.h"
      #include "util/format/u_format.h"
      #define NVE4_TIC_ENTRY_INVALID 0x000fffff
   #define NVE4_TSC_ENTRY_INVALID 0xfff00000
      static inline uint32_t
   nv50_tic_swizzle(const struct nvc0_format *fmt, unsigned swz, bool tex_int)
   {
      switch (swz) {
   case PIPE_SWIZZLE_X  : return fmt->tic.src_x;
   case PIPE_SWIZZLE_Y: return fmt->tic.src_y;
   case PIPE_SWIZZLE_Z : return fmt->tic.src_z;
   case PIPE_SWIZZLE_W: return fmt->tic.src_w;
   case PIPE_SWIZZLE_1:
         case PIPE_SWIZZLE_0:
   default:
            }
      struct pipe_sampler_view *
   nvc0_create_sampler_view(struct pipe_context *pipe,
               {
               if (templ->target == PIPE_TEXTURE_RECT || templ->target == PIPE_BUFFER)
               }
      static struct pipe_sampler_view *
   gm107_create_texture_view(struct pipe_context *pipe,
                     {
      const struct util_format_description *desc;
   const struct nvc0_format *fmt;
   uint64_t address;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t width, height;
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt;
            view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
                  view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
            view->id = -1;
                              desc = util_format_description(view->pipe.format);
            fmt = &nvc0_format_table[view->pipe.format];
   swz[0] = nv50_tic_swizzle(fmt, view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(fmt, view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(fmt, view->pipe.swizzle_b, tex_int);
            tic[0]  = fmt->tic.format << GM107_TIC2_0_COMPONENTS_SIZES__SHIFT;
   tic[0] |= fmt->tic.type_r << GM107_TIC2_0_R_DATA_TYPE__SHIFT;
   tic[0] |= fmt->tic.type_g << GM107_TIC2_0_G_DATA_TYPE__SHIFT;
   tic[0] |= fmt->tic.type_b << GM107_TIC2_0_B_DATA_TYPE__SHIFT;
   tic[0] |= fmt->tic.type_a << GM107_TIC2_0_A_DATA_TYPE__SHIFT;
   tic[0] |= swz[0] << GM107_TIC2_0_X_SOURCE__SHIFT;
   tic[0] |= swz[1] << GM107_TIC2_0_Y_SOURCE__SHIFT;
   tic[0] |= swz[2] << GM107_TIC2_0_Z_SOURCE__SHIFT;
                     tic[3]  = GM107_TIC2_3_LOD_ANISO_QUALITY_2;
   tic[4]  = GM107_TIC2_4_SECTOR_PROMOTION_PROMOTE_TO_2_V;
            if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
            if (!(flags & NV50_TEXVIEW_SCALED_COORDS))
         else
            /* check for linear storage type */
   if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (texture->target == PIPE_BUFFER) {
      assert(!(tic[5] & GM107_TIC2_5_NORMALIZED_COORDS));
   width = view->pipe.u.buf.size / (desc->block.bits / 8) - 1;
   address +=
         tic[2]  = GM107_TIC2_2_HEADER_VERSION_ONE_D_BUFFER;
   tic[3] |= width >> 16;
   tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_ONE_D_BUFFER;
      } else {
      assert(!(mt->level[0].pitch & 0x1f));
   /* must be 2D texture without mip maps */
   tic[2]  = GM107_TIC2_2_HEADER_VERSION_PITCH;
   tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_TWO_D_NO_MIPMAP;
   tic[3] |= mt->level[0].pitch >> 5;
   tic[4] |= mt->base.base.width0 - 1;
   tic[5] |= 0 << GM107_TIC2_5_DEPTH_MINUS_ONE__SHIFT;
      }
   tic[1]  = address;
   tic[2] |= address >> 32;
   tic[6]  = 0;
   tic[7]  = 0;
               tic[2]  = GM107_TIC2_2_HEADER_VERSION_BLOCKLINEAR;
   tic[3] |=
      ((mt->level[0].tile_mode & 0x0f0) >> 4 << 3) |
                  if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
   address += view->pipe.u.tex.first_layer * mt->layer_stride;
      }
   tic[1]  = address;
            switch (templ->target) {
   case PIPE_TEXTURE_1D:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_ONE_D;
      case PIPE_TEXTURE_2D:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_TWO_D;
      case PIPE_TEXTURE_RECT:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_TWO_D;
      case PIPE_TEXTURE_3D:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_THREE_D;
      case PIPE_TEXTURE_CUBE:
      depth /= 6;
   tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_CUBEMAP;
      case PIPE_TEXTURE_1D_ARRAY:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_ONE_D_ARRAY;
      case PIPE_TEXTURE_2D_ARRAY:
      tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_TWO_D_ARRAY;
      case PIPE_TEXTURE_CUBE_ARRAY:
      depth /= 6;
   tic[4] |= GM107_TIC2_4_TEXTURE_TYPE_CUBE_ARRAY;
      default:
                  tic[3] |= (flags & NV50_TEXVIEW_FILTER_MSAA8) ?
            GM107_TIC2_3_USE_HEADER_OPT_CONTROL :
         if (flags & (NV50_TEXVIEW_ACCESS_RESOLVE | NV50_TEXVIEW_IMAGE_GM107)) {
      width = mt->base.base.width0 << mt->ms_x;
      } else {
      width = mt->base.base.width0;
                        tic[5] |= (height - 1) & 0xffff;
   tic[5] |= (depth - 1) << GM107_TIC2_5_DEPTH_MINUS_ONE__SHIFT;
            /* sampling points: (?) */
   if ((flags & NV50_TEXVIEW_ACCESS_RESOLVE) && mt->ms_x > 1) {
      tic[6]  = GM107_TIC2_6_ANISO_FINE_SPREAD_MODIFIER_CONST_TWO;
      } else {
      tic[6]  = GM107_TIC2_6_ANISO_FINE_SPREAD_FUNC_TWO;
               tic[7]  = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;
               }
      struct pipe_sampler_view *
   gm107_create_texture_view_from_image(struct pipe_context *pipe,
         {
      struct nv04_resource *res = nv04_resource(view->resource);
   struct pipe_sampler_view templ = {};
   enum pipe_texture_target target;
            if (!res)
                  if (target == PIPE_TEXTURE_CUBE || target == PIPE_TEXTURE_CUBE_ARRAY)
            templ.target = target;
   templ.format = view->format;
   templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
            if (target == PIPE_BUFFER) {
      templ.u.buf.offset = view->u.buf.offset;
      } else {
      templ.u.tex.first_layer = view->u.tex.first_layer;
   templ.u.tex.last_layer = view->u.tex.last_layer;
                           }
      static struct pipe_sampler_view *
   gf100_create_texture_view(struct pipe_context *pipe,
                     {
      const struct util_format_description *desc;
   const struct nvc0_format *fmt;
   uint64_t address;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t width, height;
   uint32_t depth;
   uint32_t tex_fmt;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt;
            view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
                  view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
            view->id = -1;
                                                tex_int = util_format_is_pure_integer(view->pipe.format);
            swz[0] = nv50_tic_swizzle(fmt, view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(fmt, view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(fmt, view->pipe.swizzle_b, tex_int);
   swz[3] = nv50_tic_swizzle(fmt, view->pipe.swizzle_a, tex_int);
   tic[0] = (tex_fmt << G80_TIC_0_COMPONENTS_SIZES__SHIFT) |
            (fmt->tic.type_r << G80_TIC_0_R_DATA_TYPE__SHIFT) |
   (fmt->tic.type_g << G80_TIC_0_G_DATA_TYPE__SHIFT) |
   (fmt->tic.type_b << G80_TIC_0_B_DATA_TYPE__SHIFT) |
   (fmt->tic.type_a << G80_TIC_0_A_DATA_TYPE__SHIFT) |
   (swz[0] << G80_TIC_0_X_SOURCE__SHIFT) |
   (swz[1] << G80_TIC_0_Y_SOURCE__SHIFT) |
   (swz[2] << G80_TIC_0_Z_SOURCE__SHIFT) |
                           if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
            if (!(flags & NV50_TEXVIEW_SCALED_COORDS))
            /* check for linear storage type */
   if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (texture->target == PIPE_BUFFER) {
      assert(!(tic[2] & G80_TIC_2_NORMALIZED_COORDS));
   address +=
         tic[2] |= G80_TIC_2_LAYOUT_PITCH | G80_TIC_2_TEXTURE_TYPE_ONE_D_BUFFER;
   tic[3] = 0;
   tic[4] = /* width */
            } else {
      /* must be 2D texture without mip maps */
   tic[2] |= G80_TIC_2_LAYOUT_PITCH | G80_TIC_2_TEXTURE_TYPE_TWO_D_NO_MIPMAP;
   tic[3] = mt->level[0].pitch;
   tic[4] = mt->base.base.width0;
      }
   tic[6] =
   tic[7] = 0;
   tic[1] = address;
   tic[2] |= address >> 32;
               tic[2] |=
      ((mt->level[0].tile_mode & 0x0f0) << (22 - 4)) |
                  if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
   address += view->pipe.u.tex.first_layer * mt->layer_stride;
      }
   tic[1] = address;
            switch (templ->target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_ONE_D;
      case PIPE_TEXTURE_2D:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_TWO_D;
      case PIPE_TEXTURE_RECT:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_TWO_D;
      case PIPE_TEXTURE_3D:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_THREE_D;
      case PIPE_TEXTURE_CUBE:
      depth /= 6;
   tic[2] |= G80_TIC_2_TEXTURE_TYPE_CUBEMAP;
      case PIPE_TEXTURE_1D_ARRAY:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_ONE_D_ARRAY;
      case PIPE_TEXTURE_2D_ARRAY:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_TWO_D_ARRAY;
      case PIPE_TEXTURE_CUBE_ARRAY:
      depth /= 6;
   tic[2] |= G80_TIC_2_TEXTURE_TYPE_CUBE_ARRAY;
      default:
                           if (flags & NV50_TEXVIEW_ACCESS_RESOLVE) {
      width = mt->base.base.width0 << mt->ms_x;
      } else {
      width = mt->base.base.width0;
                        tic[5] = height & 0xffff;
   tic[5] |= depth << 16;
            /* sampling points: (?) */
   if (flags & NV50_TEXVIEW_ACCESS_RESOLVE)
         else
            tic[7] = (view->pipe.u.tex.last_level << 4) | view->pipe.u.tex.first_level;
               }
      struct pipe_sampler_view *
   nvc0_create_texture_view(struct pipe_context *pipe,
                     {
      if (nvc0_context(pipe)->screen->tic.maxwell)
            }
      bool
   nvc0_update_tic(struct nvc0_context *nvc0, struct nv50_tic_entry *tic,
         {
      uint64_t address = res->address;
   if (res->base.target != PIPE_BUFFER)
         address += tic->pipe.u.buf.offset;
   if (tic->tic[1] == (uint32_t)address &&
      (tic->tic[2] & 0xff) == address >> 32)
         tic->tic[1] = address;
   tic->tic[2] &= 0xffffff00;
            if (tic->id >= 0) {
      nvc0->base.push_data(&nvc0->base, nvc0->screen->txc, tic->id * 32,
                              }
      bool
   nvc0_validate_tic(struct nvc0_context *nvc0, int s)
   {
      uint32_t commands[32];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   unsigned n = 0;
            for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
   struct nv04_resource *res;
            if (!tic) {
      if (dirty)
            }
   res = nv04_resource(tic->pipe.texture);
            if (tic->id < 0) {
               nvc0->base.push_data(&nvc0->base, nvc0->screen->txc, tic->id * 32,
                  } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      if (unlikely(s == 5))
         else
         PUSH_DATA (push, (tic->id << 4) | 1);
      }
            res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            if (!dirty)
                  if (unlikely(s == 5))
         else
      }
   for (; i < nvc0->state.num_textures[s]; ++i)
                     if (n) {
      if (unlikely(s == 5))
         else
            }
               }
      static bool
   nve4_validate_tic(struct nvc0_context *nvc0, unsigned s)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
            for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
   struct nv04_resource *res;
            if (!tic) {
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
      }
   res = nv04_resource(tic->pipe.texture);
            if (tic->id < 0) {
               nvc0->base.push_data(&nvc0->base, nvc0->screen->txc, tic->id * 32,
                  } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
      }
            res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            nvc0->tex_handles[s][i] &= ~NVE4_TIC_ENTRY_INVALID;
   nvc0->tex_handles[s][i] |= tic->id;
   if (dirty)
      }
   for (; i < nvc0->state.num_textures[s]; ++i) {
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
                           }
      void nvc0_validate_textures(struct nvc0_context *nvc0)
   {
      bool need_flush = false;
            for (i = 0; i < 5; i++) {
      if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS)
         else
               if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TIC_FLUSH), 1);
               /* Invalidate all CP textures because they are aliased. */
   for (int i = 0; i < nvc0->num_textures[5]; i++)
         nvc0->textures_dirty[5] = ~0;
      }
      bool
   nvc0_validate_tsc(struct nvc0_context *nvc0, int s)
   {
      uint32_t commands[16];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned i;
   unsigned n = 0;
            for (i = 0; i < nvc0->num_samplers[s]; ++i) {
               if (!(nvc0->samplers_dirty[s] & (1 << i)))
         if (!tsc) {
      commands[n++] = (i << 4) | 0;
      }
   nvc0->seamless_cube_map = tsc->seamless_cube_map;
   if (tsc->id < 0) {
               nvc0_m2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                  }
               }
   for (; i < nvc0->state.num_samplers[s]; ++i)
                     // TXF, in unlinked tsc mode, will always use sampler 0. So we have to
   // ensure that it remains bound. Its contents don't matter, all samplers we
   // ever create have the SRGB_CONVERSION bit set, so as long as the first
   // entry is initialized, we're good to go. This is the only bit that has
   // any effect on what TXF does.
   if ((nvc0->samplers_dirty[s] & 1) && !nvc0->samplers[s][0]) {
      if (n == 0)
         // We're guaranteed that the first command refers to the first slot, so
   // we're not overwriting a valid entry.
               if (n) {
      if (unlikely(s == 5))
         else
            }
               }
      bool
   nve4_validate_tsc(struct nvc0_context *nvc0, int s)
   {
      unsigned i;
            for (i = 0; i < nvc0->num_samplers[s]; ++i) {
               if (!tsc) {
      nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;
      }
   if (tsc->id < 0) {
               nve4_p2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                        }
            nvc0->tex_handles[s][i] &= ~NVE4_TSC_ENTRY_INVALID;
      }
   for (; i < nvc0->state.num_samplers[s]; ++i) {
      nvc0->tex_handles[s][i] |= NVE4_TSC_ENTRY_INVALID;
                           }
      void nvc0_validate_samplers(struct nvc0_context *nvc0)
   {
      bool need_flush = false;
            for (i = 0; i < 5; i++) {
      if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS)
         else
               if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_3D(TSC_FLUSH), 1);
               /* Invalidate all CP samplers because they are aliased. */
   nvc0->samplers_dirty[5] = ~0;
      }
      void
   nvc0_upload_tsc0(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   u32 data[8] = { G80_TSC_0_SRGB_CONVERSION };
   nvc0->base.push_data(&nvc0->base, nvc0->screen->txc,
               BEGIN_NVC0(push, NVC0_3D(TSC_FLUSH), 1);
      }
      /* Upload the "diagonal" entries for the possible texture sources ($t == $s).
   * At some point we might want to get a list of the combinations used by a
   * shader and fill in those entries instead of having it extract the handles.
   */
   void
   nve4_set_tex_handles(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            if (nvc0->screen->base.class_3d < NVE4_3D_CLASS)
            for (s = 0; s < 5; ++s) {
      uint32_t dirty = nvc0->textures_dirty[s] | nvc0->samplers_dirty[s];
   if (!dirty)
         BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   do {
                     BEGIN_NVC0(push, NVC0_3D(CB_POS), 2);
   PUSH_DATA (push, NVC0_CB_AUX_TEX_INFO(i));
               nvc0->textures_dirty[s] = 0;
         }
      static uint64_t
   nve4_create_texture_handle(struct pipe_context *pipe,
               {
      /* We have to create persistent handles that won't change for these objects
   * That means that we have to upload them into place and lock them so that
   * they can't be kicked out later.
   */
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv50_tic_entry *tic = nv50_tic_entry(view);
   struct nv50_tsc_entry *tsc = pipe->create_sampler_state(pipe, sampler);
            tsc->id = nvc0_screen_tsc_alloc(nvc0->screen, tsc);
   if (tsc->id < 0)
            if (tic->id < 0) {
      tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);
   if (tic->id < 0)
            nve4_p2mf_push_linear(&nvc0->base, nvc0->screen->txc, tic->id * 32,
                              nve4_p2mf_push_linear(&nvc0->base, nvc0->screen->txc,
                                 // Add an extra reference to this sampler view effectively held by this
   // texture handle. This is to deal with the sampler view being dereferenced
   // before the handle is. However we need the view to still be live until the
   // handle to it is deleted.
   pipe_sampler_view_reference(&v, view);
            nvc0->screen->tic.lock[tic->id / 32] |= 1 << (tic->id % 32);
                  fail:
      pipe->delete_sampler_state(pipe, tsc);
      }
      static bool
   view_bound(struct nvc0_context *nvc0, struct pipe_sampler_view *view) {
      for (int s = 0; s < 6; s++) {
      for (int i = 0; i < nvc0->num_textures[s]; i++)
      if (nvc0->textures[s][i] == view)
   }
      }
      static void
   nve4_delete_texture_handle(struct pipe_context *pipe, uint64_t handle)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   uint32_t tic = handle & NVE4_TIC_ENTRY_INVALID;
   uint32_t tsc = (handle & NVE4_TSC_ENTRY_INVALID) >> 20;
            if (entry) {
      struct pipe_sampler_view *view = &entry->pipe;
   assert(entry->bindless);
   p_atomic_dec(&entry->bindless);
   if (!view_bound(nvc0, view))
                        }
      static void
   nve4_make_texture_handle_resident(struct pipe_context *pipe,
         {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   if (resident) {
      struct nvc0_resident *res = calloc(1, sizeof(struct nvc0_resident));
   struct nv50_tic_entry *tic =
         assert(tic);
            res->handle = handle;
   res->buf = nv04_resource(tic->pipe.texture);
   res->flags = NOUVEAU_BO_RD;
      } else {
      list_for_each_entry_safe(struct nvc0_resident, pos, &nvc0->tex_head, list) {
      if (pos->handle == handle) {
      list_del(&pos->list);
   free(pos);
               }
      static const uint8_t nve4_su_format_map[PIPE_FORMAT_COUNT];
   static const uint16_t nve4_su_format_aux_map[PIPE_FORMAT_COUNT];
   static const uint16_t nve4_suldp_lib_offset[PIPE_FORMAT_COUNT];
      static void
   nvc0_get_surface_dims(const struct pipe_image_view *view,
         {
      struct nv04_resource *res = nv04_resource(view->resource);
            *width = *height = *depth = 1;
   if (res->base.target == PIPE_BUFFER) {
      *width = view->u.buf.size / util_format_get_blocksize(view->format);
               level = view->u.tex.level;
   *width = u_minify(view->resource->width0, level);
   *height = u_minify(view->resource->height0, level);
            switch (res->base.target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      *depth = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_3D:
         default:
      assert(!"unexpected texture target");
         }
      void
   nvc0_mark_image_range_valid(const struct pipe_image_view *view)
   {
                        util_range_add(&res->base, &res->valid_buffer_range,
            }
      void
   nve4_set_surface_info(struct nouveau_pushbuf *push,
               {
      struct nvc0_screen *screen = nvc0->screen;
   struct nv04_resource *res;
   uint64_t address;
   uint32_t *const info = push->cur;
   int width, height, depth;
            if (view && !nve4_su_format_map[view->format])
                     if (!view || !nve4_su_format_map[view->format]) {
               info[0] = 0xbadf0000;
   info[1] = 0x80004000;
   info[12] = nve4_suldp_lib_offset[PIPE_FORMAT_R32G32B32A32_UINT] +
            }
                     /* get surface dimensions based on the target. */
            info[8] = width;
   info[9] = height;
   info[10] = depth;
   switch (res->base.target) {
   case PIPE_TEXTURE_1D_ARRAY:
      info[11] = 1;
      case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
      info[11] = 2;
      case PIPE_TEXTURE_3D:
      info[11] = 3;
      case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      info[11] = 4;
      default:
      info[11] = 0;
      }
            /* Stick the blockwidth (ie. number of bytes per pixel) to check if the
   * format doesn't mismatch. */
            /* limit in bytes for raw access */
                  #if 0
      switch (util_format_get_blocksizebits(view->format)) {
   case  16: info[1] |= 1 << 16; break;
   case  32: info[1] |= 2 << 16; break;
   case  64: info[1] |= 3 << 16; break;
   case 128: info[1] |= 4 << 16; break;
   default:
            #else
      info[1] |= log2cpp << 16;
   info[1] |=  0x4000;
      #endif
         if (res->base.target == PIPE_BUFFER) {
               info[0]  = address >> 8;
   info[2]  = width - 1;
   info[2] |= (0xff & nve4_su_format_aux_map[view->format]) << 22;
   info[3]  = 0;
   info[4]  = 0;
   info[5]  = 0;
   info[6]  = 0;
   info[7]  = 0;
   info[14] = 0;
      } else {
      struct nv50_miptree *mt = nv50_miptree(&res->base);
   struct nv50_miptree_level *lvl = &mt->level[view->u.tex.level];
            if (!mt->layout_3d) {
      address += mt->layer_stride * z;
                        info[0]  = address >> 8;
   info[2]  = (width << mt->ms_x) - 1;
   /* NOTE: this is really important: */
   info[2] |= (0xff & nve4_su_format_aux_map[view->format]) << 22;
   info[3]  = (0x88 << 24) | (lvl->pitch / 64);
   info[4]  = (height << mt->ms_y) - 1;
   info[4] |= (lvl->tile_mode & 0x0f0) << 25;
   info[4] |= NVC0_TILE_SHIFT_Y(lvl->tile_mode) << 22;
   info[5]  = mt->layer_stride >> 8;
   info[6]  = depth - 1;
   info[6] |= (lvl->tile_mode & 0xf00) << 21;
   info[6] |= NVC0_TILE_SHIFT_Z(lvl->tile_mode) << 22;
   info[7]  = mt->layout_3d ? 1 : 0;
   info[7] |= z << 16;
   info[14] = mt->ms_x;
         }
      static inline void
   nvc0_set_surface_info(struct nouveau_pushbuf *push,
               {
      struct nv04_resource *res;
                     /* Make sure to always initialize the surface information area because it's
   * used to check if the given image is bound or not. */
            if (!view || !view->resource)
                  /* Stick the image dimensions for the imageSize() builtin. */
   info[8] = width;
   info[9] = height;
            /* Stick the blockwidth (ie. number of bytes per pixel) to calculate pixel
   * offset and to check if the format doesn't mismatch. */
            if (res->base.target == PIPE_BUFFER) {
      info[0]  = address >> 8;
      } else {
      struct nv50_miptree *mt = nv50_miptree(&res->base);
   struct nv50_miptree_level *lvl = &mt->level[view->u.tex.level];
   unsigned z = mt->layout_3d ? view->u.tex.first_layer : 0;
   unsigned nby = align(util_format_get_nblocksy(view->format, height),
            /* NOTE: this does not precisely match nve4; the values are made to be
   * easier for the shader to consume.
   */
   info[0]  = address >> 8;
   info[2]  = (NVC0_TILE_SHIFT_X(lvl->tile_mode) - info[12]) << 24;
   info[4]  = NVC0_TILE_SHIFT_Y(lvl->tile_mode) << 24 | nby;
   info[5]  = mt->layer_stride >> 8;
   info[6]  = NVC0_TILE_SHIFT_Z(lvl->tile_mode) << 24;
   info[7]  = z;
   info[14] = mt->ms_x;
         }
      void
   nvc0_validate_suf(struct nvc0_context *nvc0, int s)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            for (int i = 0; i < NVC0_MAX_IMAGES; ++i) {
      struct pipe_image_view *view = &nvc0->images[s][i];
   int width = 0, height = 0, depth = 0;
            if (s == 5)
         else
            if (view->resource) {
                     if (util_format_is_depth_or_stencil(view->format))
                                       address = res->address;
                                                   PUSH_DATAh(push, address);
   PUSH_DATA (push, address);
   PUSH_DATA (push, align(width * blocksize, 0x100));
   PUSH_DATA (push, NVC0_3D_IMAGE_HEIGHT_LINEAR | 1);
   PUSH_DATA (push, rt);
      } else {
      struct nv50_miptree *mt = nv50_miptree(view->resource);
                  if (mt->layout_3d) {
      // We have to adjust the size of the 3d surface to be
   // accessible within 2d limits. The size of each z tile goes
   // into the x direction, while the number of z tiles goes into
   // the y direction.
   const unsigned nbx = util_format_get_nblocksx(view->format, width);
   const unsigned nby = util_format_get_nblocksy(view->format, height);
                        adjusted_width = align(nbx, tsx / util_format_get_blocksize(view->format)) * tsz;
      } else {
      const unsigned z = view->u.tex.first_layer;
                     PUSH_DATAh(push, address);
   PUSH_DATA (push, address);
   PUSH_DATA (push, adjusted_width << mt->ms_x);
   PUSH_DATA (push, adjusted_height << mt->ms_y);
   PUSH_DATA (push, rt);
               if (s == 5)
         else
      } else {
      PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0x14000);
               /* stick surface information into the driver constant buffer */
   if (s == 5)
         else
         PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   if (s == 5)
         else
                        }
      static inline void
   nvc0_update_surface_bindings(struct nvc0_context *nvc0)
   {
               /* Invalidate all COMPUTE images because they are aliased with FRAGMENT. */
   nouveau_bufctx_reset(nvc0->bufctx_cp, NVC0_BIND_CP_SUF);
   nvc0->dirty_cp |= NVC0_NEW_CP_SURFACES;
      }
      static void
   gm107_validate_surfaces(struct nvc0_context *nvc0,
         {
      struct nv04_resource *res = nv04_resource(view->resource);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
                     res = nv04_resource(tic->pipe.texture);
            if (tic->id < 0) {
               /* upload the texture view */
   nve4_p2mf_push_linear(&nvc0->base, nvc0->screen->txc, tic->id * 32,
            BEGIN_NVC0(push, NVC0_3D(TIC_FLUSH), 1);
      } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
      }
            res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
                     /* upload the texture handle */
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(stage));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(stage));
   BEGIN_NVC0(push, NVC0_3D(CB_POS), 2);
   PUSH_DATA (push, NVC0_CB_AUX_TEX_INFO(slot + 32));
      }
      static inline void
   nve4_update_surface_bindings(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            for (s = 0; s < 5; s++) {
      if (!nvc0->images_dirty[s])
            for (i = 0; i < NVC0_MAX_IMAGES; ++i) {
               BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
                                    if (res->base.target == PIPE_BUFFER) {
                                       if (nvc0->screen->base.class_3d >= GM107_3D_CLASS)
      } else {
      for (j = 0; j < 16; j++)
               }
      void
   nvc0_validate_surfaces(struct nvc0_context *nvc0)
   {
      if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
         } else {
            }
      static uint64_t
   nve4_create_image_handle(struct pipe_context *pipe,
         {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            while (screen->img.entries[i]) {
      i = (i + 1) & (NVE4_IMG_MAX_HANDLES - 1);
   if (i == screen->img.next)
               screen->img.next = (i + 1) & (NVE4_IMG_MAX_HANDLES - 1);
   screen->img.entries[i] = calloc(1, sizeof(struct pipe_image_view));
            for (s = 0; s < 6; s++) {
      BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 16);
   PUSH_DATA (push, NVC0_CB_AUX_BINDLESS_INFO(i));
                  }
      static void
   nve4_delete_image_handle(struct pipe_context *pipe, uint64_t handle)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_screen *screen = nvc0->screen;
            free(screen->img.entries[i]);
      }
      static void
   nve4_make_image_handle_resident(struct pipe_context *pipe, uint64_t handle,
         {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            if (resident) {
      struct nvc0_resident *res = calloc(1, sizeof(struct nvc0_resident));
   struct pipe_image_view *view =
                  if (view->resource->target == PIPE_BUFFER &&
      access & PIPE_IMAGE_ACCESS_WRITE)
      res->handle = handle;
   res->buf = nv04_resource(view->resource);
   res->flags = (access & 3) << 8;
      } else {
      list_for_each_entry_safe(struct nvc0_resident, pos, &nvc0->img_head, list) {
      if (pos->handle == handle) {
      list_del(&pos->list);
   free(pos);
               }
      static uint64_t
   gm107_create_image_handle(struct pipe_context *pipe,
         {
      /* GM107+ use TIC handles to reference images. As such, image handles are
   * just the TIC id.
   */
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_sampler_view *sview =
                  if (tic == NULL)
            tic->bindless = 1;
   tic->id = nvc0_screen_tic_alloc(nvc0->screen, tic);
   if (tic->id < 0)
            nve4_p2mf_push_linear(&nvc0->base, nvc0->screen->txc, tic->id * 32,
                                    // Compute handle. This will include the TIC as well as some additional
   // info regarding the bound 3d surface layer, if applicable.
   uint64_t handle = 0x100000000ULL | tic->id;
   struct nv04_resource *res = nv04_resource(view->resource);
   if (res->base.target == PIPE_TEXTURE_3D) {
      handle |= 1 << 11;
      }
         fail:
      FREE(tic);
      }
      static void
   gm107_delete_image_handle(struct pipe_context *pipe, uint64_t handle)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   int tic = handle & NVE4_TIC_ENTRY_INVALID;
   struct nv50_tic_entry *entry = nvc0->screen->tic.entries[tic];
   struct pipe_sampler_view *view = &entry->pipe;
   assert(entry->bindless == 1);
   assert(!view_bound(nvc0, view));
   entry->bindless = 0;
   nvc0_screen_tic_unlock(nvc0->screen, entry);
      }
      static void
   gm107_make_image_handle_resident(struct pipe_context *pipe, uint64_t handle,
         {
               if (resident) {
      struct nvc0_resident *res = calloc(1, sizeof(struct nvc0_resident));
   struct nv50_tic_entry *tic =
         assert(tic);
            res->handle = handle;
   res->buf = nv04_resource(tic->pipe.texture);
   res->flags = (access & 3) << 8;
   if (res->buf->base.target == PIPE_BUFFER &&
      access & PIPE_IMAGE_ACCESS_WRITE)
   util_range_add(&res->buf->base, &res->buf->valid_buffer_range,
               } else {
      list_for_each_entry_safe(struct nvc0_resident, pos, &nvc0->img_head, list) {
      if (pos->handle == handle) {
      list_del(&pos->list);
   free(pos);
               }
      void
   nvc0_init_bindless_functions(struct pipe_context *pipe) {
      pipe->create_texture_handle = nve4_create_texture_handle;
   pipe->delete_texture_handle = nve4_delete_texture_handle;
            if (nvc0_context(pipe)->screen->base.class_3d < GM107_3D_CLASS) {
      pipe->create_image_handle = nve4_create_image_handle;
   pipe->delete_image_handle = nve4_delete_image_handle;
      } else {
      pipe->create_image_handle = gm107_create_image_handle;
   pipe->delete_image_handle = gm107_delete_image_handle;
         }
         static const uint8_t nve4_su_format_map[PIPE_FORMAT_COUNT] =
   {
      [PIPE_FORMAT_R32G32B32A32_FLOAT] = GK104_IMAGE_FORMAT_RGBA32_FLOAT,
   [PIPE_FORMAT_R32G32B32A32_SINT] = GK104_IMAGE_FORMAT_RGBA32_SINT,
   [PIPE_FORMAT_R32G32B32A32_UINT] = GK104_IMAGE_FORMAT_RGBA32_UINT,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = GK104_IMAGE_FORMAT_RGBA16_FLOAT,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = GK104_IMAGE_FORMAT_RGBA16_UNORM,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = GK104_IMAGE_FORMAT_RGBA16_SNORM,
   [PIPE_FORMAT_R16G16B16A16_SINT] = GK104_IMAGE_FORMAT_RGBA16_SINT,
   [PIPE_FORMAT_R16G16B16A16_UINT] = GK104_IMAGE_FORMAT_RGBA16_UINT,
   [PIPE_FORMAT_B8G8R8A8_UNORM] = GK104_IMAGE_FORMAT_BGRA8_UNORM,
   [PIPE_FORMAT_R8G8B8A8_UNORM] = GK104_IMAGE_FORMAT_RGBA8_UNORM,
   [PIPE_FORMAT_R8G8B8A8_SNORM] = GK104_IMAGE_FORMAT_RGBA8_SNORM,
   [PIPE_FORMAT_R8G8B8A8_SINT] = GK104_IMAGE_FORMAT_RGBA8_SINT,
   [PIPE_FORMAT_R8G8B8A8_UINT] = GK104_IMAGE_FORMAT_RGBA8_UINT,
   [PIPE_FORMAT_R11G11B10_FLOAT] = GK104_IMAGE_FORMAT_R11G11B10_FLOAT,
   [PIPE_FORMAT_R10G10B10A2_UNORM] = GK104_IMAGE_FORMAT_RGB10_A2_UNORM,
   [PIPE_FORMAT_R10G10B10A2_UINT] = GK104_IMAGE_FORMAT_RGB10_A2_UINT,
   [PIPE_FORMAT_R32G32_FLOAT] = GK104_IMAGE_FORMAT_RG32_FLOAT,
   [PIPE_FORMAT_R32G32_SINT] = GK104_IMAGE_FORMAT_RG32_SINT,
   [PIPE_FORMAT_R32G32_UINT] = GK104_IMAGE_FORMAT_RG32_UINT,
   [PIPE_FORMAT_R16G16_FLOAT] = GK104_IMAGE_FORMAT_RG16_FLOAT,
   [PIPE_FORMAT_R16G16_UNORM] = GK104_IMAGE_FORMAT_RG16_UNORM,
   [PIPE_FORMAT_R16G16_SNORM] = GK104_IMAGE_FORMAT_RG16_SNORM,
   [PIPE_FORMAT_R16G16_SINT] = GK104_IMAGE_FORMAT_RG16_SINT,
   [PIPE_FORMAT_R16G16_UINT] = GK104_IMAGE_FORMAT_RG16_UINT,
   [PIPE_FORMAT_R8G8_UNORM] = GK104_IMAGE_FORMAT_RG8_UNORM,
   [PIPE_FORMAT_R8G8_SNORM] = GK104_IMAGE_FORMAT_RG8_SNORM,
   [PIPE_FORMAT_R8G8_SINT] = GK104_IMAGE_FORMAT_RG8_SINT,
   [PIPE_FORMAT_R8G8_UINT] = GK104_IMAGE_FORMAT_RG8_UINT,
   [PIPE_FORMAT_R32_FLOAT] = GK104_IMAGE_FORMAT_R32_FLOAT,
   [PIPE_FORMAT_R32_SINT] = GK104_IMAGE_FORMAT_R32_SINT,
   [PIPE_FORMAT_R32_UINT] = GK104_IMAGE_FORMAT_R32_UINT,
   [PIPE_FORMAT_R16_FLOAT] = GK104_IMAGE_FORMAT_R16_FLOAT,
   [PIPE_FORMAT_R16_UNORM] = GK104_IMAGE_FORMAT_R16_UNORM,
   [PIPE_FORMAT_R16_SNORM] = GK104_IMAGE_FORMAT_R16_SNORM,
   [PIPE_FORMAT_R16_SINT] = GK104_IMAGE_FORMAT_R16_SINT,
   [PIPE_FORMAT_R16_UINT] = GK104_IMAGE_FORMAT_R16_UINT,
   [PIPE_FORMAT_R8_UNORM] = GK104_IMAGE_FORMAT_R8_UNORM,
   [PIPE_FORMAT_R8_SNORM] = GK104_IMAGE_FORMAT_R8_SNORM,
   [PIPE_FORMAT_R8_SINT] = GK104_IMAGE_FORMAT_R8_SINT,
      };
      /* Auxiliary format description values for surface instructions.
   * (log2(bytes per pixel) << 12) | (unk8 << 8) | unk22
   */
   static const uint16_t nve4_su_format_aux_map[PIPE_FORMAT_COUNT] =
   {
      [PIPE_FORMAT_R32G32B32A32_FLOAT] = 0x4842,
   [PIPE_FORMAT_R32G32B32A32_SINT] = 0x4842,
            [PIPE_FORMAT_R16G16B16A16_UNORM] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_SINT] = 0x3933,
   [PIPE_FORMAT_R16G16B16A16_UINT] = 0x3933,
            [PIPE_FORMAT_R32G32_FLOAT] = 0x3433,
   [PIPE_FORMAT_R32G32_SINT] = 0x3433,
            [PIPE_FORMAT_R10G10B10A2_UNORM] = 0x2a24,
   [PIPE_FORMAT_R10G10B10A2_UINT] = 0x2a24,
   [PIPE_FORMAT_B8G8R8A8_UNORM] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_UNORM] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_SNORM] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_SINT] = 0x2a24,
   [PIPE_FORMAT_R8G8B8A8_UINT] = 0x2a24,
            [PIPE_FORMAT_R16G16_UNORM] = 0x2524,
   [PIPE_FORMAT_R16G16_SNORM] = 0x2524,
   [PIPE_FORMAT_R16G16_SINT] = 0x2524,
   [PIPE_FORMAT_R16G16_UINT] = 0x2524,
            [PIPE_FORMAT_R32_SINT] = 0x2024,
   [PIPE_FORMAT_R32_UINT] = 0x2024,
            [PIPE_FORMAT_R8G8_UNORM] = 0x1615,
   [PIPE_FORMAT_R8G8_SNORM] = 0x1615,
   [PIPE_FORMAT_R8G8_SINT] = 0x1615,
            [PIPE_FORMAT_R16_UNORM] = 0x1115,
   [PIPE_FORMAT_R16_SNORM] = 0x1115,
   [PIPE_FORMAT_R16_SINT] = 0x1115,
   [PIPE_FORMAT_R16_UINT] = 0x1115,
            [PIPE_FORMAT_R8_UNORM] = 0x0206,
   [PIPE_FORMAT_R8_SNORM] = 0x0206,
   [PIPE_FORMAT_R8_SINT] = 0x0206,
      };
      /* NOTE: These are hardcoded offsets for the shader library.
   * TODO: Automate them.
   */
   static const uint16_t nve4_suldp_lib_offset[PIPE_FORMAT_COUNT] =
   {
      [PIPE_FORMAT_R32G32B32A32_FLOAT] = 0x218,
   [PIPE_FORMAT_R32G32B32A32_SINT]  = 0x218,
   [PIPE_FORMAT_R32G32B32A32_UINT]  = 0x218,
   [PIPE_FORMAT_R16G16B16A16_UNORM] = 0x248,
   [PIPE_FORMAT_R16G16B16A16_SNORM] = 0x2b8,
   [PIPE_FORMAT_R16G16B16A16_SINT]  = 0x330,
   [PIPE_FORMAT_R16G16B16A16_UINT]  = 0x388,
   [PIPE_FORMAT_R16G16B16A16_FLOAT] = 0x3d8,
   [PIPE_FORMAT_R32G32_FLOAT]       = 0x428,
   [PIPE_FORMAT_R32G32_SINT]        = 0x468,
   [PIPE_FORMAT_R32G32_UINT]        = 0x468,
   [PIPE_FORMAT_R10G10B10A2_UNORM]  = 0x4a8,
   [PIPE_FORMAT_R10G10B10A2_UINT]   = 0x530,
   [PIPE_FORMAT_R8G8B8A8_UNORM]     = 0x588,
   [PIPE_FORMAT_R8G8B8A8_SNORM]     = 0x5f8,
   [PIPE_FORMAT_R8G8B8A8_SINT]      = 0x670,
   [PIPE_FORMAT_R8G8B8A8_UINT]      = 0x6c8,
   [PIPE_FORMAT_B5G6R5_UNORM]       = 0x718,
   [PIPE_FORMAT_B5G5R5X1_UNORM]     = 0x7a0,
   [PIPE_FORMAT_R16G16_UNORM]       = 0x828,
   [PIPE_FORMAT_R16G16_SNORM]       = 0x890,
   [PIPE_FORMAT_R16G16_SINT]        = 0x8f0,
   [PIPE_FORMAT_R16G16_UINT]        = 0x948,
   [PIPE_FORMAT_R16G16_FLOAT]       = 0x998,
   [PIPE_FORMAT_R32_FLOAT]          = 0x9e8,
   [PIPE_FORMAT_R32_SINT]           = 0xa30,
   [PIPE_FORMAT_R32_UINT]           = 0xa30,
   [PIPE_FORMAT_R8G8_UNORM]         = 0xa78,
   [PIPE_FORMAT_R8G8_SNORM]         = 0xae0,
   [PIPE_FORMAT_R8G8_UINT]          = 0xb48,
   [PIPE_FORMAT_R8G8_SINT]          = 0xb98,
   [PIPE_FORMAT_R16_UNORM]          = 0xbe8,
   [PIPE_FORMAT_R16_SNORM]          = 0xc48,
   [PIPE_FORMAT_R16_SINT]           = 0xca0,
   [PIPE_FORMAT_R16_UINT]           = 0xce8,
   [PIPE_FORMAT_R16_FLOAT]          = 0xd30,
   [PIPE_FORMAT_R8_UNORM]           = 0xd88,
   [PIPE_FORMAT_R8_SNORM]           = 0xde0,
   [PIPE_FORMAT_R8_SINT]            = 0xe38,
   [PIPE_FORMAT_R8_UINT]            = 0xe88,
      };
