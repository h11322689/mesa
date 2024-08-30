   /*
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include <assert.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "anv_private.h"
   #include "anv_measure.h"
      #include "vk_util.h"
      /** \file anv_cmd_buffer.c
   *
   * This file contains all of the stuff for emitting commands into a command
   * buffer.  This includes implementations of most of the vkCmd*
   * entrypoints.  This file is concerned entirely with state emission and
   * not with the command buffer data structure itself.  As far as this file
   * is concerned, most of anv_cmd_buffer is magic.
   */
      static void
   anv_cmd_state_init(struct anv_cmd_buffer *cmd_buffer)
   {
                        state->current_pipeline = UINT32_MAX;
   state->gfx.restart_index = UINT32_MAX;
      }
      static void
   anv_cmd_pipeline_state_finish(struct anv_cmd_buffer *cmd_buffer,
         {
      for (uint32_t i = 0; i < ARRAY_SIZE(pipe_state->push_descriptors); i++) {
      if (pipe_state->push_descriptors[i]) {
      anv_descriptor_set_layout_unref(cmd_buffer->device,
                  }
      static void
   anv_cmd_state_finish(struct anv_cmd_buffer *cmd_buffer)
   {
               anv_cmd_pipeline_state_finish(cmd_buffer, &state->gfx.base);
      }
      static void
   anv_cmd_state_reset(struct anv_cmd_buffer *cmd_buffer)
   {
      anv_cmd_state_finish(cmd_buffer);
      }
      static VkResult
   anv_create_cmd_buffer(struct vk_command_pool *pool,
         {
      struct anv_device *device =
         struct anv_cmd_buffer *cmd_buffer;
            cmd_buffer = vk_alloc(&pool->alloc, sizeof(*cmd_buffer), 8,
         if (cmd_buffer == NULL)
            result = vk_command_buffer_init(pool, &cmd_buffer->vk,
         if (result != VK_SUCCESS)
            cmd_buffer->vk.dynamic_graphics_state.ms.sample_locations =
                              assert(pool->queue_family_index < device->physical->queue.family_count);
   cmd_buffer->queue_family =
            result = anv_cmd_buffer_init_batch_bo_chain(cmd_buffer);
   if (result != VK_SUCCESS)
            anv_state_stream_init(&cmd_buffer->surface_state_stream,
         anv_state_stream_init(&cmd_buffer->dynamic_state_stream,
         anv_state_stream_init(&cmd_buffer->general_state_stream,
                                                               fail_vk:
         fail_alloc:
                  }
      static void
   anv_cmd_buffer_destroy(struct vk_command_buffer *vk_cmd_buffer)
   {
      struct anv_cmd_buffer *cmd_buffer =
                                       anv_state_stream_finish(&cmd_buffer->surface_state_stream);
   anv_state_stream_finish(&cmd_buffer->dynamic_state_stream);
                              vk_command_buffer_finish(&cmd_buffer->vk);
      }
      void
   anv_cmd_buffer_reset(struct vk_command_buffer *vk_cmd_buffer,
         {
      struct anv_cmd_buffer *cmd_buffer =
                     cmd_buffer->usage_flags = 0;
   cmd_buffer->perf_query_pool = NULL;
   anv_cmd_buffer_reset_batch_bo_chain(cmd_buffer);
            anv_state_stream_finish(&cmd_buffer->surface_state_stream);
   anv_state_stream_init(&cmd_buffer->surface_state_stream,
            anv_state_stream_finish(&cmd_buffer->dynamic_state_stream);
   anv_state_stream_init(&cmd_buffer->dynamic_state_stream,
            anv_state_stream_finish(&cmd_buffer->general_state_stream);
   anv_state_stream_init(&cmd_buffer->general_state_stream,
                     u_trace_fini(&cmd_buffer->trace);
      }
      const struct vk_command_buffer_ops anv_cmd_buffer_ops = {
      .create = anv_create_cmd_buffer,
   .reset = anv_cmd_buffer_reset,
      };
      void
   anv_cmd_buffer_emit_state_base_address(struct anv_cmd_buffer *cmd_buffer)
   {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
      }
      void
   anv_cmd_buffer_mark_image_written(struct anv_cmd_buffer *cmd_buffer,
                                       {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   anv_genX(devinfo, cmd_buffer_mark_image_written)(cmd_buffer, image,
                  }
      void
   anv_cmd_emit_conditional_render_predicate(struct anv_cmd_buffer *cmd_buffer)
   {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
      }
      static bool
   mem_update(void *dst, const void *src, size_t size)
   {
      if (memcmp(dst, src, size) == 0)
            memcpy(dst, src, size);
      }
      static void
   set_dirty_for_bind_map(struct anv_cmd_buffer *cmd_buffer,
               {
      assert(stage < ARRAY_SIZE(cmd_buffer->state.surface_sha1s));
   if (mem_update(cmd_buffer->state.surface_sha1s[stage],
                  assert(stage < ARRAY_SIZE(cmd_buffer->state.sampler_sha1s));
   if (mem_update(cmd_buffer->state.sampler_sha1s[stage],
                  assert(stage < ARRAY_SIZE(cmd_buffer->state.push_sha1s));
   if (mem_update(cmd_buffer->state.push_sha1s[stage],
            }
      static inline uint32_t
   ilog2_round_up(uint32_t value)
   {
      assert(value != 0);
      }
      void anv_CmdBindPipeline(
      VkCommandBuffer                             commandBuffer,
   VkPipelineBindPoint                         pipelineBindPoint,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            switch (pipelineBindPoint) {
   case VK_PIPELINE_BIND_POINT_COMPUTE: {
      struct anv_compute_pipeline *compute_pipeline =
         if (cmd_buffer->state.compute.pipeline == compute_pipeline)
            cmd_buffer->state.compute.pipeline = compute_pipeline;
   cmd_buffer->state.compute.pipeline_dirty = true;
   set_dirty_for_bind_map(cmd_buffer, MESA_SHADER_COMPUTE,
                     case VK_PIPELINE_BIND_POINT_GRAPHICS: {
      struct anv_graphics_pipeline *gfx_pipeline =
         if (cmd_buffer->state.gfx.pipeline == gfx_pipeline)
            cmd_buffer->state.gfx.pipeline = gfx_pipeline;
   cmd_buffer->state.gfx.vb_dirty |= gfx_pipeline->vb_used;
            anv_foreach_stage(stage, gfx_pipeline->active_stages) {
      set_dirty_for_bind_map(cmd_buffer, stage,
               /* Apply the non dynamic state from the pipeline */
   vk_cmd_set_dynamic_graphics_state(&cmd_buffer->vk,
                     default:
      unreachable("invalid bind point");
         }
      static void
   anv_cmd_buffer_bind_descriptor_set(struct anv_cmd_buffer *cmd_buffer,
                                       {
      /* Either we have no pool because it's a push descriptor or the pool is not
   * host only :
   *
   * VUID-vkCmdBindDescriptorSets-pDescriptorSets-04616:
   *
   *    "Each element of pDescriptorSets must not have been allocated from a
   *     VkDescriptorPool with the
   *     VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_VALVE flag set"
   */
            struct anv_descriptor_set_layout *set_layout =
            VkShaderStageFlags stages = set_layout->shader_stages;
            switch (bind_point) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS:
      stages &= VK_SHADER_STAGE_ALL_GRAPHICS;
   pipe_state = &cmd_buffer->state.gfx.base;
         case VK_PIPELINE_BIND_POINT_COMPUTE:
      stages &= VK_SHADER_STAGE_COMPUTE_BIT;
   pipe_state = &cmd_buffer->state.compute.base;
         default:
                  VkShaderStageFlags dirty_stages = 0;
   /* If it's a push descriptor set, we have to flag things as dirty
   * regardless of whether or not the CPU-side data structure changed as we
   * may have edited in-place.
   */
   if (pipe_state->descriptors[set_index] != set ||
            pipe_state->descriptors[set_index] = set;
               if (dynamic_offsets) {
      if (set_layout->dynamic_offset_count > 0) {
      struct anv_push_constants *push = &pipe_state->push_constants;
   uint32_t dynamic_offset_start =
                        /* Assert that everything is in range */
   assert(set_layout->dynamic_offset_count <= *dynamic_offset_count);
                  for (uint32_t i = 0; i < set_layout->dynamic_offset_count; i++) {
      if (push_offsets[i] != (*dynamic_offsets)[i]) {
      push_offsets[i] = (*dynamic_offsets)[i];
   /* dynamic_offset_stages[] elements could contain blanket
   * values like VK_SHADER_STAGE_ALL, so limit this to the
   * binding point's bits.
   */
                  *dynamic_offsets += set_layout->dynamic_offset_count;
                  cmd_buffer->state.descriptors_dirty |= dirty_stages;
      }
      void anv_CmdBindDescriptorSets(
      VkCommandBuffer                             commandBuffer,
   VkPipelineBindPoint                         pipelineBindPoint,
   VkPipelineLayout                            _layout,
   uint32_t                                    firstSet,
   uint32_t                                    descriptorSetCount,
   const VkDescriptorSet*                      pDescriptorSets,
   uint32_t                                    dynamicOffsetCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
                     for (uint32_t i = 0; i < descriptorSetCount; i++) {
      ANV_FROM_HANDLE(anv_descriptor_set, set, pDescriptorSets[i]);
   anv_cmd_buffer_bind_descriptor_set(cmd_buffer, pipelineBindPoint,
                     }
      void anv_CmdBindVertexBuffers2(
      VkCommandBuffer                              commandBuffer,
   uint32_t                                     firstBinding,
   uint32_t                                     bindingCount,
   const VkBuffer*                              pBuffers,
   const VkDeviceSize*                          pOffsets,
   const VkDeviceSize*                          pSizes,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            /* We have to defer setting up vertex buffer since we need the buffer
            assert(firstBinding + bindingCount <= MAX_VBS);
   for (uint32_t i = 0; i < bindingCount; i++) {
               if (buffer == NULL) {
      vb[firstBinding + i] = (struct anv_vertex_binding) {
            } else {
      vb[firstBinding + i] = (struct anv_vertex_binding) {
      .buffer = buffer,
   .offset = pOffsets[i],
   .size = vk_buffer_range(&buffer->vk, pOffsets[i],
         }
               if (pStrides != NULL) {
      vk_cmd_set_vertex_binding_strides(&cmd_buffer->vk, firstBinding,
         }
      void anv_CmdBindTransformFeedbackBuffersEXT(
      VkCommandBuffer                             commandBuffer,
   uint32_t                                    firstBinding,
   uint32_t                                    bindingCount,
   const VkBuffer*                             pBuffers,
   const VkDeviceSize*                         pOffsets,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
            /* We have to defer setting up vertex buffer since we need the buffer
            assert(firstBinding + bindingCount <= MAX_XFB_BUFFERS);
   for (uint32_t i = 0; i < bindingCount; i++) {
      if (pBuffers[i] == VK_NULL_HANDLE) {
         } else {
      ANV_FROM_HANDLE(anv_buffer, buffer, pBuffers[i]);
   xfb[firstBinding + i].buffer = buffer;
   xfb[firstBinding + i].offset = pOffsets[i];
   xfb[firstBinding + i].size =
      vk_buffer_range(&buffer->vk, pOffsets[i],
         }
      enum isl_format
   anv_isl_format_for_descriptor_type(const struct anv_device *device,
         {
      switch (type) {
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
      return device->physical->compiler->indirect_ubos_use_sampler ?
         case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            default:
            }
      struct anv_state
   anv_cmd_buffer_emit_dynamic(struct anv_cmd_buffer *cmd_buffer,
         {
               state = anv_cmd_buffer_alloc_dynamic_state(cmd_buffer, size, alignment);
                        }
      struct anv_state
   anv_cmd_buffer_merge_dynamic(struct anv_cmd_buffer *cmd_buffer,
               {
      struct anv_state state;
            state = anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
         p = state.map;
   for (uint32_t i = 0; i < dwords; i++)
                        }
      struct anv_state
   anv_cmd_buffer_gfx_push_constants(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_push_constants *data =
            struct anv_state state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                        }
      struct anv_state
   anv_cmd_buffer_cs_push_constants(struct anv_cmd_buffer *cmd_buffer)
   {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   struct anv_push_constants *data =
         struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
   const struct brw_cs_prog_data *cs_prog_data = get_cs_prog_data(pipeline);
            const struct brw_cs_dispatch_info dispatch =
         const unsigned total_push_constants_size =
         if (total_push_constants_size == 0)
            const unsigned push_constant_alignment =
         const unsigned aligned_total_push_constants_size =
         struct anv_state state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
               void *dst = state.map;
            if (cs_prog_data->push.cross_thread.size > 0) {
      memcpy(dst, src, cs_prog_data->push.cross_thread.size);
   dst += cs_prog_data->push.cross_thread.size;
               if (cs_prog_data->push.per_thread.size > 0) {
      for (unsigned t = 0; t < dispatch.threads; t++) {
               uint32_t *subgroup_id = dst +
      offsetof(struct anv_push_constants, cs.subgroup_id) -
                                 }
      void anv_CmdPushConstants(
      VkCommandBuffer                             commandBuffer,
   VkPipelineLayout                            layout,
   VkShaderStageFlags                          stageFlags,
   uint32_t                                    offset,
   uint32_t                                    size,
      {
               if (stageFlags & VK_SHADER_STAGE_ALL_GRAPHICS) {
      struct anv_cmd_pipeline_state *pipe_state =
               }
   if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT) {
      struct anv_cmd_pipeline_state *pipe_state =
                           }
      static struct anv_descriptor_set *
   anv_cmd_buffer_push_descriptor_set(struct anv_cmd_buffer *cmd_buffer,
                     {
               switch (bind_point) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS:
      pipe_state = &cmd_buffer->state.gfx.base;
         case VK_PIPELINE_BIND_POINT_COMPUTE:
      pipe_state = &cmd_buffer->state.compute.base;
         default:
                  struct anv_push_descriptor_set **push_set =
            if (*push_set == NULL) {
      *push_set = vk_zalloc(&cmd_buffer->vk.pool->alloc,
               if (*push_set == NULL) {
      anv_batch_set_error(&cmd_buffer->batch, VK_ERROR_OUT_OF_HOST_MEMORY);
                           if (set->layout != layout) {
      if (set->layout)
         anv_descriptor_set_layout_ref(layout);
      }
   set->size = anv_descriptor_set_layout_size(layout, 0);
   set->buffer_view_count = layout->buffer_view_count;
   set->descriptor_count = layout->descriptor_count;
            if (layout->descriptor_buffer_size &&
      ((*push_set)->set_used_on_gpu ||
   set->desc_mem.alloc_size < layout->descriptor_buffer_size)) {
   /* The previous buffer is either actively used by some GPU command (so
   * we can't modify it) or is too small.  Allocate a new one.
   */
   struct anv_state desc_mem =
      anv_state_stream_alloc(&cmd_buffer->dynamic_state_stream,
            if (set->desc_mem.alloc_size) {
      /* TODO: Do we really need to copy all the time? */
   memcpy(desc_mem.map, set->desc_mem.map,
      }
            set->desc_addr = (struct anv_address) {
      .bo = cmd_buffer->dynamic_state_stream.state_pool->block_pool.bo,
               enum isl_format format =
                  const struct isl_device *isl_dev = &cmd_buffer->device->isl_dev;
   set->desc_surface_state =
      anv_state_stream_alloc(&cmd_buffer->surface_state_stream,
      anv_fill_buffer_surface_state(cmd_buffer->device,
                                          }
      void anv_CmdPushDescriptorSetKHR(
      VkCommandBuffer commandBuffer,
   VkPipelineBindPoint pipelineBindPoint,
   VkPipelineLayout _layout,
   uint32_t _set,
   uint32_t descriptorWriteCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
                              struct anv_descriptor_set *set =
      anv_cmd_buffer_push_descriptor_set(cmd_buffer, pipelineBindPoint,
      if (!set)
            /* Go through the user supplied descriptors. */
   for (uint32_t i = 0; i < descriptorWriteCount; i++) {
               switch (write->descriptorType) {
   case VK_DESCRIPTOR_TYPE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
      anv_descriptor_set_write_image_view(cmd_buffer->device, set,
                                    case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      for (uint32_t j = 0; j < write->descriptorCount; j++) {
                     anv_descriptor_set_write_buffer_view(cmd_buffer->device, set,
                                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
   case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
   case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        anv_descriptor_set_write_buffer(cmd_buffer->device, set,
                                 &cmd_buffer->surface_state_stream,
                  default:
                     anv_cmd_buffer_bind_descriptor_set(cmd_buffer, pipelineBindPoint,
      }
      void anv_CmdPushDescriptorSetWithTemplateKHR(
      VkCommandBuffer                             commandBuffer,
   VkDescriptorUpdateTemplate                  descriptorUpdateTemplate,
   VkPipelineLayout                            _layout,
   uint32_t                                    _set,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   VK_FROM_HANDLE(vk_descriptor_update_template, template,
                                    struct anv_descriptor_set *set =
      anv_cmd_buffer_push_descriptor_set(cmd_buffer, template->bind_point,
      if (!set)
            anv_descriptor_set_write_template(cmd_buffer->device, set,
                        anv_cmd_buffer_bind_descriptor_set(cmd_buffer, template->bind_point,
      }
