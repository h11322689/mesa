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
      #define XFER_ARGS                                                              \
      struct nv30_context *nv30, enum nv30_transfer_filter filter,                \
         #include "util/u_math.h"
      #include "nv_object.xml.h"
   #include "nv_m2mf.xml.h"
   #include "nv30/nv01_2d.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
      #include "nv30/nv30_context.h"
   #include "nv30/nv30_transfer.h"
   #include "nv30/nv30_winsys.h"
      /* Various helper functions to transfer different types of data in a number
   * of different ways.
   */
      static inline bool
   nv30_transfer_scaled(struct nv30_rect *src, struct nv30_rect *dst)
   {
      if (src->x1 - src->x0 != dst->x1 - dst->x0)
         if (src->y1 - src->y0 != dst->y1 - dst->y0)
            }
      static inline bool
   nv30_transfer_blit(XFER_ARGS)
   {
      if (nv30->screen->eng3d->oclass < NV40_3D_CLASS)
         if (dst->offset & 63 || dst->pitch & 63 || dst->d > 1)
         if (dst->w < 2 || dst->h < 2)
         if (dst->cpp > 4 || (dst->cpp == 1 && !dst->pitch))
         if (src->cpp > 4)
            }
      static inline struct nouveau_heap *
   nv30_transfer_rect_vertprog(struct nv30_context *nv30)
   {
      struct nouveau_heap *heap = nv30->screen->vp_exec_heap;
            vp = nv30->blit_vp;
   if (!vp) {
      if (nouveau_heap_alloc(heap, 2, &nv30->blit_vp, &nv30->blit_vp)) {
      while (heap->next && heap->size < 2) {
      struct nouveau_heap **evict = heap->next->priv;
               if (nouveau_heap_alloc(heap, 2, &nv30->blit_vp, &nv30->blit_vp))
               vp = nv30->blit_vp;
   if (vp) {
               BEGIN_NV04(push, NV30_3D(VP_UPLOAD_FROM_ID), 1);
   PUSH_DATA (push, vp->start);
   BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
   PUSH_DATA (push, 0x401f9c6c); /* mov o[hpos], a[0]; */
   PUSH_DATA (push, 0x0040000d);
   PUSH_DATA (push, 0x8106c083);
   PUSH_DATA (push, 0x6041ff80);
   BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
   PUSH_DATA (push, 0x401f9c6c); /* mov o[tex0], a[8]; end; */
   PUSH_DATA (push, 0x0040080d);
   PUSH_DATA (push, 0x8106c083);
                     }
         static inline struct nv04_resource *
   nv30_transfer_rect_fragprog(struct nv30_context *nv30)
   {
      struct nv04_resource *fp = nv04_resource(nv30->blit_fp);
            if (!fp) {
      nv30->blit_fp =
         if (nv30->blit_fp) {
      struct pipe_transfer *transfer;
   u32 *map = pipe_buffer_map(pipe, nv30->blit_fp,
         if (map) {
      map[0] = 0x17009e00; /* texr r0, i[tex0], texture[0]; end; */
   map[1] = 0x1c9dc801;
   map[2] = 0x0001c800;
   map[3] = 0x3fe1c800;
   map[4] = 0x01401e81; /* end; */
   map[5] = 0x1c9dc800;
   map[6] = 0x0001c800;
   map[7] = 0x0001c800;
               fp = nv04_resource(nv30->blit_fp);
                     }
      static void
   nv30_transfer_rect_blit(XFER_ARGS)
   {
      struct nv04_resource *fp = nv30_transfer_rect_fragprog(nv30);
   struct nouveau_heap *vp = nv30_transfer_rect_vertprog(nv30);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_pushbuf_refn refs[] = {
      { fp->bo, fp->domain | NOUVEAU_BO_RD },
   { src->bo, src->domain | NOUVEAU_BO_RD },
      };
   u32 texfmt, texswz;
            if (!PUSH_SPACE_EX(push, 512, 8, 0) ||
      PUSH_REFN(push, refs, ARRAY_SIZE(refs)))
         /* various switches depending on cpp of the transfer */
   switch (dst->cpp) {
   case 4:
      format = NV30_3D_RT_FORMAT_COLOR_A8R8G8B8 |
         texfmt = NV40_3D_TEX_FORMAT_FORMAT_A8R8G8B8;
   texswz = 0x0000aae4;
      case 2:
      format = NV30_3D_RT_FORMAT_COLOR_R5G6B5 |
         texfmt = NV40_3D_TEX_FORMAT_FORMAT_R5G6B5;
   texswz = 0x0000a9e4;
      case 1:
      format = NV30_3D_RT_FORMAT_COLOR_B8 |
         texfmt = NV40_3D_TEX_FORMAT_FORMAT_L8;
   texswz = 0x0000aaff;
      default:
      assert(0);
               /* render target */
   if (!dst->pitch) {
      format |= NV30_3D_RT_FORMAT_TYPE_SWIZZLED;
   format |= util_logbase2(dst->w) << 16;
   format |= util_logbase2(dst->h) << 24;
      } else {
      format |= NV30_3D_RT_FORMAT_TYPE_LINEAR;
               BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
   PUSH_DATA (push, dst->w << 16);
   PUSH_DATA (push, dst->h << 16);
   BEGIN_NV04(push, NV30_3D(RT_HORIZ), 5);
   PUSH_DATA (push, dst->w << 16);
   PUSH_DATA (push, dst->h << 16);
   PUSH_DATA (push, format);
   PUSH_DATA (push, stride);
   PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
   BEGIN_NV04(push, NV30_3D(RT_ENABLE), 1);
                     /* viewport state */
   BEGIN_NV04(push, NV30_3D(VIEWPORT_TRANSLATE_X), 8);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   BEGIN_NV04(push, NV30_3D(DEPTH_RANGE_NEAR), 2);
   PUSH_DATAf(push, 0.0);
                     /* blend state */
   BEGIN_NV04(push, NV30_3D(COLOR_LOGIC_OP_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(DITHER_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(BLEND_FUNC_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(COLOR_MASK), 1);
                     /* depth-stencil-alpha state */
   BEGIN_NV04(push, NV30_3D(DEPTH_WRITE_ENABLE), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(0)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(STENCIL_ENABLE(1)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(ALPHA_FUNC_ENABLE), 1);
                     /* rasterizer state */
   BEGIN_NV04(push, NV30_3D(SHADE_MODEL), 1);
   PUSH_DATA (push, NV30_3D_SHADE_MODEL_FLAT);
   BEGIN_NV04(push, NV30_3D(CULL_FACE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(POLYGON_MODE_FRONT), 2);
   PUSH_DATA (push, NV30_3D_POLYGON_MODE_FRONT_FILL);
   PUSH_DATA (push, NV30_3D_POLYGON_MODE_BACK_FILL);
   BEGIN_NV04(push, NV30_3D(POLYGON_OFFSET_FILL_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV30_3D(POLYGON_STIPPLE_ENABLE), 1);
            nv30->state.scissor_off = 0;
            /* vertex program */
   BEGIN_NV04(push, NV30_3D(VP_START_FROM_ID), 1);
   PUSH_DATA (push, vp->start);
   BEGIN_NV04(push, NV40_3D(VP_ATTRIB_EN), 2);
   PUSH_DATA (push, 0x00000101); /* attrib: 0, 8 */
   PUSH_DATA (push, 0x00004000); /* result: hpos, tex0 */
   BEGIN_NV04(push, NV30_3D(ENGINE), 1);
   PUSH_DATA (push, 0x00000103);
   BEGIN_NV04(push, NV30_3D(VP_CLIP_PLANES_ENABLE), 1);
            nv30->dirty |= NV30_NEW_VERTPROG;
            /* fragment program */
   BEGIN_NV04(push, NV30_3D(FP_ACTIVE_PROGRAM), 1);
   PUSH_RELOC(push, fp->bo, fp->offset, fp->domain |
                     BEGIN_NV04(push, NV30_3D(FP_CONTROL), 1);
            nv30->state.fragprog = NULL;
            /* texture */
   texfmt |= 1 << NV40_3D_TEX_FORMAT_MIPMAP_COUNT__SHIFT;
   texfmt |= NV30_3D_TEX_FORMAT_NO_BORDER;
   texfmt |= NV40_3D_TEX_FORMAT_RECT;
   texfmt |= 0x00008000;
   if (src->d < 2)
         else
         if (src->pitch)
            BEGIN_NV04(push, NV30_3D(TEX_OFFSET(0)), 8);
   PUSH_RELOC(push, src->bo, src->offset, NOUVEAU_BO_LOW, 0, 0);
   PUSH_RELOC(push, src->bo, texfmt, NOUVEAU_BO_OR,
         PUSH_DATA (push, NV30_3D_TEX_WRAP_S_CLAMP_TO_EDGE |
               PUSH_DATA (push, NV40_3D_TEX_ENABLE_ENABLE);
   PUSH_DATA (push, texswz);
   switch (filter) {
   case BILINEAR:
      PUSH_DATA (push, NV30_3D_TEX_FILTER_MIN_LINEAR |
            default:
      PUSH_DATA (push, NV30_3D_TEX_FILTER_MIN_NEAREST |
            }
   PUSH_DATA (push, (src->w << 16) | src->h);
   PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV40_3D(TEX_SIZE1(0)), 1);
   PUSH_DATA (push, 0x00100000 | src->pitch);
   BEGIN_NV04(push, SUBC_3D(0x0b40), 1);
   PUSH_DATA (push, src->d < 2 ? 0x00000001 : 0x00000000);
   BEGIN_NV04(push, NV40_3D(TEX_CACHE_CTL), 1);
            nv30->fragprog.dirty_samplers |= 1;
            /* blit! */
   BEGIN_NV04(push, NV30_3D(SCISSOR_HORIZ), 2);
   PUSH_DATA (push, (dst->x1 - dst->x0) << 16 | dst->x0);
   PUSH_DATA (push, (dst->y1 - dst->y0) << 16 | dst->y0);
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_QUADS);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(8)), 3);
   PUSH_DATAf(push, src->x0);
   PUSH_DATAf(push, src->y0);
   PUSH_DATAf(push, src->z);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(0)), 1);
   PUSH_DATA (push, (dst->y0 << 16) | dst->x0);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(8)), 3);
   PUSH_DATAf(push, src->x1);
   PUSH_DATAf(push, src->y0);
   PUSH_DATAf(push, src->z);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(0)), 1);
   PUSH_DATA (push, (dst->y0 << 16) | dst->x1);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(8)), 3);
   PUSH_DATAf(push, src->x1);
   PUSH_DATAf(push, src->y1);
   PUSH_DATAf(push, src->z);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(0)), 1);
   PUSH_DATA (push, (dst->y1 << 16) | dst->x1);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(8)), 3);
   PUSH_DATAf(push, src->x0);
   PUSH_DATAf(push, src->y1);
   PUSH_DATAf(push, src->z);
   BEGIN_NV04(push, NV30_3D(VTX_ATTR_2I(0)), 1);
   PUSH_DATA (push, (dst->y1 << 16) | dst->x0);
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      }
      static bool
   nv30_transfer_sifm(XFER_ARGS)
   {
      if (!src->pitch || src->w > 1024 || src->h > 1024 || src->w < 2 || src->h < 2)
            if (src->d > 1 || dst->d > 1)
            if (dst->offset & 63)
            if (!dst->pitch) {
      if (dst->w > 2048 || dst->h > 2048 || dst->w < 2 || dst->h < 2)
      } else {
      if (dst->domain != NOUVEAU_BO_VRAM)
         if (dst->pitch & 63)
                  }
      static void
   nv30_transfer_rect_sifm(XFER_ARGS)
      {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_pushbuf_refn refs[] = {
      { src->bo, src->domain | NOUVEAU_BO_RD },
      };
   struct nv04_fifo *fifo = push->channel->data;
   unsigned si_fmt, si_arg;
            switch (dst->cpp) {
   case 4: ss_fmt = NV04_SURFACE_SWZ_FORMAT_COLOR_A8R8G8B8; break;
   case 2: ss_fmt = NV04_SURFACE_SWZ_FORMAT_COLOR_R5G6B5; break;
   default:
      ss_fmt = NV04_SURFACE_SWZ_FORMAT_COLOR_Y8;
               switch (src->cpp) {
   case 4: si_fmt = NV03_SIFM_COLOR_FORMAT_A8R8G8B8; break;
   case 2: si_fmt = NV03_SIFM_COLOR_FORMAT_R5G6B5; break;
   default:
      si_fmt = NV03_SIFM_COLOR_FORMAT_AY8;
               if (filter == NEAREST) {
      si_arg  = NV03_SIFM_FORMAT_ORIGIN_CENTER;
      } else {
      si_arg  = NV03_SIFM_FORMAT_ORIGIN_CORNER;
               if (!PUSH_SPACE_EX(push, 64, 6, 0) ||
      PUSH_REFN(push, refs, 2))
         if (dst->pitch) {
      BEGIN_NV04(push, NV04_SF2D(DMA_IMAGE_SOURCE), 2);
   PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
   PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
   BEGIN_NV04(push, NV04_SF2D(FORMAT), 4);
   PUSH_DATA (push, ss_fmt);
   PUSH_DATA (push, dst->pitch << 16 | dst->pitch);
   PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
   PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
   BEGIN_NV04(push, NV05_SIFM(SURFACE), 1);
      } else {
      BEGIN_NV04(push, NV04_SSWZ(DMA_IMAGE), 1);
   PUSH_RELOC(push, dst->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
   BEGIN_NV04(push, NV04_SSWZ(FORMAT), 2);
   PUSH_DATA (push, ss_fmt | (util_logbase2(dst->w) << 16) |
         PUSH_RELOC(push, dst->bo, dst->offset, NOUVEAU_BO_LOW, 0, 0);
   BEGIN_NV04(push, NV05_SIFM(SURFACE), 1);
               BEGIN_NV04(push, NV03_SIFM(DMA_IMAGE), 1);
   PUSH_RELOC(push, src->bo, 0, NOUVEAU_BO_OR, fifo->vram, fifo->gart);
   BEGIN_NV04(push, NV03_SIFM(COLOR_FORMAT), 8);
   PUSH_DATA (push, si_fmt);
   PUSH_DATA (push, NV03_SIFM_OPERATION_SRCCOPY);
   PUSH_DATA (push, (           dst->y0  << 16) |            dst->x0);
   PUSH_DATA (push, ((dst->y1 - dst->y0) << 16) | (dst->x1 - dst->x0));
   PUSH_DATA (push, (           dst->y0  << 16) |            dst->x0);
   PUSH_DATA (push, ((dst->y1 - dst->y0) << 16) | (dst->x1 - dst->x0));
   PUSH_DATA (push, ((src->x1 - src->x0) << 20) / (dst->x1 - dst->x0));
   PUSH_DATA (push, ((src->y1 - src->y0) << 20) / (dst->y1 - dst->y0));
   BEGIN_NV04(push, NV03_SIFM(SIZE), 4);
   PUSH_DATA (push, align(src->h, 2) << 16 | align(src->w, 2));
   PUSH_DATA (push, src->pitch | si_arg);
   PUSH_RELOC(push, src->bo, src->offset, NOUVEAU_BO_LOW, 0, 0);
      }
      /* The NOP+OFFSET_OUT stuff after each M2MF transfer *is* actually required
   * to prevent some odd things from happening, easily reproducible by
   * attempting to do conditional rendering that has a M2MF transfer done
   * some time before it.  0x1e98 will fail with a DMA_W_PROTECTION (assuming
   * that name is still accurate on nv4x) error.
   */
      static bool
   nv30_transfer_m2mf(XFER_ARGS)
   {
      if (!src->pitch || !dst->pitch)
         if (nv30_transfer_scaled(src, dst))
            }
      static void
   nv30_transfer_rect_m2mf(XFER_ARGS)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_pushbuf_refn refs[] = {
      { src->bo, src->domain | NOUVEAU_BO_RD },
      };
   struct nv04_fifo *fifo = push->channel->data;
   unsigned src_offset = src->offset;
   unsigned dst_offset = dst->offset;
   unsigned w = dst->x1 - dst->x0;
            src_offset += (src->y0 * src->pitch) + (src->x0 * src->cpp);
            BEGIN_NV04(push, NV03_M2MF(DMA_BUFFER_IN), 2);
   PUSH_DATA (push, (src->domain == NOUVEAU_BO_VRAM) ? fifo->vram : fifo->gart);
            while (h) {
               if (!PUSH_SPACE_EX(push, 32, 2, 0) ||
                  BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 8);
   PUSH_RELOC(push, src->bo, src_offset, NOUVEAU_BO_LOW, 0, 0);
   PUSH_RELOC(push, dst->bo, dst_offset, NOUVEAU_BO_LOW, 0, 0);
   PUSH_DATA (push, src->pitch);
   PUSH_DATA (push, dst->pitch);
   PUSH_DATA (push, w * src->cpp);
   PUSH_DATA (push, lines);
   PUSH_DATA (push, NV03_M2MF_FORMAT_INPUT_INC_1 |
         PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV04_GRAPH(M2MF, NOP), 1);
   PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV03_M2MF(OFFSET_OUT), 1);
            h -= lines;
   src_offset += src->pitch * lines;
         }
      static bool
   nv30_transfer_cpu(XFER_ARGS)
   {
      if (nv30_transfer_scaled(src, dst))
            }
      static char *
   linear_ptr(struct nv30_rect *rect, char *base, int x, int y, int z)
   {
         }
      static inline unsigned
   swizzle2d(unsigned v, unsigned s)
   {
      v = (v | (v << 8)) & 0x00ff00ff;
   v = (v | (v << 4)) & 0x0f0f0f0f;
   v = (v | (v << 2)) & 0x33333333;
   v = (v | (v << 1)) & 0x55555555;
      }
      static char *
   swizzle2d_ptr(struct nv30_rect *rect, char *base, int x, int y, int z)
   {
      unsigned k = util_logbase2(MIN2(rect->w, rect->h));
   unsigned km = (1 << k) - 1;
   unsigned nx = rect->w >> k;
   unsigned tx = x >> k;
   unsigned ty = y >> k;
            m  = swizzle2d(x & km, 0);
   m |= swizzle2d(y & km, 1);
               }
      static char *
   swizzle3d_ptr(struct nv30_rect *rect, char *base, int x, int y, int z)
   {
      unsigned w = rect->w >> 1;
   unsigned h = rect->h >> 1;
   unsigned d = rect->d >> 1;
   unsigned i = 0, o;
            do {
      o = i;
   if (w) {
      v |= (x & 1) << i++;
   x >>= 1;
      }
   if (h) {
      v |= (y & 1) << i++;
   y >>= 1;
      }
   if (d) {
      v |= (z & 1) << i++;
   z >>= 1;
                     }
      typedef char *(*get_ptr_t)(struct nv30_rect *, char *, int, int, int);
      static inline get_ptr_t
   get_ptr(struct nv30_rect *rect)
   {
      if (rect->pitch)
            if (rect->d <= 1)
               }
      static void
   nv30_transfer_rect_cpu(XFER_ARGS)
   {
      get_ptr_t sp = get_ptr(src);
   get_ptr_t dp = get_ptr(dst);
   char *srcmap, *dstmap;
            BO_MAP(nv30->base.screen, src->bo, NOUVEAU_BO_RD, nv30->base.client);
   BO_MAP(nv30->base.screen, dst->bo, NOUVEAU_BO_WR, nv30->base.client);
   srcmap = src->bo->map + src->offset;
            for (y = 0; y < (dst->y1 - dst->y0); y++) {
      for (x = 0; x < (dst->x1 - dst->x0); x++) {
      memcpy(dp(dst, dstmap, dst->x0 + x, dst->y0 + y, dst->z),
            }
      void
   nv30_transfer_rect(struct nv30_context *nv30, enum nv30_transfer_filter filter,
         {
      static const struct {
      char *name;
   bool (*possible)(XFER_ARGS);
      } *method, methods[] = {
      { "m2mf", nv30_transfer_m2mf, nv30_transfer_rect_m2mf },
   { "sifm", nv30_transfer_sifm, nv30_transfer_rect_sifm },
   { "blit", nv30_transfer_blit, nv30_transfer_rect_blit },
   { "rect", nv30_transfer_cpu, nv30_transfer_rect_cpu },
               for (method = methods; method->possible; method++) {
      if (method->possible(nv30, filter, src, dst)) {
      method->execute(nv30, filter, src, dst);
                     }
      void
   nv30_transfer_push_data(struct nouveau_context *nv,
               {
      /* use ifc, or scratch + copy_data? */
      }
      void
   nv30_transfer_copy_data(struct nouveau_context *nv,
                     {
      struct nv04_fifo *fifo = nv->screen->channel->data;
   struct nouveau_pushbuf_refn refs[] = {
      { src, s_dom | NOUVEAU_BO_RD },
      };
   struct nouveau_pushbuf *push = nv->pushbuf;
            pages = size >> 12;
            BEGIN_NV04(push, NV03_M2MF(DMA_BUFFER_IN), 2);
   PUSH_DATA (push, (s_dom == NOUVEAU_BO_VRAM) ? fifo->vram : fifo->gart);
            while (pages) {
      lines  = (pages > 2047) ? 2047 : pages;
            if (!PUSH_SPACE_EX(push, 32, 2, 0) ||
                  BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 8);
   PUSH_RELOC(push, src, s_off, NOUVEAU_BO_LOW, 0, 0);
   PUSH_RELOC(push, dst, d_off, NOUVEAU_BO_LOW, 0, 0);
   PUSH_DATA (push, 4096);
   PUSH_DATA (push, 4096);
   PUSH_DATA (push, 4096);
   PUSH_DATA (push, lines);
   PUSH_DATA (push, NV03_M2MF_FORMAT_INPUT_INC_1 |
         PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV04_GRAPH(M2MF, NOP), 1);
   PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV03_M2MF(OFFSET_OUT), 1);
            s_off += (lines << 12);
               if (size) {
      if (!PUSH_SPACE_EX(push, 32, 2, 0) ||
                  BEGIN_NV04(push, NV03_M2MF(OFFSET_IN), 8);
   PUSH_RELOC(push, src, s_off, NOUVEAU_BO_LOW, 0, 0);
   PUSH_RELOC(push, dst, d_off, NOUVEAU_BO_LOW, 0, 0);
   PUSH_DATA (push, size);
   PUSH_DATA (push, size);
   PUSH_DATA (push, size);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, NV03_M2MF_FORMAT_INPUT_INC_1 |
         PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV04_GRAPH(M2MF, NOP), 1);
   PUSH_DATA (push, 0x00000000);
   BEGIN_NV04(push, NV03_M2MF(OFFSET_OUT), 1);
         }
