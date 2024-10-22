   /*
   * Copyright © 2009 Corbin Simpson
   * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
   *
   * SPDX-License-Identifier: MIT
   */
      #include "radeon_drm_bo.h"
   #include "radeon_drm_cs.h"
      #include "util/os_file.h"
   #include "util/simple_mtx.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_memory.h"
   #include "util/u_hash_table.h"
   #include "util/u_pointer.h"
      #include <xf86drm.h>
   #include <stdio.h>
   #include <sys/types.h>
   #include <sys/stat.h>
   #include <unistd.h>
   #include <fcntl.h>
   #include <radeon_surface.h>
      static struct hash_table *fd_tab = NULL;
   static simple_mtx_t fd_tab_mutex = SIMPLE_MTX_INITIALIZER;
      /* Enable/disable feature access for one command stream.
   * If enable == true, return true on success.
   * Otherwise, return false.
   *
   * We basically do the same thing kernel does, because we have to deal
   * with multiple contexts (here command streams) backed by one winsys. */
   static bool radeon_set_fd_access(struct radeon_drm_cs *applier,
                           {
      struct drm_radeon_info info;
                              /* Early exit if we are sure the request will fail. */
   if (enable) {
      if (*owner) {
      mtx_unlock(&*mutex);
         } else {
      if (*owner != applier) {
      mtx_unlock(&*mutex);
                  /* Pass through the request to the kernel. */
   info.value = (unsigned long)&value;
   info.request = request;
   if (drmCommandWriteRead(applier->ws->fd, DRM_RADEON_INFO,
            mtx_unlock(&*mutex);
               /* Update the rights in the winsys. */
   if (enable) {
      if (value) {
      *owner = applier;
   mtx_unlock(&*mutex);
         } else {
                  mtx_unlock(&*mutex);
      }
      static bool radeon_get_drm_value(int fd, unsigned request,
         {
      struct drm_radeon_info info;
                     info.value = (unsigned long)out;
            retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
   if (retval) {
      if (errname) {
      fprintf(stderr, "radeon: Failed to get %s, error number %d\n",
      }
      }
      }
      /* Helper function to do the ioctls needed for setup and init. */
   static bool do_winsys_init(struct radeon_drm_winsys *ws)
   {
      struct drm_radeon_gem_info gem_info;
   int retval;
                     /* We do things in a specific order here.
   *
   * DRM version first. We need to be sure we're running on a KMS chipset.
   * This is also for some features.
   *
   * Then, the PCI ID. This is essential and should return usable numbers
   * for all Radeons. If this fails, we probably got handed an FD for some
   * non-Radeon card.
   *
   * The GEM info is actually bogus on the kernel side, as well as our side
   * (see radeon_gem_info_ioctl in radeon_gem.c) but that's alright because
   * we don't actually use the info for anything yet.
   *
   * The GB and Z pipe requests should always succeed, but they might not
   * return sensical values for all chipsets, but that's alright because
   * the pipe drivers already know that.
            /* Get DRM version. */
   version = drmGetVersion(ws->fd);
   if (!version)
            if (version->version_major != 2 ||
      version->version_minor < 50) {
   fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
               __func__,
   version->version_major,
   version->version_minor,
   drmFreeVersion(version);
               ws->info.drm_major = version->version_major;
   ws->info.drm_minor = version->version_minor;
   ws->info.drm_patchlevel = version->version_patchlevel;
   ws->info.is_amdgpu = false;
            /* Get PCI ID. */
   if (!radeon_get_drm_value(ws->fd, RADEON_INFO_DEVICE_ID, "PCI ID",
                  /* Check PCI ID. */
      #define CHIPSET(pci_id, name, cfamily) case pci_id: ws->info.family = CHIP_##cfamily; ws->gen = DRV_R300; break;
   #include "pci_ids/r300_pci_ids.h"
   #undef CHIPSET
      #define CHIPSET(pci_id, name, cfamily) case pci_id: ws->info.family = CHIP_##cfamily; ws->gen = DRV_R600; break;
   #include "pci_ids/r600_pci_ids.h"
   #undef CHIPSET
      #define CHIPSET(pci_id, cfamily) \
      case pci_id: \
   ws->info.family = CHIP_##cfamily; \
   ws->info.name = #cfamily; \
   ws->gen = DRV_SI; \
      #include "pci_ids/radeonsi_pci_ids.h"
   #undef CHIPSET
         default:
      fprintf(stderr, "radeon: Invalid PCI ID.\n");
               switch (ws->info.family) {
   default:
   case CHIP_UNKNOWN:
      fprintf(stderr, "radeon: Unknown family.\n");
      case CHIP_R300:
   case CHIP_R350:
   case CHIP_RV350:
   case CHIP_RV370:
   case CHIP_RV380:
   case CHIP_RS400:
   case CHIP_RC410:
   case CHIP_RS480:
      ws->info.gfx_level = R300;
      case CHIP_R420:     /* R4xx-based cores. */
   case CHIP_R423:
   case CHIP_R430:
   case CHIP_R480:
   case CHIP_R481:
   case CHIP_RV410:
   case CHIP_RS600:
   case CHIP_RS690:
   case CHIP_RS740:
      ws->info.gfx_level = R400;
      case CHIP_RV515:    /* R5xx-based cores. */
   case CHIP_R520:
   case CHIP_RV530:
   case CHIP_R580:
   case CHIP_RV560:
   case CHIP_RV570:
      ws->info.gfx_level = R500;
      case CHIP_R600:
   case CHIP_RV610:
   case CHIP_RV630:
   case CHIP_RV670:
   case CHIP_RV620:
   case CHIP_RV635:
   case CHIP_RS780:
   case CHIP_RS880:
      ws->info.gfx_level = R600;
      case CHIP_RV770:
   case CHIP_RV730:
   case CHIP_RV710:
   case CHIP_RV740:
      ws->info.gfx_level = R700;
      case CHIP_CEDAR:
   case CHIP_REDWOOD:
   case CHIP_JUNIPER:
   case CHIP_CYPRESS:
   case CHIP_HEMLOCK:
   case CHIP_PALM:
   case CHIP_SUMO:
   case CHIP_SUMO2:
   case CHIP_BARTS:
   case CHIP_TURKS:
   case CHIP_CAICOS:
      ws->info.gfx_level = EVERGREEN;
      case CHIP_CAYMAN:
   case CHIP_ARUBA:
      ws->info.gfx_level = CAYMAN;
      case CHIP_TAHITI:
   case CHIP_PITCAIRN:
   case CHIP_VERDE:
   case CHIP_OLAND:
   case CHIP_HAINAN:
      ws->info.gfx_level = GFX6;
      case CHIP_BONAIRE:
   case CHIP_KAVERI:
   case CHIP_KABINI:
   case CHIP_HAWAII:
      ws->info.gfx_level = GFX7;
               /* Set which chips don't have dedicated VRAM. */
   switch (ws->info.family) {
   case CHIP_RS400:
   case CHIP_RC410:
   case CHIP_RS480:
   case CHIP_RS600:
   case CHIP_RS690:
   case CHIP_RS740:
   case CHIP_RS780:
   case CHIP_RS880:
   case CHIP_PALM:
   case CHIP_SUMO:
   case CHIP_SUMO2:
   case CHIP_ARUBA:
   case CHIP_KAVERI:
   case CHIP_KABINI:
      ws->info.has_dedicated_vram = false;
         default:
                  ws->info.ip[AMD_IP_GFX].num_queues = 1;
   /* Check for dma */
   ws->info.ip[AMD_IP_SDMA].num_queues = 0;
   /* DMA is disabled on R700. There is IB corruption and hangs. */
   if (ws->info.gfx_level >= EVERGREEN) {
                  /* Check for UVD and VCE */
            uint32_t value = RADEON_CS_RING_UVD;
   if (radeon_get_drm_value(ws->fd, RADEON_INFO_RING_WORKING,
                        value = RADEON_CS_RING_VCE;
   if (radeon_get_drm_value(ws->fd, RADEON_INFO_RING_WORKING,
               if (radeon_get_drm_value(ws->fd, RADEON_INFO_VCE_FW_VERSION,
            ws->info.vce_fw_version = value;
                  /* Check for userptr support. */
   {
               /* If the ioctl doesn't exist, -EINVAL is returned.
   *
   * If the ioctl exists, it should return -EACCES
   * if RADEON_GEM_USERPTR_READONLY or RADEON_GEM_USERPTR_REGISTER
   * aren't set.
   */
   ws->info.has_userptr =
                     /* Get GEM info. */
   retval = drmCommandWriteRead(ws->fd, DRM_RADEON_GEM_INFO,
         if (retval) {
      fprintf(stderr, "radeon: Failed to get MM info, error number %d\n",
            }
   ws->info.gart_size_kb = DIV_ROUND_UP(gem_info.gart_size, 1024);
   ws->info.vram_size_kb = DIV_ROUND_UP(gem_info.vram_size, 1024);
            /* Radeon allocates all buffers contiguously, which makes large allocations
   * unlikely to succeed. */
   if (ws->info.has_dedicated_vram)
         else
            /* Both 32-bit and 64-bit address spaces only have 4GB.
   * This is a limitation of the VM allocator in the winsys.
   */
            /* Get max clock frequency info and convert it to MHz */
   radeon_get_drm_value(ws->fd, RADEON_INFO_MAX_SCLK, NULL,
                           /* Generation-specific queries. */
   if (ws->gen == DRV_R300) {
      if (!radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_GB_PIPES,
                        if (!radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_Z_PIPES,
                  }
   else if (ws->gen >= DRV_R600) {
               if (!radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_BACKENDS,
                        /* get the GPU counter frequency, failure is not fatal */
   radeon_get_drm_value(ws->fd, RADEON_INFO_CLOCK_CRYSTAL_FREQ, NULL,
            radeon_get_drm_value(ws->fd, RADEON_INFO_TILING_CONFIG, NULL,
            ws->info.r600_num_banks =
         ws->info.gfx_level >= EVERGREEN ?
            ws->info.pipe_interleave_bytes =
         ws->info.gfx_level >= EVERGREEN ?
            if (!ws->info.pipe_interleave_bytes)
                  radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_TILE_PIPES, NULL,
            /* "num_tiles_pipes" must be equal to the number of pipes (Px) in the
   * pipe config field of the GB_TILE_MODE array. Only one card (Tahiti)
   * reports a different value (12). Fix it by setting what's in the
   * GB_TILE_MODE array (8).
   */
   if (ws->gen == DRV_SI && ws->info.num_tile_pipes == 12)
            if (radeon_get_drm_value(ws->fd, RADEON_INFO_BACKEND_MAP, NULL,
                  /* Default value. */
   ws->info.enabled_rb_mask = u_bit_consecutive(0, ws->info.max_render_backends);
   /*
   * This fails (silently) on non-GCN or older kernels, overwriting the
   * default enabled_rb_mask with the result of the last query.
   */
   if (ws->gen >= DRV_SI) {
               radeon_get_drm_value(ws->fd, RADEON_INFO_SI_BACKEND_ENABLED_MASK, NULL, &mask);
                                 ws->info.r600_has_virtual_memory = true;
   if (!radeon_get_drm_value(ws->fd, RADEON_INFO_VA_START, NULL,
               if (!radeon_get_drm_value(ws->fd, RADEON_INFO_IB_VM_MAX_SIZE, NULL,
               radeon_get_drm_value(ws->fd, RADEON_INFO_VA_UNMAP_WORKING, NULL,
            if (ws->gen == DRV_R600 && !debug_get_bool_option("RADEON_VA", false))
               /* Get max pipes, this is only needed for compute shaders.  All evergreen+
   * chips have at least 2 pipes, so we use 2 as a default. */
   ws->info.r600_max_quad_pipes = 2;
   radeon_get_drm_value(ws->fd, RADEON_INFO_MAX_PIPES, NULL,
            /* All GPUs have at least one compute unit */
   ws->info.num_cu = 1;
   radeon_get_drm_value(ws->fd, RADEON_INFO_ACTIVE_CU_COUNT, NULL,
            radeon_get_drm_value(ws->fd, RADEON_INFO_MAX_SE, NULL,
            switch (ws->info.family) {
   case CHIP_HAINAN:
   case CHIP_KABINI:
      ws->info.max_tcc_blocks = 2;
      case CHIP_VERDE:
   case CHIP_OLAND:
   case CHIP_BONAIRE:
   case CHIP_KAVERI:
      ws->info.max_tcc_blocks = 4;
      case CHIP_PITCAIRN:
      ws->info.max_tcc_blocks = 8;
      case CHIP_TAHITI:
      ws->info.max_tcc_blocks = 12;
      case CHIP_HAWAII:
      ws->info.max_tcc_blocks = 16;
      default:
      ws->info.max_tcc_blocks = 0;
               if (!ws->info.max_se) {
      switch (ws->info.family) {
   default:
      ws->info.max_se = 1;
      case CHIP_CYPRESS:
   case CHIP_HEMLOCK:
   case CHIP_BARTS:
   case CHIP_CAYMAN:
   case CHIP_TAHITI:
   case CHIP_PITCAIRN:
   case CHIP_BONAIRE:
      ws->info.max_se = 2;
      case CHIP_HAWAII:
      ws->info.max_se = 4;
                           radeon_get_drm_value(ws->fd, RADEON_INFO_MAX_SH_PER_SE, NULL,
         if (ws->gen == DRV_SI) {
      ws->info.max_good_cu_per_sa =
   ws->info.min_good_cu_per_sa = ws->info.num_cu /
               radeon_get_drm_value(ws->fd, RADEON_INFO_ACCEL_WORKING2, NULL,
         if (ws->info.family == CHIP_HAWAII && ws->accel_working2 < 2) {
      fprintf(stderr, "radeon: GPU acceleration for Hawaii disabled, "
                                 if (ws->info.gfx_level == GFX7) {
      if (!radeon_get_drm_value(ws->fd, RADEON_INFO_CIK_MACROTILE_MODE_ARRAY, NULL,
            fprintf(stderr, "radeon: Kernel 3.13 is required for Sea Islands support.\n");
                  if (ws->info.gfx_level >= GFX6) {
      if (!radeon_get_drm_value(ws->fd, RADEON_INFO_SI_TILE_MODE_ARRAY, NULL,
            fprintf(stderr, "radeon: Kernel 3.10 is required for Southern Islands support.\n");
                  for (unsigned ip_type = 0; ip_type < AMD_NUM_IP_TYPES; ip_type++)
            /* Hawaii with old firmware needs type2 nop packet.
   * accel_working2 with value 3 indicates the new firmware.
   */
   ws->info.gfx_ib_pad_with_type2 = ws->info.gfx_level <= GFX6 ||
               ws->info.tcc_cache_line_size = 64; /* TC L2 line size on GCN */
   ws->info.has_bo_metadata = false;
   ws->info.has_eqaa_surface_allocator = false;
   ws->info.has_sparse_vm_mappings = false;
   ws->info.max_alignment = 1024*1024;
   ws->info.has_graphics = true;
   ws->info.cpdma_prefetch_writes_memory = true;
   ws->info.max_wave64_per_simd = 10;
   ws->info.num_physical_sgprs_per_simd = 512;
   ws->info.num_physical_wave64_vgprs_per_simd = 256;
   ws->info.has_3d_cube_border_color_mipmap = true;
   ws->info.has_image_opcodes = true;
   ws->info.spi_cu_en_has_effect = false;
   ws->info.spi_cu_en = 0xffff;
   ws->info.never_stop_sq_perf_counters = false;
   ws->info.num_rb = util_bitcount64(ws->info.enabled_rb_mask);
   ws->info.max_gflops = 128 * ws->info.num_cu * ws->info.max_gpu_freq_mhz / 1000;
   ws->info.num_tcc_blocks = ws->info.max_tcc_blocks;
   ws->info.tcp_cache_size = 16 * 1024;
   ws->info.num_simd_per_compute_unit = 4;
   ws->info.min_sgpr_alloc = 8;
   ws->info.max_sgpr_alloc = 104;
   ws->info.sgpr_alloc_granularity = 8;
   ws->info.min_wave64_vgpr_alloc = 4;
   ws->info.max_vgpr_alloc = 256;
            for (unsigned se = 0; se < ws->info.max_se; se++) {
      for (unsigned sa = 0; sa < ws->info.max_sa_per_se; sa++)
               /* The maximum number of scratch waves. The number is only a function of the number of CUs.
   * It should be large enough to hold at least 1 threadgroup. Use the minimum per-SA CU count.
   */
   const unsigned max_waves_per_tg = 1024 / 64; /* LLVM only supports 1024 threads per block */
   ws->info.max_scratch_waves = MAX2(32 * ws->info.min_good_cu_per_sa * ws->info.max_sa_per_se *
            switch (ws->info.family) {
   case CHIP_TAHITI:
   case CHIP_PITCAIRN:
   case CHIP_OLAND:
   case CHIP_HAWAII:
   case CHIP_KABINI:
      ws->info.l2_cache_size = ws->info.num_tcc_blocks * 64 * 1024;
      case CHIP_VERDE:
   case CHIP_HAINAN:
   case CHIP_BONAIRE:
   case CHIP_KAVERI:
      ws->info.l2_cache_size = ws->info.num_tcc_blocks * 128 * 1024;
      default:;
                     switch (ws->info.gfx_level) {
   case R300:
   case R400:
   case R500:
      ws->info.ip[AMD_IP_GFX].ver_major = 2;
      case R600:
   case R700:
      ws->info.ip[AMD_IP_GFX].ver_major = 3;
      case EVERGREEN:
      ws->info.ip[AMD_IP_GFX].ver_major = 4;
      case CAYMAN:
      ws->info.ip[AMD_IP_GFX].ver_major = 5;
      case GFX6:
      ws->info.ip[AMD_IP_GFX].ver_major = 6;
      case GFX7:
      ws->info.ip[AMD_IP_GFX].ver_major = 7;
      default:;
            ws->check_vm = strstr(debug_get_option("R600_DEBUG", ""), "check_vm") != NULL ||
                     }
      static void radeon_winsys_destroy(struct radeon_winsys *rws)
   {
               if (util_queue_is_initialized(&ws->cs_queue))
            mtx_destroy(&ws->hyperz_owner_mutex);
            if (ws->info.r600_has_virtual_memory)
                  if (ws->gen >= DRV_R600) {
                  _mesa_hash_table_destroy(ws->bo_names, NULL);
   _mesa_hash_table_destroy(ws->bo_handles, NULL);
   _mesa_hash_table_u64_destroy(ws->bo_vas);
   mtx_destroy(&ws->bo_handles_mutex);
   mtx_destroy(&ws->vm32.mutex);
   mtx_destroy(&ws->vm64.mutex);
            if (ws->fd >= 0)
               }
      static void radeon_query_info(struct radeon_winsys *rws, struct radeon_info *info)
   {
         }
      static bool radeon_cs_request_feature(struct radeon_cmdbuf *rcs,
               {
               switch (fid) {
   case RADEON_FID_R300_HYPERZ_ACCESS:
      return radeon_set_fd_access(cs, &cs->ws->hyperz_owner,
                     case RADEON_FID_R300_CMASK_ACCESS:
      return radeon_set_fd_access(cs, &cs->ws->cmask_owner,
                  }
      }
      uint32_t radeon_drm_get_gpu_reset_counter(struct radeon_drm_winsys *ws)
   {
               radeon_get_drm_value(ws->fd, RADEON_INFO_GPU_RESET_COUNTER,
            }
      static uint64_t radeon_query_value(struct radeon_winsys *rws,
         {
      struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;
            switch (value) {
   case RADEON_REQUESTED_VRAM_MEMORY:
         case RADEON_REQUESTED_GTT_MEMORY:
         case RADEON_MAPPED_VRAM:
         case RADEON_MAPPED_GTT:
         case RADEON_BUFFER_WAIT_TIME_NS:
         case RADEON_NUM_MAPPED_BUFFERS:
         case RADEON_TIMESTAMP:
      if (ws->gen < DRV_R600) {
      assert(0);
               radeon_get_drm_value(ws->fd, RADEON_INFO_TIMESTAMP, "timestamp",
            case RADEON_NUM_GFX_IBS:
         case RADEON_NUM_SDMA_IBS:
         case RADEON_NUM_BYTES_MOVED:
      radeon_get_drm_value(ws->fd, RADEON_INFO_NUM_BYTES_MOVED,
            case RADEON_NUM_EVICTIONS:
   case RADEON_NUM_VRAM_CPU_PAGE_FAULTS:
   case RADEON_VRAM_VIS_USAGE:
   case RADEON_GFX_BO_LIST_COUNTER:
   case RADEON_GFX_IB_SIZE_COUNTER:
   case RADEON_SLAB_WASTED_VRAM:
   case RADEON_SLAB_WASTED_GTT:
         case RADEON_VRAM_USAGE:
      radeon_get_drm_value(ws->fd, RADEON_INFO_VRAM_USAGE,
            case RADEON_GTT_USAGE:
      radeon_get_drm_value(ws->fd, RADEON_INFO_GTT_USAGE,
            case RADEON_GPU_TEMPERATURE:
      radeon_get_drm_value(ws->fd, RADEON_INFO_CURRENT_GPU_TEMP,
            case RADEON_CURRENT_SCLK:
      radeon_get_drm_value(ws->fd, RADEON_INFO_CURRENT_GPU_SCLK,
            case RADEON_CURRENT_MCLK:
      radeon_get_drm_value(ws->fd, RADEON_INFO_CURRENT_GPU_MCLK,
            case RADEON_CS_THREAD_TIME:
         }
      }
      static bool radeon_read_registers(struct radeon_winsys *rws,
               {
      struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;
            for (i = 0; i < num_registers; i++) {
               if (!radeon_get_drm_value(ws->fd, RADEON_INFO_READ_REG, NULL, &reg))
            }
      }
      DEBUG_GET_ONCE_BOOL_OPTION(thread, "RADEON_THREAD", true)
      static bool radeon_winsys_unref(struct radeon_winsys *ws)
   {
      struct radeon_drm_winsys *rws = (struct radeon_drm_winsys*)ws;
            /* When the reference counter drops to zero, remove the fd from the table.
   * This must happen while the mutex is locked, so that
   * radeon_drm_winsys_create in another thread doesn't get the winsys
   * from the table when the counter drops to 0. */
            destroy = pipe_reference(&rws->reference, NULL);
   if (destroy && fd_tab) {
      _mesa_hash_table_remove_key(fd_tab, intptr_to_pointer(rws->fd));
   if (_mesa_hash_table_num_entries(fd_tab) == 0) {
      _mesa_hash_table_destroy(fd_tab, NULL);
                  simple_mtx_unlock(&fd_tab_mutex);
      }
      static void radeon_pin_threads_to_L3_cache(struct radeon_winsys *ws,
         {
               if (util_queue_is_initialized(&rws->cs_queue)) {
      util_set_thread_affinity(rws->cs_queue.threads[0],
               }
      static bool radeon_cs_is_secure(struct radeon_cmdbuf* cs)
   {
         }
      static bool radeon_cs_set_pstate(struct radeon_cmdbuf* cs, enum radeon_ctx_pstate state)
   {
         }
      static int
   radeon_drm_winsys_get_fd(struct radeon_winsys *ws)
   {
                  }
      PUBLIC struct radeon_winsys *
   radeon_drm_winsys_create(int fd, const struct pipe_screen_config *config,
         {
               simple_mtx_lock(&fd_tab_mutex);
   if (!fd_tab) {
                  ws = util_hash_table_get(fd_tab, intptr_to_pointer(fd));
   if (ws) {
      pipe_reference(NULL, &ws->reference);
   simple_mtx_unlock(&fd_tab_mutex);
               ws = CALLOC_STRUCT(radeon_drm_winsys);
   if (!ws) {
      simple_mtx_unlock(&fd_tab_mutex);
                        if (!do_winsys_init(ws))
            pb_cache_init(&ws->bo_cache, RADEON_NUM_HEAPS,
               500000, ws->check_vm ? 1.0f : 2.0f, 0,
            if (ws->info.r600_has_virtual_memory) {
      /* There is no fundamental obstacle to using slab buffer allocation
   * without GPUVM, but enabling it requires making sure that the drivers
   * honor the address offset.
   */
   if (!pb_slabs_init(&ws->bo_slabs,
                     RADEON_SLAB_MIN_SIZE_LOG2, RADEON_SLAB_MAX_SIZE_LOG2,
   RADEON_NUM_HEAPS, false,
   ws,
               } else {
                  if (ws->gen >= DRV_R600) {
      ws->surf_man = radeon_surface_manager_new(ws->fd);
   if (!ws->surf_man)
               /* init reference */
            /* Set functions. */
   ws->base.unref = radeon_winsys_unref;
   ws->base.destroy = radeon_winsys_destroy;
   ws->base.get_fd = radeon_drm_winsys_get_fd;
   ws->base.query_info = radeon_query_info;
   ws->base.pin_threads_to_L3_cache = radeon_pin_threads_to_L3_cache;
   ws->base.cs_request_feature = radeon_cs_request_feature;
   ws->base.query_value = radeon_query_value;
   ws->base.read_registers = radeon_read_registers;
   ws->base.cs_is_secure = radeon_cs_is_secure;
            radeon_drm_bo_init_functions(ws);
   radeon_drm_cs_init_functions(ws);
            (void) mtx_init(&ws->hyperz_owner_mutex, mtx_plain);
            ws->bo_names = util_hash_table_create_ptr_keys();
   ws->bo_handles = util_hash_table_create_ptr_keys();
   ws->bo_vas = _mesa_hash_table_u64_create(NULL);
   (void) mtx_init(&ws->bo_handles_mutex, mtx_plain);
   (void) mtx_init(&ws->vm32.mutex, mtx_plain);
   (void) mtx_init(&ws->vm64.mutex, mtx_plain);
   (void) mtx_init(&ws->bo_fence_lock, mtx_plain);
   list_inithead(&ws->vm32.holes);
            /* The kernel currently returns 8MB. Make sure this doesn't change. */
   if (ws->va_start > 8 * 1024 * 1024) {
      /* Not enough 32-bit address space. */
   radeon_winsys_destroy(&ws->base);
   simple_mtx_unlock(&fd_tab_mutex);
               ws->vm32.start = ws->va_start;
            ws->vm64.start = 1ull << 32;
            /* TTM aligns the BO size to the CPU page size */
   ws->info.gart_page_size = sysconf(_SC_PAGESIZE);
            if (ws->num_cpus > 1 && debug_get_option_thread())
            /* Create the screen at the end. The winsys must be initialized
   * completely.
   *
   * Alternatively, we could create the screen based on "ws->gen"
   * and link all drivers into one binary blob. */
   ws->base.screen = screen_create(&ws->base, config);
   if (!ws->base.screen) {
      radeon_winsys_destroy(&ws->base);
   simple_mtx_unlock(&fd_tab_mutex);
                        /* We must unlock the mutex once the winsys is fully initialized, so that
   * other threads attempting to create the winsys from the same fd will
   * get a fully initialized winsys and not just half-way initialized. */
                  fail_slab:
      if (ws->info.r600_has_virtual_memory)
      fail_cache:
         fail1:
      simple_mtx_unlock(&fd_tab_mutex);
   if (ws->surf_man)
         if (ws->fd >= 0)
            FREE(ws);
      }
