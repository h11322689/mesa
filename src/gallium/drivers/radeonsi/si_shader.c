   /*
   * Copyright 2012 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_shader.h"
   #include "ac_nir.h"
   #include "ac_rtld.h"
   #include "nir.h"
   #include "nir_builder.h"
   #include "nir_serialize.h"
   #include "nir_xfb_info.h"
   #include "si_pipe.h"
   #include "si_shader_internal.h"
   #include "sid.h"
   #include "tgsi/tgsi_from_mesa.h"
   #include "util/u_memory.h"
   #include "util/mesa-sha1.h"
   #include "util/ralloc.h"
   #include "util/u_upload_mgr.h"
      static const char scratch_rsrc_dword0_symbol[] = "SCRATCH_RSRC_DWORD0";
      static const char scratch_rsrc_dword1_symbol[] = "SCRATCH_RSRC_DWORD1";
      static void si_dump_shader_key(const struct si_shader *shader, FILE *f);
   static void si_fix_resource_usage(struct si_screen *sscreen, struct si_shader *shader);
      /* Get the number of all interpolated inputs */
   unsigned si_get_ps_num_interp(struct si_shader *ps)
   {
      struct si_shader_info *info = &ps->selector->info;
   unsigned num_colors = !!(info->colors_read & 0x0f) + !!(info->colors_read & 0xf0);
   unsigned num_interp =
            assert(num_interp <= 32);
      }
      /** Whether the shader runs as a combination of multiple API shaders */
   bool si_is_multi_part_shader(struct si_shader *shader)
   {
      if (shader->selector->screen->info.gfx_level <= GFX8 ||
      shader->selector->stage > MESA_SHADER_GEOMETRY)
         return shader->key.ge.as_ls || shader->key.ge.as_es ||
            }
      /** Whether the shader runs on a merged HW stage (LSHS or ESGS) */
   bool si_is_merged_shader(struct si_shader *shader)
   {
      if (shader->selector->stage > MESA_SHADER_GEOMETRY || shader->is_gs_copy_shader)
               }
      /**
   * Returns a unique index for a semantic name and index. The index must be
   * less than 64, so that a 64-bit bitmask of used inputs or outputs can be
   * calculated.
   */
   unsigned si_shader_io_get_unique_index(unsigned semantic)
   {
      switch (semantic) {
   case VARYING_SLOT_POS:
         default:
      if (semantic >= VARYING_SLOT_VAR0 && semantic <= VARYING_SLOT_VAR31)
            if (semantic >= VARYING_SLOT_VAR0_16BIT && semantic <= VARYING_SLOT_VAR15_16BIT)
            assert(!"invalid generic index");
         /* Legacy desktop GL varyings. */
   case VARYING_SLOT_FOGC:
         case VARYING_SLOT_COL0:
         case VARYING_SLOT_COL1:
         case VARYING_SLOT_BFC0:
         case VARYING_SLOT_BFC1:
         case VARYING_SLOT_TEX0:
   case VARYING_SLOT_TEX1:
   case VARYING_SLOT_TEX2:
   case VARYING_SLOT_TEX3:
   case VARYING_SLOT_TEX4:
   case VARYING_SLOT_TEX5:
   case VARYING_SLOT_TEX6:
   case VARYING_SLOT_TEX7:
         case VARYING_SLOT_CLIP_VERTEX:
            /* Varyings present in both GLES and desktop GL. */
   case VARYING_SLOT_CLIP_DIST0:
         case VARYING_SLOT_CLIP_DIST1:
         case VARYING_SLOT_PSIZ:
         case VARYING_SLOT_LAYER:
         case VARYING_SLOT_VIEWPORT:
         case VARYING_SLOT_PRIMITIVE_ID:
            }
      static void declare_streamout_params(struct si_shader_args *args, struct si_shader *shader)
   {
               if (shader->selector->screen->info.gfx_level >= GFX11) {
      /* NGG streamout. */
   if (sel->stage == MESA_SHADER_TESS_EVAL)
                     /* Streamout SGPRs. */
   if (si_shader_uses_streamout(shader)) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.streamout_config);
            /* A streamout buffer offset is loaded if the stride is non-zero. */
   for (int i = 0; i < 4; i++) {
                           } else if (sel->stage == MESA_SHADER_TESS_EVAL) {
            }
      unsigned si_get_max_workgroup_size(const struct si_shader *shader)
   {
      gl_shader_stage stage = shader->is_gs_copy_shader ?
            switch (stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_EVAL:
      /* Use the largest workgroup size for streamout */
   if (shader->key.ge.as_ngg)
         else
         case MESA_SHADER_TESS_CTRL:
      /* Return this so that LLVM doesn't remove s_barrier
   * instructions on chips where we use s_barrier. */
         case MESA_SHADER_GEOMETRY:
      /* GS can always generate up to 256 vertices. */
         case MESA_SHADER_COMPUTE:
            default:
                  /* Compile a variable block size using the maximum variable size. */
   if (shader->selector->info.base.workgroup_size_variable)
            uint16_t *local_size = shader->selector->info.base.workgroup_size;
   unsigned max_work_group_size = (uint32_t)local_size[0] *
               assert(max_work_group_size);
      }
      static void declare_const_and_shader_buffers(struct si_shader_args *args,
               {
               if (shader->selector->info.base.num_ubos == 1 &&
      shader->selector->info.base.num_ssbos == 0)
      else
            ac_add_arg(
      &args->ac, AC_ARG_SGPR, 1, const_shader_buf_type,
   }
      static void declare_samplers_and_images(struct si_shader_args *args, bool assign_params)
   {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_CONST_IMAGE_PTR,
      }
      static void declare_per_stage_desc_pointers(struct si_shader_args *args,
               {
      declare_const_and_shader_buffers(args, shader, assign_params);
      }
      static void declare_global_desc_pointers(struct si_shader_args *args)
   {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_CONST_DESC_PTR, &args->internal_bindings);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_CONST_IMAGE_PTR,
      }
      static void declare_vb_descriptor_input_sgprs(struct si_shader_args *args,
         {
               unsigned num_vbos_in_user_sgprs = shader->selector->info.num_vbos_in_user_sgprs;
   if (num_vbos_in_user_sgprs) {
               if (si_is_merged_shader(shader))
                  /* Declare unused SGPRs to align VB descriptors to 4 SGPRs (hw requirement). */
   for (unsigned i = user_sgprs; i < SI_SGPR_VS_VB_DESCRIPTOR_FIRST; i++)
            assert(num_vbos_in_user_sgprs <= ARRAY_SIZE(args->vb_descriptors));
   for (unsigned i = 0; i < num_vbos_in_user_sgprs; i++)
         }
      static void declare_vs_input_vgprs(struct si_shader_args *args, struct si_shader *shader,
         {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vertex_id);
   if (shader->key.ge.as_ls) {
      if (shader->selector->screen->info.gfx_level >= GFX11) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
      } else if (shader->selector->screen->info.gfx_level >= GFX10) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_rel_patch_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
      } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_rel_patch_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.instance_id);
         } else if (shader->selector->screen->info.gfx_level >= GFX10) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* user VGPR */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT,
                  } else {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.instance_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.vs_prim_id);
               if (!shader->is_gs_copy_shader) {
      /* Vertex load indices. */
   if (shader->selector->info.num_inputs) {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->vertex_index0);
   for (unsigned i = 1; i < shader->selector->info.num_inputs; i++)
      }
         }
      static void declare_vs_blit_inputs(struct si_shader *shader, struct si_shader_args *args)
   {
               ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->vs_blit_inputs); /* i16 x1, y1 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);                  /* i16 x1, y1 */
            if (shader->selector->info.base.vs.blit_sgprs_amd ==
      SI_VS_BLIT_SGPRS_POS_COLOR + has_attribute_ring_address) {
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* color0 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* color1 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* color2 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* color3 */
   if (has_attribute_ring_address)
      } else if (shader->selector->info.base.vs.blit_sgprs_amd ==
            ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.x1 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.y1 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.x2 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.y2 */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.z */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_FLOAT, NULL); /* texcoord.w */
   if (has_attribute_ring_address)
         }
      static void declare_tes_input_vgprs(struct si_shader_args *args)
   {
      ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.tes_u);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.tes_v);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.tes_rel_patch_id);
      }
      enum
   {
      /* Convenient merged shader definitions. */
   SI_SHADER_MERGED_VERTEX_TESSCTRL = MESA_ALL_SHADER_STAGES,
      };
      void si_add_arg_checked(struct ac_shader_args *args, enum ac_arg_regfile file, unsigned registers,
         {
      assert(args->arg_count == idx);
      }
      void si_init_shader_args(struct si_shader *shader, struct si_shader_args *args)
   {
      unsigned i, num_returns, num_return_sgprs;
   unsigned num_prolog_vgprs = 0;
   struct si_shader_selector *sel = shader->selector;
   unsigned stage = shader->is_gs_copy_shader ? MESA_SHADER_VERTEX : sel->stage;
                     /* Set MERGED shaders. */
   if (sel->screen->info.gfx_level >= GFX9 && stage <= MESA_SHADER_GEOMETRY) {
      if (shader->key.ge.as_ls || stage == MESA_SHADER_TESS_CTRL)
         else if (shader->key.ge.as_es || shader->key.ge.as_ngg || stage == MESA_SHADER_GEOMETRY)
               switch (stage_case) {
   case MESA_SHADER_VERTEX:
               if (sel->info.base.vs.blit_sgprs_amd) {
         } else {
                     if (shader->is_gs_copy_shader) {
         } else {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.base_vertex);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.draw_id);
                  if (shader->key.ge.as_es) {
         } else if (shader->key.ge.as_ls) {
         } else {
                        /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (shader->use_aco && sel->screen->info.gfx_level < GFX11)
            /* VGPRs */
                  case MESA_SHADER_TESS_CTRL: /* GFX6-GFX8 */
      declare_global_desc_pointers(args);
   declare_per_stage_desc_pointers(args, shader, true);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tes_offchip_addr);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->vs_state_bits);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
            /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (shader->use_aco && sel->screen->info.gfx_level < GFX11)
            /* VGPRs */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.tcs_patch_id);
            /* For monolithic shaders, the TCS epilog code is generated by
   * ac_nir_lower_hs_outputs_to_mem.
   */
   if (!shader->is_monolithic) {
      /* param_tcs_offchip_offset and param_tcs_factor_offset are
   * placed after the user SGPRs.
   */
   for (i = 0; i < GFX6_TCS_NUM_USER_SGPR + 2; i++)
         for (i = 0; i < 11; i++)
      }
         case SI_SHADER_MERGED_VERTEX_TESSCTRL:
      /* Merged stages have 8 system SGPRs at the beginning. */
   /* Gfx9-10: SPI_SHADER_USER_DATA_ADDR_LO/HI_HS */
   /* Gfx11+:  SPI_SHADER_PGM_LO/HI_HS */
   declare_per_stage_desc_pointers(args, shader, stage == MESA_SHADER_TESS_CTRL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.merged_wave_info);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tcs_factor_offset);
   if (sel->screen->info.gfx_level >= GFX11)
         else
         ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL); /* unused */
            declare_global_desc_pointers(args);
            ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->vs_state_bits);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.base_vertex);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.draw_id);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.start_instance);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
            /* VGPRs (first TCS, then VS) */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.tcs_patch_id);
            if (stage == MESA_SHADER_VERTEX) {
               /* Need to keep LS/HS arg index same for shared args when ACO,
   * so this is not able to be before shared VGPRs.
                  /* LS return values are inputs to the TCS main shader part. */
   if (!shader->is_monolithic || shader->key.ge.opt.same_patch_vertices) {
      for (i = 0; i < 8 + GFX9_TCS_NUM_USER_SGPR; i++)
                        /* VS outputs passed via VGPRs to TCS. */
   if (shader->key.ge.opt.same_patch_vertices && !shader->use_aco) {
      unsigned num_outputs = util_last_bit64(shader->selector->info.outputs_written);
   for (i = 0; i < num_outputs * 4; i++)
            } else {
      /* TCS inputs are passed via VGPRs from VS. */
   if (shader->key.ge.opt.same_patch_vertices && !shader->use_aco) {
      unsigned num_inputs = util_last_bit64(shader->previous_stage_sel->info.outputs_written);
   for (i = 0; i < num_inputs * 4; i++)
               /* For monolithic shaders, the TCS epilog code is generated by
   * ac_nir_lower_hs_outputs_to_mem.
   */
   if (!shader->is_monolithic) {
      /* TCS return values are inputs to the TCS epilog.
   *
   * param_tcs_offchip_offset, param_tcs_factor_offset,
   * param_tcs_offchip_layout, and internal_bindings
   * should be passed to the epilog.
   */
   for (i = 0; i <= 8 + GFX9_SGPR_TCS_OFFCHIP_ADDR; i++)
         for (i = 0; i < 11; i++)
         }
         case SI_SHADER_MERGED_VERTEX_OR_TESSEVAL_GEOMETRY:
      /* Merged stages have 8 system SGPRs at the beginning. */
   /* Gfx9-10: SPI_SHADER_USER_DATA_ADDR_LO/HI_GS */
   /* Gfx11+:  SPI_SHADER_PGM_LO/HI_GS */
            if (shader->key.ge.as_ngg)
         else
            ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.merged_wave_info);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   if (sel->screen->info.gfx_level >= GFX11)
         else
         ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL); /* unused */
            declare_global_desc_pointers(args);
   if (stage != MESA_SHADER_VERTEX || !sel->info.base.vs.blit_sgprs_amd) {
      declare_per_stage_desc_pointers(
               if (stage == MESA_SHADER_VERTEX && sel->info.base.vs.blit_sgprs_amd) {
         } else {
               if (stage == MESA_SHADER_VERTEX) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.base_vertex);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.draw_id);
      } else if (stage == MESA_SHADER_TESS_EVAL) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tes_offchip_addr);
      } else {
      /* GS */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL); /* unused */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL); /* unused */
               ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_CONST_DESC_PTR, &args->small_prim_cull_info);
   if (sel->screen->info.gfx_level >= GFX11)
         else
               /* VGPRs (first GS, then VS/TES) */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[0]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[1]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_prim_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_invocation_id);
            if (stage == MESA_SHADER_VERTEX) {
               /* Need to keep ES/GS arg index same for shared args when ACO,
   * so this is not able to be before shared VGPRs.
   */
   if (!sel->info.base.vs.blit_sgprs_amd)
      } else if (stage == MESA_SHADER_TESS_EVAL) {
                  if (shader->key.ge.as_es && !shader->is_monolithic &&
      (stage == MESA_SHADER_VERTEX || stage == MESA_SHADER_TESS_EVAL)) {
   /* ES return values are inputs to GS. */
   for (i = 0; i < 8 + GFX9_GS_NUM_USER_SGPR; i++)
         for (i = 0; i < 5; i++)
      }
         case MESA_SHADER_TESS_EVAL:
      declare_global_desc_pointers(args);
   declare_per_stage_desc_pointers(args, shader, true);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->vs_state_bits);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
            if (shader->key.ge.as_es) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
      } else {
      declare_streamout_params(args, shader);
               /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (shader->use_aco && sel->screen->info.gfx_level < GFX11)
            /* VGPRs */
   declare_tes_input_vgprs(args);
         case MESA_SHADER_GEOMETRY:
      declare_global_desc_pointers(args);
   declare_per_stage_desc_pointers(args, shader, true);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.gs2vs_offset);
            /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (shader->use_aco && sel->screen->info.gfx_level < GFX11)
            /* VGPRs */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[0]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[1]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_prim_id);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[2]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[3]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[4]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_vtx_offset[5]);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.gs_invocation_id);
         case MESA_SHADER_FRAGMENT:
      declare_global_desc_pointers(args);
   declare_per_stage_desc_pointers(args, shader, true);
   si_add_arg_checked(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->alpha_reference,
         si_add_arg_checked(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.prim_mask,
            si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_sample,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_center,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.persp_centroid,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 3, AC_ARG_INT, NULL, SI_PARAM_PERSP_PULL_MODEL);
   si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_sample,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_center,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 2, AC_ARG_INT, &args->ac.linear_centroid,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, NULL, SI_PARAM_LINE_STIPPLE_TEX);
   si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[0],
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[1],
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[2],
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.frag_pos[3],
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.front_face,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.ancillary,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.sample_coverage,
         si_add_arg_checked(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, &args->ac.pos_fixed_pt,
            if (shader->use_aco) {
               /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (sel->screen->info.gfx_level < GFX11)
               /* Monolithic PS emit prolog and epilog in NIR directly. */
   if (!shader->is_monolithic) {
      /* Color inputs from the prolog. */
                                                /* Outputs for the epilog. */
   num_return_sgprs = SI_SGPR_ALPHA_REF + 1;
   num_returns =
      num_return_sgprs + util_bitcount(shader->selector->info.colors_written) * 4 +
               for (i = 0; i < num_return_sgprs; i++)
         for (; i < num_returns; i++)
      }
         case MESA_SHADER_COMPUTE:
      declare_global_desc_pointers(args);
   declare_per_stage_desc_pointers(args, shader, true);
   if (shader->selector->info.uses_grid_size)
         if (shader->selector->info.uses_variable_block_size)
            unsigned cs_user_data_dwords =
         if (cs_user_data_dwords) {
                  /* Some descriptors can be in user SGPRs. */
   /* Shader buffers in user SGPRs. */
   for (unsigned i = 0; i < shader->selector->cs_num_shaderbufs_in_user_sgprs; i++) {
                        }
   /* Images in user SGPRs. */
   for (unsigned i = 0; i < shader->selector->cs_num_images_in_user_sgprs; i++) {
                                          /* Hardware SGPRs. */
   for (i = 0; i < 3; i++) {
      if (shader->selector->info.uses_block_id[i]) {
            }
   if (shader->selector->info.uses_tg_size)
            /* GFX11 set FLAT_SCRATCH directly instead of using this arg. */
   if (shader->use_aco && sel->screen->info.gfx_level < GFX11)
            /* Hardware VGPRs. */
   /* Thread IDs are packed in VGPR0, 10 bits per component or stored in 3 separate VGPRs */
   if (sel->screen->info.gfx_level >= GFX11 ||
      (!sel->screen->info.has_graphics && sel->screen->info.family >= CHIP_MI200))
      else
            default:
      assert(0 && "unimplemented shader");
               shader->info.num_input_sgprs = args->ac.num_sgprs_used;
            assert(shader->info.num_input_vgprs >= num_prolog_vgprs);
      }
      static unsigned get_lds_granularity(struct si_screen *screen, gl_shader_stage stage)
   {
      return screen->info.gfx_level >= GFX11 && stage == MESA_SHADER_FRAGMENT ? 1024 :
      }
      bool si_shader_binary_open(struct si_screen *screen, struct si_shader *shader,
         {
      const struct si_shader_selector *sel = shader->selector;
   const char *part_elfs[5];
   size_t part_sizes[5];
         #define add_part(shader_or_part)                                                                   \
      if (shader_or_part) {                                                                           \
      part_elfs[num_parts] = (shader_or_part)->binary.code_buffer;                                 \
   part_sizes[num_parts] = (shader_or_part)->binary.code_size;                                  \
               add_part(shader->prolog);
   add_part(shader->previous_stage);
   add_part(shader);
         #undef add_part
         struct ac_rtld_symbol lds_symbols[2];
            if (sel && screen->info.gfx_level >= GFX9 && !shader->is_gs_copy_shader &&
      (sel->stage == MESA_SHADER_GEOMETRY ||
   (sel->stage <= MESA_SHADER_GEOMETRY && shader->key.ge.as_ngg))) {
   struct ac_rtld_symbol *sym = &lds_symbols[num_lds_symbols++];
   sym->name = "esgs_ring";
   sym->size = shader->gs_info.esgs_ring_size * 4;
               if (sel->stage == MESA_SHADER_GEOMETRY && shader->key.ge.as_ngg) {
      struct ac_rtld_symbol *sym = &lds_symbols[num_lds_symbols++];
   sym->name = "ngg_emit";
   sym->size = shader->ngg.ngg_emit_size * 4;
               bool ok = ac_rtld_open(
      rtld, (struct ac_rtld_open_info){.info = &screen->info,
                                    .options =
      {
                     if (rtld->lds_size > 0) {
      unsigned alloc_granularity = get_lds_granularity(screen, sel->stage);
                  }
      static unsigned get_shader_binaries(struct si_shader *shader, struct si_shader_binary *bin[4])
   {
               if (shader->prolog)
            if (shader->previous_stage)
                     if (shader->epilog)
               }
      static unsigned si_get_shader_binary_size(struct si_screen *screen, struct si_shader *shader)
   {
      if (shader->binary.type == SI_SHADER_BINARY_ELF) {
      struct ac_rtld_binary rtld;
   si_shader_binary_open(screen, shader, &rtld);
   uint64_t size = rtld.exec_size;
   ac_rtld_close(&rtld);
      } else {
      struct si_shader_binary *bin[4];
            unsigned size = 0;
   for (unsigned i = 0; i < num_bin; i++) {
      assert(bin[i]->type == SI_SHADER_BINARY_RAW);
      }
         }
      bool si_get_external_symbol(enum amd_gfx_level gfx_level, void *data, const char *name,
         {
               if (!strcmp(scratch_rsrc_dword0_symbol, name)) {
      *value = (uint32_t)*scratch_va;
      }
   if (!strcmp(scratch_rsrc_dword1_symbol, name)) {
      /* Enable scratch coalescing. */
            if (gfx_level >= GFX11)
         else
                        }
      static bool upload_binary_elf(struct si_screen *sscreen, struct si_shader *shader,
         {
      struct ac_rtld_binary binary;
   if (!si_shader_binary_open(sscreen, shader, &binary))
            unsigned rx_size = ac_align_shader_binary_for_prefetch(&sscreen->info, binary.rx_size);
            si_resource_reference(&shader->bo, NULL);
   shader->bo = si_aligned_buffer_create(
      &sscreen->b,
   SI_RESOURCE_FLAG_DRIVER_INTERNAL | SI_RESOURCE_FLAG_32BIT |
   (dma_upload || sscreen->info.cpdma_prefetch_writes_memory ? 0 : SI_RESOURCE_FLAG_READ_ONLY) |
   (dma_upload ? PIPE_RESOURCE_FLAG_UNMAPPABLE : 0),
      if (!shader->bo)
            /* Upload. */
   struct ac_rtld_upload_info u = {};
   u.binary = &binary;
   u.get_external_symbol = si_get_external_symbol;
   u.cb_data = &scratch_va;
            struct si_context *upload_ctx = NULL;
   struct pipe_resource *staging = NULL;
            if (dma_upload) {
      /* First upload into a staging buffer. */
            u_upload_alloc(upload_ctx->b.stream_uploader, 0, binary.rx_size, 256,
         if (!u.rx_ptr) {
      si_put_aux_context_flush(&sscreen->aux_context.shader_upload);
         } else {
      u.rx_ptr = sscreen->ws->buffer_map(sscreen->ws,
      shader->bo->buf, NULL,
      if (!u.rx_ptr)
                        if (sscreen->debug_flags & DBG(SQTT)) {
      /* Remember the uploaded code */
   shader->binary.uploaded_code_size = size;
   shader->binary.uploaded_code = malloc(size);
               if (dma_upload) {
      /* Then copy from the staging buffer to VRAM.
   *
   * We can't use the upload copy in si_buffer_transfer_unmap because that might use
   * a compute shader, and we can't use shaders in the code that is responsible for making
   * them available.
   */
   si_cp_dma_copy_buffer(upload_ctx, &shader->bo->b.b, staging, 0, staging_offset,
                  #if 0 /* debug: validate whether the copy was successful */
         uint32_t *dst_binary = malloc(binary.rx_size);
   uint32_t *src_binary = (uint32_t*)u.rx_ptr;
   pipe_buffer_read(&upload_ctx->b, &shader->bo->b.b, 0, binary.rx_size, dst_binary);
   puts("dst_binary == src_binary:");
   for (unsigned i = 0; i < binary.rx_size / 4; i++) {
         }
   free(dst_binary);
   #endif
            si_put_aux_context_flush(&sscreen->aux_context.shader_upload);
      } else {
                  ac_rtld_close(&binary);
               }
      static void calculate_needed_lds_size(struct si_screen *sscreen, struct si_shader *shader)
   {
      gl_shader_stage stage =
            if (sscreen->info.gfx_level >= GFX9 && stage <= MESA_SHADER_GEOMETRY &&
      (stage == MESA_SHADER_GEOMETRY || shader->key.ge.as_ngg)) {
            if (stage == MESA_SHADER_GEOMETRY && shader->key.ge.as_ngg)
            if (shader->key.ge.as_ngg) {
      unsigned scratch_dw_size = gfx10_ngg_get_scratch_dw_size(shader);
   if (scratch_dw_size) {
      /* scratch base address needs to be 8 byte aligned */
   size_in_dw = ALIGN(size_in_dw, 2);
                  shader->config.lds_size =
         }
      static bool upload_binary_raw(struct si_screen *sscreen, struct si_shader *shader,
         {
      struct si_shader_binary *bin[4];
            unsigned code_size = 0, exec_size = 0;
   for (unsigned i = 0; i < num_bin; i++) {
      assert(bin[i]->type == SI_SHADER_BINARY_RAW);
   code_size += bin[i]->code_size;
                        si_resource_reference(&shader->bo, NULL);
   shader->bo =
      si_aligned_buffer_create(&sscreen->b,
                           (sscreen->info.cpdma_prefetch_writes_memory ?
      if (!shader->bo)
            void *rx_ptr =
      sscreen->ws->buffer_map(sscreen->ws, shader->bo->buf, NULL,
                  if (!rx_ptr)
            unsigned exec_offset = 0, data_offset = exec_size;
   for (unsigned i = 0; i < num_bin; i++) {
               if (bin[i]->num_symbols) {
      /* Offset needed to add to const data symbol because of inserting other
   * shader part between exec code and const data.
                  /* Prolog and epilog have no symbols. */
                  si_aco_resolve_symbols(sh, rx_ptr + exec_offset, (const uint32_t *)bin[i]->code_buffer,
                        unsigned data_size = bin[i]->code_size - bin[i]->exec_size;
   if (data_size) {
      memcpy(rx_ptr + data_offset, bin[i]->code_buffer + bin[i]->exec_size, data_size);
                  sscreen->ws->buffer_unmap(sscreen->ws, shader->bo->buf);
            calculate_needed_lds_size(sscreen, shader);
      }
      bool si_shader_binary_upload(struct si_screen *sscreen, struct si_shader *shader,
         {
      if (shader->binary.type == SI_SHADER_BINARY_ELF) {
         } else {
      assert(shader->binary.type == SI_SHADER_BINARY_RAW);
         }
      static void print_disassembly(const char *disasm, size_t nbytes,
               {
      if (debug && debug->debug_message) {
      /* Very long debug messages are cut off, so send the
   * disassembly one line at a time. This causes more
   * overhead, but on the plus side it simplifies
   * parsing of resulting logs.
   */
            uint64_t line = 0;
   while (line < nbytes) {
      int count = nbytes - line;
   const char *nl = memchr(disasm + line, '\n', nbytes - line);
                  if (count) {
                                          if (file) {
      fprintf(file, "Shader %s disassembly:\n", name);
         }
      static void si_shader_dump_disassembly(struct si_screen *screen,
                           {
      if (binary->type == SI_SHADER_BINARY_RAW) {
      print_disassembly(binary->disasm_string, binary->disasm_size, name, file, debug);
                        if (!ac_rtld_open(&rtld_binary, (struct ac_rtld_open_info){
                                    .info = &screen->info,
         const char *disasm;
            if (!ac_rtld_get_section_by_name(&rtld_binary, ".AMDGPU.disasm", &disasm, &nbytes))
            if (nbytes > INT_MAX)
                  out:
         }
      static void si_calculate_max_simd_waves(struct si_shader *shader)
   {
      struct si_screen *sscreen = shader->selector->screen;
   struct ac_shader_config *conf = &shader->config;
   unsigned num_inputs = shader->selector->info.num_inputs;
   unsigned lds_increment = get_lds_granularity(sscreen, shader->selector->stage);
   unsigned lds_per_wave = 0;
                     /* Compute LDS usage for PS. */
   switch (shader->selector->stage) {
   case MESA_SHADER_FRAGMENT:
      /* The minimum usage per wave is (num_inputs * 48). The maximum
   * usage is (num_inputs * 48 * 16).
   * We can get anything in between and it varies between waves.
   *
   * The 48 bytes per input for a single primitive is equal to
   * 4 bytes/component * 4 components/input * 3 points.
   *
   * Other stages don't know the size at compile time or don't
   * allocate LDS per wave, but instead they do it per thread group.
   */
   lds_per_wave = conf->lds_size * lds_increment + align(num_inputs * 48, lds_increment);
      case MESA_SHADER_COMPUTE: {
         unsigned max_workgroup_size = si_get_max_workgroup_size(shader);
   lds_per_wave = (conf->lds_size * lds_increment) /
      }
      default:;
            /* Compute the per-SIMD wave counts. */
   if (conf->num_sgprs) {
      max_simd_waves =
               if (conf->num_vgprs) {
      /* GFX 10.3 internally:
   * - aligns VGPRS to 16 for Wave32 and 8 for Wave64
   * - aligns LDS to 1024
   *
   * For shader-db stats, set num_vgprs that the hw actually uses.
   */
   unsigned num_vgprs = conf->num_vgprs;
   if (sscreen->info.gfx_level >= GFX10_3) {
      unsigned real_vgpr_gran = sscreen->info.num_physical_wave64_vgprs_per_simd / 64;
      } else {
                  /* Always print wave limits as Wave64, so that we can compare
   * Wave32 and Wave64 with shader-db fairly. */
   unsigned max_vgprs = sscreen->info.num_physical_wave64_vgprs_per_simd;
               unsigned max_lds_per_simd = sscreen->info.lds_size_per_workgroup / 4;
   if (lds_per_wave)
               }
      void si_shader_dump_stats_for_shader_db(struct si_screen *screen, struct si_shader *shader,
         {
      const struct ac_shader_config *conf = &shader->config;
            if (screen->options.debug_disassembly)
      si_shader_dump_disassembly(screen, &shader->binary, shader->selector->stage,
                  if (shader->selector->stage <= MESA_SHADER_GEOMETRY) {
      /* This doesn't include pos exports because only param exports are interesting
   * for performance and can be optimized.
   */
   if (shader->gs_copy_shader)
         else if (shader->key.ge.as_es)
         else if (shader->key.ge.as_ls)
         else if (shader->selector->stage == MESA_SHADER_VERTEX ||
            shader->selector->stage == MESA_SHADER_TESS_EVAL ||
      else if (shader->selector->stage == MESA_SHADER_TESS_CTRL)
         else
      } else if (shader->selector->stage == MESA_SHADER_FRAGMENT) {
      num_outputs = util_bitcount(shader->selector->info.colors_written) +
                           util_debug_message(debug, SHADER_INFO,
                     "Shader Stats: SGPRS: %d VGPRS: %d Code Size: %d "
   "LDS: %d Scratch: %d Max Waves: %d Spilled SGPRs: %d "
   "Spilled VGPRs: %d PrivMem VGPRs: %d Outputs: %u PatchOutputs: %u DivergentLoop: %d "
   "InlineUniforms: %d (%s, W%u)",
   conf->num_sgprs, conf->num_vgprs, si_get_shader_binary_size(screen, shader),
   conf->lds_size, conf->scratch_bytes_per_wave, shader->info.max_simd_waves,
   conf->spilled_sgprs, conf->spilled_vgprs, shader->info.private_mem_vgprs,
   num_outputs,
      }
      bool si_can_dump_shader(struct si_screen *sscreen, gl_shader_stage stage,
         {
      static uint64_t filter[] = {
      [SI_DUMP_SHADER_KEY] = DBG(NIR) | DBG(INIT_LLVM) | DBG(LLVM) | DBG(INIT_ACO) | DBG(ACO) | DBG(ASM),
   [SI_DUMP_INIT_NIR] = DBG(INIT_NIR),
   [SI_DUMP_NIR] = DBG(NIR),
   [SI_DUMP_INIT_LLVM_IR] = DBG(INIT_LLVM),
   [SI_DUMP_LLVM_IR] = DBG(LLVM),
   [SI_DUMP_INIT_ACO_IR] = DBG(INIT_ACO),
   [SI_DUMP_ACO_IR] = DBG(ACO),
   [SI_DUMP_ASM] = DBG(ASM),
   [SI_DUMP_STATS] = DBG(STATS),
      };
            return sscreen->debug_flags & (1 << stage) &&
      }
      static void si_shader_dump_stats(struct si_screen *sscreen, struct si_shader *shader, FILE *file,
         {
               if (shader->selector->stage == MESA_SHADER_FRAGMENT) {
      fprintf(file,
         "*** SHADER CONFIG ***\n"
   "SPI_PS_INPUT_ADDR = 0x%04x\n"
               fprintf(file,
         "*** SHADER STATS ***\n"
   "SGPRS: %d\n"
   "VGPRS: %d\n"
   "Spilled SGPRs: %d\n"
   "Spilled VGPRs: %d\n"
   "Private memory VGPRs: %d\n"
   "Code Size: %d bytes\n"
   "LDS: %d bytes\n"
   "Scratch: %d bytes per wave\n"
   "Max Waves: %d\n"
   "********************\n\n\n",
   conf->num_sgprs, conf->num_vgprs, conf->spilled_sgprs, conf->spilled_vgprs,
   shader->info.private_mem_vgprs, si_get_shader_binary_size(sscreen, shader),
      }
      const char *si_get_shader_name(const struct si_shader *shader)
   {
      switch (shader->selector->stage) {
   case MESA_SHADER_VERTEX:
      if (shader->key.ge.as_es)
         else if (shader->key.ge.as_ls)
         else if (shader->key.ge.as_ngg)
         else
      case MESA_SHADER_TESS_CTRL:
         case MESA_SHADER_TESS_EVAL:
      if (shader->key.ge.as_es)
         else if (shader->key.ge.as_ngg)
         else
      case MESA_SHADER_GEOMETRY:
      if (shader->is_gs_copy_shader)
         else
      case MESA_SHADER_FRAGMENT:
         case MESA_SHADER_COMPUTE:
         default:
            }
      void si_shader_dump(struct si_screen *sscreen, struct si_shader *shader,
         {
               if (!check_debug_option || si_can_dump_shader(sscreen, stage, SI_DUMP_SHADER_KEY))
            if (!check_debug_option && shader->binary.llvm_ir_string) {
      /* This is only used with ddebug. */
   if (shader->previous_stage && shader->previous_stage->binary.llvm_ir_string) {
      fprintf(file, "\n%s - previous stage - LLVM IR:\n\n", si_get_shader_name(shader));
               fprintf(file, "\n%s - main shader part - LLVM IR:\n\n", si_get_shader_name(shader));
               if (!check_debug_option || (si_can_dump_shader(sscreen, stage, SI_DUMP_ASM))) {
               if (shader->prolog)
      si_shader_dump_disassembly(sscreen, &shader->prolog->binary, stage, shader->wave_size, debug,
      if (shader->previous_stage)
      si_shader_dump_disassembly(sscreen, &shader->previous_stage->binary, stage,
      si_shader_dump_disassembly(sscreen, &shader->binary, stage, shader->wave_size, debug, "main",
            if (shader->epilog)
      si_shader_dump_disassembly(sscreen, &shader->epilog->binary, stage, shader->wave_size, debug,
                     }
      static void si_dump_shader_key_vs(const union si_shader_key *key,
               {
      fprintf(f, "  %s.instance_divisor_is_one = %u\n", prefix, prolog->instance_divisor_is_one);
   fprintf(f, "  %s.instance_divisor_is_fetched = %u\n", prefix,
                  fprintf(f, "  mono.vs.fetch_opencode = %x\n", key->ge.mono.vs_fetch_opencode);
   fprintf(f, "  mono.vs.fix_fetch = {");
   for (int i = 0; i < SI_MAX_ATTRIBS; i++) {
      union si_vs_fix_fetch fix = key->ge.mono.vs_fix_fetch[i];
   if (i)
         if (!fix.bits)
         else
      fprintf(f, "%u.%u.%u.%u", fix.u.reverse, fix.u.log_size, fix.u.num_channels_m1,
   }
      }
      static void si_dump_shader_key(const struct si_shader *shader, FILE *f)
   {
      const union si_shader_key *key = &shader->key;
            fprintf(f, "SHADER KEY\n");
   fprintf(f, "  source_sha1 = {");
   _mesa_sha1_print(f, shader->selector->info.base.source_sha1);
            switch (stage) {
   case MESA_SHADER_VERTEX:
      si_dump_shader_key_vs(key, &key->ge.part.vs.prolog, "part.vs.prolog", f);
   fprintf(f, "  as_es = %u\n", key->ge.as_es);
   fprintf(f, "  as_ls = %u\n", key->ge.as_ls);
   fprintf(f, "  as_ngg = %u\n", key->ge.as_ngg);
   fprintf(f, "  mono.u.vs_export_prim_id = %u\n", key->ge.mono.u.vs_export_prim_id);
         case MESA_SHADER_TESS_CTRL:
      if (shader->selector->screen->info.gfx_level >= GFX9) {
         }
   fprintf(f, "  part.tcs.epilog.prim_mode = %u\n", key->ge.part.tcs.epilog.prim_mode);
   fprintf(f, "  opt.prefer_mono = %u\n", key->ge.opt.prefer_mono);
   fprintf(f, "  opt.same_patch_vertices = %u\n", key->ge.opt.same_patch_vertices);
         case MESA_SHADER_TESS_EVAL:
      fprintf(f, "  as_es = %u\n", key->ge.as_es);
   fprintf(f, "  as_ngg = %u\n", key->ge.as_ngg);
   fprintf(f, "  mono.u.vs_export_prim_id = %u\n", key->ge.mono.u.vs_export_prim_id);
         case MESA_SHADER_GEOMETRY:
      if (shader->is_gs_copy_shader)
            if (shader->selector->screen->info.gfx_level >= GFX9 &&
      key->ge.part.gs.es->stage == MESA_SHADER_VERTEX) {
      }
   fprintf(f, "  mono.u.gs_tri_strip_adj_fix = %u\n", key->ge.mono.u.gs_tri_strip_adj_fix);
   fprintf(f, "  as_ngg = %u\n", key->ge.as_ngg);
         case MESA_SHADER_COMPUTE:
            case MESA_SHADER_FRAGMENT:
      fprintf(f, "  prolog.color_two_side = %u\n", key->ps.part.prolog.color_two_side);
   fprintf(f, "  prolog.flatshade_colors = %u\n", key->ps.part.prolog.flatshade_colors);
   fprintf(f, "  prolog.poly_stipple = %u\n", key->ps.part.prolog.poly_stipple);
   fprintf(f, "  prolog.force_persp_sample_interp = %u\n",
         fprintf(f, "  prolog.force_linear_sample_interp = %u\n",
         fprintf(f, "  prolog.force_persp_center_interp = %u\n",
         fprintf(f, "  prolog.force_linear_center_interp = %u\n",
         fprintf(f, "  prolog.bc_optimize_for_persp = %u\n",
         fprintf(f, "  prolog.bc_optimize_for_linear = %u\n",
         fprintf(f, "  prolog.samplemask_log_ps_iter = %u\n",
         fprintf(f, "  epilog.spi_shader_col_format = 0x%x\n",
         fprintf(f, "  epilog.color_is_int8 = 0x%X\n", key->ps.part.epilog.color_is_int8);
   fprintf(f, "  epilog.color_is_int10 = 0x%X\n", key->ps.part.epilog.color_is_int10);
   fprintf(f, "  epilog.last_cbuf = %u\n", key->ps.part.epilog.last_cbuf);
   fprintf(f, "  epilog.alpha_func = %u\n", key->ps.part.epilog.alpha_func);
   fprintf(f, "  epilog.alpha_to_one = %u\n", key->ps.part.epilog.alpha_to_one);
   fprintf(f, "  epilog.alpha_to_coverage_via_mrtz = %u\n", key->ps.part.epilog.alpha_to_coverage_via_mrtz);
   fprintf(f, "  epilog.clamp_color = %u\n", key->ps.part.epilog.clamp_color);
   fprintf(f, "  epilog.dual_src_blend_swizzle = %u\n", key->ps.part.epilog.dual_src_blend_swizzle);
   fprintf(f, "  epilog.rbplus_depth_only_opt = %u\n", key->ps.part.epilog.rbplus_depth_only_opt);
   fprintf(f, "  epilog.kill_samplemask = %u\n", key->ps.part.epilog.kill_samplemask);
   fprintf(f, "  mono.poly_line_smoothing = %u\n", key->ps.mono.poly_line_smoothing);
   fprintf(f, "  mono.point_smoothing = %u\n", key->ps.mono.point_smoothing);
   fprintf(f, "  mono.interpolate_at_sample_force_center = %u\n",
         fprintf(f, "  mono.fbfetch_msaa = %u\n", key->ps.mono.fbfetch_msaa);
   fprintf(f, "  mono.fbfetch_is_1D = %u\n", key->ps.mono.fbfetch_is_1D);
   fprintf(f, "  mono.fbfetch_layered = %u\n", key->ps.mono.fbfetch_layered);
         default:
                  if ((stage == MESA_SHADER_GEOMETRY || stage == MESA_SHADER_TESS_EVAL ||
      stage == MESA_SHADER_VERTEX) &&
   !key->ge.as_es && !key->ge.as_ls) {
   fprintf(f, "  opt.kill_outputs = 0x%" PRIx64 "\n", key->ge.opt.kill_outputs);
   fprintf(f, "  opt.kill_pointsize = 0x%x\n", key->ge.opt.kill_pointsize);
   fprintf(f, "  opt.kill_clip_distances = 0x%x\n", key->ge.opt.kill_clip_distances);
   fprintf(f, "  opt.ngg_culling = 0x%x\n", key->ge.opt.ngg_culling);
               if (stage <= MESA_SHADER_GEOMETRY)
         else
            if (stage <= MESA_SHADER_GEOMETRY) {
      if (key->ge.opt.inline_uniforms) {
      fprintf(f, "  opt.inline_uniforms = %u (0x%x, 0x%x, 0x%x, 0x%x)\n",
         key->ge.opt.inline_uniforms,
   key->ge.opt.inlined_uniform_values[0],
   key->ge.opt.inlined_uniform_values[1],
      } else {
            } else {
      if (key->ps.opt.inline_uniforms) {
      fprintf(f, "  opt.inline_uniforms = %u (0x%x, 0x%x, 0x%x, 0x%x)\n",
         key->ps.opt.inline_uniforms,
   key->ps.opt.inlined_uniform_values[0],
   key->ps.opt.inlined_uniform_values[1],
      } else {
               }
      bool si_vs_needs_prolog(const struct si_shader_selector *sel,
         {
               /* VGPR initialization fixup for Vega10 and Raven is always done in the
   * VS prolog. */
      }
      /**
   * Compute the VS prolog key, which contains all the information needed to
   * build the VS prolog function, and set shader->info bits where needed.
   *
   * \param info             Shader info of the vertex shader.
   * \param num_input_sgprs  Number of input SGPRs for the vertex shader.
   * \param prolog_key       Key of the VS prolog
   * \param shader_out       The vertex shader, or the next shader if merging LS+HS or ES+GS.
   * \param key              Output shader part key.
   */
   void si_get_vs_prolog_key(const struct si_shader_info *info, unsigned num_input_sgprs,
               {
      memset(key, 0, sizeof(*key));
   key->vs_prolog.states = *prolog_key;
   key->vs_prolog.wave32 = shader_out->wave_size == 32;
   key->vs_prolog.num_input_sgprs = num_input_sgprs;
   key->vs_prolog.num_inputs = info->num_inputs;
   key->vs_prolog.as_ls = shader_out->key.ge.as_ls;
   key->vs_prolog.as_es = shader_out->key.ge.as_es;
            if (shader_out->selector->stage == MESA_SHADER_TESS_CTRL) {
      key->vs_prolog.as_ls = 1;
      } else if (shader_out->selector->stage == MESA_SHADER_GEOMETRY) {
      key->vs_prolog.as_es = 1;
      } else if (shader_out->key.ge.as_ngg) {
                  /* Only one of these combinations can be set. as_ngg can be set with as_es. */
   assert(key->vs_prolog.as_ls + key->vs_prolog.as_ngg +
            /* Enable loading the InstanceID VGPR. */
            if ((key->vs_prolog.states.instance_divisor_is_one |
      key->vs_prolog.states.instance_divisor_is_fetched) &
   input_mask)
   }
      /* TODO: convert to nir_shader_instructions_pass */
   static bool si_nir_kill_outputs(nir_shader *nir, const union si_shader_key *key)
   {
      nir_function_impl *impl = nir_shader_get_entrypoint(nir);
   assert(impl);
            /* Always remove the interpolated gl_Layer output for blit shaders on the first compile
   * (it's always unused by PS), otherwise we hang because we don't pass the attribute ring
   * pointer to position-only shaders that also write gl_Layer.
   */
   if (nir->info.stage == MESA_SHADER_VERTEX && nir->info.vs.blit_sgprs_amd)
            if (nir->info.stage > MESA_SHADER_GEOMETRY ||
      (!kill_outputs &&
   !key->ge.opt.kill_pointsize &&
   !key->ge.opt.kill_clip_distances &&
   (nir->info.stage != MESA_SHADER_VERTEX || !nir->info.vs.blit_sgprs_amd))) {
   nir_metadata_preserve(impl, nir_metadata_all);
                        nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  /* No indirect indexing allowed. */
                                 if (nir_slot_is_varying(sem.location) &&
      kill_outputs &
   (1ull << si_shader_io_get_unique_index(sem.location))) {
   nir_remove_varying(intr, MESA_SHADER_FRAGMENT);
               if (key->ge.opt.kill_pointsize && sem.location == VARYING_SLOT_PSIZ) {
      nir_remove_sysval_output(intr);
               /* TODO: We should only kill specific clip planes as required by kill_clip_distance,
   * not whole gl_ClipVertex. Lower ClipVertex in NIR.
   */
   if ((key->ge.opt.kill_clip_distances & SI_USER_CLIP_PLANE_MASK) == SI_USER_CLIP_PLANE_MASK &&
      sem.location == VARYING_SLOT_CLIP_VERTEX) {
   nir_remove_sysval_output(intr);
               if (key->ge.opt.kill_clip_distances &&
      (sem.location == VARYING_SLOT_CLIP_DIST0 ||
   sem.location == VARYING_SLOT_CLIP_DIST1)) {
   assert(nir_intrinsic_src_type(intr) == nir_type_float32);
                  if ((key->ge.opt.kill_clip_distances >> index) & 0x1) {
      nir_remove_sysval_output(intr);
                        if (progress) {
      nir_metadata_preserve(impl, nir_metadata_dominance |
      } else {
                     }
      static bool clamp_vertex_color_instr(nir_builder *b,
         {
      if (intrin->intrinsic != nir_intrinsic_store_output)
            unsigned location = nir_intrinsic_io_semantics(intrin).location;
   if (location != VARYING_SLOT_COL0 && location != VARYING_SLOT_COL1 &&
      location != VARYING_SLOT_BFC0 && location != VARYING_SLOT_BFC1)
         /* no indirect output */
   assert(nir_src_is_const(intrin->src[1]) && !nir_src_as_uint(intrin->src[1]));
   /* only scalar output */
                     nir_def *color = intrin->src[0].ssa;
   nir_def *clamp = nir_load_clamp_vertex_color_amd(b);
   nir_def *new_color = nir_bcsel(b, clamp, nir_fsat(b, color), color);
               }
      static bool si_nir_clamp_vertex_color(nir_shader *nir)
   {
      uint64_t mask = VARYING_BIT_COL0 | VARYING_BIT_COL1 | VARYING_BIT_BFC0 | VARYING_BIT_BFC1;
   if (!(nir->info.outputs_written & mask))
            return nir_shader_intrinsics_pass(nir, clamp_vertex_color_instr,
            }
      static unsigned si_map_io_driver_location(unsigned semantic)
   {
      if ((semantic >= VARYING_SLOT_PATCH0 && semantic < VARYING_SLOT_TESS_MAX) ||
      semantic == VARYING_SLOT_TESS_LEVEL_INNER ||
   semantic == VARYING_SLOT_TESS_LEVEL_OUTER)
            }
      static bool si_lower_io_to_mem(struct si_shader *shader, nir_shader *nir,
         {
      struct si_shader_selector *sel = shader->selector;
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      if (key->ge.as_ls) {
      NIR_PASS_V(nir, ac_nir_lower_ls_outputs_to_mem, si_map_io_driver_location,
            } else if (key->ge.as_es) {
      NIR_PASS_V(nir, ac_nir_lower_es_outputs_to_mem, si_map_io_driver_location,
               } else if (nir->info.stage == MESA_SHADER_TESS_CTRL) {
      NIR_PASS_V(nir, ac_nir_lower_hs_inputs_to_mem, si_map_io_driver_location,
            /* Used by hs_emit_write_tess_factors() when monolithic shader. */
            NIR_PASS_V(nir, ac_nir_lower_hs_outputs_to_mem, si_map_io_driver_location,
            sel->screen->info.gfx_level,
   /* Used by hs_emit_write_tess_factors() when monolithic shader. */
   key->ge.part.tcs.epilog.tes_reads_tess_factors,
   ~0ULL, ~0ULL, /* no TES inputs filter */
   util_last_bit64(sel->info.outputs_written),
   util_last_bit64(sel->info.patch_outputs_written),
   shader->wave_size,
   /* ALL TCS inputs are passed by register. */
   key->ge.opt.same_patch_vertices &&
   !(sel->info.base.inputs_read & ~sel->info.tcs_vgpr_only_inputs),
   sel->info.tessfactors_are_def_in_all_invocs,
         } else if (nir->info.stage == MESA_SHADER_TESS_EVAL) {
               if (key->ge.as_es) {
      NIR_PASS_V(nir, ac_nir_lower_es_outputs_to_mem, si_map_io_driver_location,
                  } else if (nir->info.stage == MESA_SHADER_GEOMETRY) {
      NIR_PASS_V(nir, ac_nir_lower_gs_inputs_to_mem, si_map_io_driver_location,
                        }
      static void si_lower_ngg(struct si_shader *shader, nir_shader *nir)
   {
      struct si_shader_selector *sel = shader->selector;
   const union si_shader_key *key = &shader->key;
            unsigned clipdist_mask =
      (sel->info.clipdist_mask & ~key->ge.opt.kill_clip_distances) |
         ac_nir_lower_ngg_options options = {
      .family = sel->screen->info.family,
   .gfx_level = sel->screen->info.gfx_level,
   .max_workgroup_size = si_get_max_workgroup_size(shader),
   .wave_size = shader->wave_size,
   .can_cull = !!key->ge.opt.ngg_culling,
   .disable_streamout = key->ge.opt.remove_streamout,
   .vs_output_param_offset = shader->info.vs_output_param_offset,
   .has_param_exports = shader->info.nr_param_exports,
   .clipdist_enable_mask = clipdist_mask,
   .kill_pointsize = key->ge.opt.kill_pointsize,
               if (nir->info.stage == MESA_SHADER_VERTEX ||
      nir->info.stage == MESA_SHADER_TESS_EVAL) {
   /* Per instance inputs, used to remove instance load after culling. */
            if (nir->info.stage == MESA_SHADER_VERTEX) {
      instance_rate_inputs =
                  /* Manually mark the instance ID used, so the shader can repack it. */
   if (instance_rate_inputs)
      } else {
      /* Manually mark the primitive ID used, so the shader can repack it. */
   if (key->ge.mono.u.vs_export_prim_id)
               unsigned clip_plane_enable =
            options.num_vertices_per_primitive = gfx10_ngg_get_vertices_per_prim(shader);
   options.early_prim_export = gfx10_ngg_export_prim_early(shader);
   options.passthrough = gfx10_is_ngg_passthrough(shader);
   options.use_edgeflags = gfx10_edgeflags_have_effect(shader);
   options.has_gen_prim_query = options.has_xfb_prim_query =
         options.export_primitive_id = key->ge.mono.u.vs_export_prim_id;
   options.instance_rate_inputs = instance_rate_inputs;
               } else {
               options.gs_out_vtx_bytes = sel->info.gsvs_vertex_size;
   options.has_gen_prim_query = options.has_xfb_prim_query =
                        /* may generate some subgroup op like ballot */
            /* may generate some vector output store */
      }
      struct nir_shader *si_deserialize_shader(struct si_shader_selector *sel)
   {
      struct pipe_screen *screen = &sel->screen->b;
   const void *options = screen->get_compiler_options(screen, PIPE_SHADER_IR_NIR,
            struct blob_reader blob_reader;
   blob_reader_init(&blob_reader, sel->nir_binary, sel->nir_size);
      }
      static void si_nir_assign_param_offsets(nir_shader *nir, struct si_shader *shader,
         {
      struct si_shader_selector *sel = shader->selector;
            uint64_t outputs_written = 0;
            nir_function_impl *impl = nir_shader_get_entrypoint(nir);
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  /* No indirect indexing allowed. */
                                 if (sem.location >= VARYING_SLOT_VAR0_16BIT)
                        /* Assign the param index if it's unassigned. */
   if (nir_slot_is_varying(sem.location) && !sem.no_varying &&
      (sem.gs_streams & 0x3) == 0 &&
   info->vs_output_param_offset[sem.location] == AC_EXP_PARAM_DEFAULT_VAL_0000) {
   /* The semantic and the base should be the same as in si_shader_info. */
   assert(sem.location == sel->info.output_semantic[nir_intrinsic_base(intr)]);
                                    /* Duplicated outputs are redirected here. */
   for (unsigned i = 0; i < NUM_TOTAL_VARYING_SLOTS; i++) {
      if (slot_remap[i] >= 0)
               if (shader->key.ge.mono.u.vs_export_prim_id) {
                  /* Update outputs written info, we may remove some outputs before. */
   nir->info.outputs_written = outputs_written;
      }
      static void si_assign_param_offsets(nir_shader *nir, struct si_shader *shader)
   {
      /* Initialize this first. */
            STATIC_ASSERT(sizeof(shader->info.vs_output_param_offset[0]) == 1);
   memset(shader->info.vs_output_param_offset, AC_EXP_PARAM_DEFAULT_VAL_0000,
            /* A slot remapping table for duplicated outputs, so that 1 vertex shader output can be
   * mapped to multiple fragment shader inputs.
   */
   int8_t slot_remap[NUM_TOTAL_VARYING_SLOTS];
            /* This sets DEFAULT_VAL for constant outputs in vs_output_param_offset. */
   /* TODO: This doesn't affect GS. */
   NIR_PASS_V(nir, ac_nir_optimize_outputs, false, slot_remap,
            /* Assign the non-constant outputs. */
   /* TODO: Use this for the GS copy shader too. */
      }
      static unsigned si_get_nr_pos_exports(const struct si_shader_selector *sel,
         {
               /* Must have a position export. */
            if ((info->writes_psize && !key->ge.opt.kill_pointsize) ||
      (info->writes_edgeflag && !key->ge.as_ngg) ||
   info->writes_viewport_index || info->writes_layer ||
   sel->screen->options.vrs2x2) {
               unsigned clipdist_mask =
            for (int i = 0; i < 2; i++) {
      if (clipdist_mask & BITFIELD_RANGE(i * 4, 4))
                  }
      static bool lower_ps_load_color_intrinsic(nir_builder *b, nir_instr *instr, void *state)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     if (intrin->intrinsic != nir_intrinsic_load_color0 &&
      intrin->intrinsic != nir_intrinsic_load_color1)
         unsigned index = intrin->intrinsic == nir_intrinsic_load_color0 ? 0 : 1;
                     nir_instr_remove(&intrin->instr);
      }
      static void si_nir_lower_ps_color_input(nir_shader *nir, struct si_shader *shader)
   {
               nir_builder builder = nir_builder_at(nir_before_impl(impl));
            const struct si_shader_selector *sel = shader->selector;
            /* Build ready to be used colors at the beginning of the shader. */
   nir_def *colors[2] = {0};
   for (int i = 0; i < 2; i++) {
      if (!(sel->info.colors_read & (0xf << (i * 4))))
            unsigned color_base = sel->info.color_attr_index[i];
   /* If BCOLOR0 is used, BCOLOR1 is at offset "num_inputs + 1",
   * otherwise it's at offset "num_inputs".
   */
   unsigned back_color_base = sel->info.num_inputs;
   if (i == 1 && (sel->info.colors_read & 0xf))
            enum glsl_interp_mode interp_mode = sel->info.color_interpolate[i];
   if (interp_mode == INTERP_MODE_COLOR) {
      interp_mode = key->ps.part.prolog.flatshade_colors ?
               nir_def *back_color = NULL;
   if (interp_mode == INTERP_MODE_FLAT) {
                     if (key->ps.part.prolog.color_two_side) {
      back_color = nir_load_input(b, 4, 32, nir_imm_int(b, 0),
         } else {
      nir_intrinsic_op op = 0;
   switch (sel->info.color_interpolate_loc[i]) {
   case TGSI_INTERPOLATE_LOC_CENTER:
      op = nir_intrinsic_load_barycentric_pixel;
      case TGSI_INTERPOLATE_LOC_CENTROID:
      op = nir_intrinsic_load_barycentric_centroid;
      case TGSI_INTERPOLATE_LOC_SAMPLE:
      op = nir_intrinsic_load_barycentric_sample;
      default:
      unreachable("invalid color interpolate location");
                        colors[i] =
                  if (key->ps.part.prolog.color_two_side) {
      back_color =
      nir_load_interpolated_input(b, 4, 32, barycentric, nir_imm_int(b, 0),
               if (back_color) {
      nir_def *is_front_face = nir_load_front_face(b, 1);
                  /* lower nir_load_color0/1 to use the color value. */
   nir_shader_instructions_pass(nir, lower_ps_load_color_intrinsic,
            }
      static void si_nir_emit_polygon_stipple(nir_shader *nir, struct si_shader_args *args)
   {
               nir_builder builder = nir_builder_at(nir_before_impl(impl));
            /* Load the buffer descriptor. */
   nir_def *desc =
            /* Use the fixed-point gl_FragCoord input.
   * Since the stipple pattern is 32x32 and it repeats, just get 5 bits
   * per coordinate to get the repeating effect.
   */
   nir_def *pos_x = ac_nir_unpack_arg(b, &args->ac, args->ac.pos_fixed_pt, 0, 5);
            nir_def *zero = nir_imm_int(b, 0);
   /* The stipple pattern is 32x32, each row has 32 bits. */
   nir_def *offset = nir_ishl_imm(b, pos_y, 2);
   nir_def *row = nir_load_buffer_amd(b, 1, 32, desc, offset, zero, zero);
            nir_def *pass = nir_i2b(b, bit);
      }
      struct nir_shader *si_get_nir_shader(struct si_shader *shader,
                           {
      struct si_shader_selector *sel = shader->selector;
            nir_shader *nir;
            if (sel->nir) {
         } else if (sel->nir_binary) {
      nir = si_deserialize_shader(sel);
      } else {
                           const char *original_name = NULL;
   if (unlikely(should_print_nir(nir))) {
      /* Modify the shader's name so that each variant gets its own name. */
   original_name = ralloc_strdup(nir, nir->info.name);
            /* Dummy pass to get the starting point. */
   printf("nir_dummy_pass\n");
               /* Kill outputs according to the shader key. */
   if (sel->stage <= MESA_SHADER_GEOMETRY)
            NIR_PASS(
      _, nir, ac_nir_lower_tex,
   &(ac_nir_lower_tex_options){
      .gfx_level = sel->screen->info.gfx_level,
            if (nir->info.uses_resource_info_query)
            bool inline_uniforms = false;
   uint32_t *inlined_uniform_values;
   si_get_inline_uniform_state((union si_shader_key*)key, sel->pipe_shader_type,
            if (inline_uniforms) {
               /* Most places use shader information from the default variant, not
   * the optimized variant. These are the things that the driver looks at
   * in optimized variants and the list of things that we need to do.
   *
   * The driver takes into account these things if they suddenly disappear
   * from the shader code:
   * - Register usage and code size decrease (obvious)
   * - Eliminated PS system values are disabled by LLVM
   *   (FragCoord, FrontFace, barycentrics)
   * - VS/TES/GS outputs feeding PS are eliminated if outputs are undef.
   *   The storage for eliminated outputs is also not allocated.
   * - VS/TCS/TES/GS/PS input loads are eliminated (VS relies on DCE in LLVM)
   * - TCS output stores are eliminated
   *
   * TODO: These are things the driver ignores in the final shader code
   * and relies on the default shader info.
   * - Other system values are not eliminated
   * - PS.NUM_INTERP = bitcount64(inputs_read), renumber inputs
   *   to remove holes
   * - uses_discard - if it changed to false
   * - writes_memory - if it changed to false
   * - VS->TCS, VS->GS, TES->GS output stores for the former stage are not
   *   eliminated
   * - Eliminated VS/TCS/TES outputs are still allocated. (except when feeding PS)
   *   GS outputs are eliminated except for the temporary LDS.
   *   Clip distances, gl_PointSize, and PS outputs are eliminated based
   *   on current states, so we don't care about the shader code.
   *
   * TODO: Merged shaders don't inline uniforms for the first stage.
   * VS-GS: only GS inlines uniforms; VS-TCS: only TCS; TES-GS: only GS.
   * (key == NULL for the first stage here)
   *
   * TODO: Compute shaders don't support inlinable uniforms, because they
   * don't have shader variants.
   *
   * TODO: The driver uses a linear search to find a shader variant. This
   * can be really slow if we get too many variants due to uniform inlining.
   */
   NIR_PASS_V(nir, nir_inline_uniforms,
            nir->info.num_inlinable_uniforms,
                  if (sel->stage == MESA_SHADER_FRAGMENT && key->ps.mono.poly_line_smoothing)
            if (sel->stage == MESA_SHADER_FRAGMENT && key->ps.mono.point_smoothing)
            /* This must be before si_nir_lower_resource. */
   if (!sel->screen->info.has_image_opcodes)
            /* LLVM does not work well with this, so is handled in llvm backend waterfall. */
   if (shader->use_aco && sel->info.has_non_uniform_tex_access) {
      nir_lower_non_uniform_access_options options = {
         };
                        bool is_last_vgt_stage =
      (sel->stage == MESA_SHADER_VERTEX ||
   sel->stage == MESA_SHADER_TESS_EVAL ||
   (sel->stage == MESA_SHADER_GEOMETRY && shader->key.ge.as_ngg)) &&
         /* Legacy GS is not last VGT stage because it has GS copy shader. */
            if (is_last_vgt_stage || is_legacy_gs)
            if (progress)
            /* Lower large variables that are always constant with load_constant intrinsics, which
   * get turned into PC-relative loads from a data section next to the shader.
   *
   * Loop unrolling caused by uniform inlining can help eliminate indirect indexing, so
   * this should be done after that.
   *
   * The pass crashes if there are dead temps of lowered IO interface types, so remove
   * them first.
   */
   bool progress2 = false;
   NIR_PASS_V(nir, nir_remove_dead_variables, nir_var_function_temp, NULL);
            /* Loop unrolling caused by uniform inlining can help eliminate indirect indexing, so
   * this should be done after that.
   */
            if (sel->stage == MESA_SHADER_VERTEX)
                     if (is_last_vgt_stage) {
      /* Assign param export indices. */
            /* Assign num of position exports. */
            if (key->ge.as_ngg) {
      /* Lower last VGT NGG shader stage. */
   si_lower_ngg(shader, nir);
      } else if (sel->stage == MESA_SHADER_VERTEX || sel->stage == MESA_SHADER_TESS_EVAL) {
      /* Lower last VGT none-NGG VS/TES shader stage. */
   unsigned clip_cull_mask =
                  NIR_PASS_V(nir, ac_nir_lower_legacy_vs,
            sel->screen->info.gfx_level,
   clip_cull_mask,
   shader->info.vs_output_param_offset,
   shader->info.nr_param_exports,
   shader->key.ge.mono.u.vs_export_prim_id,
   key->ge.opt.remove_streamout,
      } else if (is_legacy_gs) {
         } else if (sel->stage == MESA_SHADER_FRAGMENT && shader->is_monolithic) {
      /* two-side color selection and interpolation */
   if (sel->info.colors_read)
            ac_nir_lower_ps_options options = {
      .gfx_level = sel->screen->info.gfx_level,
   .family = sel->screen->info.family,
   .use_aco = shader->use_aco,
   .uses_discard = si_shader_uses_discard(shader),
   .alpha_to_coverage_via_mrtz = key->ps.part.epilog.alpha_to_coverage_via_mrtz,
   .dual_src_blend_swizzle = key->ps.part.epilog.dual_src_blend_swizzle,
   .spi_shader_col_format = key->ps.part.epilog.spi_shader_col_format,
   .color_is_int8 = key->ps.part.epilog.color_is_int8,
   .color_is_int10 = key->ps.part.epilog.color_is_int10,
   .clamp_color = key->ps.part.epilog.clamp_color,
   .alpha_to_one = key->ps.part.epilog.alpha_to_one,
   .alpha_func = key->ps.part.epilog.alpha_func,
                  .bc_optimize_for_persp = key->ps.part.prolog.bc_optimize_for_persp,
   .bc_optimize_for_linear = key->ps.part.prolog.bc_optimize_for_linear,
   .force_persp_sample_interp = key->ps.part.prolog.force_persp_sample_interp,
   .force_linear_sample_interp = key->ps.part.prolog.force_linear_sample_interp,
   .force_persp_center_interp = key->ps.part.prolog.force_persp_center_interp,
   .force_linear_center_interp = key->ps.part.prolog.force_linear_center_interp,
                        if (key->ps.part.prolog.poly_stipple)
                        NIR_PASS(progress2, nir, nir_opt_idiv_const, 8);
   NIR_PASS(progress2, nir, nir_lower_idiv,
            &(nir_lower_idiv_options){
         NIR_PASS(progress2, nir, ac_nir_lower_intrinsics_to_args, sel->screen->info.gfx_level,
                        if (progress2 || opt_offsets)
            if (opt_offsets) {
      static const nir_opt_offsets_options offset_options = {
      .uniform_max = 0,
   .buffer_max = ~0,
      };
               if (progress || progress2 || opt_offsets)
            /* aco only accept scalar const, must be done after si_nir_late_opts()
   * which may generate vec const.
   */
   if (shader->use_aco)
            /* This helps LLVM form VMEM clauses and thus get more GPU cache hits.
   * 200 is tuned for Viewperf. It should be done last.
   */
            if (unlikely(original_name)) {
      ralloc_free((void*)nir->info.name);
                  }
      void si_update_shader_binary_info(struct si_shader *shader, nir_shader *nir)
   {
      struct si_shader_info info;
            shader->info.uses_vmem_load_other |= info.uses_vmem_load_other;
      }
      static void si_determine_use_aco(struct si_shader *shader)
   {
               if (!(sel->screen->debug_flags & DBG(USE_ACO)))
            /* ACO does not support compute cards yet. */
   if (!sel->screen->info.has_graphics)
            switch (sel->stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
   case MESA_SHADER_GEOMETRY:
      shader->use_aco =
      !si_is_multi_part_shader(shader) || shader->is_monolithic ||
         case MESA_SHADER_FRAGMENT:
   case MESA_SHADER_COMPUTE:
      shader->use_aco = true;
      default:
            }
      /* Generate code for the hardware VS shader stage to go with a geometry shader */
   static struct si_shader *
   si_nir_generate_gs_copy_shader(struct si_screen *sscreen,
                                 {
      struct si_shader *shader;
   struct si_shader_selector *gs_selector = gs_shader->selector;
   struct si_shader_info *gsinfo = &gs_selector->info;
            shader = CALLOC_STRUCT(si_shader);
   if (!shader)
            /* We can leave the fence as permanently signaled because the GS copy
   * shader only becomes visible globally after it has been compiled. */
            shader->selector = gs_selector;
   shader->is_gs_copy_shader = true;
            STATIC_ASSERT(sizeof(shader->info.vs_output_param_offset[0]) == 1);
   memset(shader->info.vs_output_param_offset, AC_EXP_PARAM_DEFAULT_VAL_0000,
            for (unsigned i = 0; i < gsinfo->num_outputs; i++) {
               /* Skip if no channel writes to stream 0. */
   if (!nir_slot_is_varying(semantic) ||
      (gsinfo->output_streams[i] & 0x03 &&
   gsinfo->output_streams[i] & 0x0c &&
   gsinfo->output_streams[i] & 0x30 &&
                                    unsigned clip_cull_mask =
            nir_shader *nir =
      ac_nir_create_gs_copy_shader(gs_nir,
                              sscreen->info.gfx_level,
   clip_cull_mask,
                     struct si_shader_args args;
            NIR_PASS_V(nir, ac_nir_lower_intrinsics_to_args, sscreen->info.gfx_level, AC_HW_VERTEX_SHADER, &args.ac);
                     /* aco only accept scalar const */
   if (shader->use_aco)
            if (si_can_dump_shader(sscreen, MESA_SHADER_GEOMETRY, SI_DUMP_NIR)) {
      fprintf(stderr, "GS Copy Shader:\n");
               bool ok = shader->use_aco ?
      si_aco_compile_shader(shader, &args, nir, debug) :
         if (ok) {
      assert(!shader->config.scratch_bytes_per_wave);
   ok = si_shader_binary_upload(sscreen, shader, 0);
      }
            if (!ok) {
      FREE(shader);
      } else {
         }
      }
      struct si_gs_output_info {
      uint8_t streams[64];
   uint8_t streams_16bit_lo[16];
            uint8_t usage_mask[64];
   uint8_t usage_mask_16bit_lo[16];
               };
      static void
   si_init_gs_output_info(struct si_shader_info *info, struct si_gs_output_info *out_info)
   {
      for (int i = 0; i < info->num_outputs; i++) {
      unsigned slot = info->output_semantic[i];
   if (slot < VARYING_SLOT_VAR0_16BIT) {
      out_info->streams[slot] = info->output_streams[i];
      } else {
      unsigned index = slot - VARYING_SLOT_VAR0_16BIT;
   /* TODO: 16bit need separated fields for lo/hi part. */
   out_info->streams_16bit_lo[index] = info->output_streams[i];
   out_info->streams_16bit_hi[index] = info->output_streams[i];
   out_info->usage_mask_16bit_lo[index] = info->output_usagemask[i];
                           ac_info->streams = out_info->streams;
   ac_info->streams_16bit_lo = out_info->streams_16bit_lo;
            ac_info->usage_mask = out_info->usage_mask;
   ac_info->usage_mask_16bit_lo = out_info->usage_mask_16bit_lo;
            /* TODO: construct 16bit slot per component store type. */
      }
      static void si_fixup_spi_ps_input_config(struct si_shader *shader)
   {
               /* Enable POS_FIXED_PT if polygon stippling is enabled. */
   if (key->ps.part.prolog.poly_stipple)
            /* Set up the enable bits for per-sample shading if needed. */
   if (key->ps.part.prolog.force_persp_sample_interp &&
      (G_0286CC_PERSP_CENTER_ENA(shader->config.spi_ps_input_ena) ||
   G_0286CC_PERSP_CENTROID_ENA(shader->config.spi_ps_input_ena))) {
   shader->config.spi_ps_input_ena &= C_0286CC_PERSP_CENTER_ENA;
   shader->config.spi_ps_input_ena &= C_0286CC_PERSP_CENTROID_ENA;
      }
   if (key->ps.part.prolog.force_linear_sample_interp &&
      (G_0286CC_LINEAR_CENTER_ENA(shader->config.spi_ps_input_ena) ||
   G_0286CC_LINEAR_CENTROID_ENA(shader->config.spi_ps_input_ena))) {
   shader->config.spi_ps_input_ena &= C_0286CC_LINEAR_CENTER_ENA;
   shader->config.spi_ps_input_ena &= C_0286CC_LINEAR_CENTROID_ENA;
      }
   if (key->ps.part.prolog.force_persp_center_interp &&
      (G_0286CC_PERSP_SAMPLE_ENA(shader->config.spi_ps_input_ena) ||
   G_0286CC_PERSP_CENTROID_ENA(shader->config.spi_ps_input_ena))) {
   shader->config.spi_ps_input_ena &= C_0286CC_PERSP_SAMPLE_ENA;
   shader->config.spi_ps_input_ena &= C_0286CC_PERSP_CENTROID_ENA;
      }
   if (key->ps.part.prolog.force_linear_center_interp &&
      (G_0286CC_LINEAR_SAMPLE_ENA(shader->config.spi_ps_input_ena) ||
   G_0286CC_LINEAR_CENTROID_ENA(shader->config.spi_ps_input_ena))) {
   shader->config.spi_ps_input_ena &= C_0286CC_LINEAR_SAMPLE_ENA;
   shader->config.spi_ps_input_ena &= C_0286CC_LINEAR_CENTROID_ENA;
               /* POW_W_FLOAT requires that one of the perspective weights is enabled. */
   if (G_0286CC_POS_W_FLOAT_ENA(shader->config.spi_ps_input_ena) &&
      !(shader->config.spi_ps_input_ena & 0xf)) {
               /* At least one pair of interpolation weights must be enabled. */
   if (!(shader->config.spi_ps_input_ena & 0x7f))
            /* Samplemask fixup requires the sample ID. */
   if (key->ps.part.prolog.samplemask_log_ps_iter)
      }
      static void
   si_set_spi_ps_input_config(struct si_shader *shader)
   {
      const struct si_shader_selector *sel = shader->selector;
   const struct si_shader_info *info = &sel->info;
            shader->config.spi_ps_input_ena =
      S_0286CC_PERSP_CENTER_ENA(info->uses_persp_center) |
   S_0286CC_PERSP_CENTROID_ENA(info->uses_persp_centroid) |
   S_0286CC_PERSP_SAMPLE_ENA(info->uses_persp_sample) |
   S_0286CC_LINEAR_CENTER_ENA(info->uses_linear_center) |
   S_0286CC_LINEAR_CENTROID_ENA(info->uses_linear_centroid) |
   S_0286CC_LINEAR_SAMPLE_ENA(info->uses_linear_sample) |
   S_0286CC_FRONT_FACE_ENA(info->uses_frontface) |
   S_0286CC_SAMPLE_COVERAGE_ENA(info->reads_samplemask) |
         uint8_t mask = info->reads_frag_coord_mask | info->reads_sample_pos_mask;
   u_foreach_bit(i, mask) {
                  if (key->ps.part.prolog.color_two_side)
            /* INTERP_MODE_COLOR, same as SMOOTH if flat shading is disabled. */
   if (info->uses_interp_color && !key->ps.part.prolog.flatshade_colors) {
      shader->config.spi_ps_input_ena |=
      S_0286CC_PERSP_SAMPLE_ENA(info->uses_persp_sample_color) |
   S_0286CC_PERSP_CENTER_ENA(info->uses_persp_center_color) |
            /* nir_lower_poly_line_smooth use nir_load_sample_mask_in */
   if (key->ps.mono.poly_line_smoothing)
            /* nir_lower_point_smooth use nir_load_point_coord_maybe_flipped which is lowered
   * to nir_load_barycentric_pixel and nir_load_interpolated_input.
   */
   if (key->ps.mono.point_smoothing)
            if (shader->is_monolithic) {
      si_fixup_spi_ps_input_config(shader);
      } else {
      /* Part mode will call si_fixup_spi_ps_input_config() when combining multi
   * shader part in si_shader_select_ps_parts().
   *
   * Reserve register locations for VGPR inputs the PS prolog may need.
   */
   shader->config.spi_ps_input_addr =
      shader->config.spi_ps_input_ena |
      }
      static void
   debug_message_stderr(void *data, unsigned *id, enum util_debug_type ptype,
         {
      vfprintf(stderr, fmt, args);
      }
      bool si_compile_shader(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
      bool ret = true;
                     /* ACO need spi_ps_input in advance to init args and used in compiler. */
   if (sel->stage == MESA_SHADER_FRAGMENT && shader->use_aco)
            /* We need this info only when legacy GS. */
   struct si_gs_output_info legacy_gs_output_info;
   if (sel->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg) {
      memset(&legacy_gs_output_info, 0, sizeof(legacy_gs_output_info));
               struct si_shader_args args;
            bool free_nir;
   struct nir_shader *nir =
            /* Dump NIR before doing NIR->LLVM conversion in case the
   * conversion fails. */
   if (si_can_dump_shader(sscreen, sel->stage, SI_DUMP_NIR)) {
               if (nir->xfb_info)
               /* Initialize vs_output_ps_input_cntl to default. */
   for (unsigned i = 0; i < ARRAY_SIZE(shader->info.vs_output_ps_input_cntl); i++)
                           /* uses_instanceid may be set by si_nir_lower_vs_inputs(). */
   shader->info.uses_instanceid |= sel->info.uses_instanceid;
            /* Set the FP ALU behavior. */
   /* By default, we disable denormals for FP32 and enable them for FP16 and FP64
   * for performance and correctness reasons. FP32 denormals can't be enabled because
   * they break output modifiers and v_mad_f32 and are very slow on GFX6-7.
   *
   * float_controls_execution_mode defines the set of valid behaviors. Contradicting flags
   * can be set simultaneously, which means we are allowed to choose, but not really because
   * some options cause GLCTS failures.
   */
            if (!(nir->info.float_controls_execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP32) &&
      nir->info.float_controls_execution_mode & FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP32)
         if (!(nir->info.float_controls_execution_mode & (FLOAT_CONTROLS_ROUNDING_MODE_RTE_FP16 |
            nir->info.float_controls_execution_mode & (FLOAT_CONTROLS_ROUNDING_MODE_RTZ_FP16 |
               if (!(nir->info.float_controls_execution_mode & (FLOAT_CONTROLS_DENORM_PRESERVE_FP16 |
            nir->info.float_controls_execution_mode & (FLOAT_CONTROLS_DENORM_FLUSH_TO_ZERO_FP16 |
               ret = shader->use_aco ?
      si_aco_compile_shader(shader, &args, nir, debug) :
      if (!ret)
                     /* The GS copy shader is compiled next. */
   if (sel->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg) {
      shader->gs_copy_shader =
      si_nir_generate_gs_copy_shader(sscreen, compiler, shader, nir, debug,
      if (!shader->gs_copy_shader) {
      fprintf(stderr, "radeonsi: can't create GS copy shader\n");
   ret = false;
                  /* Compute vs_output_ps_input_cntl. */
   if ((sel->stage == MESA_SHADER_VERTEX ||
      sel->stage == MESA_SHADER_TESS_EVAL ||
   sel->stage == MESA_SHADER_GEOMETRY) &&
   !shader->key.ge.as_ls && !shader->key.ge.as_es) {
            if (sel->stage == MESA_SHADER_GEOMETRY && !shader->key.ge.as_ngg)
            /* We must use the original shader info before the removal of duplicated shader outputs. */
   /* VS and TES should also set primitive ID output if it's used. */
   unsigned num_outputs_with_prim_id = sel->info.num_outputs +
            for (unsigned i = 0; i < num_outputs_with_prim_id; i++) {
      unsigned semantic = sel->info.output_semantic[i];
                  if (offset <= AC_EXP_PARAM_OFFSET_31) {
      /* The input is loaded from parameter memory. */
      } else {
      /* The input is a DEFAULT_VAL constant. */
   assert(offset >= AC_EXP_PARAM_DEFAULT_VAL_0000 &&
                  /* OFFSET=0x20 means that DEFAULT_VAL is used. */
   ps_input_cntl = S_028644_OFFSET(0x20) |
                              /* Validate SGPR and VGPR usage for compute to detect compiler bugs. */
   if (sel->stage == MESA_SHADER_COMPUTE) {
      unsigned max_vgprs =
         unsigned max_sgprs = sscreen->info.num_physical_sgprs_per_simd;
   unsigned max_sgprs_per_wave = 128;
   unsigned simds_per_tg = 4; /* assuming WGP mode on gfx10 */
   unsigned threads_per_tg = si_get_max_workgroup_size(shader);
   unsigned waves_per_tg = DIV_ROUND_UP(threads_per_tg, shader->wave_size);
            max_vgprs = max_vgprs / waves_per_simd;
            if (shader->config.num_sgprs > max_sgprs || shader->config.num_vgprs > max_vgprs) {
      fprintf(stderr,
                        /* Just terminate the process, because dependent
   * shaders can hang due to bad input data, but use
   * the env var to allow shader-db to work.
   */
   if (!debug_get_bool_option("SI_PASS_BAD_SHADERS", false))
                  /* Add/remove the scratch offset to/from input SGPRs. */
   if (sel->screen->info.gfx_level < GFX11 &&
      (sel->screen->info.family < CHIP_GFX940 || sel->screen->info.has_graphics) &&
   !si_is_merged_shader(shader)) {
   if (shader->use_aco) {
      /* When aco scratch_offset arg is added explicitly at the beginning.
   * After compile if no scratch used, reduce the input sgpr count.
   */
   if (!shader->config.scratch_bytes_per_wave)
      } else {
      /* scratch_offset arg is added by llvm implicitly */
   if (shader->info.num_input_sgprs)
                  /* Calculate the number of fragment input VGPRs. */
   if (sel->stage == MESA_SHADER_FRAGMENT) {
      shader->info.num_input_vgprs = ac_get_fs_input_vgpr_cnt(
                        if (si_can_dump_shader(sscreen, sel->stage, SI_DUMP_STATS)) {
      struct util_debug_callback out_stderr = {
                     } else {
               out:
      if (free_nir)
               }
      /**
   * Create, compile and return a shader part (prolog or epilog).
   *
   * \param sscreen  screen
   * \param list     list of shader parts of the same category
   * \param type     shader type
   * \param key      shader part key
   * \param prolog   whether the part being requested is a prolog
   * \param tm       LLVM target machine
   * \param debug    debug callback
   * \return         non-NULL on success
   */
   static struct si_shader_part *
   si_get_shader_part(struct si_screen *sscreen, struct si_shader_part **list,
                     {
                        /* Find existing. */
   for (result = *list; result; result = result->next) {
      if (memcmp(&result->key, key, sizeof(*key)) == 0) {
      simple_mtx_unlock(&sscreen->shader_parts_mutex);
                  /* Compile a new one. */
   result = CALLOC_STRUCT(si_shader_part);
            bool use_aco = (sscreen->debug_flags & DBG(USE_ACO)) && sscreen->info.has_graphics;
   if (use_aco) {
      switch (stage) {
   case MESA_SHADER_VERTEX:
      use_aco = sscreen->info.gfx_level <= GFX8 ||
            case MESA_SHADER_TESS_CTRL:
      use_aco = sscreen->info.gfx_level <= GFX8;
      default:
                     bool ok = use_aco ?
      si_aco_build_shader_part(sscreen, stage, prolog, debug, name, result) :
         if (ok) {
      result->next = *list;
      } else {
      FREE(result);
               simple_mtx_unlock(&sscreen->shader_parts_mutex);
      }
      static bool si_get_vs_prolog(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
               {
               if (!si_vs_needs_prolog(vs, key))
            /* Get the prolog. */
   union si_shader_part_key prolog_key;
   si_get_vs_prolog_key(&vs->info, main_part->info.num_input_sgprs, key, shader,
            shader->prolog =
      si_get_shader_part(sscreen, &sscreen->vs_prologs, MESA_SHADER_VERTEX, true, &prolog_key,
         }
      /**
   * Select and compile (or reuse) vertex shader parts (prolog & epilog).
   */
   static bool si_shader_select_vs_parts(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
         }
      void si_get_tcs_epilog_key(struct si_shader *shader, union si_shader_part_key *key)
   {
      memset(key, 0, sizeof(*key));
   key->tcs_epilog.wave32 = shader->wave_size == 32;
            /* If output patches are wholly in one wave, we don't need a barrier. */
   key->tcs_epilog.noop_s_barrier =
      }
      /**
   * Select and compile (or reuse) TCS parts (epilog).
   */
   static bool si_shader_select_tcs_parts(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
      if (sscreen->info.gfx_level >= GFX9) {
               if (!si_get_vs_prolog(sscreen, compiler, shader, debug, ls_main_part,
                              /* Get the epilog. */
   union si_shader_part_key epilog_key;
            shader->epilog = si_get_shader_part(sscreen, &sscreen->tcs_epilogs, MESA_SHADER_TESS_CTRL, false,
                  }
      /**
   * Select and compile (or reuse) GS parts (prolog).
   */
   static bool si_shader_select_gs_parts(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
      if (sscreen->info.gfx_level >= GFX9) {
               if (shader->key.ge.as_ngg)
         else
            if (shader->key.ge.part.gs.es->stage == MESA_SHADER_VERTEX &&
      !si_get_vs_prolog(sscreen, compiler, shader, debug, es_main_part,
                              }
      /**
   * Compute the PS prolog key, which contains all the information needed to
   * build the PS prolog function, and set related bits in shader->config.
   */
   void si_get_ps_prolog_key(struct si_shader *shader, union si_shader_part_key *key)
   {
               memset(key, 0, sizeof(*key));
   key->ps_prolog.states = shader->key.ps.part.prolog;
   key->ps_prolog.wave32 = shader->wave_size == 32;
   key->ps_prolog.colors_read = info->colors_read;
   key->ps_prolog.num_input_sgprs = shader->info.num_input_sgprs;
   key->ps_prolog.wqm =
      info->base.fs.needs_quad_helper_invocations &&
   (key->ps_prolog.colors_read || key->ps_prolog.states.force_persp_sample_interp ||
   key->ps_prolog.states.force_linear_sample_interp ||
   key->ps_prolog.states.force_persp_center_interp ||
   key->ps_prolog.states.force_linear_center_interp ||
               if (shader->key.ps.part.prolog.poly_stipple)
            if (info->colors_read) {
               if (shader->key.ps.part.prolog.color_two_side) {
      /* BCOLORs are stored after the last input. */
   key->ps_prolog.num_interp_inputs = info->num_inputs;
               for (unsigned i = 0; i < 2; i++) {
                                                            switch (interp) {
   case INTERP_MODE_FLAT:
      key->ps_prolog.color_interp_vgpr_index[i] = -1;
      case INTERP_MODE_SMOOTH:
   case INTERP_MODE_COLOR:
      /* Force the interpolation location for colors here. */
   if (shader->key.ps.part.prolog.force_persp_sample_interp)
                        switch (location) {
   case TGSI_INTERPOLATE_LOC_SAMPLE:
      key->ps_prolog.color_interp_vgpr_index[i] = 0;
   shader->config.spi_ps_input_ena |= S_0286CC_PERSP_SAMPLE_ENA(1);
      case TGSI_INTERPOLATE_LOC_CENTER:
      key->ps_prolog.color_interp_vgpr_index[i] = 2;
   shader->config.spi_ps_input_ena |= S_0286CC_PERSP_CENTER_ENA(1);
      case TGSI_INTERPOLATE_LOC_CENTROID:
      key->ps_prolog.color_interp_vgpr_index[i] = 4;
   shader->config.spi_ps_input_ena |= S_0286CC_PERSP_CENTROID_ENA(1);
      default:
         }
      case INTERP_MODE_NOPERSPECTIVE:
      /* Force the interpolation location for colors here. */
   if (shader->key.ps.part.prolog.force_linear_sample_interp)
                        /* The VGPR assignment for non-monolithic shaders
   * works because InitialPSInputAddr is set on the
   * main shader and PERSP_PULL_MODEL is never used.
   */
   switch (location) {
   case TGSI_INTERPOLATE_LOC_SAMPLE:
      key->ps_prolog.color_interp_vgpr_index[i] = 6;
   shader->config.spi_ps_input_ena |= S_0286CC_LINEAR_SAMPLE_ENA(1);
      case TGSI_INTERPOLATE_LOC_CENTER:
      key->ps_prolog.color_interp_vgpr_index[i] = 8;
   shader->config.spi_ps_input_ena |= S_0286CC_LINEAR_CENTER_ENA(1);
      case TGSI_INTERPOLATE_LOC_CENTROID:
      key->ps_prolog.color_interp_vgpr_index[i] = 10;
   shader->config.spi_ps_input_ena |= S_0286CC_LINEAR_CENTROID_ENA(1);
      default:
         }
      default:
                  }
      /**
   * Check whether a PS prolog is required based on the key.
   */
   bool si_need_ps_prolog(const union si_shader_part_key *key)
   {
      return key->ps_prolog.colors_read || key->ps_prolog.states.force_persp_sample_interp ||
         key->ps_prolog.states.force_linear_sample_interp ||
   key->ps_prolog.states.force_persp_center_interp ||
   key->ps_prolog.states.force_linear_center_interp ||
   key->ps_prolog.states.bc_optimize_for_persp ||
      }
      /**
   * Compute the PS epilog key, which contains all the information needed to
   * build the PS epilog function.
   */
   void si_get_ps_epilog_key(struct si_shader *shader, union si_shader_part_key *key)
   {
      struct si_shader_info *info = &shader->selector->info;
   memset(key, 0, sizeof(*key));
   key->ps_epilog.wave32 = shader->wave_size == 32;
   key->ps_epilog.uses_discard = si_shader_uses_discard(shader);
   key->ps_epilog.colors_written = info->colors_written;
   key->ps_epilog.color_types = info->output_color_types;
   key->ps_epilog.writes_z = info->writes_z;
   key->ps_epilog.writes_stencil = info->writes_stencil;
   key->ps_epilog.writes_samplemask = info->writes_samplemask &&
            }
      /**
   * Select and compile (or reuse) pixel shader parts (prolog & epilog).
   */
   static bool si_shader_select_ps_parts(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
      union si_shader_part_key prolog_key;
            /* Get the prolog. */
            /* The prolog is a no-op if these aren't set. */
   if (si_need_ps_prolog(&prolog_key)) {
      shader->prolog =
      si_get_shader_part(sscreen, &sscreen->ps_prologs, MESA_SHADER_FRAGMENT, true, &prolog_key,
      if (!shader->prolog)
               /* Get the epilog. */
            shader->epilog =
      si_get_shader_part(sscreen, &sscreen->ps_epilogs, MESA_SHADER_FRAGMENT, false, &epilog_key,
      if (!shader->epilog)
                     /* Make sure spi_ps_input_addr bits is superset of spi_ps_input_ena. */
   unsigned spi_ps_input_ena = shader->config.spi_ps_input_ena;
   unsigned spi_ps_input_addr = shader->config.spi_ps_input_addr;
               }
      void si_multiwave_lds_size_workaround(struct si_screen *sscreen, unsigned *lds_size)
   {
      /* If tessellation is all offchip and on-chip GS isn't used, this
   * workaround is not needed.
   */
            /* SPI barrier management bug:
   *   Make sure we have at least 4k of LDS in use to avoid the bug.
   *   It applies to workgroup sizes of more than one wavefront.
   */
   if (sscreen->info.family == CHIP_BONAIRE || sscreen->info.family == CHIP_KABINI)
      }
      static void si_fix_resource_usage(struct si_screen *sscreen, struct si_shader *shader)
   {
                        if (shader->selector->stage == MESA_SHADER_COMPUTE &&
      si_get_max_workgroup_size(shader) > shader->wave_size) {
         }
      bool si_create_shader_variant(struct si_screen *sscreen, struct ac_llvm_compiler *compiler,
         {
      struct si_shader_selector *sel = shader->selector;
            if (sel->stage == MESA_SHADER_FRAGMENT) {
      shader->ps.writes_samplemask = sel->info.writes_samplemask &&
               /* LS, ES, VS are compiled on demand if the main part hasn't been
   * compiled for that stage.
   *
   * GS are compiled on demand if the main part hasn't been compiled
   * for the chosen NGG-ness.
   *
   * Vertex shaders are compiled on demand when a vertex fetch
   * workaround must be applied.
   */
   if (shader->is_monolithic) {
      /* Monolithic shader (compiled as a whole, has many variants,
   * may take a long time to compile).
   */
   if (!si_compile_shader(sscreen, compiler, shader, debug))
      } else {
      /* The shader consists of several parts:
   *
   * - the middle part is the user shader, it has 1 variant only
   *   and it was compiled during the creation of the shader
   *   selector
   * - the prolog part is inserted at the beginning
   * - the epilog part is inserted at the end
   *
   * The prolog and epilog have many (but simple) variants.
   *
   * Starting with gfx9, geometry and tessellation control
   * shaders also contain the prolog and user shader parts of
   * the previous shader stage.
            if (!mainp)
            /* Copy the compiled shader data over. */
   shader->is_binary_shared = true;
   shader->binary = mainp->binary;
   shader->config = mainp->config;
            /* Select prologs and/or epilogs. */
   switch (sel->stage) {
   case MESA_SHADER_VERTEX:
      if (!si_shader_select_vs_parts(sscreen, compiler, shader, debug))
            case MESA_SHADER_TESS_CTRL:
      if (!si_shader_select_tcs_parts(sscreen, compiler, shader, debug))
            case MESA_SHADER_TESS_EVAL:
         case MESA_SHADER_GEOMETRY:
                     /* Clone the GS copy shader for the shader variant.
   * We can't just copy the pointer because we change the pm4 state and
   * si_shader_selector::gs_copy_shader must be immutable because it's shared
   * by multiple contexts.
   */
   if (!shader->key.ge.as_ngg) {
      assert(sel->main_shader_part == mainp);
   assert(sel->main_shader_part->gs_copy_shader);
   assert(sel->main_shader_part->gs_copy_shader->bo);
                  shader->gs_copy_shader = CALLOC_STRUCT(si_shader);
   memcpy(shader->gs_copy_shader, sel->main_shader_part->gs_copy_shader,
         /* Increase the reference count. */
   pipe_reference(NULL, &shader->gs_copy_shader->bo->b.b.reference);
   /* Initialize some fields differently. */
   shader->gs_copy_shader->shader_log = NULL;
   shader->gs_copy_shader->is_binary_shared = true;
      }
      case MESA_SHADER_FRAGMENT:
                     /* Make sure we have at least as many VGPRs as there
   * are allocated inputs.
   */
   shader->config.num_vgprs = MAX2(shader->config.num_vgprs, shader->info.num_input_vgprs);
      default:;
            assert(shader->wave_size == mainp->wave_size);
            /* Update SGPR and VGPR counts. */
   if (shader->prolog) {
      shader->config.num_sgprs =
         shader->config.num_vgprs =
      }
   if (shader->previous_stage) {
      shader->config.num_sgprs =
         shader->config.num_vgprs =
         shader->config.spilled_sgprs =
         shader->config.spilled_vgprs =
         shader->info.private_mem_vgprs =
         shader->config.scratch_bytes_per_wave =
      MAX2(shader->config.scratch_bytes_per_wave,
      shader->info.uses_instanceid |= shader->previous_stage->info.uses_instanceid;
   shader->info.uses_vmem_load_other |= shader->previous_stage->info.uses_vmem_load_other;
      }
   if (shader->epilog) {
      shader->config.num_sgprs =
         shader->config.num_vgprs =
      }
               if (sel->stage <= MESA_SHADER_GEOMETRY && shader->key.ge.as_ngg) {
      assert(!shader->key.ge.as_es && !shader->key.ge.as_ls);
   if (!gfx10_ngg_calculate_subgroup_info(shader)) {
      fprintf(stderr, "Failed to compute subgroup info\n");
         } else if (sscreen->info.gfx_level >= GFX9 && sel->stage == MESA_SHADER_GEOMETRY) {
                  shader->uses_vs_state_provoking_vertex =
      sscreen->use_ngg &&
   /* Used to convert triangle strips from GS to triangles. */
   ((sel->stage == MESA_SHADER_GEOMETRY &&
   util_rast_prim_is_triangles(sel->info.base.gs.output_primitive)) ||
   (sel->stage == MESA_SHADER_VERTEX &&
   /* Used to export PrimitiveID from the correct vertex. */
         shader->uses_gs_state_outprim = sscreen->use_ngg &&
                        if (sel->stage == MESA_SHADER_VERTEX) {
      shader->uses_base_instance = sel->info.uses_base_instance ||
            } else if (sel->stage == MESA_SHADER_TESS_CTRL) {
      shader->uses_base_instance = shader->previous_stage_sel &&
                  } else if (sel->stage == MESA_SHADER_GEOMETRY) {
      shader->uses_base_instance = shader->previous_stage_sel &&
                                    /* Upload. */
   bool ok = si_shader_binary_upload(sscreen, shader, 0);
            if (!ok)
            }
      void si_shader_binary_clean(struct si_shader_binary *binary)
   {
      free((void *)binary->code_buffer);
            free(binary->llvm_ir_string);
            free((void *)binary->symbols);
            free(binary->uploaded_code);
   binary->uploaded_code = NULL;
      }
      void si_shader_destroy(struct si_shader *shader)
   {
      if (shader->scratch_bo)
                     if (!shader->is_binary_shared)
               }
      nir_shader *si_get_prev_stage_nir_shader(struct si_shader *shader,
                     {
      const struct si_shader_selector *sel = shader->selector;
            if (sel->stage == MESA_SHADER_TESS_CTRL) {
               prev_shader->selector = ls;
   prev_shader->key.ge.part.vs.prolog = key->ge.part.tcs.ls_prolog;
      } else {
               prev_shader->selector = es;
   prev_shader->key.ge.part.vs.prolog = key->ge.part.gs.vs_prolog;
   prev_shader->key.ge.as_es = 1;
               prev_shader->key.ge.mono = key->ge.mono;
   prev_shader->key.ge.opt = key->ge.opt;
   prev_shader->key.ge.opt.inline_uniforms = false; /* only TCS/GS can inline uniforms */
   /* kill_outputs was computed based on second shader's outputs so we can't use it to
   * kill first shader's outputs.
   */
   prev_shader->key.ge.opt.kill_outputs = 0;
   prev_shader->is_monolithic = true;
                     nir_shader *nir = si_get_nir_shader(prev_shader, args, free_nir,
                     shader->info.uses_instanceid |=
               }
      unsigned si_get_tcs_out_patch_stride(const struct si_shader_info *info)
   {
      unsigned tcs_out_vertices = info->base.tess.tcs_vertices_out;
   unsigned vertex_stride = util_last_bit64(info->outputs_written) * 4;
               }
      void si_get_tcs_epilog_args(enum amd_gfx_level gfx_level,
                                 {
               if (gfx_level >= GFX9) {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL); /* wave info */
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tcs_factor_offset);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
      } else {
      ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tcs_offchip_layout);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->tes_offchip_addr);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, &args->ac.tess_offchip_offset);
               ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* VGPR gap */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, NULL); /* VGPR gap */
   /* patch index within the wave (REL_PATCH_ID) */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, rel_patch_id);
   /* invocation ID within the patch */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_INT, invocation_id);
   /* LDS offset where tess factors should be loaded from */
            for (unsigned i = 0; i < 6; i++)
      }
      void si_get_vs_prolog_args(enum amd_gfx_level gfx_level,
               {
               unsigned num_input_sgprs = key->vs_prolog.num_input_sgprs;
            struct ac_arg input_sgprs[num_input_sgprs];
   for (unsigned i = 0; i < num_input_sgprs; i++)
            struct ac_arg input_vgprs[num_input_vgprs];
   for (unsigned i = 0; i < num_input_vgprs; i++)
            if (key->vs_prolog.num_merged_next_stage_vgprs)
            unsigned first_vs_vgpr = key->vs_prolog.num_merged_next_stage_vgprs;
   unsigned vertex_id_vgpr = first_vs_vgpr;
   unsigned instance_id_vgpr = gfx_level >= GFX10 ?
            args->ac.vertex_id = input_vgprs[vertex_id_vgpr];
            if (key->vs_prolog.as_ls && gfx_level < GFX11)
            unsigned user_sgpr_base = key->vs_prolog.num_merged_next_stage_vgprs ? 8 : 0;
   args->internal_bindings = input_sgprs[user_sgpr_base + SI_SGPR_INTERNAL_BINDINGS];
   args->ac.start_instance = input_sgprs[user_sgpr_base + SI_SGPR_START_INSTANCE];
      }
      void si_get_ps_prolog_args(struct si_shader_args *args,
         {
                        struct ac_arg input_sgprs[num_input_sgprs];
   for (unsigned i = 0; i < num_input_sgprs; i++)
            args->internal_bindings = input_sgprs[SI_SGPR_INTERNAL_BINDINGS];
   /* Use the absolute location of the input. */
            ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.persp_sample);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.persp_center);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.persp_centroid);
   /* skip PERSP_PULL_MODEL */
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.linear_sample);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.linear_center);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 2, AC_ARG_FLOAT, &args->ac.linear_centroid);
            /* POS_X|Y|Z|W_FLOAT */
   for (unsigned i = 0; i < key->ps_prolog.num_pos_inputs; i++)
            ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.front_face);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.ancillary);
   ac_add_arg(&args->ac, AC_ARG_VGPR, 1, AC_ARG_FLOAT, &args->ac.sample_coverage);
      }
      void si_get_ps_epilog_args(struct si_shader_args *args,
                           {
               ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
   ac_add_arg(&args->ac, AC_ARG_SGPR, 1, AC_ARG_INT, NULL);
            u_foreach_bit (i, key->ps_epilog.colors_written) {
                  if (key->ps_epilog.writes_z)
            if (key->ps_epilog.writes_stencil)
            if (key->ps_epilog.writes_samplemask)
      }
