   /*
   * Copyright © 2019 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "iris_perf.h"
   #include "iris_context.h"
      static void *
   iris_oa_bo_alloc(void *bufmgr, const char *name, uint64_t size)
   {
         }
      static void
   iris_perf_emit_stall_at_pixel_scoreboard(struct iris_context *ice)
   {
      iris_emit_end_of_pipe_sync(&ice->batches[IRIS_BATCH_RENDER],
            }
      static void
   iris_perf_emit_mi_report_perf_count(void *c,
                     {
      struct iris_context *ice = c;
   struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
      }
      static void
   iris_perf_batchbuffer_flush(void *c, const char *file, int line)
   {
      struct iris_context *ice = c;
      }
      static void
   iris_perf_store_register_mem(void *ctx, void *bo,
               {
      struct iris_context *ice = ctx;
   struct iris_batch *batch = &ice->batches[IRIS_BATCH_RENDER];
   if (reg_size == 8) {
         } else {
      assert(reg_size == 4);
         }
      typedef void (*bo_unreference_t)(void *);
   typedef void *(*bo_map_t)(void *, void *, unsigned flags);
   typedef void (*bo_unmap_t)(void *);
   typedef void (*emit_mi_report_t)(void *, void *, uint32_t, uint32_t);
   typedef void (*emit_mi_flush_t)(void *);
   typedef void (*store_register_mem_t)(void *ctx, void *bo,
               typedef bool (*batch_references_t)(void *batch, void *bo);
   typedef void (*bo_wait_rendering_t)(void *bo);
   typedef int (*bo_busy_t)(void *bo);
      void
   iris_perf_init_vtbl(struct intel_perf_config *perf_cfg)
   {
      perf_cfg->vtbl.bo_alloc = iris_oa_bo_alloc;
   perf_cfg->vtbl.bo_unreference = (bo_unreference_t)iris_bo_unreference;
   perf_cfg->vtbl.bo_map = (bo_map_t)iris_bo_map;
   perf_cfg->vtbl.bo_unmap = (bo_unmap_t)iris_bo_unmap;
   perf_cfg->vtbl.emit_stall_at_pixel_scoreboard =
            perf_cfg->vtbl.emit_mi_report_perf_count =
         perf_cfg->vtbl.batchbuffer_flush = iris_perf_batchbuffer_flush;
   perf_cfg->vtbl.store_register_mem =
         perf_cfg->vtbl.batch_references = (batch_references_t)iris_batch_references;
   perf_cfg->vtbl.bo_wait_rendering =
            }
