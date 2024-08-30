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
      #include "pvr_csb.h"
   #include "pvr_debug.h"
   #include "pvr_job_common.h"
   #include "pvr_job_context.h"
   #include "pvr_job_compute.h"
   #include "pvr_private.h"
   #include "pvr_types.h"
   #include "pvr_winsys.h"
   #include "util/macros.h"
      static void
   pvr_submit_info_stream_init(struct pvr_compute_ctx *ctx,
               {
      const struct pvr_device *const device = ctx->device;
   const struct pvr_physical_device *const pdevice = device->pdevice;
   const struct pvr_device_runtime_info *const dev_runtime_info =
         const struct pvr_device_info *const dev_info = &pdevice->dev_info;
            uint32_t *stream_ptr = (uint32_t *)submit_info->fw_stream;
            /* Leave space for stream header. */
            pvr_csb_pack ((uint64_t *)stream_ptr,
                  value.border_colour_table_address =
      }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_CDM_CTRL_STREAM_BASE, value) {
         }
            pvr_csb_pack ((uint64_t *)stream_ptr, CR_CDM_CONTEXT_STATE_BASE, state) {
         }
            pvr_csb_pack (stream_ptr, CR_CDM_CONTEXT_PDS1, state) {
      const uint32_t load_program_data_size =
            state.pds_seq_dep = false;
   state.usc_seq_dep = false;
   state.target = false;
   state.unified_size = ctx_switch->sr[0].usc.unified_size;
   state.common_shared = true;
   state.common_size =
      DIV_ROUND_UP(sub_cmd->num_shared_regs << 2,
               assert(load_program_data_size %
               state.data_size =
            }
            if (PVR_HAS_FEATURE(dev_info, compute_morton_capable)) {
      pvr_csb_pack (stream_ptr, CR_CDM_ITEM, value) {
         }
               if (PVR_HAS_FEATURE(dev_info, cluster_grouping)) {
      pvr_csb_pack (stream_ptr, CR_COMPUTE_CLUSTER, value) {
      if (PVR_HAS_FEATURE(dev_info, slc_mcu_cache_controls) &&
      dev_runtime_info->num_phantoms > 1 && sub_cmd->uses_atomic_ops) {
   /* Each phantom has its own MCU, so atomicity can only be
   * guaranteed when all work items are processed on the same
   * phantom. This means we need to disable all USCs other than
   * those of the first phantom, which has 4 clusters.
   */
      } else {
            }
               if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      pvr_finishme(
         *stream_ptr = 0;
               submit_info->fw_stream_len =
                  pvr_csb_pack ((uint64_t *)stream_len_ptr, KMD_STREAM_HDR, value) {
            }
      static void pvr_submit_info_ext_stream_init(
      struct pvr_compute_ctx *ctx,
      {
      const struct pvr_device_info *const dev_info =
            uint32_t *stream_ptr = (uint32_t *)submit_info->fw_stream;
   uint32_t main_stream_len =
         uint32_t *ext_stream_ptr =
                  header0_ptr = ext_stream_ptr;
            pvr_csb_pack (header0_ptr, KMD_STREAM_EXTHDR_COMPUTE0, header0) {
      if (PVR_HAS_QUIRK(dev_info, 49927)) {
               pvr_csb_pack (ext_stream_ptr, CR_TPU, value) {
         }
                  if ((*header0_ptr & PVRX(KMD_STREAM_EXTHDR_DATA_MASK)) != 0) {
      submit_info->fw_stream_len =
               }
      static void
   pvr_submit_info_flags_init(const struct pvr_device_info *const dev_info,
               {
      *flags = (struct pvr_winsys_compute_submit_flags){
      .prevent_all_overlap = sub_cmd->uses_barrier,
   .use_single_core = PVR_HAS_FEATURE(dev_info, gpu_multicore_support) &&
         }
      static void pvr_compute_job_ws_submit_info_init(
      struct pvr_compute_ctx *ctx,
   struct pvr_sub_cmd_compute *sub_cmd,
   struct vk_sync *wait,
      {
      const struct pvr_device *const device = ctx->device;
                     submit_info->frame_num = device->global_queue_present_count;
                     pvr_submit_info_stream_init(ctx, sub_cmd, submit_info);
   pvr_submit_info_ext_stream_init(ctx, submit_info);
      }
      VkResult pvr_compute_job_submit(struct pvr_compute_ctx *ctx,
                     {
      struct pvr_winsys_compute_submit_info submit_info;
                     if (PVR_IS_DEBUG_SET(DUMP_CONTROL_STREAM)) {
      pvr_csb_dump(&sub_cmd->control_stream,
                     return device->ws->ops->compute_submit(ctx->ws_ctx,
                  }
