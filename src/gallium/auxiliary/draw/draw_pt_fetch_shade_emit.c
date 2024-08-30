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
         #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_pt.h"
   #include "draw/draw_vs.h"
         struct fetch_shade_emit;
         /* Prototype fetch, shade, emit-hw-verts all in one go.
   */
   struct fetch_shade_emit {
      struct draw_pt_middle_end base;
            /* Temporaries:
   */
   const float *constants;
   unsigned pitch[PIPE_MAX_ATTRIBS];
   const uint8_t *src[PIPE_MAX_ATTRIBS];
            struct draw_vs_variant_key key;
               };
            static void
   fse_prepare(struct draw_pt_middle_end *middle,
               enum mesa_prim prim,
   {
      struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
   unsigned num_vs_inputs = draw->vs.vertex_shader->info.num_inputs;
   const struct vertex_info *vinfo;
            /* Can't support geometry shader on this path.
   */
                     /* Must do this after set_primitive() above:
   */
            fse->key.output_stride = vinfo->size * 4;
   fse->key.nr_outputs = vinfo->num_attribs;
            fse->key.nr_elements = MAX2(fse->key.nr_outputs,     /* outputs - translate to hw format */
            fse->key.viewport = !draw->bypass_viewport;
   fse->key.clip = draw->clip_xy || draw->clip_z || draw->clip_user;
            memset(fse->key.element, 0,
            for (unsigned i = 0; i < num_vs_inputs; i++) {
      const struct pipe_vertex_element *src = &draw->pt.vertex_element[i];
            /* Consider ignoring these, ie make generated programs
   * independent of this state:
   */
   fse->key.element[i].in.buffer = src->vertex_buffer_index;
   fse->key.element[i].in.offset = src->src_offset;
   if (src->src_stride == 0)
                     if (0) debug_printf("%s: lookup const_vbuffers: %x\n",
            {
               for (unsigned i = 0; i < vinfo->num_attribs; i++) {
                              /* The elements in the key correspond to vertex shader output
   * numbers, not to positions in the hw vertex description --
   * that's handled by the output_offset field.
   */
   fse->key.element[i].out.format = vinfo->attrib[i].emit;
                  dst_offset += emit_sz;
                           if (!fse->active) {
      assert(0);
               if (0) debug_printf("%s: found const_vbuffers: %x\n", __func__,
            /* Now set buffer pointers:
   */
   for (unsigned i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      fse->active->set_buffer(fse->active,
                                       *max_vertices = (draw->render->max_vertex_buffer_bytes /
            /* Probably need to do this somewhere (or fix exec shader not to
   * need it):
   */
   if (1) {
      struct draw_vertex_shader *vs = draw->vs.vertex_shader;
         }
         static void
   fse_bind_parameters(struct draw_pt_middle_end *middle)
   {
         }
         static void
   fse_run_linear(struct draw_pt_middle_end *middle,
                     {
      struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            if (!draw->render->allocate_vertices(draw->render,
                        hw_verts = draw->render->map_vertices(draw->render);
   if (!hw_verts)
            /* Single routine to fetch vertices, run shader and emit HW verts.
   * Clipping is done elsewhere -- either by the API or on hardware,
   * or for some other reason not required...
   */
            if (0) {
      for (unsigned i = 0; i < count; i++) {
      debug_printf("\n\n%s vertex %d: (stride %d, offset %d)\n",
                        draw_dump_emitted_vertex(fse->vinfo,
                                 /* Draw arrays path to avoid re-emitting index list again and
   * again.
   */
                           fail:
      debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
      }
         static void
   fse_run(struct draw_pt_middle_end *middle,
         const unsigned *fetch_elts,
   unsigned fetch_count,
   const uint16_t *draw_elts,
   unsigned draw_count,
   {
      struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            if (!draw->render->allocate_vertices(draw->render,
                        hw_verts = draw->render->map_vertices(draw->render);
   if (!hw_verts)
            /* Single routine to fetch vertices, run shader and emit HW verts.
   */
            if (0) {
      for (unsigned i = 0; i < fetch_count; i++) {
      debug_printf("\n\n%s vertex %d:\n", __func__, i);
   draw_dump_emitted_vertex(fse->vinfo,
                                          draw->render->release_vertices(draw->render);
         fail:
      debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
      }
         static bool
   fse_run_linear_elts(struct draw_pt_middle_end *middle,
                     unsigned start,
   unsigned count,
   {
      struct fetch_shade_emit *fse = (struct fetch_shade_emit *)middle;
   struct draw_context *draw = fse->draw;
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            if (!draw->render->allocate_vertices(draw->render,
                        hw_verts = draw->render->map_vertices(draw->render);
   if (!hw_verts)
            /* Single routine to fetch vertices, run shader and emit HW verts.
   * Clipping is done elsewhere -- either by the API or on hardware,
   * or for some other reason not required...
   */
                                          }
         static void
   fse_finish(struct draw_pt_middle_end *middle)
   {
   }
         static void
   fse_destroy(struct draw_pt_middle_end *middle)
   {
         }
         struct draw_pt_middle_end *
   draw_pt_middle_fse(struct draw_context *draw)
   {
      struct fetch_shade_emit *fse = CALLOC_STRUCT(fetch_shade_emit);
   if (!fse)
            fse->base.prepare = fse_prepare;
   fse->base.bind_parameters = fse_bind_parameters;
   fse->base.run = fse_run;
   fse->base.run_linear = fse_run_linear;
   fse->base.run_linear_elts = fse_run_linear_elts;
   fse->base.finish = fse_finish;
   fse->base.destroy = fse_destroy;
               }
