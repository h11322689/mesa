   /**************************************************************************
   *
   * Copyright 2007 VMware, Inc.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the
   * "Software"), to deal in the Software without restriction, including
   * without limitation the rights to use, copy, modify, merge, publish,
   * distribute, sub license, and/or sell copies of the Software, and to
   * permit persons to whom the Software is furnished to do so, subject to
   * the following conditions:
   *
   * The above copyright notice and this permission notice (including the
   * next paragraph) shall be included in all copies or substantial portions
   * of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
   * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * Authors:
   *   Keith Whitwell <keithw@vmware.com>
   */
      #include "main/macros.h"
   #include "main/framebuffer.h"
   #include "main/state.h"
   #include "st_context.h"
   #include "st_atom.h"
   #include "st_debug.h"
   #include "st_program.h"
   #include "st_util.h"
   #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
   #include "cso_cache/cso_context.h"
   #include "main/context.h"
         static GLuint
   translate_fill(GLenum mode)
   {
      switch (mode) {
   case GL_POINT:
         case GL_LINE:
         case GL_FILL:
         case GL_FILL_RECTANGLE_NV:
         default:
      assert(0);
         }
      void
   st_update_rasterizer(struct st_context *st)
   {
      struct gl_context *ctx = st->ctx;
   struct pipe_rasterizer_state *raster = &st->state.rasterizer;
                     /* _NEW_POLYGON, _NEW_BUFFERS
   */
   {
               /* _NEW_TRANSFORM */
   if (ctx->Transform.ClipOrigin == GL_UPPER_LEFT) {
                  /*
   * Gallium's surfaces are Y=0=TOP orientation.  OpenGL is the
   * opposite.  Window system surfaces are Y=0=TOP.  Mesa's FBOs
   * must match OpenGL conventions so FBOs use Y=0=BOTTOM.  In that
   * case, we must invert Y and flip the notion of front vs. back.
   */
   if (st->state.fb_orientation == Y_0_BOTTOM) {
      /* Drawing to an FBO.  The viewport will be inverted. */
                  /* _NEW_LIGHT_STATE */
   raster->flatshade = !st->lower_flatshade &&
            raster->flatshade_first = ctx->Light.ProvokingVertex ==
            /* _NEW_LIGHT_STATE | _NEW_PROGRAM */
   if (!st->lower_two_sided_color)
            /*_NEW_LIGHT_STATE | _NEW_BUFFERS */
   raster->clamp_vertex_color = !st->clamp_vert_color_in_shader &&
            /* _NEW_POLYGON
   */
   if (ctx->Polygon.CullFlag) {
      switch (ctx->Polygon.CullFaceMode) {
   case GL_FRONT:
      raster->cull_face = PIPE_FACE_FRONT;
      case GL_BACK:
      raster->cull_face = PIPE_FACE_BACK;
      case GL_FRONT_AND_BACK:
      raster->cull_face = PIPE_FACE_FRONT_AND_BACK;
         }
   else {
                  /* _NEW_POLYGON
   */
   {
      if (ST_DEBUG & DEBUG_WIREFRAME) {
      raster->fill_front = PIPE_POLYGON_MODE_LINE;
      }
   else {
      raster->fill_front = translate_fill(ctx->Polygon.FrontMode);
               /* Simplify when culling is active:
   */
   if (raster->cull_face & PIPE_FACE_FRONT) {
                  if (raster->cull_face & PIPE_FACE_BACK) {
                     /* _NEW_POLYGON
   */
   if (ctx->Polygon.OffsetPoint ||
      ctx->Polygon.OffsetLine ||
   ctx->Polygon.OffsetFill) {
   raster->offset_point = ctx->Polygon.OffsetPoint;
   raster->offset_line = ctx->Polygon.OffsetLine;
   raster->offset_tri = ctx->Polygon.OffsetFill;
   raster->offset_units = ctx->Polygon.OffsetUnits;
   raster->offset_scale = ctx->Polygon.OffsetFactor;
                        /* Multisampling disables point, line, and polygon smoothing.
   *
   * GL_ARB_multisample says:
   *
   *   "If MULTISAMPLE_ARB is enabled, and SAMPLE_BUFFERS_ARB is a value of
   *    one, then points are rasterized using the following algorithm,
   *    regardless of whether point antialiasing (POINT_SMOOTH) is enabled"
   *
   *   "If MULTISAMPLE_ARB is enabled, and SAMPLE_BUFFERS_ARB is a value of
   *    one, then lines are rasterized using the following algorithm,
   *    regardless of whether line antialiasing (LINE_SMOOTH) is enabled"
   *
   *   "If MULTISAMPLE_ARB is enabled, and SAMPLE_BUFFERS_ARB is a value of
   *    one, then polygons are rasterized using the following algorithm,
   *    regardless of whether polygon antialiasing (POLYGON_SMOOTH) is
   *    enabled"
            /* _NEW_MULTISAMPLE */
   bool multisample = _mesa_is_multisample_enabled(ctx);
            /* _NEW_POLYGON | _NEW_MULTISAMPLE */
            /* _NEW_POINT
   */
            /* _NEW_POINT | _NEW_MULTISAMPLE */
   raster->point_smooth = !multisample && !ctx->Point.PointSprite &&
            /* _NEW_POINT | _NEW_PROGRAM
   */
   if (ctx->Point.PointSprite) {
      /* origin */
   if ((ctx->Point.SpriteOrigin == GL_UPPER_LEFT) ^
      (st->state.fb_orientation == Y_0_BOTTOM))
      else
            /* Coord replacement flags.  If bit 'k' is set that means
   * that we need to replace GENERIC[k] attrib with an automatically
   * computed texture coord.
   */
   raster->sprite_coord_enable = ctx->Point.CoordReplace &
         if (!st->needs_texcoord_semantic &&
      fragProg->info.inputs_read & VARYING_BIT_PNTC) {
   raster->sprite_coord_enable |=
                                    /* ST_NEW_VERTEX_PROGRAM
   */
   raster->point_size_per_vertex = st_point_size_per_vertex(ctx);
   if (!raster->point_size_per_vertex) {
      /* clamp size now */
   raster->point_size = CLAMP(ctx->Point.Size,
                     /* _NEW_LINE | _NEW_MULTISAMPLE
   */
   if (!multisample && ctx->Line.SmoothFlag) {
      raster->line_smooth = 1;
   raster->line_width = CLAMP(ctx->Line.Width,
            }
   else {
      raster->line_width = CLAMP(ctx->Line.Width,
                              /* When the pattern is all 1's, it means line stippling is disabled */
   raster->line_stipple_enable = ctx->Line.StippleFlag && ctx->Line.StipplePattern != 0xffff;
   raster->line_stipple_pattern = ctx->Line.StipplePattern;
   /* GL stipple factor is in [1,256], remap to [0, 255] here */
            /* _NEW_MULTISAMPLE | _NEW_BUFFERS */
   raster->force_persample_interp =
         !st->force_persample_in_shader &&
   raster->multisample &&
   ctx->Multisample.SampleShading &&
            /* _NEW_SCISSOR */
            /* gl_driver_flags::NewFragClamp */
   raster->clamp_fragment_color = !st->clamp_frag_color_in_shader &&
            raster->half_pixel_center = 1;
   if (st->state.fb_orientation == Y_0_TOP)
            /* _NEW_TRANSFORM */
   if (ctx->Transform.ClipOrigin == GL_UPPER_LEFT)
            /* ST_NEW_RASTERIZER */
   raster->rasterizer_discard = ctx->RasterDiscard;
   if (ctx->TileRasterOrderFixed) {
      raster->tile_raster_order_fixed = true;
   raster->tile_raster_order_increasing_x = ctx->TileRasterOrderIncreasingX;
               if (ctx->Array._PolygonModeAlwaysCulls) {
      if (raster->fill_front != PIPE_POLYGON_MODE_FILL)
         if (raster->fill_back != PIPE_POLYGON_MODE_FILL)
               /* _NEW_TRANSFORM */
   raster->depth_clip_near = !ctx->Transform.DepthClampNear;
   raster->depth_clip_far = !ctx->Transform.DepthClampFar;
   raster->depth_clamp = !raster->depth_clip_far;
   /* this should be different for GL vs GLES but without NV_depth_buffer_float
         raster->unclamped_fragment_depth_values = false;
   raster->clip_plane_enable = ctx->Transform.ClipPlanesEnabled;
            /* ST_NEW_RASTERIZER */
   if (ctx->ConservativeRasterization) {
      if (ctx->ConservativeRasterMode == GL_CONSERVATIVE_RASTER_MODE_POST_SNAP_NV)
         else
      } else if (ctx->IntelConservativeRasterization) {
         } else {
                           raster->subpixel_precision_x = ctx->SubpixelPrecisionBias[0];
               }
