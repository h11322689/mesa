   /**************************************************************************
   *
   * Copyright 2012 VMware, Inc.
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
   * @file
   * u_debug_flush.c Debug flush and map-related issues:
   * - Flush while synchronously mapped.
   * - Command stream reference while synchronously mapped.
   * - Synchronous map while referenced on command stream.
   * - Recursive maps.
   * - Unmap while not mapped.
   *
   * @author Thomas Hellstrom <thellstrom@vmware.com>
   */
      #ifdef DEBUG
   #include "util/compiler.h"
   #include "util/simple_mtx.h"
   #include "util/u_debug_stack.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_debug_flush.h"
   #include "util/u_hash_table.h"
   #include "util/list.h"
   #include "util/u_inlines.h"
   #include "util/u_string.h"
   #include "util/u_thread.h"
   #include <stdio.h>
      /* Future improvement: Use realloc instead? */
   #define DEBUG_FLUSH_MAP_DEPTH 64
      struct debug_map_item {
      struct debug_stack_frame *frame;
      };
      struct debug_flush_buf {
      /* Atomic */
   struct pipe_reference reference; /* Must be the first member. */
   mtx_t mutex;
   /* Immutable */
   bool supports_persistent;
   unsigned bt_depth;
   /* Protected by mutex */
   int map_count;
   bool has_sync_map;
   int last_sync_map;
      };
      struct debug_flush_item {
      struct debug_flush_buf *fbuf;
   unsigned bt_depth;
      };
      struct debug_flush_ctx {
      /* Contexts are used by a single thread at a time */
   unsigned bt_depth;
   bool catch_map_of_referenced;
   struct hash_table *ref_hash;
      };
      static simple_mtx_t list_mutex = SIMPLE_MTX_INITIALIZER;
   static struct list_head ctx_list = {&ctx_list, &ctx_list};
      static struct debug_stack_frame *
   debug_flush_capture_frame(int start, int depth)
   {
               frames = CALLOC(depth, sizeof(*frames));
   if (!frames)
            debug_backtrace_capture(frames, start, depth);
      }
      struct debug_flush_buf *
   debug_flush_buf_create(bool supports_persistent, unsigned bt_depth)
   {
               if (!fbuf)
            fbuf->supports_persistent = supports_persistent;
   fbuf->bt_depth = bt_depth;
   pipe_reference_init(&fbuf->reference, 1);
               out_no_buf:
      debug_printf("Debug flush buffer creation failed.\n");
   debug_printf("Debug flush checking for this buffer will be incomplete.\n");
      }
      void
   debug_flush_buf_reference(struct debug_flush_buf **dst,
         {
               if (pipe_reference(&(*dst)->reference, &src->reference)) {
               for (i = 0; i < fbuf->map_count; ++i) {
         }
                  }
      static void
   debug_flush_item_destroy(struct debug_flush_item *item)
   {
                           }
      struct debug_flush_ctx *
   debug_flush_ctx_create(UNUSED bool catch_reference_of_mapped,
         {
               if (!fctx)
                     if (!fctx->ref_hash)
            fctx->bt_depth = bt_depth;
   simple_mtx_lock(&list_mutex);
   list_addtail(&fctx->head, &ctx_list);
                  out_no_ref_hash:
         out_no_ctx:
      debug_printf("Debug flush context creation failed.\n");
   debug_printf("Debug flush checking for this context will be incomplete.\n");
      }
      static void
   debug_flush_alert(const char *s, const char *op,
                     unsigned start, unsigned depth,
   {
      if (capture)
            if (s)
         if (frame) {
      debug_printf("%s backtrace follows:\n", op);
      } else
            if (continued)
         else
            if (capture)
      }
         void
   debug_flush_map(struct debug_flush_buf *fbuf, unsigned flags)
   {
               if (!fbuf)
            mtx_lock(&fbuf->mutex);
   map_sync = !(flags & PIPE_MAP_UNSYNCHRONIZED);
   persistent = !map_sync || fbuf->supports_persistent ||
            /* Recursive maps are allowed if previous maps are persistent,
   * or if the current map is unsync. In other cases we might flush
   * with unpersistent maps.
   */
   if (fbuf->has_sync_map && !map_sync) {
      debug_flush_alert("Recursive sync map detected.", "Map",
         debug_flush_alert(NULL, "Previous map", 0, fbuf->bt_depth, false,
               fbuf->maps[fbuf->map_count].frame =
         fbuf->maps[fbuf->map_count].persistent = persistent;
   if (!persistent) {
      fbuf->has_sync_map = true;
               fbuf->map_count++;
                     if (!persistent) {
               simple_mtx_lock(&list_mutex);
   LIST_FOR_EACH_ENTRY(fctx, &ctx_list, head) {
                     if (item && fctx->catch_map_of_referenced) {
      debug_flush_alert("Already referenced map detected.",
         debug_flush_alert(NULL, "Reference", 0, item->bt_depth,
         }
         }
      void
   debug_flush_unmap(struct debug_flush_buf *fbuf)
   {
      if (!fbuf)
            mtx_lock(&fbuf->mutex);
   if (--fbuf->map_count < 0) {
      debug_flush_alert("Unmap not previously mapped detected.", "Map",
      } else {
      if (fbuf->has_sync_map && fbuf->last_sync_map == fbuf->map_count) {
               fbuf->has_sync_map = false;
   while (i-- && !fbuf->has_sync_map) {
      if (!fbuf->maps[i].persistent) {
      fbuf->has_sync_map = true;
         }
   FREE(fbuf->maps[fbuf->map_count].frame);
         }
      }
         /**
   * Add the given buffer to the list of active buffers.  Active buffers
   * are those which are referenced by the command buffer currently being
   * constructed.
   */
   void
   debug_flush_cb_reference(struct debug_flush_ctx *fctx,
         {
               if (!fctx || !fbuf)
                     mtx_lock(&fbuf->mutex);
   if (fbuf->map_count && fbuf->has_sync_map) {
      debug_flush_alert("Reference of mapped buffer detected.", "Reference",
         debug_flush_alert(NULL, "Map", 0, fbuf->bt_depth, false,
      }
            if (!item) {
      item = CALLOC_STRUCT(debug_flush_item);
   if (item) {
      debug_flush_buf_reference(&item->fbuf, fbuf);
   item->bt_depth = fctx->bt_depth;
   item->ref_frame = debug_flush_capture_frame(2, item->bt_depth);
   _mesa_hash_table_insert(fctx->ref_hash, fbuf, item);
      }
      }
         out_no_item:
      debug_printf("Debug flush command buffer reference creation failed.\n");
   debug_printf("Debug flush checking will be incomplete "
      }
      static int
   debug_flush_might_flush_cb(UNUSED void *key, void *value, void *data)
   {
      struct debug_flush_item *item =
                  mtx_lock(&fbuf->mutex);
   if (fbuf->map_count && fbuf->has_sync_map) {
      const char *reason = (const char *) data;
            snprintf(message, sizeof(message),
            debug_flush_alert(message, reason, 3, item->bt_depth, true, true, NULL);
   debug_flush_alert(NULL, "Map", 0, fbuf->bt_depth, true, false,
         debug_flush_alert(NULL, "First reference", 0, item->bt_depth, false,
      }
               }
      /**
   * Called when we're about to possibly flush a command buffer.
   * We check if any active buffers are in a mapped state.  If so, print an alert.
   */
   void
   debug_flush_might_flush(struct debug_flush_ctx *fctx)
   {
      if (!fctx)
            util_hash_table_foreach(fctx->ref_hash,
            }
      static int
   debug_flush_flush_cb(UNUSED void *key, void *value, UNUSED void *data)
   {
      struct debug_flush_item *item =
                        }
         /**
   * Called when we flush a command buffer.  Two things are done:
   * 1. Check if any of the active buffers are currently mapped (alert if so).
   * 2. Discard/unreference all the active buffers.
   */
   void
   debug_flush_flush(struct debug_flush_ctx *fctx)
   {
      if (!fctx)
            util_hash_table_foreach(fctx->ref_hash,
               util_hash_table_foreach(fctx->ref_hash,
                  }
      void
   debug_flush_ctx_destroy(struct debug_flush_ctx *fctx)
   {
      if (!fctx)
            list_del(&fctx->head);
   util_hash_table_foreach(fctx->ref_hash,
               _mesa_hash_table_clear(fctx->ref_hash, NULL);
   _mesa_hash_table_destroy(fctx->ref_hash, NULL);
      }
   #endif
