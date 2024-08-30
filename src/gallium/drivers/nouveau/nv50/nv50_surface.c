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
      #include <stdint.h>
      #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
      #include "pipe/p_defines.h"
      #include "util/u_inlines.h"
   #include "util/u_pack_color.h"
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_surface.h"
   #include "util/u_thread.h"
      #include "nv50_ir_driver.h"
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_resource.h"
      #include "nv50/g80_defs.xml.h"
   #include "nv50/g80_texture.xml.h"
      /* these are used in nv50_blit.h */
   #define NV50_ENG2D_SUPPORTED_FORMATS 0xff0843e080608409ULL
   #define NV50_ENG2D_NOCONVERT_FORMATS 0x0008402000000000ULL
   #define NV50_ENG2D_LUMINANCE_FORMATS 0x0008402000000000ULL
   #define NV50_ENG2D_INTENSITY_FORMATS 0x0000000000000000ULL
   #define NV50_ENG2D_OPERATION_FORMATS 0x060001c000608000ULL
      #define NOUVEAU_DRIVER 0x50
   #include "nv50/nv50_blit.h"
      static inline uint8_t
   nv50_2d_format(enum pipe_format format, bool dst, bool dst_src_equal)
   {
               /* Hardware values for color formats range from 0xc0 to 0xff,
   * but the 2D engine doesn't support all of them.
   */
   if ((id >= 0xc0) && (NV50_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0))))
                  switch (util_format_get_blocksize(format)) {
   case 1:
         case 2:
         case 4:
         case 8:
         case 16:
         default:
            }
      static int
   nv50_2d_texture_set(struct nouveau_pushbuf *push, int dst,
               {
      struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
            format = nv50_2d_format(pformat, dst, dst_src_pformat_equal);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                     width = u_minify(mt->base.base.width0, level) << mt->ms_x;
   height = u_minify(mt->base.base.height0, level) << mt->ms_y;
            offset = mt->level[level].offset;
   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
   depth = 1;
      } else
   if (!dst) {
      offset += nv50_mt_zslice_offset(mt, level, layer);
               if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, SUBC_2D(mthd), 2);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, SUBC_2D(mthd + 0x14), 5);
   PUSH_DATA (push, mt->level[level].pitch);
   PUSH_DATA (push, width);
   PUSH_DATA (push, height);
   PUSH_DATAh(push, mt->base.address + offset);
      } else {
      BEGIN_NV04(push, SUBC_2D(mthd), 5);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, mt->level[level].tile_mode);
   PUSH_DATA (push, depth);
   PUSH_DATA (push, layer);
   BEGIN_NV04(push, SUBC_2D(mthd + 0x18), 4);
   PUSH_DATA (push, width);
   PUSH_DATA (push, height);
   PUSH_DATAh(push, mt->base.address + offset);
            #if 0
      if (dst) {
      BEGIN_NV04(push, SUBC_2D(NV50_2D_CLIP_X), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, width);
         #endif
         }
      static int
   nv50_2d_texture_do_copy(struct nouveau_pushbuf *push,
                           struct nv50_miptree *dst, unsigned dst_level,
   {
      const enum pipe_format dfmt = dst->base.base.format;
   const enum pipe_format sfmt = src->base.base.format;
   int ret;
            if (!PUSH_SPACE(push, 2 * 16 + 32))
            ret = nv50_2d_texture_set(push, 1, dst, dst_level, dz, dfmt, eqfmt);
   if (ret)
            ret = nv50_2d_texture_set(push, 0, src, src_level, sz, sfmt, eqfmt);
   if (ret)
            BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, NV50_2D_BLIT_CONTROL_FILTER_POINT_SAMPLE);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dx << dst->ms_x);
   PUSH_DATA (push, dy << dst->ms_y);
   PUSH_DATA (push, w << dst->ms_x);
   PUSH_DATA (push, h << dst->ms_y);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sx << src->ms_x);
   PUSH_DATA (push, 0);
               }
      static void
   nv50_resource_copy_region(struct pipe_context *pipe,
                           {
      struct nv50_context *nv50 = nv50_context(pipe);
   int ret;
   bool m2mf;
            if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      nouveau_copy_buffer(&nv50->base,
                           /* 0 and 1 are equal, only supporting 0/1, 2, 4 and 8 */
            m2mf = (src->format == dst->format) ||
      (util_format_get_blocksizebits(src->format) ==
                  if (m2mf) {
      struct nv50_miptree *src_mt = nv50_miptree(src);
   struct nv50_miptree *dst_mt = nv50_miptree(dst);
   struct nv50_m2mf_rect drect, srect;
   unsigned i;
   unsigned nx = util_format_get_nblocksx(src->format, src_box->width)
         unsigned ny = util_format_get_nblocksy(src->format, src_box->height)
            nv50_m2mf_rect_setup(&drect, dst, dst_level, dstx, dsty, dstz);
   nv50_m2mf_rect_setup(&srect, src, src_level,
            for (i = 0; i < src_box->depth; ++i) {
               if (dst_mt->layout_3d)
                        if (src_mt->layout_3d)
         else
      }
               assert((src->format == dst->format) ||
                  BCTX_REFN(nv50->bufctx, 2D, nv04_resource(src), RD);
   BCTX_REFN(nv50->bufctx, 2D, nv04_resource(dst), WR);
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx);
            for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nv50_2d_texture_do_copy(nv50->base.pushbuf,
                                 if (ret)
      }
      }
      static void
   nv50_clear_render_target(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
                     BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
            if (!PUSH_SPACE_EX(push, 64 + sf->depth, 1, 0))
                     BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, ( width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, 8192 << 16);
   PUSH_DATA (push, 8192 << 16);
            BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(0)), 5);
   PUSH_DATAh(push, mt->base.address + sf->offset);
   PUSH_DATA (push, mt->base.address + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, mt->layer_stride >> 2);
   BEGIN_NV04(push, NV50_3D(RT_HORIZ(0)), 2);
   if (nouveau_bo_memtype(bo))
         else
         PUSH_DATA (push, sf->height);
   BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
   if (mt->layout_3d)
         else
            BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
            if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
                        BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
            if (!render_condition_enabled) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
               BEGIN_NI04(push, NV50_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, 0x3c |
               if (!render_condition_enabled) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
                  }
      static void
   nv50_clear_depth_stencil(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   unsigned clear_flags,
   double depth,
   {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t mode = 0;
            assert(dst->texture->target != PIPE_BUFFER);
            if (clear_flags & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
   PUSH_DATAf(push, depth);
               if (clear_flags & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
   PUSH_DATA (push, stencil & 0xff);
               if (!PUSH_SPACE_EX(push, 64 + sf->depth, 1, 0))
                     BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, ( width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, 8192 << 16);
   PUSH_DATA (push, 8192 << 16);
            BEGIN_NV04(push, NV50_3D(ZETA_ADDRESS_HIGH), 5);
   PUSH_DATAh(push, mt->base.address + sf->offset);
   PUSH_DATA (push, mt->base.address + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, mt->layer_stride >> 2);
   BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(ZETA_HORIZ), 3);
   PUSH_DATA (push, sf->width);
   PUSH_DATA (push, sf->height);
            BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
            BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
            BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
            if (!render_condition_enabled) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
               BEGIN_NI04(push, NV50_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, mode |
               if (!render_condition_enabled) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
                  }
      void
   nv50_clear(struct pipe_context *pipe, unsigned buffers, const struct pipe_scissor_state *scissor_state,
               {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i, j, k;
                     /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   if (!nv50_state_validate_3d(nv50, NV50_NEW_3D_FRAMEBUFFER))
            if (scissor_state) {
      uint32_t minx = scissor_state->minx;
   uint32_t maxx = MIN2(fb->width, scissor_state->maxx);
   uint32_t miny = scissor_state->miny;
   uint32_t maxy = MIN2(fb->height, scissor_state->maxy);
   if (maxx <= minx || maxy <= miny)
            BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, minx | (maxx - minx) << 16);
               /* We have to clear ALL of the layers, not up to the min number of layers
   * of any attachment. */
   BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
            if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
   PUSH_DATAf(push, color->f[3]);
   if (buffers & PIPE_CLEAR_COLOR0)
      mode =
                  if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
   PUSH_DATA (push, fui(depth));
               if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
   PUSH_DATA (push, stencil & 0xff);
               if (mode) {
      int zs_layers = 0, color0_layers = 0;
   if (fb->cbufs[0] && (mode & 0x3c))
         if (fb->zsbuf && (mode & ~0x3c))
            for (j = 0; j < MIN2(zs_layers, color0_layers); j++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
      }
   for (k = j; k < zs_layers; k++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
      }
   for (k = j; k < color0_layers; k++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
                  for (i = 1; i < fb->nr_cbufs; i++) {
      struct pipe_surface *sf = fb->cbufs[i];
   if (!sf || !(buffers & (PIPE_CLEAR_COLOR0 << i)))
         for (j = 0; j < nv50_surface(sf)->depth; j++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, (i << 6) | 0x3c |
                  /* restore the array mode */
   BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
            /* restore screen scissor */
   if (scissor_state) {
      BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, fb->width << 16);
            out:
      PUSH_KICK(push);
      }
      static void
   nv50_clear_buffer_push(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(res);
   unsigned count = (size + 3) / 4;
   unsigned xcoord = offset & 0xff;
            if (data_size == 1) {
      tmp = *(unsigned char *)data;
   tmp = (tmp << 24) | (tmp << 16) | (tmp << 8) | tmp;
   data = &tmp;
      } else if (data_size == 2) {
      tmp = *(unsigned short *)data;
   tmp = (tmp << 16) | tmp;
   data = &tmp;
                        nouveau_bufctx_refn(nv50->bufctx, 0, buf->bo, buf->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nv50->bufctx);
                     BEGIN_NV04(push, NV50_2D(DST_FORMAT), 2);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(DST_PITCH), 5);
   PUSH_DATA (push, 262144);
   PUSH_DATA (push, 65536);
   PUSH_DATA (push, 1);
   PUSH_DATAh(push, buf->address + offset);
   PUSH_DATA (push, buf->address + offset);
   BEGIN_NV04(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   BEGIN_NV04(push, NV50_2D(SIFC_WIDTH), 10);
   PUSH_DATA (push, size);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, xcoord);
   PUSH_DATA (push, 0);
            while (count) {
      unsigned nr_data = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN) / data_words;
            BEGIN_NI04(push, NV50_2D(SIFC_DATA), nr);
   for (i = 0; i < nr_data; i++)
                                    }
      static void
   nv50_clear_buffer(struct pipe_context *pipe,
                     {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv04_resource *buf = (struct nv04_resource *)res;
   union pipe_color_union color;
   enum pipe_format dst_fmt;
            assert(res->target == PIPE_BUFFER);
            switch (data_size) {
   case 16:
      dst_fmt = PIPE_FORMAT_R32G32B32A32_UINT;
   memcpy(&color.ui, data, 16);
      case 8:
      dst_fmt = PIPE_FORMAT_R32G32_UINT;
   memcpy(&color.ui, data, 8);
   memset(&color.ui[2], 0, 8);
      case 4:
      dst_fmt = PIPE_FORMAT_R32_UINT;
   memcpy(&color.ui, data, 4);
   memset(&color.ui[1], 0, 12);
      case 2:
      dst_fmt = PIPE_FORMAT_R16_UINT;
   color.ui[0] = util_cpu_to_le32(
         memset(&color.ui[1], 0, 12);
      case 1:
      dst_fmt = PIPE_FORMAT_R8_UINT;
   color.ui[0] = util_cpu_to_le32(*(unsigned char *)data);
   memset(&color.ui[1], 0, 12);
      default:
      assert(!"Unsupported element size");
                                 if (offset & 0xff) {
      unsigned fixup_size = MIN2(size, align(offset, 0x100) - offset);
   assert(fixup_size % data_size == 0);
   nv50_clear_buffer_push(pipe, res, offset, fixup_size, data, data_size);
   offset += fixup_size;
   size -= fixup_size;
   if (!size)
               elements = size / data_size;
   height = (elements + 8191) / 8192;
   width = elements / height;
   if (height > 1)
                  BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATA (push, color.ui[0]);
   PUSH_DATA (push, color.ui[1]);
   PUSH_DATA (push, color.ui[2]);
            if (!PUSH_SPACE_EX(push, 64, 1, 0))
                     BEGIN_NV04(push, NV50_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, width << 16);
   PUSH_DATA (push, height << 16);
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, 8192 << 16);
   PUSH_DATA (push, 8192 << 16);
            BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(0)), 5);
   PUSH_DATAh(push, buf->address + offset);
   PUSH_DATA (push, buf->address + offset);
   PUSH_DATA (push, nv50_format_table[dst_fmt].rt);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(RT_HORIZ(0)), 2);
   PUSH_DATA (push, NV50_3D_RT_HORIZ_LINEAR | align(width * data_size, 0x100));
   PUSH_DATA (push, height);
   BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_MODE), 1);
                     BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16));
            BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
            BEGIN_NI04(push, NV50_3D(CLEAR_BUFFERS), 1);
            BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
                     if (width * height != elements) {
      offset += width * height * data_size;
   width = elements - width * height;
   nv50_clear_buffer_push(pipe, res, offset, width * data_size,
                  }
      /* =============================== BLIT CODE ===================================
   */
      struct nv50_blitter
   {
      struct nv50_program *fp[NV50_BLIT_MAX_TEXTURE_TYPES][NV50_BLIT_MODES];
                        };
      struct nv50_blitctx
   {
      struct nv50_context *nv50;
   struct nv50_program *fp;
   uint8_t mode;
   uint16_t color_mask;
   uint8_t filter;
   uint8_t render_condition_enable;
   enum pipe_texture_target target;
   struct {
      struct pipe_framebuffer_state fb;
   struct nv50_window_rect_stateobj window_rect;
   struct nv50_rasterizer_stateobj *rast;
   struct nv50_program *vp;
   struct nv50_program *gp;
   struct nv50_program *fp;
   unsigned num_textures[NV50_MAX_3D_SHADER_STAGES];
   unsigned num_samplers[NV50_MAX_3D_SHADER_STAGES];
   struct pipe_sampler_view *texture[2];
   struct nv50_tsc_entry *sampler[2];
   unsigned min_samples;
      } saved;
      };
      static void
   nv50_blitter_make_vp(struct nv50_blitter *blit)
   {
      static const uint32_t code[] =
   {
      0x10000001, 0x0423c788, /* mov b32 o[0x00] s[0x00] */ /* HPOS.x */
   0x10000205, 0x0423c788, /* mov b32 o[0x04] s[0x04] */ /* HPOS.y */
   0x10000409, 0x0423c788, /* mov b32 o[0x08] s[0x08] */ /* TEXC.x */
   0x1000060d, 0x0423c788, /* mov b32 o[0x0c] s[0x0c] */ /* TEXC.y */
               blit->vp.type = PIPE_SHADER_VERTEX;
   blit->vp.translated = true;
   blit->vp.code = (uint32_t *)code; /* const_cast */
   blit->vp.code_size = sizeof(code);
   blit->vp.max_gpr = 4;
   blit->vp.max_out = 5;
   blit->vp.out_nr = 2;
   blit->vp.out[0].mask = 0x3;
   blit->vp.out[0].sn = TGSI_SEMANTIC_POSITION;
   blit->vp.out[1].hw = 2;
   blit->vp.out[1].mask = 0x7;
   blit->vp.out[1].sn = TGSI_SEMANTIC_GENERIC;
   blit->vp.out[1].si = 0;
   blit->vp.vp.attrs[0] = 0x73;
   blit->vp.vp.psiz = 0x40;
      }
      void *
   nv50_blitter_make_fp(struct pipe_context *pipe,
               {
      enum glsl_sampler_dim sampler_dim
                  bool tex_rgbaz = false;
   bool tex_s = false;
            bool int_clamp = mode == NV50_BLIT_MODE_INT_CLAMP;
   if (int_clamp)
            if (mode != NV50_BLIT_MODE_PASS &&
      mode != NV50_BLIT_MODE_Z24X8 &&
   mode != NV50_BLIT_MODE_X8Z24)
         if (mode != NV50_BLIT_MODE_X24S8 &&
      mode != NV50_BLIT_MODE_S8X24 &&
   mode != NV50_BLIT_MODE_XS)
         if (mode != NV50_BLIT_MODE_PASS &&
      mode != NV50_BLIT_MODE_ZS &&
   mode != NV50_BLIT_MODE_XS)
         const int chipset = nouveau_screen(pipe->screen)->device->chipset;
   const nir_shader_compiler_options *options =
            struct nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_FRAGMENT, options,
         /* load coordinates */
   const struct glsl_type* float3 = glsl_vector_type(GLSL_TYPE_FLOAT, 3);
   nir_variable *coord_var =
         coord_var->data.location = VARYING_SLOT_VAR0;
            nir_def *coord = nir_load_var(&b, coord_var);
   if (ptarg == PIPE_TEXTURE_1D_ARRAY) {
      /* Adjust coordinates. Depth is in z, but TEX expects it to be in y. */
      } else {
      int size = glsl_get_sampler_dim_coordinate_components(sampler_dim);
   if (is_array) size += 1;
               /* sample textures */
   const struct glsl_type *sampler_type =
            nir_def *s = NULL;
   if (tex_s) {
      nir_variable *sampler =
      nir_variable_create(b.shader, nir_var_uniform,
                        s = nir_tex_deref(&b, tex_deref, tex_deref, coord);
               nir_def *rgba = NULL, *z = NULL;
   if (tex_rgbaz) {
      nir_variable *sampler =
      nir_variable_create(b.shader, nir_var_uniform,
                        rgba = nir_tex_deref(&b, tex_deref, tex_deref, coord);
               /* handle signed to unsigned integer conversions */
   if (int_clamp) {
                  /* handle conversions */
   nir_def *out_ssa;
   nir_component_mask_t out_mask = 0;
   if (cvt_un8) {
      if (tex_s) {
      s = nir_i2f32(&b, s);
      } else {
                  if (tex_rgbaz) {
      z = nir_fmul_imm(&b, z, (1 << 24) - 1);
   z = nir_f2i32(&b, z);
   z = nir_iand(&b, z, nir_imm_ivec3(&b, 0x0000ff,
               z = nir_i2f32(&b, z);
   z = nir_fmul(&b, z, nir_imm_vec3(&b, 1.0f / 0x0000ff,
            } else {
                  if (mode == NV50_BLIT_MODE_Z24S8 ||
      mode == NV50_BLIT_MODE_X24S8 ||
   mode == NV50_BLIT_MODE_Z24X8) {
   out_ssa = nir_vec4(&b,
                              if (tex_rgbaz) out_mask |= TGSI_WRITEMASK_XYZ;
      } else {
      out_ssa = nir_vec4(&b,
                              if (tex_rgbaz) out_mask |= TGSI_WRITEMASK_YZW;
         } else {
      if (mode == NV50_BLIT_MODE_PASS) {
      out_ssa = rgba;
      } else {
      out_ssa = nir_vec2(&b, z ? z : nir_undef(&b, 1, 32),
         if (tex_rgbaz) out_mask |= TGSI_WRITEMASK_X;
                  /* write output */
   const struct glsl_type* out_type =
         nir_variable *out_var =
                           /* return shader */
            struct pipe_shader_state state;
   pipe_shader_state_from_nir(&state, b.shader);
      }
      static void
   nv50_blitter_make_sampler(struct nv50_blitter *blit)
   {
                        blit->sampler[0].tsc[0] = G80_TSC_0_SRGB_CONVERSION |
      (G80_TSC_WRAP_CLAMP_TO_EDGE << G80_TSC_0_ADDRESS_U__SHIFT) |
   (G80_TSC_WRAP_CLAMP_TO_EDGE << G80_TSC_0_ADDRESS_V__SHIFT) |
      blit->sampler[0].tsc[1] =
      G80_TSC_1_MAG_FILTER_NEAREST |
   G80_TSC_1_MIN_FILTER_NEAREST |
                           blit->sampler[1].tsc[0] = blit->sampler[0].tsc[0];
   blit->sampler[1].tsc[1] =
      G80_TSC_1_MAG_FILTER_LINEAR |
   G80_TSC_1_MIN_FILTER_LINEAR |
   }
      unsigned
   nv50_blit_select_mode(const struct pipe_blit_info *info)
   {
               switch (info->dst.resource->format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X24S8_UINT:
      switch (mask & PIPE_MASK_ZS) {
   case PIPE_MASK_ZS: return NV50_BLIT_MODE_Z24S8;
   case PIPE_MASK_Z:  return NV50_BLIT_MODE_Z24X8;
   default:
            case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_S8X24_UINT:
      switch (mask & PIPE_MASK_ZS) {
   case PIPE_MASK_ZS: return NV50_BLIT_MODE_S8Z24;
   case PIPE_MASK_Z:  return NV50_BLIT_MODE_X8Z24;
   default:
            case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
   case PIPE_FORMAT_X32_S8X24_UINT:
      switch (mask & PIPE_MASK_ZS) {
   case PIPE_MASK_ZS: return NV50_BLIT_MODE_ZS;
   case PIPE_MASK_Z:  return NV50_BLIT_MODE_PASS;
   default:
            default:
      if (util_format_is_pure_uint(info->src.format) &&
      util_format_is_pure_sint(info->dst.format))
            }
      static void
   nv50_blit_select_fp(struct nv50_blitctx *ctx, const struct pipe_blit_info *info)
   {
               const enum pipe_texture_target ptarg =
            const unsigned targ = nv50_blit_texture_type(ptarg);
            if (!blitter->fp[targ][mode]) {
      mtx_lock(&blitter->mutex);
   if (!blitter->fp[targ][mode])
      blitter->fp[targ][mode] =
         }
      }
      static void
   nv50_blit_set_dst(struct nv50_blitctx *ctx,
               {
      struct nv50_context *nv50 = ctx->nv50;
   struct pipe_context *pipe = &nv50->base.pipe;
            if (util_format_is_depth_or_stencil(format))
         else
            templ.u.tex.level = level;
            if (layer == -1) {
      templ.u.tex.first_layer = 0;
   templ.u.tex.last_layer =
               nv50->framebuffer.cbufs[0] = nv50_miptree_surface_new(pipe, res, &templ);
   nv50->framebuffer.nr_cbufs = 1;
   nv50->framebuffer.zsbuf = NULL;
   nv50->framebuffer.width = nv50->framebuffer.cbufs[0]->width;
      }
      static void
   nv50_blit_set_src(struct nv50_blitctx *blit,
               {
      struct nv50_context *nv50 = blit->nv50;
   struct pipe_context *pipe = &nv50->base.pipe;
   struct pipe_sampler_view templ = {0};
   uint32_t flags;
                     templ.target = target;
   templ.format = format;
   templ.u.tex.first_level = templ.u.tex.last_level = level;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;
   templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
            if (layer == -1) {
      templ.u.tex.first_layer = 0;
   templ.u.tex.last_layer =
               flags = res->last_level ? 0 : NV50_TEXVIEW_SCALED_COORDS;
   flags |= NV50_TEXVIEW_ACCESS_RESOLVE;
   if (filter && res->nr_samples == 8)
            nv50->textures[NV50_SHADER_STAGE_FRAGMENT][0] = nv50_create_texture_view(
                  nv50->num_textures[NV50_SHADER_STAGE_VERTEX] = 0;
   nv50->num_textures[NV50_SHADER_STAGE_GEOMETRY] = 0;
            templ.format = nv50_zs_to_s_format(format);
   if (templ.format != res->format) {
      nv50->textures[NV50_SHADER_STAGE_FRAGMENT][1] = nv50_create_texture_view(
               }
      static void
   nv50_blitctx_prepare_state(struct nv50_blitctx *blit)
   {
               if (blit->nv50->cond_query && !blit->render_condition_enable) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
               /* blend state */
   BEGIN_NV04(push, NV50_3D(COLOR_MASK(0)), 1);
   PUSH_DATA (push, blit->color_mask);
   BEGIN_NV04(push, NV50_3D(BLEND_ENABLE(0)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(LOGIC_OP_ENABLE), 1);
               #ifndef NV50_SCISSORS_CLIPPING
      BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(0)), 1);
      #endif
      BEGIN_NV04(push, NV50_3D(VERTEX_TWO_SIDE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   BEGIN_NV04(push, NV50_3D(POLYGON_MODE_FRONT), 3);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_FRONT_FILL);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_BACK_FILL);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(CULL_FACE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_STIPPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_OFFSET_FILL_ENABLE), 1);
            /* zsa state */
   BEGIN_NV04(push, NV50_3D(DEPTH_TEST_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(DEPTH_BOUNDS_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(STENCIL_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ALPHA_TEST_ENABLE), 1);
      }
      static void
   nv50_blitctx_pre_blit(struct nv50_blitctx *ctx,
         {
      struct nv50_context *nv50 = ctx->nv50;
   struct nv50_blitter *blitter = nv50->screen->blitter;
            ctx->saved.fb.width = nv50->framebuffer.width;
   ctx->saved.fb.height = nv50->framebuffer.height;
   ctx->saved.fb.nr_cbufs = nv50->framebuffer.nr_cbufs;
   ctx->saved.fb.cbufs[0] = nv50->framebuffer.cbufs[0];
                     ctx->saved.vp = nv50->vertprog;
   ctx->saved.gp = nv50->gmtyprog;
            ctx->saved.min_samples = nv50->min_samples;
                     nv50->vertprog = &blitter->vp;
   nv50->gmtyprog = NULL;
            nv50->window_rect.rects =
         nv50->window_rect.inclusive = info->window_rectangle_include;
   if (nv50->window_rect.rects)
      memcpy(nv50->window_rect.rect, info->window_rectangles,
         for (s = 0; s < NV50_MAX_3D_SHADER_STAGES; ++s) {
      ctx->saved.num_textures[s] = nv50->num_textures[s];
      }
   ctx->saved.texture[0] = nv50->textures[NV50_SHADER_STAGE_FRAGMENT][0];
   ctx->saved.texture[1] = nv50->textures[NV50_SHADER_STAGE_FRAGMENT][1];
   ctx->saved.sampler[0] = nv50->samplers[NV50_SHADER_STAGE_FRAGMENT][0];
            nv50->samplers[NV50_SHADER_STAGE_FRAGMENT][0] = &blitter->sampler[ctx->filter];
            nv50->num_samplers[NV50_SHADER_STAGE_VERTEX] = 0;
   nv50->num_samplers[NV50_SHADER_STAGE_GEOMETRY] = 0;
                              nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_3D_FB);
            nv50->dirty_3d =
      NV50_NEW_3D_FRAMEBUFFER | NV50_NEW_3D_MIN_SAMPLES |
   NV50_NEW_3D_VERTPROG | NV50_NEW_3D_FRAGPROG | NV50_NEW_3D_GMTYPROG |
   }
      static void
   nv50_blitctx_post_blit(struct nv50_blitctx *blit)
   {
      struct nv50_context *nv50 = blit->nv50;
                     nv50->framebuffer.width = blit->saved.fb.width;
   nv50->framebuffer.height = blit->saved.fb.height;
   nv50->framebuffer.nr_cbufs = blit->saved.fb.nr_cbufs;
   nv50->framebuffer.cbufs[0] = blit->saved.fb.cbufs[0];
                     nv50->vertprog = blit->saved.vp;
   nv50->gmtyprog = blit->saved.gp;
            nv50->min_samples = blit->saved.min_samples;
            pipe_sampler_view_reference(&nv50->textures[NV50_SHADER_STAGE_FRAGMENT][0], NULL);
            for (s = 0; s < NV50_MAX_3D_SHADER_STAGES; ++s) {
      nv50->num_textures[s] = blit->saved.num_textures[s];
      }
   nv50->textures[NV50_SHADER_STAGE_FRAGMENT][0] = blit->saved.texture[0];
   nv50->textures[NV50_SHADER_STAGE_FRAGMENT][1] = blit->saved.texture[1];
   nv50->samplers[NV50_SHADER_STAGE_FRAGMENT][0] = blit->saved.sampler[0];
            if (nv50->cond_query && !blit->render_condition_enable)
      nv50->base.pipe.render_condition(&nv50->base.pipe, nv50->cond_query,
         nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_3D_FB);
            nv50->dirty_3d = blit->saved.dirty_3d |
      (NV50_NEW_3D_FRAMEBUFFER | NV50_NEW_3D_SCISSOR | NV50_NEW_3D_SAMPLE_MASK |
   NV50_NEW_3D_RASTERIZER | NV50_NEW_3D_ZSA | NV50_NEW_3D_BLEND |
   NV50_NEW_3D_TEXTURES | NV50_NEW_3D_SAMPLERS | NV50_NEW_3D_WINDOW_RECTS |
                  }
         static void
   nv50_blit_3d(struct nv50_context *nv50, const struct pipe_blit_info *info)
   {
      struct nv50_blitctx *blit = nv50->blit;
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_resource *src = info->src.resource;
   struct pipe_resource *dst = info->dst.resource;
   int32_t minx, maxx, miny, maxy;
   int32_t i;
   float x0, x1, y0, y1, z;
   float dz;
            blit->mode = nv50_blit_select_mode(info);
   blit->color_mask = nv50_blit_derive_color_mask(info);
   blit->filter = nv50_blit_get_filter(info);
            nv50_blit_select_fp(blit, info);
            nv50_blit_set_dst(blit, dst, info->dst.level, -1, info->dst.format);
   nv50_blit_set_src(blit, src, info->src.level, -1, info->src.format,
                              /* When flipping a surface from zeta <-> color "mode", we have to wait for
   * the GPU to flush its current draws.
   */
   struct nv50_miptree *mt = nv50_miptree(dst);
   bool serialize = util_format_is_depth_or_stencil(info->dst.format);
   if (serialize && mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      BEGIN_NV04(push, SUBC_3D(NV50_GRAPH_SERIALIZE), 1);
               x_range = (float)info->src.box.width / (float)info->dst.box.width;
            x0 = (float)info->src.box.x - x_range * (float)info->dst.box.x;
            x1 = x0 + 16384.0f * x_range;
            x0 *= (float)(1 << nv50_miptree(src)->ms_x);
   x1 *= (float)(1 << nv50_miptree(src)->ms_x);
   y0 *= (float)(1 << nv50_miptree(src)->ms_y);
            /* XXX: multiply by 6 for cube arrays ? */
   dz = (float)info->src.box.depth / (float)info->dst.box.depth;
   z = (float)info->src.box.z;
   if (nv50_miptree(src)->layout_3d)
            if (src->last_level > 0) {
      /* If there are mip maps, GPU always assumes normalized coordinates. */
   const unsigned l = info->src.level;
   const float fh = u_minify(src->width0 << nv50_miptree(src)->ms_x, l);
   const float fv = u_minify(src->height0 << nv50_miptree(src)->ms_y, l);
   x0 /= fh;
   x1 /= fh;
   y0 /= fv;
   y1 /= fv;
   if (nv50_miptree(src)->layout_3d) {
      z /= u_minify(src->depth0, l);
                  BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(VIEW_VOLUME_CLIP_CTRL), 1);
            /* Draw a large triangle in screen coordinates covering the whole
   * render target, with scissors defining the destination region.
   * The vertex is supplied with non-normalized texture coordinates
   * arranged in a way to yield the desired offset and scale.
            minx = info->dst.box.x;
   maxx = info->dst.box.x + info->dst.box.width;
   miny = info->dst.box.y;
   maxy = info->dst.box.y + info->dst.box.height;
   if (info->scissor_enable) {
      minx = MAX2(minx, info->scissor.minx);
   maxx = MIN2(maxx, info->scissor.maxx);
   miny = MAX2(miny, info->scissor.miny);
      }
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (maxx << 16) | minx);
            for (i = 0; i < info->dst.box.depth; ++i, z += dz) {
      if (info->dst.box.z + i) {
      BEGIN_NV04(push, NV50_3D(LAYER), 1);
      }
   PUSH_SPACE(push, 32);
   BEGIN_NV04(push, NV50_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (push, NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y0);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x1);
   PUSH_DATAf(push, y0);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 16384.0f);
   PUSH_DATAf(push, 0.0f);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
   PUSH_DATAf(push, x0);
   PUSH_DATAf(push, y1);
   PUSH_DATAf(push, z);
   BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
   PUSH_DATAf(push, 0.0f);
   PUSH_DATAf(push, 16384.0f);
   BEGIN_NV04(push, NV50_3D(VERTEX_END_GL), 1);
      }
   if (info->dst.box.z + info->dst.box.depth - 1) {
      BEGIN_NV04(push, NV50_3D(LAYER), 1);
                        BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
            /* mark the surface as reading, which will force a serialize next time it's
   * used for writing.
   */
   if (serialize)
               }
      static void
   nv50_blit_eng2d(struct nv50_context *nv50, const struct pipe_blit_info *info)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *dst = nv50_miptree(info->dst.resource);
   struct nv50_miptree *src = nv50_miptree(info->src.resource);
   const int32_t srcx_adj = info->src.box.width < 0 ? -1 : 0;
   const int32_t srcy_adj = info->src.box.height < 0 ? -1 : 0;
   const int32_t dz = info->dst.box.z;
   const int32_t sz = info->src.box.z;
   uint32_t dstw, dsth;
   int32_t dstx, dsty;
   int64_t srcx, srcy;
   int64_t du_dx, dv_dy;
   int i;
   uint32_t mode;
   uint32_t mask = nv50_blit_eng2d_get_mask(info);
            mode = nv50_blit_get_filter(info) ?
      NV50_2D_BLIT_CONTROL_FILTER_BILINEAR :
      mode |= (src->base.base.nr_samples > dst->base.base.nr_samples) ?
            du_dx = ((int64_t)info->src.box.width << 32) / info->dst.box.width;
            b = info->dst.format == info->src.format;
   nv50_2d_texture_set(push, 1, dst, info->dst.level, dz, info->dst.format, b);
            if (info->scissor_enable) {
      BEGIN_NV04(push, NV50_2D(CLIP_X), 5);
   PUSH_DATA (push, info->scissor.minx << dst->ms_x);
   PUSH_DATA (push, info->scissor.miny << dst->ms_y);
   PUSH_DATA (push, (info->scissor.maxx - info->scissor.minx) << dst->ms_x);
   PUSH_DATA (push, (info->scissor.maxy - info->scissor.miny) << dst->ms_y);
               if (nv50->cond_query && info->render_condition_enable) {
      BEGIN_NV04(push, NV50_2D(COND_MODE), 1);
               if (mask != 0xffffffff) {
      BEGIN_NV04(push, NV50_2D(ROP), 1);
   PUSH_DATA (push, 0xca); /* DPSDxax */
   BEGIN_NV04(push, NV50_2D(PATTERN_COLOR_FORMAT), 1);
   PUSH_DATA (push, NV50_2D_PATTERN_COLOR_FORMAT_A8R8G8B8);
   BEGIN_NV04(push, NV50_2D(PATTERN_BITMAP_COLOR(0)), 4);
   PUSH_DATA (push, 0x00000000);
   PUSH_DATA (push, mask);
   PUSH_DATA (push, 0xffffffff);
   PUSH_DATA (push, 0xffffffff);
   BEGIN_NV04(push, NV50_2D(OPERATION), 1);
      } else
   if (info->src.format != info->dst.format) {
      if (info->src.format == PIPE_FORMAT_R8_UNORM ||
      info->src.format == PIPE_FORMAT_R16_UNORM ||
   info->src.format == PIPE_FORMAT_R16_FLOAT ||
   info->src.format == PIPE_FORMAT_R32_FLOAT) {
   mask = 0xffff0000; /* also makes condition for OPERATION reset true */
   BEGIN_NV04(push, NV50_2D(BETA4), 2);
   PUSH_DATA (push, mask);
                  if (src->ms_x > dst->ms_x || src->ms_y > dst->ms_y) {
      /* ms_x is always >= ms_y */
   du_dx <<= src->ms_x - dst->ms_x;
      } else {
      du_dx >>= dst->ms_x - src->ms_x;
               srcx = (int64_t)(info->src.box.x + srcx_adj) << (src->ms_x + 32);
            if (src->base.base.nr_samples > dst->base.base.nr_samples) {
      /* center src coorinates for proper MS resolve filtering */
   srcx += (int64_t)1 << (src->ms_x + 31);
               dstx = info->dst.box.x << dst->ms_x;
            dstw = info->dst.box.width << dst->ms_x;
            if (dstx < 0) {
      dstw += dstx;
   srcx -= du_dx * dstx;
      }
   if (dsty < 0) {
      dsth += dsty;
   srcy -= dv_dy * dsty;
               BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, mode);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dstx);
   PUSH_DATA (push, dsty);
   PUSH_DATA (push, dstw);
   PUSH_DATA (push, dsth);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, du_dx);
   PUSH_DATA (push, du_dx >> 32);
   PUSH_DATA (push, dv_dy);
            BCTX_REFN(nv50->bufctx, 2D, &dst->base, WR);
   BCTX_REFN(nv50->bufctx, 2D, &src->base, RD);
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx);
   if (PUSH_VAL(nv50->base.pushbuf))
            for (i = 0; i < info->dst.box.depth; ++i) {
      if (i > 0) {
      /* no scaling in z-direction possible for eng2d blits */
   if (dst->layout_3d) {
      BEGIN_NV04(push, NV50_2D(DST_LAYER), 1);
      } else {
      const unsigned z = info->dst.box.z + i;
   const uint64_t address = dst->base.address +
      dst->level[info->dst.level].offset +
      BEGIN_NV04(push, NV50_2D(DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address);
      }
   if (src->layout_3d) {
      /* not possible because of depth tiling */
      } else {
      const unsigned z = info->src.box.z + i;
   const uint64_t address = src->base.address +
      src->level[info->src.level].offset +
      BEGIN_NV04(push, NV50_2D(SRC_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address);
      }
   BEGIN_NV04(push, NV50_2D(BLIT_SRC_Y_INT), 1); /* trigger */
      } else {
      BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, srcx);
   PUSH_DATA (push, srcx >> 32);
   PUSH_DATA (push, srcy);
         }
                     if (info->scissor_enable) {
      BEGIN_NV04(push, NV50_2D(CLIP_ENABLE), 1);
      }
   if (mask != 0xffffffff) {
      BEGIN_NV04(push, NV50_2D(OPERATION), 1);
      }
   if (nv50->cond_query && info->render_condition_enable) {
      BEGIN_NV04(push, NV50_2D(COND_MODE), 1);
         }
      static void
   nv50_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
   {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
            if (info->src.box.width == 0 || info->src.box.height == 0 ||
      info->dst.box.width == 0 || info->dst.box.height == 0) {
   util_debug_message(&nv50->base.debug, ERROR,
                     if (util_format_is_depth_or_stencil(info->dst.resource->format)) {
      if (!(info->mask & PIPE_MASK_ZS))
         if (info->dst.resource->format == PIPE_FORMAT_Z32_FLOAT ||
      info->dst.resource->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
      if (info->filter != PIPE_TEX_FILTER_NEAREST)
      } else {
      if (!(info->mask & PIPE_MASK_RGBA))
         if (info->mask != PIPE_MASK_RGBA)
               if (nv50_miptree(info->src.resource)->layout_3d) {
         } else
   if (info->src.box.depth != info->dst.box.depth) {
      eng3d = true;
               if (!eng3d && info->dst.format != info->src.format) {
      if (!nv50_2d_dst_format_faithful(info->dst.format) ||
      !nv50_2d_src_format_faithful(info->src.format)) {
      } else
   if (!nv50_2d_src_format_faithful(info->src.format)) {
      if (!util_format_is_luminance(info->src.format)) {
      if (util_format_is_intensity(info->src.format))
         else
   if (!nv50_2d_dst_format_ops_supported(info->dst.format))
         else
         } else
   if (util_format_is_luminance_alpha(info->src.format))
               if (info->src.resource->nr_samples == 8 &&
      info->dst.resource->nr_samples <= 1)
         if (info->num_window_rectangles > 0 || info->window_rectangle_include)
            /* FIXME: can't make this work with eng2d anymore */
   if ((info->src.resource->nr_samples | 1) !=
      (info->dst.resource->nr_samples | 1))
         /* FIXME: find correct src coordinate adjustments */
   if ((info->src.box.width !=  info->dst.box.width &&
      info->src.box.width != -info->dst.box.width) ||
   (info->src.box.height !=  info->dst.box.height &&
   info->src.box.height != -info->dst.box.height))
         simple_mtx_lock(&nv50->screen->state_lock);
   if (nv50->screen->num_occlusion_queries_active) {
      BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
               if (!eng3d)
         else
            if (nv50->screen->num_occlusion_queries_active) {
      BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
      }
   PUSH_KICK(push);
      }
      static void
   nv50_flush_resource(struct pipe_context *ctx,
         {
   }
      bool
   nv50_blitter_create(struct nv50_screen *screen)
   {
      screen->blitter = CALLOC_STRUCT(nv50_blitter);
   if (!screen->blitter) {
      NOUVEAU_ERR("failed to allocate blitter struct\n");
                        nv50_blitter_make_vp(screen->blitter);
               }
      void
   nv50_blitter_destroy(struct nv50_screen *screen)
   {
      struct nv50_blitter *blitter = screen->blitter;
            for (i = 0; i < NV50_BLIT_MAX_TEXTURE_TYPES; ++i) {
      for (m = 0; m < NV50_BLIT_MODES; ++m) {
      struct nv50_program *prog = blitter->fp[i][m];
   if (prog) {
      nv50_program_destroy(NULL, prog);
   ralloc_free((void *)prog->nir);
                     mtx_destroy(&blitter->mutex);
      }
      bool
   nv50_blitctx_create(struct nv50_context *nv50)
   {
      nv50->blit = CALLOC_STRUCT(nv50_blitctx);
   if (!nv50->blit) {
      NOUVEAU_ERR("failed to allocate blit context\n");
                                    }
      void
   nv50_init_surface_functions(struct nv50_context *nv50)
   {
               pipe->resource_copy_region = nv50_resource_copy_region;
   pipe->blit = nv50_blit;
   pipe->flush_resource = nv50_flush_resource;
   pipe->clear_texture = u_default_clear_texture;
   pipe->clear_render_target = nv50_clear_render_target;
   pipe->clear_depth_stencil = nv50_clear_depth_stencil;
      }
