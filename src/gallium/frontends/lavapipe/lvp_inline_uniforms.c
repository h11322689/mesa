   /*
   * Copyright © 2020 Advanced Micro Devices, Inc.
   * Copyright © 2022 Valve Corporation
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
      /* enhanced version of nir_inline_uniforms that can inline from any uniform buffer
   * see nir_inline_uniforms.c for more details
   */
      #include "nir_builder.h"
   #include "nir_loop_analyze.h"
   #include "lvp_private.h"
      static bool
   is_src_uniform_load(nir_src src)
   {
      if (nir_src_bit_size(src) != 32 || nir_src_num_components(src) != 1 || nir_src_is_const(src))
         return nir_collect_src_uniforms(&src, 0, NULL, NULL,
      }
      static void
   process_node(nir_cf_node *node, nir_loop_info *info,
               {
      switch (node->type) {
   case nir_cf_node_if: {
      nir_if *if_node = nir_cf_node_as_if(node);
   const nir_src *cond = &if_node->condition;
   nir_add_inlinable_uniforms(cond, info, uni_offsets, num_offsets,
            /* Do not pass loop info down so only alow induction variable
   * in loop terminator "if":
   *
   *     for (i = 0; true; i++)
   *         if (i == count)
   *             if (i == num)
   *                 <no break>
   *             break
   *
   * so "num" won't be inlined due to the "if" is not a
   * terminator.
   */
            foreach_list_typed(nir_cf_node, nested_node, node, &if_node->then_list)
         foreach_list_typed(nir_cf_node, nested_node, node, &if_node->else_list)
                     case nir_cf_node_loop: {
               /* Replace loop info, no nested loop info currently:
   *
   *     for (i = 0; i < count0; i++)
   *         for (j = 0; j < count1; j++)
   *             if (i == num)
   *
   * so "num" won't be inlined due to "i" is an induction
   * variable of upper loop.
   */
            foreach_list_typed(nir_cf_node, nested_node, node, &loop->body) {
      bool is_terminator = false;
   list_for_each_entry(nir_loop_terminator, terminator,
                  if (nested_node == &terminator->nif->cf_node) {
      is_terminator = true;
                  /* Allow induction variables for terminator "if" only:
   *
   *     for (i = 0; i < count; i++)
   *         if (i == num)
   *             <no break>
   *
   * so "num" won't be inlined due to the "if" is not a
   * terminator.
   */
   nir_loop_info *use_info = is_terminator ? info : NULL;
      }
               case nir_cf_node_block: {
      nir_block *block = nir_cf_node_as_block(node);
   nir_foreach_instr(instr, block) {
      if (instr->type == nir_instr_type_intrinsic) {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   if (intr->intrinsic == nir_intrinsic_store_deref && is_src_uniform_load(intr->src[1]))
         }
      }
   default:
            }
      bool
   lvp_find_inlinable_uniforms(struct lvp_shader *shader, nir_shader *nir)
   {
      bool ret = false;
   struct set *stores = _mesa_set_create(nir, _mesa_hash_pointer, _mesa_key_pointer_equal);
   nir_foreach_function_impl(impl, nir) {
               foreach_list_typed(nir_cf_node, node, node, &impl->body)
      }
   const unsigned threshold = 5;
   set_foreach(stores, entry) {
      const nir_src *src = entry->key;
   unsigned counter = 0;
   list_for_each_entry(nir_src, rsrc, &src->ssa->uses, use_link) {
      counter++;
   if (counter >= threshold)
      }
   if (counter >= threshold) {
                                    if (nir_collect_src_uniforms(src, 0, uni_offsets, new_num,
            ret = true;
            }
   for (unsigned i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      if (shader->inlines.count[i]) {
      shader->inlines.can_inline |= BITFIELD_BIT(i);
         }
      }
      void
   lvp_inline_uniforms(nir_shader *nir, const struct lvp_shader *shader, const uint32_t *uniform_values, uint32_t ubo)
   {
      if (!shader->inlines.can_inline)
            nir_foreach_function_impl(impl, nir) {
      nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              /* Only replace loads with constant offsets. */
   if (intr->intrinsic == nir_intrinsic_load_ubo &&
      nir_src_is_const(intr->src[0]) &&
   nir_src_as_uint(intr->src[0]) == ubo &&
   nir_src_is_const(intr->src[1]) &&
   /* TODO: Can't handle other bit sizes for now. */
   intr->def.bit_size == 32) {
   int num_components = intr->def.num_components;
                        if (num_components == 1) {
      /* Just replace the uniform load to constant load. */
   for (unsigned i = 0; i < num_uniforms; i++) {
      if (offset == uniform_dw_offsets[i]) {
      b.cursor = nir_before_instr(&intr->instr);
   nir_def *def = nir_imm_int(&b, uniform_values[i]);
   nir_def_rewrite_uses(&intr->def, def);
   nir_instr_remove(&intr->instr);
            } else {
      /* Lower vector uniform load to scalar and replace each
   * found component load with constant load.
   */
                                 /* Find component to replace. */
   for (unsigned i = 0; i < num_uniforms; i++) {
      uint32_t uni_offset = uniform_dw_offsets[i];
   if (uni_offset >= offset && uni_offset < max_offset) {
      int index = uni_offset - offset;
                                       /* Create per-component uniform load. */
   for (unsigned i = 0; i < num_components; i++) {
      if (!components[i]) {
      uint32_t scalar_offset = (offset + i) * 4;
   components[i] = nir_load_ubo(&b, 1, intr->def.bit_size,
               nir_intrinsic_instr *load =
         nir_intrinsic_set_align(load, NIR_ALIGN_MUL_MAX, scalar_offset);
                        /* Replace the original uniform load. */
   nir_def_rewrite_uses(&intr->def,
                              nir_metadata_preserve(impl, nir_metadata_block_index |
         }
