      #include "pipe/p_context.h"
   #include "pipe/p_state.h"
   #include "util/u_inlines.h"
   #include "util/format/u_format.h"
   #include "translate/translate.h"
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_resource.h"
      #include "nv50/nv50_3d.xml.h"
      struct push_context {
                        float edgeflag;
            uint32_t vertex_words;
                              bool need_vertex_id;
            uint32_t prim;
   uint32_t restart_index;
   uint32_t start_instance;
      };
      static inline unsigned
   prim_restart_search_i08(uint8_t *elts, unsigned push, uint8_t index)
   {
      unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         }
      static inline unsigned
   prim_restart_search_i16(uint16_t *elts, unsigned push, uint16_t index)
   {
      unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         }
      static inline unsigned
   prim_restart_search_i32(uint32_t *elts, unsigned push, uint32_t index)
   {
      unsigned i;
   for (i = 0; i < push; ++i)
      if (elts[i] == index)
         }
      static void
   emit_vertices_i08(struct push_context *ctx, unsigned start, unsigned count)
   {
               while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
            nr = push;
   if (ctx->primitive_restart)
                     if (unlikely(ctx->need_vertex_id)) {
      BEGIN_NV04(ctx->push, NV84_3D(VERTEX_ID_BASE), 1);
                        ctx->translate->run_elts8(ctx->translate, elts, nr,
                  ctx->push->cur += size;
   count -= nr;
            if (nr != push) {
      count--;
   elts++;
   BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
            }
      static void
   emit_vertices_i16(struct push_context *ctx, unsigned start, unsigned count)
   {
               while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
            nr = push;
   if (ctx->primitive_restart)
                     if (unlikely(ctx->need_vertex_id)) {
      BEGIN_NV04(ctx->push, NV84_3D(VERTEX_ID_BASE), 1);
                        ctx->translate->run_elts16(ctx->translate, elts, nr,
                  ctx->push->cur += size;
   count -= nr;
            if (nr != push) {
      count--;
   elts++;
   BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
            }
      static void
   emit_vertices_i32(struct push_context *ctx, unsigned start, unsigned count)
   {
               while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
            nr = push;
   if (ctx->primitive_restart)
                     if (unlikely(ctx->need_vertex_id)) {
      BEGIN_NV04(ctx->push, NV84_3D(VERTEX_ID_BASE), 1);
                        ctx->translate->run_elts(ctx->translate, elts, nr,
                  ctx->push->cur += size;
   count -= nr;
            if (nr != push) {
      count--;
   elts++;
   BEGIN_NV04(ctx->push, NV50_3D(VB_ELEMENT_U32), 1);
            }
      static void
   emit_vertices_seq(struct push_context *ctx, unsigned start, unsigned count)
   {
               while (count) {
      unsigned push = MIN2(count, ctx->packet_vertex_limit);
            if (unlikely(ctx->need_vertex_id)) {
      /* For non-indexed draws, gl_VertexID goes up after each vertex. */
   BEGIN_NV04(ctx->push, NV84_3D(VERTEX_ID_BASE), 1);
                        ctx->translate->run(ctx->translate, start, push,
               ctx->push->cur += size;
   count -= push;
         }
         #define NV50_PRIM_GL_CASE(n) \
            static inline unsigned
   nv50_prim_gl(unsigned prim)
   {
      switch (prim) {
   NV50_PRIM_GL_CASE(POINTS);
   NV50_PRIM_GL_CASE(LINES);
   NV50_PRIM_GL_CASE(LINE_LOOP);
   NV50_PRIM_GL_CASE(LINE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLES);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP);
   NV50_PRIM_GL_CASE(TRIANGLE_FAN);
   NV50_PRIM_GL_CASE(QUADS);
   NV50_PRIM_GL_CASE(QUAD_STRIP);
   NV50_PRIM_GL_CASE(POLYGON);
   NV50_PRIM_GL_CASE(LINES_ADJACENCY);
   NV50_PRIM_GL_CASE(LINE_STRIP_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLES_ADJACENCY);
   NV50_PRIM_GL_CASE(TRIANGLE_STRIP_ADJACENCY);
   /*
   NV50_PRIM_GL_CASE(PATCHES); */
   default:
      return NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_POINTS;
         }
      void
   nv50_push_vbo(struct nv50_context *nv50, const struct pipe_draw_info *info,
               {
      struct push_context ctx;
   unsigned i, index_size;
   unsigned inst_count = info->instance_count;
   unsigned vert_count = draw->count;
            ctx.push = nv50->base.pushbuf;
            ctx.need_vertex_id = nv50->screen->base.class_3d >= NV84_3D_CLASS &&
         ctx.index_bias = info->index_size ? draw->index_bias : 0;
            /* For indexed draws, gl_VertexID must be emitted for every vertex. */
   ctx.packet_vertex_limit =
                  assert(nv50->num_vtxbufs <= PIPE_MAX_ATTRIBS);
   for (i = 0; i < nv50->num_vtxbufs; ++i) {
      const struct pipe_vertex_buffer *vb = &nv50->vtxbuf[i];
            if (unlikely(!vb->is_user_buffer)) {
                     data = nouveau_resource_map_offset(&nv50->base,
      } else
            if (apply_bias && likely(!(nv50->vertex->instance_bufs & (1 << i))))
                        if (info->index_size) {
      if (!info->has_user_indices) {
      ctx.idxbuf = nouveau_resource_map_offset(&nv50->base,
      } else {
         }
   if (!ctx.idxbuf)
         index_size = info->index_size;
   ctx.primitive_restart = info->primitive_restart;
      } else {
      if (unlikely(indirect && indirect->count_from_stream_output)) {
      struct pipe_context *pipe = &nv50->base.pipe;
   struct nv50_so_target *targ;
   targ = nv50_so_target(indirect->count_from_stream_output);
   if (!targ->pq) {
      NOUVEAU_ERR("draw_stream_output not supported on pre-NVA0 cards\n");
      }
   pipe->get_query_result(pipe, targ->pq, true, (void *)&vert_count);
      }
   ctx.idxbuf = NULL;
   index_size = 0;
   ctx.primitive_restart = false;
               ctx.start_instance = info->start_instance;
            if (info->primitive_restart) {
      BEGIN_NV04(ctx.push, NV50_3D(PRIM_RESTART_ENABLE), 2);
   PUSH_DATA (ctx.push, 1);
      } else
   if (nv50->state.prim_restart) {
      BEGIN_NV04(ctx.push, NV50_3D(PRIM_RESTART_ENABLE), 1);
      }
            while (inst_count--) {
      BEGIN_NV04(ctx.push, NV50_3D(VERTEX_BEGIN_GL), 1);
   PUSH_DATA (ctx.push, ctx.prim);
   switch (index_size) {
   case 0:
      emit_vertices_seq(&ctx, draw->start, vert_count);
      case 1:
      emit_vertices_i08(&ctx, draw->start, vert_count);
      case 2:
      emit_vertices_i16(&ctx, draw->start, vert_count);
      case 4:
      emit_vertices_i32(&ctx, draw->start, vert_count);
      default:
      assert(0);
      }
   BEGIN_NV04(ctx.push, NV50_3D(VERTEX_END_GL), 1);
            ctx.instance_id++;
               if (unlikely(ctx.need_vertex_id)) {
      /* Reset gl_VertexID to prevent future indexed draws to be confused. */
   BEGIN_NV04(ctx.push, NV84_3D(VERTEX_ID_BASE), 1);
         }
