   /*
   * Copyright © 2020 Valve Corporation
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
      #include "radv_cs.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "vk_common_entrypoints.h"
   #include "wsi_common_entrypoints.h"
      #include "ac_rgp.h"
   #include "ac_sqtt.h"
      void
   radv_sqtt_emit_relocated_shaders(struct radv_cmd_buffer *cmd_buffer, struct radv_graphics_pipeline *pipeline)
   {
      const enum amd_gfx_level gfx_level = cmd_buffer->device->physical_device->rad_info.gfx_level;
   struct radv_sqtt_shaders_reloc *reloc = pipeline->sqtt_shaders_reloc;
   struct radeon_cmdbuf *cs = cmd_buffer->cs;
                     /* VS */
   if (pipeline->base.shaders[MESA_SHADER_VERTEX]) {
               va = reloc->va[MESA_SHADER_VERTEX];
   if (vs->info.vs.as_ls) {
         } else if (vs->info.vs.as_es) {
      radeon_set_sh_reg_seq(cs, R_00B320_SPI_SHADER_PGM_LO_ES, 2);
   radeon_emit(cs, va >> 8);
      } else if (vs->info.is_ngg) {
         } else {
      radeon_set_sh_reg_seq(cs, R_00B120_SPI_SHADER_PGM_LO_VS, 2);
   radeon_emit(cs, va >> 8);
                  /* TCS */
   if (pipeline->base.shaders[MESA_SHADER_TESS_CTRL]) {
               if (gfx_level >= GFX9) {
      if (gfx_level >= GFX10) {
         } else {
            } else {
      radeon_set_sh_reg_seq(cs, R_00B420_SPI_SHADER_PGM_LO_HS, 2);
   radeon_emit(cs, va >> 8);
                  /* TES */
   if (pipeline->base.shaders[MESA_SHADER_TESS_EVAL]) {
               va = reloc->va[MESA_SHADER_TESS_EVAL];
   if (tes->info.is_ngg) {
         } else if (tes->info.tes.as_es) {
      radeon_set_sh_reg_seq(cs, R_00B320_SPI_SHADER_PGM_LO_ES, 2);
   radeon_emit(cs, va >> 8);
      } else {
      radeon_set_sh_reg_seq(cs, R_00B120_SPI_SHADER_PGM_LO_VS, 2);
   radeon_emit(cs, va >> 8);
                  /* GS */
   if (pipeline->base.shaders[MESA_SHADER_GEOMETRY]) {
               va = reloc->va[MESA_SHADER_GEOMETRY];
   if (gs->info.is_ngg) {
         } else {
      if (gfx_level >= GFX9) {
      if (gfx_level >= GFX10) {
         } else {
            } else {
      radeon_set_sh_reg_seq(cs, R_00B220_SPI_SHADER_PGM_LO_GS, 2);
   radeon_emit(cs, va >> 8);
                     /* FS */
   if (pipeline->base.shaders[MESA_SHADER_FRAGMENT]) {
               radeon_set_sh_reg_seq(cs, R_00B020_SPI_SHADER_PGM_LO_PS, 2);
   radeon_emit(cs, va >> 8);
               /* MS */
   if (pipeline->base.shaders[MESA_SHADER_MESH]) {
                     }
      static uint64_t
   radv_sqtt_shader_get_va_reloc(struct radv_pipeline *pipeline, gl_shader_stage stage)
   {
      if (pipeline->type == RADV_PIPELINE_GRAPHICS) {
      struct radv_graphics_pipeline *graphics_pipeline = radv_pipeline_to_graphics(pipeline);
   struct radv_sqtt_shaders_reloc *reloc = graphics_pipeline->sqtt_shaders_reloc;
                  }
      static VkResult
   radv_sqtt_reloc_graphics_shaders(struct radv_device *device, struct radv_graphics_pipeline *pipeline)
   {
      struct radv_shader_dma_submission *submission = NULL;
   struct radv_sqtt_shaders_reloc *reloc;
            reloc = calloc(1, sizeof(*reloc));
   if (!reloc)
            /* Compute the total code size. */
   for (int i = 0; i < MESA_VULKAN_SHADER_STAGES; i++) {
      const struct radv_shader *shader = pipeline->base.shaders[i];
   if (!shader)
                        /* Allocate memory for all shader binaries. */
   reloc->alloc = radv_alloc_shader_memory(device, code_size, false, pipeline);
   if (!reloc->alloc) {
      free(reloc);
                        /* Relocate shader binaries to be contiguous in memory as requested by RGP. */
   uint64_t slab_va = radv_buffer_get_va(reloc->bo) + reloc->alloc->offset;
   char *slab_ptr = reloc->alloc->arena->ptr + reloc->alloc->offset;
            if (device->shader_use_invisible_vram) {
      submission = radv_shader_dma_get_submission(device, reloc->bo, slab_va, code_size);
   if (!submission)
               for (int i = 0; i < MESA_VULKAN_SHADER_STAGES; ++i) {
      const struct radv_shader *shader = pipeline->base.shaders[i];
   void *dest_ptr;
   if (!shader)
                     if (device->shader_use_invisible_vram)
         else
                                 if (device->shader_use_invisible_vram) {
      if (!radv_shader_dma_submit(device, submission, &pipeline->base.shader_upload_seq))
                           }
      static void
   radv_write_begin_general_api_marker(struct radv_cmd_buffer *cmd_buffer, enum rgp_sqtt_marker_general_api_type api_type)
   {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_GENERAL_API;
               }
      static void
   radv_write_end_general_api_marker(struct radv_cmd_buffer *cmd_buffer, enum rgp_sqtt_marker_general_api_type api_type)
   {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_GENERAL_API;
   marker.api_type = api_type;
               }
      static void
   radv_write_event_marker(struct radv_cmd_buffer *cmd_buffer, enum rgp_sqtt_marker_event_type api_type,
               {
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
   marker.api_type = api_type;
   marker.cmd_id = cmd_buffer->state.num_events++;
            if (vertex_offset_user_data == UINT_MAX || instance_offset_user_data == UINT_MAX) {
      vertex_offset_user_data = 0;
               if (draw_index_user_data == UINT_MAX)
            marker.vertex_offset_reg_idx = vertex_offset_user_data;
   marker.instance_offset_reg_idx = instance_offset_user_data;
               }
      static void
   radv_write_event_with_dims_marker(struct radv_cmd_buffer *cmd_buffer, enum rgp_sqtt_marker_event_type api_type,
         {
               marker.event.identifier = RGP_SQTT_MARKER_IDENTIFIER_EVENT;
   marker.event.api_type = api_type;
   marker.event.cmd_id = cmd_buffer->state.num_events++;
   marker.event.cb_id = cmd_buffer->sqtt_cb_id;
            marker.thread_x = x;
   marker.thread_y = y;
               }
      static void
   radv_write_user_event_marker(struct radv_cmd_buffer *cmd_buffer, enum rgp_sqtt_marker_user_event_type type,
         {
      if (type == UserEventPop) {
      assert(str == NULL);
   struct rgp_sqtt_marker_user_event marker = {0};
   marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_USER_EVENT;
               } else {
      assert(str != NULL);
   unsigned len = strlen(str);
   struct rgp_sqtt_marker_user_event_with_length marker = {0};
   marker.user_event.identifier = RGP_SQTT_MARKER_IDENTIFIER_USER_EVENT;
   marker.user_event.data_type = type;
            uint8_t *buffer = alloca(sizeof(marker) + marker.length);
   memset(buffer, 0, sizeof(marker) + marker.length);
   memcpy(buffer, &marker, sizeof(marker));
                  }
      void
   radv_describe_begin_cmd_buffer(struct radv_cmd_buffer *cmd_buffer)
   {
      uint64_t device_id = (uintptr_t)cmd_buffer->device;
            if (likely(!cmd_buffer->device->sqtt.bo))
            /* Reserve a command buffer ID for SQTT. */
   enum amd_ip_type ip_type = radv_queue_family_to_ring(cmd_buffer->device->physical_device, cmd_buffer->qf);
   union rgp_sqtt_marker_cb_id cb_id = ac_sqtt_get_next_cmdbuf_id(&cmd_buffer->device->sqtt, ip_type);
            marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_CB_START;
   marker.cb_id = cmd_buffer->sqtt_cb_id;
   marker.device_id_low = device_id;
   marker.device_id_high = device_id >> 32;
   marker.queue = cmd_buffer->qf;
            if (cmd_buffer->qf == RADV_QUEUE_GENERAL)
               }
      void
   radv_describe_end_cmd_buffer(struct radv_cmd_buffer *cmd_buffer)
   {
      uint64_t device_id = (uintptr_t)cmd_buffer->device;
            if (likely(!cmd_buffer->device->sqtt.bo))
            marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_CB_END;
   marker.cb_id = cmd_buffer->sqtt_cb_id;
   marker.device_id_low = device_id;
               }
      void
   radv_describe_draw(struct radv_cmd_buffer *cmd_buffer)
   {
      if (likely(!cmd_buffer->device->sqtt.bo))
               }
      void
   radv_describe_dispatch(struct radv_cmd_buffer *cmd_buffer, const struct radv_dispatch_info *info)
   {
      if (likely(!cmd_buffer->device->sqtt.bo))
            if (info->indirect) {
         } else {
      radv_write_event_with_dims_marker(cmd_buffer, cmd_buffer->state.current_event_type, info->blocks[0],
         }
      void
   radv_describe_begin_render_pass_clear(struct radv_cmd_buffer *cmd_buffer, VkImageAspectFlagBits aspects)
   {
      cmd_buffer->state.current_event_type =
      }
      void
   radv_describe_end_render_pass_clear(struct radv_cmd_buffer *cmd_buffer)
   {
         }
      void
   radv_describe_begin_render_pass_resolve(struct radv_cmd_buffer *cmd_buffer)
   {
         }
      void
   radv_describe_end_render_pass_resolve(struct radv_cmd_buffer *cmd_buffer)
   {
         }
      void
   radv_describe_barrier_end_delayed(struct radv_cmd_buffer *cmd_buffer)
   {
               if (likely(!cmd_buffer->device->sqtt.bo) || !cmd_buffer->state.pending_sqtt_barrier_end)
                     marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BARRIER_END;
                     if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_WAIT_ON_EOP_TS)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_VS_PARTIAL_FLUSH)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_PS_PARTIAL_FLUSH)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_CS_PARTIAL_FLUSH)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_PFP_SYNC_ME)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_SYNC_CP_DMA)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_VMEM_L0)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_ICACHE)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_SMEM_L0)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_FLUSH_L2)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_L2)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_FLUSH_CB)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_CB)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_FLUSH_DB)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_DB)
         if (cmd_buffer->state.sqtt_flush_bits & RGP_FLUSH_INVAL_L1)
                        }
      void
   radv_describe_barrier_start(struct radv_cmd_buffer *cmd_buffer, enum rgp_barrier_reason reason)
   {
               if (likely(!cmd_buffer->device->sqtt.bo))
            if (cmd_buffer->state.in_barrier) {
      assert(!"attempted to start a barrier while already in a barrier");
               radv_describe_barrier_end_delayed(cmd_buffer);
   cmd_buffer->state.sqtt_flush_bits = 0;
            marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BARRIER_START;
   marker.cb_id = cmd_buffer->sqtt_cb_id;
               }
      void
   radv_describe_barrier_end(struct radv_cmd_buffer *cmd_buffer)
   {
      cmd_buffer->state.in_barrier = false;
      }
      void
   radv_describe_layout_transition(struct radv_cmd_buffer *cmd_buffer, const struct radv_barrier_data *barrier)
   {
               if (likely(!cmd_buffer->device->sqtt.bo))
            if (!cmd_buffer->state.in_barrier) {
      assert(!"layout transition marker should be only emitted inside a barrier marker");
               marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_LAYOUT_TRANSITION;
   marker.depth_stencil_expand = barrier->layout_transitions.depth_stencil_expand;
   marker.htile_hiz_range_expand = barrier->layout_transitions.htile_hiz_range_expand;
   marker.depth_stencil_resummarize = barrier->layout_transitions.depth_stencil_resummarize;
   marker.dcc_decompress = barrier->layout_transitions.dcc_decompress;
   marker.fmask_decompress = barrier->layout_transitions.fmask_decompress;
   marker.fast_clear_eliminate = barrier->layout_transitions.fast_clear_eliminate;
   marker.fmask_color_expand = barrier->layout_transitions.fmask_color_expand;
                        }
      static void
   radv_describe_pipeline_bind(struct radv_cmd_buffer *cmd_buffer, VkPipelineBindPoint pipelineBindPoint,
         {
               if (likely(!cmd_buffer->device->sqtt.bo))
            marker.identifier = RGP_SQTT_MARKER_IDENTIFIER_BIND_PIPELINE;
   marker.cb_id = cmd_buffer->sqtt_cb_id;
   marker.bind_point = pipelineBindPoint;
   marker.api_pso_hash[0] = pipeline->pipeline_hash;
               }
      static void
   radv_handle_sqtt(VkQueue _queue)
   {
               bool trigger = queue->device->sqtt_triggered;
            if (queue->device->sqtt_enabled) {
               radv_end_sqtt(queue);
            /* TODO: Do something better than this whole sync. */
            if (radv_get_sqtt_trace(queue, &sqtt_trace)) {
                              ac_dump_rgp_capture(&queue->device->physical_device->rad_info, &sqtt_trace,
      } else {
      /* Trigger a new capture if the driver failed to get
   * the trace because the buffer was too small.
   */
               /* Clear resources used for this capture. */
               if (trigger) {
      if (ac_check_profile_state(&queue->device->physical_device->rad_info)) {
      fprintf(stderr, "radv: Canceling RGP trace request as a hang condition has been "
                                 radv_begin_sqtt(queue);
   assert(!queue->device->sqtt_enabled);
         }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_QueuePresentKHR(VkQueue _queue, const VkPresentInfoKHR *pPresentInfo)
   {
      RADV_FROM_HANDLE(radv_queue, queue, _queue);
            result = queue->device->layer_dispatch.rgp.QueuePresentKHR(_queue, pPresentInfo);
   if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
                        }
      #define EVENT_MARKER_BASE(cmd_name, api_name, event_name, ...)                                                         \
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);                                                       \
   radv_write_begin_general_api_marker(cmd_buffer, ApiCmd##api_name);                                                  \
   cmd_buffer->state.current_event_type = EventCmd##event_name;                                                        \
   cmd_buffer->device->layer_dispatch.rgp.Cmd##cmd_name(__VA_ARGS__);                                                  \
   cmd_buffer->state.current_event_type = EventInternalUnknown;                                                        \
         #define EVENT_MARKER_ALIAS(cmd_name, api_name, ...) EVENT_MARKER_BASE(cmd_name, api_name, api_name, __VA_ARGS__);
      #define EVENT_MARKER(cmd_name, ...) EVENT_MARKER_ALIAS(cmd_name, cmd_name, __VA_ARGS__);
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t drawCount,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkBuffer countBuffer,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawIndexedIndirectCount(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
               {
      EVENT_MARKER(DrawIndexedIndirectCount, commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize fillSize,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyImage2(VkCommandBuffer commandBuffer, const VkCopyImageInfo2 *pCopyImageInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyBufferToImage2(VkCommandBuffer commandBuffer, const VkCopyBufferToImageInfo2 *pCopyBufferToImageInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyImageToBuffer2(VkCommandBuffer commandBuffer, const VkCopyImageToBufferInfo2 *pCopyImageToBufferInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBlitImage2(VkCommandBuffer commandBuffer, const VkBlitImageInfo2 *pBlitImageInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image_h, VkImageLayout imageLayout,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image_h, VkImageLayout imageLayout,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount, const VkClearAttachment *pAttachments,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdResolveImage2(VkCommandBuffer commandBuffer, const VkResolveImageInfo2 *pResolveImageInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdWaitEvents2(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdPipelineBarrier2(VkCommandBuffer commandBuffer, const VkDependencyInfo *pDependencyInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
               {
      EVENT_MARKER(CopyQueryPoolResults, commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride,
      }
      #define EVENT_RT_MARKER(cmd_name, flags, ...) EVENT_MARKER_BASE(cmd_name, Dispatch, cmd_name | flags, __VA_ARGS__);
      #define EVENT_RT_MARKER_ALIAS(cmd_name, event_name, flags, ...)                                                        \
            VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdTraceRaysKHR(VkCommandBuffer commandBuffer, const VkStridedDeviceAddressRegionKHR *pRaygenShaderBindingTable,
                           {
      EVENT_RT_MARKER(TraceRaysKHR, ApiRayTracingSeparateCompiled, commandBuffer, pRaygenShaderBindingTable,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdTraceRaysIndirectKHR(VkCommandBuffer commandBuffer,
                                 {
      EVENT_RT_MARKER(TraceRaysIndirectKHR, ApiRayTracingSeparateCompiled, commandBuffer, pRaygenShaderBindingTable,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdTraceRaysIndirect2KHR(VkCommandBuffer commandBuffer, VkDeviceAddress indirectDeviceAddress)
   {
      EVENT_RT_MARKER_ALIAS(TraceRaysIndirect2KHR, TraceRaysIndirectKHR, ApiRayTracingSeparateCompiled, commandBuffer,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBuildAccelerationStructuresKHR(VkCommandBuffer commandBuffer, uint32_t infoCount,
               {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyAccelerationStructureKHR(VkCommandBuffer commandBuffer, const VkCopyAccelerationStructureInfoKHR *pInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyAccelerationStructureToMemoryKHR(VkCommandBuffer commandBuffer,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdCopyMemoryToAccelerationStructureKHR(VkCommandBuffer commandBuffer,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawMeshTasksEXT(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawMeshTasksIndirectEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDrawMeshTasksIndirectCountEXT(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
               {
      EVENT_MARKER(DrawMeshTasksIndirectCountEXT, commandBuffer, buffer, offset, countBuffer, countBufferOffset,
      }
      #undef EVENT_RT_MARKER_ALIAS
   #undef EVENT_RT_MARKER
      #undef EVENT_MARKER
   #undef EVENT_MARKER_ALIAS
   #undef EVENT_MARKER_BASE
      #define API_MARKER_ALIAS(cmd_name, api_name, ...)                                                                      \
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);                                                       \
   radv_write_begin_general_api_marker(cmd_buffer, ApiCmd##api_name);                                                  \
   cmd_buffer->device->layer_dispatch.rgp.Cmd##cmd_name(__VA_ARGS__);                                                  \
         #define API_MARKER(cmd_name, ...) API_MARKER_ALIAS(cmd_name, cmd_name, __VA_ARGS__);
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline _pipeline)
   {
                        if (pipelineBindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR) {
      /* RGP seems to expect a compute bind point to detect and report RT pipelines, which makes
   * sense somehow given that RT shaders are compiled to an unified compute shader.
   */
      } else {
            }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                     {
      API_MARKER(BindDescriptorSets, commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBindVertexBuffers2(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
               {
      API_MARKER_ALIAS(BindVertexBuffers2, BindVertexBuffers, commandBuffer, firstBinding, bindingCount, pBuffers,
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query, VkQueryControlFlags flags)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t query)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdWriteTimestamp2(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 stage, VkQueryPool queryPool,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBeginRendering(VkCommandBuffer commandBuffer, const VkRenderingInfo *pRenderingInfo)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdEndRendering(VkCommandBuffer commandBuffer)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBufferCount, const VkCommandBuffer *pCmdBuffers)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdExecuteGeneratedCommandsNV(VkCommandBuffer commandBuffer, VkBool32 isPreprocessed,
         {
      /* There is no ExecuteIndirect Vulkan event in RGP yet. */
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
         {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4])
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t compareMask)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask)
   {
         }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference)
   {
         }
      /* VK_EXT_debug_marker */
   VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDebugMarkerBeginEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDebugMarkerEndEXT(VkCommandBuffer commandBuffer)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdDebugMarkerInsertEXT(VkCommandBuffer commandBuffer, const VkDebugMarkerMarkerInfoEXT *pMarkerInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_DebugMarkerSetObjectNameEXT(VkDevice device, const VkDebugMarkerObjectNameInfoEXT *pNameInfo)
   {
      /* no-op */
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_DebugMarkerSetObjectTagEXT(VkDevice device, const VkDebugMarkerObjectTagInfoEXT *pTagInfo)
   {
      /* no-op */
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdBeginDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdEndDebugUtilsLabelEXT(VkCommandBuffer commandBuffer)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_CmdInsertDebugUtilsLabelEXT(VkCommandBuffer commandBuffer, const VkDebugUtilsLabelEXT *pLabelInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
               }
      /* Pipelines */
   static enum rgp_hardware_stages
   radv_get_rgp_shader_stage(struct radv_shader *shader)
   {
      switch (shader->info.stage) {
   case MESA_SHADER_VERTEX:
      if (shader->info.vs.as_ls)
         else if (shader->info.vs.as_es)
         else if (shader->info.is_ngg)
         else
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
      if (shader->info.tes.as_es)
         else if (shader->info.is_ngg)
         else
      case MESA_SHADER_MESH:
   case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_TASK:
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_RAYGEN:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_ANY_HIT:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_MISS:
   case MESA_SHADER_CALLABLE:
         default:
            }
      static void
   radv_fill_code_object_record(struct radv_device *device, struct rgp_shader_data *shader_data,
         {
      struct radv_physical_device *pdevice = device->physical_device;
   unsigned lds_increment = pdevice->rad_info.gfx_level >= GFX11 && shader->info.stage == MESA_SHADER_FRAGMENT
                  memset(shader_data->rt_shader_name, 0, sizeof(shader_data->rt_shader_name));
   shader_data->hash[0] = (uint64_t)(uintptr_t)shader;
   shader_data->hash[1] = (uint64_t)(uintptr_t)shader >> 32;
   shader_data->code_size = shader->code_size;
   shader_data->code = shader->code;
   shader_data->vgpr_count = shader->config.num_vgprs;
   shader_data->sgpr_count = shader->config.num_sgprs;
   shader_data->scratch_memory_size = shader->config.scratch_bytes_per_wave;
   shader_data->lds_size = shader->config.lds_size * lds_increment;
   shader_data->wavefront_size = shader->info.wave_size;
   shader_data->base_address = va & 0xffffffffffff;
   shader_data->elf_symbol_offset = 0;
   shader_data->hw_stage = radv_get_rgp_shader_stage(shader);
      }
      static VkResult
   radv_add_code_object(struct radv_device *device, struct radv_pipeline *pipeline)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
   struct rgp_code_object *code_object = &sqtt->rgp_code_object;
            record = malloc(sizeof(struct rgp_code_object_record));
   if (!record)
            record->shader_stages_mask = 0;
   record->num_shaders_combined = 0;
   record->pipeline_hash[0] = pipeline->pipeline_hash;
   record->pipeline_hash[1] = pipeline->pipeline_hash;
            for (unsigned i = 0; i < MESA_VULKAN_SHADER_STAGES; i++) {
               if (!shader)
                     record->shader_stages_mask |= (1 << i);
               simple_mtx_lock(&code_object->lock);
   list_addtail(&record->list, &code_object->record);
   code_object->record_count++;
               }
      static VkResult
   radv_add_rt_record(struct radv_device *device, struct rgp_code_object *code_object,
               {
      struct rgp_code_object_record *record = malloc(sizeof(struct rgp_code_object_record));
   if (!record)
                     record->shader_stages_mask = 0;
   record->num_shaders_combined = 0;
   record->pipeline_hash[0] = hash;
            radv_fill_code_object_record(device, shader_data, shader, shader->va);
            record->shader_stages_mask |= (1 << shader->info.stage);
   record->is_rt = true;
   switch (shader->info.stage) {
   case MESA_SHADER_RAYGEN:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "rgen_%d", index);
      case MESA_SHADER_CLOSEST_HIT:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "chit_%d", index);
      case MESA_SHADER_MISS:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "miss_%d", index);
      case MESA_SHADER_INTERSECTION:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "traversal");
      case MESA_SHADER_CALLABLE:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "call_%d", index);
      case MESA_SHADER_COMPUTE:
      snprintf(shader_data->rt_shader_name, sizeof(shader_data->rt_shader_name), "_amdgpu_cs_main");
      default:
         }
            simple_mtx_lock(&code_object->lock);
   list_addtail(&record->list, &code_object->record);
   code_object->record_count++;
               }
      static void
   compute_unique_rt_sha(uint64_t pipeline_hash, unsigned index, unsigned char sha1[SHA1_DIGEST_LENGTH])
   {
      struct mesa_sha1 ctx;
   _mesa_sha1_init(&ctx);
   _mesa_sha1_update(&ctx, &pipeline_hash, sizeof(pipeline_hash));
   _mesa_sha1_update(&ctx, &index, sizeof(index));
      }
      static VkResult
   radv_register_rt_stage(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline, uint32_t index,
         {
      unsigned char sha1[SHA1_DIGEST_LENGTH];
                     result = ac_sqtt_add_pso_correlation(&device->sqtt, *(uint64_t *)sha1, pipeline->base.base.pipeline_hash);
   if (!result)
         result = ac_sqtt_add_code_object_loader_event(&device->sqtt, *(uint64_t *)sha1, shader->va);
   if (!result)
         result =
            }
      static VkResult
   radv_register_rt_pipeline(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline)
   {
               uint32_t max_any_hit_stack_size = 0;
            for (unsigned i = 0; i < pipeline->stage_count; i++) {
      struct radv_ray_tracing_stage *stage = &pipeline->stages[i];
   if (!radv_ray_tracing_stage_is_compiled(stage)) {
      if (stage->stage == MESA_SHADER_ANY_HIT)
         else if (stage->stage == MESA_SHADER_INTERSECTION)
         else
            }
   struct radv_shader *shader = container_of(pipeline->stages[i].shader, struct radv_shader, base);
   result = radv_register_rt_stage(device, pipeline, i, stage->stack_size, shader);
   if (result != VK_SUCCESS)
                        /* Combined traversal shader */
   result = radv_register_rt_stage(device, pipeline, idx++, max_any_hit_stack_size + max_intersection_stack_size,
         if (result != VK_SUCCESS)
            /* Prolog */
               }
      static VkResult
   radv_register_pipeline(struct radv_device *device, struct radv_pipeline *pipeline)
   {
      bool result;
            result = ac_sqtt_add_pso_correlation(&device->sqtt, pipeline->pipeline_hash, pipeline->pipeline_hash);
   if (!result)
            /* Find the lowest shader BO VA. */
   for (unsigned i = 0; i < MESA_VULKAN_SHADER_STAGES; i++) {
      struct radv_shader *shader = pipeline->shaders[i];
            if (!shader)
            va = radv_sqtt_shader_get_va_reloc(pipeline, i);
               result = ac_sqtt_add_code_object_loader_event(&device->sqtt, pipeline->pipeline_hash, base_va);
   if (!result)
            result = radv_add_code_object(device, pipeline);
   if (result != VK_SUCCESS)
               }
      static void
   radv_unregister_records(struct radv_device *device, uint64_t hash)
   {
      struct ac_sqtt *sqtt = &device->sqtt;
   struct rgp_pso_correlation *pso_correlation = &sqtt->rgp_pso_correlation;
   struct rgp_loader_events *loader_events = &sqtt->rgp_loader_events;
            /* Destroy the PSO correlation record. */
   simple_mtx_lock(&pso_correlation->lock);
   list_for_each_entry_safe (struct rgp_pso_correlation_record, record, &pso_correlation->record, list) {
      if (record->pipeline_hash[0] == hash) {
      pso_correlation->record_count--;
   list_del(&record->list);
   free(record);
         }
            /* Destroy the code object loader record. */
   simple_mtx_lock(&loader_events->lock);
   list_for_each_entry_safe (struct rgp_loader_events_record, record, &loader_events->record, list) {
      if (record->code_object_hash[0] == hash) {
      loader_events->record_count--;
   list_del(&record->list);
   free(record);
         }
            /* Destroy the code object record. */
   simple_mtx_lock(&code_object->lock);
   list_for_each_entry_safe (struct rgp_code_object_record, record, &code_object->record, list) {
      if (record->pipeline_hash[0] == hash) {
      code_object->record_count--;
   list_del(&record->list);
   free(record);
         }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_CreateGraphicsPipelines(VkDevice _device, VkPipelineCache pipelineCache, uint32_t count,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
            result = device->layer_dispatch.rgp.CreateGraphicsPipelines(_device, pipelineCache, count, pCreateInfos, pAllocator,
         if (result != VK_SUCCESS)
            for (unsigned i = 0; i < count; i++) {
               if (!pipeline)
            const VkPipelineCreateFlagBits2KHR create_flags = radv_get_pipeline_create_flags(&pCreateInfos[i]);
   if (create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR)
            result = radv_sqtt_reloc_graphics_shaders(device, radv_pipeline_to_graphics(pipeline));
   if (result != VK_SUCCESS)
            result = radv_register_pipeline(device, pipeline);
   if (result != VK_SUCCESS)
                     fail:
      for (unsigned i = 0; i < count; i++) {
      sqtt_DestroyPipeline(_device, pPipelines[i], pAllocator);
      }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_CreateComputePipelines(VkDevice _device, VkPipelineCache pipelineCache, uint32_t count,
               {
      RADV_FROM_HANDLE(radv_device, device, _device);
            result = device->layer_dispatch.rgp.CreateComputePipelines(_device, pipelineCache, count, pCreateInfos, pAllocator,
         if (result != VK_SUCCESS)
            for (unsigned i = 0; i < count; i++) {
               if (!pipeline)
            result = radv_register_pipeline(device, pipeline);
   if (result != VK_SUCCESS)
                     fail:
      for (unsigned i = 0; i < count; i++) {
      sqtt_DestroyPipeline(_device, pPipelines[i], pAllocator);
      }
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   sqtt_CreateRayTracingPipelinesKHR(VkDevice _device, VkDeferredOperationKHR deferredOperation,
                     {
      RADV_FROM_HANDLE(radv_device, device, _device);
            result = device->layer_dispatch.rgp.CreateRayTracingPipelinesKHR(_device, deferredOperation, pipelineCache, count,
         if (result != VK_SUCCESS)
            for (unsigned i = 0; i < count; i++) {
               if (!pipeline)
            const VkPipelineCreateFlagBits2KHR create_flags = radv_get_pipeline_create_flags(&pCreateInfos[i]);
   if (create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR)
            result = radv_register_rt_pipeline(device, radv_pipeline_to_ray_tracing(pipeline));
   if (result != VK_SUCCESS)
                     fail:
      for (unsigned i = 0; i < count; i++) {
      sqtt_DestroyPipeline(_device, pPipelines[i], pAllocator);
      }
      }
      VKAPI_ATTR void VKAPI_CALL
   sqtt_DestroyPipeline(VkDevice _device, VkPipeline _pipeline, const VkAllocationCallbacks *pAllocator)
   {
      RADV_FROM_HANDLE(radv_device, device, _device);
            if (!_pipeline)
            /* Ray tracing pipelines have multiple records, each with their own hash */
   if (pipeline->type == RADV_PIPELINE_RAY_TRACING) {
      /* We have one record for each stage, plus one for the traversal shader and one for the prolog */
   uint32_t record_count = radv_pipeline_to_ray_tracing(pipeline)->stage_count + 2;
   unsigned char sha1[SHA1_DIGEST_LENGTH];
   for (uint32_t i = 0; i < record_count; ++i) {
      compute_unique_rt_sha(pipeline->pipeline_hash, i, sha1);
         } else
            if (pipeline->type == RADV_PIPELINE_GRAPHICS) {
      struct radv_graphics_pipeline *graphics_pipeline = radv_pipeline_to_graphics(pipeline);
            radv_free_shader_memory(device, reloc->alloc);
                  }
      #undef API_MARKER
