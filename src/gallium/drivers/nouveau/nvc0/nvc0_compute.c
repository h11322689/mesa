   /*
   * Copyright 2013 Nouveau Project
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
   * Authors: Christoph Bumiller, Samuel Pitoiset
   */
      #include "nvc0/nvc0_context.h"
      #include "nvc0/nvc0_compute.xml.h"
      int
   nvc0_screen_compute_setup(struct nvc0_screen *screen,
         {
               BEGIN_NVC0(push, SUBC_CP(NV01_SUBCHAN_OBJECT), 1);
            /* hardware limit */
   BEGIN_NVC0(push, NVC0_CP(MP_LIMIT), 1);
   PUSH_DATA (push, screen->mp_count);
   BEGIN_NVC0(push, NVC0_CP(CALL_LIMIT_LOG), 1);
            BEGIN_NVC0(push, SUBC_CP(0x02a0), 1);
            /* global memory setup */
   BEGIN_NVC0(push, SUBC_CP(0x02c4), 1);
   PUSH_DATA (push, 0);
   BEGIN_NIC0(push, NVC0_CP(GLOBAL_BASE), 0x100);
   for (i = 0; i <= 0xff; i++)
         BEGIN_NVC0(push, SUBC_CP(0x02c4), 1);
            /* local memory and cstack setup */
   BEGIN_NVC0(push, NVC0_CP(TEMP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->offset);
   BEGIN_NVC0(push, NVC0_CP(TEMP_SIZE_HIGH), 2);
   PUSH_DATAh(push, screen->tls->size);
   PUSH_DATA (push, screen->tls->size);
   BEGIN_NVC0(push, NVC0_CP(WARP_TEMP_ALLOC), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_CP(LOCAL_BASE), 1);
            /* shared memory setup */
   BEGIN_NVC0(push, NVC0_CP(CACHE_SPLIT), 1);
   PUSH_DATA (push, NVC0_COMPUTE_CACHE_SPLIT_48K_SHARED_16K_L1);
   BEGIN_NVC0(push, NVC0_CP(SHARED_BASE), 1);
   PUSH_DATA (push, 0xfe << 24);
   BEGIN_NVC0(push, NVC0_CP(SHARED_SIZE), 1);
            /* code segment setup */
   BEGIN_NVC0(push, NVC0_CP(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
            /* textures */
   BEGIN_NVC0(push, NVC0_CP(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
            /* samplers */
   BEGIN_NVC0(push, NVC0_CP(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
            /* MS sample coordinate offsets */
   BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(5));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(5));
   BEGIN_1IC0(push, NVC0_CP(CB_POS), 1 + 2 * 8);
   PUSH_DATA (push, NVC0_CB_AUX_MS_INFO);
   PUSH_DATA (push, 0); /* 0 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1); /* 1 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0); /* 2 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 1); /* 3 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 2); /* 4 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 3); /* 5 */
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 2); /* 6 */
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 3); /* 7 */
               }
      static void
   nvc0_compute_validate_samplers(struct nvc0_context *nvc0)
   {
      bool need_flush = nvc0_validate_tsc(nvc0, 5);
   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_CP(TSC_FLUSH), 1);
               /* Invalidate all 3D samplers because they are aliased. */
   for (int s = 0; s < 5; s++)
            }
      static void
   nvc0_compute_validate_textures(struct nvc0_context *nvc0)
   {
      bool need_flush = nvc0_validate_tic(nvc0, 5);
   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVC0_CP(TIC_FLUSH), 1);
               /* Invalidate all 3D textures because they are aliased. */
   for (int s = 0; s < 5; s++) {
      for (int i = 0; i < nvc0->num_textures[s]; i++)
            }
      }
      static inline void
   nvc0_compute_invalidate_constbufs(struct nvc0_context *nvc0)
   {
               /* Invalidate all 3D constbufs because they are aliased with COMPUTE. */
   for (s = 0; s < 5; s++) {
      nvc0->constbuf_dirty[s] |= nvc0->constbuf_valid[s];
      }
      }
      static void
   nvc0_compute_validate_constbufs(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            while (nvc0->constbuf_dirty[s]) {
      int i = ffs(nvc0->constbuf_dirty[s]) - 1;
            if (nvc0->constbuf[s][i].user) {
      struct nouveau_bo *bo = nvc0->screen->uniform_bo;
   const unsigned base = NVC0_CB_USR_INFO(s);
   const unsigned size = nvc0->constbuf[s][0].size;
                                    BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_MAX_CONSTBUF_SIZE);
   PUSH_DATAh(push, bo->offset + base);
   PUSH_DATA (push, bo->offset + base);
   BEGIN_NVC0(push, NVC0_CP(CB_BIND), 1);
      }
   nvc0_cb_bo_push(&nvc0->base, bo, NV_VRAM_DOMAIN(&nvc0->screen->base),
            } else {
      struct nv04_resource *res =
         if (res) {
      BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, nvc0->constbuf[s][i].size);
   PUSH_DATAh(push, res->address + nvc0->constbuf[s][i].offset);
   PUSH_DATA (push, res->address + nvc0->constbuf[s][i].offset);
                              } else {
      BEGIN_NVC0(push, NVC0_CP(CB_BIND), 1);
      }
   if (i == 0)
                           BEGIN_NVC0(push, NVC0_CP(FLUSH), 1);
      }
      static void
   nvc0_compute_validate_driverconst(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(5));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(5));
   BEGIN_NVC0(push, NVC0_CP(CB_BIND), 1);
               }
      static void
   nvc0_compute_validate_buffers(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   const int s = 5;
            BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(s));
   BEGIN_1IC0(push, NVC0_CP(CB_POS), 1 + 4 * NVC0_MAX_BUFFERS);
            for (i = 0; i < NVC0_MAX_BUFFERS; i++) {
      if (nvc0->buffers[s][i].buffer) {
      struct nv04_resource *res =
         PUSH_DATA (push, res->address + nvc0->buffers[s][i].buffer_offset);
   PUSH_DATAh(push, res->address + nvc0->buffers[s][i].buffer_offset);
   PUSH_DATA (push, nvc0->buffers[s][i].buffer_size);
   PUSH_DATA (push, 0);
   BCTX_REFN(nvc0->bufctx_cp, CP_BUF, res, RDWR);
   util_range_add(&res->base, &res->valid_buffer_range,
                  } else {
      PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0);
            }
      void
   nvc0_compute_validate_globals(struct nvc0_context *nvc0)
   {
               for (i = 0; i < nvc0->global_residents.size / sizeof(struct pipe_resource *);
      ++i) {
   struct pipe_resource *res = *util_dynarray_element(
         if (res)
      nvc0_add_resident(nvc0->bufctx_cp, NVC0_BIND_CP_GLOBAL,
      }
      static inline void
   nvc0_compute_invalidate_surfaces(struct nvc0_context *nvc0, const int s)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            for (i = 0; i < NVC0_MAX_IMAGES; ++i) {
      if (s == 5)
         else
         PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0);
   PUSH_DATA(push, 0x14000);
         }
      static void
   nvc0_compute_validate_surfaces(struct nvc0_context *nvc0)
   {
      /* TODO: Invalidating both 3D and CP surfaces before validating surfaces for
   * compute is probably not really necessary, but we didn't find any better
   * solutions for now. This fixes some invalidation issues when compute and
   * fragment shaders are used inside the same context. Anyway, we definitely
   * have invalidation issues between 3D and CP for other resources like SSBO
   * and atomic counters. */
   nvc0_compute_invalidate_surfaces(nvc0, 4);
                     /* Invalidate all FRAGMENT images because they are aliased with COMPUTE. */
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_SUF);
   nvc0->dirty_3d |= NVC0_NEW_3D_SURFACES;
      }
      static struct nvc0_state_validate
   validate_list_cp[] = {
      { nvc0_compprog_validate,              NVC0_NEW_CP_PROGRAM     },
   { nvc0_compute_validate_constbufs,     NVC0_NEW_CP_CONSTBUF    },
   { nvc0_compute_validate_driverconst,   NVC0_NEW_CP_DRIVERCONST },
   { nvc0_compute_validate_buffers,       NVC0_NEW_CP_BUFFERS     },
   { nvc0_compute_validate_textures,      NVC0_NEW_CP_TEXTURES    },
   { nvc0_compute_validate_samplers,      NVC0_NEW_CP_SAMPLERS    },
   { nvc0_compute_validate_globals,       NVC0_NEW_CP_GLOBALS     },
      };
      static bool
   nvc0_state_validate_cp(struct nvc0_context *nvc0, uint32_t mask)
   {
               ret = nvc0_state_validate(nvc0, mask, validate_list_cp,
                  if (unlikely(nvc0->state.flushed))
            }
      static void
   nvc0_compute_upload_input(struct nvc0_context *nvc0,
         {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
            if (cp->parm_size) {
      struct nouveau_bo *bo = screen->uniform_bo;
            BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, align(cp->parm_size, 0x100));
   PUSH_DATAh(push, bo->offset + base);
   PUSH_DATA (push, bo->offset + base);
   BEGIN_NVC0(push, NVC0_CP(CB_BIND), 1);
   PUSH_DATA (push, (0 << 8) | 1);
   /* NOTE: size is limited to 4 KiB, which is < NV04_PFIFO_MAX_PACKET_LEN */
   BEGIN_1IC0(push, NVC0_CP(CB_POS), 1 + cp->parm_size / 4);
   PUSH_DATA (push, 0);
                        BEGIN_NVC0(push, NVC0_CP(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(5));
            BEGIN_1IC0(push, NVC0_CP(CB_POS), 1 + 1);
   /* (7) as we only upload work_dim on nvc0, the rest uses special regs */
   PUSH_DATA (push, NVC0_CB_AUX_GRID_INFO(7));
            BEGIN_NVC0(push, NVC0_CP(FLUSH), 1);
      }
      void
   nvc0_launch_grid(struct pipe_context *pipe, const struct pipe_grid_info *info)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *cp = nvc0->compprog;
            simple_mtx_lock(&screen->state_lock);
   ret = !nvc0_state_validate_cp(nvc0, ~0);
   if (ret) {
      NOUVEAU_ERR("Failed to launch grid !\n");
                        BEGIN_NVC0(push, NVC0_CP(CP_START_ID), 1);
            BEGIN_NVC0(push, NVC0_CP(LOCAL_POS_ALLOC), 3);
   PUSH_DATA (push, cp->hdr[1] & 0xfffff0);
   PUSH_DATA (push, 0);
            BEGIN_NVC0(push, NVC0_CP(SHARED_SIZE), 3);
   PUSH_DATA (push, align(cp->cp.smem_size + info->variable_shared_mem, 0x100));
   PUSH_DATA (push, info->block[0] * info->block[1] * info->block[2]);
   PUSH_DATA (push, cp->num_barriers);
   BEGIN_NVC0(push, NVC0_CP(CP_GPR_ALLOC), 1);
            /* launch preliminary setup */
   BEGIN_NVC0(push, NVC0_CP(GRIDID), 1);
   PUSH_DATA (push, 0x1);
   BEGIN_NVC0(push, SUBC_CP(0x036c), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_CP(FLUSH), 1);
            /* block setup */
   BEGIN_NVC0(push, NVC0_CP(BLOCKDIM_YX), 2);
   PUSH_DATA (push, (info->block[1] << 16) | info->block[0]);
            PUSH_SPACE_EX(push, 32, 2, 1);
            if (unlikely(info->indirect)) {
      struct nv04_resource *res = nv04_resource(info->indirect);
   uint32_t offset = res->offset + info->indirect_offset;
            PUSH_REF1(push, res->bo, NOUVEAU_BO_RD | res->domain);
   PUSH_DATA(push, NVC0_FIFO_PKHDR_1I(1, macro, 3));
   nouveau_pushbuf_data(push, res->bo, offset,
      } else {
      /* grid setup */
   BEGIN_NVC0(push, NVC0_CP(GRIDDIM_YX), 2);
   PUSH_DATA (push, (info->grid[1] << 16) | info->grid[0]);
            /* kernel launching */
   BEGIN_NVC0(push, NVC0_CP(COMPUTE_BEGIN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_CP(0x0a08), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_CP(LAUNCH), 1);
   PUSH_DATA (push, 0x1000);
   BEGIN_NVC0(push, NVC0_CP(COMPUTE_END), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_CP(0x0360), 1);
               /* TODO: Not sure if this is really necessary. */
   nvc0_compute_invalidate_surfaces(nvc0, 5);
   nouveau_bufctx_reset(nvc0->bufctx_cp, NVC0_BIND_CP_SUF);
   nvc0->dirty_cp |= NVC0_NEW_CP_SURFACES;
                  out:
      PUSH_KICK(push);
      }
      static void
   nvc0_compute_update_indirect_invocations(struct nvc0_context *nvc0,
            struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *res = nv04_resource(info->indirect);
            PUSH_SPACE_EX(push, 16, 0, 8);
   PUSH_REF1(push, res->bo, NOUVEAU_BO_RD | res->domain);
   BEGIN_1IC0(push, NVC0_3D(MACRO_COMPUTE_COUNTER), 7);
   PUSH_DATA(push, 6);
   PUSH_DATA(push, info->block[0]);
   PUSH_DATA(push, info->block[1]);
   PUSH_DATA(push, info->block[2]);
   nouveau_pushbuf_data(push, res->bo, offset,
      }
      void
   nvc0_update_compute_invocations_counter(struct nvc0_context *nvc0,
            if (unlikely(info->indirect)) {
         } else {
      uint64_t invocations = info->block[0] * info->block[1] * info->block[2];
   invocations *= info->grid[0] * info->grid[1] * info->grid[2];
         }
