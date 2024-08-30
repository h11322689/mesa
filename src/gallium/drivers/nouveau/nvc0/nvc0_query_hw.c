   /*
   * Copyright 2011 Christoph Bumiller
   * Copyright 2015 Samuel Pitoiset
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
      #include "nvc0/nvc0_context.h"
   #include "nvc0/nvc0_query_hw.h"
   #include "nvc0/nvc0_query_hw_metric.h"
   #include "nvc0/nvc0_query_hw_sm.h"
      #define NVC0_HW_QUERY_ALLOC_SPACE 256
      bool
   nvc0_hw_query_allocate(struct nvc0_context *nvc0, struct nvc0_query *q,
         {
      struct nvc0_hw_query *hq = nvc0_hw_query(q);
   struct nvc0_screen *screen = nvc0->screen;
            if (hq->bo) {
      nouveau_bo_ref(NULL, &hq->bo);
   if (hq->mm) {
      if (hq->state == NVC0_HW_QUERY_STATE_READY)
         else
      nouveau_fence_work(nvc0->base.fence,
      }
   if (size) {
      hq->mm = nouveau_mm_allocate(screen->base.mm_GART, size, &hq->bo,
         if (!hq->bo)
                  ret = BO_MAP(&screen->base, hq->bo, 0, nvc0->base.client);
   if (ret) {
      nvc0_hw_query_allocate(nvc0, q, 0);
      }
      }
      }
      static void
   nvc0_hw_query_get(struct nouveau_pushbuf *push, struct nvc0_query *q,
         {
                        PUSH_SPACE(push, 5);
   PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_NVC0(push, NVC0_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->sequence);
      }
      static void
   nvc0_hw_query_rotate(struct nvc0_context *nvc0, struct nvc0_query *q)
   {
               hq->offset += hq->rotate;
   hq->data += hq->rotate / sizeof(*hq->data);
   if (hq->offset - hq->base_offset == NVC0_HW_QUERY_ALLOC_SPACE)
      }
      static inline void
   nvc0_hw_query_update(struct nouveau_client *cli, struct nvc0_query *q)
   {
               if (hq->is64bit) {
      if (nouveau_fence_signalled(hq->fence))
      } else {
      if (hq->data[0] == hq->sequence)
         }
      static void
   nvc0_hw_destroy_query(struct nvc0_context *nvc0, struct nvc0_query *q)
   {
               if (hq->funcs && hq->funcs->destroy_query) {
      hq->funcs->destroy_query(nvc0, hq);
               nvc0_hw_query_allocate(nvc0, q, 0);
   nouveau_fence_ref(NULL, &hq->fence);
      }
      static void
   nvc0_hw_query_write_compute_invocations(struct nvc0_context *nvc0,
               {
               PUSH_SPACE_EX(push, 16, 0, 8);
   PUSH_REF1(push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_1IC0(push, NVC0_3D(MACRO_COMPUTE_COUNTER_TO_QUERY), 4);
   PUSH_DATA (push, nvc0->compute_invocations);
   PUSH_DATAh(push, nvc0->compute_invocations);
   PUSH_DATAh(push, hq->bo->offset + hq->offset + offset);
      }
      static bool
   nvc0_hw_begin_query(struct nvc0_context *nvc0, struct nvc0_query *q)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_hw_query *hq = nvc0_hw_query(q);
            if (hq->funcs && hq->funcs->begin_query)
            /* For occlusion queries we have to change the storage, because a previous
   * query might set the initial render condition to false even *after* we re-
   * initialized it to true.
   */
   if (hq->rotate) {
               /* XXX: can we do this with the GPU, and sync with respect to a previous
   *  query ?
   */
   hq->data[0] = hq->sequence; /* initialize sequence */
   hq->data[1] = 1; /* initial render condition = true */
   hq->data[4] = hq->sequence + 1; /* for comparison COND_MODE */
      }
            switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      if (nvc0->screen->num_occlusion_queries_active++) {
         } else {
      PUSH_SPACE(push, 3);
   BEGIN_NVC0(push, NVC0_3D(COUNTER_RESET), 1);
   PUSH_DATA (push, NVC0_3D_COUNTER_RESET_SAMPLECNT);
   IMMED_NVC0(push, NVC0_3D(SAMPLECNT_ENABLE), 1);
   /* Given that the counter is reset, the contents at 0x10 are
   * equivalent to doing the query -- we would get hq->sequence as the
   * payload and 0 as the reported value. This is already set up above
   * as in the hq->rotate case.
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_hw_query_get(push, q, 0x10, 0x09005002 | (q->index << 5));
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_hw_query_get(push, q, 0x10, 0x05805002 | (q->index << 5));
      case PIPE_QUERY_SO_STATISTICS:
      nvc0_hw_query_get(push, q, 0x20, 0x05805002 | (q->index << 5));
   nvc0_hw_query_get(push, q, 0x30, 0x06805002 | (q->index << 5));
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      nvc0_hw_query_get(push, q, 0x10, 0x03005002 | (q->index << 5));
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      /* XXX: This get actually writes the number of overflowed streams */
   nvc0_hw_query_get(push, q, 0x10, 0x0f005002);
      case PIPE_QUERY_TIME_ELAPSED:
      nvc0_hw_query_get(push, q, 0x10, 0x00005002);
      case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_hw_query_get(push, q, 0xc0 + 0x00, 0x00801002); /* VFETCH, VERTICES */
   nvc0_hw_query_get(push, q, 0xc0 + 0x10, 0x01801002); /* VFETCH, PRIMS */
   nvc0_hw_query_get(push, q, 0xc0 + 0x20, 0x02802002); /* VP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0xc0 + 0x30, 0x03806002); /* GP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0xc0 + 0x40, 0x04806002); /* GP, PRIMS_OUT */
   nvc0_hw_query_get(push, q, 0xc0 + 0x50, 0x07804002); /* RAST, PRIMS_IN */
   nvc0_hw_query_get(push, q, 0xc0 + 0x60, 0x08804002); /* RAST, PRIMS_OUT */
   nvc0_hw_query_get(push, q, 0xc0 + 0x70, 0x0980a002); /* ROP, PIXELS */
   nvc0_hw_query_get(push, q, 0xc0 + 0x80, 0x0d808002); /* TCP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0xc0 + 0x90, 0x0e809002); /* TEP, LAUNCHES */
   nvc0_hw_query_write_compute_invocations(nvc0, hq, 0xc0 + 0xa0);
      default:
         }
   hq->state = NVC0_HW_QUERY_STATE_ACTIVE;
      }
      static void
   nvc0_hw_end_query(struct nvc0_context *nvc0, struct nvc0_query *q)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
            if (hq->funcs && hq->funcs->end_query) {
      hq->funcs->end_query(nvc0, hq);
               if (hq->state != NVC0_HW_QUERY_STATE_ACTIVE) {
      /* some queries don't require 'begin' to be called (e.g. GPU_FINISHED) */
   if (hq->rotate)
            }
            switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      nvc0_hw_query_get(push, q, 0, 0x0100f002);
   if (--nvc0->screen->num_occlusion_queries_active == 0) {
      PUSH_SPACE(push, 1);
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      nvc0_hw_query_get(push, q, 0, 0x09005002 | (q->index << 5));
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      nvc0_hw_query_get(push, q, 0, 0x05805002 | (q->index << 5));
      case PIPE_QUERY_SO_STATISTICS:
      nvc0_hw_query_get(push, q, 0x00, 0x05805002 | (q->index << 5));
   nvc0_hw_query_get(push, q, 0x10, 0x06805002 | (q->index << 5));
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      nvc0_hw_query_get(push, q, 0x00, 0x03005002 | (q->index << 5));
      case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      /* XXX: This get actually writes the number of overflowed streams */
   nvc0_hw_query_get(push, q, 0x00, 0x0f005002);
      case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
      nvc0_hw_query_get(push, q, 0, 0x00005002);
      case PIPE_QUERY_GPU_FINISHED:
      nvc0_hw_query_get(push, q, 0, 0x1000f010);
      case PIPE_QUERY_PIPELINE_STATISTICS:
      nvc0_hw_query_get(push, q, 0x00, 0x00801002); /* VFETCH, VERTICES */
   nvc0_hw_query_get(push, q, 0x10, 0x01801002); /* VFETCH, PRIMS */
   nvc0_hw_query_get(push, q, 0x20, 0x02802002); /* VP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0x30, 0x03806002); /* GP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0x40, 0x04806002); /* GP, PRIMS_OUT */
   nvc0_hw_query_get(push, q, 0x50, 0x07804002); /* RAST, PRIMS_IN */
   nvc0_hw_query_get(push, q, 0x60, 0x08804002); /* RAST, PRIMS_OUT */
   nvc0_hw_query_get(push, q, 0x70, 0x0980a002); /* ROP, PIXELS */
   nvc0_hw_query_get(push, q, 0x80, 0x0d808002); /* TCP, LAUNCHES */
   nvc0_hw_query_get(push, q, 0x90, 0x0e809002); /* TEP, LAUNCHES */
   nvc0_hw_query_write_compute_invocations(nvc0, hq, 0xa0);
      case PIPE_QUERY_TIMESTAMP_DISJOINT:
      /* This query is not issued on GPU because disjoint is forced to false */
   hq->state = NVC0_HW_QUERY_STATE_READY;
      case NVC0_HW_QUERY_TFB_BUFFER_OFFSET:
      /* indexed by TFB buffer instead of by vertex stream */
   nvc0_hw_query_get(push, q, 0x00, 0x0d005002 | (q->index << 5));
      default:
         }
   if (hq->is64bit)
      }
      static bool
   nvc0_hw_get_query_result(struct nvc0_context *nvc0, struct nvc0_query *q,
         {
      struct nvc0_hw_query *hq = nvc0_hw_query(q);
   uint64_t *res64 = (uint64_t*)result;
   uint32_t *res32 = (uint32_t*)result;
   uint8_t *res8 = (uint8_t*)result;
   uint64_t *data64 = (uint64_t *)hq->data;
            if (hq->funcs && hq->funcs->get_query_result)
            if (hq->state != NVC0_HW_QUERY_STATE_READY)
            if (hq->state != NVC0_HW_QUERY_STATE_READY) {
      if (!wait) {
      if (hq->state != NVC0_HW_QUERY_STATE_FLUSHED) {
      hq->state = NVC0_HW_QUERY_STATE_FLUSHED;
   /* flush for silly apps that spin on GL_QUERY_RESULT_AVAILABLE */
      }
      }
   if (BO_WAIT(&nvc0->screen->base, hq->bo, NOUVEAU_BO_RD, nvc0->base.client))
            }
            switch (q->type) {
   case PIPE_QUERY_GPU_FINISHED:
      res8[0] = true;
      case PIPE_QUERY_OCCLUSION_COUNTER: /* u32 sequence, u32 count, u64 time */
      res64[0] = hq->data[1] - hq->data[5];
      case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      res8[0] = hq->data[1] != hq->data[5];
      case PIPE_QUERY_PRIMITIVES_GENERATED: /* u64 count, u64 time */
   case PIPE_QUERY_PRIMITIVES_EMITTED: /* u64 count, u64 time */
      res64[0] = data64[0] - data64[2];
      case PIPE_QUERY_SO_STATISTICS:
      res64[0] = data64[0] - data64[4];
   res64[1] = data64[2] - data64[6];
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
   case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      res8[0] = data64[0] != data64[2];
      case PIPE_QUERY_TIMESTAMP:
      res64[0] = data64[1];
      case PIPE_QUERY_TIMESTAMP_DISJOINT:
      res64[0] = 1000000000;
   res8[8] = false;
      case PIPE_QUERY_TIME_ELAPSED:
      res64[0] = data64[1] - data64[3];
      case PIPE_QUERY_PIPELINE_STATISTICS:
      for (i = 0; i < 11; ++i)
            case NVC0_HW_QUERY_TFB_BUFFER_OFFSET:
      res32[0] = hq->data[1];
      default:
      assert(0); /* can't happen, we don't create queries with invalid type */
                  }
      static void
   nvc0_hw_get_query_result_resource(struct nvc0_context *nvc0,
                                       {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_hw_query *hq = nvc0_hw_query(q);
   struct nv04_resource *buf = nv04_resource(resource);
                     if (index == -1) {
      /* TODO: Use a macro to write the availability of the query */
   if (hq->state != NVC0_HW_QUERY_STATE_READY)
         uint32_t ready[2] = {hq->state == NVC0_HW_QUERY_STATE_READY};
   nvc0->base.push_cb(&nvc0->base, buf, offset,
                  util_range_add(&buf->base, &buf->valid_buffer_range, offset,
                                 /* If the fence guarding this query has not been emitted, that makes a lot
   * of the following logic more complicated.
   */
   if (hq->is64bit)
            /* We either need to compute a 32- or 64-bit difference between 2 values,
   * and then store the result as either a 32- or 64-bit value. As such let's
   * treat all inputs as 64-bit (and just push an extra 0 for the 32-bit
   * ones), and have one macro that clamps result to i32, u32, or just
   * outputs the difference (no need to worry about 64-bit clamping).
   */
   if (hq->state != NVC0_HW_QUERY_STATE_READY)
            if ((flags & PIPE_QUERY_WAIT) && hq->state != NVC0_HW_QUERY_STATE_READY)
            PUSH_SPACE_EX(push, 32, 2, 3);
   PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   PUSH_REF1 (push, buf->bo, buf->domain | NOUVEAU_BO_WR);
   BEGIN_1IC0(push, NVC0_3D(MACRO_QUERY_BUFFER_WRITE), 9);
   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE: /* XXX what if 64-bit? */
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
   case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
      PUSH_DATA(push, 0x00000001);
      default:
      if (result_type == PIPE_QUERY_TYPE_I32)
         else if (result_type == PIPE_QUERY_TYPE_U32)
         else
                     switch (q->type) {
   case PIPE_QUERY_SO_STATISTICS:
      stride = 2;
      case PIPE_QUERY_PIPELINE_STATISTICS:
      stride = 12;
      case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
      qoffset = 8;
      default:
      assert(index == 0);
   stride = 1;
               if (hq->is64bit || qoffset) {
      nouveau_pushbuf_data(push, hq->bo, hq->offset + qoffset + 16 * index,
         if (q->type == PIPE_QUERY_TIMESTAMP) {
      PUSH_DATA(push, 0);
      } else {
      nouveau_pushbuf_data(push, hq->bo, hq->offset + qoffset +
               } else {
      nouveau_pushbuf_data(push, hq->bo, hq->offset + 4,
         PUSH_DATA(push, 0);
   nouveau_pushbuf_data(push, hq->bo, hq->offset + 16 + 4,
                     if ((flags & PIPE_QUERY_WAIT) || hq->state == NVC0_HW_QUERY_STATE_READY) {
      PUSH_DATA(push, 0);
      } else if (hq->is64bit) {
      PUSH_DATA(push, hq->fence->sequence);
   nouveau_pushbuf_data(push, nvc0->screen->fence.bo, 0,
      } else {
      PUSH_DATA(push, hq->sequence);
   nouveau_pushbuf_data(push, hq->bo, hq->offset,
      }
   PUSH_DATAh(push, buf->address + offset);
            util_range_add(&buf->base, &buf->valid_buffer_range, offset,
               }
      static const struct nvc0_query_funcs hw_query_funcs = {
      .destroy_query = nvc0_hw_destroy_query,
   .begin_query = nvc0_hw_begin_query,
   .end_query = nvc0_hw_end_query,
   .get_query_result = nvc0_hw_get_query_result,
      };
      struct nvc0_query *
   nvc0_hw_create_query(struct nvc0_context *nvc0, unsigned type, unsigned index)
   {
      struct nvc0_hw_query *hq;
   struct nvc0_query *q;
            hq = nvc0_hw_sm_create_query(nvc0, type);
   if (hq) {
      hq->base.funcs = &hw_query_funcs;
               hq = nvc0_hw_metric_create_query(nvc0, type);
   if (hq) {
      hq->base.funcs = &hw_query_funcs;
               hq = CALLOC_STRUCT(nvc0_hw_query);
   if (!hq)
            q = &hq->base;
   q->funcs = &hw_query_funcs;
   q->type = type;
            switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      hq->rotate = 32;
   space = NVC0_HW_QUERY_ALLOC_SPACE;
      case PIPE_QUERY_PIPELINE_STATISTICS:
      hq->is64bit = true;
   space = 512;
      case PIPE_QUERY_SO_STATISTICS:
      hq->is64bit = true;
   space = 64;
      case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
   case PIPE_QUERY_SO_OVERFLOW_ANY_PREDICATE:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      hq->is64bit = true;
   space = 32;
      case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
      space = 32;
      case NVC0_HW_QUERY_TFB_BUFFER_OFFSET:
      space = 16;
      default:
      debug_printf("invalid query type: %u\n", type);
   FREE(q);
               if (!nvc0_hw_query_allocate(nvc0, q, space)) {
      FREE(hq);
               if (hq->rotate) {
      /* we advance before query_begin ! */
   hq->offset -= hq->rotate;
      } else
   if (!hq->is64bit)
               }
      int
   nvc0_hw_get_driver_query_info(struct nvc0_screen *screen, unsigned id,
         {
               num_hw_sm_queries = nvc0_hw_sm_get_driver_query_info(screen, 0, NULL);
   num_hw_metric_queries =
            if (!info)
            if (id < num_hw_sm_queries)
            return nvc0_hw_metric_get_driver_query_info(screen,
      }
      void
   nvc0_hw_query_pushbuf_submit(struct nouveau_pushbuf *push,
         {
               PUSH_REF1(push, hq->bo, NOUVEAU_BO_RD | NOUVEAU_BO_GART);
   nouveau_pushbuf_data(push, hq->bo, hq->offset + result_offset, 4 |
      }
      void
   nvc0_hw_query_fifo_wait(struct nvc0_context *nvc0, struct nvc0_query *q)
   {
      struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_hw_query *hq = nvc0_hw_query(q);
            /* ensure the query's fence has been emitted */
   if (hq->is64bit)
            PUSH_SPACE(push, 5);
   PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NVC0(push, SUBC_3D(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   if (hq->is64bit) {
      PUSH_DATAh(push, nvc0->screen->fence.bo->offset);
   PUSH_DATA (push, nvc0->screen->fence.bo->offset);
      } else {
      PUSH_DATAh(push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->bo->offset + offset);
      }
   PUSH_DATA (push, (1 << 12) |
      }
