   /*
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
      #include <assert.h>
   #include <stdbool.h>
   #include <string.h>
   #include <unistd.h>
   #include <fcntl.h>
      #include "anv_private.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
   #include "common/intel_guardband.h"
   #include "compiler/brw_prim.h"
      const uint32_t genX(vk_to_intel_blend)[] = {
      [VK_BLEND_FACTOR_ZERO]                    = BLENDFACTOR_ZERO,
   [VK_BLEND_FACTOR_ONE]                     = BLENDFACTOR_ONE,
   [VK_BLEND_FACTOR_SRC_COLOR]               = BLENDFACTOR_SRC_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR]     = BLENDFACTOR_INV_SRC_COLOR,
   [VK_BLEND_FACTOR_DST_COLOR]               = BLENDFACTOR_DST_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR]     = BLENDFACTOR_INV_DST_COLOR,
   [VK_BLEND_FACTOR_SRC_ALPHA]               = BLENDFACTOR_SRC_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA]     = BLENDFACTOR_INV_SRC_ALPHA,
   [VK_BLEND_FACTOR_DST_ALPHA]               = BLENDFACTOR_DST_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA]     = BLENDFACTOR_INV_DST_ALPHA,
   [VK_BLEND_FACTOR_CONSTANT_COLOR]          = BLENDFACTOR_CONST_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR]= BLENDFACTOR_INV_CONST_COLOR,
   [VK_BLEND_FACTOR_CONSTANT_ALPHA]          = BLENDFACTOR_CONST_ALPHA,
   [VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA]= BLENDFACTOR_INV_CONST_ALPHA,
   [VK_BLEND_FACTOR_SRC_ALPHA_SATURATE]      = BLENDFACTOR_SRC_ALPHA_SATURATE,
   [VK_BLEND_FACTOR_SRC1_COLOR]              = BLENDFACTOR_SRC1_COLOR,
   [VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR]    = BLENDFACTOR_INV_SRC1_COLOR,
   [VK_BLEND_FACTOR_SRC1_ALPHA]              = BLENDFACTOR_SRC1_ALPHA,
      };
      static const uint32_t genX(vk_to_intel_blend_op)[] = {
      [VK_BLEND_OP_ADD]                         = BLENDFUNCTION_ADD,
   [VK_BLEND_OP_SUBTRACT]                    = BLENDFUNCTION_SUBTRACT,
   [VK_BLEND_OP_REVERSE_SUBTRACT]            = BLENDFUNCTION_REVERSE_SUBTRACT,
   [VK_BLEND_OP_MIN]                         = BLENDFUNCTION_MIN,
      };
      static void
   genX(streamout_prologue)(struct anv_cmd_buffer *cmd_buffer)
   {
   #if GFX_VERx10 >= 120
      /* Wa_16013994831 - Disable preemption during streamout, enable back
   * again if XFB not used by the current pipeline.
   *
   * Although this workaround applies to Gfx12+, we already disable object
   * level preemption for another reason in genX_state.c so we can skip this
   * for Gfx12.
   */
   if (!intel_needs_workaround(cmd_buffer->device->info, 16013994831))
            if (cmd_buffer->state.gfx.pipeline->uses_xfb) {
      genX(cmd_buffer_set_preemption)(cmd_buffer, false);
               if (!cmd_buffer->state.gfx.object_preemption)
      #endif
   }
      #if GFX_VER >= 12
   static uint32_t
   get_cps_state_offset(struct anv_device *device, bool cps_enabled,
         {
      if (!cps_enabled)
            uint32_t offset;
   static const uint32_t size_index[] = {
      [1] = 0,
   [2] = 1,
            #if GFX_VERx10 >= 125
      offset =
      1 + /* skip disabled */
   fsr->combiner_ops[0] * 5 * 3 * 3 +
   fsr->combiner_ops[1] * 3 * 3 +
   size_index[fsr->fragment_size.width] * 3 +
   #else
      offset =
      1 + /* skip disabled */
   size_index[fsr->fragment_size.width] * 3 +
   #endif
                     }
   #endif /* GFX_VER >= 12 */
      UNUSED static bool
   want_stencil_pma_fix(struct anv_cmd_buffer *cmd_buffer,
         {
      if (GFX_VER > 9)
                  /* From the Skylake PRM Vol. 2c CACHE_MODE_1::STC PMA Optimization Enable:
   *
   *    Clearing this bit will force the STC cache to wait for pending
   *    retirement of pixels at the HZ-read stage and do the STC-test for
   *    Non-promoted, R-computed and Computed depth modes instead of
   *    postponing the STC-test to RCPFE.
   *
   *    STC_TEST_EN = 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
   *                  3DSTATE_WM_DEPTH_STENCIL::StencilTestEnable
   *
   *    STC_WRITE_EN = 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
   *                   (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
   *                    3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE)
   *
   *    COMP_STC_EN = STC_TEST_EN &&
   *                  3DSTATE_PS_EXTRA::PixelShaderComputesStencil
   *
   *    SW parses the pipeline states to generate the following logical
   *    signal indicating if PMA FIX can be enabled.
   *
   *    STC_PMA_OPT =
   *       3DSTATE_WM::ForceThreadDispatch != 1 &&
   *       !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0) &&
   *       3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL &&
   *       3DSTATE_DEPTH_BUFFER::HIZ Enable &&
   *       !(3DSTATE_WM::EDSC_Mode == 2) &&
   *       3DSTATE_PS_EXTRA::PixelShaderValid &&
   *       !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
   *         3DSTATE_WM_HZ_OP::DepthBufferResolve ||
   *         3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
   *         3DSTATE_WM_HZ_OP::StencilBufferClear) &&
   *       (COMP_STC_EN || STC_WRITE_EN) &&
   *       ((3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
   *         3DSTATE_WM::ForceKillPix == ON ||
   *         3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
   *         3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
   *         3DSTATE_PS_BLEND::AlphaTestEnable ||
   *         3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) ||
   *        (3DSTATE_PS_EXTRA::Pixel Shader Computed Depth mode != PSCDEPTH_OFF))
            /* These are always true:
   *    3DSTATE_WM::ForceThreadDispatch != 1 &&
   *    !(3DSTATE_RASTER::ForceSampleCount != NUMRASTSAMPLES_0)
            /* We only enable the PMA fix if we know for certain that HiZ is enabled.
   * If we don't know whether HiZ is enabled or not, we disable the PMA fix
   * and there is no harm.
   *
   * (3DSTATE_DEPTH_BUFFER::SURFACE_TYPE != NULL) &&
   * 3DSTATE_DEPTH_BUFFER::HIZ Enable
   */
   if (!cmd_buffer->state.hiz_enabled)
            /* We can't possibly know if HiZ is enabled without the depth attachment */
   ASSERTED const struct anv_image_view *d_iview =
                  /* 3DSTATE_PS_EXTRA::PixelShaderValid */
   struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   if (!anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT))
            /* !(3DSTATE_WM::EDSC_Mode == 2) */
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   if (wm_prog_data->early_fragment_tests)
            /* We never use anv_pipeline for HiZ ops so this is trivially true:
   *    !(3DSTATE_WM_HZ_OP::DepthBufferClear ||
   *      3DSTATE_WM_HZ_OP::DepthBufferResolve ||
   *      3DSTATE_WM_HZ_OP::Hierarchical Depth Buffer Resolve Enable ||
   *      3DSTATE_WM_HZ_OP::StencilBufferClear)
            /* 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
   * 3DSTATE_WM_DEPTH_STENCIL::StencilTestEnable
   */
            /* 3DSTATE_STENCIL_BUFFER::STENCIL_BUFFER_ENABLE &&
   * (3DSTATE_WM_DEPTH_STENCIL::Stencil Buffer Write Enable &&
   *  3DSTATE_DEPTH_BUFFER::STENCIL_WRITE_ENABLE)
   */
            /* STC_TEST_EN && 3DSTATE_PS_EXTRA::PixelShaderComputesStencil */
            /* COMP_STC_EN || STC_WRITE_EN */
   if (!(comp_stc_en || stc_write_en))
            /* (3DSTATE_PS_EXTRA::PixelShaderKillsPixels ||
   *  3DSTATE_WM::ForceKillPix == ON ||
   *  3DSTATE_PS_EXTRA::oMask Present to RenderTarget ||
   *  3DSTATE_PS_BLEND::AlphaToCoverageEnable ||
   *  3DSTATE_PS_BLEND::AlphaTestEnable ||
   *  3DSTATE_WM_CHROMAKEY::ChromaKeyKillEnable) ||
   * (3DSTATE_PS_EXTRA::Pixel Shader Computed Depth mode != PSCDEPTH_OFF)
   */
   return pipeline->kill_pixel ||
      }
      static void
   genX(rasterization_mode)(VkPolygonMode raster_mode,
                           {
      if (raster_mode == VK_POLYGON_MODE_LINE) {
      /* Unfortunately, configuring our line rasterization hardware on gfx8
   * and later is rather painful.  Instead of giving us bits to tell the
   * hardware what line mode to use like we had on gfx7, we now have an
   * arcane combination of API Mode and MSAA enable bits which do things
   * in a table which are expected to magically put the hardware into the
   * right mode for your API.  Sadly, Vulkan isn't any of the APIs the
   * hardware people thought of so nothing works the way you want it to.
   *
   * Look at the table titled "Multisample Rasterization Modes" in Vol 7
   * of the Skylake PRM for more details.
   */
   switch (line_mode) {
   case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_EXT:
   #if GFX_VER <= 9
            /* Prior to ICL, the algorithm the HW uses to draw wide lines
   * doesn't quite match what the CTS expects, at least for rectangular
   * lines, so we set this to false here, making it draw parallelograms
   * instead, which work well enough.
      #else
         #endif
                  case VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH_EXT:
   case VK_LINE_RASTERIZATION_MODE_BRESENHAM_EXT:
      *api_mode = DX9OGL;
               default:
            } else {
      *api_mode = DX101;
         }
      /**
   * This function takes the vulkan runtime values & dirty states and updates
   * the values in anv_gfx_dynamic_state, flagging HW instructions for
   * reemission if the values are changing.
   *
   * Nothing is emitted in the batch buffer.
   */
   void
   genX(cmd_buffer_flush_gfx_runtime_state)(struct anv_cmd_buffer *cmd_buffer)
   {
      UNUSED struct anv_device *device = cmd_buffer->device;
   struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
   const struct anv_graphics_pipeline *pipeline = gfx->pipeline;
   const struct vk_dynamic_graphics_state *dyn =
         struct anv_gfx_dynamic_state *hw_state = &gfx->dyn_state;
         #define GET(field) hw_state->field
   #define SET(bit, field, value)                               \
      do {                                                      \
      __typeof(hw_state->field) __v = value;                 \
   if (hw_state->field != __v) {                          \
      hw_state->field = __v;                              \
            #define SET_STAGE(bit, field, value, stage)                  \
      do {                                                      \
      __typeof(hw_state->field) __v = value;                 \
   if (!anv_pipeline_has_stage(pipeline,                  \
            hw_state->field = __v;                              \
      }                                                      \
   if (hw_state->field != __v) {                          \
      hw_state->field = __v;                              \
               #define SETUP_PROVOKING_VERTEX(bit, cmd, mode)                         \
      switch (mode) {                                                     \
   case VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT:                     \
      SET(bit, cmd.TriangleStripListProvokingVertexSelect, 0);         \
   SET(bit, cmd.LineStripListProvokingVertexSelect,     0);         \
   SET(bit, cmd.TriangleFanProvokingVertexSelect,       1);         \
      case VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT:                      \
      SET(bit, cmd.TriangleStripListProvokingVertexSelect, 2);         \
   SET(bit, cmd.LineStripListProvokingVertexSelect,     1);         \
   SET(bit, cmd.TriangleFanProvokingVertexSelect,       2);         \
      default:                                                            \
                  if ((gfx->dirty & (ANV_CMD_DIRTY_PIPELINE |
                  BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_RASTERIZER_DISCARD_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_RASTERIZATION_STREAM) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_PROVOKING_VERTEX)) {
   SET(STREAMOUT, so.RenderingDisable, dyn->rs.rasterizer_discard_enable);
      #if INTEL_NEEDS_WA_18022508906
         /* Wa_18022508906 :
   *
   * SKL PRMs, Volume 7: 3D-Media-GPGPU, Stream Output Logic (SOL) Stage:
   *
   * SOL_INT::Render_Enable =
   *   (3DSTATE_STREAMOUT::Force_Rending == Force_On) ||
   *   (
   *     (3DSTATE_STREAMOUT::Force_Rending != Force_Off) &&
   *     !(3DSTATE_GS::Enable && 3DSTATE_GS::Output Vertex Size == 0) &&
   *     !3DSTATE_STREAMOUT::API_Render_Disable &&
   *     (
   *       3DSTATE_DEPTH_STENCIL_STATE::Stencil_TestEnable ||
   *       3DSTATE_DEPTH_STENCIL_STATE::Depth_TestEnable ||
   *       3DSTATE_DEPTH_STENCIL_STATE::Depth_WriteEnable ||
   *       3DSTATE_PS_EXTRA::PS_Valid ||
   *       3DSTATE_WM::Legacy Depth_Buffer_Clear ||
   *       3DSTATE_WM::Legacy Depth_Buffer_Resolve_Enable ||
   *       3DSTATE_WM::Legacy Hierarchical_Depth_Buffer_Resolve_Enable
   *     )
   *   )
   *
   * If SOL_INT::Render_Enable is false, the SO stage will not forward any
   * topologies down the pipeline. Which is not what we want for occlusion
   * queries.
   *
   * Here we force rendering to get SOL_INT::Render_Enable when occlusion
   * queries are active.
   */
   SET(STREAMOUT, so.ForceRendering,
         #endif
            switch (dyn->rs.provoking_vertex) {
   case VK_PROVOKING_VERTEX_MODE_FIRST_VERTEX_EXT:
      SET(STREAMOUT, so.ReorderMode, LEADING);
               case VK_PROVOKING_VERTEX_MODE_LAST_VERTEX_EXT:
      SET(STREAMOUT, so.ReorderMode, TRAILING);
               default:
                     if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_TOPOLOGY)) {
   uint32_t topology;
   if (anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL))
         else
                                 if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDINGS_VALID) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VI_BINDING_STRIDES))
      #if GFX_VER >= 11
      if (cmd_buffer->device->vk.enabled_extensions.KHR_fragment_shading_rate &&
      (gfx->dirty & ANV_CMD_DIRTY_PIPELINE ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_FSR))) {
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   const bool cps_enable = wm_prog_data &&
   #if GFX_VER == 11
         SET(CPS, cps.CoarsePixelShadingMode,
         SET(CPS, cps.MinCPSizeX, dyn->fsr.fragment_size.width);
   #elif GFX_VER >= 12
         SET(CPS, cps.CoarsePixelShadingStateArrayPointer,
   #endif
         #endif /* GFX_VER >= 11 */
         if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_TS_DOMAIN_ORIGIN)) {
            if (tes_prog_data && anv_pipeline_has_stage(pipeline, MESA_SHADER_TESS_EVAL)) {
      if (dyn->ts.domain_origin == VK_TESSELLATION_DOMAIN_ORIGIN_LOWER_LEFT) {
         } else {
      /* When the origin is upper-left, we have to flip the winding order */
   if (tes_prog_data->output_topology == OUTPUT_TRI_CCW) {
         } else if (tes_prog_data->output_topology == OUTPUT_TRI_CW) {
         } else {
               } else {
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_WIDTH))
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_PROVOKING_VERTEX)) {
      SETUP_PROVOKING_VERTEX(SF, sf, dyn->rs.provoking_vertex);
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS)) {
      /**
   * From the Vulkan Spec:
   *
   *    "VK_DEPTH_BIAS_REPRESENTATION_FLOAT_EXT specifies that the depth
   *     bias representation is a factor of constant r equal to 1."
   *
   * From the SKL PRMs, Volume 7: 3D-Media-GPGPU, Depth Offset:
   *
   *    "When UNORM Depth Buffer is at Output Merger (or no Depth Buffer):
   *
   *     Bias = GlobalDepthOffsetConstant * r + GlobalDepthOffsetScale * MaxDepthSlope
   *
   *     Where r is the minimum representable value > 0 in the depth
   *     buffer format, converted to float32 (note: If state bit Legacy
   *     Global Depth Bias Enable is set, the r term will be forced to
   *     1.0)"
   *
   * When VK_DEPTH_BIAS_REPRESENTATION_FLOAT_EXT is set, enable
   * LegacyGlobalDepthBiasEnable.
   */
   SET(SF, sf.LegacyGlobalDepthBiasEnable,
      dyn->rs.depth_bias.representation ==
            if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE))
            if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      (gfx->dirty & ANV_CMD_DIRTY_RENDER_TARGETS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_TOPOLOGY) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_CULL_MODE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_FRONT_FACE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_POLYGON_MODE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_MODE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_WIDTH) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_CLIP_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_CLAMP_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_CONSERVATIVE_MODE)) {
   /* Take dynamic primitive topology in to account with
   *    3DSTATE_RASTER::APIMode
   *    3DSTATE_RASTER::DXMultisampleRasterizationEnable
   *    3DSTATE_RASTER::AntialiasingEnable
   */
   uint32_t api_mode = 0;
            const VkLineRasterizationModeEXT line_mode =
                  const VkPolygonMode dynamic_raster_mode =
      genX(raster_polygon_mode)(gfx->pipeline,
               genX(rasterization_mode)(dynamic_raster_mode,
               /* From the Browadwell PRM, Volume 2, documentation for
      * 3DSTATE_RASTER, "Antialiasing Enable":
   *
   * "This field must be disabled if any of the render targets
   * have integer (UINT or SINT) surface format."
   *
   * Additionally internal documentation for Gfx12+ states:
   *
   * "This bit MUST not be set when NUM_MULTISAMPLES > 1 OR
   *  FORCED_SAMPLE_COUNT > 1."
   */
   const bool aa_enable =
      anv_rasterization_aa_mode(dynamic_raster_mode, line_mode) &&
               const bool depth_clip_enable =
            const bool xy_clip_test_enable =
                     SET(RASTER, raster.APIMode, api_mode);
   SET(RASTER, raster.DXMultisampleRasterizationEnable, msaa_raster_enable);
   SET(RASTER, raster.AntialiasingEnable, aa_enable);
   SET(RASTER, raster.CullMode, genX(vk_to_intel_cullmode)[dyn->rs.cull_mode]);
   SET(RASTER, raster.FrontWinding, genX(vk_to_intel_front_face)[dyn->rs.front_face]);
   SET(RASTER, raster.GlobalDepthOffsetEnableSolid, dyn->rs.depth_bias.enable);
   SET(RASTER, raster.GlobalDepthOffsetEnableWireframe, dyn->rs.depth_bias.enable);
   SET(RASTER, raster.GlobalDepthOffsetEnablePoint, dyn->rs.depth_bias.enable);
   SET(RASTER, raster.GlobalDepthOffsetConstant, dyn->rs.depth_bias.constant);
   SET(RASTER, raster.GlobalDepthOffsetScale, dyn->rs.depth_bias.slope);
   SET(RASTER, raster.GlobalDepthOffsetClamp, dyn->rs.depth_bias.clamp);
   SET(RASTER, raster.FrontFaceFillMode, genX(vk_to_intel_fillmode)[dyn->rs.polygon_mode]);
   SET(RASTER, raster.BackFaceFillMode, genX(vk_to_intel_fillmode)[dyn->rs.polygon_mode]);
   SET(RASTER, raster.ViewportZFarClipTestEnable, depth_clip_enable);
   SET(RASTER, raster.ViewportZNearClipTestEnable, depth_clip_enable);
   SET(RASTER, raster.ConservativeRasterizationEnable,
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_MASK)) {
      /* From the Vulkan 1.0 spec:
   *    If pSampleMask is NULL, it is treated as if the mask has all bits
   *    enabled, i.e. no coverage is removed from fragments.
   *
   * 3DSTATE_SAMPLE_MASK.SampleMask is 16 bits.
   */
                  #if GFX_VER == 9
         /* For the PMA fix */
   #endif
         BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_TEST_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_WRITE_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_COMPARE_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_TEST_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_COMPARE_MASK) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_WRITE_MASK) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_REFERENCE)) {
   VkImageAspectFlags ds_aspects = 0;
   if (gfx->depth_att.vk_format != VK_FORMAT_UNDEFINED)
         if (gfx->stencil_att.vk_format != VK_FORMAT_UNDEFINED)
            struct vk_depth_stencil_state opt_ds = dyn->ds;
                     SET(WM_DEPTH_STENCIL, ds.StencilTestMask,
         SET(WM_DEPTH_STENCIL, ds.StencilWriteMask,
            SET(WM_DEPTH_STENCIL, ds.BackfaceStencilTestMask, opt_ds.stencil.back.compare_mask & 0xff);
            SET(WM_DEPTH_STENCIL, ds.StencilReferenceValue,
         SET(WM_DEPTH_STENCIL, ds.BackfaceStencilReferenceValue,
            SET(WM_DEPTH_STENCIL, ds.DepthTestEnable, opt_ds.depth.test_enable);
   SET(WM_DEPTH_STENCIL, ds.DepthBufferWriteEnable, opt_ds.depth.write_enable);
   SET(WM_DEPTH_STENCIL, ds.DepthTestFunction,
         SET(WM_DEPTH_STENCIL, ds.StencilTestEnable, opt_ds.stencil.test_enable);
   SET(WM_DEPTH_STENCIL, ds.StencilBufferWriteEnable, opt_ds.stencil.write_enable);
   SET(WM_DEPTH_STENCIL, ds.StencilFailOp,
         SET(WM_DEPTH_STENCIL, ds.StencilPassDepthPassOp,
         SET(WM_DEPTH_STENCIL, ds.StencilPassDepthFailOp,
         SET(WM_DEPTH_STENCIL, ds.StencilTestFunction,
         SET(WM_DEPTH_STENCIL, ds.BackfaceStencilFailOp,
         SET(WM_DEPTH_STENCIL, ds.BackfaceStencilPassDepthPassOp,
         SET(WM_DEPTH_STENCIL, ds.BackfaceStencilPassDepthFailOp,
         SET(WM_DEPTH_STENCIL, ds.BackfaceStencilTestFunction,
      #if GFX_VER == 9
         const bool pma = want_stencil_pma_fix(cmd_buffer, &opt_ds);
   #endif
      #if GFX_VERx10 >= 125
         if (intel_needs_workaround(cmd_buffer->device->info, 18019816803)) {
      bool ds_write_state = opt_ds.depth.write_enable || opt_ds.stencil.write_enable;
   if (gfx->ds_write_state != ds_write_state) {
      gfx->ds_write_state = ds_write_state;
         #endif
            #if GFX_VER >= 12
      if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_ENABLE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_BOUNDS_TEST_BOUNDS)) {
   SET(DEPTH_BOUNDS, db.DepthBoundsTestEnable, dyn->ds.depth.bounds_test.enable);
   /* Only look at updating the bounds if testing is enabled */
   if (dyn->ds.depth.bounds_test.enable) {
      SET(DEPTH_BOUNDS, db.DepthBoundsTestMinValue, dyn->ds.depth.bounds_test.min);
            #endif
         if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE_ENABLE)) {
   SET(LINE_STIPPLE, ls.LineStipplePattern, dyn->rs.line.stipple.pattern);
   SET(LINE_STIPPLE, ls.LineStippleInverseRepeatCount,
                              if ((gfx->dirty & ANV_CMD_DIRTY_RESTART_INDEX) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE)) {
   SET(VF, vf.IndexedDrawCutIndexEnable, dyn->ia.primitive_restart_enable);
               if (gfx->dirty & ANV_CMD_DIRTY_INDEX_BUFFER)
         #if GFX_VERx10 >= 125
      if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE))
      #endif
         if (cmd_buffer->device->vk.enabled_extensions.EXT_sample_locations &&
      (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS_ENABLE)))
         if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      (gfx->dirty & ANV_CMD_DIRTY_RENDER_TARGETS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES)) {
   /* 3DSTATE_WM in the hope we can avoid spawning fragment shaders
   * threads.
   */
   bool force_thread_dispatch =
      anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT) &&
   (pipeline->force_fragment_thread_dispatch ||
                  if ((gfx->dirty & ANV_CMD_DIRTY_PIPELINE) ||
      (gfx->dirty & ANV_CMD_DIRTY_RENDER_TARGETS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_LOGIC_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_LOGIC_OP_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_ALPHA_TO_ONE_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_WRITE_MASKS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_BLEND_ENABLES) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_BLEND_EQUATIONS)) {
   const uint8_t color_writes = dyn->cb.color_write_enables;
   const struct brw_wm_prog_data *wm_prog_data = get_wm_prog_data(pipeline);
   bool has_writeable_rt =
                  SET(BLEND_STATE, blend.AlphaToCoverageEnable,
         SET(BLEND_STATE, blend.AlphaToOneEnable,
            bool independent_alpha_blend = false;
   /* Wa_14018912822, check if we set these during RT setup. */
   bool color_blend_zero = false;
   bool alpha_blend_zero = false;
   for (uint32_t i = 0; i < MAX_RTS; i++) {
      /* Disable anything above the current number of color attachments. */
                  SET(BLEND_STATE, blend.rts[i].WriteDisableAlpha,
                     SET(BLEND_STATE, blend.rts[i].WriteDisableRed,
                     SET(BLEND_STATE, blend.rts[i].WriteDisableGreen,
                     SET(BLEND_STATE, blend.rts[i].WriteDisableBlue,
                     /* Vulkan specification 1.2.168, VkLogicOp:
   *
   *   "Logical operations are controlled by the logicOpEnable and
   *   logicOp members of VkPipelineColorBlendStateCreateInfo. If
   *   logicOpEnable is VK_TRUE, then a logical operation selected by
   *   logicOp is applied between each color attachment and the
   *   fragment’s corresponding output value, and blending of all
   *   attachments is treated as if it were disabled."
   *
   * From the Broadwell PRM Volume 2d: Command Reference: Structures:
   * BLEND_STATE_ENTRY:
   *
   *   "Enabling LogicOp and Color Buffer Blending at the same time is
   *   UNDEFINED"
   */
   SET(BLEND_STATE, blend.rts[i].LogicOpFunction,
                  SET(BLEND_STATE, blend.rts[i].ColorClampRange, COLORCLAMP_RTFORMAT);
                  /* Setup blend equation. */
   SET(BLEND_STATE, blend.rts[i].ColorBlendFunction,
               SET(BLEND_STATE, blend.rts[i].AlphaBlendFunction,
                  if (dyn->cb.attachments[i].src_color_blend_factor !=
      dyn->cb.attachments[i].src_alpha_blend_factor ||
   dyn->cb.attachments[i].dst_color_blend_factor !=
   dyn->cb.attachments[i].dst_alpha_blend_factor ||
   dyn->cb.attachments[i].color_blend_op !=
   dyn->cb.attachments[i].alpha_blend_op) {
               /* The Dual Source Blending documentation says:
   *
   * "If SRC1 is included in a src/dst blend factor and
   * a DualSource RT Write message is not used, results
   * are UNDEFINED. (This reflects the same restriction in DX APIs,
   * where undefined results are produced if “o1” is not written
   * by a PS – there are no default values defined)."
   *
   * There is no way to gracefully fix this undefined situation
   * so we just disable the blending to prevent possible issues.
   */
   if (wm_prog_data && !wm_prog_data->dual_src_blend &&
      anv_is_dual_src_blend_equation(&dyn->cb.attachments[i])) {
      } else {
      SET(BLEND_STATE, blend.rts[i].ColorBufferBlendEnable,
                     /* Our hardware applies the blend factor prior to the blend function
   * regardless of what function is used.  Technically, this means the
   * hardware can do MORE than GL or Vulkan specify.  However, it also
   * means that, for MIN and MAX, we have to stomp the blend factor to
   * ONE to make it a no-op.
   */
   uint32_t SourceBlendFactor;
   uint32_t DestinationBlendFactor;
   uint32_t SourceAlphaBlendFactor;
   uint32_t DestinationAlphaBlendFactor;
   if (dyn->cb.attachments[i].color_blend_op == VK_BLEND_OP_MIN ||
      dyn->cb.attachments[i].color_blend_op == VK_BLEND_OP_MAX) {
   SourceBlendFactor = BLENDFACTOR_ONE;
      } else {
      SourceBlendFactor = genX(vk_to_intel_blend)[
         DestinationBlendFactor = genX(vk_to_intel_blend)[
               if (dyn->cb.attachments[i].alpha_blend_op == VK_BLEND_OP_MIN ||
      dyn->cb.attachments[i].alpha_blend_op == VK_BLEND_OP_MAX) {
   SourceAlphaBlendFactor = BLENDFACTOR_ONE;
      } else {
      SourceAlphaBlendFactor = genX(vk_to_intel_blend)[
         DestinationAlphaBlendFactor = genX(vk_to_intel_blend)[
               if (instance->intel_enable_wa_14018912822 &&
      intel_needs_workaround(cmd_buffer->device->info, 14018912822) &&
   pipeline->rasterization_samples > 1) {
   if (DestinationBlendFactor == BLENDFACTOR_ZERO) {
      DestinationBlendFactor = BLENDFACTOR_CONST_COLOR;
      }
   if (DestinationAlphaBlendFactor == BLENDFACTOR_ZERO) {
      DestinationAlphaBlendFactor = BLENDFACTOR_CONST_ALPHA;
                  SET(BLEND_STATE, blend.rts[i].SourceBlendFactor, SourceBlendFactor);
   SET(BLEND_STATE, blend.rts[i].DestinationBlendFactor, DestinationBlendFactor);
   SET(BLEND_STATE, blend.rts[i].SourceAlphaBlendFactor, SourceAlphaBlendFactor);
      }
   gfx->color_blend_zero = color_blend_zero;
                     /* 3DSTATE_PS_BLEND to be consistent with the rest of the
   * BLEND_STATE_ENTRY.
   */
   SET(PS_BLEND, ps_blend.HasWriteableRT, has_writeable_rt);
   SET(PS_BLEND, ps_blend.ColorBufferBlendEnable, GET(blend.rts[0].ColorBufferBlendEnable));
   SET(PS_BLEND, ps_blend.SourceAlphaBlendFactor, GET(blend.rts[0].SourceAlphaBlendFactor));
   SET(PS_BLEND, ps_blend.DestinationAlphaBlendFactor, gfx->alpha_blend_zero ?
               SET(PS_BLEND, ps_blend.SourceBlendFactor, GET(blend.rts[0].SourceBlendFactor));
   SET(PS_BLEND, ps_blend.DestinationBlendFactor, gfx->color_blend_zero ?
               SET(PS_BLEND, ps_blend.AlphaTestEnable, false);
   SET(PS_BLEND, ps_blend.IndependentAlphaBlendEnable, GET(blend.IndependentAlphaBlendEnable));
               if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_BLEND_CONSTANTS)) {
      SET(CC_STATE, cc.BlendConstantColorRed,
         SET(CC_STATE, cc.BlendConstantColorGreen,
         SET(CC_STATE, cc.BlendConstantColorBlue,
         SET(CC_STATE, cc.BlendConstantColorAlpha,
               if ((gfx->dirty & ANV_CMD_DIRTY_RENDER_AREA) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_VIEWPORTS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_SCISSORS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_CLAMP_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_DEPTH_CLIP_NEGATIVE_ONE_TO_ONE)) {
   struct anv_instance *instance = cmd_buffer->device->physical->instance;
                     for (uint32_t i = 0; i < dyn->vp.viewport_count; i++) {
               /* The gfx7 state struct has just the matrix and guardband fields, the
   * gfx8 struct adds the min/max viewport fields. */
   struct GENX(SF_CLIP_VIEWPORT) sfv = {
      .ViewportMatrixElementm00 = vp->width / 2,
   .ViewportMatrixElementm11 = vp->height / 2,
   .ViewportMatrixElementm22 = (vp->maxDepth - vp->minDepth) * scale,
   .ViewportMatrixElementm30 = vp->x + vp->width / 2,
   .ViewportMatrixElementm31 = vp->y + vp->height / 2,
   .ViewportMatrixElementm32 = dyn->vp.depth_clip_negative_one_to_one ?
         .XMinClipGuardband = -1.0f,
   .XMaxClipGuardband = 1.0f,
   .YMinClipGuardband = -1.0f,
   .YMaxClipGuardband = 1.0f,
   .XMinViewPort = vp->x,
   .XMaxViewPort = vp->x + vp->width - 1,
   .YMinViewPort = MIN2(vp->y, vp->y + vp->height),
               /* Fix depth test misrenderings by lowering translated depth range */
                  const uint32_t fb_size_max = 1 << 14;
                  /* If we have a valid renderArea, include that */
   if (gfx->render_area.extent.width > 0 &&
      gfx->render_area.extent.height > 0) {
   x_min = MAX2(x_min, gfx->render_area.offset.x);
   x_max = MIN2(x_max, gfx->render_area.offset.x +
         y_min = MAX2(y_min, gfx->render_area.offset.y);
   y_max = MIN2(y_max, gfx->render_area.offset.y +
               /* The client is required to have enough scissors for whatever it
   * sets as ViewportIndex but it's possible that they've got more
   * viewports set from a previous command. Also, from the Vulkan
   * 1.3.207:
   *
   *    "The application must ensure (using scissor if necessary) that
   *    all rendering is contained within the render area."
   *
   * If the client doesn't set a scissor, that basically means it
   * guarantees everything is in-bounds already. If we end up using a
   * guardband of [-1, 1] in that case, there shouldn't be much loss.
   * It's theoretically possible that they could do all their clipping
   * with clip planes but that'd be a bit odd.
   */
   if (i < dyn->vp.scissor_count) {
      const VkRect2D *scissor = &dyn->vp.scissors[i];
   x_min = MAX2(x_min, scissor->offset.x);
   x_max = MIN2(x_max, scissor->offset.x + scissor->extent.width);
   y_min = MAX2(y_min, scissor->offset.y);
               /* Only bother calculating the guardband if our known render area is
   * less than the maximum size. Otherwise, it will calculate [-1, 1]
   * anyway but possibly with precision loss.
   */
   if (x_min > 0 || x_max < fb_size_max ||
      y_min > 0 || y_max < fb_size_max) {
   intel_calculate_guardband_size(x_min, x_max, y_min, y_max,
                                 sfv.ViewportMatrixElementm00,
   sfv.ViewportMatrixElementm11,
      #define SET_VP(bit, state, field)                                        \
            do {                                                           \
      if (hw_state->state.field != sfv.field) {                   \
      hw_state->state.field = sfv.field;                       \
   BITSET_SET(hw_state->dirty,                              \
         } while (0)
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm00);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm11);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm22);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm30);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm31);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], ViewportMatrixElementm32);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], XMinClipGuardband);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], XMaxClipGuardband);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], YMinClipGuardband);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], YMaxClipGuardband);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], XMinViewPort);
   SET_VP(VIEWPORT_SF_CLIP, vp_sf_clip.elem[i], XMaxViewPort);
      #undef SET_VP
               float min_depth = dyn->rs.depth_clamp_enable ?
               float max_depth = dyn->rs.depth_clamp_enable ?
                                 SET(CLIP, clip.MaximumVPIndex, dyn->vp.viewport_count > 0 ?
               /* If the HW state is already considered dirty or the previous
   * programmed viewport count is smaller than what we need, update the
   * viewport count and ensure the HW state is dirty. Otherwise if the
   * number of viewport programmed previously was larger than what we need
   * now, no need to reemit we can just keep the old programmed values.
   */
   if (BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VIEWPORT_SF_CLIP) ||
      hw_state->vp_sf_clip.count < dyn->vp.viewport_count) {
   hw_state->vp_sf_clip.count = dyn->vp.viewport_count;
      }
   if (BITSET_SET(hw_state->dirty, ANV_GFX_STATE_VIEWPORT_CC) ||
      hw_state->vp_cc.count < dyn->vp.viewport_count) {
   hw_state->vp_cc.count = dyn->vp.viewport_count;
                  if ((gfx->dirty & ANV_CMD_DIRTY_RENDER_AREA) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_SCISSORS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_VP_VIEWPORTS)) {
   const VkRect2D *scissors = dyn->vp.scissors;
            for (uint32_t i = 0; i < dyn->vp.scissor_count; i++) {
                              uint32_t y_min = MAX2(s->offset.y, MIN2(vp->y, vp->y + vp->height));
   uint32_t x_min = MAX2(s->offset.x, vp->x);
   int64_t y_max = MIN2(s->offset.y + s->extent.height - 1,
                                       /* Do this math using int64_t so overflow gets clamped correctly. */
   if (cmd_buffer->vk.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
      y_min = CLAMP((uint64_t) y_min, gfx->render_area.offset.y, max);
   x_min = CLAMP((uint64_t) x_min, gfx->render_area.offset.x, max);
   y_max = CLAMP((uint64_t) y_max, 0,
               x_max = CLAMP((uint64_t) x_max, 0,
                     if (s->extent.width <= 0 || s->extent.height <= 0) {
      /* Since xmax and ymax are inclusive, we have to have xmax < xmin
   * or ymax < ymin for empty clips. In case clip x, y, width height
   * are all 0, the clamps below produce 0 for xmin, ymin, xmax,
   * ymax, which isn't what we want. Just special case empty clips
   * and produce a canonical empty clip.
   */
   SET(SCISSOR, scissor.elem[i].ScissorRectangleYMin, 1);
   SET(SCISSOR, scissor.elem[i].ScissorRectangleXMin, 1);
   SET(SCISSOR, scissor.elem[i].ScissorRectangleYMax, 0);
      } else {
      SET(SCISSOR, scissor.elem[i].ScissorRectangleYMin, y_min);
   SET(SCISSOR, scissor.elem[i].ScissorRectangleXMin, x_min);
   SET(SCISSOR, scissor.elem[i].ScissorRectangleYMax, y_max);
                  /* If the HW state is already considered dirty or the previous
   * programmed viewport count is smaller than what we need, update the
   * viewport count and ensure the HW state is dirty. Otherwise if the
   * number of viewport programmed previously was larger than what we need
   * now, no need to reemit we can just keep the old programmed values.
   */
   if (BITSET_SET(hw_state->dirty, ANV_GFX_STATE_SCISSOR) ||
      hw_state->scissor.count < dyn->vp.scissor_count) {
   hw_state->scissor.count = dyn->vp.scissor_count;
               #undef GET
   #undef SET
   #undef SET_STAGE
            }
      /**
   * This function emits the dirty instructions in the batch buffer.
   */
   void
   genX(cmd_buffer_flush_gfx_hw_state)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_device *device = cmd_buffer->device;
   struct anv_cmd_graphics_state *gfx = &cmd_buffer->state.gfx;
   struct anv_graphics_pipeline *pipeline = gfx->pipeline;
   const struct vk_dynamic_graphics_state *dyn =
                  if (INTEL_DEBUG(DEBUG_REEMIT)) {
      BITSET_OR(gfx->dyn_state.dirty, gfx->dyn_state.dirty,
               /**
   * Put potential workarounds here if you need to reemit an instruction
   * because of another one is changing.
            /* Since Wa_16011773973 will disable 3DSTATE_STREAMOUT, we need to reemit
   * it after.
   */
   if (intel_needs_workaround(device->info, 16011773973) &&
      pipeline->uses_xfb &&
   BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SO_DECL_LIST)) {
               /* Gfx11 undocumented issue :
   * https://gitlab.freedesktop.org/mesa/mesa/-/issues/9781
      #if GFX_VER == 11
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_WM))
      #endif
         /**
   * State emission
   */
   if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_URB))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MULTISAMPLE))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_PRIMITIVE_REPLICATION))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF_SGVS_INSTANCING))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF_SGVS))
         #if GFX_VER >= 11
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF_SGVS_2))
      #endif
         if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VS))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_HS))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_DS))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF_STATISTICS))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SBE))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SBE_SWIZ))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SO_DECL_LIST)) {
      /* Wa_16011773973:
   * If SOL is enabled and SO_DECL state has to be programmed,
   *    1. Send 3D State SOL state with SOL disabled
   *    2. Send SO_DECL NP state
   *    3. Send 3D State SOL with SOL Enabled
   */
   if (intel_needs_workaround(device->info, 16011773973) &&
                  anv_batch_emit_pipeline_state(&cmd_buffer->batch, pipeline,
      #if GFX_VER >= 11
         /* ICL PRMs, Volume 2a - Command Reference: Instructions,
   * 3DSTATE_SO_DECL_LIST:
   *
   *    "Workaround: This command must be followed by a PIPE_CONTROL with
   *     CS Stall bit set."
   *
   * On DG2+ also known as Wa_1509820217.
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch, device->info,
   #endif
               if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_PS))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_PS_EXTRA))
            if (device->vk.enabled_extensions.EXT_mesh_shader) {
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_CONTROL))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_SHADER))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_DISTRIB))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_CONTROL))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_SHADER))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_REDISTRIB))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SBE_MESH))
            if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_CLIP_MESH))
      } else {
      assert(!BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_CONTROL) &&
         !BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_SHADER) &&
   !BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_MESH_DISTRIB) &&
   !BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_CONTROL) &&
   !BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_SHADER) &&
   !BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TASK_REDISTRIB) &&
            #define INIT(category, name) \
         #define SET(s, category, name) \
                        if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_CLIP)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_CLIP),
            SET(clip, clip, APIMode);
   SET(clip, clip, ViewportXYClipTestEnable);
   SET(clip, clip, TriangleStripListProvokingVertexSelect);
   SET(clip, clip, LineStripListProvokingVertexSelect);
   SET(clip, clip, TriangleFanProvokingVertexSelect);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_STREAMOUT)) {
               anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_STREAMOUT),
            SET(so, so, RenderingDisable);
   SET(so, so, RenderStreamSelect);
   SET(so, so, ReorderMode);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VIEWPORT_SF_CLIP)) {
      struct anv_state sf_clip_state =
                  for (uint32_t i = 0; i < hw_state->vp_sf_clip.count; i++) {
      struct GENX(SF_CLIP_VIEWPORT) sfv = {
      INIT(vp_sf_clip.elem[i], ViewportMatrixElementm00),
   INIT(vp_sf_clip.elem[i], ViewportMatrixElementm11),
   INIT(vp_sf_clip.elem[i], ViewportMatrixElementm22),
   INIT(vp_sf_clip.elem[i], ViewportMatrixElementm30),
   INIT(vp_sf_clip.elem[i], ViewportMatrixElementm31),
   INIT(vp_sf_clip.elem[i], ViewportMatrixElementm32),
   INIT(vp_sf_clip.elem[i], XMinClipGuardband),
   INIT(vp_sf_clip.elem[i], XMaxClipGuardband),
   INIT(vp_sf_clip.elem[i], YMinClipGuardband),
   INIT(vp_sf_clip.elem[i], YMaxClipGuardband),
   INIT(vp_sf_clip.elem[i], XMinViewPort),
   INIT(vp_sf_clip.elem[i], XMaxViewPort),
   INIT(vp_sf_clip.elem[i], YMinViewPort),
      };
               anv_batch_emit(&cmd_buffer->batch,
                           if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VIEWPORT_CC)) {
      struct anv_state cc_state =
                  for (uint32_t i = 0; i < hw_state->vp_cc.count; i++) {
      struct GENX(CC_VIEWPORT) cc_viewport = {
      INIT(vp_cc.elem[i], MinimumDepth),
      };
               anv_batch_emit(&cmd_buffer->batch,
                           if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SCISSOR)) {
      /* Wa_1409725701:
   *
   *    "The viewport-specific state used by the SF unit (SCISSOR_RECT) is
   *    stored as an array of up to 16 elements. The location of first
   *    element of the array, as specified by Pointer to SCISSOR_RECT,
   *    should be aligned to a 64-byte boundary.
   */
   struct anv_state scissor_state =
                  for (uint32_t i = 0; i < hw_state->scissor.count; i++) {
      struct GENX(SCISSOR_RECT) scissor = {
      INIT(scissor.elem[i], ScissorRectangleYMin),
   INIT(scissor.elem[i], ScissorRectangleXMin),
   INIT(scissor.elem[i], ScissorRectangleYMax),
      };
               anv_batch_emit(&cmd_buffer->batch,
                           if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF_TOPOLOGY)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_VF_TOPOLOGY), vft) {
                     if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VERTEX_INPUT)) {
      const uint32_t ve_count =
         const uint32_t num_dwords = 1 + 2 * MAX2(1, ve_count);
   uint32_t *p = anv_batch_emitn(&cmd_buffer->batch, num_dwords,
            if (p) {
      if (ve_count == 0) {
      memcpy(p + 1, cmd_buffer->device->empty_vs_input,
      } else if (ve_count == pipeline->vertex_input_elems) {
      /* MESA_VK_DYNAMIC_VI is not dynamic for this pipeline, so
   * everything is in pipeline->vertex_input_data and we can just
   * memcpy
   */
   memcpy(p + 1, pipeline->vertex_input_data, 4 * 2 * ve_count);
   anv_batch_emit_pipeline_state(&cmd_buffer->batch, pipeline,
      } else {
      assert(pipeline->final.vf_instancing.len == 0);
   /* Use dyn->vi to emit the dynamic VERTEX_ELEMENT_STATE input. */
   genX(emit_vertex_input)(&cmd_buffer->batch, p + 1,
         /* Then append the VERTEX_ELEMENT_STATE for the draw parameters */
   memcpy(p + 1 + 2 * pipeline->vs_input_elements,
                           if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_TE)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_TE),
                           if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_GS)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_GS),
                              #if GFX_VER == 11
         anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CPS), cps) {
      SET(cps, cps, CoarsePixelShadingMode);
   SET(cps, cps, MinCPSizeX);
      #elif GFX_VER >= 12
         /* TODO: we can optimize this flush in the following cases:
   *
   *    In the case where the last geometry shader emits a value that is
   *    not constant, we can avoid this stall because we can synchronize
   *    the pixel shader internally with
   *    3DSTATE_PS::EnablePSDependencyOnCPsizeChange.
   *
   *    If we know that the previous pipeline and the current one are
   *    using the same fragment shading rate.
   */
   #if GFX_VERx10 >= 125
         #else
         #endif
                  anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CPS_POINTERS), cps) {
         #endif
               if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SF)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_SF),
            SET(sf, sf, LineWidth);
   SET(sf, sf, TriangleStripListProvokingVertexSelect);
   SET(sf, sf, LineStripListProvokingVertexSelect);
   SET(sf, sf, TriangleFanProvokingVertexSelect);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_RASTER)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_RASTER),
            SET(raster, raster, APIMode);
   SET(raster, raster, DXMultisampleRasterizationEnable);
   SET(raster, raster, AntialiasingEnable);
   SET(raster, raster, CullMode);
   SET(raster, raster, FrontWinding);
   SET(raster, raster, GlobalDepthOffsetEnableSolid);
   SET(raster, raster, GlobalDepthOffsetEnableWireframe);
   SET(raster, raster, GlobalDepthOffsetEnablePoint);
   SET(raster, raster, GlobalDepthOffsetConstant);
   SET(raster, raster, GlobalDepthOffsetScale);
   SET(raster, raster, GlobalDepthOffsetClamp);
   SET(raster, raster, FrontFaceFillMode);
   SET(raster, raster, BackFaceFillMode);
   SET(raster, raster, ViewportZFarClipTestEnable);
   SET(raster, raster, ViewportZNearClipTestEnable);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_CC_STATE)) {
      struct anv_state cc_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
            struct GENX(COLOR_CALC_STATE) cc = {
      INIT(cc, BlendConstantColorRed),
   INIT(cc, BlendConstantColorGreen),
   INIT(cc, BlendConstantColorBlue),
      };
            anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CC_STATE_POINTERS), ccp) {
      ccp.ColorCalcStatePointer = cc_state.offset;
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SAMPLE_MASK)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_SAMPLE_MASK), sm) {
                     if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_WM_DEPTH_STENCIL)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_WM_DEPTH_STENCIL), ds) {
      SET(ds, ds, DoubleSidedStencilEnable);
   SET(ds, ds, StencilTestMask);
   SET(ds, ds, StencilWriteMask);
   SET(ds, ds, BackfaceStencilTestMask);
   SET(ds, ds, BackfaceStencilWriteMask);
   SET(ds, ds, StencilReferenceValue);
   SET(ds, ds, BackfaceStencilReferenceValue);
   SET(ds, ds, DepthTestEnable);
   SET(ds, ds, DepthBufferWriteEnable);
   SET(ds, ds, DepthTestFunction);
   SET(ds, ds, StencilTestEnable);
   SET(ds, ds, StencilBufferWriteEnable);
   SET(ds, ds, StencilFailOp);
   SET(ds, ds, StencilPassDepthPassOp);
   SET(ds, ds, StencilPassDepthFailOp);
   SET(ds, ds, StencilTestFunction);
   SET(ds, ds, BackfaceStencilFailOp);
   SET(ds, ds, BackfaceStencilPassDepthPassOp);
   SET(ds, ds, BackfaceStencilPassDepthFailOp);
               #if GFX_VER >= 12
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_DEPTH_BOUNDS)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_DEPTH_BOUNDS), db) {
      SET(db, db, DepthBoundsTestEnable);
   SET(db, db, DepthBoundsTestMinValue);
            #endif
         if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_LINE_STIPPLE)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_LINE_STIPPLE), ls) {
      SET(ls, ls, LineStipplePattern);
   SET(ls, ls, LineStippleInverseRepeatCount);
      #if GFX_VER >= 11
         /* ICL PRMs, Volume 2a - Command Reference: Instructions,
   * 3DSTATE_LINE_STIPPLE:
   *
   *    "Workaround: This command must be followed by a PIPE_CONTROL with
   *     CS Stall bit set."
   */
   genx_batch_emit_pipe_control(&cmd_buffer->batch,
         #endif
               if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VF)) {
      #if GFX_VERx10 >= 125
         #endif
            SET(vf, vf, IndexedDrawCutIndexEnable);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_INDEX_BUFFER)) {
      struct anv_buffer *buffer = gfx->index_buffer;
   uint32_t offset = gfx->index_offset;
   anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_INDEX_BUFFER), ib) {
      ib.IndexFormat           = gfx->index_type;
   ib.MOCS                  = anv_mocs(cmd_buffer->device,
      #if GFX_VER >= 12
         #endif
            ib.BufferStartingAddress = anv_address_add(buffer->address, offset);
               #if GFX_VERx10 >= 125
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_VFG)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_VFG),
                     #endif
         if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_SAMPLE_PATTERN)) {
      genX(emit_sample_pattern)(&cmd_buffer->batch,
                     if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_WM)) {
      anv_batch_emit_merge(&cmd_buffer->batch, GENX(3DSTATE_WM),
            SET(wm, wm, ForceThreadDispatchEnable);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_PS_BLEND)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_PS_BLEND), blend) {
      SET(blend, ps_blend, HasWriteableRT);
   SET(blend, ps_blend, ColorBufferBlendEnable);
   SET(blend, ps_blend, SourceAlphaBlendFactor);
   SET(blend, ps_blend, DestinationAlphaBlendFactor);
   SET(blend, ps_blend, SourceBlendFactor);
   SET(blend, ps_blend, DestinationBlendFactor);
   SET(blend, ps_blend, AlphaTestEnable);
   SET(blend, ps_blend, IndependentAlphaBlendEnable);
                  if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_BLEND_STATE)) {
      const uint32_t num_dwords = GENX(BLEND_STATE_length) +
         struct anv_state blend_states =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
                        struct GENX(BLEND_STATE) blend_state = {
      INIT(blend, AlphaToCoverageEnable),
   INIT(blend, AlphaToOneEnable),
      };
            /* Jump to blend entries. */
   dws += GENX(BLEND_STATE_length);
   for (uint32_t i = 0; i < MAX_RTS; i++) {
      struct GENX(BLEND_STATE_ENTRY) entry = {
      INIT(blend.rts[i], WriteDisableAlpha),
   INIT(blend.rts[i], WriteDisableRed),
   INIT(blend.rts[i], WriteDisableGreen),
   INIT(blend.rts[i], WriteDisableBlue),
   INIT(blend.rts[i], LogicOpFunction),
   INIT(blend.rts[i], LogicOpEnable),
   INIT(blend.rts[i], ColorBufferBlendEnable),
   INIT(blend.rts[i], ColorClampRange),
   INIT(blend.rts[i], PreBlendColorClampEnable),
   INIT(blend.rts[i], PostBlendColorClampEnable),
   INIT(blend.rts[i], SourceBlendFactor),
   INIT(blend.rts[i], DestinationBlendFactor),
   INIT(blend.rts[i], ColorBlendFunction),
   INIT(blend.rts[i], SourceAlphaBlendFactor),
   INIT(blend.rts[i], DestinationAlphaBlendFactor),
               GENX(BLEND_STATE_ENTRY_pack)(NULL, dws, &entry);
               gfx->blend_states = blend_states;
   /* Dirty the pointers to reemit 3DSTATE_BLEND_STATE_POINTERS below */
               if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_BLEND_STATE_POINTERS)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_BLEND_STATE_POINTERS), bsp) {
      bsp.BlendStatePointer      = gfx->blend_states.offset;
               #if GFX_VERx10 >= 125
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_WA_18019816803)) {
      genx_batch_emit_pipe_control(&cmd_buffer->batch, cmd_buffer->device->info,
         #endif
      #if GFX_VER == 9
      if (BITSET_TEST(hw_state->dirty, ANV_GFX_STATE_PMA_FIX))
      #endif
      #undef INIT
   #undef SET
            }
      void
   genX(cmd_buffer_enable_pma_fix)(struct anv_cmd_buffer *cmd_buffer, bool enable)
   {
      if (cmd_buffer->state.pma_fix_enabled == enable)
                     /* According to the Broadwell PIPE_CONTROL documentation, software should
   * emit a PIPE_CONTROL with the CS Stall and Depth Cache Flush bits set
   * prior to the LRI.  If stencil buffer writes are enabled, then a Render
   * Cache Flush is also necessary.
   *
   * The Skylake docs say to use a depth stall rather than a command
   * streamer stall.  However, the hardware seems to violently disagree.
   * A full command streamer stall seems to be needed in both cases.
   */
   genx_batch_emit_pipe_control
      (&cmd_buffer->batch, cmd_buffer->device->info,
   ANV_PIPE_DEPTH_CACHE_FLUSH_BIT |
   #if GFX_VER >= 12
         #endif
            #if GFX_VER == 9
      uint32_t cache_mode;
   anv_pack_struct(&cache_mode, GENX(CACHE_MODE_0),
               anv_batch_emit(&cmd_buffer->batch, GENX(MI_LOAD_REGISTER_IMM), lri) {
      lri.RegisterOffset   = GENX(CACHE_MODE_0_num);
            #endif /* GFX_VER == 9 */
         /* After the LRI, a PIPE_CONTROL with both the Depth Stall and Depth Cache
   * Flush bits is often necessary.  We do it regardless because it's easier.
   * The render cache flush is also necessary if stencil writes are enabled.
   *
   * Again, the Skylake docs give a different set of flushes but the BDW
   * flushes seem to work just as well.
   */
   genx_batch_emit_pipe_control
      (&cmd_buffer->batch, cmd_buffer->device->info,
   ANV_PIPE_DEPTH_STALL_BIT |
   #if GFX_VER >= 12
         #endif
         }
