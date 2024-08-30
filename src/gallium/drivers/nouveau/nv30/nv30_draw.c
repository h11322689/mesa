   /*
   * Copyright 2012 Red Hat Inc.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included in
   * all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   * OTHER DEALINGS IN THE SOFTWARE.
   *
   * Authors: Ben Skeggs
   *
   */
      #include "draw/draw_context.h"
   #include "draw/draw_vertex.h"
   #include "draw/draw_pipe.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_private.h"
      #include "nv_object.xml.h"
   #include "nv30/nv30-40_3d.xml.h"
   #include "nv30/nv30_context.h"
   #include "nv30/nv30_format.h"
   #include "nv30/nv30_winsys.h"
      struct nv30_render {
      struct vbuf_render base;
            struct pipe_transfer *transfer;
   struct pipe_resource *buffer;
   unsigned offset;
                     struct nouveau_heap *vertprog;
   uint32_t vtxprog[16][4];
   uint32_t vtxfmt[16];
   uint32_t vtxptr[16];
      };
      static inline struct nv30_render *
   nv30_render(struct vbuf_render *render)
   {
         }
      static const struct vertex_info *
   nv30_render_get_vertex_info(struct vbuf_render *render)
   {
         }
      static bool
   nv30_render_allocate_vertices(struct vbuf_render *render,
         {
      struct nv30_render *r = nv30_render(render);
                     if (r->offset + r->length >= render->max_vertex_buffer_bytes) {
      pipe_resource_reference(&r->buffer, NULL);
   r->buffer = pipe_buffer_create(&nv30->screen->base.base,
               if (!r->buffer)
                           }
      static void *
   nv30_render_map_vertices(struct vbuf_render *render)
   {
      struct nv30_render *r = nv30_render(render);
   char *map = pipe_buffer_map_range(
         &r->nv30->base.pipe, r->buffer,
   r->offset, r->length,
   PIPE_MAP_WRITE |
   PIPE_MAP_DISCARD_RANGE,
   assert(map);
      }
      static void
   nv30_render_unmap_vertices(struct vbuf_render *render,
         {
      struct nv30_render *r = nv30_render(render);
   pipe_buffer_unmap(&r->nv30->base.pipe, r->transfer);
      }
      static void
   nv30_render_set_primitive(struct vbuf_render *render, enum mesa_prim prim)
   {
                  }
      static void
   nv30_render_draw_elements(struct vbuf_render *render,
         {
      struct nv30_render *r = nv30_render(render);
   struct nv30_context *nv30 = r->nv30;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
            BEGIN_NV04(push, NV30_3D(VTXBUF(0)), r->vertex_info.num_attribs);
   for (i = 0; i < r->vertex_info.num_attribs; i++) {
      PUSH_RESRC(push, NV30_3D(VTXBUF(i)), BUFCTX_VTXTMP,
                     if (!nv30_state_validate(nv30, ~0, false))
            BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
            if (count & 1) {
      BEGIN_NV04(push, NV30_3D(VB_ELEMENT_U32), 1);
               count >>= 1;
   while (count) {
      unsigned npush = MIN2(count, NV04_PFIFO_MAX_PACKET_LEN);
            BEGIN_NI04(push, NV30_3D(VB_ELEMENT_U16), npush);
   while (npush--) {
      PUSH_DATA(push, (indices[1] << 16) | indices[0]);
                  BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
      }
      static void
   nv30_render_draw_arrays(struct vbuf_render *render, unsigned start, uint nr)
   {
      struct nv30_render *r = nv30_render(render);
   struct nv30_context *nv30 = r->nv30;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   unsigned fn = nr >> 8, pn = nr & 0xff;
   unsigned ps = fn + (pn ? 1 : 0);
            BEGIN_NV04(push, NV30_3D(VTXBUF(0)), r->vertex_info.num_attribs);
   for (i = 0; i < r->vertex_info.num_attribs; i++) {
      PUSH_RESRC(push, NV30_3D(VTXBUF(i)), BUFCTX_VTXTMP,
                     if (!nv30_state_validate(nv30, ~0, false))
            BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
            BEGIN_NI04(push, NV30_3D(VB_VERTEX_BATCH), ps);
   while (fn--) {
      PUSH_DATA (push, 0xff000000 | start);
               if (pn)
            BEGIN_NV04(push, NV30_3D(VERTEX_BEGIN_END), 1);
   PUSH_DATA (push, NV30_3D_VERTEX_BEGIN_END_STOP);
      }
      static void
   nv30_render_release_vertices(struct vbuf_render *render)
   {
      struct nv30_render *r = nv30_render(render);
      }
      static const struct {
      unsigned emit;
   unsigned vp30;
   unsigned vp40;
      } vroute [] = {
      [TGSI_SEMANTIC_POSITION] = { EMIT_4F, 0, 0, 0x00000000 },
   [TGSI_SEMANTIC_COLOR   ] = { EMIT_4F, 3, 1, 0x00000001 },
   [TGSI_SEMANTIC_BCOLOR  ] = { EMIT_4F, 1, 3, 0x00000004 },
   [TGSI_SEMANTIC_FOG     ] = { EMIT_4F, 5, 5, 0x00000010 },
   [TGSI_SEMANTIC_PSIZE   ] = { EMIT_1F_PSIZE, 6, 6, 0x00000020 },
      };
      static bool
   vroute_add(struct nv30_render *r, uint attrib, uint sem, uint *idx)
   {
      struct nv30_screen *screen = r->nv30->screen;
   struct nv30_fragprog *fp = r->nv30->fragprog.program;
   struct vertex_info *vinfo = &r->vertex_info;
   enum pipe_format format;
   uint emit = EMIT_OMIT;
            if (sem == TGSI_SEMANTIC_GENERIC) {
      uint num_texcoords = (screen->eng3d->oclass < NV40_3D_CLASS) ? 8 : 10;
   for (result = 0; result < num_texcoords; result++) {
      if (fp->texcoord[result] == *idx + 8) {
      sem = TGSI_SEMANTIC_TEXCOORD;
   emit = vroute[sem].emit;
            } else {
                  if (emit == EMIT_OMIT)
            draw_emit_vertex_attr(vinfo, emit, attrib);
            r->vtxfmt[attrib] = nv30_vtxfmt(&screen->base.base, format)->hw;
   r->vtxptr[attrib] = vinfo->size;
            if (screen->eng3d->oclass < NV40_3D_CLASS) {
      r->vtxprog[attrib][0] = 0x001f38d8;
   r->vtxprog[attrib][1] = 0x0080001b | (attrib << 9);
   r->vtxprog[attrib][2] = 0x0836106c;
      } else {
      r->vtxprog[attrib][0] = 0x401f9c6c;
   r->vtxprog[attrib][1] = 0x0040000d | (attrib << 8);
   r->vtxprog[attrib][2] = 0x8106c083;
               if (result < 8)
         else {
      assert(sem == TGSI_SEMANTIC_TEXCOORD);
      }
      }
      static bool
   nv30_render_validate(struct nv30_context *nv30)
   {
      struct nv30_render *r = nv30_render(nv30->draw->render);
   struct nv30_rasterizer_stateobj *rast = nv30->rast;
   struct pipe_screen *pscreen = &nv30->screen->base.base;
   struct nouveau_pushbuf *push = nv30->base.pushbuf;
   struct nouveau_object *eng3d = nv30->screen->eng3d;
   struct nv30_vertprog *vp = nv30->vertprog.program;
   struct vertex_info *vinfo = &r->vertex_info;
   unsigned vp_attribs = 0;
   unsigned vp_results = 0;
   unsigned attrib = 0;
   unsigned pntc;
            if (!r->vertprog) {
      struct nouveau_heap *heap = nv30_screen(pscreen)->vp_exec_heap;
   if (nouveau_heap_alloc(heap, 16, &r->vertprog, &r->vertprog)) {
      while (heap->next && heap->size < 16) {
      struct nouveau_heap **evict = heap->next->priv;
               if (nouveau_heap_alloc(heap, 16, &r->vertprog, &r->vertprog))
                  vinfo->num_attribs = 0;
            /* setup routing for all necessary vp outputs */
   for (i = 0; i < vp->info.num_outputs && attrib < 16; i++) {
      uint semantic = vp->info.output_semantic_name[i];
   uint index = vp->info.output_semantic_index[i];
   if (vroute_add(r, attrib, semantic, &index)) {
      vp_attribs |= (1 << attrib++);
                  /* setup routing for replaced point coords not written by vp */
   if (rast && rast->pipe.point_quad_rasterization)
         else
            while (pntc && attrib < 16) {
      uint index = ffs(pntc) - 1; pntc &= ~(1 << index);
   if (vroute_add(r, attrib, TGSI_SEMANTIC_TEXCOORD, &index)) {
      vp_attribs |= (1 << attrib++);
                  /* modify vertex format for correct stride, and stub out unused ones */
   BEGIN_NV04(push, NV30_3D(VP_UPLOAD_FROM_ID), 1);
   PUSH_DATA (push, r->vertprog->start);
   r->vtxprog[attrib - 1][3] |= 1;
   for (i = 0; i < attrib; i++) {
      BEGIN_NV04(push, NV30_3D(VP_UPLOAD_INST(0)), 4);
   PUSH_DATAp(push, r->vtxprog[i], 4);
      }
   for (; i < 16; i++)
            BEGIN_NV04(push, NV30_3D(VIEWPORT_TRANSLATE_X), 8);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   PUSH_DATAf(push, 1.0);
   BEGIN_NV04(push, NV30_3D(DEPTH_RANGE_NEAR), 2);
   PUSH_DATAf(push, 0.0);
   PUSH_DATAf(push, 1.0);
   BEGIN_NV04(push, NV30_3D(VIEWPORT_HORIZ), 2);
   PUSH_DATA (push, nv30->framebuffer.width << 16);
            BEGIN_NV04(push, NV30_3D(VTXFMT(0)), 16);
            BEGIN_NV04(push, NV30_3D(VP_START_FROM_ID), 1);
   PUSH_DATA (push, r->vertprog->start);
   BEGIN_NV04(push, NV30_3D(ENGINE), 1);
   PUSH_DATA (push, 0x00000103);
   if (eng3d->oclass >= NV40_3D_CLASS) {
      BEGIN_NV04(push, NV40_3D(VP_ATTRIB_EN), 2);
   PUSH_DATA (push, vp_attribs);
               vinfo->size /= 4;
      }
      void
   nv30_render_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info,
               {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct draw_context *draw = nv30->draw;
   struct pipe_transfer *transfer[PIPE_MAX_ATTRIBS] = {NULL};
   struct pipe_transfer *transferi = NULL;
                     if (nv30->draw_dirty & NV30_NEW_VIEWPORT)
         if (nv30->draw_dirty & NV30_NEW_RASTERIZER)
         if (nv30->draw_dirty & NV30_NEW_CLIP)
         if (nv30->draw_dirty & NV30_NEW_ARRAYS) {
      draw_set_vertex_buffers(draw, nv30->num_vtxbufs, 0, nv30->vtxbuf);
      }
   if (nv30->draw_dirty & NV30_NEW_FRAGPROG) {
      struct nv30_fragprog *fp = nv30->fragprog.program;
   if (!fp->draw)
            }
   if (nv30->draw_dirty & NV30_NEW_VERTPROG) {
      struct nv30_vertprog *vp = nv30->vertprog.program;
   if (!vp->draw)
            }
   if (nv30->draw_dirty & NV30_NEW_VERTCONST) {
      if (nv30->vertprog.constbuf) {
      void *map = nv04_resource(nv30->vertprog.constbuf)->data;
   draw_set_mapped_constant_buffer(draw, PIPE_SHADER_VERTEX, 0,
      } else {
                     for (i = 0; i < nv30->num_vtxbufs; i++) {
      const void *map = nv30->vtxbuf[i].is_user_buffer ?
         if (!map) {
      if (nv30->vtxbuf[i].buffer.resource)
      map = pipe_buffer_map(pipe, nv30->vtxbuf[i].buffer.resource,
         }
               if (info->index_size) {
      const void *map = info->has_user_indices ? info->index.user : NULL;
   if (!map)
      map = pipe_buffer_map(pipe, info->index.resource,
            draw_set_indexes(draw,
            } else {
                  draw_vbo(draw, info, drawid_offset, NULL, draw_one, 1, 0);
            if (info->index_size && transferi)
         for (i = 0; i < nv30->num_vtxbufs; i++)
      if (transfer[i])
         nv30->draw_dirty = 0;
      }
      static void
   nv30_render_destroy(struct vbuf_render *render)
   {
               if (r->transfer)
         pipe_resource_reference(&r->buffer, NULL);
   nouveau_heap_free(&r->vertprog);
      }
      static struct vbuf_render *
   nv30_render_create(struct nv30_context *nv30)
   {
      struct nv30_render *r = CALLOC_STRUCT(nv30_render);
   if (!r)
            r->nv30 = nv30;
            r->base.max_indices = 16 * 1024;
            r->base.get_vertex_info = nv30_render_get_vertex_info;
   r->base.allocate_vertices = nv30_render_allocate_vertices;
   r->base.map_vertices = nv30_render_map_vertices;
   r->base.unmap_vertices = nv30_render_unmap_vertices;
   r->base.set_primitive = nv30_render_set_primitive;
   r->base.draw_elements = nv30_render_draw_elements;
   r->base.draw_arrays = nv30_render_draw_arrays;
   r->base.release_vertices = nv30_render_release_vertices;
   r->base.destroy = nv30_render_destroy;
      }
      void
   nv30_draw_init(struct pipe_context *pipe)
   {
      struct nv30_context *nv30 = nv30_context(pipe);
   struct vbuf_render *render;
   struct draw_context *draw;
            draw = draw_create(pipe);
   if (!draw)
            render = nv30_render_create(nv30);
   if (!render) {
      draw_destroy(draw);
               stage = draw_vbuf_stage(draw, render);
   if (!stage) {
      render->destroy(render);
   draw_destroy(draw);
               draw_set_render(draw, render);
   draw_set_rasterize_stage(draw, stage);
   draw_wide_line_threshold(draw, 10000000.f);
   draw_wide_point_threshold(draw, 10000000.f);
   draw_wide_point_sprites(draw, true);
      }
