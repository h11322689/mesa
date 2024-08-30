   /*
   * Copyright 2019 Google LLC
   * SPDX-License-Identifier: MIT
   */
      #include "vn_cs.h"
      #include "vn_instance.h"
   #include "vn_renderer.h"
      struct vn_cs_renderer_protocol_info _vn_cs_renderer_protocol_info = {
         };
      static void
   vn_cs_renderer_protocol_info_init_once(struct vn_instance *instance)
   {
      const struct vn_renderer_info *renderer_info = &instance->renderer->info;
   /* assume renderer protocol supports all extensions if bit 0 is not set */
   const bool support_all_exts =
                     STATIC_ASSERT(sizeof(renderer_info->vk_extension_mask) >=
            for (uint32_t i = 1; i <= VN_INFO_EXTENSION_MAX_NUMBER; i++) {
      /* use protocl helper to ensure mask decoding matches encoding */
   if (support_all_exts ||
      vn_info_extension_mask_test(renderer_info->vk_extension_mask, i))
      }
      void
   vn_cs_renderer_protocol_info_init(struct vn_instance *instance)
   {
      simple_mtx_lock(&_vn_cs_renderer_protocol_info.mutex);
   if (_vn_cs_renderer_protocol_info.init_once) {
      simple_mtx_unlock(&_vn_cs_renderer_protocol_info.mutex);
                        _vn_cs_renderer_protocol_info.init_once = true;
      }
      static void
   vn_cs_encoder_sanity_check(struct vn_cs_encoder *enc)
   {
               size_t total_committed_size = 0;
   for (uint32_t i = 0; i < enc->buffer_count; i++)
                  if (enc->buffer_count) {
      const struct vn_cs_encoder_buffer *cur_buf =
         assert(cur_buf->base <= enc->cur && enc->cur <= enc->end &&
         if (cur_buf->committed_size)
      } else {
      assert(!enc->current_buffer_size);
         }
      static void
   vn_cs_encoder_add_buffer(struct vn_cs_encoder *enc,
                           {
      /* add a buffer and make it current */
   assert(enc->buffer_count < enc->buffer_max);
   struct vn_cs_encoder_buffer *cur_buf = &enc->buffers[enc->buffer_count++];
   /* shmem ownership transferred */
   cur_buf->shmem = shmem;
   cur_buf->offset = offset;
   cur_buf->base = base;
            /* update the write pointer */
   enc->cur = base;
      }
      static void
   vn_cs_encoder_commit_buffer(struct vn_cs_encoder *enc)
   {
      assert(enc->buffer_count);
   struct vn_cs_encoder_buffer *cur_buf =
         const size_t written_size = enc->cur - cur_buf->base;
   if (cur_buf->committed_size) {
         } else {
      cur_buf->committed_size = written_size;
         }
      static void
   vn_cs_encoder_gc_buffers(struct vn_cs_encoder *enc)
   {
      /* when the shmem pool is used, no need to cache the shmem in cs */
   if (enc->storage_type == VN_CS_ENCODER_STORAGE_SHMEM_POOL) {
      for (uint32_t i = 0; i < enc->buffer_count; i++) {
      vn_renderer_shmem_unref(enc->instance->renderer,
               enc->buffer_count = 0;
   enc->total_committed_size = 0;
            enc->cur = NULL;
                        /* free all but the current buffer */
   assert(enc->buffer_count);
   struct vn_cs_encoder_buffer *cur_buf =
         for (uint32_t i = 0; i < enc->buffer_count - 1; i++)
            /* move the current buffer to the beginning, skipping the used part */
   const size_t used = cur_buf->offset + cur_buf->committed_size;
   enc->buffer_count = 0;
   vn_cs_encoder_add_buffer(enc, cur_buf->shmem, used,
                     }
      void
   vn_cs_encoder_init(struct vn_cs_encoder *enc,
                     {
      /* VN_CS_ENCODER_INITIALIZER* should be used instead */
            memset(enc, 0, sizeof(*enc));
   enc->instance = instance;
   enc->storage_type = storage_type;
      }
      void
   vn_cs_encoder_fini(struct vn_cs_encoder *enc)
   {
      if (unlikely(enc->storage_type == VN_CS_ENCODER_STORAGE_POINTER))
            for (uint32_t i = 0; i < enc->buffer_count; i++)
         if (enc->buffers)
      }
      /**
   * Reset a cs for reuse.
   */
   void
   vn_cs_encoder_reset(struct vn_cs_encoder *enc)
   {
      /* enc->error is sticky */
   if (likely(enc->buffer_count))
      }
      static uint32_t
   next_array_size(uint32_t cur_size, uint32_t min_size)
   {
      const uint32_t next_size = cur_size ? cur_size * 2 : min_size;
      }
      static size_t
   next_buffer_size(size_t cur_size, size_t min_size, size_t need)
   {
      size_t next_size = cur_size ? cur_size * 2 : min_size;
   while (next_size < need) {
      next_size *= 2;
   if (!next_size)
      }
      }
      static bool
   vn_cs_encoder_grow_buffer_array(struct vn_cs_encoder *enc)
   {
      const uint32_t buf_max = next_array_size(enc->buffer_max, 4);
   if (!buf_max)
            void *bufs = realloc(enc->buffers, sizeof(*enc->buffers) * buf_max);
   if (!bufs)
            enc->buffers = bufs;
               }
      /**
   * Add a new vn_cs_encoder_buffer to a cs.
   */
   bool
   vn_cs_encoder_reserve_internal(struct vn_cs_encoder *enc, size_t size)
   {
      VN_TRACE_FUNC();
   if (unlikely(enc->storage_type == VN_CS_ENCODER_STORAGE_POINTER))
            if (enc->buffer_count >= enc->buffer_max) {
      if (!vn_cs_encoder_grow_buffer_array(enc))
                     size_t buf_size = 0;
   if (likely(enc->buffer_count)) {
               if (enc->storage_type == VN_CS_ENCODER_STORAGE_SHMEM_ARRAY) {
      /* if the current buffer is reused from the last vn_cs_encoder_reset
   * (i.e., offset != 0), do not double the size
   *
   * TODO better strategy to grow buffer size
   */
   const struct vn_cs_encoder_buffer *cur_buf =
         if (cur_buf->offset)
                  if (!buf_size) {
      /* double the size */
   buf_size = next_buffer_size(enc->current_buffer_size,
         if (!buf_size)
               struct vn_renderer_shmem *shmem;
   size_t buf_offset;
   if (enc->storage_type == VN_CS_ENCODER_STORAGE_SHMEM_ARRAY) {
      shmem = vn_renderer_shmem_create(enc->instance->renderer, buf_size);
      } else {
      assert(enc->storage_type == VN_CS_ENCODER_STORAGE_SHMEM_POOL);
   shmem =
      }
   if (!shmem)
            vn_cs_encoder_add_buffer(enc, shmem, buf_offset,
                              }
      /*
   * Commit written data.
   */
   void
   vn_cs_encoder_commit(struct vn_cs_encoder *enc)
   {
      if (likely(enc->buffer_count)) {
               /* trigger the slow path on next vn_cs_encoder_reserve */
                  }
