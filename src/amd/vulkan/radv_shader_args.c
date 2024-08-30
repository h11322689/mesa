   /*
   * Copyright © 2019 Valve Corporation.
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
      #include "radv_shader_args.h"
   #include "radv_private.h"
   #include "radv_shader.h"
      struct user_sgpr_info {
      uint64_t inline_push_constant_mask;
   bool inlined_all_push_consts;
   bool indirect_all_descriptor_sets;
      };
      static void
   allocate_inline_push_consts(const struct radv_shader_info *info, struct user_sgpr_info *user_sgpr_info)
   {
               if (!info->inline_push_constant_mask)
            uint64_t mask = info->inline_push_constant_mask;
            /* Disable the default push constants path if all constants can be inlined and if shaders don't
   * use dynamic descriptors.
   */
   if (num_push_consts <= MIN2(remaining_sgprs + 1, AC_MAX_INLINE_PUSH_CONSTS) && info->can_inline_all_push_constants &&
      !info->loads_dynamic_offsets) {
   user_sgpr_info->inlined_all_push_consts = true;
      } else {
      /* Clamp to the maximum number of allowed inlined push constants. */
   while (num_push_consts > MIN2(remaining_sgprs, AC_MAX_INLINE_PUSH_CONSTS_WITH_INDIRECT)) {
      num_push_consts--;
                  user_sgpr_info->remaining_sgprs = remaining_sgprs - util_bitcount64(mask);
      }
      static void
   add_ud_arg(struct radv_shader_args *args, unsigned size, enum ac_arg_type type, struct ac_arg *arg,
         {
                        if (ud_info->sgpr_idx == -1)
                        }
      static void
   add_descriptor_set(struct radv_shader_args *args, enum ac_arg_type type, struct ac_arg *arg, uint32_t set)
   {
               struct radv_userdata_info *ud_info = &args->user_sgprs_locs.descriptor_sets[set];
   ud_info->sgpr_idx = args->num_user_sgprs;
            args->user_sgprs_locs.descriptor_sets_enabled |= 1u << set;
      }
      static void
   declare_global_input_sgprs(const struct radv_shader_info *info, const struct user_sgpr_info *user_sgpr_info,
         {
      if (user_sgpr_info) {
      /* 1 for each descriptor set */
   if (!user_sgpr_info->indirect_all_descriptor_sets) {
                                       } else {
                  if (info->merged_shader_compiled_separately ||
      (info->loads_push_constants && !user_sgpr_info->inlined_all_push_consts)) {
   /* 1 for push constants and dynamic descriptors */
               for (unsigned i = 0; i < util_bitcount64(user_sgpr_info->inline_push_constant_mask); i++) {
         }
               if (info->so.num_outputs) {
            }
      static void
   declare_vs_specific_input_sgprs(const struct radv_shader_info *info, struct radv_shader_args *args)
   {
      if (info->vs.has_prolog)
            if (info->type != RADV_SHADER_TYPE_GS_COPY) {
      if (info->vs.vb_desc_usage_mask) {
                  add_ud_arg(args, 1, AC_ARG_INT, &args->ac.base_vertex, AC_UD_VS_BASE_VERTEX_START_INSTANCE);
   if (info->vs.needs_draw_id) {
         }
   if (info->vs.needs_base_instance) {
               }
      static void
   declare_vs_input_vgprs(enum amd_gfx_level gfx_level, const struct radv_shader_info *info, struct radv_shader_args *args,
         {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vertex_id);
   if (info->type != RADV_SHADER_TYPE_GS_COPY) {
                  if (gfx_level >= GFX11) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
      } else if (gfx_level >= GFX10) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_rel_patch_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user vgpr */
      } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_rel_patch_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.instance_id);
         } else {
      if (gfx_level >= GFX10) {
      if (info->is_ngg) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user vgpr */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user vgpr */
      } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* unused */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_prim_id);
         } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.instance_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_prim_id);
                     if (info->vs.dynamic_inputs) {
      assert(info->vs.use_per_attribute_vb_descs);
   unsigned num_attributes = util_last_bit(info->vs.input_slot_usage_mask);
   for (unsigned i = 0; i < num_attributes; i++) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 4, AC_ARG_INT, &args->vs_inputs[i]);
      }
   /* Ensure the main shader doesn't use less vgprs than the prolog. The prolog requires one
   * VGPR more than the number of shader arguments in the case of non-trivial divisors on GFX8.
   */
         }
      static void
   declare_streamout_sgprs(const struct radv_shader_info *info, struct radv_shader_args *args, gl_shader_stage stage)
   {
               /* Streamout SGPRs. */
   if (info->so.num_outputs) {
               ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.streamout_config);
      } else if (stage == MESA_SHADER_TESS_EVAL) {
                  /* A streamout buffer offset is loaded if the stride is non-zero. */
   for (i = 0; i < 4; i++) {
      if (!info->so.strides[i])
                  }
      static void
   declare_tes_input_vgprs(struct radv_shader_args *args)
   {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.tes_u);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.tes_v);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.tes_rel_patch_id);
      }
      static void
   declare_ms_input_sgprs(const struct radv_shader_info *info, struct radv_shader_args *args)
   {
      if (info->cs.uses_grid_size) {
         }
   if (info->vs.needs_draw_id) {
         }
   if (info->ms.has_task) {
            }
      static void
   declare_ms_input_vgprs(const struct radv_device *device, struct radv_shader_args *args)
   {
      if (device->mesh_fast_launch_2) {
         } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vertex_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user vgpr */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user vgpr */
         }
      static void
   declare_ps_input_vgprs(const struct radv_shader_info *info, struct radv_shader_args *args)
   {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_sample);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_center);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_centroid);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 3, AC_ARG_INT, &args->ac.pull_model);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_sample);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_center);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_centroid);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, NULL); /* line stipple tex */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[0]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[1]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[2]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[3]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.front_face);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.ancillary);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.sample_coverage);
            if (args->remap_spi_ps_input)
      }
      static void
   declare_ngg_sgprs(const struct radv_shader_info *info, struct radv_shader_args *args, bool has_ngg_provoking_vtx)
   {
      if (info->merged_shader_compiled_separately || has_ngg_provoking_vtx)
            if (info->has_ngg_culling) {
      add_ud_arg(args, 1, AC_ARG_INT, &args->ngg_culling_settings, AC_UD_NGG_CULLING_SETTINGS);
   add_ud_arg(args, 1, AC_ARG_INT, &args->ngg_viewport_scale[0], AC_UD_NGG_VIEWPORT);
   add_ud_arg(args, 1, AC_ARG_INT, &args->ngg_viewport_scale[1], AC_UD_NGG_VIEWPORT);
   add_ud_arg(args, 1, AC_ARG_INT, &args->ngg_viewport_translate[0], AC_UD_NGG_VIEWPORT);
         }
      static void
   radv_init_shader_args(const struct radv_device *device, gl_shader_stage stage, struct radv_shader_args *args)
   {
               args->explicit_scratch_args = !radv_use_llvm_for_stage(device, stage);
   args->remap_spi_ps_input = !radv_use_llvm_for_stage(device, stage);
            for (int i = 0; i < MAX_SETS; i++)
         for (int i = 0; i < AC_UD_MAX_UD; i++)
      }
      void
   radv_declare_rt_shader_args(enum amd_gfx_level gfx_level, struct radv_shader_args *args)
   {
      add_ud_arg(args, 2, AC_ARG_CONST_PTR, &args->ac.rt.uniform_shader_addr, AC_UD_SCRATCH_RING_OFFSETS);
   add_ud_arg(args, 1, AC_ARG_CONST_PTR_PTR, &args->descriptor_sets[0], AC_UD_INDIRECT_DESCRIPTOR_SETS);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_CONST_PTR, &args->ac.push_constants);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 2, AC_ARG_CONST_DESC_PTR, &args->ac.rt.sbt_descriptors);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 2, AC_ARG_CONST_PTR, &args->ac.rt.traversal_shader_addr);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 3, AC_ARG_INT, &args->ac.rt.launch_size);
   if (gfx_level < GFX9) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.scratch_offset);
               ac_add_arg(&args->ac, AC_ARG_VGPR, 3, AC_ARG_INT, &args->ac.rt.launch_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.dynamic_callable_stack_base);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_CONST_PTR, &args->ac.rt.shader_addr);
            ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.payload_offset);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 3, AC_ARG_FLOAT, &args->ac.rt.ray_origin);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 3, AC_ARG_FLOAT, &args->ac.rt.ray_direction);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.rt.ray_tmin);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.rt.ray_tmax);
            ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_CONST_PTR, &args->ac.rt.accel_struct);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.sbt_offset);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.sbt_stride);
            ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_CONST_PTR, &args->ac.rt.instance_addr);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.primitive_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.rt.geometry_id_and_flags);
      }
      static bool
   radv_tcs_needs_state_sgpr(const struct radv_shader_info *info, const struct radv_pipeline_key *key)
   {
      /* Some values are loaded from a SGPR when dynamic states are used or when the shader is unlinked. */
      }
      static bool
   radv_tes_needs_state_sgpr(const struct radv_shader_info *info)
   {
      /* Some values are loaded from a SGPR when dynamic states are used or when the shader is unlinked. */
      }
      static bool
   radv_ps_needs_state_sgpr(const struct radv_shader_info *info, const struct radv_pipeline_key *key)
   {
      if (info->ps.needs_sample_positions && key->dynamic_rasterization_samples)
            if (key->dynamic_line_rast_mode)
            if (info->ps.reads_sample_mask_in && (info->ps.uses_sample_shading || key->ps.sample_shading_enable))
            /* For computing barycentrics when the primitive topology is unknown at compile time (GPL). */
   if (info->ps.load_rasterization_prim && key->unknown_rast_prim)
               }
      static void
   declare_shader_args(const struct radv_device *device, const struct radv_pipeline_key *key,
               {
      const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   bool has_shader_query = info->has_prim_query || info->has_xfb_query ||
                     bool has_ngg_provoking_vtx =
            if (gfx_level >= GFX10 && info->is_ngg && stage != MESA_SHADER_GEOMETRY) {
      /* Handle all NGG shaders as GS to simplify the code here. */
   previous_stage = stage;
                        if (gl_shader_stage_is_rt(stage)) {
      radv_declare_rt_shader_args(gfx_level, args);
               add_ud_arg(args, 2, AC_ARG_CONST_DESC_PTR, &args->ac.ring_offsets, AC_UD_SCRATCH_RING_OFFSETS);
   if (stage == MESA_SHADER_TASK) {
                  /* For merged shaders the user SGPRs start at 8, with 8 system SGPRs in front (including
   * the rw_buffers at s0/s1. With user SGPR0 = s8, lets restart the count from 0.
   */
   if (previous_stage != MESA_SHADER_NONE)
            /* To ensure prologs match the main VS, VS specific input SGPRs have to be placed before other
   * sgprs.
            switch (stage) {
   case MESA_SHADER_COMPUTE:
   case MESA_SHADER_TASK:
               if (info->cs.uses_grid_size) {
      if (args->load_grid_size_from_user_sgpr)
         else
               if (info->cs.is_rt_shader) {
      add_ud_arg(args, 2, AC_ARG_CONST_DESC_PTR, &args->ac.rt.sbt_descriptors, AC_UD_CS_SBT_DESCRIPTORS);
   add_ud_arg(args, 2, AC_ARG_CONST_PTR, &args->ac.rt.traversal_shader_addr, AC_UD_CS_TRAVERSAL_SHADER_ADDR);
   add_ud_arg(args, 2, AC_ARG_CONST_PTR, &args->ac.rt.launch_size_addr, AC_UD_CS_RAY_LAUNCH_SIZE_ADDR);
   add_ud_arg(args, 1, AC_ARG_INT, &args->ac.rt.dynamic_callable_stack_base,
               if (info->vs.needs_draw_id) {
                  if (stage == MESA_SHADER_TASK) {
               if (has_shader_query) {
                     for (int i = 0; i < 3; i++) {
      if (info->cs.uses_block_id[i]) {
                     if (info->cs.uses_local_invocation_idx) {
                  if (args->explicit_scratch_args && gfx_level < GFX11) {
                  if (gfx_level >= GFX11)
         else
            case MESA_SHADER_VERTEX:
      /* NGG is handled by the GS case */
                              if (info->uses_view_index) {
                  if (info->force_vrs_per_vertex) {
                  if (info->vs.as_es) {
         } else if (info->vs.as_ls) {
         } else {
                  if (args->explicit_scratch_args) {
                  declare_vs_input_vgprs(gfx_level, info, args, false);
      case MESA_SHADER_TESS_CTRL:
      if (previous_stage != MESA_SHADER_NONE) {
      // First 6 system regs
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
                  if (gfx_level >= GFX11) {
         } else {
                                                   if (info->merged_shader_compiled_separately || info->uses_view_index) {
                  if (radv_tcs_needs_state_sgpr(info, key)) {
                  if (info->merged_shader_compiled_separately || info->has_epilog) {
                                                         if (info->merged_shader_compiled_separately) {
      /* SGPRs */
   ac_add_preserved(&args->ac, &args->ac.ring_offsets);
   ac_add_preserved(&args->ac, &args->ac.tess_offchip_offset);
                  if (gfx_level >= GFX11) {
         } else {
                  ac_add_preserved(&args->ac, &args->descriptor_sets[0]);
   ac_add_preserved(&args->ac, &args->ac.push_constants);
   ac_add_preserved(&args->ac, &args->ac.view_index);
                  /* VGPRs */
   ac_add_preserved(&args->ac, &args->ac.tcs_patch_id);
         } else {
               if (info->uses_view_index) {
                  if (radv_tcs_needs_state_sgpr(info, key)) {
                  if (info->has_epilog) {
                  ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tcs_factor_offset);
   if (args->explicit_scratch_args) {
         }
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.tcs_patch_id);
      }
      case MESA_SHADER_TESS_EVAL:
      /* NGG is handled by the GS case */
                     if (info->uses_view_index)
            if (radv_tes_needs_state_sgpr(info))
            if (info->tes.as_es) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
      } else {
      declare_streamout_sgprs(info, args, stage);
      }
   if (args->explicit_scratch_args) {
         }
   declare_tes_input_vgprs(args);
      case MESA_SHADER_GEOMETRY:
      if (previous_stage != MESA_SHADER_NONE) {
      // First 6 system regs
   if (info->is_ngg) {
         } else {
                                 if (gfx_level >= GFX11) {
         } else {
                                 if (previous_stage == MESA_SHADER_VERTEX) {
         } else if (previous_stage == MESA_SHADER_MESH) {
                           if (info->merged_shader_compiled_separately || info->uses_view_index) {
                                                /* Legacy GS force vrs is handled by GS copy shader. */
   if (info->force_vrs_per_vertex && info->is_ngg) {
                                 if (info->is_ngg) {
                                 if (previous_stage != MESA_SHADER_MESH || !device->mesh_fast_launch_2) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[0]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[1]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_prim_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_invocation_id);
               if (previous_stage == MESA_SHADER_VERTEX) {
         } else if (previous_stage == MESA_SHADER_TESS_EVAL) {
         } else if (previous_stage == MESA_SHADER_MESH) {
                  if (info->merged_shader_compiled_separately) {
      /* SGPRs */
   ac_add_preserved(&args->ac, &args->ac.ring_offsets);
   if (info->is_ngg) {
         } else {
         }
                  if (gfx_level >= GFX11) {
         } else {
                  ac_add_preserved(&args->ac, &args->descriptor_sets[0]);
   ac_add_preserved(&args->ac, &args->ac.push_constants);
   ac_add_preserved(&args->ac, &args->ac.view_index);
   ac_add_preserved(&args->ac, &args->shader_query_state);
                  /* VGPRs */
   ac_add_preserved(&args->ac, &args->ac.gs_vtx_offset[0]);
   ac_add_preserved(&args->ac, &args->ac.gs_vtx_offset[1]);
   ac_add_preserved(&args->ac, &args->ac.gs_prim_id);
   ac_add_preserved(&args->ac, &args->ac.gs_invocation_id);
         } else {
               if (info->uses_view_index) {
                  if (info->force_vrs_per_vertex) {
                  ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.gs2vs_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.gs_wave_id);
   if (args->explicit_scratch_args) {
         }
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[0]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[1]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_prim_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[2]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[3]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[4]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[5]);
      }
      case MESA_SHADER_FRAGMENT:
               if (info->has_epilog) {
                  if (radv_ps_needs_state_sgpr(info, key))
                     if (info->ps.pops && gfx_level < GFX11) {
                  if (info->ps.load_provoking_vtx) {
                  if (args->explicit_scratch_args && gfx_level < GFX11) {
                  declare_ps_input_vgprs(info, args);
      default:
            }
      void
   radv_declare_shader_args(const struct radv_device *device, const struct radv_pipeline_key *key,
               {
               if (gl_shader_stage_is_rt(stage))
            uint32_t num_user_sgprs = args->num_user_sgprs;
   if (info->loads_push_constants)
            const enum amd_gfx_level gfx_level = device->physical_device->rad_info.gfx_level;
   uint32_t available_sgprs = gfx_level >= GFX9 && stage != MESA_SHADER_COMPUTE && stage != MESA_SHADER_TASK ? 32 : 16;
            struct user_sgpr_info user_sgpr_info = {
                           if (info->merged_shader_compiled_separately || remaining_sgprs < num_desc_set) {
      user_sgpr_info.indirect_all_descriptor_sets = true;
      } else {
                  if (!info->merged_shader_compiled_separately)
               }
      void
   radv_declare_ps_epilog_args(const struct radv_device *device, const struct radv_ps_epilog_key *key,
         {
               /* Declare VGPR arguments for color exports. */
   for (unsigned i = 0; i < MAX_RTS; i++) {
               if (col_format == V_028714_SPI_SHADER_ZERO)
                  }
      void
   radv_declare_tcs_epilog_args(const struct radv_device *device, const struct radv_tcs_epilog_key *key,
         {
                        ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tcs_factor_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
            ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->tcs_out_current_patch_data_offset);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->invocation_id);
      }
