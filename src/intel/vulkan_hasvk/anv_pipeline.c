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
   #include "vk_pipeline.h"
   #include "vk_render_pass.h"
   #include "vk_util.h"
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
            const struct spirv_to_nir_options spirv_options = {
      .caps = {
      .demote_to_helper_invocation = true,
   .derivative_group = true,
   .descriptor_array_dynamic_indexing = true,
   .descriptor_array_non_uniform_indexing = true,
   .descriptor_indexing = true,
   .device_group = true,
   .draw_parameters = true,
   .float16 = pdevice->info.ver >= 8,
   .float32_atomic_add = pdevice->info.has_lsc,
   .float32_atomic_min_max = pdevice->info.ver >= 9,
   .float64 = pdevice->info.ver >= 8,
   .float64_atomic_min_max = pdevice->info.has_lsc,
   .fragment_shader_sample_interlock = pdevice->info.ver >= 9,
   .fragment_shader_pixel_interlock = pdevice->info.ver >= 9,
   .geometry_streams = true,
   /* When using Vulkan 1.3 or KHR_format_feature_flags2 is enabled, the
   * read/write without format is per format, so just report true. It's
   * up to the application to check.
   */
   .image_read_without_format = instance->vk.app_info.api_version >= VK_API_VERSION_1_3 || device->vk.enabled_extensions.KHR_format_feature_flags2,
   .image_write_without_format = true,
   .int8 = pdevice->info.ver >= 8,
   .int16 = pdevice->info.ver >= 8,
   .int64 = pdevice->info.ver >= 8,
   .int64_atomics = pdevice->info.ver >= 9 && pdevice->use_softpin,
   .integer_functions2 = pdevice->info.ver >= 8,
   .min_lod = true,
   .multiview = true,
   .physical_storage_buffer_address = pdevice->has_a64_buffer_access,
   .post_depth_coverage = pdevice->info.ver >= 9,
   .runtime_descriptor_array = true,
   .float_controls = true,
   .shader_clock = true,
   .shader_viewport_index_layer = true,
   .stencil_export = pdevice->info.ver >= 9,
   .storage_8bit = pdevice->info.ver >= 8,
   .storage_16bit = pdevice->info.ver >= 8,
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
            const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
         };
            const nir_opt_access_options opt_access_options = {
         };
            /* Vulkan uses the separate-shader linking model */
                                 }
      VkResult
   anv_pipeline_init(struct anv_pipeline *pipeline,
                     struct anv_device *device,
   {
                        vk_object_base_init(&device->vk, &pipeline->base,
                  /* It's the job of the child class to provide actual backing storage for
   * the batch by setting batch.start, batch.next, and batch.end.
   */
   pipeline->batch.alloc = pAllocator ? pAllocator : &device->vk.alloc;
   pipeline->batch.relocs = &pipeline->batch_relocs;
            result = anv_reloc_list_init(&pipeline->batch_relocs,
         if (result != VK_SUCCESS)
                     pipeline->type = type;
                        }
      void
   anv_pipeline_finish(struct anv_pipeline *pipeline,
               {
      anv_reloc_list_finish(&pipeline->batch_relocs,
         ralloc_free(pipeline->mem_ctx);
      }
      void anv_DestroyPipeline(
      VkDevice                                    _device,
   VkPipeline                                  _pipeline,
      {
      ANV_FROM_HANDLE(anv_device, device, _device);
            if (!pipeline)
            switch (pipeline->type) {
   case ANV_PIPELINE_GRAPHICS: {
      struct anv_graphics_pipeline *gfx_pipeline =
            for (unsigned s = 0; s < ARRAY_SIZE(gfx_pipeline->shaders); s++) {
      if (gfx_pipeline->shaders[s])
      }
               case ANV_PIPELINE_COMPUTE: {
      struct anv_compute_pipeline *compute_pipeline =
            if (compute_pipeline->cs)
                        default:
                  anv_pipeline_finish(pipeline, device, pAllocator);
      }
      static void
   populate_sampler_prog_key(const struct intel_device_info *devinfo,
         {
         }
      static void
   populate_base_prog_key(const struct anv_device *device,
               {
      key->robust_flags = robust_flags;
   key->limit_trig_input_range =
               }
      static void
   populate_vs_prog_key(const struct anv_device *device,
               {
                                    }
      static void
   populate_tcs_prog_key(const struct anv_device *device,
                     {
                           }
      static void
   populate_tes_prog_key(const struct anv_device *device,
               {
                  }
      static void
   populate_gs_prog_key(const struct anv_device *device,
               {
                  }
      static void
   populate_wm_prog_key(const struct anv_graphics_pipeline *pipeline,
                        enum brw_robustness_flags robust_flags,
      {
                                 /* We set this to 0 here and set to the actual value before we call
   * brw_compile_fs.
   */
            /* XXX Vulkan doesn't appear to specify */
                     assert(rp->color_attachment_count <= MAX_RTS);
   /* Consider all inputs as valid until look at the NIR variables. */
   key->color_outputs_valid = (1u << rp->color_attachment_count) - 1;
            /* To reduce possible shader recompilations we would need to know if
   * there is a SampleMask output variable to compute if we should emit
   * code to workaround the issue that hardware disables alpha to coverage
   * when there is SampleMask output.
   */
   key->alpha_to_coverage = ms != NULL && ms->alpha_to_coverage_enable ?
            /* Vulkan doesn't support fixed-function alpha test */
            if (ms != NULL) {
      /* We should probably pull this out of the shader, but it's fairly
   * harmless to compute it and then let dead-code take care of it.
   */
   if (ms->rasterization_samples > 1) {
      key->persample_interp =
      (ms->sample_shading_enable &&
   (ms->min_sample_shading * ms->rasterization_samples) > 1) ?
                  if (device->physical->instance->sample_mask_out_opengl_behaviour)
         }
      static void
   populate_cs_prog_key(const struct anv_device *device,
               {
                  }
      static void
   populate_bs_prog_key(const struct anv_device *device,
               {
                  }
      struct anv_pipeline_stage {
                                          struct {
      gl_shader_stage stage;
                        struct anv_pipeline_binding surface_to_descriptor[256];
   struct anv_pipeline_binding sampler_to_descriptor[256];
                     uint32_t num_stats;
   struct brw_compile_stats stats[3];
                                 };
      static void
   anv_pipeline_hash_graphics(struct anv_graphics_pipeline *pipeline,
                     {
      struct mesa_sha1 ctx;
            _mesa_sha1_update(&ctx, &pipeline->view_mask,
            if (layout)
            for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (stages[s].info) {
      _mesa_sha1_update(&ctx, stages[s].shader_sha1,
                           }
      static void
   anv_pipeline_hash_compute(struct anv_compute_pipeline *pipeline,
                     {
      struct mesa_sha1 ctx;
            if (layout)
                     const bool rba = device->vk.enabled_features.robustBufferAccess;
            const uint8_t afs = device->physical->instance->assume_full_subgroups;
            _mesa_sha1_update(&ctx, stage->shader_sha1,
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
      static void
   shared_type_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type)
         unsigned length = glsl_get_vector_elements(type);
   *size = comp_size * length,
      }
      static void
   anv_pipeline_lower_nir(struct anv_pipeline *pipeline,
                     {
      const struct anv_physical_device *pdevice = pipeline->device->physical;
            struct brw_stage_prog_data *prog_data = &stage->prog_data.base;
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      NIR_PASS(_, nir, nir_lower_wpos_center);
   NIR_PASS(_, nir, nir_lower_input_attachments,
            &(nir_input_attachment_options) {
                           if (pipeline->type == ANV_PIPELINE_GRAPHICS) {
      struct anv_graphics_pipeline *gfx_pipeline =
                              NIR_PASS(_, nir, brw_nir_lower_storage_image,
            &(struct brw_nir_lower_storage_image_opts) {
      .devinfo = compiler->devinfo,
   .lower_loads = true,
   .lower_stores = true,
            NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_global,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_push_const,
            /* Apply the actual pipeline layout to UBOs, SSBOs, and textures */
   NIR_PASS_V(nir, anv_nir_apply_pipeline_layout,
                  NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ubo,
         NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ssbo,
            /* First run copy-prop to get rid of all of the vec() that address
   * calculations often create and then constant-fold so that, when we
   * get to anv_nir_lower_ubo_loads, we can detect constant offsets.
   */
   NIR_PASS(_, nir, nir_copy_prop);
                     enum nir_lower_non_uniform_access_type lower_non_uniform_access_types =
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
                  NIR_PASS_V(nir, anv_nir_compute_push_layout,
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
                  if (gl_shader_stage_is_compute(nir->info.stage))
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
         tcs_stage->key.tcs.quads_workaround =
      compiler->devinfo->ver < 9 &&
   tes_stage->nir->info.tess._primitive_mode == TESS_PRIMITIVE_QUADS &&
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
      },
   .key = &gs_stage->key.gs,
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
   stage->key.wm.color_outputs_valid &=
         stage->key.wm.nr_color_regions =
            unsigned num_rt_bindings;
   struct anv_pipeline_binding rt_bindings[MAX_RTS];
   if (stage->key.wm.nr_color_regions > 0) {
      assert(stage->key.wm.nr_color_regions <= MAX_RTS);
   for (unsigned rt = 0; rt < stage->key.wm.nr_color_regions; rt++) {
      if (stage->key.wm.color_outputs_valid & BITFIELD_BIT(rt)) {
      rt_bindings[rt] = (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
         } else {
      /* Setup a null render target */
   rt_bindings[rt] = (struct anv_pipeline_binding) {
      .set = ANV_DESCRIPTOR_SET_COLOR_ATTACHMENTS,
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
                           {
      /* TODO: we could set this to 0 based on the information in nir_shader, but
   * we need this before we call spirv_to_nir.
   */
            struct brw_compile_fs_params params = {
      .base = {
      .nir = fs_stage->nir,
   .stats = fs_stage->stats,
   .log_data = device,
      },
   .key = &fs_stage->key.wm,
                        fs_stage->key.wm.input_slots_valid =
                     fs_stage->num_stats = (uint32_t)fs_stage->prog_data.wm.dispatch_8 +
            }
      static void
   anv_pipeline_add_executable(struct anv_pipeline *pipeline,
                     {
      char *nir = NULL;
   if (stage->nir &&
      (pipeline->flags &
   VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
               char *disasm = NULL;
   if (stage->code &&
      (pipeline->flags &
   VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR)) {
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
                                    case ANV_DESCRIPTOR_SET_SHADER_CONSTANTS:
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
      static enum brw_robustness_flags
   anv_device_get_robust_flags(const struct anv_device *device)
   {
      return device->robust_buffer_access ?
      }
      static void
   anv_graphics_pipeline_init_keys(struct anv_graphics_pipeline *pipeline,
               {
      for (uint32_t s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (!stages[s].info)
                              const struct anv_device *device = pipeline->base.device;
   enum brw_robustness_flags robust_flags = anv_device_get_robust_flags(device);
   switch (stages[s].stage) {
   case MESA_SHADER_VERTEX:
      populate_vs_prog_key(device,
                  case MESA_SHADER_TESS_CTRL:
      populate_tcs_prog_key(device,
                        case MESA_SHADER_TESS_EVAL:
      populate_tes_prog_key(device,
                  case MESA_SHADER_GEOMETRY:
      populate_gs_prog_key(device,
                  case MESA_SHADER_FRAGMENT: {
      populate_wm_prog_key(pipeline,
                        }
   default:
                  stages[s].feedback.duration += os_time_get_nano() - stage_start;
                  }
      static bool
   anv_graphics_pipeline_load_cached_shaders(struct anv_graphics_pipeline *pipeline,
                     {
      unsigned found = 0;
   unsigned cache_hits = 0;
   for (unsigned s = 0; s < ANV_GRAPHICS_SHADER_STAGE_COUNT; s++) {
      if (!stages[s].info)
                     bool cache_hit;
   struct anv_shader_bin *bin =
      anv_device_search_for_kernel(pipeline->base.device, cache,
            if (bin) {
      found++;
               if (cache_hit) {
      cache_hits++;
   stages[s].feedback.flags |=
      }
               if (found == __builtin_popcount(pipeline->active_stages)) {
      if (cache_hits == found) {
      pipeline_feedback->flags |=
      }
   /* We found all our shaders in the cache.  We're done. */
   for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
                     anv_pipeline_add_executables(&pipeline->base, &stages[s],
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
      anv_shader_bin_unref(pipeline->base.device, pipeline->shaders[s]);
                        }
      static const gl_shader_stage graphics_shader_order[] = {
      MESA_SHADER_VERTEX,
   MESA_SHADER_TESS_CTRL,
   MESA_SHADER_TESS_EVAL,
               };
      static VkResult
   anv_graphics_pipeline_load_nir(struct anv_graphics_pipeline *pipeline,
                     {
      for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
      gl_shader_stage s = graphics_shader_order[i];
   if (!stages[s].info)
                     assert(stages[s].stage == s);
            stages[s].bind_map = (struct anv_pipeline_bind_map) {
      .surface_to_descriptor = stages[s].surface_to_descriptor,
               stages[s].nir = anv_pipeline_stage_get_nir(&pipeline->base, cache,
               if (stages[s].nir == NULL) {
                                 }
      static VkResult
   anv_graphics_pipeline_compile(struct anv_graphics_pipeline *pipeline,
                     {
      ANV_FROM_HANDLE(anv_pipeline_layout, layout, info->layout);
            VkPipelineCreationFeedbackEXT pipeline_feedback = {
         };
            const struct brw_compiler *compiler = pipeline->base.device->physical->compiler;
   struct anv_pipeline_stage stages[ANV_GRAPHICS_SHADER_STAGE_COUNT] = {};
   for (uint32_t i = 0; i < info->stageCount; i++) {
      gl_shader_stage stage = vk_to_mesa_shader_stage(info->pStages[i].stage);
   stages[stage].stage = stage;
                        unsigned char sha1[20];
            for (unsigned s = 0; s < ARRAY_SIZE(stages); s++) {
      if (!stages[s].info)
            stages[s].cache_key.stage = s;
               const bool skip_cache_lookup =
         if (!skip_cache_lookup) {
      bool found_all_shaders =
      anv_graphics_pipeline_load_cached_shaders(pipeline, cache, stages,
      if (found_all_shaders)
               if (info->flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT)
                     result = anv_graphics_pipeline_load_nir(pipeline, cache, stages,
         if (result != VK_SUCCESS)
            /* Walk backwards to link */
   struct anv_pipeline_stage *next_stage = NULL;
   for (int i = ARRAY_SIZE(graphics_shader_order) - 1; i >= 0; i--) {
      gl_shader_stage s = graphics_shader_order[i];
   if (!stages[s].info)
            switch (s) {
   case MESA_SHADER_VERTEX:
      anv_pipeline_link_vs(compiler, &stages[s], next_stage);
      case MESA_SHADER_TESS_CTRL:
      anv_pipeline_link_tcs(compiler, &stages[s], next_stage);
      case MESA_SHADER_TESS_EVAL:
      anv_pipeline_link_tes(compiler, &stages[s], next_stage);
      case MESA_SHADER_GEOMETRY:
      anv_pipeline_link_gs(compiler, &stages[s], next_stage);
      case MESA_SHADER_FRAGMENT:
      anv_pipeline_link_fs(compiler, &stages[s], state->rp);
      default:
                              struct anv_pipeline_stage *prev_stage = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
      gl_shader_stage s = graphics_shader_order[i];
   if (!stages[s].info)
                                       if (prev_stage && compiler->nir_options[s]->unify_interfaces) {
      prev_stage->nir->info.outputs_written |= stages[s].nir->info.inputs_read &
         stages[s].nir->info.inputs_read |= prev_stage->nir->info.outputs_written &
         prev_stage->nir->info.patch_outputs_written |= stages[s].nir->info.patch_inputs_read;
                                             prev_stage = NULL;
   for (unsigned i = 0; i < ARRAY_SIZE(graphics_shader_order); i++) {
      gl_shader_stage s = graphics_shader_order[i];
   if (!stages[s].info)
                              switch (s) {
   case MESA_SHADER_VERTEX:
      anv_pipeline_compile_vs(compiler, stage_ctx, pipeline,
            case MESA_SHADER_TESS_CTRL:
      anv_pipeline_compile_tcs(compiler, stage_ctx, pipeline->base.device,
            case MESA_SHADER_TESS_EVAL:
      anv_pipeline_compile_tes(compiler, stage_ctx, pipeline->base.device,
            case MESA_SHADER_GEOMETRY:
      anv_pipeline_compile_gs(compiler, stage_ctx, pipeline->base.device,
            case MESA_SHADER_FRAGMENT:
      anv_pipeline_compile_fs(compiler, stage_ctx, pipeline->base.device,
            default:
         }
   if (stages[s].code == NULL) {
      ralloc_free(stage_ctx);
   result = vk_error(pipeline->base.device, VK_ERROR_OUT_OF_HOST_MEMORY);
               anv_nir_validate_push_layout(&stages[s].prog_data.base,
            struct anv_shader_bin *bin =
      anv_device_upload_kernel(pipeline->base.device, cache, s,
                           &stages[s].cache_key,
   sizeof(stages[s].cache_key),
   stages[s].code,
   stages[s].prog_data.base.program_size,
      if (!bin) {
      ralloc_free(stage_ctx);
   result = vk_error(pipeline, VK_ERROR_OUT_OF_HOST_MEMORY);
                        pipeline->shaders[s] = bin;
                                       done:
                  const VkPipelineCreationFeedbackCreateInfo *create_feedback =
         if (create_feedback) {
               uint32_t stage_count = create_feedback->pipelineStageCreationFeedbackCount;
   assert(stage_count == 0 || info->stageCount == stage_count);
   for (uint32_t i = 0; i < stage_count; i++) {
      gl_shader_stage s = vk_to_mesa_shader_stage(info->pStages[i].stage);
                        fail:
               for (unsigned s = 0; s < ARRAY_SIZE(pipeline->shaders); s++) {
      if (pipeline->shaders[s])
                  }
      static VkResult
   anv_pipeline_compile_cs(struct anv_compute_pipeline *pipeline,
               {
      const VkPipelineShaderStageCreateInfo *sinfo = &info->stage;
            VkPipelineCreationFeedback pipeline_feedback = {
         };
            struct anv_device *device = pipeline->base.device;
            struct anv_pipeline_stage stage = {
      .stage = MESA_SHADER_COMPUTE,
   .info = &info->stage,
   .cache_key = {
         },
   .feedback = {
            };
                     populate_cs_prog_key(device,
                           const bool skip_cache_lookup =
                     bool cache_hit = false;
   if (!skip_cache_lookup) {
      bin = anv_device_search_for_kernel(device, cache,
                           if (bin == NULL &&
      (info->flags & VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT))
         void *mem_ctx = ralloc_context(NULL);
   if (bin == NULL) {
               stage.bind_map = (struct anv_pipeline_bind_map) {
      .surface_to_descriptor = stage.surface_to_descriptor,
               /* Set up a binding for the gl_NumWorkGroups */
   stage.bind_map.surface_count = 1;
   stage.bind_map.surface_to_descriptor[0] = (struct anv_pipeline_binding) {
                  stage.nir = anv_pipeline_stage_get_nir(&pipeline->base, cache, mem_ctx, &stage);
   if (stage.nir == NULL) {
      ralloc_free(mem_ctx);
                        unsigned local_size = stage.nir->info.workgroup_size[0] *
                  /* Games don't always request full subgroups when they should,
   * which can cause bugs, as they may expect bigger size of the
   * subgroup than we choose for the execution.
   */
   if (device->physical->instance->assume_full_subgroups &&
      stage.nir->info.uses_wide_subgroup_intrinsics &&
   stage.nir->info.subgroup_size == SUBGROUP_SIZE_API_CONSTANT &&
   local_size &&
               /* If the client requests that we dispatch full subgroups but doesn't
   * allow us to pick a subgroup size, we have to smash it to the API
   * value of 32.  Performance will likely be terrible in this case but
   * there's nothing we can do about that.  The client should have chosen
   * a size.
   */
   if (stage.nir->info.subgroup_size == SUBGROUP_SIZE_FULL_SUBGROUPS)
      stage.nir->info.subgroup_size =
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
   bin = anv_device_upload_kernel(device, cache,
                                 MESA_SHADER_COMPUTE,
   &stage.cache_key, sizeof(stage.cache_key),
   if (!bin) {
      ralloc_free(mem_ctx);
                                             if (cache_hit) {
      stage.feedback.flags |=
         pipeline_feedback.flags |=
      }
            const VkPipelineCreationFeedbackCreateInfo *create_feedback =
         if (create_feedback) {
               if (create_feedback->pipelineStageCreationFeedbackCount) {
      assert(create_feedback->pipelineStageCreationFeedbackCount == 1);
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
               anv_batch_set_storage(&pipeline->base.batch, ANV_NULL_ADDRESS,
            result = anv_pipeline_compile_cs(pipeline, cache, pCreateInfo);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device, pAllocator);
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
      VkResult res = anv_compute_pipeline_create(device, pipeline_cache,
                  if (res == VK_SUCCESS)
            /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED as it
   * is not obvious what error should be report upon 2 different failures.
   * */
   result = res;
   if (res != VK_PIPELINE_COMPILE_REQUIRED)
                     if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
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
      static VkResult
   anv_graphics_pipeline_init(struct anv_graphics_pipeline *pipeline,
                                 {
               result = anv_pipeline_init(&pipeline->base, device,
               if (result != VK_SUCCESS)
            anv_batch_set_storage(&pipeline->base.batch, ANV_NULL_ADDRESS,
            pipeline->active_stages = 0;
   for (uint32_t i = 0; i < pCreateInfo->stageCount; i++)
            if (pipeline->active_stages & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)
            pipeline->dynamic_state.ms.sample_locations = &pipeline->sample_locations;
            pipeline->depth_clamp_enable = state->rs->depth_clamp_enable;
   pipeline->depth_clip_enable =
                  result = anv_graphics_pipeline_compile(pipeline, cache, pCreateInfo, state);
   if (result != VK_SUCCESS) {
      anv_pipeline_finish(&pipeline->base, device, alloc);
                                 u_foreach_bit(a, state->vi->attributes_valid) {
      if (inputs_read & BITFIELD64_BIT(VERT_ATTRIB_GENERIC0 + a))
               u_foreach_bit(b, state->vi->bindings_valid) {
      pipeline->vb[b].stride = state->vi->bindings[b].stride;
   pipeline->vb[b].instanced = state->vi->bindings[b].input_rate ==
                     pipeline->instance_multiplier = 1;
   if (pipeline->view_mask)
            pipeline->negative_one_to_one =
            /* Store line mode, polygon mode and rasterization samples, these are used
   * for dynamic primitive topology.
   */
   pipeline->polygon_mode = state->rs->polygon_mode;
   pipeline->rasterization_samples =
         pipeline->line_mode = state->rs->line.mode;
   if (pipeline->line_mode == VK_LINE_RASTERIZATION_MODE_DEFAULT_EXT) {
      if (pipeline->rasterization_samples > 1) {
         } else {
            }
   pipeline->patch_control_points =
            /* Store the color write masks, to be merged with color write enable if
   * dynamic.
   */
   if (state->cb != NULL) {
      for (unsigned i = 0; i < state->cb->attachment_count; i++)
                  }
      static VkResult
   anv_graphics_pipeline_create(struct anv_device *device,
                           {
      struct anv_graphics_pipeline *pipeline;
                     pipeline = vk_zalloc2(&device->vk.alloc, pAllocator, sizeof(*pipeline), 8,
         if (pipeline == NULL)
            struct vk_graphics_pipeline_all_state all;
   struct vk_graphics_pipeline_state state = { };
   result = vk_graphics_pipeline_state_fill(&device->vk, &state, pCreateInfo,
               if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
               result = anv_graphics_pipeline_init(pipeline, device, cache,
         if (result != VK_SUCCESS) {
      vk_free2(&device->vk.alloc, pAllocator, pipeline);
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
      VkResult res = anv_graphics_pipeline_create(device,
                        if (res == VK_SUCCESS)
            /* Bail out on the first error != VK_PIPELINE_COMPILE_REQUIRED as it
   * is not obvious what error should be report upon 2 different failures.
   * */
   result = res;
   if (res != VK_PIPELINE_COMPILE_REQUIRED)
                     if (pCreateInfos[i].flags & VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT)
               for (; i < count; i++)
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
   case ANV_PIPELINE_GRAPHICS: {
      prog_data = anv_pipeline_to_graphics(pipeline)->shaders[exe->stage]->prog_data;
      }
   case ANV_PIPELINE_COMPUTE: {
      prog_data = anv_pipeline_to_compute(pipeline)->cs->prog_data;
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
               if (gl_shader_stage_uses_workgroup(exe->stage)) {
      vk_outarray_append_typed(VkPipelineExecutableStatisticKHR, &out, stat) {
      WRITE_STR(stat->name, "Workgroup Memory Size");
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
