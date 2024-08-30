   /*
   * Copyright 2021 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "util/mesa-sha1.h"
   #include "util/u_prim.h"
   #include "sid.h"
   #include "nir.h"
         struct si_shader_profile {
      uint32_t sha1[SHA1_DIGEST_LENGTH32];
      };
      static struct si_shader_profile profiles[] =
   {
      {
      /* Plot3D */
   {0x485320cd, 0x87a9ba05, 0x24a60e4f, 0x25aa19f7, 0xf5287451},
      },
   {
      /* Viewperf/Medical */
   {0x4dce4331, 0x38f778d5, 0x1b75a717, 0x3e454fb9, 0xeb1527f0},
      },
   {
      /* Viewperf/Medical, a shader with a divergent loop doesn't benefit from Wave32,
   * probably due to interpolation performance.
   */
   {0x29f0f4a0, 0x0672258d, 0x47ccdcfd, 0x31e67dcc, 0xdcb1fda8},
      },
   {
      /* Viewperf/Creo */
   {0x1f288a73, 0xba46cce5, 0xbf68e6c6, 0x58543651, 0xca3c8efd},
         };
      static unsigned get_inst_tessfactor_writemask(nir_intrinsic_instr *intrin)
   {
      if (intrin->intrinsic != nir_intrinsic_store_output)
            unsigned writemask = nir_intrinsic_write_mask(intrin) << nir_intrinsic_component(intrin);
            if (location == VARYING_SLOT_TESS_LEVEL_OUTER)
         else if (location == VARYING_SLOT_TESS_LEVEL_INNER)
               }
      static void scan_tess_ctrl(nir_cf_node *cf_node, unsigned *upper_block_tf_writemask,
               {
      switch (cf_node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(cf_node);
   nir_foreach_instr (instr, block) {
                     nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
                     /* If we find a barrier in nested control flow put this in the
   * too hard basket. In GLSL this is not possible but it is in
   * SPIR-V.
   */
   if (is_nested_cf) {
                        /* The following case must be prevented:
   *    gl_TessLevelInner = ...;
   *    barrier();
   *    if (gl_InvocationID == 1)
   *       gl_TessLevelInner = ...;
   *
   * If you consider disjoint code segments separated by barriers, each
   * such segment that writes tess factor channels should write the same
   * channels in all codepaths within that segment.
   */
   if (*upper_block_tf_writemask || *cond_block_tf_writemask) {
                           /* Analyze the next code segment from scratch. */
   *upper_block_tf_writemask = 0;
         } else
                  }
   case nir_cf_node_if: {
      unsigned then_tessfactor_writemask = 0;
            nir_if *if_stmt = nir_cf_node_as_if(cf_node);
   foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->then_list)
   {
      scan_tess_ctrl(nested_node, &then_tessfactor_writemask, cond_block_tf_writemask,
               foreach_list_typed(nir_cf_node, nested_node, node, &if_stmt->else_list)
   {
      scan_tess_ctrl(nested_node, &else_tessfactor_writemask, cond_block_tf_writemask,
               if (then_tessfactor_writemask || else_tessfactor_writemask) {
      /* If both statements write the same tess factor channels,
   * we can say that the upper block writes them too.
   */
   *upper_block_tf_writemask |= then_tessfactor_writemask & else_tessfactor_writemask;
                  }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
   assert(!nir_loop_has_continue_construct(loop));
   foreach_list_typed(nir_cf_node, nested_node, node, &loop->body)
   {
      scan_tess_ctrl(nested_node, cond_block_tf_writemask, cond_block_tf_writemask,
                  }
   default:
            }
      static bool are_tessfactors_def_in_all_invocs(const struct nir_shader *nir)
   {
               /* The pass works as follows:
   * If all codepaths write tess factors, we can say that all
   * invocations define tess factors.
   *
   * Each tess factor channel is tracked separately.
   */
   unsigned main_block_tf_writemask = 0; /* if main block writes tess factors */
            /* Initial value = true. Here the pass will accumulate results from
   * multiple segments surrounded by barriers. If tess factors aren't
   * written at all, it's a shader bug and we don't care if this will be
   * true.
   */
            nir_foreach_function (function, nir) {
      if (function->impl) {
      foreach_list_typed(nir_cf_node, node, node, &function->impl->body)
   {
      scan_tess_ctrl(node, &main_block_tf_writemask, &cond_block_tf_writemask,
                     /* Accumulate the result for the last code segment separated by a
   * barrier.
   */
   if (main_block_tf_writemask || cond_block_tf_writemask) {
                     }
      static const nir_src *get_texture_src(nir_tex_instr *instr, nir_tex_src_type type)
   {
      for (unsigned i = 0; i < instr->num_srcs; i++) {
      if (instr->src[i].src_type == type)
      }
      }
      static void scan_io_usage(const nir_shader *nir, struct si_shader_info *info,
         {
               if (intr->intrinsic == nir_intrinsic_load_interpolated_input) {
      nir_instr *src_instr = intr->src[0].ssa->parent_instr;
   if (src_instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *baryc = nir_instr_as_intrinsic(src_instr);
   if (nir_intrinsic_infos[baryc->intrinsic].index_map[NIR_INTRINSIC_INTERP_MODE] > 0)
         else
      } else {
      /* May get here when si_update_shader_binary_info() after ps lower bc_optimize
   * which select center and centroid. Set to any value is OK because we don't
   * care this when si_update_shader_binary_info().
   */
                  unsigned mask, bit_size;
            if (nir_intrinsic_has_write_mask(intr)) {
      mask = nir_intrinsic_write_mask(intr); /* store */
   bit_size = nir_src_bit_size(intr->src[0]);
      } else {
      mask = nir_def_components_read(&intr->def); /* load */
   bit_size = intr->def.bit_size;
      }
            /* Convert the 16-bit component mask to a 32-bit component mask except for VS inputs
   * where the mask is untyped.
   */
   if (bit_size == 16 && !is_input) {
      unsigned new_mask = 0;
   for (unsigned i = 0; i < 4; i++) {
      if (mask & (1 << i))
      }
                        nir_src offset = *nir_get_io_offset_src(intr);
   bool indirect = !nir_src_is_const(offset);
   if (!indirect)
            unsigned semantic = 0;
   /* VS doesn't have semantics. */
   if (nir->info.stage != MESA_SHADER_VERTEX || !is_input)
            if (nir->info.stage == MESA_SHADER_FRAGMENT && !is_input) {
      /* Never use FRAG_RESULT_COLOR directly. */
   if (semantic == FRAG_RESULT_COLOR)
                     unsigned driver_location = nir_intrinsic_base(intr);
            if (is_input) {
               for (unsigned i = 0; i < num_slots; i++) {
                        if (semantic == VARYING_SLOT_PRIMITIVE_ID)
                        if (mask) {
      info->input[loc].usage_mask |= mask;
   if (bit_size == 16) {
      if (nir_intrinsic_io_semantics(intr).high_16bits)
         else
      }
            } else {
      /* Outputs. */
            for (unsigned i = 0; i < num_slots; i++) {
                        if (is_output_load) {
      /* Output loads have only a few things that we need to track. */
      } else if (mask) {
      /* Output stores. */
   unsigned gs_streams = (uint32_t)nir_intrinsic_io_semantics(intr).gs_streams <<
                  /* Iterate over all components. */
                     if (new_mask & (1 << i)) {
                        if (nir_intrinsic_has_io_xfb(intr)) {
      nir_io_xfb xfb = i < 2 ? nir_intrinsic_io_xfb(intr) :
         if (xfb.out[i % 2].num_components) {
      unsigned stream = (gs_streams >> (i * 2)) & 0x3;
   info->enabled_streamout_buffer_mask |=
                     if (nir_intrinsic_has_src_type(intr))
         else if (nir_intrinsic_has_dest_type(intr))
                                       if (nir->info.stage == MESA_SHADER_FRAGMENT &&
                     if (nir_intrinsic_src_type(intr) == nir_type_float16)
         else if (nir_intrinsic_src_type(intr) == nir_type_int16)
         else if (nir_intrinsic_src_type(intr) == nir_type_uint16)
                  }
      static bool is_bindless_handle_indirect(nir_instr *src)
   {
      /* Check if the bindless handle comes from indirect load_ubo. */
   if (src->type == nir_instr_type_intrinsic &&
      nir_instr_as_intrinsic(src)->intrinsic == nir_intrinsic_load_ubo) {
   if (!nir_src_is_const(nir_instr_as_intrinsic(src)->src[0]))
      } else {
      /* Some other instruction. Return the worst-case result. */
      }
      }
      /* TODO: convert to nir_shader_instructions_pass */
   static void scan_instruction(const struct nir_shader *nir, struct si_shader_info *info,
         {
      if (instr->type == nir_instr_type_tex) {
      nir_tex_instr *tex = nir_instr_as_tex(instr);
            /* Gather the types of used VMEM instructions that return something. */
   switch (tex->op) {
   case nir_texop_tex:
   case nir_texop_txb:
   case nir_texop_txl:
   case nir_texop_txd:
   case nir_texop_lod:
   case nir_texop_tg4:
      info->uses_vmem_sampler_or_bvh = true;
      default:
      info->uses_vmem_load_other = true;
               if (handle) {
               if (is_bindless_handle_indirect(handle->ssa->parent_instr))
      } else {
               if (nir_deref_instr_has_indirect(nir_src_as_deref(*deref)))
               info->has_non_uniform_tex_access =
      } else if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   const char *intr_name = nir_intrinsic_infos[intr->intrinsic].name;
   bool is_ssbo = strstr(intr_name, "ssbo");
   bool is_image = strstr(intr_name, "image") == intr_name;
            /* Gather the types of used VMEM instructions that return something. */
   if (nir_intrinsic_infos[intr->intrinsic].has_dest) {
      switch (intr->intrinsic) {
   case nir_intrinsic_load_ubo:
      if (!nir_src_is_const(intr->src[1]))
               case nir_intrinsic_load_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_per_vertex_input:
      if (nir->info.stage == MESA_SHADER_VERTEX ||
                     case nir_intrinsic_load_constant:
   case nir_intrinsic_load_barycentric_at_sample: /* This loads sample positions. */
   case nir_intrinsic_load_buffer_amd:
                  default:
      if (is_image ||
      is_bindless_image ||
   is_ssbo ||
   (strstr(intr_name, "global") == intr_name ||
   intr->intrinsic == nir_intrinsic_load_global ||
   intr->intrinsic == nir_intrinsic_store_global) ||
   strstr(intr_name, "scratch"))
                     if (is_bindless_image)
            if (nir_intrinsic_writes_external_memory(intr))
            if (is_image && nir_deref_instr_has_indirect(nir_src_as_deref(intr->src[0])))
            if (is_bindless_image && is_bindless_handle_indirect(intr->src[0].ssa->parent_instr))
            if (intr->intrinsic != nir_intrinsic_store_ssbo && is_ssbo &&
                  switch (intr->intrinsic) {
   case nir_intrinsic_store_ssbo:
      if (!nir_src_is_const(intr->src[1]))
            case nir_intrinsic_load_ubo:
      if (!nir_src_is_const(intr->src[0]))
            case nir_intrinsic_load_local_invocation_id:
   case nir_intrinsic_load_workgroup_id: {
      unsigned mask = nir_def_components_read(&intr->def);
                     if (intr->intrinsic == nir_intrinsic_load_workgroup_id)
         else
      }
      }
   case nir_intrinsic_load_color0:
   case nir_intrinsic_load_color1: {
      unsigned index = intr->intrinsic == nir_intrinsic_load_color1;
                  switch (info->color_interpolate[index]) {
   case INTERP_MODE_SMOOTH:
      if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_SAMPLE)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTROID)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTER)
            case INTERP_MODE_NOPERSPECTIVE:
      if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_SAMPLE)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTROID)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTER)
            case INTERP_MODE_COLOR:
      /* We don't know the final value. This will be FLAT if flatshading is enabled
   * in the rasterizer state, otherwise it will be SMOOTH.
   */
   info->uses_interp_color = true;
   if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_SAMPLE)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTROID)
         else if (info->color_interpolate_loc[index] == TGSI_INTERPOLATE_LOC_CENTER)
            }
      }
   case nir_intrinsic_load_barycentric_at_offset:   /* uses center */
   case nir_intrinsic_load_barycentric_at_sample:   /* uses center */
                     if (nir_intrinsic_interp_mode(intr) == INTERP_MODE_NOPERSPECTIVE) {
         } else {
         }
   if (intr->intrinsic == nir_intrinsic_load_barycentric_at_sample)
            case nir_intrinsic_load_frag_coord:
      info->reads_frag_coord_mask |= nir_def_components_read(&intr->def);
      case nir_intrinsic_load_sample_pos:
      info->reads_sample_pos_mask |= nir_def_components_read(&intr->def);
      case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_interpolated_input:
      scan_io_usage(nir, info, intr, true);
      case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
      scan_io_usage(nir, info, intr, false);
      case nir_intrinsic_load_deref:
   case nir_intrinsic_store_deref:
      /* These can only occur if there is indirect temp indexing. */
      case nir_intrinsic_interp_deref_at_centroid:
   case nir_intrinsic_interp_deref_at_sample:
   case nir_intrinsic_interp_deref_at_offset:
      unreachable("these opcodes should have been lowered");
      default:
               }
      void si_nir_scan_shader(struct si_screen *sscreen, const struct nir_shader *nir,
         {
      memset(info, 0, sizeof(*info));
            /* Get options from shader profiles. */
   for (unsigned i = 0; i < ARRAY_SIZE(profiles); i++) {
      if (_mesa_printed_sha1_equal(info->base.source_sha1, profiles[i].sha1)) {
      info->options = profiles[i].options;
                  if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      /* post_depth_coverage implies early_fragment_tests */
            info->color_interpolate[0] = nir->info.fs.color0_interp;
   info->color_interpolate[1] = nir->info.fs.color1_interp;
   for (unsigned i = 0; i < 2; i++) {
      if (info->color_interpolate[i] == INTERP_MODE_NONE)
               info->color_interpolate_loc[0] = nir->info.fs.color0_sample ? TGSI_INTERPOLATE_LOC_SAMPLE :
               info->color_interpolate_loc[1] = nir->info.fs.color1_sample ? TGSI_INTERPOLATE_LOC_SAMPLE :
               /* Set an invalid value. Will be determined at draw time if needed when the expected
   * conditions are met.
   */
                        if (nir->info.stage == MESA_SHADER_TESS_CTRL) {
                  /* tess factors are loaded as input instead of system value */
   info->reads_tess_factors = nir->info.inputs_read &
      (BITFIELD64_BIT(VARYING_SLOT_TESS_LEVEL_INNER) |
         info->uses_frontface = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_FRONT_FACE);
   info->uses_instanceid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INSTANCE_ID);
   info->uses_base_vertex = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_VERTEX);
   info->uses_base_instance = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BASE_INSTANCE);
   info->uses_invocationid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_INVOCATION_ID);
   info->uses_grid_size = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_NUM_WORKGROUPS);
   info->uses_tg_size = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_NUM_SUBGROUPS) ||
               info->uses_variable_block_size = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_WORKGROUP_SIZE);
   info->uses_drawid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_DRAW_ID);
   info->uses_primid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_PRIMITIVE_ID) ||
         info->reads_samplemask = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_SAMPLE_MASK_IN);
   info->uses_linear_sample = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_LINEAR_SAMPLE);
   info->uses_linear_centroid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_LINEAR_CENTROID);
   info->uses_linear_center = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_LINEAR_PIXEL);
   info->uses_persp_sample = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_PERSP_SAMPLE);
   info->uses_persp_centroid = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_PERSP_CENTROID);
   info->uses_persp_center = BITSET_TEST(nir->info.system_values_read, SYSTEM_VALUE_BARYCENTRIC_PERSP_PIXEL);
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      info->writes_z = nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_DEPTH);
   info->writes_stencil = nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_STENCIL);
            info->colors_written = nir->info.outputs_written >> FRAG_RESULT_DATA0;
   if (nir->info.outputs_written & BITFIELD64_BIT(FRAG_RESULT_COLOR)) {
      info->color0_writes_all_cbufs = true;
      }
   if (nir->info.fs.color_is_dual_source)
      } else {
      info->writes_primid = nir->info.outputs_written & VARYING_BIT_PRIMITIVE_ID;
   info->writes_viewport_index = nir->info.outputs_written & VARYING_BIT_VIEWPORT;
   info->writes_layer = nir->info.outputs_written & VARYING_BIT_LAYER;
   info->writes_psize = nir->info.outputs_written & VARYING_BIT_PSIZ;
   info->writes_clipvertex = nir->info.outputs_written & VARYING_BIT_CLIP_VERTEX;
   info->writes_edgeflag = nir->info.outputs_written & VARYING_BIT_EDGE;
               nir_function_impl *impl = nir_shader_get_entrypoint((nir_shader*)nir);
   nir_foreach_block (block, impl) {
      nir_foreach_instr (instr, block)
               if (nir->info.stage == MESA_SHADER_VERTEX || nir->info.stage == MESA_SHADER_TESS_EVAL) {
      /* Add the PrimitiveID output, but don't increment num_outputs.
   * The driver inserts PrimitiveID only when it's used by the pixel shader,
   * and si_emit_spi_map uses this unconditionally when such a pixel shader is used.
   */
   info->output_semantic[info->num_outputs] = VARYING_SLOT_PRIMITIVE_ID;
   info->output_type[info->num_outputs] = nir_type_uint32;
               if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      info->allow_flat_shading = !(info->uses_persp_center || info->uses_persp_centroid ||
                              info->uses_persp_sample || info->uses_linear_center ||
   info->uses_linear_centroid || info->uses_linear_sample ||
   info->uses_interp_at_sample || nir->info.writes_memory ||
   nir->info.fs.uses_fbfetch_output ||
   nir->info.fs.needs_quad_helper_invocations ||
                        /* Add both front and back color inputs. */
   unsigned num_inputs_with_colors = info->num_inputs;
   for (unsigned back = 0; back < 2; back++) {
      for (unsigned i = 0; i < 2; i++) {
                        info->input[index].semantic = (back ? VARYING_SLOT_BFC0 : VARYING_SLOT_COL0) + i;
                        /* Back-face color don't increment num_inputs. si_emit_spi_map will use
   * back-face colors conditionally only when they are needed.
   */
   if (!back)
                                 /* Trim output read masks based on write masks. */
   for (unsigned i = 0; i < info->num_outputs; i++)
                     if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_TESS_CTRL ||
   nir->info.stage == MESA_SHADER_TESS_EVAL ||
   nir->info.stage == MESA_SHADER_GEOMETRY) {
   if (nir->info.stage == MESA_SHADER_TESS_CTRL) {
      /* Always reserve space for these. */
   info->patch_outputs_written |=
      (1ull << ac_shader_io_get_unique_index_patch(VARYING_SLOT_TESS_LEVEL_INNER)) |
   }
   for (unsigned i = 0; i < info->num_outputs; i++) {
               if (semantic == VARYING_SLOT_TESS_LEVEL_INNER ||
      semantic == VARYING_SLOT_TESS_LEVEL_OUTER ||
   (semantic >= VARYING_SLOT_PATCH0 && semantic < VARYING_SLOT_TESS_MAX)) {
      } else if ((semantic <= VARYING_SLOT_VAR31 || semantic >= VARYING_SLOT_VAR0_16BIT) &&
                     /* Ignore outputs that are not passed from VS to PS. */
   if (semantic != VARYING_SLOT_POS &&
      semantic != VARYING_SLOT_PSIZ &&
   semantic != VARYING_SLOT_CLIP_VERTEX) {
   info->outputs_written_before_ps |= 1ull
                        if (nir->info.stage == MESA_SHADER_VERTEX) {
      info->num_vs_inputs =
         unsigned num_vbos_in_sgprs = si_num_vbos_in_user_sgprs_inline(sscreen->info.gfx_level);
            /* The prolog is a no-op if there are no inputs. */
               if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_TESS_CTRL ||
   nir->info.stage == MESA_SHADER_TESS_EVAL) {
   info->esgs_vertex_stride = util_last_bit64(info->outputs_written) * 16;
            /* Add 1 dword to reduce LDS bank conflicts, so that each vertex
   * will start on a different bank. (except for the maximum 32*16).
   */
            /* For the ESGS ring in LDS, add 1 dword to reduce LDS bank
   * conflicts, i.e. each vertex will start on a different bank.
   */
   if (sscreen->info.gfx_level >= GFX9)
         else
            info->tcs_vgpr_only_inputs = ~info->base.tess.tcs_cross_invocation_inputs_read &
                     if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      info->gsvs_vertex_size = info->num_outputs * 16;
   info->max_gsvs_emit_size = info->gsvs_vertex_size * info->base.gs.vertices_out;
   info->gs_input_verts_per_prim =
               info->clipdist_mask = info->writes_clipvertex ? SI_USER_CLIP_PLANE_MASK :
         info->culldist_mask = u_bit_consecutive(0, info->base.cull_distance_array_size) <<
            if (nir->info.stage == MESA_SHADER_FRAGMENT) {
      for (unsigned i = 0; i < info->num_inputs; i++) {
               if ((semantic <= VARYING_SLOT_VAR31 || semantic >= VARYING_SLOT_VAR0_16BIT) &&
      semantic != VARYING_SLOT_PNTC) {
                  for (unsigned i = 0; i < 8; i++)
                  for (unsigned i = 0; i < info->num_inputs; i++) {
      if (info->input[i].semantic == VARYING_SLOT_COL0)
         else if (info->input[i].semantic == VARYING_SLOT_COL1)
            }
      enum ac_hw_stage
   si_select_hw_stage(const gl_shader_stage stage, const union si_shader_key *const key,
         {
      switch (stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
      if (key->ge.as_ngg)
         else if (key->ge.as_es)
         else if (key->ge.as_ls)
         else
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_GEOMETRY:
      if (key->ge.as_ngg)
         else
      case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
   case MESA_SHADER_KERNEL:
         default:
            }
