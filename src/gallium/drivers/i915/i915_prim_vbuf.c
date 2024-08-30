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
   * Build post-transformation, post-clipping vertex buffers and element
   * lists by hooking into the end of the primitive pipeline and
   * manipulating the vertex_id field in the vertex headers.
   *
   * XXX: work in progress
   *
   * \author José Fonseca <jfonseca@vmware.com>
   * \author Keith Whitwell <keithw@vmware.com>
   */
      #include "draw/draw_context.h"
   #include "draw/draw_vbuf.h"
   #include "util/u_debug.h"
   #include "util/u_inlines.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
      #include "i915_batch.h"
   #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_state.h"
      /**
   * Primitive renderer for i915.
   */
   struct i915_vbuf_render {
                        /** Vertex size in bytes */
            /** Software primitive */
            /** Hardware primitive */
            /** Genereate a vertex list */
            /* Stuff for the vbo */
   struct i915_winsys_buffer *vbo;
   size_t vbo_size;       /**< current size of allocated buffer */
   size_t vbo_alloc_size; /**< minimum buffer size to allocate */
   size_t vbo_hw_offset;  /**< offset that we program the hardware with */
   size_t vbo_sw_offset;  /**< offset that we work with */
   size_t vbo_index;      /**< index offset to be added to all indices */
   void *vbo_ptr;
   size_t vbo_max_used;
      };
      /**
   * Basically a cast wrapper.
   */
   static inline struct i915_vbuf_render *
   i915_vbuf_render(struct vbuf_render *render)
   {
      assert(render);
      }
      /**
   * If vbo state differs between renderer and context
   * push state to the context. This function pushes
   * hw_offset to i915->vbo_offset and vbo to i915->vbo.
   *
   * Side effects:
   *    May updates context vbo_offset and vbo fields.
   */
   static void
   i915_vbuf_update_vbo_state(struct vbuf_render *render)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
            if (i915->vbo != i915_render->vbo ||
      i915->vbo_offset != i915_render->vbo_hw_offset) {
   i915->vbo = i915_render->vbo;
   i915->vbo_offset = i915_render->vbo_hw_offset;
         }
      /**
   * Callback exported to the draw module.
   * Returns the current vertex_info.
   *
   * Side effects:
   *    If state is dirty update derived state.
   */
   static const struct vertex_info *
   i915_vbuf_render_get_vertex_info(struct vbuf_render *render)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
            if (i915->dirty) {
      /* make sure we have up to date vertex layout */
                  }
      /**
   * Reserve space in the vbo for vertices.
   *
   * Side effects:
   *    None.
   */
   static bool
   i915_vbuf_render_reserve(struct i915_vbuf_render *i915_render, size_t size)
   {
               if (i915_render->vbo_size < size + i915_render->vbo_sw_offset)
            if (i915->vbo_flushed)
               }
      /**
   * Allocate a new vbo buffer should there not be enough space for
   * the requested number of vertices by the draw module.
   *
   * Side effects:
   *    Updates hw_offset, sw_offset, index and allocates a new buffer.
   *    Will set i915->vbo to null on buffer allocation.
   */
   static void
   i915_vbuf_render_new_buf(struct i915_vbuf_render *i915_render, size_t size)
   {
      struct i915_context *i915 = i915_render->i915;
            if (i915_render->vbo) {
      iws->buffer_unmap(iws, i915_render->vbo);
   iws->buffer_destroy(iws, i915_render->vbo);
   /*
   * XXX If buffers where referenced then this should be done in
   * update_vbo_state but since they arn't and malloc likes to reuse
   * memory we need to set it to null
   */
   i915->vbo = NULL;
                        i915_render->vbo_size = MAX2(size, i915_render->vbo_alloc_size);
   i915_render->vbo_hw_offset = 0;
   i915_render->vbo_sw_offset = 0;
            i915_render->vbo =
            }
      /**
   * Callback exported to the draw module.
   *
   * Side effects:
   *    Updates hw_offset, sw_offset, index and may allocate
   *    a new buffer. Also updates may update the vbo state
   *    on the i915 context.
   */
   static bool
   i915_vbuf_render_allocate_vertices(struct vbuf_render *render,
         {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   size_t size = (size_t)vertex_size * (size_t)nr_vertices;
            /*
   * Align sw_offset with first multiple of vertex size from hw_offset.
   * Set index to be the multiples from from hw_offset to sw_offset.
   * i915_vbuf_render_new_buf will reset index, sw_offset, hw_offset
   * when it allocates a new buffer this is correct.
   */
   {
      offset = i915_render->vbo_sw_offset - i915_render->vbo_hw_offset;
   offset = util_align_npot(offset, vertex_size);
   i915_render->vbo_sw_offset = i915_render->vbo_hw_offset + offset;
               if (!i915_vbuf_render_reserve(i915_render, size))
            /*
   * If a new buffer has been alocated sw_offset,
   * hw_offset & index will be reset by new_buf
                              if (!i915_render->vbo)
            }
      static void *
   i915_vbuf_render_map_vertices(struct vbuf_render *render)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
            if (i915->vbo_flushed)
               }
      static void
   i915_vbuf_render_unmap_vertices(struct vbuf_render *render, uint16_t min_index,
         {
               i915_render->vbo_max_index = max_index;
   i915_render->vbo_max_used = MAX2(i915_render->vbo_max_used,
      }
      /**
   * Ensure that the given max_index given is not larger ushort max.
   * If it is larger then ushort max it advanced the hw_offset to the
   * same position in the vbo as sw_offset and set index to zero.
   *
   * Side effects:
   *    On failure update hw_offset and index.
   */
   static void
   i915_vbuf_ensure_index_bounds(struct vbuf_render *render, unsigned max_index)
   {
               if (max_index + i915_render->vbo_index < ((1 << 17) - 1))
            i915_render->vbo_hw_offset = i915_render->vbo_sw_offset;
               }
      static void
   i915_vbuf_render_set_primitive(struct vbuf_render *render, enum mesa_prim prim)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
            switch (prim) {
   case MESA_PRIM_POINTS:
      i915_render->hwprim = PRIM3D_POINTLIST;
   i915_render->fallback = 0;
      case MESA_PRIM_LINES:
      i915_render->hwprim = PRIM3D_LINELIST;
   i915_render->fallback = 0;
      case MESA_PRIM_LINE_LOOP:
      i915_render->hwprim = PRIM3D_LINELIST;
   i915_render->fallback = MESA_PRIM_LINE_LOOP;
      case MESA_PRIM_LINE_STRIP:
      i915_render->hwprim = PRIM3D_LINESTRIP;
   i915_render->fallback = 0;
      case MESA_PRIM_TRIANGLES:
      i915_render->hwprim = PRIM3D_TRILIST;
   i915_render->fallback = 0;
      case MESA_PRIM_TRIANGLE_STRIP:
      i915_render->hwprim = PRIM3D_TRISTRIP;
   i915_render->fallback = 0;
      case MESA_PRIM_TRIANGLE_FAN:
      i915_render->hwprim = PRIM3D_TRIFAN;
   i915_render->fallback = 0;
      case MESA_PRIM_QUADS:
      i915_render->hwprim = PRIM3D_TRILIST;
   i915_render->fallback = MESA_PRIM_QUADS;
      case MESA_PRIM_QUAD_STRIP:
      i915_render->hwprim = PRIM3D_TRILIST;
   i915_render->fallback = MESA_PRIM_QUAD_STRIP;
      case MESA_PRIM_POLYGON:
      i915_render->hwprim = PRIM3D_POLY;
   i915_render->fallback = 0;
      default:
      /* FIXME: Actually, can handle a lot more just fine... */
         }
      /**
   * Used for fallbacks in draw_arrays
   */
   static void
   draw_arrays_generate_indices(struct vbuf_render *render, unsigned start,
         {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned i;
   unsigned end = start + nr + i915_render->vbo_index;
            switch (type) {
   case 0:
      for (i = start; i + 1 < end; i += 2)
         if (i < end)
            case MESA_PRIM_LINE_LOOP:
      if (nr >= 2) {
      for (i = start + 1; i < end; i++)
            }
      case MESA_PRIM_QUADS:
      for (i = start; i + 3 < end; i += 4) {
      OUT_BATCH((i + 0) | (i + 1) << 16);
   OUT_BATCH((i + 3) | (i + 1) << 16);
      }
      case MESA_PRIM_QUAD_STRIP:
      for (i = start; i + 3 < end; i += 2) {
      OUT_BATCH((i + 0) | (i + 1) << 16);
   OUT_BATCH((i + 3) | (i + 2) << 16);
      }
      default:
            }
      static unsigned
   draw_arrays_calc_nr_indices(uint32_t nr, unsigned type)
   {
      switch (type) {
   case 0:
         case MESA_PRIM_LINE_LOOP:
      if (nr >= 2)
         else
      case MESA_PRIM_QUADS:
         case MESA_PRIM_QUAD_STRIP:
         default:
      assert(0);
         }
      static void
   draw_arrays_fallback(struct vbuf_render *render, unsigned start, uint32_t nr)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
            nr_indices = draw_arrays_calc_nr_indices(nr, i915_render->fallback);
   if (!nr_indices)
                     if (i915->dirty)
            if (i915->hardware_dirty)
            if (!BEGIN_BATCH(1 + (nr_indices + 1) / 2)) {
               /* Make sure state is re-emitted after a flush:
   */
   i915_emit_hardware_state(i915);
            if (!BEGIN_BATCH(1 + (nr_indices + 1) / 2)) {
      mesa_loge("i915: Failed to allocate space for %d indices in fresh "
               assert(0);
                  OUT_BATCH(_3DPRIMITIVE | PRIM_INDIRECT | i915_render->hwprim |
                  out:
         }
      static void
   i915_vbuf_render_draw_arrays(struct vbuf_render *render, unsigned start,
         {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
            if (i915_render->fallback) {
      draw_arrays_fallback(render, start, nr);
               i915_vbuf_ensure_index_bounds(render, start + nr);
            if (i915->dirty)
            if (i915->hardware_dirty)
            if (!BEGIN_BATCH(2)) {
               /* Make sure state is re-emitted after a flush:
   */
   i915_emit_hardware_state(i915);
            if (!BEGIN_BATCH(2)) {
      assert(0);
                  OUT_BATCH(_3DPRIMITIVE | PRIM_INDIRECT | PRIM_INDIRECT_SEQUENTIAL |
               out:
         }
      /**
   * Used for normal and fallback emitting of indices
   * If type is zero normal operation assumed.
   */
   static void
   draw_generate_indices(struct vbuf_render *render, const uint16_t *indices,
         {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned i;
            switch (type) {
   case 0:
      for (i = 0; i + 1 < nr_indices; i += 2) {
         }
   if (i < nr_indices) {
         }
      case MESA_PRIM_LINE_LOOP:
      if (nr_indices >= 2) {
      for (i = 1; i < nr_indices; i++)
            }
      case MESA_PRIM_QUADS:
      for (i = 0; i + 3 < nr_indices; i += 4) {
      OUT_BATCH((o + indices[i + 0]) | (o + indices[i + 1]) << 16);
   OUT_BATCH((o + indices[i + 3]) | (o + indices[i + 1]) << 16);
      }
      case MESA_PRIM_QUAD_STRIP:
      for (i = 0; i + 3 < nr_indices; i += 2) {
      OUT_BATCH((o + indices[i + 0]) | (o + indices[i + 1]) << 16);
   OUT_BATCH((o + indices[i + 3]) | (o + indices[i + 2]) << 16);
      }
      default:
      assert(0);
         }
      static unsigned
   draw_calc_nr_indices(uint32_t nr_indices, unsigned type)
   {
      switch (type) {
   case 0:
         case MESA_PRIM_LINE_LOOP:
      if (nr_indices >= 2)
         else
      case MESA_PRIM_QUADS:
         case MESA_PRIM_QUAD_STRIP:
         default:
      assert(0);
         }
      static void
   i915_vbuf_render_draw_elements(struct vbuf_render *render,
         {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
                     nr_indices = draw_calc_nr_indices(nr_indices, i915_render->fallback);
   if (!nr_indices)
                     if (i915->dirty)
            if (i915->hardware_dirty)
            if (!BEGIN_BATCH(1 + (nr_indices + 1) / 2)) {
               /* Make sure state is re-emitted after a flush:
   */
   i915_emit_hardware_state(i915);
            if (!BEGIN_BATCH(1 + (nr_indices + 1) / 2)) {
      mesa_loge("i915: Failed to allocate space for %d indices in fresh "
               assert(0);
                  OUT_BATCH(_3DPRIMITIVE | PRIM_INDIRECT | i915_render->hwprim |
         draw_generate_indices(render, indices, save_nr_indices,
         out:
         }
      static void
   i915_vbuf_render_release_vertices(struct vbuf_render *render)
   {
               i915_render->vbo_sw_offset += i915_render->vbo_max_used;
            /*
   * Micro optimization, by calling update here we the offset change
   * will be picked up on the next pipe_context::draw_*.
   */
      }
      static void
   i915_vbuf_render_destroy(struct vbuf_render *render)
   {
      struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
            if (i915_render->vbo) {
      i915->vbo = NULL;
   iws->buffer_unmap(iws, i915_render->vbo);
                  }
      /**
   * Create a new primitive render.
   */
   static struct vbuf_render *
   i915_vbuf_render_create(struct i915_context *i915)
   {
                                 /* NOTE: it must be such that state and vertices indices fit in a single
   * batch buffer. 4096 is one batch buffer and 430 is the max amount of
   * state in dwords. The result is the number of 16-bit indices which can
   * fit in a single batch buffer.
   */
            i915_render->base.get_vertex_info = i915_vbuf_render_get_vertex_info;
   i915_render->base.allocate_vertices = i915_vbuf_render_allocate_vertices;
   i915_render->base.map_vertices = i915_vbuf_render_map_vertices;
   i915_render->base.unmap_vertices = i915_vbuf_render_unmap_vertices;
   i915_render->base.set_primitive = i915_vbuf_render_set_primitive;
   i915_render->base.draw_elements = i915_vbuf_render_draw_elements;
   i915_render->base.draw_arrays = i915_vbuf_render_draw_arrays;
   i915_render->base.release_vertices = i915_vbuf_render_release_vertices;
            i915_render->vbo = NULL;
   i915_render->vbo_ptr = NULL;
   i915_render->vbo_size = 0;
   i915_render->vbo_hw_offset = 0;
   i915_render->vbo_sw_offset = 0;
               }
      /**
   * Create a new primitive vbuf/render stage.
   */
   struct draw_stage *
   i915_draw_vbuf_stage(struct i915_context *i915)
   {
      struct vbuf_render *render;
            render = i915_vbuf_render_create(i915);
   if (!render)
            stage = draw_vbuf_stage(i915->draw, render);
   if (!stage) {
      render->destroy(render);
      }
   /** TODO JB: this shouldn't be here */
               }
