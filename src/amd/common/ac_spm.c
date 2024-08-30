   /*
   * Copyright 2021 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_spm.h"
      #include "util/bitscan.h"
   #include "util/u_memory.h"
   #include "ac_perfcounter.h"
      /* SPM counters definition. */
   /* GFX10+ */
   static struct ac_spm_counter_descr gfx10_num_l2_hits = {TCP, 0x9};
   static struct ac_spm_counter_descr gfx10_num_l2_misses = {TCP, 0x12};
   static struct ac_spm_counter_descr gfx10_num_scache_hits = {SQ, 0x14f};
   static struct ac_spm_counter_descr gfx10_num_scache_misses = {SQ, 0x150};
   static struct ac_spm_counter_descr gfx10_num_scache_misses_dup = {SQ, 0x151};
   static struct ac_spm_counter_descr gfx10_num_icache_hits = {SQ, 0x12c};
   static struct ac_spm_counter_descr gfx10_num_icache_misses = {SQ, 0x12d};
   static struct ac_spm_counter_descr gfx10_num_icache_misses_dup = {SQ, 0x12e};
   static struct ac_spm_counter_descr gfx10_num_gl1c_hits = {GL1C, 0xe};
   static struct ac_spm_counter_descr gfx10_num_gl1c_misses = {GL1C, 0x12};
   static struct ac_spm_counter_descr gfx10_num_gl2c_hits = {GL2C, 0x3};
   static struct ac_spm_counter_descr gfx10_num_gl2c_misses = {GL2C, 0x23};
      static struct ac_spm_counter_create_info gfx10_spm_counters[] = {
      {&gfx10_num_l2_hits},
   {&gfx10_num_l2_misses},
   {&gfx10_num_scache_hits},
   {&gfx10_num_scache_misses},
   {&gfx10_num_scache_misses_dup},
   {&gfx10_num_icache_hits},
   {&gfx10_num_icache_misses},
   {&gfx10_num_icache_misses_dup},
   {&gfx10_num_gl1c_hits},
   {&gfx10_num_gl1c_misses},
   {&gfx10_num_gl2c_hits},
      };
      /* GFX10.3+ */
   static struct ac_spm_counter_descr gfx103_num_gl2c_misses = {GL2C, 0x2b};
      static struct ac_spm_counter_create_info gfx103_spm_counters[] = {
      {&gfx10_num_l2_hits},
   {&gfx10_num_l2_misses},
   {&gfx10_num_scache_hits},
   {&gfx10_num_scache_misses},
   {&gfx10_num_scache_misses_dup},
   {&gfx10_num_icache_hits},
   {&gfx10_num_icache_misses},
   {&gfx10_num_icache_misses_dup},
   {&gfx10_num_gl1c_hits},
   {&gfx10_num_gl1c_misses},
   {&gfx10_num_gl2c_hits},
      };
      /* GFX11+ */
   static struct ac_spm_counter_descr gfx11_num_l2_misses = {TCP, 0x11};
   static struct ac_spm_counter_descr gfx11_num_scache_hits = {SQ_WGP, 0x126};
   static struct ac_spm_counter_descr gfx11_num_scache_misses = {SQ_WGP, 0x127};
   static struct ac_spm_counter_descr gfx11_num_scache_misses_dup = {SQ_WGP, 0x128};
   static struct ac_spm_counter_descr gfx11_num_icache_hits = {SQ_WGP, 0x10e};
   static struct ac_spm_counter_descr gfx11_num_icache_misses = {SQ_WGP, 0x10f};
   static struct ac_spm_counter_descr gfx11_num_icache_misses_dup = {SQ_WGP, 0x110};
      static struct ac_spm_counter_create_info gfx11_spm_counters[] = {
      {&gfx10_num_l2_hits},
   {&gfx11_num_l2_misses},
   {&gfx11_num_scache_hits},
   {&gfx11_num_scache_misses},
   {&gfx11_num_scache_misses_dup},
   {&gfx11_num_icache_hits},
   {&gfx11_num_icache_misses},
   {&gfx11_num_icache_misses_dup},
   {&gfx10_num_gl1c_hits},
   {&gfx10_num_gl1c_misses},
   {&gfx10_num_gl2c_hits},
      };
      static struct ac_spm_block_select *
   ac_spm_get_block_select(struct ac_spm *spm, const struct ac_pc_block *block)
   {
      struct ac_spm_block_select *block_sel, *new_block_sel;
            for (uint32_t i = 0; i < spm->num_block_sel; i++) {
      if (spm->block_sel[i].b->b->b->gpu_block == block->b->b->gpu_block)
               /* Allocate a new select block if it doesn't already exist. */
   num_block_sel = spm->num_block_sel + 1;
   block_sel = realloc(spm->block_sel, num_block_sel * sizeof(*block_sel));
   if (!block_sel)
            spm->num_block_sel = num_block_sel;
            /* Initialize the new select block. */
   new_block_sel = &spm->block_sel[spm->num_block_sel - 1];
            new_block_sel->b = block;
   new_block_sel->instances =
         if (!new_block_sel->instances)
                  for (unsigned i = 0; i < new_block_sel->num_instances; i++)
               }
      struct ac_spm_instance_mapping {
      uint32_t se_index;         /* SE index or 0 if global */
   uint32_t sa_index;         /* SA index or 0 if global or per-SE */
      };
      static bool
   ac_spm_init_instance_mapping(const struct radeon_info *info,
                     {
               if (block->b->b->flags & AC_PC_BLOCK_SE) {
      if (block->b->b->gpu_block == SQ) {
      /* Per-SE blocks. */
   se_index = counter->instance / block->num_instances;
      } else {
      /* Per-SA blocks. */
   assert(block->b->b->gpu_block == GL1C ||
         block->b->b->gpu_block == TCP ||
   se_index = (counter->instance / block->num_instances) / info->max_sa_per_se;
   sa_index = (counter->instance / block->num_instances) % info->max_sa_per_se;
         } else {
      /* Global blocks. */
   assert(block->b->b->gpu_block == GL2C);
               if (se_index >= info->num_se ||
      sa_index >= info->max_sa_per_se ||
   instance_index >= block->num_instances)
         mapping->se_index = se_index;
   mapping->sa_index = sa_index;
               }
      static void
   ac_spm_init_muxsel(const struct radeon_info *info,
                     const struct ac_pc_block *block,
   {
      const uint16_t counter_idx = 2 * spm_wire + (counter->is_even ? 0 : 1);
            if (info->gfx_level >= GFX11) {
      muxsel->gfx11.counter = counter_idx;
   muxsel->gfx11.block = block->b->b->spm_block_select;
   muxsel->gfx11.shader_array = mapping->sa_index;
      } else {
      muxsel->gfx10.counter = counter_idx;
   muxsel->gfx10.block = block->b->b->spm_block_select;
   muxsel->gfx10.shader_array = mapping->sa_index;
         }
      static uint32_t
   ac_spm_init_grbm_gfx_index(const struct ac_pc_block *block,
         {
      uint32_t instance = mapping->instance_index;
            grbm_gfx_index |= S_030800_SE_INDEX(mapping->se_index) |
            switch (block->b->b->gpu_block) {
   case GL2C:
      /* Global blocks. */
   grbm_gfx_index |= S_030800_SE_BROADCAST_WRITES(1);
      case SQ:
      /* Per-SE blocks. */
   grbm_gfx_index |= S_030800_SH_BROADCAST_WRITES(1);
      default:
      /* Other blocks shouldn't broadcast. */
               if (block->b->b->gpu_block == SQ_WGP) {
      union {
      struct {
      uint32_t block_index : 2; /* Block index withing WGP */
   uint32_t wgp_index : 3;
   uint32_t is_below_spi : 1; /* 0: lower WGP numbers, 1: higher WGP numbers */
                           const uint32_t num_wgp_above_spi = 4;
            instance_index.wgp_index =
                                          }
      static bool
   ac_spm_map_counter(struct ac_spm *spm, struct ac_spm_block_select *block_sel,
                     {
               if (block_sel->b->b->b->gpu_block == SQ_WGP) {
      if (!spm->sq_wgp[instance].grbm_gfx_index) {
      spm->sq_wgp[instance].grbm_gfx_index =
               for (unsigned i = 0; i < ARRAY_SIZE(spm->sq_wgp[instance].counters); i++) {
                              cntr_sel->sel0 |= S_036700_PERF_SEL(counter->event_id) |
                  /* Each SQ_WQP modules (GFX11+) share one 32-bit accumulator/wire
   * per pair of selects.
   */
                                 spm->sq_wgp[instance].num_counters++;
         } else if (block_sel->b->b->b->gpu_block == SQ) {
      for (unsigned i = 0; i < ARRAY_SIZE(spm->sqg[instance].counters); i++) {
                              /* SQ doesn't support 16-bit counters. */
   cntr_sel->sel0 |= S_036700_PERF_SEL(counter->event_id) |
                                                      spm->sqg[instance].num_counters++;
         } else {
      /* Generic blocks. */
   struct ac_spm_block_instance *block_instance =
            if (!block_instance->grbm_gfx_index) {
      block_instance->grbm_gfx_index =
               for (unsigned i = 0; i < block_instance->num_counters; i++) {
                     switch (index) {
   case 0: /* use S_037004_PERF_SEL */
      cntr_sel->sel0 |= S_037004_PERF_SEL(counter->event_id) |
                  case 1: /* use S_037004_PERF_SEL1 */
      cntr_sel->sel0 |= S_037004_PERF_SEL1(counter->event_id) |
            case 2: /* use S_037004_PERF_SEL2 */
      cntr_sel->sel1 |= S_037008_PERF_SEL2(counter->event_id) |
            case 3: /* use S_037004_PERF_SEL3 */
      cntr_sel->sel1 |= S_037008_PERF_SEL3(counter->event_id) |
            default:
                                                                                 }
      static bool
   ac_spm_add_counter(const struct radeon_info *info,
                     {
      struct ac_spm_instance_mapping instance_mapping = {0};
   struct ac_spm_counter_info *counter;
   struct ac_spm_block_select *block_sel;
   struct ac_pc_block *block;
            /* Check if the GPU block is valid. */
   block = ac_pc_get_block(pc, counter_info->b->gpu_block);
   if (!block) {
      fprintf(stderr, "ac/spm: Invalid GPU block.\n");
               /* Check if the number of instances is valid. */
   if (counter_info->instance > block->num_global_instances - 1) {
      fprintf(stderr, "ac/spm: Invalid instance ID.\n");
               /* Check if the event ID is valid. */
   if (counter_info->b->event_id > block->b->selectors) {
      fprintf(stderr, "ac/spm: Invalid event ID.\n");
               counter = &spm->counters[spm->num_counters];
            counter->gpu_block = counter_info->b->gpu_block;
   counter->event_id = counter_info->b->event_id;
            /* Get the select block used to configure the counter. */
   block_sel = ac_spm_get_block_select(spm, block);
   if (!block_sel)
            /* Initialize instance mapping for the counter. */
   if (!ac_spm_init_instance_mapping(info, block, counter, &instance_mapping)) {
      fprintf(stderr, "ac/spm: Failed to initialize instance mapping.\n");
               /* Map the counter to the select block. */
   if (!ac_spm_map_counter(spm, block_sel, counter, &instance_mapping, &spm_wire)) {
      fprintf(stderr, "ac/spm: No free slots available!\n");
               /* Determine the counter segment type. */
   if (block->b->b->flags & AC_PC_BLOCK_SE) {
         } else {
                  /* Configure the muxsel for SPM. */
               }
      static void
   ac_spm_fill_muxsel_ram(const struct radeon_info *info,
                     {
      struct ac_spm_muxsel_line *mappings = spm->muxsel_lines[segment_type];
   uint32_t even_counter_idx = 0, even_line_idx = 0;
            /* Add the global timestamps first. */
   if (segment_type == AC_SPM_SEGMENT_TYPE_GLOBAL) {
      if (info->gfx_level >= GFX11) {
      mappings[even_line_idx].muxsel[even_counter_idx++].value = 0xf840;
   mappings[even_line_idx].muxsel[even_counter_idx++].value = 0xf841;
   mappings[even_line_idx].muxsel[even_counter_idx++].value = 0xf842;
      } else {
      for (unsigned i = 0; i < 4; i++) {
                        for (unsigned i = 0; i < spm->num_counters; i++) {
               if (counter->segment_type != segment_type)
            if (counter->is_even) {
                     mappings[even_line_idx].muxsel[even_counter_idx] = spm->counters[i].muxsel;
   if (++even_counter_idx == AC_SPM_NUM_COUNTER_PER_MUXSEL) {
      even_counter_idx = 0;
         } else {
                     mappings[odd_line_idx].muxsel[odd_counter_idx] = spm->counters[i].muxsel;
   if (++odd_counter_idx == AC_SPM_NUM_COUNTER_PER_MUXSEL) {
      odd_counter_idx = 0;
               }
      bool ac_init_spm(const struct radeon_info *info,
               {
      const struct ac_spm_counter_create_info *create_info;
   unsigned create_info_count;
            switch (info->gfx_level) {
   case GFX10:
      create_info_count = ARRAY_SIZE(gfx10_spm_counters);
   create_info = gfx10_spm_counters;
      case GFX10_3:
      create_info_count = ARRAY_SIZE(gfx103_spm_counters);
   create_info = gfx103_spm_counters;
      case GFX11:
   case GFX11_5:
      create_info_count = ARRAY_SIZE(gfx11_spm_counters);
   create_info = gfx11_spm_counters;
      default:
                  /* Count the total number of counters. */
   for (unsigned i = 0; i < create_info_count; i++) {
               if (!block)
                        spm->counters = CALLOC(num_counters, sizeof(*spm->counters));
   if (!spm->counters)
            for (unsigned i = 0; i < create_info_count; i++) {
      const struct ac_pc_block *block = ac_pc_get_block(pc, create_info[i].b->gpu_block);
            for (unsigned j = 0; j < block->num_global_instances; j++) {
               if (!ac_spm_add_counter(info, pc, spm, &counter)) {
      fprintf(stderr, "ac/spm: Failed to add SPM counter (%d).\n", i);
                     /* Determine the segment size and create a muxsel ram for every segment. */
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
               if (s == AC_SPM_SEGMENT_TYPE_GLOBAL) {
      /* The global segment always start with a 64-bit timestamp. */
               /* Count the number of even/odd counters for this segment. */
   for (unsigned c = 0; c < spm->num_counters; c++) {
                              if (counter->is_even) {
         } else {
                     /* Compute the number of lines. */
   unsigned even_lines =
         unsigned odd_lines =
                  spm->muxsel_lines[s] = CALLOC(num_lines, sizeof(*spm->muxsel_lines[s]));
   if (!spm->muxsel_lines[s])
                     /* Compute the maximum number of muxsel lines among all SEs. On GFX11,
   * there is only one SE segment size value and the highest value is used.
   */
   for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_GLOBAL; s++) {
      spm->max_se_muxsel_lines =
               /* RLC uses the following order: Global, SE0, SE1, SE2, SE3, SE4, SE5. */
                     if (info->gfx_level >= GFX11) {
      /* On GFX11, RLC uses one segment size for every single SE. */
   for (unsigned i = 0; i < info->num_se; i++) {
                           } else {
               for (unsigned i = 0; i < info->num_se; i++) {
                                       /* On GFX11, the data size written by the hw is in units of segment. */
               }
      void ac_destroy_spm(struct ac_spm *spm)
   {
      for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
                  for (unsigned i = 0; i < spm->num_block_sel; i++) {
                  FREE(spm->block_sel);
      }
      static uint32_t ac_spm_get_sample_size(const struct ac_spm *spm)
   {
               for (unsigned s = 0; s < AC_SPM_SEGMENT_TYPE_COUNT; s++) {
                     }
      static uint32_t ac_spm_get_num_samples(const struct ac_spm *spm)
   {
      uint32_t sample_size = ac_spm_get_sample_size(spm);
   uint32_t *ptr = (uint32_t *)spm->ptr;
   uint32_t data_size, num_lines_written;
            /* Get the data size (in bytes) written by the hw to the ring buffer. */
            /* Compute the number of 256 bits (16 * 16-bits counters) lines written. */
            /* Check for overflow. */
   if (num_lines_written % (sample_size / 32)) {
         } else {
                     }
      void ac_spm_get_trace(const struct ac_spm *spm, struct ac_spm_trace *trace)
   {
               trace->ptr = spm->ptr;
   trace->sample_interval = spm->sample_interval;
   trace->num_counters = spm->num_counters;
   trace->counters = spm->counters;
   trace->sample_size_in_bytes = ac_spm_get_sample_size(spm);
      }
