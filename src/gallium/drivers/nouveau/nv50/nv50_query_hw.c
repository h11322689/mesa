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
      #define NV50_PUSH_EXPLICIT_SPACE_CHECKING
      #include "nv50/nv50_context.h"
   #include "nv50/nv50_query_hw.h"
   #include "nv50/nv50_query_hw_metric.h"
   #include "nv50/nv50_query_hw_sm.h"
   #include "nv_object.xml.h"
      /* XXX: Nested queries, and simultaneous queries on multiple gallium contexts
   * (since we use only a single GPU channel per screen) will not work properly.
   *
   * The first is not that big of an issue because OpenGL does not allow nested
   * queries anyway.
   */
      #define NV50_HW_QUERY_ALLOC_SPACE 256
      bool
   nv50_hw_query_allocate(struct nv50_context *nv50, struct nv50_query *q,
         {
      struct nv50_screen *screen = nv50->screen;
   struct nv50_hw_query *hq = nv50_hw_query(q);
            if (hq->bo) {
      nouveau_bo_ref(NULL, &hq->bo);
   if (hq->mm) {
      if (hq->state == NV50_HW_QUERY_STATE_READY)
         else
      nouveau_fence_work(nv50->base.fence,
      }
   if (size) {
      hq->mm = nouveau_mm_allocate(screen->base.mm_GART, size,
         if (!hq->bo)
                  ret = BO_MAP(&screen->base, hq->bo, 0, nv50->base.client);
   if (ret) {
      nv50_hw_query_allocate(nv50, q, 0);
      }
      }
      }
      static void
   nv50_hw_query_get(struct nouveau_pushbuf *push, struct nv50_query *q,
         {
                        PUSH_SPACE(push, 5);
   PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_WR);
   BEGIN_NV04(push, NV50_3D(QUERY_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->sequence);
      }
      static inline void
   nv50_hw_query_update(struct nv50_query *q)
   {
               if (hq->is64bit) {
      if (nouveau_fence_signalled(hq->fence))
      } else {
      if (hq->data[0] == hq->sequence)
         }
      static void
   nv50_hw_destroy_query(struct nv50_context *nv50, struct nv50_query *q)
   {
               if (hq->funcs && hq->funcs->destroy_query) {
      hq->funcs->destroy_query(nv50, hq);
               nv50_hw_query_allocate(nv50, q, 0);
   nouveau_fence_ref(NULL, &hq->fence);
      }
      static bool
   nv50_hw_begin_query(struct nv50_context *nv50, struct nv50_query *q)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
            if (hq->funcs && hq->funcs->begin_query)
            /* For occlusion queries we have to change the storage, because a previous
   * query might set the initial render condition to false even *after* we re-
   * initialized it to true.
   */
   if (hq->rotate) {
      hq->offset += hq->rotate;
   hq->data += hq->rotate / sizeof(*hq->data);
   if (hq->offset - hq->base_offset == NV50_HW_QUERY_ALLOC_SPACE)
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
      if (nv50->screen->num_occlusion_queries_active++) {
         } else {
      PUSH_SPACE(push, 4);
   BEGIN_NV04(push, NV50_3D(COUNTER_RESET), 1);
   PUSH_DATA (push, NV50_3D_COUNTER_RESET_SAMPLECNT);
   BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      nv50_hw_query_get(push, q, 0x20, 0x06805002);
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      nv50_hw_query_get(push, q, 0x20, 0x05805002);
      case PIPE_QUERY_SO_STATISTICS:
      nv50_hw_query_get(push, q, 0x30, 0x05805002);
   nv50_hw_query_get(push, q, 0x40, 0x06805002);
      case PIPE_QUERY_PIPELINE_STATISTICS:
      nv50_hw_query_get(push, q, 0x90, 0x00801002); /* VFETCH, VERTICES */
   nv50_hw_query_get(push, q, 0xa0, 0x01801002); /* VFETCH, PRIMS */
   nv50_hw_query_get(push, q, 0xb0, 0x02802002); /* VP, LAUNCHES */
   nv50_hw_query_get(push, q, 0xc0, 0x03806002); /* GP, LAUNCHES */
   nv50_hw_query_get(push, q, 0xd0, 0x04806002); /* GP, PRIMS_OUT */
   nv50_hw_query_get(push, q, 0xe0, 0x07804002); /* RAST, PRIMS_IN */
   nv50_hw_query_get(push, q, 0xf0, 0x08804002); /* RAST, PRIMS_OUT */
   nv50_hw_query_get(push, q, 0x100, 0x0980a002); /* ROP, PIXELS */
   ((uint64_t *)hq->data)[2 * 0x11] = nv50->compute_invocations;
      case PIPE_QUERY_TIME_ELAPSED:
      nv50_hw_query_get(push, q, 0x10, 0x00005002);
      default:
      assert(0);
      }
   hq->state = NV50_HW_QUERY_STATE_ACTIVE;
      }
      static void
   nv50_hw_end_query(struct nv50_context *nv50, struct nv50_query *q)
   {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
            if (hq->funcs && hq->funcs->end_query) {
      hq->funcs->end_query(nv50, hq);
                        switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      nv50_hw_query_get(push, q, 0, 0x0100f002);
   if (--nv50->screen->num_occlusion_queries_active == 0) {
      PUSH_SPACE(push, 2);
   BEGIN_NV04(push, NV50_3D(SAMPLECNT_ENABLE), 1);
      }
      case PIPE_QUERY_PRIMITIVES_GENERATED:
      nv50_hw_query_get(push, q, 0x10, 0x06805002);
   nv50_hw_query_get(push, q, 0x00, 0x00005010);
      case PIPE_QUERY_PRIMITIVES_EMITTED:
      nv50_hw_query_get(push, q, 0x10, 0x05805002);
   nv50_hw_query_get(push, q, 0x00, 0x00005010);
      case PIPE_QUERY_SO_STATISTICS:
      nv50_hw_query_get(push, q, 0x10, 0x05805002);
   nv50_hw_query_get(push, q, 0x20, 0x06805002);
   nv50_hw_query_get(push, q, 0x00, 0x00005010);
      case PIPE_QUERY_PIPELINE_STATISTICS:
      nv50_hw_query_get(push, q, 0x00, 0x00801002); /* VFETCH, VERTICES */
   nv50_hw_query_get(push, q, 0x10, 0x01801002); /* VFETCH, PRIMS */
   nv50_hw_query_get(push, q, 0x20, 0x02802002); /* VP, LAUNCHES */
   nv50_hw_query_get(push, q, 0x30, 0x03806002); /* GP, LAUNCHES */
   nv50_hw_query_get(push, q, 0x40, 0x04806002); /* GP, PRIMS_OUT */
   nv50_hw_query_get(push, q, 0x50, 0x07804002); /* RAST, PRIMS_IN */
   nv50_hw_query_get(push, q, 0x60, 0x08804002); /* RAST, PRIMS_OUT */
   nv50_hw_query_get(push, q, 0x70, 0x0980a002); /* ROP, PIXELS */
   ((uint64_t *)hq->data)[2 * 0x8] = nv50->compute_invocations;
      case PIPE_QUERY_TIMESTAMP:
      hq->sequence++;
      case PIPE_QUERY_TIME_ELAPSED:
      nv50_hw_query_get(push, q, 0, 0x00005002);
      case PIPE_QUERY_GPU_FINISHED:
      hq->sequence++;
   nv50_hw_query_get(push, q, 0, 0x1000f010);
      case NVA0_HW_QUERY_STREAM_OUTPUT_BUFFER_OFFSET:
      hq->sequence++;
   nv50_hw_query_get(push, q, 0, 0x0d005002 | (q->index << 5));
      case PIPE_QUERY_TIMESTAMP_DISJOINT:
      /* This query is not issued on GPU because disjoint is forced to false */
   hq->state = NV50_HW_QUERY_STATE_READY;
      default:
      assert(0);
      }
   if (hq->is64bit)
      }
      static bool
   nv50_hw_get_query_result(struct nv50_context *nv50, struct nv50_query *q,
         {
      struct nv50_hw_query *hq = nv50_hw_query(q);
   uint64_t *res64 = (uint64_t *)result;
   uint32_t *res32 = (uint32_t *)result;
   uint8_t *res8 = (uint8_t *)result;
   uint64_t *data64 = (uint64_t *)hq->data;
            if (hq->funcs && hq->funcs->get_query_result)
            if (hq->state != NV50_HW_QUERY_STATE_READY)
            if (hq->state != NV50_HW_QUERY_STATE_READY) {
      if (!wait) {
      /* for broken apps that spin on GL_QUERY_RESULT_AVAILABLE */
   if (hq->state != NV50_HW_QUERY_STATE_FLUSHED) {
      hq->state = NV50_HW_QUERY_STATE_FLUSHED;
      }
      }
   if (BO_WAIT(&nv50->screen->base, hq->bo, NOUVEAU_BO_RD, nv50->base.client))
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
      res64[0] = data64[2] - data64[4];
      case PIPE_QUERY_SO_STATISTICS:
      res64[0] = data64[2] - data64[6];
   res64[1] = data64[4] - data64[8];
      case PIPE_QUERY_PIPELINE_STATISTICS:
      for (i = 0; i < 8; ++i)
         result->pipeline_statistics.cs_invocations = data64[i * 2] - data64[18 + i * 2];
      case PIPE_QUERY_TIMESTAMP:
      res64[0] = data64[1];
      case PIPE_QUERY_TIMESTAMP_DISJOINT:
      res64[0] = 1000000000;
   res8[8] = false;
      case PIPE_QUERY_TIME_ELAPSED:
      res64[0] = data64[1] - data64[3];
      case NVA0_HW_QUERY_STREAM_OUTPUT_BUFFER_OFFSET:
      res32[0] = hq->data[1];
      default:
      assert(0);
                  }
      static const struct nv50_query_funcs hw_query_funcs = {
      .destroy_query = nv50_hw_destroy_query,
   .begin_query = nv50_hw_begin_query,
   .end_query = nv50_hw_end_query,
      };
      struct nv50_query *
   nv50_hw_create_query(struct nv50_context *nv50, unsigned type, unsigned index)
   {
      struct nv50_hw_query *hq;
   struct nv50_query *q;
            hq = nv50_hw_sm_create_query(nv50, type);
   if (hq) {
      hq->base.funcs = &hw_query_funcs;
               hq = nv50_hw_metric_create_query(nv50, type);
   if (hq) {
      hq->base.funcs = &hw_query_funcs;
               hq = CALLOC_STRUCT(nv50_hw_query);
   if (!hq)
            q = &hq->base;
   q->funcs = &hw_query_funcs;
            switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
   case PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE:
      hq->rotate = 32;
      case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      space = 32 + 16; /* separate fence value written here */
      case PIPE_QUERY_SO_STATISTICS:
      space = 64 + 16; /* separate fence value written here */
      case PIPE_QUERY_PIPELINE_STATISTICS:
      hq->is64bit = true;
   space = 9 * 2 * 16; /* 9 values, start/end, 16-bytes each */
      case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIMESTAMP_DISJOINT:
   case PIPE_QUERY_GPU_FINISHED:
      space = 32;
      case NVA0_HW_QUERY_STREAM_OUTPUT_BUFFER_OFFSET:
      space = 16;
      default:
      debug_printf("invalid query type: %u\n", type);
   FREE(q);
               if (!nv50_hw_query_allocate(nv50, q, space)) {
      FREE(hq);
               if (hq->rotate) {
      /* we advance before query_begin ! */
   hq->offset -= hq->rotate;
      } else
   if (!hq->is64bit)
               }
      int
   nv50_hw_get_driver_query_info(struct nv50_screen *screen, unsigned id,
         {
               num_hw_sm_queries = nv50_hw_sm_get_driver_query_info(screen, 0, NULL);
   num_hw_metric_queries =
            if (!info)
            if (id < num_hw_sm_queries)
            return nv50_hw_metric_get_driver_query_info(screen,
      }
      void
   nv50_hw_query_pushbuf_submit(struct nv50_context *nv50, uint16_t method,
         {
      struct nouveau_pushbuf *push = nv50->base.pushbuf;
            nv50_hw_query_update(q);
   if (hq->state != NV50_HW_QUERY_STATE_READY)
                  BEGIN_NV04(push, SUBC_3D(method), 1);
      }
      void
   nv84_hw_query_fifo_wait(struct nouveau_pushbuf *push, struct nv50_query *q)
   {
      struct nv50_hw_query *hq = nv50_hw_query(q);
                     PUSH_SPACE(push, 5);
   PUSH_REF1 (push, hq->bo, NOUVEAU_BO_GART | NOUVEAU_BO_RD);
   BEGIN_NV04(push, SUBC_3D(NV84_SUBCHAN_SEMAPHORE_ADDRESS_HIGH), 4);
   PUSH_DATAh(push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->bo->offset + offset);
   PUSH_DATA (push, hq->sequence);
      }
