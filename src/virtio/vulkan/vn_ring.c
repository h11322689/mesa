   /*
   * Copyright 2021 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vn_ring.h"
      #include "vn_cs.h"
   #include "vn_renderer.h"
      static uint32_t
   vn_ring_load_head(const struct vn_ring *ring)
   {
      /* the renderer is expected to store the head with memory_order_release,
   * forming a release-acquire ordering
   */
      }
      static void
   vn_ring_store_tail(struct vn_ring *ring)
   {
      /* the renderer is expected to load the tail with memory_order_acquire,
   * forming a release-acquire ordering
   */
   return atomic_store_explicit(ring->shared.tail, ring->cur,
      }
      uint32_t
   vn_ring_load_status(const struct vn_ring *ring)
   {
      /* must be called and ordered after vn_ring_store_tail for idle status */
      }
      void
   vn_ring_unset_status_bits(struct vn_ring *ring, uint32_t mask)
   {
      atomic_fetch_and_explicit(ring->shared.status, ~mask,
      }
      static void
   vn_ring_write_buffer(struct vn_ring *ring, const void *data, uint32_t size)
   {
               const uint32_t offset = ring->cur & ring->buffer_mask;
   if (offset + size <= ring->buffer_size) {
         } else {
      const uint32_t s = ring->buffer_size - offset;
   memcpy(ring->shared.buffer + offset, data, s);
                  }
      static bool
   vn_ring_ge_seqno(const struct vn_ring *ring, uint32_t a, uint32_t b)
   {
      /* this can return false negative when not called fast enough (e.g., when
   * called once every couple hours), but following calls with larger a's
   * will correct itself
   *
   * TODO use real seqnos?
   */
   if (a >= b)
         else
      }
      static void
   vn_ring_retire_submits(struct vn_ring *ring, uint32_t seqno)
   {
      list_for_each_entry_safe(struct vn_ring_submit, submit, &ring->submits,
            if (!vn_ring_ge_seqno(ring, seqno, submit->seqno))
            for (uint32_t i = 0; i < submit->shmem_count; i++)
            list_del(&submit->head);
         }
      static uint32_t
   vn_ring_wait_seqno(struct vn_ring *ring, uint32_t seqno)
   {
      /* A renderer wait incurs several hops and the renderer might poll
   * repeatedly anyway.  Let's just poll here.
   */
   struct vn_relax_state relax_state = vn_relax_init(ring, "ring seqno");
   do {
      const uint32_t head = vn_ring_load_head(ring);
   if (vn_ring_ge_seqno(ring, head, seqno)) {
      vn_relax_fini(&relax_state);
      }
         }
      static bool
   vn_ring_has_space(const struct vn_ring *ring,
               {
      const uint32_t head = vn_ring_load_head(ring);
   if (likely(ring->cur + size - head <= ring->buffer_size)) {
      *out_head = head;
                  }
      static uint32_t
   vn_ring_wait_space(struct vn_ring *ring, uint32_t size)
   {
               uint32_t head;
   if (likely(vn_ring_has_space(ring, size, &head)))
            {
               /* see the reasoning in vn_ring_wait_seqno */
   struct vn_relax_state relax_state = vn_relax_init(ring, "ring space");
   do {
      vn_relax(&relax_state);
   if (vn_ring_has_space(ring, size, &head)) {
      vn_relax_fini(&relax_state);
               }
      void
   vn_ring_get_layout(size_t buf_size,
               {
      /* this can be changed/extended quite freely */
   struct layout {
      alignas(64) uint32_t head;
   alignas(64) uint32_t tail;
                                 layout->head_offset = offsetof(struct layout, head);
   layout->tail_offset = offsetof(struct layout, tail);
            layout->buffer_offset = offsetof(struct layout, buffer);
            layout->extra_offset = layout->buffer_offset + layout->buffer_size;
               }
      void
   vn_ring_init(struct vn_ring *ring,
               struct vn_renderer *renderer,
   {
      memset(ring, 0, sizeof(*ring));
                     assert(layout->buffer_size &&
         ring->buffer_size = layout->buffer_size;
            ring->shared.head = shared + layout->head_offset;
   ring->shared.tail = shared + layout->tail_offset;
   ring->shared.status = shared + layout->status_offset;
   ring->shared.buffer = shared + layout->buffer_offset;
            list_inithead(&ring->submits);
      }
      void
   vn_ring_fini(struct vn_ring *ring)
   {
      vn_ring_retire_submits(ring, ring->cur);
            list_for_each_entry_safe(struct vn_ring_submit, submit,
                  if (ring->monitor.report_period_us)
      }
      struct vn_ring_submit *
   vn_ring_get_submit(struct vn_ring *ring, uint32_t shmem_count)
   {
      const uint32_t min_shmem_count = 2;
            /* TODO this could be simplified if we could omit shmem_count */
   if (shmem_count <= min_shmem_count &&
      !list_is_empty(&ring->free_submits)) {
   submit =
            } else {
      shmem_count = MAX2(shmem_count, min_shmem_count);
   submit =
                  }
      bool
   vn_ring_submit(struct vn_ring *ring,
                     {
      /* write cs to the ring */
            /* avoid -Wmaybe-unitialized */
            for (uint32_t i = 0; i < cs->buffer_count; i++) {
      const struct vn_cs_encoder_buffer *buf = &cs->buffers[i];
   cur_seqno = vn_ring_wait_space(ring, buf->committed_size);
               vn_ring_store_tail(ring);
   const VkRingStatusFlagsMESA status = vn_ring_load_status(ring);
   if (status & VK_RING_STATUS_FATAL_BIT_MESA) {
      vn_log(NULL, "vn_ring_submit abort on fatal");
                        submit->seqno = ring->cur;
                     /* notify renderer to wake up ring if idle */
      }
      /**
   * This is thread-safe.
   */
   void
   vn_ring_wait(struct vn_ring *ring, uint32_t seqno)
   {
         }
