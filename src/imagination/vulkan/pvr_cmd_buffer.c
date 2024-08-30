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
   #include <limits.h>
   #include <stdbool.h>
   #include <stddef.h>
   #include <stdint.h>
   #include <string.h>
   #include <vulkan/vulkan.h>
      #include "hwdef/rogue_hw_defs.h"
   #include "hwdef/rogue_hw_utils.h"
   #include "pvr_blit.h"
   #include "pvr_bo.h"
   #include "pvr_clear.h"
   #include "pvr_common.h"
   #include "pvr_csb.h"
   #include "pvr_csb_enum_helpers.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_hardcode.h"
   #include "pvr_hw_pass.h"
   #include "pvr_job_common.h"
   #include "pvr_job_render.h"
   #include "pvr_limits.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_tex_state.h"
   #include "pvr_types.h"
   #include "pvr_uscgen.h"
   #include "pvr_winsys.h"
   #include "util/bitscan.h"
   #include "util/bitset.h"
   #include "util/compiler.h"
   #include "util/list.h"
   #include "util/macros.h"
   #include "util/u_dynarray.h"
   #include "util/u_math.h"
   #include "util/u_pack_color.h"
   #include "vk_alloc.h"
   #include "vk_command_buffer.h"
   #include "vk_command_pool.h"
   #include "vk_common_entrypoints.h"
   #include "vk_format.h"
   #include "vk_graphics_state.h"
   #include "vk_log.h"
   #include "vk_object.h"
   #include "vk_util.h"
      /* Structure used to pass data into pvr_compute_generate_control_stream()
   * function.
   */
   struct pvr_compute_kernel_info {
      pvr_dev_addr_t indirect_buffer_addr;
   bool global_offsets_present;
   uint32_t usc_common_size;
   uint32_t usc_unified_size;
   uint32_t pds_temp_size;
   uint32_t pds_data_size;
   enum PVRX(CDMCTRL_USC_TARGET) usc_target;
   bool is_fence;
   uint32_t pds_data_offset;
   uint32_t pds_code_offset;
   enum PVRX(CDMCTRL_SD_TYPE) sd_type;
   bool usc_common_shared;
   uint32_t local_size[PVR_WORKGROUP_DIMENSIONS];
   uint32_t global_size[PVR_WORKGROUP_DIMENSIONS];
      };
      static void pvr_cmd_buffer_free_sub_cmd(struct pvr_cmd_buffer *cmd_buffer,
         {
      if (sub_cmd->owned) {
      switch (sub_cmd->type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS:
      util_dynarray_fini(&sub_cmd->gfx.sec_query_indices);
   pvr_csb_finish(&sub_cmd->gfx.control_stream);
   pvr_bo_free(cmd_buffer->device, sub_cmd->gfx.terminate_ctrl_stream);
   pvr_bo_suballoc_free(sub_cmd->gfx.depth_bias_bo);
               case PVR_SUB_CMD_TYPE_COMPUTE:
   case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
                  case PVR_SUB_CMD_TYPE_TRANSFER:
      list_for_each_entry_safe (struct pvr_transfer_cmd,
                        list_del(&transfer_cmd->link);
   if (!transfer_cmd->is_deferred_clear)
                  case PVR_SUB_CMD_TYPE_EVENT:
      if (sub_cmd->event.type == PVR_EVENT_TYPE_WAIT)
               default:
                     list_del(&sub_cmd->link);
      }
      static void pvr_cmd_buffer_free_sub_cmds(struct pvr_cmd_buffer *cmd_buffer)
   {
      list_for_each_entry_safe (struct pvr_sub_cmd,
                              }
      static void pvr_cmd_buffer_free_resources(struct pvr_cmd_buffer *cmd_buffer)
   {
      vk_free(&cmd_buffer->vk.pool->alloc,
         vk_free(&cmd_buffer->vk.pool->alloc,
                              list_for_each_entry_safe (struct pvr_suballoc_bo,
                        list_del(&suballoc_bo->link);
               util_dynarray_fini(&cmd_buffer->deferred_clears);
   util_dynarray_fini(&cmd_buffer->deferred_csb_commands);
   util_dynarray_fini(&cmd_buffer->scissor_array);
      }
      static void pvr_cmd_buffer_reset(struct vk_command_buffer *vk_cmd_buffer,
         {
      struct pvr_cmd_buffer *cmd_buffer =
            /* FIXME: For now we always free all resources as if
   * VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT was set.
   */
                     memset(&cmd_buffer->state, 0, sizeof(cmd_buffer->state));
               }
      static void pvr_cmd_buffer_destroy(struct vk_command_buffer *vk_cmd_buffer)
   {
      struct pvr_cmd_buffer *cmd_buffer =
            pvr_cmd_buffer_free_resources(cmd_buffer);
   vk_command_buffer_finish(&cmd_buffer->vk);
      }
      static const struct vk_command_buffer_ops cmd_buffer_ops = {
      .reset = pvr_cmd_buffer_reset,
      };
      static VkResult pvr_cmd_buffer_create(struct pvr_device *device,
                     {
      struct pvr_cmd_buffer *cmd_buffer;
            cmd_buffer = vk_zalloc(&pool->alloc,
                     if (!cmd_buffer)
            result =
         if (result != VK_SUCCESS) {
      vk_free(&pool->alloc, cmd_buffer);
                        util_dynarray_init(&cmd_buffer->depth_bias_array, NULL);
   util_dynarray_init(&cmd_buffer->scissor_array, NULL);
   util_dynarray_init(&cmd_buffer->deferred_csb_commands, NULL);
            list_inithead(&cmd_buffer->sub_cmds);
                        }
      VkResult
   pvr_AllocateCommandBuffers(VkDevice _device,
               {
      VK_FROM_HANDLE(vk_command_pool, pool, pAllocateInfo->commandPool);
   PVR_FROM_HANDLE(pvr_device, device, _device);
   VkResult result = VK_SUCCESS;
            for (i = 0; i < pAllocateInfo->commandBufferCount; i++) {
      result = pvr_cmd_buffer_create(device,
                     if (result != VK_SUCCESS)
               if (result != VK_SUCCESS) {
      while (i--) {
      VK_FROM_HANDLE(vk_command_buffer, cmd_buffer, pCommandBuffers[i]);
               for (i = 0; i < pAllocateInfo->commandBufferCount; i++)
                  }
      static void pvr_cmd_buffer_update_barriers(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
            switch (type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS:
      barriers = PVR_PIPELINE_STAGE_GEOM_BIT | PVR_PIPELINE_STAGE_FRAG_BIT;
         case PVR_SUB_CMD_TYPE_COMPUTE:
      barriers = PVR_PIPELINE_STAGE_COMPUTE_BIT;
         case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
   case PVR_SUB_CMD_TYPE_TRANSFER:
      /* Compute jobs are used for occlusion queries but to copy the results we
   * have to sync with transfer jobs because vkCmdCopyQueryPoolResults() is
   * deemed as a transfer operation by the spec.
   */
   barriers = PVR_PIPELINE_STAGE_TRANSFER_BIT;
         case PVR_SUB_CMD_TYPE_EVENT:
      barriers = 0;
         default:
                  for (uint32_t i = 0; i < ARRAY_SIZE(state->barriers_needed); i++)
      }
      static VkResult
   pvr_cmd_buffer_upload_tables(struct pvr_device *device,
               {
      const uint32_t cache_line_size =
                           if (cmd_buffer->depth_bias_array.size > 0) {
      result =
      pvr_gpu_upload(device,
                  device->heaps.general_heap,
   util_dynarray_begin(&cmd_buffer->depth_bias_array),
   if (result != VK_SUCCESS)
               if (cmd_buffer->scissor_array.size > 0) {
      result = pvr_gpu_upload(device,
                           device->heaps.general_heap,
   if (result != VK_SUCCESS)
               util_dynarray_clear(&cmd_buffer->depth_bias_array);
                  err_free_depth_bias_bo:
      pvr_bo_suballoc_free(sub_cmd->depth_bias_bo);
               }
      static VkResult
   pvr_cmd_buffer_emit_ppp_state(const struct pvr_cmd_buffer *const cmd_buffer,
         {
      const struct pvr_framebuffer *const framebuffer =
            assert(csb->stream_type == PVR_CMD_STREAM_TYPE_GRAPHICS ||
                     pvr_csb_emit (csb, VDMCTRL_PPP_STATE0, state0) {
      state0.addrmsb = framebuffer->ppp_state_bo->dev_addr;
               pvr_csb_emit (csb, VDMCTRL_PPP_STATE1, state1) {
                              }
      VkResult
   pvr_cmd_buffer_upload_general(struct pvr_cmd_buffer *const cmd_buffer,
                     {
      struct pvr_device *const device = cmd_buffer->device;
   const uint32_t cache_line_size =
         struct pvr_suballoc_bo *suballoc_bo;
            result = pvr_gpu_upload(device,
                           device->heaps.general_heap,
   if (result != VK_SUCCESS)
                                 }
      static VkResult
   pvr_cmd_buffer_upload_usc(struct pvr_cmd_buffer *const cmd_buffer,
                           {
      struct pvr_device *const device = cmd_buffer->device;
   const uint32_t cache_line_size =
         struct pvr_suballoc_bo *suballoc_bo;
                     result =
         if (result != VK_SUCCESS)
                                 }
      VkResult pvr_cmd_buffer_upload_pds(struct pvr_cmd_buffer *const cmd_buffer,
                                    const uint32_t *data,
   uint32_t data_size_dwords,
      {
      struct pvr_device *const device = cmd_buffer->device;
            result = pvr_gpu_upload_pds(device,
                              data,
   data_size_dwords,
   data_alignment,
      if (result != VK_SUCCESS)
                        }
      static inline VkResult
   pvr_cmd_buffer_upload_pds_data(struct pvr_cmd_buffer *const cmd_buffer,
                           {
      return pvr_cmd_buffer_upload_pds(cmd_buffer,
                                    data,
   data_size_dwords,
   }
      /* pbe_cs_words must be an array of length emit_count with
   * ROGUE_NUM_PBESTATE_STATE_WORDS entries
   */
   static VkResult pvr_sub_cmd_gfx_per_job_fragment_programs_create_and_upload(
      struct pvr_cmd_buffer *const cmd_buffer,
   const uint32_t emit_count,
   const uint32_t *pbe_cs_words,
      {
      struct pvr_pds_event_program pixel_event_program = {
      /* No data to DMA, just a DOUTU needed. */
      };
   const uint32_t staging_buffer_size =
         const VkAllocationCallbacks *const allocator = &cmd_buffer->vk.pool->alloc;
   struct pvr_device *const device = cmd_buffer->device;
   struct pvr_suballoc_bo *usc_eot_program = NULL;
   struct util_dynarray eot_program_bin;
   uint32_t *staging_buffer;
   uint32_t usc_temp_count;
                     pvr_uscgen_eot("per-job EOT",
                  emit_count,
         result = pvr_cmd_buffer_upload_usc(cmd_buffer,
                                       if (result != VK_SUCCESS)
            pvr_pds_setup_doutu(&pixel_event_program.task_control,
                              /* TODO: We could skip allocating this and generate directly into the device
   * buffer thus removing one allocation and memcpy() per job. Would this
   * speed up things in a noticeable way?
   */
   staging_buffer = vk_alloc(allocator,
                     if (!staging_buffer) {
      result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               /* Generate the data segment. The code segment was uploaded earlier when
   * setting up the PDS static heap data.
   */
   pvr_pds_generate_pixel_event_data_segment(&pixel_event_program,
                  result = pvr_cmd_buffer_upload_pds_data(
      cmd_buffer,
   staging_buffer,
   cmd_buffer->device->pixel_event_data_size_in_dwords,
   4,
      if (result != VK_SUCCESS)
                           err_free_pixel_event_staging_buffer:
            err_free_usc_pixel_program:
      list_del(&usc_eot_program->link);
               }
      static VkResult pvr_sub_cmd_gfx_build_terminate_ctrl_stream(
      struct pvr_device *const device,
   const struct pvr_cmd_buffer *const cmd_buffer,
      {
      struct list_head bo_list;
   struct pvr_csb csb;
                     result = pvr_cmd_buffer_emit_ppp_state(cmd_buffer, &csb);
   if (result != VK_SUCCESS)
            result = pvr_csb_emit_terminate(&csb);
   if (result != VK_SUCCESS)
            result = pvr_csb_bake(&csb, &bo_list);
   if (result != VK_SUCCESS)
            /* This is a trivial control stream, there's no reason it should ever require
   * more memory than a single bo can provide.
   */
   assert(list_is_singular(&bo_list));
   gfx_sub_cmd->terminate_ctrl_stream =
                  err_csb_finish:
                  }
      static VkResult pvr_setup_texture_state_words(
      struct pvr_device *device,
   struct pvr_combined_image_sampler_descriptor *descriptor,
      {
      const struct pvr_image *image = vk_to_pvr_image(image_view->vk.image);
   struct pvr_texture_state_info info = {
      .format = image_view->vk.format,
   .mem_layout = image->memlayout,
   .type = image_view->vk.view_type,
   .is_cube = image_view->vk.view_type == VK_IMAGE_VIEW_TYPE_CUBE ||
         .tex_state_type = PVR_TEXTURE_STATE_SAMPLE,
   .extent = image_view->vk.extent,
   .mip_levels = 1,
   .sample_count = image_view->vk.image->samples,
   .stride = image->physical_extent.width,
      };
   const uint8_t *const swizzle = pvr_get_format_swizzle(info.format);
                     /* TODO: Can we use image_view->texture_state instead of generating here? */
   result = pvr_pack_tex_state(device, &info, descriptor->image);
   if (result != VK_SUCCESS)
                     pvr_csb_pack (&descriptor->sampler.data.sampler_word,
                  sampler.non_normalized_coords = true;
   sampler.addrmode_v = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.addrmode_u = PVRX(TEXSTATE_ADDRMODE_CLAMP_TO_EDGE);
   sampler.minfilter = PVRX(TEXSTATE_FILTER_POINT);
   sampler.magfilter = PVRX(TEXSTATE_FILTER_POINT);
                  }
      static VkResult
   pvr_load_op_constants_create_and_upload(struct pvr_cmd_buffer *cmd_buffer,
               {
      const struct pvr_render_pass_info *render_pass_info =
         const struct pvr_render_pass *pass = render_pass_info->pass;
   const struct pvr_renderpass_hwsetup_render *hw_render = load_op->hw_render;
   const struct pvr_renderpass_colorinit *color_init =
         const VkClearValue *clear_value =
         struct pvr_suballoc_bo *clear_bo;
   uint32_t attachment_count;
   bool has_depth_clear;
   bool has_depth_load;
            /* These are only setup and never used for now. These will need to be
   * uploaded into a buffer based on some compiler info.
   */
   /* TODO: Remove the above comment once the compiler is hooked up and we're
   * setting up + uploading the buffer.
   */
   struct pvr_combined_image_sampler_descriptor
         uint32_t texture_count = 0;
   uint32_t hw_clear_value[PVR_LOAD_OP_CLEARS_LOADS_MAX_RTS *
                  if (load_op->is_hw_object)
         else
            for (uint32_t i = 0; i < attachment_count; i++) {
      struct pvr_image_view *image_view;
            if (load_op->is_hw_object)
         else
                     assert((load_op->clears_loads_state.rt_load_mask &
         if (load_op->clears_loads_state.rt_load_mask & BITFIELD_BIT(i)) {
      result = pvr_setup_texture_state_words(cmd_buffer->device,
                                 } else if (load_op->clears_loads_state.rt_clear_mask & BITFIELD_BIT(i)) {
                     assert(next_clear_consts +
                  /* FIXME: do this at the point we store the clear values? */
   pvr_get_hw_clear_color(image_view->vk.format,
                                 has_depth_load = false;
   for (uint32_t i = 0;
      i < ARRAY_SIZE(load_op->clears_loads_state.dest_vk_format);
   i++) {
   if (load_op->clears_loads_state.dest_vk_format[i] ==
      VK_FORMAT_D32_SFLOAT) {
   has_depth_load = true;
                                    if (has_depth_load) {
      const struct pvr_render_pass_attachment *attachment;
            assert(load_op->subpass->depth_stencil_attachment !=
         assert(!load_op->is_hw_object);
   attachment =
                     result = pvr_setup_texture_state_words(cmd_buffer->device,
               if (result != VK_SUCCESS)
               } else if (has_depth_clear) {
      const struct pvr_render_pass_attachment *attachment;
            assert(load_op->subpass->depth_stencil_attachment !=
         attachment =
                     assert(next_clear_consts < ARRAY_SIZE(hw_clear_value));
               result = pvr_cmd_buffer_upload_general(cmd_buffer,
                     if (result != VK_SUCCESS)
                        }
      static VkResult pvr_load_op_pds_data_create_and_upload(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct pvr_load_op *load_op,
   pvr_dev_addr_t constants_addr,
      {
      struct pvr_device *device = cmd_buffer->device;
   const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   struct pvr_pds_pixel_shader_sa_program program = { 0 };
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
                     pvr_csb_pack (&program.texture_dma_address[0],
                              pvr_csb_pack (&program.texture_dma_control[0],
                  value.dest = PVRX(PDSINST_DOUTD_DEST_COMMON_STORE);
   value.a0 = load_op->shareds_dest_offset;
                                 staging_buffer = vk_alloc(&cmd_buffer->vk.pool->alloc,
                     if (!staging_buffer)
            pvr_pds_generate_pixel_shader_sa_texture_state_data(&program,
                  result = pvr_cmd_buffer_upload_pds_data(cmd_buffer,
                           if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, staging_buffer);
                           }
      /* FIXME: Should this function be specific to the HW background object, in
   * which case its name should be changed, or should it have the load op
   * structure passed in?
   */
   static VkResult
   pvr_load_op_data_create_and_upload(struct pvr_cmd_buffer *cmd_buffer,
               {
      pvr_dev_addr_t constants_addr;
            result = pvr_load_op_constants_create_and_upload(cmd_buffer,
               if (result != VK_SUCCESS)
            return pvr_load_op_pds_data_create_and_upload(cmd_buffer,
                  }
      static void pvr_pds_bgnd_pack_state(
      const struct pvr_load_op *load_op,
   const struct pvr_pds_upload *load_op_program,
      {
      pvr_csb_pack (&pds_reg_values[0], CR_PDS_BGRND0_BASE, value) {
      value.shader_addr = PVR_DEV_ADDR(load_op->pds_frag_prog.data_offset);
   value.texunicode_addr =
               pvr_csb_pack (&pds_reg_values[1], CR_PDS_BGRND1_BASE, value) {
                  pvr_csb_pack (&pds_reg_values[2], CR_PDS_BGRND3_SIZEINFO, value) {
      value.usc_sharedsize =
      DIV_ROUND_UP(load_op->const_shareds_count,
      value.pds_texturestatesize = DIV_ROUND_UP(
      load_op_program->data_size,
      value.pds_tempsize =
      DIV_ROUND_UP(load_op->temps_count,
      }
      /**
   * \brief Calculates the stride in pixels based on the pitch in bytes and pixel
   * format.
   *
   * \param[in] pitch     Width pitch in bytes.
   * \param[in] vk_format Vulkan image format.
   * \return Stride in pixels.
   */
   static inline uint32_t pvr_stride_from_pitch(uint32_t pitch, VkFormat vk_format)
   {
                           }
      static void pvr_setup_pbe_state(
      const struct pvr_device_info *dev_info,
   const struct pvr_framebuffer *framebuffer,
   uint32_t mrt_index,
   const struct usc_mrt_resource *mrt_resource,
   const struct pvr_image_view *const iview,
   const VkRect2D *render_area,
   const bool down_scale,
   const uint32_t samples,
   uint32_t pbe_cs_words[static const ROGUE_NUM_PBESTATE_STATE_WORDS],
      {
      const struct pvr_image *image = pvr_image_view_get_image(iview);
            struct pvr_pbe_surf_params surface_params;
   struct pvr_pbe_render_params render_params;
   bool with_packed_usc_channel;
   const uint8_t *swizzle;
            /* down_scale should be true when performing a resolve, in which case there
   * should be more than one sample.
   */
                     if (PVR_HAS_FEATURE(dev_info, usc_f16sop_u8)) {
      with_packed_usc_channel = vk_format_is_unorm(iview->vk.format) ||
      } else {
                  swizzle = pvr_get_format_swizzle(iview->vk.format);
            pvr_pbe_get_src_format_and_gamma(iview->vk.format,
                              surface_params.is_normalized = vk_format_is_normalized(iview->vk.format);
   surface_params.pbe_packmode = pvr_get_pbe_packmode(iview->vk.format);
            /* FIXME: Should we have an inline function to return the address of a mip
   * level?
   */
   surface_params.addr =
      PVR_DEV_ADDR_OFFSET(image->vma->dev_addr,
      surface_params.addr =
      PVR_DEV_ADDR_OFFSET(surface_params.addr,
         surface_params.mem_layout = image->memlayout;
   surface_params.stride = pvr_stride_from_pitch(level_pitch, iview->vk.format);
   surface_params.depth = iview->vk.extent.depth;
   surface_params.width = iview->vk.extent.width;
   surface_params.height = iview->vk.extent.height;
   surface_params.z_only_render = false;
                     if (mrt_resource->type == USC_MRT_RESOURCE_TYPE_MEMORY) {
         } else {
      assert(mrt_resource->type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG);
                                 switch (position) {
   case 0:
   case 4:
      render_params.source_start = PVR_PBE_STARTPOS_BIT0;
      case 1:
   case 5:
      render_params.source_start = PVR_PBE_STARTPOS_BIT32;
      case 2:
   case 6:
      render_params.source_start = PVR_PBE_STARTPOS_BIT64;
      case 3:
   case 7:
      render_params.source_start = PVR_PBE_STARTPOS_BIT96;
      default:
      assert(!"Invalid output register");
            #define PVR_DEC_IF_NOT_ZERO(_v) (((_v) > 0) ? (_v)-1 : 0)
         render_params.min_x_clip = MAX2(0, render_area->offset.x);
   render_params.min_y_clip = MAX2(0, render_area->offset.y);
   render_params.max_x_clip = MIN2(
      framebuffer->width - 1,
      render_params.max_y_clip = MIN2(
      framebuffer->height - 1,
      #undef PVR_DEC_IF_NOT_ZERO
         render_params.slice = 0;
            pvr_pbe_pack_state(dev_info,
                        }
      static struct pvr_render_target *
   pvr_get_render_target(const struct pvr_render_pass *pass,
               {
      const struct pvr_renderpass_hwsetup_render *hw_render =
                  switch (hw_render->sample_count) {
   case 1:
   case 2:
   case 4:
   case 8:
      rt_idx = util_logbase2(hw_render->sample_count);
         default:
      unreachable("Unsupported sample count");
                  }
      static uint32_t
   pvr_pass_get_pixel_output_width(const struct pvr_render_pass *pass,
               {
      const struct pvr_renderpass_hwsetup_render *hw_render =
         /* Default value based on the maximum value found in all existing cores. The
   * maximum is used as this is being treated as a lower bound, making it a
   * "safer" choice than the minimum value found in all existing cores.
   */
   const uint32_t min_output_regs =
                     }
      static inline bool
   pvr_ds_attachment_requires_zls(const struct pvr_ds_attachment *attachment)
   {
               zls_used = attachment->load.d || attachment->load.s;
               }
      /**
   * \brief If depth and/or stencil attachment dimensions are not tile-aligned,
   * then we may need to insert some additional transfer subcommands.
   *
   * It's worth noting that we check whether the dimensions are smaller than a
   * tile here, rather than checking whether they're tile-aligned - this relies
   * on the assumption that we can safely use any attachment with dimensions
   * larger than a tile. If the attachment is twiddled, it will be over-allocated
   * to the nearest power-of-two (which will be tile-aligned). If the attachment
   * is not twiddled, we don't need to worry about tile-alignment at all.
   */
   static bool pvr_sub_cmd_gfx_requires_ds_subtile_alignment(
      const struct pvr_device_info *dev_info,
      {
      const struct pvr_image *const ds_image =
         uint32_t zls_tile_size_x;
                     if (ds_image->physical_extent.width >= zls_tile_size_x &&
      ds_image->physical_extent.height >= zls_tile_size_y) {
               /* If we have the zls_subtile feature, we can skip the alignment iff:
   *  - The attachment is not multisampled, and
   *  - The depth and stencil attachments are the same.
   */
   if (PVR_HAS_FEATURE(dev_info, zls_subtile) &&
      ds_image->vk.samples == VK_SAMPLE_COUNT_1_BIT &&
   job->has_stencil_attachment == job->has_depth_attachment) {
               /* No ZLS functions enabled; nothing to do. */
   if ((!job->has_depth_attachment && !job->has_stencil_attachment) ||
      !pvr_ds_attachment_requires_zls(&job->ds)) {
                  }
      static VkResult
   pvr_sub_cmd_gfx_align_ds_subtiles(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct pvr_sub_cmd *const prev_sub_cmd =
         struct pvr_ds_attachment *const ds = &gfx_sub_cmd->job.ds;
   const struct pvr_image *const ds_image = pvr_image_view_get_image(ds->iview);
            struct pvr_suballoc_bo *buffer;
   uint32_t buffer_layer_size;
   VkBufferImageCopy2 region;
   VkExtent2D zls_tile_size;
   VkExtent2D rounded_size;
   uint32_t buffer_size;
   VkExtent2D scale;
            /* The operations below assume the last command in the buffer was the target
   * gfx subcommand. Assert that this is the case.
   */
   assert(list_last_entry(&cmd_buffer->sub_cmds, struct pvr_sub_cmd, link) ==
            if (!pvr_ds_attachment_requires_zls(ds))
            rogue_get_zls_tile_size_xy(&cmd_buffer->device->pdevice->dev_info,
               rogue_get_isp_scale_xy_from_samples(ds_image->vk.samples,
                  rounded_size = (VkExtent2D){
      .width = ALIGN_POT(ds_image->physical_extent.width, zls_tile_size.width),
   .height =
               buffer_layer_size = vk_format_get_blocksize(ds_image->vk.format) *
                  if (ds->iview->vk.layer_count > 1)
                     result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            region = (VkBufferImageCopy2){
      .sType = VK_STRUCTURE_TYPE_BUFFER_IMAGE_COPY_2,
   .pNext = NULL,
   .bufferOffset = 0,
   .bufferRowLength = rounded_size.width,
   .bufferImageHeight = 0,
   .imageSubresource = {
      .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
   .mipLevel = ds->iview->vk.base_mip_level,
   .baseArrayLayer = ds->iview->vk.base_array_layer,
      },
   .imageOffset = { 0 },
   .imageExtent = {
      .width = ds->iview->vk.extent.width,
   .height = ds->iview->vk.extent.height,
                  if (ds->load.d || ds->load.s) {
               result =
         if (result != VK_SUCCESS)
            result = pvr_copy_image_to_buffer_region_format(cmd_buffer,
                                 if (result != VK_SUCCESS)
                     result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Now we have to fiddle with cmd_buffer to place this transfer command
   * *before* the target gfx subcommand.
   */
   list_move_to(&cmd_buffer->state.current_sub_cmd->link,
                        if (ds->store.d || ds->store.s) {
               result =
         if (result != VK_SUCCESS)
            result = pvr_copy_buffer_to_image_region_format(cmd_buffer,
                                       if (result != VK_SUCCESS)
                     result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
                        /* Finally, patch up the target graphics sub_cmd to use the correctly-strided
   * buffer.
   */
   ds->has_alignment_transfers = true;
   ds->addr = buffer->dev_addr;
                        }
      struct pvr_emit_state {
      uint32_t pbe_cs_words[PVR_MAX_COLOR_ATTACHMENTS]
            uint64_t pbe_reg_words[PVR_MAX_COLOR_ATTACHMENTS]
               };
      static void
   pvr_setup_emit_state(const struct pvr_device_info *dev_info,
                     {
               if (hw_render->eot_surface_count == 0) {
      emit_state->emit_count = 1;
   pvr_csb_pack (&emit_state->pbe_cs_words[0][1],
                     }
               static_assert(USC_MRT_RESOURCE_TYPE_OUTPUT_REG + 1 ==
                  emit_state->emit_count = 0;
   for (uint32_t resource_type = USC_MRT_RESOURCE_TYPE_OUTPUT_REG;
      resource_type <= USC_MRT_RESOURCE_TYPE_MEMORY;
   resource_type++) {
   for (uint32_t i = 0; i < hw_render->eot_surface_count; i++) {
      const struct pvr_framebuffer *framebuffer =
         const struct pvr_renderpass_hwsetup_eot_surface *surface =
         const struct pvr_image_view *iview =
         const struct usc_mrt_resource *mrt_resource =
                                 if (surface->need_resolve) {
                     /* Attachments that are the destination of resolve operations must
   * be loaded before their next use.
   */
                                                            pvr_setup_pbe_state(dev_info,
                     framebuffer,
   emit_state->emit_count,
   mrt_resource,
   iview,
   &render_pass_info->render_area,
   surface->need_resolve,
                     }
      static inline bool
   pvr_is_render_area_tile_aligned(const struct pvr_cmd_buffer *cmd_buffer,
         {
      const VkRect2D *render_area =
            return render_area->offset.x == 0 && render_area->offset.y == 0 &&
            }
      static VkResult pvr_sub_cmd_gfx_job_init(const struct pvr_device_info *dev_info,
               {
      static const VkClearDepthStencilValue default_ds_clear_value = {
      .depth = 1.0f,
               const struct vk_dynamic_graphics_state *dynamic_state =
         struct pvr_render_pass_info *render_pass_info =
         const struct pvr_renderpass_hwsetup_render *hw_render =
         struct pvr_render_job *job = &sub_cmd->job;
   struct pvr_pds_upload pds_pixel_event_program;
   struct pvr_framebuffer *framebuffer = render_pass_info->framebuffer;
   struct pvr_spm_bgobj_state *spm_bgobj_state =
         struct pvr_render_target *render_target;
            if (sub_cmd->barrier_store) {
      /* There can only ever be one frag job running on the hardware at any one
   * time, and a context switch is not allowed mid-tile, so instead of
   * allocating a new scratch buffer we can reuse the SPM scratch buffer to
   * perform the store.
   * So use the SPM EOT program with the SPM PBE reg words in order to store
   * the render to the SPM scratch buffer.
            memcpy(job->pbe_reg_words,
         &framebuffer->spm_eot_state_per_render[0].pbe_reg_words,
   job->pds_pixel_event_data_offset =
      framebuffer->spm_eot_state_per_render[0]
   } else {
                        memcpy(job->pbe_reg_words,
                  result = pvr_sub_cmd_gfx_per_job_fragment_programs_create_and_upload(
      cmd_buffer,
   emit_state.emit_count,
   emit_state.pbe_cs_words[0],
      if (result != VK_SUCCESS)
                        if (sub_cmd->barrier_load) {
      job->enable_bg_tag = true;
                     STATIC_ASSERT(ARRAY_SIZE(job->pds_bgnd_reg_values) ==
         typed_memcpy(job->pds_bgnd_reg_values,
            } else if (hw_render->load_op) {
      const struct pvr_load_op *load_op = hw_render->load_op;
                     /* FIXME: Should we free the PDS pixel event data or let it be freed
   * when the pool gets emptied?
   */
   result = pvr_load_op_data_create_and_upload(cmd_buffer,
               if (result != VK_SUCCESS)
            job->enable_bg_tag = render_pass_info->enable_bg_tag;
            pvr_pds_bgnd_pack_state(load_op,
                     /* TODO: In some cases a PR can be removed by storing to the color attachment
   * and have the background object load directly from it instead of using the
   * scratch buffer. In those cases we can also set this to "false" and avoid
   * extra fw overhead.
   */
   /* The scratch buffer is always needed and allocated to avoid data loss in
   * case SPM is hit so set the flag unconditionally.
   */
            memcpy(job->pr_pbe_reg_words,
         &framebuffer->spm_eot_state_per_render[0].pbe_reg_words,
   job->pr_pds_pixel_event_data_offset =
            STATIC_ASSERT(ARRAY_SIZE(job->pds_pr_bgnd_reg_values) ==
         typed_memcpy(job->pds_pr_bgnd_reg_values,
                  render_target = pvr_get_render_target(render_pass_info->pass,
                                 if (sub_cmd->depth_bias_bo)
         else
            if (sub_cmd->scissor_bo)
         else
            job->pixel_output_width =
      pvr_pass_get_pixel_output_width(render_pass_info->pass,
               /* Setup depth/stencil job information. */
   if (hw_render->ds_attach_idx != VK_ATTACHMENT_UNUSED) {
      struct pvr_image_view *ds_iview =
                  job->has_depth_attachment = vk_format_has_depth(ds_image->vk.format);
            if (job->has_depth_attachment || job->has_stencil_attachment) {
      uint32_t level_pitch =
         const bool render_area_is_tile_aligned =
         bool store_was_optimised_out = false;
                                 job->ds.stride =
         job->ds.height = ds_iview->vk.extent.height;
   job->ds.physical_extent = (VkExtent2D){
      .width = u_minify(ds_image->physical_extent.width,
         .height = u_minify(ds_image->physical_extent.height,
                              if (hw_render->ds_attach_idx < render_pass_info->clear_value_count) {
      const VkClearDepthStencilValue *const clear_values =
                                 if (job->has_stencil_attachment)
               switch (ds_iview->vk.format) {
   case VK_FORMAT_D16_UNORM:
                  case VK_FORMAT_S8_UINT:
   case VK_FORMAT_D32_SFLOAT:
                  case VK_FORMAT_D24_UNORM_S8_UINT:
                  default:
                           if (job->has_depth_attachment) {
      if (hw_render->depth_store || sub_cmd->barrier_store) {
                              if (hw_render->depth_store && render_area_is_tile_aligned &&
      !(sub_cmd->modifies_depth || depth_init_is_clear)) {
   d_store = false;
                  if (d_store && !render_area_is_tile_aligned) {
                           assert(depth_usage != PVR_DEPTH_STENCIL_USAGE_UNDEFINED);
      } else {
                     if (job->has_stencil_attachment) {
      if (hw_render->stencil_store || sub_cmd->barrier_store) {
                              if (hw_render->stencil_store && render_area_is_tile_aligned &&
      !(sub_cmd->modifies_stencil || stencil_init_is_clear)) {
   s_store = false;
                  if (s_store && !render_area_is_tile_aligned) {
         } else if (hw_render->stencil_init == VK_ATTACHMENT_LOAD_OP_LOAD) {
                     assert(stencil_usage != PVR_DEPTH_STENCIL_USAGE_UNDEFINED);
      } else {
                     job->ds.load.d = d_load;
   job->ds.load.s = s_load;
                  /* ZLS can't do masked writes for packed depth stencil formats so if
   * we store anything, we have to store everything.
   */
   if ((job->ds.store.d || job->ds.store.s) &&
      pvr_zls_format_type_is_packed(job->ds.zls_format)) {
                  /* In case we are only operating on one aspect of the attachment we
   * need to load the unused one in order to preserve its contents due
   * to the forced store which might otherwise corrupt it.
   */
                  if (hw_render->stencil_init != VK_ATTACHMENT_LOAD_OP_CLEAR)
               if (pvr_ds_attachment_requires_zls(&job->ds) ||
      store_was_optimised_out) {
               if (pvr_sub_cmd_gfx_requires_ds_subtile_alignment(dev_info, job)) {
      result = pvr_sub_cmd_gfx_align_ds_subtiles(cmd_buffer, sub_cmd);
   if (result != VK_SUCCESS)
            } else {
      job->has_depth_attachment = false;
   job->has_stencil_attachment = false;
               if (hw_render->ds_attach_idx != VK_ATTACHMENT_UNUSED) {
      struct pvr_image_view *iview =
                  /* If the HW render pass has a valid depth/stencil surface, determine the
   * sample count from the attachment's image.
   */
      } else if (hw_render->output_regs_count) {
      /* If the HW render pass has output registers, we have color attachments
   * to write to, so determine the sample count from the count specified for
   * every color attachment in this render.
   */
      } else if (cmd_buffer->state.gfx_pipeline) {
      /* If the HW render pass has no color or depth/stencil attachments, we
   * determine the sample count from the count given during pipeline
   * creation.
   */
      } else if (render_pass_info->pass->attachment_count > 0) {
      /* If we get here, we have a render pass with subpasses containing no
   * attachments. The next best thing is largest of the sample counts
   * specified by the render pass attachment descriptions.
   */
      } else {
      /* No appropriate framebuffer attachment is available. */
   mesa_logw("Defaulting render job sample count to 1.");
               if (sub_cmd->max_tiles_in_flight ==
      PVR_GET_FEATURE_VALUE(dev_info, isp_max_tiles_in_flight, 1U)) {
   /* Use the default limit based on the partition store. */
      } else {
                  job->frag_uses_atomic_ops = sub_cmd->frag_uses_atomic_ops;
   job->disable_compute_overlap = false;
   job->max_shared_registers = cmd_buffer->state.max_shared_regs;
   job->run_frag = true;
               }
      static void
   pvr_sub_cmd_compute_job_init(const struct pvr_physical_device *pdevice,
               {
      sub_cmd->num_shared_regs = MAX2(cmd_buffer->device->idfwdf_state.usc_shareds,
               }
      #define PIXEL_ALLOCATION_SIZE_MAX_IN_BLOCKS \
            static uint32_t
   pvr_compute_flat_slot_size(const struct pvr_physical_device *pdevice,
                     {
      const struct pvr_device_runtime_info *dev_runtime_info =
         const struct pvr_device_info *dev_info = &pdevice->dev_info;
   uint32_t max_workgroups_per_task = ROGUE_CDM_MAX_PACKED_WORKGROUPS_PER_TASK;
   uint32_t max_avail_coeff_regs =
         uint32_t localstore_chunks_count =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(coeff_regs_count),
         /* Ensure that we cannot have more workgroups in a slot than the available
   * number of coefficients allow us to have.
   */
   if (coeff_regs_count > 0U) {
      /* If the geometry or fragment jobs can overlap with the compute job, or
   * if there is a vertex shader already running then we need to consider
   * this in calculating max allowed work-groups.
   */
   if (PVR_HAS_QUIRK(dev_info, 52354) &&
      (PVR_HAS_FEATURE(dev_info, compute_overlap) ||
   PVR_HAS_FEATURE(dev_info, gs_rta_support))) {
   /* Solve for n (number of work-groups per task). All values are in
   * size of common store alloc blocks:
   *
   * n + (2n + 7) * (local_memory_size_max - 1) =
   * 	(coefficient_memory_pool_size) - (7 * pixel_allocation_size_max)
   * ==>
   * n + 2n * (local_memory_size_max - 1) =
   * 	(coefficient_memory_pool_size) - (7 * pixel_allocation_size_max)
   * 	- (7 * (local_memory_size_max - 1))
   * ==>
   * n * (1 + 2 * (local_memory_size_max - 1)) =
   * 	(coefficient_memory_pool_size) - (7 * pixel_allocation_size_max)
   * 	- (7 * (local_memory_size_max - 1))
   * ==>
   * n = ((coefficient_memory_pool_size) -
   * 	(7 * pixel_allocation_size_max) -
   * 	(7 * (local_memory_size_max - 1)) / (1 +
   * 2 * (local_memory_size_max - 1)))
   */
   uint32_t max_common_store_blocks =
                  /* (coefficient_memory_pool_size) - (7 * pixel_allocation_size_max)
   */
                  /* - (7 * (local_memory_size_max - 1)) */
                  /* Divide by (1 + 2 * (local_memory_size_max - 1)) */
                  max_workgroups_per_task =
               } else {
      max_workgroups_per_task =
      MIN2((max_avail_coeff_regs / coeff_regs_count),
               /* max_workgroups_per_task should at least be one. */
            if (total_workitems >= ROGUE_MAX_INSTANCES_PER_TASK) {
      /* In this case, the work group size will have been padded up to the
   * next ROGUE_MAX_INSTANCES_PER_TASK so we just set max instances to be
   * ROGUE_MAX_INSTANCES_PER_TASK.
   */
               /* In this case, the number of instances in the slot must be clamped to
   * accommodate whole work-groups only.
   */
   if (PVR_HAS_QUIRK(dev_info, 49032) || use_barrier) {
      max_workgroups_per_task =
      MIN2(max_workgroups_per_task,
                  return MIN2(total_workitems * max_workgroups_per_task,
      }
      static void
   pvr_compute_generate_control_stream(struct pvr_csb *csb,
               {
               /* Compute kernel 0. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL0, kernel0) {
      kernel0.indirect_present = !!info->indirect_buffer_addr.addr;
   kernel0.global_offsets_present = info->global_offsets_present;
   kernel0.usc_common_size = info->usc_common_size;
   kernel0.usc_unified_size = info->usc_unified_size;
   kernel0.pds_temp_size = info->pds_temp_size;
   kernel0.pds_data_size = info->pds_data_size;
   kernel0.usc_target = info->usc_target;
               /* Compute kernel 1. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL1, kernel1) {
      kernel1.data_addr = PVR_DEV_ADDR(info->pds_data_offset);
   kernel1.sd_type = info->sd_type;
               /* Compute kernel 2. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL2, kernel2) {
                  if (info->indirect_buffer_addr.addr) {
      /* Compute kernel 6. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL6, kernel6) {
                  /* Compute kernel 7. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL7, kernel7) {
            } else {
      /* Compute kernel 3. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL3, kernel3) {
      assert(info->global_size[0U] > 0U);
               /* Compute kernel 4. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL4, kernel4) {
      assert(info->global_size[1U] > 0U);
               /* Compute kernel 5. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL5, kernel5) {
      assert(info->global_size[2U] > 0U);
                  /* Compute kernel 8. */
   pvr_csb_emit (csb, CDMCTRL_KERNEL8, kernel8) {
      if (info->max_instances == ROGUE_MAX_INSTANCES_PER_TASK)
         else
            assert(info->local_size[0U] > 0U);
   kernel8.workgroup_size_x = info->local_size[0U] - 1U;
   assert(info->local_size[1U] > 0U);
   kernel8.workgroup_size_y = info->local_size[1U] - 1U;
   assert(info->local_size[2U] > 0U);
                        /* Track the highest amount of shared registers usage in this dispatch.
   * This is used by the FW for context switching, so must be large enough
   * to contain all the shared registers that might be in use for this compute
   * job. Coefficients don't need to be included as the context switch will not
   * happen within the execution of a single workgroup, thus nothing needs to
   * be preserved.
   */
   if (info->usc_common_shared) {
      sub_cmd->num_shared_regs =
         }
      /* TODO: This can be pre-packed and uploaded directly. Would that provide any
   * speed up?
   */
   static void
   pvr_compute_generate_idfwdf(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   bool *const is_sw_barier_required =
         const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
   struct pvr_csb *csb = &sub_cmd->control_stream;
            if (PVR_NEED_SW_COMPUTE_PDS_BARRIER(&pdevice->dev_info) &&
      *is_sw_barier_required) {
   *is_sw_barier_required = false;
      } else {
                  struct pvr_compute_kernel_info info = {
      .indirect_buffer_addr = PVR_DEV_ADDR_INVALID,
   .global_offsets_present = false,
   .usc_common_size = DIV_ROUND_UP(
      PVR_DW_TO_BYTES(cmd_buffer->device->idfwdf_state.usc_shareds),
      .usc_unified_size = 0U,
   .pds_temp_size = 0U,
   .pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(program->data_size),
      .usc_target = PVRX(CDMCTRL_USC_TARGET_ALL),
   .is_fence = false,
   .pds_data_offset = program->data_offset,
   .sd_type = PVRX(CDMCTRL_SD_TYPE_USC),
   .usc_common_shared = true,
   .pds_code_offset = program->code_offset,
   .global_size = { 1U, 1U, 1U },
                        info.max_instances =
      pvr_compute_flat_slot_size(pdevice,
                        }
      void pvr_compute_generate_fence(struct pvr_cmd_buffer *cmd_buffer,
               {
      const struct pvr_pds_upload *program =
         const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
            struct pvr_compute_kernel_info info = {
      .indirect_buffer_addr = PVR_DEV_ADDR_INVALID,
   .global_offsets_present = false,
   .usc_common_size = 0U,
   .usc_unified_size = 0U,
   .pds_temp_size = 0U,
   .pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(program->data_size),
      .usc_target = PVRX(CDMCTRL_USC_TARGET_ANY),
   .is_fence = true,
   .pds_data_offset = program->data_offset,
   .sd_type = PVRX(CDMCTRL_SD_TYPE_PDS),
   .usc_common_shared = deallocate_shareds,
   .pds_code_offset = program->code_offset,
   .global_size = { 1U, 1U, 1U },
               /* We don't need to pad work-group size for this case. */
   /* Here we calculate the slot size. This can depend on the use of barriers,
   * local memory, BRN's or other factors.
   */
               }
      static VkResult
   pvr_cmd_buffer_process_deferred_clears(struct pvr_cmd_buffer *cmd_buffer)
   {
      util_dynarray_foreach (&cmd_buffer->deferred_clears,
                           result = pvr_cmd_buffer_add_transfer_cmd(cmd_buffer, transfer_cmd);
   if (result != VK_SUCCESS)
                           }
      VkResult pvr_cmd_buffer_end_sub_cmd(struct pvr_cmd_buffer *cmd_buffer)
   {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_sub_cmd *sub_cmd = state->current_sub_cmd;
   struct pvr_device *device = cmd_buffer->device;
   const struct pvr_query_pool *query_pool = NULL;
   struct pvr_suballoc_bo *query_bo = NULL;
   size_t query_indices_size = 0;
            /* FIXME: Is this NULL check required because this function is called from
   * pvr_resolve_unemitted_resolve_attachments()? See comment about this
   * function being called twice in a row in pvr_CmdEndRenderPass().
   */
   if (!sub_cmd)
            if (!sub_cmd->owned) {
      state->current_sub_cmd = NULL;
               switch (sub_cmd->type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS: {
               query_indices_size =
            if (query_indices_size > 0) {
      const bool secondary_cont =
      cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY &&
                        if (secondary_cont) {
      util_dynarray_append_dynarray(&state->query_indices,
                        result = pvr_cmd_buffer_upload_general(cmd_buffer,
                                                                     if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      result = pvr_csb_emit_return(&gfx_sub_cmd->control_stream);
                              /* TODO: Check if the sub_cmd can be skipped based on
   * sub_cmd->gfx.empty_cmd flag.
            /* TODO: Set the state in the functions called with the command buffer
   * instead of here.
            result = pvr_cmd_buffer_upload_tables(device, cmd_buffer, gfx_sub_cmd);
   if (result != VK_SUCCESS)
            result = pvr_cmd_buffer_emit_ppp_state(cmd_buffer,
         if (result != VK_SUCCESS)
            result = pvr_csb_emit_terminate(&gfx_sub_cmd->control_stream);
   if (result != VK_SUCCESS)
            result = pvr_sub_cmd_gfx_job_init(&device->pdevice->dev_info,
               if (result != VK_SUCCESS)
            if (pvr_sub_cmd_gfx_requires_split_submit(gfx_sub_cmd)) {
      result = pvr_sub_cmd_gfx_build_terminate_ctrl_stream(device,
               if (result != VK_SUCCESS)
                           case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
   case PVR_SUB_CMD_TYPE_COMPUTE: {
                        result = pvr_csb_emit_terminate(&compute_sub_cmd->control_stream);
   if (result != VK_SUCCESS)
            pvr_sub_cmd_compute_job_init(device->pdevice,
                           case PVR_SUB_CMD_TYPE_TRANSFER:
            case PVR_SUB_CMD_TYPE_EVENT:
            default:
                           /* pvr_cmd_buffer_process_deferred_clears() must be called with a NULL
   * current_sub_cmd.
   *
   * We can start a sub_cmd of a different type from the current sub_cmd only
   * after having ended the current sub_cmd. However, we can't end the current
   * sub_cmd if this depends on starting sub_cmd(s) of a different type. Hence,
   * don't try to start transfer sub_cmd(s) with
   * pvr_cmd_buffer_process_deferred_clears() until the current hasn't ended.
   * Failing to do so we will cause a circular dependency between
   * pvr_cmd_buffer_{end,start}_cmd and blow the stack.
   */
   if (sub_cmd->type == PVR_SUB_CMD_TYPE_GRAPHICS) {
      result = pvr_cmd_buffer_process_deferred_clears(cmd_buffer);
   if (result != VK_SUCCESS)
               if (query_pool) {
               assert(query_bo);
                     /* sizeof(uint32_t) is for the size of single query. */
   query_info.availability_write.num_query_indices =
                  query_info.availability_write.num_queries = query_pool->query_count;
   query_info.availability_write.availability_bo =
            /* Insert a barrier after the graphics sub command and before the
   * query sub command so that the availability write program waits for the
   * fragment shader to complete.
            result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_BARRIER,
   .barrier = {
      .wait_for_stage_mask = PVR_PIPELINE_STAGE_FRAG_BIT,
                                 }
      void pvr_reset_graphics_dirty_state(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct vk_dynamic_graphics_state *const dynamic_state =
            if (start_geom) {
      /*
   * Initial geometry phase state.
   * It's the driver's responsibility to ensure that the state of the
   * hardware is correctly initialized at the start of every geometry
   * phase. This is required to prevent stale state from a previous
   * geometry phase erroneously affecting the next geometry phase.
   *
   * If a geometry phase does not contain any geometry, this restriction
   * can be ignored. If the first draw call in a geometry phase will only
   * update the depth or stencil buffers i.e. ISP_TAGWRITEDISABLE is set
   * in the ISP State Control Word, the PDS State Pointers
   * (TA_PRES_PDSSTATEPTR*) in the first PPP State Update do not need to
   * be supplied, since they will never reach the PDS in the fragment
   * phase.
            cmd_buffer->state.emit_header = (struct PVRX(TA_STATE_HEADER)){
      .pres_stream_out_size = true,
   .pres_ppp_ctrl = true,
   .pres_varying_word2 = true,
   .pres_varying_word1 = true,
   .pres_varying_word0 = true,
   .pres_outselects = true,
   .pres_wclamp = true,
   .pres_viewport = true,
   .pres_region_clip = true,
   .pres_pds_state_ptr0 = true,
   .pres_ispctl_fb = true,
         } else {
      struct PVRX(TA_STATE_HEADER) *const emit_header =
            emit_header->pres_ppp_ctrl = true;
   emit_header->pres_varying_word1 = true;
   emit_header->pres_varying_word0 = true;
   emit_header->pres_outselects = true;
   emit_header->pres_viewport = true;
   emit_header->pres_region_clip = true;
   emit_header->pres_pds_state_ptr0 = true;
   emit_header->pres_ispctl_fb = true;
               memset(&cmd_buffer->state.ppp_state,
                  cmd_buffer->state.dirty.vertex_bindings = true;
            BITSET_SET(dynamic_state->dirty, MESA_VK_DYNAMIC_VP_VIEWPORTS);
      }
      static inline bool
   pvr_cmd_uses_deferred_cs_cmds(const struct pvr_cmd_buffer *const cmd_buffer)
   {
      const VkCommandBufferUsageFlags deferred_control_stream_flags =
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT |
         return cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY &&
            }
      VkResult pvr_cmd_buffer_start_sub_cmd(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_device *device = cmd_buffer->device;
   struct pvr_sub_cmd *sub_cmd;
            /* Check the current status of the buffer. */
   if (vk_command_buffer_has_error(&cmd_buffer->vk))
                     /* TODO: Add proper support for joining consecutive event sub_cmd? */
   if (state->current_sub_cmd) {
      if (state->current_sub_cmd->type == type) {
      /* Continue adding to the current sub command. */
               /* End the current sub command. */
   result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
               sub_cmd = vk_zalloc(&cmd_buffer->vk.pool->alloc,
                     if (!sub_cmd) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
               sub_cmd->type = type;
            switch (type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS:
      sub_cmd->gfx.depth_usage = PVR_DEPTH_STENCIL_USAGE_UNDEFINED;
   sub_cmd->gfx.stencil_usage = PVR_DEPTH_STENCIL_USAGE_UNDEFINED;
   sub_cmd->gfx.modifies_depth = false;
   sub_cmd->gfx.modifies_stencil = false;
   sub_cmd->gfx.max_tiles_in_flight =
      PVR_GET_FEATURE_VALUE(&device->pdevice->dev_info,
            sub_cmd->gfx.hw_render_idx = state->render_pass_info.current_hw_subpass;
   sub_cmd->gfx.framebuffer = state->render_pass_info.framebuffer;
            if (state->vis_test_enabled)
                     if (pvr_cmd_uses_deferred_cs_cmds(cmd_buffer)) {
      pvr_csb_init(device,
            } else {
      pvr_csb_init(device,
                     util_dynarray_init(&sub_cmd->gfx.sec_query_indices, NULL);
         case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
   case PVR_SUB_CMD_TYPE_COMPUTE:
      pvr_csb_init(device,
                     case PVR_SUB_CMD_TYPE_TRANSFER:
      sub_cmd->transfer.transfer_cmds = &sub_cmd->transfer.transfer_cmds_priv;
   list_inithead(sub_cmd->transfer.transfer_cmds);
         case PVR_SUB_CMD_TYPE_EVENT:
            default:
                  list_addtail(&sub_cmd->link, &cmd_buffer->sub_cmds);
               }
      VkResult pvr_cmd_buffer_alloc_mem(struct pvr_cmd_buffer *cmd_buffer,
                     {
      const uint32_t cache_line_size =
         struct pvr_suballoc_bo *suballoc_bo;
   struct pvr_suballocator *allocator;
            if (heap == cmd_buffer->device->heaps.general_heap)
         else if (heap == cmd_buffer->device->heaps.pds_heap)
         else if (heap == cmd_buffer->device->heaps.transfer_frag_heap)
         else if (heap == cmd_buffer->device->heaps.usc_heap)
         else
            result =
         if (result != VK_SUCCESS)
                                 }
      static void pvr_cmd_bind_compute_pipeline(
      const struct pvr_compute_pipeline *const compute_pipeline,
      {
      cmd_buffer->state.compute_pipeline = compute_pipeline;
      }
      static void pvr_cmd_bind_graphics_pipeline(
      const struct pvr_graphics_pipeline *const gfx_pipeline,
      {
      cmd_buffer->state.gfx_pipeline = gfx_pipeline;
            vk_cmd_set_dynamic_graphics_state(&cmd_buffer->vk,
      }
      void pvr_CmdBindPipeline(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
            switch (pipelineBindPoint) {
   case VK_PIPELINE_BIND_POINT_COMPUTE:
      pvr_cmd_bind_compute_pipeline(to_pvr_compute_pipeline(pipeline),
               case VK_PIPELINE_BIND_POINT_GRAPHICS:
      pvr_cmd_bind_graphics_pipeline(to_pvr_graphics_pipeline(pipeline),
               default:
      unreachable("Invalid bind point.");
         }
      #if defined(DEBUG)
   static void check_viewport_quirk_70165(const struct pvr_device *device,
         {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
   float min_vertex_x, max_vertex_x, min_vertex_y, max_vertex_y;
   float min_screen_space_value, max_screen_space_value;
   float sign_to_unsigned_offset, fixed_point_max;
            if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format)) {
      /* Max representable value in 13.4 fixed point format.
   * Round-down to avoid precision issues.
   * Calculated as (2 ** 13) - 2*(2 ** -4)
   */
            if (PVR_HAS_FEATURE(dev_info, screen_size8K)) {
      if (pViewport->width <= 4096 && pViewport->height <= 4096) {
                     /* 2k of the range is negative */
      } else {
                     /* For > 4k renders, the entire range is positive */
         } else {
                     /* 2k of the range is negative */
         } else {
      /* Max representable value in 16.8 fixed point format
   * Calculated as (2 ** 16) - (2 ** -8)
   */
   fixed_point_max = 65535.99609375f;
   guardband_width = pViewport->width / 4.0f;
            /* 4k/20k of the range is negative */
               min_screen_space_value = -sign_to_unsigned_offset;
            min_vertex_x = pViewport->x - guardband_width;
   max_vertex_x = pViewport->x + pViewport->width + guardband_width;
   min_vertex_y = pViewport->y - guardband_height;
   max_vertex_y = pViewport->y + pViewport->height + guardband_height;
   if (min_vertex_x < min_screen_space_value ||
      max_vertex_x > max_screen_space_value ||
   min_vertex_y < min_screen_space_value ||
   max_vertex_y > max_screen_space_value) {
   mesa_logw("Viewport is affected by BRN70165, geometry outside "
         }
   #endif
      void pvr_CmdSetViewport(VkCommandBuffer commandBuffer,
                     {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
            assert(firstViewport < PVR_MAX_VIEWPORTS && viewportCount > 0);
                  #if defined(DEBUG)
      if (PVR_HAS_QUIRK(&cmd_buffer->device->pdevice->dev_info, 70165)) {
      for (uint32_t viewport = 0; viewport < viewportCount; viewport++) {
               #endif
         vk_common_CmdSetViewport(commandBuffer,
                  }
      void pvr_CmdSetDepthBounds(VkCommandBuffer commandBuffer,
               {
         }
      void pvr_CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                                 VkPipelineBindPoint pipelineBindPoint,
   VkPipelineLayout _layout,
   {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                              switch (pipelineBindPoint) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS:
   case VK_PIPELINE_BIND_POINT_COMPUTE:
            default:
      unreachable("Unsupported bind point.");
               if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
      descriptor_state = &cmd_buffer->state.gfx_desc_state;
      } else {
      descriptor_state = &cmd_buffer->state.compute_desc_state;
               for (uint32_t i = 0; i < descriptorSetCount; i++) {
      PVR_FROM_HANDLE(pvr_descriptor_set, set, pDescriptorSets[i]);
            if (descriptor_state->descriptor_sets[index] != set) {
      descriptor_state->descriptor_sets[index] = set;
                  if (dynamicOffsetCount > 0) {
      PVR_FROM_HANDLE(pvr_pipeline_layout, pipeline_layout, _layout);
            for (uint32_t set = 0; set < firstSet; set++)
            assert(set_offset + dynamicOffsetCount <=
            /* From the Vulkan 1.3.238 spec. :
   *
   *    "If any of the sets being bound include dynamic uniform or storage
   *    buffers, then pDynamicOffsets includes one element for each array
   *    element in each dynamic descriptor type binding in each set."
   *
   */
   for (uint32_t i = 0; i < dynamicOffsetCount; i++)
         }
      void pvr_CmdBindVertexBuffers(VkCommandBuffer commandBuffer,
                           {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
            /* We have to defer setting up vertex buffer since we need the buffer
   * stride from the pipeline.
            assert(firstBinding < PVR_MAX_VERTEX_INPUT_BINDINGS &&
                     for (uint32_t i = 0; i < bindingCount; i++) {
      vb[firstBinding + i].buffer = pvr_buffer_from_handle(pBuffers[i]);
                  }
      void pvr_CmdBindIndexBuffer(VkCommandBuffer commandBuffer,
                     {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_buffer, index_buffer, buffer);
            assert(offset < index_buffer->vk.size);
   assert(indexType == VK_INDEX_TYPE_UINT32 ||
                     state->index_buffer_binding.buffer = index_buffer;
   state->index_buffer_binding.offset = offset;
   state->index_buffer_binding.type = indexType;
      }
      void pvr_CmdPushConstants(VkCommandBuffer commandBuffer,
                           VkPipelineLayout layout,
   {
   #if defined(DEBUG)
         #endif
         PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                                       state->push_constants.dirty_stages |= stageFlags;
      }
      static VkResult
   pvr_cmd_buffer_setup_attachments(struct pvr_cmd_buffer *cmd_buffer,
               {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
                     /* Free any previously allocated attachments. */
            if (pass->attachment_count == 0) {
      info->attachments = NULL;
               info->attachments =
      vk_zalloc(&cmd_buffer->vk.pool->alloc,
            pass->attachment_count * sizeof(*info->attachments),
   if (!info->attachments) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
               for (uint32_t i = 0; i < pass->attachment_count; i++)
               }
      static VkResult pvr_init_render_targets(struct pvr_device *device,
               {
      for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
      struct pvr_render_target *render_target =
                     if (!render_target->valid) {
      const struct pvr_renderpass_hwsetup_render *hw_render =
                  result = pvr_render_target_dataset_create(device,
                                 if (result != VK_SUCCESS) {
      pthread_mutex_unlock(&render_target->mutex);
                                          }
      const struct pvr_renderpass_hwsetup_subpass *
   pvr_get_hw_subpass(const struct pvr_render_pass *pass, const uint32_t subpass)
   {
      const struct pvr_renderpass_hw_map *map =
               }
      static void pvr_perform_start_of_render_attachment_clear(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct pvr_framebuffer *framebuffer,
   uint32_t index,
   bool is_depth_stencil,
      {
      ASSERTED static const VkImageAspectFlags dsc_aspect_flags =
      VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT |
      struct pvr_render_pass_info *info = &cmd_buffer->state.render_pass_info;
   const struct pvr_render_pass *pass = info->pass;
   const struct pvr_renderpass_hwsetup *hw_setup = pass->hw_setup;
   const struct pvr_renderpass_hwsetup_render *hw_render =
         VkImageAspectFlags image_aspect;
   struct pvr_image_view *iview;
            if (is_depth_stencil) {
      bool stencil_clear;
   bool depth_clear;
   bool is_stencil;
            assert(hw_render->ds_attach_idx != VK_ATTACHMENT_UNUSED);
                     is_depth = vk_format_has_depth(pass->attachments[view_idx].vk_format);
   is_stencil = vk_format_has_stencil(pass->attachments[view_idx].vk_format);
   depth_clear = hw_render->depth_init == VK_ATTACHMENT_LOAD_OP_CLEAR;
            /* Attempt to clear the ds attachment. Do not erroneously discard an
   * attachment that has no depth clear but has a stencil attachment.
   */
   /* if not (a ∧ c) ∨ (b ∧ d) */
   if (!((is_depth && depth_clear) || (is_stencil && stencil_clear)))
      } else if (hw_render->color_init[index].op != VK_ATTACHMENT_LOAD_OP_CLEAR) {
         } else {
                           /* FIXME: It would be nice if this function and pvr_sub_cmd_gfx_job_init()
   * were doing the same check (even if it's just an assert) to determine if a
   * clear is needed.
   */
   /* If this is single-layer fullscreen, we already do the clears in
   * pvr_sub_cmd_gfx_job_init().
   */
   if (pvr_is_render_area_tile_aligned(cmd_buffer, iview) &&
      framebuffer->layers == 1) {
               image_aspect = vk_format_aspects(pass->attachments[view_idx].vk_format);
            if (image_aspect & VK_IMAGE_ASPECT_DEPTH_BIT &&
      hw_render->depth_init != VK_ATTACHMENT_LOAD_OP_CLEAR) {
               if (image_aspect & VK_IMAGE_ASPECT_STENCIL_BIT &&
      hw_render->stencil_init != VK_ATTACHMENT_LOAD_OP_CLEAR) {
               if (image_aspect != VK_IMAGE_ASPECT_NONE) {
      VkClearAttachment clear_attachment = {
      .aspectMask = image_aspect,
   .colorAttachment = index,
      };
   VkClearRect rect = {
      .rect = info->render_area,
   .baseArrayLayer = 0,
                                       }
      static void
   pvr_perform_start_of_render_clears(struct pvr_cmd_buffer *cmd_buffer)
   {
      struct pvr_render_pass_info *info = &cmd_buffer->state.render_pass_info;
   const struct pvr_framebuffer *framebuffer = info->framebuffer;
   const struct pvr_render_pass *pass = info->pass;
   const struct pvr_renderpass_hwsetup *hw_setup = pass->hw_setup;
   const struct pvr_renderpass_hwsetup_render *hw_render =
            /* Mask of attachment clears using index lists instead of background object
   * to clear.
   */
            for (uint32_t i = 0; i < hw_render->color_init_count; i++) {
      pvr_perform_start_of_render_attachment_clear(cmd_buffer,
                                          /* If we're not using index list for all clears/loads then we need to run
   * the background object on empty tiles.
   */
   if (hw_render->color_init_count &&
      index_list_clear_mask != ((1u << hw_render->color_init_count) - 1u)) {
      } else {
                  if (hw_render->ds_attach_idx != VK_ATTACHMENT_UNUSED) {
               pvr_perform_start_of_render_attachment_clear(cmd_buffer,
                                 if (index_list_clear_mask)
      }
      static void pvr_stash_depth_format(struct pvr_cmd_buffer_state *state,
         {
      const struct pvr_render_pass *pass = state->render_pass_info.pass;
   const struct pvr_renderpass_hwsetup_render *hw_render =
            if (hw_render->ds_attach_idx != VK_ATTACHMENT_UNUSED) {
                     }
      static bool pvr_loadops_contain_clear(struct pvr_renderpass_hwsetup *hw_setup)
   {
      for (uint32_t i = 0; i < hw_setup->render_count; i++) {
      struct pvr_renderpass_hwsetup_render *hw_render = &hw_setup->renders[i];
            for (uint32_t j = 0;
      j < (hw_render->color_init_count * render_targets_count);
   j += render_targets_count) {
   for (uint32_t k = 0; k < hw_render->init_setup.num_render_targets;
      k++) {
   if (hw_render->color_init[j + k].op ==
      VK_ATTACHMENT_LOAD_OP_CLEAR) {
            }
   if (hw_render->depth_init == VK_ATTACHMENT_LOAD_OP_CLEAR ||
      hw_render->stencil_init == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                     }
      static VkResult
   pvr_cmd_buffer_set_clear_values(struct pvr_cmd_buffer *cmd_buffer,
         {
               /* Free any previously allocated clear values. */
            if (pRenderPassBegin->clearValueCount) {
      const size_t size = pRenderPassBegin->clearValueCount *
            state->render_pass_info.clear_values =
      vk_zalloc(&cmd_buffer->vk.pool->alloc,
            size,
   if (!state->render_pass_info.clear_values) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
               memcpy(state->render_pass_info.clear_values,
            } else {
                  state->render_pass_info.clear_value_count =
               }
      /**
   * \brief Indicates whether to use the large or normal clear state words.
   *
   * If the current render area can fit within a quarter of the max framebuffer
   * that the device is capable of, we can use the normal clear state words,
   * otherwise the large clear state words are needed.
   *
   * The requirement of a quarter of the max framebuffer comes from the index
   * count used in the normal clear state words and the vertices uploaded at
   * device creation.
   *
   * \param[in] cmd_buffer The command buffer for the clear.
   * \return true if large clear state words are required.
   */
   static bool
   pvr_is_large_clear_required(const struct pvr_cmd_buffer *const cmd_buffer)
   {
      const struct pvr_device_info *const dev_info =
         const VkRect2D render_area = cmd_buffer->state.render_pass_info.render_area;
   const uint32_t vf_max_x = rogue_get_param_vf_max_x(dev_info);
            return (render_area.extent.width > (vf_max_x / 2) - 1) ||
      }
      static void pvr_emit_clear_words(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct pvr_device *device = cmd_buffer->device;
   struct pvr_csb *csb = &sub_cmd->control_stream;
   uint32_t vdm_state_size_in_dw;
   const uint32_t *vdm_state;
            vdm_state_size_in_dw =
                     stream = pvr_csb_alloc_dwords(csb, vdm_state_size_in_dw);
   if (!stream) {
      pvr_cmd_buffer_set_error_unwarned(cmd_buffer, csb->status);
               if (pvr_is_large_clear_required(cmd_buffer))
         else
                        }
      static VkResult pvr_cs_write_load_op(struct pvr_cmd_buffer *cmd_buffer,
                     {
      const struct pvr_device *device = cmd_buffer->device;
   struct pvr_static_clear_ppp_template template =
         uint32_t pds_state[PVR_STATIC_CLEAR_PDS_STATE_COUNT];
   struct pvr_pds_upload shareds_update_program;
   struct pvr_suballoc_bo *pvr_bo;
            result = pvr_load_op_data_create_and_upload(cmd_buffer,
               if (result != VK_SUCCESS)
                     /* It might look odd that we aren't specifying the code segment's
   * address anywhere. This is because the hardware always assumes that the
   * data size is 2 128bit words and the code segments starts after that.
   */
   pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SHADERBASE],
                              pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_TEXUNICODEBASE],
                  texunicodebase.addr =
               pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SIZEINFO1],
                  /* Dummy coefficient loading program. */
            sizeinfo1.pds_texturestatesize = DIV_ROUND_UP(
                  sizeinfo1.pds_tempsize =
      DIV_ROUND_UP(load_op->temps_count,
            pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SIZEINFO2],
                  sizeinfo2.usc_sharedsize =
      DIV_ROUND_UP(load_op->const_shareds_count,
            /* Dummy coefficient loading program. */
            pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_TEXTUREDATABASE],
                                       pvr_emit_ppp_from_template(&sub_cmd->control_stream, &template, &pvr_bo);
                                 }
      void pvr_CmdBeginRenderPass2(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_framebuffer,
               PVR_FROM_HANDLE(pvr_render_pass, pass, pRenderPassBeginInfo->renderPass);
   PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   const struct pvr_renderpass_hwsetup_subpass *hw_subpass;
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
                     assert(!state->render_pass_info.pass);
            /* FIXME: Create a separate function for everything using pass->subpasses,
   * look at cmd_buffer_begin_subpass() for example. */
   state->render_pass_info.pass = pass;
   state->render_pass_info.framebuffer = framebuffer;
   state->render_pass_info.subpass_idx = 0;
   state->render_pass_info.render_area = pRenderPassBeginInfo->renderArea;
   state->render_pass_info.current_hw_subpass = 0;
   state->render_pass_info.pipeline_bind_point =
         state->render_pass_info.isp_userpass = pass->subpasses[0].isp_userpass;
            result = pvr_cmd_buffer_setup_attachments(cmd_buffer, pass, framebuffer);
   if (result != VK_SUCCESS)
            result = pvr_init_render_targets(cmd_buffer->device, pass, framebuffer);
   if (result != VK_SUCCESS) {
      pvr_cmd_buffer_set_error_unwarned(cmd_buffer, result);
               result = pvr_cmd_buffer_set_clear_values(cmd_buffer, pRenderPassBeginInfo);
   if (result != VK_SUCCESS)
            assert(pass->subpasses[0].pipeline_bind_point ==
            result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_GRAPHICS);
   if (result != VK_SUCCESS)
            /* Run subpass 0 "soft" background object after the actual background
   * object.
   */
   hw_subpass = pvr_get_hw_subpass(pass, 0);
   if (hw_subpass->load_op) {
      result = pvr_cs_write_load_op(cmd_buffer,
                     if (result != VK_SUCCESS)
               pvr_perform_start_of_render_clears(cmd_buffer);
   pvr_stash_depth_format(&cmd_buffer->state,
      }
      VkResult pvr_BeginCommandBuffer(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state;
                     cmd_buffer->usage_flags = pBeginInfo->flags;
            /* VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT must be ignored for
   * primary level command buffers.
   *
   * From the Vulkan 1.0 spec:
   *
   *    VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT specifies that a
   *    secondary command buffer is considered to be entirely inside a render
   *    pass. If this is a primary command buffer, then this bit is ignored.
   */
   if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      cmd_buffer->usage_flags &=
               if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
      if (cmd_buffer->usage_flags &
      VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
   const VkCommandBufferInheritanceInfo *inheritance_info =
                  pass = pvr_render_pass_from_handle(inheritance_info->renderPass);
   state->render_pass_info.pass = pass;
   state->render_pass_info.framebuffer =
         state->render_pass_info.subpass_idx = inheritance_info->subpass;
                  result =
                                                         memset(state->barriers_needed,
                     }
      VkResult pvr_cmd_buffer_add_transfer_cmd(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_sub_cmd_transfer *sub_cmd;
            result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_TRANSFER);
   if (result != VK_SUCCESS)
                                 }
      static VkResult
   pvr_setup_vertex_buffers(struct pvr_cmd_buffer *cmd_buffer,
         {
      const struct pvr_vertex_shader_state *const vertex_state =
         struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   const struct pvr_pds_info *const pds_info = state->pds_shader.info;
   struct pvr_suballoc_bo *pvr_bo;
   const uint8_t *entries;
   uint32_t *dword_buffer;
   uint64_t *qword_buffer;
            result =
      pvr_cmd_buffer_alloc_mem(cmd_buffer,
                  if (result != VK_SUCCESS)
            dword_buffer = (uint32_t *)pvr_bo_suballoc_get_map_addr(pvr_bo);
                     for (uint32_t i = 0; i < pds_info->entry_count; i++) {
      const struct pvr_const_map_entry *const entry_header =
            switch (entry_header->type) {
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*literal);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_DOUTU_ADDRESS: {
      const struct pvr_const_map_entry_doutu_address *const doutu_addr =
         const pvr_dev_addr_t exec_addr =
      PVR_DEV_ADDR_OFFSET(vertex_state->bo->dev_addr,
                        PVR_WRITE(qword_buffer,
                        entries += sizeof(*doutu_addr);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_INSTANCE: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*base_instance);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_VERTEX: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*base_instance);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_VERTEX_ATTRIBUTE_ADDRESS: {
      const struct pvr_const_map_entry_vertex_attribute_address
      *const attribute =
      const struct pvr_vertex_binding *const binding =
         /* In relation to the Vulkan spec. 22.4. Vertex Input Address
   * Calculation:
   *    Adding binding->offset corresponds to calculating the
   *    `bufferBindingAddress`. Adding attribute->offset corresponds to
   *    adding the `attribDesc.offset`. The `effectiveVertexOffset` is
   *    taken care by the PDS program itself with a DDMAD which will
   *    multiply the vertex/instance idx with the binding's stride and
   *    add that to the address provided here.
   */
   const pvr_dev_addr_t addr =
                  PVR_WRITE(qword_buffer,
                        entries += sizeof(*attribute);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_ROBUST_VERTEX_ATTRIBUTE_ADDRESS: {
      const struct pvr_const_map_entry_robust_vertex_attribute_address
      *const attribute =
      (struct pvr_const_map_entry_robust_vertex_attribute_address *)
   const struct pvr_vertex_binding *const binding =
                  if (binding->buffer->vk.size <
      (attribute->offset + attribute->component_size_in_bytes)) {
   /* Replace with load from robustness buffer when no attribute is in
   * range
   */
   addr = PVR_DEV_ADDR_OFFSET(
      cmd_buffer->device->robustness_buffer->vma->dev_addr,
   } else {
      addr = PVR_DEV_ADDR_OFFSET(binding->buffer->dev_addr,
               PVR_WRITE(qword_buffer,
                        entries += sizeof(*attribute);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_VERTEX_ATTRIBUTE_MAX_INDEX: {
      const struct pvr_const_map_entry_vertex_attribute_max_index *attribute =
         const struct pvr_vertex_binding *const binding =
         const uint64_t bound_size = binding->buffer->vk.size - binding->offset;
   const uint32_t attribute_end =
                  if (PVR_HAS_FEATURE(&cmd_buffer->device->pdevice->dev_info,
            /* TODO: PVR_PDS_CONST_MAP_ENTRY_TYPE_VERTEX_ATTRIBUTE_MAX_INDEX
   * has the same define value as
   * PVR_PDS_CONST_MAP_ENTRY_TYPE_VERTEX_ATTR_DDMADT_OOB_BUFFER_SIZE
   * so maybe we want to remove one of the defines or change the
   * values.
   */
   pvr_finishme("Unimplemented robust buffer access with DDMADT");
               /* If the stride is 0 then all attributes use the same single element
   * from the binding so the index can only be up to 0.
   */
   if (bound_size < attribute_end || attribute->stride == 0) {
                           /* There's one last attribute that can fit in. */
   if (bound_size % attribute->stride >= attribute_end)
               PVR_WRITE(dword_buffer,
                        entries += sizeof(*attribute);
               default:
      unreachable("Unsupported data section map");
                  state->pds_vertex_attrib_offset =
      pvr_bo->dev_addr.addr -
            }
      static VkResult pvr_setup_descriptor_mappings_old(
      struct pvr_cmd_buffer *const cmd_buffer,
   enum pvr_stage_allocation stage,
   const struct pvr_stage_allocation_descriptor_state *descriptor_state,
   const pvr_dev_addr_t *const num_worgroups_buff_addr,
      {
      const struct pvr_pds_info *const pds_info = &descriptor_state->pds_info;
   const struct pvr_descriptor_state *desc_state;
   struct pvr_suballoc_bo *pvr_bo;
   const uint8_t *entries;
   uint32_t *dword_buffer;
   uint64_t *qword_buffer;
            if (!pds_info->data_size_in_dwords)
            result =
      pvr_cmd_buffer_alloc_mem(cmd_buffer,
                  if (result != VK_SUCCESS)
            dword_buffer = (uint32_t *)pvr_bo_suballoc_get_map_addr(pvr_bo);
                     switch (stage) {
   case PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY:
   case PVR_STAGE_ALLOCATION_FRAGMENT:
      desc_state = &cmd_buffer->state.gfx_desc_state;
         case PVR_STAGE_ALLOCATION_COMPUTE:
      desc_state = &cmd_buffer->state.compute_desc_state;
         default:
      unreachable("Unsupported stage.");
               for (uint32_t i = 0; i < pds_info->entry_count; i++) {
      const struct pvr_const_map_entry *const entry_header =
            switch (entry_header->type) {
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*literal);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_CONSTANT_BUFFER: {
      const struct pvr_const_map_entry_constant_buffer *const_buffer_entry =
         const uint32_t desc_set = const_buffer_entry->desc_set;
   const uint32_t binding = const_buffer_entry->binding;
   const struct pvr_descriptor_set *descriptor_set;
                                 /* TODO: Handle dynamic buffers. */
                  assert(descriptor->buffer_desc_range ==
                        buffer_addr =
                  PVR_WRITE(qword_buffer,
                        entries += sizeof(*const_buffer_entry);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_DESCRIPTOR_SET: {
      const struct pvr_const_map_entry_descriptor_set *desc_set_entry =
         const uint32_t desc_set_num = desc_set_entry->descriptor_set;
   const struct pvr_descriptor_set *descriptor_set;
                           /* TODO: Remove this when the compiler provides us with usage info?
   */
   /* We skip DMAing unbound descriptor sets. */
   if (!(desc_state->valid_mask & BITFIELD_BIT(desc_set_num))) {
                     /* The code segment contains a DOUT instructions so in the data
   * section we have to write a DOUTD_SRC0 and DOUTD_SRC1.
   * We'll write 0 for DOUTD_SRC0 since we don't have a buffer to DMA.
   * We're expecting a LITERAL32 entry containing the value for
   * DOUTD_SRC1 next so let's make sure we get it and write it
   * with BSIZE to 0 disabling the DMA operation.
   * We don't want the LITERAL32 to be processed as normal otherwise
                                          zero_literal_value =
                  PVR_WRITE(qword_buffer,
                        PVR_WRITE(dword_buffer,
                        entries += sizeof(*literal);
   i++;
                                 if (desc_set_entry->primary) {
      desc_portion_offset =
      descriptor_set->layout->memory_layout_in_dwords_per_stage[stage]
   } else {
      desc_portion_offset =
      descriptor_set->layout->memory_layout_in_dwords_per_stage[stage]
                                 desc_set_addr = PVR_DEV_ADDR_OFFSET(
                  PVR_WRITE(qword_buffer,
                        entries += sizeof(*desc_set_entry);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_SPECIAL_BUFFER: {
                     switch (special_buff_entry->buffer_type) {
                     PVR_WRITE(qword_buffer,
            addr,
                  case PVR_BUFFER_TYPE_BLEND_CONSTS:
      /* TODO: See if instead of reusing the blend constant buffer type
   * entry, we can setup a new buffer type specifically for
   * num_workgroups or other built-in variables. The mappings are
   * setup at pipeline creation when creating the descriptor program.
   */
                     /* TODO: Check if we need to offset this (e.g. for just y and z),
   * or cope with any reordering?
   */
   PVR_WRITE(qword_buffer,
            num_worgroups_buff_addr->addr,
   } else {
                     default:
                  entries += sizeof(*special_buff_entry);
               default:
                     *descriptor_data_offset_out =
      pvr_bo->dev_addr.addr -
            }
      /* Note that the descriptor set doesn't have any space for dynamic buffer
   * descriptors so this works on the assumption that you have a buffer with space
   * for them at the end.
   */
   static uint16_t pvr_get_dynamic_descriptor_primary_offset(
      const struct pvr_device *device,
   const struct pvr_descriptor_set_layout *layout,
   const struct pvr_descriptor_set_layout_binding *binding,
   const uint32_t stage,
      {
      struct pvr_descriptor_size_info size_info;
            assert(binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                           offset = layout->total_size_in_dwords;
   offset += binding->per_stage_offset_in_dwords[stage].primary;
            /* Offset must be less than * 16bits. */
               }
      /* Note that the descriptor set doesn't have any space for dynamic buffer
   * descriptors so this works on the assumption that you have a buffer with space
   * for them at the end.
   */
   static uint16_t pvr_get_dynamic_descriptor_secondary_offset(
      const struct pvr_device *device,
   const struct pvr_descriptor_set_layout *layout,
   const struct pvr_descriptor_set_layout_binding *binding,
   const uint32_t stage,
      {
      struct pvr_descriptor_size_info size_info;
            assert(binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC ||
                           offset = layout->total_size_in_dwords;
   offset +=
         offset += binding->per_stage_offset_in_dwords[stage].secondary;
            /* Offset must be less than * 16bits. */
               }
      /**
   * \brief Upload a copy of the descriptor set with dynamic buffer offsets
   * applied.
   */
   /* TODO: We should probably make the compiler aware of the dynamic descriptors.
   * We could use push constants like Anv seems to do. This would avoid having to
   * duplicate all sets containing dynamic descriptors each time the offsets are
   * updated.
   */
   static VkResult pvr_cmd_buffer_upload_patched_desc_set(
      struct pvr_cmd_buffer *cmd_buffer,
   const struct pvr_descriptor_set *desc_set,
   const uint32_t *dynamic_offsets,
      {
      const struct pvr_descriptor_set_layout *layout = desc_set->layout;
   const uint64_t normal_desc_set_size =
         const uint64_t dynamic_descs_size =
         struct pvr_descriptor_size_info dynamic_uniform_buffer_size_info;
   struct pvr_descriptor_size_info dynamic_storage_buffer_size_info;
   struct pvr_device *device = cmd_buffer->device;
   struct pvr_suballoc_bo *patched_desc_set_bo;
   uint32_t *src_mem_ptr, *dst_mem_ptr;
   uint32_t desc_idx_offset = 0;
                     pvr_descriptor_size_info_init(device,
               pvr_descriptor_size_info_init(device,
                  /* TODO: In the descriptor set we don't account for dynamic buffer
   * descriptors and take care of them in the pipeline layout. The pipeline
   * layout allocates them at the beginning but let's put them at the end just
   * because it makes things a bit easier. Ideally we should be using the
   * pipeline layout and use the offsets from the pipeline layout to patch
   * descriptors.
   */
   result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            src_mem_ptr = (uint32_t *)pvr_bo_suballoc_get_map_addr(desc_set->pvr_bo);
                     for (uint32_t i = 0; i < desc_set->layout->binding_count; i++) {
      const struct pvr_descriptor_set_layout_binding *binding =
         const struct pvr_descriptor *descriptors =
                  if (binding->type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
         else if (binding->type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
         else
            for (uint32_t stage = 0; stage < PVR_STAGE_ALLOCATION_COUNT; stage++) {
                                    /* Get the offsets for the first dynamic descriptor in the current
   * binding.
   */
   primary_offset =
      pvr_get_dynamic_descriptor_primary_offset(device,
                        secondary_offset =
      pvr_get_dynamic_descriptor_secondary_offset(device,
                           /* clang-format off */
   for (uint32_t desc_idx = 0;
      desc_idx < binding->descriptor_count;
   desc_idx++) {
   /* clang-format on */
   const pvr_dev_addr_t addr =
      PVR_DEV_ADDR_OFFSET(descriptors[desc_idx].buffer_dev_addr,
      const VkDeviceSize range =
            #if defined(DEBUG)
                              desc_primary_offset =
      pvr_get_dynamic_descriptor_primary_offset(device,
                        desc_secondary_offset =
      pvr_get_dynamic_descriptor_secondary_offset(device,
                           /* Check the assumption that the descriptors within a binding, for
   * a particular stage, are allocated consecutively.
   */
   assert(desc_primary_offset ==
         #endif
                           memcpy(dst_mem_ptr + primary_offset + size_info->primary * desc_idx,
         &addr.addr,
   memcpy(dst_mem_ptr + secondary_offset +
                                                      }
      #define PVR_SELECT(_geom, _frag, _compute)         \
      (stage == PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY) \
      ? (_geom)                                    \
      static VkResult
   pvr_cmd_buffer_upload_desc_set_table(struct pvr_cmd_buffer *const cmd_buffer,
               {
      uint64_t bound_desc_sets[PVR_MAX_DESCRIPTOR_SETS];
   const struct pvr_descriptor_state *desc_state;
   struct pvr_suballoc_bo *suballoc_bo;
   uint32_t dynamic_offset_idx = 0;
            switch (stage) {
   case PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY:
   case PVR_STAGE_ALLOCATION_FRAGMENT:
   case PVR_STAGE_ALLOCATION_COMPUTE:
            default:
      unreachable("Unsupported stage.");
               desc_state = PVR_SELECT(&cmd_buffer->state.gfx_desc_state,
                  for (uint32_t set = 0; set < ARRAY_SIZE(bound_desc_sets); set++)
            assert(util_last_bit(desc_state->valid_mask) <= ARRAY_SIZE(bound_desc_sets));
   for (uint32_t set = 0; set < util_last_bit(desc_state->valid_mask); set++) {
               if (!(desc_state->valid_mask & BITFIELD_BIT(set))) {
      const struct pvr_pipeline_layout *pipeline_layout =
      PVR_SELECT(cmd_buffer->state.gfx_pipeline->base.layout,
                                                                  /* TODO: Is it better if we don't set the valid_mask for empty sets? */
   if (desc_set->layout->descriptor_count == 0)
            if (desc_set->layout->dynamic_buffer_count > 0) {
                              result = pvr_cmd_buffer_upload_patched_desc_set(
      cmd_buffer,
   desc_set,
   &desc_state->dynamic_offsets[dynamic_offset_idx],
                                 } else {
                     result = pvr_cmd_buffer_upload_general(cmd_buffer,
                     if (result != VK_SUCCESS)
            *addr_out = suballoc_bo->dev_addr;
      }
      static VkResult
   pvr_process_addr_literal(struct pvr_cmd_buffer *cmd_buffer,
                     {
               switch (addr_literal_type) {
   case PVR_PDS_ADDR_LITERAL_DESC_SET_ADDRS_TABLE: {
      /* TODO: Maybe we want to free pvr_bo? And only when the data
   * section is written successfully we link all bos to the command
   * buffer.
   */
   result =
         if (result != VK_SUCCESS)
                        case PVR_PDS_ADDR_LITERAL_PUSH_CONSTS: {
      const struct pvr_pipeline_layout *layout =
      PVR_SELECT(cmd_buffer->state.gfx_pipeline->base.layout,
            const uint32_t push_constants_offset =
      PVR_SELECT(layout->vert_push_constants_offset,
               *addr_out = PVR_DEV_ADDR_OFFSET(cmd_buffer->state.push_constants.dev_addr,
                     case PVR_PDS_ADDR_LITERAL_BLEND_CONSTANTS: {
      float *blend_consts =
         size_t size =
                  result = pvr_cmd_buffer_upload_general(cmd_buffer,
                     if (result != VK_SUCCESS)
                                 default:
                     }
      #undef PVR_SELECT
      static VkResult pvr_setup_descriptor_mappings_new(
      struct pvr_cmd_buffer *const cmd_buffer,
   enum pvr_stage_allocation stage,
   const struct pvr_stage_allocation_descriptor_state *descriptor_state,
      {
      const struct pvr_pds_info *const pds_info = &descriptor_state->pds_info;
   struct pvr_suballoc_bo *pvr_bo;
   const uint8_t *entries;
   uint32_t *dword_buffer;
   uint64_t *qword_buffer;
            if (!pds_info->data_size_in_dwords)
            result =
      pvr_cmd_buffer_alloc_mem(cmd_buffer,
                  if (result != VK_SUCCESS)
            dword_buffer = (uint32_t *)pvr_bo_suballoc_get_map_addr(pvr_bo);
                     switch (stage) {
   case PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY:
   case PVR_STAGE_ALLOCATION_FRAGMENT:
   case PVR_STAGE_ALLOCATION_COMPUTE:
            default:
      unreachable("Unsupported stage.");
               for (uint32_t i = 0; i < pds_info->entry_count; i++) {
      const struct pvr_const_map_entry *const entry_header =
            switch (entry_header->type) {
   case PVR_PDS_CONST_MAP_ENTRY_TYPE_LITERAL32: {
                     PVR_WRITE(dword_buffer,
                        entries += sizeof(*literal);
               case PVR_PDS_CONST_MAP_ENTRY_TYPE_ADDR_LITERAL_BUFFER: {
      const struct pvr_pds_const_map_entry_addr_literal_buffer
      *const addr_literal_buffer_entry =
      struct pvr_device *device = cmd_buffer->device;
   struct pvr_suballoc_bo *addr_literal_buffer_bo;
                  result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                                                            PVR_WRITE(qword_buffer,
                        for (uint32_t j = i + 1; j < pds_info->entry_count; j++) {
      const struct pvr_const_map_entry *const entry_header =
                                                      result = pvr_process_addr_literal(cmd_buffer,
                                                                                             default:
                     *descriptor_data_offset_out =
      pvr_bo->dev_addr.addr -
            }
      static VkResult pvr_setup_descriptor_mappings(
      struct pvr_cmd_buffer *const cmd_buffer,
   enum pvr_stage_allocation stage,
   const struct pvr_stage_allocation_descriptor_state *descriptor_state,
   const pvr_dev_addr_t *const num_worgroups_buff_addr,
      {
      const bool old_path =
            if (old_path) {
      return pvr_setup_descriptor_mappings_old(cmd_buffer,
                                 return pvr_setup_descriptor_mappings_new(cmd_buffer,
                  }
      static void pvr_compute_update_shared(struct pvr_cmd_buffer *cmd_buffer,
         {
      const struct pvr_device *device = cmd_buffer->device;
   const struct pvr_physical_device *pdevice = device->pdevice;
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_csb *csb = &sub_cmd->control_stream;
   const struct pvr_compute_pipeline *pipeline = state->compute_pipeline;
   const uint32_t const_shared_regs =
                  /* No shared regs, no need to use an allocation kernel. */
   if (!const_shared_regs)
            /* Accumulate the MAX number of shared registers across the kernels in this
   * dispatch. This is used by the FW for context switching, so must be large
   * enough to contain all the shared registers that might be in use for this
   * compute job. Coefficients don't need to be included as the context switch
   * will not happen within the execution of a single workgroup, thus nothing
   * needs to be preserved.
   */
            info = (struct pvr_compute_kernel_info){
      .indirect_buffer_addr = PVR_DEV_ADDR_INVALID,
            .usc_target = PVRX(CDMCTRL_USC_TARGET_ALL),
   .usc_common_shared = true,
   .usc_common_size =
                  .local_size = { 1, 1, 1 },
               /* Sometimes we don't have a secondary program if there were no constants to
   * write, but we still need to run a PDS program to accomplish the
   * allocation of the local/common store shared registers. Use the
   * pre-uploaded empty PDS program in this instance.
   */
   if (pipeline->descriptor_state.pds_info.code_size_in_dwords) {
      uint32_t pds_data_size_in_dwords =
            info.pds_data_offset = state->pds_compute_descriptor_data_offset;
   info.pds_data_size =
                  /* Check that we have upload the code section. */
   assert(pipeline->descriptor_state.pds_code.code_size);
      } else {
               info.pds_data_offset = program->data_offset;
   info.pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(program->data_size),
                           info.max_instances =
               }
      void pvr_compute_update_shared_private(
      struct pvr_cmd_buffer *cmd_buffer,
   struct pvr_sub_cmd_compute *const sub_cmd,
      {
      const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   const uint32_t const_shared_regs = pipeline->const_shared_regs_count;
   struct pvr_csb *csb = &sub_cmd->control_stream;
            /* No shared regs, no need to use an allocation kernel. */
   if (!const_shared_regs)
            /* See comment in pvr_compute_update_shared() for details on this. */
            info = (struct pvr_compute_kernel_info){
      .indirect_buffer_addr = PVR_DEV_ADDR_INVALID,
   .usc_common_size =
      DIV_ROUND_UP(const_shared_regs,
      .pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(pipeline->pds_shared_update_data_size_dw),
      .usc_target = PVRX(CDMCTRL_USC_TARGET_ALL),
   .pds_data_offset = pipeline->pds_shared_update_data_offset,
   .pds_code_offset = pipeline->pds_shared_update_code_offset,
   .sd_type = PVRX(CDMCTRL_SD_TYPE_NONE),
   .usc_common_shared = true,
   .local_size = { 1, 1, 1 },
                        info.max_instances =
               }
      static uint32_t
   pvr_compute_flat_pad_workgroup_size(const struct pvr_physical_device *pdevice,
               {
      const struct pvr_device_runtime_info *dev_runtime_info =
         const struct pvr_device_info *dev_info = &pdevice->dev_info;
   uint32_t max_avail_coeff_regs =
         uint32_t coeff_regs_count_aligned =
      ALIGN_POT(coeff_regs_count,
         /* If the work group size is > ROGUE_MAX_INSTANCES_PER_TASK. We now *always*
   * pad the work group size to the next multiple of
   * ROGUE_MAX_INSTANCES_PER_TASK.
   *
   * If we use more than 1/8th of the max coefficient registers then we round
   * work group size up to the next multiple of ROGUE_MAX_INSTANCES_PER_TASK
   */
   /* TODO: See if this can be optimized. */
   if (workgroup_size > ROGUE_MAX_INSTANCES_PER_TASK ||
      coeff_regs_count_aligned > (max_avail_coeff_regs / 8)) {
                           }
      void pvr_compute_update_kernel_private(
      struct pvr_cmd_buffer *cmd_buffer,
   struct pvr_sub_cmd_compute *const sub_cmd,
   struct pvr_private_compute_pipeline *pipeline,
      {
      const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
   const struct pvr_device_runtime_info *dev_runtime_info =
                  struct pvr_compute_kernel_info info = {
      .indirect_buffer_addr = PVR_DEV_ADDR_INVALID,
   .usc_target = PVRX(CDMCTRL_USC_TARGET_ANY),
   .pds_temp_size =
                  .pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(pipeline->pds_data_size_dw),
      .pds_data_offset = pipeline->pds_data_offset,
                     .usc_unified_size =
                  /* clang-format off */
   .global_size = {
      global_workgroup_size[0],
   global_workgroup_size[1],
      },
               uint32_t work_size = pipeline->workgroup_size.width *
                        if (work_size > ROGUE_MAX_INSTANCES_PER_TASK) {
      /* Enforce a single workgroup per cluster through allocation starvation.
   */
      } else {
                  info.usc_common_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(coeff_regs),
         /* Use a whole slot per workgroup. */
                     if (pipeline->const_shared_regs_count > 0)
            work_size =
            info.local_size[0] = work_size;
   info.local_size[1] = 1U;
            info.max_instances =
               }
      /* TODO: Wire up the base_workgroup variant program when implementing
   * VK_KHR_device_group. The values will also need patching into the program.
   */
   static void pvr_compute_update_kernel(
      struct pvr_cmd_buffer *cmd_buffer,
   struct pvr_sub_cmd_compute *const sub_cmd,
   pvr_dev_addr_t indirect_addr,
      {
      const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
   const struct pvr_device_runtime_info *dev_runtime_info =
         struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_csb *csb = &sub_cmd->control_stream;
   const struct pvr_compute_pipeline *pipeline = state->compute_pipeline;
   const struct pvr_compute_shader_state *shader_state =
                  struct pvr_compute_kernel_info info = {
      .indirect_buffer_addr = indirect_addr,
   .usc_target = PVRX(CDMCTRL_USC_TARGET_ANY),
   .pds_temp_size =
                  .pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(program_info->data_size_in_dwords),
      .pds_data_offset = pipeline->primary_program.data_offset,
                     .usc_unified_size =
                  /* clang-format off */
   .global_size = {
      global_workgroup_size[0],
   global_workgroup_size[1],
      },
               uint32_t work_size = shader_state->work_size;
            if (work_size > ROGUE_MAX_INSTANCES_PER_TASK) {
      /* Enforce a single workgroup per cluster through allocation starvation.
   */
      } else {
                  info.usc_common_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(coeff_regs),
         /* Use a whole slot per workgroup. */
                     if (shader_state->const_shared_reg_count > 0)
            work_size =
            info.local_size[0] = work_size;
   info.local_size[1] = 1U;
            info.max_instances =
               }
      static VkResult pvr_cmd_upload_push_consts(struct pvr_cmd_buffer *cmd_buffer)
   {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_suballoc_bo *suballoc_bo;
            /* TODO: Here are some possible optimizations/things to consider:
   *
   *    - Currently we upload maxPushConstantsSize. The application might only
   *      be using a portion of that so we might end up with unused memory.
   *      Should we be smarter about this. If we intend to upload the push
   *      consts into shareds, we definitely want to do avoid reserving unused
   *      regs.
   *
   *    - For now we have to upload to a new buffer each time since the shaders
   *      access the push constants from memory. If we were to reuse the same
   *      buffer we might update the contents out of sync with job submission
   *      and the shaders will see the updated contents while the command
   *      buffer was still being recorded and not yet submitted.
   *      If we were to upload the push constants directly to shared regs we
   *      could reuse the same buffer (avoiding extra allocation overhead)
   *      since the contents will be DMAed only on job submission when the
   *      control stream is processed and the PDS program is executed. This
   *      approach would also allow us to avoid regenerating the PDS data
   *      section in some cases since the buffer address will be constants.
            if (cmd_buffer->state.push_constants.uploaded)
            result = pvr_cmd_buffer_upload_general(cmd_buffer,
                     if (result != VK_SUCCESS)
            cmd_buffer->state.push_constants.dev_addr = suballoc_bo->dev_addr;
               }
      static void pvr_cmd_dispatch(
      struct pvr_cmd_buffer *const cmd_buffer,
   const pvr_dev_addr_t indirect_addr,
      {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   const struct pvr_compute_pipeline *compute_pipeline =
         struct pvr_sub_cmd_compute *sub_cmd;
                     sub_cmd = &state->current_sub_cmd->compute;
   sub_cmd->uses_atomic_ops |= compute_pipeline->shader_state.uses_atomic_ops;
            if (state->push_constants.dirty_stages & VK_SHADER_STAGE_COMPUTE_BIT) {
      result = pvr_cmd_upload_push_consts(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Regenerate the PDS program to use the new push consts buffer. */
                        if (compute_pipeline->shader_state.uses_num_workgroups) {
               if (indirect_addr.addr) {
         } else {
               result = pvr_cmd_buffer_upload_general(cmd_buffer,
                                                      result = pvr_setup_descriptor_mappings(
      cmd_buffer,
   PVR_STAGE_ALLOCATION_COMPUTE,
   &compute_pipeline->descriptor_state,
   &descriptor_data_offset_out,
      if (result != VK_SUCCESS)
      } else if ((compute_pipeline->base.layout
                        result = pvr_setup_descriptor_mappings(
      cmd_buffer,
   PVR_STAGE_ALLOCATION_COMPUTE,
   &compute_pipeline->descriptor_state,
   NULL,
      if (result != VK_SUCCESS)
               pvr_compute_update_shared(cmd_buffer, sub_cmd);
      }
      void pvr_CmdDispatch(VkCommandBuffer commandBuffer,
                     {
                        if (!groupCountX || !groupCountY || !groupCountZ)
            pvr_cmd_dispatch(cmd_buffer,
            }
      void pvr_CmdDispatchIndirect(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
                     pvr_cmd_dispatch(cmd_buffer,
            }
      static void
   pvr_update_draw_state(struct pvr_cmd_buffer_state *const state,
         {
      /* We don't have a state to tell us that base_instance is being used so it
   * gets used as a boolean - 0 means we'll use a pds program that skips the
   * base instance addition. If the base_instance gets used (and the last
   * draw's base_instance was 0) then we switch to the BASE_INSTANCE attrib
   * program.
   *
   * If base_instance changes then we only need to update the data section.
   *
   * The only draw call state that doesn't really matter is the start vertex
   * as that is handled properly in the VDM state in all cases.
   */
   if ((state->draw_state.draw_indexed != draw_state->draw_indexed) ||
      (state->draw_state.draw_indirect != draw_state->draw_indirect) ||
   (state->draw_state.base_instance == 0 &&
   draw_state->base_instance != 0)) {
      } else if (state->draw_state.base_instance != draw_state->base_instance) {
                     }
      static uint32_t pvr_calc_shared_regs_count(
         {
      const struct pvr_pipeline_stage_state *const vertex_state =
            uint32_t shared_regs = vertex_state->const_shared_reg_count +
            if (gfx_pipeline->shader_state.fragment.bo) {
      const struct pvr_pipeline_stage_state *const fragment_state =
            uint32_t fragment_regs = fragment_state->const_shared_reg_count +
                           }
      static void
   pvr_emit_dirty_pds_state(const struct pvr_cmd_buffer *const cmd_buffer,
               {
      const struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   const struct pvr_stage_allocation_descriptor_state
      *const vertex_descriptor_state =
      const struct pvr_pipeline_stage_state *const vertex_stage_state =
                  if (!vertex_descriptor_state->pds_info.code_size_in_dwords)
                     pvr_csb_emit (csb, VDMCTRL_PDS_STATE0, state0) {
               state0.usc_common_size =
                  state0.pds_data_size = DIV_ROUND_UP(
      PVR_DW_TO_BYTES(vertex_descriptor_state->pds_info.data_size_in_dwords),
            pvr_csb_emit (csb, VDMCTRL_PDS_STATE1, state1) {
      state1.pds_data_addr = PVR_DEV_ADDR(pds_vertex_descriptor_data_offset);
               pvr_csb_emit (csb, VDMCTRL_PDS_STATE2, state2) {
      state2.pds_code_addr =
                  }
      static void pvr_setup_output_select(struct pvr_cmd_buffer *const cmd_buffer)
   {
      const struct pvr_graphics_pipeline *const gfx_pipeline =
         const struct pvr_vertex_shader_state *const vertex_state =
         struct vk_dynamic_graphics_state *const dynamic_state =
         struct PVRX(TA_STATE_HEADER) *const header = &cmd_buffer->state.emit_header;
   struct pvr_ppp_state *const ppp_state = &cmd_buffer->state.ppp_state;
                     pvr_csb_pack (&output_selects, TA_OUTPUT_SEL, state) {
      state.rhw_pres = true;
   state.vtxsize = DIV_ROUND_UP(vertex_state->vertex_output_size, 4U);
   state.psprite_size_pres = (dynamic_state->ia.primitive_topology ==
               if (ppp_state->output_selects != output_selects) {
      ppp_state->output_selects = output_selects;
               if (ppp_state->varying_word[0] != vertex_state->varying[0]) {
      ppp_state->varying_word[0] = vertex_state->varying[0];
               if (ppp_state->varying_word[1] != vertex_state->varying[1]) {
      ppp_state->varying_word[1] = vertex_state->varying[1];
         }
      static void
   pvr_setup_isp_faces_and_control(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct PVRX(TA_STATE_HEADER) *const header = &cmd_buffer->state.emit_header;
   const struct pvr_fragment_shader_state *const fragment_shader_state =
         const struct pvr_render_pass_info *const pass_info =
         struct vk_dynamic_graphics_state *dynamic_state =
                  const bool rasterizer_discard = dynamic_state->rs.rasterizer_discard_enable;
   const uint32_t subpass_idx = pass_info->subpass_idx;
   const uint32_t depth_stencil_attachment_idx =
         const struct pvr_render_pass_attachment *const attachment =
      depth_stencil_attachment_idx != VK_ATTACHMENT_UNUSED
               const enum PVRX(TA_OBJTYPE)
            const VkImageAspectFlags ds_aspects =
      (!rasterizer_discard && attachment)
      ? vk_format_aspects(attachment->vk_format) &
            /* This is deliberately a full copy rather than a pointer because
   * vk_optimize_depth_stencil_state() can only be run once against any given
   * instance of vk_depth_stencil_state.
   */
            uint32_t ispb_stencil_off;
   bool is_two_sided = false;
            uint32_t line_width;
   uint32_t common_a;
   uint32_t front_a;
   uint32_t front_b;
   uint32_t back_a;
                     /* Convert to 4.4 fixed point format. */
            /* Subtract 1 to shift values from range [0=0,256=16] to [0=1/16,255=16].
   * If 0 it stays at 0, otherwise we subtract 1.
   */
                     /* TODO: Part of the logic in this function is duplicated in another part
   * of the code. E.g. the dcmpmode, and sop1/2/3. Could we do this earlier?
            pvr_csb_pack (&common_a, TA_STATE_ISPA, ispa) {
               ispa.dcmpmode = pvr_ta_cmpmode(ds_state.depth.compare_op);
                              /* Return unpacked ispa structure. dcmpmode, dwritedisable, passtype and
   * objtype are needed by pvr_setup_triangle_merging_flag.
   */
   if (ispa_out)
               /* TODO: Does this actually represent the ispb control word on stencil off?
   * If not, rename the variable.
   */
   pvr_csb_pack (&ispb_stencil_off, TA_STATE_ISPB, ispb) {
      ispb.sop3 = PVRX(TA_ISPB_STENCILOP_KEEP);
   ispb.sop2 = PVRX(TA_ISPB_STENCILOP_KEEP);
   ispb.sop1 = PVRX(TA_ISPB_STENCILOP_KEEP);
               /* FIXME: This logic should be redone and improved. Can we also get rid of
   * the front and back variants?
            front_a = common_a;
            if (ds_state.stencil.test_enable) {
      uint32_t front_a_sref;
            pvr_csb_pack (&front_a_sref, TA_STATE_ISPA, ispa) {
         }
            pvr_csb_pack (&back_a_sref, TA_STATE_ISPA, ispa) {
         }
            pvr_csb_pack (&front_b, TA_STATE_ISPB, ispb) {
                                             ispb.sop3 = pvr_ta_stencilop(front->op.pass);
   ispb.sop2 = pvr_ta_stencilop(front->op.depth_fail);
   ispb.sop1 = pvr_ta_stencilop(front->op.fail);
               pvr_csb_pack (&back_b, TA_STATE_ISPB, ispb) {
                                             ispb.sop3 = pvr_ta_stencilop(back->op.pass);
   ispb.sop2 = pvr_ta_stencilop(back->op.depth_fail);
   ispb.sop1 = pvr_ta_stencilop(back->op.fail);
         } else {
      front_b = ispb_stencil_off;
               if (front_a != back_a || front_b != back_b) {
      if (dynamic_state->rs.cull_mode & VK_CULL_MODE_BACK_BIT) {
         } else if (dynamic_state->rs.cull_mode & VK_CULL_MODE_FRONT_BIT) {
               front_a = back_a;
      } else {
                                                         tmp = front_b;
   front_b = back_b;
               /* HW defaults to stencil off. */
   if (back_b != ispb_stencil_off) {
      header->pres_ispctl_fb = true;
                     if (ds_state.stencil.test_enable && front_b != ispb_stencil_off)
            pvr_csb_pack (&isp_control, TA_STATE_ISPCTL, ispctl) {
               /* TODO: is bo ever NULL? Figure out what to do. */
            ispctl.two_sided = is_two_sided;
            ispctl.dbenable = !rasterizer_discard &&
               if (!rasterizer_discard && cmd_buffer->state.vis_test_enabled) {
      ispctl.vistest = true;
                                             ppp_state->isp.control = isp_control;
   ppp_state->isp.front_a = front_a;
   ppp_state->isp.front_b = front_b;
   ppp_state->isp.back_a = back_a;
      }
      static float
   pvr_calculate_final_depth_bias_contant_factor(struct pvr_device_info *dev_info,
               {
      /* Information for future modifiers of these depth bias calculations.
   * ==================================================================
   * Specified depth bias equations scale the specified constant factor by a
   * value 'r' that is guaranteed to cause a resolvable difference in depth
   * across the entire range of depth values.
   * For floating point depth formats 'r' is calculated by taking the maximum
   * exponent across the triangle.
   * For UNORM formats 'r' is constant.
   * Here 'n' is the number of mantissa bits stored in the floating point
   * representation (23 for F32).
   *
   *    UNORM Format -> z += dbcf * r + slope
   *    FLOAT Format -> z += dbcf * 2^(e-n) + slope
   *
   * HW Variations.
   * ==============
   * The HW either always performs the F32 depth bias equation (exponent based
   * r), or in the case of HW that correctly supports the integer depth bias
   * equation for UNORM depth formats, we can select between both equations
   * using the ROGUE_CR_ISP_CTL.dbias_is_int flag - this is required to
   * correctly perform Vulkan UNORM depth bias (constant r).
   *
   *    if ern42307:
   *       if DBIAS_IS_INT_EN:
   *          z += dbcf + slope
   *       else:
   *          z += dbcf * 2^(e-n) + slope
   *    else:
   *       z += dbcf * 2^(e-n) + slope
   *
                     if (PVR_HAS_ERN(dev_info, 42307)) {
      switch (format) {
   case VK_FORMAT_D16_UNORM:
            case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
            default:
                     /* The reasoning behind clamping/nudging the value here is because UNORM
   * depth formats can have higher precision over our underlying D32F
   * representation for some depth ranges.
   *
   * When the HW scales the depth bias value by 2^(e-n) [The 'r' term'] a depth
   * bias of 1 can result in a value smaller than one F32 ULP, which will get
   * quantized to 0 - resulting in no bias.
   *
   * Biasing small values away from zero will ensure that small depth biases of
   * 1 still yield a result and overcome Z-fighting.
   */
   switch (format) {
   case VK_FORMAT_D16_UNORM:
      depth_bias *= 512.0f;
   nudge_factor = 1.0f;
         case VK_FORMAT_D24_UNORM_S8_UINT:
   case VK_FORMAT_X8_D24_UNORM_PACK32:
      depth_bias *= 2.0f;
   nudge_factor = 2.0f;
         default:
      nudge_factor = 0.0f;
               if (nudge_factor != 0.0f) {
      if (depth_bias < 0.0f && depth_bias > -nudge_factor)
         else if (depth_bias > 0.0f && depth_bias < nudge_factor)
                  }
      static void pvr_get_viewport_scissor_overlap(const VkViewport *const viewport,
               {
      /* TODO: See if we can remove this struct. */
   struct pvr_rect {
      int32_t x0, y0;
               /* TODO: Worry about overflow? */
   const struct pvr_rect scissor_rect = {
      .x0 = scissor->offset.x,
   .y0 = scissor->offset.y,
   .x1 = scissor->offset.x + scissor->extent.width,
      };
            assert(viewport->width >= 0.0f);
   assert(scissor_rect.x0 >= 0);
            if (scissor->extent.width == 0 || scissor->extent.height == 0) {
      *rect_out = (VkRect2D){ 0 };
               viewport_rect.x0 = (int32_t)viewport->x;
            /* TODO: Is there a mathematical way of doing all this and then clamp at
   * the end?
   */
   /* We flip the y0 and y1 when height is negative. */
   viewport_rect.y0 = (int32_t)viewport->y + MIN2(0, (int32_t)viewport->height);
            if (scissor_rect.x1 <= viewport_rect.x0 ||
      scissor_rect.y1 <= viewport_rect.y0 ||
   scissor_rect.x0 >= viewport_rect.x1 ||
   scissor_rect.y0 >= viewport_rect.y1) {
   *rect_out = (VkRect2D){ 0 };
               /* Determine the overlapping rectangle. */
   viewport_rect.x0 = MAX2(viewport_rect.x0, scissor_rect.x0);
   viewport_rect.y0 = MAX2(viewport_rect.y0, scissor_rect.y0);
   viewport_rect.x1 = MIN2(viewport_rect.x1, scissor_rect.x1);
            /* TODO: Is this conversion safe? Is this logic right? */
   rect_out->offset.x = (uint32_t)viewport_rect.x0;
   rect_out->offset.y = (uint32_t)viewport_rect.y0;
   rect_out->extent.height = (uint32_t)(viewport_rect.y1 - viewport_rect.y0);
      }
      static inline uint32_t
   pvr_get_geom_region_clip_align_size(struct pvr_device_info *const dev_info)
   {
      /* TODO: This should come from rogue_ppp.xml. */
      }
      static void
   pvr_setup_isp_depth_bias_scissor_state(struct pvr_cmd_buffer *const cmd_buffer)
   {
      struct PVRX(TA_STATE_HEADER) *const header = &cmd_buffer->state.emit_header;
   struct pvr_ppp_state *const ppp_state = &cmd_buffer->state.ppp_state;
   struct vk_dynamic_graphics_state *const dynamic_state =
         const struct PVRX(TA_STATE_ISPCTL) *const ispctl =
         struct pvr_device_info *const dev_info =
            if (ispctl->dbenable &&
      (BITSET_TEST(dynamic_state->dirty,
         cmd_buffer->depth_bias_array.size == 0)) {
   struct pvr_depth_bias_state depth_bias = {
      .constant_factor = pvr_calculate_final_depth_bias_contant_factor(
      dev_info,
   cmd_buffer->state.depth_format,
      .slope_factor = dynamic_state->rs.depth_bias.slope,
               ppp_state->depthbias_scissor_indices.depthbias_index =
                  util_dynarray_append(&cmd_buffer->depth_bias_array,
                              if (ispctl->scenable) {
      const uint32_t region_clip_align_size =
         const VkViewport *const viewport = &dynamic_state->vp.viewports[0];
   const VkRect2D *const scissor = &dynamic_state->vp.scissors[0];
   struct pvr_scissor_words scissor_words;
   VkRect2D overlap_rect;
   uint32_t height;
   uint32_t width;
   uint32_t x;
            /* For region clip. */
   uint32_t bottom;
   uint32_t right;
   uint32_t left;
            /* We don't support multiple viewport calculations. */
   assert(dynamic_state->vp.viewport_count == 1);
   /* We don't support multiple scissor calculations. */
                     x = overlap_rect.offset.x;
   y = overlap_rect.offset.y;
   width = overlap_rect.extent.width;
            pvr_csb_pack (&scissor_words.w0, IPF_SCISSOR_WORD_0, word0) {
      word0.scw0_xmax = x + width;
               pvr_csb_pack (&scissor_words.w1, IPF_SCISSOR_WORD_1, word1) {
      word1.scw1_ymax = y + height;
               if (cmd_buffer->scissor_array.size &&
      cmd_buffer->scissor_words.w0 == scissor_words.w0 &&
   cmd_buffer->scissor_words.w1 == scissor_words.w1) {
                                 left = x / region_clip_align_size;
            /* We prevent right=-1 with the multiplication. */
   /* TODO: Is there a better way of doing this? */
   if ((x + width) != 0U)
         else
            if ((y + height) != 0U)
         else
                     /* FIXME: Should we mask to prevent writing over other words? */
   pvr_csb_pack (&ppp_state->region_clipping.word0, TA_REGION_CLIP0, word0) {
      word0.right = right;
   word0.left = left;
               pvr_csb_pack (&ppp_state->region_clipping.word1, TA_REGION_CLIP1, word1) {
      word1.bottom = bottom;
               ppp_state->depthbias_scissor_indices.scissor_index =
                  util_dynarray_append(&cmd_buffer->scissor_array,
                  header->pres_ispctl_dbsc = true;
         }
      static void
   pvr_setup_triangle_merging_flag(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct PVRX(TA_STATE_HEADER) *const header = &cmd_buffer->state.emit_header;
   struct pvr_ppp_state *const ppp_state = &cmd_buffer->state.ppp_state;
   uint32_t merge_word;
            pvr_csb_pack (&merge_word, TA_STATE_PDS_SIZEINFO2, size_info) {
      /* Disable for lines or punch-through or for DWD and depth compare
   * always.
   */
   if (ispa->objtype == PVRX(TA_OBJTYPE_LINE) ||
      ispa->passtype == PVRX(TA_PASSTYPE_PUNCH_THROUGH) ||
   (ispa->dwritedisable && ispa->dcmpmode == PVRX(TA_CMPMODE_ALWAYS))) {
                  pvr_csb_pack (&mask, TA_STATE_PDS_SIZEINFO2, size_info) {
                           if (merge_word != ppp_state->pds.size_info2) {
      ppp_state->pds.size_info2 = merge_word;
         }
      static void
   pvr_setup_fragment_state_pointers(struct pvr_cmd_buffer *const cmd_buffer,
         {
               const struct pvr_fragment_shader_state *const fragment =
         const struct pvr_stage_allocation_descriptor_state *descriptor_shader_state =
         const struct pvr_pipeline_stage_state *fragment_state =
         const struct pvr_pds_upload *pds_coeff_program =
            const struct pvr_physical_device *pdevice = cmd_buffer->device->pdevice;
   struct PVRX(TA_STATE_HEADER) *const header = &state->emit_header;
            const uint32_t pds_uniform_size =
      DIV_ROUND_UP(descriptor_shader_state->pds_info.data_size_in_dwords,
         const uint32_t pds_varying_state_size =
      DIV_ROUND_UP(pds_coeff_program->data_size,
         const uint32_t usc_varying_size =
      DIV_ROUND_UP(fragment_state->coefficient_size,
         const uint32_t pds_temp_size =
      DIV_ROUND_UP(fragment_state->pds_temps_count,
         const uint32_t usc_shared_size =
      DIV_ROUND_UP(fragment_state->const_shared_reg_count,
         const uint32_t max_tiles_in_flight =
      pvr_calc_fscommon_size_and_tiles_in_flight(
      &pdevice->dev_info,
   &pdevice->dev_runtime_info,
   usc_shared_size *
         uint32_t size_info_mask;
            if (max_tiles_in_flight < sub_cmd->max_tiles_in_flight)
            pvr_csb_pack (&ppp_state->pds.pixel_shader_base,
                  const struct pvr_pds_upload *const pds_upload =
                        if (descriptor_shader_state->pds_code.pvr_bo) {
      pvr_csb_pack (&ppp_state->pds.texture_uniform_code_base,
                  tex_base.addr =
         } else {
                  pvr_csb_pack (&ppp_state->pds.size_info1, TA_STATE_PDS_SIZEINFO1, info1) {
      info1.pds_uniformsize = pds_uniform_size;
   info1.pds_texturestatesize = 0U;
   info1.pds_varyingsize = pds_varying_state_size;
   info1.usc_varyingsize = usc_varying_size;
               pvr_csb_pack (&size_info_mask, TA_STATE_PDS_SIZEINFO2, mask) {
                           pvr_csb_pack (&size_info2, TA_STATE_PDS_SIZEINFO2, info2) {
                           if (pds_coeff_program->pvr_bo) {
               pvr_csb_pack (&ppp_state->pds.varying_base,
                        } else {
                  pvr_csb_pack (&ppp_state->pds.uniform_state_data_base,
                              header->pres_pds_state_ptr0 = true;
      }
      static void pvr_setup_viewport(struct pvr_cmd_buffer *const cmd_buffer)
   {
      struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   struct PVRX(TA_STATE_HEADER) *const header = &state->emit_header;
   struct vk_dynamic_graphics_state *const dynamic_state =
                  if (ppp_state->viewport_count != dynamic_state->vp.viewport_count) {
      ppp_state->viewport_count = dynamic_state->vp.viewport_count;
               if (dynamic_state->rs.rasterizer_discard_enable) {
      /* We don't want to emit any viewport data as it'll just get thrown
   * away. It's after the previous condition because we still want to
   * stash the viewport_count as it's our trigger for when
   * rasterizer discard gets disabled.
   */
   header->pres_viewport = false;
               for (uint32_t i = 0; i < ppp_state->viewport_count; i++) {
      VkViewport *viewport = &dynamic_state->vp.viewports[i];
   uint32_t x_scale = fui(viewport->width * 0.5f);
   uint32_t y_scale = fui(viewport->height * 0.5f);
   uint32_t z_scale = fui(viewport->maxDepth - viewport->minDepth);
   uint32_t x_center = fui(viewport->x + viewport->width * 0.5f);
   uint32_t y_center = fui(viewport->y + viewport->height * 0.5f);
            if (ppp_state->viewports[i].a0 != x_center ||
      ppp_state->viewports[i].m0 != x_scale ||
   ppp_state->viewports[i].a1 != y_center ||
   ppp_state->viewports[i].m1 != y_scale ||
   ppp_state->viewports[i].a2 != z_center ||
   ppp_state->viewports[i].m2 != z_scale) {
   ppp_state->viewports[i].a0 = x_center;
   ppp_state->viewports[i].m0 = x_scale;
   ppp_state->viewports[i].a1 = y_center;
   ppp_state->viewports[i].m1 = y_scale;
                           }
      static void pvr_setup_ppp_control(struct pvr_cmd_buffer *const cmd_buffer)
   {
      struct vk_dynamic_graphics_state *const dynamic_state =
         const VkPrimitiveTopology topology = dynamic_state->ia.primitive_topology;
   struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   struct PVRX(TA_STATE_HEADER) *const header = &state->emit_header;
   struct pvr_ppp_state *const ppp_state = &state->ppp_state;
            pvr_csb_pack (&ppp_control, TA_STATE_PPP_CTRL, control) {
      control.drawclippededges = true;
            if (topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN)
         else
            if (dynamic_state->rs.depth_clamp_enable)
         else
            /* +--- FrontIsCCW?
   * | +--- Cull Front?
   * v v
   * 0|0 CULLMODE_CULL_CCW,
   * 0|1 CULLMODE_CULL_CW,
   * 1|0 CULLMODE_CULL_CW,
   * 1|1 CULLMODE_CULL_CCW,
   */
   switch (dynamic_state->rs.cull_mode) {
   case VK_CULL_MODE_BACK_BIT:
   case VK_CULL_MODE_FRONT_BIT:
      if ((dynamic_state->rs.front_face == VK_FRONT_FACE_COUNTER_CLOCKWISE) ^
      (dynamic_state->rs.cull_mode == VK_CULL_MODE_FRONT_BIT)) {
      } else {
                        case VK_CULL_MODE_FRONT_AND_BACK:
   case VK_CULL_MODE_NONE:
                  default:
                     if (ppp_control != ppp_state->ppp_control) {
      ppp_state->ppp_control = ppp_control;
         }
      /* Largest valid PPP State update in words = 31
   * 1 - Header
   * 3 - Stream Out Config words 0, 1 and 2
   * 1 - PPP Control word
   * 3 - Varying Config words 0, 1 and 2
   * 1 - Output Select
   * 1 - WClamp
   * 6 - Viewport Transform words
   * 2 - Region Clip words
   * 3 - PDS State for fragment phase (PDSSTATEPTR 1-3)
   * 4 - PDS State for fragment phase (PDSSTATEPTR0)
   * 6 - ISP Control Words
   */
   #define PVR_MAX_PPP_STATE_DWORDS 31
      static VkResult pvr_emit_ppp_state(struct pvr_cmd_buffer *const cmd_buffer,
         {
      const bool deferred_secondary = pvr_cmd_uses_deferred_cs_cmds(cmd_buffer);
   struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   struct PVRX(TA_STATE_HEADER) *const header = &state->emit_header;
   struct pvr_csb *const control_stream = &sub_cmd->control_stream;
   struct pvr_ppp_state *const ppp_state = &state->ppp_state;
   uint32_t ppp_state_words[PVR_MAX_PPP_STATE_DWORDS];
   const bool emit_dbsc = header->pres_ispctl_dbsc;
   uint32_t *buffer_ptr = ppp_state_words;
   uint32_t dbsc_patching_offset = 0;
   uint32_t ppp_state_words_count;
   struct pvr_suballoc_bo *pvr_bo;
         #if !defined(NDEBUG)
      struct PVRX(TA_STATE_HEADER) emit_mask = *header;
            static_assert(pvr_cmd_length(TA_STATE_HEADER) == 1,
         #   define EMIT_MASK_GET(field) (emit_mask.field)
   #   define EMIT_MASK_SET(field, value) (emit_mask.field = (value))
   #   define EMIT_MASK_IS_CLEAR                                        \
         (pvr_cmd_pack(TA_STATE_HEADER)(&packed_emit_mask, &emit_mask), \
   #else
   #   define EMIT_MASK_GET(field)
   #   define EMIT_MASK_SET(field, value)
   #endif
         header->view_port_count =
                  /* If deferred_secondary is true then we do a separate state update
   * which gets patched in vkCmdExecuteCommands().
   */
                     static_assert(pvr_cmd_length(TA_STATE_HEADER) == 1,
         /* If the header is empty we exit early and prevent a bo alloc of 0 size. */
   if (ppp_state_words[0] == 0)
            if (header->pres_ispctl) {
               assert(header->pres_ispctl_fa);
   /* This is not a mistake. FA, BA have the ISPA format, and FB, BB have the
   * ISPB format.
   */
   pvr_csb_write_value(buffer_ptr, TA_STATE_ISPA, ppp_state->isp.front_a);
            if (header->pres_ispctl_fb) {
      pvr_csb_write_value(buffer_ptr, TA_STATE_ISPB, ppp_state->isp.front_b);
               if (header->pres_ispctl_ba) {
      pvr_csb_write_value(buffer_ptr, TA_STATE_ISPA, ppp_state->isp.back_a);
               if (header->pres_ispctl_bb) {
      pvr_csb_write_value(buffer_ptr, TA_STATE_ISPB, ppp_state->isp.back_b);
                           if (header->pres_ispctl_dbsc) {
                        pvr_csb_pack (buffer_ptr, TA_STATE_ISPDBSC, ispdbsc) {
      ispdbsc.dbindex = ppp_state->depthbias_scissor_indices.depthbias_index;
      }
                        if (header->pres_pds_state_ptr0) {
      pvr_csb_write_value(buffer_ptr,
                  pvr_csb_write_value(buffer_ptr,
                  pvr_csb_write_value(buffer_ptr,
               pvr_csb_write_value(buffer_ptr,
                              if (header->pres_pds_state_ptr1) {
      pvr_csb_write_value(buffer_ptr,
                           /* We don't use pds_state_ptr2 (texture state programs) control word, but
   * this doesn't mean we need to set it to 0. This is because the hardware
   * runs the texture state program only when
   * ROGUE_TA_STATE_PDS_SIZEINFO1.pds_texturestatesize is non-zero.
   */
   assert(pvr_csb_unpack(&ppp_state->pds.size_info1, TA_STATE_PDS_SIZEINFO1)
            if (header->pres_pds_state_ptr3) {
      pvr_csb_write_value(buffer_ptr,
                           if (header->pres_region_clip) {
      pvr_csb_write_value(buffer_ptr,
               pvr_csb_write_value(buffer_ptr,
                              if (header->pres_viewport) {
      const uint32_t viewports = MAX2(1, ppp_state->viewport_count);
            for (uint32_t i = 0; i < viewports; i++) {
      /* These don't have any definitions in the csbgen xml files and none
   * will be added.
   */
   *buffer_ptr++ = ppp_state->viewports[i].a0;
   *buffer_ptr++ = ppp_state->viewports[i].m0;
   *buffer_ptr++ = ppp_state->viewports[i].a1;
   *buffer_ptr++ = ppp_state->viewports[i].m1;
                                          if (header->pres_wclamp) {
      pvr_csb_pack (buffer_ptr, TA_WCLAMP, wclamp) {
         }
   buffer_ptr += pvr_cmd_length(TA_WCLAMP);
               if (header->pres_outselects) {
      pvr_csb_write_value(buffer_ptr, TA_OUTPUT_SEL, ppp_state->output_selects);
               if (header->pres_varying_word0) {
      pvr_csb_write_value(buffer_ptr,
                           if (header->pres_varying_word1) {
      pvr_csb_write_value(buffer_ptr,
                           /* We only emit this on the first draw of a render job to prevent us from
   * inheriting a non-zero value set elsewhere.
   */
   if (header->pres_varying_word2) {
      pvr_csb_write_value(buffer_ptr, TA_STATE_VARYING2, 0);
               if (header->pres_ppp_ctrl) {
      pvr_csb_write_value(buffer_ptr,
                           /* We only emit this on the first draw of a render job to prevent us from
   * inheriting a non-zero value set elsewhere.
   */
   if (header->pres_stream_out_size) {
      pvr_csb_write_value(buffer_ptr, TA_STATE_STREAM_OUT0, 0);
                     #undef EMIT_MASK_GET
   #undef EMIT_MASK_SET
   #if !defined(NDEBUG)
   #   undef EMIT_MASK_IS_CLEAR
   #endif
         ppp_state_words_count = buffer_ptr - ppp_state_words;
            result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            memcpy(pvr_bo_suballoc_get_map_addr(pvr_bo),
                           /* Write the VDM state update into the VDM control stream. */
   pvr_csb_emit (control_stream, VDMCTRL_PPP_STATE0, state0) {
      state0.word_count = ppp_state_words_count;
               pvr_csb_emit (control_stream, VDMCTRL_PPP_STATE1, state1) {
                           if (emit_dbsc && cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
               if (deferred_secondary) {
      const uint32_t num_dwords = pvr_cmd_length(VDMCTRL_PPP_STATE0) +
                           vdm_state = pvr_csb_alloc_dwords(control_stream, num_dwords);
   if (!vdm_state) {
      result = pvr_csb_get_status(control_stream);
                        cmd = (struct pvr_deferred_cs_command){
      .type = PVR_DEFERRED_CS_COMMAND_TYPE_DBSC,
   .dbsc = {
      .state = ppp_state->depthbias_scissor_indices,
            } else {
      cmd = (struct pvr_deferred_cs_command){
      .type = PVR_DEFERRED_CS_COMMAND_TYPE_DBSC2,
   .dbsc2 = {
      .state = ppp_state->depthbias_scissor_indices,
   .ppp_cs_bo = pvr_bo,
                     util_dynarray_append(&cmd_buffer->deferred_csb_commands,
                                 }
      static inline bool
   pvr_ppp_state_update_required(const struct pvr_cmd_buffer *cmd_buffer)
   {
      const BITSET_WORD *const dynamic_dirty =
         const struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
            /* For push constants we only need to worry if they are updated for the
   * fragment stage since we're only updating the pds programs used in the
   * fragment stage.
            return header->pres_ppp_ctrl || header->pres_ispctl ||
         header->pres_ispctl_fb || header->pres_ispctl_ba ||
   header->pres_ispctl_bb || header->pres_ispctl_dbsc ||
   header->pres_pds_state_ptr0 || header->pres_pds_state_ptr1 ||
   header->pres_pds_state_ptr2 || header->pres_pds_state_ptr3 ||
   header->pres_region_clip || header->pres_viewport ||
   header->pres_wclamp || header->pres_outselects ||
   header->pres_varying_word0 || header->pres_varying_word1 ||
   header->pres_varying_word2 || header->pres_stream_out_program ||
   state->dirty.fragment_descriptors || state->dirty.vis_test ||
   state->dirty.gfx_pipeline_binding || state->dirty.isp_userpass ||
   state->push_constants.dirty_stages & VK_SHADER_STAGE_FRAGMENT_BIT ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_DS_STENCIL_COMPARE_MASK) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_DS_STENCIL_WRITE_MASK) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_DS_STENCIL_REFERENCE) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_ENABLE) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_RS_LINE_WIDTH) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_VP_SCISSORS) ||
   BITSET_TEST(dynamic_dirty, MESA_VK_DYNAMIC_VP_SCISSOR_COUNT) ||
      }
      static VkResult
   pvr_emit_dirty_ppp_state(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   struct vk_dynamic_graphics_state *const dynamic_state =
                  /* TODO: The emit_header will be dirty only if
   * pvr_reset_graphics_dirty_state() was called before this (so when command
   * buffer begins recording or when it's reset). Otherwise it will have been
   * zeroed out by the previous pvr_emit_ppp_state(). We can probably set a
   * flag in there and check it here instead of checking the header.
   * Check if this is true and implement the flag.
   */
   if (!pvr_ppp_state_update_required(cmd_buffer))
            if (state->dirty.gfx_pipeline_binding) {
               pvr_setup_output_select(cmd_buffer);
   pvr_setup_isp_faces_and_control(cmd_buffer, &ispa);
      } else if (BITSET_TEST(dynamic_state->dirty,
                  BITSET_TEST(dynamic_state->dirty,
         BITSET_TEST(dynamic_state->dirty,
         BITSET_TEST(dynamic_state->dirty,
                     if (!dynamic_state->rs.rasterizer_discard_enable &&
      state->dirty.fragment_descriptors &&
   state->gfx_pipeline->shader_state.fragment.bo) {
                        if (BITSET_TEST(dynamic_state->dirty, MESA_VK_DYNAMIC_VP_VIEWPORTS) ||
      BITSET_TEST(dynamic_state->dirty, MESA_VK_DYNAMIC_VP_VIEWPORT_COUNT))
                  /* The hardware doesn't have an explicit mode for this so we use a
   * negative viewport to make sure all objects are culled out early.
   */
   if (dynamic_state->rs.cull_mode == VK_CULL_MODE_FRONT_AND_BACK) {
      /* Shift the viewport out of the guard-band culling everything. */
            state->ppp_state.viewports[0].a0 = negative_vp_val;
   state->ppp_state.viewports[0].m0 = 0;
   state->ppp_state.viewports[0].a1 = negative_vp_val;
   state->ppp_state.viewports[0].m1 = 0;
   state->ppp_state.viewports[0].a2 = negative_vp_val;
                                 result = pvr_emit_ppp_state(cmd_buffer, sub_cmd);
   if (result != VK_SUCCESS)
               }
      void pvr_calculate_vertex_cam_size(const struct pvr_device_info *dev_info,
                           {
      /* First work out the size of a vertex in the UVS and multiply by 4 for
   * column ordering.
   */
   const uint32_t uvs_vertex_vector_size_in_dwords =
         const uint32_t vdm_cam_size =
            /* This is a proxy for 8XE. */
   if (PVR_HAS_FEATURE(dev_info, simple_internal_parameter_format) &&
      vdm_cam_size < 96U) {
   /* Comparisons are based on size including scratch per vertex vector. */
   if (uvs_vertex_vector_size_in_dwords < (14U * 4U)) {
      *cam_size_out = MIN2(31U, vdm_cam_size - 1U);
      } else if (uvs_vertex_vector_size_in_dwords < (20U * 4U)) {
      *cam_size_out = 15U;
      } else if (uvs_vertex_vector_size_in_dwords < (28U * 4U)) {
      *cam_size_out = 11U;
      } else if (uvs_vertex_vector_size_in_dwords < (44U * 4U)) {
      *cam_size_out = 7U;
      } else if (PVR_HAS_FEATURE(dev_info,
                  *cam_size_out = 7U;
      } else {
      *cam_size_out = 3U;
         } else {
      /* Comparisons are based on size including scratch per vertex vector. */
   if (uvs_vertex_vector_size_in_dwords <= (32U * 4U)) {
      /* output size <= 27 + 5 scratch. */
   *cam_size_out = MIN2(95U, vdm_cam_size - 1U);
      } else if (uvs_vertex_vector_size_in_dwords <= 48U * 4U) {
      /* output size <= 43 + 5 scratch */
   *cam_size_out = 63U;
   if (PVR_GET_FEATURE_VALUE(dev_info, uvs_vtx_entries, 144U) < 288U)
         else
      } else if (uvs_vertex_vector_size_in_dwords <= 64U * 4U) {
      /* output size <= 59 + 5 scratch. */
   *cam_size_out = 31U;
   if (PVR_GET_FEATURE_VALUE(dev_info, uvs_vtx_entries, 144U) < 288U)
         else
      } else {
      *cam_size_out = 15U;
            }
      static void pvr_emit_dirty_vdm_state(struct pvr_cmd_buffer *const cmd_buffer,
         {
      /* FIXME: Assume all state is dirty for the moment. */
   struct pvr_device_info *const dev_info =
         ASSERTED const uint32_t max_user_vertex_output_components =
         struct PVRX(VDMCTRL_VDM_STATE0)
         struct vk_dynamic_graphics_state *const dynamic_state =
         const struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   const struct pvr_vertex_shader_state *const vertex_shader_state =
         struct pvr_csb *const csb = &sub_cmd->control_stream;
   uint32_t vs_output_size;
   uint32_t max_instances;
            /* CAM Calculations and HW state take vertex size aligned to DWORDS. */
   vs_output_size =
      DIV_ROUND_UP(vertex_shader_state->vertex_output_size,
                  pvr_calculate_vertex_cam_size(dev_info,
                                       pvr_csb_emit (csb, VDMCTRL_VDM_STATE0, state0) {
               if (dynamic_state->ia.primitive_restart_enable) {
      state0.cut_index_enable = true;
               switch (dynamic_state->ia.primitive_topology) {
   case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
                  default:
      state0.flatshade_control = PVRX(VDMCTRL_FLATSHADE_CONTROL_VERTEX_0);
               /* If we've bound a different vertex buffer, or this draw-call requires
   * a different PDS attrib data-section from the last draw call (changed
   * base_instance) then we need to specify a new data section. This is
   * also the case if we've switched pipeline or attrib program as the
   * data-section layout will be different.
   */
   state0.vs_data_addr_present =
                  /* Need to specify new PDS Attrib program if we've bound a different
   * pipeline or we needed a different PDS Attrib variant for this
   * draw-call.
   */
   state0.vs_other_present = state->dirty.gfx_pipeline_binding ||
            /* UVB_SCRATCH_SELECT_ONE with no rasterization is only valid when
   * stream output is enabled. We use UVB_SCRATCH_SELECT_FIVE because
   * Vulkan doesn't support stream output and the vertex position is
   * always emitted to the UVB.
   */
   state0.uvs_scratch_size_select =
                        if (header.cut_index_present) {
      pvr_csb_emit (csb, VDMCTRL_VDM_STATE1, state1) {
      switch (state->index_buffer_binding.type) {
   case VK_INDEX_TYPE_UINT32:
      /* FIXME: Defines for these? These seem to come from the Vulkan
   * spec. for VkPipelineInputAssemblyStateCreateInfo
   * primitiveRestartEnable.
   */
               case VK_INDEX_TYPE_UINT16:
                  default:
                        if (header.vs_data_addr_present) {
      pvr_csb_emit (csb, VDMCTRL_VDM_STATE2, state2) {
      state2.vs_pds_data_base_addr =
                  if (header.vs_other_present) {
      const uint32_t usc_unified_store_size_in_bytes =
            pvr_csb_emit (csb, VDMCTRL_VDM_STATE3, state3) {
      state3.vs_pds_code_base_addr =
               pvr_csb_emit (csb, VDMCTRL_VDM_STATE4, state4) {
                  pvr_csb_emit (csb, VDMCTRL_VDM_STATE5, state5) {
      state5.vs_max_instances = max_instances;
   state5.vs_usc_common_size = 0U;
   state5.vs_usc_unified_size = DIV_ROUND_UP(
      usc_unified_store_size_in_bytes,
      state5.vs_pds_temp_size =
      DIV_ROUND_UP(state->pds_shader.info->temps_required << 2,
      state5.vs_pds_data_size = DIV_ROUND_UP(
      PVR_DW_TO_BYTES(state->pds_shader.info->data_size_in_dwords),
                  }
      static VkResult pvr_validate_draw_state(struct pvr_cmd_buffer *cmd_buffer)
   {
      struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   struct vk_dynamic_graphics_state *const dynamic_state =
         const struct pvr_graphics_pipeline *const gfx_pipeline = state->gfx_pipeline;
   const struct pvr_pipeline_stage_state *const fragment_state =
         const struct pvr_pipeline_stage_state *const vertex_state =
         const struct pvr_pipeline_layout *const pipeline_layout =
         struct pvr_sub_cmd_gfx *sub_cmd;
   bool fstencil_writemask_zero;
   bool bstencil_writemask_zero;
   bool fstencil_keep;
   bool bstencil_keep;
                     sub_cmd = &state->current_sub_cmd->gfx;
            /* Determine pipeline depth/stencil usage. If a pipeline uses depth or
   * stencil testing, those attachments are using their loaded values, and
   * the loadOps cannot be optimized out.
   */
   /* Pipeline uses depth testing. */
   if (sub_cmd->depth_usage == PVR_DEPTH_STENCIL_USAGE_UNDEFINED &&
      dynamic_state->ds.depth.compare_op != VK_COMPARE_OP_ALWAYS) {
               /* Pipeline uses stencil testing. */
   if (sub_cmd->stencil_usage == PVR_DEPTH_STENCIL_USAGE_UNDEFINED &&
      (dynamic_state->ds.stencil.front.op.compare != VK_COMPARE_OP_ALWAYS ||
   dynamic_state->ds.stencil.back.op.compare != VK_COMPARE_OP_ALWAYS)) {
               if (PVR_HAS_FEATURE(&cmd_buffer->device->pdevice->dev_info,
            uint32_t coefficient_size =
                  if (coefficient_size >
      PVRX(TA_STATE_PDS_SIZEINFO1_USC_VARYINGSIZE_MAX_SIZE))
            sub_cmd->frag_uses_atomic_ops |= fragment_state->uses_atomic_ops;
   sub_cmd->frag_has_side_effects |= fragment_state->has_side_effects;
   sub_cmd->frag_uses_texture_rw |= fragment_state->uses_texture_rw;
                     fstencil_keep =
      (dynamic_state->ds.stencil.front.op.fail == VK_STENCIL_OP_KEEP) &&
      bstencil_keep =
      (dynamic_state->ds.stencil.back.op.fail == VK_STENCIL_OP_KEEP) &&
      fstencil_writemask_zero = (dynamic_state->ds.stencil.front.write_mask == 0);
            /* Set stencil modified flag if:
   * - Neither front nor back-facing stencil has a fail_op/pass_op of KEEP.
   * - Neither front nor back-facing stencil has a write_mask of zero.
   */
   if (!(fstencil_keep && bstencil_keep) &&
      !(fstencil_writemask_zero && bstencil_writemask_zero)) {
               /* Set depth modified flag if depth write is enabled. */
   if (dynamic_state->ds.depth.write_enable)
            /* If either the data or code changes for pds vertex attribs, regenerate the
   * data segment.
   */
   if (state->dirty.vertex_bindings || state->dirty.gfx_pipeline_binding ||
      state->dirty.draw_variant || state->dirty.draw_base_instance) {
   enum pvr_pds_vertex_attrib_program_type prog_type;
            if (state->draw_state.draw_indirect)
         else if (state->draw_state.base_instance)
         else
            program =
         state->pds_shader.info = &program->info;
            state->max_shared_regs =
                        if (state->push_constants.dirty_stages & VK_SHADER_STAGE_ALL_GRAPHICS) {
      result = pvr_cmd_upload_push_consts(cmd_buffer);
   if (result != VK_SUCCESS)
               state->dirty.vertex_descriptors = state->dirty.gfx_pipeline_binding;
            /* Account for dirty descriptor set. */
   state->dirty.vertex_descriptors |=
      state->dirty.gfx_desc_dirty &&
   pipeline_layout
      state->dirty.fragment_descriptors |=
      state->dirty.gfx_desc_dirty &&
         if (BITSET_TEST(dynamic_state->dirty, MESA_VK_DYNAMIC_CB_BLEND_CONSTANTS))
            state->dirty.vertex_descriptors |=
      state->push_constants.dirty_stages &
      state->dirty.fragment_descriptors |= state->push_constants.dirty_stages &
            if (state->dirty.fragment_descriptors) {
      result = pvr_setup_descriptor_mappings(
      cmd_buffer,
   PVR_STAGE_ALLOCATION_FRAGMENT,
   &state->gfx_pipeline->shader_state.fragment.descriptor_state,
   NULL,
      if (result != VK_SUCCESS) {
      mesa_loge("Could not setup fragment descriptor mappings.");
                  if (state->dirty.vertex_descriptors) {
               result = pvr_setup_descriptor_mappings(
      cmd_buffer,
   PVR_STAGE_ALLOCATION_VERTEX_GEOMETRY,
   &state->gfx_pipeline->shader_state.vertex.descriptor_state,
   NULL,
      if (result != VK_SUCCESS) {
      mesa_loge("Could not setup vertex descriptor mappings.");
               pvr_emit_dirty_pds_state(cmd_buffer,
                     pvr_emit_dirty_ppp_state(cmd_buffer, sub_cmd);
            vk_dynamic_graphics_state_clear_dirty(dynamic_state);
   state->dirty.gfx_desc_dirty = false;
   state->dirty.draw_base_instance = false;
   state->dirty.draw_variant = false;
   state->dirty.fragment_descriptors = false;
   state->dirty.gfx_pipeline_binding = false;
   state->dirty.isp_userpass = false;
   state->dirty.vertex_bindings = false;
                        }
      static uint32_t pvr_get_hw_primitive_topology(VkPrimitiveTopology topology)
   {
      switch (topology) {
   case VK_PRIMITIVE_TOPOLOGY_POINT_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST:
         case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN:
         case VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY:
         case VK_PRIMITIVE_TOPOLOGY_PATCH_LIST:
         default:
            }
      /* TODO: Rewrite this in terms of ALIGN_POT() and pvr_cmd_length(). */
   /* Aligned to 128 bit for PDS loads / stores */
   #define DUMMY_VDM_CONTROL_STREAM_BLOCK_SIZE 8
      static VkResult
   pvr_write_draw_indirect_vdm_stream(struct pvr_cmd_buffer *cmd_buffer,
                                    struct pvr_csb *const csb,
   pvr_dev_addr_t idx_buffer_addr,
      {
      struct pvr_pds_drawindirect_program pds_prog = { 0 };
            /* Draw indirect always has index offset and instance count. */
   list_hdr->index_offset_present = true;
                     pds_prog.support_base_instance = true;
   pds_prog.arg_buffer = buffer->dev_addr.addr + offset;
   pds_prog.index_buffer = idx_buffer_addr.addr;
   pds_prog.index_block_header = word0;
   pds_prog.index_stride = idx_stride;
            /* TODO: See if we can pre-upload the code section of all the pds programs
   * and reuse them here.
   */
   /* Generate and upload the PDS programs (code + data). */
   for (uint32_t i = 0U; i < count; i++) {
      const struct pvr_device_info *dev_info =
         struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_suballoc_bo *dummy_bo;
   struct pvr_suballoc_bo *pds_bo;
   uint32_t *dummy_stream;
   uint32_t *pds_base;
   uint32_t pds_size;
            /* TODO: Move this outside the loop and allocate all of them in one go? */
   result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            pds_prog.increment_draw_id = (i != 0);
            if (state->draw_state.draw_indexed) {
      pvr_pds_generate_draw_elements_indirect(&pds_prog,
                  } else {
      pvr_pds_generate_draw_arrays_indirect(&pds_prog,
                           pds_size = PVR_DW_TO_BYTES(pds_prog.program.data_size_aligned +
            result = pvr_cmd_buffer_alloc_mem(cmd_buffer,
                     if (result != VK_SUCCESS)
            pds_base = pvr_bo_suballoc_get_map_addr(pds_bo);
   memcpy(pds_base,
                  if (state->draw_state.draw_indexed) {
      pvr_pds_generate_draw_elements_indirect(
      &pds_prog,
   pds_base + pds_prog.program.code_size_aligned,
   PDS_GENERATE_DATA_SEGMENT,
   } else {
      pvr_pds_generate_draw_arrays_indirect(
      &pds_prog,
   pds_base + pds_prog.program.code_size_aligned,
   PDS_GENERATE_DATA_SEGMENT,
                     pvr_csb_emit (csb, VDMCTRL_PDS_STATE0, state0) {
               state0.pds_temp_size =
                  state0.pds_data_size =
      DIV_ROUND_UP(PVR_DW_TO_BYTES(pds_prog.program.data_size_aligned),
            pvr_csb_emit (csb, VDMCTRL_PDS_STATE1, state1) {
      const uint32_t data_offset =
      pds_bo->dev_addr.addr +
               state1.pds_data_addr = PVR_DEV_ADDR(data_offset);
   state1.sd_type = PVRX(VDMCTRL_SD_TYPE_PDS);
               pvr_csb_emit (csb, VDMCTRL_PDS_STATE2, state2) {
      const uint32_t code_offset =
                                       /* We don't really need to set the relocation mark since the following
   * state update is just one emit but let's be nice and use it.
   */
            /* Sync task to ensure the VDM doesn't start reading the dummy blocks
   * before they are ready.
   */
   pvr_csb_emit (csb, VDMCTRL_INDEX_LIST0, list0) {
                                    /* For indexed draw cmds fill in the dummy's header (as it won't change
   * based on the indirect args) and increment by the in-use size of each
   * dummy block.
   */
   if (!state->draw_state.draw_indexed) {
      dummy_stream[0] = word0;
      } else {
                  /* clang-format off */
   pvr_csb_pack (dummy_stream, VDMCTRL_STREAM_RETURN, word);
                     /* Stream link to the first dummy which forces the VDM to discard any
   * prefetched (dummy) control stream.
   */
   pvr_csb_emit (csb, VDMCTRL_STREAM_LINK0, link) {
      link.with_return = true;
               pvr_csb_emit (csb, VDMCTRL_STREAM_LINK1, link) {
                           /* Point the pds program to the next argument buffer and the next VDM
   * dummy buffer.
   */
                  }
      #undef DUMMY_VDM_CONTROL_STREAM_BLOCK_SIZE
      static void pvr_emit_vdm_index_list(struct pvr_cmd_buffer *cmd_buffer,
                                       struct pvr_sub_cmd_gfx *const sub_cmd,
   VkPrimitiveTopology topology,
   uint32_t index_offset,
   uint32_t first_index,
   {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   const bool vertex_shader_has_side_effects =
         struct PVRX(VDMCTRL_INDEX_LIST0)
         pvr_dev_addr_t index_buffer_addr = PVR_DEV_ADDR_INVALID;
   struct pvr_csb *const csb = &sub_cmd->control_stream;
                     /* firstInstance is not handled here in the VDM state, it's implemented as
   * an addition in the PDS vertex fetch using
   * PVR_PDS_CONST_MAP_ENTRY_TYPE_BASE_INSTANCE entry type.
                     if (instance_count > 1)
            if (index_offset)
            if (state->draw_state.draw_indexed) {
      switch (state->index_buffer_binding.type) {
   case VK_INDEX_TYPE_UINT32:
      list_hdr.index_size = PVRX(VDMCTRL_INDEX_SIZE_B32);
               case VK_INDEX_TYPE_UINT16:
      list_hdr.index_size = PVRX(VDMCTRL_INDEX_SIZE_B16);
               default:
                  index_buffer_addr = PVR_DEV_ADDR_OFFSET(
                  list_hdr.index_addr_present = true;
               list_hdr.degen_cull_enable =
      PVR_HAS_FEATURE(&cmd_buffer->device->pdevice->dev_info,
               if (state->draw_state.draw_indirect) {
      assert(buffer);
   pvr_write_draw_indirect_vdm_stream(cmd_buffer,
                                    csb,
   index_buffer_addr,
                           pvr_csb_emit (csb, VDMCTRL_INDEX_LIST0, list0) {
                  if (list_hdr.index_addr_present) {
      pvr_csb_emit (csb, VDMCTRL_INDEX_LIST1, list1) {
                     if (list_hdr.index_count_present) {
      pvr_csb_emit (csb, VDMCTRL_INDEX_LIST2, list2) {
                     if (list_hdr.index_instance_count_present) {
      pvr_csb_emit (csb, VDMCTRL_INDEX_LIST3, list3) {
                     if (list_hdr.index_offset_present) {
      pvr_csb_emit (csb, VDMCTRL_INDEX_LIST4, list4) {
                        }
      void pvr_CmdDraw(VkCommandBuffer commandBuffer,
                  uint32_t vertexCount,
      {
      const struct pvr_cmd_buffer_draw_state draw_state = {
      .base_vertex = firstVertex,
               PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct vk_dynamic_graphics_state *const dynamic_state =
         struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
                              result = pvr_validate_draw_state(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Write the VDM control stream for the primitive. */
   pvr_emit_vdm_index_list(cmd_buffer,
                           &state->current_sub_cmd->gfx,
   dynamic_state->ia.primitive_topology,
   firstVertex,
   0U,
   vertexCount,
      }
      void pvr_CmdDrawIndexed(VkCommandBuffer commandBuffer,
                           uint32_t indexCount,
   {
      const struct pvr_cmd_buffer_draw_state draw_state = {
      .base_vertex = vertexOffset,
   .base_instance = firstInstance,
               PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct vk_dynamic_graphics_state *const dynamic_state =
         struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
                              result = pvr_validate_draw_state(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Write the VDM control stream for the primitive. */
   pvr_emit_vdm_index_list(cmd_buffer,
                           &state->current_sub_cmd->gfx,
   dynamic_state->ia.primitive_topology,
   vertexOffset,
   firstIndex,
   indexCount,
      }
      void pvr_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer,
                           {
      const struct pvr_cmd_buffer_draw_state draw_state = {
      .draw_indirect = true,
               PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct vk_dynamic_graphics_state *const dynamic_state =
         PVR_FROM_HANDLE(pvr_buffer, buffer, _buffer);
                              result = pvr_validate_draw_state(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Write the VDM control stream for the primitive. */
   pvr_emit_vdm_index_list(cmd_buffer,
                           &state->current_sub_cmd->gfx,
   dynamic_state->ia.primitive_topology,
   0U,
   0U,
   0U,
      }
      void pvr_CmdDrawIndirect(VkCommandBuffer commandBuffer,
                           {
      const struct pvr_cmd_buffer_draw_state draw_state = {
                  PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   PVR_FROM_HANDLE(pvr_buffer, buffer, _buffer);
   struct vk_dynamic_graphics_state *const dynamic_state =
                                    result = pvr_validate_draw_state(cmd_buffer);
   if (result != VK_SUCCESS)
            /* Write the VDM control stream for the primitive. */
   pvr_emit_vdm_index_list(cmd_buffer,
                           &state->current_sub_cmd->gfx,
   dynamic_state->ia.primitive_topology,
   0U,
   0U,
   0U,
      }
      static VkResult
   pvr_resolve_unemitted_resolve_attachments(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   const struct pvr_renderpass_hwsetup_render *hw_render =
            for (uint32_t i = 0U; i < hw_render->eot_surface_count; i++) {
      const struct pvr_renderpass_hwsetup_eot_surface *surface =
         const uint32_t color_attach_idx = surface->src_attachment_idx;
   const uint32_t resolve_attach_idx = surface->attachment_idx;
   VkImageSubresourceLayers src_subresource;
   VkImageSubresourceLayers dst_subresource;
   struct pvr_image_view *dst_view;
   struct pvr_image_view *src_view;
   VkFormat src_format;
   VkFormat dst_format;
   VkImageCopy2 region;
            if (!surface->need_resolve ||
                  dst_view = info->attachments[resolve_attach_idx];
            src_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   src_subresource.mipLevel = src_view->vk.base_mip_level;
   src_subresource.baseArrayLayer = src_view->vk.base_array_layer;
            dst_subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
   dst_subresource.mipLevel = dst_view->vk.base_mip_level;
   dst_subresource.baseArrayLayer = dst_view->vk.base_array_layer;
            region.srcOffset = (VkOffset3D){ info->render_area.offset.x,
               region.dstOffset = (VkOffset3D){ info->render_area.offset.x,
               region.extent = (VkExtent3D){ info->render_area.extent.width,
                  region.srcSubresource = src_subresource;
            /* TODO: if ERN_46863 is supported, Depth and stencil are sampled
   * separately from images with combined depth+stencil. Add logic here to
   * handle it using appropriate format from image view.
   */
   src_format = src_view->vk.image->format;
   dst_format = dst_view->vk.image->format;
   src_view->vk.image->format = src_view->vk.format;
            result = pvr_copy_or_resolve_color_image_region(
      cmd_buffer,
   vk_to_pvr_image(src_view->vk.image),
               src_view->vk.image->format = src_format;
                     if (result != VK_SUCCESS)
                  }
      void pvr_CmdEndRenderPass2(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_image_view **attachments;
   VkClearValue *clear_values;
                     assert(state->render_pass_info.pass);
            result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
            result = pvr_resolve_unemitted_resolve_attachments(cmd_buffer,
         if (result != VK_SUCCESS)
            /* Save the required fields before clearing render_pass_info struct. */
   attachments = state->render_pass_info.attachments;
                     state->render_pass_info.attachments = attachments;
      }
      static VkResult
   pvr_execute_deferred_cmd_buffer(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct vk_dynamic_graphics_state *const dynamic_state =
         const uint32_t prim_db_elems =
      util_dynarray_num_elements(&cmd_buffer->depth_bias_array,
      const uint32_t prim_scissor_elems =
      util_dynarray_num_elements(&cmd_buffer->scissor_array,
         util_dynarray_foreach (&sec_cmd_buffer->deferred_csb_commands,
                  switch (cmd->type) {
   case PVR_DEFERRED_CS_COMMAND_TYPE_DBSC: {
      const uint32_t scissor_idx =
         const uint32_t db_idx =
         const uint32_t num_dwords =
         struct pvr_suballoc_bo *suballoc_bo;
                  pvr_csb_pack (&ppp_state[0], TA_STATE_HEADER, header) {
                  pvr_csb_pack (&ppp_state[1], TA_STATE_ISPDBSC, ispdbsc) {
      ispdbsc.dbindex = db_idx;
               result = pvr_cmd_buffer_upload_general(cmd_buffer,
                                    pvr_csb_pack (&cmd->dbsc.vdm_state[0], VDMCTRL_PPP_STATE0, state) {
      state.word_count = num_dwords;
               pvr_csb_pack (&cmd->dbsc.vdm_state[1], VDMCTRL_PPP_STATE1, state) {
                              case PVR_DEFERRED_CS_COMMAND_TYPE_DBSC2: {
      const uint32_t scissor_idx =
                        uint32_t *const addr =
                           pvr_csb_pack (addr, TA_STATE_ISPDBSC, ispdbsc) {
      ispdbsc.dbindex = db_idx;
                           default:
      unreachable("Invalid deferred control stream command type.");
                  util_dynarray_append_dynarray(&cmd_buffer->depth_bias_array,
            util_dynarray_append_dynarray(&cmd_buffer->scissor_array,
            BITSET_SET(dynamic_state->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS);
               }
      /* Caller needs to make sure that it ends the current sub_cmd. This function
   * only creates a copy of sec_sub_cmd and links it to the cmd_buffer's
   * sub_cmd list.
   */
   static VkResult pvr_execute_sub_cmd(struct pvr_cmd_buffer *cmd_buffer,
         {
      struct pvr_sub_cmd *primary_sub_cmd =
      vk_zalloc(&cmd_buffer->vk.pool->alloc,
            sizeof(*primary_sub_cmd),
   if (!primary_sub_cmd) {
      return vk_command_buffer_set_error(&cmd_buffer->vk,
               primary_sub_cmd->type = sec_sub_cmd->type;
                     switch (sec_sub_cmd->type) {
   case PVR_SUB_CMD_TYPE_GRAPHICS:
      primary_sub_cmd->gfx = sec_sub_cmd->gfx;
         case PVR_SUB_CMD_TYPE_OCCLUSION_QUERY:
   case PVR_SUB_CMD_TYPE_COMPUTE:
      primary_sub_cmd->compute = sec_sub_cmd->compute;
         case PVR_SUB_CMD_TYPE_TRANSFER:
      primary_sub_cmd->transfer = sec_sub_cmd->transfer;
         case PVR_SUB_CMD_TYPE_EVENT:
      primary_sub_cmd->event = sec_sub_cmd->event;
         default:
                     }
      static VkResult
   pvr_execute_graphics_cmd_buffer(struct pvr_cmd_buffer *cmd_buffer,
         {
      const struct pvr_device_info *dev_info =
         struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_sub_cmd *primary_sub_cmd = state->current_sub_cmd;
   struct pvr_sub_cmd *first_sec_cmd;
            /* Inherited queries are not supported. */
            if (list_is_empty(&sec_cmd_buffer->sub_cmds))
            first_sec_cmd =
            /* Kick a render if we have a new base address. */
   if (primary_sub_cmd->gfx.query_pool && first_sec_cmd->gfx.query_pool &&
      primary_sub_cmd->gfx.query_pool != first_sec_cmd->gfx.query_pool) {
            result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
            result =
         if (result != VK_SUCCESS)
                     /* Use existing render setup, but load color attachments from HW
   * Background object.
   */
   primary_sub_cmd->gfx.barrier_load = true;
               list_for_each_entry (struct pvr_sub_cmd,
                        /* Only graphics secondary execution supported within a renderpass. */
            if (!sec_sub_cmd->gfx.empty_cmd)
            if (sec_sub_cmd->gfx.query_pool) {
               util_dynarray_append_dynarray(&state->query_indices,
               if (pvr_cmd_uses_deferred_cs_cmds(sec_cmd_buffer)) {
      /* TODO: In case if secondary buffer is created with
   * VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, then we patch the
   * stream and copy it to primary stream using pvr_csb_copy below.
   * This will need locking if the same secondary command buffer is
   * executed in multiple primary buffers at the same time.
   */
   result = pvr_execute_deferred_cmd_buffer(cmd_buffer, sec_cmd_buffer);
                  result = pvr_csb_copy(&primary_sub_cmd->gfx.control_stream,
         if (result != VK_SUCCESS)
      } else {
      result = pvr_execute_deferred_cmd_buffer(cmd_buffer, sec_cmd_buffer);
                  pvr_csb_emit_link(
      &primary_sub_cmd->gfx.control_stream,
   pvr_csb_get_start_address(&sec_sub_cmd->gfx.control_stream),
            if (PVR_HAS_FEATURE(&cmd_buffer->device->pdevice->dev_info,
            primary_sub_cmd->gfx.job.disable_compute_overlap |=
               primary_sub_cmd->gfx.max_tiles_in_flight =
                  /* Pass loaded depth/stencil usage from secondary command buffer. */
   if (sec_sub_cmd->gfx.depth_usage == PVR_DEPTH_STENCIL_USAGE_NEEDED)
            if (sec_sub_cmd->gfx.stencil_usage == PVR_DEPTH_STENCIL_USAGE_NEEDED)
            /* Pass depth/stencil modification state from secondary command buffer. */
   if (sec_sub_cmd->gfx.modifies_depth)
            if (sec_sub_cmd->gfx.modifies_stencil)
            if (sec_sub_cmd->gfx.barrier_store) {
                     /* This shouldn't be the last sub cmd. There should be a barrier load
   * subsequent to the barrier store.
   */
   assert(list_last_entry(&sec_cmd_buffer->sub_cmds,
                  /* Kick render to store stencil. */
                  result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
                  result =
                                 /* Use existing render setup, but load color attachments from HW
   * Background object.
   */
   primary_sub_cmd->gfx.barrier_load = sec_next->gfx.barrier_load;
   primary_sub_cmd->gfx.barrier_store = sec_next->gfx.barrier_store;
               if (!PVR_HAS_FEATURE(dev_info, gs_rta_support)) {
      util_dynarray_append_dynarray(&cmd_buffer->deferred_clears,
                     }
      void pvr_CmdExecuteCommands(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_cmd_buffer *last_cmd_buffer;
                              /* Reset the CPU copy of the most recent PPP state of the primary command
   * buffer.
   *
   * The next draw call in the primary after CmdExecuteCommands may send
   * redundant state, if it all goes in the same geom job.
   *
   * Can't just copy state from the secondary because the recording state of
   * the secondary command buffers would have been deleted at this point.
   */
            if (state->current_sub_cmd &&
      state->current_sub_cmd->type == PVR_SUB_CMD_TYPE_GRAPHICS) {
   for (uint32_t i = 0; i < commandBufferCount; i++) {
                        result = pvr_execute_graphics_cmd_buffer(cmd_buffer, sec_cmd_buffer);
   if (result != VK_SUCCESS)
               last_cmd_buffer =
            /* Set barriers from final command secondary command buffer. */
   for (uint32_t i = 0; i != PVR_NUM_SYNC_PIPELINE_STAGES; i++) {
      state->barriers_needed[i] |=
      last_cmd_buffer->state.barriers_needed[i] &
      } else {
      for (uint32_t i = 0; i < commandBufferCount; i++) {
                        result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
                  list_for_each_entry_safe (struct pvr_sub_cmd,
                        result = pvr_execute_sub_cmd(cmd_buffer, sec_sub_cmd);
   if (result != VK_SUCCESS)
                  last_cmd_buffer =
            memcpy(state->barriers_needed,
               }
      static void pvr_insert_transparent_obj(struct pvr_cmd_buffer *const cmd_buffer,
         {
      struct pvr_device *const device = cmd_buffer->device;
   /* Yes we want a copy. The user could be recording multiple command buffers
   * in parallel so writing the template in place could cause problems.
   */
   struct pvr_static_clear_ppp_template clear =
         uint32_t pds_state[PVR_STATIC_CLEAR_PDS_STATE_COUNT] = { 0 };
   struct pvr_csb *csb = &sub_cmd->control_stream;
                              pvr_csb_pack (&pds_state[PVR_STATIC_CLEAR_PPP_PDS_TYPE_SHADERBASE],
                                                         pvr_emit_ppp_from_template(csb, &clear, &ppp_bo);
                              /* Reset graphics state. */
      }
      static inline struct pvr_render_subpass *
   pvr_get_current_subpass(const struct pvr_cmd_buffer_state *const state)
   {
                  }
      void pvr_CmdNextSubpass2(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
   struct pvr_render_pass_info *rp_info = &state->render_pass_info;
   const struct pvr_renderpass_hwsetup_subpass *hw_subpass;
   struct pvr_renderpass_hwsetup_render *next_hw_render;
   const struct pvr_render_pass *pass = rp_info->pass;
   const struct pvr_renderpass_hw_map *current_map;
   const struct pvr_renderpass_hw_map *next_map;
   struct pvr_load_op *hw_subpass_load_op;
                     current_map = &pass->hw_setup->subpass_map[rp_info->subpass_idx];
   next_map = &pass->hw_setup->subpass_map[rp_info->subpass_idx + 1];
            if (current_map->render != next_map->render) {
      result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
            result = pvr_resolve_unemitted_resolve_attachments(cmd_buffer, rp_info);
   if (result != VK_SUCCESS)
                     result =
         if (result != VK_SUCCESS)
            rp_info->enable_bg_tag = false;
            /* If this subpass contains any load ops the HW Background Object must be
   * run to do the clears/loads.
   */
   if (next_hw_render->color_init_count > 0) {
               for (uint32_t i = 0; i < next_hw_render->color_init_count; i++) {
      /* Empty tiles need to be cleared too. */
   if (next_hw_render->color_init[i].op ==
      VK_ATTACHMENT_LOAD_OP_CLEAR) {
   rp_info->process_empty_tiles = true;
                     /* Set isp_userpass to zero for new hw_render. This will be used to set
   * ROGUE_CR_ISP_CTL::upass_start.
   */
               hw_subpass = &next_hw_render->subpasses[next_map->subpass];
            if (hw_subpass_load_op) {
      result = pvr_cs_write_load_op(cmd_buffer,
                           /* Pipelines are created for a particular subpass so unbind but leave the
   * vertex and descriptor bindings intact as they are orthogonal to the
   * subpass.
   */
            /* User-pass spawn is 4 bits so if the driver has to wrap it, it will emit a
   * full screen transparent object to flush all tags up until now, then the
   * user-pass spawn value will implicitly be reset to 0 because
   * pvr_render_subpass::isp_userpass values are stored ANDed with
   * ROGUE_CR_ISP_CTL_UPASS_START_SIZE_MAX.
   */
   /* If hw_subpass_load_op is valid then pvr_write_load_op_control_stream
   * has already done a full-screen transparent object.
   */
   if (rp_info->isp_userpass == PVRX(CR_ISP_CTL_UPASS_START_SIZE_MAX) &&
      !hw_subpass_load_op) {
                        rp_info->isp_userpass = pass->subpasses[rp_info->subpass_idx].isp_userpass;
            rp_info->pipeline_bind_point =
               }
      static bool
   pvr_stencil_has_self_dependency(const struct pvr_cmd_buffer_state *const state)
   {
      const struct pvr_render_subpass *const current_subpass =
                  if (current_subpass->depth_stencil_attachment == VK_ATTACHMENT_UNUSED)
            /* We only need to check the current software subpass as we don't support
   * merging to/from a subpass with self-dep stencil.
            for (uint32_t i = 0; i < current_subpass->input_count; i++) {
      if (input_attachments[i] == current_subpass->depth_stencil_attachment)
                  }
      static bool pvr_is_stencil_store_load_needed(
      const struct pvr_cmd_buffer *const cmd_buffer,
   VkPipelineStageFlags2 vk_src_stage_mask,
   VkPipelineStageFlags2 vk_dst_stage_mask,
   uint32_t memory_barrier_count,
   const VkMemoryBarrier2 *const memory_barriers,
   uint32_t image_barrier_count,
      {
      const struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   const uint32_t fragment_test_stages =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
      const struct pvr_render_pass *const pass = state->render_pass_info.pass;
   const struct pvr_renderpass_hwsetup_render *hw_render;
   struct pvr_image_view **const attachments =
         const struct pvr_image_view *attachment;
            if (!pass)
            hw_render_idx = state->current_sub_cmd->gfx.hw_render_idx;
            if (hw_render->ds_attach_idx == VK_ATTACHMENT_UNUSED)
            if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
         } else {
      assert(!attachments);
               if (!(vk_src_stage_mask & fragment_test_stages) &&
      vk_dst_stage_mask & VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT)
         for (uint32_t i = 0; i < memory_barrier_count; i++) {
      const uint32_t stencil_write_bit =
         const uint32_t input_attachment_read_bit =
            if (!(memory_barriers[i].srcAccessMask & stencil_write_bit))
            if (!(memory_barriers[i].dstAccessMask & input_attachment_read_bit))
                        for (uint32_t i = 0; i < image_barrier_count; i++) {
      PVR_FROM_HANDLE(pvr_image, image, image_barriers[i].image);
            if (!(image_barriers[i].subresourceRange.aspectMask & stencil_bit))
            if (attachment && image != vk_to_pvr_image(attachment->vk.image))
            if (!vk_format_has_stencil(image->vk.format))
                           }
      static VkResult
   pvr_cmd_buffer_insert_mid_frag_barrier_event(struct pvr_cmd_buffer *cmd_buffer,
               {
                                 /* Submit graphics job to store stencil. */
            pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_BARRIER,
   .barrier = {
      .in_render_pass = true,
   .wait_for_stage_mask = src_stage_mask,
                  pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
            /* Use existing render setup, but load color attachments from HW BGOBJ */
   cmd_buffer->state.current_sub_cmd->gfx.barrier_load = true;
               }
      static VkResult
   pvr_cmd_buffer_insert_barrier_event(struct pvr_cmd_buffer *cmd_buffer,
               {
               result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_BARRIER,
   .barrier = {
      .wait_for_stage_mask = src_stage_mask,
                     }
      /* This is just enough to handle vkCmdPipelineBarrier().
   * TODO: Complete?
   */
   void pvr_CmdPipelineBarrier2(VkCommandBuffer commandBuffer,
         {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *const state = &cmd_buffer->state;
   const struct pvr_render_pass *const render_pass =
         VkPipelineStageFlags vk_src_stage_mask = 0U;
   VkPipelineStageFlags vk_dst_stage_mask = 0U;
   bool is_stencil_store_load_needed;
   uint32_t required_stage_mask = 0U;
   uint32_t src_stage_mask;
   uint32_t dst_stage_mask;
                     for (uint32_t i = 0; i < pDependencyInfo->memoryBarrierCount; i++) {
      vk_src_stage_mask |= pDependencyInfo->pMemoryBarriers[i].srcStageMask;
               for (uint32_t i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++) {
      vk_src_stage_mask |=
         vk_dst_stage_mask |=
               for (uint32_t i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++) {
      vk_src_stage_mask |=
         vk_dst_stage_mask |=
               src_stage_mask = pvr_stage_mask_src(vk_src_stage_mask);
            for (uint32_t stage = 0U; stage != PVR_NUM_SYNC_PIPELINE_STAGES; stage++) {
      if (!(dst_stage_mask & BITFIELD_BIT(stage)))
                        src_stage_mask &= required_stage_mask;
   for (uint32_t stage = 0U; stage != PVR_NUM_SYNC_PIPELINE_STAGES; stage++) {
      if (!(dst_stage_mask & BITFIELD_BIT(stage)))
                        if (src_stage_mask == 0 || dst_stage_mask == 0) {
         } else if (src_stage_mask == PVR_PIPELINE_STAGE_GEOM_BIT &&
            /* This is implicit so no need to barrier. */
      } else if (src_stage_mask == dst_stage_mask &&
                     switch (src_stage_mask) {
   case PVR_PIPELINE_STAGE_FRAG_BIT:
                                       /* Flush all fragment work up to this point. */
               case PVR_PIPELINE_STAGE_COMPUTE_BIT:
               if (!current_sub_cmd ||
      current_sub_cmd->type != PVR_SUB_CMD_TYPE_COMPUTE) {
               /* Multiple dispatches can be merged into a single job. When back to
   * back dispatches have a sequential dependency (Compute -> compute
   * pipeline barrier) we need to do the following.
   *   - Dispatch a kernel which fences all previous memory writes and
   *     flushes the MADD cache.
   *   - Issue a compute fence which ensures all previous tasks emitted
   *     by the compute data master are completed before starting
                  /* Issue Data Fence, Wait for Data Fence (IDFWDF) makes the PDS wait
   * for data.
                  pvr_compute_generate_fence(cmd_buffer,
                     default:
      is_barrier_needed = false;
         } else {
                  is_stencil_store_load_needed =
      pvr_is_stencil_store_load_needed(cmd_buffer,
                                       if (is_stencil_store_load_needed) {
               result = pvr_cmd_buffer_insert_mid_frag_barrier_event(cmd_buffer,
               if (result != VK_SUCCESS)
      } else {
      if (is_barrier_needed) {
               result = pvr_cmd_buffer_insert_barrier_event(cmd_buffer,
               if (result != VK_SUCCESS)
            }
      void pvr_CmdResetEvent2(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_event, event, _event);
                     result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_RESET,
   .set_reset = {
      .event = event,
                     }
      void pvr_CmdSetEvent2(VkCommandBuffer commandBuffer,
               {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   PVR_FROM_HANDLE(pvr_event, event, _event);
   VkPipelineStageFlags2 stage_mask = 0;
                     result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS)
            for (uint32_t i = 0; i < pDependencyInfo->memoryBarrierCount; i++)
            for (uint32_t i = 0; i < pDependencyInfo->bufferMemoryBarrierCount; i++)
            for (uint32_t i = 0; i < pDependencyInfo->imageMemoryBarrierCount; i++)
            cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_SET,
   .set_reset = {
      .event = event,
                     }
      void pvr_CmdWaitEvents2(VkCommandBuffer commandBuffer,
                     {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_event **events_array;
   uint32_t *stage_masks;
                     VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &events_array, __typeof__(*events_array), eventCount);
            if (!vk_multialloc_alloc(&ma,
                  vk_command_buffer_set_error(&cmd_buffer->vk, VK_ERROR_OUT_OF_HOST_MEMORY);
               result = pvr_cmd_buffer_start_sub_cmd(cmd_buffer, PVR_SUB_CMD_TYPE_EVENT);
   if (result != VK_SUCCESS) {
      vk_free(&cmd_buffer->vk.pool->alloc, events_array);
                        for (uint32_t i = 0; i < eventCount; i++) {
      const VkDependencyInfo *info = &pDependencyInfos[i];
            for (uint32_t j = 0; j < info->memoryBarrierCount; j++)
            for (uint32_t j = 0; j < info->bufferMemoryBarrierCount; j++)
            for (uint32_t j = 0; j < info->imageMemoryBarrierCount; j++)
                        cmd_buffer->state.current_sub_cmd->event = (struct pvr_sub_cmd_event){
      .type = PVR_EVENT_TYPE_WAIT,
   .wait = {
      .count = eventCount,
   .events = events_array,
                     }
      void pvr_CmdWriteTimestamp2KHR(VkCommandBuffer commandBuffer,
                     {
         }
      VkResult pvr_EndCommandBuffer(VkCommandBuffer commandBuffer)
   {
      PVR_FROM_HANDLE(pvr_cmd_buffer, cmd_buffer, commandBuffer);
   struct pvr_cmd_buffer_state *state = &cmd_buffer->state;
            if (vk_command_buffer_has_error(&cmd_buffer->vk))
            /* TODO: We should be freeing all the resources, allocated for recording,
   * here.
   */
            result = pvr_cmd_buffer_end_sub_cmd(cmd_buffer);
   if (result != VK_SUCCESS)
               }
