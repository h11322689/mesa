   /*
   * Copyright 2012 Francisco Jerez
   * Copyright 2015 Samuel Pitoiset
   *
   * Permission is hereby granted, free of charge, to any person obtaining
   * a copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sublicense, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial
   * portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
   * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   */
      #include "util/format/u_format.h"
   #include "nv50/nv50_context.h"
   #include "nv50/nv50_compute.xml.h"
      #include "nv50_ir_driver.h"
      int
   nv50_screen_compute_setup(struct nv50_screen *screen,
         {
      struct nouveau_device *dev = screen->base.device;
   struct nouveau_object *chan = screen->base.channel;
   struct nv04_fifo *fifo = (struct nv04_fifo *)chan->data;
   unsigned obj_class;
            switch (dev->chipset & 0xf0) {
   case 0x50:
   case 0x80:
   case 0x90:
      obj_class = NV50_COMPUTE_CLASS;
      case 0xa0:
      switch (dev->chipset) {
   case 0xa3:
   case 0xa5:
   case 0xa8:
      obj_class = NVA3_COMPUTE_CLASS;
      default:
      obj_class = NV50_COMPUTE_CLASS;
      }
      default:
      NOUVEAU_ERR("unsupported chipset: NV%02x\n", dev->chipset);
               ret = nouveau_object_new(chan, 0xbeef50c0, obj_class, NULL, 0,
         if (ret)
            BEGIN_NV04(push, SUBC_CP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NV04(push, NV50_CP(UNK02A0), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(DMA_STACK), 1);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_CP(STACK_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->stack_bo->offset);
   PUSH_DATA (push, screen->stack_bo->offset);
   BEGIN_NV04(push, NV50_CP(STACK_SIZE_LOG), 1);
            BEGIN_NV04(push, NV50_CP(UNK0290), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(LANES32_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(REG_MODE), 1);
   PUSH_DATA (push, NV50_COMPUTE_REG_MODE_STRIPED);
   BEGIN_NV04(push, NV50_CP(UNK0384), 1);
   PUSH_DATA (push, 0x100);
   BEGIN_NV04(push, NV50_CP(DMA_GLOBAL), 1);
            for (i = 0; i < 15; i++) {
      BEGIN_NV04(push, NV50_CP(GLOBAL_ADDRESS_HIGH(i)), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_CP(GLOBAL_LIMIT(i)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_CP(GLOBAL_MODE(i)), 1);
               BEGIN_NV04(push, NV50_CP(GLOBAL_ADDRESS_HIGH(15)), 2);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_CP(GLOBAL_LIMIT(15)), 1);
   PUSH_DATA (push, ~0);
   BEGIN_NV04(push, NV50_CP(GLOBAL_MODE(15)), 1);
            BEGIN_NV04(push, NV50_CP(LOCAL_WARPS_LOG_ALLOC), 1);
   PUSH_DATA (push, 7);
   BEGIN_NV04(push, NV50_CP(LOCAL_WARPS_NO_CLAMP), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(STACK_WARPS_LOG_ALLOC), 1);
   PUSH_DATA (push, 7);
   BEGIN_NV04(push, NV50_CP(STACK_WARPS_NO_CLAMP), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(USER_PARAM_COUNT), 1);
            BEGIN_NV04(push, NV50_CP(DMA_TEXTURE), 1);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_CP(TEX_LIMITS), 1);
   PUSH_DATA (push, 0x54);
   BEGIN_NV04(push, NV50_CP(LINKED_TSC), 1);
            BEGIN_NV04(push, NV50_CP(DMA_TIC), 1);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_CP(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
            BEGIN_NV04(push, NV50_CP(DMA_TSC), 1);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_CP(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
            BEGIN_NV04(push, NV50_CP(DMA_CODE_CB), 1);
            BEGIN_NV04(push, NV50_CP(DMA_LOCAL), 1);
   PUSH_DATA (push, fifo->vram);
   BEGIN_NV04(push, NV50_CP(LOCAL_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->tls_bo->offset + 65536);
   PUSH_DATA (push, screen->tls_bo->offset + 65536);
   BEGIN_NV04(push, NV50_CP(LOCAL_SIZE_LOG), 1);
            BEGIN_NV04(push, NV50_CP(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->uniforms->offset + (3 << 16));
   PUSH_DATA (push, screen->uniforms->offset + (3 << 16));
            BEGIN_NV04(push, NV50_CP(QUERY_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->fence.bo->offset + 16);
               }
      static void
   nv50_compute_validate_samplers(struct nv50_context *nv50)
   {
      bool need_flush = nv50_validate_tsc(nv50, NV50_SHADER_STAGE_COMPUTE);
   if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_CP(TSC_FLUSH), 1);
               /* Invalidate all 3D samplers because they are aliased. */
      }
      static void
   nv50_compute_validate_textures(struct nv50_context *nv50)
   {
      bool need_flush = nv50_validate_tic(nv50, NV50_SHADER_STAGE_COMPUTE);
   if (need_flush) {
      BEGIN_NV04(nv50->base.pushbuf, NV50_CP(TIC_FLUSH), 1);
               /* Invalidate all 3D textures because they are aliased. */
   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_3D_TEXTURES);
      }
      static inline void
   nv50_compute_invalidate_constbufs(struct nv50_context *nv50)
   {
               /* Invalidate all 3D constbufs because they are aliased with COMPUTE. */
   for (s = 0; s < NV50_MAX_3D_SHADER_STAGES; s++) {
      nv50->constbuf_dirty[s] |= nv50->constbuf_valid[s];
      }
      }
      static void
   nv50_compute_validate_constbufs(struct nv50_context *nv50)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
            while (nv50->constbuf_dirty[s]) {
      int i = ffs(nv50->constbuf_dirty[s]) - 1;
            if (nv50->constbuf[s][i].user) {
      const unsigned b = NV50_CB_PVP + s;
   unsigned start = 0;
   unsigned words = nv50->constbuf[s][0].size / 4;
   if (i) {
      NOUVEAU_ERR("user constbufs only supported in slot 0\n");
      }
   if (!nv50->state.uniform_buffer_bound[s]) {
      nv50->state.uniform_buffer_bound[s] = true;
   BEGIN_NV04(push, NV50_CP(SET_PROGRAM_CB), 1);
      }
                     PUSH_SPACE(push, nr + 3);
   BEGIN_NV04(push, NV50_CP(CB_ADDR), 1);
   PUSH_DATA (push, (start << 8) | b);
                  start += nr;
         } else {
      struct nv04_resource *res =
         if (res) {
                              BEGIN_NV04(push, NV50_CP(CB_DEF_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, res->address + nv50->constbuf[s][i].offset);
   PUSH_DATA (push, res->address + nv50->constbuf[s][i].offset);
   PUSH_DATA (push, (b << 16) |
                                 nv50->cb_dirty = 1; /* Force cache flush for UBO. */
      } else {
      BEGIN_NV04(push, NV50_CP(SET_PROGRAM_CB), 1);
      }
   if (i == 0)
                  // TODO: Check if having orthogonal slots means the two don't trample over
   // each other.
      }
      static void
   nv50_get_surface_dims(const struct pipe_image_view *view,
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
      static void
   nv50_mark_image_range_valid(const struct pipe_image_view *view)
   {
                        util_range_add(&res->base, &res->valid_buffer_range,
            }
      static inline void
   nv50_set_surface_info(struct nouveau_pushbuf *push,
               {
      struct nv04_resource *res;
                     /* Make sure to always initialize the surface information area because it's
   * used to check if the given image is bound or not. */
            if (!view || !view->resource)
                  /* Stick the image dimensions for the imageSize() builtin. */
   info[0] = width;
   info[1] = height;
            /* Stick the blockwidth (ie. number of bytes per pixel) to calculate pixel
   * offset and to check if the format doesn't mismatch. */
            if (res->base.target != PIPE_BUFFER) {
      struct nv50_miptree *mt = nv50_miptree(&res->base);
   struct nv50_miptree_level *lvl = &mt->level[view->u.tex.level];
   unsigned nby = align(util_format_get_nblocksy(view->format, height),
            if (mt->layout_3d) {
      info[4] = nby;
      } else {
         }
   info[6] = mt->ms_x;
   info[7] = mt->ms_y;
   info[8] = NV50_TILE_SHIFT_X(lvl->tile_mode);
   info[9] = NV50_TILE_SHIFT_Y(lvl->tile_mode);
         }
      static void
   nv50_compute_validate_surfaces(struct nv50_context *nv50)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
            for (i = 0; i < NV50_MAX_GLOBALS - 1; i++) {
      struct nv50_gmem_state *gmem = &nv50->compprog->cp.gmem[i];
   int width, height, depth;
                     if (gmem->valid && !gmem->image && nv50->buffers[gmem->slot].buffer) {
      struct pipe_shader_buffer *buffer = &nv50->buffers[gmem->slot];
   struct nv04_resource *res = nv04_resource(buffer->buffer);
   PUSH_DATAh(push, res->address + buffer->buffer_offset);
   PUSH_DATA (push, res->address + buffer->buffer_offset);
   PUSH_DATA (push, 0); /* pitch? */
   PUSH_DATA (push, ALIGN(buffer->buffer_size, 256) - 1);
   PUSH_DATA (push, NV50_COMPUTE_GLOBAL_MODE_LINEAR);
   BCTX_REFN(nv50->bufctx_cp, CP_BUF, res, RDWR);
   util_range_add(&res->base, &res->valid_buffer_range,
                        PUSH_SPACE(push, 1 + 3);
   BEGIN_NV04(push, NV50_CP(CB_ADDR), 1);
   PUSH_DATA (push, NV50_CB_AUX_BUF_INFO(i) << (8 - 2) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_CP(CB_DATA(0)), 1);
      } else if (gmem->valid && gmem->image && nv50->images[gmem->slot].resource) {
                                    address = res->address;
   if (res->base.target == PIPE_BUFFER) {
                                    PUSH_DATAh(push, address);
   PUSH_DATA (push, address);
   PUSH_DATA (push, 0); /* pitch? */
   PUSH_DATA (push, ALIGN(view->u.buf.size, 0x100) - 1);
      } else {
      struct nv50_miptree *mt = nv50_miptree(view->resource);
   struct nv50_miptree_level *lvl = &mt->level[view->u.tex.level];
                  if (mt->layout_3d) {
      address += nv50_mt_zslice_offset(mt, view->u.tex.level, 0);
      } else {
      address += mt->layer_stride * z;
                     PUSH_DATAh(push, address);
   PUSH_DATA (push, address);
   if (mt->layout_3d) {
      // We have to adjust the size of the 3d surface to be
   // accessible within 2d limits. The size of each z tile goes
   // into the x direction, while the number of z tiles goes into
   // the y direction.
   const unsigned nby = util_format_get_nblocksy(view->format, height);
   const unsigned tsy = NV50_TILE_SIZE_Y(lvl->tile_mode);
   const unsigned tsz = NV50_TILE_SIZE_Z(lvl->tile_mode);
   const unsigned pitch = lvl->pitch * tsz;
   const unsigned maxy = align(nby, tsy) * align(depth, tsz) >> NV50_TILE_SHIFT_Z(lvl->tile_mode);
   PUSH_DATA (push, pitch * tsy);
   PUSH_DATA (push, (maxy - 1) << 16 | (pitch - 1));
      } else if (nouveau_bo_memtype(res->bo)) {
      PUSH_DATA (push, lvl->pitch * NV50_TILE_SIZE_Y(lvl->tile_mode));
   PUSH_DATA (push, (max_size / lvl->pitch - 1) << 16 | (lvl->pitch - 1));
      } else {
      PUSH_DATA (push, lvl->pitch);
   PUSH_DATA (push, align(lvl->pitch * height, 0x100) - 1);
                           PUSH_SPACE(push, 12 + 3);
   BEGIN_NV04(push, NV50_CP(CB_ADDR), 1);
   PUSH_DATA (push, NV50_CB_AUX_BUF_INFO(i) << (8 - 2) | NV50_CB_AUX);
   BEGIN_NI04(push, NV50_CP(CB_DATA(0)), 12);
      } else {
      PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
            }
      static void
   nv50_compute_validate_globals(struct nv50_context *nv50)
   {
               for (i = 0; i < nv50->global_residents.size / sizeof(struct pipe_resource *);
      ++i) {
   struct pipe_resource *res = *util_dynarray_element(
         if (res)
      nv50_add_bufctx_resident(nv50->bufctx_cp, NV50_BIND_CP_GLOBAL,
      }
      static struct nv50_state_validate
   validate_list_cp[] = {
      { nv50_compprog_validate,              NV50_NEW_CP_PROGRAM     },
   { nv50_compute_validate_constbufs,     NV50_NEW_CP_CONSTBUF    },
   { nv50_compute_validate_surfaces,      NV50_NEW_CP_SURFACES |
               { nv50_compute_validate_textures,      NV50_NEW_CP_TEXTURES    },
   { nv50_compute_validate_samplers,      NV50_NEW_CP_SAMPLERS    },
      };
      static bool
   nv50_state_validate_cp(struct nv50_context *nv50, uint32_t mask)
   {
               /* TODO: validate textures, samplers, surfaces */
   ret = nv50_state_validate(nv50, mask, validate_list_cp,
                  if (unlikely(nv50->state.flushed))
            }
      static void
   nv50_compute_upload_input(struct nv50_context *nv50, const uint32_t *input)
   {
      struct nv50_screen *screen = nv50->screen;
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
            BEGIN_NV04(push, NV50_CP(USER_PARAM_COUNT), 1);
            if (size) {
      struct nouveau_mm_allocation *mm;
   struct nouveau_bo *bo = NULL;
            mm = nouveau_mm_allocate(screen->base.mm_GART, size, &bo, &offset);
            BO_MAP(&screen->base, bo, 0, nv50->base.client);
            nouveau_bufctx_refn(nv50->bufctx, 0, bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   nouveau_pushbuf_bufctx(push, nv50->bufctx);
                     BEGIN_NV04(push, NV50_CP(USER_PARAM(1)), size / 4);
            nouveau_fence_work(nv50->base.fence, nouveau_mm_free_work, mm);
   nouveau_bo_ref(NULL, &bo);
         }
      void
   nv50_launch_grid(struct pipe_context *pipe, const struct pipe_grid_info *info)
   {
      struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   unsigned block_size = info->block[0] * info->block[1] * info->block[2];
   struct nv50_program *cp = nv50->compprog;
            simple_mtx_lock(&nv50->screen->state_lock);
   ret = !nv50_state_validate_cp(nv50, ~0);
   if (ret) {
      NOUVEAU_ERR("Failed to launch grid !\n");
                        BEGIN_NV04(push, NV50_CP(CP_START_ID), 1);
            int shared_size = cp->cp.smem_size + info->variable_shared_mem + cp->parm_size + 0x14;
   BEGIN_NV04(push, NV50_CP(SHARED_SIZE), 1);
   PUSH_DATA (push, align(shared_size, 0x40));
   BEGIN_NV04(push, NV50_CP(CP_REG_ALLOC_TEMP), 1);
            /* no indirect support - just read the parameters out */
   uint32_t grid[3];
   if (unlikely(info->indirect)) {
      pipe_buffer_read(pipe, info->indirect, info->indirect_offset,
      } else {
                  /* grid/block setup */
   BEGIN_NV04(push, NV50_CP(BLOCKDIM_XY), 2);
   PUSH_DATA (push, info->block[1] << 16 | info->block[0]);
   PUSH_DATA (push, info->block[2]);
   BEGIN_NV04(push, NV50_CP(BLOCK_ALLOC), 1);
   PUSH_DATA (push, 1 << 16 | block_size);
   BEGIN_NV04(push, NV50_CP(BLOCKDIM_LATCH), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_CP(GRIDDIM), 1);
   PUSH_DATA (push, grid[1] << 16 | grid[0]);
   BEGIN_NV04(push, NV50_CP(GRIDID), 1);
            for (int i = 0; i < grid[2]; i++) {
      BEGIN_NV04(push, NV50_CP(USER_PARAM(0)), 1);
            /* kernel launching */
   BEGIN_NV04(push, NV50_CP(LAUNCH), 1);
               BEGIN_NV04(push, SUBC_CP(NV50_GRAPH_SERIALIZE), 1);
            /* bind a compute shader clobbers fragment shader state */
            nv50->compute_invocations += info->block[0] * info->block[1] * info->block[2] *
         out:
      PUSH_KICK(push);
      }
