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
   #include <errno.h>
   #include <fcntl.h>
   #include <limits.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <unistd.h>
   #include <vulkan/vulkan.h>
      #include "fw-api/pvr_rogue_fwif.h"
   #include "fw-api/pvr_rogue_fwif_rf.h"
   #include "pvr_csb.h"
   #include "pvr_job_render.h"
   #include "pvr_private.h"
   #include "pvr_srv.h"
   #include "pvr_srv_bo.h"
   #include "pvr_srv_bridge.h"
   #include "pvr_srv_job_common.h"
   #include "pvr_srv_job_render.h"
   #include "pvr_srv_sync.h"
   #include "pvr_srv_sync_prim.h"
   #include "pvr_types.h"
   #include "pvr_winsys.h"
   #include "util/log.h"
   #include "util/macros.h"
   #include "util/u_math.h"
   #include "vk_alloc.h"
   #include "vk_log.h"
   #include "vk_util.h"
      struct pvr_srv_winsys_free_list {
                           };
      #define to_pvr_srv_winsys_free_list(free_list) \
            struct pvr_srv_winsys_rt_dataset {
               struct {
      void *handle;
         };
      #define to_pvr_srv_winsys_rt_dataset(rt_dataset) \
            struct pvr_srv_winsys_render_ctx {
               /* Handle to kernel context. */
            int timeline_geom;
      };
      #define to_pvr_srv_winsys_render_ctx(ctx) \
            VkResult pvr_srv_winsys_free_list_create(
      struct pvr_winsys *ws,
   struct pvr_winsys_vma *free_list_vma,
   uint32_t initial_num_pages,
   uint32_t max_num_pages,
   uint32_t grow_num_pages,
   uint32_t grow_threshold,
   struct pvr_winsys_free_list *parent_free_list,
      {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   struct pvr_srv_winsys_bo *srv_free_list_bo =
         struct pvr_srv_winsys_free_list *srv_free_list;
   void *parent_handle;
            srv_free_list = vk_zalloc(ws->alloc,
                     if (!srv_free_list)
            if (parent_free_list) {
      srv_free_list->parent = to_pvr_srv_winsys_free_list(parent_free_list);
      } else {
      srv_free_list->parent = NULL;
               result = pvr_srv_rgx_create_free_list(ws->render_fd,
                                    #if defined(DEBUG)
         #else
         #endif
                              if (result != VK_SUCCESS)
                                    err_vk_free_srv_free_list:
                  }
      void pvr_srv_winsys_free_list_destroy(struct pvr_winsys_free_list *free_list)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(free_list->ws);
   struct pvr_srv_winsys_free_list *srv_free_list =
            pvr_srv_rgx_destroy_free_list(srv_ws->base.render_fd, srv_free_list->handle);
      }
      static uint64_t pvr_rogue_get_cr_multisamplectl_val(uint32_t samples,
         {
      static const struct {
      uint8_t x[8];
      } sample_positions[4] = {
      /* 1 sample */
   {
      .x = { 8 },
      },
   /* 2 samples */
   {
      .x = { 12, 4 },
      },
   /* 4 samples */
   {
      .x = { 6, 14, 2, 10 },
      },
   /* 8 samples */
   {
      .x = { 9, 7, 13, 5, 3, 1, 11, 15 },
         };
   uint64_t multisamplectl;
            idx = util_fast_log2(samples);
            pvr_csb_pack (&multisamplectl, CR_PPP_MULTISAMPLECTL, value) {
      switch (samples) {
   case 8:
      value.msaa_x7 = sample_positions[idx].x[7];
   value.msaa_x6 = sample_positions[idx].x[6];
                  if (y_flip) {
      value.msaa_y7 = 16U - sample_positions[idx].y[7];
   value.msaa_y6 = 16U - sample_positions[idx].y[6];
   value.msaa_y5 = 16U - sample_positions[idx].y[5];
      } else {
      value.msaa_y7 = sample_positions[idx].y[7];
   value.msaa_y6 = sample_positions[idx].y[6];
   value.msaa_y5 = sample_positions[idx].y[5];
                  case 4:
                     if (y_flip) {
      value.msaa_y3 = 16U - sample_positions[idx].y[3];
      } else {
      value.msaa_y3 = sample_positions[idx].y[3];
                  case 2:
               if (y_flip) {
         } else {
                     case 1:
               if (y_flip) {
         } else {
                     default:
                        }
      static uint32_t
   pvr_rogue_get_cr_isp_mtile_size_val(const struct pvr_device_info *dev_info,
               {
      uint32_t samples_per_pixel =
                  pvr_csb_pack (&isp_mtile_size, CR_ISP_MTILE_SIZE, value) {
      value.x = mtile_info->mtile_x1;
            if (samples_per_pixel == 1) {
                     if (samples >= 2)
      } else if (samples_per_pixel == 2) {
                     if (samples >= 4)
      } else if (samples_per_pixel == 4) {
      if (samples >= 8)
      } else {
                        }
      static uint32_t pvr_rogue_get_ppp_screen_val(uint32_t width, uint32_t height)
   {
               pvr_csb_pack (&val, CR_PPP_SCREEN, state) {
      state.pixxmax = width - 1;
                  }
      struct pvr_rogue_cr_te {
      uint32_t aa;
   uint32_t mtile1;
   uint32_t mtile2;
   uint32_t screen;
      };
      static void pvr_rogue_ct_te_init(const struct pvr_device_info *dev_info,
                     {
      uint32_t samples_per_pixel =
            pvr_csb_pack (&te_regs->aa, CR_TE_AA, value) {
      if (samples_per_pixel == 1) {
      if (samples >= 2)
         if (samples >= 4)
      } else if (samples_per_pixel == 2) {
      if (samples >= 2)
         if (samples >= 4)
         if (samples >= 8)
      } else if (samples_per_pixel == 4) {
      if (samples >= 2)
         if (samples >= 4)
         if (samples >= 8)
      } else {
                     pvr_csb_pack (&te_regs->mtile1, CR_TE_MTILE1, value) {
      value.x1 = mtile_info->mtile_x1;
   if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      value.x2 = mtile_info->mtile_x2;
                  pvr_csb_pack (&te_regs->mtile2, CR_TE_MTILE2, value) {
      value.y1 = mtile_info->mtile_y1;
   if (!PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      value.y2 = mtile_info->mtile_y2;
                  pvr_csb_pack (&te_regs->screen, CR_TE_SCREEN, value) {
      value.xmax = mtile_info->x_tile_max;
                  }
      VkResult pvr_srv_render_target_dataset_create(
      struct pvr_winsys *ws,
   const struct pvr_winsys_rt_dataset_create_info *create_info,
   const struct pvr_device_info *dev_info,
      {
      const pvr_dev_addr_t macrotile_addrs[ROGUE_FWIF_NUM_RTDATAS] = {
      [0] = create_info->rt_datas[0].macrotile_array_dev_addr,
      };
   const pvr_dev_addr_t pm_mlist_addrs[ROGUE_FWIF_NUM_RTDATAS] = {
      [0] = create_info->rt_datas[0].pm_mlist_dev_addr,
      };
   const pvr_dev_addr_t rgn_header_addrs[ROGUE_FWIF_NUM_RTDATAS] = {
      [0] = create_info->rt_datas[0].rgn_header_dev_addr,
               struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
   struct pvr_srv_winsys_free_list *srv_local_free_list =
         void *free_lists[ROGUE_FW_MAX_FREELISTS] = { NULL };
   struct pvr_srv_winsys_rt_dataset *srv_rt_dataset;
   void *handles[ROGUE_FWIF_NUM_RTDATAS];
   struct pvr_rogue_cr_te rogue_te_regs;
   struct pvr_rt_mtile_info mtile_info;
   uint32_t isp_mtile_size;
                     if (srv_local_free_list->parent) {
      free_lists[ROGUE_FW_GLOBAL_FREELIST] =
               srv_rt_dataset = vk_zalloc(ws->alloc,
                     if (!srv_rt_dataset)
            /* If greater than 1 we'll have to pass in an array. For now just passing in
   * the reference.
   */
   STATIC_ASSERT(ROGUE_FWIF_NUM_GEOMDATAS == 1);
   /* If not 2 the arrays used in the bridge call will require updating. */
            pvr_rt_mtile_info_init(dev_info,
                              isp_mtile_size = pvr_rogue_get_cr_isp_mtile_size_val(dev_info,
                  pvr_rogue_ct_te_init(dev_info,
                        result = pvr_srv_rgx_create_hwrt_dataset(
      ws->render_fd,
   pvr_rogue_get_cr_multisamplectl_val(create_info->samples, true),
   pvr_rogue_get_cr_multisamplectl_val(create_info->samples, false),
   macrotile_addrs,
   pm_mlist_addrs,
   &create_info->rtc_dev_addr,
   rgn_header_addrs,
   &create_info->tpc_dev_addr,
   &create_info->vheap_table_dev_addr,
   free_lists,
   create_info->isp_merge_lower_x,
   create_info->isp_merge_lower_y,
   create_info->isp_merge_scale_x,
   create_info->isp_merge_scale_y,
   create_info->isp_merge_upper_x,
   create_info->isp_merge_upper_y,
   isp_mtile_size,
   rogue_te_regs.mtile_stride,
   pvr_rogue_get_ppp_screen_val(create_info->width, create_info->height),
   create_info->rgn_header_size,
   rogue_te_regs.aa,
   rogue_te_regs.mtile1,
   rogue_te_regs.mtile2,
   rogue_te_regs.screen,
   create_info->tpc_size,
   create_info->tpc_stride,
   create_info->layers,
      if (result != VK_SUCCESS)
            srv_rt_dataset->rt_datas[0].handle = handles[0];
            for (uint32_t i = 0; i < ARRAY_SIZE(srv_rt_dataset->rt_datas); i++) {
      srv_rt_dataset->rt_datas[i].sync_prim = pvr_srv_sync_prim_alloc(srv_ws);
   if (!srv_rt_dataset->rt_datas[i].sync_prim)
                                       err_srv_sync_prim_free:
      for (uint32_t i = 0; i < ARRAY_SIZE(srv_rt_dataset->rt_datas); i++) {
               if (srv_rt_dataset->rt_datas[i].handle) {
      pvr_srv_rgx_destroy_hwrt_dataset(ws->render_fd,
               err_vk_free_srv_rt_dataset:
                  }
      void pvr_srv_render_target_dataset_destroy(
         {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(rt_dataset->ws);
   struct pvr_srv_winsys_rt_dataset *srv_rt_dataset =
            for (uint32_t i = 0; i < ARRAY_SIZE(srv_rt_dataset->rt_datas); i++) {
               if (srv_rt_dataset->rt_datas[i].handle) {
      pvr_srv_rgx_destroy_hwrt_dataset(srv_ws->base.render_fd,
                     }
      static void pvr_srv_render_ctx_fw_static_state_init(
      struct pvr_winsys_render_ctx_create_info *create_info,
      {
      struct pvr_winsys_render_ctx_static_state *ws_static_state =
         struct rogue_fwif_ta_regs_cswitch *regs =
                     regs->vdm_context_state_base_addr = ws_static_state->vdm_ctx_state_base_addr;
            STATIC_ASSERT(ARRAY_SIZE(regs->ta_state) ==
         for (uint32_t i = 0; i < ARRAY_SIZE(ws_static_state->geom_state); i++) {
      regs->ta_state[i].vdm_context_store_task0 =
         regs->ta_state[i].vdm_context_store_task1 =
         regs->ta_state[i].vdm_context_store_task2 =
            regs->ta_state[i].vdm_context_resume_task0 =
         regs->ta_state[i].vdm_context_resume_task1 =
         regs->ta_state[i].vdm_context_resume_task2 =
         }
      VkResult pvr_srv_winsys_render_ctx_create(
      struct pvr_winsys *ws,
   struct pvr_winsys_render_ctx_create_info *create_info,
      {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ws);
            struct rogue_fwif_static_rendercontext_state static_state;
   struct pvr_srv_winsys_render_ctx *srv_ctx;
   const uint32_t call_stack_depth = 1U;
            srv_ctx = vk_zalloc(ws->alloc,
                     if (!srv_ctx)
            result = pvr_srv_create_timeline(ws->render_fd, &srv_ctx->timeline_geom);
   if (result != VK_SUCCESS)
            result = pvr_srv_create_timeline(ws->render_fd, &srv_ctx->timeline_frag);
   if (result != VK_SUCCESS)
                     /* TODO: Add support for reset framework. Currently we subtract
   * reset_cmd.regs size from reset_cmd size to only pass empty flags field.
   */
   result = pvr_srv_rgx_create_render_context(
      ws->render_fd,
   pvr_srv_from_winsys_priority(create_info->priority),
   create_info->vdm_callstack_addr,
   call_stack_depth,
   sizeof(reset_cmd) - sizeof(reset_cmd.regs),
   (uint8_t *)&reset_cmd,
   srv_ws->server_memctx_data,
   sizeof(static_state),
   (uint8_t *)&static_state,
   0,
   RGX_CONTEXT_FLAG_DISABLESLR,
   0,
   UINT_MAX,
   UINT_MAX,
      if (result != VK_SUCCESS)
                                    err_close_timeline_frag:
            err_close_timeline_geom:
            err_free_srv_ctx:
                  }
      void pvr_srv_winsys_render_ctx_destroy(struct pvr_winsys_render_ctx *ctx)
   {
      struct pvr_srv_winsys *srv_ws = to_pvr_srv_winsys(ctx->ws);
   struct pvr_srv_winsys_render_ctx *srv_ctx =
            pvr_srv_rgx_destroy_render_context(srv_ws->base.render_fd, srv_ctx->handle);
   close(srv_ctx->timeline_frag);
   close(srv_ctx->timeline_geom);
      }
      static uint32_t
   pvr_srv_geometry_cmd_stream_load(struct rogue_fwif_cmd_ta *const cmd,
                     {
      const uint32_t *stream_ptr = (const uint32_t *)stream;
   struct rogue_fwif_ta_regs *const regs = &cmd->regs;
   uint32_t main_stream_len =
                     regs->vdm_ctrl_stream_base = *(const uint64_t *)stream_ptr;
            regs->tpu_border_colour_table = *(const uint64_t *)stream_ptr;
            regs->ppp_ctrl = *stream_ptr;
            regs->te_psg = *stream_ptr;
            regs->vdm_context_resume_task0_size = *stream_ptr;
            regs->view_idx = *stream_ptr;
            assert((const uint8_t *)stream_ptr - stream <= stream_len);
               }
      static void pvr_srv_geometry_cmd_ext_stream_load(
      struct rogue_fwif_cmd_ta *const cmd,
   const uint8_t *const stream,
   const uint32_t stream_len,
   const uint32_t ext_stream_offset,
      {
      const uint32_t *ext_stream_ptr =
                           header0 = pvr_csb_unpack(ext_stream_ptr, KMD_STREAM_EXTHDR_GEOM0);
            assert(PVR_HAS_QUIRK(dev_info, 49927) == header0.has_brn49927);
   if (header0.has_brn49927) {
      regs->tpu = *ext_stream_ptr;
                  }
      static void pvr_srv_geometry_cmd_init(
      const struct pvr_winsys_render_submit_info *submit_info,
   const struct pvr_srv_sync_prim *sync_prim,
   struct rogue_fwif_cmd_ta *cmd,
      {
      const struct pvr_winsys_geometry_state *state = &submit_info->geometry;
                              ext_stream_offset = pvr_srv_geometry_cmd_stream_load(cmd,
                        if (ext_stream_offset < state->fw_stream_len) {
      pvr_srv_geometry_cmd_ext_stream_load(cmd,
                                 if (state->flags.is_first_geometry)
            if (state->flags.is_last_geometry)
            if (state->flags.use_single_core)
            cmd->partial_render_ta_3d_fence.ufo_addr.addr =
            }
      static uint32_t
   pvr_srv_fragment_cmd_stream_load(struct rogue_fwif_cmd_3d *const cmd,
                     {
      const uint32_t *stream_ptr = (const uint32_t *)stream;
   struct rogue_fwif_3d_regs *const regs = &cmd->regs;
   uint32_t main_stream_len =
                     regs->isp_scissor_base = *(const uint64_t *)stream_ptr;
            regs->isp_dbias_base = *(const uint64_t *)stream_ptr;
            regs->isp_oclqry_base = *(const uint64_t *)stream_ptr;
            regs->isp_zlsctl = *(const uint64_t *)stream_ptr;
            regs->isp_zload_store_base = *(const uint64_t *)stream_ptr;
            regs->isp_stencil_load_store_base = *(const uint64_t *)stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, requires_fb_cdc_zls_setup)) {
      regs->fb_cdc_zls = *(const uint64_t *)stream_ptr;
               STATIC_ASSERT(ARRAY_SIZE(regs->pbe_word) == 8U);
   STATIC_ASSERT(ARRAY_SIZE(regs->pbe_word[0]) == 3U);
   STATIC_ASSERT(sizeof(regs->pbe_word[0][0]) == sizeof(uint64_t));
   memcpy(regs->pbe_word, stream_ptr, sizeof(regs->pbe_word));
            regs->tpu_border_colour_table = *(const uint64_t *)stream_ptr;
            STATIC_ASSERT(ARRAY_SIZE(regs->pds_bgnd) == 3U);
   STATIC_ASSERT(sizeof(regs->pds_bgnd[0]) == sizeof(uint64_t));
   memcpy(regs->pds_bgnd, stream_ptr, sizeof(regs->pds_bgnd));
            STATIC_ASSERT(ARRAY_SIZE(regs->pds_pr_bgnd) == 3U);
   STATIC_ASSERT(sizeof(regs->pds_pr_bgnd[0]) == sizeof(uint64_t));
   memcpy(regs->pds_pr_bgnd, stream_ptr, sizeof(regs->pds_pr_bgnd));
            STATIC_ASSERT(ARRAY_SIZE(regs->usc_clear_register) == 8U);
   STATIC_ASSERT(sizeof(regs->usc_clear_register[0]) == sizeof(uint32_t));
   memcpy(regs->usc_clear_register,
         stream_ptr,
            regs->usc_pixel_output_ctrl = *stream_ptr;
            regs->isp_bgobjdepth = *stream_ptr;
            regs->isp_bgobjvals = *stream_ptr;
            regs->isp_aa = *stream_ptr;
            regs->isp_ctl = *stream_ptr;
            regs->event_pixel_pds_info = *stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, cluster_grouping)) {
      regs->pixel_phantom = *stream_ptr;
               regs->view_idx = *stream_ptr;
            regs->event_pixel_pds_data = *stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      regs->isp_oclqry_stride = *stream_ptr;
               if (PVR_HAS_FEATURE(dev_info, zls_subtile)) {
      regs->isp_zls_pixels = *stream_ptr;
               cmd->zls_stride = *stream_ptr;
            cmd->sls_stride = *stream_ptr;
            if (PVR_HAS_FEATURE(dev_info, gpu_multicore_support)) {
      cmd->execute_count = *stream_ptr;
               assert((const uint8_t *)stream_ptr - stream <= stream_len);
               }
      static void pvr_srv_fragment_cmd_ext_stream_load(
      struct rogue_fwif_cmd_3d *const cmd,
   const uint8_t *const stream,
   const uint32_t stream_len,
   const uint32_t ext_stream_offset,
      {
      const uint32_t *ext_stream_ptr =
                           header0 = pvr_csb_unpack(ext_stream_ptr, KMD_STREAM_EXTHDR_FRAG0);
            assert(PVR_HAS_QUIRK(dev_info, 49927) == header0.has_brn49927);
   if (header0.has_brn49927) {
      regs->tpu = *ext_stream_ptr;
                  }
      static void
   pvr_srv_fragment_cmd_init(struct rogue_fwif_cmd_3d *cmd,
                     {
                                 ext_stream_offset = pvr_srv_fragment_cmd_stream_load(cmd,
                        if (ext_stream_offset < state->fw_stream_len) {
      pvr_srv_fragment_cmd_ext_stream_load(cmd,
                                 if (state->flags.has_depth_buffer)
            if (state->flags.has_stencil_buffer)
            if (state->flags.prevent_cdm_overlap)
            if (state->flags.use_single_core)
            if (state->flags.get_vis_results)
            if (state->flags.has_spm_scratch_buffer)
      }
      VkResult pvr_srv_winsys_render_submit(
      const struct pvr_winsys_render_ctx *ctx,
   const struct pvr_winsys_render_submit_info *submit_info,
   const struct pvr_device_info *dev_info,
   struct vk_sync *signal_sync_geom,
      {
      const struct pvr_srv_winsys_rt_dataset *srv_rt_dataset =
         struct pvr_srv_sync_prim *sync_prim =
         void *rt_data_handle =
         const struct pvr_srv_winsys_render_ctx *srv_ctx =
                  struct pvr_srv_sync *srv_signal_sync_geom;
            struct rogue_fwif_cmd_ta geom_cmd;
   struct rogue_fwif_cmd_3d frag_cmd = { 0 };
            uint8_t *frag_cmd_ptr = NULL;
            uint32_t current_sync_value = sync_prim->value;
   uint32_t geom_sync_update_value;
   uint32_t frag_to_geom_fence_count = 0;
   uint32_t frag_to_geom_fence_value;
   uint32_t frag_sync_update_count = 0;
            int in_frag_fd = -1;
   int in_geom_fd = -1;
   int fence_frag;
                              pvr_srv_fragment_cmd_init(&pr_cmd,
                        if (submit_info->has_fragment_job) {
      pvr_srv_fragment_cmd_init(&frag_cmd,
                        frag_cmd_ptr = (uint8_t *)&frag_cmd;
               if (submit_info->geometry.wait) {
      struct pvr_srv_sync *srv_wait_sync =
            if (srv_wait_sync->fd >= 0) {
      in_geom_fd = dup(srv_wait_sync->fd);
   if (in_geom_fd == -1) {
      return vk_errorf(NULL,
                                 if (submit_info->fragment.wait) {
      struct pvr_srv_sync *srv_wait_sync =
            if (srv_wait_sync->fd >= 0) {
      in_frag_fd = dup(srv_wait_sync->fd);
   if (in_frag_fd == -1) {
      return vk_errorf(NULL,
                                 if (submit_info->geometry.flags.is_first_geometry) {
      frag_to_geom_fence_count = 1;
               /* Geometery is always kicked */
            if (submit_info->has_fragment_job) {
      frag_sync_update_count = 1;
               do {
      /* The fw allows the ZS and MSAA scratch buffers to be lazily allocated in
   * which case we need to provide a status update (i.e. if they are
   * physically backed or not) to the fw. In our case they will always be
   * physically backed so no need to inform the fw about their status and
   * pass in anything. We'll just pass in NULL.
   */
   result = pvr_srv_rgx_kick_render2(srv_ws->base.render_fd,
                                    srv_ctx->handle,
   frag_to_geom_fence_count,
   &sync_prim->ctx->block_handle,
   &sync_prim->offset,
   &frag_to_geom_fence_value,
   1,
   &sync_prim->ctx->block_handle,
   &sync_prim->offset,
   &geom_sync_update_value,
   frag_sync_update_count,
   &sync_prim->ctx->block_handle,
   &sync_prim->offset,
   &frag_sync_update_value,
   sync_prim->ctx->block_handle,
   sync_prim->offset,
   geom_sync_update_value,
   in_geom_fd,
   srv_ctx->timeline_geom,
   &fence_geom,
   "GEOM",
   in_frag_fd,
   srv_ctx->timeline_frag,
   &fence_frag,
   "FRAG",
   sizeof(geom_cmd),
   (uint8_t *)&geom_cmd,
   sizeof(pr_cmd),
   (uint8_t *)&pr_cmd,
   frag_cmd_size,
   frag_cmd_ptr,
   submit_info->job_num,
   /* Always kick the TA. */
   true,
   /* Always kick a PR. */
   true,
   submit_info->has_fragment_job,
   false,
   0,
   rt_data_handle,
   NULL,
   NULL,
   0,
   NULL,
            if (result != VK_SUCCESS)
            /* The job submission was succesful, update the sync prim value. */
            if (signal_sync_geom) {
      srv_signal_sync_geom = to_srv_sync(signal_sync_geom);
      } else if (fence_geom != -1) {
                  if (signal_sync_frag) {
      srv_signal_sync_frag = to_srv_sync(signal_sync_frag);
      } else if (fence_frag != -1) {
               end_close_in_fds:
      if (in_geom_fd >= 0)
            if (in_frag_fd >= 0)
               }
