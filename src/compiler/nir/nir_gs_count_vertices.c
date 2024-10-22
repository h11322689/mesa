   /*
   * Copyright © 2015 Intel Corporation
   *
   * Permission is hereby granted, free of charge, to any person obtaining a
   * copy of this software and associated documentation files (the "Software"),
   * to deal in the Software without restriction, including without limitation
   * the rights to use, copy, modify, merge, publish, distribute, sublicense,
   * and/or sell copies of the Software, and to permit persons to whom the
   * Software is furnished to do so, subject to the following conditions:
   *
   * The above copyright notice and this permission notice (including the next
   * paragraph) shall be included in all copies or substantial portions of the
   * Software.
   *
   * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   * IN THE SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
      static nir_intrinsic_instr *
   as_intrinsic(nir_instr *instr, nir_intrinsic_op op)
   {
      if (instr->type != nir_instr_type_intrinsic)
            nir_intrinsic_instr *intrin = nir_instr_as_intrinsic(instr);
   if (intrin->intrinsic != op)
               }
      static nir_intrinsic_instr *
   as_set_vertex_and_primitive_count(nir_instr *instr)
   {
         }
      /**
   * Count the number of vertices/primitives emitted by a geometry shader per stream.
   * If a constant number of vertices is emitted, the output is set to
   * that number, otherwise it is unknown at compile time and the
   * result will be -1.
   *
   * This only works if you've used nir_lower_gs_intrinsics() to do vertex
   * counting at the NIR level.
   */
   void
   nir_gs_count_vertices_and_primitives(const nir_shader *shader,
                     {
               int vtxcnt_arr[4] = { -1, -1, -1, -1 };
   int prmcnt_arr[4] = { -1, -1, -1, -1 };
            nir_foreach_function_impl(impl, shader) {
      /* set_vertex_and_primitive_count intrinsics only appear in predecessors of the
   * end block.  So we don't need to walk all of them.
   */
   set_foreach(impl->end_block->predecessors, entry) {
               nir_foreach_instr_reverse(instr, block) {
      nir_intrinsic_instr *intrin = as_set_vertex_and_primitive_count(instr);
                  unsigned stream = nir_intrinsic_stream_id(intrin);
                                 /* If the number of vertices/primitives is compile-time known, we use that,
   * otherwise we leave it at -1 which means that it's unknown.
   */
   if (nir_src_is_const(intrin->src[0]))
                        /* We've found contradictory set_vertex_and_primitive_count intrinsics.
   * This can happen if there are early-returns in main() and
   * different paths emit different numbers of vertices.
   */
   if (cnt_found[stream] && vtxcnt != vtxcnt_arr[stream])
                        vtxcnt_arr[stream] = vtxcnt;
   prmcnt_arr[stream] = prmcnt;
                     if (out_vtxcnt)
         if (out_prmcnt)
      }
