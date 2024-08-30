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
      #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_prim_assembler.h"
   #include "draw/draw_pt.h"
   #include "draw/draw_vs.h"
   #include "draw/draw_gs.h"
         struct fetch_pipeline_middle_end {
      struct draw_pt_middle_end base;
            struct pt_emit *emit;
   struct pt_so_emit *so_emit;
   struct pt_fetch *fetch;
            unsigned vertex_data_offset;
   unsigned vertex_size;
   unsigned input_prim;
      };
         /** cast wrapper */
   static inline struct fetch_pipeline_middle_end *
   fetch_pipeline_middle_end(struct draw_pt_middle_end *middle)
   {
         }
         /**
   * Prepare/validate middle part of the vertex pipeline.
   * NOTE: if you change this function, also look at the LLVM
   * function llvm_middle_end_prepare() for similar changes.
   */
   static void
   fetch_pipeline_prepare(struct draw_pt_middle_end *middle,
                     {
      struct fetch_pipeline_middle_end *fpme = fetch_pipeline_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *vs = draw->vs.vertex_shader;
   struct draw_geometry_shader *gs = draw->gs.geometry_shader;
   unsigned instance_id_index = ~0;
   const unsigned gs_out_prim = (gs ? gs->output_primitive :
         unsigned nr_vs_outputs = draw_total_vs_outputs(draw);
   unsigned nr = MAX2(vs->info.num_inputs, nr_vs_outputs);
   unsigned point_line_clip = draw->rasterizer->fill_front == PIPE_POLYGON_MODE_POINT ||
                        if (gs) {
                  /* Scan for instanceID system value.
   */
   for (unsigned i = 0; i < vs->info.num_inputs; i++) {
      if (vs->info.input_semantic_name[i] == TGSI_SEMANTIC_INSTANCEID) {
      instance_id_index = i;
                  fpme->input_prim = prim;
            /* Always leave room for the vertex header whether we need it or
   * not.  It's hard to get rid of it in particular because of the
   * viewport code in draw_pt_post_vs.c.
   */
            draw_pt_fetch_prepare(fpme->fetch,
                     draw_pt_post_vs_prepare(fpme->post_vs,
                           draw->clip_xy,
   draw->clip_z,
   draw->clip_user,
                     if (!(opt & PT_PIPELINE)) {
                  }
   else {
      /* limit max fetches by limiting max_vertices */
               /* No need to prepare the shader.
   */
            /* Make sure that the vertex size didn't change at any point above */
      }
         static void
   fetch_pipeline_bind_parameters(struct draw_pt_middle_end *middle)
   {
      /* No-op since the vertex shader executor and drawing pipeline
   * just grab the constants, viewport, etc. from the draw context state.
      }
         static void
   fetch(struct pt_fetch *fetch,
         const struct draw_fetch_info *fetch_info,
   {
      if (fetch_info->linear) {
      draw_pt_fetch_run_linear(fetch, fetch_info->start,
      }
   else {
            }
         static void
   pipeline(struct fetch_pipeline_middle_end *fpme,
               {
      if (prim_info->linear)
         else
      }
         static void
   emit(struct pt_emit *emit,
      const struct draw_vertex_info *vert_info,
      {
      if (prim_info->linear) {
         }
   else {
            }
         static void
   draw_vertex_shader_run(struct draw_vertex_shader *vshader,
                           {
      output_verts->vertex_size = input_verts->vertex_size;
   output_verts->stride = input_verts->vertex_size;
   output_verts->count = input_verts->count;
   output_verts->verts =
      (struct vertex_header *)MALLOC(output_verts->vertex_size *
               vshader->run_linear(vshader,
                     (const float (*)[4])input_verts->verts->data,
   (      float (*)[4])output_verts->verts->data,
   constants,
      }
         static void
   fetch_pipeline_generic(struct draw_pt_middle_end *middle,
               {
      struct fetch_pipeline_middle_end *fpme = fetch_pipeline_middle_end(middle);
   struct draw_context *draw = fpme->draw;
   struct draw_vertex_shader *vshader = draw->vs.vertex_shader;
   struct draw_geometry_shader *gshader = draw->gs.geometry_shader;
   struct draw_prim_info gs_prim_info[TGSI_MAX_VERTEX_STREAMS];
   struct draw_vertex_info fetched_vert_info;
   struct draw_vertex_info vs_vert_info;
   struct draw_vertex_info gs_vert_info[TGSI_MAX_VERTEX_STREAMS];
   struct draw_vertex_info *vert_info;
   struct draw_prim_info ia_prim_info;
   struct draw_vertex_info ia_vert_info;
   const struct draw_prim_info *prim_info = in_prim_info;
   bool free_prim_info = false;
   unsigned opt = fpme->opt;
            fetched_vert_info.count = fetch_info->count;
   fetched_vert_info.vertex_size = fpme->vertex_size;
   fetched_vert_info.stride = fpme->vertex_size;
   fetched_vert_info.verts =
      (struct vertex_header *)MALLOC(fpme->vertex_size *
            if (!fetched_vert_info.verts) {
      assert(0);
      }
   if (draw->collect_statistics) {
      draw->statistics.ia_vertices += prim_info->count;
   draw->statistics.ia_primitives +=
                     /* Fetch into our vertex buffer.
   */
                     /* Run the shader, note that this overwrites the data[] parts of
   * the pipeline verts.
   * Need fetch info to get vertex id correct.
   */
   if (fpme->opt & PT_SHADE) {
      draw_vertex_shader_run(vshader,
                              FREE(vert_info->verts);
               /* Finished with fetch:
   */
            if ((fpme->opt & PT_SHADE) && gshader) {
      draw_geometry_shader_run(gshader,
                           draw->pt.user.constants[PIPE_SHADER_GEOMETRY],
            FREE(vert_info->verts);
   vert_info = &gs_vert_info[0];
   prim_info = &gs_prim_info[0];
            /*
   * pt emit can only handle ushort number of vertices (see
   * render->allocate_vertices).
   * vsplit guarantees there's never more than 4096, however GS can
   * easily blow this up (by a factor of 256 (or even 1024) max).
   */
   if (vert_info->count > 65535) {
            } else {
      if (draw_prim_assembler_is_required(draw, prim_info, vert_info)) {
                     if (ia_vert_info.count) {
      FREE(vert_info->verts);
   vert_info = &ia_vert_info;
   prim_info = &ia_prim_info;
            }
   if (prim_info->count == 0) {
               FREE(vert_info->verts);
   if (free_prim_info) {
         }
                  /* Stream output needs to be done before clipping.
   *
   * XXX: Stream output surely needs to respect the prim_info->elt
   *      lists.
   */
                     /*
   * if there's no position, need to stop now, or the latter stages
   * will try to access non-existent position output.
   */
   if (draw_current_shader_position_output(draw) != -1) {
      if (draw_pt_post_vs_run(fpme->post_vs, vert_info, prim_info)) {
                  /* Do we need to run the pipeline?
   */
   if (opt & PT_PIPELINE) {
         }
   else {
            }
   FREE(vert_info->verts);
   if (free_prim_info) {
            }
         static inline unsigned
   prim_type(unsigned prim, unsigned flags)
   {
      if (flags & DRAW_LINE_LOOP_AS_STRIP)
         else
      }
         static void
   fetch_pipeline_run(struct draw_pt_middle_end *middle,
                     const unsigned *fetch_elts,
   unsigned fetch_count,
   {
      struct fetch_pipeline_middle_end *fpme = fetch_pipeline_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = false;
   fetch_info.start = 0;
   fetch_info.elts = fetch_elts;
            prim_info.linear = false;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
               }
         static void
   fetch_pipeline_linear_run(struct draw_pt_middle_end *middle,
                     {
      struct fetch_pipeline_middle_end *fpme = fetch_pipeline_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = true;
   fetch_info.start = start;
   fetch_info.count = count;
            prim_info.linear = true;
   prim_info.start = 0;
   prim_info.count = count;
   prim_info.elts = NULL;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
               }
         static bool
   fetch_pipeline_linear_run_elts(struct draw_pt_middle_end *middle,
                                 {
      struct fetch_pipeline_middle_end *fpme = fetch_pipeline_middle_end(middle);
   struct draw_fetch_info fetch_info;
            fetch_info.linear = true;
   fetch_info.start = start;
   fetch_info.count = count;
            prim_info.linear = false;
   prim_info.start = 0;
   prim_info.count = draw_count;
   prim_info.elts = draw_elts;
   prim_info.prim = prim_type(fpme->input_prim, prim_flags);
   prim_info.flags = prim_flags;
   prim_info.primitive_count = 1;
                        }
         static void
   fetch_pipeline_finish(struct draw_pt_middle_end *middle)
   {
         }
         static void
   fetch_pipeline_destroy(struct draw_pt_middle_end *middle)
   {
               if (fpme->fetch)
            if (fpme->emit)
            if (fpme->so_emit)
            if (fpme->post_vs)
               }
         struct draw_pt_middle_end *
   draw_pt_fetch_pipeline_or_emit(struct draw_context *draw)
   {
      struct fetch_pipeline_middle_end *fpme =
         if (!fpme)
            fpme->base.prepare        = fetch_pipeline_prepare;
   fpme->base.bind_parameters  = fetch_pipeline_bind_parameters;
   fpme->base.run            = fetch_pipeline_run;
   fpme->base.run_linear     = fetch_pipeline_linear_run;
   fpme->base.run_linear_elts = fetch_pipeline_linear_run_elts;
   fpme->base.finish         = fetch_pipeline_finish;
                     fpme->fetch = draw_pt_fetch_create(draw);
   if (!fpme->fetch)
            fpme->post_vs = draw_pt_post_vs_create(draw);
   if (!fpme->post_vs)
            fpme->emit = draw_pt_emit_create(draw);
   if (!fpme->emit)
            fpme->so_emit = draw_pt_so_emit_create(draw);
   if (!fpme->so_emit)
                  fail:
      if (fpme)
               }
