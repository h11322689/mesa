   /*
   * Copyright 2012 Advanced Micro Devices, Inc.
   *
   * SPDX-License-Identifier: MIT
   */
      #include "si_build_pm4.h"
      #include "util/hash_table.h"
   #define XXH_INLINE_ALL
   #include "util/xxhash.h"
   #include "util/u_cpu_detect.h"
   #include "util/u_index_modify.h"
   #include "util/u_prim.h"
   #include "util/u_upload_mgr.h"
   #include "ac_rtld.h"
   #include "si_build_pm4.h"
   #include "si_tracepoints.h"
      #if (GFX_VER == 6)
   #define GFX(name) name##GFX6
   #elif (GFX_VER == 7)
   #define GFX(name) name##GFX7
   #elif (GFX_VER == 8)
   #define GFX(name) name##GFX8
   #elif (GFX_VER == 9)
   #define GFX(name) name##GFX9
   #elif (GFX_VER == 10)
   #define GFX(name) name##GFX10
   #elif (GFX_VER == 103)
   #define GFX(name) name##GFX10_3
   #elif (GFX_VER == 11)
   #define GFX(name) name##GFX11
   #elif (GFX_VER == 115)
   #define GFX(name) name##GFX11_5
   #else
   #error "Unknown gfx level"
   #endif
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG>
   static bool si_update_shaders(struct si_context *sctx)
   {
      struct pipe_context *ctx = (struct pipe_context *)sctx;
   struct si_shader *old_vs = si_get_vs_inline(sctx, HAS_TESS, HAS_GS)->current;
   unsigned old_pa_cl_vs_out_cntl = old_vs ? old_vs->pa_cl_vs_out_cntl : 0;
   bool old_uses_vs_state_provoking_vertex = old_vs ? old_vs->uses_vs_state_provoking_vertex : false;
   bool old_uses_gs_state_outprim = old_vs ? old_vs->uses_gs_state_outprim : false;
   struct si_shader *old_ps = sctx->shader.ps.current;
   unsigned old_spi_shader_col_format =
                  /* Update TCS and TES. */
   if (HAS_TESS) {
      if (!sctx->tess_rings) {
      si_init_tess_factor_ring(sctx);
   if (!sctx->tess_rings)
               if (!sctx->is_user_tcs) {
      if (!si_set_tcs_to_fixed_func_shader(sctx))
               r = si_shader_select(ctx, &sctx->shader.tcs);
   if (r)
                  if (!HAS_GS || GFX_VERSION <= GFX8) {
      r = si_shader_select(ctx, &sctx->shader.tes);
                  if (HAS_GS) {
      /* TES as ES */
   assert(GFX_VERSION <= GFX8);
      } else if (NGG) {
         } else {
               } else {
      /* Reset TCS to clear fixed function shader. */
   if (!sctx->is_user_tcs && sctx->shader.tcs.cso) {
      sctx->shader.tcs.cso = NULL;
               if (GFX_VERSION <= GFX8) {
      si_pm4_bind_state(sctx, ls, NULL);
      }
   si_pm4_bind_state(sctx, hs, NULL);
               /* Update GS. */
   if (HAS_GS) {
      r = si_shader_select(ctx, &sctx->shader.gs);
   if (r)
         si_pm4_bind_state(sctx, gs, sctx->shader.gs.current);
   if (!NGG) {
               if (!si_update_gs_ring_buffers(sctx))
      } else if (GFX_VERSION < GFX11) {
      si_pm4_bind_state(sctx, vs, NULL);
         } else {
      if (!NGG) {
      si_pm4_bind_state(sctx, gs, NULL);
   sctx->prefetch_L2_mask &= ~SI_PREFETCH_GS;
   if (GFX_VERSION <= GFX8) {
      si_pm4_bind_state(sctx, es, NULL);
                     /* Update VS. */
   if ((!HAS_TESS && !HAS_GS) || GFX_VERSION <= GFX8) {
      r = si_shader_select(ctx, &sctx->shader.vs);
   if (r)
            if (!HAS_TESS && !HAS_GS) {
      if (NGG) {
      si_pm4_bind_state(sctx, gs, sctx->shader.vs.current);
   if (GFX_VERSION < GFX11) {
      si_pm4_bind_state(sctx, vs, NULL);
         } else {
            } else if (HAS_TESS) {
         } else {
      assert(HAS_GS);
                  if (GFX_VERSION >= GFX9 && HAS_TESS)
         else if (GFX_VERSION >= GFX9 && HAS_GS)
         else
            /* Update VGT_SHADER_STAGES_EN. */
            if (HAS_TESS) {
      vgt_stages |= S_028B54_LS_EN(V_028B54_LS_STAGE_ON) |
               S_028B54_HS_EN(1) |
               if (NGG) {
         } else {
      if (HAS_GS) {
                     vgt_stages |= S_028B54_ES_EN(HAS_TESS ? V_028B54_ES_STAGE_DS : V_028B54_ES_STAGE_REAL) |
               S_028B54_GS_EN(1) |
      } else if (HAS_TESS) {
                  vgt_stages |= S_028B54_MAX_PRIMGRP_IN_WAVE(GFX_VERSION >= GFX9 ? 2 : 0) |
                     /* Update GE_CNTL. */
            if (GFX_VERSION >= GFX10) {
               if (NGG) {
      if (HAS_TESS) {
      if (GFX_VERSION >= GFX11) {
      ge_cntl = si_get_vs_inline(sctx, HAS_TESS, HAS_GS)->current->ge_cntl |
      } else {
      /* PRIM_GRP_SIZE_GFX10 is set by si_emit_vgt_pipeline_state. */
   ge_cntl = S_03096C_VERT_GRP_SIZE(0) |
         } else {
            } else {
      unsigned primgroup_size;
                  if (HAS_TESS) {
      primgroup_size = 0; /* this is set by si_emit_vgt_pipeline_state */
      } else if (HAS_GS) {
      unsigned vgt_gs_onchip_cntl = sctx->shader.gs.current->gs.vgt_gs_onchip_cntl;
   primgroup_size = G_028A44_GS_PRIMS_PER_SUBGRP(vgt_gs_onchip_cntl);
      } else {
      primgroup_size = 128; /* recommended without a GS and tess */
               ge_cntl = S_03096C_PRIM_GRP_SIZE_GFX10(primgroup_size) |
                     /* Note: GE_CNTL.PACKET_TO_ONE_PA should only be set if LINE_STIPPLE_TEX_ENA == 1.
   * Since we don't use that, we don't have to do anything.
               if (vgt_stages != sctx->vgt_shader_stages_en ||
      (GFX_VERSION >= GFX10 && ge_cntl != sctx->ge_cntl)) {
   sctx->vgt_shader_stages_en = vgt_stages;
   sctx->ge_cntl = ge_cntl;
                        if (old_pa_cl_vs_out_cntl != hw_vs->pa_cl_vs_out_cntl)
            /* If we start to use any of these, we need to update the SGPR. */
   if ((hw_vs->uses_vs_state_provoking_vertex && !old_uses_vs_state_provoking_vertex) ||
      (hw_vs->uses_gs_state_outprim && !old_uses_gs_state_outprim))
         r = si_shader_select(ctx, &sctx->shader.ps);
   if (r)
                  unsigned db_shader_control = sctx->shader.ps.current->ps.db_shader_control;
   if (sctx->ps_db_shader_control != db_shader_control) {
      sctx->ps_db_shader_control = db_shader_control;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.db_render_state);
   if (sctx->screen->dpbb_allowed)
               if (si_pm4_state_changed(sctx, ps) ||
      (!NGG && si_pm4_state_changed(sctx, vs)) ||
   (NGG && si_pm4_state_changed(sctx, gs))) {
   sctx->atoms.s.spi_map.emit = sctx->emit_spi_map[sctx->shader.ps.current->ps.num_interp];
               if ((GFX_VERSION >= GFX10_3 || (GFX_VERSION >= GFX9 && sctx->screen->info.rbplus_allowed)) &&
      si_pm4_state_changed(sctx, ps) &&
   (!old_ps || old_spi_shader_col_format !=
               if (sctx->smoothing_enabled !=
      sctx->shader.ps.current->key.ps.mono.poly_line_smoothing) {
   sctx->smoothing_enabled = sctx->shader.ps.current->key.ps.mono.poly_line_smoothing;
            /* NGG cull state uses smoothing_enabled. */
   if (GFX_VERSION >= GFX10 && sctx->screen->use_ngg_culling)
            if (GFX_VERSION == GFX11 && sctx->screen->info.has_export_conflict_bug)
            if (sctx->framebuffer.nr_samples <= 1)
               if (HAS_TESS)
            if (GFX_VERSION >= GFX9 && unlikely(sctx->sqtt)) {
      /* Pretend the bound shaders form a vk pipeline. Include the scratch size in
   * the hash calculation to force re-emitting the pipeline if the scratch bo
   * changes.
   */
   uint64_t scratch_bo_size = sctx->scratch_buffer ? sctx->scratch_buffer->bo_size : 0;
   uint64_t pipeline_code_hash = scratch_bo_size;
            /* Compute pipeline code hash. */
   for (int i = 0; i < SI_NUM_GRAPHICS_SHADERS; i++) {
      struct si_shader *shader = sctx->shaders[i].current;
   if (sctx->shaders[i].cso && shader) {
      pipeline_code_hash = XXH64(
                                       struct si_sqtt_fake_pipeline *pipeline = NULL;
   if (!si_sqtt_pipeline_is_registered(sctx->sqtt, pipeline_code_hash)) {
      /* This is a new pipeline. Allocate a new bo to hold all the shaders. Without
   * this, shader code export process creates huge rgp files because RGP assumes
   * the shaders live sequentially in memory (shader N address = shader 0 + offset N)
   */
   struct si_resource *bo = si_aligned_buffer_create(
      &sctx->screen->b,
   (sctx->screen->info.cpdma_prefetch_writes_memory ? 0 : SI_RESOURCE_FLAG_READ_ONLY) |
               char *ptr = (char *) (bo ? sctx->screen->ws->buffer_map(sctx->screen->ws,
                                       if (ptr) {
      pipeline = (struct si_sqtt_fake_pipeline *)
                                       for (int i = 0; i < SI_NUM_GRAPHICS_SHADERS; i++) {
      struct si_shader *shader = sctx->shaders[i].current;
                           struct ac_rtld_upload_info u = {};
   u.binary = &binary;
   u.get_external_symbol = si_get_external_symbol;
                                                                           uint64_t va_low = (pipeline->bo->gpu_address + pipeline->offset[i]) >> 8;
   uint32_t reg = pm4->spi_shader_pgm_lo_reg;
         }
                                    } else {
      if (bo)
         } else {
      pipeline = (struct si_sqtt_fake_pipeline *)_mesa_hash_table_u64_search(
      }
            pipeline->code_hash = pipeline_code_hash;
   radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, pipeline->bo,
            si_sqtt_describe_pipeline_bind(sctx, pipeline_code_hash, 0);
               if ((GFX_VERSION <= GFX8 &&
      (si_pm4_state_enabled_and_changed(sctx, ls) || si_pm4_state_enabled_and_changed(sctx, es))) ||
   si_pm4_state_enabled_and_changed(sctx, hs) || si_pm4_state_enabled_and_changed(sctx, gs) ||
   (!NGG && si_pm4_state_enabled_and_changed(sctx, vs)) || si_pm4_state_enabled_and_changed(sctx, ps)) {
            if (HAS_TESS) {
                              if (HAS_GS) {
                        } else {
            } else if (HAS_GS) {
                        } else {
                           if (scratch_size && !si_update_spi_tmpring_size(sctx, scratch_size))
            if (GFX_VERSION >= GFX7) {
                                                                                 if (si_pm4_state_enabled_and_changed(sctx, ps))
                  /* si_shader_select_with_key can clear the ngg_culling in the shader key if the shader
   * compilation hasn't finished. Set it to the same value in si_context.
   */
   if (GFX_VERSION >= GFX10 && NGG)
            sctx->do_update_shaders = false;
      }
      ALWAYS_INLINE
   static unsigned si_conv_pipe_prim(unsigned mode)
   {
      static const unsigned prim_conv[] = {
      [MESA_PRIM_POINTS] = V_008958_DI_PT_POINTLIST,
   [MESA_PRIM_LINES] = V_008958_DI_PT_LINELIST,
   [MESA_PRIM_LINE_LOOP] = V_008958_DI_PT_LINELOOP,
   [MESA_PRIM_LINE_STRIP] = V_008958_DI_PT_LINESTRIP,
   [MESA_PRIM_TRIANGLES] = V_008958_DI_PT_TRILIST,
   [MESA_PRIM_TRIANGLE_STRIP] = V_008958_DI_PT_TRISTRIP,
   [MESA_PRIM_TRIANGLE_FAN] = V_008958_DI_PT_TRIFAN,
   [MESA_PRIM_QUADS] = V_008958_DI_PT_QUADLIST,
   [MESA_PRIM_QUAD_STRIP] = V_008958_DI_PT_QUADSTRIP,
   [MESA_PRIM_POLYGON] = V_008958_DI_PT_POLYGON,
   [MESA_PRIM_LINES_ADJACENCY] = V_008958_DI_PT_LINELIST_ADJ,
   [MESA_PRIM_LINE_STRIP_ADJACENCY] = V_008958_DI_PT_LINESTRIP_ADJ,
   [MESA_PRIM_TRIANGLES_ADJACENCY] = V_008958_DI_PT_TRILIST_ADJ,
   [MESA_PRIM_TRIANGLE_STRIP_ADJACENCY] = V_008958_DI_PT_TRISTRIP_ADJ,
   [MESA_PRIM_PATCHES] = V_008958_DI_PT_PATCH,
      assert(mode < ARRAY_SIZE(prim_conv));
      }
      template<amd_gfx_level GFX_VERSION>
   static void si_cp_dma_prefetch_inline(struct si_context *sctx, uint64_t address, unsigned size)
   {
               if (GFX_VERSION >= GFX11)
            /* The prefetch address and size must be aligned, so that we don't have to apply
   * the complicated hw bug workaround.
   *
   * The size should also be less than 2 MB, so that we don't have to use a loop.
   * Callers shouldn't need to prefetch more than 2 MB.
   */
   assert(size % SI_CPDMA_ALIGNMENT == 0);
   assert(address % SI_CPDMA_ALIGNMENT == 0);
            uint32_t header = S_411_SRC_SEL(V_411_SRC_ADDR_TC_L2);
            if (GFX_VERSION >= GFX9) {
      command |= S_415_DISABLE_WR_CONFIRM_GFX9(1);
      } else {
      command |= S_415_DISABLE_WR_CONFIRM_GFX6(1);
               struct radeon_cmdbuf *cs = &sctx->gfx_cs;
   radeon_begin(cs);
   radeon_emit(PKT3(PKT3_DMA_DATA, 5, 0));
   radeon_emit(header);
   radeon_emit(address);       /* SRC_ADDR_LO [31:0] */
   radeon_emit(address >> 32); /* SRC_ADDR_HI [31:0] */
   radeon_emit(address);       /* DST_ADDR_LO [31:0] */
   radeon_emit(address >> 32); /* DST_ADDR_HI [31:0] */
   radeon_emit(command);
      }
      #if GFX_VER == 6 /* declare this function only once because it handles all chips. */
      void si_cp_dma_prefetch(struct si_context *sctx, struct pipe_resource *buf,
         {
      uint64_t address = si_resource(buf)->gpu_address + offset;
   switch (sctx->gfx_level) {
   case GFX7:
      si_cp_dma_prefetch_inline<GFX7>(sctx, address, size);
      case GFX8:
      si_cp_dma_prefetch_inline<GFX8>(sctx, address, size);
      case GFX9:
      si_cp_dma_prefetch_inline<GFX9>(sctx, address, size);
      case GFX10:
      si_cp_dma_prefetch_inline<GFX10>(sctx, address, size);
      case GFX10_3:
      si_cp_dma_prefetch_inline<GFX10_3>(sctx, address, size);
      case GFX11:
      si_cp_dma_prefetch_inline<GFX11>(sctx, address, size);
      case GFX11_5:
      si_cp_dma_prefetch_inline<GFX11_5>(sctx, address, size);
      default:
            }
      #endif
      template<amd_gfx_level GFX_VERSION>
   static void si_prefetch_shader_async(struct si_context *sctx, struct si_shader *shader)
   {
      struct pipe_resource *bo = &shader->bo->b.b;
      }
      /**
   * Prefetch shaders.
   */
   template<amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG>
   ALWAYS_INLINE
   static void si_prefetch_shaders(struct si_context *sctx)
   {
               /* GFX6 doesn't support the L2 prefetch. */
   if (GFX_VERSION < GFX7 || !mask)
            /* Prefetch shaders and VBO descriptors to TC L2. */
   if (GFX_VERSION >= GFX11) {
      if (HAS_TESS && mask & SI_PREFETCH_HS)
            if (mask & SI_PREFETCH_GS)
      } else if (GFX_VERSION >= GFX9) {
      if (HAS_TESS) {
      if (mask & SI_PREFETCH_HS)
      }
   if ((HAS_GS || NGG) && mask & SI_PREFETCH_GS)
         if (!NGG && mask & SI_PREFETCH_VS)
      } else {
      /* GFX6-GFX8 */
   /* Choose the right spot for the VBO prefetch. */
   if (HAS_TESS) {
      if (mask & SI_PREFETCH_LS)
         if (mask & SI_PREFETCH_HS)
         if (mask & SI_PREFETCH_ES)
         if (mask & SI_PREFETCH_GS)
      } else if (HAS_GS) {
      if (mask & SI_PREFETCH_ES)
         if (mask & SI_PREFETCH_GS)
      }
   if (mask & SI_PREFETCH_VS)
               if (mask & SI_PREFETCH_PS)
            /* This must be cleared only when AFTER_DRAW is true. */
      }
      static unsigned si_num_prims_for_vertices(enum mesa_prim prim,
         {
      switch (prim) {
   case MESA_PRIM_PATCHES:
         case MESA_PRIM_POLYGON:
      /* It's a triangle fan with different edge flags. */
      case SI_PRIM_RECTANGLE_LIST:
         default:
            }
      static unsigned si_get_init_multi_vgt_param(struct si_screen *sscreen, union si_vgt_param_key *key)
   {
      STATIC_ASSERT(sizeof(union si_vgt_param_key) == 2);
            /* SWITCH_ON_EOP(0) is always preferable. */
   bool wd_switch_on_eop = false;
   bool ia_switch_on_eop = false;
   bool ia_switch_on_eoi = false;
   bool partial_vs_wave = false;
            if (key->u.uses_tess) {
      /* SWITCH_ON_EOI must be set if PrimID is used. */
   if (key->u.tess_uses_prim_id)
            /* Bug with tessellation and GS on Bonaire and older 2 SE chips. */
   if ((sscreen->info.family == CHIP_TAHITI || sscreen->info.family == CHIP_PITCAIRN ||
      sscreen->info.family == CHIP_BONAIRE) &&
               /* Needed for 028B6C_DISTRIBUTION_MODE != 0. (implies >= GFX8) */
   if (sscreen->info.has_distributed_tess) {
      if (key->u.uses_gs) {
      if (sscreen->info.gfx_level == GFX8)
      } else {
                        /* This is a hardware requirement. */
   if (key->u.line_stipple_enabled || (sscreen->debug_flags & DBG(SWITCH_ON_EOP))) {
      ia_switch_on_eop = true;
               if (sscreen->info.gfx_level >= GFX7) {
      /* WD_SWITCH_ON_EOP has no effect on GPUs with less than
   * 4 shader engines. Set 1 to pass the assertion below.
   * The other cases are hardware requirements.
   *
   * Polaris supports primitive restart with WD_SWITCH_ON_EOP=0
   * for points, line strips, and tri strips.
   */
   if (sscreen->info.max_se <= 2 || key->u.prim == MESA_PRIM_POLYGON ||
      key->u.prim == MESA_PRIM_LINE_LOOP || key->u.prim == MESA_PRIM_TRIANGLE_FAN ||
   key->u.prim == MESA_PRIM_TRIANGLE_STRIP_ADJACENCY ||
   (key->u.primitive_restart &&
   (sscreen->info.family < CHIP_POLARIS10 ||
      (key->u.prim != MESA_PRIM_POINTS && key->u.prim != MESA_PRIM_LINE_STRIP &&
                  /* Hawaii hangs if instancing is enabled and WD_SWITCH_ON_EOP is 0.
   * We don't know that for indirect drawing, so treat it as
   * always problematic. */
   if (sscreen->info.family == CHIP_HAWAII && key->u.uses_instancing)
            /* Performance recommendation for 4 SE Gfx7-8 parts if
   * instances are smaller than a primgroup.
   * Assume indirect draws always use small instances.
   * This is needed for good VS wave utilization.
   */
   if (sscreen->info.gfx_level <= GFX8 && sscreen->info.max_se == 4 &&
                  /* Required on GFX7 and later. */
   if (sscreen->info.max_se == 4 && !wd_switch_on_eop)
            /* HW engineers suggested that PARTIAL_VS_WAVE_ON should be set
   * to work around a GS hang.
   */
   if (key->u.uses_gs &&
      (sscreen->info.family == CHIP_TONGA || sscreen->info.family == CHIP_FIJI ||
   sscreen->info.family == CHIP_POLARIS10 || sscreen->info.family == CHIP_POLARIS11 ||
               /* Required by Hawaii and, for some special cases, by GFX8. */
   if (ia_switch_on_eoi &&
      (sscreen->info.family == CHIP_HAWAII ||
               /* Instancing bug on Bonaire. */
   if (sscreen->info.family == CHIP_BONAIRE && ia_switch_on_eoi && key->u.uses_instancing)
            /* This only applies to Polaris10 and later 4 SE chips.
   * wd_switch_on_eop is already true on all other chips.
   */
   if (!wd_switch_on_eop && key->u.primitive_restart)
            /* If the WD switch is false, the IA switch must be false too. */
               /* If SWITCH_ON_EOI is set, PARTIAL_ES_WAVE must be set too. */
   if (sscreen->info.gfx_level <= GFX8 && ia_switch_on_eoi)
            return S_028AA8_SWITCH_ON_EOP(ia_switch_on_eop) | S_028AA8_SWITCH_ON_EOI(ia_switch_on_eoi) |
         S_028AA8_PARTIAL_VS_WAVE_ON(partial_vs_wave) |
   S_028AA8_PARTIAL_ES_WAVE_ON(partial_es_wave) |
   S_028AA8_WD_SWITCH_ON_EOP(sscreen->info.gfx_level >= GFX7 ? wd_switch_on_eop : 0) |
   /* The following field was moved to VGT_SHADER_STAGES_EN in GFX9. */
   S_028AA8_MAX_PRIMGRP_IN_WAVE(sscreen->info.gfx_level == GFX8 ? max_primgroup_in_wave
            }
      static void si_init_ia_multi_vgt_param_table(struct si_context *sctx)
   {
      for (int prim = 0; prim <= SI_PRIM_RECTANGLE_LIST; prim++)
      for (int uses_instancing = 0; uses_instancing < 2; uses_instancing++)
      for (int multi_instances = 0; multi_instances < 2; multi_instances++)
      for (int primitive_restart = 0; primitive_restart < 2; primitive_restart++)
      for (int count_from_so = 0; count_from_so < 2; count_from_so++)
      for (int line_stipple = 0; line_stipple < 2; line_stipple++)
                                          key.index = 0;
   key.u.prim = prim;
   key.u.uses_instancing = uses_instancing;
   key.u.multi_instances_smaller_than_primgroup = multi_instances;
   key.u.primitive_restart = primitive_restart;
                        }
      static bool si_is_line_stipple_enabled(struct si_context *sctx)
   {
               return rs->line_stipple_enable && sctx->current_rast_prim != MESA_PRIM_POINTS &&
      }
      enum si_is_draw_vertex_state {
      DRAW_VERTEX_STATE_OFF,
      };
      enum si_has_pairs {
      HAS_PAIRS_OFF,
      };
      template <si_is_draw_vertex_state IS_DRAW_VERTEX_STATE> ALWAYS_INLINE
   static bool num_instanced_prims_less_than(const struct pipe_draw_indirect_info *indirect,
                                 {
      if (IS_DRAW_VERTEX_STATE)
            if (indirect) {
      return indirect->buffer ||
      } else {
      return instance_count > 1 &&
         }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS,
         static unsigned si_get_ia_multi_vgt_param(struct si_context *sctx,
                     {
      union si_vgt_param_key key = sctx->ia_multi_vgt_param_key;
   unsigned primgroup_size;
            if (HAS_TESS) {
         } else if (HAS_GS) {
         } else {
                  key.u.prim = prim;
   key.u.uses_instancing = !IS_DRAW_VERTEX_STATE &&
         key.u.multi_instances_smaller_than_primgroup =
      num_instanced_prims_less_than<IS_DRAW_VERTEX_STATE>(indirect, prim, min_vertex_count,
            key.u.primitive_restart = !IS_DRAW_VERTEX_STATE && primitive_restart;
   key.u.count_from_stream_output = !IS_DRAW_VERTEX_STATE && indirect &&
                  ia_multi_vgt_param =
            if (HAS_GS) {
      /* GS requirement. */
   if (GFX_VERSION <= GFX8 &&
                  /* GS hw bug with single-primitive instances and SWITCH_ON_EOI.
   * The hw doc says all multi-SE chips are affected, but Vulkan
   * only applies it to Hawaii. Do what Vulkan does.
   */
   if (GFX_VERSION == GFX7 &&
      sctx->family == CHIP_HAWAII && G_028AA8_SWITCH_ON_EOI(ia_multi_vgt_param) &&
   num_instanced_prims_less_than<IS_DRAW_VERTEX_STATE>(indirect, prim, min_vertex_count,
         /* The cache flushes should have been emitted already. */
   assert(sctx->flags == 0);
   sctx->flags = SI_CONTEXT_VGT_FLUSH;
                     }
      /* rast_prim is the primitive type after GS. */
   template<amd_gfx_level GFX_VERSION, si_has_gs HAS_GS, si_has_ngg NGG> ALWAYS_INLINE
   static void si_emit_rasterizer_prim_state(struct si_context *sctx)
   {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
                     if (unlikely(si_is_line_stipple_enabled(sctx))) {
      /* For lines, reset the stipple pattern at each primitive. Otherwise,
   * reset the stipple pattern at each packet (line strips, line loops).
   */
   enum mesa_prim rast_prim = sctx->current_rast_prim;
   bool reset_per_prim = rast_prim == MESA_PRIM_LINES ||
         /* 0 = no reset, 1 = reset per prim, 2 = reset per packet */
   unsigned value =
            radeon_opt_set_context_reg(sctx, R_028A0C_PA_SC_LINE_STIPPLE, SI_TRACKED_PA_SC_LINE_STIPPLE,
               if (NGG || HAS_GS) {
      if (GFX_VERSION >= GFX11) {
      radeon_opt_set_uconfig_reg(sctx, R_030998_VGT_GS_OUT_PRIM_TYPE,
      } else {
      radeon_opt_set_context_reg(sctx, R_028A6C_VGT_GS_OUT_PRIM_TYPE,
                  if (GFX_VERSION == GFX9)
         else
      }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
         static void si_emit_vs_state(struct si_context *sctx, unsigned index_size)
   {
      if (!IS_DRAW_VERTEX_STATE && sctx->num_vs_blit_sgprs) {
      /* Re-emit the state after we leave u_blitter. */
   sctx->last_vs_state = ~0;
   sctx->last_gs_state = ~0;
               unsigned vs_state = sctx->current_vs_state; /* all VS bits including LS bits */
            if (sctx->shader.vs.cso->info.uses_base_vertex && index_size)
            /* Copy all state bits from vs_state to gs_state except the LS bits. */
   gs_state |= vs_state &
                  if (vs_state != sctx->last_vs_state ||
      ((HAS_GS || NGG) && gs_state != sctx->last_gs_state)) {
            /* These are all constant expressions. */
   unsigned vs_base = si_get_user_data_base(GFX_VERSION, HAS_TESS, HAS_GS, NGG,
         unsigned tes_base = si_get_user_data_base(GFX_VERSION, HAS_TESS, HAS_GS, NGG,
         unsigned gs_base = si_get_user_data_base(GFX_VERSION, HAS_TESS, HAS_GS, NGG,
                  radeon_begin(cs);
   if (HAS_GS) {
               /* GS always uses the state bits for emulating VGT_ESGS_RING_ITEMSIZE on Gfx9
   * (via nir_load_esgs_vertex_stride_amd) and for emulating GS pipeline statistics
   * on gfx10.x. NGG GS also has lots of states in there.
   */
                  /* The GS copy shader (for legacy GS) always uses the state bits. */
   if (!NGG)
      } else if (HAS_TESS) {
      radeon_set_or_push_gfx_sh_reg(vs_base + SI_SGPR_VS_STATE_BITS * 4, vs_state);
      } else {
         }
            sctx->last_vs_state = vs_state;
   if (HAS_GS || NGG)
         }
      template <amd_gfx_level GFX_VERSION> ALWAYS_INLINE
   static bool si_prim_restart_index_changed(struct si_context *sctx, unsigned index_size,
         {
      if (!primitive_restart)
            if (sctx->last_restart_index == SI_RESTART_INDEX_UNKNOWN)
            /* GFX8+ only compares the index type number of bits of the restart index, so the unused bits
   * are "don't care".
   *
   * Summary of restart index comparator behavior:
   * - GFX6-7: Compare all bits.
   * - GFX8: Only compare index type bits.
   * - GFX9+: If MATCH_ALL_BITS, compare all bits, else only compare index type bits.
   */
   if (GFX_VERSION >= GFX8) {
      /* This masking eliminates no-op 0xffffffff -> 0xffff restart index changes that cause
   * unnecessary context rolls when switching the index type.
   */
   unsigned index_mask = BITFIELD_MASK(index_size * 8);
      } else {
            }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS,
         static void si_emit_ia_multi_vgt_param(struct si_context *sctx,
                     {
      struct radeon_cmdbuf *cs = &sctx->gfx_cs;
            ia_multi_vgt_param =
      si_get_ia_multi_vgt_param<GFX_VERSION, HAS_TESS, HAS_GS, IS_DRAW_VERTEX_STATE>
         radeon_begin(cs);
   if (GFX_VERSION == GFX9) {
      /* Workaround for SpecviewPerf13 Catia hang on GFX9. */
   if (prim != sctx->last_prim)
            radeon_opt_set_uconfig_reg_idx(sctx, GFX_VERSION, R_030960_IA_MULTI_VGT_PARAM,
            } else if (GFX_VERSION >= GFX7) {
      radeon_opt_set_context_reg_idx(sctx, R_028AA8_IA_MULTI_VGT_PARAM,
      } else {
      radeon_opt_set_context_reg(sctx, R_028AA8_IA_MULTI_VGT_PARAM,
      }
      }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
         static void si_emit_draw_registers(struct si_context *sctx,
                           {
               if (GFX_VERSION <= GFX9) {
      si_emit_ia_multi_vgt_param<GFX_VERSION, HAS_TESS, HAS_GS, IS_DRAW_VERTEX_STATE>
                        if (prim != sctx->last_prim) {
               if (GFX_VERSION >= GFX10)
         else if (GFX_VERSION >= GFX7)
         else
                        /* Primitive restart. */
   if (GFX_VERSION >= GFX11) {
      /* GFX11+ can ignore primitive restart for non-indexed draws because it has no effect.
   * (it's disabled for non-indexed draws by setting DISABLE_FOR_AUTO_INDEX in the preamble)
   */
   if (index_size) {
      if (primitive_restart != sctx->last_primitive_restart_en) {
      radeon_set_uconfig_reg(R_03092C_GE_MULTI_PRIM_IB_RESET_EN,
                        S_03092C_RESET_EN(primitive_restart) |
         }
   if (si_prim_restart_index_changed<GFX_VERSION>(sctx, index_size, primitive_restart,
            radeon_set_context_reg(R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, restart_index);
            } else {
      if (primitive_restart != sctx->last_primitive_restart_en) {
      if (GFX_VERSION >= GFX9)
         else
            }
   if (si_prim_restart_index_changed<GFX_VERSION>(sctx, index_size, primitive_restart,
            radeon_set_context_reg(R_02840C_VGT_MULTI_PRIM_IB_RESET_INDX, restart_index);
   sctx->last_restart_index = restart_index;
   if (GFX_VERSION == GFX9)
         }
      }
      static ALWAYS_INLINE void
   gfx11_emit_buffered_sh_regs_inline(struct si_context *sctx, unsigned *num_regs,
         {
               if (!reg_count)
                     /* If there is only one register, we can't use the packed SET packet. */
   if (reg_count == 1) {
      radeon_begin(&sctx->gfx_cs);
   radeon_emit(PKT3(PKT3_SET_SH_REG, 1, 0));
   radeon_emit(reg_pairs[0].reg_offset[0]);
   radeon_emit(reg_pairs[0].reg_value[0]);
   radeon_end();
               unsigned packet = reg_count <= 14 ? PKT3_SET_SH_REG_PAIRS_PACKED_N :
                  radeon_begin(&sctx->gfx_cs);
   radeon_emit(PKT3(packet, (padded_reg_count / 2) * 3, 0) | PKT3_RESET_FILTER_CAM_S(1));
   radeon_emit(padded_reg_count);
            if (reg_count % 2 == 1) {
               /* Pad the packet by setting the first register again at the end because the register
   * count must be even and 2 consecutive offsets must not be equal.
   */
   radeon_emit(reg_pairs[i].reg_offset[0] | ((uint32_t)reg_pairs[0].reg_offset[0] << 16));
   radeon_emit(reg_pairs[i].reg_value[0]);
      }
      }
      #if GFX_VER == 6 /* declare this function only once because there is only one variant. */
      void gfx11_emit_buffered_compute_sh_regs(struct si_context *sctx)
   {
      gfx11_emit_buffered_sh_regs_inline(sctx, &sctx->num_buffered_compute_sh_regs,
      }
      #endif
      #define EMIT_SQTT_END_DRAW                                                     \
   do {                                                                         \
      if (GFX_VERSION >= GFX9 && unlikely(sctx->sqtt_enabled)) {                 \
      radeon_begin(&sctx->gfx_cs);                                             \
   radeon_emit(PKT3(PKT3_EVENT_WRITE, 0, 0));                               \
   radeon_emit(EVENT_TYPE(V_028A90_THREAD_TRACE_MARKER) | EVENT_INDEX(0));  \
         } while (0)
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
         static void si_emit_draw_packets(struct si_context *sctx, const struct pipe_draw_info *info,
                                       {
               if (unlikely(sctx->sqtt_enabled)) {
      si_sqtt_write_event_marker(sctx, &sctx->gfx_cs, sctx->sqtt_next_event,
                        if (!IS_DRAW_VERTEX_STATE && indirect && indirect->count_from_stream_output) {
               radeon_begin(cs);
   radeon_set_context_reg(R_028B30_VGT_STRMOUT_DRAW_OPAQUE_VERTEX_STRIDE, t->stride_in_dw);
            if (GFX_VERSION >= GFX9) {
                              radeon_emit(PKT3(PKT3_LOAD_CONTEXT_REG_INDEX, 3, 0));
   radeon_emit(va);
   radeon_emit(va >> 32);
                     } else {
      si_cp_copy_data(sctx, &sctx->gfx_cs, COPY_DATA_REG, NULL,
            }
   use_opaque = S_0287F0_USE_OPAQUE(1);
               uint32_t index_max_size = 0;
   uint64_t index_va = 0;
                     if (GFX_VERSION == GFX10_3) {
      /* Workaround for incorrect stats with adjacent primitive types
   * (see PAL's waDisableInstancePacking).
   */
   if (sctx->num_pipeline_stat_queries &&
      sctx->shader.gs.cso == NULL &&
   (instance_count > 1 || indirect) &&
   (1 << info->mode) & (1 << MESA_PRIM_LINES_ADJACENCY |
                                    /* draw packet */
   if (index_size) {
      /* Register shadowing doesn't shadow INDEX_TYPE. */
   if (index_size != sctx->last_index_size ||
                     /* Index type computation. When we look at how we need to translate index_size,
   * we can see that we just need 2 shifts to get the hw value.
   *
   * 1 = 001b --> 10b = 2
   * 2 = 010b --> 00b = 0
   * 4 = 100b --> 01b = 1
   */
                  if (GFX_VERSION <= GFX7 && UTIL_ARCH_BIG_ENDIAN) {
      /* GFX7 doesn't support ubyte indices. */
   index_type |= index_size == 2 ? V_028A7C_VGT_DMA_SWAP_16_BIT
               if (GFX_VERSION >= GFX9) {
      radeon_set_uconfig_reg_idx(sctx->screen, GFX_VERSION,
      } else {
      radeon_emit(PKT3(PKT3_INDEX_TYPE, 0, 0));
               sctx->last_index_size = index_size;
   if (GFX_VERSION == GFX10_3)
               index_max_size = (indexbuf->width0 - index_offset) >> util_logbase2(index_size);
   /* Skip draw calls with 0-sized index buffers.
   * They cause a hang on some chips, like Navi10-14.
   */
   if (!index_max_size) {
      radeon_end();
                        radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(indexbuf),
      } else {
      /* On GFX7 and later, non-indexed draws overwrite VGT_INDEX_TYPE,
   * so the state must be re-emitted before the next indexed draw.
   */
   if (GFX_VERSION >= GFX7)
         if (GFX_VERSION == GFX10_3 && disable_instance_packing != sctx->disable_instance_packing) {
      radeon_set_uconfig_reg_idx(sctx->screen, GFX_VERSION,
                              unsigned sh_base_reg = si_get_user_data_base(GFX_VERSION, HAS_TESS, HAS_GS, NGG,
         bool render_cond_bit = sctx->render_cond_enabled;
            if (HAS_TESS)
         else if (HAS_GS || NGG)
         else
            if (!IS_DRAW_VERTEX_STATE && indirect) {
      assert(num_draws == 1);
                     if (HAS_PAIRS) {
      radeon_end();
   gfx11_emit_buffered_sh_regs_inline(sctx, &sctx->num_buffered_gfx_sh_regs,
                     /* Invalidate tracked draw constants because DrawIndirect overwrites them. */
   sctx->tracked_regs.other_reg_saved_mask &=
                  radeon_emit(PKT3(PKT3_SET_BASE, 2, 0));
   radeon_emit(1);
   radeon_emit(indirect_va);
            radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, si_resource(indirect->buffer),
                              if (index_size) {
      radeon_emit(PKT3(PKT3_INDEX_BASE, 1, 0));
                  radeon_emit(PKT3(PKT3_INDEX_BUFFER_SIZE, 0, 0));
               if (!sctx->screen->has_draw_indirect_multi) {
      radeon_emit(PKT3(index_size ? PKT3_DRAW_INDEX_INDIRECT : PKT3_DRAW_INDIRECT, 3,
         radeon_emit(indirect->offset);
   radeon_emit((sh_base_reg + SI_SGPR_BASE_VERTEX * 4 - SI_SH_REG_OFFSET) >> 2);
   radeon_emit((sh_base_reg + SI_SGPR_START_INSTANCE * 4 - SI_SH_REG_OFFSET) >> 2);
      } else {
                                                            radeon_emit(PKT3(index_size ? PKT3_DRAW_INDEX_INDIRECT_MULTI : PKT3_DRAW_INDIRECT_MULTI, 8,
         radeon_emit(indirect->offset);
   radeon_emit((sh_base_reg + SI_SGPR_BASE_VERTEX * 4 - SI_SH_REG_OFFSET) >> 2);
   radeon_emit((sh_base_reg + SI_SGPR_START_INSTANCE * 4 - SI_SH_REG_OFFSET) >> 2);
   radeon_emit(((sh_base_reg + SI_SGPR_DRAWID * 4 - SI_SH_REG_OFFSET) >> 2) |
               radeon_emit(indirect->draw_count);
   radeon_emit(count_va);
   radeon_emit(count_va >> 32);
   radeon_emit(indirect->stride);
         } else {
      if (sctx->last_instance_count == SI_INSTANCE_COUNT_UNKNOWN ||
      sctx->last_instance_count != instance_count) {
   radeon_emit(PKT3(PKT3_NUM_INSTANCES, 0, 0));
   radeon_emit(instance_count);
               /* Base vertex and start instance. */
            bool set_draw_id = !IS_DRAW_VERTEX_STATE && sctx->vs_uses_draw_id;
   bool set_base_instance = sctx->vs_uses_base_instance;
            if (!is_blit) {
      /* Prefer SET_SH_REG_PAIRS_PACKED* on Gfx11+. */
   if (HAS_PAIRS) {
      radeon_opt_push_gfx_sh_reg(sh_base_reg + SI_SGPR_BASE_VERTEX * 4,
         if (set_draw_id) {
      radeon_opt_push_gfx_sh_reg(sh_base_reg + SI_SGPR_DRAWID * 4,
      }
   if (set_base_instance) {
      radeon_opt_push_gfx_sh_reg(sh_base_reg + SI_SGPR_START_INSTANCE * 4,
         } else {
      if (set_base_instance) {
      radeon_opt_set_sh_reg3(sctx, sh_base_reg + SI_SGPR_BASE_VERTEX * 4,
            } else if (set_draw_id) {
      radeon_opt_set_sh_reg2(sctx, sh_base_reg + SI_SGPR_BASE_VERTEX * 4,
      } else {
      radeon_opt_set_sh_reg(sctx, sh_base_reg + SI_SGPR_BASE_VERTEX * 4,
                     if (HAS_PAIRS) {
      radeon_end();
   gfx11_emit_buffered_sh_regs_inline(sctx, &sctx->num_buffered_gfx_sh_regs,
                     /* Blit SGPRs must be set after gfx11_emit_buffered_sh_regs_inline because they can
   * overwrite them.
   */
   if (is_blit) {
      /* Re-emit draw constants after we leave u_blitter. */
                  /* Blit VS doesn't use BASE_VERTEX, START_INSTANCE, and DRAWID. */
   radeon_set_sh_reg_seq(sh_base_reg + SI_SGPR_VS_BLIT_DATA * 4, sctx->num_vs_blit_sgprs);
               /* Don't update draw_id in the following code if it doesn't increment. */
   bool increment_draw_id = !IS_DRAW_VERTEX_STATE && num_draws > 1 &&
            if (index_size) {
      /* NOT_EOP allows merging multiple draws into 1 wave, but only user VGPRs
   * can be changed between draws, and GS fast launch must be disabled.
   * NOT_EOP doesn't work on gfx9 and older.
   *
   * Instead of doing this, which evaluates the case conditions repeatedly:
   *  for (all draws) {
   *    if (case1);
   *    else;
   *  }
   *
   * Use this structuring to evaluate the case conditions once:
   *  if (case1) for (all draws);
   *  else for (all draws);
   *
   */
                  if (increment_draw_id) {
      if (index_bias_varies) {
                        if (i > 0) {
                              radeon_emit(PKT3(PKT3_DRAW_INDEX_2, 4, render_cond_bit));
   radeon_emit(index_max_size);
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit(draws[i].count);
      }
   if (num_draws > 1) {
      sctx->tracked_regs.other_reg_saved_mask &=
         } else {
                                             radeon_emit(PKT3(PKT3_DRAW_INDEX_2, 4, render_cond_bit));
   radeon_emit(index_max_size);
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit(draws[i].count);
      }
   if (num_draws > 1) {
      sctx->tracked_regs.other_reg_saved_mask &=
            } else {
      if (index_bias_varies) {
                                             radeon_emit(PKT3(PKT3_DRAW_INDEX_2, 4, render_cond_bit));
   radeon_emit(index_max_size);
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit(draws[i].count);
      }
   if (num_draws > 1) {
      sctx->tracked_regs.other_reg_saved_mask &=
         } else {
      /* DrawID and BaseVertex are constant. */
   if (GFX_VERSION == GFX10) {
      /* GFX10 has a bug that consecutive draw packets with NOT_EOP must not have
   * count == 0 in the last draw (which doesn't set NOT_EOP).
   *
   * So remove all trailing draws with count == 0.
   */
                                       radeon_emit(PKT3(PKT3_DRAW_INDEX_2, 4, render_cond_bit));
   radeon_emit(index_max_size);
   radeon_emit(va);
   radeon_emit(va >> 32);
   radeon_emit(draws[i].count);
   radeon_emit(V_0287F0_DI_SRC_SEL_DMA |
               } else {
      if (increment_draw_id) {
      for (unsigned i = 0; i < num_draws; i++) {
                        radeon_set_sh_reg_seq(sh_base_reg + SI_SGPR_BASE_VERTEX * 4, 2);
                     radeon_emit(PKT3(PKT3_DRAW_INDEX_AUTO, 1, render_cond_bit));
   radeon_emit(draws[i].count);
      }
   if (num_draws > 1 && (IS_DRAW_VERTEX_STATE || !sctx->num_vs_blit_sgprs)) {
      sctx->tracked_regs.other_reg_saved_mask &=
         } else {
      for (unsigned i = 0; i < num_draws; i++) {
                     radeon_emit(PKT3(PKT3_DRAW_INDEX_AUTO, 1, render_cond_bit));
   radeon_emit(draws[i].count);
      }
   if (num_draws > 1 && (IS_DRAW_VERTEX_STATE || !sctx->num_vs_blit_sgprs)) {
      sctx->tracked_regs.other_reg_saved_mask &=
               }
               }
      /* Return false if not bound. */
   template<amd_gfx_level GFX_VERSION>
   static void ALWAYS_INLINE si_set_vb_descriptor(struct si_vertex_elements *velems,
                     {
      struct si_resource *buf = si_resource(vb->buffer.resource);
            if (!buf || offset >= buf->b.b.width0) {
      memset(desc, 0, 16);
               uint64_t va = buf->gpu_address + offset;
            int64_t num_records = (int64_t)buf->b.b.width0 - offset;
   if (GFX_VERSION != GFX8 && stride) {
      /* Round up by rounding down and adding 1 */
      }
                     /* OOB_SELECT chooses the out-of-bounds check:
   *  - 1: index >= NUM_RECORDS (Structured)
   *  - 3: offset >= NUM_RECORDS (Raw)
   */
   if (GFX_VERSION >= GFX10)
      rsrc_word3 |= S_008F0C_OOB_SELECT(stride ? V_008F0C_OOB_SELECT_STRUCTURED
         desc[0] = va;
   desc[1] = S_008F04_BASE_ADDRESS_HI(va >> 32) | S_008F04_STRIDE(stride);
   desc[2] = num_records;
      }
      #if GFX_VER == 6 /* declare this function only once because it supports all chips. */
      void si_set_vertex_buffer_descriptor(struct si_screen *sscreen, struct si_vertex_elements *velems,
               {
      switch (sscreen->info.gfx_level) {
   case GFX6:
      si_set_vb_descriptor<GFX6>(velems, vb, element_index, out);
      case GFX7:
      si_set_vb_descriptor<GFX7>(velems, vb, element_index, out);
      case GFX8:
      si_set_vb_descriptor<GFX8>(velems, vb, element_index, out);
      case GFX9:
      si_set_vb_descriptor<GFX9>(velems, vb, element_index, out);
      case GFX10:
      si_set_vb_descriptor<GFX10>(velems, vb, element_index, out);
      case GFX10_3:
      si_set_vb_descriptor<GFX10_3>(velems, vb, element_index, out);
      case GFX11:
      si_set_vb_descriptor<GFX11>(velems, vb, element_index, out);
      case GFX11_5:
      si_set_vb_descriptor<GFX11_5>(velems, vb, element_index, out);
      default:
            }
      #endif
      template<util_popcnt POPCNT>
   static ALWAYS_INLINE unsigned get_next_vertex_state_elem(struct pipe_vertex_state *state,
         {
      unsigned semantic_index = u_bit_scan(partial_velem_mask);
   assert(state->input.full_velem_mask & BITFIELD_BIT(semantic_index));
   /* A prefix mask of the full mask gives us the index in pipe_vertex_state. */
      }
      template<amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG>
   static unsigned get_vb_descriptor_sgpr_ptr_offset(void)
   {
      /* Find the location of the VB descriptor pointer. */
   unsigned dw_offset = SI_VS_NUM_USER_SGPR;
   if (GFX_VERSION >= GFX9) {
      if (HAS_TESS)
         else if (HAS_GS || NGG)
      }
      }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
               static bool si_upload_and_prefetch_VB_descriptors(struct si_context *sctx,
               {
      struct si_vertex_state *vstate = (struct si_vertex_state *)state;
   unsigned count = IS_DRAW_VERTEX_STATE ? util_bitcount_fast<POPCNT>(partial_velem_mask) :
         unsigned sh_base = si_get_user_data_base(GFX_VERSION, HAS_TESS, HAS_GS, NGG,
                           if (sctx->vertex_buffers_dirty || IS_DRAW_VERTEX_STATE) {
               struct si_vertex_elements *velems = sctx->vertex_elements;
   unsigned alloc_size = IS_DRAW_VERTEX_STATE ?
               uint64_t vb_descriptors_address = 0;
            if (alloc_size) {
               /* Vertex buffer descriptors are the only ones which are uploaded directly
   * and don't go through si_upload_graphics_shader_descriptors.
   */
   u_upload_alloc(sctx->b.const_uploader, 0, alloc_size,
                              radeon_add_to_buffer_list(sctx, &sctx->gfx_cs, sctx->last_const_upload_buffer,
                  /* GFX6 doesn't support the L2 prefetch. */
   if (GFX_VERSION >= GFX7) {
      uint64_t address = sctx->last_const_upload_buffer->gpu_address + offset;
                  unsigned count_in_user_sgprs = MIN2(count, num_vbos_in_user_sgprs);
            if (IS_DRAW_VERTEX_STATE) {
                                 /* the first iteration always executes */
                                                                              /* the first iteration always executes */
   do {
                     memcpy(desc, &vstate->descriptors[velem_index * 4], 16);
                        if (vstate->b.input.vbuffer.buffer.resource != vstate->b.input.indexbuf) {
      radeon_add_to_buffer_list(sctx, &sctx->gfx_cs,
                     /* The next draw_vbo should recompute and rebind vertex buffer descriptors. */
      } else {
      if (count_in_user_sgprs) {
      radeon_begin(&sctx->gfx_cs);
                  /* the first iteration always executes */
   do {
                                                            if (alloc_size) {
      /* the first iteration always executes */
   do {
                                       unsigned vb_desc_ptr_offset =
         radeon_begin(&sctx->gfx_cs);
   radeon_set_or_push_gfx_sh_reg(vb_desc_ptr_offset, vb_descriptors_address);
                                 }
      static void si_get_draw_start_count(struct si_context *sctx, const struct pipe_draw_info *info,
                     {
      if (indirect && !indirect->count_from_stream_output) {
      unsigned indirect_count;
   struct pipe_transfer *transfer;
   unsigned begin, end;
   unsigned map_size;
            if (indirect->indirect_draw_count) {
      data = (unsigned*)
                                    } else {
                  if (!indirect_count) {
      *start = *count = 0;
               map_size = (indirect_count - 1) * indirect->stride + 3 * sizeof(unsigned);
   data = (unsigned*)
                  begin = UINT_MAX;
            for (unsigned i = 0; i < indirect_count; ++i) {
                     if (count > 0) {
      begin = MIN2(begin, start);
                                    if (begin < end) {
      *start = begin;
      } else {
            } else {
      unsigned min_element = UINT_MAX;
            for (unsigned i = 0; i < num_draws; i++) {
      min_element = MIN2(min_element, draws[i].start);
               *start = min_element;
         }
      ALWAYS_INLINE
   static void si_emit_all_states(struct si_context *sctx, uint64_t skip_atom_mask)
   {
      /* Emit states by calling their emit functions. */
            if (dirty) {
               /* u_bit_scan64 is too slow on i386. */
   if (sizeof(void*) == 8) {
      do {
      unsigned i = u_bit_scan64(&dirty);
         } else {
                     while (dirty_lo) {
      unsigned i = u_bit_scan(&dirty_lo);
      }
   while (dirty_hi) {
      unsigned i = 32 + u_bit_scan(&dirty_hi);
               }
      #define DRAW_CLEANUP do {                                 \
         if (index_size && indexbuf != info->index.resource) \
            template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
               static void si_draw(struct pipe_context *ctx,
                     const struct pipe_draw_info *info,
   unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
   {
      /* Keep code that uses the least number of local variables as close to the beginning
   * of this function as possible to minimize register pressure.
   *
   * It doesn't matter where we return due to invalid parameters because such cases
   * shouldn't occur in practice.
   */
                     if (GFX_VERSION < GFX11)
         else
                     if (u_trace_perfetto_active(&sctx->ds.trace_context))
                     /* GFX6-GFX7 treat instance_count==0 as instance_count==1. There is
   * no workaround for indirect draws, but we can at least skip
   * direct draws.
   * 'instance_count == 0' seems to be problematic on Renoir chips (#4866),
   * so simplify the condition and drop these draws for all <= GFX9 chips.
   */
   if (GFX_VERSION <= GFX9 && unlikely(!IS_DRAW_VERTEX_STATE && !indirect && !instance_count))
            struct si_shader_selector *vs = sctx->shader.vs.cso;
   struct si_vertex_state *vstate = (struct si_vertex_state *)state;
   if (unlikely(!vs ||
               (!IS_DRAW_VERTEX_STATE && sctx->num_vertex_elements < vs->info.num_vs_inputs) ||
      assert(0);
                        if (GFX_VERSION <= GFX9 && HAS_GS) {
      /* Determine whether the GS triangle strip adjacency fix should
   * be applied. Rotate every other triangle if triangle strips with
   * adjacency are fed to the GS. This doesn't work if primitive
   * restart occurs after an odd number of triangles.
   */
   bool gs_tri_strip_adj_fix =
            if (gs_tri_strip_adj_fix != sctx->shader.gs.key.ge.mono.u.gs_tri_strip_adj_fix) {
      sctx->shader.gs.key.ge.mono.u.gs_tri_strip_adj_fix = gs_tri_strip_adj_fix;
                  struct pipe_resource *indexbuf = info->index.resource;
   unsigned index_size = info->index_size;
            if (index_size) {
      /* Translate or upload, if needed. */
   /* 8-bit indices are supported on GFX8. */
   if (!IS_DRAW_VERTEX_STATE && GFX_VERSION <= GFX7 && index_size == 1) {
                     si_get_draw_start_count(sctx, info, indirect, draws, num_draws, &start, &count);
                  indexbuf = NULL;
   u_upload_alloc(ctx->stream_uploader, start_offset, size,
                                 /* info->start will be added by the drawing code */
   index_offset = offset - start_offset;
      } else if (!IS_DRAW_VERTEX_STATE && info->has_user_indices) {
               assert(!indirect);
                  indexbuf = NULL;
   u_upload_data(ctx->stream_uploader, start_offset, draws[0].count * index_size,
                              /* info->start will be added by the drawing code */
      } else if (GFX_VERSION <= GFX7 && si_resource(indexbuf)->TC_L2_dirty) {
      /* GFX8 reads index buffers through TC L2, so it doesn't
   * need this. */
   sctx->flags |= SI_CONTEXT_WB_L2;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.cache_flush);
                  unsigned min_direct_count = 0;
            if (!IS_DRAW_VERTEX_STATE && indirect) {
      /* Indirect buffers use TC L2 on GFX9, but not older hw. */
   if (GFX_VERSION <= GFX8) {
      if (indirect->buffer && si_resource(indirect->buffer)->TC_L2_dirty) {
      sctx->flags |= SI_CONTEXT_WB_L2;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.cache_flush);
               if (indirect->indirect_draw_count &&
      si_resource(indirect->indirect_draw_count)->TC_L2_dirty) {
   sctx->flags |= SI_CONTEXT_WB_L2;
   si_mark_atom_dirty(sctx, &sctx->atoms.s.cache_flush);
         }
      } else {
               for (unsigned i = 1; i < num_draws; i++) {
               total_direct_count += count;
                  /* Set the rasterization primitive type.
   *
   * This must be done after si_decompress_textures, which can call
   * draw_vbo recursively, and before si_update_shaders, which uses
   * current_rast_prim for this draw_vbo call.
   */
   if (!HAS_GS && !HAS_TESS) {
               if (util_rast_prim_is_triangles(prim)) {
         } else {
      /* Only possibilities, POINTS, LINE*, RECTANGLES */
               si_set_rasterized_prim(sctx, rast_prim, si_get_vs_inline(sctx, HAS_TESS, HAS_GS)->current,
               if (IS_DRAW_VERTEX_STATE) {
      /* draw_vertex_state doesn't use the current vertex buffers and vertex elements,
   * so disable any non-trivial VS prolog that is based on them, such as vertex
   * format lowering.
   */
   if (!sctx->force_trivial_vs_prolog) {
               /* Update shaders to disable the non-trivial VS prolog. */
   if (sctx->uses_nontrivial_vs_prolog) {
      si_vs_key_update_inputs(sctx);
            } else {
      if (sctx->force_trivial_vs_prolog) {
               /* Update shaders to enable the non-trivial VS prolog. */
   if (sctx->uses_nontrivial_vs_prolog) {
      si_vs_key_update_inputs(sctx);
                     /* Update NGG culling settings. */
   uint16_t old_ngg_culling = sctx->ngg_culling;
   if (GFX_VERSION >= GFX10) {
               if (NGG &&
      /* Tessellation and GS set ngg_cull_vert_threshold to UINT_MAX if the prim type
   * is not points, so this check is only needed for VS. */
   (HAS_TESS || HAS_GS || util_rast_prim_is_lines_or_triangles(sctx->current_rast_prim)) &&
   /* Only the first draw for a shader starts with culling disabled and it's disabled
   * until we pass the total_direct_count check and then it stays enabled until
   * the shader is changed. This eliminates most culling on/off state changes. */
                                          if (util_prim_is_lines(sctx->current_rast_prim)) {
      /* Overwrite it to mask out face cull flags. */
      } else {
      ngg_culling = sctx->viewport0_y_inverted ? rs->ngg_cull_flags_tris_y_inverted :
                     if (ngg_culling != old_ngg_culling) {
      /* If shader compilation is not ready, this setting will be rejected. */
   sctx->ngg_culling = ngg_culling;
         } else if (old_ngg_culling) {
      sctx->ngg_culling = 0;
                  if (unlikely(sctx->do_update_shaders)) {
      if (unlikely(!(si_update_shaders<GFX_VERSION, HAS_TESS, HAS_GS, NGG>(sctx)))) {
      DRAW_CLEANUP;
                  /* This is the optimal packet order:
   * Set all states first, so that all SET packets are processed in parallel with previous
   * draw calls. Then flush caches and wait if needed. Then draw and prefetch at the end.
   * It's better to draw before prefetches because we want to start fetching indices before
   * shaders. The idea is to minimize the time when the CUs are idle.
            /* Vega10/Raven scissor bug workaround. When any context register is
   * written (i.e. the GPU rolls the context), PA_SC_VPORT_SCISSOR
   * registers must be written too.
   */
   bool gfx9_scissor_bug = false;
            if (GFX_VERSION == GFX9 && sctx->screen->info.has_gfx9_scissor_bug) {
      masked_atoms |= si_get_atom_bit(sctx, &sctx->atoms.s.scissors);
            if ((!IS_DRAW_VERTEX_STATE && indirect && indirect->count_from_stream_output) ||
      sctx->dirty_atoms & si_atoms_that_always_roll_context())
                     /* Emit states. */
   si_emit_rasterizer_prim_state<GFX_VERSION, HAS_GS, NGG>(sctx);
   /* This emits states and flushes caches. */
   si_emit_all_states(sctx, masked_atoms);
   /* This can be done after si_emit_all_states because it doesn't set cache flush flags. */
   si_emit_draw_registers<GFX_VERSION, HAS_TESS, HAS_GS, NGG, IS_DRAW_VERTEX_STATE>
                           /* This must be done after si_emit_all_states, which can affect this. */
   si_emit_vs_state<GFX_VERSION, HAS_TESS, HAS_GS, NGG, IS_DRAW_VERTEX_STATE, HAS_PAIRS>
            /* This needs to be done after cache flushes because ACQUIRE_MEM rolls the context. */
   if (GFX_VERSION == GFX9 && gfx9_scissor_bug &&
      (sctx->context_roll || si_is_atom_dirty(sctx, &sctx->atoms.s.scissors))) {
   sctx->atoms.s.scissors.emit(sctx, -1);
      }
            /* This uploads VBO descriptors, sets user SGPRs, and executes the L2 prefetch.
   * It should done after cache flushing.
   */
   if (unlikely((!si_upload_and_prefetch_VB_descriptors
                  DRAW_CLEANUP;
               si_emit_draw_packets<GFX_VERSION, HAS_TESS, HAS_GS, NGG, IS_DRAW_VERTEX_STATE, HAS_PAIRS>
         (sctx, info, drawid_offset, indirect, draws, num_draws, indexbuf,
            /* Start prefetches after the draw has been started. Both will run
   * in parallel, but starting the draw first is more important.
   */
            /* Clear the context roll flag after the draw call.
   * Only used by the gfx9 scissor bug.
   */
   if (GFX_VERSION == GFX9)
            if (unlikely(sctx->current_saved_cs)) {
      si_trace_emit(sctx);
               /* Workaround for a VGT hang when streamout is enabled.
   * It must be done after drawing. */
   if (((GFX_VERSION == GFX7 && sctx->family == CHIP_HAWAII) ||
      (GFX_VERSION == GFX8 && (sctx->family == CHIP_TONGA || sctx->family == CHIP_FIJI))) &&
   si_get_strmout_en(sctx)) {
   sctx->flags |= SI_CONTEXT_VGT_STREAMOUT_SYNC;
               if (unlikely(sctx->decompression_enabled)) {
         } else {
                  if (sctx->framebuffer.state.zsbuf) {
      struct si_texture *zstex = (struct si_texture *)sctx->framebuffer.state.zsbuf->texture;
               if (u_trace_perfetto_active(&sctx->ds.trace_context)) {
                     }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
         static void si_draw_vbo(struct pipe_context *ctx,
                           const struct pipe_draw_info *info,
   {
      si_draw<GFX_VERSION, HAS_TESS, HAS_GS, NGG, DRAW_VERTEX_STATE_OFF, HAS_PAIRS, POPCNT_NO>
      }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG,
         static void si_draw_vertex_state(struct pipe_context *ctx,
                                 {
      struct si_vertex_state *state = (struct si_vertex_state *)vstate;
            dinfo.mode = info.mode;
   dinfo.index_size = 4;
   dinfo.instance_count = 1;
            si_draw<GFX_VERSION, HAS_TESS, HAS_GS, NGG, DRAW_VERTEX_STATE_ON, HAS_PAIRS, POPCNT>
            if (info.take_vertex_state_ownership)
      }
      static void si_draw_rectangle(struct blitter_context *blitter, void *vertex_elements_cso,
                     {
      struct pipe_context *pipe = util_blitter_get_pipe(blitter);
   struct si_context *sctx = (struct si_context *)pipe;
   uint32_t attribute_ring_address_lo =
            /* Pack position coordinates as signed int16. */
   sctx->vs_blit_sh_data[0] = (uint32_t)(x1 & 0xffff) | ((uint32_t)(y1 & 0xffff) << 16);
   sctx->vs_blit_sh_data[1] = (uint32_t)(x2 & 0xffff) | ((uint32_t)(y2 & 0xffff) << 16);
            switch (type) {
   case UTIL_BLITTER_ATTRIB_COLOR:
      memcpy(&sctx->vs_blit_sh_data[3], attrib->color, sizeof(float) * 4);
   sctx->vs_blit_sh_data[7] = attribute_ring_address_lo;
      case UTIL_BLITTER_ATTRIB_TEXCOORD_XY:
   case UTIL_BLITTER_ATTRIB_TEXCOORD_XYZW:
      memcpy(&sctx->vs_blit_sh_data[3], &attrib->texcoord, sizeof(attrib->texcoord));
   sctx->vs_blit_sh_data[9] = attribute_ring_address_lo;
      case UTIL_BLITTER_ATTRIB_NONE:;
                     struct pipe_draw_info info = {};
            info.mode = SI_PRIM_RECTANGLE_LIST;
            draw.start = 0;
            /* Blits don't use vertex buffers. */
               }
      template <amd_gfx_level GFX_VERSION, si_has_tess HAS_TESS, si_has_gs HAS_GS, si_has_ngg NGG>
   static void si_init_draw_vbo(struct si_context *sctx)
   {
      if (NGG && GFX_VERSION < GFX10)
            if (!NGG && GFX_VERSION >= GFX11)
            if (GFX_VERSION >= GFX11 && sctx->screen->info.has_set_pairs_packets) {
      sctx->draw_vbo[HAS_TESS][HAS_GS][NGG] =
            if (util_get_cpu_caps()->has_popcnt) {
      sctx->draw_vertex_state[HAS_TESS][HAS_GS][NGG] =
      } else {
      sctx->draw_vertex_state[HAS_TESS][HAS_GS][NGG] =
         } else {
      sctx->draw_vbo[HAS_TESS][HAS_GS][NGG] =
            if (util_get_cpu_caps()->has_popcnt) {
      sctx->draw_vertex_state[HAS_TESS][HAS_GS][NGG] =
      } else {
      sctx->draw_vertex_state[HAS_TESS][HAS_GS][NGG] =
            }
      template <amd_gfx_level GFX_VERSION>
   static void si_init_draw_vbo_all_pipeline_options(struct si_context *sctx)
   {
      si_init_draw_vbo<GFX_VERSION, TESS_OFF, GS_OFF, NGG_OFF>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_OFF, GS_ON,  NGG_OFF>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_ON,  GS_OFF, NGG_OFF>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_ON,  GS_ON,  NGG_OFF>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_OFF, GS_OFF, NGG_ON>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_OFF, GS_ON,  NGG_ON>(sctx);
   si_init_draw_vbo<GFX_VERSION, TESS_ON,  GS_OFF, NGG_ON>(sctx);
      }
      static void si_invalid_draw_vbo(struct pipe_context *pipe,
                                 {
         }
      static void si_invalid_draw_vertex_state(struct pipe_context *ctx,
                                 {
         }
      extern "C"
   void GFX(si_init_draw_functions_)(struct si_context *sctx)
   {
                        /* Bind a fake draw_vbo, so that draw_vbo isn't NULL, which would skip
   * initialization of callbacks in upper layers (such as u_threaded_context).
   */
   sctx->b.draw_vbo = si_invalid_draw_vbo;
   sctx->b.draw_vertex_state = si_invalid_draw_vertex_state;
               }
