   /*
   * Copyright © 2022 Collabora Ltd. and Red Hat Inc.
   * SPDX-License-Identifier: MIT
   */
   #include "nvk_pipeline.h"
      #include "nvk_device.h"
   #include "nvk_physical_device.h"
   #include "nvk_shader.h"
      #include "vk_nir.h"
   #include "vk_pipeline.h"
   #include "vk_pipeline_layout.h"
      #include "nv_push.h"
      #include "nouveau_context.h"
      #include "compiler/spirv/nir_spirv.h"
      #include "nvk_cl9097.h"
   #include "nvk_clb197.h"
   #include "nvk_clc397.h"
      static void
   emit_pipeline_ts_state(struct nv_push *p,
         {
   }
      static void
   emit_pipeline_vp_state(struct nv_push *p,
         {
   }
      static void
   emit_pipeline_rs_state(struct nv_push *p,
         {
         }
      static void
   nvk_populate_fs_key(struct nvk_fs_key *key,
               {
               if (rp->pipeline_flags &
      VK_PIPELINE_CREATE_DEPTH_STENCIL_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT)
         if (ms == NULL || ms->rasterization_samples <= 1)
            key->msaa = ms->rasterization_samples;
   if (ms->sample_shading_enable &&
      (ms->rasterization_samples * ms->min_sample_shading) > 1.0)
   }
      static void
   emit_pipeline_ms_state(struct nv_push *p,
               {
      P_IMMD(p, NV9097, SET_ANTI_ALIAS_ALPHA_CONTROL, {
      .alpha_to_coverage   = ms->alpha_to_coverage_enable,
               P_MTHD(p, NV9097, SET_SAMPLE_MASK_X0_Y0);
   P_NV9097_SET_SAMPLE_MASK_X0_Y0(p, ms->sample_mask & 0xffff);
   P_NV9097_SET_SAMPLE_MASK_X1_Y0(p, ms->sample_mask & 0xffff);
   P_NV9097_SET_SAMPLE_MASK_X0_Y1(p, ms->sample_mask & 0xffff);
            const float min_sample_shading = force_max_samples ? 1 :
         uint32_t min_samples = ceilf(ms->rasterization_samples * min_sample_shading);
            P_IMMD(p, NV9097, SET_HYBRID_ANTI_ALIAS_CONTROL, {
      .passes = min_samples,
         }
      static uint32_t
   vk_to_nv9097_blend_op(VkBlendOp vk_op)
   {
   #define OP(vk, nv) [VK_BLEND_OP_##vk] = NV9097_SET_BLEND_COLOR_OP_V_OGL_##nv
      ASSERTED uint16_t vk_to_nv9097[] = {
      OP(ADD,              FUNC_ADD),
   OP(SUBTRACT,         FUNC_SUBTRACT),
   OP(REVERSE_SUBTRACT, FUNC_REVERSE_SUBTRACT),
   OP(MIN,              MIN),
      };
      #undef OP
            }
      static uint32_t
   vk_to_nv9097_blend_factor(VkBlendFactor vk_factor)
   {
   #define FACTOR(vk, nv) [VK_BLEND_FACTOR_##vk] = \
      NV9097_SET_BLEND_COLOR_SOURCE_COEFF_V_##nv
   ASSERTED uint16_t vk_to_nv9097[] = {
      FACTOR(ZERO,                     OGL_ZERO),
   FACTOR(ONE,                      OGL_ONE),
   FACTOR(SRC_COLOR,                OGL_SRC_COLOR),
   FACTOR(ONE_MINUS_SRC_COLOR,      OGL_ONE_MINUS_SRC_COLOR),
   FACTOR(DST_COLOR,                OGL_DST_COLOR),
   FACTOR(ONE_MINUS_DST_COLOR,      OGL_ONE_MINUS_DST_COLOR),
   FACTOR(SRC_ALPHA,                OGL_SRC_ALPHA),
   FACTOR(ONE_MINUS_SRC_ALPHA,      OGL_ONE_MINUS_SRC_ALPHA),
   FACTOR(DST_ALPHA,                OGL_DST_ALPHA),
   FACTOR(ONE_MINUS_DST_ALPHA,      OGL_ONE_MINUS_DST_ALPHA),
   FACTOR(CONSTANT_COLOR,           OGL_CONSTANT_COLOR),
   FACTOR(ONE_MINUS_CONSTANT_COLOR, OGL_ONE_MINUS_CONSTANT_COLOR),
   FACTOR(CONSTANT_ALPHA,           OGL_CONSTANT_ALPHA),
   FACTOR(ONE_MINUS_CONSTANT_ALPHA, OGL_ONE_MINUS_CONSTANT_ALPHA),
   FACTOR(SRC_ALPHA_SATURATE,       OGL_SRC_ALPHA_SATURATE),
   FACTOR(SRC1_COLOR,               OGL_SRC1COLOR),
   FACTOR(ONE_MINUS_SRC1_COLOR,     OGL_INVSRC1COLOR),
   FACTOR(SRC1_ALPHA,               OGL_SRC1ALPHA),
      };
      #undef FACTOR
            }
      static void
   emit_pipeline_cb_state(struct nv_push *p,
         {
               for (uint32_t a = 0; a < cb->attachment_count; a++) {
      const struct vk_color_blend_attachment_state *att = &cb->attachments[a];
            P_MTHD(p, NV9097, SET_BLEND_PER_TARGET_SEPARATE_FOR_ALPHA(a));
   P_NV9097_SET_BLEND_PER_TARGET_SEPARATE_FOR_ALPHA(p, a, ENABLE_TRUE);
   P_NV9097_SET_BLEND_PER_TARGET_COLOR_OP(p, a,
         P_NV9097_SET_BLEND_PER_TARGET_COLOR_SOURCE_COEFF(p, a,
         P_NV9097_SET_BLEND_PER_TARGET_COLOR_DEST_COEFF(p, a,
         P_NV9097_SET_BLEND_PER_TARGET_ALPHA_OP(p, a,
         P_NV9097_SET_BLEND_PER_TARGET_ALPHA_SOURCE_COEFF(p, a,
         P_NV9097_SET_BLEND_PER_TARGET_ALPHA_DEST_COEFF(p, a,
         }
      static void
   emit_pipeline_ct_write_state(struct nv_push *p,
               {
      uint32_t att_write_masks[8] = {};
            if (rp != NULL) {
      att_count = rp->color_attachment_count;
   for (uint32_t a = 0; a < rp->color_attachment_count; a++) {
      VkFormat att_format = rp->color_attachment_formats[a];
                  if (cb != NULL) {
      assert(cb->attachment_count == att_count);
   for (uint32_t a = 0; a < cb->attachment_count; a++)
               bool indep_color_masks = true;
   for (uint32_t a = 0; a < att_count; a++) {
      P_IMMD(p, NV9097, SET_CT_WRITE(a), {
      .r_enable = (att_write_masks[a] & BITFIELD_BIT(0)) != 0,
   .g_enable = (att_write_masks[a] & BITFIELD_BIT(1)) != 0,
   .b_enable = (att_write_masks[a] & BITFIELD_BIT(2)) != 0,
               if (att_write_masks[a] != att_write_masks[0])
                  }
      static void
   emit_pipeline_xfb_state(struct nv_push *p,
         {
      const uint8_t max_buffers = 4;
   for (uint8_t b = 0; b < max_buffers; ++b) {
      const uint32_t var_count = xfb->varying_count[b];
   P_MTHD(p, NV9097, SET_STREAM_OUT_CONTROL_STREAM(b));
   P_NV9097_SET_STREAM_OUT_CONTROL_STREAM(p, b, xfb->stream[b]);
   P_NV9097_SET_STREAM_OUT_CONTROL_COMPONENT_COUNT(p, b, var_count);
            /* upload packed varying indices in multiples of 4 bytes */
   const uint32_t n = (var_count + 3) / 4;
   if (n > 0) {
      P_MTHD(p, NV9097, SET_STREAM_OUT_LAYOUT_SELECT(b, 0));
            }
      static const uint32_t mesa_to_nv9097_shader_type[] = {
      [MESA_SHADER_VERTEX]    = NV9097_SET_PIPELINE_SHADER_TYPE_VERTEX,
   [MESA_SHADER_TESS_CTRL] = NV9097_SET_PIPELINE_SHADER_TYPE_TESSELLATION_INIT,
   [MESA_SHADER_TESS_EVAL] = NV9097_SET_PIPELINE_SHADER_TYPE_TESSELLATION,
   [MESA_SHADER_GEOMETRY]  = NV9097_SET_PIPELINE_SHADER_TYPE_GEOMETRY,
      };
      static void
   emit_tessellation_paramaters(struct nv_push *p,
               {
      const uint32_t cw = NV9097_SET_TESSELLATION_PARAMETERS_OUTPUT_PRIMITIVES_TRIANGLES_CW;
   const uint32_t ccw = NV9097_SET_TESSELLATION_PARAMETERS_OUTPUT_PRIMITIVES_TRIANGLES_CCW;
   uint32_t output_prims = shader->tp.output_prims;
   /* When the origin is lower-left, we have to flip the winding order */
   if (state->domain_origin == VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT) {
      if (output_prims == cw) {
         } else if (output_prims == ccw) {
            }
   P_MTHD(p, NV9097, SET_TESSELLATION_PARAMETERS);
   P_NV9097_SET_TESSELLATION_PARAMETERS(p, {
      shader->tp.domain_type,
   shader->tp.spacing,
         }
      static void
   merge_tess_info(struct shader_info *tes_info, struct shader_info *tcs_info)
   {
      /* The Vulkan 1.0.38 spec, section 21.1 Tessellator says:
   *
   *    "PointMode. Controls generation of points rather than triangles
   *     or lines. This functionality defaults to disabled, and is
   *     enabled if either shader stage includes the execution mode.
   *
   * and about Triangles, Quads, IsoLines, VertexOrderCw, VertexOrderCcw,
   * PointMode, SpacingEqual, SpacingFractionalEven, SpacingFractionalOdd,
   * and OutputVertices, it says:
   *
   *    "One mode must be set in at least one of the tessellation
   *     shader stages."
   *
   * So, the fields can be set in either the TCS or TES, but they must
   * agree if set in both.  Our backend looks at TES, so bitwise-or in
   * the values from the TCS.
   */
   assert(tcs_info->tess.tcs_vertices_out == 0 || tes_info->tess.tcs_vertices_out == 0 ||
                  assert(tcs_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
         tes_info->tess.spacing == TESS_SPACING_UNSPECIFIED ||
            assert(tcs_info->tess._primitive_mode == TESS_PRIMITIVE_UNSPECIFIED ||
         tes_info->tess._primitive_mode == TESS_PRIMITIVE_UNSPECIFIED ||
   tes_info->tess._primitive_mode |= tcs_info->tess._primitive_mode;
   tes_info->tess.ccw |= tcs_info->tess.ccw;
            /* Copy the merged info back to the TCS */
   tcs_info->tess.tcs_vertices_out = tes_info->tess.tcs_vertices_out;
   tcs_info->tess.spacing = tes_info->tess.spacing;
   tcs_info->tess._primitive_mode = tes_info->tess._primitive_mode;
   tcs_info->tess.ccw = tes_info->tess.ccw;
      }
      VkResult
   nvk_graphics_pipeline_create(struct nvk_device *dev,
                           {
      VK_FROM_HANDLE(vk_pipeline_layout, pipeline_layout, pCreateInfo->layout);
   struct nvk_physical_device *pdev = nvk_device_physical(dev);
   struct nvk_graphics_pipeline *pipeline;
            pipeline = vk_object_zalloc(&dev->vk, pAllocator, sizeof(*pipeline),
         if (pipeline == NULL)
                     struct vk_graphics_pipeline_all_state all;
   struct vk_graphics_pipeline_state state = {};
   result = vk_graphics_pipeline_state_fill(&dev->vk, &state, pCreateInfo,
                  nir_shader *nir[MESA_SHADER_STAGES] = {};
            for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &pCreateInfo->pStages[i];
            vk_pipeline_robustness_state_fill(&dev->vk, &robustness[stage],
            result = nvk_shader_stage_to_nir(dev, sinfo, &robustness[stage],
         if (result != VK_SUCCESS)
               if (nir[MESA_SHADER_TESS_CTRL] && nir[MESA_SHADER_TESS_EVAL]) {
      nir_lower_patch_vertices(nir[MESA_SHADER_TESS_EVAL], nir[MESA_SHADER_TESS_CTRL]->info.tess.tcs_vertices_out, NULL);
               for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
      const VkPipelineShaderStageCreateInfo *sinfo = &pCreateInfo->pStages[i];
   gl_shader_stage stage = vk_to_mesa_shader_stage(sinfo->stage);
   nvk_lower_nir(dev, nir[stage], &robustness[stage],
               for (gl_shader_stage stage = 0; stage < MESA_SHADER_STAGES; stage++) {
      if (nir[stage] == NULL)
            struct nvk_fs_key fs_key_tmp, *fs_key = NULL;
   if (stage == MESA_SHADER_FRAGMENT) {
      nvk_populate_fs_key(&fs_key_tmp, state.ms, state.rp);
               result = nvk_compile_nir(pdev, nir[stage], fs_key,
         ralloc_free(nir[stage]);
   if (result != VK_SUCCESS)
            result = nvk_shader_upload(dev, &pipeline->base.shaders[stage]);
   if (result != VK_SUCCESS)
               struct nv_push push;
   nv_push_init(&push, pipeline->push_data, ARRAY_SIZE(pipeline->push_data));
                     struct nvk_shader *last_geom = NULL;
   for (gl_shader_stage stage = 0; stage <= MESA_SHADER_FRAGMENT; stage++) {
      struct nvk_shader *shader = &pipeline->base.shaders[stage];
            P_IMMD(p, NV9097, SET_PIPELINE_SHADER(idx), {
      .enable  = shader->upload_size > 0,
               if (shader->upload_size == 0)
            if (stage != MESA_SHADER_FRAGMENT)
            uint64_t addr = nvk_shader_address(shader);
   if (dev->pdev->info.cls_eng3d >= VOLTA_A) {
      P_MTHD(p, NVC397, SET_PIPELINE_PROGRAM_ADDRESS_A(idx));
   P_NVC397_SET_PIPELINE_PROGRAM_ADDRESS_A(p, idx, addr >> 32);
      } else {
      assert(addr < 0xffffffff);
                        switch (stage) {
   case MESA_SHADER_VERTEX:
   case MESA_SHADER_GEOMETRY:
            case MESA_SHADER_FRAGMENT:
      P_IMMD(p, NV9097, SET_SUBTILING_PERF_KNOB_A, {
      .fraction_of_spm_register_file_per_subtile         = 0x10,
   .fraction_of_spm_pixel_output_buffer_per_subtile   = 0x40,
   .fraction_of_spm_triangle_ram_per_subtile          = 0x16,
                              if (dev->pdev->info.cls_eng3d >= MAXWELL_B) {
      P_IMMD(p, NVB197, SET_POST_Z_PS_IMASK,
      } else {
                                 /* If we're using the incoming sample mask and doing sample shading,
   * we have to do sample shading "to the max", otherwise there's no
   * way to tell which sets of samples are covered by the current
   * invocation.
   */
   force_max_samples = shader->fs.sample_mask_in ||
               case MESA_SHADER_TESS_CTRL:
   case MESA_SHADER_TESS_EVAL:
      if (shader->tp.domain_type != ~0) {
                     default:
                     const uint8_t clip_cull = last_geom->vs.clip_enable |
         if (clip_cull) {
      P_IMMD(p, NV9097, SET_USER_CLIP_ENABLE, {
      .plane0 = (clip_cull >> 0) & 1,
   .plane1 = (clip_cull >> 1) & 1,
   .plane2 = (clip_cull >> 2) & 1,
   .plane3 = (clip_cull >> 3) & 1,
   .plane4 = (clip_cull >> 4) & 1,
   .plane5 = (clip_cull >> 5) & 1,
   .plane6 = (clip_cull >> 6) & 1,
      });
   P_IMMD(p, NV9097, SET_USER_CLIP_OP, {
      .plane0 = (last_geom->vs.cull_enable >> 0) & 1,
   .plane1 = (last_geom->vs.cull_enable >> 1) & 1,
   .plane2 = (last_geom->vs.cull_enable >> 2) & 1,
   .plane3 = (last_geom->vs.cull_enable >> 3) & 1,
   .plane4 = (last_geom->vs.cull_enable >> 4) & 1,
   .plane5 = (last_geom->vs.cull_enable >> 5) & 1,
   .plane6 = (last_geom->vs.cull_enable >> 6) & 1,
                  /* TODO: prog_selects_layer */
   P_IMMD(p, NV9097, SET_RT_LAYER, {
      .v       = 0,
   .control = (last_geom->hdr[13] & (1 << 9)) ?
                     if (last_geom->xfb) {
                  if (state.ts) emit_pipeline_ts_state(&push, state.ts);
   if (state.vp) emit_pipeline_vp_state(&push, state.vp);
   if (state.rs) emit_pipeline_rs_state(&push, state.rs);
   if (state.ms) emit_pipeline_ms_state(&push, state.ms, force_max_samples);
   if (state.cb) emit_pipeline_cb_state(&push, state.cb);
                     pipeline->dynamic.vi = &pipeline->_dynamic_vi;
   pipeline->dynamic.ms.sample_locations = &pipeline->_dynamic_sl;
                           fail:
      vk_object_free(&dev->vk, pAllocator, pipeline);
      }
