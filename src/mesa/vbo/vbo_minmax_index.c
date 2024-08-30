   /*
   * Mesa 3-D graphics library
   *
   * Copyright 2003 VMware, Inc.
   * Copyright 2009 VMware, Inc.
   * All Rights Reserved.
   * Copyright (C) 2016 Advanced Micro Devices, Inc.
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "util/glheader.h"
   #include "util/u_cpu_detect.h"
   #include "main/context.h"
   #include "main/varray.h"
   #include "main/macros.h"
   #include "main/sse_minmax.h"
   #include "util/hash_table.h"
   #include "util/u_memory.h"
   #include "pipe/p_state.h"
      struct minmax_cache_key {
      GLintptr offset;
   GLuint count;
      };
         struct minmax_cache_entry {
      struct minmax_cache_key key;
   GLuint min;
      };
         static uint32_t
   vbo_minmax_cache_hash(const struct minmax_cache_key *key)
   {
         }
         static bool
   vbo_minmax_cache_key_equal(const struct minmax_cache_key *a,
         {
      return (a->offset == b->offset) && (a->count == b->count) &&
      }
         static void
   vbo_minmax_cache_delete_entry(struct hash_entry *entry)
   {
         }
         static GLboolean
   vbo_use_minmax_cache(struct gl_buffer_object *bufferObj)
   {
      if (bufferObj->UsageHistory & (USAGE_TEXTURE_BUFFER |
                                          if ((bufferObj->Mappings[MAP_USER].AccessFlags &
      (GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT)) ==
   (GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT))
            }
         void
   vbo_delete_minmax_cache(struct gl_buffer_object *bufferObj)
   {
      _mesa_hash_table_destroy(bufferObj->MinMaxCache, vbo_minmax_cache_delete_entry);
      }
         static GLboolean
   vbo_get_minmax_cached(struct gl_buffer_object *bufferObj,
               {
      GLboolean found = GL_FALSE;
   struct minmax_cache_key key;
   uint32_t hash;
            if (!bufferObj->MinMaxCache)
         if (!vbo_use_minmax_cache(bufferObj))
                     if (bufferObj->MinMaxCacheDirty) {
      /* Disable the cache permanently for this BO if the number of hits
   * is asymptotically less than the number of misses. This happens when
   * applications use the BO for streaming.
   *
   * However, some initial optimism allows applications that interleave
   * draw calls with glBufferSubData during warmup.
   */
   unsigned optimism = bufferObj->Size;
   if (bufferObj->MinMaxCacheMissIndices > optimism &&
      bufferObj->MinMaxCacheHitIndices < bufferObj->MinMaxCacheMissIndices - optimism) {
   bufferObj->UsageHistory |= USAGE_DISABLE_MINMAX_CACHE;
   vbo_delete_minmax_cache(bufferObj);
               _mesa_hash_table_clear(bufferObj->MinMaxCache, vbo_minmax_cache_delete_entry);
   bufferObj->MinMaxCacheDirty = false;
               key.index_size = index_size;
   key.offset = offset;
   key.count = count;
   hash = vbo_minmax_cache_hash(&key);
   result = _mesa_hash_table_search_pre_hashed(bufferObj->MinMaxCache, hash, &key);
   if (result) {
      struct minmax_cache_entry *entry = result->data;
   *min_index = entry->min;
   *max_index = entry->max;
            out_invalidate:
      if (found) {
      /* The hit counter saturates so that we don't accidently disable the
   * cache in a long-running program.
   */
            if (new_hit_count >= bufferObj->MinMaxCacheHitIndices)
         else
      } else {
               out_disable:
      simple_mtx_unlock(&bufferObj->MinMaxCacheMutex);
      }
         static void
   vbo_minmax_cache_store(struct gl_context *ctx,
                     {
      struct minmax_cache_entry *entry;
   struct hash_entry *table_entry;
            if (!vbo_use_minmax_cache(bufferObj))
                     if (!bufferObj->MinMaxCache) {
      bufferObj->MinMaxCache =
      _mesa_hash_table_create(NULL,
            if (!bufferObj->MinMaxCache)
               entry = MALLOC_STRUCT(minmax_cache_entry);
   if (!entry)
            entry->key.offset = offset;
   entry->key.count = count;
   entry->key.index_size = index_size;
   entry->min = min;
   entry->max = max;
            table_entry = _mesa_hash_table_search_pre_hashed(bufferObj->MinMaxCache,
         if (table_entry) {
      /* It seems like this could happen when two contexts are rendering using
   * the same buffer object from multiple threads.
   */
   _mesa_debug(ctx, "duplicate entry in minmax cache\n");
   free(entry);
               table_entry = _mesa_hash_table_insert_pre_hashed(bufferObj->MinMaxCache,
         if (!table_entry)
         out:
         }
         void
   vbo_get_minmax_index_mapped(unsigned count, unsigned index_size,
                     {
      switch (index_size) {
   case 4: {
      const GLuint *ui_indices = (const GLuint *)indices;
   GLuint max_ui = 0;
   GLuint min_ui = ~0U;
   if (restart) {
      for (unsigned i = 0; i < count; i++) {
      if (ui_indices[i] != restartIndex) {
      if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            }
   #if defined(USE_SSE41)
            if (util_get_cpu_caps()->has_sse4_1) {
            #endif
               for (unsigned i = 0; i < count; i++) {
      if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
      }
   *min_index = min_ui;
   *max_index = max_ui;
      }
   case 2: {
      const GLushort *us_indices = (const GLushort *)indices;
   GLuint max_us = 0;
   GLuint min_us = ~0U;
   if (restart) {
      for (unsigned i = 0; i < count; i++) {
      if (us_indices[i] != restartIndex) {
      if (us_indices[i] > max_us) max_us = us_indices[i];
            }
   else {
      for (unsigned i = 0; i < count; i++) {
      if (us_indices[i] > max_us) max_us = us_indices[i];
         }
   *min_index = min_us;
   *max_index = max_us;
      }
   case 1: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
   GLuint max_ub = 0;
   GLuint min_ub = ~0U;
   if (restart) {
      for (unsigned i = 0; i < count; i++) {
      if (ub_indices[i] != restartIndex) {
      if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            }
   else {
      for (unsigned i = 0; i < count; i++) {
      if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
         }
   *min_index = min_ub;
   *max_index = max_ub;
      }
   default:
            }
         /**
   * Compute min and max elements by scanning the index buffer for
   * glDraw[Range]Elements() calls.
   * If primitive restart is enabled, we need to ignore restart
   * indexes when computing min/max.
   */
   void
   vbo_get_minmax_index(struct gl_context *ctx, struct gl_buffer_object *obj,
                           {
               if (!obj) {
         } else {
               if (vbo_get_minmax_cached(obj, index_size, offset, count, min_index,
                  indices = _mesa_bufferobj_map_range(ctx, offset, size, GL_MAP_READ_BIT,
               vbo_get_minmax_index_mapped(count, index_size, restart_index,
                  if (obj) {
      vbo_minmax_cache_store(ctx, obj, index_size, offset, count, *min_index,
               }
      /**
   * Same as vbo_get_minmax_index, but using gallium draw structures.
   */
   bool
   vbo_get_minmax_indices_gallium(struct gl_context *ctx,
                     {
      info->min_index = ~0;
            struct gl_buffer_object *buf =
            for (unsigned i = 0; i < num_draws; i++) {
               /* Do combination if possible to reduce map/unmap count */
   while ((i + 1 < num_draws) &&
            draw.count += draws[i+1].count;
               if (!draw.count)
            unsigned tmp_min, tmp_max;
   vbo_get_minmax_index(ctx, buf,
                        info->index.user,
      info->min_index = MIN2(info->min_index, tmp_min);
                  }
