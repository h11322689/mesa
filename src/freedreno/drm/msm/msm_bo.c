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
      #include "msm_priv.h"
      static int
   bo_allocate(struct msm_bo *msm_bo)
   {
      struct fd_bo *bo = &msm_bo->base;
   if (!msm_bo->offset) {
      struct drm_msm_gem_info req = {
      .handle = bo->handle,
      };
            /* if the buffer is already backed by pages then this
   * doesn't actually do anything (other than giving us
   * the offset)
   */
   ret =
         if (ret) {
      ERROR_MSG("alloc failed: %s", strerror(errno));
                              }
      static int
   msm_bo_offset(struct fd_bo *bo, uint64_t *offset)
   {
      struct msm_bo *msm_bo = to_msm_bo(bo);
   int ret = bo_allocate(msm_bo);
   if (ret)
         *offset = msm_bo->offset;
      }
      static int
   msm_bo_cpu_prep(struct fd_bo *bo, struct fd_pipe *pipe, uint32_t op)
   {
      struct drm_msm_gem_cpu_prep req = {
      .handle = bo->handle,
                           }
      static int
   msm_bo_madvise(struct fd_bo *bo, int willneed)
   {
      struct drm_msm_gem_madvise req = {
      .handle = bo->handle,
      };
            /* older kernels do not support this: */
   if (bo->dev->version < FD_VERSION_MADVISE)
            ret =
         if (ret)
               }
      static uint64_t
   msm_bo_iova(struct fd_bo *bo)
   {
      struct drm_msm_gem_info req = {
      .handle = bo->handle,
      };
            ret = drmCommandWriteRead(bo->dev->fd, DRM_MSM_GEM_INFO, &req, sizeof(req));
   if (ret)
               }
      static void
   msm_bo_set_name(struct fd_bo *bo, const char *fmt, va_list ap)
   {
      struct drm_msm_gem_info req = {
      .handle = bo->handle,
      };
   char buf[32];
            if (bo->dev->version < FD_VERSION_SOFTPIN)
                     req.value = VOID2U64(buf);
               }
      static const struct fd_bo_funcs funcs = {
      .offset = msm_bo_offset,
   .cpu_prep = msm_bo_cpu_prep,
   .madvise = msm_bo_madvise,
   .iova = msm_bo_iova,
   .set_name = msm_bo_set_name,
      };
      /* allocate a buffer handle: */
   static int
   new_handle(struct fd_device *dev, uint32_t size, uint32_t flags, uint32_t *handle)
   {
      struct drm_msm_gem_new req = {
         };
            if (flags & FD_BO_SCANOUT)
            if (flags & FD_BO_GPUREADONLY)
            if (flags & FD_BO_CACHED_COHERENT)
         else
            ret = drmCommandWriteRead(dev->fd, DRM_MSM_GEM_NEW, &req, sizeof(req));
   if (ret)
                        }
      /* allocate a new buffer object */
   struct fd_bo *
   msm_bo_new(struct fd_device *dev, uint32_t size, uint32_t flags)
   {
      uint32_t handle;
            ret = new_handle(dev, size, flags, &handle);
   if (ret)
               }
      /* allocate a new buffer object from existing handle (import) */
   struct fd_bo *
   msm_bo_from_handle(struct fd_device *dev, uint32_t size, uint32_t handle)
   {
      struct msm_bo *msm_bo;
            msm_bo = calloc(1, sizeof(*msm_bo));
   if (!msm_bo)
            bo = &msm_bo->base;
   bo->size = size;
   bo->handle = handle;
                        }
