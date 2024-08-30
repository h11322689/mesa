   /*
   * Copyright © 2022 Valve Corporation
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
      #include "ac_nir.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "radv_constants.h"
   #include "radv_nir.h"
   #include "radv_private.h"
   #include "radv_shader.h"
   #include "radv_shader_args.h"
      #define GET_SGPR_FIELD_NIR(arg, field)                                                                                 \
            typedef struct {
      enum amd_gfx_level gfx_level;
   const struct radv_shader_args *args;
   const struct radv_shader_info *info;
   const struct radv_pipeline_key *pl_key;
   uint32_t address32_hi;
      } lower_abi_state;
      static nir_def *
   load_ring(nir_builder *b, unsigned ring, lower_abi_state *s)
   {
      struct ac_arg arg =
            nir_def *ring_offsets = ac_nir_load_arg(b, &s->args->ac, arg);
   ring_offsets = nir_pack_64_2x32_split(b, nir_channel(b, ring_offsets, 0), nir_channel(b, ring_offsets, 1));
      }
      static nir_def *
   nggc_bool_setting(nir_builder *b, unsigned mask, lower_abi_state *s)
   {
      nir_def *settings = ac_nir_load_arg(b, &s->args->ac, s->args->ngg_culling_settings);
      }
      static nir_def *
   shader_query_bool_setting(nir_builder *b, unsigned mask, lower_abi_state *s)
   {
      nir_def *settings = ac_nir_load_arg(b, &s->args->ac, s->args->shader_query_state);
      }
      static bool
   lower_abi_instr(nir_builder *b, nir_intrinsic_instr *intrin, void *state)
   {
      lower_abi_state *s = (lower_abi_state *)state;
                     nir_def *replacement = NULL;
            switch (intrin->intrinsic) {
   case nir_intrinsic_load_ring_tess_factors_amd:
      replacement = load_ring(b, RING_HS_TESS_FACTOR, s);
      case nir_intrinsic_load_ring_tess_factors_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.tcs_factor_offset);
      case nir_intrinsic_load_ring_tess_offchip_amd:
      replacement = load_ring(b, RING_HS_TESS_OFFCHIP, s);
      case nir_intrinsic_load_ring_tess_offchip_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.tess_offchip_offset);
      case nir_intrinsic_load_tcs_num_patches_amd:
      if (s->info->num_tess_patches) {
         } else {
      if (stage == MESA_SHADER_TESS_CTRL) {
         } else {
            }
      case nir_intrinsic_load_ring_esgs_amd:
      replacement = load_ring(b, stage == MESA_SHADER_GEOMETRY ? RING_ESGS_GS : RING_ESGS_VS, s);
      case nir_intrinsic_load_ring_gsvs_amd:
      if (stage == MESA_SHADER_VERTEX)
         else
            case nir_intrinsic_load_ring_gs2vs_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs2vs_offset);
      case nir_intrinsic_load_ring_es2gs_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.es2gs_offset);
         case nir_intrinsic_load_ring_attr_amd:
               /* Note, the HW always assumes there is at least 1 per-vertex param. */
            nir_def *dword1 = nir_channel(b, replacement, 1);
   dword1 = nir_ior_imm(b, dword1, S_008F04_STRIDE(16 * total_num_params));
   replacement = nir_vector_insert_imm(b, replacement, dword1, 1);
         case nir_intrinsic_load_ring_attr_offset_amd: {
      nir_def *ring_attr_offset = ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_attr_offset);
   replacement = nir_ishl_imm(b, nir_ubfe_imm(b, ring_attr_offset, 0, 15), 9); /* 512b increments. */
               case nir_intrinsic_load_tess_rel_patch_id_amd:
      if (stage == MESA_SHADER_TESS_CTRL) {
         } else if (stage == MESA_SHADER_TESS_EVAL) {
      /* Setting an upper bound like this will actually make it possible
   * to optimize some multiplications (in address calculations) so that
   * constant additions can be added to the const offset in memory load instructions.
                  if (s->info->tes.tcs_vertices_out) {
      nir_intrinsic_instr *load_arg = nir_instr_as_intrinsic(arg->parent_instr);
                  } else {
         }
      case nir_intrinsic_load_patch_vertices_in:
      if (stage == MESA_SHADER_TESS_CTRL) {
      if (s->pl_key->tcs.tess_input_vertices) {
         } else {
            } else if (stage == MESA_SHADER_TESS_EVAL) {
      if (s->info->tes.tcs_vertices_out) {
         } else {
            } else
            case nir_intrinsic_load_gs_vertex_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_vtx_offset[nir_intrinsic_base(intrin)]);
      case nir_intrinsic_load_workgroup_num_input_vertices_amd:
      replacement = nir_ubfe_imm(b, ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_tg_info), 12, 9);
      case nir_intrinsic_load_workgroup_num_input_primitives_amd:
      replacement = nir_ubfe_imm(b, ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_tg_info), 22, 9);
      case nir_intrinsic_load_packed_passthrough_primitive_amd:
      /* NGG passthrough mode: the HW already packs the primitive export value to a single register.
   */
   replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_vtx_offset[0]);
      case nir_intrinsic_load_pipeline_stat_query_enabled_amd:
      replacement = shader_query_bool_setting(b, radv_shader_query_pipeline_stat, s);
      case nir_intrinsic_load_prim_gen_query_enabled_amd:
      replacement = shader_query_bool_setting(b, radv_shader_query_prim_gen, s);
      case nir_intrinsic_load_prim_xfb_query_enabled_amd:
      replacement = shader_query_bool_setting(b, radv_shader_query_prim_xfb, s);
      case nir_intrinsic_load_merged_wave_info_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.merged_wave_info);
      case nir_intrinsic_load_cull_any_enabled_amd: {
               /* Consider a workgroup small if it contains less than 16 triangles.
   *
   * The gs_tg_info[30:22] is the number of primitives, which we know is non-zero,
   * so the below is equivalent to: "ult(ubfe(gs_tg_info, 22, 9), 16)", but
   * ACO can optimize out the comparison to zero (see try_optimize_scc_nocompare).
   */
            nir_def *mask =
      nir_bcsel(b, small_workgroup, nir_imm_int(b, radv_nggc_none),
      nir_def *settings = ac_nir_load_arg(b, &s->args->ac, s->args->ngg_culling_settings);
   replacement = nir_ine_imm(b, nir_iand(b, settings, mask), 0);
      }
   case nir_intrinsic_load_cull_front_face_enabled_amd:
      replacement = nggc_bool_setting(b, radv_nggc_front_face, s);
      case nir_intrinsic_load_cull_back_face_enabled_amd:
      replacement = nggc_bool_setting(b, radv_nggc_back_face, s);
      case nir_intrinsic_load_cull_ccw_amd:
      replacement = nggc_bool_setting(b, radv_nggc_face_is_ccw, s);
      case nir_intrinsic_load_cull_small_primitives_enabled_amd:
      replacement = nggc_bool_setting(b, radv_nggc_small_primitives, s);
      case nir_intrinsic_load_cull_small_prim_precision_amd: {
      /* To save space, only the exponent is stored in the high 8 bits.
   * We calculate the precision from those 8 bits:
   * exponent = nggc_settings >> 24
   * precision = 1.0 * 2 ^ exponent
   */
   nir_def *settings = ac_nir_load_arg(b, &s->args->ac, s->args->ngg_culling_settings);
   nir_def *exponent = nir_ishr_imm(b, settings, 24u);
   replacement = nir_ldexp(b, nir_imm_float(b, 1.0f), exponent);
               case nir_intrinsic_load_viewport_xy_scale_and_offset: {
      nir_def *comps[] = {
      ac_nir_load_arg(b, &s->args->ac, s->args->ngg_viewport_scale[0]),
   ac_nir_load_arg(b, &s->args->ac, s->args->ngg_viewport_scale[1]),
   ac_nir_load_arg(b, &s->args->ac, s->args->ngg_viewport_translate[0]),
      };
   replacement = nir_vec(b, comps, 4);
               case nir_intrinsic_load_ring_task_draw_amd:
      replacement = load_ring(b, RING_TS_DRAW, s);
      case nir_intrinsic_load_ring_task_payload_amd:
      replacement = load_ring(b, RING_TS_PAYLOAD, s);
      case nir_intrinsic_load_ring_mesh_scratch_amd:
      replacement = load_ring(b, RING_MS_SCRATCH, s);
      case nir_intrinsic_load_ring_mesh_scratch_offset_amd:
      /* gs_tg_info[0:11] is ordered_wave_id. Multiply by the ring entry size. */
   replacement = nir_imul_imm(b, nir_iand_imm(b, ac_nir_load_arg(b, &s->args->ac, s->args->ac.gs_tg_info), 0xfff),
            case nir_intrinsic_load_task_ring_entry_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.task_ring_entry);
      case nir_intrinsic_load_lshs_vertex_stride_amd: {
      if (stage == MESA_SHADER_VERTEX) {
         } else {
      assert(stage == MESA_SHADER_TESS_CTRL);
   if (s->info->inputs_linked) {
         } else {
      nir_def *lshs_vertex_stride =
               }
      }
   case nir_intrinsic_load_esgs_vertex_stride_amd: {
      /* Emulate VGT_ESGS_RING_ITEMSIZE on GFX9+ to reduce context register writes. */
   assert(s->gfx_level >= GFX9);
   const unsigned stride =
         replacement = nir_imm_int(b, stride);
      }
   case nir_intrinsic_load_hs_out_patch_data_offset_amd: {
               if (stage == MESA_SHADER_TESS_CTRL) {
      num_tcs_outputs = nir_imm_int(b, s->info->tcs.num_linked_outputs);
      } else {
      if (s->info->inputs_linked) {
         } else {
                  if (s->info->tes.tcs_vertices_out) {
         } else {
                     nir_def *per_vertex_output_patch_size =
            if (s->info->num_tess_patches) {
      unsigned num_patches = s->info->num_tess_patches;
      } else {
               if (stage == MESA_SHADER_TESS_CTRL) {
         } else {
         }
      }
      }
   case nir_intrinsic_load_sample_positions_amd: {
               nir_def *ring_offsets = ac_nir_load_arg(b, &s->args->ac, s->args->ac.ring_offsets);
   nir_def *addr = nir_pack_64_2x32(b, ring_offsets);
   nir_def *sample_id = nir_umin(b, intrin->src[0].ssa, nir_imm_int(b, 7));
            nir_const_value *const_num_samples = nir_src_as_const_value(intrin->src[1]);
   if (const_num_samples) {
         } else {
                  replacement =
            }
   case nir_intrinsic_load_rasterization_samples_amd:
      if (s->pl_key->dynamic_rasterization_samples) {
         } else {
         }
      case nir_intrinsic_load_provoking_vtx_in_prim_amd: {
      if (s->pl_key->dynamic_provoking_vtx_mode) {
         } else {
      unsigned provoking_vertex = 0;
   if (s->pl_key->vs.provoking_vtx_last) {
      if (stage == MESA_SHADER_VERTEX) {
         } else if (stage == MESA_SHADER_GEOMETRY) {
         } else {
      /* TES won't use this intrinsic, because it can get primitive id directly
   * instead of using this intrinsic to pass primitive id by LDS.
   */
                     }
      }
   case nir_intrinsic_atomic_add_gs_emit_prim_count_amd:
      nir_gds_atomic_add_amd(b, 32, intrin->src[0].ssa, nir_imm_int(b, RADV_SHADER_QUERY_GS_PRIM_EMIT_OFFSET),
            case nir_intrinsic_atomic_add_gen_prim_count_amd: {
      uint32_t offset = stage == MESA_SHADER_MESH ? RADV_SHADER_QUERY_MS_PRIM_GEN_OFFSET
            nir_gds_atomic_add_amd(b, 32, intrin->src[0].ssa, nir_imm_int(b, offset), nir_imm_int(b, 0x100));
      }
   case nir_intrinsic_atomic_add_xfb_prim_count_amd:
      nir_gds_atomic_add_amd(b, 32, intrin->src[0].ssa,
                  case nir_intrinsic_atomic_add_shader_invocation_count_amd: {
               if (stage == MESA_SHADER_MESH) {
         } else if (stage == MESA_SHADER_TASK) {
         } else {
                  nir_gds_atomic_add_amd(b, 32, intrin->src[0].ssa, nir_imm_int(b, offset), nir_imm_int(b, 0x100));
      }
   case nir_intrinsic_load_streamout_config_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.streamout_config);
      case nir_intrinsic_load_streamout_write_index_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.streamout_write_index);
      case nir_intrinsic_load_streamout_buffer_amd: {
      nir_def *ptr = nir_pack_64_2x32_split(b, ac_nir_load_arg(b, &s->args->ac, s->args->streamout_buffers),
         replacement = nir_load_smem_amd(b, 4, ptr, nir_imm_int(b, nir_intrinsic_base(intrin) * 16));
      }
   case nir_intrinsic_load_streamout_offset_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.streamout_offset[nir_intrinsic_base(intrin)]);
         case nir_intrinsic_load_lds_ngg_gs_out_vertex_base_amd:
      replacement = nir_imm_int(b, s->info->ngg_info.esgs_ring_size);
      case nir_intrinsic_load_lds_ngg_scratch_base_amd:
      replacement = nir_imm_int(b, s->info->ngg_info.scratch_lds_base);
      case nir_intrinsic_load_num_vertices_per_primitive_amd: {
               if (stage == MESA_SHADER_VERTEX) {
      /* For dynamic primitive topology with streamout. */
   if (s->info->vs.dynamic_num_verts_per_prim) {
         } else {
            } else if (stage == MESA_SHADER_TESS_EVAL) {
      if (s->info->tes.point_mode) {
         } else if (s->info->tes._primitive_mode == TESS_PRIMITIVE_ISOLINES) {
         } else {
         }
      } else {
      assert(stage == MESA_SHADER_GEOMETRY);
   switch (s->info->gs.output_prim) {
   case MESA_PRIM_POINTS:
      num_vertices = 1;
      case MESA_PRIM_LINE_STRIP:
      num_vertices = 2;
      case MESA_PRIM_TRIANGLE_STRIP:
      num_vertices = 3;
      default:
      unreachable("invalid GS output primitive");
      }
      }
      }
   case nir_intrinsic_load_ordered_id_amd:
      replacement = ac_nir_unpack_arg(b, &s->args->ac, s->args->ac.gs_tg_info, 0, 12);
      case nir_intrinsic_load_force_vrs_rates_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.force_vrs_rates);
      case nir_intrinsic_load_fully_covered: {
      nir_def *sample_coverage = ac_nir_load_arg(b, &s->args->ac, s->args->ac.sample_coverage);
   replacement = nir_ine_imm(b, sample_coverage, 0);
      }
   case nir_intrinsic_load_barycentric_optimize_amd: {
      nir_def *prim_mask = ac_nir_load_arg(b, &s->args->ac, s->args->ac.prim_mask);
   /* enabled when bit 31 is set */
   replacement = nir_ilt_imm(b, prim_mask, 0);
      }
   case nir_intrinsic_load_poly_line_smooth_enabled:
      if (s->pl_key->dynamic_line_rast_mode) {
      nir_def *line_rast_mode = GET_SGPR_FIELD_NIR(s->args->ps_state, PS_STATE_LINE_RAST_MODE);
      } else {
         }
      case nir_intrinsic_load_initial_edgeflags_amd:
      replacement = nir_imm_int(b, 0);
      case nir_intrinsic_load_provoking_vtx_amd:
      replacement = ac_nir_load_arg(b, &s->args->ac, s->args->ac.load_provoking_vtx);
      case nir_intrinsic_load_rasterization_primitive_amd:
      assert(s->pl_key->unknown_rast_prim);
   /* Load the primitive topology from an user SGPR when it's unknown at compile time (GPL). */
   replacement = GET_SGPR_FIELD_NIR(s->args->ps_state, PS_STATE_RAST_PRIM);
      default:
      progress = false;
               if (!progress)
            if (replacement)
            nir_instr_remove(&intrin->instr);
               }
      static nir_def *
   load_gsvs_ring(nir_builder *b, lower_abi_state *s, unsigned stream_id)
   {
      nir_def *ring = load_ring(b, RING_GSVS_GS, s);
   unsigned stream_offset = 0;
   unsigned stride = 0;
   for (unsigned i = 0; i <= stream_id; i++) {
      stride = 4 * s->info->gs.num_stream_output_components[i] * s->info->gs.vertices_out;
   if (i < stream_id)
               /* Limit on the stride field for <= GFX7. */
            if (stream_offset) {
      nir_def *addr = nir_pack_64_2x32_split(b, nir_channel(b, ring, 0), nir_channel(b, ring, 1));
   addr = nir_iadd_imm(b, addr, stream_offset);
   ring = nir_vector_insert_imm(b, ring, nir_unpack_64_2x32_split_x(b, addr), 0);
               ring = nir_vector_insert_imm(b, ring, nir_ior_imm(b, nir_channel(b, ring, 1), S_008F04_STRIDE(stride)), 1);
      }
      void
   radv_nir_lower_abi(nir_shader *shader, enum amd_gfx_level gfx_level, const struct radv_shader_info *info,
         {
      lower_abi_state state = {
      .gfx_level = gfx_level,
   .info = info,
   .args = args,
   .pl_key = pl_key,
               if (shader->info.stage == MESA_SHADER_GEOMETRY && !info->is_ngg) {
                        u_foreach_bit (i, shader->info.gs.active_stream_mask)
                  }
