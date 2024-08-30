   /*
   * Copyright 2018 Alyssa Rosenzweig
   * Copyright 2019 Collabora, Ltd.
   * SPDX-License-Identifier: MIT
   *
   */
      #include "pool.h"
   #include "agx_bo.h"
   #include "agx_device.h"
      /* Transient command stream pooling: command stream uploads try to simply copy
   * into wherever we left off. If there isn't space, we allocate a new entry
   * into the pool and copy there */
      #define POOL_SLAB_SIZE (256 * 1024)
      static struct agx_bo *
   agx_pool_alloc_backing(struct agx_pool *pool, size_t bo_sz)
   {
      struct agx_bo *bo =
            util_dynarray_append(&pool->bos, struct agx_bo *, bo);
   pool->transient_bo = bo;
               }
      void
   agx_pool_init(struct agx_pool *pool, struct agx_device *dev,
         {
      memset(pool, 0, sizeof(*pool));
   pool->dev = dev;
   pool->create_flags = create_flags;
            if (prealloc)
      }
      void
   agx_pool_cleanup(struct agx_pool *pool)
   {
      util_dynarray_foreach(&pool->bos, struct agx_bo *, bo) {
                     }
      void
   agx_pool_get_bo_handles(struct agx_pool *pool, uint32_t *handles)
   {
      unsigned idx = 0;
   util_dynarray_foreach(&pool->bos, struct agx_bo *, bo) {
            }
      struct agx_ptr
   agx_pool_alloc_aligned_with_bo(struct agx_pool *pool, size_t sz,
         {
               /* Find or create a suitable BO */
   struct agx_bo *bo = pool->transient_bo;
            /* If we don't fit, allocate a new backing */
   if (unlikely(bo == NULL || (offset + sz) >= POOL_SLAB_SIZE)) {
      bo = agx_pool_alloc_backing(pool,
                              struct agx_ptr ret = {
      .cpu = bo->ptr.cpu + offset,
               if (out_bo)
               }
      uint64_t
   agx_pool_upload(struct agx_pool *pool, const void *data, size_t sz)
   {
         }
      uint64_t
   agx_pool_upload_aligned_with_bo(struct agx_pool *pool, const void *data,
               {
      struct agx_ptr transfer =
         memcpy(transfer.cpu, data, sz);
      }
