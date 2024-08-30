   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
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
      #include "pipe/p_defines.h"
   #include "util/u_bitmask.h"
   #include "util/format/u_format.h"
   #include "util/u_inlines.h"
   #include "util/u_memory.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_context.h"
   #include "svga_screen.h"
   #include "svga_state.h"
   #include "svga_cmd.h"
   #include "svga_format.h"
   #include "svga_shader.h"
         struct rs_queue {
      unsigned rs_count;
      };
         #define EMIT_RS(svga, value, token)                       \
   do {                                                            \
      STATIC_ASSERT(SVGA3D_RS_##token < ARRAY_SIZE(svga->state.hw_draw.rs)); \
   if (svga->state.hw_draw.rs[SVGA3D_RS_##token] != value) {    \
      svga_queue_rs(&queue, SVGA3D_RS_##token, value);          \
         } while (0)
      #define EMIT_RS_FLOAT(svga, fvalue, token)                \
   do {                                                            \
      unsigned value = fui(fvalue);                                \
   STATIC_ASSERT(SVGA3D_RS_##token < ARRAY_SIZE(svga->state.hw_draw.rs)); \
   if (svga->state.hw_draw.rs[SVGA3D_RS_##token] != value) {    \
      svga_queue_rs(&queue, SVGA3D_RS_##token, value);          \
         } while (0)
         static inline void
   svga_queue_rs(struct rs_queue *q, unsigned rss, unsigned value)
   {
      assert(q->rs_count < ARRAY_SIZE(q->rs));
   q->rs[q->rs_count].state = rss;
   q->rs[q->rs_count].uintValue = value;
      }
         static unsigned
   translate_fill_mode(unsigned fill)
   {
      switch (fill) {
   case PIPE_POLYGON_MODE_POINT:
         case PIPE_POLYGON_MODE_LINE:
         case PIPE_POLYGON_MODE_FILL:
         default:
      assert(!"Bad fill mode");
         }
         /* Compare old and new render states and emit differences between them
   * to hardware.  Simplest implementation would be to emit the whole of
   * the "to" state.
   */
   static enum pipe_error
   emit_rss_vgpu9(struct svga_context *svga, uint64_t dirty)
   {
      struct svga_screen *screen = svga_screen(svga->pipe.screen);
   struct rs_queue queue;
                     if (dirty & (SVGA_NEW_BLEND | SVGA_NEW_BLEND_COLOR)) {
               EMIT_RS(svga, curr->rt[0].writemask, COLORWRITEENABLE);
            if (curr->rt[0].blend_enable) {
      EMIT_RS(svga, curr->rt[0].srcblend, SRCBLEND);
                                 if (curr->rt[0].separate_alpha_blend_enable) {
      EMIT_RS(svga, curr->rt[0].srcblend_alpha, SRCBLENDALPHA);
   EMIT_RS(svga, curr->rt[0].dstblend_alpha, DSTBLENDALPHA);
                     if (dirty & SVGA_NEW_BLEND_COLOR) {
      uint32 color;
   uint32 r = float_to_ubyte(svga->curr.blend_color.color[0]);
   uint32 g = float_to_ubyte(svga->curr.blend_color.color[1]);
   uint32 b = float_to_ubyte(svga->curr.blend_color.color[2]);
                                 if (dirty & (SVGA_NEW_DEPTH_STENCIL_ALPHA | SVGA_NEW_RAST)) {
      const struct svga_depth_stencil_state *curr = svga->curr.depth;
            if (!curr->stencil[0].enabled) {
      /* Stencil disabled
   */
   EMIT_RS(svga, false, STENCILENABLE);
      }
   else if (curr->stencil[0].enabled && !curr->stencil[1].enabled) {
      /* Regular stencil
   */
                  EMIT_RS(svga, curr->stencil[0].func,  STENCILFUNC);
   EMIT_RS(svga, curr->stencil[0].fail,  STENCILFAIL);
                  EMIT_RS(svga, curr->stencil_mask, STENCILMASK);
      }
   else {
               /* Hardware frontwinding is always CW, so if ours is also CW,
   * then our definition of front face agrees with hardware.
   * Otherwise need to flip.
   */
   if (rast->templ.front_ccw) {
      ccw = 0;
      }
   else {
      ccw = 1;
               /* Twoside stencil
   */
                  EMIT_RS(svga, curr->stencil[cw].func,  STENCILFUNC);
   EMIT_RS(svga, curr->stencil[cw].fail,  STENCILFAIL);
                  EMIT_RS(svga, curr->stencil[ccw].func,  CCWSTENCILFUNC);
   EMIT_RS(svga, curr->stencil[ccw].fail,  CCWSTENCILFAIL);
                  EMIT_RS(svga, curr->stencil_mask, STENCILMASK);
               EMIT_RS(svga, curr->zenable, ZENABLE);
   if (curr->zenable) {
      EMIT_RS(svga, curr->zfunc, ZFUNC);
               EMIT_RS(svga, curr->alphatestenable, ALPHATESTENABLE);
   if (curr->alphatestenable) {
      EMIT_RS(svga, curr->alphafunc, ALPHAFUNC);
                  if (dirty & SVGA_NEW_STENCIL_REF) {
                  if (dirty & (SVGA_NEW_RAST | SVGA_NEW_NEED_PIPELINE)) {
      const struct svga_rasterizer_state *curr = svga->curr.rast;
            /* Shademode: still need to rearrange index list to move
   * flat-shading PV first vertex.
   */
                     /* Don't do culling while the software pipeline is active.  It
   * does it for us, and additionally introduces potentially
   * back-facing triangles.
   */
   if (svga->state.sw.need_pipeline)
                     EMIT_RS(svga, cullmode, CULLMODE);
   EMIT_RS(svga, curr->scissortestenable, SCISSORTESTENABLE);
   EMIT_RS(svga, curr->multisampleantialias, MULTISAMPLEANTIALIAS);
   EMIT_RS(svga, curr->lastpixel, LASTPIXEL);
   EMIT_RS_FLOAT(svga, curr->pointsize, POINTSIZE);
   EMIT_RS_FLOAT(svga, point_size_min, POINTSIZEMIN);
   EMIT_RS_FLOAT(svga, screen->maxPointSize, POINTSIZEMAX);
            /* Emit line state, when the device understands it */
   if (screen->haveLineStipple)
         if (screen->haveLineSmooth)
         if (screen->maxLineWidth > 1.0F)
               if (dirty & (SVGA_NEW_RAST |
                  const struct svga_rasterizer_state *curr = svga->curr.rast;
   float slope = 0.0;
            /* Need to modify depth bias according to bound depthbuffer
   * format.  Don't do hardware depthbias while the software
   * pipeline is active.
   */
   if (!svga->state.sw.need_pipeline &&
         {
      slope = curr->slopescaledepthbias;
               EMIT_RS_FLOAT(svga, slope, SLOPESCALEDEPTHBIAS);
               if (dirty & SVGA_NEW_FRAME_BUFFER) {
      /* XXX: we only look at the first color buffer's sRGB state */
   float gamma = 1.0f;
   if (svga->curr.framebuffer.cbufs[0] &&
      util_format_is_srgb(svga->curr.framebuffer.cbufs[0]->format)) {
      }
               if (dirty & SVGA_NEW_RAST) {
      /* bitmask of the enabled clip planes */
   unsigned enabled = svga->curr.rast->templ.clip_plane_enable;
               if (queue.rs_count) {
               if (SVGA3D_BeginSetRenderState(svga->swc, &rs, queue.rs_count)
      != PIPE_OK) {
   /* XXX: need to poison cached hardware state on failure to ensure
   * dirty state gets re-emitted.  Fix this by re-instating partial
   * FIFOCommit command and only updating cached hw state once the
   * initial allocation has succeeded.
                                                      }
         /** Returns a non-culling rasterizer state object to be used with
   *  point sprite.
   */
   static struct svga_rasterizer_state *
   get_no_cull_rasterizer_state(struct svga_context *svga)
   {
               if (!r->no_cull_rasterizer) {
               memset(&rast, 0, sizeof(rast));
   rast.flatshade = 1;
   rast.front_ccw = 1;
            /* All rasterizer states have the same half_pixel_center,
   * bottom_edge_rule and clip_halfz values since they are
   * constant for a context. If we ever implement
   * GL_ARB_clip_control, the clip_halfz field would have to be observed.
   */
   rast.half_pixel_center = r->templ.half_pixel_center;
   rast.bottom_edge_rule = r->templ.bottom_edge_rule;
            r->no_cull_rasterizer =
      }
      }
         /** Returns a depth stencil state object with depth and stencil test disabled.
   */
   static struct svga_depth_stencil_state *
   get_no_depth_stencil_test_state(struct svga_context *svga)
   {
      if (!svga->depthstencil_disable) {
      struct pipe_depth_stencil_alpha_state ds = {{{0}}};
   svga->depthstencil_disable =
      }
      }
         /**
   * A helper function to create an alternate svga rasterizer state object to use
   * forcedSampleCount to support multisampled framebuffer without attachments.
   */
   static SVGA3dRasterizerStateId
   get_alt_rasterizer_state_id(struct svga_context *svga,
               {
      assert(samples <= SVGA_MAX_FRAMEBUFFER_DEFAULT_SAMPLES);
            if (samples <= 1)
            if (rast->altRastIds[samples] == SVGA3D_INVALID_ID) {
                     }
         static enum pipe_error
   emit_rss_vgpu10(struct svga_context *svga, uint64_t dirty)
   {
                        if (dirty & (SVGA_NEW_BLEND | SVGA_NEW_BLEND_COLOR)) {
      const struct svga_blend_state *curr;
            if (svga_has_any_integer_cbufs(svga)) {
      /* Blending is not supported in integer-valued render targets. */
   curr = svga->noop_blend;
   blend_factor[0] =
   blend_factor[1] =
   blend_factor[2] =
      }
   else {
               if (curr->blend_color_alpha) {
      blend_factor[0] =
   blend_factor[1] =
   blend_factor[2] =
      }
   else {
      blend_factor[0] = svga->curr.blend_color.color[0];
   blend_factor[1] = svga->curr.blend_color.color[1];
   blend_factor[2] = svga->curr.blend_color.color[2];
                  /* Set/bind the blend state object */
   if (svga->state.hw_draw.blend_id != curr->id ||
      svga->state.hw_draw.blend_factor[0] != blend_factor[0] ||
   svga->state.hw_draw.blend_factor[1] != blend_factor[1] ||
   svga->state.hw_draw.blend_factor[2] != blend_factor[2] ||
   svga->state.hw_draw.blend_factor[3] != blend_factor[3] ||
   svga->state.hw_draw.blend_sample_mask != svga->curr.sample_mask) {
   ret = SVGA3D_vgpu10_SetBlendState(svga->swc, curr->id,
                              svga->state.hw_draw.blend_id = curr->id;
   svga->state.hw_draw.blend_factor[0] = blend_factor[0];
   svga->state.hw_draw.blend_factor[1] = blend_factor[1];
   svga->state.hw_draw.blend_factor[2] = blend_factor[2];
   svga->state.hw_draw.blend_factor[3] = blend_factor[3];
                  if (svga->disable_rasterizer) {
      if (!svga->state.hw_draw.rasterizer_discard) {
               /* If rasterization is to be disabled, disable depth and stencil
   * testing as well.
   */
   ds = get_no_depth_stencil_test_state(svga);
   if (ds->id != svga->state.hw_draw.depth_stencil_id) {
      ret = SVGA3D_vgpu10_SetDepthStencilState(svga->swc, ds->id, 0);
                  svga->state.hw_draw.depth_stencil_id = ds->id;
      }
         } else {
      if ((dirty & (SVGA_NEW_DEPTH_STENCIL_ALPHA | SVGA_NEW_STENCIL_REF)) ||
      svga->state.hw_draw.rasterizer_discard) {
                  if (curr->id != svga->state.hw_draw.depth_stencil_id ||
      curr_ref != svga->state.hw_draw.stencil_ref) {
   /* Set/bind the depth/stencil state object */
   ret = SVGA3D_vgpu10_SetDepthStencilState(svga->swc, curr->id,
                        svga->state.hw_draw.depth_stencil_id = curr->id;
                  if (dirty & (SVGA_NEW_REDUCED_PRIMITIVE | SVGA_NEW_RAST |
                                       /* If we are drawing a point sprite, we will need to
   * bind a non-culling rasterizer state object
   */
                        /* In the case of no-attachment framebuffer, the sample count will be
   * specified in forcedSampleCount in the RasterizerState_v2 object.
   */
   if ((svga->curr.framebuffer.nr_cbufs == 0) &&
      (svga->curr.framebuffer.zsbuf == NULL)) {
   rastId =
                  if (rastId == SVGA3D_INVALID_ID)
               if (svga->state.hw_draw.rasterizer_id != rastId) {
      /* Set/bind the rasterizer state object */
   ret = SVGA3D_vgpu10_SetRasterizerState(svga->swc, rastId);
   if (ret != PIPE_OK)
               }
      }
      }
         static enum pipe_error
   emit_rss(struct svga_context *svga, uint64_t dirty)
   {
      if (svga_have_vgpu10(svga)) {
         }
   else {
            }
         struct svga_tracked_state svga_hw_rss =
   {
               (SVGA_NEW_BLEND |
   SVGA_NEW_BLEND_COLOR |
   SVGA_NEW_DEPTH_STENCIL_ALPHA |
   SVGA_NEW_STENCIL_REF |
   SVGA_NEW_RAST |
   SVGA_NEW_FRAME_BUFFER |
   SVGA_NEW_NEED_PIPELINE |
   SVGA_NEW_FS |
               };
