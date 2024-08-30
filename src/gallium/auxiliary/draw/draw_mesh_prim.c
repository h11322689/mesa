   /**************************************************************************
   *
   * Copyright 2023 Red Hat.
   * All Rights Reserved.
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice shall be included
   * in all copies or substantial portions of the Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   *
   **************************************************************************/
      /*
   * Mesh primitive assembler
   *
   * This takes vertex and per-primitive data, and assembles a linear set
   * of draw compatible vertices.
   */
   #include "draw_mesh_prim.h"
      #include "draw/draw_context.h"
   #include "draw/draw_private.h"
   #include "util/u_debug.h"
   #include "util/u_memory.h"
   #include "util/u_prim.h"
      #include "pipe/p_defines.h"
      struct draw_mesh_prim
   {
               struct draw_prim_info *output_prims;
            const struct draw_prim_info *input_prims;
            unsigned num_prims;
   const char *per_prim;
   uint32_t num_per_prim;
   uint32_t added_prim_size;
      };
      static void
   add_prim(struct draw_mesh_prim *asmblr, unsigned length)
   {
               output_prims->primitive_lengths = realloc(output_prims->primitive_lengths, sizeof(unsigned) * (output_prims->primitive_count + 1));
   output_prims->primitive_lengths[output_prims->primitive_count] = length;
      }
         /*
   * Copy the vertex header along with its data from the current
   * vertex buffer into a buffer holding vertices arranged
   * into decomposed primitives (i.e. buffer without the
   * adjacency vertices)
   */
   static void
   copy_verts(struct draw_mesh_prim *asmblr,
         {
      char *output = (char*)asmblr->output_verts->verts;
            for (unsigned i = 0; i < num_indices; ++i) {
      unsigned idx = indices[i];
   unsigned output_offset =
         unsigned input_offset = asmblr->input_verts->stride * idx;
   memcpy(output + output_offset, input + input_offset,
            memcpy(output + output_offset + asmblr->input_verts->vertex_size,
         asmblr->per_prim + (asmblr->num_prims * asmblr->added_prim_size * 8),
      }
      }
      static bool
   cull_prim(struct draw_mesh_prim *asmblr)
   {
      if (asmblr->cull_prim_idx == -1)
            const uint32_t *cull_prim_ptr = (uint32_t *)(asmblr->per_prim + (asmblr->num_prims * asmblr->added_prim_size * 8));
               }
         static void
   prim_point(struct draw_mesh_prim *asmblr,
         {
                        if (cull_prim(asmblr)) {
      ++asmblr->num_prims;
      }
   add_prim(asmblr, 1);
      }
         static void
   prim_line(struct draw_mesh_prim *asmblr,
         {
               indices[0] = i0;
            if (cull_prim(asmblr)) {
      ++asmblr->num_prims;
      }
   add_prim(asmblr, 2);
      }
         static void
   prim_tri(struct draw_mesh_prim *asmblr,
         {
               indices[0] = i0;
   indices[1] = i1;
            if (cull_prim(asmblr)) {
      ++asmblr->num_prims;
      }
   add_prim(asmblr, 3);
      }
         #define FUNC assembler_run_linear
   #define GET_ELT(idx) (start + (idx))
   #include "draw_mesh_prim_tmp.h"
      #define FUNC assembler_run_elts
   #define LOCAL_VARS   const uint16_t *elts = input_prims->elts;
   #define GET_ELT(idx) (elts[start + (idx)])
   #include "draw_mesh_prim_tmp.h"
         void
   draw_mesh_prim_run(struct draw_context *draw,
                     unsigned num_per_prim_inputs,
   void *per_prim_inputs,
   int cull_prim_idx,
   const struct draw_prim_info *input_prims,
   {
      struct draw_mesh_prim asmblr_mesh;
   struct draw_mesh_prim *asmblr = &asmblr_mesh;
   unsigned start, i;
   unsigned max_primitives = input_prims->primitive_count;
            asmblr->output_prims = output_prims;
   asmblr->output_verts = output_verts;
   asmblr->input_prims = input_prims;
   asmblr->input_verts = input_verts;
   asmblr->num_prims = 0;
   asmblr->num_per_prim = num_per_prim_inputs;
   asmblr->per_prim = per_prim_inputs;
            output_prims->linear = true;
   output_prims->elts = NULL;
   output_prims->start = 0;
   output_prims->prim = input_prims->prim;
   output_prims->flags = 0x0;
   output_prims->primitive_lengths = MALLOC(sizeof(unsigned));
   output_prims->primitive_lengths[0] = 0;
            asmblr->added_prim_size = asmblr->num_per_prim * (4 * sizeof(float));
   output_verts->vertex_size = input_verts->vertex_size + asmblr->added_prim_size;
   output_verts->stride = output_verts->vertex_size;
   output_verts->verts = (struct vertex_header*)MALLOC(
                  for (start = i = 0; i < input_prims->primitive_count;
      start += input_prims->primitive_lengths[i], i++) {
   unsigned count = input_prims->primitive_lengths[i];
   if (input_prims->linear) {
      assembler_run_linear(asmblr, input_prims, input_verts,
      } else {
      assembler_run_elts(asmblr, input_prims, input_verts,
                     }
