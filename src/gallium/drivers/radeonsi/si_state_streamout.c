   /*
   * Copyright 2013 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
   #include "util/u_memory.h"
   #include "util/u_suballoc.h"
      static void si_set_streamout_enable(struct si_context *sctx, bool enable);
      static inline void si_so_target_reference(struct si_streamout_target **dst,
         {
         }
      static struct pipe_stream_output_target *si_create_so_target(struct pipe_context *ctx,
                     {
      struct si_streamout_target *t;
            t = CALLOC_STRUCT(si_streamout_target);
   if (!t) {
                  t->b.reference.count = 1;
   t->b.context = ctx;
   pipe_resource_reference(&t->b.buffer, buffer);
   t->b.buffer_offset = buffer_offset;
            util_range_add(&buf->b.b, &buf->valid_buffer_range, buffer_offset, buffer_offset + buffer_size);
      }
      static void si_so_target_destroy(struct pipe_context *ctx, struct pipe_stream_output_target *target)
   {
      struct si_streamout_target *t = (struct si_streamout_target *)target;
   pipe_resource_reference(&t->b.buffer, NULL);
   si_resource_reference(&t->buf_filled_size, NULL);
      }
      void si_streamout_buffers_dirty(struct si_context *sctx)
   {
      if (!sctx->streamout.enabled_mask)
            si_mark_atom_dirty(sctx, &sctx->atoms.s.streamout_begin);
      }
      static void si_set_streamout_targets(struct pipe_context *ctx, unsigned num_targets,
               {
      struct si_context *sctx = (struct si_context *)ctx;
   unsigned old_num_targets = sctx->streamout.num_targets;
   unsigned i;
            /* We are going to unbind the buffers. Mark which caches need to be flushed. */
   if (sctx->streamout.num_targets && sctx->streamout.begin_emitted) {
      /* Since streamout uses vector writes which go through TC L2
   * and most other clients can use TC L2 as well, we don't need
   * to flush it.
   *
   * The only cases which requires flushing it is VGT DMA index
   * fetching (on <= GFX7) and indirect draw data, which are rare
   * cases. Thus, flag the TC L2 dirtiness in the resource and
   * handle it at draw call time.
   */
   for (i = 0; i < sctx->streamout.num_targets; i++)
                  /* Invalidate the scalar cache in case a streamout buffer is
   * going to be used as a constant buffer.
   *
   * Invalidate vL1, because streamout bypasses it (done by
   * setting GLC=1 in the store instruction), but vL1 in other
   * CUs can contain outdated data of streamout buffers.
   *
   * VS_PARTIAL_FLUSH is required if the buffers are going to be
   * used as an input immediately.
   */
            if (sctx->gfx_level >= GFX11) {
               /* Wait now. This is needed to make sure that GDS is not
   * busy at the end of IBs.
   *
   * Also, the next streamout operation will overwrite GDS,
   * so we need to make sure that it's idle.
   */
      } else {
                     /* All readers of the streamout targets need to be finished before we can
   * start writing to the targets.
   */
   if (num_targets) {
      sctx->flags |= SI_CONTEXT_PS_PARTIAL_FLUSH | SI_CONTEXT_CS_PARTIAL_FLUSH |
               if (sctx->flags)
            /* Streamout buffers must be bound in 2 places:
   * 1) in VGT by setting the VGT_STRMOUT registers
   * 2) as shader resources
            /* Stop streamout. */
   if (sctx->streamout.num_targets && sctx->streamout.begin_emitted)
            /* TODO: This is a hack that fixes streamout failures. It shouldn't be necessary. */
   if (sctx->gfx_level >= GFX11 && !wait_now)
            /* Set the new targets. */
   unsigned enabled_mask = 0, append_bitmask = 0;
   for (i = 0; i < num_targets; i++) {
      si_so_target_reference(&sctx->streamout.targets[i], targets[i]);
   if (!targets[i])
                     if (offsets[i] == ((unsigned)-1))
            /* Allocate space for the filled buffer size. */
   struct si_streamout_target *t = sctx->streamout.targets[i];
   if (!t->buf_filled_size) {
      unsigned buf_filled_size_size = sctx->gfx_level >= GFX11 ? 8 : 4;
   u_suballocator_alloc(&sctx->allocator_zeroed_memory, buf_filled_size_size, 4,
                        for (; i < sctx->streamout.num_targets; i++)
            if (!!sctx->streamout.enabled_mask != !!enabled_mask) {
      sctx->streamout.enabled_mask = enabled_mask;
               sctx->streamout.num_targets = num_targets;
            /* Update dirty state bits. */
   if (num_targets) {
         } else {
      si_set_atom_dirty(sctx, &sctx->atoms.s.streamout_begin, false);
               /* Set the shader resources.*/
   for (i = 0; i < num_targets; i++) {
      if (targets[i]) {
                     if (sctx->gfx_level >= GFX11) {
      sbuf.buffer_offset = targets[i]->buffer_offset;
      } else {
      sbuf.buffer_offset = 0;
               si_set_internal_shader_buffer(sctx, SI_VS_STREAMOUT_BUF0 + i, &sbuf);
      } else {
            }
   for (; i < old_num_targets; i++)
            if (wait_now)
      }
      static void si_flush_vgt_streamout(struct si_context *sctx)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
                     /* The register is at different places on different ASICs. */
   if (sctx->gfx_level >= GFX9) {
      reg_strmout_cntl = R_0300FC_CP_STRMOUT_CNTL;
   radeon_emit(PKT3(PKT3_WRITE_DATA, 3, 0));
   radeon_emit(S_370_DST_SEL(V_370_MEM_MAPPED_REGISTER) | S_370_ENGINE_SEL(V_370_ME));
   radeon_emit(R_0300FC_CP_STRMOUT_CNTL >> 2);
   radeon_emit(0);
      } else if (sctx->gfx_level >= GFX7) {
      reg_strmout_cntl = R_0300FC_CP_STRMOUT_CNTL;
      } else {
      reg_strmout_cntl = R_0084FC_CP_STRMOUT_CNTL;
               radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
            radeon_emit(PKT3(PKT3_WAIT_REG_MEM, 5, 0));
   radeon_emit(WAIT_REG_MEM_EQUAL); /* wait until the register is equal to the reference value */
   radeon_emit(reg_strmout_cntl >> 2); /* register */
   radeon_emit(0);
   radeon_emit(S_0084FC_OFFSET_UPDATE_DONE(1)); /* reference value */
   radeon_emit(S_0084FC_OFFSET_UPDATE_DONE(1)); /* mask */
   radeon_emit(4);                              /* poll interval */
      }
      static void si_emit_streamout_begin(struct si_context *sctx, unsigned index)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            if (sctx->gfx_level < GFX11)
            for (unsigned i = 0; i < sctx->streamout.num_targets; i++) {
      if (!t[i])
                     if (sctx->gfx_level >= GFX11) {
      if (sctx->streamout.append_bitmask & (1 << i)) {
      /* Restore the register value. */
   si_cp_copy_data(sctx, cs, COPY_DATA_REG, NULL,
                  } else {
      /* Set to 0. */
   radeon_begin(cs);
   radeon_set_uconfig_reg(R_031088_GDS_STRMOUT_DWORDS_WRITTEN_0 + i * 4, 0);
         } else {
      /* Legacy streamout.
   *
   * The hw binds streamout buffers as shader resources. VGT only counts primitives
   * and tells the shader through SGPRs what to do.
   */
   radeon_begin(cs);
   radeon_set_context_reg_seq(R_028AD0_VGT_STRMOUT_BUFFER_SIZE_0 + 16 * i, 2);
                                    /* Append. */
   radeon_emit(PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
   radeon_emit(STRMOUT_SELECT_BUFFER(i) |
         radeon_emit(0);                                              /* unused */
   radeon_emit(0);                                              /* unused */
                  radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, t[i]->buf_filled_size,
      } else {
      /* Start from the beginning. */
   radeon_emit(PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
   radeon_emit(STRMOUT_SELECT_BUFFER(i) |
         radeon_emit(0);                                                 /* unused */
   radeon_emit(0);                                                 /* unused */
   radeon_emit(t[i]->b.buffer_offset >> 2); /* buffer offset in DW */
      }
                     }
      void si_emit_streamout_end(struct si_context *sctx)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            if (sctx->gfx_level >= GFX11) {
      /* Wait for streamout to finish before reading GDS_STRMOUT registers. */
   sctx->flags |= SI_CONTEXT_VS_PARTIAL_FLUSH;
      } else {
                  for (unsigned i = 0; i < sctx->streamout.num_targets; i++) {
      if (!t[i])
                     if (sctx->gfx_level >= GFX11) {
      si_cp_copy_data(sctx, &sctx->gfx_cs, COPY_DATA_DST_MEM,
                     sctx->flags |= SI_CONTEXT_PFP_SYNC_ME;
      } else {
      radeon_begin(cs);
   radeon_emit(PKT3(PKT3_STRMOUT_BUFFER_UPDATE, 4, 0));
   radeon_emit(STRMOUT_SELECT_BUFFER(i) | STRMOUT_OFFSET_SOURCE(STRMOUT_OFFSET_NONE) |
         radeon_emit(va);                                  /* dst address lo */
   radeon_emit(va >> 32);                            /* dst address hi */
                  /* Zero the buffer size. The counters (primitives generated,
   * primitives emitted) may be enabled even if there is not
   * buffer bound. This ensures that the primitives-emitted query
   * won't increment. */
                  radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, t[i]->buf_filled_size,
                              }
      /* STREAMOUT CONFIG DERIVED STATE
   *
   * Streamout must be enabled for the PRIMITIVES_GENERATED query to work.
   * The buffer mask is an independent state, so no writes occur if there
   * are no buffers bound.
   */
      static void si_emit_streamout_enable(struct si_context *sctx, unsigned index)
   {
               radeon_begin(&sctx->gfx_cs);
   radeon_set_context_reg_seq(R_028B94_VGT_STRMOUT_CONFIG, 2);
   radeon_emit(S_028B94_STREAMOUT_0_EN(si_get_strmout_en(sctx)) |
               S_028B94_RAST_STREAM(0) |
   S_028B94_STREAMOUT_1_EN(si_get_strmout_en(sctx)) |
   radeon_emit(sctx->streamout.hw_enabled_mask & sctx->streamout.enabled_stream_buffers_mask);
      }
      static void si_set_streamout_enable(struct si_context *sctx, bool enable)
   {
      bool old_strmout_en = si_get_strmout_en(sctx);
                     sctx->streamout.hw_enabled_mask =
      sctx->streamout.enabled_mask | (sctx->streamout.enabled_mask << 4) |
         if (sctx->gfx_level < GFX11 &&
      ((old_strmout_en != si_get_strmout_en(sctx)) ||
   (old_hw_enabled_mask != sctx->streamout.hw_enabled_mask)))
   }
      void si_update_prims_generated_query_state(struct si_context *sctx, unsigned type, int diff)
   {
      if (sctx->gfx_level < GFX11 && type == PIPE_QUERY_PRIMITIVES_GENERATED) {
               sctx->streamout.num_prims_gen_queries += diff;
                     if (old_strmout_en != si_get_strmout_en(sctx))
            if (si_update_ngg(sctx)) {
      si_shader_change_notify(sctx);
            }
      void si_init_streamout_functions(struct si_context *sctx)
   {
      sctx->b.create_stream_output_target = si_create_so_target;
   sctx->b.stream_output_target_destroy = si_so_target_destroy;
   sctx->b.set_stream_output_targets = si_set_streamout_targets;
            if (sctx->gfx_level < GFX11)
      }
