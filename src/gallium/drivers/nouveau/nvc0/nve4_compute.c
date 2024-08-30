   /*
   * Copyright 2012 Nouveau Project
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
   * Authors: Christoph Bumiller
   */
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nve4_compute.h"
      #include "nv50_ir_driver.h"
      #include "drf.h"
   #include "qmd.h"
   #include "cla0c0qmd.h"
   #include "clc0c0qmd.h"
   #include "clc3c0qmd.h"
      #define NVA0C0_QMDV00_06_VAL_SET(p,a...) NVVAL_MW_SET((p), NVA0C0, QMDV00_06, ##a)
   #define NVA0C0_QMDV00_06_DEF_SET(p,a...) NVDEF_MW_SET((p), NVA0C0, QMDV00_06, ##a)
   #define NVC0C0_QMDV02_01_VAL_SET(p,a...) NVVAL_MW_SET((p), NVC0C0, QMDV02_01, ##a)
   #define NVC0C0_QMDV02_01_DEF_SET(p,a...) NVDEF_MW_SET((p), NVC0C0, QMDV02_01, ##a)
   #define NVC3C0_QMDV02_02_VAL_SET(p,a...) NVVAL_MW_SET((p), NVC3C0, QMDV02_02, ##a)
   #define NVC3C0_QMDV02_02_DEF_SET(p,a...) NVDEF_MW_SET((p), NVC3C0, QMDV02_02, ##a)
      int
   nve4_screen_compute_setup(struct nvc0_screen *screen,
         {
      int i;
   uint32_t obj_class = screen->compute->oclass;
            BEGIN_NVC0(push, SUBC_CP(NV01_SUBCHAN_OBJECT), 1);
            BEGIN_NVC0(push, NVE4_CP(TEMP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->offset);
   /* No idea why there are 2. Divide size by 2 to be safe.
   * Actually this might be per-MP TEMP size and looks like I'm only using
   * 2 MPs instead of all 8.
   */
   BEGIN_NVC0(push, NVE4_CP(MP_TEMP_SIZE_HIGH(0)), 3);
   PUSH_DATAh(push, screen->tls->size / screen->mp_count);
   PUSH_DATA (push, (screen->tls->size / screen->mp_count) & ~0x7fff);
   PUSH_DATA (push, 0xff);
   if (obj_class < GV100_COMPUTE_CLASS) {
      BEGIN_NVC0(push, NVE4_CP(MP_TEMP_SIZE_HIGH(1)), 3);
   PUSH_DATAh(push, screen->tls->size / screen->mp_count);
   PUSH_DATA (push, (screen->tls->size / screen->mp_count) & ~0x7fff);
               /* Unified address space ? Who needs that ? Certainly not OpenCL.
   *
   * FATAL: Buffers with addresses inside [0x1000000, 0x3000000] will NOT be
   *  accessible. We cannot prevent that at the moment, so expect failure.
   */
   if (obj_class < GV100_COMPUTE_CLASS) {
      BEGIN_NVC0(push, NVE4_CP(LOCAL_BASE), 1);
   PUSH_DATA (push, 0xff << 24);
   BEGIN_NVC0(push, NVE4_CP(SHARED_BASE), 1);
            BEGIN_NVC0(push, NVE4_CP(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
      } else {
      BEGIN_NVC0(push, SUBC_CP(0x2a0), 2);
   PUSH_DATAh(push, 0xfeULL << 24);
   PUSH_DATA (push, 0xfeULL << 24);
   BEGIN_NVC0(push, SUBC_CP(0x7b0), 2);
   PUSH_DATAh(push, 0xffULL << 24);
               BEGIN_NVC0(push, SUBC_CP(0x0310), 1);
            /* NOTE: these do not affect the state used by the 3D object */
   BEGIN_NVC0(push, NVE4_CP(TIC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset);
   PUSH_DATA (push, screen->txc->offset);
   PUSH_DATA (push, NVC0_TIC_MAX_ENTRIES - 1);
   BEGIN_NVC0(push, NVE4_CP(TSC_ADDRESS_HIGH), 3);
   PUSH_DATAh(push, screen->txc->offset + 65536);
   PUSH_DATA (push, screen->txc->offset + 65536);
            if (obj_class >= NVF0_COMPUTE_CLASS) {
      /* The blob calls GK110_COMPUTE.FIRMWARE[0x6], along with the args (0x1)
   * passed with GK110_COMPUTE.GRAPH.SCRATCH[0x2]. This is currently
   * disabled because our firmware doesn't support these commands and the
   * GPU hangs if they are used. */
   BEGIN_NIC0(push, SUBC_CP(0x0248), 64);
   for (i = 63; i >= 0; i--)
                     BEGIN_NVC0(push, NVE4_CP(TEX_CB_INDEX), 1);
            /* Disabling this UNK command avoid a read fault when using texelFetch()
   * from a compute shader for weird reasons.
   if (obj_class == NVF0_COMPUTE_CLASS)
                           /* MS sample coordinate offsets: these do not work with _ALT modes ! */
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_MS_INFO);
   PUSH_DATA (push, address + NVC0_CB_AUX_MS_INFO);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 64);
   PUSH_DATA (push, 1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 17);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
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
         #ifdef NOUVEAU_NVE4_MP_TRAP_HANDLER
      BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->parm->offset + NVE4_CP_INPUT_TRAP_INFO_PTR);
   PUSH_DATA (push, screen->parm->offset + NVE4_CP_INPUT_TRAP_INFO_PTR);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 28);
   PUSH_DATA (push, 1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 8);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, screen->parm->offset + NVE4_CP_PARAM_TRAP_INFO);
   PUSH_DATAh(push, screen->parm->offset + NVE4_CP_PARAM_TRAP_INFO);
   PUSH_DATA (push, screen->tls->offset);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->size / 2); /* MP TEMP block size */
   PUSH_DATA (push, screen->tls->size / 2 / 64); /* warp TEMP block size */
      #endif
         BEGIN_NVC0(push, NVE4_CP(FLUSH), 1);
               }
      static void
   gm107_compute_validate_surfaces(struct nvc0_context *nvc0,
         {
      struct nv04_resource *res = nv04_resource(view->resource);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_bo *txc = nvc0->screen->txc;
   struct nv50_tic_entry *tic;
   uint64_t address;
                     res = nv04_resource(tic->pipe.texture);
            if (tic->id < 0) {
               /* upload the texture view */
   PUSH_SPACE(push, 16);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, txc->offset + (tic->id * 32));
   PUSH_DATA (push, txc->offset + (tic->id * 32));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 32);
   PUSH_DATA (push, 1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 9);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
            BEGIN_NIC0(push, NVE4_CP(TIC_FLUSH), 1);
      } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      BEGIN_NIC0(push, NVE4_CP(TEX_CACHE_CTL), 1);
      }
            res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
                              /* upload the texture handle */
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_TEX_INFO(slot + 32));
   PUSH_DATA (push, address + NVC0_CB_AUX_TEX_INFO(slot + 32));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 4);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 2);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
            BEGIN_NVC0(push, NVE4_CP(FLUSH), 1);
      }
      static void
   nve4_compute_validate_surfaces(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint64_t address;
   const int s = 5;
            if (!nvc0->images_dirty[s])
                     for (i = 0; i < NVC0_MAX_IMAGES; ++i) {
               BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_SU_INFO(i));
   PUSH_DATA (push, address + NVC0_CB_AUX_SU_INFO(i));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 16 * 4);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + 16);
            if (view->resource) {
               if (res->base.target == PIPE_BUFFER) {
      if (view->access & PIPE_IMAGE_ACCESS_WRITE)
                              if (nvc0->screen->base.class_3d >= GM107_3D_CLASS)
      } else {
      for (j = 0; j < 16; j++)
            }
      /* Thankfully, textures with samplers follow the normal rules. */
   static void
   nve4_compute_validate_samplers(struct nvc0_context *nvc0)
   {
      bool need_flush = nve4_validate_tsc(nvc0, 5);
   if (need_flush) {
      BEGIN_NVC0(nvc0->base.pushbuf, NVE4_CP(TSC_FLUSH), 1);
               /* Invalidate all 3D samplers because they are aliased. */
   for (int s = 0; s < 5; s++)
            }
      /* (Code duplicated at bottom for various non-convincing reasons.
   *  E.g. we might want to use the COMPUTE subchannel to upload TIC/TSC
   *  entries to avoid a subchannel switch.
   *  Same for texture cache flushes.
   *  Also, the bufctx differs, and more IFs in the 3D version looks ugly.)
   */
   static void nve4_compute_validate_textures(struct nvc0_context *);
      static void
   nve4_compute_set_tex_handles(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   uint64_t address;
   const unsigned s = nvc0_shader_stage(PIPE_SHADER_COMPUTE);
   unsigned i, n;
            if (!dirty)
         i = ffs(dirty) - 1;
   n = util_logbase2(dirty) + 1 - i;
                     BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_TEX_INFO(i));
   PUSH_DATA (push, address + NVC0_CB_AUX_TEX_INFO(i));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, n * 4);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + n);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
            BEGIN_NVC0(push, NVE4_CP(FLUSH), 1);
            nvc0->textures_dirty[s] = 0;
      }
      static void
   nve4_compute_validate_constbufs(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            while (nvc0->constbuf_dirty[s]) {
      int i = ffs(nvc0->constbuf_dirty[s]) - 1;
            if (nvc0->constbuf[s][i].user) {
      struct nouveau_bo *bo = nvc0->screen->uniform_bo;
   const unsigned base = NVC0_CB_USR_INFO(s);
   const unsigned size = nvc0->constbuf[s][0].size;
                  BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, bo->offset + base);
   PUSH_DATA (push, bo->offset + base);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, size);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + (size / 4));
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
      }
   else {
      struct nv04_resource *res =
         if (res) {
                     /* constbufs above 0 will are fetched via ubo info in the shader */
   if (i > 0) {
      BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_UBO_INFO(i - 1));
   PUSH_DATA (push, address + NVC0_CB_AUX_UBO_INFO(i - 1));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 4 * 4);
                        PUSH_DATA (push, res->address + nvc0->constbuf[s][i].offset);
   PUSH_DATAh(push, res->address + nvc0->constbuf[s][i].offset);
                     BCTX_REFN(nvc0->bufctx_cp, CP_CB(i), res, RD);
                     BEGIN_NVC0(push, NVE4_CP(FLUSH), 1);
      }
      static void
   nve4_compute_validate_buffers(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   uint64_t address;
   const int s = 5;
                     BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_BUF_INFO(0));
   PUSH_DATA (push, address + NVC0_CB_AUX_BUF_INFO(0));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 4 * NVC0_MAX_BUFFERS * 4);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + 4 * NVC0_MAX_BUFFERS);
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
      static struct nvc0_state_validate
   validate_list_cp[] = {
      { nvc0_compprog_validate,              NVC0_NEW_CP_PROGRAM     },
   { nve4_compute_validate_textures,      NVC0_NEW_CP_TEXTURES    },
   { nve4_compute_validate_samplers,      NVC0_NEW_CP_SAMPLERS    },
   { nve4_compute_set_tex_handles,        NVC0_NEW_CP_TEXTURES |
         { nve4_compute_validate_surfaces,      NVC0_NEW_CP_SURFACES    },
   { nvc0_compute_validate_globals,       NVC0_NEW_CP_GLOBALS     },
   { nve4_compute_validate_buffers,       NVC0_NEW_CP_BUFFERS     },
      };
      static bool
   nve4_state_validate_cp(struct nvc0_context *nvc0, uint32_t mask)
   {
               ret = nvc0_state_validate(nvc0, mask, validate_list_cp,
                  if (unlikely(nvc0->state.flushed))
            }
      static void
   nve4_compute_upload_input(struct nvc0_context *nvc0,
         {
      struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *cp = nvc0->compprog;
                     if (cp->parm_size) {
      BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_USR_INFO(5));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_USR_INFO(5));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, cp->parm_size);
   PUSH_DATA (push, 0x1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + DIV_ROUND_UP(cp->parm_size, 4));
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
      }
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, address + NVC0_CB_AUX_GRID_INFO(0));
   PUSH_DATA (push, address + NVC0_CB_AUX_GRID_INFO(0));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 8 * 4);
            if (unlikely(info->indirect)) {
      struct nv04_resource *res = nv04_resource(info->indirect);
            PUSH_SPACE_EX(push, 32, 0, 1);
            BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + 8);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
   PUSH_DATAp(push, info->block, 3);
   nouveau_pushbuf_data(push, res->bo, offset,
      } else {
      BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + 8);
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x20 << 1));
   PUSH_DATAp(push, info->block, 3);
      }
   PUSH_DATA (push, 0);
            BEGIN_NVC0(push, NVE4_CP(FLUSH), 1);
      }
      static inline void
   gp100_cp_launch_desc_set_cb(uint32_t *qmd, unsigned index,
         {
               assert(index < 8);
            NVC0C0_QMDV02_01_VAL_SET(qmd, CONSTANT_BUFFER_ADDR_LOWER, index, address);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CONSTANT_BUFFER_ADDR_UPPER, index, address >> 32);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CONSTANT_BUFFER_SIZE_SHIFTED4, index,
            }
      static inline void
   nve4_cp_launch_desc_set_cb(uint32_t *qmd, unsigned index, struct nouveau_bo *bo,
         {
               assert(index < 8);
            NVA0C0_QMDV00_06_VAL_SET(qmd, CONSTANT_BUFFER_ADDR_LOWER, index, address);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CONSTANT_BUFFER_ADDR_UPPER, index, address >> 32);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CONSTANT_BUFFER_SIZE, index, size);
      }
      static void
   nve4_compute_setup_buf_cb(struct nvc0_context *nvc0, bool gp100, void *desc)
   {
      // only user constant buffers 0-6 can be put in the descriptor, the rest are
   // loaded through global memory
   for (int i = 0; i <= 6; i++) {
      if (nvc0->constbuf[5][i].user || !nvc0->constbuf[5][i].u.buf)
            struct nv04_resource *res =
            uint32_t base = res->offset + nvc0->constbuf[5][i].offset;
   uint32_t size = nvc0->constbuf[5][i].size;
   if (gp100)
         else
               // there is no need to do FLUSH(NVE4_COMPUTE_FLUSH_CB) because
      }
      static void
   nve4_compute_setup_launch_desc(struct nvc0_context *nvc0, uint32_t *qmd,
         {
      const struct nvc0_screen *screen = nvc0->screen;
   const struct nvc0_program *cp = nvc0->compprog;
            NVA0C0_QMDV00_06_DEF_SET(qmd, INVALIDATE_TEXTURE_HEADER_CACHE, TRUE);
   NVA0C0_QMDV00_06_DEF_SET(qmd, INVALIDATE_TEXTURE_SAMPLER_CACHE, TRUE);
   NVA0C0_QMDV00_06_DEF_SET(qmd, INVALIDATE_TEXTURE_DATA_CACHE, TRUE);
   NVA0C0_QMDV00_06_DEF_SET(qmd, INVALIDATE_SHADER_DATA_CACHE, TRUE);
   NVA0C0_QMDV00_06_DEF_SET(qmd, INVALIDATE_SHADER_CONSTANT_CACHE, TRUE);
   NVA0C0_QMDV00_06_DEF_SET(qmd, RELEASE_MEMBAR_TYPE, FE_SYSMEMBAR);
   NVA0C0_QMDV00_06_DEF_SET(qmd, CWD_MEMBAR_TYPE, L1_SYSMEMBAR);
   NVA0C0_QMDV00_06_DEF_SET(qmd, API_VISIBLE_CALL_LIMIT, NO_CHECK);
                     NVA0C0_QMDV00_06_VAL_SET(qmd, CTA_RASTER_WIDTH, info->grid[0]);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CTA_RASTER_HEIGHT, info->grid[1]);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CTA_RASTER_DEPTH, info->grid[2]);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CTA_THREAD_DIMENSION0, info->block[0]);
   NVA0C0_QMDV00_06_VAL_SET(qmd, CTA_THREAD_DIMENSION1, info->block[1]);
            NVA0C0_QMDV00_06_VAL_SET(qmd, SHARED_MEMORY_SIZE, align(shared_size, 0x100));
   NVA0C0_QMDV00_06_VAL_SET(qmd, SHADER_LOCAL_MEMORY_LOW_SIZE, cp->hdr[1] & 0xfffff0);
   NVA0C0_QMDV00_06_VAL_SET(qmd, SHADER_LOCAL_MEMORY_HIGH_SIZE, 0);
            if (shared_size > (32 << 10))
      NVA0C0_QMDV00_06_DEF_SET(qmd, L1_CONFIGURATION,
      else
   if (shared_size > (16 << 10))
      NVA0C0_QMDV00_06_DEF_SET(qmd, L1_CONFIGURATION,
      else
      NVA0C0_QMDV00_06_DEF_SET(qmd, L1_CONFIGURATION,
         NVA0C0_QMDV00_06_VAL_SET(qmd, REGISTER_COUNT, cp->num_gprs);
            // Only bind user uniforms and the driver constant buffer through the
   // launch descriptor because UBOs are sticked to the driver cb to avoid the
   // limitation of 8 CBs.
   if (nvc0->constbuf[5][0].user || cp->parm_size) {
      nve4_cp_launch_desc_set_cb(qmd, 0, screen->uniform_bo,
            // Later logic will attempt to bind a real buffer at position 0. That
   // should not happen if we've bound a user buffer.
      }
   nve4_cp_launch_desc_set_cb(qmd, 7, screen->uniform_bo,
               }
      static void
   gp100_compute_setup_launch_desc(struct nvc0_context *nvc0, uint32_t *qmd,
         {
      const struct nvc0_screen *screen = nvc0->screen;
   const struct nvc0_program *cp = nvc0->compprog;
            NVC0C0_QMDV02_01_VAL_SET(qmd, SM_GLOBAL_CACHING_ENABLE, 1);
   NVC0C0_QMDV02_01_DEF_SET(qmd, RELEASE_MEMBAR_TYPE, FE_SYSMEMBAR);
   NVC0C0_QMDV02_01_DEF_SET(qmd, CWD_MEMBAR_TYPE, L1_SYSMEMBAR);
                     NVC0C0_QMDV02_01_VAL_SET(qmd, CTA_RASTER_WIDTH, info->grid[0]);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CTA_RASTER_HEIGHT, info->grid[1]);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CTA_RASTER_DEPTH, info->grid[2]);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CTA_THREAD_DIMENSION0, info->block[0]);
   NVC0C0_QMDV02_01_VAL_SET(qmd, CTA_THREAD_DIMENSION1, info->block[1]);
            NVC0C0_QMDV02_01_VAL_SET(qmd, SHARED_MEMORY_SIZE, align(shared_size, 0x100));
   NVC0C0_QMDV02_01_VAL_SET(qmd, SHADER_LOCAL_MEMORY_LOW_SIZE, cp->hdr[1] & 0xfffff0);
   NVC0C0_QMDV02_01_VAL_SET(qmd, SHADER_LOCAL_MEMORY_HIGH_SIZE, 0);
            NVC0C0_QMDV02_01_VAL_SET(qmd, REGISTER_COUNT, cp->num_gprs);
            // Only bind user uniforms and the driver constant buffer through the
   // launch descriptor because UBOs are sticked to the driver cb to avoid the
   // limitation of 8 CBs.
   if (nvc0->constbuf[5][0].user || cp->parm_size) {
      gp100_cp_launch_desc_set_cb(qmd, 0, screen->uniform_bo,
            // Later logic will attempt to bind a real buffer at position 0. That
   // should not happen if we've bound a user buffer.
      }
   gp100_cp_launch_desc_set_cb(qmd, 7, screen->uniform_bo,
               }
      static int
   gv100_sm_config_smem_size(u32 size)
   {
      if      (size > 64 * 1024) size = 96 * 1024;
   else if (size > 32 * 1024) size = 64 * 1024;
   else if (size > 16 * 1024) size = 32 * 1024;
   else if (size >  8 * 1024) size = 16 * 1024;
   else                       size =  8 * 1024;
      }
      static void
   gv100_compute_setup_launch_desc(struct nvc0_context *nvc0, u32 *qmd,
         {
      struct nvc0_program *cp = nvc0->compprog;
   struct nvc0_screen *screen = nvc0->screen;
   uint64_t entry = screen->text->offset + cp->code_base;
            NVC3C0_QMDV02_02_VAL_SET(qmd, SM_GLOBAL_CACHING_ENABLE, 1);
   NVC3C0_QMDV02_02_DEF_SET(qmd, API_VISIBLE_CALL_LIMIT, NO_CHECK);
   NVC3C0_QMDV02_02_DEF_SET(qmd, SAMPLER_INDEX, INDEPENDENTLY);
   NVC3C0_QMDV02_02_VAL_SET(qmd, SHARED_MEMORY_SIZE, align(shared_size, 0x100));
   NVC3C0_QMDV02_02_VAL_SET(qmd, SHADER_LOCAL_MEMORY_LOW_SIZE, cp->hdr[1] & 0xfffff0);
   NVC3C0_QMDV02_02_VAL_SET(qmd, SHADER_LOCAL_MEMORY_HIGH_SIZE, 0);
   NVC3C0_QMDV02_02_VAL_SET(qmd, MIN_SM_CONFIG_SHARED_MEM_SIZE,
         NVC3C0_QMDV02_02_VAL_SET(qmd, MAX_SM_CONFIG_SHARED_MEM_SIZE,
         NVC3C0_QMDV02_02_VAL_SET(qmd, QMD_VERSION, 2);
   NVC3C0_QMDV02_02_VAL_SET(qmd, QMD_MAJOR_VERSION, 2);
   NVC3C0_QMDV02_02_VAL_SET(qmd, TARGET_SM_CONFIG_SHARED_MEM_SIZE,
            NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_RASTER_WIDTH, info->grid[0]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_RASTER_HEIGHT, info->grid[1]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_RASTER_DEPTH, info->grid[2]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_THREAD_DIMENSION0, info->block[0]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_THREAD_DIMENSION1, info->block[1]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, CTA_THREAD_DIMENSION2, info->block[2]);
   NVC3C0_QMDV02_02_VAL_SET(qmd, REGISTER_COUNT_V, cp->num_gprs);
            // Only bind user uniforms and the driver constant buffer through the
   // launch descriptor because UBOs are sticked to the driver cb to avoid the
   // limitation of 8 CBs.
   if (nvc0->constbuf[5][0].user || cp->parm_size) {
      gp100_cp_launch_desc_set_cb(qmd, 0, screen->uniform_bo,
            // Later logic will attempt to bind a real buffer at position 0. That
   // should not happen if we've bound a user buffer.
      }
   gp100_cp_launch_desc_set_cb(qmd, 7, screen->uniform_bo,
                     NVC3C0_QMDV02_02_VAL_SET(qmd, PROGRAM_ADDRESS_LOWER, entry & 0xffffffff);
      }
      static inline void *
   nve4_compute_alloc_launch_desc(struct nouveau_context *nv,
         {
      uint8_t *ptr = nouveau_scratch_get(nv, 512, pgpuaddr, pbo);
   if (!ptr)
         if (*pgpuaddr & 255) {
      unsigned adj = 256 - (*pgpuaddr & 255);
   ptr += adj;
      }
   memset(ptr, 0x00, 256);
      }
      static void
   nve4_upload_indirect_desc(struct nouveau_pushbuf *push,
               {
      BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, gpuaddr);
   PUSH_DATA (push, gpuaddr);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, length);
            PUSH_SPACE_EX(push, 32, 0, 1);
            BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + (length / 4));
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x08 << 1));
   nouveau_pushbuf_data(push, res->bo, bo_offset,
      }
      void
   nve4_launch_grid(struct pipe_context *pipe, const struct pipe_grid_info *info)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   void *desc;
   uint64_t desc_gpuaddr;
   struct nouveau_bo *desc_bo;
            desc = nve4_compute_alloc_launch_desc(&nvc0->base, &desc_bo, &desc_gpuaddr);
   if (!desc) {
      ret = -1;
      }
   BCTX_REFN_bo(nvc0->bufctx_cp, CP_DESC, NOUVEAU_BO_GART | NOUVEAU_BO_RD,
            list_for_each_entry(struct nvc0_resident, resident, &nvc0->tex_head, list) {
      nvc0_add_resident(nvc0->bufctx_cp, NVC0_BIND_CP_BINDLESS, resident->buf,
               list_for_each_entry(struct nvc0_resident, resident, &nvc0->img_head, list) {
      nvc0_add_resident(nvc0->bufctx_cp, NVC0_BIND_CP_BINDLESS, resident->buf,
               simple_mtx_lock(&screen->state_lock);
   ret = !nve4_state_validate_cp(nvc0, ~0);
   if (ret)
            if (nvc0->screen->compute->oclass >= GV100_COMPUTE_CLASS)
         else
   if (nvc0->screen->compute->oclass >= GP100_COMPUTE_CLASS)
         else
                  #ifndef NDEBUG
      if (debug_get_num_option("NV50_PROG_DEBUG", 0)) {
      debug_printf("Queue Meta Data:\n");
   if (nvc0->screen->compute->oclass >= GV100_COMPUTE_CLASS)
         else
   if (nvc0->screen->compute->oclass >= GP100_COMPUTE_CLASS)
         else
         #endif
         if (unlikely(info->indirect)) {
      struct nv04_resource *res = nv04_resource(info->indirect);
            /* upload the descriptor */
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, desc_gpuaddr);
   PUSH_DATA (push, desc_gpuaddr);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 256);
   PUSH_DATA (push, 1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 1 + (256 / 4));
   PUSH_DATA (push, NVE4_COMPUTE_UPLOAD_EXEC_LINEAR | (0x08 << 1));
            if (nvc0->screen->compute->oclass >= GP100_COMPUTE_CLASS) {
         } else {
      /* overwrite griddim_x and griddim_y as two 32-bits integers even
                  /* overwrite the 16 high bits of griddim_y with griddim_z because
   * we need (z << 16) | x */
                  /* upload descriptor and flush */
   PUSH_SPACE_EX(push, 32, 1, 0);
   PUSH_REF1(push, screen->text, NV_VRAM_DOMAIN(&screen->base) | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, NVE4_CP(LAUNCH_DESC_ADDRESS), 1);
   PUSH_DATA (push, desc_gpuaddr >> 8);
   if (screen->compute->oclass < GA102_COMPUTE_CLASS) {
      BEGIN_NVC0(push, NVE4_CP(LAUNCH), 1);
      } else {
      BEGIN_NIC0(push, SUBC_CP(0x02c0), 2);
   PUSH_DATA (push, 1);
      }
   BEGIN_NVC0(push, SUBC_CP(NV50_GRAPH_SERIALIZE), 1);
                  out_unlock:
      PUSH_KICK(push);
         out:
      if (ret)
         nouveau_scratch_done(&nvc0->base);
   nouveau_bufctx_reset(nvc0->bufctx_cp, NVC0_BIND_CP_DESC);
      }
         #define NVE4_TIC_ENTRY_INVALID 0x000fffff
      static void
   nve4_compute_validate_textures(struct nvc0_context *nvc0)
   {
      struct nouveau_bo *txc = nvc0->screen->txc;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   const unsigned s = 5;
   unsigned i;
   uint32_t commands[2][32];
            for (i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
   struct nv04_resource *res;
            if (!tic) {
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
      }
   res = nv04_resource(tic->pipe.texture);
            if (tic->id < 0) {
               PUSH_SPACE(push, 16);
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_DST_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, txc->offset + (tic->id * 32));
   PUSH_DATA (push, txc->offset + (tic->id * 32));
   BEGIN_NVC0(push, NVE4_CP(UPLOAD_LINE_LENGTH_IN), 2);
   PUSH_DATA (push, 32);
   PUSH_DATA (push, 1);
   BEGIN_1IC0(push, NVE4_CP(UPLOAD_EXEC), 9);
                     } else
   if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
         }
            res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
            nvc0->tex_handles[s][i] &= ~NVE4_TIC_ENTRY_INVALID;
   nvc0->tex_handles[s][i] |= tic->id;
   if (dirty)
      }
   for (; i < nvc0->state.num_textures[s]; ++i) {
      nvc0->tex_handles[s][i] |= NVE4_TIC_ENTRY_INVALID;
               if (n[0]) {
      BEGIN_NIC0(push, NVE4_CP(TIC_FLUSH), n[0]);
      }
   if (n[1]) {
      BEGIN_NIC0(push, NVE4_CP(TEX_CACHE_CTL), n[1]);
                        /* Invalidate all 3D textures because they are aliased. */
   for (int s = 0; s < 5; s++) {
      for (int i = 0; i < nvc0->num_textures[s]; i++)
            }
      }
      #ifdef NOUVEAU_NVE4_MP_TRAP_HANDLER
   static void
   nve4_compute_trap_info(struct nvc0_context *nvc0)
   {
      struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_bo *bo = screen->parm;
   int ret, i;
   volatile struct nve4_mp_trap_info *info;
            ret = BO_MAP(&screen->base, bo, NOUVEAU_BO_RDWR, nvc0->base.client);
   if (ret)
         map = (uint8_t *)bo->map;
            if (info->lock) {
      debug_printf("trapstat = %08x\n", info->trapstat);
   debug_printf("warperr = %08x\n", info->warperr);
   debug_printf("PC = %x\n", info->pc);
   debug_printf("tid = %u %u %u\n",
         debug_printf("ctaid = %u %u %u\n",
         for (i = 0; i <= 63; ++i)
         for (i = 0; i <= 6; ++i)
            }
      }
   #endif
