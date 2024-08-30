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
      #include "radv_shader.h"
   #include "meta/radv_meta.h"
   #include "nir/nir.h"
   #include "nir/nir_builder.h"
   #include "nir/nir_xfb_info.h"
   #include "nir/radv_nir.h"
   #include "spirv/nir_spirv.h"
   #include "util/memstream.h"
   #include "util/mesa-sha1.h"
   #include "util/streaming-load-memcpy.h"
   #include "util/u_atomic.h"
   #include "radv_cs.h"
   #include "radv_debug.h"
   #include "radv_private.h"
   #include "radv_shader_args.h"
      #include "util/u_debug.h"
   #include "ac_binary.h"
   #include "ac_nir.h"
   #if defined(USE_LIBELF)
   #include "ac_rtld.h"
   #endif
   #include "aco_interface.h"
   #include "sid.h"
   #include "vk_format.h"
   #include "vk_nir.h"
   #include "vk_semaphore.h"
   #include "vk_sync.h"
      #include "aco_shader_info.h"
   #include "radv_aco_shader_info.h"
   #ifdef LLVM_AVAILABLE
   #include "ac_llvm_util.h"
   #endif
      static void
   get_nir_options_for_stage(struct radv_physical_device *device, gl_shader_stage stage)
   {
      bool split_fma = (stage <= MESA_SHADER_GEOMETRY || stage == MESA_SHADER_MESH) &&
         device->nir_options[stage] = (nir_shader_compiler_options){
      .vertex_id_zero_based = true,
   .lower_scmp = true,
   .lower_flrp16 = true,
   .lower_flrp32 = true,
   .lower_flrp64 = true,
   .lower_device_index_to_zero = true,
   .lower_fdiv = true,
   .lower_fmod = true,
   .lower_ineg = true,
   .lower_bitfield_insert = true,
   .lower_bitfield_extract = true,
   .lower_pack_snorm_4x8 = true,
   .lower_pack_unorm_4x8 = true,
   .lower_pack_half_2x16 = true,
   .lower_pack_64_2x32 = true,
   .lower_pack_64_4x16 = true,
   .lower_pack_32_2x16 = true,
   .lower_unpack_snorm_2x16 = true,
   .lower_unpack_snorm_4x8 = true,
   .lower_unpack_unorm_2x16 = true,
   .lower_unpack_unorm_4x8 = true,
   .lower_unpack_half_2x16 = true,
   .lower_ffma16 = split_fma || device->rad_info.gfx_level < GFX9,
   .lower_ffma32 = split_fma || device->rad_info.gfx_level < GFX10_3,
   .lower_ffma64 = split_fma,
   .lower_fpow = true,
   .lower_mul_2x32_64 = true,
   .lower_iadd_sat = device->rad_info.gfx_level <= GFX8,
   .lower_hadd = true,
   .lower_mul_32x16 = true,
   .has_bfe = true,
   .has_bfm = true,
   .has_bitfield_select = true,
   .has_fsub = true,
   .has_isub = true,
   .has_sdot_4x8 = device->rad_info.has_accelerated_dot_product,
   .has_sudot_4x8 = device->rad_info.has_accelerated_dot_product && device->rad_info.gfx_level >= GFX11,
   .has_udot_4x8 = device->rad_info.has_accelerated_dot_product,
   .has_dot_2x16 = device->rad_info.has_accelerated_dot_product && device->rad_info.gfx_level < GFX11,
   .has_find_msb_rev = true,
   .has_pack_half_2x16_rtz = true,
   .has_bit_test = !device->use_llvm,
   .has_fmulz = true,
   .max_unroll_iterations = 32,
   .max_unroll_iterations_aggressive = 128,
   .use_interpolated_input_intrinsics = true,
   .vectorize_vec2_16bit = true,
   .lower_int64_options = nir_lower_imul64 | nir_lower_imul_high64 | nir_lower_imul_2x32_64 | nir_lower_divmod64 |
         .lower_doubles_options = nir_lower_drcp | nir_lower_dsqrt | nir_lower_drsq | nir_lower_ddiv,
   .divergence_analysis_options = nir_divergence_view_index_uniform,
         }
      void
   radv_get_nir_options(struct radv_physical_device *device)
   {
      for (gl_shader_stage stage = MESA_SHADER_VERTEX; stage < MESA_VULKAN_SHADER_STAGES; stage++)
      }
      static uint8_t
   vectorize_vec2_16bit(const nir_instr *instr, const void *_)
   {
      if (instr->type != nir_instr_type_alu)
            const nir_alu_instr *alu = nir_instr_as_alu(instr);
   const unsigned bit_size = alu->def.bit_size;
   if (bit_size == 16)
         else
      }
      static bool
   is_meta_shader(nir_shader *nir)
   {
         }
      bool
   radv_can_dump_shader(struct radv_device *device, nir_shader *nir, bool meta_shader)
   {
      if (!(device->instance->debug_flags & RADV_DEBUG_DUMP_SHADERS))
            if ((is_meta_shader(nir) || meta_shader) && !(device->instance->debug_flags & RADV_DEBUG_DUMP_META_SHADERS))
               }
      bool
   radv_can_dump_shader_stats(struct radv_device *device, nir_shader *nir)
   {
      /* Only dump non-meta shader stats. */
      }
      void
   radv_optimize_nir(struct nir_shader *shader, bool optimize_conservatively)
   {
               do {
               NIR_PASS(progress, shader, nir_split_array_vars, nir_var_function_temp);
            if (!shader->info.var_copies_lowered) {
      /* Only run this pass if nir_lower_var_copies was not called
   * yet. That would lower away any copy_deref instructions and we
   * don't want to introduce any more.
   */
               NIR_PASS(progress, shader, nir_opt_copy_prop_vars);
   NIR_PASS(progress, shader, nir_opt_dead_write_vars);
            NIR_PASS(_, shader, nir_lower_alu_width, vectorize_vec2_16bit, NULL);
            NIR_PASS(progress, shader, nir_copy_prop);
   NIR_PASS(progress, shader, nir_opt_remove_phis);
   NIR_PASS(progress, shader, nir_opt_dce);
   if (nir_opt_trivial_continues(shader)) {
      progress = true;
   NIR_PASS(progress, shader, nir_copy_prop);
   NIR_PASS(progress, shader, nir_opt_remove_phis);
      }
   NIR_PASS(progress, shader, nir_opt_if, nir_opt_if_aggressive_last_continue | nir_opt_if_optimize_phi_true_false);
   NIR_PASS(progress, shader, nir_opt_dead_cf);
   NIR_PASS(progress, shader, nir_opt_cse);
   NIR_PASS(progress, shader, nir_opt_peephole_select, 8, true, true);
   NIR_PASS(progress, shader, nir_opt_constant_folding);
   NIR_PASS(progress, shader, nir_opt_intrinsics);
                     if (shader->options->max_unroll_iterations) {
                     NIR_PASS(progress, shader, nir_opt_shrink_vectors);
   NIR_PASS(progress, shader, nir_remove_dead_variables, nir_var_function_temp | nir_var_shader_in | nir_var_shader_out,
            if (shader->info.stage == MESA_SHADER_FRAGMENT && (shader->info.fs.uses_discard || shader->info.fs.uses_demote)) {
      NIR_PASS(progress, shader, nir_opt_conditional_discard);
                  }
      void
   radv_optimize_nir_algebraic(nir_shader *nir, bool opt_offsets)
   {
      bool more_algebraic = true;
   while (more_algebraic) {
      more_algebraic = false;
   NIR_PASS(_, nir, nir_copy_prop);
   NIR_PASS(_, nir, nir_opt_dce);
   NIR_PASS(_, nir, nir_opt_constant_folding);
   NIR_PASS(_, nir, nir_opt_cse);
               if (opt_offsets) {
      static const nir_opt_offsets_options offset_options = {
      .uniform_max = 0,
   .buffer_max = ~0,
      };
               /* Do late algebraic optimization to turn add(a,
   * neg(b)) back into subs, then the mandatory cleanup
   * after algebraic.  Note that it may produce fnegs,
   * and if so then we need to keep running to squash
   * fneg(fneg(a)).
   */
   bool more_late_algebraic = true;
   while (more_late_algebraic) {
      more_late_algebraic = false;
   NIR_PASS(more_late_algebraic, nir, nir_opt_algebraic_late);
   NIR_PASS(_, nir, nir_opt_constant_folding);
   NIR_PASS(_, nir, nir_copy_prop);
   NIR_PASS(_, nir, nir_opt_dce);
         }
      static void
   shared_var_info(const struct glsl_type *type, unsigned *size, unsigned *align)
   {
               uint32_t comp_size = glsl_type_is_boolean(type) ? 4 : glsl_get_bit_size(type) / 8;
   unsigned length = glsl_get_vector_elements(type);
      }
      struct radv_shader_debug_data {
      struct radv_device *device;
      };
      static void
   radv_spirv_nir_debug(void *private_data, enum nir_spirv_debug_level level, size_t spirv_offset, const char *message)
   {
      struct radv_shader_debug_data *debug_data = private_data;
            static const VkDebugReportFlagsEXT vk_flags[] = {
      [NIR_SPIRV_DEBUG_LEVEL_INFO] = VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
   [NIR_SPIRV_DEBUG_LEVEL_WARNING] = VK_DEBUG_REPORT_WARNING_BIT_EXT,
      };
                        }
      static void
   radv_compiler_debug(void *private_data, enum aco_compiler_debug_level level, const char *message)
   {
      struct radv_shader_debug_data *debug_data = private_data;
            static const VkDebugReportFlagsEXT vk_flags[] = {
      [ACO_COMPILER_DEBUG_LEVEL_PERFWARN] = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
               /* VK_DEBUG_REPORT_DEBUG_BIT_EXT specifies diagnostic information
   * from the implementation and layers.
   */
      }
      /* If the shader doesn't have an index=1 output, then assume that it meant for a location=1 to be used. This works on
   * some older hardware because the MRT1 target is used for both location=1 and index=1, but GFX11 works differently.
   */
   static void
   fix_dual_src_mrt1_export(nir_shader *nir)
   {
      nir_foreach_shader_out_variable (var, nir) {
      if (var->data.location == FRAG_RESULT_DATA0 && var->data.index == 1)
               nir_variable *loc1_var = nir_find_variable_with_location(nir, nir_var_shader_out, FRAG_RESULT_DATA1);
   if (loc1_var) {
      loc1_var->data.location = FRAG_RESULT_DATA0;
         }
      nir_shader *
   radv_shader_spirv_to_nir(struct radv_device *device, const struct radv_shader_stage *stage,
         {
      unsigned subgroup_size = 64, ballot_bit_size = 64;
   const unsigned required_subgroup_size = key->stage_info[stage->stage].subgroup_required_size * 32;
   if (required_subgroup_size) {
      /* Only compute/mesh/task shaders currently support requiring a
   * specific subgroup size.
   */
   assert(stage->stage >= MESA_SHADER_COMPUTE);
   subgroup_size = required_subgroup_size;
                        if (stage->internal_nir) {
      /* Some things such as our meta clear/blit code will give us a NIR
   * shader directly.  In that case, we just ignore the SPIR-V entirely
   * and just use the NIR shader.  We don't want to alter meta and RT
   * shaders IR directly, so clone it first. */
   nir = nir_shader_clone(NULL, stage->internal_nir);
               } else {
      uint32_t *spirv = (uint32_t *)stage->spirv.data;
            bool dump_meta = device->instance->debug_flags & RADV_DEBUG_DUMP_META_SHADERS;
   if ((device->instance->debug_flags & RADV_DEBUG_DUMP_SPIRV) && (!is_internal || dump_meta))
            uint32_t num_spec_entries = 0;
   struct nir_spirv_specialization *spec_entries = vk_spec_info_to_nir_spirv(stage->spec_info, &num_spec_entries);
   struct radv_shader_debug_data spirv_debug_data = {
      .device = device,
      };
   const bool has_fragment_shader_interlock = radv_has_pops(device->physical_device);
   const struct spirv_to_nir_options spirv_options = {
      .caps =
      {
      .amd_fragment_mask = true,
   .amd_gcn_shader = true,
   .amd_image_gather_bias_lod = true,
   .amd_image_read_write_lod = true,
   .amd_shader_ballot = true,
   .amd_shader_explicit_vertex_parameter = true,
   .amd_trinary_minmax = true,
   .demote_to_helper_invocation = true,
   .derivative_group = true,
   .descriptor_array_dynamic_indexing = true,
   .descriptor_array_non_uniform_indexing = true,
   .descriptor_indexing = true,
   .device_group = true,
   .draw_parameters = true,
   .float_controls = true,
   .float16 = device->physical_device->rad_info.has_packed_math_16bit,
   .float32_atomic_add = true,
   .float32_atomic_min_max = true,
   .float64 = true,
   .float64_atomic_min_max = true,
   .fragment_barycentric = true,
   .fragment_fully_covered = true,
   .fragment_shader_pixel_interlock = has_fragment_shader_interlock,
   .fragment_shader_sample_interlock = has_fragment_shader_interlock,
   .geometry_streams = true,
   .groups = true,
   .image_atomic_int64 = true,
   .image_ms_array = true,
   .image_read_without_format = true,
   .image_write_without_format = true,
   .int8 = true,
   .int16 = true,
   .int64 = true,
   .int64_atomics = true,
   .integer_functions2 = true,
   .mesh_shading = true,
   .min_lod = true,
   .multiview = true,
   .physical_storage_buffer_address = true,
   .post_depth_coverage = true,
   .ray_cull_mask = true,
   .ray_query = true,
   .ray_tracing = true,
   .ray_traversal_primitive_culling = true,
   .runtime_descriptor_array = true,
   .shader_clock = true,
   .shader_viewport_index_layer = true,
   .sparse_residency = true,
   .stencil_export = true,
   .storage_8bit = true,
   .storage_16bit = true,
   .storage_image_ms = true,
   .subgroup_arithmetic = true,
   .subgroup_ballot = true,
   .subgroup_basic = true,
   .subgroup_quad = true,
   .subgroup_shuffle = true,
   .subgroup_uniform_control_flow = true,
   .subgroup_vote = true,
   .tessellation = true,
   .transform_feedback = true,
   .variable_pointers = true,
   .vk_memory_model = true,
   .vk_memory_model_device_scope = true,
   .fragment_shading_rate = device->physical_device->rad_info.gfx_level >= GFX10_3,
   .workgroup_memory_explicit_layout = true,
         .ubo_addr_format = nir_address_format_vec2_index_32bit_offset,
   .ssbo_addr_format = nir_address_format_vec2_index_32bit_offset,
   .phys_ssbo_addr_format = nir_address_format_64bit_global,
   .push_const_addr_format = nir_address_format_logical,
   .shared_addr_format = nir_address_format_32bit_offset,
   .constant_addr_format = nir_address_format_64bit_global,
   .debug =
      {
      .func = radv_spirv_nir_debug,
         .force_tex_non_uniform = key->tex_non_uniform,
      };
   nir = spirv_to_nir(spirv, stage->spirv.size / 4, spec_entries, num_spec_entries, stage->stage, stage->entrypoint,
         nir->info.internal |= is_internal;
   assert(nir->info.stage == stage->stage);
                     /* TODO: This can be removed once GCM (which is more general) is used. */
            const struct nir_lower_sysvals_to_varyings_options sysvals_to_varyings = {
         };
            /* We have to lower away local constant initializers right before we
   * inline functions.  That way they get properly initialized at the top
   * of the function and not at the top of its caller.
   */
   NIR_PASS(_, nir, nir_lower_variable_initializers, nir_var_function_temp);
   NIR_PASS(_, nir, nir_lower_returns);
   bool progress = false;
   NIR_PASS(progress, nir, nir_inline_functions);
   if (progress) {
      NIR_PASS(_, nir, nir_opt_copy_prop_vars);
      }
            /* Pick off the single entrypoint that we want */
            /* Make sure we lower constant initializers on output variables so that
   * nir_remove_dead_variables below sees the corresponding stores
   */
            /* Now that we've deleted all but the main function, we can go ahead and
   * lower the rest of the constant initializers.
   */
                     /* Split member structs.  We do this before lower_io_to_temporaries so that
   * it doesn't lower system values to temporaries by accident.
   */
   NIR_PASS(_, nir, nir_split_var_copies);
            if (nir->info.stage == MESA_SHADER_FRAGMENT)
         if (nir->info.stage == MESA_SHADER_FRAGMENT)
      NIR_PASS(_, nir, nir_lower_input_attachments,
            &(nir_input_attachment_options){
            nir_remove_dead_variables_options dead_vars_opts = {
         };
   NIR_PASS(_, nir, nir_remove_dead_variables,
            if (nir->info.stage == MESA_SHADER_FRAGMENT && key->ps.epilog.mrt0_is_dual_src &&
                  /* Variables can make nir_propagate_invariant more conservative
   * than it needs to be.
   */
                                       if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL ||
                                    if (device->physical_device->rad_info.gfx_level == GFX6) {
      /* GFX6 doesn't support v_floor_f64 and the precision
   * of v_fract_f64 which is used to implement 64-bit
   * floor is less than what Vulkan requires.
   */
                                    NIR_PASS(_, nir, nir_lower_system_values);
   nir_lower_compute_system_values_options csv_options = {
      /* Mesh shaders run as NGG which can implement local_invocation_index from
   * the wave ID in merged_wave_info, but they don't have local_invocation_ids on GFX10.3.
   */
   .lower_cs_local_id_to_index = nir->info.stage == MESA_SHADER_MESH && !device->mesh_fast_launch_2,
   .lower_local_invocation_index = nir->info.stage == MESA_SHADER_COMPUTE &&
            };
            /* Vulkan uses the separate-shader linking model */
                     if (nir->info.ray_queries > 0) {
      /* Lower shared variables early to prevent the over allocation of shared memory in
   * radv_nir_lower_ray_queries.  */
   if (nir->info.stage == MESA_SHADER_COMPUTE) {
                                 NIR_PASS(_, nir, nir_opt_ray_queries);
   NIR_PASS(_, nir, nir_opt_ray_query_ranges);
               nir_lower_tex_options tex_options = {
      .lower_txp = ~0,
   .lower_txf_offset = true,
   .lower_tg4_offsets = true,
   .lower_txs_cube_array = true,
   .lower_to_fragment_fetch_amd = device->physical_device->use_fmask,
   .lower_lod_zero_width = true,
                        static const nir_lower_image_options image_options = {
                                    if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_GEOMETRY ||
      nir->info.stage == MESA_SHADER_FRAGMENT) {
      } else if (nir->info.stage == MESA_SHADER_TESS_EVAL) {
                           NIR_PASS(_, nir, nir_lower_global_vars_to_local);
            bool gfx7minus = device->physical_device->rad_info.gfx_level <= GFX7;
      #if LLVM_AVAILABLE
         #endif
         NIR_PASS(_, nir, nir_lower_subgroups,
            &(struct nir_lower_subgroups_options){
      .subgroup_size = subgroup_size,
   .ballot_bit_size = ballot_bit_size,
   .ballot_components = 1,
   .lower_to_scalar = 1,
   .lower_subgroup_masks = 1,
   .lower_relative_shuffle = 1,
   .lower_shuffle_to_32bit = 1,
   .lower_vote_eq = 1,
   .lower_quad_broadcast_dynamic = 1,
   .lower_quad_broadcast_dynamic_to_const = gfx7minus,
   .lower_shuffle_to_swizzle_amd = 1,
            NIR_PASS(_, nir, nir_lower_load_const_to_scalar);
            if (!key->optimisations_disabled)
            /* We call nir_lower_var_copies() after the first radv_optimize_nir()
   * to remove any copies introduced by nir_opt_find_array_copies().
   */
            unsigned lower_flrp = (nir->options->lower_flrp16 ? 16 : 0) | (nir->options->lower_flrp32 ? 32 : 0) |
         if (lower_flrp != 0) {
      bool progress = false;
   NIR_PASS(progress, nir, nir_lower_flrp, lower_flrp, false /* always precise */);
   if (progress)
               const nir_opt_access_options opt_access_options = {
         };
                     NIR_PASS(_, nir, nir_lower_explicit_io, nir_var_mem_ubo | nir_var_mem_ssbo,
                     /* Lower deref operations for compute shared memory. */
   if (nir->info.stage == MESA_SHADER_COMPUTE || nir->info.stage == MESA_SHADER_TASK ||
      nir->info.stage == MESA_SHADER_MESH) {
            if (nir->info.stage == MESA_SHADER_TASK || nir->info.stage == MESA_SHADER_MESH)
            if (!nir->info.shared_memory_explicit_layout)
         else if (var_modes & ~nir_var_mem_shared)
                  if (nir->info.zero_initialize_shared_memory && nir->info.shared_size > 0) {
      const unsigned chunk_size = 16; /* max single store size */
   const unsigned shared_size = ALIGN(nir->info.shared_size, chunk_size);
                           /* Lower large variables that are always constant with load_constant
   * intrinsics, which get turned into PC-relative loads from a data
   * section next to the shader.
   */
            /* Lower primitive shading rate to match HW requirements. */
   if ((nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_GEOMETRY ||
      nir->info.stage == MESA_SHADER_MESH) &&
   nir->info.outputs_written & BITFIELD64_BIT(VARYING_SLOT_PRIMITIVE_SHADING_RATE)) {
   /* Lower primitive shading rate to match HW requirements. */
               /* Indirect lowering must be called after the radv_optimize_nir() loop
   * has been called at least once. Otherwise indirect lowering can
   * bloat the instruction count of the loop and cause it to be
   * considered too large for unrolling.
   */
   if (ac_nir_lower_indirect_derefs(nir, device->physical_device->rad_info.gfx_level) && !key->optimisations_disabled &&
      nir->info.stage != MESA_SHADER_COMPUTE) {
   /* Optimize the lowered code before the linking optimizations. */
                  }
      bool
   radv_consider_culling(const struct radv_physical_device *pdevice, struct nir_shader *nir, uint64_t ps_inputs_read,
         {
      /* Culling doesn't make sense for meta shaders. */
   if (is_meta_shader(nir))
            /* We don't support culling with multiple viewports yet. */
   if (nir->info.outputs_written & (VARYING_BIT_VIEWPORT | VARYING_BIT_VIEWPORT_MASK))
            /* We don't support culling with vertex shader prologs. */
   if (info->vs.has_prolog)
            if (!pdevice->use_ngg_culling)
            /* Shader based culling efficiency can depend on PS throughput.
   * Estimate an upper limit for PS input param count based on GPU info.
   */
   unsigned max_ps_params;
   unsigned max_render_backends = pdevice->rad_info.max_render_backends;
            if (max_render_backends / max_se == 4)
         else
            /* TODO: consider other heuristics here, such as PS execution time */
   if (util_bitcount64(ps_inputs_read & ~VARYING_BIT_POS) > max_ps_params)
            /* Only triangle culling is supported. */
   if (num_vertices_per_primitive != 3)
            /* When the shader writes memory, it is difficult to guarantee correctness.
   * Future work:
   * - if only write-only SSBOs are used
   * - if we can prove that non-position outputs don't rely on memory stores
   * then may be okay to keep the memory stores in the 1st shader part, and delete them from the 2nd.
   */
   if (nir->info.writes_memory)
            /* When the shader relies on the subgroup invocation ID, we'd break it, because the ID changes after the culling.
   * Future work: try to save this to LDS and reload, but it can still be broken in subtle ways.
   */
   if (BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SUBGROUP_INVOCATION))
               }
      static void
   setup_ngg_lds_layout(struct radv_device *device, nir_shader *nir, struct radv_shader_info *info, unsigned max_vtx_in)
   {
      unsigned scratch_lds_base = 0;
            if (stage == MESA_SHADER_VERTEX || stage == MESA_SHADER_TESS_EVAL) {
      /* Get pervertex LDS usage. */
   bool uses_instanceid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INSTANCE_ID);
   bool uses_primitive_id = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_PRIMITIVE_ID);
   bool streamout_enabled = nir->xfb_info && device->physical_device->use_ngg_streamout;
   unsigned pervertex_lds_bytes = ac_ngg_nogs_get_pervertex_lds_size(
                  unsigned total_es_lds_bytes = pervertex_lds_bytes * max_vtx_in;
      } else if (stage == MESA_SHADER_GEOMETRY) {
      unsigned esgs_ring_lds_bytes = info->ngg_info.esgs_ring_size;
   unsigned gs_total_out_vtx_bytes = info->ngg_info.ngg_emit_size * 4u;
      } else {
      /* not handled here */
               /* Get scratch LDS usage. */
   unsigned scratch_lds_size = ac_ngg_get_scratch_lds_size(
            /* Get total LDS usage. */
            /* Record scratch base for abi lower of nir_load_lds_ngg_scratch_base_amd. */
      }
      void
   radv_lower_ngg(struct radv_device *device, struct radv_shader_stage *ngg_stage, const struct radv_pipeline_key *pl_key)
   {
      const struct radv_shader_info *info = &ngg_stage->info;
            assert(nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL ||
            const struct gfx10_ngg_info *ngg_info = &info->ngg_info;
            /* Get the number of vertices per input primitive */
   if (nir->info.stage == MESA_SHADER_TESS_EVAL) {
      if (nir->info.tess.point_mode)
         else if (nir->info.tess._primitive_mode == TESS_PRIMITIVE_ISOLINES)
            /* Manually mark the primitive ID used, so the shader can repack it. */
   if (info->outinfo.export_prim_id)
         } else if (nir->info.stage == MESA_SHADER_VERTEX) {
               /* Manually mark the instance ID used, so the shader can repack it. */
   if (pl_key->vs.instance_rate_inputs)
         } else if (nir->info.stage == MESA_SHADER_GEOMETRY) {
         } else if (nir->info.stage == MESA_SHADER_MESH) {
      if (nir->info.mesh.primitive_type == MESA_PRIM_POINTS)
         else if (nir->info.mesh.primitive_type == MESA_PRIM_LINES)
         else
      } else {
                  /* Invocations that process an input vertex */
                     ac_nir_lower_ngg_options options = {0};
   options.family = device->physical_device->rad_info.family;
   options.gfx_level = device->physical_device->rad_info.gfx_level;
   options.max_workgroup_size = info->workgroup_size;
   options.wave_size = info->wave_size;
   options.clipdist_enable_mask = info->outinfo.clip_dist_mask | info->outinfo.cull_dist_mask;
   options.vs_output_param_offset = info->outinfo.vs_output_param_offset;
   options.has_param_exports = info->outinfo.param_exports || info->outinfo.prim_param_exports;
   options.can_cull = nir->info.stage != MESA_SHADER_GEOMETRY && info->has_ngg_culling;
   options.disable_streamout = !device->physical_device->use_ngg_streamout;
   options.has_gen_prim_query = info->has_prim_query;
   options.has_xfb_prim_query = info->has_xfb_query;
            if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL) {
               if (info->has_ngg_culling)
            options.num_vertices_per_primitive = num_vertices_per_prim;
   options.early_prim_export = info->has_ngg_early_prim_export;
   options.passthrough = info->is_ngg_passthrough;
   options.export_primitive_id = info->outinfo.export_prim_id;
                     /* Increase ESGS ring size so the LLVM binary contains the correct LDS size. */
      } else if (nir->info.stage == MESA_SHADER_GEOMETRY) {
                           } else if (nir->info.stage == MESA_SHADER_MESH) {
      /* ACO aligns the workgroup size to the wave size. */
            bool scratch_ring = false;
   NIR_PASS_V(nir, ac_nir_lower_ngg_ms, options.gfx_level, options.clipdist_enable_mask,
                  } else {
            }
      static unsigned
   get_size_class(unsigned size, bool round_up)
   {
      size = round_up ? util_logbase2_ceil(size) : util_logbase2(size);
   unsigned size_class = MAX2(size, RADV_SHADER_ALLOC_MIN_SIZE_CLASS) - RADV_SHADER_ALLOC_MIN_SIZE_CLASS;
      }
      static void
   remove_hole(struct radv_shader_free_list *free_list, union radv_shader_arena_block *hole)
   {
      unsigned size_class = get_size_class(hole->size, false);
   list_del(&hole->freelist);
   if (list_is_empty(&free_list->free_lists[size_class]))
      }
      static void
   add_hole(struct radv_shader_free_list *free_list, union radv_shader_arena_block *hole)
   {
      unsigned size_class = get_size_class(hole->size, false);
   list_addtail(&hole->freelist, &free_list->free_lists[size_class]);
      }
      static union radv_shader_arena_block *
   alloc_block_obj(struct radv_device *device)
   {
      if (!list_is_empty(&device->shader_block_obj_pool)) {
      union radv_shader_arena_block *block =
         list_del(&block->pool);
                  }
      static void
   free_block_obj(struct radv_device *device, union radv_shader_arena_block *block)
   {
         }
      VkResult
   radv_shader_wait_for_upload(struct radv_device *device, uint64_t seq)
   {
      if (!seq)
            const VkSemaphoreWaitInfo wait_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
   .pSemaphores = &device->shader_upload_sem,
   .semaphoreCount = 1,
      };
      }
      static struct radv_shader_arena *
   radv_create_shader_arena(struct radv_device *device, struct radv_shader_free_list *free_list, unsigned min_size,
         {
      union radv_shader_arena_block *alloc = NULL;
   struct radv_shader_arena *arena = calloc(1, sizeof(struct radv_shader_arena));
   if (!arena)
            if (!arena_size)
      arena_size = MAX2(
      RADV_SHADER_ALLOC_MIN_ARENA_SIZE << MIN2(RADV_SHADER_ALLOC_MAX_ARENA_SIZE_SHIFT, device->shader_arena_shift),
            enum radeon_bo_flag flags = RADEON_FLAG_NO_INTERPROCESS_SHARING | RADEON_FLAG_32BIT;
   if (device->shader_use_invisible_vram)
         else
            if (replayable)
            VkResult result;
   result = device->ws->buffer_create(device->ws, arena_size, RADV_SHADER_ALLOC_ALIGNMENT, RADEON_DOMAIN_VRAM, flags,
         if (result != VK_SUCCESS)
                     list_inithead(&arena->entries);
   alloc = alloc_block_obj(device);
   if (!alloc)
            list_inithead(&alloc->freelist);
   alloc->arena = arena;
   alloc->offset = 0;
   alloc->size = arena_size;
   list_addtail(&alloc->list, &arena->entries);
   if (free_list)
            if (!(flags & RADEON_FLAG_NO_CPU_ACCESS)) {
      arena->ptr = (char *)device->ws->buffer_map(arena->bo);
   if (!arena->ptr)
               if (replay_va)
         else if (replayable)
         else
                  fail:
      if (alloc)
         if (arena && arena->bo) {
      radv_rmv_log_bo_destroy(device, arena->bo);
      }
   free(arena);
      }
      /* Inserts a block at an arbitrary place into a hole, splitting the hole as needed */
   static union radv_shader_arena_block *
   insert_block(struct radv_device *device, union radv_shader_arena_block *hole, uint32_t offset_in_hole, uint32_t size,
         {
      uint32_t hole_begin = hole->offset;
            /* The block might not lie exactly at the beginning or end
   * of the hole. Resize the hole to fit the block exactly,
   * and insert new holes before (left_hole) or after (right_hole) as needed.
   * left_hole or right_hole are skipped if the allocation lies exactly at the
   * beginning or end of the hole to avoid 0-sized holes. */
   union radv_shader_arena_block *left_hole = NULL;
            if (offset_in_hole) {
      left_hole = alloc_block_obj(device);
   if (!left_hole)
         list_inithead(&left_hole->freelist);
   left_hole->arena = hole->arena;
   left_hole->offset = hole->offset;
            if (free_list)
               if (hole->size > offset_in_hole + size) {
      right_hole = alloc_block_obj(device);
   if (!right_hole) {
      free(left_hole);
      }
   list_inithead(&right_hole->freelist);
   right_hole->arena = hole->arena;
   right_hole->offset = hole_begin + offset_in_hole + size;
            if (free_list)
               if (left_hole) {
      hole->offset += left_hole->size;
               }
   if (right_hole) {
                           if (free_list)
            }
      /* Segregated fit allocator, implementing a good-fit allocation policy.
   *
   * This is an variation of sequential fit allocation with several lists of free blocks ("holes")
   * instead of one. Each list of holes only contains holes of a certain range of sizes, so holes that
   * are too small can easily be ignored while allocating. Because this also ignores holes that are
   * larger than necessary (approximating best-fit allocation), this could be described as a
   * "good-fit" allocator.
   *
   * Typically, shaders are allocated and only free'd when the device is destroyed. For this pattern,
   * this should allocate blocks for shaders fast and with no fragmentation, while still allowing
   * free'd memory to be re-used.
   */
   union radv_shader_arena_block *
   radv_alloc_shader_memory(struct radv_device *device, uint32_t size, bool replayable, void *ptr)
   {
      size = ac_align_shader_binary_for_prefetch(&device->physical_device->rad_info, size);
                              /* Try to use an existing hole. Unless the shader is very large, this should only have to look
   * at the first one available.
   */
   unsigned free_list_mask = BITFIELD_MASK(RADV_SHADER_ALLOC_NUM_FREE_LISTS);
   unsigned size_class = ffs(free_list->size_mask & (free_list_mask << get_size_class(size, true)));
   if (size_class) {
               list_for_each_entry (union radv_shader_arena_block, hole, &free_list->free_lists[size_class], freelist) {
                              if (size == hole->size) {
      remove_hole(free_list, hole);
   hole->freelist.next = ptr;
   mtx_unlock(&device->shader_arena_mutex);
      } else {
      union radv_shader_arena_block *alloc = alloc_block_obj(device);
   if (!alloc) {
      mtx_unlock(&device->shader_arena_mutex);
      }
   list_addtail(&alloc->list, &hole->list);
   alloc->freelist.prev = NULL;
   alloc->freelist.next = ptr;
   alloc->arena = hole->arena;
                  remove_hole(free_list, hole);
   hole->offset += size;
                  mtx_unlock(&device->shader_arena_mutex);
                     struct radv_shader_arena *arena = radv_create_shader_arena(device, free_list, size, 0, replayable, 0);
   union radv_shader_arena_block *alloc = NULL;
   if (!arena)
            alloc =
         alloc->freelist.prev = NULL;
            ++device->shader_arena_shift;
            mtx_unlock(&device->shader_arena_mutex);
         fail:
      mtx_unlock(&device->shader_arena_mutex);
   free(alloc);
   if (arena) {
      free(arena->list.next);
   radv_rmv_log_bo_destroy(device, arena->bo);
      }
   free(arena);
      }
      static union radv_shader_arena_block *
   get_hole(struct radv_shader_arena *arena, struct list_head *head)
   {
      if (head == &arena->entries)
            union radv_shader_arena_block *hole = list_entry(head, union radv_shader_arena_block, list);
      }
      void
   radv_free_shader_memory(struct radv_device *device, union radv_shader_arena_block *alloc)
   {
               union radv_shader_arena_block *hole_prev = get_hole(alloc->arena, alloc->list.prev);
                              switch (alloc->arena->type) {
   case RADV_SHADER_ARENA_DEFAULT:
      free_list = &device->shader_free_list;
      case RADV_SHADER_ARENA_REPLAYABLE:
      free_list = &device->capture_replay_free_list;
      case RADV_SHADER_ARENA_REPLAYED:
      free_list = NULL;
      default:
                  /* merge with previous hole */
   if (hole_prev) {
      if (free_list)
            hole_prev->size += hole->size;
   list_del(&hole->list);
                        /* merge with next hole */
   if (hole_next) {
      if (free_list)
            hole_next->offset -= hole->size;
   hole_next->size += hole->size;
   list_del(&hole->list);
                        if (list_is_singular(&hole->list)) {
      struct radv_shader_arena *arena = hole->arena;
            radv_rmv_log_bo_destroy(device, arena->bo);
   device->ws->buffer_destroy(device->ws, arena->bo);
   list_del(&arena->list);
      } else if (free_list) {
                     }
      struct radv_serialized_shader_arena_block
   radv_serialize_shader_arena_block(union radv_shader_arena_block *block)
   {
      struct radv_serialized_shader_arena_block serialized_block = {
      .offset = block->offset,
   .size = block->size,
   .arena_va = block->arena->bo->va,
      };
      }
      union radv_shader_arena_block *
   radv_replay_shader_arena_block(struct radv_device *device, const struct radv_serialized_shader_arena_block *src,
         {
      mtx_lock(&device->shader_arena_mutex);
   uint64_t va = src->arena_va;
            if (!data) {
      struct radv_shader_arena *arena = radv_create_shader_arena(device, NULL, 0, src->arena_size, true, src->arena_va);
   if (!arena) {
      mtx_unlock(&device->shader_arena_mutex);
               _mesa_hash_table_u64_insert(device->capture_replay_arena_vas, src->arena_va, arena);
   list_addtail(&arena->list, &device->shader_arenas);
      }
            uint32_t block_begin = src->offset;
            struct radv_shader_arena *arena = data;
   list_for_each_entry (union radv_shader_arena_block, hole, &arena->entries, list) {
      /* Only consider holes, not allocated shaders */
   if (!hole->freelist.prev)
            uint32_t hole_begin = hole->offset;
            if (hole_end < block_end)
            /* If another allocated block overlaps the current replay block, allocation is impossible */
   if (hole_begin > block_begin)
            union radv_shader_arena_block *block = insert_block(device, hole, block_begin - hole_begin, src->size, NULL);
   if (!block)
            block->freelist.prev = NULL;
   block->freelist.next = ptr;
      }
      }
      void
   radv_init_shader_arenas(struct radv_device *device)
   {
               device->shader_free_list.size_mask = 0;
            list_inithead(&device->shader_arenas);
   list_inithead(&device->shader_block_obj_pool);
   for (unsigned i = 0; i < RADV_SHADER_ALLOC_NUM_FREE_LISTS; i++) {
      list_inithead(&device->shader_free_list.free_lists[i]);
         }
      void
   radv_destroy_shader_arenas(struct radv_device *device)
   {
      list_for_each_entry_safe (union radv_shader_arena_block, block, &device->shader_block_obj_pool, pool)
            list_for_each_entry_safe (struct radv_shader_arena, arena, &device->shader_arenas, list) {
      radv_rmv_log_bo_destroy(device, arena->bo);
   device->ws->buffer_destroy(device->ws, arena->bo);
      }
      }
      VkResult
   radv_init_shader_upload_queue(struct radv_device *device)
   {
      if (!device->shader_use_invisible_vram)
            VkDevice vk_device = radv_device_to_handle(device);
            const struct vk_device_dispatch_table *disp = &device->vk.dispatch_table;
            result = ws->ctx_create(ws, RADEON_CTX_PRIORITY_MEDIUM, &device->shader_upload_hw_ctx);
   if (result != VK_SUCCESS)
                  mtx_init(&device->shader_dma_submission_list_mutex, mtx_plain);
   cnd_init(&device->shader_dma_submission_list_cond);
            for (unsigned i = 0; i < RADV_SHADER_UPLOAD_CS_COUNT; i++) {
      struct radv_shader_dma_submission *submission = calloc(1, sizeof(struct radv_shader_dma_submission));
   submission->cs = ws->cs_create(ws, AMD_IP_SDMA, false);
   if (!submission->cs)
                     const VkSemaphoreTypeCreateInfo sem_type = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
   .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
      };
   const VkSemaphoreCreateInfo sem_create = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      };
   result = disp->CreateSemaphore(vk_device, &sem_create, NULL, &device->shader_upload_sem);
   if (result != VK_SUCCESS)
               }
      void
   radv_destroy_shader_upload_queue(struct radv_device *device)
   {
      if (!device->shader_use_invisible_vram)
            struct vk_device_dispatch_table *disp = &device->vk.dispatch_table;
            /* Upload queue should be idle assuming that pipelines are not leaked */
   if (device->shader_upload_sem)
            list_for_each_entry_safe (struct radv_shader_dma_submission, submission, &device->shader_dma_submissions, list) {
      if (submission->cs)
         if (submission->bo)
         list_del(&submission->list);
               cnd_destroy(&device->shader_dma_submission_list_cond);
            if (device->shader_upload_hw_ctx) {
      mtx_destroy(&device->shader_upload_hw_ctx_mutex);
         }
      static bool
   radv_should_use_wgp_mode(const struct radv_device *device, gl_shader_stage stage, const struct radv_shader_info *info)
   {
      enum amd_gfx_level chip = device->physical_device->rad_info.gfx_level;
   switch (stage) {
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
         default:
            }
      #if defined(USE_LIBELF)
   static bool
   radv_open_rtld_binary(struct radv_device *device, const struct radv_shader_binary *binary,
         {
      const char *elf_data = (const char *)((struct radv_shader_binary_rtld *)binary)->data;
   size_t elf_size = ((struct radv_shader_binary_rtld *)binary)->elf_size;
   struct ac_rtld_symbol lds_symbols[3];
            if (device->physical_device->rad_info.gfx_level >= GFX9 &&
      (binary->info.stage == MESA_SHADER_GEOMETRY || binary->info.is_ngg)) {
   struct ac_rtld_symbol *sym = &lds_symbols[num_lds_symbols++];
   sym->name = "esgs_ring";
   sym->size = binary->info.ngg_info.esgs_ring_size;
               if (binary->info.is_ngg && binary->info.stage == MESA_SHADER_GEOMETRY) {
      struct ac_rtld_symbol *sym = &lds_symbols[num_lds_symbols++];
   sym->name = "ngg_emit";
   sym->size = binary->info.ngg_info.ngg_emit_size * 4;
            sym = &lds_symbols[num_lds_symbols++];
   sym->name = "ngg_scratch";
   sym->size = 8;
               struct ac_rtld_open_info open_info = {
      .info = &device->physical_device->rad_info,
   .shader_type = binary->info.stage,
   .wave_size = binary->info.wave_size,
   .num_parts = 1,
   .elf_ptrs = &elf_data,
   .elf_sizes = &elf_size,
   .num_shared_lds_symbols = num_lds_symbols,
                  }
   #endif
      static bool
   radv_postprocess_binary_config(struct radv_device *device, struct radv_shader_binary *binary,
         {
                  #if !defined(USE_LIBELF)
         #else
                  if (!radv_open_rtld_binary(device, binary, &rtld_binary)) {
                  if (!ac_rtld_read_config(&device->physical_device->rad_info, &rtld_binary, config)) {
      ac_rtld_close(&rtld_binary);
               if (rtld_binary.lds_size > 0) {
      unsigned encode_granularity = device->physical_device->rad_info.lds_encode_granularity;
      }
   if (!config->lds_size && binary->info.stage == MESA_SHADER_TESS_CTRL) {
      /* This is used for reporting LDS statistics */
               assert(!binary->info.has_ngg_culling || config->lds_size);
   #endif
               const struct radv_shader_info *info = &binary->info;
   gl_shader_stage stage = binary->info.stage;
   const struct radv_physical_device *pdevice = device->physical_device;
   bool scratch_enabled = config->scratch_bytes_per_wave > 0 || info->cs.is_rt_shader;
   bool trap_enabled = !!device->trap_handler_shader;
   unsigned vgpr_comp_cnt = 0;
            if (stage == MESA_SHADER_FRAGMENT) {
                  unsigned num_vgprs = MAX2(config->num_vgprs, num_input_vgprs);
   /* +2 for the ring offsets, +3 for scratch wave offset and VCC */
   unsigned num_sgprs = MAX2(config->num_sgprs, args->ac.num_sgprs_used + 2 + 3);
   unsigned num_shared_vgprs = config->num_shared_vgprs;
   /* shared VGPRs are introduced in Navi and are allocated in blocks of 8 (RDNA ref 3.6.5) */
   assert((pdevice->rad_info.gfx_level >= GFX10 && num_shared_vgprs % 8 == 0) ||
         unsigned num_shared_vgpr_blocks = num_shared_vgprs / 8;
            config->num_vgprs = num_vgprs;
   config->num_sgprs = num_sgprs;
            config->rsrc2 = S_00B12C_USER_SGPR(args->num_user_sgprs) | S_00B12C_SCRATCH_EN(scratch_enabled) |
            if (trap_enabled) {
      /* Configure the shader exceptions like memory violation, etc.
   * TODO: Enable (and validate) more exceptions.
   */
               if (!pdevice->use_ngg_streamout) {
      config->rsrc2 |= S_00B12C_SO_BASE0_EN(!!info->so.strides[0]) | S_00B12C_SO_BASE1_EN(!!info->so.strides[1]) |
                     config->rsrc1 = S_00B848_VGPRS((num_vgprs - 1) / (info->wave_size == 32 ? 8 : 4)) | S_00B848_DX10_CLAMP(1) |
            if (pdevice->rad_info.gfx_level >= GFX10) {
         } else {
      config->rsrc1 |= S_00B228_SGPRS((num_sgprs - 1) / 8);
                        switch (stage) {
   case MESA_SHADER_TESS_EVAL:
      if (info->is_ngg) {
      config->rsrc1 |= S_00B228_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10);
      } else if (info->tes.as_es) {
                        } else {
                     config->rsrc1 |= S_00B128_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10);
      }
   config->rsrc2 |= S_00B22C_SHARED_VGPR_CNT(num_shared_vgpr_blocks);
      case MESA_SHADER_TESS_CTRL:
      if (pdevice->rad_info.gfx_level >= GFX9) {
      /* We need at least 2 components for LS.
   * VGPR0-3: (VertexID, RelAutoindex, InstanceID / StepRate0, InstanceID).
   * StepRate0 is set to 1. so that VGPR3 doesn't have to be loaded.
   */
   if (pdevice->rad_info.gfx_level >= GFX10) {
      if (info->vs.needs_instance_id) {
         } else if (pdevice->rad_info.gfx_level <= GFX10_3) {
         }
      } else {
      vgpr_comp_cnt = info->vs.needs_instance_id ? 2 : 1;
         } else {
         }
   config->rsrc1 |= S_00B428_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10) | S_00B428_WGP_MODE(wgp_mode);
   config->rsrc2 |= S_00B42C_SHARED_VGPR_CNT(num_shared_vgpr_blocks);
      case MESA_SHADER_VERTEX:
      if (info->is_ngg) {
         } else if (info->vs.as_ls) {
      assert(pdevice->rad_info.gfx_level <= GFX8);
   /* We need at least 2 components for LS.
   * VGPR0-3: (VertexID, RelAutoindex, InstanceID / StepRate0, InstanceID).
   * StepRate0 is set to 1. so that VGPR3 doesn't have to be loaded.
   */
      } else if (info->vs.as_es) {
      assert(pdevice->rad_info.gfx_level <= GFX8);
   /* VGPR0-3: (VertexID, InstanceID / StepRate0, ...) */
      } else {
      /* VGPR0-3: (VertexID, InstanceID / StepRate0, PrimID, InstanceID)
   * If PrimID is disabled. InstanceID / StepRate1 is loaded instead.
   * StepRate0 is set to 1. so that VGPR3 doesn't have to be loaded.
   */
   if (info->vs.needs_instance_id && pdevice->rad_info.gfx_level >= GFX10) {
         } else if (info->outinfo.export_prim_id) {
         } else if (info->vs.needs_instance_id) {
         } else {
                     }
   config->rsrc2 |= S_00B12C_SHARED_VGPR_CNT(num_shared_vgpr_blocks) | S_00B12C_EXCP_EN(excp_en);
      case MESA_SHADER_MESH:
      config->rsrc1 |= S_00B228_MEM_ORDERED(1);
   config->rsrc2 |= S_00B12C_SHARED_VGPR_CNT(num_shared_vgpr_blocks) | S_00B12C_EXCP_EN(excp_en);
      case MESA_SHADER_FRAGMENT:
      config->rsrc1 |= S_00B028_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10) |
         config->rsrc2 |= S_00B02C_SHARED_VGPR_CNT(num_shared_vgpr_blocks) | S_00B02C_EXCP_EN(excp_en) |
            case MESA_SHADER_GEOMETRY:
      config->rsrc1 |= S_00B228_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10);
   config->rsrc2 |= S_00B22C_SHARED_VGPR_CNT(num_shared_vgpr_blocks) | S_00B22C_EXCP_EN(excp_en);
      case MESA_SHADER_RAYGEN:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
   case MESA_SHADER_CALLABLE:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_ANY_HIT:
      config->rsrc2 |= S_00B12C_SCRATCH_EN(1);
      case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TASK:
      config->rsrc1 |= S_00B848_MEM_ORDERED(pdevice->rad_info.gfx_level >= GFX10) | S_00B848_WGP_MODE(wgp_mode);
   config->rsrc2 |= S_00B84C_TGID_X_EN(info->cs.uses_block_id[0]) | S_00B84C_TGID_Y_EN(info->cs.uses_block_id[1]) |
                  S_00B84C_TGID_Z_EN(info->cs.uses_block_id[2]) |
   S_00B84C_TIDIG_COMP_CNT(info->cs.uses_thread_id[2]   ? 2
                        default:
      unreachable("unsupported shader type");
               if (pdevice->rad_info.gfx_level >= GFX10 && info->is_ngg &&
      (stage == MESA_SHADER_VERTEX || stage == MESA_SHADER_TESS_EVAL || stage == MESA_SHADER_GEOMETRY ||
   stage == MESA_SHADER_MESH)) {
   unsigned gs_vgpr_comp_cnt, es_vgpr_comp_cnt;
   gl_shader_stage es_stage = stage;
   if (stage == MESA_SHADER_GEOMETRY)
            /* VGPR5-8: (VertexID, UserVGPR0, UserVGPR1, UserVGPR2 / InstanceID) */
   if (es_stage == MESA_SHADER_VERTEX) {
         } else if (es_stage == MESA_SHADER_TESS_EVAL) {
      bool enable_prim_id = info->outinfo.export_prim_id || info->uses_prim_id;
      } else if (es_stage == MESA_SHADER_MESH) {
         } else {
                  /* GS vertex offsets in NGG:
   * - in passthrough mode, they are all packed into VGPR0
   * - in the default mode: VGPR0: offsets 0, 1; VGPR1: offsets 2, 3
   *
   * The vertex offset 2 is always needed when NGG isn't in passthrough mode
   * and uses triangle input primitives, including with NGG culling.
   */
            /* TES only needs vertex offset 2 for triangles or quads. */
   if (stage == MESA_SHADER_TESS_EVAL)
                  if (info->uses_invocation_id) {
         } else if (info->uses_prim_id || (es_stage == MESA_SHADER_VERTEX && info->outinfo.export_prim_id)) {
         } else if (need_gs_vtx_offset2) {
         } else {
                  /* Disable the WGP mode on gfx10.3 because it can hang. (it
   * happened on VanGogh) Let's disable it on all chips that
   * disable exactly 1 CU per SA for GS.
   */
   config->rsrc1 |= S_00B228_GS_VGPR_COMP_CNT(gs_vgpr_comp_cnt) | S_00B228_WGP_MODE(wgp_mode);
   config->rsrc2 |= S_00B22C_ES_VGPR_COMP_CNT(es_vgpr_comp_cnt) | S_00B22C_LDS_SIZE(config->lds_size) |
      } else if (pdevice->rad_info.gfx_level >= GFX9 && stage == MESA_SHADER_GEOMETRY) {
      unsigned es_type = info->gs.es_type;
            if (es_type == MESA_SHADER_VERTEX) {
      /* VGPR0-3: (VertexID, InstanceID / StepRate0, ...) */
   if (info->vs.needs_instance_id) {
         } else {
            } else if (es_type == MESA_SHADER_TESS_EVAL) {
         } else {
                  /* If offsets 4, 5 are used, GS_VGPR_COMP_CNT is ignored and
   * VGPR[0:4] are always loaded.
   */
   if (info->uses_invocation_id) {
         } else if (info->uses_prim_id) {
         } else if (info->gs.vertices_in >= 3) {
         } else {
                  config->rsrc1 |= S_00B228_GS_VGPR_COMP_CNT(gs_vgpr_comp_cnt) | S_00B228_WGP_MODE(wgp_mode);
   config->rsrc2 |=
      } else if (pdevice->rad_info.gfx_level >= GFX9 && stage == MESA_SHADER_TESS_CTRL) {
         } else {
                     }
      static bool
   radv_shader_binary_upload(struct radv_device *device, const struct radv_shader_binary *binary,
         {
      shader->code = calloc(shader->code_size, 1);
   if (!shader->code) {
      radv_shader_unref(device, shader);
                  #if !defined(USE_LIBELF)
         #else
                  if (!radv_open_rtld_binary(device, binary, &rtld_binary)) {
      free(shader);
               struct ac_rtld_upload_info info = {
      .binary = &rtld_binary,
   .rx_va = radv_shader_get_va(shader),
               if (!ac_rtld_upload(&info)) {
      radv_shader_unref(device, shader);
   ac_rtld_close(&rtld_binary);
                        if (shader->code) {
      /* Instead of running RTLD twice, just copy the relocated binary back from VRAM.
   * Use streaming memcpy to reduce penalty of copying from uncachable memory.
   */
      #endif
      } else {
      struct radv_shader_binary_legacy *bin = (struct radv_shader_binary_legacy *)binary;
            if (shader->code) {
                        }
      static VkResult
   radv_shader_dma_resize_upload_buf(struct radv_shader_dma_submission *submission, struct radeon_winsys *ws,
         {
      if (submission->bo)
            VkResult result = ws->buffer_create(
      ws, size, RADV_SHADER_ALLOC_ALIGNMENT, RADEON_DOMAIN_GTT,
   RADEON_FLAG_CPU_ACCESS | RADEON_FLAG_NO_INTERPROCESS_SHARING | RADEON_FLAG_32BIT | RADEON_FLAG_GTT_WC,
      if (result != VK_SUCCESS)
            submission->ptr = ws->buffer_map(submission->bo);
               }
      struct radv_shader_dma_submission *
   radv_shader_dma_pop_submission(struct radv_device *device)
   {
                        while (list_is_empty(&device->shader_dma_submissions))
            submission = list_first_entry(&device->shader_dma_submissions, struct radv_shader_dma_submission, list);
                        }
      void
   radv_shader_dma_push_submission(struct radv_device *device, struct radv_shader_dma_submission *submission, uint64_t seq)
   {
                        list_addtail(&submission->list, &device->shader_dma_submissions);
               }
      struct radv_shader_dma_submission *
   radv_shader_dma_get_submission(struct radv_device *device, struct radeon_winsys_bo *bo, uint64_t va, uint64_t size)
   {
      struct radv_shader_dma_submission *submission = radv_shader_dma_pop_submission(device);
   struct radeon_cmdbuf *cs = submission->cs;
   struct radeon_winsys *ws = device->ws;
            /* Wait for potentially in-flight submission to settle */
   result = radv_shader_wait_for_upload(device, submission->seq);
   if (result != VK_SUCCESS)
                     if (submission->bo_size < size) {
      result = radv_shader_dma_resize_upload_buf(submission, ws, size);
   if (result != VK_SUCCESS)
               radv_sdma_copy_buffer(device, cs, radv_buffer_get_va(submission->bo), va, size);
   radv_cs_add_buffer(ws, cs, submission->bo);
            result = ws->cs_finalize(cs);
   if (result != VK_SUCCESS)
                  fail:
                  }
      /*
   * If upload_seq_out is NULL, this function blocks until the DMA is complete. Otherwise, the
   * semaphore value to wait on device->shader_upload_sem is stored in *upload_seq_out.
   */
   bool
   radv_shader_dma_submit(struct radv_device *device, struct radv_shader_dma_submission *submission,
         {
      struct radeon_cmdbuf *cs = submission->cs;
   struct radeon_winsys *ws = device->ws;
                              struct vk_semaphore *semaphore = vk_semaphore_from_handle(device->shader_upload_sem);
   struct vk_sync *sync = vk_semaphore_get_active_sync(semaphore);
   const struct vk_sync_signal signal_info = {
      .sync = sync,
   .signal_value = upload_seq,
               struct radv_winsys_submit_info submit = {
      .ip_type = AMD_IP_SDMA,
   .queue_index = 0,
   .cs_array = &cs,
               result = ws->cs_submit(device->shader_upload_hw_ctx, &submit, 0, NULL, 1, &signal_info);
   if (result != VK_SUCCESS) {
      mtx_unlock(&device->shader_upload_hw_ctx_mutex);
   radv_shader_dma_push_submission(device, submission, 0);
      }
   device->shader_upload_seq = upload_seq;
                     if (upload_seq_out) {
         } else {
      result = radv_shader_wait_for_upload(device, upload_seq);
   if (result != VK_SUCCESS)
                  }
      static bool
   radv_shader_upload(struct radv_device *device, struct radv_shader *shader, const struct radv_shader_binary *binary)
   {
      if (device->shader_use_invisible_vram) {
      struct radv_shader_dma_submission *submission =
         if (!submission)
            if (!radv_shader_binary_upload(device, binary, shader, submission->ptr)) {
      radv_shader_dma_push_submission(device, submission, 0);
               if (!radv_shader_dma_submit(device, submission, &shader->upload_seq))
      } else {
               if (!radv_shader_binary_upload(device, binary, shader, dest_ptr))
      }
      }
      VkResult
   radv_shader_create_uncached(struct radv_device *device, const struct radv_shader_binary *binary, bool replayable,
         {
      VkResult result = VK_SUCCESS;
   struct radv_shader *shader = calloc(1, sizeof(struct radv_shader));
   if (!shader) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
      }
                              /* Copy the shader binary configuration. */
               #if !defined(USE_LIBELF)
         #else
                  if (!radv_open_rtld_binary(device, binary, &rtld_binary)) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               shader->code_size = rtld_binary.rx_size;
   shader->exec_size = rtld_binary.exec_size;
   #endif
      } else {
               shader->code_size = bin->code_size;
            if (bin->stats_size) {
      shader->statistics = calloc(bin->stats_size, 1);
                  if (replay_block) {
      shader->alloc = radv_replay_shader_arena_block(device, replay_block, shader);
   if (!shader->alloc) {
      result = VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS;
                  } else {
      shader->alloc = radv_alloc_shader_memory(device, shader->code_size, replayable, shader);
   if (!shader->alloc) {
      result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                  shader->bo = shader->alloc->arena->bo;
            if (!radv_shader_upload(device, shader, binary)) {
      result = VK_ERROR_OUT_OF_DEVICE_MEMORY;
                     out:
      if (result != VK_SUCCESS) {
      free(shader);
                  }
      bool
   radv_shader_reupload(struct radv_device *device, struct radv_shader *shader)
   {
      if (device->shader_use_invisible_vram) {
      struct radv_shader_dma_submission *submission =
         if (!submission)
                     if (!radv_shader_dma_submit(device, submission, &shader->upload_seq))
      } else {
      void *dest_ptr = shader->alloc->arena->ptr + shader->alloc->offset;
      }
      }
      static bool
   radv_shader_part_binary_upload(struct radv_device *device, const struct radv_shader_part_binary *bin,
         {
      struct radv_shader_dma_submission *submission = NULL;
            if (device->shader_use_invisible_vram) {
      uint64_t va = radv_buffer_get_va(shader_part->alloc->arena->bo) + shader_part->alloc->offset;
   submission = radv_shader_dma_get_submission(device, shader_part->alloc->arena->bo, va, bin->code_size);
   if (!submission)
               } else {
                           if (device->shader_use_invisible_vram) {
      if (!radv_shader_dma_submit(device, submission, &shader_part->upload_seq))
                  }
      struct radv_shader_part *
   radv_shader_part_create(struct radv_device *device, struct radv_shader_part_binary *binary, unsigned wave_size)
   {
               shader_part = calloc(1, sizeof(struct radv_shader_part));
   if (!shader_part)
            shader_part->ref_count = 1;
   shader_part->code_size = binary->code_size;
   shader_part->rsrc1 =
                           /* Allocate memory and upload. */
   shader_part->alloc = radv_alloc_shader_memory(device, shader_part->code_size, false, NULL);
   if (!shader_part->alloc)
            shader_part->bo = shader_part->alloc->arena->bo;
            if (!radv_shader_part_binary_upload(device, binary, shader_part))
                  fail:
      radv_shader_part_destroy(device, shader_part);
      }
      static char *
   radv_dump_nir_shaders(struct nir_shader *const *shaders, int shader_count)
   {
      char *data = NULL;
   char *ret = NULL;
   size_t size = 0;
   struct u_memstream mem;
   if (u_memstream_open(&mem, &data, &size)) {
      FILE *const memf = u_memstream_get(&mem);
   for (int i = 0; i < shader_count; ++i)
                     ret = malloc(size + 1);
   if (ret) {
      memcpy(ret, data, size);
      }
   free(data);
      }
      static void
   radv_aco_build_shader_binary(void **bin, const struct ac_shader_config *config, const char *llvm_ir_str,
                     {
      struct radv_shader_binary **binary = (struct radv_shader_binary **)bin;
            size += disasm_size;
                     /* We need to calloc to prevent uninitialized data because this will be used
   * directly for the disk cache. Uninitialized data can appear because of
   * padding in the struct or because legacy_binary->data can be at an offset
   * from the start less than sizeof(radv_shader_binary_legacy). */
   struct radv_shader_binary_legacy *legacy_binary = (struct radv_shader_binary_legacy *)calloc(size, 1);
   legacy_binary->base.type = RADV_BINARY_TYPE_LEGACY;
   legacy_binary->base.total_size = size;
            if (stats_size)
                  memcpy(legacy_binary->data + legacy_binary->stats_size, code, code_dw * sizeof(uint32_t));
   legacy_binary->exec_size = exec_size;
            legacy_binary->disasm_size = 0;
            if (llvm_ir_size) {
      memcpy((char *)legacy_binary->data + legacy_binary->stats_size + legacy_binary->code_size, llvm_ir_str,
               legacy_binary->disasm_size = disasm_size;
   if (disasm_size) {
      memcpy((char *)legacy_binary->data + legacy_binary->stats_size + legacy_binary->code_size + llvm_ir_size,
      }
      }
      static void
   radv_fill_nir_compiler_options(struct radv_nir_compiler_options *options, struct radv_device *device,
               {
      if (key)
            /* robust_buffer_access_llvm here used by LLVM only, pipeline robustness is not exposed there. */
   options->robust_buffer_access_llvm = device->buffer_robustness >= RADV_BUFFER_ROBUSTNESS_1;
   options->wgp_mode = should_use_wgp;
   options->info = &device->physical_device->rad_info;
   options->dump_shader = can_dump_shader;
   options->dump_preoptir = options->dump_shader && device->instance->debug_flags & RADV_DEBUG_PREOPTIR;
   options->record_ir = keep_shader_info;
   options->record_stats = keep_statistic_info;
   options->check_ir = device->instance->debug_flags & RADV_DEBUG_CHECKIR;
      }
      static void
   radv_capture_shader_executable_info(struct radv_device *device, struct radv_shader *shader,
               {
                  #if !defined(USE_LIBELF)
         #else
         struct radv_shader_binary_rtld *bin = (struct radv_shader_binary_rtld *)binary;
            if (!radv_open_rtld_binary(device, binary, &rtld_binary)) {
                  const char *disasm_data;
   size_t disasm_size;
   if (!ac_rtld_get_section_by_name(&rtld_binary, ".AMDGPU.disasm", &disasm_data, &disasm_size)) {
                  shader->ir_string = bin->llvm_ir_size ? strdup((const char *)(bin->data + bin->elf_size)) : NULL;
   shader->disasm_string = malloc(disasm_size + 1);
   memcpy(shader->disasm_string, disasm_data, disasm_size);
            #endif
      } else {
               shader->ir_string = bin->ir_size ? strdup((const char *)(bin->data + bin->stats_size + bin->code_size)) : NULL;
   shader->disasm_string =
         }
      static struct radv_shader_binary *
   shader_compile(struct radv_device *device, struct nir_shader *const *shaders, int shader_count, gl_shader_stage stage,
               {
      struct radv_shader_debug_data debug_data = {
      .device = device,
      };
   options->debug.func = radv_compiler_debug;
                  #ifdef LLVM_AVAILABLE
      if (radv_use_llvm_for_stage(device, stage) || options->dump_shader || options->record_ir)
            if (radv_use_llvm_for_stage(device, stage)) {
      #else
         #endif
      } else {
      struct aco_shader_info ac_info;
   struct aco_compiler_options ac_opts;
   radv_aco_convert_opts(&ac_opts, options, args);
   radv_aco_convert_shader_info(&ac_info, info, args, &options->key, options->info->gfx_level);
   aco_compile_shader(&ac_opts, &ac_info, shader_count, shaders, &args->ac, &radv_aco_build_shader_binary,
                        if (!radv_postprocess_binary_config(device, binary, args)) {
      free(binary);
                  }
      struct radv_shader_binary *
   radv_shader_nir_to_asm(struct radv_device *device, struct radv_shader_stage *pl_stage,
               {
      gl_shader_stage stage = shaders[shader_count - 1]->info.stage;
            struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, key, radv_should_use_wgp_mode(device, stage, info),
                  struct radv_shader_binary *binary =
               }
      void
   radv_shader_generate_debug_info(struct radv_device *device, bool dump_shader, bool keep_shader_info,
               {
      if (dump_shader || keep_shader_info)
            if (dump_shader) {
      fprintf(stderr, "%s", radv_get_shader_name(info, shaders[0]->info.stage));
   for (int i = 1; i < shader_count; ++i)
                  }
      struct radv_shader *
   radv_create_trap_handler_shader(struct radv_device *device)
   {
      gl_shader_stage stage = MESA_SHADER_COMPUTE;
   struct radv_shader_info info = {0};
   struct radv_pipeline_key key = {0};
   struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, &key, radv_should_use_wgp_mode(device, stage, &info), false, false,
                     info.wave_size = 64;
            struct radv_shader_args args;
            struct radv_shader_binary *binary = shader_compile(device, &b.shader, 1, stage, &info, &args, &options);
   struct radv_shader *shader;
            ralloc_free(b.shader);
               }
      static void
   radv_aco_build_shader_part(void **bin, uint32_t num_sgprs, uint32_t num_vgprs, const uint32_t *code, uint32_t code_size,
         {
      struct radv_shader_part_binary **binary = (struct radv_shader_part_binary **)bin;
            size += disasm_size;
            part_binary->num_sgprs = num_sgprs;
   part_binary->num_vgprs = num_vgprs;
   part_binary->total_size = size;
   part_binary->code_size = code_size * sizeof(uint32_t);
   memcpy(part_binary->data, code, part_binary->code_size);
   if (disasm_size) {
      memcpy((char *)part_binary->data + part_binary->code_size, disasm_str, disasm_size);
                  }
      struct radv_shader *
   radv_create_rt_prolog(struct radv_device *device)
   {
      struct radv_shader *prolog;
   struct radv_shader_args in_args = {0};
   struct radv_shader_args out_args = {0};
   struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, NULL, false,
               struct radv_shader_info info = {0};
   info.stage = MESA_SHADER_COMPUTE;
   info.loads_push_constants = true;
   info.desc_set_used_mask = -1; /* just to force indirection */
   info.wave_size = device->physical_device->rt_wave_size;
   info.workgroup_size = info.wave_size;
   info.user_data_0 = R_00B900_COMPUTE_USER_DATA_0;
   info.cs.is_rt_shader = true;
   info.cs.uses_ray_launch_size = true;
   info.cs.uses_dynamic_rt_callable_stack = true;
   info.cs.block_size[0] = 8;
   info.cs.block_size[1] = device->physical_device->rt_wave_size == 64 ? 8 : 4;
   info.cs.block_size[2] = 1;
   info.cs.uses_thread_id[0] = true;
   info.cs.uses_thread_id[1] = true;
   for (unsigned i = 0; i < 3; i++)
                     radv_declare_shader_args(device, &pipeline_key, &info, MESA_SHADER_COMPUTE, MESA_SHADER_NONE, &in_args);
   radv_declare_rt_shader_args(options.info->gfx_level, &out_args);
         #ifdef LLVM_AVAILABLE
      if (options.dump_shader || options.record_ir)
      #endif
         struct radv_shader_binary *binary = NULL;
   struct aco_shader_info ac_info;
   struct aco_compiler_options ac_opts;
   radv_aco_convert_shader_info(&ac_info, &info, &in_args, &options.key, options.info->gfx_level);
   radv_aco_convert_opts(&ac_opts, &options, &in_args);
   aco_compile_rt_prolog(&ac_opts, &ac_info, &in_args.ac, &out_args.ac, &radv_aco_build_shader_binary,
                  radv_postprocess_binary_config(device, binary, &in_args);
   radv_shader_create_uncached(device, binary, false, NULL, &prolog);
   if (!prolog)
            if (device->keep_shader_info || options.dump_shader) {
                  if (options.dump_shader) {
      fprintf(stderr, "Raytracing prolog");
            done:
      free(binary);
      }
      struct radv_shader_part *
   radv_create_vs_prolog(struct radv_device *device, const struct radv_vs_prolog_key *key)
   {
      struct radv_shader_part *prolog;
   struct radv_shader_args args = {0};
   struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, NULL, false,
                  struct radv_shader_info info = {0};
   info.stage = MESA_SHADER_VERTEX;
   info.wave_size = key->wave32 ? 32 : 64;
   info.vs.needs_instance_id = true;
   info.vs.needs_base_instance = true;
   info.vs.needs_draw_id = true;
   info.vs.use_per_attribute_vb_descs = true;
   info.vs.vb_desc_usage_mask = BITFIELD_MASK(key->num_attributes);
   info.vs.has_prolog = true;
   info.vs.as_ls = key->as_ls;
                     radv_declare_shader_args(device, &pipeline_key, &info, key->next_stage,
            info.user_sgprs_locs = args.user_sgprs_locs;
         #ifdef LLVM_AVAILABLE
      if (options.dump_shader || options.record_ir)
      #endif
         struct radv_shader_part_binary *binary = NULL;
   struct aco_shader_info ac_info;
   struct aco_vs_prolog_info ac_prolog_info;
   struct aco_compiler_options ac_opts;
   radv_aco_convert_shader_info(&ac_info, &info, &args, &options.key, options.info->gfx_level);
   radv_aco_convert_opts(&ac_opts, &options, &args);
   radv_aco_convert_vs_prolog_key(&ac_prolog_info, key, &args);
            prolog = radv_shader_part_create(device, binary, info.wave_size);
   if (!prolog)
                     if (options.dump_shader) {
      fprintf(stderr, "Vertex prolog");
                              fail:
      free(binary);
      }
      struct radv_shader_part *
   radv_create_ps_epilog(struct radv_device *device, const struct radv_ps_epilog_key *key,
         {
      struct radv_shader_part *epilog;
   struct radv_shader_args args = {0};
   struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, NULL, false,
                  struct radv_shader_info info = {0};
   info.stage = MESA_SHADER_FRAGMENT;
   info.wave_size = device->physical_device->ps_wave_size;
                  #ifdef LLVM_AVAILABLE
      if (options.dump_shader || options.record_ir)
      #endif
         struct radv_shader_part_binary *binary = NULL;
   struct aco_shader_info ac_info;
   struct aco_ps_epilog_info ac_epilog_info = {0};
   struct aco_compiler_options ac_opts;
   radv_aco_convert_shader_info(&ac_info, &info, &args, &options.key, options.info->gfx_level);
   radv_aco_convert_opts(&ac_opts, &options, &args);
   radv_aco_convert_ps_epilog_key(&ac_epilog_info, key, &args);
                     epilog = radv_shader_part_create(device, binary, info.wave_size);
   if (!epilog)
            if (options.dump_shader) {
      fprintf(stderr, "Fragment epilog");
               if (binary_out) {
         } else {
                        fail:
      free(binary);
      }
      struct radv_shader_part *
   radv_create_tcs_epilog(struct radv_device *device, const struct radv_tcs_epilog_key *key)
   {
      struct radv_shader_part *epilog;
   struct radv_shader_args args = {0};
   struct radv_nir_compiler_options options = {0};
   radv_fill_nir_compiler_options(&options, device, NULL, false,
                  struct radv_shader_info info = {0};
   info.stage = MESA_SHADER_TESS_CTRL;
   info.wave_size = device->physical_device->ge_wave_size;
                  #ifdef LLVM_AVAILABLE
      if (options.dump_shader || options.record_ir)
      #endif
         struct radv_shader_part_binary *binary = NULL;
   struct aco_shader_info ac_info;
   struct aco_tcs_epilog_info ac_epilog_info;
   struct aco_compiler_options ac_opts;
   radv_aco_convert_shader_info(&ac_info, &info, &args, &options.key, options.info->gfx_level);
   radv_aco_convert_opts(&ac_opts, &options, &args);
   radv_aco_convert_tcs_epilog_key(&ac_epilog_info, key, &args);
            epilog = radv_shader_part_create(device, binary, info.wave_size);
   if (!epilog)
            if (options.dump_shader) {
      fprintf(stderr, "TCS epilog");
               free(binary);
         fail:
      free(binary);
      }
      void
   radv_shader_part_destroy(struct radv_device *device, struct radv_shader_part *shader_part)
   {
               if (device->shader_use_invisible_vram) {
      /* Wait for any pending upload to complete, or we'll be writing into freed shader memory. */
               if (shader_part->alloc)
         free(shader_part->disasm_string);
      }
      uint64_t
   radv_shader_get_va(const struct radv_shader *shader)
   {
         }
      struct radv_shader *
   radv_find_shader(struct radv_device *device, uint64_t pc)
   {
      mtx_lock(&device->shader_arena_mutex);
      #ifdef __GNUC__
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Wshadow"
   #endif
         #ifdef __GNUC__
   #pragma GCC diagnostic pop
   #endif
            uint64_t start = radv_buffer_get_va(block->arena->bo) + block->offset;
   if (!block->freelist.prev && pc >= start && pc < start + block->size) {
      mtx_unlock(&device->shader_arena_mutex);
                     mtx_unlock(&device->shader_arena_mutex);
      }
      const char *
   radv_get_shader_name(const struct radv_shader_info *info, gl_shader_stage stage)
   {
      switch (stage) {
   case MESA_SHADER_VERTEX:
      if (info->vs.as_ls)
         else if (info->vs.as_es)
         else if (info->is_ngg)
         else
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
      if (info->tes.as_es)
         else if (info->is_ngg)
         else
      case MESA_SHADER_GEOMETRY:
         case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
         case MESA_SHADER_MESH:
         case MESA_SHADER_TASK:
         case MESA_SHADER_RAYGEN:
         case MESA_SHADER_CLOSEST_HIT:
         case MESA_SHADER_INTERSECTION:
         case MESA_SHADER_ANY_HIT:
         case MESA_SHADER_MISS:
         case MESA_SHADER_CALLABLE:
         default:
            }
      unsigned
   radv_get_max_waves(const struct radv_device *device, struct radv_shader *shader, gl_shader_stage stage)
   {
      const struct radeon_info *info = &device->physical_device->rad_info;
   const enum amd_gfx_level gfx_level = info->gfx_level;
   const uint8_t wave_size = shader->info.wave_size;
   const struct ac_shader_config *conf = &shader->config;
   unsigned max_simd_waves;
                     if (stage == MESA_SHADER_FRAGMENT) {
      lds_per_wave = conf->lds_size * info->lds_encode_granularity + shader->info.ps.num_interp * 48;
      } else if (stage == MESA_SHADER_COMPUTE || stage == MESA_SHADER_TASK) {
      unsigned max_workgroup_size = shader->info.workgroup_size;
   lds_per_wave = align(conf->lds_size * info->lds_encode_granularity, info->lds_alloc_granularity);
               if (conf->num_sgprs && gfx_level < GFX10) {
      unsigned sgprs = align(conf->num_sgprs, gfx_level >= GFX8 ? 16 : 8);
               if (conf->num_vgprs) {
      unsigned physical_vgprs = info->num_physical_wave64_vgprs_per_simd * (64 / wave_size);
   unsigned vgprs = align(conf->num_vgprs, wave_size == 32 ? 8 : 4);
   if (gfx_level >= GFX10_3) {
      unsigned real_vgpr_gran = info->num_physical_wave64_vgprs_per_simd / 64;
      }
               unsigned simd_per_workgroup = info->num_simd_per_compute_unit;
   if (gfx_level >= GFX10)
            unsigned max_lds_per_simd = info->lds_size_per_workgroup / simd_per_workgroup;
   if (lds_per_wave)
               }
      unsigned
   radv_get_max_scratch_waves(const struct radv_device *device, struct radv_shader *shader)
   {
                  }
      unsigned
   radv_compute_spi_ps_input(const struct radv_pipeline_key *pipeline_key, const struct radv_shader_info *info)
   {
               spi_ps_input = S_0286CC_PERSP_CENTER_ENA(info->ps.reads_persp_center) |
                  S_0286CC_PERSP_CENTROID_ENA(info->ps.reads_persp_centroid) |
   S_0286CC_PERSP_SAMPLE_ENA(info->ps.reads_persp_sample) |
   S_0286CC_LINEAR_CENTER_ENA(info->ps.reads_linear_center) |
   S_0286CC_LINEAR_CENTROID_ENA(info->ps.reads_linear_centroid) |
         if (info->ps.reads_frag_coord_mask || info->ps.reads_sample_pos_mask) {
               for (unsigned i = 0; i < 4; i++) {
      if (mask & (1 << i))
               if (pipeline_key->adjust_frag_coord_z && info->ps.reads_frag_coord_mask & (1 << 2)) {
                     if (info->ps.reads_sample_id || info->ps.reads_frag_shading_rate || info->ps.reads_sample_mask_in) {
                  if (info->ps.reads_sample_mask_in || info->ps.reads_fully_covered) {
                  if (G_0286CC_POS_W_FLOAT_ENA(spi_ps_input)) {
      /* If POS_W_FLOAT (11) is enabled, at least one of PERSP_* must be enabled too */
               if (!(spi_ps_input & 0x7F)) {
      /* At least one of PERSP_* (0xF) or LINEAR_* (0x70) must be enabled */
                  }
      VkResult
   radv_dump_shader_stats(struct radv_device *device, struct radv_pipeline *pipeline, struct radv_shader *shader,
         {
      VkPipelineExecutablePropertiesKHR *props = NULL;
   uint32_t prop_count = 0;
            VkPipelineInfoKHR pipeline_info = {0};
   pipeline_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
            result = radv_GetPipelineExecutablePropertiesKHR(radv_device_to_handle(device), &pipeline_info, &prop_count, NULL);
   if (result != VK_SUCCESS)
            props = calloc(prop_count, sizeof(*props));
   if (!props)
            result = radv_GetPipelineExecutablePropertiesKHR(radv_device_to_handle(device), &pipeline_info, &prop_count, props);
   if (result != VK_SUCCESS)
            for (unsigned exec_idx = 0; exec_idx < prop_count; exec_idx++) {
      if (!(props[exec_idx].stages & mesa_to_vk_shader_stage(stage)))
            VkPipelineExecutableStatisticKHR *stats = NULL;
            VkPipelineExecutableInfoKHR exec_info = {0};
   exec_info.pipeline = radv_pipeline_to_handle(pipeline);
            result = radv_GetPipelineExecutableStatisticsKHR(radv_device_to_handle(device), &exec_info, &stat_count, NULL);
   if (result != VK_SUCCESS)
            stats = calloc(stat_count, sizeof(*stats));
   if (!stats) {
      result = VK_ERROR_OUT_OF_HOST_MEMORY;
               result = radv_GetPipelineExecutableStatisticsKHR(radv_device_to_handle(device), &exec_info, &stat_count, stats);
   if (result != VK_SUCCESS) {
      free(stats);
               fprintf(output, "\n%s:\n", radv_get_shader_name(&shader->info, stage));
            for (unsigned i = 0; i < stat_count; i++) {
      fprintf(output, "%s: ", stats[i].name);
   switch (stats[i].format) {
   case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
      fprintf(output, "%s", stats[i].value.b32 == VK_TRUE ? "true" : "false");
      case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
      fprintf(output, "%" PRIi64, stats[i].value.i64);
      case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
      fprintf(output, "%" PRIu64, stats[i].value.u64);
      case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
      fprintf(output, "%f", stats[i].value.f64);
      default:
         }
                                 fail:
      free(props);
      }
