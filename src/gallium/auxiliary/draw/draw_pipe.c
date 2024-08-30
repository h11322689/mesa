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
      #include "draw/draw_private.h"
   #include "draw/draw_pipe.h"
   #include "util/u_debug.h"
   #include "util/u_math.h"
         bool
   draw_pipeline_init(struct draw_context *draw)
   {
      /* create pipeline stages */
   draw->pipeline.wide_line  = draw_wide_line_stage(draw);
   draw->pipeline.wide_point = draw_wide_point_stage(draw);
   draw->pipeline.stipple   = draw_stipple_stage(draw);
   draw->pipeline.unfilled  = draw_unfilled_stage(draw);
   draw->pipeline.twoside   = draw_twoside_stage(draw);
   draw->pipeline.offset    = draw_offset_stage(draw);
   draw->pipeline.clip      = draw_clip_stage(draw);
   draw->pipeline.flatshade = draw_flatshade_stage(draw);
   draw->pipeline.cull      = draw_cull_stage(draw);
   draw->pipeline.user_cull = draw_user_cull_stage(draw);
   draw->pipeline.validate  = draw_validate_stage(draw);
            if (!draw->pipeline.wide_line ||
      !draw->pipeline.wide_point ||
   !draw->pipeline.stipple ||
   !draw->pipeline.unfilled ||
   !draw->pipeline.twoside ||
   !draw->pipeline.offset ||
   !draw->pipeline.clip ||
   !draw->pipeline.flatshade ||
   !draw->pipeline.cull ||
   !draw->pipeline.user_cull ||
   !draw->pipeline.validate)
         /* these defaults are oriented toward the needs of softpipe */
   draw->pipeline.wide_point_threshold = 1000000.0f; /* infinity */
   draw->pipeline.wide_line_threshold = 1.0f;
   draw->pipeline.wide_point_sprites = false;
   draw->pipeline.line_stipple = true;
               }
         void
   draw_pipeline_destroy(struct draw_context *draw)
   {
      if (draw->pipeline.wide_line)
         if (draw->pipeline.wide_point)
         if (draw->pipeline.stipple)
         if (draw->pipeline.unfilled)
         if (draw->pipeline.twoside)
         if (draw->pipeline.offset)
         if (draw->pipeline.clip)
         if (draw->pipeline.flatshade)
         if (draw->pipeline.cull)
         if (draw->pipeline.user_cull)
         if (draw->pipeline.validate)
         if (draw->pipeline.aaline)
         if (draw->pipeline.aapoint)
         if (draw->pipeline.pstipple)
         if (draw->pipeline.rasterize)
      }
         /**
   * Build primitive to render a point with vertex at v0.
   */
   static void
   do_point(struct draw_context *draw,
         {
               prim.flags = 0;
   prim.pad = 0;
               }
         /**
   * Build primitive to render a line with vertices at v0, v1.
   * \param flags  bitmask of DRAW_PIPE_EDGE_x, DRAW_PIPE_RESET_STIPPLE
   */
   static void
   do_line(struct draw_context *draw,
         uint16_t flags,
   const char *v0,
   {
               prim.flags = flags;
   prim.pad = 0;
   prim.v[0] = (struct vertex_header *)v0;
               }
         /**
   * Build primitive to render a triangle with vertices at v0, v1, v2.
   * \param flags  bitmask of DRAW_PIPE_EDGE_x, DRAW_PIPE_RESET_STIPPLE
   */
   static void
   do_triangle(struct draw_context *draw,
               uint16_t flags,
   char *v0,
   {
               prim.v[0] = (struct vertex_header *)v0;
   prim.v[1] = (struct vertex_header *)v1;
   prim.v[2] = (struct vertex_header *)v2;
   prim.flags = flags;
               }
         /*
   * Set up macros for draw_pt_decompose.h template code.
   * This code uses vertex indexes / elements.
   */
      #define TRIANGLE(flags,i0,i1,i2)                                 \
      do {                                                          \
      do_triangle(draw,                                          \
               flags,                                         \
            #define LINE(flags,i0,i1)                                         \
      do {                                                           \
      do_line(draw,                                               \
         flags,                                              \
            #define POINT(i0)                              \
      do {                                        \
               #define GET_ELT(idx) (MIN2(elts[idx], max_index))
      #define FUNC pipe_run_elts
   #define FUNC_VARS                              \
      struct draw_context *draw,                  \
   enum mesa_prim prim,                        \
   unsigned prim_flags,                        \
   struct vertex_header *vertices,             \
   unsigned stride,                            \
   const uint16_t *elts,                       \
   unsigned count,                             \
         #include "draw_pt_decompose.h"
            /**
   * Code to run the pipeline on a fairly arbitrary collection of vertices.
   * For drawing indexed primitives.
   *
   * Vertex headers must be pre-initialized with the
   * UNDEFINED_VERTEX_ID, this code will cause that id to become
   * overwritten, so it may have to be reset if there is the intention
   * to reuse the vertices.
   *
   * This code provides a callback to reset the vertex id's which the
   * draw_vbuf.c code uses when it has to perform a flush.
   */
   void
   draw_pipeline_run(struct draw_context *draw,
               {
      draw->pipeline.verts = (char *)vert_info->verts;
   draw->pipeline.vertex_stride = vert_info->stride;
            unsigned i, start;
   for (start = i = 0;
      i < prim_info->primitive_count;
   start += prim_info->primitive_lengths[i], i++) {
      #if DEBUG
         /* Warn if one of the element indexes go outside the vertex buffer */
   {
      unsigned max_index = 0x0;
   /* find the largest element index */
   for (unsigned i = 0; i < count; i++) {
      unsigned int index = prim_info->elts[start + i];
   if (index > max_index)
      }
   if (max_index >= vert_info->count) {
      debug_printf("%s: max_index (%u) outside vertex buffer (%u)\n",
                     #endif
            pipe_run_elts(draw,
               prim_info->prim,
   prim_info->flags,
   vert_info->verts,
   vert_info->stride,
               draw->pipeline.verts = NULL;
      }
         /*
   * Set up macros for draw_pt_decompose.h template code.
   * This code is for non-indexed (aka linear) rendering (no elts).
   */
      #define TRIANGLE(flags,i0,i1,i2)       \
      do_triangle(draw, flags,            \
                     #define LINE(flags,i0,i1)              \
      do_line(draw, flags,                \
               #define POINT(i0)                      \
               #define GET_ELT(idx) (idx)
      #define FUNC pipe_run_linear
   #define FUNC_VARS                     \
      struct draw_context *draw,         \
   enum mesa_prim prim,          \
   unsigned prim_flags,               \
   struct vertex_header *vertices,    \
   unsigned stride,                   \
         #include "draw_pt_decompose.h"
         /*
   * For drawing non-indexed primitives.
   */
   void
   draw_pipeline_run_linear(struct draw_context *draw,
               {
      for (unsigned start = 0, i = 0;
      i < prim_info->primitive_count;
   start += prim_info->primitive_lengths[i], i++) {
   unsigned count = prim_info->primitive_lengths[i];
   char *verts = ((char*)vert_info->verts) +
            draw->pipeline.verts = verts;
   draw->pipeline.vertex_stride = vert_info->stride;
                     pipe_run_linear(draw,
                  prim_info->prim,
   prim_info->flags,
            draw->pipeline.verts = NULL;
      }
         void
   draw_pipeline_flush(struct draw_context *draw,
         {
      draw->pipeline.first->flush(draw->pipeline.first, flags);
   if (flags & DRAW_FLUSH_STATE_CHANGE)
      }
