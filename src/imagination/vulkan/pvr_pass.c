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
      #include <stdbool.h>
   #include <stdint.h>
      #include "hwdef/rogue_hw_utils.h"
   #include "pvr_bo.h"
   #include "pvr_device_info.h"
   #include "pvr_formats.h"
   #include "pvr_hw_pass.h"
   #include "pvr_pds.h"
   #include "pvr_private.h"
   #include "pvr_types.h"
   #include "pvr_usc_fragment_shader.h"
   #include "util/macros.h"
   #include "rogue/rogue.h"
   #include "vk_alloc.h"
   #include "vk_format.h"
   #include "vk_log.h"
   #include "vk_render_pass.h"
      /*****************************************************************************
   PDS pre-baked program generation parameters and variables.
   *****************************************************************************/
   /* These would normally be produced by the compiler or other code. We're using
   * them for now just to speed up things. All of these should eventually be
   * removed.
   */
      static const struct {
      /* Indicates the amount of temporaries for the shader. */
   uint32_t temp_count;
   enum rogue_msaa_mode msaa_mode;
   /* Indicates the presence of PHAS instruction. */
      } pvr_pds_fragment_program_params = {
      .temp_count = 0,
   .msaa_mode = ROGUE_MSAA_MODE_PIXEL,
      };
      static inline bool pvr_subpass_has_msaa_input_attachment(
      struct pvr_render_subpass *subpass,
      {
      for (uint32_t i = 0; i < subpass->input_count; i++) {
               if (pCreateInfo->pAttachments[attachment].samples > 1)
                  }
      static bool pvr_is_subpass_initops_flush_needed(
      const struct pvr_render_pass *pass,
      {
      struct pvr_render_subpass *subpass = &pass->subpasses[0];
   uint32_t render_loadop_mask = 0;
            for (uint32_t i = 0; i < hw_render->color_init_count; i++) {
      if (hw_render->color_init[i].op != VK_ATTACHMENT_LOAD_OP_DONT_CARE)
               /* If there are no load ops then there's nothing to flush. */
   if (render_loadop_mask == 0)
            /* If the first subpass has any input attachments, they need to be
   * initialized with the result of the load op. Since the input attachment
   * may be read from fragments with an opaque pass type, the load ops must be
   * flushed or else they would be obscured and eliminated by HSR.
   */
   if (subpass->input_count != 0)
                     for (uint32_t i = 0; i < subpass->color_count; i++) {
               if (color_idx != VK_ATTACHMENT_UNUSED)
               /* If the first subpass does not write to all attachments which have a load
   * op then the load ops need to be flushed to ensure they don't get obscured
   * and removed by HSR.
   */
      }
      static void
   pvr_init_subpass_isp_userpass(struct pvr_renderpass_hwsetup *hw_setup,
               {
               for (uint32_t i = 0; i < hw_setup->render_count; i++) {
      struct pvr_renderpass_hwsetup_render *hw_render = &hw_setup->renders[i];
   const uint32_t initial_isp_userpass =
            for (uint32_t j = 0; j < hw_render->subpass_count; j++) {
      subpasses[subpass_idx].isp_userpass =
                           }
      static inline bool pvr_has_output_register_writes(
         {
      for (uint32_t i = 0; i < hw_render->init_setup.num_render_targets; i++) {
      struct usc_mrt_resource *mrt_resource =
            if (mrt_resource->type == USC_MRT_RESOURCE_TYPE_OUTPUT_REG)
                  }
      VkResult pvr_pds_unitex_state_program_create_and_upload(
      struct pvr_device *device,
   const VkAllocationCallbacks *allocator,
   uint32_t texture_kicks,
   uint32_t uniform_kicks,
      {
      struct pvr_pds_pixel_shader_sa_program program = {
      .num_texture_dma_kicks = texture_kicks,
      };
   uint32_t staging_buffer_size;
   uint32_t *staging_buffer;
                              staging_buffer = vk_alloc2(&device->vk.alloc,
                           if (!staging_buffer)
                     /* FIXME: Figure out the define for alignment of 16. */
   result = pvr_gpu_upload_pds(device,
                              NULL,
   0U,
   0U,
      if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, allocator, staging_buffer);
                           }
      /* TODO: pvr_create_subpass_load_op() and pvr_create_render_load_op() are quite
   * similar. See if we can dedup them?
   */
   static VkResult
   pvr_create_subpass_load_op(struct pvr_device *device,
                                 {
      const struct pvr_renderpass_hwsetup_subpass *hw_subpass =
         const struct pvr_render_subpass *subpass =
            struct pvr_load_op *load_op = vk_zalloc2(&device->vk.alloc,
                           if (!load_op)
                     if (hw_subpass->z_replicate != -1) {
               switch (hw_subpass->depth_initop) {
   case VK_ATTACHMENT_LOAD_OP_LOAD:
      assert(z_replicate < PVR_LOAD_OP_CLEARS_LOADS_MAX_RTS);
   load_op->clears_loads_state.rt_load_mask = BITFIELD_BIT(z_replicate);
   load_op->clears_loads_state.dest_vk_format[z_replicate] =
               case VK_ATTACHMENT_LOAD_OP_CLEAR:
                  default:
                     assert(subpass->color_count <= PVR_LOAD_OP_CLEARS_LOADS_MAX_RTS);
   for (uint32_t i = 0; i < subpass->color_count; i++) {
               assert(attachment_idx < pass->attachment_count);
   load_op->clears_loads_state.dest_vk_format[i] =
            if (pass->attachments[attachment_idx].sample_count > 1)
            if (hw_subpass->color_initops[i] == VK_ATTACHMENT_LOAD_OP_LOAD)
         else if (hw_subpass->color_initops[i] == VK_ATTACHMENT_LOAD_OP_CLEAR)
               load_op->is_hw_object = false;
                        }
      static VkResult
   pvr_create_render_load_op(struct pvr_device *device,
                           {
      struct pvr_load_op *load_op = vk_zalloc2(&device->vk.alloc,
                           if (!load_op)
                     assert(hw_render->color_init_count <= PVR_LOAD_OP_CLEARS_LOADS_MAX_RTS);
   for (uint32_t i = 0; i < hw_render->color_init_count; i++) {
               assert(color_init->index < pass->attachment_count);
   load_op->clears_loads_state.dest_vk_format[i] =
            if (pass->attachments[color_init->index].sample_count > 1)
            if (color_init->op == VK_ATTACHMENT_LOAD_OP_LOAD)
         else if (color_init->op == VK_ATTACHMENT_LOAD_OP_CLEAR)
               load_op->is_hw_object = true;
                        }
      static VkResult
   pvr_generate_load_op_shader(struct pvr_device *device,
                     {
      const struct pvr_device_info *dev_info = &device->pdevice->dev_info;
            VkResult result = pvr_gpu_upload_usc(device,
                           if (result != VK_SUCCESS)
            result = pvr_pds_fragment_program_create_and_upload(
      device,
   allocator,
   load_op->usc_frag_prog_bo,
   pvr_pds_fragment_program_params.temp_count,
   pvr_pds_fragment_program_params.msaa_mode,
   pvr_pds_fragment_program_params.has_phase_rate_change,
      if (result != VK_SUCCESS)
            result = pvr_pds_unitex_state_program_create_and_upload(
      device,
   allocator,
   1U,
   0U,
      if (result != VK_SUCCESS)
            /* FIXME: These should be based on the USC and PDS programs, but are hard
   * coded for now.
   */
   load_op->const_shareds_count = 1;
   load_op->shareds_dest_offset = 0;
   load_op->shareds_count = 1;
                  err_free_pds_frag_prog:
            err_free_usc_frag_prog_bo:
                  }
      static void pvr_load_op_destroy(struct pvr_device *device,
               {
      pvr_bo_suballoc_free(load_op->pds_tex_state_prog.pvr_bo);
   pvr_bo_suballoc_free(load_op->pds_frag_prog.pvr_bo);
   pvr_bo_suballoc_free(load_op->usc_frag_prog_bo);
      }
      #define PVR_SPM_LOAD_IN_BUFFERS_COUNT(dev_info)              \
      ({                                                        \
      int __ret = PVR_MAX_TILE_BUFFER_COUNT;                 \
   if (PVR_HAS_FEATURE(dev_info, eight_output_registers)) \
                  static bool
   pvr_is_load_op_needed(const struct pvr_render_pass *pass,
               {
      struct pvr_renderpass_hwsetup_subpass *hw_subpass =
         const struct pvr_render_subpass *subpass =
            if (hw_subpass->z_replicate != -1 &&
      (hw_subpass->depth_initop == VK_ATTACHMENT_LOAD_OP_LOAD ||
   hw_subpass->depth_initop == VK_ATTACHMENT_LOAD_OP_CLEAR)) {
               for (uint32_t i = 0; i < subpass->color_count; i++) {
      if (subpass->color_attachments[i] == VK_ATTACHMENT_UNUSED)
            if (hw_subpass->color_initops[i] == VK_ATTACHMENT_LOAD_OP_LOAD ||
      hw_subpass->color_initops[i] == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                     }
      VkResult pvr_CreateRenderPass2(VkDevice _device,
                     {
      struct pvr_render_pass_attachment *attachments;
   PVR_FROM_HANDLE(pvr_device, device, _device);
   struct pvr_render_subpass *subpasses;
   const VkAllocationCallbacks *alloc;
   size_t subpass_attachment_count;
   uint32_t *subpass_attachments;
   struct pvr_render_pass *pass;
   uint32_t *dep_list;
   bool *flush_on_dep;
                     VK_MULTIALLOC(ma);
   vk_multialloc_add(&ma, &pass, __typeof__(*pass), 1);
   vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                        subpass_attachment_count = 0;
   for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
      const VkSubpassDescription2 *desc = &pCreateInfo->pSubpasses[i];
   subpass_attachment_count +=
      desc->inputAttachmentCount + desc->colorAttachmentCount +
            vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                     vk_multialloc_add(&ma,
                        if (!vk_multialloc_zalloc(&ma, alloc, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT))
            vk_object_base_init(&device->vk, &pass->base, VK_OBJECT_TYPE_RENDER_PASS);
   pass->attachment_count = pCreateInfo->attachmentCount;
   pass->attachments = attachments;
   pass->subpass_count = pCreateInfo->subpassCount;
   pass->subpasses = subpasses;
            /* Copy attachment descriptions. */
   for (uint32_t i = 0; i < pass->attachment_count; i++) {
      const VkAttachmentDescription2 *desc = &pCreateInfo->pAttachments[i];
                     attachment->load_op = desc->loadOp;
            attachment->aspects = vk_format_aspects(desc->format);
   if (attachment->aspects & VK_IMAGE_ASPECT_STENCIL_BIT) {
      attachment->stencil_load_op = desc->stencilLoadOp;
               attachment->vk_format = desc->format;
   attachment->sample_count = desc->samples;
   attachment->initial_layout = desc->initialLayout;
   attachment->is_pbe_downscalable =
                  if (attachment->sample_count > pass->max_sample_count)
               /* Count how many dependencies each subpass has. */
   for (uint32_t i = 0; i < pCreateInfo->dependencyCount; i++) {
               if (dep->srcSubpass != VK_SUBPASS_EXTERNAL &&
      dep->dstSubpass != VK_SUBPASS_EXTERNAL &&
   dep->srcSubpass != dep->dstSubpass) {
                  /* Assign reference pointers to lists, and fill in the attachments list, we
   * need to re-walk the dependencies array later to fill the per-subpass
   * dependencies lists in.
   */
   for (uint32_t i = 0; i < pass->subpass_count; i++) {
      const VkSubpassDescription2 *desc = &pCreateInfo->pSubpasses[i];
                     /* From the Vulkan spec. 1.3.265
   * VUID-VkSubpassDescription2-multisampledRenderToSingleSampled-06872:
   *
   *   "If none of the VK_AMD_mixed_attachment_samples extension, the
   *   VK_NV_framebuffer_mixed_samples extension, or the
   *   multisampledRenderToSingleSampled feature are enabled, all
   *   attachments in pDepthStencilAttachment or pColorAttachments that are
   *   not VK_ATTACHMENT_UNUSED must have the same sample count"
   *
   */
            if (desc->pDepthStencilAttachment) {
                                 } else {
                  subpass->color_count = desc->colorAttachmentCount;
   if (subpass->color_count > 0) {
                     for (uint32_t j = 0; j < subpass->color_count; j++) {
                                    if (subpass->sample_count == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM) {
      uint32_t index;
   index = subpass->color_attachments[j];
                     if (subpass->sample_count == VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM)
            if (desc->pResolveAttachments) {
                     for (uint32_t j = 0; j < subpass->color_count; j++) {
      subpass->resolve_attachments[j] =
                  subpass->input_count = desc->inputAttachmentCount;
   if (subpass->input_count > 0) {
                     for (uint32_t j = 0; j < subpass->input_count; j++) {
      subpass->input_attachments[j] =
                  /* Give the dependencies a slice of the subpass_attachments array. */
   subpass->dep_list = dep_list;
   dep_list += subpass->dep_count;
   subpass->flush_on_dep = flush_on_dep;
            /* Reset the dependencies count so we can start from 0 and index into
   * the dependencies array.
   */
   subpass->dep_count = 0;
               /* Compute dependencies and populate dep_list and flush_on_dep. */
   for (uint32_t i = 0; i < pCreateInfo->dependencyCount; i++) {
               if (dep->srcSubpass != VK_SUBPASS_EXTERNAL &&
      dep->dstSubpass != VK_SUBPASS_EXTERNAL &&
   dep->srcSubpass != dep->dstSubpass) {
   struct pvr_render_subpass *subpass = &pass->subpasses[dep->dstSubpass];
   bool is_dep_fb_local =
      vk_subpass_dependency_is_fb_local(dep,
               subpass->dep_list[subpass->dep_count] = dep->srcSubpass;
   if (pvr_subpass_has_msaa_input_attachment(subpass, pCreateInfo) ||
      !is_dep_fb_local) {
                              pass->max_tilebuffer_count =
            result =
         if (result != VK_SUCCESS)
                     for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
      struct pvr_renderpass_hwsetup_render *hw_render =
                  if (hw_render->tile_buffers_count) {
      result = pvr_device_tile_buffer_ensure_cap(
      device,
   hw_render->tile_buffers_count,
      if (result != VK_SUCCESS)
                        if (hw_render->color_init_count != 0U) {
      if (!pvr_has_output_register_writes(hw_render)) {
                              mrt_resources =
      vk_realloc(alloc,
            hw_render->init_setup.mrt_resources,
   hw_render->init_setup.num_render_targets *
         if (!mrt_resources) {
                                 mrt_resources[last].type = USC_MRT_RESOURCE_TYPE_OUTPUT_REG;
   mrt_resources[last].reg.output_reg = 0U;
   mrt_resources[last].reg.offset = 0U;
   mrt_resources[last].intermediate_size = 4U;
   mrt_resources[last].mrt_desc.intermediate_size = 4U;
   mrt_resources[last].mrt_desc.priority = 0U;
   mrt_resources[last].mrt_desc.valid_mask[0U] = ~0;
   mrt_resources[last].mrt_desc.valid_mask[1U] = ~0;
   mrt_resources[last].mrt_desc.valid_mask[2U] = ~0;
               result = pvr_create_render_load_op(device,
                                          result =
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, load_op);
                           for (uint32_t j = 0; j < hw_render->subpass_count; j++) {
                     result = pvr_create_subpass_load_op(device,
                                 if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, load_op);
               result =
                                                      err_load_op_destroy:
      for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
      struct pvr_renderpass_hwsetup_render *hw_render =
            for (uint32_t j = 0; j < hw_render->subpass_count; j++) {
      if (hw_render->subpasses[j].load_op) {
      pvr_load_op_destroy(device,
                        if (hw_render->load_op)
                     err_free_pass:
      vk_object_base_finish(&pass->base);
               }
      void pvr_DestroyRenderPass(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            if (!pass)
            for (uint32_t i = 0; i < pass->hw_setup->render_count; i++) {
      struct pvr_renderpass_hwsetup_render *hw_render =
            for (uint32_t j = 0; j < hw_render->subpass_count; j++) {
      if (hw_render->subpasses[j].load_op) {
      pvr_load_op_destroy(device,
                        if (hw_render->load_op)
               pvr_destroy_renderpass_hwsetup(pAllocator ? pAllocator : &device->vk.alloc,
         vk_object_base_finish(&pass->base);
      }
      void pvr_GetRenderAreaGranularity(VkDevice _device,
               {
      PVR_FROM_HANDLE(pvr_device, device, _device);
            /* Granularity does not depend on any settings in the render pass, so return
   * the tile granularity.
   *
   * The default value is based on the minimum value found in all existing
   * cores.
   */
   pGranularity->width = PVR_GET_FEATURE_VALUE(dev_info, tile_size_x, 16);
      }
