   /**********************************************************
   * Copyright 2018-2022 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_simple_shaders.h"
      #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_tgsi.h"
   #include "svga_shader.h"
         static void
   make_tcs_key(struct svga_context *svga, struct svga_compile_key *key)
   {
                        /*
   * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
   */
            /* SVGA_NEW_TCS_PARAM */
            /* The tessellator parameters come from the layout section in the
   * tessellation evaluation shader. Get these parameters from the
   * current tessellation evaluation shader variant.
   * Note: this requires the tessellation evaluation shader to be
   * compiled first.
   */
   struct svga_tes_variant *tes = svga_tes_variant(svga->state.hw_draw.tes);
   key->tcs.prim_mode = tes->prim_mode;
   key->tcs.spacing = tes->spacing;
   key->tcs.vertices_order_cw = tes->vertices_order_cw;
            /* The number of control point output from tcs is determined by the
   * number of control point input expected in tes. If tes does not expect
   * any control point input, then vertices_per_patch in the tes key will
   * be 0, otherwise it will contain the number of vertices out as specified
   * in the tcs property.
   */
            if (svga->tcs.passthrough)
                     /* tcs is always followed by tes */
      }
         static enum pipe_error
   emit_hw_tcs(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_shader_variant *variant;
   struct svga_tcs_shader *tcs = svga->curr.tcs;
   enum pipe_error ret = PIPE_OK;
                              if (!tcs) {
      /* If there is no active tcs, then there should not be
   * active tes either
   */
   assert(!svga->curr.tes);
               /** The previous tessellation control shader is made inactive.
   *  Needs to unbind the tessellation control shader.
   */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_HS, NULL);
   if (ret != PIPE_OK)
            }
                        /* See if we already have a TCS variant that matches the key */
            if (!variant) {
      ret = svga_compile_shader(svga, &tcs->base, &key, &variant);
   if (ret != PIPE_OK)
               if (variant != svga->state.hw_draw.tcs) {
      /* Bind the new variant */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_HS, variant);
   if (ret != PIPE_OK)
            svga->rebind.flags.tcs = false;
   svga->dirty |= SVGA_NEW_TCS_VARIANT;
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         struct svga_tracked_state svga_hw_tcs =
   {
      "tessellation control shader (hwtnl)",
   (SVGA_NEW_VS |
   SVGA_NEW_TCS |
   SVGA_NEW_TES |
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_RAST |
   SVGA_NEW_TCS_RAW_BUFFER),
      };
         static void
   make_tes_key(struct svga_context *svga, struct svga_compile_key *key)
   {
                        /*
   * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
   */
                     key->tes.vertices_per_patch = tes->base.info.tes.reads_control_point ?
            key->tes.need_prescale = svga->state.hw_clear.prescale[0].enabled &&
            /* tcs emits tessellation factors as extra outputs.
   * Since tes depends on them, save the tessFactor output index
   * from tcs in the tes compile key, so that if a different
   * tcs is bound and if the tessFactor index is different,
   * a different tes variant will be generated.
   */
                     /* This is the last vertex stage if there is no geometry shader. */
            key->tes.need_tessinner = svga->curr.tcs->base.info.tcs.writes_tess_factor;
      }
         static void
   get_passthrough_tcs(struct svga_context *svga)
   {
      if (svga->tcs.passthrough_tcs &&
      svga->tcs.vs == svga->curr.vs &&
   svga->tcs.tes == svga->curr.tes &&
   svga->tcs.vertices_per_patch == svga->curr.vertices_per_patch) {
   svga->pipe.bind_tcs_state(&svga->pipe,
      }
   else {
               /* delete older passthrough shader*/
   if (svga->tcs.passthrough_tcs) {
      svga->pipe.delete_tcs_state(&svga->pipe,
               new_tcs = (struct svga_tcs_shader *)
      util_make_tess_ctrl_passthrough_shader(&svga->pipe,
      svga->curr.vs->base.tgsi_info.num_outputs,
   svga->curr.tes->base.tgsi_info.num_inputs,
   svga->curr.vs->base.tgsi_info.output_semantic_name,
   svga->curr.vs->base.tgsi_info.output_semantic_index,
   svga->curr.tes->base.tgsi_info.input_semantic_name,
   svga->curr.tes->base.tgsi_info.input_semantic_index,
   svga->pipe.bind_tcs_state(&svga->pipe, new_tcs);
   svga->tcs.passthrough_tcs = new_tcs;
   svga->tcs.vs = svga->curr.vs;
   svga->tcs.tes = svga->curr.tes;
                        cb.buffer = NULL;
   cb.user_buffer = (void *) svga->curr.default_tesslevels;
   cb.buffer_offset = 0;
   cb.buffer_size = 2 * 4 * sizeof(float);
      }
         static enum pipe_error
   emit_hw_tes(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_shader_variant *variant;
   struct svga_tes_shader *tes = svga->curr.tes;
   enum pipe_error ret = PIPE_OK;
                              if (!tes) {
      /* The GL spec implies that TES is optional when there's a TCS,
   * but that's apparently a spec error. Assert if we have a TCS
   * but no TES.
   */
   assert(!svga->curr.tcs);
               /** The previous tessellation evaluation shader is made inactive.
   *  Needs to unbind the tessellation evaluation shader.
   */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_DS, NULL);
   if (ret != PIPE_OK)
            }
               if (!svga->curr.tcs) {
      /* TES state is processed before the TCS
   * shader and that's why we're checking for and creating the
   * passthough TCS in the emit_hw_tes() function.
   */
   get_passthrough_tcs(svga);
      }
   else {
                           /* See if we already have a TES variant that matches the key */
            if (!variant) {
      ret = svga_compile_shader(svga, &tes->base, &key, &variant);
   if (ret != PIPE_OK)
               if (variant != svga->state.hw_draw.tes) {
      /* Bind the new variant */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_DS, variant);
   if (ret != PIPE_OK)
            svga->rebind.flags.tes = false;
   svga->dirty |= SVGA_NEW_TES_VARIANT;
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         struct svga_tracked_state svga_hw_tes =
   {
      "tessellation evaluation shader (hwtnl)",
   /* TBD SVGA_NEW_VS/SVGA_NEW_FS/SVGA_NEW_GS are required or not*/
   (SVGA_NEW_VS |
   SVGA_NEW_FS |
   SVGA_NEW_GS |
   SVGA_NEW_TCS |
   SVGA_NEW_TES |
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_RAST |
   SVGA_NEW_TES_RAW_BUFFER),
      };
