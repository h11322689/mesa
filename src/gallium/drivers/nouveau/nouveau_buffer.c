      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_surface.h"
      #include "nouveau_screen.h"
   #include "nouveau_context.h"
   #include "nouveau_winsys.h"
   #include "nouveau_fence.h"
   #include "nouveau_buffer.h"
   #include "nouveau_mm.h"
      struct nouveau_transfer {
               uint8_t *map;
   struct nouveau_bo *bo;
   struct nouveau_mm_allocation *mm;
      };
      static void *
   nouveau_user_ptr_transfer_map(struct pipe_context *pipe,
                              static void
   nouveau_user_ptr_transfer_unmap(struct pipe_context *pipe,
            static inline struct nouveau_transfer *
   nouveau_transfer(struct pipe_transfer *transfer)
   {
         }
      static inline bool
   nouveau_buffer_malloc(struct nv04_resource *buf)
   {
      if (!buf->data)
            }
      static inline bool
   nouveau_buffer_allocate(struct nouveau_screen *screen,
         {
               if (domain == NOUVEAU_BO_VRAM) {
      buf->mm = nouveau_mm_allocate(screen->mm_VRAM, size,
         if (!buf->bo)
            } else
   if (domain == NOUVEAU_BO_GART) {
      buf->mm = nouveau_mm_allocate(screen->mm_GART, size,
         if (!buf->bo)
            } else {
      assert(domain == 0);
   if (!nouveau_buffer_malloc(buf))
      }
   buf->domain = domain;
   if (buf->bo)
                        }
      static inline void
   release_allocation(struct nouveau_mm_allocation **mm,
         {
      nouveau_fence_work(fence, nouveau_mm_free_work, *mm);
      }
      inline void
   nouveau_buffer_release_gpu_storage(struct nv04_resource *buf)
   {
               nouveau_fence_work(buf->fence, nouveau_fence_unref_bo, buf->bo);
            if (buf->mm)
            if (buf->domain == NOUVEAU_BO_VRAM)
         if (buf->domain == NOUVEAU_BO_GART)
               }
      static inline bool
   nouveau_buffer_reallocate(struct nouveau_screen *screen,
         {
               nouveau_fence_ref(NULL, &buf->fence);
                        }
      void
   nouveau_buffer_destroy(struct pipe_screen *pscreen,
         {
               if (res->status & NOUVEAU_BUFFER_STATUS_USER_PTR) {
      FREE(res);
                        if (res->data && !(res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY))
            nouveau_fence_ref(NULL, &res->fence);
                                 }
      /* Set up a staging area for the transfer. This is either done in "regular"
   * system memory if the driver supports push_data (nv50+) and the data is
   * small enough (and permit_pb == true), or in GART memory.
   */
   static uint8_t *
   nouveau_transfer_staging(struct nouveau_context *nv,
         {
      const unsigned adj = tx->base.box.x & NOUVEAU_MIN_BUFFER_MAP_ALIGN_MASK;
            if (!nv->push_data)
            if ((size <= nv->screen->transfer_pushbuf_threshold) && permit_pb) {
      tx->map = align_malloc(size, NOUVEAU_MIN_BUFFER_MAP_ALIGN);
   if (tx->map)
      } else {
      tx->mm =
         if (tx->bo) {
      tx->offset += adj;
   if (!BO_MAP(nv->screen, tx->bo, 0, NULL))
         }
      }
      /* Copies data from the resource into the transfer's temporary GART
   * buffer. Also updates buf->data if present.
   *
   * Maybe just migrate to GART right away if we actually need to do this. */
   static bool
   nouveau_transfer_read(struct nouveau_context *nv, struct nouveau_transfer *tx)
   {
      struct nv04_resource *buf = nv04_resource(tx->base.resource);
   const unsigned base = tx->base.box.x;
                     nv->copy_data(nv, tx->bo, tx->offset, NOUVEAU_BO_GART,
            if (BO_WAIT(nv->screen, tx->bo, NOUVEAU_BO_RD, nv->client))
            if (buf->data)
               }
      static void
   nouveau_transfer_write(struct nouveau_context *nv, struct nouveau_transfer *tx,
         {
      struct nv04_resource *buf = nv04_resource(tx->base.resource);
   uint8_t *data = tx->map + offset;
   const unsigned base = tx->base.box.x + offset;
            if (buf->data)
         else
            if (buf->domain == NOUVEAU_BO_VRAM)
         if (buf->domain == NOUVEAU_BO_GART)
            if (tx->bo)
      nv->copy_data(nv, buf->bo, buf->offset + base, buf->domain,
      else
   if (nv->push_cb && can_cb)
      nv->push_cb(nv, buf,
      else
            nouveau_fence_ref(nv->fence, &buf->fence);
      }
      /* Does a CPU wait for the buffer's backing data to become reliably accessible
   * for write/read by waiting on the buffer's relevant fences.
   */
   static inline bool
   nouveau_buffer_sync(struct nouveau_context *nv,
         {
      if (rw == PIPE_MAP_READ) {
      if (!buf->fence_wr)
         NOUVEAU_DRV_STAT_RES(buf, buf_non_kernel_fence_sync_count,
         if (!nouveau_fence_wait(buf->fence_wr, &nv->debug))
      } else {
      if (!buf->fence)
         NOUVEAU_DRV_STAT_RES(buf, buf_non_kernel_fence_sync_count,
         if (!nouveau_fence_wait(buf->fence, &nv->debug))
               }
               }
      static inline bool
   nouveau_buffer_busy(struct nv04_resource *buf, unsigned rw)
   {
      if (rw == PIPE_MAP_READ)
         else
      }
      static inline void
   nouveau_buffer_transfer_init(struct nouveau_transfer *tx,
                     {
      tx->base.resource = resource;
   tx->base.level = 0;
   tx->base.usage = usage;
   tx->base.box.x = box->x;
   tx->base.box.y = 0;
   tx->base.box.z = 0;
   tx->base.box.width = box->width;
   tx->base.box.height = 1;
   tx->base.box.depth = 1;
   tx->base.stride = 0;
            tx->bo = NULL;
      }
      static inline void
   nouveau_buffer_transfer_del(struct nouveau_context *nv,
         {
      if (tx->map) {
      if (likely(tx->bo)) {
      nouveau_fence_work(nv->fence, nouveau_fence_unref_bo, tx->bo);
   if (tx->mm)
      } else {
      align_free(tx->map -
            }
      /* Creates a cache in system memory of the buffer data. */
   static bool
   nouveau_buffer_cache(struct nouveau_context *nv, struct nv04_resource *buf)
   {
      struct nouveau_transfer tx;
   bool ret;
   tx.base.resource = &buf->base;
   tx.base.box.x = 0;
   tx.base.box.width = buf->base.width0;
   tx.bo = NULL;
            if (!buf->data)
      if (!nouveau_buffer_malloc(buf))
      if (!(buf->status & NOUVEAU_BUFFER_STATUS_DIRTY))
                  if (!nouveau_transfer_staging(nv, &tx, false))
            ret = nouveau_transfer_read(nv, &tx);
   if (ret) {
      buf->status &= ~NOUVEAU_BUFFER_STATUS_DIRTY;
      }
   nouveau_buffer_transfer_del(nv, &tx);
      }
         #define NOUVEAU_TRANSFER_DISCARD \
            /* Checks whether it is possible to completely discard the memory backing this
   * resource. This can be useful if we would otherwise have to wait for a read
   * operation to complete on this data.
   */
   static inline bool
   nouveau_buffer_should_discard(struct nv04_resource *buf, unsigned usage)
   {
      if (!(usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE))
         if (unlikely(buf->base.bind & PIPE_BIND_SHARED))
         if (unlikely(usage & PIPE_MAP_PERSISTENT))
            }
      /* Returns a pointer to a memory area representing a window into the
   * resource's data.
   *
   * This may or may not be the _actual_ memory area of the resource. However
   * when calling nouveau_buffer_transfer_unmap, if it wasn't the actual memory
   * area, the contents of the returned map are copied over to the resource.
   *
   * The usage indicates what the caller plans to do with the map:
   *
   *   WRITE means that the user plans to write to it
   *
   *   READ means that the user plans on reading from it
   *
   *   DISCARD_WHOLE_RESOURCE means that the whole resource is going to be
   *   potentially overwritten, and even if it isn't, the bits that aren't don't
   *   need to be maintained.
   *
   *   DISCARD_RANGE means that all the data in the specified range is going to
   *   be overwritten.
   *
   * The strategy for determining what kind of memory area to return is complex,
   * see comments inside of the function.
   */
   void *
   nouveau_buffer_transfer_map(struct pipe_context *pipe,
                           {
      struct nouveau_context *nv = nouveau_context(pipe);
            if (buf->status & NOUVEAU_BUFFER_STATUS_USER_PTR)
            struct nouveau_transfer *tx = MALLOC_STRUCT(nouveau_transfer);
   uint8_t *map;
            if (!tx)
         nouveau_buffer_transfer_init(tx, resource, box, usage);
            if (usage & PIPE_MAP_READ)
         if (usage & PIPE_MAP_WRITE)
            /* If we are trying to write to an uninitialized range, the user shouldn't
   * care what was there before. So we can treat the write as if the target
   * range were being discarded. Furthermore, since we know that even if this
   * buffer is busy due to GPU activity, because the contents were
   * uninitialized, the GPU can't care what was there, and so we can treat
   * the write as being unsynchronized.
   */
   if ((usage & PIPE_MAP_WRITE) &&
      !util_ranges_intersect(&buf->valid_buffer_range, box->x, box->x + box->width))
         if (buf->domain == NOUVEAU_BO_VRAM) {
      if (usage & NOUVEAU_TRANSFER_DISCARD) {
      /* Set up a staging area for the user to write to. It will be copied
   * back into VRAM on unmap. */
   if (usage & PIPE_MAP_DISCARD_WHOLE_RESOURCE)
            } else {
      if (buf->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING) {
      /* The GPU is currently writing to this buffer. Copy its current
   * contents to a staging area in the GART. This is necessary since
   * not the whole area being mapped is being discarded.
   */
   if (buf->data) {
      align_free(buf->data);
      }
   nouveau_transfer_staging(nv, tx, false);
      } else {
      /* The buffer is currently idle. Create a staging area for writes,
   * and make sure that the cached data is up-to-date. */
   if (usage & PIPE_MAP_WRITE)
         if (!buf->data)
         }
      } else
   if (unlikely(buf->domain == 0)) {
                           if (nouveau_buffer_should_discard(buf, usage)) {
      int ref = buf->base.reference.count - 1;
   nouveau_buffer_reallocate(nv->screen, buf, buf->domain);
   if (ref > 0) /* any references inside context possible ? */
               /* Note that nouveau_bo_map ends up doing a nouveau_bo_wait with the
   * relevant flags. If buf->mm is set, that means this resource is part of a
   * larger slab bo that holds multiple resources. So in that case, don't
   * wait on the whole slab and instead use the logic below to return a
   * reasonable buffer for that case.
   */
   ret = BO_MAP(nv->screen, buf->bo,
               if (ret) {
      FREE(tx);
      }
            /* using kernel fences only if !buf->mm */
   if ((usage & PIPE_MAP_UNSYNCHRONIZED) || !buf->mm)
            /* If the GPU is currently reading/writing this buffer, we shouldn't
   * interfere with its progress. So instead we either wait for the GPU to
   * complete its operation, or set up a staging area to perform our work in.
   */
   if (nouveau_buffer_busy(buf, usage & PIPE_MAP_READ_WRITE)) {
      if (unlikely(usage & (PIPE_MAP_DISCARD_WHOLE_RESOURCE |
            /* Discarding was not possible, must sync because
   * subsequent transfers might use UNSYNCHRONIZED. */
      } else
   if (usage & PIPE_MAP_DISCARD_RANGE) {
      /* The whole range is being discarded, so it doesn't matter what was
   * there before. No need to copy anything over. */
   nouveau_transfer_staging(nv, tx, true);
      } else
   if (nouveau_buffer_busy(buf, PIPE_MAP_READ)) {
      if (usage & PIPE_MAP_DONTBLOCK)
         else
      } else {
      /* It is expected that the returned buffer be a representation of the
   * data in question, so we must copy it over from the buffer. */
   nouveau_transfer_staging(nv, tx, true);
   if (tx->map)
               }
   if (!map)
            }
            void
   nouveau_buffer_transfer_flush_region(struct pipe_context *pipe,
               {
      struct nouveau_transfer *tx = nouveau_transfer(transfer);
            if (tx->map)
            util_range_add(&buf->base, &buf->valid_buffer_range,
            }
      /* Unmap stage of the transfer. If it was a WRITE transfer and the map that
   * was returned was not the real resource's data, this needs to transfer the
   * data back to the resource.
   *
   * Also marks vbo dirty based on the buffer's binding
   */
   void
   nouveau_buffer_transfer_unmap(struct pipe_context *pipe,
         {
      struct nouveau_context *nv = nouveau_context(pipe);
            if (buf->status & NOUVEAU_BUFFER_STATUS_USER_PTR)
                     if (tx->base.usage & PIPE_MAP_WRITE) {
      if (!(tx->base.usage & PIPE_MAP_FLUSH_EXPLICIT)) {
                     util_range_add(&buf->base, &buf->valid_buffer_range,
               if (likely(buf->domain)) {
      const uint8_t bind = buf->base.bind;
   /* make sure we invalidate dedicated caches */
   if (bind & (PIPE_BIND_VERTEX_BUFFER | PIPE_BIND_INDEX_BUFFER))
                  if (!tx->bo && (tx->base.usage & PIPE_MAP_WRITE))
            nouveau_buffer_transfer_del(nv, tx);
      }
         void
   nouveau_copy_buffer(struct nouveau_context *nv,
               {
               if (likely(dst->domain) && likely(src->domain)) {
      nv->copy_data(nv,
                  dst->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;
   nouveau_fence_ref(nv->fence, &dst->fence);
            src->status |= NOUVEAU_BUFFER_STATUS_GPU_READING;
      } else {
      struct pipe_box src_box;
   src_box.x = srcx;
   src_box.y = 0;
   src_box.z = 0;
   src_box.width = size;
   src_box.height = 1;
   src_box.depth = 1;
   util_resource_copy_region(&nv->pipe,
                        }
         void *
   nouveau_resource_map_offset(struct nouveau_context *nv,
               {
      if (unlikely(res->status & NOUVEAU_BUFFER_STATUS_USER_MEMORY) ||
      unlikely(res->status & NOUVEAU_BUFFER_STATUS_USER_PTR))
         if (res->domain == NOUVEAU_BO_VRAM) {
      if (!res->data || (res->status & NOUVEAU_BUFFER_STATUS_GPU_WRITING))
      }
   if (res->domain != NOUVEAU_BO_GART)
            if (res->mm) {
      unsigned rw;
   rw = (flags & NOUVEAU_BO_WR) ? PIPE_MAP_WRITE : PIPE_MAP_READ;
   nouveau_buffer_sync(nv, res, rw);
   if (BO_MAP(nv->screen, res->bo, 0, NULL))
      } else {
      if (BO_MAP(nv->screen, res->bo, flags, nv->client))
      }
      }
      static void *
   nouveau_user_ptr_transfer_map(struct pipe_context *pipe,
                           {
      struct nouveau_transfer *tx = MALLOC_STRUCT(nouveau_transfer);
   if (!tx)
         nouveau_buffer_transfer_init(tx, resource, box, usage);
   *ptransfer = &tx->base;
      }
      static void
   nouveau_user_ptr_transfer_unmap(struct pipe_context *pipe,
         {
      struct nouveau_transfer *tx = nouveau_transfer(transfer);
      }
      struct pipe_resource *
   nouveau_buffer_create(struct pipe_screen *pscreen,
         {
      struct nouveau_screen *screen = nouveau_screen(pscreen);
   struct nv04_resource *buffer;
            buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
            buffer->base = *templ;
   pipe_reference_init(&buffer->base.reference, 1);
            if (buffer->base.flags & (PIPE_RESOURCE_FLAG_MAP_PERSISTENT |
               } else if (buffer->base.bind == 0 || (buffer->base.bind &
            switch (buffer->base.usage) {
   case PIPE_USAGE_DEFAULT:
   case PIPE_USAGE_IMMUTABLE:
      buffer->domain = NV_VRAM_DOMAIN(screen);
      case PIPE_USAGE_DYNAMIC:
      /* For most apps, we'd have to do staging transfers to avoid sync
   * with this usage, and GART -> GART copies would be suboptimal.
   */
   buffer->domain = NV_VRAM_DOMAIN(screen);
      case PIPE_USAGE_STAGING:
   case PIPE_USAGE_STREAM:
      buffer->domain = NOUVEAU_BO_GART;
      default:
      assert(0);
         } else {
      if (buffer->base.bind & screen->vidmem_bindings)
         else
   if (buffer->base.bind & screen->sysmem_bindings)
                        if (ret == false)
            if (buffer->domain == NOUVEAU_BO_VRAM && screen->hint_buf_keep_sysmem_copy)
                                    fail:
      FREE(buffer);
      }
      struct pipe_resource *
   nouveau_buffer_create_from_user(struct pipe_screen *pscreen,
               {
               buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
            buffer->base = *templ;
   /* set address and data to the same thing for higher compatibility with
   * existing code. It's correct nonetheless as the same pointer is equally
   * valid on the CPU and the GPU.
   */
   buffer->address = (uintptr_t)user_ptr;
   buffer->data = user_ptr;
   buffer->status = NOUVEAU_BUFFER_STATUS_USER_PTR;
                        }
      struct pipe_resource *
   nouveau_user_buffer_create(struct pipe_screen *pscreen, void *ptr,
         {
               buffer = CALLOC_STRUCT(nv04_resource);
   if (!buffer)
            pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = pscreen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.bind = bind;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
            buffer->data = ptr;
            util_range_init(&buffer->valid_buffer_range);
               }
      static inline bool
   nouveau_buffer_data_fetch(struct nouveau_context *nv, struct nv04_resource *buf,
         {
      if (!nouveau_buffer_malloc(buf))
         if (BO_MAP(nv->screen, bo, NOUVEAU_BO_RD, nv->client))
         memcpy(buf->data, (uint8_t *)bo->map + offset, size);
      }
      /* Migrate a linear buffer (vertex, index, constants) USER -> GART -> VRAM. */
   bool
   nouveau_buffer_migrate(struct nouveau_context *nv,
         {
               struct nouveau_screen *screen = nv->screen;
   struct nouveau_bo *bo;
   const unsigned old_domain = buf->domain;
   unsigned size = buf->base.width0;
   unsigned offset;
                     if (new_domain == NOUVEAU_BO_GART && old_domain == 0) {
      if (!nouveau_buffer_allocate(screen, buf, new_domain))
         ret = BO_MAP(nv->screen, buf->bo, 0, nv->client);
   if (ret)
         memcpy((uint8_t *)buf->bo->map + buf->offset, buf->data, size);
      } else
   if (old_domain != 0 && new_domain != 0) {
               if (new_domain == NOUVEAU_BO_VRAM) {
      /* keep a system memory copy of our data in case we hit a fallback */
   if (!nouveau_buffer_data_fetch(nv, buf, buf->bo, buf->offset, size))
         if (nouveau_mesa_debug)
               offset = buf->offset;
   bo = buf->bo;
   buf->bo = NULL;
   buf->mm = NULL;
            nv->copy_data(nv, buf->bo, buf->offset, new_domain,
            nouveau_fence_work(nv->fence, nouveau_fence_unref_bo, bo);
   if (mm)
      } else
   if (new_domain == NOUVEAU_BO_VRAM && old_domain == 0) {
      struct nouveau_transfer tx;
   if (!nouveau_buffer_allocate(screen, buf, NOUVEAU_BO_VRAM))
         tx.base.resource = &buf->base;
   tx.base.box.x = 0;
   tx.base.box.width = buf->base.width0;
   tx.bo = NULL;
   tx.map = NULL;
   if (!nouveau_transfer_staging(nv, &tx, false))
         nouveau_transfer_write(nv, &tx, 0, tx.base.box.width);
      } else
            assert(buf->domain == new_domain);
      }
      /* Migrate data from glVertexAttribPointer(non-VBO) user buffers to GART.
   * We'd like to only allocate @size bytes here, but then we'd have to rebase
   * the vertex indices ...
   */
   bool
   nouveau_user_buffer_upload(struct nouveau_context *nv,
               {
               struct nouveau_screen *screen = nouveau_screen(buf->base.screen);
                     buf->base.width0 = base + size;
   if (!nouveau_buffer_reallocate(screen, buf, NOUVEAU_BO_GART))
            ret = BO_MAP(nv->screen, buf->bo, 0, nv->client);
   if (ret)
                     }
      /* Invalidate underlying buffer storage, reset fences, reallocate to non-busy
   * buffer.
   */
   void
   nouveau_buffer_invalidate(struct pipe_context *pipe,
         {
      struct nouveau_context *nv = nouveau_context(pipe);
   struct nv04_resource *buf = nv04_resource(resource);
                     /* Shared buffers shouldn't get reallocated */
   if (unlikely(buf->base.bind & PIPE_BIND_SHARED))
            /* If the buffer is sub-allocated and not currently being written, just
   * wipe the valid buffer range. Otherwise we have to create fresh
   * storage. (We don't keep track of fences for non-sub-allocated BO's.)
   */
   if (buf->mm && !nouveau_buffer_busy(buf, PIPE_MAP_WRITE)) {
         } else {
      nouveau_buffer_reallocate(nv->screen, buf, buf->domain);
   if (ref > 0) /* any references inside context possible ? */
         }
         /* Scratch data allocation. */
      static inline int
   nouveau_scratch_bo_alloc(struct nouveau_context *nv, struct nouveau_bo **pbo,
         {
      return nouveau_bo_new(nv->screen->device, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
      }
      static void
   nouveau_scratch_unref_bos(void *d)
   {
      struct runout *b = d;
            for (i = 0; i < b->nr; ++i)
               }
      void
   nouveau_scratch_runout_release(struct nouveau_context *nv)
   {
      if (!nv->scratch.runout)
            if (!nouveau_fence_work(nv->fence, nouveau_scratch_unref_bos,
                  nv->scratch.end = 0;
      }
      /* Allocate an extra bo if we can't fit everything we need simultaneously.
   * (Could happen for very large user arrays.)
   */
   static inline bool
   nouveau_scratch_runout(struct nouveau_context *nv, unsigned size)
   {
      int ret;
            if (nv->scratch.runout)
         else
         nv->scratch.runout = REALLOC(nv->scratch.runout, n == 0 ? 0 :
               nv->scratch.runout->nr = n + 1;
            ret = nouveau_scratch_bo_alloc(nv, &nv->scratch.runout->bo[n], size);
   if (!ret) {
      ret = BO_MAP(nv->screen, nv->scratch.runout->bo[n], 0, NULL);
   if (ret)
      }
   if (!ret) {
      nv->scratch.current = nv->scratch.runout->bo[n];
   nv->scratch.offset = 0;
   nv->scratch.end = size;
      }
      }
      /* Continue to next scratch buffer, if available (no wrapping, large enough).
   * Allocate it if it has not yet been created.
   */
   static inline bool
   nouveau_scratch_next(struct nouveau_context *nv, unsigned size)
   {
      struct nouveau_bo *bo;
   int ret;
            if ((size > nv->scratch.bo_size) || (i == nv->scratch.wrap))
                  bo = nv->scratch.bo[i];
   if (!bo) {
      ret = nouveau_scratch_bo_alloc(nv, &bo, nv->scratch.bo_size);
   if (ret)
            }
   nv->scratch.current = bo;
   nv->scratch.offset = 0;
            ret = BO_MAP(nv->screen, bo, NOUVEAU_BO_WR, nv->client);
   if (!ret)
            }
      static bool
   nouveau_scratch_more(struct nouveau_context *nv, unsigned min_size)
   {
               ret = nouveau_scratch_next(nv, min_size);
   if (!ret)
            }
         /* Copy data to a scratch buffer and return address & bo the data resides in. */
   uint64_t
   nouveau_scratch_data(struct nouveau_context *nv,
               {
      unsigned bgn = MAX2(base, nv->scratch.offset);
            if (end >= nv->scratch.end) {
      end = base + size;
   if (!nouveau_scratch_more(nv, end))
            }
                     *bo = nv->scratch.current;
      }
      void *
   nouveau_scratch_get(struct nouveau_context *nv,
         {
      unsigned bgn = nv->scratch.offset;
            if (end >= nv->scratch.end) {
      end = size;
   if (!nouveau_scratch_more(nv, end))
            }
            *pbo = nv->scratch.current;
   *gpu_addr = nv->scratch.current->offset + bgn;
      }
