   /*
   * Copyright © Microsoft Corporation
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
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "dzn_private.h"
      #include "vk_alloc.h"
   #include "vk_debug_report.h"
   #include "vk_format.h"
   #include "vk_util.h"
      #include "dxil_spirv_nir.h"
      static void
   dzn_cmd_buffer_exec_transition_barriers(struct dzn_cmd_buffer *cmdbuf,
               {
      assert(!cmdbuf->enhanced_barriers);
   uint32_t flush_count = 0;
   for (uint32_t b = 0; b < barrier_count; b++) {
               /* some layouts map to the same states, and NOP-barriers are illegal */
   if (barriers[b].Transition.StateBefore == barriers[b].Transition.StateAfter) {
      if (flush_count) {
      ID3D12GraphicsCommandList1_ResourceBarrier(cmdbuf->cmdlist, flush_count,
               } else {
                     if (flush_count)
      ID3D12GraphicsCommandList1_ResourceBarrier(cmdbuf->cmdlist, flush_count,
         /* Set Before = After so we don't execute the same barrier twice. */
   for (uint32_t b = 0; b < barrier_count; b++)
      }
      static void
   dzn_cmd_buffer_flush_transition_barriers(struct dzn_cmd_buffer *cmdbuf,
                     {
      assert(!cmdbuf->enhanced_barriers);
   struct hash_entry *he =
                  if (!barriers)
               }
      enum dzn_queue_transition_flags {
      DZN_QUEUE_TRANSITION_FLUSH = 1 << 0,
      };
      static VkResult
   dzn_cmd_buffer_queue_transition_barriers(struct dzn_cmd_buffer *cmdbuf,
                                       {
      assert(!cmdbuf->enhanced_barriers);
   struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct hash_entry *he =
                  if (!barriers) {
      D3D12_RESOURCE_DESC desc = dzn_ID3D12Resource_GetDesc(res);
   D3D12_FEATURE_DATA_FORMAT_INFO fmt_info = { desc.Format, 0 };
   ID3D12Device_CheckFeatureSupport(device->dev, D3D12_FEATURE_FORMAT_INFO, &fmt_info, sizeof(fmt_info));
   uint32_t barrier_count =
                  barriers =
      vk_zalloc(&cmdbuf->vk.pool->alloc, sizeof(*barriers) * barrier_count,
      if (!barriers)
            he = _mesa_hash_table_insert(cmdbuf->transition_barriers, res, barriers);
   if (!he)
               for (uint32_t subres = first_subres; subres < first_subres + subres_count; subres++) {
      if (!barriers[subres].Transition.pResource) {
      barriers[subres] = (D3D12_RESOURCE_BARRIER) {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
   .Flags = 0,
   .Transition = {
      .pResource = res,
   .Subresource = subres,
   .StateBefore = before,
            if (flags & DZN_QUEUE_TRANSITION_BEFORE_IS_UNDEFINED)
                     assert(barriers[subres].Transition.StateAfter == before ||
                        if (flags & DZN_QUEUE_TRANSITION_FLUSH)
               }
      static VkResult
   dzn_cmd_buffer_queue_image_range_state_transition(struct dzn_cmd_buffer *cmdbuf,
                                 {
      assert(!cmdbuf->enhanced_barriers);
   uint32_t first_barrier = 0, barrier_count = 0;
            dzn_foreach_aspect(aspect, range->aspectMask) {
      uint32_t layer_count = dzn_get_layer_count(image, range);
   uint32_t level_count = dzn_get_level_count(image, range);
   for (uint32_t layer = 0; layer < layer_count; layer++) {
      uint32_t subres = dzn_image_range_get_subresource_index(image, range, aspect, 0, layer);
   if (!barrier_count) {
      first_barrier = subres;
   barrier_count = level_count;
      } else if (first_barrier + barrier_count == subres) {
      barrier_count += level_count;
               ret = dzn_cmd_buffer_queue_transition_barriers(cmdbuf, image->res,
                                          if (barrier_count) {
      ret = dzn_cmd_buffer_queue_transition_barriers(cmdbuf, image->res,
               if (ret != VK_SUCCESS)
                     }
      static VkResult
   dzn_cmd_buffer_queue_image_range_layout_transition(struct dzn_cmd_buffer *cmdbuf,
                                 {
      assert(!cmdbuf->enhanced_barriers);
   uint32_t first_barrier = 0, barrier_count = 0;
            if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED)
            dzn_foreach_aspect(aspect, range->aspectMask) {
      D3D12_RESOURCE_STATES after =
         D3D12_RESOURCE_STATES before =
      (old_layout == VK_IMAGE_LAYOUT_UNDEFINED ||
   old_layout == VK_IMAGE_LAYOUT_PREINITIALIZED) ?
               uint32_t layer_count = dzn_get_layer_count(image, range);
   uint32_t level_count = dzn_get_level_count(image, range);
   for (uint32_t layer = 0; layer < layer_count; layer++) {
      uint32_t subres = dzn_image_range_get_subresource_index(image, range, aspect, 0, layer);
   if (!barrier_count) {
      first_barrier = subres;
   barrier_count = level_count;
      } else if (first_barrier + barrier_count == subres) {
      barrier_count += level_count;
               ret = dzn_cmd_buffer_queue_transition_barriers(cmdbuf, image->res,
                                          if (barrier_count) {
      ret = dzn_cmd_buffer_queue_transition_barriers(cmdbuf, image->res,
               if (ret != VK_SUCCESS)
                     }
      static void
   dzn_cmd_buffer_global_barrier(struct dzn_cmd_buffer *cmdbuf,
                           {
      assert(cmdbuf->enhanced_barriers);
   D3D12_GLOBAL_BARRIER global = {
      .SyncBefore = sync_before,
   .SyncAfter = sync_after,
   .AccessBefore = access_before,
      };
   D3D12_BARRIER_GROUP group = {
      .Type = D3D12_BARRIER_TYPE_GLOBAL,
   .NumBarriers = 1,
      };
      }
      static void
   dzn_cmd_buffer_buffer_barrier(struct dzn_cmd_buffer *cmdbuf,
                                 {
      assert(cmdbuf->enhanced_barriers);
   D3D12_BUFFER_BARRIER buffer = {
      .SyncBefore = sync_before,
   .SyncAfter = sync_after,
   .AccessBefore = access_before,
   .AccessAfter = access_after,
   .pResource = buf,
   .Offset = 0,
      };
   D3D12_BARRIER_GROUP group = {
      .Type = D3D12_BARRIER_TYPE_BUFFER,
   .NumBarriers = 1,
      };
      }
      static void
   dzn_cmd_buffer_image_barrier(struct dzn_cmd_buffer *cmdbuf,
                              const struct dzn_image *image,
   D3D12_BARRIER_SYNC sync_before,
   D3D12_BARRIER_SYNC sync_after,
      {
      assert(cmdbuf->enhanced_barriers);
   uint32_t first_plane = (range->aspectMask == VK_IMAGE_ASPECT_STENCIL_BIT) ? 1 : 0;
   uint32_t plane_count = first_plane == 0 && (range->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) ? 2 : 1;
   D3D12_TEXTURE_BARRIER texture = {
      .SyncBefore = sync_before,
   .SyncAfter = sync_after,
   .AccessBefore = access_before,
   .AccessAfter = access_after,
   .LayoutBefore = layout_before,
   .LayoutAfter = layout_after,
   .Subresources.FirstArraySlice = range->baseArrayLayer,
   .Subresources.NumArraySlices = dzn_get_layer_count(image, range),
   .Subresources.IndexOrFirstMipLevel = range->baseMipLevel,
   .Subresources.NumMipLevels = dzn_get_level_count(image, range),
   .Subresources.FirstPlane = first_plane,
   .Subresources.NumPlanes = plane_count,
      };
   D3D12_BARRIER_GROUP group = {
      .Type = D3D12_BARRIER_TYPE_TEXTURE,
   .NumBarriers = 1,
      };
      }
      static D3D12_BARRIER_LAYOUT
   dzn_cmd_buffer_require_layout(struct dzn_cmd_buffer *cmdbuf,
                           {
      assert(cmdbuf->enhanced_barriers);
   /* We shouldn't need these fixups on a subresource range which includes depth and stencil,
         if (range->aspectMask == (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
      assert(current_layout != VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL &&
      }
      /* Nothing needs to be done for these, the appropriate sync/access was already handled */
   if (image->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS)
            D3D12_BARRIER_LAYOUT current_d3d_layout = dzn_vk_layout_to_d3d_layout(current_layout, cmdbuf->type, range->aspectMask);
   if (current_d3d_layout != needed_layout) {
      dzn_cmd_buffer_image_barrier(cmdbuf, image,
                  }
      }
      static void
   dzn_cmd_buffer_restore_layout(struct dzn_cmd_buffer *cmdbuf,
                                 const struct dzn_image *image,
   {
      if (needed_layout != restore_layout) {
      dzn_cmd_buffer_image_barrier(cmdbuf, image,
                     }
      static void
   dzn_cmd_buffer_destroy(struct vk_command_buffer *cbuf)
   {
      if (!cbuf)
                     if (cmdbuf->cmdlist)
            if (cmdbuf->cmdlist8)
            if (cmdbuf->cmdlist9)
            if (cmdbuf->cmdalloc)
            for (uint32_t bucket = 0; bucket < DZN_INTERNAL_BUF_BUCKET_COUNT; ++bucket) {
      list_for_each_entry_safe(struct dzn_internal_resource, res, &cmdbuf->internal_bufs[bucket], link) {
      list_del(&res->link);
   ID3D12Resource_Release(res->res);
                  dzn_descriptor_heap_pool_finish(&cmdbuf->cbv_srv_uav_pool);
   dzn_descriptor_heap_pool_finish(&cmdbuf->sampler_pool);
   dzn_descriptor_heap_pool_finish(&cmdbuf->rtvs.pool);
   dzn_descriptor_heap_pool_finish(&cmdbuf->dsvs.pool);
   util_dynarray_fini(&cmdbuf->events.signal);
   util_dynarray_fini(&cmdbuf->queries.reset);
            if (cmdbuf->rtvs.ht) {
      hash_table_foreach(cmdbuf->rtvs.ht, he)
                     if (cmdbuf->dsvs.ht) {
      hash_table_foreach(cmdbuf->dsvs.ht, he)
                     if (cmdbuf->events.ht)
            if (cmdbuf->queries.ht) {
      hash_table_foreach(cmdbuf->queries.ht, he) {
      struct dzn_cmd_buffer_query_pool_state *qpstate = he->data;
   util_dynarray_fini(&qpstate->reset);
   util_dynarray_fini(&qpstate->collect);
   util_dynarray_fini(&qpstate->signal);
   util_dynarray_fini(&qpstate->zero);
      }
               if (cmdbuf->transition_barriers) {
      hash_table_foreach(cmdbuf->transition_barriers, he)
                     vk_command_buffer_finish(&cmdbuf->vk);
      }
      static void
   dzn_cmd_buffer_reset(struct vk_command_buffer *cbuf, VkCommandBufferResetFlags flags)
   {
               /* Reset the state */
   memset(&cmdbuf->state, 0, sizeof(cmdbuf->state));
   cmdbuf->state.multiview.num_views = 1;
            /* TODO: Return resources to the pool */
   for (uint32_t bucket = 0; bucket < DZN_INTERNAL_BUF_BUCKET_COUNT; ++bucket) {
      list_for_each_entry_safe(struct dzn_internal_resource, res, &cmdbuf->internal_bufs[bucket], link) {
      list_del(&res->link);
   ID3D12Resource_Release(res->res);
         }
            util_dynarray_clear(&cmdbuf->events.signal);
   util_dynarray_clear(&cmdbuf->queries.reset);
   util_dynarray_clear(&cmdbuf->queries.signal);
   hash_table_foreach(cmdbuf->rtvs.ht, he)
         _mesa_hash_table_clear(cmdbuf->rtvs.ht, NULL);
   cmdbuf->null_rtv.ptr = 0;
   dzn_descriptor_heap_pool_reset(&cmdbuf->rtvs.pool);
   hash_table_foreach(cmdbuf->dsvs.ht, he)
         _mesa_hash_table_clear(cmdbuf->dsvs.ht, NULL);
   hash_table_foreach(cmdbuf->queries.ht, he) {
      struct dzn_cmd_buffer_query_pool_state *qpstate = he->data;
   util_dynarray_fini(&qpstate->reset);
   util_dynarray_fini(&qpstate->collect);
   util_dynarray_fini(&qpstate->signal);
   util_dynarray_fini(&qpstate->zero);
      }
   _mesa_hash_table_clear(cmdbuf->queries.ht, NULL);
   _mesa_hash_table_clear(cmdbuf->events.ht, NULL);
   hash_table_foreach(cmdbuf->transition_barriers, he)
         _mesa_hash_table_clear(cmdbuf->transition_barriers, NULL);
   dzn_descriptor_heap_pool_reset(&cmdbuf->dsvs.pool);
   dzn_descriptor_heap_pool_reset(&cmdbuf->cbv_srv_uav_pool);
            if (cmdbuf->vk.state == MESA_VK_COMMAND_BUFFER_STATE_RECORDING &&
      cmdbuf->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
                  if (cmdbuf->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
      }
      static uint32_t
   dzn_cmd_buffer_rtv_key_hash_function(const void *key)
   {
         }
      static bool
   dzn_cmd_buffer_rtv_key_equals_function(const void *a, const void *b)
   {
         }
      static uint32_t
   dzn_cmd_buffer_dsv_key_hash_function(const void *key)
   {
         }
      static bool
   dzn_cmd_buffer_dsv_key_equals_function(const void *a, const void *b)
   {
         }
      static const struct vk_command_buffer_ops cmd_buffer_ops = {
      .destroy = dzn_cmd_buffer_destroy,
      };
      static const D3D12_BARRIER_SYNC cmd_buffer_valid_sync[] = {
      [D3D12_COMMAND_LIST_TYPE_DIRECT] = ~(D3D12_BARRIER_SYNC_VIDEO_DECODE |
               [D3D12_COMMAND_LIST_TYPE_COMPUTE] = (D3D12_BARRIER_SYNC_ALL |
                                       D3D12_BARRIER_SYNC_COMPUTE_SHADING |
   D3D12_BARRIER_SYNC_RAYTRACING |
   D3D12_BARRIER_SYNC_COPY |
   D3D12_BARRIER_SYNC_EXECUTE_INDIRECT |
   D3D12_BARRIER_SYNC_PREDICATION |
      };
   static const D3D12_BARRIER_ACCESS cmd_buffer_valid_access[] = {
      [D3D12_COMMAND_LIST_TYPE_DIRECT] = ~(D3D12_BARRIER_ACCESS_VIDEO_DECODE_READ |
                                 [D3D12_COMMAND_LIST_TYPE_COMPUTE] = (D3D12_BARRIER_ACCESS_CONSTANT_BUFFER |
                                       D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
   D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
      };
      static VkResult
   dzn_cmd_buffer_create(const VkCommandBufferAllocateInfo *info,
         {
      VK_FROM_HANDLE(vk_command_pool, pool, info->commandPool);
   struct dzn_device *device = container_of(pool->base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev =
                     D3D12_COMMAND_LIST_TYPE type =
            struct dzn_cmd_buffer *cmdbuf =
         if (!cmdbuf)
            VkResult result =
         if (result != VK_SUCCESS) {
      vk_free(&pool->alloc, cmdbuf);
               memset(&cmdbuf->state, 0, sizeof(cmdbuf->state));
   cmdbuf->state.multiview.num_views = 1;
   cmdbuf->state.multiview.view_mask = 1;
   for (uint32_t bucket = 0; bucket < DZN_INTERNAL_BUF_BUCKET_COUNT; ++bucket)
         util_dynarray_init(&cmdbuf->events.signal, NULL);
   util_dynarray_init(&cmdbuf->queries.reset, NULL);
   util_dynarray_init(&cmdbuf->queries.signal, NULL);
   dzn_descriptor_heap_pool_init(&cmdbuf->rtvs.pool, device,
               dzn_descriptor_heap_pool_init(&cmdbuf->dsvs.pool, device,
               dzn_descriptor_heap_pool_init(&cmdbuf->cbv_srv_uav_pool, device,
               dzn_descriptor_heap_pool_init(&cmdbuf->sampler_pool, device,
                  cmdbuf->events.ht =
         cmdbuf->queries.ht =
         cmdbuf->transition_barriers =
         cmdbuf->rtvs.ht =
      _mesa_hash_table_create(NULL,
            cmdbuf->dsvs.ht =
      _mesa_hash_table_create(NULL,
            if (!cmdbuf->events.ht || !cmdbuf->queries.ht ||
      !cmdbuf->transition_barriers ||
   !cmdbuf->rtvs.ht || !cmdbuf->dsvs.ht) {
   result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               if (cmdbuf->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      if (FAILED(ID3D12Device1_CreateCommandAllocator(device->dev, type,
                  result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               if (FAILED(ID3D12Device4_CreateCommandList1(device->dev, 0, type,
                        result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               (void)ID3D12GraphicsCommandList_QueryInterface(cmdbuf->cmdlist, &IID_ID3D12GraphicsCommandList8, (void **)&cmdbuf->cmdlist8);
               cmdbuf->type = type;
   cmdbuf->valid_sync = cmd_buffer_valid_sync[type];
   cmdbuf->valid_access = cmd_buffer_valid_access[type];
         out:
      if (result != VK_SUCCESS)
         else
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_AllocateCommandBuffers(VkDevice device,
               {
      VK_FROM_HANDLE(dzn_device, dev, device);
   VkResult result = VK_SUCCESS;
            for (i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      result = dzn_cmd_buffer_create(pAllocateInfo,
         if (result != VK_SUCCESS)
               if (result != VK_SUCCESS) {
      dev->vk.dispatch_table.FreeCommandBuffers(device, pAllocateInfo->commandPool,
         for (i = 0; i < pAllocateInfo->commandBufferCount; i++)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_BeginCommandBuffer(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   vk_command_buffer_begin(&cmdbuf->vk, info);
   if (cmdbuf->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY)
            }
      static void
   dzn_cmd_buffer_gather_events(struct dzn_cmd_buffer *cmdbuf)
   {
      if (vk_command_buffer_has_error(&cmdbuf->vk))
            hash_table_foreach(cmdbuf->events.ht, he) {
               struct dzn_cmd_event_signal signal = { (struct dzn_event *)he->key, state == DZN_EVENT_STATE_SET };
   struct dzn_cmd_event_signal *entry =
            if (!entry) {
      vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
                        out:
         }
      static VkResult
   dzn_cmd_buffer_dynbitset_reserve(struct dzn_cmd_buffer *cmdbuf, struct util_dynarray *array, uint32_t bit)
   {
         if (bit < util_dynarray_num_elements(array, BITSET_WORD) * BITSET_WORDBITS)
            unsigned old_sz = array->size;
   void *ptr = util_dynarray_grow(array, BITSET_WORD, (bit + BITSET_WORDBITS) / BITSET_WORDBITS);
   if (!ptr)
            memset(ptr, 0, array->size - old_sz);
      }
      static bool
   dzn_cmd_buffer_dynbitset_test(struct util_dynarray *array, uint32_t bit)
   {
               if (bit < nbits)
               }
      static VkResult
   dzn_cmd_buffer_dynbitset_set(struct dzn_cmd_buffer *cmdbuf, struct util_dynarray *array, uint32_t bit)
   {
      VkResult result = dzn_cmd_buffer_dynbitset_reserve(cmdbuf, array, bit);
   if (result != VK_SUCCESS)
            BITSET_SET(util_dynarray_element(array, BITSET_WORD, 0), bit);
      }
      static void
   dzn_cmd_buffer_dynbitset_clear(struct dzn_cmd_buffer *cmdbuf, struct util_dynarray *array, uint32_t bit)
   {
      if (bit >= util_dynarray_num_elements(array, BITSET_WORD) * BITSET_WORDBITS)
               }
      static VkResult
   dzn_cmd_buffer_dynbitset_set_range(struct dzn_cmd_buffer *cmdbuf,
               {
      VkResult result = dzn_cmd_buffer_dynbitset_reserve(cmdbuf, array, bit + count - 1);
   if (result != VK_SUCCESS)
            BITSET_SET_RANGE(util_dynarray_element(array, BITSET_WORD, 0), bit, bit + count - 1);
      }
      static void
   dzn_cmd_buffer_dynbitset_clear_range(struct dzn_cmd_buffer *cmdbuf,
               {
               if (!nbits)
                     while (bit <= end) {
      uint32_t subcount = MIN2(end + 1 - bit, 32 - (bit % 32));
   BITSET_CLEAR_RANGE(util_dynarray_element(array, BITSET_WORD, 0), bit, bit + subcount - 1);
         }
      static struct dzn_cmd_buffer_query_pool_state *
   dzn_cmd_buffer_create_query_pool_state(struct dzn_cmd_buffer *cmdbuf)
   {
      struct dzn_cmd_buffer_query_pool_state *state =
      vk_alloc(&cmdbuf->vk.pool->alloc, sizeof(*state),
      if (!state) {
      vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               util_dynarray_init(&state->reset, NULL);
   util_dynarray_init(&state->collect, NULL);
   util_dynarray_init(&state->signal, NULL);
   util_dynarray_init(&state->zero, NULL);
      }
      static void
   dzn_cmd_buffer_destroy_query_pool_state(struct dzn_cmd_buffer *cmdbuf,
         {
      util_dynarray_fini(&state->reset);
   util_dynarray_fini(&state->collect);
   util_dynarray_fini(&state->signal);
   util_dynarray_fini(&state->zero);
      }
      static struct dzn_cmd_buffer_query_pool_state *
   dzn_cmd_buffer_get_query_pool_state(struct dzn_cmd_buffer *cmdbuf,
         {
      struct dzn_cmd_buffer_query_pool_state *state = NULL;
   struct hash_entry *he =
            if (!he) {
      state = dzn_cmd_buffer_create_query_pool_state(cmdbuf);
   if (!state)
            he = _mesa_hash_table_insert(cmdbuf->queries.ht, qpool, state);
   if (!he) {
      dzn_cmd_buffer_destroy_query_pool_state(cmdbuf, state);
   vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
         } else {
                     }
      static VkResult
   dzn_cmd_buffer_collect_queries(struct dzn_cmd_buffer *cmdbuf,
                           {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   uint32_t nbits_collect = util_dynarray_num_elements(&state->collect, BITSET_WORD) * BITSET_WORDBITS;
   uint32_t nbits_zero = util_dynarray_num_elements(&state->zero, BITSET_WORD) * BITSET_WORDBITS;
            if (!nbits_collect && !nbits_zero)
            query_count = MIN2(query_count, MAX2(nbits_collect, nbits_zero) - first_query);
   nbits_collect = MIN2(first_query + query_count, nbits_collect);
            VkResult result =
         if (result != VK_SUCCESS)
            if (cmdbuf->enhanced_barriers) {
      /* A global barrier is used because both resolve_buffer and collect_buffer might have been
   * copied from recently, and it's not worth the effort to track whether that's true. */
   dzn_cmd_buffer_global_barrier(cmdbuf,
            } else {
                  /* Resolve the valid query regions into the resolve buffer */
   BITSET_WORD *collect =
            for (start = first_query, end = first_query,
      __bitset_next_range(&start, &end, collect, nbits_collect);
   start < nbits_collect;
   __bitset_next_range(&start, &end, collect, nbits_collect)) {
   ID3D12GraphicsCommandList1_ResolveQueryData(cmdbuf->cmdlist,
                                       /* Zero out sections of the resolve buffer that contain queries for multi-view rendering
   * for views other than the first one. */
   BITSET_WORD *zero =
                  for (start = first_query, end = first_query,
      __bitset_next_range(&start, &end, zero, nbits_zero);
   start < nbits_zero;
   __bitset_next_range(&start, &end, zero, nbits_zero)) {
            for (unsigned i = 0; i < count; i += step) {
               ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist,
                                          uint32_t offset = dzn_query_pool_get_result_offset(qpool, first_query);
            if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf,
                  } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, qpool->resolve_buffer,
                                 ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist,
                        struct query_pass_data {
      struct util_dynarray *dynarray;
   BITSET_WORD *bitset;
      } passes[] = {
      { &state->collect, collect, nbits_collect },
      };
   for (uint32_t pass = 0; pass < ARRAY_SIZE(passes); ++pass) {
      BITSET_WORD *bitset = passes[pass].bitset;
   uint32_t nbits = passes[pass].count;
   for (start = first_query, end = first_query,
      __bitset_next_range(&start, &end, bitset, nbits);
   start < nbits;
   __bitset_next_range(&start, &end, bitset, nbits)) {
                                    ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist,
                                       dzn_cmd_buffer_dynbitset_set_range(cmdbuf, &state->signal, start, count);
                  if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, qpool->resolve_buffer,
                        }
      }
      static VkResult
   dzn_cmd_buffer_collect_query_ops(struct dzn_cmd_buffer *cmdbuf,
                     {
      BITSET_WORD *bitset = util_dynarray_element(bitset_array, BITSET_WORD, 0);
   uint32_t nbits = util_dynarray_num_elements(bitset_array, BITSET_WORD) * BITSET_WORDBITS;
            BITSET_FOREACH_RANGE(start, end, bitset, nbits) {
      struct dzn_cmd_buffer_query_range range = { qpool, start, end - start };
   struct dzn_cmd_buffer_query_range *entry =
            if (!entry)
                           }
      static VkResult
   dzn_cmd_buffer_gather_queries(struct dzn_cmd_buffer *cmdbuf)
   {
      hash_table_foreach(cmdbuf->queries.ht, he) {
      struct dzn_query_pool *qpool = (struct dzn_query_pool *)he->key;
   struct dzn_cmd_buffer_query_pool_state *state = he->data;
   VkResult result =
         if (result != VK_SUCCESS)
            result = dzn_cmd_buffer_collect_query_ops(cmdbuf, qpool, &state->reset, &cmdbuf->queries.reset);
   if (result != VK_SUCCESS)
            result = dzn_cmd_buffer_collect_query_ops(cmdbuf, qpool, &state->signal, &cmdbuf->queries.signal);
   if (result != VK_SUCCESS)
                  }
      VKAPI_ATTR VkResult VKAPI_CALL
   dzn_EndCommandBuffer(VkCommandBuffer commandBuffer)
   {
               if (cmdbuf->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      dzn_cmd_buffer_gather_events(cmdbuf);
   dzn_cmd_buffer_gather_queries(cmdbuf);
   HRESULT hres = ID3D12GraphicsCommandList1_Close(cmdbuf->cmdlist);
   if (FAILED(hres))
                  }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdPipelineBarrier2(VkCommandBuffer commandBuffer,
         {
               bool execution_barrier =
      !info->memoryBarrierCount &&
   !info->bufferMemoryBarrierCount &&
         if (execution_barrier) {
      /* Execution barrier can be emulated with a NULL UAV barrier (AKA
   * pipeline flush). That's the best we can do with the standard D3D12
   * barrier API.
   */
   D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
   .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                           /* Global memory barriers can be emulated with NULL UAV/Aliasing barriers.
   * Scopes are not taken into account, but that's inherent to the current
   * D3D12 barrier API.
   */
   if (info->memoryBarrierCount) {
               barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
   barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barriers[0].UAV.pResource = NULL;
   barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
   barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barriers[1].Aliasing.pResourceBefore = NULL;
   barriers[1].Aliasing.pResourceAfter = NULL;
               for (uint32_t i = 0; i < info->bufferMemoryBarrierCount; i++) {
      VK_FROM_HANDLE(dzn_buffer, buf, info->pBufferMemoryBarriers[i].buffer);
            /* UAV are used only for storage buffers, skip all other buffers. */
   if (!(buf->usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
   barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
   barrier.UAV.pResource = buf->res;
               for (uint32_t i = 0; i < info->imageMemoryBarrierCount; i++) {
      const VkImageMemoryBarrier2 *ibarrier = &info->pImageMemoryBarriers[i];
   const VkImageSubresourceRange *range = &ibarrier->subresourceRange;
            VkImageLayout old_layout = ibarrier->oldLayout;
   VkImageLayout new_layout = ibarrier->newLayout;
   if ((image->vk.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
      old_layout == VK_IMAGE_LAYOUT_GENERAL &&
   (ibarrier->srcAccessMask & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
      if ((image->vk.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) &&
      new_layout == VK_IMAGE_LAYOUT_GENERAL &&
   (ibarrier->dstAccessMask & VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                     }
      /* A straightforward translation of the Vulkan sync flags to D3D sync flags */
   static D3D12_BARRIER_SYNC
   translate_sync(VkPipelineStageFlags2 flags, bool before)
   {
      if (!before && (flags & VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT))
         else if (before && (flags & VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT))
            if (flags & (VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT |
               /* Theoretically transfer should be less, but it encompasses blit
   * (which can be draws) and clears, so bloat it up to everything. */
            D3D12_BARRIER_SYNC ret = D3D12_BARRIER_SYNC_NONE;
   if (flags & (VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT |
                     if (flags & VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT)
         if (flags & (VK_PIPELINE_STAGE_2_TESSELLATION_CONTROL_SHADER_BIT |
               VK_PIPELINE_STAGE_2_TESSELLATION_EVALUATION_SHADER_BIT |
   VK_PIPELINE_STAGE_2_GEOMETRY_SHADER_BIT |
   VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT |
   VK_PIPELINE_STAGE_2_TRANSFORM_FEEDBACK_BIT_EXT |
         if (flags & (VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
               if (flags & (VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
               if (flags & VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT)
         if (flags & VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT)
         if (flags & VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT)
         if (flags & VK_PIPELINE_STAGE_2_COPY_BIT)
         if (flags & VK_PIPELINE_STAGE_2_RESOLVE_BIT)
         if (flags & VK_PIPELINE_STAGE_2_CLEAR_BIT)
      ret |= D3D12_BARRIER_SYNC_RENDER_TARGET |
            if (flags & VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT)
         if (flags & VK_PIPELINE_STAGE_2_COMMAND_PREPROCESS_BIT_NV)
         if (flags & VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR)
         if (flags & VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR)
         if (flags & VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_COPY_BIT_KHR)
               }
      /* A straightforward translation of Vulkan access to D3D access */
   static D3D12_BARRIER_ACCESS
   translate_access(VkAccessFlags2 flags)
   {
      D3D12_BARRIER_ACCESS ret = D3D12_BARRIER_ACCESS_COMMON;
   if (flags & VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT)
         if (flags & VK_ACCESS_2_INDEX_READ_BIT)
         if (flags & VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT)
         if (flags & VK_ACCESS_2_UNIFORM_READ_BIT)
         if (flags & (VK_ACCESS_2_INPUT_ATTACHMENT_READ_BIT |
               if (flags & VK_ACCESS_2_SHADER_READ_BIT)
      ret |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER |
            if (flags & (VK_ACCESS_2_SHADER_WRITE_BIT |
                     if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT)
      ret |= D3D12_BARRIER_ACCESS_RENDER_TARGET |
      if (flags & VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT)
      ret |= D3D12_BARRIER_ACCESS_RENDER_TARGET |
      if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT)
         if (flags & VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)
         if (flags & VK_ACCESS_2_TRANSFER_READ_BIT)
      ret |= D3D12_BARRIER_ACCESS_COPY_SOURCE |
      if (flags & VK_ACCESS_2_TRANSFER_WRITE_BIT)
      ret |= D3D12_BARRIER_ACCESS_RENDER_TARGET |
         D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE |
   D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
      if (flags & VK_ACCESS_2_MEMORY_READ_BIT)
      ret |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT |
         D3D12_BARRIER_ACCESS_INDEX_BUFFER |
   D3D12_BARRIER_ACCESS_VERTEX_BUFFER |
   D3D12_BARRIER_ACCESS_CONSTANT_BUFFER |
   D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
   D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
   D3D12_BARRIER_ACCESS_RENDER_TARGET |
   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ |
      if (flags & VK_ACCESS_2_MEMORY_WRITE_BIT)
      ret |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS |
         D3D12_BARRIER_ACCESS_RENDER_TARGET |
   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE |
      if (flags & (VK_ACCESS_2_TRANSFORM_FEEDBACK_WRITE_BIT_EXT |
                     if (flags & VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT)
         if (flags & VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR)
         if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR)
         if (flags & VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR)
            }
      /* For texture barriers, D3D will validate that the access flags used are actually
   * things that were valid for the specified layout. Use the mask returned from here
   * to scope down the set of app-provided access flags to make validation happy. */
   static D3D12_BARRIER_ACCESS
   valid_access_for_layout(D3D12_BARRIER_LAYOUT layout)
   {
      switch (layout) {
   case D3D12_BARRIER_LAYOUT_UNDEFINED:
         case D3D12_BARRIER_LAYOUT_COMMON:
      return D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
            case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COMMON:
   case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COMMON:
      return D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
         D3D12_BARRIER_ACCESS_COPY_SOURCE |
      case D3D12_BARRIER_LAYOUT_GENERIC_READ:
   case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_GENERIC_READ:
      return D3D12_BARRIER_ACCESS_SHADER_RESOURCE |
         D3D12_BARRIER_ACCESS_COPY_SOURCE |
   D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ |
      case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_GENERIC_READ:
      return D3D12_BARRIER_ACCESS_SHADER_RESOURCE|
      case D3D12_BARRIER_LAYOUT_RENDER_TARGET:
         case D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS:
   case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_UNORDERED_ACCESS:
   case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_UNORDERED_ACCESS:
         case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE:
         case D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ:
         case D3D12_BARRIER_LAYOUT_SHADER_RESOURCE:
   case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_SHADER_RESOURCE:
   case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_SHADER_RESOURCE:
         case D3D12_BARRIER_LAYOUT_COPY_SOURCE:
   case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_SOURCE:
   case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_SOURCE:
         case D3D12_BARRIER_LAYOUT_COPY_DEST:
   case D3D12_BARRIER_LAYOUT_DIRECT_QUEUE_COPY_DEST:
   case D3D12_BARRIER_LAYOUT_COMPUTE_QUEUE_COPY_DEST:
         case D3D12_BARRIER_LAYOUT_RESOLVE_SOURCE:
         case D3D12_BARRIER_LAYOUT_RESOLVE_DEST:
         case D3D12_BARRIER_LAYOUT_SHADING_RATE_SOURCE:
         default:
            }
      /* Similar to layout -> access, there's access -> sync validation too. D3D
   * doesn't like over-synchronizing if you weren't accessing a resource through
   * a relevant access bit. */
   static D3D12_BARRIER_SYNC
   adjust_sync_for_access(D3D12_BARRIER_SYNC in, D3D12_BARRIER_ACCESS access)
   {
      /* NO_ACCESS must not add sync */
   if (access == D3D12_BARRIER_ACCESS_NO_ACCESS)
         /* SYNC_ALL can be used with any access bits */
   if (in == D3D12_BARRIER_SYNC_ALL)
         /* ACCESS_COMMON needs at least one sync bit */
   if (access == D3D12_BARRIER_ACCESS_COMMON)
            D3D12_BARRIER_SYNC out = D3D12_BARRIER_SYNC_NONE;
   if (access & D3D12_BARRIER_ACCESS_VERTEX_BUFFER)
      out |= in & (D3D12_BARRIER_SYNC_VERTEX_SHADING |
                  if (access & D3D12_BARRIER_ACCESS_CONSTANT_BUFFER)
      out |= in & (D3D12_BARRIER_SYNC_VERTEX_SHADING |
               D3D12_BARRIER_SYNC_PIXEL_SHADING |
   D3D12_BARRIER_SYNC_COMPUTE_SHADING |
      if (access & D3D12_BARRIER_ACCESS_INDEX_BUFFER)
         if (access & D3D12_BARRIER_ACCESS_RENDER_TARGET)
         if (access & D3D12_BARRIER_ACCESS_UNORDERED_ACCESS)
      out |= in & (D3D12_BARRIER_SYNC_VERTEX_SHADING |
               D3D12_BARRIER_SYNC_PIXEL_SHADING |
   D3D12_BARRIER_SYNC_COMPUTE_SHADING |
      if (access & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE)
      out |= in & (D3D12_BARRIER_SYNC_DRAW |
      if (access & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ)
      out |= in & (D3D12_BARRIER_SYNC_DRAW |
      if (access & D3D12_BARRIER_ACCESS_SHADER_RESOURCE)
      out |= in & (D3D12_BARRIER_SYNC_VERTEX_SHADING |
               D3D12_BARRIER_SYNC_PIXEL_SHADING |
   D3D12_BARRIER_SYNC_COMPUTE_SHADING |
      if (access & D3D12_BARRIER_ACCESS_STREAM_OUTPUT)
      out |= in & (D3D12_BARRIER_SYNC_VERTEX_SHADING |
                  if (access & D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT)
      out |= in & (D3D12_BARRIER_SYNC_DRAW |
      if (access & D3D12_BARRIER_ACCESS_PREDICATION)
      out |= in & (D3D12_BARRIER_SYNC_DRAW |
      if (access & (D3D12_BARRIER_ACCESS_COPY_DEST | D3D12_BARRIER_ACCESS_COPY_SOURCE))
         if (access & (D3D12_BARRIER_ACCESS_RESOLVE_DEST | D3D12_BARRIER_ACCESS_RESOLVE_SOURCE))
         if (access & D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ)
      out |= in & (D3D12_BARRIER_SYNC_COMPUTE_SHADING |
               D3D12_BARRIER_SYNC_RAYTRACING |
   D3D12_BARRIER_SYNC_ALL_SHADING |
   D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE |
      if (access & D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE)
      out |= in & (D3D12_BARRIER_SYNC_COMPUTE_SHADING |
               D3D12_BARRIER_SYNC_RAYTRACING |
   D3D12_BARRIER_SYNC_ALL_SHADING |
      if (access & D3D12_BARRIER_ACCESS_SHADING_RATE_SOURCE)
      out |= in & (D3D12_BARRIER_SYNC_PIXEL_SHADING |
      /* SYNC_NONE means it won't be accessed, so if we can't express the app's original intent
   * here, then be conservative and over-sync. */
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdPipelineBarrier2_enhanced(VkCommandBuffer commandBuffer,
         {
               uint32_t num_barrier_groups = 0;
            /* Some input image barriers will expand into 2 outputs, and some will turn into buffer barriers.
   * Do a first pass and count how much we need to allocate. */
   uint32_t num_image_barriers = 0;
   uint32_t num_buffer_barriers = info->bufferMemoryBarrierCount;
   for (uint32_t i = 0; i < info->imageMemoryBarrierCount; ++i) {
      VK_FROM_HANDLE(dzn_image, image, info->pImageMemoryBarriers[i].image);
   bool need_separate_aspect_barriers =
      info->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
   info->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL ||
   info->pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
      if (image->vk.tiling == VK_IMAGE_TILING_LINEAR)
         else
               VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, D3D12_GLOBAL_BARRIER, global_barriers, info->memoryBarrierCount);
   VK_MULTIALLOC_DECL(&ma, D3D12_BUFFER_BARRIER, buffer_barriers, num_buffer_barriers);
            if (ma.size == 0)
            if (!vk_multialloc_alloc(&ma, &cmdbuf->vk.pool->alloc,
            vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               if (info->memoryBarrierCount) {
      groups[num_barrier_groups].NumBarriers = info->memoryBarrierCount;
   groups[num_barrier_groups].Type = D3D12_BARRIER_TYPE_GLOBAL;
   groups[num_barrier_groups].pGlobalBarriers = global_barriers;
   ++num_barrier_groups;
   for (uint32_t i = 0; i < info->memoryBarrierCount; ++i) {
      global_barriers[i].SyncBefore = translate_sync(info->pMemoryBarriers[i].srcStageMask, true) & cmdbuf->valid_sync;
   global_barriers[i].SyncAfter = translate_sync(info->pMemoryBarriers[i].dstStageMask, false) & cmdbuf->valid_sync;
   global_barriers[i].AccessBefore = global_barriers[i].SyncBefore == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
      global_barriers[i].AccessAfter = global_barriers[i].SyncAfter == D3D12_BARRIER_SYNC_NONE ?
                  if ((global_barriers[i].AccessBefore & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE) &&
      (global_barriers[i].AccessAfter == D3D12_BARRIER_ACCESS_COMMON ||
   global_barriers[i].AccessAfter & ~(D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE | D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ))) {
   /* D3D validates against a global barrier attempting to transition from depth write to something other than depth write,
   * but this is a D3D bug; it's absolutely valid to use a global barrier to transition *multiple* types of accesses.
   * The validation does say that you'd need an image barrier to actually get that kind of transition, which is still correct,
   * so just remove this bit under the assumption that a dedicated image barrier will be submitted to do any necessary work later. */
      }
   if (global_barriers[i].AccessBefore == D3D12_BARRIER_ACCESS_COMMON)
         global_barriers[i].SyncBefore = adjust_sync_for_access(global_barriers[i].SyncBefore, global_barriers[i].AccessBefore);
                  if (num_buffer_barriers) {
      groups[num_barrier_groups].NumBarriers = num_buffer_barriers;
   groups[num_barrier_groups].Type = D3D12_BARRIER_TYPE_BUFFER;
   groups[num_barrier_groups].pBufferBarriers = buffer_barriers;
   ++num_barrier_groups;
   for (uint32_t i = 0; i < info->bufferMemoryBarrierCount; ++i) {
      VK_FROM_HANDLE(dzn_buffer, buf, info->pBufferMemoryBarriers[i].buffer);
   buffer_barriers[i].SyncBefore = translate_sync(info->pBufferMemoryBarriers[i].srcStageMask, true) & cmdbuf->valid_sync;
   buffer_barriers[i].SyncAfter = translate_sync(info->pBufferMemoryBarriers[i].dstStageMask, false) & cmdbuf->valid_sync;
   buffer_barriers[i].AccessBefore = buffer_barriers[i].SyncBefore == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
      buffer_barriers[i].AccessAfter = buffer_barriers[i].SyncAfter == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
      buffer_barriers[i].SyncBefore = adjust_sync_for_access(buffer_barriers[i].SyncBefore, buffer_barriers[i].AccessBefore);
   buffer_barriers[i].SyncAfter = adjust_sync_for_access(buffer_barriers[i].SyncAfter, buffer_barriers[i].AccessAfter);
   buffer_barriers[i].pResource = buf->res;
   buffer_barriers[i].Offset = 0;
                  if (num_image_barriers) {
      groups[num_barrier_groups].Type = D3D12_BARRIER_TYPE_TEXTURE;
   groups[num_barrier_groups].pTextureBarriers = texture_barriers;
   groups[num_barrier_groups].NumBarriers = num_image_barriers;
               uint32_t tbar = 0;
   uint32_t bbar = info->bufferMemoryBarrierCount;
   for (uint32_t i = 0; i < info->imageMemoryBarrierCount; ++i) {
               if (image->vk.tiling == VK_IMAGE_TILING_LINEAR) {
      /* Barriers on linear images turn into buffer barriers */
   buffer_barriers[bbar].SyncBefore = translate_sync(info->pImageMemoryBarriers[i].srcStageMask, true) & cmdbuf->valid_sync;
   buffer_barriers[bbar].SyncAfter = translate_sync(info->pImageMemoryBarriers[i].dstStageMask, false) & cmdbuf->valid_sync;
   buffer_barriers[bbar].AccessBefore = buffer_barriers[bbar].SyncBefore == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
      buffer_barriers[bbar].AccessAfter = buffer_barriers[bbar].SyncAfter == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
      buffer_barriers[bbar].SyncBefore = adjust_sync_for_access(buffer_barriers[bbar].SyncBefore, buffer_barriers[bbar].AccessBefore);
   buffer_barriers[bbar].SyncAfter = adjust_sync_for_access(buffer_barriers[bbar].SyncAfter, buffer_barriers[bbar].AccessAfter);
   buffer_barriers[bbar].pResource = image->res;
   buffer_barriers[bbar].Offset = 0;
   buffer_barriers[bbar].Size = UINT64_MAX;
   ++bbar;
               const VkImageSubresourceRange *range = &info->pImageMemoryBarriers[i].subresourceRange;
            bool need_separate_aspect_barriers =
      info->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
   info->pImageMemoryBarriers[i].oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL ||
   info->pImageMemoryBarriers[i].newLayout == VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL ||
      uint32_t num_aspects = need_separate_aspect_barriers ? 2 : 1;
   VkImageAspectFlags aspect_0_mask = need_separate_aspect_barriers ?
         VkImageAspectFlags aspects[] = {
      range->aspectMask & aspect_0_mask,
               for (uint32_t aspect_idx = 0; aspect_idx < num_aspects; ++aspect_idx) {
      VkImageAspectFlags aspect = aspects[aspect_idx];
   texture_barriers[tbar].SyncBefore = translate_sync(info->pImageMemoryBarriers[i].srcStageMask, true) & cmdbuf->valid_sync;
   texture_barriers[tbar].SyncAfter = translate_sync(info->pImageMemoryBarriers[i].dstStageMask, false) & cmdbuf->valid_sync;
   const bool queue_ownership_transfer = info->pImageMemoryBarriers[i].srcQueueFamilyIndex != info->pImageMemoryBarriers[i].dstQueueFamilyIndex;
   D3D12_BARRIER_ACCESS layout_before_valid_access = ~0;
   D3D12_BARRIER_ACCESS layout_after_valid_access = ~0;
   if (simultaneous_access) {
      /* Simultaneous access textures never perform layout transitions, and can do any type of access from COMMON layout */
      } else if (queue_ownership_transfer) {
      /* For an ownership transfer, force the foreign layout to COMMON and the matching sync/access to NONE */
   assert(info->pImageMemoryBarriers[i].srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED);
   assert(info->pImageMemoryBarriers[i].dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED);
   const bool is_release = info->pImageMemoryBarriers[i].srcQueueFamilyIndex == cmdbuf->vk.pool->queue_family_index;
   const bool is_acquire = info->pImageMemoryBarriers[i].dstQueueFamilyIndex == cmdbuf->vk.pool->queue_family_index;
   assert(is_release ^ is_acquire);
   texture_barriers[tbar].LayoutBefore = is_acquire ?
         texture_barriers[tbar].LayoutAfter = is_release ?
         if (is_acquire) {
      texture_barriers[tbar].SyncBefore = D3D12_BARRIER_SYNC_NONE;
   texture_barriers[tbar].AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS;
      } else {
      texture_barriers[tbar].SyncAfter = D3D12_BARRIER_SYNC_NONE;
   texture_barriers[tbar].AccessAfter = D3D12_BARRIER_ACCESS_NO_ACCESS;
         } else {
      texture_barriers[tbar].LayoutBefore = dzn_vk_layout_to_d3d_layout(info->pImageMemoryBarriers[i].oldLayout, cmdbuf->type, aspect);
   texture_barriers[tbar].LayoutAfter = dzn_vk_layout_to_d3d_layout(info->pImageMemoryBarriers[i].newLayout, cmdbuf->type, aspect);
   layout_before_valid_access = valid_access_for_layout(texture_barriers[tbar].LayoutBefore);
               texture_barriers[tbar].AccessBefore = texture_barriers[tbar].SyncBefore == D3D12_BARRIER_SYNC_NONE ||
            D3D12_BARRIER_ACCESS_NO_ACCESS :
   translate_access(info->pImageMemoryBarriers[i].srcAccessMask) &
      texture_barriers[tbar].AccessAfter = texture_barriers[tbar].SyncAfter == D3D12_BARRIER_SYNC_NONE ?
      D3D12_BARRIER_ACCESS_NO_ACCESS :
               texture_barriers[tbar].SyncBefore = adjust_sync_for_access(texture_barriers[tbar].SyncBefore, texture_barriers[tbar].AccessBefore);
   texture_barriers[tbar].SyncAfter = adjust_sync_for_access(texture_barriers[tbar].SyncAfter, texture_barriers[tbar].AccessAfter);
   texture_barriers[tbar].Subresources.FirstArraySlice = range->baseArrayLayer;
   texture_barriers[tbar].Subresources.NumArraySlices = dzn_get_layer_count(image, range);
   texture_barriers[tbar].Subresources.IndexOrFirstMipLevel = range->baseMipLevel;
   texture_barriers[tbar].Subresources.NumMipLevels = dzn_get_level_count(image, range);
   texture_barriers[tbar].Subresources.FirstPlane = aspect_idx;
   texture_barriers[tbar].Subresources.NumPlanes = util_bitcount(aspect);
   texture_barriers[tbar].pResource = image->res;
   texture_barriers[tbar].Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE;
   if (texture_barriers[tbar].LayoutBefore == D3D12_BARRIER_LAYOUT_UNDEFINED)
               }
   assert(bbar == num_buffer_barriers);
                        }
      static D3D12_CPU_DESCRIPTOR_HANDLE
   dzn_cmd_buffer_get_dsv(struct dzn_cmd_buffer *cmdbuf,
               {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_cmd_buffer_dsv_key key = { image, *desc };
   struct hash_entry *he = _mesa_hash_table_search(cmdbuf->dsvs.ht, &key);
            if (!he) {
      struct dzn_descriptor_heap *heap;
            // TODO: error handling
   dsve = vk_alloc(&cmdbuf->vk.pool->alloc, sizeof(*dsve), 8,
         dsve->key = key;
   dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->dsvs.pool, device, 1, &heap, &slot);
   dsve->handle = dzn_descriptor_heap_get_cpu_handle(heap, slot);
   ID3D12Device1_CreateDepthStencilView(device->dev, image->res, desc, dsve->handle);
      } else {
                     }
      static D3D12_CPU_DESCRIPTOR_HANDLE
   dzn_cmd_buffer_get_rtv(struct dzn_cmd_buffer *cmdbuf,
               {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_cmd_buffer_rtv_key key = { image, *desc };
   struct hash_entry *he = _mesa_hash_table_search(cmdbuf->rtvs.ht, &key);
            if (!he) {
      struct dzn_descriptor_heap *heap;
            // TODO: error handling
   rtve = vk_alloc(&cmdbuf->vk.pool->alloc, sizeof(*rtve), 8,
         rtve->key = key;
   dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->rtvs.pool, device, 1, &heap, &slot);
   rtve->handle = dzn_descriptor_heap_get_cpu_handle(heap, slot);
   ID3D12Device1_CreateRenderTargetView(device->dev, image->res, desc, rtve->handle);
      } else {
                     }
      static D3D12_CPU_DESCRIPTOR_HANDLE
   dzn_cmd_buffer_get_null_rtv(struct dzn_cmd_buffer *cmdbuf)
   {
               if (!cmdbuf->null_rtv.ptr) {
      struct dzn_descriptor_heap *heap;
   uint32_t slot;
   dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->rtvs.pool, device, 1, &heap, &slot);
            D3D12_RENDER_TARGET_VIEW_DESC desc = { 0 };
   desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
   desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
   desc.Texture2D.MipSlice = 0;
                           }
      static D3D12_HEAP_TYPE
   heap_type_for_bucket(enum dzn_internal_buf_bucket bucket)
   {
      switch (bucket) {
   case DZN_INTERNAL_BUF_UPLOAD: return D3D12_HEAP_TYPE_UPLOAD;
   case DZN_INTERNAL_BUF_DEFAULT: return D3D12_HEAP_TYPE_DEFAULT;
   default: unreachable("Invalid value");
      }
      static VkResult
   dzn_cmd_buffer_alloc_internal_buf(struct dzn_cmd_buffer *cmdbuf,
                                       {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   ID3D12Resource *res;
   *out = NULL;
            if (bucket == DZN_INTERNAL_BUF_UPLOAD && cmdbuf->cur_upload_buf) {
      uint64_t new_offset = ALIGN_POT(cmdbuf->cur_upload_buf_offset, align);
   if (cmdbuf->cur_upload_buf->size >= size + new_offset) {
      cmdbuf->cur_upload_buf_offset = new_offset + size;
   *out = cmdbuf->cur_upload_buf->res;
   *offset = new_offset;
      }
   cmdbuf->cur_upload_buf = NULL;
               uint32_t alloc_size = size;
   if (bucket == DZN_INTERNAL_BUF_UPLOAD)
      /* Walk through a 4MB upload buffer */
      else
      /* Align size on 64k (the default alignment) */
         D3D12_HEAP_PROPERTIES hprops = dzn_ID3D12Device4_GetCustomHeapProperties(device->dev, 0, heap_type);
   D3D12_RESOURCE_DESC rdesc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
   .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .Width = alloc_size,
   .Height = 1,
   .DepthOrArraySize = 1,
   .MipLevels = 1,
   .Format = DXGI_FORMAT_UNKNOWN,
   .SampleDesc = { .Count = 1, .Quality = 0 },
   .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
               HRESULT hres =
      ID3D12Device1_CreateCommittedResource(device->dev, &hprops,
                        if (FAILED(hres)) {
                  struct dzn_internal_resource *entry =
      vk_alloc(&cmdbuf->vk.pool->alloc, sizeof(*entry), 8,
      if (!entry) {
      ID3D12Resource_Release(res);
               entry->res = res;
   entry->size = alloc_size;
   list_addtail(&entry->link, &cmdbuf->internal_bufs[bucket]);
   *out = entry->res;
   if (offset)
         if (bucket == DZN_INTERNAL_BUF_UPLOAD) {
      cmdbuf->cur_upload_buf = entry;
      }
      }
      static void
   dzn_cmd_buffer_clear_rects_with_copy(struct dzn_cmd_buffer *cmdbuf,
                                 {
      enum pipe_format pfmt = vk_format_to_pipe_format(image->vk.format);
   uint32_t blksize = util_format_get_blocksize(pfmt);
   uint8_t buf[D3D12_TEXTURE_DATA_PITCH_ALIGNMENT * 3] = { 0 };
            assert(blksize <= sizeof(raw));
                     uint32_t fill_step = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
   while (fill_step % blksize)
            uint32_t max_w = u_minify(image->vk.extent.width, range->baseMipLevel);
   uint32_t max_h = u_minify(image->vk.extent.height, range->baseMipLevel);
   uint32_t row_pitch = ALIGN_NPOT(max_w * blksize, fill_step);
                     for (uint32_t i = 0; i < fill_step; i += blksize)
            ID3D12Resource *src_res;
            VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, res_size,
                              if (result != VK_SUCCESS)
                     uint8_t *cpu_ptr;
   ID3D12Resource_Map(src_res, 0, NULL, (void **)&cpu_ptr);
   cpu_ptr += src_offset;
   for (uint32_t i = 0; i < res_size; i += fill_step)
                     D3D12_TEXTURE_COPY_LOCATION src_loc = {
      .pResource = src_res,
   .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
   .PlacedFootprint = {
      .Offset = src_offset,
   .Footprint = {
      .Width = max_w,
   .Height = max_h,
   .Depth = 1,
                     if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                           assert(dzn_get_level_count(image, range) == 1);
            dzn_foreach_aspect(aspect, range->aspectMask) {
      VkImageSubresourceLayers subres = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .mipLevel = range->baseMipLevel,
   .baseArrayLayer = range->baseArrayLayer,
               for (uint32_t layer = 0; layer < layer_count; layer++) {
                     src_loc.PlacedFootprint.Footprint.Format =
      dst_loc.Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT ?
               for (uint32_t r = 0; r < rect_count; r++) {
      D3D12_BOX src_box = {
      .left = 0,
   .top = 0,
   .front = 0,
   .right = (UINT)(rects[r].right - rects[r].left),
                     ID3D12GraphicsCommandList1_CopyTextureRegion(cmdbuf->cmdlist,
                                             if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                     }
      static VkClearColorValue
   adjust_clear_color(struct dzn_physical_device *pdev,
         {
               // D3D12 doesn't support bgra4, so we map it to rgba4 and swizzle things
   // manually where it matters, like here, in the clear path.
   if (format == VK_FORMAT_B4G4R4A4_UNORM_PACK16) {
      if (pdev->support_a4b4g4r4) {
         } else {
      DZN_SWAP(float, out.float32[0], out.float32[1]);
                     }
      static void
   dzn_cmd_buffer_clear_ranges_with_copy(struct dzn_cmd_buffer *cmdbuf,
                                 {
      enum pipe_format pfmt = vk_format_to_pipe_format(image->vk.format);
   uint32_t blksize = util_format_get_blocksize(pfmt);
   uint8_t buf[D3D12_TEXTURE_DATA_PITCH_ALIGNMENT * 3] = { 0 };
            assert(blksize <= sizeof(raw));
                     uint32_t fill_step = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
   while (fill_step % blksize)
            uint32_t res_size = 0;
   for (uint32_t r = 0; r < range_count; r++) {
      uint32_t w = u_minify(image->vk.extent.width, ranges[r].baseMipLevel);
   uint32_t h = u_minify(image->vk.extent.height, ranges[r].baseMipLevel);
   uint32_t d = u_minify(image->vk.extent.depth, ranges[r].baseMipLevel);
                                 for (uint32_t i = 0; i < fill_step; i += blksize)
            ID3D12Resource *src_res;
            VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, res_size,
                              if (result != VK_SUCCESS)
                     uint8_t *cpu_ptr;
   ID3D12Resource_Map(src_res, 0, NULL, (void **)&cpu_ptr);
   cpu_ptr += src_offset;
   for (uint32_t i = 0; i < res_size; i += fill_step)
                     D3D12_TEXTURE_COPY_LOCATION src_loc = {
      .pResource = src_res,
   .Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
   .PlacedFootprint = {
                     for (uint32_t r = 0; r < range_count; r++) {
      uint32_t level_count = dzn_get_level_count(image, &ranges[r]);
            if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &ranges[r],
                           dzn_foreach_aspect(aspect, ranges[r].aspectMask) {
      for (uint32_t lvl = 0; lvl < level_count; lvl++) {
      uint32_t w = u_minify(image->vk.extent.width, ranges[r].baseMipLevel + lvl);
   uint32_t h = u_minify(image->vk.extent.height, ranges[r].baseMipLevel + lvl);
   uint32_t d = u_minify(image->vk.extent.depth, ranges[r].baseMipLevel + lvl);
   VkImageSubresourceLayers subres = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .mipLevel = ranges[r].baseMipLevel + lvl,
                     for (uint32_t layer = 0; layer < layer_count; layer++) {
                     src_loc.PlacedFootprint.Footprint.Format =
      dst_loc.Type == D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT ?
   dst_loc.PlacedFootprint.Footprint.Format :
      src_loc.PlacedFootprint.Footprint.Width = w;
   src_loc.PlacedFootprint.Footprint.Height = h;
   src_loc.PlacedFootprint.Footprint.Depth = d;
   src_loc.PlacedFootprint.Footprint.RowPitch =
         D3D12_BOX src_box = {
      .left = 0,
   .top = 0,
   .front = 0,
   .right = w,
                                                if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &ranges[r],
                        }
      static void
   dzn_cmd_buffer_clear_attachment(struct dzn_cmd_buffer *cmdbuf,
                                 struct dzn_image_view *view,
   VkImageLayout layout,
   const VkClearValue *value,
   {
      struct dzn_image *image =
         struct dzn_physical_device *pdev =
            VkImageSubresourceRange range = {
      .aspectMask = aspects,
   .baseMipLevel = view->vk.base_mip_level,
   .levelCount = 1,
   .baseArrayLayer = view->vk.base_array_layer + base_layer,
   .layerCount = layer_count == VK_REMAINING_ARRAY_LAYERS ?
               layer_count = vk_image_subresource_layer_count(&image->vk, &range);
            if (vk_format_is_depth_or_stencil(view->vk.format)) {
               if (aspects & VK_IMAGE_ASPECT_DEPTH_BIT)
         if (aspects & VK_IMAGE_ASPECT_STENCIL_BIT)
            if (flags != 0) {
      if (cmdbuf->enhanced_barriers) {
      restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, image,
            } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &range,
                           D3D12_DEPTH_STENCIL_VIEW_DESC desc = dzn_image_get_dsv_desc(image, &range, 0);
   D3D12_CPU_DESCRIPTOR_HANDLE handle = dzn_cmd_buffer_get_dsv(cmdbuf, image, &desc);
   ID3D12GraphicsCommandList1_ClearDepthStencilView(cmdbuf->cmdlist, handle, flags,
                        if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, image,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &range,
                        } else if (aspects & VK_IMAGE_ASPECT_COLOR_BIT) {
      VkClearColorValue color = adjust_clear_color(pdev, view->vk.format, &value->color);
   bool clear_with_cpy = false;
            if (vk_format_is_sint(view->vk.format)) {
      for (uint32_t i = 0; i < 4; i++) {
      vals[i] = color.int32[i];
   if (color.int32[i] != (int32_t)vals[i]) {
      clear_with_cpy = true;
            } else if (vk_format_is_uint(view->vk.format)) {
      for (uint32_t i = 0; i < 4; i++) {
      vals[i] = color.uint32[i];
   if (color.uint32[i] != (uint32_t)vals[i]) {
      clear_with_cpy = true;
            } else {
      for (uint32_t i = 0; i < 4; i++)
               if (clear_with_cpy) {
      dzn_cmd_buffer_clear_rects_with_copy(cmdbuf, image,
                  } else {
      if (cmdbuf->enhanced_barriers) {
      restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, image,
            } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &range,
                           D3D12_RENDER_TARGET_VIEW_DESC desc = dzn_image_get_rtv_desc(image, &range, 0);
                  if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, image,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &range,
                           }
      static void
   dzn_cmd_buffer_clear_color(struct dzn_cmd_buffer *cmdbuf,
                                 {
      struct dzn_physical_device *pdev =
         if (!(image->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) ||
      cmdbuf->type != D3D12_COMMAND_LIST_TYPE_DIRECT) {
   dzn_cmd_buffer_clear_ranges_with_copy(cmdbuf, image, layout, col, range_count, ranges);
               VkClearColorValue color = adjust_clear_color(pdev, image->vk.format, col);
            enum pipe_format pfmt = vk_format_to_pipe_format(image->vk.format);
            if (util_format_is_pure_sint(pfmt)) {
      for (uint32_t c = 0; c < ARRAY_SIZE(clear_vals); c++) {
      clear_vals[c] = color.int32[c];
   if (color.int32[c] != (int32_t)clear_vals[c]) {
      dzn_cmd_buffer_clear_ranges_with_copy(cmdbuf, image, layout, col, range_count, ranges);
            } else if (util_format_is_pure_uint(pfmt)) {
      for (uint32_t c = 0; c < ARRAY_SIZE(clear_vals); c++) {
      clear_vals[c] = color.uint32[c];
   if (color.uint32[c] != (uint32_t)clear_vals[c]) {
      dzn_cmd_buffer_clear_ranges_with_copy(cmdbuf, image, layout, col, range_count, ranges);
            } else {
                  for (uint32_t r = 0; r < range_count; r++) {
      const VkImageSubresourceRange *range = &ranges[r];
            if (cmdbuf->enhanced_barriers) {
      restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, image,
            } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                           for (uint32_t lvl = 0; lvl < level_count; lvl++) {
               if (image->vk.image_type == VK_IMAGE_TYPE_3D) {
      view_range.baseArrayLayer = 0;
               D3D12_RENDER_TARGET_VIEW_DESC desc = dzn_image_get_rtv_desc(image, &view_range, lvl);
   D3D12_CPU_DESCRIPTOR_HANDLE handle = dzn_cmd_buffer_get_rtv(cmdbuf, image, &desc);
               if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, image,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                        }
      static void
   dzn_cmd_buffer_clear_zs(struct dzn_cmd_buffer *cmdbuf,
                           const struct dzn_image *image,
   {
               for (uint32_t r = 0; r < range_count; r++) {
      const VkImageSubresourceRange *range = &ranges[r];
            D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
            if (range->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)
         if (range->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)
            if (cmdbuf->enhanced_barriers) {
      restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, image,
            } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                           for (uint32_t lvl = 0; lvl < level_count; lvl++) {
      D3D12_DEPTH_STENCIL_VIEW_DESC desc = dzn_image_get_dsv_desc(image, range, lvl);
   D3D12_CPU_DESCRIPTOR_HANDLE handle = dzn_cmd_buffer_get_dsv(cmdbuf, image, &desc);
   ID3D12GraphicsCommandList1_ClearDepthStencilView(cmdbuf->cmdlist,
                                 if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, image,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, range,
                        }
      static void
   dzn_cmd_buffer_copy_buf2img_region(struct dzn_cmd_buffer *cmdbuf,
                           {
      VK_FROM_HANDLE(dzn_buffer, src_buffer, info->srcBuffer);
   VK_FROM_HANDLE(dzn_image, dst_image, info->dstImage);
   struct dzn_physical_device *pdev =
                     VkBufferImageCopy2 region = info->pRegions[r];
   enum pipe_format pfmt = vk_format_to_pipe_format(dst_image->vk.format);
   uint32_t blkh = util_format_get_blockheight(pfmt);
            /* D3D12 wants block aligned offsets/extent, but vulkan allows the extent
   * to not be block aligned if it's reaching the image boundary, offsets still
   * have to be aligned. Align the image extent to make D3D12 happy.
   */
            D3D12_TEXTURE_COPY_LOCATION dst_img_loc =
         D3D12_TEXTURE_COPY_LOCATION src_buf_loc =
            if (dzn_buffer_supports_region_copy(pdev, &src_buf_loc)) {
      /* RowPitch and Offset are properly aligned, we can copy
   * the whole thing in one call.
   */
   D3D12_BOX src_box = {
      .left = 0,
   .top = 0,
   .front = 0,
   .right = region.imageExtent.width,
   .bottom = region.imageExtent.height,
               ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist, &dst_img_loc,
                                       /* Copy line-by-line if things are not properly aligned. */
   D3D12_BOX src_box = {
      .top = 0,
   .front = 0,
   .bottom = blkh,
               for (uint32_t z = 0; z < region.imageExtent.depth; z += blkd) {
      for (uint32_t y = 0; y < region.imageExtent.height; y += blkh) {
               D3D12_TEXTURE_COPY_LOCATION src_buf_line_loc =
      dzn_buffer_get_line_copy_loc(src_buffer, dst_image->vk.format,
               src_box.left = src_x;
   src_box.right = src_x + region.imageExtent.width;
   ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist,
                                          }
      static void
   dzn_cmd_buffer_copy_img2buf_region(struct dzn_cmd_buffer *cmdbuf,
                           {
      VK_FROM_HANDLE(dzn_image, src_image, info->srcImage);
   VK_FROM_HANDLE(dzn_buffer, dst_buffer, info->dstBuffer);
   struct dzn_physical_device *pdev =
                     VkBufferImageCopy2 region = info->pRegions[r];
   enum pipe_format pfmt = vk_format_to_pipe_format(src_image->vk.format);
   uint32_t blkh = util_format_get_blockheight(pfmt);
            /* D3D12 wants block aligned offsets/extent, but vulkan allows the extent
   * to not be block aligned if it's reaching the image boundary, offsets still
   * have to be aligned. Align the image extent to make D3D12 happy.
   */
            D3D12_TEXTURE_COPY_LOCATION src_img_loc =
         D3D12_TEXTURE_COPY_LOCATION dst_buf_loc =
            if (dzn_buffer_supports_region_copy(pdev, &dst_buf_loc)) {
      /* RowPitch and Offset are properly aligned on 256 bytes, we can copy
   * the whole thing in one call.
   */
   D3D12_BOX src_box = {
      .left = (UINT)region.imageOffset.x,
   .top = (UINT)region.imageOffset.y,
   .front = (UINT)region.imageOffset.z,
   .right = (UINT)(region.imageOffset.x + region.imageExtent.width),
   .bottom = (UINT)(region.imageOffset.y + region.imageExtent.height),
               ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist, &dst_buf_loc,
                           D3D12_BOX src_box = {
      .left = (UINT)region.imageOffset.x,
               /* Copy line-by-line if things are not properly aligned. */
   for (uint32_t z = 0; z < region.imageExtent.depth; z += blkd) {
      src_box.front = region.imageOffset.z + z;
            for (uint32_t y = 0; y < region.imageExtent.height; y += blkh) {
               D3D12_TEXTURE_COPY_LOCATION dst_buf_line_loc =
      dzn_buffer_get_line_copy_loc(dst_buffer, src_image->vk.format,
                              ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist,
                              }
      static void
   dzn_cmd_buffer_copy_img_chunk(struct dzn_cmd_buffer *cmdbuf,
                                 const VkCopyImageInfo2 *info,
   {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   VK_FROM_HANDLE(dzn_image, src, info->srcImage);
            ID3D12Device4 *dev = device->dev;
            VkImageCopy2 region = info->pRegions[r];
            const VkImageSubresourceLayers *src_subres = &region.srcSubresource;
   const VkImageSubresourceLayers *dst_subres = &region.dstSubresource;
   VkFormat src_format =
         VkFormat dst_format =
            enum pipe_format src_pfmt = vk_format_to_pipe_format(src_format);
   uint32_t src_blkw = util_format_get_blockwidth(src_pfmt);
   uint32_t src_blkh = util_format_get_blockheight(src_pfmt);
   uint32_t src_blkd = util_format_get_blockdepth(src_pfmt);
   enum pipe_format dst_pfmt = vk_format_to_pipe_format(dst_format);
   uint32_t dst_blkw = util_format_get_blockwidth(dst_pfmt);
   uint32_t dst_blkh = util_format_get_blockheight(dst_pfmt);
   uint32_t dst_blkd = util_format_get_blockdepth(dst_pfmt);
   uint32_t dst_z = region.dstOffset.z, src_z = region.srcOffset.z;
   uint32_t depth = region.extent.depth;
                     if (src->vk.image_type == VK_IMAGE_TYPE_3D &&
      dst->vk.image_type == VK_IMAGE_TYPE_2D) {
   assert(src_subres->layerCount == 1);
   src_l = 0;
   src_z += l;
      } else if (src->vk.image_type == VK_IMAGE_TYPE_2D &&
            assert(dst_subres->layerCount == 1);
   dst_l = 0;
   dst_z += l;
      } else {
                  D3D12_TEXTURE_COPY_LOCATION dst_loc = dzn_image_get_copy_loc(dst, dst_subres, aspect, dst_l);
            D3D12_BOX src_box = {
      .left = (UINT)MAX2(region.srcOffset.x, 0),
   .top = (UINT)MAX2(region.srcOffset.y, 0),
   .front = (UINT)MAX2(src_z, 0),
   .right = (UINT)region.srcOffset.x + region.extent.width,
   .bottom = (UINT)region.srcOffset.y + region.extent.height,
               if (!tmp_loc->pResource) {
      ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist, &dst_loc,
                                       tmp_desc->Format =
         tmp_desc->Width = region.extent.width;
            ID3D12Device1_GetCopyableFootprints(dev, tmp_desc,
                                 if (r > 0 || l > 0) {
      if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, tmp_loc->pResource,
            } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, tmp_loc->pResource, 0, 1,
                                       if (r > 0 || l > 0) {
      if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, tmp_loc->pResource,
            } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, tmp_loc->pResource, 0, 1,
                              tmp_desc->Format =
         if (src_blkw != dst_blkw)
         if (src_blkh != dst_blkh)
            ID3D12Device1_GetCopyableFootprints(device->dev, tmp_desc,
                        if (src_blkd != dst_blkd) {
      tmp_loc->PlacedFootprint.Footprint.Depth =
      } else {
                  D3D12_BOX tmp_box = {
      .left = 0,
   .top = 0,
   .front = 0,
   .right = tmp_loc->PlacedFootprint.Footprint.Width,
   .bottom = tmp_loc->PlacedFootprint.Footprint.Height,
               ID3D12GraphicsCommandList1_CopyTextureRegion(cmdlist, &dst_loc,
                        }
      static void
   dzn_cmd_buffer_blit_prepare_src_view(struct dzn_cmd_buffer *cmdbuf,
                                 {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   VK_FROM_HANDLE(dzn_image, img, image);
   VkImageViewCreateInfo iview_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
   .image = image,
   .format = img->vk.format,
   .subresourceRange = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .baseMipLevel = subres->mipLevel,
   .levelCount = 1,
   .baseArrayLayer = subres->baseArrayLayer,
                  switch (img->vk.image_type) {
   case VK_IMAGE_TYPE_1D:
      iview_info.viewType = img->vk.array_layers > 1 ?
            case VK_IMAGE_TYPE_2D:
      iview_info.viewType = img->vk.array_layers > 1 ?
            case VK_IMAGE_TYPE_3D:
      iview_info.viewType = VK_IMAGE_VIEW_TYPE_3D;
      default:
                  struct dzn_image_view iview;
   dzn_image_view_init(device, &iview, &iview_info);
   dzn_descriptor_heap_write_image_view_desc(device, heap, heap_slot, false, false, &iview);
            D3D12_GPU_DESCRIPTOR_HANDLE handle =
            }
      static void
   dzn_cmd_buffer_blit_prepare_dst_view(struct dzn_cmd_buffer *cmdbuf,
                     {
      bool ds = aspect & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
   VkImageSubresourceRange range = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .baseMipLevel = level,
   .levelCount = 1,
   .baseArrayLayer = layer,
               if (ds) {
      D3D12_DEPTH_STENCIL_VIEW_DESC desc = dzn_image_get_dsv_desc(img, &range, 0);
   D3D12_CPU_DESCRIPTOR_HANDLE handle = dzn_cmd_buffer_get_dsv(cmdbuf, img, &desc);
      } else {
      D3D12_RENDER_TARGET_VIEW_DESC desc = dzn_image_get_rtv_desc(img, &range, 0);
   D3D12_CPU_DESCRIPTOR_HANDLE handle = dzn_cmd_buffer_get_rtv(cmdbuf, img, &desc);
         }
      static void
   dzn_cmd_buffer_blit_set_pipeline(struct dzn_cmd_buffer *cmdbuf,
                                 {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   enum pipe_format pfmt = vk_format_to_pipe_format(dst->vk.format);
   VkImageUsageFlags usage =
      vk_format_is_depth_or_stencil(dst->vk.format) ?
   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
      struct dzn_meta_blit_key ctx_key = {
      .out_format = dzn_image_get_dxgi_format(pdev, dst->vk.format, usage, aspect),
   .samples = (uint32_t)src->vk.samples,
   .loc = (uint32_t)(aspect == VK_IMAGE_ASPECT_DEPTH_BIT ?
                     FRAG_RESULT_DEPTH :
   .out_type = (uint32_t)(util_format_is_pure_uint(pfmt) ? GLSL_TYPE_UINT :
                     .sampler_dim = (uint32_t)(src->vk.image_type == VK_IMAGE_TYPE_1D ? GLSL_SAMPLER_DIM_1D :
                     .src_is_array = src->vk.array_layers > 1,
   .resolve_mode = resolve_mode,
   .linear_filter = filter == VK_FILTER_LINEAR,
               const struct dzn_meta_blit *ctx =
                  cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |= DZN_CMD_BINDPOINT_DIRTY_PIPELINE;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].root_sig = NULL;
   ID3D12GraphicsCommandList1_SetGraphicsRootSignature(cmdbuf->cmdlist, ctx->root_sig);
      }
      static void
   dzn_cmd_buffer_blit_set_2d_region(struct dzn_cmd_buffer *cmdbuf,
                                    const struct dzn_image *src,
      {
      uint32_t dst_w = u_minify(dst->vk.extent.width, dst_subres->mipLevel);
   uint32_t dst_h = u_minify(dst->vk.extent.height, dst_subres->mipLevel);
   uint32_t src_w = u_minify(src->vk.extent.width, src_subres->mipLevel);
            float dst_pos[4] = {
      (2 * (float)dst_offsets[0].x / (float)dst_w) - 1.0f, -((2 * (float)dst_offsets[0].y / (float)dst_h) - 1.0f),
               float src_pos[4] = {
      (float)src_offsets[0].x, (float)src_offsets[0].y,
               if (normalize_src_coords) {
      src_pos[0] /= src_w;
   src_pos[1] /= src_h;
   src_pos[2] /= src_w;
               float coords[] = {
      dst_pos[0], dst_pos[1], src_pos[0], src_pos[1],
   dst_pos[2], dst_pos[1], src_pos[2], src_pos[1],
   dst_pos[0], dst_pos[3], src_pos[0], src_pos[3],
                        D3D12_VIEWPORT vp = {
      .TopLeftX = 0,
   .TopLeftY = 0,
   .Width = (float)dst_w,
   .Height = (float)dst_h,
   .MinDepth = 0,
      };
            D3D12_RECT scissor = {
      .left = MIN2(dst_offsets[0].x, dst_offsets[1].x),
   .top = MIN2(dst_offsets[0].y, dst_offsets[1].y),
   .right = MAX2(dst_offsets[0].x, dst_offsets[1].x),
      };
      }
      static void
   dzn_cmd_buffer_blit_issue_barriers(struct dzn_cmd_buffer *cmdbuf,
                                    struct dzn_image *src, VkImageLayout src_layout,
      {
      VkImageSubresourceRange src_range = {
      .aspectMask = aspect,
   .baseMipLevel = src_subres->mipLevel,
   .levelCount = 1,
   .baseArrayLayer = src_subres->baseArrayLayer,
      };
   VkImageSubresourceRange dst_range = {
      .aspectMask = aspect,
   .baseMipLevel = dst_subres->mipLevel,
   .levelCount = 1,
   .baseArrayLayer = dst_subres->baseArrayLayer,
               if (!post) {
      if (cmdbuf->enhanced_barriers) {
      *restore_dst_layout = dzn_cmd_buffer_require_layout(cmdbuf, dst,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, src, &src_range,
                     dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, dst, &dst_range,
                     } else {
      if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, dst,
                  } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, src, &src_range,
                     dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, dst, &dst_range,
                        }
      static void
   dzn_cmd_buffer_blit_region(struct dzn_cmd_buffer *cmdbuf,
                           {
      VK_FROM_HANDLE(dzn_image, src, info->srcImage);
            const VkImageBlit2 *region = &info->pRegions[r];
   bool src_is_3d = src->vk.image_type == VK_IMAGE_TYPE_3D;
            dzn_foreach_aspect(aspect, region->srcSubresource.aspectMask) {
      D3D12_BARRIER_LAYOUT restore_dst_layout = D3D12_BARRIER_LAYOUT_COMMON;
   dzn_cmd_buffer_blit_set_pipeline(cmdbuf, src, dst, aspect, info->filter, dzn_blit_resolve_none);
   dzn_cmd_buffer_blit_issue_barriers(cmdbuf,
                     dzn_cmd_buffer_blit_prepare_src_view(cmdbuf, info->srcImage,
               dzn_cmd_buffer_blit_set_2d_region(cmdbuf,
                        uint32_t dst_depth =
      region->dstOffsets[1].z > region->dstOffsets[0].z ?
   region->dstOffsets[1].z - region->dstOffsets[0].z :
      uint32_t src_depth =
      region->srcOffsets[1].z > region->srcOffsets[0].z ?
               uint32_t layer_count = dzn_get_layer_count(src, &region->srcSubresource);
            float src_slice_step = src_is_3d ? (float)src_depth / dst_depth : 1;
   if (region->srcOffsets[0].z > region->srcOffsets[1].z)
         float src_z_coord =
         uint32_t slice_count = dst_is_3d ? dst_depth : layer_count;
   uint32_t dst_z_coord =
         if (region->dstOffsets[0].z > region->dstOffsets[1].z)
            uint32_t dst_slice_step = region->dstOffsets[0].z < region->dstOffsets[1].z ?
            /* Normalize the src coordinates/step */
   if (src_is_3d) {
      src_z_coord /= src->vk.extent.depth;
               for (uint32_t slice = 0; slice < slice_count; slice++) {
      dzn_cmd_buffer_blit_prepare_dst_view(cmdbuf, dst, aspect, dst_level, dst_z_coord);
   ID3D12GraphicsCommandList1_SetGraphicsRoot32BitConstants(cmdbuf->cmdlist, 1, 1, &src_z_coord, 16);
   ID3D12GraphicsCommandList1_DrawInstanced(cmdbuf->cmdlist, 4, 1, 0, 0);
   src_z_coord += src_slice_step;
               dzn_cmd_buffer_blit_issue_barriers(cmdbuf,
                     }
      static enum dzn_blit_resolve_mode
   get_blit_resolve_mode(VkResolveModeFlagBits mode)
   {
      switch (mode) {
   case VK_RESOLVE_MODE_AVERAGE_BIT: return dzn_blit_resolve_average;
   case VK_RESOLVE_MODE_MIN_BIT: return dzn_blit_resolve_min;
   case VK_RESOLVE_MODE_MAX_BIT: return dzn_blit_resolve_max;
   case VK_RESOLVE_MODE_SAMPLE_ZERO_BIT: return dzn_blit_resolve_sample_zero;
   default: unreachable("Unexpected resolve mode");
      }
      static void
   dzn_cmd_buffer_resolve_region(struct dzn_cmd_buffer *cmdbuf,
                                 {
      VK_FROM_HANDLE(dzn_image, src, info->srcImage);
                     dzn_foreach_aspect(aspect, region->srcSubresource.aspectMask) {
      D3D12_BARRIER_LAYOUT restore_dst_layout = D3D12_BARRIER_LAYOUT_COMMON;
   dzn_cmd_buffer_blit_set_pipeline(cmdbuf, src, dst, aspect, VK_FILTER_NEAREST, get_blit_resolve_mode(mode));
   dzn_cmd_buffer_blit_issue_barriers(cmdbuf,
                     dzn_cmd_buffer_blit_prepare_src_view(cmdbuf, info->srcImage, aspect,
                  VkOffset3D src_offset[2] = {
      {
      .x = region->srcOffset.x,
      },
   {
      .x = (int32_t)(region->srcOffset.x + region->extent.width),
         };
   VkOffset3D dst_offset[2] = {
      {
      .x = region->dstOffset.x,
      },
   {
      .x = (int32_t)(region->dstOffset.x + region->extent.width),
                  dzn_cmd_buffer_blit_set_2d_region(cmdbuf,
                        uint32_t layer_count = dzn_get_layer_count(src, &region->srcSubresource);
   for (uint32_t layer = 0; layer < layer_count; layer++) {
               dzn_cmd_buffer_blit_prepare_dst_view(cmdbuf,
               ID3D12GraphicsCommandList1_SetGraphicsRoot32BitConstants(cmdbuf->cmdlist, 1, 1, &src_z_coord, 16);
               dzn_cmd_buffer_blit_issue_barriers(cmdbuf,
                     }
      static void
   dzn_cmd_buffer_update_pipeline(struct dzn_cmd_buffer *cmdbuf, uint32_t bindpoint)
   {
               if (!pipeline)
            struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   ID3D12PipelineState *old_pipeline_state =
            if (cmdbuf->state.bindpoint[bindpoint].dirty & DZN_CMD_BINDPOINT_DIRTY_PIPELINE) {
      if (cmdbuf->state.bindpoint[bindpoint].root_sig != pipeline->root.sig) {
      cmdbuf->state.bindpoint[bindpoint].root_sig = pipeline->root.sig;
                  if (device->bindless) {
      /* Note: The D3D12 spec for descriptor heap indexing requires that the descriptor heaps
   * are bound *before* the root signature. */
   bool bind_heaps = false;
   dzn_foreach_pool_type(type) {
      if (cmdbuf->state.heaps[type] != &device->device_heaps[type].heap) {
      bind_heaps = true;
         }
   if (bind_heaps) {
      ID3D12DescriptorHeap *heaps[NUM_POOL_TYPES];
   dzn_foreach_pool_type(type)
                        if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
         else
      }
   if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      struct dzn_graphics_pipeline *gfx =
         ID3D12GraphicsCommandList1_IASetPrimitiveTopology(cmdbuf->cmdlist, gfx->ia.topology);
   dzn_graphics_pipeline_get_state(gfx, &cmdbuf->state.pipeline_variant);
   if (gfx->multiview.native_view_instancing)
                        if (gfx->zsa.dynamic_depth_bias && gfx->use_gs_for_polygon_mode_point)
                           if (old_pipeline_state != new_pipeline_state) {
      ID3D12GraphicsCommandList1_SetPipelineState(cmdbuf->cmdlist, pipeline->state);
         }
      static void
   dzn_cmd_buffer_update_heaps(struct dzn_cmd_buffer *cmdbuf, uint32_t bindpoint)
   {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_descriptor_state *desc_state =
         struct dzn_descriptor_heap *new_heaps[NUM_POOL_TYPES] = {
      desc_state->heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
      };
   uint32_t new_heap_offsets[NUM_POOL_TYPES] = { 0 };
   bool update_root_desc_table[NUM_POOL_TYPES] = { 0 };
   const struct dzn_pipeline *pipeline =
            /* The set of dirty bits that are cleared by running this function. Notably, 
   * for bindless, descriptor sets that are bound but unused by the currently
   * set pipeline are not processed, meaning their dirty bits should persist 
   * until such a point as a pipeline does use them. For not-bindless,
   * all sets are processed. */
   uint32_t dirty_bits_bindless =
      (pipeline->dynamic_buffer_count ? DZN_CMD_BINDPOINT_DIRTY_DYNAMIC_BUFFERS : 0) |
      uint32_t dirty_bits = (device->bindless ? dirty_bits_bindless : DZN_CMD_BINDPOINT_DIRTY_DESC_SETS | DZN_CMD_BINDPOINT_DIRTY_DYNAMIC_BUFFERS);
   if (!(cmdbuf->state.bindpoint[bindpoint].dirty & dirty_bits))
            dzn_foreach_pool_type (type) {
      if (device->bindless) {
         } else {
      uint32_t desc_count = pipeline->desc_count[type];
                  struct dzn_descriptor_heap_pool *pool =
      type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ?
                     dzn_descriptor_heap_pool_alloc_slots(pool, device, desc_count,
                        for (uint32_t s = 0; s < MAX_SETS; s++) {
                     uint32_t set_heap_offset = pipeline->sets[s].heap_offsets[type];
   uint32_t set_desc_count = MIN2(pipeline->sets[s].range_desc_count[type], set->heap_sizes[type]);
   if (set_desc_count) {
      dzn_descriptor_heap_copy(device, dst_heap, dst_heap_offset + set_heap_offset,
                     if (type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) {
      uint32_t dynamic_buffer_count = pipeline->sets[s].dynamic_buffer_count;
   for (uint32_t o = 0; o < dynamic_buffer_count; o++) {
      struct dzn_buffer_desc bdesc = set->dynamic_buffers[o];
                        bool primary_is_writable = bdesc.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
   uint32_t desc_heap_offset = pipeline->sets[s].dynamic_buffer_heap_offsets[o].primary;
                        if (pipeline->sets[s].dynamic_buffer_heap_offsets[o].alt != ~0) {
      assert(primary_is_writable);
   desc_heap_offset = pipeline->sets[s].dynamic_buffer_heap_offsets[o].alt;
   dzn_descriptor_heap_write_buffer_desc(device, dst_heap,
                                             if (new_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] != cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] ||
      new_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] != cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]) {
   ID3D12DescriptorHeap *desc_heaps[2];
   uint32_t num_desc_heaps = 0;
   if (new_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV])
         if (new_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER])
                  for (unsigned h = 0; h < ARRAY_SIZE(cmdbuf->state.heaps); h++)
               if (!device->bindless) {
      for (uint32_t r = 0; r < pipeline->root.sets_param_count; r++) {
                                             if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
         else
                  if (device->bindless) {
      for (uint32_t s = 0; s < pipeline->set_count; ++s) {
      const struct dzn_descriptor_set *set = desc_state->sets[s].set;
                  uint32_t dirty_bit = DZN_CMD_BINDPOINT_DIRTY_DESC_SET0 << s;
   if (cmdbuf->state.bindpoint[bindpoint].dirty & dirty_bit) {
      uint64_t gpuva = set->pool->bindless.gpuva + (set->heap_offsets[0] * sizeof(struct dxil_spirv_bindless_entry));
   if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
         else
         }
   if (pipeline->dynamic_buffer_count &&
      (cmdbuf->state.bindpoint[bindpoint].dirty & DZN_CMD_BINDPOINT_DIRTY_DYNAMIC_BUFFERS)) {
   ID3D12Resource *dynamic_buffer_buf = NULL;
   uint64_t dynamic_buffer_buf_offset;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, sizeof(struct dxil_spirv_bindless_entry) * pipeline->dynamic_buffer_count,
                                             uint64_t gpuva = ID3D12Resource_GetGPUVirtualAddress(dynamic_buffer_buf) + dynamic_buffer_buf_offset;
   struct dxil_spirv_bindless_entry *map;
                  for (uint32_t s = 0; s < MAX_SETS; ++s) {
      const struct dzn_descriptor_set *set = desc_state->sets[s].set;
                  uint32_t dynamic_buffer_count = pipeline->sets[s].dynamic_buffer_count;
   for (uint32_t o = 0; o < dynamic_buffer_count; o++) {
      const struct dzn_buffer_desc *bdesc = &set->dynamic_buffers[o];
   volatile struct dxil_spirv_bindless_entry *map_entry = &map[pipeline->sets[s].dynamic_buffer_heap_offsets[o].primary];
   struct dzn_buffer_desc bdesc_updated = *bdesc;
   bdesc_updated.offset += cmdbuf->state.bindpoint[bindpoint].desc_state.sets[s].dynamic_offsets[o];
                  ID3D12Resource_Unmap(dynamic_buffer_buf, 0, NULL);
   if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
      ID3D12GraphicsCommandList1_SetGraphicsRootShaderResourceView(cmdbuf->cmdlist,
            else
      ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(cmdbuf->cmdlist,
                        }
      static void
   dzn_cmd_buffer_update_sysvals(struct dzn_cmd_buffer *cmdbuf, uint32_t bindpoint)
   {
      if (!(cmdbuf->state.bindpoint[bindpoint].dirty & DZN_CMD_BINDPOINT_DIRTY_SYSVALS))
            const struct dzn_pipeline *pipeline = cmdbuf->state.bindpoint[bindpoint].pipeline;
            if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      ID3D12GraphicsCommandList1_SetGraphicsRoot32BitConstants(cmdbuf->cmdlist, sysval_cbv_param_idx,
            } else {
      ID3D12GraphicsCommandList1_SetComputeRoot32BitConstants(cmdbuf->cmdlist, sysval_cbv_param_idx,
                        }
      static void
   dzn_cmd_buffer_update_viewports(struct dzn_cmd_buffer *cmdbuf)
   {
      const struct dzn_graphics_pipeline *pipeline =
            if (!(cmdbuf->state.dirty & DZN_CMD_DIRTY_VIEWPORTS) ||
      !pipeline->vp.count)
            }
      static void
   dzn_cmd_buffer_update_scissors(struct dzn_cmd_buffer *cmdbuf)
   {
      const struct dzn_graphics_pipeline *pipeline =
            if (!(cmdbuf->state.dirty & DZN_CMD_DIRTY_SCISSORS))
            if (!pipeline->scissor.count) {
      /* Apply a scissor delimiting the render area. */
   ID3D12GraphicsCommandList1_RSSetScissorRects(cmdbuf->cmdlist, 1, &cmdbuf->state.render.area);
                        memcpy(scissors, cmdbuf->state.scissors, sizeof(D3D12_RECT) * pipeline->scissor.count);
   for (uint32_t i = 0; i < pipeline->scissor.count; i++) {
      scissors[i].left = MAX2(scissors[i].left, cmdbuf->state.render.area.left);
   scissors[i].top = MAX2(scissors[i].top, cmdbuf->state.render.area.top);
   scissors[i].right = MIN2(scissors[i].right, cmdbuf->state.render.area.right);
                  }
      static void
   dzn_cmd_buffer_update_vbviews(struct dzn_cmd_buffer *cmdbuf)
   {
               BITSET_FOREACH_RANGE(start, end, cmdbuf->state.vb.dirty, MAX_VBS)
               }
      static void
   dzn_cmd_buffer_update_ibview(struct dzn_cmd_buffer *cmdbuf)
   {
      if (!(cmdbuf->state.dirty & DZN_CMD_DIRTY_IB))
               }
      static void
   dzn_cmd_buffer_update_push_constants(struct dzn_cmd_buffer *cmdbuf, uint32_t bindpoint)
   {
      struct dzn_cmd_buffer_push_constant_state *state =
      bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS ?
         uint32_t offset = state->offset / 4;
   uint32_t end = ALIGN(state->end, 4) / 4;
            if (!count)
            uint32_t slot = cmdbuf->state.pipeline->root.push_constant_cbv_param_idx;
            if (bindpoint == VK_PIPELINE_BIND_POINT_GRAPHICS)
         else
            state->offset = 0;
      }
      static void
   dzn_cmd_buffer_update_zsa(struct dzn_cmd_buffer *cmdbuf)
   {
      struct dzn_physical_device *pdev =
         if (cmdbuf->state.dirty & DZN_CMD_DIRTY_STENCIL_REF) {
      const struct dzn_graphics_pipeline *gfx = (const struct dzn_graphics_pipeline *)
         if (cmdbuf->cmdlist8 &&
      pdev->options14.IndependentFrontAndBackStencilRefMaskSupported) {
   ID3D12GraphicsCommandList8_OMSetFrontAndBackStencilRef(cmdbuf->cmdlist8,
            } else {
      uint32_t ref =
      gfx->zsa.stencil_test.front.uses_ref ?
   cmdbuf->state.zsa.stencil_test.front.ref :
               }
      static void
   dzn_cmd_buffer_update_blend_constants(struct dzn_cmd_buffer *cmdbuf)
   {
      if (cmdbuf->state.dirty & DZN_CMD_DIRTY_BLEND_CONSTANTS)
      ID3D12GraphicsCommandList1_OMSetBlendFactor(cmdbuf->cmdlist,
   }
      static void
   dzn_cmd_buffer_update_depth_bounds(struct dzn_cmd_buffer *cmdbuf)
   {
      if (cmdbuf->state.dirty & DZN_CMD_DIRTY_DEPTH_BOUNDS) {
      ID3D12GraphicsCommandList1_OMSetDepthBounds(cmdbuf->cmdlist,
               }
      static void
   dzn_cmd_buffer_update_depth_bias(struct dzn_cmd_buffer *cmdbuf)
   {
      if (cmdbuf->state.dirty & DZN_CMD_DIRTY_DEPTH_BIAS) {
      assert(cmdbuf->cmdlist9);
   ID3D12GraphicsCommandList9_RSSetDepthBias(cmdbuf->cmdlist9,
                     }
      static VkResult
   dzn_cmd_buffer_triangle_fan_create_index(struct dzn_cmd_buffer *cmdbuf, uint32_t *vertex_count)
   {
      uint8_t index_size = *vertex_count <= 0xffff ? 2 : 4;
            *vertex_count = triangle_count * 3;
   if (!*vertex_count)
            ID3D12Resource *index_buf;
   uint64_t index_offset;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, *vertex_count * index_size,
                              if (result != VK_SUCCESS)
            void *cpu_ptr;
   ID3D12Resource_Map(index_buf, 0, NULL, &cpu_ptr);
            /* TODO: VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT */
   if (index_size == 2) {
      uint16_t *indices = (uint16_t *)cpu_ptr;
   for (uint32_t t = 0; t < triangle_count; t++) {
      indices[t * 3] = t + 1;
   indices[(t * 3) + 1] = t + 2;
      }
      } else {
      uint32_t *indices = (uint32_t *)cpu_ptr;
   for (uint32_t t = 0; t < triangle_count; t++) {
      indices[t * 3] = t + 1;
   indices[(t * 3) + 1] = t + 2;
      }
               cmdbuf->state.ib.view.SizeInBytes = *vertex_count * index_size;
   cmdbuf->state.ib.view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(index_buf) + index_offset;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_IB;
      }
      static VkResult
   dzn_cmd_buffer_triangle_fan_rewrite_index(struct dzn_cmd_buffer *cmdbuf,
               {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
            *index_count = triangle_count * 3;
   if (!*index_count)
            /* New index is always 32bit to make the compute shader rewriting the
   * index simpler */
   ID3D12Resource *new_index_buf;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, *index_count * 4,
                              if (result != VK_SUCCESS)
            D3D12_GPU_VIRTUAL_ADDRESS old_index_buf_gpu =
            ASSERTED const struct dzn_graphics_pipeline *gfx_pipeline = (const struct dzn_graphics_pipeline *)
         ASSERTED bool prim_restart =
                     enum dzn_index_type index_type =
         const struct dzn_meta_triangle_fan_rewrite_index *rewrite_index =
            struct dzn_triangle_fan_rewrite_index_params params = {
                  cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |= DZN_CMD_BINDPOINT_DIRTY_PIPELINE;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].root_sig = NULL;
   ID3D12GraphicsCommandList1_SetComputeRootSignature(cmdbuf->cmdlist, rewrite_index->root_sig);
   ID3D12GraphicsCommandList1_SetPipelineState(cmdbuf->cmdlist, rewrite_index->pipeline_state);
   ID3D12GraphicsCommandList1_SetComputeRootUnorderedAccessView(cmdbuf->cmdlist, 0, ID3D12Resource_GetGPUVirtualAddress(new_index_buf));
   ID3D12GraphicsCommandList1_SetComputeRoot32BitConstants(cmdbuf->cmdlist, 1, sizeof(params) / 4,
         ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(cmdbuf->cmdlist, 2, old_index_buf_gpu);
            if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, new_index_buf,
            } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, new_index_buf, 0, 1,
                           /* We don't mess up with the driver state when executing our internal
   * compute shader, but we still change the D3D12 state, so let's mark
   * things dirty if needed.
   */
   cmdbuf->state.pipeline = NULL;
   if (cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].pipeline) {
      cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |=
               cmdbuf->state.ib.view.SizeInBytes = *index_count * 4;
   cmdbuf->state.ib.view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(new_index_buf);
   cmdbuf->state.ib.view.Format = DXGI_FORMAT_R32_UINT;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_IB;
   *first_index = 0;
      }
      static void
   dzn_cmd_buffer_prepare_draw(struct dzn_cmd_buffer *cmdbuf, bool indexed)
   {
      if (indexed)
            dzn_cmd_buffer_update_pipeline(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS);
   dzn_cmd_buffer_update_heaps(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS);
   dzn_cmd_buffer_update_sysvals(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS);
   dzn_cmd_buffer_update_viewports(cmdbuf);
   dzn_cmd_buffer_update_scissors(cmdbuf);
   dzn_cmd_buffer_update_vbviews(cmdbuf);
   dzn_cmd_buffer_update_push_constants(cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS);
   dzn_cmd_buffer_update_zsa(cmdbuf);
   dzn_cmd_buffer_update_blend_constants(cmdbuf);
   dzn_cmd_buffer_update_depth_bounds(cmdbuf);
            /* Reset the dirty states */
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty &= DZN_CMD_BINDPOINT_DIRTY_HEAPS;
      }
      static uint32_t
   dzn_cmd_buffer_triangle_fan_get_max_index_buf_size(struct dzn_cmd_buffer *cmdbuf, bool indexed)
   {
      struct dzn_graphics_pipeline *pipeline = (struct dzn_graphics_pipeline *)
            if (!pipeline->ia.triangle_fan)
                     if (indexed) {
      uint32_t index_size = cmdbuf->state.ib.view.Format == DXGI_FORMAT_R32_UINT ? 4 : 2;
               } else {
      uint32_t max_vertex = 0;
   for (uint32_t i = 0; i < pipeline->vb.count; i++) {
      max_vertex =
      MAX2(max_vertex,
                           }
      static void
   dzn_cmd_buffer_indirect_draw(struct dzn_cmd_buffer *cmdbuf,
                              ID3D12Resource *draw_buf,
   size_t draw_buf_offset,
      {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_graphics_pipeline *pipeline = (struct dzn_graphics_pipeline *)
         uint32_t min_draw_buf_stride =
      indexed ?
   sizeof(struct dzn_indirect_indexed_draw_params) :
      bool prim_restart =
            draw_buf_stride = draw_buf_stride ? draw_buf_stride : min_draw_buf_stride;
   assert(draw_buf_stride >= min_draw_buf_stride);
            uint32_t triangle_fan_index_buf_stride =
      dzn_cmd_buffer_triangle_fan_get_max_index_buf_size(cmdbuf, indexed) *
      uint32_t exec_buf_stride =
      triangle_fan_index_buf_stride > 0 ?
   sizeof(struct dzn_indirect_triangle_fan_draw_exec_params) :
      uint32_t triangle_fan_exec_buf_stride =
         uint32_t exec_buf_size = max_draw_count * exec_buf_stride;
            // We reserve the first slot for the draw_count value when indirect count is
   // involved.
   if (count_buf != NULL) {
      exec_buf_size += exec_buf_stride;
               ID3D12Resource *exec_buf;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, exec_buf_size,
                        if (result != VK_SUCCESS)
            D3D12_GPU_VIRTUAL_ADDRESS draw_buf_gpu =
         ID3D12Resource *triangle_fan_index_buf = NULL;
            if (triangle_fan_index_buf_stride) {
      result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf,
                              if (result != VK_SUCCESS)
            result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf,
                              if (result != VK_SUCCESS)
               struct dzn_indirect_draw_triangle_fan_prim_restart_rewrite_params params = {
      .draw_buf_stride = draw_buf_stride,
   .triangle_fan_index_buf_stride = triangle_fan_index_buf_stride,
   .triangle_fan_index_buf_start =
      triangle_fan_index_buf ?
      .exec_buf_start =
      prim_restart ?
   };
   uint32_t params_size;
   if (triangle_fan_index_buf_stride > 0 && prim_restart)
         else if (triangle_fan_index_buf_stride > 0)
         else
                     if (indexed && triangle_fan_index_buf_stride > 0) {
      if (prim_restart && count_buf)
         else if (prim_restart && !count_buf)
         else if (!prim_restart && count_buf)
         else
      } else if (!indexed && triangle_fan_index_buf_stride > 0) {
      draw_type = count_buf ?
            } else if (indexed) {
      draw_type = count_buf ?
            } else {
                  struct dzn_meta_indirect_draw *indirect_draw = &device->indirect_draws[draw_type];
            cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |= DZN_CMD_BINDPOINT_DIRTY_PIPELINE;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].root_sig = NULL;
   ID3D12GraphicsCommandList1_SetComputeRootSignature(cmdbuf->cmdlist, indirect_draw->root_sig);
   ID3D12GraphicsCommandList1_SetPipelineState(cmdbuf->cmdlist, indirect_draw->pipeline_state);
   ID3D12GraphicsCommandList1_SetComputeRoot32BitConstants(cmdbuf->cmdlist, root_param_idx++,
         ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(cmdbuf->cmdlist, root_param_idx++,
         ID3D12GraphicsCommandList1_SetComputeRootUnorderedAccessView(cmdbuf->cmdlist, root_param_idx++,
         if (count_buf) {
      ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(cmdbuf->cmdlist,
                           if (triangle_fan_exec_buf) {
      ID3D12GraphicsCommandList1_SetComputeRootUnorderedAccessView(cmdbuf->cmdlist,
                              D3D12_INDEX_BUFFER_VIEW ib_view = { 0 };
   D3D12_BUFFER_BARRIER buf_barriers[2];
   D3D12_BARRIER_GROUP enhanced_barriers = {
      .NumBarriers = 0,
   .Type = D3D12_BARRIER_TYPE_BUFFER,
               if (triangle_fan_exec_buf) {
      enum dzn_index_type index_type =
      indexed ?
   dzn_index_type_from_dxgi_format(cmdbuf->state.ib.view.Format, prim_restart) :
      struct dzn_meta_triangle_fan_rewrite_index *rewrite_index =
                     assert(rewrite_index->root_sig);
   assert(rewrite_index->pipeline_state);
            if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, triangle_fan_exec_buf,
            } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, triangle_fan_exec_buf, 0, 1,
                           cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |= DZN_CMD_BINDPOINT_DIRTY_PIPELINE;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].root_sig = NULL;
   ID3D12GraphicsCommandList1_SetComputeRootSignature(cmdbuf->cmdlist, rewrite_index->root_sig);
   ID3D12GraphicsCommandList1_SetPipelineState(cmdbuf->cmdlist, rewrite_index->pipeline_state);
   root_param_idx = 0;
   ID3D12GraphicsCommandList1_SetComputeRootUnorderedAccessView(cmdbuf->cmdlist, root_param_idx++,
         ID3D12GraphicsCommandList1_SetComputeRoot32BitConstants(cmdbuf->cmdlist, root_param_idx++,
                  if (indexed) {
      ID3D12GraphicsCommandList1_SetComputeRootShaderResourceView(cmdbuf->cmdlist,
                     ID3D12GraphicsCommandList1_ExecuteIndirect(cmdbuf->cmdlist, rewrite_index->cmd_sig,
                  if (cmdbuf->enhanced_barriers) {
      buf_barriers[enhanced_barriers.NumBarriers++] = (D3D12_BUFFER_BARRIER){
      .SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
   .SyncAfter = D3D12_BARRIER_SYNC_INDEX_INPUT,
   .AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
   .AccessAfter = D3D12_BARRIER_ACCESS_INDEX_BUFFER,
   .pResource = triangle_fan_index_buf,
         } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, triangle_fan_index_buf, 0, 1,
                           /* After our triangle-fan lowering the draw is indexed */
   indexed = true;
   ib_view = cmdbuf->state.ib.view;
   cmdbuf->state.ib.view.BufferLocation = ID3D12Resource_GetGPUVirtualAddress(triangle_fan_index_buf);
   cmdbuf->state.ib.view.SizeInBytes = triangle_fan_index_buf_stride;
   cmdbuf->state.ib.view.Format = DXGI_FORMAT_R32_UINT;
               if (cmdbuf->enhanced_barriers) {
      buf_barriers[enhanced_barriers.NumBarriers++] = (D3D12_BUFFER_BARRIER){
      .SyncBefore = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
   .SyncAfter = D3D12_BARRIER_SYNC_EXECUTE_INDIRECT,
   .AccessBefore = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
   .AccessAfter = D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT,
   .pResource = exec_buf,
      };
      } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, exec_buf, 0, 1,
                           /* We don't mess up with the driver state when executing our internal
   * compute shader, but we still change the D3D12 state, so let's mark
   * things dirty if needed.
   */
   cmdbuf->state.pipeline = NULL;
   if (cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].pipeline) {
      cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |=
               enum dzn_indirect_draw_cmd_sig_type cmd_sig_type =
      triangle_fan_index_buf_stride > 0 ?
   DZN_INDIRECT_DRAW_TRIANGLE_FAN_CMD_SIG :
   indexed ?
   DZN_INDIRECT_INDEXED_DRAW_CMD_SIG :
      ID3D12CommandSignature *cmdsig =
            if (!cmdsig) {
      vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_DEVICE_MEMORY);
               cmdbuf->state.sysvals.gfx.first_vertex = 0;
   cmdbuf->state.sysvals.gfx.base_instance = 0;
            uint32_t view_mask = pipeline->multiview.native_view_instancing ?
         u_foreach_bit(view, view_mask) {
      cmdbuf->state.sysvals.gfx.view_index = view;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
                     ID3D12GraphicsCommandList1_ExecuteIndirect(cmdbuf->cmdlist, cmdsig,
                           /* Restore the old IB view if we modified it during the triangle fan lowering */
   if (ib_view.SizeInBytes) {
      cmdbuf->state.ib.view = ib_view;
         }
      static void
   dzn_cmd_buffer_prepare_dispatch(struct dzn_cmd_buffer *cmdbuf)
   {
      dzn_cmd_buffer_update_pipeline(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE);
   dzn_cmd_buffer_update_heaps(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE);
   dzn_cmd_buffer_update_sysvals(cmdbuf, VK_PIPELINE_BIND_POINT_COMPUTE);
            /* Reset the dirty states */
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdCopyBuffer2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(dzn_buffer, src_buffer, info->srcBuffer);
            for (int i = 0; i < info->regionCount; i++) {
               ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, dst_buffer->res, region->dstOffset,
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdCopyBufferToImage2(VkCommandBuffer commandBuffer,
         {
               for (int i = 0; i < info->regionCount; i++) {
               dzn_foreach_aspect(aspect, region->imageSubresource.aspectMask) {
      for (uint32_t l = 0; l < region->imageSubresource.layerCount; l++)
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdCopyImageToBuffer2(VkCommandBuffer commandBuffer,
         {
               for (int i = 0; i < info->regionCount; i++) {
               dzn_foreach_aspect(aspect, region->imageSubresource.aspectMask) {
      for (uint32_t l = 0; l < region->imageSubresource.layerCount; l++)
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdCopyImage2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev = container_of(device->vk.physical, struct dzn_physical_device, vk);
   VK_FROM_HANDLE(dzn_image, src, info->srcImage);
                              for (uint32_t i = 0; i < info->regionCount && !requires_temp_res; i++) {
               dzn_foreach_aspect(aspect, region->srcSubresource.aspectMask) {
               if (!dzn_image_formats_are_compatible(device, src->vk.format, dst->vk.format,
            src->vk.tiling != VK_IMAGE_TILING_LINEAR &&
   dst->vk.tiling != VK_IMAGE_TILING_LINEAR) {
   requires_temp_res = true;
                     bool use_blit = false;
   if (src->vk.samples > 1) {
               for (int i = 0; i < info->regionCount; i++) {
      const VkImageCopy2 *region = info->pRegions + i;
   if (region->srcOffset.x != 0 || region->srcOffset.y != 0 ||
      region->extent.width != u_minify(src->vk.extent.width, region->srcSubresource.mipLevel) ||
   region->extent.height != u_minify(src->vk.extent.height, region->srcSubresource.mipLevel) ||
   region->dstOffset.x != 0 || region->dstOffset.y != 0 ||
   region->extent.width != u_minify(dst->vk.extent.width, region->dstSubresource.mipLevel) ||
   region->extent.height != u_minify(dst->vk.extent.height, region->dstSubresource.mipLevel))
               if (use_blit) {
      /* This copy -> blit lowering doesn't work if the vkCmdCopyImage[2]() is
   * is issued on a transfer queue, but we don't have any better option
   * right now...
   */
            VkBlitImageInfo2 blit_info = {
      .sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2,
   .srcImage = info->srcImage,
   .srcImageLayout = info->srcImageLayout,
   .dstImage = info->dstImage,
   .dstImageLayout = info->dstImageLayout,
   .regionCount = info->regionCount,
   .pRegions = blit_regions,
               for (uint32_t r = 0; r < info->regionCount; r++) {
      blit_regions[r] = (VkImageBlit2) {
      .sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2,
   .srcSubresource = info->pRegions[r].srcSubresource,
   .srcOffsets = {
      info->pRegions[r].srcOffset,
      },
   .dstSubresource = info->pRegions[r].dstSubresource,
   .dstOffsets = {
      info->pRegions[r].dstOffset,
                  blit_regions[r].srcOffsets[1].x += info->pRegions[r].extent.width;
   blit_regions[r].srcOffsets[1].y += info->pRegions[r].extent.height;
   blit_regions[r].srcOffsets[1].z += info->pRegions[r].extent.depth;
   blit_regions[r].dstOffsets[1].x += info->pRegions[r].extent.width;
   blit_regions[r].dstOffsets[1].y += info->pRegions[r].extent.height;
                        STACK_ARRAY_FINISH(blit_regions);
               D3D12_TEXTURE_COPY_LOCATION tmp_loc = { 0 };
   D3D12_RESOURCE_DESC tmp_desc = {
      .Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
   .Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT,
   .DepthOrArraySize = 1,
   .MipLevels = 1,
   .Format = src->desc.Format,
   .SampleDesc = { .Count = 1, .Quality = 0 },
   .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
               if (requires_temp_res) {
      ID3D12Device4 *dev = device->dev;
   VkImageAspectFlags aspect = 0;
            if (vk_format_has_depth(src->vk.format))
         else if (vk_format_has_stencil(src->vk.format))
         else
            for (uint32_t i = 0; i < info->regionCount; i++) {
                     tmp_desc.Format =
      dzn_image_get_dxgi_format(pdev, src->vk.format,
                           ID3D12Device1_GetCopyableFootprints(dev, &src->desc,
                                 VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, max_size,
                        if (result != VK_SUCCESS)
                        for (int i = 0; i < info->regionCount; i++) {
               dzn_foreach_aspect(aspect, region->srcSubresource.aspectMask) {
      for (uint32_t l = 0; l < region->srcSubresource.layerCount; l++)
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBlitImage2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (info->regionCount == 0)
            uint32_t desc_count = 0;
   for (uint32_t r = 0; r < info->regionCount; r++)
            struct dzn_descriptor_heap *heap;
   uint32_t heap_slot;
   VkResult result =
      dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->cbv_srv_uav_pool, device,
         if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmdbuf->vk, result);
               if (heap != cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]) {
      ID3D12DescriptorHeap * const heaps[] = { heap->heap };
   cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = heap;
                        for (uint32_t r = 0; r < info->regionCount; r++)
            cmdbuf->state.pipeline = NULL;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_VIEWPORTS | DZN_CMD_DIRTY_SCISSORS;
   if (cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline) {
      cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdResolveImage2(VkCommandBuffer commandBuffer,
         {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (info->regionCount == 0)
            uint32_t desc_count = 0;
   for (uint32_t r = 0; r < info->regionCount; r++)
            struct dzn_descriptor_heap *heap;
   uint32_t heap_slot;
   VkResult result =
      dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->cbv_srv_uav_pool, device,
      if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmdbuf->vk, result);
               if (heap != cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]) {
      ID3D12DescriptorHeap * const heaps[] = { heap->heap };
   cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = heap;
                        for (uint32_t r = 0; r < info->regionCount; r++)
            cmdbuf->state.pipeline = NULL;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_VIEWPORTS | DZN_CMD_DIRTY_SCISSORS;
   if (cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline) {
      cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdClearColorImage(VkCommandBuffer commandBuffer,
                        VkImage image,
      {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer,
                                 {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDispatchBase(VkCommandBuffer commandBuffer,
                     uint32_t baseGroupX,
   uint32_t baseGroupY,
   uint32_t baseGroupZ,
   {
               cmdbuf->state.sysvals.compute.group_count_x = groupCountX;
   cmdbuf->state.sysvals.compute.group_count_y = groupCountY;
   cmdbuf->state.sysvals.compute.group_count_z = groupCountZ;
   cmdbuf->state.sysvals.compute.base_group_x = baseGroupX;
   cmdbuf->state.sysvals.compute.base_group_y = baseGroupY;
   cmdbuf->state.sysvals.compute.base_group_z = baseGroupZ;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |=
            dzn_cmd_buffer_prepare_dispatch(cmdbuf);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdFillBuffer(VkCommandBuffer commandBuffer,
                     VkBuffer dstBuffer,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (size == VK_WHOLE_SIZE)
                     ID3D12Resource *src_res;
   uint64_t src_offset;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, size,
                              if (result != VK_SUCCESS)
            uint32_t *cpu_ptr;
   ID3D12Resource_Map(src_res, 0, NULL, (void **)&cpu_ptr);
   cpu_ptr += src_offset / sizeof(uint32_t);
   for (uint32_t i = 0; i < size / 4; i++)
                        }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdUpdateBuffer(VkCommandBuffer commandBuffer,
                     VkBuffer dstBuffer,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (size == VK_WHOLE_SIZE)
            /*
   * The spec says:
   *   4, or VK_WHOLE_SIZE to fill the range from offset to the end of the
   *   buffer. If VK_WHOLE_SIZE is used and the remaining size of the buffer
   *   is not a multiple of 4, then the nearest smaller multiple is used."
   */
            ID3D12Resource *src_res;
   uint64_t src_offset;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, size,
                        if (result != VK_SUCCESS)
            void *cpu_ptr;
   ID3D12Resource_Map(src_res, 0, NULL, &cpu_ptr);
   memcpy((uint8_t *)cpu_ptr + src_offset, data, size),
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdClearAttachments(VkCommandBuffer commandBuffer,
                           {
               for (unsigned i = 0; i < attachmentCount; i++) {
      VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            if (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
      assert(pAttachments[i].colorAttachment < cmdbuf->state.render.attachments.color_count);
   view = cmdbuf->state.render.attachments.colors[pAttachments[i].colorAttachment].iview;
      } else {
      if (cmdbuf->state.render.attachments.depth.iview &&
      (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)) {
   view = cmdbuf->state.render.attachments.depth.iview;
               if (cmdbuf->state.render.attachments.stencil.iview &&
      (pAttachments[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
   assert(!view || view == cmdbuf->state.render.attachments.depth.iview);
   view = cmdbuf->state.render.attachments.stencil.iview;
                  if (!view)
            for (uint32_t j = 0; j < rectCount; j++) {
                     uint32_t view_mask = cmdbuf->state.multiview.view_mask;
   if (view_mask != 0) {
      u_foreach_bit(layer, view_mask) {
      dzn_cmd_buffer_clear_attachment(cmdbuf, view, layout,
                                 } else {
      dzn_cmd_buffer_clear_attachment(cmdbuf, view, layout,
                                       }
      static D3D12_RESOLVE_MODE
   dzn_get_resolve_mode(VkResolveModeFlags mode)
   {
      switch (mode) {
   case VK_RESOLVE_MODE_AVERAGE_BIT: return D3D12_RESOLVE_MODE_AVERAGE;
   case VK_RESOLVE_MODE_MAX_BIT: return D3D12_RESOLVE_MODE_MAX;
   case VK_RESOLVE_MODE_MIN_BIT: return D3D12_RESOLVE_MODE_MIN;
   /* TODO */
   case VK_RESOLVE_MODE_SAMPLE_ZERO_BIT: return D3D12_RESOLVE_MODE_MIN;
   default: return D3D12_RESOLVE_MODE_AVERAGE;
      }
      static void
   dzn_cmd_buffer_resolve_rendering_attachment_via_blit(struct dzn_cmd_buffer *cmdbuf,
                           {
      struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
            struct dzn_descriptor_heap *heap;
   uint32_t heap_slot;
   VkResult result =
      dzn_descriptor_heap_pool_alloc_slots(&cmdbuf->cbv_srv_uav_pool, device,
      if (result != VK_SUCCESS) {
      vk_command_buffer_set_error(&cmdbuf->vk, result);
               if (heap != cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]) {
      ID3D12DescriptorHeap *const heaps[] = { heap->heap };
   cmdbuf->state.heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = heap;
                        VkImageResolve2 region = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_RESOLVE_2,
   .srcSubresource = {
      .aspectMask = aspect,
   .baseArrayLayer = src_range->baseArrayLayer,
      },
   .dstSubresource = {
      .aspectMask = aspect,
   .baseArrayLayer = dst_range->baseArrayLayer,
         };
   VkResolveImageInfo2 resolve_info = {
      .sType = VK_STRUCTURE_TYPE_RESOLVE_IMAGE_INFO_2,
   .srcImage = vk_image_to_handle(att->iview->vk.image),
   .dstImage = vk_image_to_handle(att->resolve.iview->vk.image),
   .srcImageLayout = att->layout,
   .dstImageLayout = att->resolve.layout,
   .regionCount = 1,
      };
   for (uint32_t level = 0; level < src_range->levelCount; ++level) {
      region.srcSubresource.mipLevel = level + src_range->baseMipLevel;
   region.dstSubresource.mipLevel = level + dst_range->baseMipLevel;
   region.extent = (VkExtent3D){
      u_minify(att->iview->vk.image->extent.width, region.srcSubresource.mipLevel),
   u_minify(att->iview->vk.image->extent.height, region.srcSubresource.mipLevel),
      };
               cmdbuf->state.pipeline = NULL;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_VIEWPORTS | DZN_CMD_DIRTY_SCISSORS;
   if (cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline) {
      cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         }
      static void
   dzn_cmd_buffer_resolve_rendering_attachment(struct dzn_cmd_buffer *cmdbuf,
                     {
      struct dzn_image_view *src = att->iview;
            if (!src || !dst)
            struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev =
            struct dzn_image *src_img = container_of(src->vk.image, struct dzn_image, vk);
            VkImageSubresourceRange src_range = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .baseMipLevel = src->vk.base_mip_level,
   .levelCount = MIN2(src->vk.level_count, dst->vk.level_count),
   .baseArrayLayer = src->vk.base_array_layer,
      };
   if (src_img->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
      src_range.baseArrayLayer = 0;
               VkImageSubresourceRange dst_range = {
      .aspectMask = (VkImageAspectFlags)aspect,
   .baseMipLevel = dst->vk.base_mip_level,
   .levelCount = MIN2(src->vk.level_count, dst->vk.level_count),
   .baseArrayLayer = dst->vk.base_array_layer,
      };
   if (dst_img->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
      dst_range.baseArrayLayer = 0;
               if (force_blit_resolve ||
      att->resolve.mode == VK_RESOLVE_MODE_SAMPLE_ZERO_BIT ||
   /* D3D resolve API can't go from (e.g.) D32S8X24 to D32 */
   src->vk.view_format != dst->vk.view_format ||
   (att->resolve.mode != VK_RESOLVE_MODE_AVERAGE_BIT &&
   pdev->options2.ProgrammableSamplePositionsTier == D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED)) {
   dzn_cmd_buffer_resolve_rendering_attachment_via_blit(cmdbuf, att, aspect, &src_range, &dst_range);
               VkImageLayout src_layout = att->layout;
            D3D12_RESOURCE_STATES src_state = dzn_image_layout_to_state(src_img, src_layout, aspect, cmdbuf->type);
   D3D12_RESOURCE_STATES dst_state = dzn_image_layout_to_state(dst_img, dst_layout, aspect, cmdbuf->type);
   D3D12_BARRIER_LAYOUT src_restore_layout = D3D12_BARRIER_LAYOUT_COMMON,
         D3D12_BARRIER_LAYOUT dst_restore_layout = D3D12_BARRIER_LAYOUT_COMMON,
         if (cmdbuf->enhanced_barriers) {
      src_restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, src_img,
               dst_restore_layout = dzn_cmd_buffer_require_layout(cmdbuf, dst_img,
            } else {
      dzn_cmd_buffer_queue_image_range_state_transition(cmdbuf, src_img, &src_range,
                     dzn_cmd_buffer_queue_image_range_state_transition(cmdbuf, dst_img, &dst_range,
                           for (uint32_t level = 0; level < src_range.levelCount; level++) {
      for (uint32_t layer = 0; layer < src_range.layerCount; layer++) {
      uint32_t src_subres =
                        if (cmdbuf->cmdlist8 &&
      pdev->options2.ProgrammableSamplePositionsTier > D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED) {
   ID3D12GraphicsCommandList8_ResolveSubresourceRegion(cmdbuf->cmdlist8,
                                    } else {
      ID3D12GraphicsCommandList1_ResolveSubresource(cmdbuf->cmdlist,
                                 if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_restore_layout(cmdbuf, src_img,
                     dzn_cmd_buffer_restore_layout(cmdbuf, dst_img,
                  } else {
      dzn_cmd_buffer_queue_image_range_state_transition(cmdbuf, src_img, &src_range,
                     dzn_cmd_buffer_queue_image_range_state_transition(cmdbuf, dst_img, &dst_range,
                     }
      static void
   dzn_rendering_attachment_initial_transition(struct dzn_cmd_buffer *cmdbuf,
               {
      const VkRenderingAttachmentInitialLayoutInfoMESA *initial_layout =
                  if (!initial_layout || !iview)
            struct dzn_image *image = container_of(iview->vk.image, struct dzn_image, vk);
   VkImageSubresourceRange range = {
      .aspectMask = aspect,
   .baseMipLevel = iview->vk.base_mip_level,
   .levelCount = iview->vk.level_count,
   .baseArrayLayer = iview->vk.base_array_layer,
      };
   if (image->desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
      range.baseArrayLayer = 0;
               if (cmdbuf->enhanced_barriers) {
      D3D12_BARRIER_SYNC sync_before = D3D12_BARRIER_SYNC_ALL;
   D3D12_BARRIER_ACCESS access_before = D3D12_BARRIER_ACCESS_COMMON;
   if (initial_layout->initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
      sync_before = D3D12_BARRIER_SYNC_NONE;
               D3D12_BARRIER_LAYOUT layout_before = dzn_vk_layout_to_d3d_layout(initial_layout->initialLayout, cmdbuf->type, aspect);
   D3D12_BARRIER_LAYOUT layout_after = dzn_vk_layout_to_d3d_layout(att->imageLayout, cmdbuf->type, aspect);
   if (image->desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS) {
      layout_before = D3D12_BARRIER_LAYOUT_UNDEFINED;
               dzn_cmd_buffer_image_barrier(cmdbuf, image,
                              } else {
      dzn_cmd_buffer_queue_image_range_layout_transition(cmdbuf, image, &range,
                     }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBeginRendering(VkCommandBuffer commandBuffer,
         {
               D3D12_RECT new_render_area = {
      .left = pRenderingInfo->renderArea.offset.x,
   .top = pRenderingInfo->renderArea.offset.y,
   .right = (LONG)(pRenderingInfo->renderArea.offset.x + pRenderingInfo->renderArea.extent.width),
               // The render area has an impact on the scissor state.
   if (memcmp(&cmdbuf->state.render.area, &new_render_area, sizeof(new_render_area))) {
      cmdbuf->state.dirty |= DZN_CMD_DIRTY_SCISSORS;
               cmdbuf->state.render.flags = pRenderingInfo->flags;
   cmdbuf->state.render.layer_count = pRenderingInfo->layerCount;
            D3D12_CPU_DESCRIPTOR_HANDLE rt_handles[MAX_RTS] = { 0 };
            cmdbuf->state.render.attachments.color_count = pRenderingInfo->colorAttachmentCount;
   for (uint32_t i = 0; i < pRenderingInfo->colorAttachmentCount; i++) {
      const VkRenderingAttachmentInfo *att = &pRenderingInfo->pColorAttachments[i];
            cmdbuf->state.render.attachments.colors[i].iview = iview;
   cmdbuf->state.render.attachments.colors[i].layout = att->imageLayout;
   cmdbuf->state.render.attachments.colors[i].resolve.mode = att->resolveMode;
   cmdbuf->state.render.attachments.colors[i].resolve.iview =
         cmdbuf->state.render.attachments.colors[i].resolve.layout =
                  if (!iview) {
      rt_handles[i] = dzn_cmd_buffer_get_null_rtv(cmdbuf);
               struct dzn_image *img = container_of(iview->vk.image, struct dzn_image, vk);
   rt_handles[i] = dzn_cmd_buffer_get_rtv(cmdbuf, img, &iview->rtv_desc);
   dzn_rendering_attachment_initial_transition(cmdbuf, att,
               if (pRenderingInfo->pDepthAttachment) {
               cmdbuf->state.render.attachments.depth.iview =
         cmdbuf->state.render.attachments.depth.layout = att->imageLayout;
   cmdbuf->state.render.attachments.depth.resolve.mode = att->resolveMode;
   cmdbuf->state.render.attachments.depth.resolve.iview =
         cmdbuf->state.render.attachments.depth.resolve.layout =
         cmdbuf->state.render.attachments.depth.store_op = att->storeOp;
   dzn_rendering_attachment_initial_transition(cmdbuf, att,
               if (pRenderingInfo->pStencilAttachment) {
               cmdbuf->state.render.attachments.stencil.iview =
         cmdbuf->state.render.attachments.stencil.layout = att->imageLayout;
   cmdbuf->state.render.attachments.stencil.resolve.mode = att->resolveMode;
   cmdbuf->state.render.attachments.stencil.resolve.iview =
         cmdbuf->state.render.attachments.stencil.resolve.layout =
         cmdbuf->state.render.attachments.stencil.store_op = att->storeOp;
   dzn_rendering_attachment_initial_transition(cmdbuf, att,
               if (pRenderingInfo->pDepthAttachment || pRenderingInfo->pStencilAttachment) {
      struct dzn_image_view *z_iview =
      pRenderingInfo->pDepthAttachment ?
   dzn_image_view_from_handle(pRenderingInfo->pDepthAttachment->imageView) :
      struct dzn_image_view *s_iview =
      pRenderingInfo->pStencilAttachment ?
   dzn_image_view_from_handle(pRenderingInfo->pStencilAttachment->imageView) :
      struct dzn_image_view *iview = z_iview ? z_iview : s_iview;
            if (iview) {
                              ID3D12GraphicsCommandList1_OMSetRenderTargets(cmdbuf->cmdlist,
                        for (uint32_t a = 0; a < pRenderingInfo->colorAttachmentCount; a++) {
      const VkRenderingAttachmentInfo *att = &pRenderingInfo->pColorAttachments[a];
            if (iview != NULL && att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR &&
      !(pRenderingInfo->flags & VK_RENDERING_RESUMING_BIT)) {
   if (pRenderingInfo->viewMask != 0) {
      u_foreach_bit(layer, pRenderingInfo->viewMask) {
      dzn_cmd_buffer_clear_attachment(cmdbuf, iview, att->imageLayout,
                     } else {
      dzn_cmd_buffer_clear_attachment(cmdbuf, iview, att->imageLayout,
                                       if ((pRenderingInfo->pDepthAttachment || pRenderingInfo->pStencilAttachment) &&
      !(pRenderingInfo->flags & VK_RENDERING_RESUMING_BIT)) {
   const VkRenderingAttachmentInfo *z_att = pRenderingInfo->pDepthAttachment;
   const VkRenderingAttachmentInfo *s_att = pRenderingInfo->pStencilAttachment;
   struct dzn_image_view *z_iview = z_att ? dzn_image_view_from_handle(z_att->imageView) : NULL;
   struct dzn_image_view *s_iview = s_att ? dzn_image_view_from_handle(s_att->imageView) : NULL;
   struct dzn_image_view *iview = z_iview ? z_iview : s_iview;
                     VkImageAspectFlags aspects = 0;
            if (z_iview && z_att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      aspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
   clear_val.depthStencil.depth = z_att->clearValue.depthStencil.depth;
               if (s_iview && s_att->loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      aspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
   clear_val.depthStencil.stencil = s_att->clearValue.depthStencil.stencil;
               if (aspects != 0) {
      if (pRenderingInfo->viewMask != 0) {
      u_foreach_bit(layer, pRenderingInfo->viewMask) {
      dzn_cmd_buffer_clear_attachment(cmdbuf, iview, layout,
               } else {
      dzn_cmd_buffer_clear_attachment(cmdbuf, iview, layout,
                                 cmdbuf->state.multiview.num_views = MAX2(util_bitcount(pRenderingInfo->viewMask), 1);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdEndRendering(VkCommandBuffer commandBuffer)
   {
               if (!(cmdbuf->state.render.flags & VK_RENDERING_SUSPENDING_BIT)) {
      for (uint32_t i = 0; i < cmdbuf->state.render.attachments.color_count; i++) {
      dzn_cmd_buffer_resolve_rendering_attachment(cmdbuf,
                     bool separate_stencil_resolve =
      cmdbuf->state.render.attachments.depth.resolve.mode !=
      dzn_cmd_buffer_resolve_rendering_attachment(cmdbuf,
                     dzn_cmd_buffer_resolve_rendering_attachment(cmdbuf,
                              }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBindPipeline(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->state.bindpoint[pipelineBindPoint].pipeline = pipeline;
   cmdbuf->state.bindpoint[pipelineBindPoint].dirty |= DZN_CMD_BINDPOINT_DIRTY_PIPELINE;
   if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
               if (!gfx->vp.dynamic) {
      memcpy(cmdbuf->state.viewports, gfx->vp.desc,
         cmdbuf->state.sysvals.gfx.viewport_width = cmdbuf->state.viewports[0].Width;
   cmdbuf->state.sysvals.gfx.viewport_height = cmdbuf->state.viewports[0].Height;
   cmdbuf->state.dirty |= DZN_CMD_DIRTY_VIEWPORTS;
               if (!gfx->scissor.dynamic) {
      memcpy(cmdbuf->state.scissors, gfx->scissor.desc,
                     if (gfx->zsa.stencil_test.enable && !gfx->zsa.stencil_test.dynamic_ref) {
      cmdbuf->state.zsa.stencil_test.front.ref = gfx->zsa.stencil_test.front.ref;
   cmdbuf->state.zsa.stencil_test.back.ref = gfx->zsa.stencil_test.back.ref;
               if (gfx->zsa.depth_bounds.enable && !gfx->zsa.depth_bounds.dynamic) {
      cmdbuf->state.zsa.depth_bounds.min = gfx->zsa.depth_bounds.min;
   cmdbuf->state.zsa.depth_bounds.max = gfx->zsa.depth_bounds.max;
               if (!gfx->blend.dynamic_constants) {
      memcpy(cmdbuf->state.blend.constants, gfx->blend.constants,
                     for (uint32_t vb = 0; vb < gfx->vb.count; vb++)
            if (gfx->vb.count > 0)
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                           VkPipelineBindPoint pipelineBindPoint,
   VkPipelineLayout layout,
   uint32_t firstSet,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            struct dzn_descriptor_state *desc_state =
                  for (uint32_t i = 0; i < descriptorSetCount; i++) {
      uint32_t idx = firstSet + i;
            if (desc_state->sets[idx].set != set) {
      desc_state->sets[idx].set = set;
               uint32_t dynamic_buffer_count = playout->sets[idx].dynamic_buffer_count;
   if (dynamic_buffer_count) {
                              dynamicOffsetCount -= dynamic_buffer_count;
   pDynamicOffsets += dynamic_buffer_count;
                     }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetViewport(VkCommandBuffer commandBuffer,
                     {
                        for (uint32_t i = 0; i < viewportCount; i++) {
                        if (pViewports[i].minDepth > pViewports[i].maxDepth)
         else
            if (pViewports[i].height > 0)
         else
               cmdbuf->state.sysvals.gfx.viewport_width = cmdbuf->state.viewports[0].Width;
            if (viewportCount) {
      cmdbuf->state.dirty |= DZN_CMD_DIRTY_VIEWPORTS;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetScissor(VkCommandBuffer commandBuffer,
                     {
               for (uint32_t i = 0; i < scissorCount; i++)
            if (scissorCount)
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   struct dzn_cmd_buffer_push_constant_state *states[2];
            if (stageFlags & VK_SHADER_STAGE_ALL_GRAPHICS)
            if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT)
            for (uint32_t i = 0; i < num_states; i++) {
      memcpy(((char *)states[i]->values) + offset, pValues, size);
   states[i]->offset =
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDraw(VkCommandBuffer commandBuffer,
               uint32_t vertexCount,
   uint32_t instanceCount,
   {
               const struct dzn_graphics_pipeline *pipeline = (const struct dzn_graphics_pipeline *)
            cmdbuf->state.sysvals.gfx.first_vertex = firstVertex;
            uint32_t view_mask = pipeline->multiview.native_view_instancing ?
            if (pipeline->ia.triangle_fan) {
               VkResult result =
         if (result != VK_SUCCESS || !vertexCount)
            cmdbuf->state.sysvals.gfx.is_indexed_draw = true;
   u_foreach_bit(view, view_mask) {
      cmdbuf->state.sysvals.gfx.view_index = view;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         dzn_cmd_buffer_prepare_draw(cmdbuf, true);
   ID3D12GraphicsCommandList1_DrawIndexedInstanced(cmdbuf->cmdlist, vertexCount, instanceCount, 0,
               /* Restore the IB view if we modified it when lowering triangle fans. */
   if (ib_view.SizeInBytes > 0) {
      cmdbuf->state.ib.view = ib_view;
         } else {
      cmdbuf->state.sysvals.gfx.is_indexed_draw = false;
   u_foreach_bit(view, view_mask) {
      cmdbuf->state.sysvals.gfx.view_index = view;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
         dzn_cmd_buffer_prepare_draw(cmdbuf, false);
   ID3D12GraphicsCommandList1_DrawInstanced(cmdbuf->cmdlist, vertexCount, instanceCount,
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDrawIndexed(VkCommandBuffer commandBuffer,
                     uint32_t indexCount,
   uint32_t instanceCount,
   {
               const struct dzn_graphics_pipeline *pipeline = (const struct dzn_graphics_pipeline *)
            if (pipeline->ia.triangle_fan &&
      dzn_graphics_pipeline_get_desc_template(pipeline, ib_strip_cut)) {
   /* The indexed+primitive-restart+triangle-fan combination is a mess,
   * since we have to walk the index buffer, skip entries with the
   * special 0xffff/0xffffffff values, and push triangle list indices
   * for the remaining values. All of this has an impact on the index
   * count passed to the draw call, which forces us to use the indirect
   * path.
   */
   struct dzn_indirect_indexed_draw_params params = {
      .index_count = indexCount,
   .instance_count = instanceCount,
   .first_index = firstIndex,
   .vertex_offset = vertexOffset,
               ID3D12Resource *draw_buf;
   uint64_t offset;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, sizeof(params),
                        if (result != VK_SUCCESS)
            void *cpu_ptr;
   ID3D12Resource_Map(draw_buf, 0, NULL, &cpu_ptr);
                     dzn_cmd_buffer_indirect_draw(cmdbuf, draw_buf, offset, NULL, 0, 1, sizeof(params), true);
               cmdbuf->state.sysvals.gfx.first_vertex = vertexOffset;
   cmdbuf->state.sysvals.gfx.base_instance = firstInstance;
                     if (pipeline->ia.triangle_fan) {
      VkResult result =
         if (result != VK_SUCCESS || !indexCount)
               uint32_t view_mask = pipeline->multiview.native_view_instancing ?
         u_foreach_bit(view, view_mask) {
      cmdbuf->state.sysvals.gfx.view_index = view;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_GRAPHICS].dirty |=
            dzn_cmd_buffer_prepare_draw(cmdbuf, true);
   ID3D12GraphicsCommandList1_DrawIndexedInstanced(cmdbuf->cmdlist, indexCount, instanceCount, firstIndex,
               /* Restore the IB view if we modified it when lowering triangle fans. */
   if (pipeline->ia.triangle_fan && ib_view.SizeInBytes) {
      cmdbuf->state.ib.view = ib_view;
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDrawIndirect(VkCommandBuffer commandBuffer,
                     VkBuffer buffer,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer,
                           {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDrawIndirectCount(VkCommandBuffer commandBuffer,
                           VkBuffer buffer,
   VkDeviceSize offset,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(dzn_buffer, buf, buffer);
            dzn_cmd_buffer_indirect_draw(cmdbuf, buf->res, offset,
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer,
                                 VkBuffer buffer,
   {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(dzn_buffer, buf, buffer);
            dzn_cmd_buffer_indirect_draw(cmdbuf, buf->res, offset,
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                           {
               if (!bindingCount)
                     for (uint32_t i = 0; i < bindingCount; i++) {
               vbviews[firstBinding + i].BufferLocation = buf->gpuva + pOffsets[i];
               BITSET_SET_RANGE(cmdbuf->state.vb.dirty, firstBinding,
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->state.ib.view.BufferLocation = buf->gpuva + offset;
   cmdbuf->state.ib.view.SizeInBytes = buf->size - offset;
   switch (indexType) {
   case VK_INDEX_TYPE_UINT16:
      cmdbuf->state.ib.view.Format = DXGI_FORMAT_R16_UINT;
   cmdbuf->state.pipeline_variant.ib_strip_cut = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFF;
      case VK_INDEX_TYPE_UINT32:
      cmdbuf->state.ib.view.Format = DXGI_FORMAT_R32_UINT;
   cmdbuf->state.pipeline_variant.ib_strip_cut = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_0xFFFFFFFF;
      default: unreachable("Invalid index type");
                     const struct dzn_graphics_pipeline *pipeline =
            if (pipeline &&
      dzn_graphics_pipeline_get_desc_template(pipeline, ib_strip_cut))
   }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdResetEvent2(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (!_mesa_hash_table_insert(cmdbuf->events.ht, evt, (void *)(uintptr_t)DZN_EVENT_STATE_RESET))
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetEvent2(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            if (!_mesa_hash_table_insert(cmdbuf->events.ht, evt, (void *)(uintptr_t)DZN_EVENT_STATE_SET))
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdWaitEvents2(VkCommandBuffer commandBuffer,
                     {
               /* Intra-command list wait is handle by this pipeline flush, which is
   * overkill, but that's the best we can do with the standard D3D12 barrier
   * API.
   *
   * Inter-command list is taken care of by the serialization done at the
   * ExecuteCommandList() level:
   * "Calling ExecuteCommandLists twice in succession (from the same thread,
   *  or different threads) guarantees that the first workload (A) finishes
   *  before the second workload (B)"
   *
   * HOST -> DEVICE signaling is ignored and we assume events are always
   * signaled when we reach the vkCmdWaitEvents() point.:
   * "Command buffers in the submission can include vkCmdWaitEvents commands
   *  that wait on events that will not be signaled by earlier commands in the
   *  queue. Such events must be signaled by the application using vkSetEvent,
   *  and the vkCmdWaitEvents commands that wait upon them must not be inside
   *  a render pass instance.
   *  The event must be set before the vkCmdWaitEvents command is executed."
   */
            for (uint32_t i = 0; i < eventCount; i++) {
               struct hash_entry *he =
         if (he) {
      enum dzn_event_state state = (uintptr_t)he->data;
   assert(state != DZN_EVENT_STATE_RESET);
                  if (flush_pipeline) {
      if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_global_barrier(cmdbuf,
            } else {
      D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
   .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                     }
   cmdbuf->vk.base.device->dispatch_table.CmdPipelineBarrier2(
      vk_command_buffer_to_handle(&cmdbuf->vk),
   }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdBeginQuery(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            struct dzn_cmd_buffer_query_pool_state *state =
         if (!state)
            for (uint32_t i = 0; i < cmdbuf->state.multiview.num_views; ++i)
                     dzn_cmd_buffer_dynbitset_clear_range(cmdbuf, &state->collect, query, cmdbuf->state.multiview.num_views);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdEndQuery(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            struct dzn_cmd_buffer_query_pool_state *state =
         if (!state)
                     dzn_cmd_buffer_dynbitset_set(cmdbuf, &state->collect, query);
   if (cmdbuf->state.multiview.num_views > 1)
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdWriteTimestamp2(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            struct dzn_cmd_buffer_query_pool_state *state =
         if (!state)
            /* Execution barrier so the timestamp gets written after the pipeline flush. */
   D3D12_RESOURCE_BARRIER barrier = {
      .Type = D3D12_RESOURCE_BARRIER_TYPE_UAV,
   .Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
                        for (uint32_t i = 0; i < cmdbuf->state.multiview.num_views; ++i)
                  dzn_cmd_buffer_dynbitset_set(cmdbuf, &state->collect, query);
   if (cmdbuf->state.multiview.num_views > 1)
      }
         VKAPI_ATTR void VKAPI_CALL
   dzn_CmdResetQueryPool(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
            struct dzn_cmd_buffer_query_pool_state *state =
            if (!state)
                     for (uint32_t q = 0; q < queryCount; q += q_step) {
               ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, qpool->collect_buffer,
                                          for (uint32_t q = 0; q < queryCount; q += q_step) {
      ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, qpool->collect_buffer,
                                 dzn_cmd_buffer_dynbitset_set_range(cmdbuf, &state->reset, firstQuery, queryCount);
   dzn_cmd_buffer_dynbitset_clear_range(cmdbuf, &state->collect, firstQuery, queryCount);
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer,
                              VkQueryPool queryPool,
   uint32_t firstQuery,
      {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(dzn_query_pool, qpool, queryPool);
            struct dzn_cmd_buffer_query_pool_state *qpstate =
         if (!qpstate)
            VkResult result =
         if (result != VK_SUCCESS)
            bool raw_copy = (flags & VK_QUERY_RESULT_64_BIT) &&
            #define ALL_STATS \
         (VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT | \
      VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_INVOCATIONS_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_GEOMETRY_SHADER_PRIMITIVES_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT | \
   VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT | \
   if (qpool->heap_type == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS &&
      qpool->pipeline_statistics != ALL_STATS)
   #undef ALL_STATS
         if (cmdbuf->enhanced_barriers) {
      if (flags & VK_QUERY_RESULT_WAIT_BIT) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, qpool->collect_buffer,
               } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, qpool->collect_buffer, 0, 1,
                           if (raw_copy) {
      ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, buf->res, dstOffset,
                  } else {
               for (uint32_t q = 0; q < queryCount; q++) {
                     if (qpool->heap_type == D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS) {
      for (uint32_t c = 0; c < sizeof(D3D12_QUERY_DATA_PIPELINE_STATISTICS) / sizeof(uint64_t); c++) {
                     ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, buf->res, dstOffset + dst_counter_offset,
                           } else {
      ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, buf->res, dstOffset,
                           if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
      ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, buf->res, dstOffset + dst_counter_offset,
                                          if (!cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, qpool->collect_buffer, 0, 1,
                     }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdDispatchIndirect(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->state.sysvals.compute.group_count_x = 0;
   cmdbuf->state.sysvals.compute.group_count_y = 0;
   cmdbuf->state.sysvals.compute.group_count_z = 0;
   cmdbuf->state.sysvals.compute.base_group_x = 0;
   cmdbuf->state.sysvals.compute.base_group_y = 0;
   cmdbuf->state.sysvals.compute.base_group_z = 0;
   cmdbuf->state.bindpoint[VK_PIPELINE_BIND_POINT_COMPUTE].dirty |=
                     struct dzn_compute_pipeline *pipeline = (struct dzn_compute_pipeline *)
         ID3D12CommandSignature *cmdsig =
            if (!cmdsig) {
      vk_command_buffer_set_error(&cmdbuf->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               ID3D12Resource *exec_buf;
   VkResult result =
      dzn_cmd_buffer_alloc_internal_buf(cmdbuf, sizeof(D3D12_DISPATCH_ARGUMENTS) * 2,
                        if (result != VK_SUCCESS)
            ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, exec_buf, 0,
                     ID3D12GraphicsCommandList1_CopyBufferRegion(cmdbuf->cmdlist, exec_buf, sizeof(D3D12_DISPATCH_ARGUMENTS),
                        if (cmdbuf->enhanced_barriers) {
      dzn_cmd_buffer_buffer_barrier(cmdbuf, exec_buf,
            } else {
      dzn_cmd_buffer_queue_transition_barriers(cmdbuf, exec_buf, 0, 1,
                              }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetLineWidth(VkCommandBuffer commandBuffer,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetDepthBias(VkCommandBuffer commandBuffer,
                     {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->state.pipeline_variant.depth_bias.constant_factor = depthBiasConstantFactor;
   cmdbuf->state.pipeline_variant.depth_bias.clamp = depthBiasClamp;
   cmdbuf->state.pipeline_variant.depth_bias.slope_factor = depthBiasSlopeFactor;
   cmdbuf->state.sysvals.gfx.depth_bias = depthBiasConstantFactor;
   if (pdev->options16.DynamicDepthBiasSupported)
         else
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetBlendConstants(VkCommandBuffer commandBuffer,
         {
               memcpy(cmdbuf->state.blend.constants, blendConstants,
            }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetDepthBounds(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(dzn_cmd_buffer, cmdbuf, commandBuffer);
   struct dzn_device *device = container_of(cmdbuf->vk.base.device, struct dzn_device, vk);
   struct dzn_physical_device *pdev =
            if (pdev->options2.DepthBoundsTestSupported) {
      cmdbuf->state.zsa.depth_bounds.min = minDepthBounds;
   cmdbuf->state.zsa.depth_bounds.max = maxDepthBounds;
         }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetStencilCompareMask(VkCommandBuffer commandBuffer,
               {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
      cmdbuf->state.zsa.stencil_test.front.compare_mask = compareMask;
               if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
      cmdbuf->state.zsa.stencil_test.back.compare_mask = compareMask;
               cmdbuf->state.dirty |= DZN_CMD_DIRTY_STENCIL_COMPARE_MASK;
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetStencilWriteMask(VkCommandBuffer commandBuffer,
               {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
      cmdbuf->state.zsa.stencil_test.front.write_mask = writeMask;
               if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
      cmdbuf->state.zsa.stencil_test.back.write_mask = writeMask;
               cmdbuf->state.dirty |= DZN_CMD_DIRTY_STENCIL_WRITE_MASK;
      }
      VKAPI_ATTR void VKAPI_CALL
   dzn_CmdSetStencilReference(VkCommandBuffer commandBuffer,
               {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
            if (faceMask & VK_STENCIL_FACE_BACK_BIT)
               }
