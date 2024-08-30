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
      #include "freedreno_context.h"
   #include "freedreno_query_hw.h"
   #include "freedreno_util.h"
      #include "fd4_context.h"
   #include "fd4_draw.h"
   #include "fd4_format.h"
   #include "fd4_query.h"
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
            /* low bits of sample addr should be zero (since they are control
   * flags in RB_SAMPLE_COUNT_CONTROL):
   */
            /* Set RB_SAMPLE_COUNT_ADDR to samp->offset plus value of
   * HW_QUERY_BASE_REG register:
   */
   OUT_PKT3(ring, CP_SET_CONSTANT, 3);
   OUT_RING(ring, CP_REG(REG_A4XX_RB_SAMPLE_COUNT_CONTROL) | 0x80000000);
   OUT_RING(ring, HW_QUERY_BASE_REG);
            OUT_PKT3(ring, CP_DRAW_INDX_OFFSET, 3);
   OUT_RING(ring, DRAW4(DI_PT_POINTLIST_PSIZE, DI_SRC_SEL_AUTO_INDEX,
         OUT_RING(ring, 1); /* NumInstances */
                        }
      static uint64_t
   count_samples(const struct fd_rb_samp_ctrs *start,
         {
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
      /*
   * Time Elapsed Query:
   *
   * Note: we could in theory support timestamp queries, but they
   * won't give sensible results for tilers.
   */
      static void
   time_elapsed_enable(struct fd_context *ctx,
         {
      /* Right now, the assignment of countable to counter register is
   * just hard coded.  If we start exposing more countables than we
   * have counters, we will need to be more clever.
   */
   struct fd_batch *batch = fd_context_batch(ctx);
   fd_wfi(batch, ring);
   OUT_PKT0(ring, REG_A4XX_CP_PERFCTR_CP_SEL_0, 1);
   OUT_RING(ring, CP_ALWAYS_COUNT);
      }
      static struct fd_hw_sample *
   time_elapsed_get_sample(struct fd_batch *batch,
         {
               /* use unused part of vsc_size_mem as scratch space, to avoid
   * extra allocation:
   */
   struct fd_bo *scratch_bo = fd4_context(batch->ctx)->vsc_size_mem;
   const int sample_off = 128;
                     /* Basic issue is that we need to read counter value to a relative
   * destination (with per-tile offset) rather than absolute dest
   * addr.  But there is no pm4 packet that can do that.  This is
   * where it would be *really* nice if we could write our own fw
   * since afaict implementing the sort of packet we need would be
   * trivial.
   *
   * Instead, we:
   * (1) CP_REG_TO_MEM to do a 64b copy of counter to scratch buffer
   * (2) CP_MEM_WRITE to write per-sample offset to scratch buffer
   * (3) CP_REG_TO_MEM w/ accumulate flag to add the per-tile base
   *     address to the per-sample offset in the scratch buffer
   * (4) CP_MEM_TO_REG to copy resulting address from steps #2 and #3
   *     to CP_ME_NRT_ADDR
   * (5) CP_MEM_TO_REG's to copy saved counter value from scratch
   *     buffer to CP_ME_NRT_DATA to trigger the write out to query
   *     result buffer
   *
   * Straightforward, right?
   *
   * Maybe could swap the order of things in the scratch buffer to
   * put address first, and copy back to CP_ME_NRT_ADDR+DATA in one
   * shot, but that's really just polishing a turd..
                     /* copy sample counter _LO and _HI to scratch: */
   OUT_PKT3(ring, CP_REG_TO_MEM, 2);
   OUT_RING(ring, CP_REG_TO_MEM_0_REG(REG_A4XX_RBBM_PERFCTR_CP_0_LO) |
                        /* ok... here we really *would* like to use the CP_SET_CONSTANT
   * mode which can add a constant to value in reg2 and write to
   * reg1... *but* that only works for banked/context registers,
   * and CP_ME_NRT_DATA isn't one of those.. so we need to do some
   * CP math to the scratch buffer instead:
   *
   * (note first 8 bytes are counter value, use offset 0x8 for
   * address calculation)
            /* per-sample offset to scratch bo: */
   OUT_PKT3(ring, CP_MEM_WRITE, 2);
   OUT_RELOC(ring, scratch_bo, addr_off, 0, 0);
            /* now add to that the per-tile base: */
   OUT_PKT3(ring, CP_REG_TO_MEM, 2);
   OUT_RING(ring, CP_REG_TO_MEM_0_REG(HW_QUERY_BASE_REG) |
                        /* now copy that back to CP_ME_NRT_ADDR: */
   OUT_PKT3(ring, CP_MEM_TO_REG, 2);
   OUT_RING(ring, REG_A4XX_CP_ME_NRT_ADDR);
            /* and finally, copy sample from scratch buffer to CP_ME_NRT_DATA
   * to trigger the write to result buffer
   */
   OUT_PKT3(ring, CP_MEM_TO_REG, 2);
   OUT_RING(ring, REG_A4XX_CP_ME_NRT_DATA);
            /* and again to get the value of the _HI reg from scratch: */
   OUT_PKT3(ring, CP_MEM_TO_REG, 2);
   OUT_RING(ring, REG_A4XX_CP_ME_NRT_DATA);
                        }
      static void
   time_elapsed_accumulate_result(struct fd_context *ctx, const void *start,
         {
      uint64_t n = *(uint64_t *)end - *(uint64_t *)start;
   /* max_freq is in Hz, convert cycle count to ns: */
      }
      static void
   timestamp_accumulate_result(struct fd_context *ctx, const void *start,
         {
      /* just return the value from fist tile: */
   if (result->u64 != 0)
         uint64_t n = *(uint64_t *)start;
   /* max_freq is in Hz, convert cycle count to ns: */
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
      static const struct fd_hw_sample_provider time_elapsed = {
      .query_type = PIPE_QUERY_TIME_ELAPSED,
   .always = true,
   .enable = time_elapsed_enable,
   .get_sample = time_elapsed_get_sample,
      };
      /* NOTE: timestamp query isn't going to give terribly sensible results
   * on a tiler.  But it is needed by qapitrace profile heatmap.  If you
   * add in a binning pass, the results get even more non-sensical.  So
   * we just return the timestamp on the first tile and hope that is
   * kind of good enough.
   */
   static const struct fd_hw_sample_provider timestamp = {
      .query_type = PIPE_QUERY_TIMESTAMP,
   .always = true,
   .enable = time_elapsed_enable,
   .get_sample = time_elapsed_get_sample,
      };
      void
   fd4_query_context_init(struct pipe_context *pctx) disable_thread_safety_analysis
   {
               ctx->create_query = fd_hw_create_query;
   ctx->query_prepare = fd_hw_query_prepare;
   ctx->query_prepare_tile = fd_hw_query_prepare_tile;
            fd_hw_query_register_provider(pctx, &occlusion_counter);
   fd_hw_query_register_provider(pctx, &occlusion_predicate);
   fd_hw_query_register_provider(pctx, &occlusion_predicate_conservative);
   fd_hw_query_register_provider(pctx, &time_elapsed);
      }
