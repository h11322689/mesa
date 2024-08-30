   /*
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
      /**
   * @file iris_disk_cache.c
   *
   * Functions for interacting with the on-disk shader cache.
   */
      #include <stdio.h>
   #include <stdint.h>
   #include <assert.h>
   #include <string.h>
      #include "compiler/nir/nir.h"
   #include "util/blob.h"
   #include "util/build_id.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
      #include "iris_context.h"
      static bool debug = false;
      /**
   * Compute a disk cache key for the given uncompiled shader and NOS key.
   */
   static void
   iris_disk_cache_compute_key(struct disk_cache *cache,
                           {
      /* Create a copy of the program key with program_string_id zeroed out.
   * It's essentially random data which we don't want to include in our
   * hashing and comparisons.  We'll set a proper value on a cache hit.
   */
   union brw_any_prog_key prog_key;
   memcpy(&prog_key, orig_prog_key, prog_key_size);
            uint8_t data[sizeof(prog_key) + sizeof(ish->nir_sha1)];
            memcpy(data, ish->nir_sha1, sizeof(ish->nir_sha1));
               }
      /**
   * Store the given compiled shader in the disk cache.
   *
   * This should only be called on newly compiled shaders.  No checking is
   * done to prevent repeated stores of the same shader.
   */
   void
   iris_disk_cache_store(struct disk_cache *cache,
                           {
   #ifdef ENABLE_SHADER_CACHE
      if (!cache)
            gl_shader_stage stage = ish->nir->info.stage;
            cache_key cache_key;
            if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               struct blob blob;
            /* We write the following data to the cache blob:
   *
   * 1. Prog data (must come first because it has the assembly size)
   * 2. Assembly code
   * 3. Number of entries in the system value array
   * 4. System value array
   * 5. Size (in bytes) of kernel inputs
   * 6. Shader relocations
   * 7. Legacy param array (only used for compute workgroup ID)
   * 8. Binding table
   */
   blob_write_bytes(&blob, shader->prog_data, brw_prog_data_size(stage));
   blob_write_bytes(&blob, shader->map, shader->prog_data->program_size);
   blob_write_uint32(&blob, shader->num_system_values);
   blob_write_bytes(&blob, shader->system_values,
         blob_write_uint32(&blob, shader->kernel_input_size);
   blob_write_bytes(&blob, prog_data->relocs,
         blob_write_bytes(&blob, prog_data->param,
                  disk_cache_put(cache, cache_key, blob.data, blob.size, NULL);
      #endif
   }
      static const enum iris_program_cache_id cache_id_for_stage[] = {
      [MESA_SHADER_VERTEX]    = IRIS_CACHE_VS,
   [MESA_SHADER_TESS_CTRL] = IRIS_CACHE_TCS,
   [MESA_SHADER_TESS_EVAL] = IRIS_CACHE_TES,
   [MESA_SHADER_GEOMETRY]  = IRIS_CACHE_GS,
   [MESA_SHADER_FRAGMENT]  = IRIS_CACHE_FS,
      };
      /**
   * Search for a compiled shader in the disk cache.  If found, upload it
   * to the in-memory program cache so we can use it.
   */
   bool
   iris_disk_cache_retrieve(struct iris_screen *screen,
                           struct u_upload_mgr *uploader,
   {
   #ifdef ENABLE_SHADER_CACHE
      struct disk_cache *cache = screen->disk_cache;
            if (!cache)
            cache_key cache_key;
            if (debug) {
      char sha1[41];
   _mesa_sha1_format(sha1, cache_key);
               size_t size;
            if (debug)
            if (!buffer)
                     struct brw_stage_prog_data *prog_data = ralloc_size(NULL, prog_data_size);
   const void *assembly;
   uint32_t num_system_values;
   uint32_t kernel_input_size;
   uint32_t *system_values = NULL;
            struct blob_reader blob;
   blob_reader_init(&blob, buffer, size);
   blob_copy_bytes(&blob, prog_data, prog_data_size);
   assembly = blob_read_bytes(&blob, prog_data->program_size);
   num_system_values = blob_read_uint32(&blob);
   if (num_system_values) {
      system_values =
         blob_copy_bytes(&blob, system_values,
                        prog_data->relocs = NULL;
   if (prog_data->num_relocs) {
      struct brw_shader_reloc *relocs =
         blob_copy_bytes(&blob, relocs,
                     prog_data->param = NULL;
   if (prog_data->nr_params) {
      prog_data->param = ralloc_array(NULL, uint32_t, prog_data->nr_params);
   blob_copy_bytes(&blob, prog_data->param,
               struct iris_binding_table bt;
            if (stage == MESA_SHADER_VERTEX ||
      stage == MESA_SHADER_TESS_EVAL ||
   stage == MESA_SHADER_GEOMETRY) {
   struct brw_vue_prog_data *vue_prog_data = (void *) prog_data;
   so_decls = screen->vtbl.create_so_decl_list(&ish->stream_output,
               /* System values and uniforms are stored in constant buffer 0, the
   * user-facing UBOs are indexed by one.  So if any constant buffer is
   * needed, the constant buffer 0 will be needed, so account for it.
   */
            if (num_cbufs || ish->nir->num_uniforms)
            if (num_system_values || kernel_input_size)
            iris_finalize_program(shader, prog_data, so_decls, system_values,
                  assert(stage < ARRAY_SIZE(cache_id_for_stage));
            /* Upload our newly read shader to the in-memory program cache. */
   iris_upload_shader(screen, ish, shader, NULL, uploader,
                        #else
         #endif
   }
      /**
   * Initialize the on-disk shader cache.
   */
   void
   iris_disk_cache_init(struct iris_screen *screen)
   {
   #ifdef ENABLE_SHADER_CACHE
      if (INTEL_DEBUG(DEBUG_DISK_CACHE_DISABLE_MASK))
            /* array length = print length + nul char + 1 extra to verify it's unused */
   char renderer[11];
   UNUSED int len =
                  const struct build_id_note *note =
                  const uint8_t *id_sha1 = build_id_data(note);
            char timestamp[41];
            const uint64_t driver_flags =
            #endif
   }
