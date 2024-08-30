   /*
   * Copyright © 2021 Google
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
      #include "nir/nir.h"
   #include "nir/nir_builder.h"
      #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "vk_pipeline.h"
      struct rt_handle_hash_entry {
      uint32_t key;
      };
      static uint32_t
   handle_from_stages(struct radv_device *device, struct radv_ray_tracing_stage *stages, unsigned stage_count,
         {
      struct mesa_sha1 ctx;
            for (uint32_t i = 0; i < stage_count; i++)
            unsigned char hash[20];
            uint32_t ret;
            /* Leave the low half for resume shaders etc. */
            /* Ensure we have dedicated space for replayable shaders */
   ret &= ~(1u << 30);
                     struct hash_entry *he = NULL;
   for (;;) {
      he = _mesa_hash_table_search(device->rt_handles, &ret);
   if (!he)
            if (memcmp(he->data, hash, sizeof(hash)) == 0)
                        if (!he) {
      struct rt_handle_hash_entry *e = ralloc(device->rt_handles, struct rt_handle_hash_entry);
   e->key = ret;
   memcpy(e->hash, hash, sizeof(e->hash));
                           }
      static struct radv_pipeline_key
   radv_generate_rt_pipeline_key(const struct radv_device *device, const struct radv_ray_tracing_pipeline *pipeline,
         {
      struct radv_pipeline_key key = radv_generate_pipeline_key(device, pCreateInfo->pStages, pCreateInfo->stageCount,
            if (pCreateInfo->pLibraryInfo) {
      for (unsigned i = 0; i < pCreateInfo->pLibraryInfo->libraryCount; ++i) {
      RADV_FROM_HANDLE(radv_pipeline, pipeline_lib, pCreateInfo->pLibraryInfo->pLibraries[i]);
   struct radv_ray_tracing_pipeline *library_pipeline = radv_pipeline_to_ray_tracing(pipeline_lib);
   /* apply shader robustness from merged shaders */
                  if (library_pipeline->traversal_uniform_robustness2)
                     }
      static VkResult
   radv_create_group_handles(struct radv_device *device, const struct radv_ray_tracing_pipeline *pipeline,
               {
      bool capture_replay =
         for (unsigned i = 0; i < pCreateInfo->groupCount; ++i) {
      const VkRayTracingShaderGroupCreateInfoKHR *group_info = &pCreateInfo->pGroups[i];
   switch (group_info->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
      if (group_info->generalShader != VK_SHADER_UNUSED_KHR)
                     case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
      if (group_info->closestHitShader != VK_SHADER_UNUSED_KHR)
                  if (group_info->intersectionShader != VK_SHADER_UNUSED_KHR) {
                                                }
      case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
      if (group_info->closestHitShader != VK_SHADER_UNUSED_KHR)
                  if (group_info->anyHitShader != VK_SHADER_UNUSED_KHR)
                     case VK_SHADER_GROUP_SHADER_MAX_ENUM_KHR:
                  if (group_info->pShaderGroupCaptureReplayHandle) {
      const struct radv_rt_capture_replay_handle *handle = group_info->pShaderGroupCaptureReplayHandle;
   if (memcmp(&handle->non_recursive_idx, &groups[i].handle.any_hit_index, sizeof(uint32_t)) != 0) {
                           }
      static VkResult
   radv_rt_fill_group_info(struct radv_device *device, const struct radv_ray_tracing_pipeline *pipeline,
                     {
               uint32_t idx;
   for (idx = 0; idx < pCreateInfo->groupCount; idx++) {
      groups[idx].type = pCreateInfo->pGroups[idx].type;
   if (groups[idx].type == VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR)
         else
         groups[idx].any_hit_shader = pCreateInfo->pGroups[idx].anyHitShader;
            if (pCreateInfo->pGroups[idx].pShaderGroupCaptureReplayHandle) {
                     if (groups[idx].recursive_shader < pCreateInfo->stageCount) {
         } else if (groups[idx].recursive_shader != VK_SHADER_UNUSED_KHR) {
      assert(stages[groups[idx].recursive_shader].shader);
   struct radv_shader *library_shader =
         simple_mtx_lock(&library_shader->replay_mtx);
   if (!library_shader->has_replay_alloc) {
      union radv_shader_arena_block *new_block =
         if (!new_block) {
                                                                     if (!radv_shader_reupload(device, library_shader)) {
      result = VK_ERROR_UNKNOWN;
               reloc_out:
      simple_mtx_unlock(&library_shader->replay_mtx);
   if (result != VK_SUCCESS)
                     /* copy and adjust library groups (incl. handles) */
   if (pCreateInfo->pLibraryInfo) {
      unsigned stage_count = pCreateInfo->stageCount;
   for (unsigned i = 0; i < pCreateInfo->pLibraryInfo->libraryCount; ++i) {
                     for (unsigned j = 0; j < library_pipeline->group_count; ++j) {
      struct radv_ray_tracing_group *dst = &groups[idx + j];
   *dst = library_pipeline->groups[j];
   if (dst->recursive_shader != VK_SHADER_UNUSED_KHR)
         if (dst->any_hit_shader != VK_SHADER_UNUSED_KHR)
         if (dst->intersection_shader != VK_SHADER_UNUSED_KHR)
         /* Don't set the shader VA since the handles are part of the pipeline hash */
      }
   idx += library_pipeline->group_count;
                     }
      static void
   radv_rt_fill_stage_info(const VkRayTracingPipelineCreateInfoKHR *pCreateInfo, struct radv_ray_tracing_stage *stages)
   {
      uint32_t idx;
   for (idx = 0; idx < pCreateInfo->stageCount; idx++)
            if (pCreateInfo->pLibraryInfo) {
      for (unsigned i = 0; i < pCreateInfo->pLibraryInfo->libraryCount; ++i) {
      RADV_FROM_HANDLE(radv_pipeline, pipeline, pCreateInfo->pLibraryInfo->pLibraries[i]);
   struct radv_ray_tracing_pipeline *library_pipeline = radv_pipeline_to_ray_tracing(pipeline);
   for (unsigned j = 0; j < library_pipeline->stage_count; ++j) {
      if (library_pipeline->stages[j].nir)
                        stages[idx].stage = library_pipeline->stages[j].stage;
   stages[idx].stack_size = library_pipeline->stages[j].stack_size;
   memcpy(stages[idx].sha1, library_pipeline->stages[j].sha1, SHA1_DIGEST_LENGTH);
               }
      static void
   radv_init_rt_stage_hashes(struct radv_device *device, const VkRayTracingPipelineCreateInfoKHR *pCreateInfo,
         {
               for (uint32_t idx = 0; idx < pCreateInfo->stageCount; idx++) {
      struct radv_shader_stage stage;
                  }
      static VkRayTracingPipelineCreateInfoKHR
   radv_create_merged_rt_create_info(const VkRayTracingPipelineCreateInfoKHR *pCreateInfo)
   {
      VkRayTracingPipelineCreateInfoKHR local_create_info = *pCreateInfo;
   uint32_t total_stages = pCreateInfo->stageCount;
            if (pCreateInfo->pLibraryInfo) {
      for (unsigned i = 0; i < pCreateInfo->pLibraryInfo->libraryCount; ++i) {
                     total_stages += library_pipeline->stage_count;
         }
   local_create_info.stageCount = total_stages;
               }
      static bool
   should_move_rt_instruction(nir_intrinsic_op intrinsic)
   {
      switch (intrinsic) {
   case nir_intrinsic_load_hit_attrib_amd:
   case nir_intrinsic_load_rt_arg_scratch_offset_amd:
   case nir_intrinsic_load_ray_flags:
   case nir_intrinsic_load_ray_object_origin:
   case nir_intrinsic_load_ray_world_origin:
   case nir_intrinsic_load_ray_t_min:
   case nir_intrinsic_load_ray_object_direction:
   case nir_intrinsic_load_ray_world_direction:
   case nir_intrinsic_load_ray_t_max:
         default:
            }
      static void
   move_rt_instructions(nir_shader *shader)
   {
               nir_foreach_block (block, nir_shader_get_entrypoint(shader)) {
      nir_foreach_instr_safe (instr, block) {
                                                               }
      static VkResult
   radv_rt_nir_to_asm(struct radv_device *device, struct vk_pipeline_cache *cache,
                     const VkRayTracingPipelineCreateInfoKHR *pCreateInfo, const struct radv_pipeline_key *pipeline_key,
   {
      struct radv_shader_binary *binary;
   bool keep_executable_info = radv_pipeline_capture_shaders(device, pipeline->base.base.create_flags);
            /* Gather shader info. */
   nir_shader_gather_info(stage->nir, nir_shader_get_entrypoint(stage->nir));
   radv_nir_shader_info_init(stage->stage, MESA_SHADER_NONE, &stage->info);
   radv_nir_shader_info_pass(device, stage->nir, &stage->layout, pipeline_key, RADV_PIPELINE_RAY_TRACING, false,
            /* Declare shader arguments. */
            stage->info.user_sgprs_locs = stage->args.user_sgprs_locs;
            /* Move ray tracing system values to the top that are set by rt_trace_ray
   * to prevent them from being overwritten by other rt_trace_ray calls.
   */
            uint32_t num_resume_shaders = 0;
            if (stage->stage != MESA_SHADER_INTERSECTION && !monolithic) {
      nir_builder b = nir_builder_at(nir_after_impl(nir_shader_get_entrypoint(stage->nir)));
            const nir_lower_shader_calls_options opts = {
      .address_format = nir_address_format_32bit_offset,
   .stack_alignment = 16,
   .localized_loads = true,
   .vectorizer_callback = radv_mem_vectorize_callback,
      };
               unsigned num_shaders = num_resume_shaders + 1;
   nir_shader **shaders = ralloc_array(stage->nir, nir_shader *, num_shaders);
   if (!shaders)
            shaders[0] = stage->nir;
   for (uint32_t i = 0; i < num_resume_shaders; i++)
            /* Postprocess shader parts. */
   for (uint32_t i = 0; i < num_shaders; i++) {
      struct radv_shader_stage temp_stage = *stage;
   temp_stage.nir = shaders[i];
   radv_nir_lower_rt_abi(temp_stage.nir, pCreateInfo, &temp_stage.args, &stage->info, stack_size, i > 0, device,
            /* Info might be out-of-date after inlining in radv_nir_lower_rt_abi(). */
            radv_optimize_nir(temp_stage.nir, pipeline_key->optimisations_disabled);
            if (radv_can_dump_shader(device, temp_stage.nir, false))
               bool dump_shader = radv_can_dump_shader(device, shaders[0], false);
   bool replayable =
            /* Compile NIR shader to AMD assembly. */
   binary = radv_shader_nir_to_asm(device, stage, shaders, num_shaders, pipeline_key, keep_executable_info,
         struct radv_shader *shader;
   if (replay_block || replayable) {
      VkResult result = radv_shader_create_uncached(device, binary, replayable, replay_block, &shader);
   if (result != VK_SUCCESS) {
      free(binary);
         } else
            if (shader) {
      radv_shader_generate_debug_info(device, dump_shader, keep_executable_info, binary, shader, shaders, num_shaders,
            if (shader && keep_executable_info && stage->spirv.size) {
      shader->spirv = malloc(stage->spirv.size);
   memcpy(shader->spirv, stage->spirv.data, stage->spirv.size);
                           *out_shader = shader;
      }
      static bool
   radv_rt_can_inline_shader(nir_shader *nir)
   {
      if (nir->info.stage == MESA_SHADER_RAYGEN || nir->info.stage == MESA_SHADER_ANY_HIT ||
      nir->info.stage == MESA_SHADER_INTERSECTION)
         if (nir->info.stage == MESA_SHADER_CALLABLE)
            nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
                     if (nir_instr_as_intrinsic(instr)->intrinsic == nir_intrinsic_trace_ray)
                     }
      static VkResult
   radv_rt_compile_shaders(struct radv_device *device, struct vk_pipeline_cache *cache,
                           {
               if (pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR)
                           struct radv_shader_stage *stages = calloc(pCreateInfo->stageCount, sizeof(struct radv_shader_stage));
   if (!stages)
            bool monolithic = !(pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR);
   for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
      if (rt_stages[i].shader || rt_stages[i].nir)
                     struct radv_shader_stage *stage = &stages[i];
            /* precompile the shader */
                                 bool has_callable = false;
   for (uint32_t i = 0; i < pipeline->stage_count; i++) {
      has_callable |= rt_stages[i].stage == MESA_SHADER_CALLABLE;
               for (uint32_t idx = 0; idx < pCreateInfo->stageCount; idx++) {
      if (rt_stages[idx].shader || rt_stages[idx].nir)
                              /* Cases in which we need to keep around the NIR:
   *    - pipeline library: The final pipeline might be monolithic in which case it will need every NIR shader.
   *                        If there is a callable shader, we can be sure that the final pipeline won't be
   *                        monolithic.
   *    - non-recursive:    Non-recursive shaders are inlined into the traversal shader.
   *    - monolithic:       Callable shaders (chit/miss) are inlined into the raygen shader.
   */
   bool compiled = radv_ray_tracing_stage_is_compiled(&rt_stages[idx]);
   bool library = pCreateInfo->flags & VK_PIPELINE_CREATE_LIBRARY_BIT_KHR;
   bool nir_needed =
         nir_needed &= !rt_stages[idx].nir;
   if (nir_needed) {
      rt_stages[idx].stack_size = stage->nir->scratch_size;
   rt_stages[idx].nir = radv_pipeline_cache_nir_to_handle(device, cache, stage->nir, rt_stages[idx].sha1,
                           for (uint32_t idx = 0; idx < pCreateInfo->stageCount; idx++) {
      int64_t stage_start = os_time_get_nano();
            /* Cases in which we need to compile the shader (raygen/callable/chit/miss):
   *    TODO: - monolithic: Extend the loop to cover imported stages and force compilation of imported raygen
   *                        shaders since pipeline library shaders use separate compilation.
   *    - separate:   Compile any recursive stage if wasn't compiled yet.
   * TODO: Skip chit and miss shaders in the monolithic case.
   */
   bool shader_needed = radv_ray_tracing_stage_is_compiled(&rt_stages[idx]) && !rt_stages[idx].shader;
   if (shader_needed) {
      uint32_t stack_size = 0;
                           struct radv_shader *shader;
   result = radv_rt_nir_to_asm(device, cache, pCreateInfo, key, pipeline, monolithic_raygen, stage, &stack_size,
                        assert(rt_stages[idx].stack_size <= stack_size);
   rt_stages[idx].stack_size = stack_size;
               if (creation_feedback && creation_feedback->pipelineStageCreationFeedbackCount) {
      assert(idx < creation_feedback->pipelineStageCreationFeedbackCount);
   stage->feedback.duration += os_time_get_nano() - stage_start;
                  if (pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR)
            /* create traversal shader */
   struct vk_shader_module traversal_module = {
      .base.type = VK_OBJECT_TYPE_SHADER_MODULE,
      };
   const VkPipelineShaderStageCreateInfo pStage = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
   .stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
   .module = vk_shader_module_to_handle(&traversal_module),
      };
   struct radv_shader_stage traversal_stage = {
      .stage = MESA_SHADER_INTERSECTION,
      };
   vk_pipeline_hash_shader_stage(&pStage, NULL, traversal_stage.shader_sha1);
   radv_shader_layout_init(pipeline_layout, MESA_SHADER_INTERSECTION, &traversal_stage.layout);
   result = radv_rt_nir_to_asm(device, cache, pCreateInfo, key, pipeline, false, &traversal_stage, NULL, NULL,
               cleanup:
      for (uint32_t i = 0; i < pCreateInfo->stageCount; i++)
         free(stages);
      }
      static bool
   radv_rt_pipeline_has_dynamic_stack_size(const VkRayTracingPipelineCreateInfoKHR *pCreateInfo)
   {
      if (!pCreateInfo->pDynamicState)
            for (unsigned i = 0; i < pCreateInfo->pDynamicState->dynamicStateCount; ++i) {
      if (pCreateInfo->pDynamicState->pDynamicStates[i] == VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR)
                  }
      static void
   compute_rt_stack_size(const VkRayTracingPipelineCreateInfoKHR *pCreateInfo, struct radv_ray_tracing_pipeline *pipeline)
   {
      if (radv_rt_pipeline_has_dynamic_stack_size(pCreateInfo)) {
      pipeline->stack_size = -1u;
               unsigned raygen_size = 0;
   unsigned callable_size = 0;
   unsigned chit_miss_size = 0;
   unsigned intersection_size = 0;
            for (unsigned i = 0; i < pipeline->stage_count; ++i) {
      uint32_t size = pipeline->stages[i].stack_size;
   switch (pipeline->stages[i].stage) {
   case MESA_SHADER_RAYGEN:
      raygen_size = MAX2(raygen_size, size);
      case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
      chit_miss_size = MAX2(chit_miss_size, size);
      case MESA_SHADER_CALLABLE:
      callable_size = MAX2(callable_size, size);
      case MESA_SHADER_INTERSECTION:
      intersection_size = MAX2(intersection_size, size);
      case MESA_SHADER_ANY_HIT:
      any_hit_size = MAX2(any_hit_size, size);
      default:
            }
   pipeline->stack_size =
      raygen_size +
   MIN2(pCreateInfo->maxPipelineRayRecursionDepth, 1) * MAX2(chit_miss_size, intersection_size + any_hit_size) +
   }
      static void
   combine_config(struct ac_shader_config *config, struct ac_shader_config *other)
   {
      config->num_sgprs = MAX2(config->num_sgprs, other->num_sgprs);
   config->num_vgprs = MAX2(config->num_vgprs, other->num_vgprs);
   config->num_shared_vgprs = MAX2(config->num_shared_vgprs, other->num_shared_vgprs);
   config->spilled_sgprs = MAX2(config->spilled_sgprs, other->spilled_sgprs);
   config->spilled_vgprs = MAX2(config->spilled_vgprs, other->spilled_vgprs);
   config->lds_size = MAX2(config->lds_size, other->lds_size);
               }
      static void
   postprocess_rt_config(struct ac_shader_config *config, enum amd_gfx_level gfx_level, unsigned wave_size)
   {
      config->rsrc1 =
         if (gfx_level < GFX10)
            config->rsrc2 = (config->rsrc2 & C_00B84C_LDS_SIZE) | S_00B84C_LDS_SIZE(config->lds_size);
      }
      static void
   compile_rt_prolog(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline)
   {
               /* create combined config */
   struct ac_shader_config *config = &pipeline->prolog->config;
   for (unsigned i = 0; i < pipeline->stage_count; i++) {
      if (radv_ray_tracing_stage_is_compiled(&pipeline->stages[i])) {
      struct radv_shader *shader = container_of(pipeline->stages[i].shader, struct radv_shader, base);
         }
   combine_config(config, &pipeline->base.base.shaders[MESA_SHADER_INTERSECTION]->config);
      }
      static VkResult
   radv_rt_pipeline_create(VkDevice _device, VkPipelineCache _cache, const VkRayTracingPipelineCreateInfoKHR *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   VK_FROM_HANDLE(vk_pipeline_cache, cache, _cache);
   RADV_FROM_HANDLE(radv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
   VkResult result;
   const VkPipelineCreationFeedbackCreateInfo *creation_feedback =
         if (creation_feedback)
                              VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct radv_ray_tracing_pipeline, pipeline, 1);
   VK_MULTIALLOC_DECL(&ma, struct radv_ray_tracing_stage, stages, local_create_info.stageCount);
   VK_MULTIALLOC_DECL(&ma, struct radv_ray_tracing_group, groups, local_create_info.groupCount);
   VK_MULTIALLOC_DECL(&ma, struct radv_serialized_shader_arena_block, capture_replay_blocks, pCreateInfo->stageCount);
   if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT))
            radv_pipeline_init(device, &pipeline->base.base, RADV_PIPELINE_RAY_TRACING);
   pipeline->base.base.create_flags = radv_get_pipeline_create_flags(pCreateInfo);
   pipeline->stage_count = local_create_info.stageCount;
   pipeline->group_count = local_create_info.groupCount;
   pipeline->stages = stages;
                              /* cache robustness state for making merged shaders */
   if (key.stage_info[MESA_SHADER_INTERSECTION].storage_robustness2)
            if (key.stage_info[MESA_SHADER_INTERSECTION].uniform_robustness2)
            radv_init_rt_stage_hashes(device, pCreateInfo, stages, &key);
   result = radv_rt_fill_group_info(device, pipeline, pCreateInfo, stages, capture_replay_blocks, pipeline->groups);
   if (result != VK_SUCCESS)
            bool keep_statistic_info = radv_pipeline_capture_shader_stats(device, pipeline->base.base.create_flags);
            radv_hash_rt_shaders(pipeline->sha1, pCreateInfo, &key, pipeline->groups,
                  bool cache_hit = false;
   if (!keep_executable_info)
            if (!cache_hit) {
      result =
            if (result != VK_SUCCESS)
               if (!(pipeline->base.base.create_flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR)) {
      compute_rt_stack_size(pCreateInfo, pipeline);
            radv_compute_pipeline_init(device, &pipeline->base, pipeline_layout, pipeline->prolog);
               if (!cache_hit)
            /* write shader VAs into group handles */
   for (unsigned i = 0; i < pipeline->group_count; i++) {
      if (pipeline->groups[i].recursive_shader != VK_SHADER_UNUSED_KHR) {
      struct radv_shader *shader =
                     fail:
      if (creation_feedback)
            if (result == VK_SUCCESS)
         else
            }
      void
   radv_destroy_ray_tracing_pipeline(struct radv_device *device, struct radv_ray_tracing_pipeline *pipeline)
   {
      for (unsigned i = 0; i < pipeline->stage_count; i++) {
      if (pipeline->stages[i].nir)
         if (pipeline->stages[i].shader)
               if (pipeline->prolog)
         if (pipeline->base.base.shaders[MESA_SHADER_INTERSECTION])
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateRayTracingPipelinesKHR(VkDevice _device, VkDeferredOperationKHR deferredOperation,
                     {
               unsigned i = 0;
   for (; i < count; i++) {
      VkResult r;
   r = radv_rt_pipeline_create(_device, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
   if (r != VK_SUCCESS) {
                     const VkPipelineCreateFlagBits2KHR create_flags = radv_get_pipeline_create_flags(&pCreateInfos[i]);
   if (create_flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
                  for (; i < count; ++i)
            if (result != VK_SUCCESS)
            /* Work around Portal RTX not handling VK_OPERATION_NOT_DEFERRED_KHR correctly. */
   if (deferredOperation != VK_NULL_HANDLE)
               }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetRayTracingShaderGroupHandlesKHR(VkDevice device, VkPipeline _pipeline, uint32_t firstGroup, uint32_t groupCount,
         {
      RADV_FROM_HANDLE(radv_pipeline, pipeline, _pipeline);
   struct radv_ray_tracing_group *groups = radv_pipeline_to_ray_tracing(pipeline)->groups;
                              for (uint32_t i = 0; i < groupCount; ++i) {
                     }
      VKAPI_ATTR VkDeviceSize VKAPI_CALL
   radv_GetRayTracingShaderGroupStackSizeKHR(VkDevice device, VkPipeline _pipeline, uint32_t group,
         {
      RADV_FROM_HANDLE(radv_pipeline, pipeline, _pipeline);
   struct radv_ray_tracing_pipeline *rt_pipeline = radv_pipeline_to_ray_tracing(pipeline);
   struct radv_ray_tracing_group *rt_group = &rt_pipeline->groups[group];
   switch (groupShader) {
   case VK_SHADER_GROUP_SHADER_GENERAL_KHR:
   case VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR:
         case VK_SHADER_GROUP_SHADER_ANY_HIT_KHR:
         case VK_SHADER_GROUP_SHADER_INTERSECTION_KHR:
         default:
            }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_GetRayTracingCaptureReplayShaderGroupHandlesKHR(VkDevice device, VkPipeline _pipeline, uint32_t firstGroup,
         {
      RADV_FROM_HANDLE(radv_pipeline, pipeline, _pipeline);
   struct radv_ray_tracing_pipeline *rt_pipeline = radv_pipeline_to_ray_tracing(pipeline);
                     for (uint32_t i = 0; i < groupCount; ++i) {
      uint32_t recursive_shader = rt_pipeline->groups[firstGroup + i].recursive_shader;
   if (recursive_shader != VK_SHADER_UNUSED_KHR) {
      struct radv_shader *shader =
            }
                  }
