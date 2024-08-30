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
      #include "draw/draw_context.h"
   #include "util/u_dynarray.h"
   #include "tgsi/tgsi_parse.h"
   #include "nir/nir_to_tgsi.h"
      #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nvfx_shader.h"
   #include "nv30/nv30_state.h"
   #include "nv30/nv30_winsys.h"
      static void
   nv30_vertprog_destroy(struct nv30_vertprog *vp)
   {
      util_dynarray_fini(&vp->branch_relocs);
   nouveau_heap_free(&vp->exec);
   FREE(vp->insns);
   vp->insns = NULL;
            util_dynarray_fini(&vp->const_relocs);
   nouveau_heap_free(&vp->data);
   FREE(vp->consts);
   vp->consts = NULL;
               }
      void
   nv30_vertprog_validate(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
   struct nv30_vertprog *vp = nv30->vertprog.program;
   struct nv30_fragprog *fp = nv30->fragprog.program;
   bool upload_code = false;
   bool upload_data = false;
            if (nv30->dirty & NV30_NEW_FRAGPROG) {
      if (memcmp(vp->texcoord, fp->texcoord, sizeof(vp->texcoord))) {
      if (vp->translated)
                        if (nv30->rast && nv30->rast->pipe.clip_plane_enable != vp->enabled_ucps) {
      vp->enabled_ucps = nv30->rast->pipe.clip_plane_enable;
   if (vp->translated)
               if (!vp->translated) {
      vp->translated = _nvfx_vertprog_translate(eng3d->oclass, vp);
   if (!vp->translated) {
      nv30->draw_flags |= NV30_NEW_VERTPROG;
      }
               if (!vp->exec) {
      struct nouveau_heap *heap = nv30->screen->vp_exec_heap;
   struct nv30_shader_reloc *reloc = vp->branch_relocs.data;
   unsigned nr_reloc = vp->branch_relocs.size / sizeof(*reloc);
            if (nouveau_heap_alloc(heap, vp->nr_insns, &vp->exec, &vp->exec)) {
      while (heap->next && heap->size < vp->nr_insns) {
      struct nouveau_heap **evict = heap->next->priv;
               if (nouveau_heap_alloc(heap, vp->nr_insns, &vp->exec, &vp->exec)) {
      nv30->draw_flags |= NV30_NEW_VERTPROG;
                  if (eng3d->oclass < NV40_3D_CLASS) {
      while (nr_reloc--) {
                     inst[2] &= ~0x000007fc;
   inst[2] |= target << 2;
         } else {
      while (nr_reloc--) {
                     inst[2] &= ~0x0000003f;
   inst[2] |= target >> 3;
   inst[3] &= ~0xe0000000;
   inst[3] |= target << 29;
                              if (vp->nr_consts && !vp->data) {
      struct nouveau_heap *heap = nv30->screen->vp_data_heap;
   struct nv30_shader_reloc *reloc = vp->const_relocs.data;
   unsigned nr_reloc = vp->const_relocs.size / sizeof(*reloc);
            if (nouveau_heap_alloc(heap, vp->nr_consts, vp, &vp->data)) {
      while (heap->next && heap->size < vp->nr_consts) {
      struct nv30_vertprog *evp = heap->next->priv;
               if (nouveau_heap_alloc(heap, vp->nr_consts, vp, &vp->data)) {
      nv30->draw_flags |= NV30_NEW_VERTPROG;
                  if (eng3d->oclass < NV40_3D_CLASS) {
      while (nr_reloc--) {
                     inst[1] &= ~0x0007fc000;
   inst[1] |= (target & 0x1ff) << 14;
         } else {
      while (nr_reloc--) {
                     inst[1] &= ~0x0001ff000;
   inst[1] |= (target & 0x1ff) << 12;
                  upload_code = true;
               if (vp->nr_consts) {
               for (i = 0; i < vp->nr_consts; i++) {
               if (data->index < 0) {
      if (!upload_data)
      } else {
      float *constbuf = (float *)res->data;
   if (!upload_data &&
      !memcmp(data->value, &constbuf[data->index * 4], 16))
                  BEGIN_NV04(push, NV30_3D(VP_UPLOAD_CONST_ID), 5);
   PUSH_DATA (push, vp->data->start + i);
                  if (upload_code) {
      BEGIN_NV04(push, NV30_3D(VP_UPLOAD_FROM_ID), 1);
   PUSH_DATA (push, vp->exec->start);
   for (i = 0; i < vp->nr_insns; i++) {
      BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
                  if (nv30->dirty & (NV30_NEW_VERTPROG | NV30_NEW_FRAGPROG)) {
      BEGIN_NV04(push, NV30_3D(VP_START_FROM_ID), 1);
   PUSH_DATA (push, vp->exec->start);
   if (eng3d->oclass < NV40_3D_CLASS) {
      BEGIN_NV04(push, NV30_3D(ENGINE), 1);
      } else {
      BEGIN_NV04(push, NV40_3D(VP_ATTRIB_EN), 2);
   PUSH_DATA (push, vp->ir);
   PUSH_DATA (push, vp->or | fp->vp_or);
   BEGIN_NV04(push, NV30_3D(ENGINE), 1);
            }
      static void *
   nv30_vp_state_create(struct pipe_context *pipe,
         {
      struct nv30_vertprog *vp = CALLOC_STRUCT(nv30_vertprog);
   if (!vp)
            if (cso->type == PIPE_SHADER_IR_NIR) {
         } else {
      assert(cso->type == PIPE_SHADER_IR_TGSI);
   /* we need to keep a local copy of the tokens */
               tgsi_scan_shader(vp->pipe.tokens, &vp->info);
      }
      static void
   nv30_vp_state_delete(struct pipe_context *pipe, void *hwcso)
   {
               if (vp->translated)
            if (vp->draw)
            FREE((void *)vp->pipe.tokens);
      }
      static void
   nv30_vp_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv30->vertprog.program = hwcso;
      }
      void
   nv30_vertprog_init(struct pipe_context *pipe)
   {
      pipe->create_vs_state = nv30_vp_state_create;
   pipe->bind_vs_state = nv30_vp_state_bind;
      }
