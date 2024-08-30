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
      #include "pipe/p_defines.h"
   #include "util/u_pack_color.h"
      #include "nouveau_gldefs.h"
   #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_winsys.h"
   #include "nv30/nv30_format.h"
      static inline uint32_t
   pack_rgba(enum pipe_format format, const float *rgba)
   {
      union util_color uc;
   util_pack_color(rgba, format, &uc);
      }
      static inline uint32_t
   pack_zeta(enum pipe_format format, double depth, unsigned stencil)
   {
      uint32_t zuint = (uint32_t)(depth * 4294967295.0);
   if (format != PIPE_FORMAT_Z16_UNORM)
            }
      static void
   nv30_clear(struct pipe_context *pipe, unsigned buffers, const struct pipe_scissor_state *scissor_state,
         {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nv30->framebuffer;
            if (!nv30_state_validate(nv30, NV30_NEW_FRAMEBUFFER, true))
            if (scissor_state) {
      uint32_t minx = scissor_state->minx;
   uint32_t maxx = MIN2(fb->width, scissor_state->maxx);
   uint32_t miny = scissor_state->miny;
            BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   PUSH_DATA (push, minx | (maxx - minx) << 16);
      }
   else {
      BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   PUSH_DATA (push, 0x10000000);
               if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      colr  = pack_rgba(fb->cbufs[0]->format, color->f);
   mode |= NV30_3D_CLEAR_BUFFERS_COLOR_R |
         NV30_3D_CLEAR_BUFFERS_COLOR_G |
               if (fb->zsbuf) {
      zeta = pack_zeta(fb->zsbuf->format, depth, stencil);
   if (buffers & PIPE_CLEAR_DEPTH)
         if (buffers & PIPE_CLEAR_STENCIL) {
      mode |= NV30_3D_CLEAR_BUFFERS_STENCIL;
   BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(0)), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0x000000ff);
                  /*XXX: wtf? fixes clears sometimes not clearing on nv3x... */
   if (nv30->screen->eng3d->oclass < NV40_3D_CLASS) {
      BEGIN_NV04(push, NV30_3D(CLEAR_DEPTH_VALUE), 3);
   PUSH_DATA (push, zeta);
   PUSH_DATA (push, colr);
               BEGIN_NV04(push, NV30_3D(CLEAR_DEPTH_VALUE), 3);
   PUSH_DATA (push, zeta);
   PUSH_DATA (push, colr);
                     /* Make sure regular draw commands will get their scissor state set */
   nv30->dirty |= NV30_NEW_SCISSOR;
      }
      static void
   nv30_clear_render_target(struct pipe_context *pipe, struct pipe_surface *ps,
                     {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_surface *sf = nv30_surface(ps);
   struct nv30_miptree *mt = nv30_miptree(ps->texture);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
            rt_format = nv30_format(pipe->screen, ps->format)->hw;
   if (util_format_get_blocksize(ps->format) == 4)
         else
            if (nv30_miptree(ps->texture)->swizzled) {
      rt_format |= NV30_3D_RT_FORMAT_TYPE_SWIZZLED;
   rt_format |= util_logbase2(sf->width) << 16;
      } else {
                  if (!PUSH_SPACE_EX(push, 32, 1, 0) ||
      PUSH_REF1(push, mt->base.bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR))
         BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
   PUSH_DATA (push, NV30_3D_RT_ENABLE_COLOR0);
   BEGIN_NV04(push, NV30_3D(RT_HORIZ), 3);
   PUSH_DATA (push, sf->width << 16);
   PUSH_DATA (push, sf->height << 16);
   PUSH_DATA (push, rt_format);
   BEGIN_NV04(push, NV30_3D(COLOR0_PITCH), 2);
   if (eng3d->oclass < NV40_3D_CLASS)
         else
         PUSH_RELOC(push, mt->base.bo, sf->offset, NOUVEAU_BO_LOW, 0, 0);
   BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   PUSH_DATA (push, (w << 16) | x);
            BEGIN_NV04(push, NV30_3D(CLEAR_COLOR_VALUE), 2);
   PUSH_DATA (push, pack_rgba(ps->format, color->f));
   PUSH_DATA (push, NV30_3D_CLEAR_BUFFERS_COLOR_R |
                        nv30->dirty |= NV30_NEW_FRAMEBUFFER | NV30_NEW_SCISSOR;
      }
      static void
   nv30_clear_depth_stencil(struct pipe_context *pipe, struct pipe_surface *ps,
                     {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct nv30_surface *sf = nv30_surface(ps);
   struct nv30_miptree *mt = nv30_miptree(ps->texture);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
            rt_format = nv30_format(pipe->screen, ps->format)->hw;
   if (util_format_get_blocksize(ps->format) == 4)
         else
            if (nv30_miptree(ps->texture)->swizzled) {
      rt_format |= NV30_3D_RT_FORMAT_TYPE_SWIZZLED;
   rt_format |= util_logbase2(sf->width) << 16;
      } else {
                  if (buffers & PIPE_CLEAR_DEPTH)
         if (buffers & PIPE_CLEAR_STENCIL)
            if (!PUSH_SPACE_EX(push, 32, 1, 0) ||
      PUSH_REF1(push, mt->base.bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR))
         BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(RT_HORIZ), 3);
   PUSH_DATA (push, sf->width << 16);
   PUSH_DATA (push, sf->height << 16);
   PUSH_DATA (push, rt_format);
   if (eng3d->oclass < NV40_3D_CLASS) {
      BEGIN_NV04(push, NV30_3D(COLOR0_PITCH), 1);
      } else {
      BEGIN_NV04(push, NV40_3D(ZETA_PITCH), 1);
      }
   BEGIN_NV04(push, NV30_3D(ZETA_OFFSET), 1);
   PUSH_RELOC(push, mt->base.bo, sf->offset, NOUVEAU_BO_LOW, 0, 0);
   BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   PUSH_DATA (push, (w << 16) | x);
            BEGIN_NV04(push, NV30_3D(CLEAR_DEPTH_VALUE), 1);
   PUSH_DATA (push, pack_zeta(ps->format, depth, stencil));
   BEGIN_NV04(push, NV30_3D(CLEAR_BUFFERS), 1);
            nv30->dirty |= NV30_NEW_FRAMEBUFFER | NV30_NEW_SCISSOR;
      }
      void
   nv30_clear_init(struct pipe_context *pipe)
   {
      pipe->clear = nv30_clear;
   pipe->clear_render_target = nv30_clear_render_target;
      }
