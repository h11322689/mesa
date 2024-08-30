   /*
   * Copyright 2018 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "si_query.h"
   #include "sid.h"
   #include "util/u_memory.h"
   #include "util/u_suballoc.h"
      #include <stddef.h>
      static void emit_shader_query(struct si_context *sctx, unsigned index)
   {
               struct gfx11_sh_query_buffer *qbuf =
            }
      static void gfx11_release_query_buffers(struct si_context *sctx,
               {
      while (first) {
      struct gfx11_sh_query_buffer *qbuf = first;
   if (first != last)
         else
            qbuf->refcount--;
   if (qbuf->refcount)
            if (qbuf->list.next == &sctx->shader_query_buffers)
         if (qbuf->list.prev == &sctx->shader_query_buffers)
            list_del(&qbuf->list);
   si_resource_reference(&qbuf->buf, NULL);
         }
      static bool gfx11_alloc_query_buffer(struct si_context *sctx)
   {
      if (si_is_atom_dirty(sctx, &sctx->atoms.s.shader_query))
                     if (!list_is_empty(&sctx->shader_query_buffers)) {
      qbuf = list_last_entry(&sctx->shader_query_buffers, struct gfx11_sh_query_buffer, list);
   if (qbuf->head + sizeof(struct gfx11_sh_query_buffer_mem) <= qbuf->buf->b.b.width0)
            qbuf = list_first_entry(&sctx->shader_query_buffers, struct gfx11_sh_query_buffer, list);
   if (!qbuf->refcount &&
      !si_cs_is_buffer_referenced(sctx, qbuf->buf->buf, RADEON_USAGE_READWRITE) &&
   sctx->ws->buffer_wait(sctx->ws, qbuf->buf->buf, 0, RADEON_USAGE_READWRITE)) {
   /* Can immediately re-use the oldest buffer */
      } else {
                     if (!qbuf) {
      qbuf = CALLOC_STRUCT(gfx11_sh_query_buffer);
   if (unlikely(!qbuf))
            struct si_screen *screen = sctx->screen;
   unsigned buf_size =
         qbuf->buf = si_resource(pipe_buffer_create(&screen->b, 0, PIPE_USAGE_STAGING, buf_size));
   if (unlikely(!qbuf->buf)) {
      FREE(qbuf);
                  /* The buffer is currently unused by the GPU. Initialize it.
   *
   * We need to set the high bit of all the primitive counters for
   * compatibility with the SET_PREDICATION packet.
   */
   uint64_t *results = sctx->ws->buffer_map(sctx->ws, qbuf->buf->buf, NULL,
                  for (unsigned i = 0, e = qbuf->buf->b.b.width0 / sizeof(struct gfx11_sh_query_buffer_mem); i < e;
      ++i) {
   for (unsigned j = 0; j < 16; ++j)
                     list_addtail(&qbuf->list, &sctx->shader_query_buffers);
   qbuf->head = 0;
         success:;
      struct pipe_shader_buffer sbuf;
   sbuf.buffer = &qbuf->buf->b.b;
   sbuf.buffer_offset = qbuf->head;
   sbuf.buffer_size = sizeof(struct gfx11_sh_query_buffer_mem);
   si_set_internal_shader_buffer(sctx, SI_GS_QUERY_BUF, &sbuf);
            si_mark_atom_dirty(sctx, &sctx->atoms.s.shader_query);
      }
      static void gfx11_sh_query_destroy(struct si_context *sctx, struct si_query *rquery)
   {
      struct gfx11_sh_query *query = (struct gfx11_sh_query *)rquery;
   gfx11_release_query_buffers(sctx, query->first, query->last);
      }
      static bool gfx11_sh_query_begin(struct si_context *sctx, struct si_query *rquery)
   {
               gfx11_release_query_buffers(sctx, query->first, query->last);
            if (unlikely(!gfx11_alloc_query_buffer(sctx)))
            query->first = list_last_entry(&sctx->shader_query_buffers, struct gfx11_sh_query_buffer, list);
            sctx->num_active_shader_queries++;
               }
      static bool gfx11_sh_query_end(struct si_context *sctx, struct si_query *rquery)
   {
               if (unlikely(!query->first))
            query->last = list_last_entry(&sctx->shader_query_buffers, struct gfx11_sh_query_buffer, list);
            /* Signal the fence of the previous chunk */
   if (query->last_end != 0) {
      uint64_t fence_va = query->last->buf->gpu_address;
   fence_va += query->last_end - sizeof(struct gfx11_sh_query_buffer_mem);
   fence_va += offsetof(struct gfx11_sh_query_buffer_mem, fence);
   si_cp_release_mem(sctx, &sctx->gfx_cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM,
                              if (sctx->num_active_shader_queries <= 0 || !si_is_atom_dirty(sctx, &sctx->atoms.s.shader_query)) {
      si_set_internal_shader_buffer(sctx, SI_GS_QUERY_BUF, NULL);
            /* If a query_begin is followed by a query_end without a draw
   * in-between, we need to clear the atom to ensure that the
   * next query_begin will re-initialize the shader buffer. */
                  }
      static void gfx11_sh_query_add_result(struct gfx11_sh_query *query,
               {
               switch (query->b.type) {
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      result->u64 += qmem->stream[query->stream].emitted_primitives & mask;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      result->u64 += qmem->stream[query->stream].generated_primitives & mask;
      case PIPE_QUERY_SO_STATISTICS:
      result->so_statistics.num_primitives_written +=
         result->so_statistics.primitives_storage_needed +=
            case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      result->b |= qmem->stream[query->stream].emitted_primitives !=
            case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      for (unsigned stream = 0; stream < SI_MAX_STREAMS; ++stream) {
      result->b |= qmem->stream[stream].emitted_primitives !=
      }
      default:
            }
      static bool gfx11_sh_query_get_result(struct si_context *sctx, struct si_query *rquery, bool wait,
         {
                        if (unlikely(!query->first))
                  for (struct gfx11_sh_query_buffer *qbuf = query->last;;
      qbuf = list_entry(qbuf->list.prev, struct gfx11_sh_query_buffer, list)) {
   unsigned usage = PIPE_MAP_READ | (wait ? 0 : PIPE_MAP_DONTBLOCK);
            if (rquery->b.flushed)
         else
            if (!map)
            unsigned results_begin = 0;
   unsigned results_end = qbuf->head;
   if (qbuf == query->first)
         if (qbuf == query->last)
            while (results_begin != results_end) {
                                 if (qbuf == query->first)
                  }
      static void gfx11_sh_query_get_result_resource(struct si_context *sctx, struct si_query *rquery,
                           {
      struct gfx11_sh_query *query = (struct gfx11_sh_query *)rquery;
   struct si_qbo_state saved_state = {};
   struct pipe_resource *tmp_buffer = NULL;
            if (!sctx->sh_query_result_shader) {
      sctx->sh_query_result_shader = gfx11_create_sh_query_result_cs(sctx);
   if (!sctx->sh_query_result_shader)
               if (query->first != query->last) {
      u_suballocator_alloc(&sctx->allocator_zeroed_memory, 16, 16, &tmp_buffer_offset, &tmp_buffer);
   if (!tmp_buffer)
                        /* Pre-fill the constants configuring the shader behavior. */
   struct {
      uint32_t config;
   uint32_t offset;
   uint32_t chain;
      } consts;
            if (index >= 0) {
      switch (query->b.type) {
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      consts.offset = 4 * sizeof(uint64_t) * query->stream + 2 * sizeof(uint64_t);
   consts.config = 0;
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      consts.offset = 4 * sizeof(uint64_t) * query->stream + 3 * sizeof(uint64_t);
   consts.config = 0;
      case PIPE_QUERY_SO_STATISTICS:
      consts.offset = sizeof(uint32_t) * (4 * index + query->stream);
   consts.config = 0;
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      consts.offset = 4 * sizeof(uint64_t) * query->stream;
   consts.config = 2;
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      consts.offset = 0;
   consts.config = 3;
      default:
            } else {
      /* Check result availability. */
   consts.offset = 0;
               if (result_type == PIPE_QUERY_TYPE_I64 || result_type == PIPE_QUERY_TYPE_U64)
            constant_buffer.buffer_size = sizeof(consts);
            /* Pre-fill the SSBOs and grid. */
   struct pipe_shader_buffer ssbo[3];
            ssbo[1].buffer = tmp_buffer;
   ssbo[1].buffer_offset = tmp_buffer_offset;
                     grid.block[0] = 1;
   grid.block[1] = 1;
   grid.block[2] = 1;
   grid.grid[0] = 1;
   grid.grid[1] = 1;
            struct gfx11_sh_query_buffer *qbuf = query->first;
   for (;;) {
      unsigned begin = qbuf == query->first ? query->first_begin : 0;
   unsigned end = qbuf == query->last ? query->last_end : qbuf->buf->b.b.width0;
   if (!end)
            ssbo[0].buffer = &qbuf->buf->b.b;
   ssbo[0].buffer_offset = begin;
            consts.result_count = (end - begin) / sizeof(struct gfx11_sh_query_buffer_mem);
   consts.chain = 0;
   if (qbuf != query->first)
         if (qbuf != query->last)
            if (qbuf == query->last) {
      ssbo[2].buffer = resource;
   ssbo[2].buffer_offset = offset;
                        if (flags & PIPE_QUERY_WAIT) {
               /* Wait for result availability. Wait only for readiness
   * of the last entry, since the fence writes should be
   * serialized in the CP.
   */
   va = qbuf->buf->gpu_address;
                              /* ssbo[2] is either tmp_buffer or resource */
   assert(ssbo[2].buffer);
   si_launch_grid_internal_ssbos(sctx, &grid, sctx->sh_query_result_shader,
                  if (qbuf == query->last)
                     si_restore_qbo_state(sctx, &saved_state);
      }
      static const struct si_query_ops gfx11_sh_query_ops = {
      .destroy = gfx11_sh_query_destroy,
   .begin = gfx11_sh_query_begin,
   .end = gfx11_sh_query_end,
   .get_result = gfx11_sh_query_get_result,
      };
      struct pipe_query *gfx11_sh_query_create(struct si_screen *screen, enum pipe_query_type query_type,
         {
      struct gfx11_sh_query *query = CALLOC_STRUCT(gfx11_sh_query);
   if (unlikely(!query))
            query->b.ops = &gfx11_sh_query_ops;
   query->b.type = query_type;
               }
      void si_gfx11_init_query(struct si_context *sctx)
   {
      list_inithead(&sctx->shader_query_buffers);
      }
      void si_gfx11_destroy_query(struct si_context *sctx)
   {
      if (!sctx->shader_query_buffers.next)
            while (!list_is_empty(&sctx->shader_query_buffers)) {
      struct gfx11_sh_query_buffer *qbuf =
                  assert(!qbuf->refcount);
   si_resource_reference(&qbuf->buf, NULL);
         }
