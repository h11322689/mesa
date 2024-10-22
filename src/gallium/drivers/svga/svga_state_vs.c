   /**********************************************************
   * Copyright 2008-2022 VMware, Inc.  All rights reserved.
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
   #include "pipe/p_defines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
   #include "translate/translate.h"
   #include "tgsi/tgsi_ureg.h"
      #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_cmd.h"
   #include "svga_shader.h"
   #include "svga_tgsi.h"
      #include "svga_hw_reg.h"
         /**
   * If we fail to compile a vertex shader we'll use a dummy/fallback shader
   * that simply emits a (0,0,0,1) vertex position.
   */
   static const struct tgsi_token *
   get_dummy_vertex_shader(void)
   {
      static const float zero[4] = { 0.0, 0.0, 0.0, 1.0 };
   struct ureg_program *ureg;
   const struct tgsi_token *tokens;
   struct ureg_src src;
            ureg = ureg_create(PIPE_SHADER_VERTEX);
   if (!ureg)
            dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   src = ureg_DECL_immediate(ureg, zero, 4);
   ureg_MOV(ureg, dst, src);
                                 }
         /**
   * Replace the given shader's instruction with a simple / dummy shader.
   * We use this when normal shader translation fails.
   */
   struct svga_shader_variant *
   svga_get_compiled_dummy_vertex_shader(struct svga_context *svga,
               {
      struct svga_vertex_shader *vs = (struct svga_vertex_shader *)shader;
   const struct tgsi_token *dummy = get_dummy_vertex_shader();
            if (!dummy) {
                  FREE((void *) vs->base.tokens);
                     variant = svga_tgsi_compile_shader(svga, shader, key);
      }
         /* SVGA_NEW_PRESCALE, SVGA_NEW_RAST, SVGA_NEW_FS
   */
   static void
   make_vs_key(struct svga_context *svga, struct svga_compile_key *key)
   {
                        if (svga->state.sw.need_swtnl && svga_have_vgpu10(svga)) {
      /* Set both of these flags, to match compile_passthrough_vs() */
   key->vs.passthrough = 1;
   key->vs.undo_viewport = 1;
               if (svga_have_vgpu10(svga)) {
                  /* SVGA_NEW_PRESCALE */
   key->vs.need_prescale = svga->state.hw_clear.prescale[0].enabled &&
                  /* SVGA_NEW_RAST */
            /* SVGA_NEW_FS */
                     /* SVGA_NEW_VELEMENT */
   key->vs.adjust_attrib_range = svga->curr.velems->adjust_attrib_range;
   key->vs.adjust_attrib_w_1 = svga->curr.velems->adjust_attrib_w_1;
   key->vs.attrib_is_pure_int = svga->curr.velems->attrib_is_pure_int;
   key->vs.adjust_attrib_itof = svga->curr.velems->adjust_attrib_itof;
   key->vs.adjust_attrib_utof = svga->curr.velems->adjust_attrib_utof;
   key->vs.attrib_is_bgra = svga->curr.velems->attrib_is_bgra;
   key->vs.attrib_puint_to_snorm = svga->curr.velems->attrib_puint_to_snorm;
   key->vs.attrib_puint_to_uscaled = svga->curr.velems->attrib_puint_to_uscaled;
            /* SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER */
            /* SVGA_NEW_RAST */
            /* Determine if this shader is the last shader in the vertex
   * processing stage.
   */
   key->last_vertex_stage = !(svga->curr.gs ||
      }
         /**
   * svga_reemit_vs_bindings - Reemit the vertex shader bindings
   */
   enum pipe_error
   svga_reemit_vs_bindings(struct svga_context *svga)
   {
      enum pipe_error ret;
   struct svga_winsys_gb_shader *gbshader = NULL;
            assert(svga->rebind.flags.vs);
            if (svga->state.hw_draw.vs) {
      gbshader = svga->state.hw_draw.vs->gb_shader;
               if (!svga_need_to_rebind_resources(svga)) {
      ret =  svga->swc->resource_rebind(svga->swc, NULL, gbshader,
      }
   else {
      if (svga_have_vgpu10(svga))
      ret = SVGA3D_vgpu10_SetShader(svga->swc, SVGA3D_SHADERTYPE_VS,
      else
               if (ret != PIPE_OK)
            svga->rebind.flags.vs = false;
      }
         /**
   * The current vertex shader is already executed by the 'draw'
   * module, so we just need to generate a simple vertex shader
   * to pass through all those VS outputs that will
   * be consumed by the fragment shader.
   * Used when we employ the 'draw' module.
   */
   static enum pipe_error
   compile_passthrough_vs(struct svga_context *svga,
                     {
      struct svga_shader_variant *variant = NULL;
   unsigned num_inputs;
   unsigned i;
   unsigned num_elements;
   struct svga_vertex_shader new_vs;
   struct ureg_src src[PIPE_MAX_SHADER_INPUTS];
   struct ureg_dst dst[PIPE_MAX_SHADER_OUTPUTS];
   struct ureg_program *ureg;
   struct svga_compile_key key;
            assert(svga_have_vgpu10(svga));
                     ureg = ureg_create(PIPE_SHADER_VERTEX);
   if (!ureg)
            /* draw will always add position */
   dst[0] = ureg_DECL_output(ureg, TGSI_SEMANTIC_POSITION, 0);
   src[0] = ureg_DECL_vs_input(ureg, 0);
            /**
   * swtnl backend redefines the input layout based on the
   * fragment shader's inputs. So we only need to passthrough
   * those inputs that will be consumed by the fragment shader.
   * Note: DX10 requires the number of vertex elements
   * specified in the input layout to be no less than the
   * number of inputs to the vertex shader.
   */
   for (i = 0; i < num_inputs; i++) {
      switch (fs->base.tgsi_info.input_semantic_name[i]) {
   case TGSI_SEMANTIC_COLOR:
   case TGSI_SEMANTIC_GENERIC:
   case TGSI_SEMANTIC_FOG:
      dst[num_elements] = ureg_DECL_output(ureg,
               src[num_elements] = ureg_DECL_vs_input(ureg, num_elements);
   num_elements++;
      default:
                     for (i = 0; i < num_elements; i++) {
                           memset(&new_vs, 0, sizeof(new_vs));
   new_vs.base.tokens = ureg_get_tokens(ureg, NULL);
            memset(&key, 0, sizeof(key));
            ret = svga_compile_shader(svga, &new_vs.base, &key, &variant);
   if (ret != PIPE_OK)
            ureg_free_tokens(new_vs.base.tokens);
            /* Overwrite the variant key to indicate it's a pass-through VS */
   memset(&variant->key, 0, sizeof(variant->key));
   variant->key.vs.passthrough = 1;
                        }
         static enum pipe_error
   emit_hw_vs(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_shader_variant *variant;
   struct svga_vertex_shader *vs = svga->curr.vs;
   struct svga_fragment_shader *fs = svga->curr.fs;
   enum pipe_error ret = PIPE_OK;
                     /* If there is an active geometry shader, and it has stream output
   * defined, then we will skip the stream output from the vertex shader
   */
   if (!svga_have_gs_streamout(svga)) {
      /* No GS stream out */
   if (svga_have_vs_streamout(svga)) {
      /* Set VS stream out */
      }
   else {
      /* turn off stream out */
      }
   if (ret != PIPE_OK) {
                     /* SVGA_NEW_NEED_SWTNL */
   if (svga->state.sw.need_swtnl && !svga_have_vgpu10(svga)) {
      /* No vertex shader is needed */
      }
   else {
               /* See if we already have a VS variant that matches the key */
            if (!variant) {
      /* Create VS variant now */
   if (key.vs.passthrough) {
         }
   else {
         }
   if (ret != PIPE_OK)
                  if (variant != svga->state.hw_draw.vs) {
      /* Bind the new variant */
   if (variant) {
      ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_VS, variant);
   if (ret != PIPE_OK)
                     svga->dirty |= SVGA_NEW_VS_VARIANT;
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
      struct svga_tracked_state svga_hw_vs = 
   {
      "vertex shader (hwtnl)",
   (SVGA_NEW_VS |
   SVGA_NEW_FS |
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_RAST |
   SVGA_NEW_PRESCALE |
   SVGA_NEW_VELEMENT |
   SVGA_NEW_NEED_SWTNL |
   SVGA_NEW_VS_RAW_BUFFER),
      };
