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
   #include "util/u_surface.h"
   #include "util/u_thread.h"
      #include "nv50_ir_driver.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_resource.h"
      #include "nv50/g80_defs.xml.h"
   #include "nv50/g80_texture.xml.h"
      /* these are used in nv50_blit.h */
   #define NV50_ENG2D_SUPPORTED_FORMATS 0xff9ccfe1cce3ccc9ULL
   #define NV50_ENG2D_NOCONVERT_FORMATS 0x009cc02000000000ULL
   #define NV50_ENG2D_LUMINANCE_FORMATS 0x001cc02000000000ULL
   #define NV50_ENG2D_INTENSITY_FORMATS 0x0080000000000000ULL
   #define NV50_ENG2D_OPERATION_FORMATS 0x060001c000638000ULL
      #define NOUVEAU_DRIVER 0xc0
   #include "nv50/nv50_blit.h"
      static inline uint8_t
   nvc0_2d_format(enum pipe_format format, bool dst, bool dst_src_equal)
   {
               /* A8_UNORM is treated as I8_UNORM as far as the 2D engine is concerned. */
   if (!dst && unlikely(format == PIPE_FORMAT_I8_UNORM) && !dst_src_equal)
            /* Hardware values for color formats range from 0xc0 to 0xff,
   * but the 2D engine doesn't support all of them.
   */
   if (nv50_2d_format_supported(format))
                  switch (util_format_get_blocksize(format)) {
   case 1:
         case 2:
         case 4:
         case 8:
         case 16:
         default:
      assert(0);
         }
      static int
   nvc0_2d_texture_set(struct nouveau_pushbuf *push, bool dst,
               {
      struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
            format = nvc0_2d_format(pformat, dst, dst_src_pformat_equal);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                     width = u_minify(mt->base.base.width0, level) << mt->ms_x;
   height = u_minify(mt->base.base.height0, level) << mt->ms_y;
                     if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
   layer = 0;
      } else
   if (!dst) {
      offset += nvc0_mt_zslice_offset(mt, level, layer);
               if (!nouveau_bo_memtype(bo)) {
      BEGIN_NVC0(push, SUBC_2D(mthd), 2);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, SUBC_2D(mthd + 0x14), 5);
   PUSH_DATA (push, mt->level[level].pitch);
   PUSH_DATA (push, width);
   PUSH_DATA (push, height);
   PUSH_DATAh(push, bo->offset + offset);
      } else {
      BEGIN_NVC0(push, SUBC_2D(mthd), 5);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, mt->level[level].tile_mode);
   PUSH_DATA (push, depth);
   PUSH_DATA (push, layer);
   BEGIN_NVC0(push, SUBC_2D(mthd + 0x18), 4);
   PUSH_DATA (push, width);
   PUSH_DATA (push, height);
   PUSH_DATAh(push, bo->offset + offset);
               if (dst) {
      IMMED_NVC0(push, SUBC_2D(NVC0_2D_SET_DST_COLOR_RENDER_TO_ZETA_SURFACE),
            #if 0
      if (dst) {
      BEGIN_NVC0(push, SUBC_2D(NVC0_2D_CLIP_X), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, width);
         #endif
         }
      static int
   nvc0_2d_texture_do_copy(struct nouveau_pushbuf *push,
                           struct nv50_miptree *dst, unsigned dst_level,
   {
      const enum pipe_format dfmt = dst->base.base.format;
   const enum pipe_format sfmt = src->base.base.format;
   int ret;
            if (!PUSH_SPACE(push, 2 * 16 + 32))
            ret = nvc0_2d_texture_set(push, true, dst, dst_level, dz, dfmt, eqfmt);
   if (ret)
            ret = nvc0_2d_texture_set(push, false, src, src_level, sz, sfmt, eqfmt);
   if (ret)
            IMMED_NVC0(push, NVC0_2D(BLIT_CONTROL), 0x00);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dx << dst->ms_x);
   PUSH_DATA (push, dy << dst->ms_y);
   PUSH_DATA (push, w << dst->ms_x);
   PUSH_DATA (push, h << dst->ms_y);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sx << src->ms_x);
   PUSH_DATA (push, 0);
               }
      static void
   nvc0_resource_copy_region(struct pipe_context *pipe,
                           {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   int ret;
   bool m2mf;
            if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      nouveau_copy_buffer(&nvc0->base,
               NOUVEAU_DRV_STAT(&nvc0->screen->base, buf_copy_bytes, src_box->width);
      }
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
               assert(nv50_2d_dst_format_faithful(dst->format));
            BCTX_REFN(nvc0->bufctx, 2D, nv04_resource(src), RD);
   BCTX_REFN(nvc0->bufctx, 2D, nv04_resource(dst), WR);
   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx);
            for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nvc0_2d_texture_do_copy(nvc0->base.pushbuf,
                                 if (ret)
      }
      }
      static void
   nvc0_clear_render_target(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv50_surface *sf = nv50_surface(dst);
   struct nv04_resource *res = nv04_resource(sf->base.texture);
                     if (!PUSH_SPACE(push, 32 + sf->depth))
                     BEGIN_NVC0(push, NVC0_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
            BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, ( width << 16) | dstx);
            BEGIN_NVC0(push, NVC0_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(0)), 9);
   PUSH_DATAh(push, res->address + sf->offset);
   PUSH_DATA (push, res->address + sf->offset);
   if (likely(nouveau_bo_memtype(res->bo))) {
               PUSH_DATA(push, sf->width);
   PUSH_DATA(push, sf->height);
   PUSH_DATA(push, nvc0_format_table[dst->format].rt);
   PUSH_DATA(push, (mt->layout_3d << 16) |
         PUSH_DATA(push, dst->u.tex.first_layer + sf->depth);
   PUSH_DATA(push, mt->layer_stride >> 2);
   PUSH_DATA(push, dst->u.tex.first_layer);
      } else {
      if (res->base.target == PIPE_BUFFER) {
      PUSH_DATA(push, 262144);
      } else {
      PUSH_DATA(push, nv50_miptree(&res->base)->level[0].pitch);
      }
   PUSH_DATA(push, nvc0_format_table[sf->base.format].rt);
   PUSH_DATA(push, 1 << 12);
   PUSH_DATA(push, 1);
   PUSH_DATA(push, 0);
            IMMED_NVC0(push, NVC0_3D(ZETA_ENABLE), 0);
            /* tiled textures don't have to be fenced, they're not mapped directly */
               if (!render_condition_enabled)
            BEGIN_NIC0(push, NVC0_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, 0x3c |
               if (!render_condition_enabled)
               }
      static void
   nvc0_clear_buffer_push_nvc0(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(res);
            nouveau_bufctx_refn(nvc0->bufctx, 0, buf->bo, buf->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
            unsigned count = (size + 3) / 4;
            while (count) {
      unsigned nr_data = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN) / data_words;
            if (!PUSH_SPACE(push, nr + 9))
            BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
   PUSH_DATAh(push, buf->address + offset);
   PUSH_DATA (push, buf->address + offset);
   BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
   PUSH_DATA (push, MIN2(size, nr * 4));
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
            /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
   BEGIN_NIC0(push, NVC0_M2MF(DATA), nr);
   for (i = 0; i < nr_data; i++)
            count -= nr;
   offset += nr * 4;
                           }
      static void
   nvc0_clear_buffer_push_nve4(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(res);
            nouveau_bufctx_refn(nvc0->bufctx, 0, buf->bo, buf->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
            unsigned count = (size + 3) / 4;
            while (count) {
      unsigned nr_data = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN) / data_words;
            if (!PUSH_SPACE(push, nr + 10))
            BEGIN_NVC0(push, NVE4_P2MF(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, buf->address + offset);
   PUSH_DATA (push, buf->address + offset);
   BEGIN_NVC0(push, NVE4_P2MF(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, MIN2(size, nr * 4));
   PUSH_DATA (push, 1);
   /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
   BEGIN_1IC0(push, NVE4_P2MF(UPLOAD_EXEC), nr + 1);
   PUSH_DATA (push, 0x1001);
   for (i = 0; i < nr_data; i++)
            count -= nr;
   offset += nr * 4;
                           }
      static void
   nvc0_clear_buffer_push(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            if (data_size == 1) {
      tmp = *(unsigned char *)data;
   tmp = (tmp << 24) | (tmp << 16) | (tmp << 8) | tmp;
   data = &tmp;
      } else if (data_size == 2) {
      tmp = *(unsigned short *)data;
   tmp = (tmp << 16) | tmp;
   data = &tmp;
               if (nvc0->screen->base.class_3d < NVE4_3D_CLASS)
         else
      }
      static void
   nvc0_clear_buffer(struct pipe_context *pipe,
                     {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(res);
   union pipe_color_union color;
   enum pipe_format dst_fmt;
            assert(res->target == PIPE_BUFFER);
            switch (data_size) {
   case 16:
      dst_fmt = PIPE_FORMAT_R32G32B32A32_UINT;
   memcpy(&color.ui, data, 16);
      case 12:
      /* RGB32 is not a valid RT format. This will be handled by the pushbuf
   * uploader.
   */
   dst_fmt = PIPE_FORMAT_NONE; /* Init dst_fmt to silence gcc warning */
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
                                 if (data_size == 12) {
      nvc0_clear_buffer_push(pipe, res, offset, size, data, data_size);
               if (offset & 0xff) {
      unsigned fixup_size = MIN2(size, align(offset, 0x100) - offset);
   assert(fixup_size % data_size == 0);
   nvc0_clear_buffer_push(pipe, res, offset, fixup_size, data, data_size);
   offset += fixup_size;
   size -= fixup_size;
   if (!size)
               elements = size / data_size;
   height = (elements + 16383) / 16384;
   width = elements / height;
   if (height > 1)
                  if (!PUSH_SPACE(push, 40))
                     BEGIN_NVC0(push, NVC0_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATA (push, color.ui[0]);
   PUSH_DATA (push, color.ui[1]);
   PUSH_DATA (push, color.ui[2]);
   PUSH_DATA (push, color.ui[3]);
   BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, width << 16);
                     BEGIN_NVC0(push, NVC0_3D(RT_ADDRESS_HIGH(0)), 9);
   PUSH_DATAh(push, buf->address + offset);
   PUSH_DATA (push, buf->address + offset);
   PUSH_DATA (push, align(width * data_size, 0x100));
   PUSH_DATA (push, height);
   PUSH_DATA (push, nvc0_format_table[dst_fmt].rt);
   PUSH_DATA (push, NVC0_3D_RT_TILE_MODE_LINEAR);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
            IMMED_NVC0(push, NVC0_3D(ZETA_ENABLE), 0);
                                                if (width * height != elements) {
      offset += width * height * data_size;
   width = elements - width * height;
   nvc0_clear_buffer_push(pipe, res, offset, width * data_size,
                  }
      static void
   nvc0_clear_depth_stencil(struct pipe_context *pipe,
                           struct pipe_surface *dst,
   unsigned clear_flags,
   double depth,
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   uint32_t mode = 0;
   int unk = mt->base.base.target == PIPE_TEXTURE_2D;
                     if (!PUSH_SPACE(push, 32 + sf->depth))
                     if (clear_flags & PIPE_CLEAR_DEPTH) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_DEPTH), 1);
   PUSH_DATAf(push, depth);
               if (clear_flags & PIPE_CLEAR_STENCIL) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_STENCIL), 1);
   PUSH_DATA (push, stencil & 0xff);
               BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, ( width << 16) | dstx);
            BEGIN_NVC0(push, NVC0_3D(ZETA_ADDRESS_HIGH), 5);
   PUSH_DATAh(push, mt->base.address + sf->offset);
   PUSH_DATA (push, mt->base.address + sf->offset);
   PUSH_DATA (push, nvc0_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, mt->layer_stride >> 2);
   BEGIN_NVC0(push, NVC0_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_3D(ZETA_HORIZ), 3);
   PUSH_DATA (push, sf->width);
   PUSH_DATA (push, sf->height);
   PUSH_DATA (push, (unk << 16) | (dst->u.tex.first_layer + sf->depth));
   BEGIN_NVC0(push, NVC0_3D(ZETA_BASE_LAYER), 1);
   PUSH_DATA (push, dst->u.tex.first_layer);
            if (!render_condition_enabled)
            BEGIN_NIC0(push, NVC0_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, mode |
               if (!render_condition_enabled)
               }
      void
   nvc0_clear(struct pipe_context *pipe, unsigned buffers,
            const struct pipe_scissor_state *scissor_state,
      {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nvc0->framebuffer;
   unsigned i, j, k;
                     /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   if (!nvc0_state_validate_3d(nvc0, NVC0_NEW_3D_FRAMEBUFFER))
            if (scissor_state) {
      uint32_t minx = scissor_state->minx;
   uint32_t maxx = MIN2(fb->width, scissor_state->maxx);
   uint32_t miny = scissor_state->miny;
   uint32_t maxy = MIN2(fb->height, scissor_state->maxy);
   if (maxx <= minx || maxy <= miny)
            BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, minx | (maxx - minx) << 16);
               if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
   PUSH_DATAf(push, color->f[3]);
   if (buffers & PIPE_CLEAR_COLOR0)
      mode =
                  if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_DEPTH), 1);
   PUSH_DATA (push, fui(depth));
               if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_STENCIL), 1);
   PUSH_DATA (push, stencil & 0xff);
               if (mode) {
      int zs_layers = 0, color0_layers = 0;
   if (fb->cbufs[0] && (mode & 0x3c))
      color0_layers = fb->cbufs[0]->u.tex.last_layer -
      if (fb->zsbuf && (mode & ~0x3c))
                  for (j = 0; j < MIN2(zs_layers, color0_layers); j++) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
      }
   for (k = j; k < zs_layers; k++) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
      }
   for (k = j; k < color0_layers; k++) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
                  for (i = 1; i < fb->nr_cbufs; i++) {
      struct pipe_surface *sf = fb->cbufs[i];
   if (!sf || !(buffers & (PIPE_CLEAR_COLOR0 << i)))
         for (j = 0; j <= sf->u.tex.last_layer - sf->u.tex.first_layer; j++) {
      BEGIN_NVC0(push, NVC0_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, (i << 6) | 0x3c |
                  /* restore screen scissor */
   if (scissor_state) {
      BEGIN_NVC0(push, NVC0_3D(SCREEN_SCISSOR_HORIZ), 2);
   PUSH_DATA (push, fb->width << 16);
            out:
      PUSH_KICK(push);
      }
      static void
   gm200_evaluate_depth_buffer(struct pipe_context *pipe)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
            simple_mtx_lock(&nvc0->screen->state_lock);
   nvc0_state_validate_3d(nvc0, NVC0_NEW_3D_FRAMEBUFFER);
   IMMED_NVC0(push, SUBC_3D(0x11fc), 1);
   PUSH_KICK(push);
      }
         /* =============================== BLIT CODE ===================================
   */
      struct nvc0_blitter
   {
      struct nvc0_program *fp[NV50_BLIT_MAX_TEXTURE_TYPES][NV50_BLIT_MODES];
                                 };
      struct nvc0_blitctx
   {
      struct nvc0_context *nvc0;
   struct nvc0_program *fp;
   struct nvc0_program *vp;
   uint8_t mode;
   uint16_t color_mask;
   uint8_t filter;
   uint8_t render_condition_enable;
   enum pipe_texture_target target;
   struct {
      struct pipe_framebuffer_state fb;
   struct nvc0_window_rect_stateobj window_rect;
   struct nvc0_rasterizer_stateobj *rast;
   struct nvc0_program *vp;
   struct nvc0_program *tcp;
   struct nvc0_program *tep;
   struct nvc0_program *gp;
   struct nvc0_program *fp;
   unsigned num_textures[5];
   unsigned num_samplers[5];
   struct pipe_sampler_view *texture[2];
   struct nv50_tsc_entry *sampler[2];
   unsigned min_samples;
      } saved;
      };
      static void *
   nvc0_blitter_make_vp(struct pipe_context *pipe)
   {
      const nir_shader_compiler_options *options =
      nv50_ir_nir_shader_compiler_options(nouveau_screen(pipe->screen)->device->chipset,
         struct nir_builder b =
      nir_builder_init_simple_shader(MESA_SHADER_VERTEX, options,
         const struct glsl_type* float2 = glsl_vector_type(GLSL_TYPE_FLOAT, 2);
            nir_variable *ipos =
         ipos->data.location = VERT_ATTRIB_GENERIC0;
            nir_variable *opos =
         opos->data.location = VARYING_SLOT_POS;
            nir_variable *itex =
         itex->data.location = VERT_ATTRIB_GENERIC1;
            nir_variable *otex =
         otex->data.location = VARYING_SLOT_VAR0;
            nir_copy_var(&b, opos, ipos);
                     struct pipe_shader_state state;
   pipe_shader_state_from_nir(&state, b.shader);
      }
      static void
   nvc0_blitter_make_sampler(struct nvc0_blitter *blit)
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
      static void
   nvc0_blit_select_vp(struct nvc0_blitctx *ctx)
   {
               if (!blitter->vp) {
      mtx_lock(&blitter->mutex);
   if (!blitter->vp)
            }
      }
      static void
   nvc0_blit_select_fp(struct nvc0_blitctx *ctx, const struct pipe_blit_info *info)
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
   nvc0_blit_set_dst(struct nvc0_blitctx *ctx,
               {
      struct nvc0_context *nvc0 = ctx->nvc0;
   struct pipe_context *pipe = &nvc0->base.pipe;
            if (util_format_is_depth_or_stencil(format))
         else
            templ.u.tex.level = level;
            if (layer == -1) {
      templ.u.tex.first_layer = 0;
   templ.u.tex.last_layer =
               nvc0->framebuffer.cbufs[0] = nvc0_miptree_surface_new(pipe, res, &templ);
   nvc0->framebuffer.nr_cbufs = 1;
   nvc0->framebuffer.zsbuf = NULL;
   nvc0->framebuffer.width = nvc0->framebuffer.cbufs[0]->width;
      }
      static void
   nvc0_blit_set_src(struct nvc0_blitctx *ctx,
               {
      struct nvc0_context *nvc0 = ctx->nvc0;
   struct pipe_context *pipe = &nvc0->base.pipe;
   struct pipe_sampler_view templ = {0};
   uint32_t flags;
   unsigned s;
                     templ.target = target;
   templ.format = format;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;
   templ.u.tex.first_level = templ.u.tex.last_level = level;
   templ.swizzle_r = PIPE_SWIZZLE_X;
   templ.swizzle_g = PIPE_SWIZZLE_Y;
   templ.swizzle_b = PIPE_SWIZZLE_Z;
            if (layer == -1) {
      templ.u.tex.first_layer = 0;
   templ.u.tex.last_layer =
               flags = res->last_level ? 0 : NV50_TEXVIEW_SCALED_COORDS;
   flags |= NV50_TEXVIEW_ACCESS_RESOLVE;
   if (filter && res->nr_samples == 8)
            nvc0->textures[4][0] = nvc0_create_texture_view(
                  for (s = 0; s <= 3; ++s)
                  templ.format = nv50_zs_to_s_format(format);
   if (templ.format != format) {
      nvc0->textures[4][1] = nvc0_create_texture_view(
               }
      static void
   nvc0_blitctx_prepare_state(struct nvc0_blitctx *blit)
   {
                        if (blit->nvc0->cond_query && !blit->render_condition_enable)
            /* blend state */
   BEGIN_NVC0(push, NVC0_3D(COLOR_MASK(0)), 1);
   PUSH_DATA (push, blit->color_mask);
   IMMED_NVC0(push, NVC0_3D(BLEND_ENABLE(0)), 0);
            /* rasterizer state */
   IMMED_NVC0(push, NVC0_3D(FRAG_COLOR_CLAMP_EN), 0);
   IMMED_NVC0(push, NVC0_3D(MULTISAMPLE_ENABLE), 0);
   BEGIN_NVC0(push, NVC0_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   BEGIN_NVC0(push, NVC0_3D(MACRO_POLYGON_MODE_FRONT), 1);
   PUSH_DATA (push, NVC0_3D_MACRO_POLYGON_MODE_FRONT_FILL);
   BEGIN_NVC0(push, NVC0_3D(MACRO_POLYGON_MODE_BACK), 1);
   PUSH_DATA (push, NVC0_3D_MACRO_POLYGON_MODE_BACK_FILL);
   IMMED_NVC0(push, NVC0_3D(POLYGON_SMOOTH_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(POLYGON_OFFSET_FILL_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(POLYGON_STIPPLE_ENABLE), 0);
            /* zsa state */
   IMMED_NVC0(push, NVC0_3D(DEPTH_TEST_ENABLE), 0);
   IMMED_NVC0(push, NVC0_3D(DEPTH_BOUNDS_EN), 0);
   IMMED_NVC0(push, NVC0_3D(STENCIL_ENABLE), 0);
            /* disable transform feedback */
      }
      static void
   nvc0_blitctx_pre_blit(struct nvc0_blitctx *ctx,
         {
      struct nvc0_context *nvc0 = ctx->nvc0;
   struct nvc0_blitter *blitter = nvc0->screen->blitter;
            ctx->saved.fb.width = nvc0->framebuffer.width;
   ctx->saved.fb.height = nvc0->framebuffer.height;
   ctx->saved.fb.samples = nvc0->framebuffer.samples;
   ctx->saved.fb.layers = nvc0->framebuffer.layers;
   ctx->saved.fb.nr_cbufs = nvc0->framebuffer.nr_cbufs;
   ctx->saved.fb.cbufs[0] = nvc0->framebuffer.cbufs[0];
                     ctx->saved.vp = nvc0->vertprog;
   ctx->saved.tcp = nvc0->tctlprog;
   ctx->saved.tep = nvc0->tevlprog;
   ctx->saved.gp = nvc0->gmtyprog;
            ctx->saved.min_samples = nvc0->min_samples;
                     nvc0->vertprog = ctx->vp;
   nvc0->tctlprog = NULL;
   nvc0->tevlprog = NULL;
   nvc0->gmtyprog = NULL;
            nvc0->window_rect.rects =
         nvc0->window_rect.inclusive = info->window_rectangle_include;
   if (nvc0->window_rect.rects)
      memcpy(nvc0->window_rect.rect, info->window_rectangles,
         for (s = 0; s <= 4; ++s) {
      ctx->saved.num_textures[s] = nvc0->num_textures[s];
   ctx->saved.num_samplers[s] = nvc0->num_samplers[s];
   nvc0->textures_dirty[s] = (1 << nvc0->num_textures[s]) - 1;
      }
   ctx->saved.texture[0] = nvc0->textures[4][0];
   ctx->saved.texture[1] = nvc0->textures[4][1];
   ctx->saved.sampler[0] = nvc0->samplers[4][0];
            nvc0->samplers[4][0] = &blitter->sampler[ctx->filter];
            for (s = 0; s <= 3; ++s)
                                    nvc0->textures_dirty[4] |= 3;
            nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_FB);
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TEX(4, 0));
            nvc0->dirty_3d = NVC0_NEW_3D_FRAMEBUFFER | NVC0_NEW_3D_MIN_SAMPLES |
      NVC0_NEW_3D_VERTPROG | NVC0_NEW_3D_FRAGPROG |
   NVC0_NEW_3D_TCTLPROG | NVC0_NEW_3D_TEVLPROG | NVC0_NEW_3D_GMTYPROG |
   }
      static void
   nvc0_blitctx_post_blit(struct nvc0_blitctx *blit)
   {
      struct nvc0_context *nvc0 = blit->nvc0;
                     nvc0->framebuffer.width = blit->saved.fb.width;
   nvc0->framebuffer.height = blit->saved.fb.height;
   nvc0->framebuffer.samples = blit->saved.fb.samples;
   nvc0->framebuffer.layers = blit->saved.fb.layers;
   nvc0->framebuffer.nr_cbufs = blit->saved.fb.nr_cbufs;
   nvc0->framebuffer.cbufs[0] = blit->saved.fb.cbufs[0];
                     nvc0->vertprog = blit->saved.vp;
   nvc0->tctlprog = blit->saved.tcp;
   nvc0->tevlprog = blit->saved.tep;
   nvc0->gmtyprog = blit->saved.gp;
            nvc0->min_samples = blit->saved.min_samples;
            pipe_sampler_view_reference(&nvc0->textures[4][0], NULL);
            for (s = 0; s <= 4; ++s) {
      nvc0->num_textures[s] = blit->saved.num_textures[s];
   nvc0->num_samplers[s] = blit->saved.num_samplers[s];
   nvc0->textures_dirty[s] = (1 << nvc0->num_textures[s]) - 1;
      }
   nvc0->textures[4][0] = blit->saved.texture[0];
   nvc0->textures[4][1] = blit->saved.texture[1];
   nvc0->samplers[4][0] = blit->saved.sampler[0];
            nvc0->textures_dirty[4] |= 3;
            if (nvc0->cond_query && !blit->render_condition_enable)
      nvc0->base.pipe.render_condition(&nvc0->base.pipe, nvc0->cond_query,
         nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_VTX_TMP);
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TEXT);
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_FB);
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TEX(4, 0));
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TEX(4, 1));
            nvc0->dirty_3d = blit->saved.dirty_3d |
      (NVC0_NEW_3D_FRAMEBUFFER | NVC0_NEW_3D_SCISSOR | NVC0_NEW_3D_SAMPLE_MASK |
   NVC0_NEW_3D_RASTERIZER | NVC0_NEW_3D_ZSA | NVC0_NEW_3D_BLEND |
   NVC0_NEW_3D_VIEWPORT | NVC0_NEW_3D_WINDOW_RECTS |
   NVC0_NEW_3D_TEXTURES | NVC0_NEW_3D_SAMPLERS |
   NVC0_NEW_3D_VERTPROG | NVC0_NEW_3D_FRAGPROG |
   NVC0_NEW_3D_TCTLPROG | NVC0_NEW_3D_TEVLPROG | NVC0_NEW_3D_GMTYPROG |
      nvc0->scissors_dirty |= 1;
               }
      static void
   nvc0_blit_3d(struct nvc0_context *nvc0, const struct pipe_blit_info *info)
   {
      struct nvc0_screen *screen = nvc0->screen;
   struct nvc0_blitctx *blit = nvc0->blit;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_resource *src = info->src.resource;
   struct pipe_resource *dst = info->dst.resource;
   struct nouveau_bo *vtxbuf_bo;
   uint32_t stride, length, *vbuf;
   uint64_t vtxbuf;
   int32_t minx, maxx, miny, maxy;
   int32_t i, n;
   float x0, x1, y0, y1, z;
   float dz;
            blit->mode = nv50_blit_select_mode(info);
   blit->color_mask = nv50_blit_derive_color_mask(info);
   blit->filter = nv50_blit_get_filter(info);
            nvc0_blit_select_vp(blit);
   nvc0_blit_select_fp(blit, info);
            nvc0_blit_set_dst(blit, dst, info->dst.level, -1, info->dst.format);
   nvc0_blit_set_src(blit, src, info->src.level, -1, info->src.format,
                              x_range = (float)info->src.box.width / (float)info->dst.box.width;
            x0 = (float)info->src.box.x - x_range * (float)info->dst.box.x;
            x1 = x0 + 32768.0f * x_range;
            x0 *= (float)(1 << nv50_miptree(src)->ms_x);
   x1 *= (float)(1 << nv50_miptree(src)->ms_x);
   y0 *= (float)(1 << nv50_miptree(src)->ms_y);
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
                  bool serialize = false;
   struct nv50_miptree *mt = nv50_miptree(dst);
   if (screen->eng3d->oclass >= TU102_3D_CLASS) {
      IMMED_NVC0(push, SUBC_3D(TU102_3D_SET_COLOR_RENDER_TO_ZETA_SURFACE),
      } else {
      /* When flipping a surface from zeta <-> color "mode", we have to wait for
   * the GPU to flush its current draws.
   */
   serialize = util_format_is_depth_or_stencil(info->dst.format);
   if (serialize && mt->base.status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
                     IMMED_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 0);
   IMMED_NVC0(push, NVC0_3D(VIEW_VOLUME_CLIP_CTRL), 0x2 |
         BEGIN_NVC0(push, NVC0_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, nvc0->framebuffer.width << 16);
            /* Draw a large triangle in screen coordinates covering the whole
   * render target, with scissors defining the destination region.
   * The vertex is supplied with non-normalized texture coordinates
   * arranged in a way to yield the desired offset and scale.
   *
   * Note that while the source texture is presented to the sampler as
   * non-MSAA (even if it is), the destination texture is treated as MSAA for
   * rendering. This means that
   *  - destination coordinates shouldn't be scaled
   *  - without per-sample rendering, the target will be a solid-fill for all
   *    of the samples
   *
   * The last point implies that this process is very bad for 1:1 blits, as
   * well as scaled blits between MSAA surfaces. This works fine for
   * upscaling and downscaling though. The 1:1 blits should ideally be
   * handled by the 2d engine, which can do it perfectly.
            minx = info->dst.box.x;
   maxx = info->dst.box.x + info->dst.box.width;
   miny = info->dst.box.y;
   maxy = info->dst.box.y + info->dst.box.height;
   if (info->scissor_enable) {
      minx = MAX2(minx, info->scissor.minx);
   maxx = MIN2(maxx, info->scissor.maxx);
   miny = MAX2(miny, info->scissor.miny);
      }
   BEGIN_NVC0(push, NVC0_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (maxx << 16) | minx);
            stride = (3 + 2) * 4;
            vbuf = nouveau_scratch_get(&nvc0->base, length, &vtxbuf, &vtxbuf_bo);
   if (!vbuf) {
      assert(vbuf);
               BCTX_REFN_bo(nvc0->bufctx_3d, 3D_VTX_TMP,
         BCTX_REFN_bo(nvc0->bufctx_3d, 3D_TEXT,
                  BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(0)), 4);
   PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | stride <<
         PUSH_DATAh(push, vtxbuf);
   PUSH_DATA (push, vtxbuf);
   PUSH_DATA (push, 0);
   if (screen->eng3d->oclass < TU102_3D_CLASS)
         else
         PUSH_DATAh(push, vtxbuf + length - 1);
                     BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(0)), n);
   PUSH_DATA (push, NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |
               PUSH_DATA (push, NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |
               for (i = 2; i < n; i++) {
      PUSH_DATA(push, NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |
            }
   for (i = 1; i < n; ++i)
         if (nvc0->state.instance_elts) {
      nvc0->state.instance_elts = 0;
   BEGIN_NVC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_PER_INSTANCE), 2);
   PUSH_DATA (push, n);
      }
            if (nvc0->state.prim_restart) {
      IMMED_NVC0(push, NVC0_3D(PRIM_RESTART_ENABLE), 0);
               if (nvc0->state.index_bias) {
      IMMED_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 0);
   IMMED_NVC0(push, NVC0_3D(VERTEX_ID_BASE), 0);
               for (i = 0; i < info->dst.box.depth; ++i, z += dz) {
      if (info->dst.box.z + i) {
      BEGIN_NVC0(push, NVC0_3D(LAYER), 1);
               *(vbuf++) = fui(0.0f);
   *(vbuf++) = fui(0.0f);
   *(vbuf++) = fui(x0);
   *(vbuf++) = fui(y0);
            *(vbuf++) = fui(32768.0f);
   *(vbuf++) = fui(0.0f);
   *(vbuf++) = fui(x1);
   *(vbuf++) = fui(y0);
            *(vbuf++) = fui(0.0f);
   *(vbuf++) = fui(32768.0f);
   *(vbuf++) = fui(x0);
   *(vbuf++) = fui(y1);
            IMMED_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL),
         BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, i * 3);
   PUSH_DATA (push, 3);
      }
   if (info->dst.box.z + info->dst.box.depth - 1)
                     /* restore viewport transform */
   IMMED_NVC0(push, NVC0_3D(VIEWPORT_TRANSFORM_EN), 1);
   if (screen->eng3d->oclass >= TU102_3D_CLASS)
         else if (serialize)
      /* mark the surface as reading, which will force a serialize next time
   * it's used for writing.
   */
   }
      static void
   nvc0_blit_eng2d(struct nvc0_context *nvc0, const struct pipe_blit_info *info)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv50_miptree *dst = nv50_miptree(info->dst.resource);
   struct nv50_miptree *src = nv50_miptree(info->src.resource);
   const int32_t srcx_adj = info->src.box.width < 0 ? -1 : 0;
   const int32_t srcy_adj = info->src.box.height < 0 ? -1 : 0;
   const int dz = info->dst.box.z;
   const int sz = info->src.box.z;
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
   nvc0_2d_texture_set(push, 1, dst, info->dst.level, dz, info->dst.format, b);
            if (info->scissor_enable) {
      BEGIN_NVC0(push, NVC0_2D(CLIP_X), 5);
   PUSH_DATA (push, info->scissor.minx << dst->ms_x);
   PUSH_DATA (push, info->scissor.miny << dst->ms_y);
   PUSH_DATA (push, (info->scissor.maxx - info->scissor.minx) << dst->ms_x);
   PUSH_DATA (push, (info->scissor.maxy - info->scissor.miny) << dst->ms_y);
               if (nvc0->cond_query && info->render_condition_enable)
            if (mask != 0xffffffff) {
      IMMED_NVC0(push, NVC0_2D(ROP), 0xca); /* DPSDxax */
   IMMED_NVC0(push, NVC0_2D(PATTERN_COLOR_FORMAT),
         BEGIN_NVC0(push, NVC0_2D(PATTERN_BITMAP_COLOR(0)), 4);
   PUSH_DATA (push, 0x00000000);
   PUSH_DATA (push, mask);
   PUSH_DATA (push, 0xffffffff);
   PUSH_DATA (push, 0xffffffff);
      } else
   if (info->src.format != info->dst.format) {
      if (info->src.format == PIPE_FORMAT_R8_UNORM ||
      info->src.format == PIPE_FORMAT_R8_SNORM ||
   info->src.format == PIPE_FORMAT_R16_UNORM ||
   info->src.format == PIPE_FORMAT_R16_SNORM ||
   info->src.format == PIPE_FORMAT_R16_FLOAT ||
   info->src.format == PIPE_FORMAT_R32_FLOAT) {
   mask = 0xffff0000; /* also makes condition for OPERATION reset true */
   BEGIN_NVC0(push, NVC0_2D(BETA4), 2);
   PUSH_DATA (push, mask);
      } else
   if (info->src.format == PIPE_FORMAT_A8_UNORM) {
      mask = 0xff000000;
   BEGIN_NVC0(push, NVC0_2D(BETA4), 2);
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
               IMMED_NVC0(push, NVC0_2D(BLIT_CONTROL), mode);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dstx);
   PUSH_DATA (push, dsty);
   PUSH_DATA (push, dstw);
   PUSH_DATA (push, dsth);
   BEGIN_NVC0(push, NVC0_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, du_dx);
   PUSH_DATA (push, du_dx >> 32);
   PUSH_DATA (push, dv_dy);
            BCTX_REFN(nvc0->bufctx, 2D, &dst->base, WR);
   BCTX_REFN(nvc0->bufctx, 2D, &src->base, RD);
   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx);
   if (PUSH_VAL(nvc0->base.pushbuf))
            for (i = 0; i < info->dst.box.depth; ++i) {
      if (i > 0) {
      /* no scaling in z-direction possible for eng2d blits */
   if (dst->layout_3d) {
      BEGIN_NVC0(push, NVC0_2D(DST_LAYER), 1);
      } else {
      const unsigned z = info->dst.box.z + i;
   const uint64_t address = dst->base.address +
      dst->level[info->dst.level].offset +
      BEGIN_NVC0(push, NVC0_2D(DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address);
      }
   if (src->layout_3d) {
      /* not possible because of depth tiling */
      } else {
      const unsigned z = info->src.box.z + i;
   const uint64_t address = src->base.address +
      src->level[info->src.level].offset +
      BEGIN_NVC0(push, NVC0_2D(SRC_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address);
      }
   BEGIN_NVC0(push, NVC0_2D(BLIT_SRC_Y_INT), 1); /* trigger */
      } else {
      BEGIN_NVC0(push, NVC0_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, srcx);
   PUSH_DATA (push, srcx >> 32);
   PUSH_DATA (push, srcy);
         }
   nvc0_resource_validate(nvc0, &dst->base, NOUVEAU_BO_WR);
                     if (info->scissor_enable)
         if (mask != 0xffffffff)
         if (nvc0->cond_query && info->render_condition_enable)
      }
      static void
   nvc0_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (info->src.box.width == 0 || info->src.box.height == 0 ||
      info->dst.box.width == 0 || info->dst.box.height == 0) {
   util_debug_message(&nvc0->base.debug, ERROR,
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
      if (!nv50_2d_dst_format_faithful(info->dst.format)) {
         } else
   if (!nv50_2d_src_format_faithful(info->src.format)) {
      if (!util_format_is_luminance(info->src.format)) {
      if (!nv50_2d_dst_format_ops_supported(info->dst.format))
         else
   if (util_format_is_intensity(info->src.format))
         else
   if (util_format_is_alpha(info->src.format))
         else
   if (util_format_is_srgb(info->dst.format) &&
      util_format_get_nr_components(info->src.format) == 1)
      else
         } else
   if (util_format_is_luminance_alpha(info->src.format))
               if (info->src.resource->nr_samples == 8 &&
      info->dst.resource->nr_samples <= 1)
   #if 0
      /* FIXME: can't make this work with eng2d anymore, at least not on nv50 */
   if (info->src.resource->nr_samples > 1 ||
      info->dst.resource->nr_samples > 1)
   #endif
      /* FIXME: find correct src coordinates adjustments */
   if ((info->src.box.width !=  info->dst.box.width &&
      info->src.box.width != -info->dst.box.width) ||
   (info->src.box.height !=  info->dst.box.height &&
   info->src.box.height != -info->dst.box.height))
         if (info->num_window_rectangles > 0 || info->window_rectangle_include)
            simple_mtx_lock(&nvc0->screen->state_lock);
   if (nvc0->screen->num_occlusion_queries_active)
            if (!eng3d)
         else
            if (nvc0->screen->num_occlusion_queries_active)
         PUSH_KICK(push);
               }
      static void
   nvc0_flush_resource(struct pipe_context *ctx,
         {
   }
      bool
   nvc0_blitter_create(struct nvc0_screen *screen)
   {
      screen->blitter = CALLOC_STRUCT(nvc0_blitter);
   if (!screen->blitter) {
      NOUVEAU_ERR("failed to allocate blitter struct\n");
      }
                                 }
      void
   nvc0_blitter_destroy(struct nvc0_screen *screen)
   {
      struct nvc0_blitter *blitter = screen->blitter;
            for (i = 0; i < NV50_BLIT_MAX_TEXTURE_TYPES; ++i) {
      for (m = 0; m < NV50_BLIT_MODES; ++m) {
      struct nvc0_program *prog = blitter->fp[i][m];
   if (prog) {
      nvc0_program_destroy(NULL, prog);
   ralloc_free((void *)prog->nir);
            }
   if (blitter->vp) {
      struct nvc0_program *prog = blitter->vp;
   nvc0_program_destroy(NULL, prog);
   ralloc_free((void *)prog->nir);
               mtx_destroy(&blitter->mutex);
      }
      bool
   nvc0_blitctx_create(struct nvc0_context *nvc0)
   {
      nvc0->blit = CALLOC_STRUCT(nvc0_blitctx);
   if (!nvc0->blit) {
      NOUVEAU_ERR("failed to allocate blit context\n");
                                    }
      void
   nvc0_blitctx_destroy(struct nvc0_context *nvc0)
   {
         }
      void
   nvc0_init_surface_functions(struct nvc0_context *nvc0)
   {
               pipe->resource_copy_region = nvc0_resource_copy_region;
   pipe->blit = nvc0_blit;
   pipe->flush_resource = nvc0_flush_resource;
   pipe->clear_render_target = nvc0_clear_render_target;
   pipe->clear_depth_stencil = nvc0_clear_depth_stencil;
   pipe->clear_texture = u_default_clear_texture;
   pipe->clear_buffer = nvc0_clear_buffer;
   if (nvc0->screen->base.class_3d >= GM200_3D_CLASS)
      }
