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
      /**
   * \file nir_sweep.c
   *
   * The nir_sweep() pass performs a mark and sweep pass over a nir_shader's associated
   * memory - anything still connected to the program will be kept, and any dead memory
   * we dropped on the floor will be freed.
   *
   * The expectation is that drivers should call this when finished compiling the shader
   * (after any optimization, lowering, and so on).  However, it's also fine to call it
   * earlier, and even many times, trading CPU cycles for memory savings.
   */
      #define steal_list(mem_ctx, type, list)        \
      foreach_list_typed(type, obj, node, list) { \
               static void sweep_cf_node(nir_shader *nir, nir_cf_node *cf_node);
      static void
   sweep_block(nir_shader *nir, nir_block *block)
   {
               /* sweep_impl will mark all metadata invalid.  We can safely release all of
   * this here.
   */
   ralloc_free(block->live_in);
            ralloc_free(block->live_out);
            nir_foreach_instr(instr, block) {
               switch (instr->type) {
   case nir_instr_type_tex:
      gc_mark_live(nir->gctx, nir_instr_as_tex(instr)->src);
      case nir_instr_type_phi:
      nir_foreach_phi_src(src, nir_instr_as_phi(instr))
            default:
               }
      static void
   sweep_if(nir_shader *nir, nir_if *iff)
   {
               foreach_list_typed(nir_cf_node, cf_node, node, &iff->then_list) {
                  foreach_list_typed(nir_cf_node, cf_node, node, &iff->else_list) {
            }
      static void
   sweep_loop(nir_shader *nir, nir_loop *loop)
   {
      assert(!nir_loop_has_continue_construct(loop));
            foreach_list_typed(nir_cf_node, cf_node, node, &loop->body) {
            }
      static void
   sweep_cf_node(nir_shader *nir, nir_cf_node *cf_node)
   {
      switch (cf_node->type) {
   case nir_cf_node_block:
      sweep_block(nir, nir_cf_node_as_block(cf_node));
      case nir_cf_node_if:
      sweep_if(nir, nir_cf_node_as_if(cf_node));
      case nir_cf_node_loop:
      sweep_loop(nir, nir_cf_node_as_loop(cf_node));
      default:
            }
      static void
   sweep_impl(nir_shader *nir, nir_function_impl *impl)
   {
                        foreach_list_typed(nir_cf_node, cf_node, node, &impl->body) {
                           /* Wipe out all the metadata, if any. */
      }
      static void
   sweep_function(nir_shader *nir, nir_function *f)
   {
      ralloc_steal(nir, f);
            if (f->impl)
      }
      void
   nir_sweep(nir_shader *nir)
   {
               struct list_head instr_gc_list;
            /* First, move ownership of all the memory to a temporary context; assume dead. */
            /* Start sweeping */
            ralloc_steal(nir, nir->gctx);
   ralloc_steal(nir, (char *)nir->info.name);
   if (nir->info.label)
            /* Variables are not dead.  Steal them back. */
            /* Recurse into functions, stealing their contents back. */
   foreach_list_typed(nir_function, func, node, &nir->functions) {
                  ralloc_steal(nir, nir->constant_data);
   ralloc_steal(nir, nir->xfb_info);
   ralloc_steal(nir, nir->printf_info);
   for (int i = 0; i < nir->printf_info_count; i++) {
      ralloc_steal(nir, nir->printf_info[i].arg_sizes);
               /* Free everything we didn't steal back. */
   gc_sweep_end(nir->gctx);
      }
