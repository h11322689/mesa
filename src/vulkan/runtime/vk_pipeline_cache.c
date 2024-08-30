   /*
   * Copyright © 2021 Intel Corporation
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
      #include "vk_pipeline_cache.h"
      #include "vk_alloc.h"
   #include "vk_common_entrypoints.h"
   #include "vk_device.h"
   #include "vk_log.h"
   #include "vk_physical_device.h"
      #include "compiler/nir/nir_serialize.h"
      #include "util/blob.h"
   #include "util/u_debug.h"
   #include "util/disk_cache.h"
   #include "util/hash_table.h"
   #include "util/set.h"
      #define vk_pipeline_cache_log(cache, ...)                                      \
      if (cache->base.client_visible)                                             \
         static bool
   vk_raw_data_cache_object_serialize(struct vk_pipeline_cache_object *object,
         {
      struct vk_raw_data_cache_object *data_obj =
                        }
      static struct vk_pipeline_cache_object *
   vk_raw_data_cache_object_deserialize(struct vk_pipeline_cache *cache,
                     {
      /* We consume the entire blob_reader.  Each call to ops->deserialize()
   * happens with a brand new blob reader for error checking anyway so we
   * can assume the blob consumes the entire reader and we don't need to
   * serialize the data size separately.
   */
   assert(blob->current < blob->end);
   size_t data_size = blob->end - blob->current;
            struct vk_raw_data_cache_object *data_obj =
      vk_raw_data_cache_object_create(cache->base.device, key_data, key_size,
            }
      static void
   vk_raw_data_cache_object_destroy(struct vk_device *device,
         {
      struct vk_raw_data_cache_object *data_obj =
               }
      const struct vk_pipeline_cache_object_ops vk_raw_data_cache_object_ops = {
      .serialize = vk_raw_data_cache_object_serialize,
   .deserialize = vk_raw_data_cache_object_deserialize,
      };
      struct vk_raw_data_cache_object *
   vk_raw_data_cache_object_create(struct vk_device *device,
               {
      VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct vk_raw_data_cache_object, data_obj, 1);
   VK_MULTIALLOC_DECL_SIZE(&ma, char, obj_key_data, key_size);
            if (!vk_multialloc_alloc(&ma, &device->alloc,
                  vk_pipeline_cache_object_init(device, &data_obj->base,
               data_obj->data = obj_data;
            memcpy(obj_key_data, key_data, key_size);
               }
      static bool
   object_keys_equal(const void *void_a, const void *void_b)
   {
      const struct vk_pipeline_cache_object *a = void_a, *b = void_b;
   if (a->key_size != b->key_size)
               }
      static uint32_t
   object_key_hash(const void *void_object)
   {
      const struct vk_pipeline_cache_object *object = void_object;
      }
      static void
   vk_pipeline_cache_lock(struct vk_pipeline_cache *cache)
   {
         if (!(cache->flags & VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT))
      }
      static void
   vk_pipeline_cache_unlock(struct vk_pipeline_cache *cache)
   {
      if (!(cache->flags & VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT))
      }
      /* cache->lock must be held when calling */
   static void
   vk_pipeline_cache_remove_object(struct vk_pipeline_cache *cache,
               {
      struct set_entry *entry =
         if (entry && entry->key == (const void *)object) {
      /* Drop the reference owned by the cache */
   if (!cache->weak_ref)
                  }
      static inline struct vk_pipeline_cache_object *
   vk_pipeline_cache_object_weak_ref(struct vk_pipeline_cache *cache,
         {
      assert(!object->weak_owner);
   p_atomic_set(&object->weak_owner, cache);
      }
      void
   vk_pipeline_cache_object_unref(struct vk_device *device, struct vk_pipeline_cache_object *object)
   {
               struct vk_pipeline_cache *weak_owner = p_atomic_read(&object->weak_owner);
   if (!weak_owner) {
      if (p_atomic_dec_zero(&object->ref_cnt))
      } else {
      vk_pipeline_cache_lock(weak_owner);
   bool destroy = p_atomic_dec_zero(&object->ref_cnt);
   if (destroy) {
      uint32_t hash = object_key_hash(object);
      }
   vk_pipeline_cache_unlock(weak_owner);
   if (destroy)
         }
      static bool
   vk_pipeline_cache_object_serialize(struct vk_pipeline_cache *cache,
               {
      if (object->ops->serialize == NULL)
            assert(blob->size == align64(blob->size, VK_PIPELINE_CACHE_BLOB_ALIGN));
            /* Special case for if we're writing to a NULL blob (just to get the size)
   * and we already know the data size of the allocation.  This should make
   * the first GetPipelineCacheData() call to get the data size faster in the
   * common case where a bunch of our objects were loaded from a previous
   * cache or where we've already serialized the cache once.
   */
   if (blob->data == NULL && blob->fixed_allocation) {
      *data_size = p_atomic_read(&object->data_size);
   if (*data_size > 0) {
      blob_write_bytes(blob, NULL, *data_size);
                  if (!object->ops->serialize(object, blob)) {
      vk_pipeline_cache_log(cache, "Failed to serialize pipeline cache object");
               size_t size = blob->size - start;
   if (size > UINT32_MAX) {
      vk_pipeline_cache_log(cache, "Skipping giant (4 GiB or larger) object");
               if (blob->out_of_memory) {
      vk_pipeline_cache_log(cache,
                     *data_size = (uint32_t)size;
               }
      static struct vk_pipeline_cache_object *
   vk_pipeline_cache_object_deserialize(struct vk_pipeline_cache *cache,
                     {
      if (ops == NULL)
            if (unlikely(ops->deserialize == NULL)) {
      vk_pipeline_cache_log(cache,
                     struct blob_reader reader;
            struct vk_pipeline_cache_object *object =
            if (object == NULL)
            assert(reader.current == reader.end && !reader.overrun);
   assert(object->ops == ops);
   assert(object->ref_cnt == 1);
   assert(object->key_size == key_size);
               }
      static struct vk_pipeline_cache_object *
   vk_pipeline_cache_insert_object(struct vk_pipeline_cache *cache,
         {
               if (cache->object_cache == NULL)
                     vk_pipeline_cache_lock(cache);
   bool found = false;
   struct set_entry *entry = _mesa_set_search_or_add_pre_hashed(
            struct vk_pipeline_cache_object *result = NULL;
   /* add reference to either the found or inserted object */
   if (found) {
      struct vk_pipeline_cache_object *found_object = (void *)entry->key;
   if (found_object->ops != object->ops) {
      /* The found object in the cache isn't fully formed. Replace it. */
   assert(!cache->weak_ref);
   assert(found_object->ops == &vk_raw_data_cache_object_ops);
   assert(object->ref_cnt == 1);
   entry->key = object;
                  } else {
      result = object;
   if (!cache->weak_ref)
         else
      }
            if (found) {
         }
      }
      struct vk_pipeline_cache_object *
   vk_pipeline_cache_lookup_object(struct vk_pipeline_cache *cache,
                     {
      assert(key_size <= UINT32_MAX);
            if (cache_hit != NULL)
            struct vk_pipeline_cache_object key = {
      .key_data = key_data,
      };
                     if (cache != NULL && cache->object_cache != NULL) {
      vk_pipeline_cache_lock(cache);
   struct set_entry *entry =
         if (entry) {
      object = vk_pipeline_cache_object_ref((void *)entry->key);
   if (cache_hit != NULL)
      }
               if (object == NULL) {
      struct disk_cache *disk_cache = cache->base.device->physical->disk_cache;
   if (!cache->skip_disk_cache && disk_cache && cache->object_cache) {
                     size_t data_size;
   uint8_t *data = disk_cache_get(disk_cache, cache_key, &data_size);
   if (data) {
      object = vk_pipeline_cache_object_deserialize(cache,
                     free(data);
   if (object != NULL) {
                        /* No disk cache or not found in the disk cache */
               if (object->ops == &vk_raw_data_cache_object_ops &&
      ops != &vk_raw_data_cache_object_ops) {
   /* The object isn't fully formed yet and we need to deserialize it into
   * a real object before it can be used.
   */
   struct vk_raw_data_cache_object *data_obj =
            struct vk_pipeline_cache_object *real_object =
      vk_pipeline_cache_object_deserialize(cache,
                        if (real_object == NULL) {
                     vk_pipeline_cache_lock(cache);
   vk_pipeline_cache_remove_object(cache, hash, object);
   vk_pipeline_cache_unlock(cache);
   vk_pipeline_cache_object_unref(cache->base.device, object);
               vk_pipeline_cache_object_unref(cache->base.device, object);
                           }
      struct vk_pipeline_cache_object *
   vk_pipeline_cache_add_object(struct vk_pipeline_cache *cache,
         {
      struct vk_pipeline_cache_object *inserted =
            if (object == inserted) {
      /* If it wasn't in the object cache, it might not be in the disk cache
   * either.  Better try and add it.
            struct disk_cache *disk_cache = cache->base.device->physical->disk_cache;
   if (!cache->skip_disk_cache && object->ops->serialize && disk_cache) {
                     if (object->ops->serialize(object, &blob) && !blob.out_of_memory) {
      cache_key cache_key;
                                                }
      struct vk_pipeline_cache_object *
   vk_pipeline_cache_create_and_insert_object(struct vk_pipeline_cache *cache,
                     {
      struct disk_cache *disk_cache = cache->base.device->physical->disk_cache;
   if (!cache->skip_disk_cache && disk_cache) {
      cache_key cache_key;
   disk_cache_compute_key(disk_cache, key_data, key_size, cache_key);
               struct vk_pipeline_cache_object *object =
      vk_pipeline_cache_object_deserialize(cache, key_data, key_size, data,
         if (object)
               }
      nir_shader *
   vk_pipeline_cache_lookup_nir(struct vk_pipeline_cache *cache,
                     {
      struct vk_pipeline_cache_object *object =
      vk_pipeline_cache_lookup_object(cache, key_data, key_size,
            if (object == NULL)
            struct vk_raw_data_cache_object *data_obj =
            struct blob_reader blob;
            nir_shader *nir = nir_deserialize(mem_ctx, nir_options, &blob);
            if (blob.overrun) {
      ralloc_free(nir);
                  }
      void
   vk_pipeline_cache_add_nir(struct vk_pipeline_cache *cache,
               {
      struct blob blob;
            nir_serialize(&blob, nir, false);
   if (blob.out_of_memory) {
      vk_pipeline_cache_log(cache, "Ran out of memory serializing NIR shader");
   blob_finish(&blob);
               struct vk_raw_data_cache_object *data_obj =
      vk_raw_data_cache_object_create(cache->base.device,
                     struct vk_pipeline_cache_object *cached =
            }
      static int32_t
   find_type_for_ops(const struct vk_physical_device *pdevice,
         {
      const struct vk_pipeline_cache_object_ops *const *import_ops =
            if (import_ops == NULL)
            for (int32_t i = 0; import_ops[i]; i++) {
      if (import_ops[i] == ops)
                  }
      static const struct vk_pipeline_cache_object_ops *
   find_ops_for_type(const struct vk_physical_device *pdevice,
         {
      const struct vk_pipeline_cache_object_ops *const *import_ops =
            if (import_ops == NULL || type < 0)
               }
      static void
   vk_pipeline_cache_load(struct vk_pipeline_cache *cache,
         {
      struct blob_reader blob;
            struct vk_pipeline_cache_header header;
   blob_copy_bytes(&blob, &header, sizeof(header));
   uint32_t count = blob_read_uint32(&blob);
   if (blob.overrun)
            if (memcmp(&header, &cache->header, sizeof(header)) != 0)
            for (uint32_t i = 0; i < count; i++) {
      int32_t type = blob_read_uint32(&blob);
   uint32_t key_size = blob_read_uint32(&blob);
   uint32_t data_size = blob_read_uint32(&blob);
   const void *key_data = blob_read_bytes(&blob, key_size);
   blob_reader_align(&blob, VK_PIPELINE_CACHE_BLOB_ALIGN);
   const void *data = blob_read_bytes(&blob, data_size);
   if (blob.overrun)
            const struct vk_pipeline_cache_object_ops *ops =
            struct vk_pipeline_cache_object *object =
                  if (object == NULL) {
      vk_pipeline_cache_log(cache, "Failed to load pipeline cache object");
                     }
      struct vk_pipeline_cache *
   vk_pipeline_cache_create(struct vk_device *device,
               {
      static const struct VkPipelineCacheCreateInfo default_create_info = {
         };
            const struct VkPipelineCacheCreateInfo *pCreateInfo =
                     cache = vk_object_zalloc(device, pAllocator, sizeof(*cache),
         if (cache == NULL)
            cache->flags = pCreateInfo->flags;
      #ifndef ENABLE_SHADER_CACHE
         #else
         #endif
         struct VkPhysicalDeviceProperties pdevice_props;
   device->physical->dispatch_table.GetPhysicalDeviceProperties(
            cache->header = (struct vk_pipeline_cache_header) {
      .header_size = sizeof(struct vk_pipeline_cache_header),
   .header_version = VK_PIPELINE_CACHE_HEADER_VERSION_ONE,
   .vendor_id = pdevice_props.vendorID,
      };
                     if (info->force_enable ||
      debug_get_bool_option("VK_ENABLE_PIPELINE_CACHE", true)) {
   cache->object_cache = _mesa_set_create(NULL, object_key_hash,
               if (cache->object_cache && pCreateInfo->initialDataSize > 0) {
      vk_pipeline_cache_load(cache, pCreateInfo->pInitialData,
                  }
      void
   vk_pipeline_cache_destroy(struct vk_pipeline_cache *cache,
         {
      if (cache->object_cache) {
      if (!cache->weak_ref) {
      set_foreach(cache->object_cache, entry) {
            } else {
         }
      }
   simple_mtx_destroy(&cache->lock);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_CreatePipelineCache(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
            struct vk_pipeline_cache_create_info info = {
         };
   cache = vk_pipeline_cache_create(device, &info, pAllocator);
   if (cache == NULL)
                        }
      VKAPI_ATTR void VKAPI_CALL
   vk_common_DestroyPipelineCache(VkDevice device,
               {
               if (cache == NULL)
            assert(cache->base.device == vk_device_from_handle(device));
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_GetPipelineCacheData(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_device, device, _device);
            struct blob blob;
   if (pData) {
         } else {
                           uint32_t count = 0;
   intptr_t count_offset = blob_reserve_uint32(&blob);
   if (count_offset < 0) {
      *pDataSize = 0;
   blob_finish(&blob);
                        VkResult result = VK_SUCCESS;
   if (cache->object_cache != NULL) {
      set_foreach(cache->object_cache, entry) {
                                       int32_t type = find_type_for_ops(device->physical, object->ops);
   blob_write_uint32(&blob, type);
   blob_write_uint32(&blob, object->key_size);
                  if (!blob_align(&blob, VK_PIPELINE_CACHE_BLOB_ALIGN)) {
      result = VK_INCOMPLETE;
               uint32_t data_size;
   if (!vk_pipeline_cache_object_serialize(cache, object,
            blob.size = blob_size_save;
   if (blob.out_of_memory) {
                        /* Failed for some other reason; keep going */
                                                                                                   }
      VKAPI_ATTR VkResult VKAPI_CALL
   vk_common_MergePipelineCaches(VkDevice _device,
                     {
      VK_FROM_HANDLE(vk_pipeline_cache, dst, dstCache);
   VK_FROM_HANDLE(vk_device, device, _device);
   assert(dst->base.device == device);
            if (!dst->object_cache)
                     for (uint32_t i = 0; i < srcCacheCount; i++) {
      VK_FROM_HANDLE(vk_pipeline_cache, src, pSrcCaches[i]);
            if (!src->object_cache)
            assert(src != dst);
   if (src == dst)
                     set_foreach(src->object_cache, src_entry) {
               bool found_in_dst = false;
   struct set_entry *dst_entry =
      _mesa_set_search_or_add_pre_hashed(dst->object_cache,
            if (found_in_dst) {
      struct vk_pipeline_cache_object *dst_object = (void *)dst_entry->key;
   if (dst_object->ops == &vk_raw_data_cache_object_ops &&
      src_object->ops != &vk_raw_data_cache_object_ops) {
   /* Even though dst has the object, it only has the blob version
   * which isn't as useful.  Replace it with the real object.
   */
   vk_pipeline_cache_object_unref(device, dst_object);
         } else {
      /* We inserted src_object in dst so it needs a reference */
   assert(dst_entry->key == (const void *)src_object);
                                          }
