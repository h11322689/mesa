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
   * \file
   * Vertex buffer drawing stage.
   *
   * \author Jose Fonseca <jfonseca@vmware.com>
   * \author Keith Whitwell <keithw@vmware.com>
   */
         #include "util/u_debug.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "draw_vbuf.h"
   #include "draw_private.h"
   #include "draw_vertex.h"
   #include "draw_pipe.h"
   #include "translate/translate.h"
   #include "translate/translate_cache.h"
         /**
   * Vertex buffer emit stage.
   */
   struct vbuf_stage {
                                 /** Vertex size in bytes */
                              /** Vertices in hardware format */
   uint8_t *vertices;
   uint8_t *vertex_ptr;
   unsigned max_vertices;
            /** Indices */
   uint16_t *indices;
   unsigned max_indices;
            /* Cache point size somewhere its address won't change:
   */
   float point_size;
               };
         /**
   * Basically a cast wrapper.
   */
   static inline struct vbuf_stage *
   vbuf_stage(struct draw_stage *stage)
   {
      assert(stage);
      }
         static void vbuf_flush_vertices(struct vbuf_stage *vbuf);
   static void vbuf_alloc_vertices(struct vbuf_stage *vbuf);
         static inline void
   check_space(struct vbuf_stage *vbuf, unsigned nr)
   {
      if (vbuf->nr_vertices + nr > vbuf->max_vertices ||
      vbuf->nr_indices + nr > vbuf->max_indices) {
   vbuf_flush_vertices(vbuf);
         }
         /**
   * Extract the needed fields from post-transformed vertex and emit
   * a hardware(driver) vertex.
   * Recall that the vertices are constructed by the 'draw' module and
   * have a couple of slots at the beginning (1-dword header, 4-dword
   * clip pos) that we ignore here.  We only use the vertex->data[] fields.
   */
   static inline uint16_t
   emit_vertex(struct vbuf_stage *vbuf, struct vertex_header *vertex)
   {
      if (vertex->vertex_id == UNDEFINED_VERTEX_ID && vbuf->vertex_ptr) {
      /* Hmm - vertices are emitted one at a time - better make sure
   * set_buffer is efficient.  Consider a special one-shot mode for
   * translate.
   */
   /* Note: we really do want data[0] here, not data[pos]:
   */
   vbuf->translate->set_buffer(vbuf->translate, 0, vertex->data[0], 0, ~0);
                     vbuf->vertex_ptr += vbuf->vertex_size;
                  }
         static void
   vbuf_tri(struct draw_stage *stage, struct prim_header *prim)
   {
      struct vbuf_stage *vbuf = vbuf_stage(stage);
                     for (i = 0; i < 3; i++) {
            }
         static void
   vbuf_line(struct draw_stage *stage, struct prim_header *prim)
   {
      struct vbuf_stage *vbuf = vbuf_stage(stage);
                     for (i = 0; i < 2; i++) {
            }
         static void
   vbuf_point(struct draw_stage *stage, struct prim_header *prim)
   {
                           }
         /**
   * Set the prim type for subsequent vertices.
   * This may result in a new vertex size.  The existing vbuffer (if any)
   * will be flushed if needed and a new one allocated.
   */
   static void
   vbuf_start_prim(struct vbuf_stage *vbuf, enum mesa_prim prim)
   {
      struct translate_key hw_key;
   unsigned dst_offset;
   unsigned i;
            vbuf->render->set_primitive(vbuf->render, prim);
   if (vbuf->render->set_view_index)
            /* Must do this after set_primitive() above:
   *
   * XXX: need some state managment to track when this needs to be
   * recalculated.  The driver should tell us whether there was a
   * state change.
   */
   vbuf->vinfo = vbuf->render->get_vertex_info(vbuf->render);
   vinfo = vbuf->vinfo;
            /* Translate from pipeline vertices to hw vertices.
   */
            for (i = 0; i < vinfo->num_attribs; i++) {
      unsigned emit_sz = 0;
   unsigned src_buffer = 0;
   enum pipe_format output_format;
            output_format = draw_translate_vinfo_format(vinfo->attrib[i].emit);
            /* doesn't handle EMIT_OMIT */
            if (vinfo->attrib[i].emit == EMIT_1F_PSIZE) {
      src_buffer = 1;
      }
   else if (vinfo->attrib[i].src_index == DRAW_ATTR_NONEXIST) {
      /* elements which don't exist will get assigned zeros */
   src_buffer = 2;
               hw_key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   hw_key.element[i].input_buffer = src_buffer;
   hw_key.element[i].input_offset = src_offset;
   hw_key.element[i].instance_divisor = 0;
   hw_key.element[i].output_format = output_format;
                        hw_key.nr_elements = vinfo->num_attribs;
            /* Don't bother with caching at this stage:
   */
   if (!vbuf->translate ||
      translate_key_compare(&vbuf->translate->key, &hw_key) != 0) {
   translate_key_sanitize(&hw_key);
            vbuf->translate->set_buffer(vbuf->translate, 1, &vbuf->point_size, 0, ~0);
                        /* Allocate new buffer?
   */
   assert(vbuf->vertices == NULL);
      }
         static void
   vbuf_first_tri(struct draw_stage *stage, struct prim_header *prim)
   {
               vbuf_flush_vertices(vbuf);
   vbuf_start_prim(vbuf, MESA_PRIM_TRIANGLES);
   stage->tri = vbuf_tri;
      }
         static void
   vbuf_first_line(struct draw_stage *stage, struct prim_header *prim)
   {
               vbuf_flush_vertices(vbuf);
   vbuf_start_prim(vbuf, MESA_PRIM_LINES);
   stage->line = vbuf_line;
      }
         static void
   vbuf_first_point(struct draw_stage *stage, struct prim_header *prim)
   {
               vbuf_flush_vertices(vbuf);
   vbuf_start_prim(vbuf, MESA_PRIM_POINTS);
   stage->point = vbuf_point;
      }
            /**
   * Flush existing vertex buffer and allocate a new one.
   */
   static void
   vbuf_flush_vertices(struct vbuf_stage *vbuf)
   {
      if (vbuf->vertices) {
               if (vbuf->nr_indices) {
      vbuf->render->draw_elements(vbuf->render,
                              /* Reset temporary vertices ids */
   if (vbuf->nr_vertices)
            /* Free the vertex buffer */
            vbuf->max_vertices = vbuf->nr_vertices = 0;
               /* Reset point/line/tri function pointers.
   * If (for example) we transition from points to tris and back to points
   * again, we need to call the vbuf_first_point() function again to flush
   * the triangles before drawing more points.  This can happen when drawing
   * with front polygon mode = filled and back polygon mode = line or point.
   */
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
      }
         static void
   vbuf_alloc_vertices(struct vbuf_stage *vbuf)
   {
      if (vbuf->vertex_ptr) {
      assert(!vbuf->nr_indices);
               /* Allocate a new vertex buffer */
   vbuf->max_vertices =
            if (vbuf->max_vertices >= UNDEFINED_VERTEX_ID)
            /* Must always succeed -- driver gives us a
   * 'max_vertex_buffer_bytes' which it guarantees it can allocate,
   * and it will flush itself if necessary to do so.  If this does
   * fail, we are basically without usable hardware.
   */
   vbuf->render->allocate_vertices(vbuf->render,
                  vbuf->vertex_ptr = vbuf->vertices =
      }
         static void
   vbuf_flush(struct draw_stage *stage, unsigned flags)
   {
                  }
         static void
   vbuf_reset_stipple_counter(struct draw_stage *stage)
   {
      /* XXX: Need to do something here for hardware with linestipple.
   */
      }
         static void
   vbuf_destroy(struct draw_stage *stage)
   {
               if (vbuf->indices)
            if (vbuf->render)
            if (vbuf->cache)
               }
         /**
   * Create a new primitive vbuf/render stage.
   */
   struct draw_stage *
   draw_vbuf_stage(struct draw_context *draw, struct vbuf_render *render)
   {
      struct vbuf_stage *vbuf = CALLOC_STRUCT(vbuf_stage);
   if (!vbuf)
            vbuf->stage.draw = draw;
   vbuf->stage.name = "vbuf";
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
   vbuf->stage.flush = vbuf_flush;
   vbuf->stage.reset_stipple_counter = vbuf_reset_stipple_counter;
            vbuf->render = render;
            vbuf->indices = (uint16_t *) align_malloc(vbuf->max_indices *
               if (!vbuf->indices)
            vbuf->cache = translate_cache_create();
   if (!vbuf->cache)
                                    fail:
      if (vbuf)
               }
