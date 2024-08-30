   /*
   * Copyright © 2019 Raspberry Pi Ltd
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3dv_private.h"
   #include "drm-uapi/v3d_drm.h"
      #include "broadcom/clif/clif_dump.h"
   #include "util/libsync.h"
   #include "util/os_time.h"
   #include "vk_drm_syncobj.h"
      #include <errno.h>
   #include <time.h>
      static void
   v3dv_clif_dump(struct v3dv_device *device,
               {
      if (!(V3D_DBG(CL) ||
         V3D_DBG(CL_NO_BIN) ||
            struct clif_dump *clif = clif_dump_init(&device->devinfo,
                              set_foreach(job->bos, entry) {
      struct v3dv_bo *bo = (void *)entry->key;
   char *name = ralloc_asprintf(NULL, "%s_0x%x",
            bool ok = v3dv_bo_map(device, bo, bo->size);
   if (!ok) {
      fprintf(stderr, "failed to map BO for clif_dump.\n");
   ralloc_free(name);
      }
                              free_clif:
         }
      static VkResult
   queue_wait_idle(struct v3dv_queue *queue,
         {
      if (queue->device->pdevice->caps.multisync) {
      int ret = drmSyncobjWait(queue->device->pdevice->render_fd,
                     if (ret) {
      return vk_errorf(queue, VK_ERROR_DEVICE_LOST,
               bool first = true;
   for (int i = 0; i < 3; i++) {
      if (!queue->last_job_syncs.first[i])
               /* If we're not the first job, that means we're waiting on some
   * per-queue-type syncobj which transitively waited on the semaphores
   * so we can skip the semaphore wait.
   */
   if (first) {
      VkResult result = vk_sync_wait_many(&queue->device->vk,
                           if (result != VK_SUCCESS)
         } else {
      /* Without multisync, all the semaphores are baked into the one syncobj
   * at the start of each submit so we only need to wait on the one.
   */
   int ret = drmSyncobjWait(queue->device->pdevice->render_fd,
                     if (ret) {
      return vk_errorf(queue, VK_ERROR_DEVICE_LOST,
                  for (int i = 0; i < 3; i++)
               }
      static VkResult
   handle_reset_query_cpu_job(struct v3dv_queue *queue, struct v3dv_job *job,
         {
      struct v3dv_reset_query_cpu_job_info *info = &job->cpu.query_reset;
            /* We are about to reset query counters so we need to make sure that
   * The GPU is not using them. The exception is timestamp queries, since
   * we handle those in the CPU.
   */
   if (info->pool->query_type == VK_QUERY_TYPE_OCCLUSION)
            if (info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      struct vk_sync_wait waits[info->count];
   unsigned wait_count = 0;
   for (int i = 0; i < info->count; i++) {
      struct v3dv_query *query = &info->pool->queries[i];
   /* Only wait for a query if we've used it otherwise we will be
   * waiting forever for the fence to become signaled.
   */
   if (query->maybe_available) {
      waits[wait_count] = (struct vk_sync_wait){
         };
                  VkResult result = vk_sync_wait_many(&job->device->vk, wait_count, waits,
            if (result != VK_SUCCESS)
                           }
      static VkResult
   export_perfmon_last_job_sync(struct v3dv_queue *queue, struct v3dv_job *job, int *fd)
   {
      int err;
   if (job->device->pdevice->caps.multisync) {
      static const enum v3dv_queue_type queues_to_sync[] = {
      V3DV_QUEUE_CL,
               for (uint32_t i = 0; i < ARRAY_SIZE(queues_to_sync); i++) {
                     err = drmSyncobjExportSyncFile(job->device->pdevice->render_fd,
                  if (err) {
      close(*fd);
   return vk_errorf(&job->device->queue, VK_ERROR_UNKNOWN,
                        if (err) {
      close(tmp_fd);
   close(*fd);
   return vk_errorf(&job->device->queue, VK_ERROR_UNKNOWN,
            } else {
      err = drmSyncobjExportSyncFile(job->device->pdevice->render_fd,
                  if (err) {
      return vk_errorf(&job->device->queue, VK_ERROR_UNKNOWN,
         }
      }
      static VkResult
   handle_end_query_cpu_job(struct v3dv_job *job, uint32_t counter_pass_idx)
   {
                        struct v3dv_end_query_info *info = &job->cpu.query_end;
            int err = 0;
            assert(info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
            if (info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
               if (result != VK_SUCCESS)
                        for (uint32_t i = 0; i < info->count; i++) {
      assert(info->query + i < info->pool->query_count);
            if (info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR) {
      uint32_t syncobj = vk_sync_as_drm_syncobj(query->perf.last_job_sync)->syncobj;
                  if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
                                 fail:
      if (info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR)
            cnd_broadcast(&job->device->query_ended);
               }
      static VkResult
   handle_copy_query_results_cpu_job(struct v3dv_job *job)
   {
      struct v3dv_copy_query_results_cpu_job_info *info =
            assert(info->pool->query_type == VK_QUERY_TYPE_PERFORMANCE_QUERY_KHR ||
            assert(info->dst && info->dst->mem && info->dst->mem->bo);
            /* Map the entire dst buffer for the CPU copy if needed */
   assert(!bo->map || bo->map_size == bo->size);
   if (!bo->map && !v3dv_bo_map(job->device, bo, bo->size))
            uint8_t *offset = ((uint8_t *) bo->map) +
         v3dv_get_query_pool_results_cpu(job->device,
                                             }
      static VkResult
   handle_timestamp_query_cpu_job(struct v3dv_queue *queue, struct v3dv_job *job,
         {
      assert(job->type == V3DV_JOB_TYPE_CPU_TIMESTAMP_QUERY);
            /* Wait for completion of all work queued before the timestamp query */
   VkResult result = queue_wait_idle(queue, sync_info);
   if (result != VK_SUCCESS)
                     /* Compute timestamp */
   struct timespec t;
            for (uint32_t i = 0; i < info->count; i++) {
      assert(info->query + i < info->pool->query_count);
   struct v3dv_query *query = &info->pool->queries[info->query + i];
   query->maybe_available = true;
   if (i == 0)
               cnd_broadcast(&job->device->query_ended);
               }
      static VkResult
   handle_csd_indirect_cpu_job(struct v3dv_queue *queue,
               {
      assert(job->type == V3DV_JOB_TYPE_CPU_CSD_INDIRECT);
   struct v3dv_csd_indirect_cpu_job_info *info = &job->cpu.csd_indirect;
            /* Make sure the GPU is no longer using the indirect buffer*/
   assert(info->buffer && info->buffer->mem && info->buffer->mem->bo);
            /* Map the indirect buffer and read the dispatch parameters */
   assert(info->buffer && info->buffer->mem && info->buffer->mem->bo);
   struct v3dv_bo *bo = info->buffer->mem->bo;
   if (!bo->map && !v3dv_bo_map(job->device, bo, bo->size))
                  const uint32_t offset = info->buffer->mem_offset + info->offset;
   const uint32_t *group_counts = (uint32_t *) (bo->map + offset);
   if (group_counts[0] == 0 || group_counts[1] == 0|| group_counts[2] == 0)
            if (memcmp(group_counts, info->csd_job->csd.wg_count,
                           }
      /**
   * This handles semaphore waits for the single sync path by accumulating
   * wait semaphores into the QUEUE_ANY syncobj. Notice this is only required
   * to ensure we accumulate any *external* semaphores (since for anything else
   * we are already accumulating out syncs with each submission to the kernel).
   */
   static VkResult
   process_singlesync_waits(struct v3dv_queue *queue,
         {
      struct v3dv_device *device = queue->device;
            if (count == 0)
                     int err = 0;
   int fd = -1;
   err = drmSyncobjExportSyncFile(device->pdevice->render_fd,
               if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
                     for (uint32_t i = 0; i < count; i++) {
      uint32_t syncobj = vk_sync_as_drm_syncobj(waits[i].sync)->syncobj;
            err = drmSyncobjExportSyncFile(device->pdevice->render_fd,
         if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
                     err = sync_accumulate("v3dv", &fd, wait_fd);
   close(wait_fd);
   if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
                        err = drmSyncobjImportSyncFile(device->pdevice->render_fd,
               if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
            fail:
      close(fd);
      }
      /**
   * This handles signaling for the single-sync path by importing the QUEUE_ANY
   * syncobj into all syncs to be signaled.
   */
   static VkResult
   process_singlesync_signals(struct v3dv_queue *queue,
         {
      struct v3dv_device *device = queue->device;
            if (device->pdevice->caps.multisync)
            int fd = -1;
   drmSyncobjExportSyncFile(device->pdevice->render_fd,
               if (fd == -1) {
      return vk_errorf(queue, VK_ERROR_UNKNOWN,
               VkResult result = VK_SUCCESS;
   for (uint32_t i = 0; i < count; i++) {
      uint32_t syncobj = vk_sync_as_drm_syncobj(signals[i].sync)->syncobj;
   int err = drmSyncobjImportSyncFile(device->pdevice->render_fd,
         if (err) {
      result = vk_errorf(queue, VK_ERROR_UNKNOWN,
                        assert(fd >= 0);
               }
      static void
   multisync_free(struct v3dv_device *device,
         {
      vk_free(&device->vk.alloc, (void *)(uintptr_t)ms->out_syncs);
      }
      static struct drm_v3d_sem *
   set_in_syncs(struct v3dv_queue *queue,
               struct v3dv_job *job,
   enum v3dv_queue_type queue_sync,
   {
      struct v3dv_device *device = queue->device;
            /* If this is the first job submitted to a given GPU queue in this cmd buf
   * batch, it has to wait on wait semaphores (if any) before running.
   */
   if (queue->last_job_syncs.first[queue_sync])
            /* If the serialize flag is set the job needs to be serialized in the
   * corresponding queues. Notice that we may implement transfer operations
   * as both CL or TFU jobs.
   *
   * FIXME: maybe we could track more precisely if the source of a transfer
   * barrier is a CL and/or a TFU job.
   */
   bool sync_csd  = job->serialize & V3DV_BARRIER_COMPUTE_BIT;
   bool sync_tfu  = job->serialize & V3DV_BARRIER_TRANSFER_BIT;
   bool sync_cl   = job->serialize & (V3DV_BARRIER_GRAPHICS_BIT |
         *count = n_syncs;
   if (sync_cl)
         if (sync_tfu)
         if (sync_csd)
            if (!*count)
            struct drm_v3d_sem *syncs =
      vk_zalloc(&device->vk.alloc, *count * sizeof(struct drm_v3d_sem),
         if (!syncs)
            for (int i = 0; i < n_syncs; i++) {
      syncs[i].handle =
               if (sync_cl)
            if (sync_csd)
            if (sync_tfu)
            assert(n_syncs == *count);
      }
      static struct drm_v3d_sem *
   set_out_syncs(struct v3dv_queue *queue,
               struct v3dv_job *job,
   enum v3dv_queue_type queue_sync,
   uint32_t *count,
   {
                        /* We always signal the syncobj from `device->last_job_syncs` related to
   * this v3dv_queue_type to track the last job submitted to this queue.
   */
            struct drm_v3d_sem *syncs =
      vk_zalloc(&device->vk.alloc, *count * sizeof(struct drm_v3d_sem),
         if (!syncs)
            if (n_vk_syncs) {
      for (unsigned i = 0; i < n_vk_syncs; i++) {
      syncs[i].handle =
                              }
      static void
   set_ext(struct drm_v3d_extension *ext,
   struct drm_v3d_extension *next,
   uint32_t id,
   uintptr_t flags)
   {
      ext->next = (uintptr_t)(void *)next;
   ext->id = id;
      }
      /* This function sets the extension for multiple in/out syncobjs. When it is
   * successful, it sets the extension id to DRM_V3D_EXT_ID_MULTI_SYNC.
   * Otherwise, the extension id is 0, which means an out-of-memory error.
   */
   static void
   set_multisync(struct drm_v3d_multi_sync *ms,
               struct v3dv_submit_sync_info *sync_info,
   struct drm_v3d_extension *next,
   struct v3dv_device *device,
   struct v3dv_job *job,
   enum v3dv_queue_type queue_sync,
   {
      struct v3dv_queue *queue = &device->queue;
   uint32_t out_sync_count = 0, in_sync_count = 0;
            in_syncs = set_in_syncs(queue, job, queue_sync,
         if (!in_syncs && in_sync_count)
            out_syncs = set_out_syncs(queue, job, queue_sync,
                     if (!out_syncs)
            set_ext(&ms->base, next, DRM_V3D_EXT_ID_MULTI_SYNC, 0);
   ms->wait_stage = wait_stage;
   ms->out_sync_count = out_sync_count;
   ms->out_syncs = (uintptr_t)(void *)out_syncs;
   ms->in_sync_count = in_sync_count;
                  fail:
      if (in_syncs)
                     }
      /* This must be called after every submission in the single-sync path to
   * accumulate the out_sync into the QUEUE_ANY sync so we can serialize
   * jobs by waiting on the QUEUE_ANY sync.
   */
   static int
   update_any_queue_sync(struct v3dv_queue *queue, uint32_t out_sync)
   {
      struct v3dv_device *device = queue->device;
            int render_fd = device->pdevice->render_fd;
   int fd_any = -1, fd_out_sync = -1;
   int err;
   err  = drmSyncobjExportSyncFile(render_fd,
               if (err)
            err = drmSyncobjExportSyncFile(render_fd, out_sync, &fd_out_sync);
   if (err)
            err = sync_accumulate("v3dv", &fd_any, fd_out_sync);
   if (err)
            err = drmSyncobjImportSyncFile(render_fd,
               fail:
      close(fd_any);
   close(fd_out_sync);
      }
      static VkResult
   handle_cl_job(struct v3dv_queue *queue,
               struct v3dv_job *job,
   uint32_t counter_pass_idx,
   {
                        /* Sanity check: we should only flag a bcl sync on a job that needs to be
   * serialized.
   */
            /* We expect to have just one RCL per job which should fit in just one BO.
   * Our BCL, could chain multiple BOS together though.
   */
   assert(list_length(&job->rcl.bo_list) == 1);
   assert(list_length(&job->bcl.bo_list) >= 1);
   struct v3dv_bo *bcl_fist_bo =
         submit.bcl_start = bcl_fist_bo->offset;
   submit.bcl_end = job->bcl.bo->offset + v3dv_cl_offset(&job->bcl);
   submit.rcl_start = job->rcl.bo->offset;
            submit.qma = job->tile_alloc->offset;
   submit.qms = job->tile_alloc->size;
            submit.flags = 0;
   if (job->tmu_dirty_rcl)
            /* If the job uses VK_KHR_buffer_device_address we need to ensure all
   * buffers flagged with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR
   * are included.
   */
   if (job->uses_buffer_device_address) {
      util_dynarray_foreach(&queue->device->device_address_bo_list,
                           submit.bo_handle_count = job->bo_count;
   uint32_t *bo_handles =
         uint32_t bo_idx = 0;
   set_foreach(job->bos, entry) {
      struct v3dv_bo *bo = (struct v3dv_bo *)entry->key;
      }
   assert(bo_idx == submit.bo_handle_count);
            submit.perfmon_id = job->perf ?
         const bool needs_perf_sync = queue->last_perfmon_id != submit.perfmon_id;
            /* We need a binning sync if we are the first CL job waiting on a semaphore
   * with a wait stage that involves the geometry pipeline, or if the job
   * comes after a pipeline barrier that involves geometry stages
   * (needs_bcl_sync) or when performance queries are in use.
   *
   * We need a render sync if the job doesn't need a binning sync but has
   * still been flagged for serialization. It should be noted that RCL jobs
   * don't start until the previous RCL job has finished so we don't really
   * need to add a fence for those, however, we might need to wait on a CSD or
   * TFU job, which are not automatically serialized with CL jobs.
   */
   bool needs_bcl_sync = job->needs_bcl_sync || needs_perf_sync;
   if (queue->last_job_syncs.first[V3DV_QUEUE_CL]) {
      for (int i = 0; !needs_bcl_sync && i < sync_info->wait_count; i++) {
      needs_bcl_sync = sync_info->waits[i].stage_mask &
      (VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT |
   VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT |
   VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT |
   VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT |
   VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT |
   VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT |
   VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT |
   VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
   VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
   VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
   VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
                        /* Replace single semaphore settings whenever our kernel-driver supports
   * multiple semaphores extension.
   */
   struct drm_v3d_multi_sync ms = { 0 };
   if (device->pdevice->caps.multisync) {
      enum v3d_queue wait_stage = needs_rcl_sync ? V3D_RENDER : V3D_BIN;
   set_multisync(&ms, sync_info, NULL, device, job,
         if (!ms.base.id)
            submit.flags |= DRM_V3D_SUBMIT_EXTENSION;
   submit.extensions = (uintptr_t)(void *)&ms;
   /* Disable legacy sync interface when multisync extension is used */
   submit.in_sync_rcl = 0;
   submit.in_sync_bcl = 0;
      } else {
      uint32_t last_job_sync = queue->last_job_syncs.syncs[V3DV_QUEUE_ANY];
   submit.in_sync_bcl = needs_bcl_sync ? last_job_sync : 0;
   submit.in_sync_rcl = needs_rcl_sync ? last_job_sync : 0;
               v3dv_clif_dump(device, job, &submit);
   int ret = v3dv_ioctl(device->pdevice->render_fd,
            static bool warned = false;
   if (ret && !warned) {
      fprintf(stderr, "Draw call returned %s. Expect corruption.\n",
                     if (!device->pdevice->caps.multisync && ret == 0)
            free(bo_handles);
                     if (ret)
               }
      static VkResult
   handle_tfu_job(struct v3dv_queue *queue,
                     {
                                 /* Replace single semaphore settings whenever our kernel-driver supports
   * multiple semaphore extension.
   */
   struct drm_v3d_multi_sync ms = { 0 };
   if (device->pdevice->caps.multisync) {
      set_multisync(&ms, sync_info, NULL, device, job,
         if (!ms.base.id)
            job->tfu.flags |= DRM_V3D_SUBMIT_EXTENSION;
   job->tfu.extensions = (uintptr_t)(void *)&ms;
   /* Disable legacy sync interface when multisync extension is used */
   job->tfu.in_sync = 0;
      } else {
      uint32_t last_job_sync = queue->last_job_syncs.syncs[V3DV_QUEUE_ANY];
   job->tfu.in_sync = needs_sync ? last_job_sync : 0;
      }
   int ret = v3dv_ioctl(device->pdevice->render_fd,
            if (!device->pdevice->caps.multisync && ret == 0)
            multisync_free(device, &ms);
            if (ret != 0)
               }
      static VkResult
   handle_csd_job(struct v3dv_queue *queue,
                  struct v3dv_job *job,
      {
                        /* If the job uses VK_KHR_buffer_device_address we need to ensure all
   * buffers flagged with VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR
   * are included.
   */
   if (job->uses_buffer_device_address) {
      util_dynarray_foreach(&queue->device->device_address_bo_list,
                           submit->bo_handle_count = job->bo_count;
   uint32_t *bo_handles =
         uint32_t bo_idx = 0;
   set_foreach(job->bos, entry) {
      struct v3dv_bo *bo = (struct v3dv_bo *)entry->key;
      }
   assert(bo_idx == submit->bo_handle_count);
                     /* Replace single semaphore settings whenever our kernel-driver supports
   * multiple semaphore extension.
   */
   struct drm_v3d_multi_sync ms = { 0 };
   if (device->pdevice->caps.multisync) {
      set_multisync(&ms, sync_info, NULL, device, job,
         if (!ms.base.id)
            submit->flags |= DRM_V3D_SUBMIT_EXTENSION;
   submit->extensions = (uintptr_t)(void *)&ms;
   /* Disable legacy sync interface when multisync extension is used */
   submit->in_sync = 0;
      } else {
      uint32_t last_job_sync = queue->last_job_syncs.syncs[V3DV_QUEUE_ANY];
   submit->in_sync = needs_sync ? last_job_sync : 0;
      }
   submit->perfmon_id = job->perf ?
         queue->last_perfmon_id = submit->perfmon_id;
   int ret = v3dv_ioctl(device->pdevice->render_fd,
            static bool warned = false;
   if (ret && !warned) {
      fprintf(stderr, "Compute dispatch returned %s. Expect corruption.\n",
                     if (!device->pdevice->caps.multisync && ret == 0)
                     multisync_free(device, &ms);
            if (ret)
               }
      static VkResult
   queue_handle_job(struct v3dv_queue *queue,
                  struct v3dv_job *job,
      {
      switch (job->type) {
   case V3DV_JOB_TYPE_GPU_CL:
         case V3DV_JOB_TYPE_GPU_TFU:
         case V3DV_JOB_TYPE_GPU_CSD:
         case V3DV_JOB_TYPE_CPU_RESET_QUERIES:
         case V3DV_JOB_TYPE_CPU_END_QUERY:
         case V3DV_JOB_TYPE_CPU_COPY_QUERY_RESULTS:
         case V3DV_JOB_TYPE_CPU_CSD_INDIRECT:
         case V3DV_JOB_TYPE_CPU_TIMESTAMP_QUERY:
         default:
            }
      static VkResult
   queue_create_noop_job(struct v3dv_queue *queue)
   {
      struct v3dv_device *device = queue->device;
   queue->noop_job = vk_zalloc(&device->vk.alloc, sizeof(struct v3dv_job), 8,
         if (!queue->noop_job)
                           /* We use no-op jobs to signal semaphores/fences. These jobs needs to be
   * serialized across all hw queues to comply with Vulkan's signal operation
   * order requirements, which basically require that signal operations occur
   * in submission order.
   */
               }
      static VkResult
   queue_submit_noop_job(struct v3dv_queue *queue,
                     {
      if (!queue->noop_job) {
      VkResult result = queue_create_noop_job(queue);
   if (result != VK_SUCCESS)
               assert(queue->noop_job);
   return queue_handle_job(queue, queue->noop_job, counter_pass_idx,
      }
      VkResult
   v3dv_queue_driver_submit(struct vk_queue *vk_queue,
         {
      struct v3dv_queue *queue = container_of(vk_queue, struct v3dv_queue, vk);
            struct v3dv_submit_sync_info sync_info = {
      .wait_count = submit->wait_count,
   .waits = submit->waits,
   .signal_count = submit->signal_count,
               for (int i = 0; i < V3DV_QUEUE_COUNT; i++)
            /* If we do not have multisync we need to ensure we accumulate any wait
   * semaphores into our QUEUE_ANY syncobj so we can handle waiting on
   * external semaphores.
   */
   if (!queue->device->pdevice->caps.multisync) {
      result =
         if (result != VK_SUCCESS)
               for (uint32_t i = 0; i < submit->command_buffer_count; i++) {
      struct v3dv_cmd_buffer *cmd_buffer =
         list_for_each_entry_safe(struct v3dv_job, job,
               result = queue_handle_job(queue, job, submit->perf_pass_index,
         if (result != VK_SUCCESS)
               /* If the command buffer ends with a barrier we need to consume it now.
   *
   * FIXME: this will drain all hw queues. Instead, we could use the pending
   * barrier state to limit the queues we serialize against.
   */
   if (cmd_buffer->state.barrier.dst_mask) {
      result = queue_submit_noop_job(queue, submit->perf_pass_index,
         if (result != VK_SUCCESS)
                  /* Handle signaling now */
   if (submit->signal_count > 0) {
      if (queue->device->pdevice->caps.multisync) {
      /* Finish by submitting a no-op job that synchronizes across all queues.
   * This will ensure that the signal semaphores don't get triggered until
   * all work on any queue completes. See Vulkan's signal operation order
   * requirements.
   */
   return queue_submit_noop_job(queue, submit->perf_pass_index,
      } else {
      return process_singlesync_signals(queue, sync_info.signal_count,
                     }
      VKAPI_ATTR VkResult VKAPI_CALL
   v3dv_QueueBindSparse(VkQueue _queue,
                     {
      V3DV_FROM_HANDLE(v3dv_queue, queue, _queue);
      }
