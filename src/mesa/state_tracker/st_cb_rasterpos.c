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
   * glRasterPos implementation.  Basically render a GL_POINT with our
   * private draw module.  Plug in a special "rasterpos" stage at the end
   * of the 'draw' pipeline to capture the results and update the current
   * raster pos attributes.
   *
   * Authors:
   *   Brian Paul
   */
            #include "main/macros.h"
   #include "main/arrayobj.h"
   #include "main/feedback.h"
   #include "main/framebuffer.h"
   #include "main/rastpos.h"
   #include "main/state.h"
   #include "main/varray.h"
      #include "util/u_memory.h"
      #include "st_context.h"
   #include "st_atom.h"
   #include "st_draw.h"
   #include "st_program.h"
   #include "st_cb_rasterpos.h"
   #include "st_util.h"
   #include "draw/draw_context.h"
   #include "draw/draw_pipe.h"
   #include "vbo/vbo.h"
         /**
   * Our special drawing pipeline stage (replaces rasterization).
   */
   struct rastpos_stage
   {
      struct draw_stage stage;   /**< Base class */
            /* vertex attrib info we can setup once and re-use */
   struct gl_vertex_array_object *VAO;
   struct pipe_draw_info info;
      };
         static inline struct rastpos_stage *
   rastpos_stage( struct draw_stage *stage )
   {
         }
      static void
   rastpos_flush( struct draw_stage *stage, unsigned flags )
   {
         }
      static void
   rastpos_reset_stipple_counter( struct draw_stage *stage )
   {
         }
      static void
   rastpos_tri( struct draw_stage *stage, struct prim_header *prim )
   {
      /* should never get here */
      }
      static void
   rastpos_line( struct draw_stage *stage, struct prim_header *prim )
   {
      /* should never get here */
      }
      static void
   rastpos_destroy(struct draw_stage *stage)
   {
      struct rastpos_stage *rstage = (struct rastpos_stage*)stage;
   _mesa_reference_vao(rstage->ctx, &rstage->VAO, NULL);
      }
         /**
   * Update a raster pos attribute from the vertex result if it's present,
   * else copy the current attrib.
   */
   static void
   update_attrib(struct gl_context *ctx, const uint8_t *outputMapping,
               const struct vertex_header *vert,
   {
      const GLfloat *src;
   const uint8_t k = outputMapping[result];
   if (k != 0xff)
         else
            }
         /**
   * Normally, this function would render a GL_POINT.
   */
   static void
   rastpos_point(struct draw_stage *stage, struct prim_header *prim)
   {
      struct rastpos_stage *rs = rastpos_stage(stage);
   struct gl_context *ctx = rs->ctx;
   const GLfloat height = (GLfloat) ctx->DrawBuffer->Height;
   struct gl_vertex_program *stvp =
         const uint8_t *outputMapping = stvp->result_to_output;
   const GLfloat *pos;
                     /* if we get here, we didn't get clipped */
            /* update raster pos */
   pos = prim->v[0]->data[0];
   ctx->Current.RasterPos[0] = pos[0];
   if (_mesa_fb_orientation(ctx->DrawBuffer) == Y_0_TOP)
         else
         ctx->Current.RasterPos[2] = pos[2];
            /* update other raster attribs */
   update_attrib(ctx, outputMapping, prim->v[0],
                  update_attrib(ctx, outputMapping, prim->v[0],
                  for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      update_attrib(ctx, outputMapping, prim->v[0],
                     if (ctx->RenderMode == GL_SELECT) {
            }
         /**
   * Create rasterpos "drawing" stage.
   */
   static struct rastpos_stage *
   new_draw_rastpos_stage(struct gl_context *ctx, struct draw_context *draw)
   {
               rs->stage.draw = draw;
   rs->stage.next = NULL;
   rs->stage.point = rastpos_point;
   rs->stage.line = rastpos_line;
   rs->stage.tri = rastpos_tri;
   rs->stage.flush = rastpos_flush;
   rs->stage.destroy = rastpos_destroy;
   rs->stage.reset_stipple_counter = rastpos_reset_stipple_counter;
   rs->stage.destroy = rastpos_destroy;
            rs->VAO = _mesa_new_vao(ctx, ~((GLuint)0));
   _mesa_vertex_attrib_binding(ctx, rs->VAO, VERT_ATTRIB_POS, 0);
   _mesa_update_array_format(ctx, rs->VAO, VERT_ATTRIB_POS, 4, GL_FLOAT,
                  rs->info.mode = MESA_PRIM_POINTS;
   rs->info.instance_count = 1;
               }
         void
   st_RasterPos(struct gl_context *ctx, const GLfloat v[4])
   {
      struct st_context *st = st_context(ctx);
   struct draw_context *draw = st_get_draw_context(st);
            if (!st->draw)
            if (ctx->VertexProgram._Current == NULL ||
      ctx->VertexProgram._Current == ctx->VertexProgram._TnlProgram) {
   /* No vertex shader/program is enabled, used the simple/fast fixed-
   * function implementation of RasterPos.
   */
   _mesa_RasterPos(ctx, v);
               if (st->rastpos_stage) {
      /* get rastpos stage info */
      }
   else {
      /* create rastpos draw stage */
   rs = new_draw_rastpos_stage(ctx, draw);
               /* plug our rastpos stage into the draw module */
            /* make sure everything's up to date */
            /* This will get set only if rastpos_point(), above, gets called */
   ctx->PopAttribState |= GL_CURRENT_BIT;
            /* All vertex attribs but position were previously initialized above.
   * Just plug in position pointer now.
   */
   rs->VAO->VertexAttrib[VERT_ATTRIB_POS].Ptr = (GLubyte *) v;
            /* Non-dynamic VAOs merge vertex buffers, which changes vertex elements. */
   if (!rs->VAO->IsDynamic) {
                  /* Save the Draw VAO before we override it. */
   struct gl_vertex_array_object *old_vao;
            _mesa_save_and_set_draw_vao(ctx, rs->VAO, VERT_BIT_POS,
         _mesa_set_varying_vp_inputs(ctx, VERT_BIT_POS &
                              /* restore draw's rasterization stage depending on rendermode */
   if (ctx->RenderMode == GL_FEEDBACK) {
         }
   else if (ctx->RenderMode == GL_SELECT) {
            }
