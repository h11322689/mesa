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
   #include "draw/draw_context.h"
   #include "util/u_bitmask.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_cmd.h"
   #include "svga_context.h"
   #include "svga_hw_reg.h"
   #include "svga_screen.h"
         /* Hardware frontwinding is always set up as SVGA3D_FRONTWINDING_CW.
   */
   static SVGA3dFace
   svga_translate_cullmode(unsigned mode, unsigned front_ccw)
   {
      const int hw_front_ccw = 0;  /* hardware is always CW */
   switch (mode) {
   case PIPE_FACE_NONE:
         case PIPE_FACE_FRONT:
         case PIPE_FACE_BACK:
         case PIPE_FACE_FRONT_AND_BACK:
         default:
      assert(0);
         }
      static SVGA3dShadeMode
   svga_translate_flatshade(unsigned mode)
   {
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
         static unsigned
   translate_cull_mode(unsigned cull)
   {
      switch (cull) {
   case PIPE_FACE_NONE:
         case PIPE_FACE_FRONT:
         case PIPE_FACE_BACK:
         case PIPE_FACE_FRONT_AND_BACK:
      /* NOTE: we simply no-op polygon drawing in svga_draw_vbo() */
      default:
      assert(!"Bad cull mode");
         }
         int
   svga_define_rasterizer_object(struct svga_context *svga,
               {
      struct svga_screen *svgascreen = svga_screen(svga->pipe.screen);
   unsigned fill_mode = translate_fill_mode(rast->templ.fill_front);
   const unsigned cull_mode = translate_cull_mode(rast->templ.cull_face);
   const int depth_bias = rast->templ.offset_units;
   const float slope_scaled_depth_bias = rast->templ.offset_scale;
   /* PIPE_CAP_POLYGON_OFFSET_CLAMP not supported: */
   const float depth_bias_clamp = 0.0;
   const float line_width = rast->templ.line_width > 0.0f ?
         const uint8 line_factor = rast->templ.line_stipple_enable ?
         const uint16 line_pattern = rast->templ.line_stipple_enable ?
         const uint8 pv_last = !rast->templ.flatshade_first &&
         int rastId;
                     if (rast->templ.fill_front != rast->templ.fill_back) {
      /* The VGPU10 device can't handle different front/back fill modes.
   * We'll handle that with a swtnl/draw fallback.  But we need to
   * make sure we always fill triangles in that case.
   */
               if (samples > 1 && svga_have_gl43(svga) &&
               ret = SVGA3D_sm5_DefineRasterizerState_v2(svga->swc,
               rastId,
   fill_mode,
   cull_mode,
   rast->templ.front_ccw,
   depth_bias,
   depth_bias_clamp,
   slope_scaled_depth_bias,
   rast->templ.depth_clip_near,
   rast->templ.scissor,
   rast->templ.multisample,
   rast->templ.line_smooth,
   line_width,
   rast->templ.line_stipple_enable,
   line_factor,
      } else {
      ret = SVGA3D_vgpu10_DefineRasterizerState(svga->swc,
               rastId,
   fill_mode,
   cull_mode,
   rast->templ.front_ccw,
   depth_bias,
   depth_bias_clamp,
   slope_scaled_depth_bias,
   rast->templ.depth_clip_near,
   rast->templ.scissor,
   rast->templ.multisample,
   rast->templ.line_smooth,
   line_width,
   rast->templ.line_stipple_enable,
               if (ret != PIPE_OK) {
      util_bitmask_clear(svga->rast_object_id_bm, rastId);
                  }
         static void *
   svga_create_rasterizer_state(struct pipe_context *pipe,
         {
      struct svga_context *svga = svga_context(pipe);
   struct svga_rasterizer_state *rast = CALLOC_STRUCT(svga_rasterizer_state);
            if (!rast)
            /* need this for draw module. */
            rast->shademode = svga_translate_flatshade(templ->flatshade);
   rast->cullmode = svga_translate_cullmode(templ->cull_face, templ->front_ccw);
   rast->scissortestenable = templ->scissor;
   rast->multisampleantialias = templ->multisample;
   rast->antialiasedlineenable = templ->line_smooth;
   rast->lastpixel = templ->line_last_pixel;
            if (rast->templ.multisample) {
      /* The OpenGL 3.0 spec says points are always drawn as circles when
   * MSAA is enabled.  Note that our implementation isn't 100% correct,
   * though.  Our smooth point implementation involves drawing a square,
   * computing fragment distance from point center, then attenuating
   * the fragment alpha value.  We should not attenuate alpha if msaa
   * is enabled.  We should discard fragments entirely outside the circle
   * and let the GPU compute per-fragment coverage.
   * But as-is, our implementation gives acceptable results and passes
   * Piglit's MSAA point smooth test.
   */
               if (rast->templ.point_smooth &&
      rast->templ.point_size_per_vertex == 0 &&
   rast->templ.point_size <= screen->pointSmoothThreshold) {
   /* If the point size is less than the threshold, deactivate smoothing.
   * Note that this only effects point rendering when we use the
   * pipe_rasterizer_state::point_size value, not when the point size
   * is set in the VS.
   */
               if (rast->templ.point_smooth) {
      /* For smooth points we need to generate fragments for at least
   * a 2x2 region.  Otherwise the quad we draw may be too small and
   * we may generate no fragments at all.
   */
      }
   else {
                           /* Use swtnl + decomposition implement these:
            if (templ->line_width <= screen->maxLineWidth) {
      /* pass line width to device */
      }
   else if (svga->debug.no_line_width) {
         }
   else {
      /* use 'draw' pipeline for wide line */
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_LINES;
               if (templ->line_stipple_enable) {
      if (screen->haveLineStipple || svga->debug.force_hw_line_stipple) {
      SVGA3dLinePattern lp;
   lp.repeat = templ->line_stipple_factor + 1;
   lp.pattern = templ->line_stipple_pattern;
      }
   else {
      /* use 'draw' module to decompose into short line segments */
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_LINES;
                  if (!svga_have_vgpu10(svga) && rast->templ.point_smooth) {
      rast->need_pipeline |= SVGA_PIPELINE_FLAG_POINTS;
               if (templ->line_smooth && !screen->haveLineSmooth) {
      /*
   * XXX: Enabling the pipeline slows down performance immensely, so ignore
   * line smooth state, where there is very little visual improvement.
   * Smooth lines will still be drawn for wide lines.
   #if 0
         rast->need_pipeline |= SVGA_PIPELINE_FLAG_LINES;
   #endif
               {
      int fill_front = templ->fill_front;
   int fill_back = templ->fill_back;
   int fill = PIPE_POLYGON_MODE_FILL;
   bool offset_front = util_get_offset(templ, fill_front);
   bool offset_back = util_get_offset(templ, fill_back);
            switch (templ->cull_face) {
   case PIPE_FACE_FRONT_AND_BACK:
      offset = false;
               case PIPE_FACE_FRONT:
      offset = offset_back;
               case PIPE_FACE_BACK:
      offset = offset_front;
               case PIPE_FACE_NONE:
      if (fill_front != fill_back || offset_front != offset_back) {
      /* Always need the draw module to work out different
   * front/back fill modes:
   */
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
   rast->need_pipeline_tris_str = "different front/back fillmodes";
      }
   else {
      offset = offset_front;
                  default:
      assert(0);
               /* Unfilled primitive modes aren't implemented on all virtual
   * hardware.  We can do some unfilled processing with index
   * translation, but otherwise need the draw module:
   */
   if (fill != PIPE_POLYGON_MODE_FILL &&
      (templ->flatshade ||
   templ->light_twoside ||
   offset)) {
   fill = PIPE_POLYGON_MODE_FILL;
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
               /* If we are decomposing to lines, and lines need the pipeline,
   * then we also need the pipeline for tris.
   */
   if (fill == PIPE_POLYGON_MODE_LINE &&
      (rast->need_pipeline & SVGA_PIPELINE_FLAG_LINES)) {
   fill = PIPE_POLYGON_MODE_FILL;
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
               /* Similarly for points:
   */
   if (fill == PIPE_POLYGON_MODE_POINT &&
      (rast->need_pipeline & SVGA_PIPELINE_FLAG_POINTS)) {
   fill = PIPE_POLYGON_MODE_FILL;
   rast->need_pipeline |= SVGA_PIPELINE_FLAG_TRIS;
               if (offset) {
      rast->slopescaledepthbias = templ->offset_scale;
                           if (rast->need_pipeline & SVGA_PIPELINE_FLAG_TRIS) {
      /* Turn off stuff which will get done in the draw module:
   */
   rast->hw_fillmode = PIPE_POLYGON_MODE_FILL;
   rast->slopescaledepthbias = 0;
               if (0 && rast->need_pipeline) {
      debug_printf("svga: rast need_pipeline = 0x%x\n", rast->need_pipeline);
   debug_printf(" pnts: %s \n", rast->need_pipeline_points_str);
   debug_printf(" lins: %s \n", rast->need_pipeline_lines_str);
               if (svga_have_vgpu10(svga)) {
      rast->id = svga_define_rasterizer_object(svga, rast, 0);
   if (rast->id == SVGA3D_INVALID_ID) {
      svga_context_flush(svga, NULL);
   rast->id = svga_define_rasterizer_object(svga, rast, 0);
                  if (svga_have_gl43(svga)) {
      /* initialize the alternate rasterizer state ids.
   * For 0 and 1 sample count, we can use the same rasterizer object.
   */
            for (unsigned i = 2; i < ARRAY_SIZE(rast->altRastIds); i++) {
                     if (templ->poly_smooth) {
      util_debug_message(&svga->debug.callback, CONFORMANCE,
               svga->hud.num_rasterizer_objects++;
   SVGA_STATS_COUNT_INC(svga_screen(svga->pipe.screen)->sws,
               }
         static void
   svga_bind_rasterizer_state(struct pipe_context *pipe, void *state)
   {
      struct svga_context *svga = svga_context(pipe);
            if (!raster || !svga->curr.rast) {
         }
   else {
      if (raster->templ.poly_stipple_enable !=
      svga->curr.rast->templ.poly_stipple_enable) {
      }
   if (raster->templ.rasterizer_discard !=
      svga->curr.rast->templ.rasterizer_discard) {
                              }
         static void
   svga_delete_rasterizer_state(struct pipe_context *pipe, void *state)
   {
      struct svga_context *svga = svga_context(pipe);
   struct svga_rasterizer_state *raster =
            /* free any alternate rasterizer state used for point sprite */
   if (raster->no_cull_rasterizer)
            if (svga_have_vgpu10(svga)) {
      SVGA_RETRY(svga, SVGA3D_vgpu10_DestroyRasterizerState(svga->swc,
            if (raster->id == svga->state.hw_draw.rasterizer_id)
                        FREE(state);
      }
         void
   svga_init_rasterizer_functions(struct svga_context *svga)
   {
      svga->pipe.create_rasterizer_state = svga_create_rasterizer_state;
   svga->pipe.bind_rasterizer_state = svga_bind_rasterizer_state;
      }
