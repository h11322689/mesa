   /*
   * Copyright © 2020 Advanced Micro Devices, Inc.
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
      /* These passes enable converting uniforms to literals when it's profitable,
   * effectively inlining uniform values in the IR. The main benefit is register
   * usage decrease leading to better SMT (hyperthreading). It's accomplished
   * by targetting uniforms that determine whether a conditional branch is
   * taken or a loop can be unrolled.
   *
   * Only uniforms used in these places are analyzed:
   *   1. if condition
   *   2. loop terminator
   *   3. init and update value of induction variable used in loop terminator
   *
   * nir_find_inlinable_uniforms finds uniforms that can be inlined and stores
   * that information in shader_info.
   *
   * nir_inline_uniforms inlines uniform values.
   *
   * (uniforms must be lowered to load_ubo before calling this)
   */
      #include "nir_builder.h"
   #include "nir_loop_analyze.h"
      /* Maximum value in shader_info::inlinable_uniform_dw_offsets[] */
   #define MAX_OFFSET (UINT16_MAX * 4)
      #define MAX_NUM_BO 32
      /**
   * Collect uniforms used in a source
   *
   * Recursively collects all of the UBO loads with constant UBO index and
   * constant offset (per the restictions of \c max_num_bo and \c
   * max_offset). If any values are discovered that are non-constant, uniforms
   * that don't meet the restrictions, or if more than \c
   * MAX_INLINEABLE_UNIFORMS are discoverd for any one UBO, false is returned.
   *
   * When false is returned, the state of \c uni_offsets and \c num_offsets is
   * undefined.
   *
   * \param max_num_bo Maximum number of uniform buffer objects
   * \param max_offset Maximum offset within a UBO
   * \param uni_offset Array of \c max_num_bo * \c MAX_INLINABLE_UNIFORMS values
   *                   used to store offsets of discovered uniform loads.
   * \param num_offsets Array of \c max_num_bo values used to store the number
   *                    of uniforms collected from each UBO.
   */
   bool
   nir_collect_src_uniforms(const nir_src *src, int component,
               {
      assert(max_num_bo > 0 && max_num_bo <= MAX_NUM_BO);
                     switch (instr->type) {
   case nir_instr_type_alu: {
               /* Vector ops only need to check the corresponding component. */
   if (alu->op == nir_op_mov) {
      return nir_collect_src_uniforms(&alu->src[0].src,
                  } else if (nir_op_is_vec(alu->op)) {
      nir_alu_src *alu_src = alu->src + component;
   return nir_collect_src_uniforms(&alu_src->src, alu_src->swizzle[0],
                     /* Return true if all sources return true. */
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
                     if (input_sizes == 0) {
      /* For ops which has no input size, each component of dest is
   * only determined by the same component of srcs.
   */
   if (!nir_collect_src_uniforms(&alu_src->src, alu_src->swizzle[component],
                  } else {
      /* For ops which has input size, all components of dest are
   * determined by all components of srcs (except vec ops).
   */
   for (unsigned j = 0; j < input_sizes; j++) {
      if (!nir_collect_src_uniforms(&alu_src->src, alu_src->swizzle[j],
                        }
               case nir_instr_type_intrinsic: {
      nir_intrinsic_instr *intr = nir_instr_as_intrinsic(instr);
   /* Return true if the intrinsic loads from UBO 0 with a constant
   * offset.
   */
   if (intr->intrinsic == nir_intrinsic_load_ubo &&
      nir_src_is_const(intr->src[0]) && nir_src_num_components(intr->src[0]) == 1 &&
   nir_src_as_uint(intr->src[0]) < max_num_bo &&
   nir_src_is_const(intr->src[1]) &&
   nir_src_as_uint(intr->src[1]) <= max_offset &&
   /* TODO: Can't handle other bit sizes for now. */
   intr->def.bit_size == 32) {
                  /* If we're just checking that it's a uniform load, don't check (or
   * add to) the table.
   */
                                          /* Already recorded by other one */
   for (int i = 0; i < num_offsets[ubo]; i++) {
      if (uni_offsets[ubo * MAX_INLINABLE_UNIFORMS + i] == offset)
               /* Exceed uniform number limit */
                  /* Record the uniform offset. */
   uni_offsets[ubo * MAX_INLINABLE_UNIFORMS + num_offsets[ubo]++] = offset;
      }
               case nir_instr_type_load_const:
      /* Always return true for constants. */
         default:
            }
      static bool
   is_induction_variable(const nir_src *src, int component, nir_loop_info *info,
               {
               /* Return true for induction variable (ie. i in for loop) */
   for (int i = 0; i < info->num_induction_vars; i++) {
      nir_loop_induction_variable *var = info->induction_vars + i;
   if (var->def == src->ssa) {
      /* Induction variable should have constant initial value (ie. i = 0),
   * constant update value (ie. i++) and constant end condition
   * (ie. i < 10), so that we know the exact loop count for unrolling
   * the loop.
   *
   * Add uniforms need to be inlined for this induction variable's
   * initial and update value to be constant, for example:
   *
   *     for (i = init; i < count; i += step)
   *
   * We collect uniform "init" and "step" here.
   */
   if (var->init_src) {
      if (!nir_collect_src_uniforms(var->init_src, component,
                           if (var->update_src) {
      nir_alu_src *alu_src = var->update_src;
   if (!nir_collect_src_uniforms(&alu_src->src,
                                                   }
      void
   nir_add_inlinable_uniforms(const nir_src *cond, nir_loop_info *info,
               {
      uint8_t new_num[MAX_NUM_BO];
            /* If condition SSA is always scalar, so component is 0. */
            /* Allow induction variable which means a loop terminator. */
   if (info) {
               /* Limit terminator condition to loop unroll support case which is a simple
   * comparison (ie. "i < count" is supported, but "i + 1 < count" is not).
   */
   if (nir_is_supported_terminator_condition(cond_scalar)) {
                              /* One side of comparison is induction variable, the other side is
   * only uniform.
   */
   for (int i = 0; i < 2; i++) {
      if (is_induction_variable(&alu->src[i].src, alu->src[i].swizzle[0],
                  cond = &alu->src[1 - i].src;
   component = alu->src[1 - i].swizzle[0];
                        /* Only update uniform number when all uniforms in the expression
   * can be inlined. Partially inline uniforms can't lower if/loop.
   *
   * For example, uniform can be inlined for a shader is limited to 4,
   * and we have already added 3 uniforms, then want to deal with
   *
   *     if (uniform0 + uniform1 == 10)
   *
   * only uniform0 can be inlined due to we exceed the 4 limit. But
   * unless both uniform0 and uniform1 are inlined, can we eliminate
   * the if statement.
   *
   * This is even possible when we deal with loop if the induction
   * variable init and update also contains uniform like
   *
   *    for (i = uniform0; i < uniform1; i+= uniform2)
   *
   * unless uniform0, uniform1 and uniform2 can be inlined at once,
   * can the loop be unrolled.
   */
   if (nir_collect_src_uniforms(cond, component, uni_offsets, new_num,
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
      nir_loop *loop = nir_cf_node_as_loop(node);
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
               default:
            }
      void
   nir_find_inlinable_uniforms(nir_shader *shader)
   {
      uint32_t uni_offsets[MAX_INLINABLE_UNIFORMS];
            nir_foreach_function_impl(impl, shader) {
      nir_metadata_require(impl, nir_metadata_loop_analysis,
            foreach_list_typed(nir_cf_node, node, node, &impl->body)
               for (int i = 0; i < num_offsets[0]; i++)
            }
      void
   nir_inline_uniforms(nir_shader *shader, unsigned num_uniforms,
               {
      if (!num_uniforms)
            nir_foreach_function_impl(impl, shader) {
      nir_builder b = nir_builder_create(impl);
   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
                              /* Only replace UBO 0 with constant offsets. */
   if (intr->intrinsic == nir_intrinsic_load_ubo &&
      nir_src_is_const(intr->src[0]) &&
   nir_src_as_uint(intr->src[0]) == 0 &&
   nir_src_is_const(intr->src[1]) &&
   /* TODO: Can't handle other bit sizes for now. */
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
