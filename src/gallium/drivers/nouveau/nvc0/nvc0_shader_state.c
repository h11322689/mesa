   /*
   * Copyright 2010 Christoph Bumiller
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
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "util/u_inlines.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_query_hw.h"
      #include "nvc0/nvc0_compute.xml.h"
      static inline void
   nvc0_program_update_context_state(struct nvc0_context *nvc0,
         {
      if (prog && prog->need_tls) {
      const uint32_t flags = NV_VRAM_DOMAIN(&nvc0->screen->base) | NOUVEAU_BO_RDWR;
   if (!nvc0->state.tls_required)
            } else {
      if (nvc0->state.tls_required == (1 << stage))
               }
      static inline bool
   nvc0_program_validate(struct nvc0_context *nvc0, struct nvc0_program *prog)
   {
      if (prog->mem)
            if (!prog->translated) {
      prog->translated = nvc0_program_translate(
      prog, nvc0->screen->base.device->chipset,
      if (!prog->translated)
               if (likely(prog->code_size))
            }
      void
   nvc0_program_sp_start_id(struct nvc0_context *nvc0, int stage,
         {
               simple_mtx_assert_locked(&nvc0->screen->state_lock);
   if (nvc0->screen->eng3d->oclass < GV100_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(SP_START_ID(stage)), 1);
      } else {
      BEGIN_NVC0(push, SUBC_3D(GV100_3D_SP_ADDRESS_HIGH(stage)), 2);
   PUSH_DATAh(push, nvc0->screen->text->offset + prog->code_base);
         }
      void
   nvc0_vertprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (!nvc0_program_validate(nvc0, vp))
                  BEGIN_NVC0(push, NVC0_3D(SP_SELECT(1)), 1);
   PUSH_DATA (push, 0x11);
   nvc0_program_sp_start_id(nvc0, 1, vp);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(1)), 1);
            // BEGIN_NVC0(push, NVC0_3D_(0x163c), 1);
      }
      void
   nvc0_fragprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *fp = nvc0->fragprog;
            if (fp->fp.force_persample_interp != rast->force_persample_interp) {
      /* Force the program to be reuploaded, which will trigger interp fixups
   * to get applied
   */
   if (fp->mem)
                        if (fp->fp.msaa != rast->multisample) {
      /* Force the program to be reuploaded, which will trigger interp fixups
   * to get applied
   */
   if (fp->mem)
                        /* Shade model works well enough when both colors follow it. However if one
   * (or both) is explicitly set, then we have to go the patching route.
   */
   bool has_explicit_color = fp->fp.colors &&
      (((fp->fp.colors & 1) && !fp->fp.color_interp[0]) ||
      bool hwflatshade = false;
   if (has_explicit_color && fp->fp.flatshade != rast->flatshade) {
      /* Force re-upload */
   if (fp->mem)
                     /* Always smooth-shade in this mode, the shader will decide on its own
   * when to flat-shade.
      } else if (!has_explicit_color) {
               /* No need to binary-patch the shader each time, make sure that it's set
   * up for the default behaviour.
   */
               if (hwflatshade != nvc0->state.flatshade) {
      nvc0->state.flatshade = hwflatshade;
   BEGIN_NVC0(push, NVC0_3D(SHADE_MODEL), 1);
   PUSH_DATA (push, hwflatshade ? NVC0_3D_SHADE_MODEL_FLAT :
               if (fp->mem && !(nvc0->dirty_3d & NVC0_NEW_3D_FRAGPROG)) {
                  if (!nvc0_program_validate(nvc0, fp))
                  if (fp->fp.early_z != nvc0->state.early_z_forced) {
      nvc0->state.early_z_forced = fp->fp.early_z;
      }
   if (fp->fp.post_depth_coverage != nvc0->state.post_depth_coverage) {
      nvc0->state.post_depth_coverage = fp->fp.post_depth_coverage;
   IMMED_NVC0(push, NVC0_3D(POST_DEPTH_COVERAGE),
               BEGIN_NVC0(push, NVC0_3D(SP_SELECT(5)), 1);
   PUSH_DATA (push, 0x51);
   nvc0_program_sp_start_id(nvc0, 5, fp);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(5)), 1);
            BEGIN_NVC0(push, SUBC_3D(0x0360), 2);
   PUSH_DATA (push, 0x20164010);
   PUSH_DATA (push, 0x20);
   BEGIN_NVC0(push, NVC0_3D(ZCULL_TEST_MASK), 1);
      }
      void
   nvc0_tctlprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (tp && nvc0_program_validate(nvc0, tp)) {
      if (tp->tp.tess_mode != ~0) {
      BEGIN_NVC0(push, NVC0_3D(TESS_MODE), 1);
      }
   BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 1);
   PUSH_DATA (push, 0x21);
   nvc0_program_sp_start_id(nvc0, 2, tp);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(2)), 1);
      } else {
      tp = nvc0->tcp_empty;
   /* not a whole lot we can do to handle this failure */
   if (!nvc0_program_validate(nvc0, tp))
         BEGIN_NVC0(push, NVC0_3D(SP_SELECT(2)), 1);
   PUSH_DATA (push, 0x20);
      }
      }
      void
   nvc0_tevlprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (tp && nvc0_program_validate(nvc0, tp)) {
      if (tp->tp.tess_mode != ~0) {
      BEGIN_NVC0(push, NVC0_3D(TESS_MODE), 1);
      }
   BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
   PUSH_DATA (push, 0x31);
   nvc0_program_sp_start_id(nvc0, 3, tp);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(3)), 1);
      } else {
      BEGIN_NVC0(push, NVC0_3D(MACRO_TEP_SELECT), 1);
      }
      }
      void
   nvc0_gmtyprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            /* we allow GPs with no code for specifying stream output state only */
   if (gp && nvc0_program_validate(nvc0, gp) && gp->code_size) {
      BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
   PUSH_DATA (push, 0x41);
   nvc0_program_sp_start_id(nvc0, 4, gp);
   BEGIN_NVC0(push, NVC0_3D(SP_GPR_ALLOC(4)), 1);
      } else {
      BEGIN_NVC0(push, NVC0_3D(MACRO_GP_SELECT), 1);
      }
      }
      void
   nvc0_compprog_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (cp && !nvc0_program_validate(nvc0, cp))
            BEGIN_NVC0(push, NVC0_CP(FLUSH), 1);
      }
      void
   nvc0_layer_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *last;
   bool prog_selects_layer = false;
            if (nvc0->gmtyprog)
         else if (nvc0->tevlprog)
         else
            if (last) {
      prog_selects_layer = !!(last->hdr[13] & (1 << 9));
               BEGIN_NVC0(push, NVC0_3D(LAYER), 1);
   PUSH_DATA (push, prog_selects_layer ? NVC0_3D_LAYER_USE_GP : 0);
   if (nvc0->screen->eng3d->oclass >= GM200_3D_CLASS) {
      IMMED_NVC0(push, NVC0_3D(LAYER_VIEWPORT_RELATIVE),
         }
      void
   nvc0_tfb_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_transform_feedback_state *tfb;
            if (nvc0->gmtyprog) tfb = nvc0->gmtyprog->tfb;
   else
   if (nvc0->tevlprog) tfb = nvc0->tevlprog->tfb;
   else
                     if (tfb && tfb != nvc0->state.tfb) {
      for (b = 0; b < 4; ++b) {
                        BEGIN_NVC0(push, NVC0_3D(TFB_STREAM(b)), 3);
   PUSH_DATA (push, tfb->stream[b]);
   PUSH_DATA (push, tfb->varying_count[b]);
   PUSH_DATA (push, tfb->stride[b]);
                  if (nvc0->tfbbuf[b])
      } else {
                        simple_mtx_assert_locked(&nvc0->screen->state_lock);
            if (!(nvc0->dirty_3d & NVC0_NEW_3D_TFB_TARGETS))
            for (b = 0; b < nvc0->num_tfbbufs; ++b) {
      struct nvc0_so_target *targ = nvc0_so_target(nvc0->tfbbuf[b]);
            if (targ && tfb)
            if (!targ || !targ->stride) {
      IMMED_NVC0(push, NVC0_3D(TFB_BUFFER_ENABLE(b)), 0);
                                 if (!(nvc0->tfbbuf_dirty & (1 << b)))
            if (!targ->clean)
         PUSH_SPACE_EX(push, 0, 0, 1);
   BEGIN_NVC0(push, NVC0_3D(TFB_BUFFER_ENABLE(b)), 5);
   PUSH_DATA (push, 1);
   PUSH_DATAh(push, buf->address + targ->pipe.buffer_offset);
   PUSH_DATA (push, buf->address + targ->pipe.buffer_offset);
   PUSH_DATA (push, targ->pipe.buffer_size);
   if (!targ->clean) {
         } else {
      PUSH_DATA(push, 0); /* TFB_BUFFER_OFFSET */
         }
   for (; b < 4; ++b)
      }
