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
      #include "d3d12_batch.h"
   #include "d3d12_context.h"
   #include "d3d12_fence.h"
   #include "d3d12_query.h"
   #include "d3d12_residency.h"
   #include "d3d12_resource.h"
   #include "d3d12_resource_state.h"
   #include "d3d12_screen.h"
   #include "d3d12_surface.h"
      #include "util/hash_table.h"
   #include "util/set.h"
   #include "util/u_inlines.h"
      #include <dxguids/dxguids.h>
         unsigned d3d12_sampler_desc_table_key_hash(const void* key)
   {
                  }
   bool d3d12_sampler_desc_table_key_equals(const void* a, const void* b)
   {
      const d3d12_sampler_desc_table_key* table_a = (d3d12_sampler_desc_table_key*)a;
   const d3d12_sampler_desc_table_key* table_b = (d3d12_sampler_desc_table_key*)b;
      }
      bool
   d3d12_init_batch(struct d3d12_context *ctx, struct d3d12_batch *batch)
   {
               batch->bos = _mesa_hash_table_create(NULL, _mesa_hash_pointer,
         batch->sampler_tables = _mesa_hash_table_create(NULL, d3d12_sampler_desc_table_key_hash,
         batch->sampler_views = _mesa_set_create(NULL, _mesa_hash_pointer,
         batch->surfaces = _mesa_set_create(NULL, _mesa_hash_pointer,
         batch->objects = _mesa_set_create(NULL,
                  if (!batch->bos || !batch->sampler_tables || !batch->sampler_views || !batch->surfaces || !batch->objects)
            util_dynarray_init(&batch->zombie_samplers, NULL);
            if (FAILED(screen->dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                  batch->sampler_heap =
      d3d12_descriptor_heap_new(screen->dev,
                     batch->view_heap =
      d3d12_descriptor_heap_new(screen->dev,
                     if (!batch->sampler_heap && !batch->view_heap)
               }
      static inline void
   delete_bo(d3d12_bo *bo)
   {
         }
   static void
   delete_bo_entry(hash_entry *entry)
   {
      struct d3d12_bo *bo = (struct d3d12_bo *)entry->key;
      }
      static void
   delete_sampler_view_table(hash_entry *entry)
   {
      FREE((void*)entry->key);
      }
      static void
   delete_sampler_view(set_entry *entry)
   {
      struct pipe_sampler_view *pres = (struct pipe_sampler_view *)entry->key;
      }
      static void
   delete_surface(set_entry *entry)
   {
      struct pipe_surface *surf = (struct pipe_surface *)entry->key;
      }
      static void
   delete_object(set_entry *entry)
   {
      ID3D12Object *object = (ID3D12Object *)entry->key;
      }
      bool
   d3d12_reset_batch(struct d3d12_context *ctx, struct d3d12_batch *batch, uint64_t timeout_ns)
   {
      // batch hasn't been submitted before
   if (!batch->fence && !batch->has_errors)
            if (batch->fence) {
      if (!d3d12_fence_finish(batch->fence, timeout_ns))
                     _mesa_hash_table_clear(batch->bos, delete_bo_entry);
   _mesa_hash_table_clear(batch->sampler_tables, delete_sampler_view_table);
   _mesa_set_clear(batch->sampler_views, delete_sampler_view);
   _mesa_set_clear(batch->surfaces, delete_surface);
               util_dynarray_foreach(&batch->local_bos, d3d12_bo*, bo) {
      (*bo)->local_reference_mask[batch->ctx_id] &= ~(1 << batch->ctx_index);
      }
            util_dynarray_foreach(&batch->zombie_samplers, d3d12_descriptor_handle, handle)
                  d3d12_descriptor_heap_clear(batch->view_heap);
            if (FAILED(batch->cmdalloc->Reset())) {
      debug_printf("D3D12: resetting ID3D12CommandAllocator failed\n");
      }
   batch->has_errors = false;
   batch->pending_memory_barrier = false;
      }
      void
   d3d12_destroy_batch(struct d3d12_context *ctx, struct d3d12_batch *batch)
   {
      d3d12_reset_batch(ctx, batch, OS_TIMEOUT_INFINITE);
   batch->cmdalloc->Release();
   d3d12_descriptor_heap_free(batch->sampler_heap);
   d3d12_descriptor_heap_free(batch->view_heap);
   _mesa_hash_table_destroy(batch->bos, NULL);
   _mesa_hash_table_destroy(batch->sampler_tables, NULL);
   _mesa_set_destroy(batch->sampler_views, NULL);
   _mesa_set_destroy(batch->surfaces, NULL);
   _mesa_set_destroy(batch->objects, NULL);
   util_dynarray_fini(&batch->zombie_samplers);
      }
      void
   d3d12_start_batch(struct d3d12_context *ctx, struct d3d12_batch *batch)
   {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   ID3D12DescriptorHeap* heaps[2] = { d3d12_descriptor_heap_get(batch->view_heap),
                     /* Create or reset global command list */
   if (ctx->cmdlist) {
      if (FAILED(ctx->cmdlist->Reset(batch->cmdalloc, NULL))) {
      debug_printf("D3D12: resetting ID3D12GraphicsCommandList failed\n");
   batch->has_errors = true;
         } else {
      if (FAILED(screen->dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
                  debug_printf("D3D12: creating ID3D12GraphicsCommandList failed\n");
   batch->has_errors = true;
      }
   if (FAILED(ctx->cmdlist->QueryInterface(IID_PPV_ARGS(&ctx->cmdlist8)))) {
                     ctx->cmdlist->SetDescriptorHeaps(2, heaps);
   ctx->cmdlist_dirty = ~0;
   for (int i = 0; i < PIPE_SHADER_TYPES; ++i)
            if (!ctx->queries_disabled)
         if (ctx->current_predication)
               }
      void
   d3d12_end_batch(struct d3d12_context *ctx, struct d3d12_batch *batch)
   {
               if (!ctx->queries_disabled)
            if (FAILED(ctx->cmdlist->Close())) {
      debug_printf("D3D12: closing ID3D12GraphicsCommandList failed\n");
   batch->has_errors = true;
                     #ifndef _GAMING_XBOX
         #endif
                  ID3D12CommandList *cmdlists[] = { ctx->state_fixup_cmdlist, ctx->cmdlist };
   ID3D12CommandList **to_execute = cmdlists;
   UINT count_to_execute = ARRAY_SIZE(cmdlists);
   if (!has_state_fixup) {
      to_execute++;
      }
                     util_dynarray_foreach(&ctx->ended_queries, struct d3d12_query*, query) {
         }
               }
         inline uint8_t*
   d3d12_batch_get_reference(struct d3d12_batch *batch,
         {
      if (batch->ctx_id != D3D12_CONTEXT_NO_ID) {
      if ((bo->local_reference_mask[batch->ctx_id] & (1 << batch->ctx_index)) != 0) {
         }
   else
      }
   else {
      hash_entry* entry = _mesa_hash_table_search(batch->bos, bo);
   if (entry == NULL)
         else
         }
      inline uint8_t*
   d3d12_batch_acquire_reference(struct d3d12_batch *batch,
         {
      if (batch->ctx_id != D3D12_CONTEXT_NO_ID) {
      if ((bo->local_reference_mask[batch->ctx_id] & (1 << batch->ctx_index)) == 0) {
      d3d12_bo_reference(bo);
   util_dynarray_append(&batch->local_bos, d3d12_bo*, bo);
   bo->local_reference_mask[batch->ctx_id] |= (1 << batch->ctx_index);
      }
      }
   else {
      hash_entry* entry = _mesa_hash_table_search(batch->bos, bo);
   if (entry == NULL) {
      d3d12_bo_reference(bo);
                     }
      bool
   d3d12_batch_has_references(struct d3d12_batch *batch,
               {
      uint8_t*state = d3d12_batch_get_reference(batch, bo);
   if (state == NULL)
         bool resource_was_written = ((batch_bo_reference_state)(size_t)*state & batch_bo_reference_written) != 0;
      }
      void
   d3d12_batch_reference_resource(struct d3d12_batch *batch,
               {
               uint8_t new_data = write ? batch_bo_reference_written : batch_bo_reference_read;
   uint8_t old_data = (uint8_t)*state;
      }
      void
   d3d12_batch_reference_sampler_view(struct d3d12_batch *batch,
         {
      struct set_entry *entry = _mesa_set_search(batch->sampler_views, sv);
   if (!entry) {
      entry = _mesa_set_add(batch->sampler_views, sv);
                  }
      void
   d3d12_batch_reference_surface_texture(struct d3d12_batch *batch,
         {
         }
      void
   d3d12_batch_reference_object(struct d3d12_batch *batch,
         {
      struct set_entry *entry = _mesa_set_search(batch->objects, object);
   if (!entry) {
      entry = _mesa_set_add(batch->objects, object);
         }
