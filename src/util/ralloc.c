   /*
   * Copyright © 2010 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdarg.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <stdlib.h>
   #include <string.h>
      #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "util/u_printf.h"
      #include "ralloc.h"
      #define CANARY 0x5A1106
      #if defined(__LP64__) || defined(_WIN64)
   #define HEADER_ALIGN 16
   #else
   #define HEADER_ALIGN 8
   #endif
      /* Align the header's size so that ralloc() allocations will return with the
   * same alignment as a libc malloc would have (8 on 32-bit GLIBC, 16 on
   * 64-bit), avoiding performance penalities on x86 and alignment faults on
   * ARM.
   */
   struct ralloc_header
   {
            #ifndef NDEBUG
      /* A canary value used to determine whether a pointer is ralloc'd. */
   unsigned canary;
      #endif
                  /* The first child (head of a linked list) */
            /* Linked list of siblings */
   struct ralloc_header *prev;
               };
      typedef struct ralloc_header ralloc_header;
      static void unlink_block(ralloc_header *info);
   static void unsafe_free(ralloc_header *info);
      static ralloc_header *
   get_header(const void *ptr)
   {
      ralloc_header *info = (ralloc_header *) (((char *) ptr) -
         assert(info->canary == CANARY);
      }
      #define PTR_FROM_HEADER(info) (((char *) info) + sizeof(ralloc_header))
      static void
   add_child(ralloc_header *parent, ralloc_header *info)
   {
      if (parent != NULL) {
      info->parent = parent;
   info->next = parent->child;
            info->next->prev = info;
         }
      void *
   ralloc_context(const void *ctx)
   {
         }
      void *
   ralloc_size(const void *ctx, size_t size)
   {
      /* Some malloc allocation doesn't always align to 16 bytes even on 64 bits
   * system, from Android bionic/tests/malloc_test.cpp:
   *  - Allocations of a size that rounds up to a multiple of 16 bytes
   *    must have at least 16 byte alignment.
   *  - Allocations of a size that rounds up to a multiple of 8 bytes and
   *    not 16 bytes, are only required to have at least 8 byte alignment.
   */
   void *block = malloc(align64(size + sizeof(ralloc_header),
         ralloc_header *info;
            if (unlikely(block == NULL))
            info = (ralloc_header *) block;
   /* measurements have shown that calloc is slower (because of
   * the multiplication overflow checking?), so clear things
   * manually
   */
   info->parent = NULL;
   info->child = NULL;
   info->prev = NULL;
   info->next = NULL;
                           #ifndef NDEBUG
      info->canary = CANARY;
      #endif
            }
      void *
   rzalloc_size(const void *ctx, size_t size)
   {
               if (likely(ptr))
               }
      /* helper function - assumes ptr != NULL */
   static void *
   resize(void *ptr, size_t size)
   {
               old = get_header(ptr);
   info = realloc(old, align64(size + sizeof(ralloc_header),
            if (info == NULL)
            /* Update parent and sibling's links to the reallocated node. */
   if (info != old && info->parent != NULL) {
      info->parent->child = info;
            info->prev->next = info;
            info->next->prev = info;
               /* Update child->parent links for all children */
   for (child = info->child; child != NULL; child = child->next)
               }
      void *
   reralloc_size(const void *ctx, void *ptr, size_t size)
   {
      if (unlikely(ptr == NULL))
            assert(ralloc_parent(ptr) == ctx);
      }
      void *
   rerzalloc_size(const void *ctx, void *ptr, size_t old_size, size_t new_size)
   {
      if (unlikely(ptr == NULL))
            assert(ralloc_parent(ptr) == ctx);
            if (new_size > old_size)
               }
      void *
   ralloc_array_size(const void *ctx, size_t size, unsigned count)
   {
      if (count > SIZE_MAX/size)
               }
      void *
   rzalloc_array_size(const void *ctx, size_t size, unsigned count)
   {
      if (count > SIZE_MAX/size)
               }
      void *
   reralloc_array_size(const void *ctx, void *ptr, size_t size, unsigned count)
   {
      if (count > SIZE_MAX/size)
               }
      void *
   rerzalloc_array_size(const void *ctx, void *ptr, size_t size,
         {
      if (new_count > SIZE_MAX/size)
               }
      void
   ralloc_free(void *ptr)
   {
               if (ptr == NULL)
            info = get_header(ptr);
   unlink_block(info);
      }
      static void
   unlink_block(ralloc_header *info)
   {
      /* Unlink from parent & siblings */
   if (info->parent != NULL) {
      info->parent->child = info->next;
            info->prev->next = info->next;
            info->next->prev = info->prev;
      }
   info->parent = NULL;
   info->prev = NULL;
      }
      static void
   unsafe_free(ralloc_header *info)
   {
      /* Recursively free any children...don't waste time unlinking them. */
   ralloc_header *temp;
   while (info->child != NULL) {
      temp = info->child;
   info->child = temp->next;
               /* Free the block itself.  Call the destructor first, if any. */
   if (info->destructor != NULL)
               }
      void
   ralloc_steal(const void *new_ctx, void *ptr)
   {
               if (unlikely(ptr == NULL))
            info = get_header(ptr);
                        }
      void
   ralloc_adopt(const void *new_ctx, void *old_ctx)
   {
               if (unlikely(old_ctx == NULL))
            old_info = get_header(old_ctx);
            /* If there are no children, bail. */
   if (unlikely(old_info->child == NULL))
            /* Set all the children's parent to new_ctx; get a pointer to the last child. */
   for (child = old_info->child; child->next != NULL; child = child->next) {
         }
            /* Connect the two lists together; parent them to new_ctx; make old_ctx empty. */
   child->next = new_info->child;
   if (child->next)
         new_info->child = old_info->child;
      }
      void *
   ralloc_parent(const void *ptr)
   {
               if (unlikely(ptr == NULL))
            info = get_header(ptr);
      }
      void
   ralloc_set_destructor(const void *ptr, void(*destructor)(void *))
   {
      ralloc_header *info = get_header(ptr);
      }
      char *
   ralloc_strdup(const void *ctx, const char *str)
   {
      size_t n;
            if (unlikely(str == NULL))
            n = strlen(str);
   ptr = ralloc_array(ctx, char, n + 1);
   memcpy(ptr, str, n);
   ptr[n] = '\0';
      }
      char *
   ralloc_strndup(const void *ctx, const char *str, size_t max)
   {
      size_t n;
            if (unlikely(str == NULL))
            n = strnlen(str, max);
   ptr = ralloc_array(ctx, char, n + 1);
   memcpy(ptr, str, n);
   ptr[n] = '\0';
      }
      /* helper routine for strcat/strncat - n is the exact amount to copy */
   static bool
   cat(char **dest, const char *str, size_t n)
   {
      char *both;
   size_t existing_length;
            existing_length = strlen(*dest);
   both = resize(*dest, existing_length + n + 1);
   if (unlikely(both == NULL))
            memcpy(both + existing_length, str, n);
            *dest = both;
      }
         bool
   ralloc_strcat(char **dest, const char *str)
   {
         }
      bool
   ralloc_strncat(char **dest, const char *str, size_t n)
   {
         }
      bool
   ralloc_str_append(char **dest, const char *str,
         {
      char *both;
            both = resize(*dest, existing_length + str_size + 1);
   if (unlikely(both == NULL))
            memcpy(both + existing_length, str, str_size);
                        }
      char *
   ralloc_asprintf(const void *ctx, const char *fmt, ...)
   {
      char *ptr;
   va_list args;
   va_start(args, fmt);
   ptr = ralloc_vasprintf(ctx, fmt, args);
   va_end(args);
      }
      char *
   ralloc_vasprintf(const void *ctx, const char *fmt, va_list args)
   {
               char *ptr = ralloc_size(ctx, size);
   if (ptr != NULL)
               }
      bool
   ralloc_asprintf_append(char **str, const char *fmt, ...)
   {
      bool success;
   va_list args;
   va_start(args, fmt);
   success = ralloc_vasprintf_append(str, fmt, args);
   va_end(args);
      }
      bool
   ralloc_vasprintf_append(char **str, const char *fmt, va_list args)
   {
      size_t existing_length;
   assert(str != NULL);
   existing_length = *str ? strlen(*str) : 0;
      }
      bool
   ralloc_asprintf_rewrite_tail(char **str, size_t *start, const char *fmt, ...)
   {
      bool success;
   va_list args;
   va_start(args, fmt);
   success = ralloc_vasprintf_rewrite_tail(str, start, fmt, args);
   va_end(args);
      }
      bool
   ralloc_vasprintf_rewrite_tail(char **str, size_t *start, const char *fmt,
         {
      size_t new_length;
                     if (unlikely(*str == NULL)) {
      // Assuming a NULL context is probably bad, but it's expected behavior.
   *str = ralloc_vasprintf(NULL, fmt, args);
   *start = strlen(*str);
                        ptr = resize(*str, *start + new_length + 1);
   if (unlikely(ptr == NULL))
            vsnprintf(ptr + *start, new_length + 1, fmt, args);
   *str = ptr;
   *start += new_length;
      }
      /***************************************************************************
   * GC context.
   ***************************************************************************
   */
      /* The maximum size of an object that will be allocated specially.
   */
   #define MAX_FREELIST_SIZE 512
      /* Allocations small enough to be allocated from a freelist will be aligned up
   * to this size.
   */
   #define FREELIST_ALIGNMENT 32
      #define NUM_FREELIST_BUCKETS (MAX_FREELIST_SIZE / FREELIST_ALIGNMENT)
      /* The size of a slab. */
   #define SLAB_SIZE (32 * 1024)
      #define GC_CONTEXT_CANARY 0xAF6B6C83
   #define GC_CANARY 0xAF6B5B72
      enum gc_flags {
      IS_USED = (1 << 0),
   CURRENT_GENERATION = (1 << 1),
      };
      typedef struct
   {
   #ifndef NDEBUG
      /* A canary value used to determine whether a pointer is allocated using gc_alloc. */
      #endif
         uint16_t slab_offset;
   uint8_t bucket;
            /* The last padding byte must have IS_PADDING set and is used to store the amount of padding. If
   * there is no padding, the IS_PADDING bit of "flags" is unset and "flags" is checked instead.
   * Because of this, "flags" must be the last member of this struct.
   */
      } gc_block_header;
      /* This structure is at the start of the slab. Objects inside a slab are
   * allocated using a freelist backed by a simple linear allocator.
   */
   typedef struct gc_slab {
                        /* Objects are allocated using either linear or freelist allocation. "next_available" is the
   * pointer used for linear allocation, while "freelist" is the next free object for freelist
   * allocation.
   */
   char *next_available;
            /* Slabs that handle the same-sized objects. */
            /* Free slabs that handle the same-sized objects. */
            /* Number of allocated and free objects, recorded so that we can free the slab if it
   * becomes empty or add one to the freelist if it's no longer full.
   */
   unsigned num_allocated;
      } gc_slab;
      struct gc_ctx {
   #ifndef NDEBUG
         #endif
         /* Array of slabs for fixed-size allocations. Each slab tracks allocations
   * of specific sized blocks. User allocations are rounded up to the nearest
   * fixed size. slabs[N] contains allocations of size
   * FREELIST_ALIGNMENT * (N + 1).
   */
   struct {
      /* List of slabs in this bucket. */
            /* List of slabs with free space in this bucket, so we can quickly choose one when
   * allocating.
   */
               uint8_t current_gen;
      };
      static gc_block_header *
   get_gc_header(const void *ptr)
   {
               /* Adjust for padding added to ensure alignment of the allocation. There might also be padding
   * added by the compiler into gc_block_header, but that isn't counted in the IS_PADDING byte.
   */
   if (c_ptr[-1] & IS_PADDING)
                     gc_block_header *info = (gc_block_header *)c_ptr;
   assert(info->canary == GC_CANARY);
      }
      static gc_block_header *
   get_gc_freelist_next(gc_block_header *ptr)
   {
      gc_block_header *next;
   /* work around possible strict aliasing bug using memcpy */
   memcpy(&next, (void*)(ptr + 1), sizeof(next));
      }
      static void
   set_gc_freelist_next(gc_block_header *ptr, gc_block_header *next)
   {
         }
      static gc_slab *
   get_gc_slab(gc_block_header *header)
   {
         }
      gc_ctx *
   gc_context(const void *parent)
   {
      gc_ctx *ctx = rzalloc(parent, gc_ctx);
   for (unsigned i = 0; i < NUM_FREELIST_BUCKETS; i++) {
      list_inithead(&ctx->slabs[i].slabs);
         #ifndef NDEBUG
         #endif
         }
      static_assert(UINT32_MAX >= MAX_FREELIST_SIZE, "Freelist sizes use uint32_t");
      static uint32_t
   gc_bucket_obj_size(uint32_t bucket)
   {
         }
      static uint32_t
   gc_bucket_for_size(uint32_t size)
   {
         }
      static_assert(UINT32_MAX >= SLAB_SIZE, "SLAB_SIZE use uint32_t");
      static uint32_t
   gc_bucket_num_objs(uint32_t bucket)
   {
         }
      static gc_block_header *
   alloc_from_slab(gc_slab *slab, uint32_t bucket)
   {
      uint32_t size = gc_bucket_obj_size(bucket);
   gc_block_header *header;
   if (slab->freelist) {
      /* Prioritize already-allocated chunks, since they probably have a page
   * backing them.
   */
   header = slab->freelist;
      } else if (slab->next_available + size <= ((char *) slab) + SLAB_SIZE) {
      header = (gc_block_header *) slab->next_available;
   header->slab_offset = (char *) header - (char *) slab;
   header->bucket = bucket;
      } else {
                  slab->num_allocated++;
   slab->num_free--;
   if (!slab->num_free)
            }
      static void
   free_slab(gc_slab *slab)
   {
      if (list_is_linked(&slab->free_link))
         list_del(&slab->link);
      }
      static void
   free_from_slab(gc_block_header *header, bool keep_empty_slabs)
   {
               if (slab->num_allocated == 1 && !(keep_empty_slabs && list_is_singular(&slab->free_link))) {
      /* Free the slab if this is the last object. */
   free_slab(slab);
      } else if (slab->num_free == 0) {
         } else {
      /* Keep the free list sorted by the number of free objects in ascending order. By prefering to
   * allocate from the slab with the fewest free objects, we help free the slabs with many free
   * objects.
   */
   while (slab->free_link.next != &slab->ctx->slabs[header->bucket].free_slabs &&
                     /* Move "slab" to after "next". */
                  set_gc_freelist_next(header, slab->freelist);
            slab->num_allocated--;
      }
      static uint32_t
   get_slab_size(uint32_t bucket)
   {
      /* SLAB_SIZE rounded down to a multiple of the object size so that it's not larger than what can
   * be used.
   */
   uint32_t obj_size = gc_bucket_obj_size(bucket);
   uint32_t num_objs = gc_bucket_num_objs(bucket);
      }
      static gc_slab *
   create_slab(gc_ctx *ctx, unsigned bucket)
   {
      gc_slab *slab = ralloc_size(ctx, get_slab_size(bucket));
   if (unlikely(!slab))
            slab->ctx = ctx;
   slab->freelist = NULL;
   slab->next_available = (char*)(slab + 1);
   slab->num_allocated = 0;
            list_addtail(&slab->link, &ctx->slabs[bucket].slabs);
               }
      void *
   gc_alloc_size(gc_ctx *ctx, size_t size, size_t align)
   {
      assert(ctx);
                     /* Alignment will add at most align-alignof(gc_block_header) bytes of padding to the header, and
   * the IS_PADDING byte can only encode up to 127.
   */
            /* We can only align as high as the slab is. */
            size_t header_size = align64(sizeof(gc_block_header), align);
   size = align64(size, align);
            gc_block_header *header = NULL;
   if (size <= MAX_FREELIST_SIZE) {
      uint32_t bucket = gc_bucket_for_size((uint32_t)size);
   if (list_is_empty(&ctx->slabs[bucket].free_slabs) && !create_slab(ctx, bucket))
         gc_slab *slab = list_first_entry(&ctx->slabs[bucket].free_slabs, gc_slab, free_link);
      } else {
      header = ralloc_size(ctx, size);
   if (unlikely(!header))
         /* Mark the header as allocated directly, so we know to actually free it. */
                  #ifndef NDEBUG
         #endif
         uint8_t *ptr = (uint8_t *)header + header_size;
   if ((header_size - 1) != offsetof(gc_block_header, flags))
            assert(((uintptr_t)ptr & (align - 1)) == 0);
      }
      void *
   gc_zalloc_size(gc_ctx *ctx, size_t size, size_t align)
   {
               if (likely(ptr))
               }
      void
   gc_free(void *ptr)
   {
      if (!ptr)
            gc_block_header *header = get_gc_header(ptr);
            if (header->bucket < NUM_FREELIST_BUCKETS)
         else
      }
      gc_ctx *gc_get_context(void *ptr)
   {
               if (header->bucket < NUM_FREELIST_BUCKETS)
         else
      }
      void
   gc_sweep_start(gc_ctx *ctx)
   {
               ctx->rubbish = ralloc_context(NULL);
      }
      void
   gc_mark_live(gc_ctx *ctx, const void *mem)
   {
      gc_block_header *header = get_gc_header(mem);
   if (header->bucket < NUM_FREELIST_BUCKETS)
         else
      }
      void
   gc_sweep_end(gc_ctx *ctx)
   {
               for (unsigned i = 0; i < NUM_FREELIST_BUCKETS; i++) {
      unsigned obj_size = gc_bucket_obj_size(i);
   list_for_each_entry_safe(gc_slab, slab, &ctx->slabs[i].slabs, link) {
      if (!slab->num_allocated) {
      free_slab(slab);
               for (char *ptr = (char*)(slab + 1); ptr != slab->next_available; ptr += obj_size) {
      gc_block_header *header = (gc_block_header *)ptr;
   if (!(header->flags & IS_USED))
                                                if (last)
                     for (unsigned i = 0; i < NUM_FREELIST_BUCKETS; i++) {
      list_for_each_entry(gc_slab, slab, &ctx->slabs[i].slabs, link) {
      assert(slab->num_allocated > 0); /* free_from_slab() should free it otherwise */
                  ralloc_free(ctx->rubbish);
      }
      /***************************************************************************
   * Linear allocator for short-lived allocations.
   ***************************************************************************
   *
   * The allocator consists of a parent node (2K buffer), which requires
   * a ralloc parent, and child nodes (allocations). Child nodes can't be freed
   * directly, because the parent doesn't track them. You have to release
   * the parent node in order to release all its children.
   *
   * The allocator uses a fixed-sized buffer with a monotonically increasing
   * offset after each allocation. If the buffer is all used, another buffer
   * is allocated, using the linear parent node as ralloc parent.
   *
   * The linear parent node is always the first buffer and keeps track of all
   * other buffers.
   */
      #define MIN_LINEAR_BUFSIZE 2048
   #define SUBALLOC_ALIGNMENT 8
   #define LMAGIC_CONTEXT 0x87b9c7d3
   #define LMAGIC_NODE    0x87b910d3
      struct linear_ctx {
               #ifndef NDEBUG
         #endif
      unsigned offset;  /* points to the first unused byte in the latest buffer */
   unsigned size;    /* size of the latest buffer */
      };
      typedef struct linear_ctx linear_ctx;
      #ifndef NDEBUG
   struct linear_node_canary {
      alignas(HEADER_ALIGN)
   unsigned magic;
      };
      typedef struct linear_node_canary linear_node_canary;
      static linear_node_canary *
   get_node_canary(void *ptr)
   {
         }
   #endif
      static unsigned
   get_node_canary_size()
   {
   #ifndef NDEBUG
         #else
         #endif
   }
      void *
   linear_alloc_child(linear_ctx *ctx, unsigned size)
   {
      assert(ctx->magic == LMAGIC_CONTEXT);
   assert(get_node_canary(ctx->latest)->magic == LMAGIC_NODE);
                     if (unlikely(ctx->offset + size > ctx->size)) {
      /* allocate a new node */
   unsigned node_size = size;
   if (likely(node_size < MIN_LINEAR_BUFSIZE))
            const unsigned canary_size = get_node_canary_size();
            /* linear context is also a ralloc context */
   char *ptr = ralloc_size(ctx, full_size);
   if (unlikely(!ptr))
      #ifndef NDEBUG
         linear_node_canary *canary = (void *) ptr;
   canary->magic = LMAGIC_NODE;
   #endif
            /* If the new buffer is going to be full, don't update `latest`
   * pointer.  Either the current one is also full, so doesn't
   * matter, or the current one is not full, so there's still chance
   * to use that space.
   */
   #ifndef NDEBUG
         #endif
            assert((uintptr_t)(ptr + canary_size) % SUBALLOC_ALIGNMENT == 0);
               ctx->offset = 0;
   ctx->size = node_size;
               void *ptr = (char *)ctx->latest + ctx->offset;
         #ifndef NDEBUG
      linear_node_canary *canary = get_node_canary(ctx->latest);
      #endif
         assert((uintptr_t)ptr % SUBALLOC_ALIGNMENT == 0);
      }
      linear_ctx *
   linear_context(void *ralloc_ctx)
   {
               if (unlikely(!ralloc_ctx))
            const unsigned size = MIN_LINEAR_BUFSIZE;
   const unsigned canary_size = get_node_canary_size();
   const unsigned full_size =
            ctx = ralloc_size(ralloc_ctx, full_size);
   if (unlikely(!ctx))
            ctx->offset = 0;
   ctx->size = size;
      #ifndef NDEBUG
      ctx->magic = LMAGIC_CONTEXT;
   linear_node_canary *canary = get_node_canary(ctx->latest);
   canary->magic = LMAGIC_NODE;
      #endif
            }
      void *
   linear_zalloc_child(linear_ctx *ctx, unsigned size)
   {
               if (likely(ptr))
            }
      void
   linear_free_context(linear_ctx *ctx)
   {
      if (unlikely(!ctx))
                     /* Linear context is also the ralloc parent of extra nodes. */
      }
      void
   ralloc_steal_linear_context(void *new_ralloc_ctx, linear_ctx *ctx)
   {
      if (unlikely(!ctx))
                     /* Linear context is also the ralloc parent of extra nodes. */
      }
      void *
   ralloc_parent_of_linear_context(linear_ctx *ctx)
   {
      assert(ctx->magic == LMAGIC_CONTEXT);
      }
      /* All code below is pretty much copied from ralloc and only the alloc
   * calls are different.
   */
      char *
   linear_strdup(linear_ctx *ctx, const char *str)
   {
      unsigned n;
            if (unlikely(!str))
            n = strlen(str);
   ptr = linear_alloc_child(ctx, n + 1);
   if (unlikely(!ptr))
            memcpy(ptr, str, n);
   ptr[n] = '\0';
      }
      char *
   linear_asprintf(linear_ctx *ctx, const char *fmt, ...)
   {
      char *ptr;
   va_list args;
   va_start(args, fmt);
   ptr = linear_vasprintf(ctx, fmt, args);
   va_end(args);
      }
      char *
   linear_vasprintf(linear_ctx *ctx, const char *fmt, va_list args)
   {
               char *ptr = linear_alloc_child(ctx, size);
   if (ptr != NULL)
               }
      bool
   linear_asprintf_append(linear_ctx *ctx, char **str, const char *fmt, ...)
   {
      bool success;
   va_list args;
   va_start(args, fmt);
   success = linear_vasprintf_append(ctx, str, fmt, args);
   va_end(args);
      }
      bool
   linear_vasprintf_append(linear_ctx *ctx, char **str, const char *fmt, va_list args)
   {
      size_t existing_length;
   assert(str != NULL);
   existing_length = *str ? strlen(*str) : 0;
      }
      bool
   linear_asprintf_rewrite_tail(linear_ctx *ctx, char **str, size_t *start,
         {
      bool success;
   va_list args;
   va_start(args, fmt);
   success = linear_vasprintf_rewrite_tail(ctx, str, start, fmt, args);
   va_end(args);
      }
      bool
   linear_vasprintf_rewrite_tail(linear_ctx *ctx, char **str, size_t *start,
         {
      size_t new_length;
                     if (unlikely(*str == NULL)) {
      *str = linear_vasprintf(ctx, fmt, args);
   *start = strlen(*str);
                        ptr = linear_alloc_child(ctx, *start + new_length + 1);
   if (unlikely(ptr == NULL))
                     vsnprintf(ptr + *start, new_length + 1, fmt, args);
   *str = ptr;
   *start += new_length;
      }
      /* helper routine for strcat/strncat - n is the exact amount to copy */
   static bool
   linear_cat(linear_ctx *ctx, char **dest, const char *str, unsigned n)
   {
      char *both;
   unsigned existing_length;
            existing_length = strlen(*dest);
   both = linear_alloc_child(ctx, existing_length + n + 1);
   if (unlikely(both == NULL))
            memcpy(both, *dest, existing_length);
   memcpy(both + existing_length, str, n);
            *dest = both;
      }
      bool
   linear_strcat(linear_ctx *ctx, char **dest, const char *str)
   {
         }
      void *
   linear_alloc_child_array(linear_ctx *ctx, size_t size, unsigned count)
   {
      if (count > SIZE_MAX/size)
               }
      void *
   linear_zalloc_child_array(linear_ctx *ctx, size_t size, unsigned count)
   {
      if (count > SIZE_MAX/size)
               }
      typedef struct {
      FILE *f;
            unsigned ralloc_count;
   unsigned linear_count;
            /* These don't include padding or metadata from suballocators. */
   unsigned content_bytes;
   unsigned ralloc_metadata_bytes;
   unsigned linear_metadata_bytes;
            bool inside_linear;
      } ralloc_print_info_state;
      static void
   ralloc_print_info_helper(ralloc_print_info_state *state, const ralloc_header *info)
   {
               if (f) {
      for (unsigned i = 0; i < state->indent; i++) fputc(' ', f);
                     #ifndef NDEBUG
      assert(info->canary == CANARY);
   if (f) fprintf(f, " (%d bytes)", info->size);
   state->content_bytes += info->size;
            const void *ptr = PTR_FROM_HEADER(info);
   const linear_ctx *lin_ctx = ptr;
            if (lin_ctx->magic == LMAGIC_CONTEXT) {
      if (f) fprintf(f, " (linear context)");
   assert(!state->inside_gc && !state->inside_linear);
   state->inside_linear = true;
   state->linear_metadata_bytes += sizeof(linear_ctx);
   state->content_bytes -= sizeof(linear_ctx);
      } else if (gc_ctx->canary == GC_CONTEXT_CANARY) {
      if (f) fprintf(f, " (gc context)");
   assert(!state->inside_gc && !state->inside_linear);
   state->inside_gc = true;
      } else if (state->inside_linear) {
      const linear_node_canary *lin_node = ptr;
   if (lin_node->magic == LMAGIC_NODE) {
      if (f) fprintf(f, " (linear node buffer)");
   state->content_bytes -= sizeof(linear_node_canary);
   state->linear_metadata_bytes += sizeof(linear_node_canary);
         } else if (state->inside_gc) {
      if (f) fprintf(f, " (gc slab or large block)");
         #endif
         state->ralloc_count++;
            const ralloc_header *c = info->child;
   state->indent += 2;
   while (c != NULL) {
      ralloc_print_info_helper(state, c);
      }
         #ifndef NDEBUG
      if (lin_ctx->magic == LMAGIC_CONTEXT) state->inside_linear = false;
      #endif
   }
      void
   ralloc_print_info(FILE *f, const void *p, unsigned flags)
   {
      ralloc_print_info_state state = {
                  const ralloc_header *info = get_header(p);
            fprintf(f, "==== RALLOC INFO ptr=%p info=%p\n"
            "ralloc allocations    = %d\n"
   "  - linear            = %d\n"
   "  - gc                = %d\n"
   "  - other             = %d\n",
   p, info,
   state.ralloc_count,
   state.linear_count,
         if (state.content_bytes) {
      fprintf(f,
         "content bytes         = %d\n"
   "ralloc metadata bytes = %d\n"
   "linear metadata bytes = %d\n",
   state.content_bytes,
                  }
   