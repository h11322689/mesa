   /*
   * Copyright © 2021 Collabora Ltd.
   *
   * Derived from tu_cmd_buffer.c which is:
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   * Copyright © 2015 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include "panvk_private.h"
      #include "pan_encoder.h"
      #include "util/rounding.h"
   #include "vk_format.h"
      void
   panvk_CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding,
               {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   struct panvk_descriptor_state *desc_state =
                     for (uint32_t i = 0; i < bindingCount; i++) {
               cmdbuf->state.vb.bufs[firstBinding + i].address =
         cmdbuf->state.vb.bufs[firstBinding + i].size =
               cmdbuf->state.vb.count =
            }
      void
   panvk_CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->state.ib.buffer = buf;
   cmdbuf->state.ib.offset = offset;
   switch (indexType) {
   case VK_INDEX_TYPE_UINT16:
      cmdbuf->state.ib.index_size = 16;
      case VK_INDEX_TYPE_UINT32:
      cmdbuf->state.ib.index_size = 32;
      case VK_INDEX_TYPE_NONE_KHR:
      cmdbuf->state.ib.index_size = 0;
      case VK_INDEX_TYPE_UINT8_EXT:
      cmdbuf->state.ib.index_size = 8;
      default:
            }
      static void
   panvk_set_dyn_ssbo_pointers(struct panvk_descriptor_state *desc_state,
               {
               for (unsigned i = 0; i < set->layout->num_dyn_ssbos; i++) {
      const struct panvk_buffer_desc *ssbo =
            sysvals->dyn_ssbos[dyn_ssbo_offset + i] = (struct panvk_ssbo_addr){
      .base_addr = panvk_buffer_gpu_ptr(ssbo->buffer, ssbo->offset),
                     }
      void
   panvk_CmdBindDescriptorSets(VkCommandBuffer commandBuffer,
                              VkPipelineBindPoint pipelineBindPoint,
      {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            struct panvk_descriptor_state *descriptors_state =
            unsigned dynoffset_idx = 0;
   for (unsigned i = 0; i < descriptorSetCount; ++i) {
      unsigned idx = i + firstSet;
                     if (set->layout->num_dyn_ssbos || set->layout->num_dyn_ubos) {
                     for (unsigned b = 0; b < set->layout->binding_count; b++) {
                        if (set->layout->bindings[b].type ==
      VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
   bdesc = &descriptors_state->dyn.ubos[dyn_ubo_offset++];
   *bdesc =
      } else if (set->layout->bindings[b].type ==
            bdesc = &descriptors_state->dyn.ssbos[dyn_ssbo_offset++];
                     if (bdesc) {
                           if (set->layout->num_dyn_ssbos) {
      panvk_set_dyn_ssbo_pointers(descriptors_state,
               if (set->layout->num_dyn_ssbos)
            if (set->layout->num_ubos || set->layout->num_dyn_ubos ||
                  if (set->layout->num_textures)
            if (set->layout->num_samplers)
            if (set->layout->num_imgs) {
      descriptors_state->vs_attrib_bufs =
                           }
      void
   panvk_CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout,
               {
                        if (stageFlags & VK_SHADER_STAGE_ALL_GRAPHICS) {
      struct panvk_descriptor_state *desc_state =
            desc_state->ubos = 0;
               if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT) {
      struct panvk_descriptor_state *desc_state =
            desc_state->ubos = 0;
         }
      void
   panvk_CmdBindPipeline(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
            cmdbuf->bind_points[pipelineBindPoint].pipeline = pipeline;
            if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS) {
               if (!(pipeline->dynamic_state_mask &
            cmdbuf->state.viewport = pipeline->viewport;
      }
   if (!(pipeline->dynamic_state_mask &
            cmdbuf->state.scissor = pipeline->scissor;
                  /* Sysvals are passed through UBOs, we need dirty the UBO array if the
   * pipeline contain shaders using sysvals.
   */
      }
      void
   panvk_CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   assert(viewportCount == 1);
            cmdbuf->state.viewport = pViewports[0];
   cmdbuf->state.vpd = 0;
      }
      void
   panvk_CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor,
         {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   assert(scissorCount == 1);
            cmdbuf->state.scissor = pScissors[0];
   cmdbuf->state.vpd = 0;
      }
      void
   panvk_CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
   {
               cmdbuf->state.rast.line_width = lineWidth;
      }
      void
   panvk_CmdSetDepthBias(VkCommandBuffer commandBuffer,
               {
               cmdbuf->state.rast.depth_bias.constant_factor = depthBiasConstantFactor;
   cmdbuf->state.rast.depth_bias.clamp = depthBiasClamp;
   cmdbuf->state.rast.depth_bias.slope_factor = depthBiasSlopeFactor;
   cmdbuf->state.dirty |= PANVK_DYNAMIC_DEPTH_BIAS;
      }
      void
   panvk_CmdSetBlendConstants(VkCommandBuffer commandBuffer,
         {
               for (unsigned i = 0; i < 4; i++)
            cmdbuf->state.dirty |= PANVK_DYNAMIC_BLEND_CONSTANTS;
      }
      void
   panvk_CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds,
         {
         }
      void
   panvk_CmdSetStencilCompareMask(VkCommandBuffer commandBuffer,
               {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
            if (faceMask & VK_STENCIL_FACE_BACK_BIT)
            cmdbuf->state.dirty |= PANVK_DYNAMIC_STENCIL_COMPARE_MASK;
      }
      void
   panvk_CmdSetStencilWriteMask(VkCommandBuffer commandBuffer,
         {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
            if (faceMask & VK_STENCIL_FACE_BACK_BIT)
            cmdbuf->state.dirty |= PANVK_DYNAMIC_STENCIL_WRITE_MASK;
      }
      void
   panvk_CmdSetStencilReference(VkCommandBuffer commandBuffer,
         {
               if (faceMask & VK_STENCIL_FACE_FRONT_BIT)
            if (faceMask & VK_STENCIL_FACE_BACK_BIT)
            cmdbuf->state.dirty |= PANVK_DYNAMIC_STENCIL_REFERENCE;
      }
      VkResult
   panvk_CreateCommandPool(VkDevice _device,
                     {
      VK_FROM_HANDLE(panvk_device, device, _device);
            pool = vk_alloc2(&device->vk.alloc, pAllocator, sizeof(*pool), 8,
         if (pool == NULL)
            VkResult result =
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pool);
               panvk_bo_pool_init(&pool->desc_bo_pool);
   panvk_bo_pool_init(&pool->varying_bo_pool);
   panvk_bo_pool_init(&pool->tls_bo_pool);
   *pCmdPool = panvk_cmd_pool_to_handle(pool);
      }
      static void
   panvk_cmd_prepare_clear_values(struct panvk_cmd_buffer *cmdbuf,
         {
      for (unsigned i = 0; i < cmdbuf->state.pass->attachment_count; i++) {
      const struct panvk_render_pass_attachment *attachment =
                  if (util_format_is_depth_or_stencil(fmt)) {
      if (attachment->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR ||
      attachment->stencil_load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
   cmdbuf->state.clear[i].depth = in[i].depthStencil.depth;
      } else {
      cmdbuf->state.clear[i].depth = 0;
         } else {
      if (attachment->load_op == VK_ATTACHMENT_LOAD_OP_CLEAR) {
      union pipe_color_union *col =
            } else {
      memset(cmdbuf->state.clear[i].color, 0,
               }
      void
   panvk_cmd_fb_info_set_subpass(struct panvk_cmd_buffer *cmdbuf)
   {
      const struct panvk_subpass *subpass = cmdbuf->state.subpass;
   struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
   const struct panvk_framebuffer *fb = cmdbuf->state.framebuffer;
   const struct panvk_clear_value *clears = cmdbuf->state.clear;
            fbinfo->nr_samples = 1;
   fbinfo->rt_count = subpass->color_count;
   memset(&fbinfo->bifrost.pre_post.dcds, 0,
            for (unsigned cb = 0; cb < subpass->color_count; cb++) {
      int idx = subpass->color_attachments[cb].idx;
   view = idx != VK_ATTACHMENT_UNUSED ? fb->attachments[idx].iview : NULL;
   if (!view)
         fbinfo->rts[cb].view = &view->pview;
   fbinfo->rts[cb].clear = subpass->color_attachments[cb].clear;
   fbinfo->rts[cb].preload = subpass->color_attachments[cb].preload;
            memcpy(fbinfo->rts[cb].clear_value, clears[idx].color,
         fbinfo->nr_samples =
               if (subpass->zs_attachment.idx != VK_ATTACHMENT_UNUSED) {
      view = fb->attachments[subpass->zs_attachment.idx].iview;
   const struct util_format_description *fdesc =
            fbinfo->nr_samples =
            if (util_format_has_depth(fdesc)) {
      fbinfo->zs.clear.z = subpass->zs_attachment.clear;
   fbinfo->zs.clear_value.depth =
                     if (util_format_has_stencil(fdesc)) {
      fbinfo->zs.clear.s = subpass->zs_attachment.clear;
   fbinfo->zs.clear_value.stencil =
         if (!fbinfo->zs.view.zs)
            }
      void
   panvk_cmd_fb_info_init(struct panvk_cmd_buffer *cmdbuf)
   {
      struct pan_fb_info *fbinfo = &cmdbuf->state.fb.info;
                     *fbinfo = (struct pan_fb_info){
      .width = fb->width,
   .height = fb->height,
   .extent.maxx = fb->width - 1,
         }
      void
   panvk_CmdBeginRenderPass2(VkCommandBuffer commandBuffer,
               {
      VK_FROM_HANDLE(panvk_cmd_buffer, cmdbuf, commandBuffer);
   VK_FROM_HANDLE(panvk_render_pass, pass, pRenderPassBegin->renderPass);
            cmdbuf->state.pass = pass;
   cmdbuf->state.subpass = pass->subpasses;
   cmdbuf->state.framebuffer = fb;
   cmdbuf->state.render_area = pRenderPassBegin->renderArea;
   cmdbuf->state.batch =
      vk_zalloc(&cmdbuf->vk.pool->alloc, sizeof(*cmdbuf->state.batch), 8,
      util_dynarray_init(&cmdbuf->state.batch->jobs, NULL);
   util_dynarray_init(&cmdbuf->state.batch->event_ops, NULL);
   assert(pRenderPassBegin->clearValueCount <= pass->attachment_count);
   cmdbuf->state.clear =
      vk_zalloc(&cmdbuf->vk.pool->alloc,
            panvk_cmd_prepare_clear_values(cmdbuf, pRenderPassBegin->pClearValues);
   panvk_cmd_fb_info_init(cmdbuf);
      }
      void
   panvk_cmd_preload_fb_after_batch_split(struct panvk_cmd_buffer *cmdbuf)
   {
      for (unsigned i = 0; i < cmdbuf->state.fb.info.rt_count; i++) {
      if (cmdbuf->state.fb.info.rts[i].view) {
      cmdbuf->state.fb.info.rts[i].clear = false;
                  if (cmdbuf->state.fb.info.zs.view.zs) {
      cmdbuf->state.fb.info.zs.clear.z = false;
               if (cmdbuf->state.fb.info.zs.view.s ||
      (cmdbuf->state.fb.info.zs.view.zs &&
   util_format_is_depth_and_stencil(
         cmdbuf->state.fb.info.zs.clear.s = false;
         }
      struct panvk_batch *
   panvk_cmd_open_batch(struct panvk_cmd_buffer *cmdbuf)
   {
      assert(!cmdbuf->state.batch);
   cmdbuf->state.batch =
      vk_zalloc(&cmdbuf->vk.pool->alloc, sizeof(*cmdbuf->state.batch), 8,
      assert(cmdbuf->state.batch);
      }
      void
   panvk_CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer _buffer,
         {
         }
      void
   panvk_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer _buffer,
               {
         }
      void
   panvk_CmdDispatchBase(VkCommandBuffer commandBuffer, uint32_t base_x,
               {
         }
      void
   panvk_CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer _buffer,
         {
         }
