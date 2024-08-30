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
   #include "util/u_math.h"
   #include "util/half_float.h"
      #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_format.h"
   #include "nv30/nv30_winsys.h"
      static void
   nv30_validate_fb(struct nv30_context *nv30)
   {
      struct pipe_screen *pscreen = &nv30->screen->base.base;
   struct pipe_framebuffer_state *fb = &nv30->framebuffer;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
   uint32_t rt_format;
   int h = fb->height;
   int w = fb->width;
   int x = 0;
            nv30->state.rt_enable = (NV30_3D_RT_ENABLE_COLOR0 << fb->nr_cbufs) - 1;
   if (nv30->state.rt_enable > 1)
            rt_format = 0;
   if (fb->nr_cbufs > 0) {
      struct nv30_miptree *mt = nv30_miptree(fb->cbufs[0]->texture);
   rt_format |= nv30_format(pscreen, fb->cbufs[0]->format)->hw;
   rt_format |= mt->ms_mode;
   if (mt->swizzled)
         else
      } else {
      if (fb->zsbuf && util_format_get_blocksize(fb->zsbuf->format) > 2)
         else
               if (fb->zsbuf) {
      rt_format |= nv30_format(pscreen, fb->zsbuf->format)->hw;
   if (nv30_miptree(fb->zsbuf->texture)->swizzled)
         else
      } else {
      if (fb->nr_cbufs && util_format_get_blocksize(fb->cbufs[0]->format) > 2)
         else
               /* hardware rounds down render target offset to 64 bytes, but surfaces
   * with a size of 2x2 pixel (16bpp) or 1x1 pixel (32bpp) have an
   * unaligned start address.  For these two important square formats
   * we can hack around this limitation by adjusting the viewport origin
   */
   if (nv30->state.rt_enable) {
      int off = nv30_surface(fb->cbufs[0])->offset & 63;
   if (off) {
      x += off / (util_format_get_blocksize(fb->cbufs[0]->format) * 2);
   w  = 16;
                  if (rt_format & NV30_3D_RT_FORMAT_TYPE_SWIZZLED) {
      rt_format |= util_logbase2(w) << 16;
               if (!PUSH_SPACE(push, 64))
                  BEGIN_NV04(push, SUBC_3D(0x1da4), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(RT_HORIZ), 3);
   PUSH_DATA (push, w << 16);
   PUSH_DATA (push, h << 16);
   PUSH_DATA (push, rt_format);
   BEGIN_NV04(push, NV30_3D(VIEWPORT_TX_ORIGIN), 4);
   PUSH_DATA (push, (y << 16) | x);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, ((w - 1) << 16) | 0);
            if ((nv30->state.rt_enable & NV30_3D_RT_ENABLE_COLOR0) || fb->zsbuf) {
      struct nv30_surface *rsf = nv30_surface(fb->cbufs[0]);
   struct nv30_surface *zsf = nv30_surface(fb->zsbuf);
            if (!rsf)      rsf = zsf;
   else if (!zsf) zsf = rsf;
   rbo = nv30_miptree(rsf->base.texture)->base.bo;
            if (eng3d->oclass >= NV40_3D_CLASS) {
      BEGIN_NV04(push, NV40_3D(ZETA_PITCH), 1);
   PUSH_DATA (push, zsf->pitch);
   BEGIN_NV04(push, NV40_3D(COLOR0_PITCH), 3);
      } else {
      BEGIN_NV04(push, NV30_3D(COLOR0_PITCH), 3);
      }
   PUSH_MTHDl(push, NV30_3D(COLOR0_OFFSET), BUFCTX_FB, rbo, rsf->offset & ~63,
         PUSH_MTHDl(push, NV30_3D(ZETA_OFFSET), BUFCTX_FB, zbo, zsf->offset & ~63,
               if (nv30->state.rt_enable & NV30_3D_RT_ENABLE_COLOR1) {
      struct nv30_surface *sf = nv30_surface(fb->cbufs[1]);
            BEGIN_NV04(push, NV30_3D(COLOR1_OFFSET), 2);
   PUSH_MTHDl(push, NV30_3D(COLOR1_OFFSET), BUFCTX_FB, bo, sf->offset,
                     if (nv30->state.rt_enable & NV40_3D_RT_ENABLE_COLOR2) {
      struct nv30_surface *sf = nv30_surface(fb->cbufs[2]);
            BEGIN_NV04(push, NV40_3D(COLOR2_OFFSET), 1);
   PUSH_MTHDl(push, NV40_3D(COLOR2_OFFSET), BUFCTX_FB, bo, sf->offset,
         BEGIN_NV04(push, NV40_3D(COLOR2_PITCH), 1);
               if (nv30->state.rt_enable & NV40_3D_RT_ENABLE_COLOR3) {
      struct nv30_surface *sf = nv30_surface(fb->cbufs[3]);
            BEGIN_NV04(push, NV40_3D(COLOR3_OFFSET), 1);
   PUSH_MTHDl(push, NV40_3D(COLOR3_OFFSET), BUFCTX_FB, bo, sf->offset,
         BEGIN_NV04(push, NV40_3D(COLOR3_PITCH), 1);
         }
      static void
   nv30_validate_blend_colour(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
            if (nv30->framebuffer.nr_cbufs) {
      switch (nv30->framebuffer.cbufs[0]->format) {
   case PIPE_FORMAT_R16G16B16A16_FLOAT:
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      BEGIN_NV04(push, NV30_3D(BLEND_COLOR), 1);
   PUSH_DATA (push, (_mesa_float_to_half(rgba[0]) <<  0) |
         BEGIN_NV04(push, SUBC_3D(0x037c), 1);
   PUSH_DATA (push, (_mesa_float_to_half(rgba[2]) <<  0) |
            default:
                     BEGIN_NV04(push, NV30_3D(BLEND_COLOR), 1);
   PUSH_DATA (push, (float_to_ubyte(rgba[3]) << 24) |
                  }
      static void
   nv30_validate_stencil_ref(struct nv30_context *nv30)
   {
               BEGIN_NV04(push, NV30_3D(STENCIL_FUNC_REF(0)), 1);
   PUSH_DATA (push, nv30->stencil_ref.ref_value[0]);
   BEGIN_NV04(push, NV30_3D(STENCIL_FUNC_REF(1)), 1);
      }
      static void
   nv30_validate_stipple(struct nv30_context *nv30)
   {
               BEGIN_NV04(push, NV30_3D(POLYGON_STIPPLE_PATTERN(0)), 32);
      }
      static void
   nv30_validate_scissor(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct pipe_scissor_state *s = &nv30->scissor;
            if (!(nv30->dirty & NV30_NEW_SCISSOR) &&
      rast_scissor != nv30->state.scissor_off)
               BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   if (rast_scissor) {
      PUSH_DATA (push, ((s->maxx - s->minx) << 16) | s->minx);
      } else {
      PUSH_DATA (push, 0x10000000);
         }
      static void
   nv30_validate_viewport(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
            unsigned x = CLAMP(vp->translate[0] - fabsf(vp->scale[0]), 0, 4095);
   unsigned y = CLAMP(vp->translate[1] - fabsf(vp->scale[1]), 0, 4095);
   unsigned w = CLAMP(2.0f * fabsf(vp->scale[0]), 0, 4096);
            BEGIN_NV04(push, NV30_3D(VIEWPORT_TRANSLATE_X), 8);
   PUSH_DATAf(push, vp->translate[0]);
   PUSH_DATAf(push, vp->translate[1]);
   PUSH_DATAf(push, vp->translate[2]);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, vp->scale[0]);
   PUSH_DATAf(push, vp->scale[1]);
   PUSH_DATAf(push, vp->scale[2]);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV30_3D(DEPTH_RANGE_NEAR), 2);
   PUSH_DATAf(push, vp->translate[2] - fabsf(vp->scale[2]));
            BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
   PUSH_DATA (push, (w << 16) | x);
      }
      static void
   nv30_validate_clip(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   unsigned i;
            for (i = 0; i < 6; i++) {
      if (nv30->dirty & NV30_NEW_CLIP) {
      BEGIN_NV04(push, NV30_3D(VP_UPLOAD_CONST_ID), 5);
   PUSH_DATA (push, i);
      }
   if (nv30->rast->pipe.clip_plane_enable & (1 << i))
               BEGIN_NV04(push, NV30_3D(VP_CLIP_PLANES_ENABLE), 1);
      }
      static void
   nv30_validate_blend(struct nv30_context *nv30)
   {
               PUSH_SPACE(push, nv30->blend->size);
      }
      static void
   nv30_validate_zsa(struct nv30_context *nv30)
   {
               PUSH_SPACE(push, nv30->zsa->size);
      }
      static void
   nv30_validate_rasterizer(struct nv30_context *nv30)
   {
               PUSH_SPACE(push, nv30->rast->size);
      }
      static void
   nv30_validate_multisample(struct nv30_context *nv30)
   {
      struct pipe_rasterizer_state *rasterizer = &nv30->rast->pipe;
   struct pipe_blend_state *blend = &nv30->blend->pipe;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
            if (blend->alpha_to_one)
         if (blend->alpha_to_coverage)
         if (rasterizer->multisample)
            BEGIN_NV04(push, NV30_3D(MULTISAMPLE_CONTROL), 1);
      }
      static void
   nv30_validate_fragment(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
            BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
   PUSH_DATA (push, nv30->state.rt_enable & (fp ? ~fp->rt_enable : 0x1f));
   BEGIN_NV04(push, NV30_3D(COORD_CONVENTIONS), 1);
      }
      static void
   nv30_validate_point_coord(struct nv30_context *nv30)
   {
      struct pipe_rasterizer_state *rasterizer = &nv30->rast->pipe;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nv30_fragprog *fp = nv30->fragprog.program;
            if (rasterizer) {
      hw |= (nv30->rast->pipe.sprite_coord_enable & 0xff) << 8;
   if (fp)
            if (rasterizer->sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT) {
      if (hw)
      } else
   if (rasterizer->point_quad_rasterization) {
                     BEGIN_NV04(push, NV30_3D(POINT_SPRITE), 1);
      }
      struct state_validate {
      void (*func)(struct nv30_context *);
      };
      static struct state_validate hwtnl_validate_list[] = {
      { nv30_validate_fb,            NV30_NEW_FRAMEBUFFER },
   { nv30_validate_blend,         NV30_NEW_BLEND },
   { nv30_validate_zsa,           NV30_NEW_ZSA },
   { nv30_validate_rasterizer,    NV30_NEW_RASTERIZER },
   { nv30_validate_multisample,   NV30_NEW_SAMPLE_MASK | NV30_NEW_BLEND |
         { nv30_validate_blend_colour,  NV30_NEW_BLEND_COLOUR |
         { nv30_validate_stencil_ref,   NV30_NEW_STENCIL_REF },
   { nv30_validate_stipple,       NV30_NEW_STIPPLE },
   { nv30_validate_scissor,       NV30_NEW_SCISSOR | NV30_NEW_RASTERIZER },
   { nv30_validate_viewport,      NV30_NEW_VIEWPORT },
   { nv30_validate_clip,          NV30_NEW_CLIP | NV30_NEW_RASTERIZER },
   { nv30_fragprog_validate,      NV30_NEW_FRAGPROG | NV30_NEW_FRAGCONST },
   { nv30_vertprog_validate,      NV30_NEW_VERTPROG | NV30_NEW_VERTCONST |
         { nv30_validate_fragment,      NV30_NEW_FRAMEBUFFER | NV30_NEW_FRAGPROG },
   { nv30_validate_point_coord,   NV30_NEW_RASTERIZER | NV30_NEW_FRAGPROG },
   { nv30_fragtex_validate,       NV30_NEW_FRAGTEX },
   { nv40_verttex_validate,       NV30_NEW_VERTTEX },
   { nv30_vbo_validate,           NV30_NEW_VERTEX | NV30_NEW_ARRAYS },
      };
      #define NV30_SWTNL_MASK (NV30_NEW_VIEWPORT |  \
                           NV30_NEW_CLIP |      \
   NV30_NEW_VERTPROG |  \
      static struct state_validate swtnl_validate_list[] = {
      { nv30_validate_fb,            NV30_NEW_FRAMEBUFFER },
   { nv30_validate_blend,         NV30_NEW_BLEND },
   { nv30_validate_zsa,           NV30_NEW_ZSA },
   { nv30_validate_rasterizer,    NV30_NEW_RASTERIZER },
   { nv30_validate_multisample,   NV30_NEW_SAMPLE_MASK | NV30_NEW_BLEND |
         { nv30_validate_blend_colour,  NV30_NEW_BLEND_COLOUR |
         { nv30_validate_stencil_ref,   NV30_NEW_STENCIL_REF },
   { nv30_validate_stipple,       NV30_NEW_STIPPLE },
   { nv30_validate_scissor,       NV30_NEW_SCISSOR | NV30_NEW_RASTERIZER },
   { nv30_fragprog_validate,      NV30_NEW_FRAGPROG | NV30_NEW_FRAGCONST },
   { nv30_validate_fragment,      NV30_NEW_FRAMEBUFFER | NV30_NEW_FRAGPROG },
   { nv30_fragtex_validate,       NV30_NEW_FRAGTEX },
      };
      static void
   nv30_state_context_switch(struct nv30_context *nv30)
   {
               if (prev)
                  if (!nv30->vertex)
            if (!nv30->vertprog.program)
         if (!nv30->fragprog.program)
            if (!nv30->blend)
         if (!nv30->rast)
         if (!nv30->zsa)
               }
      bool
   nv30_state_validate(struct nv30_context *nv30, uint32_t mask, bool hwtnl)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_bufctx *bctx = nv30->bufctx;
   struct nouveau_bufref *bref;
            if (nv30->screen->cur_ctx != nv30)
            if (hwtnl) {
      nv30->draw_dirty |= nv30->dirty;
   if (nv30->draw_flags) {
      nv30->draw_flags &= ~nv30->dirty;
   if (!nv30->draw_flags)
                  if (!nv30->draw_flags)
         else
                     if (mask) {
      while (validate->func) {
      if (mask & validate->mask)
                                 nouveau_pushbuf_bufctx(push, bctx);
   if (PUSH_VAL(push)) {
      nouveau_pushbuf_bufctx(push, NULL);
               /*XXX*/
   BEGIN_NV04(push, NV30_3D(VTX_CACHE_INVALIDATE_1710), 1);
   PUSH_DATA (push, 0);
   if (nv30->screen->eng3d->oclass >= NV40_3D_CLASS) {
      BEGIN_NV04(push, NV40_3D(TEX_CACHE_CTL), 1);
   PUSH_DATA (push, 2);
   BEGIN_NV04(push, NV40_3D(TEX_CACHE_CTL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV30_3D(R1718), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(R1718), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(R1718), 1);
               LIST_FOR_EACH_ENTRY(bref, &bctx->current, thead) {
      struct nv04_resource *res = bref->priv;
   if (res && res->mm) {
                              if (bref->flags & NOUVEAU_BO_WR) {
      nouveau_fence_ref(nv30->base.fence, &res->fence_wr);
                        }
      void
   nv30_state_release(struct nv30_context *nv30)
   {
         }
