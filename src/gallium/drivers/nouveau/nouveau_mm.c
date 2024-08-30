      #include <inttypes.h>
      #include "util/simple_mtx.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/list.h"
      #include "nouveau_winsys.h"
   #include "nouveau_screen.h"
   #include "nouveau_mm.h"
      /* TODO: Higher orders can waste a lot of space for npot size buffers, should
   * add an extra cache for such buffer objects.
   *
   * HACK: Max order == 21 to accommodate TF2's 1.5 MiB, frequently reallocated
   * vertex buffer (VM flush (?) decreases performance dramatically).
   */
      #define MM_MIN_ORDER 7 /* >= 6 to not violate ARB_map_buffer_alignment */
   #define MM_MAX_ORDER 21
      #define MM_NUM_BUCKETS (MM_MAX_ORDER - MM_MIN_ORDER + 1)
      #define MM_MIN_SIZE (1 << MM_MIN_ORDER)
   #define MM_MAX_SIZE (1 << MM_MAX_ORDER)
      struct mm_bucket {
      struct list_head free;
   struct list_head used;
   struct list_head full;
   int num_free;
      };
      struct nouveau_mman {
      struct nouveau_device *dev;
   struct mm_bucket bucket[MM_NUM_BUCKETS];
   uint32_t domain;
   union nouveau_bo_config config;
      };
      struct mm_slab {
      struct list_head head;
   struct nouveau_bo *bo;
   struct nouveau_mman *cache;
   int order;
   int count;
   int free;
      };
      static int
   mm_slab_alloc(struct mm_slab *slab)
   {
               if (slab->free == 0)
            for (i = 0; i < (slab->count + 31) / 32; ++i) {
      b = ffs(slab->bits[i]) - 1;
   if (b >= 0) {
      n = i * 32 + b;
   assert(n < slab->count);
   slab->free--;
   slab->bits[i] &= ~(1 << b);
         }
      }
      static inline void
   mm_slab_free(struct mm_slab *slab, int i)
   {
      assert(i < slab->count);
   slab->bits[i / 32] |= 1 << (i % 32);
   slab->free++;
      }
      static inline int
   mm_get_order(uint32_t size)
   {
               if (size > (1 << s))
            }
      static struct mm_bucket *
   mm_bucket_by_order(struct nouveau_mman *cache, int order)
   {
      if (order > MM_MAX_ORDER)
            }
      static struct mm_bucket *
   mm_bucket_by_size(struct nouveau_mman *cache, unsigned size)
   {
         }
      /* size of bo allocation for slab with chunks of (1 << chunk_order) bytes */
   static inline uint32_t
   mm_default_slab_size(unsigned chunk_order)
   {
      static const int8_t slab_order[MM_MAX_ORDER - MM_MIN_ORDER + 1] =
   {
                              }
      static int
   mm_slab_new(struct nouveau_mman *cache, struct mm_bucket *bucket, int chunk_order)
   {
      struct mm_slab *slab;
   int words, ret;
                     words = ((size >> chunk_order) + 31) / 32;
            slab = MALLOC(sizeof(struct mm_slab) + words * 4);
   if (!slab)
                              ret = nouveau_bo_new(cache->dev, cache->domain, 0, size, &cache->config,
         if (ret) {
      FREE(slab);
                        slab->cache = cache;
   slab->order = chunk_order;
            assert(bucket == mm_bucket_by_order(cache, chunk_order));
                     if (nouveau_mesa_debug)
      debug_printf("MM: new slab, total memory = %"PRIu64" KiB\n",
            }
      /* @return token to identify slab or NULL if we just allocated a new bo */
   struct nouveau_mm_allocation *
   nouveau_mm_allocate(struct nouveau_mman *cache,
         {
      struct mm_bucket *bucket;
   struct mm_slab *slab;
   struct nouveau_mm_allocation *alloc;
            bucket = mm_bucket_by_size(cache, size);
   if (!bucket) {
      ret = nouveau_bo_new(cache->dev, cache->domain, 0, size, &cache->config,
         if (ret)
                  *offset = 0;
               alloc = MALLOC_STRUCT(nouveau_mm_allocation);
   if (!alloc)
            simple_mtx_lock(&bucket->lock);
   if (!list_is_empty(&bucket->used)) {
         } else {
      if (list_is_empty(&bucket->free)) {
         }
            list_del(&slab->head);
                                 if (slab->free == 0) {
      list_del(&slab->head);
      }
            alloc->offset = *offset;
               }
      void
   nouveau_mm_free(struct nouveau_mm_allocation *alloc)
   {
      struct mm_slab *slab = (struct mm_slab *)alloc->priv;
            simple_mtx_lock(&bucket->lock);
            if (slab->free == slab->count) {
      list_del(&slab->head);
      } else
   if (slab->free == 1) {
      list_del(&slab->head);
      }
               }
      void
   nouveau_mm_free_work(void *data)
   {
         }
      struct nouveau_mman *
   nouveau_mm_create(struct nouveau_device *dev, uint32_t domain,
         {
      struct nouveau_mman *cache = MALLOC_STRUCT(nouveau_mman);
            if (!cache)
            cache->dev = dev;
   cache->domain = domain;
   cache->config = *config;
            for (i = 0; i < MM_NUM_BUCKETS; ++i) {
      list_inithead(&cache->bucket[i].free);
   list_inithead(&cache->bucket[i].used);
   list_inithead(&cache->bucket[i].full);
                  }
      static inline void
   nouveau_mm_free_slabs(struct list_head *head)
   {
               LIST_FOR_EACH_ENTRY_SAFE(slab, next, head, head) {
      list_del(&slab->head);
   nouveau_bo_ref(NULL, &slab->bo);
         }
      void
   nouveau_mm_destroy(struct nouveau_mman *cache)
   {
               if (!cache)
            for (i = 0; i < MM_NUM_BUCKETS; ++i) {
      if (!list_is_empty(&cache->bucket[i].used) ||
      !list_is_empty(&cache->bucket[i].full))
               nouveau_mm_free_slabs(&cache->bucket[i].free);
   nouveau_mm_free_slabs(&cache->bucket[i].used);
   nouveau_mm_free_slabs(&cache->bucket[i].full);
                  }
