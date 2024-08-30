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
   #include "util/format/u_format.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_bitmask.h"
   #include "tgsi/tgsi_ureg.h"
      #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_cmd.h"
   #include "svga_shader.h"
   #include "svga_resource_texture.h"
   #include "svga_tgsi.h"
   #include "svga_format.h"
      #include "svga_hw_reg.h"
            /**
   * If we fail to compile a fragment shader (because it uses too many
   * registers, for example) we'll use a dummy/fallback shader that
   * simply emits a constant color (red for debug, black for release).
   * We hit this with the Unigine/Heaven demo when Shaders = High.
   * With black, the demo still looks good.
   */
   static const struct tgsi_token *
   get_dummy_fragment_shader(void)
   {
   #ifdef DEBUG
         #else
         #endif
      struct ureg_program *ureg;
   const struct tgsi_token *tokens;
   struct ureg_src src;
            ureg = ureg_create(PIPE_SHADER_FRAGMENT);
   if (!ureg)
            dst = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   src = ureg_DECL_immediate(ureg, color, 4);
   ureg_MOV(ureg, dst, src);
                                 }
         /**
   * Replace the given shader's instruction with a simple constant-color
   * shader.  We use this when normal shader translation fails.
   */
   struct svga_shader_variant *
   svga_get_compiled_dummy_fragment_shader(struct svga_context *svga,
               {
      struct svga_fragment_shader *fs = (struct svga_fragment_shader *)shader;
   const struct tgsi_token *dummy = get_dummy_fragment_shader();
            if (!dummy) {
                  FREE((void *) fs->base.tokens);
            svga_tgsi_scan_shader(&fs->base);
   svga_remap_generics(fs->base.info.generic_inputs_mask,
            variant = svga_tgsi_compile_shader(svga, shader, key);
      }
         /* SVGA_NEW_TEXTURE_BINDING
   * SVGA_NEW_RAST
   * SVGA_NEW_NEED_SWTNL
   * SVGA_NEW_SAMPLER
   */
   static enum pipe_error
   make_fs_key(const struct svga_context *svga,
               {
      const enum pipe_shader_type shader = PIPE_SHADER_FRAGMENT;
                     memcpy(key->generic_remap_table, fs->generic_remap_table,
            /* SVGA_NEW_GS, SVGA_NEW_VS
   */
   struct svga_geometry_shader *gs = svga->curr.gs;
   struct svga_vertex_shader *vs = svga->curr.vs;
   if (gs) {
      key->fs.gs_generic_outputs = gs->base.info.generic_outputs_mask;
      } else {
      key->fs.vs_generic_outputs = vs->base.info.generic_outputs_mask;
               /* Only need fragment shader fixup for twoside lighting if doing
   * hwtnl.  Otherwise the draw module does the whole job for us.
   *
   * SVGA_NEW_SWTNL
   */
   if (!svga->state.sw.need_swtnl) {
      /* SVGA_NEW_RAST, SVGA_NEW_REDUCED_PRIMITIVE
   */
   enum mesa_prim prim_mode;
            /* Find the last shader in the vertex pipeline and the output primitive mode
   * from that shader.
   */
   if (svga->curr.tes) {
      shader = &svga->curr.tes->base;
      } else if (svga->curr.gs) {
      shader = &svga->curr.gs->base;
      } else {
      shader = &svga->curr.vs->base;
               key->fs.light_twoside = svga->curr.rast->templ.light_twoside;
   key->fs.front_ccw = svga->curr.rast->templ.front_ccw;
   key->fs.pstipple = (svga->curr.rast->templ.poly_stipple_enable &&
            if (svga->curr.gs) {
         shader->info.gs.in_prim == MESA_PRIM_POINTS &&
                     if (key->fs.aa_point) {
      }
                     /* The blend workaround for simulating logicop xor behaviour
   * requires that the incoming fragment color be white.  This change
   * achieves that by creating a variant of the current fragment
   * shader that overrides all output colors with 1,1,1,1
   *   
   * This will work for most shaders, including those containing
   * TEXKIL and/or depth-write.  However, it will break on the
   * combination of xor-logicop plus alphatest.
   *
   * Ultimately, we could implement alphatest in the shader using
   * texkil prior to overriding the outgoing fragment color.
   *   
   * SVGA_NEW_BLEND
   */
                  #ifdef DEBUG
      /*
   * We expect a consistent set of samplers and sampler views.
   * Do some debug checks/warnings here.
   */
   {
      static bool warned = false;
   unsigned i, n = MAX2(svga->curr.num_sampler_views[shader],
         /* Only warn once to prevent too much debug output */
   if (!warned) {
      if (svga->curr.num_sampler_views[shader] !=
      svga->curr.num_samplers[shader]) {
   debug_printf("svga: mismatched number of sampler views (%u) "
                  }
   for (i = 0; i < n; i++) {
      if ((svga->curr.sampler_views[shader][i] == NULL) !=
      (svga->curr.sampler[shader][i] == NULL))
   debug_printf("sampler_view[%u] = %p but sampler[%u] = %p\n",
         }
            #endif
         /* XXX: want to limit this to the textures that the shader actually
   * refers to.
   *
   * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
   */
            for (i = 0; i < svga->curr.num_samplers[shader]; ++i) {
      struct pipe_sampler_view *view = svga->curr.sampler_views[shader][i];
   const struct svga_sampler_state *sampler = svga->curr.sampler[shader][i];
   if (view) {
      struct pipe_resource *tex = view->texture;
   if (tex->target != PIPE_BUFFER) {
                     if (!svga_have_vgpu10(svga) &&
      (format == SVGA3D_Z_D16 ||
   format == SVGA3D_Z_D24X8 ||
   format == SVGA3D_Z_D24S8)) {
   /* If we're sampling from a SVGA3D_Z_D16, SVGA3D_Z_D24X8,
   * or SVGA3D_Z_D24S8 surface, we'll automatically get
   * shadow comparison.  But we only get LEQUAL mode.
   * Set TEX_COMPARE_NONE here so we don't emit the extra FS
   * code for shadow comparison.
   */
   key->tex[i].compare_mode = PIPE_TEX_COMPARE_NONE;
   key->tex[i].compare_func = PIPE_FUNC_NEVER;
   /* These depth formats _only_ support comparison mode and
   * not ordinary sampling so warn if the later is expected.
   */
   if (sampler->compare_mode != PIPE_TEX_COMPARE_R_TO_TEXTURE) {
         }
   /* The shader translation code can emit code to
   * handle ALWAYS and NEVER compare functions
   */
   else if (sampler->compare_func == PIPE_FUNC_ALWAYS ||
            key->tex[i].compare_mode = sampler->compare_mode;
      }
   else if (sampler->compare_func != PIPE_FUNC_LEQUAL) {
                              /* sprite coord gen state */
            key->sprite_origin_lower_left = (svga->curr.rast->templ.sprite_coord_mode
                     /* SVGA_NEW_DEPTH_STENCIL_ALPHA */
   if (svga_have_vgpu10(svga)) {
      /* Alpha testing is not supported in integer-valued render targets. */
   if (svga_has_any_integer_cbufs(svga)) {
      key->fs.alpha_func = SVGA3D_CMP_ALWAYS;
      }
   else {
      key->fs.alpha_func = svga->curr.depth->alphafunc;
                  /* SVGA_NEW_FRAME_BUFFER | SVGA_NEW_BLEND */
   if (fs->base.info.fs.color0_writes_all_cbufs ||
      svga->curr.blend->need_white_fragments) {
   /* Replicate color0 output (or white) to N colorbuffers */
                  }
         /**
   * svga_reemit_fs_bindings - Reemit the fragment shader bindings
   */
   enum pipe_error
   svga_reemit_fs_bindings(struct svga_context *svga)
   {
               assert(svga->rebind.flags.fs);
            if (!svga->state.hw_draw.fs)
            if (!svga_need_to_rebind_resources(svga)) {
      ret =  svga->swc->resource_rebind(svga->swc, NULL,
            }
   else {
      if (svga_have_vgpu10(svga))
      ret = SVGA3D_vgpu10_SetShader(svga->swc, SVGA3D_SHADERTYPE_PS,
            else
      ret = SVGA3D_SetGBShader(svga->swc, SVGA3D_SHADERTYPE_PS,
            if (ret != PIPE_OK)
            svga->rebind.flags.fs = false;
      }
            static enum pipe_error
   emit_hw_fs(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_shader_variant *variant = NULL;
   enum pipe_error ret = PIPE_OK;
   struct svga_fragment_shader *fs = svga->curr.fs;
   struct svga_compile_key key;
                     prevShader = svga->curr.gs ?
      &svga->curr.gs->base : (svga->curr.tes ?
         /* Disable rasterization if rasterizer_discard flag is set or
   * vs/gs does not output position.
   */
   svga->disable_rasterizer =
      svga->curr.rast->templ.rasterizer_discard ||
         /* Set FS to NULL when rasterization is to be disabled */
   if (svga->disable_rasterizer) {
      /* Set FS to NULL if it has not been done */
   if (svga->state.hw_draw.fs) {
      ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_PS, NULL);
   if (ret != PIPE_OK)
      }
   svga->rebind.flags.fs = false;
   svga->state.hw_draw.fs = NULL;
               /* SVGA_NEW_BLEND
   * SVGA_NEW_TEXTURE_BINDING
   * SVGA_NEW_RAST
   * SVGA_NEW_NEED_SWTNL
   * SVGA_NEW_SAMPLER
   * SVGA_NEW_FRAME_BUFFER
   * SVGA_NEW_DEPTH_STENCIL_ALPHA
   * SVGA_NEW_VS
   */
   ret = make_fs_key(svga, fs, &key);
   if (ret != PIPE_OK)
            variant = svga_search_shader_key(&fs->base, &key);
   if (!variant) {
      ret = svga_compile_shader(svga, &fs->base, &key, &variant);
   if (ret != PIPE_OK)
                        if (variant != svga->state.hw_draw.fs) {
      ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_PS, variant);
   if (ret != PIPE_OK)
                     svga->dirty |= SVGA_NEW_FS_VARIANT;
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
      struct svga_tracked_state svga_hw_fs = 
   {
      "fragment shader (hwtnl)",
   (SVGA_NEW_FS |
   SVGA_NEW_GS |
   SVGA_NEW_VS |
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_NEED_SWTNL |
   SVGA_NEW_RAST |
   SVGA_NEW_STIPPLE |
   SVGA_NEW_REDUCED_PRIMITIVE |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_FRAME_BUFFER |
   SVGA_NEW_DEPTH_STENCIL_ALPHA |
   SVGA_NEW_BLEND |
   SVGA_NEW_FS_RAW_BUFFER),
      };
         