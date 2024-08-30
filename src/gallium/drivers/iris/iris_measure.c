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
      /**
   * @file iris_measure.c
   */
      #include <stdio.h>
   #include "util/u_debug.h"
   #include "util/list.h"
   #include "util/crc32.h"
   #include "iris_context.h"
   #include "iris_defines.h"
   #include "compiler/shader_info.h"
      /**
   * This callback is registered with intel_measure.  It will be called when
   * snapshot data has been fully collected, so iris can release the associated
   * resources.
   */
   static void
   measure_batch_free(struct intel_measure_batch *base)
   {
      struct iris_measure_batch *batch =
            }
      void
   iris_init_screen_measure(struct iris_screen *screen)
   {
               memset(measure_device, 0, sizeof(*measure_device));
   intel_measure_init(measure_device);
   measure_device->release_batch = &measure_batch_free;
   struct intel_measure_config *config = measure_device->config;
   if (config == NULL)
            /* the final member of intel_measure_ringbuffer is a zero-length array of
   * intel_measure_buffered_result objects.  Allocate additional space for
   * the buffered objects based on the run-time configurable buffer_size
   */
   const size_t rb_bytes = sizeof(struct intel_measure_ringbuffer) +
         struct intel_measure_ringbuffer *rb = rzalloc_size(screen, rb_bytes);
      }
      static struct intel_measure_config *
   config_from_screen(struct iris_screen *screen)
   {
         }
      static struct intel_measure_config *
   config_from_context(struct iris_context *ice)
   {
         }
      void
   iris_destroy_screen_measure(struct iris_screen *screen)
   {
      if (!config_from_screen(screen))
                     if (measure_device->config->file &&
      measure_device->config->file != stderr)
         ralloc_free(measure_device->ringbuffer);
      }
         void
   iris_init_batch_measure(struct iris_context *ice, struct iris_batch *batch)
   {
      const struct intel_measure_config *config = config_from_context(ice);
   struct iris_screen *screen = batch->screen;
            if (!config)
            /* the final member of iris_measure_batch is a zero-length array of
   * intel_measure_snapshot objects.  Create additional space for the
   * snapshot objects based on the run-time configurable batch_size
   */
   const size_t batch_bytes = sizeof(struct iris_measure_batch) +
         assert(batch->measure == NULL);
   batch->measure = malloc(batch_bytes);
   memset(batch->measure, 0, batch_bytes);
            measure->bo = iris_bo_alloc(bufmgr, "measure",
               measure->base.timestamps = iris_bo_map(NULL, measure->bo, MAP_READ);
   measure->base.renderpass =
      (uintptr_t)util_hash_crc32(&ice->state.framebuffer,
   }
      void
   iris_destroy_batch_measure(struct iris_measure_batch *batch)
   {
      if (!batch)
         iris_bo_unmap(batch->bo);
   iris_bo_unreference(batch->bo);
   batch->bo = NULL;
      }
      static uint32_t
   fetch_hash(const struct iris_uncompiled_shader *uncompiled)
   {
         }
      static void
   measure_start_snapshot(struct iris_context *ice,
                           {
      struct intel_measure_batch *measure_batch = &batch->measure->base;
   const struct intel_measure_config *config = config_from_context(ice);
   const struct iris_screen *screen = (void *) ice->ctx.screen;
            /* if the command buffer is not associated with a frame, associate it with
   * the most recent acquired frame
   */
   if (measure_batch->frame == 0)
                     if (measure_batch->index == config->batch_size) {
      /* Snapshot buffer is full.  The batch must be flushed before additional
   * snapshots can be taken.
   */
   static bool warned = false;
   if (unlikely(!warned)) {
      fprintf(config->file,
         "WARNING: batch size exceeds INTEL_MEASURE limit: %d. "
   "Data has been dropped. "
   "Increase setting with INTEL_MEASURE=batch_size={count}\n",
      }
               unsigned index = measure_batch->index++;
   assert(index < config->batch_size);
   if (event_name == NULL)
            if(config->cpu_measure) {
      intel_measure_print_cpu_result(measure_batch->frame,
                                 measure_batch->batch_count,
               iris_emit_pipe_control_write(batch, "measurement snapshot",
                        struct intel_measure_snapshot *snapshot = &(measure_batch->snapshots[index]);
   memset(snapshot, 0, sizeof(*snapshot));
   snapshot->type = type;
   snapshot->count = (unsigned) count;
   snapshot->event_count = measure_batch->event_count;
   snapshot->event_name = event_name;
            if (type == INTEL_SNAPSHOT_COMPUTE) {
         } else if (type == INTEL_SNAPSHOT_DRAW) {
      snapshot->vs  = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_VERTEX]);
   snapshot->tcs = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_TESS_CTRL]);
   snapshot->tes = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL]);
   snapshot->gs  = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_GEOMETRY]);
         }
      static void
   measure_end_snapshot(struct iris_batch *batch,
         {
      struct intel_measure_batch *measure_batch = &batch->measure->base;
            unsigned index = measure_batch->index++;
   assert(index % 2 == 1);
   if(config->cpu_measure)
            iris_emit_pipe_control_write(batch, "measurement snapshot",
                              struct intel_measure_snapshot *snapshot = &(measure_batch->snapshots[index]);
   memset(snapshot, 0, sizeof(*snapshot));
   snapshot->type = INTEL_SNAPSHOT_END;
      }
      static bool
   state_changed(const struct iris_context *ice,
               {
               if (type == INTEL_SNAPSHOT_COMPUTE) {
         } else if (type == INTEL_SNAPSHOT_DRAW) {
      vs  = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_VERTEX]);
   tcs = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_TESS_CTRL]);
   tes = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_TESS_EVAL]);
   gs  = fetch_hash(ice->shaders.uncompiled[MESA_SHADER_GEOMETRY]);
      }
            return intel_measure_state_changed(&batch->measure->base,
      }
      static void
   iris_measure_renderpass(struct iris_context *ice)
   {
      const struct intel_measure_config *config = config_from_context(ice);
   struct intel_measure_batch *batch =
            if (!config)
         uint32_t framebuffer_crc = util_hash_crc32(&ice->state.framebuffer,
         if (framebuffer_crc == batch->renderpass)
         bool filtering = config->flags & INTEL_MEASURE_RENDERPASS;
   if (filtering && batch->index % 2 == 1) {
      /* snapshot for previous renderpass was not ended */
   measure_end_snapshot(&ice->batches[IRIS_BATCH_RENDER],
                        }
      void
   _iris_measure_snapshot(struct iris_context *ice,
                        struct iris_batch *batch,
      {
         const struct intel_measure_config *config = config_from_context(ice);
            assert(config);
   if (!config->enabled)
         if (measure_batch == NULL)
            assert(type != INTEL_SNAPSHOT_END);
            static unsigned batch_count = 0;
   if (measure_batch->event_count == 0)
            if (!state_changed(ice, batch, type)) {
      /* filter out this event */
               /* increment event count */
   ++measure_batch->event_count;
   if (measure_batch->event_count == 1 ||
      measure_batch->event_count == config->event_interval + 1) {
   /* the first event of an interval */
   if (measure_batch->index % 2) {
      /* end the previous event */
      }
            const char *event_name = NULL;
   int count = 0;
   if (sc)
            if (draw != NULL) {
      const struct shader_info *fs_info =
         if (fs_info && fs_info->name && strncmp(fs_info->name, "st/", 2) == 0) {
         } else if (indirect) {
      event_name = "DrawIndirect";
   if (indirect->count_from_stream_output) {
            }
   else if (draw->index_size)
         else
                     measure_start_snapshot(ice, batch, type, event_name, count);
         }
      void
   iris_destroy_ctx_measure(struct iris_context *ice)
   {
      /* All outstanding snapshots must be collected before the context is
   * destroyed.
   */
   struct iris_screen *screen = (struct iris_screen *) ice->ctx.screen;
      }
      void
   iris_measure_batch_end(struct iris_context *ice, struct iris_batch *batch)
   {
      const struct intel_measure_config *config = config_from_context(ice);
   struct iris_screen *screen = (struct iris_screen *) ice->ctx.screen;
   struct iris_measure_batch *iris_measure_batch = batch->measure;
   struct intel_measure_batch *measure_batch = &iris_measure_batch->base;
            if (!config)
         if (!config->enabled)
            assert(measure_batch);
            if (measure_batch->index % 2) {
      /* We hit the end of the batch, but never terminated our section of
   * drawing with the same render target or shaders.  End it now.
   */
               if (measure_batch->index == 0)
            /* At this point, total_chained_batch_size is not yet updated because the
   * batch_end measurement is within the batch and the batch is not quite
   * ended yet (it'll be just after this function call). So combined the
   * already summed total_chained_batch_size with whatever was written in the
   * current batch BO.
   */
   measure_batch->batch_size = batch->total_chained_batch_size +
            /* enqueue snapshot for gathering */
   pthread_mutex_lock(&measure_device->mutex);
   list_addtail(&iris_measure_batch->base.link, &measure_device->queued_snapshots);
   batch->measure = NULL;
   pthread_mutex_unlock(&measure_device->mutex);
   /* init new measure_batch */
            static int interval = 0;
   if (++interval > 10) {
      intel_measure_gather(measure_device, screen->devinfo);
         }
      void
   iris_measure_frame_end(struct iris_context *ice)
   {
      struct iris_screen *screen = (struct iris_screen *) ice->ctx.screen;
   struct intel_measure_device *measure_device = &screen->measure;
            if (!config)
            /* increment frame counter */
               }
