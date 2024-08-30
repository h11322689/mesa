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
      #include "util/format/u_format.h"
   #include "util/u_draw.h"
   #include "util/u_inlines.h"
   #include "util/u_prim.h"
   #include "translate/translate.h"
      #include "nouveau_fence.h"
   #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_format.h"
   #include "nv30/nv30_winsys.h"
      static void
   nv30_emit_vtxattr(struct nv30_context *nv30, struct pipe_vertex_buffer *vb,
         {
      const unsigned nc = util_format_get_nr_components(ve->src_format);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nv04_resource *res = nv04_resource(vb->buffer.resource);
   const void *data;
            data = nouveau_resource_map_offset(&nv30->base, res, vb->buffer_offset +
                     switch (nc) {
   case 4:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_4F(attr)), 4);
   PUSH_DATAf(push, v[0]);
   PUSH_DATAf(push, v[1]);
   PUSH_DATAf(push, v[2]);
   PUSH_DATAf(push, v[3]);
      case 3:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_3F(attr)), 3);
   PUSH_DATAf(push, v[0]);
   PUSH_DATAf(push, v[1]);
   PUSH_DATAf(push, v[2]);
      case 2:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_2F(attr)), 2);
   PUSH_DATAf(push, v[0]);
   PUSH_DATAf(push, v[1]);
      case 1:
      BEGIN_NV04(push, NV30_3D(VTX_ATTR_1F(attr)), 1);
   PUSH_DATAf(push, v[0]);
      default:
      assert(0);
         }
      static inline void
   nv30_vbuf_range(struct nv30_context *nv30, int vbi,
         {
      assert(nv30->vbo_max_index != ~0);
   *base = nv30->vbo_min_index * nv30->vertex->strides[vbi];
   *size = (nv30->vbo_max_index -
      }
      static void
   nv30_prevalidate_vbufs(struct nv30_context *nv30)
   {
      struct pipe_vertex_buffer *vb;
   struct nv04_resource *buf;
   int i;
                     for (i = 0; i < nv30->num_vtxbufs; i++) {
      vb = &nv30->vtxbuf[i];
   if (!nv30->vertex->strides[i] || !vb->buffer.resource) /* NOTE: user_buffer not implemented */
                  /* NOTE: user buffers with temporary storage count as mapped by GPU */
   if (!nouveau_resource_mapped_by_gpu(vb->buffer.resource)) {
      if (nv30->vbo_push_hint) {
      nv30->vbo_fifo = ~0;
      } else {
      if (buf->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY) {
      nv30->vbo_user |= 1 << i;
   assert(nv30->vertex->strides[i] > vb->buffer_offset);
   nv30_vbuf_range(nv30, i, &base, &size);
      } else {
         }
               }
      static void
   nv30_update_user_vbufs(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   uint32_t base, offset, size;
   int i;
            for (i = 0; i < nv30->vertex->num_elements; i++) {
      struct pipe_vertex_element *ve = &nv30->vertex->pipe[i];
   const int b = ve->vertex_buffer_index;
   struct pipe_vertex_buffer *vb = &nv30->vtxbuf[b];
            if (!(nv30->vbo_user & (1 << b)))
            if (!nv30->vertex->strides[i]) {
      nv30_emit_vtxattr(nv30, vb, ve, i);
      }
            if (!(written & (1 << b))) {
      written |= 1 << b;
                        BEGIN_NV04(push, NV30_3D(VTXBUF(i)), 1);
   PUSH_RESRC(push, NV30_3D(VTXBUF(i)), BUFCTX_VTXTMP, buf, offset,
            }
      }
      static inline void
   nv30_release_user_vbufs(struct nv30_context *nv30)
   {
               while (vbo_user) {
      int i = ffs(vbo_user) - 1;
                           }
      void
   nv30_vbo_validate(struct nv30_context *nv30)
   {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nv30_vertex_stateobj *vertex = nv30->vertex;
   struct pipe_vertex_element *ve;
   struct pipe_vertex_buffer *vb;
            nouveau_bufctx_reset(nv30->bufctx, BUFCTX_VTXBUF);
   if (!nv30->vertex || nv30->draw_flags)
         #if UTIL_ARCH_BIG_ENDIAN
         #else
         #endif
         nv30->vbo_fifo = ~0;
      } else {
                  if (!PUSH_SPACE(push, 128))
            redefine = MAX2(vertex->num_elements, nv30->state.num_vtxelts);
   if (redefine == 0)
                     for (i = 0; i < vertex->num_elements; i++) {
      ve = &vertex->pipe[i];
            if (likely(vertex->strides[ve->vertex_buffer_index]) || nv30->vbo_fifo)
         else
               for (; i < nv30->state.num_vtxelts; i++) {
                  for (i = 0; i < vertex->num_elements; i++) {
      struct nv04_resource *res;
   unsigned offset;
            ve = &vertex->pipe[i];
   vb = &nv30->vtxbuf[ve->vertex_buffer_index];
                     if (nv30->vbo_fifo || unlikely(ve->src_stride == 0)) {
      if (!nv30->vbo_fifo)
                              BEGIN_NV04(push, NV30_3D(VTXBUF(i)), 1);
   PUSH_RESRC(push, NV30_3D(VTXBUF(i)), user ? BUFCTX_VTXTMP : BUFCTX_VTXBUF,
                        }
      static void *
   nv30_vertex_state_create(struct pipe_context *pipe, unsigned num_elements,
         {
      struct nv30_vertex_stateobj *so;
   struct translate_key transkey;
            so = CALLOC(1, sizeof(*so) + sizeof(*so->element) * num_elements);
   if (!so)
         memcpy(so->pipe, elements, sizeof(*elements) * num_elements);
   so->num_elements = num_elements;
            transkey.nr_elements = 0;
            for (i = 0; i < num_elements; i++) {
      const struct pipe_vertex_element *ve = &elements[i];
   const unsigned vbi = ve->vertex_buffer_index;
            so->element[i].state = nv30_vtxfmt(pipe->screen, fmt)->hw;
   if (!so->element[i].state) {
         switch (util_format_get_nr_components(fmt)) {
   case 1: fmt = PIPE_FORMAT_R32_FLOAT; break;
   case 2: fmt = PIPE_FORMAT_R32G32_FLOAT; break;
   case 3: fmt = PIPE_FORMAT_R32G32B32_FLOAT; break;
   case 4: fmt = PIPE_FORMAT_R32G32B32A32_FLOAT; break;
   default:
      assert(0);
   FREE(so);
      }
   so->element[i].state = nv30_vtxfmt(pipe->screen, fmt)->hw;
            if (1) {
                  transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
   transkey.element[j].input_format = ve->src_format;
   transkey.element[j].input_buffer = vbi;
                  transkey.element[j].output_format = fmt;
   transkey.element[j].output_offset = transkey.output_stride;
   }
               so->translate = translate_create(&transkey);
   so->vtx_size = transkey.output_stride / 4;
   so->vtx_per_packet_max = NV04_PFIFO_MAX_PACKET_LEN / MAX2(so->vtx_size, 1);
      }
      static void
   nv30_vertex_state_delete(struct pipe_context *pipe, void *hwcso)
   {
               if (so->translate)
            }
      static void
   nv30_vertex_state_bind(struct pipe_context *pipe, void *hwcso)
   {
               nv30->vertex = hwcso;
      }
      static void
   nv30_draw_arrays(struct nv30_context *nv30,
               {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
                     BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, prim);
   while (count) {
      const unsigned mpush = 2047 * 256;
   unsigned npush  = (count > mpush) ? mpush : count;
                     BEGIN_NI04(push, NV30_3D(VB_VERTEX_BATCH), wpush);
   while (npush >= 256) {
      PUSH_DATA (push, 0xff000000 | start);
   start += 256;
               if (npush)
      }
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
      }
      static void
   nv30_draw_elements_inline_u08(struct nouveau_pushbuf *push, const uint8_t *map,
         {
               if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
               count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
            BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
   while (npush--) {
      PUSH_DATA (push, (map[1] << 16) | map[0]);
               }
      static void
   nv30_draw_elements_inline_u16(struct nouveau_pushbuf *push, const uint16_t *map,
         {
               if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
               count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
            BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
   while (npush--) {
      PUSH_DATA (push, (map[1] << 16) | map[0]);
            }
      static void
   nv30_draw_elements_inline_u32(struct nouveau_pushbuf *push, const uint32_t *map,
         {
               while (count) {
               BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U32), nr);
            map += nr;
         }
      static void
   nv30_draw_elements_inline_u32_short(struct nouveau_pushbuf *push,
               {
               if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
               count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
            BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
   while (npush--) {
      PUSH_DATA (push, (map[1] << 16) | map[0]);
            }
      static void
   nv30_draw_elements(struct nv30_context *nv30, bool shorten,
                           {
      struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
            if (eng3d->oclass >= NV40_3D_CLASS && index_bias != nv30->state.index_bias) {
      BEGIN_NV04(push, NV40_3D(VB_ELEMENT_BASE), 1);
   PUSH_DATA (push, index_bias);
               if (eng3d->oclass == NV40_3D_CLASS && index_size > 1 &&
      !info->has_user_indices) {
   struct nv04_resource *res = nv04_resource(info->index.resource);
                     BEGIN_NV04(push, NV30_3D(IDXBUF_OFFSET), 2);
   PUSH_RESRC(push, NV30_3D(IDXBUF_OFFSET), BUFCTX_IDXBUF, res, offset,
         PUSH_MTHD (push, NV30_3D(IDXBUF_FORMAT), BUFCTX_IDXBUF, res->bo,
                     BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, prim);
   while (count) {
      const unsigned mpush = 2047 * 256;
                           BEGIN_NI04(push, NV30_3D(VB_INDEX_BATCH), wpush);
   while (npush >= 256) {
      PUSH_DATA (push, 0xff000000 | start);
   start += 256;
               if (npush)
      }
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
      } else {
      const void *data;
   if (!info->has_user_indices)
      data = nouveau_resource_map_offset(&nv30->base,
            else
         if (!data)
            BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, prim);
   switch (index_size) {
   case 1:
      nv30_draw_elements_inline_u08(push, data, start, count);
      case 2:
      nv30_draw_elements_inline_u16(push, data, start, count);
      case 4:
      if (shorten)
         else
            default:
      assert(0);
      }
   BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
         }
      static void
   nv30_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(pipe, info, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !info->instance_count))
            struct nv30_context *nv30 = nv30_context(pipe);
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
            if (!info->primitive_restart &&
      !u_trim_pipe_prim(info->mode, (unsigned*)&draws[0].count))
         /* For picking only a few vertices from a large user buffer, push is better,
   * if index count is larger and we expect repeated vertices, suggest upload.
   */
   nv30->vbo_push_hint = /* the 64 is heuristic */
      !(info->index_size &&
   info->index_bounds_valid &&
         if (info->index_bounds_valid) {
      nv30->vbo_min_index = info->min_index;
      } else {
      nv30->vbo_min_index = 0;
               if (nv30->vbo_push_hint != !!nv30->vbo_fifo)
            if (nv30->vbo_user && !(nv30->dirty & (NV30_NEW_VERTEX | NV30_NEW_ARRAYS)))
            nv30_state_validate(nv30, ~0, true);
   if (nv30->draw_flags) {
      nv30_render_vbo(pipe, info, drawid_offset, &draws[0]);
      } else
   if (nv30->vbo_fifo) {
      nv30_push_vbo(nv30, info, &draws[0]);
               for (i = 0; i < nv30->num_vtxbufs && !nv30->base.vbo_dirty; ++i) {
      if (!nv30->vtxbuf[i].buffer.resource)
         if (nv30->vtxbuf[i].buffer.resource->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
               if (!nv30->base.vbo_dirty && info->index_size && !info->has_user_indices &&
      info->index.resource->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
         if (nv30->base.vbo_dirty) {
      BEGIN_NV04(push, NV30_3D(VTX_CACHE_INVALIDATE_1710), 1);
   PUSH_DATA (push, 0);
               if (!info->index_size) {
      nv30_draw_arrays(nv30,
            } else {
               if (info->primitive_restart != nv30->state.prim_restart) {
      if (info->primitive_restart) {
      BEGIN_NV04(push, NV40_3D(PRIM_RESTART_ENABLE), 2);
                  if (info->restart_index > 65535)
      } else {
      BEGIN_NV04(push, NV40_3D(PRIM_RESTART_ENABLE), 1);
      }
      } else
   if (info->primitive_restart) {
                     if (info->restart_index > 65535)
               nv30_draw_elements(nv30, shorten, info,
                     nv30_state_release(nv30);
      }
      void
   nv30_vbo_init(struct pipe_context *pipe)
   {
      pipe->create_vertex_elements_state = nv30_vertex_state_create;
   pipe->delete_vertex_elements_state = nv30_vertex_state_delete;
   pipe->bind_vertex_elements_state = nv30_vertex_state_bind;
      }
