   /*
   * Copyright © 2022 Collabora, LTD
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
      #include "vk_pipeline.h"
      #include "vk_device.h"
   #include "vk_log.h"
   #include "vk_nir.h"
   #include "vk_shader_module.h"
   #include "vk_util.h"
      #include "nir_serialize.h"
      #include "util/mesa-sha1.h"
   #include "util/mesa-blake3.h"
      bool
   vk_pipeline_shader_stage_is_null(const VkPipelineShaderStageCreateInfo *info)
   {
      if (info->module != VK_NULL_HANDLE)
            vk_foreach_struct_const(ext, info->pNext) {
      if (ext->sType == VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO ||
      ext->sType == VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_MODULE_IDENTIFIER_CREATE_INFO_EXT)
               }
      static nir_shader *
   get_builtin_nir(const VkPipelineShaderStageCreateInfo *info)
   {
               nir_shader *nir = NULL;
   if (module != NULL) {
         } else {
      const VkPipelineShaderStageNirCreateInfoMESA *nir_info =
         if (nir_info != NULL)
               if (nir == NULL)
            assert(nir->info.stage == vk_to_mesa_shader_stage(info->stage));
   ASSERTED nir_function_impl *entrypoint = nir_shader_get_entrypoint(nir);
   assert(strcmp(entrypoint->function->name, info->pName) == 0);
               }
      static uint32_t
   get_required_subgroup_size(const VkPipelineShaderStageCreateInfo *info)
   {
      const VkPipelineShaderStageRequiredSubgroupSizeCreateInfo *rss_info =
      vk_find_struct_const(info->pNext,
         }
      VkResult
   vk_pipeline_shader_stage_to_nir(struct vk_device *device,
                           {
      VK_FROM_HANDLE(vk_shader_module, module, info->module);
                     nir_shader *builtin_nir = get_builtin_nir(info);
   if (builtin_nir != NULL) {
               nir_shader *clone = nir_shader_clone(mem_ctx, builtin_nir);
   if (clone == NULL)
            assert(clone->options == NULL || clone->options == nir_options);
            *nir_out = clone;
               const uint32_t *spirv_data;
   uint32_t spirv_size;
   if (module != NULL) {
      spirv_data = (uint32_t *)module->data;
      } else {
      const VkShaderModuleCreateInfo *minfo =
         if (unlikely(minfo == NULL)) {
      return vk_errorf(device, VK_ERROR_UNKNOWN,
      }
   spirv_data = minfo->pCode;
               enum gl_subgroup_size subgroup_size;
   uint32_t req_subgroup_size = get_required_subgroup_size(info);
   if (req_subgroup_size > 0) {
      assert(util_is_power_of_two_nonzero(req_subgroup_size));
   assert(req_subgroup_size >= 8 && req_subgroup_size <= 128);
      } else if (info->flags & VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT ||
            /* Starting with SPIR-V 1.6, varying subgroup size the default */
      } else if (info->flags & VK_PIPELINE_SHADER_STAGE_CREATE_REQUIRE_FULL_SUBGROUPS_BIT) {
      assert(stage == MESA_SHADER_COMPUTE);
      } else {
                  nir_shader *nir = vk_spirv_to_nir(device, spirv_data, spirv_size, stage,
                                 if (nir == NULL)
                        }
      void
   vk_pipeline_hash_shader_stage(const VkPipelineShaderStageCreateInfo *info,
               {
               const nir_shader *builtin_nir = get_builtin_nir(info);
   if (builtin_nir != NULL) {
      /* Internal NIR module: serialize and hash the NIR shader.
   * We don't need to hash other info fields since they should match the
   * NIR data.
   */
            blob_init(&blob);
   nir_serialize(&blob, builtin_nir, false);
   assert(!blob.out_of_memory);
   _mesa_sha1_compute(blob.data, blob.size, stage_sha1);
   blob_finish(&blob);
               const VkShaderModuleCreateInfo *minfo =
         const VkPipelineShaderStageModuleIdentifierCreateInfoEXT *iinfo =
                                       assert(util_bitcount(info->stage) == 1);
            if (module) {
         } else if (minfo) {
               _mesa_blake3_compute(minfo->pCode, minfo->codeSize, spirv_hash);
      } else {
      /* It is legal to pass in arbitrary identifiers as long as they don't exceed
   * the limit. Shaders with bogus identifiers are more or less guaranteed to fail. */
   assert(iinfo);
   assert(iinfo->identifierSize <= VK_MAX_SHADER_MODULE_IDENTIFIER_SIZE_EXT);
               if (rstate) {
      _mesa_sha1_update(&ctx, &rstate->storage_buffers, sizeof(rstate->storage_buffers));
   _mesa_sha1_update(&ctx, &rstate->uniform_buffers, sizeof(rstate->uniform_buffers));
   _mesa_sha1_update(&ctx, &rstate->vertex_inputs, sizeof(rstate->vertex_inputs));
                        if (info->pSpecializationInfo) {
      _mesa_sha1_update(&ctx, info->pSpecializationInfo->pMapEntries,
               _mesa_sha1_update(&ctx, info->pSpecializationInfo->pData,
               uint32_t req_subgroup_size = get_required_subgroup_size(info);
               }
      static VkPipelineRobustnessBufferBehaviorEXT
   vk_device_default_robust_buffer_behavior(const struct vk_device *device)
   {
      if (device->enabled_features.robustBufferAccess2) {
         } else if (device->enabled_features.robustBufferAccess) {
         } else {
            }
      static VkPipelineRobustnessImageBehaviorEXT
   vk_device_default_robust_image_behavior(const struct vk_device *device)
   {
      if (device->enabled_features.robustImageAccess2) {
         } else if (device->enabled_features.robustImageAccess) {
         } else {
            }
      void
   vk_pipeline_robustness_state_fill(const struct vk_device *device,
                     {
      rs->uniform_buffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT;
   rs->storage_buffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT;
   rs->vertex_inputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT;
            const VkPipelineRobustnessCreateInfoEXT *shader_info =
      vk_find_struct_const(shader_stage_pNext,
      if (shader_info) {
      rs->storage_buffers = shader_info->storageBuffers;
   rs->uniform_buffers = shader_info->uniformBuffers;
   rs->vertex_inputs = shader_info->vertexInputs;
      } else {
      const VkPipelineRobustnessCreateInfoEXT *pipeline_info =
      vk_find_struct_const(pipeline_pNext,
      if (pipeline_info) {
      rs->storage_buffers = pipeline_info->storageBuffers;
   rs->uniform_buffers = pipeline_info->uniformBuffers;
   rs->vertex_inputs = pipeline_info->vertexInputs;
                  if (rs->storage_buffers ==
      VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT)
         if (rs->uniform_buffers ==
      VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT)
         if (rs->vertex_inputs ==
      VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT)
         if (rs->images == VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT_EXT)
      }
