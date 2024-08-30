   /*
   * Copyright 2023 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "ac_nir.h"
   #include "sid.h"
   #include "nir_builder.h"
   #include "nir_builtin_builder.h"
      typedef struct {
               nir_variable *persp_center;
   nir_variable *persp_centroid;
   nir_variable *persp_sample;
   nir_variable *linear_center;
   nir_variable *linear_centroid;
   nir_variable *linear_sample;
            /* Add one for dual source blend second output. */
   nir_def *outputs[FRAG_RESULT_MAX + 1][4];
            /* MAX_DRAW_BUFFERS for MRT export, 1 for MRTZ export */
   nir_intrinsic_instr *exp[MAX_DRAW_BUFFERS + 1];
               } lower_ps_state;
      #define DUAL_SRC_BLEND_SLOT FRAG_RESULT_MAX
      static void
   create_interp_param(nir_builder *b, lower_ps_state *s)
   {
      if (s->options->force_persp_sample_interp) {
      s->persp_center =
               if (s->options->bc_optimize_for_persp ||
      s->options->force_persp_sample_interp ||
   s->options->force_persp_center_interp) {
   s->persp_centroid =
               if (s->options->force_persp_center_interp) {
      s->persp_sample =
               if (s->options->force_linear_sample_interp) {
      s->linear_center =
               if (s->options->bc_optimize_for_linear ||
      s->options->force_linear_sample_interp ||
   s->options->force_linear_center_interp) {
   s->linear_centroid =
               if (s->options->force_linear_center_interp) {
      s->linear_sample =
               s->lower_load_barycentric =
      s->persp_center || s->persp_centroid || s->persp_sample ||
   }
      static void
   init_interp_param(nir_builder *b, lower_ps_state *s)
   {
               /* The shader should do: if (PRIM_MASK[31]) CENTROID = CENTER;
   * The hw doesn't compute CENTROID if the whole wave only
   * contains fully-covered quads.
   */
   if (s->options->bc_optimize_for_persp || s->options->bc_optimize_for_linear) {
               if (s->options->bc_optimize_for_persp) {
      nir_def *center =
                        nir_def *value = nir_bcsel(b, bc_optimize, center, centroid);
               if (s->options->bc_optimize_for_linear) {
      nir_def *center =
                        nir_def *value = nir_bcsel(b, bc_optimize, center, centroid);
                  if (s->options->force_persp_sample_interp) {
      nir_def *sample =
         nir_store_var(b, s->persp_center, sample, 0x3);
               if (s->options->force_linear_sample_interp) {
      nir_def *sample =
         nir_store_var(b, s->linear_center, sample, 0x3);
               if (s->options->force_persp_center_interp) {
      nir_def *center =
         nir_store_var(b, s->persp_sample, center, 0x3);
               if (s->options->force_linear_center_interp) {
      nir_def *center =
         nir_store_var(b, s->linear_sample, center, 0x3);
         }
      static bool
   lower_ps_load_barycentric(nir_builder *b, nir_intrinsic_instr *intrin, lower_ps_state *s)
   {
      enum glsl_interp_mode mode = nir_intrinsic_interp_mode(intrin);
            switch (mode) {
   case INTERP_MODE_NONE:
   case INTERP_MODE_SMOOTH:
      switch (intrin->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
      var = s->persp_center;
      case nir_intrinsic_load_barycentric_centroid:
      var = s->persp_centroid;
      case nir_intrinsic_load_barycentric_sample:
      var = s->persp_sample;
      default:
         }
         case INTERP_MODE_NOPERSPECTIVE:
      switch (intrin->intrinsic) {
   case nir_intrinsic_load_barycentric_pixel:
      var = s->linear_center;
      case nir_intrinsic_load_barycentric_centroid:
      var = s->linear_centroid;
      case nir_intrinsic_load_barycentric_sample:
      var = s->linear_sample;
      default:
         }
         default:
                  if (!var)
                     nir_def *replacement = nir_load_var(b, var);
            nir_instr_remove(&intrin->instr);
      }
      static bool
   gather_ps_store_output(nir_builder *b, nir_intrinsic_instr *intrin, lower_ps_state *s)
   {
      nir_io_semantics sem = nir_intrinsic_io_semantics(intrin);
   unsigned write_mask = nir_intrinsic_write_mask(intrin);
   unsigned component = nir_intrinsic_component(intrin);
   nir_alu_type type = nir_intrinsic_src_type(intrin);
                     unsigned slot = sem.dual_source_blend_index ?
            u_foreach_bit (i, write_mask) {
      unsigned comp = component + i;
               /* Same slot should have same type for all components. */
                     /* Keep color output instruction if not exported in nir. */
   if (!s->options->no_color_export ||
      (slot < FRAG_RESULT_DATA0 && slot != FRAG_RESULT_COLOR)) {
                  }
      static bool
   lower_ps_load_sample_mask_in(nir_builder *b, nir_intrinsic_instr *intrin, lower_ps_state *s)
   {
      /* Section 15.2.2 (Shader Inputs) of the OpenGL 4.5 (Core Profile) spec
   * says:
   *
   *    "When per-sample shading is active due to the use of a fragment
   *     input qualified by sample or due to the use of the gl_SampleID
   *     or gl_SamplePosition variables, only the bit for the current
   *     sample is set in gl_SampleMaskIn. When state specifies multiple
   *     fragment shader invocations for a given fragment, the sample
   *     mask for any single fragment shader invocation may specify a
   *     subset of the covered samples for the fragment. In this case,
   *     the bit corresponding to each covered sample will be set in
   *     exactly one fragment shader invocation."
   *
   * The samplemask loaded by hardware is always the coverage of the
   * entire pixel/fragment, so mask bits out based on the sample ID.
                     uint32_t ps_iter_mask = ac_get_ps_iter_mask(s->options->ps_iter_samples);
   nir_def *sampleid = nir_load_sample_id(b);
            nir_def *sample_mask = nir_load_sample_mask_in(b);
                     nir_instr_remove(&intrin->instr);
      }
      static bool
   lower_ps_intrinsic(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     switch (intrin->intrinsic) {
   case nir_intrinsic_store_output:
         case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_sample:
      if (s->lower_load_barycentric)
            case nir_intrinsic_load_sample_mask_in:
      if (s->options->ps_iter_samples > 1)
            default:
                     }
      static void
   emit_ps_color_clamp_and_alpha_test(nir_builder *b, lower_ps_state *s)
   {
      uint32_t color_mask =
      BITFIELD_BIT(FRAG_RESULT_COLOR) |
      uint32_t color_outputs =
      (b->shader->info.outputs_written & color_mask) |
   /* both dual source blend outputs use FRAG_RESULT_DATA0 slot in nir,
   * but we use an extra slot number in lower_ps_state for the second
   * output
   */
         u_foreach_bit (slot, color_outputs) {
      if (s->options->clamp_color) {
      for (int i = 0; i < 4; i++) {
      if (s->outputs[slot][i])
                  if (s->options->alpha_to_one) {
      /* any one has written to this slot */
   if (s->output_types[slot] != nir_type_invalid) {
      unsigned bit_size = nir_alu_type_get_type_size(s->output_types[slot]);
                  if (slot == FRAG_RESULT_COLOR || slot == FRAG_RESULT_DATA0) {
      if (s->options->alpha_func == COMPARE_FUNC_ALWAYS) {
         } else if (s->options->alpha_func == COMPARE_FUNC_NEVER) {
         } else if (s->outputs[slot][3]) {
      nir_def *ref = nir_load_alpha_reference_amd(b);
   nir_def *cond =
                     }
      static void
   emit_ps_mrtz_export(nir_builder *b, lower_ps_state *s)
   {
               nir_def *mrtz_alpha = NULL;
   if (s->options->alpha_to_coverage_via_mrtz) {
      mrtz_alpha = s->outputs[FRAG_RESULT_COLOR][3] ?
      s->outputs[FRAG_RESULT_COLOR][3] :
            nir_def *depth = s->outputs[FRAG_RESULT_DEPTH][0];
   nir_def *stencil = s->outputs[FRAG_RESULT_STENCIL][0];
            if (s->options->kill_samplemask) {
      sample_mask = NULL;
               /* skip mrtz export if no one has written to any of them */
   if (!depth && !stencil && !sample_mask && !mrtz_alpha)
            /* use outputs_written to determine export format as we use it to set
   * R_028710_SPI_SHADER_Z_FORMAT instead of relying on the real store output,
   * because store output may be optimized out.
   */
   unsigned format =
      ac_get_spi_shader_z_format(outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH),
                     nir_def *undef = nir_undef(b, 1, 32);
   nir_def *outputs[4] = {undef, undef, undef, undef};
   unsigned write_mask = 0;
            if (format == V_028710_SPI_SHADER_UINT16_ABGR) {
               if (s->options->gfx_level < GFX11)
            if (stencil) {
      outputs[0] = nir_ishl_imm(b, stencil, 16);
               if (sample_mask) {
      outputs[1] = sample_mask;
         } else {
      if (depth) {
      outputs[0] = depth;
               if (stencil) {
      outputs[1] = stencil;
               if (sample_mask) {
      outputs[2] = sample_mask;
               if (mrtz_alpha) {
      outputs[3] = mrtz_alpha;
                  /* GFX6 (except OLAND and HAINAN) has a bug that it only looks at the
   * X writemask component.
   */
   if (s->options->gfx_level == GFX6 &&
      s->options->family != CHIP_OLAND &&
   s->options->family != CHIP_HAINAN) {
               s->exp[s->exp_num++] = nir_export_amd(b, nir_vec(b, outputs, 4),
                  }
      static unsigned
   get_ps_color_export_target(lower_ps_state *s)
   {
               if (s->options->dual_src_blend_swizzle && s->compacted_mrt_index < 2)
                        }
      static bool
   emit_ps_color_export(nir_builder *b, lower_ps_state *s, gl_frag_result slot, unsigned cbuf)
   {
               unsigned spi_shader_col_format = (s->options->spi_shader_col_format >> (cbuf * 4)) & 0xf;
   if (spi_shader_col_format == V_028714_SPI_SHADER_ZERO)
            /* get target after checking spi_shader_col_format as we need to increase
   * compacted_mrt_index anyway regardless of whether the export is built
   */
            nir_alu_type type = s->output_types[slot];
   /* no one has written to this slot */
   if (type == nir_type_invalid)
            bool is_int8 = s->options->color_is_int8 & BITFIELD_BIT(cbuf);
   bool is_int10 = s->options->color_is_int10 & BITFIELD_BIT(cbuf);
   bool enable_mrt_output_nan_fixup =
            nir_def *undef = nir_undef(b, 1, 32);
   nir_def *outputs[4] = {undef, undef, undef, undef};
   unsigned write_mask = 0;
            nir_alu_type base_type = nir_alu_type_get_base_type(type);
            nir_def *data[4];
            /* Replace NaN by zero (for 32-bit float formats) to fix game bugs if requested. */
   if (enable_mrt_output_nan_fixup && type == nir_type_float32) {
      for (int i = 0; i < 4; i++) {
      if (data[i]) {
      nir_def *isnan = nir_fisnan(b, data[i]);
                     switch (spi_shader_col_format) {
   case V_028714_SPI_SHADER_32_R:
      if (!data[0])
            outputs[0] = nir_convert_to_bit_size(b, data[0], base_type, 32);
   write_mask = 0x1;
         case V_028714_SPI_SHADER_32_GR:
      if (!data[0] && !data[1])
            if (data[0]) {
      outputs[0] = nir_convert_to_bit_size(b, data[0], base_type, 32);
               if (data[1]) {
      outputs[1] = nir_convert_to_bit_size(b, data[1], base_type, 32);
      }
         case V_028714_SPI_SHADER_32_AR:
      if (!data[0] && !data[3])
            if (data[0]) {
      outputs[0] = nir_convert_to_bit_size(b, data[0], base_type, 32);
               if (data[3]) {
      unsigned index = s->options->gfx_level >= GFX10 ? 1 : 3;
   outputs[index] = nir_convert_to_bit_size(b, data[3], base_type, 32);
      }
         case V_028714_SPI_SHADER_32_ABGR:
      for (int i = 0; i < 4; i++) {
      if (data[i]) {
      outputs[i] = nir_convert_to_bit_size(b, data[i], base_type, 32);
         }
         default: {
               switch (spi_shader_col_format) {
   case V_028714_SPI_SHADER_FP16_ABGR:
      if (type_size == 32)
            case V_028714_SPI_SHADER_UINT16_ABGR:
      if (type_size == 32) {
      pack_op = nir_op_pack_uint_2x16;
   if (is_int8 || is_int10) {
                                             uint32_t max_value = i == 3 && is_int10 ? 3 : max_rgb;
            }
      case V_028714_SPI_SHADER_SINT16_ABGR:
      if (type_size == 32) {
      pack_op = nir_op_pack_sint_2x16;
   if (is_int8 || is_int10) {
                                                                  data[i] = nir_imin(b, data[i], nir_imm_int(b, max_value));
            }
      case V_028714_SPI_SHADER_UNORM16_ABGR:
      pack_op = nir_op_pack_unorm_2x16;
      case V_028714_SPI_SHADER_SNORM16_ABGR:
      pack_op = nir_op_pack_snorm_2x16;
      default:
      unreachable("unsupported color export format");
               for (int i = 0; i < 2; i++) {
      nir_def *lo = data[i * 2];
   nir_def *hi = data[i * 2 + 1];
                  lo = lo ? lo : nir_undef(b, 1, type_size);
                           if (s->options->gfx_level >= GFX11)
         else
               if (s->options->gfx_level < GFX11)
      }
            s->exp[s->exp_num++] = nir_export_amd(b, nir_vec(b, outputs, 4),
                        }
      static void
   emit_ps_dual_src_blend_swizzle(nir_builder *b, lower_ps_state *s, unsigned first_color_export)
   {
               nir_intrinsic_instr *mrt0_exp = s->exp[first_color_export];
            /* There are some instructions which operate mrt1_exp's argument
   * between mrt0_exp and mrt1_exp. Move mrt0_exp next to mrt1_exp,
   * so that we can swizzle their arguments.
   */
   unsigned target0 = nir_intrinsic_base(mrt0_exp);
   unsigned target1 = nir_intrinsic_base(mrt1_exp);
   if (target0 > target1) {
      /* mrt0 export is after mrt1 export, this happens when src0 is missing,
   * so we emit mrt1 first then emit an empty mrt0.
   *
   * swap the pointer
   */
   nir_intrinsic_instr *tmp = mrt0_exp;
   mrt0_exp = mrt1_exp;
            /* move mrt1_exp down to after mrt0_exp */
      } else {
      /* move mrt0_exp down to before mrt1_exp */
               uint32_t mrt0_write_mask = nir_intrinsic_write_mask(mrt0_exp);
   uint32_t mrt1_write_mask = nir_intrinsic_write_mask(mrt1_exp);
            nir_def *mrt0_arg = mrt0_exp->src[0].ssa;
            /* Swizzle code is right before mrt0_exp. */
            /* ACO need to emit the swizzle code by a pseudo instruction. */
   if (s->options->use_aco) {
      nir_export_dual_src_blend_amd(b, mrt0_arg, mrt1_arg, .write_mask = write_mask);
   nir_instr_remove(&mrt0_exp->instr);
   nir_instr_remove(&mrt1_exp->instr);
               nir_def *undef = nir_undef(b, 1, 32);
   nir_def *arg0_vec[4] = {undef, undef, undef, undef};
            /* For illustration, originally
   *   lane0 export arg00 and arg01
   *   lane1 export arg10 and arg11.
   *
   * After the following operation
   *   lane0 export arg00 and arg10
   *   lane1 export arg01 and arg11.
   */
   u_foreach_bit (i, write_mask) {
      nir_def *arg0 = nir_channel(b, mrt0_arg, i);
            /* swap odd,even lanes of arg0 */
            /* swap even lanes between arg0 and arg1 */
   nir_def *tid = nir_load_subgroup_invocation(b);
            nir_def *tmp = arg0;
   arg0 = nir_bcsel(b, is_even, arg1, arg0);
            /* swap odd,even lanes again for arg0 */
            arg0_vec[i] = arg0;
               nir_src_rewrite(&mrt0_exp->src[0], nir_vec(b, arg0_vec, 4));
            nir_intrinsic_set_write_mask(mrt0_exp, write_mask);
      }
      static void
   emit_ps_null_export(nir_builder *b, lower_ps_state *s)
   {
      const bool pops = b->shader->info.fs.sample_interlock_ordered ||
                        /* Gfx10+ doesn't need to export anything if we don't need to export the EXEC mask
   * for discard.
   * In Primitive Ordered Pixel Shading, however, GFX11+ explicitly uses the `done` export to exit
   * the ordered section, and before GFX11, shaders with POPS also need an export.
   */
   if (s->options->gfx_level >= GFX10 && !s->options->uses_discard && !pops)
            /* The `done` export exits the POPS ordered section on GFX11+, make sure UniformMemory and
   * ImageMemory (in SPIR-V terms) accesses from the ordered section may not be reordered below it.
   */
   if (s->options->gfx_level >= GFX11 && pops)
      nir_scoped_memory_barrier(b, SCOPE_QUEUE_FAMILY, NIR_MEMORY_RELEASE,
               /* Gfx11 doesn't support null exports, and mrt0 should be exported instead. */
   unsigned target = s->options->gfx_level >= GFX11 ?
            nir_intrinsic_instr *intrin =
      nir_export_amd(b, nir_undef(b, 4, 32),
            /* To avoid builder set write mask to 0xf. */
      }
      static void
   export_ps_outputs(nir_builder *b, lower_ps_state *s)
   {
                                 /* When non-monolithic shader, RADV export mrtz in main part and export color in epilog. */
   if (s->options->no_color_export)
                     /* When dual src blend is enabled and we need both src0 and src1
   * export present, try to export both src, and add an empty export
   * for either src missing.
   */
   if (s->output_types[DUAL_SRC_BLEND_SLOT] != nir_type_invalid ||
      s->options->dual_src_blend_swizzle) {
   unsigned slot;
   if (s->output_types[FRAG_RESULT_COLOR] != nir_type_invalid) {
      /* when dual source blending, there must be only one color buffer */
   assert(s->options->broadcast_last_cbuf == 0);
      } else {
                  bool src0_exported = emit_ps_color_export(b, s, slot, 0);
   /* src1 use cubf1 info, when dual src blend is enabled it's
   * same as cbuf0, but when dual src blend is disabled it's used
   * to disable src1 export.
   */
            bool need_empty_export =
      /* miss src1, need to add src1 only when swizzle case */
   (src0_exported && !src1_exported && s->options->dual_src_blend_swizzle) ||
               if (need_empty_export) {
                              s->exp[s->exp_num++] =
         } else {
      if (s->output_types[FRAG_RESULT_COLOR] != nir_type_invalid) {
      /* write to all color buffers */
   for (int cbuf = 0; cbuf <= s->options->broadcast_last_cbuf; cbuf++)
      } else {
      for (int cbuf = 0; cbuf < MAX_DRAW_BUFFERS; cbuf++) {
      unsigned slot = FRAG_RESULT_DATA0 + cbuf;
                     if (s->exp_num) {
      if (s->options->dual_src_blend_swizzle) {
      emit_ps_dual_src_blend_swizzle(b, s, first_color_export);
   /* Skip last export flag setting because they have been replaced by
   * a pseudo instruction.
   */
   if (s->options->use_aco)
               /* Specify that this is the last export */
   nir_intrinsic_instr *final_exp = s->exp[s->exp_num - 1];
   unsigned final_exp_flags = nir_intrinsic_flags(final_exp);
   final_exp_flags |= AC_EXP_FLAG_DONE | AC_EXP_FLAG_VALID_MASK;
            /* The `done` export exits the POPS ordered section on GFX11+, make sure UniformMemory and
   * ImageMemory (in SPIR-V terms) accesses from the ordered section may not be reordered below
   * it.
   */
   if (s->options->gfx_level >= GFX11 &&
      (b->shader->info.fs.sample_interlock_ordered ||
   b->shader->info.fs.sample_interlock_unordered ||
   b->shader->info.fs.pixel_interlock_ordered ||
   b->shader->info.fs.pixel_interlock_unordered)) {
   b->cursor = nir_before_instr(&final_exp->instr);
   nir_scoped_memory_barrier(b, SCOPE_QUEUE_FAMILY, NIR_MEMORY_RELEASE,
               } else {
            }
      void
   ac_nir_lower_ps(nir_shader *nir, const ac_nir_lower_ps_options *options)
   {
               nir_builder builder = nir_builder_create(impl);
            lower_ps_state state = {
                           nir_shader_instructions_pass(nir, lower_ps_intrinsic,
                  /* Must be after lower_ps_intrinsic() to prevent it lower added intrinsic here. */
                     /* Cleanup nir variable, as RADV won't do this. */
   if (state.lower_load_barycentric)
      }
