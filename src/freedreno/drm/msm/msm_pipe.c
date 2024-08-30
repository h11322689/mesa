   /*
   * Copyright (C) 2012-2018 Rob Clark <robclark@freedesktop.org>
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
      #include "util/slab.h"
      #include "freedreno_ringbuffer_sp.h"
   #include "msm_priv.h"
      static int
   query_param(struct fd_pipe *pipe, uint32_t param, uint64_t *value)
   {
      struct msm_pipe *msm_pipe = to_msm_pipe(pipe);
   struct drm_msm_param req = {
      .pipe = msm_pipe->pipe,
      };
            ret =
         if (ret)
                        }
      static int
   query_queue_param(struct fd_pipe *pipe, uint32_t param, uint64_t *value)
   {
      struct msm_pipe *msm_pipe = to_msm_pipe(pipe);
   struct drm_msm_submitqueue_query req = {
      .data = VOID2U64(value),
   .id = msm_pipe->queue_id,
   .param = param,
      };
            ret = drmCommandWriteRead(pipe->dev->fd, DRM_MSM_SUBMITQUEUE_QUERY, &req,
         if (ret)
               }
      static int
   msm_pipe_get_param(struct fd_pipe *pipe, enum fd_param_id param,
         {
      struct msm_pipe *msm_pipe = to_msm_pipe(pipe);
   switch (param) {
   case FD_DEVICE_ID: // XXX probably get rid of this..
   case FD_GPU_ID:
      *value = msm_pipe->gpu_id;
      case FD_GMEM_SIZE:
      *value = msm_pipe->gmem;
      case FD_GMEM_BASE:
      *value = msm_pipe->gmem_base;
      case FD_CHIP_ID:
      *value = msm_pipe->chip_id;
      case FD_MAX_FREQ:
         case FD_TIMESTAMP:
         case FD_NR_PRIORITIES:
         case FD_CTX_FAULTS:
         case FD_GLOBAL_FAULTS:
         case FD_SUSPEND_COUNT:
         case FD_VA_SIZE:
         default:
      ERROR_MSG("invalid param id: %d", param);
         }
      static int
   set_param(struct fd_pipe *pipe, uint32_t param, uint64_t value)
   {
      struct msm_pipe *msm_pipe = to_msm_pipe(pipe);
   struct drm_msm_param req = {
      .pipe  = msm_pipe->pipe,
   .param = param,
               return drmCommandWriteRead(pipe->dev->fd, DRM_MSM_SET_PARAM,
      }
      static int
   msm_pipe_set_param(struct fd_pipe *pipe, enum fd_param_id param, uint64_t value)
   {
      switch (param) {
   case FD_SYSPROF:
         default:
      ERROR_MSG("invalid param id: %d", param);
         }
      static int
   msm_pipe_wait(struct fd_pipe *pipe, const struct fd_fence *fence, uint64_t timeout)
   {
      struct fd_device *dev = pipe->dev;
   struct drm_msm_wait_fence req = {
      .fence = fence->kfence,
      };
                     ret = drmCommandWrite(dev->fd, DRM_MSM_WAIT_FENCE, &req, sizeof(req));
   if (ret && (ret != -ETIMEDOUT)) {
                     }
      static int
   open_submitqueue(struct fd_pipe *pipe, uint32_t prio)
   {
      struct drm_msm_submitqueue req = {
      .flags = 0,
      };
   uint64_t nr_prio = 1;
            if (fd_device_version(pipe->dev) < FD_VERSION_SUBMIT_QUEUES) {
      to_msm_pipe(pipe)->queue_id = 0;
                                 ret = drmCommandWriteRead(pipe->dev->fd, DRM_MSM_SUBMITQUEUE_NEW, &req,
         if (ret) {
      ERROR_MSG("could not create submitqueue! %d (%s)", ret, strerror(errno));
               to_msm_pipe(pipe)->queue_id = req.id;
      }
      static void
   close_submitqueue(struct fd_pipe *pipe, uint32_t queue_id)
   {
      if (fd_device_version(pipe->dev) < FD_VERSION_SUBMIT_QUEUES)
            drmCommandWrite(pipe->dev->fd, DRM_MSM_SUBMITQUEUE_CLOSE, &queue_id,
      }
      static void
   msm_pipe_destroy(struct fd_pipe *pipe)
   {
               close_submitqueue(pipe, msm_pipe->queue_id);
   fd_pipe_sp_ringpool_fini(pipe);
      }
      static const struct fd_pipe_funcs sp_funcs = {
      .ringbuffer_new_object = fd_ringbuffer_sp_new_object,
   .submit_new = msm_submit_sp_new,
   .flush = fd_pipe_sp_flush,
   .get_param = msm_pipe_get_param,
   .set_param = msm_pipe_set_param,
   .wait = msm_pipe_wait,
      };
      static const struct fd_pipe_funcs legacy_funcs = {
      .ringbuffer_new_object = msm_ringbuffer_new_object,
   .submit_new = msm_submit_new,
   .get_param = msm_pipe_get_param,
   .set_param = msm_pipe_set_param,
   .wait = msm_pipe_wait,
      };
      static uint64_t
   get_param(struct fd_pipe *pipe, uint32_t param)
   {
      uint64_t value;
   int ret = query_param(pipe, param, &value);
   if (ret) {
      ERROR_MSG("get-param failed! %d (%s)", ret, strerror(errno));
      }
      }
      struct fd_pipe *
   msm_pipe_new(struct fd_device *dev, enum fd_pipe_id id, uint32_t prio)
   {
      static const uint32_t pipe_id[] = {
      [FD_PIPE_3D] = MSM_PIPE_3D0,
      };
   struct msm_pipe *msm_pipe = NULL;
            msm_pipe = calloc(1, sizeof(*msm_pipe));
   if (!msm_pipe) {
      ERROR_MSG("allocation failed");
                        if (fd_device_version(dev) >= FD_VERSION_SOFTPIN) {
         } else {
                  /* initialize before get_param(): */
   pipe->dev = dev;
            /* these params should be supported since the first version of drm/msm: */
   msm_pipe->gpu_id = get_param(pipe, MSM_PARAM_GPU_ID);
   msm_pipe->gmem = get_param(pipe, MSM_PARAM_GMEM_SIZE);
            if (fd_device_version(pipe->dev) >= FD_VERSION_GMEM_BASE)
            if (!(msm_pipe->gpu_id || msm_pipe->chip_id))
            INFO_MSG("Pipe Info:");
   INFO_MSG(" GPU-id:          %d", msm_pipe->gpu_id);
   INFO_MSG(" Chip-id:         0x%016"PRIx64, msm_pipe->chip_id);
            if (open_submitqueue(pipe, prio))
                        fail:
      if (pipe)
            }
