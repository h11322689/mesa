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
      #include "virtio_priv.h"
      static int
   bo_allocate(struct virtio_bo *virtio_bo)
   {
      struct fd_bo *bo = &virtio_bo->base;
   if (!virtio_bo->offset) {
      struct drm_virtgpu_map req = {
         };
            ret = virtio_ioctl(bo->dev->fd, VIRTGPU_MAP, &req);
   if (ret) {
      ERROR_MSG("alloc failed: %s", strerror(errno));
                              }
      static int
   virtio_bo_offset(struct fd_bo *bo, uint64_t *offset)
   {
      struct virtio_bo *virtio_bo = to_virtio_bo(bo);
            if (ret)
            /* If we have uploaded, we need to wait for host to handle that
   * before we can allow guest-side CPU access:
   */
   if (virtio_bo->has_upload_seqno) {
      virtio_bo->has_upload_seqno = false;
   virtio_execbuf_flush(bo->dev);
   virtio_host_sync(bo->dev, &(struct msm_ccmd_req) {
                                 }
      static int
   virtio_bo_cpu_prep_guest(struct fd_bo *bo)
   {
      struct drm_virtgpu_3d_wait args = {
         };
            /* Side note, this ioctl is defined as IO_WR but should be IO_W: */
   ret = virtio_ioctl(bo->dev->fd, VIRTGPU_WAIT, &args);
   if (ret && errno == EBUSY)
               }
      static int
   virtio_bo_cpu_prep(struct fd_bo *bo, struct fd_pipe *pipe, uint32_t op)
   {
      MESA_TRACE_FUNC();
            /*
   * Wait first in the guest, to avoid a blocking call in host.
   * If implicit sync it used, we still need to *also* wait in
   * host, if it is a shared buffer, because the guest doesn't
   * know about usage of the bo in the host (or other guests).
            ret = virtio_bo_cpu_prep_guest(bo);
   if (ret)
            /*
   * The buffer could be shared with other things on the host side
   * so have to poll the host.  But we only get here with the shared
   * buffers plus implicit sync.  Hopefully that is rare enough.
            struct msm_ccmd_gem_cpu_prep_req req = {
         .hdr = MSM_CCMD(GEM_CPU_PREP, sizeof(req)),
   .res_id = to_virtio_bo(bo)->res_id,
   };
            /* We can't do a blocking wait in the host, so we have to poll: */
   do {
               ret = virtio_execbuf(bo->dev, &req.hdr, true);
   if (ret)
                     out:
         }
      static int
   virtio_bo_madvise(struct fd_bo *bo, int willneed)
   {
      /* TODO:
   * Currently unsupported, synchronous WILLNEED calls would introduce too
   * much latency.. ideally we'd keep state in the guest and only flush
   * down to host when host is under memory pressure.  (Perhaps virtio-balloon
   * could signal this?)
   */
      }
      static uint64_t
   virtio_bo_iova(struct fd_bo *bo)
   {
      /* The shmem bo is allowed to have no iova, as it is only used for
   * guest<->host communications:
   */
   assert(bo->iova || (to_virtio_bo(bo)->blob_id == 0));
      }
      static void
   virtio_bo_set_name(struct fd_bo *bo, const char *fmt, va_list ap)
   {
      char name[32];
            /* Note, we cannot set name on the host for the shmem bo, as
   * that isn't a real gem obj on the host side.. not having
   * an iova is a convenient way to detect this case:
   */
   if (!bo->iova)
            sz = vsnprintf(name, sizeof(name), fmt, ap);
                     uint8_t buf[req_len];
            req->hdr = MSM_CCMD(GEM_SET_NAME, req_len);
   req->res_id = to_virtio_bo(bo)->res_id;
                        }
      static void
   bo_upload(struct fd_bo *bo, unsigned off, void *src, unsigned len)
   {
      MESA_TRACE_FUNC();
   unsigned req_len = sizeof(struct msm_ccmd_gem_upload_req) + align(len, 4);
            uint8_t buf[req_len];
            req->hdr = MSM_CCMD(GEM_UPLOAD, req_len);
   req->res_id = virtio_bo->res_id;
   req->pad = 0;
   req->off = off;
                              virtio_bo->upload_seqno = req->hdr.seqno;
      }
      static void
   virtio_bo_upload(struct fd_bo *bo, void *src, unsigned off, unsigned len)
   {
      while (len > 0) {
      unsigned sz = MIN2(len, 0x1000);
   bo_upload(bo, off, src, sz);
   off += sz;
   src += sz;
         }
      /**
   * For recently allocated buffers, an immediate mmap would stall waiting
   * for the host to handle the allocation and map to the guest, which
   * could take a few ms.  So for small transfers to recently allocated
   * buffers, we'd prefer to use the upload path instead.
   */
   static bool
   virtio_bo_prefer_upload(struct fd_bo *bo, unsigned len)
   {
               /* If we've already taken the hit of mmap'ing the buffer, then no reason
   * to take the upload path:
   */
   if (bo->map)
            if (len > 0x4000)
            int64_t age_ns = os_time_get_nano() - virtio_bo->alloc_time_ns;
   if (age_ns > 5000000)
               }
      static void
   set_iova(struct fd_bo *bo, uint64_t iova)
   {
      struct msm_ccmd_gem_set_iova_req req = {
         .hdr = MSM_CCMD(GEM_SET_IOVA, sizeof(req)),
   .res_id = to_virtio_bo(bo)->res_id,
               }
      static void
   virtio_bo_finalize(struct fd_bo *bo)
   {
      /* Release iova by setting to zero: */
   if (bo->iova) {
                     }
      static const struct fd_bo_funcs funcs = {
      .offset = virtio_bo_offset,
   .cpu_prep = virtio_bo_cpu_prep,
   .madvise = virtio_bo_madvise,
   .iova = virtio_bo_iova,
   .set_name = virtio_bo_set_name,
   .upload = virtio_bo_upload,
   .prefer_upload = virtio_bo_prefer_upload,
   .finalize = virtio_bo_finalize,
      };
      static struct fd_bo *
   bo_from_handle(struct fd_device *dev, uint32_t size, uint32_t handle)
   {
      struct virtio_bo *virtio_bo;
            virtio_bo = calloc(1, sizeof(*virtio_bo));
   if (!virtio_bo)
                              /* Note we need to set these because allocation_wait_execute() could
   * run before bo_init_commont():
   */
   bo->dev = dev;
            bo->size = size;
   bo->funcs = &funcs;
            /* Don't assume we can mmap an imported bo: */
            struct drm_virtgpu_resource_info args = {
         };
            ret = virtio_ioctl(dev->fd, VIRTGPU_RESOURCE_INFO, &args);
   if (ret) {
      INFO_MSG("failed to get resource info: %s", strerror(errno));
   free(virtio_bo);
                                    }
      /* allocate a new buffer object from existing handle */
   struct fd_bo *
   virtio_bo_from_handle(struct fd_device *dev, uint32_t size, uint32_t handle)
   {
               if (!bo)
            bo->iova = virtio_dev_alloc_iova(dev, size);
   if (!bo->iova)
                           fail:
      virtio_bo_finalize(bo);
   fd_bo_fini_common(bo);
      }
      /* allocate a buffer object: */
   struct fd_bo *
   virtio_bo_new(struct fd_device *dev, uint32_t size, uint32_t flags)
   {
      struct virtio_device *virtio_dev = to_virtio_device(dev);
   struct drm_virtgpu_resource_create_blob args = {
         .blob_mem   = VIRTGPU_BLOB_MEM_HOST3D,
   };
   struct msm_ccmd_gem_new_req req = {
         .hdr = MSM_CCMD(GEM_NEW, sizeof(req)),
   };
            if (flags & FD_BO_SCANOUT)
            if (flags & FD_BO_GPUREADONLY)
            if (flags & FD_BO_CACHED_COHERENT) {
         } else {
                  if (flags & _FD_BO_VIRTIO_SHM) {
      args.blob_id = 0;
      } else {
      if (flags & (FD_BO_SHARED | FD_BO_SCANOUT)) {
      args.blob_flags = VIRTGPU_BLOB_FLAG_USE_CROSS_DEVICE |
               if (!(flags & FD_BO_NOMAP)) {
                  args.blob_id = p_atomic_inc_return(&virtio_dev->next_blob_id);
   args.cmd = VOID2U64(&req);
            /* tunneled cmds are processed separately on host side,
   * before the renderer->get_blob() callback.. the blob_id
   * is used to like the created bo to the get_blob() call
   */
   req.blob_id = args.blob_id;
   req.iova = virtio_dev_alloc_iova(dev, size);
   if (!req.iova) {
      ret = -ENOMEM;
                  simple_mtx_lock(&virtio_dev->eb_lock);
   if (args.cmd) {
      virtio_execbuf_flush_locked(dev);
      }
   ret = virtio_ioctl(dev->fd, VIRTGPU_RESOURCE_CREATE_BLOB, &args);
   simple_mtx_unlock(&virtio_dev->eb_lock);
   if (ret)
            struct fd_bo *bo = bo_from_handle(dev, size, args.bo_handle);
            virtio_bo->blob_id = args.blob_id;
                  fail:
      if (req.iova) {
         }
      }
