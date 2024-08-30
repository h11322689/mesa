   /*
   * Copyright © 2019 Raspberry Pi Ltd
   *
   * Based in part on v3d driver which is:
   *
   * Copyright © 2014-2017 Broadcom
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
      /* The only version specific structure that we need is
   * TMU_CONFIG_PARAMETER_1. This didn't seem to change significantly from
   * previous V3D versions and we don't expect that to change, so for now let's
   * just hardcode the V3D version here.
   */
   #define V3D_VERSION 41
   #include "broadcom/common/v3d_macros.h"
   #include "broadcom/cle/v3dx_pack.h"
      /* Our Vulkan resource indices represent indices in descriptor maps which
   * include all shader stages, so we need to size the arrays below
   * accordingly. For now we only support a maximum of 3 stages: VS, GS, FS.
   */
   #define MAX_STAGES 3
      #define MAX_TOTAL_TEXTURE_SAMPLERS (V3D_MAX_TEXTURE_SAMPLERS * MAX_STAGES)
   struct texture_bo_list {
         };
      /* This tracks state BOs for both textures and samplers, so we
   * multiply by 2.
   */
   #define MAX_TOTAL_STATES (2 * V3D_MAX_TEXTURE_SAMPLERS * MAX_STAGES)
   struct state_bo_list {
      uint32_t count;
      };
      #define MAX_TOTAL_UNIFORM_BUFFERS ((MAX_UNIFORM_BUFFERS + \
         #define MAX_TOTAL_STORAGE_BUFFERS (MAX_STORAGE_BUFFERS * MAX_STAGES)
   struct buffer_bo_list {
      struct v3dv_bo *ubo[MAX_TOTAL_UNIFORM_BUFFERS];
      };
      static bool
   state_bo_in_list(struct state_bo_list *list, struct v3dv_bo *bo)
   {
      for (int i = 0; i < list->count; i++) {
      if (list->states[i] == bo)
      }
      }
      static void
   push_constants_bo_free(VkDevice _device,
               {
      V3DV_FROM_HANDLE(v3dv_device, device, _device);
      }
      /*
   * This method checks if the ubo used for push constants is needed to be
   * updated or not.
   *
   * push constants ubo is only used for push constants accessed by a non-const
   * index.
   */
   static void
   check_push_constants_ubo(struct v3dv_cmd_buffer *cmd_buffer,
         {
      if (!(cmd_buffer->state.dirty & V3DV_CMD_DIRTY_PUSH_CONSTANTS_UBO) ||
      pipeline->layout->push_constant_size == 0)
         if (cmd_buffer->push_constants_resource.bo == NULL) {
      cmd_buffer->push_constants_resource.bo =
            v3dv_job_add_bo(cmd_buffer->state.job,
            if (!cmd_buffer->push_constants_resource.bo) {
      fprintf(stderr, "Failed to allocate memory for push constants\n");
               bool ok = v3dv_bo_map(cmd_buffer->device,
               if (!ok) {
      fprintf(stderr, "failed to map push constants buffer\n");
         } else {
      if (cmd_buffer->push_constants_resource.offset +
      cmd_buffer->state.push_constants_size <=
   cmd_buffer->push_constants_resource.bo->size) {
   cmd_buffer->push_constants_resource.offset +=
      } else {
      /* We ran out of space so we'll have to allocate a new buffer but we
   * need to ensure the old one is preserved until the end of the command
   * buffer life and make sure it is eventually freed. We use the
   * private object machinery in the command buffer for this.
   */
   v3dv_cmd_buffer_add_private_obj(
                  /* Now call back so we create a new BO */
   cmd_buffer->push_constants_resource.bo = NULL;
   check_push_constants_ubo(cmd_buffer, pipeline);
                  assert(cmd_buffer->state.push_constants_size <= MAX_PUSH_CONSTANTS_SIZE);
   memcpy(cmd_buffer->push_constants_resource.bo->map +
         cmd_buffer->push_constants_resource.offset,
               }
      /** V3D 4.x TMU configuration parameter 0 (texture) */
   static void
   write_tmu_p0(struct v3dv_cmd_buffer *cmd_buffer,
               struct v3dv_pipeline *pipeline,
   enum broadcom_shader_stage stage,
   struct v3dv_cl_out **uniforms,
   uint32_t data,
   {
               struct v3dv_descriptor_state *descriptor_state =
            /* We need to ensure that the texture bo is added to the job */
   struct v3dv_bo *texture_bo =
      v3dv_descriptor_map_get_texture_bo(descriptor_state,
            assert(texture_bo);
   assert(texture_idx < V3D_MAX_TEXTURE_SAMPLERS);
            struct v3dv_cl_reloc state_reloc =
      v3dv_descriptor_map_get_texture_shader_state(cmd_buffer->device, descriptor_state,
                     cl_aligned_u32(uniforms, state_reloc.bo->offset +
                  /* Texture and Sampler states are typically suballocated, so they are
   * usually the same BO: only flag them once to avoid trying to add them
   * multiple times to the job later.
   */
   if (!state_bo_in_list(state_bos, state_reloc.bo)) {
      assert(state_bos->count < 2 * V3D_MAX_TEXTURE_SAMPLERS);
         }
      /** V3D 4.x TMU configuration parameter 1 (sampler) */
   static void
   write_tmu_p1(struct v3dv_cmd_buffer *cmd_buffer,
               struct v3dv_pipeline *pipeline,
   enum broadcom_shader_stage stage,
   struct v3dv_cl_out **uniforms,
   {
      uint32_t sampler_idx = v3d_unit_data_get_unit(data);
   struct v3dv_descriptor_state *descriptor_state =
            assert(sampler_idx != V3DV_NO_SAMPLER_16BIT_IDX &&
            struct v3dv_cl_reloc sampler_state_reloc =
      v3dv_descriptor_map_get_sampler_state(cmd_buffer->device, descriptor_state,
               const struct v3dv_sampler *sampler =
      v3dv_descriptor_map_get_sampler(descriptor_state,
                     /* Set unnormalized coordinates flag from sampler object */
   uint32_t p1_packed = v3d_unit_data_get_offset(data);
   if (sampler->unnormalized_coordinates) {
      struct V3DX(TMU_CONFIG_PARAMETER_1) p1_unpacked;
   V3DX(TMU_CONFIG_PARAMETER_1_unpack)((uint8_t *)&p1_packed, &p1_unpacked);
   p1_unpacked.unnormalized_coordinates = true;
   V3DX(TMU_CONFIG_PARAMETER_1_pack)(NULL, (uint8_t *)&p1_packed,
               cl_aligned_u32(uniforms, sampler_state_reloc.bo->offset +
                  /* Texture and Sampler states are typically suballocated, so they are
   * usually the same BO: only flag them once to avoid trying to add them
   * multiple times to the job later.
   */
   if (!state_bo_in_list(state_bos, sampler_state_reloc.bo)) {
      assert(state_bos->count < 2 * V3D_MAX_TEXTURE_SAMPLERS);
         }
      static void
   write_ubo_ssbo_uniforms(struct v3dv_cmd_buffer *cmd_buffer,
                           struct v3dv_pipeline *pipeline,
   enum broadcom_shader_stage stage,
   {
      struct v3dv_descriptor_state *descriptor_state =
            struct v3dv_descriptor_map *map =
      content == QUNIFORM_UBO_ADDR || content == QUNIFORM_GET_UBO_SIZE ?
   &pipeline->shared_data->maps[stage]->ubo_map :
         uint32_t offset =
      content == QUNIFORM_UBO_ADDR ?
   v3d_unit_data_get_offset(data) :
                  /* For ubos, index is shifted, as 0 is reserved for push constants
   * and 1..MAX_INLINE_UNIFORM_BUFFERS are reserved for inline uniform
   * buffers.
   */
   uint32_t index = v3d_unit_data_get_unit(data);
   if (content == QUNIFORM_UBO_ADDR && index == 0) {
      /* Ensure the push constants UBO is created and updated. This also
   * adds the BO to the job so we don't need to track it in buffer_bos.
   */
            struct v3dv_cl_reloc *resource =
                  cl_aligned_u32(uniforms, resource->bo->offset +
            } else {
      if (content == QUNIFORM_UBO_ADDR) {
      /* We reserve UBO index 0 for push constants in Vulkan (and for the
   * constant buffer in GL) so the compiler always adds one to all UBO
   * indices, fix it up before we access the descriptor map, since
   * indices start from 0 there.
   */
   assert(index > 0);
      } else {
                  struct v3dv_descriptor *descriptor =
      v3dv_descriptor_map_get_descriptor(descriptor_state, map,
               /* Inline UBO descriptors store UBO data in descriptor pool memory,
   * instead of an external buffer.
   */
            if (content == QUNIFORM_GET_SSBO_SIZE ||
      content == QUNIFORM_GET_UBO_SIZE) {
      } else {
      /* Inline uniform buffers store their contents in pool memory instead
   * of an external buffer.
   */
   struct v3dv_bo *bo;
   uint32_t addr;
   if (descriptor->type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK) {
      assert(dynamic_offset == 0);
   struct v3dv_cl_reloc reloc =
      v3dv_descriptor_map_get_descriptor_bo(cmd_buffer->device,
                  bo = reloc.bo;
      } else {
      assert(descriptor->buffer);
                  bo = descriptor->buffer->mem->bo;
   addr = bo->offset +
         descriptor->buffer->mem_offset +
                        if (content == QUNIFORM_UBO_ADDR) {
      assert(index < MAX_TOTAL_UNIFORM_BUFFERS);
      } else {
      assert(index < MAX_TOTAL_STORAGE_BUFFERS);
               }
      static void
   write_inline_uniform(struct v3dv_cl_out **uniforms,
                        uint32_t index,
      {
               struct v3dv_descriptor_state *descriptor_state =
            struct v3dv_descriptor_map *map =
            struct v3dv_cl_reloc reloc =
      v3dv_descriptor_map_get_descriptor_bo(cmd_buffer->device,
                     /* Offset comes in 32-bit units */
   uint32_t *addr = reloc.bo->map + reloc.offset + 4 * offset;
      }
      static uint32_t
   get_texture_size_from_image_view(struct v3dv_image_view *image_view,
               {
      switch(contents) {
   case QUNIFORM_IMAGE_WIDTH:
   case QUNIFORM_TEXTURE_WIDTH:
      /* We don't u_minify the values, as we are using the image_view
   * extents
   */
      case QUNIFORM_IMAGE_HEIGHT:
   case QUNIFORM_TEXTURE_HEIGHT:
         case QUNIFORM_IMAGE_DEPTH:
   case QUNIFORM_TEXTURE_DEPTH:
         case QUNIFORM_IMAGE_ARRAY_SIZE:
   case QUNIFORM_TEXTURE_ARRAY_SIZE:
      if (image_view->vk.view_type != VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
         } else {
      assert(image_view->vk.layer_count % 6 == 0);
         case QUNIFORM_TEXTURE_LEVELS:
         case QUNIFORM_TEXTURE_SAMPLES:
      assert(image_view->vk.image);
      default:
            }
         static uint32_t
   get_texture_size_from_buffer_view(struct v3dv_buffer_view *buffer_view,
               {
      switch(contents) {
   case QUNIFORM_IMAGE_WIDTH:
   case QUNIFORM_TEXTURE_WIDTH:
         /* Only size can be queried for texel buffers  */
   default:
            }
      static uint32_t
   get_texture_size(struct v3dv_cmd_buffer *cmd_buffer,
                  struct v3dv_pipeline *pipeline,
      {
               struct v3dv_descriptor_state *descriptor_state =
            struct v3dv_descriptor *descriptor =
      v3dv_descriptor_map_get_descriptor(descriptor_state,
                              switch (descriptor->type) {
   case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
   case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
   case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
   case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
      return get_texture_size_from_image_view(descriptor->image_view,
      case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
   case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
      return get_texture_size_from_buffer_view(descriptor->buffer_view,
      default:
            }
      struct v3dv_cl_reloc
   v3dv_write_uniforms_wg_offsets(struct v3dv_cmd_buffer *cmd_buffer,
                     {
      struct v3d_uniform_list *uinfo =
                  struct v3dv_job *job = cmd_buffer->state.job;
   assert(job);
            struct texture_bo_list tex_bos = { 0 };
   struct state_bo_list state_bos = { 0 };
            /* The hardware always pre-fetches the next uniform (also when there
   * aren't any), so we always allocate space for an extra slot. This
   * fixes MMU exceptions reported since Linux kernel 5.4 when the
   * uniforms fill up the tail bytes of a page in the indirect
   * BO. In that scenario, when the hardware pre-fetches after reading
   * the last uniform it will read beyond the end of the page and trigger
   * the MMU exception.
   */
                     struct v3dv_cl_out *uniforms = cl_start(&job->indirect);
   for (int i = 0; i < uinfo->count; i++) {
               switch (uinfo->contents[i]) {
   case QUNIFORM_CONSTANT:
                  case QUNIFORM_UNIFORM:
                  case QUNIFORM_INLINE_UBO_0:
   case QUNIFORM_INLINE_UBO_1:
   case QUNIFORM_INLINE_UBO_2:
   case QUNIFORM_INLINE_UBO_3:
      write_inline_uniform(&uniforms,
                     case QUNIFORM_VIEWPORT_X_SCALE: {
      float clipper_xy_granularity = V3DV_X(cmd_buffer->device, CLIPPER_XY_GRANULARITY);
   cl_aligned_f(&uniforms, dynamic->viewport.scale[0][0] * clipper_xy_granularity);
               case QUNIFORM_VIEWPORT_Y_SCALE: {
      float clipper_xy_granularity = V3DV_X(cmd_buffer->device, CLIPPER_XY_GRANULARITY);
   cl_aligned_f(&uniforms, dynamic->viewport.scale[0][1] * clipper_xy_granularity);
               case QUNIFORM_VIEWPORT_Z_OFFSET: {
      float translate_z;
   v3dv_cmd_buffer_state_get_viewport_z_xform(&cmd_buffer->state, 0,
         cl_aligned_f(&uniforms, translate_z);
               case QUNIFORM_VIEWPORT_Z_SCALE: {
      float scale_z;
   v3dv_cmd_buffer_state_get_viewport_z_xform(&cmd_buffer->state, 0,
         cl_aligned_f(&uniforms, scale_z);
               case QUNIFORM_SSBO_OFFSET:
   case QUNIFORM_UBO_ADDR:
   case QUNIFORM_GET_SSBO_SIZE:
   case QUNIFORM_GET_UBO_SIZE:
                           case QUNIFORM_IMAGE_TMU_CONFIG_P0:
   case QUNIFORM_TMU_CONFIG_P0:
      write_tmu_p0(cmd_buffer, pipeline, variant->stage,
               case QUNIFORM_TMU_CONFIG_P1:
      write_tmu_p1(cmd_buffer, pipeline, variant->stage,
               case QUNIFORM_IMAGE_WIDTH:
   case QUNIFORM_IMAGE_HEIGHT:
   case QUNIFORM_IMAGE_DEPTH:
   case QUNIFORM_IMAGE_ARRAY_SIZE:
   case QUNIFORM_TEXTURE_WIDTH:
   case QUNIFORM_TEXTURE_HEIGHT:
   case QUNIFORM_TEXTURE_DEPTH:
   case QUNIFORM_TEXTURE_ARRAY_SIZE:
   case QUNIFORM_TEXTURE_LEVELS:
   case QUNIFORM_TEXTURE_SAMPLES:
      cl_aligned_u32(&uniforms,
                  get_texture_size(cmd_buffer,
                  /* We generate this from geometry shaders to cap the generated gl_Layer
   * to be within the number of layers of the framebuffer so we prevent the
   * binner from trying to access tile state memory out of bounds (for
   * layers that don't exist).
   *
   * Unfortunately, for secondary command buffers we may not know the
   * number of layers in the framebuffer at this stage. Since we are
   * only using this to sanitize the shader and it should not have any
   * impact on correct shaders that emit valid values for gl_Layer,
   * we just work around it by using the largest number of layers we
   * support.
   *
   * FIXME: we could do better than this by recording in the job that
   * the value at this uniform offset is not correct, and patch it when
   * we execute the secondary command buffer into a primary, since we do
   * have the correct number of layers at that point, but again, since this
   * is only for sanityzing the shader and it only affects the specific case
   * of secondary command buffers without framebuffer info available it
   * might not be worth the trouble.
   *
   * With multiview the number of layers is dictated by the view mask
   * and not by the framebuffer layers. We do set the job's frame tiling
   * information correctly from the view mask in that case, however,
   * secondary command buffers may not have valid frame tiling data,
   * so when multiview is enabled, we always set the number of layers
   * from the subpass view mask.
   */
   case QUNIFORM_FB_LAYERS: {
      const struct v3dv_cmd_buffer_state *state = &job->cmd_buffer->state;
                  uint32_t num_layers;
   if (view_mask != 0) {
         } else if (job->frame_tiling.layers != 0) {
         } else if (cmd_buffer->state.framebuffer) {
         } else {
      #if DEBUG
               #endif
            }
   cl_aligned_u32(&uniforms, num_layers);
               case QUNIFORM_VIEW_INDEX:
                  case QUNIFORM_NUM_WORK_GROUPS:
      assert(job->type == V3DV_JOB_TYPE_GPU_CSD);
   assert(job->csd.wg_count[data] > 0);
   if (wg_count_offsets)
                     case QUNIFORM_WORK_GROUP_BASE:
      assert(job->type == V3DV_JOB_TYPE_GPU_CSD);
               case QUNIFORM_SHARED_OFFSET:
      assert(job->type == V3DV_JOB_TYPE_GPU_CSD);
   assert(job->csd.shared_memory);
               case QUNIFORM_SPILL_OFFSET:
      assert(pipeline->spill.bo);
               case QUNIFORM_SPILL_SIZE_PER_THREAD:
      assert(pipeline->spill.size_per_thread > 0);
               default:
                              for (int i = 0; i < MAX_TOTAL_TEXTURE_SAMPLERS; i++) {
      if (tex_bos.tex[i])
               for (int i = 0; i < state_bos.count; i++)
            for (int i = 0; i < MAX_TOTAL_UNIFORM_BUFFERS; i++) {
      if (buffer_bos.ubo[i])
               for (int i = 0; i < MAX_TOTAL_STORAGE_BUFFERS; i++) {
      if (buffer_bos.ssbo[i])
               if (job->csd.shared_memory)
            if (pipeline->spill.bo)
               }
      struct v3dv_cl_reloc
   v3dv_write_uniforms(struct v3dv_cmd_buffer *cmd_buffer,
               {
         }
