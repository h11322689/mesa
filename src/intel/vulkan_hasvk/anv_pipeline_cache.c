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
      #include "util/blob.h"
   #include "util/hash_table.h"
   #include "util/u_debug.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
   #include "nir/nir_serialize.h"
   #include "anv_private.h"
   #include "nir/nir_xfb_info.h"
   #include "vulkan/util/vk_util.h"
      static bool
   anv_shader_bin_serialize(struct vk_pipeline_cache_object *object,
            struct vk_pipeline_cache_object *
   anv_shader_bin_deserialize(struct vk_pipeline_cache *cache,
                  static void
   anv_shader_bin_destroy(struct vk_device *_device,
         {
      struct anv_device *device =
         struct anv_shader_bin *shader =
            anv_state_pool_free(&device->instruction_state_pool, shader->kernel);
   vk_pipeline_cache_object_finish(&shader->base);
      }
      static const struct vk_pipeline_cache_object_ops anv_shader_bin_ops = {
      .serialize = anv_shader_bin_serialize,
   .deserialize = anv_shader_bin_deserialize,
      };
      const struct vk_pipeline_cache_object_ops *const anv_cache_import_ops[2] = {
      &anv_shader_bin_ops,
      };
      struct anv_shader_bin *
   anv_shader_bin_create(struct anv_device *device,
                        gl_shader_stage stage,
   const void *key_data, uint32_t key_size,
   const void *kernel_data, uint32_t kernel_size,
   const struct brw_stage_prog_data *prog_data_in,
      {
      VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_shader_bin, shader, 1);
   VK_MULTIALLOC_DECL_SIZE(&ma, void, obj_key_data, key_size);
   VK_MULTIALLOC_DECL_SIZE(&ma, struct brw_stage_prog_data, prog_data,
         VK_MULTIALLOC_DECL(&ma, struct brw_shader_reloc, prog_data_relocs,
                  VK_MULTIALLOC_DECL_SIZE(&ma, nir_xfb_info, xfb_info,
                  VK_MULTIALLOC_DECL(&ma, struct anv_pipeline_binding, surface_to_descriptor,
         VK_MULTIALLOC_DECL(&ma, struct anv_pipeline_binding, sampler_to_descriptor,
            if (!vk_multialloc_alloc(&ma, &device->vk.alloc,
                  memcpy(obj_key_data, key_data, key_size);
   vk_pipeline_cache_object_init(&device->vk, &shader->base,
                     shader->kernel =
         memcpy(shader->kernel.map, kernel_data, kernel_size);
            uint64_t shader_data_addr = INSTRUCTION_STATE_POOL_MIN_ADDRESS +
                  int rv_count = 0;
   struct brw_shader_reloc_value reloc_values[5];
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_CONST_DATA_ADDR_LOW,
      };
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_CONST_DATA_ADDR_HIGH,
      };
   reloc_values[rv_count++] = (struct brw_shader_reloc_value) {
      .id = BRW_SHADER_RELOC_SHADER_START_OFFSET,
      };
   brw_write_shader_relocs(&device->physical->compiler->isa,
                  memcpy(prog_data, prog_data_in, prog_data_size);
   typed_memcpy(prog_data_relocs, prog_data_in->relocs,
         prog_data->relocs = prog_data_relocs;
   memset(prog_data_param, 0,
         prog_data->param = prog_data_param;
   shader->prog_data = prog_data;
            assert(num_stats <= ARRAY_SIZE(shader->stats));
   typed_memcpy(shader->stats, stats, num_stats);
            if (xfb_info_in) {
      *xfb_info = *xfb_info_in;
   typed_memcpy(xfb_info->outputs, xfb_info_in->outputs,
            } else {
                  shader->bind_map = *bind_map;
   typed_memcpy(surface_to_descriptor, bind_map->surface_to_descriptor,
         shader->bind_map.surface_to_descriptor = surface_to_descriptor;
   typed_memcpy(sampler_to_descriptor, bind_map->sampler_to_descriptor,
                     }
      static bool
   anv_shader_bin_serialize(struct vk_pipeline_cache_object *object,
         {
      struct anv_shader_bin *shader =
                     blob_write_uint32(blob, shader->kernel_size);
            blob_write_uint32(blob, shader->prog_data_size);
   blob_write_bytes(blob, shader->prog_data, shader->prog_data_size);
   blob_write_bytes(blob, shader->prog_data->relocs,
                  blob_write_uint32(blob, shader->num_stats);
   blob_write_bytes(blob, shader->stats,
            if (shader->xfb_info) {
      uint32_t xfb_info_size =
         blob_write_uint32(blob, xfb_info_size);
      } else {
                  blob_write_bytes(blob, shader->bind_map.surface_sha1,
         blob_write_bytes(blob, shader->bind_map.sampler_sha1,
         blob_write_bytes(blob, shader->bind_map.push_sha1,
         blob_write_uint32(blob, shader->bind_map.surface_count);
   blob_write_uint32(blob, shader->bind_map.sampler_count);
   blob_write_bytes(blob, shader->bind_map.surface_to_descriptor,
               blob_write_bytes(blob, shader->bind_map.sampler_to_descriptor,
               blob_write_bytes(blob, shader->bind_map.push_ranges,
               }
      struct vk_pipeline_cache_object *
   anv_shader_bin_deserialize(struct vk_pipeline_cache *cache,
               {
      struct anv_device *device =
                     uint32_t kernel_size = blob_read_uint32(blob);
            uint32_t prog_data_size = blob_read_uint32(blob);
   const void *prog_data_bytes = blob_read_bytes(blob, prog_data_size);
   if (blob->overrun)
            union brw_any_prog_data prog_data;
   memcpy(&prog_data, prog_data_bytes,
         prog_data.base.relocs =
      blob_read_bytes(blob, prog_data.base.num_relocs *
         uint32_t num_stats = blob_read_uint32(blob);
   const struct brw_compile_stats *stats =
            const nir_xfb_info *xfb_info = NULL;
   uint32_t xfb_size = blob_read_uint32(blob);
   if (xfb_size)
            struct anv_pipeline_bind_map bind_map;
   blob_copy_bytes(blob, bind_map.surface_sha1, sizeof(bind_map.surface_sha1));
   blob_copy_bytes(blob, bind_map.sampler_sha1, sizeof(bind_map.sampler_sha1));
   blob_copy_bytes(blob, bind_map.push_sha1, sizeof(bind_map.push_sha1));
   bind_map.surface_count = blob_read_uint32(blob);
   bind_map.sampler_count = blob_read_uint32(blob);
   bind_map.surface_to_descriptor = (void *)
      blob_read_bytes(blob, bind_map.surface_count *
      bind_map.sampler_to_descriptor = (void *)
      blob_read_bytes(blob, bind_map.sampler_count *
               if (blob->overrun)
            struct anv_shader_bin *shader =
      anv_shader_bin_create(device, stage,
                        if (shader == NULL)
               }
      struct anv_shader_bin *
   anv_device_search_for_kernel(struct anv_device *device,
                     {
      /* Use the default pipeline cache if none is specified */
   if (cache == NULL)
            bool cache_hit = false;
   struct vk_pipeline_cache_object *object =
      vk_pipeline_cache_lookup_object(cache, key_data, key_size,
      if (user_cache_hit != NULL) {
      *user_cache_hit = object != NULL && cache_hit &&
      }
   if (object == NULL)
               }
      struct anv_shader_bin *
   anv_device_upload_kernel(struct anv_device *device,
                           struct vk_pipeline_cache *cache,
   gl_shader_stage stage,
   const void *key_data, uint32_t key_size,
   const void *kernel_data, uint32_t kernel_size,
   const struct brw_stage_prog_data *prog_data,
   uint32_t prog_data_size,
   {
      /* Use the default pipeline cache if none is specified */
   if (cache == NULL)
            struct anv_shader_bin *shader =
      anv_shader_bin_create(device, stage,
                        key_data, key_size,
   if (shader == NULL)
            struct vk_pipeline_cache_object *cached =
               }
      #define SHA1_KEY_SIZE 20
      struct nir_shader *
   anv_device_search_for_nir(struct anv_device *device,
                           {
      if (cache == NULL)
            return vk_pipeline_cache_lookup_nir(cache, sha1_key, SHA1_KEY_SIZE,
      }
      void
   anv_device_upload_nir(struct anv_device *device,
                     {
      if (cache == NULL)
               }
