      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "translate/translate.h"
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_resource.h"
      #include "nvc0/nvc0_3d.xml.h"
      struct push_context {
               struct translate *translate;
   void *dest;
            uint32_t vertex_size;
   uint32_t restart_index;
   uint32_t start_instance;
            bool prim_restart;
            struct {
      bool enabled;
   bool value;
   uint8_t width;
   unsigned stride;
         };
      static void nvc0_push_upload_vertex_ids(struct push_context *,
                        static void
   nvc0_push_context_init(struct nvc0_context *nvc0, struct push_context *ctx)
   {
               ctx->translate = nvc0->vertex->translate;
   ctx->vertex_size = nvc0->vertex->size;
            ctx->need_vertex_id =
            ctx->edgeflag.value = true;
            /* silence warnings */
   ctx->edgeflag.data = NULL;
   ctx->edgeflag.stride = 0;
      }
      static inline void
   nvc0_vertex_configure_translate(struct nvc0_context *nvc0, int32_t index_bias)
   {
      struct translate *translate = nvc0->vertex->translate;
            for (i = 0; i < nvc0->num_vtxbufs; ++i) {
      const uint8_t *map;
            if (likely(vb->is_user_buffer))
         else {
                     map = nouveau_resource_map_offset(&nvc0->base,
               if (index_bias && !unlikely(nvc0->vertex->instance_bufs & (1 << i)))
                  }
      static inline void
   nvc0_push_map_idxbuf(struct push_context *ctx, struct nvc0_context *nvc0,
         {
      if (!info->has_user_indices) {
      struct nv04_resource *buf = nv04_resource(info->index.resource);
   ctx->idxbuf = nouveau_resource_map_offset(
      } else {
            }
      static inline void
   nvc0_push_map_edgeflag(struct push_context *ctx, struct nvc0_context *nvc0,
         {
      unsigned attr = nvc0->vertprog->vp.edgeflag;
   struct pipe_vertex_element *ve = &nvc0->vertex->element[attr].pipe;
   struct pipe_vertex_buffer *vb = &nvc0->vtxbuf[ve->vertex_buffer_index];
            ctx->edgeflag.stride = ve->src_stride;
   ctx->edgeflag.width = util_format_get_blocksize(ve->src_format);
   if (!vb->is_user_buffer) {
      unsigned offset = vb->buffer_offset + ve->src_offset;
   ctx->edgeflag.data = nouveau_resource_map_offset(&nvc0->base,
      } else {
                  if (index_bias)
      }
      static inline unsigned
   prim_restart_search_i08(const uint8_t *elts, unsigned push, uint8_t index)
   {
      unsigned i;
   for (i = 0; i < push && elts[i] != index; ++i);
      }
      static inline unsigned
   prim_restart_search_i16(const uint16_t *elts, unsigned push, uint16_t index)
   {
      unsigned i;
   for (i = 0; i < push && elts[i] != index; ++i);
      }
      static inline unsigned
   prim_restart_search_i32(const uint32_t *elts, unsigned push, uint32_t index)
   {
      unsigned i;
   for (i = 0; i < push && elts[i] != index; ++i);
      }
      static inline bool
   ef_value_8(const struct push_context *ctx, uint32_t index)
   {
      uint8_t *pf = (uint8_t *)&ctx->edgeflag.data[index * ctx->edgeflag.stride];
      }
      static inline bool
   ef_value_32(const struct push_context *ctx, uint32_t index)
   {
      uint32_t *pf = (uint32_t *)&ctx->edgeflag.data[index * ctx->edgeflag.stride];
      }
      static inline bool
   ef_toggle(struct push_context *ctx)
   {
      ctx->edgeflag.value = !ctx->edgeflag.value;
      }
      static inline unsigned
   ef_toggle_search_i08(struct push_context *ctx, const uint8_t *elts, unsigned n)
   {
      unsigned i;
   bool ef = ctx->edgeflag.value;
   if (ctx->edgeflag.width == 1)
         else
            }
      static inline unsigned
   ef_toggle_search_i16(struct push_context *ctx, const uint16_t *elts, unsigned n)
   {
      unsigned i;
   bool ef = ctx->edgeflag.value;
   if (ctx->edgeflag.width == 1)
         else
            }
      static inline unsigned
   ef_toggle_search_i32(struct push_context *ctx, const uint32_t *elts, unsigned n)
   {
      unsigned i;
   bool ef = ctx->edgeflag.value;
   if (ctx->edgeflag.width == 1)
         else
            }
      static inline unsigned
   ef_toggle_search_seq(struct push_context *ctx, unsigned start, unsigned n)
   {
      unsigned i;
   bool ef = ctx->edgeflag.value;
   if (ctx->edgeflag.width == 1)
         else
            }
      static inline void *
   nvc0_push_setup_vertex_array(struct nvc0_context *nvc0, const unsigned count)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nouveau_bo *bo;
   uint64_t va;
                     BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_START_HIGH(0)), 2);
   PUSH_DATAh(push, va);
            if (nvc0->screen->eng3d->oclass < TU102_3D_CLASS)
         else
         PUSH_DATAh(push, va + size - 1);
            BCTX_REFN_bo(nvc0->bufctx_3d, 3D_VTX_TMP, NOUVEAU_BO_GART | NOUVEAU_BO_RD,
                     }
      static void
   disp_vertices_i08(struct push_context *ctx, unsigned start, unsigned count)
   {
      struct nouveau_pushbuf *push = ctx->push;
   struct translate *translate = ctx->translate;
   const uint8_t *restrict elts = (uint8_t *)ctx->idxbuf + start;
            do {
               if (unlikely(ctx->prim_restart))
            translate->run_elts8(translate, elts, nR,
         count -= nR;
            while (nR) {
                              PUSH_SPACE(push, 4);
   if (likely(nE >= 2)) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, pos);
      } else
   if (nE) {
      if (pos <= 0xff) {
         } else {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
         }
                  pos += nE;
   elts += nE;
      }
   if (count) {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
   PUSH_DATA (push, 0xffffffff);
   ++elts;
   ctx->dest += ctx->vertex_size;
   ++pos;
            }
      static void
   disp_vertices_i16(struct push_context *ctx, unsigned start, unsigned count)
   {
      struct nouveau_pushbuf *push = ctx->push;
   struct translate *translate = ctx->translate;
   const uint16_t *restrict elts = (uint16_t *)ctx->idxbuf + start;
            do {
               if (unlikely(ctx->prim_restart))
            translate->run_elts16(translate, elts, nR,
         count -= nR;
            while (nR) {
                              PUSH_SPACE(push, 4);
   if (likely(nE >= 2)) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, pos);
      } else
   if (nE) {
      if (pos <= 0xff) {
         } else {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
         }
                  pos += nE;
   elts += nE;
      }
   if (count) {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
   PUSH_DATA (push, 0xffffffff);
   ++elts;
   ctx->dest += ctx->vertex_size;
   ++pos;
            }
      static void
   disp_vertices_i32(struct push_context *ctx, unsigned start, unsigned count)
   {
      struct nouveau_pushbuf *push = ctx->push;
   struct translate *translate = ctx->translate;
   const uint32_t *restrict elts = (uint32_t *)ctx->idxbuf + start;
            do {
               if (unlikely(ctx->prim_restart))
            translate->run_elts(translate, elts, nR,
         count -= nR;
            while (nR) {
                              PUSH_SPACE(push, 4);
   if (likely(nE >= 2)) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, pos);
      } else
   if (nE) {
      if (pos <= 0xff) {
         } else {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
         }
                  pos += nE;
   elts += nE;
      }
   if (count) {
      BEGIN_NVC0(push, NVC0_3D(VB_ELEMENT_U32), 1);
   PUSH_DATA (push, 0xffffffff);
   ++elts;
   ctx->dest += ctx->vertex_size;
   ++pos;
            }
      static void
   disp_vertices_seq(struct push_context *ctx, unsigned start, unsigned count)
   {
      struct nouveau_pushbuf *push = ctx->push;
   struct translate *translate = ctx->translate;
            /* XXX: This will read the data corresponding to the primitive restart index,
   *  maybe we should avoid that ?
   */
   translate->run(translate, start, count,
         do {
               if (unlikely(ctx->edgeflag.enabled))
            PUSH_SPACE(push, 4);
   if (likely(nr)) {
      BEGIN_NVC0(push, NVC0_3D(VERTEX_BUFFER_FIRST), 2);
   PUSH_DATA (push, pos);
      }
   if (unlikely(nr != count))
            pos += nr;
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
      typedef struct {
      uint32_t count;
   uint32_t primCount;
   uint32_t first;
      } DrawArraysIndirectCommand;
      typedef struct {
      uint32_t count;
   uint32_t primCount;
   uint32_t firstIndex;
   int32_t  baseVertex;
      } DrawElementsIndirectCommand;
      void
   nvc0_push_vbo_indirect(struct nvc0_context *nvc0, const struct pipe_draw_info *info,
                     {
      /* The strategy here is to just read the commands from the indirect buffer
   * and do the draws. This is suboptimal, but will only happen in the case
   * that conversion is required for FIXED or DOUBLE inputs.
   */
   struct nvc0_screen *screen = nvc0->screen;
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nv04_resource *buf = nv04_resource(indirect->buffer);
   struct nv04_resource *buf_count = nv04_resource(indirect->indirect_draw_count);
            unsigned draw_count = indirect->draw_count;
   if (buf_count) {
      uint32_t *count = nouveau_resource_map_offset(
         &nvc0->base, buf_count, indirect->indirect_draw_count_offset,
               uint8_t *buf_data = nouveau_resource_map_offset(
         struct pipe_draw_info single = *info;
   struct pipe_draw_start_count_bias sdraw = *draw;
   for (i = 0; i < draw_count; i++, buf_data += indirect->stride) {
      if (info->index_size) {
      DrawElementsIndirectCommand *cmd = (void *)buf_data;
   sdraw.start = draw->start + cmd->firstIndex;
   sdraw.count = cmd->count;
   single.start_instance = cmd->baseInstance;
   single.instance_count = cmd->primCount;
      } else {
      DrawArraysIndirectCommand *cmd = (void *)buf_data;
   sdraw.start = cmd->first;
   sdraw.count = cmd->count;
   single.start_instance = cmd->baseInstance;
               if (nvc0->vertprog->vp.need_draw_parameters) {
      PUSH_SPACE(push, 9);
   BEGIN_NVC0(push, NVC0_3D(CB_SIZE), 3);
   PUSH_DATA (push, NVC0_CB_AUX_SIZE);
   PUSH_DATAh(push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   PUSH_DATA (push, screen->uniform_bo->offset + NVC0_CB_AUX_INFO(0));
   BEGIN_1IC0(push, NVC0_3D(CB_POS), 1 + 3);
   PUSH_DATA (push, NVC0_CB_AUX_DRAW_INFO);
   PUSH_DATA (push, sdraw.index_bias);
   PUSH_DATA (push, single.start_instance);
                           nouveau_resource_unmap(buf);
   if (buf_count)
      }
      void
   nvc0_push_vbo(struct nvc0_context *nvc0, const struct pipe_draw_info *info,
               {
      struct push_context ctx;
   unsigned i, index_size;
   unsigned index_bias = info->index_size ? draw->index_bias : 0;
   unsigned inst_count = info->instance_count;
   unsigned vert_count = draw->count;
                              if (nvc0->state.index_bias) {
      /* this is already taken care of by translate */
   IMMED_NVC0(ctx.push, NVC0_3D(VB_ELEMENT_BASE), 0);
               if (unlikely(ctx.edgeflag.enabled))
            ctx.prim_restart = info->primitive_restart;
            if (info->primitive_restart) {
      /* NOTE: I hope we won't ever need that last index (~0).
   * If we do, we have to disable primitive restart here always and
   * use END,BEGIN to restart. (XXX: would that affect PrimitiveID ?)
   * We could also deactivate PRIM_RESTART_WITH_DRAW_ARRAYS temporarily,
   * and add manual restart to disp_vertices_seq.
   */
   BEGIN_NVC0(ctx.push, NVC0_3D(PRIM_RESTART_ENABLE), 2);
   PUSH_DATA (ctx.push, 1);
      } else
   if (nvc0->state.prim_restart) {
         }
            if (info->index_size) {
      nvc0_push_map_idxbuf(&ctx, nvc0, info);
      } else {
      if (unlikely(indirect && indirect->count_from_stream_output)) {
      struct pipe_context *pipe = &nvc0->base.pipe;
   struct nvc0_so_target *targ;
   targ = nvc0_so_target(indirect->count_from_stream_output);
   pipe->get_query_result(pipe, targ->pq, true, (void *)&vert_count);
      }
   ctx.idxbuf = NULL; /* shut up warnings */
                        prim = nvc0_prim_gl(info->mode);
   do {
               ctx.dest = nvc0_push_setup_vertex_array(nvc0, vert_count);
   if (unlikely(!ctx.dest))
            if (unlikely(ctx.need_vertex_id))
            if (nvc0->screen->eng3d->oclass < GM107_3D_CLASS)
         BEGIN_NVC0(ctx.push, NVC0_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (ctx.push, prim);
   switch (index_size) {
   case 1:
      disp_vertices_i08(&ctx, draw->start, vert_count);
      case 2:
      disp_vertices_i16(&ctx, draw->start, vert_count);
      case 4:
      disp_vertices_i32(&ctx, draw->start, vert_count);
      default:
      assert(index_size == 0);
   disp_vertices_seq(&ctx, draw->start, vert_count);
      }
   PUSH_SPACE(ctx.push, 1);
            if (--inst_count) {
      prim |= NVC0_3D_VERTEX_BEGIN_GL_INSTANCE_NEXT;
      }
   nouveau_bufctx_reset(nvc0->bufctx_3d, NVC0_BIND_3D_VTX_TMP);
                           if (unlikely(!ctx.edgeflag.value)) {
      PUSH_SPACE(ctx.push, 1);
               if (unlikely(ctx.need_vertex_id)) {
      PUSH_SPACE(ctx.push, 4);
   IMMED_NVC0(ctx.push, NVC0_3D(VERTEX_ID_REPLACE), 0);
   BEGIN_NVC0(ctx.push, NVC0_3D(VERTEX_ATTRIB_FORMAT(1)), 1);
   PUSH_DATA (ctx.push,
            NVC0_3D_VERTEX_ATTRIB_FORMAT_CONST |
                  if (info->index_size && !info->has_user_indices)
         for (i = 0; i < nvc0->num_vtxbufs; ++i)
               }
      static inline void
   copy_indices_u8(uint32_t *dst, const uint8_t *elts, uint32_t bias, unsigned n)
   {
      unsigned i;
   for (i = 0; i < n; ++i)
      }
      static inline void
   copy_indices_u16(uint32_t *dst, const uint16_t *elts, uint32_t bias, unsigned n)
   {
      unsigned i;
   for (i = 0; i < n; ++i)
      }
      static inline void
   copy_indices_u32(uint32_t *dst, const uint32_t *elts, uint32_t bias, unsigned n)
   {
      unsigned i;
   for (i = 0; i < n; ++i)
      }
      static void
   nvc0_push_upload_vertex_ids(struct push_context *ctx,
                        {
      struct nouveau_pushbuf *push = ctx->push;
   struct nouveau_bo *bo;
   uint64_t va;
   uint32_t *data;
   uint32_t format;
   unsigned index_size = info->index_size;
   unsigned i;
            if (!index_size || draw->index_bias)
         data = (uint32_t *)nouveau_scratch_get(&nvc0->base,
            BCTX_REFN_bo(nvc0->bufctx_3d, 3D_VTX_TMP, NOUVEAU_BO_GART | NOUVEAU_BO_RD,
                  if (info->index_size) {
      if (!draw->index_bias) {
         } else {
      switch (info->index_size) {
   case 1:
      copy_indices_u8(data, ctx->idxbuf, draw->index_bias, draw->count);
      case 2:
      copy_indices_u16(data, ctx->idxbuf, draw->index_bias, draw->count);
      default:
      copy_indices_u32(data, ctx->idxbuf, draw->index_bias, draw->count);
            } else {
      for (i = 0; i < draw->count; ++i)
               format = (1 << NVC0_3D_VERTEX_ATTRIB_FORMAT_BUFFER__SHIFT) |
            switch (index_size) {
   case 1:
      format |= NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_8;
      case 2:
      format |= NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_16;
      default:
      format |= NVC0_3D_VERTEX_ATTRIB_FORMAT_SIZE_32;
                        if (unlikely(nvc0->state.instance_elts & 2)) {
      nvc0->state.instance_elts &= ~2;
               BEGIN_NVC0(push, NVC0_3D(VERTEX_ATTRIB_FORMAT(a)), 1);
            BEGIN_NVC0(push, NVC0_3D(VERTEX_ARRAY_FETCH(1)), 3);
   PUSH_DATA (push, NVC0_3D_VERTEX_ARRAY_FETCH_ENABLE | index_size);
   PUSH_DATAh(push, va);
            if (nvc0->screen->eng3d->oclass < TU102_3D_CLASS)
         else
         PUSH_DATAh(push, va + draw->count * index_size - 1);
         #define NVC0_3D_VERTEX_ID_REPLACE_SOURCE_ATTR_X(a) \
               BEGIN_NVC0(push, NVC0_3D(VERTEX_ID_REPLACE), 1);
      }
