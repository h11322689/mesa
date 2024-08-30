   /*
   * Copyright © 2017 Intel Corporation
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
   * @file crocus_program_cache.c
   *
   * The in-memory program cache.  This is basically a hash table mapping
   * API-specified shaders and a state key to a compiled variant.  It also
   * takes care of uploading shader assembly into a BO for use on the GPU.
   */
      #include <stdio.h>
   #include <errno.h>
   #include "pipe/p_defines.h"
   #include "pipe/p_state.h"
   #include "pipe/p_context.h"
   #include "pipe/p_screen.h"
   #include "util/u_atomic.h"
   #include "util/u_upload_mgr.h"
   #include "compiler/nir/nir.h"
   #include "compiler/nir/nir_builder.h"
   #include "intel/compiler/brw_compiler.h"
   #include "intel/compiler/brw_eu.h"
   #include "intel/compiler/brw_nir.h"
   #include "crocus_context.h"
   #include "crocus_resource.h"
      struct keybox {
      uint16_t size;
   enum crocus_program_cache_id cache_id;
      };
      static struct keybox *
   make_keybox(void *mem_ctx, enum crocus_program_cache_id cache_id,
         {
      struct keybox *keybox =
            keybox->cache_id = cache_id;
   keybox->size = key_size;
               }
      static uint32_t
   keybox_hash(const void *void_key)
   {
      const struct keybox *key = void_key;
      }
      static bool
   keybox_equals(const void *void_a, const void *void_b)
   {
      const struct keybox *a = void_a, *b = void_b;
   if (a->size != b->size)
               }
      struct crocus_compiled_shader *
   crocus_find_cached_shader(struct crocus_context *ice,
               {
      struct keybox *keybox = make_keybox(NULL, cache_id, key, key_size);
   struct hash_entry *entry =
                        }
      const void *
   crocus_find_previous_compile(const struct crocus_context *ice,
               {
      hash_table_foreach(ice->shaders.cache, entry) {
      const struct keybox *keybox = entry->key;
   const struct brw_base_prog_key *key = (const void *)keybox->data;
   if (keybox->cache_id == cache_id &&
      key->program_string_id == program_string_id) {
                     }
      /**
   * Look for an existing entry in the cache that has identical assembly code.
   *
   * This is useful for programs generating shaders at runtime, where multiple
   * distinct shaders (from an API perspective) may compile to the same assembly
   * in our backend.  This saves space in the program cache buffer.
   */
   static const struct crocus_compiled_shader *
   find_existing_assembly(struct hash_table *cache, void *map,
         {
      hash_table_foreach (cache, entry) {
               if (existing->map_size != assembly_size)
            if (memcmp(map + existing->offset, assembly, assembly_size) == 0)
      }
      }
      static void
   crocus_cache_new_bo(struct crocus_context *ice,
         {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   struct crocus_bo *new_bo;
            void *map = crocus_bo_map(NULL, new_bo, MAP_READ | MAP_WRITE |
            if (ice->shaders.cache_next_offset != 0) {
                  crocus_bo_unmap(ice->shaders.cache_bo);
   crocus_bo_unreference(ice->shaders.cache_bo);
   ice->shaders.cache_bo = new_bo;
            if (screen->devinfo.ver <= 5) {
      /* reemit all shaders on GEN4 only. */
   ice->state.dirty |= CROCUS_DIRTY_CLIP | CROCUS_DIRTY_RASTER |
            }
   ice->batches[CROCUS_BATCH_RENDER].state_base_address_emitted = false;
   ice->batches[CROCUS_BATCH_COMPUTE].state_base_address_emitted = false;
      }
      static uint32_t
   crocus_alloc_item_data(struct crocus_context *ice, uint32_t size)
   {
      if (ice->shaders.cache_next_offset + size > ice->shaders.cache_bo->size) {
      uint32_t new_size = ice->shaders.cache_bo->size * 2;
   while (ice->shaders.cache_next_offset + size > new_size)
               }
            /* Programs are always 64-byte aligned, so set up the next one now */
   ice->shaders.cache_next_offset = ALIGN(offset + size, 64);
      }
      struct crocus_compiled_shader *
   crocus_upload_shader(struct crocus_context *ice,
                        enum crocus_program_cache_id cache_id, uint32_t key_size,
   const void *key, const void *assembly, uint32_t asm_size,
   struct brw_stage_prog_data *prog_data,
      {
      struct hash_table *cache = ice->shaders.cache;
   struct crocus_compiled_shader *shader =
         const struct crocus_compiled_shader *existing = find_existing_assembly(
            /* If we can find a matching prog in the cache already, then reuse the
   * existing stuff without creating new copy into the underlying buffer
   * object.  This is notably useful for programs generating shaders at
   * runtime, where multiple shaders may compile to the same thing in our
   * backend.
   */
   if (existing) {
      shader->offset = existing->offset;
      } else {
      shader->offset = crocus_alloc_item_data(ice, asm_size);
                        shader->prog_data = prog_data;
   shader->prog_data_size = prog_data_size;
   shader->streamout = streamout;
   shader->system_values = system_values;
   shader->num_system_values = num_system_values;
   shader->num_cbufs = num_cbufs;
            ralloc_steal(shader, shader->prog_data);
   if (prog_data_size > 16)
         ralloc_steal(shader, shader->streamout);
            struct keybox *keybox = make_keybox(shader, cache_id, key, key_size);
               }
      bool
   crocus_blorp_lookup_shader(struct blorp_batch *blorp_batch, const void *key,
               {
      struct blorp_context *blorp = blorp_batch->blorp;
   struct crocus_context *ice = blorp->driver_ctx;
   struct crocus_compiled_shader *shader =
            if (!shader)
            *kernel_out = shader->offset;
               }
      bool
   crocus_blorp_upload_shader(struct blorp_batch *blorp_batch, uint32_t stage,
                                 {
      struct blorp_context *blorp = blorp_batch->blorp;
            struct brw_stage_prog_data *prog_data = ralloc_size(NULL, prog_data_size);
            struct crocus_binding_table bt;
            struct crocus_compiled_shader *shader = crocus_upload_shader(
      ice, CROCUS_CACHE_BLORP, key_size, key, kernel, kernel_size, prog_data,
         *kernel_out = shader->offset;
               }
      void
   crocus_init_program_cache(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
   ice->shaders.cache =
            ice->shaders.cache_bo =
         ice->shaders.cache_bo_map =
      crocus_bo_map(NULL, ice->shaders.cache_bo,
   }
      void
   crocus_destroy_program_cache(struct crocus_context *ice)
   {
      for (int i = 0; i < MESA_SHADER_STAGES; i++) {
                  if (ice->shaders.cache_bo) {
      crocus_bo_unmap(ice->shaders.cache_bo);
   crocus_bo_unreference(ice->shaders.cache_bo);
   ice->shaders.cache_bo_map = NULL;
                  }
      static const char *
   cache_name(enum crocus_program_cache_id cache_id)
   {
      if (cache_id == CROCUS_CACHE_BLORP)
            if (cache_id == CROCUS_CACHE_SF)
            if (cache_id == CROCUS_CACHE_CLIP)
            if (cache_id == CROCUS_CACHE_FF_GS)
               }
      void
   crocus_print_program_cache(struct crocus_context *ice)
   {
      struct crocus_screen *screen = (struct crocus_screen *)ice->ctx.screen;
            hash_table_foreach(ice->shaders.cache, entry) {
      const struct keybox *keybox = entry->key;
   struct crocus_compiled_shader *shader = entry->data;
   fprintf(stderr, "%s:\n", cache_name(keybox->cache_id));
   brw_disassemble(isa, ice->shaders.cache_bo_map + shader->offset, 0,
         }
