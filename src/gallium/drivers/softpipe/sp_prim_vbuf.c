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
   * Interface between 'draw' module's output and the softpipe rasterizer/setup
   * code.  When the 'draw' module has finished filling a vertex buffer, the
   * draw_arrays() functions below will be called.  Loop over the vertices and
   * call the point/line/tri setup functions.
   *
   * Authors
   *  Brian Paul
   */
         #include "sp_context.h"
   #include "sp_setup.h"
   #include "sp_state.h"
   #include "sp_prim_vbuf.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
         #define SP_MAX_VBUF_INDEXES 1024
   #define SP_MAX_VBUF_SIZE    4096
      typedef const float (*cptrf4)[4];
      /**
   * Subclass of vbuf_render.
   */
   struct softpipe_vbuf_render
   {
      struct vbuf_render base;
   struct softpipe_context *softpipe;
            enum mesa_prim prim;
   uint vertex_size;
   uint nr_vertices;
   uint vertex_buffer_size;
      };
         /** cast wrapper */
   static struct softpipe_vbuf_render *
   softpipe_vbuf_render(struct vbuf_render *vbr)
   {
         }
         /** This tells the draw module about our desired vertex layout */
   static const struct vertex_info *
   sp_vbuf_get_vertex_info(struct vbuf_render *vbr)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
      }
         static bool
   sp_vbuf_allocate_vertices(struct vbuf_render *vbr,
         {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
            if (cvbr->vertex_buffer_size < size) {
      align_free(cvbr->vertex_buffer);
   cvbr->vertex_buffer = align_malloc(size, 16);
               cvbr->vertex_size = vertex_size;
   cvbr->nr_vertices = nr_vertices;
         }
         static void
   sp_vbuf_release_vertices(struct vbuf_render *vbr)
   {
         }
         static void *
   sp_vbuf_map_vertices(struct vbuf_render *vbr)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
      }
         static void 
   sp_vbuf_unmap_vertices(struct vbuf_render *vbr, 
               {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   assert( cvbr->vertex_buffer_size >= (max_index+1) * cvbr->vertex_size );
   (void) cvbr;
      }
         static void
   sp_vbuf_set_primitive(struct vbuf_render *vbr, enum mesa_prim prim)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct setup_context *setup_ctx = cvbr->setup;
               cvbr->softpipe->reduced_prim = u_reduced_prim(prim);
      }
         static inline cptrf4 get_vert( const void *vertex_buffer,
               {
         }
         /**
   * draw elements / indexed primitives
   */
   static void
   sp_vbuf_draw_elements(struct vbuf_render *vbr, const uint16_t *indices, uint nr)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct softpipe_context *softpipe = cvbr->softpipe;
   const unsigned stride = softpipe->vertex_info.size * sizeof(float);
   const void *vertex_buffer = cvbr->vertex_buffer;
   struct setup_context *setup = cvbr->setup;
   const bool flatshade_first = softpipe->rasterizer->flatshade_first;
            switch (cvbr->prim) {
   case MESA_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
      sp_setup_point( setup,
      }
         case MESA_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
      sp_setup_line( setup,
            }
   if (nr) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_TRIANGLES:
      for (i = 2; i < nr; i += 3) {
      sp_setup_tri( setup,
                  }
         case MESA_PRIM_TRIANGLE_STRIP:
      if (flatshade_first) {
      for (i = 2; i < nr; i += 1) {
      /* emit first triangle vertex as first triangle vertex */
   sp_setup_tri( setup,
                        }
   else {
      for (i = 2; i < nr; i += 1) {
      /* emit last triangle vertex as last triangle vertex */
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_TRIANGLE_FAN:
      if (flatshade_first) {
      for (i = 2; i < nr; i += 1) {
      /* emit first non-spoke vertex as first vertex */
   sp_setup_tri( setup,
                     }
   else {
      for (i = 2; i < nr; i += 1) {
      /* emit last non-spoke vertex as last vertex */
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_QUADS:
      /* GL quads don't follow provoking vertex convention */
   if (flatshade_first) { 
      /* emit last quad vertex as first triangle vertex */
   for (i = 3; i < nr; i += 4) {
      sp_setup_tri( setup,
                        sp_setup_tri( setup,
                     }
   else {
      /* emit last quad vertex as last triangle vertex */
   for (i = 3; i < nr; i += 4) {
      sp_setup_tri( setup,
                        sp_setup_tri( setup,
                     }
         case MESA_PRIM_QUAD_STRIP:
      /* GL quad strips don't follow provoking vertex convention */
   if (flatshade_first) { 
      /* emit last quad vertex as first triangle vertex */
   for (i = 3; i < nr; i += 2) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, indices[i-0], stride),
   sp_setup_tri( setup,
                     }
   else {
      /* emit last quad vertex as last triangle vertex */
   for (i = 3; i < nr; i += 2) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, indices[i-3], stride),
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_POLYGON:
      /* Almost same as tri fan but the _first_ vertex specifies the flat
   * shading color.
   */
   if (flatshade_first) { 
      /* emit first polygon  vertex as first triangle vertex */
   for (i = 2; i < nr; i += 1) {
      sp_setup_tri( setup,
                     }
   else {
      /* emit first polygon  vertex as last triangle vertex */
   for (i = 2; i < nr; i += 1) {
      sp_setup_tri( setup,
                     }
         default:
            }
         /**
   * This function is hit when the draw module is working in pass-through mode.
   * It's up to us to convert the vertex array into point/line/tri prims.
   */
   static void
   sp_vbuf_draw_arrays(struct vbuf_render *vbr, uint start, uint nr)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct softpipe_context *softpipe = cvbr->softpipe;
   struct setup_context *setup = cvbr->setup;
   const unsigned stride = softpipe->vertex_info.size * sizeof(float);
   const void *vertex_buffer =
         const bool flatshade_first = softpipe->rasterizer->flatshade_first;
            switch (cvbr->prim) {
   case MESA_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
      sp_setup_point( setup,
      }
         case MESA_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINES_ADJACENCY:
      for (i = 3; i < nr; i += 4) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINE_STRIP_ADJACENCY:
      for (i = 3; i < nr; i++) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
      sp_setup_line( setup,
            }
   if (nr) {
      sp_setup_line( setup,
            }
         case MESA_PRIM_TRIANGLES:
      for (i = 2; i < nr; i += 3) {
      sp_setup_tri( setup,
                  }
         case MESA_PRIM_TRIANGLES_ADJACENCY:
      for (i = 5; i < nr; i += 6) {
      sp_setup_tri( setup,
                  }
         case MESA_PRIM_TRIANGLE_STRIP:
      if (flatshade_first) {
      for (i = 2; i < nr; i++) {
      /* emit first triangle vertex as first triangle vertex */
   sp_setup_tri( setup,
                     }
   else {
      for (i = 2; i < nr; i++) {
      /* emit last triangle vertex as last triangle vertex */
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_TRIANGLE_STRIP_ADJACENCY:
      if (flatshade_first) {
      for (i = 5; i < nr; i += 2) {
      /* emit first triangle vertex as first triangle vertex */
   sp_setup_tri( setup,
                     }
   else {
      for (i = 5; i < nr; i += 2) {
      /* emit last triangle vertex as last triangle vertex */
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_TRIANGLE_FAN:
      if (flatshade_first) {
      for (i = 2; i < nr; i += 1) {
      /* emit first non-spoke vertex as first vertex */
   sp_setup_tri( setup,
                     }
   else {
      for (i = 2; i < nr; i += 1) {
      /* emit last non-spoke vertex as last vertex */
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_QUADS:
      /* GL quads don't follow provoking vertex convention */
   if (flatshade_first) { 
      /* emit last quad vertex as first triangle vertex */
   for (i = 3; i < nr; i += 4) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, i-0, stride),
   sp_setup_tri( setup,
                     }
   else {
      /* emit last quad vertex as last triangle vertex */
   for (i = 3; i < nr; i += 4) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, i-3, stride),
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_QUAD_STRIP:
      /* GL quad strips don't follow provoking vertex convention */
   if (flatshade_first) { 
      /* emit last quad vertex as first triangle vertex */
   for (i = 3; i < nr; i += 2) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, i-0, stride),
   sp_setup_tri( setup,
                     }
   else {
      /* emit last quad vertex as last triangle vertex */
   for (i = 3; i < nr; i += 2) {
      sp_setup_tri( setup,
               get_vert(vertex_buffer, i-3, stride),
   sp_setup_tri( setup,
                     }
         case MESA_PRIM_POLYGON:
      /* Almost same as tri fan but the _first_ vertex specifies the flat
   * shading color.
   */
   if (flatshade_first) { 
      /* emit first polygon  vertex as first triangle vertex */
   for (i = 2; i < nr; i += 1) {
      sp_setup_tri( setup,
                     }
   else {
      /* emit first polygon  vertex as last triangle vertex */
   for (i = 2; i < nr; i += 1) {
      sp_setup_tri( setup,
                     }
         default:
            }
      /*
   * FIXME: it is unclear if primitives_storage_needed (which is generally
   * the same as pipe query num_primitives_generated) should increase
   * if SO is disabled for d3d10, but for GL we definitely need to
   * increase num_primitives_generated and this is only called for active
   * SO. If it must not increase for d3d10 need to disambiguate the counters
   * in the driver and do some work for getting correct values, if it should
   * increase too should call this from outside streamout code.
   */
   static void
   sp_vbuf_so_info(struct vbuf_render *vbr, uint stream, uint primitives, uint prim_generated)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
            softpipe->so_stats[stream].num_primitives_written += primitives;
      }
      static void
   sp_vbuf_pipeline_statistics(
      struct vbuf_render *vbr, 
      {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
            softpipe->pipeline_statistics.ia_vertices +=
         softpipe->pipeline_statistics.ia_primitives +=
         softpipe->pipeline_statistics.vs_invocations +=
         softpipe->pipeline_statistics.gs_invocations +=
         softpipe->pipeline_statistics.gs_primitives +=
         softpipe->pipeline_statistics.c_invocations +=
      }
         static void
   sp_vbuf_destroy(struct vbuf_render *vbr)
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   if (cvbr->vertex_buffer)
         sp_setup_destroy_context(cvbr->setup);
      }
         /**
   * Create the post-transform vertex handler for the given context.
   */
   struct vbuf_render *
   sp_create_vbuf_backend(struct softpipe_context *sp)
   {
                        cvbr->base.max_indices = SP_MAX_VBUF_INDEXES;
            cvbr->base.get_vertex_info = sp_vbuf_get_vertex_info;
   cvbr->base.allocate_vertices = sp_vbuf_allocate_vertices;
   cvbr->base.map_vertices = sp_vbuf_map_vertices;
   cvbr->base.unmap_vertices = sp_vbuf_unmap_vertices;
   cvbr->base.set_primitive = sp_vbuf_set_primitive;
   cvbr->base.draw_elements = sp_vbuf_draw_elements;
   cvbr->base.draw_arrays = sp_vbuf_draw_arrays;
   cvbr->base.release_vertices = sp_vbuf_release_vertices;
   cvbr->base.set_stream_output_info = sp_vbuf_so_info;
   cvbr->base.pipeline_statistics = sp_vbuf_pipeline_statistics;
                                 }
