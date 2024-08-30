   /*
   * Copyright © 2008 Jérôme Glisse
   * Copyright © 2010 Marek Olšák <maraeo@gmail.com>
   * Copyright © 2015 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "amdgpu_cs.h"
   #include "util/os_time.h"
   #include <inttypes.h>
   #include <stdio.h>
      #include "amd/common/sid.h"
      /* FENCES */
      static struct pipe_fence_handle *
   amdgpu_fence_create(struct amdgpu_ctx *ctx, unsigned ip_type)
   {
               fence->reference.count = 1;
   fence->ws = ctx->ws;
   fence->ctx = ctx;
   fence->fence.context = ctx->ctx;
   fence->fence.ip_type = ip_type;
   util_queue_fence_init(&fence->submitted);
   util_queue_fence_reset(&fence->submitted);
   p_atomic_inc(&ctx->refcount);
      }
      static struct pipe_fence_handle *
   amdgpu_fence_import_syncobj(struct radeon_winsys *rws, int fd)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   struct amdgpu_fence *fence = CALLOC_STRUCT(amdgpu_fence);
            if (!fence)
            pipe_reference_init(&fence->reference, 1);
            r = amdgpu_cs_import_syncobj(ws->dev, fd, &fence->syncobj);
   if (r) {
      FREE(fence);
                        assert(amdgpu_fence_is_syncobj(fence));
      }
      static struct pipe_fence_handle *
   amdgpu_fence_import_sync_file(struct radeon_winsys *rws, int fd)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
            if (!fence)
            pipe_reference_init(&fence->reference, 1);
   fence->ws = ws;
            /* Convert sync_file into syncobj. */
   int r = amdgpu_cs_create_syncobj(ws->dev, &fence->syncobj);
   if (r) {
      FREE(fence);
               r = amdgpu_cs_syncobj_import_sync_file(ws->dev, fence->syncobj, fd);
   if (r) {
      amdgpu_cs_destroy_syncobj(ws->dev, fence->syncobj);
   FREE(fence);
                           }
      static int amdgpu_fence_export_sync_file(struct radeon_winsys *rws,
         {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
            if (amdgpu_fence_is_syncobj(fence)) {
               /* Convert syncobj into sync_file. */
   r = amdgpu_cs_syncobj_export_sync_file(ws->dev, fence->syncobj, &fd);
                        /* Convert the amdgpu fence into a fence FD. */
   int fd;
   if (amdgpu_cs_fence_to_handle(ws->dev, &fence->fence,
                           }
      static int amdgpu_export_signalled_sync_file(struct radeon_winsys *rws)
   {
      struct amdgpu_winsys *ws = amdgpu_winsys(rws);
   uint32_t syncobj;
            int r = amdgpu_cs_create_syncobj2(ws->dev, DRM_SYNCOBJ_CREATE_SIGNALED,
         if (r) {
                  r = amdgpu_cs_syncobj_export_sync_file(ws->dev, syncobj, &fd);
   if (r) {
                  amdgpu_cs_destroy_syncobj(ws->dev, syncobj);
      }
      static void amdgpu_fence_submitted(struct pipe_fence_handle *fence,
               {
               afence->fence.fence = seq_no;
   afence->user_fence_cpu_address = user_fence_cpu_address;
      }
      static void amdgpu_fence_signalled(struct pipe_fence_handle *fence)
   {
               afence->signalled = true;
      }
      bool amdgpu_fence_wait(struct pipe_fence_handle *fence, uint64_t timeout,
         {
      struct amdgpu_fence *afence = (struct amdgpu_fence*)fence;
   uint32_t expired;
   int64_t abs_timeout;
   uint64_t *user_fence_cpu;
            if (afence->signalled)
            if (absolute)
         else
            /* Handle syncobjs. */
   if (amdgpu_fence_is_syncobj(afence)) {
      if (abs_timeout == OS_TIMEOUT_INFINITE)
            if (amdgpu_cs_syncobj_wait(afence->ws->dev, &afence->syncobj, 1,
                  afence->signalled = true;
               /* The fence might not have a number assigned if its IB is being
   * submitted in the other thread right now. Wait until the submission
   * is done. */
   if (!util_queue_fence_wait_timeout(&afence->submitted, abs_timeout))
            user_fence_cpu = afence->user_fence_cpu_address;
   if (user_fence_cpu) {
      if (*user_fence_cpu >= afence->fence.fence) {
      afence->signalled = true;
               /* No timeout, just query: no need for the ioctl. */
   if (!absolute && !timeout)
               /* Now use the libdrm query. */
   r = amdgpu_cs_query_fence_status(&afence->fence,
      abs_timeout,
   AMDGPU_QUERY_FENCE_TIMEOUT_IS_ABSOLUTE,
      if (r) {
      fprintf(stderr, "amdgpu: amdgpu_cs_query_fence_status failed.\n");
               if (expired) {
      /* This variable can only transition from false to true, so it doesn't
   * matter if threads race for it. */
   afence->signalled = true;
      }
      }
      static bool amdgpu_fence_wait_rel_timeout(struct radeon_winsys *rws,
               {
         }
      static struct pipe_fence_handle *
   amdgpu_cs_get_next_fence(struct radeon_cmdbuf *rcs)
   {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
            if (cs->noop)
            if (cs->next_fence) {
      amdgpu_fence_reference(&fence, cs->next_fence);
               fence = amdgpu_fence_create(cs->ctx,
         if (!fence)
            amdgpu_fence_reference(&cs->next_fence, fence);
      }
      /* CONTEXTS */
      static uint32_t
   radeon_to_amdgpu_priority(enum radeon_ctx_priority radeon_priority)
   {
      switch (radeon_priority) {
   case RADEON_CTX_PRIORITY_REALTIME:
         case RADEON_CTX_PRIORITY_HIGH:
         case RADEON_CTX_PRIORITY_MEDIUM:
         case RADEON_CTX_PRIORITY_LOW:
         default:
            }
      static struct radeon_winsys_ctx *amdgpu_ctx_create(struct radeon_winsys *ws,
               {
      struct amdgpu_ctx *ctx = CALLOC_STRUCT(amdgpu_ctx);
   int r;
   struct amdgpu_bo_alloc_request alloc_buffer = {};
   uint32_t amdgpu_priority = radeon_to_amdgpu_priority(priority);
            if (!ctx)
            ctx->ws = amdgpu_winsys(ws);
   ctx->refcount = 1;
            r = amdgpu_cs_ctx_create2(ctx->ws->dev, amdgpu_priority, &ctx->ctx);
   if (r) {
      fprintf(stderr, "amdgpu: amdgpu_cs_ctx_create2 failed. (%i)\n", r);
               alloc_buffer.alloc_size = ctx->ws->info.gart_page_size;
   alloc_buffer.phys_alignment = ctx->ws->info.gart_page_size;
            r = amdgpu_bo_alloc(ctx->ws->dev, &alloc_buffer, &buf_handle);
   if (r) {
      fprintf(stderr, "amdgpu: amdgpu_bo_alloc failed. (%i)\n", r);
               r = amdgpu_bo_cpu_map(buf_handle, (void**)&ctx->user_fence_cpu_address_base);
   if (r) {
      fprintf(stderr, "amdgpu: amdgpu_bo_cpu_map failed. (%i)\n", r);
               memset(ctx->user_fence_cpu_address_base, 0, alloc_buffer.alloc_size);
                  error_user_fence_map:
         error_user_fence_alloc:
         error_create:
      FREE(ctx);
      }
      static void amdgpu_ctx_destroy(struct radeon_winsys_ctx *rwctx)
   {
         }
      static void amdgpu_pad_gfx_compute_ib(struct amdgpu_winsys *ws, enum amd_ip_type ip_type,
         {
      unsigned pad_dw_mask = ws->info.ip[ip_type].ib_pad_dw_mask;
            if (unaligned_dw) {
               /* Only pad by 1 dword with the type-2 NOP if necessary. */
   if (remaining == 1 && ws->info.gfx_ib_pad_with_type2) {
         } else {
      /* Pad with a single NOP packet to minimize CP overhead because NOP is a variable-sized
   * packet. The size of the packet body after the header is always count + 1.
   * If count == -1, there is no packet body. NOP is the only packet that can have
   * count == -1, which is the definition of PKT3_NOP_PAD (count == 0x3fff means -1).
   */
   ib[(*num_dw)++] = PKT3(PKT3_NOP, remaining - 2, 0);
         }
      }
      static int amdgpu_submit_gfx_nop(struct amdgpu_ctx *ctx)
   {
      struct amdgpu_bo_alloc_request request = {0};
   struct drm_amdgpu_bo_list_in bo_list_in;
   struct drm_amdgpu_cs_chunk_ib ib_in = {0};
   amdgpu_bo_handle buf_handle;
   amdgpu_va_handle va_handle = NULL;
   struct drm_amdgpu_cs_chunk chunks[2];
   void *cpu = NULL;
   uint64_t seq_no;
   uint64_t va;
            /* Older amdgpu doesn't report if the reset is complete or not. Detect
   * it by submitting a no-op job. If it reports an error, then assume
   * that the reset is not complete.
   */
   amdgpu_context_handle temp_ctx;
   r = amdgpu_cs_ctx_create2(ctx->ws->dev, AMDGPU_CTX_PRIORITY_NORMAL, &temp_ctx);
   if (r)
            request.preferred_heap = AMDGPU_GEM_DOMAIN_VRAM;
   request.alloc_size = 4096;
   request.phys_alignment = 4096;
   r = amdgpu_bo_alloc(ctx->ws->dev, &request, &buf_handle);
   if (r)
            r = amdgpu_va_range_alloc(ctx->ws->dev, amdgpu_gpu_va_range_general,
               request.alloc_size, request.phys_alignment,
   if (r)
         r = amdgpu_bo_va_op_raw(ctx->ws->dev, buf_handle, 0, request.alloc_size, va,
               if (r)
            r = amdgpu_bo_cpu_map(buf_handle, &cpu);
   if (r)
            unsigned noop_dw_size = ctx->ws->info.ip[AMD_IP_GFX].ib_pad_dw_mask + 1;
                     struct drm_amdgpu_bo_list_entry list;
   amdgpu_bo_export(buf_handle, amdgpu_bo_handle_type_kms, &list.bo_handle);
            bo_list_in.list_handle = ~0;
   bo_list_in.bo_number = 1;
   bo_list_in.bo_info_size = sizeof(struct drm_amdgpu_bo_list_entry);
            ib_in.ip_type = AMD_IP_GFX;
   ib_in.ib_bytes = noop_dw_size * 4;
            chunks[0].chunk_id = AMDGPU_CHUNK_ID_BO_HANDLES;
   chunks[0].length_dw = sizeof(struct drm_amdgpu_bo_list_in) / 4;
            chunks[1].chunk_id = AMDGPU_CHUNK_ID_IB;
   chunks[1].length_dw = sizeof(struct drm_amdgpu_cs_chunk_ib) / 4;
                  destroy_bo:
      if (va_handle)
            destroy_ctx:
                  }
      static void
   amdgpu_ctx_set_sw_reset_status(struct radeon_winsys_ctx *rwctx, enum pipe_reset_status status,
         {
               /* Don't overwrite the last reset status. */
   if (ctx->sw_status != PIPE_NO_RESET)
                     if (!ctx->allow_context_lost) {
               va_start(args, format);
   vfprintf(stderr, format, args);
            /* Non-robust contexts are allowed to terminate the process. The only alternative is
   * to skip command submission, which would look like a freeze because nothing is drawn,
   * which looks like a hang without any reset.
   */
         }
      static enum pipe_reset_status
   amdgpu_ctx_query_reset_status(struct radeon_winsys_ctx *rwctx, bool full_reset_only,
         {
      struct amdgpu_ctx *ctx = (struct amdgpu_ctx*)rwctx;
            if (needs_reset)
         if (reset_completed)
            /* Return a failure due to a GPU hang. */
   if (ctx->ws->info.drm_minor >= 24) {
               if (full_reset_only && ctx->sw_status == PIPE_NO_RESET) {
      /* If the caller is only interested in full reset (= wants to ignore soft
   * recoveries), we can use the rejected cs count as a quick first check.
   */
               r = amdgpu_cs_query_reset_state2(ctx->ctx, &flags);
   if (r) {
      fprintf(stderr, "amdgpu: amdgpu_cs_query_reset_state2 failed. (%i)\n", r);
               if (flags & AMDGPU_CTX_QUERY2_FLAGS_RESET) {
      if (reset_completed) {
      /* The ARB_robustness spec says:
   *
   *    If a reset status other than NO_ERROR is returned and subsequent
   *    calls return NO_ERROR, the context reset was encountered and
   *    completed. If a reset status is repeatedly returned, the context may
   *    be in the process of resetting.
   *
   * Starting with drm_minor >= 54 amdgpu reports if the reset is complete,
   * so don't do anything special. On older kernels, submit a no-op cs. If it
   * succeeds then assume the reset is complete.
   */
                  if (ctx->ws->info.drm_minor < 54 && ctx->ws->info.has_graphics)
               if (needs_reset)
         if (flags & AMDGPU_CTX_QUERY2_FLAGS_GUILTY)
         else
         } else {
               r = amdgpu_cs_query_reset_state(ctx->ctx, &result, &hangs);
   if (r) {
      fprintf(stderr, "amdgpu: amdgpu_cs_query_reset_state failed. (%i)\n", r);
               if (needs_reset)
         switch (result) {
   case AMDGPU_CTX_GUILTY_RESET:
         case AMDGPU_CTX_INNOCENT_RESET:
         case AMDGPU_CTX_UNKNOWN_RESET:
                     /* Return a failure due to SW issues. */
   if (ctx->sw_status != PIPE_NO_RESET) {
      if (needs_reset)
            }
   if (needs_reset)
            }
      /* COMMAND SUBMISSION */
      static bool amdgpu_cs_has_user_fence(struct amdgpu_cs_context *cs)
   {
      return cs->ib[IB_MAIN].ip_type != AMDGPU_HW_IP_UVD &&
         cs->ib[IB_MAIN].ip_type != AMDGPU_HW_IP_VCE &&
   cs->ib[IB_MAIN].ip_type != AMDGPU_HW_IP_UVD_ENC &&
   cs->ib[IB_MAIN].ip_type != AMDGPU_HW_IP_VCN_DEC &&
      }
      static inline unsigned amdgpu_cs_epilog_dws(struct amdgpu_cs *cs)
   {
      if (cs->has_chaining)
               }
      static int amdgpu_lookup_buffer(struct amdgpu_cs_context *cs, struct amdgpu_winsys_bo *bo,
         {
      unsigned hash = bo->unique_id & (BUFFER_HASHLIST_SIZE-1);
            /* not found or found */
   if (i < 0 || (i < num_buffers && buffers[i].bo == bo))
            /* Hash collision, look for the BO in the list of buffers linearly. */
   for (int i = num_buffers - 1; i >= 0; i--) {
      if (buffers[i].bo == bo) {
      /* Put this buffer in the hash list.
   * This will prevent additional hash collisions if there are
   * several consecutive lookup_buffer calls for the same buffer.
   *
   * Example: Assuming buffers A,B,C collide in the hash list,
   * the following sequence of buffers:
   *         AAAAAAAAAAABBBBBBBBBBBBBBCCCCCCCC
   * will collide here: ^ and here:   ^,
   * meaning that we should get very few collisions in the end. */
   cs->buffer_indices_hashlist[hash] = i & 0x7fff;
         }
      }
      int amdgpu_lookup_buffer_any_type(struct amdgpu_cs_context *cs, struct amdgpu_winsys_bo *bo)
   {
      struct amdgpu_cs_buffer *buffers;
            if (bo->bo) {
      buffers = cs->real_buffers;
      } else if (!(bo->base.usage & RADEON_FLAG_SPARSE)) {
      buffers = cs->slab_buffers;
      } else {
      buffers = cs->sparse_buffers;
                  }
      static int
   amdgpu_do_add_real_buffer(struct amdgpu_cs_context *cs,
         {
      struct amdgpu_cs_buffer *buffer;
            /* New buffer, check if the backing array is large enough. */
   if (cs->num_real_buffers >= cs->max_real_buffers) {
      unsigned new_max =
                           if (!new_buffers) {
      fprintf(stderr, "amdgpu_do_add_buffer: allocation failed\n");
   FREE(new_buffers);
                                 cs->max_real_buffers = new_max;
               idx = cs->num_real_buffers;
            memset(buffer, 0, sizeof(*buffer));
   amdgpu_winsys_bo_reference(cs->ws, &buffer->bo, bo);
               }
      static int
   amdgpu_lookup_or_add_real_buffer(struct radeon_cmdbuf *rcs, struct amdgpu_cs_context *cs,
         {
      unsigned hash;
            if (idx >= 0)
                     hash = bo->unique_id & (BUFFER_HASHLIST_SIZE-1);
            if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else if (bo->base.placement & RADEON_DOMAIN_GTT)
               }
      static int amdgpu_lookup_or_add_slab_buffer(struct radeon_cmdbuf *rcs,
               {
      struct amdgpu_cs_buffer *buffer;
   unsigned hash;
   int idx = amdgpu_lookup_buffer(cs, bo, cs->slab_buffers, cs->num_slab_buffers);
            if (idx >= 0)
            real_idx = amdgpu_lookup_or_add_real_buffer(rcs, cs, bo->u.slab.real);
   if (real_idx < 0)
            /* New buffer, check if the backing array is large enough. */
   if (cs->num_slab_buffers >= cs->max_slab_buffers) {
      unsigned new_max =
                  new_buffers = REALLOC(cs->slab_buffers,
               if (!new_buffers) {
      fprintf(stderr, "amdgpu_lookup_or_add_slab_buffer: allocation failed\n");
               cs->max_slab_buffers = new_max;
               idx = cs->num_slab_buffers;
            memset(buffer, 0, sizeof(*buffer));
   amdgpu_winsys_bo_reference(cs->ws, &buffer->bo, bo);
   buffer->slab_real_idx = real_idx;
            hash = bo->unique_id & (BUFFER_HASHLIST_SIZE-1);
               }
      static int amdgpu_lookup_or_add_sparse_buffer(struct radeon_cmdbuf *rcs,
               {
      struct amdgpu_cs_buffer *buffer;
   unsigned hash;
            if (idx >= 0)
            /* New buffer, check if the backing array is large enough. */
   if (cs->num_sparse_buffers >= cs->max_sparse_buffers) {
      unsigned new_max =
                  new_buffers = REALLOC(cs->sparse_buffers,
               if (!new_buffers) {
      fprintf(stderr, "amdgpu_lookup_or_add_sparse_buffer: allocation failed\n");
               cs->max_sparse_buffers = new_max;
               idx = cs->num_sparse_buffers;
            memset(buffer, 0, sizeof(*buffer));
   amdgpu_winsys_bo_reference(cs->ws, &buffer->bo, bo);
            hash = bo->unique_id & (BUFFER_HASHLIST_SIZE-1);
            /* We delay adding the backing buffers until we really have to. However,
   * we cannot delay accounting for memory use.
   */
            list_for_each_entry(struct amdgpu_sparse_backing, backing, &bo->u.sparse.backing, list) {
      if (bo->base.placement & RADEON_DOMAIN_VRAM)
         else if (bo->base.placement & RADEON_DOMAIN_GTT)
                           }
      static unsigned amdgpu_cs_add_buffer(struct radeon_cmdbuf *rcs,
                     {
      /* Don't use the "domains" parameter. Amdgpu doesn't support changing
   * the buffer placement during command submission.
   */
   struct amdgpu_cs_context *cs = (struct amdgpu_cs_context*)rcs->csc;
   struct amdgpu_winsys_bo *bo = (struct amdgpu_winsys_bo*)buf;
   struct amdgpu_cs_buffer *buffer;
            /* Fast exit for no-op calls.
   * This is very effective with suballocators and linear uploaders that
   * are outside of the winsys.
   */
   if (bo == cs->last_added_bo &&
      (usage & cs->last_added_bo_usage) == usage)
         if (!(bo->base.usage & RADEON_FLAG_SPARSE)) {
      if (!bo->bo) {
      index = amdgpu_lookup_or_add_slab_buffer(rcs, cs, bo);
                  buffer = &cs->slab_buffers[index];
                  index = buffer->slab_real_idx;
   buffer = &cs->real_buffers[index];
      } else {
      index = amdgpu_lookup_or_add_real_buffer(rcs, cs, bo);
                  buffer = &cs->real_buffers[index];
   buffer->usage |= usage;
         } else {
      index = amdgpu_lookup_or_add_sparse_buffer(rcs, cs, bo);
   if (index < 0)
            buffer = &cs->sparse_buffers[index];
   buffer->usage |= usage;
               cs->last_added_bo = bo;
   cs->last_added_bo_index = index;
      }
      static bool amdgpu_ib_new_buffer(struct amdgpu_winsys *ws,
               {
      struct pb_buffer *pb;
   uint8_t *mapped;
            /* Always create a buffer that is at least as large as the maximum seen IB
   * size, aligned to a power of two (and multiplied by 4 to reduce internal
   * fragmentation if chaining is not available). Limit to 512k dwords, which
   * is the largest power of two that fits into the size field of the
   * INDIRECT_BUFFER packet.
   */
   if (cs->has_chaining)
         else
            const unsigned min_size = MAX2(ib->max_check_space_size, 8 * 1024 * 4);
            buffer_size = MIN2(buffer_size, max_size);
            /* Use cached GTT for command buffers. Writing to other heaps is very slow on the CPU.
   * The speed of writing to GTT WC is somewhere between no difference and very slow, while
   * VRAM being very slow a lot more often.
   */
   enum radeon_bo_domain domain = RADEON_DOMAIN_GTT;
            if (cs->ip_type == AMD_IP_GFX ||
      cs->ip_type == AMD_IP_COMPUTE ||
   cs->ip_type == AMD_IP_SDMA) {
   /* Avoids hangs with "rendercheck -t cacomposite -f a8r8g8b8" via glamor
   * on Navi 14
   */
               pb = amdgpu_bo_create(ws, buffer_size,
               if (!pb)
            mapped = amdgpu_bo_map(&ws->dummy_ws.base, pb, NULL, PIPE_MAP_WRITE);
   if (!mapped) {
      radeon_bo_reference(&ws->dummy_ws.base, &pb, NULL);
               radeon_bo_reference(&ws->dummy_ws.base, &ib->big_ib_buffer, pb);
            ib->ib_mapped = mapped;
               }
      static bool amdgpu_get_new_ib(struct amdgpu_winsys *ws,
                     {
      /* Small IBs are better than big IBs, because the GPU goes idle quicker
   * and there is less waiting for buffers and fences. Proof:
   *   http://www.phoronix.com/scan.php?page=article&item=mesa-111-si&num=1
   */
   struct drm_amdgpu_cs_chunk_ib *info = &cs->csc->ib[ib->ib_type];
   /* This is the minimum size of a contiguous IB. */
            /* Always allocate at least the size of the biggest cs_check_space call,
   * because precisely the last call might have requested this size.
   */
            if (!cs->has_chaining) {
      ib_size = MAX2(ib_size,
                              rcs->prev_dw = 0;
   rcs->num_prev = 0;
   rcs->current.cdw = 0;
            /* Allocate a new buffer for IBs if the current buffer is all used. */
   if (!ib->big_ib_buffer ||
      ib->used_ib_space + ib_size > ib->big_ib_buffer->size) {
   if (!amdgpu_ib_new_buffer(ws, ib, cs))
               info->va_start = amdgpu_winsys_bo(ib->big_ib_buffer)->va + ib->used_ib_space;
   info->ib_bytes = 0;
   /* ib_bytes is in dwords and the conversion to bytes will be done before
   * the CS ioctl. */
   ib->ptr_ib_size = &info->ib_bytes;
            amdgpu_cs_add_buffer(cs->main.rcs, ib->big_ib_buffer,
                     if (ib->ib_type == IB_MAIN)
            ib_size = ib->big_ib_buffer->size - ib->used_ib_space;
   rcs->current.max_dw = ib_size / 4 - amdgpu_cs_epilog_dws(cs);
   rcs->gpu_address = info->va_start;
      }
      static void amdgpu_set_ib_size(struct radeon_cmdbuf *rcs, struct amdgpu_ib *ib)
   {
      if (ib->ptr_ib_size_inside_ib) {
      *ib->ptr_ib_size = rcs->current.cdw |
            } else {
            }
      static void amdgpu_ib_finalize(struct amdgpu_winsys *ws, struct radeon_cmdbuf *rcs,
         {
      amdgpu_set_ib_size(rcs, ib);
   ib->used_ib_space += rcs->current.cdw * 4;
   ib->used_ib_space = align(ib->used_ib_space, ws->info.ip[ip_type].ib_alignment);
      }
      static bool amdgpu_init_cs_context(struct amdgpu_winsys *ws,
               {
      switch (ip_type) {
   case AMD_IP_SDMA:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_DMA;
         case AMD_IP_UVD:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_UVD;
         case AMD_IP_UVD_ENC:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_UVD_ENC;
         case AMD_IP_VCE:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_VCE;
         case AMD_IP_VCN_DEC:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_VCN_DEC;
         case AMD_IP_VCN_ENC:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_VCN_ENC;
         case AMD_IP_VCN_JPEG:
      cs->ib[IB_MAIN].ip_type = AMDGPU_HW_IP_VCN_JPEG;
         case AMD_IP_COMPUTE:
   case AMD_IP_GFX:
      cs->ib[IB_MAIN].ip_type = ip_type == AMD_IP_GFX ? AMDGPU_HW_IP_GFX :
            /* The kernel shouldn't invalidate L2 and vL1. The proper place for cache
   * invalidation is the beginning of IBs (the previous commit does that),
   * because completion of an IB doesn't care about the state of GPU caches,
   * but the beginning of an IB does. Draw calls from multiple IBs can be
   * executed in parallel, so draw calls from the current IB can finish after
   * the next IB starts drawing, and so the cache flush at the end of IB
   * is always late.
   */
   if (ws->info.drm_minor >= 26) {
      cs->ib[IB_PREAMBLE].flags = AMDGPU_IB_FLAG_TC_WB_NOT_INVALIDATE;
      }
         default:
                  cs->ib[IB_PREAMBLE].flags |= AMDGPU_IB_FLAG_PREAMBLE;
            cs->last_added_bo = NULL;
      }
      static void cleanup_fence_list(struct amdgpu_fence_list *fences)
   {
      for (unsigned i = 0; i < fences->num; i++)
            }
      static void amdgpu_cs_context_cleanup(struct amdgpu_winsys *ws, struct amdgpu_cs_context *cs)
   {
               for (i = 0; i < cs->num_real_buffers; i++) {
         }
   for (i = 0; i < cs->num_slab_buffers; i++) {
         }
   for (i = 0; i < cs->num_sparse_buffers; i++) {
         }
   cleanup_fence_list(&cs->fence_dependencies);
   cleanup_fence_list(&cs->syncobj_dependencies);
            cs->num_real_buffers = 0;
   cs->num_slab_buffers = 0;
   cs->num_sparse_buffers = 0;
   amdgpu_fence_reference(&cs->fence, NULL);
      }
      static void amdgpu_destroy_cs_context(struct amdgpu_winsys *ws, struct amdgpu_cs_context *cs)
   {
      amdgpu_cs_context_cleanup(ws, cs);
   FREE(cs->real_buffers);
   FREE(cs->slab_buffers);
   FREE(cs->sparse_buffers);
   FREE(cs->fence_dependencies.list);
   FREE(cs->syncobj_dependencies.list);
      }
         static bool
   amdgpu_cs_create(struct radeon_cmdbuf *rcs,
                  struct radeon_winsys_ctx *rwctx,
   enum amd_ip_type ip_type,
      {
      struct amdgpu_ctx *ctx = (struct amdgpu_ctx*)rwctx;
            cs = CALLOC_STRUCT(amdgpu_cs);
   if (!cs) {
                           cs->ws = ctx->ws;
   cs->ctx = ctx;
   cs->flush_cs = flush;
   cs->flush_data = flush_ctx;
   cs->ip_type = ip_type;
   cs->noop = ctx->ws->noop_cs;
   cs->has_chaining = ctx->ws->info.gfx_level >= GFX7 &&
            struct amdgpu_cs_fence_info fence_info;
   fence_info.handle = cs->ctx->user_fence_bo;
   fence_info.offset = cs->ip_type * 4;
                     if (!amdgpu_init_cs_context(ctx->ws, &cs->csc1, ip_type)) {
      FREE(cs);
               if (!amdgpu_init_cs_context(ctx->ws, &cs->csc2, ip_type)) {
      amdgpu_destroy_cs_context(ctx->ws, &cs->csc1);
   FREE(cs);
                        /* Set the first submission context as current. */
   rcs->csc = cs->csc = &cs->csc1;
            /* Assign to both amdgpu_cs_context; only csc will use it. */
   cs->csc1.buffer_indices_hashlist = cs->buffer_indices_hashlist;
            cs->csc1.ws = ctx->ws;
            cs->main.rcs = rcs;
            if (!amdgpu_get_new_ib(ctx->ws, rcs, &cs->main, cs)) {
      amdgpu_destroy_cs_context(ctx->ws, &cs->csc2);
   amdgpu_destroy_cs_context(ctx->ws, &cs->csc1);
   FREE(cs);
   rcs->priv = NULL;
               p_atomic_inc(&ctx->ws->num_cs);
      }
      static bool
   amdgpu_cs_setup_preemption(struct radeon_cmdbuf *rcs, const uint32_t *preamble_ib,
         {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
   struct amdgpu_winsys *ws = cs->ws;
   struct amdgpu_cs_context *csc[2] = {&cs->csc1, &cs->csc2};
   unsigned size = align(preamble_num_dw * 4, ws->info.ip[AMD_IP_GFX].ib_alignment);
   struct pb_buffer *preamble_bo;
            /* Create the preamble IB buffer. */
   preamble_bo = amdgpu_bo_create(ws, size, ws->info.ip[AMD_IP_GFX].ib_alignment,
                           if (!preamble_bo)
            map = (uint32_t*)amdgpu_bo_map(&ws->dummy_ws.base, preamble_bo, NULL,
         if (!map) {
      radeon_bo_reference(&ws->dummy_ws.base, &preamble_bo, NULL);
               /* Upload the preamble IB. */
            /* Pad the IB. */
   amdgpu_pad_gfx_compute_ib(ws, cs->ip_type, map, &preamble_num_dw, 0);
            for (unsigned i = 0; i < 2; i++) {
      csc[i]->ib[IB_PREAMBLE].va_start = amdgpu_winsys_bo(preamble_bo)->va;
                        assert(!cs->preamble_ib_bo);
            amdgpu_cs_add_buffer(rcs, cs->preamble_ib_bo,
            }
      static bool amdgpu_cs_validate(struct radeon_cmdbuf *rcs)
   {
         }
      static bool amdgpu_cs_check_space(struct radeon_cmdbuf *rcs, unsigned dw)
   {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
                     /* 125% of the size for IB epilog. */
            if (requested_size > IB_MAX_SUBMIT_DWORDS)
            if (rcs->current.max_dw - rcs->current.cdw >= dw)
            unsigned cs_epilog_dw = amdgpu_cs_epilog_dws(cs);
   unsigned need_byte_size = (dw + cs_epilog_dw) * 4;
   unsigned safe_byte_size = need_byte_size + need_byte_size / 4;
   ib->max_check_space_size = MAX2(ib->max_check_space_size,
                  if (!cs->has_chaining)
            /* Allocate a new chunk */
   if (rcs->num_prev >= rcs->max_prev) {
      unsigned new_max_prev = MAX2(1, 2 * rcs->max_prev);
            new_prev = REALLOC(rcs->prev,
               if (!new_prev)
            rcs->prev = new_prev;
               if (!amdgpu_ib_new_buffer(cs->ws, ib, cs))
            assert(ib->used_ib_space == 0);
            /* This space was originally reserved. */
            /* Pad with NOPs but leave 4 dwords for INDIRECT_BUFFER. */
            radeon_emit(rcs, PKT3(PKT3_INDIRECT_BUFFER, 2, 0));
   radeon_emit(rcs, va);
   radeon_emit(rcs, va >> 32);
            assert((rcs->current.cdw & cs->ws->info.ip[cs->ip_type].ib_pad_dw_mask) == 0);
            amdgpu_set_ib_size(rcs, ib);
   ib->ptr_ib_size = new_ptr_ib_size;
            /* Hook up the new chunk */
   rcs->prev[rcs->num_prev].buf = rcs->current.buf;
   rcs->prev[rcs->num_prev].cdw = rcs->current.cdw;
   rcs->prev[rcs->num_prev].max_dw = rcs->current.cdw; /* no modifications */
            rcs->prev_dw += rcs->current.cdw;
            rcs->current.buf = (uint32_t*)(ib->ib_mapped + ib->used_ib_space);
   rcs->current.max_dw = ib->big_ib_buffer->size / 4 - cs_epilog_dw;
            amdgpu_cs_add_buffer(cs->main.rcs, ib->big_ib_buffer,
               }
      static unsigned amdgpu_cs_get_buffer_list(struct radeon_cmdbuf *rcs,
         {
      struct amdgpu_cs_context *cs = amdgpu_cs(rcs)->csc;
            if (list) {
      for (i = 0; i < cs->num_real_buffers; i++) {
         list[i].bo_size = cs->real_buffers[i].bo->base.size;
   list[i].vm_address = cs->real_buffers[i].bo->va;
      }
      }
      static void add_fence_to_list(struct amdgpu_fence_list *fences,
         {
               if (idx >= fences->max) {
      unsigned size;
            fences->max = idx + increment;
   size = fences->max * sizeof(fences->list[0]);
   fences->list = realloc(fences->list, size);
   /* Clear the newly-allocated elements. */
   memset(fences->list + idx, 0,
      }
      }
      static bool is_noop_fence_dependency(struct amdgpu_cs *acs,
         {
               /* Detect no-op dependencies only when there is only 1 ring,
   * because IBs on one ring are always executed one at a time.
   *
   * We always want no dependency between back-to-back gfx IBs, because
   * we need the parallelism between IBs for good performance.
   */
   if ((acs->ip_type == AMD_IP_GFX ||
      acs->ws->info.ip[acs->ip_type].num_queues == 1) &&
   !amdgpu_fence_is_syncobj(fence) &&
   fence->ctx == acs->ctx &&
   fence->fence.ip_type == cs->ib[IB_MAIN].ip_type)
            }
      static void amdgpu_cs_add_fence_dependency(struct radeon_cmdbuf *rws,
               {
      struct amdgpu_cs *acs = amdgpu_cs(rws);
   struct amdgpu_cs_context *cs = acs->csc;
                     if (is_noop_fence_dependency(acs, fence))
            if (amdgpu_fence_is_syncobj(fence))
         else
      }
      static void amdgpu_add_bo_fence_dependencies(struct amdgpu_cs *acs,
               {
      struct amdgpu_winsys_bo *bo = buffer->bo;
   unsigned new_num_fences = 0;
            for (unsigned j = 0; j < num_fences; ++j) {
               if (is_noop_fence_dependency(acs, bo_fence))
            amdgpu_fence_reference(&bo->fences[new_num_fences], bo->fences[j]);
            if (!(buffer->usage & RADEON_USAGE_SYNCHRONIZED))
                        for (unsigned j = new_num_fences; j < num_fences; ++j)
               }
      /* Add the given list of fences to the buffer's fence list.
   *
   * Must be called with the winsys bo_fence_lock held.
   */
   void amdgpu_add_fences(struct amdgpu_winsys_bo *bo,
               {
      if (bo->num_fences + num_fences > bo->max_fences) {
      unsigned new_max_fences = MAX2(bo->num_fences + num_fences, bo->max_fences * 2);
   struct pipe_fence_handle **new_fences =
      REALLOC(bo->fences,
            if (likely(new_fences && new_max_fences < UINT16_MAX)) {
      bo->fences = new_fences;
      } else {
               fprintf(stderr, new_fences ? "amdgpu_add_fences: too many fences, dropping some\n"
                                                drop = bo->num_fences + num_fences - bo->max_fences;
   num_fences -= drop;
                           for (unsigned i = 0; i < num_fences; ++i) {
      bo->fences[bo_num_fences] = NULL;
   amdgpu_fence_reference(&bo->fences[bo_num_fences], fences[i]);
      }
      }
      static void amdgpu_inc_bo_num_active_ioctls(unsigned num_buffers,
         {
      for (unsigned i = 0; i < num_buffers; i++)
      }
      static void amdgpu_add_fence_dependencies_bo_list(struct amdgpu_cs *acs,
                           {
      for (unsigned i = 0; i < num_buffers; i++) {
      struct amdgpu_cs_buffer *buffer = &buffers[i];
            amdgpu_add_bo_fence_dependencies(acs, cs, buffer);
         }
      /* Since the kernel driver doesn't synchronize execution between different
   * rings automatically, we have to add fence dependencies manually.
   */
   static void amdgpu_add_fence_dependencies_bo_lists(struct amdgpu_cs *acs,
         {
      amdgpu_add_fence_dependencies_bo_list(acs, cs, cs->fence, cs->num_real_buffers, cs->real_buffers);
   amdgpu_add_fence_dependencies_bo_list(acs, cs, cs->fence, cs->num_slab_buffers, cs->slab_buffers);
      }
      static void amdgpu_cs_add_syncobj_signal(struct radeon_cmdbuf *rws,
         {
      struct amdgpu_cs *acs = amdgpu_cs(rws);
                        }
      /* Add backing of sparse buffers to the buffer list.
   *
   * This is done late, during submission, to keep the buffer list short before
   * submit, and to avoid managing fences for the backing buffers.
   */
   static bool amdgpu_add_sparse_backing_buffers(struct amdgpu_cs_context *cs)
   {
      for (unsigned i = 0; i < cs->num_sparse_buffers; ++i) {
      struct amdgpu_cs_buffer *buffer = &cs->sparse_buffers[i];
                     list_for_each_entry(struct amdgpu_sparse_backing, backing, &bo->u.sparse.backing, list) {
      /* We can directly add the buffer here, because we know that each
   * backing buffer occurs only once.
   */
   int idx = amdgpu_do_add_real_buffer(cs, backing->bo);
   if (idx < 0) {
      fprintf(stderr, "%s: failed to add buffer\n", __func__);
   simple_mtx_unlock(&bo->lock);
                                          }
      static void amdgpu_cs_submit_ib(void *job, void *gdata, int thread_index)
   {
      struct amdgpu_cs *acs = (struct amdgpu_cs*)job;
   struct amdgpu_winsys *ws = acs->ws;
   struct amdgpu_cs_context *cs = acs->cst;
   int i, r;
   uint32_t bo_list = 0;
   uint64_t seq_no = 0;
   bool has_user_fence = amdgpu_cs_has_user_fence(cs);
   bool use_bo_list_create = ws->info.drm_minor < 27;
   struct drm_amdgpu_bo_list_in bo_list_in;
            simple_mtx_lock(&ws->bo_fence_lock);
   amdgpu_add_fence_dependencies_bo_lists(acs, cs);
         #if DEBUG
      /* Prepare the buffer list. */
   if (ws->debug_all_bos) {
      /* The buffer list contains all buffers. This is a slow path that
   * ensures that no buffer is missing in the BO list.
   */
   unsigned num_handles = 0;
   struct drm_amdgpu_bo_list_entry *list =
                  simple_mtx_lock(&ws->global_bo_list_lock);
   LIST_FOR_EACH_ENTRY(bo, &ws->global_bo_list, u.real.global_list_item) {
      list[num_handles].bo_handle = bo->u.real.kms_handle;
   list[num_handles].bo_priority = 0;
               r = amdgpu_bo_list_create_raw(ws->dev, ws->num_buffers, list, &bo_list);
   simple_mtx_unlock(&ws->global_bo_list_lock);
   if (r) {
      fprintf(stderr, "amdgpu: buffer list creation failed (%d)\n", r);
            #endif
      {
      if (!amdgpu_add_sparse_backing_buffers(cs)) {
      fprintf(stderr, "amdgpu: amdgpu_add_sparse_backing_buffers failed\n");
   r = -ENOMEM;
               struct drm_amdgpu_bo_list_entry *list =
            unsigned num_handles = 0;
   for (i = 0; i < cs->num_real_buffers; ++i) {
               list[num_handles].bo_handle = buffer->bo->u.real.kms_handle;
   list[num_handles].bo_priority =
                     if (use_bo_list_create) {
      /* Legacy path creating the buffer list handle and passing it to the CS ioctl. */
   r = amdgpu_bo_list_create_raw(ws->dev, num_handles, list, &bo_list);
   if (r) {
      fprintf(stderr, "amdgpu: buffer list creation failed (%d)\n", r);
         } else {
      /* Standard path passing the buffer list via the CS ioctl. */
   bo_list_in.operation = ~0;
   bo_list_in.list_handle = ~0;
   bo_list_in.bo_number = num_handles;
   bo_list_in.bo_info_size = sizeof(struct drm_amdgpu_bo_list_entry);
                  if (acs->ip_type == AMD_IP_GFX)
            struct drm_amdgpu_cs_chunk chunks[8];
            /* BO list */
   if (!use_bo_list_create) {
      chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_BO_HANDLES;
   chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_bo_list_in) / 4;
   chunks[num_chunks].chunk_data = (uintptr_t)&bo_list_in;
               /* Fence dependencies. */
   unsigned num_dependencies = cs->fence_dependencies.num;
   if (num_dependencies) {
      struct drm_amdgpu_cs_chunk_dep *dep_chunk =
            for (unsigned i = 0; i < num_dependencies; i++) {
                     assert(util_queue_fence_is_signalled(&fence->submitted));
               chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_DEPENDENCIES;
   chunks[num_chunks].length_dw = sizeof(dep_chunk[0]) / 4 * num_dependencies;
   chunks[num_chunks].chunk_data = (uintptr_t)dep_chunk;
               /* Syncobj dependencies. */
   unsigned num_syncobj_dependencies = cs->syncobj_dependencies.num;
   if (num_syncobj_dependencies) {
      struct drm_amdgpu_cs_chunk_sem *sem_chunk =
            for (unsigned i = 0; i < num_syncobj_dependencies; i++) {
                                    assert(util_queue_fence_is_signalled(&fence->submitted));
               chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_SYNCOBJ_IN;
   chunks[num_chunks].length_dw = sizeof(sem_chunk[0]) / 4 * num_syncobj_dependencies;
   chunks[num_chunks].chunk_data = (uintptr_t)sem_chunk;
               /* Syncobj signals. */
   unsigned num_syncobj_to_signal = cs->syncobj_to_signal.num;
   if (num_syncobj_to_signal) {
      struct drm_amdgpu_cs_chunk_sem *sem_chunk =
            for (unsigned i = 0; i < num_syncobj_to_signal; i++) {
                     assert(amdgpu_fence_is_syncobj(fence));
               chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_SYNCOBJ_OUT;
   chunks[num_chunks].length_dw = sizeof(sem_chunk[0]) / 4
         chunks[num_chunks].chunk_data = (uintptr_t)sem_chunk;
               if (ws->info.has_fw_based_shadowing && acs->mcbp_fw_shadow_chunk.shadow_va) {
      chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_CP_GFX_SHADOW;
   chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_cs_chunk_cp_gfx_shadow) / 4;
   chunks[num_chunks].chunk_data = (uintptr_t)&acs->mcbp_fw_shadow_chunk;
               /* Fence */
   if (has_user_fence) {
      chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_FENCE;
   chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_cs_chunk_fence) / 4;
   chunks[num_chunks].chunk_data = (uintptr_t)&acs->fence_chunk;
               /* IB */
   if (cs->ib[IB_PREAMBLE].ib_bytes) {
      chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_IB;
   chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_cs_chunk_ib) / 4;
   chunks[num_chunks].chunk_data = (uintptr_t)&cs->ib[IB_PREAMBLE];
               /* IB */
   cs->ib[IB_MAIN].ib_bytes *= 4; /* Convert from dwords to bytes. */
   chunks[num_chunks].chunk_id = AMDGPU_CHUNK_ID_IB;
   chunks[num_chunks].length_dw = sizeof(struct drm_amdgpu_cs_chunk_ib) / 4;
   chunks[num_chunks].chunk_data = (uintptr_t)&cs->ib[IB_MAIN];
            if (cs->secure) {
      cs->ib[IB_PREAMBLE].flags |= AMDGPU_IB_FLAGS_SECURE;
      } else {
      cs->ib[IB_PREAMBLE].flags &= ~AMDGPU_IB_FLAGS_SECURE;
                        if (noop && acs->ip_type == AMD_IP_GFX) {
      /* Reduce the IB size and fill it with NOP to make it like an empty IB. */
   unsigned noop_dw_size = ws->info.ip[AMD_IP_GFX].ib_pad_dw_mask + 1;
            cs->ib_main_addr[0] = PKT3(PKT3_NOP, noop_dw_size - 2, 0);
   cs->ib[IB_MAIN].ib_bytes = noop_dw_size * 4;
                        if (unlikely(acs->ctx->sw_status != PIPE_NO_RESET)) {
         } else if (unlikely(noop)) {
         } else {
      /* Submit the command buffer.
   *
   * The kernel returns -ENOMEM with many parallel processes using GDS such as test suites
   * quite often, but it eventually succeeds after enough attempts. This happens frequently
   * with dEQP using NGG streamout.
   */
            do {
      /* Wait 1 ms and try again. */
                  r = amdgpu_cs_submit_raw2(ws->dev, acs->ctx->ctx, bo_list,
               if (!r) {
                     /* Need to reserve 4 QWORD for user fence:
   *   QWORD[0]: completed fence
   *   QWORD[1]: preempted fence
   *   QWORD[2]: reset fence
   *   QWORD[3]: preempted then reset
   */
   if (has_user_fence)
                        /* Cleanup. */
   if (bo_list)
         cleanup:
      if (unlikely(r)) {
      amdgpu_ctx_set_sw_reset_status((struct radeon_winsys_ctx*)acs->ctx,
                     /* If there was an error, signal the fence, because it won't be signalled
   * by the hardware. */
   if (r || noop)
            if (unlikely(ws->info.has_fw_based_shadowing && acs->mcbp_fw_shadow_chunk.flags && r == 0))
                     /* Only decrement num_active_ioctls for those buffers where we incremented it. */
   for (i = 0; i < initial_num_real_buffers; i++)
         for (i = 0; i < cs->num_slab_buffers; i++)
         for (i = 0; i < cs->num_sparse_buffers; i++)
               }
      /* Make sure the previous submission is completed. */
   void amdgpu_cs_sync_flush(struct radeon_cmdbuf *rcs)
   {
               /* Wait for any pending ioctl of this CS to complete. */
      }
      static int amdgpu_cs_flush(struct radeon_cmdbuf *rcs,
               {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
   struct amdgpu_winsys *ws = cs->ws;
   int error_code = 0;
                     /* Pad the IB according to the mask. */
   switch (cs->ip_type) {
   case AMD_IP_SDMA:
      if (ws->info.gfx_level <= GFX6) {
      while (rcs->current.cdw & ib_pad_dw_mask)
      } else {
      while (rcs->current.cdw & ib_pad_dw_mask)
      }
      case AMD_IP_GFX:
   case AMD_IP_COMPUTE:
      amdgpu_pad_gfx_compute_ib(ws, cs->ip_type, rcs->current.buf, &rcs->current.cdw, 0);
   if (cs->ip_type == AMD_IP_GFX)
            case AMD_IP_UVD:
   case AMD_IP_UVD_ENC:
      while (rcs->current.cdw & ib_pad_dw_mask)
            case AMD_IP_VCN_JPEG:
      if (rcs->current.cdw % 2)
         while (rcs->current.cdw & ib_pad_dw_mask) {
      radeon_emit(rcs, 0x60000000); /* nop packet */
      }
      case AMD_IP_VCN_DEC:
      while (rcs->current.cdw & ib_pad_dw_mask)
            default:
                  if (rcs->current.cdw > rcs->current.max_dw) {
                  /* If the CS is not empty or overflowed.... */
   if (likely(radeon_emitted(rcs, 0) &&
      rcs->current.cdw <= rcs->current.max_dw &&
   !(flags & RADEON_FLUSH_NOOP))) {
            /* Set IB sizes. */
            /* Create a fence. */
   amdgpu_fence_reference(&cur->fence, NULL);
   if (cs->next_fence) {
      /* just move the reference */
   cur->fence = cs->next_fence;
      } else {
      cur->fence = amdgpu_fence_create(cs->ctx,
      }
   if (fence)
            amdgpu_inc_bo_num_active_ioctls(cur->num_real_buffers, cur->real_buffers);
   amdgpu_inc_bo_num_active_ioctls(cur->num_slab_buffers, cur->slab_buffers);
                     /* Swap command streams. "cst" is going to be submitted. */
   rcs->csc = cs->csc = cs->cst;
            /* Submit. */
   util_queue_add_job(&ws->cs_queue, cs, &cs->flush_completed,
            if (flags & RADEON_FLUSH_TOGGLE_SECURE_SUBMISSION)
         else
            if (!(flags & PIPE_FLUSH_ASYNC)) {
      amdgpu_cs_sync_flush(rcs);
         } else {
      if (flags & RADEON_FLUSH_TOGGLE_SECURE_SUBMISSION)
                                       if (cs->preamble_ib_bo) {
      amdgpu_cs_add_buffer(rcs, cs->preamble_ib_bo,
               rcs->used_gart_kb = 0;
            if (cs->ip_type == AMD_IP_GFX)
         else if (cs->ip_type == AMD_IP_SDMA)
               }
      static void amdgpu_cs_destroy(struct radeon_cmdbuf *rcs)
   {
               if (!cs)
            amdgpu_cs_sync_flush(rcs);
   util_queue_fence_destroy(&cs->flush_completed);
   p_atomic_dec(&cs->ws->num_cs);
   radeon_bo_reference(&cs->ws->dummy_ws.base, &cs->preamble_ib_bo, NULL);
   radeon_bo_reference(&cs->ws->dummy_ws.base, &cs->main.big_ib_buffer, NULL);
   FREE(rcs->prev);
   amdgpu_destroy_cs_context(cs->ws, &cs->csc1);
   amdgpu_destroy_cs_context(cs->ws, &cs->csc2);
   amdgpu_fence_reference(&cs->next_fence, NULL);
      }
      static bool amdgpu_bo_is_referenced(struct radeon_cmdbuf *rcs,
               {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
               }
      static void amdgpu_cs_set_mcbp_reg_shadowing_va(struct radeon_cmdbuf *rcs,uint64_t regs_va,
         {
      struct amdgpu_cs *cs = amdgpu_cs(rcs);
   cs->mcbp_fw_shadow_chunk.shadow_va = regs_va;
   cs->mcbp_fw_shadow_chunk.csa_va = csa_va;
   cs->mcbp_fw_shadow_chunk.gds_va = 0;
      }
      void amdgpu_cs_init_functions(struct amdgpu_screen_winsys *ws)
   {
      ws->base.ctx_create = amdgpu_ctx_create;
   ws->base.ctx_destroy = amdgpu_ctx_destroy;
   ws->base.ctx_set_sw_reset_status = amdgpu_ctx_set_sw_reset_status;
   ws->base.ctx_query_reset_status = amdgpu_ctx_query_reset_status;
   ws->base.cs_create = amdgpu_cs_create;
   ws->base.cs_setup_preemption = amdgpu_cs_setup_preemption;
   ws->base.cs_destroy = amdgpu_cs_destroy;
   ws->base.cs_add_buffer = amdgpu_cs_add_buffer;
   ws->base.cs_validate = amdgpu_cs_validate;
   ws->base.cs_check_space = amdgpu_cs_check_space;
   ws->base.cs_get_buffer_list = amdgpu_cs_get_buffer_list;
   ws->base.cs_flush = amdgpu_cs_flush;
   ws->base.cs_get_next_fence = amdgpu_cs_get_next_fence;
   ws->base.cs_is_buffer_referenced = amdgpu_bo_is_referenced;
   ws->base.cs_sync_flush = amdgpu_cs_sync_flush;
   ws->base.cs_add_fence_dependency = amdgpu_cs_add_fence_dependency;
   ws->base.cs_add_syncobj_signal = amdgpu_cs_add_syncobj_signal;
   ws->base.fence_wait = amdgpu_fence_wait_rel_timeout;
   ws->base.fence_reference = amdgpu_fence_reference;
   ws->base.fence_import_syncobj = amdgpu_fence_import_syncobj;
   ws->base.fence_import_sync_file = amdgpu_fence_import_sync_file;
   ws->base.fence_export_sync_file = amdgpu_fence_export_sync_file;
            if (ws->aws->info.has_fw_based_shadowing)
      }
