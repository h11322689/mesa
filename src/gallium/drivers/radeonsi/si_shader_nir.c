   /*
   * Copyright 2017 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "nir_builder.h"
   #include "nir_xfb_info.h"
   #include "si_pipe.h"
   #include "ac_nir.h"
         bool si_alu_to_scalar_packed_math_filter(const nir_instr *instr, const void *data)
   {
      if (instr->type == nir_instr_type_alu) {
               if (alu->def.bit_size == 16 &&
      alu->def.num_components == 2)
               }
      static uint8_t si_vectorize_callback(const nir_instr *instr, const void *data)
   {
      if (instr->type != nir_instr_type_alu)
            nir_alu_instr *alu = nir_instr_as_alu(instr);
   if (alu->def.bit_size == 16) {
      switch (alu->op) {
   case nir_op_unpack_32_2x16_split_x:
   case nir_op_unpack_32_2x16_split_y:
         default:
                        }
      static unsigned si_lower_bit_size_callback(const nir_instr *instr, void *data)
   {
      if (instr->type != nir_instr_type_alu)
                     switch (alu->op) {
   case nir_op_imul_high:
   case nir_op_umul_high:
      if (alu->def.bit_size < 32)
            default:
                     }
      void si_nir_opts(struct si_screen *sscreen, struct nir_shader *nir, bool first)
   {
               do {
      progress = false;
   bool lower_alu_to_scalar = false;
            NIR_PASS(progress, nir, nir_lower_vars_to_ssa);
   NIR_PASS(progress, nir, nir_lower_alu_to_scalar,
                  if (first) {
      NIR_PASS(progress, nir, nir_split_array_vars, nir_var_function_temp);
   NIR_PASS(lower_alu_to_scalar, nir, nir_shrink_vec_array_vars, nir_var_function_temp);
      }
   NIR_PASS(progress, nir, nir_opt_copy_prop_vars);
            NIR_PASS(lower_alu_to_scalar, nir, nir_opt_trivial_continues);
   /* (Constant) copy propagation is needed for txf with offsets. */
   NIR_PASS(progress, nir, nir_copy_prop);
   NIR_PASS(progress, nir, nir_opt_remove_phis);
   NIR_PASS(progress, nir, nir_opt_dce);
   /* nir_opt_if_optimize_phi_true_false is disabled on LLVM14 (#6976) */
   NIR_PASS(lower_phis_to_scalar, nir, nir_opt_if,
                        if (lower_alu_to_scalar) {
      NIR_PASS_V(nir, nir_lower_alu_to_scalar,
      }
   if (lower_phis_to_scalar)
                  NIR_PASS(progress, nir, nir_opt_cse);
            /* Needed for algebraic lowering */
   NIR_PASS(progress, nir, nir_lower_bit_size, si_lower_bit_size_callback, NULL);
   NIR_PASS(progress, nir, nir_opt_algebraic);
            if (!nir->info.flrp_lowered) {
      unsigned lower_flrp = (nir->options->lower_flrp16 ? 16 : 0) |
                              NIR_PASS(lower_flrp_progress, nir, nir_lower_flrp, lower_flrp, false /* always_precise */);
   if (lower_flrp_progress) {
      NIR_PASS(progress, nir, nir_opt_constant_folding);
               /* Nothing should rematerialize any flrps, so we only
   * need to do this lowering once.
   */
               NIR_PASS(progress, nir, nir_opt_undef);
   NIR_PASS(progress, nir, nir_opt_conditional_discard);
   if (nir->options->max_unroll_iterations) {
                  if (nir->info.stage == MESA_SHADER_FRAGMENT)
            if (sscreen->info.has_packed_math_16bit)
                  }
      void si_nir_late_opts(nir_shader *nir)
   {
      bool more_late_algebraic = true;
   while (more_late_algebraic) {
      more_late_algebraic = false;
   NIR_PASS(more_late_algebraic, nir, nir_opt_algebraic_late);
            /* We should run this after constant folding for stages that support indirect
   * inputs/outputs.
   */
   if (nir->options->support_indirect_inputs & BITFIELD_BIT(nir->info.stage) ||
                  NIR_PASS_V(nir, nir_copy_prop);
   NIR_PASS_V(nir, nir_opt_dce);
         }
      static void si_late_optimize_16bit_samplers(struct si_screen *sscreen, nir_shader *nir)
   {
      /* Optimize types of image_sample sources and destinations.
   *
   * The image_sample sources bit sizes are:
   *   nir_tex_src_coord:       a16 ? 16 : 32
   *   nir_tex_src_comparator:  32
   *   nir_tex_src_offset:      32
   *   nir_tex_src_bias:        a16 ? 16 : 32
   *   nir_tex_src_lod:         a16 ? 16 : 32
   *   nir_tex_src_min_lod:     a16 ? 16 : 32
   *   nir_tex_src_ms_index:    a16 ? 16 : 32
   *   nir_tex_src_ddx:         has_g16 ? (g16 ? 16 : 32) : (a16 ? 16 : 32)
   *   nir_tex_src_ddy:         has_g16 ? (g16 ? 16 : 32) : (a16 ? 16 : 32)
   *
   * We only use a16/g16 if all of the affected sources are 16bit.
   */
   bool has_g16 = sscreen->info.gfx_level >= GFX10;
   struct nir_fold_tex_srcs_options fold_srcs_options[] = {
      {
      .sampler_dims =
         .src_types = (1 << nir_tex_src_coord) | (1 << nir_tex_src_lod) |
                  },
   {
      .sampler_dims = ~BITFIELD_BIT(GLSL_SAMPLER_DIM_CUBE),
         };
   struct nir_fold_16bit_tex_image_options fold_16bit_options = {
      .rounding_mode = nir_rounding_mode_rtz,
   .fold_tex_dest_types = nir_type_float,
   .fold_image_dest_types = nir_type_float,
   .fold_image_store_data = true,
   .fold_srcs_options_count = has_g16 ? 2 : 1,
      };
   bool changed = false;
            if (changed) {
      si_nir_opts(sscreen, nir, false);
         }
      static bool
   lower_intrinsic_filter(const nir_instr *instr, const void *dummy)
   {
         }
      static nir_def *
   lower_intrinsic_instr(nir_builder *b, nir_instr *instr, void *dummy)
   {
               switch (intrin->intrinsic) {
   case nir_intrinsic_is_sparse_texels_resident:
      /* code==0 means sparse texels are resident */
      case nir_intrinsic_sparse_residency_code_and:
         default:
            }
      static bool si_lower_intrinsics(nir_shader *nir)
   {
      return nir_shader_lower_instructions(nir,
                  }
      const nir_lower_subgroups_options si_nir_subgroups_options = {
      .subgroup_size = 64,
   .ballot_bit_size = 64,
   .ballot_components = 1,
   .lower_to_scalar = true,
   .lower_subgroup_masks = true,
   .lower_vote_trivial = false,
   .lower_vote_eq = true,
      };
      /**
   * Perform "lowering" operations on the NIR that are run once when the shader
   * selector is created.
   */
   static void si_lower_nir(struct si_screen *sscreen, struct nir_shader *nir)
   {
      /* Perform lowerings (and optimizations) of code.
   *
   * Performance considerations aside, we must:
   * - lower certain ALU operations
   * - ensure constant offsets for texture instructions are folded
   *   and copy-propagated
            const struct nir_lower_tex_options lower_tex_options = {
      .lower_txp = ~0u,
   .lower_txf_offset = true,
   .lower_txs_cube_array = true,
   .lower_invalid_implicit_lod = true,
   .lower_tg4_offsets = true,
      };
            const struct nir_lower_image_options lower_image_options = {
      .lower_cube_size = true,
      };
                                                /* Lower load constants to scalar and then clean up the mess */
   NIR_PASS_V(nir, nir_lower_load_const_to_scalar);
   NIR_PASS_V(nir, nir_lower_var_copies);
   NIR_PASS_V(nir, nir_opt_intrinsics);
   NIR_PASS_V(nir, nir_lower_system_values);
            /* si_nir_kill_outputs and ac_nir_optimize_outputs require outputs to be scalar. */
   if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_TESS_EVAL ||
   nir->info.stage == MESA_SHADER_GEOMETRY)
         if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      unsigned flags = nir_lower_gs_intrinsics_per_stream;
   if (sscreen->use_ngg) {
      flags |= nir_lower_gs_intrinsics_count_primitives |
      nir_lower_gs_intrinsics_count_vertices_per_primitive |
                        if (nir->info.stage == MESA_SHADER_COMPUTE) {
      if (nir->info.cs.derivative_group == DERIVATIVE_GROUP_QUADS) {
      /* If we are shuffling local_invocation_id for quad derivatives, we
   * need to derive local_invocation_index from local_invocation_id
   * first, so that the value corresponds to the shuffled
   * local_invocation_id.
   */
   nir_lower_compute_system_values_options options = {0};
   options.lower_local_invocation_index = true;
               nir_opt_cse(nir); /* CSE load_local_invocation_id */
   nir_lower_compute_system_values_options options = {0};
   options.shuffle_local_ids_for_quad_derivatives = true;
               if (sscreen->b.get_shader_param(&sscreen->b, PIPE_SHADER_FRAGMENT, PIPE_SHADER_CAP_FP16)) {
      NIR_PASS_V(nir, nir_lower_mediump_io,
            /* TODO: LLVM fails to compile this test if VS inputs are 16-bit:
      * dEQP-GLES31.functional.shaders.builtin_functions.integer.bitfieldinsert.uvec3_lowp_geometry
      (nir->info.stage != MESA_SHADER_VERTEX ? nir_var_shader_in : 0) | nir_var_shader_out,
            si_nir_opts(sscreen, nir, true);
   /* Run late optimizations to fuse ffma and eliminate 16-bit conversions. */
            if (sscreen->b.get_shader_param(&sscreen->b, PIPE_SHADER_FRAGMENT, PIPE_SHADER_CAP_FP16))
               }
      static bool si_mark_divergent_texture_non_uniform(struct nir_shader *nir)
   {
               /* sampler_non_uniform and texture_non_uniform are always false in GLSL,
   * but this can lead to unexpected behavior if texture/sampler index come from
   * a vertex attribute.
   *
   * For instance, 2 consecutive draws using 2 different index values,
   * could be squashed together by the hw - producing a single draw with
   * non-dynamically uniform index.
   *
   * To avoid this, detect divergent indexing, mark them as non-uniform,
   * so that we can apply waterfall loop on these index later (either llvm
   * backend or nir_lower_non_uniform_access).
   *
   * See https://gitlab.freedesktop.org/mesa/mesa/-/issues/2253
                     nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   nir_foreach_block_safe(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_tex_instr *tex = nir_instr_as_tex(instr);
                     switch (tex->src[i].src_type) {
   case nir_tex_src_texture_deref:
   case nir_tex_src_texture_handle:
      tex->texture_non_uniform |= divergent;
      case nir_tex_src_sampler_deref:
   case nir_tex_src_sampler_handle:
      tex->sampler_non_uniform |= divergent;
      default:
                     /* If dest is already divergent, divergence won't change. */
   divergence_changed |= !tex->def.divergent &&
                  nir_metadata_preserve(impl, nir_metadata_all);
      }
      char *si_finalize_nir(struct pipe_screen *screen, void *nirptr)
   {
      struct si_screen *sscreen = (struct si_screen *)screen;
            nir_lower_io_passes(nir, false);
            if (nir->info.stage == MESA_SHADER_FRAGMENT)
            NIR_PASS_V(nir, ac_nir_lower_subdword_loads,
            (ac_nir_lower_subdword_options) {
      .modes_1_comp = nir_var_mem_ubo,
            /* Remove dead derefs, so that we can remove uniforms. */
            /* Remove uniforms because those should have been lowered to UBOs already. */
   nir_foreach_variable_with_modes_safe(var, nir, nir_var_uniform) {
      if (!glsl_type_get_image_count(var->type) &&
      !glsl_type_get_texture_count(var->type) &&
   !glsl_type_get_sampler_count(var->type))
            si_lower_nir(sscreen, nir);
            /* Update xfb info after we did medium io lowering. */
   if (nir->xfb_info && nir->info.outputs_written_16bit)
            if (sscreen->options.inline_uniforms)
            /* Lower large variables that are always constant with load_constant intrinsics, which
   * get turned into PC-relative loads from a data section next to the shader.
   *
   * Run this once before lcssa because the added phis may prevent this
   * pass from operating correctly.
   *
   * nir_opt_large_constants may use op_amul (see nir_build_deref_offset),
   * or may create unneeded code, so run si_nir_opts if needed.
   */
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
   bool progress = false;
   NIR_PASS(progress, nir, nir_opt_large_constants, glsl_get_natural_size_align_bytes, 16);
   if (progress)
            NIR_PASS_V(nir, nir_convert_to_lcssa, true, true); /* required by divergence analysis */
            /* Must be after divergence analysis. */
   bool divergence_changed = false;
   NIR_PASS(divergence_changed, nir, si_mark_divergent_texture_non_uniform);
   /* Re-analysis whole shader if texture instruction divergence changed. */
   if (divergence_changed)
               }
