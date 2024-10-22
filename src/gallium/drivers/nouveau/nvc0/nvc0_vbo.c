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
      #define NVC0_PUSH_EXPLICIT_SPACE_CHECKING
      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/u_draw.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "translate/translate.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_query_hw.h"
   #include "nvc0/nvc0_resource.h"
      #include "nvc0/nvc0_3d.xml.h"
      void
   nvc0_vertex_state_delete(struct pipe_context *pipe,
         {
               if (so->translate)
            }
      void *
   nvc0_vertex_state_create(struct pipe_context *pipe,
               {
      struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nvc0_vertex_stateobj *so;
   struct translate_key transkey;
   unsigned i;
            so = CALLOC(1, sizeof(*so) +
         if (!so)
         so->num_elements = num_elements;
   so->instance_elts = 0;
   so->instance_bufs = 0;
   so->shared_slots = false;
                     for (i = 0; i < PIPE_MAX_ATTRIBS; ++i)
            transkey.nr_elements = 0;
            for (i = 0; i < num_elements; ++i) {
      const struct pipe_vertex_element *ve = &elements[i];
   const unsigned vbi = ve->vertex_buffer_index;
   unsigned size;
            so->element[i].pipe = elements[i];
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
   so->element[i].state = nvc0_vertex_format[fmt].vtx;
   so->need_conversion = true;
   util_debug_message(&nouveau_context(pipe)->debug, FALLBACK,
         }
                     if (so->vb_access_size[vbi] < (ve->src_offset + size))
            if (unlikely(ve->instance_divisor)) {
      so->instance_elts |= 1 << i;
   so->instance_bufs |= 1 << vbi;
   if (ve->instance_divisor < so->min_instance_div[vbi])
               so->strides[vbi] = ve->src_stride;
   if (!ve->src_stride && nvc0->screen->eng3d->oclass < GM107_3D_CLASS)
            if (1) {
                        ca = util_format_description(fmt)->channel[0].size / 8;
                  transkey.element[j].type = TRANSLATE_ELEMENT_NORMAL;
   transkey.element[j].input_format = ve->src_format;
   transkey.element[j].input_buffer = vbi;
                  transkey.output_stride = align(transkey.output_stride, ca);
   transkey.element[j].output_format = fmt;
                  so->element[i].state_alt = so->element[i].state;
               }
            so->size = transkey.output_stride;
            if (so->instance_elts || src_offset_max >= (1 << 14))
                  for (i = 0; i < num_elements; ++i) {
      const unsigned b = elements[i].vertex_buffer_index;
   const unsigned s = elements[i].src_offset;
   so->element[i].state &= ~NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__MASK;
   so->element[i].state |= b << NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__SHIFT;
      }
      }
      #define NVC0_3D_VERTEX_ATTRIB_INACTIVE                                       \
      NVC0_3D_VERTEX_ATTRIB_FORMAT_TYPE_FLOAT |                                 \
         #define VTX_ATTR(a, c, t, s)                            \
      ((NVC0_3D_VTX_ATTR_DEFINE_TYPE_##t) |                \
   (NVC0_3D_VTX_ATTR_DEFINE_SIZE_##s) |                \
   ((a) << NVC0_3D_VTX_ATTR_DEFINE_ATTR__SHIFT) |      \
         static void
   nvc0_set_constant_vertex_attrib(struct nvc0_context *nvc0, const unsigned a)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct pipe_vertex_element *ve = &nvc0->vertex->element[a].pipe;
   struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[ve->vertex_buffer_index];
   uint32_t mode;
   const struct util_format_description *desc;
   void *dst;
   const void *src = (const uint8_t *)vb->buffer.user + ve->src_offset;
                     PUSH_SPACE(push, 6);
   BEGIN_NVC0(push, NVC0_3D(VTX_ATTR_DEFINE), 5);
   dst = &push->cur[1];
   util_format_unpack_rgba(ve->src_format, dst, src, 1);
   if (desc->channel[0].pure_integer) {
      if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         } else {
            } else {
         }
   push->cur[0] = mode;
      }
      static inline void
   nvc0_user_vbuf_range(struct nvc0_context *nvc0, int vbi,
         {
      if (unlikely(nvc0->vertex->instance_bufs & (1 << vbi))) {
      const uint32_t div = nvc0->vertex->min_instance_div[vbi];
   *base = nvc0->instance_off * nvc0->vertex->strides[vbi];
   *size = (nvc0->instance_max / div) * nvc0->vertex->strides[vbi] +
      } else {
      /* NOTE: if there are user buffers, we *must* have index bounds */
   assert(nvc0->vb_elt_limit != ~0);
   *base = nvc0->vb_elt_first * nvc0->vertex->strides[vbi];
   *size = nvc0->vb_elt_limit * nvc0->vertex->strides[vbi] +
         }
      static inline void
   nvc0_release_user_vbufs(struct nvc0_context *nvc0)
   {
      if (nvc0->vbo_user) {
      nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_VTX_TMP);
         }
      static void
   nvc0_update_user_vbufs(struct nvc0_context *nvc0)
   {
      uint64_t address[PIPE_MAX_ATTRIBS];
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   int i;
            PUSH_SPACE(push, nvc0->vertex->num_elements * 8);
   for (i = 0; i < nvc0->vertex->num_elements; ++i) {
      struct pipe_vertex_element *ve = &nvc0->vertex->element[i].pipe;
   const unsigned b = ve->vertex_buffer_index;
   struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[b];
            if (!(nvc0->vbo_user & (1 << b)))
         if (nvc0->constant_vbos & (1 << b)) {
      nvc0_set_constant_vertex_attrib(nvc0, i);
      }
            if (!(written & (1 << b))) {
      struct nouveau_bo *bo;
   const uint32_t bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART;
   written |= 1 << b;
   address[b] = nouveau_scratch_data(&nvc0->base, vb->buffer.user,
                                    BEGIN_1IC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_SELECT), 5);
   PUSH_DATA (push, i);
   PUSH_DATAh(push, address[b] + base + size - 1);
   PUSH_DATA (push, address[b] + base + size - 1);
   PUSH_DATAh(push, address[b] + ve->src_offset);
      }
      }
      static void
   nvc0_update_user_vbufs_shared(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            PUSH_SPACE(push, nvc0->num_vtxbufs * 8);
   while (mask) {
      struct nouveau_bo *bo;
   const uint32_t bo_flags = NOUVEAU_BO_RD | NOUVEAU_BO_GART;
   uint64_t address;
   uint32_t base, size;
   const int b = ffs(mask) - 1;
                     address = nouveau_scratch_data(&nvc0->base, nvc0->vtxbuf[b].buffer.user,
         if (bo)
            BEGIN_1IC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_SELECT), 5);
   PUSH_DATA (push, b);
   PUSH_DATAh(push, address + base + size - 1);
   PUSH_DATA (push, address + base + size - 1);
   PUSH_DATAh(push, address);
                        mask = nvc0->state.constant_elts;
   while (mask) {
      int i = ffs(mask) - 1;
   mask &= ~(1 << i);
         }
      static void
   nvc0_validate_vertex_buffers(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   const struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   uint32_t refd = 0;
            PUSH_SPACE(push, vertex->num_elements * 8);
   for (i = 0; i < vertex->num_elements; ++i) {
      const struct nvc0_vertex_element *ve;
   const struct pipe_vertex_buffer *vb;
   struct nv04_resource *res;
   unsigned b;
            if (nvc0->state.constant_elts & (1 << i))
         ve = &vertex->element[i];
   b = ve->pipe.vertex_buffer_index;
            if (nvc0->vbo_user & (1 << b)) {
      if (!(nvc0->constant_vbos & (1 << b))) {
      if (ve->pipe.instance_divisor) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_DIVISOR(i)), 1);
      }
   BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 1);
      }
   /* address/value set in nvc0_update_user_vbufs */
      }
   res = nv04_resource(vb->buffer.resource);
   offset = ve->pipe.src_offset + vb->buffer_offset;
            if (unlikely(ve->pipe.instance_divisor)) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 4);
   PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | vertex->strides[b]);
   PUSH_DATAh(push, res->address + offset);
   PUSH_DATA (push, res->address + offset);
      } else {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(i)), 3);
   PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | vertex->strides[b]);
   PUSH_DATAh(push, res->address + offset);
               if (nvc0->screen->eng3d->oclass < TU102_3D_CLASS)
         else
         PUSH_DATAh(push, res->address + limit);
            if (!(refd & (1 << b))) {
      refd |= 1 << b;
         }
   if (nvc0->vbo_user)
      }
      static void
   nvc0_validate_vertex_buffers_shared(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   unsigned b;
            PUSH_SPACE(push, nvc0->num_vtxbufs * 8 + nvc0->vertex->num_elements);
   for (b = 0; b < nvc0->num_vtxbufs; ++b) {
      struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[b];
   struct nv04_resource *buf;
            if (mask & (1 << b)) {
      if (!(nvc0->constant_vbos & (1 << b))) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(b)), 1);
      }
   /* address/value set in nvc0_update_user_vbufs_shared */
      } else if (!vb->buffer.resource) {
      /* there can be holes in the vertex buffer lists */
   IMMED_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(b)), 0);
      }
   buf = nv04_resource(vb->buffer.resource);
   offset = vb->buffer_offset;
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(b)), 3);
   PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | nvc0->vertex->strides[b]);
   PUSH_DATAh(push, buf->address + offset);
            if (nvc0->screen->eng3d->oclass < TU102_3D_CLASS)
         else
         PUSH_DATAh(push, buf->address + limit);
               }
   /* If there are more elements than buffers, we might not have unset
   * fetching on the later elements.
   */
   for (; b < nvc0->vertex->num_elements; ++b)
            if (nvc0->vbo_user)
      }
      void
   nvc0_vertex_arrays_validate(struct nvc0_context *nvc0)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_vertex_stateobj *vertex = nvc0->vertex;
   struct nvc0_vertex_element *ve;
   uint32_t const_vbos;
   unsigned i;
   uint8_t vbo_mode;
                     assert(vertex);
   if (unlikely(vertex->need_conversion) ||
      unlikely(nvc0->vertprog->vp.edgeflag < PIPE_MAX_ATTRIBS)) {
      } else if (nvc0->vbo_user & ~nvc0->constant_vbos) {
         } else {
         }
            update_vertex = (nvc0->dirty_3d & NVC0_NEW_3D_VERTEX) ||
      (const_vbos != nvc0->state.constant_vbos) ||
         if (update_vertex) {
               simple_mtx_assert_locked(&nvc0->screen->state_lock);
   nvc0->state.constant_vbos = const_vbos;
   nvc0->state.constant_elts = 0;
   nvc0->state.num_vtxelts = vertex->num_elements;
            if (unlikely(vbo_mode)) {
      if (unlikely(nvc0->state.instance_elts & 3)) {
      /* translate mode uses only 2 vertex buffers */
   nvc0->state.instance_elts &= ~3;
   PUSH_SPACE(push, 3);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_PER_INSTANCE(0)), 2);
   PUSH_DATA (push, 0);
                        BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(0)), n);
   for (i = 0; i < vertex->num_elements; ++i)
                        BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(0)), 1);
   PUSH_DATA (push, (1 << 12) | vertex->size);
   for (i = 1; i < n; ++i)
      } else {
               if (unlikely(vertex->instance_elts != nvc0->state.instance_elts)) {
      nvc0->state.instance_elts = vertex->instance_elts;
   assert(n); /* if (n == 0), both masks should be 0 */
   PUSH_SPACE(push, 3);
   BEGIN_NVC0(push, NVC0_3D(MACRO_VERTEX_ARRAY_PER_INSTANCE), 2);
   PUSH_DATA (push, n);
               PUSH_SPACE(push, n * 2 + 1);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(0)), n);
   data = push->cur;
   push->cur += n;
   for (i = 0; i < vertex->num_elements; ++i) {
      ve = &vertex->element[i];
   data[i] = ve->state;
   if (unlikely(const_vbos & (1 << ve->pipe.vertex_buffer_index))) {
      nvc0->state.constant_elts |= 1 << i;
   data[i] |= NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST;
         }
   for (; i < n; ++i) {
      data[i] = NVC0_3D_VERTEX_ATTRIB_INACTIVE;
            }
   if (nvc0->state.vbo_mode) /* using translate, don't set up arrays here */
            if (vertex->shared_slots)
         else
      }
      #define NVC0_PRIM_GL_CASE(n) \
            static inline unsigned
   nvc0_prim_gl(unsigned prim)
   {
      switch (prim) {
   NVC0_PRIM_GL_CASE(POINTS);
   NVC0_PRIM_GL_CASE(LINES);
   NVC0_PRIM_GL_CASE(LINE_LOOP);
   NVC0_PRIM_GL_CASE(LINE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLES);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP);
   NVC0_PRIM_GL_CASE(TRIANGLE_FAN);
   NVC0_PRIM_GL_CASE(QUADS);
   NVC0_PRIM_GL_CASE(QUAD_STRIP);
   NVC0_PRIM_GL_CASE(POLYGON);
   NVC0_PRIM_GL_CASE(LINES_ADJACENCY);
   NVC0_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NVC0_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   NVC0_PRIM_GL_CASE(PATCHES);
   default:
            }
      static void
   nvc0_draw_vbo_kick_notify(struct nouveau_context *context)
   {
         }
      static void
   nvc0_draw_arrays(struct nvc0_context *nvc0,
               {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (nvc0->state.index_bias) {
      /* index_bias is implied 0 if !info->index_size (really ?) */
   /* TODO: can we deactivate it for the VERTEX_BUFFER_FIRST command ? */
   PUSH_SPACE(push, 2);
   IMMED_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 0);
   IMMED_NVC0(push, NVC0_3D(VERTEX_ID_BASE), 0);
                        while (instance_count--) {
      PUSH_SPACE(push, 6);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (push, prim);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, start);
   PUSH_DATA (push, count);
               }
      }
      static void
   nvc0_draw_elements_inline_u08(struct nouveau_pushbuf *push, const uint8_t *map,
         {
               if (count & 3) {
      unsigned i;
   PUSH_SPACE(push, 4);
   BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U32), count & 3);
   for (i = 0; i < (count & 3); ++i)
            }
   while (count) {
               PUSH_SPACE(push, nr + 1);
   BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U8), nr);
   for (i = 0; i < nr; ++i) {
      PUSH_DATA(push,
            }
         }
      static void
   nvc0_draw_elements_inline_u16(struct nouveau_pushbuf *push, const uint16_t *map,
         {
               if (count & 1) {
      count &= ~1;
   PUSH_SPACE(push, 2);
   BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
      }
   while (count) {
               PUSH_SPACE(push, nr + 1);
   BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U16), nr);
   for (i = 0; i < nr; ++i) {
      PUSH_DATA(push, (map[1] << 16) | map[0]);
      }
         }
      static void
   nvc0_draw_elements_inline_u32(struct nouveau_pushbuf *push, const uint32_t *map,
         {
               while (count) {
               PUSH_SPACE(push, nr + 1);
   BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U32), nr);
            map += nr;
         }
      static void
   nvc0_draw_elements_inline_u32_short(struct nouveau_pushbuf *push,
               {
               if (count & 1) {
      count--;
   PUSH_SPACE(push, 2);
   BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
      }
   while (count) {
               PUSH_SPACE(push, nr + 1);
   BEGIN_NIC0(push, NVC0_3D(VB_ELEMENT_U16), nr);
   for (i = 0; i < nr; ++i) {
      PUSH_DATA(push, (map[1] << 16) | map[0]);
      }
         }
      static void
   nvc0_draw_elements(struct nvc0_context *nvc0, bool shorten,
                           {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
                     if (index_bias != nvc0->state.index_bias) {
      PUSH_SPACE(push, 4);
   BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 1);
   PUSH_DATA (push, index_bias);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_ID_BASE), 1);
   PUSH_DATA (push, index_bias);
               if (!info->has_user_indices) {
      PUSH_SPACE(push, 1);
   IMMED_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), prim);
   do {
      PUSH_SPACE(push, 7);
   BEGIN_NVC0(push, NVC0_3D(INDEX_BATCH_FIRST), 2);
   PUSH_DATA (push, start);
   PUSH_DATA (push, count);
   if (--instance_count) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_END_GL), 2);
   PUSH_DATA (push, 0);
         } while (instance_count);
      } else {
               while (instance_count--) {
      PUSH_SPACE(push, 2);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (push, prim);
   switch (index_size) {
   case 1:
      nvc0_draw_elements_inline_u08(push, data, start, count);
      case 2:
      nvc0_draw_elements_inline_u16(push, data, start, count);
      case 4:
      if (shorten)
         else
            default:
      assert(0);
      }
                        }
      }
      static void
   nvc0_draw_stream_output(struct nvc0_context *nvc0,
               {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_so_target *so = nvc0_so_target(indirect->count_from_stream_output);
   struct nv04_resource *res = nv04_resource(so->pipe.buffer);
   unsigned mode = nvc0_prim_gl(info->mode);
            if (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      res->status &= ~NOUVEAU_BUFFER_STATUS_GPU_WRITING;
   PUSH_SPACE(push, 2);
   IMMED_NVC0(push, NVC0_3D(SERIALIZE), 0);
   nvc0_hw_query_fifo_wait(nvc0, nvc0_query(so->pq));
   if (nvc0->screen->eng3d->oclass < GM107_3D_CLASS)
                        while (num_instances--) {
      PUSH_SPACE_EX(push, 16, 0, 1);
   BEGIN_NVC0(push, NVC0_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (push, mode);
   BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_BASE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_STRIDE), 1);
   PUSH_DATA (push, so->stride);
   BEGIN_NVC0(push, NVC0_3D(DRAW_TFB_BYTES), 1);
   nvc0_hw_query_pushbuf_submit(push, nvc0_query(so->pq), 0x4);
                  }
      static void
   nvc0_draw_indirect(struct nvc0_context *nvc0, const struct pipe_draw_info *info,
               {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(indirect->buffer);
   struct nv04_resource *buf_count = nv04_resource(indirect->indirect_draw_count);
   unsigned size, macro, count = indirect->draw_count, drawid = drawid_offset;
   uint32_t offset = buf->offset + indirect->offset;
                     /* must make FIFO wait for engines idle before continuing to process */
   if ((buf->fence_wr && !nouveau_fence_signalled(buf->fence_wr)) ||
      (buf_count && buf_count->fence_wr &&
   !nouveau_fence_signalled(buf_count->fence_wr))) {
               /* Queue things up to let the macros write params to the driver constbuf */
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   BEGIN_NVC0(push, NVC0_3D(CB_POS), 1);
            if (info->index_size) {
      assert(!info->has_user_indices);
   assert(nouveau_resource_mapped_by_gpu(info->index.resource));
   size = 5;
   if (buf_count)
         else
      } else {
      if (nvc0->state.index_bias) {
      /* index_bias is implied 0 if !info->index_size (really ?) */
   IMMED_NVC0(push, NVC0_3D(VB_ELEMENT_BASE), 0);
   IMMED_NVC0(push, NVC0_3D(VERTEX_ID_BASE), 0);
      }
   size = 4;
   if (buf_count)
         else
               /* If the stride is not the natural stride, we have to stick a separate
   * push data reference for each draw. Otherwise it can all go in as one.
   * Of course there is a maximum packet size, so we have to break things up
   * along those borders as well.
   */
   while (count) {
      unsigned draws = count, pushes, i;
   if (indirect->stride == size * 4) {
      draws = MIN2(draws, (NV04_PFIFO_MAX_PACKET_LEN - 4) / size);
      } else {
      draws = MIN2(draws, 32);
               PUSH_SPACE_EX(push, 16, 0, pushes + !!buf_count);
   PUSH_REF1(push, buf->bo, NOUVEAU_BO_RD | buf->domain);
   if (buf_count)
         PUSH_DATA(push,
         PUSH_DATA(push, nvc0_prim_gl(info->mode));
   PUSH_DATA(push, drawid);
   PUSH_DATA(push, draws);
   if (buf_count) {
      nouveau_pushbuf_data(push,
                  }
   if (pushes == 1) {
      nouveau_pushbuf_data(push,
                  } else {
      for (i = 0; i < pushes; i++) {
      nouveau_pushbuf_data(push,
                     }
   count -= draws;
         }
      static inline void
   nvc0_update_prim_restart(struct nvc0_context *nvc0, bool en, uint32_t index)
   {
               if (en != nvc0->state.prim_restart) {
      if (en) {
      BEGIN_NVC0(push, NVC0_3D(PRIM_RESTART_ENABLE), 2);
   PUSH_DATA (push, 1);
      } else {
         }
      } else
   if (en) {
      BEGIN_NVC0(push, NVC0_3D(PRIM_RESTART_INDEX), 1);
         }
      void
   nvc0_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
               unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   {
      if (num_draws > 1) {
      util_draw_multi(pipe, info, drawid_offset, indirect, draws, num_draws);
               if (!indirect && (!draws[0].count || !info->instance_count))
            struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   unsigned vram_domain = NV_VRAM_DOMAIN(&screen->base);
            /* NOTE: caller must ensure that (min_index + index_bias) is >= 0 */
   if (info->index_bounds_valid) {
      nvc0->vb_elt_first = info->min_index + (info->index_size ? draws->index_bias : 0);
      } else {
      nvc0->vb_elt_first = 0;
      }
   nvc0->instance_off = info->start_instance;
            /* For picking only a few vertices from a large user buffer, push is better,
   * if index count is larger and we expect repeated vertices, suggest upload.
   */
   nvc0->vbo_push_hint =
      (!indirect || indirect->count_from_stream_output) && info->index_size &&
         if (nvc0->dirty_3d & (NVC0_NEW_3D_ARRAYS | NVC0_NEW_3D_VERTEX))
         /* Check whether we want to switch vertex-submission mode. */
   if (nvc0->vbo_user && !(nvc0->dirty_3d & (NVC0_NEW_3D_ARRAYS | NVC0_NEW_3D_VERTEX))) {
      if (nvc0->vbo_push_hint != !!nvc0->state.vbo_mode)
                  if (!(nvc0->dirty_3d & NVC0_NEW_3D_ARRAYS) && nvc0->state.vbo_mode == 0) {
      if (nvc0->vertex->shared_slots)
         else
                  if (info->mode == MESA_PRIM_PATCHES &&
      nvc0->state.patch_vertices != nvc0->patch_vertices) {
   nvc0->state.patch_vertices = nvc0->patch_vertices;
   PUSH_SPACE(push, 1);
               if (info->index_size && !info->has_user_indices) {
               assert(buf);
            PUSH_SPACE(push, 6);
   if (nvc0->screen->eng3d->oclass < TU102_3D_CLASS) {
      BEGIN_NVC0(push, NVC0_3D(INDEX_ARRAY_START_HIGH), 5);
   PUSH_DATAh(push, buf->address);
   PUSH_DATA (push, buf->address);
   PUSH_DATAh(push, buf->address + buf->base.width0 - 1);
   PUSH_DATA (push, buf->address + buf->base.width0 - 1);
      } else {
      BEGIN_NVC0(push, NVC0_3D(INDEX_ARRAY_START_HIGH), 2);
   PUSH_DATAh(push, buf->address);
   PUSH_DATA (push, buf->address);
   BEGIN_NVC0(push, SUBC_3D(TU102_3D_INDEX_ARRAY_LIMIT_HIGH), 2);
   PUSH_DATAh(push, buf->address + buf->base.width0 - 1);
   PUSH_DATA (push, buf->address + buf->base.width0 - 1);
   BEGIN_NVC0(push, NVC0_3D(INDEX_FORMAT), 1);
                           list_for_each_entry(struct nvc0_resident, resident, &nvc0->tex_head, list) {
      nvc0_add_resident(nvc0->bufctx_3d, NVC0_BIND_3D_BINDLESS, resident->buf,
               list_for_each_entry(struct nvc0_resident, resident, &nvc0->img_head, list) {
      nvc0_add_resident(nvc0->bufctx_3d, NVC0_BIND_3D_BINDLESS, resident->buf,
               BCTX_REFN_bo(nvc0->bufctx_3d, 3D_TEXT, vram_domain | NOUVEAU_BO_RD,
                              if (nvc0->vertprog->vp.need_draw_parameters && (!indirect || indirect->count_from_stream_output)) {
      PUSH_SPACE(push, 9);
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 3);
   PUSH_DATA (push, NVC0_CB_AUX_DRAW_INFO);
   PUSH_DATA (push, info->index_size ? draws->index_bias : 0);
   PUSH_DATA (push, info->start_instance);
               if (nvc0->screen->base.class_3d < NVE4_3D_CLASS &&
      nvc0->seamless_cube_map != nvc0->state.seamless_cube_map) {
   nvc0->state.seamless_cube_map = nvc0->seamless_cube_map;
   PUSH_SPACE(push, 1);
   IMMED_NVC0(push, NVC0_3D(TEX_MISC),
                        for (s = 0; s < 5 && !nvc0->cb_dirty; ++s) {
      if (nvc0->constbuf_coherent[s])
               if (nvc0->cb_dirty) {
      PUSH_SPACE(push, 1);
   IMMED_NVC0(push, NVC0_3D(MEM_BARRIER), 0x1011);
               for (s = 0; s < 5; ++s) {
      if (!nvc0->textures_coherent[s])
                     for (int i = 0; i < nvc0->num_textures[s]; ++i) {
      struct nv50_tic_entry *tic = nv50_tic_entry(nvc0->textures[s][i]);
                  BEGIN_NVC0(push, NVC0_3D(TEX_CACHE_CTL), 1);
   PUSH_DATA (push, (tic->id << 4) | 1);
                  if (nvc0->state.vbo_mode) {
      if (indirect && indirect->buffer)
         else
                     /* space for base instance, flush, and prim restart */
            if (nvc0->state.instance_base != info->start_instance) {
      nvc0->state.instance_base = info->start_instance;
   /* NOTE: this does not affect the shader input, should it ? */
   BEGIN_NVC0(push, NVC0_3D(VB_INSTANCE_BASE), 1);
                        if (!nvc0->base.vbo_dirty && info->index_size && !info->has_user_indices &&
      info->index.resource->flags & PIPE_RESOURCE_FLAG_MAP_COHERENT)
                  if (nvc0->base.vbo_dirty) {
      if (nvc0->screen->eng3d->oclass < GM107_3D_CLASS)
                     if (unlikely(indirect && indirect->buffer)) {
         } else
   if (unlikely(indirect && indirect->count_from_stream_output)) {
         } else
   if (info->index_size) {
               if (info->primitive_restart && info->restart_index > 65535)
            nvc0_draw_elements(nvc0, shorten, info,
            } else {
      nvc0_draw_arrays(nvc0,
                  cleanup:
      PUSH_KICK(push);
                                       nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_TEXT);
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_IDX);
      }
