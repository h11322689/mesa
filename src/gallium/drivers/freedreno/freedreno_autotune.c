   /*
   * Copyright © 2021 Google, Inc.
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
   */
      #include "freedreno_autotune.h"
   #include "freedreno_batch.h"
   #include "freedreno_util.h"
      /**
   * Tracks, for a given batch key (which maps to a FBO/framebuffer state),
   *
   * ralloc parent is fd_autotune::ht
   */
   struct fd_batch_history {
               /* Entry in fd_autotune::lru: */
                     /**
   * List of recent fd_batch_result's
   */
      #define MAX_RESULTS 5
   };
      static struct fd_batch_history *
   get_history(struct fd_autotune *at, struct fd_batch *batch)
   {
               /* draw batches should still have their key at this point. */
   assert(batch->key || batch->nondraw);
   if (!batch->key)
            struct hash_entry *entry =
            if (entry) {
      history = entry->data;
                        history->key = fd_batch_key_clone(history, batch->key);
   list_inithead(&history->node);
            /* Note: We cap # of cached GMEM states at 20.. so assuming double-
   * buffering, 40 should be a good place to cap cached autotune state
   */
   if (at->ht->entries >= 40) {
      struct fd_batch_history *last =
         _mesa_hash_table_remove_key(at->ht, last->key);
   list_del(&last->node);
               _mesa_hash_table_insert_pre_hashed(at->ht, batch->hash, history->key,
         found:
      /* Move to the head of the LRU: */
   list_delinit(&history->node);
               }
      static void
   result_destructor(void *r)
   {
               /* Just in case we manage to somehow still be on the pending_results list: */
      }
      static struct fd_batch_result *
   get_result(struct fd_autotune *at, struct fd_batch_history *history)
   {
               result->fence =
                  if (at->idx_counter >= ARRAY_SIZE(at->results->result))
            result->history = history;
                        }
      static void
   process_results(struct fd_autotune *at)
   {
               list_for_each_entry_safe (struct fd_batch_result, result,
            if (result->fence > current_fence)
                     result->samples_passed = at->results->result[result->idx].samples_end -
            list_delinit(&result->node);
            if (history->num_results < MAX_RESULTS) {
         } else {
      /* Once above a limit, start popping old results off the
   * tail of the list:
   */
   struct fd_batch_result *old_result =
         list_delinit(&old_result->node);
            }
      static bool
   fallback_use_bypass(struct fd_batch *batch)
   {
               /* Fallback logic if we have no historical data about the rendertarget: */
   if (batch->cleared || batch->gmem_reason ||
      (batch->num_draws > 5) || (pfb->samples > 1)) {
                  }
      /**
   * A magic 8-ball that tells the gmem code whether we should do bypass mode
   * for moar fps.
   */
   bool
   fd_autotune_use_bypass(struct fd_autotune *at, struct fd_batch *batch)
   {
                        /* Only enable on gen's that opt-in (and actually have sample-passed
   * collection wired up:
   */
   if (!batch->ctx->screen->gmem_reason_mask)
            if (batch->gmem_reason & ~batch->ctx->screen->gmem_reason_mask)
            for (unsigned i = 0; i < pfb->nr_cbufs; i++) {
      /* If ms-rtt is involved, force GMEM, as we don't currently
   * implement a temporary render target that we can MSAA resolve
   * from
   */
   if (pfb->cbufs[i] && pfb->cbufs[i]->nr_samples)
               struct fd_batch_history *history = get_history(at, batch);
   if (!history)
            batch->autotune_result = get_result(at, history);
                     if (use_bypass)
            if (history->num_results > 0) {
               // TODO we should account for clears somehow
   // TODO should we try to notice if there is a drastic change from
   // frame to frame?
   list_for_each_entry (struct fd_batch_result, result, &history->results,
                                 /* Low sample count could mean there was only a clear.. or there was
   * a clear plus draws that touch no or few samples
   */
   if (avg_samples < 500.0f)
            /* Cost-per-sample is an estimate for the average number of reads+
   * writes for a given passed sample.
   */
   float sample_cost = batch->cost;
            float total_draw_cost = (avg_samples * sample_cost) / batch->num_draws;
   DBG("%08x:%u\ttotal_samples=%u, avg_samples=%f, sample_cost=%f, "
      "total_draw_cost=%f\n",
               if (total_draw_cost < 3000.0f)
                  }
      void
   fd_autotune_init(struct fd_autotune *at, struct fd_device *dev)
   {
      at->ht =
                  at->results_mem = fd_bo_new(dev, sizeof(struct fd_autotune_results),
                     }
      void
   fd_autotune_fini(struct fd_autotune *at)
   {
      _mesa_hash_table_destroy(at->ht, NULL);
      }
