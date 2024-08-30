      #include "util/format/u_format.h"
      #include "nv50/nv50_context.h"
      #include "nv50/g80_defs.xml.h"
      struct nv50_transfer {
      struct pipe_transfer base;
   struct nv50_m2mf_rect rect[2];
   uint32_t nblocksx;
      };
      void
   nv50_m2mf_rect_setup(struct nv50_m2mf_rect *rect,
               {
      struct nv50_miptree *mt = nv50_miptree(res);
   const unsigned w = u_minify(res->width0, l);
            rect->bo = mt->base.bo;
   rect->domain = mt->base.domain;
   rect->base = mt->level[l].offset;
   if (mt->base.bo->offset != mt->base.address)
         rect->pitch = mt->level[l].pitch;
   if (util_format_is_plain(res->format)) {
      rect->width = w << mt->ms_x;
   rect->height = h << mt->ms_y;
   rect->x = x << mt->ms_x;
      } else {
      rect->width = util_format_get_nblocksx(res->format, w);
   rect->height = util_format_get_nblocksy(res->format, h);
   rect->x = util_format_get_nblocksx(res->format, x);
      }
   rect->tile_mode = mt->level[l].tile_mode;
            if (mt->layout_3d) {
      rect->z = z;
      } else {
      rect->base += z * mt->layer_stride;
   rect->z = 0;
         }
      /* This is very similar to nv50_2d_texture_do_copy, but doesn't require
   * miptree objects. Maybe refactor? Although it's not straightforward.
   */
   static void
   nv50_2d_transfer_rect(struct nv50_context *nv50,
                     {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nouveau_bufctx *bctx = nv50->bufctx;
            nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            uint32_t format;
   switch (cpp) {
   case 1:
      format = G80_SURFACE_FORMAT_R8_UNORM;
      case 2:
      format = G80_SURFACE_FORMAT_R16_UNORM;
      case 4:
      format = G80_SURFACE_FORMAT_BGRA8_UNORM;
      case 8:
      format = G80_SURFACE_FORMAT_RGBA16_FLOAT;
      case 16:
      format = G80_SURFACE_FORMAT_RGBA32_FLOAT;
      default:
      assert(!"Unexpected cpp");
               if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NV04(push, NV50_2D(SRC_FORMAT), 5);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, src->tile_mode);
   PUSH_DATA (push, src->depth);
   PUSH_DATA (push, src->z);
   BEGIN_NV04(push, NV50_2D(SRC_WIDTH), 4);
   PUSH_DATA (push, src->width);
   PUSH_DATA (push, src->height);
   PUSH_DATAh(push, src->bo->offset + src->base);
      } else {
      BEGIN_NV04(push, NV50_2D(SRC_FORMAT), 2);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(SRC_PITCH), 5);
   PUSH_DATA (push, src->pitch);
   PUSH_DATA (push, src->width);
   PUSH_DATA (push, src->height);
   PUSH_DATAh(push, src->bo->offset + src->base);
               if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NV04(push, NV50_2D(DST_FORMAT), 5);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, dst->tile_mode);
   PUSH_DATA (push, dst->depth);
   PUSH_DATA (push, dst->z);
   BEGIN_NV04(push, NV50_2D(DST_WIDTH), 4);
   PUSH_DATA (push, dst->width);
   PUSH_DATA (push, dst->height);
   PUSH_DATAh(push, dst->bo->offset + dst->base);
      } else {
      BEGIN_NV04(push, NV50_2D(DST_FORMAT), 2);
   PUSH_DATA (push, format);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(DST_PITCH), 5);
   PUSH_DATA (push, dst->pitch);
   PUSH_DATA (push, dst->width);
   PUSH_DATA (push, dst->height);
   PUSH_DATAh(push, dst->bo->offset + dst->base);
               BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, NV50_2D_BLIT_CONTROL_FILTER_POINT_SAMPLE);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dst->x);
   PUSH_DATA (push, dst->y);
   PUSH_DATA (push, nblocksx);
   PUSH_DATA (push, nblocksy);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, src->x);
   PUSH_DATA (push, 0);
               }
      void
   nv50_m2mf_transfer_rect(struct nv50_context *nv50,
                     {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nouveau_bufctx *bctx = nv50->bufctx;
   const int cpp = dst->cpp;
   uint32_t src_ofst = src->base;
   uint32_t dst_ofst = dst->base;
   uint32_t height = nblocksy;
   uint32_t sy = src->y;
                     /* Workaround: M2MF appears to break at the 64k boundary for tiled
   * textures, which can really only happen with RGBA32 formats.
   */
   bool eng2d = false;
   if (nouveau_bo_memtype(src->bo)) {
      if (src->width * cpp > 65536)
      }
   if (nouveau_bo_memtype(dst->bo)) {
      if (dst->width * cpp > 65536)
      }
   if (eng2d) {
      nv50_2d_transfer_rect(nv50, dst, src, nblocksx, nblocksy);
               nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NV04(push, NV50_M2MF(LINEAR_IN), 6);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, src->tile_mode);
   PUSH_DATA (push, src->width * cpp);
   PUSH_DATA (push, src->height);
   PUSH_DATA (push, src->depth);
      } else {
               BEGIN_NV04(push, NV50_M2MF(LINEAR_IN), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_PITCH_IN), 1);
               if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NV04(push, NV50_M2MF(LINEAR_OUT), 6);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, dst->tile_mode);
   PUSH_DATA (push, dst->width * cpp);
   PUSH_DATA (push, dst->height);
   PUSH_DATA (push, dst->depth);
      } else {
               BEGIN_NV04(push, NV50_M2MF(LINEAR_OUT), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_PITCH_OUT), 1);
               while (height) {
               BEGIN_NV04(push, NV50_M2MF(OFFSET_IN_HIGH), 2);
   PUSH_DATAh(push, src->bo->offset + src_ofst);
            BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_OFFSET_IN), 2);
   PUSH_DATA (push, src->bo->offset + src_ofst);
            if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NV04(push, NV50_M2MF(TILING_POSITION_IN), 1);
      } else {
         }
   if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NV04(push, NV50_M2MF(TILING_POSITION_OUT), 1);
      } else {
                  BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_LINE_LENGTH_IN), 4);
   PUSH_DATA (push, nblocksx * cpp);
   PUSH_DATA (push, line_count);
   PUSH_DATA (push, (1 << 8) | (1 << 0));
            height -= line_count;
   sy += line_count;
                  }
      void
   nv50_sifc_linear_u8(struct nouveau_context *nv,
               {
      struct nv50_context *nv50 = nv50_context(&nv->pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   uint32_t *src = (uint32_t *)data;
   unsigned count = DIV_ROUND_UP(size, 4);
            nouveau_bufctx_refn(nv50->bufctx, 0, dst, domain | NOUVEAU_BO_WR);
                     while (count) {
      unsigned xcoord = offset & 0xff;
            BEGIN_NV04(push, NV50_2D(DST_FORMAT), 2);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(DST_PITCH), 5);
   PUSH_DATA (push, 262144);
   PUSH_DATA (push, 65536);
   PUSH_DATA (push, 1);
   PUSH_DATAh(push, dst->offset + offset);
   PUSH_DATA (push, dst->offset + offset);
   BEGIN_NV04(push, NV50_2D(SIFC_BITMAP_ENABLE), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, G80_SURFACE_FORMAT_R8_UNORM);
   BEGIN_NV04(push, NV50_2D(SIFC_WIDTH), 10);
   PUSH_DATA (push, MIN2(size, max_size));
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, xcoord);
   PUSH_DATA (push, 0);
            unsigned iter_count = MIN2(count, max_size / 4);
   count -= iter_count;
   offset += max_size;
            while (iter_count) {
                              src += nr;
                     }
      void
   nv50_m2mf_copy_linear(struct nouveau_context *nv,
                     {
      struct nouveau_pushbuf *push = nv->pushbuf;
            nouveau_bufctx_refn(bctx, 0, src, srcdom | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst, dstdom | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            BEGIN_NV04(push, NV50_M2MF(LINEAR_IN), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_M2MF(LINEAR_OUT), 1);
            while (size) {
               BEGIN_NV04(push, NV50_M2MF(OFFSET_IN_HIGH), 2);
   PUSH_DATAh(push, src->offset + srcoff);
   PUSH_DATAh(push, dst->offset + dstoff);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_OFFSET_IN), 2);
   PUSH_DATA (push, src->offset + srcoff);
   PUSH_DATA (push, dst->offset + dstoff);
   BEGIN_NV04(push, SUBC_M2MF(NV03_M2MF_LINE_LENGTH_IN), 4);
   PUSH_DATA (push, bytes);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, (1 << 8) | (1 << 0));
            srcoff += bytes;
   dstoff += bytes;
                  }
      void *
   nv50_miptree_transfer_map(struct pipe_context *pctx,
                           struct pipe_resource *res,
   {
      struct nv50_context *nv50 = nv50_context(pctx);
   struct nouveau_device *dev = nv50->screen->base.device;
   const struct nv50_miptree *mt = nv50_miptree(res);
   struct nv50_transfer *tx;
   uint32_t size;
   int ret;
            if (usage & PIPE_MAP_DIRECTLY)
            tx = CALLOC_STRUCT(nv50_transfer);
   if (!tx)
                     tx->base.level = level;
   tx->base.usage = usage;
            if (util_format_is_plain(res->format)) {
      tx->nblocksx = box->width << mt->ms_x;
      } else {
      tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
               tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);
                              ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
         if (ret) {
      FREE(tx);
               tx->rect[1].cpp = tx->rect[0].cpp;
   tx->rect[1].width = tx->nblocksx;
   tx->rect[1].height = tx->nblocksy;
   tx->rect[1].depth = 1;
   tx->rect[1].pitch = tx->base.stride;
            if (usage & PIPE_MAP_READ) {
      unsigned base = tx->rect[0].base;
   unsigned z = tx->rect[0].z;
   unsigned i;
   for (i = 0; i < box->depth; ++i) {
      nv50_m2mf_transfer_rect(nv50, &tx->rect[1], &tx->rect[0],
         if (mt->layout_3d)
         else
            }
   tx->rect[0].z = z;
   tx->rect[0].base = base;
               if (tx->rect[1].bo->map) {
      *ptransfer = &tx->base;
               if (usage & PIPE_MAP_READ)
         if (usage & PIPE_MAP_WRITE)
            ret = BO_MAP(nv50->base.screen, tx->rect[1].bo, flags, nv50->base.client);
   if (ret) {
      nouveau_bo_ref(NULL, &tx->rect[1].bo);
   FREE(tx);
               *ptransfer = &tx->base;
      }
      void
   nv50_miptree_transfer_unmap(struct pipe_context *pctx,
         {
      struct nv50_context *nv50 = nv50_context(pctx);
   struct nv50_transfer *tx = (struct nv50_transfer *)transfer;
   struct nv50_miptree *mt = nv50_miptree(tx->base.resource);
            if (tx->base.usage & PIPE_MAP_WRITE) {
      for (i = 0; i < tx->base.box.depth; ++i) {
      nv50_m2mf_transfer_rect(nv50, &tx->rect[0], &tx->rect[1],
         if (mt->layout_3d)
         else
                     /* Allow the copies above to finish executing before freeing the source */
   nouveau_fence_work(nv50->base.fence,
      } else {
                              }
      static void
   nv50_cb_bo_push(struct nouveau_context *nv,
                  struct nouveau_bo *bo, unsigned domain,
      {
                        while (words) {
               PUSH_SPACE(push, nr + 3);
   PUSH_REF1 (push, bo, NOUVEAU_BO_WR | domain);
   BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
   PUSH_DATA (push, (offset << 6) | bufid);
   BEGIN_NI04(push, NV50_3D(CB_DATA(0)), nr);
            words -= nr;
   data += nr;
         }
      void
   nv50_cb_push(struct nouveau_context *nv,
               {
      struct nv50_context *nv50 = nv50_context(&nv->pipe);
   struct nv50_constbuf *cb = NULL;
   int s, bufid;
   /* Go through all the constbuf binding points of this buffer and try to
   * find one which contains the region to be updated.
   */
   for (s = 0; s < NV50_MAX_SHADER_STAGES && !cb; s++) {
      uint16_t bindings = res->cb_bindings[s];
   while (bindings) {
                     bindings &= ~(1 << i);
   if (cb_offset <= offset &&
      cb_offset + nv50->constbuf[s][i].size >= offset + words * 4) {
   cb = &nv50->constbuf[s][i];
   bufid = s * 16 + i;
                     if (cb) {
      nv50_cb_bo_push(nv, res->bo, res->domain,
      } else {
      nv->push_data(nv, res->bo, res->offset + offset, res->domain,
         }
