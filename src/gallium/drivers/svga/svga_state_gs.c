   /**********************************************************
   * Copyright 2014-2022 VMware, Inc.  All rights reserved.
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
   #include "util/u_bitmask.h"
   #include "translate/translate.h"
      #include "svga_context.h"
   #include "svga_cmd.h"
   #include "svga_shader.h"
   #include "svga_tgsi.h"
   #include "svga_streamout.h"
   #include "svga_format.h"
      /**
   * If we fail to compile a geometry shader we'll use a dummy/fallback shader
   * that simply emits the incoming vertices.
   */
   static const struct tgsi_token *
   get_dummy_geometry_shader(void)
   {
      //XXX
      }
         struct svga_shader_variant *
   svga_get_compiled_dummy_geometry_shader(struct svga_context *svga,
               {
      const struct tgsi_token *dummy = get_dummy_geometry_shader();
   struct svga_shader_variant *variant;
            if (!dummy)
            FREE((void *) gs->base.tokens);
   gs->base.tokens = dummy;
   svga_tgsi_scan_shader(&gs->base);
               }
         static void
   make_gs_key(struct svga_context *svga, struct svga_compile_key *key)
   {
                        /*
   * SVGA_NEW_TEXTURE_BINDING | SVGA_NEW_SAMPLER
   */
            memcpy(key->generic_remap_table, gs->generic_remap_table,
                              key->gs.writes_psize = gs->base.info.writes_psize;
   key->gs.wide_point = gs->wide_point;
   key->gs.writes_viewport_index = gs->base.info.writes_viewport_index;
   if (key->gs.writes_viewport_index) {
         } else {
         }
   key->sprite_coord_enable = svga->curr.rast->templ.sprite_coord_enable;
   key->sprite_origin_lower_left = (svga->curr.rast->templ.sprite_coord_mode
            /* SVGA_NEW_RAST */
            /* Mark this as the last shader in the vertex processing stage */
      }
         static enum pipe_error
   emit_hw_gs(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_shader_variant *variant;
   struct svga_geometry_shader *gs = svga->curr.gs;
   enum pipe_error ret = PIPE_OK;
                              /* If there's a user-defined GS, we should have a pointer to a derived
   * GS.  This should have been resolved in update_tgsi_transform().
   */
   if (svga->curr.user_gs)
            if (!gs) {
                  /** The previous geometry shader is made inactive.
   *  Needs to unbind the geometry shader.
   */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_GS, NULL);
   if (ret != PIPE_OK)
            }
               /* If there is stream output info for this geometry shader, then use
   * it instead of the one from the vertex shader.
   */
   if (svga_have_gs_streamout(svga)) {
      ret = svga_set_stream_output(svga, gs->base.stream_output);
   if (ret != PIPE_OK) {
            }
   else if (!svga_have_vs_streamout(svga)) {
      /* turn off stream out */
   ret = svga_set_stream_output(svga, NULL);
   if (ret != PIPE_OK) {
                     /* SVGA_NEW_NEED_SWTNL */
   if (svga->state.sw.need_swtnl && !svga_have_vgpu10(svga)) {
      /* No geometry shader is needed */
      }
   else {
               /* See if we already have a GS variant that matches the key */
            if (!variant) {
      ret = svga_compile_shader(svga, &gs->base, &key, &variant);
   if (ret != PIPE_OK)
                  if (variant != svga->state.hw_draw.gs) {
      /* Bind the new variant */
   ret = svga_set_shader(svga, SVGA3D_SHADERTYPE_GS, variant);
   if (ret != PIPE_OK)
            svga->rebind.flags.gs = false;
   svga->dirty |= SVGA_NEW_GS_VARIANT;
            done:
      SVGA_STATS_TIME_POP(svga_sws(svga));
      }
      struct svga_tracked_state svga_hw_gs =
   {
      "geometry shader (hwtnl)",
   (SVGA_NEW_VS |
   SVGA_NEW_FS |
   SVGA_NEW_GS |
   SVGA_NEW_TEXTURE_BINDING |
   SVGA_NEW_SAMPLER |
   SVGA_NEW_RAST |
   SVGA_NEW_NEED_SWTNL |
   SVGA_NEW_GS_RAW_BUFFER),
      };
