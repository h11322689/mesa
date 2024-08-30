   /*
   * Copyright 2023 Rose Hudson
   * Copyright 2022 Amazon.com, Inc. or its affiliates.
   * Copyright 2018 Intel Corporation
   * SPDX-License-Identifier: MIT
   */
      #include <assert.h>
   #include <stdio.h>
      #include "asahi/compiler/agx_debug.h"
   #include "compiler/shader_enums.h"
   #include "util/blob.h"
   #include "util/build_id.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
   #include "agx_bo.h"
   #include "agx_disk_cache.h"
   #include "agx_state.h"
      /* Flags that are allowed and do not disable the disk cache */
   #define ALLOWED_FLAGS (AGX_DBG_NO16)
      /**
   * Compute a disk cache key for the given uncompiled shader and shader key.
   */
   static void
   agx_disk_cache_compute_key(struct disk_cache *cache,
                     {
      uint8_t data[sizeof(uncompiled->nir_sha1) + sizeof(*shader_key)];
   int hash_size = sizeof(uncompiled->nir_sha1);
   int key_size;
   if (uncompiled->type == PIPE_SHADER_VERTEX)
         else if (uncompiled->type == PIPE_SHADER_FRAGMENT)
         else if (uncompiled->type == PIPE_SHADER_COMPUTE)
         else
                     if (key_size)
               }
      /**
   * Store the given compiled shader in the disk cache.
   *
   * This should only be called on newly compiled shaders.  No checking is
   * done to prevent repeated stores of the same shader.
   */
   void
   agx_disk_cache_store(struct disk_cache *cache,
                     {
   #ifdef ENABLE_SHADER_CACHE
      if (!cache)
                     cache_key cache_key;
            struct blob blob;
            uint32_t shader_size = binary->bo->size;
   blob_write_uint32(&blob, shader_size);
   blob_write_bytes(&blob, binary->bo->ptr.cpu, shader_size);
   blob_write_bytes(&blob, &binary->info, sizeof(binary->info));
   blob_write_uint32(&blob, binary->push_range_count);
   blob_write_bytes(&blob, binary->push,
            disk_cache_put(cache, cache_key, blob.data, blob.size, NULL);
      #endif
   }
      /**
   * Search for a compiled shader in the disk cache.
   */
   struct agx_compiled_shader *
   agx_disk_cache_retrieve(struct agx_screen *screen,
               {
   #ifdef ENABLE_SHADER_CACHE
      struct disk_cache *cache = screen->disk_cache;
   if (!cache)
            cache_key cache_key;
            size_t size;
   void *buffer = disk_cache_get(cache, cache_key, &size);
   if (!buffer)
                     struct blob_reader blob;
            uint32_t binary_size = blob_read_uint32(&blob);
   binary->bo = agx_bo_create(&screen->dev, binary_size,
                  blob_copy_bytes(&blob, &binary->info, sizeof(binary->info));
   binary->push_range_count = blob_read_uint32(&blob);
   blob_copy_bytes(&blob, binary->push,
            free(buffer);
      #else
         #endif
   }
      /**
   * Initialise the on-disk shader cache.
   */
   void
   agx_disk_cache_init(struct agx_screen *screen)
   {
   #ifdef ENABLE_SHADER_CACHE
      if (agx_get_compiler_debug() || (screen->dev.debug & ~ALLOWED_FLAGS))
                     const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
            uint64_t driver_flags = screen->dev.debug;
      #endif
   }
