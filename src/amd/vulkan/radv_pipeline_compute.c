   /*
   * Copyright © 2016 Red Hat.
   * Copyright © 2016 Bas Nieuwenhuizen
   *
   * based in part on anv driver which is:
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
      #include "meta/radv_meta.h"
   #include "nir/nir.h"
   #include "nir/nir_builder.h"
   #include "nir/nir_serialize.h"
   #include "nir/radv_nir.h"
   #include "spirv/nir_spirv.h"
   #include "util/disk_cache.h"
   #include "util/mesa-sha1.h"
   #include "util/os_time.h"
   #include "util/u_atomic.h"
   #include "radv_cs.h"
   #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "radv_shader_args.h"
   #include "vk_nir_convert_ycbcr.h"
   #include "vk_pipeline.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
      #include "util/u_debug.h"
   #include "ac_binary.h"
   #include "ac_nir.h"
   #include "ac_shader_util.h"
   #include "aco_interface.h"
   #include "sid.h"
   #include "vk_format.h"
      void
   radv_emit_compute_shader(const struct radv_physical_device *pdevice, struct radeon_cmdbuf *cs,
         {
      uint64_t va = radv_shader_get_va(shader);
   unsigned threads_per_threadgroup;
   unsigned threadgroups_per_cu = 1;
   unsigned waves_per_threadgroup;
                     radeon_set_sh_reg_seq(cs, R_00B848_COMPUTE_PGM_RSRC1, 2);
   radeon_emit(cs, shader->config.rsrc1);
   radeon_emit(cs, shader->config.rsrc2);
   if (pdevice->rad_info.gfx_level >= GFX10) {
                  /* Calculate best compute resource limits. */
   threads_per_threadgroup =
                  if (pdevice->rad_info.gfx_level >= GFX10 && waves_per_threadgroup == 1)
            radeon_set_sh_reg(
      cs, R_00B854_COMPUTE_RESOURCE_LIMITS,
         radeon_set_sh_reg_seq(cs, R_00B81C_COMPUTE_NUM_THREAD_X, 3);
   radeon_emit(cs, S_00B81C_NUM_THREAD_FULL(shader->info.cs.block_size[0]));
   radeon_emit(cs, S_00B81C_NUM_THREAD_FULL(shader->info.cs.block_size[1]));
      }
      static void
   radv_compute_generate_pm4(const struct radv_device *device, struct radv_compute_pipeline *pipeline,
         {
      struct radv_physical_device *pdevice = device->physical_device;
            cs->reserved_dw = cs->max_dw = pdevice->rad_info.gfx_level >= GFX10 ? 19 : 16;
                        }
      static struct radv_pipeline_key
   radv_generate_compute_pipeline_key(const struct radv_device *device, const struct radv_compute_pipeline *pipeline,
         {
         }
      void
   radv_compute_pipeline_init(const struct radv_device *device, struct radv_compute_pipeline *pipeline,
         {
               pipeline->base.push_constant_size = layout->push_constant_size;
                        }
      static struct radv_shader *
   radv_compile_cs(struct radv_device *device, struct vk_pipeline_cache *cache, struct radv_shader_stage *cs_stage,
               {
               /* Compile SPIR-V shader to NIR. */
                     /* Gather info again, information such as outputs_read can be out-of-date. */
            /* Run the shader info pass. */
   radv_nir_shader_info_init(cs_stage->stage, MESA_SHADER_NONE, &cs_stage->info);
   radv_nir_shader_info_pass(device, cs_stage->nir, &cs_stage->layout, pipeline_key, RADV_PIPELINE_COMPUTE, false,
            radv_declare_shader_args(device, pipeline_key, &cs_stage->info, MESA_SHADER_COMPUTE, MESA_SHADER_NONE,
            cs_stage->info.user_sgprs_locs = cs_stage->args.user_sgprs_locs;
            /* Postprocess NIR. */
            if (radv_can_dump_shader(device, cs_stage->nir, false))
            /* Compile NIR shader to AMD assembly. */
            *cs_binary = radv_shader_nir_to_asm(device, cs_stage, &cs_stage->nir, 1, pipeline_key, keep_executable_info,
                     radv_shader_generate_debug_info(device, dump_shader, keep_executable_info, *cs_binary, cs_shader, &cs_stage->nir, 1,
            if (keep_executable_info && cs_stage->spirv.size) {
      cs_shader->spirv = malloc(cs_stage->spirv.size);
   memcpy(cs_shader->spirv, cs_stage->spirv.data, cs_stage->spirv.size);
                  }
      static VkResult
   radv_compute_pipeline_compile(struct radv_compute_pipeline *pipeline, struct radv_pipeline_layout *pipeline_layout,
                           {
      struct radv_shader_binary *cs_binary = NULL;
   unsigned char hash[20];
   bool keep_executable_info = radv_pipeline_capture_shaders(device, pipeline->base.create_flags);
   bool keep_statistic_info = radv_pipeline_capture_shader_stats(device, pipeline->base.create_flags);
   struct radv_shader_stage cs_stage = {0};
   VkPipelineCreationFeedback pipeline_feedback = {
         };
                              radv_hash_shaders(hash, &cs_stage, 1, pipeline_layout, pipeline_key,
                     bool found_in_application_cache = true;
   if (!keep_executable_info &&
      radv_pipeline_cache_search(device, cache, &pipeline->base, hash, &found_in_application_cache)) {
   if (found_in_application_cache)
         result = VK_SUCCESS;
               if (pipeline->base.create_flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR)
                     pipeline->base.shaders[MESA_SHADER_COMPUTE] =
      radv_compile_cs(device, cache, &cs_stage, pipeline_key, keep_executable_info, keep_statistic_info,
                  if (!keep_executable_info) {
                  free(cs_binary);
   if (radv_can_dump_shader_stats(device, cs_stage.nir)) {
      radv_dump_shader_stats(device, &pipeline->base, pipeline->base.shaders[MESA_SHADER_COMPUTE], MESA_SHADER_COMPUTE,
      }
         done:
               if (creation_feedback) {
               if (creation_feedback->pipelineStageCreationFeedbackCount) {
      assert(creation_feedback->pipelineStageCreationFeedbackCount == 1);
                     }
      VkResult
   radv_compute_pipeline_create(VkDevice _device, VkPipelineCache _cache, const VkComputePipelineCreateInfo *pCreateInfo,
         {
      RADV_FROM_HANDLE(radv_device, device, _device);
   VK_FROM_HANDLE(vk_pipeline_cache, cache, _cache);
   RADV_FROM_HANDLE(radv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
   struct radv_compute_pipeline *pipeline;
            pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8, VK_SYSTEM_ALLOCATION_SCOPE_OBJECT);
   if (pipeline == NULL) {
                  radv_pipeline_init(device, &pipeline->base, RADV_PIPELINE_COMPUTE);
   pipeline->base.create_flags = radv_get_pipeline_create_flags(pCreateInfo);
            const VkPipelineCreationFeedbackCreateInfo *creation_feedback =
                     result = radv_compute_pipeline_compile(pipeline, pipeline_layout, device, cache, &key, &pCreateInfo->stage,
         if (result != VK_SUCCESS) {
      radv_pipeline_destroy(device, &pipeline->base, pAllocator);
                        *pPipeline = radv_pipeline_to_handle(&pipeline->base);
   radv_rmv_log_compute_pipeline_create(device, &pipeline->base, pipeline->base.is_internal);
      }
      static VkResult
   radv_create_compute_pipelines(VkDevice _device, VkPipelineCache pipelineCache, uint32_t count,
               {
               unsigned i = 0;
   for (; i < count; i++) {
      VkResult r;
   r = radv_compute_pipeline_create(_device, pipelineCache, &pCreateInfos[i], pAllocator, &pPipelines[i]);
   if (r != VK_SUCCESS) {
                     VkPipelineCreateFlagBits2KHR create_flags = radv_get_pipeline_create_flags(&pCreateInfos[i]);
   if (create_flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
                  for (; i < count; ++i)
               }
      void
   radv_destroy_compute_pipeline(struct radv_device *device, struct radv_compute_pipeline *pipeline)
   {
      if (pipeline->base.shaders[MESA_SHADER_COMPUTE])
      }
      VKAPI_ATTR VkResult VKAPI_CALL
   radv_CreateComputePipelines(VkDevice _device, VkPipelineCache pipelineCache, uint32_t count,
               {
         }
