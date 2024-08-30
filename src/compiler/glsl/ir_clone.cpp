   /*
   * Copyright © 2010 Intel Corporation
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
   * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   * DEALINGS IN THE SOFTWARE.
   */
      #include <string.h>
   #include "util/compiler.h"
   #include "ir.h"
   #include "compiler/glsl_types.h"
   #include "util/hash_table.h"
      ir_rvalue *
   ir_rvalue::clone(void *mem_ctx, struct hash_table *) const
   {
      /* The only possible instantiation is the generic error value. */
      }
      /**
   * Duplicate an IR variable
   */
   ir_variable *
   ir_variable::clone(void *mem_ctx, struct hash_table *ht) const
   {
      ir_variable *var = new(mem_ctx) ir_variable(this->type, this->name,
            var->data.max_array_access = this->data.max_array_access;
   if (this->is_interface_instance()) {
      var->u.max_ifc_array_access =
         memcpy(var->u.max_ifc_array_access, this->u.max_ifc_array_access,
                        if (this->get_state_slots()) {
      ir_state_slot *s = var->allocate_state_slots(this->get_num_state_slots());
   memcpy(s, this->get_state_slots(),
               if (this->constant_value)
            if (this->constant_initializer)
      this->constant_initializer->clone(mem_ctx, ht);
                  if (ht)
               }
      ir_swizzle *
   ir_swizzle::clone(void *mem_ctx, struct hash_table *ht) const
   {
         }
      ir_return *
   ir_return::clone(void *mem_ctx, struct hash_table *ht) const
   {
               if (this->value)
               }
      ir_discard *
   ir_discard::clone(void *mem_ctx, struct hash_table *ht) const
   {
               if (this->condition != NULL)
               }
      ir_demote *
   ir_demote::clone(void *mem_ctx, struct hash_table *ht) const
   {
         }
      ir_loop_jump *
   ir_loop_jump::clone(void *mem_ctx, struct hash_table *ht) const
   {
                  }
      ir_if *
   ir_if::clone(void *mem_ctx, struct hash_table *ht) const
   {
               foreach_in_list(ir_instruction, ir, &this->then_instructions) {
                  foreach_in_list(ir_instruction, ir, &this->else_instructions) {
                     }
      ir_loop *
   ir_loop::clone(void *mem_ctx, struct hash_table *ht) const
   {
               foreach_in_list(ir_instruction, ir, &this->body_instructions) {
                     }
      ir_call *
   ir_call::clone(void *mem_ctx, struct hash_table *ht) const
   {
      ir_dereference_variable *new_return_ref = NULL;
   if (this->return_deref != NULL)
                     foreach_in_list(ir_instruction, ir, &this->actual_parameters) {
                     }
      ir_expression *
   ir_expression::clone(void *mem_ctx, struct hash_table *ht) const
   {
      ir_rvalue *op[ARRAY_SIZE(this->operands)] = { NULL, };
            for (i = 0; i < num_operands; i++) {
                  return new(mem_ctx) ir_expression(this->operation, this->type,
      }
      ir_dereference_variable *
   ir_dereference_variable::clone(void *mem_ctx, struct hash_table *ht) const
   {
               if (ht) {
      hash_entry *entry = _mesa_hash_table_search(ht, this->var);
      } else {
                     }
      ir_dereference_array *
   ir_dereference_array::clone(void *mem_ctx, struct hash_table *ht) const
   {
      return new(mem_ctx) ir_dereference_array(this->array->clone(mem_ctx, ht),
            }
      ir_dereference_record *
   ir_dereference_record::clone(void *mem_ctx, struct hash_table *ht) const
   {
      assert(this->field_idx >= 0);
   const char *field_name =
         return new(mem_ctx) ir_dereference_record(this->record->clone(mem_ctx, ht),
      }
      ir_texture *
   ir_texture::clone(void *mem_ctx, struct hash_table *ht) const
   {
      ir_texture *new_tex = new(mem_ctx) ir_texture(this->op, this->is_sparse);
            new_tex->sampler = this->sampler->clone(mem_ctx, ht);
   if (this->coordinate)
         if (this->projector)
         if (this->shadow_comparator)
         if (this->clamp)
            if (this->offset != NULL)
            switch (this->op) {
   case ir_tex:
   case ir_lod:
   case ir_query_levels:
   case ir_texture_samples:
   case ir_samples_identical:
         case ir_txb:
      new_tex->lod_info.bias = this->lod_info.bias->clone(mem_ctx, ht);
      case ir_txl:
   case ir_txf:
   case ir_txs:
      new_tex->lod_info.lod = this->lod_info.lod->clone(mem_ctx, ht);
      case ir_txf_ms:
      new_tex->lod_info.sample_index = this->lod_info.sample_index->clone(mem_ctx, ht);
      case ir_txd:
      new_tex->lod_info.grad.dPdx = this->lod_info.grad.dPdx->clone(mem_ctx, ht);
   new_tex->lod_info.grad.dPdy = this->lod_info.grad.dPdy->clone(mem_ctx, ht);
      case ir_tg4:
      new_tex->lod_info.component = this->lod_info.component->clone(mem_ctx, ht);
                  }
      ir_assignment *
   ir_assignment::clone(void *mem_ctx, struct hash_table *ht) const
   {
      return new(mem_ctx) ir_assignment(this->lhs->clone(mem_ctx, ht),
            }
      ir_function *
   ir_function::clone(void *mem_ctx, struct hash_table *ht) const
   {
               copy->is_subroutine = this->is_subroutine;
   copy->subroutine_index = this->subroutine_index;
   copy->num_subroutine_types = this->num_subroutine_types;
   copy->subroutine_types = ralloc_array(mem_ctx, const struct glsl_type *, copy->num_subroutine_types);
   for (int i = 0; i < copy->num_subroutine_types; i++)
            foreach_in_list(const ir_function_signature, sig, &this->signatures) {
      ir_function_signature *sig_copy = sig->clone(mem_ctx, ht);
            if (ht != NULL) {
      _mesa_hash_table_insert(ht,
                     }
      ir_function_signature *
   ir_function_signature::clone(void *mem_ctx, struct hash_table *ht) const
   {
                        /* Clone the instruction list.
   */
   foreach_in_list(const ir_instruction, inst, &this->body) {
      ir_instruction *const inst_copy = inst->clone(mem_ctx, ht);
                  }
      ir_function_signature *
   ir_function_signature::clone_prototype(void *mem_ctx, struct hash_table *ht) const
   {
      ir_function_signature *copy =
            copy->is_defined = false;
   copy->builtin_avail = this->builtin_avail;
            /* Clone the parameter list, but NOT the body.
   */
   foreach_in_list(const ir_variable, param, &this->parameters) {
               ir_variable *const param_copy = param->clone(mem_ctx, ht);
                  }
      ir_constant *
   ir_constant::clone(void *mem_ctx, struct hash_table *ht) const
   {
               switch (this->type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_FLOAT16:
   case GLSL_TYPE_DOUBLE:
   case GLSL_TYPE_BOOL:
   case GLSL_TYPE_UINT64:
   case GLSL_TYPE_INT64:
   case GLSL_TYPE_UINT16:
   case GLSL_TYPE_INT16:
   case GLSL_TYPE_UINT8:
   case GLSL_TYPE_INT8:
   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_TEXTURE:
   case GLSL_TYPE_IMAGE:
            case GLSL_TYPE_STRUCT:
   case GLSL_TYPE_ARRAY: {
               c->type = this->type;
   c->const_elements = ralloc_array(c, ir_constant *, this->type->length);
   for (unsigned i = 0; i < this->type->length; i++) {
         }
               case GLSL_TYPE_ATOMIC_UINT:
   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   case GLSL_TYPE_SUBROUTINE:
   case GLSL_TYPE_INTERFACE:
      assert(!"Should not get here.");
         case GLSL_TYPE_COOPERATIVE_MATRIX:
                     }
         class fixup_ir_call_visitor : public ir_hierarchical_visitor {
   public:
      fixup_ir_call_visitor(struct hash_table *ht)
   {
                  virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      /* Try to find the function signature referenced by the ir_call in the
   * table.  If it is found, replace it with the value from the table.
   */
   ir_function_signature *sig;
            if (entry != NULL) {
      sig = (ir_function_signature *) entry->data;
               /* Since this may be used before function call parameters are flattened,
   * the children also need to be processed.
   */
            private:
         };
         static void
   fixup_function_calls(struct hash_table *ht, exec_list *instructions)
   {
      fixup_ir_call_visitor v(ht);
      }
         void
   clone_ir_list(void *mem_ctx, exec_list *out, const exec_list *in)
   {
               foreach_in_list(const ir_instruction, original, in) {
                           /* Make a pass over the cloned tree to fix up ir_call nodes to point to the
   * cloned ir_function_signature nodes.  This cannot be done automatically
   * during cloning because the ir_call might be a forward reference (i.e.,
   * the function signature that it references may not have been cloned yet).
   */
               }
