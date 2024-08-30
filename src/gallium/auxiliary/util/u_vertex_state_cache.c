   /*
   * Copyright 2021 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
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
      #include "util/u_vertex_state_cache.h"
   #include "util/u_inlines.h"
   #include "util/hash_table.h"
   #include "util/set.h"
      static uint32_t key_hash(const void *key)
   {
                  }
      static bool key_equals(const void *a, const void *b)
   {
      const struct pipe_vertex_state *sa = a;
               }
      void
   util_vertex_state_cache_init(struct util_vertex_state_cache *cache,
               {
      simple_mtx_init(&cache->lock, mtx_plain);
   cache->set = _mesa_set_create(NULL, key_hash, key_equals);
   cache->create = create;
      }
      void
   util_vertex_state_cache_deinit(struct util_vertex_state_cache *cache)
   {
      if (cache->set) {
      _mesa_set_destroy(cache->set, NULL);
         }
      struct pipe_vertex_state *
   util_vertex_state_cache_get(struct pipe_screen *screen,
                              struct pipe_vertex_buffer *buffer,
      {
               memset(&key, 0, sizeof(key));
   key.input.indexbuf = indexbuf;
   assert(!buffer->is_user_buffer);
   key.input.vbuffer.buffer_offset = buffer->buffer_offset;
   key.input.vbuffer.buffer = buffer->buffer;
   key.input.num_elements = num_elements;
   for (unsigned i = 0; i < num_elements; i++)
                           /* Find the state in the live cache. */
   simple_mtx_lock(&cache->lock);
   struct set_entry *entry = _mesa_set_search_pre_hashed(cache->set, hash, &key);
            /* Return if the state already exists. */
   if (state) {
      /* Increase the refcount. */
   p_atomic_inc(&state->reference.count);
   assert(state->reference.count >= 1);
   simple_mtx_unlock(&cache->lock);
               state = cache->create(screen, buffer, elements, num_elements, indexbuf,
         if (state) {
      assert(key_hash(state) == hash);
               simple_mtx_unlock(&cache->lock);
      }
      void
   util_vertex_state_destroy(struct pipe_screen *screen,
               {
      simple_mtx_lock(&cache->lock);
   /* There could have been a thread race and the cache might have returned
   * the vertex state being destroyed. Check the reference count and do
   * nothing if it's positive.
   */
   if (p_atomic_read(&state->reference.count) <= 0) {
      _mesa_set_remove_key(cache->set, state);
      }
      }
