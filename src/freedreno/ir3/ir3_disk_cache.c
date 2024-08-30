   /*
   * Copyright © 2020 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "nir_serialize.h"
      #include "ir3_compiler.h"
   #include "ir3_nir.h"
      #define debug 0
      /*
   * Shader disk-cache implementation.
   *
   * Note that at least in the EGL_ANDROID_blob_cache, we should never
   * rely on inter-dependencies between different cache entries:
   *
   *    No guarantees are made as to whether a given key/value pair is present in
   *    the cache after the set call.  If a different value has been associated
   *    with the given key in the past then it is undefined which value, if any,
   *    is associated with the key after the set call.  Note that while there are
   *    no guarantees, the cache implementation should attempt to cache the most
   *    recently set value for a given key.
   *
   * for this reason, because binning pass variants share const_state with
   * their draw-pass counterpart, both variants are serialized together.
   */
      void
   ir3_disk_cache_init(struct ir3_compiler *compiler)
   {
      if (ir3_shader_debug & IR3_DBG_NOCACHE)
            const char *renderer = fd_dev_name(compiler->dev_id);
   const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
            uint64_t driver_flags = ir3_shader_debug;
   if (compiler->options.robust_buffer_access2)
            }
      void
   ir3_disk_cache_init_shader_key(struct ir3_compiler *compiler,
         {
      if (!compiler->disk_cache)
                              /* Serialize the NIR to a binary blob that we can hash for the disk
   * cache.  Drop unnecessary information (like variable names)
   * so the serialized NIR is smaller, and also to let us detect more
   * isomorphic shaders when hashing, increasing cache hits.
   */
   struct blob blob;
   blob_init(&blob);
   nir_serialize(&blob, shader->nir, true);
   _mesa_sha1_update(&ctx, blob.data, blob.size);
            _mesa_sha1_update(&ctx, &shader->options.api_wavesize,
         _mesa_sha1_update(&ctx, &shader->options.real_wavesize,
            /* Note that on some gens stream-out is lowered in ir3 to stg.  For later
   * gens we maybe don't need to include stream-out in the cache key.
   */
   _mesa_sha1_update(&ctx, &shader->stream_output,
               }
      static void
   compute_variant_key(struct ir3_shader *shader, struct ir3_shader_variant *v,
         {
      struct blob blob;
            blob_write_bytes(&blob, &shader->cache_key, sizeof(shader->cache_key));
   blob_write_bytes(&blob, &v->key, sizeof(v->key));
            disk_cache_compute_key(shader->compiler->disk_cache, blob.data, blob.size,
               }
      static void
   retrieve_variant(struct blob_reader *blob, struct ir3_shader_variant *v)
   {
               /*
   * pointers need special handling:
            v->bin = rzalloc_size(v, v->info.size);
            if (!v->binning_pass) {
      blob_copy_bytes(blob, v->const_state, sizeof(*v->const_state));
   unsigned immeds_sz = v->const_state->immediates_size *
         v->const_state->immediates = ralloc_size(v->const_state, immeds_sz);
         }
      static void
   store_variant(struct blob *blob, const struct ir3_shader_variant *v)
   {
               /*
   * pointers need special handling:
                              if (!v->binning_pass) {
      blob_write_bytes(blob, v->const_state, sizeof(*v->const_state));
   unsigned immeds_sz = v->const_state->immediates_size *
               }
      struct ir3_shader_variant *
   ir3_retrieve_variant(struct blob_reader *blob, struct ir3_compiler *compiler,
         {
               v->id = 0;
   v->compiler = compiler;
   v->binning_pass = false;
   v->nonbinning = NULL;
   v->binning = NULL;
   blob_copy_bytes(blob, &v->key, sizeof(v->key));
   v->type = blob_read_uint32(blob);
   v->mergedregs = blob_read_uint32(blob);
                     if (v->type == MESA_SHADER_VERTEX && ir3_has_binning_vs(&v->key)) {
      v->binning = rzalloc_size(v, sizeof(*v->binning));
   v->binning->id = 0;
   v->binning->compiler = compiler;
   v->binning->binning_pass = true;
   v->binning->nonbinning = v;
   v->binning->key = v->key;
   v->binning->type = MESA_SHADER_VERTEX;
   v->binning->mergedregs = v->mergedregs;
               }
         }
      void
   ir3_store_variant(struct blob *blob, const struct ir3_shader_variant *v)
   {
      blob_write_bytes(blob, &v->key, sizeof(v->key));
   blob_write_uint32(blob, v->type);
                     if (v->type == MESA_SHADER_VERTEX && ir3_has_binning_vs(&v->key)) {
            }
      bool
   ir3_disk_cache_retrieve(struct ir3_shader *shader,
         {
      if (!shader->compiler->disk_cache)
                              if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (debug)
            if (!buffer)
            struct blob_reader blob;
                     if (v->binning)
                        }
      void
   ir3_disk_cache_store(struct ir3_shader *shader,
         {
      if (!shader->compiler->disk_cache)
                              if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
                     if (v->binning)
            disk_cache_put(shader->compiler->disk_cache, cache_key, blob.data, blob.size, NULL);
      }
