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
      #include "vtn_private.h"
   #include "spirv_info.h"
   #include "nir/nir_vla.h"
   #include "util/u_debug.h"
      static unsigned
   glsl_type_count_function_params(const struct glsl_type *type)
   {
      if (glsl_type_is_vector_or_scalar(type)) {
         } else if (glsl_type_is_array_or_matrix(type)) {
      return glsl_get_length(type) *
      } else {
      assert(glsl_type_is_struct_or_ifc(type));
   unsigned count = 0;
   unsigned elems = glsl_get_length(type);
   for (unsigned i = 0; i < elems; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
      }
         }
      static void
   glsl_type_add_to_function_params(const struct glsl_type *type,
               {
      if (glsl_type_is_vector_or_scalar(type)) {
      func->params[(*param_idx)++] = (nir_parameter) {
      .num_components = glsl_get_vector_elements(type),
         } else if (glsl_type_is_array_or_matrix(type)) {
      unsigned elems = glsl_get_length(type);
   const struct glsl_type *elem_type = glsl_get_array_element(type);
   for (unsigned i = 0; i < elems; i++)
      } else {
      assert(glsl_type_is_struct_or_ifc(type));
   unsigned elems = glsl_get_length(type);
   for (unsigned i = 0; i < elems; i++) {
      const struct glsl_type *elem_type = glsl_get_struct_field(type, i);
            }
      static void
   vtn_ssa_value_add_to_call_params(struct vtn_builder *b,
                     {
      if (glsl_type_is_vector_or_scalar(value->type)) {
         } else {
      unsigned elems = glsl_get_length(value->type);
   for (unsigned i = 0; i < elems; i++) {
      vtn_ssa_value_add_to_call_params(b, value->elems[i],
            }
      static void
   vtn_ssa_value_load_function_param(struct vtn_builder *b,
               {
      if (glsl_type_is_vector_or_scalar(value->type)) {
         } else {
      unsigned elems = glsl_get_length(value->type);
   for (unsigned i = 0; i < elems; i++)
         }
      void
   vtn_handle_function_call(struct vtn_builder *b, SpvOp opcode,
         {
      struct vtn_function *vtn_callee =
                     nir_call_instr *call = nir_call_instr_create(b->nb.shader,
                     nir_deref_instr *ret_deref = NULL;
   struct vtn_type *ret_type = vtn_callee->type->return_type;
   if (ret_type->base_type != vtn_base_type_void) {
      nir_variable *ret_tmp =
      nir_local_variable_create(b->nb.impl,
            ret_deref = nir_build_deref_var(&b->nb, ret_tmp);
               for (unsigned i = 0; i < vtn_callee->type->length; i++) {
      vtn_ssa_value_add_to_call_params(b, vtn_ssa_value(b, w[4 + i]),
      }
                     if (ret_type->base_type == vtn_base_type_void) {
         } else {
            }
      static void
   function_decoration_cb(struct vtn_builder *b, struct vtn_value *val, int member,
         {
               switch (dec->decoration) {
   case SpvDecorationLinkageAttributes: {
      unsigned name_words;
   const char *name =
         vtn_fail_if(name_words >= dec->num_operands,
         (void)name; /* TODO: What is this? */
   func->linkage = dec->operands[name_words];
               default:
            }
      bool
   vtn_cfg_handle_prepass_instruction(struct vtn_builder *b, SpvOp opcode,
         {
      switch (opcode) {
   case SpvOpFunction: {
      vtn_assert(b->func == NULL);
            list_inithead(&b->func->body);
   b->func->linkage = SpvLinkageTypeMax;
   b->func->control = w[3];
            UNUSED const struct glsl_type *result_type = vtn_get_type(b, w[1])->type;
   struct vtn_value *val = vtn_push_value(b, w[2], vtn_value_type_function);
                     b->func->type = vtn_get_type(b, w[4]);
                     nir_function *func =
            unsigned num_params = 0;
   for (unsigned i = 0; i < func_type->length; i++)
            /* Add one parameter for the function return value */
   if (func_type->return_type->base_type != vtn_base_type_void)
            func->should_inline = b->func->control & SpvFunctionControlInlineMask;
            func->num_params = num_params;
            unsigned idx = 0;
   if (func_type->return_type->base_type != vtn_base_type_void) {
      nir_address_format addr_format =
         /* The return value is a regular pointer */
   func->params[idx++] = (nir_parameter) {
      .num_components = nir_address_format_num_components(addr_format),
                  for (unsigned i = 0; i < func_type->length; i++)
                           /* Set up a nir_function_impl and the builder so we can load arguments
   * directly in our OpFunctionParameter handler.
   */
   nir_function_impl *impl = nir_function_impl_create(func);
   b->nb = nir_builder_at(nir_before_impl(impl));
                     /* The return value is the first parameter */
   if (func_type->return_type->base_type != vtn_base_type_void)
                     case SpvOpFunctionEnd:
      b->func->end = w;
   if (b->func->start_block == NULL) {
      vtn_fail_if(b->func->linkage != SpvLinkageTypeImport,
                        /* In this case, the function didn't have any actual blocks.  It's
   * just a prototype so delete the function_impl.
   */
      } else {
      vtn_fail_if(b->func->linkage == SpvLinkageTypeImport,
            }
   b->func = NULL;
         case SpvOpFunctionParameter: {
      vtn_assert(b->func_param_idx < b->func->nir_func->num_params);
   struct vtn_type *type = vtn_get_type(b, w[1]);
   struct vtn_ssa_value *value = vtn_create_ssa_value(b, type->type);
   vtn_ssa_value_load_function_param(b, value, &b->func_param_idx);
   vtn_push_ssa_value(b, w[2], value);
               case SpvOpLabel: {
      vtn_assert(b->block == NULL);
   b->block = rzalloc(b, struct vtn_block);
   b->block->label = w;
                     if (b->func->start_block == NULL) {
      /* This is the first block encountered for this function.  In this
   * case, we set the start block and add it to the list of
   * implemented functions that we'll walk later.
   */
   b->func->start_block = b->block;
      }
               case SpvOpSelectionMerge:
   case SpvOpLoopMerge:
      vtn_assert(b->block && b->block->merge == NULL);
   b->block->merge = w;
         case SpvOpBranch:
   case SpvOpBranchConditional:
   case SpvOpSwitch:
   case SpvOpKill:
   case SpvOpTerminateInvocation:
   case SpvOpIgnoreIntersectionKHR:
   case SpvOpTerminateRayKHR:
   case SpvOpEmitMeshTasksEXT:
   case SpvOpReturn:
   case SpvOpReturnValue:
   case SpvOpUnreachable:
      if (b->wa_ignore_return_after_emit_mesh_tasks &&
      opcode == SpvOpReturn && !b->block) {
      /* At this point block was already reset by
   * SpvOpEmitMeshTasksEXT. */
   }
   vtn_assert(b->block && b->block->branch == NULL);
   b->block->branch = w;
   b->block = NULL;
         default:
      /* Continue on as per normal */
                  }
      /* returns the default block */
   void
   vtn_parse_switch(struct vtn_builder *b,
               {
               struct vtn_value *sel_val = vtn_untyped_value(b, branch[1]);
   vtn_fail_if(!sel_val->type ||
                  nir_alu_type sel_type =
         vtn_fail_if(nir_alu_type_get_base_type(sel_type) != nir_type_int &&
                           bool is_default = true;
   const unsigned bitsize = nir_alu_type_get_type_size(sel_type);
   for (const uint32_t *w = branch + 2; w < branch_end;) {
      uint64_t literal = 0;
   if (!is_default) {
      if (bitsize <= 32) {
         } else {
      assert(bitsize == 64);
   literal = vtn_u64_literal(w);
         }
            struct hash_entry *case_entry =
            struct vtn_case *cse;
   if (case_entry) {
         } else {
      cse = rzalloc(b, struct vtn_case);
   cse->block = case_block;
                  list_addtail(&cse->link, case_list);
               if (is_default) {
         } else {
                                 }
      void
   vtn_build_cfg(struct vtn_builder *b, const uint32_t *words, const uint32_t *end)
   {
      vtn_foreach_instruction(b, words, end,
            if (b->shader->info.stage == MESA_SHADER_KERNEL)
               }
      bool
   vtn_handle_phis_first_pass(struct vtn_builder *b, SpvOp opcode,
         {
      if (opcode == SpvOpLabel)
            /* If this isn't a phi node, stop. */
   if (opcode != SpvOpPhi)
            /* For handling phi nodes, we do a poor-man's out-of-ssa on the spot.
   * For each phi, we create a variable with the appropreate type and
   * do a load from that variable.  Then, in a second pass, we add
   * stores to that variable to each of the predecessor blocks.
   *
   * We could do something more intelligent here.  However, in order to
   * handle loops and things properly, we really need dominance
   * information.  It would end up basically being the into-SSA
   * algorithm all over again.  It's easier if we just let
   * lower_vars_to_ssa do that for us instead of repeating it here.
   */
   struct vtn_type *type = vtn_get_type(b, w[1]);
   nir_variable *phi_var =
            struct vtn_value *phi_val = vtn_untyped_value(b, w[2]);
   if (vtn_value_is_relaxed_precision(b, phi_val))
                     vtn_push_ssa_value(b, w[2],
               }
      static bool
   vtn_handle_phi_second_pass(struct vtn_builder *b, SpvOp opcode,
         {
      if (opcode != SpvOpPhi)
                     /* It's possible that this phi is in an unreachable block in which case it
   * may never have been emitted and therefore may not be in the hash table.
   * In this case, there's no var for it and it's safe to just bail.
   */
   if (phi_entry == NULL)
                     for (unsigned i = 3; i < count; i += 2) {
               /* If block does not have end_nop, that is because it is an unreacheable
   * block, and hence it is not worth to handle it */
   if (!pred->end_nop)
                                             }
      void
   vtn_emit_ret_store(struct vtn_builder *b, const struct vtn_block *block)
   {
      if ((*block->branch & SpvOpCodeMask) != SpvOpReturnValue)
            vtn_fail_if(b->func->type->return_type->base_type == vtn_base_type_void,
         struct vtn_ssa_value *src = vtn_ssa_value(b, block->branch[1]);
   const struct glsl_type *ret_type =
         nir_deref_instr *ret_deref =
      nir_build_deref_cast(&b->nb, nir_load_param(&b->nb, 0),
         }
      static struct nir_block *
   vtn_new_unstructured_block(struct vtn_builder *b, struct vtn_function *func)
   {
      struct nir_block *n = nir_block_create(b->shader);
   exec_list_push_tail(&func->nir_func->impl->body, &n->cf_node.node);
   n->cf_node.parent = &func->nir_func->impl->cf_node;
      }
      static void
   vtn_add_unstructured_block(struct vtn_builder *b,
                     {
      if (!block->block) {
      block->block = vtn_new_unstructured_block(b, func);
         }
      static void
   vtn_emit_cf_func_unstructured(struct vtn_builder *b, struct vtn_function *func,
         {
      struct list_head work_list;
            func->start_block->block = nir_start_block(func->nir_func->impl);
   list_addtail(&func->start_block->link, &work_list);
   while (!list_is_empty(&work_list)) {
      struct vtn_block *block =
                           const uint32_t *block_start = block->label;
            b->nb.cursor = nir_after_block(block->block);
   block_start = vtn_foreach_instruction(b, block_start, block_end,
         vtn_foreach_instruction(b, block_start, block_end, handler);
            SpvOp op = *block_end & SpvOpCodeMask;
   switch (op) {
   case SpvOpBranch: {
      struct vtn_block *branch_block = vtn_block(b, block->branch[1]);
   vtn_add_unstructured_block(b, func, &work_list, branch_block);
   nir_goto(&b->nb, branch_block->block);
               case SpvOpBranchConditional: {
      nir_def *cond = vtn_ssa_value(b, block->branch[1])->def;
                  vtn_add_unstructured_block(b, func, &work_list, then_block);
   if (then_block == else_block) {
         } else {
      vtn_add_unstructured_block(b, func, &work_list, else_block);
   nir_goto_if(&b->nb, then_block->block, nir_src_for_ssa(cond),
                           case SpvOpSwitch: {
      struct list_head cases;
                           struct vtn_case *def = NULL;
   vtn_foreach_case(cse, &cases) {
      if (cse->is_default) {
      assert(def == NULL);
                     nir_def *cond = nir_imm_false(&b->nb);
                  /* block for the next check */
                  /* add branching */
   nir_goto_if(&b->nb, cse->block->block, nir_src_for_ssa(cond), e);
                              /* now that all cases are handled, branch into the default block */
   nir_goto(&b->nb, def->block->block);
               case SpvOpKill: {
      nir_discard(&b->nb);
   nir_goto(&b->nb, b->func->nir_func->impl->end_block);
               case SpvOpUnreachable:
   case SpvOpReturn:
   case SpvOpReturnValue: {
      vtn_emit_ret_store(b, block);
   nir_goto(&b->nb, b->func->nir_func->impl->end_block);
               default:
               }
      void
   vtn_function_emit(struct vtn_builder *b, struct vtn_function *func,
         {
      static int force_unstructured = -1;
   if (force_unstructured < 0) {
      force_unstructured =
               nir_function_impl *impl = func->nir_func->impl;
   b->nb = nir_builder_at(nir_after_impl(impl));
   b->func = func;
   b->nb.exact = b->exact;
            if (b->shader->info.stage == MESA_SHADER_KERNEL || force_unstructured) {
      impl->structured = false;
      } else {
                  vtn_foreach_instruction(b, func->start_block->label, func->end,
            if (func->nir_func->impl->structured)
                  /*
   * There are some cases where we need to repair SSA to insert
   * the needed phi nodes:
   *
   * - Early termination instructions `OpKill` and `OpTerminateInvocation`,
   *   in NIR. They're represented by regular intrinsics with no control-flow
   *   semantics. This means that the SSA form from the SPIR-V may not
   *   100% match NIR.
   *
   * - Switches with only default case may also define SSA which may
   *   subsequently be used out of the switch.
   */
   if (func->nir_func->impl->structured)
               }
