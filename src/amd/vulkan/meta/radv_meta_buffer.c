   #include "nir/nir_builder.h"
   #include "radv_meta.h"
      #include "radv_cs.h"
   #include "sid.h"
      static nir_shader *
   build_buffer_fill_shader(struct radv_device *dev)
   {
      nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_buffer_fill");
            nir_def *pconst = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
   nir_def *buffer_addr = nir_pack_64_2x32(&b, nir_channels(&b, pconst, 0b0011));
   nir_def *max_offset = nir_channel(&b, pconst, 2);
            nir_def *global_id =
      nir_iadd(&b, nir_imul_imm(&b, nir_channel(&b, nir_load_workgroup_id(&b), 0), b.shader->info.workgroup_size[0]),
         nir_def *offset = nir_imin(&b, nir_imul_imm(&b, global_id, 16), max_offset);
   nir_def *dst_addr = nir_iadd(&b, buffer_addr, nir_u2u64(&b, offset));
               }
      static nir_shader *
   build_buffer_copy_shader(struct radv_device *dev)
   {
      nir_builder b = radv_meta_init_shader(dev, MESA_SHADER_COMPUTE, "meta_buffer_copy");
            nir_def *pconst = nir_load_push_constant(&b, 4, 32, nir_imm_int(&b, 0), .range = 16);
   nir_def *max_offset = nir_load_push_constant(&b, 1, 32, nir_imm_int(&b, 0), .base = 16, .range = 4);
   nir_def *src_addr = nir_pack_64_2x32(&b, nir_channels(&b, pconst, 0b0011));
            nir_def *global_id =
      nir_iadd(&b, nir_imul_imm(&b, nir_channel(&b, nir_load_workgroup_id(&b), 0), b.shader->info.workgroup_size[0]),
                  nir_def *data = nir_build_load_global(&b, 4, 32, nir_iadd(&b, src_addr, offset), .align_mul = 4);
               }
      struct fill_constants {
      uint64_t addr;
   uint32_t max_offset;
      };
      struct copy_constants {
      uint64_t src_addr;
   uint64_t dst_addr;
      };
      VkResult
   radv_device_init_meta_buffer_state(struct radv_device *device)
   {
      VkResult result;
   nir_shader *fill_cs = build_buffer_fill_shader(device);
            VkPipelineLayoutCreateInfo fill_pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &fill_pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineLayoutCreateInfo copy_pl_create_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
   .setLayoutCount = 0,
   .pushConstantRangeCount = 1,
               result = radv_CreatePipelineLayout(radv_device_to_handle(device), &copy_pl_create_info, &device->meta_state.alloc,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo fill_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(fill_cs),
   .pName = "main",
               VkComputePipelineCreateInfo fill_vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = fill_pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache,
         if (result != VK_SUCCESS)
            VkPipelineShaderStageCreateInfo copy_pipeline_shader_stage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_COMPUTE_BIT,
   .module = vk_shader_module_handle_from_nir(copy_cs),
   .pName = "main",
               VkComputePipelineCreateInfo copy_vk_pipeline_info = {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
   .stage = copy_pipeline_shader_stage,
   .flags = 0,
               result = radv_compute_pipeline_create(radv_device_to_handle(device), device->meta_state.cache,
         if (result != VK_SUCCESS)
            ralloc_free(fill_cs);
   ralloc_free(copy_cs);
      fail:
      ralloc_free(fill_cs);
   ralloc_free(copy_cs);
      }
      void
   radv_device_finish_meta_buffer_state(struct radv_device *device)
   {
               radv_DestroyPipeline(radv_device_to_handle(device), state->buffer.copy_pipeline, &state->alloc);
   radv_DestroyPipeline(radv_device_to_handle(device), state->buffer.fill_pipeline, &state->alloc);
   radv_DestroyPipelineLayout(radv_device_to_handle(device), state->buffer.copy_p_layout, &state->alloc);
      }
      static void
   fill_buffer_shader(struct radv_cmd_buffer *cmd_buffer, uint64_t va, uint64_t size, uint32_t data)
   {
      struct radv_device *device = cmd_buffer->device;
                     radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                     struct fill_constants fill_consts = {
      .addr = va,
   .max_offset = size - 16,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.buffer.fill_p_layout,
                        }
      static void
   copy_buffer_shader(struct radv_cmd_buffer *cmd_buffer, uint64_t src_va, uint64_t dst_va, uint64_t size)
   {
      struct radv_device *device = cmd_buffer->device;
                     radv_CmdBindPipeline(radv_cmd_buffer_to_handle(cmd_buffer), VK_PIPELINE_BIND_POINT_COMPUTE,
                     struct copy_constants copy_consts = {
      .src_addr = src_va,
   .dst_addr = dst_va,
               radv_CmdPushConstants(radv_cmd_buffer_to_handle(cmd_buffer), device->meta_state.buffer.copy_p_layout,
                        }
      static bool
   radv_prefer_compute_dma(const struct radv_device *device, uint64_t size, struct radeon_winsys_bo *src_bo,
         {
               if (device->physical_device->rad_info.gfx_level >= GFX10 && device->physical_device->rad_info.has_dedicated_vram) {
      if ((src_bo && !(src_bo->initial_domain & RADEON_DOMAIN_VRAM)) ||
      (dst_bo && !(dst_bo->initial_domain & RADEON_DOMAIN_VRAM))) {
   /* Prefer CP DMA for GTT on dGPUS due to slow PCIe. */
                     }
      uint32_t
   radv_fill_buffer(struct radv_cmd_buffer *cmd_buffer, const struct radv_image *image, struct radeon_winsys_bo *bo,
         {
      bool use_compute = radv_prefer_compute_dma(cmd_buffer->device, size, NULL, bo);
            assert(!(va & 3));
            if (bo)
            if (use_compute) {
                        flush_bits = RADV_CMD_FLAG_CS_PARTIAL_FLUSH | RADV_CMD_FLAG_INV_VCACHE |
      } else if (size)
               }
      void
   radv_copy_buffer(struct radv_cmd_buffer *cmd_buffer, struct radeon_winsys_bo *src_bo, struct radeon_winsys_bo *dst_bo,
         {
      bool use_compute = !(size & 3) && !(src_offset & 3) && !(dst_offset & 3) &&
            uint64_t src_va = radv_buffer_get_va(src_bo) + src_offset;
            radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, src_bo);
            if (use_compute)
         else if (size)
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize fillSize,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
                     radv_fill_buffer(cmd_buffer, NULL, dst_buffer->bo,
      }
      static void
   copy_buffer(struct radv_cmd_buffer *cmd_buffer, struct radv_buffer *src_buffer, struct radv_buffer *dst_buffer,
         {
               /* VK_EXT_conditional_rendering says that copy commands should not be
   * affected by conditional rendering.
   */
   old_predicating = cmd_buffer->state.predicating;
            radv_copy_buffer(cmd_buffer, src_buffer->bo, dst_buffer->bo, src_buffer->offset + region->srcOffset,
            /* Restore conditional rendering. */
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdCopyBuffer2(VkCommandBuffer commandBuffer, const VkCopyBufferInfo2 *pCopyBufferInfo)
   {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_buffer, src_buffer, pCopyBufferInfo->srcBuffer);
            for (unsigned r = 0; r < pCopyBufferInfo->regionCount; r++) {
            }
      void
   radv_update_buffer_cp(struct radv_cmd_buffer *cmd_buffer, uint64_t va, const void *data, uint64_t size)
   {
      uint64_t words = size / 4;
                     si_emit_cache_flush(cmd_buffer);
            radeon_emit(cmd_buffer->cs, PKT3(PKT3_WRITE_DATA, 2 + words, 0));
   radeon_emit(cmd_buffer->cs,
         radeon_emit(cmd_buffer->cs, va);
   radeon_emit(cmd_buffer->cs, va >> 32);
            if (unlikely(cmd_buffer->device->trace_bo))
      }
      VKAPI_ATTR void VKAPI_CALL
   radv_CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset, VkDeviceSize dataSize,
         {
      RADV_FROM_HANDLE(radv_cmd_buffer, cmd_buffer, commandBuffer);
   RADV_FROM_HANDLE(radv_buffer, dst_buffer, dstBuffer);
   uint64_t va = radv_buffer_get_va(dst_buffer->bo);
            assert(!(dataSize & 3));
            if (!dataSize)
            if (dataSize < RADV_BUFFER_UPDATE_THRESHOLD) {
      radv_cs_add_buffer(cmd_buffer->device->ws, cmd_buffer->cs, dst_buffer->bo);
      } else {
      uint32_t buf_offset;
   radv_cmd_buffer_upload_data(cmd_buffer, dataSize, pData, &buf_offset);
   radv_copy_buffer(cmd_buffer, cmd_buffer->upload.upload_bo, dst_buffer->bo, buf_offset,
         }
