      #include "util/format/u_format.h"
      #include "nvc0/nvc0_context.h"
      struct nvc0_transfer {
      struct pipe_transfer base;
   struct nv50_m2mf_rect rect[2];
   uint32_t nblocksx;
   uint16_t nblocksy;
      };
      static void
   nvc0_m2mf_transfer_rect(struct nvc0_context *nvc0,
                     {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bufctx *bctx = nvc0->bufctx;
   const int cpp = dst->cpp;
   uint32_t src_ofst = src->base;
   uint32_t dst_ofst = dst->base;
   uint32_t height = nblocksy;
   uint32_t sy = src->y;
   uint32_t dy = dst->y;
                     nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_IN), 5);
   PUSH_DATA (push, src->tile_mode);
   PUSH_DATA (push, src->width * cpp);
   PUSH_DATA (push, src->height);
   PUSH_DATA (push, src->depth);
      } else {
               BEGIN_NVC0(push, NVC0_M2MF(PITCH_IN), 1);
                        if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_MODE_OUT), 5);
   PUSH_DATA (push, dst->tile_mode);
   PUSH_DATA (push, dst->width * cpp);
   PUSH_DATA (push, dst->height);
   PUSH_DATA (push, dst->depth);
      } else {
               BEGIN_NVC0(push, NVC0_M2MF(PITCH_OUT), 1);
                        while (height) {
               BEGIN_NVC0(push, NVC0_M2MF(OFFSET_IN_HIGH), 2);
   PUSH_DATAh(push, src->bo->offset + src_ofst);
            BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
   PUSH_DATAh(push, dst->bo->offset + dst_ofst);
            if (!(exec & NVC0_M2MF_EXEC_LINEAR_IN)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_IN_X), 2);
   PUSH_DATA (push, src->x * cpp);
      } else {
         }
   if (!(exec & NVC0_M2MF_EXEC_LINEAR_OUT)) {
      BEGIN_NVC0(push, NVC0_M2MF(TILING_POSITION_OUT_X), 2);
   PUSH_DATA (push, dst->x * cpp);
      } else {
                  BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
   PUSH_DATA (push, nblocksx * cpp);
   PUSH_DATA (push, line_count);
   BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
            height -= line_count;
   sy += line_count;
                  }
      static void
   nve4_m2mf_transfer_rect(struct nvc0_context *nvc0,
                     {
      static const struct {
      int cs;
      } cpbs[] = {
      [ 1] = { 1, 1 },
   [ 2] = { 1, 2 },
   [ 3] = { 1, 3 },
   [ 4] = { 1, 4 },
   [ 6] = { 2, 3 },
   [ 8] = { 2, 4 },
   [ 9] = { 3, 3 },
   [12] = { 3, 4 },
      };
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bufctx *bctx = nvc0->bufctx;
   uint32_t exec;
   uint32_t src_base = src->base;
            assert(dst->cpp < ARRAY_SIZE(cpbs) && cpbs[dst->cpp].cs);
            nouveau_bufctx_refn(bctx, 0, dst->bo, dst->domain | NOUVEAU_BO_WR);
   nouveau_bufctx_refn(bctx, 0, src->bo, src->domain | NOUVEAU_BO_RD);
   nouveau_pushbuf_bufctx(push, bctx);
                     BEGIN_NVC0(push, NVE4_COPY(SWIZZLE), 1);
   PUSH_DATA (push, (cpbs[dst->cpp].nc - 1) << 24 |
                  (cpbs[src->cpp].nc - 1) << 20 |
   (cpbs[src->cpp].cs - 1) << 16 |
   3 << 12 /* DST_W = SRC_W */ |
         if (nouveau_bo_memtype(dst->bo)) {
      BEGIN_NVC0(push, NVE4_COPY(DST_BLOCK_DIMENSIONS), 6);
   PUSH_DATA (push, dst->tile_mode | NVE4_COPY_SRC_BLOCK_DIMENSIONS_GOB_HEIGHT_FERMI_8);
   PUSH_DATA (push, dst->width);
   PUSH_DATA (push, dst->height);
   PUSH_DATA (push, dst->depth);
   PUSH_DATA (push, dst->z);
      } else {
      assert(!dst->z);
   dst_base += dst->y * dst->pitch + dst->x * dst->cpp;
               if (nouveau_bo_memtype(src->bo)) {
      BEGIN_NVC0(push, NVE4_COPY(SRC_BLOCK_DIMENSIONS), 6);
   PUSH_DATA (push, src->tile_mode | NVE4_COPY_SRC_BLOCK_DIMENSIONS_GOB_HEIGHT_FERMI_8);
   PUSH_DATA (push, src->width);
   PUSH_DATA (push, src->height);
   PUSH_DATA (push, src->depth);
   PUSH_DATA (push, src->z);
      } else {
      assert(!src->z);
   src_base += src->y * src->pitch + src->x * src->cpp;
               BEGIN_NVC0(push, NVE4_COPY(SRC_ADDRESS_HIGH), 8);
   PUSH_DATAh(push, src->bo->offset + src_base);
   PUSH_DATA (push, src->bo->offset + src_base);
   PUSH_DATAh(push, dst->bo->offset + dst_base);
   PUSH_DATA (push, dst->bo->offset + dst_base);
   PUSH_DATA (push, src->pitch);
   PUSH_DATA (push, dst->pitch);
   PUSH_DATA (push, nblocksx);
            BEGIN_NVC0(push, NVE4_COPY(EXEC), 1);
               }
      void
   nvc0_m2mf_push_linear(struct nouveau_context *nv,
               {
      struct nvc0_context *nvc0 = nvc0_context(&nv->pipe);
   struct nouveau_pushbuf *push = nv->pushbuf;
   uint32_t *src = (uint32_t *)data;
            nouveau_bufctx_refn(nvc0->bufctx, 0, dst, domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
            while (count) {
               if (!PUSH_SPACE(push, nr + 9))
            BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
   PUSH_DATAh(push, dst->offset + offset);
   PUSH_DATA (push, dst->offset + offset);
   BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
   PUSH_DATA (push, MIN2(size, nr * 4));
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
            /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
   BEGIN_NIC0(push, NVC0_M2MF(DATA), nr);
            count -= nr;
   src += nr;
   offset += nr * 4;
                  }
      void
   nve4_p2mf_push_linear(struct nouveau_context *nv,
               {
      struct nvc0_context *nvc0 = nvc0_context(&nv->pipe);
   struct nouveau_pushbuf *push = nv->pushbuf;
   uint32_t *src = (uint32_t *)data;
            nouveau_bufctx_refn(nvc0->bufctx, 0, dst, domain | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, nvc0->bufctx);
            while (count) {
               if (!PUSH_SPACE(push, nr + 10))
            BEGIN_NVC0(push, NVE4_P2MF(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, dst->offset + offset);
   PUSH_DATA (push, dst->offset + offset);
   BEGIN_NVC0(push, NVE4_P2MF(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, MIN2(size, nr * 4));
   PUSH_DATA (push, 1);
   /* must not be interrupted (trap on QUERY fence, 0x50 works however) */
   BEGIN_1IC0(push, NVE4_P2MF(UPLOAD_EXEC), nr + 1);
   PUSH_DATA (push, 0x1001);
            count -= nr;
   src += nr;
   offset += nr * 4;
                  }
      static void
   nvc0_m2mf_copy_linear(struct nouveau_context *nv,
                     {
      struct nouveau_pushbuf *push = nv->pushbuf;
            nouveau_bufctx_refn(bctx, 0, src, srcdom | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst, dstdom | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            while (size) {
               BEGIN_NVC0(push, NVC0_M2MF(OFFSET_OUT_HIGH), 2);
   PUSH_DATAh(push, dst->offset + dstoff);
   PUSH_DATA (push, dst->offset + dstoff);
   BEGIN_NVC0(push, NVC0_M2MF(OFFSET_IN_HIGH), 2);
   PUSH_DATAh(push, src->offset + srcoff);
   PUSH_DATA (push, src->offset + srcoff);
   BEGIN_NVC0(push, NVC0_M2MF(LINE_LENGTH_IN), 2);
   PUSH_DATA (push, bytes);
   PUSH_DATA (push, 1);
   BEGIN_NVC0(push, NVC0_M2MF(EXEC), 1);
   PUSH_DATA (push, NVC0_M2MF_EXEC_QUERY_SHORT |
            srcoff += bytes;
   dstoff += bytes;
                  }
      static void
   nve4_m2mf_copy_linear(struct nouveau_context *nv,
                     {
      struct nouveau_pushbuf *push = nv->pushbuf;
            nouveau_bufctx_refn(bctx, 0, src, srcdom | NOUVEAU_BO_RD);
   nouveau_bufctx_refn(bctx, 0, dst, dstdom | NOUVEAU_BO_WR);
   nouveau_pushbuf_bufctx(push, bctx);
            BEGIN_NVC0(push, NVE4_COPY(SRC_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, src->offset + srcoff);
   PUSH_DATA (push, src->offset + srcoff);
   PUSH_DATAh(push, dst->offset + dstoff);
   PUSH_DATA (push, dst->offset + dstoff);
   BEGIN_NVC0(push, NVE4_COPY(X_COUNT), 1);
   PUSH_DATA (push, size);
   BEGIN_NVC0(push, NVE4_COPY(EXEC), 1);
   PUSH_DATA (push, NVE4_COPY_EXEC_COPY_MODE_NON_PIPELINED |
      NVE4_COPY_EXEC_FLUSH |
   NVE4_COPY_EXEC_SRC_LAYOUT_BLOCKLINEAR |
            }
         static inline bool
   nvc0_mt_transfer_can_map_directly(struct nv50_miptree *mt)
   {
      if (mt->base.domain == NOUVEAU_BO_VRAM)
         if (mt->base.base.usage != PIPE_USAGE_STAGING)
            }
      static inline bool
   nvc0_mt_sync(struct nvc0_context *nvc0, struct nv50_miptree *mt, unsigned usage)
   {
      if (!mt->base.mm) {
      uint32_t access = (usage & PIPE_MAP_WRITE) ?
            }
   if (usage & PIPE_MAP_WRITE)
            }
      void *
   nvc0_miptree_transfer_map(struct pipe_context *pctx,
                           struct pipe_resource *res,
   {
      struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct nouveau_device *dev = nvc0->screen->base.device;
   struct nv50_miptree *mt = nv50_miptree(res);
   struct nvc0_transfer *tx;
   uint32_t size;
   int ret;
            if (nvc0_mt_transfer_can_map_directly(mt)) {
      ret = !nvc0_mt_sync(nvc0, mt, usage);
   if (!ret)
         if (ret &&
      (usage & PIPE_MAP_DIRECTLY))
      if (!ret)
      } else
   if (usage & PIPE_MAP_DIRECTLY)
            tx = CALLOC_STRUCT(nvc0_transfer);
   if (!tx)
                     tx->base.level = level;
   tx->base.usage = usage;
            if (util_format_is_plain(res->format)) {
      tx->nblocksx = box->width << mt->ms_x;
      } else {
      tx->nblocksx = util_format_get_nblocksx(res->format, box->width);
      }
            if (usage & PIPE_MAP_DIRECTLY) {
      tx->base.stride = mt->level[level].pitch;
   tx->base.layer_stride = mt->layer_stride;
   uint32_t offset = box->y * tx->base.stride +
         if (!mt->layout_3d)
         else
         *ptransfer = &tx->base;
               tx->base.stride = tx->nblocksx * util_format_get_blocksize(res->format);
                              ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0,
         if (ret) {
      pipe_resource_reference(&tx->base.resource, NULL);
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
   for (i = 0; i < tx->nlayers; ++i) {
      nvc0->m2mf_copy_rect(nvc0, &tx->rect[1], &tx->rect[0],
         if (mt->layout_3d)
         else
            }
   tx->rect[0].z = z;
   tx->rect[0].base = base;
               if (tx->rect[1].bo->map) {
      *ptransfer = &tx->base;
               if (usage & PIPE_MAP_READ)
         if (usage & PIPE_MAP_WRITE)
            ret = BO_MAP(nvc0->base.screen, tx->rect[1].bo, flags, nvc0->base.client);
   if (ret) {
      pipe_resource_reference(&tx->base.resource, NULL);
   nouveau_bo_ref(NULL, &tx->rect[1].bo);
   FREE(tx);
               *ptransfer = &tx->base;
      }
      void
   nvc0_miptree_transfer_unmap(struct pipe_context *pctx,
         {
      struct nvc0_context *nvc0 = nvc0_context(pctx);
   struct nvc0_transfer *tx = (struct nvc0_transfer *)transfer;
   struct nv50_miptree *mt = nv50_miptree(tx->base.resource);
            if (tx->base.usage & PIPE_MAP_DIRECTLY) {
               FREE(tx);
               if (tx->base.usage & PIPE_MAP_WRITE) {
      for (i = 0; i < tx->nlayers; ++i) {
      nvc0->m2mf_copy_rect(nvc0, &tx->rect[0], &tx->rect[1],
         if (mt->layout_3d)
         else
            }
            /* Allow the copies above to finish executing before freeing the source */
   nouveau_fence_work(nvc0->base.fence,
      } else {
         }
   if (tx->base.usage & PIPE_MAP_READ)
                        }
      /* This happens rather often with DTD9/st. */
   static void
   nvc0_cb_push(struct nouveau_context *nv,
               {
      struct nvc0_context *nvc0 = nvc0_context(&nv->pipe);
   struct nvc0_constbuf *cb = NULL;
            /* Go through all the constbuf binding points of this buffer and try to
   * find one which contains the region to be updated.
   */
   for (s = 0; s < 6 && !cb; s++) {
      uint16_t bindings = res->cb_bindings[s];
   while (bindings) {
                     bindings &= ~(1 << i);
   if (cb_offset <= offset &&
      cb_offset + nvc0->constbuf[s][i].size >= offset + words * 4) {
   cb = &nvc0->constbuf[s][i];
                     if (cb) {
      nvc0_cb_bo_push(nv, res->bo, res->domain,
            } else {
      nv->push_data(nv, res->bo, res->offset + offset, res->domain,
         }
      void
   nvc0_cb_bo_push(struct nouveau_context *nv,
                     {
               NOUVEAU_DRV_STAT(nv->screen, constbuf_upload_count, 1);
            assert(!(offset & 3));
            assert(offset < size);
            BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, size);
   PUSH_DATAh(push, bo->offset + base);
            while (words) {
               PUSH_SPACE(push, nr + 2);
   PUSH_REF1 (push, bo, NOUVEAU_BO_WR | domain);
   BEGIN_1IC0(push, NVC0_3D(CB_POS), nr + 1);
   PUSH_DATA (push, offset);
            words -= nr;
   data += nr;
         }
      void
   nvc0_init_transfer_functions(struct nvc0_context *nvc0)
   {
      if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) {
      nvc0->m2mf_copy_rect = nve4_m2mf_transfer_rect;
   nvc0->base.copy_data = nve4_m2mf_copy_linear;
      } else {
      nvc0->m2mf_copy_rect = nvc0_m2mf_transfer_rect;
   nvc0->base.copy_data = nvc0_m2mf_copy_linear;
      }
      }
