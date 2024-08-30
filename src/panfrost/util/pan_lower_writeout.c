   /*
   * Copyright (C) 2018-2020 Collabora, Ltd.
   * Copyright (C) 2019-2020 Icecream95
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
      #include "compiler/nir/nir_builder.h"
   #include "pan_ir.h"
      /* Midgard can write all of color, depth and stencil in a single writeout
   * operation, so we merge depth/stencil stores with color stores.
   * If there are no color stores, we add a write to the "depth RT".
   *
   * For Bifrost, we want these combined so we can properly order
   * +ZS_EMIT with respect to +ATEST and +BLEND, as well as combining
   * depth/stencil stores into a single +ZS_EMIT op.
   */
      /*
   * Get the type to report for a piece of a combined store, given the store it
   * is combining from. If there is no store to render target #0, a dummy <0.0,
   * 0.0, 0.0, 0.0> write is used, so report a matching float32 type.
   */
   static nir_alu_type
   pan_nir_rt_store_type(nir_intrinsic_instr *store)
   {
         }
      static void
   pan_nir_emit_combined_store(nir_builder *b, nir_intrinsic_instr *rt0_store,
         {
      nir_intrinsic_instr *intr = nir_intrinsic_instr_create(
                     if (rt0_store)
      nir_intrinsic_set_io_semantics(intr,
      nir_intrinsic_set_src_type(intr, pan_nir_rt_store_type(rt0_store));
   nir_intrinsic_set_dest_type(intr, pan_nir_rt_store_type(stores[2]));
            nir_def *zero = nir_imm_int(b, 0);
            nir_def *src[] = {
      rt0_store ? rt0_store->src[0].ssa : zero4,
   rt0_store ? rt0_store->src[1].ssa : zero,
   stores[0] ? stores[0]->src[0].ssa : zero,
   stores[1] ? stores[1]->src[0].ssa : zero,
               for (int i = 0; i < ARRAY_SIZE(src); ++i)
               }
   bool
   pan_nir_lower_zs_store(nir_shader *nir)
   {
               if (nir->info.stage != MESA_SHADER_FRAGMENT)
            nir_foreach_function_impl(impl, nir) {
      nir_intrinsic_instr *stores[3] = {NULL};
            nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                  nir_io_semantics sem = nir_intrinsic_io_semantics(intr);
   if (sem.location == FRAG_RESULT_DEPTH) {
      stores[0] = intr;
      } else if (sem.location == FRAG_RESULT_STENCIL) {
      stores[1] = intr;
      } else if (sem.dual_source_blend_index) {
      assert(!stores[2]); /* there should be only 1 source for dual
         stores[2] = intr;
                     if (!writeout)
                     /* Ensure all stores are in the same block */
   for (unsigned i = 0; i < ARRAY_SIZE(stores); ++i) {
                              if (common_block)
         else
                        nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                     nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
                                                                                 /* Trying to write depth twice results in the
   * wrong blend shader being executed on
                                                   /* Insert a store to the depth RT (0xff) if needed */
   if (!replaced) {
                                 for (unsigned i = 0; i < ARRAY_SIZE(stores); ++i) {
      if (stores[i])
               nir_metadata_preserve(impl,
                        }
