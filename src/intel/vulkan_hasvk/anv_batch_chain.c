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
      #include <assert.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include <xf86drm.h>
      #include "anv_private.h"
   #include "anv_measure.h"
      #include "genxml/gen8_pack.h"
   #include "genxml/genX_bits.h"
   #include "perf/intel_perf.h"
      #include "util/u_debug.h"
   #include "util/perf/u_trace.h"
      /** \file anv_batch_chain.c
   *
   * This file contains functions related to anv_cmd_buffer as a data
   * structure.  This involves everything required to create and destroy
   * the actual batch buffers as well as link them together and handle
   * relocations and surface state.  It specifically does *not* contain any
   * handling of actual vkCmd calls beyond vkCmdExecuteCommands.
   */
      /*-----------------------------------------------------------------------*
   * Functions related to anv_reloc_list
   *-----------------------------------------------------------------------*/
      VkResult
   anv_reloc_list_init(struct anv_reloc_list *list,
         {
      memset(list, 0, sizeof(*list));
      }
      static VkResult
   anv_reloc_list_init_clone(struct anv_reloc_list *list,
               {
      list->num_relocs = other_list->num_relocs;
            if (list->num_relocs > 0) {
      list->relocs =
      vk_alloc(alloc, list->array_length * sizeof(*list->relocs), 8,
      if (list->relocs == NULL)
            list->reloc_bos =
      vk_alloc(alloc, list->array_length * sizeof(*list->reloc_bos), 8,
      if (list->reloc_bos == NULL) {
      vk_free(alloc, list->relocs);
               memcpy(list->relocs, other_list->relocs,
         memcpy(list->reloc_bos, other_list->reloc_bos,
      } else {
      list->relocs = NULL;
                        if (list->dep_words > 0) {
      list->deps =
      vk_alloc(alloc, list->dep_words * sizeof(BITSET_WORD), 8,
      memcpy(list->deps, other_list->deps,
      } else {
                     }
      void
   anv_reloc_list_finish(struct anv_reloc_list *list,
         {
      vk_free(alloc, list->relocs);
   vk_free(alloc, list->reloc_bos);
      }
      static VkResult
   anv_reloc_list_grow(struct anv_reloc_list *list,
               {
      if (list->num_relocs + num_additional_relocs <= list->array_length)
            size_t new_length = MAX2(16, list->array_length * 2);
   while (new_length < list->num_relocs + num_additional_relocs)
            struct drm_i915_gem_relocation_entry *new_relocs =
      vk_realloc(alloc, list->relocs,
            if (new_relocs == NULL)
                  struct anv_bo **new_reloc_bos =
      vk_realloc(alloc, list->reloc_bos,
            if (new_reloc_bos == NULL)
                              }
      static VkResult
   anv_reloc_list_grow_deps(struct anv_reloc_list *list,
               {
      if (min_num_words <= list->dep_words)
            uint32_t new_length = MAX2(32, list->dep_words * 2);
   while (new_length < min_num_words)
            BITSET_WORD *new_deps =
      vk_realloc(alloc, list->deps, new_length * sizeof(BITSET_WORD), 8,
      if (new_deps == NULL)
                  /* Zero out the new data */
   memset(list->deps + list->dep_words, 0,
                     }
      #define READ_ONCE(x) (*(volatile __typeof__(x) *)&(x))
      VkResult
   anv_reloc_list_add_bo(struct anv_reloc_list *list,
               {
      assert(!target_bo->is_wrapper);
            uint32_t idx = target_bo->gem_handle;
   VkResult result = anv_reloc_list_grow_deps(list, alloc,
         if (unlikely(result != VK_SUCCESS))
                        }
      VkResult
   anv_reloc_list_add(struct anv_reloc_list *list,
                     {
      struct drm_i915_gem_relocation_entry *entry;
            struct anv_bo *unwrapped_target_bo = anv_bo_unwrap(target_bo);
   uint64_t target_bo_offset = READ_ONCE(unwrapped_target_bo->offset);
   if (address_u64_out)
            assert(unwrapped_target_bo->gem_handle > 0);
            if (anv_bo_is_pinned(unwrapped_target_bo))
            VkResult result = anv_reloc_list_grow(list, alloc, 1);
   if (result != VK_SUCCESS)
            /* XXX: Can we use I915_EXEC_HANDLE_LUT? */
   index = list->num_relocs++;
   list->reloc_bos[index] = target_bo;
   entry = &list->relocs[index];
   entry->target_handle = -1; /* See also anv_cmd_buffer_process_relocs() */
   entry->delta = delta;
   entry->offset = offset;
   entry->presumed_offset = target_bo_offset;
   entry->read_domains = 0;
   entry->write_domain = 0;
               }
      static void
   anv_reloc_list_clear(struct anv_reloc_list *list)
   {
      list->num_relocs = 0;
   if (list->dep_words > 0)
      }
      static VkResult
   anv_reloc_list_append(struct anv_reloc_list *list,
               {
      VkResult result = anv_reloc_list_grow(list, alloc, other->num_relocs);
   if (result != VK_SUCCESS)
            if (other->num_relocs > 0) {
      memcpy(&list->relocs[list->num_relocs], &other->relocs[0],
         memcpy(&list->reloc_bos[list->num_relocs], &other->reloc_bos[0],
            for (uint32_t i = 0; i < other->num_relocs; i++)
                        anv_reloc_list_grow_deps(list, alloc, other->dep_words);
   for (uint32_t w = 0; w < other->dep_words; w++)
               }
      /*-----------------------------------------------------------------------*
   * Functions related to anv_batch
   *-----------------------------------------------------------------------*/
      void *
   anv_batch_emit_dwords(struct anv_batch *batch, int num_dwords)
   {
      if (batch->next + num_dwords * 4 > batch->end) {
      VkResult result = batch->extend_cb(batch, batch->user_data);
   if (result != VK_SUCCESS) {
      anv_batch_set_error(batch, result);
                           batch->next += num_dwords * 4;
               }
      struct anv_address
   anv_batch_address(struct anv_batch *batch, void *batch_location)
   {
               /* Allow a jump at the current location of the batch. */
               }
      void
   anv_batch_emit_batch(struct anv_batch *batch, struct anv_batch *other)
   {
               size = other->next - other->start;
            if (batch->next + size > batch->end) {
      VkResult result = batch->extend_cb(batch, batch->user_data);
   if (result != VK_SUCCESS) {
      anv_batch_set_error(batch, result);
                           VG(VALGRIND_CHECK_MEM_IS_DEFINED(other->start, size));
            offset = batch->next - batch->start;
   VkResult result = anv_reloc_list_append(batch->relocs, batch->alloc,
         if (result != VK_SUCCESS) {
      anv_batch_set_error(batch, result);
                  }
      /*-----------------------------------------------------------------------*
   * Functions related to anv_batch_bo
   *-----------------------------------------------------------------------*/
      static VkResult
   anv_batch_bo_create(struct anv_cmd_buffer *cmd_buffer,
               {
               struct anv_batch_bo *bbo = vk_zalloc(&cmd_buffer->vk.pool->alloc, sizeof(*bbo),
         if (bbo == NULL)
            result = anv_bo_pool_alloc(&cmd_buffer->device->batch_bo_pool,
         if (result != VK_SUCCESS)
            result = anv_reloc_list_init(&bbo->relocs, &cmd_buffer->vk.pool->alloc);
   if (result != VK_SUCCESS)
                           fail_bo_alloc:
         fail_alloc:
                  }
      static VkResult
   anv_batch_bo_clone(struct anv_cmd_buffer *cmd_buffer,
               {
               struct anv_batch_bo *bbo = vk_alloc(&cmd_buffer->vk.pool->alloc, sizeof(*bbo),
         if (bbo == NULL)
            result = anv_bo_pool_alloc(&cmd_buffer->device->batch_bo_pool,
         if (result != VK_SUCCESS)
            result = anv_reloc_list_init_clone(&bbo->relocs, &cmd_buffer->vk.pool->alloc,
         if (result != VK_SUCCESS)
            bbo->length = other_bbo->length;
   memcpy(bbo->bo->map, other_bbo->bo->map, other_bbo->length);
                  fail_bo_alloc:
         fail_alloc:
                  }
      static void
   anv_batch_bo_start(struct anv_batch_bo *bbo, struct anv_batch *batch,
         {
      anv_batch_set_storage(batch, (struct anv_address) { .bo = bbo->bo, },
         batch->relocs = &bbo->relocs;
      }
      static void
   anv_batch_bo_continue(struct anv_batch_bo *bbo, struct anv_batch *batch,
         {
      batch->start_addr = (struct anv_address) { .bo = bbo->bo, };
   batch->start = bbo->bo->map;
   batch->next = bbo->bo->map + bbo->length;
   batch->end = bbo->bo->map + bbo->bo->size - batch_padding;
      }
      static void
   anv_batch_bo_finish(struct anv_batch_bo *bbo, struct anv_batch *batch)
   {
      assert(batch->start == bbo->bo->map);
   bbo->length = batch->next - batch->start;
      }
      static VkResult
   anv_batch_bo_grow(struct anv_cmd_buffer *cmd_buffer, struct anv_batch_bo *bbo,
               {
      assert(batch->start == bbo->bo->map);
            size_t new_size = bbo->bo->size;
   while (new_size <= bbo->length + additional + batch_padding)
            if (new_size == bbo->bo->size)
            struct anv_bo *new_bo;
   VkResult result = anv_bo_pool_alloc(&cmd_buffer->device->batch_bo_pool,
         if (result != VK_SUCCESS)
                              bbo->bo = new_bo;
               }
      static void
   anv_batch_bo_link(struct anv_cmd_buffer *cmd_buffer,
                     {
      const uint32_t bb_start_offset =
                  /* Make sure we're looking at a MI_BATCH_BUFFER_START */
   assert(((*bb_start >> 29) & 0x07) == 0);
            if (anv_use_relocations(cmd_buffer->device->physical)) {
      uint32_t reloc_idx = prev_bbo->relocs.num_relocs - 1;
            prev_bbo->relocs.reloc_bos[reloc_idx] = next_bbo->bo;
            /* Use a bogus presumed offset to force a relocation */
      } else {
      assert(anv_bo_is_pinned(prev_bbo->bo));
            write_reloc(cmd_buffer->device,
               }
      static void
   anv_batch_bo_destroy(struct anv_batch_bo *bbo,
         {
      anv_reloc_list_finish(&bbo->relocs, &cmd_buffer->vk.pool->alloc);
   anv_bo_pool_free(&cmd_buffer->device->batch_bo_pool, bbo->bo);
      }
      static VkResult
   anv_batch_bo_list_clone(const struct list_head *list,
               {
                        struct anv_batch_bo *prev_bbo = NULL;
   list_for_each_entry(struct anv_batch_bo, bbo, list, link) {
      struct anv_batch_bo *new_bbo = NULL;
   result = anv_batch_bo_clone(cmd_buffer, bbo, &new_bbo);
   if (result != VK_SUCCESS)
                  if (prev_bbo)
                        if (result != VK_SUCCESS) {
      list_for_each_entry_safe(struct anv_batch_bo, bbo, new_list, link) {
      list_del(&bbo->link);
                     }
      /*-----------------------------------------------------------------------*
   * Functions related to anv_batch_bo
   *-----------------------------------------------------------------------*/
      static struct anv_batch_bo *
   anv_cmd_buffer_current_batch_bo(struct anv_cmd_buffer *cmd_buffer)
   {
         }
      struct anv_address
   anv_cmd_buffer_surface_base_address(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_state_pool *pool = anv_binding_table_pool(cmd_buffer->device);
   struct anv_state *bt_block = u_vector_head(&cmd_buffer->bt_block_states);
   return (struct anv_address) {
      .bo = pool->block_pool.bo,
         }
      static void
   emit_batch_buffer_start(struct anv_cmd_buffer *cmd_buffer,
         {
      /* In gfx8+ the address field grew to two dwords to accommodate 48 bit
   * offsets. The high 16 bits are in the last dword, so we can use the gfx8
   * version in either case, as long as we set the instruction length in the
   * header accordingly.  This means that we always emit three dwords here
   * and all the padding and adjustment we do in this file works for all
   * gens.
         #define GFX7_MI_BATCH_BUFFER_START_length      2
   #define GFX7_MI_BATCH_BUFFER_START_length_bias      2
         const uint32_t gfx7_length =
         const uint32_t gfx8_length =
            anv_batch_emit(&cmd_buffer->batch, GFX8_MI_BATCH_BUFFER_START, bbs) {
      bbs.DWordLength               = cmd_buffer->device->info->ver < 8 ?
         bbs.SecondLevelBatchBuffer    = Firstlevelbatch;
   bbs.AddressSpaceIndicator     = ASI_PPGTT;
         }
      static void
   cmd_buffer_chain_to_batch_bo(struct anv_cmd_buffer *cmd_buffer,
         {
      struct anv_batch *batch = &cmd_buffer->batch;
   struct anv_batch_bo *current_bbo =
            /* We set the end of the batch a little short so we would be sure we
   * have room for the chaining command.  Since we're about to emit the
   * chaining command, let's set it back where it should go.
   */
   batch->end += GFX8_MI_BATCH_BUFFER_START_length * 4;
                        }
      static void
   anv_cmd_buffer_record_chain_submit(struct anv_cmd_buffer *cmd_buffer_from,
         {
                        struct anv_batch_bo *last_bbo =
         struct anv_batch_bo *first_bbo =
            struct GFX8_MI_BATCH_BUFFER_START gen_bb_start = {
      __anv_cmd_header(GFX8_MI_BATCH_BUFFER_START),
   .SecondLevelBatchBuffer    = Firstlevelbatch,
   .AddressSpaceIndicator     = ASI_PPGTT,
      };
   struct anv_batch local_batch = {
      .start  = last_bbo->bo->map,
   .end    = last_bbo->bo->map + last_bbo->bo->size,
   .relocs = &last_bbo->relocs,
                           }
      static void
   anv_cmd_buffer_record_end_submit(struct anv_cmd_buffer *cmd_buffer)
   {
               struct anv_batch_bo *last_bbo =
                  uint32_t *batch = cmd_buffer->batch_end;
   anv_pack_struct(batch, GFX8_MI_BATCH_BUFFER_END,
      }
      static VkResult
   anv_cmd_buffer_chain_batch(struct anv_batch *batch, void *_data)
   {
      struct anv_cmd_buffer *cmd_buffer = _data;
   struct anv_batch_bo *new_bbo = NULL;
   /* Cap reallocation to chunk. */
   uint32_t alloc_size = MIN2(cmd_buffer->total_batch_size,
            VkResult result = anv_batch_bo_create(cmd_buffer, alloc_size, &new_bbo);
   if (result != VK_SUCCESS)
                     struct anv_batch_bo **seen_bbo = u_vector_add(&cmd_buffer->seen_bbos);
   if (seen_bbo == NULL) {
      anv_batch_bo_destroy(new_bbo, cmd_buffer);
      }
                                          }
      static VkResult
   anv_cmd_buffer_grow_batch(struct anv_batch *batch, void *_data)
   {
      struct anv_cmd_buffer *cmd_buffer = _data;
            anv_batch_bo_grow(cmd_buffer, bbo, &cmd_buffer->batch, 4096,
               }
      /** Allocate a binding table
   *
   * This function allocates a binding table.  This is a bit more complicated
   * than one would think due to a combination of Vulkan driver design and some
   * unfortunate hardware restrictions.
   *
   * The 3DSTATE_BINDING_TABLE_POINTERS_* packets only have a 16-bit field for
   * the binding table pointer which means that all binding tables need to live
   * in the bottom 64k of surface state base address.  The way the GL driver has
   * classically dealt with this restriction is to emit all surface states
   * on-the-fly into the batch and have a batch buffer smaller than 64k.  This
   * isn't really an option in Vulkan for a couple of reasons:
   *
   *  1) In Vulkan, we have growing (or chaining) batches so surface states have
   *     to live in their own buffer and we have to be able to re-emit
   *     STATE_BASE_ADDRESS as needed which requires a full pipeline stall.  In
   *     order to avoid emitting STATE_BASE_ADDRESS any more often than needed
   *     (it's not that hard to hit 64k of just binding tables), we allocate
   *     surface state objects up-front when VkImageView is created.  In order
   *     for this to work, surface state objects need to be allocated from a
   *     global buffer.
   *
   *  2) We tried to design the surface state system in such a way that it's
   *     already ready for bindless texturing.  The way bindless texturing works
   *     on our hardware is that you have a big pool of surface state objects
   *     (with its own state base address) and the bindless handles are simply
   *     offsets into that pool.  With the architecture we chose, we already
   *     have that pool and it's exactly the same pool that we use for regular
   *     surface states so we should already be ready for bindless.
   *
   *  3) For render targets, we need to be able to fill out the surface states
   *     later in vkBeginRenderPass so that we can assign clear colors
   *     correctly.  One way to do this would be to just create the surface
   *     state data and then repeatedly copy it into the surface state BO every
   *     time we have to re-emit STATE_BASE_ADDRESS.  While this works, it's
   *     rather annoying and just being able to allocate them up-front and
   *     re-use them for the entire render pass.
   *
   * While none of these are technically blockers for emitting state on the fly
   * like we do in GL, the ability to have a single surface state pool is
   * simplifies things greatly.  Unfortunately, it comes at a cost...
   *
   * Because of the 64k limitation of 3DSTATE_BINDING_TABLE_POINTERS_*, we can't
   * place the binding tables just anywhere in surface state base address.
   * Because 64k isn't a whole lot of space, we can't simply restrict the
   * surface state buffer to 64k, we have to be more clever.  The solution we've
   * chosen is to have a block pool with a maximum size of 2G that starts at
   * zero and grows in both directions.  All surface states are allocated from
   * the top of the pool (positive offsets) and we allocate blocks (< 64k) of
   * binding tables from the bottom of the pool (negative offsets).  Every time
   * we allocate a new binding table block, we set surface state base address to
   * point to the bottom of the binding table block.  This way all of the
   * binding tables in the block are in the bottom 64k of surface state base
   * address.  When we fill out the binding table, we add the distance between
   * the bottom of our binding table block and zero of the block pool to the
   * surface state offsets so that they are correct relative to out new surface
   * state base address at the bottom of the binding table block.
   *
   * \see adjust_relocations_from_block_pool()
   * \see adjust_relocations_too_block_pool()
   *
   * \param[in]  entries        The number of surface state entries the binding
   *                            table should be able to hold.
   *
   * \param[out] state_offset   The offset surface surface state base address
   *                            where the surface states live.  This must be
   *                            added to the surface state offset when it is
   *                            written into the binding table entry.
   *
   * \return                    An anv_state representing the binding table
   */
   struct anv_state
   anv_cmd_buffer_alloc_binding_table(struct anv_cmd_buffer *cmd_buffer,
         {
                        struct anv_state state = cmd_buffer->bt_next;
   if (bt_size > state.alloc_size)
            state.alloc_size = bt_size;
   cmd_buffer->bt_next.offset += bt_size;
   cmd_buffer->bt_next.map += bt_size;
            assert(bt_block->offset < 0);
               }
      struct anv_state
   anv_cmd_buffer_alloc_surface_state(struct anv_cmd_buffer *cmd_buffer)
   {
      struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
   return anv_state_stream_alloc(&cmd_buffer->surface_state_stream,
      }
      struct anv_state
   anv_cmd_buffer_alloc_dynamic_state(struct anv_cmd_buffer *cmd_buffer,
         {
      return anv_state_stream_alloc(&cmd_buffer->dynamic_state_stream,
      }
      VkResult
   anv_cmd_buffer_new_binding_table_block(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_state *bt_block = u_vector_add(&cmd_buffer->bt_block_states);
   if (bt_block == NULL) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_HOST_MEMORY);
                        /* The bt_next state is a rolling state (we update it as we suballocate
   * from it) which is relative to the start of the binding table block.
   */
   cmd_buffer->bt_next = *bt_block;
               }
      VkResult
   anv_cmd_buffer_init_batch_bo_chain(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_batch_bo *batch_bo = NULL;
                              result = anv_batch_bo_create(cmd_buffer,
               if (result != VK_SUCCESS)
                     cmd_buffer->batch.alloc = &cmd_buffer->vk.pool->alloc;
            if (cmd_buffer->device->can_chain_batches) {
         } else {
                  anv_batch_bo_start(batch_bo, &cmd_buffer->batch,
            int success = u_vector_init_pow2(&cmd_buffer->seen_bbos, 8,
         if (!success)
                     success = u_vector_init(&cmd_buffer->bt_block_states, 8,
         if (!success)
            result = anv_reloc_list_init(&cmd_buffer->surface_relocs,
         if (result != VK_SUCCESS)
                  result = anv_cmd_buffer_new_binding_table_block(cmd_buffer);
   if (result != VK_SUCCESS)
                  fail_bt_blocks:
         fail_seen_bbos:
         fail_batch_bo:
                  }
      void
   anv_cmd_buffer_fini_batch_bo_chain(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_state *bt_block;
   u_vector_foreach(bt_block, &cmd_buffer->bt_block_states)
                                    /* Destroy all of the batch buffers */
   list_for_each_entry_safe(struct anv_batch_bo, bbo,
            list_del(&bbo->link);
         }
      void
   anv_cmd_buffer_reset_batch_bo_chain(struct anv_cmd_buffer *cmd_buffer)
   {
      /* Delete all but the first batch bo */
   assert(!list_is_empty(&cmd_buffer->batch_bos));
   while (cmd_buffer->batch_bos.next != cmd_buffer->batch_bos.prev) {
      struct anv_batch_bo *bbo = anv_cmd_buffer_current_batch_bo(cmd_buffer);
   list_del(&bbo->link);
      }
            anv_batch_bo_start(anv_cmd_buffer_current_batch_bo(cmd_buffer),
                  while (u_vector_length(&cmd_buffer->bt_block_states) > 1) {
      struct anv_state *bt_block = u_vector_remove(&cmd_buffer->bt_block_states);
      }
   assert(u_vector_length(&cmd_buffer->bt_block_states) == 1);
   cmd_buffer->bt_next = *(struct anv_state *)u_vector_head(&cmd_buffer->bt_block_states);
            anv_reloc_list_clear(&cmd_buffer->surface_relocs);
            /* Reset the list of seen buffers */
   cmd_buffer->seen_bbos.head = 0;
                                 assert(!cmd_buffer->device->can_chain_batches ||
            }
      void
   anv_cmd_buffer_end_batch_buffer(struct anv_cmd_buffer *cmd_buffer)
   {
               if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      /* When we start a batch buffer, we subtract a certain amount of
   * padding from the end to ensure that we always have room to emit a
   * BATCH_BUFFER_START to chain to the next BO.  We need to remove
   * that padding before we end the batch; otherwise, we may end up
   * with our BATCH_BUFFER_END in another BO.
   */
   cmd_buffer->batch.end += GFX8_MI_BATCH_BUFFER_START_length * 4;
   assert(cmd_buffer->batch.start == batch_bo->bo->map);
            /* Save end instruction location to override it later. */
            /* If we can chain this command buffer to another one, leave some place
   * for the jump instruction.
   */
   batch_bo->chained = anv_cmd_buffer_is_chainable(cmd_buffer);
   if (batch_bo->chained)
         else
            /* Round batch up to an even number of dwords. */
   if ((cmd_buffer->batch.next - cmd_buffer->batch.start) & 4)
               } else {
      assert(cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
   /* If this is a secondary command buffer, we need to determine the
   * mode in which it will be executed with vkExecuteCommands.  We
   * determine this statically here so that this stays in sync with the
   * actual ExecuteCommands implementation.
   */
   const uint32_t length = cmd_buffer->batch.next - cmd_buffer->batch.start;
   if (!cmd_buffer->device->can_chain_batches) {
         } else if (cmd_buffer->device->physical->use_call_secondary) {
      cmd_buffer->exec_mode = ANV_CMD_BUFFER_EXEC_MODE_CALL_AND_RETURN;
   /* If the secondary command buffer begins & ends in the same BO and
   * its length is less than the length of CS prefetch, add some NOOPs
   * instructions so the last MI_BATCH_BUFFER_START is outside the CS
   * prefetch.
   */
   if (cmd_buffer->batch_bos.next == cmd_buffer->batch_bos.prev) {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   const enum intel_engine_class engine_class = cmd_buffer->queue_family->engine_class;
   /* Careful to have everything in signed integer. */
                  for (int32_t i = 0; i < (prefetch_len - batch_len); i += 4)
               void *jump_addr =
      anv_batch_emitn(&cmd_buffer->batch,
                  GFX8_MI_BATCH_BUFFER_START_length,
                  /* The emit above may have caused us to chain batch buffers which
   * would mean that batch_bo is no longer valid.
   */
      } else if ((cmd_buffer->batch_bos.next == cmd_buffer->batch_bos.prev) &&
            /* If the secondary has exactly one batch buffer in its list *and*
   * that batch buffer is less than half of the maximum size, we're
   * probably better of simply copying it into our batch.
   */
      } else if (!(cmd_buffer->usage_flags &
                     /* In order to chain, we need this command buffer to contain an
   * MI_BATCH_BUFFER_START which will jump back to the calling batch.
   * It doesn't matter where it points now so long as has a valid
   * relocation.  We'll adjust it later as part of the chaining
   * process.
   *
   * We set the end of the batch a little short so we would be sure we
   * have room for the chaining command.  Since we're about to emit the
   * chaining command, let's set it back where it should go.
   */
   cmd_buffer->batch.end += GFX8_MI_BATCH_BUFFER_START_length * 4;
                  emit_batch_buffer_start(cmd_buffer, batch_bo->bo, 0);
      } else {
                        }
      static VkResult
   anv_cmd_buffer_add_seen_bbos(struct anv_cmd_buffer *cmd_buffer,
         {
      list_for_each_entry(struct anv_batch_bo, bbo, list, link) {
      struct anv_batch_bo **bbo_ptr = u_vector_add(&cmd_buffer->seen_bbos);
   if (bbo_ptr == NULL)
                           }
      void
   anv_cmd_buffer_add_secondary(struct anv_cmd_buffer *primary,
         {
      anv_measure_add_secondary(primary, secondary);
   switch (secondary->exec_mode) {
   case ANV_CMD_BUFFER_EXEC_MODE_EMIT:
      anv_batch_emit_batch(&primary->batch, &secondary->batch);
      case ANV_CMD_BUFFER_EXEC_MODE_GROW_AND_EMIT: {
      struct anv_batch_bo *bbo = anv_cmd_buffer_current_batch_bo(primary);
   unsigned length = secondary->batch.end - secondary->batch.start;
   anv_batch_bo_grow(primary, bbo, &primary->batch, length,
         anv_batch_emit_batch(&primary->batch, &secondary->batch);
      }
   case ANV_CMD_BUFFER_EXEC_MODE_CHAIN: {
      struct anv_batch_bo *first_bbo =
         struct anv_batch_bo *last_bbo =
                     struct anv_batch_bo *this_bbo = anv_cmd_buffer_current_batch_bo(primary);
   assert(primary->batch.start == this_bbo->bo->map);
            /* Make the tail of the secondary point back to right after the
   * MI_BATCH_BUFFER_START in the primary batch.
   */
            anv_cmd_buffer_add_seen_bbos(primary, &secondary->batch_bos);
      }
   case ANV_CMD_BUFFER_EXEC_MODE_COPY_AND_CHAIN: {
      struct list_head copy_list;
   VkResult result = anv_batch_bo_list_clone(&secondary->batch_bos,
               if (result != VK_SUCCESS)
                     struct anv_batch_bo *first_bbo =
         struct anv_batch_bo *last_bbo =
                              anv_batch_bo_continue(last_bbo, &primary->batch,
            }
   case ANV_CMD_BUFFER_EXEC_MODE_CALL_AND_RETURN: {
      struct anv_batch_bo *first_bbo =
            uint64_t *write_return_addr =
      anv_batch_emitn(&primary->batch,
                                    *write_return_addr =
                  anv_cmd_buffer_add_seen_bbos(primary, &secondary->batch_bos);
      }
   default:
                  anv_reloc_list_append(&primary->surface_relocs, &primary->vk.pool->alloc,
      }
      struct anv_execbuf {
                        struct drm_i915_gem_exec_object2 *        objects;
   uint32_t                                  bo_count;
            /* Allocated length of the 'objects' and 'bos' arrays */
            uint32_t                                  syncobj_count;
   uint32_t                                  syncobj_array_length;
   struct drm_i915_gem_exec_fence *          syncobjs;
            /* List of relocations for surface states, only used with platforms not
   * using softpin.
   */
            uint32_t                                  cmd_buffer_count;
            /* Indicates whether any of the command buffers have relocations. This
   * doesn't not necessarily mean we'll need the kernel to process them. It
   * might be that a previous execbuf has already placed things in the VMA
   * and we can make i915 skip the relocations.
   */
            const VkAllocationCallbacks *             alloc;
               };
      static void
   anv_execbuf_finish(struct anv_execbuf *exec)
   {
      vk_free(exec->alloc, exec->syncobjs);
   vk_free(exec->alloc, exec->syncobj_values);
   vk_free(exec->alloc, exec->surface_states_relocs);
   vk_free(exec->alloc, exec->objects);
      }
      static void
   anv_execbuf_add_ext(struct anv_execbuf *exec,
               {
                        while (*iter != 0) {
                              }
      static VkResult
   anv_execbuf_add_bo_bitset(struct anv_device *device,
                              static VkResult
   anv_execbuf_add_bo(struct anv_device *device,
                     struct anv_execbuf *exec,
   {
                        if (bo->exec_obj_index < exec->bo_count &&
      exec->bos[bo->exec_obj_index] == bo)
         if (obj == NULL) {
      /* We've never seen this one before.  Add it to the list and assign
   * an id that we can use later.
   */
   if (exec->bo_count >= exec->array_length) {
               struct drm_i915_gem_exec_object2 *new_objects =
                        struct anv_bo **new_bos =
         if (new_bos == NULL) {
      vk_free(exec->alloc, new_objects);
               if (exec->objects) {
      memcpy(new_objects, exec->objects,
         memcpy(new_bos, exec->bos,
                              exec->objects = new_objects;
   exec->bos = new_bos;
                        bo->exec_obj_index = exec->bo_count++;
   obj = &exec->objects[bo->exec_obj_index];
            obj->handle = bo->gem_handle;
   obj->relocation_count = 0;
   obj->relocs_ptr = 0;
   obj->alignment = 0;
   obj->offset = bo->offset;
   obj->flags = bo->flags | extra_flags;
   obj->rsvd1 = 0;
               if (extra_flags & EXEC_OBJECT_WRITE) {
      obj->flags |= EXEC_OBJECT_WRITE;
               if (relocs != NULL) {
               if (relocs->num_relocs > 0) {
      /* This is the first time we've ever seen a list of relocations for
   * this BO.  Go ahead and set the relocations and then walk the list
   * of relocations and add them all.
   */
   exec->has_relocs = true;
                                    /* A quick sanity check on relocations */
   assert(relocs->relocs[i].offset < bo->size);
   result = anv_execbuf_add_bo(device, exec, relocs->reloc_bos[i],
         if (result != VK_SUCCESS)
                  return anv_execbuf_add_bo_bitset(device, exec, relocs->dep_words,
                  }
      /* Add BO dependencies to execbuf */
   static VkResult
   anv_execbuf_add_bo_bitset(struct anv_device *device,
                           {
      for (uint32_t w = 0; w < dep_words; w++) {
      BITSET_WORD mask = deps[w];
   while (mask) {
      int i = u_bit_scan(&mask);
   uint32_t gem_handle = w * BITSET_WORDBITS + i;
   struct anv_bo *bo = anv_device_lookup_bo(device, gem_handle);
   assert(bo->refcount > 0);
   VkResult result =
         if (result != VK_SUCCESS)
                     }
      static void
   anv_cmd_buffer_process_relocs(struct anv_cmd_buffer *cmd_buffer,
         {
      for (size_t i = 0; i < list->num_relocs; i++) {
      list->relocs[i].target_handle =
         }
      static void
   adjust_relocations_from_state_pool(struct anv_state_pool *pool,
               {
      assert(last_pool_center_bo_offset <= pool->block_pool.center_bo_offset);
            for (size_t i = 0; i < relocs->num_relocs; i++) {
      /* All of the relocations from this block pool to other BO's should
   * have been emitted relative to the surface block pool center.  We
   * need to add the center offset to make them relative to the
   * beginning of the actual GEM bo.
   */
         }
      static void
   adjust_relocations_to_state_pool(struct anv_state_pool *pool,
                     {
      assert(!from_bo->is_wrapper);
   assert(last_pool_center_bo_offset <= pool->block_pool.center_bo_offset);
            /* When we initially emit relocations into a block pool, we don't
   * actually know what the final center_bo_offset will be so we just emit
   * it as if center_bo_offset == 0.  Now that we know what the center
   * offset is, we need to walk the list of relocations and adjust any
   * relocations that point to the pool bo with the correct offset.
   */
   for (size_t i = 0; i < relocs->num_relocs; i++) {
      if (relocs->reloc_bos[i] == pool->block_pool.bo) {
      /* Adjust the delta value in the relocation to correctly
   * correspond to the new delta.  Initially, this value may have
   * been negative (if treated as unsigned), but we trust in
   * uint32_t roll-over to fix that for us at this point.
                  /* Since the delta has changed, we need to update the actual
   * relocated value with the new presumed value.  This function
   * should only be called on batch buffers, so we know it isn't in
   * use by the GPU at the moment.
   */
   assert(relocs->relocs[i].offset < from_bo->size);
   write_reloc(pool->block_pool.device,
                        }
      static void
   anv_reloc_list_apply(struct anv_device *device,
                     {
               for (size_t i = 0; i < list->num_relocs; i++) {
      struct anv_bo *target_bo = anv_bo_unwrap(list->reloc_bos[i]);
   if (list->relocs[i].presumed_offset == target_bo->offset &&
                  void *p = bo->map + list->relocs[i].offset;
   write_reloc(device, p, target_bo->offset + list->relocs[i].delta, true);
         }
      /**
   * This function applies the relocation for a command buffer and writes the
   * actual addresses into the buffers as per what we were told by the kernel on
   * the previous execbuf2 call.  This should be safe to do because, for each
   * relocated address, we have two cases:
   *
   *  1) The target BO is inactive (as seen by the kernel).  In this case, it is
   *     not in use by the GPU so updating the address is 100% ok.  It won't be
   *     in-use by the GPU (from our context) again until the next execbuf2
   *     happens.  If the kernel decides to move it in the next execbuf2, it
   *     will have to do the relocations itself, but that's ok because it should
   *     have all of the information needed to do so.
   *
   *  2) The target BO is active (as seen by the kernel).  In this case, it
   *     hasn't moved since the last execbuffer2 call because GTT shuffling
   *     *only* happens when the BO is idle. (From our perspective, it only
   *     happens inside the execbuffer2 ioctl, but the shuffling may be
   *     triggered by another ioctl, with full-ppgtt this is limited to only
   *     execbuffer2 ioctls on the same context, or memory pressure.)  Since the
   *     target BO hasn't moved, our anv_bo::offset exactly matches the BO's GTT
   *     address and the relocated value we are writing into the BO will be the
   *     same as the value that is already there.
   *
   *     There is also a possibility that the target BO is active but the exact
   *     RENDER_SURFACE_STATE object we are writing the relocation into isn't in
   *     use.  In this case, the address currently in the RENDER_SURFACE_STATE
   *     may be stale but it's still safe to write the relocation because that
   *     particular RENDER_SURFACE_STATE object isn't in-use by the GPU and
   *     won't be until the next execbuf2 call.
   *
   * By doing relocations on the CPU, we can tell the kernel that it doesn't
   * need to bother.  We want to do this because the surface state buffer is
   * used by every command buffer so, if the kernel does the relocations, it
   * will always be busy and the kernel will always stall.  This is also
   * probably the fastest mechanism for doing relocations since the kernel would
   * have to make a full copy of all the relocations lists.
   */
   static bool
   execbuf_can_skip_relocations(struct anv_execbuf *exec)
   {
      if (!exec->has_relocs)
            static int userspace_relocs = -1;
   if (userspace_relocs < 0)
         if (!userspace_relocs)
            /* First, we have to check to see whether or not we can even do the
   * relocation.  New buffers which have never been submitted to the kernel
   * don't have a valid offset so we need to let the kernel do relocations so
   * that we can get offsets for them.  On future execbuf2 calls, those
   * buffers will have offsets and we will be able to skip relocating.
   * Invalid offsets are indicated by anv_bo::offset == (uint64_t)-1.
   */
   for (uint32_t i = 0; i < exec->bo_count; i++) {
      assert(!exec->bos[i]->is_wrapper);
   if (exec->bos[i]->offset == (uint64_t)-1)
                  }
      static void
   relocate_cmd_buffer(struct anv_cmd_buffer *cmd_buffer,
         {
      /* Since surface states are shared between command buffers and we don't
   * know what order they will be submitted to the kernel, we don't know
   * what address is actually written in the surface state object at any
   * given time.  The only option is to always relocate them.
   */
   struct anv_bo *surface_state_bo =
         anv_reloc_list_apply(cmd_buffer->device, &cmd_buffer->surface_relocs,
                  /* Since we own all of the batch buffers, we know what values are stored
   * in the relocated addresses and only have to update them if the offsets
   * have changed.
   */
   struct anv_batch_bo **bbo;
   u_vector_foreach(bbo, &cmd_buffer->seen_bbos) {
      anv_reloc_list_apply(cmd_buffer->device,
               for (uint32_t i = 0; i < exec->bo_count; i++)
      }
      static void
   reset_cmd_buffer_surface_offsets(struct anv_cmd_buffer *cmd_buffer)
   {
      /* In the case where we fall back to doing kernel relocations, we need to
   * ensure that the relocation list is valid. All relocations on the batch
   * buffers are already valid and kept up-to-date. Since surface states are
   * shared between command buffers and we don't know what order they will be
   * submitted to the kernel, we don't know what address is actually written
   * in the surface state object at any given time. The only option is to set
   * a bogus presumed offset and let the kernel relocate them.
   */
   for (size_t i = 0; i < cmd_buffer->surface_relocs.num_relocs; i++)
      }
      static VkResult
   anv_execbuf_add_syncobj(struct anv_device *device,
                           {
      if (exec->syncobj_count >= exec->syncobj_array_length) {
               struct drm_i915_gem_exec_fence *new_syncobjs =
      vk_alloc(exec->alloc, new_len * sizeof(*new_syncobjs),
      if (!new_syncobjs)
            if (exec->syncobjs)
                     if (exec->syncobj_values) {
      uint64_t *new_syncobj_values =
      vk_alloc(exec->alloc, new_len * sizeof(*new_syncobj_values),
                                                            if (timeline_value && !exec->syncobj_values) {
      exec->syncobj_values =
      vk_zalloc(exec->alloc, exec->syncobj_array_length *
            if (!exec->syncobj_values)
               exec->syncobjs[exec->syncobj_count] = (struct drm_i915_gem_exec_fence) {
      .handle = syncobj,
      };
   if (exec->syncobj_values)
                        }
      static VkResult
   anv_execbuf_add_sync(struct anv_device *device,
                           {
      /* It's illegal to signal a timeline with value 0 because that's never
   * higher than the current value.  A timeline wait on value 0 is always
   * trivial because 0 <= uint64_t always.
   */
   if ((sync->flags & VK_SYNC_IS_TIMELINE) && value == 0)
            if (vk_sync_is_anv_bo_sync(sync)) {
      struct anv_bo_sync *bo_sync =
                     return anv_execbuf_add_bo(device, execbuf, bo_sync->bo, NULL,
      } else if (vk_sync_type_is_drm_syncobj(sync->type)) {
               if (!(sync->flags & VK_SYNC_IS_TIMELINE))
            return anv_execbuf_add_syncobj(device, execbuf, syncobj->syncobj,
                              }
      static VkResult
   setup_execbuf_for_cmd_buffer(struct anv_execbuf *execbuf,
         {
      struct anv_state_pool *ss_pool =
            adjust_relocations_from_state_pool(ss_pool, &cmd_buffer->surface_relocs,
         VkResult result;
   if (anv_use_relocations(cmd_buffer->device->physical)) {
      /* Since we aren't in the softpin case, all of our STATE_BASE_ADDRESS BOs
   * will get added automatically by processing relocations on the batch
   * buffer.  We have to add the surface state BO manually because it has
   * relocations of its own that we need to be sure are processed.
   */
   result = anv_execbuf_add_bo(cmd_buffer->device, execbuf,
               if (result != VK_SUCCESS)
      } else {
      /* Add surface dependencies (BOs) to the execbuf */
   result = anv_execbuf_add_bo_bitset(cmd_buffer->device, execbuf,
               if (result != VK_SUCCESS)
               /* First, we walk over all of the bos we've seen and add them and their
   * relocations to the validate list.
   */
   struct anv_batch_bo **bbo;
   u_vector_foreach(bbo, &cmd_buffer->seen_bbos) {
      adjust_relocations_to_state_pool(ss_pool, (*bbo)->bo, &(*bbo)->relocs,
            result = anv_execbuf_add_bo(cmd_buffer->device, execbuf,
         if (result != VK_SUCCESS)
               /* Now that we've adjusted all of the surface state relocations, we need to
   * record the surface state pool center so future executions of the command
   * buffer can adjust correctly.
   */
               }
      static void
   chain_command_buffers(struct anv_cmd_buffer **cmd_buffers,
         {
      if (!anv_cmd_buffer_is_chainable(cmd_buffers[0])) {
      assert(num_cmd_buffers == 1);
               /* Chain the N-1 first batch buffers */
   for (uint32_t i = 0; i < (num_cmd_buffers - 1); i++)
            /* Put an end to the last one */
      }
      static VkResult
   setup_execbuf_for_cmd_buffers(struct anv_execbuf *execbuf,
                     {
      struct anv_device *device = queue->device;
   struct anv_state_pool *ss_pool = &device->surface_state_pool;
            /* Edit the tail of the command buffers to chain them all together if they
   * can be.
   */
            for (uint32_t i = 0; i < num_cmd_buffers; i++) {
      anv_measure_submit(cmd_buffers[i]);
   result = setup_execbuf_for_cmd_buffer(execbuf, cmd_buffers[i]);
   if (result != VK_SUCCESS)
               /* Add all the global BOs to the object list for softpin case. */
   if (!anv_use_relocations(device->physical)) {
      anv_block_pool_foreach_bo(bo, &ss_pool->block_pool) {
      result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
               struct anv_block_pool *pool;
   pool = &device->dynamic_state_pool.block_pool;
   anv_block_pool_foreach_bo(bo, pool) {
      result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
               pool = &device->general_state_pool.block_pool;
   anv_block_pool_foreach_bo(bo, pool) {
      result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
               pool = &device->instruction_state_pool.block_pool;
   anv_block_pool_foreach_bo(bo, pool) {
      result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
               pool = &device->binding_table_pool.block_pool;
   anv_block_pool_foreach_bo(bo, pool) {
      result = anv_execbuf_add_bo(device, execbuf, bo, NULL, 0);
   if (result != VK_SUCCESS)
               /* Add the BOs for all user allocated memory objects because we can't
   * track after binding updates of VK_EXT_descriptor_indexing.
   */
   list_for_each_entry(struct anv_device_memory, mem,
            result = anv_execbuf_add_bo(device, execbuf, mem->bo, NULL, 0);
   if (result != VK_SUCCESS)
         } else {
      /* We do not support chaining primary command buffers without
   * softpin.
   */
               bool no_reloc = true;
   if (execbuf->has_relocs) {
      no_reloc = execbuf_can_skip_relocations(execbuf);
   if (no_reloc) {
      /* If we were able to successfully relocate everything, tell the
   * kernel that it can skip doing relocations. The requirement for
   * using NO_RELOC is:
   *
   *  1) The addresses written in the objects must match the
   *     corresponding reloc.presumed_offset which in turn must match
   *     the corresponding execobject.offset.
   *
   *  2) To avoid stalling, execobject.offset should match the current
   *     address of that object within the active context.
   *
   * In order to satisfy all of the invariants that make userspace
   * relocations to be safe (see relocate_cmd_buffer()), we need to
   * further ensure that the addresses we use match those used by the
   * kernel for the most recent execbuf2.
   *
   * The kernel may still choose to do relocations anyway if something
   * has moved in the GTT. In this case, the relocation list still
   * needs to be valid. All relocations on the batch buffers are
   * already valid and kept up-to-date. For surface state relocations,
   * by applying the relocations in relocate_cmd_buffer, we ensured
   * that the address in the RENDER_SURFACE_STATE matches
   * presumed_offset, so it should be safe for the kernel to relocate
   * them as needed.
   */
                     anv_reloc_list_apply(device, &cmd_buffers[i]->surface_relocs,
               } else {
      /* In the case where we fall back to doing kernel relocations, we
   * need to ensure that the relocation list is valid. All relocations
   * on the batch buffers are already valid and kept up-to-date. Since
   * surface states are shared between command buffers and we don't
   * know what order they will be submitted to the kernel, we don't
   * know what address is actually written in the surface state object
   * at any given time. The only option is to set a bogus presumed
   * offset and let the kernel relocate them.
   */
   for (uint32_t i = 0; i < num_cmd_buffers; i++)
                  struct anv_batch_bo *first_batch_bo =
            /* The kernel requires that the last entry in the validation list be the
   * batch buffer to execute.  We can simply swap the element
   * corresponding to the first batch_bo in the chain with the last
   * element in the list.
   */
   if (first_batch_bo->bo->exec_obj_index != execbuf->bo_count - 1) {
      uint32_t idx = first_batch_bo->bo->exec_obj_index;
            struct drm_i915_gem_exec_object2 tmp_obj = execbuf->objects[idx];
            execbuf->objects[idx] = execbuf->objects[last_idx];
   execbuf->bos[idx] = execbuf->bos[last_idx];
            execbuf->objects[last_idx] = tmp_obj;
   execbuf->bos[last_idx] = first_batch_bo->bo;
               /* If we are pinning our BOs, we shouldn't have to relocate anything */
   if (!anv_use_relocations(device->physical))
            /* Now we go through and fixup all of the relocation lists to point to the
   * correct indices in the object array (I915_EXEC_HANDLE_LUT).  We have to
   * do this after we reorder the list above as some of the indices may have
   * changed.
   */
   struct anv_batch_bo **bbo;
   if (execbuf->has_relocs) {
      assert(num_cmd_buffers == 1);
   u_vector_foreach(bbo, &cmd_buffers[0]->seen_bbos)
                     #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush) {
      __builtin_ia32_mfence();
   for (uint32_t i = 0; i < num_cmd_buffers; i++) {
      u_vector_foreach(bbo, &cmd_buffers[i]->seen_bbos) {
            }
         #endif
         struct anv_batch *batch = &cmd_buffers[0]->batch;
   execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   /* On platforms that cannot chain batch buffers because of the i915
   * command parser, we have to provide the batch length. Everywhere else
   * we'll chain batches so no point in passing a length.
   */
   .batch_len = device->can_chain_batches ? 0 : batch->next - batch->start,
   .cliprects_ptr = 0,
   .num_cliprects = 0,
   .DR1 = 0,
   .DR4 = 0,
   .flags = I915_EXEC_HANDLE_LUT | queue->exec_flags | (no_reloc ? I915_EXEC_NO_RELOC : 0),
   .rsvd1 = device->context_id,
                  }
      static VkResult
   setup_empty_execbuf(struct anv_execbuf *execbuf, struct anv_queue *queue)
   {
      struct anv_device *device = queue->device;
   VkResult result = anv_execbuf_add_bo(device, execbuf,
               if (result != VK_SUCCESS)
            execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   .batch_len = 8, /* GFX7_MI_BATCH_BUFFER_END and NOOP */
   .flags = I915_EXEC_HANDLE_LUT | queue->exec_flags | I915_EXEC_NO_RELOC,
   .rsvd1 = device->context_id,
                  }
      static VkResult
   setup_utrace_execbuf(struct anv_execbuf *execbuf, struct anv_queue *queue,
         {
      struct anv_device *device = queue->device;
   VkResult result = anv_execbuf_add_bo(device, execbuf,
               if (result != VK_SUCCESS)
            result = anv_execbuf_add_sync(device, execbuf, flush->sync,
         if (result != VK_SUCCESS)
            if (flush->batch_bo->exec_obj_index != execbuf->bo_count - 1) {
      uint32_t idx = flush->batch_bo->exec_obj_index;
            struct drm_i915_gem_exec_object2 tmp_obj = execbuf->objects[idx];
            execbuf->objects[idx] = execbuf->objects[last_idx];
   execbuf->bos[idx] = execbuf->bos[last_idx];
            execbuf->objects[last_idx] = tmp_obj;
   execbuf->bos[last_idx] = flush->batch_bo;
            #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
         execbuf->execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf->objects,
   .buffer_count = execbuf->bo_count,
   .batch_start_offset = 0,
   .batch_len = flush->batch.next - flush->batch.start,
   .flags = I915_EXEC_HANDLE_LUT | I915_EXEC_FENCE_ARRAY | queue->exec_flags |
         .rsvd1 = device->context_id,
   .rsvd2 = 0,
   .num_cliprects = execbuf->syncobj_count,
                  }
      static VkResult
   anv_queue_exec_utrace_locked(struct anv_queue *queue,
         {
               struct anv_device *device = queue->device;
   struct anv_execbuf execbuf = {
      .alloc = &device->vk.alloc,
               VkResult result = setup_utrace_execbuf(&execbuf, queue, flush);
   if (result != VK_SUCCESS)
            int ret = queue->device->info->no_hw ? 0 :
         if (ret)
            struct drm_i915_gem_exec_object2 *objects = execbuf.objects;
   for (uint32_t k = 0; k < execbuf.bo_count; k++) {
      if (anv_bo_is_pinned(execbuf.bos[k]))
                  error:
                  }
      /* We lock around execbuf for three main reasons:
   *
   *  1) When a block pool is resized, we create a new gem handle with a
   *     different size and, in the case of surface states, possibly a different
   *     center offset but we re-use the same anv_bo struct when we do so. If
   *     this happens in the middle of setting up an execbuf, we could end up
   *     with our list of BOs out of sync with our list of gem handles.
   *
   *  2) The algorithm we use for building the list of unique buffers isn't
   *     thread-safe. While the client is supposed to synchronize around
   *     QueueSubmit, this would be extremely difficult to debug if it ever came
   *     up in the wild due to a broken app. It's better to play it safe and
   *     just lock around QueueSubmit.
   *
   *  3) The anv_cmd_buffer_execbuf function may perform relocations in
   *      userspace. Due to the fact that the surface state buffer is shared
   *      between batches, we can't afford to have that happen from multiple
   *      threads at the same time. Even though the user is supposed to ensure
   *      this doesn't happen, we play it safe as in (2) above.
   *
   * Since the only other things that ever take the device lock such as block
   * pool resize only rarely happen, this will almost never be contended so
   * taking a lock isn't really an expensive operation in this case.
   */
   static VkResult
   anv_queue_exec_locked(struct anv_queue *queue,
                        uint32_t wait_count,
   const struct vk_sync_wait *waits,
   uint32_t cmd_buffer_count,
   struct anv_cmd_buffer **cmd_buffers,
      {
      struct anv_device *device = queue->device;
   struct anv_utrace_flush_copy *utrace_flush_data = NULL;
   struct anv_execbuf execbuf = {
      .alloc = &queue->device->vk.alloc,
   .alloc_scope = VK_SYSTEM_ALLOCATION_SCOPE_DEVICE,
               /* Flush the trace points first, they need to be moved */
   VkResult result =
      anv_device_utrace_flush_cmd_buffers(queue,
                  if (result != VK_SUCCESS)
            if (utrace_flush_data && !utrace_flush_data->batch_bo) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
                        /* Always add the workaround BO as it includes a driver identifier for the
   * error_state.
   */
   result =
         if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < wait_count; i++) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               for (uint32_t i = 0; i < signal_count; i++) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               if (queue->sync) {
      result = anv_execbuf_add_sync(device, &execbuf,
                     if (result != VK_SUCCESS)
               if (cmd_buffer_count) {
      result = setup_execbuf_for_cmd_buffers(&execbuf, queue,
            } else {
                  if (result != VK_SUCCESS)
            const bool has_perf_query =
            if (INTEL_DEBUG(DEBUG_SUBMIT)) {
      fprintf(stderr, "Batch offset=0x%x len=0x%x on queue 0\n",
         for (uint32_t i = 0; i < execbuf.bo_count; i++) {
               fprintf(stderr, "   BO: addr=0x%016"PRIx64"-0x%016"PRIx64" size=0x%010"PRIx64
                        if (INTEL_DEBUG(DEBUG_BATCH)) {
      fprintf(stderr, "Batch on queue %d\n", (int)(queue - device->queues));
   if (cmd_buffer_count) {
      if (has_perf_query) {
      struct anv_bo *pass_batch_bo = perf_query_pool->bo;
                  intel_print_batch(&device->decoder_ctx,
                     for (uint32_t i = 0; i < cmd_buffer_count; i++) {
      struct anv_batch_bo **bo =
         device->cmd_buffer_being_decoded = cmd_buffers[i];
   intel_print_batch(&device->decoder_ctx, (*bo)->bo->map,
               } else {
      intel_print_batch(&device->decoder_ctx,
                              if (execbuf.syncobj_values) {
      execbuf.timeline_fences.fence_count = execbuf.syncobj_count;
   execbuf.timeline_fences.handles_ptr = (uintptr_t)execbuf.syncobjs;
   execbuf.timeline_fences.values_ptr = (uintptr_t)execbuf.syncobj_values;
   anv_execbuf_add_ext(&execbuf,
            } else if (execbuf.syncobjs) {
      execbuf.execbuf.flags |= I915_EXEC_FENCE_ARRAY;
   execbuf.execbuf.num_cliprects = execbuf.syncobj_count;
               if (has_perf_query) {
      assert(perf_query_pass < perf_query_pool->n_passes);
   struct intel_perf_query_info *query_info =
            /* Some performance queries just the pipeline statistic HW, no need for
   * OA in that case, so no need to reconfigure.
   */
   if (!INTEL_DEBUG(DEBUG_NO_OACONFIG) &&
      (query_info->kind == INTEL_PERF_QUERY_TYPE_OA ||
   query_info->kind == INTEL_PERF_QUERY_TYPE_RAW)) {
   int ret = intel_ioctl(device->perf_fd, I915_PERF_IOCTL_CONFIG,
         if (ret < 0) {
      result = vk_device_set_lost(&device->vk,
                                 struct drm_i915_gem_exec_object2 query_pass_object = {
      .handle = pass_batch_bo->gem_handle,
   .offset = pass_batch_bo->offset,
      };
   struct drm_i915_gem_execbuffer2 query_pass_execbuf = {
      .buffers_ptr = (uintptr_t) &query_pass_object,
   .buffer_count = 1,
   .batch_start_offset = khr_perf_query_preamble_offset(perf_query_pool,
         .flags = I915_EXEC_HANDLE_LUT | queue->exec_flags,
               int ret = queue->device->info->no_hw ? 0 :
         if (ret)
               int ret = queue->device->info->no_hw ? 0 :
         if (ret)
            if (result == VK_SUCCESS && queue->sync) {
      result = vk_sync_wait(&device->vk, queue->sync, 0,
         if (result != VK_SUCCESS)
               struct drm_i915_gem_exec_object2 *objects = execbuf.objects;
   for (uint32_t k = 0; k < execbuf.bo_count; k++) {
      if (anv_bo_is_pinned(execbuf.bos[k]))
                  error:
               if (result == VK_SUCCESS && utrace_flush_data)
               }
      static inline bool
   can_chain_query_pools(struct anv_query_pool *p1, struct anv_query_pool *p2)
   {
         }
      static VkResult
   anv_queue_submit_locked(struct anv_queue *queue,
         {
               if (submit->command_buffer_count == 0) {
      result = anv_queue_exec_locked(queue, submit->wait_count, submit->waits,
                                 if (result != VK_SUCCESS)
      } else {
      /* Everything's easier if we don't have to bother with container_of() */
   STATIC_ASSERT(offsetof(struct anv_cmd_buffer, vk) == 0);
   struct vk_command_buffer **vk_cmd_buffers = submit->command_buffers;
   struct anv_cmd_buffer **cmd_buffers = (void *)vk_cmd_buffers;
   uint32_t start = 0;
   uint32_t end = submit->command_buffer_count;
   struct anv_query_pool *perf_query_pool =
         for (uint32_t n = 0; n < end; n++) {
      bool can_chain = false;
   uint32_t next = n + 1;
   /* Can we chain the last buffer into the next one? */
   if (next < end &&
      anv_cmd_buffer_is_chainable(cmd_buffers[next]) &&
   can_chain_query_pools
   (cmd_buffers[next]->perf_query_pool, perf_query_pool)) {
   can_chain = true;
   perf_query_pool =
      perf_query_pool ? perf_query_pool :
   }
   if (!can_chain) {
      /* The next buffer cannot be chained, or we have reached the
   * last buffer, submit what have been chained so far.
   */
   VkResult result =
      anv_queue_exec_locked(queue,
                        start == 0 ? submit->wait_count : 0,
   start == 0 ? submit->waits : NULL,
   next - start, &cmd_buffers[start],
   if (result != VK_SUCCESS)
         if (next < end) {
      start = next;
               }
   for (uint32_t i = 0; i < submit->signal_count; i++) {
      if (!vk_sync_is_anv_bo_sync(submit->signals[i].sync))
            struct anv_bo_sync *bo_sync =
            /* Once the execbuf has returned, we need to set the fence state to
   * SUBMITTED.  We can't do this before calling execbuf because
   * anv_GetFenceStatus does take the global device lock before checking
   * fence->state.
   *
   * We set the fence state to SUBMITTED regardless of whether or not the
   * execbuf succeeds because we need to ensure that vkWaitForFences() and
   * vkGetFenceStatus() return a valid result (VK_ERROR_DEVICE_LOST or
   * VK_SUCCESS) in a finite amount of time even if execbuf fails.
   */
   assert(bo_sync->state == ANV_BO_SYNC_STATE_RESET);
                           }
      VkResult
   anv_queue_submit(struct vk_queue *vk_queue,
         {
      struct anv_queue *queue = container_of(vk_queue, struct anv_queue, vk);
   struct anv_device *device = queue->device;
            if (queue->device->info->no_hw) {
      for (uint32_t i = 0; i < submit->signal_count; i++) {
      result = vk_sync_signal(&device->vk,
               if (result != VK_SUCCESS)
      }
                        pthread_mutex_lock(&device->mutex);
   result = anv_queue_submit_locked(queue, submit);
   /* Take submission ID under lock */
                        }
      VkResult
   anv_queue_submit_simple_batch(struct anv_queue *queue,
         {
      struct anv_device *device = queue->device;
   VkResult result = VK_SUCCESS;
            if (queue->device->info->no_hw)
            /* This is only used by device init so we can assume the queue is empty and
   * we aren't fighting with a submit thread.
   */
                     struct anv_bo *batch_bo = NULL;
   result = anv_bo_pool_alloc(&device->batch_bo_pool, batch_size, &batch_bo);
   if (result != VK_SUCCESS)
               #ifdef SUPPORT_INTEL_INTEGRATED_GPUS
      if (device->physical->memory.need_flush)
      #endif
         struct anv_execbuf execbuf = {
      .alloc = &queue->device->vk.alloc,
               result = anv_execbuf_add_bo(device, &execbuf, batch_bo, NULL, 0);
   if (result != VK_SUCCESS)
            if (INTEL_DEBUG(DEBUG_BATCH)) {
      intel_print_batch(&device->decoder_ctx,
                           execbuf.execbuf = (struct drm_i915_gem_execbuffer2) {
      .buffers_ptr = (uintptr_t) execbuf.objects,
   .buffer_count = execbuf.bo_count,
   .batch_start_offset = 0,
   .batch_len = batch_size,
   .flags = I915_EXEC_HANDLE_LUT | queue->exec_flags | I915_EXEC_NO_RELOC,
   .rsvd1 = device->context_id,
               err = anv_gem_execbuffer(device, &execbuf.execbuf);
   if (err) {
      result = vk_device_set_lost(&device->vk, "anv_gem_execbuffer failed: %m");
               result = anv_device_wait(device, batch_bo, INT64_MAX);
   if (result != VK_SUCCESS) {
      result = vk_device_set_lost(&device->vk,
                  fail:
      anv_execbuf_finish(&execbuf);
               }
