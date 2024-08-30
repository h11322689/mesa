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
   state->gfx.object_preemption = true;
            memcpy(state->gfx.dyn_state.dirty,
            }
      static void
   anv_cmd_pipeline_state_finish(struct anv_cmd_buffer *cmd_buffer,
         {
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
      VkResult
   anv_create_companion_rcs_command_buffer(struct anv_cmd_buffer *cmd_buffer)
   {
      VkResult result = VK_SUCCESS;
   pthread_mutex_lock(&cmd_buffer->device->mutex);
   if (cmd_buffer->companion_rcs_cmd_buffer == NULL) {
      VK_FROM_HANDLE(vk_command_pool, pool,
                  struct vk_command_buffer *tmp_cmd_buffer = NULL;
   result = pool->command_buffer_ops->create(pool, &tmp_cmd_buffer);
   if (result != VK_SUCCESS) {
      pthread_mutex_unlock(&cmd_buffer->device->mutex);
               cmd_buffer->companion_rcs_cmd_buffer =
         cmd_buffer->companion_rcs_cmd_buffer->vk.level = cmd_buffer->vk.level;
      }
               }
      static VkResult
   anv_create_cmd_buffer(struct vk_command_pool *pool,
         {
      struct anv_device *device =
         struct anv_cmd_buffer *cmd_buffer;
            cmd_buffer = vk_zalloc(&pool->alloc, sizeof(*cmd_buffer), 8,
         if (cmd_buffer == NULL)
            result = vk_command_buffer_init(pool, &cmd_buffer->vk,
         if (result != VK_SUCCESS)
            cmd_buffer->vk.dynamic_graphics_state.ms.sample_locations =
         cmd_buffer->vk.dynamic_graphics_state.vi =
            cmd_buffer->batch.status = VK_SUCCESS;
                     assert(pool->queue_family_index < device->physical->queue.family_count);
   cmd_buffer->queue_family =
            result = anv_cmd_buffer_init_batch_bo_chain(cmd_buffer);
   if (result != VK_SUCCESS)
            anv_state_stream_init(&cmd_buffer->surface_state_stream,
         anv_state_stream_init(&cmd_buffer->dynamic_state_stream,
         anv_state_stream_init(&cmd_buffer->general_state_stream,
         anv_state_stream_init(&cmd_buffer->push_descriptor_stream,
            int success = u_vector_init_pow2(&cmd_buffer->dynamic_bos, 8,
         if (!success)
            cmd_buffer->self_mod_locations = NULL;
   cmd_buffer->companion_rcs_cmd_buffer = NULL;
            cmd_buffer->generation.jump_addr = ANV_NULL_ADDRESS;
                     memset(&cmd_buffer->generation.shader_state, 0,
                                                      fail_batch_bo:
         fail_vk:
         fail_alloc:
                  }
      static void
   destroy_cmd_buffer(struct anv_cmd_buffer *cmd_buffer)
   {
                                 anv_state_stream_finish(&cmd_buffer->surface_state_stream);
   anv_state_stream_finish(&cmd_buffer->dynamic_state_stream);
   anv_state_stream_finish(&cmd_buffer->general_state_stream);
            while (u_vector_length(&cmd_buffer->dynamic_bos) > 0) {
      struct anv_bo **bo = u_vector_remove(&cmd_buffer->dynamic_bos);
   anv_bo_pool_free((*bo)->map != NULL ?
            }
                              vk_command_buffer_finish(&cmd_buffer->vk);
      }
      static void
   anv_cmd_buffer_destroy(struct vk_command_buffer *vk_cmd_buffer)
   {
      struct anv_cmd_buffer *cmd_buffer =
                  pthread_mutex_lock(&device->mutex);
   if (cmd_buffer->companion_rcs_cmd_buffer) {
      destroy_cmd_buffer(cmd_buffer->companion_rcs_cmd_buffer);
               destroy_cmd_buffer(cmd_buffer);
      }
      static void
   reset_cmd_buffer(struct anv_cmd_buffer *cmd_buffer,
         {
               cmd_buffer->usage_flags = 0;
   cmd_buffer->perf_query_pool = NULL;
   cmd_buffer->is_companion_rcs_cmd_buffer = false;
   anv_cmd_buffer_reset_batch_bo_chain(cmd_buffer);
            memset(&cmd_buffer->generation.shader_state, 0,
            cmd_buffer->generation.jump_addr = ANV_NULL_ADDRESS;
            anv_state_stream_finish(&cmd_buffer->surface_state_stream);
   anv_state_stream_init(&cmd_buffer->surface_state_stream,
            anv_state_stream_finish(&cmd_buffer->dynamic_state_stream);
   anv_state_stream_init(&cmd_buffer->dynamic_state_stream,
            anv_state_stream_finish(&cmd_buffer->general_state_stream);
   anv_state_stream_init(&cmd_buffer->general_state_stream,
            anv_state_stream_finish(&cmd_buffer->push_descriptor_stream);
   anv_state_stream_init(&cmd_buffer->push_descriptor_stream,
            while (u_vector_length(&cmd_buffer->dynamic_bos) > 0) {
      struct anv_bo **bo = u_vector_remove(&cmd_buffer->dynamic_bos);
                        u_trace_fini(&cmd_buffer->trace);
      }
      void
   anv_cmd_buffer_reset(struct vk_command_buffer *vk_cmd_buffer,
         {
      struct anv_cmd_buffer *cmd_buffer =
            if (cmd_buffer->companion_rcs_cmd_buffer) {
      reset_cmd_buffer(cmd_buffer->companion_rcs_cmd_buffer, flags);
   destroy_cmd_buffer(cmd_buffer->companion_rcs_cmd_buffer);
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
   anv_cmd_buffer_mark_image_fast_cleared(struct anv_cmd_buffer *cmd_buffer,
                     {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
   anv_genX(devinfo, set_fast_clear_state)(cmd_buffer, image, format,
      }
      void
   anv_cmd_buffer_load_clear_color_from_image(struct anv_cmd_buffer *cmd_buffer,
               {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
      }
      void
   anv_cmd_emit_conditional_render_predicate(struct anv_cmd_buffer *cmd_buffer)
   {
      const struct intel_device_info *devinfo = cmd_buffer->device->info;
      }
      static void
   clear_pending_query_bits(enum anv_query_bits *query_bits,
         {
      if (flushed_bits & ANV_PIPE_RENDER_TARGET_CACHE_FLUSH_BIT)
            if (flushed_bits & ANV_PIPE_TILE_CACHE_FLUSH_BIT)
            if ((flushed_bits & ANV_PIPE_DATA_CACHE_FLUSH_BIT) &&
      (flushed_bits & ANV_PIPE_HDC_PIPELINE_FLUSH_BIT) &&
   (flushed_bits & ANV_PIPE_UNTYPED_DATAPORT_CACHE_FLUSH_BIT))
         /* Once RT/TILE have been flushed, we can consider the CS_STALL flush */
   if ((*query_bits & (ANV_QUERY_WRITES_TILE_FLUSH |
                  (flushed_bits & (ANV_PIPE_END_OF_PIPE_SYNC_BIT | ANV_PIPE_CS_STALL_BIT)))
   }
      void
   anv_cmd_buffer_update_pending_query_bits(struct anv_cmd_buffer *cmd_buffer,
         {
      clear_pending_query_bits(&cmd_buffer->state.queries.clear_bits, flushed_bits);
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
      static void
   anv_cmd_buffer_set_ray_query_buffer(struct anv_cmd_buffer *cmd_buffer,
                     {
               uint64_t ray_shadow_size =
      align64(brw_rt_ray_queries_shadow_stacks_size(device->info,
            if (ray_shadow_size > 0 &&
      (!cmd_buffer->state.ray_query_shadow_bo ||
   cmd_buffer->state.ray_query_shadow_bo->size < ray_shadow_size)) {
   unsigned shadow_size_log2 = MAX2(util_logbase2_ceil(ray_shadow_size), 16);
   unsigned bucket = shadow_size_log2 - 16;
            struct anv_bo *bo = p_atomic_read(&device->ray_query_shadow_bos[bucket]);
   if (bo == NULL) {
      struct anv_bo *new_bo;
   VkResult result = anv_device_alloc_bo(device, "RT queries shadow",
                           if (result != VK_SUCCESS) {
      anv_batch_set_error(&cmd_buffer->batch, result);
               bo = p_atomic_cmpxchg(&device->ray_query_shadow_bos[bucket], NULL, new_bo);
   if (bo != NULL) {
         } else {
            }
            /* Add the ray query buffers to the batch list. */
   anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
               /* Add the HW buffer to the list of BO used. */
   anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
            /* Fill the push constants & mark them dirty. */
   struct anv_state ray_query_global_state =
            struct anv_address ray_query_globals_addr =
      anv_state_pool_state_address(&device->dynamic_state_pool,
      pipeline_state->push_constants.ray_query_globals =
            }
      /**
   * This function compute changes between 2 pipelines and flags the dirty HW
   * state appropriately.
   */
   static void
   anv_cmd_buffer_flush_pipeline_state(struct anv_cmd_buffer *cmd_buffer,
               {
      struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
         #define diff_fix_state(bit, name)                                       \
      do {                                                                 \
      /* Fixed states should always have matching sizes */              \
   assert(old_pipeline == NULL ||                                    \
         /* Don't bother memcmp if the state is already dirty */           \
   if (!BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_##bit) &&         \
      (old_pipeline == NULL ||                                      \
   memcmp(&old_pipeline->batch_data[old_pipeline->name.offset], \
                  #define diff_var_state(bit, name)                                       \
      do {                                                                 \
      /* Don't bother memcmp if the state is already dirty */           \
   /* Also if the new state is empty, avoid marking dirty */         \
   if (!BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_##bit) &&         \
      new_pipeline->name.len != 0 &&                                \
   (old_pipeline == NULL ||                                      \
   old_pipeline->name.len != new_pipeline->name.len ||          \
   memcmp(&old_pipeline->batch_data[old_pipeline->name.offset], \
                  #define assert_identical(bit, name)                                     \
      do {                                                                 \
      /* Fixed states should always have matching sizes */              \
   assert(old_pipeline == NULL ||                                    \
         assert(old_pipeline == NULL ||                                    \
         memcmp(&old_pipeline->batch_data[old_pipeline->name.offset], \
         #define assert_empty(name) assert(new_pipeline->name.len == 0)
         /* Compare all states, including partial packed ones, the dynamic part is
   * left at 0 but the static part could still change.
   */
   diff_fix_state(URB,                      final.urb);
   diff_fix_state(VF_SGVS,                  final.vf_sgvs);
   if (cmd_buffer->device->info->ver >= 11)
         if (cmd_buffer->device->info->ver >= 12)
         diff_fix_state(SBE,                      final.sbe);
   diff_fix_state(SBE_SWIZ,                 final.sbe_swiz);
   diff_fix_state(MULTISAMPLE,              final.ms);
   diff_fix_state(VS,                       final.vs);
   diff_fix_state(HS,                       final.hs);
   diff_fix_state(DS,                       final.ds);
   diff_fix_state(PS,                       final.ps);
            diff_fix_state(CLIP,                     partial.clip);
   diff_fix_state(SF,                       partial.sf);
   diff_fix_state(RASTER,                   partial.raster);
   diff_fix_state(WM,                       partial.wm);
   diff_fix_state(STREAMOUT,                partial.so);
   diff_fix_state(GS,                       partial.gs);
   diff_fix_state(TE,                       partial.te);
            if (cmd_buffer->device->vk.enabled_extensions.EXT_mesh_shader) {
      diff_fix_state(TASK_CONTROL,          final.task_control);
   diff_fix_state(TASK_SHADER,           final.task_shader);
   diff_fix_state(TASK_REDISTRIB,        final.task_redistrib);
   diff_fix_state(MESH_CONTROL,          final.mesh_control);
   diff_fix_state(MESH_SHADER,           final.mesh_shader);
   diff_fix_state(MESH_DISTRIB,          final.mesh_distrib);
   diff_fix_state(CLIP_MESH,             final.clip_mesh);
      } else {
      assert_empty(final.task_control);
   assert_empty(final.task_shader);
   assert_empty(final.task_redistrib);
   assert_empty(final.mesh_control);
   assert_empty(final.mesh_shader);
   assert_empty(final.mesh_distrib);
   assert_empty(final.clip_mesh);
               /* States that should never vary between pipelines, but can be affected by
   * blorp etc...
   */
            /* States that can vary in length */
   diff_var_state(VF_SGVS_INSTANCING,       final.vf_sgvs_instancing);
         #undef diff_fix_state
   #undef diff_var_state
   #undef assert_identical
   #undef assert_empty
         /* We're not diffing the following :
   *    - anv_graphics_pipeline::vertex_input_data
   *    - anv_graphics_pipeline::final::vf_instancing
   *
   * since they are tracked by the runtime.
      }
      void anv_CmdBindPipeline(
      VkCommandBuffer                             commandBuffer,
   VkPipelineBindPoint                         pipelineBindPoint,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_pipeline, pipeline, _pipeline);
   struct anv_cmd_pipeline_state *state;
            switch (pipelineBindPoint) {
   case VK_PIPELINE_BIND_POINT_COMPUTE: {
      struct anv_compute_pipeline *compute_pipeline =
         if (cmd_buffer->state.compute.pipeline == compute_pipeline)
            cmd_buffer->state.compute.base.pipeline = pipeline;
   cmd_buffer->state.compute.pipeline = compute_pipeline;
   cmd_buffer->state.compute.pipeline_dirty = true;
   set_dirty_for_bind_map(cmd_buffer, MESA_SHADER_COMPUTE,
            state = &cmd_buffer->state.compute.base;
   stages = VK_SHADER_STAGE_COMPUTE_BIT;
               case VK_PIPELINE_BIND_POINT_GRAPHICS: {
      struct anv_graphics_pipeline *old_pipeline =
         struct anv_graphics_pipeline *new_pipeline =
         if (old_pipeline == new_pipeline)
            cmd_buffer->state.gfx.base.pipeline = pipeline;
   cmd_buffer->state.gfx.pipeline = new_pipeline;
            anv_foreach_stage(stage, new_pipeline->base.base.active_stages) {
      set_dirty_for_bind_map(cmd_buffer, stage,
               /* Apply the non dynamic state from the pipeline */
   vk_cmd_set_dynamic_graphics_state(&cmd_buffer->vk,
            state = &cmd_buffer->state.gfx.base;
               /* When the pipeline is using independent states and dynamic buffers,
   * this will trigger an update of anv_push_constants::dynamic_base_index
   * & anv_push_constants::dynamic_offsets.
   */
   struct anv_push_constants *push =
         struct anv_pipeline_sets_layout *layout = &new_pipeline->base.base.layout;
   if (layout->independent_sets && layout->num_dynamic_buffers > 0) {
      bool modified = false;
   for (uint32_t s = 0; s < layout->num_sets; s++) {
                     assert(layout->set[s].dynamic_offset_start < MAX_DYNAMIC_BUFFERS);
   if (layout->set[s].layout->dynamic_offset_count > 0 &&
      (push->desc_offsets[s] & ANV_DESCRIPTOR_SET_DYNAMIC_INDEX_MASK) != layout->set[s].dynamic_offset_start) {
   push->desc_offsets[s] &= ~ANV_DESCRIPTOR_SET_DYNAMIC_INDEX_MASK;
   push->desc_offsets[s] |= (layout->set[s].dynamic_offset_start &
               }
   if (modified)
               if ((new_pipeline->fs_msaa_flags & BRW_WM_MSAA_FLAG_ENABLE_DYNAMIC) &&
      push->gfx.fs_msaa_flags != new_pipeline->fs_msaa_flags) {
   push->gfx.fs_msaa_flags = new_pipeline->fs_msaa_flags;
      }
   if (new_pipeline->dynamic_patch_control_points) {
      cmd_buffer->state.push_constants_dirty |=
               anv_cmd_buffer_flush_pipeline_state(cmd_buffer, old_pipeline, new_pipeline);
               case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR: {
      struct anv_ray_tracing_pipeline *rt_pipeline =
         if (cmd_buffer->state.rt.pipeline == rt_pipeline)
            cmd_buffer->state.rt.base.pipeline = pipeline;
   cmd_buffer->state.rt.pipeline = rt_pipeline;
            if (rt_pipeline->stack_size > 0) {
      anv_CmdSetRayTracingPipelineStackSizeKHR(commandBuffer,
               state = &cmd_buffer->state.rt.base;
               default:
      unreachable("invalid bind point");
               if (pipeline->ray_queries > 0)
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
   *     VK_DESCRIPTOR_POOL_CREATE_HOST_ONLY_BIT_EXT flag set"
   */
            struct anv_descriptor_set_layout *set_layout = set->layout;
   VkShaderStageFlags stages = set_layout->shader_stages;
            switch (bind_point) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS:
      stages &= VK_SHADER_STAGE_ALL_GRAPHICS |
            (cmd_buffer->device->vk.enabled_extensions.EXT_mesh_shader ?
      pipe_state = &cmd_buffer->state.gfx.base;
         case VK_PIPELINE_BIND_POINT_COMPUTE:
      stages &= VK_SHADER_STAGE_COMPUTE_BIT;
   pipe_state = &cmd_buffer->state.compute.base;
         case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
      stages &= VK_SHADER_STAGE_RAYGEN_BIT_KHR |
            VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
   VK_SHADER_STAGE_MISS_BIT_KHR |
      pipe_state = &cmd_buffer->state.rt.base;
         default:
                  VkShaderStageFlags dirty_stages = 0;
   /* If it's a push descriptor set, we have to flag things as dirty
   * regardless of whether or not the CPU-side data structure changed as we
   * may have edited in-place.
   */
   if (pipe_state->descriptors[set_index] != set ||
                     /* When using indirect descriptors, stages that have access to the HW
   * binding tables, never need to access the
   * anv_push_constants::desc_offsets fields, because any data they need
   * from the descriptor buffer is accessible through a binding table
   * entry. For stages that are "bindless" (Mesh/Task/RT), we need to
   * provide anv_push_constants::desc_offsets matching the bound
   * descriptor so that shaders can access the descriptor buffer through
   * A64 messages.
   *
   * With direct descriptors, the shaders can use the
   * anv_push_constants::desc_offsets to build bindless offsets. So it's
   * we always need to update the push constant data.
   */
   bool update_desc_sets =
      !cmd_buffer->device->physical->indirect_descriptors ||
   (stages & (VK_SHADER_STAGE_TASK_BIT_EXT |
            VK_SHADER_STAGE_MESH_BIT_EXT |
   VK_SHADER_STAGE_RAYGEN_BIT_KHR |
   VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
            if (update_desc_sets) {
               struct anv_address set_addr = anv_descriptor_set_address(set);
   uint64_t offset =
      anv_address_physical(set_addr) -
      assert((offset & ~ANV_DESCRIPTOR_SET_OFFSET_MASK) == 0);
                  if (set_addr.bo) {
      anv_reloc_list_add_bo(cmd_buffer->batch.relocs,
                              if (dynamic_offsets) {
      if (set_layout->dynamic_offset_count > 0) {
      struct anv_push_constants *push = &pipe_state->push_constants;
   uint32_t dynamic_offset_start =
                        memcpy(pipe_state->dynamic_offsets[set_index].offsets,
                        /* Assert that everything is in range */
   assert(set_layout->dynamic_offset_count <= *dynamic_offset_count);
                  for (uint32_t i = 0; i < set_layout->dynamic_offset_count; i++) {
      if (push_offsets[i] != (*dynamic_offsets)[i]) {
      pipe_state->dynamic_offsets[set_index].offsets[i] =
         /* dynamic_offset_stages[] elements could contain blanket
   * values like VK_SHADER_STAGE_ALL, so limit this to the
   * binding point's bits.
   */
                  *dynamic_offsets += set_layout->dynamic_offset_count;
                  if (set->is_push)
         else
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
   ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, _layout);
                     for (uint32_t i = 0; i < descriptorSetCount; i++) {
      ANV_FROM_HANDLE(anv_descriptor_set, set, pDescriptorSets[i]);
   if (set == NULL)
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
   struct anv_cmd_pipeline_state *pipe_state = &cmd_buffer->state.compute.base;
   struct anv_push_constants *data = &pipe_state->push_constants;
   struct anv_compute_pipeline *pipeline = cmd_buffer->state.compute.pipeline;
   const struct brw_cs_prog_data *cs_prog_data = get_cs_prog_data(pipeline);
            const struct brw_cs_dispatch_info dispatch =
         const unsigned total_push_constants_size =
         if (total_push_constants_size == 0)
            const unsigned push_constant_alignment = 64;
   const unsigned aligned_total_push_constants_size =
         struct anv_state state;
   if (devinfo->verx10 >= 125) {
      state = anv_state_stream_alloc(&cmd_buffer->general_state_stream,
            } else {
      state = anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
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
               if (stageFlags & (VK_SHADER_STAGE_ALL_GRAPHICS |
                  struct anv_cmd_pipeline_state *pipe_state =
               }
   if (stageFlags & VK_SHADER_STAGE_COMPUTE_BIT) {
      struct anv_cmd_pipeline_state *pipe_state =
               }
   if (stageFlags & (VK_SHADER_STAGE_RAYGEN_BIT_KHR |
                     VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
      struct anv_cmd_pipeline_state *pipe_state =
                           }
      static struct anv_cmd_pipeline_state *
   anv_cmd_buffer_get_pipe_state(struct anv_cmd_buffer *cmd_buffer,
         {
      switch (bind_point) {
   case VK_PIPELINE_BIND_POINT_GRAPHICS:
         case VK_PIPELINE_BIND_POINT_COMPUTE:
         case VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR:
      return &cmd_buffer->state.rt.base;
      default:
            }
      void anv_CmdPushDescriptorSetKHR(
      VkCommandBuffer commandBuffer,
   VkPipelineBindPoint pipelineBindPoint,
   VkPipelineLayout _layout,
   uint32_t _set,
   uint32_t descriptorWriteCount,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, _layout);
                              struct anv_push_descriptor_set *push_set =
      &anv_cmd_buffer_get_pipe_state(cmd_buffer,
               anv_descriptor_set_write(cmd_buffer->device, &push_set->set,
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
         ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, _layout);
                              struct anv_push_descriptor_set *push_set =
      &anv_cmd_buffer_get_pipe_state(cmd_buffer,
               anv_descriptor_set_write_template(cmd_buffer->device, &push_set->set,
                  anv_cmd_buffer_bind_descriptor_set(cmd_buffer, template->bind_point,
            }
      void anv_CmdSetRayTracingPipelineStackSizeKHR(
      VkCommandBuffer                             commandBuffer,
      {
      ANV_FROM_HANDLE(anv_cmd_buffer, cmd_buffer, commandBuffer);
   struct anv_cmd_ray_tracing_state *rt = &cmd_buffer->state.rt;
            if (anv_batch_has_error(&cmd_buffer->batch))
                     unsigned stack_size_log2 = util_logbase2_ceil(pipelineStackSize);
   if (stack_size_log2 < 10)
            if (rt->scratch.layout.total_size == 1 << stack_size_log2)
            brw_rt_compute_scratch_layout(&rt->scratch.layout, device->info,
            unsigned bucket = stack_size_log2 - 10;
            struct anv_bo *bo = p_atomic_read(&device->rt_scratch_bos[bucket]);
   if (bo == NULL) {
      struct anv_bo *new_bo;
   VkResult result = anv_device_alloc_bo(device, "RT scratch",
                           if (result != VK_SUCCESS) {
      rt->scratch.layout.total_size = 0;
   anv_batch_set_error(&cmd_buffer->batch, result);
               bo = p_atomic_cmpxchg(&device->rt_scratch_bos[bucket], NULL, new_bo);
   if (bo != NULL) {
         } else {
                        }
      void
   anv_cmd_buffer_save_state(struct anv_cmd_buffer *cmd_buffer,
               {
               /* we only support the compute pipeline at the moment */
   assert(state->flags & ANV_CMD_SAVED_STATE_COMPUTE_PIPELINE);
   const struct anv_cmd_pipeline_state *pipe_state =
            if (state->flags & ANV_CMD_SAVED_STATE_COMPUTE_PIPELINE)
            if (state->flags & ANV_CMD_SAVED_STATE_DESCRIPTOR_SET_0)
            if (state->flags & ANV_CMD_SAVED_STATE_PUSH_CONSTANTS) {
      memcpy(state->push_constants, pipe_state->push_constants.client_data,
         }
      void
   anv_cmd_buffer_restore_state(struct anv_cmd_buffer *cmd_buffer,
         {
               assert(state->flags & ANV_CMD_SAVED_STATE_COMPUTE_PIPELINE);
   const VkPipelineBindPoint bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
   const VkShaderStageFlags stage_flags = VK_SHADER_STAGE_COMPUTE_BIT;
   struct anv_cmd_compute_state *comp_state = &cmd_buffer->state.compute;
            if (state->flags & ANV_CMD_SAVED_STATE_COMPUTE_PIPELINE) {
      if (state->pipeline) {
      anv_CmdBindPipeline(cmd_buffer_, bind_point,
      } else {
      comp_state->pipeline = NULL;
                  if (state->flags & ANV_CMD_SAVED_STATE_DESCRIPTOR_SET_0) {
      if (state->descriptor_set) {
      anv_cmd_buffer_bind_descriptor_set(cmd_buffer, bind_point, NULL, 0,
      } else {
                     if (state->flags & ANV_CMD_SAVED_STATE_PUSH_CONSTANTS) {
      anv_CmdPushConstants(cmd_buffer_, VK_NULL_HANDLE, stage_flags, 0,
               }
