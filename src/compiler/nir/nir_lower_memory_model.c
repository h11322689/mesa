   /*
   * Copyright © 2020 Valve Corporation
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
   *
   */
      /*
   * Replaces make availability/visible semantics on barriers with
   * ACCESS_COHERENT on memory loads/stores
   */
      #include "nir/nir.h"
   #include "shader_enums.h"
      static bool
   get_intrinsic_info(nir_intrinsic_instr *intrin, nir_variable_mode *modes,
         {
      switch (intrin->intrinsic) {
   case nir_intrinsic_image_deref_load:
   case nir_intrinsic_image_deref_sparse_load:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *reads = true;
      case nir_intrinsic_image_deref_store:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *writes = true;
      case nir_intrinsic_image_deref_atomic:
   case nir_intrinsic_image_deref_atomic_swap:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *reads = true;
   *writes = true;
      case nir_intrinsic_load_ssbo:
      *modes = nir_var_mem_ssbo;
   *reads = true;
      case nir_intrinsic_store_ssbo:
      *modes = nir_var_mem_ssbo;
   *writes = true;
      case nir_intrinsic_ssbo_atomic:
   case nir_intrinsic_ssbo_atomic_swap:
      *modes = nir_var_mem_ssbo;
   *reads = true;
   *writes = true;
      case nir_intrinsic_load_global:
      *modes = nir_var_mem_global;
   *reads = true;
      case nir_intrinsic_store_global:
      *modes = nir_var_mem_global;
   *writes = true;
      case nir_intrinsic_global_atomic:
   case nir_intrinsic_global_atomic_swap:
      *modes = nir_var_mem_global;
   *reads = true;
   *writes = true;
      case nir_intrinsic_load_deref:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *reads = true;
      case nir_intrinsic_store_deref:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *writes = true;
      case nir_intrinsic_deref_atomic:
   case nir_intrinsic_deref_atomic_swap:
      *modes = nir_src_as_deref(intrin->src[0])->modes;
   *reads = true;
   *writes = true;
      default:
         }
      }
      static bool
   visit_instr(nir_instr *instr, uint32_t *cur_modes, unsigned vis_avail_sem)
   {
      if (instr->type != nir_instr_type_intrinsic)
                  if (intrin->intrinsic == nir_intrinsic_barrier &&
      (nir_intrinsic_memory_semantics(intrin) & vis_avail_sem)) {
            unsigned semantics = nir_intrinsic_memory_semantics(intrin);
   nir_intrinsic_set_memory_semantics(
                     if (!*cur_modes)
            nir_variable_mode modes;
   bool reads = false, writes = false;
   if (!get_intrinsic_info(intrin, &modes, &reads, &writes))
            if (!reads && vis_avail_sem == NIR_MEMORY_MAKE_VISIBLE)
         if (!writes && vis_avail_sem == NIR_MEMORY_MAKE_AVAILABLE)
            if (!nir_intrinsic_has_access(intrin))
                     if (access & (ACCESS_NON_READABLE | ACCESS_NON_WRITEABLE | ACCESS_CAN_REORDER | ACCESS_COHERENT))
            if (*cur_modes & modes) {
      nir_intrinsic_set_access(intrin, access | ACCESS_COHERENT);
                  }
      static bool
   lower_make_visible(nir_cf_node *cf_node, uint32_t *cur_modes)
   {
      bool progress = false;
   switch (cf_node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(cf_node);
   nir_foreach_instr(instr, block)
            }
   case nir_cf_node_if: {
      nir_if *nif = nir_cf_node_as_if(cf_node);
   uint32_t cur_modes_then = *cur_modes;
   uint32_t cur_modes_else = *cur_modes;
   foreach_list_typed(nir_cf_node, if_node, node, &nif->then_list)
         foreach_list_typed(nir_cf_node, if_node, node, &nif->else_list)
         *cur_modes |= cur_modes_then | cur_modes_else;
      }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
   assert(!nir_loop_has_continue_construct(loop));
   bool loop_progress;
   do {
      loop_progress = false;
   foreach_list_typed(nir_cf_node, loop_node, node, &loop->body)
            } while (loop_progress);
      }
   case nir_cf_node_function:
         }
      }
      static bool
   lower_make_available(nir_cf_node *cf_node, uint32_t *cur_modes)
   {
      bool progress = false;
   switch (cf_node->type) {
   case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(cf_node);
   nir_foreach_instr_reverse(instr, block)
            }
   case nir_cf_node_if: {
      nir_if *nif = nir_cf_node_as_if(cf_node);
   uint32_t cur_modes_then = *cur_modes;
   uint32_t cur_modes_else = *cur_modes;
   foreach_list_typed_reverse(nir_cf_node, if_node, node, &nif->then_list)
         foreach_list_typed_reverse(nir_cf_node, if_node, node, &nif->else_list)
         *cur_modes |= cur_modes_then | cur_modes_else;
      }
   case nir_cf_node_loop: {
      nir_loop *loop = nir_cf_node_as_loop(cf_node);
   assert(!nir_loop_has_continue_construct(loop));
   bool loop_progress;
   do {
      loop_progress = false;
   foreach_list_typed_reverse(nir_cf_node, loop_node, node, &loop->body)
            } while (loop_progress);
      }
   case nir_cf_node_function:
         }
      }
      bool
   nir_lower_memory_model(nir_shader *shader)
   {
               nir_function_impl *impl = nir_shader_get_entrypoint(shader);
            uint32_t modes = 0;
   foreach_list_typed(nir_cf_node, cf_node, node, cf_list)
            modes = 0;
   foreach_list_typed_reverse(nir_cf_node, cf_node, node, cf_list)
            if (progress)
         else
               }
