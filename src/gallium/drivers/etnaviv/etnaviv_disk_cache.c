   /*
   * Copyright © 2020 Google, Inc.
   * Copyright (c) 2020 Etnaviv Project
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
   * Authors:
   *    Christian Gmeiner <christian.gmeiner@gmail.com>
   */
      #include "etnaviv_debug.h"
   #include "etnaviv_disk_cache.h"
   #include "nir_serialize.h"
      #define debug 0
      void
   etna_disk_cache_init(struct etna_compiler *compiler, const char *renderer)
   {
      if (DBG_ENABLED(ETNA_DBG_NOCACHE))
            const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
               }
      void
   etna_disk_cache_init_shader_key(struct etna_compiler *compiler, struct etna_shader *shader)
   {
      if (!compiler->disk_cache)
                              /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache.  Drop unnecessary information (like variable names)
   * so the serialized NIR is smaller, and also to let us detect more
   * isomorphic shaders when hashing, increasing cache hits.
   */
            blob_init(&blob);
   nir_serialize(&blob, shader->nir, true);
   _mesa_sha1_update(&ctx, blob.data, blob.size);
               }
      static void
   compute_variant_key(struct etna_compiler *compiler, struct etna_shader_variant *v,
         {
                        blob_write_bytes(&blob, &v->shader->cache_key, sizeof(v->shader->cache_key));
                        }
      static void
   retrieve_variant(struct blob_reader *blob, struct etna_shader_variant *v)
   {
               v->code = malloc(4 * v->code_size);
            blob_copy_bytes(blob, &v->uniforms.count, sizeof(v->uniforms.count));
   v->uniforms.contents = malloc(v->uniforms.count * sizeof(*v->uniforms.contents));
            blob_copy_bytes(blob, v->uniforms.contents, v->uniforms.count * sizeof(*v->uniforms.contents));
      }
      static void
   store_variant(struct blob *blob, const struct etna_shader_variant *v)
   {
               blob_write_bytes(blob, VARIANT_CACHE_PTR(v), VARIANT_CACHE_SIZE);
            blob_write_bytes(blob, &v->uniforms.count, sizeof(v->uniforms.count));
   blob_write_bytes(blob, v->uniforms.contents, imm_count * sizeof(*v->uniforms.contents));
      }
      bool
   etna_disk_cache_retrieve(struct etna_compiler *compiler, struct etna_shader_variant *v)
   {
      if (!compiler->disk_cache)
                              if (debug) {
               _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (debug)
            if (!buffer)
            struct blob_reader blob;
                                 }
      void
   etna_disk_cache_store(struct etna_compiler *compiler, struct etna_shader_variant *v)
   {
      if (!compiler->disk_cache)
                              if (debug) {
               _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
                     disk_cache_put(compiler->disk_cache, cache_key, blob.data, blob.size, NULL);
      }
