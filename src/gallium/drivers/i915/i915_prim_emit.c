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
      #include "draw/draw_pipe.h"
   #include "util/u_math.h"
   #include "util/u_memory.h"
   #include "util/u_pack_color.h"
      #include "i915_batch.h"
   #include "i915_context.h"
   #include "i915_reg.h"
   #include "i915_state.h"
      /**
   * Primitive emit to hardware.  No support for vertex buffers or any
   * nice fast paths.
   */
   struct setup_stage {
                  };
      /**
   * Basically a cast wrapper.
   */
   static inline struct setup_stage *
   setup_stage(struct draw_stage *stage)
   {
         }
      /**
   * Extract the needed fields from vertex_header and emit i915 dwords.
   * Recall that the vertices are constructed by the 'draw' module and
   * have a couple of slots at the beginning (1-dword header, 4-dword
   * clip pos) that we ignore here.
   */
   static inline void
   emit_hw_vertex(struct i915_context *i915, const struct vertex_header *vertex)
   {
      const struct vertex_info *vinfo = &i915->current.vertex_info.draw;
   uint32_t i;
                     for (i = 0; i < vinfo->num_attribs; i++) {
      const uint32_t j = vinfo->attrib[i].src_index;
   const float *attrib = vertex->data[j];
   switch (vinfo->attrib[i].emit) {
   case EMIT_1F:
      OUT_BATCH(fui(attrib[0]));
   count++;
      case EMIT_2F:
      OUT_BATCH(fui(attrib[0]));
   OUT_BATCH(fui(attrib[1]));
   count += 2;
      case EMIT_3F:
      OUT_BATCH(fui(attrib[0]));
   OUT_BATCH(fui(attrib[1]));
   OUT_BATCH(fui(attrib[2]));
   count += 3;
      case EMIT_4F:
      OUT_BATCH(fui(attrib[0]));
   OUT_BATCH(fui(attrib[1]));
   OUT_BATCH(fui(attrib[2]));
   OUT_BATCH(fui(attrib[3]));
   count += 4;
      case EMIT_4UB:
      OUT_BATCH(
      pack_ub4(float_to_ubyte(attrib[0]), float_to_ubyte(attrib[1]),
      count += 1;
      case EMIT_4UB_BGRA:
      OUT_BATCH(
      pack_ub4(float_to_ubyte(attrib[2]), float_to_ubyte(attrib[1]),
      count += 1;
      default:
            }
      }
      static inline void
   emit_prim(struct draw_stage *stage, struct prim_header *prim, unsigned hwprim,
         {
      struct i915_context *i915 = setup_stage(stage)->i915;
   unsigned vertex_size;
            if (i915->dirty)
            if (i915->hardware_dirty)
            /* need to do this after validation! */
   vertex_size = i915->current.vertex_info.draw.size * 4; /* in bytes */
            if (!BEGIN_BATCH(1 + nr * vertex_size / 4)) {
               /* Make sure state is re-emitted after a flush:
   */
            if (!BEGIN_BATCH(1 + nr * vertex_size / 4)) {
      assert(0);
                  /* Emit each triangle as a single primitive.  I told you this was
   * simple.
   */
            for (i = 0; i < nr; i++)
      }
      static void
   setup_tri(struct draw_stage *stage, struct prim_header *prim)
   {
         }
      static void
   setup_line(struct draw_stage *stage, struct prim_header *prim)
   {
         }
      static void
   setup_point(struct draw_stage *stage, struct prim_header *prim)
   {
         }
      static void
   setup_flush(struct draw_stage *stage, unsigned flags)
   {
   }
      static void
   reset_stipple_counter(struct draw_stage *stage)
   {
   }
      static void
   render_destroy(struct draw_stage *stage)
   {
         }
      /**
   * Create a new primitive setup/render stage.  This gets plugged into
   * the 'draw' module's pipeline.
   */
   struct draw_stage *
   i915_draw_render_stage(struct i915_context *i915)
   {
               setup->i915 = i915;
   setup->stage.draw = i915->draw;
   setup->stage.point = setup_point;
   setup->stage.line = setup_line;
   setup->stage.tri = setup_tri;
   setup->stage.flush = setup_flush;
   setup->stage.reset_stipple_counter = reset_stipple_counter;
               }
