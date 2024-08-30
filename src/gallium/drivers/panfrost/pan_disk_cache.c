   /*
   * Copyright (c) 2022 Amazon.com, Inc. or its affiliates.
   * Copyright © 2018 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdint.h>
   #include <stdio.h>
   #include <string.h>
      #include "compiler/nir/nir.h"
   #include "util/blob.h"
   #include "util/build_id.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
      #include "pan_context.h"
      static bool debug = false;
      extern int midgard_debug;
   extern int bifrost_debug;
      /**
   * Compute a disk cache key for the given uncompiled shader and shader key.
   */
   static void
   panfrost_disk_cache_compute_key(
      struct disk_cache *cache,
   const struct panfrost_uncompiled_shader *uncompiled,
      {
               memcpy(data, uncompiled->nir_sha1, sizeof(uncompiled->nir_sha1));
               }
      /**
   * Store the given compiled shader in the disk cache.
   *
   * This should only be called on newly compiled shaders.  No checking is
   * done to prevent repeated stores of the same shader.
   */
   void
   panfrost_disk_cache_store(struct disk_cache *cache,
                     {
   #ifdef ENABLE_SHADER_CACHE
      if (!cache)
            cache_key cache_key;
            if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
            /* We write the following data to the cache blob:
   *
   * 1. Size of program binary
   * 2. Program binary
   * 3. Shader info
   * 4. System values
   */
   blob_write_uint32(&blob, binary->binary.size);
   blob_write_bytes(&blob, binary->binary.data, binary->binary.size);
   blob_write_bytes(&blob, &binary->info, sizeof(binary->info));
            disk_cache_put(cache, cache_key, blob.data, blob.size, NULL);
      #endif
   }
      /**
   * Search for a compiled shader in the disk cache.
   */
   bool
   panfrost_disk_cache_retrieve(struct disk_cache *cache,
                     {
   #ifdef ENABLE_SHADER_CACHE
      if (!cache)
            cache_key cache_key;
            if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (debug)
            if (!buffer)
            struct blob_reader blob;
                     uint32_t binary_size = blob_read_uint32(&blob);
            blob_copy_bytes(&blob, ptr, binary_size);
   blob_copy_bytes(&blob, &binary->info, sizeof(binary->info));
                        #else
         #endif
   }
      /**
   * Initialize the on-disk shader cache.
   */
   void
   panfrost_disk_cache_init(struct panfrost_screen *screen)
   {
   #ifdef ENABLE_SHADER_CACHE
               const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
            /* Consider any flags affecting the compile when caching */
   uint64_t driver_flags = screen->dev.debug;
               #endif
   }
