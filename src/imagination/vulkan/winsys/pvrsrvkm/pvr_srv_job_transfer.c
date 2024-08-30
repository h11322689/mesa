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
      #include <errno.h>
   #include <fcntl.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <unistd.h>
   #include <vulkan/vulkan.h>
      #include "fw-api/pvr_rogue_fwif.h"
   #include "fw-api/pvr_rogue_fwif_rf.h"
   #include "pvr_device_info.h"
   #include "pvr_private.h"
   #include "pvr_srv.h"
   #include "pvr_srv_bridge.h"
   #include "pvr_srv_job_common.h"
   #include "pvr_srv_job_transfer.h"
   #include "pvr_srv_sync.h"
   #include "pvr_winsys.h"
   #include "util/macros.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
   #include "vk_util.h"
      #define PVR_SRV_TRANSFER_CONTEXT_INITIAL_CCB_SIZE_LOG2 16U
   #define PVR_SRV_TRANSFER_CONTEXT_MAX_CCB_SIZE_LOG2 0U
      struct pvr_srv_winsys_transfer_ctx {
                           };
      #define to_pvr_srv_winsys_transfer_ctx(ctx) \
            VkResult pvr_srv_winsys_transfer_ctx_create(
      struct pvr_winsys *ws,
   const struct pvr_winsys_transfer_ctx_create_info *create_info,
      {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   struct pvr_srv_winsys_transfer_ctx *srv_ctx;
   struct rogue_fwif_rf_cmd reset_cmd = { 0 };
            /* First 2 U8s are 2d work load related, and the last 2 are 3d workload
   * related.
   */
   const uint32_t packed_ccb_size =
      PVR_U8888_TO_U32(PVR_SRV_TRANSFER_CONTEXT_INITIAL_CCB_SIZE_LOG2,
                     srv_ctx = vk_alloc(ws->alloc,
                     if (!srv_ctx)
            result = pvr_srv_create_timeline(ws->render_fd, &srv_ctx->timeline_3d);
   if (result != VK_SUCCESS)
            /* TODO: Add support for reset framework. Currently we subtract
   * reset_cmd.regs size from reset_cmd size to only pass empty flags field.
   */
   result = pvr_srv_rgx_create_transfer_context(
      ws->render_fd,
   pvr_srv_from_winsys_priority(create_info->priority),
   sizeof(reset_cmd) - sizeof(reset_cmd.regs),
   (uint8_t *)&reset_cmd,
   srv_ws->server_memctx_data,
   packed_ccb_size,
   RGX_CONTEXT_FLAG_DISABLESLR,
   0U,
   NULL,
   NULL,
      if (result != VK_SUCCESS)
            srv_ctx->base.ws = ws;
                  err_close_timeline:
            err_free_srv_ctx:
                  }
      void pvr_srv_winsys_transfer_ctx_destroy(struct pvr_winsys_transfer_ctx *ctx)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ctx->ws);
   struct pvr_srv_winsys_transfer_ctx *srv_ctx =
            pvr_srv_rgx_destroy_transfer_context(srv_ws->base.render_fd,
         close(srv_ctx->timeline_3d);
      }
      static void
   pvr_srv_transfer_cmd_stream_load(struct rogue_fwif_cmd_transfer *const cmd,
                     {
      const uint32_t *stream_ptr = (const uint32_t *)stream;
   struct rogue_fwif_transfer_regs *const regs = &cmd->regs;
   uint32_t main_stream_len =
                     regs->pds_bgnd0_base = *(uint64_t *)stream_ptr;
            regs->pds_bgnd1_base = *(uint64_t *)stream_ptr;
            regs->pds_bgnd3_sizeinfo = *(uint64_t *)stream_ptr;
            regs->isp_mtile_base = *(uint64_t *)stream_ptr;
            STATIC_ASSERT(ARRAY_SIZE(regs->pbe_wordx_mrty) == 9U);
   STATIC_ASSERT(sizeof(regs->pbe_wordx_mrty[0]) == sizeof(uint64_t));
   memcpy(regs->pbe_wordx_mrty, stream_ptr, sizeof(regs->pbe_wordx_mrty));
            regs->isp_bgobjvals = *stream_ptr;
            regs->usc_pixel_output_ctrl = *stream_ptr;
            regs->usc_clear_register0 = *stream_ptr;
            regs->usc_clear_register1 = *stream_ptr;
            regs->usc_clear_register2 = *stream_ptr;
            regs->usc_clear_register3 = *stream_ptr;
            regs->isp_mtile_size = *stream_ptr;
            regs->isp_render_origin = *stream_ptr;
            regs->isp_ctl = *stream_ptr;
            regs->isp_aa = *stream_ptr;
            regs->event_pixel_pds_info = *stream_ptr;
            regs->event_pixel_pds_code = *stream_ptr;
            regs->event_pixel_pds_data = *stream_ptr;
            regs->isp_render = *stream_ptr;
            regs->isp_rgn = *stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      regs->frag_screen = *stream_ptr;
               assert((const uint8_t *)stream_ptr - stream == stream_len);
      }
      static void pvr_srv_transfer_cmds_init(
      const struct pvr_winsys_transfer_submit_info *submit_info,
   struct rogue_fwif_cmd_transfer *cmds,
   uint32_t cmd_count,
      {
               for (uint32_t i = 0; i < cmd_count; i++) {
      const struct pvr_winsys_transfer_cmd *submit_cmd = &submit_info->cmds[i];
                     pvr_srv_transfer_cmd_stream_load(cmd,
                        if (submit_info->cmds[i].flags.use_single_core)
         }
      VkResult pvr_srv_winsys_transfer_submit(
      const struct pvr_winsys_transfer_ctx *ctx,
   const struct pvr_winsys_transfer_submit_info *submit_info,
   const struct pvr_device_info *const dev_info,
      {
      const struct pvr_srv_winsys_transfer_ctx *srv_ctx =
                  struct rogue_fwif_cmd_transfer
         uint32_t *update_sync_offsets[PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT] = { 0 };
   uint32_t client_update_count[PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT] = { 0 };
   void **update_ufo_syc_prims[PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT] = { 0 };
   uint32_t *update_values[PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT] = { 0 };
   uint32_t cmd_flags[PVR_TRANSFER_MAX_PREPARES_PER_SUBMIT] = { 0 };
            struct pvr_srv_sync *srv_signal_sync;
   uint32_t job_num;
   VkResult result;
   int in_fd = -1;
            STACK_ARRAY(struct rogue_fwif_cmd_transfer,
               if (!transfer_cmds)
            pvr_srv_transfer_cmds_init(submit_info,
                        for (uint32_t i = 0U; i < submit_info->cmd_count; i++) {
      cmd_sizes[i] = sizeof(**cmds_ptr_arr);
               if (submit_info->wait) {
               if (srv_wait_sync->fd >= 0) {
      in_fd = dup(srv_wait_sync->fd);
   if (in_fd == -1) {
      return vk_errorf(NULL,
                                          do {
      result = pvr_srv_rgx_submit_transfer2(srv_ws->base.render_fd,
                                       srv_ctx->handle,
   submit_info->cmd_count,
   client_update_count,
   update_ufo_syc_prims,
   update_sync_offsets,
   update_values,
   in_fd,
   -1,
   srv_ctx->timeline_3d,
   "TRANSFER",
   cmd_sizes,
   (uint8_t **)cmds_ptr_arr,
               if (result != VK_SUCCESS)
            if (signal_sync) {
      srv_signal_sync = to_srv_sync(signal_sync);
      } else if (fence != -1) {
               end_close_in_fd:
      if (in_fd >= 0)
                        }
