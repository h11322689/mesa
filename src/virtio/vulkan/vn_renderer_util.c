   /*
   * Copyright 2021 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vn_renderer_util.h"
      void
   vn_renderer_shmem_pool_init(UNUSED struct vn_renderer *renderer,
               {
      *pool = (struct vn_renderer_shmem_pool){
      /* power-of-two to hit shmem cache */
         }
      void
   vn_renderer_shmem_pool_fini(struct vn_renderer *renderer,
         {
      if (pool->shmem)
      }
      static bool
   vn_renderer_shmem_pool_grow(struct vn_renderer *renderer,
               {
      VN_TRACE_FUNC();
   /* power-of-two to hit shmem cache */
   size_t alloc_size = pool->min_alloc_size;
   while (alloc_size < size) {
      alloc_size <<= 1;
   if (!alloc_size)
               struct vn_renderer_shmem *shmem =
         if (!shmem)
            if (pool->shmem)
            pool->shmem = shmem;
   pool->size = alloc_size;
               }
      struct vn_renderer_shmem *
   vn_renderer_shmem_pool_alloc(struct vn_renderer *renderer,
                     {
      if (unlikely(size > pool->size - pool->used)) {
      if (!vn_renderer_shmem_pool_grow(renderer, pool, size))
                        struct vn_renderer_shmem *shmem =
         *out_offset = pool->used;
               }
