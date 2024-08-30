   /*
   * Copyright 2015 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
   #include "si_query.h"
   #include "util/u_memory.h"
      #include "ac_perfcounter.h"
      struct si_query_group {
      struct si_query_group *next;
   struct ac_pc_block *block;
   unsigned sub_gid;     /* only used during init */
   unsigned result_base; /* only used during init */
   int se;
   int instance;
   unsigned num_counters;
      };
      struct si_query_counter {
      unsigned base;
   unsigned qwords;
      };
      struct si_query_pc {
      struct si_query b;
            /* Size of the results in memory, in bytes. */
            unsigned shaders;
   unsigned num_counters;
   struct si_query_counter *counters;
      };
      static void si_pc_emit_instance(struct si_context *sctx, int se, int instance)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            if (se >= 0) {
         } else {
                  if (sctx->gfx_level >= GFX10) {
      /* TODO: Expose counters from each shader array separately if needed. */
               if (instance >= 0) {
         } else {
                  radeon_begin(cs);
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX, value);
      }
      void si_pc_emit_shaders(struct radeon_cmdbuf *cs, unsigned shaders)
   {
      radeon_begin(cs);
   radeon_set_uconfig_reg_seq(R_036780_SQ_PERFCOUNTER_CTRL, 2, false);
   radeon_emit(shaders & 0x7f);
   radeon_emit(0xffffffff);
      }
      static void si_pc_emit_select(struct si_context *sctx, struct ac_pc_block *block, unsigned count,
         {
      struct ac_pc_block_base *regs = block->b->b;
   struct radeon_cmdbuf *cs = &sctx->gfx_cs;
                     /* Fake counters. */
   if (!regs->select0)
                     for (idx = 0; idx < count; ++idx) {
      radeon_set_uconfig_reg_seq(regs->select0[idx], 1, false);
               for (idx = 0; idx < regs->num_spm_counters; idx++) {
      radeon_set_uconfig_reg_seq(regs->select1[idx], 1, false);
                  }
      static void si_pc_emit_start(struct si_context *sctx, struct si_resource *buffer, uint64_t va)
   {
               si_cp_copy_data(sctx, &sctx->gfx_cs, COPY_DATA_DST_MEM, buffer, va - buffer->gpu_address,
            radeon_begin(cs);
   radeon_set_uconfig_reg(R_036020_CP_PERFMON_CNTL,
         radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
   radeon_emit(EVENT_TYPE(V_028A90_PERFCOUNTER_START) | EVENT_INDEX(0));
   radeon_set_uconfig_reg(R_036020_CP_PERFMON_CNTL,
            }
      /* Note: The buffer was already added in si_pc_emit_start, so we don't have to
   * do it again in here. */
   static void si_pc_emit_stop(struct si_context *sctx, struct si_resource *buffer, uint64_t va)
   {
               si_cp_release_mem(sctx, cs, V_028A90_BOTTOM_OF_PIPE_TS, 0, EOP_DST_SEL_MEM, EOP_INT_SEL_NONE,
                  radeon_begin(cs);
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
            if (!sctx->screen->info.never_send_perfcounter_stop) {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
               radeon_set_uconfig_reg(
      R_036020_CP_PERFMON_CNTL,
   S_036020_PERFMON_STATE(sctx->screen->info.never_stop_sq_perf_counters ?
                     }
      void si_pc_emit_spm_start(struct radeon_cmdbuf *cs)
   {
               /* Start SPM counters. */
   radeon_set_uconfig_reg(R_036020_CP_PERFMON_CNTL,
               /* Start windowed performance counters. */
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
   radeon_emit(EVENT_TYPE(V_028A90_PERFCOUNTER_START) | EVENT_INDEX(0));
               }
      void si_pc_emit_spm_stop(struct radeon_cmdbuf *cs, bool never_stop_sq_perf_counters,
         {
               /* Stop windowed performance counters. */
   if (!never_send_perfcounter_stop) {
      radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));
                        /* Stop SPM counters. */
   radeon_set_uconfig_reg(R_036020_CP_PERFMON_CNTL,
                                 }
      void si_pc_emit_spm_reset(struct radeon_cmdbuf *cs)
   {
      radeon_begin(cs);
   radeon_set_uconfig_reg(R_036020_CP_PERFMON_CNTL,
                  }
         static void si_pc_emit_read(struct si_context *sctx, struct ac_pc_block *block, unsigned count,
         {
      struct ac_pc_block_base *regs = block->b->b;
   struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   unsigned idx;
   unsigned reg = regs->counter0_lo;
                     if (regs->select0) {
      for (idx = 0; idx < count; ++idx) {
                     radeon_emit(PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(COPY_DATA_SRC_SEL(COPY_DATA_PERF) | COPY_DATA_DST_SEL(COPY_DATA_DST_MEM) |
         radeon_emit(reg >> 2);
   radeon_emit(0); /* unused */
   radeon_emit(va);
   radeon_emit(va >> 32);
   va += sizeof(uint64_t);
         } else {
      /* Fake counters. */
   for (idx = 0; idx < count; ++idx) {
      radeon_emit(PKT3(PKT3_COPY_DATA, 4, 0));
   radeon_emit(COPY_DATA_SRC_SEL(COPY_DATA_IMM) | COPY_DATA_DST_SEL(COPY_DATA_DST_MEM) |
         radeon_emit(0); /* immediate */
   radeon_emit(0);
   radeon_emit(va);
   radeon_emit(va >> 32);
         }
      }
      static void si_pc_query_destroy(struct si_context *sctx, struct si_query *squery)
   {
               while (query->groups) {
      struct si_query_group *group = query->groups;
   query->groups = group->next;
                        si_query_buffer_destroy(sctx->screen, &query->buffer);
      }
      void si_inhibit_clockgating(struct si_context *sctx, struct radeon_cmdbuf *cs, bool inhibit)
   {
      if (sctx->gfx_level >= GFX11)
                     if (sctx->gfx_level >= GFX10) {
      radeon_set_uconfig_reg(R_037390_RLC_PERFMON_CLK_CNTL,
      } else if (sctx->gfx_level >= GFX8) {
      radeon_set_uconfig_reg(R_0372FC_RLC_PERFMON_CLK_CNTL,
      }
      }
      static void si_pc_query_resume(struct si_context *sctx, struct si_query *squery)
   /*
               {
      struct si_query_pc *query = (struct si_query_pc *)squery;
   int current_se = -1;
            if (!si_query_buffer_alloc(sctx, &query->buffer, NULL, query->result_size))
                  if (query->shaders)
                     for (struct si_query_group *group = query->groups; group; group = group->next) {
               if (group->se != current_se || group->instance != current_instance) {
      current_se = group->se;
   current_instance = group->instance;
                           if (current_se != -1 || current_instance != -1)
            uint64_t va = query->buffer.buf->gpu_address + query->buffer.results_end;
      }
      static void si_pc_query_suspend(struct si_context *sctx, struct si_query *squery)
   {
               if (!query->buffer.buf)
            uint64_t va = query->buffer.buf->gpu_address + query->buffer.results_end;
                     for (struct si_query_group *group = query->groups; group; group = group->next) {
      struct ac_pc_block *block = group->block;
   unsigned se = group->se >= 0 ? group->se : 0;
            if ((block->b->b->flags & AC_PC_BLOCK_SE) && (group->se < 0))
            do {
               do {
      si_pc_emit_instance(sctx, se, instance);
   si_pc_emit_read(sctx, block, group->num_counters, va);
                                 }
      static bool si_pc_query_begin(struct si_context *ctx, struct si_query *squery)
   {
                        list_addtail(&query->b.active_list, &ctx->active_queries);
                        }
      static bool si_pc_query_end(struct si_context *ctx, struct si_query *squery)
   {
                        list_del(&squery->active_list);
               }
      static void si_pc_query_add_result(struct si_query_pc *query, void *buffer,
         {
      uint64_t *results = buffer;
            for (i = 0; i < query->num_counters; ++i) {
               for (j = 0; j < counter->qwords; ++j) {
      uint32_t value = results[counter->base + j * counter->stride];
            }
      static bool si_pc_query_get_result(struct si_context *sctx, struct si_query *squery, bool wait,
         {
                        for (struct si_query_buffer *qbuf = &query->buffer; qbuf; qbuf = qbuf->previous) {
      unsigned usage = PIPE_MAP_READ | (wait ? 0 : PIPE_MAP_DONTBLOCK);
   unsigned results_base = 0;
            if (squery->b.flushed)
         else
            if (!map)
            while (results_base != qbuf->results_end) {
      si_pc_query_add_result(query, map + results_base, result);
                     }
      static const struct si_query_ops batch_query_ops = {
      .destroy = si_pc_query_destroy,
   .begin = si_pc_query_begin,
   .end = si_pc_query_end,
            .suspend = si_pc_query_suspend,
      };
      static struct si_query_group *get_group_state(struct si_screen *screen, struct si_query_pc *query,
         {
      struct si_perfcounters *pc = screen->perfcounters;
            while (group) {
      if (group->block == block && group->sub_gid == sub_gid)
                     group = CALLOC_STRUCT(si_query_group);
   if (!group)
            group->block = block;
            if (block->b->b->flags & AC_PC_BLOCK_SHADER) {
      unsigned sub_gids = block->num_instances;
   unsigned shader_id;
   unsigned shaders;
            if (ac_pc_block_has_per_se_groups(&pc->base, block))
         shader_id = sub_gid / sub_gids;
                     query_shaders = query->shaders & ~AC_PC_SHADERS_WINDOWING;
   if (query_shaders && query_shaders != shaders) {
      fprintf(stderr, "si_perfcounter: incompatible shader groups\n");
   FREE(group);
      }
               if (block->b->b->flags & AC_PC_BLOCK_SHADER_WINDOWED && !query->shaders) {
      // A non-zero value in query->shaders ensures that the shader
   // masking is reset unless the user explicitly requests one.
               if (ac_pc_block_has_per_se_groups(&pc->base, block)) {
      group->se = sub_gid / block->num_instances;
      } else {
                  if (ac_pc_block_has_per_instance_groups(&pc->base, block)) {
         } else {
                  group->next = query->groups;
               }
      struct pipe_query *si_create_batch_query(struct pipe_context *ctx, unsigned num_queries,
         {
      struct si_screen *screen = (struct si_screen *)ctx->screen;
   struct si_perfcounters *pc = screen->perfcounters;
   struct ac_pc_block *block;
   struct si_query_group *group;
   struct si_query_pc *query;
   unsigned base_gid, sub_gid, sub_index;
            if (!pc)
            query = CALLOC_STRUCT(si_query_pc);
   if (!query)
                              /* Collect selectors per group */
   for (i = 0; i < num_queries; ++i) {
               if (query_types[i] < SI_QUERY_FIRST_PERFCOUNTER)
            block =
         if (!block)
            sub_gid = sub_index / block->b->selectors;
            group = get_group_state(screen, query, block, sub_gid);
   if (!group)
            if (group->num_counters >= block->b->b->num_counters) {
      fprintf(stderr, "perfcounter group %s: too many selected\n", block->b->b->name);
      }
   group->selectors[group->num_counters] = sub_index;
               /* Compute result bases and CS size per group */
   query->b.num_cs_dw_suspend = pc->num_stop_cs_dwords;
            i = 0;
   for (group = query->groups; group; group = group->next) {
      struct ac_pc_block *block = group->block;
   unsigned read_dw;
            if ((block->b->b->flags & AC_PC_BLOCK_SE) && group->se < 0)
         if (group->instance < 0)
            group->result_base = i;
   query->result_size += sizeof(uint64_t) * instances * group->num_counters;
            read_dw = 6 * group->num_counters;
   query->b.num_cs_dw_suspend += instances * read_dw;
               if (query->shaders) {
      if (query->shaders == AC_PC_SHADERS_WINDOWING)
               /* Map user-supplied query array to result indices */
   query->counters = CALLOC(num_queries, sizeof(*query->counters));
   for (i = 0; i < num_queries; ++i) {
      struct si_query_counter *counter = &query->counters[i];
            block =
            sub_gid = sub_index / block->b->selectors;
            group = get_group_state(screen, query, block, sub_gid);
            for (j = 0; j < group->num_counters; ++j) {
      if (group->selectors[j] == sub_index)
               counter->base = group->result_base + j;
            counter->qwords = 1;
   if ((block->b->b->flags & AC_PC_BLOCK_SE) && group->se < 0)
         if (group->instance < 0)
                     error:
      si_pc_query_destroy((struct si_context *)ctx, &query->b);
      }
      int si_get_perfcounter_info(struct si_screen *screen, unsigned index,
         {
      struct si_perfcounters *pc = screen->perfcounters;
   struct ac_pc_block *block;
            if (!pc)
            if (!info) {
               for (bid = 0; bid < pc->base.num_blocks; ++bid) {
                              block = ac_lookup_counter(&pc->base, index, &base_gid, &sub);
   if (!block)
            if (!block->selector_names) {
      if (!ac_init_block_names(&screen->info, &pc->base, block))
      }
   info->name = block->selector_names + sub * block->selector_name_stride;
   info->query_type = SI_QUERY_FIRST_PERFCOUNTER + index;
   info->max_value.u64 = 0;
   info->type = PIPE_DRIVER_QUERY_TYPE_UINT64;
   info->result_type = PIPE_DRIVER_QUERY_RESULT_TYPE_AVERAGE;
   info->group_id = base_gid + sub / block->b->selectors;
   info->flags = PIPE_DRIVER_QUERY_FLAG_BATCH;
   if (sub > 0 && sub + 1 < block->b->selectors * block->num_groups)
            }
      int si_get_perfcounter_group_info(struct si_screen *screen, unsigned index,
         {
      struct si_perfcounters *pc = screen->perfcounters;
            if (!pc)
            if (!info)
            block = ac_lookup_group(&pc->base, &index);
   if (!block)
            if (!block->group_names) {
      if (!ac_init_block_names(&screen->info, &pc->base, block))
      }
   info->name = block->group_names + index * block->group_name_stride;
   info->num_queries = block->b->selectors;
   info->max_active_queries = block->b->b->num_counters;
      }
      void si_destroy_perfcounters(struct si_screen *screen)
   {
               if (!pc)
            ac_destroy_perfcounters(&pc->base);
   FREE(pc);
      }
      void si_init_perfcounters(struct si_screen *screen)
   {
               separate_se = debug_get_bool_option("RADEON_PC_SEPARATE_SE", false);
            screen->perfcounters = CALLOC_STRUCT(si_perfcounters);
   if (!screen->perfcounters)
            screen->perfcounters->num_stop_cs_dwords = 14 + si_cp_write_fence_dwords(screen);
            if (!ac_init_perfcounters(&screen->info, separate_se, separate_instance,
                  }
      static bool
   si_spm_init_bo(struct si_context *sctx)
   {
      struct radeon_winsys *ws = sctx->ws;
            sctx->spm.buffer_size = size;
            sctx->spm.bo = ws->buffer_create(
      ws, size, 4096,
   RADEON_DOMAIN_VRAM,
   RADEON_FLAG_NO_INTERPROCESS_SHARING |
                  }
         static void
   si_emit_spm_counters(struct si_context *sctx, struct radeon_cmdbuf *cs)
   {
                        for (uint32_t instance = 0; instance < ARRAY_SIZE(spm->sqg); instance++) {
               if (!num_counters)
            radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                        for (uint32_t b = 0; b < num_counters; b++) {
                     radeon_set_uconfig_reg_seq(reg_base + b * 4, 1, false);
                  for (uint32_t b = 0; b < spm->num_block_sel; b++) {
      struct ac_spm_block_select *block_sel = &spm->block_sel[b];
            for (unsigned i = 0; i < block_sel->num_instances; i++) {
                                                                        radeon_set_uconfig_reg_seq(regs->select1[c], 1, false);
                     /* Restore global broadcasting. */
   radeon_set_uconfig_reg(R_030800_GRBM_GFX_INDEX,
                     }
      #define SPM_RING_BASE_ALIGN 32
      void
   si_emit_spm_setup(struct si_context *sctx, struct radeon_cmdbuf *cs)
   {
      struct ac_spm *spm = &sctx->spm;
   uint64_t va = sctx->screen->ws->buffer_get_virtual_address(spm->bo);
            /* It's required that the ring VA and the size are correctly aligned. */
   assert(!(va & (SPM_RING_BASE_ALIGN - 1)));
   assert(!(ring_size & (SPM_RING_BASE_ALIGN - 1)));
                     /* Configure the SPM ring buffer. */
   radeon_set_uconfig_reg(R_037200_RLC_SPM_PERFMON_CNTL,
               radeon_set_uconfig_reg(R_037204_RLC_SPM_PERFMON_RING_BASE_LO, va);
   radeon_set_uconfig_reg(R_037208_RLC_SPM_PERFMON_RING_BASE_HI,
                  /* Configure the muxsel. */
   uint32_t total_muxsel_lines = 0;
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
                  radeon_set_uconfig_reg(R_03726C_RLC_SPM_ACCUM_MODE, 0);
   radeon_set_uconfig_reg(R_037210_RLC_SPM_PERFMON_SEGMENT_SIZE, 0);
   radeon_set_uconfig_reg(R_03727C_RLC_SPM_PERFMON_SE3TO0_SEGMENT_SIZE,
                           radeon_set_uconfig_reg(R_037280_RLC_SPM_PERFMON_GLB_SEGMENT_SIZE,
                  /* Upload each muxsel ram to the RLC. */
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
      unsigned rlc_muxsel_addr, rlc_muxsel_data;
   unsigned grbm_gfx_index = S_030800_SH_BROADCAST_WRITES(1) |
            if (!spm->num_muxsel_lines[s])
            if (s == AC_SPM_SEGMENT_TYPE_GLOBAL) {
               rlc_muxsel_addr = R_037224_RLC_SPM_GLOBAL_MUXSEL_ADDR;
      } else {
               rlc_muxsel_addr = R_03721C_RLC_SPM_SE_MUXSEL_ADDR;
                        for (unsigned l = 0; l < spm->num_muxsel_lines[s]; l++) {
                              /* Write the muxsel line configuration with MUXSEL_DATA. */
   radeon_emit(PKT3(PKT3_WRITE_DATA, 2 + AC_SPM_MUXSEL_LINE_SIZE, 0));
   radeon_emit(S_370_DST_SEL(V_370_MEM_MAPPED_REGISTER) |
               S_370_WR_CONFIRM(1) |
   radeon_emit(rlc_muxsel_data >> 2);
   radeon_emit(0);
         }
            /* Select SPM counters. */
      }
      bool
   si_spm_init(struct si_context *sctx)
   {
               sctx->screen->perfcounters = CALLOC_STRUCT(si_perfcounters);
   sctx->screen->perfcounters->num_stop_cs_dwords = 14 + si_cp_write_fence_dwords(sctx->screen);
                     if (!ac_init_perfcounters(info, false, false, pc))
            if (!ac_init_spm(info, pc, &sctx->spm))
            if (!si_spm_init_bo(sctx))
               }
      void
   si_spm_finish(struct si_context *sctx)
   {
      struct pb_buffer *bo = sctx->spm.bo;
               }
