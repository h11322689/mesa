   /**********************************************************
   * Copyright 2008-2009 VMware, Inc.  All rights reserved.
   *
   * Permission is hereby granted, free of charge, to any person
   * obtaining a copy of this software and associated documentation
   * files (the "Software"), to deal in the Software without
   * restriction, including without limitation the rights to use, copy,
   * modify, merge, publish, distribute, sublicense, and/or sell copies
   * of the Software, and to permit persons to whom the Software is
   * furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be
   * included in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   *
   **********************************************************/
      #include "draw/draw_vbuf.h"
   #include "draw/draw_context.h"
   #include "draw/draw_vertex.h"
      #include "util/u_debug.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "svga_context.h"
   #include "svga_state.h"
   #include "svga_swtnl.h"
      #include "svga_types.h"
   #include "svga_reg.h"
   #include "svga3d_reg.h"
   #include "svga_draw.h"
   #include "svga_shader.h"
   #include "svga_swtnl_private.h"
         static const struct vertex_info *
   svga_vbuf_render_get_vertex_info(struct vbuf_render *render)
   {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
                        }
         static bool
   svga_vbuf_render_allocate_vertices(struct vbuf_render *render,
               {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   struct pipe_screen *screen = svga->pipe.screen;
   size_t size = (size_t)nr_vertices * (size_t)vertex_size;
   bool new_vbuf = false;
            SVGA_STATS_TIME_PUSH(svga_sws(svga),
            if (svga_render->vertex_size != vertex_size)
                  if (svga->swtnl.new_vbuf)
                  if (svga_render->vbuf_size
      < svga_render->vbuf_offset + svga_render->vbuf_used + size)
         if (new_vbuf)
         if (new_ibuf)
            if (!svga_render->vbuf) {
      svga_render->vbuf_size = MAX2(size, svga_render->vbuf_alloc_size);
   svga_render->vbuf = SVGA_TRY_PTR(pipe_buffer_create
                     if (!svga_render->vbuf) {
      svga_retry_enter(svga);
   svga_context_flush(svga, NULL);
   assert(!svga_render->vbuf);
   svga_render->vbuf = pipe_buffer_create(screen,
                     /* The buffer allocation may fail if we run out of memory.
   * The draw module's vbuf code should handle that without crashing.
   */
               svga->swtnl.new_vdecl = true;
      } else {
                           if (svga->swtnl.new_vdecl)
                        }
         static void *
   svga_vbuf_render_map_vertices(struct vbuf_render *render)
   {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
            SVGA_STATS_TIME_PUSH(svga_sws(svga),
            if (svga_render->vbuf) {
      char *ptr = (char*)pipe_buffer_map(&svga->pipe,
                                       if (ptr) {
      svga_render->vbuf_ptr = ptr;
      }
   else {
      svga_render->vbuf_ptr = NULL;
   svga_render->vbuf_transfer = NULL;
         }
   else {
      /* we probably ran out of memory when allocating the vertex buffer */
               SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         static void
   svga_vbuf_render_unmap_vertices(struct vbuf_render *render,
               {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   unsigned offset, length;
            SVGA_STATS_TIME_PUSH(svga_sws(svga),
            offset = svga_render->vbuf_offset + svga_render->vertex_size * min_index;
            if (0) {
      /* dump vertex data */
   const float *f = (const float *) ((char *) svga_render->vbuf_ptr +
         unsigned i;
   debug_printf("swtnl vertex data:\n");
   for (i = 0; i < length / 4; i += 4) {
                     pipe_buffer_flush_mapped_range(&svga->pipe,
               pipe_buffer_unmap(&svga->pipe, svga_render->vbuf_transfer);
   svga_render->min_index = min_index;
   svga_render->max_index = max_index;
               }
         static void
   svga_vbuf_render_set_primitive(struct vbuf_render *render,
         {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
      }
         static void
   svga_vbuf_submit_state(struct svga_vbuf_render *svga_render)
   {
      struct svga_context *svga = svga_render->svga;
   SVGA3dVertexDecl vdecl[PIPE_MAX_ATTRIBS];
   unsigned i;
   static const unsigned zero[PIPE_MAX_ATTRIBS] = {0};
            /* if the vdecl or vbuf hasn't changed do nothing */
   if (!svga->swtnl.new_vdecl)
            SVGA_STATS_TIME_PUSH(svga_sws(svga),
                     /* flush the hw state */
   SVGA_RETRY_CHECK(svga, svga_hwtnl_flush(svga->hwtnl), retried);
   if (retried) {
      /* if we hit this path we might become synced with hw */
               for (i = 0; i < svga_render->vdecl_count; i++) {
                  svga_hwtnl_vertex_decls(svga->hwtnl,
                              /* Specify the vertex buffer (there's only ever one) */
   {
      struct pipe_vertex_buffer vb;
   vb.is_user_buffer = false;
   vb.buffer.resource = svga_render->vbuf;
   vb.buffer_offset = svga_render->vdecl_offset;
               /* We have already taken care of flatshading, so let the hwtnl
   * module use whatever is most convenient:
   */
   if (svga->state.sw.need_pipeline) {
      svga_hwtnl_set_flatshade(svga->hwtnl, false, false);
      }
   else {
      svga_hwtnl_set_flatshade(svga->hwtnl,
                                    svga->swtnl.new_vdecl = false;
      }
         static void
   svga_vbuf_render_draw_arrays(struct vbuf_render *render,
         {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   unsigned bias = (svga_render->vbuf_offset - svga_render->vdecl_offset)
         /* instancing will already have been resolved at this point by 'draw' */
   const unsigned start_instance = 0;
   const unsigned instance_count = 1;
                     /* off to hardware */
            /* Need to call update_state() again as the draw module may have
   * altered some of our state behind our backs.  Testcase:
   * redbook/polys.c
   */
   svga_update_state_retry(svga, SVGA_STATE_HW_DRAW);
   SVGA_RETRY_CHECK(svga, svga_hwtnl_draw_arrays
               if (retried) {
                     }
         static void
   svga_vbuf_render_draw_elements(struct vbuf_render *render,
               {
      struct svga_vbuf_render *svga_render = svga_vbuf_render(render);
   struct svga_context *svga = svga_render->svga;
   int bias = (svga_render->vbuf_offset - svga_render->vdecl_offset)
         bool retried;
   /* instancing will already have been resolved at this point by 'draw' */
   const struct pipe_draw_info info = {
      .index_size = 2,
   .mode = svga_render->prim,
   .has_user_indices = 1,
   .index.user = indices,
   .start_instance = 0,
   .instance_count = 1,
   .index_bounds_valid = true,
   .min_index = svga_render->min_index,
      };
   const struct pipe_draw_start_count_bias draw = {
      .start = 0,
   .count = nr_indices,
               assert((svga_render->vbuf_offset - svga_render->vdecl_offset)
                     /* off to hardware */
            /* Need to call update_state() again as the draw module may have
   * altered some of our state behind our backs.  Testcase:
   * redbook/polys.c
   */
   svga_update_state_retry(svga, SVGA_STATE_HW_DRAW);
   SVGA_RETRY_CHECK(svga, svga_hwtnl_draw_range_elements(svga->hwtnl, &info,
               if (retried) {
                     }
         static void
   svga_vbuf_render_release_vertices(struct vbuf_render *render)
   {
      }
         static void
   svga_vbuf_render_destroy(struct vbuf_render *render)
   {
               pipe_resource_reference(&svga_render->vbuf, NULL);
   pipe_resource_reference(&svga_render->ibuf, NULL);
      }
         /**
   * Create a new primitive render.
   */
   struct vbuf_render *
   svga_vbuf_render_create(struct svga_context *svga)
   {
               svga_render->svga = svga;
   svga_render->ibuf_size = 0;
   svga_render->vbuf_size = 0;
   svga_render->ibuf_alloc_size = 4*1024;
   svga_render->vbuf_alloc_size = 64*1024;
   svga_render->layout_id = SVGA3D_INVALID_ID;
   svga_render->base.max_vertex_buffer_bytes = 64*1024/10;
   svga_render->base.max_indices = 65536;
   svga_render->base.get_vertex_info = svga_vbuf_render_get_vertex_info;
   svga_render->base.allocate_vertices = svga_vbuf_render_allocate_vertices;
   svga_render->base.map_vertices = svga_vbuf_render_map_vertices;
   svga_render->base.unmap_vertices = svga_vbuf_render_unmap_vertices;
   svga_render->base.set_primitive = svga_vbuf_render_set_primitive;
   svga_render->base.draw_elements = svga_vbuf_render_draw_elements;
   svga_render->base.draw_arrays = svga_vbuf_render_draw_arrays;
   svga_render->base.release_vertices = svga_vbuf_render_release_vertices;
               }
