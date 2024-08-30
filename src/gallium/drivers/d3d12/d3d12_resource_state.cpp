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
      #include "d3d12_bufmgr.h"
   #include "d3d12_context.h"
   #include "d3d12_format.h"
   #include "d3d12_resource.h"
   #include "d3d12_resource_state.h"
   #include "d3d12_screen.h"
      #include <dxguids/dxguids.h>
      #include <assert.h>
      #define UNKNOWN_RESOURCE_STATE (D3D12_RESOURCE_STATES) 0x8000u
      static bool
   desired_resource_state_init(d3d12_desired_resource_state *state, uint32_t subresource_count)
   {
      state->homogenous = true;
   state->num_subresources = subresource_count;
   state->subresource_states = (D3D12_RESOURCE_STATES *)calloc(subresource_count, sizeof(D3D12_RESOURCE_STATES));
      }
      static void
   desired_resource_state_cleanup(d3d12_desired_resource_state *state)
   {
         }
      static D3D12_RESOURCE_STATES
   get_desired_subresource_state(const d3d12_desired_resource_state *state, uint32_t subresource_index)
   {
      if (state->homogenous)
            }
      static void
   update_subresource_state(D3D12_RESOURCE_STATES *existing_state, D3D12_RESOURCE_STATES new_state)
   {
      if (*existing_state == UNKNOWN_RESOURCE_STATE || new_state == UNKNOWN_RESOURCE_STATE ||
      d3d12_is_write_state(new_state)) {
      } else {
      /* Accumulate read state state bits */
         }
      static void
   set_desired_resource_state(d3d12_desired_resource_state *state_obj, D3D12_RESOURCE_STATES state)
   {
      state_obj->homogenous = true;
      }
      static void
   set_desired_subresource_state(d3d12_desired_resource_state *state_obj,
               {
      if (state_obj->homogenous && state_obj->num_subresources > 1) {
      for (unsigned i = 1; i < state_obj->num_subresources; ++i) {
         }
                  }
      static void
   reset_desired_resource_state(d3d12_desired_resource_state *state_obj)
   {
         }
      bool
   d3d12_resource_state_init(d3d12_resource_state *state, uint32_t subresource_count, bool simultaneous_access)
   {
      state->homogenous = true;
   state->supports_simultaneous_access = simultaneous_access;
   state->num_subresources = subresource_count;
   state->subresource_states = (d3d12_subresource_state *)calloc(subresource_count, sizeof(d3d12_subresource_state));
      }
      void
   d3d12_resource_state_cleanup(d3d12_resource_state *state)
   {
         }
      static const d3d12_subresource_state *
   get_subresource_state(const d3d12_resource_state *state, uint32_t subresource)
   {
      if (state->homogenous)
            }
      static void
   set_resource_state(d3d12_resource_state *state_obj, const d3d12_subresource_state *state)
   {
      state_obj->homogenous = true;
      }
      static void
   set_subresource_state(d3d12_resource_state *state_obj, uint32_t subresource, const d3d12_subresource_state *state)
   {
      if (state_obj->homogenous && state_obj->num_subresources > 1) {
      for (unsigned i = 1; i < state_obj->num_subresources; ++i) {
         }
                  }
      static void
   reset_resource_state(d3d12_resource_state *state)
   {
      d3d12_subresource_state subres_state = {};
      }
      static D3D12_RESOURCE_STATES
   resource_state_if_promoted(D3D12_RESOURCE_STATES desired_state,
               {
      const D3D12_RESOURCE_STATES promotable_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
                  if (simultaneous_access ||
      (desired_state & promotable_states) != D3D12_RESOURCE_STATE_COMMON) {
   // If the current state is COMMON...
   if (current_state->state == D3D12_RESOURCE_STATE_COMMON)
                  // If the current state is a read state resulting from previous promotion...
   if (current_state->is_promoted &&
      (current_state->state & D3D12_RESOURCE_STATE_GENERIC_READ) != D3D12_RESOURCE_STATE_COMMON)
   // ...then (accumulated) promotion is allowed
               }
      static void
   copy_resource_state(d3d12_resource_state *dest, d3d12_resource_state *src)
   {
      assert(dest->num_subresources == src->num_subresources);
   if (src->homogenous)
         else {
      dest->homogenous = false;
   for (unsigned i = 0; i < src->num_subresources; ++i)
         }
      void
   d3d12_destroy_context_state_table_entry(d3d12_context_state_table_entry *entry)
   {
      desired_resource_state_cleanup(&entry->desired);
   d3d12_resource_state_cleanup(&entry->batch_begin);
      }
      void
   d3d12_context_state_table_init(struct d3d12_context *ctx)
   {
      ctx->bo_state_table = _mesa_hash_table_u64_create(nullptr);
   ctx->pending_barriers_bos = _mesa_pointer_set_create(nullptr);
      }
      void
   d3d12_context_state_table_destroy(struct d3d12_context *ctx)
   {
      hash_table_foreach(ctx->bo_state_table->table, entry) {
      d3d12_destroy_context_state_table_entry((d3d12_context_state_table_entry *)entry->data);
      }
   _mesa_hash_table_u64_destroy(ctx->bo_state_table);
   util_dynarray_fini(&ctx->barrier_scratch);
   if (ctx->state_fixup_cmdlist)
            _mesa_set_destroy(ctx->pending_barriers_bos, nullptr);
      }
      static unsigned
   get_subresource_count(const D3D12_RESOURCE_DESC *desc)
   {
      unsigned array_size = desc->Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D ? 1 : desc->DepthOrArraySize;
      }
      static void
   init_state_table_entry(d3d12_context_state_table_entry *bo_state, d3d12_bo *bo)
   {
      /* Default parameters for bos for suballocated buffers */
   unsigned subresource_count = 1;
   bool supports_simultaneous_access = true;
   if (bo->res) {
      D3D12_RESOURCE_DESC desc = GetDesc(bo->res);
   subresource_count = get_subresource_count(&desc);
               desired_resource_state_init(&bo_state->desired, subresource_count);
            /* We'll never need state fixups for simultaneous access resources, so don't bother initializing this second state */
   if (!supports_simultaneous_access)
         else
      }
      static d3d12_context_state_table_entry *
   find_or_create_state_entry(struct d3d12_context *ctx, d3d12_bo *bo)
   {
      if (ctx->id != D3D12_CONTEXT_NO_ID) {
      unsigned context_bit = 1 << ctx->id;
   if ((bo->local_context_state_mask & context_bit) == 0) {
      init_state_table_entry(&bo->local_context_states[ctx->id], bo);
      }
               d3d12_context_state_table_entry *bo_state =
         if (!bo_state) {
      bo_state = CALLOC_STRUCT(d3d12_context_state_table_entry);
   init_state_table_entry(bo_state, bo);
      }
      }
      static ID3D12GraphicsCommandList *
   ensure_state_fixup_cmdlist(struct d3d12_context *ctx, ID3D12CommandAllocator *alloc)
   {
      if (!ctx->state_fixup_cmdlist) {
      struct d3d12_screen *screen = d3d12_screen(ctx->base.screen);
   screen->dev->CreateCommandList(0,
                        } else if (FAILED(ctx->state_fixup_cmdlist->Reset(alloc, nullptr))) {
      ctx->state_fixup_cmdlist->Release();
                  }
      static bool
   transition_required(D3D12_RESOURCE_STATES current_state, D3D12_RESOURCE_STATES *destination_state)
   {
      // An exact match never needs a transition.
   if (current_state == *destination_state) {
                  if (current_state == D3D12_RESOURCE_STATE_COMMON || *destination_state == D3D12_RESOURCE_STATE_COMMON) {
                  // Current state already contains the destination state, we're good.
   if ((current_state & *destination_state) == *destination_state) {
      *destination_state = current_state;
               // If the transition involves a write state, then the destination should just be the requested destination.
   // Otherwise, accumulate read states to minimize future transitions (by triggering the above condition).
   if (!d3d12_is_write_state(*destination_state) && !d3d12_is_write_state(current_state)) {
         }
      }
      static void
   resolve_global_state(struct d3d12_context *ctx, ID3D12Resource *res, d3d12_resource_state *batch_state, d3d12_resource_state *res_state)
   {
      assert(batch_state->num_subresources == res_state->num_subresources);
   unsigned num_subresources = batch_state->homogenous && res_state->homogenous ? 1 : batch_state->num_subresources;
   for (unsigned i = 0; i < num_subresources; ++i) {
      const d3d12_subresource_state *current_state = get_subresource_state(res_state, i);
   const d3d12_subresource_state *target_state = get_subresource_state(batch_state, i);
   D3D12_RESOURCE_STATES promotable_state =
            D3D12_RESOURCE_STATES after = target_state->state;
   if ((promotable_state & target_state->state) == target_state->state ||
                  D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
   barrier.Transition.pResource = res;
   barrier.Transition.StateBefore = current_state->state;
   barrier.Transition.StateAfter = after;
   barrier.Transition.Subresource = num_subresources == 1 ? D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES : i;
         }
      static void
   context_state_resolve_submission(struct d3d12_context *ctx, d3d12_bo *bo)
   {
      d3d12_context_state_table_entry *bo_state = find_or_create_state_entry(ctx, bo);
   if (!bo_state->batch_end.supports_simultaneous_access) {
                        copy_resource_state(&bo_state->batch_begin, &bo_state->batch_end);
      } else {
            }
      bool
   d3d12_context_state_resolve_submission(struct d3d12_context *ctx, struct d3d12_batch *batch)
   {
      util_dynarray_foreach(&ctx->recently_destroyed_bos, uint64_t, id) {
      void *data = _mesa_hash_table_u64_search(ctx->bo_state_table, *id);
   if (data)
                                 util_dynarray_foreach(&batch->local_bos, d3d12_bo*, bo)
         hash_table_foreach(batch->bos, bo_entry) 
                  bool needs_execute_fixup = false;
   if (ctx->barrier_scratch.size) {
      ID3D12GraphicsCommandList *cmdlist = ensure_state_fixup_cmdlist(ctx, batch->cmdalloc);
   if (cmdlist) {
      cmdlist->ResourceBarrier(util_dynarray_num_elements(&ctx->barrier_scratch, D3D12_RESOURCE_BARRIER),
                        }
      }
      static void
   append_barrier(struct d3d12_context *ctx,
                  d3d12_bo *bo,
   d3d12_context_state_table_entry *state_entry,
      {
      uint64_t offset;
   ID3D12Resource *res = d3d12_bo_get_base(bo, &offset)->res;
            D3D12_RESOURCE_BARRIER transition_desc = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
   transition_desc.Transition.pResource = res;
            // This is a transition into a state that is both write and non-write.
   // This is invalid according to D3D12. We're venturing into undefined behavior
   // land, but let's just pick the write state.
   if (d3d12_is_write_state(after) && (after & ~RESOURCE_STATE_ALL_WRITE_BITS) != 0) {
               // For now, this is the only way I've seen where this can happen.
               assert((subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES && current_state->homogenous) ||
                  // If the last time this state was set was in a different execution
   // period and is decayable then decay the current state to COMMON
   if (ctx->submit_id != current_subresource_state.execution_id && current_subresource_state.may_decay) {
      current_subresource_state.state = D3D12_RESOURCE_STATE_COMMON;
      }
   bool may_decay = false;
            D3D12_RESOURCE_STATES state_if_promoted =
            if (D3D12_RESOURCE_STATE_COMMON == state_if_promoted) {
      // No promotion
   if (current_subresource_state.state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
         after == D3D12_RESOURCE_STATE_UNORDERED_ACCESS &&
      D3D12_RESOURCE_BARRIER uav_barrier = { D3D12_RESOURCE_BARRIER_TYPE_UAV };
   uav_barrier.UAV.pResource = res;
      } else if (transition_required(current_subresource_state.state, /*inout*/ &after)) {
      // Insert a single concrete barrier (for non-simultaneous access resources).
   transition_desc.Transition.StateBefore = current_subresource_state.state;
   transition_desc.Transition.StateAfter = after;
                  may_decay = current_state->supports_simultaneous_access && !d3d12_is_write_state(after);
         } else if (after != state_if_promoted) {
      after = state_if_promoted;
   may_decay = !d3d12_is_write_state(after);
               d3d12_subresource_state new_subresource_state { after, ctx->submit_id, is_promotion, may_decay };
   if (subresource == D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
         else
      }
      void
   d3d12_transition_resource_state(struct d3d12_context *ctx,
                     {
      if (flags & D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS)
            d3d12_context_state_table_entry *state_entry = find_or_create_state_entry(ctx, res->bo);
   if (flags & D3D12_TRANSITION_FLAG_ACCUMULATE_STATE) {
               if (ctx->id != D3D12_CONTEXT_NO_ID) {
      if ((res->bo->local_needs_resolve_state & (1 << ctx->id)) == 0) {
      util_dynarray_append(&ctx->local_pending_barriers_bos, struct d3d12_bo*, res->bo);
         }
   else 
         } else if (state_entry->batch_end.homogenous) {
         } else {
      for (unsigned i = 0; i < state_entry->batch_end.num_subresources; ++i) {
               }
      void
   d3d12_transition_subresources_state(struct d3d12_context *ctx,
                                       {
      if(flags & D3D12_TRANSITION_FLAG_INVALIDATE_BINDINGS)
            d3d12_context_state_table_entry *state_entry = find_or_create_state_entry(ctx, res->bo);
   bool is_whole_resource = num_levels * num_layers * num_planes == state_entry->batch_end.num_subresources;
            if (is_whole_resource && is_accumulate) {
         } else if (is_whole_resource && state_entry->batch_end.homogenous) {
         } else {
      for (uint32_t l = 0; l < num_levels; l++) {
      const uint32_t level = start_level + l;
   for (uint32_t a = 0; a < num_layers; a++) {
      const uint32_t layer = start_layer + a;
   for (uint32_t p = 0; p < num_planes; p++) {
      const uint32_t plane = start_plane + p;
   uint32_t subres_id =
         assert(subres_id < state_entry->desired.num_subresources);
   if (is_accumulate)
         else
                        if (is_accumulate) {
      if (ctx->id != D3D12_CONTEXT_NO_ID) {
      if ((res->bo->local_needs_resolve_state & (1 << ctx->id)) == 0) {
      util_dynarray_append(&ctx->local_pending_barriers_bos, struct d3d12_bo*, res->bo);
         }
   else
         }
      static void apply_resource_state(struct d3d12_context *ctx, bool is_implicit_dispatch, d3d12_bo *bo)
   {
      d3d12_context_state_table_entry* state_entry = find_or_create_state_entry(ctx, bo);
   d3d12_desired_resource_state* destination_state = &state_entry->desired;
            // Figure out the set of subresources that are transitioning
            UINT num_subresources = all_resources_at_once ? 1 : current_state->num_subresources;
   for (UINT i = 0; i < num_subresources; ++i) {
      D3D12_RESOURCE_STATES after = get_desired_subresource_state(destination_state, i);
            // Is this subresource currently being used, or is it just being iterated over?
   if (after == UNKNOWN_RESOURCE_STATE) {
      // This subresource doesn't have any transition requested - move on to the next.
                           // Update destination states.
      }
      void
   d3d12_apply_resource_states(struct d3d12_context *ctx, bool is_implicit_dispatch)
   {
      set_foreach_remove(ctx->pending_barriers_bos, entry) 
            util_dynarray_foreach(&ctx->local_pending_barriers_bos, struct d3d12_bo*, bo) {
      apply_resource_state(ctx, is_implicit_dispatch, *bo);
      }
            if (ctx->barrier_scratch.size) {
      ctx->cmdlist->ResourceBarrier(util_dynarray_num_elements(&ctx->barrier_scratch, D3D12_RESOURCE_BARRIER),
               }
