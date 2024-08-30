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
      #include "pipe/p_defines.h"
   #include "util/u_framebuffer.h"
   #include "util/u_upload_mgr.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_screen.h"
   #include "nvc0/nvc0_resource.h"
         #include "xf86drm.h"
   #include "nouveau_drm.h"
         static void
   nvc0_svm_migrate(struct pipe_context *pipe, unsigned num_ptrs,
               {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_screen *screen = &nvc0->screen->base;
   int fd = screen->drm->fd;
            for (i = 0; i < num_ptrs; i++) {
      struct drm_nouveau_svm_bind args;
            args.va_start = (uint64_t)(uintptr_t)ptrs[i];
   if (sizes && sizes[i]) {
      args.va_end = (uint64_t)(uintptr_t)ptrs[i] + sizes[i];
      } else {
      args.va_end = 0;
      }
            args.reserved0 = 0;
            prio = 0;
   cmd = NOUVEAU_SVM_BIND_COMMAND__MIGRATE;
            args.header = cmd << NOUVEAU_SVM_BIND_COMMAND_SHIFT;
   args.header |= prio << NOUVEAU_SVM_BIND_PRIORITY_SHIFT;
            /* This is best effort, so no garanty whatsoever */
   drmCommandWrite(fd, DRM_NOUVEAU_SVM_BIND,
         }
         static void
   nvc0_flush(struct pipe_context *pipe,
               {
               if (fence)
                        }
      static void
   nvc0_texture_barrier(struct pipe_context *pipe, unsigned flags)
   {
               IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);
      }
      static void
   nvc0_memory_barrier(struct pipe_context *pipe, unsigned flags)
   {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (!(flags & ~PIPE_BARRIER_UPDATE))
            if (flags & PIPE_BARRIER_MAPPED_BUFFER) {
      for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      if (!nvc0->vtxbuf[i].buffer.resource && !nvc0->vtxbuf[i].is_user_buffer)
         if (nvc0->vtxbuf[i].buffer.resource->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
               for (s = 0; s < 5 && !nvc0->cb_dirty; ++s) {
               while (valid && !nvc0->cb_dirty) {
                     valid &= ~(1 << i);
                  res = nvc0->constbuf[s][i].u.buf;
                  if (res->flags & PIPE_RESOURCE_FLAG_MAP_PERSISTENT)
            } else {
      /* Pretty much any writing by shaders needs a serialize after
   * it. Especially when moving between 3d and compute pipelines, but even
   * without that.
   */
               /* If we're going to texture from a buffer/image written by a shader, we
   * must flush the texture cache.
   */
   if (flags & PIPE_BARRIER_TEXTURE)
            if (flags & PIPE_BARRIER_CONSTANT_BUFFER)
         if (flags & (PIPE_BARRIER_VERTEX_BUFFER | PIPE_BARRIER_INDEX_BUFFER))
      }
      static void
   nvc0_emit_string_marker(struct pipe_context *pipe, const char *str, int len)
   {
      struct nouveau_pushbuf *push = nvc0_context(pipe)->base.pushbuf;
   int string_words = len / 4;
            if (len <= 0)
         string_words = MIN2(string_words, NV04_PFIFO_MAX_PACKET_LEN);
   if (string_words == NV04_PFIFO_MAX_PACKET_LEN)
         else
         BEGIN_NIC0(push, SUBC_3D(NV04_GRAPH_NOP), data_words);
   if (string_words)
         if (string_words != data_words) {
      int data = 0;
   memcpy(&data, &str[string_words * 4], len & 3);
         }
      static enum pipe_reset_status
   nvc0_get_device_reset_status(struct pipe_context *pipe)
   {
         }
      static void
   nvc0_context_unreference_resources(struct nvc0_context *nvc0)
   {
               nouveau_bufctx_del(&nvc0->bufctx_3d);
   nouveau_bufctx_del(&nvc0->bufctx);
                     for (i = 0; i < nvc0->num_vtxbufs; ++i)
            for (s = 0; s < 6; ++s) {
      for (i = 0; i < nvc0->num_textures[s]; ++i)
            for (i = 0; i < NVC0_MAX_PIPE_CONSTBUFS; ++i)
                  for (i = 0; i < NVC0_MAX_BUFFERS; ++i)
            for (i = 0; i < NVC0_MAX_IMAGES; ++i) {
      pipe_resource_reference(&nvc0->images[s][i].resource, NULL);
   if (nvc0->screen->base.class_3d >= GM107_3D_CLASS)
                  for (s = 0; s < 2; ++s) {
      for (i = 0; i < NVC0_MAX_SURFACE_SLOTS; ++i)
               for (i = 0; i < nvc0->num_tfbbufs; ++i)
            for (i = 0; i < nvc0->global_residents.size / sizeof(struct pipe_resource *);
      ++i) {
   struct pipe_resource **res = util_dynarray_element(
            }
            if (nvc0->tcp_empty)
      }
      static void
   nvc0_destroy(struct pipe_context *pipe)
   {
               simple_mtx_lock(&nvc0->screen->state_lock);
   if (nvc0->screen->cur_ctx == nvc0) {
      nvc0->screen->cur_ctx = NULL;
   nvc0->screen->save_state = nvc0->state;
      }
            if (nvc0->base.pipe.stream_uploader)
            /* Unset bufctx, we don't want to revalidate any resources after the flush.
   * Other contexts will always set their bufctx again on action calls.
   */
   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, NULL);
            nvc0_context_unreference_resources(nvc0);
            list_for_each_entry_safe(struct nvc0_resident, pos, &nvc0->tex_head, list) {
      list_del(&pos->list);
               list_for_each_entry_safe(struct nvc0_resident, pos, &nvc0->img_head, list) {
      list_del(&pos->list);
               nouveau_fence_cleanup(&nvc0->base);
      }
      void
   nvc0_default_kick_notify(struct nouveau_context *context)
   {
               _nouveau_fence_next(context);
               }
      static int
   nvc0_invalidate_resource_storage(struct nouveau_context *ctx,
               {
      struct nvc0_context *nvc0 = nvc0_context(&ctx->pipe);
            if (res->bind & PIPE_BIND_RENDER_TARGET) {
      for (i = 0; i < nvc0->framebuffer.nr_cbufs; ++i) {
      if (nvc0->framebuffer.cbufs[i] &&
      nvc0->framebuffer.cbufs[i]->texture == res) {
   nvc0->dirty_3d |= NVC0_NEW_3D_FRAMEBUFFER;
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_FB);
   if (!--ref)
            }
   if (res->bind & PIPE_BIND_DEPTH_STENCIL) {
      if (nvc0->framebuffer.zsbuf &&
      nvc0->framebuffer.zsbuf->texture == res) {
   nvc0->dirty_3d |= NVC0_NEW_3D_FRAMEBUFFER;
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_FB);
   if (!--ref)
                  if (res->target == PIPE_BUFFER) {
      for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      if (nvc0->vtxbuf[i].buffer.resource == res) {
      nvc0->dirty_3d |= NVC0_NEW_3D_ARRAYS;
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_VTX);
   if (!--ref)
                  for (s = 0; s < 6; ++s) {
      for (i = 0; i < nvc0->num_textures[s]; ++i) {
      if (nvc0->textures[s][i] &&
      nvc0->textures[s][i]->texture == res) {
   nvc0->textures_dirty[s] |= 1 << i;
   if (unlikely(s == 5)) {
      nvc0->dirty_cp |= NVC0_NEW_CP_TEXTURES;
      } else {
      nvc0->dirty_3d |= NVC0_NEW_3D_TEXTURES;
      }
   if (!--ref)
                     for (s = 0; s < 6; ++s) {
      for (i = 0; i < NVC0_MAX_PIPE_CONSTBUFS; ++i) {
      if (!(nvc0->constbuf_valid[s] & (1 << i)))
         if (!nvc0->constbuf[s][i].user &&
      nvc0->constbuf[s][i].u.buf == res) {
   nvc0->constbuf_dirty[s] |= 1 << i;
   if (unlikely(s == 5)) {
      nvc0->dirty_cp |= NVC0_NEW_CP_CONSTBUF;
      } else {
      nvc0->dirty_3d |= NVC0_NEW_3D_CONSTBUF;
      }
   if (!--ref)
                     for (s = 0; s < 6; ++s) {
      for (i = 0; i < NVC0_MAX_BUFFERS; ++i) {
      if (nvc0->buffers[s][i].buffer == res) {
      nvc0->buffers_dirty[s] |= 1 << i;
   if (unlikely(s == 5)) {
      nvc0->dirty_cp |= NVC0_NEW_CP_BUFFERS;
      } else {
      nvc0->dirty_3d |= NVC0_NEW_3D_BUFFERS;
      }
   if (!--ref)
                     for (s = 0; s < 6; ++s) {
      for (i = 0; i < NVC0_MAX_IMAGES; ++i) {
      if (nvc0->images[s][i].resource == res) {
      nvc0->images_dirty[s] |= 1 << i;
   if (unlikely(s == 5)) {
      nvc0->dirty_cp |= NVC0_NEW_CP_SURFACES;
      } else {
      nvc0->dirty_3d |= NVC0_NEW_3D_SURFACES;
         }
   if (!--ref)
                        }
      static void
   nvc0_context_get_sample_position(struct pipe_context *, unsigned, unsigned,
            struct pipe_context *
   nvc0_create(struct pipe_screen *pscreen, void *priv, unsigned ctxflags)
   {
      struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nvc0_context *nvc0;
   struct pipe_context *pipe;
   int ret;
            nvc0 = CALLOC_STRUCT(nvc0_context);
   if (!nvc0)
                  if (!nvc0_blitctx_create(nvc0))
            if (nouveau_context_init(&nvc0->base, &screen->base))
         nvc0->base.kick_notify = nvc0_default_kick_notify;
            ret = nouveau_bufctx_new(nvc0->base.client, 2, &nvc0->bufctx);
   if (!ret)
      ret = nouveau_bufctx_new(nvc0->base.client, NVC0_BIND_3D_COUNT,
      if (!ret)
      ret = nouveau_bufctx_new(nvc0->base.client, NVC0_BIND_CP_COUNT,
      if (ret)
            nvc0->screen = screen;
   pipe->screen = pscreen;
   pipe->priv = priv;
   pipe->stream_uploader = u_upload_create_default(pipe);
   if (!pipe->stream_uploader)
                           pipe->draw_vbo = nvc0_draw_vbo;
   pipe->clear = nvc0_clear;
   pipe->launch_grid = (nvc0->screen->base.class_3d >= NVE4_3D_CLASS) ?
                     pipe->flush = nvc0_flush;
   pipe->texture_barrier = nvc0_texture_barrier;
   pipe->memory_barrier = nvc0_memory_barrier;
   pipe->get_sample_position = nvc0_context_get_sample_position;
   pipe->emit_string_marker = nvc0_emit_string_marker;
            nvc0_init_query_functions(nvc0);
   nvc0_init_surface_functions(nvc0);
   nvc0_init_state_functions(nvc0);
   nvc0_init_transfer_functions(nvc0);
   nvc0_init_resource_functions(pipe);
   if (nvc0->screen->base.class_3d >= NVE4_3D_CLASS)
            list_inithead(&nvc0->tex_head);
                     pipe->create_video_codec = nvc0_create_decoder;
            /* shader builtin library is per-screen, but we need a context for m2mf */
   nvc0_program_library_upload(nvc0);
   nvc0_program_init_tcp_empty(nvc0);
   if (!nvc0->tcp_empty)
         /* set the empty tctl prog on next draw in case one is never set */
            /* Do not bind the COMPUTE driver constbuf at screen initialization because
   * CBs are aliased between 3D and COMPUTE, but make sure it will be bound if
   * a grid is launched later. */
            /* now that there are no more opportunities for errors, set the current
   * context if there isn't already one.
   */
   simple_mtx_lock(&screen->state_lock);
   if (!screen->cur_ctx) {
      nvc0->state = screen->save_state;
      }
            nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx);
                              BCTX_REFN_bo(nvc0->bufctx_3d, 3D_SCREEN, flags, screen->uniform_bo);
   BCTX_REFN_bo(nvc0->bufctx_3d, 3D_SCREEN, flags, screen->txc);
   if (screen->compute) {
      BCTX_REFN_bo(nvc0->bufctx_cp, CP_SCREEN, flags, screen->uniform_bo);
                        if (screen->poly_cache)
         if (screen->compute)
                     BCTX_REFN_bo(nvc0->bufctx_3d, 3D_SCREEN, flags, screen->fence.bo);
   BCTX_REFN_bo(nvc0->bufctx, FENCE, flags, screen->fence.bo);
   if (screen->compute)
                                       // Make sure that the first TSC entry has SRGB conversion bit set, since we
   // use it as a fallback on Fermi for TXF, and on Kepler+ generations for
   // FBFETCH handling (which also uses TXF).
   //
   // NOTE: Preliminary testing suggests that this isn't necessary at all at
   // least on GM20x (untested on Kepler). However this is ~free, so no reason
   // not to do it.
   if (!screen->tsc.entries[0])
            // On Fermi, mark samplers dirty so that the proper binding can happen
   if (screen->base.class_3d < NVE4_3D_CLASS) {
      for (int s = 0; s < 6; s++)
         nvc0->dirty_3d |= NVC0_NEW_3D_SAMPLERS;
                              out_err:
      if (nvc0) {
      if (pipe->stream_uploader)
         if (nvc0->bufctx_3d)
         if (nvc0->bufctx_cp)
         if (nvc0->bufctx)
         FREE(nvc0->blit);
      }
      }
      void
   nvc0_bufctx_fence(struct nvc0_context *nvc0, struct nouveau_bufctx *bufctx,
         {
      struct nouveau_list *list = on_flush ? &bufctx->current : &bufctx->pending;
   struct nouveau_list *it;
            for (it = list->next; it != list; it = it->next) {
      struct nouveau_bufref *ref = (struct nouveau_bufref *)it;
   struct nv04_resource *res = ref->priv;
   if (res)
            }
      }
      const void *
   nvc0_get_sample_locations(unsigned sample_count)
   {
      static const uint8_t ms1[1][2] = { { 0x8, 0x8 } };
   static const uint8_t ms2[2][2] = {
         static const uint8_t ms4[4][2] = {
      { 0x6, 0x2 }, { 0xe, 0x6 },   /* (0,0), (1,0) */
      static const uint8_t ms8[8][2] = {
      { 0x1, 0x7 }, { 0x5, 0x3 },   /* (0,0), (1,0) */
   { 0x3, 0xd }, { 0x7, 0xb },   /* (0,1), (1,1) */
   { 0x9, 0x5 }, { 0xf, 0x1 },   /* (2,0), (3,0) */
                  switch (sample_count) {
   case 0:
   case 1: ptr = ms1; break;
   case 2: ptr = ms2; break;
   case 4: ptr = ms4; break;
   case 8: ptr = ms8; break;
   default:
      assert(0);
      }
      }
      static void
   nvc0_context_get_sample_position(struct pipe_context *pipe,
               {
               ptr = nvc0_get_sample_locations(sample_count);
   if (!ptr)
            xy[0] = ptr[sample_index][0] * 0.0625f;
      }
