   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * based in part on radv driver which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      /**
   * This file implements VkQueue, VkFence, and VkSemaphore
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <unistd.h>
   #include <vulkan/vulkan.h>
      #include "pvr_job_compute.h"
   #include "pvr_job_context.h"
   #include "pvr_job_render.h"
   #include "pvr_job_transfer.h"
   #include "pvr_limits.h"
   #include "pvr_private.h"
   #include "util/macros.h"
   #include "util/u_atomic.h"
   #include "vk_alloc.h"
   #include "vk_fence.h"
   #include "vk_log.h"
   #include "vk_object.h"
   #include "vk_queue.h"
   #include "vk_semaphore.h"
   #include "vk_sync.h"
   #include "vk_sync_dummy.h"
   #include "vk_util.h"
      static VkResult pvr_driver_queue_submit(struct vk_queue *queue,
            static VkResult pvr_queue_init(struct pvr_device *device,
                     {
      struct pvr_transfer_ctx *transfer_ctx;
   struct pvr_compute_ctx *compute_ctx;
   struct pvr_compute_ctx *query_ctx;
   struct pvr_render_ctx *gfx_ctx;
                     result =
         if (result != VK_SUCCESS)
            if (device->ws->features.supports_threaded_submit) {
      result = vk_queue_enable_submit_thread(&queue->vk);
   if (result != VK_SUCCESS)
               result = pvr_transfer_ctx_create(device,
               if (result != VK_SUCCESS)
            result = pvr_compute_ctx_create(device,
               if (result != VK_SUCCESS)
            result = pvr_compute_ctx_create(device,
               if (result != VK_SUCCESS)
            result =
         if (result != VK_SUCCESS)
            queue->device = device;
   queue->gfx_ctx = gfx_ctx;
   queue->compute_ctx = compute_ctx;
   queue->query_ctx = query_ctx;
                           err_query_ctx_destroy:
            err_compute_ctx_destroy:
            err_transfer_ctx_destroy:
            err_vk_queue_finish:
                  }
      VkResult pvr_queues_create(struct pvr_device *device,
         {
               /* Check requested queue families and queues */
   assert(pCreateInfo->queueCreateInfoCount == 1);
   assert(pCreateInfo->pQueueCreateInfos[0].queueFamilyIndex == 0);
            const VkDeviceQueueCreateInfo *queue_create =
            device->queues = vk_alloc(&device->vk.alloc,
                     if (!device->queues)
                     for (uint32_t i = 0; i < queue_create->queueCount; i++) {
      result = pvr_queue_init(device, &device->queues[i], queue_create, i);
   if (result != VK_SUCCESS)
                              err_queues_finish:
      pvr_queues_destroy(device);
      }
      static void pvr_queue_finish(struct pvr_queue *queue)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(queue->next_job_wait_sync); i++) {
      if (queue->next_job_wait_sync[i])
               for (uint32_t i = 0; i < ARRAY_SIZE(queue->last_job_signal_sync); i++) {
      if (queue->last_job_signal_sync[i])
               pvr_render_ctx_destroy(queue->gfx_ctx);
   pvr_compute_ctx_destroy(queue->query_ctx);
   pvr_compute_ctx_destroy(queue->compute_ctx);
               }
      void pvr_queues_destroy(struct pvr_device *device)
   {
      for (uint32_t q_idx = 0; q_idx < device->queue_count; q_idx++)
               }
      static void pvr_update_job_syncs(struct pvr_device *device,
                     {
      if (queue->next_job_wait_sync[submitted_job_type]) {
      vk_sync_destroy(&device->vk,
                     if (queue->last_job_signal_sync[submitted_job_type]) {
      vk_sync_destroy(&device->vk,
                  }
      static VkResult pvr_process_graphics_cmd(struct pvr_device *device,
                     {
      pvr_dev_addr_t original_ctrl_stream_addr = { 0 };
   struct vk_sync *geom_signal_sync;
   struct vk_sync *frag_signal_sync = NULL;
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            if (sub_cmd->job.run_frag) {
      result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
                        /* Perform two render submits when using multiple framebuffer layers. The
   * first submit contains just geometry, while the second only terminates
   * (and triggers the fragment render if originally specified). This is needed
   * because the render target cache gets cleared on terminating submits, which
   * could result in missing primitives.
   */
   if (pvr_sub_cmd_gfx_requires_split_submit(sub_cmd)) {
      /* If fragment work shouldn't be run there's no need for a split,
   * and if geometry_terminate is false this kick can't have a fragment
   * stage without another terminating geometry kick.
   */
            /* First submit must not touch fragment work. */
   sub_cmd->job.geometry_terminate = false;
            result =
      pvr_render_job_submit(queue->gfx_ctx,
                                 sub_cmd->job.geometry_terminate = true;
            if (result != VK_SUCCESS)
                     /* Second submit contains only a trivial control stream to terminate the
   * geometry work.
   */
   assert(sub_cmd->terminate_ctrl_stream);
   sub_cmd->job.ctrl_stream_addr =
               result = pvr_render_job_submit(queue->gfx_ctx,
                                    if (original_ctrl_stream_addr.addr > 0)
            if (result != VK_SUCCESS)
                     if (sub_cmd->job.run_frag)
                           err_destroy_frag_sync:
      if (frag_signal_sync)
      err_destroy_geom_sync:
                  }
      static VkResult pvr_process_compute_cmd(struct pvr_device *device,
               {
      struct vk_sync *sync;
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            result =
      pvr_compute_job_submit(queue->compute_ctx,
                  if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, sync);
                           }
      static VkResult pvr_process_transfer_cmds(struct pvr_device *device,
               {
      struct vk_sync *sync;
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            result =
      pvr_transfer_job_submit(queue->transfer_ctx,
                  if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, sync);
                           }
      static VkResult
   pvr_process_occlusion_query_cmd(struct pvr_device *device,
               {
      struct vk_sync *sync;
            /* TODO: Currently we add barrier event sub commands to handle the sync
   * necessary for the different occlusion query types. Would we get any speed
   * up in processing the queue by doing that sync here without using event sub
   * commands?
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            result = pvr_compute_job_submit(
      queue->query_ctx,
   sub_cmd,
   queue->next_job_wait_sync[PVR_JOB_TYPE_OCCLUSION_QUERY],
      if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, sync);
                           }
      static VkResult
   pvr_process_event_cmd_barrier(struct pvr_device *device,
               {
      const uint32_t src_mask = sub_cmd->wait_for_stage_mask;
   const uint32_t dst_mask = sub_cmd->wait_at_stage_mask;
   struct vk_sync_wait wait_syncs[PVR_JOB_TYPE_MAX + 1];
   uint32_t src_wait_count = 0;
            assert(!(src_mask & ~(PVR_PIPELINE_STAGE_ALL_BITS |
         assert(!(dst_mask & ~(PVR_PIPELINE_STAGE_ALL_BITS |
            u_foreach_bit (stage, src_mask) {
      if (queue->last_job_signal_sync[stage]) {
      wait_syncs[src_wait_count++] = (struct vk_sync_wait){
      .sync = queue->last_job_signal_sync[stage],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                     /* No previous src jobs that need finishing so no need for a barrier. */
   if (src_wait_count == 0)
            u_foreach_bit (stage, dst_mask) {
      uint32_t wait_count = src_wait_count;
   struct vk_sync_signal signal;
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            signal = (struct vk_sync_signal){
      .sync = signal_sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
               if (queue->next_job_wait_sync[stage]) {
      wait_syncs[wait_count++] = (struct vk_sync_wait){
      .sync = queue->next_job_wait_sync[stage],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                  result = device->ws->ops->null_job_submit(device->ws,
                     if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, signal_sync);
               if (queue->next_job_wait_sync[stage])
                           }
      static VkResult
   pvr_process_event_cmd_set_or_reset(struct pvr_device *device,
                     {
      /* Not PVR_JOB_TYPE_MAX since that also includes
   * PVR_JOB_TYPE_OCCLUSION_QUERY so no stage in the src mask.
   */
   struct vk_sync_wait waits[PVR_NUM_SYNC_PIPELINE_STAGES];
   struct vk_sync_signal signal;
            uint32_t wait_count = 0;
                     u_foreach_bit (stage, sub_cmd->wait_for_stage_mask) {
      if (!queue->last_job_signal_sync[stage])
            waits[wait_count++] = (struct vk_sync_wait){
      .sync = queue->last_job_signal_sync[stage],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                  result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            signal = (struct vk_sync_signal){
      .sync = signal_sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
               result =
         if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, signal_sync);
               if (sub_cmd->event->sync)
            sub_cmd->event->sync = signal_sync;
               }
      static inline VkResult
   pvr_process_event_cmd_set(struct pvr_device *device,
               {
      return pvr_process_event_cmd_set_or_reset(device,
                  }
      static inline VkResult
   pvr_process_event_cmd_reset(struct pvr_device *device,
               {
      return pvr_process_event_cmd_set_or_reset(device,
                  }
      /**
   * \brief Process an event sub command of wait type.
   *
   * This sets up barrier syncobjs to create a dependency from the event syncobjs
   * onto the next job submissions.
   *
   * The barriers are setup by taking into consideration each event's dst stage
   * mask so this is in line with vkCmdWaitEvents2().
   *
   * \param[in] device                       Device to create the syncobjs on.
   * \param[in] sub_cmd                      Sub command to process.
   * \param[in,out] barriers                 Current barriers as input. Barriers
   *                                         for the next jobs as output.
   * \parma[in,out] per_cmd_buffer_syncobjs  Completion syncobjs for the command
   *                                         buffer being processed.
   */
   static VkResult
   pvr_process_event_cmd_wait(struct pvr_device *device,
               {
      uint32_t dst_mask = 0;
            STACK_ARRAY(struct vk_sync_wait, waits, sub_cmd->count + 1);
   if (!waits)
            for (uint32_t i = 0; i < sub_cmd->count; i++)
            u_foreach_bit (stage, dst_mask) {
      struct vk_sync_signal signal;
   struct vk_sync *signal_sync;
            for (uint32_t i = 0; i < sub_cmd->count; i++) {
      if (sub_cmd->wait_at_stage_masks[i] & stage) {
      waits[wait_count++] = (struct vk_sync_wait){
      .sync = sub_cmd->events[i]->sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
                     if (!wait_count)
            if (queue->next_job_wait_sync[stage]) {
      waits[wait_count++] = (struct vk_sync_wait){
      .sync = queue->next_job_wait_sync[stage],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                           result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            signal = (struct vk_sync_signal){
      .sync = signal_sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
               result = device->ws->ops->null_job_submit(device->ws,
                     if (result != VK_SUCCESS) {
      vk_sync_destroy(&device->vk, signal.sync);
               if (queue->next_job_wait_sync[stage])
                                       err_free_waits:
                  }
      static VkResult pvr_process_event_cmd(struct pvr_device *device,
               {
      switch (sub_cmd->type) {
   case PVR_EVENT_TYPE_SET:
         case PVR_EVENT_TYPE_RESET:
         case PVR_EVENT_TYPE_WAIT:
         case PVR_EVENT_TYPE_BARRIER:
         default:
            }
      static VkResult pvr_process_cmd_buffer(struct pvr_device *device,
               {
               list_for_each_entry_safe (struct pvr_sub_cmd,
                        switch (sub_cmd->type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS: {
      /* If the fragment job utilizes occlusion queries, for data integrity
   * it needs to wait for the occlusion query to be processed.
   */
   if (sub_cmd->gfx.has_occlusion_query) {
      struct pvr_sub_cmd_event_barrier barrier = {
                        result = pvr_process_event_cmd_barrier(device, queue, &barrier);
   if (result != VK_SUCCESS)
               if (sub_cmd->gfx.wait_on_previous_transfer) {
      struct pvr_sub_cmd_event_barrier barrier = {
                        result = pvr_process_event_cmd_barrier(device, queue, &barrier);
   if (result != VK_SUCCESS)
               result =
                     case PVR_SUB_CMD_TYPE_COMPUTE:
                  case PVR_SUB_CMD_TYPE_TRANSFER: {
               if (serialize_with_frag) {
      struct pvr_sub_cmd_event_barrier barrier = {
                        result = pvr_process_event_cmd_barrier(device, queue, &barrier);
   if (result != VK_SUCCESS)
                        if (serialize_with_frag) {
      struct pvr_sub_cmd_event_barrier barrier = {
                                                               case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
      result =
               case PVR_SUB_CMD_TYPE_EVENT:
                  default:
      mesa_loge("Unsupported sub-command type %d", sub_cmd->type);
               if (result != VK_SUCCESS)
                           }
      static VkResult pvr_clear_last_submits_syncs(struct pvr_queue *queue)
   {
      struct vk_sync_wait waits[PVR_JOB_TYPE_MAX * 2];
   uint32_t wait_count = 0;
            for (uint32_t i = 0; i < PVR_JOB_TYPE_MAX; i++) {
      if (queue->next_job_wait_sync[i]) {
      waits[wait_count++] = (struct vk_sync_wait){
      .sync = queue->next_job_wait_sync[i],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                  if (queue->last_job_signal_sync[i]) {
      waits[wait_count++] = (struct vk_sync_wait){
      .sync = queue->last_job_signal_sync[i],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                     result = vk_sync_wait_many(&queue->device->vk,
                              if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < PVR_JOB_TYPE_MAX; i++) {
      if (queue->next_job_wait_sync[i]) {
      vk_sync_destroy(&queue->device->vk, queue->next_job_wait_sync[i]);
               if (queue->last_job_signal_sync[i]) {
      vk_sync_destroy(&queue->device->vk, queue->last_job_signal_sync[i]);
                     }
      static VkResult pvr_process_queue_signals(struct pvr_queue *queue,
               {
      struct vk_sync_wait signal_waits[PVR_JOB_TYPE_MAX];
   struct pvr_device *device = queue->device;
            for (uint32_t signal_idx = 0; signal_idx < signal_count; signal_idx++) {
      struct vk_sync_signal *signal = &signals[signal_idx];
   const enum pvr_pipeline_stage_bits signal_stage_src =
                  for (uint32_t i = 0; i < PVR_JOB_TYPE_MAX; i++) {
      /* Exception for occlusion query jobs since that's something internal,
   * so the user provided syncs won't ever have it as a source stage.
   */
   if (!(signal_stage_src & BITFIELD_BIT(i)) &&
                                 signal_waits[wait_count++] = (struct vk_sync_wait){
      .sync = queue->last_job_signal_sync[i],
   .stage_mask = ~(VkPipelineStageFlags2)0,
                  result = device->ws->ops->null_job_submit(device->ws,
                     if (result != VK_SUCCESS)
                  }
      static VkResult pvr_process_queue_waits(struct pvr_queue *queue,
               {
      struct pvr_device *device = queue->device;
            STACK_ARRAY(struct vk_sync_wait, stage_waits, wait_count);
   if (!stage_waits)
            for (uint32_t i = 0; i < PVR_JOB_TYPE_MAX; i++) {
      struct vk_sync_signal next_job_wait_signal_sync;
            for (uint32_t wait_idx = 0; wait_idx < wait_count; wait_idx++) {
                     stage_waits[stage_wait_count++] = (struct vk_sync_wait){
      .sync = waits[wait_idx].sync,
   .stage_mask = ~(VkPipelineStageFlags2)0,
                  result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            next_job_wait_signal_sync = (struct vk_sync_signal){
      .sync = queue->next_job_wait_sync[i],
   .stage_mask = ~(VkPipelineStageFlags2)0,
               result = device->ws->ops->null_job_submit(device->ws,
                     if (result != VK_SUCCESS)
                              err_free_waits:
                  }
      static VkResult pvr_driver_queue_submit(struct vk_queue *queue,
         {
      struct pvr_queue *driver_queue = container_of(queue, struct pvr_queue, vk);
   struct pvr_device *device = driver_queue->device;
            result = pvr_clear_last_submits_syncs(driver_queue);
   if (result != VK_SUCCESS)
            result =
         if (result != VK_SUCCESS)
            for (uint32_t i = 0U; i < submit->command_buffer_count; i++) {
      result = pvr_process_cmd_buffer(
      device,
   driver_queue,
      if (result != VK_SUCCESS)
               result = pvr_process_queue_signals(driver_queue,
               if (result != VK_SUCCESS)
               }
