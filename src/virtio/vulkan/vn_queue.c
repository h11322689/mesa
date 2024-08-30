   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   *
   * based in part on anv and radv which are:
   * Copyright © 2015 Intel Corporation
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   */
      #include "vn_queue.h"
      #include "util/libsync.h"
   #include "venus-protocol/vn_protocol_driver_event.h"
   #include "venus-protocol/vn_protocol_driver_fence.h"
   #include "venus-protocol/vn_protocol_driver_queue.h"
   #include "venus-protocol/vn_protocol_driver_semaphore.h"
   #include "venus-protocol/vn_protocol_driver_transport.h"
      #include "vn_command_buffer.h"
   #include "vn_device.h"
   #include "vn_device_memory.h"
   #include "vn_physical_device.h"
   #include "vn_query_pool.h"
   #include "vn_renderer.h"
   #include "vn_wsi.h"
      /* queue commands */
      static bool
   vn_semaphore_wait_external(struct vn_device *dev, struct vn_semaphore *sem);
      struct vn_queue_submission {
      VkStructureType batch_type;
   VkQueue queue_handle;
   uint32_t batch_count;
   union {
      const void *batches;
   const VkSubmitInfo *submit_batches;
   const VkSubmitInfo2 *submit_batches2;
      };
            bool has_feedback_fence;
   bool has_feedback_query;
   bool has_feedback_semaphore;
   const struct vn_device_memory *wsi_mem;
   uint32_t feedback_cmd_buffer_count;
   struct vn_sync_payload_external external_payload;
            /* Temporary storage allocation for submission
   * A single alloc for storage is performed and the offsets inside
   * storage are set as below:
   * batches
   *  - copy of SubmitInfos
   *  - an extra SubmitInfo for appending fence feedback
   * cmds
   *  - copy of cmd buffers for any batch with sem feedback with
   *    additional cmd buffers for each signal semaphore that uses
   *    feedback
   *  - an extra cmd buffer info for recording and appending defered
   *    query feedback
   *  - an extra cmd buffer info for appending fence feedback
   *    when using SubmitInfo2
   */
   struct {
               union {
      void *batches;
   VkSubmitInfo *submit_batches;
                     };
      static inline uint32_t
   vn_get_wait_semaphore_count(struct vn_queue_submission *submit,
         {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
         case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
         case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
         default:
            }
      static inline uint32_t
   vn_get_signal_semaphore_count(struct vn_queue_submission *submit,
         {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
         case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
         case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
         default:
            }
      static inline VkSemaphore
   vn_get_wait_semaphore(struct vn_queue_submission *submit,
               {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
      return submit->submit_batches[batch_index]
      case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
      return submit->submit_batches2[batch_index]
      .pWaitSemaphoreInfos[semaphore_index]
   case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
      return submit->sparse_batches[batch_index]
      default:
            }
      static inline VkSemaphore
   vn_get_signal_semaphore(struct vn_queue_submission *submit,
               {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
      return submit->submit_batches[batch_index]
      case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
      return submit->submit_batches2[batch_index]
      .pSignalSemaphoreInfos[semaphore_index]
   case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
      return submit->sparse_batches[batch_index]
      default:
            }
      static inline uint32_t
   vn_get_cmd_buffer_count(struct vn_queue_submission *submit,
         {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
         case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
         case VK_STRUCTURE_TYPE_BIND_SPARSE_INFO:
         default:
            }
      static inline const void *
   vn_get_cmd_buffer_ptr(struct vn_queue_submission *submit,
         {
      assert((submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO) ||
            return submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO
            ? (const void *)submit->submit_batches[batch_index]
         }
      static inline const VkCommandBuffer
   vn_get_cmd_handle(struct vn_queue_submission *submit,
               {
      assert((submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO) ||
            return submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO
            ? submit->submit_batches[batch_index].pCommandBuffers[cmd_index]
   : submit->submit_batches2[batch_index]
   }
      static uint64_t
   vn_get_signal_semaphore_counter(struct vn_queue_submission *submit,
               {
      switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO: {
      const struct VkTimelineSemaphoreSubmitInfo *timeline_semaphore_info =
      vk_find_struct_const(submit->submit_batches[batch_index].pNext,
         }
   case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
      return submit->submit_batches2[batch_index]
      .pSignalSemaphoreInfos[semaphore_index]
   default:
            }
      static VkResult
   vn_queue_submission_fix_batch_semaphores(struct vn_queue_submission *submit,
         {
      struct vk_queue *queue_vk = vk_queue_from_handle(submit->queue_handle);
   VkDevice dev_handle = vk_device_to_handle(queue_vk->base.device);
                     for (uint32_t i = 0; i < wait_count; i++) {
      VkSemaphore sem_handle = vn_get_wait_semaphore(submit, batch_index, i);
   struct vn_semaphore *sem = vn_semaphore_from_handle(sem_handle);
            if (payload->type != VN_SYNC_TYPE_IMPORTED_SYNC_FD)
            if (!vn_semaphore_wait_external(dev, sem))
                     const VkImportSemaphoreResourceInfoMESA res_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_RESOURCE_INFO_MESA,
   .semaphore = sem_handle,
      };
   vn_async_vkImportSemaphoreResourceMESA(dev->instance, dev_handle,
                  }
      static void
   vn_queue_submission_count_batch_feedback(struct vn_queue_submission *submit,
         {
               bool batch_has_feedback_sem = false;
   for (uint32_t i = 0; i < signal_count; i++) {
      struct vn_semaphore *sem = vn_semaphore_from_handle(
         if (sem->feedback.slot) {
      batch_has_feedback_sem = true;
                           if (submit->batch_type != VK_STRUCTURE_TYPE_BIND_SPARSE_INFO) {
      uint32_t cmd_count = vn_get_cmd_buffer_count(submit, batch_index);
   for (uint32_t i = 0; i < cmd_count; i++) {
      struct vn_command_buffer *cmd = vn_command_buffer_from_handle(
         if (!list_is_empty(&cmd->builder.query_batches))
                  if (batch_has_feedback_query)
            if (batch_has_feedback_sem || batch_has_feedback_query) {
      submit->feedback_cmd_buffer_count +=
               submit->has_feedback_query |= batch_has_feedback_query;
      }
      static VkResult
   vn_queue_submission_prepare(struct vn_queue_submission *submit)
   {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   struct vn_fence *fence = vn_fence_from_handle(submit->fence_handle);
            submit->external_payload.ring_idx = queue->ring_idx;
   submit->has_feedback_fence = fence && fence->feedback.slot;
            submit->wsi_mem = NULL;
   if (submit->batch_count == 1 &&
      submit->batch_type != VK_STRUCTURE_TYPE_BIND_SPARSE_INFO) {
   const struct wsi_memory_signal_submit_info *info = vk_find_struct_const(
         if (info) {
      submit->wsi_mem = vn_device_memory_from_handle(info->memory);
                  for (uint32_t i = 0; i < submit->batch_count; i++) {
      VkResult result = vn_queue_submission_fix_batch_semaphores(submit, i);
   if (result != VK_SUCCESS)
                           }
      static VkResult
   vn_queue_submission_alloc_storage(struct vn_queue_submission *submit)
   {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   const VkAllocationCallbacks *alloc = &queue->base.base.base.device->alloc;
   size_t batch_size = 0;
   size_t cmd_size = 0;
   size_t alloc_size = 0;
            if (!submit->has_feedback_fence && !submit->has_feedback_semaphore &&
      !submit->has_feedback_query)
         switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
      batch_size = sizeof(VkSubmitInfo);
   cmd_size = sizeof(VkCommandBuffer);
      case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
      batch_size = sizeof(VkSubmitInfo2);
   cmd_size = sizeof(VkCommandBufferSubmitInfo);
      default:
                  /* space for copied batches */
   alloc_size = batch_size * submit->batch_count;
            if (submit->has_feedback_fence) {
      /* add space for an additional batch for fence feedback
   * and move cmd offset
   */
   alloc_size += batch_size;
            /* SubmitInfo2 needs a cmd buffer info struct for the fence
   * feedback cmd
   */
   if (submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO_2)
               /* space for copied cmds and sem/query feedback cmds */
   if (submit->has_feedback_semaphore || submit->has_feedback_query)
            submit->temp.storage = vk_alloc(alloc, alloc_size, VN_DEFAULT_ALIGN,
            if (!submit->temp.storage)
            submit->temp.batches = submit->temp.storage;
               }
      struct vn_feedback_src {
      struct vn_feedback_slot *src_slot;
               };
      static VkResult
   vn_timeline_semaphore_feedback_src_init(struct vn_device *dev,
                     {
      VkResult result;
            feedback_src->src_slot = vn_feedback_pool_alloc(
            if (!feedback_src->src_slot)
            feedback_src->commands = vk_zalloc(
      alloc, sizeof(feedback_src->commands) * dev->queue_family_count,
         if (!feedback_src->commands) {
      vn_feedback_pool_free(&dev->feedback_pool, feedback_src->src_slot);
               for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      result = vn_feedback_cmd_alloc(dev_handle, &dev->cmd_pools[i], slot,
               if (result != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; j++) {
      vn_feedback_cmd_free(dev_handle, &dev->cmd_pools[j],
      }
   vk_free(alloc, feedback_src->commands);
   vn_feedback_pool_free(&dev->feedback_pool, feedback_src->src_slot);
                     }
      static VkResult
   vn_set_sem_feedback_cmd(struct vn_queue *queue,
                     {
      VkResult result;
   struct vk_queue *queue_vk = &queue->base.base;
   struct vn_device *dev = (void *)queue_vk->base.device;
   const VkAllocationCallbacks *alloc = &dev->base.base.alloc;
                     simple_mtx_lock(&sem->feedback.src_lists_mtx);
   if (!list_is_empty(&sem->feedback.free_src_list)) {
      free_feedback_src = list_first_entry(&sem->feedback.free_src_list,
            }
            if (!free_feedback_src) {
      /* allocate a new src slot if none are free */
   free_feedback_src =
                  if (!free_feedback_src)
            result = vn_timeline_semaphore_feedback_src_init(
         if (result != VK_SUCCESS) {
      vk_free(alloc, free_feedback_src);
               simple_mtx_lock(&sem->feedback.src_lists_mtx);
   list_add(&free_feedback_src->head, &sem->feedback.pending_src_list);
                        for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      if (dev->queue_families[i] == queue_vk->queue_family_index) {
      *cmd_handle = free_feedback_src->commands[i];
                     }
      struct vn_feedback_cmds {
      union {
      void *cmds;
   VkCommandBuffer *cmd_buffers;
         };
      static inline VkCommandBuffer *
   vn_get_feedback_cmd_handle(struct vn_queue_submission *submit,
               {
      assert((submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO) ||
            return submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO
            }
      static VkResult
   vn_combine_query_feedback_batches_and_record(
      VkDevice dev_handle,
   VkCommandBuffer *cmd_handles,
   uint32_t cmd_count,
   uint32_t stride,
   struct vn_feedback_cmd_pool *feedback_cmd_pool,
      {
      struct vn_command_pool *cmd_pool =
                  struct list_head combined_batches;
            uintptr_t cmd_handle_ptr = (uintptr_t)cmd_handles;
   for (uint32_t i = 0; i < cmd_count; i++) {
      struct vn_command_buffer *cmd =
            list_for_each_entry(struct vn_feedback_query_batch, batch,
            if (!batch->copy) {
      list_for_each_entry_safe(struct vn_feedback_query_batch,
            /* If we previously added a query feedback that is now getting
   * reset, remove it since it is now a no-op and the deferred
   * feedback copy will cause a hang waiting for the reset query
   * to become available.
   */
   if (batch_clone->copy &&
      (vn_query_pool_to_handle(batch_clone->query_pool) ==
   vn_query_pool_to_handle(batch->query_pool)) &&
   batch_clone->query >= batch->query &&
   batch_clone->query < batch->query + batch->query_count) {
   simple_mtx_lock(&feedback_cmd_pool->mutex);
   list_move_to(&batch_clone->head,
                           simple_mtx_lock(&feedback_cmd_pool->mutex);
   struct vn_feedback_query_batch *batch_clone =
      vn_cmd_query_batch_alloc(cmd_pool, batch->query_pool,
            simple_mtx_unlock(&feedback_cmd_pool->mutex);
   if (!batch_clone) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
                                       struct vn_query_feedback_cmd *feedback_cmd;
   result = vn_feedback_query_cmd_alloc(dev_handle, feedback_cmd_pool,
         if (result == VK_SUCCESS) {
      result = vn_feedback_query_batch_record(dev_handle, feedback_cmd,
         if (result != VK_SUCCESS)
            recycle_combined_batches:
      simple_mtx_lock(&feedback_cmd_pool->mutex);
   list_for_each_entry_safe(struct vn_feedback_query_batch, batch_clone,
                                    }
      static VkResult
   vn_queue_submission_add_query_feedback(struct vn_queue_submission *submit,
               {
      struct vk_queue *queue_vk = vk_queue_from_handle(submit->queue_handle);
   VkDevice dev_handle = vk_device_to_handle(queue_vk->base.device);
   struct vn_device *dev = vn_device_from_handle(dev_handle);
   VkCommandBuffer *src_cmd_handles =
         VkCommandBuffer *feedback_cmd_handle =
         const uint32_t stride = submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO
                  struct vn_feedback_cmd_pool *feedback_cmd_pool = NULL;
   for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      if (dev->queue_families[i] == queue_vk->queue_family_index) {
      feedback_cmd_pool = &dev->cmd_pools[i];
                  struct vn_query_feedback_cmd *feedback_cmd;
   VkResult result = vn_combine_query_feedback_batches_and_record(
      dev_handle, src_cmd_handles, cmd_count, stride, feedback_cmd_pool,
      if (result != VK_SUCCESS)
                     /* link query feedback cmd lifecycle with a cmd in the original batch so
   * that the feedback cmd can be reset and recycled when that cmd gets
   * reset/freed.
   *
   * Avoid cmd buffers with VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT
   * since we don't know if all its instances have completed execution.
   * Should be rare enough to just log and leak the feedback cmd.
   */
   struct vn_command_buffer *linked_cmd = NULL;
   for (uint32_t i = 0; i < cmd_count; i++) {
      VkCommandBuffer *cmd_handle =
         struct vn_command_buffer *cmd =
         if (!cmd->builder.is_simultaneous) {
      linked_cmd = cmd;
                  if (!linked_cmd) {
      vn_log(dev->instance,
                     /* If a cmd that was submitted previously and already has a feedback cmd
   * linked, as long as VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT is not
   * set we can assume it has completed execution and is no longer in the
   * pending state so its safe to recycle the old feedback command before
   * linking a new one. Defer the actual recycle operation to
   * vn_queue_submission_cleanup.
   */
   if (linked_cmd->linked_query_feedback_cmd)
      submit->recycle_query_feedback_cmd =
                     }
      static VkResult
   vn_queue_submission_add_sem_feedback(struct vn_queue_submission *submit,
                     {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   uint32_t signal_semaphore_count =
                  /* Set the sem feedback cmds we appended in our copy of cmd buffers
   * with cmds to write the signal value.
   */
   uint32_t cmd_index = cmd_buffer_count;
   for (uint32_t i = 0; i < signal_semaphore_count; i++) {
      struct vn_semaphore *sem = vn_semaphore_from_handle(
            if (sem->feedback.slot) {
                                    result = vn_set_sem_feedback_cmd(queue, sem, counter, cmd_handle);
                                    }
      static VkResult
   vn_queue_submission_add_feedback_cmds(struct vn_queue_submission *submit,
                                       {
      VkResult result;
            /* Update SubmitInfo to use our copy of cmd buffers with sem adn query
   * feedback cmds appended and update the cmd buffer count.
   * SubmitInfo2 also needs to initialize the cmd buffer info struct.
   */
   switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO: {
               submit_info->pCommandBuffers = feedback_cmds->cmd_buffers;
   submit_info->commandBufferCount = new_cmd_buffer_count;
      }
   case VK_STRUCTURE_TYPE_SUBMIT_INFO_2: {
      VkSubmitInfo2 *submit_info2 =
            for (uint32_t i = cmd_buffer_count; i < new_cmd_buffer_count; i++) {
                     cmd_buffer_info->sType =
         cmd_buffer_info->pNext = NULL;
               submit_info2->pCommandBufferInfos = feedback_cmds->cmd_buffer_infos;
   submit_info2->commandBufferInfoCount = new_cmd_buffer_count;
      }
   default:
                  if (batch_has_feedback_query) {
      result = vn_queue_submission_add_query_feedback(
         if (result != VK_SUCCESS)
            /* increment for the cmd buffer used for query feedback cmd */
               if (batch_has_feedback_sem) {
      result = vn_queue_submission_add_sem_feedback(
         if (result != VK_SUCCESS)
                  }
      static const VkCommandBuffer *
   vn_get_fence_feedback_cmd(struct vn_queue *queue, struct vn_fence *fence)
   {
      struct vk_queue *queue_vk = &queue->base.base;
                     for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      if (dev->queue_families[i] == queue_vk->queue_family_index)
                  }
      static void
   vn_queue_submission_add_fence_feedback(
      struct vn_queue_submission *submit,
      {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
                     const VkCommandBuffer *cmd_handle =
            /* These structs were not initialized during alloc_storage */
   switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO: {
      VkSubmitInfo *submit_info =
            *submit_info = (VkSubmitInfo){
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .commandBufferCount = 1,
      };
      }
   case VK_STRUCTURE_TYPE_SUBMIT_INFO_2: {
      *fence_feedback_cmd = (VkCommandBufferSubmitInfo){
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
               VkSubmitInfo2 *submit_info2 =
            *submit_info2 = (VkSubmitInfo2){
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
   .commandBufferInfoCount = 1,
      };
      }
   default:
                     }
      static VkResult
   vn_queue_submission_setup_batches(struct vn_queue_submission *submit)
   {
      VkResult result;
   size_t batch_size = 0;
            if (!submit->has_feedback_fence && !submit->has_feedback_semaphore &&
      !submit->has_feedback_query)
         switch (submit->batch_type) {
   case VK_STRUCTURE_TYPE_SUBMIT_INFO:
      batch_size = sizeof(VkSubmitInfo);
   cmd_size = sizeof(VkCommandBuffer);
      case VK_STRUCTURE_TYPE_SUBMIT_INFO_2:
      batch_size = sizeof(VkSubmitInfo2);
   cmd_size = sizeof(VkCommandBufferSubmitInfo);
      default:
                  /* Copy batches and leave an empty batch for fence feedback.
   * Timeline semaphore and query feedback also require a copy
   * to modify cmd buffer.
   * Only needed for non-empty submissions
   */
   if (submit->batches) {
      memcpy(submit->temp.batches, submit->batches,
               /* For any batches with semaphore or query feedback, copy
   * the original cmd_buffer handles and append feedback cmds.
   */
   uint32_t cmd_offset = 0;
   for (uint32_t batch_index = 0; batch_index < submit->batch_count;
      batch_index++) {
   uint32_t cmd_buffer_count =
         uint32_t signal_count =
            bool batch_has_feedback_sem = false;
   uint32_t feedback_cmd_count = 0;
   for (uint32_t i = 0; i < signal_count; i++) {
                     if (sem->feedback.slot) {
      feedback_cmd_count++;
                  bool batch_has_feedback_query = false;
   for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      struct vn_command_buffer *cmd = vn_command_buffer_from_handle(
         if (!list_is_empty(&cmd->builder.query_batches)) {
                     if (batch_has_feedback_query)
            if (feedback_cmd_count) {
      struct vn_feedback_cmds feedback_cmds = {
                  size_t cmd_buffer_size = cmd_buffer_count * cmd_size;
   /* copy only needed for non-empty batches */
   if (cmd_buffer_size) {
      memcpy(feedback_cmds.cmds,
                     result = vn_queue_submission_add_feedback_cmds(
      submit, batch_index, cmd_buffer_count, feedback_cmd_count,
                     /* Set offset to next batches cmd_buffers */
                  if (submit->has_feedback_fence) {
      VkCommandBufferSubmitInfo *fence_feedback_cmd =
                                 }
      static void
   vn_queue_sem_recycle_src_feedback(VkDevice dev_handle, VkSemaphore sem_handle)
   {
                  if (!sem->feedback.slot)
            uint64_t curr_counter = 0;
            /* search pending src list for already signaled values*/
   simple_mtx_lock(&sem->feedback.src_lists_mtx);
   list_for_each_entry_safe(struct vn_feedback_src, feedback_src,
            if (curr_counter >= vn_feedback_get_counter(feedback_src->src_slot)) {
            }
      }
      static void
   vn_queue_recycle_src_feedback(struct vn_queue_submission *submit)
   {
      struct vk_queue *queue_vk = vk_queue_from_handle(submit->queue_handle);
            for (uint32_t batch_index = 0; batch_index < submit->batch_count;
               uint32_t wait_count = vn_get_wait_semaphore_count(submit, batch_index);
   uint32_t signal_count =
            for (uint32_t i = 0; i < wait_count; i++) {
      VkSemaphore sem_handle =
                     for (uint32_t i = 0; i < signal_count; i++) {
      VkSemaphore sem_handle =
                  }
      static void
   vn_queue_submission_cleanup(struct vn_queue_submission *submit)
   {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
            if (submit->recycle_query_feedback_cmd) {
      vn_ResetCommandBuffer(
      vn_command_buffer_to_handle(submit->recycle_query_feedback_cmd->cmd),
      list_add(
      &submit->recycle_query_feedback_cmd->head,
            /* TODO clean up pending src feedbacks on failure? */
   if (submit->has_feedback_semaphore)
            if (submit->has_feedback_fence || submit->has_feedback_semaphore ||
      submit->has_feedback_query)
   }
      static VkResult
   vn_queue_submission_prepare_submit(struct vn_queue_submission *submit)
   {
      VkResult result = vn_queue_submission_prepare(submit);
   if (result != VK_SUCCESS)
            result = vn_queue_submission_alloc_storage(submit);
   if (result != VK_SUCCESS)
            result = vn_queue_submission_setup_batches(submit);
   if (result != VK_SUCCESS) {
      vn_queue_submission_cleanup(submit);
                  }
      static void
   vn_queue_wsi_present(struct vn_queue_submission *submit)
   {
      struct vk_queue *queue_vk = vk_queue_from_handle(submit->queue_handle);
   struct vn_device *dev = (void *)queue_vk->base.device;
            if (!submit->wsi_mem)
            if (instance->renderer->info.has_implicit_fencing) {
      struct vn_renderer_submit_batch batch = {
                  uint32_t local_data[8];
   struct vn_cs_encoder local_enc =
         if (submit->external_payload.ring_seqno_valid) {
      vn_encode_vkWaitRingSeqnoMESA(&local_enc, 0, instance->ring.id,
         batch.cs_data = local_data;
               const struct vn_renderer_submit renderer_submit = {
      .bos = &submit->wsi_mem->base_bo,
   .bo_count = 1,
   .batches = &batch,
      };
      } else {
      if (VN_DEBUG(WSI)) {
               if (num_rate_limit_warning++ < 10)
                     }
      static VkResult
   vn_queue_submit(struct vn_queue_submission *submit)
   {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   struct vn_device *dev = (void *)queue->base.base.base.device;
   struct vn_instance *instance = dev->instance;
            /* To ensure external components waiting on the correct fence payload,
   * below sync primitives must be installed after the submission:
   * - explicit fencing: sync file export
   * - implicit fencing: dma-fence attached to the wsi bo
   *
   * We enforce above via an asynchronous vkQueueSubmit(2) via ring followed
   * by an asynchronous renderer submission to wait for the ring submission:
   * - struct wsi_memory_signal_submit_info
   * - fence is an external fence
   * - has an external signal semaphore
   */
   result = vn_queue_submission_prepare_submit(submit);
   if (result != VK_SUCCESS)
            /* skip no-op submit */
   if (!submit->batch_count && submit->fence_handle == VK_NULL_HANDLE)
            if (VN_PERF(NO_ASYNC_QUEUE_SUBMIT)) {
      if (submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO_2) {
      result = vn_call_vkQueueSubmit2(
      instance, submit->queue_handle, submit->batch_count,
   } else {
      result = vn_call_vkQueueSubmit(
      instance, submit->queue_handle, submit->batch_count,
            if (result != VK_SUCCESS) {
      vn_queue_submission_cleanup(submit);
         } else {
      struct vn_instance_submit_command instance_submit;
   if (submit->batch_type == VK_STRUCTURE_TYPE_SUBMIT_INFO_2) {
      vn_submit_vkQueueSubmit2(
      instance, 0, submit->queue_handle, submit->batch_count,
   } else {
      vn_submit_vkQueueSubmit(instance, 0, submit->queue_handle,
            }
   if (!instance_submit.ring_seqno_valid) {
      vn_queue_submission_cleanup(submit);
      }
   submit->external_payload.ring_seqno_valid = true;
               /* If external fence, track the submission's ring_idx to facilitate
   * sync_file export.
   *
   * Imported syncs don't need a proxy renderer sync on subsequent export,
   * because an fd is already available.
   */
   struct vn_fence *fence = vn_fence_from_handle(submit->fence_handle);
   if (fence && fence->is_external) {
      assert(fence->payload->type == VN_SYNC_TYPE_DEVICE_ONLY);
               for (uint32_t i = 0; i < submit->batch_count; i++) {
      uint32_t signal_semaphore_count =
         for (uint32_t j = 0; j < signal_semaphore_count; j++) {
      struct vn_semaphore *sem =
         if (sem->is_external) {
      assert(sem->payload->type == VN_SYNC_TYPE_DEVICE_ONLY);
                                          }
      VkResult
   vn_QueueSubmit(VkQueue queue,
                     {
               struct vn_queue_submission submit = {
      .batch_type = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .queue_handle = queue,
   .batch_count = submitCount,
   .submit_batches = pSubmits,
                  }
      VkResult
   vn_QueueSubmit2(VkQueue queue,
                     {
               struct vn_queue_submission submit = {
      .batch_type = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
   .queue_handle = queue,
   .batch_count = submitCount,
   .submit_batches2 = pSubmits,
                  }
      static VkResult
   vn_queue_bind_sparse_submit(struct vn_queue_submission *submit)
   {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   struct vn_device *dev = (void *)queue->base.base.base.device;
   struct vn_instance *instance = dev->instance;
            if (VN_PERF(NO_ASYNC_QUEUE_SUBMIT)) {
      result = vn_call_vkQueueBindSparse(
      instance, submit->queue_handle, submit->batch_count,
      if (result != VK_SUCCESS)
      } else {
      struct vn_instance_submit_command instance_submit;
   vn_submit_vkQueueBindSparse(instance, 0, submit->queue_handle,
                  if (!instance_submit.ring_seqno_valid)
                  }
      static VkResult
   vn_queue_bind_sparse_submit_batch(struct vn_queue_submission *submit,
         {
      struct vn_queue *queue = vn_queue_from_handle(submit->queue_handle);
   VkDevice dev_handle = vk_device_to_handle(queue->base.base.base.device);
   const VkBindSparseInfo *sparse_info = &submit->sparse_batches[batch_index];
   const VkSemaphore *signal_sem = sparse_info->pSignalSemaphores;
   uint32_t signal_sem_count = sparse_info->signalSemaphoreCount;
            struct vn_queue_submission sparse_batch = {
      .batch_type = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
   .queue_handle = submit->queue_handle,
   .batch_count = 1,
               /* lazily create sparse semaphore */
   if (queue->sparse_semaphore == VK_NULL_HANDLE) {
      queue->sparse_semaphore_counter = 1;
   const VkSemaphoreTypeCreateInfo sem_type_create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
   .pNext = NULL,
   /* This must be timeline type to adhere to mesa's requirement
   * not to mix binary semaphores with wait-before-signal.
   */
   .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      };
   const VkSemaphoreCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
   .pNext = &sem_type_create_info,
               result = vn_CreateSemaphore(dev_handle, &create_info, NULL,
         if (result != VK_SUCCESS)
               /* Setup VkTimelineSemaphoreSubmitInfo's for our queue sparse semaphore
   * so that the vkQueueSubmit waits on the vkQueueBindSparse signal.
   */
   queue->sparse_semaphore_counter++;
   struct VkTimelineSemaphoreSubmitInfo wait_timeline_sem_info = { 0 };
   wait_timeline_sem_info.sType =
         wait_timeline_sem_info.signalSemaphoreValueCount = 1;
   wait_timeline_sem_info.pSignalSemaphoreValues =
            struct VkTimelineSemaphoreSubmitInfo signal_timeline_sem_info = { 0 };
   signal_timeline_sem_info.sType =
         signal_timeline_sem_info.waitSemaphoreValueCount = 1;
   signal_timeline_sem_info.pWaitSemaphoreValues =
            /* Split up the original wait and signal semaphores into its respective
   * vkTimelineSemaphoreSubmitInfo
   */
   const struct VkTimelineSemaphoreSubmitInfo *timeline_sem_info =
      vk_find_struct_const(sparse_info->pNext,
      if (timeline_sem_info) {
      if (timeline_sem_info->waitSemaphoreValueCount) {
      wait_timeline_sem_info.waitSemaphoreValueCount =
         wait_timeline_sem_info.pWaitSemaphoreValues =
               if (timeline_sem_info->signalSemaphoreValueCount) {
      signal_timeline_sem_info.signalSemaphoreValueCount =
         signal_timeline_sem_info.pSignalSemaphoreValues =
                  /* Attach the original VkDeviceGroupBindSparseInfo if it exists */
   struct VkDeviceGroupBindSparseInfo batch_device_group_info;
   const struct VkDeviceGroupBindSparseInfo *device_group_info =
         if (device_group_info) {
      memcpy(&batch_device_group_info, device_group_info,
                              /* Copy the original batch VkBindSparseInfo modified to signal
   * our sparse semaphore.
   */
   VkBindSparseInfo batch_sparse_info;
            batch_sparse_info.pNext = &wait_timeline_sem_info;
   batch_sparse_info.signalSemaphoreCount = 1;
            /* Set up the SubmitInfo to wait on our sparse semaphore before sending
   * feedback and signaling the original semaphores/fence
   *
   * Even if this VkBindSparse batch does not have feedback semaphores,
   * we still glue all the batches together to ensure the feedback
   * fence occurs after.
   */
   VkPipelineStageFlags stage_masks = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
   VkSubmitInfo batch_submit_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
   .pNext = &signal_timeline_sem_info,
   .waitSemaphoreCount = 1,
   .pWaitSemaphores = &queue->sparse_semaphore,
   .pWaitDstStageMask = &stage_masks,
   .signalSemaphoreCount = signal_sem_count,
               /* Set the possible fence if on the last batch */
   VkFence fence_handle = VK_NULL_HANDLE;
   if (submit->has_feedback_fence &&
      batch_index == (submit->batch_count - 1)) {
               sparse_batch.sparse_batches = &batch_sparse_info;
   result = vn_queue_bind_sparse_submit(&sparse_batch);
   if (result != VK_SUCCESS)
            result = vn_QueueSubmit(submit->queue_handle, 1, &batch_submit_info,
         if (result != VK_SUCCESS)
               }
      VkResult
   vn_QueueBindSparse(VkQueue queue,
                     {
      VN_TRACE_FUNC();
            struct vn_queue_submission submit = {
      .batch_type = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO,
   .queue_handle = queue,
   .batch_count = bindInfoCount,
   .sparse_batches = pBindInfo,
               result = vn_queue_submission_prepare(&submit);
   if (result != VK_SUCCESS)
            if (!submit.batch_count) {
      /* skip no-op submit */
   if (submit.fence_handle == VK_NULL_HANDLE)
            /* if empty batch, just send a vkQueueSubmit with the fence */
   result =
         if (result != VK_SUCCESS)
               /* if feedback isn't used in the batch, can directly submit */
   if (!submit.has_feedback_fence && !submit.has_feedback_semaphore &&
      !submit.has_feedback_query) {
               for (uint32_t i = 0; i < submit.batch_count; i++) {
      result = vn_queue_bind_sparse_submit_batch(&submit, i);
   if (result != VK_SUCCESS)
                  }
      VkResult
   vn_QueueWaitIdle(VkQueue _queue)
   {
      VN_TRACE_FUNC();
   struct vn_queue *queue = vn_queue_from_handle(_queue);
   VkDevice dev_handle = vk_device_to_handle(queue->base.base.base.device);
   struct vn_device *dev = vn_device_from_handle(dev_handle);
            /* lazily create queue wait fence for queue idle waiting */
   if (queue->wait_fence == VK_NULL_HANDLE) {
      const VkFenceCreateInfo create_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      };
   result =
         if (result != VK_SUCCESS)
               result = vn_QueueSubmit(_queue, 0, NULL, queue->wait_fence);
   if (result != VK_SUCCESS)
            result =
                     }
      /* fence commands */
      static void
   vn_sync_payload_release(UNUSED struct vn_device *dev,
         {
      if (payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD && payload->fd >= 0)
               }
      static VkResult
   vn_fence_init_payloads(struct vn_device *dev,
                     {
      fence->permanent.type = VN_SYNC_TYPE_DEVICE_ONLY;
   fence->temporary.type = VN_SYNC_TYPE_INVALID;
               }
      void
   vn_fence_signal_wsi(struct vn_device *dev, struct vn_fence *fence)
   {
               vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = -1;
      }
      static VkResult
   vn_fence_feedback_init(struct vn_device *dev,
                     {
      VkDevice dev_handle = vn_device_to_handle(dev);
   struct vn_feedback_slot *slot;
   VkCommandBuffer *cmd_handles;
            if (fence->is_external)
            /* Fence feedback implementation relies on vkWaitForFences to cover the gap
   * between feedback slot signaling and the actual fence signal operation.
   */
   if (unlikely(!dev->instance->renderer->info.allow_vk_wait_syncs))
            if (VN_PERF(NO_FENCE_FEEDBACK))
            slot = vn_feedback_pool_alloc(&dev->feedback_pool, VN_FEEDBACK_TYPE_FENCE);
   if (!slot)
                     cmd_handles =
      vk_zalloc(alloc, sizeof(*cmd_handles) * dev->queue_family_count,
      if (!cmd_handles) {
      vn_feedback_pool_free(&dev->feedback_pool, slot);
               for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      result = vn_feedback_cmd_alloc(dev_handle, &dev->cmd_pools[i], slot,
         if (result != VK_SUCCESS) {
      for (uint32_t j = 0; j < i; j++) {
      vn_feedback_cmd_free(dev_handle, &dev->cmd_pools[j],
      }
                  if (result != VK_SUCCESS) {
      vk_free(alloc, cmd_handles);
   vn_feedback_pool_free(&dev->feedback_pool, slot);
               fence->feedback.slot = slot;
               }
      static void
   vn_fence_feedback_fini(struct vn_device *dev,
               {
               if (!fence->feedback.slot)
            for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      vn_feedback_cmd_free(dev_handle, &dev->cmd_pools[i],
                           }
      VkResult
   vn_CreateFence(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
         const bool signaled = pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT;
            struct vn_fence *fence = vk_zalloc(alloc, sizeof(*fence), VN_DEFAULT_ALIGN,
         if (!fence)
                     const struct VkExportFenceCreateInfo *export_info =
                  result = vn_fence_init_payloads(dev, fence, signaled, alloc);
   if (result != VK_SUCCESS)
            result = vn_fence_feedback_init(dev, fence, signaled, alloc);
   if (result != VK_SUCCESS)
            *pFence = vn_fence_to_handle(fence);
                  out_payloads_fini:
      vn_sync_payload_release(dev, &fence->permanent);
         out_object_base_fini:
      vn_object_base_fini(&fence->base);
   vk_free(alloc, fence);
      }
      void
   vn_DestroyFence(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(_fence);
   const VkAllocationCallbacks *alloc =
            if (!fence)
                              vn_sync_payload_release(dev, &fence->permanent);
            vn_object_base_fini(&fence->base);
      }
      VkResult
   vn_ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences)
   {
      VN_TRACE_FUNC();
            /* TODO if the fence is shared-by-ref, this needs to be synchronous */
   if (false)
         else
            for (uint32_t i = 0; i < fenceCount; i++) {
      struct vn_fence *fence = vn_fence_from_handle(pFences[i]);
                     assert(perm->type == VN_SYNC_TYPE_DEVICE_ONLY);
            if (fence->feedback.slot)
                  }
      VkResult
   vn_GetFenceStatus(VkDevice device, VkFence _fence)
   {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(_fence);
            VkResult result;
   switch (payload->type) {
   case VN_SYNC_TYPE_DEVICE_ONLY:
      if (fence->feedback.slot) {
      result = vn_feedback_get_status(fence->feedback.slot);
   if (result == VK_SUCCESS) {
      /* When fence feedback slot gets signaled, the real fence
   * signal operation follows after but the signaling isr can be
   * deferred or preempted. To avoid racing, we let the
   * renderer wait for the fence. This also helps resolve
   * synchronization validation errors, because the layer no
   * longer sees any fence status checks and falsely believes the
   * caller does not sync.
   */
   vn_async_vkWaitForFences(dev->instance, device, 1, &_fence,
         } else {
         }
      case VN_SYNC_TYPE_IMPORTED_SYNC_FD:
      if (payload->fd < 0 || sync_wait(payload->fd, 0) == 0)
         else
            default:
      unreachable("unexpected fence payload type");
                  }
      static VkResult
   vn_find_first_signaled_fence(VkDevice device,
               {
      for (uint32_t i = 0; i < count; i++) {
      VkResult result = vn_GetFenceStatus(device, fences[i]);
   if (result == VK_SUCCESS || result < 0)
      }
      }
      static VkResult
   vn_remove_signaled_fences(VkDevice device, VkFence *fences, uint32_t *count)
   {
      uint32_t cur = 0;
   for (uint32_t i = 0; i < *count; i++) {
      VkResult result = vn_GetFenceStatus(device, fences[i]);
   if (result != VK_SUCCESS) {
      if (result < 0)
                        *count = cur;
      }
      static VkResult
   vn_update_sync_result(struct vn_device *dev,
                     {
      switch (result) {
   case VK_NOT_READY:
      if (abs_timeout != OS_TIMEOUT_INFINITE &&
      os_time_get_nano() >= abs_timeout)
      else
            default:
      assert(result == VK_SUCCESS || result < 0);
                  }
      VkResult
   vn_WaitForFences(VkDevice device,
                  uint32_t fenceCount,
      {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            const int64_t abs_timeout = os_time_get_absolute_timeout(timeout);
   VkResult result = VK_NOT_READY;
   if (fenceCount > 1 && waitAll) {
      VkFence local_fences[8];
   VkFence *fences = local_fences;
   if (fenceCount > ARRAY_SIZE(local_fences)) {
      fences =
      vk_alloc(alloc, sizeof(*fences) * fenceCount, VN_DEFAULT_ALIGN,
      if (!fences)
      }
            struct vn_relax_state relax_state =
         while (result == VK_NOT_READY) {
      result = vn_remove_signaled_fences(device, fences, &fenceCount);
   result =
      }
            if (fences != local_fences)
      } else {
      struct vn_relax_state relax_state =
         while (result == VK_NOT_READY) {
      result = vn_find_first_signaled_fence(device, pFences, fenceCount);
   result =
      }
                  }
      static VkResult
   vn_create_sync_file(struct vn_device *dev,
               {
      struct vn_renderer_sync *sync;
   VkResult result = vn_renderer_sync_create(dev->renderer, 0,
         if (result != VK_SUCCESS)
            struct vn_renderer_submit_batch batch = {
      .syncs = &sync,
   .sync_values = &(const uint64_t){ 1 },
   .sync_count = 1,
               uint32_t local_data[8];
   struct vn_cs_encoder local_enc =
         if (external_payload->ring_seqno_valid) {
      vn_encode_vkWaitRingSeqnoMESA(&local_enc, 0, dev->instance->ring.id,
         batch.cs_data = local_data;
               const struct vn_renderer_submit submit = {
      .batches = &batch,
      };
   result = vn_renderer_submit(dev->renderer, &submit);
   if (result != VK_SUCCESS) {
      vn_renderer_sync_destroy(dev->renderer, sync);
               *out_fd = vn_renderer_sync_export_syncobj(dev->renderer, sync, true);
               }
      static inline bool
   vn_sync_valid_fd(int fd)
   {
      /* the special value -1 for fd is treated like a valid sync file descriptor
   * referring to an object that has already signaled
   */
      }
      VkResult
   vn_ImportFenceFdKHR(VkDevice device,
         {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(pImportFenceFdInfo->fence);
   ASSERTED const bool sync_file = pImportFenceFdInfo->handleType ==
                           if (!vn_sync_valid_fd(fd))
            struct vn_sync_payload *temp = &fence->temporary;
   vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = fd;
               }
      VkResult
   vn_GetFenceFdKHR(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_fence *fence = vn_fence_from_handle(pGetFdInfo->fence);
   const bool sync_file =
         struct vn_sync_payload *payload = fence->payload;
            assert(sync_file);
            int fd = -1;
   if (payload->type == VN_SYNC_TYPE_DEVICE_ONLY) {
      result = vn_create_sync_file(dev, &fence->external_payload, &fd);
   if (result != VK_SUCCESS)
            vn_async_vkResetFenceResourceMESA(dev->instance, device,
            vn_sync_payload_release(dev, &fence->temporary);
      #ifdef VN_USE_WSI_PLATFORM
         if (!dev->renderer->info.has_implicit_fencing)
   #endif
      } else {
               /* transfer ownership of imported sync fd to save a dup */
   fd = payload->fd;
            /* reset host fence in case in signaled state before import */
   result = vn_ResetFences(device, 1, &pGetFdInfo->fence);
   if (result != VK_SUCCESS) {
      /* transfer sync fd ownership back on error */
   payload->fd = fd;
                  *pFd = fd;
      }
      /* semaphore commands */
      static VkResult
   vn_semaphore_init_payloads(struct vn_device *dev,
                     {
      sem->permanent.type = VN_SYNC_TYPE_DEVICE_ONLY;
   sem->temporary.type = VN_SYNC_TYPE_INVALID;
               }
      static bool
   vn_semaphore_wait_external(struct vn_device *dev, struct vn_semaphore *sem)
   {
                        if (temp->fd >= 0) {
      if (sync_wait(temp->fd, -1))
               vn_sync_payload_release(dev, &sem->temporary);
               }
      void
   vn_semaphore_signal_wsi(struct vn_device *dev, struct vn_semaphore *sem)
   {
               vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = -1;
      }
      static VkResult
   vn_timeline_semaphore_feedback_init(struct vn_device *dev,
                     {
                        if (sem->is_external)
            if (VN_PERF(NO_TIMELINE_SEM_FEEDBACK))
            slot = vn_feedback_pool_alloc(&dev->feedback_pool,
         if (!slot)
            list_inithead(&sem->feedback.pending_src_list);
                     simple_mtx_init(&sem->feedback.src_lists_mtx, mtx_plain);
            sem->feedback.signaled_counter = initial_value;
               }
      static void
   vn_timeline_semaphore_feedback_free(struct vn_device *dev,
         {
      VkDevice dev_handle = vn_device_to_handle(dev);
            for (uint32_t i = 0; i < dev->queue_family_count; i++) {
      vn_feedback_cmd_free(dev_handle, &dev->cmd_pools[i],
      }
            vn_feedback_pool_free(&dev->feedback_pool, feedback_src->src_slot);
   /* feedback_src was allocated laziy at submission time using the
   * device level alloc, not the vkCreateSemaphore passed alloc
   */
      }
      static void
   vn_timeline_semaphore_feedback_fini(struct vn_device *dev,
         {
      if (!sem->feedback.slot)
            list_for_each_entry_safe(struct vn_feedback_src, feedback_src,
                        list_for_each_entry_safe(struct vn_feedback_src, feedback_src,
                        simple_mtx_destroy(&sem->feedback.src_lists_mtx);
               }
      VkResult
   vn_CreateSemaphore(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_semaphore *sem = vk_zalloc(alloc, sizeof(*sem), VN_DEFAULT_ALIGN,
         if (!sem)
                     const VkSemaphoreTypeCreateInfo *type_info =
         uint64_t initial_val = 0;
   if (type_info && type_info->semaphoreType == VK_SEMAPHORE_TYPE_TIMELINE) {
      sem->type = VK_SEMAPHORE_TYPE_TIMELINE;
      } else {
                  const struct VkExportSemaphoreCreateInfo *export_info =
                  VkResult result = vn_semaphore_init_payloads(dev, sem, initial_val, alloc);
   if (result != VK_SUCCESS)
            if (sem->type == VK_SEMAPHORE_TYPE_TIMELINE) {
      result =
         if (result != VK_SUCCESS)
               VkSemaphore sem_handle = vn_semaphore_to_handle(sem);
   vn_async_vkCreateSemaphore(dev->instance, device, pCreateInfo, NULL,
                           out_payloads_fini:
      vn_sync_payload_release(dev, &sem->permanent);
         out_object_base_fini:
      vn_object_base_fini(&sem->base);
   vk_free(alloc, sem);
      }
      void
   vn_DestroySemaphore(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem = vn_semaphore_from_handle(semaphore);
   const VkAllocationCallbacks *alloc =
            if (!sem)
                     if (sem->type == VK_SEMAPHORE_TYPE_TIMELINE)
            vn_sync_payload_release(dev, &sem->permanent);
            vn_object_base_fini(&sem->base);
      }
      VkResult
   vn_GetSemaphoreCounterValue(VkDevice device,
               {
      struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem = vn_semaphore_from_handle(semaphore);
                     if (sem->feedback.slot) {
                        if (sem->feedback.signaled_counter < *pValue) {
      /* When the timeline semaphore feedback slot gets signaled, the real
   * semaphore signal operation follows after but the signaling isr can
   * be deferred or preempted. To avoid racing, we let the renderer
   * wait for the semaphore by sending an asynchronous wait call for
   * the feedback value.
   * We also cache the counter value to only send the async call once
   * per counter value to prevent spamming redundant async wait calls.
   * The cached counter value requires a lock to ensure multiple
   * threads querying for the same value are guaranteed to encode after
   * the async wait call.
   *
   * This also helps resolve synchronization validation errors, because
   * the layer no longer sees any semaphore status checks and falsely
   * believes the caller does not sync.
   */
   VkSemaphoreWaitInfo wait_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
   .pNext = NULL,
   .flags = 0,
   .semaphoreCount = 1,
   .pSemaphores = &semaphore,
               vn_async_vkWaitSemaphores(dev->instance, device, &wait_info,
            }
               } else {
      return vn_call_vkGetSemaphoreCounterValue(dev->instance, device,
         }
      VkResult
   vn_SignalSemaphore(VkDevice device, const VkSemaphoreSignalInfo *pSignalInfo)
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem =
            /* TODO if the semaphore is shared-by-ref, this needs to be synchronous */
   if (false)
         else
            if (sem->feedback.slot) {
               vn_feedback_set_counter(sem->feedback.slot, pSignalInfo->value);
   /* Update async counters. Since we're signaling, we're aligned with
   * the renderer.
   */
                           }
      static VkResult
   vn_find_first_signaled_semaphore(VkDevice device,
                     {
      for (uint32_t i = 0; i < count; i++) {
      uint64_t val = 0;
   VkResult result =
         if (result != VK_SUCCESS || val >= values[i])
      }
      }
      static VkResult
   vn_remove_signaled_semaphores(VkDevice device,
                     {
      uint32_t cur = 0;
   for (uint32_t i = 0; i < *count; i++) {
      uint64_t val = 0;
   VkResult result =
         if (result != VK_SUCCESS)
         if (val < values[i])
               *count = cur;
      }
      VkResult
   vn_WaitSemaphores(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            const int64_t abs_timeout = os_time_get_absolute_timeout(timeout);
   VkResult result = VK_NOT_READY;
   if (pWaitInfo->semaphoreCount > 1 &&
      !(pWaitInfo->flags & VK_SEMAPHORE_WAIT_ANY_BIT)) {
   uint32_t semaphore_count = pWaitInfo->semaphoreCount;
   VkSemaphore local_semaphores[8];
   uint64_t local_values[8];
   VkSemaphore *semaphores = local_semaphores;
   uint64_t *values = local_values;
   if (semaphore_count > ARRAY_SIZE(local_semaphores)) {
      semaphores = vk_alloc(
      alloc, (sizeof(*semaphores) + sizeof(*values)) * semaphore_count,
                        }
   memcpy(semaphores, pWaitInfo->pSemaphores,
                  struct vn_relax_state relax_state =
         while (result == VK_NOT_READY) {
      result = vn_remove_signaled_semaphores(device, semaphores, values,
         result =
      }
            if (semaphores != local_semaphores)
      } else {
      struct vn_relax_state relax_state =
         while (result == VK_NOT_READY) {
      result = vn_find_first_signaled_semaphore(
      device, pWaitInfo->pSemaphores, pWaitInfo->pValues,
      result =
      }
                  }
      VkResult
   vn_ImportSemaphoreFdKHR(
         {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem =
         ASSERTED const bool sync_file =
      pImportSemaphoreFdInfo->handleType ==
                        if (!vn_sync_valid_fd(fd))
            struct vn_sync_payload *temp = &sem->temporary;
   vn_sync_payload_release(dev, temp);
   temp->type = VN_SYNC_TYPE_IMPORTED_SYNC_FD;
   temp->fd = fd;
               }
      VkResult
   vn_GetSemaphoreFdKHR(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_semaphore *sem = vn_semaphore_from_handle(pGetFdInfo->semaphore);
   const bool sync_file =
                  assert(sync_file);
   assert(dev->physical_device->renderer_sync_fd.semaphore_exportable);
            int fd = -1;
   if (payload->type == VN_SYNC_TYPE_DEVICE_ONLY) {
      VkResult result = vn_create_sync_file(dev, &sem->external_payload, &fd);
   if (result != VK_SUCCESS)
      #ifdef VN_USE_WSI_PLATFORM
         if (!dev->renderer->info.has_implicit_fencing)
   #endif
      } else {
               /* transfer ownership of imported sync fd to save a dup */
   fd = payload->fd;
               /* When payload->type is VN_SYNC_TYPE_IMPORTED_SYNC_FD, the current
   * payload is from a prior temporary sync_fd import. The permanent
   * payload of the sempahore might be in signaled state. So we do an
   * import here to ensure later wait operation is legit. With resourceId
   * 0, renderer does a signaled sync_fd -1 payload import on the host
   * semaphore.
   */
   if (payload->type == VN_SYNC_TYPE_IMPORTED_SYNC_FD) {
      const VkImportSemaphoreResourceInfoMESA res_info = {
      .sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_RESOURCE_INFO_MESA,
   .semaphore = pGetFdInfo->semaphore,
      };
   vn_async_vkImportSemaphoreResourceMESA(dev->instance, device,
               /* perform wait operation on the host semaphore */
   vn_async_vkWaitSemaphoreResourceMESA(dev->instance, device,
            vn_sync_payload_release(dev, &sem->temporary);
            *pFd = fd;
      }
      /* event commands */
      static VkResult
   vn_event_feedback_init(struct vn_device *dev, struct vn_event *ev)
   {
               if (VN_PERF(NO_EVENT_FEEDBACK))
            slot = vn_feedback_pool_alloc(&dev->feedback_pool, VN_FEEDBACK_TYPE_EVENT);
   if (!slot)
            /* newly created event object is in the unsignaled state */
                        }
      static inline void
   vn_event_feedback_fini(struct vn_device *dev, struct vn_event *ev)
   {
      if (ev->feedback_slot)
      }
      VkResult
   vn_CreateEvent(VkDevice device,
                     {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   const VkAllocationCallbacks *alloc =
            struct vn_event *ev = vk_zalloc(alloc, sizeof(*ev), VN_DEFAULT_ALIGN,
         if (!ev)
                     /* feedback is only needed to speed up host operations */
   if (!(pCreateInfo->flags & VK_EVENT_CREATE_DEVICE_ONLY_BIT)) {
      VkResult result = vn_event_feedback_init(dev, ev);
   if (result != VK_SUCCESS)
               VkEvent ev_handle = vn_event_to_handle(ev);
   vn_async_vkCreateEvent(dev->instance, device, pCreateInfo, NULL,
                        }
      void
   vn_DestroyEvent(VkDevice device,
               {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);
   const VkAllocationCallbacks *alloc =
            if (!ev)
                              vn_object_base_fini(&ev->base);
      }
      VkResult
   vn_GetEventStatus(VkDevice device, VkEvent event)
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
   struct vn_event *ev = vn_event_from_handle(event);
            if (ev->feedback_slot)
         else
               }
      VkResult
   vn_SetEvent(VkDevice device, VkEvent event)
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            if (ev->feedback_slot) {
      vn_feedback_set_status(ev->feedback_slot, VK_EVENT_SET);
      } else {
      VkResult result = vn_call_vkSetEvent(dev->instance, device, event);
   if (result != VK_SUCCESS)
                  }
      VkResult
   vn_ResetEvent(VkDevice device, VkEvent event)
   {
      VN_TRACE_FUNC();
   struct vn_device *dev = vn_device_from_handle(device);
            if (ev->feedback_slot) {
      vn_feedback_reset_status(ev->feedback_slot);
      } else {
      VkResult result = vn_call_vkResetEvent(dev->instance, device, event);
   if (result != VK_SUCCESS)
                  }
