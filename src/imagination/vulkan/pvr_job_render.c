   /*
   * Copyright © 2022 Imagination Technologies Ltd.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a copy
   * of this software and associated documentation files (the "Software"), to deal
   * in the Software without restriction, including without limitation the rights
   * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   * copies of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <stdint.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_defs.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_csb.h"
   #include "pvr_debug.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_debug.h"
   #include "pvr_job_common.h"
   #include "pvr_job_context.h"
   #include "pvr_job_render.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_rogue_fw.h"
   #include "pvr_types.h"
   #include "pvr_winsys.h"
   #include "util/compiler.h"
   #include "util/format/format_utils.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
   #include "vk_util.h"
      #define ROGUE_BIF_PM_FREELIST_BASE_ADDR_ALIGNSIZE 16U
      /* FIXME: Is there a hardware define we can use instead? */
   /* 1 DWord per PM physical page stored in the free list */
   #define ROGUE_FREE_LIST_ENTRY_SIZE ((uint32_t)sizeof(uint32_t))
      /* FIXME: The three defines below, for the number of PC, PD and PT entries in a
   * 4KB page, come from rgxmmudefs_km.h (meaning they're part of the
   * auto-generated hwdefs). Should these be defined in rogue_mmu.xml? Keeping in
   * mind that we probably only need these three values. */
   #define ROGUE_NUM_PC_ENTRIES_PER_PAGE 0x400U
      #define ROGUE_NUM_PD_ENTRIES_PER_PAGE 0x200U
      #define ROGUE_NUM_PT_ENTRIES_PER_PAGE 0x200U
      struct pvr_free_list {
                                    };
      struct pvr_rt_dataset {
               /* RT dataset information */
   uint32_t width;
   uint32_t height;
   uint32_t samples;
            struct pvr_free_list *global_free_list;
            struct pvr_bo *vheap_rtc_bo;
   pvr_dev_addr_t vheap_dev_addr;
            struct pvr_bo *tpc_bo;
   uint64_t tpc_stride;
                     /* RT data information */
            struct pvr_bo *rgn_headers_bo;
                              struct {
      pvr_dev_addr_t mta_dev_addr;
   pvr_dev_addr_t mlist_dev_addr;
         };
      VkResult pvr_free_list_create(struct pvr_device *device,
                                 uint32_t initial_size,
   {
      const struct pvr_device_runtime_info *runtime_info =
         struct pvr_winsys_free_list *parent_ws_free_list =
         const uint64_t bo_flags = PVR_BO_ALLOC_FLAG_GPU_UNCACHED |
         struct pvr_free_list *free_list;
   uint32_t cache_line_size;
   uint32_t initial_num_pages;
   uint32_t grow_num_pages;
   uint32_t max_num_pages;
   uint64_t addr_alignment;
   uint64_t size_alignment;
   uint64_t size;
            assert((initial_size + grow_size) <= max_size);
   assert(max_size != 0);
            /* Make sure the free list is created with at least a single page. */
   if (initial_size == 0)
            /* The freelists sizes must respect the PM freelist base address alignment
   * requirement. As the freelist entries are cached by the SLC, it's also
   * necessary to ensure the sizes respect the SLC cache line size to avoid
   * invalid entries appearing in the cache, which would be problematic after
   * a grow operation, as the SLC entries aren't invalidated. We do this by
   * making sure the freelist values are appropriately aligned.
   *
   * To calculate the alignment, we first take the largest of the freelist
   * base address alignment and the SLC cache line size. We then divide this
   * by the freelist entry size to determine the number of freelist entries
   * required by the PM. Finally, as each entry holds a single PM physical
   * page, we multiple the number of entries by the page size.
   *
   * As an example, if the base address alignment is 16 bytes, the SLC cache
   * line size is 64 bytes and the freelist entry size is 4 bytes then 16
   * entries are required, as we take the SLC cacheline size (being the larger
   * of the two values) and divide this by 4. If the PM page size is 4096
   * bytes then we end up with an alignment of 65536 bytes.
   */
            addr_alignment =
         size_alignment = (addr_alignment / ROGUE_FREE_LIST_ENTRY_SIZE) *
                     initial_size = align64(initial_size, size_alignment);
   max_size = align64(max_size, size_alignment);
            /* Make sure the 'max' size doesn't exceed what the firmware supports and
   * adjust the other sizes accordingly.
   */
   if (max_size > runtime_info->max_free_list_size) {
      max_size = runtime_info->max_free_list_size;
               if (initial_size > max_size)
            if (initial_size == max_size)
            initial_num_pages = initial_size >> ROGUE_BIF_PM_PHYSICAL_PAGE_SHIFT;
   max_num_pages = max_size >> ROGUE_BIF_PM_PHYSICAL_PAGE_SHIFT;
            /* Calculate the size of the buffer needed to store the free list entries
   * based on the maximum number of pages we can have.
   */
   size = max_num_pages * ROGUE_FREE_LIST_ENTRY_SIZE;
            free_list = vk_alloc(&device->vk.alloc,
                     if (!free_list)
            /* FIXME: The memory is mapped GPU uncached, but this seems to contradict
   * the comment above about aligning to the SLC cache line size.
   */
   result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
            result = device->ws->ops->free_list_create(device->ws,
                                             if (result != VK_SUCCESS)
            free_list->device = device;
                           err_pvr_bo_free_bo:
            err_vk_free_free_list:
                  }
      void pvr_free_list_destroy(struct pvr_free_list *free_list)
   {
               device->ws->ops->free_list_destroy(free_list->ws_free_list);
   pvr_bo_free(device, free_list->bo);
      }
      static inline void pvr_get_samples_in_xy(uint32_t samples,
               {
      switch (samples) {
   case 1:
      *x_out = 1;
   *y_out = 1;
      case 2:
      *x_out = 1;
   *y_out = 2;
      case 4:
      *x_out = 2;
   *y_out = 2;
      case 8:
      *x_out = 2;
   *y_out = 4;
      default:
            }
      void pvr_rt_mtile_info_init(const struct pvr_device_info *dev_info,
                           {
      uint32_t samples_in_x;
                     info->tile_size_x = PVR_GET_FEATURE_VALUE(dev_info, tile_size_x, 1);
            info->num_tiles_x = DIV_ROUND_UP(width, info->tile_size_x);
                     if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      assert(PVR_GET_FEATURE_VALUE(dev_info,
               /* Set up 16 macrotiles with a multiple of 2x2 tiles per macrotile,
   * which is aligned to a tile group.
   */
   info->mtile_x1 = DIV_ROUND_UP(info->num_tiles_x, 8) * 2;
   info->mtile_y1 = DIV_ROUND_UP(info->num_tiles_y, 8) * 2;
   info->mtile_x2 = 0;
   info->mtile_y2 = 0;
   info->mtile_x3 = 0;
   info->mtile_y3 = 0;
   info->x_tile_max = ALIGN_POT(info->num_tiles_x, 2) - 1;
      } else {
      /* Set up 16 macrotiles with a multiple of 4x4 tiles per macrotile. */
   info->mtile_x1 = ALIGN_POT(DIV_ROUND_UP(info->num_tiles_x, 4), 4);
   info->mtile_y1 = ALIGN_POT(DIV_ROUND_UP(info->num_tiles_y, 4), 4);
   info->mtile_x2 = info->mtile_x1 * 2;
   info->mtile_y2 = info->mtile_y1 * 2;
   info->mtile_x3 = info->mtile_x1 * 3;
   info->mtile_y3 = info->mtile_y1 * 3;
   info->x_tile_max = info->num_tiles_x - 1;
               info->tiles_per_mtile_x = info->mtile_x1 * samples_in_x;
      }
      /* Note that the unit of the return value depends on the GPU. For cores with the
   * simple_internal_parameter_format feature the returned size is interpreted as
   * the number of region headers. For cores without this feature its interpreted
   * as the size in dwords.
   */
   static uint64_t
   pvr_rt_get_isp_region_size(struct pvr_device *device,
         {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   uint64_t rgn_size =
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
                        if (PVR_FEATURE_VALUE(dev_info,
                              if (version == 2) {
      /* One region header per 2x2 tile group. */
         } else {
      const uint64_t single_rgn_header_size =
            /* Round up to next dword to prevent IPF overrun and convert to bytes.
   */
                  }
      static VkResult pvr_rt_vheap_rtc_data_init(struct pvr_device *device,
               {
      uint64_t vheap_size;
   uint32_t alignment;
   uint64_t rtc_size;
                     if (layers > 1) {
                        rtc_entries = ROGUE_NUM_TEAC + ROGUE_NUM_TE + ROGUE_NUM_VCE;
   if (PVR_HAS_QUIRK(&device->pdevice->dev_info, 48545))
               } else {
                  alignment = MAX2(PVRX(CR_PM_VHEAP_TABLE_BASE_ADDR_ALIGNMENT),
            result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
                     if (rtc_size > 0) {
      rt_dataset->rtc_dev_addr =
      } else {
                     }
      static void pvr_rt_vheap_rtc_data_fini(struct pvr_rt_dataset *rt_dataset)
   {
               pvr_bo_free(rt_dataset->device, rt_dataset->vheap_rtc_bo);
      }
      static void
   pvr_rt_get_tail_ptr_stride_size(const struct pvr_device *device,
                           {
      uint32_t max_num_mtiles;
   uint32_t num_mtiles_x;
   uint32_t num_mtiles_y;
   uint32_t version;
            num_mtiles_x = mtile_info->mtiles_x * mtile_info->tiles_per_mtile_x;
            max_num_mtiles = MAX2(util_next_power_of_two64(num_mtiles_x),
                     if (PVR_FEATURE_VALUE(&device->pdevice->dev_info,
                              if (version == 2) {
      /* One tail pointer cache entry per 2x2 tile group. */
                        if (layers > 1) {
               *stride_out = size / ROGUE_BIF_PM_PHYSICAL_PAGE_SIZE;
      } else {
      *stride_out = 0;
         }
      static VkResult pvr_rt_tpc_data_init(struct pvr_device *device,
                     {
               pvr_rt_get_tail_ptr_stride_size(device,
                                    return pvr_bo_alloc(device,
                     device->heaps.general_heap,
      }
      static void pvr_rt_tpc_data_fini(struct pvr_rt_dataset *rt_dataset)
   {
      pvr_bo_free(rt_dataset->device, rt_dataset->tpc_bo);
      }
      static uint32_t
   pvr_rt_get_mlist_size(const struct pvr_free_list *global_free_list,
         {
      uint32_t num_pte_pages;
   uint32_t num_pde_pages;
   uint32_t num_pce_pages;
   uint64_t total_pages;
            assert(global_free_list->size + local_free_list->size <=
            total_pages = (global_free_list->size + local_free_list->size) >>
            /* Calculate the total number of physical pages required to hold the page
   * table, directory and catalog entries for the freelist pages.
   */
   num_pte_pages = DIV_ROUND_UP(total_pages, ROGUE_NUM_PT_ENTRIES_PER_PAGE);
   num_pde_pages = DIV_ROUND_UP(num_pte_pages, ROGUE_NUM_PD_ENTRIES_PER_PAGE);
            /* Calculate the MList size considering the total number of pages in the PB
   * are shared among all the PM address spaces.
   */
   mlist_size = (num_pce_pages + num_pde_pages + num_pte_pages) *
               }
      static void pvr_rt_get_region_headers_stride_size(
      const struct pvr_device *device,
   const struct pvr_rt_mtile_info *mtile_info,
   uint32_t layers,
   uint64_t *const stride_out,
      {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t single_rgn_header_size =
         uint64_t rgn_headers_size;
   uint32_t num_tiles_x;
   uint32_t num_tiles_y;
   uint32_t group_size;
            if (PVR_FEATURE_VALUE(dev_info, simple_parameter_format_version, &version))
                     num_tiles_x = mtile_info->mtiles_x * mtile_info->tiles_per_mtile_x;
            rgn_headers_size = (uint64_t)num_tiles_x / group_size;
   /* Careful here. We want the division to happen first. */
   rgn_headers_size *= num_tiles_y / group_size;
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      rgn_headers_size =
               if (layers > 1) {
      rgn_headers_size =
               *stride_out = rgn_headers_size;
      }
      static VkResult
   pvr_rt_mta_mlist_data_init(struct pvr_device *device,
                           {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   const uint32_t mlist_size =
         uint32_t mta_size = rogue_get_macrotile_array_size(dev_info);
   const uint32_t num_rt_datas = ARRAY_SIZE(rt_dataset->rt_datas);
   uint32_t rt_datas_mlist_size;
   uint32_t rt_datas_mta_size;
   pvr_dev_addr_t dev_addr;
            /* Allocate memory for macrotile array and Mlist for all RT datas.
   *
   * Allocation layout: MTA[0..N] + Mlist alignment padding + Mlist[0..N].
   *
   * N is number of RT datas.
   */
   rt_datas_mta_size = ALIGN_POT(mta_size * num_rt_datas,
                  result = pvr_bo_alloc(device,
                        device->heaps.general_heap,
      if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < num_rt_datas; i++) {
      if (mta_size != 0) {
      rt_dataset->rt_datas[i].mta_dev_addr = dev_addr;
      } else {
                     dev_addr = PVR_DEV_ADDR_OFFSET(rt_dataset->mta_mlist_bo->vma->dev_addr,
            for (uint32_t i = 0; i < num_rt_datas; i++) {
      if (mlist_size != 0) {
      rt_dataset->rt_datas[i].mlist_dev_addr = dev_addr;
      } else {
                        }
      static void pvr_rt_mta_mlist_data_fini(struct pvr_rt_dataset *rt_dataset)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(rt_dataset->rt_datas); i++) {
      rt_dataset->rt_datas[i].mlist_dev_addr = PVR_DEV_ADDR_INVALID;
               pvr_bo_free(rt_dataset->device, rt_dataset->mta_mlist_bo);
      }
      static VkResult
   pvr_rt_rgn_headers_data_init(struct pvr_device *device,
                     {
      const uint32_t num_rt_datas = ARRAY_SIZE(rt_dataset->rt_datas);
   uint64_t rgn_headers_size;
   pvr_dev_addr_t dev_addr;
            pvr_rt_get_region_headers_stride_size(device,
                              result = pvr_bo_alloc(device,
                        device->heaps.rgn_hdr_heap,
      if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < num_rt_datas; i++) {
      rt_dataset->rt_datas[i].rgn_headers_dev_addr = dev_addr;
                  }
      static void pvr_rt_rgn_headers_data_fini(struct pvr_rt_dataset *rt_dataset)
   {
      for (uint32_t i = 0; i < ARRAY_SIZE(rt_dataset->rt_datas); i++)
            pvr_bo_free(rt_dataset->device, rt_dataset->rgn_headers_bo);
      }
      static VkResult pvr_rt_datas_init(struct pvr_device *device,
                                 {
               result = pvr_rt_mta_mlist_data_init(device,
                           if (result != VK_SUCCESS)
            result =
         if (result != VK_SUCCESS)
                  err_pvr_rt_mta_mlist_data_fini:
                  }
      static void pvr_rt_datas_fini(struct pvr_rt_dataset *rt_dataset)
   {
      pvr_rt_rgn_headers_data_fini(rt_dataset);
      }
      static void pvr_rt_dataset_ws_create_info_init(
      struct pvr_rt_dataset *rt_dataset,
   const struct pvr_rt_mtile_info *mtile_info,
      {
      struct pvr_device *device = rt_dataset->device;
                     /* Local freelist. */
            create_info->width = rt_dataset->width;
   create_info->height = rt_dataset->height;
   create_info->samples = rt_dataset->samples;
            /* ISP register values. */
   if (PVR_HAS_ERN(dev_info, 42307) &&
      !(PVR_HAS_FEATURE(dev_info, roguexe) && mtile_info->tile_size_x == 16)) {
            if (rt_dataset->width != 0) {
      value =
                  value =
                     if (rt_dataset->height != 0) {
      value =
                  value =
                     value = ((float)rt_dataset->width * ROGUE_ISP_MERGE_SCALE_FACTOR) /
         (ROGUE_ISP_MERGE_UPPER_LIMIT_NUMERATOR -
            value = ((float)rt_dataset->height * ROGUE_ISP_MERGE_SCALE_FACTOR) /
         (ROGUE_ISP_MERGE_UPPER_LIMIT_NUMERATOR -
               /* Allocations and associated information. */
   create_info->vheap_table_dev_addr = rt_dataset->vheap_dev_addr;
            create_info->tpc_dev_addr = rt_dataset->tpc_bo->vma->dev_addr;
   create_info->tpc_stride = rt_dataset->tpc_stride;
            STATIC_ASSERT(ARRAY_SIZE(create_info->rt_datas) ==
         for (uint32_t i = 0; i < ARRAY_SIZE(create_info->rt_datas); i++) {
      create_info->rt_datas[i].pm_mlist_dev_addr =
         create_info->rt_datas[i].macrotile_array_dev_addr =
         create_info->rt_datas[i].rgn_header_dev_addr =
               create_info->rgn_header_size =
      }
      VkResult
   pvr_render_target_dataset_create(struct pvr_device *device,
                                 {
      struct pvr_device_runtime_info *runtime_info =
         const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   struct pvr_winsys_rt_dataset_create_info rt_dataset_create_info;
   struct pvr_rt_mtile_info mtile_info;
   struct pvr_rt_dataset *rt_dataset;
            assert(device->global_free_list);
   assert(width <= rogue_get_render_size_max_x(dev_info));
   assert(height <= rogue_get_render_size_max_y(dev_info));
                     rt_dataset = vk_zalloc(&device->vk.alloc,
                     if (!rt_dataset)
            rt_dataset->device = device;
   rt_dataset->width = width;
   rt_dataset->height = height;
   rt_dataset->samples = samples;
   rt_dataset->layers = layers;
            /* The maximum supported free list size is based on the assumption that this
   * freelist (the "local" freelist) is always the minimum size required by
   * the hardware. See the documentation of ROGUE_FREE_LIST_MAX_SIZE for more
   * details.
   */
   result = pvr_free_list_create(device,
                                 runtime_info->min_free_list_size,
   if (result != VK_SUCCESS)
            result = pvr_rt_vheap_rtc_data_init(device, rt_dataset, layers);
   if (result != VK_SUCCESS)
            result = pvr_rt_tpc_data_init(device, rt_dataset, &mtile_info, layers);
   if (result != VK_SUCCESS)
            result = pvr_rt_datas_init(device,
                                 if (result != VK_SUCCESS)
            /* rt_dataset must be fully initialized by this point since
   * pvr_rt_dataset_ws_create_info_init() depends on this.
   */
   pvr_rt_dataset_ws_create_info_init(rt_dataset,
                  result =
      device->ws->ops->render_target_dataset_create(device->ws,
                  if (result != VK_SUCCESS)
                           err_pvr_rt_datas_fini:
            err_pvr_rt_tpc_data_fini:
            err_pvr_rt_vheap_rtc_data_fini:
            err_pvr_free_list_destroy:
            err_vk_free_rt_dataset:
                  }
      void pvr_render_target_dataset_destroy(struct pvr_rt_dataset *rt_dataset)
   {
                        pvr_rt_datas_fini(rt_dataset);
   pvr_rt_tpc_data_fini(rt_dataset);
                        }
      static void pvr_geom_state_stream_init(struct pvr_render_ctx *ctx,
               {
      const struct pvr_device *const device = ctx->device;
            uint32_t *stream_ptr = (uint32_t *)state->fw_stream;
            /* Leave space for stream header. */
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_VDM_CTRL_STREAM_BASE, value) {
         }
            pvr_csb_pack ((uint64_t *)stream_ptr,
                  value.border_colour_table_address =
      }
            pvr_csb_pack (stream_ptr, CR_PPP_CTRL, value) {
      value.wclampen = true;
      }
            pvr_csb_pack (stream_ptr, CR_TE_PSG, value) {
               value.region_stride = job->rt_dataset->rgn_headers_stride /
               }
            /* Set up the USC common size for the context switch resume/load program
   * (ctx->ctx_switch.programs[i].sr->pds_load_program), which was created
   * as part of the render context.
   */
   pvr_csb_pack (stream_ptr, VDMCTRL_PDS_STATE0, value) {
      /* Calculate the size in bytes. */
            value.usc_common_size =
      DIV_ROUND_UP(shared_registers_size,
   }
            /* clang-format off */
   pvr_csb_pack (stream_ptr, KMD_STREAM_VIEW_IDX, value);
   /* clang-format on */
            state->fw_stream_len = (uint8_t *)stream_ptr - (uint8_t *)state->fw_stream;
            pvr_csb_pack ((uint64_t *)stream_len_ptr, KMD_STREAM_HDR, value) {
            }
      static void
   pvr_geom_state_stream_ext_init(struct pvr_render_ctx *ctx,
               {
               uint32_t main_stream_len =
         uint32_t *ext_stream_ptr =
                  header0_ptr = ext_stream_ptr;
            pvr_csb_pack (header0_ptr, KMD_STREAM_EXTHDR_GEOM0, header0) {
      if (PVR_HAS_QUIRK(dev_info, 49927)) {
               /* The set up of CR_TPU must be identical to
   * pvr_render_job_ws_fragment_state_stream_ext_init().
   */
   pvr_csb_pack (ext_stream_ptr, CR_TPU, value) {
         }
                  if ((*header0_ptr & PVRX(KMD_STREAM_EXTHDR_DATA_MASK)) != 0) {
      state->fw_stream_len =
               }
      static void
   pvr_geom_state_flags_init(const struct pvr_render_job *const job,
         {
      *flags = (struct pvr_winsys_geometry_state_flags){
      .is_first_geometry = !job->rt_dataset->need_frag,
   .is_last_geometry = job->geometry_terminate,
         }
      static void
   pvr_render_job_ws_geometry_state_init(struct pvr_render_ctx *ctx,
                     {
      pvr_geom_state_stream_init(ctx, job, state);
            state->wait = wait;
      }
      static inline uint32_t pvr_frag_km_stream_pbe_reg_words_offset(
         {
               offset += pvr_cmd_length(KMD_STREAM_HDR);
   offset += pvr_cmd_length(CR_ISP_SCISSOR_BASE);
   offset += pvr_cmd_length(CR_ISP_DBIAS_BASE);
   offset += pvr_cmd_length(CR_ISP_OCLQRY_BASE);
   offset += pvr_cmd_length(CR_ISP_ZLSCTL);
   offset += pvr_cmd_length(CR_ISP_ZLOAD_BASE);
            if (PVR_HAS_FEATURE(dev_info, requires_fb_cdc_zls_setup))
               }
      #define DWORDS_PER_U64 2
      static inline uint32_t pvr_frag_km_stream_pds_eot_data_addr_offset(
         {
               offset += pvr_frag_km_stream_pbe_reg_words_offset(dev_info) / 4U;
   offset +=
         offset += pvr_cmd_length(CR_TPU_BORDER_COLOUR_TABLE_PDM);
   offset += ROGUE_NUM_CR_PDS_BGRND_WORDS * DWORDS_PER_U64;
   offset += ROGUE_NUM_CR_PDS_BGRND_WORDS * DWORDS_PER_U64;
   offset += PVRX(KMD_STREAM_USC_CLEAR_REGISTER_COUNT) *
         offset += pvr_cmd_length(CR_USC_PIXEL_OUTPUT_CTRL);
   offset += pvr_cmd_length(CR_ISP_BGOBJDEPTH);
   offset += pvr_cmd_length(CR_ISP_BGOBJVALS);
   offset += pvr_cmd_length(CR_ISP_AA);
   offset += pvr_cmd_length(CR_ISP_CTL);
            if (PVR_HAS_FEATURE(dev_info, cluster_grouping))
                        }
      static void pvr_frag_state_stream_init(struct pvr_render_ctx *ctx,
               {
      const struct pvr_device *const device = ctx->device;
   const struct pvr_physical_device *const pdevice = device->pdevice;
   const struct pvr_device_runtime_info *dev_runtime_info =
         const struct pvr_device_info *dev_info = &pdevice->dev_info;
   const enum PVRX(CR_ISP_AA_MODE_TYPE)
            enum PVRX(CR_ZLS_FORMAT_TYPE) zload_format = PVRX(CR_ZLS_FORMAT_TYPE_F32Z);
   uint32_t *stream_ptr = (uint32_t *)state->fw_stream;
   uint32_t *stream_len_ptr = stream_ptr;
   uint32_t pixel_ctl;
            /* Leave space for stream header. */
            /* FIXME: pass in the number of samples rather than isp_aa_mode? */
   pvr_setup_tiles_in_flight(dev_info,
                           dev_runtime_info,
   isp_aa_mode,
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_SCISSOR_BASE, value) {
         }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_DBIAS_BASE, value) {
         }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_OCLQRY_BASE, value) {
      const struct pvr_sub_cmd_gfx *sub_cmd =
            if (sub_cmd->query_pool)
         else
      }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_ZLSCTL, value) {
      if (job->has_depth_attachment || job->has_stencil_attachment) {
                     if (job->ds.has_alignment_transfers) {
         } else {
      alignment_x = ROGUE_IPF_TILE_SIZE_PIXELS;
               rogue_get_isp_num_tiles_xy(
      dev_info,
   job->samples,
   ALIGN_POT(job->ds.physical_extent.width, alignment_x),
   ALIGN_POT(job->ds.physical_extent.height, alignment_y),
                              if (job->ds.memlayout == PVR_MEMLAYOUT_TWIDDLED &&
      !job->ds.has_alignment_transfers) {
   value.loadtwiddled = true;
                                          if (job->has_depth_attachment) {
      value.zloaden = job->ds.load.d;
               if (job->has_stencil_attachment) {
      value.sloaden = job->ds.load.s;
               value.forcezload = value.zloaden || value.sloaden;
      }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_ZLOAD_BASE, value) {
      if (job->has_depth_attachment)
      }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_ISP_STENCIL_LOAD_BASE, value) {
      if (job->has_stencil_attachment) {
               /* Enable separate stencil. This should be enabled iff the buffer set
   * in CR_ISP_STENCIL_LOAD_BASE does not contain a depth component.
   */
   assert(job->has_depth_attachment ||
               }
            if (PVR_HAS_FEATURE(dev_info, requires_fb_cdc_zls_setup)) {
      /* Currently no support for FBC, so just go ahead and set the default
   * values.
   */
   pvr_csb_pack ((uint64_t *)stream_ptr, CR_FB_CDC_ZLS, value) {
      value.fbdc_depth_fmt = PVRX(TEXSTATE_FORMAT_F32);
      }
               /* Make sure that the pvr_frag_km_...() function is returning the correct
   * offset.
   */
   assert((uint8_t *)stream_ptr - (uint8_t *)state->fw_stream ==
            STATIC_ASSERT(ARRAY_SIZE(job->pbe_reg_words) == PVR_MAX_COLOR_ATTACHMENTS);
   STATIC_ASSERT(ARRAY_SIZE(job->pbe_reg_words[0]) ==
         STATIC_ASSERT(sizeof(job->pbe_reg_words[0][0]) == sizeof(uint64_t));
   memcpy(stream_ptr, job->pbe_reg_words, sizeof(job->pbe_reg_words));
   stream_ptr +=
            pvr_csb_pack ((uint64_t *)stream_ptr,
                  value.border_colour_table_address =
      }
            STATIC_ASSERT(ARRAY_SIZE(job->pds_bgnd_reg_values) ==
         STATIC_ASSERT(sizeof(job->pds_bgnd_reg_values[0]) == sizeof(uint64_t));
   memcpy(stream_ptr,
         job->pds_bgnd_reg_values,
            STATIC_ASSERT(ARRAY_SIZE(job->pds_pr_bgnd_reg_values) ==
         STATIC_ASSERT(sizeof(job->pds_pr_bgnd_reg_values[0]) == sizeof(uint64_t));
   memcpy(stream_ptr,
         job->pds_pr_bgnd_reg_values,
         #undef DWORDS_PER_U64
         memset(stream_ptr,
         0,
   PVRX(KMD_STREAM_USC_CLEAR_REGISTER_COUNT) *
   stream_ptr += PVRX(KMD_STREAM_USC_CLEAR_REGISTER_COUNT) *
            *stream_ptr = pixel_ctl;
            pvr_csb_pack (stream_ptr, CR_ISP_BGOBJDEPTH, value) {
               /* This is valid even when we don't have a depth attachment because:
   *  - zload_format is set to a sensible default above, and
   *  - job->depth_clear_value is set to a sensible default in that case.
   */
   switch (zload_format) {
   case PVRX(CR_ZLS_FORMAT_TYPE_F32Z):
                  case PVRX(CR_ZLS_FORMAT_TYPE_16BITINT):
                  case PVRX(CR_ZLS_FORMAT_TYPE_24BITINT):
                  default:
            }
            pvr_csb_pack (stream_ptr, CR_ISP_BGOBJVALS, value) {
                           }
            pvr_csb_pack (stream_ptr, CR_ISP_AA, value) {
         }
            pvr_csb_pack (stream_ptr, CR_ISP_CTL, value) {
      value.sample_pos = true;
            /* For integer depth formats we'll convert the specified floating point
   * depth bias values and specify them as integers. In this mode a depth
   * bias factor of 1.0 equates to 1 ULP of increase to the depth value.
   */
   value.dbias_is_int = PVR_HAS_ERN(dev_info, 42307) &&
      }
   /* FIXME: When pvr_setup_tiles_in_flight() is refactored it might be
   * possible to fully pack CR_ISP_CTL above rather than having to OR in part
   * of the value.
   */
   *stream_ptr |= isp_ctl;
            pvr_csb_pack (stream_ptr, CR_EVENT_PIXEL_PDS_INFO, value) {
      value.const_size =
      DIV_ROUND_UP(ctx->device->pixel_event_data_size_in_dwords,
      value.temp_stride = 0;
   value.usc_sr_size =
      DIV_ROUND_UP(PVR_STATE_PBE_DWORDS,
   }
            if (PVR_HAS_FEATURE(dev_info, cluster_grouping)) {
      pvr_csb_pack (stream_ptr, KMD_STREAM_PIXEL_PHANTOM, value) {
      /* Each phantom has its own MCU, so atomicity can only be guaranteed
   * when all work items are processed on the same phantom. This means
   * we need to disable all USCs other than those of the first
   * phantom, which has 4 clusters. Note that we only need to do this
   * for atomic operations in fragment shaders, since hardware
   * prevents the TA to run on more than one phantom anyway.
   */
   /* Note that leaving all phantoms disabled (as csbgen will do by
   * default since it will zero out things) will set them to their
   * default state (i.e. enabled) instead of disabling them.
   */
   if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls) &&
      dev_runtime_info->num_phantoms > 1 && job->frag_uses_atomic_ops) {
         }
               /* clang-format off */
   pvr_csb_pack (stream_ptr, KMD_STREAM_VIEW_IDX, value);
   /* clang-format on */
            /* Make sure that the pvr_frag_km_...() function is returning the correct
   * offset.
   */
   assert((uint8_t *)stream_ptr - (uint8_t *)state->fw_stream ==
            pvr_csb_pack (stream_ptr, CR_EVENT_PIXEL_PDS_DATA, value) {
         }
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      pvr_finishme(
         *stream_ptr = 0;
               if (PVR_HAS_FEATURE(dev_info, zls_subtile)) {
      pvr_csb_pack (stream_ptr, CR_ISP_ZLS_PIXELS, value) {
      if (job->has_depth_attachment) {
      if (job->ds.has_alignment_transfers) {
      value.x = job->ds.physical_extent.width - 1;
      } else {
      value.x = job->ds.stride - 1;
            }
               /* zls_stride */
   *stream_ptr = job->has_depth_attachment ? job->ds.layer_size : 0;
            /* sls_stride */
   *stream_ptr = job->has_stencil_attachment ? job->ds.layer_size : 0;
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      pvr_finishme(
         *stream_ptr = 0;
               state->fw_stream_len = (uint8_t *)stream_ptr - (uint8_t *)state->fw_stream;
            pvr_csb_pack ((uint64_t *)stream_len_ptr, KMD_STREAM_HDR, value) {
            }
      #undef DWORDS_PER_U64
      static void
   pvr_frag_state_stream_ext_init(struct pvr_render_ctx *ctx,
               {
               uint32_t main_stream_len =
         uint32_t *ext_stream_ptr =
                  header0_ptr = ext_stream_ptr;
            pvr_csb_pack (header0_ptr, KMD_STREAM_EXTHDR_FRAG0, header0) {
      if (PVR_HAS_QUIRK(dev_info, 49927)) {
               /* The set up of CR_TPU must be identical to
   * pvr_render_job_ws_geometry_state_stream_ext_init().
   */
   pvr_csb_pack (ext_stream_ptr, CR_TPU, value) {
         }
                  if ((*header0_ptr & PVRX(KMD_STREAM_EXTHDR_DATA_MASK)) != 0) {
      state->fw_stream_len =
               }
      static void
   pvr_frag_state_flags_init(const struct pvr_render_job *const job,
         {
      *flags = (struct pvr_winsys_fragment_state_flags){
      .has_depth_buffer = job->has_depth_attachment,
   .has_stencil_buffer = job->has_stencil_attachment,
   .prevent_cdm_overlap = job->disable_compute_overlap,
   .use_single_core = job->frag_uses_atomic_ops,
   .get_vis_results = job->get_vis_results,
         }
      static void
   pvr_render_job_ws_fragment_state_init(struct pvr_render_ctx *ctx,
                     {
      pvr_frag_state_stream_init(ctx, job, state);
            state->wait = wait;
      }
      /**
   * \brief Sets up the fragment state for a Partial Render (PR) based on the
   * state for a normal fragment job.
   *
   * The state of a fragment PR is almost the same as of that for a normal
   * fragment job apart the PBE words and the EOT program, both of which are
   * necessary for the render to use the SPM scratch buffer instead of the final
   * render targets.
   *
   * By basing the fragment PR state on that of a normal fragment state,
   * repacking of the same words can be avoided as we end up mostly doing copies
   * instead.
   */
   static void pvr_render_job_ws_fragment_pr_init_based_on_fragment_state(
      const struct pvr_render_ctx *ctx,
   struct pvr_render_job *job,
   struct vk_sync *wait,
   struct pvr_winsys_fragment_state *frag,
      {
      const struct pvr_device_info *const dev_info =
         const uint32_t pbe_reg_byte_offset =
         const uint32_t eot_data_addr_byte_offset =
            /* Massive copy :( */
            assert(state->fw_stream_len >=
         memcpy(&state->fw_stream[pbe_reg_byte_offset],
                  /* TODO: Update this when csbgen is byte instead of dword granular. */
   assert(state->fw_stream_len >=
         eot_data_addr_byte_offset +
   pvr_csb_pack ((uint32_t *)&state->fw_stream[eot_data_addr_byte_offset],
                        }
      static void pvr_render_job_ws_submit_info_init(
      struct pvr_render_ctx *ctx,
   struct pvr_render_job *job,
   struct vk_sync *wait_geom,
   struct vk_sync *wait_frag,
      {
               submit_info->rt_dataset = job->rt_dataset->ws_rt_dataset;
            submit_info->frame_num = ctx->device->global_queue_present_count;
            pvr_render_job_ws_geometry_state_init(ctx,
                                          /* TODO: See if it's worth avoiding setting up the fragment state and setup
   * the pr state directly if `!job->run_frag`. For now we'll always set it up.
   */
   pvr_render_job_ws_fragment_state_init(ctx,
                        /* TODO: In some cases we could eliminate the pr and use the frag directly in
   * case we enter SPM. There's likely some performance improvement to be had
   * there. For now we'll always setup the pr.
   */
   pvr_render_job_ws_fragment_pr_init_based_on_fragment_state(
      ctx,
   job,
   wait_frag,
   &submit_info->fragment,
   }
      VkResult pvr_render_job_submit(struct pvr_render_ctx *ctx,
                                 {
      struct pvr_rt_dataset *rt_dataset = job->rt_dataset;
   struct pvr_winsys_render_submit_info submit_info;
   struct pvr_device *device = ctx->device;
            pvr_render_job_ws_submit_info_init(ctx,
                              if (PVR_IS_DEBUG_SET(DUMP_CONTROL_STREAM)) {
      /* FIXME: This isn't an ideal method of accessing the information we
   * need, but it's considered good enough for a debug code path. It can be
   * streamlined and made more correct if/when pvr_render_job becomes a
   * subclass of pvr_sub_cmd.
   */
   const struct pvr_sub_cmd *sub_cmd =
            pvr_csb_dump(&sub_cmd->gfx.control_stream,
                     result = device->ws->ops->render_submit(ctx->ws_ctx,
                           if (result != VK_SUCCESS)
            if (job->run_frag) {
      /* Move to the next render target data now that a fragment job has been
   * successfully submitted. This will allow the next geometry job to be
   * submitted to been run in parallel with it.
   */
   rt_dataset->rt_data_idx =
               } else {
                     }
