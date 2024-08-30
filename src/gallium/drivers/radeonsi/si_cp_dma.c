   /*
   * Copyright 2013 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "sid.h"
   #include "si_build_pm4.h"
      /* Set this if you want the ME to wait until CP DMA is done.
   * It should be set on the last CP DMA packet. */
   #define CP_DMA_SYNC (1 << 0)
      /* Set this if the source data was used as a destination in a previous CP DMA
   * packet. It's for preventing a read-after-write (RAW) hazard between two
   * CP DMA packets. */
   #define CP_DMA_RAW_WAIT    (1 << 1)
   #define CP_DMA_DST_IS_GDS  (1 << 2)
   #define CP_DMA_CLEAR       (1 << 3)
   #define CP_DMA_PFP_SYNC_ME (1 << 4)
   #define CP_DMA_SRC_IS_GDS  (1 << 5)
      /* The max number of bytes that can be copied per packet. */
   static inline unsigned cp_dma_max_byte_count(struct si_context *sctx)
   {
      unsigned max =
      sctx->gfx_level >= GFX11 ? 32767 :
         /* make it aligned for optimal performance */
      }
      /* should cp dma skip the hole in sparse bo */
   static inline bool cp_dma_sparse_wa(struct si_context *sctx, struct si_resource *sdst)
   {
      if ((sctx->gfx_level == GFX9) && sdst && (sdst->flags & RADEON_FLAG_SPARSE))
               }
      /* Emit a CP DMA packet to do a copy from one buffer to another, or to clear
   * a buffer. The size must fit in bits [20:0]. If CP_DMA_CLEAR is set, src_va is a 32-bit
   * clear value.
   */
   static void si_emit_cp_dma(struct si_context *sctx, struct radeon_cmdbuf *cs, uint64_t dst_va,
               {
               assert(size <= cp_dma_max_byte_count(sctx));
            if (sctx->gfx_level >= GFX9)
         else
            /* Sync flags. */
   if (flags & CP_DMA_SYNC)
            if (flags & CP_DMA_RAW_WAIT)
            /* Src and dst flags. */
   if (sctx->gfx_level >= GFX9 && !(flags & CP_DMA_CLEAR) && src_va == dst_va) {
         } else if (flags & CP_DMA_DST_IS_GDS) {
      header |= S_411_DST_SEL(V_411_GDS);
   /* GDS increments the address, not CP. */
      } else if (sctx->gfx_level >= GFX7 && cache_policy != L2_BYPASS) {
      header |=
               if (flags & CP_DMA_CLEAR) {
         } else if (flags & CP_DMA_SRC_IS_GDS) {
      header |= S_411_SRC_SEL(V_411_GDS);
   /* Both of these are required for GDS. It does increment the address. */
      } else if (sctx->gfx_level >= GFX7 && cache_policy != L2_BYPASS) {
      header |=
                        if (sctx->gfx_level >= GFX7) {
      radeon_emit(PKT3(PKT3_DMA_DATA, 5, 0));
   radeon_emit(header);
   radeon_emit(src_va);       /* SRC_ADDR_LO [31:0] */
   radeon_emit(src_va >> 32); /* SRC_ADDR_HI [31:0] */
   radeon_emit(dst_va);       /* DST_ADDR_LO [31:0] */
   radeon_emit(dst_va >> 32); /* DST_ADDR_HI [31:0] */
      } else {
               radeon_emit(PKT3(PKT3_CP_DMA, 4, 0));
   radeon_emit(src_va);                  /* SRC_ADDR_LO [31:0] */
   radeon_emit(header);                  /* SRC_ADDR_HI [15:0] + flags. */
   radeon_emit(dst_va);                  /* DST_ADDR_LO [31:0] */
   radeon_emit((dst_va >> 32) & 0xffff); /* DST_ADDR_HI [15:0] */
               /* CP DMA is executed in ME, but index buffers are read by PFP.
   * This ensures that ME (CP DMA) is idle before PFP starts fetching
   * indices. If we wanted to execute CP DMA in PFP, this packet
   * should precede it.
   */
   if (sctx->has_graphics && flags & CP_DMA_PFP_SYNC_ME) {
      radeon_emit(PKT3(PKT3_PFP_SYNC_ME, 0, 0));
      }
      }
      void si_cp_dma_wait_for_idle(struct si_context *sctx, struct radeon_cmdbuf *cs)
   {
      /* Issue a dummy DMA that copies zero bytes.
   *
   * The DMA engine will see that there's no work to do and skip this
   * DMA request, however, the CP will see the sync flag and still wait
   * for all DMAs to complete.
   */
      }
      static void si_cp_dma_prepare(struct si_context *sctx, struct pipe_resource *dst,
                     {
      if (!(user_flags & SI_OP_CPDMA_SKIP_CHECK_CS_SPACE))
            /* This must be done after need_cs_space. */
   if (dst)
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(dst),
      if (src)
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(src),
         /* Flush the caches for the first copy only.
   * Also wait for the previous CP DMA operations.
   */
   if (*is_first && sctx->flags)
            if (user_flags & SI_OP_SYNC_CPDMA_BEFORE && *is_first && !(*packet_flags & CP_DMA_CLEAR))
                     /* Do the synchronization after the last dma, so that all data
   * is written to memory.
   */
   if (user_flags & SI_OP_SYNC_AFTER && byte_count == remaining_size) {
               if (coher == SI_COHERENCY_SHADER)
         }
      void si_cp_dma_clear_buffer(struct si_context *sctx, struct radeon_cmdbuf *cs,
                     {
      struct si_resource *sdst = si_resource(dst);
   uint64_t va = (sdst ? sdst->gpu_address : 0) + offset;
                     if (user_flags & SI_OP_SYNC_GE_BEFORE)
            if (user_flags & SI_OP_SYNC_CS_BEFORE)
            if (user_flags & SI_OP_SYNC_PS_BEFORE)
            /* Mark the buffer range of destination as valid (initialized),
   * so that transfer_map knows it should wait for the GPU when mapping
   * that range. */
   if (sdst) {
               if (!(user_flags & SI_OP_SKIP_CACHE_INV_BEFORE))
               if (sctx->flags)
            while (size) {
      unsigned byte_count = MIN2(size, cp_dma_max_byte_count(sctx));
            if (cp_dma_sparse_wa(sctx,sdst)) {
      unsigned skip_count =
      sctx->ws->buffer_find_next_committed_memory(sdst->buf,
      va += skip_count;
               if (!byte_count)
            si_cp_dma_prepare(sctx, dst, NULL, byte_count, size, user_flags, coher, &is_first,
            /* Emit the clear packet. */
            size -= byte_count;
               if (sdst && cache_policy != L2_BYPASS)
            /* If it's not a framebuffer fast clear... */
   if (coher == SI_COHERENCY_SHADER)
      }
      /**
   * Realign the CP DMA engine. This must be done after a copy with an unaligned
   * size.
   *
   * \param size  Remaining size to the CP DMA alignment.
   */
   static void si_cp_dma_realign_engine(struct si_context *sctx, unsigned size, unsigned user_flags,
               {
      uint64_t va;
   unsigned dma_flags = 0;
                     /* Use the scratch buffer as the dummy buffer. The 3D engine should be
   * idle at this point.
   */
   if (!sctx->scratch_buffer || sctx->scratch_buffer->b.b.width0 < scratch_size) {
      si_resource_reference(&sctx->scratch_buffer, NULL);
   sctx->scratch_buffer = si_aligned_buffer_create(&sctx->screen->b,
                     if (!sctx->scratch_buffer)
                        si_cp_dma_prepare(sctx, &sctx->scratch_buffer->b.b, &sctx->scratch_buffer->b.b, size, size,
            va = sctx->scratch_buffer->gpu_address;
      }
      /**
   * Do memcpy between buffers using CP DMA.
   * If src or dst is NULL, it means read or write GDS, respectively.
   *
   * \param user_flags    bitmask of SI_CPDMA_*
   */
   void si_cp_dma_copy_buffer(struct si_context *sctx, struct pipe_resource *dst,
                     {
      uint64_t main_dst_offset, main_src_offset;
   unsigned skipped_size = 0;
   unsigned realign_size = 0;
   unsigned gds_flags = (dst ? 0 : CP_DMA_DST_IS_GDS) | (src ? 0 : CP_DMA_SRC_IS_GDS);
                     if (dst) {
      /* Skip this for the L2 prefetch. */
   if (dst != src || dst_offset != src_offset) {
      /* Mark the buffer range of destination as valid (initialized),
   * so that transfer_map knows it should wait for the GPU when mapping
   * that range. */
                  }
   if (src)
            /* The workarounds aren't needed on Fiji and beyond. */
   if (sctx->family <= CHIP_CARRIZO || sctx->family == CHIP_STONEY) {
      /* If the size is not aligned, we must add a dummy copy at the end
   * just to align the internal counter. Otherwise, the DMA engine
   * would slow down by an order of magnitude for following copies.
   */
   if (size % SI_CPDMA_ALIGNMENT)
            /* If the copy begins unaligned, we must start copying from the next
   * aligned block and the skipped part should be copied after everything
   * else has been copied. Only the src alignment matters, not dst.
   *
   * GDS doesn't need the source address to be aligned.
   */
   if (src && src_offset % SI_CPDMA_ALIGNMENT) {
      skipped_size = SI_CPDMA_ALIGNMENT - (src_offset % SI_CPDMA_ALIGNMENT);
   /* The main part will be skipped if the size is too small. */
   skipped_size = MIN2(skipped_size, size);
                  /* TMZ handling */
   if (unlikely(radeon_uses_secure_bos(sctx->ws))) {
      bool secure = src && (si_resource(src)->flags & RADEON_FLAG_ENCRYPTED);
   assert(!secure || (!dst || (si_resource(dst)->flags & RADEON_FLAG_ENCRYPTED)));
   if (secure != sctx->ws->cs_is_secure(&sctx->gfx_cs)) {
      si_flush_gfx_cs(sctx, RADEON_FLUSH_ASYNC_START_NEXT_GFX_IB_NOW |
                  if (user_flags & SI_OP_SYNC_GE_BEFORE)
            if (user_flags & SI_OP_SYNC_CS_BEFORE)
            if (user_flags & SI_OP_SYNC_PS_BEFORE)
            if ((dst || src) && !(user_flags & SI_OP_SKIP_CACHE_INV_BEFORE))
            if (sctx->flags)
            /* This is the main part doing the copying. Src is always aligned. */
   main_dst_offset = dst_offset + skipped_size;
            while (size) {
      unsigned byte_count = MIN2(size, cp_dma_max_byte_count(sctx));
            if (cp_dma_sparse_wa(sctx, si_resource(dst))) {
      unsigned skip_count =
      sctx->ws->buffer_find_next_committed_memory(si_resource(dst)->buf,
      main_dst_offset += skip_count;
   main_src_offset += skip_count;
               if (cp_dma_sparse_wa(sctx, si_resource(src))) {
      unsigned skip_count =
      sctx->ws->buffer_find_next_committed_memory(si_resource(src)->buf,
      main_dst_offset += skip_count;
   main_src_offset += skip_count;
               if (!byte_count)
            si_cp_dma_prepare(sctx, dst, src, byte_count, size + skipped_size + realign_size, user_flags,
            si_emit_cp_dma(sctx, &sctx->gfx_cs, main_dst_offset, main_src_offset, byte_count, dma_flags,
            size -= byte_count;
   main_src_offset += byte_count;
               /* Copy the part we skipped because src wasn't aligned. */
   if (skipped_size) {
               si_cp_dma_prepare(sctx, dst, src, skipped_size, skipped_size + realign_size, user_flags,
            si_emit_cp_dma(sctx, &sctx->gfx_cs, dst_offset, src_offset, skipped_size, dma_flags,
               /* Finally, realign the engine if the size wasn't aligned. */
   if (realign_size) {
                  if (dst && cache_policy != L2_BYPASS)
            /* If it's not a prefetch or GDS copy... */
   if (dst && src && (dst != src || dst_offset != src_offset))
      }
      void si_test_gds(struct si_context *sctx)
   {
      struct pipe_context *ctx = &sctx->b;
   struct pipe_resource *src, *dst;
   unsigned r[4] = {};
            src = pipe_buffer_create(ctx->screen, 0, PIPE_USAGE_DEFAULT, 16);
   dst = pipe_buffer_create(ctx->screen, 0, PIPE_USAGE_DEFAULT, 16);
   si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, src, 0, 4, 0xabcdef01, SI_OP_SYNC_BEFORE_AFTER,
         si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, src, 4, 4, 0x23456789, SI_OP_SYNC_BEFORE_AFTER,
         si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, src, 8, 4, 0x87654321, SI_OP_SYNC_BEFORE_AFTER,
         si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, src, 12, 4, 0xfedcba98, SI_OP_SYNC_BEFORE_AFTER,
         si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, dst, 0, 16, 0xdeadbeef, SI_OP_SYNC_BEFORE_AFTER,
            si_cp_dma_copy_buffer(sctx, NULL, src, offset, 0, 16, SI_OP_SYNC_BEFORE_AFTER,
         si_cp_dma_copy_buffer(sctx, dst, NULL, 0, offset, 16, SI_OP_SYNC_BEFORE_AFTER,
            pipe_buffer_read(ctx, dst, 0, sizeof(r), r);
   printf("GDS copy  = %08x %08x %08x %08x -> %s\n", r[0], r[1], r[2], r[3],
         r[0] == 0xabcdef01 && r[1] == 0x23456789 && r[2] == 0x87654321 && r[3] == 0xfedcba98
            si_cp_dma_clear_buffer(sctx, &sctx->gfx_cs, NULL, offset, 16, 0xc1ea4146,
         si_cp_dma_copy_buffer(sctx, dst, NULL, 0, offset, 16, SI_OP_SYNC_BEFORE_AFTER,
            pipe_buffer_read(ctx, dst, 0, sizeof(r), r);
   printf("GDS clear = %08x %08x %08x %08x -> %s\n", r[0], r[1], r[2], r[3],
         r[0] == 0xc1ea4146 && r[1] == 0xc1ea4146 && r[2] == 0xc1ea4146 && r[3] == 0xc1ea4146
            pipe_resource_reference(&src, NULL);
   pipe_resource_reference(&dst, NULL);
      }
      void si_cp_write_data(struct si_context *sctx, struct si_resource *buf, unsigned offset,
         {
               assert(offset % 4 == 0);
            if (sctx->gfx_level == GFX6 && dst_sel == V_370_MEM)
            radeon_add_to_buffer_list(sctx, cs, buf, RADEON_USAGE_WRITE | RADEON_PRIO_CP_DMA);
            radeon_begin(cs);
   radeon_emit(PKT3(PKT3_WRITE_DATA, 2 + size / 4, 0));
   radeon_emit(S_370_DST_SEL(dst_sel) | S_370_WR_CONFIRM(1) | S_370_ENGINE_SEL(engine));
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit_array((const uint32_t *)data, size / 4);
      }
      void si_cp_copy_data(struct si_context *sctx, struct radeon_cmdbuf *cs, unsigned dst_sel,
               {
      /* cs can point to the compute IB, which has the buffer list in gfx_cs. */
   if (dst) {
         }
   if (src) {
                  uint64_t dst_va = (dst ? dst->gpu_address : 0ull) + dst_offset;
            radeon_begin(cs);
   radeon_emit(PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(COPY_DATA_SRC_SEL(src_sel) | COPY_DATA_DST_SEL(dst_sel) | COPY_DATA_WR_CONFIRM);
   radeon_emit(src_va);
   radeon_emit(src_va >> 32);
   radeon_emit(dst_va);
   radeon_emit(dst_va >> 32);
      }
