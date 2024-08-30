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
   #include <limits.h>
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
   #include "pvr_srv_job_compute.h"
   #include "pvr_srv_sync.h"
   #include "pvr_winsys.h"
   #include "util/macros.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
      struct pvr_srv_winsys_compute_ctx {
                           };
      #define to_pvr_srv_winsys_compute_ctx(ctx) \
            VkResult pvr_srv_winsys_compute_ctx_create(
      struct pvr_winsys *ws,
   const struct pvr_winsys_compute_ctx_create_info *create_info,
      {
         .ctx_switch_regs = {
      .cdm_context_pds0 = create_info->static_state.cdm_ctx_store_pds0,
   .cdm_context_pds0_b =
   create_info->static_state.cdm_ctx_store_pds0_b,
            .cdm_terminate_pds = create_info->static_state.cdm_ctx_terminate_pds,
   .cdm_terminate_pds1 =
            .cdm_resume_pds0 = create_info->static_state.cdm_ctx_resume_pds0,
      },
   };
                  struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   struct pvr_srv_winsys_compute_ctx *srv_ctx;
            srv_ctx = vk_alloc(ws->alloc,
                     if (!srv_ctx)
            result = pvr_srv_create_timeline(ws->render_fd, &srv_ctx->timeline);
   if (result != VK_SUCCESS)
            /* TODO: Add support for reset framework. Currently we subtract
   * reset_cmd.regs size from reset_cmd size to only pass empty flags field.
   */
   result = pvr_srv_rgx_create_compute_context(
      ws->render_fd,
   pvr_srv_from_winsys_priority(create_info->priority),
   sizeof(reset_cmd) - sizeof(reset_cmd.regs),
   (uint8_t *)&reset_cmd,
   srv_ws->server_memctx_data,
   sizeof(static_state),
   (uint8_t *)&static_state,
   0U,
   RGX_CONTEXT_FLAG_DISABLESLR,
   0U,
   UINT_MAX,
      if (result != VK_SUCCESS)
                                    err_close_timeline:
            err_free_srv_ctx:
                  }
      void pvr_srv_winsys_compute_ctx_destroy(struct pvr_winsys_compute_ctx *ctx)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ctx->ws);
   struct pvr_srv_winsys_compute_ctx *srv_ctx =
            pvr_srv_rgx_destroy_compute_context(srv_ws->base.render_fd, srv_ctx->handle);
   close(srv_ctx->timeline);
      }
      static uint32_t
   pvr_srv_compute_cmd_stream_load(struct rogue_fwif_cmd_compute *const cmd,
                     {
      const uint32_t *stream_ptr = (const uint32_t *)stream;
   struct rogue_fwif_cdm_regs *const regs = &cmd->regs;
   uint32_t main_stream_len =
                     regs->tpu_border_colour_table = *(const uint64_t *)stream_ptr;
            regs->cdm_ctrl_stream_base = *(const uint64_t *)stream_ptr;
            regs->cdm_context_state_base_addr = *(const uint64_t *)stream_ptr;
            regs->cdm_resume_pds1 = *stream_ptr;
            regs->cdm_item = *stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, cluster_grouping)) {
      regs->compute_cluster = *stream_ptr;
               if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      cmd->execute_count = *stream_ptr;
               assert((const uint8_t *)stream_ptr - stream <= stream_len);
               }
      static void pvr_srv_compute_cmd_ext_stream_load(
      struct rogue_fwif_cmd_compute *const cmd,
   const uint8_t *const stream,
   const uint32_t stream_len,
   const uint32_t ext_stream_offset,
      {
      const uint32_t *ext_stream_ptr =
                           header0 = pvr_csb_unpack(ext_stream_ptr, KMD_STREAM_EXTHDR_COMPUTE0);
            assert(PVR_HAS_QUIRK(dev_info, 49927) == header0.has_brn49927);
   if (header0.has_brn49927) {
      regs->tpu = *ext_stream_ptr;
                  }
      static void pvr_srv_compute_cmd_init(
      const struct pvr_winsys_compute_submit_info *submit_info,
   struct rogue_fwif_cmd_compute *cmd,
      {
                                 ext_stream_offset =
      pvr_srv_compute_cmd_stream_load(cmd,
                     if (ext_stream_offset < submit_info->fw_stream_len) {
      pvr_srv_compute_cmd_ext_stream_load(cmd,
                                 if (submit_info->flags.prevent_all_overlap)
            if (submit_info->flags.use_single_core)
      }
      VkResult pvr_srv_winsys_compute_submit(
      const struct pvr_winsys_compute_ctx *ctx,
   const struct pvr_winsys_compute_submit_info *submit_info,
   const struct pvr_device_info *const dev_info,
      {
      const struct pvr_srv_winsys_compute_ctx *srv_ctx =
         const struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ctx->ws);
   struct rogue_fwif_cmd_compute compute_cmd;
   struct pvr_srv_sync *srv_signal_sync;
   VkResult result;
   int in_fd = -1;
                     if (submit_info->wait) {
               if (srv_wait_sync->fd >= 0) {
      in_fd = dup(srv_wait_sync->fd);
   if (in_fd == -1) {
      return vk_errorf(NULL,
                                 do {
      result = pvr_srv_rgx_kick_compute2(srv_ws->base.render_fd,
                                    srv_ctx->handle,
   0U,
   NULL,
   NULL,
   NULL,
   in_fd,
   srv_ctx->timeline,
   sizeof(compute_cmd),
   (uint8_t *)&compute_cmd,
   submit_info->job_num,
   0,
   NULL,
   NULL,
            if (result != VK_SUCCESS)
            if (signal_sync) {
      srv_signal_sync = to_srv_sync(signal_sync);
      } else if (fence != -1) {
               end_close_in_fd:
      if (in_fd >= 0)
               }
