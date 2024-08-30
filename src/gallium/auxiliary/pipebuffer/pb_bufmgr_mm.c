   /**************************************************************************
   *
   * Copyright 2006 VMware, Inc.
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
   * Buffer manager using the old texture memory manager.
   * 
   * \author Jose Fonseca <jfonseca@vmware.com>
   */
         #include "pipe/p_defines.h"
   #include "util/u_debug.h"
   #include "util/u_thread.h"
   #include "util/u_memory.h"
   #include "util/list.h"
   #include "util/u_mm.h"
   #include "pb_buffer.h"
   #include "pb_bufmgr.h"
         /**
   * Convenience macro (type safe).
   */
   #define SUPER(__derived) (&(__derived)->base)
         struct mm_pb_manager
   {
      struct pb_manager base;
      mtx_t mutex;
      pb_size size;
   struct mem_block *heap;
      pb_size align2;
      struct pb_buffer *buffer;
      };
         static inline struct mm_pb_manager *
   mm_pb_manager(struct pb_manager *mgr)
   {
      assert(mgr);
      }
         struct mm_buffer
   {
      struct pb_buffer base;
      struct mm_pb_manager *mgr;
         };
         static inline struct mm_buffer *
   mm_buffer(struct pb_buffer *buf)
   {
      assert(buf);
      }
         static void
   mm_buffer_destroy(void *winsys, struct pb_buffer *buf)
   {
      struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
      assert(!pipe_is_referenced(&mm_buf->base.reference));
      mtx_lock(&mm->mutex);
   u_mmFreeMem(mm_buf->block);
   FREE(mm_buf);
      }
         static void *
   mm_buffer_map(struct pb_buffer *buf,
               {
      struct mm_buffer *mm_buf = mm_buffer(buf);
                        }
         static void
   mm_buffer_unmap(struct pb_buffer *buf)
   {
         }
         static enum pipe_error 
   mm_buffer_validate(struct pb_buffer *buf, 
               {
      struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
      }
         static void
   mm_buffer_fence(struct pb_buffer *buf, 
         {
      struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
      }
         static void
   mm_buffer_get_base_buffer(struct pb_buffer *buf,
               {
      struct mm_buffer *mm_buf = mm_buffer(buf);
   struct mm_pb_manager *mm = mm_buf->mgr;
   pb_get_base_buffer(mm->buffer, base_buf, offset);
      }
         static const struct pb_vtbl 
   mm_buffer_vtbl = {
         mm_buffer_destroy,
   mm_buffer_map,
   mm_buffer_unmap,
   mm_buffer_validate,
   mm_buffer_fence,
   };
         static struct pb_buffer *
   mm_bufmgr_create_buffer(struct pb_manager *mgr, 
               {
      struct mm_pb_manager *mm = mm_pb_manager(mgr);
            /* We don't handle alignments larger then the one initially setup */
   assert(pb_check_alignment(desc->alignment, 1u << mm->align2));
   if(!pb_check_alignment(desc->alignment, 1u << mm->align2))
                     mm_buf = CALLOC_STRUCT(mm_buffer);
   if (!mm_buf) {
      mtx_unlock(&mm->mutex);
               pipe_reference_init(&mm_buf->base.reference, 1);
   mm_buf->base.alignment_log2 = util_logbase2(desc->alignment);
   mm_buf->base.usage = desc->usage;
   mm_buf->base.size = size;
      mm_buf->base.vtbl = &mm_buffer_vtbl;
      mm_buf->mgr = mm;
      mm_buf->block = u_mmAllocMem(mm->heap, (int)size, (int)mm->align2, 0);
      #if 0
         debug_printf("warning: heap full\n");
   #endif
         FREE(mm_buf);
   mtx_unlock(&mm->mutex);
      }
      /* Some sanity checks */
   assert(0 <= (pb_size)mm_buf->block->ofs && (pb_size)mm_buf->block->ofs < mm->size);
   assert(size <= (pb_size)mm_buf->block->size && (pb_size)mm_buf->block->ofs + (pb_size)mm_buf->block->size <= mm->size);
      mtx_unlock(&mm->mutex);
      }
         static void
   mm_bufmgr_flush(struct pb_manager *mgr)
   {
         }
         static void
   mm_bufmgr_destroy(struct pb_manager *mgr)
   {
      struct mm_pb_manager *mm = mm_pb_manager(mgr);
               u_mmDestroy(mm->heap);
      pb_unmap(mm->buffer);
   pb_reference(&mm->buffer, NULL);
      mtx_unlock(&mm->mutex);
         }
         struct pb_manager *
   mm_bufmgr_create_from_buffer(struct pb_buffer *buffer, 
         {
               if (!buffer)
            mm = CALLOC_STRUCT(mm_pb_manager);
   if (!mm)
            mm->base.destroy = mm_bufmgr_destroy;
   mm->base.create_buffer = mm_bufmgr_create_buffer;
            mm->size = size;
                              mm->map = pb_map(mm->buffer, 
      PB_USAGE_CPU_READ |
      if(!mm->map)
            mm->heap = u_mmInit(0, (int)size); 
   if (!mm->heap)
            return SUPER(mm);
      failure:
      if(mm->heap)
         if(mm->map)
         FREE(mm);
      }
         struct pb_manager *
   mm_bufmgr_create(struct pb_manager *provider, 
         {
      struct pb_buffer *buffer;
   struct pb_manager *mgr;
            if (!provider)
            memset(&desc, 0, sizeof(desc));
   desc.alignment = 1 << align2;
      buffer = provider->create_buffer(provider, size, &desc); 
   if (!buffer)
            mgr = mm_bufmgr_create_from_buffer(buffer, size, align2);
   if (!mgr) {
      pb_reference(&buffer, NULL);
            return mgr;
   }
