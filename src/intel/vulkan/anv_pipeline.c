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
      #include "util/mesa-sha1.h"
   #include "util/os_time.h"
   #include "common/intel_l3_config.h"
   #include "common/intel_disasm.h"
   #include "common/intel_sample_positions.h"
   #include "anv_private.h"
   #include "compiler/brw_nir.h"
   #include "compiler/brw_nir_rt.h"
   #include "anv_nir.h"
   #include "nir/nir_xfb_info.h"
   #include "spirv/nir_spirv.h"
   #include "vk_nir_convert_ycbcr.h"
   #include "vk_nir.h"
   #include "vk_pipeline.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
      struct lower_set_vtx_and_prim_count_state {
         };
      static nir_variable *
   anv_nir_prim_count_store(nir_builder *b, nir_def *val)
   {
      nir_variable *primitive_count =
         nir_variable_create(b->shader,
               primitive_count->data.location = VARYING_SLOT_PRIMITIVE_COUNT;
                     nir_def *cmp = nir_ieq_imm(b, local_invocation_index, 0);
   nir_if *if_stmt = nir_push_if(b, cmp);
   {
      nir_deref_instr *prim_count_deref = nir_build_deref_var(b, primitive_count);
      }
               }
      static bool
   anv_nir_lower_set_vtx_and_prim_count_instr(nir_builder *b,
               {
      if (intrin->intrinsic != nir_intrinsic_set_vertex_and_primitive_count)
            /* Detect some cases of invalid primitive count. They might lead to URB
   * memory corruption, where workgroups overwrite each other output memory.
   */
   if (nir_src_is_const(intrin->src[1]) &&
                        struct lower_set_vtx_and_prim_count_state *state = data;
   /* this intrinsic should show up only once */
                                          }
      static bool
   anv_nir_lower_set_vtx_and_prim_count(nir_shader *nir)
   {
               nir_shader_intrinsics_pass(nir, anv_nir_lower_set_vtx_and_prim_count_instr,
                  /* If we didn't find set_vertex_and_primitive_count, then we have to
   * insert store of value 0 to primitive_count.
   */
   if (state.primitive_count == NULL) {
      nir_builder b;
   nir_function_impl *entrypoint = nir_shader_get_entrypoint(nir);
   b = nir_builder_at(nir_before_impl(entrypoint));
   nir_def *zero = nir_imm_int(&b, 0);
               assert(state.primitive_count != NULL);
      }
      /* Eventually, this will become part of anv_CreateShader.  Unfortunately,
   * we can't do that yet because we don't have the ability to copy nir.
   */
   static nir_shader *
   anv_shader_stage_to_nir(struct anv_device *device,
                     {
      const struct anv_physical_device *pdevice = device->physical;
   const struct anv_instance *instance = pdevice->instance;
   const struct brw_compiler *compiler = pdevice->compiler;
   gl_shader_stage stage = vk_to_mesa_shader_stage(stage_info->stage);
   const nir_shader_compiler_options *nir_options =
            const bool rt_enabled = ANV_SUPPORT_RT && pdevice->info.has_ray_tracing;
   const struct spirv_to_nir_options spirv_options = {
      .caps = {
      .demote_to_helper_invocation = true,
   .derivative_group = true,
   .descriptor_array_dynamic_indexing = true,
   .descriptor_array_non_uniform_indexing = true,
   .descriptor_indexing = true,
   .device_group = true,
   .draw_parameters = true,
   .float16 = true,
   .float32_atomic_add = pdevice->info.has_lsc,
   .float32_atomic_min_max = true,
   .float64 = true,
   .float64_atomic_min_max = pdevice->info.has_lsc,
   .fragment_shader_sample_interlock = true,
   .fragment_shader_pixel_interlock = true,
   .geometry_streams = true,
   /* When using Vulkan 1.3 or KHR_format_feature_flags2 is enabled, the
   * read/write without format is per format, so just report true. It's
   * up to the application to check.
   */
   .image_read_without_format =
      pdevice->info.verx10 >= 125 ||
   instance->vk.app_info.api_version >= VK_API_VERSION_1_3 ||
      .image_write_without_format = true,
   .int8 = true,
   .int16 = true,
   .int64 = true,
   .int64_atomics = true,
   .integer_functions2 = true,
   .mesh_shading = pdevice->vk.supported_extensions.EXT_mesh_shader,
   .mesh_shading_nv = false,
   .min_lod = true,
   .multiview = true,
   .physical_storage_buffer_address = true,
   .post_depth_coverage = true,
   .runtime_descriptor_array = true,
   .float_controls = true,
   .ray_cull_mask = rt_enabled,
   .ray_query = rt_enabled,
   .ray_tracing = rt_enabled,
   .ray_tracing_position_fetch = rt_enabled,
   .shader_clock = true,
   .shader_viewport_index_layer = true,
   .sparse_residency = pdevice->has_sparse,
   .stencil_export = true,
   .storage_8bit = true,
   .storage_16bit = true,
   .subgroup_arithmetic = true,
   .subgroup_basic = true,
   .subgroup_ballot = true,
   .subgroup_dispatch = true,
   .subgroup_quad = true,
   .subgroup_uniform_control_flow = true,
   .subgroup_shuffle = true,
   .subgroup_vote = true,
   .tessellation = true,
   .transform_feedback = true,
   .variable_pointers = true,
   .vk_memory_model = true,
   .vk_memory_model_device_scope = true,
   .workgroup_memory_explicit_layout = true,
      },
   .ubo_addr_format = anv_nir_ubo_addr_format(pdevice, robust_flags),
   .ssbo_addr_format = anv_nir_ssbo_addr_format(pdevice, robust_flags),
   .phys_ssbo_addr_format = nir_address_format_64bit_global,
            /* TODO: Consider changing this to an address format that has the NULL
   * pointer equals to 0.  That might be a better format to play nice
   * with certain code / code generators.
   */
            .min_ubo_alignment = ANV_UBO_ALIGNMENT,
               nir_shader *nir;
   VkResult result =
      vk_pipeline_shader_stage_to_nir(&device->vk, stage_info,
            if (result != VK_SUCCESS)
            if (INTEL_DEBUG(intel_debug_flag_for_shader_stage(stage))) {
      fprintf(stderr, "NIR (from SPIR-V) for %s shader:\n",
                     NIR_PASS_V(nir, nir_lower_io_to_temporaries,
               }
      static VkResult
   anv_pipeline_init(struct anv_pipeline *pipeline,
                     struct anv_device *device,
   {
                        vk_object_base_init(&device->vk, &pipeline->base,
                  /* It's the job of the child class to provide actual backing storage for
   * the batch by setting batch.start, batch.next, and batch.end.
   */
   pipeline->batch.alloc = pAllocator ? pAllocator : &device->vk.alloc;
   pipeline->batch.relocs = &pipeline->batch_relocs;
            const bool uses_relocs = device->physical->uses_relocs;
   result = anv_reloc_list_init(&pipeline->batch_relocs,
         if (result != VK_SUCCESS)
                     pipeline->type = type;
                     anv_pipeline_sets_layout_init(&pipeline->layout, device,
               }
      static void
   anv_pipeline_init_layout(struct anv_pipeline *pipeline,
         {
      if (pipeline_layout) {
      struct anv_pipeline_sets_layout *layout = &pipeline_layout->sets_layout;
   for (uint32_t s = 0; s < layout->num_sets; s++) {
                     anv_pipeline_sets_layout_add(&pipeline->layout, s,
                  anv_pipeline_sets_layout_hash(&pipeline->layout);
   assert(!pipeline_layout ||
         !memcmp(pipeline->layout.sha1,
      }
      static void
   anv_pipeline_finish(struct anv_pipeline *pipeline,
         {
      anv_pipeline_sets_layout_fini(&pipeline->layout);
   anv_reloc_list_finish(&pipeline->batch_relocs);
   ralloc_free(pipeline->mem_ctx);
      }
      void anv_DestroyPipeline(
      VkDevice                                    _device,
   VkPipeline                                  _pipeline,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pipeline)
            switch (pipeline->type) {
   case ANV_PIPELINE_GRAPHICS_LIB: {
      struct anv_graphics_lib_pipeline *gfx_pipeline =
            for (unsigned s = 0; s < ARRAY_SIZE(gfx_pipeline->base.shaders); s++) {
      if (gfx_pipeline->base.shaders[s])
      }
               case ANV_PIPELINE_GRAPHICS: {
      struct anv_graphics_pipeline *gfx_pipeline =
            for (unsigned s = 0; s < ARRAY_SIZE(gfx_pipeline->base.shaders); s++) {
      if (gfx_pipeline->base.shaders[s])
      }
               case ANV_PIPELINE_COMPUTE: {
      struct anv_compute_pipeline *compute_pipeline =
            if (compute_pipeline->cs)
                        case ANV_PIPELINE_RAY_TRACING: {
      struct anv_ray_tracing_pipeline *rt_pipeline =
            util_dynarray_foreach(&rt_pipeline->shaders,
               }
               default:
                  anv_pipeline_finish(pipeline, device);
      }
      struct anv_pipeline_stage {
                        /* VkComputePipelineCreateInfo, VkGraphicsPipelineCreateInfo or
   * VkRayTracingPipelineCreateInfoKHR pNext field
   */
   const void *pipeline_pNext;
            unsigned char shader_sha1[20];
                     struct {
      gl_shader_stage stage;
                        struct {
      nir_shader *nir;
                                          struct anv_pipeline_binding surface_to_descriptor[256];
   struct anv_pipeline_binding sampler_to_descriptor[256];
                                       uint32_t num_stats;
   struct brw_compile_stats stats[3];
            VkPipelineCreationFeedback feedback;
                        };
      static enum brw_robustness_flags
   anv_get_robust_flags(const struct vk_pipeline_robustness_state *rstate)
   {
      return
      ((rstate->storage_buffers !=
   VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT) ?
   BRW_ROBUSTNESS_SSBO : 0) |
   ((rstate->uniform_buffers !=
   VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT) ?
   }
      static void
   populate_base_prog_key(struct anv_pipeline_stage *stage,
         {
      stage->key.base.robust_flags = anv_get_robust_flags(&stage->rstate);
   stage->key.base.limit_trig_input_range =
      }
      static void
   populate_vs_prog_key(struct anv_pipeline_stage *stage,
         {
                  }
      static void
   populate_tcs_prog_key(struct anv_pipeline_stage *stage,
               {
                           }
      static void
   populate_tes_prog_key(struct anv_pipeline_stage *stage,
         {
                  }
      static void
   populate_gs_prog_key(struct anv_pipeline_stage *stage,
         {
                  }
      static bool
   pipeline_has_coarse_pixel(const BITSET_WORD *dynamic,
               {
      /* The Vulkan 1.2.199 spec says:
   *
   *    "If any of the following conditions are met, Cxy' must be set to
   *    {1,1}:
   *
   *     * If Sample Shading is enabled.
   *     * [...]"
   *
   * And "sample shading" is defined as follows:
   *
   *    "Sample shading is enabled for a graphics pipeline:
   *
   *     * If the interface of the fragment shader entry point of the
   *       graphics pipeline includes an input variable decorated with
   *       SampleId or SamplePosition. In this case minSampleShadingFactor
   *       takes the value 1.0.
   *
   *     * Else if the sampleShadingEnable member of the
   *       VkPipelineMultisampleStateCreateInfo structure specified when
   *       creating the graphics pipeline is set to VK_TRUE. In this case
   *       minSampleShadingFactor takes the value of
   *       VkPipelineMultisampleStateCreateInfo::minSampleShading.
   *
   *    Otherwise, sample shading is considered disabled."
   *
   * The first bullet above is handled by the back-end compiler because those
   * inputs both force per-sample dispatch.  The second bullet is handled
   * here.  Note that this sample shading being enabled has nothing to do
   * with minSampleShading.
   */
   if (ms != NULL && ms->sample_shading_enable)
            /* Not dynamic & pipeline has a 1x1 fragment shading rate with no
   * possibility for element of the pipeline to change the value or fragment
   * shading rate not specified at all.
   */
   if (!BITSET_TEST(dynamic, MESA_VK_DYNAMIC_FSR) &&
      (fsr == NULL ||
   (fsr->fragment_size.width <= 1 &&
      fsr->fragment_size.height <= 1 &&
   fsr->combiner_ops[0] == VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR &&
               }
      static void
   populate_task_prog_key(struct anv_pipeline_stage *stage,
         {
                  }
      static void
   populate_mesh_prog_key(struct anv_pipeline_stage *stage,
               {
                           }
      static uint32_t
   rp_color_mask(const struct vk_render_pass_state *rp)
   {
      if (rp == NULL || rp->attachment_aspects == VK_IMAGE_ASPECT_METADATA_BIT)
            uint32_t color_mask = 0;
   for (uint32_t i = 0; i < rp->color_attachment_count; i++) {
      if (rp->color_attachment_formats[i] != VK_FORMAT_UNDEFINED)
                  }
      static void
   populate_wm_prog_key(struct anv_pipeline_stage *stage,
                        const struct anv_graphics_base_pipeline *pipeline,
   const BITSET_WORD *dynamic,
      {
                                          /* We set this to 0 here and set to the actual value before we call
   * brw_compile_fs.
   */
            /* XXX Vulkan doesn't appear to specify */
                     assert(rp == NULL || rp->color_attachment_count <= MAX_RTS);
   /* Consider all inputs as valid until look at the NIR variables. */
   key->color_outputs_valid = rp_color_mask(rp);
            /* To reduce possible shader recompilations we would need to know if
   * there is a SampleMask output variable to compute if we should emit
   * code to workaround the issue that hardware disables alpha to coverage
   * when there is SampleMask output.
   *
   * If the pipeline we compile the fragment shader in includes the output
   * interface, then we can be sure whether alpha_coverage is enabled or not.
   * If we don't have that output interface, then we have to compile the
   * shader with some conditionals.
   */
   if (ms != NULL) {
      /* VUID-VkGraphicsPipelineCreateInfo-rasterizerDiscardEnable-00751:
   *
   *   "If the pipeline is being created with fragment shader state,
   *    pMultisampleState must be a valid pointer to a valid
   *    VkPipelineMultisampleStateCreateInfo structure"
   *
   * It's also required for the fragment output interface.
   */
   key->alpha_to_coverage =
         key->multisample_fbo =
         key->persample_interp =
   (ms->sample_shading_enable &&
   (ms->min_sample_shading * ms->rasterization_samples) > 1) ?
            /* TODO: We should make this dynamic */
   if (device->physical->instance->sample_mask_out_opengl_behaviour)
      } else {
      /* Consider all inputs as valid until we look at the NIR variables. */
   key->color_outputs_valid = (1u << MAX_RTS) - 1;
            key->alpha_to_coverage = BRW_SOMETIMES;
   key->multisample_fbo = BRW_SOMETIMES;
                        /* Vulkan doesn't support fixed-function alpha test */
         key->coarse_pixel =
      device->vk.enabled_extensions.KHR_fragment_shading_rate &&
      }
      static bool
   wm_prog_data_dynamic(const struct brw_wm_prog_data *prog_data)
   {
      return prog_data->alpha_to_coverage == BRW_SOMETIMES ||
            }
      static void
   populate_cs_prog_key(struct anv_pipeline_stage *stage,
         {
                  }
      static void
   populate_bs_prog_key(struct anv_pipeline_stage *stage,
               {
                        stage->key.bs.pipeline_ray_flags = ray_flags;
      }
      static void
   anv_stage_write_shader_hash(struct anv_pipeline_stage *stage,
         {
      vk_pipeline_robustness_state_fill(&device->vk,
                                 stage->robust_flags =
      ((stage->rstate.storage_buffers !=
   VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT) ?
   BRW_ROBUSTNESS_SSBO : 0) |
   ((stage->rstate.uniform_buffers !=
   VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DISABLED_EXT) ?
         /* Use lowest dword of source shader sha1 for shader hash. */
      }
      static bool
   anv_graphics_pipeline_stage_fragment_dynamic(const struct anv_pipeline_stage *stage)
   {
      if (stage->stage != MESA_SHADER_FRAGMENT)
            return stage->key.wm.persample_interp == BRW_SOMETIMES ||
            }
      static void
   anv_pipeline_hash_common(struct mesa_sha1 *ctx,
         {
                        const bool indirect_descriptors = device->physical->indirect_descriptors;
            const bool rba = device->robust_buffer_access;
            const int spilling_rate = device->physical->compiler->spilling_rate;
      }
      static void
   anv_pipeline_hash_graphics(struct anv_graphics_base_pipeline *pipeline,
                     {
      const struct anv_device *device = pipeline->base.device;
   struct mesa_sha1 ctx;
                              for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (pipeline->base.active_stages & BITFIELD_BIT(s)) {
      _mesa_sha1_update(&ctx, stages[s].shader_sha1,
                        if (stages[MESA_SHADER_MESH].info || stages[MESA_SHADER_TASK].info) {
      const uint8_t afs = device->physical->instance->assume_full_subgroups;
                  }
      static void
   anv_pipeline_hash_compute(struct anv_compute_pipeline *pipeline,
               {
      const struct anv_device *device = pipeline->base.device;
   struct mesa_sha1 ctx;
                     const uint8_t afs = device->physical->instance->assume_full_subgroups;
            _mesa_sha1_update(&ctx, stage->shader_sha1,
                     }
      static void
   anv_pipeline_hash_ray_tracing_shader(struct anv_ray_tracing_pipeline *pipeline,
               {
      struct mesa_sha1 ctx;
                     _mesa_sha1_update(&ctx, stage->shader_sha1, sizeof(stage->shader_sha1));
               }
      static void
   anv_pipeline_hash_ray_tracing_combined_shader(struct anv_ray_tracing_pipeline *pipeline,
                     {
      struct mesa_sha1 ctx;
            _mesa_sha1_update(&ctx, pipeline->base.layout.sha1,
            const bool rba = pipeline->base.device->robust_buffer_access;
            _mesa_sha1_update(&ctx, intersection->shader_sha1, sizeof(intersection->shader_sha1));
   _mesa_sha1_update(&ctx, &intersection->key, sizeof(intersection->key.bs));
   _mesa_sha1_update(&ctx, any_hit->shader_sha1, sizeof(any_hit->shader_sha1));
               }
      static nir_shader *
   anv_pipeline_stage_get_nir(struct anv_pipeline *pipeline,
                     {
      const struct brw_compiler *compiler =
         const nir_shader_compiler_options *nir_options =
                  nir = anv_device_search_for_nir(pipeline->device, cache,
                     if (nir) {
      assert(nir->info.stage == stage->stage);
               nir = anv_shader_stage_to_nir(pipeline->device, stage->info,
         if (nir) {
      anv_device_upload_nir(pipeline->device, cache, nir, stage->shader_sha1);
                  }
      static const struct vk_ycbcr_conversion_state *
   lookup_ycbcr_conversion(const void *_sets_layout, uint32_t set,
         {
               assert(set < MAX_SETS);
   assert(binding < sets_layout->set[set].layout->binding_count);
   const struct anv_descriptor_set_binding_layout *bind_layout =
            if (bind_layout->immutable_samplers == NULL)
                     const struct anv_sampler *sampler =
            return sampler && sampler->vk.ycbcr_conversion ?
      }
      static void
   shared_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type)
         unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length,
      }
      static enum anv_dynamic_push_bits
   anv_nir_compute_dynamic_push_bits(nir_shader *shader)
   {
               nir_foreach_function_impl(impl, shader) {
      nir_foreach_block(block, impl) {
      nir_foreach_instr(instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                  switch (nir_intrinsic_base(intrin)) {
   case offsetof(struct anv_push_constants, gfx.tcs_input_vertices):
                  default:
                              }
      static void
   anv_pipeline_lower_nir(struct anv_pipeline *pipeline,
                        void *mem_ctx,
      {
      const struct anv_physical_device *pdevice = pipeline->device->physical;
            struct brw_stage_prog_data *prog_data = &stage->prog_data.base;
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS(_, nir, nir_lower_wpos_center);
   NIR_PASS(_, nir, nir_lower_input_attachments,
            &(nir_input_attachment_options) {
                  if (nir->info.stage == MESA_SHADER_MESH ||
            nir_lower_compute_system_values_options options = {
         .lower_cs_local_id_to_index = true,
   .lower_workgroup_id_to_index = true,
   /* nir_lower_idiv generates expensive code */
                                 if (pipeline->type == ANV_PIPELINE_GRAPHICS ||
      pipeline->type == ANV_PIPELINE_GRAPHICS_LIB) {
   NIR_PASS(_, nir, anv_nir_lower_multiview, view_mask,
               /* The patch control points are delivered through a push constant when
   * dynamic.
   */
   if (nir->info.stage == MESA_SHADER_TESS_CTRL &&
      stage->key.tcs.input_vertices == 0)
                  NIR_PASS(_, nir, brw_nir_lower_storage_image,
            &(struct brw_nir_lower_storage_image_opts) {
      /* Anv only supports Gfx9+ which has better defined typed read
   * behavior. It allows us to only have to care about lowering
   * loads/atomics.
   */
   .devinfo = compiler->devinfo,
   .lower_loads = true,
   .lower_stores = false,
            NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_global,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_push_const,
                     stage->push_desc_info.used_descriptors =
                     /* Apply the actual pipeline layout to UBOs, SSBOs, and textures */
   NIR_PASS_V(nir, anv_nir_apply_pipeline_layout,
            pdevice, stage->key.base.robust_flags,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ssbo,
            /* First run copy-prop to get rid of all of the vec() that address
   * calculations often create and then constant-fold so that, when we
   * get to anv_nir_lower_ubo_loads, we can detect constant offsets.
   */
   bool progress;
   do {
      progress = false;
   NIR_PASS(progress, nir, nir_opt_algebraic);
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_constant_folding);
               /* Required for nir_divergence_analysis() which is needed for
   * anv_nir_lower_ubo_loads.
   */
   NIR_PASS(_, nir, nir_convert_to_lcssa, true, true);
                              enum nir_lower_non_uniform_access_type lower_non_uniform_access_types =
      nir_lower_non_uniform_texture_access |
   nir_lower_non_uniform_image_access |
         /* In practice, most shaders do not have non-uniform-qualified
   * accesses (see
   * https://gitlab.freedesktop.org/mesa/mesa/-/merge_requests/17558#note_1475069)
   * thus a cheaper and likely to fail check is run first.
   */
   if (nir_has_non_uniform_access(nir, lower_non_uniform_access_types)) {
               /* We don't support non-uniform UBOs and non-uniform SSBO access is
   * handled naturally by falling back to A64 messages.
   */
   NIR_PASS(_, nir, nir_lower_non_uniform_access,
            &(nir_lower_non_uniform_access_options) {
               NIR_PASS(_, nir, brw_nir_lower_non_uniform_resource_intel);
   NIR_PASS(_, nir, brw_nir_cleanup_resource_intel);
                                 NIR_PASS_V(nir, anv_nir_compute_push_layout,
            pdevice, stage->key.base.robust_flags,
         NIR_PASS_V(nir, anv_nir_lower_resource_intel, pdevice,
            if (gl_shader_stage_uses_workgroup(nir->info.stage)) {
      if (!nir->info.shared_memory_explicit_layout) {
      NIR_PASS(_, nir, nir_lower_vars_to_explicit_types,
               NIR_PASS(_, nir, nir_lower_explicit_io,
            if (nir->info.zero_initialize_shared_memory &&
      nir->info.shared_size > 0) {
   /* The effective Shared Local Memory size is at least 1024 bytes and
   * is always rounded to a power of two, so it is OK to align the size
   * used by the shader to chunk_size -- which does simplify the logic.
   */
   const unsigned chunk_size = 16;
   const unsigned shared_size = ALIGN(nir->info.shared_size, chunk_size);
                  NIR_PASS(_, nir, nir_zero_initialize_shared_memory,
                  if (gl_shader_stage_is_compute(nir->info.stage) ||
      gl_shader_stage_is_mesh(nir->info.stage))
         stage->push_desc_info.used_set_buffer =
         stage->push_desc_info.fully_promoted_ubo_descriptors =
               }
      static void
   anv_pipeline_link_vs(const struct brw_compiler *compiler,
               {
      if (next_stage)
      }
      static void
   anv_pipeline_compile_vs(const struct brw_compiler *compiler,
                           {
      /* When using Primitive Replication for multiview, each view gets its own
   * position slot.
   */
   uint32_t pos_slots =
      (vs_stage->nir->info.per_view_outputs & VARYING_BIT_POS) ?
         /* Only position is allowed to be per-view */
            brw_compute_vue_map(compiler->devinfo,
                                       struct brw_compile_vs_params params = {
      .base = {
      .nir = vs_stage->nir,
   .stats = vs_stage->stats,
   .log_data = pipeline->base.device,
   .mem_ctx = mem_ctx,
      },
   .key = &vs_stage->key.vs,
                  }
      static void
   merge_tess_info(struct shader_info *tes_info,
         {
      /* The Vulkan 1.0.38 spec, section 21.1 Tessellator says:
   *
   *    "PointMode. Controls generation of points rather than triangles
   *     or lines. This functionality defaults to disabled, and is
   *     enabled if either shader stage includes the execution mode.
   *
   * and about Triangles, Quads, IsoLines, VertexOrderCw, VertexOrderCcw,
   * PointMode, SpacingEqual, SpacingFractionalEven, SpacingFractionalOdd,
   * and OutputVertices, it says:
   *
   *    "One mode must be set in at least one of the tessellation
   *     shader stages."
   *
   * So, the fields can be set in either the TCS or TES, but they must
   * agree if set in both.  Our backend looks at TES, so bitwise-or in
   * the values from the TCS.
   */
   assert(tcs_info->tess.tcs_vertices_out == 0 ||
         tes_info->tess.tcs_vertices_out == 0 ||
            assert(tcs_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
         tes_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
            assert(tcs_info->tess._primitive_mode == 0 ||
         tes_info->tess._primitive_mode == 0 ||
   tes_info->tess._primitive_mode |= tcs_info->tess._primitive_mode;
   tes_info->tess.ccw |= tcs_info->tess.ccw;
      }
      static void
   anv_pipeline_link_tcs(const struct brw_compiler *compiler,
               {
                        nir_lower_patch_vertices(tes_stage->nir,
                  /* Copy TCS info into the TES info */
            /* Whacking the key after cache lookup is a bit sketchy, but all of
   * this comes from the SPIR-V, which is part of the hash used for the
   * pipeline cache.  So it should be safe.
   */
   tcs_stage->key.tcs._tes_primitive_mode =
      }
      static void
   anv_pipeline_compile_tcs(const struct brw_compiler *compiler,
                           {
      tcs_stage->key.tcs.outputs_written =
         tcs_stage->key.tcs.patch_outputs_written =
                     struct brw_compile_tcs_params params = {
      .base = {
      .nir = tcs_stage->nir,
   .stats = tcs_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &tcs_stage->key.tcs,
                  }
      static void
   anv_pipeline_link_tes(const struct brw_compiler *compiler,
               {
      if (next_stage)
      }
      static void
   anv_pipeline_compile_tes(const struct brw_compiler *compiler,
                           {
      tes_stage->key.tes.inputs_read =
         tes_stage->key.tes.patch_inputs_read =
                     struct brw_compile_tes_params params = {
      .base = {
      .nir = tes_stage->nir,
   .stats = tes_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &tes_stage->key.tes,
   .prog_data = &tes_stage->prog_data.tes,
                  }
      static void
   anv_pipeline_link_gs(const struct brw_compiler *compiler,
               {
      if (next_stage)
      }
      static void
   anv_pipeline_compile_gs(const struct brw_compiler *compiler,
                           {
      brw_compute_vue_map(compiler->devinfo,
                                 struct brw_compile_gs_params params = {
      .base = {
      .nir = gs_stage->nir,
   .stats = gs_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &gs_stage->key.gs,
                  }
      static void
   anv_pipeline_link_task(const struct brw_compiler *compiler,
               {
      assert(next_stage);
   assert(next_stage->stage == MESA_SHADER_MESH);
      }
      static void
   anv_pipeline_compile_task(const struct brw_compiler *compiler,
                     {
               struct brw_compile_task_params params = {
      .base = {
      .nir = task_stage->nir,
   .stats = task_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &task_stage->key.task,
                  }
      static void
   anv_pipeline_link_mesh(const struct brw_compiler *compiler,
               {
      if (next_stage) {
            }
      static void
   anv_pipeline_compile_mesh(const struct brw_compiler *compiler,
                           {
               struct brw_compile_mesh_params params = {
      .base = {
      .nir = mesh_stage->nir,
   .stats = mesh_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &mesh_stage->key.mesh,
               if (prev_stage) {
      assert(prev_stage->stage == MESA_SHADER_TASK);
                  }
      static void
   anv_pipeline_link_fs(const struct brw_compiler *compiler,
               {
      /* Initially the valid outputs value is set to all possible render targets
   * valid (see populate_wm_prog_key()), before we look at the shader
   * variables. Here we look at the output variables of the shader an compute
   * a correct number of render target outputs.
   */
   stage->key.wm.color_outputs_valid = 0;
   nir_foreach_shader_out_variable_safe(var, stage->nir) {
      if (var->data.location < FRAG_RESULT_DATA0)
            const unsigned rt = var->data.location - FRAG_RESULT_DATA0;
   const unsigned array_len =
                     }
   stage->key.wm.color_outputs_valid &= rp_color_mask(rp);
   stage->key.wm.nr_color_regions =
            unsigned num_rt_bindings;
   struct anv_pipeline_binding rt_bindings[MAX_RTS];
   if (stage->key.wm.nr_color_regions > 0) {
      assert(stage->key.wm.nr_color_regions <= MAX_RTS);
   for (unsigned rt = 0; rt < stage->key.wm.nr_color_regions; rt++) {
      if (stage->key.wm.color_outputs_valid & BITFIELD_BIT(rt)) {
      rt_bindings[rt] = (struct anv_pipeline_binding) {
                           } else {
      /* Setup a null render target */
   rt_bindings[rt] = (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
   .index = UINT32_MAX,
            }
      } else {
      /* Setup a null render target */
   rt_bindings[0] = (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
      };
               assert(num_rt_bindings <= MAX_RTS);
   assert(stage->bind_map.surface_count == 0);
   typed_memcpy(stage->bind_map.surface_to_descriptor,
            }
      static void
   anv_pipeline_compile_fs(const struct brw_compiler *compiler,
                           void *mem_ctx,
   struct anv_device *device,
   struct anv_pipeline_stage *fs_stage,
   {
      /* When using Primitive Replication for multiview, each view gets its own
   * position slot.
   */
   uint32_t pos_slots = use_primitive_replication ?
            /* If we have a previous stage we can use that to deduce valid slots.
   * Otherwise, rely on inputs of the input shader.
   */
   if (prev_stage) {
      fs_stage->key.wm.input_slots_valid =
      } else {
      struct brw_vue_map prev_vue_map;
   brw_compute_vue_map(compiler->devinfo,
                                          struct brw_compile_fs_params params = {
      .base = {
      .nir = fs_stage->nir,
   .stats = fs_stage->stats,
   .log_data = device,
   .mem_ctx = mem_ctx,
      },
   .key = &fs_stage->key.wm,
                        if (prev_stage && prev_stage->stage == MESA_SHADER_MESH) {
      params.mue_map = &prev_stage->prog_data.mesh.map;
                        fs_stage->num_stats = (uint32_t)fs_stage->prog_data.wm.dispatch_8 +
            }
      static void
   anv_pipeline_add_executable(struct anv_pipeline *pipeline,
                     {
      char *nir = NULL;
   if (stage->nir &&
      (pipeline->flags &
   VK_PIPELINE_CREATE_2_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
               char *disasm = NULL;
   if (stage->code &&
      (pipeline->flags &
   VK_PIPELINE_CREATE_2_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
   char *stream_data = NULL;
   size_t stream_size = 0;
            uint32_t push_size = 0;
   for (unsigned i = 0; i < 4; i++)
         if (push_size > 0) {
      fprintf(stream, "Push constant ranges:\n");
   for (unsigned i = 0; i < 4; i++) {
                                    switch (stage->bind_map.push_ranges[i].set) {
   case ANV_DESCRIPTOR_SET_NULL:
                  case ANV_DESCRIPTOR_SET_PUSH_CONSTANTS:
                  case ANV_DESCRIPTOR_SET_DESCRIPTORS:
      fprintf(stream, "Descriptor buffer for set %d (start=%dB)",
                                                   default:
      fprintf(stream, "UBO (set=%d binding=%d start=%dB)",
         stage->bind_map.push_ranges[i].set,
   stage->bind_map.push_ranges[i].index,
      }
      }
               /* Creating this is far cheaper than it looks.  It's perfectly fine to
   * do it for every binary.
   */
   intel_disassemble(&pipeline->device->physical->compiler->isa,
                     /* Copy it to a ralloc'd thing */
   disasm = ralloc_size(pipeline->mem_ctx, stream_size + 1);
   memcpy(disasm, stream_data, stream_size);
                        const struct anv_pipeline_executable exe = {
      .stage = stage->stage,
   .stats = *stats,
   .nir = nir,
      };
   util_dynarray_append(&pipeline->executables,
      }
      static void
   anv_pipeline_add_executables(struct anv_pipeline *pipeline,
         {
      if (stage->stage == MESA_SHADER_FRAGMENT) {
      /* We pull the prog data and stats out of the anv_shader_bin because
   * the anv_pipeline_stage may not be fully populated if we successfully
   * looked up the shader in a cache.
   */
   const struct brw_wm_prog_data *wm_prog_data =
                  if (wm_prog_data->dispatch_8) {
                  if (wm_prog_data->dispatch_16) {
      anv_pipeline_add_executable(pipeline, stage, stats++,
               if (wm_prog_data->dispatch_32) {
      anv_pipeline_add_executable(pipeline, stage, stats++,
         } else {
            }
      static void
   anv_pipeline_account_shader(struct anv_pipeline *pipeline,
         {
      pipeline->scratch_size = MAX2(pipeline->scratch_size,
            pipeline->ray_queries = MAX2(pipeline->ray_queries,
            if (shader->push_desc_info.used_set_buffer) {
      pipeline->use_push_descriptor_buffer |=
      }
   if (shader->push_desc_info.used_descriptors &
      ~shader->push_desc_info.fully_promoted_ubo_descriptors)
   }
      /* This function return true if a shader should not be looked at because of
   * fast linking. Instead we should use the shader binaries provided by
   * libraries.
   */
   static bool
   anv_graphics_pipeline_skip_shader_compile(struct anv_graphics_base_pipeline *pipeline,
                     {
      /* Always skip non active stages */
   if (!anv_pipeline_base_has_stage(pipeline, stage))
            /* When link optimizing, consider all stages */
   if (link_optimize)
            /* Otherwise check if the stage was specified through
   * VkGraphicsPipelineCreateInfo
   */
   assert(stages[stage].info != NULL || stages[stage].imported.bin != NULL);
      }
      static void
   anv_graphics_pipeline_init_keys(struct anv_graphics_base_pipeline *pipeline,
               {
      for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
                     const struct anv_device *device = pipeline->base.device;
   switch (stages[s].stage) {
   case MESA_SHADER_VERTEX:
      populate_vs_prog_key(&stages[s], device);
      case MESA_SHADER_TESS_CTRL:
      populate_tcs_prog_key(&stages[s],
                              case MESA_SHADER_TESS_EVAL:
      populate_tes_prog_key(&stages[s], device);
      case MESA_SHADER_GEOMETRY:
      populate_gs_prog_key(&stages[s], device);
      case MESA_SHADER_FRAGMENT: {
      /* Assume rasterization enabled in any of the following case :
   *
   *    - We're a pipeline library without pre-rasterization information
   *
   *    - Rasterization is not disabled in the non dynamic state
   *
   *    - Rasterization disable is dynamic
   */
   const bool raster_enabled =
      state->rs == NULL ||
   !state->rs->rasterizer_discard_enable ||
      enum brw_sometimes is_mesh = BRW_NEVER;
   if (device->vk.enabled_extensions.EXT_mesh_shader) {
      if (anv_pipeline_base_has_stage(pipeline, MESA_SHADER_VERTEX))
         else if (anv_pipeline_base_has_stage(pipeline, MESA_SHADER_MESH))
         else {
      assert(pipeline->base.type == ANV_PIPELINE_GRAPHICS_LIB);
         }
   populate_wm_prog_key(&stages[s],
                                       case MESA_SHADER_TASK:
                  case MESA_SHADER_MESH: {
      const bool compact_mue =
      !(pipeline->base.type == ANV_PIPELINE_GRAPHICS_LIB &&
      populate_mesh_prog_key(&stages[s], device, compact_mue);
               default:
                  stages[s].feedback.duration += os_time_get_nano() - stage_start;
         }
      static void
   anv_graphics_lib_retain_shaders(struct anv_graphics_base_pipeline *pipeline,
               {
      /* There isn't much point in retaining NIR shaders on final pipelines. */
                     for (int s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
            memcpy(lib->retained_shaders[s].shader_sha1, stages[s].shader_sha1,
                     nir_shader *nir = stages[s].nir != NULL ? stages[s].nir : stages[s].imported.nir;
            if (!will_compile) {
         } else {
      lib->retained_shaders[s].nir =
            }
      static bool
   anv_graphics_pipeline_load_cached_shaders(struct anv_graphics_base_pipeline *pipeline,
                           {
      struct anv_device *device = pipeline->base.device;
            for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
                     bool cache_hit;
   stages[s].bin =
      anv_device_search_for_kernel(device, cache, &stages[s].cache_key,
      if (stages[s].bin) {
      found++;
               if (cache_hit) {
      cache_hits++;
   stages[s].feedback.flags |=
      }
               /* When not link optimizing, lookup the missing shader in the imported
   * libraries.
   */
   if (!link_optimize) {
      for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
                                                   stages[s].bin = stages[s].imported.bin;
   pipeline->shaders[s] = anv_shader_bin_ref(stages[s].imported.bin);
                  if ((found + imported) == __builtin_popcount(pipeline->base.active_stages)) {
      if (cache_hits == found && found != 0) {
      pipeline_feedback->flags |=
      }
   /* We found all our shaders in the cache.  We're done. */
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
                     /* Only add the executables when we're not importing or doing link
   * optimizations. The imported executables are added earlier. Link
   * optimization can produce different binaries.
   */
   if (stages[s].imported.bin == NULL || link_optimize)
            }
      } else if (found > 0) {
      /* We found some but not all of our shaders. This shouldn't happen most
   * of the time but it can if we have a partially populated pipeline
   * cache.
   */
            vk_perf(VK_LOG_OBJS(cache ? &cache->base :
               "Found a partial pipeline in the cache.  This is "
            /* We're going to have to recompile anyway, so just throw away our
   * references to the shaders in the cache.  We'll get them out of the
   * cache again as part of the compilation process.
   */
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      stages[s].feedback.flags = 0;
   if (pipeline->shaders[s]) {
      anv_shader_bin_unref(device, pipeline->shaders[s]);
                        }
      static const gl_shader_stage graphics_shader_order[] = {
      MESA_SHADER_VERTEX,
   MESA_SHADER_TESS_CTRL,
   MESA_SHADER_TESS_EVAL,
            MESA_SHADER_TASK,
               };
      /* This function loads NIR only for stages specified in
   * VkGraphicsPipelineCreateInfo::pStages[]
   */
   static VkResult
   anv_graphics_pipeline_load_nir(struct anv_graphics_base_pipeline *pipeline,
                           {
      for (unsigned s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
                              stages[s].bind_map = (struct anv_pipeline_bind_map) {
      .surface_to_descriptor = stages[s].surface_to_descriptor,
               /* Only use the create NIR from the pStages[] element if we don't have
   * an imported library for the same stage.
   */
   if (stages[s].imported.bin == NULL) {
      stages[s].nir = anv_pipeline_stage_get_nir(&pipeline->base, cache,
         if (stages[s].nir == NULL)
      } else {
      stages[s].nir = need_clone ?
                                    }
      static void
   anv_fixup_subgroup_size(struct anv_device *device, struct shader_info *info)
   {
      switch (info->stage) {
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TASK:
   case MESA_SHADER_MESH:
         default:
                  unsigned local_size = info->workgroup_size[0] *
                  /* Games don't always request full subgroups when they should,
   * which can cause bugs, as they may expect bigger size of the
   * subgroup than we choose for the execution.
   */
   if (device->physical->instance->assume_full_subgroups &&
      info->uses_wide_subgroup_intrinsics &&
   info->subgroup_size == SUBGROUP_SIZE_API_CONSTANT &&
   local_size &&
   local_size % BRW_SUBGROUP_SIZE == 0)
         /* If the client requests that we dispatch full subgroups but doesn't
   * allow us to pick a subgroup size, we have to smash it to the API
   * value of 32.  Performance will likely be terrible in this case but
   * there's nothing we can do about that.  The client should have chosen
   * a size.
   */
   if (info->subgroup_size == SUBGROUP_SIZE_FULL_SUBGROUPS)
      info->subgroup_size =
         }
      static void
   anv_pipeline_nir_preprocess(struct anv_pipeline *pipeline,
         {
      struct anv_device *device = pipeline->device;
            const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
         };
            const nir_opt_access_options opt_access_options = {
         };
            /* Vulkan uses the separate-shader linking model */
            struct brw_nir_compiler_opts opts = {
      .softfp64 = device->fp64_nir,
   /* Assume robustness with EXT_pipeline_robustness because this can be
   * turned on/off per pipeline and we have no visibility on this here.
   */
   .robust_image_access = device->vk.enabled_features.robustImageAccess ||
               .input_vertices = stage->nir->info.stage == MESA_SHADER_TESS_CTRL ?
      };
            if (stage->nir->info.stage == MESA_SHADER_MESH) {
      NIR_PASS(_, stage->nir, anv_nir_lower_set_vtx_and_prim_count);
   NIR_PASS(_, stage->nir, nir_opt_dce);
                           }
      static void
   anv_fill_pipeline_creation_feedback(const struct anv_graphics_base_pipeline *pipeline,
                     {
      const VkPipelineCreationFeedbackCreateInfo *create_feedback =
         if (create_feedback) {
               /* VkPipelineCreationFeedbackCreateInfo:
   *
   *    "An implementation must set or clear the
   *     VK_PIPELINE_CREATION_FEEDBACK_VALID_BIT in
   *     VkPipelineCreationFeedback::flags for pPipelineCreationFeedback
   *     and every element of pPipelineStageCreationFeedbacks."
   *
   */
   for (uint32_t i = 0; i < create_feedback->pipelineStageCreationFeedbackCount; i++) {
      create_feedback->pPipelineStageCreationFeedbacks[i].flags &=
      }
   /* This part is not really specified in the Vulkan spec at the moment.
   * We're kind of guessing what the CTS wants. We might need to update
   * when https://gitlab.khronos.org/vulkan/vulkan/-/issues/3115 is
   * clarified.
   */
   for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
                     if (stages[s].feedback_idx < create_feedback->pipelineStageCreationFeedbackCount) {
      create_feedback->pPipelineStageCreationFeedbacks[
               }
      static uint32_t
   anv_graphics_pipeline_imported_shader_count(struct anv_pipeline_stage *stages)
   {
      uint32_t count = 0;
   for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (stages[s].imported.bin != NULL)
      }
      }
      static VkResult
   anv_graphics_pipeline_compile(struct anv_graphics_base_pipeline *pipeline,
                                 {
               struct anv_device *device = pipeline->base.device;
   const struct intel_device_info *devinfo = device->info;
            /* Setup the shaders given in this VkGraphicsPipelineCreateInfo::pStages[].
   * Other shaders imported from libraries should have been added by
   * anv_graphics_pipeline_import_lib().
   */
   uint32_t shader_count = anv_graphics_pipeline_imported_shader_count(stages);
   for (uint32_t i = 0; i < info->stageCount; i++) {
               /* If a pipeline library is loaded in this stage, we should ignore the
   * pStages[] entry of the same stage.
   */
   if (stages[stage].imported.bin != NULL)
            stages[stage].stage = stage;
   stages[stage].pipeline_pNext = info->pNext;
   stages[stage].info = &info->pStages[i];
                        /* Prepare shader keys for all shaders in pipeline->base.active_stages
   * (this includes libraries) before generating the hash for cache look up.
   *
   * We're doing this because the spec states that :
   *
   *    "When an implementation is looking up a pipeline in a pipeline cache,
   *     if that pipeline is being created using linked libraries,
   *     implementations should always return an equivalent pipeline created
   *     with VK_PIPELINE_CREATE_LINK_TIME_OPTIMIZATION_BIT_EXT if available,
   *     whether or not that bit was specified."
   *
   * So even if the application does not request link optimization, we have
   * to do our cache lookup with the entire set of shader sha1s so that we
   * can find what would be the best optimized pipeline in the case as if we
   * had compiled all the shaders together and known the full graphics state.
   */
                     unsigned char sha1[20];
            for (unsigned s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
            stages[s].cache_key.stage = s;
               const bool retain_shaders =
         const bool link_optimize =
            VkResult result = VK_SUCCESS;
   const bool skip_cache_lookup =
            if (!skip_cache_lookup) {
      bool found_all_shaders =
      anv_graphics_pipeline_load_cached_shaders(pipeline, cache, stages,
               if (found_all_shaders) {
      /* If we need to retain shaders, we need to also load from the NIR
   * cache.
   */
   if (pipeline->base.type == ANV_PIPELINE_GRAPHICS_LIB && retain_shaders) {
      result = anv_graphics_pipeline_load_nir(pipeline, cache,
                     if (result != VK_SUCCESS) {
      vk_perf(VK_LOG_OBJS(cache ? &cache->base :
                                                for (unsigned s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
                     if (stages[s].nir) {
                        assert(pipeline->shaders[s] != NULL);
   anv_shader_bin_unref(device, pipeline->shaders[s]);
                     if (pipeline->base.flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR)
                     result = anv_graphics_pipeline_load_nir(pipeline, cache, stages,
         if (result != VK_SUCCESS)
            /* Retain shaders now if asked, this only applies to libraries */
   if (pipeline->base.type == ANV_PIPELINE_GRAPHICS_LIB && retain_shaders)
            /* The following steps will be executed for shaders we need to compile :
   *
   *    - specified through VkGraphicsPipelineCreateInfo::pStages[]
   *
   *    - or compiled from libraries with retained shaders (libraries
   *      compiled with CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT) if the
   *      pipeline has the CREATE_LINK_TIME_OPTIMIZATION_BIT flag.
            /* Preprocess all NIR shaders. */
   for (int s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (anv_graphics_pipeline_skip_shader_compile(pipeline, stages,
                              if (stages[MESA_SHADER_MESH].info && stages[MESA_SHADER_FRAGMENT].info) {
      anv_apply_per_prim_attr_wa(stages[MESA_SHADER_MESH].nir,
                           /* Walk backwards to link */
   struct anv_pipeline_stage *next_stage = NULL;
   for (int i = ARRAY_SIZE(graphics_shader_order) - 1; i >= 0; i--) {
      gl_shader_stage s = graphics_shader_order[i];
   if (anv_graphics_pipeline_skip_shader_compile(pipeline, stages,
                           switch (s) {
   case MESA_SHADER_VERTEX:
      anv_pipeline_link_vs(compiler, stage, next_stage);
      case MESA_SHADER_TESS_CTRL:
      anv_pipeline_link_tcs(compiler, stage, next_stage);
      case MESA_SHADER_TESS_EVAL:
      anv_pipeline_link_tes(compiler, stage, next_stage);
      case MESA_SHADER_GEOMETRY:
      anv_pipeline_link_gs(compiler, stage, next_stage);
      case MESA_SHADER_TASK:
      anv_pipeline_link_task(compiler, stage, next_stage);
      case MESA_SHADER_MESH:
      anv_pipeline_link_mesh(compiler, stage, next_stage);
      case MESA_SHADER_FRAGMENT:
      anv_pipeline_link_fs(compiler, stage, state->rp);
      default:
                              bool use_primitive_replication = false;
   if (devinfo->ver >= 12 && view_mask != 0) {
      /* For some pipelines HW Primitive Replication can be used instead of
   * instancing to implement Multiview.  This depend on how viewIndex is
   * used in all the active shaders, so this check can't be done per
   * individual shaders.
   */
   nir_shader *shaders[ANV_GRAPHICS_SHADER_STAGE_COUNT] = {};
   for (unsigned s = 0; s < ARRAY_SIZE(shaders); s++)
            use_primitive_replication =
      anv_check_for_primitive_replication(device,
                  struct anv_pipeline_stage *prev_stage = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
      gl_shader_stage s = graphics_shader_order[i];
   if (anv_graphics_pipeline_skip_shader_compile(pipeline, stages,
                                    anv_pipeline_lower_nir(&pipeline->base, tmp_ctx, stage,
                           if (prev_stage && compiler->nir_options[s]->unify_interfaces) {
               prev_info->outputs_written |= cur_info->inputs_read &
         cur_info->inputs_read |= prev_info->outputs_written &
         prev_info->patch_outputs_written |= cur_info->patch_inputs_read;
                                             /* In the case the platform can write the primitive variable shading rate
   * and KHR_fragment_shading_rate is enabled :
   *    - there can be a fragment shader but we don't have it yet
   *    - the fragment shader needs fragment shading rate
   *
   * figure out the last geometry stage that should write the primitive
   * shading rate, and ensure it is marked as used there. The backend will
   * write a default value if the shader doesn't actually write it.
   *
   * We iterate backwards in the stage and stop on the first shader that can
   * set the value.
   *
   * Don't apply this to MESH stages, as this is a per primitive thing.
   */
   if (devinfo->has_coarse_pixel_primitive_and_cb &&
      device->vk.enabled_extensions.KHR_fragment_shading_rate &&
   pipeline_has_coarse_pixel(state->dynamic, state->ms, state->fsr) &&
   (!stages[MESA_SHADER_FRAGMENT].info ||
   stages[MESA_SHADER_FRAGMENT].key.wm.coarse_pixel) &&
   stages[MESA_SHADER_MESH].nir == NULL) {
            for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
                     if (anv_graphics_pipeline_skip_shader_compile(pipeline, stages,
                        last_psr = &stages[s];
               /* Only set primitive shading rate if there is a pre-rasterization
   * shader in this pipeline/pipeline-library.
   */
   if (last_psr)
               prev_stage = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
      gl_shader_stage s = graphics_shader_order[i];
            if (anv_graphics_pipeline_skip_shader_compile(pipeline, stages, link_optimize, s))
                              switch (s) {
   case MESA_SHADER_VERTEX:
      anv_pipeline_compile_vs(compiler, stage_ctx, pipeline,
            case MESA_SHADER_TESS_CTRL:
      anv_pipeline_compile_tcs(compiler, stage_ctx, device,
            case MESA_SHADER_TESS_EVAL:
      anv_pipeline_compile_tes(compiler, stage_ctx, device,
            case MESA_SHADER_GEOMETRY:
      anv_pipeline_compile_gs(compiler, stage_ctx, device,
            case MESA_SHADER_TASK:
      anv_pipeline_compile_task(compiler, stage_ctx, device,
            case MESA_SHADER_MESH:
      anv_pipeline_compile_mesh(compiler, stage_ctx, device,
            case MESA_SHADER_FRAGMENT:
      anv_pipeline_compile_fs(compiler, stage_ctx, device,
                        default:
         }
   if (stage->code == NULL) {
      ralloc_free(stage_ctx);
   result = vk_error(device, VK_ERROR_OUT_OF_HOST_MEMORY);
               anv_nir_validate_push_layout(&stage->prog_data.base,
            stage->bin =
      anv_device_upload_kernel(device, cache, s,
                           &stage->cache_key,
   sizeof(stage->cache_key),
   stage->code,
   stage->prog_data.base.program_size,
   &stage->prog_data.base,
   brw_prog_data_size(s),
      if (!stage->bin) {
      ralloc_free(stage_ctx);
   result = vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
               anv_pipeline_add_executables(&pipeline->base, stage);
   pipeline->source_hashes[s] = stage->source_hash;
                                          /* Finally add the imported shaders that were not compiled as part of this
   * step.
   */
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
            if (pipeline->shaders[s] != NULL)
            /* We should have recompiled everything with link optimization. */
                     pipeline->source_hashes[s] = stage->source_hash;
                     done:
         /* Write the feedback index into the pipeline */
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (!anv_pipeline_base_has_stage(pipeline, s))
            struct anv_pipeline_stage *stage = &stages[s];
   pipeline->feedback_index[s] = stage->feedback_idx;
                                 if (pipeline->shaders[MESA_SHADER_FRAGMENT]) {
      pipeline->fragment_dynamic =
      anv_graphics_pipeline_stage_fragment_dynamic(
                  fail:
               for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (pipeline->shaders[s])
                  }
      static VkResult
   anv_pipeline_compile_cs(struct anv_compute_pipeline *pipeline,
               {
      ASSERTED const VkPipelineShaderStageCreateInfo *sinfo = &info->stage;
            VkPipelineCreationFeedback pipeline_feedback = {
         };
            struct anv_device *device = pipeline->base.device;
            struct anv_pipeline_stage stage = {
      .stage = MESA_SHADER_COMPUTE,
   .info = &info->stage,
   .pipeline_pNext = info->pNext,
   .cache_key = {
         },
   .feedback = {
            };
                     const bool skip_cache_lookup =
                     bool cache_hit = false;
   if (!skip_cache_lookup) {
      stage.bin = anv_device_search_for_kernel(device, cache,
                           if (stage.bin == NULL &&
      (pipeline->base.flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT))
         void *mem_ctx = ralloc_context(NULL);
   if (stage.bin == NULL) {
               stage.bind_map = (struct anv_pipeline_bind_map) {
      .surface_to_descriptor = stage.surface_to_descriptor,
               /* Set up a binding for the gl_NumWorkGroups */
   stage.bind_map.surface_count = 1;
   stage.bind_map.surface_to_descriptor[0] = (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_NUM_WORK_GROUPS,
               stage.nir = anv_pipeline_stage_get_nir(&pipeline->base, cache, mem_ctx, &stage);
   if (stage.nir == NULL) {
      ralloc_free(mem_ctx);
                        anv_pipeline_lower_nir(&pipeline->base, mem_ctx, &stage,
                                    struct brw_compile_cs_params params = {
      .base = {
      .nir = stage.nir,
   .stats = stage.stats,
   .log_data = device,
      },
   .key = &stage.key.cs,
               stage.code = brw_compile_cs(compiler, &params);
   if (stage.code == NULL) {
      ralloc_free(mem_ctx);
                        if (!stage.prog_data.cs.uses_num_work_groups) {
      assert(stage.bind_map.surface_to_descriptor[0].set ==
                     const unsigned code_size = stage.prog_data.base.program_size;
   stage.bin = anv_device_upload_kernel(device, cache,
                                 MESA_SHADER_COMPUTE,
   &stage.cache_key, sizeof(stage.cache_key),
   stage.code, code_size,
   &stage.prog_data.base,
   if (!stage.bin) {
      ralloc_free(mem_ctx);
                           anv_pipeline_account_shader(&pipeline->base, stage.bin);
   anv_pipeline_add_executables(&pipeline->base, &stage);
                     if (cache_hit) {
      stage.feedback.flags |=
         pipeline_feedback.flags |=
      }
            const VkPipelineCreationFeedbackCreateInfo *create_feedback =
         if (create_feedback) {
               if (create_feedback->pipelineStageCreationFeedbackCount) {
      assert(create_feedback->pipelineStageCreationFeedbackCount == 1);
                              }
      static VkPipelineCreateFlags2KHR
   get_pipeline_flags(VkPipelineCreateFlags flags, const void *pNext)
   {
      const VkPipelineCreateFlags2CreateInfoKHR *flags2 =
            if (flags2)
               }
      static VkResult
   anv_compute_pipeline_create(struct anv_device *device,
                           {
      struct anv_compute_pipeline *pipeline;
                     pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
         if (pipeline == NULL)
            result = anv_pipeline_init(&pipeline->base, device,
                           if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
                  ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
                     anv_batch_set_storage(&pipeline->base.batch, ANV_NULL_ADDRESS,
            result = anv_pipeline_compile_cs(pipeline, cache, pCreateInfo);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
                                    }
      VkResult anv_CreateComputePipelines(
      VkDevice                                    _device,
   VkPipelineCache                             pipelineCache,
   uint32_t                                    count,
   const VkComputePipelineCreateInfo*          pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     unsigned i;
   for (i = 0; i < count; i++) {
      const VkPipelineCreateFlags2KHR flags =
         VkResult res = anv_compute_pipeline_create(device, pipeline_cache,
                  if (res == VK_SUCCESS)
            /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED as it
   * is not obvious what error should be report upon 2 different failures.
   * */
   result = res;
   if (res != VK_PIPELINE_COMPILE_REQUIRED)
                     if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
               for (; i < count; i++)
               }
      /**
   * Calculate the desired L3 partitioning based on the current state of the
   * pipeline.  For now this simply returns the conservative defaults calculated
   * by get_default_l3_weights(), but we could probably do better by gathering
   * more statistics from the pipeline state (e.g. guess of expected URB usage
   * and bound surfaces), or by using feed-back from performance counters.
   */
   void
   anv_pipeline_setup_l3_config(struct anv_pipeline *pipeline, bool needs_slm)
   {
               const struct intel_l3_weights w =
               }
      static uint32_t
   get_vs_input_elements(const struct brw_vs_prog_data *vs_prog_data)
   {
      /* Pull inputs_read out of the VS prog data */
   const uint64_t inputs_read = vs_prog_data->inputs_read;
   const uint64_t double_inputs_read =
         assert((inputs_read & ((1 << VERT_ATTRIB_GENERIC0) - 1)) == 0);
   const uint32_t elements = inputs_read >> VERT_ATTRIB_GENERIC0;
            return __builtin_popcount(elements) -
      }
      static void
   anv_graphics_pipeline_emit(struct anv_graphics_pipeline *pipeline,
         {
                        if (anv_pipeline_is_primitive(pipeline)) {
               /* The total number of vertex elements we need to program. We might need
   * a couple more to implement some of the draw parameters.
   */
   pipeline->svgs_count =
      (vs_prog_data->uses_vertexid ||
   vs_prog_data->uses_instanceid ||
                        pipeline->vertex_input_elems =
                  /* Our implementation of VK_KHR_multiview uses instancing to draw the
   * different views when primitive replication cannot be used.  If the
   * client asks for instancing, we need to multiply by the client's
   * instance count at draw time and instance divisor in the vertex
   * bindings by the number of views ensure that we repeat the client's
   * per-instance data once for each view.
   */
   const bool uses_primitive_replication =
         pipeline->instance_multiplier = 1;
   if (pipeline->view_mask && !uses_primitive_replication)
      } else {
      assert(anv_pipeline_is_mesh(pipeline));
               /* Store line mode and rasterization samples, these are used
   * for dynamic primitive topology.
   */
   pipeline->rasterization_samples =
            pipeline->dynamic_patch_control_points =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_CTRL) &&
   BITSET_TEST(state->dynamic, MESA_VK_DYNAMIC_TS_PATCH_CONTROL_POINTS) &&
   (pipeline->base.shaders[MESA_SHADER_TESS_CTRL]->dynamic_push_values &
         if (pipeline->base.shaders[MESA_SHADER_FRAGMENT]) {
               if (wm_prog_data_dynamic(wm_prog_data)) {
               assert(wm_prog_data->persample_dispatch == BRW_SOMETIMES);
                     if (wm_prog_data->sample_shading) {
                        if (state->ms->sample_shading_enable &&
      (state->ms->min_sample_shading * state->ms->rasterization_samples) > 1) {
   pipeline->fs_msaa_flags |= BRW_WM_MSAA_FLAG_PERSAMPLE_DISPATCH |
                                 assert(wm_prog_data->coarse_pixel_dispatch != BRW_ALWAYS);
   if (wm_prog_data->coarse_pixel_dispatch == BRW_SOMETIMES &&
      !(pipeline->fs_msaa_flags & BRW_WM_MSAA_FLAG_PERSAMPLE_DISPATCH) &&
   (!state->ms || !state->ms->sample_shading_enable)) {
   pipeline->fs_msaa_flags |= BRW_WM_MSAA_FLAG_COARSE_PI_MSG |
         } else {
      assert(wm_prog_data->alpha_to_coverage != BRW_SOMETIMES);
   assert(wm_prog_data->coarse_pixel_dispatch != BRW_SOMETIMES);
                  const struct anv_device *device = pipeline->base.base.device;
   const struct intel_device_info *devinfo = device->info;
      }
      static void
   anv_graphics_pipeline_import_layout(struct anv_graphics_base_pipeline *pipeline,
         {
               for (uint32_t s = 0; s < layout->num_sets; s++) {
      if (layout->set[s].layout == NULL)
            anv_pipeline_sets_layout_add(&pipeline->base.layout, s,
         }
      static void
   anv_graphics_pipeline_import_lib(struct anv_graphics_base_pipeline *pipeline,
                           {
      struct anv_pipeline_sets_layout *lib_layout =
                  /* We can't have shaders specified twice through libraries. */
            /* VK_EXT_graphics_pipeline_library:
   *
   *    "To perform link time optimizations,
   *     VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT must
   *     be specified on all pipeline libraries that are being linked
   *     together. Implementations should retain any additional information
   *     needed to perform optimizations at the final link step when this bit
   *     is present."
   */
                     /* Propagate the fragment dynamic flag, unless we're doing link
   * optimization, in that case we'll have all the state information and this
   * will never be dynamic.
   */
   if (!link_optimize) {
      if (lib->base.fragment_dynamic) {
      assert(lib->base.base.active_stages & VK_SHADER_STAGE_FRAGMENT_BIT);
                  uint32_t shader_count = anv_graphics_pipeline_imported_shader_count(stages);
   for (uint32_t s = 0; s < ARRAY_SIZE(lib->base.shaders); s++) {
      if (lib->base.shaders[s] == NULL)
            stages[s].stage = s;
   stages[s].feedback_idx = shader_count + lib->base.feedback_index[s];
            /* Always import the shader sha1, this will be used for cache lookup. */
   memcpy(stages[s].shader_sha1, lib->retained_shaders[s].shader_sha1,
                  stages[s].subgroup_size_type = lib->retained_shaders[s].subgroup_size_type;
   stages[s].imported.nir = lib->retained_shaders[s].nir;
            stages[s].bind_map = (struct anv_pipeline_bind_map) {
      .surface_to_descriptor = stages[s].surface_to_descriptor,
                  /* When not link optimizing, import the executables (shader descriptions
   * for VK_KHR_pipeline_executable_properties). With link optimization there
   * is a chance it'll produce different binaries, so we'll add the optimized
   * version later.
   */
   if (!link_optimize) {
      util_dynarray_foreach(&lib->base.base.executables,
            util_dynarray_append(&pipeline->base.executables,
            }
      static void
   anv_graphics_lib_validate_shaders(struct anv_graphics_lib_pipeline *lib,
         {
      for (uint32_t s = 0; s < ARRAY_SIZE(lib->retained_shaders); s++) {
      if (anv_pipeline_base_has_stage(&lib->base, s)) {
      assert(!retained_shaders || lib->retained_shaders[s].nir != NULL);
            }
      static VkResult
   anv_graphics_lib_pipeline_create(struct anv_device *device,
                           {
      struct anv_pipeline_stage stages[ANV_GRAPHICS_SHADER_STAGE_COUNT] = {};
   VkPipelineCreationFeedbackEXT pipeline_feedback = {
         };
            struct anv_graphics_lib_pipeline *pipeline;
            const VkPipelineCreateFlags2KHR flags =
                  const VkPipelineLibraryCreateInfoKHR *libs_info =
      vk_find_struct_const(pCreateInfo->pNext,
         pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
         if (pipeline == NULL)
            result = anv_pipeline_init(&pipeline->base.base, device,
               if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
   if (result == VK_PIPELINE_COMPILE_REQUIRED_EXT)
                     /* Capture the retain state before we compile/load any shader. */
   pipeline->retain_shaders =
            /* If we have libraries, import them first. */
   if (libs_info) {
      for (uint32_t i = 0; i < libs_info->libraryCount; i++) {
      ANV_FROM_HANDLE(anv_pipeline, pipeline_lib, libs_info->pLibraries[i]);
                  vk_graphics_pipeline_state_merge(&pipeline->state, &gfx_pipeline_lib->state);
   anv_graphics_pipeline_import_lib(&pipeline->base,
                              result = vk_graphics_pipeline_state_fill(&device->vk,
                     if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base.base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
                        /* After we've imported all the libraries' layouts, import the pipeline
   * layout and hash the whole lot.
   */
   ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
   if (pipeline_layout != NULL) {
      anv_graphics_pipeline_import_layout(&pipeline->base,
                        /* Compile shaders. We can skip this if there are no active stage in that
   * pipeline.
   */
   if (pipeline->base.base.active_stages != 0) {
      result = anv_graphics_pipeline_compile(&pipeline->base, stages,
               if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base.base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
                           anv_fill_pipeline_creation_feedback(&pipeline->base, &pipeline_feedback,
            anv_graphics_lib_validate_shaders(
      pipeline,
                     }
      static VkResult
   anv_graphics_pipeline_create(struct anv_device *device,
                           {
      struct anv_pipeline_stage stages[ANV_GRAPHICS_SHADER_STAGE_COUNT] = {};
   VkPipelineCreationFeedbackEXT pipeline_feedback = {
         };
            struct anv_graphics_pipeline *pipeline;
            const VkPipelineCreateFlags2KHR flags =
                  const VkPipelineLibraryCreateInfoKHR *libs_info =
      vk_find_struct_const(pCreateInfo->pNext,
         pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
         if (pipeline == NULL)
            /* Initialize some information required by shaders */
   result = anv_pipeline_init(&pipeline->base.base, device,
               if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
               const bool link_optimize =
            struct vk_graphics_pipeline_all_state all;
            /* If we have libraries, import them first. */
   if (libs_info) {
      for (uint32_t i = 0; i < libs_info->libraryCount; i++) {
      ANV_FROM_HANDLE(anv_pipeline, pipeline_lib, libs_info->pLibraries[i]);
                  /* If we have link time optimization, all libraries must be created
   * with
   * VK_PIPELINE_CREATE_RETAIN_LINK_TIME_OPTIMIZATION_INFO_BIT_EXT.
                  vk_graphics_pipeline_state_merge(&state, &gfx_pipeline_lib->state);
   anv_graphics_pipeline_import_lib(&pipeline->base,
                                    result = vk_graphics_pipeline_state_fill(&device->vk, &state, pCreateInfo,
               if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base.base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
               pipeline->dynamic_state.vi = &pipeline->vertex_input;
   pipeline->dynamic_state.ms.sample_locations = &pipeline->base.sample_locations;
                     /* Sanity check on the shaders */
   assert(pipeline->base.base.active_stages & VK_SHADER_STAGE_VERTEX_BIT ||
            if (anv_pipeline_is_mesh(pipeline)) {
                  /* After we've imported all the libraries' layouts, import the pipeline
   * layout and hash the whole lot.
   */
   ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
   if (pipeline_layout != NULL) {
      anv_graphics_pipeline_import_layout(&pipeline->base,
                        /* Compile shaders, all required information should be have been copied in
   * the previous step. We can skip this if there are no active stage in that
   * pipeline.
   */
   result = anv_graphics_pipeline_compile(&pipeline->base, stages,
               if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base.base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
               /* Prepare a batch for the commands and emit all the non dynamic ones.
   */
   anv_batch_set_storage(&pipeline->base.base.batch, ANV_NULL_ADDRESS,
            if (pipeline->base.base.active_stages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            if (anv_pipeline_is_mesh(pipeline))
                              anv_fill_pipeline_creation_feedback(&pipeline->base, &pipeline_feedback,
                        }
      VkResult anv_CreateGraphicsPipelines(
      VkDevice                                    _device,
   VkPipelineCache                             pipelineCache,
   uint32_t                                    count,
   const VkGraphicsPipelineCreateInfo*         pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     unsigned i;
   for (i = 0; i < count; i++) {
               const VkPipelineCreateFlags2KHR flags =
         VkResult res;
   if (flags & VK_PIPELINE_CREATE_2_LIBRARY_BIT_KHR) {
      res = anv_graphics_lib_pipeline_create(device, pipeline_cache,
                  } else {
      res = anv_graphics_pipeline_create(device,
                           if (res == VK_SUCCESS)
            /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED as it
   * is not obvious what error should be report upon 2 different failures.
   * */
   result = res;
   if (res != VK_PIPELINE_COMPILE_REQUIRED)
                     if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
               for (; i < count; i++)
               }
      static bool
   should_remat_cb(nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_intrinsic)
               }
      static VkResult
   compile_upload_rt_shader(struct anv_ray_tracing_pipeline *pipeline,
                           {
      const struct brw_compiler *compiler =
                  nir_shader **resume_shaders = NULL;
   uint32_t num_resume_shaders = 0;
   if (nir->info.stage != MESA_SHADER_COMPUTE) {
      const nir_lower_shader_calls_options opts = {
      .address_format = nir_address_format_64bit_global,
   .stack_alignment = BRW_BTD_STACK_ALIGN,
   .localized_loads = true,
   .vectorizer_callback = brw_nir_should_vectorize_mem,
   .vectorizer_data = NULL,
               NIR_PASS(_, nir, nir_lower_shader_calls, &opts,
         NIR_PASS(_, nir, brw_nir_lower_shader_calls, &stage->key.bs);
               for (unsigned i = 0; i < num_resume_shaders; i++) {
      NIR_PASS(_,resume_shaders[i], brw_nir_lower_shader_calls, &stage->key.bs);
               struct brw_compile_bs_params params = {
      .base = {
      .nir = nir,
   .stats = stage->stats,
   .log_data = pipeline->base.device,
      },
   .key = &stage->key.bs,
   .prog_data = &stage->prog_data.bs,
   .num_resume_shaders = num_resume_shaders,
               stage->code = brw_compile_bs(compiler, &params);
   if (stage->code == NULL)
            /* Ray-tracing shaders don't have a "real" bind map */
            const unsigned code_size = stage->prog_data.base.program_size;
   stage->bin =
      anv_device_upload_kernel(pipeline->base.device,
                           cache,
   stage->stage,
   &stage->cache_key, sizeof(stage->cache_key),
   stage->code, code_size,
   &stage->prog_data.base,
      if (stage->bin == NULL)
                        }
      static bool
   is_rt_stack_size_dynamic(const VkRayTracingPipelineCreateInfoKHR *info)
   {
      if (info->pDynamicState == NULL)
            for (unsigned i = 0; i < info->pDynamicState->dynamicStateCount; i++) {
      if (info->pDynamicState->pDynamicStates[i] ==
      VK_DYNAMIC_STATE_RAY_TRACING_PIPELINE_STACK_SIZE_KHR)
               }
      static void
   anv_pipeline_compute_ray_tracing_stacks(struct anv_ray_tracing_pipeline *pipeline,
               {
      if (is_rt_stack_size_dynamic(info)) {
         } else {
      /* From the Vulkan spec:
   *
   *    "If the stack size is not set explicitly, the stack size for a
   *    pipeline is:
   *
   *       rayGenStackMax +
   *       min(1, maxPipelineRayRecursionDepth) ×
   *       max(closestHitStackMax, missStackMax,
   *           intersectionStackMax + anyHitStackMax) +
   *       max(0, maxPipelineRayRecursionDepth-1) ×
   *       max(closestHitStackMax, missStackMax) +
   *       2 × callableStackMax"
   */
   pipeline->stack_size =
      stack_max[MESA_SHADER_RAYGEN] +
   MIN2(1, info->maxPipelineRayRecursionDepth) *
   MAX4(stack_max[MESA_SHADER_CLOSEST_HIT],
      stack_max[MESA_SHADER_MISS],
   stack_max[MESA_SHADER_INTERSECTION],
      MAX2(0, (int)info->maxPipelineRayRecursionDepth - 1) *
   MAX2(stack_max[MESA_SHADER_CLOSEST_HIT],
               /* This is an extremely unlikely case but we need to set it to some
   * non-zero value so that we don't accidentally think it's dynamic.
   * Our minimum stack size is 2KB anyway so we could set to any small
   * value we like.
   */
   if (pipeline->stack_size == 0)
         }
      static enum brw_rt_ray_flags
   anv_pipeline_get_pipeline_ray_flags(VkPipelineCreateFlags2KHR flags)
   {
               const bool rt_skip_triangles =
         const bool rt_skip_aabbs =
                  if (rt_skip_triangles)
         else if (rt_skip_aabbs)
               }
      static struct anv_pipeline_stage *
   anv_pipeline_init_ray_tracing_stages(struct anv_ray_tracing_pipeline *pipeline,
               {
      struct anv_device *device = pipeline->base.device;
   /* Create enough stage entries for all shader modules plus potential
   * combinaisons in the groups.
   */
   struct anv_pipeline_stage *stages =
            enum brw_rt_ray_flags ray_flags =
            for (uint32_t i = 0; i < info->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &info->pStages[i];
   if (vk_pipeline_shader_stage_is_null(sinfo))
                     stages[i] = (struct anv_pipeline_stage) {
      .stage = vk_to_mesa_shader_stage(sinfo->stage),
   .pipeline_pNext = info->pNext,
   .info = sinfo,
   .cache_key = {
         },
   .feedback = {
                                       populate_bs_prog_key(&stages[i],
                  if (stages[i].stage != MESA_SHADER_INTERSECTION) {
      anv_pipeline_hash_ray_tracing_shader(pipeline, &stages[i],
                           for (uint32_t i = 0; i < info->groupCount; i++) {
               if (ginfo->type != VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR)
                     uint32_t intersection_idx = ginfo->intersectionShader;
            uint32_t any_hit_idx = ginfo->anyHitShader;
   if (any_hit_idx != VK_SHADER_UNUSED_KHR) {
      assert(any_hit_idx < info->stageCount);
   anv_pipeline_hash_ray_tracing_combined_shader(pipeline,
                  } else {
      anv_pipeline_hash_ray_tracing_shader(pipeline,
                                    }
      static bool
   anv_ray_tracing_pipeline_load_cached_shaders(struct anv_ray_tracing_pipeline *pipeline,
                     {
      uint32_t shaders = 0, cache_hits = 0;
   for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].info == NULL)
                              bool cache_hit;
   stages[i].bin = anv_device_search_for_kernel(pipeline->base.device, cache,
                     if (cache_hit) {
      cache_hits++;
   stages[i].feedback.flags |=
               if (stages[i].bin != NULL)
                           }
      static VkResult
   anv_pipeline_compile_ray_tracing(struct anv_ray_tracing_pipeline *pipeline,
                           {
      const struct intel_device_info *devinfo = pipeline->base.device->info;
            VkPipelineCreationFeedback pipeline_feedback = {
         };
            const bool skip_cache_lookup =
            if (!skip_cache_lookup &&
      anv_ray_tracing_pipeline_load_cached_shaders(pipeline, cache, info, stages)) {
   pipeline_feedback.flags |=
                     if (pipeline->base.flags & VK_PIPELINE_CREATE_2_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT_KHR)
            for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].info == NULL)
                     stages[i].nir = anv_pipeline_stage_get_nir(&pipeline->base, cache,
         if (stages[i].nir == NULL)
                     anv_pipeline_lower_nir(&pipeline->base, tmp_pipeline_ctx, &stages[i],
                              for (uint32_t i = 0; i < info->stageCount; i++) {
      if (stages[i].info == NULL)
            /* Shader found in cache already. */
   if (stages[i].bin != NULL)
            /* We handle intersection shaders as part of the group */
   if (stages[i].stage == MESA_SHADER_INTERSECTION)
                              nir_shader *nir = nir_shader_clone(tmp_stage_ctx, stages[i].nir);
   switch (stages[i].stage) {
   case MESA_SHADER_RAYGEN:
                  case MESA_SHADER_ANY_HIT:
                  case MESA_SHADER_CLOSEST_HIT:
                  case MESA_SHADER_MISS:
                  case MESA_SHADER_INTERSECTION:
            case MESA_SHADER_CALLABLE:
                  default:
                  result = compile_upload_rt_shader(pipeline, cache, nir, &stages[i],
         if (result != VK_SUCCESS) {
      ralloc_free(tmp_stage_ctx);
                                 done:
      for (uint32_t i = 0; i < info->groupCount; i++) {
      const VkRayTracingShaderGroupCreateInfoKHR *ginfo = &info->pGroups[i];
   struct anv_rt_shader_group *group = &pipeline->groups[i];
   group->type = ginfo->type;
   switch (ginfo->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
      assert(ginfo->generalShader < info->stageCount);
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
                     if (ginfo->closestHitShader < info->stageCount)
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR: {
                                    /* Only compile this stage if not already found in the cache. */
   if (stages[intersection_idx].bin == NULL) {
      /* The any-hit and intersection shader have to be combined */
   uint32_t any_hit_idx = info->pGroups[i].anyHitShader;
   const nir_shader *any_hit = NULL;
                  void *tmp_group_ctx = ralloc_context(tmp_pipeline_ctx);
                                 result = compile_upload_rt_shader(pipeline, cache,
                     ralloc_free(tmp_group_ctx);
   if (result != VK_SUCCESS)
               group->intersection = stages[intersection_idx].bin;
               default:
                              const VkPipelineCreationFeedbackCreateInfo *create_feedback =
         if (create_feedback) {
               uint32_t stage_count = create_feedback->pipelineStageCreationFeedbackCount;
   assert(stage_count == 0 || info->stageCount == stage_count);
   for (uint32_t i = 0; i < stage_count; i++) {
      gl_shader_stage s = vk_to_mesa_shader_stage(info->pStages[i].stage);
                     }
      VkResult
   anv_device_init_rt_shaders(struct anv_device *device)
   {
               if (!device->vk.enabled_extensions.KHR_ray_tracing_pipeline)
                     struct brw_rt_trampoline {
      char name[16];
      } trampoline_key = {
         };
   device->rt_trampoline =
      anv_device_search_for_kernel(device, device->internal_cache,
                        void *tmp_ctx = ralloc_context(NULL);
   nir_shader *trampoline_nir =
                     struct anv_push_descriptor_info push_desc_info = {};
   struct anv_pipeline_bind_map bind_map = {
      .surface_count = 0,
      };
   uint32_t dummy_params[4] = { 0, };
   struct brw_cs_prog_data trampoline_prog_data = {
      .base.nr_params = 4,
   .base.param = dummy_params,
   .uses_inline_data = true,
      };
   struct brw_compile_cs_params params = {
      .base = {
      .nir = trampoline_nir,
   .log_data = device,
      },
   .key = &trampoline_key.key,
      };
   const unsigned *tramp_data =
            device->rt_trampoline =
      anv_device_upload_kernel(device, device->internal_cache,
                           MESA_SHADER_COMPUTE,
   &trampoline_key, sizeof(trampoline_key),
   tramp_data,
                        if (device->rt_trampoline == NULL)
               /* The cache already has a reference and it's not going anywhere so there
   * is no need to hold a second reference.
   */
            struct brw_rt_trivial_return {
      char name[16];
      } return_key = {
         };
   device->rt_trivial_return =
      anv_device_search_for_kernel(device, device->internal_cache,
            if (device->rt_trivial_return == NULL) {
      void *tmp_ctx = ralloc_context(NULL);
   nir_shader *trivial_return_nir =
                     struct anv_push_descriptor_info push_desc_info = {};
   struct anv_pipeline_bind_map bind_map = {
      .surface_count = 0,
      };
   struct brw_bs_prog_data return_prog_data = { 0, };
   struct brw_compile_bs_params params = {
      .base = {
      .nir = trivial_return_nir,
   .log_data = device,
      },
   .key = &return_key.key,
      };
   const unsigned *return_data =
            device->rt_trivial_return =
      anv_device_upload_kernel(device, device->internal_cache,
                           MESA_SHADER_CALLABLE,
                        if (device->rt_trivial_return == NULL)
               /* The cache already has a reference and it's not going anywhere so there
   * is no need to hold a second reference.
   */
               }
      void
   anv_device_finish_rt_shaders(struct anv_device *device)
   {
      if (!device->vk.enabled_extensions.KHR_ray_tracing_pipeline)
      }
      static void
   anv_ray_tracing_pipeline_init(struct anv_ray_tracing_pipeline *pipeline,
                           {
               ANV_FROM_HANDLE(anv_pipeline_layout, pipeline_layout, pCreateInfo->layout);
               }
      static void
   assert_rt_stage_index_valid(const VkRayTracingPipelineCreateInfoKHR* pCreateInfo,
               {
      if (stage_idx == VK_SHADER_UNUSED_KHR)
            assert(stage_idx <= pCreateInfo->stageCount);
   assert(util_bitcount(pCreateInfo->pStages[stage_idx].stage) == 1);
      }
      static VkResult
   anv_ray_tracing_pipeline_create(
      VkDevice                                    _device,
   struct vk_pipeline_cache *                  cache,
   const VkRayTracingPipelineCreateInfoKHR*    pCreateInfo,
   const VkAllocationCallbacks*                pAllocator,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
                     uint32_t group_count = pCreateInfo->groupCount;
   if (pCreateInfo->pLibraryInfo) {
      for (uint32_t l = 0; l < pCreateInfo->pLibraryInfo->libraryCount; l++) {
      ANV_FROM_HANDLE(anv_pipeline, library,
         struct anv_ray_tracing_pipeline *rt_library =
                        VK_MULTIALLOC(ma);
   VK_MULTIALLOC_DECL(&ma, struct anv_ray_tracing_pipeline, pipeline, 1);
   VK_MULTIALLOC_DECL(&ma, struct anv_rt_shader_group, groups, group_count);
   if (!vk_multialloc_zalloc2(&ma, &device->vk.alloc, pAllocator,
                  result = anv_pipeline_init(&pipeline->base, device,
                           if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
               pipeline->group_count = group_count;
            ASSERTED const VkShaderStageFlags ray_tracing_stages =
      VK_SHADER_STAGE_RAYGEN_BIT_KHR |
   VK_SHADER_STAGE_ANY_HIT_BIT_KHR |
   VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
   VK_SHADER_STAGE_MISS_BIT_KHR |
   VK_SHADER_STAGE_INTERSECTION_BIT_KHR |
         for (uint32_t i = 0; i < pCreateInfo->stageCount; i++)
            for (uint32_t i = 0; i < pCreateInfo->groupCount; i++) {
      const VkRayTracingShaderGroupCreateInfoKHR *ginfo =
         assert_rt_stage_index_valid(pCreateInfo, ginfo->generalShader,
                     assert_rt_stage_index_valid(pCreateInfo, ginfo->closestHitShader,
         assert_rt_stage_index_valid(pCreateInfo, ginfo->anyHitShader,
         assert_rt_stage_index_valid(pCreateInfo, ginfo->intersectionShader,
         switch (ginfo->type) {
   case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
      assert(ginfo->generalShader < pCreateInfo->stageCount);
   assert(ginfo->anyHitShader == VK_SHADER_UNUSED_KHR);
   assert(ginfo->closestHitShader == VK_SHADER_UNUSED_KHR);
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
      assert(ginfo->generalShader == VK_SHADER_UNUSED_KHR);
               case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
                  default:
                     anv_ray_tracing_pipeline_init(pipeline, device, cache,
                     struct anv_pipeline_stage *stages =
            result = anv_pipeline_compile_ray_tracing(pipeline, tmp_ctx, stages,
         if (result != VK_SUCCESS) {
      ralloc_free(tmp_ctx);
   util_dynarray_foreach(&pipeline->shaders, struct anv_shader_bin *, shader)
         anv_pipeline_finish(&pipeline->base, device);
   vk_free2(&device->vk.alloc, pAllocator, pipeline);
               /* Compute the size of the scratch BO (for register spilling) by taking the
   * max of all the shaders in the pipeline. Also add the shaders to the list
   * of executables.
   */
   uint32_t stack_max[MESA_VULKAN_SHADER_STAGES] = {};
   for (uint32_t s = 0; s < pCreateInfo->stageCount; s++) {
      util_dynarray_append(&pipeline->shaders,
                  uint32_t stack_size =
                                       if (pCreateInfo->pLibraryInfo) {
      uint32_t g = pCreateInfo->groupCount;
   for (uint32_t l = 0; l < pCreateInfo->pLibraryInfo->libraryCount; l++) {
      ANV_FROM_HANDLE(anv_pipeline, library,
         struct anv_ray_tracing_pipeline *rt_library =
                        /* Account for shaders in the library. */
   util_dynarray_foreach(&rt_library->shaders,
            util_dynarray_append(&pipeline->shaders,
                           /* Add the library shaders to this pipeline's executables. */
   util_dynarray_foreach(&rt_library->base.executables,
            util_dynarray_append(&pipeline->base.executables,
                                                            }
      VkResult
   anv_CreateRayTracingPipelinesKHR(
      VkDevice                                    _device,
   VkDeferredOperationKHR                      deferredOperation,
   VkPipelineCache                             pipelineCache,
   uint32_t                                    createInfoCount,
   const VkRayTracingPipelineCreateInfoKHR*    pCreateInfos,
   const VkAllocationCallbacks*                pAllocator,
      {
                        unsigned i;
   for (i = 0; i < createInfoCount; i++) {
      const VkPipelineCreateFlags2KHR flags =
         VkResult res = anv_ray_tracing_pipeline_create(_device, pipeline_cache,
                  if (res == VK_SUCCESS)
            /* Bail out on the first error as it is not obvious what error should be
   * report upon 2 different failures. */
   result = res;
   if (result != VK_PIPELINE_COMPILE_REQUIRED)
                     if (flags & VK_PIPELINE_CREATE_2_EARLY_RETURN_ON_FAILURE_BIT_KHR)
               for (; i < createInfoCount; i++)
               }
      #define WRITE_STR(field, ...) ({                               \
      memset(field, 0, sizeof(field));                            \
   UNUSED int i = snprintf(field, sizeof(field), __VA_ARGS__); \
      })
      VkResult anv_GetPipelineExecutablePropertiesKHR(
      VkDevice                                    device,
   const VkPipelineInfoKHR*                    pPipelineInfo,
   uint32_t*                                   pExecutableCount,
      {
      ANV_FROM_HANDLE(anv_pipeline, pipeline, pPipelineInfo->pipeline);
   VK_OUTARRAY_MAKE_TYPED(VkPipelineExecutablePropertiesKHR, out,
            util_dynarray_foreach (&pipeline->executables, struct anv_pipeline_executable, exe) {
      vk_outarray_append_typed(VkPipelineExecutablePropertiesKHR, &out, props) {
                     unsigned simd_width = exe->stats.dispatch_width;
   if (stage == MESA_SHADER_FRAGMENT) {
      WRITE_STR(props->name, "%s%d %s",
            simd_width ? "SIMD" : "vec",
   } else {
         }
   WRITE_STR(props->description, "%s%d %s shader",
                        /* The compiler gives us a dispatch width of 0 for vec4 but Vulkan
   * wants a subgroup size of 1.
   */
                     }
      static const struct anv_pipeline_executable *
   anv_pipeline_get_executable(struct anv_pipeline *pipeline, uint32_t index)
   {
      assert(index < util_dynarray_num_elements(&pipeline->executables,
         return util_dynarray_element(
      }
      VkResult anv_GetPipelineExecutableStatisticsKHR(
      VkDevice                                    device,
   const VkPipelineExecutableInfoKHR*          pExecutableInfo,
   uint32_t*                                   pStatisticCount,
      {
      ANV_FROM_HANDLE(anv_pipeline, pipeline, pExecutableInfo->pipeline);
   VK_OUTARRAY_MAKE_TYPED(VkPipelineExecutableStatisticKHR, out,
            const struct anv_pipeline_executable *exe =
            const struct brw_stage_prog_data *prog_data;
   switch (pipeline->type) {
   case ANV_PIPELINE_GRAPHICS:
   case ANV_PIPELINE_GRAPHICS_LIB: {
      prog_data = anv_pipeline_to_graphics(pipeline)->base.shaders[exe->stage]->prog_data;
      }
   case ANV_PIPELINE_COMPUTE: {
      prog_data = anv_pipeline_to_compute(pipeline)->cs->prog_data;
      }
   case ANV_PIPELINE_RAY_TRACING: {
      struct anv_shader_bin **shader =
      util_dynarray_element(&anv_pipeline_to_ray_tracing(pipeline)->shaders,
            prog_data = (*shader)->prog_data;
      }
   default:
                  vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Instruction Count");
   WRITE_STR(stat->description,
               stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "SEND Count");
   WRITE_STR(stat->description,
            "Number of instructions in the final generated shader "
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Loop Count");
   WRITE_STR(stat->description,
               stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Cycle Count");
   WRITE_STR(stat->description,
            "Estimate of the number of EU cycles required to execute "
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Spill Count");
   WRITE_STR(stat->description,
            "Number of scratch spill operations.  This gives a rough "
   "estimate of the cost incurred due to spilling temporary "
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Fill Count");
   WRITE_STR(stat->description,
            "Number of scratch fill operations.  This gives a rough "
   "estimate of the cost incurred due to spilling temporary "
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Scratch Memory Size");
   WRITE_STR(stat->description,
            "Number of bytes of scratch memory required by the "
   "generated shader executable.  If this is non-zero, you "
      stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Max dispatch width");
   WRITE_STR(stat->description,
         stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
   /* Report the max dispatch width only on the smallest SIMD variant */
   if (exe->stage != MESA_SHADER_FRAGMENT || exe->stats.dispatch_width == 8)
         else
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Max live registers");
   WRITE_STR(stat->description,
         stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Workgroup Memory Size");
   WRITE_STR(stat->description,
               stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
   if (gl_shader_stage_uses_workgroup(exe->stage))
         else
               vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      uint32_t hash = pipeline->type == ANV_PIPELINE_COMPUTE ?
                  anv_pipeline_to_compute(pipeline)->source_hash :
   (pipeline->type == ANV_PIPELINE_GRAPHICS_LIB ||
      WRITE_STR(stat->name, "Source hash");
   WRITE_STR(stat->description,
         stat->format = VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR;
                  }
      static bool
   write_ir_text(VkPipelineExecutableInternalRepresentationKHR* ir,
         {
                        if (ir->pData == NULL) {
      ir->dataSize = data_len;
               strncpy(ir->pData, data, ir->dataSize);
   if (ir->dataSize < data_len)
            ir->dataSize = data_len;
      }
      VkResult anv_GetPipelineExecutableInternalRepresentationsKHR(
      VkDevice                                    device,
   const VkPipelineExecutableInfoKHR*          pExecutableInfo,
   uint32_t*                                   pInternalRepresentationCount,
      {
      ANV_FROM_HANDLE(anv_pipeline, pipeline, pExecutableInfo->pipeline);
   VK_OUTARRAY_MAKE_TYPED(VkPipelineExecutableInternalRepresentationKHR, out,
                  const struct anv_pipeline_executable *exe =
            if (exe->nir) {
      vk_outarray_append_typed(VkPipelineExecutableInternalRepresentationKHR, &out, ir) {
      WRITE_STR(ir->name, "Final NIR");
                  if (!write_ir_text(ir, exe->nir))
                  if (exe->disasm) {
      vk_outarray_append_typed(VkPipelineExecutableInternalRepresentationKHR, &out, ir) {
      WRITE_STR(ir->name, "GEN Assembly");
                  if (!write_ir_text(ir, exe->disasm))
                     }
      VkResult
   anv_GetRayTracingShaderGroupHandlesKHR(
      VkDevice                                    _device,
   VkPipeline                                  _pipeline,
   uint32_t                                    firstGroup,
   uint32_t                                    groupCount,
   size_t                                      dataSize,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (pipeline->type != ANV_PIPELINE_RAY_TRACING)
            struct anv_ray_tracing_pipeline *rt_pipeline =
            assert(firstGroup + groupCount <= rt_pipeline->group_count);
   for (uint32_t i = 0; i < groupCount; i++) {
      struct anv_rt_shader_group *group = &rt_pipeline->groups[firstGroup + i];
   memcpy(pData, group->handle, sizeof(group->handle));
                  }
      VkResult
   anv_GetRayTracingCaptureReplayShaderGroupHandlesKHR(
      VkDevice                                    _device,
   VkPipeline                                  pipeline,
   uint32_t                                    firstGroup,
   uint32_t                                    groupCount,
   size_t                                      dataSize,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
   unreachable("Unimplemented");
      }
      VkDeviceSize
   anv_GetRayTracingShaderGroupStackSizeKHR(
      VkDevice                                    device,
   VkPipeline                                  _pipeline,
   uint32_t                                    group,
      {
      ANV_FROM_HANDLE(anv_pipeline, pipeline, _pipeline);
            struct anv_ray_tracing_pipeline *rt_pipeline =
                     struct anv_shader_bin *bin;
   switch (groupShader) {
   case VK_SHADER_GROUP_SHADER_GENERAL_KHR:
      bin = rt_pipeline->groups[group].general;
         case VK_SHADER_GROUP_SHADER_CLOSEST_HIT_KHR:
      bin = rt_pipeline->groups[group].closest_hit;
         case VK_SHADER_GROUP_SHADER_ANY_HIT_KHR:
      bin = rt_pipeline->groups[group].any_hit;
         case VK_SHADER_GROUP_SHADER_INTERSECTION_KHR:
      bin = rt_pipeline->groups[group].intersection;
         default:
                  if (bin == NULL)
               }
