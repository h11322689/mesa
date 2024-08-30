   /*
   * Copyright © 2018 Intel Corporation
   * Copyright (c) 2021 Lima Project
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sub license,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   */
      #include "compiler/nir/nir.h"
   #include "util/blob.h"
   #include "util/build_id.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
      #include "lima_context.h"
   #include "lima_screen.h"
   #include "lima_disk_cache.h"
      void
   lima_vs_disk_cache_store(struct disk_cache *cache,
               {
      if (!cache)
            cache_key cache_key;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
            blob_write_bytes(&blob, &shader->state, sizeof(shader->state));
   blob_write_bytes(&blob, shader->shader, shader->state.shader_size);
            disk_cache_put(cache, cache_key, blob.data, blob.size, NULL);
      }
      void
   lima_fs_disk_cache_store(struct disk_cache *cache,
               {
      if (!cache)
            cache_key cache_key;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
            blob_write_bytes(&blob, &shader->state, sizeof(shader->state));
            disk_cache_put(cache, cache_key, blob.data, blob.size, NULL);
      }
      struct lima_vs_compiled_shader *
   lima_vs_disk_cache_retrieve(struct disk_cache *cache,
         {
               if (!cache)
            cache_key cache_key;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE)
            if (!buffer)
            shader = rzalloc(NULL, struct lima_vs_compiled_shader);
   if (!shader)
            struct blob_reader blob;
   blob_reader_init(&blob, buffer, size);
   blob_copy_bytes(&blob, &shader->state, sizeof(shader->state));
   shader->shader = rzalloc_size(shader, shader->state.shader_size);
   if (!shader->shader)
         blob_copy_bytes(&blob, shader->shader, shader->state.shader_size);
   shader->constant = rzalloc_size(shader, shader->state.constant_size);
   if (!shader->constant)
               out:
      free(buffer);
         err:
      ralloc_free(shader);
      }
      struct lima_fs_compiled_shader *
   lima_fs_disk_cache_retrieve(struct disk_cache *cache,
         {
               if (!cache)
            cache_key cache_key;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (lima_debug & LIMA_DEBUG_DISK_CACHE)
            if (!buffer)
            shader = rzalloc(NULL, struct lima_fs_compiled_shader);
   if (!shader)
            struct blob_reader blob;
   blob_reader_init(&blob, buffer, size);
   blob_copy_bytes(&blob, &shader->state, sizeof(shader->state));
   shader->shader = rzalloc_size(shader, shader->state.shader_size);
   if (!shader->shader)
               out:
      free(buffer);
         err:
      ralloc_free(shader);
      }
      void
   lima_disk_cache_init(struct lima_screen *screen)
   {
      const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
               }
