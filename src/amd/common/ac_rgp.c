   /*
   * Copyright 2020 Advanced Micro Devices, Inc.
   * Copyright © 2020 Valve Corporation
   *
   * SPDX-License-Identifier: MIT
   */
   #include "ac_rgp.h"
      #include "util/macros.h"
   #include "util/os_misc.h"
   #include "util/os_time.h"
   #include "util/u_process.h"
   #include "util/u_math.h"
      #include "ac_spm.h"
   #include "ac_sqtt.h"
   #include "ac_gpu_info.h"
   #include "amd_family.h"
      #include <stdbool.h>
   #include <string.h>
      #define SQTT_FILE_MAGIC_NUMBER  0x50303042
   #define SQTT_FILE_VERSION_MAJOR 1
   #define SQTT_FILE_VERSION_MINOR 5
      #define SQTT_GPU_NAME_MAX_SIZE 256
   #define SQTT_MAX_NUM_SE        32
   #define SQTT_SA_PER_SE         2
   #define SQTT_ACTIVE_PIXEL_PACKER_MASK_DWORDS 4
      enum sqtt_version
   {
      SQTT_VERSION_NONE = 0x0,
   SQTT_VERSION_2_2 = 0x5, /* GFX8 */
   SQTT_VERSION_2_3 = 0x6, /* GFX9 */
   SQTT_VERSION_2_4 = 0x7, /* GFX10+ */
      };
      /**
   * SQTT chunks.
   */
   enum sqtt_file_chunk_type
   {
      SQTT_FILE_CHUNK_TYPE_ASIC_INFO,
   SQTT_FILE_CHUNK_TYPE_SQTT_DESC,
   SQTT_FILE_CHUNK_TYPE_SQTT_DATA,
   SQTT_FILE_CHUNK_TYPE_API_INFO,
   SQTT_FILE_CHUNK_TYPE_RESERVED,
   SQTT_FILE_CHUNK_TYPE_QUEUE_EVENT_TIMINGS,
   SQTT_FILE_CHUNK_TYPE_CLOCK_CALIBRATION,
   SQTT_FILE_CHUNK_TYPE_CPU_INFO,
   SQTT_FILE_CHUNK_TYPE_SPM_DB,
   SQTT_FILE_CHUNK_TYPE_CODE_OBJECT_DATABASE,
   SQTT_FILE_CHUNK_TYPE_CODE_OBJECT_LOADER_EVENTS,
   SQTT_FILE_CHUNK_TYPE_PSO_CORRELATION,
   SQTT_FILE_CHUNK_TYPE_INSTRUMENTATION_TABLE,
      };
      struct sqtt_file_chunk_id {
      enum sqtt_file_chunk_type type : 8;
   int32_t index : 8;
      };
      struct sqtt_file_chunk_header {
      struct sqtt_file_chunk_id chunk_id;
   uint16_t minor_version;
   uint16_t major_version;
   int32_t size_in_bytes;
      };
      /**
   * SQTT file header.
   */
   struct sqtt_file_header_flags {
      union {
      struct {
      uint32_t is_semaphore_queue_timing_etw : 1;
   uint32_t no_queue_semaphore_timestamps : 1;
                     };
      struct sqtt_file_header {
      uint32_t magic_number;
   uint32_t version_major;
   uint32_t version_minor;
   struct sqtt_file_header_flags flags;
   int32_t chunk_offset;
   int32_t second;
   int32_t minute;
   int32_t hour;
   int32_t day_in_month;
   int32_t month;
   int32_t year;
   int32_t day_in_week;
   int32_t day_in_year;
      };
      static_assert(sizeof(struct sqtt_file_header) == 56, "sqtt_file_header doesn't match RGP spec");
      static void ac_sqtt_fill_header(struct sqtt_file_header *header)
   {
      struct tm *timep, result;
            header->magic_number = SQTT_FILE_MAGIC_NUMBER;
   header->version_major = SQTT_FILE_VERSION_MAJOR;
   header->version_minor = SQTT_FILE_VERSION_MINOR;
   header->flags.value = 0;
   header->flags.is_semaphore_queue_timing_etw = 1;
   header->flags.no_queue_semaphore_timestamps = 0;
            time(&raw_time);
            header->second = timep->tm_sec;
   header->minute = timep->tm_min;
   header->hour = timep->tm_hour;
   header->day_in_month = timep->tm_mday;
   header->month = timep->tm_mon;
   header->year = timep->tm_year;
   header->day_in_week = timep->tm_wday;
   header->day_in_year = timep->tm_yday;
      }
      /**
   * SQTT CPU info.
   */
   struct sqtt_file_chunk_cpu_info {
      struct sqtt_file_chunk_header header;
   uint32_t vendor_id[4];
   uint32_t processor_brand[12];
   uint32_t reserved[2];
   uint64_t cpu_timestamp_freq;
   uint32_t clock_speed;
   uint32_t num_logical_cores;
   uint32_t num_physical_cores;
      };
      static_assert(sizeof(struct sqtt_file_chunk_cpu_info) == 112,
            static void ac_sqtt_fill_cpu_info(struct sqtt_file_chunk_cpu_info *chunk)
   {
      uint32_t cpu_clock_speed_total = 0;
   uint64_t system_ram_size = 0;
   char line[1024];
            chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_CPU_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
                     strncpy((char *)chunk->vendor_id, "Unknown", sizeof(chunk->vendor_id));
   strncpy((char *)chunk->processor_brand, "Unknown", sizeof(chunk->processor_brand));
   chunk->clock_speed = 0;
   chunk->num_logical_cores = 0;
   chunk->num_physical_cores = 0;
   chunk->system_ram_size = 0;
   if (os_get_total_physical_memory(&system_ram_size))
            /* Parse cpuinfo to get more detailed information. */
   f = fopen("/proc/cpuinfo", "r");
   if (!f)
            while (fgets(line, sizeof(line), f)) {
               /* Parse vendor name. */
   str = strstr(line, "vendor_id");
   if (str) {
      char *ptr = (char *)chunk->vendor_id;
   char *v = strtok(str, ":");
   v = strtok(NULL, ":");
   strncpy(ptr, v + 1, sizeof(chunk->vendor_id) - 1);
               /* Parse processor name. */
   str = strstr(line, "model name");
   if (str) {
      char *ptr = (char *)chunk->processor_brand;
   char *v = strtok(str, ":");
   v = strtok(NULL, ":");
   strncpy(ptr, v + 1, sizeof(chunk->processor_brand) - 1);
               /* Parse the current CPU clock speed for each cores. */
   str = strstr(line, "cpu MHz");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "cpu MHz : %d", &v) == 1)
               /* Parse the number of logical cores. */
   str = strstr(line, "siblings");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "siblings : %d", &v) == 1)
               /* Parse the number of physical cores. */
   str = strstr(line, "cpu cores");
   if (str) {
      uint32_t v = 0;
   if (sscanf(str, "cpu cores : %d", &v) == 1)
                  if (chunk->num_logical_cores)
               }
      /**
   * SQTT ASIC info.
   */
   enum sqtt_file_chunk_asic_info_flags
   {
      SQTT_FILE_CHUNK_ASIC_INFO_FLAG_SC_PACKER_NUMBERING = (1 << 0),
      };
      enum sqtt_gpu_type
   {
      SQTT_GPU_TYPE_UNKNOWN = 0x0,
   SQTT_GPU_TYPE_INTEGRATED = 0x1,
   SQTT_GPU_TYPE_DISCRETE = 0x2,
      };
      enum sqtt_gfxip_level
   {
      SQTT_GFXIP_LEVEL_NONE = 0x0,
   SQTT_GFXIP_LEVEL_GFXIP_6 = 0x1,
   SQTT_GFXIP_LEVEL_GFXIP_7 = 0x2,
   SQTT_GFXIP_LEVEL_GFXIP_8 = 0x3,
   SQTT_GFXIP_LEVEL_GFXIP_8_1 = 0x4,
   SQTT_GFXIP_LEVEL_GFXIP_9 = 0x5,
   SQTT_GFXIP_LEVEL_GFXIP_10_1 = 0x7,
   SQTT_GFXIP_LEVEL_GFXIP_10_3 = 0x9,
      };
      enum sqtt_memory_type
   {
      SQTT_MEMORY_TYPE_UNKNOWN = 0x0,
   SQTT_MEMORY_TYPE_DDR = 0x1,
   SQTT_MEMORY_TYPE_DDR2 = 0x2,
   SQTT_MEMORY_TYPE_DDR3 = 0x3,
   SQTT_MEMORY_TYPE_DDR4 = 0x4,
   SQTT_MEMORY_TYPE_DDR5 = 0x5,
   SQTT_MEMORY_TYPE_GDDR3 = 0x10,
   SQTT_MEMORY_TYPE_GDDR4 = 0x11,
   SQTT_MEMORY_TYPE_GDDR5 = 0x12,
   SQTT_MEMORY_TYPE_GDDR6 = 0x13,
   SQTT_MEMORY_TYPE_HBM = 0x20,
   SQTT_MEMORY_TYPE_HBM2 = 0x21,
   SQTT_MEMORY_TYPE_HBM3 = 0x22,
   SQTT_MEMORY_TYPE_LPDDR4 = 0x30,
      };
      struct sqtt_file_chunk_asic_info {
      struct sqtt_file_chunk_header header;
   uint64_t flags;
   uint64_t trace_shader_core_clock;
   uint64_t trace_memory_clock;
   int32_t device_id;
   int32_t device_revision_id;
   int32_t vgprs_per_simd;
   int32_t sgprs_per_simd;
   int32_t shader_engines;
   int32_t compute_unit_per_shader_engine;
   int32_t simd_per_compute_unit;
   int32_t wavefronts_per_simd;
   int32_t minimum_vgpr_alloc;
   int32_t vgpr_alloc_granularity;
   int32_t minimum_sgpr_alloc;
   int32_t sgpr_alloc_granularity;
   int32_t hardware_contexts;
   enum sqtt_gpu_type gpu_type;
   enum sqtt_gfxip_level gfxip_level;
   int32_t gpu_index;
   int32_t gds_size;
   int32_t gds_per_shader_engine;
   int32_t ce_ram_size;
   int32_t ce_ram_size_graphics;
   int32_t ce_ram_size_compute;
   int32_t max_number_of_dedicated_cus;
   int64_t vram_size;
   int32_t vram_bus_width;
   int32_t l2_cache_size;
   int32_t l1_cache_size;
   int32_t lds_size;
   char gpu_name[SQTT_GPU_NAME_MAX_SIZE];
   float alu_per_clock;
   float texture_per_clock;
   float prims_per_clock;
   float pixels_per_clock;
   uint64_t gpu_timestamp_frequency;
   uint64_t max_shader_core_clock;
   uint64_t max_memory_clock;
   uint32_t memory_ops_per_clock;
   enum sqtt_memory_type memory_chip_type;
   uint32_t lds_granularity;
   uint16_t cu_mask[SQTT_MAX_NUM_SE][SQTT_SA_PER_SE];
   char reserved1[128];
   uint32_t active_pixel_packer_mask[SQTT_ACTIVE_PIXEL_PACKER_MASK_DWORDS];
   char reserved2[16];
   uint32_t gl1_cache_size;
   uint32_t instruction_cache_size;
   uint32_t scalar_cache_size;
   uint32_t mall_cache_size;
      };
      static_assert(sizeof(struct sqtt_file_chunk_asic_info) == 768,
            static enum sqtt_gfxip_level ac_gfx_level_to_sqtt_gfxip_level(enum amd_gfx_level gfx_level)
   {
      switch (gfx_level) {
   case GFX8:
         case GFX9:
         case GFX10:
         case GFX10_3:
         case GFX11:
         default:
            }
      static enum sqtt_memory_type ac_vram_type_to_sqtt_memory_type(uint32_t vram_type)
   {
      switch (vram_type) {
   case AMD_VRAM_TYPE_UNKNOWN:
         case AMD_VRAM_TYPE_DDR2:
         case AMD_VRAM_TYPE_DDR3:
         case AMD_VRAM_TYPE_DDR4:
         case AMD_VRAM_TYPE_DDR5:
         case AMD_VRAM_TYPE_GDDR3:
         case AMD_VRAM_TYPE_GDDR4:
         case AMD_VRAM_TYPE_GDDR5:
         case AMD_VRAM_TYPE_GDDR6:
         case AMD_VRAM_TYPE_HBM:
         case AMD_VRAM_TYPE_LPDDR4:
         case AMD_VRAM_TYPE_LPDDR5:
         default:
            }
      static void ac_sqtt_fill_asic_info(const struct radeon_info *rad_info,
         {
               chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_ASIC_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 5;
                     /* All chips older than GFX9 are affected by the "SPI not
   * differentiating pkr_id for newwave commands" bug.
   */
   if (rad_info->gfx_level < GFX9)
            /* Only GFX9+ support PS1 events. */
   if (rad_info->gfx_level >= GFX9)
            chunk->trace_shader_core_clock = rad_info->max_gpu_freq_mhz * 1000000ull;
            /* RGP gets very confused if these clocks are 0. The numbers here are for profile_peak on
   * VGH since that is the chips where we've seen the need for this workaround. */
   if (!chunk->trace_shader_core_clock)
         if (!chunk->trace_memory_clock)
            chunk->device_id = rad_info->pci_id;
   chunk->device_revision_id = rad_info->pci_rev_id;
   chunk->vgprs_per_simd = rad_info->num_physical_wave64_vgprs_per_simd * (has_wave32 ? 2 : 1);
   chunk->sgprs_per_simd = rad_info->num_physical_sgprs_per_simd;
   chunk->shader_engines = rad_info->max_se;
   chunk->compute_unit_per_shader_engine = rad_info->min_good_cu_per_sa * rad_info->max_sa_per_se;
   chunk->simd_per_compute_unit = rad_info->num_simd_per_compute_unit;
            chunk->minimum_vgpr_alloc = rad_info->min_wave64_vgpr_alloc;
   chunk->vgpr_alloc_granularity = rad_info->wave64_vgpr_alloc_granularity * (has_wave32 ? 2 : 1);
   chunk->minimum_sgpr_alloc = rad_info->min_sgpr_alloc;
            chunk->hardware_contexts = 8;
   chunk->gpu_type =
         chunk->gfxip_level = ac_gfx_level_to_sqtt_gfxip_level(rad_info->gfx_level);
            chunk->max_number_of_dedicated_cus = 0;
   chunk->ce_ram_size = 0;
   chunk->ce_ram_size_graphics = 0;
            chunk->vram_bus_width = rad_info->memory_bus_width;
   chunk->vram_size = (uint64_t)rad_info->vram_size_kb * 1024;
   chunk->l2_cache_size = rad_info->l2_cache_size;
   chunk->l1_cache_size = rad_info->tcp_cache_size;
   chunk->lds_size = rad_info->lds_size_per_workgroup;
   if (rad_info->gfx_level >= GFX10) {
      /* RGP expects the LDS size in CU mode. */
                        chunk->alu_per_clock = 0.0;
   chunk->texture_per_clock = 0.0;
   chunk->prims_per_clock = rad_info->max_se;
   if (rad_info->gfx_level == GFX10)
                  chunk->gpu_timestamp_frequency = rad_info->clock_crystal_freq * 1000;
   chunk->max_shader_core_clock = rad_info->max_gpu_freq_mhz * 1000000;
   chunk->max_memory_clock = rad_info->memory_freq_mhz * 1000000;
   chunk->memory_ops_per_clock = ac_memory_ops_per_clock(rad_info->vram_type);
   chunk->memory_chip_type = ac_vram_type_to_sqtt_memory_type(rad_info->vram_type);
            for (unsigned se = 0; se < AMD_MAX_SE; se++) {
      for (unsigned sa = 0; sa < AMD_MAX_SA_PER_SE; sa++) {
                     chunk->gl1_cache_size = rad_info->l1_cache_size;
   chunk->instruction_cache_size = rad_info->sqc_inst_cache_size;
   chunk->scalar_cache_size = rad_info->sqc_scalar_cache_size;
      }
      /**
   * SQTT API info.
   */
   enum sqtt_api_type
   {
      SQTT_API_TYPE_DIRECTX_12,
   SQTT_API_TYPE_VULKAN,
   SQTT_API_TYPE_GENERIC,
      };
      enum sqtt_instruction_trace_mode
   {
      SQTT_INSTRUCTION_TRACE_DISABLED = 0x0,
   SQTT_INSTRUCTION_TRACE_FULL_FRAME = 0x1,
      };
      enum sqtt_profiling_mode
   {
      SQTT_PROFILING_MODE_PRESENT = 0x0,
   SQTT_PROFILING_MODE_USER_MARKERS = 0x1,
   SQTT_PROFILING_MODE_INDEX = 0x2,
      };
      union sqtt_profiling_mode_data {
      struct {
      char start[256];
               struct {
      uint32_t start;
               struct {
      uint32_t begin_hi;
   uint32_t begin_lo;
   uint32_t end_hi;
         };
      union sqtt_instruction_trace_data {
      struct {
                  struct {
            };
      struct sqtt_file_chunk_api_info {
      struct sqtt_file_chunk_header header;
   enum sqtt_api_type api_type;
   uint16_t major_version;
   uint16_t minor_version;
   enum sqtt_profiling_mode profiling_mode;
   uint32_t reserved;
   union sqtt_profiling_mode_data profiling_mode_data;
   enum sqtt_instruction_trace_mode instruction_trace_mode;
   uint32_t reserved2;
      };
      static_assert(sizeof(struct sqtt_file_chunk_api_info) == 560,
            static void ac_sqtt_fill_api_info(struct sqtt_file_chunk_api_info *chunk)
   {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_API_INFO;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 2;
            chunk->api_type = SQTT_API_TYPE_VULKAN;
   chunk->major_version = 0;
   chunk->minor_version = 0;
   chunk->profiling_mode = SQTT_PROFILING_MODE_PRESENT;
      }
      struct sqtt_code_object_database_record {
         };
      struct sqtt_file_chunk_code_object_database {
      struct sqtt_file_chunk_header header;
   uint32_t offset;
   uint32_t flags;
   uint32_t size;
      };
      static void
   ac_sqtt_fill_code_object(const struct rgp_code_object *rgp_code_object,
               {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_CODE_OBJECT_DATABASE;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
   chunk->header.size_in_bytes = chunk_size;
   chunk->offset = file_offset;
   chunk->flags = 0;
   chunk->size = chunk_size;
      }
      struct sqtt_code_object_loader_events_record {
      uint32_t loader_event_type;
   uint32_t reserved;
   uint64_t base_address;
   uint64_t code_object_hash[2];
      };
      struct sqtt_file_chunk_code_object_loader_events {
      struct sqtt_file_chunk_header header;
   uint32_t offset;
   uint32_t flags;
   uint32_t record_size;
      };
      static void
   ac_sqtt_fill_loader_events(const struct rgp_loader_events *rgp_loader_events,
               {
      chunk->header.chunk_id.type =
         chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 1;
   chunk->header.minor_version = 0;
   chunk->header.size_in_bytes = (rgp_loader_events->record_count *
               chunk->offset = file_offset;
   chunk->flags = 0;
   chunk->record_size = sizeof(struct sqtt_code_object_loader_events_record);
      }
   struct sqtt_pso_correlation_record {
      uint64_t api_pso_hash;
   uint64_t pipeline_hash[2];
      };
      struct sqtt_file_chunk_pso_correlation {
      struct sqtt_file_chunk_header header;
   uint32_t offset;
   uint32_t flags;
   uint32_t record_size;
      };
      static void
   ac_sqtt_fill_pso_correlation(const struct rgp_pso_correlation *rgp_pso_correlation,
         {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_PSO_CORRELATION;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
   chunk->header.size_in_bytes = (rgp_pso_correlation->record_count *
               chunk->offset = file_offset;
   chunk->flags = 0;
   chunk->record_size = sizeof(struct sqtt_pso_correlation_record);
      }
      /**
   * SQTT desc info.
   */
   struct sqtt_file_chunk_sqtt_desc {
      struct sqtt_file_chunk_header header;
   int32_t shader_engine_index;
   enum sqtt_version sqtt_version;
   union {
      struct {
         } v0;
   struct {
      int16_t instrumentation_spec_version;
   int16_t instrumentation_api_version;
            };
      static_assert(sizeof(struct sqtt_file_chunk_sqtt_desc) == 32,
            static enum sqtt_version ac_gfx_level_to_sqtt_version(enum amd_gfx_level gfx_level)
   {
      switch (gfx_level) {
   case GFX8:
         case GFX9:
         case GFX10:
         case GFX10_3:
         case GFX11:
         default:
            }
      static void ac_sqtt_fill_sqtt_desc(const struct radeon_info *info,
               {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_SQTT_DESC;
   chunk->header.chunk_id.index = chunk_index;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 2;
            chunk->sqtt_version =
         chunk->shader_engine_index = shader_engine_index;
   chunk->v1.instrumentation_spec_version = 1;
   chunk->v1.instrumentation_api_version = 0;
      }
      /**
   * SQTT data info.
   */
   struct sqtt_file_chunk_sqtt_data {
      struct sqtt_file_chunk_header header;
   int32_t offset; /* in bytes */
      };
      static_assert(sizeof(struct sqtt_file_chunk_sqtt_data) == 24,
            static void ac_sqtt_fill_sqtt_data(struct sqtt_file_chunk_sqtt_data *chunk, int32_t chunk_index,
         {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_SQTT_DATA;
   chunk->header.chunk_id.index = chunk_index;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
            chunk->offset = sizeof(*chunk) + offset;
      }
      /**
   * SQTT queue event timings info.
   */
   struct sqtt_file_chunk_queue_event_timings {
      struct sqtt_file_chunk_header header;
   uint32_t queue_info_table_record_count;
   uint32_t queue_info_table_size;
   uint32_t queue_event_table_record_count;
      };
      static_assert(sizeof(struct sqtt_file_chunk_queue_event_timings) == 32,
            struct sqtt_queue_info_record {
      uint64_t queue_id;
   uint64_t queue_context;
   struct sqtt_queue_hardware_info hardware_info;
      };
      static_assert(sizeof(struct sqtt_queue_info_record) == 24,
            struct sqtt_queue_event_record {
      enum sqtt_queue_event_type event_type;
   uint32_t sqtt_cb_id;
   uint64_t frame_index;
   uint32_t queue_info_index;
   uint32_t submit_sub_index;
   uint64_t api_id;
   uint64_t cpu_timestamp;
      };
      static_assert(sizeof(struct sqtt_queue_event_record) == 56,
            static void
   ac_sqtt_fill_queue_event_timings(const struct rgp_queue_info *rgp_queue_info,
               {
      unsigned queue_info_size =
         unsigned queue_event_size =
            chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_QUEUE_EVENT_TIMINGS;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 1;
   chunk->header.minor_version = 1;
   chunk->header.size_in_bytes = queue_info_size + queue_event_size +
            chunk->queue_info_table_record_count = rgp_queue_info->record_count;
   chunk->queue_info_table_size = queue_info_size;
   chunk->queue_event_table_record_count = rgp_queue_event->record_count;
      }
      /**
   * SQTT clock calibration info.
   */
   struct sqtt_file_chunk_clock_calibration {
      struct sqtt_file_chunk_header header;
   uint64_t cpu_timestamp;
   uint64_t gpu_timestamp;
      };
      static_assert(sizeof(struct sqtt_file_chunk_clock_calibration) == 40,
            static void
   ac_sqtt_fill_clock_calibration(struct sqtt_file_chunk_clock_calibration *chunk,
         {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_CLOCK_CALIBRATION;
   chunk->header.chunk_id.index = chunk_index;
   chunk->header.major_version = 0;
   chunk->header.minor_version = 0;
      }
      /* Below values are from from llvm project
   * llvm/include/llvm/BinaryFormat/ELF.h
   */
   enum elf_gfxip_level
   {
      EF_AMDGPU_MACH_AMDGCN_GFX801 = 0x028,
   EF_AMDGPU_MACH_AMDGCN_GFX900 = 0x02c,
   EF_AMDGPU_MACH_AMDGCN_GFX1010 = 0x033,
   EF_AMDGPU_MACH_AMDGCN_GFX1030 = 0x036,
      };
      static enum elf_gfxip_level ac_gfx_level_to_elf_gfxip_level(enum amd_gfx_level gfx_level)
   {
      switch (gfx_level) {
   case GFX8:
         case GFX9:
         case GFX10:
         case GFX10_3:
         case GFX11:
         default:
            }
      /**
   * SQTT SPM DB info.
   */
   struct sqtt_spm_counter_info {
      enum ac_pc_gpu_block block;
   uint32_t instance;
   uint32_t event_index; /* index of counter within the block */
   uint32_t data_offset; /* offset of counter from the beginning of the chunk */
      };
      struct sqtt_file_chunk_spm_db {
      struct sqtt_file_chunk_header header;
   uint32_t flags;
   uint32_t preamble_size;
   uint32_t num_timestamps;
   uint32_t num_spm_counter_info;
   uint32_t spm_counter_info_size;
      };
      static_assert(sizeof(struct sqtt_file_chunk_spm_db) == 40,
            static void ac_sqtt_fill_spm_db(const struct ac_spm_trace *spm_trace,
                     {
      chunk->header.chunk_id.type = SQTT_FILE_CHUNK_TYPE_SPM_DB;
   chunk->header.chunk_id.index = 0;
   chunk->header.major_version = 2;
   chunk->header.minor_version = 0;
            chunk->flags = 0;
   chunk->preamble_size = sizeof(struct sqtt_file_chunk_spm_db);
   chunk->num_timestamps = num_samples;
   chunk->num_spm_counter_info = spm_trace->num_counters;
   chunk->spm_counter_info_size = sizeof(struct sqtt_spm_counter_info);
      }
      static void ac_sqtt_dump_spm(const struct ac_spm_trace *spm_trace,
               {
      uint32_t sample_size_in_bytes = spm_trace->sample_size_in_bytes;
   uint32_t num_samples = spm_trace->num_samples;
   uint8_t *spm_data_ptr = (uint8_t *)spm_trace->ptr;
   struct sqtt_file_chunk_spm_db spm_db;
            fseek(output, sizeof(struct sqtt_file_chunk_spm_db), SEEK_CUR);
            /* Skip the reserved 32 bytes of data at beginning. */
            /* SPM timestamps. */
   uint32_t sample_size_in_qwords = sample_size_in_bytes / sizeof(uint64_t);
            for (uint32_t s = 0; s < num_samples; s++) {
      uint64_t index = s * sample_size_in_qwords;
            file_offset += sizeof(timestamp);
               /* SPM counter info. */
   uint64_t counter_values_size = num_samples * sizeof(uint16_t);
   uint64_t counter_values_offset = num_samples * sizeof(uint64_t) +
            for (uint32_t c = 0; c < spm_trace->num_counters; c++) {
      struct sqtt_spm_counter_info cntr_info = {
      .block = spm_trace->counters[c].gpu_block,
   .instance = spm_trace->counters[c].instance,
   .data_offset = counter_values_offset,
   .data_size = sizeof(uint16_t),
               file_offset += sizeof(cntr_info);
                        /* SPM counter values. */
   uint32_t sample_size_in_hwords = sample_size_in_bytes / sizeof(uint16_t);
            for (uint32_t c = 0; c < spm_trace->num_counters; c++) {
               for (uint32_t s = 0; s < num_samples; s++) {
                     file_offset += sizeof(value);
                  /* SQTT SPM DB chunk. */
   ac_sqtt_fill_spm_db(spm_trace, &spm_db, num_samples,
         fseek(output, file_spm_db_offset, SEEK_SET);
   fwrite(&spm_db, sizeof(struct sqtt_file_chunk_spm_db), 1, output);
      }
      #if defined(USE_LIBELF)
   static void
   ac_sqtt_dump_data(const struct radeon_info *rad_info, struct ac_sqtt_trace *sqtt_trace,
         {
      struct sqtt_file_chunk_asic_info asic_info = {0};
   struct sqtt_file_chunk_cpu_info cpu_info = {0};
   struct sqtt_file_chunk_api_info api_info = {0};
   struct sqtt_file_header header = {0};
   size_t file_offset = 0;
   const struct rgp_code_object *rgp_code_object = sqtt_trace->rgp_code_object;
   const struct rgp_loader_events *rgp_loader_events = sqtt_trace->rgp_loader_events;
   const struct rgp_pso_correlation *rgp_pso_correlation = sqtt_trace->rgp_pso_correlation;
   const struct rgp_queue_info *rgp_queue_info = sqtt_trace->rgp_queue_info;
   const struct rgp_queue_event *rgp_queue_event = sqtt_trace->rgp_queue_event;
            /* SQTT header file. */
   ac_sqtt_fill_header(&header);
   file_offset += sizeof(header);
            /* SQTT cpu chunk. */
   ac_sqtt_fill_cpu_info(&cpu_info);
   file_offset += sizeof(cpu_info);
            /* SQTT asic chunk. */
   ac_sqtt_fill_asic_info(rad_info, &asic_info);
   file_offset += sizeof(asic_info);
            /* SQTT api chunk. */
   ac_sqtt_fill_api_info(&api_info);
   file_offset += sizeof(api_info);
            /* SQTT code object database chunk. */
   if (rgp_code_object->record_count) {
      size_t file_code_object_offset = file_offset;
   struct sqtt_file_chunk_code_object_database code_object;
   struct sqtt_code_object_database_record code_object_record;
   uint32_t elf_size_calc = 0;
            fseek(output, sizeof(struct sqtt_file_chunk_code_object_database), SEEK_CUR);
   file_offset += sizeof(struct sqtt_file_chunk_code_object_database);
   list_for_each_entry_safe(struct rgp_code_object_record, record,
            fseek(output, sizeof(struct sqtt_code_object_database_record), SEEK_CUR);
   ac_rgp_file_write_elf_object(output, file_offset +
               /* Align to 4 bytes per the RGP file spec. */
   code_object_record.size = ALIGN(elf_size_calc, 4);
   fseek(output, file_offset, SEEK_SET);
   fwrite(&code_object_record, sizeof(struct sqtt_code_object_database_record),
         file_offset += (sizeof(struct sqtt_code_object_database_record) +
            }
   ac_sqtt_fill_code_object(rgp_code_object, &code_object,
               fseek(output, file_code_object_offset, SEEK_SET);
   fwrite(&code_object, sizeof(struct sqtt_file_chunk_code_object_database), 1, output);
               /* SQTT code object loader events chunk. */
   if (rgp_loader_events->record_count) {
               ac_sqtt_fill_loader_events(rgp_loader_events, &loader_events,
         fwrite(&loader_events, sizeof(struct sqtt_file_chunk_code_object_loader_events),
         file_offset += sizeof(struct sqtt_file_chunk_code_object_loader_events);
   list_for_each_entry_safe(struct rgp_loader_events_record, record,
               }
   file_offset += (rgp_loader_events->record_count *
               /* SQTT pso correlation chunk. */
   if (rgp_pso_correlation->record_count) {
               ac_sqtt_fill_pso_correlation(rgp_pso_correlation,
         fwrite(&pso_correlation, sizeof(struct sqtt_file_chunk_pso_correlation), 1,
         file_offset += sizeof(struct sqtt_file_chunk_pso_correlation);
   list_for_each_entry_safe(struct rgp_pso_correlation_record, record,
            fwrite(record, sizeof(struct sqtt_pso_correlation_record),
      }
   file_offset += (rgp_pso_correlation->record_count *
               /* SQTT queue event timings. */
   if (rgp_queue_info->record_count || rgp_queue_event->record_count) {
               ac_sqtt_fill_queue_event_timings(rgp_queue_info, rgp_queue_event,
         fwrite(&queue_event_timings, sizeof(struct sqtt_file_chunk_queue_event_timings), 1,
                  /* Queue info. */
   list_for_each_entry_safe(struct rgp_queue_info_record, record,
               }
   file_offset += (rgp_queue_info->record_count *
            /* Queue event. */
   list_for_each_entry_safe(struct rgp_queue_event_record, record,
               }
   file_offset += (rgp_queue_event->record_count *
               /* SQTT clock calibration. */
   if (rgp_clock_calibration->record_count) {
               list_for_each_entry_safe(struct rgp_clock_calibration_record, record,
                                                   fwrite(&clock_calibration, sizeof(struct sqtt_file_chunk_clock_calibration), 1,
                                 if (sqtt_trace) {
      for (unsigned i = 0; i < sqtt_trace->num_traces; i++) {
      const struct ac_sqtt_data_se *se = &sqtt_trace->traces[i];
   const struct ac_sqtt_data_info *info = &se->info;
   struct sqtt_file_chunk_sqtt_desc desc = {0};
                  /* SQTT desc chunk. */
   ac_sqtt_fill_sqtt_desc(rad_info, &desc, i, se->shader_engine, se->compute_unit);
                  /* SQTT data chunk. */
   ac_sqtt_fill_sqtt_data(&data, i, file_offset, size);
                  /* Copy thread trace data generated by the hardware. */
   file_offset += size;
                  if (spm_trace) {
            }
   #endif
      int
   ac_dump_rgp_capture(const struct radeon_info *info, struct ac_sqtt_trace *sqtt_trace,
         {
   #if !defined(USE_LIBELF)
         #else
      char filename[2048];
   struct tm now;
   time_t t;
            t = time(NULL);
            snprintf(filename, sizeof(filename), "/tmp/%s_%04d.%02d.%02d_%02d.%02d.%02d.rgp",
                  f = fopen(filename, "w+");
   if (!f)
                              fclose(f);
      #endif
   }
