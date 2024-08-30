   /*
   * Copyright 2023 Advanced Micro Devices, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * on the rights to use, copy, modify, merge, publish, distribute, sub
   * license, and/or sell copies of the Software, and to permit persons to whom
   * the Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
   * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
   * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
   * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   * USE OR OTHER DEALINGS IN THE SOFTWARE.
   */
      #include "si_shader_internal.h"
   #include "si_pipe.h"
   #include "ac_hw_stage.h"
   #include "aco_interface.h"
      static void
   si_aco_compiler_debug(void *private_data, enum aco_compiler_debug_level level,
         {
                  }
      static void
   si_fill_aco_options(struct si_screen *screen, gl_shader_stage stage,
               {
      options->dump_shader =
      si_can_dump_shader(screen, stage, SI_DUMP_ACO_IR) ||
      options->dump_preoptir = si_can_dump_shader(screen, stage, SI_DUMP_INIT_ACO_IR);
   options->record_ir = screen->record_llvm_ir;
            options->has_ls_vgpr_init_bug = screen->info.has_ls_vgpr_init_bug;
   options->load_grid_size_from_user_sgpr = true;
   options->family = screen->info.family;
   options->gfx_level = screen->info.gfx_level;
            options->debug.func = si_aco_compiler_debug;
      }
      static void
   si_fill_aco_shader_info(struct si_shader *shader, struct aco_shader_info *info,
         {
      const struct si_shader_selector *sel = shader->selector;
   const union si_shader_key *key = &shader->key;
   const enum amd_gfx_level gfx_level = sel->screen->info.gfx_level;
            info->wave_size = shader->wave_size;
   info->workgroup_size = si_get_max_workgroup_size(shader);
   /* aco need non-zero value */
   if (!info->workgroup_size)
            info->image_2d_view_of_3d = gfx_level == GFX9;
            if (stage <= MESA_SHADER_GEOMETRY && key->ge.as_ngg && !key->ge.as_es) {
      info->has_ngg_culling = key->ge.opt.ngg_culling;
               switch (stage) {
   case MESA_SHADER_VERTEX:
      /* Only part mode VS may have prolog, mono mode VS will embed prolog in nir.
   * But we don't know exactly if part mode VS needs prolog because it also depends
   * on shader select key ls_vgpr_fix which is not known when VS main part compile.
   * Now just assume ls_vgpr_fix is always false, which just cause ACO to add extra
   * s_setprio and exec init code when it's finally combined with prolog.
   */
   if (!shader->is_gs_copy_shader && !shader->is_monolithic)
            case MESA_SHADER_TESS_CTRL:
      info->vs.tcs_in_out_eq = key->ge.opt.same_patch_vertices;
   info->vs.tcs_temp_only_input_mask = sel->info.tcs_vgpr_only_inputs;
   info->has_epilog = !shader->is_monolithic;
   info->tcs.pass_tessfactors_by_reg = sel->info.tessfactors_are_def_in_all_invocs;
   info->tcs.patch_stride = si_get_tcs_out_patch_stride(&sel->info);
   info->tcs.tcs_offchip_layout = args->tcs_offchip_layout;
   info->tcs.tes_offchip_addr = args->tes_offchip_addr;
   info->tcs.vs_state_bits = args->vs_state_bits;
      case MESA_SHADER_FRAGMENT:
      info->ps.num_interp = si_get_ps_num_interp(shader);
   info->ps.spi_ps_input_ena = shader->config.spi_ps_input_ena;
   info->ps.spi_ps_input_addr = shader->config.spi_ps_input_addr;
   info->ps.alpha_reference = args->alpha_reference;
   info->has_epilog = !shader->is_monolithic;
      default:
            }
      static void
   si_aco_build_shader_binary(void **data, const struct ac_shader_config *config,
                           {
               unsigned code_size = code_dw * 4;
   char *buffer = MALLOC(code_size + disasm_size);
            shader->binary.type = SI_SHADER_BINARY_RAW;
   shader->binary.code_buffer = buffer;
   shader->binary.code_size = code_size;
            if (disasm_size) {
      memcpy(buffer + code_size, disasm_str, disasm_size);
   shader->binary.disasm_string = buffer + code_size;
               if (llvm_ir_size) {
      shader->binary.llvm_ir_string = MALLOC(llvm_ir_size);
               if (num_symbols) {
      unsigned symbol_size = num_symbols * sizeof(*symbols);
   void *data = MALLOC(symbol_size);
   memcpy(data, symbols, symbol_size);
   shader->binary.symbols = data;
                  }
      bool
   si_aco_compile_shader(struct si_shader *shader,
                     {
               struct aco_compiler_options options = {0};
            struct aco_shader_info info = {0};
            nir_shader *shaders[2];
            bool free_nir = false;
   struct si_shader prev_shader = {};
            /* For merged shader stage. */
   if (shader->is_monolithic && sel->screen->info.gfx_level >= GFX9 &&
               shaders[num_shaders++] =
                                 aco_compile_shader(&options, &info, num_shaders, shaders, &args->ac,
            if (free_nir)
               }
      void
   si_aco_resolve_symbols(struct si_shader *shader, uint32_t *code_for_write,
         {
      const struct aco_symbol *symbols = (struct aco_symbol *)shader->binary.symbols;
   const struct si_shader_selector *sel = shader->selector;
            for (int i = 0; i < shader->binary.num_symbols; i++) {
               switch (symbols[i].id) {
   case aco_symbol_scratch_addr_lo:
      value = scratch_va;
      case aco_symbol_scratch_addr_hi:
               if (sel->screen->info.gfx_level >= GFX11)
         else
            case aco_symbol_lds_ngg_scratch_base:
      assert(sel->stage <= MESA_SHADER_GEOMETRY && key->ge.as_ngg);
   value = shader->gs_info.esgs_ring_size * 4;
   if (sel->stage == MESA_SHADER_GEOMETRY)
         value = ALIGN(value, 8);
      case aco_symbol_lds_ngg_gs_out_vertex_base:
      assert(sel->stage == MESA_SHADER_GEOMETRY && key->ge.as_ngg);
   value = shader->gs_info.esgs_ring_size * 4;
      case aco_symbol_const_data_addr:
      if (!const_offset)
         value = code_for_read[symbols[i].offset] + const_offset;
      default:
      unreachable("invalid aco symbol");
                     }
      static void
   si_aco_build_shader_part_binary(void** priv_ptr, uint32_t num_sgprs, uint32_t num_vgprs,
               {
      struct si_shader_part *result = (struct si_shader_part *)priv_ptr;
            char *buffer = MALLOC(code_size + disasm_size);
            result->binary.type = SI_SHADER_BINARY_RAW;
   result->binary.code_buffer = buffer;
   result->binary.code_size = code_size;
            if (disasm_size) {
      memcpy(buffer + code_size, disasm_str, disasm_size);
   result->binary.disasm_string = buffer + code_size;
               result->config.num_sgprs = num_sgprs;
      }
      static bool
   si_aco_build_tcs_epilog(struct si_screen *screen,
               {
               struct si_shader_args args;
   struct ac_arg rel_patch_id;
   struct ac_arg invocation_id;
   struct ac_arg tcs_out_current_patch_data_offset;
   struct ac_arg tess_factors[6];
   si_get_tcs_epilog_args(screen->info.gfx_level, &args, &rel_patch_id, &invocation_id,
            struct aco_tcs_epilog_info einfo = {
      .pass_tessfactors_by_reg = key->tcs_epilog.states.invoc0_tess_factors_are_def,
   .tcs_out_patch_fits_subgroup = key->tcs_epilog.noop_s_barrier,
   .primitive_mode = key->tcs_epilog.states.prim_mode,
   .tess_offchip_ring_size = screen->hs.tess_offchip_ring_size,
            .rel_patch_id = rel_patch_id,
   .invocation_id = invocation_id,
   .tcs_out_current_patch_data_offset = tcs_out_current_patch_data_offset,
   .tcs_out_lds_layout = args.tes_offchip_addr,
      };
   memcpy(einfo.tess_lvl_out, tess_factors, sizeof(einfo.tess_lvl_out));
            struct aco_shader_info info = {0};
   info.hw_stage = AC_HW_HULL_SHADER;
   info.wave_size = key->tcs_epilog.wave32 ? 32 : 64;
   /* Set to >wave_size to keep p_barrier work. GFX6 has single wave for HS. */
            aco_compile_tcs_epilog(options, &info, &einfo, &args.ac,
            }
      static bool
   si_aco_build_vs_prolog(struct si_screen *screen,
               {
               struct si_shader_args args;
            struct aco_gl_vs_prolog_info pinfo = {
      .instance_divisor_is_one = key->vs_prolog.states.instance_divisor_is_one,
   .instance_divisor_is_fetched = key->vs_prolog.states.instance_divisor_is_fetched,
   .instance_diviser_buf_offset = SI_VS_CONST_INSTANCE_DIVISORS * 16,
   .num_inputs = key->vs_prolog.num_inputs,
                        struct aco_shader_info info = {0};
            if (key->vs_prolog.as_ngg)
         else if (key->vs_prolog.as_es)
         else if (key->vs_prolog.as_ls)
         else
            aco_compile_gl_vs_prolog(options, &info, &pinfo, &args.ac,
            }
      static bool
   si_aco_build_ps_prolog(struct aco_compiler_options *options,
         {
               struct si_shader_args args;
            struct aco_ps_prolog_info pinfo = {
      .poly_stipple = key->ps_prolog.states.poly_stipple,
            .bc_optimize_for_persp = key->ps_prolog.states.bc_optimize_for_persp,
   .bc_optimize_for_linear = key->ps_prolog.states.bc_optimize_for_linear,
   .force_persp_sample_interp = key->ps_prolog.states.force_persp_sample_interp,
   .force_linear_sample_interp = key->ps_prolog.states.force_linear_sample_interp,
   .force_persp_center_interp = key->ps_prolog.states.force_persp_center_interp,
            .samplemask_log_ps_iter = key->ps_prolog.states.samplemask_log_ps_iter,
   .num_interp_inputs = key->ps_prolog.num_interp_inputs,
   .colors_read = key->ps_prolog.colors_read,
   .color_interp_vgpr_index[0] = key->ps_prolog.color_interp_vgpr_index[0],
   .color_interp_vgpr_index[1] = key->ps_prolog.color_interp_vgpr_index[1],
   .color_attr_index[0] = key->ps_prolog.color_attr_index[0],
   .color_attr_index[1] = key->ps_prolog.color_attr_index[1],
   .color_two_side = key->ps_prolog.states.color_two_side,
                        struct aco_shader_info info = {0};
   info.hw_stage = AC_HW_PIXEL_SHADER;
            aco_compile_ps_prolog(options, &info, &pinfo, &args.ac,
            }
      static bool
   si_aco_build_ps_epilog(struct aco_compiler_options *options,
         {
               struct aco_ps_epilog_info pinfo = {
      .spi_shader_col_format = key->ps_epilog.states.spi_shader_col_format,
   .color_is_int8 = key->ps_epilog.states.color_is_int8,
   .color_is_int10 = key->ps_epilog.states.color_is_int10,
   .mrt0_is_dual_src = key->ps_epilog.states.dual_src_blend_swizzle,
   .color_types = key->ps_epilog.color_types,
   .clamp_color = key->ps_epilog.states.clamp_color,
   .alpha_to_one = key->ps_epilog.states.alpha_to_one,
   .alpha_to_coverage_via_mrtz = key->ps_epilog.states.alpha_to_coverage_via_mrtz,
   .skip_null_export = options->gfx_level >= GFX10 && !key->ps_epilog.uses_discard,
   .broadcast_last_cbuf = key->ps_epilog.states.last_cbuf,
               struct si_shader_args args;
   si_get_ps_epilog_args(&args, key, pinfo.colors, &pinfo.depth, &pinfo.stencil,
                  struct aco_shader_info info = {0};
   info.hw_stage = AC_HW_PIXEL_SHADER;
            aco_compile_ps_epilog(options, &info, &pinfo, &args.ac,
            }
      bool
   si_aco_build_shader_part(struct si_screen *screen, gl_shader_stage stage, bool prolog,
               {
      struct aco_compiler_options options = {0};
            switch (stage) {
   case MESA_SHADER_VERTEX:
         case MESA_SHADER_TESS_CTRL:
      return si_aco_build_tcs_epilog(screen, &options, result);
      case MESA_SHADER_FRAGMENT:
      if (prolog)
         else
      default:
                     }
