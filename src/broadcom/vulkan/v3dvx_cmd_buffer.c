   /*
   * Copyright © 2021 Raspberry Pi Ltd
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "v3dv_private.h"
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/common/v3d_util.h"
   #include "broadcom/cle/v3dx_pack.h"
   #include "broadcom/compiler/v3d_compiler.h"
      #include "util/half_float.h"
   #include "vulkan/util/vk_format.h"
   #include "util/u_pack_color.h"
      void
   v3dX(job_emit_binning_flush)(struct v3dv_job *job)
   {
               v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(FLUSH));
               }
      void
   v3dX(job_emit_enable_double_buffer)(struct v3dv_job *job)
   {
      assert(job->can_use_double_buffer);
   assert(job->frame_tiling.double_buffer);
   assert(!job->frame_tiling.msaa);
            const struct v3dv_frame_tiling *tiling = &job->frame_tiling;
   struct cl_packet_struct(TILE_BINNING_MODE_CFG) config = {
         };
   config.width_in_pixels = tiling->width;
      #if V3D_VERSION == 42
      config.number_of_render_targets = MAX2(tiling->render_target_count, 1);
   config.multisample_mode_4x = tiling->msaa;
   config.double_buffer_in_non_ms_mode = tiling->double_buffer;
      #endif
   #if V3D_VERSION >= 71
         #endif
         uint8_t *rewrite_addr = (uint8_t *)job->bcl_tile_binning_mode_ptr;
      }
      void
   v3dX(job_emit_binning_prolog)(struct v3dv_job *job,
               {
      /* This must go before the binning mode configuration. It is
   * required for layered framebuffers to work.
   */
   cl_emit(&job->bcl, NUMBER_OF_LAYERS, config) {
                  assert(!tiling->double_buffer || !tiling->msaa);
   job->bcl_tile_binning_mode_ptr = cl_start(&job->bcl);
   cl_emit(&job->bcl, TILE_BINNING_MODE_CFG, config) {
      config.width_in_pixels = tiling->width;
   #if V3D_VERSION == 42
         config.number_of_render_targets = MAX2(tiling->render_target_count, 1);
   config.multisample_mode_4x = tiling->msaa;
   config.double_buffer_in_non_ms_mode = tiling->double_buffer;
   #endif
   #if V3D_VERSION >= 71
         config.log2_tile_width = log2_tile_size(tiling->tile_width);
   config.log2_tile_height = log2_tile_size(tiling->tile_height);
   /* FIXME: ideally we would like next assert on the packet header (as is
   * general, so also applies to GL). We would need to expand
   * gen_pack_header for that.
   */
   assert(config.log2_tile_width == config.log2_tile_height ||
   #endif
               /* There's definitely nothing in the VCD cache we want. */
            /* "Binning mode lists must have a Start Tile Binning item (6) after
   *  any prefix state data before the binning list proper starts."
   */
      }
      void
   v3dX(cmd_buffer_end_render_pass_secondary)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      assert(cmd_buffer->state.job);
   v3dv_cl_ensure_space_with_branch(&cmd_buffer->state.job->bcl,
         v3dv_return_if_oom(cmd_buffer, NULL);
      }
      void
   v3dX(job_emit_clip_window)(struct v3dv_job *job, const VkRect2D *rect)
   {
               v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(CLIP_WINDOW));
            cl_emit(&job->bcl, CLIP_WINDOW, clip) {
      clip.clip_window_left_pixel_coordinate = rect->offset.x;
   clip.clip_window_bottom_pixel_coordinate = rect->offset.y;
   clip.clip_window_width_in_pixels = rect->extent.width;
         }
      static void
   cmd_buffer_render_pass_emit_load(struct v3dv_cmd_buffer *cmd_buffer,
                           {
               /* We don't support rendering to ycbcr images, so the image view should be
   * single-plane, and using a single-plane format. But note that the underlying
   * image can be a ycbcr format, as we support rendering to a specific plane
   * of an image. This is used for example on some meta_copy code paths, in
   * order to copy from/to a plane of a ycbcr image.
   */
   assert(iview->plane_count == 1);
            uint8_t image_plane = v3dv_plane_from_aspect(iview->vk.aspects);
   const struct v3d_resource_slice *slice =
            uint32_t layer_offset =
      v3dv_layer_offset(image, iview->vk.base_mip_level,
         cl_emit(cl, LOAD_TILE_BUFFER_GENERAL, load) {
      load.buffer_to_load = buffer;
                     /* If we create an image view with only the stencil format, we
   * re-interpret the format as RGBA8_UINT, as it is want we want in
   * general (see CreateImageView).
   *
   * However, when we are loading/storing tiles from the ZSTENCIL tile
   * buffer, we need to use the underlying DS format.
   */
   if (buffer == ZSTENCIL &&
      iview->format->planes[0].rt_type == V3D_OUTPUT_IMAGE_FORMAT_RGBA8UI) {
   assert(image->format->planes[image_plane].rt_type == V3D_OUTPUT_IMAGE_FORMAT_D24S8);
               load.r_b_swap = iview->planes[0].swap_rb;
   load.channel_reverse = iview->planes[0].channel_reverse;
            if (slice->tiling == V3D_TILING_UIF_NO_XOR ||
      slice->tiling == V3D_TILING_UIF_XOR) {
   load.height_in_ub_or_stride =
      } else if (slice->tiling == V3D_TILING_RASTER) {
                  if (image->vk.samples > VK_SAMPLE_COUNT_1_BIT)
         else
         }
      static inline uint32_t
   v3dv_zs_buffer(bool depth, bool stencil)
   {
      if (depth && stencil)
         else if (depth)
         else if (stencil)
            }
      static void
   cmd_buffer_render_pass_emit_loads(struct v3dv_cmd_buffer *cmd_buffer,
               {
      const struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
   const struct v3dv_render_pass *pass = state->pass;
         assert(!pass->multiview_enabled || layer < MAX_MULTIVIEW_VIEW_COUNT);
         for (uint32_t i = 0; i < subpass->color_count; i++) {
               if (attachment_idx == VK_ATTACHMENT_UNUSED)
            const struct v3dv_render_pass_attachment *attachment =
            /* According to the Vulkan spec:
   *
   *    "The load operation for each sample in an attachment happens before
   *     any recorded command which accesses the sample in the first subpass
   *     where the attachment is used."
   *
   * If the load operation is CLEAR, we must only clear once on the first
   * subpass that uses the attachment (and in that case we don't LOAD).
   * After that, we always want to load so we don't lose any rendering done
   * by a previous subpass to the same attachment. We also want to load
   * if the current job is continuing subpass work started by a previous
   * job, for the same reason.
   *
   * If the render area is not aligned to tile boundaries then we have
   * tiles which are partially covered by it. In this case, we need to
   * load the tiles so we can preserve the pixels that are outside the
   * render area for any such tiles.
   */
   uint32_t first_subpass = !pass->multiview_enabled ?
                  uint32_t last_subpass = !pass->multiview_enabled ?
                  bool needs_load =
      v3dv_cmd_buffer_check_needs_load(state,
                              if (needs_load) {
      struct v3dv_image_view *iview =
         cmd_buffer_render_pass_emit_load(cmd_buffer, cl, iview,
                  uint32_t ds_attachment_idx = subpass->ds_attachment.attachment;
   if (ds_attachment_idx != VK_ATTACHMENT_UNUSED) {
      const struct v3dv_render_pass_attachment *ds_attachment =
            const VkImageAspectFlags ds_aspects =
            uint32_t ds_first_subpass = !pass->multiview_enabled ?
                  uint32_t ds_last_subpass = !pass->multiview_enabled ?
                  const bool needs_depth_load =
      v3dv_cmd_buffer_check_needs_load(state,
                                 const bool needs_stencil_load =
      v3dv_cmd_buffer_check_needs_load(state,
                                 if (needs_depth_load || needs_stencil_load) {
      struct v3dv_image_view *iview =
         /* From the Vulkan spec:
   *
   *   "When an image view of a depth/stencil image is used as a
   *   depth/stencil framebuffer attachment, the aspectMask is ignored
   *   and both depth and stencil image subresources are used."
   *
   * So we ignore the aspects from the subresource range of the image
   * view for the depth/stencil attachment, but we still need to restrict
   * the to aspects compatible with the render pass and the image.
   */
   const uint32_t zs_buffer =
         cmd_buffer_render_pass_emit_load(cmd_buffer, cl,
                     }
      static void
   cmd_buffer_render_pass_emit_store(struct v3dv_cmd_buffer *cmd_buffer,
                                       {
      const struct v3dv_image_view *iview =
                  /* We don't support rendering to ycbcr images, so the image view should be
   * one-plane, and using a single-plane format. But note that the underlying
   * image can be a ycbcr format, as we support rendering to a specific plane
   * of an image. This is used for example on some meta_copy code paths, in
   * order to copy from/to a plane of a ycbcr image.
   */
   assert(iview->plane_count == 1);
            uint8_t image_plane = v3dv_plane_from_aspect(iview->vk.aspects);
   const struct v3d_resource_slice *slice =
         uint32_t layer_offset = v3dv_layer_offset(image,
                        /* The Clear Buffer bit is not supported for Z/Stencil stores in 7.x and it
   * is broken in earlier V3D versions.
   */
            cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
      store.buffer_to_store = buffer;
   store.address = v3dv_cl_address(image->planes[image_plane].mem->bo, layer_offset);
                     /* If we create an image view with only the stencil format, we
   * re-interpret the format as RGBA8_UINT, as it is want we want in
   * general (see CreateImageView).
   *
   * However, when we are loading/storing tiles from the ZSTENCIL tile
   * buffer, we need to use the underlying DS format.
   */
   if (buffer == ZSTENCIL &&
      iview->format->planes[0].rt_type == V3D_OUTPUT_IMAGE_FORMAT_RGBA8UI) {
   assert(image->format->planes[image_plane].rt_type == V3D_OUTPUT_IMAGE_FORMAT_D24S8);
               store.r_b_swap = iview->planes[0].swap_rb;
   store.channel_reverse = iview->planes[0].channel_reverse;
            if (slice->tiling == V3D_TILING_UIF_NO_XOR ||
      slice->tiling == V3D_TILING_UIF_XOR) {
   store.height_in_ub_or_stride =
      } else if (slice->tiling == V3D_TILING_RASTER) {
                  if (image->vk.samples > VK_SAMPLE_COUNT_1_BIT)
         else if (is_multisample_resolve)
         else
         }
      static bool
   check_needs_clear(const struct v3dv_cmd_buffer_state *state,
                     VkImageAspectFlags aspect,
   {
      /* We call this with image->vk.aspects & aspect, so 0 means the aspect we are
   * testing does not exist in the image.
   */
   if (!aspect)
            /* If the aspect needs to be cleared with a draw call then we won't emit
   * the clear here.
   */
   if (do_clear_with_draw)
            /* If this is resuming a subpass started with another job, then attachment
   * load operations don't apply.
   */
   if (state->job->is_subpass_continue)
            /* If the render area is not aligned to tile boundaries we can't use the
   * TLB for a clear.
   */
   if (!state->tile_aligned_render_area)
            /* If this job is running in a subpass other than the first subpass in
   * which this attachment (or view) is used then attachment load operations
   * don't apply.
   */
   if (state->job->first_subpass != first_subpass_idx)
            /* The attachment load operation must be CLEAR */
      }
      static void
   cmd_buffer_render_pass_emit_stores(struct v3dv_cmd_buffer *cmd_buffer,
               {
      struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
   struct v3dv_render_pass *pass = state->pass;
   const struct v3dv_subpass *subpass =
            bool has_stores = false;
   bool use_global_zs_clear = false;
                     /* FIXME: separate stencil */
   uint32_t ds_attachment_idx = subpass->ds_attachment.attachment;
   if (ds_attachment_idx != VK_ATTACHMENT_UNUSED) {
      const struct v3dv_render_pass_attachment *ds_attachment =
            assert(state->job->first_subpass >= ds_attachment->first_subpass);
   assert(state->subpass_idx >= ds_attachment->first_subpass);
            /* From the Vulkan spec, VkImageSubresourceRange:
   *
   *   "When an image view of a depth/stencil image is used as a
   *   depth/stencil framebuffer attachment, the aspectMask is ignored
   *   and both depth and stencil image subresources are used."
   *
   * So we ignore the aspects from the subresource range of the image
   * view for the depth/stencil attachment, but we still need to restrict
   * the to aspects compatible with the render pass and the image.
   */
   const VkImageAspectFlags aspects =
      #if V3D_VERSION <= 42
         /* GFXH-1689: The per-buffer store command's clear buffer bit is broken
   * for depth/stencil.
   *
   * There used to be some confusion regarding the Clear Tile Buffers
   * Z/S bit also being broken, but we confirmed with Broadcom that this
   * is not the case, it was just that some other hardware bugs (that we
   * need to work around, such as GFXH-1461) could cause this bit to behave
   * incorrectly.
   *
   * There used to be another issue where the RTs bit in the Clear Tile
   * Buffers packet also cleared Z/S, but Broadcom confirmed this is
   * fixed since V3D 4.1.
   *
   * So if we have to emit a clear of depth or stencil we don't use
   * the per-buffer store clear bit, even if we need to store the buffers,
   * instead we always have to use the Clear Tile Buffers Z/S bit.
   * If we have configured the job to do early Z/S clearing, then we
   * don't want to emit any Clear Tile Buffers command at all here.
   *
   * Note that GFXH-1689 is not reproduced in the simulator, where
   * using the clear buffer bit in depth/stencil stores works fine.
            /* Only clear once on the first subpass that uses the attachment */
   uint32_t ds_first_subpass = !state->pass->multiview_enabled ?
                  bool needs_depth_clear =
      check_needs_clear(state,
                           bool needs_stencil_clear =
      check_needs_clear(state,
                           use_global_zs_clear = !state->job->early_zs_clear &&
   #endif
   #if V3D_VERSION >= 71
         /* The store command's clear buffer bit cannot be used for Z/S stencil:
   * since V3D 4.5.6 Z/S buffers are automatically cleared between tiles,
   * so we don't want to emit redundant clears here.
   */
   #endif
            /* Skip the last store if it is not required */
   uint32_t ds_last_subpass = !pass->multiview_enabled ?
                  bool needs_depth_store =
      v3dv_cmd_buffer_check_needs_store(state,
                     bool needs_stencil_store =
      v3dv_cmd_buffer_check_needs_store(state,
                     /* If we have a resolve, handle it before storing the tile */
   const struct v3dv_cmd_buffer_attachment_state *ds_att_state =
         if (ds_att_state->use_tlb_resolve) {
      assert(ds_att_state->has_resolve);
   assert(subpass->resolve_depth || subpass->resolve_stencil);
   const uint32_t resolve_attachment_idx =
                  const uint32_t zs_buffer =
         cmd_buffer_render_pass_emit_store(cmd_buffer, cl,
                        } else if (ds_att_state->has_resolve) {
      /* If we can't use the TLB to implement the resolve we will need to
   * store the attachment so we can implement it later using a blit.
   */
   needs_depth_store = subpass->resolve_depth;
               if (needs_depth_store || needs_stencil_store) {
      const uint32_t zs_buffer =
         cmd_buffer_render_pass_emit_store(cmd_buffer, cl,
                              for (uint32_t i = 0; i < subpass->color_count; i++) {
               if (attachment_idx == VK_ATTACHMENT_UNUSED)
            const struct v3dv_render_pass_attachment *attachment =
            assert(state->job->first_subpass >= attachment->first_subpass);
   assert(state->subpass_idx >= attachment->first_subpass);
            /* Only clear once on the first subpass that uses the attachment */
   uint32_t first_subpass = !pass->multiview_enabled ?
                  bool needs_clear =
      check_needs_clear(state,
                           /* Skip the last store if it is not required  */
   uint32_t last_subpass = !pass->multiview_enabled ?
                  bool needs_store =
      v3dv_cmd_buffer_check_needs_store(state,
                     /* If we need to resolve this attachment emit that store first. Notice
   * that we must not request a tile buffer clear here in that case, since
   * that would clear the tile buffer before we get to emit the actual
   * color attachment store below, since the clear happens after the
   * store is completed.
   *
   * If the attachment doesn't support TLB resolves (or the render area
   * is not aligned to tile boundaries) then we will have to fallback to
   * doing the resolve in a shader separately after this job, so we will
   * need to store the multisampled attachment even if that wasn't
   * requested by the client.
   */
   const struct v3dv_cmd_buffer_attachment_state *att_state =
         if (att_state->use_tlb_resolve) {
      assert(att_state->has_resolve);
   const uint32_t resolve_attachment_idx =
         cmd_buffer_render_pass_emit_store(cmd_buffer, cl,
                        } else if (att_state->has_resolve) {
                  /* Emit the color attachment store if needed */
   if (needs_store) {
      cmd_buffer_render_pass_emit_store(cmd_buffer, cl,
                              } else if (needs_clear) {
                     /* We always need to emit at least one dummy store */
   if (!has_stores) {
      cl_emit(cl, STORE_TILE_BUFFER_GENERAL, store) {
                     /* If we have any depth/stencil clears we can't use the per-buffer clear
   * bit and instead we have to emit a single clear of all tile buffers.
   */
      #if V3D_VERSION == 42
         cl_emit(cl, CLEAR_TILE_BUFFERS, clear) {
      clear.clear_z_stencil_buffer = use_global_zs_clear;
      #endif
   #if V3D_VERSION >= 71
         #endif
         }
      static void
   cmd_buffer_render_pass_emit_per_tile_rcl(struct v3dv_cmd_buffer *cmd_buffer,
         {
      struct v3dv_job *job = cmd_buffer->state.job;
            /* Emit the generic list in our indirect state -- the rcl will just
   * have pointers into it.
   */
   struct v3dv_cl *cl = &job->indirect;
   v3dv_cl_ensure_space(cl, 200, 1);
                                       /* The binner starts out writing tiles assuming that the initial mode
   * is triangles, so make sure that's the case.
   */
   cl_emit(cl, PRIM_LIST_FORMAT, fmt) {
                  /* PTB assumes that value to be 0, but hw will not set it. */
   cl_emit(cl, SET_INSTANCEID, set) {
                                                      cl_emit(&job->rcl, START_ADDRESS_OF_GENERIC_TILE_LIST, branch) {
      branch.start = tile_list_start;
         }
      static void
   cmd_buffer_emit_render_pass_layer_rcl(struct v3dv_cmd_buffer *cmd_buffer,
         {
               struct v3dv_job *job = cmd_buffer->state.job;
            /* If doing multicore binning, we would need to initialize each
   * core's tile list here.
   */
   const struct v3dv_frame_tiling *tiling = &job->frame_tiling;
   const uint32_t tile_alloc_offset =
         cl_emit(rcl, MULTICORE_RENDERING_TILE_LIST_SET_BASE, list) {
                           uint32_t supertile_w_in_pixels =
         uint32_t supertile_h_in_pixels =
         const uint32_t min_x_supertile =
         const uint32_t min_y_supertile =
            uint32_t max_render_x = state->render_area.offset.x;
   if (state->render_area.extent.width > 0)
         uint32_t max_render_y = state->render_area.offset.y;
   if (state->render_area.extent.height > 0)
         const uint32_t max_x_supertile = max_render_x / supertile_w_in_pixels;
            for (int y = min_y_supertile; y <= max_y_supertile; y++) {
      for (int x = min_x_supertile; x <= max_x_supertile; x++) {
      cl_emit(rcl, SUPERTILE_COORDINATES, coords) {
      coords.column_number_in_supertiles = x;
               }
      static void
   set_rcl_early_z_config(struct v3dv_job *job,
               {
      /* Disable if none of the draw calls in this job enabled EZ */
   if (!job->has_ez_draws) {
      *early_z_disable = true;
               switch (job->first_ez_state) {
   case V3D_EZ_UNDECIDED:
   case V3D_EZ_LT_LE:
      *early_z_disable = false;
   *early_z_test_and_update_direction = EARLY_Z_DIRECTION_LT_LE;
      case V3D_EZ_GT_GE:
      *early_z_disable = false;
   *early_z_test_and_update_direction = EARLY_Z_DIRECTION_GT_GE;
      case V3D_EZ_DISABLED:
      *early_z_disable = true;
         }
      /* Note that for v71, render target cfg packets has just one field that
   * combined the internal type and clamp mode. For simplicity we keep just one
   * helper.
   *
   * Note: rt_type is in fact a "enum V3DX(Internal_Type)".
   *
   * FIXME: for v71 we are not returning all the possible combinations for
   * render target internal type and clamp. For example for int types we are
   * always using clamp int, and for 16f we are using clamp none or pos (that
   * seems to be the equivalent for no-clamp on 4.2), but not pq or hlg. In
   * summary right now we are just porting what we were doing on 4.2
   */
   uint32_t
   v3dX(clamp_for_format_and_type)(uint32_t rt_type,
         {
   #if V3D_VERSION == 42
      if (vk_format_is_int(vk_format))
         else if (vk_format_is_srgb(vk_format))
         else
      #endif
   #if V3D_VERSION >= 71
      switch (rt_type) {
   case V3D_INTERNAL_TYPE_8I:
         case V3D_INTERNAL_TYPE_8UI:
         case V3D_INTERNAL_TYPE_8:
         case V3D_INTERNAL_TYPE_16I:
         case V3D_INTERNAL_TYPE_16UI:
         case V3D_INTERNAL_TYPE_16F:
      return vk_format_is_srgb(vk_format) ?
      V3D_RENDER_TARGET_TYPE_CLAMP_16F_CLAMP_NORM :
   case V3D_INTERNAL_TYPE_32I:
         case V3D_INTERNAL_TYPE_32UI:
         case V3D_INTERNAL_TYPE_32F:
         default:
                     #endif
   }
      static void
   cmd_buffer_render_pass_setup_render_target(struct v3dv_cmd_buffer *cmd_buffer,
               #if V3D_VERSION == 42
               #else
         #endif
   {
               assert(state->subpass_idx < state->pass->subpass_count);
   const struct v3dv_subpass *subpass =
            if (rt >= subpass->color_count)
            struct v3dv_subpass_attachment *attachment = &subpass->color_attachments[rt];
   const uint32_t attachment_idx = attachment->attachment;
   if (attachment_idx == VK_ATTACHMENT_UNUSED)
            assert(attachment_idx < state->framebuffer->attachment_count &&
         struct v3dv_image_view *iview = state->attachments[attachment_idx].image_view;
            assert(iview->plane_count == 1);
      #if V3D_VERSION == 42
      *rt_type = iview->planes[0].internal_type;
   *rt_clamp = v3dX(clamp_for_format_and_type)(iview->planes[0].internal_type,
      #endif
   #if V3D_VERSION >= 71
      *rt_type_clamp = v3dX(clamp_for_format_and_type)(iview->planes[0].internal_type,
      #endif
   }
      void
   v3dX(cmd_buffer_emit_render_pass_rcl)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            const struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
            /* We can't emit the RCL until we have a framebuffer, which we may not have
   * if we are recording a secondary command buffer. In that case, we will
   * have to wait until vkCmdExecuteCommands is called from a primary command
   * buffer.
   */
   if (!framebuffer) {
      assert(cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
                                 v3dv_cl_ensure_space_with_branch(&job->rcl, 200 +
                        assert(state->subpass_idx < state->pass->subpass_count);
   const struct v3dv_render_pass *pass = state->pass;
   const struct v3dv_subpass *subpass = &pass->subpasses[state->subpass_idx];
            /* Common config must be the first TILE_RENDERING_MODE_CFG and
   * Z_STENCIL_CLEAR_VALUES must be last. The ones in between are optional
   * updates to the previous HW state.
   */
   bool do_early_zs_clear = false;
   const uint32_t ds_attachment_idx = subpass->ds_attachment.attachment;
   assert(!tiling->msaa || !tiling->double_buffer);
   cl_emit(rcl, TILE_RENDERING_MODE_CFG_COMMON, config) {
      config.image_width_pixels = framebuffer->width;
   config.image_height_pixels = framebuffer->height;
   config.number_of_render_targets = MAX2(subpass->color_count, 1);
   config.multisample_mode_4x = tiling->msaa;
   #if V3D_VERSION == 42
         #endif
   #if V3D_VERSION >= 71
         config.log2_tile_width = log2_tile_size(tiling->tile_width);
   config.log2_tile_height = log2_tile_size(tiling->tile_height);
   /* FIXME: ideallly we would like next assert on the packet header (as is
   * general, so also applies to GL). We would need to expand
   * gen_pack_header for that.
   */
   assert(config.log2_tile_width == config.log2_tile_height ||
   #endif
            if (ds_attachment_idx != VK_ATTACHMENT_UNUSED) {
                     /* At this point the image view should be single-plane. But note that
   * the underlying image can be multi-plane, and the image view refer
   * to one specific plane.
   */
   assert(iview->plane_count == 1);
                  set_rcl_early_z_config(job,
                  /* Early-Z/S clear can be enabled if the job is clearing and not
   * storing (or loading) depth. If a stencil aspect is also present
   * we have the same requirements for it, however, in this case we
   * can accept stencil loadOp DONT_CARE as well, so instead of
   * checking that stencil is cleared we check that is not loaded.
   *
   * Early-Z/S clearing is independent of Early Z/S testing, so it is
   * possible to enable one but not the other so long as their
   * respective requirements are met.
   *
   * From V3D 4.5.6, Z/S buffers are always cleared automatically
   * between tiles, but we still want to enable early ZS clears
   * when Z/S are not loaded or stored.
   */
                                 bool needs_depth_store =
      v3dv_cmd_buffer_check_needs_store(state,
               #if V3D_VERSION <= 42
            bool needs_depth_clear =
      check_needs_clear(state,
                        #endif
   #if V3D_VERSION >= 71
            bool needs_depth_load =
      v3dv_cmd_buffer_check_needs_load(state,
                           #endif
               if (do_early_zs_clear &&
      vk_format_has_stencil(ds_attachment->desc.format)) {
   bool needs_stencil_load =
      v3dv_cmd_buffer_check_needs_load(state,
                                 bool needs_stencil_store =
      v3dv_cmd_buffer_check_needs_store(state,
                                          } else {
                     /* If we enabled early Z/S clear, then we can't emit any "Clear Tile Buffers"
   * commands with the Z/S bit set, so keep track of whether we enabled this
   * in the job so we can skip these later.
   */
         #if V3D_VERSION >= 71
         #endif
      for (uint32_t i = 0; i < subpass->color_count; i++) {
      uint32_t attachment_idx = subpass->color_attachments[i].attachment;
   #if V3D_VERSION >= 71
            cl_emit(rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1, rt) {
      rt.render_target_number = i;
   #endif
                        struct v3dv_image_view *iview =
                           uint8_t plane = v3dv_plane_from_aspect(iview->vk.aspects);
   const struct v3d_resource_slice *slice =
            UNUSED const uint32_t *clear_color =
            UNUSED uint32_t clear_pad = 0;
   if (slice->tiling == V3D_TILING_UIF_NO_XOR ||
                                    if (slice->padded_height_of_output_image_in_uif_blocks -
      implicit_padded_height >= 15) {
            #if V3D_VERSION == 42
         cl_emit(rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART1, clear) {
      clear.clear_color_low_32_bits = clear_color[0];
   clear.clear_color_next_24_bits = clear_color[1] & 0xffffff;
               if (iview->planes[0].internal_bpp >= V3D_INTERNAL_BPP_64) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART2, clear) {
      clear.clear_color_mid_low_32_bits =
         clear.clear_color_mid_high_24_bits =
                        if (iview->planes[0].internal_bpp >= V3D_INTERNAL_BPP_128 || clear_pad) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_CLEAR_COLORS_PART3, clear) {
      clear.uif_padded_height_in_uif_blocks = clear_pad;
   clear.clear_color_high_16_bits = clear_color[3] >> 16;
         #endif
      #if V3D_VERSION >= 71
         cl_emit(rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1, rt) {
      rt.clear_color_low_bits = clear_color[0];
   cmd_buffer_render_pass_setup_render_target(cmd_buffer, i, &rt.internal_bpp,
         rt.stride =
      v3d_compute_rt_row_row_stride_128_bits(tiling->tile_width,
                     /* base_addr in multiples of 512 bits. We divide by 8 because stride
   * is in 128-bit units, but it is packing 2 rows worth of data, so we
   * need to divide it by 2 so it is only 1 row, and then again by 4 so
   * it is in 512-bit units.
   */
               if (iview->planes[0].internal_bpp >= V3D_INTERNAL_BPP_64) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART2, rt) {
      rt.clear_color_mid_bits = /* 40 bits (32 + 8)  */
      ((uint64_t) clear_color[1]) |
                     if (iview->planes[0].internal_bpp >= V3D_INTERNAL_BPP_128) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART3, rt) {
      rt.clear_color_top_bits = /* 56 bits (24 + 32) */
      (((uint64_t) (clear_color[2] & 0xffffff00)) >> 8) |
            #endif
            #if V3D_VERSION >= 71
      /* If we don't have any color RTs, we still need to emit one and flag
   * it as not used using stride = 1.
   */
   if (subpass->color_count == 0) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_RENDER_TARGET_PART1, rt) {
               #endif
      #if V3D_VERSION == 42
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_COLOR, rt) {
      cmd_buffer_render_pass_setup_render_target
      (cmd_buffer, 0, &rt.render_target_0_internal_bpp,
      cmd_buffer_render_pass_setup_render_target
      (cmd_buffer, 1, &rt.render_target_1_internal_bpp,
      cmd_buffer_render_pass_setup_render_target
      (cmd_buffer, 2, &rt.render_target_2_internal_bpp,
      cmd_buffer_render_pass_setup_render_target
      (cmd_buffer, 3, &rt.render_target_3_internal_bpp,
      #endif
         /* Ends rendering mode config. */
   if (ds_attachment_idx != VK_ATTACHMENT_UNUSED) {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_ZS_CLEAR_VALUES, clear) {
      clear.z_clear_value =
         clear.stencil_clear_value =
         } else {
      cl_emit(rcl, TILE_RENDERING_MODE_CFG_ZS_CLEAR_VALUES, clear) {
      clear.z_clear_value = 1.0f;
                  /* Always set initial block size before the first branch, which needs
   * to match the value from binning mode config.
   */
   cl_emit(rcl, TILE_LIST_INITIAL_BLOCK_SIZE, init) {
      init.use_auto_chained_tile_lists = true;
   init.size_of_first_block_in_chained_tile_lists =
               cl_emit(rcl, MULTICORE_RENDERING_SUPERTILE_CFG, config) {
      config.number_of_bin_tile_lists = 1;
   config.total_frame_width_in_tiles = tiling->draw_tiles_x;
            config.supertile_width_in_tiles = tiling->supertile_width;
            config.total_frame_width_in_supertiles =
         config.total_frame_height_in_supertiles =
               /* Emit an initial clear of the tile buffers. This is necessary
   * for any buffers that should be cleared (since clearing
   * normally happens at the *end* of the generic tile list), but
   * it's also nice to clear everything so the first tile doesn't
   * inherit any contents from some previous frame.
   *
   * Also, implement the GFXH-1742 workaround. There's a race in
   * the HW between the RCL updating the TLB's internal type/size
   * and the spawning of the QPU instances using the TLB's current
   * internal type/size. To make sure the QPUs get the right
   * state, we need 1 dummy store in between internal type/size
   * changes on V3D 3.x, and 2 dummy stores on 4.x.
   */
   for (int i = 0; i < 2; i++) {
      cl_emit(rcl, TILE_COORDINATES, coords);
   cl_emit(rcl, END_OF_LOADS, end);
   cl_emit(rcl, STORE_TILE_BUFFER_GENERAL, store) {
         }
   if (cmd_buffer->state.tile_aligned_render_area &&
   #if V3D_VERSION == 42
            cl_emit(rcl, CLEAR_TILE_BUFFERS, clear) {
      clear.clear_z_stencil_buffer = !job->early_zs_clear;
   #endif
   #if V3D_VERSION >= 71
         #endif
         }
                        for (int layer = 0; layer < MAX2(1, fb_layers); layer++) {
      if (subpass->view_mask == 0 || (subpass->view_mask & (1u << layer)))
                  }
      void
   v3dX(viewport_compute_xform)(const VkViewport *viewport,
               {
      float x = viewport->x;
   float y = viewport->y;
   float half_width = 0.5f * viewport->width;
   float half_height = 0.5f * viewport->height;
   double n = viewport->minDepth;
            scale[0] = half_width;
   translate[0] = half_width + x;
   scale[1] = half_height;
            scale[2] = (f - n);
            /* It seems that if the scale is small enough the hardware won't clip
   * correctly so we work around this my choosing the smallest scale that
   * seems to work.
   *
   * This case is exercised by CTS:
   * dEQP-VK.draw.renderpass.inverted_depth_ranges.nodepthclamp_deltazero
   *
   * V3D 7.x fixes this by using the new
   * CLIPPER_Z_SCALE_AND_OFFSET_NO_GUARDBAND.
      #if V3D_VERSION <= 42
      const float min_abs_scale = 0.0005f;
   if (fabs(scale[2]) < min_abs_scale)
      #endif
   }
      void
   v3dX(cmd_buffer_emit_viewport)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_dynamic_state *dynamic = &cmd_buffer->state.dynamic;
   struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            /* FIXME: right now we only support one viewport. viewporst[0] would work
   * now, would need to change if we allow multiple viewports
   */
   float *vptranslate = dynamic->viewport.translate[0];
            struct v3dv_job *job = cmd_buffer->state.job;
            const uint32_t required_cl_size =
      cl_packet_length(CLIPPER_XY_SCALING) +
   cl_packet_length(CLIPPER_Z_SCALE_AND_OFFSET) +
   cl_packet_length(CLIPPER_Z_MIN_MAX_CLIPPING_PLANES) +
      v3dv_cl_ensure_space_with_branch(&job->bcl, required_cl_size);
         #if V3D_VERSION == 42
      cl_emit(&job->bcl, CLIPPER_XY_SCALING, clip) {
      clip.viewport_half_width_in_1_256th_of_pixel = vpscale[0] * 256.0f;
         #endif
   #if V3D_VERSION >= 71
      cl_emit(&job->bcl, CLIPPER_XY_SCALING, clip) {
      clip.viewport_half_width_in_1_64th_of_pixel = vpscale[0] * 64.0f;
         #endif
         float translate_z, scale_z;
   v3dv_cmd_buffer_state_get_viewport_z_xform(&cmd_buffer->state, 0,
         #if V3D_VERSION == 42
      cl_emit(&job->bcl, CLIPPER_Z_SCALE_AND_OFFSET, clip) {
      clip.viewport_z_offset_zc_to_zs = translate_z;
         #endif
      #if V3D_VERSION >= 71
      /* If the Z scale is too small guardband clipping may not clip correctly */
   if (fabsf(scale_z) < 0.01f) {
      cl_emit(&job->bcl, CLIPPER_Z_SCALE_AND_OFFSET_NO_GUARDBAND, clip) {
      clip.viewport_z_offset_zc_to_zs = translate_z;
         } else {
      cl_emit(&job->bcl, CLIPPER_Z_SCALE_AND_OFFSET, clip) {
      clip.viewport_z_offset_zc_to_zs = translate_z;
            #endif
         cl_emit(&job->bcl, CLIPPER_Z_MIN_MAX_CLIPPING_PLANES, clip) {
      /* Vulkan's default Z NDC is [0..1]. If 'negative_one_to_one' is enabled,
   * we are using OpenGL's [-1, 1] instead.
   */
   float z1 = pipeline->negative_one_to_one ? translate_z - scale_z :
         float z2 = translate_z + scale_z;
   clip.minimum_zw = MIN2(z1, z2);
               cl_emit(&job->bcl, VIEWPORT_OFFSET, vp) {
      float vp_fine_x = vptranslate[0];
   float vp_fine_y = vptranslate[1];
   int32_t vp_coarse_x = 0;
            /* The fine coordinates must be unsigned, but coarse can be signed */
   if (unlikely(vp_fine_x < 0)) {
      int32_t blocks_64 = DIV_ROUND_UP(fabsf(vp_fine_x), 64);
   vp_fine_x += 64.0f * blocks_64;
               if (unlikely(vp_fine_y < 0)) {
      int32_t blocks_64 = DIV_ROUND_UP(fabsf(vp_fine_y), 64);
   vp_fine_y += 64.0f * blocks_64;
               vp.fine_x = vp_fine_x;
   vp.fine_y = vp_fine_y;
   vp.coarse_x = vp_coarse_x;
                  }
      void
   v3dX(cmd_buffer_emit_stencil)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            const uint32_t dynamic_stencil_states = V3DV_DYNAMIC_STENCIL_COMPARE_MASK |
      V3DV_DYNAMIC_STENCIL_WRITE_MASK |
         v3dv_cl_ensure_space_with_branch(&job->bcl,
                  bool emitted_stencil = false;
   for (uint32_t i = 0; i < 2; i++) {
      if (pipeline->emit_stencil_cfg[i]) {
      if (dynamic_state->mask & dynamic_stencil_states) {
      cl_emit_with_prepacked(&job->bcl, STENCIL_CFG,
            if (dynamic_state->mask & V3DV_DYNAMIC_STENCIL_COMPARE_MASK) {
      config.stencil_test_mask =
      i == 0 ? dynamic_state->stencil_compare_mask.front :
   }
   if (dynamic_state->mask & V3DV_DYNAMIC_STENCIL_WRITE_MASK) {
      config.stencil_write_mask =
      i == 0 ? dynamic_state->stencil_write_mask.front :
   }
   if (dynamic_state->mask & V3DV_DYNAMIC_STENCIL_REFERENCE) {
      config.stencil_ref_value =
      i == 0 ? dynamic_state->stencil_reference.front :
         } else {
                                 if (emitted_stencil) {
      const uint32_t dynamic_stencil_dirty_flags =
      V3DV_CMD_DIRTY_STENCIL_COMPARE_MASK |
   V3DV_CMD_DIRTY_STENCIL_WRITE_MASK |
            }
      void
   v3dX(cmd_buffer_emit_depth_bias)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            if (!pipeline->depth_bias.enabled)
            struct v3dv_job *job = cmd_buffer->state.job;
            v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(DEPTH_OFFSET));
            struct v3dv_dynamic_state *dynamic = &cmd_buffer->state.dynamic;
   cl_emit(&job->bcl, DEPTH_OFFSET, bias) {
      bias.depth_offset_factor = dynamic->depth_bias.slope_factor;
   #if V3D_VERSION <= 42
         if (pipeline->depth_bias.is_z16)
   #endif
                        }
      void
   v3dX(cmd_buffer_emit_depth_bounds)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      /* No depthBounds support for v42, so this method is empty in that case.
   *
   * Note that this method is being called as v3dv_job_init flags all state
   * as dirty. See FIXME note in v3dv_job_init.
         #if V3D_VERSION >= 71
      struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            if (!pipeline->depth_bounds_test_enabled)
            struct v3dv_job *job = cmd_buffer->state.job;
            v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(DEPTH_BOUNDS_TEST_LIMITS));
            struct v3dv_dynamic_state *dynamic = &cmd_buffer->state.dynamic;
   cl_emit(&job->bcl, DEPTH_BOUNDS_TEST_LIMITS, bounds) {
      bounds.lower_test_limit = dynamic->depth_bounds.min;
                  #endif
   }
      void
   v3dX(cmd_buffer_emit_line_width)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(LINE_WIDTH));
            cl_emit(&job->bcl, LINE_WIDTH, line) {
                     }
      void
   v3dX(cmd_buffer_emit_sample_state)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            struct v3dv_job *job = cmd_buffer->state.job;
            v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(SAMPLE_STATE));
            cl_emit(&job->bcl, SAMPLE_STATE, state) {
      state.coverage = 1.0f;
         }
      void
   v3dX(cmd_buffer_emit_blend)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            const struct v3d_device_info *devinfo = &cmd_buffer->device->devinfo;
            const uint32_t blend_packets_size =
      cl_packet_length(BLEND_ENABLES) +
   cl_packet_length(BLEND_CONSTANT_COLOR) +
         v3dv_cl_ensure_space_with_branch(&job->bcl, blend_packets_size);
            if (cmd_buffer->state.dirty & V3DV_CMD_DIRTY_PIPELINE) {
      if (pipeline->blend.enables) {
      cl_emit(&job->bcl, BLEND_ENABLES, enables) {
                     for (uint32_t i = 0; i < max_color_rts; i++) {
      if (pipeline->blend.enables & (1 << i))
                  if (pipeline->blend.needs_color_constants &&
      cmd_buffer->state.dirty & V3DV_CMD_DIRTY_BLEND_CONSTANTS) {
   struct v3dv_dynamic_state *dynamic = &cmd_buffer->state.dynamic;
   cl_emit(&job->bcl, BLEND_CONSTANT_COLOR, color) {
      color.red_f16 = _mesa_float_to_half(dynamic->blend_constants[0]);
   color.green_f16 = _mesa_float_to_half(dynamic->blend_constants[1]);
   color.blue_f16 = _mesa_float_to_half(dynamic->blend_constants[2]);
      }
         }
      void
   v3dX(cmd_buffer_emit_color_write_mask)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   struct v3dv_dynamic_state *dynamic = &cmd_buffer->state.dynamic;
   uint32_t color_write_mask = ~dynamic->color_write_enable |
      #if V3D_VERSION <= 42
      /* Only 4 RTs */
      #endif
         cl_emit(&job->bcl, COLOR_WRITE_MASKS, mask) {
                     }
      static void
   emit_flat_shade_flags(struct v3dv_job *job,
                           {
      v3dv_cl_ensure_space_with_branch(&job->bcl,
                  cl_emit(&job->bcl, FLAT_SHADE_FLAGS, flags) {
      flags.varying_offset_v0 = varying_offset;
   flags.flat_shade_flags_for_varyings_v024 = varyings;
   flags.action_for_flat_shade_flags_of_lower_numbered_varyings = lower;
         }
      static void
   emit_noperspective_flags(struct v3dv_job *job,
                           {
      v3dv_cl_ensure_space_with_branch(&job->bcl,
                  cl_emit(&job->bcl, NON_PERSPECTIVE_FLAGS, flags) {
      flags.varying_offset_v0 = varying_offset;
   flags.non_perspective_flags_for_varyings_v024 = varyings;
   flags.action_for_non_perspective_flags_of_lower_numbered_varyings = lower;
         }
      static void
   emit_centroid_flags(struct v3dv_job *job,
                     int varying_offset,
   {
      v3dv_cl_ensure_space_with_branch(&job->bcl,
                  cl_emit(&job->bcl, CENTROID_FLAGS, flags) {
      flags.varying_offset_v0 = varying_offset;
   flags.centroid_flags_for_varyings_v024 = varyings;
   flags.action_for_centroid_flags_of_lower_numbered_varyings = lower;
         }
      static bool
   emit_varying_flags(struct v3dv_job *job,
                     uint32_t num_flags,
   const uint32_t *flags,
   void (*flag_emit_callback)(struct v3dv_job *job,
         {
      bool emitted_any = false;
   for (int i = 0; i < num_flags; i++) {
      if (!flags[i])
            if (emitted_any) {
      flag_emit_callback(job, i, flags[i],
            } else if (i == 0) {
      flag_emit_callback(job, i, flags[i],
            } else {
      flag_emit_callback(job, i, flags[i],
                                    }
      void
   v3dX(cmd_buffer_emit_varyings_state)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3d_fs_prog_data *prog_data_fs =
            const uint32_t num_flags =
         const uint32_t *flat_shade_flags = prog_data_fs->flat_shade_flags;
   const uint32_t *noperspective_flags =  prog_data_fs->noperspective_flags;
            if (!emit_varying_flags(job, num_flags, flat_shade_flags,
            v3dv_cl_ensure_space_with_branch(
                              if (!emit_varying_flags(job, num_flags, noperspective_flags,
            v3dv_cl_ensure_space_with_branch(
                              if (!emit_varying_flags(job, num_flags, centroid_flags,
            v3dv_cl_ensure_space_with_branch(
                        }
      /* Updates job early Z state tracking. Returns False if EZ must be disabled
   * for the current draw call.
   */
   static bool
   job_update_ez_state(struct v3dv_job *job,
               {
      /* If first_ez_state is V3D_EZ_DISABLED it means that we have already
   * determined that we should disable EZ completely for all draw calls in
   * this job. This will cause us to disable EZ for the entire job in the
   * Tile Rendering Mode RCL packet and when we do that we need to make sure
   * we never emit a draw call in the job with EZ enabled in the CFG_BITS
   * packet, so ez_state must also be V3D_EZ_DISABLED;
   */
   if (job->first_ez_state == V3D_EZ_DISABLED) {
      assert(job->ez_state == V3D_EZ_DISABLED);
               /* If ez_state is V3D_EZ_DISABLED it means that we have already decided
   * that EZ must be disabled for the remaining of the frame.
   */
   if (job->ez_state == V3D_EZ_DISABLED)
            /* This is part of the pre draw call handling, so we should be inside a
   * render pass.
   */
            /* If this is the first time we update EZ state for this job we first check
   * if there is anything that requires disabling it completely for the entire
   * job (based on state that is not related to the current draw call and
   * pipeline state).
   */
   if (!job->decided_global_ez_enable) {
               struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
   assert(state->subpass_idx < state->pass->subpass_count);
   struct v3dv_subpass *subpass = &state->pass->subpasses[state->subpass_idx];
   if (subpass->ds_attachment.attachment == VK_ATTACHMENT_UNUSED) {
      job->first_ez_state = V3D_EZ_DISABLED;
   job->ez_state = V3D_EZ_DISABLED;
               /* GFXH-1918: the early-z buffer may load incorrect depth values
   * if the frame has odd width or height.
   *
   * So we need to disable EZ in this case.
   */
   const struct v3dv_render_pass_attachment *ds_attachment =
            const VkImageAspectFlags ds_aspects =
            bool needs_depth_load =
      v3dv_cmd_buffer_check_needs_load(state,
                                 if (needs_depth_load) {
               if (!fb) {
      assert(cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY);
   perf_debug("Loading depth aspect in a secondary command buffer "
         job->first_ez_state = V3D_EZ_DISABLED;
   job->ez_state = V3D_EZ_DISABLED;
               if (((fb->width % 2) != 0 || (fb->height % 2) != 0)) {
      perf_debug("Loading depth aspect for framebuffer with odd width "
         job->first_ez_state = V3D_EZ_DISABLED;
   job->ez_state = V3D_EZ_DISABLED;
                     /* Otherwise, we can decide to selectively enable or disable EZ for draw
   * calls using the CFG_BITS packet based on the bound pipeline state.
   */
   bool disable_ez = false;
   bool incompatible_test = false;
   switch (pipeline->ez_state) {
   case V3D_EZ_UNDECIDED:
      /* If the pipeline didn't pick a direction but didn't disable, then go
   * along with the current EZ state. This allows EZ optimization for Z
   * func == EQUAL or NEVER.
   */
         case V3D_EZ_LT_LE:
   case V3D_EZ_GT_GE:
      /* If the pipeline picked a direction, then it needs to match the current
   * direction if we've decided on one.
   */
   if (job->ez_state == V3D_EZ_UNDECIDED) {
         } else if (job->ez_state != pipeline->ez_state) {
      disable_ez = true;
      }
         case V3D_EZ_DISABLED:
         disable_ez = true;
                  if (job->first_ez_state == V3D_EZ_UNDECIDED && !disable_ez) {
      assert(job->ez_state != V3D_EZ_DISABLED);
               /* If we had to disable EZ because of an incompatible test direction and
   * and the pipeline writes depth then we need to disable EZ for the rest of
   * the frame.
   */
   if (incompatible_test && pipeline->z_updates_enable) {
      assert(disable_ez);
               if (!disable_ez)
               }
      void
   v3dX(cmd_buffer_emit_configuration_bits)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            v3dv_cl_ensure_space_with_branch(&job->bcl, cl_packet_length(CFG_BITS));
               #if V3D_VERSION == 42
         bool enable_ez = job_update_ez_state(job, pipeline, cmd_buffer);
   config.early_z_enable = enable_ez;
   config.early_z_updates_enable = config.early_z_enable &&
   #endif
         }
      void
   v3dX(cmd_buffer_emit_occlusion_query)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            v3dv_cl_ensure_space_with_branch(&job->bcl,
                  cl_emit(&job->bcl, OCCLUSION_QUERY_COUNTER, counter) {
      if (cmd_buffer->state.query.active_query.bo) {
      counter.address =
      v3dv_cl_address(cmd_buffer->state.query.active_query.bo,
                  }
      static struct v3dv_job *
   cmd_buffer_subpass_split_for_barrier(struct v3dv_cmd_buffer *cmd_buffer,
         {
      assert(cmd_buffer->state.subpass_idx != -1);
   v3dv_cmd_buffer_finish_job(cmd_buffer);
   struct v3dv_job *job =
      v3dv_cmd_buffer_subpass_resume(cmd_buffer,
      if (!job)
            /* FIXME: we can do better than all barriers */
   job->serialize = V3DV_BARRIER_ALL;
   job->needs_bcl_sync = is_bcl_barrier;
      }
      static void
   cmd_buffer_copy_secondary_end_query_state(struct v3dv_cmd_buffer *primary,
         {
      struct v3dv_cmd_buffer_state *p_state = &primary->state;
            const uint32_t total_state_count =
         v3dv_cmd_buffer_ensure_array_state(primary,
                                    for (uint32_t i = 0; i < s_state->query.end.used_count; i++) {
      const struct v3dv_end_query_info *s_qstate =
            struct v3dv_end_query_info *p_qstate =
            p_qstate->pool = s_qstate->pool;
         }
      void
   v3dX(cmd_buffer_execute_inside_pass)(struct v3dv_cmd_buffer *primary,
               {
               /* Typically we postpone applying binning syncs until we see a draw call
   * that may actually access proteted resources in the binning stage. However,
   * if the draw calls are recorded in a secondary command buffer and the
   * barriers were recorded in a primary command buffer, that won't work
   * and we will have to check if we need a binning sync when executing the
   * secondary.
   */
   struct v3dv_job *primary_job = primary->state.job;
   if (primary_job->serialize &&
      (primary->state.barrier.bcl_buffer_access ||
   primary->state.barrier.bcl_image_access)) {
               /* Emit occlusion query state if needed so the draw calls inside our
   * secondaries update the counters.
   */
   bool has_occlusion_query =
         if (has_occlusion_query)
            /* FIXME: if our primary job tiling doesn't enable MSSA but any of the
   * pipelines used by the secondaries do, we need to re-start the primary
   * job to enable MSAA. See cmd_buffer_restart_job_for_msaa_if_needed.
   */
   struct v3dv_barrier_state pending_barrier = { 0 };
   for (uint32_t i = 0; i < cmd_buffer_count; i++) {
               assert(secondary->usage_flags &
            list_for_each_entry(struct v3dv_job, secondary_job,
            if (secondary_job->type == V3DV_JOB_TYPE_GPU_CL_SECONDARY) {
      /* If the job is a CL, then we branch to it from the primary BCL.
   * In this case the secondary's BCL is finished with a
   * RETURN_FROM_SUB_LIST command to return back to the primary BCL
   * once we are done executing it.
   */
                  /* Sanity check that secondary BCL ends with RETURN_FROM_SUB_LIST */
   STATIC_ASSERT(cl_packet_length(RETURN_FROM_SUB_LIST) == 1);
   assert(v3dv_cl_offset(&secondary_job->bcl) >= 1);
                  /* If this secondary has any barriers (or we had any pending barrier
   * to apply), then we can't just branch to it from the primary, we
   * need to split the primary to create a new job that can consume
   * the barriers first.
   *
   * FIXME: in this case, maybe just copy the secondary BCL without
   * the RETURN_FROM_SUB_LIST into the primary job to skip the
   * branch?
   */
   primary_job = primary->state.job;
   if (!primary_job || secondary_job->serialize ||
      pending_barrier.dst_mask) {
   const bool needs_bcl_barrier =
                        primary_job =
                        /* Since we have created a new primary we need to re-emit
   * occlusion query state.
   */
                     /* Make sure our primary job has all required BO references */
   set_foreach(secondary_job->bos, entry) {
                        /* Emit required branch instructions. We expect each of these
   * to end with a corresponding 'return from sub list' item.
   */
   list_for_each_entry(struct v3dv_bo, bcl_bo,
            v3dv_cl_ensure_space_with_branch(&primary_job->bcl,
         v3dv_return_if_oom(primary, NULL);
   cl_emit(&primary_job->bcl, BRANCH_TO_SUB_LIST, branch) {
                     if (!secondary_job->can_use_double_buffer) {
         } else {
      primary_job->double_buffer_score.geom +=
         primary_job->double_buffer_score.render +=
      }
      } else {
      /* This is a regular job (CPU or GPU), so just finish the current
   * primary job (if any) and then add the secondary job to the
   * primary's job list right after it.
   */
   v3dv_cmd_buffer_finish_job(primary);
   v3dv_job_clone_in_cmd_buffer(secondary_job, primary);
   if (pending_barrier.dst_mask) {
      /* FIXME: do the same we do for primaries and only choose the
   * relevant src masks.
   */
   secondary_job->serialize = pending_barrier.src_mask_graphics |
               if (pending_barrier.bcl_buffer_access ||
      pending_barrier.bcl_image_access) {
                                 /* If the secondary has recorded any vkCmdEndQuery commands, we need to
   * copy this state to the primary so it is processed properly when the
   * current primary job is finished.
   */
            /* If this secondary had any pending barrier state we will need that
   * barrier state consumed with whatever comes next in the primary.
   */
   assert(secondary->state.barrier.dst_mask ||
                              if (pending_barrier.dst_mask) {
      v3dv_cmd_buffer_merge_barrier_state(&primary->state.barrier,
         }
      static void
   emit_gs_shader_state_record(struct v3dv_job *job,
                                 {
      cl_emit(&job->indirect, GEOMETRY_SHADER_STATE_RECORD, shader) {
      shader.geometry_bin_mode_shader_code_address =
         shader.geometry_bin_mode_shader_4_way_threadable =
         shader.geometry_bin_mode_shader_start_in_final_thread_section =
   #if V3D_VERSION <= 42
         #endif
         shader.geometry_bin_mode_shader_uniforms_address =
            shader.geometry_render_mode_shader_code_address =
         shader.geometry_render_mode_shader_4_way_threadable =
         shader.geometry_render_mode_shader_start_in_final_thread_section =
   #if V3D_VERSION <= 42
         #endif
         shader.geometry_render_mode_shader_uniforms_address =
         }
      static uint8_t
   v3d_gs_output_primitive(enum mesa_prim prim_type)
   {
      switch (prim_type) {
   case MESA_PRIM_POINTS:
         case MESA_PRIM_LINE_STRIP:
         case MESA_PRIM_TRIANGLE_STRIP:
         default:
            }
      static void
   emit_tes_gs_common_params(struct v3dv_job *job,
               {
      cl_emit(&job->indirect, TESSELLATION_GEOMETRY_COMMON_PARAMS, shader) {
      shader.tessellation_type = TESSELLATION_TYPE_TRIANGLE;
   shader.tessellation_point_mode = false;
   shader.tessellation_edge_spacing = TESSELLATION_EDGE_SPACING_EVEN;
   shader.tessellation_clockwise = true;
            shader.geometry_shader_output_format =
               }
      static uint8_t
   simd_width_to_gs_pack_mode(uint32_t width)
   {
      switch (width) {
   case 16:
         case 8:
         case 4:
         case 1:
         default:
            }
      static void
   emit_tes_gs_shader_params(struct v3dv_job *job,
                     {
      cl_emit(&job->indirect, TESSELLATION_GEOMETRY_SHADER_PARAMS, shader) {
      shader.tcs_batch_flush_mode = V3D_TCS_FLUSH_MODE_FULLY_PACKED;
   shader.per_patch_data_column_depth = 1;
   shader.tcs_output_segment_size_in_sectors = 1;
   shader.tcs_output_segment_pack_mode = V3D_PACK_MODE_16_WAY;
   shader.tes_output_segment_size_in_sectors = 1;
   shader.tes_output_segment_pack_mode = V3D_PACK_MODE_16_WAY;
   shader.gs_output_segment_size_in_sectors = gs_vpm_output_size;
   shader.gs_output_segment_pack_mode =
         shader.tbg_max_patches_per_tcs_batch = 1;
   shader.tbg_max_extra_vertex_segs_for_patches_after_first = 0;
   shader.tbg_min_tcs_output_segments_required_in_play = 1;
   shader.tbg_min_per_patch_data_segments_required_in_play = 1;
   shader.tpg_max_patches_per_tes_batch = 1;
   shader.tpg_max_vertex_segments_per_tes_batch = 0;
   shader.tpg_max_tcs_output_segments_per_tes_batch = 1;
   shader.tpg_min_tes_output_segments_required_in_play = 1;
   shader.gbg_max_tes_output_vertex_segments_per_gs_batch =
               }
      void
   v3dX(cmd_buffer_emit_gl_shader_state)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
   struct v3dv_pipeline *pipeline = state->gfx.pipeline;
            struct v3dv_shader_variant *vs_variant =
                  struct v3dv_shader_variant *vs_bin_variant =
                  struct v3dv_shader_variant *fs_variant =
                  struct v3dv_shader_variant *gs_variant = NULL;
   struct v3dv_shader_variant *gs_bin_variant = NULL;
   struct v3d_gs_prog_data *prog_data_gs = NULL;
   struct v3d_gs_prog_data *prog_data_gs_bin = NULL;
   if (pipeline->has_gs) {
      gs_variant =
                  gs_bin_variant =
                     /* Update the cache dirty flag based on the shader progs data */
   job->tmu_dirty_rcl |= prog_data_vs_bin->base.tmu_dirty_rcl;
   job->tmu_dirty_rcl |= prog_data_vs->base.tmu_dirty_rcl;
   job->tmu_dirty_rcl |= prog_data_fs->base.tmu_dirty_rcl;
   if (pipeline->has_gs) {
      job->tmu_dirty_rcl |= prog_data_gs_bin->base.tmu_dirty_rcl;
               /* See GFXH-930 workaround below */
            uint32_t shader_state_record_length =
         if (pipeline->has_gs) {
      shader_state_record_length +=
      cl_packet_length(GEOMETRY_SHADER_STATE_RECORD) +
   cl_packet_length(TESSELLATION_GEOMETRY_COMMON_PARAMS) +
            uint32_t shader_rec_offset =
      v3dv_cl_ensure_space(&job->indirect,
                                          if (pipeline->has_gs) {
      emit_gs_shader_state_record(job,
                                    emit_tes_gs_common_params(job,
                  emit_tes_gs_shader_params(job,
                        emit_tes_gs_shader_params(job,
                        #if V3D_VERSION == 42
      struct v3dv_bo *default_attribute_values =
      pipeline->default_attribute_values != NULL ?
   pipeline->default_attribute_values :
   #endif
         cl_emit_with_prepacked(&job->indirect, GL_SHADER_STATE_RECORD,
               /* FIXME: we are setting this values here and during the
   * prepacking. This is because both cl_emit_with_prepacked and v3dvx_pack
   * asserts for minimum values of these. It would be good to get
   * v3dvx_pack to assert on the final value if possible
   */
   shader.min_coord_shader_input_segments_required_in_play =
         shader.min_vertex_shader_input_segments_required_in_play =
            shader.coordinate_shader_code_address =
         shader.vertex_shader_code_address =
         shader.fragment_shader_code_address =
            shader.coordinate_shader_uniforms_address = cmd_buffer->state.uniforms.vs_bin;
   shader.vertex_shader_uniforms_address = cmd_buffer->state.uniforms.vs;
      #if V3D_VERSION == 42
         shader.address_of_default_attribute_values =
   #endif
            shader.any_shader_reads_hardware_written_primitive_id =
         shader.insert_primitive_id_as_first_varying_to_fragment_shader =
               /* Upload vertex element attributes (SHADER_STATE_ATTRIBUTE_RECORD) */
   bool cs_loaded_any = false;
   const bool cs_uses_builtins = prog_data_vs_bin->uses_iid ||
               const uint32_t packet_length =
            uint32_t emitted_va_count = 0;
   for (uint32_t i = 0; emitted_va_count < pipeline->va_count; i++) {
               if (pipeline->va[i].vk_format == VK_FORMAT_UNDEFINED)
                     /* We store each vertex attribute in the array using its driver location
   * as index.
   */
                     cl_emit_with_prepacked(&job->indirect, GL_SHADER_STATE_ATTRIBUTE_RECORD,
               assert(c_vb->buffer->mem->bo);
   attr.address = v3dv_cl_address(c_vb->buffer->mem->bo,
                        attr.number_of_values_read_by_coordinate_shader =
                        /* GFXH-930: At least one attribute must be enabled and read by CS
   * and VS.  If we have attributes being consumed by the VS but not
   * the CS, then set up a dummy load of the last attribute into the
   * CS's VPM inputs.  (Since CS is just dead-code-elimination compared
   * to VS, we can't have CS loading but not VS).
   *
   * GFXH-1602: first attribute must be active if using builtins.
   */
                  if (i == 0 && cs_uses_builtins && !cs_loaded_any) {
      attr.number_of_values_read_by_coordinate_shader = 1;
      } else if (i == pipeline->va_count - 1 && !cs_loaded_any) {
      attr.number_of_values_read_by_coordinate_shader = 1;
                                       if (pipeline->va_count == 0) {
      /* GFXH-930: At least one attribute must be enabled and read
   * by CS and VS.  If we have no attributes being consumed by
   * the shader, set up a dummy to be loaded into the VPM.
   */
   cl_emit(&job->indirect, GL_SHADER_STATE_ATTRIBUTE_RECORD, attr) {
                     attr.type = ATTRIBUTE_FLOAT;
                  attr.number_of_values_read_by_coordinate_shader = 1;
                  if (cmd_buffer->state.dirty & V3DV_CMD_DIRTY_PIPELINE) {
      v3dv_cl_ensure_space_with_branch(&job->bcl,
                              v3dv_cl_ensure_space_with_branch(&job->bcl,
                  if (pipeline->has_gs) {
      cl_emit(&job->bcl, GL_SHADER_STATE_INCLUDING_GS, state) {
      state.address = v3dv_cl_address(job->indirect.bo, shader_rec_offset);
         } else {
      cl_emit(&job->bcl, GL_SHADER_STATE, state) {
      state.address = v3dv_cl_address(job->indirect.bo, shader_rec_offset);
                  /* Clearing push constants and descriptor sets for all stages is not quite
   * correct (some shader stages may not be used at all or they may not be
   * consuming push constants), however this is not relevant because if we
   * bind a different pipeline we always have to rebuild the uniform streams.
   */
   cmd_buffer->state.dirty &= ~(V3DV_CMD_DIRTY_VERTEX_BUFFER |
               cmd_buffer->state.dirty_descriptor_stages &= ~VK_SHADER_STAGE_ALL_GRAPHICS;
      }
      void
   v3dX(cmd_buffer_emit_draw)(struct v3dv_cmd_buffer *cmd_buffer,
         {
      struct v3dv_job *job = cmd_buffer->state.job;
            struct v3dv_cmd_buffer_state *state = &cmd_buffer->state;
                              if (info->first_instance > 0) {
      v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, BASE_VERTEX_BASE_INSTANCE, base) {
      base.base_instance = info->first_instance;
                  if (info->instance_count > 1) {
      v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, VERTEX_ARRAY_INSTANCED_PRIMS, prim) {
      prim.mode = hw_prim_type;
   prim.index_of_first_vertex = info->first_vertex;
   prim.number_of_instances = info->instance_count;
         } else {
      v3dv_cl_ensure_space_with_branch(
         v3dv_return_if_oom(cmd_buffer, NULL);
   cl_emit(&job->bcl, VERTEX_ARRAY_PRIMS, prim) {
      prim.mode = hw_prim_type;
   prim.length = info->vertex_count;
            }
      void
   v3dX(cmd_buffer_emit_index_buffer)(struct v3dv_cmd_buffer *cmd_buffer)
   {
      struct v3dv_job *job = cmd_buffer->state.job;
            /* We flag all state as dirty when we create a new job so make sure we
   * have a valid index buffer before attempting to emit state for it.
   */
   struct v3dv_buffer *ibuffer =
         if (ibuffer) {
      v3dv_cl_ensure_space_with_branch(
                  const uint32_t offset = cmd_buffer->state.index_buffer.offset;
   cl_emit(&job->bcl, INDEX_BUFFER_SETUP, ib) {
      ib.address = v3dv_cl_address(ibuffer->mem->bo,
                           }
      void
   v3dX(cmd_buffer_emit_draw_indexed)(struct v3dv_cmd_buffer *cmd_buffer,
                                 {
      struct v3dv_job *job = cmd_buffer->state.job;
            const struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   uint32_t hw_prim_type = v3d_hw_prim_type(pipeline->topology);
   uint8_t index_type = ffs(cmd_buffer->state.index_buffer.index_size) - 1;
            if (vertexOffset != 0 || firstInstance != 0) {
      v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, BASE_VERTEX_BASE_INSTANCE, base) {
      base.base_instance = firstInstance;
                  if (instanceCount == 1) {
      v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, INDEXED_PRIM_LIST, prim) {
      prim.index_type = index_type;
   prim.length = indexCount;
   prim.index_offset = index_offset;
   prim.mode = hw_prim_type;
         } else if (instanceCount > 1) {
      v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, INDEXED_INSTANCED_PRIM_LIST, prim) {
      prim.index_type = index_type;
   prim.index_offset = index_offset;
   prim.mode = hw_prim_type;
   prim.enable_primitive_restarts = pipeline->primitive_restart;
   prim.number_of_instances = instanceCount;
            }
      void
   v3dX(cmd_buffer_emit_draw_indirect)(struct v3dv_cmd_buffer *cmd_buffer,
                           {
      struct v3dv_job *job = cmd_buffer->state.job;
            const struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
            v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, INDIRECT_VERTEX_ARRAY_INSTANCED_PRIMS, prim) {
      prim.mode = hw_prim_type;
   prim.number_of_draw_indirect_array_records = drawCount;
   prim.stride_in_multiples_of_4_bytes = stride >> 2;
   prim.address = v3dv_cl_address(buffer->mem->bo,
         }
      void
   v3dX(cmd_buffer_emit_indexed_indirect)(struct v3dv_cmd_buffer *cmd_buffer,
                           {
      struct v3dv_job *job = cmd_buffer->state.job;
            const struct v3dv_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   uint32_t hw_prim_type = v3d_hw_prim_type(pipeline->topology);
            v3dv_cl_ensure_space_with_branch(
                  cl_emit(&job->bcl, INDIRECT_INDEXED_INSTANCED_PRIM_LIST, prim) {
      prim.index_type = index_type;
   prim.mode = hw_prim_type;
   prim.enable_primitive_restarts = pipeline->primitive_restart;
   prim.number_of_draw_indirect_indexed_records = drawCount;
   prim.stride_in_multiples_of_4_bytes = stride >> 2;
   prim.address = v3dv_cl_address(buffer->mem->bo,
         }
