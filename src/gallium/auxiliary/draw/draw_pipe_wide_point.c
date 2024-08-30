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
      /* Authors:  Keith Whitwell <keithw@vmware.com>
   */
      /**
   * Notes on wide points and sprite mode:
   *
   * In wide point/sprite mode we effectively need to convert each incoming
   * vertex into four outgoing vertices specifying the corners of a quad.
   * Since we don't (yet) have geometry shaders, we have to handle this here
   * in the draw module.
   *
   * For sprites, it also means that this is where we have to handle texcoords
   * for the vertices of the quad.  OpenGL's GL_COORD_REPLACE state specifies
   * if/how enabled texcoords are automatically generated for sprites.  We pass
   * that info through gallium in the pipe_rasterizer_state::sprite_coord_mode
   * array.
   *
   * Additionally, GLSL's gl_PointCoord fragment attribute has to be handled
   * here as well.  This is basically an additional texture/generic attribute
   * that varies .x from 0 to 1 horizontally across the point and varies .y
   * vertically from 0 to 1 down the sprite.
   *
   * With geometry shaders, the gallium frontends could create a GS to do
   * most/all of this.
   */
         #include "pipe/p_screen.h"
   #include "pipe/p_context.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "pipe/p_defines.h"
   #include "pipe/p_shader_tokens.h"
   #include "draw_fs.h"
   #include "draw_vs.h"
   #include "draw_pipe.h"
         struct widepoint_stage {
                        float xbias;
            /** for automatic texcoord generation/replacement */
   unsigned num_texcoord_gen;
            /* TGSI_SEMANTIC to which sprite_coord_enable applies */
               };
            static inline struct widepoint_stage *
   widepoint_stage(struct draw_stage *stage)
   {
         }
         /**
   * Set the vertex texcoords for sprite mode.
   * Coords may be left untouched or set to a right-side-up or upside-down
   * orientation.
   */
   static void
   set_texcoords(const struct widepoint_stage *wide,
         {
      const struct draw_context *draw = wide->stage.draw;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
            for (unsigned i = 0; i < wide->num_texcoord_gen; i++) {
      const unsigned slot = wide->texcoord_gen_slot[i];
   v->data[slot][0] = tc[0];
   if (texcoord_mode == PIPE_SPRITE_COORD_LOWER_LEFT)
         else
         v->data[slot][2] = tc[2];
         }
         /* If there are lots of sprite points (and why wouldn't there be?) it
   * would probably be more sensible to change hardware setup to
   * optimize this rather than doing the whole thing in software like
   * this.
   */
   static void
   widepoint_point(struct draw_stage *stage,
         {
      const struct widepoint_stage *wide = widepoint_stage(stage);
   const unsigned pos = draw_current_shader_position_output(stage->draw);
   const bool sprite = (bool) stage->draw->rasterizer->point_quad_rasterization;
   float half_size;
                     /* four dups of original vertex */
   struct vertex_header *v0 = dup_vert(stage, header->v[0], 0);
   struct vertex_header *v1 = dup_vert(stage, header->v[0], 1);
   struct vertex_header *v2 = dup_vert(stage, header->v[0], 2);
            float *pos0 = v0->data[pos];
   float *pos1 = v1->data[pos];
   float *pos2 = v2->data[pos];
            /* point size is either per-vertex or fixed size */
   if (wide->psize_slot >= 0) {
      half_size = header->v[0]->data[wide->psize_slot][0];
      }
   else {
                  left_adj = -half_size + wide->xbias;
   right_adj = half_size + wide->xbias;
   bot_adj = half_size + wide->ybias;
            pos0[0] += left_adj;
            pos1[0] += left_adj;
            pos2[0] += right_adj;
            pos3[0] += right_adj;
            if (sprite) {
      static const float tex00[4] = { 0, 0, 0, 1 };
   static const float tex01[4] = { 0, 1, 0, 1 };
   static const float tex11[4] = { 1, 1, 0, 1 };
   static const float tex10[4] = { 1, 0, 0, 1 };
   set_texcoords(wide, v0, tex00);
   set_texcoords(wide, v1, tex01);
   set_texcoords(wide, v2, tex10);
               tri.det = header->det;  /* only the sign matters */
   tri.v[0] = v0;
   tri.v[1] = v2;
   tri.v[2] = v3;
            tri.v[0] = v0;
   tri.v[1] = v3;
   tri.v[2] = v1;
      }
         static void
   widepoint_first_point(struct draw_stage *stage,
         {
      struct widepoint_stage *wide = widepoint_stage(stage);
   struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = draw->pipe;
   const struct pipe_rasterizer_state *rast = draw->rasterizer;
            wide->half_point_size = 0.5f * rast->point_size;
   wide->xbias = 0.0;
            if (rast->half_pixel_center) {
      wide->xbias = 0.125;
               /* Disable triangle culling, stippling, unfilled mode etc. */
   r = draw_get_rasterizer_no_cull(draw, rast);
   draw->suspend_flushing = true;
   pipe->bind_rasterizer_state(pipe, r);
            /* XXX we won't know the real size if it's computed by the vertex shader! */
   if ((rast->point_size > draw->pipeline.wide_point_threshold) ||
      (rast->point_quad_rasterization && draw->pipeline.point_sprite)) {
      }
   else {
                           if (rast->point_quad_rasterization) {
                                 /* Loop over fragment shader inputs looking for the PCOORD input or inputs
   * for which bit 'k' in sprite_coord_enable is set.
   */
   for (unsigned i = 0; i < fs->info.num_inputs; i++) {
      int slot;
                  if (sn == wide->sprite_coord_semantic) {
      /* Note that sprite_coord_enable is a bitfield of 32 bits. */
   if (si >= 32 || !(rast->sprite_coord_enable & (1 << si)))
      } else if (sn != TGSI_SEMANTIC_PCOORD) {
                  /* OK, this generic attribute needs to be replaced with a
   * sprite coord (see above).
                  /* add this slot to the texcoord-gen list */
                  wide->psize_slot = -1;
   if (rast->point_size_per_vertex) {
      /* find PSIZ vertex output */
                  }
         static void
   widepoint_flush(struct draw_stage *stage, unsigned flags)
   {
      struct draw_context *draw = stage->draw;
            stage->point = widepoint_first_point;
                     /* restore original rasterizer state */
   if (draw->rast_handle) {
      draw->suspend_flushing = true;
   pipe->bind_rasterizer_state(pipe, draw->rast_handle);
         }
         static void
   widepoint_reset_stipple_counter(struct draw_stage *stage)
   {
         }
         static void
   widepoint_destroy(struct draw_stage *stage)
   {
      draw_free_temp_verts(stage);
      }
         struct draw_stage *draw_wide_point_stage(struct draw_context *draw)
   {
      struct widepoint_stage *wide = CALLOC_STRUCT(widepoint_stage);
   if (!wide)
            wide->stage.draw = draw;
   wide->stage.name = "wide-point";
   wide->stage.next = NULL;
   wide->stage.point = widepoint_first_point;
   wide->stage.line = draw_pipe_passthrough_line;
   wide->stage.tri = draw_pipe_passthrough_tri;
   wide->stage.flush = widepoint_flush;
   wide->stage.reset_stipple_counter = widepoint_reset_stipple_counter;
            if (!draw_alloc_temp_verts(&wide->stage, 4))
            wide->sprite_coord_semantic =
      draw->pipe->screen->get_param(draw->pipe->screen, PIPE_CAP_TGSI_TEXCOORD)
   ?
               fail:
      if (wide)
               }
