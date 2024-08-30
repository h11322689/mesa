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
      #include "draw/draw_context.h"
   #include "draw/draw_gs.h"
   #include "draw/draw_tess.h"
   #include "draw/draw_private.h"
   #include "draw/draw_pt.h"
   #include "draw/draw_vbuf.h"
   #include "draw/draw_vs.h"
   #include "tgsi/tgsi_dump.h"
   #include "util/u_math.h"
   #include "util/u_prim.h"
   #include "util/format/u_format.h"
   #include "util/u_draw.h"
         DEBUG_GET_ONCE_BOOL_OPTION(draw_fse, "DRAW_FSE", false)
   DEBUG_GET_ONCE_BOOL_OPTION(draw_no_fse, "DRAW_NO_FSE", false)
         /* Overall we split things into:
   *     - frontend -- prepare fetch_elts, draw_elts - eg vsplit
   *     - middle   -- fetch, shade, cliptest, viewport
   *     - pipeline -- the prim pipeline: clipping, wide lines, etc
   *     - backend  -- the vbuf_render provided by the driver.
   */
   static bool
   draw_pt_arrays(struct draw_context *draw,
                  enum mesa_prim prim,
      {
               if (draw->gs.geometry_shader)
         else if (draw->tes.tess_eval_shader)
            unsigned opt = PT_SHADE;
   if (!draw->render) {
                  if (draw_need_pipeline(draw, draw->rasterizer, out_prim)) {
                  if ((draw->clip_xy ||
         draw->clip_z ||
                  struct draw_pt_middle_end *middle;
   if (draw->pt.middle.llvm) {
         } else {
      if (opt == PT_SHADE && !draw->pt.no_fse)
         else
               struct draw_pt_front_end *frontend = draw->pt.frontend;
   if (frontend) {
      if (draw->pt.prim != prim || draw->pt.opt != opt) {
      /* In certain conditions switching primitives requires us to flush
   * and validate the different stages. One example is when smooth
   * lines are active but first drawn with triangles and then with
   * lines.
   */
   draw_do_flush(draw, DRAW_FLUSH_STATE_CHANGE);
      } else if (draw->pt.eltSize != draw->pt.user.eltSize || draw->pt.viewid != draw->pt.user.viewid) {
      /* Flush draw state if eltSize or viewid changed.
   * eltSize changes could be improved so only the frontend is flushed since it
   * converts all indices to ushorts and the fetch part of the middle
   * always prepares both linear and indexed.
   */
   frontend->flush(frontend, DRAW_FLUSH_STATE_CHANGE);
                  if (!frontend) {
                        draw->pt.frontend = frontend;
   draw->pt.eltSize = draw->pt.user.eltSize;
   draw->pt.viewid = draw->pt.user.viewid;
   draw->pt.prim = prim;
               if (draw->pt.rebind_parameters) {
      /* update constants, viewport dims, clip planes, etc */
   middle->bind_parameters(middle);
               for (unsigned i = 0; i < num_draws; i++) {
      /* Sanitize primitive length:
   */
            if (prim == MESA_PRIM_PATCHES) {
      first = draw->pt.vertices_per_patch;
      } else {
                  unsigned count = draw_pt_trim_count(draw_info[i].count, first, incr);
   if (draw->pt.user.eltSize) {
      if (index_bias_varies) {
         } else {
            } else {
                           if (count >= first)
            if (num_draws > 1 && draw->pt.user.increment_draw_id)
                  }
         void
   draw_pt_flush(struct draw_context *draw, unsigned flags)
   {
               if (draw->pt.frontend) {
               /* don't prepare if we only are flushing the backend */
   if (flags & DRAW_FLUSH_STATE_CHANGE)
               if (flags & DRAW_FLUSH_PARAMETER_CHANGE) {
            }
         bool
   draw_pt_init(struct draw_context *draw)
   {
      draw->pt.test_fse = debug_get_option_draw_fse();
            draw->pt.front.vsplit = draw_pt_vsplit(draw);
   if (!draw->pt.front.vsplit)
            draw->pt.middle.fetch_shade_emit = draw_pt_middle_fse(draw);
   if (!draw->pt.middle.fetch_shade_emit)
            draw->pt.middle.general = draw_pt_fetch_pipeline_or_emit(draw);
   if (!draw->pt.middle.general)
         #ifdef DRAW_LLVM_AVAILABLE
      if (draw->llvm) {
      draw->pt.middle.llvm = draw_pt_fetch_pipeline_or_emit_llvm(draw);
         #endif
            }
         void
   draw_pt_destroy(struct draw_context *draw)
   {
      if (draw->pt.middle.mesh) {
      draw->pt.middle.mesh->destroy(draw->pt.middle.mesh);
               if (draw->pt.middle.llvm) {
      draw->pt.middle.llvm->destroy(draw->pt.middle.llvm);
               if (draw->pt.middle.general) {
      draw->pt.middle.general->destroy(draw->pt.middle.general);
               if (draw->pt.middle.fetch_shade_emit) {
      draw->pt.middle.fetch_shade_emit->destroy(draw->pt.middle.fetch_shade_emit);
               if (draw->pt.front.vsplit) {
      draw->pt.front.vsplit->destroy(draw->pt.front.vsplit);
         }
         /**
   * Debug- print the first 'count' vertices.
   */
   static void
   draw_print_arrays(struct draw_context *draw, enum mesa_prim prim,
         {
      debug_printf("Draw arrays(prim = %u, start = %u, count = %u)\n",
            for (unsigned i = 0; i < count; i++) {
               if (draw->pt.user.eltSize) {
               switch (draw->pt.user.eltSize) {
   case 1:
      {
      const uint8_t *elem = (const uint8_t *) draw->pt.user.elts;
      }
      case 2:
      {
      const uint16_t *elem = (const uint16_t *) draw->pt.user.elts;
      }
      case 4:
      {
      const uint32_t *elem = (const uint32_t *) draw->pt.user.elts;
      }
      default:
      assert(0);
      }
   ii += index_bias;
   debug_printf("Element[%u + %u] + %i -> Vertex %u:\n", start, i,
      }
   else {
      /* non-indexed arrays */
   ii = start + i;
               for (unsigned j = 0; j < draw->pt.nr_vertex_elements; j++) {
                     if (draw->pt.vertex_element[j].instance_divisor) {
                  ptr += draw->pt.vertex_buffer[buf].buffer_offset;
                  debug_printf("  Attr %u: ", j);
   switch (draw->pt.vertex_element[j].src_format) {
   case PIPE_FORMAT_R32_FLOAT:
      {
      float *v = (float *) ptr;
      }
      case PIPE_FORMAT_R32G32_FLOAT:
      {
      float *v = (float *) ptr;
      }
      case PIPE_FORMAT_R32G32B32_FLOAT:
      {
      float *v = (float *) ptr;
      }
      case PIPE_FORMAT_R32G32B32A32_FLOAT:
      {
      float *v = (float *) ptr;
   debug_printf("RGBA %f %f %f %f  @ %p\n", v[0], v[1], v[2], v[3],
      }
      case PIPE_FORMAT_B8G8R8A8_UNORM:
      {
      uint8_t *u = (uint8_t *) ptr;
   debug_printf("BGRA %d %d %d %d  @ %p\n", u[0], u[1], u[2], u[3],
      }
      case PIPE_FORMAT_A8R8G8B8_UNORM:
      {
      uint8_t *u = (uint8_t *) ptr;
   debug_printf("ARGB %d %d %d %d  @ %p\n", u[0], u[1], u[2], u[3],
      }
      default:
      debug_printf("other format %s (fix me)\n",
               }
         /** Helper code for below */
   static inline void
   prim_restart_loop(struct draw_context *draw,
                     {
      const unsigned elt_max = draw->pt.user.eltMax;
   struct pipe_draw_start_count_bias cur = *draw_info;
            for (unsigned j = 0; j < draw_info->count; j++) {
      unsigned index = 0;
   unsigned i = util_clamped_uadd(draw_info->start, j);
   if (i < elt_max) {
      switch (draw->pt.user.eltSize) {
   case 1:
      index = ((const uint8_t*)elements)[i];
      case 2:
      index = ((const uint16_t*)elements)[i];
      case 4:
      index = ((const uint32_t*)elements)[i];
      default:
                     if (index == info->restart_index) {
      if (cur.count > 0) {
      /* draw elts up to prev pos */
      }
   /* begin new prim at next elt */
   cur.start = i + 1;
      }
   else {
            }
   if (cur.count > 0) {
            }
         /**
   * For drawing prims with primitive restart enabled.
   * Scan for restart indexes and draw the runs of elements/vertices between
   * the restarts.
   */
   static void
   draw_pt_arrays_restart(struct draw_context *draw,
                     {
               if (draw->pt.user.eltSize) {
      /* indexed prims (draw_elements) */
   for (unsigned i = 0; i < num_draws; i++)
      } else {
      /* Non-indexed prims (draw_arrays).
   * Primitive restart should have been handled in gallium frontends.
   */
         }
         /**
   * Resolve true values within pipe_draw_info.
   * If we're rendering from transform feedback/stream output
   * buffers both the count and max_index need to be computed
   * from the attached stream output target.
   */
   static void
   resolve_draw_info(const struct pipe_draw_info *raw_info,
                     const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *raw_draw,
   struct pipe_draw_info *info,
   {
      *info = *raw_info;
            struct draw_so_target *target =
         assert(vertex_buffer != NULL);
   draw->count = vertex_element->src_stride == 0 ? 0 :
            /* Stream output draw can not be indexed */
   assert(!info->index_size);
      }
         /*
   * Loop over all instances and execute draws for them.
   */
   static void
   draw_instances(struct draw_context *draw,
                  unsigned drawid_offset,
      {
               for (unsigned instance = 0; instance < info->instance_count; instance++) {
      unsigned instance_idx = instance + info->start_instance;
   draw->instance_id = instance;
   /* check for overflow */
   if (instance_idx < instance ||
      instance_idx < draw->start_instance) {
   /* if we overflown just set the instance id to the max */
               draw->pt.user.drawid = drawid_offset;
            if (info->primitive_restart) {
         } else {
      draw_pt_arrays(draw, info->mode, info->index_bias_varies,
            }
         /**
   * Draw vertex arrays.
   * This is the main entrypoint into the drawing module.  If drawing an indexed
   * primitive, the draw_set_indexes() function should have already been called
   * to specify the element/index buffer information.
   */
   void
   draw_vbo(struct draw_context *draw,
            const struct pipe_draw_info *info,
   unsigned drawid_offset,
   const struct pipe_draw_indirect_info *indirect,
   const struct pipe_draw_start_count_bias *draws,
      {
      unsigned fpstate = util_fpstate_get();
   struct pipe_draw_info resolved_info;
   struct pipe_draw_start_count_bias resolved_draw;
   const struct pipe_draw_info *use_info;
            if (info->instance_count == 0)
            /* Make sure that denorms are treated like zeros. This is
   * the behavior required by D3D10. OpenGL doesn't care.
   */
            if (indirect && indirect->count_from_stream_output) {
      resolve_draw_info(info, indirect, &draws[0], &resolved_info,
               use_info = &resolved_info;
   use_draws = &resolved_draw;
      } else {
      use_info = info;
               if (info->index_size) {
      assert(draw->pt.user.elts);
   draw->pt.user.min_index = use_info->index_bounds_valid ? use_info->min_index : 0;
      } else {
      draw->pt.user.min_index = 0;
      }
   draw->pt.user.eltSize = use_info->index_size ? draw->pt.user.eltSizeIB : 0;
   draw->pt.user.drawid = drawid_offset;
   draw->pt.user.increment_draw_id = use_info->increment_draw_id;
   draw->pt.user.viewid = 0;
            if (0) {
      for (unsigned i = 0; i < num_draws; i++)
      debug_printf("draw_vbo(mode=%u start=%u count=%u):\n",
            if (0)
            if (0) {
      debug_printf("Elements:\n");
   for (unsigned i = 0; i < draw->pt.nr_vertex_elements; i++) {
      debug_printf("  %u: src_offset=%u src_stride=%u inst_div=%u   vbuf=%u  format=%s\n",
               i,
   draw->pt.vertex_element[i].src_offset,
   draw->pt.vertex_element[i].src_stride,
      }
   debug_printf("Buffers:\n");
   for (unsigned i = 0; i < draw->pt.nr_vertex_buffers; i++) {
      debug_printf("  %u: offset=%u size=%d ptr=%p\n",
               i,
                  if (0) {
      for (unsigned i = 0; i < num_draws; i++)
      draw_print_arrays(draw, use_info->mode, use_draws[i].start,
                              unsigned index_limit = util_draw_max_index(draw->pt.vertex_buffer,
                  #ifdef DRAW_LLVM_AVAILABLE
         #endif
      {
      if (index_limit == 0) {
      /* one of the buffers is too small to do any valid drawing */
   debug_warning("draw: VBO too small to draw anything\n");
   util_fpstate_set(fpstate);
                  /* If we're collecting stats then make sure we start from scratch */
   if (draw->collect_statistics) {
                           /*
   * TODO: We could use draw->pt.max_index to further narrow
   * the min_index/max_index hints given by gallium frontends.
            if (use_info->view_mask) {
      u_foreach_bit(i, use_info->view_mask) {
      draw->pt.user.viewid = i;
         } else {
                  /* If requested emit the pipeline statistics for this run */
   if (draw->collect_statistics) {
         }
      }
      /* to be called after a mesh shader is run */
   void
   draw_mesh(struct draw_context *draw,
               {
               draw->pt.user.eltSize = 0;
   draw->pt.user.viewid = 0;
   draw->pt.user.drawid = 0;
   draw->pt.user.increment_draw_id = false;
                        }
