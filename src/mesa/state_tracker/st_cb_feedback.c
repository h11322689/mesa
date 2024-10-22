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
      /**
   * GL_SELECT and GL_FEEDBACK render modes.
   * Basically, we use a private instance of the 'draw' module for doing
   * selection/feedback.  It would be nice to use the transform_feedback
   * hardware feature, but it's defined as happening pre-clip and we want
   * post-clipped primitives.  Also, there's concerns about the efficiency
   * of using the hardware for this anyway.
   *
   * Authors:
   *   Brian Paul
   */
         #include "main/context.h"
   #include "main/feedback.h"
   #include "main/framebuffer.h"
   #include "main/varray.h"
      #include "util/u_memory.h"
      #include "vbo/vbo.h"
      #include "st_context.h"
   #include "st_draw.h"
   #include "st_cb_feedback.h"
   #include "st_program.h"
   #include "st_util.h"
      #include "pipe/p_context.h"
   #include "pipe/p_defines.h"
      #include "draw/draw_context.h"
   #include "draw/draw_pipe.h"
         /**
   * This is actually used for both feedback and selection.
   */
   struct feedback_stage
   {
      struct draw_stage stage;   /**< Base class */
   struct gl_context *ctx;            /**< Rendering context */
      };
         /**********************************************************************
   * GL Feedback functions
   **********************************************************************/
      static inline struct feedback_stage *
   feedback_stage( struct draw_stage *stage )
   {
         }
         static void
   feedback_vertex(struct gl_context *ctx, const struct draw_context *draw,
         {
      struct gl_vertex_program *stvp =
         GLfloat win[4];
   const GLfloat *color, *texcoord;
            win[0] = v->data[0][0];
   if (_mesa_fb_orientation(ctx->DrawBuffer) == Y_0_TOP)
         else
         win[2] = v->data[0][2];
            /* XXX
   * When we compute vertex layout, save info about position of the
   * color and texcoord attribs to use here.
            slot = stvp->result_to_output[VARYING_SLOT_COL0];
   if (slot != 0xff)
         else
            slot = stvp->result_to_output[VARYING_SLOT_TEX0];
   if (slot != 0xff)
         else
               }
         static void
   feedback_tri( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   _mesa_feedback_token(fs->ctx, (GLfloat) GL_POLYGON_TOKEN);
   _mesa_feedback_token(fs->ctx, (GLfloat) 3); /* three vertices */
   feedback_vertex(fs->ctx, draw, prim->v[0]);
   feedback_vertex(fs->ctx, draw, prim->v[1]);
      }
         static void
   feedback_line( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   if (fs->reset_stipple_counter) {
      _mesa_feedback_token(fs->ctx, (GLfloat) GL_LINE_RESET_TOKEN);
      }
   else {
         }
   feedback_vertex(fs->ctx, draw, prim->v[0]);
      }
         static void
   feedback_point( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
   struct draw_context *draw = stage->draw;
   _mesa_feedback_token(fs->ctx, (GLfloat) GL_POINT_TOKEN);
      }
         static void
   feedback_flush( struct draw_stage *stage, unsigned flags )
   {
         }
         static void
   feedback_reset_stipple_counter( struct draw_stage *stage )
   {
      struct feedback_stage *fs = feedback_stage(stage);
      }
         static void
   feedback_destroy( struct draw_stage *stage )
   {
         }
      /**
   * Create GL feedback drawing stage.
   */
   static struct draw_stage *
   draw_glfeedback_stage(struct gl_context *ctx, struct draw_context *draw)
   {
               fs->stage.draw = draw;
   fs->stage.next = NULL;
   fs->stage.point = feedback_point;
   fs->stage.line = feedback_line;
   fs->stage.tri = feedback_tri;
   fs->stage.flush = feedback_flush;
   fs->stage.reset_stipple_counter = feedback_reset_stipple_counter;
   fs->stage.destroy = feedback_destroy;
               }
            /**********************************************************************
   * GL Selection functions
   **********************************************************************/
      static void
   select_tri( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
   _mesa_update_hitflag( fs->ctx, prim->v[0]->data[0][2] );
   _mesa_update_hitflag( fs->ctx, prim->v[1]->data[0][2] );
      }
      static void
   select_line( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
   _mesa_update_hitflag( fs->ctx, prim->v[0]->data[0][2] );
      }
         static void
   select_point( struct draw_stage *stage, struct prim_header *prim )
   {
      struct feedback_stage *fs = feedback_stage(stage);
      }
         static void
   select_flush( struct draw_stage *stage, unsigned flags )
   {
         }
         static void
   select_reset_stipple_counter( struct draw_stage *stage )
   {
         }
      static void
   select_destroy( struct draw_stage *stage )
   {
         }
         /**
   * Create GL selection mode drawing stage.
   */
   static struct draw_stage *
   draw_glselect_stage(struct gl_context *ctx, struct draw_context *draw)
   {
               fs->stage.draw = draw;
   fs->stage.next = NULL;
   fs->stage.point = select_point;
   fs->stage.line = select_line;
   fs->stage.tri = select_tri;
   fs->stage.flush = select_flush;
   fs->stage.reset_stipple_counter = select_reset_stipple_counter;
   fs->stage.destroy = select_destroy;
               }
         void
   st_RenderMode(struct gl_context *ctx, GLenum newMode )
   {
      struct st_context *st = st_context(ctx);
            if (!st->draw)
            if (newMode == GL_RENDER) {
      /* restore normal VBO draw function */
      }
   else if (newMode == GL_SELECT) {
      if (ctx->Const.HardwareAcceleratedSelect)
         else {
      if (!st->selection_stage)
         draw_set_rasterize_stage(draw, st->selection_stage);
   /* Plug in new vbo draw function */
   ctx->Driver.DrawGallium = st_feedback_draw_vbo;
         }
   else {
               if (!st->feedback_stage)
         draw_set_rasterize_stage(draw, st->feedback_stage);
   /* Plug in new vbo draw function */
   ctx->Driver.DrawGallium = st_feedback_draw_vbo;
   ctx->Driver.DrawGalliumMultiMode = st_feedback_draw_vbo_multi_mode;
   /* need to generate/use a vertex program that emits pos/color/tex */
   if (vp)
               /* Restore geometry shader states when leaving GL_SELECT mode. */
   if (ctx->RenderMode == GL_SELECT && ctx->Const.HardwareAcceleratedSelect)
      }
