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
   #include "vk_format.h"
      #include "genxml/gen_macros.h"
   #include "genxml/genX_pack.h"
      static uint32_t
   get_depth_format(struct anv_cmd_buffer *cmd_buffer)
   {
               switch (gfx->depth_att.vk_format) {
   case VK_FORMAT_D16_UNORM:
   case VK_FORMAT_D16_UNORM_S8_UINT:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
   case VK_FORMAT_D24_UNORM_S8_UINT:
            case VK_FORMAT_D32_SFLOAT:
   case VK_FORMAT_D32_SFLOAT_S8_UINT:
            default:
            }
      void
   genX(cmd_buffer_flush_dynamic_state)(struct anv_cmd_buffer *cmd_buffer)
   {
      struct anv_graphics_pipeline *pipeline = cmd_buffer->state.gfx.pipeline;
   const struct vk_dynamic_graphics_state *dyn =
            if ((cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
            BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_TOPOLOGY) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_CULL_MODE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_FRONT_FACE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_DEPTH_BIAS_FACTORS) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_WIDTH)) {
   /* Take dynamic primitive topology in to account with
   *    3DSTATE_SF::MultisampleRasterizationMode
   */
   VkPolygonMode dynamic_raster_mode =
      genX(raster_polygon_mode)(cmd_buffer->state.gfx.pipeline,
      uint32_t ms_rast_mode =
         /* From the Haswell PRM, Volume 2b, documentation for
      * 3DSTATE_SF, "Antialiasing Enable":
   *
   * "This field must be disabled if any of the render targets
   * have integer (UINT or SINT) surface format."
   */
   bool aa_enable = anv_rasterization_aa_mode(dynamic_raster_mode,
                  uint32_t sf_dw[GENX(3DSTATE_SF_length)];
   struct GENX(3DSTATE_SF) sf = {
      GENX(3DSTATE_SF_header),
   .DepthBufferSurfaceFormat = get_depth_format(cmd_buffer),
   .LineWidth = dyn->rs.line.width,
   .AntialiasingEnable = aa_enable,
   .CullMode     = genX(vk_to_intel_cullmode)[dyn->rs.cull_mode],
   .FrontWinding = genX(vk_to_intel_front_face)[dyn->rs.front_face],
   .MultisampleRasterizationMode = ms_rast_mode,
   .GlobalDepthOffsetEnableSolid       = dyn->rs.depth_bias.enable,
   .GlobalDepthOffsetEnableWireframe   = dyn->rs.depth_bias.enable,
   .GlobalDepthOffsetEnablePoint       = dyn->rs.depth_bias.enable,
   .GlobalDepthOffsetConstant          = dyn->rs.depth_bias.constant,
   .GlobalDepthOffsetScale             = dyn->rs.depth_bias.slope,
      };
                        if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_REFERENCE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_BLEND_CONSTANTS)) {
   struct anv_state cc_state =
      anv_cmd_buffer_alloc_dynamic_state(cmd_buffer,
            struct GENX(COLOR_CALC_STATE) cc = {
      .BlendConstantColorRed = dyn->cb.blend_constants[0],
   .BlendConstantColorGreen = dyn->cb.blend_constants[1],
   .BlendConstantColorBlue = dyn->cb.blend_constants[2],
   .BlendConstantColorAlpha = dyn->cb.blend_constants[3],
   .StencilReferenceValue = dyn->ds.stencil.front.reference & 0xff,
      };
            anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_CC_STATE_POINTERS), ccp) {
                     if (BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_RS_LINE_STIPPLE)) {
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_LINE_STIPPLE), ls) {
      ls.LineStipplePattern = dyn->rs.line.stipple.pattern;
   ls.LineStippleInverseRepeatCount =
                        if ((cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
            BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_TEST_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_WRITE_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_DEPTH_COMPARE_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_TEST_ENABLE) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_COMPARE_MASK) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_DS_STENCIL_WRITE_MASK)) {
            VkImageAspectFlags ds_aspects = 0;
   if (cmd_buffer->state.gfx.depth_att.vk_format != VK_FORMAT_UNDEFINED)
         if (cmd_buffer->state.gfx.stencil_att.vk_format != VK_FORMAT_UNDEFINED)
            struct vk_depth_stencil_state opt_ds = dyn->ds;
            struct GENX(DEPTH_STENCIL_STATE) depth_stencil = {
                                             .DepthTestEnable = opt_ds.depth.test_enable,
   .DepthBufferWriteEnable = opt_ds.depth.write_enable,
   .DepthTestFunction = genX(vk_to_intel_compare_op)[opt_ds.depth.compare_op],
   .StencilTestEnable = opt_ds.stencil.test_enable,
   .StencilBufferWriteEnable = opt_ds.stencil.write_enable,
   .StencilFailOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.front.op.fail],
   .StencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.front.op.pass],
   .StencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.front.op.depth_fail],
   .StencilTestFunction = genX(vk_to_intel_compare_op)[opt_ds.stencil.front.op.compare],
   .BackfaceStencilFailOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.back.op.fail],
   .BackfaceStencilPassDepthPassOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.back.op.pass],
   .BackfaceStencilPassDepthFailOp = genX(vk_to_intel_stencil_op)[opt_ds.stencil.back.op.depth_fail],
      };
            struct anv_state ds_state =
                  anv_batch_emit(&cmd_buffer->batch,
                           if (cmd_buffer->state.gfx.index_buffer &&
      ((cmd_buffer->state.gfx.dirty & (ANV_CMD_DIRTY_PIPELINE |
         BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_RESTART_ENABLE))) {
   struct anv_buffer *buffer = cmd_buffer->state.gfx.index_buffer;
      #if GFX_VERx10 == 75
         anv_batch_emit(&cmd_buffer->batch, GFX75_3DSTATE_VF, vf) {
      vf.IndexedDrawCutIndexEnable  = dyn->ia.primitive_restart_enable;
      #endif
            #if GFX_VERx10 != 75
         #endif
            ib.IndexFormat           = cmd_buffer->state.gfx.index_type;
   ib.MOCS                  = anv_mocs(cmd_buffer->device,
                  ib.BufferStartingAddress = anv_address_add(buffer->address, offset);
   ib.BufferEndingAddress   = anv_address_add(buffer->address,
                  /* 3DSTATE_WM in the hope we can avoid spawning fragment shaders
   * threads or if we have dirty dynamic primitive topology state and
   * need to toggle 3DSTATE_WM::MultisampleRasterizationMode dynamically.
   */
   if ((cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_IA_PRIMITIVE_TOPOLOGY) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES)) {
   VkPolygonMode dynamic_raster_mode =
                  uint32_t dwords[GENX(3DSTATE_WM_length)];
   struct GENX(3DSTATE_WM) wm = {
               .ThreadDispatchEnable = anv_pipeline_has_stage(pipeline, MESA_SHADER_FRAGMENT) &&
               .MultisampleRasterizationMode =
            };
                        if ((cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_RENDER_TARGETS) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_MS_SAMPLE_LOCATIONS)) {
   const uint32_t samples = MAX2(1, cmd_buffer->state.gfx.samples);
   const struct vk_sample_locations_state *sl = dyn->ms.sample_locations;
   genX(emit_multisample)(&cmd_buffer->batch, samples,
               if ((cmd_buffer->state.gfx.dirty & ANV_CMD_DIRTY_PIPELINE) ||
      BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_LOGIC_OP) ||
   BITSET_TEST(dyn->dirty, MESA_VK_DYNAMIC_CB_COLOR_WRITE_ENABLES)) {
            /* Blend states of each RT */
   uint32_t blend_dws[GENX(BLEND_STATE_length) +
         uint32_t *dws = blend_dws;
            /* Skip this part */
            for (uint32_t i = 0; i < MAX_RTS; i++) {
      /* Disable anything above the current number of color attachments. */
   bool write_disabled = i >= cmd_buffer->state.gfx.color_att_count ||
         struct GENX(BLEND_STATE_ENTRY) entry = {
      .WriteDisableAlpha = write_disabled ||
               .WriteDisableRed   = write_disabled ||
               .WriteDisableGreen = write_disabled ||
               .WriteDisableBlue  = write_disabled ||
                  };
   GENX(BLEND_STATE_ENTRY_pack)(NULL, dws, &entry);
               uint32_t num_dwords = GENX(BLEND_STATE_length) +
            struct anv_state blend_states =
      anv_cmd_buffer_merge_dynamic(cmd_buffer, blend_dws,
      anv_batch_emit(&cmd_buffer->batch, GENX(3DSTATE_BLEND_STATE_POINTERS), bsp) {
                     /* When we're done, there is no more dirty gfx state. */
   vk_dynamic_graphics_state_clear_dirty(&cmd_buffer->vk.dynamic_graphics_state);
      }
      void
   genX(cmd_buffer_enable_pma_fix)(struct anv_cmd_buffer *cmd_buffer,
         {
         }
