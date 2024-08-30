   /*
   * Copyright 2021 Alyssa Rosenzweig
   * Copyright 2019 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   */
      #include "agx_device.h"
   #include <inttypes.h>
   #include "agx_bo.h"
   #include "decode.h"
      #include <fcntl.h>
   #include <xf86drm.h>
   #include "drm-uapi/dma-buf.h"
   #include "util/log.h"
   #include "util/os_file.h"
   #include "util/os_mman.h"
   #include "util/simple_mtx.h"
      /* TODO: Linux UAPI. Dummy defines to get some things to compile. */
   #define ASAHI_BIND_READ  0
   #define ASAHI_BIND_WRITE 0
      void
   agx_bo_free(struct agx_device *dev, struct agx_bo *bo)
   {
               if (bo->ptr.cpu)
            if (bo->ptr.gpu) {
      struct util_vma_heap *heap;
            if (bo->flags & AGX_BO_LOW_VA) {
      heap = &dev->usc_heap;
      } else {
                  simple_mtx_lock(&dev->vma_lock);
   util_vma_heap_free(heap, bo_addr, bo->size + dev->guard_size);
            /* No need to unmap the BO, as the kernel will take care of that when we
               if (bo->prime_fd != -1)
            /* Reset the handle. This has to happen before the GEM close to avoid a race.
   */
   memset(bo, 0, sizeof(*bo));
            struct drm_gem_close args = {.handle = handle};
      }
      static int
   agx_bo_bind(struct agx_device *dev, struct agx_bo *bo, uint64_t addr,
         {
         }
      struct agx_bo *
   agx_bo_alloc(struct agx_device *dev, size_t size, enum agx_bo_flags flags)
   {
      struct agx_bo *bo;
                     /* executable implies low va */
                     pthread_mutex_lock(&dev->bo_map_lock);
   bo = agx_lookup_bo(dev, handle);
   dev->max_handle = MAX2(dev->max_handle, handle);
            /* Fresh handle */
            bo->type = AGX_ALLOC_REGULAR;
   bo->size = size; /* TODO: gem_create.size */
   bo->flags = flags;
   bo->dev = dev;
   bo->handle = handle;
                     struct util_vma_heap *heap;
   if (lo)
         else
            simple_mtx_lock(&dev->vma_lock);
   bo->ptr.gpu = util_vma_heap_alloc(heap, size + dev->guard_size,
         simple_mtx_unlock(&dev->vma_lock);
   if (!bo->ptr.gpu) {
      fprintf(stderr, "Failed to allocate BO VMA\n");
   agx_bo_free(dev, bo);
                        uint32_t bind = ASAHI_BIND_READ;
   if (!(flags & AGX_BO_READONLY)) {
                  int ret = agx_bo_bind(dev, bo, bo->ptr.gpu, bind);
   if (ret) {
      agx_bo_free(dev, bo);
                        if (flags & AGX_BO_LOW_VA)
                        }
      void
   agx_bo_mmap(struct agx_bo *bo)
   {
         }
      struct agx_bo *
   agx_bo_import(struct agx_device *dev, int fd)
   {
      struct agx_bo *bo;
   ASSERTED int ret;
                     ret = drmPrimeFDToHandle(dev->fd, fd, &gem_handle);
   if (ret) {
      fprintf(stderr, "import failed: Could not map fd %d to handle\n", fd);
   pthread_mutex_unlock(&dev->bo_map_lock);
               bo = agx_lookup_bo(dev, gem_handle);
            if (!bo->dev) {
      bo->dev = dev;
            /* Sometimes this can fail and return -1. size of -1 is not
   * a nice thing for mmap to try mmap. Be more robust also
   * for zero sized maps and fail nicely too
   */
   if ((bo->size == 0) || (bo->size == (size_t)-1)) {
      pthread_mutex_unlock(&dev->bo_map_lock);
      }
   if (bo->size & (dev->params.vm_page_size - 1)) {
      fprintf(
      stderr,
   "import failed: BO is not a multiple of the page size (0x%llx bytes)\n",
                  bo->flags = AGX_BO_SHARED | AGX_BO_SHAREABLE;
   bo->handle = gem_handle;
   bo->prime_fd = os_dupfd_cloexec(fd);
   bo->label = "Imported BO";
                     simple_mtx_lock(&dev->vma_lock);
   bo->ptr.gpu = util_vma_heap_alloc(
                  if (!bo->ptr.gpu) {
      fprintf(
      stderr,
   "import failed: Could not allocate from VMA heap (0x%llx bytes)\n",
                  ret =
         if (ret) {
      fprintf(stderr, "import failed: Could not bind BO at 0x%llx\n",
               } else {
      /* bo->refcnt == 0 can happen if the BO
   * was being released but agx_bo_import() acquired the
   * lock before agx_bo_unreference(). In that case, refcnt
   * is 0 and we can't use agx_bo_reference() directly, we
   * have to re-initialize the refcnt().
   * Note that agx_bo_unreference() checks
   * refcnt value just after acquiring the lock to
   * make sure the object is not freed if agx_bo_import()
   * acquired it in the meantime.
   */
   if (p_atomic_read(&bo->refcnt) == 0)
         else
      }
                  error:
      memset(bo, 0, sizeof(*bo));
   pthread_mutex_unlock(&dev->bo_map_lock);
      }
      int
   agx_bo_export(struct agx_bo *bo)
   {
                        if (drmPrimeHandleToFD(bo->dev->fd, bo->handle, DRM_CLOEXEC, &fd))
            if (!(bo->flags & AGX_BO_SHARED)) {
      bo->flags |= AGX_BO_SHARED;
   assert(bo->prime_fd == -1);
            /* If there is a pending writer to this BO, import it into the buffer
   * for implicit sync.
   */
   uint32_t writer_syncobj = p_atomic_read_relaxed(&bo->writer_syncobj);
   if (writer_syncobj) {
      int out_sync_fd = -1;
   int ret =
                        ret = agx_import_sync_file(bo->dev, bo, out_sync_fd);
   assert(ret >= 0);
                  assert(bo->prime_fd >= 0);
      }
      static void
   agx_get_global_ids(struct agx_device *dev)
   {
      dev->next_global_id = 0;
      }
      uint64_t
   agx_get_global_id(struct agx_device *dev)
   {
      if (unlikely(dev->next_global_id >= dev->last_global_id)) {
                     }
      static ssize_t
   agx_get_params(struct agx_device *dev, void *buf, size_t size)
   {
      /* TODO: Linux UAPI */
      }
      bool
   agx_open_device(void *memctx, struct agx_device *dev)
   {
               /* TODO: Linux UAPI */
            params_size = agx_get_params(dev, &dev->params, sizeof(dev->params));
   if (params_size <= 0) {
      assert(0);
      }
            /* TODO: Linux UAPI: Params */
            util_sparse_array_init(&dev->bo_map, sizeof(struct agx_bo), 512);
            simple_mtx_init(&dev->bo_cache.lock, mtx_plain);
            for (unsigned i = 0; i < ARRAY_SIZE(dev->bo_cache.buckets); ++i)
                     simple_mtx_init(&dev->vma_lock, mtx_plain);
   util_vma_heap_init(&dev->main_heap, dev->params.vm_user_start,
         util_vma_heap_init(
      &dev->usc_heap, dev->params.vm_shader_start,
         dev->queue_id = agx_create_command_queue(dev, 0 /* TODO: CAPS */);
               }
      void
   agx_close_device(struct agx_device *dev)
   {
      agx_bo_cache_evict_all(dev);
            util_vma_heap_finish(&dev->main_heap);
               }
      uint32_t
   agx_create_command_queue(struct agx_device *dev, uint32_t caps)
   {
         }
      int
   agx_submit_single(struct agx_device *dev, enum drm_asahi_cmd_type cmd_type,
                     uint32_t barriers, struct drm_asahi_sync *in_syncs,
   {
         }
      int
   agx_import_sync_file(struct agx_device *dev, struct agx_bo *bo, int fd)
   {
      struct dma_buf_import_sync_file import_sync_file_ioctl = {
      .flags = DMA_BUF_SYNC_WRITE,
               assert(fd >= 0);
            int ret = drmIoctl(bo->prime_fd, DMA_BUF_IOCTL_IMPORT_SYNC_FILE,
                     }
      int
   agx_export_sync_file(struct agx_device *dev, struct agx_bo *bo)
   {
      struct dma_buf_export_sync_file export_sync_file_ioctl = {
      .flags = DMA_BUF_SYNC_RW,
                        int ret = drmIoctl(bo->prime_fd, DMA_BUF_IOCTL_EXPORT_SYNC_FILE,
         assert(ret >= 0);
               }
      void
   agx_debug_fault(struct agx_device *dev, uint64_t addr)
   {
                        for (uint32_t handle = 0; handle < dev->max_handle; handle++) {
      struct agx_bo *bo = agx_lookup_bo(dev, handle);
   uint64_t bo_addr = bo->ptr.gpu;
   if (bo->flags & AGX_BO_LOW_VA)
            if (!bo->dev || bo_addr > addr)
            if (!best || bo_addr > best->ptr.gpu)
               if (!best) {
         } else {
      uint64_t start = best->ptr.gpu;
   uint64_t end = best->ptr.gpu + best->size;
   if (addr > (end + 1024 * 1024 * 1024)) {
      /* 1GiB max as a sanity check */
      } else if (addr > end) {
      mesa_logw("Address 0x%" PRIx64 " is 0x%" PRIx64
            " bytes beyond an object at 0x%" PRIx64 "..0x%" PRIx64
   } else {
      mesa_logw("Address 0x%" PRIx64 " is 0x%" PRIx64
            " bytes into an object at 0x%" PRIx64 "..0x%" PRIx64
                  }
