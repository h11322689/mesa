   /*
   * Copyright (C) 2014 Rob Clark <robclark@freedesktop.org>
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
      #include "freedreno_batch.h"
   #include "freedreno_context.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_util.h"
      #include "fd3_format.h"
   #include "fd3_query.h"
      struct fd_rb_samp_ctrs {
         };
      /*
   * Occlusion Query:
   *
   * OCCLUSION_COUNTER and OCCLUSION_PREDICATE differ only in how they
   * interpret results
   */
      static struct fd_hw_sample *
   occlusion_get_sample(struct fd_batch *batch, struct fd_ringbuffer *ring)
   {
      struct fd_hw_sample *samp =
            /* Set RB_SAMPLE_COUNT_ADDR to samp->offset plus value of
   * HW_QUERY_BASE_REG register:
   */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A3XX_RB_SAMPLE_COUNT_ADDR) | 0x80000000);
   OUT_RING(ring, HW_QUERY_BASE_REG);
            OUT_PKT0(ring, REG_A3XX_RB_SAMPLE_COUNT_CONTROL, 1);
            OUT_PKT3(ring, CP_DRAW_INDX, 3);
   OUT_RING(ring, 0x00000000);
   OUT_RING(ring, DRAW(DI_PT_POINTLIST_PSIZE, DI_SRC_SEL_AUTO_INDEX,
                           OUT_PKT0(ring, REG_A3XX_RBBM_PERFCTR_CTL, 1);
            OUT_PKT0(ring, REG_A3XX_VBIF_PERF_CNT_EN, 1);
   OUT_RING(ring, A3XX_VBIF_PERF_CNT_EN_CNT0 | A3XX_VBIF_PERF_CNT_EN_CNT1 |
                           }
      static uint64_t
   count_samples(const struct fd_rb_samp_ctrs *start,
         {
      uint64_t n = 0;
            /* not quite sure what all of these are, possibly different
   * counters for each MRT render target:
   */
   for (i = 0; i < 16; i += 4)
               }
      static void
   occlusion_counter_accumulate_result(struct fd_context *ctx, const void *start,
               {
      uint64_t n = count_samples(start, end);
      }
      static void
   occlusion_predicate_accumulate_result(struct fd_context *ctx, const void *start,
               {
      uint64_t n = count_samples(start, end);
      }
      static const struct fd_hw_sample_provider occlusion_counter = {
      .query_type = PIPE_QUERY_OCCLUSION_COUNTER,
   .get_sample = occlusion_get_sample,
      };
      static const struct fd_hw_sample_provider occlusion_predicate = {
      .query_type = PIPE_QUERY_OCCLUSION_PREDICATE,
   .get_sample = occlusion_get_sample,
      };
      static const struct fd_hw_sample_provider occlusion_predicate_conservative = {
      .query_type = PIPE_QUERY_OCCLUSION_PREDICATE_CONSERVATIVE,
   .get_sample = occlusion_get_sample,
      };
      void
   fd3_query_context_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->create_query = fd_hw_create_query;
   ctx->query_prepare = fd_hw_query_prepare;
   ctx->query_prepare_tile = fd_hw_query_prepare_tile;
            fd_hw_query_register_provider(pctx, &occlusion_counter);
   fd_hw_query_register_provider(pctx, &occlusion_predicate);
      }
