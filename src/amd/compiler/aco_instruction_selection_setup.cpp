   /*
   * Copyright © 2018 Valve Corporation
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
   *
   */
      #include "aco_instruction_selection.h"
      #include "common/ac_nir.h"
   #include "common/sid.h"
      #include "nir_control_flow.h"
      #include <vector>
      namespace aco {
      namespace {
      bool
   is_loop_header_block(nir_block* block)
   {
      return block->cf_node.parent->type == nir_cf_node_loop &&
      }
      /* similar to nir_block_is_unreachable(), but does not require dominance information */
   bool
   is_block_reachable(nir_function_impl* impl, nir_block* known_reachable, nir_block* block)
   {
      if (block == nir_start_block(impl) || block == known_reachable)
            /* skip loop back-edges */
   if (is_loop_header_block(block)) {
      nir_loop* loop = nir_cf_node_as_loop(block->cf_node.parent);
   nir_block* preheader = nir_block_cf_tree_prev(nir_loop_first_block(loop));
               set_foreach (block->predecessors, entry) {
      if (is_block_reachable(impl, known_reachable, (nir_block*)entry->key))
                  }
      /* Check whether the given SSA def is only used by cross-lane instructions. */
   bool
   only_used_by_cross_lane_instrs(nir_def* ssa, bool follow_phis = true)
   {
      nir_foreach_use (src, ssa) {
      switch (nir_src_parent_instr(src)->type) {
   case nir_instr_type_alu: {
      nir_alu_instr* alu = nir_instr_as_alu(nir_src_parent_instr(src));
   if (alu->op != nir_op_unpack_64_2x32_split_x && alu->op != nir_op_unpack_64_2x32_split_y)
                           }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr* intrin = nir_instr_as_intrinsic(nir_src_parent_instr(src));
   if (intrin->intrinsic != nir_intrinsic_read_invocation &&
      intrin->intrinsic != nir_intrinsic_read_first_invocation &&
                  }
   case nir_instr_type_phi: {
      /* Don't follow more than 1 phis, this avoids infinite loops. */
                  nir_phi_instr* phi = nir_instr_as_phi(nir_src_parent_instr(src));
                     }
   default: return false;
                  }
      /* If one side of a divergent IF ends in a branch and the other doesn't, we
   * might have to emit the contents of the side without the branch at the merge
   * block instead. This is so that we can use any SGPR live-out of the side
   * without the branch without creating a linear phi in the invert or merge block. */
   bool
   sanitize_if(nir_function_impl* impl, nir_if* nif)
   {
               nir_block* then_block = nir_if_last_then_block(nif);
   nir_block* else_block = nir_if_last_else_block(nif);
   bool then_jump = nir_block_ends_in_jump(then_block) ||
         bool else_jump = nir_block_ends_in_jump(else_block) ||
         if (then_jump == else_jump)
            /* If the continue from block is empty then return as there is nothing to
   * move.
   */
   if (nir_cf_list_is_empty_block(else_jump ? &nif->then_list : &nif->else_list))
            /* Even though this if statement has a jump on one side, we may still have
   * phis afterwards.  Single-source phis can be produced by loop unrolling
   * or dead control-flow passes and are perfectly legal.  Run a quick phi
   * removal on the block after the if to clean up any such phis.
   */
            /* Finally, move the continue from branch after the if-statement. */
   nir_block* last_continue_from_blk = else_jump ? then_block : else_block;
   nir_block* first_continue_from_blk =
            nir_cf_list tmp;
   nir_cf_extract(&tmp, nir_before_block(first_continue_from_blk),
                     }
      bool
   sanitize_cf_list(nir_function_impl* impl, struct exec_list* cf_list)
   {
      bool progress = false;
   foreach_list_typed (nir_cf_node, cf_node, node, cf_list) {
      switch (cf_node->type) {
   case nir_cf_node_block: break;
   case nir_cf_node_if: {
      nir_if* nif = nir_cf_node_as_if(cf_node);
   progress |= sanitize_cf_list(impl, &nif->then_list);
   progress |= sanitize_cf_list(impl, &nif->else_list);
   progress |= sanitize_if(impl, nif);
      }
   case nir_cf_node_loop: {
      nir_loop* loop = nir_cf_node_as_loop(cf_node);
   assert(!nir_loop_has_continue_construct(loop));
   progress |= sanitize_cf_list(impl, &loop->body);
      }
   case nir_cf_node_function: unreachable("Invalid cf type");
                  }
      void
   apply_nuw_to_ssa(isel_context* ctx, nir_def* ssa)
   {
      nir_scalar scalar;
   scalar.def = ssa;
            if (!nir_scalar_is_alu(scalar) || nir_scalar_alu_op(scalar) != nir_op_iadd)
                     if (add->no_unsigned_wrap)
            nir_scalar src0 = nir_scalar_chase_alu_src(scalar, 0);
            if (nir_scalar_is_const(src0)) {
      nir_scalar tmp = src0;
   src0 = src1;
               uint32_t src1_ub = nir_unsigned_upper_bound(ctx->shader, ctx->range_ht, src1, &ctx->ub_config);
   add->no_unsigned_wrap =
      }
      void
   apply_nuw_to_offsets(isel_context* ctx, nir_function_impl* impl)
   {
      nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
      if (instr->type != nir_instr_type_intrinsic)
                  switch (intrin->intrinsic) {
   case nir_intrinsic_load_constant:
   case nir_intrinsic_load_uniform:
   case nir_intrinsic_load_push_constant:
      if (!nir_src_is_divergent(intrin->src[0]))
            case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
      if (!nir_src_is_divergent(intrin->src[1]))
            case nir_intrinsic_store_ssbo:
      if (!nir_src_is_divergent(intrin->src[2]))
            case nir_intrinsic_load_scratch: apply_nuw_to_ssa(ctx, intrin->src[0].ssa); break;
   case nir_intrinsic_store_scratch:
   case nir_intrinsic_load_smem_amd: apply_nuw_to_ssa(ctx, intrin->src[1].ssa); break;
   default: break;
            }
      RegClass
   get_reg_class(isel_context* ctx, RegType type, unsigned components, unsigned bitsize)
   {
      if (bitsize == 1)
         else
      }
      void
   setup_tcs_info(isel_context* ctx)
   {
      ctx->tcs_in_out_eq = ctx->program->info.vs.tcs_in_out_eq;
      }
      void
   setup_lds_size(isel_context* ctx, nir_shader* nir)
   {
      /* TCS and GFX9 GS are special cases, already in units of the allocation granule. */
   if (ctx->stage.has(SWStage::TCS))
         else if (ctx->stage.hw == AC_HW_LEGACY_GEOMETRY_SHADER && ctx->options->gfx_level >= GFX9)
         else
      ctx->program->config->lds_size =
         /* Make sure we fit the available LDS space. */
   assert((ctx->program->config->lds_size * ctx->program->dev.lds_encoding_granule) <=
      }
      void
   setup_nir(isel_context* ctx, nir_shader* nir)
   {
      nir_convert_to_lcssa(nir, true, false);
            nir_function_impl* func = nir_shader_get_entrypoint(nir);
      }
      } /* end namespace */
      void
   init_context(isel_context* ctx, nir_shader* shader)
   {
      nir_function_impl* impl = nir_shader_get_entrypoint(shader);
            /* Init NIR range analysis. */
   ctx->range_ht = _mesa_pointer_hash_table_create(NULL);
   ctx->ub_config.min_subgroup_size = 64;
   ctx->ub_config.max_subgroup_size = 64;
   if (ctx->shader->info.stage == MESA_SHADER_COMPUTE && ctx->program->info.cs.subgroup_size) {
      ctx->ub_config.min_subgroup_size = ctx->program->info.cs.subgroup_size;
      }
   ctx->ub_config.max_workgroup_invocations = 2048;
   ctx->ub_config.max_workgroup_count[0] = 65535;
   ctx->ub_config.max_workgroup_count[1] = 65535;
   ctx->ub_config.max_workgroup_count[2] = 65535;
   ctx->ub_config.max_workgroup_size[0] = 2048;
   ctx->ub_config.max_workgroup_size[1] = 2048;
            nir_divergence_analysis(shader);
   if (nir_opt_uniform_atomics(shader) && nir_lower_int64(shader))
                     /* sanitize control flow */
   sanitize_cf_list(impl, &impl->body);
            /* we'll need these for isel */
            if (ctx->options->dump_preoptir) {
      fprintf(stderr, "NIR shader before instruction selection:\n");
               ctx->first_temp_id = ctx->program->peekAllocationId();
   ctx->program->allocateRange(impl->ssa_alloc);
                     /* TODO: make this recursive to improve compile times */
   bool done = false;
   while (!done) {
      done = true;
   nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block) {
      switch (instr->type) {
   case nir_instr_type_alu: {
      nir_alu_instr* alu_instr = nir_instr_as_alu(instr);
   RegType type = alu_instr->def.divergent ? RegType::vgpr : RegType::sgpr;
   switch (alu_instr->op) {
   case nir_op_fmul:
   case nir_op_fmulz:
   case nir_op_fadd:
   case nir_op_fsub:
   case nir_op_ffma:
   case nir_op_ffmaz:
   case nir_op_fmax:
   case nir_op_fmin:
   case nir_op_fneg:
   case nir_op_fabs:
   case nir_op_fsat:
   case nir_op_fsign:
   case nir_op_frcp:
   case nir_op_frsq:
   case nir_op_fsqrt:
   case nir_op_fexp2:
   case nir_op_flog2:
   case nir_op_ffract:
   case nir_op_ffloor:
   case nir_op_fceil:
   case nir_op_ftrunc:
   case nir_op_fround_even:
   case nir_op_fsin_amd:
   case nir_op_fcos_amd:
   case nir_op_f2f16:
   case nir_op_f2f16_rtz:
   case nir_op_f2f16_rtne:
   case nir_op_f2f32:
   case nir_op_f2f64:
   case nir_op_u2f16:
   case nir_op_u2f32:
   case nir_op_u2f64:
   case nir_op_i2f16:
   case nir_op_i2f32:
   case nir_op_i2f64:
   case nir_op_pack_half_2x16_rtz_split:
   case nir_op_pack_half_2x16_split:
   case nir_op_pack_unorm_2x16:
   case nir_op_pack_snorm_2x16:
   case nir_op_pack_uint_2x16:
   case nir_op_pack_sint_2x16:
   case nir_op_unpack_half_2x16_split_x:
   case nir_op_unpack_half_2x16_split_y:
   case nir_op_fddx:
   case nir_op_fddy:
   case nir_op_fddx_fine:
   case nir_op_fddy_fine:
   case nir_op_fddx_coarse:
   case nir_op_fddy_coarse:
   case nir_op_fquantize2f16:
   case nir_op_ldexp:
   case nir_op_frexp_sig:
   case nir_op_frexp_exp:
   case nir_op_cube_amd:
   case nir_op_sad_u8x4:
   case nir_op_udot_4x8_uadd:
   case nir_op_sdot_4x8_iadd:
   case nir_op_sudot_4x8_iadd:
   case nir_op_udot_4x8_uadd_sat:
   case nir_op_sdot_4x8_iadd_sat:
   case nir_op_sudot_4x8_iadd_sat:
   case nir_op_udot_2x16_uadd:
   case nir_op_sdot_2x16_iadd:
   case nir_op_udot_2x16_uadd_sat:
   case nir_op_sdot_2x16_iadd_sat: type = RegType::vgpr; break;
   case nir_op_f2i16:
   case nir_op_f2u16:
   case nir_op_f2i32:
   case nir_op_f2u32:
   case nir_op_b2i8:
   case nir_op_b2i16:
   case nir_op_b2i32:
   case nir_op_b2b32:
   case nir_op_b2f16:
   case nir_op_b2f32:
   case nir_op_mov: break;
   case nir_op_iabs:
   case nir_op_iadd:
   case nir_op_iadd_sat:
   case nir_op_uadd_sat:
   case nir_op_isub:
   case nir_op_isub_sat:
   case nir_op_usub_sat:
   case nir_op_imul:
   case nir_op_imin:
   case nir_op_imax:
   case nir_op_umin:
   case nir_op_umax:
   case nir_op_ishl:
   case nir_op_ishr:
   case nir_op_ushr:
      /* packed 16bit instructions have to be VGPR */
   type = alu_instr->def.num_components == 2 ? RegType::vgpr : type;
      default:
      for (unsigned i = 0; i < nir_op_infos[alu_instr->op].num_inputs; i++) {
      if (regclasses[alu_instr->src[i].src.ssa->index].type() == RegType::vgpr)
                        RegClass rc =
         regclasses[alu_instr->def.index] = rc;
      }
   case nir_instr_type_load_const: {
      unsigned num_components = nir_instr_as_load_const(instr)->def.num_components;
   unsigned bit_size = nir_instr_as_load_const(instr)->def.bit_size;
   RegClass rc = get_reg_class(ctx, RegType::sgpr, num_components, bit_size);
   regclasses[nir_instr_as_load_const(instr)->def.index] = rc;
      }
   case nir_instr_type_intrinsic: {
      nir_intrinsic_instr* intrinsic = nir_instr_as_intrinsic(instr);
   if (!nir_intrinsic_infos[intrinsic->intrinsic].has_dest)
         if (intrinsic->intrinsic == nir_intrinsic_strict_wqm_coord_amd) {
      regclasses[intrinsic->def.index] =
      RegClass::get(RegType::vgpr, intrinsic->def.num_components * 4 +
               }
   RegType type = RegType::sgpr;
   switch (intrinsic->intrinsic) {
   case nir_intrinsic_load_push_constant:
   case nir_intrinsic_load_workgroup_id:
   case nir_intrinsic_load_num_workgroups:
   case nir_intrinsic_load_ray_launch_size:
   case nir_intrinsic_load_ray_launch_size_addr_amd:
   case nir_intrinsic_load_sbt_base_amd:
   case nir_intrinsic_load_subgroup_id:
   case nir_intrinsic_load_num_subgroups:
   case nir_intrinsic_load_first_vertex:
   case nir_intrinsic_load_base_instance:
   case nir_intrinsic_vote_all:
   case nir_intrinsic_vote_any:
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_first_invocation:
   case nir_intrinsic_ballot:
   case nir_intrinsic_bindless_image_samples:
   case nir_intrinsic_load_force_vrs_rates_amd:
   case nir_intrinsic_load_scalar_arg_amd:
   case nir_intrinsic_load_lds_ngg_scratch_base_amd:
   case nir_intrinsic_load_lds_ngg_gs_out_vertex_base_amd:
   case nir_intrinsic_load_smem_amd: type = RegType::sgpr; break;
   case nir_intrinsic_load_sample_id:
   case nir_intrinsic_load_input:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_vertex_id_zero_base:
   case nir_intrinsic_load_barycentric_sample:
   case nir_intrinsic_load_barycentric_pixel:
   case nir_intrinsic_load_barycentric_model:
   case nir_intrinsic_load_barycentric_centroid:
   case nir_intrinsic_load_barycentric_at_offset:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_frag_coord:
   case nir_intrinsic_load_frag_shading_rate:
   case nir_intrinsic_load_sample_pos:
   case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_local_invocation_index:
   case nir_intrinsic_load_subgroup_invocation:
   case nir_intrinsic_load_ray_launch_id:
   case nir_intrinsic_load_tess_coord:
   case nir_intrinsic_write_invocation_amd:
   case nir_intrinsic_mbcnt_amd:
   case nir_intrinsic_lane_permute_16_amd:
   case nir_intrinsic_load_instance_id:
   case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
   case nir_intrinsic_global_atomic_amd:
   case nir_intrinsic_global_atomic_swap_amd:
   case nir_intrinsic_bindless_image_atomic:
   case nir_intrinsic_bindless_image_atomic_swap:
   case nir_intrinsic_bindless_image_size:
   case nir_intrinsic_shared_atomic:
   case nir_intrinsic_shared_atomic_swap:
   case nir_intrinsic_load_scratch:
   case nir_intrinsic_load_invocation_id:
   case nir_intrinsic_load_primitive_id:
   case nir_intrinsic_load_typed_buffer_amd:
   case nir_intrinsic_load_buffer_amd:
   case nir_intrinsic_load_initial_edgeflags_amd:
   case nir_intrinsic_gds_atomic_add_amd:
   case nir_intrinsic_bvh64_intersect_ray_amd:
   case nir_intrinsic_load_vector_arg_amd:
   case nir_intrinsic_load_rt_dynamic_callable_stack_base_amd:
   case nir_intrinsic_ordered_xfb_counter_add_amd:
   case nir_intrinsic_cmat_muladd_amd: type = RegType::vgpr; break;
   case nir_intrinsic_load_shared:
   case nir_intrinsic_load_shared2_amd:
      /* When the result of these loads is only used by cross-lane instructions,
   * it is beneficial to use a VGPR destination. This is because this allows
   * to put the s_waitcnt further down, which decreases latency.
   */
   if (only_used_by_cross_lane_instrs(&intrinsic->def)) {
      type = RegType::vgpr;
      }
      case nir_intrinsic_shuffle:
   case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_quad_swizzle_amd:
   case nir_intrinsic_masked_swizzle_amd:
   case nir_intrinsic_inclusive_scan:
   case nir_intrinsic_exclusive_scan:
   case nir_intrinsic_reduce:
   case nir_intrinsic_load_ubo:
   case nir_intrinsic_load_ssbo:
   case nir_intrinsic_load_global_amd:
      type = intrinsic->def.divergent ? RegType::vgpr : RegType::sgpr;
      case nir_intrinsic_load_view_index:
      type = ctx->stage == fragment_fs ? RegType::vgpr : RegType::sgpr;
      default:
      for (unsigned i = 0; i < nir_intrinsic_infos[intrinsic->intrinsic].num_srcs;
      i++) {
   if (regclasses[intrinsic->src[i].ssa->index].type() == RegType::vgpr)
      }
      }
   RegClass rc =
         regclasses[intrinsic->def.index] = rc;
      }
   case nir_instr_type_tex: {
                                          RegClass rc = get_reg_class(ctx, type, tex->def.num_components, tex->def.bit_size);
   regclasses[tex->def.index] = rc;
      }
   case nir_instr_type_undef: {
      unsigned num_components = nir_instr_as_undef(instr)->def.num_components;
   unsigned bit_size = nir_instr_as_undef(instr)->def.bit_size;
   RegClass rc = get_reg_class(ctx, RegType::sgpr, num_components, bit_size);
   regclasses[nir_instr_as_undef(instr)->def.index] = rc;
      }
   case nir_instr_type_phi: {
      nir_phi_instr* phi = nir_instr_as_phi(instr);
   RegType type = RegType::sgpr;
                        if (phi->def.divergent) {
         } else {
      nir_foreach_phi_src (src, phi) {
                           RegClass rc = get_reg_class(ctx, type, num_components, phi->def.bit_size);
   if (rc != regclasses[phi->def.index])
         regclasses[phi->def.index] = rc;
      }
   default: break;
                     ctx->program->config->spi_ps_input_ena = ctx->program->info.ps.spi_ps_input_ena;
                     /* align and copy constant data */
   while (ctx->program->constant_data.size() % 4u)
         ctx->constant_data_offset = ctx->program->constant_data.size();
   ctx->program->constant_data.insert(ctx->program->constant_data.end(),
            }
      void
   cleanup_context(isel_context* ctx)
   {
         }
      isel_context
   setup_isel_context(Program* program, unsigned shader_count, struct nir_shader* const* shaders,
                     {
      for (unsigned i = 0; i < shader_count; i++) {
      switch (shaders[i]->info.stage) {
   case MESA_SHADER_VERTEX: sw_stage = sw_stage | SWStage::VS; break;
   case MESA_SHADER_TESS_CTRL: sw_stage = sw_stage | SWStage::TCS; break;
   case MESA_SHADER_TESS_EVAL: sw_stage = sw_stage | SWStage::TES; break;
   case MESA_SHADER_GEOMETRY: sw_stage = sw_stage | SWStage::GS; break;
   case MESA_SHADER_FRAGMENT: sw_stage = sw_stage | SWStage::FS; break;
   case MESA_SHADER_KERNEL:
   case MESA_SHADER_COMPUTE: sw_stage = sw_stage | SWStage::CS; break;
   case MESA_SHADER_TASK: sw_stage = sw_stage | SWStage::TS; break;
   case MESA_SHADER_MESH: sw_stage = sw_stage | SWStage::MS; break;
   case MESA_SHADER_RAYGEN:
   case MESA_SHADER_CLOSEST_HIT:
   case MESA_SHADER_MISS:
   case MESA_SHADER_CALLABLE:
   case MESA_SHADER_INTERSECTION:
   case MESA_SHADER_ANY_HIT: sw_stage = SWStage::RT; break;
   default: unreachable("Shader stage not implemented");
               init_program(program, Stage{info->hw_stage, sw_stage}, info, options->gfx_level, options->family,
            isel_context ctx = {};
   ctx.program = program;
   ctx.args = args;
   ctx.options = options;
            program->workgroup_size = program->info.workgroup_size;
            /* Mesh shading only works on GFX10.3+. */
   ASSERTED bool mesh_shading = ctx.stage.has(SWStage::TS) || ctx.stage.has(SWStage::MS);
                              unsigned scratch_size = 0;
   for (unsigned i = 0; i < shader_count; i++) {
      nir_shader* nir = shaders[i];
   setup_nir(&ctx, nir);
               for (unsigned i = 0; i < shader_count; i++)
                     unsigned nir_num_blocks = 0;
   for (unsigned i = 0; i < shader_count; i++)
         ctx.program->blocks.reserve(nir_num_blocks * 2);
   ctx.block = ctx.program->create_and_insert_block();
               }
      } // namespace aco
