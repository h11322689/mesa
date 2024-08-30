   #include "nouveau_bo.h"
      #include "drm-uapi/nouveau_drm.h"
   #include "util/hash_table.h"
   #include "util/u_math.h"
      #include <errno.h>
   #include <fcntl.h>
   #include <stddef.h>
   #include <sys/mman.h>
   #include <xf86drm.h>
      static void
   bo_bind(struct nouveau_ws_device *dev,
         uint32_t handle, uint64_t addr,
   uint64_t range, uint64_t bo_offset,
   {
               struct drm_nouveau_vm_bind_op newbindop = {
      .op = DRM_NOUVEAU_VM_BIND_OP_MAP,
   .handle = handle,
   .addr = addr,
   .range = range,
   .bo_offset = bo_offset,
      };
   struct drm_nouveau_vm_bind vmbind = {
      .op_count = 1,
      };
   ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_VM_BIND, &vmbind, sizeof(vmbind));
   if (ret)
            }
      static void
   bo_unbind(struct nouveau_ws_device *dev,
               {
      struct drm_nouveau_vm_bind_op newbindop = {
      .op = DRM_NOUVEAU_VM_BIND_OP_UNMAP,
   .addr = offset,
   .range = range,
      };
   struct drm_nouveau_vm_bind vmbind = {
      .op_count = 1,
      };
   ASSERTED int ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_VM_BIND, &vmbind, sizeof(vmbind));
      }
      uint64_t
   nouveau_ws_alloc_vma(struct nouveau_ws_device *dev,
               {
               uint64_t offset;
   simple_mtx_lock(&dev->vma_mutex);
   offset = util_vma_heap_alloc(&dev->vma_heap, size, align);
            if (dev->debug_flags & NVK_DEBUG_VM)
      fprintf(stderr, "alloc vma %" PRIx64 " %" PRIx64 " sparse: %d\n",
         if (sparse_resident)
               }
      void
   nouveau_ws_free_vma(struct nouveau_ws_device *dev,
               {
               if (dev->debug_flags & NVK_DEBUG_VM)
      fprintf(stderr, "free vma %" PRIx64 " %" PRIx64 "\n",
         if (sparse_resident)
            simple_mtx_lock(&dev->vma_mutex);
   util_vma_heap_free(&dev->vma_heap, offset, size);
      }
      void
   nouveau_ws_bo_unbind_vma(struct nouveau_ws_device *dev,
         {
               if (dev->debug_flags & NVK_DEBUG_VM)
      fprintf(stderr, "unbind vma %" PRIx64 " %" PRIx64 "\n",
         }
      void
   nouveau_ws_bo_bind_vma(struct nouveau_ws_device *dev,
                        struct nouveau_ws_bo *bo,
      {
               if (dev->debug_flags & NVK_DEBUG_VM)
      fprintf(stderr, "bind vma %x %" PRIx64 " %" PRIx64 " %" PRIx64 " %d\n",
         }
      struct nouveau_ws_bo *
   nouveau_ws_bo_new(struct nouveau_ws_device *dev,
               {
         }
      struct nouveau_ws_bo *
   nouveau_ws_bo_new_mapped(struct nouveau_ws_device *dev,
                           {
      struct nouveau_ws_bo *bo = nouveau_ws_bo_new(dev, size, align,
         if (!bo)
            void *map = nouveau_ws_bo_map(bo, map_flags);
   if (map == NULL) {
      nouveau_ws_bo_destroy(bo);
               *map_out = map;
      }
      struct nouveau_ws_bo *
   nouveau_ws_bo_new_tiled(struct nouveau_ws_device *dev,
                     {
      struct nouveau_ws_bo *bo = CALLOC_STRUCT(nouveau_ws_bo);
            /* if the caller doesn't care, use the GPU page size */
   if (align == 0)
            /* Align the size */
                     if (flags & NOUVEAU_WS_BO_GART)
         else
            if (flags & NOUVEAU_WS_BO_MAP)
            if (flags & NOUVEAU_WS_BO_NO_SHARE)
               req.info.size = size;
                     int ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_GEM_NEW, &req, sizeof(req));
   if (ret == 0) {
      bo->size = size;
   bo->align = align;
   bo->offset = -1ULL;
   bo->handle = req.info.handle;
   bo->map_handle = req.info.map_handle;
   bo->dev = dev;
   bo->flags = flags;
            if (dev->has_vm_bind) {
      assert(pte_kind == 0);
   bo->offset = nouveau_ws_alloc_vma(dev, bo->size, align, false);
                  } else {
      FREE(bo);
                           }
      struct nouveau_ws_bo *
   nouveau_ws_bo_from_dma_buf(struct nouveau_ws_device *dev, int fd)
   {
                        uint32_t handle;
   int ret = drmPrimeFDToHandle(dev->fd, fd, &handle);
   if (ret == 0) {
      struct hash_entry *entry =
         if (entry != NULL) {
         } else {
      struct drm_nouveau_gem_info info = {
         };
   ret = drmCommandWriteRead(dev->fd, DRM_NOUVEAU_GEM_INFO,
         if (ret == 0) {
      enum nouveau_ws_bo_flags flags = 0;
   if (info.domain & NOUVEAU_GEM_DOMAIN_GART)
                        bo = CALLOC_STRUCT(nouveau_ws_bo);
   bo->size = info.size;
   bo->offset = info.offset;
   bo->handle = info.handle;
   bo->map_handle = info.map_handle;
   bo->dev = dev;
                  uint64_t align = (1ULL << 12);
                           bo->offset = nouveau_ws_alloc_vma(dev, bo->size, align, false);
   nouveau_ws_bo_bind_vma(dev, bo, bo->offset, bo->size, 0, 0);
                                 }
      void
   nouveau_ws_bo_destroy(struct nouveau_ws_bo *bo)
   {
      if (--bo->refcnt)
                                       if (dev->has_vm_bind) {
      nouveau_ws_bo_unbind_vma(bo->dev, bo->offset, bo->size);
               drmCloseBufferHandle(bo->dev->fd, bo->handle);
               }
      void *
   nouveau_ws_bo_map(struct nouveau_ws_bo *bo, enum nouveau_ws_bo_map_flags flags)
   {
               if (flags & NOUVEAU_WS_BO_RD)
         if (flags & NOUVEAU_WS_BO_WR)
            void *res = mmap(NULL, bo->size, prot, MAP_SHARED, bo->dev->fd, bo->map_handle);
   if (res == MAP_FAILED)
               }
      bool
   nouveau_ws_bo_wait(struct nouveau_ws_bo *bo, enum nouveau_ws_bo_map_flags flags)
   {
               req.handle = bo->handle;
   if (flags & NOUVEAU_WS_BO_WR)
               }
      int
   nouveau_ws_bo_dma_buf(struct nouveau_ws_bo *bo, int *fd)
   {
         }
