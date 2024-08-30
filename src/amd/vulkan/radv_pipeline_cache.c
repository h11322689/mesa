   /*
   * Copyright © 2015 Intel Corporation
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "util/disk_cache.h"
   #include "util/macros.h"
   #include "util/mesa-blake3.h"
   #include "util/mesa-sha1.h"
   #include "util/u_atomic.h"
   #include "util/u_debug.h"
   #include "vulkan/util/vk_util.h"
   #include "aco_interface.h"
   #include "nir_serialize.h"
   #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "vk_pipeline.h"
      static bool
   radv_is_cache_disabled(struct radv_device *device)
   {
      /* Pipeline caches can be disabled with RADV_DEBUG=nocache, with MESA_GLSL_CACHE_DISABLE=1 and
   * when ACO_DEBUG is used. MESA_GLSL_CACHE_DISABLE is done elsewhere.
   */
   return (device->instance->debug_flags & RADV_DEBUG_NO_CACHE) ||
      }
      void
   radv_hash_shaders(unsigned char *hash, const struct radv_shader_stage *stages, uint32_t stage_count,
         {
               _mesa_sha1_init(&ctx);
   if (key)
         if (layout)
            for (unsigned s = 0; s < stage_count; s++) {
      if (!stages[s].entrypoint)
               }
   _mesa_sha1_update(&ctx, &flags, 4);
      }
      void
   radv_hash_rt_stages(struct mesa_sha1 *ctx, const VkPipelineShaderStageCreateInfo *stages, unsigned stage_count)
   {
      for (unsigned i = 0; i < stage_count; ++i) {
      unsigned char hash[20];
   vk_pipeline_hash_shader_stage(&stages[i], NULL, hash);
         }
      void
   radv_hash_rt_shaders(unsigned char *hash, const VkRayTracingPipelineCreateInfoKHR *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_pipeline_layout, layout, pCreateInfo->layout);
            _mesa_sha1_init(&ctx);
   if (layout)
                              for (uint32_t i = 0; i < pCreateInfo->groupCount; i++) {
      _mesa_sha1_update(&ctx, &pCreateInfo->pGroups[i].type, sizeof(pCreateInfo->pGroups[i].type));
   _mesa_sha1_update(&ctx, &pCreateInfo->pGroups[i].generalShader, sizeof(pCreateInfo->pGroups[i].generalShader));
   _mesa_sha1_update(&ctx, &pCreateInfo->pGroups[i].anyHitShader, sizeof(pCreateInfo->pGroups[i].anyHitShader));
   _mesa_sha1_update(&ctx, &pCreateInfo->pGroups[i].closestHitShader,
         _mesa_sha1_update(&ctx, &pCreateInfo->pGroups[i].intersectionShader,
                     if (pCreateInfo->pLibraryInfo) {
      for (uint32_t i = 0; i < pCreateInfo->pLibraryInfo->libraryCount; ++i) {
      RADV_FROM_HANDLE(radv_pipeline, lib_pipeline, pCreateInfo->pLibraryInfo->pLibraries[i]);
   struct radv_ray_tracing_pipeline *lib = radv_pipeline_to_ray_tracing(lib_pipeline);
                  const uint64_t pipeline_flags =
      radv_get_pipeline_create_flags(pCreateInfo) &
   (VK_PIPELINE_CREATE_2_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR | VK_PIPELINE_CREATE_2_RAY_TRACING_SKIP_AABBS_BIT_KHR |
   VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_ANY_HIT_SHADERS_BIT_KHR |
   VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR |
   VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR |
               _mesa_sha1_update(&ctx, &flags, 4);
      }
      static void
   radv_shader_destroy(struct vk_device *_device, struct vk_pipeline_cache_object *object)
   {
      struct radv_device *device = container_of(_device, struct radv_device, vk);
            if (device->shader_use_invisible_vram) {
      /* Wait for any pending upload to complete, or we'll be writing into freed shader memory. */
                        free(shader->code);
   free(shader->spirv);
   free(shader->nir_string);
   free(shader->disasm_string);
   free(shader->ir_string);
            vk_pipeline_cache_object_finish(&shader->base);
      }
      static struct vk_pipeline_cache_object *
   radv_shader_deserialize(struct vk_pipeline_cache *cache, const void *key_data, size_t key_size,
         {
      struct radv_device *device = container_of(cache->base.device, struct radv_device, vk);
            struct radv_shader *shader;
   radv_shader_create_uncached(device, binary, false, NULL, &shader);
   if (!shader)
            assert(key_size == sizeof(shader->hash));
   memcpy(shader->hash, key_data, key_size);
               }
      static bool
   radv_shader_serialize(struct vk_pipeline_cache_object *object, struct blob *blob)
   {
      struct radv_shader *shader = container_of(object, struct radv_shader, base);
   size_t stats_size = shader->statistics ? aco_num_statistics * sizeof(uint32_t) : 0;
   size_t code_size = shader->code_size;
            struct radv_shader_binary_legacy binary = {
      .base =
      {
      .type = RADV_BINARY_TYPE_LEGACY,
   .config = shader->config,
   .info = shader->info,
         .code_size = code_size,
   .exec_size = shader->exec_size,
   .ir_size = 0,
   .disasm_size = 0,
               blob_write_bytes(blob, &binary, sizeof(struct radv_shader_binary_legacy));
   blob_write_bytes(blob, shader->statistics, stats_size);
               }
      struct radv_shader *
   radv_shader_create(struct radv_device *device, struct vk_pipeline_cache *cache, const struct radv_shader_binary *binary,
         {
      if (radv_is_cache_disabled(device) || skip_cache) {
      struct radv_shader *shader;
   radv_shader_create_uncached(device, binary, false, NULL, &shader);
               if (!cache)
            blake3_hash hash;
            struct vk_pipeline_cache_object *shader_obj;
   shader_obj = vk_pipeline_cache_create_and_insert_object(cache, hash, sizeof(hash), binary, binary->total_size,
               }
      const struct vk_pipeline_cache_object_ops radv_shader_ops = {
      .serialize = radv_shader_serialize,
   .deserialize = radv_shader_deserialize,
      };
      struct radv_pipeline_cache_object {
      struct vk_pipeline_cache_object base;
   unsigned num_shaders;
   unsigned num_stack_sizes;
   unsigned ps_epilog_binary_size;
   struct radv_shader_part *ps_epilog;
   void *data; /* pointer to stack sizes or ps epilog binary */
   uint8_t sha1[SHA1_DIGEST_LENGTH];
      };
      const struct vk_pipeline_cache_object_ops radv_pipeline_ops;
      static struct radv_pipeline_cache_object *
   radv_pipeline_cache_object_create(struct vk_device *device, unsigned num_shaders, const void *hash,
         {
      assert(num_stack_sizes == 0 || ps_epilog_binary_size == 0);
   const size_t size = sizeof(struct radv_pipeline_cache_object) + (num_shaders * sizeof(struct radv_shader *)) +
            struct radv_pipeline_cache_object *object = vk_alloc(&device->alloc, size, 8, VK_SYSTEM_ALLOCATION_SCOPE_CACHE);
   if (!object)
            vk_pipeline_cache_object_init(device, &object->base, &radv_pipeline_ops, object->sha1, SHA1_DIGEST_LENGTH);
   object->num_shaders = num_shaders;
   object->num_stack_sizes = num_stack_sizes;
   object->ps_epilog_binary_size = ps_epilog_binary_size;
   object->ps_epilog = NULL;
   object->data = &object->shaders[num_shaders];
   memcpy(object->sha1, hash, SHA1_DIGEST_LENGTH);
               }
      static void
   radv_pipeline_cache_object_destroy(struct vk_device *_device, struct vk_pipeline_cache_object *object)
   {
      struct radv_device *device = container_of(_device, struct radv_device, vk);
            for (unsigned i = 0; i < pipeline_obj->num_shaders; i++) {
      if (pipeline_obj->shaders[i])
      }
   if (pipeline_obj->ps_epilog)
            vk_pipeline_cache_object_finish(&pipeline_obj->base);
      }
      static struct vk_pipeline_cache_object *
   radv_pipeline_cache_object_deserialize(struct vk_pipeline_cache *cache, const void *key_data, size_t key_size,
         {
      struct radv_device *device = container_of(cache->base.device, struct radv_device, vk);
   assert(key_size == SHA1_DIGEST_LENGTH);
   unsigned total_size = blob->end - blob->current;
   unsigned num_shaders = blob_read_uint32(blob);
   unsigned num_stack_sizes = blob_read_uint32(blob);
            struct radv_pipeline_cache_object *object;
   object =
         if (!object)
                     for (unsigned i = 0; i < num_shaders; i++) {
      const uint8_t *hash = blob_read_bytes(blob, sizeof(blake3_hash));
   struct vk_pipeline_cache_object *shader =
            if (!shader) {
      /* If some shader could not be created from cache, better return NULL here than having
   * an incomplete cache object which needs to be fixed up later.
   */
   vk_pipeline_cache_object_unref(&device->vk, &object->base);
                           const size_t data_size = ps_epilog_binary_size + (num_stack_sizes * sizeof(uint32_t));
            if (ps_epilog_binary_size) {
      assert(num_stack_sizes == 0);
   struct radv_shader_part_binary *binary = object->data;
            if (!object->ps_epilog) {
      vk_pipeline_cache_object_unref(&device->vk, &object->base);
                     }
      static bool
   radv_pipeline_cache_object_serialize(struct vk_pipeline_cache_object *object, struct blob *blob)
   {
               blob_write_uint32(blob, pipeline_obj->num_shaders);
   blob_write_uint32(blob, pipeline_obj->num_stack_sizes);
            for (unsigned i = 0; i < pipeline_obj->num_shaders; i++)
            const size_t data_size = pipeline_obj->ps_epilog_binary_size + (pipeline_obj->num_stack_sizes * sizeof(uint32_t));
               }
      const struct vk_pipeline_cache_object_ops radv_pipeline_ops = {
      .serialize = radv_pipeline_cache_object_serialize,
   .deserialize = radv_pipeline_cache_object_deserialize,
      };
      bool
   radv_pipeline_cache_search(struct radv_device *device, struct vk_pipeline_cache *cache, struct radv_pipeline *pipeline,
         {
               if (radv_is_cache_disabled(device))
            bool *found = found_in_application_cache;
   if (!cache) {
      cache = device->mem_cache;
               struct vk_pipeline_cache_object *object =
            if (!object)
                     for (unsigned i = 0; i < pipeline_obj->num_shaders; i++) {
      gl_shader_stage s = pipeline_obj->shaders[i]->info.stage;
   if (s == MESA_SHADER_VERTEX && i > 0) {
      /* The GS copy-shader is a VS placed after all other stages */
   assert(i == pipeline_obj->num_shaders - 1 && pipeline->shaders[MESA_SHADER_GEOMETRY]);
      } else {
                     if (pipeline_obj->ps_epilog) {
               if (pipeline->type == RADV_PIPELINE_GRAPHICS)
         else
               pipeline->cache_object = object;
      }
      void
   radv_pipeline_cache_insert(struct radv_device *device, struct vk_pipeline_cache *cache, struct radv_pipeline *pipeline,
         {
      if (radv_is_cache_disabled(device))
            if (!cache)
            /* Count shaders */
   unsigned num_shaders = 0;
   for (unsigned i = 0; i < MESA_VULKAN_SHADER_STAGES; ++i)
                           struct radv_pipeline_cache_object *pipeline_obj;
            if (!pipeline_obj)
            unsigned idx = 0;
   for (unsigned i = 0; i < MESA_VULKAN_SHADER_STAGES; ++i) {
      if (pipeline->shaders[i])
      }
   /* Place the GS copy-shader after all other stages */
   if (pipeline->gs_copy_shader)
                     if (ps_epilog_binary) {
      memcpy(pipeline_obj->data, ps_epilog_binary, ps_epilog_binary_size);
   struct radv_shader_part *ps_epilog;
   if (pipeline->type == RADV_PIPELINE_GRAPHICS)
         else
                        /* Add the object to the cache */
      }
      bool
   radv_ray_tracing_pipeline_cache_search(struct radv_device *device, struct vk_pipeline_cache *cache,
               {
      if (radv_is_cache_disabled(device))
            if (!cache)
            bool cache_hit = false;
   struct vk_pipeline_cache_object *object =
            if (!object)
                     bool is_library = pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR;
   bool complete = true;
            if (!is_library)
            for (unsigned i = 0; i < pCreateInfo->stageCount; i++) {
      if (radv_ray_tracing_stage_is_compiled(&pipeline->stages[i]))
            if (is_library) {
      pipeline->stages[i].nir = radv_pipeline_cache_search_nir(device, cache, pipeline->stages[i].sha1);
                  assert(idx == pipeline_obj->num_shaders);
            uint32_t *stack_sizes = pipeline_obj->data;
   for (unsigned i = 0; i < pipeline->stage_count; i++)
            if (cache_hit && cache != device->mem_cache) {
      const VkPipelineCreationFeedbackCreateInfo *creation_feedback =
         if (creation_feedback)
      creation_feedback->pPipelineCreationFeedback->flags |=
            pipeline->base.base.cache_object = object;
      }
      void
   radv_ray_tracing_pipeline_cache_insert(struct radv_device *device, struct vk_pipeline_cache *cache,
               {
      if (radv_is_cache_disabled(device))
            if (!cache)
            /* Skip insertion on cache hit.
   * This branch can be triggered if a cache_object was found but not all NIR shaders could be
   * looked up. The cache_object is already complete in that case.
   */
   if (pipeline->base.base.cache_object)
            /* Count compiled shaders excl. library shaders */
   unsigned num_shaders = pipeline->base.base.shaders[MESA_SHADER_INTERSECTION] ? 1 : 0;
   for (unsigned i = 0; i < num_stages; ++i)
            struct radv_pipeline_cache_object *pipeline_obj =
            unsigned idx = 0;
   if (pipeline->base.base.shaders[MESA_SHADER_INTERSECTION])
            for (unsigned i = 0; i < num_stages; ++i) {
      if (radv_ray_tracing_stage_is_compiled(&pipeline->stages[i]))
      pipeline_obj->shaders[idx++] =
   }
            uint32_t *stack_sizes = pipeline_obj->data;
   for (unsigned i = 0; i < pipeline->stage_count; i++)
            /* Add the object to the cache */
      }
      struct vk_pipeline_cache_object *
   radv_pipeline_cache_search_nir(struct radv_device *device, struct vk_pipeline_cache *cache, const uint8_t *sha1)
   {
      if (radv_is_cache_disabled(device))
            if (!cache)
               }
      struct nir_shader *
   radv_pipeline_cache_handle_to_nir(struct radv_device *device, struct vk_pipeline_cache_object *object)
   {
      struct blob_reader blob;
   struct vk_raw_data_cache_object *nir_object = container_of(object, struct vk_raw_data_cache_object, base);
   blob_reader_init(&blob, nir_object->data, nir_object->data_size);
            if (blob.overrun) {
      ralloc_free(nir);
      }
               }
      struct vk_pipeline_cache_object *
   radv_pipeline_cache_nir_to_handle(struct radv_device *device, struct vk_pipeline_cache *cache, struct nir_shader *nir,
         {
      if (!cache)
            struct blob blob;
   blob_init(&blob);
            if (blob.out_of_memory) {
      blob_finish(&blob);
               void *data;
   size_t size;
   blob_finish_get_buffer(&blob, &data, &size);
            if (cached && !radv_is_cache_disabled(device)) {
      object = vk_pipeline_cache_create_and_insert_object(cache, sha1, SHA1_DIGEST_LENGTH, data, size,
      } else {
      struct vk_raw_data_cache_object *nir_object =
                     free(data);
      }
