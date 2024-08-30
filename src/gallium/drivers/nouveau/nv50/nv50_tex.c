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
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_resource.h"
   #include "nv50/g80_texture.xml.h"
   #include "nv50/g80_defs.xml.h"
      #include "util/format/u_format.h"
      static inline uint32_t
   nv50_tic_swizzle(const struct nv50_format *fmt, unsigned swz, bool tex_int)
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
   nv50_create_sampler_view(struct pipe_context *pipe,
               {
               if (templ->target == PIPE_TEXTURE_RECT || templ->target == PIPE_BUFFER)
               }
      struct pipe_sampler_view *
   nv50_create_texture_view(struct pipe_context *pipe,
                     {
      const uint32_t class_3d = nouveau_context(pipe)->screen->class_3d;
   const struct util_format_description *desc;
   const struct nv50_format *fmt;
   uint64_t addr;
   uint32_t *tic;
   uint32_t swz[4];
   uint32_t depth;
   struct nv50_tic_entry *view;
   struct nv50_miptree *mt = nv50_miptree(texture);
            view = MALLOC_STRUCT(nv50_tic_entry);
   if (!view)
            view->pipe = *templ;
   view->pipe.reference.count = 1;
   view->pipe.texture = NULL;
                                                                           swz[0] = nv50_tic_swizzle(fmt, view->pipe.swizzle_r, tex_int);
   swz[1] = nv50_tic_swizzle(fmt, view->pipe.swizzle_g, tex_int);
   swz[2] = nv50_tic_swizzle(fmt, view->pipe.swizzle_b, tex_int);
   swz[3] = nv50_tic_swizzle(fmt, view->pipe.swizzle_a, tex_int);
   tic[0] = (fmt->tic.format << G80_TIC_0_COMPONENTS_SIZES__SHIFT) |
            (fmt->tic.type_r << G80_TIC_0_R_DATA_TYPE__SHIFT) |
   (fmt->tic.type_g << G80_TIC_0_G_DATA_TYPE__SHIFT) |
   (fmt->tic.type_b << G80_TIC_0_B_DATA_TYPE__SHIFT) |
   (fmt->tic.type_a << G80_TIC_0_A_DATA_TYPE__SHIFT) |
   (swz[0] << G80_TIC_0_X_SOURCE__SHIFT) |
   (swz[1] << G80_TIC_0_Y_SOURCE__SHIFT) |
                           if (mt->base.base.array_size > 1) {
      /* there doesn't seem to be a base layer field in TIC */
   addr += view->pipe.u.tex.first_layer * mt->layer_stride;
                        if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB)
            if (!(flags & NV50_TEXVIEW_SCALED_COORDS))
            if (unlikely(!nouveau_bo_memtype(nv04_resource(texture)->bo))) {
      if (templ->target == PIPE_BUFFER) {
      addr += view->pipe.u.buf.offset;
   tic[2] |= G80_TIC_2_LAYOUT_PITCH | G80_TIC_2_TEXTURE_TYPE_ONE_D_BUFFER;
   tic[3] = 0;
   tic[4] = /* width */
            } else {
      tic[2] |= G80_TIC_2_LAYOUT_PITCH | G80_TIC_2_TEXTURE_TYPE_TWO_D_NO_MIPMAP;
   tic[3] = mt->level[0].pitch;
   tic[4] = mt->base.base.width0;
      }
   tic[6] =
   tic[7] = 0;
   tic[1] = addr;
   tic[2] |= addr >> 32;
               tic[1] = addr;
            tic[2] |=
      ((mt->level[0].tile_mode & 0x0f0) << (22 - 4)) |
         switch (templ->target) {
   case PIPE_TEXTURE_1D:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_ONE_D;
      case PIPE_TEXTURE_2D:
      if (mt->ms_x)
         else
            case PIPE_TEXTURE_RECT:
      tic[2] |= G80_TIC_2_TEXTURE_TYPE_TWO_D_NO_MIPMAP;
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
      case PIPE_BUFFER:
      assert(0); /* should be linear and handled above ! */
   tic[2] |= G80_TIC_2_TEXTURE_TYPE_ONE_D_BUFFER | G80_TIC_2_LAYOUT_PITCH;
      default:
                                    tic[5] = (mt->base.base.height0 << mt->ms_y) & 0xffff;
   tic[5] |= depth << 16;
   if (class_3d > NV50_3D_CLASS)
         else
                     if (class_3d > NV50_3D_CLASS)
         else
            if (unlikely(!(tic[2] & G80_TIC_2_NORMALIZED_COORDS)))
      if (mt->base.base.last_level)
            }
      static void
   nv50_update_tic(struct nv50_context *nv50, struct nv50_tic_entry *tic,
         {
      uint64_t address = res->address;
   if (res->base.target != PIPE_BUFFER)
         address += tic->pipe.u.buf.offset;
   if (tic->tic[1] == (uint32_t)address &&
      (tic->tic[2] & 0xff) == address >> 32)
         nv50_screen_tic_unlock(nv50->screen, tic);
   tic->id = -1;
   tic->tic[1] = address;
   tic->tic[2] &= 0xffffff00;
      }
      bool
   nv50_validate_tic(struct nv50_context *nv50, int s)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nouveau_bo *txc = nv50->screen->txc;
   unsigned i;
   bool need_flush = false;
            assert(nv50->num_textures[s] <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nv50->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nv50->textures[s][i]);
            if (!tic) {
      if (unlikely(is_compute_stage))
         else
         PUSH_DATA (push, (i << 1) | 0);
      }
   res = &nv50_miptree(tic->pipe.texture)->base;
            if (tic->id < 0) {
               BEGIN_NV04(push, NV50_2D(DST_FORMAT), 2);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(DST_PITCH), 5);
   PUSH_DATA (push, 262144);
   PUSH_DATA (push, 65536);
   PUSH_DATA (push, 1);
   PUSH_DATAh(push, txc->offset);
   PUSH_DATA (push, txc->offset);
   BEGIN_NV04(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   BEGIN_NV04(push, NV50_2D(SIFC_WIDTH), 10);
   PUSH_DATA (push, 32);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, tic->id * 32);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
                     } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      if (unlikely(is_compute_stage))
         else
                              res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            if (unlikely(is_compute_stage)) {
      BCTX_REFN(nv50->bufctx_cp, CP_TEXTURES, res, RD);
      } else {
      BCTX_REFN(nv50->bufctx_3d, 3D_TEXTURES, res, RD);
      }
      }
   for (; i < nv50->state.num_textures[s]; ++i) {
      if (unlikely(is_compute_stage))
         else
            }
   if (nv50->num_textures[s]) {
      if (unlikely(is_compute_stage))
         else
         PUSH_DATA (push, ((NV50_CB_AUX_TEX_MS_OFFSET + 16 * s * 2 * 4) << (8 - 2)) | NV50_CB_AUX);
   if (unlikely(is_compute_stage))
         else
         for (i = 0; i < nv50->num_textures[s]; i++) {
                     if (!tic || tic->pipe.target == PIPE_BUFFER) {
      PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
      }
   res = nv50_miptree(tic->pipe.texture);
   PUSH_DATA (push, res->ms_x);
         }
               }
      void nv50_validate_textures(struct nv50_context *nv50)
   {
      unsigned s;
            for (s = 0; s < NV50_MAX_3D_SHADER_STAGES; ++s)
            if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_3D(TIC_FLUSH), 1);
               /* Invalidate all CP textures because they are aliased. */
   nouveau_bufctx_reset(nv50->bufctx_cp, NV50_BIND_CP_TEXTURES);
      }
      bool
   nv50_validate_tsc(struct nv50_context *nv50, int s)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   unsigned i;
   bool need_flush = false;
            assert(nv50->num_samplers[s] <= PIPE_MAX_SAMPLERS);
   for (i = 0; i < nv50->num_samplers[s]; ++i) {
               if (!tsc) {
      if (is_compute_stage)
         else
         PUSH_DATA (push, (i << 4) | 0);
      }
   nv50->seamless_cube_map = tsc->seamless_cube_map;
   if (tsc->id < 0) {
               nv50_sifc_linear_u8(&nv50->base, nv50->screen->txc,
                  }
            if (is_compute_stage)
         else
            }
   for (; i < nv50->state.num_samplers[s]; ++i) {
      if (is_compute_stage)
         else
            }
            // TXF, in unlinked tsc mode, will always use sampler 0. So we have to
   // ensure that it remains bound. Its contents don't matter, all samplers we
   // ever create have the SRGB_CONVERSION bit set, so as long as the first
   // entry is initialized, we're good to go. This is the only bit that has
   // any effect on what TXF does.
   if (!nv50->samplers[s][0]) {
      if (is_compute_stage)
         else
                        }
      void nv50_validate_samplers(struct nv50_context *nv50)
   {
      unsigned s;
            for (s = 0; s < NV50_MAX_3D_SHADER_STAGES; ++s)
            if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_3D(TSC_FLUSH), 1);
               /* Invalidate all CP samplers because they are aliased. */
      }
      /* There can be up to 4 different MS levels (1, 2, 4, 8). To simplify the
   * shader logic, allow each one to take up 8 offsets.
   */
   #define COMBINE(x, y) x, y
   #define DUMMY 0, 0
   static const uint32_t msaa_sample_xy_offsets[] = {
      /* MS1 */
   COMBINE(0, 0),
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
            /* MS2 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
   DUMMY,
            /* MS4 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   COMBINE(0, 1),
   COMBINE(1, 1),
   DUMMY,
   DUMMY,
   DUMMY,
            /* MS8 */
   COMBINE(0, 0),
   COMBINE(1, 0),
   COMBINE(0, 1),
   COMBINE(1, 1),
   COMBINE(2, 0),
   COMBINE(3, 0),
   COMBINE(2, 1),
      };
      void nv50_upload_ms_info(struct nouveau_pushbuf *push)
   {
      BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, (NV50_CB_AUX_MS_OFFSET << (8 - 2)) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), ARRAY_SIZE(msaa_sample_xy_offsets));
      }
      void nv50_upload_tsc0(struct nv50_context *nv50)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   u32 data[8] = { G80_TSC_0_SRGB_CONVERSION };
   nv50_sifc_linear_u8(&nv50->base, nv50->screen->txc,
               BEGIN_NV04(push, NV50_3D(TSC_FLUSH), 1);
      }
