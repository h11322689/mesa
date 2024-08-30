   /*
   * Copyright © 2020 Google, Inc.
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
   * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   * SOFTWARE.
   */
      #include "nir.h"
   #include "nir_builder.h"
      /* A pass to split intrinsics with discontinuous writemasks into ones
   * with contiguous writemasks starting with .x, ie:
   *
   *   vec4 32 ssa_76 = vec4 ssa_35, ssa_35, ssa_35, ssa_35
   *   intrinsic store_ssbo (ssa_76, ssa_105, ssa_106) (2, 0, 4, 0) // wrmask=y
   *
   * is turned into:
   *
   *   vec4 32 ssa_76 = vec4 ssa_35, ssa_35, ssa_35, ssa_35
   *   vec1 32 ssa_107 = load_const (0x00000001)
   *   vec1 32 ssa_108 = iadd ssa_106, ssa_107
   *   vec1 32 ssa_109 = mov ssa_76.y
   *   intrinsic store_ssbo (ssa_109, ssa_105, ssa_108) (1, 0, 4, 0) // wrmask=x
   *
   * and likewise:
   *
   *   vec4 32 ssa_76 = vec4 ssa_35, ssa_35, ssa_35, ssa_35
   *   intrinsic store_ssbo (ssa_76, ssa_105, ssa_106) (15, 0, 4, 0) // wrmask=xzw
   *
   * is split into:
   *
   *   // .x component:
   *   vec4 32 ssa_76 = vec4 ssa_35, ssa_35, ssa_35, ssa_35
   *   vec1 32 ssa_107 = load_const (0x00000000)
   *   vec1 32 ssa_108 = iadd ssa_106, ssa_107
   *   vec1 32 ssa_109 = mov ssa_76.x
   *   intrinsic store_ssbo (ssa_109, ssa_105, ssa_108) (1, 0, 4, 0) // wrmask=x
   *   // .zw components:
   *   vec1 32 ssa_110 = load_const (0x00000002)
   *   vec1 32 ssa_111 = iadd ssa_106, ssa_110
   *   vec2 32 ssa_112 = mov ssa_76.zw
   *   intrinsic store_ssbo (ssa_112, ssa_105, ssa_111) (3, 0, 4, 0) // wrmask=xy
   */
      static int
   value_src(nir_intrinsic_op intrinsic)
   {
      switch (intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_ssbo:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         default:
            }
      static int
   offset_src(nir_intrinsic_op intrinsic)
   {
      switch (intrinsic) {
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_shared:
   case nir_intrinsic_store_global:
   case nir_intrinsic_store_scratch:
         case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_ssbo:
         default:
            }
      static void
   split_wrmask(nir_builder *b, nir_intrinsic_instr *intr)
   {
                                 unsigned num_srcs = info->num_srcs;
   unsigned value_idx = value_src(intr->intrinsic);
            unsigned wrmask = nir_intrinsic_write_mask(intr);
   while (wrmask) {
      unsigned first_component = ffs(wrmask) - 1;
            nir_def *value = intr->src[value_idx].ssa;
            /* swizzle out the consecutive components that we'll store
   * in this iteration:
   */
   unsigned cur_mask = (BITFIELD_MASK(length) << first_component);
            /* and create the replacement intrinsic: */
   nir_intrinsic_instr *new_intr =
            nir_intrinsic_copy_const_indices(new_intr, intr);
                     if (nir_intrinsic_has_align_mul(intr)) {
      assert(nir_intrinsic_has_align_offset(intr));
                                             /* if the instruction has a BASE, fold the offset adjustment
   * into that instead of adding alu instructions, otherwise add
   * instructions
   */
   unsigned offset_adj = offset_units * first_component;
   if (nir_intrinsic_has_base(intr)) {
      nir_intrinsic_set_base(new_intr,
      } else {
      offset = nir_iadd(b, offset,
                        /* Copy the sources, replacing value/offset, and passing everything
   * else through to the new instrution:
   */
   for (unsigned i = 0; i < num_srcs; i++) {
      if (i == value_idx) {
         } else if (i == offset_idx) {
         } else {
                              /* Clear the bits in the writemask that we just wrote, then try
   * again to see if more channels are left.
   */
               /* Finally remove the original intrinsic. */
      }
      struct nir_lower_wrmasks_state {
      nir_instr_filter_cb cb;
      };
      static bool
   nir_lower_wrmasks_instr(nir_builder *b, nir_instr *instr, void *data)
   {
               if (instr->type != nir_instr_type_intrinsic)
                     /* if no wrmask, then skip it: */
   if (!nir_intrinsic_has_write_mask(intr))
            /* if wrmask is already contiguous, then nothing to do: */
   if (nir_intrinsic_write_mask(intr) == BITFIELD_MASK(intr->num_components))
            /* do we know how to lower this instruction? */
   if (value_src(intr->intrinsic) < 0)
                     /* does backend need us to lower this intrinsic? */
   if (state->cb && !state->cb(instr, state->data))
                        }
      bool
   nir_lower_wrmasks(nir_shader *shader, nir_instr_filter_cb cb, const void *data)
   {
      struct nir_lower_wrmasks_state state = {
      .cb = cb,
               return nir_shader_instructions_pass(shader,
                        }
