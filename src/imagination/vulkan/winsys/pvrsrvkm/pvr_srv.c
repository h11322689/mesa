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
   #include <xf86drm.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_csb.h"
   #include "pvr_device_info.h"
   #include "pvr_private.h"
   #include "pvr_srv.h"
   #include "pvr_srv_bo.h"
   #include "pvr_srv_bridge.h"
   #include "pvr_srv_job_common.h"
   #include "pvr_srv_job_compute.h"
   #include "pvr_srv_job_render.h"
   #include "pvr_srv_job_transfer.h"
   #include "pvr_srv_public.h"
   #include "pvr_srv_sync.h"
   #include "pvr_srv_sync_prim.h"
   #include "pvr_srv_job_null.h"
   #include "pvr_types.h"
   #include "pvr_winsys.h"
   #include "pvr_winsys_helper.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "util/os_misc.h"
   #include "util/u_atomic.h"
   #include "vk_log.h"
   #include "vk_sync.h"
   #include "vk_sync_timeline.h"
      /* carveout_size can be 0 when no carveout is needed. carveout_address must
   * be 0 if carveout_size is 0.
   */
   static VkResult pvr_winsys_heap_init(
      struct pvr_winsys *const ws,
   pvr_dev_addr_t base_address,
   uint64_t size,
   pvr_dev_addr_t carveout_address,
   uint64_t carveout_size,
   uint32_t log2_page_size,
   const struct pvr_winsys_static_data_offsets *const static_data_offsets,
      {
      const bool carveout_area_bottom_of_heap = carveout_address.addr ==
         const pvr_dev_addr_t vma_heap_begin_addr =
      carveout_area_bottom_of_heap
      ? PVR_DEV_ADDR_OFFSET(base_address, carveout_size)
            assert(base_address.addr);
            /* As per the static_data_carveout_base powervr-km uapi documentation the
   * carveout region can only be at the beginning of the heap or at the end.
   * carveout_address is 0 if there is no carveout region.
   * pvrsrv-km doesn't explicitly provide this info and it's assumed that it's
   * always at the beginning.
   */
   assert(carveout_area_bottom_of_heap ||
                  heap->ws = ws;
   heap->base_addr = base_address;
            heap->size = size;
            heap->page_size = 1 << log2_page_size;
                              /* It's expected that the heap destroy function to be the last thing that's
   * called, so we start the ref_count at 0.
   */
            if (pthread_mutex_init(&heap->lock, NULL))
                        }
      /**
   * Maximum PB free list size supported by RGX and Services.
   *
   * Maximum PB free list size must ensure that no PM address space can be fully
   * used, because if the full address space was used it would wrap and corrupt
   * itself. Since there are two freelists (local is always minimum sized) this
   * can be described as following three conditions being met:
   *
   *  Minimum PB + Maximum PB < ALIST PM address space size (16GB)
   *  Minimum PB + Maximum PB < TE PM address space size (16GB) / NUM_TE_PIPES
   *  Minimum PB + Maximum PB < VCE PM address space size (16GB) / NUM_VCE_PIPES
   *
   * Since the max of NUM_TE_PIPES and NUM_VCE_PIPES is 4, we have a hard limit
   * of 4GB minus the Minimum PB. For convenience we take the smaller power-of-2
   * value of 2GB. This is far more than any normal application would request
   * or use.
   */
   #define PVR_SRV_FREE_LIST_MAX_SIZE (2ULL * 1024ULL * 1024ULL * 1024ULL)
      static VkResult pvr_srv_heap_init(
      struct pvr_srv_winsys *srv_ws,
   struct pvr_srv_winsys_heap *srv_heap,
   uint32_t heap_idx,
      {
      pvr_dev_addr_t base_address;
   uint32_t log2_page_size;
   uint64_t reserved_size;
   VkResult result;
            result = pvr_srv_get_heap_details(srv_ws->base.render_fd,
                                    heap_idx,
      if (result != VK_SUCCESS)
            result = pvr_winsys_heap_init(&srv_ws->base,
                                 base_address,
   size,
   if (result != VK_SUCCESS)
            assert(srv_heap->base.page_size == srv_ws->base.page_size);
   assert(srv_heap->base.log2_page_size == srv_ws->base.log2_page_size);
   assert(srv_heap->base.static_data_carveout_size %
                  /* Create server-side counterpart of Device Memory heap */
   result = pvr_srv_int_heap_create(srv_ws->base.render_fd,
                                 if (result != VK_SUCCESS) {
      pvr_winsys_helper_winsys_heap_finish(&srv_heap->base);
                  }
      static bool pvr_srv_heap_finish(struct pvr_srv_winsys *srv_ws,
         {
      if (!pvr_winsys_helper_winsys_heap_finish(&srv_heap->base))
                        }
      static VkResult pvr_srv_memctx_init(struct pvr_srv_winsys *srv_ws)
   {
      const struct pvr_winsys_static_data_offsets
      general_heap_static_data_offsets = {
            const struct pvr_winsys_static_data_offsets pds_heap_static_data_offsets = {
      .eot = FWIF_PDS_HEAP_EOT_OFFSET_BYTES,
      };
   const struct pvr_winsys_static_data_offsets usc_heap_static_data_offsets = {
         };
            char heap_name[PVR_SRV_DEVMEM_HEAPNAME_MAXLENGTH];
   int transfer_3d_heap_idx = -1;
   int vis_test_heap_idx = -1;
   int general_heap_idx = -1;
   int rgn_hdr_heap_idx = -1;
   int pds_heap_idx = -1;
   int usc_heap_idx = -1;
   uint32_t heap_count;
            result = pvr_srv_int_ctx_create(srv_ws->base.render_fd,
               if (result != VK_SUCCESS)
            os_get_page_size(&srv_ws->base.page_size);
            result = pvr_srv_get_heap_count(srv_ws->base.render_fd, &heap_count);
   if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < heap_count; i++) {
      result = pvr_srv_get_heap_details(srv_ws->base.render_fd,
                                    i,
      if (result != VK_SUCCESS)
            if (general_heap_idx == -1 &&
      strncmp(heap_name,
                  } else if (pds_heap_idx == -1 &&
            strncmp(heap_name,
            } else if (rgn_hdr_heap_idx == -1 &&
            strncmp(heap_name,
            } else if (transfer_3d_heap_idx == -1 &&
            strncmp(heap_name,
            } else if (usc_heap_idx == -1 &&
            strncmp(heap_name,
            } else if (vis_test_heap_idx == -1 &&
            strncmp(heap_name,
                        /* Check for and initialise required heaps. */
   if (general_heap_idx == -1 || pds_heap_idx == -1 ||
      transfer_3d_heap_idx == -1 || usc_heap_idx == -1 ||
   vis_test_heap_idx == -1) {
   result = vk_error(NULL, VK_ERROR_INITIALIZATION_FAILED);
               result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
            result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
            result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
            result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
            result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
            /* Check for and set up optional heaps. */
   if (rgn_hdr_heap_idx != -1) {
      result = pvr_srv_heap_init(srv_ws,
                     if (result != VK_SUCCESS)
               } else {
                  result =
      pvr_winsys_helper_allocate_static_memory(&srv_ws->base,
                                          if (result != VK_SUCCESS)
            result = pvr_winsys_helper_fill_static_memory(&srv_ws->base,
                     if (result != VK_SUCCESS)
                  err_pvr_srv_free_static_memory:
      pvr_winsys_helper_free_static_memory(srv_ws->general_vma,
               err_pvr_srv_heap_finish_rgn_hdr:
      if (srv_ws->rgn_hdr_heap_present)
         err_pvr_srv_heap_finish_vis_test:
            err_pvr_srv_heap_finish_usc:
            err_pvr_srv_heap_finish_transfer_3d:
            err_pvr_srv_heap_finish_pds:
            err_pvr_srv_heap_finish_general:
            err_pvr_srv_int_ctx_destroy:
                  }
      static void pvr_srv_memctx_finish(struct pvr_srv_winsys *srv_ws)
   {
      pvr_winsys_helper_free_static_memory(srv_ws->general_vma,
                  if (srv_ws->rgn_hdr_heap_present) {
      if (!pvr_srv_heap_finish(srv_ws, &srv_ws->rgn_hdr_heap)) {
      vk_errorf(NULL,
                        if (!pvr_srv_heap_finish(srv_ws, &srv_ws->vis_test_heap)) {
      vk_errorf(NULL,
                     if (!pvr_srv_heap_finish(srv_ws, &srv_ws->usc_heap))
            if (!pvr_srv_heap_finish(srv_ws, &srv_ws->transfer_3d_heap)) {
      vk_errorf(NULL,
                     if (!pvr_srv_heap_finish(srv_ws, &srv_ws->pds_heap))
            if (!pvr_srv_heap_finish(srv_ws, &srv_ws->general_heap)) {
                     }
      static void pvr_srv_winsys_destroy(struct pvr_winsys *ws)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
            if (srv_ws->presignaled_sync) {
      vk_sync_destroy(&srv_ws->presignaled_sync_device->vk,
               pvr_srv_sync_prim_block_finish(srv_ws);
   pvr_srv_memctx_finish(srv_ws);
   vk_free(ws->alloc, srv_ws);
      }
      static uint64_t
   pvr_srv_get_min_free_list_size(const struct pvr_device_info *dev_info)
   {
               if (PVR_HAS_FEATURE(dev_info, roguexe)) {
      if (PVR_HAS_QUIRK(dev_info, 66011))
         else
      } else {
                     }
      static inline uint64_t
   pvr_srv_get_num_phantoms(const struct pvr_device_info *dev_info)
   {
         }
      /* Return the total reserved size of partition in dwords. */
   static inline uint64_t pvr_srv_get_total_reserved_partition_size(
         {
      uint32_t tile_size_x = PVR_GET_FEATURE_VALUE(dev_info, tile_size_x, 0);
   uint32_t tile_size_y = PVR_GET_FEATURE_VALUE(dev_info, tile_size_y, 0);
            if (tile_size_x == 16 && tile_size_y == 16) {
      return tile_size_x * tile_size_y * max_partitions *
         PVR_GET_FEATURE_VALUE(dev_info,
                  }
      static inline uint64_t
   pvr_srv_get_reserved_shared_size(const struct pvr_device_info *dev_info)
   {
      uint32_t common_store_size_in_dwords =
      PVR_GET_FEATURE_VALUE(dev_info,
            uint32_t reserved_shared_size =
      common_store_size_in_dwords - (256U * 4U) -
         if (PVR_HAS_QUIRK(dev_info, 44079)) {
                              }
      static inline uint64_t
   pvr_srv_get_max_coeffs(const struct pvr_device_info *dev_info)
   {
      uint32_t max_coeff_additional_portion = ROGUE_MAX_VERTEX_SHARED_REGISTERS;
   uint32_t pending_allocation_shared_regs = 2U * 1024U;
   uint32_t pending_allocation_coeff_regs = 0U;
   uint32_t num_phantoms = pvr_srv_get_num_phantoms(dev_info);
   uint32_t tiles_in_flight =
         uint32_t max_coeff_pixel_portion =
                     /* Compute tasks on cores with BRN48492 and without compute overlap may lock
   * up without two additional lines of coeffs.
   */
   if (PVR_HAS_QUIRK(dev_info, 48492) &&
      !PVR_HAS_FEATURE(dev_info, compute_overlap)) {
               if (PVR_HAS_ERN(dev_info, 38748))
            if (PVR_HAS_ERN(dev_info, 38020)) {
      max_coeff_additional_portion +=
               return pvr_srv_get_reserved_shared_size(dev_info) +
         pending_allocation_coeff_regs -
      }
      static inline uint64_t
   pvr_srv_get_cdm_max_local_mem_size_regs(const struct pvr_device_info *dev_info)
   {
               if (PVR_HAS_QUIRK(dev_info, 48492) && PVR_HAS_FEATURE(dev_info, roguexe) &&
      !PVR_HAS_FEATURE(dev_info, compute_overlap)) {
   /* Driver must not use the 2 reserved lines. */
               /* The maximum amount of local memory available to a kernel is the minimum
   * of the total number of coefficient registers available and the max common
   * store allocation size which can be made by the CDM.
   *
   * If any coeff lines are reserved for tessellation or pixel then we need to
   * subtract those too.
   */
   return MIN2(available_coeffs_in_dwords,
      }
      static VkResult
   pvr_srv_winsys_device_info_init(struct pvr_winsys *ws,
               {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   VkResult result;
            ret = pvr_device_info_init(dev_info, srv_ws->bvnc);
   if (ret) {
      return vk_errorf(NULL,
                  VK_ERROR_INCOMPATIBLE_DRIVER,
   "Unsupported BVNC: %u.%u.%u.%u\n",
   PVR_BVNC_UNPACK_B(srv_ws->bvnc),
            runtime_info->min_free_list_size = pvr_srv_get_min_free_list_size(dev_info);
   runtime_info->max_free_list_size = PVR_SRV_FREE_LIST_MAX_SIZE;
   runtime_info->reserved_shared_size =
         runtime_info->total_reserved_partition_size =
         runtime_info->num_phantoms = pvr_srv_get_num_phantoms(dev_info);
   runtime_info->max_coeffs = pvr_srv_get_max_coeffs(dev_info);
   runtime_info->cdm_max_local_mem_size_regs =
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      result = pvr_srv_get_multicore_info(ws->render_fd,
                     if (result != VK_SUCCESS)
      } else {
                     }
      static void pvr_srv_winsys_get_heaps_info(struct pvr_winsys *ws,
         {
               heaps->general_heap = &srv_ws->general_heap.base;
   heaps->pds_heap = &srv_ws->pds_heap.base;
   heaps->transfer_frag_heap = &srv_ws->transfer_3d_heap.base;
   heaps->usc_heap = &srv_ws->usc_heap.base;
            if (srv_ws->rgn_hdr_heap_present)
         else
      }
      static const struct pvr_winsys_ops srv_winsys_ops = {
      .destroy = pvr_srv_winsys_destroy,
   .device_info_init = pvr_srv_winsys_device_info_init,
   .get_heaps_info = pvr_srv_winsys_get_heaps_info,
   .buffer_create = pvr_srv_winsys_buffer_create,
   .buffer_create_from_fd = pvr_srv_winsys_buffer_create_from_fd,
   .buffer_destroy = pvr_srv_winsys_buffer_destroy,
   .buffer_get_fd = pvr_srv_winsys_buffer_get_fd,
   .buffer_map = pvr_srv_winsys_buffer_map,
   .buffer_unmap = pvr_srv_winsys_buffer_unmap,
   .heap_alloc = pvr_srv_winsys_heap_alloc,
   .heap_free = pvr_srv_winsys_heap_free,
   .vma_map = pvr_srv_winsys_vma_map,
   .vma_unmap = pvr_srv_winsys_vma_unmap,
   .free_list_create = pvr_srv_winsys_free_list_create,
   .free_list_destroy = pvr_srv_winsys_free_list_destroy,
   .render_target_dataset_create = pvr_srv_render_target_dataset_create,
   .render_target_dataset_destroy = pvr_srv_render_target_dataset_destroy,
   .render_ctx_create = pvr_srv_winsys_render_ctx_create,
   .render_ctx_destroy = pvr_srv_winsys_render_ctx_destroy,
   .render_submit = pvr_srv_winsys_render_submit,
   .compute_ctx_create = pvr_srv_winsys_compute_ctx_create,
   .compute_ctx_destroy = pvr_srv_winsys_compute_ctx_destroy,
   .compute_submit = pvr_srv_winsys_compute_submit,
   .transfer_ctx_create = pvr_srv_winsys_transfer_ctx_create,
   .transfer_ctx_destroy = pvr_srv_winsys_transfer_ctx_destroy,
   .transfer_submit = pvr_srv_winsys_transfer_submit,
      };
      static bool pvr_is_driver_compatible(int render_fd)
   {
               version = drmGetVersion(render_fd);
   if (!version)
                     /* Only the 1.17 driver is supported for now. */
   if (version->version_major != PVR_SRV_VERSION_MAJ ||
      version->version_minor != PVR_SRV_VERSION_MIN) {
   vk_errorf(NULL,
            VK_ERROR_INCOMPATIBLE_DRIVER,
   "Unsupported downstream driver version (%u.%u)",
                                       }
      VkResult pvr_srv_winsys_create(const int render_fd,
                     {
      struct pvr_srv_winsys *srv_ws;
   VkResult result;
            if (!pvr_is_driver_compatible(render_fd))
            result = pvr_srv_init_module(render_fd, PVR_SRVKM_MODULE_TYPE_SERVICES);
   if (result != VK_SUCCESS)
            result = pvr_srv_connection_create(render_fd, &bvnc);
   if (result != VK_SUCCESS)
            srv_ws =
         if (!srv_ws) {
      result = vk_error(NULL, VK_ERROR_OUT_OF_HOST_MEMORY);
               srv_ws->base.ops = &srv_winsys_ops;
   srv_ws->base.render_fd = render_fd;
   srv_ws->base.display_fd = display_fd;
                     srv_ws->base.syncobj_type = pvr_srv_sync_type;
            srv_ws->base.timeline_syncobj_type =
         srv_ws->base.sync_types[1] = &srv_ws->base.timeline_syncobj_type.sync;
            /* Threaded submit requires VK_SYNC_FEATURE_WAIT_PENDING which pvrsrv
   * doesn't support.
   */
            result = pvr_srv_memctx_init(srv_ws);
   if (result != VK_SUCCESS)
            result = pvr_srv_sync_prim_block_init(srv_ws);
   if (result != VK_SUCCESS)
                           err_pvr_srv_memctx_finish:
            err_vk_free_srv_ws:
            err_pvr_srv_connection_destroy:
            err_out:
         }
      static VkResult pvr_srv_create_presignaled_sync(struct pvr_device *device,
         {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(device->ws);
            int timeline_fd;
                     result = pvr_srv_create_timeline(srv_ws->base.render_fd, &timeline_fd);
   if (result != VK_SUCCESS)
            result = pvr_srv_set_timeline_sw_only(timeline_fd);
   if (result != VK_SUCCESS)
            result = pvr_srv_create_sw_fence(timeline_fd, &sync_fd, NULL);
   if (result != VK_SUCCESS)
            result = pvr_srv_sw_sync_timeline_increment(timeline_fd, NULL);
   if (result != VK_SUCCESS)
            result = vk_sync_create(&device->vk,
                           if (result != VK_SUCCESS)
            result = vk_sync_import_sync_file(&device->vk, sync, sync_fd);
   if (result != VK_SUCCESS)
            *out_sync = to_srv_sync(sync);
                           err_destroy_sync:
            err_close_sw_fence:
            err_close_timeline:
                  }
      VkResult pvr_srv_sync_get_presignaled_sync(struct pvr_device *device,
         {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(device->ws);
            if (!srv_ws->presignaled_sync) {
      result =
         if (result != VK_SUCCESS)
                                             }
