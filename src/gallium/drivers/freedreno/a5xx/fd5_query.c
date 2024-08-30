   /*
   * Copyright (C) 2017 Rob Clark <robclark@freedesktop.org>
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   * Authors:
   *    Rob Clark <robclark@freedesktop.org>
   */
      /* NOTE: see https://gitlab.freedesktop.org/freedreno/freedreno/-/wikis/A5xx-Queries */
      #include "freedreno_query_acc.h"
   #include "freedreno_resource.h"
      #include "fd5_context.h"
   #include "fd5_emit.h"
   #include "fd5_format.h"
   #include "fd5_query.h"
      struct PACKED fd5_query_sample {
               /* The RB_SAMPLE_COUNT_ADDR destination needs to be 16-byte aligned: */
            uint64_t start;
   uint64_t result;
      };
   DEFINE_CAST(fd_acc_query_sample, fd5_query_sample);
      /* offset of a single field of an array of fd5_query_sample: */
   #define query_sample_idx(aq, idx, field)                                       \
      fd_resource((aq)->prsc)->bo,                                                \
      (idx * sizeof(struct fd5_query_sample)) +                                \
            /* offset of a single field of fd5_query_sample: */
   #define query_sample(aq, field) query_sample_idx(aq, 0, field)
      /*
   * Occlusion Query:
   *
   * OCCLUSION_COUNTER and OCCLUSION_PREDICATE differ only in how they
   * interpret results
   */
      static void
   occlusion_resume(struct fd_acc_query *aq, struct fd_batch *batch)
   {
               OUT_PKT4(ring, REG_A5XX_RB_SAMPLE_COUNT_CONTROL, 1);
                     OUT_PKT4(ring, REG_A5XX_RB_SAMPLE_COUNT_ADDR_LO, 2);
            fd5_event_write(batch, ring, ZPASS_DONE, false);
               }
      static void
   occlusion_pause(struct fd_acc_query *aq, struct fd_batch *batch)
   {
               OUT_PKT7(ring, CP_MEM_WRITE, 4);
   OUT_RELOC(ring, query_sample(aq, stop));
   OUT_RING(ring, 0xffffffff);
                     OUT_PKT4(ring, REG_A5XX_RB_SAMPLE_COUNT_CONTROL, 1);
                     OUT_PKT4(ring, REG_A5XX_RB_SAMPLE_COUNT_ADDR_LO, 2);
            fd5_event_write(batch, ring, ZPASS_DONE, false);
            OUT_PKT7(ring, CP_WAIT_REG_MEM, 6);
   OUT_RING(ring, 0x00000014); // XXX
   OUT_RELOC(ring, query_sample(aq, stop));
   OUT_RING(ring, 0xffffffff);
   OUT_RING(ring, 0xffffffff);
            /* result += stop - start: */
   OUT_PKT7(ring, CP_MEM_TO_MEM, 9);
   OUT_RING(ring, CP_MEM_TO_MEM_0_DOUBLE | CP_MEM_TO_MEM_0_NEG_C);
   OUT_RELOC(ring, query_sample(aq, result)); /* dst */
   OUT_RELOC(ring, query_sample(aq, result)); /* srcA */
   OUT_RELOC(ring, query_sample(aq, stop));   /* srcB */
               }
      static void
   occlusion_counter_result(struct fd_acc_query *aq,
               {
      struct fd5_query_sample *sp = fd5_query_sample(s);
      }
      static void
   occlusion_predicate_result(struct fd_acc_query *aq,
               {
      struct fd5_query_sample *sp = fd5_query_sample(s);
      }
      static const struct fd_acc_sample_provider occlusion_counter = {
      .query_type = PIPE_QUERY_OCCLUSION_COUNTER,
   .size = sizeof(struct fd5_query_sample),
   .resume = occlusion_resume,
   .pause = occlusion_pause,
      };
      static const struct fd_acc_sample_provider occlusion_predicate = {
      .query_type = PIPE_QUERY_OCCLUSION_PREDICATE,
   .size = sizeof(struct fd5_query_sample),
   .resume = occlusion_resume,
   .pause = occlusion_pause,
      };
      static const struct fd_acc_sample_provider occlusion_predicate_conservative = {
      .query_type = PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE,
   .size = sizeof(struct fd5_query_sample),
   .resume = occlusion_resume,
   .pause = occlusion_pause,
      };
      /*
   * Timestamp Queries:
   */
      static void
   timestamp_resume(struct fd_acc_query *aq, struct fd_batch *batch) assert_dt
   {
               OUT_PKT7(ring, CP_EVENT_WRITE, 4);
   OUT_RING(ring,
         OUT_RELOC(ring, query_sample(aq, start));
               }
      static void
   timestamp_pause(struct fd_acc_query *aq, struct fd_batch *batch) assert_dt
   {
               OUT_PKT7(ring, CP_EVENT_WRITE, 4);
   OUT_RING(ring,
         OUT_RELOC(ring, query_sample(aq, stop));
            fd_reset_wfi(batch);
            /* result += stop - start: */
   OUT_PKT7(ring, CP_MEM_TO_MEM, 9);
   OUT_RING(ring, CP_MEM_TO_MEM_0_DOUBLE | CP_MEM_TO_MEM_0_NEG_C);
   OUT_RELOC(ring, query_sample(aq, result)); /* dst */
   OUT_RELOC(ring, query_sample(aq, result)); /* srcA */
   OUT_RELOC(ring, query_sample(aq, stop));   /* srcB */
      }
      static void
   time_elapsed_accumulate_result(struct fd_acc_query *aq,
               {
      struct fd5_query_sample *sp = fd5_query_sample(s);
      }
      static void
   timestamp_accumulate_result(struct fd_acc_query *aq,
               {
      struct fd5_query_sample *sp = fd5_query_sample(s);
      }
      static const struct fd_acc_sample_provider time_elapsed = {
      .query_type = PIPE_QUERY_TIME_ELAPSED,
   .always = true,
   .size = sizeof(struct fd5_query_sample),
   .resume = timestamp_resume,
   .pause = timestamp_pause,
      };
      /* NOTE: timestamp query isn't going to give terribly sensible results
   * on a tiler.  But it is needed by qapitrace profile heatmap.  If you
   * add in a binning pass, the results get even more non-sensical.  So
   * we just return the timestamp on the first tile and hope that is
   * kind of good enough.
   */
      static const struct fd_acc_sample_provider timestamp = {
      .query_type = PIPE_QUERY_TIMESTAMP,
   .always = true,
   .size = sizeof(struct fd5_query_sample),
   .resume = timestamp_resume,
   .pause = timestamp_pause,
      };
      /*
   * Performance Counter (batch) queries:
   *
   * Only one of these is active at a time, per design of the gallium
   * batch_query API design.  On perfcntr query tracks N query_types,
   * each of which has a 'fd_batch_query_entry' that maps it back to
   * the associated group and counter.
   */
      struct fd_batch_query_entry {
      uint8_t gid; /* group-id */
      };
      struct fd_batch_query_data {
      struct fd_screen *screen;
   unsigned num_query_entries;
      };
      static void
   perfcntr_resume(struct fd_acc_query *aq, struct fd_batch *batch) assert_dt
   {
      struct fd_batch_query_data *data = aq->query_data;
   struct fd_screen *screen = data->screen;
            unsigned counters_per_group[screen->num_perfcntr_groups];
                     /* configure performance counters for the requested queries: */
   for (unsigned i = 0; i < data->num_query_entries; i++) {
      struct fd_batch_query_entry *entry = &data->query_entries[i];
   const struct fd_perfcntr_group *g = &screen->perfcntr_groups[entry->gid];
                     OUT_PKT4(ring, g->counters[counter_idx].select_reg, 1);
                        /* and snapshot the start values */
   for (unsigned i = 0; i < data->num_query_entries; i++) {
      struct fd_batch_query_entry *entry = &data->query_entries[i];
   const struct fd_perfcntr_group *g = &screen->perfcntr_groups[entry->gid];
   unsigned counter_idx = counters_per_group[entry->gid]++;
            OUT_PKT7(ring, CP_REG_TO_MEM, 3);
   OUT_RING(ring, CP_REG_TO_MEM_0_64B |
               }
      static void
   perfcntr_pause(struct fd_acc_query *aq, struct fd_batch *batch) assert_dt
   {
      struct fd_batch_query_data *data = aq->query_data;
   struct fd_screen *screen = data->screen;
            unsigned counters_per_group[screen->num_perfcntr_groups];
                              /* snapshot the end values: */
   for (unsigned i = 0; i < data->num_query_entries; i++) {
      struct fd_batch_query_entry *entry = &data->query_entries[i];
   const struct fd_perfcntr_group *g = &screen->perfcntr_groups[entry->gid];
   unsigned counter_idx = counters_per_group[entry->gid]++;
            OUT_PKT7(ring, CP_REG_TO_MEM, 3);
   OUT_RING(ring, CP_REG_TO_MEM_0_64B |
                     /* and compute the result: */
   for (unsigned i = 0; i < data->num_query_entries; i++) {
      /* result += stop - start: */
   OUT_PKT7(ring, CP_MEM_TO_MEM, 9);
   OUT_RING(ring, CP_MEM_TO_MEM_0_DOUBLE | CP_MEM_TO_MEM_0_NEG_C);
   OUT_RELOC(ring, query_sample_idx(aq, i, result)); /* dst */
   OUT_RELOC(ring, query_sample_idx(aq, i, result)); /* srcA */
   OUT_RELOC(ring, query_sample_idx(aq, i, stop));   /* srcB */
         }
      static void
   perfcntr_accumulate_result(struct fd_acc_query *aq,
               {
      struct fd_batch_query_data *data = aq->query_data;
            for (unsigned i = 0; i < data->num_query_entries; i++) {
            }
      static const struct fd_acc_sample_provider perfcntr = {
      .query_type = FD_QUERY_FIRST_PERFCNTR,
   .always = true,
   .resume = perfcntr_resume,
   .pause = perfcntr_pause,
      };
      static struct pipe_query *
   fd5_create_batch_query(struct pipe_context *pctx, unsigned num_queries,
         {
      struct fd_context *ctx = fd_context(pctx);
   struct fd_screen *screen = ctx->screen;
   struct fd_query *q;
   struct fd_acc_query *aq;
            data = CALLOC_VARIANT_LENGTH_STRUCT(
            data->screen = screen;
            /* validate the requested query_types and ensure we don't try
   * to request more query_types of a given group than we have
   * counters:
   */
   unsigned counters_per_group[screen->num_perfcntr_groups];
            for (unsigned i = 0; i < num_queries; i++) {
               /* verify valid query_type, ie. is it actually a perfcntr? */
   if ((query_types[i] < FD_QUERY_FIRST_PERFCNTR) ||
      (idx >= screen->num_perfcntr_queries)) {
   mesa_loge("invalid batch query query_type: %u", query_types[i]);
               struct fd_batch_query_entry *entry = &data->query_entries[i];
                     /* the perfcntr_queries[] table flattens all the countables
   * for each group in series, ie:
   *
   *   (G0,C0), .., (G0,Cn), (G1,C0), .., (G1,Cm), ...
   *
   * So to find the countable index just step back through the
   * table to find the first entry with the same group-id.
   */
   while (pq > screen->perfcntr_queries) {
      pq--;
   if (pq->group_id == entry->gid)
               if (counters_per_group[entry->gid] >=
      screen->perfcntr_groups[entry->gid].num_counters) {
   mesa_loge("too many counters for group %u\n", entry->gid);
                           q = fd_acc_create_query2(ctx, 0, 0, &perfcntr);
            /* sample buffer size is based on # of queries: */
   aq->size = num_queries * sizeof(struct fd5_query_sample);
                  error:
      free(data);
      }
      void
   fd5_query_context_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->create_query = fd_acc_create_query;
                     fd_acc_query_register_provider(pctx, &occlusion_counter);
   fd_acc_query_register_provider(pctx, &occlusion_predicate);
            fd_acc_query_register_provider(pctx, &time_elapsed);
      }
