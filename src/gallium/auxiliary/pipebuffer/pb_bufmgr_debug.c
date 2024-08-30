   /**************************************************************************
   *
   * Copyright 2007-2008 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /**
   * \file
   * Debug buffer manager to detect buffer under- and overflows.
   * 
   * \author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "util/compiler.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/list.h"
   #include "util/u_debug_stack.h"
   #include <inttypes.h>
      #include "pb_buffer.h"
   #include "pb_bufmgr.h"
         #ifdef DEBUG
         #define PB_DEBUG_CREATE_BACKTRACE 8
   #define PB_DEBUG_MAP_BACKTRACE 8
         /**
   * Convenience macro (type safe).
   */
   #define SUPER(__derived) (&(__derived)->base)
         struct pb_debug_manager;
         /**
   * Wrapper around a pipe buffer which adds delayed destruction.
   */
   struct pb_debug_buffer
   {
      struct pb_buffer base;
      struct pb_buffer *buffer;
   struct pb_debug_manager *mgr;
      pb_size underflow_size;
                     mtx_t mutex;
   unsigned map_count;
   struct debug_stack_frame map_backtrace[PB_DEBUG_MAP_BACKTRACE];
         };
         struct pb_debug_manager
   {
                        pb_size underflow_size;
   pb_size overflow_size;
      mtx_t mutex;
      };
         static inline struct pb_debug_buffer *
   pb_debug_buffer(struct pb_buffer *buf)
   {
      assert(buf);
      }
         static inline struct pb_debug_manager *
   pb_debug_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         static const uint8_t random_pattern[32] = {
      0xaf, 0xcf, 0xa5, 0xa2, 0xc2, 0x63, 0x15, 0x1a, 
   0x7e, 0xe2, 0x7e, 0x84, 0x15, 0x49, 0xa2, 0x1e,
   0x49, 0x63, 0xf5, 0x52, 0x74, 0x66, 0x9e, 0xc4, 
      };
         static inline void 
   fill_random_pattern(uint8_t *dst, pb_size size)
   {
      pb_size i = 0;
   while(size--) {
      *dst++ = random_pattern[i++];
         }
         static inline bool 
   check_random_pattern(const uint8_t *dst, pb_size size, 
         {
      bool result = true;
   pb_size i;
   *min_ofs = size;
   *max_ofs = 0;
   for(i = 0; i < size; ++i) {
      if(*dst++ != random_pattern[i % sizeof(random_pattern)]) {
         result = false;
            }
      }
         static void
   pb_debug_buffer_fill(struct pb_debug_buffer *buf)
   {
      uint8_t *map;
      map = pb_map(buf->buffer, PB_USAGE_CPU_WRITE, NULL);
   assert(map);
   if (map) {
      fill_random_pattern(map, buf->underflow_size);
   fill_random_pattern(map + buf->underflow_size + buf->base.size,
               }
         /**
   * Check for under/over flows.
   * 
   * Should be called with the buffer unmaped.
   */
   static void
   pb_debug_buffer_check(struct pb_debug_buffer *buf)
   {
      uint8_t *map;
      map = pb_map(buf->buffer,
               assert(map);
   if (map) {
      bool underflow, overflow;
   pb_size min_ofs, max_ofs;
      underflow = !check_random_pattern(map, buf->underflow_size, 
         if(underflow) {
      debug_printf("buffer underflow (offset -%"PRIu64"%s to -%"PRIu64" bytes) detected\n",
                  }
      overflow = !check_random_pattern(map + buf->underflow_size + buf->base.size,
               if(overflow) {
      debug_printf("buffer overflow (size %"PRIu64" plus offset %"PRIu64" to %"PRIu64"%s bytes) detected\n",
               buf->base.size,
      }
      if(underflow || overflow)
            assert(!underflow);
            /* re-fill if not aborted */
   if(underflow)
         if(overflow)
                        }
         static void
   pb_debug_buffer_destroy(void *winsys, struct pb_buffer *_buf)
   {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
   struct pb_debug_manager *mgr = buf->mgr;
      assert(!pipe_is_referenced(&buf->base.reference));
               mtx_lock(&mgr->mutex);
   list_del(&buf->head);
            mtx_destroy(&buf->mutex);
      pb_reference(&buf->buffer, NULL);
      }
         static void *
   pb_debug_buffer_map(struct pb_buffer *_buf, 
         {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
   void *map;
               map = pb_map(buf->buffer, flags, flush_ctx);
   if (!map)
            mtx_lock(&buf->mutex);
   ++buf->map_count;
   debug_backtrace_capture(buf->map_backtrace, 1, PB_DEBUG_MAP_BACKTRACE);
   mtx_unlock(&buf->mutex);
         }
         static void
   pb_debug_buffer_unmap(struct pb_buffer *_buf)
   {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);   
      mtx_lock(&buf->mutex);
   assert(buf->map_count);
   if(buf->map_count)
         mtx_unlock(&buf->mutex);
      pb_unmap(buf->buffer);
         }
         static void
   pb_debug_buffer_get_base_buffer(struct pb_buffer *_buf,
               {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
   pb_get_base_buffer(buf->buffer, base_buf, offset);
      }
         static enum pipe_error 
   pb_debug_buffer_validate(struct pb_buffer *_buf, 
               {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
               mtx_lock(&buf->mutex);
   if(buf->map_count) {
      debug_printf("%s: attempting to validate a mapped buffer\n", __func__);
   debug_printf("last map backtrace is\n");
      }
                        }
         static void
   pb_debug_buffer_fence(struct pb_buffer *_buf, 
         {
      struct pb_debug_buffer *buf = pb_debug_buffer(_buf);
      }
         const struct pb_vtbl 
   pb_debug_buffer_vtbl = {
         pb_debug_buffer_destroy,
   pb_debug_buffer_map,
   pb_debug_buffer_unmap,
   pb_debug_buffer_validate,
   pb_debug_buffer_fence,
   };
         static void
   pb_debug_manager_dump_locked(struct pb_debug_manager *mgr)
   {
      struct list_head *curr, *next;
            curr = mgr->list.next;
   next = curr->next;
   while(curr != &mgr->list) {
               debug_printf("buffer = %p\n", (void *) buf);
   debug_printf("    .size = 0x%"PRIx64"\n", buf->base.size);
   debug_backtrace_dump(buf->create_backtrace, PB_DEBUG_CREATE_BACKTRACE);
      curr = next; 
            }
         static struct pb_buffer *
   pb_debug_manager_create_buffer(struct pb_manager *_mgr, 
               {
      struct pb_debug_manager *mgr = pb_debug_manager(_mgr);
   struct pb_debug_buffer *buf;
   struct pb_desc real_desc;
   pb_size real_size;
      assert(size);
            buf = CALLOC_STRUCT(pb_debug_buffer);
   if (!buf)
            real_size = mgr->underflow_size + size + mgr->overflow_size;
   real_desc = *desc;
   real_desc.usage |= PB_USAGE_CPU_WRITE;
            buf->buffer = mgr->provider->create_buffer(mgr->provider, 
               if(!buf->buffer) {
      #if 0
         mtx_lock(&mgr->mutex);
   debug_printf("%s: failed to create buffer\n", __func__);
   if(!list_is_empty(&mgr->list))
         #endif
            }
      assert(pipe_is_referenced(&buf->buffer->reference));
   assert(pb_check_alignment(real_desc.alignment, 1u << buf->buffer->alignment_log2));
   assert(pb_check_usage(real_desc.usage, buf->buffer->usage));
   assert(buf->buffer->size >= real_size);
      pipe_reference_init(&buf->base.reference, 1);
   buf->base.alignment_log2 = util_logbase2(desc->alignment);
   buf->base.usage = desc->usage;
   buf->base.size = size;
      buf->base.vtbl = &pb_debug_buffer_vtbl;
            buf->underflow_size = mgr->underflow_size;
   buf->overflow_size = buf->buffer->size - buf->underflow_size - size;
               pb_debug_buffer_fill(buf);
      (void) mtx_init(&buf->mutex, mtx_plain);
      mtx_lock(&mgr->mutex);
   list_addtail(&buf->head, &mgr->list);
               }
         static void
   pb_debug_manager_flush(struct pb_manager *_mgr)
   {
      struct pb_debug_manager *mgr = pb_debug_manager(_mgr);
   assert(mgr->provider->flush);
   if(mgr->provider->flush)
      }
         static void
   pb_debug_manager_destroy(struct pb_manager *_mgr)
   {
      struct pb_debug_manager *mgr = pb_debug_manager(_mgr);
      mtx_lock(&mgr->mutex);
   if(!list_is_empty(&mgr->list)) {
      debug_printf("%s: unfreed buffers\n", __func__);
      }
   mtx_unlock(&mgr->mutex);
      mtx_destroy(&mgr->mutex);
   mgr->provider->destroy(mgr->provider);
      }
         struct pb_manager *
   pb_debug_manager_create(struct pb_manager *provider, 
         {
               if (!provider)
            mgr = CALLOC_STRUCT(pb_debug_manager);
   if (!mgr)
            mgr->base.destroy = pb_debug_manager_destroy;
   mgr->base.create_buffer = pb_debug_manager_create_buffer;
   mgr->base.flush = pb_debug_manager_flush;
   mgr->provider = provider;
   mgr->underflow_size = underflow_size;
   mgr->overflow_size = overflow_size;
      (void) mtx_init(&mgr->mutex, mtx_plain);
               }
         #else /* !DEBUG */
         struct pb_manager *
   pb_debug_manager_create(struct pb_manager *provider, 
         {
         }
         #endif /* !DEBUG */
