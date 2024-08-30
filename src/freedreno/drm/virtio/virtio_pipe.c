   /*
   * Copyright © 2022 Google, Inc.
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
      #include "util/libsync.h"
   #include "util/slab.h"
      #include "freedreno_ringbuffer_sp.h"
   #include "virtio_priv.h"
      static int
   query_param(struct fd_pipe *pipe, uint32_t param, uint64_t *value)
   {
      struct virtio_pipe *virtio_pipe = to_virtio_pipe(pipe);
   struct drm_msm_param req = {
      .pipe = virtio_pipe->pipe,
      };
            ret = virtio_simple_ioctl(pipe->dev, DRM_IOCTL_MSM_GET_PARAM, &req);
   if (ret)
                        }
      static int
   query_faults(struct fd_pipe *pipe, uint64_t *value)
   {
      struct virtio_device *virtio_dev = to_virtio_device(pipe->dev);
   uint32_t async_error = 0;
            if (msm_shmem_has_field(virtio_dev->shmem, async_error))
            if (msm_shmem_has_field(virtio_dev->shmem, global_faults)) {
         } else {
      int ret = query_param(pipe, MSM_PARAM_FAULTS, &global_faults);
   if (ret)
                           }
      static int
   virtio_pipe_get_param(struct fd_pipe *pipe, enum fd_param_id param,
         {
      struct virtio_pipe *virtio_pipe = to_virtio_pipe(pipe);
            switch (param) {
   case FD_DEVICE_ID: // XXX probably get rid of this..
   case FD_GPU_ID:
      *value = virtio_pipe->gpu_id;
      case FD_GMEM_SIZE:
      *value = virtio_pipe->gmem;
      case FD_GMEM_BASE:
      *value = virtio_pipe->gmem_base;
      case FD_CHIP_ID:
      *value = virtio_pipe->chip_id;
      case FD_MAX_FREQ:
      *value = virtio_dev->caps.u.msm.max_freq;
      case FD_TIMESTAMP:
         case FD_NR_PRIORITIES:
      *value = virtio_dev->caps.u.msm.priorities;
      case FD_CTX_FAULTS:
   case FD_GLOBAL_FAULTS:
         case FD_SUSPEND_COUNT:
         case FD_VA_SIZE:
      *value = virtio_dev->caps.u.msm.va_size;
      default:
      ERROR_MSG("invalid param id: %d", param);
         }
      static int
   virtio_pipe_wait(struct fd_pipe *pipe, const struct fd_fence *fence, uint64_t timeout)
   {
               struct msm_ccmd_wait_fence_req req = {
         .hdr = MSM_CCMD(WAIT_FENCE, sizeof(req)),
   .queue_id = to_virtio_pipe(pipe)->queue_id,
   };
   struct msm_ccmd_submitqueue_query_rsp *rsp;
   int64_t end_time = os_time_get_nano() + timeout;
            /* Do a non-blocking wait to trigger host-side wait-boost,
   * if the host kernel is new enough
   */
   rsp = virtio_alloc_rsp(pipe->dev, &req.hdr, sizeof(*rsp));
   ret = virtio_execbuf(pipe->dev, &req.hdr, false);
   if (ret)
                     if (fence->use_fence_fd)
            do {
               ret = virtio_execbuf(pipe->dev, &req.hdr, true);
   if (ret)
            if ((timeout != OS_TIMEOUT_INFINITE) &&
                           out:
         }
      static int
   open_submitqueue(struct fd_pipe *pipe, uint32_t prio)
   {
               struct drm_msm_submitqueue req = {
      .flags = 0,
      };
   uint64_t nr_prio = 1;
                              ret = virtio_simple_ioctl(pipe->dev, DRM_IOCTL_MSM_SUBMITQUEUE_NEW, &req);
   if (ret) {
      ERROR_MSG("could not create submitqueue! %d (%s)", ret, strerror(errno));
               virtio_pipe->queue_id = req.id;
               }
      static void
   close_submitqueue(struct fd_pipe *pipe, uint32_t queue_id)
   {
         }
      static void
   virtio_pipe_destroy(struct fd_pipe *pipe)
   {
               if (util_queue_is_initialized(&virtio_pipe->retire_queue))
            close_submitqueue(pipe, virtio_pipe->queue_id);
   fd_pipe_sp_ringpool_fini(pipe);
      }
      static const struct fd_pipe_funcs funcs = {
      .ringbuffer_new_object = fd_ringbuffer_sp_new_object,
   .submit_new = virtio_submit_new,
   .flush = fd_pipe_sp_flush,
   .get_param = virtio_pipe_get_param,
   .wait = virtio_pipe_wait,
      };
      static void
   init_shmem(struct fd_device *dev)
   {
                        /* One would like to do this in virtio_device_new(), but we'd
   * have to bypass/reinvent fd_bo_new()..
   */
   if (unlikely(!virtio_dev->shmem)) {
      virtio_dev->shmem_bo = fd_bo_new(dev, 0x4000,
         virtio_dev->shmem = fd_bo_map(virtio_dev->shmem_bo);
            uint32_t offset = virtio_dev->shmem->rsp_mem_offset;
   virtio_dev->rsp_mem_len = fd_bo_size(virtio_dev->shmem_bo) - offset;
                  }
      struct fd_pipe *
   virtio_pipe_new(struct fd_device *dev, enum fd_pipe_id id, uint32_t prio)
   {
      static const uint32_t pipe_id[] = {
      [FD_PIPE_3D] = MSM_PIPE_3D0,
      };
   struct virtio_device *virtio_dev = to_virtio_device(dev);
   struct virtio_pipe *virtio_pipe = NULL;
                     virtio_pipe = calloc(1, sizeof(*virtio_pipe));
   if (!virtio_pipe) {
      ERROR_MSG("allocation failed");
                                 /* initialize before get_param(): */
   pipe->dev = dev;
            virtio_pipe->gpu_id = virtio_dev->caps.u.msm.gpu_id;
   virtio_pipe->gmem = virtio_dev->caps.u.msm.gmem_size;
   virtio_pipe->gmem_base = virtio_dev->caps.u.msm.gmem_base;
               if (!(virtio_pipe->gpu_id || virtio_pipe->chip_id))
            util_queue_init(&virtio_pipe->retire_queue, "rq", 8, 1,
            INFO_MSG("Pipe Info:");
   INFO_MSG(" GPU-id:          %d", virtio_pipe->gpu_id);
   INFO_MSG(" Chip-id:         0x%016"PRIx64, virtio_pipe->chip_id);
            if (open_submitqueue(pipe, prio))
                        fail:
      if (pipe)
            }
