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
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "util/u_inlines.h"
   #include "pipe/p_state.h"
      #include "svga_context.h"
   #include "svga_screen.h"
   #include "svga_swtnl.h"
   #include "svga_state.h"
   #include "svga_swtnl_private.h"
            enum pipe_error
   svga_swtnl_draw_vbo(struct svga_context *svga,
                     const struct pipe_draw_info *info,
   {
      struct pipe_transfer *vb_transfer[PIPE_MAX_ATTRIBS] = { 0 };
   struct pipe_transfer *ib_transfer = NULL;
   struct pipe_transfer *cb_transfer[SVGA_MAX_CONST_BUFS] = { 0 };
   struct draw_context *draw = svga->swtnl.draw;
   ASSERTED unsigned old_num_vertex_buffers;
   unsigned i;
   const void *map;
                     assert(!svga->dirty);
   assert(svga->state.sw.need_swtnl);
            /* Make sure that the need_swtnl flag does not go away */
            SVGA_RETRY_CHECK(svga, svga_update_state(svga, SVGA_STATE_SWTNL_DRAW), retried);
   if (retried) {
                  /*
   * Map vertex buffers
   */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      if (svga->curr.vb[i].buffer.resource) {
      map = pipe_buffer_map(&svga->pipe,
                                    }
            /* Map index buffer, if present */
   map = NULL;
   if (info->index_size) {
      if (info->has_user_indices) {
         } else {
      map = pipe_buffer_map(&svga->pipe, info->index.resource,
            }
   draw_set_indexes(draw,
                     /* Map constant buffers */
   for (i = 0; i < ARRAY_SIZE(svga->curr.constbufs[PIPE_SHADER_VERTEX]); ++i) {
      if (svga->curr.constbufs[PIPE_SHADER_VERTEX][i].buffer == NULL) {
                  map = pipe_buffer_map(&svga->pipe,
                           assert(map);
   draw_set_mapped_constant_buffer(
      draw, PIPE_SHADER_VERTEX, i,
   map,
            draw_vbo(draw, info, drawid_offset, indirect, draw_one, 1,
                     /* Ensure the draw module didn't touch this */
            /*
   * unmap vertex/index buffers
   */
   for (i = 0; i < svga->curr.num_vertex_buffers; i++) {
      if (svga->curr.vb[i].buffer.resource) {
      pipe_buffer_unmap(&svga->pipe, vb_transfer[i]);
                  if (ib_transfer) {
      pipe_buffer_unmap(&svga->pipe, ib_transfer);
               for (i = 0; i < ARRAY_SIZE(svga->curr.constbufs[PIPE_SHADER_VERTEX]); ++i) {
      if (svga->curr.constbufs[PIPE_SHADER_VERTEX][i].buffer) {
                     /* Now safe to remove the need_swtnl flag in any update_state call */
   svga->state.sw.in_swtnl_draw = false;
            SVGA_STATS_TIME_POP(svga_sws(svga));
      }
         bool
   svga_init_swtnl(struct svga_context *svga)
   {
               svga->swtnl.backend = svga_vbuf_render_create(svga);
   if (!svga->swtnl.backend)
            /*
   * Create drawing context and plug our rendering stage into it.
   */
   svga->swtnl.draw = draw_create(&svga->pipe);
   if (svga->swtnl.draw == NULL)
               draw_set_rasterize_stage(svga->swtnl.draw,
                     svga->blitter = util_blitter_create(&svga->pipe);
   if (!svga->blitter)
            /* must be done before installing Draw stages */
            const nir_alu_type bool_type =
      screen->screen.get_shader_param(&screen->screen, PIPE_SHADER_FRAGMENT,
               if (!screen->haveLineSmooth)
            /* enable/disable line stipple stage depending on device caps */
            /* always install AA point stage */
            /* Set wide line threshold above device limit (so we'll never really use it)
   */
   draw_wide_line_threshold(svga->swtnl.draw,
                  if (debug_get_bool_option("SVGA_SWTNL_FSE", false))
                  fail:
      if (svga->blitter)
            if (svga->swtnl.backend)
            if (svga->swtnl.draw)
               }
         void
   svga_destroy_swtnl(struct svga_context *svga)
   {
         }
