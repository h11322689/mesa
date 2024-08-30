   /**************************************************************************
   *
   * Copyright 2008 VMware, Inc.
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
      #include "util/u_memory.h"
   #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_pt.h"
   #include "translate/translate.h"
   #include "translate/translate_cache.h"
   #include "util/u_prim.h"
      struct pt_emit {
                        struct translate_cache *cache;
                        };
         void
   draw_pt_emit_prepare(struct pt_emit *emit,
               {
      struct draw_context *draw = emit->draw;
   const struct vertex_info *vinfo;
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            /* XXX: may need to defensively reset this later on as clipping can
   * clobber this state in the render backend.
   */
            draw->render->set_primitive(draw->render, emit->prim);
   if (draw->render->set_view_index)
            /* Must do this after set_primitive() above:
   */
            /* Translate from pipeline vertices to hw vertices.
   */
   unsigned dst_offset = 0;
   for (unsigned i = 0; i < vinfo->num_attribs; i++) {
      unsigned emit_sz = 0;
   unsigned src_buffer = 0;
   unsigned output_format;
            output_format = draw_translate_vinfo_format(vinfo->attrib[i].emit);
            /* doesn't handle EMIT_OMIT */
            if (vinfo->attrib[i].emit == EMIT_1F_PSIZE) {
      src_buffer = 1;
      } else if (vinfo->attrib[i].src_index == DRAW_ATTR_NONEXIST) {
      /* elements which don't exist will get assigned zeros */
   src_buffer = 2;
               hw_key.element[i].type = TRANSLATE_ELEMENT_NORMAL;
   hw_key.element[i].input_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
   hw_key.element[i].input_buffer = src_buffer;
   hw_key.element[i].input_offset = src_offset;
   hw_key.element[i].instance_divisor = 0;
   hw_key.element[i].output_format = output_format;
                        hw_key.nr_elements = vinfo->num_attribs;
            if (!emit->translate ||
      translate_key_compare(&emit->translate->key, &hw_key) != 0) {
   translate_key_sanitize(&hw_key);
                        if (!vinfo->size)
         else
      *max_vertices = (draw->render->max_vertex_buffer_bytes /
   }
         void
   draw_pt_emit(struct pt_emit *emit,
               {
      const float (*vertex_data)[4] = (const float (*)[4])vert_info->verts->data;
   unsigned vertex_count = vert_info->count;
   unsigned stride = vert_info->stride;
   const uint16_t *elts = prim_info->elts;
   struct draw_context *draw = emit->draw;
   struct translate *translate = emit->translate;
   struct vbuf_render *render = draw->render;
   unsigned start, i;
            /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            if (vertex_count == 0)
            /* XXX: and work out some way to coordinate the render primitive
   * between vbuf.c and here...
   */
   render->set_primitive(draw->render, prim_info->prim);
   if (draw->render->set_view_index)
            assert(vertex_count <= 65535);
   render->allocate_vertices(render,
                  hw_verts = render->map_vertices(render);
   if (!hw_verts) {
      debug_warn_once("map of vertex buffer failed (out of memory?)");
               translate->set_buffer(translate,
                              translate->set_buffer(translate,
                              /* fetch/translate vertex attribs to fill hw_verts[] */
   translate->run(translate,
                  0,
   vertex_count,
         if (0) {
      for (unsigned i = 0; i < vertex_count; i++) {
      debug_printf("\n\n%s vertex %d:\n", __func__, i);
   draw_dump_emitted_vertex(emit->vinfo,
                                 for (start = i = 0;
      i < prim_info->primitive_count;
   start += prim_info->primitive_lengths[i], i++) {
   render->draw_elements(render,
                        }
         void
   draw_pt_emit_linear(struct pt_emit *emit,
               {
      const float (*vertex_data)[4] = (const float (*)[4])vert_info->verts->data;
   unsigned stride = vert_info->stride;
   unsigned count = vert_info->count;
   struct draw_context *draw = emit->draw;
   struct translate *translate = emit->translate;
   struct vbuf_render *render = draw->render;
   void *hw_verts;
         #if 0
         #endif
      /* XXX: need to flush to get prim_vbuf.c to release its allocation??
   */
            /* XXX: and work out some way to coordinate the render primitive
   * between vbuf.c and here...
   */
   render->set_primitive(draw->render, prim_info->prim);
   if (draw->render->set_view_index)
            assert(count <= 65535);
   if (!render->allocate_vertices(render,
                        hw_verts = render->map_vertices(render);
   if (!hw_verts)
            translate->set_buffer(translate, 0,
            translate->set_buffer(translate, 1,
                  translate->run(translate,
                  0,
   count,
         if (0) {
      for (unsigned i = 0; i < count; i++) {
      debug_printf("\n\n%s vertex %d:\n", __func__, i);
   draw_dump_emitted_vertex(emit->vinfo,
                                 for (start = i = 0;
      i < prim_info->primitive_count;
   start += prim_info->primitive_lengths[i], i++) {
   render->draw_arrays(render,
                                    fail:
      debug_warn_once("allocate or map of vertex buffer failed (out of memory?)");
      }
         struct pt_emit *
   draw_pt_emit_create(struct draw_context *draw)
   {
      struct pt_emit *emit = CALLOC_STRUCT(pt_emit);
   if (!emit)
            emit->draw = draw;
   emit->cache = translate_cache_create();
   if (!emit->cache) {
      FREE(emit);
                           }
         void
   draw_pt_emit_destroy(struct pt_emit *emit)
   {
      if (emit->cache)
               }
